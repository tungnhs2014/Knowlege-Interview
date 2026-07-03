# 36 - ASoC Machine Drivers And Audio Routing Interview Questions

Strong candidates should be able to reason from the board schematic to the ALSA card that userspace sees. The important skill is not memorizing struct fields; it is understanding how reusable codec/CPU DAI/PCM components are bound into a board-specific sound card, and how to debug failures along that path.

## Beginner Questions

### 1. What is an ASoC machine driver?

**Level:** Beginner

**Short Answer:**  
An ASoC machine driver is the board-specific glue that binds CPU DAI, codec DAI, and platform/PCM components into an ALSA sound card.

**Deep Explanation:**  
ASoC separates embedded audio into reusable pieces and board wiring. Codec drivers describe codec hardware; CPU DAI and PCM drivers describe the SoC audio path and DMA. The machine driver describes how those pieces are connected on one board.

It answers questions like:

- Which CPU DAI talks to which codec DAI?
- Which DAI format is used, such as I2S or TDM?
- Who provides bit clock and frame clock?
- Which codec pins are connected to speakers, microphones, or headphone jacks?
- Which board-specific callbacks or controls are needed?

The final result is a registered `struct snd_soc_card`.

**API / Code Anchor:**

- `struct snd_soc_card`
- `struct snd_soc_dai_link`
- `devm_snd_soc_register_card()`
- `struct platform_driver`

**Production or Debugging Angle:**  
If codec and CPU DAI drivers probe but no ALSA card appears, the machine-driver or simple-card layer is the first place to inspect.

**Common Traps:**

- Thinking a codec driver alone creates `/dev/snd/pcm*`.
- Putting board-specific routing in the codec driver.
- Treating the machine driver as reusable across unrelated boards.

**Follow-up Questions:**

- What is the difference between a codec driver and a machine driver?
- Why is board routing not usually placed in the codec driver?
- What does userspace see after a card is successfully registered?

### 2. Why does ASoC split codec, platform, and machine drivers?

**Level:** Beginner

**Short Answer:**  
The split lets codec and SoC audio drivers be reused while keeping board-specific wiring, routing, and clock policy in the machine layer.

**Deep Explanation:**  
The same codec can be connected to different SoCs. The same SoC I2S controller can be connected to different codecs. The wiring, connectors, clock roles, jacks, amplifiers, and board routes vary by product.

ASoC uses:

- Codec/component drivers for reusable codec behavior.
- CPU DAI/PCM drivers for reusable SoC audio interface and DMA behavior.
- Machine drivers or simple-card for board-specific integration.

This avoids duplicating codec logic in every board driver and avoids baking board wiring into reusable component drivers.

**API / Code Anchor:**

- Codec/component side: `devm_snd_soc_register_component()`
- Machine side: `struct snd_soc_card`, `struct snd_soc_dai_link`
- Card registration: `devm_snd_soc_register_card()`

**Production or Debugging Angle:**  
During bring-up, check each layer separately: component probe, DAI registration, card registration, DAPM routing, and clocks.

**Common Traps:**

- Assuming "platform" in ASoC always means generic Linux platform bus.
- Reusing a machine driver without checking board-specific clock and route assumptions.
- Debugging only the codec when the DAI link is wrong.

**Follow-up Questions:**

- What belongs in a codec driver?
- What belongs in the CPU DAI driver?
- What belongs in the machine driver?

### 3. What is `simple-audio-card`, and when should you use it?

**Level:** Beginner

**Short Answer:**  
`simple-audio-card` is a generic ASoC machine driver described in Device Tree. Use it when the board has a straightforward CPU DAI to codec DAI connection and does not need custom C callbacks.

**Deep Explanation:**  
Instead of writing a board-specific machine driver, Device Tree can describe the card name, format, CPU DAI, codec DAI, widgets, routes, and clock master/provider relationships.

It is a good fit when:

- One or a few normal links can be described declaratively.
- DAI format and routing are static.
- No special amplifier sequence, jack policy, PLL callback, or vendor quirk is needed.

Use a custom machine driver when the board needs code.

**API / Code Anchor:**

