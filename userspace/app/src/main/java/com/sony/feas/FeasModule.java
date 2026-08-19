package com.sony.feas;

import android.util.Log;

import java.io.FileOutputStream;
import java.lang.reflect.Method;

import io.github.libxposed.api.XposedInterface;
import io.github.libxposed.api.XposedModule;
import io.github.libxposed.api.XposedModuleInterface.PackageReadyParam;
import io.github.libxposed.api.XposedModuleInterface.SystemServerStartingParam;

/**
 * FEAS 模块入口(libxposed API 102,Vector 框架)。v3.0 Binder 重构。
 *
 * 完整功能:
 *  - hook onVsync 检测帧(FrameTimeTracker 统一测量,EMA 自适应 jank)
 *  - 双通道上报:
 *      * 内核 sysfs frame(avgNs,驱动 perf_anim_active/CPU 调度,不可移除)
 *      * Binder REPORT_FRAMES(total, avgNs, jankCount) -> feasd
 *        (dfps 决策 + jank 升频,jank 不再被批平均抹平)
 *  - 目标帧率:手动优先;自动模式动态跟随实际刷新率
 *  - 分辨率解锁(system_server + Settings,见 ResolutionUnlock)
 *  - 统计供 UI 显示
 */
public class FeasModule extends XposedModule {

    private static final String TAG = "FEAS";
    private static final String FRAME_SYSFS = "/sys/kernel/perf_manager/frame";
    private static final String FRAME_TOTAL_SYSFS = "/sys/kernel/perf_manager/frame_total";
    private static final String FPS_SYSFS = "/sys/kernel/perf_manager/fps";

    private static volatile FileOutputStream frameWriter;
    private static volatile boolean writerFailed = false;

    /* 统一帧时间测量(Choreographer 线程热路径,见 FrameTimeTracker) */
    private static volatile FrameTimeTracker tracker;

    private static final String PROCESS_SYSTEM_UI = "com.android.systemui";
    private static final String PROCESS_LAUNCHER = "com.sony.sonyericsson.home";
    private static final String PROCESS_SETTINGS = "com.android.settings";
    private static boolean hooksStarted = false;

    @Override
    public void onModuleLoaded(ModuleLoadedParam param) {
        Log.i(TAG, "FEAS v3.0 模块已加载, api=" + getApiVersion()
                + ", process=" + param.getProcessName());
        trace("onModuleLoaded process=" + param.getProcessName()
                + " isSystemServer=" + param.isSystemServer());
    }

    /** 文件追踪(system_server 的 logcat 输出不可见时用于定位生命周期)。 */
    private static void trace(String msg) {
        try {
            java.io.FileOutputStream fos = new java.io.FileOutputStream(
                    "/data/system/feas_bootres.log", true);
            fos.write((System.currentTimeMillis() + " " + msg + "\n")
                    .getBytes("UTF-8"));
            fos.close();
        } catch (Throwable ignored) {
        }
    }

    @Override
    public void onSystemServerStarting(SystemServerStartingParam param) {
        trace("onSystemServerStarting called");
        Log.i(TAG, "system_server starting, 安装分辨率解锁 hooks");
        try {
            ResolutionUnlock.hookSystemServer(this, param.getClassLoader());
            Log.i(TAG, "system_server 分辨率 hooks 已安装");
        } catch (Throwable t) {
            Log.e(TAG, "system_server 分辨率 hooks 安装失败", t);
        }
        scheduleBootResolutionRestore();
    }

