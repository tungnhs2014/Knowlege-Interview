# Topic Brief - 28 - Watchdog And NVMEM Frameworks

## Output Targets
- Knowledge: `knowledge/28-watchdog-and-nvmem-frameworks.md`
- Interview: `interview/28-watchdog-and-nvmem-frameworks.md`
- Example: `examples/28-watchdog-and-nvmem-frameworks/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-source-root` | `docs/Linux Device Driver Development/` | searched/mapped/gap | No dedicated watchdog framework or NVMEM framework chapter found. Relevant EEPROM, DT, char-device, I2C/SPI, and NIC-watchdog false-positive sources are mapped below. |
| `ldd1-ch04` | `docs/Linux Device Driver Development/Chapter 4-Character Device Drivers.md` | read/mapped/covered-adjacent | EEPROM-like char-device examples: custom `/dev/eep-memN`, read/write bounds checking, `copy_to_user()`, `copy_from_user()`, `llseek`, ioctl, and private ABI. Useful contrast to NVMEM framework, which avoids every storage chip inventing its own userspace/kernel API. |
| `ldd1-ch06` | `docs/Linux Device Driver Development/Chapter 6-The Concept of a Device Tree .md` | read/mapped/covered-adjacent | DT example of an EEPROM node with child partition-like subnodes, `reg`, `read-only`, and labels. Useful precursor to NVMEM cell/subnode representation, but not the NVMEM binding itself. |
| `ldd1-ch07` | `docs/Linux Device Driver Development/Chapter 7-I2C Client Drivers.md` | read/mapped/covered-adjacent | I2C EEPROM context: serial EEPROM as a common I2C device, 24LC512 read flow, EEPROM DT node at `reg = <0x55>`, and `module_i2c_driver(serial_eeprom_i2c_driver)`. Shows pre-framework bus-specific storage handling. |
| `ldd1-ch08` | `docs/Linux Device Driver Development/Chapter 8-SPI Device Drivers.md` | read/mapped/covered-adjacent | SPI serial EEPROM driver registration sequence; useful only as storage-device context behind a possible NVMEM provider. |
| `ldd1-ch22` | `docs/Linux Device Driver Development/Chapter 22-Network Interface Card Drivers.md` | read/mapped/related-false-positive | Contains `dev->watchdog`/`ndo_tx_timeout` and ethtool EEPROM register content. Mapped to prevent confusion: NIC transmit watchdog is not the watchdog subsystem, and ethtool EEPROM ops are not the NVMEM framework. |
| `ldd2-ch12` | `docs/Linux Device Driver Development 2/Chapter 12-NVMEM_Framework.md` | read/mapped/covered/merged | Primary NVMEM source: purpose, producer/consumer model, `struct nvmem_config`, `struct nvmem_device`, cells, provider registration, RTC-backed NVMEM, provider read/write callbacks, DT cells, consumer APIs, and sysfs binary ABI. |
| `ldd2-ch13` | `docs/Linux Device Driver Development 2/Chapter 13-Watchdog_Device_Drivers.md` | read/mapped/covered/merged | Primary watchdog source: watchdog mental model, `struct watchdog_device`, `struct watchdog_info`, `struct watchdog_ops`, registration, timeout/ping/keepalive, nowayout, hardware-already-running handling, restart handler, pretimeout governors, GPIO watchdog, `/dev/watchdog`, ioctls, and sysfs ABI. |
| `ldd2-ch03` | `docs/Linux Device Driver Development 2/Chapter 3-MFD_Subsystem_and_Syscon_API.md` | read/mapped/covered-adjacent | MFD PMIC examples instantiate watchdog children with `.of_compatible = "dlg,da9055-watchdog"` and DA9062 `watchdog` DT child. Useful real-world context for watchdogs under MFD parent devices; full MFD remains topic 19. |
| `ldd2-ch11` | `docs/Linux Device Driver Development 2/Chapter 11-Writing_PCI_Device_Drivers.md` | searched/mapped/incidental | Only points forward to the NVMEM framework chapter. No technical content merged. |
| `notion-source-root` | `docs/Linux-Device-Driver-Notion/` | searched/mapped/gap | No standalone Notion watchdog or NVMEM framework chapter found. Relevant source-tree, DT, I2C, char-device, and calibration/MAC snippets are mapped below. |
| `notion-ch01-part1` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 1 Environment Setup and Getting Source Code.md` | read/mapped/incidental | Kernel source tree map lists `drivers/watchdog/`; orientation only. |
| `notion-ch04-part3` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 3 Read, Write, and Seek Operations.md` | read/mapped/covered-adjacent | Complete simulated EEPROM char-device example with `read`, `write`, `llseek`, mutex protection, `/dev/eeprom`, and cleanup. Strong contrast source for why NVMEM standardizes nonvolatile storage access. |
| `notion-ch04-part4` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 4 Advanced Features - Poll and IOCTL.md` | read/mapped/incidental | Mentions the complete EEPROM example from part 3; no additional framework detail. |
| `notion-ch06-part1` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 1 Device Tree Fundamentals.md` | read/mapped/covered-adjacent | DT byte arrays and examples for `mac-address`, `local-mac-address`, and `eeprom@50`; useful consumer-data context for NVMEM cells. |
| `notion-ch06-part2` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 2 Device Addressing and Resources.md` | read/mapped/covered-adjacent | I2C addressing example with `eeprom@50`, `compatible = "atmel,24c02"`, `reg`, and `pagesize`. |
| `notion-ch06-part3` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 3 OF APIs and Platform Integration.md` | read/mapped/covered-adjacent | OF property examples for `local-mac-address`/`mac-address` and an EEPROM-with-partitions DT example. Useful to compare direct OF property parsing with NVMEM consumer cell lookup. |
| `notion-ch07-part1` | `docs/Linux-Device-Driver-Notion/Chapter 7-Part 1 I2C Architecture and Driver Structures.md` | read/mapped/covered-adjacent | I2C architecture examples list serial EEPROM and `eeprom.c`; reinforces EEPROM as a bus client, not the NVMEM framework itself. |
| `notion-ch07-part2` | `docs/Linux-Device-Driver-Notion/Chapter 7-Part 2 I2C Communication APIs.md` | read/mapped/covered-adjacent | 24LC512 EEPROM read/write examples using `i2c_transfer()`, `i2c_master_send()`, 16-bit offsets, write-cycle delay, and direct char-device copying. Useful contrast to provider `reg_read`/`reg_write` callbacks. |
| `notion-ch07-part3` | `docs/Linux-Device-Driver-Notion/Chapter 7-Part 3 Device Tree Integration.md` | read/mapped/covered-adjacent | EEPROM and RTC DT example, calibration property parsing, and device variants marked with `DEVICE_HAS_EEPROM`; useful context for NVMEM-backed calibration/config data. |
| `notion-ch08-part2` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 2 SPI Transfer Mechanisms and Communication APIs.md` | searched/mapped/incidental | Mentions camera sensor calibration; not an NVMEM framework source, but calibration is a common consumer-data use case. |
| `notion-ch14-part1` | `docs/Linux-Device-Driver-Notion/Chapter 14-Part 1 Pin Control Framework.md` | searched/mapped/incidental | Contains `eeprom@50` in a bus/pinctrl context only; no NVMEM framework content. |

## Source Files Read
- `ldd2-ch12`: complete NVMEM framework chapter. Read purpose, producer/consumer split, `nvmem_device`, `nvmem_config`, cells, provider registration, RTC NVMEM, provider callbacks, DT binding, consumer APIs, and sysfs ABI.
- `ldd2-ch13`: complete watchdog chapter. Read watchdog basics, kernel structs/APIs, registration/removal, restart handler, pretimeout governors, GPIO watchdog, userspace `/dev/watchdog` operations, ioctls, boot/status reporting, and sysfs.
- `ldd2-ch03`: targeted MFD watchdog child sections around DA9055 and DA9062 watchdog subdevices.
- `ldd2-ch11`: targeted forward reference to NVMEM; no technical content merged.
- `ldd1-ch04`: targeted EEPROM-like char-device examples for custom read/write/seek/ioctl behavior and error handling.
- `ldd1-ch06`: targeted DT subnode/EEPROM partition example.
- `ldd1-ch07`: targeted I2C EEPROM overview, 24LC512 read flow, EEPROM DT node, and I2C driver registration.
- `ldd1-ch08`: targeted SPI serial EEPROM driver registration mention.
- `ldd1-ch22`: targeted NIC `dev->watchdog`/`ndo_tx_timeout` and ethtool EEPROM mentions to classify them as related but not this framework.
- `notion-ch01-part1`: targeted source-tree section listing `drivers/watchdog/`.
- `notion-ch04-part3`: targeted complete EEPROM-like char-device implementation and test context.
- `notion-ch04-part4`: targeted reference back to complete EEPROM example.
- `notion-ch06-part1`: targeted DT byte array examples, MAC-address data, and `eeprom@50`.
- `notion-ch06-part2`: targeted I2C `eeprom@50` resource/address snippet.
- `notion-ch06-part3`: targeted `local-mac-address`, `mac-address`, and EEPROM partition parsing sections.
- `notion-ch07-part1`: targeted I2C architecture sections listing serial EEPROM and `eeprom.c`.
- `notion-ch07-part2`: targeted 24LC512 read/write examples.
- `notion-ch07-part3`: targeted EEPROM/RTC DT, calibration properties, and `DEVICE_HAS_EEPROM` variant flags.
- `notion-ch08-part2`: targeted calibration mention; not merged as framework content.
- `notion-ch14-part1`: targeted EEPROM bus/pinctrl false-positive; not merged as framework content.

### Inventory Decisions
- `ldd2-ch12` and `ldd2-ch13` are the only dedicated internal framework chapters and are primary for topic 28.
- `ldd1` predates or omits dedicated watchdog/NVMEM chapters. Its EEPROM examples are valuable because they show the problem NVMEM solves: custom char-device ABI, bus-specific transfer code, and ad hoc partitioning.
- Notion does not contain a direct watchdog/NVMEM framework chapter. Its contribution is contextual: source-tree orientation, DT byte arrays/MAC/calibration data, EEPROM bus examples, and char-device examples.
- NIC `dev->watchdog` and `ndo_tx_timeout` are intentionally classified as non-watchdog-framework content so the final lesson can warn against terminology collision.
- Same-number chapters were not merged across source groups. For example, `ldd1-ch04` is char devices, while `ldd2-ch04` is CCF and not a source for this topic.

## Merged Source Notes
- Topic scope has two independent frameworks:
  - **Watchdog framework**: reliability/reset supervision. A hardware or GPIO watchdog must be pinged before timeout; otherwise it resets the system.
  - **NVMEM framework**: nonvolatile storage abstraction. EEPROM, eFuse, OTP, battery-backed RAM, and similar devices expose bytes/cells to kernel consumers and optionally userspace.
- Shared design theme:
  - Both frameworks replace private one-off driver logic with standard kernel registration, core-owned userspace ABI, DT integration, lifetime rules, and common debugging paths.
  - The driver provides hardware-specific callbacks; the framework owns generic policy, ABI, and consumer-facing API.
- Watchdog mental model:
  - A watchdog is a last-resort availability mechanism. User space or the kernel opens/starts it and periodically sends keepalive pings.
  - If user space hangs, scheduling stalls, or the driver stops feeding the hardware, the watchdog expires and reboots/resets the board.
  - `nowayout` changes the operational contract: once started, the watchdog cannot be stopped and must keep being serviced until reboot.
- Watchdog kernel mechanism:
  - A driver fills `struct watchdog_device` with `parent`, `info`, `ops`, timeout fields, pretimeout fields, and status flags.
  - `struct watchdog_ops.start` is mandatory. Optional ops include `.stop`, `.ping`, `.status`, `.set_timeout`, `.set_pretimeout`, `.get_timeleft`, `.restart`, and `.ioctl`.
  - The core provides `/dev/watchdog*`, ioctl handling, status/bootstatus logic, sysfs attributes, restart-handler integration, and pretimeout governor dispatch.
  - If `.ping` is absent, the framework may use `.start` to refresh the device.
  - If hardware is already running at probe, the driver must mark `WDOG_HW_RUNNING` so the core knows it needs continued feeding.
  - If `.restart` is implemented, the core can register it as a restart handler; priority is set before registration.
  - Pretimeout is an early event before the final reset; IRQ handlers notify the core with `watchdog_notify_pretimeout()`, and a governor such as `noop` or `panic` handles policy.
- Watchdog userspace ABI:
  - Opening `/dev/watchdog` starts the watchdog.
  - Writing data or issuing `WDIOC_KEEPALIVE` pings it.
  - Magic close requires writing `V` and then closing, and only works when the driver advertises/supports that behavior and `nowayout` is not active.
  - Important ioctls include `WDIOC_GETSUPPORT`, `WDIOC_KEEPALIVE`, `WDIOC_SETTIMEOUT`, `WDIOC_GETTIMEOUT`, `WDIOC_SETPRETIMEOUT`, `WDIOC_GETTIMELEFT`, `WDIOC_GETSTATUS`, and `WDIOC_GETBOOTSTATUS`.
  - Sysfs under `/sys/class/watchdog/watchdogN/` exposes attributes such as `nowayout`, `status`, `timeleft`, `timeout`, `identity`, `bootstatus`, `state`, `pretimeout`, and governor controls when configured.
- NVMEM mental model:
  - A provider owns bytes stored in nonvolatile hardware; consumers ask for named cells instead of knowing bus transactions or raw offsets in driver code.
  - A cell is a defined byte or bit range inside the NVMEM device. It can hold MAC addresses, calibration constants, SoC IDs, revision IDs, part numbers, OTP trim values, or configuration flags.
- NVMEM kernel mechanism:
  - Provider drivers include `<linux/nvmem-provider.h>` and fill `struct nvmem_config`.
  - Consumer drivers include `<linux/nvmem-consumer.h>` and use cell/device APIs.
  - Provider config includes parent device, optional name/id, optional cells, access policy (`read_only`, `root_only`), size, word size, stride, private context, and `reg_read`/`reg_write` callbacks.
  - `devm_nvmem_register()` or `nvmem_register()` creates the `struct nvmem_device` and sysfs binary file.
  - DT subnodes under the provider describe cells with `reg = <offset size>` and optional `bits = <bit-offset bit-count>`.
  - Consumer nodes reference provider cells through `nvmem-cells` plus `nvmem-cell-names`; drivers look them up by name.
  - Consumers use `devm_nvmem_cell_get()`, `nvmem_cell_read()`, `nvmem_cell_write()`, `nvmem_cell_read_u32()`, and related helpers.
  - Sysfs exposes the raw provider binary at `/sys/bus/nvmem/devices/<name>/nvmem`; internal source notes that this exposes the backing memory, not a friendly per-cell userspace API.
- RTC/NVMEM overlap:
  - RTC chips may include EEPROM or battery-backed RAM.
  - The ldd2 source says RTC NVMEM should be registered only after RTC device registration succeeds.
  - Current local headers expose managed RTC NVMEM registration; final docs should avoid stale helper names without caveats.
- GPIO watchdog:
  - ldd2 covers the generic GPIO watchdog driver for external watchdog chips toggled through a GPIO line.
  - Use `linux,wdt-gpio`/GPIO watchdog support instead of userspace GPIO toggling, because it integrates with the watchdog framework and gives standard ABI.

## Source Differences
- Kernel baseline:
  - `ldd2-ch12` and `ldd2-ch13` are written for Linux kernel `v4.19.x`.
  - Local validation used Linux `6.8.0-124-generic` headers, so version drift is expected.
- NVMEM include typo/drift:
  - `ldd2-ch12` early text says consumers include `<linux/nvmemconsumer.h>`, while later text and current headers use `<linux/nvmem-consumer.h>`. Final docs should use `<linux/nvmem-consumer.h>`.
- NVMEM provider callbacks:
  - `ldd2-ch12` states provider `reg_read`/`reg_write` callbacks return the number of bytes read/written on success.
  - Current `include/linux/nvmem-provider.h` keeps `typedef int (*nvmem_reg_read_t)(...)`/`reg_write_t` but common in-tree implementations and core expectations should be validated before giving buildable provider code. Treat the source wording as a version-sensitive claim.
- NVMEM registration APIs:
  - `ldd2-ch12` lists `devm_nvmem_unregister()`. Local Linux 6.8 headers expose `nvmem_unregister()` and `devm_nvmem_register()` but not `devm_nvmem_unregister()`. Final docs should prefer managed registration without manual devm unregister.
  - Current `struct nvmem_config` has newer fields not in ldd2: `add_legacy_fixed_of_cells`, `fixup_dt_cell_info`, `keepout`, `type`, `ignore_wp`, `layout`, `of_node`, `compat`, and `base_dev`.
- RTC NVMEM drift:
  - `ldd2-ch12` names `rtc_nvmem_register()`/`rtc_nvmem_unregister()`.
  - Local Linux 6.8 headers expose `devm_rtc_nvmem_register()`. Final lesson should mention source-era versus current helper naming.
- NVMEM DT binding drift:
  - ldd2 points to old text binding path `Documentation/devicetree/bindings/nvmem/nvmem.txt`.
  - Current kernels use YAML bindings under `Documentation/devicetree/bindings/nvmem/`; final docs should validate exact provider binding for examples.
- NVMEM userspace ABI drift:
  - ldd2 emphasizes the raw `/sys/bus/nvmem/devices/<dev>/nvmem` binary file and says cells are not directly exposed.
  - Current kernel docs mention both device binary access and NVMEM cell sysfs behavior depending on configuration/bindings. Final docs should state the ABI carefully and version-gate any per-cell sysfs claim.
- Watchdog API drift:
  - ldd2 does not emphasize `watchdog_init_timeout()`, but local headers expose it and modern drivers commonly use it to initialize timeout from module parameter/DT/defaults.
  - Local headers include helper flags not highlighted in ldd2, such as `watchdog_stop_on_reboot()`, `watchdog_stop_on_unregister()`, and `watchdog_stop_ping_on_suspend()`.
  - Local `struct watchdog_device` includes `min_hw_heartbeat_ms`/`max_hw_heartbeat_ms`, `groups`, `pm_nb`, and `driver_data`; ldd2 covers an older subset.
  - `struct watchdog_ops.start` remains mandatory in current headers, while most other ops remain optional.
- Watchdog userspace path:
  - ldd2 uses `/dev/watchdog`. Current systems often expose `/dev/watchdog` plus numbered `/dev/watchdogN`; final docs should teach both and prefer checking the actual device node.
- Terminology collision:
  - `ldd1-ch22` uses “watchdog” for network transmit timeout (`dev->watchdog`, `ndo_tx_timeout`). That is unrelated to `drivers/watchdog/` and should be explicitly separated in learner docs.

## Gaps / Uncertainties
- Internal sources do not provide a modern buildable NVMEM provider example using current `struct nvmem_config` fields, YAML bindings, and validated callback return semantics.
- Internal sources do not provide a modern buildable NVMEM consumer example reading a MAC/calibration cell from DT with error handling.
- Internal sources do not deeply cover NVMEM layouts, keepout regions, cell lookup tables, direct `nvmem_device_read()`/`nvmem_device_write()`, write-protect behavior, or security policy for exposing OTP/eFuse contents to userspace.
- Internal sources do not cover common in-tree NVMEM providers such as `at24`, `imx-ocotp`, `qfprom`, or SoC OTP/efuse drivers in detail.
- Internal sources do not provide a modern minimal watchdog platform driver example using `watchdog_init_timeout()`, `watchdog_set_nowayout()`, `watchdog_stop_on_reboot()`, `watchdog_stop_on_unregister()`, and `devm_watchdog_register_device()`.
- Internal sources do not deeply cover watchdog daemon policy, systemd integration, bootloader handoff, panic/reboot sequencing, suspend/resume behavior, or multi-watchdog systems.
- Internal sources do not validate current GPIO watchdog YAML binding path/properties.
- Hardware validation is required for both frameworks:
  - Watchdog expiration/reset, magic close, nowayout, and pretimeout cannot be safely tested on a development host without a controlled target.
  - NVMEM write tests can permanently change EEPROM/OTP/eFuse contents; examples should default to read-only or learning-only fake storage unless target hardware is explicit.

## External Validation
- Used: `https://docs.kernel.org/watchdog/watchdog-kernel-api.html`
  - Purpose: validate current watchdog kernel-side API names, `struct watchdog_device`, `struct watchdog_ops`, helper functions, registration flow, and driver-data/lifetime guidance.
