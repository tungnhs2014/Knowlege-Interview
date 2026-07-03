# Topic Brief - 36 - ASoC Machine Drivers And Audio Routing

## Output Targets

| Learning Path | Slug | Coverage Brief | Knowledge Target | Interview Target | Example Target |
| --- | --- | --- | --- | --- | --- |
| 36 - ASoC Machine Drivers And Audio Routing | `asoc-machine-drivers-and-audio-routing` | `coverage/topic-briefs/36-asoc-machine-drivers-and-audio-routing.md` | `knowledge/36-asoc-machine-drivers-and-audio-routing.md` | `interview/36-asoc-machine-drivers-and-audio-routing.md` | `examples/36-asoc-machine-drivers-and-audio-routing/README.md` |

## Source Coverage

| Source ID | Source Root | File | Status | Coverage Notes |
| --- | --- | --- | --- | --- |
| `ldd1-source-root` | ldd1 | `docs/Linux Device Driver Development/` | `searched/mapped/gap` | No dedicated ALSA SoC, machine-driver, sound-card, DAI-link, simple-card, or audio-routing chapter found in book 1. Relevant material is adjacent only: platform driver basics, Device Tree phandles/resources, clocks, and regulator supplies. |
| `ldd1-ch05` | ldd1 | `Chapter 5-Platform Device Drivers.md` | `read/mapped/related` | Generic platform-driver probe/match/register context and a passing I2S/platform-driver example from `sound/soc/fsl/imx-ssi.c`; useful because custom ASoC machine drivers are usually platform drivers, but it does not teach ASoC card binding. |
| `ldd1-ch06` | ldd1 | `Chapter 6-The Concept of a Device Tree .md` | `read/mapped/related` | Phandle labels, named resources, clocks, DMA names, and arbitrary DT properties. Useful for understanding `audio-codec`, `ssi-controller`, `sound-dai`, `clocks`, `dma-names`, and board sound nodes. |
| `ldd1-ch20` | ldd1 | `Chapter 20-Regulator Framework.md` | `read/mapped/related` | Codec consumer supply and `*-supply` binding context. Useful for board audio power sequencing and DAPM regulator-supply context, but full regulator behavior remains topic 23. |
| `ldd2-ch04` | ldd2 | `Chapter 4-Common_Clock_Framework.md` | `read/mapped/related` | Notes that ASoC heavily relies on clocks for audio sampling. Machine drivers often choose MCLK/BCLK/LRCLK mastership and call DAI clock helpers. Full CCF remains topic 22. |
| `ldd2-ch05` | ldd2 | `Chapter 5-ASoC_Framework.md` | `read/mapped/covered-adjacent` | Prerequisite/boundary source: codec/component/CPU-DAI/PCM-DMA drivers are reusable pieces and do not create a sound card alone. Machine drivers bind them with `snd_soc_dai_link` and `snd_soc_card`. Full codec/component/DAPM coverage is topic 35. |
| `ldd2-ch06` | ldd2 | `Chapter 6-ASoC_Machine_Drivers.md` | `read/mapped/covered/merged-primary` | Primary source. Covers machine-driver purpose, `struct snd_soc_dai_link`, CPU/codec/platform references, phandle parsing, board widgets, DT/static routing, `struct snd_soc_ops`, DAI format/clock helpers, `struct snd_soc_card`, `devm_snd_soc_register_card()`, simple-card, and codec-less/SPDIF-style cards. |
| `notion-source-root` | notion | `docs/Linux-Device-Driver-Notion/` | `searched/mapped/gap` | No standalone Notion ASoC machine-driver or audio-routing chapter found. Notion was searched and not skipped; relevant DT/platform/I2C/regmap-IRQ snippets were read separately. |
| `notion-ch05-part1` | notion | `Chapter 5-Part 1 Platform Bus & Driver Basics.md` | `read/mapped/related` | Platform-driver mental model: `probe()` per matched device, `remove()`, `module_platform_driver()`, deferred probe, OF matching. Adjacent because custom machine drivers are platform drivers. |
| `notion-ch05-part2` | notion | `Chapter 5-Part 2 Probe, Remove & Resource Management.md` | `read/mapped/related` | Probe/remove and resource-management workflow; useful for machine-driver lifetime and devm registration context. No ASoC-specific card binding. |
| `notion-ch06-part1` | notion | `Chapter 6-Part 1 Device Tree Fundamentals.md` | `read/mapped/related` | Board DTS example containing `sgtl5000@0a` codec node with clock and supplies. Related codec resource context only. |
| `notion-ch06-part2` | notion | `Chapter 6-Part 2 Device Addressing and Resources.md` | `read/mapped/related` | Audio-related `sai4: audio@50027000` example plus another `sgtl5000@0a` I2C codec node with `clocks` and supply. Read separately from Part 1; not assumed duplicate. |
| `notion-ch06-part3` | notion | `Chapter 6-Part 3 OF APIs and Platform Integration.md` | `read/mapped/related` | OF matching, `of_device_id`, phandle parsing, named resources, `devm_*`, and platform-driver integration. Useful for custom machine-driver probe and DT parsing. |
| `notion-ch07-part1` | notion | `Chapter 7-Part 1 I2C Architecture and Driver Structures.md` | `read/mapped/related` | Lists audio codecs as common I2C devices. Useful for separating codec bus binding from machine-driver card binding. |
| `notion-ch07-part3` | notion | `Chapter 7-Part 3 Device Tree Integration.md` | `read/mapped/related` | `wm8962@1a` codec DT example with clock and multiple supplies. Related resource/binding context; no sound-card node or DAI-link teaching. |
| `notion-ch16-part3` | notion | `Chapter 16-Part 3 Advanced Topics.md` | `read/mapped/related` | Mentions audio codecs with multiple interrupt sources as regmap IRQ use cases. Related to codec support hardware, not machine routing. |
| `local-linux-6.8-headers` | external/local | `/lib/modules/6.8.0-124-generic/build/include/sound/` | `read/mapped/validation` | Validated current `snd_soc_dai_link_component`, `struct snd_soc_dai_link`, `SND_SOC_DAILINK_DEFS`, `SND_SOC_DAILINK_REG`, `COMP_CPU`, `COMP_CODEC`, `COMP_PLATFORM`, `struct snd_soc_card`, card registration, OF parsing helpers, simple-card utility helpers, and current DAI format names/aliases. |

