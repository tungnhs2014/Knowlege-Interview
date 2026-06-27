// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only SPI device driver for a tiny register-based temperature
 * sensor model.
 *
 * The driver demonstrates SPI device matching, probe-time bus setup,
 * per-device private data, synchronous command-then-read transfers, locking,
 * and registration with the standard hwmon subsystem.
 */

#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/spi/spi.h>

#define SPI_TEMP_DEMO_REG_ID		0x00
#define SPI_TEMP_DEMO_REG_TEMP		0x01
#define SPI_TEMP_DEMO_EXPECTED_ID	0x5a
#define SPI_TEMP_DEMO_READ_CMD		0x80

struct spi_temp_demo {
	struct spi_device *spi;
	struct mutex lock;
	u8 tx_buf[2];
	u8 rx_buf[2];
};

static int spi_temp_demo_read_reg(struct spi_temp_demo *data, u8 reg, u8 *val)
{
	int ret;

	mutex_lock(&data->lock);

	data->tx_buf[0] = SPI_TEMP_DEMO_READ_CMD | reg;
	data->rx_buf[0] = 0;

	ret = spi_write_then_read(data->spi, data->tx_buf, 1,
				  data->rx_buf, 1);
	if (!ret)
		*val = data->rx_buf[0];

	mutex_unlock(&data->lock);

	return ret;
}

static int spi_temp_demo_read_temp(struct spi_temp_demo *data, long *mdegc)
{
	u8 raw;
	int ret;

	ret = spi_temp_demo_read_reg(data, SPI_TEMP_DEMO_REG_TEMP, &raw);
	if (ret)
		return ret;

	*mdegc = (long)(s8)raw * 1000;

	return 0;
}

static umode_t spi_temp_demo_is_visible(const void *drvdata,
					enum hwmon_sensor_types type,
					u32 attr, int channel)
{
	if (type == hwmon_temp && attr == hwmon_temp_input)
		return 0444;

	return 0;
}

static int spi_temp_demo_hwmon_read(struct device *dev,
				    enum hwmon_sensor_types type,
				    u32 attr, int channel, long *val)
{
	struct spi_temp_demo *data = dev_get_drvdata(dev);

	if (type != hwmon_temp || attr != hwmon_temp_input)
		return -EOPNOTSUPP;

	return spi_temp_demo_read_temp(data, val);
}

static const struct hwmon_ops spi_temp_demo_hwmon_ops = {
	.is_visible = spi_temp_demo_is_visible,
	.read = spi_temp_demo_hwmon_read,
};

static const struct hwmon_channel_info * const spi_temp_demo_hwmon_info[] = {
	HWMON_CHANNEL_INFO(temp, HWMON_T_INPUT),
	NULL
};

static const struct hwmon_chip_info spi_temp_demo_chip_info = {
	.ops = &spi_temp_demo_hwmon_ops,
	.info = spi_temp_demo_hwmon_info,
};

static int spi_temp_demo_probe(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct spi_temp_demo *data;
	struct device *hwmon;
	long temp;
	u8 id;
	int ret;

	spi->mode &= ~(SPI_CPOL | SPI_CPHA);
	spi->bits_per_word = 8;

	if (!spi->max_speed_hz)
		spi->max_speed_hz = 1000000;

	ret = spi_setup(spi);
	if (ret)
		return dev_err_probe(dev, ret, "spi_setup\n");

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->spi = spi;
	mutex_init(&data->lock);
	spi_set_drvdata(spi, data);

	ret = spi_temp_demo_read_reg(data, SPI_TEMP_DEMO_REG_ID, &id);
	if (ret)
		return dev_err_probe(dev, ret, "read chip id\n");

	if (id != SPI_TEMP_DEMO_EXPECTED_ID)
		return dev_err_probe(dev, -ENODEV,
				     "unexpected chip id 0x%02x\n", id);

	ret = spi_temp_demo_read_temp(data, &temp);
	if (ret)
		return dev_err_probe(dev, ret, "initial temperature read\n");

	hwmon = devm_hwmon_device_register_with_info(dev, "spi_temp_demo",
						     data,
						     &spi_temp_demo_chip_info,
						     NULL);
	if (IS_ERR(hwmon))
		return dev_err_probe(dev, PTR_ERR(hwmon),
				     "register hwmon device\n");

	dev_info(dev, "registered hwmon temperature sensor, temp=%ld mC\n",
		 temp);

	return 0;
}

static void spi_temp_demo_remove(struct spi_device *spi)
{
	dev_info(&spi->dev, "removed hwmon temperature sensor\n");
}

static const struct of_device_id spi_temp_demo_of_match[] = {
	{ .compatible = "ldd,spi-temp-demo" },
	{ }
};
MODULE_DEVICE_TABLE(of, spi_temp_demo_of_match);

static const struct spi_device_id spi_temp_demo_id[] = {
	{ "spi_temp_demo", 0 },
	{ }
};
MODULE_DEVICE_TABLE(spi, spi_temp_demo_id);

static struct spi_driver spi_temp_demo_driver = {
	.driver = {
		.name = "spi_temp_demo",
		.of_match_table = spi_temp_demo_of_match,
	},
	.probe = spi_temp_demo_probe,
	.remove = spi_temp_demo_remove,
	.id_table = spi_temp_demo_id,
};
module_spi_driver(spi_temp_demo_driver);

MODULE_AUTHOR("Linux Device Driver learning example");
MODULE_DESCRIPTION("Learning-only SPI hwmon device driver");
MODULE_LICENSE("GPL");
