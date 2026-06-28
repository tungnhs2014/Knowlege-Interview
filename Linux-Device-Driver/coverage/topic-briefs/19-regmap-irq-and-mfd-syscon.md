# Topic Brief - 19 - Regmap IRQ And MFD/Syscon

## Output Targets
- Knowledge: `knowledge/19-regmap-irq-and-mfd-syscon.md`
- Interview: `interview/19-regmap-irq-and-mfd-syscon.md`
- Example: `examples/19-regmap-irq-and-mfd-syscon/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-source-root` | `docs/Linux Device Driver Development/` | searched/mapped/gap | No dedicated MFD, syscon, `simple-mfd`, or regmap IRQ chapter was found in book 1. Search hits were generic IRQ sharing or core regmap material. |
| `ldd1-ch09` | `docs/Linux Device Driver Development/Chapter 9-Regmap API .md` | read/mapped/related | Core regmap prerequisite: `struct regmap`, `struct regmap_config`, register access, cache/access policy. Does not cover regmap IRQ, MFD, syscon, or `simple-mfd`. |
| `ldd1-ch16` | `docs/Linux Device Driver Development/Chapter 16-Advanced IRQ Management.md` | read/mapped/related | Generic advanced IRQ background only. It helps with parent/child IRQ reasoning, but has no regmap IRQ or MFD/syscon material. |
| `ldd2-ch02` | `docs/Linux Device Driver Development 2/Chapter 2-Regmap_API.md` | read/mapped/covered/merged | Primary regmap IRQ source: IRQ domain recap, nested IRQ model, `struct regmap_irq`, `struct regmap_irq_chip`, `struct regmap_irq_chip_data`, `devm_regmap_add_irq_chip()`, `regmap_irq_get_virq()`, threaded parent handler, child virq mapping, and GPIO expander example. |
| `ldd2-ch03` | `docs/Linux Device Driver Development 2/Chapter 3-MFD_Subsystem_and_Syscon_API.md` | read/mapped/covered/merged | Primary MFD/syscon source: MFD parent/child model, `struct mfd_cell`, child resources, `devm_mfd_add_devices()`, MFD IRQ-domain handoff from regmap IRQ, multi-address I2C MFD caveat, DT child-node binding, syscon lookup APIs, and `simple-mfd`. |
| `ldd2-ch12` | `docs/Linux Device Driver Development 2/Chapter 12-NVMEM_Framework.md` | read/mapped/related | Shows an MMIO NVMEM provider node using a `"syscon"` compatible fallback. Useful as applied syscon/NVMEM context; full NVMEM remains topic 28. |
| `ldd2-ch14` | `docs/Linux Device Driver Development 2/Chapter 14-Linux_Kernel_Debugging_Tips.md` | read/mapped/supporting | Lists `regmap` among ftrace event directories. Supports debug workflow notes after current-kernel validation. |
| `notion-ch05-part1` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 1 Platform Bus & Driver Basics.md` | read/mapped/related | Mentions `struct platform_device` containing `struct mfd_cell *mfd_cell`, reinforcing that MFD children appear as platform devices. Main platform-bus material belongs to topic 09. |
| `notion-ch15-part2` | `docs/Linux-Device-Driver-Notion/Chapter 15-Part 2 IRQ Chip Integration.md` | read/mapped/supporting | GPIO IRQ-domain, chained/nested IRQ, and modern gpiolib irqchip background. Useful contrast for when regmap IRQ is preferable; detailed GPIO irqchip remains topic 14. |
| `notion-ch16-part1` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 1 IRQ Architecture and Propagation.md` | read/mapped/supporting | IRQ architecture, hwirq/virq, IRQ domain mapping, and `handle_nested_irq()` background. Supports regmap IRQ mental model; generic IRQ fundamentals remain topic 15. |
| `notion-ch16-part2` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 2 IRQ Multiplexing - Chained and Nested Inter.md` | read/mapped/supporting | Chained versus nested IRQ comparison and implementations. Important because regmap IRQ uses a threaded/nested style; full manual implementations remain topics 14-15. |
| `notion-ch16-part3` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 3 Advanced Topics.md` | read/mapped/covered/merged | Primary Notion regmap IRQ source: purpose, use cases, structures, `devm_regmap_add_irq_chip()`, MFD child registration with `regmap_irq_get_domain()`, `regmap_irq_get_virq()`, decision tree, and common IRQ traps. |

