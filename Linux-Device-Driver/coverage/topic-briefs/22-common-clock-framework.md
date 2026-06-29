# Topic Brief - 22 - Common Clock Framework

## Output Targets
- Knowledge: `knowledge/22-common-clock-framework.md`
- Interview: `interview/22-common-clock-framework.md`
- Example: `examples/22-common-clock-framework/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-source-root` | `docs/Linux Device Driver Development/` | searched/mapped/gap | No dedicated Common Clock Framework chapter found in book 1. Relevant hits are Device Tree clock references and applied consumer examples. |
| `ldd1-ch06` | `docs/Linux Device Driver Development/Chapter 6-The Concept of a Device Tree .md` | read/mapped/covered/merged | Book 1 DT resource source: `clocks`, `clock-names`, named resource motivation, and `devm_clk_get()` consumer lookup. Contains a typo/bug in the second clock variable/name that must be corrected later. |
| `ldd1-ch19` | `docs/Linux Device Driver Development/Chapter 19-PWM Drivers.md` | read/mapped/related | Applied PWM controller DT node with `clocks` and `clock-names`; useful for showing that subsystem providers often consume bus/peripheral clocks. Full PWM behavior remains topic 27. |
| `ldd1-ch21` | `docs/Linux Device Driver Development/Chapter 21-Framebuffer Drivers.md` | read/mapped/related | Applied display lifecycle note: probe sets clocks, blanking should stop clocks and power down, unblanking reverses it. Full framebuffer behavior remains topic 29. |
| `ldd2-ch04` | `docs/Linux Device Driver Development 2/Chapter 4-Common_Clock_Framework.md` | read/mapped/covered/merged | Primary CCF source: purpose, provider/consumer split, `struct clk_hw`, `struct clk`, `struct clk_core`, `struct clk_ops`, `struct clk_init_data`, provider registration, DT provider lookup, clock types, flags, consumer APIs, rate/parent APIs, and debugfs `clk_summary`. |
| `ldd2-ch05` | `docs/Linux Device Driver Development 2/Chapter 5-ASoC_Framework.md` | read/mapped/related | ASoC depends on CCF; DAI callbacks configure sysclk, PLL, dividers, and DAPM has a clock-supply widget tied to the clock framework. Full ASoC remains topics 35-36. |
| `ldd2-ch06` | `docs/Linux Device Driver Development 2/Chapter 6-ASoC_Machine_Drivers.md` | read/mapped/related | Applied audio clocking: DT CPU/codec clocks, `snd_soc_dai_set_sysclk()`, PLL, divider, master/slave clock settings. Useful for interview/application examples only. |
| `ldd2-ch10` | `docs/Linux Device Driver Development 2/Chapter 10-Linux_Kernel_Power_Management.md` | read/mapped/related | DVFS/OPP and power-domain notes show clock rates as part of frequency/power management. Detailed runtime/system PM remains topic 24. |
| `ldd2-ch12` | `docs/Linux Device Driver Development 2/Chapter 12-NVMEM_Framework.md` | read/mapped/related | NVMEM producer/consumer analogy to CCF and an applied DT clock reference in a tempmon node. Full NVMEM remains topic 28. |
| `ldd2-ch13` | `docs/Linux Device Driver Development 2/Chapter 13-Watchdog_Device_Drivers.md` | read/mapped/related | Watchdog private data includes `struct clk *clk`, reinforcing ordinary framework drivers as clock consumers. Full watchdog remains topic 28. |
| `notion-index` | `docs/Linux-Device-Driver-Notion/Linux Device Drivers.md` | inspected/mapped/index-only | Notion index inspected; no standalone Notion CCF chapter found. Relevant material is distributed across DT/platform/media notes. |
| `notion-ch01-part1` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 1 Environment Setup and Getting Source Code.md` | read/mapped/related | Source-tree orientation notes `drivers/clk/` as the clock framework location. |
| `notion-ch05-part1` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 1 Platform Bus & Driver Basics.md` | read/mapped/related | Probe/remove mental model includes enabling clocks during hardware init and disabling clocks on remove. |
| `notion-ch05-part2` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 2 Probe, Remove & Resource Management.md` | read/mapped/related | Resource-management table maps `clk_get()` to `devm_clk_get()` and remove notes include disabling clocks. |
| `notion-ch06-part1` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 1 Device Tree Fundamentals.md` | read/mapped/covered/merged | Beginner DT examples for clock controller nodes, `#clock-cells`, phandle references, `clocks`, `clock-names`, and SoC-level clock declarations. |
| `notion-ch06-part2` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 2 Device Addressing and Resources.md` | read/mapped/covered/merged | Strongest Notion clock-reference section: clock provider/consumer model, `#clock-cells`, header-defined IDs, `devm_clk_get()`, `clk_prepare_enable()`, error unwind, `clk_get_rate()`, and multiple providers. |
| `notion-ch06-part3` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 3 OF APIs and Platform Integration.md` | read/mapped/covered/merged | OF phandle parsing with `of_parse_phandle_with_args()`, `struct of_phandle_args`, manual clock-specifier parsing, and a platform-driver snippet using `devm_clk_get()`, `clk_prepare_enable()`, `clk_get_rate()`, and `clk_disable_unprepare()`. |
| `notion-ch07-part3` | `docs/Linux-Device-Driver-Notion/Chapter 7-Part 3 Device Tree Integration.md` | read/mapped/related | I2C DT examples include controller clocks and an external clock for a child device; full I2C remains topic 16. |
| `notion-ch08-part3` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 3 Device Tree Integration and Userspace Inter.md` | read/mapped/related | SPI DT example with controller `clocks` and a `fixed-clock` CAN oscillator; full SPI and `spidev` remain topic 17. |
| `notion-ch07-extra-v4l2-part2` | `docs/Linux-Device-Driver-Notion/Chapter 7-v4l2_part2.md` | read/mapped/related | Applied camera-sensor power sequencing with `devm_clk_get("xclk")`, `clk_prepare_enable()`, and `clk_disable_unprepare()`. Detailed V4L2 remains topics 32-34. |
| `notion-ch08-extra-part1` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 1.md` | read/mapped/related | Same raw Notion chapter number as SPI material but V4L2/fwnode content; read separately and mapped only for media clock/fwnode references. |
| `notion-ch08-extra-part2` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 2.md` | read/mapped/related | V4L2 media-controller material with camera clocks (`bus`, `mod`, `ram`, `xvclk`); not merged as SPI duplicate. |

