# 27 - RTC And PWM Drivers

## Learning Goal
Understand how Linux handles two common hardware classes: RTC chips that keep wall-clock time and PWM controllers that generate timed output waveforms. After this topic, you should be able to write or review a small RTC driver, use a PWM as a consumer, understand the provider side, and debug the usual bring-up failures without inventing a private ABI.

After this topic you should be able to:

- Explain the difference between an **RTC hardware clock**, the Linux **system clock**, kernel timers, and PWM.
- Implement RTC callbacks for reading/setting time and alarms.
- Decode and validate `struct rtc_time`, including BCD and year/month traps.
- Report RTC alarm interrupts with `rtc_update_irq()`.
- Explain how `/dev/rtcN`, `/sys/class/rtc/rtcN`, `hwclock`, and `wakealarm` fit together.
- Explain PWM period, duty cycle, polarity, and enable state.
- Use `devm_pwm_get()`, `pwm_init_state()`, and `pwm_apply_might_sleep()` as a PWM consumer.
- Describe provider-side objects such as `struct pwm_chip`, `struct pwm_ops`, and `.apply()`.
- Debug RTC wake failures and PWM waveform failures on real boards.

## Why This Matters In Real Work
RTC and PWM drivers appear in ordinary embedded boards: battery-backed clocks, PMIC alarm blocks, LED dimming, display backlights, fans, buzzers, regulators, and PWM-derived clocks. Linux already has subsystems for these jobs, so a production driver should plug into those subsystems instead of exposing random sysfs files or custom ioctls.

You meet this topic when:

- A board has an I2C/SPI/PMIC RTC that must set system time at boot.
- A product needs alarm wake from suspend.
- A chip stores time in BCD registers, has oscillator-stop flags, or uses unusual year ranges.
- A display backlight, fan, LED, regulator, or clock uses PWM hardware.
- A driver needs to change PWM duty cycle without glitches.
- A board fails because a PWM pin is not muxed, a clock is disabled, or a userspace test uses the wrong RTC node.

The shared rule is simple: **common hardware class, common kernel framework**. RTC drivers register with the RTC core. PWM controllers register with the PWM core. PWM consumers request channels from the framework instead of poking controller registers directly.

## Mental Model
An RTC is a small wall-clock device. A PWM is a small waveform generator. They both involve time, but they solve very different problems.

```text
RTC:
  battery-backed clock registers
    -> RTC driver converts chip fields
    -> RTC core
    -> /dev/rtcN, /sys/class/rtc/rtcN, hwclock, wakealarm

PWM:
  clocked output hardware
    -> PWM provider registers channels
    -> PWM consumer requests a channel
    -> consumer applies period, duty, polarity, enabled
    -> hardware waveform appears on a pin
```

Useful comparison:

| Concept | What It Does | Typical Unit | Userspace / Consumer |
| --- | --- | --- | --- |
| RTC | Stores wall-clock calendar time across power loss | year/month/day/hour/min/sec | `/dev/rtcN`, `hwclock`, `wakealarm` |
| System clock | Linux software time after boot | nanoseconds since epoch internally | `clock_gettime()`, file timestamps |
| Kernel timer/hrtimer | Runs code after a delay or at intervals | jiffies or nanoseconds | kernel-internal |
| PWM | Generates repeated active/inactive output cycles | nanoseconds period/duty | backlight, LED, fan, regulator, clock, sysfs debug |

**RTC is not a timer callback mechanism. PWM is not a wall-clock.** A GPIO toggled by an hrtimer may look like PWM in a lab, but real PWM-capable hardware should normally use the PWM framework.

## Core Concepts
RTC concepts are about translating chip-specific calendar registers into the RTC core's standard representation.

| Concept | Meaning |
| --- | --- |
| Hardware clock | Battery-backed or always-on clock maintained outside the Linux system clock. |
| System clock | Linux software-maintained time used by userspace and kernel timestamps. |
| `struct rtc_time` | Kernel broken-down time format for RTC callbacks. |
| BCD | Binary-coded decimal storage used by many RTC chips. |
| Alarm | Hardware event scheduled for a future RTC time. |
| Wakealarm | RTC alarm used as a system wake source during suspend. |
| RTC class device | Registered object exposed as `/dev/rtcN` and `/sys/class/rtc/rtcN`. |

