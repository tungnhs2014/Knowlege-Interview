```bash
# Chapter 5 - ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
```
Audio is an analog phenomenon that can be produced in all sorts of ways. Voice and audio have been communication media since the beginning of humanity. Almost every kernel provides audio support to userspace applications as an interaction mechanism between computers and humans. To achieve this, the Linux kernel provides a set of APIs known as ALSA, which stands for Advanced Linux Sound Architecture.
218 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
ALSA was designed for desktop computers, not taking into account embedded world constraints. This added a lot of drawbacks when it came to dealing with embedded devices, such as the following:
• Strong coupling between codec and CPU code, leading to difficulties in porting and code duplication.
• No standard way to handle notifications about users' audio-related behavior.
In mobile scenarios, users' audio-related behaviors are frequent, so a special mechanism is needed.
• In the original ALSA architecture, power efficiency was not considered. But for embedded devices (most of the time, battery-backed), this is a key point, so there needs to be a mechanism.
This is where ASoC comes into the picture. The purpose of the ALSA System on Chip
(ASoC) layer is to provide better ALSA support for embedded processors and various codecs.
ASoC is a new architecture designed to solve the aforementioned problems and comes with the following advantages:
• An independent codec driver to reduce coupling with the CPU
• More convenient configuration of the audio data interface between the CPU and codec Dynamic Audio Power Management (DAPM), dynamically controlling power consumption (more information can be found here: https://www.
kernel.org/doc/html/latest/sound/soc/dapm.html)
• Reduced pop and click and increased platform-related controls
To achieve the aforementioned features, ASoC divides the embedded audio system into three reusable component drivers, namely the machine class, platform class, and codec class. Among them, the platform and codec classes are cross-platform, and the machine class is board-specific. In this chapter and the next chapter, we will walk through these component drivers, dealing with their respective data structures and how they are implemented.
Here, we will present the Linux ASoC driver architecture and the implementation of its different parts, looking specifically at the following:
• Introduction to ASoC
• Writing codec class drivers
• Writing platform class drivers
Technical requirements 219
## Technical requirements
• Strong knowledge of device tree concepts
• Familiarity with the Common Clock Framework (CCF) (discussed in Chapter 4,
Storming the Common Clock Framework)
• Familiarity with the regmap API
• Strong knowledge of the Linux kernel DMA framework
• Linux kernel v4.19.X sources, available at https://git.kernel.org/pub/
scm/linux/kernel/git/stable/linux.git/refs/tags
## Introduction to ASoC
From an architectural point of view, the ASoC subsystem elements and their relationship can be represented as follows:
Figure 5.1 – ASoC architecture
The preceding diagram summarizes the new ASoC architecture, in which the machine entity wraps both platform and codec entities.
In the ASoC implementation prior to kernel v4.18, there was strict separation between
SoC audio codec devices (represented by struct snd_soc_codec these days) and
SoC platform interfaces (represented by struct snd_soc_platform) and their respective digital audio interfaces. However, there was an increasing amount of similar code between codecs, platforms, and other components. This led to a new and generic approach, the concept of the component. All drivers have been moved over to this new generic component, platform code has been removed, and everything has been refactored so that nowadays, we only talk about struct snd_soc_component (which may refer to either a codec or a platform) and struct snd_soc_component_driver (which refers to their respective audio interface drivers).
220 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
Now that we have introduced the ASoC concept, we can get deeper into the details,
discussing digital audio interfaces first.
## ASoC Digital Audio Interface
The Digital Audio Interface (DAI) is the bus controller that actually carries audio data from one end (the SoC, for example) to the other end (the codec). ASoC currently supports most of the DAIs found on SoC controllers and portable audio codecs today,
such as AC97, I2S, PCM, S/PDIF, and TDM.
Important note
An I2S module supports six different modes, the most useful of which are I2S
and TDM.
## ASoC sub-elements
As we have seen earlier, an ASoC system is divided into three elements, each having a dedicated driver, described as follows:
• Platform: This refers to the audio DMA engine of the SoC (AKA the platform),
such as i.MX, Rockchip, and STM32. Platform class drivers can be subdivided into two parts:
--CPU DAI driver: In embedded systems, it usually refers to the audio bus controller (I2S, S/PDIF, AC97, and PCM bus controllers, sometimes integrated into a bigger module, the Serial Audio Interface (SAI)) of the CPU. It is responsible for transporting audio data from the bus Tx FIFO to the codec in the case of playback
(the recording is in opposite direction, from the codec to the bus Rx FIFO) . The platform driver defines DAIs and registers them with the ASoC core.
--PCM DMA driver: The PCM driver helps perform DMA operations by overriding the function pointers exposed by the struct snd_soc_component_driver
(see the struct snd_pcm_ops element) structure. The PCM driver is platformagnostic and interacts only with the SOC DMA engine upstream APIs. The DMA
engine then interacts with the platform-specific DMA driver to get the correct
DMA settings.
Introduction to ASoC 221
```c
It is responsible for carrying the audio data in the DMA buffer to the bus (or port)
```
Tx FIFO. The logic of this part is more complicated. The next sections will elaborate on it.
• Codec: Codec literally means codec, but there are many features in the chip.
Common ones are AIF, DAC, ADC, Mixer, PGA, Line-in, and Line-out. Some high-end codec chips also have an echo canceller, noise suppression, and other components. The codec is responsible for the conversion of analog signals from sound sources into digital signals that the processor can operate (for capture operations) or the conversion of digital signals from sound sources (the CPU)
to analog signals that humans can recognize in the case of playback. If necessary,
it makes the corresponding adjustment to the audio signal and controls the path between the audio signals since there may be a different flow path for each audio signal in the chip.
• Machine: This is the system-level representation (the board actually), linking both audio interfaces (cpu_dai and codec_dai). This link is abstracted in the kernel by instances of struct snd_soc_dai_link. After configuring the link, the machine driver registers (by means of devm_snd_soc_register_card())
a struct snd_soc_card object, which is the Linux kernel abstraction of a sound card. Whereas platform and codec drivers are generally reusable, the machine has its specific hardware features that are almost non-reusable. The so-called hardware characteristics refer to the link between DAIs; an open amplifier through a GPIO; detecting the plugin through a GPIO; using a clock such as
MCLK/eternal OSC as the reference clock source of the I2S CODEC module,
and so on.
222 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
From the preceding description, we can produce the following ASoC scheme and its relationships:
Figure 5.2 – Linux audio layers and relationships
The preceding diagram is a snapshot of the interaction between Linux kernel audio components. Now that we are familiar with ASoC concepts, we can move on to its first device driver class, which deals with codec devices.
## Writing codec class drivers
In order to be coupled together, machine, platform, and codec entities need dedicated drivers. The codec class driver is the most basic. It implements code that should leverage the codec device and expose its hardware properties so that user space tools such as amixer can play with it. The codec class driver is and should be platform-independent.
The same codec driver can be used whatever the platform. Since it targets a specific codec,
it should contain audio controls, audio interface capabilities, a codec DAPM definition,
and I/O functions. Each codec driver must fulfill the following specifications:
• Provide an interface to other modules by defining DAI and PCM configurations.
• Provide codec control IO hooks (using I2C or SPI or both APIs).
Writing codec class drivers 223
• Expose additional kcontrols (kernel controls) as needed for userspace utilities to dynamically control module behavior.
• Optionally, define DAPM widgets and establish DAPM routes for dynamic power switching and also provide DAC digital mute control.
The codec driver includes the codec device (component, actually) itself and DAIs components, which are used during the binding with the platform. It is platformindependent. By means of devm_snd_soc_register_component(), the codec driver registers a struct snd_soc_component_driver object (which is actually the instance of the codec driver that contains pointers to the codec's routes, widgets,
controls, and a set of codec-related function callbacks) along with one or more struct snd_soc_dai_driver, which is an instance of the codec DAI driver that may contain an audio stream, for example:
```c
struct snd_soc_component_driver {
const char *name;
```
/* Default control and setup, added after probe() is run */
```c
const struct snd_kcontrol_new *controls;
unsigned int num_controls;
const struct snd_soc_dapm_widget *dapm_widgets;
unsigned int num_dapm_widgets;
const struct snd_soc_dapm_route *dapm_routes;
unsigned int num_dapm_routes;
int (*probe)(struct snd_soc_component *);
void (*remove)(struct snd_soc_component *);
int (*suspend)(struct snd_soc_component *);
int (*resume)(struct snd_soc_component *);
unsigned int (*read)(struct snd_soc_component *,
unsigned int);
int (*write)(struct snd_soc_component *, unsigned int,
unsigned int);
```
/* pcm creation and destruction */
```c
int (*pcm_new)(struct snd_soc_pcm_runtime *);
void (*pcm_free)(struct snd_pcm *);
```
/* component wide operations */
```c
int (*set_sysclk)(struct snd_soc_component *component,
```
224 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers int clk_id,
```c
int source, unsigned int freq, int dir);
int (*set_pll)(struct snd_soc_component *component,
int pll_id, int source,
unsigned int freq_in,
unsigned int freq_out);
int (*set_jack)(struct snd_soc_component *component,
struct snd_soc_jack *jack, void *data);
```
[...]
```c
const struct snd_pcm_ops *ops;
```
[...]
```c
unsigned int non_legacy_dai_naming:1;
};
```
This structure must also be provided by the platform driver. However, in the ASoC core,
the only element in this structure that is mandatory is name, since it is used for matching the component. The following are the meanings of the elements in the structure:
• name: The name of this component is mandatory for both codec and platform.
Other elements in the structure may not be needed on the platform side.
• probe: Component driver probe function, executed (in order to complete the component initialization if necessary) when this component driver is probed by the machine driver (actually, when the machine driver registers a card made of this component with the ASoC core: see snd_soc_instantiate_card()).
• remove: When the component driver is unregistered (which occurs when the sound card to which this component driver is bound is unregistered).
• suspend and resume: Power management callbacks, invoked during system suspend or resume stages.
• controls: Controls interface pointers, such as controlling volume adjustment,
channel selection, and so on, mostly for codecs.
• set_pll: Sets function pointers for phase-locked loops.
• read: The function to read the codec register.
• write: The function to write into codec registers.
• num_controls: The number of controls in controls, that is, the number of snd_
kcontrol_new objects.
• dapm_widgets: The dapm widget pointer.
Writing codec class drivers 225
• num_dapm_widgets : The number of dapm part pointers.
• dapm_routes: dapm route pointers.
• num_dapm_routes : The number of dapm route pointers.
• set_sysclk: Sets clock function pointers.
• ops: Platform DMA-related callbacks, only necessary when providing this structure from within the platform driver (ALSA only); however, with ASoC, this field is set up by the ASoC core for you by means of a dedicated ASoC DMA-related API when using the generic PCM DMA engine framework.
So far, we have introduced the struct snd_soc_component_driver data structure in the context of the codec class driver. Remember, this structure abstracts both codec and platform devices and will be discussed in the platform driver context as well. Still, in the context of the codec class driver, we will need to discuss the struct snd_soc_dai_
driver data structure, which, along with struct snd_soc_component_driver,
abstracts a codec or a platform device, along with its DAI driver.
## Codec DAI and PCM (AKA DSP) configurations
```c
This section is rather generic and should probably be named DAI and PCM (AKA DSP)
```
configurations, without the word Codec. Each codec (or should I say "component")
driver must expose the DAIs of the codec (component) as well as their capabilities and operations. This is done by filling and registering as many instances of struct snd_
soc_dai_driver as there are DAIs on the codec, and that must be exported using the devm_snd_soc_register_component() API. This function also takes a pointer to a struct snd_soc_component_driver, which is the component driver to which the provided DAI drivers will be bound and will be exported (actually, inserted into the ASoC global list of components, component_list, defined in sound/soc/
soc-core.c) so that it can be registered with the core by the machine driver prior to registering the sound card. This structure covers the clocking, formatting, and ALSA
operations for each interface and is defined in include/sound/soc-dai.h as follows:
```c
struct snd_soc_dai_driver {
```
/* DAI description */
```c
const char *name;
```
/* DAI driver callbacks */
```c
int (*probe)(struct snd_soc_dai *dai);
int (*remove)(struct snd_soc_dai *dai);
int (*suspend)(struct snd_soc_dai *dai);
```
226 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers int (*resume)(struct snd_soc_dai *dai);
[...]
/* ops */
```c
const struct snd_soc_dai_ops *ops;
```
/* DAI capabilities */
```c
struct snd_soc_pcm_stream capture;
struct snd_soc_pcm_stream playback;
unsigned int symmetric_rates:1;
unsigned int symmetric_channels:1;
unsigned int symmetric_samplebits:1;
```
[...]
```c
};
```
In the preceding block, only the main elements of the structure have been enumerated for the sake of readability. The following are their meanings:
• name: This is the name of the DAI interface.
• probe: The DAI driver probe function, executed when the component driver to which this DAI driver belongs is probed by the machine driver (actually, when the machine driver registers a card with the ASoC core).
• remove: Invoked when the component driver to which this DAI driver belongs is unregistered.
• suspend and resume: Power management callbacks.
• ops: Points to the struct snd_soc_dai_ops structure, which provides callbacks for configuring and controlling the DAI.
• capture: Points to a struct snd_soc_pcm_stream structure, which represents the hardware parameters for audio capture. This member describes the number of channels, bit rate, data format, and so on supported during audio capture. It does not need to be initialized if the capture feature is not needed.
• playback: The hardware parameter for audio playback. This member describes the number of channels, bit rate, data format, and so on supported during playback.
It does not need to be initialized if the audio playback feature is not needed.
Writing codec class drivers 227
Actually, codec and platform drivers must register this structure for every DAI they have.
This is what makes this section generic. It is used later by the machine driver to build the link between the codec and the SoC. However, there are other data structures that need to be granted some study time in order to understand how the whole configuration is done: these are struct snd_soc_pcm_stream and struct snd_soc_dai_ops,
described in the next sections.
## DAI operations
The operations are abstracted by instances of the struct snd_soc_dai_ops structure. This structure contains a set of callbacks that relate to different events regarding the PCM interface (that is, it's most probable that you'll want to prepare the device in some manner before audio transfer starts, so you would put the code to do this into your prepare callback) or callbacks that relate the DAI clock and format configurations. This structure is defined as follows:
```c
struct snd_soc_dai_ops {
int (*set_sysclk)(struct snd_soc_dai *dai, int clk_id,
unsigned int freq, int dir);
int (*set_pll)(struct snd_soc_dai *dai, int pll_id,
int source,
unsigned int freq_in,
unsigned int freq_out);
int (*set_clkdiv)(struct snd_soc_dai *dai, int div_id,
int div);
int (*set_bclk_ratio)(struct snd_soc_dai *dai,
unsigned int ratio);
int (*set_fmt)(struct snd_soc_dai *dai, unsigned int fmt);
int (*xlate_tdm_slot_mask)(unsigned int slots,
unsigned int *tx_mask,
unsigned int *rx_mask);
int (*set_tdm_slot)(struct snd_soc_dai *dai,
unsigned int tx_mask,
unsigned int rx_mask,
int slots, int slot_width);
int (*set_channel_map)(struct snd_soc_dai *dai,
unsigned int tx_num,
unsigned int *tx_slot,
unsigned int rx_num,
```
228 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers unsigned int *rx_slot);
```c
int (*get_channel_map)(struct snd_soc_dai *dai,
unsigned int *tx_num,
unsigned int *tx_slot,
unsigned int *rx_num,
unsigned int *rx_slot);
int (*set_tristate)(struct snd_soc_dai *dai, int tristate);
int (*set_sdw_stream)(struct snd_soc_dai *dai,
void *stream,
int direction);
int (*digital_mute)(struct snd_soc_dai *dai, int mute);
int (*mute_stream)(struct snd_soc_dai *dai, int mute,
int stream);
int (*startup)(struct snd_pcm_substream *,
struct snd_soc_dai *);
void (*shutdown)(struct snd_pcm_substream *,
struct snd_soc_dai *);
int (*hw_params)(struct snd_pcm_substream *,
struct snd_pcm_hw_params *,
struct snd_soc_dai *);
int (*hw_free)(struct snd_pcm_substream *,
struct snd_soc_dai *);
int (*prepare)(struct snd_pcm_substream *,
struct snd_soc_dai *);
int (*trigger)(struct snd_pcm_substream *, int,
struct snd_soc_dai *);
};
```
Callback functions in this structure can be basically divided into three classes, and the driver can implement some of them according to the actual situation.
Writing codec class drivers 229
The first class gathers clock configuration callbacks, usually called by the machine driver.
These callbacks are as follows:
• set_sysclk sets the main clock of the DAI. If implemented, this callback should derive the best DAI bit and frame clocks from the system or master clock. The machine driver can use the snd_soc_dai_set_sysclk() API on cpu_dai and/or codec_dai in order to invoke this callback.
• set_pll sets the PLL parameters. If implemented, this callback should configure and enable PLL to generate the output clock based on the input clock. The machine driver can use the snd_soc_dai_set_pll() API on cpu_dai and/or codec_
dai in order to invoke this callback.
• set_clkdiv sets the clock division factor. The API from the machine driver to invoke this callback is snd_soc_dai_set_clkdiv().
The second callback class is the format configuration callbacks of the DAI, usually called by the machine driver. These callbacks are as follows:
• set_fmt sets the format of the DAI. The machine driver can use the snd_soc_
dai_set_fmt() API to invoke this callback (on either CPU or codec DAIs,
or both) in order to configure the DAI hardware audio format.
• set_tdm_slot: If the DAI supports time-division multiplexing (TDM), it is used to set the TDM slot. The machine driver API to invoke this callback is snd_
soc_dai_set_tdm_slot(), in order to configure the specified DAI for TDM
operations.
• set_channel_map: Channel TDM mapping settings. The machine driver invokes this callback for the specified DAI using the snd_soc_dai_set_channel_
map() API.
• set_tristate: Sets the state of the DAI pin, which is needed when using the same pin in parallel with other DAIs. It is invoked from the machine driver by using the snd_soc_dai_set_tristate() API.
The last callback class is the normal standard frontend witch gathers PCM correction operations usually invoked by the ASoC core. The concerned callbacks are as follows:
• startup: Invoked by ALSA when a PCM sub-stream is opened (when someone has opened the capture/playback device (At device file open for example).
• shutdown: This callback should implement code that will undo what has been done during startup.
230 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
• hw_params: This is called when setting up the audio stream. The struct snd_
pcm_hw_params contains the audio characteristics.
• hw_free: Should undo what has been done in hw_params.
• prepare: This is called when PCM is ready. Please see the following PCM
common state change flow in order to understand when this callback is called.
DMA transfer parameters are set according to channels, buffer_bytes, and so on, which are related to the specific hardware platform.
• trigger: This is called when PCM starts, stops, and pauses. The int argument in this callback is a command that may be one of SNDRV_PCM_TRIGGER_START,
```c
SNDRV_PCM_TRIGGER_RESUME, or SNDRV_PCM_TRIGGER_PAUSE_RELEASE
```
according to the event. Drivers can use switch...case in order to iterate over events.
• (Optional) digital_mute: An anti-pop sound called by the ASoC core.
It may be invoked by the core when the system is being suspended, for example.
In order to figure out how the preceding callbacks could be invoked by the core, let's have a look at the PCM common state changes flow:
```c
1. First started: off --> standby --> prepare --> on
2. Stop: on --> prepare --> standby
3. Resume: standby --> prepare --> on
```
Each state in the preceding flow will invoke a callback. All this being said, we can delve into hardware configuration data structures, either for capture or playback operations.
## Capture and playback hardware configuration
During capture or playback operations, the DAI setting (such as the channel number) and capabilities should be set in order to allow the underlying PCM stream to be configured.
You achieve this by filling an instance of struct snd_soc_pcm_stream defined as follows for each operation and for each DAI, in both codec and platform drivers:
```c
struct snd_soc_pcm_stream {
const char *stream_name;
```
u64 formats;
```c
unsigned int rates;
unsigned int rate_min;
unsigned int rate_max;
unsigned int channels_min;
```
Writing codec class drivers 231 unsigned int channels_max;
```c
unsigned int sig_bits;
};
```
The main members of this structure can be described as follows:
• stream_name: The name of the stream, either "Playback" or "Capture".
• formats: A collection of supported data formats (valid values are defined in include/sound/pcm.h prefixed with SNDRV_PCM_FMTBIT_), such as
```c
SNDRV_PCM_FMTBIT_S16_LE or SNDRV_PCM_FMTBIT_S24_LE. If multiple formats are supported, each format can be combined, such as SNDRV_PCM_
```
FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE.
• rates: A set of supported sampling rates (prefixed with SNDRV_PCM_RATE_
and the whole valid values are defined in include/sound/pcm.h), such as
```c
SNDRV_PCM_RATE_44100 or SNDRV_PCM_RATE_48000. If multiple sampling rates are supported, each sampling rate can be increased, such as SNDRV_PCM_
```
RATE_48000 | SNDRV_PCM_RATE_88200.
• rate_min: The minimum supported sample rate.
• rate_max: The maximum supported sample rate.
• channels_min: The minimum supported number of channels.
## The concept of controls
It is common for codec drivers to expose some codec properties that can be altered from the userspace. These are codec controls. When the codec is initialized, all the defined audio controls are registered to the ALSA core. The structure of an audio control is struct snd_kcontrol_new defined as include/sound/control.h.
In addition to the DAI bus, codec devices are equipped with a control bus, an I2C
or SPI bus most of time. In order to not bother about each codec driver to implement its control access routines, the codec control I/O has been standardized. This was where the regmap API originated. You can use regmap to abstract the control interface so that the codec driver does not have to worry about what the current control method is. The audio codec frontend is implemented in sound/soc/soc-io.c. This relies on the regmap
API, which has already been discussed, in Chapter 2, Leveraging the Regmap API and
Simplifying the Code.
232 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
The codec driver then needs to provide read and write interfaces in order to access the underlying codec registers. These callbacks need to be set in the .read and .write fields of the codec component driver, the struct snd_soc_component_driver.
The following are the high-level APIs that can be used to access the component registers:
```c
int snd_soc_component_write(struct snd_soc_component *component,
unsigned int reg, unsigned int val)
int snd_soc_component_read(struct snd_soc_component *component,
unsigned int reg,
unsigned int *val)
int snd_soc_component_update_bits(struct snd_soc_component *component,
unsigned int reg,
unsigned int mask,
unsigned int val)
int snd_soc_component_test_bits(struct snd_soc_component *component,
unsigned int reg,
unsigned int mask,
unsigned int value)
```
Each helper in the preceding is self-descriptive. Before we delve into control implementation, notice that the control framework is made of several types:
• A simple switch control, which is a single logical value in a register
• A stereo control, which is the stereo version of the previous simple switch control,
controlling two logical values at the same time in the register
• A mixer control, which is a combination of multiple simple controls and whose output is the mix of its inputs
• MUX controls – the same as the aforementioned mixer control, but selecting one among many
From within ALSA, a control is abstracted by means of the struct snd_kcontrol_
new structure, defined as follows:
```c
struct snd_kcontrol_new {
snd_ctl_elem_iface_t iface;
unsigned int device;
unsigned int subdevice;
```
Writing codec class drivers 233 const unsigned char *name;
```c
unsigned int index;
unsigned int access;
unsigned int count;
snd_kcontrol_info_t *info;
snd_kcontrol_get_t *get;
snd_kcontrol_put_t *put;
union {
snd_kcontrol_tlv_rw_t *c;
const unsigned int *p;
} tlv;
```
[...]
```c
};
```
The following are the descriptions of the fields in the aforementioned data structure:
• The iface field specifies the control type. It is of type snd_ctl_elem_iface_t,
which is an enum of SNDRV_CTL_ELEM_IFACE_XXX, where XXX can be
MIXER, PCM, and so on. The list of possible values can be found here: https://
elixir.bootlin.com/linux/v4.19/source/include/uapi/sound/
asound.h#L848. If the control is closely associated with a specific device on the sound card, you can use HWDEP, PCM, RAWMIDI, TIMER, or SEQUENCER, and specify the device number with the device and subdevice (which is the substream in the device) fields.
• name is the name of the control. This field has an important role that allows controls to be categorized by name. ALSA has somehow standardized some control names, which we discuss in detail in the Control naming convention section.
• The index field is used to save the number of controls on the card. If there is more than one codec on the sound card, and each codec has a control with the same name, then we can distinguish these controls by index. When index is 0,
this differentiation strategy can be ignored.
• access contains the access right of the control in the form SNDRV_CTL_ELEM_
ACCESS_XXX. Each bit represents an access type that can be combined with multiple OR operations. XXX can be either READ, WRITE, or VOLATILE, and so on. Possible bitmasks can be found here: https://elixir.bootlin.com/
linux/v4.19/source/include/uapi/sound/asound.h#L858.
234 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
• get is the callback function used to read the current value of the control and return it to the application in the userspace.
• put is the callback function used to set the application's control value to the control.
• The info callback function is used to get the details about the control.
• The tlv field provides metadata for the control.
## Control naming convention
ALSA expects controls to be named in a certain way. In order to achieve this, ALSA has predefined some commonly used sources (such as Master, PCM, CD, Line, and so on),
directions (representing the data flow of the control, such as Playback, Capture, Bypass,
Bypass Capture, and so on), and functions (according to the function of the control, such as Switch, Volume, Route, and so on). Do note that no definition of the direction means that the control is two-way (playback and capture).
You can refer to the following link for more details on ALSA control naming: https://
www.kernel.org/doc/html/v4.19/sound/designs/control-names.html.
## Control metadata
There are mixer controls that need to provide information in decibels (dB). We can use the DECLARE_TLV_xxx macro to define some variables containing this information,
then point the control tlv.p field to these variables, and finally add the SNDRV_CTL_
ELEM_ACCESS_TLV_READ flag to the access field.
TLV literally means Type-Length-Value (or Tag-Length-Value) and is an encoding scheme. This has been adopted by ALSA in order to define dB range/scale containers.
For example, DECLARE_TLV_DB_SCALE will define information about a mixer control where each step in the control's value changes the dB value by a constant dB amount.
Let's take the following example:
```c
static DECLARE_TLV_DB_SCALE(db_scale_my_control, -4050, 150,
```
0);
According to the definition of this macro in include/sound/tlv.h, the preceding example could be expanded into the following:
```c
static struct snd_kcontrol_new my_control devinitdata = {
```
[...]
.access =
```c
SNDRV_CTL_ELEM_ACCESS_READWRITE |
SNDRV_CTL_ELEM_ACCESS_TLV_READ,
```
Writing codec class drivers 235
[...]
.tlv.p = db_scale_my_control,
```c
};
```
The first parameter of the macro represents the name of the variable to be defined; the second one represents the minimum value this control can accept, in units of 0.01 dB.
The third parameter is the step size of the change, also in steps of 0.01 dB. If a mute operation is performed when the control is at the minimum value, the fourth parameter needs to be set to 1. Please have a look at include/sound/tlv.h to see the available macros.
Upon sound card registration, the snd_ctl_dev_register() function is called in order to save relevant information about the control device and make it available to users.
## Defining kcontrols kcontrols are used by the ASoC core to export audio controls (such as switch, volume,
*MUX…) to the userspace. This means, for example, when a userspace application like
PulseAudio switches off headphones or switches on speakers when no headphones are plugged in, the action is handled in the kernel by kcontrols. Normal kcontrols are not involved in power management (DAPM). They are specially meant to control non-power-management-based elements such as volume level, gain level, and so on.
Once controls have been set up using the appropriate macros, they must be registered with the system control list using the snd_soc_add_component_controls()
method, whose prototype is as follows:
```c
int snd_soc_add_component_controls(
struct snd_soc_component *component,
const struct snd_kcontrol_new *controls,
unsigned int num_controls);
```
In the preceding prototype, component is the component you add the controls for,
controls is the array of controls to add, and num_controls is the number of entries in the array that need to be added.
In order to see how simple this API is, let's consider the following sample, which defines some controls:
```c
static const DECLARE_TLV_DB_SCALE(dac_tlv, -12750, 50, 1);
static const DECLARE_TLV_DB_SCALE(out_tlv, -12100, 100, 1);
static const DECLARE_TLV_DB_SCALE(bypass_tlv, -2100, 300, 0);
236 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers static const struct snd_kcontrol_new wm8960_snd_controls[] = {
```
[...]
```c
SOC_DOUBLE_R_TLV("Playback Volume", WM8960_LDAC,
```
WM8960_RDAC, 0,
255, 0, dac_tlv),
```c
SOC_DOUBLE_R_TLV("Headphone Playback Volume", WM8960_LOUT1,
```
WM8960_ROUT1, 0, 127, 0, out_tlv),
```c
SOC_DOUBLE_R("Headphone Playback ZC Switch", WM8960_LOUT1,
```
WM8960_ROUT1, 7, 1, 0),
```c
SOC_DOUBLE_R_TLV("Speaker Playback Volume", WM8960_LOUT2,
```
WM8960_ROUT2, 0, 127, 0, out_tlv),
```c
SOC_DOUBLE_R("Speaker Playback ZC Switch", WM8960_LOUT2,
```
WM8960_ROUT2, 7, 1, 0),
```c
SOC_SINGLE("Speaker DC Volume", WM8960_CLASSD3, 3, 5, 0),
SOC_SINGLE("Speaker AC Volume", WM8960_CLASSD3, 0, 5, 0),
SOC_ENUM("DAC Polarity", wm8960_enum[1]),
SOC_SINGLE_BOOL_EXT("DAC Deemphasis Switch", 0,
```
wm8960_get_deemph,
wm8960_put_deemph),
[...]
```c
SOC_SINGLE("Noise Gate Threshold", WM8960_NOISEG, 3, 31,
```
0),
```c
SOC_SINGLE("Noise Gate Switch", WM8960_NOISEG, 0, 1, 0),
SOC_DOUBLE_R_TLV("ADC PCM Capture Volume", WM8960_LADC,
```
WM8960_RADC, 0, 255, 0, adc_tlv),
```c
SOC_SINGLE_TLV("Left Output Mixer Boost Bypass Volume",
```
WM8960_BYPASS1, 4, 7, 1, bypass_tlv),
```c
};
```
The corresponding code that would register the preceding controls is as follows:
```c
snd_soc_add_component_controls(component, wm8960_snd_controls,
ARRAY_SIZE(wm8960_snd_controls));
```
The following are ways to define commonly used controls with these preset macro definitions.
Writing codec class drivers 237
```c
SOC_SINGLE(xname, reg, shift, max, invert)
```
To set up a simple switch, we can use SOC_SINGLE. This is the simplest control:
```c
#define SOC_SINGLE(xname, reg, shift, max, invert) \
{ .iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
```
.info = snd_soc_info_volsw, .get = snd_soc_get_volsw,\
.put = snd_soc_put_volsw, \
.private_value = SOC_SINGLE_VALUE(reg, shift, max, invert) }
This type of control has only one setting and is generally used for component switches.
Descriptions of the parameters defined by the macro are as follows:
• xname: The name of the control.
• reg: The register address corresponding to the control.
• Shift: The offset control bit (from where to apply the change) for this control in the register reg.
• max: The range of values set by the control. Generally speaking, if the control bit has only 1 bit, then max=1, because the possible values are only 0 and 1.
• invert: Whether the set value is inverted.
Let's study the following example:
```c
SOC_SINGLE("PCM Playback -6dB Switch", WM8960_DACCTL1, 7, 1,
```
0),
In the previous example, PCM Playback -6dB Switch is the name of the control.
WM8960_DACCTL1 (defined in wm8960.h) is the address of the register in the codec (the
WM8960 chip), which allows you to control this switch:
• 7 means the 7th bit in the DACCTL1 register is used to enable/disable the DAC 6dB
attenuation.
• 1 means there is only one enable or disable option.
• 0 means the value you set is not inverted.
238 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
```c
SOC_SINGLE_TLV(xname, reg, shift, max, invert, tlv_array)
```
This macro sets up a switch with levels. It is an extension of SOC_SINGLE that is used to define controls that have gain control, such as volume controls, EQ equalizers, and so on. In this example, the left input volume control is from 000000 (-17.25 dB) to
111111(+30 dB). Each step is 0.75 dB, meaning a total of 63 steps:
```c
SOC_SINGLE_TLV("Input Volume of LINPUT1",
```
WM8960_LINVOL, 0, 63, 0, in_tlv),
The scale of in_tlv (which represents the control metadata) is declared like this:
```c
static const DECLARE_TLV_DB_SCALE(in_tlv, -1725, 75, 0);
```
In the preceding, -1725 means the control scale starts from -17.25dB. 75 means each step is 0.75dB, and 0 means the step starts from 0. For some volume control cases, the first step is "mute" and the step starts from 1. Thus the 0 in the preceding code should be replaced by 1.
```c
SOC_DOUBLE_R(xname, reg_left, reg_right, xshift, xmax, xinvert)
SOC_DOUBLE_R is a stereo version of SOC_SINGLE. The difference is that SOC_SINGLE
```
only controls one variable, while SOC_DOUBLE can control two similar variables in one register at the same time. We may use this to control the left and right channels at the same time.
Because there is one more channel, the parameter has a shift value corresponding to it.
The following is an example:
```c
SOC_DOUBLE_R("Headphone ZC Switch", WM8960_LOUT1,
```
WM8960_ROUT1, 7, 1, 0),
```c
SOC_DOUBLE_R_TLV(xname, reg_left, reg_right, xshift, xmax, xinvert, tlv_
```
array)
```c
SOC_DOUBLE_R_TLV is the stereo version of SOC_SINGLE_TLV. The following is an example of its usage:
SOC_DOUBLE_R_TLV("PCM DAC Playback Volume", WM8960_LDAC,
```
WM8960_RDAC, 0, 255, 0, dac_tlv),
Writing codec class drivers 239
## The mixer control
The mixer control is used for routing the control of audio channels. It consists of multiple inputs and one output. Multiple inputs can be freely mixed together to form a mixed output:
```c
static const struct snd_kcontrol_new left_speaker_mixer[] = {
SOC_SINGLE("Input Switch", WM8993_SPEAKER_MIXER, 7, 1, 0),
SOC_SINGLE("IN1LP Switch", WM8993_SPEAKER_MIXER, 5, 1, 0),
SOC_SINGLE("Output Switch", WM8993_SPEAKER_MIXER, 3, 1, 0),
SOC_SINGLE("DAC Switch", WM8993_SPEAKER_MIXER, 6, 1, 0),
};
```
The preceding mixer uses the third, fifth, sixth, and seventh bits of the WM8993_
SPEAKER_MIXER register to control the opening and closing of the four inputs.
```c
SOC_ENUM_SINGLE(xreg, xshift, xmax, xtexts)
```
This macro defines a single enumerated control, where xreg is the register to modify to apply settings, xshift is the control bit(s) offset in the register, xmask is the control bit(s) size, and xtexts is a pointer to the array of strings that describe each setting. This is used when the control options are some texts.
As an example, we can set up the array for the texts as follows:
```c
static const char *aif_text[] = {
```
"Left" , "Right"
```c
};
```
And then define the enum as follows:
```c
static const struct soc_enum aifinl_enum =
SOC_ENUM_SINGLE(WM8993_AUDIO_INTERFACE_2, 15, 2, aif_text);
```
Now we are done with the concept of controls, which are used to change the properties of an audio device, we will learn how to leverage it and play with the power properties of an audio device.
240 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
## The concept of DAPM
Modern sound cards consist of many independent discrete components. Each component has functional units that can be powered independently. The thing is, embedded systems are, most of the time, battery-powered and require the lowest power mode. Managing power domain dependencies by hand could be tedious and error-prone. Dynamic Audio
Power Management (DAPM) targets the lowest use of power at all times in the audio subsystem. DAPM is to be used for things for which there is power control and can be skipped if power management is not necessary. Things only go into DAPM if they have some relevance to power – that is, if they're a thing for which there is power control or if they control the routing of audio through the chip (and therefore let the core decide which parts of the chip need to be powered on).
DAPM lies in the ASoC Core (this means power switching is done from within the kernel) and becomes active as and when audio streams/paths/settings change, making it completely transparent for all userspace applications.
In the previous sections, we introduced the concept of controls and how to deal with them. However, kcontrols on their own are not involved in audio power management.
A normal kcontrol has the following characteristics:
• Self-descriptive and cannot describe the connection relationship between each kcontrol.
• Lacks a power management mechanism.
• Lacks a time processing mechanism to respond to audio events such as playing,
stopping, powering on, and powering off.
• Lacks a pop-pop sound prevention mechanism, so that it is up to the user program to pay attention to the power-on and power-off sequence for each kcontrol.
• Manual, because all the control involved in an audio path can't be automatically closed. When an audio path is no longer valid, it requires userspace intervention.
DAPM introduced the concept of widgets in order to solve the aforementioned problems.
A widget is the basic DAPM unit. Thus, the so-called widget can be understood as a further upgrade and encapsulation of kcontrols.
A widget is a combination of kcontrols and dynamic power management, and also has the link function of the audio path. It can have a dynamic connection relationship with its neighbor widget.
Writing codec class drivers 241
The DAPM framework abstracts a widget by means of the struct snd_soc_dapm_
widget structure, defined in include/sound/soc-dapm.h as follows:
```c
struct snd_soc_dapm_widget {
enum snd_soc_dapm_type id;
const char *name;
const char *sname;
```
[...]
/* dapm control */
```c
int reg; /* negative reg = no direct dapm */
unsigned char shift;
unsigned int mask;
unsigned int on_val;
unsigned int off_val;
```
[...]
```c
int (*power_check)(struct snd_soc_dapm_widget *w);
```
/* external events */
```c
unsigned short event_flags;
int (*event)(struct snd_soc_dapm_widget*,
struct snd_kcontrol *, int);
```
/* kcontrols that relate to this widget */
```c
int num_kcontrols;
const struct snd_kcontrol_new *kcontrol_news;
struct snd_kcontrol **kcontrols;
struct snd_soc_dobj dobj;
```
/* widget input and output edges */
```c
struct list_head edges[2];
```
/* used during DAPM updates */
```c
struct list_head dirty;
```
[...]
```c
}
```
242 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
For the sake of readability, only the relevant fields are listed in the preceding snippet, and the following are their descriptions:
• id is of type enum snd_soc_dapm_type and represents the type of the widget,
such as snd_soc_dapm_output, snd_soc_dapm_mixer, and so on. The full list is defined in include/sound/soc-dapm.h.
• name is the name of the widget.
• shift and mask are used to control the power state of the widget, corresponding to the register address reg.
• The on_val and off_val values represent the values that are to be used to change the current power state of the widget. They respectively correspond to when it is turned on and when it is turned off.
• event represents the DAPM event handling callback function pointer. Each widget is associated with a kcontrol object, pointed to by **kcontrols.
• *kcontrol_news is the array of controls this kcontrol is made of, and num_
kcontrols is the number of entries in it. These three fields are used to describe the kcontrol control contained in the widget, such as a mixer control or a MUX
control.
• dirty is used to insert this widget into a dirty list when the state of the widget is changed. This dirty list is then scanned in order to perform the update of the entire path.
## Defining widgets
Like the normal kcontrol, the DAPM framework provides us with a large number of auxiliary macros to define a variety of widget controls. These macro definitions can be spread into several fields according to the type of widgets and to the domain in which they are powered. They are as follows:
• The codec domain: Such as VREF and VMID; they provide reference voltage widgets. These widgets are usually controlled in the codec probe/remove callback.
• The platform/machine domain: These widgets are usually input/output interfaces for the platform or board (machine actually) that need to be physically connected,
such as headphones, speakers, and microphones. That being said, because these interfaces may differ on each board, they are usually configured by the machine driver and respond to asynchronous events, for example, when headphones are inserted. They can also be controlled by userspace applications to turn them on and off in some way.
Writing codec class drivers 243
• The audio path domain: Generally, refers to the MUX, mixer, and other widgets that control the audio path inside the codec. These widgets can automatically set their power state according to the connection relationship of the userspace, for example, alsamixer and amixer.
• The audio stream domain: These need to process audio data streams, such as
ADCs, DACs, and so on. Enabled and disabled when stream playback/capture is started and stopped respectively, for example, aplay and arecord.
All DAPM power switching decisions are made automatically according to a machine-specific audio routing map, which consists of the interconnections between every audio component (including internal codec components).
Codec domain definition
There is only one macro provided by the DAPM framework for this domain:
/* codec domain */
```c
#define SND_SOC_DAPM_VMID(wname) \
```
.id = snd_soc_dapm_vmid, .name = wname,
.kcontrol_news = NULL, \
.num_kcontrols = 0}
Defining platform domain widgets
The widgets of the platform domain correspond to the signal generator, input pin, output pin, microphone, earphone, speaker, and line input interface respectively. The DAPM
framework provides us with a number of auxiliary definition macros for the platform domain widgets. These are defined as the following:
```c
#define SND_SOC_DAPM_SIGGEN(wname) \
{ .id = snd_soc_dapm_siggen, .name = wname,
```
.kcontrol_news = NULL, \
.num_kcontrols = 0, .reg = SND_SOC_NOPM }
```c
#define SND_SOC_DAPM_SINK(wname) \
{ .id = snd_soc_dapm_sink, .name = wname,
```
.kcontrol_news = NULL, \
.num_kcontrols = 0, .reg = SND_SOC_NOPM }
```c
#define SND_SOC_DAPM_INPUT(wname) \
{ .id = snd_soc_dapm_input, .name = wname,
```
.kcontrol_news = NULL, \
244 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
.num_kcontrols = 0, .reg = SND_SOC_NOPM }
```c
#define SND_SOC_DAPM_OUTPUT(wname) \
{ .id = snd_soc_dapm_output, .name = wname,
```
.kcontrol_news = NULL, \
.num_kcontrols = 0, .reg = SND_SOC_NOPM }
```c
#define SND_SOC_DAPM_MIC(wname, wevent) \
{ .id = snd_soc_dapm_mic, .name = wname,
```
.kcontrol_news = NULL, \
.num_kcontrols = 0, .reg = SND_SOC_NOPM, .event = wevent, \
.event_flags = SND_SOC_DAPM_PRE_PMU |
```c
SND_SOC_DAPM_POST_PMD}
#define SND_SOC_DAPM_HP(wname, wevent) \
{ .id = snd_soc_dapm_hp, .name = wname,
```
.kcontrol_news = NULL, \
.num_kcontrols = 0, .reg = SND_SOC_NOPM, .event = wevent, \
.event_flags = SND_SOC_DAPM_POST_PMU |
```c
SND_SOC_DAPM_PRE_PMD}
#define SND_SOC_DAPM_SPK(wname, wevent) \
{ .id = snd_soc_dapm_spk, .name = wname,
```
.kcontrol_news = NULL, \
.num_kcontrols = 0, .reg = SND_SOC_NOPM, .event = wevent, \
.event_flags = SND_SOC_DAPM_POST_PMU |
```c
SND_SOC_DAPM_PRE_PMD}
#define SND_SOC_DAPM_LINE(wname, wevent) \
{ .id = snd_soc_dapm_line, .name = wname,
```
.kcontrol_news = NULL, \
.num_kcontrols = 0, .reg = SND_SOC_NOPM, .event = wevent, \
.event_flags = SND_SOC_DAPM_POST_PMU |
```c
SND_SOC_DAPM_PRE_PMD}
#define SND_SOC_DAPM_INIT_REG_VAL(wreg, wshift, winvert) \
```
.reg = wreg, .mask = 1, .shift = wshift, \
.on_val = winvert ? 0 : 1, .off_val = winvert ? 1 : 0
Writing codec class drivers 245
In the preceding code, most of the fields in these macros are common. The fact that the reg field is set to SND_SOC_NOPM (defined to -1) means that these widgets have no register control bits to control the power state of the widgets. SND_SOC_DAPM_INPUT
and SND_SOC_DAPM_OUTPUT are used to define the output and input pins of the codec chip from within the codec driver. From what we can see, the MIC, HP, SPK, and
LINE widgets respond to SND_SOC_DAPM_POST_PMU (after widget power-up) and
```c
SND_SOC_DAPM_PMD (before widget power-down) events, and these widgets are usually defined in the machine driver.
```
Defining an audio path domain widget
This kind of widget usually repackages the ordinary kcontrols and extends them with audio path and power management functions. This extension somehow makes this kind of widget DAPM-aware. Widgets in this domain will contain one or more kcontrols that are not the ordinary kcontrols. There are DAPM-enabled kcontrols. These cannot be defined using the standard method, that is, SOC_*-based macro controls. They need to be defined using the definition macros provided by the DAPM framework. We will discuss them in detail later, in the Defining DAPM kcontrols section. However, here are the definition macros for these widgets:
```c
#define SND_SOC_DAPM_PGA(wname, wreg, wshift, winvert,\
```
wcontrols, wncontrols) \
```c
{ .id = snd_soc_dapm_pga, .name = wname, \
SND_SOC_DAPM_INIT_REG_VAL(wreg, wshift, winvert), \
```
.kcontrol_news = wcontrols, .num_kcontrols = wncontrols}
```c
#define SND_SOC_DAPM_OUT_DRV(wname, wreg, wshift, winvert,\
```
wcontrols, wncontrols) \
```c
{ .id = snd_soc_dapm_out_drv, .name = wname, \
SND_SOC_DAPM_INIT_REG_VAL(wreg, wshift, winvert), \
```
.kcontrol_news = wcontrols, .num_kcontrols = wncontrols}
```c
#define SND_SOC_DAPM_MIXER(wname, wreg, wshift, winvert, \
```
wcontrols, wncontrols)\
```c
{ .id = snd_soc_dapm_mixer, .name = wname, \
SND_SOC_DAPM_INIT_REG_VAL(wreg, wshift, winvert), \
```
.kcontrol_news = wcontrols, .num_kcontrols = wncontrols}
```c
#define SND_SOC_DAPM_MIXER_NAMED_CTL(wname, wreg,
```
246 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers wshift, winvert, \
wcontrols, wncontrols)\
```c
{ .id = snd_soc_dapm_mixer_named_ctl, .name = wname, \
SND_SOC_DAPM_INIT_REG_VAL(wreg, wshift, winvert), \
```
.kcontrol_news = wcontrols, .num_kcontrols = wncontrols}
```c
#define SND_SOC_DAPM_SWITCH(wname, wreg, wshift, winvert,
```
wcontrols) \
```c
{ .id = snd_soc_dapm_switch, .name = wname, \
SND_SOC_DAPM_INIT_REG_VAL(wreg, wshift, winvert), \
```
.kcontrol_news = wcontrols, .num_kcontrols = 1}
```c
#define SND_SOC_DAPM_MUX(wname, wreg, wshift,
```
winvert, wcontrols) \
```c
{ .id = snd_soc_dapm_mux, .name = wname, \
SND_SOC_DAPM_INIT_REG_VAL(wreg, wshift, winvert), \
```
.kcontrol_news = wcontrols, .num_kcontrols = 1}
```c
#define SND_SOC_DAPM_DEMUX(wname, wreg, wshift,
```
winvert, wcontrols) \
```c
{ .id = snd_soc_dapm_demux, .name = wname, \
SND_SOC_DAPM_INIT_REG_VAL(wreg, wshift, winvert), \
```
.kcontrol_news = wcontrols, .num_kcontrols = 1}
Unlike platform and codec domain widgets, the reg and shift fields need to be assigned,
indicating that these widgets have corresponding power control registers. The DAPM
framework uses these registers to control the power state of the widgets when scanning and updating the audio path. Their power states are dynamically allocated, powered up when needed (on a valid audio path), and powered down when not needed (on an inactive audio path). These widgets need to perform the same functions as the mixer, MUX, and so on introduced earlier. In fact, this is done by the kcontrol controls they contain. The driver code must define kcontrols before defining the widget, and then pass the wcontrols and num_kcontrols parameters to these auxiliary definition macros.
Writing codec class drivers 247
There is another variant of those macros that exists and that has a pointer to an event handler. Such macros have the _E suffix. These are SND_SOC_DAPM_PGA_E, SND_SOC_
DAPM_OUT_DRV_E, SND_SOC_DAPM_MIXER_E, SND_SOC_DAPM_MIXER_NAMED_
CTL_E, SND_SOC_DAPM_SWITCH_E, SND_SOC_DAPM_MUX_E, and SND_SOC_
DAPM_VIRT_MUX_E. You are encouraged to have a look at the kernel source code to see their definitions at https://elixir.bootlin.com/linux/v4.19/source/
include/sound/soc-dapm.h#L136.
Defining the audio stream domain
These widgets mainly include audio input/output interfaces, ADC/DAC, and clock lines.
Starting with the audio interface widgets, these are the following:
```c
#define SND_SOC_DAPM_AIF_IN(wname, stname, wslot, wreg, wshift,
```
winvert) \
```c
{ .id = snd_soc_dapm_aif_in, .name = wname, .sname = stname, \
SND_SOC_DAPM_INIT_REG_VAL(wreg, wshift, winvert), }
#define SND_SOC_DAPM_AIF_IN_E(wname, stname, wslot, wreg, \
```
wshift, winvert, wevent, wflags) \
```c
{ .id = snd_soc_dapm_aif_in, .name = wname, .sname = stname, \
SND_SOC_DAPM_INIT_REG_VAL(wreg, wshift, winvert), \
```
.event = wevent, .event_flags = wflags }
```c
#define SND_SOC_DAPM_AIF_OUT(wname, stname, wslot, wreg,
```
wshift, winvert) \
```c
{ .id = snd_soc_dapm_aif_out, .name = wname, .sname = stname, \
SND_SOC_DAPM_INIT_REG_VAL(wreg, wshift, winvert), }
#define SND_SOC_DAPM_AIF_OUT_E(wname, stname, wslot, wreg, \
```
wshift, winvert, wevent, wflags) \
```c
{ .id = snd_soc_dapm_aif_out, .name = wname, .sname = stname, \
SND_SOC_DAPM_INIT_REG_VAL(wreg, wshift, winvert), \
```
.event = wevent, .event_flags = wflags }
248 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
In the preceding macro definition list, SND_SOC_DAPM_AIF_IN and SND_SOC_DAPM_
AIF_OUT are respectively the audio interface input and output. The former defines the connection to the host that receives the audio to be passed into the DAC(s) and the latter defines the connection to the host that transmits the audio received from the ADC(s).
```c
SND_SOC_DAPM_AIF_IN_E and SND_SOC_DAPM_AIF_OUT_E are their respective event variants, allowing wevent to be called when one of the events enabled in wflags occurs.
```
Now come the ADC/DAC-related widgets, as well as the clock-related one, defined as the following:
```c
#define SND_SOC_DAPM_DAC(wname, stname, wreg,
```
wshift, winvert) \
```c
{ .id = snd_soc_dapm_dac, .name = wname, .sname = stname, \
SND_SOC_DAPM_INIT_REG_VAL(wreg, wshift, winvert) }
#define SND_SOC_DAPM_DAC_E(wname, stname, wreg, wshift, \
```
winvert, wevent, wflags) \
```c
{ .id = snd_soc_dapm_dac, .name = wname, .sname = stname, \
SND_SOC_DAPM_INIT_REG_VAL(wreg, wshift, winvert), \
```
.event = wevent, .event_flags = wflags}
```c
#define SND_SOC_DAPM_ADC(wname, stname, wreg,
```
wshift, winvert) \
```c
{ .id = snd_soc_dapm_adc, .name = wname, .sname = stname, \
SND_SOC_DAPM_INIT_REG_VAL(wreg, wshift, winvert), }
#define SND_SOC_DAPM_ADC_E(wname, stname, wreg, wshift,\
```
winvert, wevent, wflags) \
```c
{ .id = snd_soc_dapm_adc, .name = wname, .sname = stname, \
SND_SOC_DAPM_INIT_REG_VAL(wreg, wshift, winvert), \
```
.event = wevent, .event_flags = wflags}
```c
#define SND_SOC_DAPM_CLOCK_SUPPLY(wname) \
{ .id = snd_soc_dapm_clock_supply, .name = wname, \
```
.reg = SND_SOC_NOPM, .event = dapm_clock_event, \
.event_flags = SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD
```c
}
```
Writing codec class drivers 249
In the preceding list of macros, SND_SOC_DAPM_ADC and SND_SOC_DAPM_DAC are
ADC and DAC widgets respectively. The former is used to control the powering up and shutting down of the ADC on an as-needed basis, while the latter targets DAC(s). The former is typically associated with a capture stream on the device, for example, "Left
Capture" or "Right Capture," and the latter is typically associated with a playback stream,
for example, "Left Playback" or "Right Playback." The register settings define a single register and bit position that, when flipped, will turn the ADC/DAC on or off. You should also notice their event variants, SND_SOC_DAPM_ADC_E and SND_SOC_DAPM_DAC_E
respectively. SND_SOC_DAPM_CLOCK_SUPPLY is a supply-widget variant for connection to the clock framework.
There are other widget types for which no definition macro is provided, and that do not end in any of the domains we have introduced so far. These are snd_soc_dapm_dai_
in, snd_soc_dapm_dai_out, and snd_soc_dapm_dai_link.
Such widgets are implicitly created upon DAI registration, either from the CPU or the codec driver. In other words, whenever a DAI is registered, the DAPM core will create a widget either of type snd_soc_dapm_dai_in or of type snd_soc_dapm_dai_
out according to the streams of the DAI being registered. Usually, both widgets will be connected to widgets with the same stream name in the codec. Additionally, when the machine driver decides to bind codec and CPU DAIs together, this will result in the
DAPM framework creating a widget of type snd_soc_dapm_dai_link to describe the power state of the connection.
The concept of a path – a connector between widgets
Widgets are meant to be linked one to the other in order to build a functional audio stream path. That being said, the connection between two widgets needs to be tracked in order to maintain the audio state. To describe the patch between two widgets, the DAPM
core uses the struct snd_soc_dapm_path data structure, defined as follows:
/* dapm audio path between two widgets */
```c
struct snd_soc_dapm_path {
const char *name;
```
/*
* source (input) and sink (output) widgets
* The union is for convenience,
* since it is a lot nicer to type
```c
* p->source, rather than p->node[SND_SOC_DAPM_DIR_IN]
```
*/
```c
union {
250 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers struct {
struct snd_soc_dapm_widget *source;
struct snd_soc_dapm_widget *sink;
};
struct snd_soc_dapm_widget *node[2];
};
```
/* status */
u32 connect:1; /* source and sink widgets are connected */
u32 walking:1; /* path is in the process of being walked */
u32 weak:1; /* path ignored for power management */
u32 is_supply:1; /* At least one of the connected widgets is a supply */
```c
int (*connected)(struct snd_soc_dapm_widget *source, struct snd_soc_dapm_widget *sink);
struct list_head list_node[2];
struct list_head list_kcontrol;
struct list_head list;
};
```
This structure abstracts the link between two widgets. Its source field points to the start widget of the connection, whereas its sink field points to the arrival widget of the connection. The input and output (that is, an endpoint) of the widget may be connected to multiple paths. The snd_soc_dapm_path structure of all inputs is hung in the sources list of the widget through the list_node[SND_SOC_DAPM_DIR_IN] field while the snd_soc_dapm_path structure of all outputs is stored in the sinks list of the widget,
```c
which is list_node[SND_SOC_DAPM_DIR_OUT]. The connection goes from the source to the sink and the principle is quite simple. Just remember the connection path is this: the output of the start widget --> the input of the path data structure* and *the output of the path data structure --> Arrival side widget input.
```
The list field will end up in the sound card's path list header field upon card registration. This list allows the sound card to track all the available paths it can use.
Finally, the connected field is there to let you implement your own custom method to check the current connection state of the path.
Writing codec class drivers 251
Important note
```c
SND_SOC_DAPM_DIR_IN and SND_SOC_DAPM_DIR_OUT are enumerators that are 0 and 1 respectively.
```
You'll probably never want to deal with a path directly. However, this concept has been introduced here for the sake of pedagogy, as it will help us understand the next section.
The concept of a route – widget inter-connections
The concept of a path, introduced earlier in this chapter, was an introduction to this one.
From the preceding discussion, we can introduce the concept of a route. A route connection is made of at least the starter widget, the jumper path, the sink widget, and in the DAPM the struct snd_soc_dapm_route structure is used to describe such a connection:
```c
struct snd_soc_dapm_route {
const char *sink;
const char *control;
const char *source;
```
/* Note: currently only supported for links where source is a supply */
```c
int (*connected)(struct snd_soc_dapm_widget *source,
struct snd_soc_dapm_widget *sink);
};
```
In the preceding data structure, sink points to the name string of the arriving widget,
source points to the name string of the starting widget, control points to the kcontrol name string responsible for controlling the connection, and connected defines the custom connection check callback. The meaning of this structure is obvious: source is connected to sink via a kcontrol and a connected callback function can be called to check the connection state.
Routes are to be defined using the following scheme:
```c
{Destination Widget, Switch, Source Widget},
```
This means Source Widget is connected to Destination Widget via Swtich.
This way, the DAPM core will take care of closing the switch whenever the connection needs to be activated, and both source and destination widget will be powered on as well.
Sometimes, the connection may be direct. In this case, Switch should be NULL. You'll then have something like the following:
```c
{end point, NULL, starting point},
```
252 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
You should directly use the name string to describe the connection relationship, all defined routes, and finally, you have to register to the DAPM core. DAPM core will find the corresponding widget according to these names, and dynamically generate the required snd_soc_dapm_path to describe the connection between the two widgets. In the next sections, we'll see how to create routes.
Defining DAPM kcontrols
As mentioned in the previous sections, mixers or MUX-type widgets in the audio path domain are made of several kcontrols, which must be defined using DAPM-based macros.
DAPM uses these kcontrols to complete the audio path. However, for widgets, this task is more than that. DAPM also dynamically manages the connection relationships of these audio paths so that the power state of these widgets can be controlled according to these connection relationships. If these kcontrols are defined in the usual way, this is not possible, so DAPM provides us with another set of definition macros that define the kcontrols that are included in the widget:
```c
#define SOC_DAPM_SINGLE(xname, reg, shift, max, invert) \
{ .iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
```
.info = snd_soc_info_volsw, \
.get = snd_soc_dapm_get_volsw,
.put = snd_soc_dapm_put_volsw, \
.private_value = SOC_SINGLE_VALUE(reg, shift, max, invert) }
```c
#define SOC_DAPM_SINGLE_TLV(xname, reg, shift, max, invert,
```
tlv_array) \
```c
{ .iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
```
.info = snd_soc_info_volsw, \
.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ | \
```c
SNDRV_CTL_ELEM_ACCESS_READWRITE, \
```
.tlv.p = (tlv_array), \
.get = snd_soc_dapm_get_volsw,
.put = snd_soc_dapm_put_volsw, \
.private_value = SOC_SINGLE_VALUE(reg, shift, max, invert) }
```c
#define SOC_DAPM_ENUM(xname, xenum) \
{ .iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
```
.info = snd_soc_info_enum_double, \
.get = snd_soc_dapm_get_enum_double, \
Writing codec class drivers 253
.put = snd_soc_dapm_put_enum_double, \
.private_value = (unsigned long)&xenum}
```c
#define SOC_DAPM_ENUM_VIRT(xname, xenum) \
{ .iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
```
.info = snd_soc_info_enum_double, \
.get = snd_soc_dapm_get_enum_virt, \
.put = snd_soc_dapm_put_enum_virt, \
.private_value = (unsigned long)&xenum}
```c
#define SOC_DAPM_ENUM_EXT(xname, xenum, xget, xput) \
{ .iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
```
.info = snd_soc_info_enum_double, \
.get = xget, \
.put = xput, \
.private_value = (unsigned long)&xenum }
```c
#define SOC_DAPM_VALUE_ENUM(xname, xenum) \
{ .iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, \
```
.info = snd_soc_info_enum_double, \
.get = snd_soc_dapm_get_value_enum_double, \
.put = snd_soc_dapm_put_value_enum_double, \
.private_value = (unsigned long)&xenum }
```c
#define SOC_DAPM_PIN_SWITCH(xname) \
{ .iface = SNDRV_CTL_ELEM_IFACE_MIXER,
```
.name = xname " Switch" , \
.info = snd_soc_dapm_info_pin_switch, \
.get = snd_soc_dapm_get_pin_switch, \
.put = snd_soc_dapm_put_pin_switch, \
.private_value = (unsigned long)xname }
254 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
It can be seen that SOC_DAPM_SINGLE is the DAPM equivalent to SOC_SINGLE of the standard control, SOC_DAPM_SINGLE_TLV corresponds to SOC_SINGLE_TLV, and so on. Compared to the ordinary kcontrols, DAPM's kcontrols just replace the info, get,
and put callback functions. The put callback function provided by DAPM kcontrols not only updates the state of the control itself but also passes this change to the adjacent
DAPM kcontrol. The adjacent DAPM kcontrol will pass this change to its own neighbor
DAPM kcontrol, knowing at the end of the audio path, by changing the connection state of one of the widgets, all widgets associated with it are scanned and tested to see if they are still in the active audio path, thus dynamically changing their power state. This is the essence of DAPM.
## Creating widgets and routes
The previous section introduced a lot of auxiliary macros. However, it was theoretical and did not explain how to define the widgets we need for a real system, nor how to define the connection relationship of widgets. Here, we take Wolfson's codec chip WM8960 as an example to understand this process:
Figure 5.3 – WM8960 internal audio paths and controls
Writing codec class drivers 255
Given the preceding diagram as an example, from the Wolfson WM8960 codec chip, the first step is to use the helper macro to define the DAPM kcontrol required by the widgets:
```c
static const struct snd_kcontrol_new wm8960_loutput_mixer[] = {
SOC_DAPM_SINGLE("PCM Playback Switch", WM8960_LOUTMIX, 8,
```
1, 0),
```c
SOC_DAPM_SINGLE("LINPUT3 Switch", WM8960_LOUTMIX, 7, 1, 0),
SOC_DAPM_SINGLE("Boost Bypass Switch", WM8960_BYPASS1, 7,
```
1, 0),
```c
};
static const struct snd_kcontrol_new wm8960_routput_mixer[] = {
SOC_DAPM_SINGLE("PCM Playback Switch", WM8960_ROUTMIX, 8,
```
1, 0),
```c
SOC_DAPM_SINGLE("RINPUT3 Switch", WM8960_ROUTMIX, 7, 1, 0),
SOC_DAPM_SINGLE("Boost Bypass Switch", WM8960_BYPASS2, 7,
```
1, 0),
```c
};
static const struct snd_kcontrol_new wm8960_mono_out[] = {
SOC_DAPM_SINGLE("Left Switch", WM8960_MONOMIX1, 7, 1, 0),
SOC_DAPM_SINGLE("Right Switch", WM8960_MONOMIX2, 7, 1, 0),
};
```
In the preceding, we defined the mixer controls for the left and right output channels in wm8960, as well as the mono output mixer: wm8960_loutput_mixer, wm8960_
routput_mixer, and wm8960_mono_out.
The second step consists of defining the real widget, including the DAPM control defined in the first step:
```c
static const struct snd_soc_dapm_widget wm8960_dapm_widgets[] =
{
```
[...]
```c
SND_SOC_DAPM_INPUT("LINPUT3"),
SND_SOC_DAPM_INPUT("RINPUT3"),
SND_SOC_DAPM_SUPPLY("MICB", WM8960_POWER1, 1, 0, NULL, 0),
```
[...]
```c
SND_SOC_DAPM_DAC("Left DAC", "Playback", WM8960_POWER2, 8,
```
0),
```c
SND_SOC_DAPM_DAC("Right DAC", "Playback", WM8960_POWER2, 7,
```
256 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
0),
```c
SND_SOC_DAPM_MIXER("Left Output Mixer", WM8960_POWER3, 3,
```
0,
&wm8960_loutput_mixer[0],
```c
ARRAY_SIZE(wm8960_loutput_mixer)),
SND_SOC_DAPM_MIXER("Right Output Mixer", WM8960_POWER3, 2,
```
0,
&wm8960_routput_mixer[0],
```c
ARRAY_SIZE(wm8960_routput_mixer)),
SND_SOC_DAPM_PGA("LOUT1 PGA", WM8960_POWER2, 6, 0, NULL,
```
0),
```c
SND_SOC_DAPM_PGA("ROUT1 PGA", WM8960_POWER2, 5, 0, NULL,
```
0),
```c
SND_SOC_DAPM_PGA("Left Speaker PGA", WM8960_POWER2,
```
4, 0, NULL, 0),
```c
SND_SOC_DAPM_PGA("Right Speaker PGA", WM8960_POWER2,
```
3, 0, NULL, 0),
```c
SND_SOC_DAPM_PGA("Right Speaker Output", WM8960_CLASSD1,
```
7, 0, NULL, 0);
```c
SND_SOC_DAPM_PGA("Left Speaker Output", WM8960_CLASSD1,
```
6, 0, NULL, 0),
```c
SND_SOC_DAPM_OUTPUT("SPK_LP"),
SND_SOC_DAPM_OUTPUT("SPK_LN"),
SND_SOC_DAPM_OUTPUT("HP_L"),
SND_SOC_DAPM_OUTPUT("HP_R"),
SND_SOC_DAPM_OUTPUT("SPK_RP"),
SND_SOC_DAPM_OUTPUT("SPK_RN"),
SND_SOC_DAPM_OUTPUT("OUT3"),
};
static const struct snd_soc_dapm_widget wm8960_dapm_widgets_out3[] = {
SND_SOC_DAPM_MIXER("Mono Output Mixer", WM8960_POWER2, 1,
```
0,
&wm8960_mono_out[0],
Writing codec class drivers 257
```c
ARRAY_SIZE(wm8960_mono_out)),
};
```
In this step, a MUX widget is defined for each of the left and right channels as well as the channel selector: these are Left Output Mixer, Right Output Mixer, and Mono Output
Mixer. We also define a mixer widget for each of the left and right speakers: SPK_LP,
SPK_LN, HP_L, HP_R, SPK_RP, OUT3, and SPK_RN. The specific mixer control is done by wm8960_loutput_mixer, wm8960_routput_mixer, and wm8960_mono_out defined in the previous step. The three widgets have power properties, so when one (or more) of these widgets are in one valid audio path, the DAPM framework can control its power state via bits 7 and/or 8 of their respective registers.
The third step is to define the connection path of these widgets:
```c
static const struct snd_soc_dapm_route audio_paths[] = {
```
[...]
```c
{"Left Output Mixer", "LINPUT3 Switch", "LINPUT3"},
{"Left Output Mixer", "Boost Bypass Switch",
```
"Left Boost Mixer"},
```c
{"Left Output Mixer", "PCM Playback Switch", "Left DAC"},
{"Right Output Mixer", "RINPUT3 Switch", "RINPUT3"},
{"Right Output Mixer", "Boost Bypass Switch",
```
"Right Boost Mixer"},
```c
{"Right Output Mixer", "PCM Playback Switch", "Right DAC"},
{"LOUT1 PGA", NULL, "Left Output Mixer"},
{"ROUT1 PGA", NULL, "Right Output Mixer"},
{"HP_L", NULL, "LOUT1 PGA"},
{"HP_R", NULL, "ROUT1 PGA"},
{"Left Speaker PGA", NULL, "Left Output Mixer"},
{"Right Speaker PGA", NULL, "Right Output Mixer"},
{"Left Speaker Output", NULL, "Left Speaker PGA"},
{"Right Speaker Output", NULL, "Right Speaker PGA"},
{"SPK_LN", NULL, "Left Speaker Output"},
```
258 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
```c
{"SPK_LP", NULL, "Left Speaker Output"},
{"SPK_RN", NULL, "Right Speaker Output"},
{"SPK_RP", NULL, "Right Speaker Output"},
};
static const struct snd_soc_dapm_route audio_paths_out3[] = {
{"Mono Output Mixer", "Left Switch", "Left Output Mixer"},
{"Mono Output Mixer", "Right Switch", "Right Output Mixer"},
{"OUT3", NULL, "Mono Output Mixer"}
};
```
Through the definition of the first step, we know that "Left output Mux" and
"right output Mux" have three input pins, respectively, "Boost Bypass
Switch", "LINPUT3 Switch" (or "RINPUT3 Switch"), and "PCM Playback
Switch". "Mono Output Mixer" has only two input select pins, which are "Left
Switch" and "Right Switch". So, obviously, the meaning of the preceding path definition is as follows:
• "Left Boost Mixer" is connected to "Left Output Mixer" via "Boost
Bypass Switch".
• "Left DAC" is connected to "Left Output Mixer" via "PCM Playback
Switch".
• "RINPUT3" is connected to "Right Output Mixer" via "RINPUT3
Switch".
• "Right Boost Mixer" is connected to "Right Output Mixer" via
"Boost Bypass Switch".
• "Right DAC" is connected to "Right Output Mixer" via "PCM Playback
Switch".
• "Left Output Mixer" is connected to "LOUT1 PGA". However, there is no switch control for this link.
• "Right Output Mixer" is connected to "ROUT1 PGA", with no switch controlling this connection.
Not all of the connections have been described, but the idea is there. The fourth step is to register these widgets and paths in the codec-driven probe callback:
```c
static int wm8960_add_widgets(struct snd_soc_component *component)
```
Writing codec class drivers 259
```c
{
```
[...]
```c
struct snd_soc_dapm_context *dapm =
snd_soc_component_get_dapm(component);
struct snd_soc_dapm_widget *w;
snd_soc_dapm_new_controls(dapm, wm8960_dapm_widgets,
ARRAY_SIZE(wm8960_dapm_widgets));
snd_soc_dapm_add_routes(dapm, audio_paths,
ARRAY_SIZE(audio_paths));
```
[...]
```c
return 0;
}
static int wm8960_probe(struct snd_soc_component *component)
{
```
[...]
```c
snd_soc_add_component_controls(component,
```
wm8960_snd_controls,
```c
ARRAY_SIZE(wm8960_snd_controls));
wm8960_add_widgets(component);
return 0;
}
static const struct snd_soc_component_driver soc_component_dev_wm8960 = {
```
.probe = wm8960_probe,
.set_bias_level = wm8960_set_bias_level,
.suspend_bias_off = 1,
.idle_bias_on = 1,
.use_pmdown_time = 1,
.endianness = 1,
.non_legacy_dai_naming = 1,
```c
};
static int wm8960_i2c_probe(struct i2c_client *i2c,
const struct i2c_device_id *id)
```
260 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
```c
{
```
[...]
```c
ret = devm_snd_soc_register_component(&i2c->dev,
```
&soc_component_dev_wm8960,
&wm8960_dai, 1);
```c
return ret;
}
```
In the preceding example, controls, widgets, and route registration are deferred into the component driver's probe callback. This helps make sure that these elements are created only when the component is probed by the machine driver. In the machine driver, we can define and register the board-specific widgets and path information in the same way.
## Codec component registration
After the codec component has been set up, it has to be registered with the system so that it can be used for what it is designed for. For this purpose, you should use devm_snd_
soc_register_component(). This function will take care of unregistration/cleaning automatically when needed. The following is its prototype:
```c
int devm_snd_soc_register_component(struct device *dev,
const struct snd_soc_component_driver *cmpnt_drv,
struct snd_soc_dai_driver *dai_drv,
int num_dai)
```
The following is an example of codec registration, which is an excerpt from the wm8960 codec driver. The component driver is first defined as the following:
```c
static const struct snd_soc_component_driver soc_component_dev_wm8900 = {
```
.probe = wm8900_probe,
.suspend = wm8900_suspend,
.resume = wm8900_resume,
[...]
/* control, widget and route setup */
.controls = wm8900_snd_controls,
.num_controls = ARRAY_SIZE(wm8900_snd_controls),
.dapm_widgets = wm8900_dapm_widgets,
Writing codec class drivers 261
.num_dapm_widgets = ARRAY_SIZE(wm8900_dapm_widgets),
.dapm_routes = wm8900_dapm_routes,
.num_dapm_routes = ARRAY_SIZE(wm8900_dapm_routes),
```c
};
```
That component driver contains dapm routes and widgets, as well as a set of controls.
Then, the codec dai callbacks are provided through the struct snd_soc_dai_ops,
as follows:
```c
static const struct snd_soc_dai_ops wm8900_dai_ops = {
```
.hw_params = wm8900_hw_params,
.set_clkdiv = wm8900_set_dai_clkdiv,
.set_pll = wm8900_set_dai_pll,
.set_fmt = wm8900_set_dai_fmt,
.digital_mute = wm8900_digital_mute,
```c
};
```
Those codec dai callbacks the assigned to the codec dai driver (via the ops field) for registration with the ASoC core as follows:
```c
#define WM8900_RATES (SNDRV_PCM_RATE_8000 |\
SNDRV_PCM_RATE_11025 |\
SNDRV_PCM_RATE_16000 |\
SNDRV_PCM_RATE_22050 |\
SNDRV_PCM_RATE_44100 |\
SNDRV_PCM_RATE_48000)
#define WM8900_PCM_FORMATS \
```
(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | \
```c
SNDRV_PCM_FMTBIT_S24_LE)
static struct snd_soc_dai_driver wm8900_dai = {
```
.name = "wm8900-hifi",
```c
.playback = {
```
.stream_name = "HiFi Playback",
.channels_min = 1,
.channels_max = 2,
.rates = WM8900_RATES,
.formats = WM8900_PCM_FORMATS,
262 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
```c
},
.capture = {
```
.stream_name = "HiFi Capture",
.channels_min = 1,
.channels_max = 2,
.rates = WM8900_RATES,
.formats = WM8900_PCM_FORMATS,
```c
},
```
.ops = &wm8900_dai_ops,
```c
};
static int wm8900_spi_probe(struct spi_device *spi)
{
```
[...]
```c
ret = devm_snd_soc_register_component(&spi->dev,
```
&soc_component_dev_wm8900,
&wm8900_dai, 1);
```c
return ret;
}
```
When the machine driver probes this codec, then the probe callback of the codec component driver (wm8900_probe) will be invoked, and they will complete the codec driver initialization. The full version of this codec device driver is sound/soc/codecs/
wm8900.c in the Linux kernel source.
Now we are familiar with the codec class driver and its architecture. We have seen how to export codec properties as well, how to build audio routes, and how to implement DAPM
features. On its own, the codec driver is quite useless, though it manages the codec device.
It needs to be tied to the platform driver, which is the next driver class we are going to learn about.
## Writing the platform class driver
The platform driver registers the PCM driver, CPU DAI driver, and their operation functions, pre-allocates buffers for PCM components, and sets playback and capture operations as applicable. In other words, the platform driver contains the audio DMA
engine and audio interface drivers (for example, I2S, AC97, and PCM) for that platform.
Writing the platform class driver 263
The platform driver targets the SoC the platform is made of. It concerns the platform's
DMA, which is how audio data transits between each block in the SoC, and CPU DAI,
which is the path the CPU uses to send/carry audio data to/from the codec. Such a driver has two important data structures: struct snd_soc_component_driver and struct snd_soc_dai_driver. The former is responsible for DMA data management, and the latter is responsible for the parameter configuration of the DAI.
However, both of these data structures have already been described while dealing with codec class drivers. Thus, this part will just deal with additional concepts, related to the platform code.
## The CPU DAI driver
Since the platform code has been refactored too, as with the codec driver, the CPU DAI
drivers must export an instance of the component driver as well as an instance of the DAI
driver, respectively struct snd_soc_component_driver and struct snd_
soc_dai_driver.
On the platform side, most of the work can be done by the core, especially for
DMA-related stuff. Thus, it is common for CPU DAI drivers to provide only the name of the interface within the component driver structure and to let the core do the rest. The following is an example from the Rockchip SPDIF driver, implemented in sound/soc/
rockchip/rockchip_spdif.c:
```c
static const struct snd_soc_dai_ops rk_spdif_dai_ops = {
```
[...]
```c
};
```
/* SPDIF has no capture channel */
```c
static struct snd_soc_dai_driver rk_spdif_dai = {
```
.probe = rk_spdif_dai_probe,
```c
.playback = {
```
.stream_name = "Playback",
[...]
```c
},
```
.ops = &rk_spdif_dai_ops,
```c
};
```
/* fill in the name only */
```c
static const struct snd_soc_component_driver rk_spdif_component
= {
```
.name = "rockchip-spdif",
264 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
```c
};
static int rk_spdif_probe(struct platform_device *pdev)
{
struct device_node *np = pdev->dev.of_node;
struct rk_spdif_dev *spdif;
int ret;
```
[...]
```c
spdif->playback_dma_data.addr = res->start + SPDIF_SMPDR;
spdif->playback_dma_data.addr_width =
```
DMA_SLAVE_BUSWIDTH_4_BYTES;
```c
spdif->playback_dma_data.maxburst = 4;
ret = devm_snd_soc_register_component(&pdev->dev,
```
&rk_spdif_component,
&rk_spdif_dai, 1);
```c
if (ret) {
dev_err(&pdev->dev, "Could not register DAI\n");
```
goto err_pm_runtime;
```c
}
ret = devm_snd_dmaengine_pcm_register(&pdev->dev, NULL, 0);
if (ret) {
dev_err(&pdev->dev, "Could not register PCM\n");
```
goto err_pm_runtime;
```c
}
return 0;
}
```
In the preceding excerpt, spdif is the driver state data structure. We can see that only the name is filled in in the component driver, and both the component and DAI drivers are registered as usual by means of devm_snd_soc_register_component().
The struct snd_soc_dai_driver must be set up according to the actual DAI
properties, and the dai_ops should be set if necessary. However, a big part of the setup is done by devm_snd_dmaengine_pcm_register(), which will set the component driver's PCM ops according to the dma_data provided. This is explained in detail in the next section.
Writing the platform class driver 265
## The platform DMA driver AKA PCM DMA driver
In a sound ecosystem, we have several types of devices: PCM, MIDI, mixer, sequencer,
timer, and so on. Here, PCM does refer to Pulse Code Modulation, but it is a reference to a device that processes sample-based digital audio, that is, not midi and so on. The PCM
layer (part of the ALSA core) is responsible for doing all the digital audio work, such as preparing the card for capture or playback, initiating the transfer to and from the device,
and so on. In short, if you want to play back or capture sound, you're going to need a PCM.
The PCM driver helps perform DMA operations by overriding the function pointers exposed by the struct snd_pcm_ops structure. It is platform agnostic and interacts only with the SOC DMA engine upstream APIs. The DMA engine then interacts with the platform-specific DMA driver to get the correct DMA settings. The struct snd_pcm_
ops is a structure that contains a set of callbacks that relate to different events regarding the PCM interface.
While dealing with ASoC (not purely ALSA), you'll never need to instantiate this structure as is, as long as you use the generic PCM DMA engine framework. The ASoC
```c
core does this for you. Have a look at the following call stack: snd_soc_register_card ->
snd_soc_instantiate_card -> soc_probe_link_dais -> soc_new_pcm.
```
The audio DMA interface
Each audio bus driver of the SoC is responsible for providing a DMA interface by means of this API. This is the case, for example, for the audio buses on i.MX-based SoCs, such as
ESAI, SAI, SPDIF and SSI whose drivers are located in sound/soc/fsl/, respectively sound/soc/fsl/fsl_esai.c, sound/soc/fsl/fsl_sai.c, sound/soc/fsl/
fsl_spdif.c, and sound/soc/fsl/fsl_ssi.c.
The audio DMA driver is registered via devm_snd_dmaengine_pcm_register().
This function registers a struct snd_dmaengine_pcm_config for the device. The following is its prototype:
```c
int devm_snd_dmaengine_pcm_register(
struct device *dev,
const struct snd_dmaengine_pcm_config *config,
unsigned int flags);
```
266 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
In the preceding prototype, dev is the parent device for the PCM device, usually
```c
&pdev->dev. config is the platform-specific PCM configuration, which is of type struct snd_dmaengine_pcm_config. This structure needs to be described in detail. flags represents additional flags describing how to deal with DMA channels.
```
Most of the time, it is 0. However, possible values are defined in include/sound/
dmaengine_pcm.h and are all prefixed with SND_DMAENGINE_PCM_FLAG_.
Frequently used ones are SND_DMAENGINE_PCM_FLAG_HALF_DUPLEX, SND_
DMAENGINE_PCM_FLAG_NO_DT, and SND_DMAENGINE_PCM_FLAG_COMPAT. The former indicates that the PCM is half-duplex and the DMA channel is shared between capture and playback. The second one asks the core not to try to request the DMA
channels through the device tree. The last one means a custom callback will be used to request the DMA channel. Upon registration, the generic PCM DMA engine framework will build a suitable snd_pcm_ops and set the component driver's .ops field with it.
The classic DMA operation flow in Linux is as follows:
1. dma_request_channel: For allocating the slave channel.
2. dmaengine_slave_config: To set slave- and controller-specific parameters.
3. dma_prep_xxxx: To get a descriptor for the transaction.
4. dma_cookie = dmaengine_submit(tx): Submit the transaction and grab the DMA cookie.
5. dma_async_issue_pending(chan): To start transmission and wait for a callback notification.
In ASoC, the device tree is used to map the DMA channels to the PCM device.
devm_snd_dmaengine_pcm_register() requests the DMA channel through dmaengine_pcm_request_chan_of(), which is a device-tree-based interface. In order to perform steps 1 to 3, the PCM DMA engine core needs to be provided additional information. This can be done either by populating a struct snd_dmaengine_pcm_
config, which will be given to the registration function or by letting the PCM DMA
engine framework retrieve information from the system's DMA engine core. Steps 4 and 5 are transparently handled by the PCM DMA engine core.
The following is what the struct snd_dma_engine_pcm_config looks like:
```c
struct snd_dmaengine_pcm_config {
int (*prepare_slave_config)(
struct snd_pcm_substream *substream,
struct snd_pcm_hw_params *params,
struct dma_slave_config *slave_config);
```
Writing the platform class driver 267 struct dma_chan *(*compat_request_channel)(
```c
struct snd_soc_pcm_runtime *rtd,
struct snd_pcm_substream *substream);
```
[...]
```c
dma_filter_fn compat_filter_fn;
struct device *dma_dev;
const char *chan_names[SNDRV_PCM_STREAM_LAST + 1];
const struct snd_pcm_hardware *pcm_hardware;
unsigned int prealloc_buffer_size;
};
```
The preceding data structure mainly deals with DMA channel management, buffer management, and channel configuration:
• prepare_slave_config: This callback is used to fill in the DMA slave_
config (of type struct dma_slave_config, which is the DMA slave channel runtime config) for a PCM sub-stream. It will be called from the PCM driver's hwparams callback. Here, you can use snd_dmaengine_pcm_prepare_
slave_config, which is a generic prepare_slave_config callback for platforms that make use of the snd_dmaengine_dai_dma_data struct for their
DAI DMA data. This generic callback will internally call snd_hwparams_to_
```c
dma_slave_config to fill in the slave config based on hw_params, followed by snd_dmaengine_set_config_from_dai_data to fill in the remaining fields based on the DAI DMA data.
```
When using the generic callback approach, you should call snd_soc_dai_
init_dma_data() (given the DAI-specific capture and playback DMA data config, which are of type struct snd_dmaengine_dai_dma_data) from within your CPU DAI driver's .probe callback, which will set both the cpu_
```c
dai->playback_dma_data and cpu_dai->capture_dma_data fields.
```
The snd_soc_dai_init_dma_data() method simply sets the DMA settings
(for either capture, playback, or both) for the DAI given as a parameter.
• compat_request_channel: This is used to request DMA channels for platforms that do not use the device tree. If set, .compat_filter_fn will be ignored.
Writing the platform class driver 269
Though snd_soc_dai_init_dma_data() accepts both capture and playback as void types, the values passed should actually be of type struct snd_dmaengine_
dai_dma_data, defined in include/sound/dmaengine_pcm.h as follows:
```c
struct snd_dmaengine_dai_dma_data {
dma_addr_t addr;
enum dma_slave_buswidth addr_width;
```
u32 maxburst;
```c
unsigned int slave_id;
void *filter_data;
const char *chan_name;
unsigned int fifo_size;
unsigned int flags;
};
```
This structure represents DMA channel data (or config or whatever you prefer) for a DAI
channel. You should refer to the header where it is defined for the meaning of its fields.
Additionally, you can have a look at other drivers for more details on how to set up this data structure.
## PCM hardware configuration
When the DMA settings are not automatically fed from the system by the PCM DMA
engine core, the platform PCM driver may need to provide PCM hardware settings, which describe how hardware lays out the PCM data. Those settings are provided through the snd_dmaengine_pcm_config.pcm_hardware field, which is of type struct snd_pcm_hardware, defined as follows:
```c
struct snd_pcm_hardware {
unsigned int info;
```
u64 formats;
```c
unsigned int rates;
unsigned int rate_min;
unsigned int rate_max;
unsigned int channels_min;
unsigned int channels_max;
```
size_t buffer_bytes_max;
size_t period_bytes_min;
size_t period_bytes_max;
270 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers unsigned int periods_min;
```c
unsigned int periods_max;
```
size_t fifo_size;
```c
};
```
This structure describes the hardware limitations of the platform itself (or should I say,
it sets the allowed parameters), such as the number of channels/sampling rate/data format that can be supported, the range of period size supported by DMA, the range of period counts, and so on. In the preceding data structure, range values, period min, and period max depend on the capabilities of the DMA controller, the DAI hardware, and the codec.
The following are the detailed meanings of each field:
• info contains the type and capabilities of this PCM. The possible values are bit flags that are all defined in include/uapi/sound/asound.h (this means user code should include <sound/asound.h>) as SNDRV_PCM_INFO_XXX.
For example, SNDRV_PCM_INFO_MMAP would mean the hardware supports the mmap() system call. Here, at least, you have to specify whether the mmap system call is supported and which interleaved format is supported. When the mmap()
system call is supported, add the SNDRV_PCM_INFO_MMAP flag here. When the hardware supports the interleaved or the non-interleaved formats, the SNDRV_
PCM_INFO_INTERLEAVED or SNDRV_PCM_INFO_NONINTERLEAVED flag must be set, respectively. If both are supported, you can set both, too.
• The formats field contains the bit flags of supported formats (SNDRV_PCM_
FMTBIT_XXX). If the hardware supports more than one format, you should use all
OR'ed bits.
• The rates field contains the bit flags of supported rates (SNDRV_PCM_RATE_
XXX).
• rate_min and rate_max define the minimum and maximum sample rate. This should correspond somehow to rate bits.
• channel_min and channel_max define, as you might have already guessed, the minimum and the maximum number of channels.
• buffer_bytes_max defines the maximum buffer size in bytes. There is no buffer_bytes_min field since it can be calculated from the minimum period size and the minimum number of periods. Meanwhile, period_bytes_min and period_bytes_max define the minimum and maximum size of the period in bytes.
• periods_max and periods_min define the maximum and the minimum number of periods in the buffer.
Writing the platform class driver 271
The other fields need the concept of a period to be introduced. The period defines the size at which a PCM interrupt is generated. The concept of a period is very important.
A period basically describes an interrupt. It sums up the "chunk" size that the hardware supplies data in:
• period_bytes_min is the minimum transfer size of the DMA written to as the number of bytes processed between interrupts. For example, if the DMA can transmit a minimum of 2,048 bytes, it should be written as 2048.
• period_bytes_max is the maximum transfer size of the DMA aka the maximum number of bytes processed between interrupts. For example, if the DMA can transmit a maximum of 4,096 bytes, it should be written as 4096.
The following is an example of such PCM constraints from the STM32 I2S DMA driver,
defined in sound/soc/stm/stm32_i2s.c:
```c
static const struct snd_pcm_hardware stm32_i2s_pcm_hw = {
```
.info = SNDRV_PCM_INFO_INTERLEAVED | SNDRV_PCM_INFO_MMAP,
.buffer_bytes_max = 8 * PAGE_SIZE,
.period_bytes_max = 2048,
.periods_min = 2,
.periods_max = 8,
```c
};
```
Once set up, this structure should end up in the snd_dmaengine_pcm_config.pcm_
hardware field prior to the struct snd_dmaengine_pcm_config object given to devm_snd_dmaengine_pcm_register().
The following is a playback flow, showing the involved components and PCM data flow:
Figure 5.4 – ASoC audio playback flow
272 ALSA SoC Framework – Leveraging Codec and Platform Class Drivers
The preceding figure shows the audio playback flow and blocks involved in each step.
We can see audio data is copied from the user to DMA buffers, followed by DMA
transactions to move the data into the platform audio Tx FIFO, which, thanks to its link with the codec (via their respective DAIs), sends this data to the codec in charge of playing the audio through the speaker. The capture operation is the opposite flow with the speaker replaced by a microphone.
This brings us to the end of dealing with the platform class driver. We have seen the data structures and concepts it shares with the codec class driver. Note that both codec and platform drivers need to be linked together in order to build the real audio path from a system point of view. According to the ASoC architecture, this has to be done in another class driver, the so-called machine driver, which is the topic of the next chapter.
## Summary
In this chapter, we analyzed the ASoC architecture. On this basis, we dealt with both the codec driver and the platform driver. By learning about these topics, we went through several concepts, such as controls and widgets. We have seen how the ASoC framework differs from the classic PC ALSA system, mostly by targeting code reusability and implementing power management.
Last but not least, we have seen that platform and codec drivers do not work standalone.
They need to be bound together by the machine driver, which is responsible for registering the final audio device, and this is the main topic in the next chapter