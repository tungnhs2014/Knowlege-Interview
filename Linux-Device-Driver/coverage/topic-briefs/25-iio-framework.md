# Topic Brief - 25 - IIO Framework

## Output Targets
- Knowledge: `knowledge/25-iio-framework.md`
- Interview: `interview/25-iio-framework.md`
- Example: `examples/25-iio-framework/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch10` | `docs/Linux Device Driver Development/Chapter 10-IIO Framework.md` | read/mapped/covered/merged | Primary IIO source: device/channel model, `struct iio_dev`, `struct iio_info`, `struct iio_chan_spec`, sysfs ABI naming, direct reads, triggered buffers, triggers, scan elements, BMA220 case study, userspace capture flow, and IIO tools. |
| `ldd1-ch02` | `docs/Linux Device Driver Development/Chapter 2-Device Driver Basis.md` | read/mapped/covered-adjacent | Error-pointer example built around `struct iio_dev *` allocation; useful for IIO allocation error paths and `ERR_PTR()`/`IS_ERR()`/`PTR_ERR()` handling. Main error-handling content belongs to topic 04. |
| `ldd1-ch06` | `docs/Linux Device Driver Development/Chapter 6-The Concept of a Device Tree .md` | read/mapped/related | Platform/DT probe excerpt uses `struct iio_dev *` and XADC-style resources; useful only as resource-extraction context for IIO-capable platform devices. Main DT API content remains topic 11. |
| `ldd1-ch09` | `docs/Linux Device Driver Development/Chapter 9-Regmap API .md` | read/mapped/related | Regmap chapter explicitly points to IIO ADC devices on SPI/I2C and suggests writing an IIO driver using regmap. Full regmap remains topics 18-19. |
| `ldd1-ch01` | `docs/Linux Device Driver Development/Chapter 1-Introduction to Kernel Development .md` | read/mapped/incidental | Lists IIO devices among framework device types that are dynamically allocated with subsystem-specific APIs. |
| `ldd1-ch04` | `docs/Linux Device Driver Development/Chapter 4-Character Device Drivers.md` | read/mapped/incidental | Notes that frameworks such as IIO hide direct character-device number allocation behind subsystem APIs. Character-device mechanics remain topic 07. |
| `ldd2-source-root` | `docs/Linux Device Driver Development 2/` | searched/mapped/gap | No dedicated IIO framework chapter found in book 2. Relevant hits are runtime-PM and regmap references, recorded below. |
| `ldd2-ch10` | `docs/Linux Device Driver Development 2/Chapter 10-Linux_Kernel_Power_Management.md` | read/mapped/covered-adjacent | IIO ambient light sensor `bh1780` runtime-PM case study: `iio_device_register()`, `bh1780_read_raw()`, `IIO_LIGHT`, `IIO_CHAN_INFO_RAW`, `IIO_CHAN_INFO_INT_TIME`, `iio_priv()`, and autosuspend around channel reads. Full PM remains topic 24. |
| `ldd2-ch02` | `docs/Linux Device Driver Development 2/Chapter 2-Regmap_API.md` | read/mapped/related | Mentions an IIO light driver (`apds9960.c`) as a regmap cache/default example. Full regmap remains topic 18. |
| `notion-source-root` | `docs/Linux-Device-Driver-Notion/` | searched/mapped/gap | No dedicated Notion IIO chapter found. Notion IIO mentions are setup, error-handling, and bus-integration snippets, recorded below. |
| `notion-ch02-part3` | `docs/Linux-Device-Driver-Notion/Chapter 2-Part 3 Error Handling & Message Printing.md` | read/mapped/covered-adjacent | Expanded `ERR_PTR()`/`IS_ERR()`/`PTR_ERR()` example using `devm_iio_device_alloc()` and `struct iio_dev *`; useful for IIO probe helper error handling. |
| `notion-ch01-part2` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 2 Kernel Configuration.md` | read/mapped/related | Kconfig/menuconfig example enabling Industrial I/O support and the BMA220 accelerometer driver. |
| `notion-ch01-part1` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 1 Environment Setup and Getting Source Code.md` | read/mapped/incidental | Kernel source-tree overview lists `drivers/iio/` as Industrial I/O for ADC, DAC, and sensors. |
| `notion-ch07-part1` | `docs/Linux-Device-Driver-Notion/Chapter 7-Part 1 I2C Architecture and Driver Structures.md` | read/mapped/related | I2C probe skeleton ends by registering with a higher framework such as IIO; useful for bus-to-subsystem lifecycle framing. Full I2C remains topic 16. |
| `notion-ch08-part1` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 1 SPI Architecture and Driver Structures.md` | read/mapped/related | SPI ADC-style probe skeleton verifies chip ID then registers with IIO or another subsystem. Full SPI remains topic 17. |

## Source Files Read
- `ldd1-ch10`: complete chapter. Read introduction, IIO architecture/layout, headers, `iio_dev`, `iio_info`, channel specification, channel attribute naming, indexed and modified channel examples, dummy voltage driver, triggered buffer support, sysfs/interrupt/hrtimer triggers, buffer sysfs interface, scan elements, BMA220 accelerometer case study, one-shot capture, buffered capture commands, hrtimer capture commands, and IIO tools.
- `ldd1-ch02`: targeted section "Handling null pointer errors" containing IIO allocation with `devm_iio_device_alloc()` and error-pointer propagation.
- `ldd1-ch06`: targeted platform/DT probe excerpt around resource extraction with `struct iio_dev *`.
- `ldd1-ch09`: summary section pointing from regmap to IIO ADC/sensor devices on SPI/I2C.
- `ldd1-ch01`: framework-device allocation list containing IIO.
- `ldd1-ch04`: character-device registration note that frameworks such as IIO wrap lower-level char-device allocation.
- `ldd2-ch10`: targeted runtime-PM sections containing the `bh1780` IIO light sensor probe, `read_raw()` flow, autosuspend behavior, and remove path.
- `ldd2-ch02`: targeted regmap configuration section mentioning `drivers/iio/light/apds9960.c`.
- `notion-ch02-part3`: targeted `ERR_PTR()` example using `devm_iio_device_alloc()`.
- `notion-ch01-part2`: targeted kernel configuration/menuconfig example enabling Industrial I/O and BMA220.
- `notion-ch01-part1`: targeted kernel source-tree map showing `drivers/iio/`.
- `notion-ch07-part1`: targeted I2C probe skeleton framework-registration step.
- `notion-ch08-part1`: targeted SPI ADC probe skeleton framework-registration step.

### Inventory Decisions
- `ldd1-ch10` is the only dedicated internal IIO framework source and is primary for topic 25.
- `ldd2` has no dedicated IIO chapter. `ldd2-ch10` is not treated as "same-number" IIO content; it was read and mapped as a power-management chapter with an IIO sensor case study.
- Notion has no Chapter 10 IIO equivalent in the source root. The Notion files with IIO hits were read individually and mapped as setup/error-handling/bus-adjacent material.
- Broad terms such as "sensor", "buffer", "trigger", "ADC", and "sysfs" appear heavily in V4L2, ASoC, framebuffer, RTC, GPIO, and generic driver chapters. Those were not treated as IIO sources unless the file also had IIO-specific terms or a clear IIO subsystem relationship.
- Related topic boundaries:
  - I2C mechanics stay in topic 16.
  - SPI mechanics stay in topic 17.
  - Regmap stays in topics 18-19.
  - Runtime PM stays in topic 24.
  - Input sensors/buttons stay in topic 26.
  - V4L2 camera sensors stay in topics 32-34.

## Merged Source Notes
- IIO is the kernel subsystem for ADCs, DACs, and many sensors: accelerometers, gyroscopes, current/voltage monitors, light sensors, pressure sensors, and similar measurement devices.
- The central mental model is device plus channels:
  - The IIO device represents the physical chip.
  - A channel represents one acquisition/output line or one logical measurement stream, such as `x`, `y`, `z` acceleration axes or ADC voltage inputs.
- Userspace has two main interfaces:
  - `/sys/bus/iio/devices/iio:deviceX/` for attributes, one-shot reads, configuration, events, trigger selection, buffer controls, and scan-element controls.
  - `/dev/iio:deviceX` for buffered data and event character-device access.
- `struct iio_dev` is the runtime object registered with the IIO core. Important fields for learning: `dev.parent`, `name`, `modes`, `channels`, `num_channels`, `info`, `available_scan_masks`, `active_scan_mask`, `scan_bytes`, `buffer`, `trig`, and `pollfunc`.
- Allocate IIO devices with `devm_iio_device_alloc()` when the IIO object lifetime is tied to the parent device. Use `iio_priv(indio_dev)` for per-device private state.
- Register only after the object is fully initialized. For triggered buffers, set up the buffer before `iio_device_register()`, and clean it up on registration failure or remove when unmanaged.
- `struct iio_info` provides core callbacks such as `read_raw()` and `write_raw()`. Sysfs channel reads call `read_raw()` with a channel and mask such as `IIO_CHAN_INFO_RAW`, `IIO_CHAN_INFO_SCALE`, or `IIO_CHAN_INFO_INT_TIME`.
- `read_raw()` fills `*val` and optionally `*val2`, then returns an `IIO_VAL_*` code that tells the core how to format the value.
- `write_raw()` is optional and should exist only when userspace may change attributes such as scale, sampling frequency, integration time, or calibration.
- `struct iio_chan_spec` describes channel type, index/modifier, register address or channel address, sysfs attributes, scan order, and buffer storage format.
- Channel types include values such as `IIO_VOLTAGE`, `IIO_LIGHT`, `IIO_ACCEL`, and `IIO_TEMP`; available types live in the IIO UAPI type definitions.
- Attribute naming is generated by the core from direction, type, index, modifier, and info mask. Examples:
  - Indexed ADC: `in_voltage0_raw`, `in_voltage1_raw`.
  - Modified accelerometer: `in_accel_x_raw`, `in_accel_y_raw`, `in_accel_z_raw`.
  - Shared type attribute: `in_accel_scale`.
- Use `info_mask_separate` for per-channel attributes, `info_mask_shared_by_type` for all channels of the same type, `info_mask_shared_by_dir` for same direction, and `info_mask_shared_by_all` for all channels.
- Direct mode is one-shot sysfs access. Reading a raw attribute causes the driver to access the chip for that channel and return one result.
- Triggered buffering is for repeated or synchronized acquisition. A trigger tells the driver when to sample, the driver pushes enabled channel data into the buffer, and userspace reads from `/dev/iio:deviceX`.
- Triggered buffer support normally uses `iio_triggered_buffer_setup()` or a related helper before registration. The top half should be minimal, often just timestamp capture via `iio_pollfunc_store_time`; the threaded bottom half reads the device, fills the scan buffer, calls `iio_push_to_buffers_with_timestamp()`, and finishes with `iio_trigger_notify_done()`.
- A driver must avoid direct `read_raw()` hardware access while buffered capture is active if the paths would conflict. The source uses `iio_buffer_enabled(indio_dev)` and returns `-EBUSY`.
- `available_scan_masks` restricts valid combinations of enabled channels. `active_scan_mask` tells the trigger handler which channels userspace enabled.
- `scan_index` orders channels in the buffer. `scan_type` describes how each channel is packed: sign, valid bits, storage bits, shift, repeat, and endianness.
- Sysfs scan-element type strings such as `le:s12/16>>4` must be interpreted according to the sensor datasheet and `scan_type`.
- Trigger sources can be independent of the sensor. Internal sources cover sysfs triggers, interrupt triggers, and hrtimer triggers. Current docs also emphasize that a trigger may come from a hardware event such as data-ready or threshold exceeded, or from a separate source.
- Userspace buffered capture flow:
  - Create or select a trigger.
  - Write trigger name to `trigger/current_trigger`.
  - Enable desired `scan_elements/*_en`.
  - Set `buffer/length`.
  - Enable the buffer.
  - Fire or wait for trigger.
  - Read `/dev/iio:deviceX`.
  - Disable buffer and detach trigger during cleanup.
- IIO tools from kernel `tools/iio` include `lsiio`, `generic_buffer`, and `iio_event_monitor`. `libiio` is useful for userspace applications and remote IIO workflows.
- IIO drivers commonly sit under I2C, SPI, or platform bus probe paths. The bus driver verifies communication and hardware identity, configures power/resources, then registers the IIO device with the core.
- Runtime PM matters for real sensors. The `bh1780` case study resumes the device before channel reads, reads the conversion value, marks last busy, and schedules autosuspend.
- Regmap is a natural fit for many IIO chips because sensors and ADCs are often register-oriented I2C/SPI devices.

## Source Differences
- `ldd1-ch10` uses older code style and extraction artifacts:
  - It presents both `iio_device_alloc()` and `devm_iio_device_alloc()`, but examples mix wording. Final docs should prefer `devm_iio_device_alloc()` for normal driver-owned lifetimes and explain unmanaged cleanup separately.
  - Some code snippets have formatting errors, such as `struct struct iio_chan_spec`, odd code-fence markers, typo `scant_type`, and a likely bad example assignment for `available_scan_masks`. Final examples must be rewritten, not copied.
- `ldd1-ch10` says `iio_triggered_buffer_setup()` gives `INDIO_DIRECT_MODE` capability. The safer final phrasing is: initialize `indio_dev->modes` for direct mode as appropriate; the triggered-buffer helper adds buffer/pollfunc support and should be called before registration.
- `ldd1-ch10` describes paths such as `/sys/bus/iio/iio:deviceX/buffer/*`. Current docs primarily use `/sys/bus/iio/devices/iio:deviceX/...`; current buffer docs also mention `bufferY` directories for devices with buffer support. Final docs should use current `devices/iio:deviceX` spelling and mention `buffer` versus `buffer0` drift carefully.
- `ldd1-ch10` says the interrupt trigger driver does not support DT yet but then shows a DT node. This is stale/ambiguous and should not be taught as a modern production binding without validation.
- `ldd1-ch10` names older trigger modules/options:
  - `iio-trig-gpio` versus `iio-trig-interrupt`.
  - `iio-trig-rtc` versus `iio-trig-hrtimer`.
  - Final docs should validate Kconfig symbols and module names against the target kernel before giving build instructions.
- `ldd2-ch10` contributes an applied IIO runtime-PM example but does not teach IIO architecture. It should be merged as production lifecycle guidance, not as a second IIO source.
- Notion does not duplicate the IIO chapter. Its IIO references are supportive and should not be inflated into topic coverage.
- Current docs add or emphasize details not clear in `ldd1-ch10`:
  - Standard IIO attributes are documented in `Documentation/ABI/testing/sysfs-bus-iio`.
  - Trigger setup has resource-managed helpers such as `devm_iio_trigger_alloc()` and `devm_iio_trigger_register()`.
  - Triggered-buffer setup has `iio_triggered_buffer_setup_ext()` with data stream direction and extra buffer attributes.
  - ADXL345 documentation shows current-style device attributes, events, and both `buffer` and `buffer0` usage examples.

## Gaps / Uncertainties
- Internal sources do not deeply cover IIO event APIs: `struct iio_event_spec`, event code helpers, event enable/value callbacks, event character-device flow, and `iio_event_monitor` beyond tool naming.
- Internal sources do not cover IIO consumer APIs or in-kernel consumers, including `iio_channel_get()`, `iio_read_channel_raw()`, processed reads, and hw-consumer paths.
- Internal sources do not cover triggered buffer variants, DMA/hardware buffers, multi-buffer devices, FIFO-based IIO drivers, or high-rate sampling design.
- Internal sources do not explain timestamp clock selection, timestamp alignment, scan-byte padding/alignment, or userspace binary parsing in enough depth for a robust example.
- Internal sources do not provide a clean current minimal IIO driver example. Later code should be checked against target kernel headers.
- Internal sources do not cover current DT bindings for specific IIO devices, interrupts/data-ready GPIOs, regulators, clocks, mount matrices, calibration, or sampling-frequency properties.
- Internal sources do not compare IIO with hwmon/input/V4L2. Final docs should briefly explain subsystem choice for sensors without drifting into those topics.
- Internal sources do not cover libiio usage beyond naming it.
- Internal sources do not cover concurrency/lifetime in enough detail: locking between `read_raw()`, trigger handlers, PM callbacks, IRQ handlers, remove/unbind, and buffer enable/disable.

## External Validation
- Used: `https://docs.kernel.org/driver-api/iio/core.html`
  - Validated current IIO core model: `iio_device_alloc()`, `iio_device_register()`, reverse-order unregister/free, sysfs attributes, `/dev/iio:deviceX`, channels, `struct iio_chan_spec`, and info-mask grouping.
- Used: `https://docs.kernel.org/driver-api/iio/buffers.html`
  - Validated driver-API buffer concepts, scan elements, `length`, `enable`, and `/sys/bus/iio/devices/iio:deviceX/scan_elements/`.
- Used: `https://docs.kernel.org/iio/iio_devbuf.html`
  - Validated current userspace buffer documentation, `bufferY` directory wording, and the rule that buffer `enable` should be written after length and scan-element selection.
- Used: `https://docs.kernel.org/driver-api/iio/triggers.html`
  - Validated trigger model, trigger sysfs paths, `current_trigger`, independent trigger sources, and current managed trigger helper names.
- Used: `https://docs.kernel.org/driver-api/iio/triggered-buffers.html`
  - Validated triggered-buffer setup order: initialize `indio_dev`, call setup before `iio_device_register()`, and clean up with `iio_triggered_buffer_cleanup()`.
- Used: `https://docs.kernel.org/iio/adxl345.html`
  - Validated a current applied IIO accelerometer example: raw/scale/calibration/sampling-frequency attributes, processed-value formula `(_raw + _offset) * _scale`, event examples, and userspace buffer flow.
- Still needed before final learner/example files:
  - Validate actual target-kernel headers for `struct iio_info`, `struct iio_chan_spec`, `devm_iio_device_alloc()`, `devm_iio_device_register()`, `iio_triggered_buffer_setup_ext()`, event APIs, trigger helpers, and buffer directory naming.
  - Validate Kconfig symbols and module names for `CONFIG_IIO`, `CONFIG_IIO_BUFFER`, `CONFIG_IIO_TRIGGERED_BUFFER`, `CONFIG_IIO_SYSFS_TRIGGER`, `CONFIG_IIO_CONFIGFS`, hrtimer trigger, interrupt trigger, and sample drivers.
  - Validate a real minimal driver pattern from current in-tree simple IIO drivers before writing examples.
  - Validate ABI paths directly against current `Documentation/ABI/testing/sysfs-bus-iio*` before documenting uncommon attributes.

## Learning Content Brief
- Learning path number: `25`.
- Slug: `iio-framework`.
- Topic scope:
  - Sensors, ADCs/DACs, IIO device/channel model, sysfs ABI, `read_raw()`/`write_raw()`, channel specs, direct mode, triggered buffers, triggers, scan elements, userspace capture, debugging, production sensor-driver lifecycle, and interview reasoning.
  - Keep raw I2C/SPI bus access in topics 16-17, regmap in topics 18-19, runtime PM depth in topic 24, input devices in topic 26, and V4L2 camera sensors in topics 32-34.
- Beginner mental model:
  - An IIO driver is a translator between a physical measurement chip and a standard Linux sensor ABI.
  - The chip is the IIO device; each measurable signal is a channel; channel attributes expose one-shot values and settings; buffers expose repeated samples.
  - `read_raw()` is "give me one value now"; triggered buffer mode is "sample selected channels whenever a trigger fires and stream packed samples to userspace".
- Core mechanism:
  - Bus probe allocates private state and `struct iio_dev`.
  - Driver fills `indio_dev->name`, `modes`, `channels`, `num_channels`, `info`, parent device, and optional scan masks.
  - Optional triggered-buffer setup allocates buffer/pollfunc support.
  - `iio_device_register()` exposes sysfs attributes and character-device access.
  - Sysfs reads/writes call `iio_info` callbacks with a channel and mask.
  - Trigger handlers read enabled channels, pack samples according to `scan_type`, add timestamp if used, push to buffers, and notify trigger completion.
- Important structs/APIs:
  - Core structs: `struct iio_dev`, `struct iio_info`, `struct iio_chan_spec`, `struct iio_buffer`, `struct iio_trigger`, `struct iio_poll_func`, `struct iio_buffer_setup_ops`, `struct iio_event_spec`.
  - Allocation/lifetime: `devm_iio_device_alloc()`, `iio_priv()`, `iio_device_register()`, `iio_device_unregister()`, unmanaged `iio_device_alloc()`/`iio_device_free()` where needed, and possibly `devm_iio_device_register()` after validation.
  - Raw callbacks and return formats: `read_raw()`, `write_raw()`, `IIO_CHAN_INFO_RAW`, `IIO_CHAN_INFO_PROCESSED`, `IIO_CHAN_INFO_SCALE`, `IIO_CHAN_INFO_OFFSET`, `IIO_CHAN_INFO_SAMP_FREQ`, `IIO_CHAN_INFO_INT_TIME`, `IIO_VAL_INT`, `IIO_VAL_INT_PLUS_MICRO`, `IIO_VAL_FRACTIONAL`.
  - Channel fields: `type`, `channel`, `channel2`, `address`, `modified`, `indexed`, `output`, `differential`, `info_mask_separate`, `info_mask_shared_by_type`, `scan_index`, `scan_type`.
  - Buffer/trigger APIs: `iio_triggered_buffer_setup()`, `iio_triggered_buffer_cleanup()`, `iio_triggered_buffer_setup_ext()`, `iio_pollfunc_store_time`, `iio_push_to_buffers_with_timestamp()`, `iio_trigger_notify_done()`, `iio_buffer_enabled()`, `for_each_set_bit()`.
  - Trigger APIs after validation: `devm_iio_trigger_alloc()`, `devm_iio_trigger_register()`, `iio_trigger_validate_own_device()`.
- Lifecycle/data flow:
  - Probe: enable resources needed for detection; verify chip ID; allocate `iio_dev`; initialize private state, locks, bus/regmap pointer, channel table, info callbacks, modes, and scan masks; set up triggered buffers if supported; register with IIO core.
  - One-shot read: userspace reads `in_*_raw`; IIO core calls `read_raw()`; driver locks, resumes/powers hardware if needed, reads register/conversion, fills `val`/`val2`, returns `IIO_VAL_*`, and releases resources.
  - Attribute write: userspace writes scale/sampling frequency/calibration; IIO core calls `write_raw()`; driver validates supported values and programs hardware safely.
  - Buffer enable: userspace selects scan elements, sets buffer length, chooses trigger, and enables buffer; setup callbacks may configure hardware for streaming.
  - Trigger fire: top half timestamps; threaded handler reads active channels, packs data, pushes buffer sample, and notifies done.
  - Remove/error: unregister IIO users first, clean triggered buffer resources if needed, stop IRQ/work/PM users, then let devm release memory/resources.
- Practical examples for later:
  - Learning-only I2C or SPI accelerometer/ADC skeleton with `devm_iio_device_alloc()`, three channels or indexed voltage channels, `read_raw()`, scale handling, and direct mode.
  - Triggered-buffer extension with `iio_triggered_buffer_setup()`, `iio_buffer_enabled()` guarding `read_raw()`, `active_scan_mask`, timestamped samples, and userspace capture commands.
  - Debug README using `ls /sys/bus/iio/devices/`, `cat name`, raw/scale reads, `scan_elements/*_type`, `buffer/length`, `buffer/enable`, `generic_buffer`, `lsiio`, and `iio_event_monitor`.
- Common bugs:
  - Registering the IIO device before channels/info/buffer setup are fully initialized.
  - Returning raw values without correct `IIO_VAL_*` format or forgetting `val2` for fractional values.
  - Wrong `info_mask_*`, causing missing or incorrectly shared sysfs attributes.
  - Using indexes when modifiers are required, or modifiers when indexed voltage channels would be clearer.
  - Wrong `scan_type` sign, realbits, storagebits, shift, endianness, or scan order, causing userspace to decode buffers incorrectly.
  - Reading hardware in `read_raw()` while the buffer trigger handler is active; use locking and `iio_buffer_enabled()` where needed.
  - Forgetting `iio_trigger_notify_done()` on error paths in the trigger handler.
  - Holding a mutex in the top half or doing slow bus I/O in interrupt context.
  - Not validating scan masks, allowing unsupported channel combinations.
  - Confusing raw, processed, scale, and offset semantics.
  - Exposing custom sysfs attributes instead of standard IIO ABI names.
  - Forgetting runtime PM around slow sensor reads or allowing autosuspend during active buffered capture.
  - Letting remove/unbind race with buffer enable, triggers, IRQs, or userspace character-device reads.
- Debugging notes:
  - Confirm the device exists under `/sys/bus/iio/devices/iio:deviceX` and check `name`.
  - Inspect generated attributes to verify channel table and info masks.
  - Compare raw/scale/offset values with the datasheet formula.
  - Inspect `scan_elements/*_type` before parsing `/dev/iio:deviceX`.
  - If buffer enable returns `-EINVAL`, check enabled channel combination, `available_scan_masks`, trigger selection, and scan element setup.
  - If reads return `-EBUSY`, check whether the buffer is enabled.
  - Use `lsiio`, `generic_buffer`, `iio_event_monitor`, `dmesg`, dynamic debug in the driver, and bus-level debug for I2C/SPI/regmap.
  - For PM-related failures, temporarily force runtime PM on/off where available and check whether register reads happen while the chip is suspended.
- Production concerns:
  - Use standard IIO ABI attributes wherever possible; avoid custom ABI unless a standard attribute cannot represent the hardware.
  - Keep channel definitions static, accurate, and datasheet-driven.
  - Define locking rules for direct reads, writes, trigger handler, IRQ/event path, PM callbacks, and remove.
  - Use regmap when the chip is register-oriented and cache/access policy helps.
  - Handle regulators, clocks, reset GPIOs, interrupts, data-ready lines, and runtime PM before exposing userspace access.
  - Make buffer sample layout stable and documented by `scan_elements`.
  - Choose the correct subsystem: IIO for measurement/streaming sensors and converters, input for user-input events, hwmon for simple hardware monitoring, and V4L2 for camera/video sensors.
- Interview angles:
  - Explain why IIO exists instead of every sensor driver creating custom sysfs files.
  - Explain IIO device versus IIO channel.
  - Explain `struct iio_dev`, `struct iio_info`, and `struct iio_chan_spec`.
  - Explain how sysfs attribute names are generated from channel fields and masks.
  - Explain raw, scale, offset, and processed values.
  - Explain direct mode versus triggered buffered capture.
  - Explain what a trigger is and why it can be unrelated to the sensor.
  - Explain scan elements and how userspace decodes binary samples.
  - Explain why `read_raw()` may return `-EBUSY` while a buffer is enabled.
  - Explain the correct probe order for a sensor behind I2C/SPI that registers with IIO.
  - Explain how runtime PM fits around one-shot reads and buffered capture.
  - Explain common review issues in an IIO driver: ABI naming, locking, scan layout, trigger completion, PM, and cleanup ordering.
