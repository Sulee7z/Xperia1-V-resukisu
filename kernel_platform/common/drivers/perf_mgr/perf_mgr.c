// SPDX-License-Identifier: GPL-2.0
/*
 * perf_mgr.c - FPS-driven energy-aware frequency governor
 *
 * Ported from Samsung GPIS (sm8150-samsung perf_mgr) to GKI 5.15.
 * Adapted for Sony Xperia 1 V (SM8550):
 *   - No private task_struct fields (drawing_flag/drawing_mig_boost)
 *   - No panel notifier dependency; target FPS from userspace ioctl
 *   - Per-task state kept in internal list (rcu + spinlock)
 *   - Uses scheduler per-entity util (cpu_util_cfs) via task->se
 *
 * Userspace (Vector module) reports every rendered frame:
 *   ioctl(fd, PERF_MGR_FRAME_END, {tid, duration_ns})
 * Kernel computes required capacity to hit target FPS and
 * nudges the schedutil governor via sugov update / freq_qos.
 */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/sched/loadavg.h>
#include <linux/sched/task.h>
#include <linux/sched/topology.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/pm_qos.h>
#include <linux/suspend.h>
#include <linux/devfreq.h>
#include <linux/of.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include "perf_mgr.h"

#define PFX "[perfmgr] "

struct perf_task {
	struct list_head list;
	struct rcu_head rcu;
	pid_t tid;
	int group_id;
	unsigned long updated_fps_util;
	int last_update_frame;
	int running_cpu;
};

static LIST_HEAD(perf_task_list);
static spinlock_t perf_list_lock;
static int perf_task_count;

/* Tunables */
static int g_fps = 60;               /* target FPS from userspace */
static unsigned long us_frame_time = 16666;  /* 1e6 / g_fps */
static int fps_margin_percent = 30;
static int frame_hold_ms = 120;            /* time window: reset util if no frame within 120ms */
static bool perfmgr_enable = true;
static struct freq_qos_request perfmgr_qos[NR_CPUS];
static struct freq_qos_request perfmgr_qos_cap[NR_CPUS]; /* per-policy FREQ_QOS_MAX */
static unsigned long perfmgr_qos_max[NR_CPUS];  /* per-policy max freq */
static int perfmgr_qos_count;

/* ---- Cluster config (SM8550 / Xperia 1 V) ----
 * A510 little (cpu0-2): disable 2 of them (hot & slow)
 * A715/A720 mid:       energy-efficient ceiling 1536000 Hz
 * X3 big:              energy-efficient ceiling 1708800 Hz
 */
static unsigned long little_freq_cap = 1228800;  /* A510 eff max */
static unsigned long mid_freq_cap = 1536000;    /* A715/A720 eff max */
static unsigned long big_freq_cap = 1708800;    /* X3 eff max */
static unsigned long gpu_touch_cap = 680000000;  /* GPU max while touching (jank escape) */
static unsigned long gpu_idle_cap = 124800000;  /* GPU max when fully idle / screen off (lowest) */
static unsigned long gpu_boost_min = 401000000; /* GPU touch boost floor 401MHz (220/401/475/550) */

/* ---- Adaptive GPU: extreme power saving with scene memory ----
 * Idle -> lowest level. Otherwise the GPU cap DRIFTS DOWN one level at a
 * time while frames stay in budget (max energy saving), and jumps back UP
 * one level the moment a frame drops (>2x budget). It therefore converges
 * to the lowest level that keeps frames on time - no fixed baseline, no
 * hard-coded frequencies, no polling (driven by frame events).
 * Learned levels are remembered per load-bucket: a scene that was already
 * tuned is served at its remembered level directly and is NOT re-learned
 * (it also stops drifting below its known-good level). ---- */
static unsigned long fps_floor_util;   /* fwd tentative def (defined below) */
#define GPU_TABLE_MAX 32
#define GPU_MEM_BUCKETS 16
#define GPU_DRIFT_FRAMES 24             /* in-budget frames before drifting down */
#define GPU_RAISE_HOLD_MS 2000          /* no drifting right after a raise */
static unsigned long gpu_freq_table[GPU_TABLE_MAX];
static int gpu_freq_count;
static int gpu_learn_inited;
static int gpu_learn_idx;              /* current baseline index into table */
static int gpu_mem[GPU_MEM_BUCKETS];   /* bucket -> known-good level idx (-1 unknown) */
static int gpu_ok_frames;
static unsigned long gpu_last_raise;
#define GPU_LEARN_SETTLE_MS 5000

/* Read the device GPU level table at runtime (no hard-coded list). */
static void gpu_table_init(void)
{
	struct file *f;
	char buf[256];
	loff_t pos = 0;
	ssize_t rd;
	char *p;
	unsigned long v;
	int i, j;

	if (gpu_freq_count)
		return;
	f = filp_open("/sys/class/kgsl/kgsl-3d0/gpu_available_frequencies",
		      O_RDONLY, 0);
	if (IS_ERR(f))
		return;
	rd = kernel_read(f, buf, sizeof(buf) - 1, &pos);
	filp_close(f, NULL);
	if (rd <= 0)
		return;
	buf[rd] = '\0';
	p = buf;
	while (*p && gpu_freq_count < GPU_TABLE_MAX) {
		while (*p == ' ' || *p == '\n')
			p++;
		v = simple_strtoul(p, &p, 10);
		if (v)
			gpu_freq_table[gpu_freq_count++] = v;
	}
	if (gpu_freq_count < 2)
		return;
	/* ascending insertion sort (table is short) */
	for (i = 1; i < gpu_freq_count; i++) {
		v = gpu_freq_table[i];
		j = i - 1;
		while (j >= 0 && gpu_freq_table[j] > v) {
			gpu_freq_table[j + 1] = gpu_freq_table[j];
			j--;
		}
		gpu_freq_table[j + 1] = v;
	}
	pr_info(PFX "GPU table loaded count=%d min=%lu max=%lu\n",
		gpu_freq_count, gpu_freq_table[0],
		gpu_freq_table[gpu_freq_count - 1]);
}

static void gpu_learn_init(void)
{
	int i;

	gpu_table_init();
	if (gpu_freq_count < 2)
		return;
	for (i = 0; i < GPU_MEM_BUCKETS; i++)
		gpu_mem[i] = -1;
	/* start at the second-lowest non-idle level; drift handles the rest */
	gpu_learn_idx = (gpu_freq_count > 2) ? 2 : 0;
	gpu_learn_inited = 1;
}

/* load bucket: frame duration relative to the frame budget */
static int gpu_load_bucket(unsigned long long duration_ns,
			   unsigned long long budget)
{
	unsigned long long ratio;

	if (!budget)
		return 8;
	ratio = duration_ns / (budget / GPU_MEM_BUCKETS + 1);
	if (ratio >= GPU_MEM_BUCKETS)
		ratio = GPU_MEM_BUCKETS - 1;
	return (int)ratio;
}

/* Event-driven learning on every frame report. Returns 1 when the cap
 * changed and should be re-applied. */
static int perf_gpu_adjust(unsigned long long duration_ns,
			   unsigned long us_frame_time)
{
	unsigned long long budget = (unsigned long long)us_frame_time * 1000UL;
	int bucket;

	if (!gpu_learn_inited)
		gpu_learn_init();
	if (!budget || gpu_freq_count < 2)
		return 0;
	bucket = gpu_load_bucket(duration_ns, budget);

	if (duration_ns > budget * 3) {
		/* severe frame drop (>3x budget): raise one level, remember it */
		if (gpu_learn_idx < gpu_freq_count - 1)
			gpu_learn_idx++;
		gpu_mem[bucket] = gpu_learn_idx;
		gpu_ok_frames = 0;
		gpu_last_raise = jiffies;
		return 1;
	} else if (duration_ns <= budget) {
		/* in budget: drift down fast (no hold) for max energy saving */
		if (++gpu_ok_frames >= 10) {
			gpu_ok_frames = 0;
			if (gpu_learn_idx > 0 &&
			    (gpu_mem[bucket] < 0 ||
			     gpu_learn_idx > gpu_mem[bucket])) {
				gpu_learn_idx--;
				return 1;
			}
		}
	} else {
		/* borderline (1x-3x): reset drift counter */
		gpu_ok_frames = 0;
	}
	return 0;
}

/* Frame load -> GPU cap. Table not ready -> 0 (do not force anything; the
 * GPU stays on its native/devfreq behavior until learning is available). */
static unsigned long perf_gpu_cap_for_util(unsigned long util)
{
	int idx;

	if (!gpu_learn_inited)
		gpu_learn_init();
	if (gpu_freq_count < 2)
		return 0;
	if (util > 1024)
		util = 1024;
	idx = gpu_learn_idx + util / 256;
	if (idx >= gpu_freq_count)
		idx = gpu_freq_count - 1;
	return gpu_freq_table[idx];
}

