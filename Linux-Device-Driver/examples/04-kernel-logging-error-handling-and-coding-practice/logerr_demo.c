// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only logging and error-path example.
 *
 * This module does not bind to hardware. It demonstrates negative errno
 * returns, ERR_PTR()/IS_ERR()/PTR_ERR(), cleanup labels, log levels,
 * pr_fmt(), pr_debug(), and module-parameter-driven failure injection.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>

#define LOGERR_BUF_SIZE 128

struct logerr_state {
	char *name;
	u8 *rx_buf;
	u8 *tx_buf;
};

static int fail_step;
static int debug_level;
static struct logerr_state *demo;

module_param(fail_step, int, 0444);
MODULE_PARM_DESC(fail_step,
		 "Failure injection: 0=success, 1=ERR_PTR helper, 2=after rx alloc, 3=after tx alloc");

module_param(debug_level, int, 0644);
MODULE_PARM_DESC(debug_level, "Debug level: 0=quiet, 1=basic debug log");

static char *logerr_build_name(void)
{
	char *name;

	if (fail_step == 1)
		return ERR_PTR(-ENODEV);

	name = kstrdup("logerr-demo0", GFP_KERNEL);
	if (!name)
		return ERR_PTR(-ENOMEM);

	return name;
}

static int __init logerr_demo_init(void)
{
	int ret;

	pr_info("init start fail_step=%d debug_level=%d\n",
		fail_step, debug_level);

	demo = kzalloc(sizeof(*demo), GFP_KERNEL);
	if (!demo)
		return -ENOMEM;

	demo->name = logerr_build_name();
	if (IS_ERR(demo->name)) {
		ret = PTR_ERR(demo->name);
		pr_err("failed to build logical name: %d\n", ret);
		goto err_free_state;
	}

	demo->rx_buf = kmalloc(LOGERR_BUF_SIZE, GFP_KERNEL);
	if (!demo->rx_buf) {
		ret = -ENOMEM;
		pr_err("failed to allocate rx buffer: %d\n", ret);
		goto err_free_name;
	}

	pr_debug("rx buffer allocated for %s\n", demo->name);

	if (fail_step == 2) {
		ret = -EBUSY;
		pr_warn("simulated busy resource after rx allocation: %d\n", ret);
		goto err_free_rx;
	}

	demo->tx_buf = kmalloc(LOGERR_BUF_SIZE, GFP_KERNEL);
	if (!demo->tx_buf) {
		ret = -ENOMEM;
		pr_err("failed to allocate tx buffer: %d\n", ret);
		goto err_free_rx;
	}

	pr_debug("tx buffer allocated for %s\n", demo->name);

	if (fail_step == 3) {
		ret = -EIO;
		pr_err("simulated hardware setup failure: %d\n", ret);
		goto err_free_tx;
	}

	if (debug_level)
		pr_debug("debug breadcrumb: buffers are ready\n");

	pr_info("loaded successfully name=%s\n", demo->name);
	return 0;

err_free_tx:
	kfree(demo->tx_buf);
	demo->tx_buf = NULL;
err_free_rx:
	kfree(demo->rx_buf);
	demo->rx_buf = NULL;
err_free_name:
	kfree(demo->name);
	demo->name = NULL;
err_free_state:
	kfree(demo);
	demo = NULL;
	return ret;
}

static void __exit logerr_demo_exit(void)
{
	pr_info("exit start\n");

	kfree(demo->tx_buf);
	kfree(demo->rx_buf);
	kfree(demo->name);
	kfree(demo);
	demo = NULL;

	pr_info("unloaded\n");
}

module_init(logerr_demo_init);
module_exit(logerr_demo_exit);

MODULE_AUTHOR("Linux Device Driver learning example");
MODULE_DESCRIPTION("Learning-only logging and error-handling demo");
MODULE_LICENSE("GPL");