`struct rtc_time` has a few interview-grade traps:

| Field | Meaning |
| --- | --- |
| `tm_sec` | Seconds, usually `0..59`. |
| `tm_min` | Minutes, `0..59`. |
| `tm_hour` | Hours, normally `0..23`. |
| `tm_mday` | Day of month, `1..31`. |
| `tm_mon` | Month counted from `0`, so January is `0`. |
| `tm_year` | Years since 1900. |
| `tm_wday` | Day of week, often `0..6`; chip encodings vary. |

PWM concepts are about a repeated waveform.

```text
period:      full cycle time
duty_cycle: active part of the cycle
polarity:   whether active means high or low
enabled:    whether the PWM output is running
```

Example:

```text
period = 20 ms
duty_cycle = 5 ms
duty = 25%
```

Key PWM terms:

| Concept | Meaning |
| --- | --- |
| PWM provider | Controller driver that owns PWM hardware channels. |
| PWM consumer | Driver using a PWM channel, such as backlight, LED, fan, regulator, or clock. |
| `struct pwm_state` | Desired state: period, duty cycle, polarity, enabled, and usage hints. |
| `#pwm-cells` | Provider DT property describing PWM specifier format. |
| `pwms` | Consumer DT property linking to a provider channel. |
| `pwm-names` | Optional names for multiple PWM inputs. |

## Kernel Mechanism
The RTC core provides the standard class device and calls the driver's chip-specific operations. The driver owns bus access, register conversion, interrupt acknowledgement, and wakeup setup.

RTC object relationship:

```text
I2C / SPI / platform / MFD child device
  -> driver private data
       -> regmap/client/MMIO pointer
       -> irq number
       -> struct rtc_device *
  -> RTC core
       -> struct rtc_class_ops callbacks
       -> /dev/rtcN and /sys/class/rtc/rtcN
```

The driver normally:

- Gets bus resources, regmap, IRQs, clocks, or PMIC parent data.
- Allocates/registers an RTC device.
- Provides `read_time()` and usually `set_time()`.
- Adds alarm callbacks when hardware supports alarms.
- Converts chip register format to/from `struct rtc_time`.
- Validates decoded time before returning it.
- Requests and handles alarm/update IRQs when available.
- Enables wakeup only when the board and hardware really support it.
- Optionally registers RTC-backed NVMEM after the RTC device exists.

The PWM core separates controller ownership from consumers. A provider registers channels; a consumer requests one by firmware description or lookup and applies a state.

PWM object relationship:

```text
PWM controller driver
  -> struct pwm_chip
  -> struct pwm_ops
  -> PWM core
       -> struct pwm_device channels
       -> consumers request channels

consumer driver
  -> devm_pwm_get()
  -> pwm_init_state()
  -> pwm_apply_might_sleep()
```

The provider should not know whether a channel drives a backlight, fan, LED, or regulator. The consumer should not know provider registers. The binding or lookup connects them.

## Key Structs And APIs
Learn these APIs by flow and ownership. The framework objects are not just containers; they define who owns lifetime and who is allowed to touch hardware.

### RTC Core

| Struct / API | Role |
| --- | --- |
| `struct rtc_device` | Registered RTC class device. |
| `struct rtc_class_ops` | Callback table implemented by the RTC driver. |
| `struct rtc_time` | Broken-down time passed between core and driver. |
| `struct rtc_wkalrm` | Alarm time plus enabled/pending state. |
| `devm_rtc_allocate_device(dev)` | Managed allocation for a modern RTC device flow. |
| `devm_rtc_register_device(rtc)` | Register a previously allocated RTC device. |
| `devm_rtc_device_register()` | Older/common helper that allocates and registers in one call. |
| `rtc_valid_tm(tm)` | Validate a decoded `struct rtc_time`. |
| `rtc_update_irq(rtc, num, events)` | Notify RTC core about alarm/update/periodic IRQ events. |
| `rtc_update_irq_enable(rtc, enabled)` | Enable or disable update IRQ behavior when supported. |
| `rtc_tm_to_time64()` / `rtc_time64_to_tm()` | Convert between broken-down RTC time and seconds. |
| `devm_rtc_nvmem_register()` | Register RTC-backed NVMEM where supported. |

