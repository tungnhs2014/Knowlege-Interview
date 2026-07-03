# 35 - ASoC Codec, Component, DAI, And DAPM

## Learning Goal

After this chapter, you should understand how a reusable ASoC codec/component driver describes audio hardware to the Linux sound subsystem.

By the end, you should be able to:

- Explain what ASoC is and why embedded audio needs it.
- Distinguish codec/component, CPU DAI, platform/PCM DMA, and machine-driver roles.
- Describe what a codec/component driver registers with the ASoC core.
- Define DAI capabilities with `struct snd_soc_dai_driver` and `struct snd_soc_pcm_stream`.
- Explain how DAI ops configure clocks, formats, TDM slots, stream startup, and mute.
- Expose ALSA controls with `SOC_*` macros and volume TLV metadata.
- Build a DAPM graph from widgets and routes.
- Debug common "codec probes but no audio" failures.
- Recognize kernel-version-sensitive ASoC API names.

## Why This Matters In Real Work

Audio bring-up often fails in quiet ways: the I2C codec probes, `aplay` appears to run, but there is no sound; a mixer switch seems to change, but the amplifier never powers; idle current is too high; or the sample clock is wrong. ASoC is where those problems meet.

Common real work:

| Situation | What you need to understand |
| --- | --- |
| Port an existing codec to a new board | Keep codec driver reusable; move board wiring to machine/simple-card |
| Add a new audio codec driver | Regmap, component registration, DAI capabilities, controls, DAPM |
| Debug silent playback | DAI format, clocks, routes, DAPM power state, codec registers |
| Reduce audio idle power | DAPM widgets, routes, supplies, clocks, bias states |
| Expose mixer controls | ALSA control naming, `SOC_*` macros, TLV dB scales |
| Prepare for upstream review | Stable controls, no board policy in codec driver, correct DAPM graph |

**Production rule:** a codec driver should describe reusable hardware behavior. Board connectors, jack GPIOs, amplifier GPIOs, final routes, and CPU-to-codec binding normally belong to the machine layer or Device Tree simple-card description.

## Mental Model

Think of ASoC as a way to assemble a sound card from reusable pieces.

```text
userspace
  aplay / arecord / amixer / alsamixer
        |
        v
ALSA PCM + control core
        |
        v
ASoC card
  machine driver or simple-card
        |
        +-- CPU DAI / platform component
        |     I2S, SAI, AC97, PCM, S/PDIF, DMA engine
        |
        +-- codec/component driver
              DAI capabilities, codec registers, controls,
              DAC/ADC/mixers/muxes/PGA, DAPM graph
```

The codec driver is not "the whole sound card." It is one reusable component. It says:

- "These are my playback and capture formats."
- "These are my DAI callbacks for format, clocks, and stream setup."
- "These are my ALSA mixer controls."
- "These widgets and routes describe my internal audio paths."
- "Use these register operations to control me."

The machine driver or simple-card says:

- "This CPU DAI is wired to this codec DAI."
- "These codec pins go to this headphone jack, microphone, speaker, or amplifier."
- "This side is bit-clock/frame-clock master."
- "These are the board-specific clock and route choices."

## Core Concepts

ASoC uses old vocabulary and modern vocabulary side by side, so it helps to anchor the terms.

| Term | Meaning |
| --- | --- |
| ALSA | The broader Linux sound subsystem: PCM devices, controls, cards, userspace ABI. |
| ASoC | ALSA System on Chip layer for embedded/SoC audio integration. |
| Component | Modern reusable ASoC driver object. A codec, CPU DAI block, DSP, amplifier, or platform PCM block can be a component. |
| Codec | Audio component that converts, routes, mixes, amplifies, or processes audio. Often controlled over I2C/SPI/regmap. |
| DAI | Digital Audio Interface. The digital audio link endpoint, such as I2S, PCM, AC97, S/PDIF, PDM, or TDM-style interface. |
| CPU DAI | The SoC-side audio interface controller. |
| Codec DAI | The codec-side audio interface endpoint. |
| PCM DMA/platform component | The part that moves PCM samples between memory and the CPU DAI FIFO, often through DMAengine. |
| Machine driver | Board-specific glue that binds CPU DAI, codec DAI, platform component, clocks, and routes into a sound card. |
| DAPM | Dynamic Audio Power Management: graph-based audio path and power management. |
| kcontrol | ALSA control visible to userspace tools such as `amixer` and `alsamixer`. |

