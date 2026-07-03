// SPDX-License-Identifier: GPL-2.0
/*
 * Learning-only ASoC codec/component skeleton.
 *
 * This module demonstrates:
 *   - I2C codec-style bus probe
 *   - devm regmap initialization
 *   - ASoC component registration
 *   - one DAI with playback/capture capabilities
 *   - DAI ops for format, sysclk, hw_params, and mute
 *   - ALSA controls and DAPM widgets/routes
 *
 * It is not a production codec driver and does not create a sound card by
 * itself. A real board still needs a machine driver or simple-card style
 * binding that links a CPU DAI to this codec DAI.
 */

#include <linux/bits.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>

#define DEMO_REG_POWER			0x00
#define DEMO_REG_FORMAT			0x01
#define DEMO_REG_VOLUME			0x02
#define DEMO_REG_MIXER			0x03
#define DEMO_REG_MAX			0x03

#define DEMO_POWER_DAC_EN		BIT(0)
#define DEMO_POWER_ADC_EN		BIT(1)
#define DEMO_POWER_MUTE			BIT(7)
#define DEMO_MIXER_DAC_TO_HP		BIT(0)

struct demo_asoc_codec {
	struct regmap *regmap;
	unsigned int sysclk;
	unsigned int dai_fmt;
};

static bool demo_readable_reg(struct device *dev, unsigned int reg)
{
	switch (reg) {
	case DEMO_REG_POWER:
	case DEMO_REG_FORMAT:
	case DEMO_REG_VOLUME:
	case DEMO_REG_MIXER:
		return true;
	default:
		return false;
	}
}

static bool demo_writeable_reg(struct device *dev, unsigned int reg)
{
	return demo_readable_reg(dev, reg);
}

static const struct reg_default demo_reg_defaults[] = {
	{ DEMO_REG_POWER, 0x00 },
	{ DEMO_REG_FORMAT, 0x00 },
	{ DEMO_REG_VOLUME, 0x40 },
	{ DEMO_REG_MIXER, 0x00 },
};

static const struct regmap_config demo_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = DEMO_REG_MAX,
	.readable_reg = demo_readable_reg,
	.writeable_reg = demo_writeable_reg,
	.reg_defaults = demo_reg_defaults,
	.num_reg_defaults = ARRAY_SIZE(demo_reg_defaults),
	.cache_type = REGCACHE_RBTREE,
};

static int demo_set_sysclk(struct snd_soc_dai *dai, int clk_id,
			   unsigned int freq, int dir)
{
	struct snd_soc_component *component = dai->component;
	struct demo_asoc_codec *priv = snd_soc_component_get_drvdata(component);

	priv->sysclk = freq;
	dev_dbg(component->dev, "set_sysclk: clk_id=%d freq=%u dir=%d\n",
		clk_id, freq, dir);

	return 0;
}

static int demo_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_component *component = dai->component;
	struct demo_asoc_codec *priv = snd_soc_component_get_drvdata(component);
	unsigned int format = fmt & SND_SOC_DAIFMT_FORMAT_MASK;

	switch (format) {
	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_LEFT_J:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_DSP_A:
	case SND_SOC_DAIFMT_DSP_B:
		priv->dai_fmt = fmt;
		return 0;
	default:
		dev_err(component->dev, "unsupported DAI format 0x%x\n", fmt);
		return -EINVAL;
	}
}

static int demo_hw_params(struct snd_pcm_substream *substream,
			  struct snd_pcm_hw_params *params,
			  struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	unsigned int rate = params_rate(params);
	unsigned int fmt_code;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		fmt_code = 0x0;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		fmt_code = 0x1;
		break;
	default:
		return -EINVAL;
	}

	dev_dbg(component->dev, "hw_params: rate=%u channels=%u format=%d\n",
		rate, params_channels(params), params_format(params));

	return snd_soc_component_update_bits(component, DEMO_REG_FORMAT,
					     GENMASK(1, 0), fmt_code);
}

static int demo_mute_stream(struct snd_soc_dai *dai, int mute, int stream)
{
	struct snd_soc_component *component = dai->component;

	return snd_soc_component_update_bits(component, DEMO_REG_POWER,
					     DEMO_POWER_MUTE,
					     mute ? DEMO_POWER_MUTE : 0);
}

