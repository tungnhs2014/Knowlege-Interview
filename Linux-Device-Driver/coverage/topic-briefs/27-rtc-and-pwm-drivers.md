# Topic Brief - 27 - RTC And PWM Drivers

## Output Targets
- Knowledge: `knowledge/27-rtc-and-pwm-drivers.md`
- Interview: `interview/27-rtc-and-pwm-drivers.md`
- Example: `examples/27-rtc-and-pwm-drivers/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch18` | `docs/Linux Device Driver Development/Chapter 18-RTC Drivers.md` | read/mapped/covered/merged | Primary RTC source: RTC purpose, hardware clock versus system clock, `struct rtc_time`, `struct rtc_device`, `struct rtc_class_ops`, registration, read/set time, BCD conversion, `rtc_valid_tm()`, epoch quirks, alarms through `struct rtc_wkalrm`, alarm IRQ reporting with `rtc_update_irq()`, wake IRQ setup, `/sys/class/rtc/rtcN`, `/dev/rtc`, `hwclock`, and `CONFIG_RTC_HCTOSYS`. |
| `ldd1-ch19` | `docs/Linux Device Driver Development/Chapter 19-PWM Drivers.md` | read/mapped/covered/merged | Primary PWM source: PWM period/duty-cycle mental model, provider versus consumer split, `struct pwm_chip`, `struct pwm_ops`, `struct pwm_device`, DT `#pwm-cells`, `pwms` and `pwm-names`, PWM controller dummy driver, PWM consumer flow, sysfs ABI under `/sys/class/pwm`, and older `pwm_config()`/`pwm_enable()`/`pwm_disable()` API history. |
| `ldd1-ch03` | `docs/Linux Device Driver Development/Chapter 3-Kernel Facilities and Helper Functions.md` | read/mapped/covered-adjacent | Distinguishes absolute time, handled by RTC hardware, from relative kernel timers, jiffies, hrtimers, and tickless idle. Useful to prevent confusing RTC drivers with timer/PWM generation. |
| `ldd1-ch06` | `docs/Linux Device Driver Development/Chapter 6-The Concept of a Device Tree .md` | read/mapped/covered-adjacent | I2C/SPI DT addressing example with `pcf8523: rtc@68`, `compatible`, and `reg`; useful for RTC client instantiation boundaries. |
| `ldd1-ch07` | `docs/Linux Device Driver Development/Chapter 7-I2C Client Drivers.md` | read/mapped/covered-adjacent | I2C bus source that names RTC chips as common I2C clients and shows a PCF8523 RTC node under an I2C controller. Detailed I2C probe and transfer APIs remain topic 16. |
| `ldd1-ch01` | `docs/Linux Device Driver Development/Chapter 1-Introduction to Kernel Development .md` | read/mapped/incidental | Lists PWM devices and RTCs among framework-owned device types with subsystem allocation/registration APIs. |
| `ldd1-ch11` | `docs/Linux Device Driver Development/Chapter 11-Kernel Memory Management.md` | read/mapped/incidental | Devres overview lists PWMs among framework resources with managed lifetime. Detailed memory/devres behavior remains topic 20. |
| `ldd1-ch20` | `docs/Linux Device Driver Development/Chapter 20-Regulator Framework.md` | read/mapped/related | Regulator consumer binding comparison notes that named `*-supply` resources follow the same idea as PWM consumer binding. Full regulator/PWM-regulator coverage remains topic 23 plus this topic's PWM consumer discussion. |
| `ldd1-ch21` | `docs/Linux Device Driver Development/Chapter 21-Framebuffer Drivers.md` | read/mapped/incidental | Mentions framebuffer backlight device fields; useful only as a possible PWM consumer context. Full framebuffer/display and backlight subsystem details remain topic 29. |
| `ldd2-source-root` | `docs/Linux Device Driver Development 2/` | searched/mapped/gap | No dedicated book-2 RTC/PWM framework chapter found. Relevant RTC/PWM-adjacent material is mapped below. |
| `ldd2-ch03` | `docs/Linux Device Driver Development 2/Chapter 3-MFD_Subsystem_and_Syscon_API.md` | read/mapped/covered-adjacent | MFD/PMIC RTC child examples: DA9055 RTC alarm resources, `platform_get_irq_byname()`, `devm_request_threaded_irq()`, MFD cells with `.of_compatible = "dlg,da9055-rtc"`, subdevice regmap/client setup, and SNVS/DA9062 RTC nodes. Full MFD/syscon remains topic 19. |
| `ldd2-ch12` | `docs/Linux Device Driver Development 2/Chapter 12-NVMEM_Framework.md` | read/mapped/covered-adjacent | RTC-backed NVMEM: RTC devices may include EEPROM or battery-backed RAM; `struct rtc_device` has NVMEM-related fields; RTC NVMEM must be registered after RTC registration; DS1307 example uses RTC NVMEM callbacks. Full NVMEM remains topic 28. |
| `ldd2-ch10` | `docs/Linux Device Driver Development 2/Chapter 10-Linux_Kernel_Power_Management.md` | read/mapped/covered-adjacent | RTC wakealarm suspend test through `/sys/class/rtc/rtc0/wakealarm`; thermal framework mentions PWM-fan as a cooling device; full PM/thermal treatment remains topics 24 and 37 as needed. |
| `ldd2-ch04` | `docs/Linux Device Driver Development 2/Chapter 4-Common_Clock_Framework.md` | read/mapped/related | PWM-clock alternative: DT `compatible = "pwm-clock"`, `pwms = <&pwm3 0 45>`, and using PWM output as a fixed-rate clock source. Full CCF remains topic 22. |
| `notion-source-root` | `docs/Linux-Device-Driver-Notion/` | searched/mapped/gap | No standalone Notion RTC/PWM framework chapter found. Distributed setup, module, timer, DT, I2C/SPI, and pinctrl snippets are mapped below. |
| `notion-ch01-part1` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 1 Environment Setup and Getting Source Code.md` | read/mapped/incidental | Kernel source tree map lists `drivers/rtc/` and `drivers/pwm/`. |
| `notion-ch02-part2` | `docs/Linux-Device-Driver-Notion/Chapter 2-Part 2 Module Dependencies, Loading & Information.md` | read/mapped/incidental | Module examples show `rtc_ds1307` depending on `i2c_core` and being listed in boot-load configuration; useful for module/dependency debugging. |
| `notion-ch03-part5` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 5 Work Queues and Scheduling.md` | read/mapped/covered-adjacent | Contrasts absolute time/RTC with relative timers and includes a software hrtimer GPIO "PWM" example. Use as a cautionary contrast: real PWM-capable hardware should normally use the PWM framework, not ad hoc timer toggling. |
| `notion-ch06-part2` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 2 Device Addressing and Resources.md` | read/mapped/covered-adjacent | I2C DT addressing snippet with PCF8523 RTC at `rtc@68`. Full DT addressing remains topic 10. |
| `notion-ch07-part1` | `docs/Linux-Device-Driver-Notion/Chapter 7-Part 1 I2C Architecture and Driver Structures.md` | read/mapped/covered-adjacent | I2C architecture examples list RTC chips and `rtc-ds1307.c` as I2C client drivers. Full I2C driver coverage remains topic 16. |
| `notion-ch07-part3` | `docs/Linux-Device-Driver-Notion/Chapter 7-Part 3 Device Tree Integration.md` | read/mapped/covered-adjacent | Real-world EEPROM + RTC DT example with `pcf8523: rtc@68`, generic node naming, and `reg = <0x68>`. |
| `notion-ch08-part1` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 1 SPI Architecture and Driver Structures.md` | read/mapped/related | SPI source lists DS3234-style real-time clocks as common SPI devices. Full SPI mechanics remain topic 17. |
| `notion-ch14-part1` | `docs/Linux-Device-Driver-Notion/Chapter 14-Part 1 Pin Control Framework.md` | read/mapped/related | Pinctrl source shows `PWM_OUT` as one possible pin mux mode and reinforces that a pin can only have one active function. Full pinctrl remains topic 13. |

