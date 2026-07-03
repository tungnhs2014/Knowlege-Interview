// SPDX-License-Identifier: GPL-2.0
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

static unsigned int interval_ms = 1000;
module_param(interval_ms, uint, 0444);
MODULE_PARM_DESC(interval_ms, "Timer interval in milliseconds");

static struct device *demo_dev;
static struct timer_list demo_timer;
static struct work_struct demo_work;
static unsigned long demo_ticks;

static void ldd_dbgtrace_work(struct work_struct *work)
{
	demo_ticks++;

	dev_dbg(demo_dev, "work ran, ticks=%lu interval_ms=%u\n",
		demo_ticks, interval_ms);

	if (demo_ticks % 5 == 0)
		dev_info(demo_dev, "heartbeat ticks=%lu\n", demo_ticks);
}

static void ldd_dbgtrace_timer(struct timer_list *timer)
{
	queue_work(system_wq, &demo_work);
	mod_timer(&demo_timer, jiffies + msecs_to_jiffies(interval_ms));
}

static int __init ldd_dbgtrace_init(void)
{
	if (!interval_ms || interval_ms > 60000)
		return -EINVAL;

	demo_dev = root_device_register("ldd_dbgtrace");
	if (IS_ERR(demo_dev))
		return PTR_ERR(demo_dev);

	INIT_WORK(&demo_work, ldd_dbgtrace_work);
	timer_setup(&demo_timer, ldd_dbgtrace_timer, 0);
	mod_timer(&demo_timer, jiffies + msecs_to_jiffies(interval_ms));

	dev_info(demo_dev, "loaded, interval_ms=%u\n", interval_ms);
	dev_dbg(demo_dev, "dynamic debug callsite is active\n");

	return 0;
}

static void __exit ldd_dbgtrace_exit(void)
{
	del_timer_sync(&demo_timer);
	cancel_work_sync(&demo_work);

	dev_info(demo_dev, "unloaded after %lu ticks\n", demo_ticks);
	root_device_unregister(demo_dev);
}

module_init(ldd_dbgtrace_init);
module_exit(ldd_dbgtrace_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Device Driver learning path");
MODULE_DESCRIPTION("Learning-only kernel debugging and tracing demo");