## Source Files Read
- `ldd1-source-root`: searched for `regmap_irq`, `regmap irq`, `MFD`, `syscon`, `simple-mfd`, `mfd_cell`, `mfd_add_devices`, `syscon_node_to_regmap`, and related strings. No direct ldd1 source for topic 19 was found.
- `ldd1-ch09`: core regmap chapter, used only as prerequisite context for why regmap IRQ sits on top of a `struct regmap`.
- `ldd1-ch16`: search hits around shared IRQ and advanced IRQ concepts; no regmap IRQ/MFD/syscon coverage.
- `ldd2-ch02`: "Regmap and IRQ management", "Quick recap on Linux kernel IRQ management", IRQ domain mapping recap, "Regmap IRQ API and data structures", "`struct regmap_irq_chip` and `struct regmap_irq`", "`struct regmap_irq_chip_data`", "Regmap IRQ API", and "Regmap IRQ API example".
- `ldd2-ch03`: complete MFD/syscon chapter, especially "Introducing the MFD subsystem and Syscon APIs", `struct mfd_cell`, child resources and named IRQ resources, `devm_mfd_add_devices()`, MFD IRQ-domain parameter, DA9055 regmap IRQ/MFD example, multi-address I2C MFD subdevice caveat, "Device tree binding for MFD devices", "Understanding Syscon and simple-mfd", syscon lookup APIs, and `simple-mfd` example.
- `ldd2-ch12`: "Device tree bindings for NVMEM providers" excerpt with `"fsl,imx6sx-ocotp", "syscon"`.
- `ldd2-ch14`: tracing event-list section showing `regmap` as a trace event group.
- `notion-ch05-part1`: `struct platform_device` section showing `struct mfd_cell *mfd_cell`.
- `notion-ch15-part2`: IRQ chip integration headings and relevant IRQ-domain/nested IRQ sections.
- `notion-ch16-part1`: IRQ architecture, hwirq/virq, IRQ domain, mapping, and propagation sections.
- `notion-ch16-part2`: chained/nested interrupt comparison and implementation sections.
- `notion-ch16-part3`: "Regmap IRQ API", example implementation, MFD child registration snippet, `regmap_irq_get_virq()`, "Device Tree IRQ Binding", and "Best Practices".

## Merged Source Notes
- **Regmap IRQ mental model:** a register-backed interrupt controller usually has status, mask/unmask, ack, wake, and type registers. Regmap IRQ lets the driver describe those registers and bit positions once, then the regmap IRQ core creates an IRQ domain, requests a threaded parent IRQ, reads/acks/masks through regmap, and dispatches nested child IRQs.
- **MFD mental model:** an MFD chip is one physical device with several functional blocks. The parent/core driver owns common services such as bus access, regmap, shared IRQ handling, chip identification, and subdevice creation. Each child appears to subsystem drivers as a platform device with its own resources.
- **Regmap IRQ and MFD fit together:** for PMIC-style devices, the parent I2C/SPI/MMIO driver creates the main regmap, registers a regmap IRQ chip, then passes `regmap_irq_get_domain()` into `devm_mfd_add_devices()` so child cells with `IORESOURCE_IRQ` get Linux virqs mapped through the parent IRQ domain.
- **Child resources:** `struct mfd_cell` describes child name, optional `.of_compatible`, platform data, resources, and resource count. Child drivers retrieve resources with platform APIs such as `platform_get_irq_byname()` or `platform_get_irq()`.
- **Resource translation:** `devm_mfd_add_devices()` can translate child `IORESOURCE_MEM` using the parent memory base and child `IORESOURCE_IRQ` using an `irq_base` or an IRQ domain. With a regmap IRQ domain, child IRQ resources become usable virqs.
- **Device Tree MFD shape:** parent MFD nodes contain child nodes for subdevices. Child `compatible` strings should line up with the MFD cell `.of_compatible` and the child platform driver's OF match table. This avoids ambiguous name-only matching.
- **Multi-address I2C MFDs:** some chips expose internal functions at additional I2C addresses. The MFD core creates platform devices, not I2C clients, so the parent driver may need dummy/secondary I2C clients and per-subdevice regmaps. The old source uses `i2c_new_dummy()`; current code must validate modern helpers before teaching code.
- **Syscon mental model:** a syscon is a miscellaneous MMIO register block shared by unrelated consumers. It is not cohesive enough to be one normal functional device, so the syscon framework exposes the region as a regmap discoverable by node, compatible string, or phandle.
- **Syscon lookup APIs:** internal and current-header sources align on `syscon_node_to_regmap()`, `syscon_regmap_lookup_by_compatible()`, and `syscon_regmap_lookup_by_phandle()`. Current headers also expose phandle-args and optional variants.
- **`simple-mfd` mental model:** for simple MMIO register blocks whose child devices need no parent-driver probing/configuration, DT can use `"syscon", "simple-mfd"` so child platform devices are populated without a custom MFD core driver.
- **Nested IRQ background:** Notion and ldd2 agree that slow bus-backed interrupt controllers should use threaded/nested IRQ handling, not chained hard-IRQ handlers. Regmap IRQ's parent handler is threaded and dispatches child interrupts with nested IRQ semantics.
- **Debugging:** internal docs only lightly mention `regmap` trace events. Current source validation should drive final debug commands around ftrace/regmap events, dynamic debug, `/proc/interrupts`, debugfs regmap entries, and child IRQ mapping inspection.