## Source Files Read

- `Linux-Device-Driver/CODEX.md`
- `Linux-Device-Driver/LEARNING_PATH.md`
- `Linux-Device-Driver/.agents/skills/linux-device-driver/SKILL.md`
- `Linux-Device-Driver/.agents/skills/linux-device-driver/references/topic-brief-template.md`
- `Linux-Device-Driver/.codex/agents/lld-topic-explorer.toml`
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 5-Platform Device Drivers.md`
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 6-The Concept of a Device Tree .md`
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 20-Regulator Framework.md`
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 4-Common_Clock_Framework.md`
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 5-ASoC_Framework.md`
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 6-ASoC_Machine_Drivers.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 5-Part 1 Platform Bus & Driver Basics.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 5-Part 2 Probe, Remove & Resource Management.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 6-Part 1 Device Tree Fundamentals.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 6-Part 2 Device Addressing and Resources.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 6-Part 3 OF APIs and Platform Integration.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 7-Part 1 I2C Architecture and Driver Structures.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 7-Part 3 Device Tree Integration.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 16-Part 3 Advanced Topics.md`
- Local Linux headers under `/lib/modules/6.8.0-124-generic/build/include/sound/`: `soc.h`, `soc-dai.h`, `soc-dapm.h`, `soc-card.h`, and `simple_card_utils.h`.

## Inventory Decisions

- `ldd2-ch06` is the only dedicated internal source for ASoC machine drivers and is the primary base for this topic.
- `ldd2-ch05` is not a duplicate of `ldd2-ch06`; it explains the reusable codec/component/CPU-DAI/PCM-DMA pieces that machine drivers bind. It was read as a boundary source because topic 36 depends on the topic 35 object model.
- Book 1 has no ASoC machine-driver source. `ldd1-ch05`, `ldd1-ch06`, and `ldd1-ch20` were retained as adjacent platform/DT/supply context, not as ASoC coverage.
- Notion has no standalone ASoC source. `notion-ch05-*` and `notion-ch06-*` were retained because they improve the platform-probe and Device Tree phandle mental model needed for custom machine drivers.
- Same-number chapters were not merged by number. `ldd1-ch05` and `notion-ch05-*` teach generic platform drivers, while `ldd2-ch05` teaches ASoC codec/component/DAI/DAPM.
- Apparent duplicate codec DT snippets in Notion Chapter 6 Part 1 and Part 2 were read separately and compared. Both show I2C codec nodes, but Part 2 also shows an audio controller resource example.
- Generic hits for "audio" in non-ASoC sources were treated as adjacent only unless they contained explicit ASoC, sound-card, DAI, codec DT, or machine-driver concepts.