## Source Files Read
- `ldd1-ch18`: complete RTC chapter. Read RTC purpose, framework structs, registration APIs, read/set time examples, BCD conversion, alarm handling, IRQ reporting, wakeup integration, sysfs, `hwclock`, and summary.
- `ldd1-ch19`: complete PWM chapter. Read PWM waveform concepts, controller/provider API, dummy PWM chip driver, DT provider binding, consumer API, DT client binding, sysfs ABI, and summary.
- `ldd1-ch03`: targeted delay/timer section distinguishing absolute time from relative timers, jiffies, hrtimers, and dynamic tick.
- `ldd1-ch06`: targeted I2C/SPI DT addressing example containing `pcf8523: rtc@68`.
- `ldd1-ch07`: targeted I2C device-tree section containing PCF8523 RTC at address `0x68`.
- `ldd1-ch01`: targeted framework-device list containing PWM device and RTC.
- `ldd1-ch11`: targeted devres list containing PWMs as managed framework resources.
- `ldd1-ch20`: targeted regulator binding note comparing regulator consumer names with PWM consumer binding.
- `ldd1-ch21`: targeted framebuffer backlight field snippet; retained only as consumer-context evidence.
- `ldd2-ch03`: targeted MFD RTC resource sections around DA9055 RTC alarm IRQs, subdevice IRQ retrieval, subdevice regmap/client setup, DA9062 RTC node, and SNVS RTC context.
- `ldd2-ch12`: targeted "NVMEM storage in RTC devices" section and DS1307 RTC/NVMEM registration excerpt.
- `ldd2-ch10`: targeted thermal PWM-fan mention and RTC `wakealarm` suspend test.
- `ldd2-ch04`: targeted PWM-clock alternative section.
- `notion-ch01-part1`: targeted source-tree map for `drivers/rtc/` and `drivers/pwm/`.
- `notion-ch02-part2`: targeted `lsmod`/`/proc/modules` examples containing `rtc_ds1307` and module autoload configuration containing `rtc-ds1307`.
- `notion-ch03-part5`: targeted absolute time versus relative timers intro plus hrtimer GPIO software-PWM example.
- `notion-ch06-part2`: targeted I2C DT addressing example containing PCF8523.
- `notion-ch07-part1`: targeted I2C architecture examples listing RTC chips and `rtc-ds1307.c`.
- `notion-ch07-part3`: targeted real-world EEPROM + RTC DT example and node naming explanation.
- `notion-ch08-part1`: targeted SPI common-device list containing real-time clocks.
- `notion-ch14-part1`: targeted pinmux mode list containing `PWM_OUT`.