Common `struct rtc_class_ops` callbacks:

| Callback | Purpose |
| --- | --- |
| `.read_time` | Read hardware registers and fill `struct rtc_time`. |
| `.set_time` | Program hardware registers from `struct rtc_time`. |
| `.read_alarm` | Read alarm registers and enabled/pending state. |
| `.set_alarm` | Program alarm registers. |
| `.alarm_irq_enable` | Enable or disable alarm interrupt generation. |
| `.ioctl` | Optional chip-specific ioctl support; avoid unnecessary private ABI. |

### RTC Helpers And Front Ends

| API / File | Meaning |
| --- | --- |
| `bcd2bin()` / `bin2bcd()` | Convert BCD register fields to/from binary integers. |
| `device_init_wakeup(dev, true)` | Mark device as wake-capable. |
| `dev_pm_set_wake_irq(dev, irq)` | Associate a wake IRQ with the device. |
| `/dev/rtcN` | Character device for RTC userspace operations. |
| `/sys/class/rtc/rtcN/date` | Date view. |
| `/sys/class/rtc/rtcN/time` | Time view. |
| `/sys/class/rtc/rtcN/since_epoch` | Seconds since Unix epoch if supported. |
| `/sys/class/rtc/rtcN/hctosys` | Whether this RTC was used to initialize system time. |
| `/sys/class/rtc/rtcN/wakealarm` | Program alarm wake time on capable hardware. |
| `/proc/driver/rtc` | Legacy/common debug view for the selected RTC on many systems. |

### PWM Provider

| Struct / API | Role |
| --- | --- |
| `struct pwm_chip` | PWM controller registered with the PWM core. |
| `struct pwm_ops` | Provider callbacks. |
| `struct pwm_state` | Desired or current state for one PWM channel. |
| `pwmchip_add(chip)` | Register a PWM chip. |
| `devm_pwmchip_add(dev, chip)` | Managed chip registration in kernels that expose it. |
| `pwmchip_remove(chip)` | Remove chip registration; modern kernels may return `void`. |
| `.apply()` | Provider callback to apply a full PWM state atomically as far as hardware allows. |
| `.get_state()` | Provider callback to report current hardware state. |
| `.capture()` | Optional callback for PWM capture-capable hardware. |

Provider drivers usually also need clock, reset, pinctrl, MMIO/regmap, and PM handling. The PWM core does not magically enable the controller clock for you unless the provider driver does it.

### PWM Consumer

| Struct / API | Role |
| --- | --- |
| `struct pwm_device` | One requested PWM channel. |
| `devm_pwm_get(dev, con_id)` | Managed request for a PWM channel. |
| `pwm_get()` / `pwm_put()` | Unmanaged request/release pair. |
| `pwm_init_state(pwm, &state)` | Initialize state from PWM defaults/args. |
| `pwm_get_state(pwm, &state)` | Read last configured state known to the framework/provider. |
| `pwm_get_args(pwm)` | Inspect firmware-provided defaults, such as period. |
| `pwm_apply_might_sleep(pwm, &state)` | Apply state when the provider may sleep. |
| `pwm_apply_atomic(pwm, &state)` | Apply state only when provider supports atomic use. |
| `pwm_might_sleep(pwm)` | Check whether sleepable API is required. |
| `pwm_set_relative_duty_cycle(&state, duty, scale)` | Set duty as a fraction of period. |
| `pwm_get_relative_duty_cycle(&state, scale)` | Convert state duty back to a relative value. |

Prefer one state update:

```c
pwm_init_state(pwm, &state);
state.period = 20000000; /* ns */
pwm_set_relative_duty_cycle(&state, 50, 100);
state.enabled = true;
ret = pwm_apply_might_sleep(pwm, &state);
```

This is better than separately calling older wrappers such as `pwm_config()` and `pwm_enable()` because the provider can apply a coherent state with fewer glitches.

### Device Tree And ABI

