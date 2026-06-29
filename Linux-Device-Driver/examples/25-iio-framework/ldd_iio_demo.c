// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only Industrial I/O platform demo.
 *
 * This module creates a synthetic platform device so learners can inspect IIO
 * sysfs attributes and, when a trigger is available, IIO buffered samples.
 * It does not drive real ADC hardware.
 */

#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/iio/buffer.h>
#include <linux/iio/iio.h>
#include <linux/iio/trigger_consumer.h>
#include <linux/iio/triggered_buffer.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/types.h>

#define LDD_IIO_DEMO_NAME	"ldd-iio-demo"
#define LDD_IIO_DEMO_MAX_RAW	4095

struct ldd_iio_demo {
	struct mutex lock;
	u16 sample_counter;
};

struct ldd_iio_demo_scan {
	__le16 channels[3];
	s64 timestamp __aligned(8);
};

#define LDD_IIO_VOLTAGE_CHANNEL(_idx) {				\
	.type = IIO_VOLTAGE,					\
	.indexed = 1,						\
	.channel = (_idx),					\
	.address = (_idx),					\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),		\
	.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),	\
	.scan_index = (_idx),					\
	.scan_type = {						\
		.sign = 'u',					\
		.realbits = 12,					\
		.storagebits = 16,				\
		.shift = 0,					\
		.endianness = IIO_LE,				\
	},							\
}

static const struct iio_chan_spec ldd_iio_demo_channels[] = {
	LDD_IIO_VOLTAGE_CHANNEL(0),
	LDD_IIO_VOLTAGE_CHANNEL(1),
	LDD_IIO_VOLTAGE_CHANNEL(2),
	IIO_CHAN_SOFT_TIMESTAMP(3),
};

static u16 ldd_iio_demo_next_sample(struct ldd_iio_demo *demo,
				    unsigned int channel)
{
	/*
	 * Deterministic fake ADC pattern:
	 *   ch0 slowly increments, ch1 and ch2 are offset from it.
	 */
	demo->sample_counter = (demo->sample_counter + 17) & LDD_IIO_DEMO_MAX_RAW;
	return (demo->sample_counter + channel * 700) & LDD_IIO_DEMO_MAX_RAW;
}

static int ldd_iio_demo_read_raw(struct iio_dev *indio_dev,
				 const struct iio_chan_spec *chan,
				 int *val, int *val2, long mask)
{
	struct ldd_iio_demo *demo = iio_priv(indio_dev);
	int ret;

	mutex_lock(&demo->lock);

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		if (iio_buffer_enabled(indio_dev)) {
			ret = -EBUSY;
			break;
		}

		*val = ldd_iio_demo_next_sample(demo, chan->address);
		ret = IIO_VAL_INT;
		break;

	case IIO_CHAN_INFO_SCALE:
		/*
		 * Pretend the ADC has a 1.8 V reference and 12-bit output:
		 * 1.8 / 4096 = 0.000439453 V per count.
		 */
		*val = 0;
		*val2 = 439;
		ret = IIO_VAL_INT_PLUS_MICRO;
		break;

	default:
		ret = -EINVAL;
		break;
	}

	mutex_unlock(&demo->lock);
	return ret;
}

static const struct iio_info ldd_iio_demo_info = {
	.read_raw = ldd_iio_demo_read_raw,
};

static irqreturn_t ldd_iio_demo_trigger_handler(int irq, void *p)
{
	struct iio_poll_func *pf = p;
	struct iio_dev *indio_dev = pf->indio_dev;
	struct ldd_iio_demo *demo = iio_priv(indio_dev);
	struct ldd_iio_demo_scan scan = { };
	unsigned int bit;
	unsigned int i = 0;

	mutex_lock(&demo->lock);

	for_each_set_bit(bit, indio_dev->active_scan_mask,
			 indio_dev->masklength) {
		if (bit >= 3)
			continue;

		scan.channels[i++] = cpu_to_le16(ldd_iio_demo_next_sample(demo,
									  bit));
	}

	mutex_unlock(&demo->lock);

	iio_push_to_buffers_with_timestamp(indio_dev, &scan, pf->timestamp);
	iio_trigger_notify_done(indio_dev->trig);

	return IRQ_HANDLED;
}

static int ldd_iio_demo_probe(struct platform_device *pdev)
{
	struct iio_dev *indio_dev;
	struct ldd_iio_demo *demo;
	int ret;

	indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*demo));
	if (!indio_dev)
		return -ENOMEM;

	demo = iio_priv(indio_dev);
	mutex_init(&demo->lock);

	platform_set_drvdata(pdev, indio_dev);

	indio_dev->dev.parent = &pdev->dev;
	indio_dev->name = LDD_IIO_DEMO_NAME;
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->info = &ldd_iio_demo_info;
	indio_dev->channels = ldd_iio_demo_channels;
	indio_dev->num_channels = ARRAY_SIZE(ldd_iio_demo_channels);

	ret = devm_iio_triggered_buffer_setup(&pdev->dev, indio_dev,
					      iio_pollfunc_store_time,
					      ldd_iio_demo_trigger_handler,
					      NULL);
	if (ret)
		return dev_err_probe(&pdev->dev, ret,
				     "failed to set up triggered buffer\n");

	ret = devm_iio_device_register(&pdev->dev, indio_dev);
	if (ret)
		return dev_err_probe(&pdev->dev, ret,
				     "failed to register IIO device\n");

	dev_info(&pdev->dev, "registered learning-only IIO demo\n");
	return 0;
}

static int ldd_iio_demo_remove(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "removed learning-only IIO demo\n");
	return 0;
}

static struct platform_driver ldd_iio_demo_driver = {
	.probe = ldd_iio_demo_probe,
	.remove = ldd_iio_demo_remove,
	.driver = {
		.name = LDD_IIO_DEMO_NAME,
	},
};

static struct platform_device *ldd_iio_demo_pdev;

static int __init ldd_iio_demo_init(void)
{
	int ret;

	ret = platform_driver_register(&ldd_iio_demo_driver);
	if (ret)
		return ret;

	ldd_iio_demo_pdev = platform_device_register_simple(LDD_IIO_DEMO_NAME,
							    -1, NULL, 0);
	if (IS_ERR(ldd_iio_demo_pdev)) {
		ret = PTR_ERR(ldd_iio_demo_pdev);
		platform_driver_unregister(&ldd_iio_demo_driver);
		return ret;
	}

	return 0;
}

static void __exit ldd_iio_demo_exit(void)
{
	platform_device_unregister(ldd_iio_demo_pdev);
	platform_driver_unregister(&ldd_iio_demo_driver);
}

module_init(ldd_iio_demo_init);
module_exit(ldd_iio_demo_exit);

MODULE_AUTHOR("Linux Device Driver learning example");
MODULE_DESCRIPTION("Learning-only IIO direct and triggered-buffer demo");
MODULE_LICENSE("GPL");