### Inventory Decisions
- `ldd1-ch18` and `ldd1-ch19` are the only dedicated internal RTC and PWM framework chapters and are primary for topic 27.
- `ldd2` has no dedicated RTC/PWM framework chapter. Its RTC/PWM content is adjacent: MFD-created RTC children, RTC NVMEM, PM wakealarm testing, and PWM-as-clock use. These are not substitutes for `ldd1-ch18`/`ldd1-ch19`.
- Notion has no standalone RTC/PWM framework chapter. Its relevant material is distributed across source-tree orientation, module dependency examples, kernel timer material, I2C/SPI/DT examples, and pinctrl.
- `ldd1-ch10`, `ldd2-ch09`, and Notion V4L2 files have false-positive "capture" or "backlight" hits and are not RTC/PWM framework sources.
- `ldd2-ch05` has many audio "period" and "capture" hits unrelated to PWM period/duty or RTC; excluded after search/mapping.
- `notion-ch03-part3` has generic "periodic polling" workqueue content; not mapped to RTC/PWM beyond the timer distinction already covered by `notion-ch03-part5`.

## Merged Source Notes
- RTCs and PWM controllers share one high-level theme: **do not invent a private ABI for common hardware classes**. Register with the framework so the kernel exposes standard device nodes, sysfs attributes, callbacks, and consumers.
- RTC mental model:
  - RTC hardware tracks wall-clock time while the system is off, often from a battery-backed domain.
  - The Linux system clock is software-maintained time used by system calls and file timestamps.
  - An RTC driver translates chip registers into `struct rtc_time`, provides callbacks through `struct rtc_class_ops`, and reports alarms/periodic/update interrupts to the RTC core.
  - Userspace normally interacts through `/dev/rtcN`, `/sys/class/rtc/rtcN`, `hwclock`, and wakealarm sysfs where supported.
- RTC core mechanisms from sources:
  - Minimal operations are `read_time()` and usually `set_time()`.
  - Alarm-capable devices add `read_alarm()`, `set_alarm()`, and `alarm_irq_enable()`.
  - Alarm IRQ handlers clear/read device status as required by the chip, then notify the core with `rtc_update_irq(rtc, 1, RTC_IRQF | RTC_AF)`.
  - RTCs often store values in BCD; drivers convert with `bcd2bin()` and `bin2bcd()`.
  - `rtc_time` uses `tm_mon` in `[0, 11]`, `tm_wday` in `[0, 6]`, and `tm_year` as years since 1900; many chips use different month/day/year encoding.
  - `rtc_valid_tm()` validates a decoded time before returning it to the core.
  - RTC wakeup requires both alarm support and PM wake IRQ integration, such as `device_init_wakeup()` and `dev_pm_set_wake_irq()`.