## Merged Source Notes

### Core Mental Model

- ASoC splits embedded audio into reusable components and board-specific glue:
  - Codec/component drivers expose codec pins, DAIs, controls, DAPM widgets/routes, and register operations.
  - CPU DAI and PCM/DMA drivers expose the SoC audio interface and memory-to-FIFO path.
  - The machine driver or generic simple-card description binds those reusable parts into one ALSA sound card.
- A machine driver is the board wiring description in kernel form. It answers: which CPU DAI talks to which codec DAI, which platform/PCM/DMA device backs the stream, which pins are connected to actual jacks/speakers/mics, who is clock master, and what clocks/PLL/sysclk settings the link needs.
- A codec driver can be perfect and still produce no `/dev/snd/pcm*` devices until a sound card is registered with valid DAI links.

### Why Machine Drivers Exist

- Codec and CPU DAI drivers are designed to be reusable across many boards. The board-specific facts are not reusable:
  - Which codec is connected to which I2S/TDM/PCM/S/PDIF controller.
  - Which DAI names must be paired.
  - Whether the codec or CPU provides BCLK/LRCLK/MCLK.
  - Which sample rates require which PLL/sysclk settings.
  - Which codec pins go to "Headphone Jack", "Mic Jack", "Speaker", or an amplifier.
  - Which GPIO turns on an amplifier or reports jack insertion.
- The machine layer captures this board policy with `struct snd_soc_dai_link`, `struct snd_soc_card`, DAPM widgets/routes, and machine-level callbacks.

### DAI Link Responsibilities

- `struct snd_soc_dai_link` is the logical link between CPU-side DAI(s), codec-side DAI(s), and optional platform/PCM component(s).
- Older book excerpts show direct fields such as `cpu_of_node`, `codec_of_node`, `platform_of_node`, `cpu_dai_name`, and `codec_dai_name`.
- Current Linux 6.8 headers model these endpoints with `struct snd_soc_dai_link_component` arrays:
  - `link->cpus` / `num_cpus`
  - `link->codecs` / `num_codecs`
  - `link->platforms` / `num_platforms`
  - each component has `.name`, `.of_node`, `.dai_name`, and optional `.dai_args`
- Current helper macros include `SND_SOC_DAILINK_DEFS()`, `SND_SOC_DAILINK_DEF()`, `SND_SOC_DAILINK_REG()`, `DAILINK_COMP_ARRAY()`, `COMP_CPU()`, `COMP_CODEC()`, `COMP_PLATFORM()`, and `COMP_DUMMY()`.
- Important DAI link fields and concepts:
  - `name` and `stream_name` identify the link and stream.
  - `dai_fmt` stores audio protocol, clock provider/consumer relationship, and signal inversion flags.
  - `ops` points to machine-level `struct snd_soc_ops`.
  - `init` can add link-specific controls/widgets or initialize runtime link behavior.
  - `playback_only` and `capture_only` handle unidirectional links such as S/PDIF.
  - DPCM and codec-to-codec fields exist for more advanced routing but should not dominate this introductory machine-driver topic.

### Sound Card Responsibilities

- `struct snd_soc_card` is the ASoC representation of the board sound card.
- Important fields:
  - `name`, `long_name`, `driver_name`, `dev`, and `owner`.
  - `dai_link` and `num_links`.
  - `controls` and `num_controls`.
  - `dapm_widgets` / `num_dapm_widgets`.
  - `dapm_routes` / `num_dapm_routes`.
  - `of_dapm_widgets` / `num_of_dapm_widgets`.
  - `of_dapm_routes` / `num_of_dapm_routes`.
  - card callbacks such as `probe`, `late_probe`, `remove`, suspend/resume hooks, and bias-level hooks.
