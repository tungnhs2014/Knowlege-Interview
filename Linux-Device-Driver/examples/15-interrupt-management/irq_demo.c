// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only interrupt management demo.
 *
 * This is not a production hardware driver. It demonstrates a platform
 * driver's IRQ request path, hard IRQ context, threaded IRQ deferral,
 * workqueue deferral, and remove-time cleanup.
 */

#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>

#define IRQ_DEMO_STATUS		0x00
#define IRQ_DEMO_CLEAR		0x04
#define IRQ_DEMO_ENABLE		0x08
#define IRQ_DEMO_DISABLE	0x0c

struct irq_demo {
	struct device *dev;
	void __iomem *base;
	int irq;
	bool use_threaded;
	spinlock_t lock;
	struct work_struct work;
	u32 pending;
};

static bool use_threaded = true;
module_param(use_threaded, bool, 0644);
MODULE_PARM_DESC(use_threaded,
		 "Use request_threaded_irq() path instead of workqueue deferral");

static void irq_demo_hw_disable(struct irq_demo *demo)
{
	writel(0xffffffff, demo->base + IRQ_DEMO_DISABLE);
}

static void irq_demo_hw_enable(struct irq_demo *demo)
{
	writel(0xffffffff, demo->base + IRQ_DEMO_ENABLE);
}

static void irq_demo_process_status(struct irq_demo *demo, u32 status,
				    const char *where)
{
	/*
	 * This stands in for slower processing such as parsing a FIFO,
	 * notifying a framework, or reading more registers in sleepable context.
	 */
	dev_info(demo->dev, "%s handled status=0x%08x\n", where, status);
}

static void irq_demo_work(struct work_struct *work)
{
	struct irq_demo *demo = container_of(work, struct irq_demo, work);
	unsigned long flags;
	u32 status;

	spin_lock_irqsave(&demo->lock, flags);
	status = demo->pending;
	demo->pending = 0;
	spin_unlock_irqrestore(&demo->lock, flags);

	if (status)
		irq_demo_process_status(demo, status, "workqueue");
}

static irqreturn_t irq_demo_hardirq(int irq, void *dev_id)
{
	struct irq_demo *demo = dev_id;
	u32 status;

	status = readl(demo->base + IRQ_DEMO_STATUS);
	if (!status)
		return IRQ_NONE;

	/*
	 * This demo assumes write-one-to-clear status bits.
	 * Real drivers must follow the device datasheet exactly.
	 */
	writel(status, demo->base + IRQ_DEMO_CLEAR);

	spin_lock(&demo->lock);
	demo->pending |= status;
	spin_unlock(&demo->lock);

	if (demo->use_threaded)
		return IRQ_WAKE_THREAD;

	schedule_work(&demo->work);
	return IRQ_HANDLED;
}

static irqreturn_t irq_demo_thread(int irq, void *dev_id)
{
	struct irq_demo *demo = dev_id;
	unsigned long flags;
	u32 status;

	spin_lock_irqsave(&demo->lock, flags);
	status = demo->pending;
	demo->pending = 0;
	spin_unlock_irqrestore(&demo->lock, flags);

	if (status)
		irq_demo_process_status(demo, status, "threaded IRQ");

	return IRQ_HANDLED;
}

static int irq_demo_probe(struct platform_device *pdev)
{
	struct irq_demo *demo;
	int ret;

	demo = devm_kzalloc(&pdev->dev, sizeof(*demo), GFP_KERNEL);
	if (!demo)
		return -ENOMEM;

	demo->dev = &pdev->dev;
	demo->use_threaded = use_threaded;
	spin_lock_init(&demo->lock);
	INIT_WORK(&demo->work, irq_demo_work);

	demo->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(demo->base))
		return PTR_ERR(demo->base);

	demo->irq = platform_get_irq(pdev, 0);
	if (demo->irq < 0)
		return demo->irq;

	platform_set_drvdata(pdev, demo);

	if (demo->use_threaded) {
		ret = devm_request_threaded_irq(&pdev->dev, demo->irq,
						irq_demo_hardirq,
						irq_demo_thread,
						IRQF_ONESHOT,
						dev_name(&pdev->dev), demo);
	} else {
		ret = devm_request_irq(&pdev->dev, demo->irq,
				       irq_demo_hardirq, 0,
				       dev_name(&pdev->dev), demo);
	}

	if (ret)
		return dev_err_probe(&pdev->dev, ret,
				     "failed to request IRQ %d\n", demo->irq);

	irq_demo_hw_enable(demo);
	dev_info(&pdev->dev, "registered IRQ %d using %s deferral\n",
		 demo->irq, demo->use_threaded ? "threaded IRQ" : "workqueue");

	return 0;
}

static void irq_demo_remove(struct platform_device *pdev)
{
	struct irq_demo *demo = platform_get_drvdata(pdev);

	irq_demo_hw_disable(demo);
	cancel_work_sync(&demo->work);
	dev_info(&pdev->dev, "removed IRQ demo\n");
}

static const struct of_device_id irq_demo_of_match[] = {
	{ .compatible = "training,irq-demo" },
	{ }
};
MODULE_DEVICE_TABLE(of, irq_demo_of_match);

static struct platform_driver irq_demo_driver = {
	.probe = irq_demo_probe,
	.remove_new = irq_demo_remove,
	.driver = {
		.name = "irq-demo",
		.of_match_table = irq_demo_of_match,
	},
};
module_platform_driver(irq_demo_driver);

MODULE_AUTHOR("TungNHS Linux Device Driver Learning");
MODULE_DESCRIPTION("Learning-only interrupt management demo driver");
MODULE_LICENSE("GPL");
