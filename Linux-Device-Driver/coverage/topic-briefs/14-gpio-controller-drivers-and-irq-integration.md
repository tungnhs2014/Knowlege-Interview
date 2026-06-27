# Topic Brief - 14 - GPIO Controller Drivers And IRQ Integration

## Output Targets
- Knowledge: `knowledge/14-gpio-controller-drivers-and-irq-integration.md`
- Interview: `interview/14-gpio-controller-drivers-and-irq-integration.md`
- Example: `examples/14-gpio-controller-drivers-and-irq-integration/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch14` | `docs/Linux Device Driver Development/Chapter 14-Pin Control and GPIO Subsystem.md` | read/mapped/covered | Consumer-side context for GPIO-to-IRQ mapping, `gpio_to_irq()`, `gpiod_to_irq()`, `gpio_cansleep()`, DT `interrupt-parent`/`interrupts`, and why GPIO controller callbacks sit below consumer APIs. |
| `ldd1-ch15` | `docs/Linux Device Driver Development/Chapter 15-GPIO Controller Drivers.md` | read/mapped/covered | Primary GPIO controller source: `struct gpio_chip`, required callbacks, `can_sleep`, dynamic base allocation, `to_irq`, controller DT properties, MCP23016 GPIO expander shape. |
| `ldd1-ch16` | `docs/Linux Device Driver Development/Chapter 16-Advanced IRQ Management.md` | read/mapped/covered/merged | Primary IRQ integration source: `struct irq_chip`, `struct irq_domain`, hwirq-to-virq mapping, legacy manual GPIO IRQ domain flow, chained vs nested interrupts, gpiolib irqchip helpers, DT interrupt-controller binding. |
| `ldd2-ch01` | `docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md` | read/mapped/covered | Supporting IRQ fundamentals: hard IRQ constraints, threaded IRQs, `IRQF_ONESHOT`, `request_any_context_irq()`, locking choices for IRQ/thread contexts, `/proc/interrupts` and `/proc/irq` debugging clues. |
| `ldd2-ch02` | `docs/Linux Device Driver Development 2/Chapter 2-Regmap_API.md` | read/mapped/covered/merged | Deeper and newer duplicate/extension of `ldd1-ch16`: IRQ domain details, chained/nested GPIO irqchip API, `gpiochip_irqchip_add[_nested]()`, `gpiochip_set_[chained|nested]_irqchip()`, regmap IRQ API and max77620 GPIO IRQ example. |
| `ldd2-ch03` | `docs/Linux Device Driver Development 2/Chapter 3-MFD_Subsystem_and_Syscon_API.md` | read/mapped/related | Related MFD/regmap IRQ coverage: parent MFD IRQ domain passed to child platform devices, `devm_regmap_add_irq_chip()`, `regmap_irq_get_domain()`, and GPIO subdevice/regmap context. |
| `notion-ch14-part2` | `docs/Linux-Device-Driver-Notion/Chapter 14-Part 2 GPIO Consumer Interfaces.md` | read/mapped/related | Notion consumer-side duplicate/context: descriptor API, `gpiod_to_irq()`, cansleep variants, debounce, polarity, and resource-managed consumer code. |
| `notion-ch14-part3` | `docs/Linux-Device-Driver-Notion/Chapter 14-Part 3 Userspace GPIO Access.md` | read/mapped/related | Userspace/debug context for GPIO chips: sysfs legacy export functions and modern `/dev/gpiochipN`/libgpiod inspection tools. |
| `notion-ch15-part1` | `docs/Linux-Device-Driver-Notion/Chapter 15-Part 1 GPIO Controller Architecture.md` | read/mapped/covered/merged | Expanded GPIO controller architecture: controller responsibilities, `gpio_chip` field explanations, offset vs global number, registration with `devm_gpiochip_add_data()`, private data access, callback implementation, MCP23016 example. |
| `notion-ch15-part2` | `docs/Linux-Device-Driver-Notion/Chapter 15-Part 2 IRQ Chip Integration.md` | read/mapped/covered/merged | Expanded IRQ integration: GPIO controllers as interrupt controllers, IRQ multiplexing mental model, manual IRQ domain setup, chained vs nested comparison, modern gpiolib irqchip helper examples, custom `irq_chip` callbacks. |
| `notion-ch15-part3` | `docs/Linux-Device-Driver-Notion/Chapter 15-Part 3 Advanced Features and Integration.md` | read/mapped/covered/merged | DT integration and best practices: required `gpio-controller`/`#gpio-cells`, optional `interrupt-controller`/`#interrupt-cells`, complete MCP23016 driver, mutex-protected register cache, sysfs chip attributes. |
| `notion-ch16-part1` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 1 IRQ Architecture and Propagation.md` | read/mapped/related | Supporting IRQ-domain deep dive: why domains avoid hwirq collisions, `irq_desc`, `irq_chip`, `irq_domain_ops`, mapping creation/finding, `irq_domain_add_linear()`/tree/legacy/simple. |
| `notion-ch16-part2` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 2 IRQ Multiplexing - Chained and Nested Inter.md` | read/mapped/covered/merged | Detailed chained/nested implementation examples, including SoC GPIO chained IRQ flow, `chained_irq_enter()/exit()`, `generic_handle_irq()`, raw spinlocks for MMIO GPIO, and nested threaded I2C expander flow. |
| `notion-ch16-part3` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 3 Advanced Topics.md` | read/mapped/related | Related advanced IRQ source: regmap IRQ overview, DT interrupt binding examples, IRQ type flags, driver-side IRQ parsing APIs, common IRQ pitfalls, approach-selection decision tree. |

## Source Files Read
- `ldd1-ch14`: sections "The GPIO subsystem", "In atomic context", "In a non-atomic context (that may sleep)", "GPIOs mapped to IRQ", and "GPIO mapping to IRQ in the device tree".
- `ldd1-ch15`: complete chapter; especially "Driver architecture and data structures", "Pin controller guidelines", "Sysfs interface for GPIO controller", and "GPIO controllers and the DT".
- `ldd1-ch16`: sections "Advanced peripheral IRQ management", IRQ domain/`irq_chip` structures, "Interrupt request and propagation", "Chained interrupts", "Nested interrupts", "A case study - the GPIO and IRQ chip", "The legacy GPIO and IRQ chip", "The new gpiolib irqchip API", and "The interrupt controller and DT".
- `ldd2-ch01`: sections "Linux kernel interrupt management", "Threaded IRQ handlers", "Requesting a context IRQ", and "Locking from within an interrupt handler".
- `ldd2-ch02`: sections "Regmap and IRQ management" through "Nested gpiochip-based irqchips", plus "Regmap IRQ API and data structures", "Regmap IRQ API", and "Regmap IRQ API example".
- `ldd2-ch03`: sections around MFD IRQ domain handling, `devm_regmap_add_irq_chip()`, `devm_mfd_add_devices()`, and subdevice GPIO/regmap context.
- `notion-ch14-part2`: GPIO subsystem overview, context-sensitive value access, `gpiod_to_irq()`, resource-managed GPIO consumer examples, and polarity handling.
- `notion-ch14-part3`: kernel GPIO export functions, libgpiod overview, `/dev/gpiochipN`, `gpiodetect`, and `gpioinfo`.
- `notion-ch15-part1`: complete file.
- `notion-ch15-part2`: complete file.
- `notion-ch15-part3`: complete file.
- `notion-ch16-part1`: "IRQ Domain API Deep Dive".
- `notion-ch16-part2`: complete file.
- `notion-ch16-part3`: "Regmap IRQ API", "Device Tree IRQ Binding", and "Best Practices".

## Merged Source Notes
- The mental model should start with `gpio_chip`: a GPIO controller driver exports a bank of lines to gpiolib, and consumers never call hardware-specific code directly. Gpiolib translates consumer operations into controller callbacks using a chip-local offset.
- The minimum controller implementation is direction control plus value get/set; robust implementations also provide `get_direction`, `set_multiple`, `set_config` or debounce support when hardware supports it, line names, dynamic base allocation (`base = -1`), and private data retrieval through `gpiochip_get_data()` or `container_of()`.
- `can_sleep` is central. MMIO/SoC GPIO controllers should use non-sleeping register access and can participate in chained hard-IRQ flows. I2C/SPI expanders set `can_sleep = true`; their GPIO value and IRQ status access must happen in sleepable/thread context.
- `to_irq` bridges GPIO offsets to Linux IRQ numbers. In manual implementations it returns `irq_create_mapping(domain, offset)` or a previously created `irq_find_mapping()`. In gpiolib irqchip-helper implementations the framework supplies this wiring.
- GPIO IRQ integration is interrupt multiplexing: a controller may expose many child IRQs but signal one parent IRQ line. The parent handler reads a status register, maps each active GPIO hwirq/offset to a virq, and dispatches the child IRQ.
- Chained IRQ flow is for fast non-sleeping parent handlers. Register the parent handler with `irq_set_chained_handler_and_data()` or gpiolib chained helpers, use `chained_irq_enter()/chained_irq_exit()` around parent handling, and call `generic_handle_irq()` on each active child virq.
- Nested IRQ flow is for bus-based controllers. Register the parent IRQ with `devm_request_threaded_irq()` and `IRQF_ONESHOT`; the threaded parent handler can read I2C/SPI status registers and then call `handle_nested_irq()` for active child virqs.
- `irq_domain` maps local hwirq numbers to global Linux IRQs. For GPIO irqchips, the hwirq is normally the GPIO line offset. Linear domains fit small fixed banks; tree domains are for sparse/large hwirq spaces.
- `irq_domain_ops.map()` configures each mapped virq by setting chip data, `irq_chip`, flow handler, nested-thread flags if needed, and probe/threading restrictions. `irq_domain_ops.xlate()` decodes DT interrupt specifiers; two-cell GPIO interrupt specifiers are common: `<hwirq flags>`.
- `irq_chip` represents interrupt-controller operations such as mask, unmask, ack, set type, set wake, and bus lock/sync unlock. Simple/dumb controllers may use `dummy_irq_chip`; richer controllers should implement custom callbacks.
- Device Tree controller nodes need `gpio-controller` and `#gpio-cells`; if the GPIO bank also provides IRQs, add `interrupt-controller` and `#interrupt-cells`. Parent interrupt wiring is described with `interrupt-parent` and `interrupts`; child devices can use the GPIO controller as their `interrupt-parent`.
- Regmap IRQ is related but should be a supporting sidebar for this topic. It is especially useful when GPIO/PMIC/MFD interrupt status, mask, ack, wake, and type registers match regmap's generic model. It creates an IRQ domain, requests a threaded parent handler, and exposes child virqs with `regmap_irq_get_virq()`.
- Debug coverage should connect kernel and userspace views: `/sys/kernel/debug/gpio`, `/sys/class/gpio/gpiochipX` legacy chip attributes, `/proc/interrupts`, `/proc/irq/<irq>/`, dynamic debug/dev_dbg, tracepoints/ftrace where available, and libgpiod tools such as `gpiodetect` and `gpioinfo`.