/* Apply the adaptive cap if learning is ready; otherwise leave the GPU
 * to its native behavior (no hard fallback). */
static void perf_gpu_set_max(unsigned long max_hz);   /* fwd decl */
static void perf_gpu_apply_cap(void)
{
	unsigned long cap = perf_gpu_cap_for_util(READ_ONCE(fps_floor_util));

	if (cap)
		perf_gpu_set_max(cap);
}
/* Touch boost targets per cluster (energy-friendly, not max):
 * [0]=A510 little, [1]=A715/A720 mid, [2]=X3 big */
static struct notifier_block perf_pm_nb;
static struct dev_pm_qos_request gpu_max_qos;   /* hard GPU ceiling via PM QoS */
static bool gpu_max_qos_active;
static bool gpu_max_qos_failed;
static unsigned long perfmgr_qos_min[NR_CPUS];  /* per-policy lowest freq (idle clamp) */
static unsigned long perfmgr_qos_cap_orig[NR_CPUS]; /* cluster cap to restore after idle */
static bool perfmgr_qos_registered[NR_CPUS]; /* per-policy qos added flag */
static int perfmgr_qos_cluster[NR_CPUS];      /* 0=A510 1=mid 2=big */
static int perfmgr_qos_cpu[NR_CPUS];         /* representative cpu per slot */
static struct notifier_block perf_cpufreq_nb;
static unsigned long last_touch_jiffies;
static struct delayed_work perf_idle_wq;        /* idle -> lowest freq clamp */

/* GPU touch boost */
static struct devfreq *perf_gpu_devfreq;
static unsigned long touch_gpu_mhz = 550;  /* touch boost GPU min freq (MHz) */
static int touch_hold_ms = 2000;           /* hold after finger lift (ms) */
static int touch_mid_delay_ms = 500;       /* drop to mid level after this long of continuous touch (iOS-style) */
static bool perf_touch_boosted;
static bool perf_touch_in_mid;
/* App cold-launch boost: one-shot sysfs write -> TOUCH caps for
 * LAUNCH_BOOST_MS, auto-released by delayed work. Zero residency. */
#define LAUNCH_BOOST_MS 2500
static bool perf_launch_boosted;
static struct delayed_work perf_launch_work;
static unsigned long perf_touch_util;   /* touch-driven floor util (0/600/1024) */
static unsigned long fps_floor_util;    /* frame-driven floor util (0-1024) */
static unsigned long last_frame_jiffies;   /* jiffies of last frame report */
static bool perf_anim_active;   /* animation in progress (frames flowing) */
static bool perf_jank;           /* frame over budget -> jank detected */
static int perf_ok_frames;       /* consecutive in-budget frames */
static int perf_jank_frames;      /* consecutive over-budget frames */
static struct delayed_work perf_floor_decay_wq;
static atomic_t perf_touch_gen = ATOMIC_INIT(0);
static int perf_touch_mid_gen;   /* gen the mid work was scheduled for */
static struct work_struct perf_touch_boost_work;
static struct delayed_work perf_touch_mid_work;
static struct delayed_work perf_touch_work;

/* Diagnostics */
static atomic_t frame_reports = ATOMIC_INIT(0);   /* frames reported to kernel */
static atomic_t frame_ok = ATOMIC_INIT(0);        /* frames actually applied */
static atomic_t frame_total = ATOMIC_INIT(0);     /* real frame count from module */
static atomic_t touch_events = ATOMIC_INIT(0);
static atomic_t touch_press_count = ATOMIC_INIT(0);
static atomic_t input_connect_calls = ATOMIC_INIT(0);
static atomic_t input_connect_ok = ATOMIC_INIT(0);
static int perf_gpu_found = -1;   /* -1 unknown, 0 not found, 1 found */
static int perf_handler_registered = -1;  /* -1 unknown, 0 fail, 1 ok */

static void perf_mgr_apply_freq(unsigned long util);
static void perf_set_fps(int fps);
static void perf_frame_apply_max(void);
static void perf_qos_register_policy(struct cpufreq_policy *p);
static void perf_gpu_set_min(unsigned long min_hz);
static void perf_gpu_set_max(unsigned long max_hz);
static void perf_fps_gpu_floor(unsigned long eff);

/* Scheduler interfaces (kernel internal, 5.15) */
extern unsigned long cpu_util_cfs(int cpu);

static unsigned long perf_calc_required_util(unsigned long rn_sum,
					     unsigned long dur)
{
	unsigned long required_cap;
	unsigned long us_scale_dur = dur / 1000;  /* ns -> us */
	unsigned long required_rate = 0;
	unsigned long margin = 0;

	if (fps_margin_percent > 0)
		margin = (us_frame_time * fps_margin_percent) / 100;

	if (g_fps == 0 || us_frame_time == 0)
		return 0;

	/* 余量不能超过帧时间一半(避免除零/负值) */
	if (margin >= us_frame_time / 2)
		margin = us_frame_time / 2;

	required_rate = (us_scale_dur * FP_SCALE) / (us_frame_time - margin);

	if (required_rate <= (1 * FP_SCALE))
		return 0;

	required_cap = required_rate * rn_sum / FP_SCALE;
	if (required_cap > 1024)
		required_cap = 1024;

	return required_cap;
}

static struct perf_task *perf_get_task(pid_t tid)
{
	struct perf_task *fi = NULL, *ret = NULL;

	if (list_empty(&perf_task_list))
		return NULL;

	rcu_read_lock();
	list_for_each_entry_rcu(fi, &perf_task_list, list) {
		if (fi->tid == tid) {
			ret = fi;
			break;
		}
	}
	rcu_read_unlock();

	return ret;
}

/* Get task utilization from scheduler (PELT, capped at 1024) */
static unsigned long perf_task_util(struct task_struct *p)
{
	unsigned long util;

	if (!p)
		return 0;

	util = READ_ONCE(p->se.avg.util_avg);
	return min(util, 1024UL);
}

/* Highest frame-driven util across all registered tasks */
static unsigned long perf_fps_floor_recalc(void)
{
	unsigned long m = 0;
	struct perf_task *fi;

	rcu_read_lock();
	list_for_each_entry_rcu(fi, &perf_task_list, list)
		m = max(m, READ_ONCE(fi->updated_fps_util));
	rcu_read_unlock();
	return m;
}

/* Frame floor smoothing: rise instantly, decay at most 25% per frame.
 * Prevents frequency churn from frame-duration jitter while keeping the
 * floor immediately responsive when rendering gets heavy (no jank).
 */
static unsigned long perf_fps_smooth(unsigned long new_cap)
{
	unsigned long cur = READ_ONCE(fps_floor_util);

	if (new_cap >= cur)
		return new_cap;
	return max(new_cap, cur - cur / 4);
}

/* Apply effective frequency floor = max(touch util, frame-driven floor).
 * Proportional mapping eff/1024 * max_freq: frame reports get exactly the
 * capacity they need (energy-lean), touch keeps its iOS-style levels
 * (1024 -> max, 600 -> ~60%).
 */
static void perf_mgr_apply_freq(unsigned long util)
{
	/* Frequency is controlled via policy->max (perf_apply_cluster);
	 * MIN floors are not used anymore. */
}

/* ---- Kernel-side cluster frequency control (direct policy->max) ----
 * Animation: cap every cluster's policy->max (mid renders, little/big
 * standby) + pin cpuset top-app/foreground to mid cores (3-6) + GPU.
 * Restore: hard caps + full cpuset. Direct policy writes + update_policy
 * force the governor to re-run limits immediately.
 */
/* 3-state cluster policy:
 *   IDLE   - no touch, no animation: little stays low (background only)
 *   ANIM   - animation detected (no touch): mid renders, little/big standby
 *   TOUCH  - touch active: allow exceeding ANIM caps (jank escape hatch)
 */
#define CLUSTER_IDLE_L 768000UL
#define CLUSTER_IDLE_M 1228800UL
#define CLUSTER_IDLE_B 1401600UL
#define CLUSTER_ANIM_L 768000UL
#define CLUSTER_ANIM_M 1401600UL
#define CLUSTER_ANIM_B 864000UL
#define CLUSTER_TOUCH_L 1228800UL
#define CLUSTER_TOUCH_M 1536000UL
#define CLUSTER_TOUCH_B 1708800UL

enum perf_cluster_state { CLUSTER_IDLE = 0, CLUSTER_ANIM, CLUSTER_TOUCH };

static unsigned long perf_cluster_target(int state, unsigned long max_freq)
{
	int little = (max_freq <= 2100000);
	int mid = (max_freq <= 2900000);

	switch (state) {
	case CLUSTER_ANIM:
		return little ? CLUSTER_ANIM_L : (mid ? CLUSTER_ANIM_M : CLUSTER_ANIM_B);
	case CLUSTER_TOUCH:
		return little ? CLUSTER_TOUCH_L : (mid ? CLUSTER_TOUCH_M : CLUSTER_TOUCH_B);
	default:
		return little ? CLUSTER_IDLE_L : (mid ? CLUSTER_IDLE_M : CLUSTER_IDLE_B);
	}
}

