package com.sony.feas;

import android.util.Log;

import java.io.FileOutputStream;
import java.lang.reflect.Method;

import io.github.libxposed.api.XposedInterface;
import io.github.libxposed.api.XposedModule;
import io.github.libxposed.api.XposedModuleInterface.PackageReadyParam;
import io.github.libxposed.api.XposedModuleInterface.SystemServerStartingParam;

/**
 * FEAS 模块入口(libxposed API 102,Vector 框架)。
 *
 * 完整功能:
 *  - hook onVsync 检测帧
 *  - 批量上报帧间隔到内核(sysfs)
 *  - 目标帧率:手动优先;自动模式动态跟随实际刷新率
 *    (60Hz → 60, 120Hz → 120,避免调频错乱卡顿)
 *  - 分辨率解锁(system_server + Settings,原 pdx234 合并,见 ResolutionUnlock)
 *  - 统计供 UI 显示
 */
public class FeasModule extends XposedModule {

    private static final String TAG = "FEAS";
    private static final int FRAME_BATCH = 30;
    private static final String FRAME_SYSFS = "/sys/kernel/perf_manager/frame";
    private static final String FRAME_TOTAL_SYSFS = "/sys/kernel/perf_manager/frame_total";
    private static final String FPS_SYSFS = "/sys/kernel/perf_manager/fps";

    private static volatile FileOutputStream frameWriter;
    private static volatile boolean writerFailed = false;
    private static volatile long lastReportedAvg = 0;

    // 轻量帧间隔统计(替代 map,减少每帧分配)
    private static volatile long lastVsyncNs = 0;
    private static int batchCount = 0;
    private static long batchSumNs = 0;

    private static final String PROCESS_SYSTEM_UI = "com.android.systemui";
    private static final String PROCESS_SETTINGS = "com.android.settings";
    private static boolean hooksStarted = false;