- Registration uses `devm_snd_soc_register_card(dev, card)` or the unmanaged `snd_soc_register_card()` / `snd_soc_unregister_card()` pair.
- When the card is registered, ASoC resolves and probes the referenced components/DAIs, creates runtime links, builds ALSA PCM devices for successful links, and connects the DAPM graph.

### Device Tree Binding And Phandles

- Custom machine drivers often use a platform `sound` node that references CPU and codec nodes by phandle.
- `ldd2-ch06` examples use custom properties such as:
  - `ssi-controller = <&ssi1>;`
  - `audio-codec = <&sgtl5000>;`
  - `model = "imx51-babbage-sgtl5000";`
  - `audio-routing = "MIC_IN", "Mic Jack", "Mic Jack", "Mic Bias", "Headphone Jack", "HP_OUT";`
- Property names are custom when writing a board-specific driver, but generic simple-card has standardized property names.
- `of_parse_phandle()` and related OF helpers extract the CPU DAI and codec nodes. `snd_soc_of_parse_card_name()` and `snd_soc_of_parse_audio_routing()` parse common sound-card properties.
- Book 1 and Notion Device Tree sources reinforce that labels such as `&ssi1`, `&sgtl5000`, or `&clks` compile into phandles and that named resources such as `clock-names`, `dma-names`, and `reg-names` prevent order-dependent bugs.

### Audio Routing

- Codec drivers define real codec pins with DAPM input/output widgets such as `MIC_IN`, `LINE_IN`, `HP_OUT`, and `LINE_OUT`.
- Machine drivers define board connectors with DAPM machine widgets such as `SND_SOC_DAPM_MIC("Mic Jack")`, `SND_SOC_DAPM_HP("Headphone Jack")`, `SND_SOC_DAPM_SPK("Speaker")`, and `SND_SOC_DAPM_LINE("Line In Jack")`.
- Machine routing connects codec pins to board connectors and may extend the codec power graph by connecting to supply widgets such as `SND_SOC_DAPM_SUPPLY` or `SND_SOC_DAPM_REGULATOR_SUPPLY`.
- DT routing is a list of string pairs, usually sink/source. Names must match widgets exported by the codec and machine/card.
- Static routing embeds `struct snd_soc_dapm_route` arrays in the machine driver and assigns them to `card->dapm_routes`; it is simpler in code but requires a kernel rebuild to change routing.
- Dynamic/DT routing lets board DTS choose pin-to-connector mapping when the machine driver calls the appropriate OF parsing helpers.

### Clocking And Formatting

- Machine-level PCM operations use `struct snd_soc_ops`. Important callbacks include `startup`, `shutdown`, `hw_params`, `hw_free`, `prepare`, and `trigger`.
- `startup()` runs when a PCM substream is opened; `hw_params()` runs when userspace commits stream parameters and can use sample rate, format, and channel count.
- Machine code must configure CPU DAI and codec DAI consistently:
  - `snd_soc_dai_set_fmt()` for format, clock provider/consumer relationship, and signal inversion.
  - `snd_soc_dai_set_pll()` for codec or CPU PLL setup.
  - `snd_soc_dai_set_sysclk()` for system/master clock selection and rate.
  - `snd_soc_dai_set_clkdiv()` for clock dividers.
  - `snd_soc_dai_set_tdm_slot()` and channel-map helpers for TDM links when needed.
- `SND_SOC_DAIFMT_I2S`, `SND_SOC_DAIFMT_LEFT_J`, `SND_SOC_DAIFMT_RIGHT_J`, `SND_SOC_DAIFMT_DSP_A/B`, `SND_SOC_DAIFMT_AC97`, and `SND_SOC_DAIFMT_PDM` define the protocol family.
- `SND_SOC_DAIFMT_NB_NF`, `NB_IF`, `IB_NF`, and `IB_IF` define bit-clock/frame inversion.
- Current headers prefer provider/consumer wording (`CBP_CFP`, `CBC_CFC`, etc.) while older docs often use master/slave aliases (`CBM_CFM`, `CBS_CFS`). The aliases remain visible locally but learner docs should explain current terminology.

### Simple-Card