| Binding / ABI | Meaning |
| --- | --- |
| `rtc@68` with `compatible` and `reg` | Typical I2C RTC node shape; chip binding decides exact properties. |
| `wakeup-source` | Common DT property used when a device can wake the system. |
| `#pwm-cells` | Provider declares how many cells consumer specifiers contain. |
| `pwms = <&pwm3 0 20000000 0>` | Consumer references provider, channel, period, and sometimes flags. |
| `pwm-names = "backlight"` | Names a PWM input for `devm_pwm_get(dev, "backlight")`. |
| `/sys/class/pwm/pwmchipN` | Debug/lab ABI for manual PWM export and control. |

## Lifecycle / Data Flow
RTC lifecycle has three important paths: probe, normal operations, and interrupt/wakeup handling.

```text
probe()
  allocate driver state
  initialize I2C/SPI/regmap/MMIO access
  read/clear chip status if needed
  allocate RTC device
  assign rtc->ops
  set supported range/features if needed
  register RTC device
  request alarm IRQ if present
  configure wakeup if supported by board and chip
  optionally register RTC NVMEM
```

Read/set time flow:

```text
userspace: hwclock --show or cat /sys/class/rtc/rtc0/time
  -> RTC core
  -> driver .read_time()
  -> read chip registers
  -> bcd2bin() and field adjustments
  -> rtc_valid_tm()
  -> return struct rtc_time
```

Alarm IRQ flow:

```text
userspace writes wakealarm or sets alarm
  -> RTC core calls .set_alarm()
  -> driver programs alarm registers and enables IRQ
  -> chip asserts IRQ at alarm time
  -> IRQ handler reads/clears chip alarm status
  -> rtc_update_irq(rtc, 1, RTC_IRQF | RTC_AF)
  -> RTC core wakes waiters and PM wake path if configured
```

PWM provider lifecycle:

```text
provider probe()
  get MMIO/regmap, clocks, resets, pinctrl
  initialize hardware-safe default state
  fill struct pwm_chip
  fill struct pwm_ops with .apply() and .get_state()
  register PWM chip

provider remove()
  ensure framework unregisters chip
  release managed resources
  leave outputs in board-safe state where possible
```

PWM consumer lifecycle:

```text
consumer probe()
  pwm = devm_pwm_get(dev, "name")
  pwm_init_state(pwm, &state)
  choose period/duty/polarity/enabled
  pwm_apply_might_sleep(pwm, &state)

runtime
  adjust duty or enable state through pwm_apply_might_sleep()

suspend/remove
  apply safe state or disable if the device requires it
```

## Minimal Practical Example
These are learning-only snippets. They show framework shape and common traps, not a complete chip driver.

### RTC Read/Set Time Skeleton

```c
struct demo_rtc {
	struct device *dev;
	struct regmap *regmap;
	struct rtc_device *rtc;
	int irq;
};

static int demo_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct demo_rtc *drtc = dev_get_drvdata(dev);
	unsigned int regs[7];
	int ret;

	ret = regmap_bulk_read(drtc->regmap, DEMO_REG_SEC, regs, 7);
	if (ret)
		return ret;

	tm->tm_sec  = bcd2bin(regs[0] & 0x7f);
	tm->tm_min  = bcd2bin(regs[1] & 0x7f);
	tm->tm_hour = bcd2bin(regs[2] & 0x3f);
	tm->tm_mday = bcd2bin(regs[3] & 0x3f);
	tm->tm_mon  = bcd2bin(regs[4] & 0x1f) - 1;
	tm->tm_year = bcd2bin(regs[5]) + 100; /* 20xx -> years since 1900 */
	tm->tm_wday = bcd2bin(regs[6] & 0x07);

	return rtc_valid_tm(tm);
}

static int demo_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct demo_rtc *drtc = dev_get_drvdata(dev);
	u8 regs[6];

	regs[0] = bin2bcd(tm->tm_sec);
	regs[1] = bin2bcd(tm->tm_min);
	regs[2] = bin2bcd(tm->tm_hour);
	regs[3] = bin2bcd(tm->tm_mday);
	regs[4] = bin2bcd(tm->tm_mon + 1);
	regs[5] = bin2bcd(tm->tm_year - 100);

	return regmap_bulk_write(drtc->regmap, DEMO_REG_SEC, regs, sizeof(regs));
}

static const struct rtc_class_ops demo_rtc_ops = {
	.read_time = demo_rtc_read_time,
	.set_time = demo_rtc_set_time,
};

static int demo_rtc_probe(struct platform_device *pdev)
{
	struct demo_rtc *drtc;
	int ret;

	drtc = devm_kzalloc(&pdev->dev, sizeof(*drtc), GFP_KERNEL);
	if (!drtc)
		return -ENOMEM;

	platform_set_drvdata(pdev, drtc);
	drtc->dev = &pdev->dev;

	/* Initialize drtc->regmap from the real bus or parent device here. */

	drtc->rtc = devm_rtc_allocate_device(&pdev->dev);
	if (IS_ERR(drtc->rtc))
		return PTR_ERR(drtc->rtc);

	drtc->rtc->ops = &demo_rtc_ops;

	ret = devm_rtc_register_device(drtc->rtc);
	if (ret)
		return ret;

	return 0;
}
```

