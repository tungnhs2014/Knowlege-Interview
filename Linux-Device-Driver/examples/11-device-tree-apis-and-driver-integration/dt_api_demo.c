// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only DT-backed platform driver.
 *
 * This demonstrates OF matching, match data, standard platform resource
 * lookup, typed DT property reading, optional clock/GPIO consumers, IRQ
 * request, and cleanup for stateful clock enable.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/types.h>

struct dt_api_demo_variant {
	const char *name;
	u32 fifo_depth;
	u32 status_offset;
};

struct dt_api_demo {
	struct device *dev;
	const struct dt_api_demo_variant *variant;
	void __iomem *base;
	struct clk *core_clk;
	struct gpio_desc *reset_gpio;
	int irq;
	u32 sample_period_ns;
	bool use_dma;
};

static const struct dt_api_demo_variant dt_api_demo_v1 = {
	.name = "v1",
	.fifo_depth = 16,
	.status_offset = 0x00,
};

static const struct dt_api_demo_variant dt_api_demo_v2 = {
	.name = "v2",
	.fifo_depth = 64,
	.status_offset = 0x04,
};

static void dt_api_demo_clk_disable(void *data)
{
	struct dt_api_demo *demo = data;

	clk_disable_unprepare(demo->core_clk);
}

static irqreturn_t dt_api_demo_irq(int irq, void *data)
{
	struct dt_api_demo *demo = data;
	u32 status;

	status = readl(demo->base + demo->variant->status_offset);
	dev_info(demo->dev, "irq: status=0x%08x\n", status);

	return IRQ_HANDLED;
}

static int dt_api_demo_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct dt_api_demo *demo;
	const struct of_device_id *match;
	int ret;

	match = of_match_device(dev->driver->of_match_table, dev);
	if (!match)
		return dev_err_probe(dev, -ENODEV, "missing OF match data\n");

	demo = devm_kzalloc(dev, sizeof(*demo), GFP_KERNEL);
	if (!demo)
		return -ENOMEM;

	demo->dev = dev;
	demo->variant = match->data;
	platform_set_drvdata(pdev, demo);

	demo->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(demo->base))
		return dev_err_probe(dev, PTR_ERR(demo->base), "MMIO resource\n");

	demo->irq = platform_get_irq(pdev, 0);
	if (demo->irq < 0)
		return dev_err_probe(dev, demo->irq, "IRQ resource\n");

	demo->core_clk = devm_clk_get_optional(dev, "core");
	if (IS_ERR(demo->core_clk))
		return dev_err_probe(dev, PTR_ERR(demo->core_clk), "core clock\n");

	if (demo->core_clk) {
		ret = clk_prepare_enable(demo->core_clk);
		if (ret)
			return dev_err_probe(dev, ret, "enable core clock\n");

		ret = devm_add_action_or_reset(dev, dt_api_demo_clk_disable,
					       demo);
		if (ret)
			return ret;
	}

	demo->reset_gpio = devm_gpiod_get_optional(dev, "reset",
						   GPIOD_OUT_LOW);
	if (IS_ERR(demo->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(demo->reset_gpio),
				     "reset GPIO\n");

	ret = of_property_read_u32(dev->of_node, "acme,sample-period-ns",
				   &demo->sample_period_ns);
	if (ret)
		demo->sample_period_ns = 1000;

	demo->use_dma = of_property_read_bool(dev->of_node, "acme,use-dma");

	ret = devm_request_irq(dev, demo->irq, dt_api_demo_irq, 0,
			       dev_name(dev), demo);
	if (ret)
		return dev_err_probe(dev, ret, "request IRQ %d\n", demo->irq);

	dev_info(dev,
		 "probe: variant=%s fifo=%u irq=%d sample=%u ns dma=%s\n",
		 demo->variant->name, demo->variant->fifo_depth, demo->irq,
		 demo->sample_period_ns, demo->use_dma ? "yes" : "no");

	return 0;
}

static void dt_api_demo_remove(struct platform_device *pdev)
{
	struct dt_api_demo *demo = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "remove: variant=%s\n", demo->variant->name);
}

static const struct of_device_id dt_api_demo_of_match[] = {
	{ .compatible = "acme,dt-api-demo-v1", .data = &dt_api_demo_v1 },
	{ .compatible = "acme,dt-api-demo-v2", .data = &dt_api_demo_v2 },
	{ }
};
MODULE_DEVICE_TABLE(of, dt_api_demo_of_match);

static struct platform_driver dt_api_demo_driver = {
	.probe = dt_api_demo_probe,
	.remove_new = dt_api_demo_remove,
	.driver = {
		.name = "dt-api-demo",
		.of_match_table = dt_api_demo_of_match,
	},
};
module_platform_driver(dt_api_demo_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Learning Path 11");
MODULE_DESCRIPTION("Learning-only Device Tree API integration demo");
