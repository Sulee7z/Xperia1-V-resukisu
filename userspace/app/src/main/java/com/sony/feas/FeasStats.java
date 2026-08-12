package com.sony.feas;

import java.util.concurrent.atomic.AtomicLong;

/**
 * FEAS 运行统计(轻量,原子计数,无锁无开销)。
 */
public final class FeasStats {

    private static final AtomicLong frameCount = new AtomicLong();
    private static final AtomicLong reportOk = new AtomicLong();
    private static final AtomicLong reportFail = new AtomicLong();

    private FeasStats() {}

    public static long getFrameCount() { return frameCount.get(); }
    public static long getReportOk() { return reportOk.get(); }
    public static long getReportFail() { return reportFail.get(); }

    static void incFrame() { frameCount.incrementAndGet(); }
    static void incOk() { reportOk.incrementAndGet(); }
    static void incFail() { reportFail.incrementAndGet(); }
}
