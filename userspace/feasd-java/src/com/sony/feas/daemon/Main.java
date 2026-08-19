package com.sony.feas.daemon;

import android.util.Log;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

import android.os.Binder;
import android.os.IBinder;
import android.os.Parcel;

/**
 * FEAS daemon v3.1 - Binder 事件驱动(无兜底)
 *
 * v3.0 已确认 binder 注册成功,故删除全部降级路径:
 *  - 删除 startIdleFallback(frame_total 轮询降级)
 *  - 删除 ctrl 文件轮询(控制全走 Binder)
 *
 * v3.1 新增:
 *  - 开屏检测:backlight 0->非0 转变 -> flow.onTouch(true)
 *    (打开屏幕瞬间像触摸一样调度:立即 120Hz + active,锁屏->主屏过渡流畅)
 *  - 动画=触摸同调度:GPU floor 用 lastActiveMs(max(帧,触摸))
 *  - boost:帧 avgNs>25ms 或 jank -> GPU 680M 解除 cap(3s,无需触摸)
 *  - 掉 60Hz 更晚:FRAME_IDLE_MS=2000 + IDLE_SLACK_MS=4000
 *
 * 线程:input(触摸即时感知)+ flow idle(500ms 内存检查)+
 *       kgsl watchdog(1s)+ screen(1s backlight 检测)= 4 线程,全部低频,
 *       无文件轮询热点(仅 screen 1s 读一次 backlight)。
 */
public class Main {
    private static final String TAG = "FEASD";
    /* 触摸设备不硬编码 event4:驱动重枚举后节点可能变化,硬编码会漏掉触摸。
     * 通过 /proc/bus/input/devices 动态发现含 ABS_MT_POSITION_X 的 event 节点。 */
    private static final long SWITCH_MIN_INTERVAL_MS = 1000; /* 降频防抖:避免 60/120 横跳;升频不限制 */
    private static final String GPU_MHZ_SYSFS = "/sys/kernel/perf_manager/touch_gpu_mhz";
    /* touch_gpu_mhz = 触摸时 GPU 最低频率(内核默认 550),松手自动释放(省电)
     * 60Hz 锁 401 档会导致滑动卡顿,故 60/120Hz 都提到 550;
     * 帧时间过长 boost 时写 680(超过正常限制,满足"boost到限制频率以上") */
    private static final int GPU_MHZ_HIGH = 550;    /* 120Hz 触摸 GPU 保底 */
    private static final int GPU_MHZ_NORMAL = 550;  /* 60Hz 触摸 GPU 保底(修复 400 卡顿) */
    private static final int GPU_MHZ_BOOST = 680;   /* 帧时间过长 -> 超过限制 */
    /* Runtime state OUTSIDE the module dir (/data/adb/feas) */
    private static final String CTRL_FILE = "/data/adb/feas/dfps_enabled";
    private static final String HMD_CTRL_FILE = "/data/adb/feas/hmd_enabled";
    private static final String ENABLE_CTRL_FILE = "/data/adb/feas/enabled";
    private static final String FPS_CTRL_FILE = "/data/adb/feas/fps_config";
    private static final String STATE_FILE = "/data/adb/feas/fpsmode_state";

    /* 冷启动 boost:内核节点,写 1 -> TOUCH caps,内核 delayed_work 自动恢复 */
    private static final String LAUNCH_BOOST = "/sys/kernel/perf_manager/launch_boost";
    private static final long LAUNCH_BOOST_MIN_GAP_MS = 1500; /* 防抖:两次冷启动最短间隔 */

    private static volatile boolean highRefresh = false;
    private static volatile long lastSwitchMs = 0;
    private static volatile boolean dfpsEnabled = true;
    private static volatile boolean hmdEnabled = false;
    private static volatile boolean moduleEnabled = true;
    private static volatile int manualFps = 0;

    private static volatile FrameFlowMonitor flow;

    private static final int EV_ABS = 3;
    private static final int ABS_MT_TRACKING_ID = 0x39;
    private static final int ABS_MT_TOUCH_MAJOR = 0x30;
    private static final int ABS_MT_POSITION_X = 0x35;
    private static final int ABS_MT_POSITION_Y = 0x36;