static void perf_policy_set_max(struct cpufreq_policy *p, unsigned long max)
{
	if (!p || p->max == max)
		return;
	p->max = max;
	cpufreq_update_policy(p->cpu);
}

static void perf_apply_cluster(int state)
{
	int cpu;

	for_each_online_cpu(cpu) {
		struct cpufreq_policy *p = cpufreq_cpu_get(cpu);
		unsigned long want;

		if (!p)
			continue;
		want = perf_cluster_target(state, p->cpuinfo.max_freq);

		/* sync the freq_qos MAX request too: cpufreq_update_policy()
		 * verifies policy->max against constraints, and our hard-cap
		 * FREQ_QOS_MAX would otherwise yank it back to 1228/1536/1708
		 * every time we lower it for animation peak. */
		if (perfmgr_qos_registered[p->cpu]) {
			int idx;

			for (idx = 0; idx < perfmgr_qos_count; idx++) {
				if (perfmgr_qos_cpu[idx] == p->cpu) {
					/* skip if already at this level */
					if (freq_qos_read_value(&p->constraints,
								FREQ_QOS_MAX) != want)
						freq_qos_update_request(
							&perfmgr_qos_cap[idx],
							want);
					break;
				}
			}
		}
		trace_printk(PFX "cluster cpu%d state=%d want=%lu qosmax=%d\n",
			     p->cpu, state, want,
			     freq_qos_read_value(&p->constraints, FREQ_QOS_MAX));
		perf_policy_set_max(p, want);
		trace_printk(PFX "cluster cpu%d after: max=%lu\n",
			     p->cpu, p->max);
		cpufreq_cpu_put(p);
	}
}

/* Touch boost: mark state; perf_mgr_get_target_freq() returns the
 * moderate per-cluster target and perf_apply_state() forces the governor.
 */
static void perf_touch_apply_boost(unsigned long lvl)
{
	perf_apply_cluster(lvl ? CLUSTER_TOUCH : CLUSTER_ANIM);
}

/* Over-limit escape on detected jank (touch or pure animation) */
static bool perf_allow_overlimit(void)
{
	/* Lift caps on real jank regardless of who renders: touch or a
	 * pure animation both get the escape when frames over budget. */
	return READ_ONCE(perf_jank) &&
	       (READ_ONCE(perf_touch_boosted) || READ_ONCE(perf_anim_active));
}

/* Animation frame need -> perf_mgr_get_target_freq() maps fps_floor_util
 * to a fraction of the cluster cap; refresh the governor.
 */
static void perf_frame_apply_max(void)
{
	/* State follows frame FLOW (perf_anim_active), not the momentary
	 * fps_floor_util: a single fast frame must never drop us to IDLE
	 * while the animation is still playing (that early downclock was
	 * the visible stutter). Only the decay worker (500ms silence)
	 * clears perf_anim_active -> IDLE.
	 * Real jank (3 consecutive frames over budget) temporarily lifts
	 * to CLUSTER_TOUCH; the next in-budget frame returns to ANIM. */
	if (READ_ONCE(perf_launch_boosted)) {
		/* cold-launch window: keep TOUCH caps regardless of frame flow */
		perf_apply_cluster(CLUSTER_TOUCH);
		return;
	}
	if (READ_ONCE(perf_anim_active)) {
		if (READ_ONCE(perf_jank))
			perf_apply_cluster(CLUSTER_TOUCH);
		else
			perf_apply_cluster(CLUSTER_ANIM);
	} else {
		perf_apply_cluster(CLUSTER_IDLE);   /* no frames: idle */
	}
}

/* Recompute frame floor from task list, smooth it, and re-apply frequency.
 * Call after any updated_fps_util change (frame report or timeout reset).
 */
static void perf_fps_apply_floor(void)
{
	unsigned long cap;
	unsigned long eff;

	cap = perf_fps_floor_recalc();
	WRITE_ONCE(fps_floor_util, perf_fps_smooth(cap));
	/* frame need drives the performance-governor ceiling */
	if (READ_ONCE(perf_touch_util) == 0)
		perf_frame_apply_max();
	eff = max(READ_ONCE(perf_touch_util), READ_ONCE(fps_floor_util));
	perf_fps_gpu_floor(eff);
}

/* Frame-driven GPU min-frequency floor.
 * GPU renders the frames, so 60Hz jank is mostly GPU-bound. Map the
 * effective need (0-1024) onto a fraction of the touch GPU boost level:
 *   eff>=1024 -> touch_gpu_mhz (full), eff/2 -> half, 0 -> release.
 * Touch release keeps this floor while frames keep coming; it drops to
 * 0 only when frames stop (decay worker) - battery-friendly.
 */
static void perf_fps_gpu_floor(unsigned long eff)
{
	unsigned long min_hz;

	if (eff == 0) {
		/* Animation still playing: keep a floor so the tail of the
		 * animation doesn't lose GPU headroom frame-by-frame. It is
		 * released only when frames stop (decay worker clears
		 * perf_anim_active). Cold-launch window also keeps the floor. */
		if (READ_ONCE(perf_anim_active) || READ_ONCE(perf_launch_boosted))
			perf_gpu_set_min(gpu_boost_min);
		else
			perf_gpu_set_min(0);
		return;
	}
	min_hz = touch_gpu_mhz * 1000000UL * eff / FP_SCALE;
	if (READ_ONCE(perf_anim_active) && min_hz < gpu_boost_min)
		min_hz = gpu_boost_min;
	perf_gpu_set_min(min_hz);
}

/* ---- Idle clamp: no touch & no frame for 300ms -> pin CPU to LOWEST
 * (both MIN and MAX so even stray load cannot ramp up), GPU to table min.
 * Any touch/frame activity wakes it (perf_idle_clamp_off).
 */
/* Catch any cpufreq policy that appeared without our notifier seeing it.
 * Cheap: perfmgr_qos_registered[] short-circuits already-registered ones.
 */
static void perf_qos_ensure_registered(void)
{
	int cpu;

	for_each_online_cpu(cpu) {
		struct cpufreq_policy *p;

		if (perfmgr_qos_registered[cpu])
			continue;
		p = cpufreq_cpu_get(cpu);
		if (p) {
			perf_qos_register_policy(p);
			cpufreq_cpu_put(p);
		}
	}
}

/* cpuset: pin foreground rendering to mid cores during animation */
static void perf_cpuset_set(const char *mask)
{
	struct file *f;
	loff_t pos = 0;

	f = filp_open("/dev/cpuset/top-app/cpus", O_WRONLY, 0);
	if (!IS_ERR(f)) {
		kernel_write(f, mask, strlen(mask), &pos);
		filp_close(f, NULL);
	}
	f = filp_open("/dev/cpuset/foreground/cpus", O_WRONLY, 0);
	if (!IS_ERR(f)) {
		pos = 0;
		kernel_write(f, mask, strlen(mask), &pos);
		filp_close(f, NULL);
	}
}

static void perf_idle_work(struct work_struct *work)
{
	perf_qos_ensure_registered();

	/* kgsl resets max_gpuclk periodically - re-apply every tick.
	 * 680M ONLY on confirmed jank (frames over budget); launch boost
	 * and normal anim/touch stay hard-capped at 550M. */
	if (perf_allow_overlimit())
		perf_gpu_set_max(gpu_touch_cap);   /* jank escape: 680M */
	else if (!READ_ONCE(perf_touch_boosted) && !READ_ONCE(perf_launch_boosted) &&
		 !READ_ONCE(perf_anim_active))
		perf_gpu_set_max(gpu_idle_cap);    /* screen off / fully idle: 124.8M */
	else
		/* frames flowing (anim/touch/launch): GPU cap is learned/adaptive */
		perf_gpu_apply_cap();
	if (!READ_ONCE(perf_touch_boosted) && !READ_ONCE(perf_launch_boosted) &&
	    !READ_ONCE(perf_anim_active))
		perf_apply_cluster(CLUSTER_IDLE);

	schedule_delayed_work(&perf_idle_wq, msecs_to_jiffies(1000));
}

static void perf_idle_work_init(void)
{
	INIT_DELAYED_WORK(&perf_idle_wq, perf_idle_work);
	schedule_delayed_work(&perf_idle_wq, msecs_to_jiffies(200));
}

/* Battery safety net: if the app stops reporting frames (video paused,
 * screen off, app backgrounded) the frame floor must decay instead of
 * pinning the CPU frequency. Runs every 250ms; after 500ms of silence
 * each tick halves the floor (fast fallback -> energy), while active
 * frame reports keep it alive.
 */