## Source Differences
- `ldd1` has no dedicated topic-19 source. It teaches core regmap and generic IRQ concepts only; do not force ldd1 content into MFD/syscon/regmap IRQ teaching beyond prerequisite context.
- `ldd2-ch02` and `notion-ch16-part3` overlap on regmap IRQ purpose and APIs. `ldd2-ch02` is deeper on internal mechanics, IRQ domain mapping, and field meanings. Notion is clearer for beginner use cases and approach selection.
- `ldd2-ch03` is the only internal source with complete MFD/syscon/`simple-mfd` coverage. Notion has platform-device/MFD-cell and IRQ background but no dedicated MFD/syscon chapter.
- The internal sources target older kernels. Current `include/linux/regmap.h` has API drift:
  - `struct regmap_irq` now uses a nested `struct regmap_irq_type type` rather than the old flat `type_reg_offset`, `type_rising_mask`, and `type_falling_mask` fields shown in the book excerpt.
  - `struct regmap_irq_chip` has many more fields, including domain suffix/main-status/sub-IRQ/config callback support. Final learner code must not paste the old struct definition verbatim.
  - `devm_regmap_add_irq_chip_fwnode()` exists in current headers and may matter for firmware-node-specific IRQ domains.
- Current `include/linux/mfd/core.h` adds fields not shown in the book excerpt, including software-node and OF address matching details. The conceptual `mfd_cell` model remains valid, but examples must be checked against current headers.
- Current `include/linux/mfd/syscon.h` exposes extra helper variants such as `syscon_regmap_lookup_by_phandle_args()` and `syscon_regmap_lookup_by_phandle_optional()`.
- The old ldd2 source mentions `Documentation/devicetree/bindings/mfd/syscon.txt`; current kernels use YAML schemas such as `Documentation/devicetree/bindings/mfd/syscon.yaml`, plus the older `mfd.txt` still documents `simple-mfd`.
- `ldd2-ch03` says syscon nodes are declared by adding `"syscon"` to compatible strings. Current YAML schemas generally require a specific compatible plus `"syscon"` fallback, with `reg` required. Final docs should teach "specific compatible plus syscon fallback" rather than bare `"syscon"` as a universal best practice.
- `ldd2-ch03` uses `i2c_new_dummy()` and manual `i2c_unregister_device()` in an old multi-address I2C MFD example. Validate modern helpers such as managed dummy-device APIs before writing final examples.
- `notion-ch16-part3` presents a simplified `struct regmap_irq_chip` with function-pointer fields for `init_ack_masked` and `ack_invert`; current headers define these concepts differently. Use it for mental model, not code copying.

