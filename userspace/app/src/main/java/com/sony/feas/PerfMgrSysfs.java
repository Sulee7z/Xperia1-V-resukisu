package com.sony.feas;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStreamReader;

/**
 * 内核 perf_manager 驱动访问工具 —— v3.1 Binder-only。
 * 独立于 Xposed 模块,app 和模块都可调用。
 *
 * v3.1:控制一律走 Binder(daemon 已确认成功),删除 su 兜底。
 * setManualFps/setEnabled 失败时仅记录 prefs,不 spawn su。
 * 保留:只读内核 fps/enable(sysfs 0666 可读,非兜底)。
 */
public final class PerfMgrSysfs {

    private static final String TAG = "FEAS-SYSFS";
    private static final String SYSFS_PATH = "/sys/kernel/perf_manager";
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

    /** 设置手动目标帧率(0 = 自动)。Binder-only,失败静默。 */
    public static void setManualFps(int fps) {
        SharedPreferences p = prefs();
        if (p != null) p.edit().putInt(KEY_MANUAL_FPS, fps).apply();
        if (!FeasBinderClient.setManualFps(fps)) {
            Log.w(TAG, "setManualFps(" + fps + ") FAILED (daemon binder 不可用)");
        }
    }

    /** 设置模块开关。Binder-only,失败静默。 */
    public static void setEnabled(boolean enable) {
        SharedPreferences p = prefs();
        if (p != null) p.edit().putBoolean(KEY_ENABLED, enable).apply();
        if (!FeasBinderClient.setEnabled(enable)) {
            Log.w(TAG, "setEnabled(" + enable + ") FAILED (daemon binder 不可用)");
        }
    }
}