## Source Files Read
- `ldd2-ch04`: complete chapter, including CCF motivation, data structures, provider registration/unregistration, `of_clk_add_hw_provider()`, clock provider DT bindings, clock decoding callbacks, provider-driver design, fixed-rate/fixed-factor/gate/mux/divider/composite clocks, `clk_summary`, and consumer APIs.
- `ldd1-ch06`: "Handling resources" and "Concept of named resources" sections covering `clocks`, `clock-names`, and `devm_clk_get()`.
- `ldd1-ch19`: PWM controller DT binding excerpt containing clock resources.
- `ldd1-ch21`: framebuffer probe and blank/unblank lifecycle excerpts that mention clock setup/stop.
- `ldd2-ch05`: ASoC technical requirements, `snd_soc_dai_ops` clock callbacks, and `SND_SOC_DAPM_CLOCK_SUPPLY`.
- `ldd2-ch06`: SSI/codec DT clock examples and "Clocking and formatting considerations".
- `ldd2-ch10`: CPUfreq/DVFS and power-domain sections where frequency and clocks intersect power management.
- `ldd2-ch12`: producer/consumer analogy and tempmon DT clock reference.
- `ldd2-ch13`: watchdog private-data/probe area where a watchdog consumes a clock.
- `notion-index`: inspected to confirm no standalone Notion CCF chapter.
- `notion-ch01-part1`: source-tree map entry for `drivers/clk/`.
- `notion-ch05-part1` and `notion-ch05-part2`: probe/remove resource lifecycle and managed clock acquisition notes.
- `notion-ch06-part1`: clock-controller phandle examples and multi-clock consumer examples.
- `notion-ch06-part2`: full `6.5 Clock References` section plus named-clock resource examples.
- `notion-ch06-part3`: string-array handling for `clock-names`, phandle parsing for clocks, and the complete platform-driver example using `struct clk`.
- `notion-ch07-part3`, `notion-ch08-part3`, `notion-ch07-extra-v4l2-part2`, `notion-ch08-extra-part1`, and `notion-ch08-extra-part2`: applied I2C/SPI/media clock snippets, read separately because raw chapter numbers and names overlap.