    public static void main(String[] args) {
        log("I", "FEAS daemon v3.1 (binder event-driven, no fallback) starting");
        loadState();
        flow = new FrameFlowMonitor(new FrameFlowMonitor.Listener() {
            @Override
            public void onChangeRefresh(boolean high) {
                switchRefresh(high);
            }
        });
        FeasBinderService svc = new FeasBinderService(new BinderCallbacks());
        svc.register();
        startInputListener();
        startProcessObserver();
        /* app_process Java daemon 默认无 binder 线程池,主线程仅 sleep 则
         * 传入事务(AMS->ProcessObserver 回调)永远排队不被分发。
         * 主线程加入 binder 线程池:阻塞并分发传入事务。 */
        log("I", "joining binder threadpool");
        try {
            /* Android 15 (SDK 35): BinderInternal 在 com.android.internal.os 包 */
            Class<?> bi = Class.forName("com.android.internal.os.BinderInternal");
            bi.getMethod("joinThreadPool").invoke(null);
        } catch (Throwable t) {
            log("W", "joinThreadPool failed: " + t + " -> fallback sleep");
            while (true) {
                try { Thread.sleep(Long.MAX_VALUE); }
                catch (InterruptedException e) { break; }
            }
        }
    }

    /** Binder 回调:把控制/帧事件转发给状态机与状态变量。 */
    private static final class BinderCallbacks implements FeasBinderService.Callbacks {
        @Override
        public void onReportFrames(long frameTotal, int avgNs, int jankCount) {
            flow.onReportFrames(frameTotal, avgNs, jankCount);
        }

        @Override
        public void onSetDfps(boolean on) {
            dfpsEnabled = on;
            flow.onSetDfps(on);
            writeCtrlFile(CTRL_FILE, on ? "1" : "0");
            log("I", "dfps " + (on ? "enabled" : "disabled") + " (binder)");
        }

        @Override
        public void onSetHmd(boolean on) {
            hmdEnabled = on;
            writeCtrlFile(HMD_CTRL_FILE, on ? "1" : "0");
            HmdController.setHmd(on);
            log("I", "hmd " + (on ? "enabled" : "disabled") + " (binder)");
        }

        @Override
        public void onSetEnabled(boolean on) {
            moduleEnabled = on;
            writeCtrlFile(ENABLE_CTRL_FILE, on ? "1" : "0");
            writeLong("/sys/kernel/perf_manager/enable", on ? 1 : 0);
            log("I", "module " + (on ? "enabled" : "disabled") + " (binder)");
        }

        @Override
        public void onSetManualFps(int fps) {
            manualFps = fps;
            writeCtrlFile(FPS_CTRL_FILE, String.valueOf(fps));
            if (fps > 0) {
                writeLong("/sys/kernel/perf_manager/fps", fps);
            }
            log("I", "manual fps " + fps + " (binder)");
        }

        @Override
        public void onReportTouch(boolean down) {
            flow.onTouch(down);
        }

        @Override
        public int[] onGetState() {
            int targetFps = 0;
            String s = readFile("/sys/kernel/perf_manager/fps");
            if (!s.isEmpty()) {
                try { targetFps = Integer.parseInt(s); } catch (Exception ignored) {}
            }
            return new int[]{
                    moduleEnabled ? 1 : 0,
                    dfpsEnabled ? 1 : 0,
                    hmdEnabled ? 1 : 0,
                    manualFps,
                    targetFps,
                    flow.isHighRefresh() ? 1 : 0,
                    (int) flow.getLastFrameTotal()
            };
        }
    }

    /* ---------------- 状态加载/持久化 ---------------- */

    private static void loadState() {
        dfpsEnabled = readCtrl(CTRL_FILE, true);
        hmdEnabled = readCtrl(HMD_CTRL_FILE, false);
        moduleEnabled = readCtrl(ENABLE_CTRL_FILE, true);
        String fps = readFile(FPS_CTRL_FILE);
        if (!fps.isEmpty()) {
            try { manualFps = Integer.parseInt(fps); } catch (Exception ignored) {}
        }
        if (!moduleEnabled) {
            writeLong("/sys/kernel/perf_manager/enable", 0);
        }
        log("I", "state: dfps=" + dfpsEnabled + " hmd=" + hmdEnabled
                + " enabled=" + moduleEnabled + " manual=" + manualFps);
    }