```dts
sound {
	compatible = "simple-audio-card";
	simple-audio-card,name = "Board Audio";
	simple-audio-card,format = "i2s";

	simple-audio-card,cpu {
		sound-dai = <&sai1>;
	};

	simple-audio-card,codec {
		sound-dai = <&codec>;
	};
};
```

**Production or Debugging Angle:**  
Prefer simple-card for simple boards because less custom C code means fewer lifetime and callback bugs. But verify the binding against the target kernel schema.

**Common Traps:**

- Writing a custom machine driver when simple-card is enough.
- Using simple-card when the board needs non-trivial custom clock or GPIO sequencing.
- Using old text-binding examples without checking current YAML binding requirements.

**Follow-up Questions:**

- What information must the DT node provide?
- When is a custom machine driver still required?
- How would you debug a simple-card node that does not create a card?

## Mid-Level Questions

### 4. Walk through a custom machine driver probe path.

**Level:** Mid-level

**Short Answer:**  
The probe parses the sound node, resolves CPU/codec phandles, fills DAI link endpoints, sets up the card fields, parses optional routing/name properties, and calls `devm_snd_soc_register_card()`.

**Deep Explanation:**  
A custom machine driver is usually a `platform_driver`. Its `probe()` is called when the board `sound` node matches the driver's OF table.

Typical steps:

1. Get `pdev->dev.of_node`.
2. Parse phandles for CPU DAI and codec node.
3. Allocate private card data.
4. Fill `struct snd_soc_dai_link_component` arrays for CPU, codec, and platform endpoints.
5. Fill `struct snd_soc_dai_link`.
6. Fill `struct snd_soc_card`.
7. Attach board widgets, routes, controls, and optional `struct snd_soc_ops`.
8. Parse card name and routing from DT if used.
9. Register the card.

If any endpoint is not ready, probe may defer.

**API / Code Anchor:**

- `of_parse_phandle()`
- `snd_soc_of_parse_card_name()`
- `snd_soc_of_parse_audio_routing()`
- `struct snd_soc_dai_link_component`
- `struct snd_soc_dai_link`
- `devm_snd_soc_register_card()`

**Production or Debugging Angle:**  
Use `dev_err_probe()` for dependencies that may defer. Poor error messages make ASoC bring-up painfully slow.

**Common Traps:**

- Forgetting `card->dev`.
- Using the wrong codec DAI name.
- Pointing platform/PCM endpoint at the wrong node.
- Leaking DT node references in non-devm/non-helper paths.
- Treating `-EPROBE_DEFER` as a hard failure.

**Follow-up Questions:**

- Why might CPU DAI and platform endpoint point to the same DT node?
- What should happen if `audio-routing` is absent?
- How would you handle multiple DAI links?

### 5. What does `struct snd_soc_dai_link` describe?

**Level:** Mid-level

**Short Answer:**  
It describes one audio link between CPU-side DAI(s), codec-side DAI(s), and platform/PCM component(s), plus link format, direction, and machine callbacks.

**Deep Explanation:**  
The DAI link is the object ASoC uses to know which runtime connection to instantiate. It is not just a name. It tells ASoC how to match the CPU DAI, codec DAI, and PCM provider.

Modern kernels commonly represent endpoints with component arrays:

- `cpus` / `num_cpus`
- `codecs` / `num_codecs`
- `platforms` / `num_platforms`

The link also carries:

- `name`
- `stream_name`
- `dai_fmt`
- `ops`
- `init`
- playback/capture-only flags
- advanced fields for DPCM or codec-to-codec cases

**API / Code Anchor:**

- `struct snd_soc_dai_link`
- `struct snd_soc_dai_link_component`
- `SND_SOC_DAILINK_DEFS()`
- `SND_SOC_DAILINK_REG()`
- `COMP_CPU()`, `COMP_CODEC()`, `COMP_PLATFORM()`

**Production or Debugging Angle:**  
If the card appears but no PCM is created, inspect the DAI links. A typo in the codec DAI name or wrong phandle can prevent runtime creation.

**Common Traps:**

