// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only pinctrl and GPIO consumer demo.
 *
 * This demonstrates descriptor GPIO lookup, logical active-low handling,
 * optional GPIOs, GPIO-to-IRQ mapping, threaded IRQ context, and explicit
 * pinctrl default/sleep state switching for PM.
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/pm.h>

struct pinctrl_gpio_consumer_demo {
	struct device *dev;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *enable_gpio;
	struct gpio_desc *event_gpio;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_sleep;
	int irq;
};

static irqreturn_t pinctrl_gpio_consumer_demo_irq(int irq, void *data)
{
	struct pinctrl_gpio_consumer_demo *demo = data;
	int active;

	active = gpiod_get_value_cansleep(demo->event_gpio);
	if (active < 0) {
		dev_warn(demo->dev, "irq: failed to read event GPIO: %d\n",
			 active);
		return IRQ_NONE;
	}

	dev_info(demo->dev, "irq: event GPIO logical value=%d\n", active);

	return IRQ_HANDLED;
}

static int pinctrl_gpio_consumer_demo_get_pinctrl(
	struct pinctrl_gpio_consumer_demo *demo)
{
	int ret;

	demo->pinctrl = devm_pinctrl_get(demo->dev);
	if (IS_ERR(demo->pinctrl)) {
		ret = PTR_ERR(demo->pinctrl);
		demo->pinctrl = NULL;

		if (ret == -ENODEV) {
			dev_info(demo->dev, "no explicit pinctrl states\n");
			return 0;
		}

		return dev_err_probe(demo->dev, ret, "get pinctrl\n");
	}

	demo->pins_default = pinctrl_lookup_state(demo->pinctrl, "default");
	if (IS_ERR(demo->pins_default)) {
		dev_dbg(demo->dev, "no default pinctrl state\n");
		demo->pins_default = NULL;
	}

	demo->pins_sleep = pinctrl_lookup_state(demo->pinctrl, "sleep");
	if (IS_ERR(demo->pins_sleep)) {
		dev_dbg(demo->dev, "no sleep pinctrl state\n");
		demo->pins_sleep = NULL;
	}

	return 0;
}

static int pinctrl_gpio_consumer_demo_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct pinctrl_gpio_consumer_demo *demo;
	int ret;

	demo = devm_kzalloc(dev, sizeof(*demo), GFP_KERNEL);
	if (!demo)
		return -ENOMEM;

	demo->dev = dev;
	demo->irq = -1;
	platform_set_drvdata(pdev, demo);

	ret = pinctrl_gpio_consumer_demo_get_pinctrl(demo);
	if (ret)
		return ret;

	demo->reset_gpio = devm_gpiod_get_optional(dev, "reset",
						   GPIOD_OUT_HIGH);
	if (IS_ERR(demo->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(demo->reset_gpio),
				     "get reset GPIO\n");

	demo->enable_gpio = devm_gpiod_get_optional(dev, "enable",
						    GPIOD_OUT_LOW);
	if (IS_ERR(demo->enable_gpio))
		return dev_err_probe(dev, PTR_ERR(demo->enable_gpio),
				     "get enable GPIO\n");

	demo->event_gpio = devm_gpiod_get_optional(dev, "event", GPIOD_IN);
	if (IS_ERR(demo->event_gpio))
		return dev_err_probe(dev, PTR_ERR(demo->event_gpio),
				     "get event GPIO\n");

	if (demo->enable_gpio) {
		gpiod_set_value_cansleep(demo->enable_gpio, 1);
		dev_info(dev, "enable GPIO asserted logically\n");
	}

	if (demo->reset_gpio) {
		dev_info(dev, "reset GPIO active_low=%d\n",
			 gpiod_is_active_low(demo->reset_gpio));
		gpiod_set_value_cansleep(demo->reset_gpio, 1);
		usleep_range(1000, 2000);
		gpiod_set_value_cansleep(demo->reset_gpio, 0);
		usleep_range(5000, 10000);
		dev_info(dev, "reset pulse complete using logical values\n");
	}

	if (demo->event_gpio) {
		demo->irq = gpiod_to_irq(demo->event_gpio);
		if (demo->irq < 0)
			return dev_err_probe(dev, demo->irq,
					     "map event GPIO to IRQ\n");

		ret = devm_request_threaded_irq(dev, demo->irq, NULL,
						pinctrl_gpio_consumer_demo_irq,
						IRQF_TRIGGER_FALLING |
						IRQF_ONESHOT,
						dev_name(dev), demo);
		if (ret)
			return dev_err_probe(dev, ret, "request IRQ %d\n",
					     demo->irq);

		dev_info(dev, "event GPIO mapped to IRQ %d\n", demo->irq);
	}

	dev_info(dev, "probe complete\n");

	return 0;
}

static void pinctrl_gpio_consumer_demo_remove(struct platform_device *pdev)
{
	struct pinctrl_gpio_consumer_demo *demo = platform_get_drvdata(pdev);

	if (demo->enable_gpio)
		gpiod_set_value_cansleep(demo->enable_gpio, 0);

	if (demo->reset_gpio)
		gpiod_set_value_cansleep(demo->reset_gpio, 1);

	dev_info(&pdev->dev, "remove: outputs placed in safe logical state\n");
}

static int pinctrl_gpio_consumer_demo_suspend(struct device *dev)
{
	struct pinctrl_gpio_consumer_demo *demo = dev_get_drvdata(dev);
	int ret;

	if (demo->enable_gpio)
		gpiod_set_value_cansleep(demo->enable_gpio, 0);

	if (demo->pins_sleep) {
		ret = pinctrl_select_state(demo->pinctrl, demo->pins_sleep);
		if (ret)
			return ret;
	}

	dev_info(dev, "suspend: selected sleep pins\n");

	return 0;
}

static int pinctrl_gpio_consumer_demo_resume(struct device *dev)
{
	struct pinctrl_gpio_consumer_demo *demo = dev_get_drvdata(dev);
	int ret;

	if (demo->pins_default) {
		ret = pinctrl_select_state(demo->pinctrl, demo->pins_default);
		if (ret)
			return ret;
	}

	if (demo->enable_gpio)
		gpiod_set_value_cansleep(demo->enable_gpio, 1);

	dev_info(dev, "resume: restored default pins\n");

	return 0;
}

static const struct dev_pm_ops pinctrl_gpio_consumer_demo_pm_ops = {
	.suspend = pinctrl_gpio_consumer_demo_suspend,
	.resume = pinctrl_gpio_consumer_demo_resume,
};

static const struct of_device_id pinctrl_gpio_consumer_demo_of_match[] = {
	{ .compatible = "acme,pinctrl-gpio-consumer-demo" },
	{ }
};
MODULE_DEVICE_TABLE(of, pinctrl_gpio_consumer_demo_of_match);

static struct platform_driver pinctrl_gpio_consumer_demo_driver = {
	.probe = pinctrl_gpio_consumer_demo_probe,
	.remove_new = pinctrl_gpio_consumer_demo_remove,
	.driver = {
		.name = "pinctrl-gpio-consumer-demo",
		.of_match_table = pinctrl_gpio_consumer_demo_of_match,
		.pm = &pinctrl_gpio_consumer_demo_pm_ops,
	},
};
module_platform_driver(pinctrl_gpio_consumer_demo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Learning Path 13");
MODULE_DESCRIPTION("Learning-only pinctrl and GPIO consumer API demo");
