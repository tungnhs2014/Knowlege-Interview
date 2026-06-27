# Topic Brief - 16 - I2C Client Drivers

## Output Targets
- Knowledge: `knowledge/16-i2c-client-drivers.md`
- Interview: `interview/16-i2c-client-drivers.md`
- Example: `examples/16-i2c-client-drivers/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch07` | `docs/Linux Device Driver Development/Chapter 7-I2C Client Drivers.md` | read/mapped/covered/merged | Primary book source for I2C client architecture, `struct i2c_driver`, `struct i2c_client`, probe/remove lifecycle, private data, driver registration, plain I2C/SMBus APIs, board-file instantiation, DT client nodes, OF matching, and older `id_table` caveat. |
| `ldd1-ch05` | `docs/Linux Device Driver Development/Chapter 5-Platform Device Drivers.md` | read/mapped/related | Bus matching and module autoload context: every bus has device/driver matching, `MODULE_DEVICE_TABLE(i2c, ...)`, I2C devices are not platform-bus devices even though they are non-discoverable board devices. |
| `ldd1-ch06` | `docs/Linux Device Driver Development/Chapter 6-The Concept of a Device Tree .md` | read/mapped/related | DT addressing context for I2C/SPI child nodes, `#address-cells = <1>`, `#size-cells = <0>`, and `reg` meaning bus address rather than CPU MMIO address. |
| `ldd1-ch09` | `docs/Linux Device Driver Development/Chapter 9-Regmap API .md` | read/mapped/related | I2C register-access abstraction via `regmap_init_i2c()`, `struct regmap_config`, `regmap_read/write`, and why regmap is often better than open-coded I2C register helpers. |
| `ldd1-ch14` | `docs/Linux Device Driver Development/Chapter 14-Pin Control and GPIO Subsystem.md` | read/mapped/related | IRQ mapping note for I2C/SPI devices: DT IRQ mapping is made available through `i2c_client.irq` or `spi_device.irq`. |
| `ldd1-ch17` | `docs/Linux Device Driver Development/Chapter 17- Input Devices Drivers.md` | read/mapped/related | Applied I2C-client examples for input devices, `BUS_I2C`, and I2C/SMBus block reads. Kept as related framework context, not core I2C-client source. |
| `ldd1-ch20` | `docs/Linux Device Driver Development/Chapter 20-Regulator Framework.md` | read/mapped/related | Applied PMIC/regulator I2C-client example using `i2c_check_functionality()`, SMBus byte operations, `i2c_set_clientdata()`, and framework registration. Kept as related framework context. |
| `ldd2-ch01` | `docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md` | read/mapped/covered | Supporting concurrency/IRQ source for I2C clients: I2C access can sleep, thread-only IRQ handlers for I2C devices, `IRQF_ONESHOT`, `request_any_context_irq()`, and mutex locking when threaded work accesses I2C state. |
| `ldd2-ch02` | `docs/Linux Device Driver Development 2/Chapter 2-Regmap_API.md` | read/mapped/related | Updated regmap source: `devm_regmap_init_i2c()`, devm cleanup, regmap locking/abstraction, nested IRQ examples using `client->irq` for I2C GPIO expanders. |
| `ldd2-ch03` | `docs/Linux Device Driver Development 2/Chapter 3-MFD_Subsystem_and_Syscon_API.md` | read/mapped/related | I2C MFD case study: `devm_regmap_init_i2c()`, multi-address I2C chips, dummy secondary clients, per-client regmaps, and `i2c_unregister_device()` cleanup. |
| `ldd2-ch10` | `docs/Linux Device Driver Development 2/Chapter 10-Linux_Kernel_Power_Management.md` | read/mapped/related | Runtime PM case study for an I2C ambient-light sensor: `struct i2c_driver` with `dev_pm_ops`, runtime resume before register access, autosuspend, and remove-time PM cleanup. |
| `notion-ch07-part1` | `docs/Linux-Device-Driver-Notion/Chapter 7-Part 1 I2C Architecture and Driver Structures.md` | read/mapped/covered/merged | Notion duplicate/expansion of I2C architecture: protocol mental model, I2C core/adapter/client split, `i2c_driver`/`i2c_client`, `module_i2c_driver()`, probe tasks, `i2c_check_functionality()`, private data, remove, and complete template. |
| `notion-ch07-part2` | `docs/Linux-Device-Driver-Notion/Chapter 7-Part 2 I2C Communication APIs.md` | read/mapped/covered/merged | Notion communication source: plain I2C send/recv, `i2c_transfer()` with repeated START, `struct i2c_msg`, SMBus byte/word/block operations, return-value handling, 64 KiB I2C message limit, mutex protection, and LM75/EEPROM/MCP examples. |
| `notion-ch07-part3` | `docs/Linux-Device-Driver-Notion/Chapter 7-Part 3 Device Tree Integration.md` | read/mapped/covered/merged | Notion DT source: I2C child-node structure, address/size cells, `compatible`/`reg`, IRQ properties, OF match tables, `of_match_device()`, DT property parsing, `of_device_id.data`, backward compatibility, and checklists. |
| `notion-ch02-part2` | `docs/Linux-Device-Driver-Notion/Chapter 2-Part 2 Module Dependencies, Loading & Information.md` | read/mapped/related | Module alias context for I2C autoloading (`alias i2c:*`, `i2c-core`, `MODULE_ALIAS`). Related to module-loading topic; only small autoload note belongs here. |
| `notion-ch06-part1` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 1 Device Tree Fundamentals.md` | read/mapped/related | General DT examples that include I2C controller/device nodes and `dtbs_check`; main DT details should remain in topics 10/11, with only I2C child-node rules repeated here. |
| `notion-ch06-part3` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 3 OF APIs and Platform Integration.md` | read/mapped/related | General OF API examples, including an EEPROM I2C probe; use as related DT API context, not duplicate I2C-client core content. |
| `notion-ch15-part3` | `docs/Linux-Device-Driver-Notion/Chapter 15-Part 3 Advanced Features and Integration.md` | read/mapped/related | I2C GPIO-expander example with SMBus register access; belongs primarily to GPIO controller topic, but provides an applied I2C-client framework example. |
| `notion-ch16-part3` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 3 Advanced Topics.md` | read/mapped/related | I2C sensor threaded IRQ example and I2C sleep-in-IRQ warning; belongs primarily to interrupt topic, but supports I2C-client IRQ guidance. |

## Source Files Read
- `ldd1-ch07`: complete chapter; especially "The driver architecture", "The i2c_driver structure", "The probe() function", "Per-device data", "The remove() function", "Driver initialization and registration", "Driver and device provisioning", "Accessing the client", "Plain I2C communication", "I2C and device trees", "Defining and registering the I2C driver", "Remark", and "Putting it all together".
- `ldd1-ch05`: "Devices, drivers, and bus matching", `MODULE_DEVICE_TABLE`, "ID table matching", and the I2C `i2c_device_id` per-device-data example.
- `ldd1-ch06`: "Representing and addressing devices" and "SPI and I2C addressing".
- `ldd1-ch09`: "regmap initialization", "I2C initialization", "Device access functions", and "Putting it all together".
- `ldd1-ch14`: "GPIO mapping to IRQ in the device tree", especially the `i2c_client.irq` note.
- `ldd1-ch17`: "Allocating and registering an input device" I2C probe excerpt and BMA150 `i2c_smbus_read_i2c_block_data()` event-reporting excerpt.
- `ldd1-ch20`: "Case study - Intersil ISL6271A voltage regulator", including SMBus callbacks and I2C probe/framework-registration flow.
- `ldd2-ch01`: "Threaded IRQ handlers", "Requesting a context IRQ", and "Locking from within an interrupt handler".
- `ldd2-ch02`: `devm_regmap_init_i2c()` introduction, regmap access functions, nested IRQ examples around I2C clients, and `pcf857x_probe()`.
- `ldd2-ch03`: `da9055_i2c_probe()`, `devm_regmap_add_irq_chip()`, multi-address I2C MFD subdevices, dummy I2C client creation, per-subdevice regmaps, and dummy-client cleanup.
- `ldd2-ch10`: "Implementing runtime PM capability", I2C `bh1780_driver`, "Runtime PM anywhere in the driver", and the `bh1780_probe()`/read/remove excerpts.
- `notion-ch07-part1`: complete file.
- `notion-ch07-part2`: complete file.
- `notion-ch07-part3`: complete file.
- `notion-ch02-part2`: module dependency chain, `modules.alias`, `MODULE_ALIAS("i2c:*")`, `modinfo` aliases, and `i2c-core` dependency examples.
- `notion-ch06-part1`: node naming, I2C expander/controller examples, I2C child node example, and `dtbs_check`/binding validation note.
- `notion-ch06-part3`: EEPROM I2C probe with child partitions and `of_match_ptr()` reference.
- `notion-ch15-part3`: MCP23016 I2C GPIO-expander code sections with SMBus register helpers and I2C driver declaration.
- `notion-ch16-part3`: I2C sensor threaded IRQ example and "sleeping in hard IRQ context" warning.

## Merged Source Notes
- The core mental model is shared across `ldd1-ch07` and `notion-ch07-part1`: an I2C client driver manages a specific slave device, while the adapter/controller driver owns the physical bus. The client driver should use I2C core APIs and should not depend on the SoC-specific controller implementation.
- `struct i2c_driver` is the driver-side object: it contains probe/remove/shutdown callbacks, generic `struct device_driver`, match tables, and optional PM callbacks. `struct i2c_client` is the device instance: it carries address, adapter, embedded `struct device`, IRQ, name, and flags populated from firmware/board instantiation.
- Probe should validate bus functionality, optionally verify chip identity, allocate private data, initialize locks/state/register cache, save private data with `i2c_set_clientdata()`, configure the hardware, and register with the correct kernel subsystem such as GPIO, IIO, hwmon, input, regulator, RTC, MFD, or nvmem.
- Remove should undo framework registration and non-devm resources. Devm allocation simplifies memory/resource cleanup, but subsystem unregister ordering still matters.
- `i2c_set_clientdata()`/`i2c_get_clientdata()` are wrappers over device driver data and are the standard way to associate per-device state with an I2C client.
- Linux exposes two communication layers: plain I2C and SMBus. Plain I2C uses `i2c_master_send()`, `i2c_master_recv()`, and `i2c_transfer()`. SMBus uses helpers such as `i2c_smbus_read_byte_data()`, `i2c_smbus_write_word_data()`, and block helpers.
- `i2c_transfer()` is the key API for combined transfers with no STOP between messages, such as "write register address, repeated START, read data". Use `struct i2c_msg`, `I2C_M_RD`, `client->addr`, and `client->adapter`.
- Return-value checking matters: I2C send/recv return the number of bytes transferred or a negative errno; SMBus writes return 0 on success, reads return data or a negative errno, and block reads return the number of bytes read.
- `i2c_check_functionality()` should verify the adapter supports the operations the driver will use, such as `I2C_FUNC_I2C`, `I2C_FUNC_SMBUS_BYTE_DATA`, `I2C_FUNC_SMBUS_WORD_DATA`, or block operations.
- I2C devices are not hardware-enumerated like PCI/USB. They must be instantiated by firmware/DT/ACPI/board data or by another driver using I2C core creation APIs.
- In Device Tree, I2C devices are children of an I2C controller node. The parent bus uses `#address-cells = <1>` and `#size-cells = <0>`, and the child `reg` property is the I2C slave address, not a CPU MMIO address.
- OF matching uses `struct of_device_id`, `.compatible`, `MODULE_DEVICE_TABLE(of, ...)`, and `.driver.of_match_table`. Legacy/name matching uses `struct i2c_device_id`, `.id_table`, and `MODULE_DEVICE_TABLE(i2c, ...)`.
- `client->irq` is populated by the I2C/firmware path when the device has an interrupt described by board data/firmware. Because I2C transfers can sleep, IRQ work that talks to the chip should run in process/thread context, usually via `devm_request_threaded_irq()` with `IRQF_ONESHOT`.
- Regmap is a strong production pattern for register-oriented I2C devices. It centralizes register widths, access policy, caching, locking, and bus abstraction; current sources prefer `devm_regmap_init_i2c()` over unmanaged `regmap_init_i2c()` when lifetime matches the device.
- Runtime PM appears in applied ldd2 examples. An I2C client may need to runtime-resume before register access and mark itself idle afterward; PM callbacks can sleep and can use I2C messaging in the normal driver-model suspend/resume path.

