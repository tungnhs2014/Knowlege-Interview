# Topic Brief - 35 - ASoC Codec, Component, DAI, And DAPM

## Output Targets

| Learning Path | Slug | Coverage Brief | Knowledge Target | Interview Target | Example Target |
| --- | --- | --- | --- | --- | --- |
| 35 - ASoC Codec, Component, DAI, And DAPM | `asoc-codec-component-dai-and-dapm` | `coverage/topic-briefs/35-asoc-codec-component-dai-and-dapm.md` | `knowledge/35-asoc-codec-component-dai-and-dapm.md` | `interview/35-asoc-codec-component-dai-and-dapm.md` | `examples/35-asoc-codec-component-dai-and-dapm/README.md` |

## Source Coverage

| Source ID | Source Root | File | Status | Coverage Notes |
| --- | --- | --- | --- | --- |
| `ldd1-source-root` | ldd1 | `docs/Linux Device Driver Development/` | searched/mapped/gap | No dedicated ALSA SoC, codec, component, DAI, or DAPM chapter found in book 1. Relevant material is adjacent only: regmap, regulators, device tree, and generic bus/resource context. |
| `ldd1-ch05` | ldd1 | `Chapter 5-Platform Device Drivers.md` | searched/mapped/gap | Contains generic platform-device context and a passing I2S mention, but not ASoC platform/component semantics. Same chapter number as `ldd2-ch05` was not treated as equivalent content. |
| `ldd1-ch09` | ldd1 | `Chapter 9-Regmap API .md` | read/mapped/related | Supplies generic `regmap_config`, I2C/SPI regmap initialization, `regmap_read()`, `regmap_write()`, and `regmap_update_bits()` context used by many codec drivers. It does not teach ASoC. |
| `ldd1-ch20` | ldd1 | `Chapter 20-Regulator Framework.md` | read/mapped/related | Notes codec consumers with analog and digital supplies. Useful for DAPM/regulator-supply context, but generic regulator details remain topic 23. |
| `ldd2-ch02` | ldd2 | `Chapter 2-Regmap_API.md` | read/mapped/related | Explains that regmap was originally developed for ASoC codec drivers to remove duplicated SPI/I2C register access, then expanded to I2C/SPI/MMIO/cache/IRQ use. |
| `ldd2-ch04` | ldd2 | `Chapter 4-Common_Clock_Framework.md` | read/mapped/related | Notes that the following ASoC chapter depends heavily on the clock framework for sampling audio and includes an audio clock generator example. Detailed CCF remains topic 22. |
| `ldd2-ch05` | ldd2 | `Chapter 5-ASoC_Framework.md` | read/mapped/covered/merged-primary | Primary source. Covers ASoC motivation, codec/platform/machine split, modern component model, DAI concepts, codec/component registration, DAI ops, PCM stream constraints, kcontrols, TLV metadata, DAPM widgets, DAPM domains, DAPM routes, and platform CPU-DAI/PCM-DMA context. |
| `ldd2-ch06` | ldd2 | `Chapter 6-ASoC_Machine_Drivers.md` | read/mapped/covered-adjacent | Adjacent boundary source. Explains how machine drivers and simple-card bind codec/CPU DAIs, route codec pins to board connectors, parse DT `audio-routing`, set DAI format/clocks, and register `snd_soc_card`. Full machine-driver teaching remains topic 36. |
| `notion-source-root` | notion | `docs/Linux-Device-Driver-Notion/` | searched/mapped/gap | No standalone Notion ASoC chapter found. Relevant snippets are DT/I2C/regmap-IRQ examples involving audio codecs. Notion was searched and not skipped. |
| `notion-ch06-part1` | notion | `Chapter 6-Part 1 Device Tree Fundamentals.md` | read/mapped/related | Contains an I2C audio codec DT node (`sgtl5000@0a`) with clock and supply properties. Useful as codec resource context only. |
| `notion-ch06-part2` | notion | `Chapter 6-Part 2 Device Addressing and Resources.md` | read/mapped/related | Contains another audio codec DT example at I2C address `0x0a`. Read separately from Part 1 and not assumed duplicate without comparison. |
| `notion-ch07-part1` | notion | `Chapter 7-Part 1 I2C Architecture and Driver Structures.md` | read/mapped/related | Lists audio codecs as common I2C devices. Useful for bus-placement context only. |
| `notion-ch07-part3` | notion | `Chapter 7-Part 3 Device Tree Integration.md` | read/mapped/related | Contains a `wm8962@1a` codec DT example with multiple supplies and clock/reference properties. Useful for codec resource and DT-binding context. |
| `notion-ch16-part3` | notion | `Chapter 16-Part 3 Advanced Topics.md` | read/mapped/related | Mentions audio codecs as a use case for regmap IRQ when a device has multiple interrupt sources. Detailed regmap IRQ remains topic 19. |
| `local-linux-6.8-headers` | external/local | `/lib/modules/6.8.0-124-generic/build/include/sound/` | read/mapped/validation | Confirmed current locations and signatures for `struct snd_soc_component_driver`, `struct snd_soc_dai_driver`, `struct snd_soc_dai_ops`, DAPM widgets/routes, component I/O helpers, and `devm_snd_soc_register_component()`. |
| `kernel-doc-asoc-overview` | external/official | `https://docs.kernel.org/sound/soc/overview.html` | read/mapped/validation | Validates current ASoC design goals: codec independence, DAI setup, DAPM, pop/click reduction, and machine-specific controls. |
| `kernel-doc-asoc-codec` | external/official | `https://docs.kernel.org/sound/soc/codec.html` | read/mapped/validation | Validates codec-driver responsibilities: DAI/PCM configuration, regmap I/O, mixers/controls, audio operations, DAPM, DAPM events, and optional digital mute. |
| `kernel-doc-asoc-dapm` | external/official | `https://docs.kernel.org/sound/soc/dapm.html` | read/mapped/validation | Validates DAPM as a graph of widgets/routes, domains, static component-driver DAPM registration, route syntax, stream-name matching, and current widget categories. |