- Using old examples with direct `cpu_of_node` / `codec_of_node` fields without checking target kernel headers.
- Confusing codec device name with codec DAI name.
- Omitting the platform endpoint when the CPU PCM provider still needs it.
- Forgetting playback-only/capture-only direction for unidirectional links.

**Follow-up Questions:**

- What is the difference between `.name` and `.dai_name` in a link component?
- What creates the PCM runtime?
- Why might one card have multiple DAI links?

### 6. How do machine DAPM routes work?

**Level:** Mid-level

**Short Answer:**  
Machine DAPM routes connect board-level widgets, such as headphone jacks and microphones, to codec pins and internal codec routes so DAPM can power the correct path.

**Deep Explanation:**  
Codec drivers define real codec pins and internal paths. Machine drivers define board connectors and describe how they connect to those codec pins.

Example:

```c
static const struct snd_soc_dapm_widget widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_MIC("Mic Jack", NULL),
};

static const struct snd_soc_dapm_route routes[] = {
	{ "Headphone Jack", NULL, "HP_OUT" },
	{ "MIC_IN", NULL, "Mic Jack" },
};
```

The strings must match widget names. DAPM uses the graph to decide what can be powered on or off.

**API / Code Anchor:**

- `struct snd_soc_dapm_widget`
- `struct snd_soc_dapm_route`
- `SND_SOC_DAPM_HP()`
- `SND_SOC_DAPM_MIC()`
- `SND_SOC_DAPM_SPK()`
- `SND_SOC_DAPM_LINE()`
- `snd_soc_of_parse_audio_routing()`

**Production or Debugging Angle:**  
Silent playback with a working PCM often means the DAPM graph does not connect the active stream to the physical output.

**Common Traps:**

- Route strings do not match codec pin names exactly.
- Source and sink order is wrong.
- Board connector widgets are missing.
- Amplifier or supply widget is not connected to the active path.
- Assuming normal mixer controls and DAPM controls are the same thing.

**Follow-up Questions:**

- How would you inspect DAPM routes at runtime?
- Why might `amixer` show a control but DAPM still powers down the path?
- When should routing be static in C versus parsed from DT?

### 7. Where should DAI clock and format setup happen?

**Level:** Mid-level

**Short Answer:**  
Machine-level clock and format policy is usually applied through `struct snd_soc_ops`, often in `startup()` or `hw_params()`, using DAI helper APIs.

**Deep Explanation:**  
The machine layer knows board-level policy: which side provides bit clock/frame clock, what MCLK rate is wired, and how the codec PLL should be programmed for a stream rate.

`hw_params()` is common because it receives the selected sample rate, format, and channel count. The driver can compute sysclk or PLL rates from `params_rate(params)`.

Typical helpers:

- `snd_soc_dai_set_fmt()`
- `snd_soc_dai_set_sysclk()`
- `snd_soc_dai_set_pll()`
- `snd_soc_dai_set_clkdiv()`
- `snd_soc_dai_set_tdm_slot()`

**API / Code Anchor:**

```c
static int board_hw_params(struct snd_pcm_substream *substream,
			   struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = snd_soc_substream_to_rtd(substream);
	struct snd_soc_dai *cpu_dai = snd_soc_rtd_to_cpu(rtd, 0);
	struct snd_soc_dai *codec_dai = snd_soc_rtd_to_codec(rtd, 0);
	unsigned int fmt = SND_SOC_DAIFMT_I2S |
			   SND_SOC_DAIFMT_NB_NF |
			   SND_SOC_DAIFMT_CBC_CFC;

	return snd_soc_dai_set_fmt(cpu_dai, fmt) ?:
	       snd_soc_dai_set_fmt(codec_dai, fmt);
}
```

**Production or Debugging Angle:**  
If audio plays at the wrong speed, fails only for certain sample rates, or has bad framing, inspect DAI format and clock setup first.

**Common Traps:**

- Setting CPU and codec DAIs inconsistently.
- Using master/slave assumptions from old code without checking provider/consumer meaning.
- Forgetting MCLK/sysclk requirements for 44.1 kHz versus 48 kHz families.
- Hardcoding one rate when userspace can request many rates.

