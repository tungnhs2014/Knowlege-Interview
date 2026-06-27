// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only driver lifecycle demo.
 *
 * The module registers a platform driver. Hardware-like initialization happens
 * only when a matching platform device exists and the driver core calls probe.
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>

struct overview_demo_data {
	u32 instance_id;
};

struct overview_demo {
	struct device *dev;
	u32 instance_id;
};

static bool fail_probe;
module_param(fail_probe, bool, 0644);
MODULE_PARM_DESC(fail_probe, "Force probe to fail after managed allocation");

static void overview_demo_release(void *data)
{
	struct overview_demo *demo = data;

	dev_info(demo->dev, "managed cleanup: instance=%u\n",
		 demo->instance_id);
}

static int overview_demo_probe(struct platform_device *pdev)
{
	const struct overview_demo_data *pdata;
	struct overview_demo *demo;
	int ret;

	dev_info(&pdev->dev, "probe entered\n");

	demo = devm_kzalloc(&pdev->dev, sizeof(*demo), GFP_KERNEL);
	if (!demo)
		return -ENOMEM;

	pdata = dev_get_platdata(&pdev->dev);
	demo->dev = &pdev->dev;
	demo->instance_id = pdata ? pdata->instance_id : 0;
	platform_set_drvdata(pdev, demo);

	ret = devm_add_action_or_reset(&pdev->dev, overview_demo_release, demo);
	if (ret)
		return ret;

	if (fail_probe) {
		dev_err(&pdev->dev, "forced probe failure\n");
		return -EIO;
	}

	dev_info(&pdev->dev, "probe complete: instance=%u\n",
		 demo->instance_id);

	return 0;
}

static void overview_demo_remove(struct platform_device *pdev)
{
	struct overview_demo *demo = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "remove: instance=%u\n", demo->instance_id);
}

static const struct platform_device_id overview_demo_ids[] = {
	{ "lld-overview-demo", 0 },
	{ }
};
MODULE_DEVICE_TABLE(platform, overview_demo_ids);

static struct platform_driver overview_demo_driver = {
	.probe = overview_demo_probe,
	.remove_new = overview_demo_remove,
	.id_table = overview_demo_ids,
	.driver = {
		.name = "lld-overview-demo",
	},
};

static int __init overview_demo_driver_init(void)
{
	pr_info("overview_demo_driver: registering driver\n");
	return platform_driver_register(&overview_demo_driver);
}

static void __exit overview_demo_driver_exit(void)
{
	pr_info("overview_demo_driver: unregistering driver\n");
	platform_driver_unregister(&overview_demo_driver);
}

module_init(overview_demo_driver_init);
module_exit(overview_demo_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Linux Device Driver Learning Path");
MODULE_DESCRIPTION("Learning-only driver registration and probe demo");