    @Override
    public void onModuleLoaded(ModuleLoadedParam param) {
        Log.i(TAG, "FEAS 模块已加载, api=" + getApiVersion()
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
     * 开机恢复用户分辨率(pdx234 机制源码级复刻,不依赖 PersistentDataStore):
     *
     * 背景(DMS 源码分析,67.2.A.3.163):
     *  - PersistentDataStore(display_settings.xml)对内部显示
     *    (uniqueId="local:131", hasStableUniqueId()==false)不保存用户偏好 →
     *    configurePreferredDisplayModeLocked 开机读不到 → 无恢复。
     *  - cmd display set(service.sh,boot 后期)太晚:只写 settings 全局 +
     *    内存 mUserPreferredMode,display 配置流程早已完成。
     *  - 正确路径:在 system_server 内早期调用
     *    DisplayManagerGlobal.setUserPreferredDisplayMode(0, mode) →
     *    DisplayDevice.setUserPreferredDisplayModeLocked →
     *    SurfaceFlinger.setBootDisplayMode → bootanimation 即 4K,
     *    且 device preferred 生效,systemui 以 4K 启动。
     *
     * 时机:等待 WMS mSystemBooted=true(与原模块 service.sh 一致)后执行;
     * 此时 bootanimation 通常仍在播放(索尼开机动画较长),systemui 尚未
     * 完成初始化,切换发生在显示栈早期,安全(实验证明风险窗口是
     * systemui 初始化中段,而非 boot 早期)。
     */
    private void scheduleBootResolutionRestore() {
        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    trace("bootrest thread start");
                    // Wait for boot_completed via SystemProperties reflection
                    // (spawning dumpsys from system_server is blocked, which made
                    // the mSystemBooted wait loop time out for 100s).
                    boolean booted = false;
                    Class<?> sp = Class.forName("android.os.SystemProperties");
                    for (int i = 0; i < 180; i++) {
                        try {
                            String v = (String) sp.getMethod("get", String.class)
                                    .invoke(null, "sys.boot_completed");
                            if ("1".equals(v)) { booted = true; break; }
                        } catch (Throwable ignored) {
                        }
                        Thread.sleep(1000);
                    }
                    trace("bootrest boot_completed=" + booted);
                    if (!booted) return;

                    // Read user preference (SettingsProvider is ready)
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

                    // 1) Force the mode now, exactly like the Settings page does
                    //    (DisplayManager.requestDisplayModes is a public API and
                    //     the in-system_server DisplayManagerGlobal token is valid).
                    Object info = dmg.getMethod("getDisplayInfo", int.class)
                            .invoke(inst, 0);
                    java.lang.reflect.Field sf = info.getClass().getField("supportedModes");
                    Object[] modes = (Object[]) sf.get(info);
                    // CRITICAL: use the REAL Display.Mode object from the display
                    // (exact refreshRate like 120.00001f). Constructing a new
                    // Display.Mode(1644,3840,120f) fails Display.Mode.matches()
                    // (exact == comparison) inside findUserPreferredModeIdLocked
                    // and setUserPreferredDisplayMode is silently dropped.
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
                    // Settings page (ScreenResolutionFragment.setDisplayMode) writes
                    // Settings.System.user_selected_resolution - replicate it.
                    android.provider.Settings.System.putString(cr,
                            "user_selected_resolution", wantW + "x" + wantH);
                    trace("bootrest user_selected_resolution=" + wantW + "x" + wantH);

                    if (modeId > 0) {
                        java.lang.reflect.Method req = dmg.getMethod(
                                "requestDisplayModes", int.class, int[].class);
                        req.invoke(inst, 0, new int[]{modeId});
                        trace("bootrest requestDisplayModes modeId=" + modeId);
                    }

                    // Store the preference with the REAL mode object (sets
                    // device preferred + SF boot mode; exact refreshRate matches).
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

        // 帧上报:只在 systemui 进程挂钩(参照 cirno 的 onPackageReady 模式)
        if (!PROCESS_SYSTEM_UI.equals(packageName) || hooksStarted) {
            return;
        }
        hooksStarted = true;

        Log.i(TAG, "SystemUI ready, hooking onVsync");
        try {
            hookVsync(param.getClassLoader());
            Log.i(TAG, "Vsync 挂钩已安装");
            openFrameWriter();
            initTargetFps();
        } catch (Throwable t) {
            Log.e(TAG, "挂钩失败", t);
        }
    }

    private void hookVsync(ClassLoader hostLoader) {
        // 精确锁定 Choreographer.FrameDisplayEventReceiver.onVsync(long,long,int),
        // 不遍历所有内部类找第一个 onVsync(可能命中错误方法)。
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
        // libxposed 官方语义:deoptimize 仅在"钩子因内联不生效"时作为补救。
        hook(onVsync).intercept(new FrameHooker());
        Log.i(TAG, "已挂钩 FrameDisplayEventReceiver.onVsync");
    }

    /** 初始化目标帧率:手动优先;自动模式写默认 120(feasd 切换时直写内核)。
     * 模块不做任何轮询/进程 spawn(极限省电)。
     */
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

    /** 写真实累计帧数到内核(死区跳过上报时也能反映真实渲染量) */
    private static volatile java.io.FileOutputStream frameTotalWriter;

    private static synchronized void writeFrameTotal() {
        try {
            java.io.FileOutputStream fos = frameTotalWriter;
            if (fos == null) {
                fos = new java.io.FileOutputStream(FRAME_TOTAL_SYSFS);
                frameTotalWriter = fos;
            }
            fos.write((FeasStats.getFrameCount() + "\n").getBytes("US-ASCII"));
            fos.flush();
        } catch (Throwable ignored) {
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

    public static final class FrameHooker implements XposedInterface.Hooker {

        @Override
        public Object intercept(Chain chain) throws Throwable {
            FeasStats.incFrame();
            long n = FeasStats.getFrameCount();
            if (n % 600 == 0) {
                Log.i(TAG, "vsync 调用计数: " + n);
            }
            try {
                // 用轻量字段替代 map:onVsync 在 Choreographer 线程串行调用
                long now = System.nanoTime();
                long last = lastVsyncNs;
                lastVsyncNs = now;
                if (last != 0) {
                    long durationNs = now - last;
                    if (durationNs > 0 && durationNs < 5_000_000_000L) {
                        batchCount++;
                        batchSumNs += durationNs;
                        if (batchCount >= FRAME_BATCH) {
                            long avgNs = batchSumNs / batchCount;
                            batchCount = 0;
                            batchSumNs = 0;

                            // 死区:帧间隔变化 < 10% 不上报内核
                            // 但统计记为成功(模块在正常工作,只是帧稳定无需上报)
                            if (lastReportedAvg != 0) {
                                long diff = Math.abs(avgNs - lastReportedAvg);
                                if (diff * 10 < lastReportedAvg) {
                                    FeasStats.incOk();
                                    writeFrameTotal();
                                    return chain.proceed();
                                }
                            }
                            lastReportedAvg = avgNs;

                            boolean ok = writeFrame(avgNs);
                            writeFrameTotal();
                            if (ok) {
                                FeasStats.incOk();
                                long rn = FeasStats.getReportOk();
                                if (rn % 60 == 0) {
                                    Log.i(TAG, "帧上报累计: " + rn + " 成功, "
                                            + FeasStats.getReportFail() + " 失败");
                                }
                            } else {
                                FeasStats.incFail();
                            }
                        }
                    }
                }
            } catch (Throwable ignored) {
            }
            return chain.proceed();
        }
    }
}