- Used: `https://docs.kernel.org/watchdog/watchdog-api.html`
  - Purpose: validate current userspace watchdog behavior through `/dev/watchdog`, keepalive writes/ioctls, magic close, timeout ioctls, boot/status reporting, and numbered watchdog device nodes.
- Used: `https://docs.kernel.org/driver-api/nvmem.html`
  - Purpose: validate current NVMEM provider/consumer overview, sysfs behavior, cell-based access, and modern NVMEM concepts beyond the v4.19 book baseline.
- Local validation: Linux `6.8.0-124-generic` headers were inspected at `/lib/modules/6.8.0-124-generic/build/include/linux/watchdog.h`, `/lib/modules/6.8.0-124-generic/build/include/uapi/linux/watchdog.h`, `/lib/modules/6.8.0-124-generic/build/include/linux/nvmem-provider.h`, `/lib/modules/6.8.0-124-generic/build/include/linux/nvmem-consumer.h`, and `/lib/modules/6.8.0-124-generic/build/include/linux/rtc.h`.
  - Watchdog headers confirm `struct watchdog_ops`, `struct watchdog_device`, `watchdog_init_timeout()`, `watchdog_register_device()`, `devm_watchdog_register_device()`, `watchdog_set_nowayout()`, `watchdog_stop_on_reboot()`, `watchdog_stop_on_unregister()`, `watchdog_set_drvdata()`, `watchdog_get_drvdata()`, `watchdog_notify_pretimeout()`, `watchdog_set_restart_priority()`, and `WDIOC_GETTIMELEFT`/`WDIOF_PRETIMEOUT`.
  - NVMEM headers confirm `struct nvmem_config`, `nvmem_register()`, `nvmem_unregister()`, `devm_nvmem_register()`, `nvmem_add_cell_table()`, `nvmem_add_one_cell()`, `nvmem_cell_read_u32()`, `nvmem_cell_read_variable_le_u32()`, `nvmem_device_read()`, and `devm_rtc_nvmem_register()`.
