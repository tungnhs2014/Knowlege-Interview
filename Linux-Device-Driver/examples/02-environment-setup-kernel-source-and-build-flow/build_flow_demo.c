// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <generated/utsrelease.h>

static int __init build_flow_demo_init(void)
{
	pr_info("build_flow_demo: loaded; built for kernel %s\n",
		UTS_RELEASE);
	return 0;
}

static void __exit build_flow_demo_exit(void)
{
	pr_info("build_flow_demo: unloaded\n");
}

module_init(build_flow_demo_init);
module_exit(build_flow_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Device Driver learning project");
MODULE_DESCRIPTION("Minimal external-module build-flow demonstration");
