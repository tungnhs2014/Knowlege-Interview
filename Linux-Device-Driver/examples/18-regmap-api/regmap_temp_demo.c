// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only regmap-backed I2C temperature driver.
 *
 * Training register protocol:
 *   0x00: chip ID, expected 0x5a
 *   0x01: control register, bit 0 enables measurements
 *   0x02: signed 8-bit temperature in degrees Celsius
 *   0x03: clear-on-read event register, shown as precious policy
 */

#include <linux/bitfield.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/slab.h>

#define REGMAP_TEMP_REG_ID		0x00
#define REGMAP_TEMP_REG_CTRL		0x01
#define REGMAP_TEMP_REG_TEMP		0x02
#define REGMAP_TEMP_REG_EVENT		0x03
#define REGMAP_TEMP_REG_MAX		0x03

#define REGMAP_TEMP_ID_EXPECTED		0x5a
#define REGMAP_TEMP_CTRL_ENABLE		BIT(0)
#define REGMAP_TEMP_CTRL_RATE_MASK	GENMASK(2, 1)
#define REGMAP_TEMP_CTRL_RATE_1HZ	FIELD_PREP(REGMAP_TEMP_CTRL_RATE_MASK, 1)

struct regmap_temp_demo {
	struct device *dev;
	struct regmap *map;
};

static bool regmap_temp_readable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case REGMAP_TEMP_REG_ID:
	case REGMAP_TEMP_REG_CTRL:
	case REGMAP_TEMP_REG_TEMP:
	case REGMAP_TEMP_REG_EVENT:
		return true;
	default:
		return false;
	}
}

static bool regmap_temp_writeable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case REGMAP_TEMP_REG_CTRL:
		return true;
	default:
		return false;
	}
}

static bool regmap_temp_volatile_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case REGMAP_TEMP_REG_TEMP:
	case REGMAP_TEMP_REG_EVENT:
		return true;
	default:
		return false;
	}
}

static bool regmap_temp_precious_reg(struct device *dev, unsigned int reg)
{
	return reg == REGMAP_TEMP_REG_EVENT;
}

static const struct reg_default regmap_temp_defaults[] = {
	{ REGMAP_TEMP_REG_CTRL, 0x00 },
};

static const struct regmap_config regmap_temp_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = REGMAP_TEMP_REG_MAX,
	.readable_reg = regmap_temp_readable_reg,
	.writeable_reg = regmap_temp_writeable_reg,
	.volatile_reg = regmap_temp_volatile_reg,
	.precious_reg = regmap_temp_precious_reg,
	.reg_defaults = regmap_temp_defaults,
	.num_reg_defaults = ARRAY_SIZE(regmap_temp_defaults),
	.cache_type = REGCACHE_RBTREE,
};

static int regmap_temp_read_temp(struct regmap_temp_demo *demo, long *mdegc)
{
	unsigned int val;
	int ret;

	ret = regmap_read(demo->map, REGMAP_TEMP_REG_TEMP, &val);
	if (ret)
		return ret;

	*mdegc = (s8)val * 1000;
	return 0;
}

static umode_t regmap_temp_is_visible(const void *drvdata,
				      enum hwmon_sensor_types type,
				      u32 attr, int channel)
{
	if (type == hwmon_temp && attr == hwmon_temp_input)
		return 0444;

	return 0;
}

static int regmap_temp_hwmon_read(struct device *dev,
				  enum hwmon_sensor_types type,
				  u32 attr, int channel, long *val)
{
	struct regmap_temp_demo *demo = dev_get_drvdata(dev);

	if (type != hwmon_temp || attr != hwmon_temp_input)
		return -EOPNOTSUPP;

	return regmap_temp_read_temp(demo, val);
}

static const struct hwmon_ops regmap_temp_hwmon_ops = {
	.is_visible = regmap_temp_is_visible,
	.read = regmap_temp_hwmon_read,
};

static const struct hwmon_channel_info * const regmap_temp_hwmon_info[] = {
	HWMON_CHANNEL_INFO(temp, HWMON_T_INPUT),
	NULL
};

static const struct hwmon_chip_info regmap_temp_chip_info = {
	.ops = &regmap_temp_hwmon_ops,
	.info = regmap_temp_hwmon_info,
};

static int regmap_temp_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct regmap_temp_demo *demo;
	struct device *hwmon;
	unsigned int id;
	long temp;
	int ret;

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_BYTE_DATA))
		return dev_err_probe(dev, -EOPNOTSUPP,
				     "adapter lacks SMBus byte-data support\n");

	demo = devm_kzalloc(dev, sizeof(*demo), GFP_KERNEL);
	if (!demo)
		return -ENOMEM;

	demo->dev = dev;
	demo->map = devm_regmap_init_i2c(client, &regmap_temp_config);
	if (IS_ERR(demo->map))
		return dev_err_probe(dev, PTR_ERR(demo->map),
				     "failed to initialize regmap\n");

	i2c_set_clientdata(client, demo);

	ret = regmap_read(demo->map, REGMAP_TEMP_REG_ID, &id);
	if (ret)
		return dev_err_probe(dev, ret, "failed to read chip id\n");

	if (id != REGMAP_TEMP_ID_EXPECTED)
		return dev_err_probe(dev, -ENODEV, "unexpected chip id 0x%02x\n",
				     id);

	ret = regmap_update_bits(demo->map, REGMAP_TEMP_REG_CTRL,
				 REGMAP_TEMP_CTRL_ENABLE |
				 REGMAP_TEMP_CTRL_RATE_MASK,
				 REGMAP_TEMP_CTRL_ENABLE |
				 REGMAP_TEMP_CTRL_RATE_1HZ);
	if (ret)
		return dev_err_probe(dev, ret, "failed to configure control register\n");

	ret = regmap_temp_read_temp(demo, &temp);
	if (ret)
		return dev_err_probe(dev, ret, "failed initial temperature read\n");

	hwmon = devm_hwmon_device_register_with_info(dev, "regmap_temp_demo",
						     demo,
						     &regmap_temp_chip_info,
						     NULL);
	if (IS_ERR(hwmon))
		return dev_err_probe(dev, PTR_ERR(hwmon),
				     "failed to register hwmon device\n");

	dev_info(dev, "registered regmap hwmon demo, temp=%ld mC\n", temp);

	return 0;
}

static void regmap_temp_remove(struct i2c_client *client)
{
	dev_info(&client->dev, "removed regmap hwmon demo\n");
}

static const struct of_device_id regmap_temp_of_match[] = {
	{ .compatible = "ldd,regmap-temp-demo" },
	{ }
};
MODULE_DEVICE_TABLE(of, regmap_temp_of_match);

static const struct i2c_device_id regmap_temp_id[] = {
	{ "regmap_temp_demo", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, regmap_temp_id);

static struct i2c_driver regmap_temp_driver = {
	.driver = {
		.name = "regmap_temp_demo",
		.of_match_table = regmap_temp_of_match,
	},
	.probe = regmap_temp_probe,
	.remove = regmap_temp_remove,
	.id_table = regmap_temp_id,
};
module_i2c_driver(regmap_temp_driver);

MODULE_AUTHOR("Linux Device Driver learning example");
MODULE_DESCRIPTION("Learning-only regmap-backed I2C hwmon demo");
MODULE_LICENSE("GPL");