### Inventory Decisions
- `ldd2-ch04` is the only dedicated CCF source. It is primary for provider internals and consumer APIs.
- `ldd1` has no dedicated CCF source. `ldd1-ch06` is kept as a covered adjacent source because it explains the DT/named-resource side that CCF consumers use. `ldd1-ch19` and `ldd1-ch21` are related applied examples only.
- Notion has no standalone CCF chapter. Its strongest coverage is `notion-ch06-part2` and `notion-ch06-part3`; both were read independently from `ldd1-ch06` because they add clearer beginner explanations and driver snippets.
- Notion files with apparent Chapter 8 collisions were read separately. `Chapter 8-Part 3 Device Tree Integration and Userspace Inter.md` is SPI material; `Chapter 8-Part 1.md` and `Chapter 8-Part 2.md` are V4L2/media material. They are not duplicates.
- ASoC, DVFS, framebuffer, PWM, NVMEM, watchdog, I2C, SPI, and V4L2 references are retained only as applications of clock consumption or clock-driven subsystem behavior. Their subsystem APIs remain in their own learning-path topics.
- Search hits for "clock" meaning RTC, timer clockids, SPI SCK, I2C SCL, pixel-clock fields, or audio bit-clock protocol were screened and not promoted unless they touched CCF APIs, DT clock resources, or clock lifecycle.

## Merged Source Notes
- CCF exists because old SoC-specific clock APIs duplicated the same ideas across platforms. The framework gives consumers a hardware-independent `clk_*()` API while providers implement hardware-specific `clk_ops`.
- Keep the two faces separate:
  - Provider side: `struct clk_hw`, `struct clk_ops`, `struct clk_init_data`, type helpers, and provider registration.
  - Consumer side: opaque `struct clk *`, `devm_clk_get()`, prepare/enable, rate, parent, optional/bulk helpers, and cleanup.
- `struct clk_core` is internal framework state. Drivers should not depend on its layout.
- Provider drivers embed `struct clk_hw` in hardware-specific structs, recover private data with `container_of()`, populate init data, register clocks with `clk_hw_register()` or helper APIs, then expose them to firmware consumers with `of_clk_add_hw_provider()` or managed variants.
- Device Tree connects provider and consumer:
  - Provider node: `#clock-cells` describes the specifier length; `clock-output-names` may name provided outputs.
  - Consumer node: `clocks` references provider phandles/specifiers; `clock-names` names the consumer's input lines.
  - Consumers should request their local input names, not provider output names.
- CCF lookup through DT parses the consumer's `clocks` list with `of_parse_phandle_with_args()`, matches the provider node in the registered provider list, invokes the provider's decode callback, and returns a consumer `struct clk *`.
- Generic provider decode helpers:
  - `of_clk_hw_simple_get()` for one exported `struct clk_hw`.
  - `of_clk_hw_onecell_get()` with `struct clk_hw_onecell_data` for indexed multi-output providers.
- Base clock types:
  - Fixed-rate: constant rate, no gate, often pure DT `fixed-clock`.
  - Fixed-factor: multiply/divide a parent by constants.
  - Gate: enable/disable a parent-derived signal.
  - Mux: choose one parent from several.
  - Divider: divide parent rate by register value/table.
  - Composite: combine mux, rate/divider, and gate behavior into one logical clock.
- The provider's callback choice defines capabilities. For example, a mux needs parent callbacks; a divider/rate clock needs rate callbacks; a gate needs enable/disable or prepare/unprepare depending on bus context.
- Prepare versus enable is a central rule:
  - `prepare`/`unprepare` may sleep.
  - `enable`/`disable` must not sleep.
  - Consumers in process context usually use `clk_prepare_enable()` and `clk_disable_unprepare()`.
  - I2C/SPI-backed clock chips should put sleepable register access in provider `prepare`/`unprepare`, not provider `enable`/`disable`.