## Source Files Read

- `Linux-Device-Driver/CODEX.md`
- `Linux-Device-Driver/LEARNING_PATH.md`
- `Linux-Device-Driver/.agents/skills/linux-device-driver/SKILL.md`
- `Linux-Device-Driver/.agents/skills/linux-device-driver/references/topic-brief-template.md`
- `Linux-Device-Driver/.codex/agents/lld-topic-explorer.toml`
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 9-Regmap API .md`
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 20-Regulator Framework.md`
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 2-Regmap_API.md`
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 4-Common_Clock_Framework.md`
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 5-ASoC_Framework.md`
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 6-ASoC_Machine_Drivers.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 6-Part 1 Device Tree Fundamentals.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 6-Part 2 Device Addressing and Resources.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 7-Part 1 I2C Architecture and Driver Structures.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 7-Part 3 Device Tree Integration.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 16-Part 3 Advanced Topics.md`
- Local Linux headers under `/lib/modules/6.8.0-124-generic/build/include/sound/`: `soc.h`, `soc-component.h`, `soc-dai.h`, `soc-dapm.h`, and adjacent ASoC headers.

## Inventory Decisions

- `ldd2-ch05` is the only dedicated internal source for ASoC codec/component/DAI/DAPM and is the primary base for this topic.
- `ldd2-ch06` is adjacent but important. Topic 35 should explain the boundary: codec/component/DAI/DAPM drivers expose reusable hardware description and operations; machine drivers or simple-card bind those pieces into an actual sound card. Deep machine-driver routing remains topic 36.
- Book 1 has no ASoC chapter. `ldd1-ch09` and `ldd1-ch20` were retained only as adjacent regmap/regulator context because codec drivers commonly use regmap-backed register access and DAPM can model supplies.
- Notion has no ASoC chapter. Notion snippets were read separately and mapped as related DT/I2C/regmap-IRQ evidence rather than skipped.
- Same-number chapters were not merged by number. `ldd1-ch05` is generic platform-device material, while `ldd2-ch05` is the ASoC framework chapter.
- Apparent duplicate Notion codec DT snippets in `Chapter 6-Part 1` and `Chapter 6-Part 2` were read separately and compared. They both show an I2C codec node, but Part 1 places it in a broader board DTS walkthrough while Part 2 emphasizes address/resource examples.
- Generic uses of "component", "route", "codec", or "audio" in V4L2, media, workqueue, and platform sources were not treated as ASoC sources unless they had an explicit ASoC, codec-driver, DAI, DAPM, sound-card, or codec DT relationship.

