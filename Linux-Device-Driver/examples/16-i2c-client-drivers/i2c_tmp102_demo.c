// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only I2C client driver for a TMP102-like temperature register.
 *
 * The driver demonstrates I2C client matching, probe-time capability checks,
 * private data, SMBus word reads, endian/signed conversion, and registration
 * with the standard hwmon subsystem.
 */

#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/swab.h>

#define TMP102_DEMO_REG_TEMP	0x00

struct tmp102_demo {
	struct i2c_client *client;
	struct mutex lock;
};

static int tmp102_demo_read_temp(struct tmp102_demo *data, long *mdegc)
{
	s16 raw;
	s32 ret;

	mutex_lock(&data->lock);
	ret = i2c_smbus_read_word_data(data->client, TMP102_DEMO_REG_TEMP);
	mutex_unlock(&data->lock);
	if (ret < 0)
		return ret;

	/*
	 * TMP102-style temperature is a signed 12-bit value left-aligned in a
	 * big-endian 16-bit register. Each LSB after shifting is 0.0625 C.
	 */
	raw = (s16)swab16((u16)ret);
	raw >>= 4;
	*mdegc = raw * 625 / 10;

	return 0;
}

static umode_t tmp102_demo_is_visible(const void *drvdata,
				      enum hwmon_sensor_types type,
				      u32 attr, int channel)
{
	if (type == hwmon_temp && attr == hwmon_temp_input)
		return 0444;

	return 0;
}

static int tmp102_demo_hwmon_read(struct device *dev,
				  enum hwmon_sensor_types type,
				  u32 attr, int channel, long *val)
{
	struct tmp102_demo *data = dev_get_drvdata(dev);

	if (type != hwmon_temp || attr != hwmon_temp_input)
		return -EOPNOTSUPP;

	return tmp102_demo_read_temp(data, val);
}

static const struct hwmon_ops tmp102_demo_hwmon_ops = {
	.is_visible = tmp102_demo_is_visible,
	.read = tmp102_demo_hwmon_read,
};

static const struct hwmon_channel_info * const tmp102_demo_hwmon_info[] = {
	HWMON_CHANNEL_INFO(temp, HWMON_T_INPUT),
	NULL
};

static const struct hwmon_chip_info tmp102_demo_chip_info = {
	.ops = &tmp102_demo_hwmon_ops,
	.info = tmp102_demo_hwmon_info,
};

static int tmp102_demo_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct tmp102_demo *data;
	struct device *hwmon;
	long temp;
	int ret;

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_WORD_DATA))
		return dev_err_probe(dev, -EOPNOTSUPP,
				     "adapter lacks SMBus word-data support\n");

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->client = client;
	mutex_init(&data->lock);
	i2c_set_clientdata(client, data);

	ret = tmp102_demo_read_temp(data, &temp);
	if (ret)
		return dev_err_probe(dev, ret, "initial temperature read\n");

	hwmon = devm_hwmon_device_register_with_info(dev, "i2c_tmp102_demo",
						     data,
						     &tmp102_demo_chip_info,
						     NULL);
	if (IS_ERR(hwmon))
		return dev_err_probe(dev, PTR_ERR(hwmon),
				     "register hwmon device\n");

	dev_info(dev, "registered hwmon temperature sensor, temp=%ld mC\n",
		 temp);

	return 0;
}

static void tmp102_demo_remove(struct i2c_client *client)
{
	dev_info(&client->dev, "removed hwmon temperature sensor\n");
}

static const struct of_device_id tmp102_demo_of_match[] = {
	{ .compatible = "ldd,tmp102-demo" },
	{ }
};
MODULE_DEVICE_TABLE(of, tmp102_demo_of_match);

static const struct i2c_device_id tmp102_demo_id[] = {
	{ "i2c_tmp102_demo", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tmp102_demo_id);

static struct i2c_driver tmp102_demo_driver = {
	.driver = {
		.name = "i2c_tmp102_demo",
		.of_match_table = tmp102_demo_of_match,
	},
	.probe = tmp102_demo_probe,
	.remove = tmp102_demo_remove,
	.id_table = tmp102_demo_id,
};
module_i2c_driver(tmp102_demo_driver);

MODULE_AUTHOR("Linux Device Driver learning example");
MODULE_DESCRIPTION("Learning-only TMP102-like I2C hwmon client driver");
MODULE_LICENSE("GPL");