## Source Differences
- `ldd1-ch15` and early Notion examples show older `struct gpio_chip` fields such as direct `irqchip`/`irqdomain`; current kernel documentation exposes IRQ integration mainly through `struct gpio_irq_chip` embedded as `gpio_chip.irq`. Learner-facing docs should mention the historical names only as source/API drift and teach current style.
- `ldd1-ch16` describes `gpiochip_irqchip_add()` and `gpiochip_set_chained_irqchip()` as the reduced modern API, then notes nested helpers added since Linux v4.10. Current kernel docs say the preferred approach is to populate `gpio_chip.irq` before `gpiochip_add_data()`/`devm_gpiochip_add_data()` so gpiolib sets up the irqchip with the GPIO chip registration.
- Several sources use `gpiochip->irq_domain`, `gpiochip->irqdomain`, and `gpio_chip->irq.domain` interchangeably. Treat this as API-version terminology drift; do not copy stale field names into final code examples without checking current headers.
- `notion-ch15-part2` comparison table says chained flow uses `IRQF_ONESHOT`; this is misleading for parent chained interrupt controllers, because chained parent handlers are installed by `irq_set_chained_handler_and_data()`/gpiolib chained helpers, not requested through `request_threaded_irq()`. `IRQF_ONESHOT` matters for threaded/nested parent handlers.
- `notion-ch16-part1` shows `irq_set_chip_and_handler(..., handle_nested_irq)` as a possible flow-handler style. The stronger and repeated source pattern for nested GPIO irqchips is to set the child IRQ as nested with `irq_set_nested_thread()`/gpiolib nested helpers, request the parent threaded IRQ, and call `handle_nested_irq()` from that parent thread.
- Some Notion code examples are learner sketches and omit full error handling, valid masks, current immutable irqchip helper macros, IRQ domain removal ordering, and precise register definitions. Use them for explanation, not production-ready code.
- `ldd1-ch14`/`notion-ch14-part3` still discuss sysfs GPIO access. Modern learner docs should mark sysfs GPIO as legacy/deprecated and point to the GPIO character device/libgpiod for userspace inspection.
- `ldd2-ch02` contains the most complete explanation of regmap IRQ but belongs primarily to learning-path topics 18/19. For topic 14, include only enough to recognize when GPIO IRQ support is better delegated to regmap IRQ.

