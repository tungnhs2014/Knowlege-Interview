# Topic Brief - 17 - SPI Device Drivers

## Output Targets
- Knowledge: `knowledge/17-spi-device-drivers.md`
- Interview: `interview/17-spi-device-drivers.md`
- Example: `examples/17-spi-device-drivers/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch08` | `docs/Linux Device Driver Development/Chapter 8-SPI Device Drivers.md` | read/mapped/covered/merged | Primary SPI chapter: SPI signal model, `struct spi_device`, `struct spi_driver`, probe/remove, `spi_setup()`, driver registration, `spi_device_id`, board-file provisioning, DT child nodes, OF matching, `spi_transfer`, `spi_message`, `spi_sync()`, `spi_async()`, helper APIs, and `spidev`. |
| `ldd1-ch05` | `docs/Linux Device Driver Development/Chapter 5-Platform Device Drivers.md` | read/mapped/related | Bus and matching context: SPI devices are non-discoverable but should use the SPI bus, not the platform bus; `module_spi_driver()`, `MODULE_DEVICE_TABLE(spi, ...)`, `MODULE_DEVICE_TABLE(of, ...)`, and module autoloading through `modules.alias`. |
| `ldd1-ch06` | `docs/Linux Device Driver Development/Chapter 6-The Concept of a Device Tree .md` | read/mapped/related | General DT context for labels, phandles, DTS/DTB, node naming, and address cells. SPI-specific DT details are covered mainly by `ldd1-ch08` and Notion part 3. |
| `ldd1-ch09` | `docs/Linux Device Driver Development/Chapter 9-Regmap API .md` | read/mapped/related | Follow-on abstraction for register-oriented SPI/I2C chips: `regmap_init_spi()`, `struct regmap_config`, `regmap_read()`, `regmap_write()`, `regmap_update_bits()`, cache/access policy. Belongs primarily to topic 18. |
| `ldd2-ch02` | `docs/Linux Device Driver Development 2/Chapter 2-Regmap_API.md` | read/mapped/related | Updated regmap source: `devm_regmap_init_spi()`, sleepable-bus locking caveats, `read_flag_mask`/`write_flag_mask`, cache/access tables, and why regmap hides SPI/I2C/MMIO differences. |
| `ldd2-ch07` | `docs/Linux Device Driver Development 2/Chapter 7-Demystifying V4L2 and Video Capture Device Drivers.md` | read/mapped/related | Applied SPI-client context for V4L2 sensors/subdevices: `V4L2_SUBDEV_FL_IS_SPI`, `v4l2_spi_subdev_init()`, and bus-specific private-data links. Belongs mainly to V4L2 topics 32-34. |
| `ldd2-ch14` | `docs/Linux Device Driver Development 2/Chapter 14-Linux_Kernel_Debugging_Tips.md` | read/mapped/related | Debugging context: ftrace event directories include `spi`, supporting later SPI trace/debug workflow validation. |
| `notion-ch08-part1` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 1 SPI Architecture and Driver Structures.md` | read/mapped/covered/merged | Expanded SPI architecture source: signal synonyms, topology, SPI vs I2C comparison, Linux SPI core/controller/device layers, `spi_device`, SPI modes and flags, `spi_driver`, `module_spi_driver()`, probe responsibilities, `spi_setup()`, private data, remove, `spi_get_device_id()`, and complete driver template. |
| `notion-ch08-part2` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 2 SPI Transfer Mechanisms and Communication APIs.md` | read/mapped/covered/merged | Expanded transfer source: queued message model, `spi_transfer` fields, buffer/DMA rules, `spi_message` lifecycle, `spi_sync()`, `spi_async()`, helper functions, SPI flash command examples, locking, wait-ready loops, and common command/address/data transfer patterns. |
| `notion-ch08-part3` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 3 Device Tree Integration and Userspace Inter.md` | read/mapped/covered/merged | Expanded DT/userspace source: SPI child-node addressing, `spi-max-frequency`, mode flags, real DTS examples, OF matching, DT property parsing, platform-data fallback, `spidev`, ioctl commands, userspace read/write and `SPI_IOC_MESSAGE(N)`, and driver checklist. |
| `notion-ch08-part1-v4l2` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 1.md` | inspected/mapped/out-of-scope | Same raw Notion chapter number, but headings show V4L2 async/fwnode material, not SPI. Do not merge into topic 17 except to note numbering collision. |
| `notion-ch08-part2-v4l2` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 2.md` | inspected/mapped/out-of-scope | Same raw Notion chapter number, but headings show media-controller/V4L2 material, not SPI. Do not merge into topic 17 except to note numbering collision. |

## Source Files Read
- `ldd1-ch08`: complete chapter; especially "The driver architecture", "The device structure", "spi_driver structure", "The probe() function", "Per-device data", "The remove() function", "Driver initialization and registration", "Driver and device provisioning", "SPI and device tree", "Instantiate SPI devices in device tree", "Define and register SPI driver", "Accessing and talking to the client", "Putting it all together", "SPI user mode driver", and "With IOCTL".
- `ldd1-ch05`: platform/SPI bus distinction, bus-specific driver-registration macros, `MODULE_DEVICE_TABLE`, OF/device-ID matching, and generic `struct device_driver` matching context.
- `ldd1-ch06`: DT mechanism, node naming, labels/phandles, DTS/DTB basics, and general address-cell context.
- `ldd1-ch09`: regmap motivation, `struct regmap_config`, SPI regmap initialization, and `regmap_read/write/update_bits()`.
- `ldd2-ch02`: introduction through regmap initialization and access rules, including `devm_regmap_init_spi()` and slow-bus locking caveats.
- `ldd2-ch07`: V4L2 subdevice structure flags and `v4l2_spi_subdev_init()` section.
- `ldd2-ch14`: ftrace event listing showing `spi` trace event directory.
- `notion-ch08-part1`: complete file.
- `notion-ch08-part2`: complete file.
- `notion-ch08-part3`: complete file.
- `notion-ch08-part1-v4l2`: headings inspected; content is V4L2 async/fwnode.
- `notion-ch08-part2-v4l2`: headings inspected; content is media controller/V4L2.

## Merged Source Notes
- The core mental model is shared by `ldd1-ch08` and all three SPI Notion parts: a SPI device driver, also called a protocol/client driver in current kernel docs, manages one SPI peripheral behind an already-existing SPI controller. It does not drive the SoC SPI controller registers directly.
- SPI is synchronous and full-duplex at the wire level. The controller drives SCK and chip select; every transmitted bit on MOSI is paired with a received bit on MISO. "Read-only" SPI still clocks dummy data out, and "write-only" transfers may ignore received data.
- Signal names vary by vendor. MOSI may appear as SIMO/SDI/DI/SDA, MISO as SOMI/SDO/DO/SDA, SCK as CLK/SCL, and chip select as SS/CSx/EN/ENB. This should be included early because datasheets often use alternate names.
- Linux SPI has three practical layers for this topic: SPI core, SPI controller driver, and SPI device/protocol driver. The controller driver owns timing, chip-select control, FIFO/DMA, and hardware registers. The device driver owns chip-specific commands, registers, delays, IRQ behavior, and subsystem registration.
- `struct spi_device` represents the bound peripheral instance. Important fields for a device driver include embedded `struct device dev`, controller/master pointer, `max_speed_hz`, `chip_select`, `bits_per_word`, `mode`, and `irq`.
- `struct spi_driver` is the driver-side object. It contains `probe`, `remove`, optional `shutdown`, `id_table`, and embedded `.driver` with `.name` and `.of_match_table`.
- Probe should configure SPI mode/speed/word size as required, call `spi_setup()`, allocate and initialize private state, save it with `spi_set_drvdata()`, optionally verify communication by reading an ID/status register, parse DT or platform data, then register with the appropriate subsystem such as IIO, input, RTC, CAN, MTD/SPI-NOR, GPIO, V4L2, ASoC, or regmap-backed framework code.
- Remove should undo probe's framework registrations and non-devm resources. Devm memory helps but does not eliminate ordering requirements for IRQs, workqueues, runtime PM, subsystem objects, and outstanding async transfers.
- `spi_set_drvdata()` and `spi_get_drvdata()` are the SPI equivalents of device driver-data helpers and are wrappers around `dev_set_drvdata()`/`dev_get_drvdata()`.
- Device matching uses `struct spi_device_id` with `MODULE_DEVICE_TABLE(spi, ...)` for legacy/name matching, and `struct of_device_id` with `MODULE_DEVICE_TABLE(of, ...)` for DT matching/autoloading. `spi_get_device_id()` can retrieve the matched legacy ID when that path is used.
- SPI devices are non-discoverable. They must be provisioned by DT/ACPI/board information or created by another driver. A driver should not treat probe as bus scanning.
- In DT, SPI child nodes live under an enabled SPI controller node. The parent uses `#address-cells = <1>` and `#size-cells = <0>`. A child node's `reg = <N>` is the chip-select index, not a CPU MMIO address.
- Common SPI DT properties include `spi-max-frequency`, `spi-cpol`, `spi-cpha`, `spi-cs-high`, `spi-lsb-first`, `spi-3wire`, `spi-tx-bus-width`, and `spi-rx-bus-width`, plus device-specific supplies, clocks, reset GPIOs, and interrupts.
- `spi_setup()` applies `spi_device` bus settings such as mode, speed, and bits per word. Unlike I2C clients, SPI device drivers can adjust these parameters, and transfers can override speed/word size per transfer.
- SPI modes are combinations of CPOL and CPHA: `SPI_MODE_0`, `SPI_MODE_1`, `SPI_MODE_2`, and `SPI_MODE_3`. Additional flags include `SPI_CS_HIGH`, `SPI_LSB_FIRST`, `SPI_3WIRE`, `SPI_LOOP`, `SPI_NO_CS`, and `SPI_READY`.
- The SPI I/O model is message-based. A `struct spi_message` contains one or more `struct spi_transfer` entries. The controller processes transfers in FIFO order for the same message, and the message completion reports status and actual length.
- `struct spi_transfer` fields to teach: `tx_buf`, `rx_buf`, `len`, `tx_dma`, `rx_dma`, `speed_hz`, `bits_per_word`, `cs_change`, `delay_usecs` in older sources, and `tx_nbits`/`rx_nbits` for dual/quad-style transfers. Current kernels add richer delay fields and more capability fields; validate before final code.
- `tx_buf` and `rx_buf` can be NULL for write-only/read-only operations. If both are present, `len` applies to both because SPI shifts data in and out simultaneously. Buffers must remain valid until transfer completion, especially with `spi_async()`.
- DMA is controller-dependent. Internal sources correctly warn that DMA-backed transfers need DMA-safe buffers, but final examples should avoid stack buffers for DMA-heavy or async examples and should keep simple examples explicitly learning-only.
- `spi_message_init()` initializes a message, `spi_message_add_tail()` appends transfers, `spi_sync()` submits a blocking message, and `spi_async()` submits a nonblocking message with callback/context.
- `spi_sync()` may sleep and must not be used in hard IRQ context. `spi_async()` can be queued without blocking, but the message, transfers, buffers, and context must live until completion.
- Helper APIs such as `spi_read()`, `spi_write()`, and `spi_write_then_read()` are useful for simple/small operations. Complex command/address/data transactions, timing needs, chip-select behavior, or multiple transfers should use `spi_message` plus `spi_sync()`/`spi_async()`.
- Register-oriented SPI chips often should use regmap instead of open-coded SPI transfers. `regmap_init_spi()`/`devm_regmap_init_spi()` let the driver describe register/value width, readable/writable/volatile/precious registers, cache policy, and read/write flag masks.
- `spidev` exposes raw SPI from userspace through `/dev/spidevB.C`. Plain `read()`/`write()` are half-duplex and deassert CS between calls; `SPI_IOC_MESSAGE(N)` supports full-duplex and composite transfers. Use `spidev` for prototyping and simple userspace-controlled devices, not as a substitute for a kernel driver when IRQs, framework integration, PM, or kernel ABI are needed.

