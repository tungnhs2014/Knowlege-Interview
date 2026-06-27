// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only synthetic device provider.
 *
 * The module creates one platform device so the overview driver can demonstrate
 * matching, probe, remove, and cleanup without requiring physical hardware.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/types.h>

struct overview_demo_data {
	u32 instance_id;
};

static const struct overview_demo_data overview_demo_pdata = {
	.instance_id = 1,
};

static struct platform_device *overview_demo_pdev;

static int __init overview_demo_device_init(void)
{
	overview_demo_pdev =
		platform_device_register_data(NULL, "lld-overview-demo",
					      PLATFORM_DEVID_AUTO,
					      &overview_demo_pdata,
					      sizeof(overview_demo_pdata));
	if (IS_ERR(overview_demo_pdev))
		return PTR_ERR(overview_demo_pdev);

	pr_info("overview_demo_device: registered %s.%d\n",
		overview_demo_pdev->name, overview_demo_pdev->id);

	return 0;
}

static void __exit overview_demo_device_exit(void)
{
	pr_info("overview_demo_device: unregistering %s.%d\n",
		overview_demo_pdev->name, overview_demo_pdev->id);
	platform_device_unregister(overview_demo_pdev);
}

module_init(overview_demo_device_init);
module_exit(overview_demo_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Device Driver Learning Path");
MODULE_DESCRIPTION("Learning-only synthetic platform device");
