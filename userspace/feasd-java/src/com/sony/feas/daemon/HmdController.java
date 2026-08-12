package com.sony.feas.daemon;

import android.util.Log;

import java.lang.reflect.Method;

/**
 * 240Hz MBR(黑帧插入/BFI)控制 —— 复用 DisplayBooster 的官方客户端。
 *
 * 原理(逆向 com.sonymobile.displaybooster,1V 固件 67.0.A.1.xxx):
 *  - DisplayBooster 的 FpsModeManager 模式表(disp_panel_conf.xml v2.0)中,
 *    TURBO 模式(fps_mode=6)是唯一 hmd=1 的档位:120Hz + 黑帧插入 = 240Hz MBR。
 *  - 应用路径:FramerateControllerWrapper.create() 优先 AIDL
 *    (vendor.semc.hardware.aidldisplay.IFramerateController/default,1V 上不存在),
 *    失败回退 HIDL(vendor.semc.hardware.display@2.2::IFramerateController/default,在线)
 *  - HIDL 服务(pid 1884 vendor 显示服务)最终把 set_hmd_mode(1) 传给内核,
 *    下发面板 somc,mdss-dsi-hmd-on-command(仅在 120Hz timing 定义)。
 *
 * 实现:PathClassLoader 加载设备上的 DisplayBooster APK,反射调用官方 wrapper,
 * 不复制任何索尼代码,天然兼容 AIDL/HIDL 自动选择。
 *
 * 约束:必须在 120Hz 模式调用才有效(hmd 命令只存在于 120Hz timing)。
 * 调用者负责:120Hz 时 ON,降 60Hz 前 OFF。
 */
public final class HmdController {

    private static final String TAG = "FEASD";

    /** 1V(67.0.A.1.xxx)上 DisplayBooster 的安装路径。 */
    private static final String BOOSTER_APK =
            "/system_ext/priv-app/DisplayBooster/DisplayBooster.apk";
    private static final String WRAPPER_CLASS =
            "vendor.semc.hardware.display.wrapper.FramerateControllerWrapper";

    private static Object controller;   // FramerateControllerWrapper 实例
    private static Method setHmdMode;   // set_hmd_mode(int)
    private static volatile boolean initFailed = false;
    private static volatile boolean currentOn = false;

    private HmdController() {
    }

    /** 设置 240Hz MBR 开关。失败静默并记日志,不影响 dfps 主流程。 */
    public static synchronized void setHmd(boolean on) {
        if (on == currentOn) return;
        try {
            ensureInit();
            if (controller == null || setHmdMode == null) return;
            int ret = (Integer) setHmdMode.invoke(controller, on ? 1 : 0);
            Main.log("I", "HmdController.set_hmd_mode(" + (on ? 1 : 0) + ") -> " + ret);
            currentOn = on;
        } catch (Throwable t) {
            Main.log("W", "HmdController failed: " + t);
        }
    }

    private static void ensureInit() {
        if (initFailed) return;
        try {
            if (setHmdMode == null) {
                ClassLoader cl = new dalvik.system.PathClassLoader(
                        BOOSTER_APK, HmdController.class.getClassLoader());
                Class<?> wrapper = Class.forName(WRAPPER_CLASS, true, cl);
                Method create = wrapper.getMethod("create");
                controller = create.invoke(null);
                setHmdMode = wrapper.getMethod("set_hmd_mode", int.class);
                Main.log("I", "HmdController init ok: "
                        + controller.getClass().getName());
            }
        } catch (Throwable t) {
            initFailed = true;
            Main.log("W", "HmdController init failed (BFI unavailable): " + t);
        }
    }
}
