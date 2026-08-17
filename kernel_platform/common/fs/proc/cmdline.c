// SPDX-License-Identifier: GPL-2.0
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#ifdef CONFIG_KSU_CMDLINE_SPOOF
#include <linux/moduleparam.h>
#include <linux/string.h>
#endif

/*
 * cmdline spoofing (independent of SUSFS) for tampered-kernel hiding.
 * When CONFIG_KSU_CMDLINE_SPOOF=y, /proc/cmdline reports the stock-like
 * string below (or one set at runtime via /sys/module/...); the kernel
 * itself keeps using the real saved_command_line, so no functional impact.
 */
#ifdef CONFIG_KSU_CMDLINE_SPOOF
static char spoof_cmdline[1024] = CONFIG_KSU_CMDLINE_SPOOF_STRING;
#endif

static int cmdline_proc_show(struct seq_file *m, void *v)
{
#ifdef CONFIG_KSU_CMDLINE_SPOOF
	if (spoof_cmdline[0])
		seq_puts(m, spoof_cmdline);
	else
#endif
		seq_puts(m, saved_command_line);
	seq_putc(m, '\n');
	return 0;
}

#ifdef CONFIG_KSU_CMDLINE_SPOOF
/* runtime update: echo <cmdline> > /sys/module/cmdline_spoof/parameters/spoof */
static int spoof_set(const char *val, const struct kernel_param *kp)
{
	size_t n = strnlen(val, sizeof(spoof_cmdline) - 1);
	memcpy(spoof_cmdline, val, n);
	spoof_cmdline[n] = '\0';
	return 0;
}
static int spoof_get(char *buf, const struct kernel_param *kp)
{
	return scnprintf(buf, PAGE_SIZE, "%s", spoof_cmdline);
}
static const struct kernel_param_ops spoof_ops = {
	.set = spoof_set,
	.get = spoof_get,
};
module_param_cb(spoof_cmdline, &spoof_ops, NULL, 0644);
#endif

static int __init proc_cmdline_init(void)
{
	proc_create_single("cmdline", 0, NULL, cmdline_proc_show);
	return 0;
}
fs_initcall(proc_cmdline_init);