- Still needed before learner/example files:
  - Validate exact current NVMEM callback return-value expectations against in-tree provider examples and `drivers/nvmem/core.c`.
  - Validate current DT YAML bindings for `nvmem.yaml`, `nvmem-consumer.yaml`, common EEPROM providers, SoC OTP/efuse providers, and `gpio-wdt.yaml`.
  - Decide whether the example step should use a learning-only software watchdog/NVMEM fake module, DTS snippets around real in-tree providers (`gpio-wdt`, `at24`, SoC OTP), or a README-only lab to avoid unsafe resets/writes.

## Learning Content Brief
- Learning path number: `28`.
- Slug: `watchdog-and-nvmem-frameworks`.
- Topic scope:
  - Watchdog: kernel driver registration, ops, timeout initialization, keepalive/ping, nowayout, pretimeout/governors, restart handling, GPIO watchdog, `/dev/watchdog*`, ioctls, sysfs, common production hazards.
  - NVMEM: provider/consumer split, cells, DT cell bindings, provider callbacks, registration, RTC-backed storage context, consumer APIs, sysfs ABI, read-only/security concerns.
  - Keep raw I2C/SPI transfer details in topics 16-17, MFD child-device creation in topic 19, RTC driver mechanics in topic 27, and networking `ndo_tx_timeout` in network topic 29/30 as applicable.