static const struct snd_soc_dai_ops demo_dai_ops = {
	.set_sysclk = demo_set_sysclk,
	.set_fmt = demo_set_fmt,
	.hw_params = demo_hw_params,
	.mute_stream = demo_mute_stream,
};

static struct snd_soc_dai_driver demo_dai = {
	.name = "demo-asoc-hifi",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_96000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE |
			   SNDRV_PCM_FMTBIT_S24_LE,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = SNDRV_PCM_RATE_8000_48000,
		.formats = SNDRV_PCM_FMTBIT_S16_LE,
	},
	.ops = &demo_dai_ops,
	.symmetric_rate = 1,
};

static const DECLARE_TLV_DB_SCALE(demo_volume_tlv, -6350, 50, 0);

static const struct snd_kcontrol_new demo_controls[] = {
	SOC_SINGLE_TLV("Playback Volume", DEMO_REG_VOLUME, 0, 127, 0,
		       demo_volume_tlv),
};

static const struct snd_kcontrol_new demo_output_mixer_controls[] = {
	SOC_DAPM_SINGLE("DAC Switch", DEMO_REG_MIXER, 0, 1, 0),
};

static const struct snd_soc_dapm_widget demo_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_IN("AIFIN", "Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC("DAC", "Playback", DEMO_REG_POWER, 0, 0),
	SND_SOC_DAPM_MIXER("Output Mixer", SND_SOC_NOPM, 0, 0,
			   demo_output_mixer_controls,
			   ARRAY_SIZE(demo_output_mixer_controls)),
	SND_SOC_DAPM_OUTPUT("HP_OUT"),

	SND_SOC_DAPM_INPUT("MIC_IN"),
	SND_SOC_DAPM_ADC("ADC", "Capture", DEMO_REG_POWER, 1, 0),
	SND_SOC_DAPM_AIF_OUT("AIFOUT", "Capture", 0, SND_SOC_NOPM, 0, 0),
};

static const struct snd_soc_dapm_route demo_dapm_routes[] = {
	{ "DAC", NULL, "AIFIN" },
	{ "Output Mixer", "DAC Switch", "DAC" },
	{ "HP_OUT", NULL, "Output Mixer" },

	{ "ADC", NULL, "MIC_IN" },
	{ "AIFOUT", NULL, "ADC" },
};

static int demo_component_probe(struct snd_soc_component *component)
{
	dev_info(component->dev, "ASoC component bound to a sound card\n");
	return 0;
}

static const struct snd_soc_component_driver demo_component_driver = {
	.probe = demo_component_probe,
	.controls = demo_controls,
	.num_controls = ARRAY_SIZE(demo_controls),
	.dapm_widgets = demo_dapm_widgets,
	.num_dapm_widgets = ARRAY_SIZE(demo_dapm_widgets),
	.dapm_routes = demo_dapm_routes,
	.num_dapm_routes = ARRAY_SIZE(demo_dapm_routes),
};

static int demo_i2c_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct demo_asoc_codec *priv;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->regmap = devm_regmap_init_i2c(client, &demo_regmap_config);
	if (IS_ERR(priv->regmap))
		return dev_err_probe(dev, PTR_ERR(priv->regmap),
				     "failed to initialize regmap\n");

	i2c_set_clientdata(client, priv);

	dev_info(dev, "registered learning-only ASoC codec component\n");

	return devm_snd_soc_register_component(dev, &demo_component_driver,
					       &demo_dai, 1);
}

static const struct i2c_device_id demo_i2c_ids[] = {
	{ "demo_asoc_codec" },
	{ }
};
MODULE_DEVICE_TABLE(i2c, demo_i2c_ids);

static const struct of_device_id demo_of_match[] = {
	{ .compatible = "ldd,demo-asoc-codec" },
	{ }
};
MODULE_DEVICE_TABLE(of, demo_of_match);

static struct i2c_driver demo_i2c_driver = {
	.driver = {
		.name = "demo_asoc_codec",
		.of_match_table = demo_of_match,
	},
	.probe = demo_i2c_probe,
	.id_table = demo_i2c_ids,
};
module_i2c_driver(demo_i2c_driver);

MODULE_DESCRIPTION("Learning-only ASoC codec/component DAI and DAPM example");
MODULE_AUTHOR("Linux Device Driver learning project");
MODULE_LICENSE("GPL");