- Common consumer flow:
  - Get clock handles during probe, normally by name.
  - Set or read rates if the hardware contract requires it.
  - Prepare/enable before register access, transfer, streaming, or peripheral activation.
  - Disable/unprepare on error unwind, remove, runtime suspend, stream stop, or blanking.
- Rate changes are not free-form. Drivers should ask what the clock can provide, handle fixed-rate or shared-parent refusal, and understand `CLK_SET_RATE_PARENT` if a child should propagate rate changes upstream.
- Parent changes use `clk_set_parent()`/`clk_get_parent()` on the consumer side and `.set_parent`/`.get_parent` provider ops for mux-like hardware.
- Debugging starts with `/sys/kernel/debug/clk/clk_summary` after mounting debugfs. It reveals topology, enable/prepare counts, rates, and suspicious orphan/disabled clocks.
- `CLK_IGNORE_UNUSED` and the `clk_ignore_unused` boot argument are bring-up/debug tools. They can hide missing clock ownership in drivers and should not be treated as production fixes.

## Source Differences
- `ldd2-ch04` targets Linux v4.19. Current kernels add or emphasize more APIs than the chapter covers:
  - Consumer bulk APIs: `clk_bulk_*()` and `struct clk_bulk_data`.
  - Optional/managed helpers: `devm_clk_get_optional()`, `devm_clk_get_enabled()`, `devm_clk_get_prepared()`, optional enabled/prepared variants, and bulk managed enabled variants.
  - Rate control helpers: `clk_rate_exclusive_get()`, `clk_set_rate_exclusive()`, `clk_set_rate_range()`.
  - Phase/duty-cycle APIs: `clk_set_phase()` and `clk_set_duty_cycle()`.
  - Notifiers: `clk_notifier_register()` and managed variants.
  - Provider parent description drift: `struct clk_parent_data` and parent-hw/parent-data registration helper variants reduce reliance on raw parent-name strings.
- `ldd2-ch04` says the consumer interface "entirely relies on the device tree." Current code still supports clkdev lookup, ACPI/fwnode/platform data, and firmware-independent provider paths. Teach DT as common for embedded SoCs, not the only mechanism.
- `ldd2-ch04` uses old text binding paths such as `fixed-factor-clock.txt`, `pwm-clock.txt`, `gpio-gateclock.txt`, and `clock-bindings.txt`. Current validation shows common clock binding text moved to DT schema, and fixed-clock is YAML (`fixed-clock.yaml`).
- `ldd2-ch04` shows `clk_register_*()` examples in several places. Teach them as legacy/older-code recognition and prefer `clk_hw_*()` plus managed helpers for new provider code.
- Some helper names in the source are stale or extracted oddly, for example "struct clk *C(...)" where `devm_clk_get()` is intended, and `clk_register_mux_table()` rather than modern `clk_hw_register_mux_table()`/parent-data variants. Later learner docs should correct these without hiding the source difference.
- `ldd1-ch06` has extraction/typo issues: `structclk`, `clck_per`, and the second clock lookup stores into `clk_ipg` with `"pre"` even though the DT name is `"per"`. Teach the intended pattern as `clk_ipg = devm_clk_get(..., "ipg")` and `clk_per = devm_clk_get(..., "per")`.
- Notion `notion-ch06-part2` gives a good beginner `clk_prepare_enable()` sequence but omits a remove path in the same snippet. The learner-facing chapter should add remove/error-path balancing.
- Notion `notion-ch06-part3` manually parses clock phandles for demonstration. Normal consumers should prefer CCF APIs (`devm_clk_get*`) rather than open-coding `of_parse_phandle_with_args()` unless writing provider/binding/debug code.
- ASoC sources use subsystem clock APIs (`snd_soc_dai_set_sysclk()`, `set_pll`, `set_clkdiv`) that may internally relate to CCF but are not replacements for generic CCF provider/consumer APIs.
- Framebuffer and media sources show power sequencing but do not explain runtime PM ordering. Deeper suspend/resume/runtime PM belongs to topic 24.