## Gaps / Uncertainties
- Need target kernel version before final code examples because regmap IRQ, MFD cell, syscon, and I2C dummy-client APIs drift across releases.
- Need current in-tree examples before final learner code:
  - PMIC/MFD parent using `devm_regmap_add_irq_chip()` plus `devm_mfd_add_devices()`.
  - GPIO or input child using a virq passed through an MFD cell resource.
  - A syscon consumer using `syscon_regmap_lookup_by_phandle()`.
  - A `simple-mfd` DT node with children validated by YAML bindings.
- Internal sources do not cover `regmap_irq_get_domain()` deeply, although it is essential for MFD child IRQ resource translation.
- Internal sources do not deeply cover hierarchical IRQ domains, immutable irqchip guidance, or modern GPIO irqchip helpers in relation to regmap IRQ. Keep this topic focused on regmap IRQ/MFD/syscon and cross-link topics 14-15 for GPIO/IRQ internals.
- Internal sources do not provide a production-grade example with current probe/remove callback signatures, PM handling, child device teardown, and runtime validation.
- Syscon is easy to misuse as a global register escape hatch. The final topic needs explicit design guidance: prefer a real subsystem/provider API when the register block has cohesive functionality; use syscon for miscellaneous shared control registers.
- Learning-path boundaries:
  - Topic 14 owns GPIO controller and gpiochip irqchip integration.
  - Topic 15 owns generic IRQ domains, top/bottom halves, threaded IRQ, and manual interrupt-controller design.
  - Topic 18 owns core regmap access/cache/locking.
  - Topic 24 owns suspend/resume and wakeup policy depth.
  - Topic 28 owns NVMEM/watchdog details that may use syscon or regmap.
  - Topics 35-36 own ASoC codec/machine-driver use of regmap/MFD.

## External Validation
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/regmap.h`
  - Validated current `struct regmap_irq`, `struct regmap_irq_type`, `struct regmap_irq_chip`, `regmap_add_irq_chip()`, `devm_regmap_add_irq_chip()`, fwnode variants, `regmap_irq_chip_get_base()`, `regmap_irq_get_virq()`, and `regmap_irq_get_domain()`.
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/drivers/base/regmap/regmap-irq.c`
  - Validated that current regmap IRQ code requests a threaded parent IRQ with `IRQF_ONESHOT`, creates an IRQ domain, maps child IRQs through regmap IRQ domain operations, and calls `handle_nested_irq()` for active child IRQs.
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/mfd/core.h`
  - Validated current `struct mfd_cell`, `mfd_get_cell()`, `mfd_add_devices()`, `devm_mfd_add_devices()`, and `mfd_remove_devices()` declarations and field drift.
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/mfd/syscon.h`
  - Validated current syscon lookup helper prototypes and `CONFIG_MFD_SYSCON` fallback behavior.
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/Documentation/devicetree/bindings/mfd/syscon.yaml`
  - Validated current syscon YAML schema direction, `compatible`/`reg` requirements, and modern schema location.
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/Documentation/devicetree/bindings/mfd/mfd.txt`
  - Validated the documented `simple-mfd` binding concept and example.
- Still needed before final knowledge/example files:
  - Validate current managed I2C dummy-client helpers for multi-address MFD examples.
  - Validate current `regmap` ftrace event names and debugfs paths on the target kernel.
  - Validate one or two current in-tree PMIC/MFD examples for recommended resource ordering and child registration style.

## Learning Content Brief
- Topic scope:
  - Teach how register-backed interrupt controllers use **regmap IRQ**.
  - Teach how **MFD core drivers** split one physical chip into child platform devices.
  - Teach how **regmap IRQ domains** provide child IRQ resources to MFD children.
  - Teach how **syscon** exposes shared miscellaneous MMIO register blocks as regmaps.
  - Teach when **simple-mfd** avoids a custom parent driver.
  - Keep core register access/cache teaching in topic 18 and generic IRQ theory in topics 14-15.
