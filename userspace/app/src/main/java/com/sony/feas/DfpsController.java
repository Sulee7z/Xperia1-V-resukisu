package com.sony.feas;

import android.util.Log;

/**
 * dfps (dynamic refresh rate) controller for FEAS —— v3.1 Binder-only。
 *
 * v3.1:daemon Binder 注册已确认成功,删除 su 兜底(用户要求)。
 * 控制一律走 TX_SET_DFPS,binder 不可用时静默失败(不 spawn su)。
 */
public final class DfpsController {

    private static final String TAG = "FEAS-DFPS";

    private static volatile boolean enabled = true;

    private DfpsController() {}

    /** Enable/disable dfps。Binder-only,失败静默(不 spawn su)。 */
    public static void setEnabled(boolean on) {
        enabled = on;
        if (FeasBinderClient.setDfps(on)) {
            Log.i(TAG, "dfps " + (on ? "enabled" : "disabled") + " (binder)");
        } else {
            Log.w(TAG, "dfps " + (on ? "enabled" : "disabled")
                    + " FAILED (daemon binder 不可用)");
        }
    }

    public static boolean isEnabled() {
        return enabled;
    }

    /* ---------- 不再使用(daemon 事件驱动,app 无需轮询) ---------- */

    /** @deprecated daemon 直接监听 /dev/input,app 无需上报。 */
    public static void onFrameActivity() {}

    /** @deprecated daemon 状态机自行判定 idle。 */
    public static void onIdle() {}

    /** @deprecated 状态经 FeasBinderClient.getState() 查询。 */
    public static boolean isHighRefresh() {
        return false;
    }
}