### ASoC Split

| Layer | Reusable? | Owns |
| --- | --- | --- |
| Codec/component driver | Usually yes | Codec registers, controls, DAI capabilities, DAPM widgets/routes inside the component |
| CPU DAI/platform driver | Usually yes | SoC audio controller, FIFO, DMA/PCM behavior, supported bus formats |
| Machine driver/simple-card | Board-specific | DAI links, connector routes, clock master/slave policy, amp/jack GPIOs, final card registration |

**Interview trap:** "platform driver" in ASoC does not always mean the generic Linux platform bus topic. In ASoC, "platform" often means the PCM DMA/CPU-side audio component role.

## Kernel Mechanism

ASoC registers reusable components first, then a card binds them together later.

```text
I2C/SPI/platform bus probe
  allocate private data
  initialize regmap, reset GPIOs, regulators, clocks
  devm_snd_soc_register_component()
        |
        v
ASoC component and DAI are now visible to ASoC core

machine/simple-card probe
  create snd_soc_card and DAI links
  bind CPU DAI <-> codec DAI <-> platform component
  devm_snd_soc_register_card()
        |
        v
ASoC instantiates card
  component probe callbacks
  DAI probe callbacks
  PCM devices created
  controls registered
  DAPM widgets/routes built
```

Important ownership and lifetime points:

- The bus probe creates hardware access state, such as `struct regmap`.
- `devm_snd_soc_register_component()` registers the ASoC view of that device.
- A component may be registered even before a sound card exists.
- The ASoC component `probe()` is tied to card/component binding, not necessarily the same moment as the I2C/SPI probe.
- Userspace normally sees ALSA card, PCM, and control interfaces, not a private codec `/dev` node.
- A codec driver must not assume its routes are complete; machine routes may connect codec pins to board endpoints.

### DAPM Power Graph

DAPM models audio hardware as a graph:

```text
source widget --route--> mixer/mux/PGA --route--> DAC/ADC/AIF --route--> sink widget
```

Examples:

```text
"Headphone Jack" <- "HP_OUT" <- "DAC" <- "AIF Playback"
"ADC" <- "Input Mixer" <- "MIC_IN" <- "Mic Jack"
```

Route entries are written as:

```c
{ "Sink Widget", "Control Name or NULL", "Source Widget" }
```

That order matters. It follows signal flow from source to sink, but the struct fields are sink/control/source.

DAPM power decisions use:

- Active playback/capture streams.
- ALSA mixer/mux settings.
- Widget routes and event callbacks.
- Supply, regulator, clock, and bias widgets.

**Production rule:** if a path affects power or signal routing, model it with DAPM widgets/routes and `SOC_DAPM_*` controls instead of plain `SOC_*` controls.

## Key Structs And APIs

These APIs matter because they map directly to what a codec/component driver tells ASoC.

### Component Driver

`struct snd_soc_component_driver` describes the reusable component.

Common fields:

| Field | Purpose |
| --- | --- |
| `name` | Component driver name. |
| `controls`, `num_controls` | Static ALSA controls. |
| `dapm_widgets`, `num_dapm_widgets` | Static DAPM widgets. |
| `dapm_routes`, `num_dapm_routes` | Static DAPM routes. |
| `probe`, `remove` | Component-level setup/cleanup when bound into a card. |
| `suspend`, `resume` | Power-management callbacks. |
| `read`, `write` | Optional component register I/O hooks. Many drivers rely on regmap-backed helpers instead. |
| `set_sysclk`, `set_pll`, `set_jack` | Component-wide clock or jack integration callbacks. |

