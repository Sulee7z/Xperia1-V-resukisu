package com.sony.feas;

/**
 * 帧时间测量 —— 统一测量点,消除 v2.x 的混乱。
 *
 * 设计(契约 3.1):
 *  - 在 Choreographer 线程串行调用,无锁无分配(热路径零开销)
 *  - EMA 自适应基准:60Hz(16.7ms)与 120Hz(8.3ms)自动适应,无需静态阈值
 *  - jank 检测:单帧间隔 > EMA×1.8 计一次掉帧,保留掉帧信息(不被批平均抹平)
 *  - 每 FRAME_BATCH 帧 flush 一次:产出 avgNs + jankCount,重置批计数
 *
 * 消费者:
 *  - 内核:frame sysfs(avgNs,驱动 perf_anim_active/CPU 调度)
 *  - daemon:Binder REPORT_FRAMES(frameTotal, avgNs, jankCount)
 *    (jankCount>0 -> daemon 立即 GPU 升频 680M,不等内核 sysfs)
 */
public final class FrameTimeTracker {

    /* ---- 契约常量 ---- */
    public static final int FRAME_BATCH = 15;        /* 批大小(60Hz:250ms/批) */
    public static final double JANK_MULT = 1.8;      /* 单帧 > EMA*1.8 -> jank */
    public static final double EMA_ALPHA = 0.1;      /* EMA 平滑系数 */

    /* 热路径状态(Choreographer 线程独占,无需 volatile) */
    private long lastVsyncNs = 0;
    private long emaNs = 0;          /* EMA 基准(首个间隔直接作为初值) */
    private boolean emaInit = false;
    private long batchSumNs = 0;
    private int batchCount = 0;
    private int batchJank = 0;
    private long frameTotal = 0;

    /* 最近一次 flush 结果(供读取,Binder 上报线程可见) */
    private volatile int lastAvgNs = 0;
    private volatile int lastJankCount = 0;

    public FrameTimeTracker() {
    }

    /**
     * 记录一帧(onVsync 时刻,由 FeasModule.FrameHooker 调用)。
     * 单线程串行,纯算术,无分配。
     */
    public void record(long nowNs) {
        if (lastVsyncNs != 0) {
            long durationNs = nowNs - lastVsyncNs;
            if (durationNs > 0 && durationNs < 5_000_000_000L) {
                if (!emaInit) {
                    emaNs = durationNs;
                    emaInit = true;
                } else {
                    /* EMA 平滑:滤掉单帧噪声,自适应 60/120Hz */
                    emaNs = (long) (emaNs * (1.0 - EMA_ALPHA)
                            + durationNs * EMA_ALPHA);
                }
                if (durationNs > emaNs * JANK_MULT) {
                    batchJank++;
                }
                batchSumNs += durationNs;
                batchCount++;
                frameTotal++;
            }
        }
        lastVsyncNs = nowNs;
    }

    /**
     * 帧数是否达到一批(flush 判定)。调用方在返回 true 后调用 flush()。
     */
    public boolean isBatchFull() {
        return batchCount >= FRAME_BATCH;
    }

    /**
     * 结算本批:返回本批平均间隔(ns),并重置批计数。
     * jank 数经 getAndResetJank() 读取。
     */
    public long flush() {
        long avgNs = 0;
        if (batchCount > 0) {
            avgNs = batchSumNs / batchCount;
        }
        lastAvgNs = (int) avgNs;
        lastJankCount = batchJank;
        batchSumNs = 0;
        batchCount = 0;
        batchJank = 0;
        return avgNs;
    }

    /** 最近一次 flush 的平均间隔(ns),0=无数据。 */
    public int getLastAvgNs() {
        return lastAvgNs;
    }

    /** 最近一次 flush 的掉帧数。 */
    public int getLastJankCount() {
        return lastJankCount;
    }

    /** 累计帧数(Binder 上报 frameTotal 用)。 */
    public long getFrameTotal() {
        return frameTotal;
    }
}