## Source Differences
- `ldd1-ch07` is the compact original chapter; Notion chapter 7 is an expanded rewrite split into architecture, communication APIs, and DT integration. The Notion files should not be treated as duplicates because they add diagrams, API decision guidance, example drivers, DT checklists, property parsing, and better return-value guidance.
- `ldd1-ch07` and Notion examples use older I2C callback prototypes: `probe(struct i2c_client *, const struct i2c_device_id *)` and `int remove(struct i2c_client *)`. Current Linux 6.17 documentation shows `probe(struct i2c_client *client)` and `void remove(struct i2c_client *client)`. Learner-facing examples should be written against a chosen current kernel and mention this API drift.
- `ldd1-ch07` states that both `.id_table` and `.of_match_table` are needed for kernels around 4.10 or older. Current docs still show `id_table` for many examples and explain how to retrieve a matching ID with `i2c_match_id()`, but pure OF/fwnode-based matching may not need the old two-argument probe path. Treat this as version-sensitive.
- `ldd1-ch07` says "I2C devices are SMBus-compatible but not the reverse"; current kernel docs phrase the relationship more carefully: SMBus is a sibling/subset-style protocol, I2C controllers can support most SMBus operations, but SMBus controllers do not support all I2C protocol options. Avoid overclaiming universal compatibility.
- Notion part 2 recommends SMBus for "maximum compatibility"; current kernel docs similarly say to use SMBus-level communication when possible because all adapters understand SMBus-level commands while only some understand plain I2C. Still, devices requiring repeated START or vendor-specific combined transactions need `i2c_transfer()`.
- `ldd1-ch09` uses unmanaged `regmap_init_i2c()`/`regmap_exit()`; `ldd2-ch02` uses `devm_regmap_init_i2c()`. Prefer devm-managed regmap in modern examples unless there is a specific lifetime reason not to.
- `ldd2-ch03` uses `i2c_new_dummy()`, while modern kernels have newer naming such as `i2c_new_dummy_device()`/`devm_i2c_new_dummy_device()` in many codebases. Validate current helper names before writing final code examples.
- Several source code snippets are educational and omit important error paths, short-transfer checks, endian handling, register cache invalidation, PM integration, and full subsystem cleanup. The brief should feed production cautions into final docs.
- `ldd1-ch07` contains a probable typo in the DT example (`reg = <55>` instead of hexadecimal-style `<0x55>`). Use valid DTS examples in final learner docs.

