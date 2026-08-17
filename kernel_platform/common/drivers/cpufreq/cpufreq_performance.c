// SPDX-License-Identifier: GPL-2.0-only
/*
 *  linux/drivers/cpufreq/cpufreq_performance.c
 *
 *  Copyright (C) 2002 - 2003 Dominik Brodowski <linux@brodo.de>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/module.h>

extern unsigned long perf_mgr_get_cluster_cap(struct cpufreq_policy *p);

static void cpufreq_gov_performance_limits(struct cpufreq_policy *policy)
{
	unsigned long cap = perf_mgr_get_cluster_cap(policy);
	unsigned int freq = policy->max;

	/* clamp mid/little clusters to energy ceiling; big (cap==0) is left
	 * to the FEAS feasd daemon via sysfs scaling_max_freq */
	if (cap && freq > cap)
		freq = cap;

	pr_debug("setting to %u kHz\n", freq);
	/* RELATION_L: hard ceiling even if table lacks an exact cap step */
	__cpufreq_driver_target(policy, freq, CPUFREQ_RELATION_L);
}

static struct cpufreq_governor cpufreq_gov_performance = {
	.name		= "performance",
	.owner		= THIS_MODULE,
	.flags		= CPUFREQ_GOV_STRICT_TARGET,
	.limits		= cpufreq_gov_performance_limits,
};

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_PERFORMANCE
struct cpufreq_governor *cpufreq_default_governor(void)
{
	return &cpufreq_gov_performance;
}
#endif
#ifndef CONFIG_CPU_FREQ_GOV_PERFORMANCE_MODULE
struct cpufreq_governor *cpufreq_fallback_governor(void)
{
	return &cpufreq_gov_performance;
}
#endif

MODULE_AUTHOR("Dominik Brodowski <linux@brodo.de>");
MODULE_DESCRIPTION("CPUfreq policy governor 'performance'");
MODULE_LICENSE("GPL");

cpufreq_governor_init(cpufreq_gov_performance);
cpufreq_governor_exit(cpufreq_gov_performance);