- PWM mental model:
  - PWM hardware repeatedly toggles a signal. `period` is one full cycle; `duty_cycle` is the active portion of that cycle; polarity defines which electrical level is active.
  - A PWM controller/provider exports one or more channels through `struct pwm_chip`.
  - A PWM consumer requests a channel and applies a `struct pwm_state` containing period, duty cycle, polarity, and enable state.
  - Device Tree connects consumers to providers with `pwms` and optional `pwm-names`; provider nodes advertise `#pwm-cells`.
- PWM core mechanisms from sources:
  - Book 1 teaches `struct pwm_chip`, `struct pwm_ops`, `pwmchip_add()`, `pwmchip_remove()`, `pwm_get()`, `devm_pwm_get()`, and older `pwm_config()`/`pwm_enable()`/`pwm_disable()`.
  - Current local headers and current kernel docs show the state-based API: `pwm_init_state()`, `pwm_get_state()`, `pwm_apply_might_sleep()`, `pwm_apply_atomic()`, `pwm_might_sleep()`, `pwm_get_args()`, and `pwm_set_relative_duty_cycle()`.
  - Current PWM providers should implement `.apply()` and `.get_state()` instead of legacy split `.config()`, `.enable()`, and `.disable()` hooks.
  - PWM sysfs exposes `/sys/class/pwm/pwmchipN`, `npwm`, `export`, `unexport`, and per-channel `period`, `duty_cycle`, `polarity`, and `enable` files when configured.
- MFD and NVMEM connections:
  - MFD PMICs can instantiate RTC children as platform devices with named IRQ resources such as `ALM` and `TICK`.
  - RTC chips may provide battery-backed NVMEM; RTC NVMEM should be registered only after RTC registration succeeds.
  - Full MFD and NVMEM teaching should stay in topics 19 and 28, but topic 27 should mention these as realistic RTC contexts.

## Source Differences
- `ldd1-ch18` uses `rtc_device_register()` / `devm_rtc_device_register()` as the main registration flow. Local Linux `6.8.0-124-generic` headers also expose newer `devm_rtc_allocate_device()` plus `devm_rtc_register_device()`; final docs should teach the current allocate/fill/register pattern and mention the older helper as source history where useful.
- `ldd2-ch12` names `rtc_nvmem_register()` / `rtc_nvmem_unregister()`. Local Linux `6.8.0-124-generic` headers expose `devm_rtc_nvmem_register()`; final docs should validate current RTC NVMEM helper names before including buildable code.
- `ldd1-ch19` says `pwmchip_remove()` returns `0` or `-EBUSY`; local Linux `6.8.0-124-generic` headers define `void pwmchip_remove(struct pwm_chip *chip)`. Final docs/examples should avoid relying on an integer return on modern targets.
- `ldd1-ch19` centers `pwm_config()`, `pwm_enable()`, and `pwm_disable()` and provider hooks `.config()`, `.enable()`, `.disable()`. Current kernel PWM docs say these consumer helpers are wrappers and should be replaced by `pwm_apply_might_sleep()` when changing multiple parameters; current providers are encouraged to implement `.apply()` and `.get_state()`.
- Current docs.kernel.org PWM documentation for 7.2.0-rc1 mentions `pwmchip_alloc()` / `pwmchip_put()` as the newer controller allocation model, while local Linux `6.8.0-124-generic` headers expose `devm_pwmchip_add()` but not `pwmchip_alloc()`. Final docs should either target the local 6.8 headers or clearly mark newer mainline-only provider allocation drift.
- `notion-ch03-part5` implements a software PWM with an hrtimer and GPIO. That can teach period/duty math, but it should not be presented as the normal Linux PWM framework pattern for real PWM-capable hardware. Use it only as a contrast or caution.
- `ldd1-ch18` mentions `/dev/rtc` symlink and old PC behavior. Current RTC docs distinguish old `/dev/rtc` from portable `/dev/rtcN`; final docs should prefer `/dev/rtcN` and `/sys/class/rtc/rtcN` for embedded work.
- Book and Notion DTS snippets show generic `rtc@68` nodes but do not cover current YAML binding validation or chip-specific required properties.