## Gaps / Uncertainties
- Internal sources do not cover modern reset/clock/power-domain combined sequencing in depth. Topic 24 should handle runtime PM and system suspend/resume ordering.
- Internal sources do not cover assigned-clock DT properties (`assigned-clocks`, `assigned-clock-parents`, `assigned-clock-rates`) in enough detail. Needs current binding validation before learner docs.
- Internal sources do not deeply explain clock rate notifiers, exclusive rate ownership, phase/duty-cycle APIs, or clk bulk APIs. External validation identified them; final docs should include them at least as version notes.
- Provider locking is only lightly covered. Final docs should explain when provider callbacks need spinlocks around shared MMIO, and why sleepable bus transactions cannot happen in `.enable`/`.disable`.
- The sources do not provide a production-grade CCF provider example that can be built without real hardware. A later example should probably use DTS/debug workflow or pseudo-code, not a fake provider module that might teach bad binding habits.
- Debugging coverage is mostly `clk_summary`. Later material should add common log/error symptoms: `-EPROBE_DEFER`, missing provider, wrong `clock-names`, zero rate, unused clocks disabled late, and bootloader-left-on dependency.
- Exact current APIs must be checked against the target kernel headers before final C examples. The source docs were written against v4.19-era interfaces.

## External Validation
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/Documentation/driver-api/clk.rst`
  - Validated current high-level CCF split, `CONFIG_COMMON_CLK`, `struct clk_ops`, provider `struct clk_hw` pattern, callback capability matrix, and `clk_ignore_unused` debug behavior.
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/clk-provider.h`
  - Validated current provider-side API drift: `struct clk_parent_data`, current `struct clk_init_data`, provider `clk_hw_*` helpers, managed helpers, parent-data/parent-hw variants, `CLK_OPS_PARENT_ENABLE`, and current `of_clk_add_hw_provider()`/`devm_of_clk_add_hw_provider()`.
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/clk.h`
  - Validated current consumer-side API drift: bulk APIs, optional clocks, managed enabled/prepared helpers, rate-exclusive helpers, rate range, phase/duty-cycle APIs, and clock notifiers.
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/Documentation/devicetree/bindings/clock/clock-bindings.txt`
  - Validated that the old common clock text binding has moved to DT schema.
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/Documentation/devicetree/bindings/clock/fixed-clock.yaml`
  - Validated current fixed-clock YAML requirements: `compatible = "fixed-clock"`, `#clock-cells = <0>`, required `clock-frequency`, optional `clock-accuracy`, and `clock-output-names`.
- Still needed before final learner/example files:
  - Validate chosen target kernel headers for any example APIs.
  - Validate `assigned-clocks`/parent/rate schema if the example uses assigned clocks.
  - Validate the relevant SoC clock-controller binding if a real SoC DTS example is used.

## Learning Content Brief
- Learning path number: `22`.
- Slug: `common-clock-framework`.
- Topic scope:
  - CCF mental model, provider/consumer split, DT clock references, provider registration, clock types, consumer lifecycle, rates/parents, debugfs, error handling, and version-sensitive API drift.
  - Keep deep runtime/system PM in topic 24, PWM/RTC in topic 27, NVMEM/watchdog in topic 28, display/media/audio subsystem-specific clocking in topics 29 and 32-36.
- Beginner mental model:
  - A clock is a hardware signal feeding a device or another clock block.
  - A clock tree starts at roots such as oscillators/PLLs and flows through muxes, dividers, and gates to devices.
  - Consumers should not know register details. They ask CCF for named clocks and enable or set them through `clk_*()` APIs.
  - Providers know the hardware registers and expose clock lines to the framework.
- Core mechanism:
  - Provider registers `struct clk_hw` objects with ops and init/parent data.
  - Provider exposes those clocks to firmware consumers through OF provider callbacks.
  - Consumer `clk_get()`/`devm_clk_get()` resolves a local clock name to a provider output and returns an opaque `struct clk *`.
  - The CCF core maintains the topology, rates, prepare/enable counts, orphan reparenting, and callback dispatch.
