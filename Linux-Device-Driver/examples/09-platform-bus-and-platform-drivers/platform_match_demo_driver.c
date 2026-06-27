// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only platform driver.
 *
 * This module demonstrates platform bus matching, probe/remove, per-device
 * state, platform data, and devm-managed allocation. It intentionally does
 * not map fake MMIO or request fake IRQs.
 */

#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>

struct platform_match_demo_pdata {
	u32 instance_id;
	const char *label;
};

struct platform_match_demo {
	struct device *dev;
	u32 instance_id;
	const char *label;
};

static int platform_match_demo_probe(struct platform_device *pdev)
{
	struct platform_match_demo_pdata *pdata;
	struct platform_match_demo *demo;

	demo = devm_kzalloc(&pdev->dev, sizeof(*demo), GFP_KERNEL);
	if (!demo)
		return -ENOMEM;

	pdata = dev_get_platdata(&pdev->dev);

	demo->dev = &pdev->dev;
	demo->instance_id = pdata ? pdata->instance_id : (u32)pdev->id;
	demo->label = pdata && pdata->label ? pdata->label : "no-platform-data";

	platform_set_drvdata(pdev, demo);

	dev_info(&pdev->dev,
		 "probe: name=%s id=%d instance=%u label=%s\n",
		 pdev->name, pdev->id, demo->instance_id, demo->label);

	return 0;
}

static void platform_match_demo_remove(struct platform_device *pdev)
{
	struct platform_match_demo *demo = platform_get_drvdata(pdev);

	dev_info(&pdev->dev,
		 "remove: instance=%u label=%s\n",
		 demo->instance_id, demo->label);
}

static const struct platform_device_id platform_match_demo_ids[] = {
	{ "lld-platform-demo", 0 },
	{ }
};
MODULE_DEVICE_TABLE(platform, platform_match_demo_ids);

static struct platform_driver platform_match_demo_driver = {
	.probe = platform_match_demo_probe,
	.remove_new = platform_match_demo_remove,
	.id_table = platform_match_demo_ids,
	.driver = {
		.name = "lld-platform-demo",
	},
};
module_platform_driver(platform_match_demo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Learning Path 09");
MODULE_DESCRIPTION("Learning-only platform bus matching demo driver");