## Gaps / Uncertainties
- Internal sources do not provide a modern buildable RTC driver using `devm_rtc_allocate_device()` + `devm_rtc_register_device()`.
- Internal sources do not provide a modern buildable PWM provider using `.apply()` / `.get_state()` and state-based `struct pwm_state` semantics.
- Internal sources do not deeply cover RTC ioctl details, selftests, `/proc/driver/rtc`, `wakealarm` corner cases, time64/Y2038 behavior, timezone policy, or how systemd/hwclock synchronize RTC and system time.
- Internal sources do not explain RTC range handling, invalid register values after battery loss, oscillator-stop flags, century bits, 12-hour versus 24-hour mode, or chip-specific alarm mask semantics in depth.
- Internal sources do not cover current RTC DT YAML bindings, `wakeup-source`, or `start-year` / range properties by chip.
- Internal sources do not cover PWM capture, `usage_power`, `pwm_apply_atomic()` constraints beyond current external docs, or exact behavior when hardware cannot realize requested period/duty exactly.
- Internal sources do not cover modern PWM consumer helpers in applied subsystems such as `pwm-backlight`, `pwm-leds`, `pwm-fan`, or `pwm-regulator` beyond mentions.
- Internal sources do not cover sysfs ABI stability/deprecation status for PWM relative to framework consumers; final docs should avoid recommending sysfs as production control when a subsystem driver exists.
- Hardware validation for RTC alarms/wakeup and PWM waveform timing requires real target hardware, an oscilloscope/logic analyzer for PWM, and suspend-capable RTC hardware for wakealarm tests.

## External Validation
- Used: `https://docs.kernel.org/admin-guide/rtc.html`
  - Validated current user-facing RTC framing: RTCs normally track UTC wall-clock time, portable class devices are `/dev/rtcN`, `/sys/class/rtc/rtcN`, and `/proc/driver/rtc`, not every RTC has IRQ/alarm capability, and multiple RTCs can exist on embedded systems.
- Used: `https://docs.kernel.org/driver-api/pwm.html`
  - Validated current PWM consumer guidance: `pwm_get()` / `devm_pwm_get()`, `pwm_apply_might_sleep()`, `pwm_apply_atomic()` gated by `pwm_might_sleep()`, `pwm_get_state()`, `pwm_get_args()`, sysfs files, `.apply()` / `.get_state()` provider preference, locking, and PM responsibility split.
- Local validation: Linux `6.8.0-124-generic` headers were inspected at `/lib/modules/6.8.0-124-generic/build/include/linux/rtc.h` and `/lib/modules/6.8.0-124-generic/build/include/linux/pwm.h`.
  - RTC header exposes `rtc_valid_tm()`, `rtc_tm_to_time64()`, `rtc_time64_to_tm()`, `struct rtc_class_ops`, `devm_rtc_device_register()`, `devm_rtc_allocate_device()`, `devm_rtc_register_device()`, `rtc_update_irq()`, `rtc_update_irq_enable()`, and `devm_rtc_nvmem_register()`.
  - PWM header exposes `struct pwm_state`, `pwm_get_state()`, `pwm_init_state()`, `pwm_get_relative_duty_cycle()`, `pwm_set_relative_duty_cycle()`, `struct pwm_ops` with `.apply()` / `.get_state()`, `pwm_apply_might_sleep()`, `pwm_apply_atomic()`, `pwm_might_sleep()`, `pwmchip_add()`, `pwmchip_remove()` as `void`, `devm_pwmchip_add()`, `pwm_get()`, `devm_pwm_get()`, and compatibility wrappers `pwm_config()`, `pwm_enable()`, `pwm_disable()`.