static void perf_floor_decay_work(struct work_struct *work)
{
	unsigned long floor;

	floor = READ_ONCE(fps_floor_util);
	if (time_after(jiffies, last_frame_jiffies + HZ / 2)) {
		WRITE_ONCE(perf_anim_active, false);
		if (!READ_ONCE(perf_touch_boosted)) {
			perf_apply_cluster(CLUSTER_IDLE);
			/* Animation over: release the mid-core pin so the
			 * system can use the full cluster set again. */
			perf_cpuset_set("0-7");
		}
	}
	if (time_after(jiffies, last_frame_jiffies + HZ / 2) &&
	    floor > 0) {
		WRITE_ONCE(fps_floor_util, floor / 2);
		perf_mgr_apply_freq(READ_ONCE(perf_touch_util));
		perf_fps_gpu_floor(READ_ONCE(fps_floor_util));
		pr_debug(PFX "frame idle: floor decay -> %lu\n",
			 READ_ONCE(fps_floor_util));
	}
	schedule_delayed_work(&perf_floor_decay_wq, msecs_to_jiffies(250));
}

static long perf_mgr_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
	void __user *uarg = (void __user *)arg;
	long ret = -EINVAL;
	int target_tid;
	struct fps_info info, ofi;
	struct task_struct *task;
	struct perf_task *fi, *target_fi = NULL;
	unsigned long rn_sum = 0;
	unsigned long duration = 0;
	unsigned long prev_util, new_util;

	if (!uarg)
		return -EINVAL;

	switch (cmd) {
	case PERF_MGR_FPS_NUM:
		return g_fps;

	case PERF_MGR_SET_FPS:
		if (copy_from_user(&target_tid, uarg, sizeof(int)))
			return -EFAULT;
		if (target_tid > 0 && target_tid <= 240) {
			perf_set_fps(target_tid);
			pr_debug(PFX "fps set to %d (margin %d%%)\n",
				 g_fps, fps_margin_percent);
		}
		return 0;

	case PERF_MGR_PROCESS_KILL:
		if (copy_from_user(&target_tid, uarg, sizeof(int)))
			return -EFAULT;

		if (target_tid < 0) {
			/* clear all */
			spin_lock(&perf_list_lock);
			while (!list_empty(&perf_task_list)) {
				fi = list_first_entry(&perf_task_list,
						      struct perf_task, list);
				list_del_rcu(&fi->list);
				kfree_rcu(fi, rcu);
			}
			perf_task_count = 0;
			spin_unlock(&perf_list_lock);
		} else {
			fi = perf_get_task(target_tid);
			if (!fi)
				break;
			spin_lock(&perf_list_lock);
			list_del_rcu(&fi->list);
			spin_unlock(&perf_list_lock);
			perf_task_count--;
			synchronize_rcu();
			kfree(fi);
		}
		return 0;

	case PERF_MGR_TASK_ADD:
		if (copy_from_user(&info, uarg, sizeof(info)))
			return -EFAULT;

		task = find_task_by_vpid(info.tid);
		if (!task)
			break;

		if (perf_get_task(info.tid))
			break;  /* already registered */

		fi = kzalloc(sizeof(*fi), GFP_KERNEL);
		if (!fi)
			return -EAGAIN;

		fi->tid = info.tid;
		fi->group_id = info.group_id;
		fi->updated_fps_util = 0;
		fi->running_cpu = 9999;

		spin_lock(&perf_list_lock);
		list_add_tail_rcu(&fi->list, &perf_task_list);
		spin_unlock(&perf_list_lock);
		perf_task_count++;
		pr_debug(PFX "add tid=%d group=%d cnt=%d\n",
			 fi->tid, fi->group_id, perf_task_count);
		return 0;

	case PERF_MGR_FRAME_END:
		if (copy_from_user(&info, uarg, sizeof(info)))
			return -EFAULT;

		if (!perfmgr_enable)
			return 0;

		task = find_task_by_vpid(info.tid);
		if (list_empty(&perf_task_list) || !task)
			break;

		rcu_read_lock();
		list_for_each_entry_rcu(fi, &perf_task_list, list) {
			ofi.tid = fi->tid;
			ofi.group_id = fi->group_id;

			if (ofi.tid == info.tid)
				target_fi = fi;

			if (fi->last_update_frame &&
			    time_after(jiffies, fi->last_update_frame +
					   msecs_to_jiffies(frame_hold_ms))) {
				fi->last_update_frame = 0;
				fi->updated_fps_util = 0;
			}
			if (!fi->last_update_frame)
				fi->last_update_frame = jiffies;

			{
				struct task_struct *tmp;

				tmp = find_task_by_vpid(ofi.tid);
				if (tmp && task_cpu(tmp) == task_cpu(task))
					rn_sum += perf_task_util(tmp);
			}
		}
		rcu_read_unlock();

		if (!target_fi)
			break;

		prev_util = target_fi->updated_fps_util;
		duration = (info.boosting_lvl > BOOST_OFF) ?
			   (us_frame_time * 1000) : info.duration;
		new_util = perf_calc_required_util(rn_sum, duration);

		if (info.boosting_lvl != BOOST_MID)
			target_fi->updated_fps_util =
				max(target_fi->updated_fps_util, new_util);
		else
			target_fi->updated_fps_util = 0;

		if (target_fi->updated_fps_util != prev_util)
			target_fi->last_update_frame = 0;

		WRITE_ONCE(last_frame_jiffies, jiffies);
		perf_fps_apply_floor();

		trace_printk(PFX "fps=%d tid=%d util=%lu floor=%lu\n",
			     g_fps, target_fi->tid,
			     target_fi->updated_fps_util,
			     READ_ONCE(fps_floor_util));
		return 0;

	default:
		break;
	}
	return ret;
}

/* Set target FPS; margin adapts to a fixed ~2ms scheduling headroom:
 *   60Hz  -> 2000*100/16666 = 12%  (was 10%: too tight -> jank on drops)
 *   120Hz -> 2000*100/8333  = 24%  (was 30%: wasted headroom -> power)
 * Leaner but sufficient headroom = maximum energy saving without jank.
 */
static void perf_set_fps(int fps)
{
	g_fps = fps;
	us_frame_time = 1000000 / fps;
	fps_margin_percent = max(10, min(35, 2000 * 100 / (int)us_frame_time));
}

/* Simple frame report via write():
 *   write(fd, "<duration_ns>\n", len)
 * Java FileOutputStream can report frames without ioctl/JNI.
 */
static ssize_t perf_mgr_write(struct file *file, const char __user *buf,
			      size_t count, loff_t *ppos)
{
	char kbuf[32];
	unsigned long long duration_ns;
	unsigned long rn_sum = 0;
	unsigned long new_util;
	struct perf_task *fi, *target_fi = NULL;
	ssize_t ret = count;

	if (count >= sizeof(kbuf))
		count = sizeof(kbuf) - 1;

	if (copy_from_user(kbuf, buf, count))
		return -EFAULT;

	kbuf[count] = '\0';
	if (kstrtoull(kbuf, 10, &duration_ns))
		return -EINVAL;

	if (!perfmgr_enable || duration_ns == 0 || duration_ns > 5000000000ULL)
		return ret;

	rcu_read_lock();
	list_for_each_entry_rcu(fi, &perf_task_list, list) {
		if (fi->last_update_frame &&
		    time_after(jiffies, fi->last_update_frame +
				       msecs_to_jiffies(frame_hold_ms))) {
			fi->last_update_frame = 0;
			fi->updated_fps_util = 0;
		}
		if (!fi->last_update_frame)
			fi->last_update_frame = jiffies;
		target_fi = fi; /* aggregate across all registered tasks */
	}
	if (target_fi) {
		rn_sum = 0;
		list_for_each_entry_rcu(fi, &perf_task_list, list) {
			struct task_struct *t = find_task_by_vpid(fi->tid);
			if (t)
				rn_sum += perf_task_util(t);
		}
		new_util = perf_calc_required_util(rn_sum, duration_ns);

		/* Dead-zone: only block small DOWN-steps to reduce freq churn;
		 * up-steps always apply immediately (jank-safe).
		 */
		if (new_util < target_fi->updated_fps_util &&
		    abs((long)new_util - (long)target_fi->updated_fps_util) < 31)
			goto out_frame;
		target_fi->updated_fps_util = new_util;
		pr_debug(PFX "frame dur=%llu util=%lu\n",
			 duration_ns, new_util);
	}
out_frame:
	rcu_read_unlock();

	WRITE_ONCE(last_frame_jiffies, jiffies);
	WRITE_ONCE(perf_anim_active, true);
	perf_fps_apply_floor();

	/* Frame reports drive a frequency FLOOR (max with touch boost).
	 * The floor is proportional to the computed capacity need, so it
	 * never overrides an active touch boost and decays when frames stop.
	 */
	return ret;
}

