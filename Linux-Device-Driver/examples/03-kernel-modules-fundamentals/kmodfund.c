// SPDX-License-Identifier: GPL-2.0
/*
 * kmodfund.c - learning-only kernel module fundamentals example.
 *
 * This module demonstrates load/unload entry points, MODULE_* metadata,
 * module parameters, sysfs parameter visibility, and reverse-order cleanup
 * after a simulated initialization failure.
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>

#define KMODFUND_BUF_SIZE 64

static int debug_level;
static char *device_name = "kmodfund0";
static bool fail_after_alloc;
static int values[4];
static int value_count;
static char *demo_buf;

module_param(debug_level, int, 0644);
MODULE_PARM_DESC(debug_level, "Debug level: 0=off, 1=basic, 2=verbose");

module_param(device_name, charp, 0444);
MODULE_PARM_DESC(device_name, "Logical name printed by the demo module");

module_param(fail_after_alloc, bool, 0444);
MODULE_PARM_DESC(fail_after_alloc, "Force init failure after allocation");

module_param_array(values, int, &value_count, 0444);
MODULE_PARM_DESC(values, "Optional integer list, up to 4 values");

static void kmodfund_print_values(void)
{
	int i;

	for (i = 0; i < value_count; i++)
		pr_info("kmodfund: values[%d]=%d\n", i, values[i]);
}

static int __init kmodfund_init(void)
{
	int ret = 0;

	pr_info("kmodfund: init start name=%s debug=%d\n",
		device_name, debug_level);

	demo_buf = kzalloc(KMODFUND_BUF_SIZE, GFP_KERNEL);
	if (!demo_buf)
		return -ENOMEM;

	snprintf(demo_buf, KMODFUND_BUF_SIZE, "hello from %s", device_name);
	kmodfund_print_values();

	if (fail_after_alloc) {
		pr_err("kmodfund: simulated init failure after allocation\n");
		ret = -EINVAL;
		goto err_free_buf;
	}

	pr_info("kmodfund: loaded message=\"%s\"\n", demo_buf);
	return 0;

err_free_buf:
	kfree(demo_buf);
	demo_buf = NULL;
	return ret;
}

static void __exit kmodfund_exit(void)
{
	pr_info("kmodfund: exit start\n");

	kfree(demo_buf);
	demo_buf = NULL;

	pr_info("kmodfund: unloaded\n");
}

module_init(kmodfund_init);
module_exit(kmodfund_exit);

MODULE_AUTHOR("Linux Device Driver learning example");
MODULE_DESCRIPTION("Learning-only module fundamentals demo");
MODULE_LICENSE("GPL");