## Gaps / Uncertainties
- Need target-kernel decision before writing learner-facing code examples because I2C callback prototypes changed across kernel versions.
- Internal sources do not deeply cover ACPI/software-node instantiation for I2C clients. Keep DT as the main embedded-Linux path and mention ACPI only as a firmware alternative if externally validated.
- Internal sources do not cover modern userspace I2C tooling and debugging in depth (`i2cdetect`, `i2cdump`, `i2cget`, `/sys/bus/i2c/devices`, dynamic debug, tracepoints). Needs external validation before final docs.
- Internal sources do not cover `i2c_new_client_device()`, `i2c_new_scanned_device()`, or current dummy-client helper names enough for production examples.
- Internal examples do not cover DMA-safe I2C helpers, PEC, ten-bit addressing, host notify, SMBus alert, or I2C slave mode. These are advanced/out-of-scope for the main beginner topic unless a device requires them.
- Learning-path boundaries:
  - Topic 10/11 owns general DT syntax and OF APIs.
  - Topic 15 owns generic IRQ theory and threaded IRQ internals.
  - Topic 18 owns regmap details.
  - Topic 19 owns regmap IRQ/MFD/syscon details.
  - Topic 24 owns runtime PM depth.

## External Validation
- Used: https://docs.kernel.org/6.17/i2c/writing-clients.html
  - Validates current I2C client-driver guidance, client/driver distinction, `i2c_set_clientdata()`/`i2c_get_clientdata()`, device binding/creation/detection cautions, `module_i2c_driver()`, PM/shutdown callbacks, plain I2C and SMBus APIs, return-value rules, and SMBus block buffer limit.
