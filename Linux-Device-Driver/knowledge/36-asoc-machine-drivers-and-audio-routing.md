# 36 - ASoC Machine Drivers And Audio Routing

## Learning Goal

After this topic, you should be able to explain how an embedded Linux sound card is assembled from reusable ASoC pieces, decide when a board can use `simple-audio-card`, and reason through common bring-up failures such as no card, no PCM device, silent playback, or broken clocks.

You should be comfortable with:

- The role of a **machine driver** in the ASoC architecture.
- How `struct snd_soc_card` and `struct snd_soc_dai_link` describe a board sound card.
- How CPU DAI, codec DAI, and platform/PCM components are matched.
- How board widgets and DAPM routes connect codec pins to real connectors.
- How clock format, provider/consumer roles, PLL, sysclk, and TDM slot setup are usually applied.
- How to debug card registration, DAI-link matching, routing, and audio clock problems.

## Why This Matters In Real Work

Most embedded audio failures are not caused by a bad codec register write. They happen because the board-level wiring description is wrong: the wrong DAI is linked, a route name does not match, a clock role is inverted, or the card never binds because one component is not ready.

ASoC splits audio support into reusable and board-specific parts:

| Layer | Main Job | Usually Reusable? |
| --- | --- | --- |
| Codec/component driver | Describes codec DAIs, controls, pins, DAPM internals, register access | Yes |
| CPU DAI / PCM DMA driver | Describes SoC audio controller and DMA path | Yes |
| Machine driver / simple-card | Describes board wiring, DAI links, card name, routing, clock policy | Usually no |

The machine layer matters because it is the point where working individual drivers become a real ALSA card visible to userspace.

Without a valid machine description:

- The codec can probe successfully.
- The CPU audio controller can probe successfully.
- The DMA engine can probe successfully.
- But `aplay -l` still shows no useful sound card.

## Mental Model

Think of ASoC components as parts on a bench.

- The **codec driver** says: "I have a DAI named `HiFi`, pins called `HP_OUT` and `MIC_IN`, and controls for volume/muxes."
- The **CPU DAI driver** says: "I can send samples over this I2S/TDM/S/PDIF block."
- The **PCM DMA/platform part** says: "I can move audio samples between memory and the audio FIFO."
- The **machine driver** says: "On this board, that CPU DAI is wired to that codec DAI, the codec is the bit-clock provider, the headphone jack is connected to `HP_OUT`, and this is the card userspace should see."

The registered `struct snd_soc_card` is the final assembled sound card.

## Core Concepts

Machine-driver work is mostly about matching names, nodes, clocks, and routes correctly.

### Machine Driver

A custom machine driver is usually a platform driver whose `probe()` builds a `struct snd_soc_card`.

It usually handles:

- DAI-link endpoint selection.
- Card name/model parsing.
- Board widgets such as speakers, microphones, headphones, and line connectors.
- DAPM routes between board connectors and codec pins.
- Machine-level controls such as pin switches.
- DAI format and clock setup in `startup()` or `hw_params()`.
- Board-specific quirks such as amplifier GPIOs, jack GPIOs, or special PLL setup.

### DAI Link

A DAI link is the logical audio connection between CPU side and codec side.

It describes:

- CPU DAI endpoint.
- Codec DAI endpoint.
- Platform/PCM endpoint.
- Stream name.
- Audio bus format.
- Clock provider/consumer relationship.
- Optional machine callbacks.
- Playback-only or capture-only direction when needed.

### Sound Card

`struct snd_soc_card` is the ASoC card object. It owns the board-level description that ASoC uses to instantiate ALSA PCM/control devices.

It points to:

- One or more DAI links.
- Card-level controls.
- Board DAPM widgets.
- Board DAPM routes.
- Optional card callbacks.

### Board Widgets And Routes

Codec drivers usually define codec pins such as `MIC_IN`, `LINE_IN`, `HP_OUT`, and `LINE_OUT`.

Machine drivers define board names such as:

- `Mic Jack`
- `Headphone Jack`
- `Speaker`
- `Line In Jack`

DAPM routes connect those names. If the string names do not match exactly, DAPM cannot build the path.