static const struct file_operations perf_mgr_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = perf_mgr_ioctl,
	.write = perf_mgr_write,
};

static struct miscdevice perf_mgr_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "perf_manager",
	.mode = 0666,  /* world-writable: systemui/games report frame timings */
	.fops = &perf_mgr_fops,
};

static ssize_t enable_show(struct kobject *kobj, struct kobj_attribute *attr,
			   char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", perfmgr_enable);
}

static ssize_t enable_store(struct kobject *kobj, struct kobj_attribute *attr,
			    const char *buf, size_t n)
{
	int val;

	if (kstrtoint(buf, 10, &val))
		return -EINVAL;
	perfmgr_enable = !!val;
	return n;
}
static struct kobj_attribute enable_attr = __ATTR(enable, 0644, enable_show, enable_store);

static ssize_t fps_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", g_fps);
}

static ssize_t fps_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t n)
{
	int val;

	if (kstrtoint(buf, 10, &val) || val <= 0 || val > 240)
		return -EINVAL;
	perf_set_fps(val);
	pr_info(PFX "fps set to %d (margin %d%%)\n", g_fps, fps_margin_percent);
	return n;
}
static struct kobj_attribute fps_attr = __ATTR(fps, 0644, fps_show, fps_store);

static ssize_t margin_show(struct kobject *kobj, struct kobj_attribute *attr,
			   char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", fps_margin_percent);
}

static ssize_t margin_store(struct kobject *kobj, struct kobj_attribute *attr,
			    const char *buf, size_t n)
{
	int val;

	if (kstrtoint(buf, 10, &val))
		return -EINVAL;
	fps_margin_percent = val;
	return n;
}
static struct kobj_attribute margin_attr = __ATTR(margin, 0644, margin_show, margin_store);

/* Frame report: echo <duration_ns> > frame (0666, apps can write) */
/* Module writes its real cumulative frame count here (via sysfs) */
static ssize_t frame_total_store(struct kobject *kobj,
				 struct kobj_attribute *attr,
				 const char *buf, size_t n)
{
	unsigned long val;

	if (kstrtoul(buf, 10, &val))
		return -EINVAL;
	atomic_set(&frame_total, val);
	return n;
}

static ssize_t frame_total_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n",
			 atomic_read(&frame_total));
}

static struct kobj_attribute frame_total_attr =
	__ATTR(frame_total, 0644, frame_total_show, frame_total_store);

static ssize_t frame_store(struct kobject *kobj, struct kobj_attribute *attr,
			   const char *buf, size_t n)
{
	unsigned long long duration_ns;
	unsigned long rn_sum = 0;
	unsigned long new_util;
	struct perf_task *fi, *target_fi = NULL;

	if (!perfmgr_enable)
		return n;

	if (kstrtoull(buf, 10, &duration_ns))
		return -EINVAL;
	if (duration_ns == 0 || duration_ns > 5000000000ULL)
		return n;

	/* Adaptive learning: dropped frames raise the GPU baseline one level
	 * now (and keep raising if they keep dropping); sustained ok lowers
	 * it slowly. Runs before jank detection - learns from the same frames. */
	if (perf_gpu_adjust(duration_ns, us_frame_time) &&
	    !perf_allow_overlimit() &&
	    (READ_ONCE(perf_touch_boosted) || READ_ONCE(perf_launch_boosted) ||
	     READ_ONCE(perf_anim_active))) {
		/* cap changed (raised or drifted): apply it immediately */
		perf_gpu_apply_cap();
	}

	/* jank detection: 3 CONSECUTIVE frames >2.5x budget (real jank, not
	 * single-frame dips). Cleared on the first in-budget frame.
	 * NOTE: us_frame_time is in MICROseconds, duration_ns in NANOseconds:
	 * compare duration_ns > us_frame_time * 1000 * 2500. */
	if (us_frame_time && duration_ns > (unsigned long long)us_frame_time * 1000UL * 2500) {
		if (++perf_jank_frames >= 3) {
			WRITE_ONCE(perf_jank, true);
			WRITE_ONCE(perf_ok_frames, 0);
			/* Jank confirmed: lift caps RIGHT NOW, don't wait for
			 * the 1s idle worker. CPU to TOUCH caps, GPU to 680M -
			 * unconditional, also while touching (a touch mid
			 * worker would otherwise delay the escape ~500ms). */
			perf_gpu_set_max(gpu_touch_cap);
			perf_apply_cluster(CLUSTER_TOUCH);
		}
	} else {
		WRITE_ONCE(perf_jank_frames, 0);
		if (perf_jank) {
			WRITE_ONCE(perf_jank, false);
			WRITE_ONCE(perf_ok_frames, 0);
			/* Recovered: back to lean caps immediately.
			 * Touching -> ANIM (touch non-jank state); otherwise
			 * frame_apply_max() picks ANIM (frames flowing, no
			 * jank) or IDLE (no frames). */
			perf_gpu_apply_cap();
			if (READ_ONCE(perf_touch_util) > 0)
				perf_apply_cluster(CLUSTER_ANIM);
			else
				perf_frame_apply_max();
		}
	}

	atomic_inc(&frame_reports);
	/* A valid frame write is a success regardless of task registration.
	 * (The module reports via sysfs; ioctl TASK_ADD is optional.)
	 */
	atomic_inc(&frame_ok);

	rcu_read_lock();
	list_for_each_entry_rcu(fi, &perf_task_list, list) {
		if (fi->last_update_frame &&
		    time_after(jiffies, fi->last_update_frame +
				       msecs_to_jiffies(frame_hold_ms))) {
			fi->last_update_frame = 0;
			fi->updated_fps_util = 0;
		}
		if (!fi->last_update_frame)
			fi->last_update_frame = jiffies;
		target_fi = fi;
	}
	if (target_fi) {
		rn_sum = 0;
		list_for_each_entry_rcu(fi, &perf_task_list, list) {
			struct task_struct *t = find_task_by_vpid(fi->tid);
			if (t)
				rn_sum += perf_task_util(t);
		}
		new_util = perf_calc_required_util(rn_sum, duration_ns);
		target_fi->updated_fps_util = new_util;
		pr_debug(PFX "frame dur=%llu util=%lu\n",
			 duration_ns, new_util);
	}
	rcu_read_unlock();

	WRITE_ONCE(last_frame_jiffies, jiffies);
	WRITE_ONCE(perf_anim_active, true);
	perf_fps_apply_floor();

	return n;
}

/* Register per-policy qos (min floor + max cap) for one policy.
 * Called from init AND from the cpufreq policy notifier so it works even
 * when the qcom cpufreq driver loads after this built-in driver.
 */
static void perf_qos_register_policy(struct cpufreq_policy *p)
{
	unsigned long cap = 0;
	int idx;
	int qos_ret;

	if (!p || p->cpu >= NR_CPUS)
		return;
	if (perfmgr_qos_registered[p->cpu])
		return;

	idx = perfmgr_qos_count;
	if (idx >= NR_CPUS)
		return;

	perfmgr_qos_max[idx] = p->cpuinfo.max_freq;
	perfmgr_qos_min[idx] = p->cpuinfo.min_freq;
	perfmgr_qos_cpu[idx] = p->cpu;

	/* classify cluster by max freq and apply energy cap */
	if (p->cpuinfo.max_freq <= 2100000) {
		cap = little_freq_cap;     /* A510 */
		perfmgr_qos_cluster[idx] = 0;
	} else if (p->cpuinfo.max_freq <= 2900000) {
		cap = mid_freq_cap;        /* A715/A720 */
		perfmgr_qos_cluster[idx] = 1;
	} else {
		cap = big_freq_cap;        /* X3 */
		perfmgr_qos_cluster[idx] = 2;
	}

	/* MIN request stays at default (no floor) - frequency is driven by
	 * policy->max only. Keep the request registered so array indexes align. */
	qos_ret = freq_qos_add_request(&p->constraints,
					&perfmgr_qos[idx],
					FREQ_QOS_MIN,
					FREQ_QOS_MIN_DEFAULT_VALUE);
	if (qos_ret < 0) {
		pr_warn(PFX "freq_qos min add failed cpu%d: %d\n",
			p->cpu, qos_ret);
		return;
	}
	if (cap) {
		qos_ret = freq_qos_add_request(&p->constraints,
						&perfmgr_qos_cap[idx],
						FREQ_QOS_MAX,
						cap);
		if (qos_ret < 0)
			pr_warn(PFX "freq_qos max add failed cpu%d: %d\n",
				p->cpu, qos_ret);
		else {
			perfmgr_qos_cap_orig[idx] = cap;
			pr_info(PFX "cpu%d cap=%luHz\n", p->cpu, cap);
		}
	}
	perfmgr_qos_registered[p->cpu] = true;
	perfmgr_qos_count = idx + 1;
}

