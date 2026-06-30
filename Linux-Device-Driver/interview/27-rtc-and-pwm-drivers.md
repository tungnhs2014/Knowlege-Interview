# 27 - RTC And PWM Drivers Interview Questions

Strong candidates can separate wall-clock time, kernel timers, and PWM waveforms. They can explain how RTC drivers translate chip registers into `struct rtc_time`, how alarms and wakeup work, how PWM providers and consumers are connected, and where real boards fail around BCD, IRQs, pinmux, context, lifetime, and stale APIs.

## Beginner

### 1. What problem does the RTC framework solve?

**Level:** Beginner

**Short Answer:**  
It gives real-time clock chips a standard kernel and userspace interface for reading/setting wall-clock time and, when supported, alarms.

**Deep Explanation:**  
RTC chips are hardware clocks, often battery-backed, that keep calendar time while the main system is off. Each chip has its own register layout, encoding, year range, and alarm behavior. The RTC framework lets each driver translate that chip-specific behavior into standard callbacks. Userspace then uses `/dev/rtcN`, `/sys/class/rtc/rtcN`, `hwclock`, and `wakealarm` instead of a private driver-specific ABI.

**API / Code Anchor:**  
`struct rtc_device`, `struct rtc_class_ops`, `struct rtc_time`, `devm_rtc_allocate_device()`, `devm_rtc_register_device()`, `/dev/rtcN`, `/sys/class/rtc/rtcN`.

**Production or Debugging Angle:**  
If a board has no `/dev/rtcN` or `/sys/class/rtc/rtcN`, first check whether the bus device probed, the compatible string matched, and the RTC device was registered.

**Common Traps:**  
- Treating the RTC as a kernel timer.
- Exposing custom sysfs files instead of using the RTC class.
- Assuming every RTC has alarm or IRQ support.

**Follow-up Questions:**  
- How is the RTC hardware clock different from the Linux system clock?
- What userspace tool commonly reads or sets RTC time?
- Why might an embedded board have more than one RTC?

### 2. What is the difference between RTC time and system time?

**Level:** Beginner

**Short Answer:**  
RTC time comes from hardware calendar registers; system time is Linux's software-maintained clock after boot.

**Deep Explanation:**  
The RTC persists across power loss, often using a coin cell or always-on power domain. Linux system time is maintained by kernel timekeeping after boot and is used for timestamps, timers, and userspace time APIs. During boot, the kernel or userspace may use an RTC to initialize system time. Later, userspace may write corrected time back to the RTC.

**API / Code Anchor:**  
`/sys/class/rtc/rtcN/hctosys`, `hwclock --show`, `hwclock --systohc`, `struct rtc_time`, `rtc_tm_to_time64()`.

**Production or Debugging Angle:**  
Wrong boot time may come from a bad RTC value, a dead battery, an oscillator-stop flag, or selecting the wrong RTC as the system time source.

**Common Traps:**  
- Storing local time in the RTC without a clear product policy.
- Assuming `date` and `hwclock` must always match exactly.
- Forgetting that system time can be corrected by NTP after boot.

**Follow-up Questions:**  
- Why is UTC normally preferred for RTC storage?
- What does `hctosys` tell you?
- What happens when the RTC battery is lost?

### 3. What is `struct rtc_time`, and what are its common traps?

**Level:** Beginner

**Short Answer:**  
`struct rtc_time` is the broken-down time format used by RTC callbacks, with C-library style fields such as month starting at zero and year counted since 1900.

**Deep Explanation:**  
An RTC driver reads chip registers and fills `struct rtc_time`. The fields do not all match human date notation. January is `tm_mon = 0`, not `1`. Year 2026 is `tm_year = 126`, not `2026` or `26`. Many chips store values in BCD, so a register value `0x25` means decimal 25, not binary 37. Drivers must convert, adjust, and validate before returning the time.

**API / Code Anchor:**  
`struct rtc_time`, `bcd2bin()`, `bin2bcd()`, `rtc_valid_tm()`, `rtc_time64_to_tm()`, `rtc_tm_to_time64()`.

**Production or Debugging Angle:**  
If date/month/year is off by one month, 1900 years, 100 years, or nonsense values appear after battery loss, inspect register decoding and `rtc_valid_tm()` usage.