- Used: https://docs.kernel.org/6.17/i2c/instantiating-devices.html
  - Validates that I2C devices are not hardware-enumerated and must be instantiated explicitly, with DT child nodes as a standard embedded-system method.
- Used: https://docs.kernel.org/6.17/driver-api/i2c.html
  - Validates current `struct i2c_driver`, `struct i2c_client`, `struct i2c_board_info`, `i2c_master_send()/recv()` return and size rules, task-context caveat for I2C functions, and current field/prototype drift.
- Used: https://docs.kernel.org/6.17/i2c/smbus-protocol.html
  - Validates SMBus protocol transaction terminology and should be used when final docs need exact SMBus command diagrams.
- External validation still needed before final knowledge/example files:
  - Current in-tree drivers for simple I2C sensor/hwmon/IIO clients using the target kernel version.
  - Current helper names for creating secondary/dummy I2C clients.
  - Current trace/debug facilities under `trace/events/i2c` and dynamic debug practices for I2C client debugging.

## Learning Content Brief
- Topic scope: write Linux I2C client drivers for slave devices behind an existing I2C adapter/controller. Do not cover writing I2C adapter/controller drivers except as background.
- Related topics:
  - Topic 09: platform bus and bus/device/driver matching mental model.
  - Topic 10/11: DT syntax, bindings, OF property parsing.
  - Topic 15: interrupt handling.
  - Topic 18: regmap.
  - Topic 24: runtime/system power management.
  - Topic 25/26/27/28/35: applied I2C clients in IIO, input, RTC, watchdog/nvmem, and ASoC.