## Gaps / Uncertainties
- Need current-code validation before writing final examples because gpiolib irqchip helper APIs evolved from direct `gpiochip_irqchip_add[_nested]()` toward `struct gpio_irq_chip` setup before chip registration.
- Internal sources do not cover hierarchical IRQ domains for modern GPIO irqchips in depth; mention as advanced/current-kernel context only unless validated against current docs and in-tree drivers.
- Internal sources do not cover `valid_mask`/`need_valid_mask` well, but current docs say they matter when only some GPIO lines can generate IRQs.
- Internal sources do not cover immutable `irq_chip`/`GPIOCHIP_IRQ_RESOURCE_HELPERS` expectations in current kernels; this needs validation before production-quality code is written.
- Device-specific examples such as MCP23016 are simplified. Real datasheet behavior for interrupt status/clear/type registers must be checked before creating a runnable driver example.
- Learning-path boundary: generic interrupt request, top/bottom halves, threaded IRQ internals, and broader IRQ domains belong mainly to topic 15; regmap IRQ and MFD/syscon belong mainly to topics 18/19.

## External Validation
- Used: https://docs.kernel.org/driver-api/gpio/driver.html
  - Validates current `gpio_chip` controller responsibilities, `devm_gpiochip_add_data()`, gpiolib irqchip infrastructure, `struct gpio_irq_chip`, valid masks for lines without IRQ support, and the current preferred setup pattern.
