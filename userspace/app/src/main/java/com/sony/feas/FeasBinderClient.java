package com.sony.feas;

import android.os.IBinder;
import android.os.Parcel;
import android.util.Log;

import java.lang.reflect.Method;

/**
 * FEAS Binder 客户端 —— app 与 Xposed module 共用。
 *
 * 契约见 docs/feas_v3_contract.md。
 * 服务名 "feas" 由 root daemon(feasd)注册;本类反射获取。
 *
 * 使用要点:
 *  - REPORT_FRAMES 用 FLAG_ONEWAY(异步,不阻塞 Choreographer 线程)
 *  - 其他控制事务同步等待 reply
 *  - binder 不可用时 isConnected()=false,调用方静默降级(不崩溃)
 */
public final class FeasBinderClient {

    private static final String TAG = "FEAS-BINDER";

    /* ---- 契约常量(与 FeasBinderService.java 严格一致) ---- */
    public static final String SERVICE_NAME = "feas";
    public static final int TX_REPORT_FRAMES = 1;   /* oneway: long frameTotal; int avgNs; int jankCount */
    public static final int TX_SET_DFPS       = 2;  /* int 0/1 */
    public static final int TX_SET_HMD        = 3;  /* int 0/1 */
    public static final int TX_SET_ENABLED    = 4;  /* int 0/1 */
    public static final int TX_SET_MANUAL_FPS = 5;  /* int 0/60/90/120 */
    public static final int TX_GET_STATE      = 6;  /* reply: int[7] */
    public static final int TX_REPORT_TOUCH   = 7;  /* int 0/1 */

    private static volatile IBinder binder;
    private static volatile boolean lookupFailed = false;
    private static long lastLookupMs = 0;

    private FeasBinderClient() {}

    /** 获取服务(带 30s 冷却的重试,避免热路径反复反射)。 */
    public static IBinder getService() {
        IBinder b = binder;
        if (b != null && b.isBinderAlive()) return b;
        long now = System.currentTimeMillis();
        if (lookupFailed && now - lastLookupMs < 30_000) return null;
        lastLookupMs = now;
        try {
            Class<?> sm = Class.forName("android.os.ServiceManager");
            Method get = sm.getMethod("getService", String.class);
            b = (IBinder) get.invoke(null, SERVICE_NAME);
            if (b != null && b.isBinderAlive()) {
                binder = b;
                lookupFailed = false;
                return b;
            }
        } catch (Throwable ignored) {
        }
        lookupFailed = true;
        return null;
    }

    public static boolean isConnected() {
        return getService() != null;
    }

    /**
     * 帧上报(oneway,异步)。失败静默(调用方降级到 sysfs 通道)。
     * 调用频率:每 FRAME_BATCH=15 帧一次,非每帧。
     */
    public static void reportFrames(long frameTotal, int avgNs, int jankCount) {
        IBinder b = getService();
        if (b == null) return;
        Parcel data = Parcel.obtain();
        try {
            data.writeInterfaceToken("com.sony.feas.daemon.FeasBinderService");
            data.writeLong(frameTotal);
            data.writeInt(avgNs);
            data.writeInt(jankCount);
            b.transact(TX_REPORT_FRAMES, data, null, IBinder.FLAG_ONEWAY);
        } catch (Throwable ignored) {
            /* oneway 失败:静默降级 */
        } finally {
            data.recycle();
        }
    }

    /** 触摸事件(可选增强通道;feasd 本身也监听 /dev/input)。 */
    public static void reportTouch(boolean down) {
        IBinder b = getService();
        if (b == null) return;
        Parcel data = Parcel.obtain();
        try {
            data.writeInterfaceToken("com.sony.feas.daemon.FeasBinderService");
            data.writeInt(down ? 1 : 0);
            b.transact(TX_REPORT_TOUCH, data, null, IBinder.FLAG_ONEWAY);
        } catch (Throwable ignored) {
        } finally {
            data.recycle();
        }
    }

    /** 设置 dfps 开关。返回是否成功送达。 */
    public static boolean setDfps(boolean on) {
        return transactInt(TX_SET_DFPS, on ? 1 : 0);
    }

    /** 设置 240Hz MBR 开关。 */
    public static boolean setHmd(boolean on) {
        return transactInt(TX_SET_HMD, on ? 1 : 0);
    }

    /** 设置模块总开关。 */
    public static boolean setEnabled(boolean on) {
        return transactInt(TX_SET_ENABLED, on ? 1 : 0);
    }

    /** 设置手动目标帧率(0=自动)。 */
    public static boolean setManualFps(int fps) {
        return transactInt(TX_SET_MANUAL_FPS, fps);
    }

    private static boolean transactInt(int code, int value) {
        IBinder b = getService();
        if (b == null) return false;
        Parcel data = Parcel.obtain();
        Parcel reply = Parcel.obtain();
        try {
            data.writeInterfaceToken("com.sony.feas.daemon.FeasBinderService");
            data.writeInt(value);
            b.transact(code, data, reply, 0);
            return true;
        } catch (Throwable t) {
            return false;
        } finally {
            data.recycle();
            reply.recycle();
        }
    }

    /**
     * 读取状态快照:int[10] = {enabled, dfps, hmd, manualFps, targetFps,
     * highRefresh, lastFrameTotal, frameTotal, reports, fails}。
     * daemon 端 reply 顺序严格为此;binder 不可用或失败返回 null。
     */
    public static int[] getState() {
        IBinder b = getService();
        if (b == null) return null;
        Parcel data = Parcel.obtain();
        Parcel reply = Parcel.obtain();
        try {
            data.writeInterfaceToken("com.sony.feas.daemon.FeasBinderService");
            b.transact(TX_GET_STATE, data, reply, 0);
            reply.readException();
            int[] st = new int[10];
            for (int i = 0; i < 10; i++) {
                st[i] = reply.readInt();
            }
            return st;
        } catch (Throwable t) {
            return null;
        } finally {
            data.recycle();
            reply.recycle();
        }
    }

    /* ---------------- 调试 ---------------- */

    static void logIfMissing() {
        if (!isConnected()) {
            Log.i(TAG, "daemon binder not connected (daemon not running?)");
        }
    }
}