- Still needed before final learner/example files:
  - Validate current RTC and PWM in-tree examples against the target kernel, such as `drivers/rtc/rtc-ds1307.c`, `drivers/rtc/rtc-pcf8523.c`, `drivers/pwm/`, `drivers/leds/leds-pwm*`, `drivers/video/backlight/pwm_bl.c`, `drivers/hwmon/pwm-fan.c`, and `drivers/regulator/pwm-regulator.c`.
  - Validate current DT YAML bindings for generic RTC, chip-specific RTCs, generic PWM provider binding, `pwm-backlight`, `pwm-leds`, `pwm-fan`, `pwm-regulator`, and `pwm-clock`.
  - Decide whether the example step should include an RTC-only module, a PWM consumer DTS/README lab, or split examples. A single fake module that claims both RTC and PWM may be too artificial.
  - If kernel module code is included later, build-check against local headers and avoid stale `.remove` signatures, stale PWM split hooks, and obsolete RTC registration flow.

## Learning Content Brief
- Learning path number: `27`.
- Slug: `rtc-and-pwm-drivers`.
- Topic scope:
  - RTC framework registration, read/set time callbacks, BCD and `struct rtc_time` conversion, alarms, alarm IRQ reporting, wakealarm/user ABI, and RTC debugging.
  - PWM framework provider and consumer roles, period/duty-cycle/polarity semantics, state-based apply, DT provider/consumer binding, sysfs debug interface, and common PWM consumer choices.
  - Keep I2C/SPI bus transfer details in topics 16-17, MFD/syscon/regmap child-device handoff in topic 19, CCF in topic 22, regulator details in topic 23, runtime/system PM depth in topic 24, and NVMEM in topic 28.
- Beginner mental model:
  - RTC is "the battery-backed wall-clock chip"; the driver translates chip registers into Linux time/alarm operations.
  - PWM is "hardware-controlled on/off timing"; the driver should express period, duty cycle, polarity, and enable state through the PWM framework.
  - RTC is not a kernel timer; PWM is not usually something you should bit-bang with a timer if real PWM hardware exists.
- Core RTC mechanism:
  - Probe gets bus/regmap/IRQ resources and allocates/registers an RTC device.
  - Driver fills `struct rtc_class_ops` callbacks for reading/setting time and optionally alarm operations.
  - `read_time()` reads chip registers, converts BCD/register fields into `struct rtc_time`, adjusts month/day/year conventions, and validates.
  - `set_time()` converts `struct rtc_time` back into chip encoding.
  - Alarm IRQ handler clears/acknowledges chip status and calls `rtc_update_irq()`.
  - Userspace sees `/dev/rtcN` and `/sys/class/rtc/rtcN`; board/kernel policy may use RTC to seed system time.
- Core PWM mechanism:
  - Provider driver describes channels with `struct pwm_chip` and `struct pwm_ops`.
  - Consumer driver gets a channel with `devm_pwm_get()`, initializes a `struct pwm_state`, sets `period`, `duty_cycle`, `polarity`, and `enabled`, then applies it.
  - Device Tree connects consumers through `pwms = <&pwmchip channel period ...>` and optional `pwm-names`.
  - Sysfs can manually export channels for debug/lab work, but production users usually use a subsystem consumer such as backlight, LED, fan, regulator, or clock.
- Important structs/APIs:
  - RTC: `struct rtc_time`, `struct rtc_device`, `struct rtc_class_ops`, `struct rtc_wkalrm`, `devm_rtc_allocate_device()`, `devm_rtc_register_device()`, `devm_rtc_device_register()`, `rtc_valid_tm()`, `rtc_update_irq()`, `rtc_update_irq_enable()`, `bcd2bin()`, `bin2bcd()`, `device_init_wakeup()`, `dev_pm_set_wake_irq()`, `devm_rtc_nvmem_register()`.
  - PWM provider: `struct pwm_chip`, `struct pwm_ops`, `struct pwm_state`, `pwmchip_add()`, `devm_pwmchip_add()`, `pwmchip_remove()`, `.apply()`, `.get_state()`, `.capture()`.
  - PWM consumer: `struct pwm_device`, `devm_pwm_get()`, `pwm_get()`, `pwm_put()`, `pwm_init_state()`, `pwm_get_state()`, `pwm_get_args()`, `pwm_apply_might_sleep()`, `pwm_apply_atomic()`, `pwm_might_sleep()`, `pwm_set_relative_duty_cycle()`, `pwm_get_relative_duty_cycle()`.
  - DT/ABI: `#pwm-cells`, `pwms`, `pwm-names`, `/sys/class/pwm/pwmchipN`, `/sys/class/rtc/rtcN`, `/dev/rtcN`, `wakealarm`, `hwclock`.
