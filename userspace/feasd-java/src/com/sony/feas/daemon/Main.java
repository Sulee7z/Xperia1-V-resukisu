/*
 * FeasDaemon - root daemon for FEAS
 *
 * Roles:
 *  1. (legacy) Binder service "feas" for frame reporting via /dev/perf_manager
 *  2. (dfps)   Listen /dev/input touch events -> switch refresh rate:
 *              touch down -> 120Hz, idle 4s -> 60Hz
 *              Uses the verified Sony protocol:
 *                setprop persist.sony.user_fpsmode true|false
 *                am broadcast -a com.sonymobile.USER_FPSMODE_CHANGED
 *
 * Runs as root via module service.sh using app_process:
 *   app_process -Djava.class.path=/data/adb/modules/feas/feasd.jar \
 *                /system/bin --nice-name=feasd com.sony.feas.daemon.Main
 */
package com.sony.feas.daemon;

import android.util.Log;

import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;

public class Main {

    private static final String TAG = "FEASD";

    // ---- dfps ----
    private static final String FPSMODE_PROP = "persist.sony.user_fpsmode";
    private static final String[] INPUT_DEVICES = {"/dev/input/event4"};
    private static final long IDLE_SLACK_MS = 4000;  // idle -> 60Hz after this
    private static final long SWITCH_MIN_INTERVAL_MS = 1000; // debounce
    private static final String BROADCAST_CMD =
        "am broadcast -a com.sonymobile.USER_FPSMODE_CHANGED -p com.sonymobile.displaybooster";

    // GPU compensation: keep touch GPU floor identical between 60Hz and 120Hz.
    // Stock: touch_gpu_mhz=550. When screen drops to 60Hz, WALT sees lower load
    // and lets GPU idle below the 120Hz touch level; raise floor to GPU max (680)
    // so interaction performance matches 120Hz exactly.
    private static final String GPU_MHZ_SYSFS = "/sys/kernel/perf_manager/touch_gpu_mhz";
    private static final int GPU_MHZ_HIGH = 680;   // applied while in 60Hz mode
    private static final int GPU_MHZ_NORMAL = 550; // restored in 120Hz mode

    // Control file: written by FEAS app (via su) to enable/disable dfps.
    private static final String CTRL_FILE = "/data/adb/modules/feas/dfps_enabled";
    // Control file: written by FEAS app (via su) to enable/disable 240Hz MBR (BFI).
    private static final String HMD_CTRL_FILE = "/data/adb/modules/feas/hmd_enabled";
    // State file: written by daemon so the FEAS module can read current
    // refresh-rate state without spawning a getprop process (power saving).
    private static final String STATE_FILE = "/data/adb/modules/feas/fpsmode_state";

    // Animation GPU boost: monitor GPU utilization directly (reliable for ALL
    // apps' animations - module vsync hook only sees systemui's own rendering).
    // GPU busy > threshold -> animation active -> raise GPU + big-core CPU floor.
    // Refresh rate is NOT changed - only performance floors, so screenshot/system
    // animations stay smooth without flipping 60/120.
    private static final String GPU_BUSY_SYSFS = "/sys/class/kgsl/kgsl-3d0/gpu_busy_percentage";
    private static final String GPU_DEVFREQ_MIN = "/sys/class/devfreq/3d00000.qcom,kgsl-3d0/min_freq";
    private static final long GPU_FLOOR_ANIM = 680000000L;   // animation: GPU >= 680 MHz
    private static final long GPU_FLOOR_IDLE = 0L;           // idle: restore governor control
    private static final int GPU_BUSY_THRESHOLD = 15;        // % busy = animation (lower for smooth)
    private static final long GPU_BUSY_IDLE_MS = 3000;       // idle after 3s low load
    // Big-core CPU floor while animating (avoids ramp-up stutter on screenshots)
    private static final String CPU_BIG_MIN = "/sys/devices/system/cpu/cpu6/cpufreq/scaling_min_freq";
    private static final String CPU_BIG_MAX = "/sys/devices/system/cpu/cpu6/cpufreq/scaling_max_freq";
    private static final long CPU_BIG_FLOOR_ANIM = 2000000L; // 2 GHz on big cluster
    private static final long CPU_BIG_MAX_DEFAULT = 3187200L;

    private static volatile boolean highRefresh = false;
    private static volatile long lastSwitchMs = 0;
    private static volatile long lastTouchMs = 0;
    private static volatile boolean dfpsEnabled = true;
    private static volatile boolean hmdEnabled = false;