- Beginner mental model:
  - A PMIC or SoC system-controller block often looks like one chip but behaves like many devices: regulators, RTC, watchdog, GPIO, power key, NVMEM, LEDs, and interrupt sources.
  - The parent driver handles the chip-wide pieces: register access, common interrupt status/mask/ack registers, and creation of child devices.
  - Regmap IRQ turns "bits in interrupt registers" into Linux child IRQ numbers.
  - MFD turns "functions inside the chip" into child platform devices.
  - Syscon turns "miscellaneous shared MMIO registers" into a shared regmap.
- Core mechanism:
  - Parent probe creates or receives a `struct regmap`.
  - Parent describes interrupt bits with an array of `struct regmap_irq`.
  - Parent describes the interrupt controller registers with `struct regmap_irq_chip`.
  - Parent calls `devm_regmap_add_irq_chip()` with the parent IRQ and regmap.
  - Regmap IRQ creates runtime `struct regmap_irq_chip_data`, an IRQ domain, and a threaded parent handler.
  - When hardware asserts the parent IRQ, regmap IRQ reads status registers, applies masks/ack/type/wake rules, and dispatches active child virqs using nested IRQ handling.
  - Parent describes children with `struct mfd_cell` and calls `devm_mfd_add_devices()`.
  - If the parent passes `regmap_irq_get_domain(data)` to MFD, child IRQ resources are mapped through that domain.
  - Child subsystem drivers probe as platform drivers and use normal resource APIs to get their IRQs and memory/resources.
  - Syscon consumers look up a regmap by node, compatible, or phandle and access shared MMIO registers through regmap helpers.
- Important structs/APIs:
  - Regmap IRQ: `struct regmap_irq`, `struct regmap_irq_type`, `struct regmap_irq_chip`, `struct regmap_irq_chip_data`, `devm_regmap_add_irq_chip()`, `devm_regmap_add_irq_chip_fwnode()`, `regmap_add_irq_chip()`, `regmap_irq_get_virq()`, `regmap_irq_get_domain()`, `regmap_irq_chip_get_base()`, `regmap_del_irq_chip()`.
  - IRQ core context: `struct irq_domain`, `struct irq_domain_ops`, `irq_create_mapping()`, `irq_find_mapping()`, `handle_nested_irq()`, `request_threaded_irq()`, `IRQF_ONESHOT`.
  - MFD: `struct mfd_cell`, `mfd_get_cell()`, `mfd_add_devices()`, `devm_mfd_add_devices()`, `mfd_remove_devices()`, `IORESOURCE_IRQ`, `IORESOURCE_MEM`, `PLATFORM_DEVID_AUTO`, `PLATFORM_DEVID_NONE`.
  - Platform child APIs: `platform_get_irq()`, `platform_get_irq_byname()`, `platform_get_resource()`, `dev_get_drvdata(pdev->dev.parent)`, `dev_get_platdata()`.
  - Syscon: `syscon_node_to_regmap()`, `syscon_regmap_lookup_by_compatible()`, `syscon_regmap_lookup_by_phandle()`, `syscon_regmap_lookup_by_phandle_args()`, `syscon_regmap_lookup_by_phandle_optional()`.
  - DT/bindings: `"syscon"`, `"simple-mfd"`, child `compatible`, `reg`, `interrupts`, `interrupt-controller`, `#interrupt-cells`, phandle properties.
- Lifecycle/data flow:
  - Match parent device from I2C/SPI/MMIO/DT.
  - Allocate parent private data and initialize the parent regmap.
  - Optionally verify chip ID and clear stale interrupt status.
  - Register regmap IRQ chip with the parent IRQ.
  - Store returned `struct regmap_irq_chip_data *` in parent private data.
  - Register MFD children with `devm_mfd_add_devices()`, passing child cell resources and the regmap IRQ domain when child IRQs are needed.
  - Child drivers probe as platform drivers, get resources, and register with their subsystems.
  - Runtime interrupt: hardware asserts parent IRQ; regmap IRQ thread reads status; active child virqs dispatch to child handlers.
  - Remove/unbind: unregister children/users first, stop late work/IRQs, then devm-managed regmap IRQ and regmap resources release with the parent device. For unmanaged/dummy I2C subdevices, explicitly unregister in correct order.