- Beginner mental model:
  - The I2C controller/adapter owns the wires and timing.
  - The I2C client is one addressed chip on that bus.
  - The I2C client driver owns the chip-specific protocol: registers, commands, delays, IRQ behavior, and subsystem registration.
  - Probe is not "scan the bus"; probe means the kernel has a matched `i2c_client` and asks the driver to bind to it.
- Core mechanism:
  - Firmware/board code describes or creates an I2C device with name/compatible, address, IRQ, and configuration.
  - I2C core creates a `struct i2c_client` and matches it with a `struct i2c_driver`.
  - The driver's probe callback initializes the chip and registers a kernel-facing interface.
  - Runtime code communicates with the chip through I2C/SMBus/regmap helpers.
  - Remove/shutdown/PM callbacks quiesce the hardware and undo registrations.
- Important structs/APIs:
  - Driver/model: `struct i2c_driver`, `struct i2c_client`, `struct i2c_adapter`, `struct i2c_device_id`, `struct of_device_id`, `struct i2c_board_info`, `module_i2c_driver()`, `i2c_add_driver()`, `i2c_del_driver()`, `MODULE_DEVICE_TABLE(i2c, ...)`, `MODULE_DEVICE_TABLE(of, ...)`.
  - Private data: `i2c_set_clientdata()`, `i2c_get_clientdata()`, `dev_set_drvdata()`, `dev_get_drvdata()`, `devm_kzalloc()`.
  - Capability check: `i2c_check_functionality()`, `I2C_FUNC_I2C`, `I2C_FUNC_SMBUS_BYTE_DATA`, `I2C_FUNC_SMBUS_WORD_DATA`, `I2C_FUNC_SMBUS_BLOCK_DATA`, `I2C_FUNC_SMBUS_I2C_BLOCK`.
  - Plain I2C: `i2c_master_send()`, `i2c_master_recv()`, `i2c_transfer()`, `struct i2c_msg`, `I2C_M_RD`.
  - SMBus: `i2c_smbus_read_byte()`, `i2c_smbus_write_byte()`, `i2c_smbus_read_byte_data()`, `i2c_smbus_write_byte_data()`, `i2c_smbus_read_word_data()`, `i2c_smbus_write_word_data()`, `i2c_smbus_read_block_data()`, `i2c_smbus_write_block_data()`, `i2c_smbus_read_i2c_block_data()`, `i2c_smbus_write_i2c_block_data()`.
  - DT/OF: `.driver.of_match_table`, `of_match_ptr()`, `of_match_device()`, `client->dev.of_node`, `of_property_read_u32()`, `of_property_read_bool()`, `of_property_read_string()`, `of_property_read_u32_array()`.
  - Regmap/PM/IRQ: `devm_regmap_init_i2c()`, `regmap_read()`, `regmap_write()`, `client->irq`, `devm_request_threaded_irq()`, `IRQF_ONESHOT`, `struct dev_pm_ops`, `pm_runtime_get_sync()`, `pm_runtime_put_autosuspend()`.
- Lifecycle/data flow:
  - Driver load: module init or `module_i2c_driver()` registers `i2c_driver` with I2C core.
  - Device creation: DT/ACPI/board data or another driver creates `i2c_client` with address/configuration.
  - Match: I2C core matches OF compatible or I2C ID/name and calls probe.
  - Probe: check functionality -> allocate state -> initialize mutex/cache -> set client data -> verify chip ID/register access -> configure hardware -> register subsystem device -> enable IRQ/PM if needed.
  - Normal operation: subsystem callbacks read/write registers via SMBus, plain I2C, or regmap; protect multi-step state updates with locks; runtime-resume first if PM can suspend the chip.
  - Remove/shutdown: unregister subsystem objects, stop IRQ/work, disable PM, put hardware in safe state, unregister secondary clients if any.