- Important structs/APIs:
  - Provider: `struct clk_hw`, `struct clk_ops`, `struct clk_init_data`, `struct clk_parent_data`, `struct clk_hw_onecell_data`, `of_clk_hw_simple_get()`, `of_clk_hw_onecell_get()`, `devm_clk_hw_register()`, `devm_of_clk_add_hw_provider()`.
  - Clock type helpers: `clk_hw_register_fixed_rate()`, `clk_hw_register_fixed_factor()`, `clk_hw_register_gate()`, `clk_hw_register_mux()`, `clk_hw_register_divider()`, `clk_hw_register_composite()` and managed/parent-data variants.
  - Consumer: `devm_clk_get()`, `devm_clk_get_optional()`, `devm_clk_get_enabled()`, `clk_prepare_enable()`, `clk_disable_unprepare()`, `clk_get_rate()`, `clk_round_rate()`, `clk_set_rate()`, `clk_set_rate_range()`, `clk_set_parent()`, `clk_get_parent()`, `clk_bulk_*()`.
  - DT: `#clock-cells`, `clocks`, `clock-names`, `clock-output-names`, `fixed-clock`, and provider-specific clock IDs in `include/dt-bindings/clock/...`.
- Lifecycle/data flow:
  - Provider probe: map registers/acquire parent clocks -> allocate private data -> initialize `clk_hw` entries -> register each clock -> register OF provider -> unwind on errors.
  - Consumer probe: get named clocks -> optionally set parent/rate/range -> enable before touching clocked registers or starting transfer -> register subsystem object.
  - Runtime use: enable when active, disable when idle, integrate with runtime PM if the device can sleep.
  - Remove/error: stop user/subsystem activity -> disable hardware -> `clk_disable_unprepare()` in reverse order -> devm releases handles/providers after driver cleanup.
- Practical examples for later:
  - DTS-only example with a fixed-clock provider and two consumers showing `clocks`, `clock-names`, and `clk_summary` inspection.
  - Consumer driver pseudo-code: get `core`/`bus` clocks, enable in correct order, unwind failures, disable in remove.
  - Provider pseudo-code: a simple MMIO gate or onecell provider using `devm_clk_hw_register_gate()` plus `devm_of_clk_add_hw_provider()`.
- Common bugs:
  - Using provider `clock-output-names` as consumer `clock-names`.
  - Wrong `clock-names` order or typo causing `devm_clk_get()` failure or wrong clock use.
  - Forgetting `clk_prepare_enable()` before register access.
  - Calling sleepable prepare/get/rate APIs from atomic context.
  - Implementing sleepable I2C/SPI register access in provider `.enable`/`.disable`.
  - Leaking enabled clocks on probe error or remove.
  - Assuming `clk_get_rate()` is nonzero or exact without checking provider capability.
  - Setting a shared clock rate and breaking another consumer.
  - Using `clk_ignore_unused` as a production workaround.
  - Writing new provider code with old `struct clk *` registration APIs instead of `clk_hw` APIs.
- Debugging notes:
  - Inspect `/sys/kernel/debug/clk/clk_summary` for topology, rates, enable/prepare counts, and unexpected disabled/orphan clocks.
  - Look for `-EPROBE_DEFER` when a provider has not registered yet.
  - Compare DT `clocks`/`clock-names` against driver `devm_clk_get()` strings.
  - Temporarily use `clk_ignore_unused` only to prove a missing clock enable during bring-up.
  - Use dynamic debug/ftrace around probe, runtime PM, and subsystem start/stop paths when clocks are enabled/disabled indirectly.
- Production concerns:
  - Treat clock ownership like any other resource: every enable needs a matching disable on every path.
  - Use optional helpers only when the hardware truly has optional clocks.
  - Use bulk helpers for multiple clocks with identical lifecycle where supported.
  - Avoid changing rates/parents of shared clocks unless the binding/datasheet and consumers make it safe.
  - Put slow provider operations in prepare/unprepare and protect MMIO provider state with the right lock.
  - Prefer current YAML bindings and `clk_hw`/managed APIs for new code.
- Interview angles:
  - Explain `struct clk` versus `struct clk_hw`.
  - Why do both prepare and enable exist?
  - How does `devm_clk_get(dev, "ipg")` find the hardware clock from DT?
  - What does `#clock-cells = <0>` versus `<1>` mean?
  - Why should consumers not reference provider output names directly?
  - How would you debug a device that works with the bootloader but fails after `clk_disable_unused`?
  - How would you write a provider for an I2C clock generator without sleeping in `.enable`?
  - What can go wrong when setting the rate of a shared parent clock?