## Source Differences
- `ldd1-ch08` is the compact book chapter. Notion chapter 8 SPI parts are not mere duplicates: part 1 adds clearer architecture and mode diagrams, part 2 adds cleaner transfer patterns and flash examples, and part 3 adds richer DT/userspace examples and a checklist.
- Notion has two different chapter-8 sequences. `Chapter 8-Part 1.md` and `Chapter 8-Part 2.md` are V4L2/media-controller notes, while `Chapter 8-Part 1 SPI Architecture...`, `Part 2 SPI Transfer...`, and `Part 3 Device Tree...` are the relevant SPI notes. Same raw chapter number is not enough to infer topic identity.
- Internal sources use older `struct spi_master` terminology in several places. Current kernel documentation uses `struct spi_controller` for controller-side APIs, while `spi_device` still represents SPI peripheral devices. Final docs should say older material may call this "master"; modern kernel docs increasingly use "controller".
- `ldd1-ch08` uses older DT binding file names such as `Documentation/devicetree/bindings/spi/spi-bus.txt`. Current kernels use YAML bindings such as `spi-controller.yaml` and per-device binding YAMLs. Final docs should point learners to current YAML bindings and `dtbs_check`.
- `ldd1-ch08` and Notion part 3 show `compatible = "spidev"` examples. Current upstream spidev documentation says defining a SPI device as `.modalias = "spidev"` or `compatible = "spidev"` is no longer supported; a real device name from spidev's supported tables or explicit sysfs `driver_override` binding is required.
- `ldd1-ch08` shows `delay_usecs` in `struct spi_transfer`. Current kernel docs show richer `struct spi_delay delay`, `cs_change_delay`, and `word_delay` fields plus newer fields such as `effective_speed_hz`, `SPI_NBITS_OCTAL`, timestamp/offload flags, and error flags. Avoid copying old struct definitions verbatim into final examples.
- Internal sources describe `spi_async()` as usable in atomic/IRQ context. That high-level point should be validated against the exact call path and object lifetime in final examples; even if submission is nonblocking, buffers and completion context must be valid and any completion processing must obey context rules.
- `ldd1-ch08` code snippets have formatting/extraction artifacts and a few likely typos, such as duplicated `spi_setup(spi);`, malformed designated initializers, and inconsistent variable names in spidev examples. Use the concepts, not the literal code, for final learner-facing examples.
- `ldd1-ch09` uses unmanaged `regmap_init_spi()`/`regmap_exit()`. `ldd2-ch02` adds `devm_regmap_init_spi()` and better devm lifetime framing. Prefer devm-managed regmap in modern examples when lifetime matches `&spi->dev`.
- `ldd2` has no dedicated SPI device-driver chapter. Its relevant SPI content is mostly cross-topic: regmap, V4L2 subdevices, CCF slow-bus caveats, and tracing/debugging. Do not pretend there is an `ldd2` equivalent of `ldd1-ch08`.