- Beginner mental model:
  - Watchdog: “a timer that reboots the board if healthy software stops saying I am alive.”
  - NVMEM: “a standard way to expose small nonvolatile bytes like MAC addresses, calibration, OTP trim, and EEPROM data to drivers without every device inventing its own API.”
- Core watchdog mechanism:
  - Probe allocates driver-private state with an embedded `struct watchdog_device`.
  - Driver fills `info`, `ops`, timeout limits/defaults, parent pointer, status flags, and private data.
  - Driver initializes timeout, applies nowayout policy, marks already-running hardware if needed, and registers with `devm_watchdog_register_device()` or `watchdog_register_device()`.
  - Open/write/ioctl/sysfs requests go through watchdog core into driver ops.
  - Remove unregisters the device; if hardware cannot be stopped and still runs, the system may reset unless a new owner keeps pinging it.
- Core NVMEM mechanism:
  - Provider probe configures the real storage device, fills `struct nvmem_config`, and registers it.
  - Provider `reg_read`/`reg_write` callbacks translate generic offset/length requests into bus/MMIO/register operations.
  - DT or driver data defines cells; core turns cell descriptions into consumer-visible `struct nvmem_cell` objects.
  - Consumer probe requests named cells and reads/writes data without knowing where the NVMEM provider lives.
  - Sysfs can expose raw provider bytes; production policy must avoid leaking secrets or allowing unsafe writes.
