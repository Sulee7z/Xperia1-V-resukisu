package com.sony.feas.daemon;

import android.os.Binder;
import android.os.IBinder;
import android.os.Parcel;

import java.lang.reflect.Method;

/**
 * FEAS Binder 服务端(feasd,root)。
 *
 * 契约见 docs/feas_v3_contract.md —— 事务码/Parcel 布局是跨文件唯一事实来源。
 *
 * 注册方式(与 lspd 同构):反射 ServiceManager.addService("feas", this)。
 * app_process 运行时 bootclasspath 含完整 framework,@hide 方法可反射调用;
 * 编译期 android.jar 是 stub,故用反射避免 "method not found" 编译错误。
 *
 * 全部入参事务 oneway 语义由客户端 FLAG_ONEWAY 决定;服务端一律
 * 同步处理并写 reply(oneway 时 reply 被驱动丢弃,无影响)。
 */
public final class FeasBinderService extends Binder {

    /* ---- 契约常量(与 FeasBinderClient.java 严格一致) ---- */
    public static final String SERVICE_NAME = "feas";
    private static final String DESCRIPTOR = "com.sony.feas.daemon.FeasBinderService";
    public static final int TX_REPORT_FRAMES = 1;   /* oneway: long frameTotal; int avgNs; int jankCount */
    public static final int TX_SET_DFPS       = 2;  /* int 0/1 */
    public static final int TX_SET_HMD        = 3;  /* int 0/1 */
    public static final int TX_SET_ENABLED    = 4;  /* int 0/1 */
    public static final int TX_SET_MANUAL_FPS = 5;  /* int 0/60/90/120 */
    public static final int TX_GET_STATE      = 6;  /* reply: int[10] */
    public static final int TX_REPORT_TOUCH   = 7;  /* int 0/1 */

    /** 事件回调,由 Main 提供实现(持有全部 daemon 状态)。 */
    public interface Callbacks {
        void onReportFrames(long frameTotal, int avgNs, int jankCount);
        void onSetDfps(boolean on);
        void onSetHmd(boolean on);
        void onSetEnabled(boolean on);
        void onSetManualFps(int fps);
        void onReportTouch(boolean down);
        /** 供 TX_GET_STATE 读取当前快照:返回 int[9] 或 null 表示不可用。 */
        int[] onGetState();
    }

    /* ---- 统计(daemon 侧累计,经 GET_STATE 供 app 读取;替代内核 diag) ---- */
    private long statsFrameTotal = 0;   /* 最近一次模块上报的真实累计帧数 */
    private long statsReports = 0;      /* 收到的帧上报批数(binder 送达即成功) */
    private long statsFails = 0;        /* binder 帧上报失败计数(客户端侧,经 REPORT 附带) */

    private final Callbacks cb;

    public FeasBinderService(Callbacks cb) {
        this.cb = cb;
    }

    /** 反射注册到 ServiceManager。失败返回 false(调用方决定是否降级)。 */
    public boolean register() {
        try {
            Class<?> sm = Class.forName("android.os.ServiceManager");
            Method add = sm.getMethod("addService", String.class, IBinder.class);
            add.invoke(null, SERVICE_NAME, this);
            Main.log("I", "binder registered: " + SERVICE_NAME);
            return true;
        } catch (Throwable t) {
            Main.log("W", "binder register FAILED: " + t);
            return false;
        }
    }

    @Override
    protected boolean onTransact(int code, Parcel data, Parcel reply, int flags) {
        /* 消费 interface token:客户端 writeInterfaceToken 后 data 读取位置
         * 从 0 开始,必须先跳过 descriptor 才能按契约读取字段 */
        data.enforceInterface(DESCRIPTOR);
        switch (code) {
            case TX_REPORT_FRAMES: {
                long total = data.readLong();
                int avgNs = data.readInt();
                int jank = data.readInt();
                /* daemon 侧统计:binder 送达即成功 */
                statsFrameTotal = total;
                statsReports++;
                cb.onReportFrames(total, avgNs, jank);
                return true;  /* oneway: reply unused */
            }
            case TX_SET_DFPS: {
                cb.onSetDfps(data.readInt() != 0);
                if (reply != null) reply.writeNoException();
                return true;
            }
            case TX_SET_HMD: {
                cb.onSetHmd(data.readInt() != 0);
                if (reply != null) reply.writeNoException();
                return true;
            }
            case TX_SET_ENABLED: {
                cb.onSetEnabled(data.readInt() != 0);
                if (reply != null) reply.writeNoException();
                return true;
            }
            case TX_SET_MANUAL_FPS: {
                cb.onSetManualFps(data.readInt());
                if (reply != null) reply.writeNoException();
                return true;
            }
            case TX_GET_STATE: {
                int[] st = cb.onGetState();
                if (reply == null) return true;
                reply.writeNoException();
                if (st == null) {
                    reply.writeInt(0);
                    return true;
                }
                for (int v : st) reply.writeInt(v);
                /* 追加统计:int[10] = {enabled, dfps, hmd, manualFps, targetFps,
                 * highRefresh, lastFrameTotal, frameTotal, reports, fails} */
                reply.writeInt((int) statsFrameTotal);
                reply.writeInt((int) Math.min(statsReports, Integer.MAX_VALUE));
                reply.writeInt((int) Math.min(statsFails, Integer.MAX_VALUE));
                return true;
            }
            case TX_REPORT_TOUCH: {
                cb.onReportTouch(data.readInt() != 0);
                if (reply != null) reply.writeNoException();
                return true;
            }
            default:
                try {
                    return super.onTransact(code, data, reply, flags);
                } catch (android.os.RemoteException e) {
                    return false;
                }
        }
    }
}