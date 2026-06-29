# Topic Brief - 23 - Regulator Framework

## Output Targets
- Knowledge: `knowledge/23-regulator-framework.md`
- Interview: `interview/23-regulator-framework.md`
- Example: `examples/23-regulator-framework/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch20` | `docs/Linux Device Driver Development/Chapter 20-Regulator Framework.md` | read/mapped/covered/merged | Primary dedicated source: regulator purpose, PMIC/provider interface, consumer interface, constraints, `struct regulator_desc`, `struct regulator_ops`, `struct regulator_config`, `struct regulator_init_data`, DT producer and consumer bindings, enable/disable, voltage/current/mode APIs, sysfs inspection, ISL6271A case study, and dummy regulator example. |
| `ldd1-ch06` | `docs/Linux Device Driver Development/Chapter 6-The Concept of a Device Tree .md` | read/mapped/related | General DT resource model and reminder that subsystem-specific resources such as regulators are binding-defined properties rather than generic resource arrays. |
| `ldd1-ch09` | `docs/Linux Device Driver Development/Chapter 9-Regmap API .md` | read/mapped/related | PMIC/regulator driver example using regmap cache defaults from `drivers/regulator/ltc3589.c`; supports the point that many regulator providers are I2C/SPI PMIC register-map drivers. |
| `ldd1-ch14` | `docs/Linux Device Driver Development/Chapter 14-Pin Control and GPIO Subsystem.md` | read/mapped/related | Applied consumer DT example: `vmmc-supply = <&reg_3p3v>` in an MMC/pinctrl context. Full pinctrl/GPIO behavior remains topics 13-14. |
| `ldd1-ch19` | `docs/Linux Device Driver Development/Chapter 19-PWM Drivers.md` | read/mapped/related | Explicitly links next chapter to regulators and notes that some regulators may be PWM-driven. Full PWM behavior remains topic 27. |
| `ldd1-ch21` | `docs/Linux Device Driver Development/Chapter 21-Framebuffer Drivers.md` | read/mapped/related | Applied display consumer example that obtains an LCD regulator during probe; full framebuffer/display behavior remains topic 29. |
| `ldd1-ch01` | `docs/Linux Device Driver Development/Chapter 1-Introduction to Kernel Development .md` | read/mapped/related | Framework-object context: regulator devices are dynamically allocated framework objects and appear as subsystem objects such as `/sys/class/regulator`. |
| `ldd2-source-root` | `docs/Linux Device Driver Development 2/` | searched/mapped/gap | No standalone regulator-framework chapter found in book 2. Relevant material is distributed across MFD/PMIC, ASoC, CCF, PM, regmap IRQ, and debugging chapters. |
| `ldd2-ch03` | `docs/Linux Device Driver Development 2/Chapter 3-MFD_Subsystem_and_Syscon_API.md` | read/mapped/covered/merged-adjacent | Strongest second-book regulator source: PMIC as MFD parent, `mfd_cell` for `da9055-regulator`, DA9062 DT with `regulators` subnode, buck/LDO constraints, `regulator-boot-on`, min/max voltage/current properties, and PMIC child-device layout. |
| `ldd2-ch04` | `docs/Linux Device Driver Development 2/Chapter 4-Common_Clock_Framework.md` | read/mapped/related | Applied examples where a clock generator provider consumes a regulator and a camera sensor declares `DOVDD-supply`, `AVDD-supply`, and `DVDD-supply`. Full CCF remains topic 22. |
| `ldd2-ch05` | `docs/Linux Device Driver Development 2/Chapter 5-ASoC_Framework.md` | read/mapped/related | ASoC power graph context: `SND_SOC_DAPM_SUPPLY`, supply widgets, and subsystem power sequencing; generic regulator framework remains topic 23 and detailed ASoC remains topics 35-36. |
| `ldd2-ch06` | `docs/Linux Device Driver Development 2/Chapter 6-ASoC_Machine_Drivers.md` | read/mapped/related | Applied audio DT supply examples (`VDDA-supply`, `VDDIO-supply`, `VDDD-supply`) and `SND_SOC_DAPM_REGULATOR_SUPPLY` board/audio power-map context. |
| `ldd2-ch10` | `docs/Linux Device Driver Development 2/Chapter 10-Linux_Kernel_Power_Management.md` | read/mapped/related | DVFS/OPP context: performance points are frequency/voltage tuples, tying regulators to CPU/device power management. Full runtime/system PM remains topic 24. |
| `ldd2-ch14` | `docs/Linux Device Driver Development 2/Chapter 14-Linux_Kernel_Debugging_Tips.md` | read/mapped/covered/merged-adjacent | Debugging source: ftrace events include a `regulator` event directory; useful for tracing regulator enable/disable/rate-like operations. Full tracing remains topic 37. |
| `ldd2-ch02` | `docs/Linux Device Driver Development 2/Chapter 2-Regmap_API.md` | read/mapped/related | PMIC interrupt handling and regmap-IRQ context; relevant for PMIC regulator drivers that share register maps and IRQ lines. Detailed regmap IRQ remains topic 19. |
| `notion-index` | `docs/Linux-Device-Driver-Notion/Linux Device Drivers.md` | inspected/mapped/index-only | Notion index inspected; no standalone regulator-framework chapter found. Relevant material is distributed across DT/resource/I2C/SPI/media/pinctrl notes. |
| `notion-ch05-part2` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 2 Probe, Remove & Resource Management.md` | read/mapped/covered/merged-adjacent | Managed-resource table maps `regulator_get()` to `devm_regulator_get()`, reinforcing devres lifetime for regulator handles. |
| `notion-ch06-part1` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 1 Device Tree Fundamentals.md` | read/mapped/covered/merged | Best Notion DT source: `regulator-fixed` node, `regulator-name`, min/max microvolts, GPIO enable, `enable-active-high`, and consumer supplies such as `VDDA-supply`, `VDDIO-supply`, `vbus-supply`, and `vmmc-supply`. |
| `notion-ch06-part2` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 2 Device Addressing and Resources.md` | read/mapped/related | Applied I2C/DT resource example with `VDDA-supply`; useful for explaining supply property naming in ordinary consumers. |
| `notion-ch07-part1` | `docs/Linux-Device-Driver-Notion/Chapter 7-Part 1 I2C Architecture and Driver Structures.md` | read/mapped/related | I2C device taxonomy mentions PMICs as common I2C devices; supports PMIC bus-placement context. |
| `notion-ch07-part3` | `docs/Linux-Device-Driver-Notion/Chapter 7-Part 3 Device Tree Integration.md` | read/mapped/related | I2C/audio codec DT supply examples (`DCVDD-supply`, `DBVDD-supply`, `AVDD-supply`, `CPVDD-supply`) and checklist item for supply references. |
| `notion-ch08-part3` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 3 Device Tree Integration and Userspace Inter.md` | read/mapped/related | SPI ADC/CAN examples using `vref-supply`, `vcc-supply`, `vdd-supply`, and `xceiver-supply`; demonstrates supply naming across buses. |
| `notion-ch14-part1` | `docs/Linux-Device-Driver-Notion/Chapter 14-Part 1 Pin Control Framework.md` | read/mapped/related | Repeats MMC `vmmc-supply` context inside pinctrl examples; kept as a separate Notion source, not treated as duplicate of book 1. |
| `notion-ch07-extra-v4l2-part2` | `docs/Linux-Device-Driver-Notion/Chapter 7-v4l2_part2.md` | read/mapped/covered/merged-adjacent | Applied camera-sensor lifecycle source: `devm_regulator_get("vdd")`, `regulator_enable()`, `regulator_disable()`, clock/GPIO sequencing, and power-on stabilization delay. |
| `notion-ch16-part3` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 3 Advanced Topics.md` | read/mapped/related | Mentions PMICs in advanced GPIO/IRQ architecture context; relevant only as cross-framework PMIC context. |

## Source Files Read
- `ldd1-ch20`: complete chapter, including producer/provider data structures, constraints, board-file versus DT init data, `of_regulator_match()`, `regulator_config`, `regulator_ops`, PMIC probe/remove flow, ISL6271A case study, dummy regulator example, sysfs inspection, consumer APIs, voltage/current/mode APIs, and consumer DT binding.
- `ldd1-ch06`: DT resource section and application-specific property boundary around non-generic resources such as regulators.
- `ldd1-ch09`: regmap cache example using `drivers/regulator/ltc3589.c`.
- `ldd1-ch14`: `vmmc-supply` MMC example.
- `ldd1-ch19`: summary note that some regulators are PWM-driven.
- `ldd1-ch21`: framebuffer probe excerpt with `regulator_get(&pdev->dev, "lcd")`.
- `ldd1-ch01`: framework-object list that includes regulator devices.
- `ldd2-ch03`: MFD cells and DT binding sections around DA9055/DA9062 PMIC regulator children and `regulators` subnodes.
- `ldd2-ch04`: clock-provider private data with `struct regulator *supply` and camera sensor `*-supply` DT excerpt.
- `ldd2-ch05`: DAPM supply-widget excerpts.
- `ldd2-ch06`: SGTL5000 DT supply references and DAPM regulator-supply routing note.
- `ldd2-ch10`: CPUfreq/DVFS OPP voltage/frequency section.
- `ldd2-ch14`: ftrace event-list section showing `regulator` events.
- `ldd2-ch02`: PMIC regmap IRQ handling context.
- `notion-index`: inspected to confirm no standalone regulator chapter.
- `notion-ch05-part2`: managed-resource table entry for `devm_regulator_get()`.
- `notion-ch06-part1`: fixed regulator and multiple consumer supply examples.
- `notion-ch06-part2`: `VDDA-supply` resource example.
- `notion-ch07-part1`: I2C architecture mention of PMICs.
- `notion-ch07-part3`: audio codec supply examples and supply-reference checklist.
- `notion-ch08-part3`: SPI ADC/CAN supply examples.
- `notion-ch14-part1`: repeated MMC `vmmc-supply` example read independently from `ldd1-ch14`.
- `notion-ch07-extra-v4l2-part2`: sensor power-on/off and probe resource snippets.
- `notion-ch16-part3`: PMIC advanced-context mention.

### Inventory Decisions
- `ldd1-ch20` is the only dedicated regulator-framework source and is primary for framework structure, provider callbacks, constraints, consumer APIs, and DT binding shape.
- `ldd2` has no dedicated regulator chapter. `ldd2-ch03` is the strongest second-book source because PMICs are commonly MFD parents with regulator child devices. Other `ldd2` files are applied examples or cross-framework context only.
- Notion has no dedicated regulator chapter. `notion-ch06-part1` is the strongest Notion DT source; `notion-ch07-extra-v4l2-part2` is the strongest lifecycle example.
- Same-number sources were not merged by number. For example, `ldd1-ch20` is a regulator chapter, while Notion has no Chapter 20 file; Notion Chapter 7 and Chapter 8 files were read as I2C/SPI/V4L2-specific context, not as duplicates.
- Audio, media, framebuffer, PWM, CCF, regmap IRQ, MFD, and PM sources are retained only as regulator applications or cross-framework boundaries. Their subsystem-specific behavior remains in their learning-path topics.

## Merged Source Notes
- The regulator framework abstracts hardware that supplies voltage/current to devices. It lets consumer drivers request named supplies and lets provider drivers expose PMIC/LDO/buck/switch outputs through a common API.
- The main split is:
  - Provider side: PMIC/regulator driver owns hardware, registers each output with regulator core, implements `struct regulator_ops`, and describes capabilities with `struct regulator_desc`.
  - Consumer side: ordinary device driver obtains opaque `struct regulator *` handles and calls consumer APIs such as `regulator_enable()`, `regulator_disable()`, `regulator_set_voltage()`, and `regulator_set_current_limit()`.
  - Machine/firmware side: board constraints define safe voltage/current/mode/status operations and suspend states.
- `struct regulator_desc` describes fixed per-output capability: name, ID, type (`REGULATOR_VOLTAGE` or `REGULATOR_CURRENT`), voltage selector count, linear range fields, operation callbacks, and ownership/provider metadata.
- `struct regulation_constraints` is a safety contract. It limits what consumers can request: min/max voltage/current, valid modes, valid operations, suspend states, `always_on`, `boot_on`, offset, and startup behavior.
- DT regulator provider nodes commonly live under a PMIC `regulators` subnode. Each child describes one output with properties such as `regulator-name`, `regulator-min-microvolt`, `regulator-max-microvolt`, `regulator-min-microamp`, `regulator-max-microamp`, `regulator-always-on`, `regulator-boot-on`, parent supply links, and ramp/delay fields.
- Consumer DT binding uses `<supply-name>-supply = <&regulator_node>;`. The driver calls `devm_regulator_get(dev, "<supply-name>")`. Example: `vmmc-supply = <&reg_3p3v>` maps to `devm_regulator_get(dev, "vmmc")`.
- `regulator_enable()` and `regulator_disable()` are reference-counted from the consumer perspective. A shared regulator is physically disabled only when the core determines no enabled users remain and constraints allow disabling.
- Consumers should normally acquire regulators in probe, configure voltage/current/load before enabling where required by hardware, enable before touching powered registers or starting I/O, and disable on every error path, remove, runtime suspend, stream stop, or power-off sequence.
- Voltage/current requests are ranges, not exact magic numbers. The core chooses a supported selector using provider capabilities and constraints. Consumer code must check return values and, where needed, verify with `regulator_get_voltage()` or `regulator_get_current_limit()`.
- Direct mode changes are risky on shared supplies. Older material contrasts direct `regulator_set_mode()` with load-based optimum-mode selection; modern learner docs should teach load/current-aware APIs and shared-supply caution.
- PMIC regulators often sit behind I2C/SPI and use regmap for register access. Multi-function PMICs may expose regulator, RTC, GPIO, watchdog, and IRQ child devices through MFD cells; regulator behavior should be taught without turning the chapter into MFD/syscon or regmap IRQ.
- Debugging sources include sysfs regulator class inspection (`/sys/class/regulator/...`) from book 1 and regulator ftrace events from book 2. Current learner docs should also mention debugfs summaries if available on the target kernel.
- Applied examples show regulators in MMC (`vmmc`), USB VBUS (`vbus`), camera sensors (`DOVDD`, `AVDD`, `DVDD`, `vdd`), SPI ADC/CAN (`vref`, `vcc`, `xceiver`), audio codecs (`VDDA`, `VDDIO`, `VDDD`, `DCVDD`, `AVDD`), framebuffer LCD power, clock generators, and DVFS OPPs.

## Source Differences
- `ldd1-ch20` is older and uses board-file/platform-data initialization heavily. Teach it as legacy recognition; current embedded Linux should prefer DT/YAML bindings for board-specific regulator constraints.
- `ldd1-ch20` uses `regulator_register()` and manual `regulator_unregister()` in the dummy provider. Current new provider code should usually prefer `devm_regulator_register()` unless there is a specific reason for manual lifetime management.
- `ldd1-ch20` shows older prototypes and callback names. Current headers include much richer `struct regulator_desc`, `struct regulator_ops`, `struct regulator_config`, voltage-map helpers, linear ranges, active discharge, bypass, pull-down, error flags, notifiers, coupling, and `enable_time`/settling behavior.
- `ldd1-ch20` says `of_regulator_match().name` should match a DT label-like value. Current docs should explain carefully: provider matching depends on regulator child node names, descriptor `of_match`, and binding conventions; labels are just DTS-local references and are not runtime names.
- `ldd1-ch20` exposes internal `struct regulator` layout from old `drivers/regulator/internal.h`. Learner docs should not teach drivers to depend on its fields; consumers treat `struct regulator *` as opaque.
- `ldd1-ch20` has extraction/code issues: stray `dts`/`c` markers, `regulator_set_uptimum_mode()` typo for `regulator_set_optimum_mode()`, `regulator_enable(regulator)` missing type names, a `dummy_regulator_rdev` array/pointer mismatch in the example, and a likely `io_regulator` versus `main_regulator` naming error in the consumer snippet.
- `regulator_set_optimum_mode()` appears in older sources. Current headers document `regulator_set_load()` as the consumer load request API and may still provide compatibility depending on kernel version. Learner docs should validate against target headers and avoid presenting old names as universal.
- `ldd1-ch20` mentions sysfs as one of the framework interfaces and shows `/sys/class/regulator`. Current docs should frame this as inspection/debug/ABI exposed by the kernel, not as the normal control path for device power policy.
- `ldd2-ch03` models regulators mostly as PMIC MFD children. Good for real hardware layout, but regulator provider internals remain from `ldd1-ch20` plus current external validation.
- Notion DT files use supply examples but rarely explain regulator core semantics. They are useful for beginner DT pattern recognition, not enough for provider implementation.
- Notion V4L2 power snippets show `regulator_enable()`/`regulator_disable()` but omit robust error unwinding after later clock/GPIO/register failures. Learner docs should add correct unwind and sequencing.

## Gaps / Uncertainties
- Internal sources do not cover modern consumer helpers in depth: `devm_regulator_get_optional()`, `devm_regulator_get_enable()`, `devm_regulator_bulk_get_enable()`, `regulator_bulk_*()`, `regulator_set_load()`, `regulator_get_error_flags()`, notifiers, or coupling.
- Internal sources do not deeply cover current YAML schema validation for regulator provider/consumer bindings, fixed regulators, GPIO-controlled regulators, enable-active-high/open-drain polarity, startup/off-on delays, and `vin-supply` naming.
- Internal sources do not explain regulator debugfs/debug output in current kernels. They mention sysfs and ftrace events, but final docs should validate target-kernel debug interfaces.
- Runtime PM and system suspend ordering with regulators, clocks, resets, pinctrl, and power domains is only applied through snippets. Deep sequencing belongs to topic 24, but topic 23 should still include basic error path and runtime PM rules.
- Provider locking and sleepability are not treated deeply. PMIC providers using I2C/SPI/regmap can sleep in register operations; consumer drivers must not call regulator enable/voltage APIs from atomic context unless the specific API and provider path are known safe.
- Regulator coupling, load balancing, bypass, active discharge, over-current/protection events, and notification handling require current external validation before a production-level provider example.
- The internal dummy regulator example is not production-ready and has stale/manual lifetime patterns. A later example should likely be DTS-focused plus consumer pseudo-code, or a very clearly learning-only fixed-regulator DTS, rather than an unvalidated fake provider module.

## External Validation
- Used: `https://docs.kernel.org/driver-api/regulator.html`
  - Validated current regulator overview: consumer/provider split, machine constraints, sysfs/debug role, consumer APIs, bulk helpers, optional/exclusive/get-enable helpers, load/mode behavior, notifier/error APIs, and provider registration concepts.
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/regulator/consumer.h`
  - Validated current consumer API drift: `devm_regulator_get_optional()`, `devm_regulator_get_enable()`, `devm_regulator_get_enable_optional()`, bulk helpers, load/current/voltage APIs, error flags, notifiers, and opaque `struct regulator *` use.
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/regulator/driver.h`
  - Validated current provider API drift: `struct regulator_desc`, `struct regulator_ops`, `struct regulator_config`, `devm_regulator_register()`, linear range helpers, active discharge/bypass/pull-down/error-flag/provider callbacks, and provider-private data accessors.
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/regulator/machine.h`
  - Validated current `struct regulation_constraints`, valid operation/mode masks, suspend state fields, `always_on`, `boot_on`, coupling, and machine-constraint concepts.
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/Documentation/devicetree/bindings/regulator/regulator.yaml`
  - Validated current common regulator binding properties and YAML schema direction.
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/Documentation/devicetree/bindings/regulator/fixed-regulator.yaml`
  - Validated current fixed-regulator binding concepts, fixed voltage constraints, GPIO-controlled enable properties, and delay/startup fields for a likely learning DTS example.
- Still needed before final learner/example files:
  - Validate example code, if any, against the target kernel headers.
  - Validate any specific PMIC/fixed-regulator binding used in an example with `dt_binding_check`/`dtbs_check` where practical.
  - Check target-kernel availability of `devm_regulator_get_enable*()` and bulk managed helpers before recommending them as buildable code.

## Learning Content Brief
- Learning path number: `23`.
- Slug: `regulator-framework`.
- Topic scope:
  - Regulator framework mental model, provider/consumer split, machine constraints, DT supply bindings, consumer lifecycle, voltage/current/load/mode control, provider registration, PMIC/MFD context, debugging, common bugs, and version-sensitive API drift.
  - Keep full MFD/syscon/regmap IRQ in topic 19, CCF in topic 22, runtime/system PM in topic 24, PWM in topic 27, watchdog/NVMEM in topic 28, display/media/audio subsystem-specific power sequencing in topics 29 and 32-36.
- Beginner mental model:
  - A regulator is a controllable power rail: it may turn a supply on/off, set voltage, set current limit, or choose operating mode.
  - A consumer is the device that needs power, such as an MMC slot, camera sensor, codec, CAN controller, display panel, or USB VBUS.
  - A provider is the hardware that supplies power, often a PMIC with buck/LDO outputs.
  - DT connects them with `<name>-supply = <&reg_node>`, and the driver requests `<name>`.
- Core mechanism:
  - Provider probe registers one `struct regulator_dev` per output using `struct regulator_desc` and `struct regulator_config`.
  - Provider ops implement hardware behavior: list/map/set/get voltage, enable/disable, current limits, mode, bypass, status/error reporting, and protection features.
  - Machine/DT constraints restrict what consumers may do; the core rejects unsafe operations outside the allowed ranges/masks.
  - Consumer code obtains a `struct regulator *` handle, sets required voltage/current/load, enables the rail, waits for hardware stabilization when required, uses the device, then disables/unwinds.
- Important structs/APIs:
  - Provider: `struct regulator_desc`, `struct regulator_ops`, `struct regulator_config`, `struct regulator_dev`, `struct regulator_init_data`, `struct regulation_constraints`, `devm_regulator_register()`, `rdev_get_drvdata()`, `rdev_get_id()`, linear voltage/list/map helpers.
  - Consumer: `devm_regulator_get()`, `devm_regulator_get_optional()`, `devm_regulator_get_enable()`, `regulator_enable()`, `regulator_disable()`, `regulator_is_enabled()`, `regulator_set_voltage()`, `regulator_get_voltage()`, `regulator_set_current_limit()`, `regulator_get_current_limit()`, `regulator_set_load()`, `regulator_set_mode()`, `regulator_bulk_*()`.
  - DT: `regulators` subnode, `regulator-name`, `regulator-min-microvolt`, `regulator-max-microvolt`, `regulator-min-microamp`, `regulator-max-microamp`, `regulator-always-on`, `regulator-boot-on`, `*-supply`, `vin-supply`, GPIO enable properties, startup/off-on delay properties.
- Lifecycle/data flow:
  - Provider probe: map/regmap hardware -> allocate private state -> parse regulator child nodes/constraints -> initialize descriptors/config -> register each regulator -> expose child regulators to consumers.
  - Consumer probe: acquire supplies by local names -> set voltage/current/load if required -> enable before register access or subsystem registration that touches hardware -> unwind in reverse order after failures.
  - Runtime active path: enable regulators before clocks/register access; disable after transactions/streaming stop once hardware is quiesced.
  - Remove/suspend: stop users/IRQ/DMA first, put device in safe state, disable supplies in correct order, then let devm release handles.
- Practical examples for later:
  - DTS-only fixed-regulator example feeding a fake MMC or sensor node using `vmmc-supply`/`vdd-supply`.
  - Consumer pseudo-code with one required supply and one optional supply, showing `devm_regulator_get()`, `regulator_set_voltage()`, `regulator_enable()`, error unwind, and `regulator_disable()`.
  - Optional provider pseudo-code for an I2C PMIC output using `devm_regulator_register()` and regmap, marked learning-only unless built against real hardware.
- Common bugs:
  - Driver calls `devm_regulator_get(dev, "vdd")` but DT property is `VDD-supply`, `vcc-supply`, or missing.
  - Treating `regulator-name` as the consumer lookup name instead of the `<name>` part of `<name>-supply`.
  - Using optional helpers to hide a missing required supply.
  - Enabling a regulator and leaking it on probe failure.
  - Touching device registers before the rail is enabled and stabilized.
  - Setting an exact voltage without checking constraints/provider support.
  - Changing mode/voltage on a shared rail and breaking another consumer.
  - Relying on bootloader-enabled rails instead of declaring and enabling supplies.
  - Disabling an `always-on` or shared regulator manually/forcefully in normal paths.
  - Calling regulator APIs from inappropriate atomic contexts.
- Debugging notes:
  - Compare driver supply IDs with DT `*-supply` property names.
  - Check probe deferral when provider PMIC/regulator has not registered.
  - Inspect `/sys/class/regulator/` when enabled in the kernel for names, type, voltage, state, and consumers.
  - Use ftrace regulator events where available.
  - Check dmesg for constraint violations, missing supply messages, dummy supply fallback warnings, and `-EPROBE_DEFER`.
  - Verify board rails physically with a meter/scope during bring-up when software state and hardware disagree.
- Production concerns:
  - DT binding must describe hardware rails and safe constraints, not Linux driver convenience.
  - Enable/disable ordering must follow the datasheet and interact correctly with clocks, resets, pinctrl, GPIOs, and runtime PM.
  - Every successful enable needs a matching disable on every error/remove/suspend path unless a managed get-enable helper is deliberately used and its lifetime is exactly correct.
  - Voltage/current/load requests must be checked and should be tied to subsystem policy, OPPs, or datasheet limits.
  - Shared supplies need coordination; avoid direct mode/force-disable/rate-like changes unless ownership is clear.
  - Provider code should use current managed registration and regmap helpers, not stale manual examples, for new PMIC drivers.
- Interview angles:
  - Explain regulator provider versus consumer.
  - What does `vdd-supply = <&reg_3v3>` mean, and why does the driver request `"vdd"`?
  - Difference between `regulator-name` and consumer supply name.
  - How constraints protect hardware.
  - How to unwind two enabled regulators if the second enable or later clock enable fails.
  - Why `regulator_disable()` may not physically turn off a shared rail.
  - How to debug a device that works only because the bootloader left power on.
  - How PMIC/MFD/regmap fit around regulator providers.
  - Why optional supplies and `always-on`/`boot-on` need careful review.