**Common Traps:**  
- Forgetting `tm_mon` is zero-based.
- Forgetting `tm_year` is years since 1900.
- Returning unvalidated data after oscillator stop.
- Treating BCD as normal binary.

**Follow-up Questions:**  
- Why do many RTC drivers use `bcd2bin()`?
- What should `read_time()` return for invalid chip contents?
- How would you handle a chip that stores only two year digits?

### 4. What problem does the PWM framework solve?

**Level:** Beginner

**Short Answer:**  
It standardizes access to PWM controllers so consumers can request channels and configure period, duty cycle, polarity, and enable state without knowing controller registers.

**Deep Explanation:**  
PWM hardware produces repeated active/inactive output cycles. Many devices consume PWM: backlights, LEDs, fans, regulators, buzzers, and sometimes clock inputs. The PWM framework separates the provider driver that owns hardware from the consumer driver that uses a channel. Device Tree or lookup data connects them.

**API / Code Anchor:**  
`struct pwm_chip`, `struct pwm_ops`, `struct pwm_device`, `struct pwm_state`, `devm_pwm_get()`, `pwm_apply_might_sleep()`.

**Production or Debugging Angle:**  
A display backlight driver should not program SoC PWM registers directly. It should request a PWM and apply a state, or use an existing binding such as `pwm-backlight`.

**Common Traps:**  
- Writing private PWM control instead of using the framework.
- Confusing PWM with kernel timers.
- Forgetting that pinmux and controller clocks must be configured.

**Follow-up Questions:**  
- What is the difference between a PWM provider and consumer?
- What kinds of devices commonly consume PWM?
- How does Device Tree connect a consumer to a provider?

### 5. Explain PWM period, duty cycle, and polarity.

**Level:** Beginner

**Short Answer:**  
Period is the full cycle length, duty cycle is the active portion of the cycle, and polarity defines whether active means high or low.

**Deep Explanation:**  
If a PWM period is 20 ms and duty cycle is 5 ms, the output is active for 25% of each cycle. With normal polarity, active usually means high. With inverted polarity, the electrical high/low meaning flips. Consumers should express the desired state in nanoseconds and polarity, then let the provider program hardware as closely as possible.

**API / Code Anchor:**  
`struct pwm_state.period`, `struct pwm_state.duty_cycle`, `struct pwm_state.polarity`, `PWM_POLARITY_NORMAL`, `PWM_POLARITY_INVERSED`.

**Production or Debugging Angle:**  
If brightness or fan speed behaves backward, check polarity and board-level inversion before changing duty math.

**Common Traps:**  
- Supplying percentages where nanoseconds are expected.
- Setting `duty_cycle > period`.
- Assuming disabled output level is always inactive.

**Follow-up Questions:**  
- What is 50% duty for a 10 ms period?
- Why might hardware not produce the exact requested period?
- What does inverted polarity change?

## Mid-level

### 6. Walk through the probe flow for a simple RTC driver.

**Level:** Mid-level

**Short Answer:**  
Probe gets chip resources, initializes private state, allocates/registers an RTC device, assigns RTC ops, requests alarm IRQ if present, and enables wake support only if hardware and board wiring support it.

**Deep Explanation:**  
An RTC driver is usually an I2C, SPI, platform, or MFD child driver. Probe sets up bus access or regmap, checks status flags, allocates private state, creates the RTC class device, and registers callbacks. If the chip supports alarms, the driver wires alarm callbacks and requests the IRQ. If the alarm can wake the system, it also configures PM wakeup.

**API / Code Anchor:**  
`devm_rtc_allocate_device()`, `devm_rtc_register_device()`, `struct rtc_class_ops`, `devm_request_threaded_irq()`, `device_init_wakeup()`, `dev_pm_set_wake_irq()`.

**Production or Debugging Angle:**  
Requesting an IRQ before the `rtc` pointer and private state are initialized can create a race. Registering NVMEM or wake support before the core RTC object exists can also produce fragile ordering.

**Common Traps:**  
- Assuming registration alone makes wakealarm work.
- Ignoring oscillator-stop flags during probe.
- Not handling `-EPROBE_DEFER` for parent regmap, clocks, or IRQ resources.

**Follow-up Questions:**  
- Which callbacks are minimally useful for an RTC?
- When would an RTC be an MFD child device?
- Where should RTC-backed NVMEM registration happen?