- If the board only needs to connect a CPU DAI to a codec DAI with standard clocks/widgets/routes and no custom machine code, simple-card can describe the whole card in Device Tree.
- The source example uses:
  - `compatible = "simple-audio-card";`
  - `simple-audio-card,name`
  - `simple-audio-card,format`
  - `simple-audio-card,bitclock-master`
  - `simple-audio-card,frame-master`
  - `simple-audio-card,widgets`
  - `simple-audio-card,routing`
  - child `simple-audio-card,cpu` and `simple-audio-card,codec` nodes with `sound-dai`
- Simple-card should be presented as the default choice when no board-specific C callback, GPIO event, unusual clock programming, jack policy, or multi-link topology is needed.
- Custom machine drivers remain appropriate for non-trivial clock setup, amplifier/jack GPIO events, multiple links, DPCM, codec-less cards, board-specific controls, or vendor-specific quirks.

### Codec-Less Cards

- Some digital audio links have no normal external codec, such as S/PDIF-style preformatted digital audio.
- The source shows using dummy codec endpoints such as `snd-soc-dummy-dai` and `snd-soc-dummy`, plus `playback_only` / `capture_only` flags.
- Current code may use `COMP_DUMMY()` or `snd_soc_dummy_dlc` style helpers depending on kernel version.
- Learner docs should distinguish codec-less links from missing codec bugs: dummy endpoints are intentional only for hardware where no codec component should exist.

## Source Differences

- `ldd2-ch06` targets Linux v4.19-era code. Current Linux 6.8 headers keep the same conceptual objects but use endpoint component arrays and helper macros rather than the older direct `cpu_of_node` / `codec_of_node` / `platform_of_node` fields shown in the source snippets.
- Current local headers include both current provider/consumer DAI format names and older master/slave aliases. Final learner docs should use provider/consumer wording while recognizing older driver examples.
- The `ldd2-ch06` text appears to give the wrong prototype label near DT routing: it introduces `snd_soc_of_parse_audio_routing()` but shows `snd_soc_of_parse_card_name()` in the code block. Current local headers confirm both helpers exist with separate purposes.
- `ldd2-ch06` simple-card binding cites old `Documentation/devicetree/bindings/sound/simple-card.txt`; current kernels commonly use YAML bindings. This should be externally validated before final example work.
- Notion snippets show codec nodes and SAI/audio controller resources, but no full `sound` card node with `sound-dai`, `simple-audio-card`, or `audio-routing`; they are not duplicates of `ldd2-ch06`.
- Book 1 platform and DT chapters are generic. Their use here is limited to understanding platform-driver probe and phandle/resource mechanics.

## Gaps / Uncertainties

- Internal sources do not provide a current Linux 6.x custom machine-driver skeleton using `SND_SOC_DAILINK_DEFS()`, `SND_SOC_DAILINK_REG()`, component arrays, and devm card registration.
- Internal sources do not deeply cover audio-graph-card, simple-audio-card YAML schemas, multi-DAI/multi-codec links, DPCM front-end/back-end topology, codec-to-codec links, HDMI/DisplayPort audio, Sound Open Firmware, or topology files.
- Internal sources do not deeply cover `snd_soc_jack`, jack GPIOs, button reporting, external amplifier components, aux devices, or pin-switch parsing beyond basic board widgets.
- Internal sources do not give target hardware schematics. Real machine drivers require board-level validation of MCLK/BCLK/LRCLK direction, clock rates, codec reset/power timing, DAI slot width, TDM slots, DMA channels, regulator rails, pinmux, and connector wiring.
- The simple-card and custom machine-driver examples should be checked against current kernel docs and target headers before learner-facing code is written.
- Topic 36 must not re-teach codec internals at full depth. Detailed codec/component/DAI/DAPM implementation remains topic 35; topic 36 focuses on card binding and board routing.

## External Validation

External validation was partially performed with local Linux 6.8 headers because ASoC machine-driver APIs are version-sensitive.