## Gaps / Uncertainties
- Need a target kernel version before writing final code examples because SPI struct fields and naming have drifted. Use current docs for Linux 6.x unless the user specifies an older target.
- Internal sources do not deeply cover ACPI/software-node SPI device instantiation. Keep DT as the embedded-Linux primary path and mention ACPI only as an alternative after validation.
- Internal sources do not deeply cover controller-driver development. This topic should remain focused on SPI device/protocol drivers; controller-driver details should be a gap or advanced appendix if needed.
- Internal sources do not provide a production-grade SPI DT binding workflow. Final docs should validate against current `Documentation/devicetree/bindings/spi/` YAML and per-device bindings.
- Internal sources only lightly mention SPI tracepoints. Final debugging docs should validate current trace events under `/sys/kernel/debug/tracing/events/spi`, dynamic debug in `drivers/spi/`, and useful sysfs paths.
- Internal sources do not cover SPI memory (`spi-mem`), SPI-NOR framework internals, quad/octal/DTR modes, or controller offload APIs deeply. Mention as advanced/out-of-scope unless the final examples target flash.
- Internal sources do not cover runtime PM/system PM for SPI clients in depth. Final docs should add current PM guidance if examples include sensors, displays, flash, or CAN controllers.
- Learning-path boundaries:
  - Topic 10/11 owns general DT syntax, OF APIs, bindings, and `dtbs_check`.
  - Topic 15 owns generic IRQ handling and threaded IRQ design.
  - Topic 18 owns regmap details.
  - Topic 21 owns DMA mapping details.
  - Topic 24 owns power management.
  - Topic 25/26/27/30/32/35 may contain applied SPI-client examples for IIO, input, RTC, networking, V4L2, and ASoC.