### 7. How should `read_time()` and `set_time()` work?

**Level:** Mid-level

**Short Answer:**  
`read_time()` reads chip registers, converts fields into `struct rtc_time`, validates them, and returns. `set_time()` converts `struct rtc_time` back into the chip's register encoding and writes it.

**Deep Explanation:**  
The RTC core does not know chip register layout. The driver must mask status bits, convert BCD fields, handle 12-hour or 24-hour mode, adjust month and year conventions, and reject invalid data. `set_time()` performs the reverse conversion and may need to stop/update/restart the oscillator depending on chip requirements.

**API / Code Anchor:**  
`.read_time`, `.set_time`, `regmap_bulk_read()`, `regmap_bulk_write()`, `bcd2bin()`, `bin2bcd()`, `rtc_valid_tm()`.

**Production or Debugging Angle:**  
Wrong decoded fields show up as off-by-one month, wrong century, impossible dates, or time jumping after reboot. Add targeted dev_dbg logs around raw register values during bring-up.

**Common Traps:**  
- Returning success when the chip says the time is invalid.
- Forgetting to mask control bits from time registers.
- Mishandling chips with 12-hour mode or century bits.

**Follow-up Questions:**  
- Why validate after conversion, not before?
- What should happen if the chip has lost oscillator state?
- Why might `tm_wday` need chip-specific handling?

### 8. How does an RTC alarm interrupt reach userspace or wake the system?

**Level:** Mid-level

**Short Answer:**  
The driver programs alarm registers, enables the alarm IRQ, handles the interrupt by acknowledging the chip, and calls `rtc_update_irq()` with alarm flags.

**Deep Explanation:**  
Userspace can set an alarm through RTC interfaces such as `wakealarm`. The RTC core calls the driver's alarm callbacks. At alarm time, the hardware asserts an IRQ. The driver must read and clear the hardware alarm status, then notify the RTC core. For suspend wake, the device must also be configured as a wake source and the IRQ path must be wake-capable.

**API / Code Anchor:**  
`.read_alarm`, `.set_alarm`, `.alarm_irq_enable`, `struct rtc_wkalrm`, `rtc_update_irq(rtc, 1, RTC_IRQF | RTC_AF)`, `/sys/class/rtc/rtcN/wakealarm`.

**Production or Debugging Angle:**  
For a failed wakealarm, check `/proc/interrupts`, wakeup sources, DT interrupts, `wakeup-source`, and whether the driver clears status before or after notifying the core as required by the chip.

**Common Traps:**  
- Programming alarm registers but never enabling the IRQ.
- Clearing chip status but not calling `rtc_update_irq()`.
- Calling `rtc_update_irq()` but not clearing the hardware flag, causing an interrupt storm.
- Claiming wake support when the IRQ cannot wake the SoC.

**Follow-up Questions:**  
- What flags indicate an alarm interrupt?
- Why can an RTC have alarm support but no system wake support?
- How would you test wakealarm from the shell?

### 9. Walk through the flow for a PWM consumer driver.

**Level:** Mid-level

**Short Answer:**  
The consumer requests a PWM, initializes a state, sets period/duty/polarity/enabled, and applies the state with `pwm_apply_might_sleep()`.

**Deep Explanation:**  
The consumer should not touch provider registers. It uses `devm_pwm_get()` with a connection ID that matches firmware data, often `pwm-names`. `pwm_init_state()` starts from firmware/provider defaults. The driver modifies the state and applies it in one operation so the provider can update hardware coherently.

**API / Code Anchor:**  
`devm_pwm_get(dev, "output")`, `pwm_init_state()`, `pwm_set_relative_duty_cycle()`, `pwm_apply_might_sleep()`, `pwms`, `pwm-names`.

**Production or Debugging Angle:**  
If `devm_pwm_get()` returns `-EPROBE_DEFER`, the provider may not be ready. If it returns `-ENOENT`, check the DT property name, `pwm-names`, and consumer binding.

**Common Traps:**  
- Calling `pwm_config()` and `pwm_enable()` separately in new code.
- Forgetting to handle probe deferral.
- Recomputing duty in nanoseconds incorrectly.
- Using sysfs as the product control path instead of a proper subsystem consumer.