/* Called by the (patched) performance governor's limits on every policy
 * update. Self-contained state machine: no registration needed.
 *   idle           -> lowest freq
 *   touch          -> moderate boost target
 *   animation      -> frame-need fraction of cluster cap
 *   default        -> cluster energy cap
 */
unsigned long perf_mgr_get_cluster_cap(struct cpufreq_policy *p)
{
	unsigned long max_freq = p->cpuinfo.max_freq;

	if (max_freq <= 2100000)
		return 1228800;   /* A510 */
	if (max_freq <= 2900000)
		return 1536000;   /* A715/A720 */
	return 0;                /* X3: FEAS feasd controls via sysfs */
}
EXPORT_SYMBOL_GPL(perf_mgr_get_cluster_cap);

static int perf_cpufreq_notifier(struct notifier_block *nb,
				 unsigned long event, void *data)
{
	struct cpufreq_policy *p = data;

	if (event == CPUFREQ_CREATE_POLICY)
		perf_qos_register_policy(p);
	return 0;
}

/* Sultan-style screen-on MAX_BOOST: on wake, drive a brief higher boost so
 * the unlock/launch animation is smooth, then it expires (touch_hold_ms)
 * and the state machine returns to frame-need/idle frequencies.
 */
static int perf_pm_notifier(struct notifier_block *nb,
			    unsigned long mode, void *_unused)
{
	if (mode == PM_POST_SUSPEND || mode == PM_POST_HIBERNATION) {
		WRITE_ONCE(last_touch_jiffies, jiffies);
		WRITE_ONCE(perf_touch_boosted, true);
		WRITE_ONCE(perf_touch_in_mid, false);
		WRITE_ONCE(last_touch_jiffies, jiffies);
		perf_gpu_set_min(gpu_boost_min);
		perf_apply_cluster(CLUSTER_ANIM);   /* screen-on animation */
		perf_cpuset_set("3-6");
		perf_gpu_apply_cap();
		/* auto-release after hold: back to caps */
		schedule_delayed_work(&perf_touch_work,
				      msecs_to_jiffies(touch_hold_ms));
		pr_info(PFX "wake: screen-on animation boost\n");
	}
	return NOTIFY_OK;
}

/* Lazily (re)locate GPU devfreq: msm_kgsl is a module that loads AFTER
 * this built-in driver's initcall, so we must retry on each touch.
 */
static void perf_gpu_lazy_find(void)
{
	struct device_node *gpu_np;

	if (perf_gpu_devfreq)
		return;

	gpu_np = of_find_node_by_path("/soc/qcom,kgsl-3d0@3d00000");
	if (gpu_np) {
		perf_gpu_devfreq = devfreq_get_devfreq_by_node(gpu_np);
		of_node_put(gpu_np);
		if (IS_ERR(perf_gpu_devfreq))
			perf_gpu_devfreq = NULL;
	}
	if (perf_gpu_devfreq) {
		perf_gpu_found = 1;
		/* kgsl loads AFTER this built-in driver: enforce the cap now */
		perf_gpu_apply_cap();
		pr_info(PFX "GPU devfreq located (lazy)\n",
			dev_name(&perf_gpu_devfreq->dev));
	}
}

/* Set GPU devfreq min frequency (Hz); 0 = restore system control */
static void perf_gpu_set_min(unsigned long min_hz)
{
	if (!perf_gpu_devfreq)
		perf_gpu_lazy_find();
	if (!perf_gpu_devfreq)
		return;

	mutex_lock(&perf_gpu_devfreq->lock);
	/* Do NOT unconditionally loosen scaling_max_freq to gpu_touch_cap -
	 * that fights the adaptive cap (perf_gpu_apply_cap) and pins the GPU
	 * at 680M while any frame flows. Only nudge the max up to min_hz when
	 * needed to avoid an illegal min>max state; otherwise leave max alone. */
	if (min_hz > perf_gpu_devfreq->scaling_max_freq)
		perf_gpu_devfreq->scaling_max_freq = min_hz;
	if (perf_gpu_devfreq->scaling_min_freq != min_hz) {
		perf_gpu_devfreq->scaling_min_freq = min_hz;
		update_devfreq(perf_gpu_devfreq);
	}
	mutex_unlock(&perf_gpu_devfreq->lock);
}

/* Set GPU devfreq max frequency (Hz) - caps the freq table.
 * Uses BOTH scaling_max_freq and a hard DEV_PM_QOS_MAX_FREQUENCY request
 * (devfreq reads the PM QoS ceiling in get_effective_freq, so kgsl cannot
 * bypass it like it can with scaling_max_freq alone).
 */
static void perf_gpu_set_max(unsigned long max_hz)
{
	char buf[32];
	struct file *f;
	loff_t pos = 0;
	int n;

	/* kgsl has its OWN hard clamp (max_gpuclk, defaults 680MHz) that
	 * devfreq max_freq does NOT control - write it directly. */
	n = snprintf(buf, sizeof(buf), "%lu\n", max_hz);
	f = filp_open("/sys/class/kgsl/kgsl-3d0/max_gpuclk", O_WRONLY, 0);
	if (!IS_ERR(f)) {
		kernel_write(f, buf, n, &pos);
		filp_close(f, NULL);
	}

	if (!perf_gpu_devfreq)
		perf_gpu_lazy_find();
	if (!perf_gpu_devfreq)
		return;

	if (!gpu_max_qos_active && !gpu_max_qos_failed) {
		if (!dev_pm_qos_add_request(perf_gpu_devfreq->dev.parent,
					    &gpu_max_qos,
					    DEV_PM_QOS_MAX_FREQUENCY,
					    max_hz))
			gpu_max_qos_active = true;
		else
			gpu_max_qos_failed = true;  /* stop retrying (WARN spam) */
	} else {
		dev_pm_qos_update_request(&gpu_max_qos, max_hz);
	}

	mutex_lock(&perf_gpu_devfreq->lock);
	if (perf_gpu_devfreq->scaling_max_freq != max_hz) {
		perf_gpu_devfreq->scaling_max_freq = max_hz;
		update_devfreq(perf_gpu_devfreq);
	}
	mutex_unlock(&perf_gpu_devfreq->lock);
}

/* Continuous touch for touch_mid_delay_ms: drop from max to mid (iOS f3) */
static void perf_touch_mid(struct work_struct *work)
{
	/* Only valid for the press generation this work was scheduled for.
	 * A newer press (gen mismatch) means a newer boost is in flight,
	 * so an old mid work must NOT downgrade it.
	 */
	(void)work;
	if (atomic_read(&perf_touch_gen) != READ_ONCE(perf_touch_mid_gen))
		return;
	if (!READ_ONCE(perf_touch_boosted) || READ_ONCE(perf_touch_in_mid))
		return;

	WRITE_ONCE(perf_touch_in_mid, true);
	WRITE_ONCE(perf_touch_util, 600);  /* mid floor */
	perf_touch_apply_boost(perf_allow_overlimit() ? 1 : 0);
	if (perf_allow_overlimit())
		perf_gpu_set_max(gpu_touch_cap);
	else
		perf_gpu_apply_cap();
	pr_debug(PFX "touch state (overlimit=%d)\n",
		 perf_allow_overlimit() ? 1 : 0);
}

/* Called 3s after finger lift: restore hard caps + full cpuset */
static void perf_touch_release(struct work_struct *work)
{
	if (!READ_ONCE(perf_touch_boosted))
		return;

	WRITE_ONCE(perf_touch_boosted, false);
	WRITE_ONCE(perf_touch_in_mid, false);
	WRITE_ONCE(perf_touch_util, 0);
	if (READ_ONCE(perf_anim_active)) {
		/* Fingers lifted but the animation (inertial scroll etc.)
		 * is still playing: keep ANIM caps instead of dropping
		 * straight to IDLE and stuttering the anim tail. */
		perf_frame_apply_max();
		perf_cpuset_set("3-6");
		if (perf_allow_overlimit())
			perf_gpu_set_max(gpu_touch_cap);
		else
			perf_gpu_apply_cap();
	} else {
		perf_apply_cluster(CLUSTER_IDLE); /* idle caps */
		perf_cpuset_set("0-7");          /* full cpuset */
		perf_gpu_set_min(0);             /* GPU idle */
		perf_gpu_apply_cap();
	}
	pr_info(PFX "touch released: anim=%d\n", READ_ONCE(perf_anim_active));
}

/* Touch/animation boost: mid cores render, little/big standby */
/* Cold-launch boost: called by sysfs write (process context).
 * TOUCH caps now, GPU 680M + 401M floor, auto-release after
 * LAUNCH_BOOST_MS. Touch/frame flow takes over on release. */