### Simple-Card

`simple-audio-card` is a generic machine driver described from Device Tree.

Use it when:

- There is one straightforward CPU DAI to codec DAI link.
- Routing can be described in DT.
- Clock format/mastership can be described in DT.
- No custom C callback or board-specific sequence is needed.

Use a custom machine driver when you need non-trivial clock setup, GPIO sequencing, jack logic, multiple links, DPCM, codec-less links, or board-specific quirks.

## Kernel Mechanism

The machine driver glues existing ASoC components into a card. It does not usually implement codec register access or audio DMA itself.

Typical object relationship:

```text
Device Tree sound node
        |
        v
platform_driver probe()
        |
        v
struct snd_soc_card
        |
        +-- struct snd_soc_dai_link[0]
        |       +-- CPU DAI endpoint
        |       +-- codec DAI endpoint
        |       +-- platform/PCM endpoint
        |       +-- format/clock policy
        |       +-- machine stream ops
        |
        +-- board DAPM widgets
        +-- board DAPM routes
        +-- card controls
        |
        v
devm_snd_soc_register_card()
        |
        v
ASoC resolves components, probes runtimes, creates PCM/control devices
```

Important matching rules:

- CPU endpoint must match a registered CPU DAI.
- Codec endpoint must match a registered codec/component DAI.
- Platform endpoint must match the component that provides PCM/DMA support, unless the link form does not require one.
- DAI names must match the names exported by the DAI drivers.
- DT phandles must point to the correct nodes.
- Route names must match DAPM widget names.

When card registration runs, ASoC may return `-EPROBE_DEFER` if the CPU DAI, codec, clock, regulator, or another dependency is not ready. That is normal during boot.

## Key Structs And APIs

These APIs are easiest to remember by the job they do.

### Card And Link Objects

| Struct / API | Purpose |
| --- | --- |
| `struct snd_soc_card` | Board-level sound card description |
| `struct snd_soc_dai_link` | One logical CPU-to-codec audio link |
| `struct snd_soc_dai_link_component` | Current-kernel endpoint descriptor for CPU/codec/platform sides |
| `struct snd_soc_ops` | Machine-level PCM callbacks for a DAI link |
| `struct snd_soc_pcm_runtime` | Runtime object created when ASoC instantiates a DAI link |
| `struct snd_soc_dai` | Runtime DAI object used to configure CPU or codec DAI |

### Registration

| API | Purpose |
| --- | --- |
| `devm_snd_soc_register_card(dev, card)` | Managed card registration, preferred for platform-probed machine drivers |
| `snd_soc_register_card(card)` | Unmanaged card registration |
| `snd_soc_unregister_card(card)` | Unmanaged unregister path |
| `module_platform_driver(driver)` | Usual wrapper for custom machine-driver platform modules |

### Current DAI-Link Helpers

Modern kernels commonly use component arrays and macros:

| Helper | Purpose |
| --- | --- |
| `SND_SOC_DAILINK_DEFS()` | Define CPU, codec, and platform endpoint arrays |
| `SND_SOC_DAILINK_DEF()` | Define one endpoint array manually |
| `SND_SOC_DAILINK_REG()` | Attach endpoint arrays to a `struct snd_soc_dai_link` |
| `DAILINK_COMP_ARRAY()` | Wrap endpoint definitions |
| `COMP_CPU()` | CPU DAI endpoint by DAI name |
| `COMP_CODEC()` | Codec endpoint by device name and DAI name |
| `COMP_PLATFORM()` | Platform/PCM endpoint by name |
| `COMP_DUMMY()` | Dummy endpoint for special links when appropriate |

Device Tree based drivers often fill `.of_node` fields dynamically after parsing phandles.

### Device Tree Parsing

| API | Purpose |
| --- | --- |
| `of_parse_phandle()` | Get referenced DT node such as codec or CPU DAI |
| `of_parse_phandle_with_args()` | Get phandle plus argument cells, useful for provider/consumer bindings |
| `snd_soc_of_parse_card_name()` | Parse card name/model property into `struct snd_soc_card` |
| `snd_soc_of_parse_audio_routing()` | Parse route string pairs from DT |
| `snd_soc_of_parse_audio_simple_widgets()` | Parse simple widgets from DT |
| `snd_soc_of_parse_pin_switches()` | Parse DAPM pin switches from DT |

