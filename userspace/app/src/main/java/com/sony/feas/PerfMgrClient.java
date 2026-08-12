package com.sony.feas;

import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.charset.StandardCharsets;

/**
 * Userspace client for the kernel perf_manager driver.
 *
 * Frame reporting via write():
 *   open("/dev/perf_manager") then write("<duration_ns>\n")
 * Tuning via sysfs:
 *   /sys/kernel/perf_manager/{enable,fps,margin}
 */
public final class PerfMgrClient {
    private static final String TAG = "FEAS";
    private static final String DEVICE_PATH = "/dev/perf_manager";
    private static final String SYSFS_PATH = "/sys/kernel/perf_manager";

    private static FileOutputStream writer;
    private static int openFailures = 0;

    private PerfMgrClient() {}

    public static synchronized boolean open() {
        if (writer != null) return true;
        try {
            writer = new FileOutputStream(DEVICE_PATH);
            Log.i(TAG, "opened " + DEVICE_PATH);
            return true;
        } catch (Exception e) {
            openFailures++;
            if (openFailures <= 3) {
                Log.w(TAG, "open " + DEVICE_PATH + " failed: " + e.getMessage());
            }
            return false;
        }
    }

    public static synchronized void close() {
        if (writer != null) {
            try { writer.close(); } catch (Exception ignored) {}
            writer = null;
        }
    }

    public static boolean isKernelDriverPresent() {
        return new File(DEVICE_PATH).exists();
    }

    /** Report a completed frame with its duration in ns (best-effort). */
    public static synchronized boolean reportFrame(long durationNs, int boost) {
        if (writer == null) return false;
        try {
            writer.write((durationNs + "\n").getBytes(StandardCharsets.US_ASCII));
            writer.flush();
            return true;
        } catch (Exception e) {
            return false; // silent - frame reporting is best-effort
        }
    }

    /** Set target FPS via sysfs (0 = auto). */
    public static void setTargetFps(int fps) {
        try {
            FileOutputStream fos = new FileOutputStream(SYSFS_PATH + "/fps");
            fos.write(String.valueOf(fps).getBytes(StandardCharsets.US_ASCII));
            fos.close();
        } catch (Exception ignored) {}
    }

    public static int getTargetFps() {
        try {
            FileInputStream fis = new FileInputStream(SYSFS_PATH + "/fps");
            byte[] buf = new byte[16];
            int n = fis.read(buf);
            fis.close();
            if (n > 0) return Integer.parseInt(new String(buf, 0, n).trim());
        } catch (Exception ignored) {}
        return 0;
    }
}