static void perf_launch_boost_apply(void)
{
	WRITE_ONCE(perf_launch_boosted, true);
	perf_apply_cluster(CLUSTER_TOUCH);
	/* GPU cap adaptive; 680M only on confirmed jank */
	if (perf_allow_overlimit())
		perf_gpu_set_max(gpu_touch_cap);
	else
		perf_gpu_apply_cap();
	perf_gpu_set_min(gpu_boost_min);
	schedule_delayed_work(&perf_launch_work,
			      msecs_to_jiffies(LAUNCH_BOOST_MS));
	pr_info(PFX "launch boost: TOUCH caps %dms\n", LAUNCH_BOOST_MS);
}

static void perf_launch_release(struct work_struct *work)
{
	if (!READ_ONCE(perf_launch_boosted))
		return;
	WRITE_ONCE(perf_launch_boosted, false);
	/* Back to frame-flow caps (ANIM/IDLE). Touch, if any, is untouched:
	 * touch state machine keeps its own boost until its release. */
	if (READ_ONCE(perf_anim_active))
		perf_frame_apply_max();
	else
		perf_apply_cluster(CLUSTER_IDLE);
	if (perf_allow_overlimit())
		perf_gpu_set_max(gpu_touch_cap);
	else
		perf_gpu_apply_cap();
	perf_fps_gpu_floor(READ_ONCE(fps_floor_util));
	pr_info(PFX "launch boost released\n");
}

static void perf_touch_do_boost(struct work_struct *work)
{
	WRITE_ONCE(perf_touch_in_mid, false);
	WRITE_ONCE(perf_touch_util, 1024);
	perf_apply_cluster(CLUSTER_ANIM); /* anim peak caps */
	perf_cpuset_set("3-6");          /* render on mid cores only */
	perf_gpu_set_min(gpu_boost_min); /* GPU 401M floor */
	if (perf_allow_overlimit())
		perf_gpu_set_max(gpu_touch_cap);
	else
		perf_gpu_apply_cap();
	if (perf_allow_overlimit())
		pr_info(PFX "anim boost + jank: gpu cap 680M\n");
	else
		pr_info(PFX "anim boost: adaptive gpu cap\n");
}

/* Input event (ATOMIC context: input core holds spinlock here!
 * MUST NOT sleep: no mutex, no update_devfreq, no _sync cancel.
 * Only set flags and defer heavy work to process context.
 */
static void perf_touch_event(struct input_handle *handle, unsigned int type,
			     unsigned int code, int value)
{
	/* Press/release: BTN_TOUCH (single) or ABS_MT_TRACKING_ID (multi) */
	if (type == EV_KEY && code == BTN_TOUCH) {
		/* BTN_TOUCH: 1=down 0=up */
		atomic_inc(&touch_events);
		if (value) {
			atomic_inc(&touch_press_count);
			WRITE_ONCE(perf_touch_boosted, true);
			WRITE_ONCE(perf_touch_in_mid, false);
			WRITE_ONCE(last_touch_jiffies, jiffies);
			cancel_delayed_work(&perf_touch_work);
			queue_work(system_highpri_wq, &perf_touch_boost_work);
			/* iOS-style: after continuous touch, drop to mid */
			WRITE_ONCE(perf_touch_mid_gen,
				   atomic_inc_return(&perf_touch_gen));
			schedule_delayed_work(&perf_touch_mid_work,
					      msecs_to_jiffies(touch_mid_delay_ms));
		} else {
			cancel_delayed_work(&perf_touch_mid_work);
			schedule_delayed_work(&perf_touch_work,
					      msecs_to_jiffies(touch_hold_ms));
		}
	} else if (type == EV_ABS && code == ABS_MT_TRACKING_ID) {
		/* TRACKING_ID: >=0 finger down, -1 all fingers up */
		atomic_inc(&touch_events);
		if (value >= 0) {
			atomic_inc(&touch_press_count);
			WRITE_ONCE(perf_touch_boosted, true);
			WRITE_ONCE(perf_touch_in_mid, false);
			WRITE_ONCE(last_touch_jiffies, jiffies);
			cancel_delayed_work(&perf_touch_work);
			queue_work(system_highpri_wq, &perf_touch_boost_work);
			WRITE_ONCE(perf_touch_mid_gen,
				   atomic_inc_return(&perf_touch_gen));
			schedule_delayed_work(&perf_touch_mid_work,
					      msecs_to_jiffies(touch_mid_delay_ms));
		} else {
			WRITE_ONCE(last_touch_jiffies, jiffies);
			cancel_delayed_work(&perf_touch_mid_work);
			schedule_delayed_work(&perf_touch_work,
					      msecs_to_jiffies(touch_hold_ms));
		}
	} else if (type == EV_ABS && code == ABS_MT_POSITION_X) {
		/* Finger moving while pressed: keep boost alive.
		 * Reset BOTH release and mid timers on every move event so a
		 * continuous scroll never drops back to the idle floor
		 * (WALT would otherwise clock CPUs down between touch events
		 * in 60Hz mode, causing micro-jank).
		 */
		if (READ_ONCE(perf_touch_boosted)) {
			WRITE_ONCE(last_touch_jiffies, jiffies);
			cancel_delayed_work(&perf_touch_work);
			cancel_delayed_work(&perf_touch_mid_work);
			WRITE_ONCE(perf_touch_mid_gen,
				   atomic_inc_return(&perf_touch_gen));
			schedule_delayed_work(&perf_touch_mid_work,
					      msecs_to_jiffies(touch_mid_delay_ms));
		}
	}
}

static int perf_touch_connect(struct input_handler *handler,
			      struct input_dev *dev,
			      const struct input_device_id *id)
{
	struct input_handle *handle;
	int err;

	atomic_inc(&input_connect_calls);

	/* Only multi-touch touchscreens (EV_ABS + ABS_MT_POSITION_X/Y),
	 * same matching as cpufreq_interactive input boost.
	 */
	if (!test_bit(EV_ABS, dev->evbit) ||
	    !test_bit(ABS_MT_POSITION_X, dev->absbit) ||
	    !test_bit(ABS_MT_POSITION_Y, dev->absbit))
		return -ENODEV;

	handle = kzalloc(sizeof(*handle), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "perfmgr-touch";

	err = input_register_handle(handle);
	if (err) {
		kfree(handle);
		return err;
	}

	/* CRITICAL: input_pass_values only dispatches to OPEN handles.
	 * Without input_open_device() -> handle->open stays 0 and we
	 * NEVER receive touch events.
	 */
	err = input_open_device(handle);
	if (err) {
		input_unregister_handle(handle);
		kfree(handle);
		return err;
	}

	atomic_inc(&input_connect_ok);
	pr_info(PFX "touch handler connected: %s\n", dev->name);
	return 0;
}

static void perf_touch_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);
}

static const struct input_device_id perf_input_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT |
			 INPUT_DEVICE_ID_MATCH_ABSBIT,
		.evbit = { BIT_MASK(EV_ABS) },
		.absbit = { [BIT_WORD(ABS_MT_POSITION_X)] =
				BIT_MASK(ABS_MT_POSITION_X) |
				BIT_MASK(ABS_MT_POSITION_Y) },
	}, /* multi-touch touchscreen */
	{ },
};
MODULE_DEVICE_TABLE(input, perf_input_ids);

static struct input_handler perf_input_handler = {
	.event = perf_touch_event,
	.connect = perf_touch_connect,
	.disconnect = perf_touch_disconnect,
	.name = "perfmgr",
	.id_table = perf_input_ids,
};

static ssize_t diag_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
	return scnprintf(buf, PAGE_SIZE,
			"touch_events=%d\n"
			"touch_presses=%d\n"
			"frame_reports=%d\n"
			"frame_ok=%d\n"
			"frame_total=%d\n"
			"input_connect_calls=%d\n"
			"input_connect_ok=%d\n"
			"handler_registered=%d\n"
			"gpu_devfreq_found=%d\n"
			"touch_boosted=%d\n"
			"touch_in_mid=%d\n"
			"touch_mid_delay_ms=%d\n"
			"touch_gpu_mhz=%lu\n"
			"touch_hold_ms=%d\n",
			atomic_read(&touch_events),
			atomic_read(&touch_press_count),
			atomic_read(&frame_reports),
			atomic_read(&frame_ok),
			atomic_read(&frame_total),
			atomic_read(&input_connect_calls),
			atomic_read(&input_connect_ok),
			perf_handler_registered,
			perf_gpu_found,
			READ_ONCE(perf_touch_boosted),
			READ_ONCE(perf_touch_in_mid),
			touch_mid_delay_ms,
			touch_gpu_mhz, touch_hold_ms);
}
static struct kobj_attribute diag_attr = __ATTR(diag, 0444, diag_show, NULL);

static ssize_t jank_show(struct kobject *kobj, struct kobj_attribute *attr,
			 char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n",
			 READ_ONCE(perf_jank) ? 1 : 0);
}
static struct kobj_attribute jank_attr = __ATTR(jank, 0444, jank_show, NULL);