**Follow-up Questions:**  
- Why call `pwm_init_state()`?
- When would you use `pwm_get_args()`?
- How do you represent a PWM channel in Device Tree?

### 10. What does a PWM provider driver implement?

**Level:** Mid-level

**Short Answer:**  
It registers a `struct pwm_chip` and implements `struct pwm_ops` callbacks that translate requested PWM states into controller register programming.

**Deep Explanation:**  
The provider owns hardware: clocks, resets, pinctrl, registers, runtime PM, and channel limits. Modern providers should implement `.apply()` so period, duty, polarity, and enable can be changed as one state. `.get_state()` lets the framework inspect current hardware state. The provider should hide rounding and hardware quirks behind the framework API as much as possible.

**API / Code Anchor:**  
`struct pwm_chip`, `struct pwm_ops`, `.apply`, `.get_state`, `pwmchip_add()`, `devm_pwmchip_add()`, `pwmchip_remove()`.

**Production or Debugging Angle:**  
If `/sys/class/pwm/pwmchipN` never appears, check provider probe, clocks, resets, compatible string, and registration return values.

**Common Traps:**  
- Implementing only legacy `.config`, `.enable`, and `.disable` in new code.
- Forgetting controller clock preparation.
- Not documenting rounding when hardware cannot realize requested periods.
- Removing or disabling hardware while consumers still depend on it.

**Follow-up Questions:**  
- Why is `.apply()` preferred?
- What should `.get_state()` report after boot?
- How do provider and consumer drivers stay decoupled?

### 11. How do you debug a PWM that has no signal on the output pin?

**Level:** Mid-level

**Short Answer:**  
Check provider registration, consumer binding, pinmux, clocks/resets, enable state, duty/period values, and the physical signal with a scope or logic analyzer.

**Deep Explanation:**  
PWM has both software and hardware paths. The kernel may have a valid PWM object while the board pin is still muxed as GPIO or the controller clock is disabled. Conversely, sysfs may show values that were requested but the hardware may round, invert, or fail due to pinctrl or power state. A real waveform check is often required.

**API / Code Anchor:**  
`/sys/class/pwm/pwmchipN`, `npwm`, `export`, `period`, `duty_cycle`, `polarity`, `enable`, pinctrl debugfs, `pwm_apply_might_sleep()`.

**Production or Debugging Angle:**  
Manual sysfs export is useful in a lab, but production should usually use `pwm-backlight`, `pwm-leds`, `pwm-fan`, `pwm-regulator`, or a real consumer driver.

**Common Traps:**  
- Measuring the wrong pin.
- Forgetting pinctrl default state.
- Using a duty cycle of zero and expecting a waveform.
- Ignoring polarity inversion on the board.

**Follow-up Questions:**  
- What does `npwm` tell you?
- Why can sysfs values be misleading?
- What debugfs area helps inspect pin muxing?

## Senior

### 12. A board wakes from RTC alarm on one kernel but not another. How do you approach it?

**Level:** Senior

**Short Answer:**  
Separate alarm programming from system wake configuration, then verify IRQ wiring, wake source registration, suspend state, driver callbacks, and kernel-version API differences.

**Deep Explanation:**  
An RTC alarm can work while the system is running but still fail to wake from suspend. The wake path requires an alarm-capable RTC, a wired IRQ, a correctly described interrupt, PM wake capability, and suspend state support. Kernel changes can affect helper names, IRQ wake handling, or driver ordering. The investigation should prove each layer rather than guessing.

**API / Code Anchor:**  
`.set_alarm`, `.alarm_irq_enable`, `rtc_update_irq()`, `device_init_wakeup()`, `dev_pm_set_wake_irq()`, `/sys/class/rtc/rtc0/wakealarm`, `/sys/kernel/debug/wakeup_sources`.

**Production or Debugging Angle:**  
Test in stages: confirm alarm IRQ while running, then confirm wake source accounting, then test suspend. Check `dmesg`, `/proc/interrupts`, wakeup debugfs, and board schematics for wake-capable IRQ routing.

**Common Traps:**  
- Assuming a working alarm means suspend wake must work.
- Testing the wrong RTC node.
- Forgeting that not all suspend states keep the RTC/wake domain alive.
- Missing a DTS `wakeup-source` or wake IRQ property required by that platform.