- Important structs/APIs:
  - Watchdog: `struct watchdog_device`, `struct watchdog_info`, `struct watchdog_ops`, `watchdog_init_timeout()`, `watchdog_set_nowayout()`, `watchdog_stop_on_reboot()`, `watchdog_stop_on_unregister()`, `watchdog_hw_running()`, `watchdog_set_drvdata()`, `watchdog_get_drvdata()`, `watchdog_register_device()`, `devm_watchdog_register_device()`, `watchdog_unregister_device()`, `watchdog_notify_pretimeout()`, `watchdog_set_restart_priority()`, `WDIOC_KEEPALIVE`, `WDIOC_SETTIMEOUT`, `WDIOC_GETTIMELEFT`, `WDIOC_GETBOOTSTATUS`.
  - NVMEM provider: `struct nvmem_config`, `struct nvmem_device`, `struct nvmem_cell_info`, `nvmem_reg_read_t`, `nvmem_reg_write_t`, `nvmem_register()`, `devm_nvmem_register()`, `nvmem_unregister()`, `nvmem_add_cell_table()`, `nvmem_add_one_cell()`, `devm_rtc_nvmem_register()`.
  - NVMEM consumer: `struct nvmem_cell`, `nvmem_cell_get()`, `devm_nvmem_cell_get()`, `nvmem_cell_put()`, `nvmem_cell_read()`, `nvmem_cell_write()`, `nvmem_cell_read_u32()`, `nvmem_cell_read_variable_le_u32()`, `nvmem_device_read()`.
  - DT/ABI: watchdog `linux,wdt-gpio`, NVMEM `nvmem-cells`, `nvmem-cell-names`, provider cell child `reg`, optional `bits`, `/sys/class/watchdog/watchdogN/`, `/dev/watchdogN`, `/sys/bus/nvmem/devices/<provider>/nvmem`.
