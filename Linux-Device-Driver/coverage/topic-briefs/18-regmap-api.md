# Topic Brief - 18 - Regmap API

## Output Targets
- Knowledge: `knowledge/18-regmap-api.md`
- Interview: `interview/18-regmap-api.md`
- Example: `examples/18-regmap-api/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch09` | `docs/Linux Device Driver Development/Chapter 9-Regmap API .md` | read/mapped/covered/merged | Primary first-book regmap chapter: motivation, `struct regmap_config`, I2C/SPI initialization, `regmap_read()`, `regmap_write()`, `regmap_update_bits()`, `regmap_multi_reg_write()`, bulk APIs, cache defaults, access tables, and a learning SPI regmap example. |
| `ldd1-ch08` | `docs/Linux Device Driver Development/Chapter 8-SPI Device Drivers.md` | read/mapped/related | SPI chapter summary points forward to regmap as the higher-level abstraction for SPI/I2C register devices. Core SPI details belong to topic 17. |
| `ldd1-ch20` | `docs/Linux Device Driver Development/Chapter 20-Regulator Framework.md` | read/mapped/related | Regulator source notes that many regulator drivers let the regulator core operate through regmap-backed chip register addresses. Applied framework context for topic 23, with a small regmap use-case note for topic 18. |
| `ldd2-ch02` | `docs/Linux Device Driver Development 2/Chapter 2-Regmap_API.md` | read/mapped/covered/merged | Primary second-book regmap chapter: I2C/SPI/MMIO abstraction, `devm_regmap_init_*()`, `reg_stride`, `pad_bits`, locking, `fast_io`, cache/access policy, bulk/multi-write APIs, debugfs register dump, and a large regmap IRQ section that primarily belongs to topic 19. |
| `ldd2-ch03` | `docs/Linux Device Driver Development 2/Chapter 3-MFD_Subsystem_and_Syscon_API.md` | read/mapped/related | Shows regmap as shared infrastructure for MFD cores, regmap IRQ, syscon, and `simple-mfd`. Mostly topic 19, but important as a real regmap design pattern for shared register blocks. |
| `ldd2-ch05` | `docs/Linux Device Driver Development 2/Chapter 5-ASoC_Framework.md` | read/mapped/related | ASoC codec-control source explains regmap's origin/use in codec I/O abstraction. Main content belongs to ASoC topics 35-36. |
| `ldd2-ch07` | `docs/Linux Device Driver Development 2/Chapter 7-Demystifying V4L2 and Video Capture Device Drivers.md` | read/mapped/related | V4L2 sensor-control excerpt uses a sensor `struct regmap *` and `regmap_update_bits()` for image flip/mirror and controls. Applied example for topics 32-34. |
| `ldd2-ch12` | `docs/Linux Device Driver Development 2/Chapter 12-NVMEM_Framework.md` | read/mapped/related | NVMEM/syscon DT excerpt shows `"syscon"` compatibility for MMIO register blocks. Related to topic 19 and topic 28. |
| `ldd2-ch13` | `docs/Linux Device Driver Development 2/Chapter 13-Watchdog_Device_Drivers.md` | read/mapped/related | Watchdog private structure contains `struct regmap *` as applied MMIO/syscon context. Main content belongs to topic 28. |
| `ldd2-ch14` | `docs/Linux Device Driver Development 2/Chapter 14-Linux_Kernel_Debugging_Tips.md` | read/mapped/related | Debugging source lists `regmap` among ftrace event directories. Supports final debugging workflow after current-kernel validation. |
| `notion-ch02-part2` | `docs/Linux-Device-Driver-Notion/Chapter 2-Part 2 Module Dependencies, Loading & Information.md` | read/mapped/related | Notion module-dependency example shows `gpio-mcp23s08.ko` depending on `gpio-regmap.ko`. Related module/autoload evidence, not core regmap API teaching. |
| `notion-ch02-part3` | `docs/Linux-Device-Driver-Notion/Chapter 2-Part 3 Error Handling & Message Printing.md` | read/mapped/related | Error-handling example uses `regmap_init_i2c()`, `IS_ERR()`, `PTR_ERR()`, and `regmap_exit()` cleanup. Useful for regmap probe error-path teaching, though source belongs mainly to topic 04. |
| `notion-ch15-part1` | `docs/Linux-Device-Driver-Notion/Chapter 15-Part 1 GPIO Controller Architecture.md` | read/mapped/related | Contains a device datasheet-style "Register Map" section and GPIO-expander context, not the Linux regmap API. Keep as distinction: hardware register map is not automatically Linux `regmap`. |
| `notion-ch16-part3` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 3 Advanced Topics.md` | read/mapped/related/deferred | Dedicated Notion regmap IRQ section: `struct regmap_irq`, `struct regmap_irq_chip`, `devm_regmap_add_irq_chip()`, `regmap_irq_get_virq()`, MFD child IRQ domains. Defer detail to topic 19. |

## Source Files Read
- `ldd1-ch09`: complete chapter; especially "Programming with the regmap API", "`regmap_config` structure", "regmap initialization", "SPI initialization", "I2C initialization", "Device access functions", "`regmap_update_bits` function", "`regmap_multi_reg_write`", "`regmap_bulk_read()` and `regmap_bulk_write()`", "regmap and cache", "Putting it all together", and "A regmap example".
- `ldd1-ch08`: beginning SPI architecture and summary section that points to regmap as the next abstraction.
- `ldd1-ch20`: regulator provider section around regmap-backed regulator drivers.
- `ldd2-ch02`: complete chapter. For topic 18, use introduction through debugfs register dump. The IRQ-domain, GPIO irqchip, and regmap IRQ sections were read and mapped mainly to topic 19.
- `ldd2-ch03`: syscon/simple-mfd section and regmap-related MFD/regmap IRQ excerpts.
- `ldd2-ch05`: ASoC technical requirements and codec-control I/O section around regmap origin.
- `ldd2-ch07`: V4L2 control-handler excerpt using `regmap_update_bits()`.
- `ldd2-ch12`: syscon-compatible NVMEM DT excerpt.
- `ldd2-ch13`: watchdog private state excerpt containing `struct regmap *`.
- `ldd2-ch14`: tracing event list containing `regmap`.
- `notion-ch02-part2`: `modules.dep` example showing `gpio-regmap.ko`.
- `notion-ch02-part3`: I2C probe error-path example around `regmap_init_i2c()` and `regmap_exit()`.
- `notion-ch15-part1`: MCP23016 register-map hardware overview; inspected to avoid confusing datasheet register maps with the Linux regmap API.
- `notion-ch16-part3`: "Regmap IRQ API" section, including structures and example implementation.

## Merged Source Notes
- Regmap's mental model is consistent across `ldd1-ch09` and `ldd2-ch02`: a driver describes a device's register layout and access rules once, then uses bus-agnostic helpers instead of hand-writing I2C/SPI/MMIO access code.
- `struct regmap` is the runtime map instance. It is normally stored in the driver's private data and passed to all register access helpers.
- `struct regmap_config` is the important driver-authored policy object. Core fields to teach: `reg_bits`, `val_bits`, `reg_stride`, `pad_bits`, `max_register`, `read_flag_mask`, `write_flag_mask`, access callbacks/tables, cache fields, custom `reg_read`/`reg_write`, locking fields, endian fields, and range configuration.
- Access policy can be expressed either by callbacks (`writeable_reg`, `readable_reg`, `volatile_reg`, `precious_reg`) or tables (`wr_table`, `rd_table`, `volatile_table`, `precious_table`). The final docs should teach callback form first, then tables for compact range-based devices.
- A volatile register is not safe to satisfy from cache because hardware may change it independently, or because reading/writing it has special behavior. A precious register should not be casually read for debug/cache reasons because reading may have destructive or side-effect semantics.
- Regmap initialization is bus-specific, while normal access is bus-agnostic. `ldd1` teaches unmanaged `regmap_init_i2c()`/`regmap_init_spi()` plus `regmap_exit()`. `ldd2` adds `devm_regmap_init_i2c()`, `devm_regmap_init_spi()`, `devm_regmap_init_mmio()`, and other managed helpers.
- Prefer devm-managed regmap setup in final beginner examples when the regmap lifetime is exactly the client/platform device lifetime. Keep unmanaged `regmap_init_*()`/`regmap_exit()` as an error-path/lifetime concept.
- MMIO regmap is important in `ldd2`: an MMIO driver maps registers first, then passes the `void __iomem *` base to `devm_regmap_init_mmio()`. For MMIO, `reg_stride` and `fast_io` become more visible.
- Locking is part of the regmap value proposition. By default, regmap serializes accesses. `fast_io` should be reserved for fast non-sleeping register I/O such as MMIO; do not use spinlock-style fast I/O for I2C/SPI transactions that may sleep. `disable_locking` is only safe if the driver provides equivalent external serialization or truly has single-threaded access.
- `regmap_read()` and `regmap_write()` perform alignment/range/access checks, locking, optional cache interaction, flag-mask formatting, and bus/MMIO access. The final docs should present them as "checked register transactions", not just thin wrappers.
- `regmap_update_bits()` performs a read-modify-write for masked fields. The correct mental model is: `new = (old & ~mask) | (val & mask)`. The `mask` selects bits to change; `val` supplies the desired values in those bit positions.
- `regmap_bulk_read()` and `regmap_bulk_write()` are for contiguous register ranges and native register-sized units. `use_single_rw` in old sources maps to modern split settings; validate final field names against the target kernel.
- `regmap_multi_reg_write()` writes an array of `{ register, value, optional delay }` sequences and is suitable for initialization tables that are not a single contiguous block.
- Regcache is a central topic. `cache_type = REGCACHE_NONE` disables caching. Other cache types store register values for faster reads and suspend/resume style flows. `reg_defaults`/`num_reg_defaults` describe power-on defaults and sparse register maps.
- Cache correctness depends on access policy. Volatile registers should not be cached; precious registers should not be dumped/read without intent; default values must match the hardware reset state; cache synchronization needs care after power loss or reset.
- Debugging support appears in `ldd2-ch02`: regmap entries can be inspected through debugfs, for example under `/sys/kernel/debug/regmap/.../registers`, when enabled. Final docs should validate current paths and permissions.
- Applied sources show why regmap matters in real drivers: ASoC codec controls, regulator PMICs, V4L2 sensors, GPIO expanders, watchdog/syscon users, MFD cores, and NVMEM/syscon blocks all benefit from a common register abstraction.

## Source Differences
- `ldd1-ch09` covers I2C/SPI only in its main initialization path. `ldd2-ch02` expands the scope to MMIO and additional bus helpers, making it the better modern conceptual base.
- `ldd1-ch09` uses unmanaged `regmap_init_i2c()`/`regmap_init_spi()` and explicitly calls `regmap_exit()`. `ldd2-ch02` uses devm-managed helpers. Final examples should prefer `devm_regmap_init_i2c()`, `devm_regmap_init_spi()`, and `devm_regmap_init_mmio()` unless demonstrating manual cleanup.
- `ldd1-ch09` says cached writes update the cache and then the hardware. `ldd2-ch02` has wording that suggests cached writes may be cached "instead of" written to hardware. This needs careful external validation before final learner docs; normal regcache behavior is more nuanced because cache-only/bypass/dirty states affect whether hardware is touched.
- `ldd2-ch02` lists `disable_locking` but its prose appears inverted: it says "If false, no locking mechanisms will be used." The field name and current kernel header indicate disabling is controlled when the field is true. Final docs must validate against current `include/linux/regmap.h`.
- `ldd1-ch09` shows `use_single_rw`; current headers split this into `use_single_read` and `use_single_write`. Treat this as API drift.
- `ldd2-ch02` uses a Linux v4.19.X technical baseline. Current kernels have more `regmap_config` fields, including no-increment register callbacks/tables, `reg_update_bits`, raw read/write limits, `can_sleep`, hardware spinlock support, and relaxed MMIO options. Do not copy old struct definitions verbatim.
- `ldd1-ch09` and `ldd2-ch02` both contain extraction/formatting artifacts in code blocks. Final code examples should be rewritten against current headers and checked for compile-shaped syntax.
- `ldd2-ch02` includes a large IRQ-management chapter under the regmap chapter. Learning-path row 18 should cover only core register access/cache/debugfs. Regmap IRQ, MFD, and syscon should be cross-linked but handled in row 19.
- Notion does not have a dedicated core Regmap API chapter. Its regmap mentions are cross-topic examples or regmap IRQ notes. Do not mark Notion as missing; mark it as read/mapped with no core topic-18 equivalent.

## Gaps / Uncertainties
- Need target kernel version before writing final code because `struct regmap_config` fields and helper names drift across kernel releases.
- Internal sources do not deeply cover `regcache_cache_only()`, `regcache_cache_bypass()`, `regcache_mark_dirty()`, `regcache_sync()`, or suspend/resume cache synchronization. These should be externally validated before final knowledge/example files.
- Internal sources mention debugfs register dumps but do not validate current debugfs path layout, required config options, or tracepoint names.
- Internal sources do not teach `regmap_field`/`devm_regmap_field_alloc()`, which is useful for named bitfields in shared registers. Decide whether to include as a production note or defer to advanced examples.
- Internal sources do not deeply cover endian conversions, `reg_format_endian`, `val_format_endian`, or raw/bulk endian expectations.
- Internal sources do not cover no-increment FIFO-style register APIs (`regmap_noinc_read()`/`regmap_noinc_write()`) or paged/indirect register windows in depth.
- Internal sources do not give a production-grade minimal regmap example with current probe prototypes, DT binding, power management, cache sync, and full error handling.
- Learning-path boundaries:
  - Topic 16 owns raw I2C client driver mechanics.
  - Topic 17 owns raw SPI device driver mechanics.
  - Topic 19 owns regmap IRQ, MFD, syscon, and shared register blocks.
  - Topic 23 owns regulator use of regmap.
  - Topic 24 owns suspend/resume and runtime PM depth, including cache-only/sync patterns.
  - Topics 25, 32, and 35 own applied IIO, V4L2, and ASoC examples that may use regmap.

## External Validation
- Used: https://github.com/torvalds/linux/blob/master/include/linux/regmap.h
  - Validates current `struct regmap_config` field drift, current bus init prototypes, managed init helpers, `use_single_read`/`use_single_write`, no-increment callbacks/tables, cache type field, endian fields, and bus callback structure.
- Used: https://docs.kernel.org/driver-api/index.html
  - Confirms current kernel driver API documentation organization, but there is no single comprehensive regmap tutorial page in the browsed docs index.
- Still needed before final knowledge/example files:
  - Current `drivers/base/regmap/` source and tracepoint/debugfs behavior for the target kernel.
  - Current `Documentation/` or in-tree examples for regcache suspend/resume patterns.
  - Current simple I2C, SPI, and MMIO drivers using `devm_regmap_init_*()`, preferably one sensor/PMIC and one platform MMIO driver.
  - Current debug workflow for `/sys/kernel/debug/regmap`, dynamic debug, and ftrace `regmap` events.

## Learning Content Brief
- Topic scope: teach the core Regmap API for register-oriented I2C, SPI, and MMIO devices: setup, access policy, read/write/update, bulk/multi-write, caching, locking, debug, common bugs, and production choices. Mention regmap IRQ/MFD/syscon only as related follow-up topic 19.
- Beginner mental model:
  - Many devices are "a table of registers plus bus rules".
  - Without regmap, every driver repeats register read/write helpers, bit manipulation, range checks, cache rules, and locking.
  - With regmap, the driver describes the register map once and then uses one API regardless of whether the device is behind I2C, SPI, or MMIO.
- Core mechanism:
  - Probe allocates private state and initializes a regmap using the bus-specific helper.
  - `struct regmap_config` tells the core how register addresses and values are formatted, which registers are valid, which registers can be cached, and how locking/access should work.
  - Runtime code calls `regmap_read()`, `regmap_write()`, `regmap_update_bits()`, bulk helpers, or multi-register sequence helpers.
  - Regmap performs validation, locking, optional cache handling, bus formatting, and the actual bus/MMIO operation.
  - Remove is mostly automatic with `devm_regmap_init_*()`, but the driver still owns subsystem unregister ordering and preventing late users from accessing the regmap after teardown.
- Important structs/APIs:
  - Core: `struct regmap`, `struct regmap_config`, `struct regmap_access_table`, `struct regmap_range`, `struct reg_default`, `struct reg_sequence`, `enum regcache_type`.
  - Init: `devm_regmap_init_i2c()`, `devm_regmap_init_spi()`, `devm_regmap_init_mmio()`, unmanaged `regmap_init_i2c()`, `regmap_init_spi()`, `regmap_exit()`.
  - Access: `regmap_read()`, `regmap_write()`, `regmap_update_bits()`, `regmap_bulk_read()`, `regmap_bulk_write()`, `regmap_multi_reg_write()`.
  - Policy fields: `reg_bits`, `val_bits`, `reg_stride`, `pad_bits`, `max_register`, `read_flag_mask`, `write_flag_mask`, `writeable_reg`, `readable_reg`, `volatile_reg`, `precious_reg`, `wr_table`, `rd_table`, `volatile_table`, `precious_table`, `reg_defaults`, `num_reg_defaults`, `cache_type`, `fast_io`, `disable_locking`, `lock`, `unlock`, `reg_read`, `reg_write`, `reg_format_endian`, `val_format_endian`, `ranges`.
  - Cache/debug, after validation: `regcache_mark_dirty()`, `regcache_sync()`, `regcache_cache_only()`, debugfs regmap register dumps, ftrace regmap events.
- Lifecycle/data flow:
  - Driver bind: I2C/SPI/platform probe is called after device matching.
  - Resource setup: enable regulators/clocks/reset as needed, map MMIO if applicable, allocate private data.
  - Regmap setup: fill static `regmap_config`, call `devm_regmap_init_i2c()`/`spi()`/`mmio()`, check `IS_ERR()`/`PTR_ERR()`, store the map in private data.
  - Hardware init: optionally write register defaults or patch sequences with `regmap_multi_reg_write()`, set important bits with `regmap_update_bits()`, and register with the relevant subsystem.
  - Runtime: subsystem callbacks read/write registers through the map; regmap serializes and validates access.
  - Power transitions: if caching is used, mark dirty/sync/bypass/cache-only according to actual suspend/reset behavior after external validation.
  - Remove: unregister users first, stop work/IRQs, then let devm release the map.
- Examples to produce later:
  - I2C register device using `devm_regmap_init_i2c()`, 8-bit register/8-bit value config, `max_register`, readable/writable/volatile callbacks, and `regmap_update_bits()`.
  - SPI register device using `devm_regmap_init_spi()`, `read_flag_mask`/`write_flag_mask`, and a multi-register initialization sequence.
  - MMIO platform device using `devm_platform_ioremap_resource()` plus `devm_regmap_init_mmio()`, `reg_stride = 4`, `val_bits = 32`, and `fast_io = true` only if appropriate.
  - Debug workflow showing dynamic debug, debugfs regmap dump, and tracepoints after validation.
- Common bugs:
  - Copying stale `struct regmap_config` fields from old material without checking the target kernel.
  - Using raw I2C/SPI helpers for a register-heavy chip and then duplicating locking, cache, and update-bit logic badly.
  - Wrong `reg_bits`, `val_bits`, `reg_stride`, endian setting, or read/write flag mask, causing every bus transaction to be malformed.
  - Misusing `regmap_update_bits()` by putting unshifted field values into `val`; `val` must already be positioned under `mask`.
  - Marking status/FIFO/clear-on-read registers as cacheable.
  - Forgetting `volatile_reg` for hardware-updated status registers.
  - Forgetting `precious_reg` for destructive read or sensitive registers that should not be debug-dumped casually.
  - Using `fast_io` for I2C/SPI or other sleeping buses.
  - Disabling regmap locking without external serialization.
  - Trusting `reg_defaults` that do not match silicon reset state or bootloader-modified hardware.
  - Reading debugfs regmap dumps on registers with side effects if access policy is wrong.
  - Returning the wrong errno path from failed regmap initialization or leaking unmanaged regmaps on later probe failure.
- Debugging notes:
  - First check probe failure: helper used for the correct bus, `IS_ERR()` return handled, MMIO region mapped, clocks/power/reset ready before first access.
  - If reads/writes fail with `-EINVAL`, inspect `reg_stride` alignment and invalid arguments.
  - If reads/writes fail with `-EIO`, inspect `max_register`, readable/writable callbacks, access tables, and bus errors.
  - If values are wrong, inspect endian settings, address/value widths, SPI read/write flag masks, I2C register address width, and datasheet reset values.
  - If cached values are stale, inspect `volatile_reg`, cache dirty/sync behavior, power/reset sequencing, and whether hardware changed registers outside regmap.
  - Use debugfs regmap dumps only after access policy is correct and only when the target registers are safe to read.
  - Use dynamic debug and regmap tracepoints after validating current kernel support.
- Production concerns:
  - Prefer `devm_regmap_init_*()` for normal device-owned lifetime, but explicitly order subsystem unregister, IRQ/work cancellation, and PM transitions.
  - Keep `regmap_config` static/const where possible and tied to the actual datasheet register layout.
  - Treat the datasheet's "register map" as the source of truth for width, valid ranges, volatile/status bits, write-one-to-clear bits, and reset defaults.
  - Choose cache only when the device benefits from it and the driver can keep it coherent across reset, suspend, runtime PM, and out-of-band hardware changes.
  - Keep raw bus access only for unusual protocols, streaming FIFOs, vendor transactions, or operations regmap cannot model cleanly.
  - Add clear comments around registers with side effects, write-one-to-clear behavior, or cache exceptions.
  - Validate against current kernel headers before writing examples, because regmap evolves quietly.
- Interview angles:
  - Explain what problem regmap solves compared with open-coded I2C/SPI register helpers.
  - Explain `struct regmap` vs `struct regmap_config`.
  - Explain why init is bus-specific but access is bus-agnostic.
  - Explain `reg_bits`, `val_bits`, `reg_stride`, `max_register`, and flag masks.
  - Explain `readable_reg`, `writeable_reg`, `volatile_reg`, and `precious_reg`.
  - Explain when to use callbacks versus access tables.
  - Explain `regmap_update_bits()` and the mask/value rule.
  - Explain bulk read/write versus multi-register write.
  - Explain when caching helps and when it is dangerous.
  - Explain why I2C/SPI regmap access can sleep and why `fast_io` is not for slow buses.
  - Explain devm-managed versus unmanaged regmap lifetime.
  - Explain why regmap IRQ/MFD/syscon are related but separate from the core regmap API.