- Lifecycle/data flow:
  - RTC probe: allocate state; initialize regmap/bus resources; configure hardware; allocate/register RTC; request alarm IRQ; enable wake capability if supported; optionally register NVMEM after RTC registration.
  - RTC operation: userspace or kernel calls read/set time/alarm; RTC core calls driver ops; driver performs chip-specific conversion and register I/O.
  - RTC interrupt: chip asserts alarm/update/periodic IRQ; driver acknowledges chip; driver calls `rtc_update_irq()`; waiting userspace and wakeup paths are notified.
  - PWM provider probe: get MMIO/clocks/resets/pinctrl; initialize hardware; fill chip/ops; register chip; remove only after consumers are gone.
  - PWM consumer probe: request PWM; initialize state from args; calculate duty from percentage or desired output; apply state; disable or reconfigure on suspend/remove as needed.
- Practical examples for later:
  - RTC learning-only pseudo-driver that implements `read_time()`/`set_time()` over fake registers, validates `rtc_time`, and explains alarm IRQ flow without pretending to wake real hardware.
  - PWM consumer README/DTS lab using `pwm-backlight` or `pwm-leds`, showing period/duty, `/sys/class/pwm` debug, and why production should bind to a real subsystem consumer.
  - Optional tiny PWM consumer module using `devm_pwm_get()`, `pwm_init_state()`, `pwm_set_relative_duty_cycle()`, and `pwm_apply_might_sleep()` if local headers support it.
- Common bugs:
  - Treating RTC time as local time instead of UTC policy.
  - Forgetting BCD conversion or mishandling `tm_mon`, `tm_wday`, and `tm_year`.
  - Failing to validate decoded time after oscillator/battery loss.
  - Enabling an RTC alarm but not requesting/acking the IRQ or not calling `rtc_update_irq()`.
  - Claiming wakealarm support without `wakeup-source`, wake IRQ, or hardware validation.
  - Registering RTC NVMEM before RTC registration succeeds.
  - Using `pwm_config()` then `pwm_enable()` separately in new code when one `pwm_apply_might_sleep()` state update is better.
  - Setting `duty_cycle > period`, confusing percent with nanoseconds, or ignoring polarity.
  - Calling sleepable PWM APIs from atomic context without checking `pwm_might_sleep()`.
  - Relying on disabled PWM output level instead of applying duty 0 with enabled state when a deterministic inactive level is required.
  - Writing a software hrtimer GPIO PWM when a hardware PWM/subsystem binding should be used.
- Debugging notes:
  - RTC: inspect `dmesg`, `/sys/class/rtc/`, `/dev/rtcN`, `/proc/driver/rtc`, `hwclock --show`, `cat /sys/class/rtc/rtc0/date`, `time`, `since_epoch`, `hctosys`, and `wakealarm`.
  - RTC wake: set `wakealarm`, suspend with `/sys/power/state`, confirm interrupt/wakeup source behavior, and check whether the RTC has an IRQ wired.
  - PWM: inspect `/sys/class/pwm/`, export a channel for lab debug, check `period`, `duty_cycle`, `polarity`, and `enable`, and use a scope/logic analyzer for real waveform validation.
  - PWM driver debug: check DT `pwms` specifier, `pwm-names`, provider registration, pinmux, clocks, and whether a consumer framework already owns the channel.
- Production concerns:
  - RTC drivers need chip-specific register-map handling, battery/oscillator status handling, clear error reporting, alarm/wakeup testing, and careful suspend/resume integration.
  - PWM providers need glitch avoidance, atomic state application when hardware permits, clock/rate rounding awareness, correct polarity, and scope-verified output.
  - PWM consumers should normally use standard subsystem bindings (`pwm-backlight`, `pwm-leds`, `pwm-fan`, `pwm-regulator`, `pwm-clock`) instead of custom private control paths.
  - DTS examples must be validated against current YAML bindings before being presented as production-ready.
- Interview angles:
  - Explain RTC hardware clock versus system clock.
  - Decode `struct rtc_time` correctly and explain BCD traps.
  - Describe how an RTC alarm wakes a suspended system.
  - Compare RTC, kernel timers, hrtimers, and PWM.
  - Explain PWM period, duty cycle, polarity, and why `pwm_apply_might_sleep()` is safer than separate config/enable calls.
  - Explain provider versus consumer roles in PWM and how DT connects them.
  - Debug a PWM LED that never changes brightness or an RTC alarm that never fires.
