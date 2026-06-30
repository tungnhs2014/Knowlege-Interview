// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only Linux input subsystem example.
 *
 * The module creates a synthetic platform device so the example can be built
 * and loaded without board-specific GPIO hardware. A production input driver
 * should bind to real firmware-described hardware or use an existing generic
 * driver such as gpio-keys when possible.
 */

#include <linux/delay.h>
#include <linux/input.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>

#define LDD_INPUT_NAME "ldd-input-demo"

static unsigned int key_code = KEY_ENTER;
module_param(key_code, uint, 0444);
MODULE_PARM_DESC(key_code, "Input key code to emit, default KEY_ENTER");

static unsigned int emit_count = 3;
module_param(emit_count, uint, 0444);
MODULE_PARM_DESC(emit_count, "Number of press/release pairs emitted per open");

static unsigned int interval_ms = 250;
module_param(interval_ms, uint, 0444);
MODULE_PARM_DESC(interval_ms, "Delay between synthetic input events in ms");

struct ldd_input_demo {
	struct device *dev;
	struct input_dev *input;
	struct delayed_work work;
	/* Protects event generation state shared by open, close, and work. */
	struct mutex lock;
	unsigned int remaining_events;
	bool active;
	bool press_next;
	bool key_down;
};

static void ldd_input_report(struct ldd_input_demo *demo, int value)
{
	input_report_key(demo->input, key_code, value);
	input_sync(demo->input);
	demo->key_down = value;

	dev_dbg(demo->dev, "reported key code %u value %d\n", key_code, value);
}

static void ldd_input_work(struct work_struct *work)
{
	struct ldd_input_demo *demo;
	unsigned long delay;
	int value;

	demo = container_of(to_delayed_work(work), struct ldd_input_demo, work);

	mutex_lock(&demo->lock);
	if (!demo->active || !demo->remaining_events) {
		mutex_unlock(&demo->lock);
		return;
	}

	value = demo->press_next ? 1 : 0;
	ldd_input_report(demo, value);
	demo->press_next = !demo->press_next;
	demo->remaining_events--;

	if (demo->active && demo->remaining_events) {
		delay = msecs_to_jiffies(interval_ms);
		schedule_delayed_work(&demo->work, delay);
	} else {
		demo->active = false;
	}
	mutex_unlock(&demo->lock);
}

static int ldd_input_open(struct input_dev *input)
{
	struct ldd_input_demo *demo = input_get_drvdata(input);

	mutex_lock(&demo->lock);
	if (!demo->active) {
		demo->remaining_events = emit_count * 2;
		demo->press_next = true;
		demo->active = demo->remaining_events != 0;
		if (demo->active)
			schedule_delayed_work(&demo->work, 0);
	}
	mutex_unlock(&demo->lock);

	return 0;
}

static void ldd_input_close(struct input_dev *input)
{
	struct ldd_input_demo *demo = input_get_drvdata(input);
	bool release_key;

	mutex_lock(&demo->lock);
	demo->active = false;
	demo->remaining_events = 0;
	mutex_unlock(&demo->lock);

	cancel_delayed_work_sync(&demo->work);

	mutex_lock(&demo->lock);
	release_key = demo->key_down;
	if (release_key)
		ldd_input_report(demo, 0);
	mutex_unlock(&demo->lock);
}

static int ldd_input_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ldd_input_demo *demo;
	struct input_dev *input;
	int ret;

	if (key_code > KEY_MAX)
		return dev_err_probe(dev, -EINVAL, "key_code exceeds KEY_MAX\n");

	if (!interval_ms)
		return dev_err_probe(dev, -EINVAL, "interval_ms must be nonzero\n");

	demo = devm_kzalloc(dev, sizeof(*demo), GFP_KERNEL);
	if (!demo)
		return -ENOMEM;

	input = devm_input_allocate_device(dev);
	if (!input)
		return -ENOMEM;

	mutex_init(&demo->lock);
	INIT_DELAYED_WORK(&demo->work, ldd_input_work);

	demo->dev = dev;
	demo->input = input;

	input->name = LDD_INPUT_NAME;
	input->phys = LDD_INPUT_NAME "/input0";
	input->id.bustype = BUS_HOST;
	input->dev.parent = dev;
	input->open = ldd_input_open;
	input->close = ldd_input_close;

	input_set_drvdata(input, demo);
	input_set_capability(input, EV_KEY, key_code);

	ret = input_register_device(input);
	if (ret)
		return dev_err_probe(dev, ret, "failed to register input device\n");

	platform_set_drvdata(pdev, demo);
	dev_info(dev, "registered learning-only input demo, key_code=%u\n",
		 key_code);

	return 0;
}

static int ldd_input_remove(struct platform_device *pdev)
{
	struct ldd_input_demo *demo = platform_get_drvdata(pdev);

	mutex_lock(&demo->lock);
	demo->active = false;
	demo->remaining_events = 0;
	mutex_unlock(&demo->lock);

	cancel_delayed_work_sync(&demo->work);
	dev_info(&pdev->dev, "removed learning-only input demo\n");

	return 0;
}

static struct platform_driver ldd_input_driver = {
	.probe = ldd_input_probe,
	.remove = ldd_input_remove,
	.driver = {
		.name = LDD_INPUT_NAME,
	},
};

static struct platform_device *ldd_input_pdev;

static int __init ldd_input_init(void)
{
	int ret;

	ret = platform_driver_register(&ldd_input_driver);
	if (ret)
		return ret;

	ldd_input_pdev = platform_device_register_simple(LDD_INPUT_NAME,
							 PLATFORM_DEVID_AUTO,
							 NULL, 0);
	if (IS_ERR(ldd_input_pdev)) {
		ret = PTR_ERR(ldd_input_pdev);
		platform_driver_unregister(&ldd_input_driver);
		return ret;
	}

	return 0;
}

static void __exit ldd_input_exit(void)
{
	platform_device_unregister(ldd_input_pdev);
	platform_driver_unregister(&ldd_input_driver);
}

module_init(ldd_input_init);
module_exit(ldd_input_exit);

MODULE_AUTHOR("TungNHS");
MODULE_DESCRIPTION("Learning-only Linux input device driver example");
MODULE_LICENSE("GPL");
