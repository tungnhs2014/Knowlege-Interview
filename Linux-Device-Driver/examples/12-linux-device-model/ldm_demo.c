// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only Linux Device Model demo.
 *
 * This module creates a synthetic platform device and a matching platform
 * driver. The driver's probe path creates a class device with one sysfs
 * attribute, so learners can inspect /sys/devices, /sys/bus/platform, and
 * /sys/class as different views of related device-model objects.
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/mod_devicetable.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#define LDM_DEMO_NAME "lld-ldm-demo"

struct ldm_demo {
	struct platform_device *pdev;
	struct device *class_dev;
	struct mutex lock;
	bool enabled;
};

static struct class *ldm_demo_class;
static struct platform_device *ldm_demo_pdev;

static ssize_t enabled_show(struct device *dev,
			    struct device_attribute *attr,
			    char *buf)
{
	struct ldm_demo *demo = dev_get_drvdata(dev);
	bool enabled;

	mutex_lock(&demo->lock);
	enabled = demo->enabled;
	mutex_unlock(&demo->lock);

	return sysfs_emit(buf, "%u\n", enabled);
}

static ssize_t enabled_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf,
			     size_t count)
{
	struct ldm_demo *demo = dev_get_drvdata(dev);
	bool enabled;
	int ret;

	ret = kstrtobool(buf, &enabled);
	if (ret)
		return ret;

	mutex_lock(&demo->lock);
	demo->enabled = enabled;
	mutex_unlock(&demo->lock);

	dev_info(dev, "enabled=%u\n", enabled);

	return count;
}
static DEVICE_ATTR_RW(enabled);

static struct attribute *ldm_demo_attrs[] = {
	&dev_attr_enabled.attr,
	NULL,
};
ATTRIBUTE_GROUPS(ldm_demo);

static int ldm_demo_probe(struct platform_device *pdev)
{
	struct ldm_demo *demo;

	demo = devm_kzalloc(&pdev->dev, sizeof(*demo), GFP_KERNEL);
	if (!demo)
		return -ENOMEM;

	mutex_init(&demo->lock);
	demo->pdev = pdev;
	platform_set_drvdata(pdev, demo);

	demo->class_dev = device_create_with_groups(ldm_demo_class,
						    &pdev->dev,
						    0,
						    demo,
						    ldm_demo_groups,
						    "ldm-demo%d",
						    pdev->id);
	if (IS_ERR(demo->class_dev))
		return dev_err_probe(&pdev->dev,
				     PTR_ERR(demo->class_dev),
				     "failed to create class device\n");

	dev_info(&pdev->dev,
		 "probe: created /sys/class/ldm_demo/ldm-demo%d/enabled\n",
		 pdev->id);

	return 0;
}

static void ldm_demo_remove(struct platform_device *pdev)
{
	struct ldm_demo *demo = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "remove: unregistering class device\n");
	device_unregister(demo->class_dev);
}

static const struct platform_device_id ldm_demo_ids[] = {
	{ LDM_DEMO_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(platform, ldm_demo_ids);

static struct platform_driver ldm_demo_driver = {
	.probe = ldm_demo_probe,
	.remove_new = ldm_demo_remove,
	.id_table = ldm_demo_ids,
	.driver = {
		.name = LDM_DEMO_NAME,
	},
};

static int __init ldm_demo_init(void)
{
	int ret;

	ldm_demo_class = class_create("ldm_demo");
	if (IS_ERR(ldm_demo_class))
		return PTR_ERR(ldm_demo_class);

	ret = platform_driver_register(&ldm_demo_driver);
	if (ret)
		goto err_destroy_class;

	ldm_demo_pdev = platform_device_register_simple(LDM_DEMO_NAME,
							0, NULL, 0);
	if (IS_ERR(ldm_demo_pdev)) {
		ret = PTR_ERR(ldm_demo_pdev);
		goto err_unregister_driver;
	}

	pr_info("ldm_demo: loaded synthetic platform device and driver\n");
	return 0;

err_unregister_driver:
	platform_driver_unregister(&ldm_demo_driver);
err_destroy_class:
	class_destroy(ldm_demo_class);
	return ret;
}

static void __exit ldm_demo_exit(void)
{
	platform_device_unregister(ldm_demo_pdev);
	platform_driver_unregister(&ldm_demo_driver);
	class_destroy(ldm_demo_class);
	pr_info("ldm_demo: unloaded\n");
}

module_init(ldm_demo_init);
module_exit(ldm_demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Learning Path 12");
MODULE_DESCRIPTION("Learning-only Linux Device Model demo");