## External Validation
- Used: https://docs.kernel.org/6.17/driver-api/spi.html
  - Validates current SPI architecture, controller/protocol-driver split, `struct spi_controller`, `struct spi_device`, `struct spi_driver`, message queue model, `struct spi_transfer` current field drift, controller queue behavior, and current naming caveats.
- Used: https://docs.kernel.org/6.6/spi/spidev.html
  - Validates userspace SPI API, `read()`/`write()` half-duplex behavior, `SPI_IOC_MESSAGE(N)` for full-duplex/composite operations, ioctl settings, `/dev/spidevB.C` naming, and the upstream rule that `compatible = "spidev"` is no longer supported directly.
- Still needed before final knowledge/example files:
  - Current in-tree simple SPI device drivers for the chosen target kernel, preferably one register-oriented regmap client and one subsystem-integrated client.
  - Current SPI DT YAML binding details, especially `spi-controller.yaml`, `spi-peripheral-props.yaml` if present in the target tree, and the target device's binding.
  - Current tracepoint names and debug workflow under `/sys/kernel/debug/tracing/events/spi`.

## Learning Content Brief
- Topic scope: write Linux SPI device/protocol drivers for peripheral chips behind an existing SPI controller. Do not teach SPI controller-driver implementation except as background.
- Related topics:
  - Topic 09: platform bus, non-discoverable devices, and matching mental model.
  - Topic 10/11: DT syntax, bindings, and OF property APIs.
  - Topic 15: IRQ handling for SPI devices with interrupt pins.
  - Topic 18: regmap for register-based SPI chips.
  - Topic 21: DMA-safe buffers and DMA mapping.
  - Topic 24: runtime/system PM.