- Used: https://docs.kernel.org/core-api/irq/irq-domain.html
  - Validates current irq-domain purpose, hwirq-to-IRQ mapping, GPIO controller irqchip motivation, and hierarchical irq-domain context.
- External validation still needed before final code/examples:
  - Current in-tree GPIO controller examples for both MMIO chained and I2C/SPI nested designs.
  - Current header prototypes from the target kernel version for `struct gpio_chip`, `struct gpio_irq_chip`, and gpiolib irqchip helper functions.
  - Current GPIO userspace ABI docs if a learner-facing debugging section mentions sysfs vs character-device behavior.

## Learning Content Brief
- Topic scope: write a GPIO controller driver and integrate its GPIO lines with the Linux IRQ subsystem when the controller can also act as an interrupt controller.
- Related topics:
  - Topic 13: pinctrl and GPIO consumer APIs.
  - Topic 15: generic interrupt management, request/free IRQ, threaded IRQs, and IRQ-domain internals.
  - Topic 18/19: regmap and regmap IRQ for register-map-based interrupt controllers.
- Beginner mental model:
  - A GPIO controller is the provider of GPIO lines.
  - `gpio_chip` is the provider object registered with gpiolib.
  - Consumers ask gpiolib for lines; gpiolib calls the controller's callbacks using chip-local offsets.
  - If GPIO lines can generate interrupts, the same hardware also behaves like a small interrupt controller.
  - An IRQ domain is the translation table from local GPIO line numbers to Linux IRQ numbers.
- Core mechanism:
  - Probe allocates private data, initializes hardware/register cache/locks, fills `struct gpio_chip`, registers it with gpiolib, then optionally wires IRQ support.
  - For IRQ-capable controllers, each GPIO line that can interrupt is treated as a child IRQ. The parent IRQ handler demultiplexes the hardware status register and dispatches child IRQs.
  - MMIO controllers use chained handling and `generic_handle_irq()`.
  - Sleepable bus controllers use threaded/nested handling and `handle_nested_irq()`.
- Important structs/APIs:
  - GPIO: `struct gpio_chip`, `struct gpio_desc`, `gpiochip_add_data()`, `devm_gpiochip_add_data()`, `gpiochip_get_data()`, `gpiochip_request_own_desc()`, `gpiochip_free_own_desc()`.
  - GPIO callbacks: `.request`, `.free`, `.get_direction`, `.direction_input`, `.direction_output`, `.get`, `.set`, `.set_multiple`, `.set_config`, `.set_debounce`, `.to_irq`.
  - IRQ: `struct irq_chip`, `struct irq_domain`, `struct irq_domain_ops`, `struct irq_data`, `struct irq_desc`, `irq_domain_add_linear()`, `irq_create_mapping()`, `irq_find_mapping()`, `irq_set_chip_data()`, `irq_set_chip_and_handler()`, `irq_set_nested_thread()`.
  - Dispatch/setup: `generic_handle_irq()`, `handle_nested_irq()`, `irq_set_chained_handler_and_data()`, `devm_request_threaded_irq()`, `IRQF_ONESHOT`, `chained_irq_enter()`, `chained_irq_exit()`.
  - Gpiolib irqchip: historical helpers `gpiochip_irqchip_add[_nested]()` and `gpiochip_set_[chained|nested]_irqchip()`; current docs prefer filling `gpio_chip.irq`/`struct gpio_irq_chip` before registration.
  - DT: `gpio-controller`, `#gpio-cells`, `interrupt-controller`, `#interrupt-cells`, `interrupt-parent`, `interrupts`, `GPIO_ACTIVE_*`, `IRQ_TYPE_*`.