### Format And Clock Setup

| API / Macro | Purpose |
| --- | --- |
| `snd_soc_dai_set_fmt()` | Set DAI bus protocol, clock provider/consumer roles, inversion |
| `snd_soc_dai_set_sysclk()` | Select/configure system clock |
| `snd_soc_dai_set_pll()` | Configure DAI PLL/FLL when hardware supports it |
| `snd_soc_dai_set_clkdiv()` | Configure clock divider |
| `snd_soc_dai_set_tdm_slot()` | Configure TDM slot masks, slot count, slot width |
| `SND_SOC_DAIFMT_I2S` | I2S protocol |
| `SND_SOC_DAIFMT_LEFT_J`, `SND_SOC_DAIFMT_RIGHT_J` | Left/right justified protocols |
| `SND_SOC_DAIFMT_DSP_A`, `SND_SOC_DAIFMT_DSP_B` | DSP/PCM-style frame timing |
| `SND_SOC_DAIFMT_NB_NF`, `SND_SOC_DAIFMT_NB_IF`, `SND_SOC_DAIFMT_IB_NF`, `SND_SOC_DAIFMT_IB_IF` | Bit-clock/frame inversion |
| `SND_SOC_DAIFMT_CBP_CFP`, `SND_SOC_DAIFMT_CBC_CFC` and related macros | Clock/frame provider or consumer role |

Older examples often use master/slave aliases such as `SND_SOC_DAIFMT_CBM_CFM` and `SND_SOC_DAIFMT_CBS_CFS`. Modern explanations should prefer provider/consumer wording.

### Board Widgets And Routes

| Macro / Struct | Purpose |
| --- | --- |
| `SND_SOC_DAPM_MIC()` | Board microphone connector |
| `SND_SOC_DAPM_HP()` | Headphone connector |
| `SND_SOC_DAPM_SPK()` | Speaker connector |
| `SND_SOC_DAPM_LINE()` | Line input/output connector |
| `struct snd_soc_dapm_route` | Connection between DAPM widgets |
| `SOC_DAPM_PIN_SWITCH()` | Userspace-visible DAPM pin switch |

## Lifecycle / Data Flow

The card lifecycle is a dependency-resolution problem as much as it is an audio problem.

```text
1. CPU DAI / PCM driver probes
   - registers SoC audio DAI
   - registers PCM/DMA support

2. Codec/component driver probes
   - initializes regmap, supplies, clocks, reset, IRQs
   - registers codec component and codec DAI(s)

3. Machine driver probes
   - parses sound node
   - parses CPU DAI and codec phandles
   - fills DAI link endpoints
   - sets card name/device
   - attaches widgets, routes, controls, ops
   - registers the card

4. ASoC instantiates the card
   - resolves endpoints
   - probes component/card runtime objects
   - creates PCM/control devices
   - builds DAPM graph

5. Userspace opens a stream
   - ALSA opens PCM substream
   - machine `startup()` may apply constraints
   - `hw_params()` configures rate/format/channels
   - machine code configures DAI format/clocks/PLL/TDM
   - DAPM powers the required path

6. Stream runs
   - DMA moves samples between memory and CPU audio FIFO
   - CPU DAI transfers samples over I2S/TDM/PCM/S/PDIF
   - codec converts/routes audio through selected path

7. Stream stops or device removes
   - ALSA stops trigger path
   - DAPM powers unused widgets down
   - devm-managed card/resources clean up on driver removal
```

## Minimal Practical Example

This is **learning-only pseudo-code**. It shows the shape of a custom ASoC machine driver, not a copy-paste production driver. Real code must match the target kernel, the binding schema, and the board schematic.