**Follow-up Questions:**

- Why is `hw_params()` often better than `startup()` for PLL setup?
- How do you verify BCLK and LRCLK on hardware?
- What is the difference between protocol format and clock inversion?

## Senior Questions

### 8. Debug this: codec and CPU DAI probe, but `aplay -l` shows no card.

**Level:** Senior

**Short Answer:**  
Focus on card registration and endpoint resolution: machine/simple-card driver matching, phandles, DAI names, missing dependencies, and `devm_snd_soc_register_card()` errors.

**Deep Explanation:**  
Successful component probe only means the reusable pieces are available. A card appears only after the machine driver registers `struct snd_soc_card` and ASoC resolves every required DAI link endpoint.

Debug path:

1. Confirm machine driver or simple-card driver is built and loaded.
2. Confirm the `sound` node `compatible` matches.
3. Inspect `dmesg` for ASoC errors and `-EPROBE_DEFER`.
4. Verify `sound-dai`, `audio-codec`, `cpu-dai`, or board-specific phandles.
5. Verify codec DAI name and CPU DAI binding.
6. Check clocks/regulators/pinctrl dependencies.
7. Confirm `devm_snd_soc_register_card()` is called and check its return value.

**API / Code Anchor:**

- `devm_snd_soc_register_card()`
- `of_parse_phandle()`
- `snd_soc_of_parse_card_name()`
- `struct of_device_id`
- `MODULE_DEVICE_TABLE(of, ...)`
- `struct snd_soc_dai_link_component`

**Production or Debugging Angle:**  
Good probe logs should name the missing endpoint or dependency. `dev_err_probe()` is helpful because it handles deferred-probe noise.

**Common Traps:**

- Debugging codec registers before proving the card exists.
- Missing `CONFIG_SND_SIMPLE_CARD` or custom machine driver config.
- Wrong DT overlay loaded.
- DAI name typo hidden behind a successful codec bus probe.
- Treating deferred probe as fatal.

**Follow-up Questions:**

- What log lines would you expect when card registration defers?
- How do you prove the machine driver's `probe()` ran?
- What would you inspect in `/proc/asound/cards` and ASoC debugfs?

### 9. Debug this: card and PCM exist, `aplay` succeeds, but no sound comes out.

**Level:** Senior

**Short Answer:**  
The likely areas are DAPM routing, mixer/pin switch state, amplifier/supply enable, pinmux, or physical clocks/data. Start from DAPM and userspace controls, then verify hardware signals.

**Deep Explanation:**  
If `aplay` succeeds, the ALSA PCM path can accept data. Silence means the samples may not be reaching the analog output or the output is not powered/enabled.

Check:

- `amixer` and `alsamixer` for muted controls and pin switches.
- ASoC debugfs DAPM widgets/routes.
- Board route strings and codec pin names.
- Amplifier GPIO/regulator handling.
- Pinmux state for audio pins.
- MCLK, BCLK, LRCLK, and data on a scope or logic analyzer.

The software path and hardware path must agree.

**API / Code Anchor:**

- `struct snd_soc_dapm_route`
- `SND_SOC_DAPM_HP()`, `SND_SOC_DAPM_SPK()`, `SND_SOC_DAPM_MIC()`
- `SOC_DAPM_PIN_SWITCH()`
- `snd_soc_of_parse_audio_routing()`
- `snd_soc_dai_set_fmt()`

**Production or Debugging Angle:**  
This is a classic bring-up case where logs may look clean. The fastest path is to inspect DAPM state and measure clocks/data.

**Common Traps:**

- Assuming `aplay` success proves analog output works.
- Ignoring muted ALSA controls.
- Misspelling route strings.
- Forgetting external amplifier enable GPIO.
- Debugging only software when BCLK/LRCLK are absent.

**Follow-up Questions:**

- What does DAPM debugfs tell you?
- How would you check if the codec output pin is powered?
- What would you measure on the I2S bus?

### 10. How do you choose between static routing in C and DT `audio-routing`?

**Level:** Senior