## Merged Source Notes

### Core Mental Model

- ASoC is the Linux ALSA layer for embedded audio systems where CPU audio interfaces, external/internal codecs, clocks, supplies, jacks, amplifiers, and board routing vary independently.
- The reusable parts are codec/component drivers and platform/CPU-DAI/PCM-DMA drivers. The board-specific part is the machine driver, or a generic machine driver such as simple-card when DT can describe the whole link.
- Topic 35 owns the reusable codec/component/DAI/DAPM side: audio controls, stream capabilities, register access, DAI callbacks, PCM constraints, and the power/routing graph inside and around components.
- Topic 36 owns the board-level sound-card side: `snd_soc_card`, `snd_soc_dai_link`, CPU/codec/platform binding, simple-card, board widgets, and board-level clock/routing policy.

### Why ASoC Exists

- Older ALSA audio drivers were often tightly coupled to a specific CPU/platform and powered large parts of a codec for simple playback or capture.
- ASoC separates codec, CPU/platform, and machine concerns so the same codec driver can be reused on many boards and the same CPU DAI driver can work with many codecs.
- ASoC adds embedded-audio-specific behavior: DAI setup, DAPM power sequencing, machine-specific controls/events, jack/amp handling, pop/click reduction, and stream-aware power control.

### Codec And Component Model

- The book explicitly notes the pre-v4.18 split between `struct snd_soc_codec` and `struct snd_soc_platform`, then the shift to generic `struct snd_soc_component` and `struct snd_soc_component_driver`.
- Modern learning should lead with `struct snd_soc_component_driver`, not the old codec/platform object split.
- A codec driver should be platform-independent. It describes the codec's DAIs, PCM stream capabilities, controls, register I/O, optional DAPM graph, and optional digital mute behavior.
- Component registration uses `devm_snd_soc_register_component(dev, component_driver, dai_driver, num_dai)`. The component and DAI drivers become visible to the ASoC core and are later bound by a machine driver/card.
- The component `probe()` in ASoC is card-instantiation time, not necessarily the same moment as the I2C/SPI/platform bus probe that created the regmap and called component registration.

### DAI And PCM Capability Model

- DAI means Digital Audio Interface: the bus/controller/path used to carry audio samples between the CPU side and the codec side.
- Common DAI formats/protocols include I2S, left/right justified, AC97, PCM/DSP modes, S/PDIF, PDM, and TDM-like slotting.
- `struct snd_soc_dai_driver` describes each DAI exposed by a component. Important fields include `name`, `ops`, `playback`, `capture`, and symmetry constraints.
- `struct snd_soc_pcm_stream` describes playback/capture capabilities: stream name, channel count, supported rates, formats, and sample-bit constraints.
- DAI ops are split conceptually into clock/format setup (`set_sysclk`, `set_pll`, `set_clkdiv`, `set_bclk_ratio`, `set_fmt`, `set_tdm_slot`, channel map, tristate), stream lifecycle (`startup`, `shutdown`, `hw_params`, `hw_free`, `prepare`, `trigger`, `delay`), and mute/stream-specific hooks.
- Machine or card code usually calls helpers such as `snd_soc_dai_set_fmt()`, `snd_soc_dai_set_sysclk()`, `snd_soc_dai_set_pll()`, `snd_soc_dai_set_clkdiv()`, and `snd_soc_dai_set_tdm_slot()`, which delegate to the CPU or codec DAI's ops.
- Current headers use `mute_stream` in `struct snd_soc_dai_ops`; older sources and docs may mention `digital_mute` or codec mute callbacks. The learner docs should explain this as version-sensitive naming/API drift.

### Register I/O And Controls