    // struct input_event: timeval(16) + type(2) + code(2) + value(4) = 24 bytes
    private static final int EV_ABS = 3;
    private static final int ABS_MT_SLOT = 0x2f;
    private static final int ABS_MT_TRACKING_ID = 0x39;
    private static final int ABS_MT_TOUCH_MAJOR = 0x30;

    public static void main(String[] args) {
        log("I", "FEAS daemon starting (root)");

        // dfps: touch listener + idle monitor
        loadDfpsState();
        loadHmdState();
        startCtrlMonitor();
        startAnimMonitor();
        startInputListener();
        startIdleMonitor();


        while (true) {
            try {
                Thread.sleep(Long.MAX_VALUE);
            } catch (InterruptedException e) {
                break;
            }
        }
    }

    /** Read enable state from control file (written by FEAS app). */
    private static void loadDfpsState() {
        try {
            File f = new File(CTRL_FILE);
            if (f.exists()) {
                String s = new String(java.nio.file.Files.readAllBytes(f.toPath()),
                        StandardCharsets.US_ASCII).trim();
                dfpsEnabled = s.equals("1");
                log("I", "dfps " + (dfpsEnabled ? "enabled" : "disabled") + " (ctrl file)");
            }
        } catch (Exception e) {
            log("W", "load ctrl failed: " + e.getMessage());
        }
    }


    /** File log: root processes cannot write logcat on this build (SELinux/logd),
     *  so daemon logs go to the module directory. UTF-8, append. */
    static void log(String level, String msg) {
        try {
            FileOutputStream fos = new FileOutputStream(
                    "/data/adb/modules/feas/feasd.log", true);
            String line = System.currentTimeMillis() + " " + level + " " + msg + "\n";
            fos.write(line.getBytes(StandardCharsets.UTF_8));
            fos.close();
        } catch (Throwable ignored) {
        }
        Log.i(TAG, msg);
    }

    /** Read 240Hz MBR (BFI) enable state from control file. */
    private static void loadHmdState() {
        try {
            File f = new File(HMD_CTRL_FILE);
            if (f.exists()) {
                String s = new String(java.nio.file.Files.readAllBytes(f.toPath()),
                        StandardCharsets.US_ASCII).trim();
                hmdEnabled = s.equals("1");
                log("I", "hmd(BFI) " + (hmdEnabled ? "enabled" : "disabled") + " (ctrl file)");
            }
        } catch (Exception e) {
            log("W", "load hmd ctrl failed: " + e.getMessage());
        }
    }