- Examples to produce later:
  - Learning-only I2C temperature sensor or EEPROM client with DT match, current probe/remove prototypes, `i2c_check_functionality()`, private data, and SMBus reads.
  - Combined-transfer EEPROM read example using `i2c_transfer()` and repeated START.
  - Optional regmap variant showing how the same register operations become `regmap_read()`/`regmap_write()`.
  - DTS snippet with an I2C controller enabled and one child node with `compatible`, `reg`, optional `interrupts`, supply, and custom properties.
  - Debug workflow using `/sys/bus/i2c/devices`, `dmesg`, dynamic debug, i2c-tools, and tracepoints after validation.
- Common bugs:
  - Treating probe as bus scanning rather than binding to an already-created `i2c_client`.
  - Forgetting `i2c_check_functionality()` and then using an API the adapter cannot support.
  - Not checking for short transfers from `i2c_master_send()`/`i2c_master_recv()`.
  - Misinterpreting SMBus read return values, where non-negative values are data and negative values are errors.
  - Using separate send/recv where the device requires a repeated START combined transfer.
  - Sleeping in hard IRQ context by performing I2C access outside a threaded handler/workqueue.
  - Missing `IRQF_ONESHOT` for thread-only IRQ handlers.
  - Using the wrong DT `reg` value or treating I2C `reg` as an MMIO base/size tuple.
  - Missing `MODULE_DEVICE_TABLE()` entries, preventing module autoload.
  - Forgetting endian conversion for word registers or datasheet-specific byte order.
  - Omitting lock protection around read-modify-write register sequences.
  - Copying stale callback prototypes into a newer kernel tree.
- Debugging notes:
  - Start with instantiation: is the child node present in DT, is the adapter enabled, and does `/sys/bus/i2c/devices` show the expected bus-address device?
  - Check probe logs with `dev_err()`/`dev_dbg()` and dynamic debug.
  - Validate the adapter functionality and address with i2c-tools only when safe for the hardware; some chips react badly to generic probing.
  - For communication failures, separate address/NACK problems, adapter capability problems, short transfers, register-endian mistakes, power/reset sequencing, and missing pull-ups/pinctrl.
  - For IRQ clients, confirm `client->irq`, DT interrupt properties, `/proc/interrupts`, trigger type, and that the handler does not access I2C in hard IRQ context.
  - For PM-related failures, confirm the device is resumed before register access and that runtime PM usage counts are balanced.
- Production concerns:
  - Prefer DT bindings over board files for embedded platforms and treat binding properties as ABI.
  - Prefer `devm_*` helpers where lifetime is tied to `client->dev`, but keep explicit cleanup for subsystem objects, runtime PM, workqueues, and secondary clients.
  - Prefer regmap for register-heavy devices unless the protocol is too unusual.
  - Do not expose raw I2C register poking as a driver ABI when an existing subsystem ABI fits.
  - Use device-specific delays, reset GPIOs, regulators, clocks, and PM ordering from the datasheet.
  - Avoid generic I2C device detection in new embedded drivers unless the hardware has reliable identification and the subsystem requires it.
  - Write examples against a declared kernel version because I2C callback prototypes and helper names change.
- Interview angles:
  - Explain `i2c_adapter` vs `i2c_client` vs `i2c_driver`.
  - Explain why I2C devices need firmware/board instantiation.
  - Explain what probe should and should not do.
  - Compare SMBus helpers, `i2c_master_send/recv()`, and `i2c_transfer()`.
  - Explain why repeated START matters for register reads.
  - Explain `i2c_check_functionality()` and when to use `I2C_FUNC_I2C` versus SMBus flags.
  - Explain `i2c_set_clientdata()` and lifetime/ownership of private data.
  - Explain how DT describes I2C child devices and why `#size-cells = <0>`.
  - Explain why I2C access must not run in hard IRQ context.
  - Explain when regmap improves an I2C client driver.
  - Identify stale I2C callback prototypes and how to validate against the target kernel.