- Lifecycle/data flow:
  - Watchdog start: userspace opens device or kernel starts hardware; core calls `.start`; system must ping before timeout.
  - Watchdog keepalive: userspace write/ioctl or core worker calls `.ping` or `.start`; hardware counter is refreshed.
  - Watchdog timeout: no keepalive arrives; hardware resets board or pretimeout IRQ fires first.
  - Watchdog pretimeout: IRQ calls `watchdog_notify_pretimeout()`; governor policy may log, panic, or no-op before final reset.
  - NVMEM provider read: consumer asks for cell; core resolves provider and offset/size; core calls provider `reg_read`; consumer gets typed or byte buffer.
  - NVMEM sysfs read/write: userspace reads/writes provider binary attribute; core enforces access/read-only policy then calls provider callbacks.
- Practical examples for later:
  - Watchdog README/DTS lab using `gpio-wdt` on target hardware, with explicit warnings about reset behavior.
  - Learning-only watchdog platform driver skeleton showing `watchdog_device`, ops, timeout init, nowayout, and devm registration without arming real hardware.
  - NVMEM DTS example using an EEPROM/OTP provider with cells for MAC/calibration and a consumer reading them with `devm_nvmem_cell_get()`/`nvmem_cell_read()`.
  - Learning-only fake NVMEM provider backed by memory, marked unsafe for production secrets and not representative of OTP programming rules.