**Follow-up Questions:**  
- What would you log in the IRQ handler?
- How would you distinguish driver bug from board wiring issue?
- What changes when the RTC is inside an MFD/PMIC?

### 13. Why is state-based PWM application safer than split config/enable calls?

**Level:** Senior

**Short Answer:**  
One state update lets the provider apply period, duty, polarity, and enable coherently, reducing transient glitches and stale intermediate states.

**Deep Explanation:**  
Older consumer code often configured period/duty and then separately enabled the PWM. That can expose intermediate states: old polarity with new duty, enabled output before period is stable, or disabled output levels that are not deterministic. `struct pwm_state` expresses the desired final state, and `.apply()` lets the provider program hardware in the safest order it supports.

**API / Code Anchor:**  
`struct pwm_state`, `pwm_init_state()`, `pwm_apply_might_sleep()`, `pwm_apply_atomic()`, `pwm_might_sleep()`, provider `.apply()`.

**Production or Debugging Angle:**  
Backlights, regulators, and fans can show visible flicker, voltage glitches, or audible artifacts if updates are not coherent.

**Common Traps:**  
- Blindly converting old code without understanding sleep context.
- Calling `pwm_apply_atomic()` on a provider that might sleep.
- Assuming the provider can avoid all glitches on all hardware.

**Follow-up Questions:**  
- When is `pwm_apply_atomic()` allowed?
- What does `pwm_might_sleep()` tell you?
- How should a provider handle impossible period/duty requests?

### 14. How would you design a driver for a chip that has both RTC and NVMEM?

**Level:** Senior

**Short Answer:**  
Keep RTC time/alarm support in the RTC framework and expose battery-backed memory through NVMEM after the RTC device is registered, using shared register access carefully.

**Deep Explanation:**  
Some RTC chips include EEPROM or battery-backed RAM. The RTC part should not expose ad hoc memory files, and the NVMEM part should not bypass RTC lifetime ordering. The driver should use a common regmap or bus lock, register RTC functionality first, then register NVMEM cells/provider support with current helpers. Access serialization matters because time registers and memory windows may share bus transactions or control bits.

**API / Code Anchor:**  
`struct rtc_device`, `struct rtc_class_ops`, `devm_rtc_register_device()`, `devm_rtc_nvmem_register()`, `struct nvmem_config`, regmap APIs.

**Production or Debugging Angle:**  
Review probe ordering and remove/error paths. If NVMEM appears but RTC registration failed, the driver has created a confusing partial device.

**Common Traps:**  
- Registering NVMEM before RTC registration succeeds.
- Treating NVMEM as RTC private sysfs.
- Ignoring locking around shared register pages.
- Using helper names from a different kernel version.

**Follow-up Questions:**  
- Why does NVMEM belong in a separate framework?
- What should happen if NVMEM registration fails after RTC registration?
- How do MFD and regmap change this design?

### 15. A PWM LED works through sysfs, but the product driver cannot get the PWM. What do you check?

**Level:** Senior

**Short Answer:**  
Check ownership, Device Tree binding, `pwm-names`, provider readiness, probe deferral, and whether sysfs testing consumed the channel.

**Deep Explanation:**  
PWM channels are exclusive resources. If a lab export owns the PWM, a real consumer may fail. If the consumer's `devm_pwm_get(dev, "led")` does not match `pwm-names`, lookup fails. If the provider has not probed, the consumer should defer. If a standard subsystem such as `pwm-leds` already owns the channel, a custom driver should not also request it.

**API / Code Anchor:**  
`devm_pwm_get()`, `-EPROBE_DEFER`, `-ENOENT`, `pwms`, `pwm-names`, `/sys/class/pwm/pwmchipN/export`, `/sys/class/pwm/pwmchipN/unexport`.

**Production or Debugging Angle:**  
Always unexport lab sysfs PWMs before testing the product driver. Inspect `dmesg` for probe deferral and binding messages.

**Common Traps:**  
- Hardcoding the wrong connection ID.
- Forgetting that sysfs export is a consumer.
- Duplicating a channel between `pwm-leds` and a custom driver.
- Assuming `pwmchip0` numbering is stable across boots.

**Follow-up Questions:**  
- How do you make a custom consumer coexist with standard bindings?
- Why should tests avoid hardcoding `pwmchip0`?
- What return codes from `devm_pwm_get()` are most useful?

