// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only platform device provider.
 *
 * This module creates one synthetic platform_device so the matching driver can
 * probe without requiring real board hardware or a Device Tree overlay.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/types.h>

struct platform_match_demo_pdata {
	u32 instance_id;
	const char *label;
};

static struct platform_device *demo_pdev;

static struct platform_match_demo_pdata demo_pdata = {
	.instance_id = 42,
	.label = "runtime-created-platform-device",
};

static int __init platform_match_demo_device_init(void)
{
	demo_pdev = platform_device_register_data(NULL,
						  "lld-platform-demo",
						  PLATFORM_DEVID_AUTO,
						  &demo_pdata,
						  sizeof(demo_pdata));
	if (IS_ERR(demo_pdev))
		return PTR_ERR(demo_pdev);

	pr_info("platform_match_demo_device: registered %s.%d\n",
		demo_pdev->name, demo_pdev->id);

	return 0;
}

static void __exit platform_match_demo_device_exit(void)
{
	pr_info("platform_match_demo_device: unregistering %s.%d\n",
		demo_pdev->name, demo_pdev->id);

	platform_device_unregister(demo_pdev);
}

module_init(platform_match_demo_device_init);
module_exit(platform_match_demo_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Learning Path 09");
MODULE_DESCRIPTION("Learning-only runtime platform_device provider");
