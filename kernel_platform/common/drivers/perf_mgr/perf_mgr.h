/* SPDX-License-Identifier: GPL-2.0 */
/*
 * perf_mgr.h - FPS-driven energy-aware frequency governor interface
 *
 * Ported from Samsung GPIS (sm8150-samsung perf_mgr) to GKI 5.15.
 * Adapted for Sony Xperia 1 V (SM8550): no private task_struct fields,
 * no panel notifier dependency; target FPS comes from userspace.
 *
 * Userspace protocol:
 *   open("/dev/perf_manager")
 *   ioctl(fd, PERF_MGR_TASK_ADD, struct fps_info)   - register render thread
 *   ioctl(fd, PERF_MGR_FRAME_END, struct fps_info)  - per-frame report
 *   ioctl(fd, PERF_MGR_SET_FPS, &int)               - set target FPS
 *   ioctl(fd, PERF_MGR_PROCESS_KILL, &int)          - unregister
 */
#ifndef _PERF_MGR_H_
#define _PERF_MGR_H_

#include <linux/ioctl.h>
#include <linux/types.h>

#define PERF_MGR_IOC_MAGIC 'P'

/* Register a task (render thread) with its group id */
struct fps_info {
	__s32 tid;
	__s32 group_id;        /* apps with same group share boost state */
	__s32 boosting_lvl;    /* 0=off, 1=low, 2=mid, 3=high */
	__s64 duration;        /* frame duration in ns (FRAME_END only) */
};

#define PERF_MGR_FPS_NUM     _IOR(PERF_MGR_IOC_MAGIC, 1, int)
#define PERF_MGR_TASK_ADD    _IOW(PERF_MGR_IOC_MAGIC, 2, struct fps_info)
#define PERF_MGR_FRAME_END   _IOW(PERF_MGR_IOC_MAGIC, 3, struct fps_info)
#define PERF_MGR_PROCESS_KILL _IOW(PERF_MGR_IOC_MAGIC, 4, int)
#define PERF_MGR_SET_FPS     _IOW(PERF_MGR_IOC_MAGIC, 5, int)

/* Boost levels */
#define BOOST_OFF  0
#define BOOST_LOW  1
#define BOOST_MID  2
#define BOOST_HIGH 3

/* Fixed-point scale for util calculations */
#define FP_SCALE 1024

#endif /* _PERF_MGR_H_ */