- Codecs are often controlled through I2C or SPI while audio samples flow over a DAI. AC97 is a special case where control and data are combined.
- Regmap is the expected register I/O abstraction for most modern codec drivers. Book 2 explicitly says regmap originated in ASoC codec driver cleanup.
- Codec controls are ALSA controls exposed to userspace tools such as `amixer` and `alsamixer`.
- Common control macros include `SOC_SINGLE`, `SOC_SINGLE_TLV`, `SOC_DOUBLE_R`, `SOC_DOUBLE_R_TLV`, `SOC_ENUM`, and `SOC_ENUM_SINGLE`.
- TLV metadata describes dB scales for volume controls. Control naming should follow ALSA conventions such as `Playback Volume`, `Capture Switch`, `Line`, `Mic`, `PCM`, `Master`, and `Route`.
- Component I/O helpers include `snd_soc_component_read()`, `snd_soc_component_write()`, `snd_soc_component_update_bits()`, and `snd_soc_component_test_bits()`. Current headers also expose field and async helpers.
- Static controls can be listed in `struct snd_soc_component_driver.controls` and `num_controls`; dynamic controls can be added with ASoC control registration helpers when hardware variants require it.

### DAPM Model

- DAPM means Dynamic Audio Power Management. It models audio hardware as a graph and powers only the widgets that are needed for active streams and selected mixer/mux paths.
- DAPM is transparent to userspace. Userspace changes normal mixer settings or starts/stops streams; the ASoC core computes power changes from the graph.
- The graph is made of widgets and routes. A widget is a controllable hardware or logical audio block; a route connects a sink widget to a source widget, optionally through a named control.
- Main DAPM domains:
  - Codec bias domain: VREF/VMID/core codec power.
  - Platform/machine domain: pins, jacks, speakers, microphones, external amplifiers, and board-specific events.
  - Path domain: muxes, mixers, PGAs, internal signal paths.
  - Stream domain: ADCs, DACs, AIFs, DAI widgets, and clocks tied to stream start/stop.
- Important widget classes include `SND_SOC_DAPM_INPUT`, `SND_SOC_DAPM_OUTPUT`, `SND_SOC_DAPM_MIC`, `SND_SOC_DAPM_HP`, `SND_SOC_DAPM_SPK`, `SND_SOC_DAPM_LINE`, `SND_SOC_DAPM_MIXER`, `SND_SOC_DAPM_MUX`, `SND_SOC_DAPM_PGA`, `SND_SOC_DAPM_DAC`, `SND_SOC_DAPM_ADC`, `SND_SOC_DAPM_AIF_IN`, `SND_SOC_DAPM_AIF_OUT`, `SND_SOC_DAPM_SUPPLY`, `SND_SOC_DAPM_REGULATOR_SUPPLY`, and `SND_SOC_DAPM_CLOCK_SUPPLY`.
- DAPM controls use `SOC_DAPM_*` macros, not ordinary `SOC_*` macros, so DAPM can recompute paths and power state when the control changes.
- Static DAPM arrays can live in `struct snd_soc_component_driver.dapm_widgets`, `num_dapm_widgets`, `dapm_routes`, and `num_dapm_routes`.
- Dynamic DAPM registration uses `snd_soc_dapm_new_controls()` and `snd_soc_dapm_add_routes()` on a `struct snd_soc_dapm_context`, often obtained from the component.
- Route syntax is sink/control/source. A direct connection uses `NULL` control. A mixer-controlled route uses the mixer control name as the middle field.
- Stream-name matching matters: DAPM stream widgets must match the corresponding DAI stream names so the core can power ADC/DAC/AIF widgets with playback/capture state.

### Lifecycle And Data Flow

- Bus probe, such as I2C or SPI codec probe:
  - Allocate private state.
  - Initialize regmap, clocks, supplies, reset GPIOs, IRQs, and variant data as needed.
  - Register the ASoC component and one or more DAI drivers with `devm_snd_soc_register_component()`.
- Card instantiation:
  - A machine driver or simple-card binds CPU DAI, codec DAI, and platform/PCM component into a `snd_soc_card`.
  - ASoC probes component and DAI instances for the card, creates PCM devices, and builds the DAPM graph.
- Playback/capture open and configure:
  - ALSA opens a PCM substream.
  - Machine/card and DAI callbacks constrain/configure sample rate, format, channels, clocks, PLLs, TDM slots, and bus format.
  - DAPM prepares the widgets/routes required by the chosen stream and mixer state.