Registration:

```c
devm_snd_soc_register_component(dev, &component_driver,
                                dai_drivers, num_dais);
```

Component register helpers:

```c
snd_soc_component_read(component, reg);
snd_soc_component_write(component, reg, val);
snd_soc_component_update_bits(component, reg, mask, val);
snd_soc_component_test_bits(component, reg, mask, val);
```

### DAI Driver

`struct snd_soc_dai_driver` describes a digital audio interface exposed by the component.

Important fields:

| Field | Purpose |
| --- | --- |
| `name` | DAI name matched by the card/machine layer. |
| `ops` | DAI callbacks for clocks, format, TDM, stream lifecycle, mute. |
| `playback` | Playback stream capability. |
| `capture` | Capture stream capability. |
| `symmetric_rate`, `symmetric_channels`, `symmetric_sample_bits` | Constraints when playback and capture must match. |

`struct snd_soc_pcm_stream` describes stream capability:

```c
static struct snd_soc_dai_driver demo_dai = {
        .name = "demo-hifi",
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
};
```

### DAI Ops

`struct snd_soc_dai_ops` callbacks are called by ASoC while setting up and running streams.

| Callback group | Examples | Purpose |
| --- | --- | --- |
| Clocking | `set_sysclk`, `set_pll`, `set_clkdiv`, `set_bclk_ratio` | Select and program audio clocks. |
| Format | `set_fmt`, `set_tdm_slot`, `set_channel_map`, `set_tristate` | Configure I2S/PCM/TDM format, polarity, master/slave, slots. |
| Stream lifecycle | `startup`, `shutdown`, `hw_params`, `hw_free`, `prepare`, `trigger` | Validate stream parameters and program hardware at the right stage. |
| Mute/delay | `mute_stream`, `delay` | Reduce pops/clicks and report FIFO/hardware delay. |

Machine/card code commonly uses helpers:

```c
snd_soc_dai_set_fmt(dai, fmt);
snd_soc_dai_set_sysclk(dai, clk_id, freq, dir);
snd_soc_dai_set_pll(dai, pll_id, source, freq_in, freq_out);
snd_soc_dai_set_clkdiv(dai, div_id, div);
snd_soc_dai_set_tdm_slot(dai, tx_mask, rx_mask, slots, slot_width);
```

Those helpers call the underlying DAI driver's ops.

### Controls

ASoC controls become ALSA controls visible to userspace.

Common macros:

| Macro | Use |
| --- | --- |
| `SOC_SINGLE` | Single register-backed switch/value. |
| `SOC_SINGLE_TLV` | Single value with dB scale metadata. |
| `SOC_DOUBLE_R` | Stereo control stored in two registers. |
| `SOC_DOUBLE_R_TLV` | Stereo volume with dB scale metadata. |
| `SOC_ENUM` / `SOC_ENUM_SINGLE` | Enumerated mux-like choices. |
| `DECLARE_TLV_DB_SCALE` | Defines dB scale metadata for volume controls. |

Example:

```c
static const DECLARE_TLV_DB_SCALE(out_tlv, -6350, 50, 0);

static const struct snd_kcontrol_new demo_controls[] = {
        SOC_SINGLE_TLV("Playback Volume", DEMO_VOL, 0, 127, 0, out_tlv),
        SOC_SINGLE("Playback Switch", DEMO_MUTE, 0, 1, 1),
};
```

Control naming is part of the userspace ABI. Names such as `Playback Volume`, `Capture Switch`, `Mic`, `Line`, `PCM`, and `Master` should follow ALSA conventions.

### DAPM Widgets And Routes