- Beginner mental model:
  - The SPI controller owns the wires and timing.
  - The SPI device is one chip selected by one chip-select line.
  - The SPI driver owns the chip protocol: command bytes, register layout, delays, status polling, IRQ handling, and framework registration.
  - A SPI transfer always clocks data both directions. A read is usually "send command/dummy bytes while receiving useful bytes".
  - Probe means "the kernel matched this described chip to your driver", not "scan all chip selects".
- Core mechanism:
  - Firmware/DT/board data describes a SPI child device with `compatible`, `reg` chip-select index, `spi-max-frequency`, mode flags, IRQs, supplies, clocks, and device properties.
  - SPI core creates a `struct spi_device` and matches it with a `struct spi_driver`.
  - Probe configures bus parameters, calls `spi_setup()`, allocates state, verifies/configures the chip, and registers with a higher-level subsystem.
  - Runtime callbacks submit `spi_message` objects containing `spi_transfer` objects, or use helpers/regmap for simple register operations.
  - Remove/shutdown/PM callbacks stop new operations, complete/cancel outstanding work, unregister subsystem objects, and leave hardware safe.
- Important structs/APIs:
  - Driver/model: `struct spi_device`, `struct spi_driver`, `struct spi_device_id`, `struct of_device_id`, `struct spi_board_info`, `module_spi_driver()`, `spi_register_driver()`, `spi_unregister_driver()`, `MODULE_DEVICE_TABLE(spi, ...)`, `MODULE_DEVICE_TABLE(of, ...)`.
  - Setup/matching/private data: `spi_setup()`, `spi_get_device_id()`, `spi_set_drvdata()`, `spi_get_drvdata()`, `devm_kzalloc()`, `dev_get_platdata()`, `of_match_device()`, `of_property_read_u32()`, `of_property_read_bool()`, `of_property_read_string()`.
  - Modes/settings: `SPI_MODE_0`, `SPI_MODE_1`, `SPI_MODE_2`, `SPI_MODE_3`, `SPI_CPOL`, `SPI_CPHA`, `SPI_CS_HIGH`, `SPI_LSB_FIRST`, `SPI_3WIRE`, `SPI_LOOP`, `SPI_NO_CS`, `SPI_READY`, `max_speed_hz`, `bits_per_word`.
  - Transfers: `struct spi_transfer`, `struct spi_message`, `spi_message_init()`, `spi_message_add_tail()`, `spi_sync()`, `spi_async()`, `spi_read()`, `spi_write()`, `spi_write_then_read()`.
  - Userspace: `spidev`, `/dev/spidevB.C`, `struct spi_ioc_transfer`, `SPI_IOC_RD_MODE`, `SPI_IOC_WR_MODE`, `SPI_IOC_RD_MODE32`, `SPI_IOC_WR_MODE32`, `SPI_IOC_RD_BITS_PER_WORD`, `SPI_IOC_WR_BITS_PER_WORD`, `SPI_IOC_RD_MAX_SPEED_HZ`, `SPI_IOC_WR_MAX_SPEED_HZ`, `SPI_IOC_MESSAGE(N)`.
  - Regmap: `devm_regmap_init_spi()`, `regmap_read()`, `regmap_write()`, `regmap_update_bits()`, `struct regmap_config`.
