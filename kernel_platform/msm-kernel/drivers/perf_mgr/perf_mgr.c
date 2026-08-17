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
#include <linux/cpufreq.h>
#include <linux/pm_qos.h>
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
static int fps_margin_percent = 10;
static int hold_frame_count = 2;
static bool perfmgr_enable = true;

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
		margin = (us_frame_time * (fps_margin_percent * 10)) >> 10;

	if (g_fps == 0 || us_frame_time == 0)
		return 0;

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

	if (!p || !p->se.on_rq)
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
				if (tmp && task_rq(tmp)->cpu == task_cpu(task))
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

static const struct file_operations perf_mgr_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = perf_mgr_ioctl,
};

static struct miscdevice perf_mgr_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "perf_manager",
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
perf_attr(enable);

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
perf_attr(fps);

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
perf_attr(margin);

static struct attribute *perf_attrs[] = {
	&enable_attr.attr,
	&fps_attr.attr,
	&margin_attr.attr,
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
	if (err) {
		kobject_put(perf_kobj);
		misc_deregister(&perf_mgr_device);
		return err;
	}

	pr_info(PFX "initialized (fps=%d)\n", g_fps);
	return 0;
}

static void __exit perf_mgr_exit(void)
{
	sysfs_remove_group(perf_kobj, &perf_attr_group);
	kobject_put(perf_kobj);
	misc_deregister(&perf_mgr_device);
}

module_init(perf_mgr_init);
module_exit(perf_mgr_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("FPS-driven energy-aware frequency governor (FEAS-like)");
MODULE_VERSION("0.1.0");