### 16. How do RTC/PWM APIs differ across kernel versions, and how do you keep code review-safe?

**Level:** Senior

**Short Answer:**  
Check the target kernel headers and in-tree examples. RTC registration, RTC NVMEM helpers, PWM provider ops, PWM chip allocation, and `pwmchip_remove()` signatures have changed across kernels.

**Deep Explanation:**  
Driver code often lives in vendor kernels, LTS kernels, and upstream kernels with different API shapes. Older RTC examples may use one-shot registration helpers; newer code may allocate then register. Older PWM examples may use split config/enable callbacks; newer code should use state-based APIs. Even function return types can differ. Review should target the actual kernel tree being built, not a remembered API.

**API / Code Anchor:**  
`devm_rtc_device_register()`, `devm_rtc_allocate_device()`, `devm_rtc_register_device()`, `devm_rtc_nvmem_register()`, `pwm_config()`, `pwm_enable()`, `pwm_apply_might_sleep()`, `.apply`, `.get_state`, `pwmchip_remove()`.

**Production or Debugging Angle:**  
Build failures from stale APIs are easy to catch. Behavioral bugs from stale patterns, such as split PWM updates or wrong wake setup, require deeper review.

**Common Traps:**  
- Copying examples from a different kernel without checking headers.
- Assuming compatibility wrappers mean preferred modern style.
- Ignoring provider-side API drift.
- Treating docs for a future mainline version as valid for an older LTS tree.

**Follow-up Questions:**  
- Which files would you inspect before writing PWM provider code?
- How would you modernize an old PWM consumer?
- What should a review comment say about stale helper usage?

### 17. Debug scenario: RTC reports impossible dates after battery replacement. What is your answer?

**Level:** Senior

**Short Answer:**  
Suspect invalid hardware state, oscillator-stop/battery flags, BCD decoding, field masks, and missing validation before blaming userspace.

**Deep Explanation:**  
After battery loss, many RTCs preserve random register values or set a validity flag. A correct driver should detect invalid state, avoid returning nonsense as valid time, and report an error or require time initialization. It should mask control bits from time registers, convert BCD correctly, adjust month/year fields, and call `rtc_valid_tm()`.

**API / Code Anchor:**  
`.read_time`, `bcd2bin()`, `rtc_valid_tm()`, `dev_warn()`, `hwclock --show`, `/sys/class/rtc/rtcN/time`.

**Production or Debugging Angle:**  
Log raw register values during bring-up and document product behavior for first boot after battery loss. Manufacturing tests should set and verify RTC time.

**Common Traps:**  
- Silently returning `2000-00-00` or another invalid date.
- Clearing oscillator-stop flags without warning.
- Letting userspace believe the time is valid.
- Not testing battery removal.

**Follow-up Questions:**  
- Should the driver auto-set a default time?
- What error should `read_time()` return for invalid data?
- How would you expose battery-low state if the chip supports it?

### 18. Debug scenario: A PWM fan spins during boot but stops when the driver loads. What do you investigate?

**Level:** Senior

**Short Answer:**  
Investigate bootloader state versus kernel-applied state, polarity, duty/period defaults, consumer binding, suspend/runtime PM, and whether the driver disables the PWM during probe.

**Deep Explanation:**  
The bootloader may leave the PWM running. When the Linux provider or consumer probes, it may reinitialize hardware, change pinmux, disable clocks, apply a default off state, or invert polarity. A fan consumer should apply a safe state deliberately, not accidentally inherit bootloader state or disable cooling.

**API / Code Anchor:**  
`pwm_get_state()`, `pwm_init_state()`, `pwm_apply_might_sleep()`, `pwm-fan` binding, pinctrl states, clock/reset APIs.

**Production or Debugging Angle:**  
Thermal and safety devices need explicit boot and handoff policy. Test from cold boot, warm reboot, module reload, suspend/resume, and thermal trip conditions.

**Common Traps:**  
- Assuming bootloader PWM state survives provider probe.
- Misinterpreting inverted polarity as duty failure.
- Disabling PWM on remove/suspend without considering board safety.
- Not checking the real waveform when the fan tach says zero.

**Follow-up Questions:**  
- Should a provider preserve hardware state at probe?
- When should a consumer disable a PWM at remove?
- How would you test fan behavior without risking hardware?