- Lifecycle/data flow:
  - Driver load: `module_spi_driver()` registers the `spi_driver`.
  - Device creation: DT/ACPI/board data creates a `spi_device` under a controller.
  - Match: SPI core matches OF compatible or SPI ID/name and calls probe.
  - Probe: parse firmware/config -> set mode/speed/bits -> `spi_setup()` -> allocate private state -> initialize locks/work/PM -> save private data -> verify chip ID/status -> register subsystem interface -> enable IRQ/PM.
  - Runtime operation: build transfers -> initialize message -> append transfers -> submit sync/async -> check return value and `message.status`/`actual_length` when relevant -> decode received bytes.
  - Remove: block new work -> unregister subsystem object -> stop IRQ/work/timers -> wait for outstanding async messages if any -> cleanup non-devm resources.
- Examples to produce later:
  - Learning-only SPI sensor/register driver with DT match, current probe/remove style, `spi_setup()`, private state, `spi_write_then_read()` register read, and subsystem placeholder.
  - Multi-transfer command/address/read example using `spi_message` and `spi_sync()`.
  - Regmap variant for the same register device using `devm_regmap_init_spi()`.
  - DTS snippet for an enabled SPI controller and one child node with `compatible`, `reg`, `spi-max-frequency`, mode flags, interrupt, reset GPIO, supply, and custom property.
  - Userspace `spidev` example as a prototyping/debug supplement, with the modern warning against `compatible = "spidev"` in upstream DT.