- Common bugs:
  - Forgetting `watchdog_init_timeout()`/timeout limits and accepting impossible timeout values.
  - Not calling `watchdog_set_nowayout()` consistently with module parameter/config policy.
  - Failing to mark already-running bootloader watchdog hardware with `WDOG_HW_RUNNING`.
  - Assuming close stops the watchdog when magic close or nowayout policy says otherwise.
  - Calling watchdog ioctls without checking `watchdog_info.options`.
  - Implementing custom `.ioctl` unnecessarily and breaking core defaults.
  - Using userspace GPIO toggling instead of the GPIO watchdog driver for external watchdog hardware.
  - Defining NVMEM cells in code when board-specific DT cells are the right abstraction.
  - Exposing writable NVMEM sysfs for OTP/eFuse or security-sensitive calibration/secrets.
  - Confusing `reg` address/size in DT cells, bit offsets, endianness, or cell lengths.
  - Treating direct OF property parsing of MAC/calibration data as equivalent to reusable NVMEM provider/consumer design.
- Debugging notes:
  - Watchdog: check `dmesg`, `/dev/watchdog*`, `/sys/class/watchdog/watchdogN/identity`, `timeout`, `timeleft`, `nowayout`, `state`, `bootstatus`, `status`, and pretimeout governor files.
  - Watchdog reset testing: use a controlled target, serial console, boot reason registers, and a known recovery path; never casually test expiry on a shared development host.
  - NVMEM: check `dmesg`, `/sys/bus/nvmem/devices/`, provider name, raw `nvmem` binary size/permissions, DT `nvmem-cells` phandles, `nvmem-cell-names`, provider child cell `reg`, and consumer probe errors.
  - NVMEM data debugging: use `hexdump` carefully for read-only inspection; avoid `echo` writes unless the provider is a scratch EEPROM and write policy is understood.
- Production concerns:
  - Watchdog timeout must account for worst-case boot, firmware handoff, suspend/resume, flash update, and userspace daemon startup time.
  - A production system needs a watchdog owner policy: kernel-managed, userspace daemon, systemd watchdog integration, or hardware always-running path.
  - NVMEM cells may contain unique identity, calibration, keys, trim, or provisioning data; access permissions and write protection are product/security decisions.
  - NVMEM providers must handle bus errors, partial accesses, stride/word-size restrictions, wear/write-cycle timing, OTP irreversibility, and endianness.
- Interview angles:
  - Explain watchdog open/write/magic-close/nowayout behavior and why closing the fd may still reboot the board.
  - Design a minimal watchdog driver and explain which ops are mandatory.
  - Debug a board that reboots 60 seconds after boot because the bootloader left the watchdog running.
  - Explain pretimeout versus timeout and how governors are notified.
  - Explain NVMEM provider versus consumer and why DT cells are preferable to hardcoded offsets in consumers.
  - Read a MAC address/calibration cell from NVMEM and discuss error handling, size checking, and lifetime of the returned buffer.
  - Compare custom EEPROM char devices, direct OF properties, ethtool EEPROM ops, and NVMEM cells.
