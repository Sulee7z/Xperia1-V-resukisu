/*
 * Copyright 2017 Sony Corporation
 */

package com.sonyericsson.idd.api;

import android.os.SystemProperties;

/**
 * Logging utilities for IDD Java API.
 *
 */
public class Log {

    private Log(String tag, String subtag, boolean debug) {
        mTag = tag;
        mSubtag = subtag + ": ";
        mDebug = isDebugLogEnabled() || debug;
    }

    public void d(String msg) {
        if (mDebug) {
            if (mSubtag != null) {
                android.util.Log.d(mTag, mSubtag + msg);
            } else {
                android.util.Log.d(mTag, msg);
            }
        }
    }

    public void d(String msg, Throwable tr) {
        if (mDebug) {
            if (mSubtag != null) {
                android.util.Log.d(mTag, mSubtag + msg, tr);
            } else {
                android.util.Log.d(mTag, msg, tr);
            }
        }
    }

    public void i(String msg) {
        if (mSubtag != null) {
            android.util.Log.i(mTag, mSubtag + msg);
        } else {
            android.util.Log.i(mTag, msg);
        }
    }

    public void i(String msg, Throwable tr) {
        if (mSubtag != null) {
            android.util.Log.i(mTag, mSubtag + msg, tr);
        } else {
            android.util.Log.i(mTag, msg, tr);
        }
    }

    public void e(String msg) {
        if (mSubtag != null) {
            android.util.Log.e(mTag, mSubtag + msg);
        } else {
            android.util.Log.e(mTag, msg);
        }
    }

    public void e(String msg, Throwable tr) {
        if (mSubtag != null) {
            android.util.Log.e(mTag, mSubtag + msg, tr);
        } else {
            android.util.Log.e(mTag, msg, tr);
        }
    }

    public void w(String msg) {
        if (mSubtag != null) {
            android.util.Log.w(mTag, mSubtag + msg);
        } else {
            android.util.Log.w(mTag, msg);
        }
    }

    public void w(String msg, Throwable tr) {
        if (mSubtag != null) {
            android.util.Log.w(mTag, mSubtag + msg, tr);
        } else {
            android.util.Log.w(mTag, msg, tr);
        }
    }

    public void v(String msg) {
        if (mSubtag != null) {
            android.util.Log.v(mTag, mSubtag + msg);
        } else {
            android.util.Log.v(mTag, msg);
        }
    }

    public static Log createLog(String tag) {
        return new Log(LOG_TAG, tag, false);
    }

    public static Log createLog(String tag, boolean debug) {
        return new Log(LOG_TAG, tag, debug);
    }

    public static Log createLog(Object o) {
        return createLog(o.getClass());
    }

    @SuppressWarnings("unchecked")
    public static Log createLog(Class clazz) {
        return new Log(LOG_TAG, clazz.getSimpleName(), false);
    }

    private static boolean isDebugLogEnabled() {
        return SystemProperties.get("idd.debug.log", "0").equals("1");
    }

    private final String mTag;

    private String mSubtag;

    private boolean mDebug;

    private static final String LOG_TAG = "idd-api";
}