- Common bugs:
  - Writing a platform driver for a chip that is actually a SPI peripheral.
  - Treating SPI probe as bus scanning rather than binding to a described device.
  - Missing `MODULE_DEVICE_TABLE()` so module autoload does not work.
  - Wrong CPOL/CPHA mode, active-high/active-low CS polarity, bit order, bits per word, or max speed.
  - Treating `reg` in a SPI child node as an MMIO address instead of chip-select index.
  - Forgetting `#address-cells = <1>` and `#size-cells = <0>` on the SPI controller node.
  - Using `spi_sync()` in hard IRQ context.
  - Letting stack or temporary buffers go out of scope before async completion.
  - Assuming `read()`/`write()` on spidev are full-duplex or preserve chip select across calls.
  - Copying `compatible = "spidev"` from old material into an upstream-oriented DT.
  - Ignoring short/failed transfers, `message.status`, or `actual_length`.
  - Not protecting multi-step state updates, read-modify-write sequences, and shared transfer buffers with a mutex.
  - Using raw SPI helper loops for a register-heavy chip where regmap would simplify locking, cache, access policy, and debug.
- Debugging notes:
  - Start with instantiation: controller enabled, child node present, `reg` CS index correct, `spi-max-frequency` set, pinctrl and CS GPIOs configured, and module aliases available.
  - Check binding/matching: `dmesg`, `modinfo`, `/sys/bus/spi/devices`, `/sys/bus/spi/drivers`, and whether probe logs appear.
  - For no communication, separate wrong mode, wrong CS polarity/index, excessive clock, missing power/reset, wrong pinmux, bad level shifting, and invalid command protocol.
  - For transfer bugs, log command bytes and lengths with `dev_dbg()`/dynamic debug, check `spi_sync()` return, `message.status`, and `message.actual_length`.
  - For IRQ-driven SPI devices, confirm the interrupt GPIO maps to `spi->irq`, the trigger type is right, and SPI access runs in a threaded handler or workqueue, not a hard IRQ handler.
  - Use logic analyzer traces when possible; SPI failures are often visible as wrong CS timing, wrong clock phase, or missing dummy cycles.
  - Validate current SPI tracepoints before final docs; ldd2 debugging notes show a `spi` ftrace event directory exists in the example system.
- Production concerns:
  - Prefer a real subsystem driver over raw spidev when the device needs IRQs, power management, kernel frameworks, security/ABI discipline, or shared access.
  - Prefer DT bindings with documented vendor properties; treat bindings as ABI and run binding checks.
  - Prefer `devm_*` helpers when lifetime matches `&spi->dev`, but explicitly handle subsystem unregister order, outstanding async transfers, workqueues, IRQs, PM, and device-safe shutdown.
  - Prefer regmap for register maps unless the protocol is unusual or streaming-oriented.
  - Keep SPI transfer buffers valid and DMA-safe for the chosen transfer path; avoid relying on stack buffers in async/DMA-sensitive paths.
  - Respect datasheet timing: reset delays, write-enable/status polling, page/sector boundaries, dummy bytes, max SCK per mode/voltage, and chip-select timing.
  - Add runtime PM only when the device and subsystem need it, but ensure register access resumes the device first.
  - Avoid baking board-specific GPIO/regulator/clock assumptions into the driver; parse them from firmware and validate required resources.
- Interview angles:
  - Explain SPI controller driver vs SPI device/protocol driver.
  - Explain why SPI is full-duplex and how that affects "read" operations.
  - Explain CPOL/CPHA and how `SPI_MODE_0..3` map to them.
  - Explain why SPI devices need DT/board/ACPI instantiation.
  - Explain what `reg` means in a SPI child node.
  - Explain `struct spi_device` vs `struct spi_driver`.
  - Explain what probe should do before the first transfer.
  - Explain `spi_setup()` and when per-transfer overrides are useful.
  - Explain `spi_transfer` vs `spi_message` and how chip select behaves across transfers.
  - Compare `spi_sync()`, `spi_async()`, and helper APIs.
  - Explain why `spi_sync()` cannot be used in hard IRQ context.
  - Explain why async buffers/context lifetime matters.
  - Explain when to use regmap instead of raw SPI APIs.
  - Explain `spidev` limitations and why `compatible = "spidev"` is stale upstream guidance.