DAPM widgets describe audio blocks and endpoints.

Common widget macros:

| Widget | Example use |
| --- | --- |
| `SND_SOC_DAPM_INPUT` / `OUTPUT` | Codec pins. |
| `SND_SOC_DAPM_MIC`, `HP`, `SPK`, `LINE` | Board or endpoint widgets. |
| `SND_SOC_DAPM_MIXER`, `MUX`, `PGA` | Internal analog path blocks. |
| `SND_SOC_DAPM_DAC`, `ADC` | Stream-powered converters. |
| `SND_SOC_DAPM_AIF_IN`, `AIF_OUT` | Audio interface stream widgets. |
| `SND_SOC_DAPM_SUPPLY` | Internal supply or bias enable. |
| `SND_SOC_DAPM_REGULATOR_SUPPLY` | External regulator modeled in DAPM. |
| `SND_SOC_DAPM_CLOCK_SUPPLY` | Clock supply modeled in DAPM. |

DAPM controls use `SOC_DAPM_*` macros:

```c
static const struct snd_kcontrol_new out_mix_controls[] = {
        SOC_DAPM_SINGLE("DAC Switch", DEMO_MIX, 0, 1, 0),
        SOC_DAPM_SINGLE("Line Bypass Switch", DEMO_MIX, 1, 1, 0),
};
```

Static DAPM registration:

```c
static const struct snd_soc_dapm_widget demo_widgets[] = {
        SND_SOC_DAPM_AIF_IN("AIFIN", "Playback", 0, SND_SOC_NOPM, 0, 0),
        SND_SOC_DAPM_DAC("DAC", "Playback", DEMO_PWR, 0, 1),
        SND_SOC_DAPM_MIXER("Output Mixer", DEMO_PWR, 1, 1,
                           out_mix_controls, ARRAY_SIZE(out_mix_controls)),
        SND_SOC_DAPM_OUTPUT("HP_OUT"),
};

static const struct snd_soc_dapm_route demo_routes[] = {
        { "DAC", NULL, "AIFIN" },
        { "Output Mixer", "DAC Switch", "DAC" },
        { "HP_OUT", NULL, "Output Mixer" },
};
```

Static arrays are normally attached to `struct snd_soc_component_driver`.

## Lifecycle / Data Flow

The audio path starts at userspace, but several layers participate.

### Probe And Registration

```text
codec bus probe
  devm_kzalloc private state
  devm_regmap_init_i2c/spi/mmio
  get regulators/clocks/reset GPIOs
  read chip ID or variant data
  devm_snd_soc_register_component()
```

At this point the codec exists as an ASoC component, but there may still be no sound card.

### Card Binding

```text
machine/simple-card probe
  parse CPU DAI phandle
  parse codec DAI phandle
  parse routing/widgets/clocks
  register snd_soc_card
  ASoC probes components and DAIs
  PCM devices and controls appear
```

This is why `i2cdetect` or a successful codec probe does not guarantee that `aplay -l` will show a usable card.

### Playback Example

```text
aplay opens PCM
  ALSA/ASoC startup
  hw_params chooses rate/format/channels
  machine/card configures CPU and codec DAIs
  codec DAI programs PLL/sysclk/format if needed
  DAPM powers AIF/DAC/mixer/output path
  PCM DMA feeds CPU DAI FIFO
  CPU DAI sends samples over I2S/TDM/etc.
  codec DAI receives samples
  DAC/mixer/output path drives speaker/headphone
```

### Capture Example

```text
arecord opens PCM
  DAPM powers input/mic bias/ADC/AIF path
  codec captures analog signal into digital samples
  codec DAI sends samples to CPU DAI
  PCM DMA writes samples to memory
  ALSA returns PCM data to userspace
```

### Stop And Power Down

```text
STREAM stop / PCM close
  trigger/shutdown callbacks run
  DAPM recomputes active paths
  unused ADC/DAC/AIF/mixer/supply widgets power down
  clocks/regulators/bias may turn off if no active route uses them
```