Important lines:

- `bcd2bin()` and `bin2bcd()` are chip-format conversion, not optional decoration.
- `tm_mon` is adjusted because Linux counts months from `0`.
- `tm_year` is adjusted because Linux stores years since 1900.
- `rtc_valid_tm()` catches invalid decoded values before userspace receives nonsense.
- Real drivers must handle oscillator-stop flags, range limits, alarm registers, IRQ clear rules, and chip-specific century/year behavior.

### RTC Alarm IRQ Skeleton

```c
static irqreturn_t demo_rtc_irq(int irq, void *data)
{
	struct demo_rtc *drtc = data;
	unsigned int status;
	int ret;

	ret = regmap_read(drtc->regmap, DEMO_REG_STATUS, &status);
	if (ret)
		return IRQ_NONE;

	if (!(status & DEMO_ALARM_FLAG))
		return IRQ_NONE;

	regmap_write(drtc->regmap, DEMO_REG_STATUS, status & ~DEMO_ALARM_FLAG);
	rtc_update_irq(drtc->rtc, 1, RTC_IRQF | RTC_AF);

	return IRQ_HANDLED;
}
```

The key idea is: acknowledge the chip and then notify the RTC core. If you only clear the chip, userspace may never see the alarm. If you only call `rtc_update_irq()` without clearing hardware status, the IRQ may storm.

### PWM Consumer Skeleton

```c
struct demo_pwm_consumer {
	struct pwm_device *pwm;
};

static int demo_pwm_set_percent(struct demo_pwm_consumer *priv, unsigned int percent)
{
	struct pwm_state state;

	if (percent > 100)
		return -EINVAL;

	pwm_init_state(priv->pwm, &state);

	/*
	 * Keep the firmware/default period when possible. If the device
	 * requires a fixed period, set state.period explicitly.
	 */
	pwm_set_relative_duty_cycle(&state, percent, 100);
	state.enabled = percent != 0;

	return pwm_apply_might_sleep(priv->pwm, &state);
}

static int demo_consumer_probe(struct platform_device *pdev)
{
	struct demo_pwm_consumer *priv;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->pwm = devm_pwm_get(&pdev->dev, "output");
	if (IS_ERR(priv->pwm))
		return dev_err_probe(&pdev->dev, PTR_ERR(priv->pwm),
				     "failed to get PWM\n");

	return demo_pwm_set_percent(priv, 50);
}
```

Matching DTS shape:

```dts
demo-device {
	compatible = "vendor,demo-pwm-consumer";
	pwms = <&pwm3 0 20000000 0>;
	pwm-names = "output";
};
```

Important lines:

- `devm_pwm_get(&pdev->dev, "output")` matches `pwm-names = "output"`.
- `pwm_init_state()` starts from provider/firmware defaults.
- `pwm_set_relative_duty_cycle()` prevents percent-to-nanosecond math mistakes.
- `pwm_apply_might_sleep()` is the normal consumer API unless you have proven atomic constraints and provider support.

## Common Bugs And Debugging
Start from the symptom. RTC and PWM failures are often board-integration problems, not only C bugs.

### RTC: Time Is Wrong After Boot

Likely causes:

- RTC stores local time but Linux expects UTC policy.
- Driver forgot BCD conversion.
- `tm_mon` or `tm_year` adjustment is wrong.
- Battery died or oscillator stopped; registers contain invalid values.
- Wrong RTC was selected as the system `hctosys` source.