- Runtime stream:
  - PCM DMA/platform code moves audio samples between memory and CPU DAI FIFO.
  - CPU DAI sends/receives samples over I2S/TDM/PCM/S/PDIF/etc.
  - Codec DAI, ADC, DAC, mixers, muxes, PGAs, and pins transform and route the audio.
  - DAPM powers the path on/off as streams and mixer controls change.
- Stop/remove:
  - Streams stop, DAPM powers down unused widgets in sequence, DAIs and component callbacks unwind, and managed registration/resources clean up when the device is removed.

### Platform/CPU-DAI/PCM-DMA Boundary

- `ldd2-ch05` also teaches the platform side: CPU DAI driver plus PCM DMA driver.
- This topic should mention the boundary enough to explain where codec DAI callbacks meet CPU DAI callbacks.
- Deep PCM DMA implementation, DMAengine configuration, and `snd_dmaengine_pcm_config` details should be referenced only lightly and left to DMA/platform audio follow-up unless needed for DAI examples.

### Machine Boundary

- A codec/component driver alone does not create a sound card.
- The machine driver or simple-card supplies the `snd_soc_dai_link`, board connectors, final routes, codec pin selection, DAI format, clocks, and card registration.
- `ldd2-ch06` shows codec pins such as `MIC_IN`, `HP_OUT`, and board connectors such as `Mic Jack` and `Headphone Jack`, with DT `audio-routing` pairs parsed by machine code.
- Topic 35 should explain that DAPM routes may span components and the whole card, but detailed `snd_soc_card`/`snd_soc_dai_link` teaching belongs to topic 36.

## Source Differences

- Version baseline:
  - `ldd2-ch05` targets Linux v4.19-era ASoC. Current local Linux 6.8 headers still support the component model but some structure fields and callback names have changed.
  - Older source excerpts include `pcm_new`/`pcm_free` wording in `struct snd_soc_component_driver`; current headers use `pcm_construct`/`pcm_destruct`.
  - Older material discusses `digital_mute`; current `struct snd_soc_dai_ops` uses `mute_stream` for DAI mute handling.
- Component model:
  - The book itself notes a major pre-v4.18 to post-v4.18 transition from strict codec/platform objects to generic components. Final learner docs should treat old names as historical context, not the main API.
- DAPM registration:
  - The book shows both explicit `snd_soc_dapm_new_controls()` / `snd_soc_dapm_add_routes()` and component-driver arrays. Current kernel docs emphasize static arrays in `struct snd_soc_component_driver` as the common path, with dynamic registration for variant-dependent cases.
- Widget set:
  - Current `soc-dapm.h` includes additional widgets such as regulator, clock, pinctrl, DAI link, DSP/internal pipeline-oriented types, and event variants beyond the simpler examples in the book.
- Regmap:
  - Book 1 uses unmanaged `regmap_init_i2c()` / `regmap_init_spi()` examples. Current production codec drivers normally prefer devm regmap helpers unless a special lifetime is required.
- Notion scope:
  - Notion content gives real codec DT examples but does not explain ASoC registration, DAIs, kcontrols, or DAPM. It is supporting context, not a duplicate of `ldd2-ch05`.

## Gaps / Uncertainties

- Internal sources do not provide a complete current Linux 6.x codec driver skeleton using devm regmap, component-driver static controls/DAPM arrays, current `mute_stream`, clocks, regulators, runtime PM, and variant data together.
- Internal sources do not deeply cover jack detection, button reporting, `struct snd_soc_jack`, headset accessory detection, or IRQ integration for codecs.
- Internal sources do not deeply cover Sound Open Firmware, topology files, DPCM front-end/back-end links, codec-to-codec links, HDMI/DisplayPort audio codecs, Bluetooth/FM/modem codec roles, or DSP component widgets.
- Internal sources do not give a hardware datasheet. Real codec work still depends on register maps, reset timing, power sequencing, clock tree, PLL rules, DAI timing diagrams, analog path diagrams, and board schematics.
- Internal sources do not deeply cover userspace ALSA control ABI stability, UCM profiles, PulseAudio/PipeWire integration, or distro audio policy.
- Topic 35 must avoid swallowing topic 36. Machine-driver details should be included only when needed to explain why component/codec/DAI/DAPM definitions are not sufficient by themselves.

## External Validation

External validation was used because ASoC APIs are version-sensitive and the primary book source is based around Linux v4.19.