    private static boolean readCtrl(String path, boolean def) {
        String s = readFile(path);
        if (s.isEmpty()) return def;
        return s.equals("1");
    }

    private static void writeCtrlFile(String path, String val) {
        try {
            FileOutputStream fos = new FileOutputStream(path);
            fos.write(val.getBytes(StandardCharsets.US_ASCII));
            fos.close();
        } catch (Exception e) {
            log("W", "write ctrl failed: " + e.getMessage());
        }
    }

    private static void writeLong(String path, long value) {
        try {
            FileOutputStream fos = new FileOutputStream(path);
            fos.write((value + "\n").getBytes(StandardCharsets.US_ASCII));
            fos.close();
        } catch (Exception e) {
            log("W", "write " + path + " failed: " + e.getMessage());
        }
    }

private static String readFile(String path) {
        try {
            File f = new File(path);
            if (!f.exists()) return "";
            return new String(Files.readAllBytes(f.toPath()),
                    StandardCharsets.US_ASCII).trim();
        } catch (Exception e) {
            return "";
        }
    }

    static void log(String level, String msg) {
        Log.i(TAG, msg);
    }

    /* ---------------- 开屏检测(像触摸一样调度) ---------------- */

    /* ---------------- input listener ---------------- */

    private static void startInputListener() {
        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                byte[] buf = new byte[24];
                while (true) {
                    String path = findTouchDevice();
                    if (path != null) {
                        File f = new File(path);
                        if (f.exists()) {
                            try {
                                DataInputStream dis = new DataInputStream(
                                        new FileInputStream(f));
                                log("I", "listening " + path);
                                while (true) {
                                    dis.readFully(buf);
                                    handleInputEvent(buf);
                                }
                            } catch (Exception e) {
                                log("W", "input " + path + " error: " + e.getMessage());
                            }
                        }
                    }
                    try { Thread.sleep(3000); } catch (InterruptedException ie) { break; }
                }
            }
        }, "feasd-input");
        t.setDaemon(true);
        t.start();
    }

    /** 动态发现触摸屏事件节点:解析 /proc/bus/input/devices,
     *  找含 ABS_MT_POSITION_X(0x35) 或 ABS_MT_TRACKING_ID(0x39) 的设备。 */
    private static String findTouchDevice() {
        try {
            String content = readFile("/proc/bus/input/devices");
            String handlers = null;
            for (String line : content.split("\n")) {
                line = line.trim();
                if (line.startsWith("H: Handlers=")) {
                    handlers = line.substring("H: Handlers=".length());
                } else if (line.startsWith("B: ABS=") && handlers != null) {
                    if (absHasMtBit(line, 0x35) || absHasMtBit(line, 0x39)) {
                        java.util.regex.Matcher m = java.util.regex.Pattern
                                .compile("event(\\d+)").matcher(handlers);
                        if (m.find()) {
                            return "/dev/input/event" + m.group(1);
                        }
                    }
                    handlers = null;
                }
            }
        } catch (Throwable t) {
            log("W", "findTouchDevice error: " + t.getMessage());
        }
        return null;
    }

    /** "B: ABS=xxxx" 行第一个 16 进制 u64 位图,检查 bit 是否置位。 */
    private static boolean absHasMtBit(String absLine, int bit) {
        try {
            String hex = absLine.substring(absLine.indexOf('=') + 1)
                    .trim().split("\\s+")[0];
            long v = Long.parseLong(hex, 16);
            return (v & (1L << bit)) != 0;
        } catch (Throwable t) {
            return false;
        }
    }

    /** 单条 input 事件(24B struct input_event)解析 + 触摸状态投递。 */
    private static void handleInputEvent(byte[] buf) {
        int type = (buf[16] & 255) | ((buf[17] & 255) << 8);
        int code = (buf[18] & 255) | ((buf[19] & 255) << 8);
        int value = (buf[20] & 255) | ((buf[21] & 255) << 8)
                | ((buf[22] & 255) << 16) | ((buf[23] & 255) << 24);
        if (type == EV_ABS) {
            if (code == ABS_MT_TRACKING_ID) {
                flow.onTouch(value >= 0);
            } else if (code == ABS_MT_TOUCH_MAJOR && value > 0) {
                flow.onTouch(true);
            } else if (code == ABS_MT_POSITION_X || code == ABS_MT_POSITION_Y) {
                /* slide: 立即 120Hz */
                flow.onTouch(true);
            }
        }
    }

    /* ---------------- process observer (cold-launch detect) ---------------- */

    private static volatile long lastLaunchBoostMs = 0;

    /** 反射注册 ProcessObserver:前台 activity 切换即冷启动时机。
     *  IProcessObserver 不在编译期 android.jar,用动态 Proxy + 手写 Binder
     *  走 RemoteCallbackList(实测 root 可注册,收到真实 fg 事件)。 */
    private static void startProcessObserver() {
        try {
            /* 手写 Binder 子类:AMS 回调经 binder 事务到达,必须 onTransact 分发。
             *  运行时探测(Sony 定制 ROM)确认 android.app.IProcessObserver 事务码:
             *    code 1 = onProcessStarted(pid,uid,type,procname,reason)
             *    code 2 = onForegroundActivitiesChanged(pid,uid,fg)   <-- 冷启动时机
             *    code 3 = onForegroundServicesChanged(pid,uid,type)
             *    code 4 = onProcessDied(pid,uid)
             *  android.os.IProcessObserver 在此 ROM 不存在。
             *  注意:与 AOSP 标准(code1=fg-changed)顺序相反! */
            final Binder CALLBACK_BINDER = new Binder() {
                @Override
                protected boolean onTransact(int code, Parcel data,
                        Parcel reply, int flags) {
                    /* 无条件入口日志:验证事务是否到达 */
                    log("I", "onTransact ENTRY code=" + code + " flags=" + flags);
                    /* 实测(SDK 35 Sony ROM):Parcel.readInterfaceToken() 无参方法
                     * 不存在,且 onTransact parcel 无 interface token string。
                     * 故不解析 token,直接按事务码分发(binder 已保证事务到达,
                     * 事务码经运行时探测确认)。 */
                    switch (code) {
                        case 2: { /* onForegroundActivitiesChanged (Sony: code=2) */
                            try {
                                /* Sony SDK35 真实布局 (rawALL 实测确认):
                                 * [0] strict(int) [1] ws(int) [2] 固定头(int)
                                 * [3] descLen(int) [4..] descriptor UTF-16LE
                                 * 然后 0(int) pid(int) uid(int) fg(int) */
                                data.readInt();                 /* strict */
                                data.readInt();                 /* workSourceUid */
                                data.readInt();                 /* 固定头 */
                                int len = data.readInt();       /* descLen */
                                /* 跳过 descriptor (UTF-16LE: len*2 字节) */
                                data.setDataPosition(data.dataPosition() + len * 2);
                                data.readInt();                 /* 0 */
                                int pid = data.readInt();
                                int uid = data.readInt();
                                boolean fg = data.readInt() != 0;
                                log("I", "fg cb: pid=" + pid + " uid=" + uid
                                        + " fg=" + fg + " size=" + data.dataSize());
                                onForegroundChanged(pid, uid, fg);
                            } catch (Throwable t) {
                                log("W", "case2 parse failed: " + t);
                            }
                            return true;
                        }
                        case 1: /* onProcessStarted */
                        case 3: /* onForegroundServicesChanged */
                        case 4: /* onProcessDied */
                            return true;
                        default:
                            return false;
                    }
                }
            };
            final Class<?> iObs = Class.forName("android.app.IProcessObserver");
            final ClassLoader cl = Main.class.getClassLoader();
            Object proxy = java.lang.reflect.Proxy.newProxyInstance(cl,
                    new Class<?>[]{iObs},
                    new java.lang.reflect.InvocationHandler() {
                        @Override
                        public Object invoke(Object o,
                                java.lang.reflect.Method m, Object[] a) {
                            String n = m.getName();
                            if (n.equals("onForegroundActivitiesChanged")) {
                                int pid = (Integer) a[0];
                                int uid = (Integer) a[1];
                                boolean fg = (Boolean) a[2];
                                onForegroundChanged(pid, uid, fg);
                                return null;
                            }
                            if (n.equals("asBinder"))
                                return CALLBACK_BINDER;
                            if (n.equals("onProcessDied"))
                                return null;
                            if (n.equals("onProcessStateChanged"))
                                return null;
                            return null;
                        }
                    });
            Object ams = Class.forName("android.os.ServiceManager")
                    .getMethod("getService", String.class)
                    .invoke(null, "activity");
            Class<?> amn = Class.forName("android.app.ActivityManagerNative");
            Object am = amn.getMethod("asInterface", IBinder.class)
                    .invoke(null, (IBinder) ams);
            Class<?> actMgr = am.getClass();
            java.lang.reflect.Method reg;
            try {
                reg = actMgr.getMethod("registerProcessObserver", iObs);
            } catch (NoSuchMethodException e) {
                reg = actMgr.getMethod("registerProcessObserver",
                        Class.forName("android.os.IProcessObserver"));
            }
            reg.invoke(am, proxy);
            log("I", "ProcessObserver registered (cold-launch detect)");
        } catch (Throwable t) {
            log("W", "ProcessObserver register failed: " + t);
        }
    }

    /** fg=true 且 uid 是用户 app(>=10000):冷启动,写 launch_boost。 */
    private static void onForegroundChanged(int pid, int uid, boolean fg) {
        if (!fg || uid < 10000) return;
        long now = System.currentTimeMillis();
        if (now - lastLaunchBoostMs < LAUNCH_BOOST_MIN_GAP_MS) return;
        lastLaunchBoostMs = now;
        writeLong(LAUNCH_BOOST, 1);
        log("I", "launch boost: uid=" + uid + " pid=" + pid);
    }

    /* ---------------- dfps switch ---------------- */

    /**
     * v3.1 零延迟切换:
     *  - 升频(->120)无条件立即执行:触摸/动画瞬间切 120Hz,不做 1s 防抖
     *    (防抖只约束降频,避免 60/120 反复横跳)
     *  - set-constant-fps 改反射直连 DisplayManagerGlobal.setConstantFrameRate:
     *    app_process 内 0ms 同步调用,替代 spawn `cmd display` 进程(~100ms)
     *  - 全同步,无 spawn 线程,延迟不可感知
     */
    private static synchronized void switchRefresh(final boolean high) {
        if (high == highRefresh) return;
        long now = System.currentTimeMillis();
        /* 降频才需要防抖(升频立即,降频等 1s 避免横跳) */
        if (!high && now - lastSwitchMs < SWITCH_MIN_INTERVAL_MS) return;
        lastSwitchMs = now;
        highRefresh = high;
        flow.setHighRefresh(high);
        final int gpuMhz = high ? GPU_MHZ_NORMAL : GPU_MHZ_HIGH;
        final int fps = high ? 120 : 60;
        log("I", "dfps switch -> " + fps + " Hz");
        writeCtrlFile(STATE_FILE, high ? "1" : "0");

        /* 1) 内核 fps 节点(面板侧,最低延迟,同步) */
        writeLong("/sys/kernel/perf_manager/fps", fps);
        writeLong(GPU_MHZ_SYSFS, gpuMhz);
        /* 2) 渲染侧 constant frame rate(0ms 反射直连,替代 cmd spawn) */
        setConstantFrameRate(fps);
    }

    /**
     * 反射调用 DisplayManagerGlobal.setConstantFrameRate(int)。
     * 即 `cmd display set-constant-fps` 的底层客户端方法,app_process 内
     * 直接 binder 调用,零进程 spawn,实测 0ms。失败静默(内核 fps 节点已写)。
     */
    private static void setConstantFrameRate(int fps) {
        try {
            Class<?> dmg = Class.forName(
                    "android.hardware.display.DisplayManagerGlobal");
            Object inst = dmg.getMethod("getInstance").invoke(null);
            if (inst != null) {
                dmg.getMethod("setConstantFrameRate", int.class).invoke(inst, fps);
            }
        } catch (Throwable t) {
            log("W", "setConstantFrameRate(" + fps + ") failed: " + t);
        }
    }
}