**Short Answer:**  
Use DT routing when board variants need different connector-to-codec pin maps. Use static C routing when routing is fixed by the machine driver and not expected to vary by board.

**Deep Explanation:**  
Static routing is simple and close to the driver, but changing it requires changing code and rebuilding. DT routing lets board files describe actual wiring without modifying the driver.

For a reusable machine driver across multiple boards, DT routing is usually better. For a one-off board with fixed wiring or complex callbacks tied to the route, static C routing can be acceptable.

The route names still must match DAPM widgets exactly in both cases.

**API / Code Anchor:**

- Static:

```c
card->dapm_routes = board_routes;
card->num_dapm_routes = ARRAY_SIZE(board_routes);
```

- Device Tree:

```c
ret = snd_soc_of_parse_audio_routing(card, "audio-routing");
```

**Production or Debugging Angle:**  
DT routing is easier to review against a schematic and easier to adjust across board revisions, but it must be covered by binding documentation and validation.

**Common Traps:**

- Hardcoding one board's route into a driver used by multiple boards.
- Allowing arbitrary DT route names that do not match widgets.
- Forgetting board widgets when parsing DT routes.
- Believing DT routing can fix missing codec pins that the codec driver never defined.

**Follow-up Questions:**

- What must exist before a route can be parsed successfully?
- How would you design one machine driver for two board variants?
- What should be documented in the binding?

### 11. What are the version-sensitive traps in ASoC machine-driver examples?

**Level:** Senior

**Short Answer:**  
Older examples often use direct DAI-link fields and master/slave clock wording. Modern kernels commonly use component arrays and provider/consumer terminology.

**Deep Explanation:**  
The ASoC machine-driver model is stable, but structure layouts and preferred helper style evolve. A driver copied from an older source may not compile or may use outdated terminology.

Common drift:

- Old examples: `cpu_of_node`, `codec_of_node`, `platform_of_node`.
- Current style: `cpus`, `codecs`, `platforms` arrays of `struct snd_soc_dai_link_component`.
- Old clock wording: codec/CPU master or slave.
- Current wording: clock/frame provider or consumer.
- Old simple-card docs: text binding files.
- Current kernels: YAML binding schemas.

**API / Code Anchor:**

- `struct snd_soc_dai_link_component`
- `SND_SOC_DAILINK_DEFS()`
- `SND_SOC_DAILINK_REG()`
- `COMP_CPU()`, `COMP_CODEC()`, `COMP_PLATFORM()`
- `SND_SOC_DAIFMT_CBP_CFP`, `SND_SOC_DAIFMT_CBC_CFC`
- compatibility aliases such as `SND_SOC_DAIFMT_CBM_CFM`

**Production or Debugging Angle:**  
Always write against target kernel headers and nearby in-tree examples. Audio code is especially easy to get "almost right" from old snippets.

**Common Traps:**

- Treating old book code as current skeleton code.
- Mixing direct fields and component-array style.
- Misreading clock provider role after terminology changes.
- Not validating Device Tree binding with current schemas.

**Follow-up Questions:**

- How would you modernize an old DAI-link example?
- How do you confirm which DAI format macros exist in your kernel?
- Why is provider/consumer wording clearer than master/slave?

### 12. How would you design a board with an external amplifier and jack detection?

**Level:** Senior

**Short Answer:**  
Keep reusable codec behavior in the codec driver, put board wiring and policy in the machine layer, and model power/events through DAPM, jack helpers, GPIO descriptors, regulators, and aux components where appropriate.

**Deep Explanation:**  
An external amplifier and headset jack are board-level facts. The codec may expose pins and controls, but the board determines which GPIO enables the amp, which connector is attached, and what jack behavior userspace should see.

Possible design choices:

- Use DAPM speaker/headphone widgets and routes.
- Use DAPM event callbacks or an amplifier component for power sequencing.
- Use `snd_soc_jack` helpers for jack detection when needed.
- Use GPIO descriptors for detect/enable lines.
- Use regulators and clocks through normal frameworks.
- Use simple-card only if the binding can express the required behavior cleanly.
- Use custom machine driver if callbacks or policy are required.

