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
#include <linux/pm_qos.h>
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
static int hold_frame_count = 2;
static bool perfmgr_enable = true;
static struct freq_qos_request perfmgr_qos[NR_CPUS];
static unsigned long perfmgr_qos_max[NR_CPUS];  /* per-policy max freq */
static int perfmgr_qos_count;

/* GPU touch boost */
static struct devfreq *perf_gpu_devfreq;
static unsigned long touch_gpu_mhz = 550;  /* touch boost GPU min freq (MHz) */
static int touch_hold_ms = 2000;           /* hold after finger lift (ms) */
static int touch_mid_delay_ms = 500;       /* drop to mid level after this long of continuous touch (iOS-style) */
static bool perf_touch_boosted;
static bool perf_touch_in_mid;
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
			g_fps = target_tid;
			us_frame_time = 1000000 / g_fps;
			if (g_fps <= 60)
				fps_margin_percent = 10;
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

			if (++fi->last_update_frame > hold_frame_count) {
				fi->last_update_frame = 0;
				fi->updated_fps_util = 0;
			}

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

		trace_printk(PFX "fps=%d tid=%d util=%lu\n",
			     g_fps, target_fi->tid,
			     target_fi->updated_fps_util);
		return 0;

	default:
		break;
	}
	return ret;
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
		if (fi->last_update_frame >= 0) {
			if (++fi->last_update_frame > hold_frame_count) {
				fi->last_update_frame = 0;
				fi->updated_fps_util = 0;
			}
		}
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

		/* Dead-zone: skip update if util change < 3% (reduce freq churn) */
		if (abs((long)new_util - (long)target_fi->updated_fps_util) < 31)
			goto out_frame;
		target_fi->updated_fps_util = new_util;
		pr_debug(PFX "frame dur=%llu util=%lu\n",
			 duration_ns, new_util);
	}
out_frame:
	rcu_read_unlock();

	/* NOTE: frame reporting does NOT drive frequency directly.
	 * Frequency is managed exclusively by touch boost (iOS-style).
	 * This avoids frame reports overriding the touch boost state.
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
	g_fps = val;
	us_frame_time = 1000000 / g_fps;
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
	__ATTR(frame_total, 0666, frame_total_show, frame_total_store);

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

	atomic_inc(&frame_reports);
	/* A valid frame write is a success regardless of task registration.
	 * (The module reports via sysfs; ioctl TASK_ADD is optional.)
	 */
	atomic_inc(&frame_ok);

	rcu_read_lock();
	list_for_each_entry_rcu(fi, &perf_task_list, list) {
		if (++fi->last_update_frame > hold_frame_count) {
			fi->last_update_frame = 0;
			fi->updated_fps_util = 0;
		}
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

	return n;
}

/*
 * Touch boost applies a FREQUENCY FLOOR (FREQ_QOS_MIN) — same idea as
 * AOSP touch boost and iOS interaction hint. Per-policy floor so big
 * cores are not capped by a little-core max.
 */
static void perf_mgr_apply_freq(unsigned long util)
{
	int i;

	if (perfmgr_qos_count == 0)
		return;

	if (util >= 1000) {
		/* finger down: full boost (iOS f2 - max) */
		for (i = 0; i < perfmgr_qos_count; i++)
			freq_qos_update_request(&perfmgr_qos[i],
						perfmgr_qos_max[i]);
	} else if (util >= 550) {
		/* continuous touch: mid level (iOS f3 - ~60% of max) */
		for (i = 0; i < perfmgr_qos_count; i++)
			freq_qos_update_request(&perfmgr_qos[i],
						perfmgr_qos_max[i] * 60 / 100);
	} else {
		/* release: remove floor */
		for (i = 0; i < perfmgr_qos_count; i++)
			freq_qos_update_request(&perfmgr_qos[i],
						FREQ_QOS_MIN_DEFAULT_VALUE);
	}
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
		pr_info(PFX "GPU devfreq located (lazy): %s\n",
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
	if (perf_gpu_devfreq->scaling_min_freq != min_hz) {
		perf_gpu_devfreq->scaling_min_freq = min_hz;
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
	perf_mgr_apply_freq(600);  /* mid floor */
	pr_debug(PFX "touch boost -> mid\n");
}

/* Called after finger lift: release boost */
static void perf_touch_release(struct work_struct *work)
{
	if (!READ_ONCE(perf_touch_boosted))
		return;

	WRITE_ONCE(perf_touch_boosted, false);
	WRITE_ONCE(perf_touch_in_mid, false);
	perf_gpu_set_min(0);
	perf_mgr_apply_freq(0);
	pr_debug(PFX "touch boost released\n");
}

/* Do the actual boosting in process context (input event is atomic!) */
static void perf_touch_do_boost(struct work_struct *work)
{
	WRITE_ONCE(perf_touch_in_mid, false);
	perf_gpu_set_min(touch_gpu_mhz * 1000000UL);
	perf_mgr_apply_freq(1024);  /* full boost (iOS f2) */
	pr_debug(PFX "touch boost on (gpu>=%luMHz)\n", touch_gpu_mhz);
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
			cancel_delayed_work(&perf_touch_work);
			schedule_work(&perf_touch_boost_work);
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
			cancel_delayed_work(&perf_touch_work);
			schedule_work(&perf_touch_boost_work);
			WRITE_ONCE(perf_touch_mid_gen,
				   atomic_inc_return(&perf_touch_gen));
			schedule_delayed_work(&perf_touch_mid_work,
					      msecs_to_jiffies(touch_mid_delay_ms));
		} else {
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
		for_each_online_cpu(cpu) {
			struct cpufreq_policy *p = cpufreq_cpu_get(cpu);
			int qos_ret;

			if (!p || perfmgr_qos_count >= NR_CPUS)
				continue;
			perfmgr_qos_max[perfmgr_qos_count] =
				p->cpuinfo.max_freq;
			qos_ret = freq_qos_add_request(
					&p->constraints,
					&perfmgr_qos[perfmgr_qos_count],
					FREQ_QOS_MIN,
					FREQ_QOS_MIN_DEFAULT_VALUE);
			if (qos_ret < 0) {
				pr_warn(PFX "freq_qos add failed cpu%d: %d\n",
					cpu, qos_ret);
				cpufreq_cpu_put(p);
				continue;
			}
			perfmgr_qos_count++;
			cpufreq_cpu_put(p);
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
			pr_info(PFX "GPU devfreq: %s\n",
				dev_name(&perf_gpu_devfreq->dev));
		} else {
			perf_gpu_found = 0;
			pr_info(PFX "GPU devfreq NOT found (touch boost GPU off)\n");
		}
	}

	INIT_WORK(&perf_touch_boost_work, perf_touch_do_boost);
	INIT_DELAYED_WORK(&perf_touch_mid_work, perf_touch_mid);
	INIT_DELAYED_WORK(&perf_touch_work, perf_touch_release);

	err = input_register_handler(&perf_input_handler);
	if (err) {
		perf_handler_registered = 0;
		pr_warn(PFX "input handler register failed: %d\n", err);
	} else {
		perf_handler_registered = 1;
		pr_info(PFX "touch boost armed\n");
	}

	pr_info(PFX "initialized (fps=%d)\n", g_fps);
	return 0;
}

static void __exit perf_mgr_exit(void)
{
	cancel_work_sync(&perf_touch_boost_work);
	cancel_delayed_work_sync(&perf_touch_mid_work);
	cancel_delayed_work_sync(&perf_touch_work);
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