    /**
     * 开机恢复用户分辨率(pdx234 机制源码级复刻,不依赖 PersistentDataStore)。
     * 时机:等待 sys.boot_completed 后执行(bootanimation 期间,显示栈早期)。
     * v3.0:轮询收紧为 60×500ms(30s 上限),恢复更早完成,避免无谓等待。
     */
    private void scheduleBootResolutionRestore() {
        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    trace("bootrest thread start");
                    boolean booted = false;
                    Class<?> sp = Class.forName("android.os.SystemProperties");
                    for (int i = 0; i < 60; i++) {
                        try {
                            String v = (String) sp.getMethod("get", String.class)
                                    .invoke(null, "sys.boot_completed");
                            if ("1".equals(v)) { booted = true; break; }
                        } catch (Throwable ignored) {
                        }
                        Thread.sleep(500);
                    }
                    trace("bootrest boot_completed=" + booted);
                    if (!booted) return;

                    Object app = Class.forName("android.app.ActivityThread")
                            .getMethod("currentApplication").invoke(null);
                    if (app == null) return;
                    android.content.ContentResolver cr = ((android.content.Context) app)
                            .getContentResolver();
                    String w = android.provider.Settings.Global.getString(
                            cr, "user_preferred_resolution_width");
                    String h = android.provider.Settings.Global.getString(
                            cr, "user_preferred_resolution_height");
                    String r = android.provider.Settings.Global.getString(
                            cr, "user_preferred_refresh_rate");
                    trace("bootrest prefs w=" + w + " h=" + h + " r=" + r);
                    if (w == null || h == null) return;
                    int wantW = Integer.parseInt(w);
                    int wantH = Integer.parseInt(h);
                    float rate = (r == null) ? 120f : Float.parseFloat(r);

                    Class<?> dmg = Class.forName(
                            "android.hardware.display.DisplayManagerGlobal");
                    Object inst = dmg.getMethod("getInstance").invoke(null);

                    Object info = dmg.getMethod("getDisplayInfo", int.class)
                            .invoke(inst, 0);
                    java.lang.reflect.Field sf = info.getClass().getField("supportedModes");
                    Object[] modes = (Object[]) sf.get(info);
                    Object targetMode = null;
                    int modeId = -1;
                    for (Object m : modes) {
                        int mw = (Integer) m.getClass()
                                .getMethod("getPhysicalWidth").invoke(m);
                        float mr = (Float) m.getClass()
                                .getMethod("getRefreshRate").invoke(m);
                        if (mw == wantW && Math.abs(mr - rate) < 1f) {
                            targetMode = m;
                            modeId = (Integer) m.getClass()
                                    .getMethod("getModeId").invoke(m);
                            break;
                        }
                    }
                    if (targetMode == null) {
                        trace("bootrest NO MATCHING MODE for " + wantW + "x"
                                + wantH + "@" + rate);
                        return;
                    }
                    trace("bootrest target modeId=" + modeId);
                    android.provider.Settings.System.putString(cr,
                            "user_selected_resolution", wantW + "x" + wantH);
                    trace("bootrest user_selected_resolution=" + wantW + "x" + wantH);

                    if (modeId > 0) {
                        java.lang.reflect.Method req = dmg.getMethod(
                                "requestDisplayModes", int.class, int[].class);
                        req.invoke(inst, 0, new int[]{modeId});
                        trace("bootrest requestDisplayModes modeId=" + modeId);
                    }

                    java.lang.reflect.Method set = dmg.getMethod(
                            "setUserPreferredDisplayMode", int.class,
                            targetMode.getClass());
                    set.invoke(inst, 0, targetMode);
                    trace("bootrest applied " + wantW + "x" + wantH + "@" + rate);
                } catch (Throwable t) {
                    trace("bootrest FAILED: " + t);
                }
            }
        }, "feas-bootres");
        t.setDaemon(true);
        t.start();
    }

    @Override
    public void onPackageReady(PackageReadyParam param) {
        String packageName = param.getPackageName();

        // 分辨率解锁:Settings 进程(显示设置页)
        if (PROCESS_SETTINGS.equals(packageName)) {
            Log.i(TAG, "Settings ready, 安装分辨率选项 hooks");
            try {
                ResolutionUnlock.hookSettings(this, param.getClassLoader());
                Log.i(TAG, "Settings 分辨率 hooks 已安装");
            } catch (Throwable t) {
                Log.e(TAG, "Settings 分辨率 hooks 安装失败", t);
            }
            return;
        }

        // 帧上报:挂钩所有 UI 进程(scope.list 限定注入范围,Settings 与
        // system_server 已在上方 return)。每个进程注入独立的静态状态,
        // 任一进程渲染都持续上报 -> feasd 跨进程保持 120Hz。
        if (hooksStarted) {
            return;
        }
        hooksStarted = true;

        Log.i(TAG, packageName + " ready, hooking onVsync");
        try {
            hookVsync(param.getClassLoader());
            Log.i(TAG, "Vsync 挂钩已安装 (" + packageName + ")");
            tracker = new FrameTimeTracker();
            openFrameWriter();
            initTargetFps();
        } catch (Throwable t) {
            Log.e(TAG, "挂钩失败", t);
        }
    }

    private void hookVsync(ClassLoader hostLoader) {
        Class<?> choreographer;
        try {
            choreographer = Class.forName("android.view.Choreographer",
                    false, hostLoader);
        } catch (ClassNotFoundException e) {
            Log.e(TAG, "未找到 Choreographer 类", e);
            return;
        }

        Method onVsync = null;
        try {
            Class<?> receiver = Class.forName(
                    "android.view.Choreographer$FrameDisplayEventReceiver",
                    false, hostLoader);
            for (Method m : receiver.getDeclaredMethods()) {
                if (m.getName().equals("onVsync")) {
                    onVsync = m;
                    Log.i(TAG, "找到 onVsync: " + receiver.getSimpleName() + "." + m);
                    break;
                }
            }
        } catch (ClassNotFoundException e) {
            Log.e(TAG, "未找到 FrameDisplayEventReceiver 类", e);
            return;
        }
        if (onVsync == null) {
            Log.e(TAG, "未找到 onVsync 方法");
            return;
        }

        // NOTE: 不做 deoptimize。deopt 热路径方法会强制 ART 去内联,在开机
        // 高峰期间拖慢 systemui 主线程(曾导致 systemui 静默被杀)。
        hook(onVsync).intercept(new FrameHooker());
        Log.i(TAG, "已挂钩 FrameDisplayEventReceiver.onVsync");
    }

    /** 初始化目标帧率:手动优先;自动模式写默认 120(feasd 切换时直写内核)。 */
    private static void initTargetFps() {
        try {
            int manual = PerfMgrSysfs.getManualFps();
            if (manual > 0) {
                writeFpsToKernel(manual);
                Log.i(TAG, "目标帧率(手动): " + manual);
            } else {
                writeFpsToKernel(120);
                Log.i(TAG, "目标帧率(自动): 默认 120,feasd 切换时同步");
            }
        } catch (Throwable t) {
            Log.w(TAG, "初始化失败: " + t.getMessage());
        }
    }

    private static void writeFpsToKernel(int fps) {
        try {
            FileOutputStream fos = new FileOutputStream(FPS_SYSFS);
            fos.write((fps + "\n").getBytes("US-ASCII"));
            fos.close();
        } catch (Throwable ignored) {
        }
    }

    private static synchronized void openFrameWriter() {
        if (frameWriter != null) return;
        if (writerFailed) return;
        try {
            frameWriter = new FileOutputStream(FRAME_SYSFS);
            Log.i(TAG, "已打开 " + FRAME_SYSFS);
        } catch (Throwable t) {
            Log.w(TAG, "打开 " + FRAME_SYSFS + " 失败: " + t.getMessage());
            writerFailed = true;
        }
    }

    private static synchronized boolean writeFrame(long durationNs) {
        if (frameWriter == null) {
            openFrameWriter();
            if (frameWriter == null) return false;
        }
        try {
            frameWriter.write((durationNs + "\n").getBytes("US-ASCII"));
            frameWriter.flush();
            return true;
        } catch (Throwable t) {
            frameWriter = null;
            return false;
        }
    }

    /* ---------- 异步帧写:onVsync 只入队,后台线程做 sysfs 写 ---------- */

    /* 有界队列:满则丢(后台会排空,丢旧值无害,avgNs 只要反映最近帧况) */
    private static final java.util.concurrent.ArrayBlockingQueue<Long> FRAME_Q =
            new java.util.concurrent.ArrayBlockingQueue<Long>(32);
    private static volatile boolean writerStarted = false;

    private static void enqueueFrame(long avgNs) {
        try {
            FRAME_Q.offer(avgNs);
        } catch (Throwable ignored) {
        }
        ensureWriterThread();
    }

    private static synchronized void ensureWriterThread() {
        if (writerStarted) {
            return;
        }
        writerStarted = true;
        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                while (true) {
                    try {
                        Long last = FRAME_Q.take();
                        Long cur;
                        // 排空积压,只写最新(防止队列堆积导致延迟越来越大)
                        while ((cur = FRAME_Q.poll()) != null) {
                            last = cur;
                        }
                        writeFrame(last);
                    } catch (InterruptedException e) {
                        return;
                    } catch (Throwable ignored) {
                    }
                }
            }
        }, "feas-frame-writer");
        t.setDaemon(true);
        t.start();
    }

    public static final class FrameHooker implements XposedInterface.Hooker {

        @Override
        public Object intercept(Chain chain) throws Throwable {
            FrameTimeTracker tr = tracker;
            if (tr == null) {
                return chain.proceed();
            }
            try {
                // 统一测量点:FrameTimeTracker 在 Choreographer 线程串行调用
                tr.record(System.nanoTime());
                if (tr.isBatchFull()) {
                    long avgNs = tr.flush();
                    int jankCount = tr.getLastJankCount();

                    // 双通道上报:
                    // 1) Binder oneway(total, avgNs, jankCount) -> feasd dfps/jank
                    //    (oneway 不阻塞,直接发)
                    // 2) 内核 sysfs frame(avgNs) -> perf_anim_active/CPU 调度
                    //    入队移交后台线程写,onVsync 渲染关键路径零阻塞 I/O
                    FeasBinderClient.reportFrames(
                            tr.getFrameTotal(), (int) avgNs, jankCount);
                    enqueueFrame(avgNs);
                    FeasStats.incOk();
                }
                FeasStats.incFrame();
            } catch (Throwable ignored) {
            }
            return chain.proceed();
        }
    }
}