**API / Code Anchor:**

- `SND_SOC_DAPM_SPK()`
- `SND_SOC_DAPM_HP()`
- `struct snd_soc_dapm_route`
- `SOC_DAPM_PIN_SWITCH()`
- `struct snd_soc_card`
- `struct snd_soc_ops`
- `snd_soc_card_jack_new()` family, depending on kernel version
- GPIO/regulator consumer APIs as supporting frameworks

**Production or Debugging Angle:**  
Power sequencing mistakes can cause pops, clicks, battery drain, or speaker damage. This is not just a "make sound" problem.

**Common Traps:**

- Toggling amp GPIO in random PCM callbacks instead of modeling power path.
- Encoding board jack policy in a reusable codec driver.
- Not considering suspend/resume and stream-stop ordering.
- Ignoring userspace control names and UCM policy.

**Follow-up Questions:**

- When would you use an aux component?
- How would you avoid pop/click during power-up?
- How would you test jack insertion and removal?

### 13. Debug this: 48 kHz works, but 44.1 kHz fails or plays at the wrong speed.

**Level:** Senior

**Short Answer:**  
Check clock tree and `hw_params()` logic. The machine driver may only configure PLL/sysclk correctly for the 48 kHz family, or MCLK cannot generate the 44.1 kHz family.

**Deep Explanation:**  
Audio sample rates depend on exact clock relationships. A board might use one oscillator that naturally supports 48 kHz multiples but not 44.1 kHz multiples, or the machine driver may hardcode `rate * 256` without checking what the codec PLL can generate.

Debug path:

1. Inspect `hw_params()` and DAI clock setup.
2. Check requested `params_rate(params)`.
3. Verify codec and CPU DAI supported rates.
4. Inspect clock parents and rates.
5. Measure MCLK/BCLK/LRCLK.
6. Check whether both CPU and codec use the same provider/consumer relationship.

**API / Code Anchor:**

- `snd_soc_dai_set_pll()`
- `snd_soc_dai_set_sysclk()`
- `snd_soc_dai_set_clkdiv()`
- `snd_soc_dai_set_fmt()`
- `params_rate(params)`
- `/sys/kernel/debug/clk/clk_summary`

**Production or Debugging Angle:**  
Audio clock bugs often pass basic probe tests and fail only during real playback at a specific rate.

**Common Traps:**

- Assuming all codecs support all sample rates with one MCLK.
- Hardcoding one sysclk rate.
- Not checking the clock provider role.
- Looking only at ALSA constraints and not the physical clock tree.

**Follow-up Questions:**

- How do 44.1 kHz and 48 kHz clock families differ?
- What should the driver return if it cannot support the requested rate?
- How would you expose only supported rates?

### 14. How do codec-less cards differ from broken codec links?

**Level:** Senior

**Short Answer:**  
A codec-less card intentionally uses a dummy endpoint for hardware that has no normal codec, such as some S/PDIF paths. A broken codec link is an accidental failure to bind the real codec.

**Deep Explanation:**  
Some digital audio interfaces carry already formatted data and do not need a traditional analog codec component. In that case, the machine driver can use dummy codec endpoints and set playback-only or capture-only direction as appropriate.

This should be a deliberate design choice. If a board has a real codec but the machine driver falls back to dummy because the codec did not bind, that hides the real bug.

**API / Code Anchor:**

- `playback_only`
- `capture_only`
- dummy DAI/component endpoint helpers such as `COMP_DUMMY()` depending on kernel version
- `struct snd_soc_dai_link_component`

**Production or Debugging Angle:**  
Codec-less links are valid for some digital paths, but they should be documented in the board binding and driver comments because they can confuse future debugging.

**Common Traps:**

- Using dummy codec endpoints to avoid fixing a codec probe problem.
- Setting both playback-only and capture-only incorrectly.
- Forgetting that S/PDIF or digital links still need correct clocks and formats.
- Assuming codec-less means no DAI constraints.

**Follow-up Questions:**

- What hardware examples might use codec-less links?
- How would userspace see a codec-less PCM?
- What must still be configured even without a codec?