    /** Poll control files for enable/disable changes from the FEAS app. */
    private static void startCtrlMonitor() {
        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                long lastMod = 0;
                long lastHmdMod = 0;
                while (true) {
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        break;
                    }
                    File f = new File(CTRL_FILE);
                    if (f.exists()) {
                        long mod = f.lastModified();
                        if (mod != lastMod) {
                            lastMod = mod;
                            boolean was = dfpsEnabled;
                            loadDfpsState();
                            if (was != dfpsEnabled) {
                                log("I", "dfps " + (dfpsEnabled ? "enabled" : "disabled")
                                        + " by app");
                                if (!dfpsEnabled && highRefresh) {
                                    highRefresh = false; // allow re-switch when re-enabled
                                }
                            }
                        }
                    }
                    // 240Hz MBR (BFI) control: apply immediately when 120Hz is active
                    File h = new File(HMD_CTRL_FILE);
                    if (h.exists()) {
                        long mod = h.lastModified();
                        if (mod != lastHmdMod) {
                            lastHmdMod = mod;
                            boolean was = hmdEnabled;
                            loadHmdState();
                            if (was != hmdEnabled) {
                                log("I", "hmd(BFI) " + (hmdEnabled ? "enabled" : "disabled")
                                        + " by app");
                                if (highRefresh) {
                                    HmdController.setHmd(hmdEnabled);
                                }
                            }
                        }
                    }
                }
            }
        }, "feasd-ctrl");
        t.setDaemon(true);
        t.start();
    }

    /** Animation GPU boost: monitor GPU utilization, raise min_freq while busy. */
    private static void startAnimMonitor() {
        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                boolean lastAnim = false;
                long lastBusyMs = 0;
                while (true) {
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        break;
                    }
                    int busy = readGpuBusy();
                    boolean anim = false;
                    if (busy >= GPU_BUSY_THRESHOLD) {
                        lastBusyMs = System.currentTimeMillis();
                        anim = true;
                    } else if (lastBusyMs > 0
                            && System.currentTimeMillis() - lastBusyMs < GPU_BUSY_IDLE_MS) {
                        anim = true;  // keep boost briefly after load drops
                    }
                    if (anim == lastAnim) continue;
                    lastAnim = anim;
                    if (anim) {
                        writeSysfs(GPU_DEVFREQ_MIN, GPU_FLOOR_ANIM);
                        writeSysfs(CPU_BIG_MIN, CPU_BIG_FLOOR_ANIM);
                        writeSysfs(CPU_BIG_MAX, CPU_BIG_MAX_DEFAULT);
                        log("I", "animation boost: gpu busy=" + busy
                                + "%, gpu=" + GPU_FLOOR_ANIM
                                + " cpu_big>=" + CPU_BIG_FLOOR_ANIM);
                    } else {
                        writeSysfs(GPU_DEVFREQ_MIN, GPU_FLOOR_IDLE);
                        writeSysfs(CPU_BIG_MIN, 0);
                        log("I", "animation idle: gpu busy=" + busy
                                + "%, floors restored");
                    }
                }
            }
        }, "feasd-anim");
        t.setDaemon(true);
        t.start();
    }

    private static String execCapture(String cmd) {
        try {
            Process p = new ProcessBuilder("sh", "-c", cmd)
                    .redirectErrorStream(true).start();
            java.io.BufferedReader r = new java.io.BufferedReader(
                    new java.io.InputStreamReader(p.getInputStream()));
            StringBuilder sb = new StringBuilder();
            String line;
            while ((line = r.readLine()) != null) sb.append(line).append('\n');
            r.close();
            p.waitFor();
            return sb.toString();
        } catch (Exception e) {
            return "";
        }
    }

    private static void execQuiet(String cmd) {
        try {
            Process p = new ProcessBuilder("sh", "-c", cmd)
                    .redirectErrorStream(true).start();
            p.waitFor();
        } catch (Exception e) {
            log("W", "exec failed: " + cmd);
        }
    }

    /** Read GPU busy percentage (0-100). */
    private static int readGpuBusy() {
        try {
            FileInputStream fis = new FileInputStream(GPU_BUSY_SYSFS);
            byte[] buf = new byte[16];
            int n = fis.read(buf);
            fis.close();
            if (n > 0) {
                String s = new String(buf, 0, n, StandardCharsets.US_ASCII).trim();
                return Integer.parseInt(s);
            }
        } catch (Exception e) {
            // ignore
        }
        return 0;
    }

    private static boolean readCtrlBool(String path) {
        try {
            File f = new File(path);
            if (!f.exists()) return false;
            String s = new String(Files.readAllBytes(f.toPath()),
                    StandardCharsets.US_ASCII).trim();
            return s.equals("1");
        } catch (Exception e) {
            return false;
        }
    }

    private static void writeSysfs(String path, long value) {
        try {
            FileOutputStream fos = new FileOutputStream(path);
            fos.write((value + "\n").getBytes(StandardCharsets.US_ASCII));
            fos.close();
        } catch (Exception e) {
            log("W", "write " + path + " failed: " + e.getMessage());
        }
    }

    /** setprop without spawning: reflection into android.os.SystemProperties. */
    private static void setProp(String key, String value) {
        try {
            Class<?> sp = Class.forName("android.os.SystemProperties");
            java.lang.reflect.Method set = sp.getMethod("set", String.class, String.class);
            set.invoke(null, key, value);
        } catch (Throwable t) {
            log("W", "setprop " + key + " failed: " + t);
        }
    }

    private static void writeCtrlFile(String path, String value) {
        try {
            FileOutputStream fos = new FileOutputStream(path);
            fos.write(value.getBytes(StandardCharsets.US_ASCII));
            fos.close();
        } catch (Exception e) {
            log("W", "write " + path + " failed: " + e.getMessage());
        }
    }

    /** Read /dev/input touch events; update lastTouchMs + switch to 120Hz on down. */
    private static void startInputListener() {
        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                byte[] buf = new byte[24];
                while (true) {
                    boolean anyOk = false;
                    for (String path : INPUT_DEVICES) {
                        File f = new File(path);
                        if (!f.exists()) continue;
                        try {
                            FileInputStream fis = new FileInputStream(f);
                            DataInputStream dis = new DataInputStream(fis);
                            log("I", "listening " + path);
                            anyOk = true;
                            while (true) {
                                try {
                                    dis.readFully(buf);
                                } catch (Exception e) {
                                    break; // device removed, retry
                                }
                                int type = ((buf[16] & 0xff) | ((buf[17] & 0xff) << 8));
                                int code = ((buf[18] & 0xff) | ((buf[19] & 0xff) << 8));
                                int value = ((buf[20] & 0xff) | ((buf[21] & 0xff) << 8)
                                        | ((buf[22] & 0xff) << 16) | ((buf[23] & 0xff) << 24));
                                if (type == EV_ABS) {
                                    if (code == ABS_MT_TRACKING_ID) {
                                        boolean down = (value >= 0);
                                        onTouchEvent(down);
                                    } else if (code == ABS_MT_TOUCH_MAJOR && value > 0) {
                                        onTouchEvent(true);
                                    } else if (code == ABS_MT_SLOT) {
                                        // slot switch: keep state from tracking id
                                    }
                                }
                            }
                        } catch (Exception e) {
                            log("W", "input " + path + " error: " + e.getMessage());
                        }
                    }
                    if (!anyOk) {
                        try { Thread.sleep(3000); } catch (InterruptedException ie) { break; }
                    }
                }
            }
        }, "feasd-input");
        t.setDaemon(true);
        t.start();
    }

    /** Idle monitor: no touch for IDLE_SLACK_MS -> 60Hz. */
    private static void startIdleMonitor() {
        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                while (true) {
                    try {
                        Thread.sleep(1000);
                    } catch (InterruptedException e) {
                        break;
                    }
                    if (!dfpsEnabled) continue;
                    if (highRefresh && lastTouchMs > 0) {
                        long idle = System.currentTimeMillis() - lastTouchMs;
                        if (idle > IDLE_SLACK_MS) {
                            switchRefresh(false);
                        }
                    }
                }
            }
        }, "feasd-idle");
        t.setDaemon(true);
        t.start();
    }

    private static void onTouchEvent(boolean down) {
        if (down) {
            lastTouchMs = System.currentTimeMillis();
            if (dfpsEnabled) {
                switchRefresh(true);
            }
        }
        // up: idle monitor handles the delayed downgrade
    }

    private static synchronized void switchRefresh(final boolean wantHigh) {
        if (wantHigh == highRefresh) return;
        long now = System.currentTimeMillis();
        // Debounce only downgrades (120->60). Up-switch on touch must be
        // immediate: after auto-downgrade, pulling the shade right away
        // otherwise stays at 60Hz and stutters.
        if (now - lastSwitchMs < SWITCH_MIN_INTERVAL_MS && !wantHigh) return;
        lastSwitchMs = now;
        highRefresh = wantHigh;
        final int gpuMhz = wantHigh ? GPU_MHZ_NORMAL : GPU_MHZ_HIGH;
        final int fps = wantHigh ? 120 : 60;
        log("I", "dfps switch -> " + fps + " Hz"
                + ", gpu floor " + gpuMhz + " MHz");
        // state file for the FEAS module (no getprop process needed)
        writeCtrlFile(STATE_FILE, wantHigh ? "1" : "0");
        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    // 240Hz MBR (BFI): must be OFF before dropping to 60Hz
                    // (hmd panel commands only exist in the 120Hz timing)
                    if (!wantHigh && hmdEnabled) {
                        HmdController.setHmd(false);
                    }
                    // Zero-spawn path: setprop via reflection + direct sysfs
                    // writes (daemon runs as root in the ksu domain, which the
                    // module sepolicy.rule already grants perf_manager access).
                    // Only the broadcast needs a spawned process.
                    setProp(FPSMODE_PROP, wantHigh ? "true" : "false");
                    writeSysfs(GPU_MHZ_SYSFS, gpuMhz);
                    writeSysfs("/sys/kernel/perf_manager/fps", fps);
                    execQuiet(BROADCAST_CMD);
                    // BFI only applies while the panel is at 120Hz
                    if (wantHigh && hmdEnabled) {
                        HmdController.setHmd(true);
                    }
                } catch (Exception e) {
                    log("E", "dfps exec failed: " + e);
                }
            }
        }, "feasd-switch");
        t.setDaemon(true);
        t.start();
    }
}