## Minimal Practical Example

This is **learning-only pseudo-code**. It shows the shape of a small codec/component driver, not a complete production driver for real hardware.

```c
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <sound/soc.h>
#include <sound/tlv.h>

#define DEMO_PWR        0x01
#define DEMO_FMT        0x02
#define DEMO_VOL        0x10
#define DEMO_MIX        0x20

struct demo_priv {
        struct regmap *regmap;
        unsigned int sysclk;
        unsigned int fmt;
};

static const struct regmap_config demo_regmap_config = {
        .reg_bits = 8,
        .val_bits = 8,
        .max_register = 0x7f,
};

static int demo_set_sysclk(struct snd_soc_dai *dai, int clk_id,
                           unsigned int freq, int dir)
{
        struct snd_soc_component *component = dai->component;
        struct demo_priv *priv = snd_soc_component_get_drvdata(component);

        priv->sysclk = freq;
        return 0;
}

static int demo_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
        struct snd_soc_component *component = dai->component;
        struct demo_priv *priv = snd_soc_component_get_drvdata(component);

        priv->fmt = fmt;
        return 0;
}

static int demo_hw_params(struct snd_pcm_substream *substream,
                          struct snd_pcm_hw_params *params,
                          struct snd_soc_dai *dai)
{
        struct snd_soc_component *component = dai->component;
        unsigned int rate = params_rate(params);

        if (rate < 8000 || rate > 96000)
                return -EINVAL;

        return snd_soc_component_update_bits(component, DEMO_FMT,
                                             0x03, 0x01);
}

static int demo_mute_stream(struct snd_soc_dai *dai, int mute, int stream)
{
        struct snd_soc_component *component = dai->component;

        return snd_soc_component_update_bits(component, DEMO_PWR,
                                             BIT(7), mute ? BIT(7) : 0);
}

static const struct snd_soc_dai_ops demo_dai_ops = {
        .set_sysclk = demo_set_sysclk,
        .set_fmt = demo_set_fmt,
        .hw_params = demo_hw_params,
        .mute_stream = demo_mute_stream,
};

static struct snd_soc_dai_driver demo_dai = {
        .name = "demo-hifi",
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
};

static const DECLARE_TLV_DB_SCALE(demo_out_tlv, -6350, 50, 0);

static const struct snd_kcontrol_new demo_controls[] = {
        SOC_SINGLE_TLV("Playback Volume", DEMO_VOL, 0, 127, 0,
                       demo_out_tlv),
};

static const struct snd_kcontrol_new demo_mixer_controls[] = {
        SOC_DAPM_SINGLE("DAC Switch", DEMO_MIX, 0, 1, 0),
};

static const struct snd_soc_dapm_widget demo_widgets[] = {
        SND_SOC_DAPM_AIF_IN("AIFIN", "Playback", 0, SND_SOC_NOPM, 0, 0),
        SND_SOC_DAPM_DAC("DAC", "Playback", DEMO_PWR, 0, 1),
        SND_SOC_DAPM_MIXER("Output Mixer", DEMO_PWR, 1, 1,
                           demo_mixer_controls,
                           ARRAY_SIZE(demo_mixer_controls)),
        SND_SOC_DAPM_OUTPUT("HP_OUT"),
};

static const struct snd_soc_dapm_route demo_routes[] = {
        { "DAC", NULL, "AIFIN" },
        { "Output Mixer", "DAC Switch", "DAC" },
        { "HP_OUT", NULL, "Output Mixer" },
};

static const struct snd_soc_component_driver demo_component = {
        .controls = demo_controls,
        .num_controls = ARRAY_SIZE(demo_controls),
        .dapm_widgets = demo_widgets,
        .num_dapm_widgets = ARRAY_SIZE(demo_widgets),
        .dapm_routes = demo_routes,
        .num_dapm_routes = ARRAY_SIZE(demo_routes),
};

static int demo_i2c_probe(struct i2c_client *client)
{
        struct demo_priv *priv;

        priv = devm_kzalloc(&client->dev, sizeof(*priv), GFP_KERNEL);
        if (!priv)
                return -ENOMEM;

        priv->regmap = devm_regmap_init_i2c(client, &demo_regmap_config);
        if (IS_ERR(priv->regmap))
                return PTR_ERR(priv->regmap);

        i2c_set_clientdata(client, priv);

        return devm_snd_soc_register_component(&client->dev,
                                               &demo_component,
                                               &demo_dai, 1);
}
```