| External Source | Purpose |
| --- | --- |
| `/lib/modules/6.8.0-124-generic/build/include/sound/soc.h` | Confirm current `struct snd_soc_dai_link_component`, `struct snd_soc_dai_link`, `struct snd_soc_card`, DAI link helper macros, card registration APIs, and OF parsing helpers. |
| `/lib/modules/6.8.0-124-generic/build/include/sound/soc-dai.h` | Confirm current DAI format/protocol/inversion/provider macros and the presence of old master/slave aliases. |
| `/lib/modules/6.8.0-124-generic/build/include/sound/soc-dapm.h` | Confirm current DAPM card/widget/route helper context used by machine-level board widgets and routes. |
| `/lib/modules/6.8.0-124-generic/build/include/sound/simple_card_utils.h` | Confirm current simple-card utility concepts and parser helpers present in local headers. |

External validation still needed before final learner-facing code/example:

- `https://docs.kernel.org/sound/soc/machine.html` or current official ASoC machine-driver documentation, if present, for modern machine-driver guidance.
- Current kernel Device Tree binding docs for `simple-audio-card`, `audio-graph-card`, and any chosen example codec/CPU DAI binding.
- Current in-tree examples under `sound/soc/` for a simple custom machine driver and a simple-card board DTS that match the target kernel version.

## Learning Content Brief

### What To Teach

- What an ASoC machine driver is: board glue that registers a sound card by binding CPU DAI, codec DAI, and platform/PCM components.
- Why codec/component drivers do not work alone and why board routing is intentionally separated from reusable codec and CPU DAI drivers.
- When to use simple-card versus a custom machine driver.
- How `struct snd_soc_dai_link`, `struct snd_soc_dai_link_component`, and `struct snd_soc_card` fit together.
- How a custom machine-driver platform `probe()` parses phandles, fills DAI link endpoints, assigns card device/name/routes/widgets, and calls `devm_snd_soc_register_card()`.
- How board connectors and codec pins form final DAPM routes.
- How `struct snd_soc_ops` configures DAI format, clocks, PLLs, dividers, and TDM slots in `startup()` / `hw_params()`.
- How simple-card expresses common card topology in Device Tree.
- How codec-less links use dummy endpoints and playback/capture-only flags.
- Practical debug flow: card enumeration, PCM nodes, controls, DAPM graph, clock signals, DT phandles, probe deferral, and routing mistakes.

### Beginner Mental Model

- Think of codec and CPU DAI drivers as reusable parts on a shelf.
- The machine driver is the board-specific wiring diagram that tells Linux which parts are connected and how the sound should flow.
- The registered `snd_soc_card` is what userspace finally sees as an ALSA sound card.

### Core Mechanism

1. CPU DAI/PCM driver registers the SoC audio interface and DMA/PCM support.
2. Codec/component driver registers codec DAIs, controls, pins, and DAPM graph.
3. Machine driver or simple-card describes the card:
   - card name
   - one or more DAI links
   - CPU endpoint(s)
   - codec endpoint(s)
   - platform/PCM endpoint(s)
   - board widgets and routes
   - DAI format and clock policy
4. `devm_snd_soc_register_card()` asks ASoC to instantiate the card.
5. ASoC resolves components, probes link runtimes, creates PCM devices, and connects DAPM.
6. Userspace opens PCM/control devices; machine and DAI callbacks configure the hardware for the stream.

### Important Structs / APIs

- `struct snd_soc_card`
- `struct snd_soc_dai_link`
- `struct snd_soc_dai_link_component`
- `struct snd_soc_ops`
- `struct snd_soc_pcm_runtime`
- `struct snd_soc_dai`
- `struct snd_soc_dapm_widget`
- `struct snd_soc_dapm_route`
- `devm_snd_soc_register_card()`
- `snd_soc_register_card()` / `snd_soc_unregister_card()`
- `SND_SOC_DAILINK_DEFS()`, `SND_SOC_DAILINK_DEF()`, `SND_SOC_DAILINK_REG()`
- `DAILINK_COMP_ARRAY()`, `COMP_CPU()`, `COMP_CODEC()`, `COMP_PLATFORM()`, `COMP_DUMMY()`
- `snd_soc_of_parse_card_name()`
- `snd_soc_of_parse_audio_routing()`
- `snd_soc_of_parse_audio_simple_widgets()`
- `snd_soc_of_parse_pin_switches()`
- `of_parse_phandle()` / `of_parse_phandle_with_args()`
- `snd_soc_dai_set_fmt()`
- `snd_soc_dai_set_sysclk()`
- `snd_soc_dai_set_pll()`
- `snd_soc_dai_set_clkdiv()`
- `snd_soc_dai_set_tdm_slot()`
- `SND_SOC_DAIFMT_*`
- `SND_SOC_DAPM_MIC()`, `SND_SOC_DAPM_HP()`, `SND_SOC_DAPM_SPK()`, `SND_SOC_DAPM_LINE()`
- `SOC_DAPM_PIN_SWITCH()`

