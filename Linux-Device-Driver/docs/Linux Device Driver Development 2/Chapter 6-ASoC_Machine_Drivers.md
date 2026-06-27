```bash
# Chapter 6 - ALSA SoC Framework – Delving into the Machine Class Drivers
```
While starting our ALSA SoC framework series, we noticed that neither platform nor codec class drivers are intended to work on their own. The ASoC architecture is designed in such a way that platform and codec class drivers must be bound together in order to build the audio device. This binding can be done either from a so-called machine driver or from within the device tree, each of which being machine specific. It then goes without saying that the machine driver targets a specific system, and it may change from one board to another. In this chapter, we highlight the dark side of AsoC machine class drivers and discuss specific cases we may encounter when we need to write a machine class driver.
274 ALSA SoC Framework – Delving into the Machine Class Drivers
In this chapter, we will present the Linux ASoC driver architecture and implementation.
This chapter will be split into different parts, which are as follows:
• Introduction to machine class drivers
• Machine routing considerations
• Clocking and formatting considerations
• Sound card registration
• Leveraging the simple-card machine driver
## Technical requirements
You need the following for this chapter:
• Strong knowledge of the concept of device trees
• Familiarity with both platform and codec class drivers (discussed in Chapter 5,
ALSA SoC Framework – Leveraging Codec and Platform Class Drivers)
• Linux kernel v4.19.X sources, available at https://git.kernel.org/pub/
scm/linux/kernel/git/stable/linux.git/refs/tags
## Introduction to machine class drivers
Codec and platform drivers cannot work alone. Machine drivers are responsible for binding them together in order to finish audio information processing. The machine driver class acts as the glue that describes and ties the other component drivers together to form an ALSA sound card device. It manages any machine-specific controls and machine-level audio events (such as turning on an amp at the start of playback). The machine drivers describe and bind the CPU Digital Audio Interfaces (DAIs) and codec drivers together to create the DAI links and the ALSA sound card. The machine driver connects the codec drivers by linking the DAIs exposed by each module (the CPU and codec) described in Chapter 5, ALSA SoC Framework – Leveraging Codec and Platform
Class Drivers. It defines the struct snd_soc_dai_link structure and instantiates the sound card, struct snd_soc_card.
Introduction to machine class drivers 275
Platform and codec drivers are generally reusable, but machine drivers are not because they have specific hardware features that are non-reusable most of time. The so-called hardware characteristics refer to the link between DAIs; opening the amplifier through a GPIO; detecting the plug-in through a GPIO; using a clock such as MCLK/External
OSC as the reference clock source of I2; the codec module, and so on. In general, machine driver responsibilities include the following:
• Populating the struct snd_soc_dai_link structure with appropriate CPU
and codec DAIs
• Physical codec clock settings (if any) and codec initialization master/slave configurations (if any)
• Defining DAPM widgets to route through the physical codec internals and complete the DAPM path as needed
• Propagating the runtime sampling frequency to the individual codec drivers as needed
To put it together, we have the following flow:
1. The codec driver registers a component driver, a DAI driver, and their operation functions.
2. The platform driver registers a component driver, the PCM driver, the CPU DAI
driver, and their operation functions and sets playback and capture operations as applicable.
3. The machine layer creates the DAI link between the codec and CPU and registers the sound card and PCM devices.
Now that we have seen the development flow of a machine class driver, let's start with the first step, which consists of populating the DAI link.
## The DAI link
A DAI link is the logical representation of the link between the CPU and the codec DAIs.
It is represented from within the kernel using struct snd_soc_dai_link, defined as follows:
```c
struct snd_soc_dai_link {
const char *name;
const char *stream_name;
const char *cpu_name;
```
276 ALSA SoC Framework – Delving into the Machine Class Drivers struct device_node *cpu_of_node;
```c
const char *cpu_dai_name;
const char *codec_name;
struct device_node *codec_of_node;
const char *codec_dai_name;
struct snd_soc_dai_link_component *codecs;
unsigned int num_codecs;
const char *platform_name;
struct device_node *platform_of_node;
int id;
const struct snd_soc_pcm_stream *params;
unsigned int num_params;
unsigned int dai_fmt;
enum snd_soc_dpcm_trigger trigger[2];
```
/* codec/machine specific init - e.g. add machine controls */
```c
int (*init)(struct snd_soc_pcm_runtime *rtd);
```
/* machine stream operations */
```c
const struct snd_soc_ops *ops;
```
/* For unidirectional dai links */
```c
unsigned int playback_only:1;
unsigned int capture_only:1;
```
/* Keep DAI active over suspend */
```c
unsigned int ignore_suspend:1;
```
[...]
/* DPCM capture and Playback support */
```c
unsigned int dpcm_capture:1;
unsigned int dpcm_playback:1;
```
Introduction to machine class drivers 277 struct list_head list; /* DAI link list of the soc card */
```c
};
```
Important note
The full snd_soc_dai_link data structure definition can be found at https://elixir.bootlin.com/linux/v4.19/source/
include/sound/soc.h#L880.
This link is set up from within the machine driver. It should specify the cpu_dai, the codec_dai, and the platform that is used. Once set up, DAI links are fed to struct snd_soc_card, which represents a sound card. The following list describes the elements in the structure:
• name: This is chosen arbitrarily. It can be anything.
• codec_dai_name: This must match the snd_soc_dai_driver.name field from within the codec chip driver. Codecs may have one or more DAIs. Refer to the codec driver to identify the DAI names.
• cpu_dai_name: This must match the snd_soc_dai_driver.name field from within the CPU DAI driver.
• stream_name: This is the stream name of this link.
• init: This is the DAI link initialization callback. It is typically used to add DAI
link-specific widgets or other types of one-time settings.
• dai_fmt: This should be set with the supported format and clock configuration,
which should be coherent for both CPU and CODEC DAI drivers. Possible bit flags for this field are introduced later.
• ops: This field is of the struct snd_soc_ops type. It should be set with machine-level PCM operations of the DAI link: startup, hw_params, prepare,
trigger, hw_free, shutdown. This field is described in detail later.
• codec_name: If set, this should be the name of the codec driver, such as platform_driver.driver.name or i2c_driver.driver.name.
• codec_of_node: The device tree node associated with the codec.
• cpu_name: If set, this should be the name of the CPU DAI driver CPU.
• cpu_of_node: This is the device tree node associated with the CPU DAI.
278 ALSA SoC Framework – Delving into the Machine Class Drivers
• platform_name or platform_of_node: This is the name or DT node reference to the platform node, which provides DMA capabilities.
• playback_only and capture_only are to be used in case of unidirectional links, such as SPDIF. If this is an output only link (playback only), then playback_
only and capture_only must be set to true and false respectively. With an input-only link, the opposite values should be used.
In most cases, .cpu_of_node and .platform_of_node are the same, since the CPU
DAI driver and the DMA PCM driver are implemented by the same device. That being said, you must specify the link's codec either by name or by of_node, but not both. You must do the same for the CPU and platform. However, at least one of the CPU DAI name or the CPU device name/node must be specified. This could be summarized as follows:
```c
if (link->platform_name && link->platform_of_node)
==> Error if (link->cpu_name && link->cpu_of_node)
==> Eror if (!link->cpu_dai_name && !(link->cpu_name ||
link->cpu_of_node))
```
==> Error
There is a key point it is worth noting here. How do we reference the platform or CPU
node in the DAI link? We will answer this question later. Let's first consider the following two device nodes. The first one (ssi1) is the SSI cpu-dai node for the i.mx6 SoC. The second node (sgtl5000) represents the sgtl5000 codec chip:
```c
ssi1: ssi@2028000 {
dts
#sound-dai-cells = <0>;
compatible = "fsl,imx6q-ssi", "fsl,imx51-ssi";
reg = <0x02028000 0x4000>;
interrupts = <0 46 IRQ_TYPE_LEVEL_HIGH>;
clocks = <&clks IMX6QDL_CLK_SSI1_IPG>,
```
<&clks IMX6QDL_CLK_SSI1>;
```dts
clock-names = "ipg", "baud";
```
dmas = <&sdma 37 1 0>, <&sdma 38 1 0>;
```dts
dma-names = "rx", "tx";
```
fsl,fifo-depth = <15>;
```dts
status = "disabled";
```
Introduction to machine class drivers 279
```c
};
&i2c0{
sgtl5000: codec@0a {
dts
compatible = "fsl,sgtl5000";
#sound-dai-cells = <0>;
reg = <0x0a>;
clocks = <&audio_clock>;
```
VDDA-supply = <&reg_3p3v>;
VDDIO-supply = <&reg_3p3v>;
VDDD-supply = <&reg_1p5v>;
```c
};
};
```
Important note
```dts
In the SSI node, you can see the dma-names = "rx", "tx"; property,
```
which is the expected DMA channel names requested by the pcmdmaengine framework. This may also be an indication that the CPU DAI and platform
PCM are represented by the same node.
We will consider a system where an i.MX6 SoC is connected to an sgtl5000 audio codec.
It is common for machine drivers to grab either CPU or CODEC device tree nodes by referencing those nodes (their phandle actually) as its properties. This way, you can just use one of the OF helpers (such as of_parse_phandle()) to grab a reference on these nodes. The following is an example of a machine node that references both the codec and the platform by an OF node:
```c
sound {
dts
compatible = "fsl,imx51-babbage-sgtl5000",
```
"fsl,imx-audio-sgtl5000";
model = "imx51-babbage-sgtl5000";
ssi-controller = <&ssi1>;
audio-codec = <&sgtl5000>;
[...]
```c
};
```
280 ALSA SoC Framework – Delving into the Machine Class Drivers
In the preceding machine node, the codec and CPUE are passed by reference (their phandle) via the audio-codec and ssi-controller properties. These property names are not standardized as long as the machine driver is written by you (this is not true if you use the simple-card machine driver, for example, which expects some predefined names). In the machine driver, you'll see something like this:
```c
static int imx_sgtl5000_probe(struct platform_device *pdev)
{
struct device_node *np = pdev->dev.of_node;
struct device_node *ssi_np, *codec_np;
struct imx_sgtl5000_data *data = NULL;
int int_port, ext_port; int ret;
```
[...]
```c
ssi_np = of_parse_phandle(pdev->dev.of_node,
```
"ssi-controller", 0);
```c
codec_np = of_parse_phandle(pdev->dev.of_node,
```
"audio-codec", 0);
```c
if (!ssi_np || !codec_np) {
dev_err(&pdev->dev, "phandle missing or invalid\n");
```
ret = -EINVAL;
goto fail;
```c
}
data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
if (!data) {
```
ret = -ENOMEM;
goto fail;
```c
}
data->dai.name = "HiFi";
data->dai.stream_name = "HiFi";
data->dai.codec_dai_name = "sgtl5000";
data->dai.codec_of_node = codec_np;
data->dai.cpu_of_node = ssi_np;
data->dai.platform_of_node = ssi_np;
data->dai.init = &imx_sgtl5000_dai_init;
Machine routing consideration 281 data->card.dev = &pdev->dev;
```
[...]
```c
};
```
The preceding excerpt used of_parse_phandle() to obtain node references.
This is an excerpt from the imx_sgtl5000 machine, which is sound/soc/fsl/
imx-sgtl5000.c in the kernel sources. Now that we are familiar with the way the DAI
link should be handled, we can proceed to audio routing from within the machine driver in order to define the path the audio data should follow.
## Machine routing consideration
The machine driver can alter (or should I say append) the routes defined from within the codec. It has the last word on which codec pins must be used, for example.
## Codec pins
Codec pins are meant to be connected to the board connectors. The available codec pins are defined in the codec driver using the SND_SOC_DAPM_INPUT and SND_SOC_DAPM_
OUTPUT macros. These macros can be searched with the grep command in the codec driver in order to find the available PIN.
For example, the sgtl5000 codec driver defines the following output and input:
```c
static const struct snd_soc_dapm_widget sgtl5000_dapm_widgets[]
= {
SND_SOC_DAPM_INPUT("LINE_IN"),
SND_SOC_DAPM_INPUT("MIC_IN"),
SND_SOC_DAPM_OUTPUT("HP_OUT"),
SND_SOC_DAPM_OUTPUT("LINE_OUT"),
SND_SOC_DAPM_SUPPLY("Mic Bias", SGTL5000_CHIP_MIC_CTRL, 8,
```
0,
mic_bias_event,
```c
SND_SOC_DAPM_POST_PMU |
SND_SOC_DAPM_PRE_PMD),
```
[...]
```c
};
```
In the next sections, we will see how those pins are connected to the board.
282 ALSA SoC Framework – Delving into the Machine Class Drivers
## Board connectors
The board connectors are defined in the machine driver in the struct snd_soc_
dapm_widget part of the registered struct snd_soc_card. Most of the time,
these board connectors are virtual. They are just logical stickers that are connected with codec pins (which are real this time). The following lists the connectors defined by the imx-sgtl5000 machine driver, sound/soc/fsl/imx-sgtl5000.c (whose documentation is Documentation/devicetree/bindings/sound/imx-audiosgtl5000.txt), which has been given as an example so far:
```c
static const struct snd_soc_dapm_widget imx_sgtl5000_dapm_widgets[] = {
SND_SOC_DAPM_MIC("Mic Jack", NULL),
SND_SOC_DAPM_LINE("Line In Jack", NULL),
SND_SOC_DAPM_HP("Headphone Jack", NULL),
SND_SOC_DAPM_SPK("Line Out Jack", NULL),
SND_SOC_DAPM_SPK("Ext Spk", NULL),
};
```
The next section will connect this connector to the codec pins.
## Machine routing
The final machine routing can be either static (that is, populated from within the machine driver itself) or populated from within the device tree. Moreover, the machine driver can optionally extend the codec power map and become an audio power map of the audio subsystem by connecting to the supply widget that has been defined in the codec driver with either SND_SOC_DAPM_SUPPLY or SND_SOC_DAPM_REGULATOR_SUPPLY.
## Device tree routing
Let's take the node of our machine as an example, which connects an i.MX6 SoC to an sgtl5000 codec (this excerpt can be found in the machine documentation):
```c
sound {
dts
compatible = "fsl,imx51-babbage-sgtl5000",
```
"fsl,imx-audio-sgtl5000";
model = "imx51-babbage-sgtl5000";
ssi-controller = <&ssi1>;
audio-codec = <&sgtl5000>;
```dts
audio-routing = "MIC_IN", "Mic Jack",
```
Machine routing consideration 283
"Mic Jack", "Mic Bias",
"Headphone Jack", "HP_OUT";
[...]
```c
};
```
Routing from the device tree expects the audio map to be given in a certain format. That is, entries are parsed as pairs of strings, the first being the connection's sink, the second being the connection's source. Most of the time, these connections are materialized as codec pins and board connector mappings. Valid names for sources and sinks depend on the hardware binding, which is as follows:
• The codec: This should have defined the pins whose names are used here.
• The machine: This should have defined the connectors or jacks whose names are used here.
In the preceding excerpt, what do you notice there? We can see MIC_IN, HP_OUT, and
"Mic Bias", which are codec pins (coming from the codec driver), and "Mic Jack"
and "Headphone Jack", which have been defined in the machine driver as board connectors.
In order to use the route defined in the DT, the machine driver must call snd_soc_of_
parse_audio_routing(), which has the following prototype:
```c
int snd_soc_of_parse_card_name(struct snd_soc_card *card,
const char *prop);
```
In the preceding prototype, card represents the sound card for which the routes are parsed, and prop is the name of the property that contains the routes in the device tree node. This function returns 0 on success and a negative error code on error.
## Static routing
Static routing consists of defining a DAPM route map from the machine driver and assigning it to the sound card directly as follows:
```c
static const struct snd_soc_dapm_widget rk_dapm_widgets[] = {
SND_SOC_DAPM_HP("Headphone", NULL),
SND_SOC_DAPM_MIC("Headset Mic", NULL),
SND_SOC_DAPM_MIC("Int Mic", NULL),
SND_SOC_DAPM_SPK("Speaker", NULL),
};
```
284 ALSA SoC Framework – Delving into the Machine Class Drivers
/* Connection to the codec pin */
```c
static const struct snd_soc_dapm_route rk_audio_map[] = {
{"IN34", NULL, "Headset Mic"},
{"Headset Mic", NULL, "MICBIAS"},
{"DMICL", NULL, "Int Mic"},
{"Headphone", NULL, "HPL"},
{"Headphone", NULL, "HPR"},
{"Speaker", NULL, "SPKL"},
{"Speaker", NULL, "SPKR"},
};
static struct snd_soc_card snd_soc_card_rk = {
```
.name = "ROCKCHIP-I2S",
.owner = THIS_MODULE,
[...]
.dapm_widgets = rk_dapm_widgets,
.num_dapm_widgets = ARRAY_SIZE(rk_dapm_widgets),
.dapm_routes = rk_audio_map,
.num_dapm_routes = ARRAY_SIZE(rk_audio_map),
.controls = rk_mc_controls,
.num_controls = ARRAY_SIZE(rk_mc_controls),
```c
};
```
The preceding snippet is an excerpt from sound/soc/rockchip/rockchip_
rt5645.c. By using it this way, it is not necessary to use snd_soc_of_parse_
audio_routing(). However, a con of using this method is that it is not possible to change the route without recompiling the kernel. Next, we will be looking at clocking and formatting considerations.
## Clocking and formatting considerations
Before delving deeper into this section, let's spend some time on the snd_soc_dai_
```c
link->ops field. This field is of type struct snd_soc_ops, defined as follows:
struct snd_soc_ops {
int (*startup)(struct snd_pcm_substream *);
void (*shutdown)(struct snd_pcm_substream *);
int (*hw_params)(struct snd_pcm_substream *,
```
Clocking and formatting considerations 285 struct snd_pcm_hw_params *);
```c
int (*hw_free)(struct snd_pcm_substream *);
int (*prepare)(struct snd_pcm_substream *);
int (*trigger)(struct snd_pcm_substream *, int);
};
These callback fields in this structure should remind you of those defined in the snd_soc_dai_driver->ops field, which is of type struct snd_soc_dai_ops.
```
From within the DAI link, these callbacks represent the machine-level PCM operations of the DAI link, while in struct snd_soc_dai_driver, they are either codec-DAIspecific or CPU-DAI-specific.
startup() is invoked by ALSA when a PCM substream is opened (when someone has opened the capture/playback device), while hw_params() is called when setting up the audio stream. The machine driver may configure DAI link data format from within both of these callbacks. hw_params() offers the advantage of receiving stream parameters
(channel count, format, sample rate, and so forth).
The data format configuration should be consistent between the CPU DAI and the codec.
The ASoC core provides helper functions to change those configurations. They are as follows:
```c
int snd_soc_dai_set_fmt(struct snd_soc_dai *dai,
unsigned int fmt)
int snd_soc_dai_set_pll(struct snd_soc_dai *dai, int pll_id,
int source, unsigned int freq_in,
unsigned int freq_out)
int snd_soc_dai_set_sysclk(struct snd_soc_dai *dai, int clk_id,
unsigned int freq, int dir)
int snd_soc_dai_set_clkdiv(struct snd_soc_dai *dai,
int div_id, int div)
```
In the preceding helper list, snd_soc_dai_set_fmt sets the DAI format for things such as the clock master/slave relationship, audio format, and signal inversion; snd_soc_
dai_set_pll configures the clock PLL; snd_soc_dai_set_sysclk configures the clock source; and snd_soc_dai_set_clkdiv configures the clock divider. Each of these helpers will call the appropriate callback in the underlying DAI's driver ops. For example, calling snd_soc_dai_set_fmt() with the CPU DAI will invoke this CPU
```c
DAI's dai->driver->ops->set_fmt callback.
```
286 ALSA SoC Framework – Delving into the Machine Class Drivers
The following is the actual list of formats/flags that can be assigned either to DAIs or the dai_link.format field:
• Format: Configured through snd_soc_dai_set_fmt():
A) Clock master/slave:
a) SND_SOC_DAIFMT_CBM_CFM: The CPU is the slave for the bit clock and frame sync. This also means the codec is the master for both.
b) SND_SOC_DAIFMT_CBS_CFS. The CPU is the master for the bit clock and frame sync. This also means the codec is the slave for both.
c) SND_SOC_DAIFMT_CBM_CFS. The CPU is the slave for the bit clock and the master for frame sync. This also means the codec is the master for the former and the slave for the latter.
B) Audio format:
a) SND_SOC_DAIFMT_DSP_A: Frame syncing is 1 bit-clock wide, 1-bit delay.
b) SND_SOC_DAIFMT_DSP_B: Frame syncing is 1 bit-clock wide, 0-bit delay.
This format can be used for the TDM protocol.
c) SND_SOC_DAIFMT_I2S: Frame syncing is 1 audio word wide, 1-bit delay,
I2S mode.
d) SND_SOC_DAIFMT_RIGHT_J: Right justified mode.
e) SND_SOC_DAIFMT_LEFT_J: Left justified mode.
f) SND_SOC_DAIFMT_DSP_A: Frame syncing is 1 bit-clock wide,1-bit delay.
g) SND_SOC_DAIFMT_AC97: AC97 mode.
h) SND_SOC_DAIFMT_PDM: Pulse-density modulation.
i) SND_SOC_DAIFMT_DSP_B: Frame sync is 1 bit-clock wide, 1-bit delay.
C) Signal inversion:
a) SND_SOC_DAIFMT_NB_NF: Normal bit clock, normal frame sync. The CPU
transmitter shifts data out on the falling edge of the bit clock, the receiver samples data on the rising edge. The CPU frame sync generator starts the frame on the rising edge of the frame sync. This parameter is recommended for I2S on the CPU side.
Clocking and formatting considerations 287 b) SND_SOC_DAIFMT_NB_IF: Normal bit clock, inverted frame sync. The CPU
transmitter shifts data out on the falling edge of the bit clock, and the receiver samples data on the rising edge. The CPU frame sync generator starts the frame on the falling edge of the frame sync.
c) SND_SOC_DAIFMT_IB_NF: Inverted bit clock, normal frame sync. The CPU
transmitter shifts data out on the rising edge of the bit clock, and the receiver samples data on the falling edge. The CPU frame sync generator starts the frame on the rising edge of the frame sync.
d) SND_SOC_DAIFMT_IB_IF: Inverted bit clock, inverted frame sync. The CPU
transmitter shifts data out on the rising edge of the bit clock, and the receiver samples data on the falling edge. The CPU frame sync generator starts the frame on the falling edge of the frame sync. This configuration can be used for
PCM mode (such as Bluetooth or modem-based audio chips).
• Clock source: Configured through snd_soc_dai_set_sysclk(). The following are the direction parameters letting ALSA know which clock is used:
a) SND_SOC_CLOCK_IN: This means an internal clock is used for sysclock.
b) SND_SOC_CLOCK_OUT: This means an external clock is used for sysclock.
• Clock divider: Configured through snd_soc_dai_set_clkdiv().
```c
The preceding flags are the possible values that can be set in the dai_link->dai_fmt field or assigned to either codec or CPU DAIs from within the machine driver. The following is a typical hw_param() implementation:
static int foo_hw_params(struct snd_pcm_substream *substream,
struct snd_pcm_hw_params *params)
{
struct snd_soc_pcm_runtime *rtd = substream->private_data;
struct snd_soc_dai *codec_dai = rtd->codec_dai;
struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
unsigned int pll_out = 24000000;
int ret = 0;
```
/* set the cpu DAI configuration */
ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
```c
SND_SOC_DAIFMT_NB_NF |
SND_SOC_DAIFMT_CBM_CFM);
if (ret < 0)
```
288 ALSA SoC Framework – Delving into the Machine Class Drivers return ret;
/* set codec DAI configuration */
ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
```c
SND_SOC_DAIFMT_NB_NF |
SND_SOC_DAIFMT_CBM_CFM);
if (ret < 0)
return ret;
```
/* set the codec PLL */
ret = snd_soc_dai_set_pll(codec_dai, WM8994_FLL1, 0,
pll_out, params_rate(params) * 256);
```c
if (ret < 0)
return ret;
```
/* set the codec system clock */
ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_FLL1,
params_rate(params) * 256, SND_SOC_CLOCK_IN);
```c
if (ret < 0)
return ret;
return 0;
}
```
In the preceding implementation of the foo_hw_params() function, we can see how both codec and platform DAIs are configured, with both format and clock settings. Now we come to the last step of machine driver implementation, which consists of registering the audio sound card, which is the device through which audio operations on the system are performed.
## Sound card registration
A sound card is represented in the kernel as an instance of struct snd_soc_card,
defined as follows:
```c
struct snd_soc_card {
const char *name;
```
Sound card registration 289 struct module *owner;
[...]
/* callbacks */
```c
int (*set_bias_level)(struct snd_soc_card *,
struct snd_soc_dapm_context *dapm,
enum snd_soc_bias_level level);
int (*set_bias_level_post)(struct snd_soc_card *,
struct snd_soc_dapm_context *dapm,
enum snd_soc_bias_level level);
```
[...]
```c
/* CPU <--> Codec DAI links */
struct snd_soc_dai_link *dai_link;
int num_links;
const struct snd_kcontrol_new *controls;
int num_controls;
const struct snd_soc_dapm_widget *dapm_widgets;
int num_dapm_widgets;
const struct snd_soc_dapm_route *dapm_routes;
int num_dapm_routes;
const struct snd_soc_dapm_widget *of_dapm_widgets;
int num_of_dapm_widgets;
const struct snd_soc_dapm_route *of_dapm_routes;
int num_of_dapm_routes;
```
[...]
```c
};
```
For the sake of readability, only the relevant field has been listed, and the full definition can be found at https://elixir.bootlin.com/linux/v4.19/source/
include/sound/soc.h#L1010. That being said, the following list describes the fields we have listed:
• name is the name of the sound card.
• owner is the module owner for this sound card.
290 ALSA SoC Framework – Delving into the Machine Class Drivers
• dai_link is the array of DAI links this sound card is made of, and num_links specifies the number of entries in the array.
• controls is an array that contains the controls that are statically defined and set by the machine driver, and num_controls specifies the number of entries in the array.
• dapm_widgets is an array that contains the DAPM widgets that are statically defined and set by the machine driver, and num_dapm_widgets specifies the number of entries in the array.
• damp_routes is an array that contains the DAPM routes that are statically defined and set by the machine driver, and num_dapm_routes specifies the number of entries in the array.
• of_dapm_widgets represents the DAPM widgets fed from the DT (via snd_
soc_of_parse_audio_simple_widgets()), and num_of_dapm_widgets is the actual number of widget entries.
• of_dapm_routes represents the DAPM routes fed from the DT (via snd_soc_
of_parse_audio_routing()), and num_of_dapm_routes is the actual number of route entries.
After the sound card structure has been set up, it can be registered by the machine using the devm_snd_soc_register_card() method, whose prototype is as follows:
```c
int devm_snd_soc_register_card(struct device *dev,
struct snd_soc_card *card);
In the preceding prototype, dev represents the underlying device used to manage the card, and card is the actual sound card data structure that was set up previously. This function returns 0 on success. However, when this function is called, every component driver and DAI driver will be probed. As a result, the component_driver->probe()
and dai_driver->probe() methods will be invoked for both the CPU and CODEC.
```
Additionally, a new PCM device will be created for each successfully probed DAI link.
The following excerpts (from a Rockchip machine ASoC driver for boards using a MAX90809 CODEC, implemented in sound/soc/rockchip/rockchip_
max98090.c in kernel sources) will show the entire sound card creation, from widgets to routes, through DAI link configurations. Let's start by defining a widget and control for this machine, as well as the callback, which is used to configure the CPU and codec DAIs:
```c
static const struct snd_soc_dapm_widget rk_dapm_widgets[] = {
```
[...]
```c
};
Sound card registration 291 static const struct snd_soc_dapm_route rk_audio_map[] = {
```
[...]
```c
};
static const struct snd_kcontrol_new rk_mc_controls[] = {
SOC_DAPM_PIN_SWITCH("Headphone"),
SOC_DAPM_PIN_SWITCH("Headset Mic"),
SOC_DAPM_PIN_SWITCH("Int Mic"),
SOC_DAPM_PIN_SWITCH("Speaker"),
};
static const struct snd_soc_ops rk_aif1_ops = {
```
.hw_params = rk_aif1_hw_params,
```c
};
static struct snd_soc_dai_link rk_dailink = {
```
.name = "max98090",
.stream_name = "Audio",
.codec_dai_name = "HiFi",
.ops = &rk_aif1_ops,
/* set max98090 as slave */
.dai_fmt = SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF |
```c
SND_SOC_DAIFMT_CBS_CFS,
};
```
In the preceding excerpt, rk_aif1_hw_params can be seen in the original code implementation file. Now comes the data structure, which is used to build the sound card,
defined as follows:
```c
static struct snd_soc_card snd_soc_card_rk = {
```
.name = "ROCKCHIP-I2S",
.owner = THIS_MODULE,
.dai_link = &rk_dailink,
.num_links = 1,
.dapm_widgets = rk_dapm_widgets,
.num_dapm_widgets = ARRAY_SIZE(rk_dapm_widgets),
292 ALSA SoC Framework – Delving into the Machine Class Drivers
.dapm_routes = rk_audio_map,
.num_dapm_routes = ARRAY_SIZE(rk_audio_map),
.controls = rk_mc_controls,
.num_controls = ARRAY_SIZE(rk_mc_controls),
```c
};
```
This sound card is finally created in the driver probe method as follows:
```c
static int snd_rk_mc_probe(struct platform_device *pdev)
{
int ret = 0;
struct snd_soc_card *card = &snd_soc_card_rk;
struct device_node *np = pdev->dev.of_node;
```
[...]
```c
card->dev = &pdev->dev;
```
/* Assign codec, cpu and platform node */
rk_dailink.codec_of_node = of_parse_phandle(np,
"rockchip,audio-codec", 0);
rk_dailink.cpu_of_node = of_parse_phandle(np,
"rockchip,i2s-controller", 0);
rk_dailink.platform_of_node = rk_dailink.cpu_of_node;
[...]
ret = snd_soc_of_parse_card_name(card, "rockchip,model");
```c
ret = devm_snd_soc_register_card(&pdev->dev, card);
```
[...]
```c
}
```
Once again, the three preceding code blocks are excerpts from sound/soc/rockchip/
rockchip_max98090.c. So far, we have learned the main purpose of machine drivers,
which is to bind Codec and CPU drivers together and to define the audio path. That being said, there are cases when we might need even less code. Such cases concern boards where neither the CPU nor the Codecs need special hacks before being bound together. In this case, the ASoC framework provides the simple-card machine driver, introduced in the next section.
Leveraging the simple-card machine driver 293
## Leveraging the simple-card machine driver
There are cases when your board does not require any hacks from the Codec nor the CPU
DAI. The ASoC core provides the simple-audio machine driver, which can be used to describe a whole sound card from the DT. The following is an excerpt of such a node:
```c
sound {
dts
compatible ="simple-audio-card";
simple-audio-card,name ="VF610-Tower-Sound-Card";
simple-audio-card,format ="left_j";
simple-audio-card,bitclock-master = <&dailink0_master>;
simple-audio-card,frame-master = <&dailink0_master>;
simple-audio-card,widgets ="Microphone","Microphone Jack",
```
"Headphone","Headphone Jack",
"Speaker","External Speaker";
```dts
simple-audio-card,routing = "MIC_IN","Microphone Jack",
```
"Headphone Jack","HP_OUT",
"External Speaker","LINE_OUT";
```dts
simple-audio-card,cpu {
sound-dai = <&sh_fsi20>;
c
};
dts
dailink0_master: simple-audio-card,codec {
sound-dai = <&ak4648>;
clocks = <&osc>;
c
};
};
```
This is fully documented in Documentation/devicetree/bindings/sound/
simple-card.txt. In the preceding excerpt, we can see machine widgets and route maps being specified, as well as both the codec and the CPU nodes, which are referenced.
Now that we are familiar with the simple-card machine driver, we can leverage it and try as much as possible not to write our own machine driver. Having said that, there are situations where the codec device can't be dissociated, and this changes the way the machine should be written. Such audio devices are called codec-less sound cards, and we discuss them in the next section.
294 ALSA SoC Framework – Delving into the Machine Class Drivers
## Codec-less sound cards
There may be situations where digital audio data is sampled from an external system,
such as when using the SPDIF interface, and the data is therefore preformatted. In this case, the sound card registration is the same, but the ASoC core needs to be aware of this particular case.
With the output, the DAI link object's .capture_only field should be false,
while .playback_only should be true. The reverse should be done with the input.
Additionally, the machine driver must set the DAI link's codec_dai_name and codec_name to "snd-soc-dummy-dai" and "snd-soc-dummy" respectively.
This is, for example, the case for the imx-spdif machine driver (sound/soc/fsl/
imx-spdif.c), which contains the following excerpt:
```c
data->dai.name = "S/PDIF PCM";
data->dai.stream_name = "S/PDIF PCM";
data->dai.codecs->dai_name = "snd-soc-dummy-dai";
data->dai.codecs->name = "snd-soc-dummy";
data->dai.cpus->of_node = spdif_np;
data->dai.platforms->of_node = spdif_np;
data->dai.playback_only = true;
data->dai.capture_only = true;
if (of_property_read_bool(np, "spdif-out"))
data->dai.capture_only = false;
if (of_property_read_bool(np, "spdif-in"))
data->dai.playback_only = false;
if (data->dai.playback_only && data->dai.capture_only) {
dev_err(&pdev->dev, "no enabled S/PDIF DAI link\n");
```
goto end;
```c
}
```
You can find the binding documentation of this driver in Documentation/
devicetree/bindings/sound/imx-audio-spdif.txt. At the end of machine class driver study, we are done with the whole ASoC class driver development. In this machine class driver, in addition to bound CPU and Codec in the code, as well as providing a setup callback, we have seen how to avoid writing code by using the simple-card machine driver and implementing the rest in the device tree.
Summary 295
## Summary
In this chapter, we have gone through the architecture of ASoC machine class drivers,
which represents the last element in this ASoC series. We have learned how to bind platform and subdevice drivers, but also how to define routes for audio data.
In the next chapter, we will cover another Linux media subsystem, that is, V4L2, which is used to deal with video devices