- Practical examples to build later:
  - Learning-only PMIC-like parent with an I2C regmap, a tiny regmap IRQ chip, two MFD cells, and child IRQ resources.
  - DTS fragment for a parent MFD node with child nodes and interrupt resources.
  - Syscon consumer pseudo-code using `syscon_regmap_lookup_by_phandle()` plus `regmap_update_bits()`.
  - `simple-mfd` DTS-only example for an MMIO system-controller block with child nodes.
- Common bugs:
  - Copying old `struct regmap_irq` or `struct regmap_irq_chip` definitions from v4.19-era material into a current kernel.
  - Forgetting `IRQF_ONESHOT` or using a hard IRQ path for I2C/SPI register access that can sleep.
  - Misdescribing status/mask/ack register bases, strides, inverted masks, or type registers.
  - Forgetting to clear stale status before enabling child IRQs, causing an interrupt storm.
  - Passing no IRQ domain to `devm_mfd_add_devices()` and then wondering why child IRQ resources do not resolve.
  - Using MFD for a device that is really one cohesive subsystem driver, or using syscon as an unstructured global register shortcut.
  - Using `simple-mfd` when child creation depends on runtime chip probing, revision detection, or parent-driver setup.
  - Letting child drivers access parent private data after parent teardown.
  - Failing to order child removal before unregistering dummy I2C clients or parent regmap access.
  - Declaring DT child compatible strings that do not match the MFD cell `.of_compatible` or child driver's OF table.
  - Confusing V4L2 "subdevice" terminology with MFD child platform devices.
- Debugging notes:
  - Check parent probe first: regmap init, chip IRQ number, status clear, and `devm_regmap_add_irq_chip()` return.
  - Inspect `/proc/interrupts` for the parent IRQ and child virqs.
  - Inspect child platform device creation under sysfs, for example parent/child device relationships and driver binding.
  - Check whether `platform_get_irq()` or `platform_get_irq_byname()` returns `-ENXIO`, `-EINVAL`, or `-EPROBE_DEFER`.
  - Verify DT: parent interrupt, child node compatible strings, child `interrupts`, `interrupt-controller` where applicable, and syscon phandles.
  - Use regmap debugfs and trace events only after validating current kernel support and ensuring registers are safe to read.
  - Use dynamic debug on parent MFD/regmap IRQ code and child drivers when probe order or IRQ mapping is unclear.
- Production concerns:
  - Keep parent/child ownership explicit: parent owns shared regmap, interrupt controller, chip-wide locking, and child creation.
  - Prefer devm-managed helpers where lifetime matches the parent device, but still explicitly stop child-visible activity before teardown.
  - Treat DT binding quality as part of the driver design; do not rely on ad hoc child names or unvalidated phandles.
  - Make register side effects explicit, especially write-one-to-clear status and masked status behavior.
  - Choose between manual IRQ domain, gpiolib irqchip, and regmap IRQ based on hardware shape and bus sleepability.
  - Use syscon sparingly and document register ownership if multiple drivers touch the same register block.
  - Validate suspend/resume: wake registers, mask state, regcache sync, and parent IRQ wake behavior must survive power transitions.
- Interview angles:
  - Explain why regmap IRQ exists instead of writing a custom IRQ domain for every PMIC/GPIO expander.
  - Explain the relationship between parent IRQ, hwirq, virq, IRQ domain, and child IRQ handlers.
  - Explain why regmap IRQ uses threaded/nested interrupt handling.
  - Explain how `struct regmap_irq` differs from `struct regmap_irq_chip`.
  - Explain what `struct regmap_irq_chip_data` owns at runtime.
  - Explain how `regmap_irq_get_virq()` and `regmap_irq_get_domain()` are used differently.
  - Explain how an MFD parent creates child platform devices.
  - Explain how child IRQ resources are translated through the parent's IRQ domain.
  - Explain when to use MFD versus a normal platform driver versus `simple-mfd`.
  - Explain what syscon is, why it is regmap-backed, and when it is a design smell.
  - Debug scenario: child driver gets no IRQ after parent probe succeeds.
  - Debug scenario: interrupt storm after enabling regmap IRQ.
  - Debug scenario: syscon lookup returns an error pointer.
  - Senior scenario: design a PMIC parent with regulators, RTC, GPIO, and power-key children while keeping register ownership and suspend/wakeup behavior sane.