### Common Bugs / Debugging Clues

- No sound card appears:
  - Machine/simple-card node did not match a driver.
  - `devm_snd_soc_register_card()` failed.
  - CPU or codec component did not probe yet and card registration deferred.
  - Wrong phandle in `sound-dai`, `audio-codec`, `ssi-controller`, or equivalent property.
- Card appears but no PCM:
  - DAI link endpoints do not resolve.
  - `codec_dai_name` or CPU DAI name does not match the registered DAI.
  - Missing platform/PCM component or DMA channel names.
- PCM exists but stream fails:
  - Unsupported rate/format/channel constraints across CPU DAI, codec DAI, and DMA.
  - Wrong `dai_fmt`, bit-clock/frame inversion, or provider/consumer clock role.
  - MCLK/sysclk/PLL not configured or at the wrong rate.
- Stream runs but silent audio:
  - Missing DAPM route from board connector to codec pin.
  - Wrong route names; DAPM routes are string-matched.
  - Mixer or pin switch disabled.
  - Amplifier GPIO/regulator not enabled or not modeled.
  - Pinmux or external clock missing.
- Pops/clicks or power issues:
  - Board power sequencing implemented outside DAPM.
  - Supply/clock widgets not connected to active paths.
  - Machine callback toggles GPIOs at the wrong stream event.

### Debug Commands / Checks To Mention Later

- `dmesg | grep -i -E 'asoc|alsa|snd|codec|dai|dapm|defer'`
- `aplay -l`
- `arecord -l`
- `cat /proc/asound/cards`
- `amixer -c <card>`
- `alsamixer -c <card>`
- `ls /sys/kernel/debug/asoc/` when debugfs is mounted and ASoC debug is enabled.
- Inspect DAPM widgets/routes under ASoC debugfs.
- Inspect `/sys/kernel/debug/clk/clk_summary` for MCLK/BCLK-related providers.
- Inspect regulator sysfs/debugfs for codec rails when available.
- Use oscilloscope or logic analyzer for MCLK, BCLK, LRCLK, and data lines on real hardware.
- Use Device Tree inspection: `dtc -I fs /proc/device-tree` or target-specific DT dump workflows.

### Production Concerns

- Prefer simple-card or audio-graph-card when the board can be described without custom C code.
- Use a custom machine driver when board policy needs real callbacks, GPIO sequencing, jack logic, multiple links, special clocks, or nonstandard routing.
- Keep codec details in codec drivers and board wiring in machine drivers.
- Use managed resources where practical, but understand that card registration can probe other components and create PCM devices.
- Treat `-EPROBE_DEFER` as normal when clocks, regulators, pinctrl, codec, or CPU DAI are not ready.
- Validate DAI format and clock-provider roles against the schematic, not only the codec datasheet.
- Keep DAPM widget and route names exact and stable.
- Do not hardcode routing in C if the same driver must support multiple board variants and DT routing is sufficient.
- Keep userspace ABI expectations in mind: ALSA card/control names may be consumed by UCM profiles, PipeWire/PulseAudio policy, scripts, and manufacturing tests.

### Interview Angles

- Explain why ASoC has codec, platform/CPU-DAI/PCM, and machine layers.
- Explain why a codec driver can probe successfully but no ALSA sound card appears.
- Compare simple-card and a custom machine driver.
- Walk through `snd_soc_dai_link` endpoint matching and current component-array style.
- Explain how DAPM routes connect codec pins to board connectors.
- Explain where to configure I2S format, clock master/provider, sysclk, PLL, and TDM slots.
- Debug a board where `aplay -l` shows no card.
- Debug a board where the card appears but playback is silent.
- Debug a board where sample rate changes fail.
- Explain why route strings and DAI names are fragile and how to verify them.