What the example teaches:

- The bus driver initializes private state and regmap first.
- `devm_snd_soc_register_component()` exports the component and its DAI to ASoC.
- Static controls become ALSA mixer controls.
- Static DAPM widgets/routes describe the component's internal path.
- The DAI driver advertises playback/capture constraints and callbacks.

What it does not do:

- It does not create a sound card by itself.
- It does not include a real Device Tree binding.
- It does not program real codec clock trees, reset timing, supplies, or register defaults.
- It does not replace a hardware datasheet.

## Common Bugs And Debugging

Start with the symptom. ASoC failures often look similar from userspace, but the failing layer is different.

### Codec Probes, But No Sound Card Appears

Likely causes:

- No machine driver or simple-card node binds the CPU DAI and codec DAI.
- DAI names in DT/machine code do not match the driver.
- Component registration failed but the error was hidden.
- Required clocks/regulators/reset GPIOs are missing or deferred.

Check:

```bash
dmesg | grep -i -E 'asoc|snd|codec|defer|failed'
aplay -l
arecord -l
ls /sys/kernel/debug/asoc 2>/dev/null
```

Fix pattern:

- Confirm codec bus probe and `devm_snd_soc_register_component()` succeed.
- Confirm the card/machine driver probes.
- Confirm CPU DAI, codec DAI, and platform component names/phandles match.

### PCM Plays, But No Audio Comes Out

Likely causes:

- Missing DAPM route from DAC/AIF to output pin.
- Machine route does not connect codec pin to board endpoint.
- Mixer switch is off.
- Wrong DAI format, clock polarity, master/slave, or sample format.
- Amplifier, regulator, or clock supply is not modeled or enabled.

Check:

```bash
amixer -c <card>
alsamixer -c <card>
cat /sys/kernel/debug/asoc/*/dapm/* 2>/dev/null
dmesg -w
```

Hardware checks:

- MCLK present?
- BCLK/LRCLK present during playback?
- Expected sample rate?
- Any activity on data line?
- Output amplifier enabled?

### DAPM Widget Never Powers

Likely causes:

- Route direction reversed.
- DAPM stream widget name does not match DAI stream name.
- Used `SOC_*` where `SOC_DAPM_*` was needed.
- Endpoint is not connected by machine route.
- Control name in route does not match the DAPM control name.

Debug pattern:

- Print or inspect the DAPM graph.
- Verify route entries are `{ sink, control, source }`.
- Toggle mixer controls with `amixer` and watch DAPM state.
- Compare widget names exactly, including spaces and capitalization.

### Wrong Rate, Noise, Or Distorted Audio

Likely causes:

- MCLK/PLL/sysclk frequency does not match sample rate family.
- CPU and codec disagree on I2S/left-justified/right-justified/DSP mode.
- Bit clock or frame clock polarity is wrong.
- TDM slot width or mask is wrong.
- `hw_params()` accepts a format that hardware was not programmed for.

Check:

- `aplay -v` for actual ALSA parameters.
- DAI `hw_params()` logs.
- Clock debugfs for MCLK source/rate.
- Logic analyzer for BCLK/LRCLK/data timing.

### High Idle Current Or Pops/Clicks

Likely causes:

