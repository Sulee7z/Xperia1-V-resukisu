package com.sony.feas;

import android.content.Context;
import android.content.SharedPreferences;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStreamReader;

/**
 * 内核 perf_manager 驱动访问工具。
 * 独立于 Xposed 模块,app 和模块都可调用。
 */
public final class PerfMgrSysfs {

    private static final String SYSFS_PATH = "/sys/kernel/perf_manager";
    private static final String CONFIG_DIR = "/data/adb/feas";
    private static final String CONFIG_FILE = CONFIG_DIR + "/fps_config";
    private static final String PREF_NAME = "feas_prefs";
    private static final String KEY_MANUAL_FPS = "manual_fps";
    private static final String KEY_ENABLED = "enabled";

    private static Context appContext;

    private PerfMgrSysfs() {}

    public static void init(Context ctx) {
        appContext = ctx.getApplicationContext();
    }

    private static SharedPreferences prefs() {
        if (appContext == null) return null;
        return appContext.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
    }

    public static boolean isDriverPresent() {
        return new File(SYSFS_PATH + "/frame").exists();
    }

    /** 读取内核目标帧率 */
    public static int readTargetFps() {
        try {
            FileInputStream fis = new FileInputStream(SYSFS_PATH + "/fps");
            BufferedReader r = new BufferedReader(new InputStreamReader(fis));
            String line = r.readLine();
            r.close();
            if (line != null) return Integer.parseInt(line.trim());
        } catch (Throwable ignored) {
        }
        return 0;
    }

    /** 读取内核模块开关(真实状态) */
    public static boolean isEnabled() {
        try {
            FileInputStream fis = new FileInputStream(SYSFS_PATH + "/enable");
            BufferedReader r = new BufferedReader(new InputStreamReader(fis));
            String line = r.readLine();
            r.close();
            if (line != null) return line.trim().equals("1");
        } catch (Throwable ignored) {
        }
        // 读不到时回退到 prefs
        SharedPreferences p = prefs();
        if (p != null) return p.getBoolean(KEY_ENABLED, true);
        return true;
    }

    /** UI 显示的手动帧率 */
    public static int getManualFps() {
        SharedPreferences p = prefs();
        if (p == null) return 0;
        return p.getInt(KEY_MANUAL_FPS, 0);
    }

    /** 设置手动目标帧率(0 = 自动) */
    public static void setManualFps(int fps) {
        SharedPreferences p = prefs();
        if (p != null) p.edit().putInt(KEY_MANUAL_FPS, fps).apply();
        execSu("mkdir -p " + CONFIG_DIR);
        execSu("echo " + fps + " > " + CONFIG_FILE);
        if (fps > 0) {
            execSu("echo " + fps + " > " + SYSFS_PATH + "/fps");
        }
    }

    /** 设置模块开关 */
    public static void setEnabled(boolean enable) {
        SharedPreferences p = prefs();
        if (p != null) p.edit().putBoolean(KEY_ENABLED, enable).apply();
        execSu("echo " + (enable ? 1 : 0) + " > " + SYSFS_PATH + "/enable");
    }

    /** 通过 su 执行命令 */
    public static void execSu(String cmd) {
        try {
            Process p = new ProcessBuilder("su", "-c", cmd)
                    .redirectErrorStream(true).start();
            p.waitFor();
        } catch (Throwable ignored) {
        }
    }
}