Inspect:

```bash
dmesg | grep -i rtc
ls -l /dev/rtc*
ls /sys/class/rtc/
cat /sys/class/rtc/rtc0/date
cat /sys/class/rtc/rtc0/time
cat /sys/class/rtc/rtc0/since_epoch
cat /sys/class/rtc/rtc0/hctosys
hwclock --show --rtc /dev/rtc0
cat /proc/driver/rtc 2>/dev/null
```

Fix patterns:

- Validate decoded `struct rtc_time` with `rtc_valid_tm()`.
- Handle oscillator-stop or battery-low flags explicitly.
- Use UTC consistently in product policy.
- Confirm the correct RTC node and driver are bound.

### RTC: Alarm Or Wakealarm Does Not Fire

Likely causes:

- No alarm IRQ is wired or described.
- Driver did not implement `.set_alarm`, `.read_alarm`, or `.alarm_irq_enable`.
- IRQ handler clears the chip but does not call `rtc_update_irq()`.
- IRQ handler calls the core but does not clear chip status, causing repeated interrupts.
- Device is not marked wake-capable or wake IRQ is not configured.
- Suspend state does not allow that wake source.

Inspect:

```bash
cat /sys/class/rtc/rtc0/wakealarm
date +%s
echo 0 > /sys/class/rtc/rtc0/wakealarm
echo $(( $(date +%s) + 60 )) > /sys/class/rtc/rtc0/wakealarm
cat /proc/interrupts | grep -i rtc
cat /sys/kernel/debug/wakeup_sources 2>/dev/null | grep -i rtc
```

Fix patterns:

- Check DT for interrupt and `wakeup-source` where appropriate.
- Request the correct IRQ and acknowledge the chip status register.
- Pair alarm programming with `.alarm_irq_enable`.
- Use `device_init_wakeup()` and `dev_pm_set_wake_irq()` when the hardware path supports wake.

### PWM: No Waveform On The Pin

Likely causes:

- Pinmux still selects GPIO or another peripheral, not PWM output.
- Provider driver is not bound or `pwmchipN` is missing.
- Controller clock/reset is disabled.
- Consumer uses the wrong `pwms` phandle, channel, or `pwm-names`.
- Another consumer already owns the channel.
- Duty cycle is zero or PWM is disabled.

Inspect:

```bash
ls /sys/class/pwm/
cat /sys/class/pwm/pwmchip0/npwm 2>/dev/null
dmesg | grep -i pwm
grep -R . /sys/kernel/debug/pinctrl/*/pinmux-pins 2>/dev/null | grep -i pwm
```

Lab-only sysfs check:

```bash
cd /sys/class/pwm/pwmchip0
echo 0 > export
echo 20000000 > pwm0/period
echo 5000000 > pwm0/duty_cycle
echo normal > pwm0/polarity
echo 1 > pwm0/enable
```

Use a scope or logic analyzer. Sysfs values only show what you requested, not what the pin physically does.

### PWM: Brightness/Fan Speed Is Wrong

Likely causes:

- Duty is treated as percent when API expects nanoseconds.
- `duty_cycle > period`.
- Polarity is inverted.
- Hardware cannot represent the requested period or duty exactly.
- Consumer changes duty and enable in separate calls, causing glitches.
- Driver assumes disabled PWM output level is deterministic.

Fix patterns:

- Use `pwm_set_relative_duty_cycle()` for percentages.
- Apply one coherent `struct pwm_state`.
- Read back or trace provider state where possible.
- Verify the real waveform on hardware.
- If a deterministic inactive output is required, consider enabled duty `0` rather than relying on disabled output state, depending on hardware behavior.

### Context And Lifetime Bugs

Common traps:

- Calling `pwm_apply_might_sleep()` from atomic context.
- Using `pwm_apply_atomic()` without checking `pwm_might_sleep()`.
- Forgetting that managed resources are released after driver remove returns.
- Registering RTC-backed NVMEM before RTC registration succeeds.
- Requesting an IRQ before driver state and `rtc` pointer are ready.
- Removing a PWM provider while consumers still depend on it.