- Supplies/clocks/amps are always enabled instead of DAPM-controlled.
- Missing `mute_stream` or wrong mute timing.
- Bias/VREF/VMID sequencing is incomplete.
- Widget event callbacks use the wrong event flags.
- Suspend/resume does not sync regmap cache or restore state.

Fix pattern:

- Model power blocks as `SND_SOC_DAPM_SUPPLY`, `SND_SOC_DAPM_REGULATOR_SUPPLY`, or `SND_SOC_DAPM_CLOCK_SUPPLY`.
- Use DAPM event callbacks for sequencing that cannot be represented by a single register bit.
- Mute before power-down and unmute after the path is stable.

## Production Checklist

Use this before review or board bring-up.

### Hardware And Register Access

- Confirm chip ID, revision, reset defaults, and register address width/value width.
- Define regmap readable/writeable/volatile/precious registers correctly.
- Use regmap cache carefully around suspend/resume and reset.
- Handle register pages/windows if the codec has them.
- Do not bypass regmap for normal register access unless there is a clear reason.

### Clocks And Formats

- Validate MCLK, BCLK, LRCLK, PLL source, dividers, and supported rate families.
- Implement only supported DAI formats and reject unsupported ones.
- Check master/slave policy against board design.
- Validate TDM slot masks, slot width, channel count, and sample width.
- Test 8 kHz, 44.1 kHz, 48 kHz, and maximum supported rates when applicable.

### Controls And ABI

- Use stable ALSA control names.
- Use TLV dB metadata for volume controls.
- Avoid exposing raw debug controls as stable user ABI.
- Use DAPM controls for path-affecting switches/muxes.
- Test with `amixer` and `alsamixer`.

### DAPM

- Draw the real audio path from datasheet and schematic.
- Define widgets for ADC, DAC, AIF, mixers, muxes, PGAs, pins, supplies, clocks, and regulators.
- Verify route direction and exact names.
- Make stream widget names match DAI stream names.
- Keep board endpoints and connector routes in machine/simple-card unless the component genuinely owns them.
- Check idle power and active path power with DAPM debugfs and hardware measurements.

### Lifecycle And Error Paths

- Use managed allocation where appropriate.
- Return probe deferral cleanly for missing clocks/regulators/GPIOs.
- Stop streams and mute safely before disabling supplies.
- Ensure remove/suspend paths cannot race active streams.
- Test module unload/reload if built as a module.

## Interview Readiness

You are ready for interviews when you can explain the flow without reciting structs by memory.

You should be able to answer:

- Why does ASoC split codec, CPU/platform, and machine drivers?
- What does `devm_snd_soc_register_component()` register?
- Why can a codec probe successfully but no ALSA card appear?
- What does a DAI driver describe?
- What happens during `hw_params()`?
- Why does DAPM need widgets and routes?
- What is the difference between `SOC_SINGLE` and `SOC_DAPM_SINGLE`?
- Why is route direction sink/control/source?
- How would you debug silent playback?
- Which details belong in topic 36 machine-driver code instead of the codec driver?

See `interview/35-asoc-codec-component-dai-and-dapm.md` for full question-and-answer practice.

## Kernel Version Notes

ASoC has important naming and API drift.

- Before Linux v4.18, many docs and drivers used a stronger split between `struct snd_soc_codec` and `struct snd_soc_platform`.
- Modern code should think primarily in terms of `struct snd_soc_component` and `struct snd_soc_component_driver`.
- Older material may mention `digital_mute`; current DAI ops commonly use `mute_stream`.
- Older component examples may show `pcm_new` / `pcm_free`; current headers use names such as `pcm_construct` / `pcm_destruct`.
- Static component-driver arrays for controls, DAPM widgets, and routes are the common modern style; dynamic DAPM registration is still useful for variant-dependent hardware.
- Always check the target kernel headers for exact callback signatures before writing code.