```c
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <sound/soc.h>

struct demo_audio {
	struct snd_soc_card card;
	struct snd_soc_dai_link link;
	struct snd_soc_dai_link_component cpus[1];
	struct snd_soc_dai_link_component codecs[1];
	struct snd_soc_dai_link_component platforms[1];
};

static const struct snd_soc_dapm_widget demo_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_MIC("Mic Jack", NULL),
};

static const struct snd_soc_dapm_route demo_routes[] = {
	{ "Headphone Jack", NULL, "HP_OUT" },
	{ "MIC_IN", NULL, "Mic Jack" },
};

static int demo_hw_params(struct snd_pcm_substream *substream,
			  struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = snd_soc_substream_to_rtd(substream);
	struct snd_soc_dai *cpu_dai = snd_soc_rtd_to_cpu(rtd, 0);
	struct snd_soc_dai *codec_dai = snd_soc_rtd_to_codec(rtd, 0);
	unsigned int fmt;
	int ret;

	fmt = SND_SOC_DAIFMT_I2S |
	      SND_SOC_DAIFMT_NB_NF |
	      SND_SOC_DAIFMT_CBC_CFC; /* codec consumes BCLK/LRCLK */

	ret = snd_soc_dai_set_fmt(cpu_dai, fmt);
	if (ret)
		return ret;

	ret = snd_soc_dai_set_fmt(codec_dai, fmt);
	if (ret)
		return ret;

	return snd_soc_dai_set_sysclk(codec_dai, 0,
				      params_rate(params) * 256,
				      SND_SOC_CLOCK_IN);
}

static const struct snd_soc_ops demo_ops = {
	.hw_params = demo_hw_params,
};

static int demo_audio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *cpu_np, *codec_np;
	struct demo_audio *priv;
	int ret;

	cpu_np = of_parse_phandle(np, "cpu-dai", 0);
	codec_np = of_parse_phandle(np, "audio-codec", 0);
	if (!cpu_np || !codec_np)
		return dev_err_probe(dev, -EINVAL, "missing DAI phandle\n");

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->cpus[0].of_node = cpu_np;
	priv->codecs[0].of_node = codec_np;
	priv->codecs[0].dai_name = "HiFi";
	priv->platforms[0].of_node = cpu_np;

	priv->link.name = "demo-link";
	priv->link.stream_name = "HiFi";
	priv->link.cpus = priv->cpus;
	priv->link.num_cpus = ARRAY_SIZE(priv->cpus);
	priv->link.codecs = priv->codecs;
	priv->link.num_codecs = ARRAY_SIZE(priv->codecs);
	priv->link.platforms = priv->platforms;
	priv->link.num_platforms = ARRAY_SIZE(priv->platforms);
	priv->link.ops = &demo_ops;

	priv->card.dev = dev;
	priv->card.owner = THIS_MODULE;
	priv->card.dai_link = &priv->link;
	priv->card.num_links = 1;
	priv->card.dapm_widgets = demo_widgets;
	priv->card.num_dapm_widgets = ARRAY_SIZE(demo_widgets);
	priv->card.dapm_routes = demo_routes;
	priv->card.num_dapm_routes = ARRAY_SIZE(demo_routes);

	ret = snd_soc_of_parse_card_name(&priv->card, "model");
	if (ret)
		priv->card.name = "demo-audio";

	ret = snd_soc_of_parse_audio_routing(&priv->card, "audio-routing");
	if (ret && ret != -EINVAL)
		return dev_err_probe(dev, ret, "bad audio-routing\n");

	return devm_snd_soc_register_card(dev, &priv->card);
}
```

Important lines:

- `of_parse_phandle()` ties the sound node to CPU and codec nodes.
- `priv->codecs[0].dai_name = "HiFi"` must match the codec DAI name.
- `platforms[0].of_node = cpu_np` is common when the CPU DAI and PCM/DMA component are represented by the same node.
- `demo_routes[]` connects board connector names to codec pin/widget names.
- `demo_hw_params()` is where link-level clock and format policy is applied.
- `devm_snd_soc_register_card()` makes the ALSA card visible if all components resolve.

Example DT shape:

```dts
sound {
	compatible = "vendor,demo-audio";
	model = "Demo Board Audio";
	cpu-dai = <&sai1>;
	audio-codec = <&codec>;

	audio-routing =
		"Headphone Jack", "HP_OUT",
		"MIC_IN", "Mic Jack";
};
```

For a simple board, a `simple-audio-card` node may replace the custom C driver:

```dts
sound {
	compatible = "simple-audio-card";
	simple-audio-card,name = "Demo Board Audio";
	simple-audio-card,format = "i2s";
	simple-audio-card,widgets =
		"Headphone", "Headphone Jack",
		"Microphone", "Mic Jack";
	simple-audio-card,routing =
		"Headphone Jack", "HP_OUT",
		"MIC_IN", "Mic Jack";

	simple-audio-card,cpu {
		sound-dai = <&sai1>;
	};

	simple-audio-card,codec {
		sound-dai = <&codec>;
	};
};
```

## Common Bugs And Debugging

Start with what userspace can see, then walk backward into card registration, DAI links, DAPM, clocks, and hardware.

### No Sound Card Appears

Symptoms:

- `aplay -l` shows no expected card.
- `/proc/asound/cards` does not list the board audio card.
- `dmesg` shows ASoC probe errors or repeated deferrals.

Likely causes:

- Machine/simple-card compatible string does not match any driver.
- Machine driver was not built or loaded.
- `devm_snd_soc_register_card()` failed.
- Codec or CPU DAI component has not probed yet.
- Wrong phandle in `sound-dai`, `cpu-dai`, `audio-codec`, or board-specific property.
- Missing clock, regulator, pinctrl, or bus dependency caused `-EPROBE_DEFER`.

Useful checks:

```bash
dmesg | grep -i -E 'asoc|alsa|snd|codec|dai|defer|audio'
cat /proc/asound/cards
aplay -l
find /sys/bus/platform/drivers -iname '*audio*' -o -iname '*sound*'
```

### Card Appears But No PCM Device

Symptoms:

- Card exists, but playback/capture devices are missing.
- `aplay -l` does not show the expected PCM.

Likely causes:

- DAI link did not resolve.
- Codec DAI name does not match the codec driver's registered DAI name.
- CPU DAI node is wrong.
- Platform/PCM component is missing.
- Link direction flags are wrong.

Useful checks:

- Look for errors about DAI lookup or link creation in `dmesg`.
- Check codec driver DAI names in the driver source.
- Check CPU DAI names or DT `#sound-dai-cells`.
- Inspect ASoC debugfs if enabled:

```bash
mount -t debugfs none /sys/kernel/debug
ls /sys/kernel/debug/asoc
```

### PCM Opens But Playback Is Silent

Symptoms:

- `aplay` runs without obvious errors.
- No sound at the speaker/headphone.

Likely causes:

- Missing DAPM route from board connector to codec pin.
- Route string names do not exactly match widget names.
- Mixer or pin switch is off.
- Amplifier GPIO or regulator is not enabled.
- Wrong pinmux for audio pins.
- Codec output path is not powered because DAPM graph is incomplete.

Useful checks:

```bash
amixer -c <card>
alsamixer -c <card>
grep -R . /sys/kernel/debug/asoc/* 2>/dev/null | less
```

Also check the schematic. Software can only route to pins that are really wired.

### Stream Fails At A Specific Rate Or Format

Symptoms:

- 48 kHz works but 44.1 kHz fails.
- 16-bit works but 24-bit fails.
- `hw_params` or `prepare` returns an error.

Likely causes:

- CPU DAI, codec DAI, and DMA constraints do not overlap.
- MCLK cannot generate the requested rate family.
- PLL/sysclk formula is wrong.
- TDM slot width/count does not match the codec and CPU.

Useful checks:

- Inspect `hw_params()` return path.
- Compare supported rates/formats/channels in codec and CPU DAI drivers.
- Check `/sys/kernel/debug/clk/clk_summary` for clock rates.
- Use a scope or logic analyzer for MCLK, BCLK, LRCLK, and data.

### Audio Has Noise, Wrong Speed, Or Channels Are Swapped

Likely causes:

- Wrong `SND_SOC_DAIFMT_*` protocol.
- Bit-clock or frame-clock inversion is wrong.
- Clock provider/consumer role is wrong.
- TDM slot mask or slot width is wrong.
- CPU and codec disagree on sample size or word alignment.

Debug strategy:

- Verify bus timing against codec datasheet timing diagrams.
- Check whether codec or CPU should provide BCLK/LRCLK.
- Capture BCLK/LRCLK/data with a logic analyzer.
- Try only one variable at a time: protocol, inversion, clock role, slot width.

## Production Checklist

Before submitting or bringing up an ASoC machine driver, check the board story end to end.

Card and binding:

- The card name is stable and meaningful.
- The DT binding is documented or uses an existing generic binding.
- `compatible` strings match the intended driver.
- `MODULE_DEVICE_TABLE(of, ...)` exists for custom platform machine drivers.
- `devm_snd_soc_register_card()` error paths use helpful `dev_err_probe()` style messages where appropriate.

DAI links:

- CPU and codec endpoints match the actual board wiring.
- DAI names match the registered DAI names.
- Platform/PCM endpoint is correct or intentionally omitted.
- Multi-link boards have clear link names and directions.
- Playback-only/capture-only flags are correct.

Routing:

- Board widgets represent real connectors or useful logical controls.
- DAPM route names exactly match codec and machine widget names.
- `audio-routing` is used when board variants need routing changes.
- Static routing is acceptable only when routing is fixed for that driver.
- Pin switches and user-facing controls are named consistently.

Clocks and formats:

- DAI format matches the electrical protocol.
- Clock provider/consumer roles match the schematic.
- MCLK/sysclk/PLL rates are valid for every supported sample-rate family.
- TDM slot masks, slot widths, and channel maps are documented and checked.
- Clock and regulator dependencies handle probe deferral correctly.

Power and hardware:

- Codec supplies and external amplifiers are modeled through regulator, GPIO, DAPM, or component mechanisms where appropriate.
- Jack/amp GPIOs are not toggled in random PCM paths if DAPM events would be cleaner.
- Pop/click prevention is considered during power sequencing.
- Suspend/resume behavior does not leave clocks, routes, or amplifiers in an unsafe state.

Debug and userspace:

- `aplay -l`, `arecord -l`, `/proc/asound/cards`, and `amixer` output are checked.
- ASoC debugfs and clock summary are usable during bring-up.
- Card/control names are suitable for UCM profiles, PipeWire/PulseAudio policy, factory tests, and scripts.

## Interview Readiness

You are ready for interviews when you can explain the board-level audio path without hiding behind API names.

Be able to answer:

- Why does ASoC need a machine driver at all?
- What does `struct snd_soc_card` own?
- What does `struct snd_soc_dai_link` connect?
- Why can a codec probe successfully but no sound card appear?
- How do `simple-audio-card` and a custom machine driver differ?
- Where do DAPM board widgets and routes fit?
- Where do you configure I2S format and clock provider/consumer roles?
- How would you debug no card, no PCM, silent playback, and bad sample-rate behavior?

Practice with:

- `interview/36-asoc-machine-drivers-and-audio-routing.md`

## Kernel Version Notes

ASoC machine-driver APIs are version-sensitive. The core mental model is stable, but some field names and helper styles changed over time.

Important notes:

- Older examples often show direct `cpu_of_node`, `codec_of_node`, and `platform_of_node` fields in `struct snd_soc_dai_link`.
- Current kernels commonly use `struct snd_soc_dai_link_component` arrays through `cpus`, `codecs`, and `platforms`.
- Helper macros such as `SND_SOC_DAILINK_DEFS()`, `SND_SOC_DAILINK_REG()`, `COMP_CPU()`, `COMP_CODEC()`, and `COMP_PLATFORM()` are common in modern examples.
- Older format flags use master/slave wording such as `SND_SOC_DAIFMT_CBM_CFM`; newer explanations prefer provider/consumer wording such as codec bit-clock provider and frame provider.
- Old simple-card text bindings are commonly replaced by YAML binding schemas in current kernels.

For production code, always compare against the target kernel headers and nearby in-tree `sound/soc/` examples.