- Lifecycle/data flow:
  - GPIO-only: allocate private data -> initialize locks/register cache -> fill `gpio_chip` -> `devm_gpiochip_add_data()` -> consumers can request lines -> callbacks access hardware by offset -> devm cleanup on detach.
  - Manual IRQ-capable nested expander: fill `gpio_chip.to_irq` -> create `irq_domain` -> map child hwirqs lazily or during probe -> request threaded parent IRQ -> parent thread reads status -> `irq_find_mapping()` -> `handle_nested_irq()`.
  - Manual IRQ-capable chained SoC GPIO: create domain and mappings -> set child flow handlers/chip data -> install parent chained handler -> parent hard IRQ reads MMIO status -> `generic_handle_irq()` -> clear/ack status.
  - Current gpiolib irqchip style: configure `gpio_chip.irq` with irqchip, parent info, handler/default type/threading information before registering the chip, letting gpiolib set up the IRQ domain and mapping.
- Examples to produce later:
  - Learning-only MMIO GPIO controller with chained IRQ and a small register block.
  - Learning-only I2C GPIO expander with nested IRQ, mutex-protected register cache, and DT node.
  - DTS snippets showing the controller as both GPIO provider and interrupt provider, plus consumers using `gpios` and `interrupts`.
  - Debug commands using `/sys/kernel/debug/gpio`, `gpioinfo`, `/proc/interrupts`, and dynamic debug.
- Common bugs:
  - Sleeping in chained/hard IRQ context by calling I2C/SPI/regmap bus operations.
  - Forgetting `IRQF_ONESHOT` for thread-only parent IRQ handlers.
  - Not clearing/acking hardware interrupt status, causing interrupt storms.
  - Using global GPIO numbers inside `gpio_chip` callbacks instead of offsets.
  - Setting `can_sleep` incorrectly, causing consumers to call non-sleeping accessors on sleepable GPIOs.
  - Failing to protect read-modify-write register sequences with mutexes for bus expanders or raw spinlocks for MMIO controllers.
  - Assuming every GPIO line can generate IRQs; current gpiolib supports valid masks for partial IRQ-capable banks.
  - Mixing stale direct `irqdomain` fields with current `gpio_chip.irq` APIs.
  - Registering a chained parent IRQ with `request_irq()` instead of chaining helpers, or using chaining helpers for a sleepable nested controller.
- Debugging notes:
  - Confirm the chip registered and line ownership with `/sys/kernel/debug/gpio` and `gpioinfo`.
  - Confirm chip attributes through legacy `/sys/class/gpio/gpiochipX/base`, `label`, and `ngpio` when available.
  - Confirm parent/child interrupt counters in `/proc/interrupts`.
  - Use dev_dbg/dynamic debug around parent handler status reads, mapping lookup, mask/unmask/ack/type callbacks, and error paths.
  - For interrupt storms, check trigger type, status clear order, mask/unmask callbacks, level-vs-edge configuration, and shared parent IRQ flags.
  - For missing interrupts, check DT `interrupt-parent`, `#interrupt-cells`, pinctrl state, GPIO direction/input mode, mask registers, and whether the child mapping exists.
- Production concerns:
  - Prefer dynamic GPIO base allocation and line names.
  - Prefer devm-managed registration where lifetime fits, but ensure IRQ domain/helper cleanup semantics are understood for the target kernel.
  - Implement custom `irq_chip` callbacks when hardware supports masking, unmasking, acking, type selection, or wakeup.
  - Use `set_config`/pinctrl integration for bias, drive strength, open drain/source, and debounce when appropriate.
  - Treat DT bindings as ABI: document provider cells, flags, interrupt support, and line names carefully.
  - Validate current kernel API before publishing code because gpiolib IRQ setup changed across kernel versions.
- Interview angles:
  - Explain provider vs consumer: `gpio_chip` exposes lines; `gpiod_get()` consumes them.
  - Explain why callbacks receive offsets, not global GPIO numbers.
  - Explain `can_sleep` and how it drives chained vs nested IRQ choices.
  - Explain hwirq vs virq and why GPIO controllers need IRQ domains.
  - Compare `generic_handle_irq()` and `handle_nested_irq()`.
  - Explain why `IRQF_ONESHOT` is needed for thread-only IRQ handlers.
  - Explain how DT describes a GPIO controller that is also an interrupt controller.
  - Discuss how to debug an interrupt storm or a GPIO interrupt that never fires.
  - Identify stale GPIO APIs and current gpiolib irqchip setup patterns.