static ssize_t touch_gpu_mhz_show(struct kobject *kobj,
				   struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%lu\n", touch_gpu_mhz);
}

static ssize_t touch_gpu_mhz_store(struct kobject *kobj,
				   struct kobj_attribute *attr,
				   const char *buf, size_t n)
{
	unsigned long val;

	if (kstrtoul(buf, 10, &val))
		return -EINVAL;
	touch_gpu_mhz = val;
	return n;
}
static struct kobj_attribute touch_gpu_mhz_attr =
	__ATTR(touch_gpu_mhz, 0644, touch_gpu_mhz_show, touch_gpu_mhz_store);

static ssize_t touch_mid_delay_ms_show(struct kobject *kobj,
				     struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", touch_mid_delay_ms);
}

static ssize_t touch_mid_delay_ms_store(struct kobject *kobj,
					struct kobj_attribute *attr,
					const char *buf, size_t n)
{
	int val;

	if (kstrtoint(buf, 10, &val) || val < 0 || val > 30000)
		return -EINVAL;
	touch_mid_delay_ms = val;
	return n;
}
static struct kobj_attribute touch_mid_delay_ms_attr =
	__ATTR(touch_mid_delay_ms, 0644,
	       touch_mid_delay_ms_show, touch_mid_delay_ms_store);

static ssize_t touch_hold_ms_show(struct kobject *kobj,
				  struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", touch_hold_ms);
}

static ssize_t touch_hold_ms_store(struct kobject *kobj,
				   struct kobj_attribute *attr,
				   const char *buf, size_t n)
{
	int val;

	if (kstrtoint(buf, 10, &val) || val < 0 || val > 30000)
		return -EINVAL;
	touch_hold_ms = val;
	return n;
}
static struct kobj_attribute touch_hold_ms_attr =
	__ATTR(touch_hold_ms, 0644, touch_hold_ms_show, touch_hold_ms_store);

static struct kobj_attribute frame_attr =
	__ATTR(frame, 0644, NULL, frame_store);

static ssize_t launch_boost_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", READ_ONCE(perf_launch_boosted) ? 1 : 0);
}

static ssize_t launch_boost_store(struct kobject *kobj,
				  struct kobj_attribute *attr,
				  const char *buf, size_t n)
{
	int on;

	if (kstrtoint(buf, 10, &on))
		return -EINVAL;
	if (on) {
		cancel_delayed_work(&perf_launch_work);
		perf_launch_boost_apply();
	} else {
		cancel_delayed_work(&perf_launch_work);
		if (READ_ONCE(perf_launch_boosted)) {
			WRITE_ONCE(perf_launch_boosted, false);
			perf_frame_apply_max();
			if (perf_allow_overlimit())
				perf_gpu_set_max(gpu_touch_cap);
			else
				perf_gpu_apply_cap();
			perf_fps_gpu_floor(READ_ONCE(fps_floor_util));
		}
	}
	return n;
}

static struct kobj_attribute launch_boost_attr =
	__ATTR(launch_boost, 0644, launch_boost_show, launch_boost_store);

static struct attribute *perf_attrs[] = {
	&enable_attr.attr,
	&fps_attr.attr,
	&margin_attr.attr,
	&diag_attr.attr,
	&frame_total_attr.attr,
	&touch_gpu_mhz_attr.attr,
	&touch_mid_delay_ms_attr.attr,
	&touch_hold_ms_attr.attr,
	&frame_attr.attr,
	&jank_attr.attr,
	&launch_boost_attr.attr,
	NULL,
};

static const struct attribute_group perf_attr_group = {
	.attrs = perf_attrs,
};

static struct kobject *perf_kobj;

static int __init perf_mgr_init(void)
{
	int err;

	err = misc_register(&perf_mgr_device);
	if (err)
		return err;

	spin_lock_init(&perf_list_lock);

	perf_kobj = kobject_create_and_add("perf_manager", kernel_kobj);
	if (!perf_kobj) {
		misc_deregister(&perf_mgr_device);
		return -ENOMEM;
	}

	err = sysfs_create_group(perf_kobj, &perf_attr_group);
	if (!err) {
		int cpu;

		perfmgr_qos_count = 0;
		/* register notifier FIRST so policies created later get caps */
		perf_cpufreq_nb.notifier_call = perf_cpufreq_notifier;
		cpufreq_register_notifier(&perf_cpufreq_nb,
					  CPUFREQ_POLICY_NOTIFIER);
		perf_pm_nb.notifier_call = perf_pm_notifier;
		register_pm_notifier(&perf_pm_nb);
		/* also grab any already-online policies now */
		for_each_online_cpu(cpu) {
			struct cpufreq_policy *p = cpufreq_cpu_get(cpu);

			if (p) {
				perf_qos_register_policy(p);
				cpufreq_cpu_put(p);
			}
		}
		pr_info(PFX "freq_qos registered: %d policies\n",
			perfmgr_qos_count);
	}
	if (err) {
		kobject_put(perf_kobj);
		misc_deregister(&perf_mgr_device);
		return err;
	}

	/* Allow apps to read/write fps and write frame nodes */
	{
		struct kernfs_node *kn = sysfs_get_dirent(perf_kobj->sd, "frame");
		if (kn) {
			struct iattr ia = { .ia_valid = ATTR_MODE, .ia_mode = 0666 };
			kernfs_setattr(kn, &ia);
			kernfs_put(kn);
		}
		kn = sysfs_get_dirent(perf_kobj->sd, "fps");
		if (kn) {
			struct iattr ia = { .ia_valid = ATTR_MODE, .ia_mode = 0666 };
			kernfs_setattr(kn, &ia);
			kernfs_put(kn);
		}
		kn = sysfs_get_dirent(perf_kobj->sd, "launch_boost");
		if (kn) {
			struct iattr ia = { .ia_valid = ATTR_MODE, .ia_mode = 0666 };
			kernfs_setattr(kn, &ia);
			kernfs_put(kn);
		}
	}

	/* Locate GPU devfreq (qcom kgsl) */
	{
		struct device_node *gpu_np = of_find_node_by_path(
			"/soc/qcom,kgsl-3d0@3d00000");
		if (gpu_np) {
			perf_gpu_devfreq = devfreq_get_devfreq_by_node(gpu_np);
			of_node_put(gpu_np);
			if (IS_ERR(perf_gpu_devfreq))
				perf_gpu_devfreq = NULL;
		}
		if (perf_gpu_devfreq) {
			perf_gpu_found = 1;
			perf_gpu_apply_cap();
			pr_info(PFX "GPU devfreq located\n",
				dev_name(&perf_gpu_devfreq->dev));
		} else {
			perf_gpu_found = 0;
			pr_info(PFX "GPU devfreq NOT found (touch boost GPU off)\n");
		}
	}

	INIT_WORK(&perf_touch_boost_work, perf_touch_do_boost);
	INIT_DELAYED_WORK(&perf_touch_mid_work, perf_touch_mid);
	INIT_DELAYED_WORK(&perf_touch_work, perf_touch_release);
	INIT_DELAYED_WORK(&perf_floor_decay_wq, perf_floor_decay_work);
	INIT_DELAYED_WORK(&perf_launch_work, perf_launch_release);
	schedule_delayed_work(&perf_floor_decay_wq, msecs_to_jiffies(400));
	perf_idle_work_init();

	err = input_register_handler(&perf_input_handler);
	if (err) {
		perf_handler_registered = 0;
		pr_warn(PFX "input handler register failed: %d\n", err);
	} else {
		perf_handler_registered = 1;
		pr_info(PFX "touch boost armed\n");
	}

	perf_set_fps(g_fps);
	pr_info(PFX "initialized (fps=%d margin=%d%%)\n",
		g_fps, fps_margin_percent);
	return 0;
}

static void __exit perf_mgr_exit(void)
{
	cpufreq_unregister_notifier(&perf_cpufreq_nb,
				    CPUFREQ_POLICY_NOTIFIER);
	unregister_pm_notifier(&perf_pm_nb);
	cancel_work_sync(&perf_touch_boost_work);
	cancel_delayed_work_sync(&perf_touch_mid_work);
	cancel_delayed_work_sync(&perf_touch_work);
	cancel_delayed_work_sync(&perf_floor_decay_wq);
	cancel_delayed_work_sync(&perf_idle_wq);
	cancel_delayed_work_sync(&perf_launch_work);
	if (gpu_max_qos_active)
		dev_pm_qos_remove_request(&gpu_max_qos);
	input_unregister_handler(&perf_input_handler);
	sysfs_remove_group(perf_kobj, &perf_attr_group);
	kobject_put(perf_kobj);
	misc_deregister(&perf_mgr_device);
}

module_init(perf_mgr_init);
module_exit(perf_mgr_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("FPS-driven energy-aware frequency governor (FEAS-like)");
MODULE_VERSION("0.1.0");
