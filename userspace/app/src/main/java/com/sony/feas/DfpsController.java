package com.sony.feas;

import android.util.Log;

import java.io.BufferedReader;
import java.io.InputStreamReader;

/**
 * dfps (dynamic refresh rate) controller for FEAS.
 *
 * The actual refresh-rate switching is done by the root daemon (feasd):
 *   - listens /dev/input/event4 touch events
 *   - touch -> 120 Hz (setprop persist.sony.user_fpsmode true + broadcast)
 *   - idle 4s -> 60 Hz (false + broadcast)
 *   - GPU compensation: 60Hz mode keeps touch_gpu_mhz=680 (same as 120Hz perf)
 *
 * This class only toggles the daemon's enable state via a control file,
 * so the UI can turn dfps on/off without restarting the daemon.
 */
public final class DfpsController {

    private static final String TAG = "FEAS-DFPS";
    private static final String CTRL_FILE = "/data/adb/modules/feas/dfps_enabled";

    private static volatile boolean enabled = true;

    private DfpsController() {}

    /** Enable/disable dfps by writing the daemon's control file. */
    public static void setEnabled(boolean on) {
        enabled = on;
        execSu("echo " + (on ? "1" : "0") + " > " + CTRL_FILE);
        Log.i(TAG, "dfps " + (on ? "enabled" : "disabled") + " (ctrl file written)");
    }

    public static boolean isEnabled() {
        return enabled;
    }

    /** Not used anymore: daemon listens /dev/input directly. */
    public static void onFrameActivity() {}

    /** Not used anymore: daemon handles idle timeout. */
    public static void onIdle() {}

    public static boolean isHighRefresh() {
        return false;
    }

    private static void execSu(String cmd) {
        try {
            Process p = new ProcessBuilder("su", "-c", cmd)
                    .redirectErrorStream(true).start();
            BufferedReader r = new BufferedReader(
                    new InputStreamReader(p.getInputStream()));
            while (r.readLine() != null) { /* drain */ }
            r.close();
            p.waitFor();
        } catch (Throwable ignored) {
        }
    }
}