Debugging approach:

- Check error returns from every resource request.
- Use `dev_err_probe()` for probe dependencies that may defer.
- Enable dynamic debug for the relevant driver when available.
- Check `/proc/interrupts`, sysfs class devices, pinctrl debugfs, and real board signals.

## Production Checklist
Use this list before code review or board bring-up. RTC/PWM bugs tend to look tiny in code and expensive in hardware labs.

RTC checklist:

- `read_time()` and `set_time()` handle chip encoding, BCD, masks, 12/24-hour mode, month/year offsets, and range limits.
- Decoded time is validated with `rtc_valid_tm()`.
- Oscillator-stop, battery-low, invalid-time, and century/range flags are handled clearly.
- Alarm callbacks match hardware capability and alarm mask semantics.
- IRQ handler acknowledges hardware status and calls `rtc_update_irq()` with the right flags.
- Wake support is enabled only when the board wiring and PM path are valid.
- `/dev/rtcN`, `/sys/class/rtc/rtcN`, `hwclock`, and `wakealarm` are tested on the target board.
- NVMEM registration, if any, happens after RTC registration and follows current helper APIs.
- DTS binding and chip-specific properties are validated.

PWM provider checklist:

- Provider implements state-based `.apply()` and `.get_state()` for modern kernels where possible.
- Clock, reset, pinctrl, runtime PM, and hardware-safe default state are handled.
- Period/duty rounding is documented or visible through readback where possible.
- Polarity is implemented correctly.
- Glitch behavior during reconfiguration is understood.
- Remove/suspend paths leave outputs in a board-safe state.
- Real waveform is verified with a scope or logic analyzer.

PWM consumer checklist:

- Uses `devm_pwm_get()` or a managed subsystem binding where possible.
- Uses `pwm_init_state()` and one `pwm_apply_might_sleep()` call for coherent updates.
- Handles `-EPROBE_DEFER` and other errors.
- Does not call sleepable PWM APIs from atomic context.
- Does not use PWM sysfs as the production ABI when a subsystem driver exists.
- Uses standard bindings such as `pwm-backlight`, `pwm-leds`, `pwm-fan`, `pwm-regulator`, or `pwm-clock` when they fit.
- Validates DT `pwms`, `pwm-names`, period, flags, and pinmux.

## Interview Readiness
For interviews, focus on reasoning from hardware behavior to framework behavior. You do not need to memorize every helper, but you should be able to explain the data path and the failure modes.

Be ready to explain:

- RTC hardware clock versus Linux system clock.
- Why RTC time should usually be treated as UTC.
- How `struct rtc_time` fields differ from normal human date notation.
- Why BCD conversion and validation matter.
- How an RTC alarm reaches userspace or wakes a suspended system.
- PWM period versus duty cycle versus polarity.
- PWM provider versus consumer roles.
- Why state-based `pwm_apply_might_sleep()` is preferred over split config/enable calls.
- Why GPIO hrtimer "PWM" is not the normal answer for real PWM hardware.
- How to debug a missing `/dev/rtcN`, a failed wakealarm, a missing PWM waveform, or an inverted PWM output.

See `interview/27-rtc-and-pwm-drivers.md` for structured beginner, mid-level, and senior questions.

## Kernel Version Notes
RTC and PWM APIs have evolved, so always check your target kernel headers and in-tree drivers before writing buildable code.

Practical caveats:

- Older RTC material may show `rtc_device_register()` or `devm_rtc_device_register()`. Modern code often uses `devm_rtc_allocate_device()` plus `devm_rtc_register_device()` so the driver can fill fields before registration.
- RTC-backed NVMEM helper names vary across kernel versions. Check your target for helpers such as `devm_rtc_nvmem_register()`.
- Older PWM examples may use `pwm_config()`, `pwm_enable()`, and `pwm_disable()`. New consumer code should prefer state-based `pwm_apply_might_sleep()` when changing configuration.
- Older PWM provider examples may implement `.config`, `.enable`, and `.disable`. Modern provider code should prefer `.apply()` and `.get_state()`.
- `pwmchip_remove()` signatures and PWM chip allocation helpers vary by kernel version. Do not assume return types or allocation APIs from a different kernel tree.