| External Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/sound/soc/overview.html` | Validate current ASoC architecture, design goals, and split between codec, platform, and machine drivers. |
| `https://docs.kernel.org/sound/soc/codec.html` | Validate codec-driver responsibilities, DAI/PCM capability definitions, regmap I/O, control macros, DAPM description, event handling, and mute behavior. |
| `https://docs.kernel.org/sound/soc/dapm.html` | Validate current DAPM graph model, power domains, widget categories, route syntax, stream-name matching, static component-driver DAPM registration, and event types. |
| `/lib/modules/6.8.0-124-generic/build/include/sound/soc.h` | Confirm current component registration APIs such as `devm_snd_soc_register_component()` and ASoC card/DAPM declarations. |
| `/lib/modules/6.8.0-124-generic/build/include/sound/soc-component.h` | Confirm current `struct snd_soc_component_driver` fields and component I/O helpers. |
| `/lib/modules/6.8.0-124-generic/build/include/sound/soc-dai.h` | Confirm current `struct snd_soc_dai_driver`, `struct snd_soc_dai_ops`, PCM stream capability fields, and callback naming such as `mute_stream`. |
| `/lib/modules/6.8.0-124-generic/build/include/sound/soc-dapm.h` | Confirm current DAPM widget macros, route/path/widget structures, event flags, and DAPM registration helpers. |

## Learning Content Brief

### What To Teach

- ASoC's purpose and the codec/platform/machine split.
- The modern component model: `struct snd_soc_component`, `struct snd_soc_component_driver`, and why old codec/platform terminology still appears in docs and drivers.
- Codec driver responsibilities: regmap-backed register I/O, controls, DAI definitions, PCM capabilities, DAPM widgets/routes, events, clocks, supplies, and optional mute.
- DAI meaning and lifecycle: supported formats/rates/channels, `snd_soc_dai_driver`, `snd_soc_dai_ops`, `snd_soc_pcm_stream`, and callbacks used during startup/hw_params/prepare/trigger/shutdown.
- ALSA controls and ASoC control macros, including TLV volume metadata and naming conventions.
- DAPM graph mental model: widgets, routes, domains, event ordering, path recomputation, and how ordinary controls differ from DAPM controls.
- How codec/component definitions become useful only after a machine driver/simple-card binds CPU DAI, codec DAI, and routing into a sound card.
- Practical debug flow: `aplay -l`, `arecord -l`, `amixer`, `alsamixer`, `/sys/kernel/debug/asoc`, DAPM debugfs, dmesg, dynamic debug, clock/regulator checks, and scope/logic-analyzer checks for BCLK/LRCLK/MCLK when hardware is available.

### Why It Exists

- Embedded audio hardware is highly board-specific but many codecs and SoC DAIs are reusable.
- ASoC lets reusable codec and CPU-DAI drivers be combined by small board descriptions.
- DAPM reduces power and pop/click problems by sequencing only the active audio path instead of letting userspace manually toggle register bits.

### When To Use It

- Use ASoC component/codec APIs when writing a driver for an audio codec, audio interface, amplifier, DSP, or digital audio component that participates in an ALSA SoC sound card.
- Use a codec/component driver when the hardware block is reusable across boards.
- Use a machine driver or simple-card when the problem is board wiring, codec-to-CPU DAI binding, connector routing, clock master/slave policy, jack GPIOs, or amplifier GPIOs.
- Do not expose codec registers through a private char device for normal audio use; expose ALSA controls and DAPM routes through ASoC.

### Key Structures And APIs

