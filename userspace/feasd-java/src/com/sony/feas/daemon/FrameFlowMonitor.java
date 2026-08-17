package com.sony.feas.daemon;

/**
 * dfps 帧流状态机 —— 事件驱动,v3.1。
 *
 * 输入事件(均由 Binder 线程投递,内部同步,零锁竞争):
 *   onReportFrames(total, avgNs, jankCount)  模块每批帧上报(任意 UI 进程)
 *   onTouch(down)                触摸事件(/dev/input 或 app Binder)
 *   onSetDfps(on)                控制开关(不再轮询文件)
 *
 * 输出:
 *   onChangeRefresh(high)        需要切 120/60Hz 时回调(Main 执行切换)
 *   onBoost()                    boost 事件(帧间隔过大/掉帧),GPU 解除 cap
 *
 * 状态机:
 *   - 收到帧上报:帧在流动 -> 若 60Hz 且 dfps 开,立即切 120Hz
 *   - 收到触摸 down:立即切 120Hz(最低延迟,不等帧)
 *   - avgNs > BOOST_AVG_NS(40fps 以下卡顿)或 jankCount>0 -> boost 3s
 *   - idle 检查(500ms,仅内存时间戳,零文件 IO):
 *       高刷 && 帧停 > FRAME_IDLE_MS && 触摸停 > IDLE_SLACK_MS -> 切 60Hz
 *       掉 60Hz 更晚:FRAME_IDLE_MS=2000 + IDLE_SLACK_MS=4000,减少频繁切换
 *
 * 动画=触摸同调度:
 *   lastActiveMs = max(lastFrameMs, lastTouchMs)。帧上报与触摸同样刷新,
 *   动画期间 GPU floor 401M 生效(kgsl watchdog 消费),动画结束随帧停释放。
 */
public final class FrameFlowMonitor {

    /* ---- 契约常量 ---- */
    public static final long FRAME_IDLE_MS = 2000;      /* 帧停这么久 -> 动画结束 */
    public static final long IDLE_SLACK_MS = 4000;      /* 触摸停这么久兜底(120Hz 保持更久) */
    public static final long IDLE_CHECK_MS = 500;       /* 内存检查周期 */
    public static final long BOOST_AVG_NS = 25_000_000L; /* 帧平均间隔 >25ms(40fps 以下)-> boost */
    public static final long BOOST_HOLD_MS = 3000;      /* boost 保持时间 */

    private final Listener listener;

    private volatile boolean dfpsEnabled = true;
    private volatile boolean highRefresh = false;
    private volatile long lastFrameMs = 0;
    private volatile long lastTouchMs = 0;
    private volatile long lastActiveMs = 0;   /* max(帧,触摸):动画=触摸同调度 */

    /* boost 状态(kgsl watchdog 消费) */
    private volatile boolean boostActive = false;
    private volatile long boostUntilMs = 0;

    /* 最近一次帧上报(供 UI 显示,非热路径) */
    private volatile int lastAvgNs = 0;
    private volatile int lastJankCount = 0;
    private volatile long lastFrameTotal = 0;

    public interface Listener {
        /** 请求切换刷新率。实现方(Main)负责去抖/执行。 */
        void onChangeRefresh(boolean high);
    }

    public FrameFlowMonitor(Listener l) {
        this.listener = l;
        startIdleThread();
    }

    /* ---------------- 事件入口(Binder 线程) ---------------- */

    public void onReportFrames(long frameTotal, int avgNs, int jankCount) {
        long now = System.currentTimeMillis();
        lastFrameTotal = frameTotal;
        lastAvgNs = avgNs;
        lastJankCount = jankCount;
        lastFrameMs = now;
        lastActiveMs = now;               /* 动画=触摸同调度:刷新 active */
        if (avgNs > BOOST_AVG_NS || jankCount > 0) {
            onBoost();
        }
        if (dfpsEnabled && !highRefresh) {
            highRefresh = true;
            listener.onChangeRefresh(true);
        }
    }

    public void onTouch(boolean down) {
        long now = System.currentTimeMillis();
        lastTouchMs = now;
        lastActiveMs = now;
        if (down && dfpsEnabled && !highRefresh) {
            highRefresh = true;
            listener.onChangeRefresh(true);
        }
    }

    /** 内核 sysfs jank 标志(jank sysfs=1)也喂入同一状态机。 */
    public void onKernelJank(boolean jank) {
        if (jank) {
            onBoost();
        }
    }

    private void onBoost() {
        long now = System.currentTimeMillis();
        boostActive = true;
        boostUntilMs = now + BOOST_HOLD_MS;
    }

    public void onSetDfps(boolean on) {
        dfpsEnabled = on;
        if (!on && highRefresh) {
            highRefresh = false;
            listener.onChangeRefresh(false);
        }
    }

    /* ---------------- 控制(供 Main 同步状态) ---------------- */

    public void setHighRefresh(boolean high) {
        highRefresh = high;
    }

    public boolean isHighRefresh() {
        return highRefresh;
    }

    public boolean isDfpsEnabled() {
        return dfpsEnabled;
    }

    /** kgsl watchdog 读取:boost 保持期内返回 true -> GPU 解除 cap(680M)。 */
    public boolean isBoostActive() {
        if (!boostActive) return false;
        if (System.currentTimeMillis() > boostUntilMs) {
            boostActive = false;
            return false;
        }
        return true;
    }

    /** active 时间戳(帧或触摸最近一次):GPU floor 判断,动画=触摸。 */
    public long getLastActiveMs() {
        return lastActiveMs;
    }

    public int getLastAvgNs() {
        return lastAvgNs;
    }

    public int getLastJankCount() {
        return lastJankCount;
    }

    public long getLastFrameTotal() {
        return lastFrameTotal;
    }

    /* ---------------- idle 检查(500ms,仅内存) ---------------- */

    private void startIdleThread() {
        Thread t = new Thread(new Runnable() {
            @Override
            public void run() {
                while (true) {
                    try {
                        Thread.sleep(IDLE_CHECK_MS);
                    } catch (InterruptedException e) {
                        break;
                    }
                    long now = System.currentTimeMillis();
                    if (highRefresh && dfpsEnabled
                            && now - lastFrameMs > FRAME_IDLE_MS
                            && now - lastTouchMs > IDLE_SLACK_MS) {
                        highRefresh = false;
                        listener.onChangeRefresh(false);
                    }
                }
            }
        }, "feasd-flow");
        t.setDaemon(true);
        t.start();
    }
}