- Core/component: `struct snd_soc_component`, `struct snd_soc_component_driver`, `devm_snd_soc_register_component()`.
- DAI: `struct snd_soc_dai`, `struct snd_soc_dai_driver`, `struct snd_soc_dai_ops`, `struct snd_soc_pcm_stream`.
- Controls: `struct snd_kcontrol_new`, `SOC_SINGLE`, `SOC_SINGLE_TLV`, `SOC_DOUBLE_R`, `SOC_DOUBLE_R_TLV`, `SOC_ENUM`, `SOC_ENUM_SINGLE`, `DECLARE_TLV_DB_SCALE`.
- Component I/O: `snd_soc_component_read()`, `snd_soc_component_write()`, `snd_soc_component_update_bits()`, `snd_soc_component_test_bits()`, plus regmap initialization in the bus probe.
- DAPM: `struct snd_soc_dapm_widget`, `struct snd_soc_dapm_route`, `struct snd_soc_dapm_context`, `SND_SOC_DAPM_*` widget macros, `SOC_DAPM_*` control macros, `snd_soc_dapm_new_controls()`, `snd_soc_dapm_add_routes()`.
- DAI helpers: `snd_soc_dai_set_fmt()`, `snd_soc_dai_set_sysclk()`, `snd_soc_dai_set_pll()`, `snd_soc_dai_set_clkdiv()`, `snd_soc_dai_set_tdm_slot()`.

### Practical Example Direction

- A minimal later example should be learning-only and can be a skeleton codec/component driver rather than a fake working hardware driver.
- The example should show:
  - I2C probe creates regmap and registers the component.
  - Static `snd_soc_component_driver` with controls, widgets, and routes.
  - One `snd_soc_dai_driver` with playback/capture capability.
  - Minimal `snd_soc_dai_ops` for `hw_params`, `set_fmt`, `set_sysclk`, and `mute_stream`.
  - Clear note that a machine driver or simple-card DT is still required to create a sound card.
- A pure README example may be more useful than buildable code if no real codec hardware or binding is present.

### Common Bugs And Debugging

- Treating the I2C/SPI bus probe as proof that the ALSA sound card is registered. Component registration and card binding are separate steps.
- Forgetting that component/DAI probes can happen during card instantiation, after the bus driver has already initialized private state.
- Mismatched DAI stream names and DAPM stream widget names, causing ADC/DAC/AIF widgets not to power correctly.
- Using ordinary `SOC_*` controls for a path that should be DAPM-aware.
- Incorrect DAPM route direction. Route entries are sink/control/source, not source/control/sink.
- Missing codec pins or board routes, leading to silent playback even when PCM data flows.
- Wrong clock master/slave, BCLK/LRCLK polarity, sample format, MCLK rate, PLL source, or TDM slot mask.
- Updating codec registers directly without regmap/cache awareness, especially around suspend/resume or volatile registers.
- Not modeling supplies, clocks, amplifiers, or GPIO-controlled bias in DAPM, causing pops, high idle current, or silent paths.
- Exposing unstable or incorrectly named ALSA controls that become hard userspace ABI.
- Overmixing topic 35 and topic 36: codec/component drivers should not hard-code board-specific connector routing when it belongs in the machine layer.

### Production Checklist

- Confirm the codec datasheet register map, reset defaults, volatile/precious registers, cache policy, page/windowing model, and endianness.
- Use devm regmap and managed resource allocation where lifetime permits.
- Define controls with stable ALSA names and correct TLV dB metadata.
- Define DAPM widgets/routes from the real analog/digital path diagram.
- Ensure DAPM stream widget names match DAI stream names.
- Validate clocks: MCLK, BCLK, LRCLK, PLL source, dividers, master/slave, and sample-rate families.
- Validate supplies and reset sequencing, including regulator enable delays and bias paths.
- Test playback, capture, mute, mixer paths, suspend/resume, stream reopen, format/rate changes, and route changes.
- Check debugfs DAPM power state, dmesg, dynamic debug, actual clocks on pins, and current consumption.
- Keep machine-specific connector names, amp GPIOs, jack GPIOs, and board audio routing outside the reusable codec driver unless the hardware component genuinely owns them.

### Interview Readiness Targets

- Be able to explain ASoC's three-way split and why the component model replaced older codec/platform object separation.
- Be able to sketch a minimal codec driver: regmap, component driver, DAI driver, controls, widgets, routes, registration.
- Be able to explain what DAI ops are called for and why machine/card code often calls `snd_soc_dai_set_fmt()` and clock helpers.
- Be able to debug "codec probes but no sound card", "PCM plays but no audio", "DAPM widget never powers", "wrong sample rate/format", and "high idle power".
- Be able to distinguish normal ALSA mixer controls from DAPM controls and explain why route direction matters.
- Be able to state which material belongs to the next topic: `snd_soc_card`, `snd_soc_dai_link`, simple-card, board routing, and sound-card registration.
