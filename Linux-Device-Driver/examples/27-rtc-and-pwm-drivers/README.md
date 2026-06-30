# 27 - RTC And PWM Drivers Example

## Status
This is a **learning-only** board-integration example.

It does not build a custom kernel module. That is intentional: a single fake module that owns both RTC and PWM would teach the wrong ownership model. In real systems, an RTC is usually an I2C/SPI/platform/MFD child driver registered with the RTC core, while PWM users are usually standard consumers such as `pwm-leds`, `pwm-backlight`, `pwm-fan`, `pwm-regulator`, or a focused product driver.

## Goal
Use this example to connect the runtime chain:

```text
Device Tree describes:
  I2C RTC chip at address 0x68
  PWM LED consumer using pwm3 channel 0

kernel boot:
  -> I2C core creates RTC client
  -> rtc-pcf8523 driver registers /dev/rtcN and /sys/class/rtc/rtcN
  -> PWM provider registers /sys/class/pwm/pwmchipN
  -> pwm-leds requests a PWM channel
  -> LED class exposes /sys/class/leds/status:white/brightness
```

The example demonstrates:

- RTC binding shape for an I2C RTC chip.
- Standard RTC userspace ABI under `/dev/rtcN` and `/sys/class/rtc/rtcN`.
- RTC wakealarm test commands when the board supports alarm wake.
- PWM consumer binding through `pwms` and `pwm-names`-style lookup.
- Why production should prefer a subsystem consumer over manual PWM sysfs control.
- Lab-only PWM sysfs debug for waveform bring-up.

## Kernel Version Assumptions
Build and test against the exact target kernel and board DTS.

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

This example assumes:

- Linux has RTC class support enabled.
- The PCF8523-compatible RTC driver is available, often as `rtc-pcf8523`.
- The board I2C controller and pinctrl are already working.
- The board PWM provider driver for `&pwm3` is enabled and registers a PWM chip.
- The LED PWM consumer driver is enabled, often through `CONFIG_LEDS_PWM`.
- PWM sysfs debug exists only if the target kernel enables it.
- Suspend wakealarm tests require real alarm-capable RTC hardware and wake-capable IRQ routing.

Kernel APIs and bindings drift. Validate final DTS against the YAML bindings in the target kernel tree before treating it as production DTS.

## Files
| File | Purpose |
| --- | --- |
| `board-snippets.dts` | Learning-only DTS fragments for one I2C RTC and one PWM LED consumer. |
| `README.md` | Build, deploy, test, debug, ABI, cleanup, and production notes. |

No `Makefile` is included because there is no out-of-tree kernel module in this example.

## Userspace ABI Impact
If the RTC node binds successfully, Linux exposes standard RTC ABI:

- `/dev/rtcN`
- `/sys/class/rtc/rtcN/date`
- `/sys/class/rtc/rtcN/time`
- `/sys/class/rtc/rtcN/since_epoch`
- `/sys/class/rtc/rtcN/hctosys`
- `/sys/class/rtc/rtcN/wakealarm` if alarm support is available
- `/proc/driver/rtc` on systems that provide it

If the PWM LED node binds successfully, Linux exposes LED class ABI:

- `/sys/class/leds/status:white/brightness`
- `/sys/class/leds/status:white/max_brightness`

This example does not create a private character device, ioctl, procfs file, debugfs file, or custom sysfs ABI.

Manual `/sys/class/pwm/pwmchipN` export is shown only for lab debugging. In production, prefer a real consumer driver or standard subsystem binding so userspace talks to the appropriate class, such as LED, backlight, fan, regulator, or clock.

## Device Tree Fragment
The fragment is in `board-snippets.dts`.

Adapt the labels to your board:

- `&i2c1` must be the real I2C bus connected to the RTC.
- `&pwm3` must be the real PWM controller.
- The PWM specifier cell layout depends on the provider binding.
- The RTC `interrupts` and `wakeup-source` properties are board-specific; keep them only when the IRQ is physically wired and wake-capable.

## Build / Integrate
Copy the relevant pieces from `board-snippets.dts` into the board DTS or a board overlay in your kernel tree.

Example in-tree build:

```sh
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- dtbs
```

For a native build on the target:

```sh
make dtbs
```

If your platform supports overlays, build the overlay with the platform's normal overlay flow. Do not deploy this fragment directly as a complete DTS; it is intentionally not a standalone board file.

## Deploy / Load
Deploy the new DTB or overlay using your board's boot flow, then reboot.

Useful module checks:

```sh
lsmod | grep -E 'rtc|pwm|leds'
modinfo rtc-pcf8523 2>/dev/null | head
modinfo leds_pwm 2>/dev/null | head
```

If the drivers are modules and not auto-loaded:

```sh
sudo modprobe rtc-pcf8523
sudo modprobe leds_pwm
```

The PWM provider module name is SoC-specific, so check the target kernel config, DTS compatible string, and `dmesg`.

## Expected Logs
Exact logs vary by kernel and board. The shape should look like this:

```text
rtc-pcf8523 1-0068: registered as rtc0
rtc-pcf8523 1-0068: setting system clock to ...
leds_pwm: registered PWM LED status:white
pwm <provider>: registered ... PWM channels
```

If the RTC was not used to initialize system time, `hctosys` may be `0`. That is not automatically a driver failure; systems with multiple RTCs or userspace time sync may choose a different policy.

## RTC Test Commands
List RTC devices:

```sh
ls -l /dev/rtc*
ls /sys/class/rtc/
```

Inspect the RTC:

```sh
RTC=/sys/class/rtc/rtc0
cat "$RTC/name"
cat "$RTC/date"
cat "$RTC/time"
cat "$RTC/since_epoch"
cat "$RTC/hctosys"
cat /proc/driver/rtc 2>/dev/null || true
```

Compare system time and RTC time:

```sh
date -u
sudo hwclock --show --rtc /dev/rtc0
```

Set RTC from system time during lab testing:

```sh
sudo hwclock --systohc --utc --rtc /dev/rtc0
sudo hwclock --show --rtc /dev/rtc0
```

Expected result:

```text
/dev/rtc0 exists
/sys/class/rtc/rtc0/date and time show valid values
hwclock --show returns a sensible UTC time
```

## RTC Wakealarm Test
Only run this when the board has a wake-capable RTC alarm IRQ and suspend/resume is safe on the target.

Program an alarm 60 seconds in the future:

```sh
RTC=/sys/class/rtc/rtc0
echo 0 | sudo tee "$RTC/wakealarm"
echo $(( $(date +%s) + 60 )) | sudo tee "$RTC/wakealarm"
cat "$RTC/wakealarm"
```

Suspend:

```sh
echo mem | sudo tee /sys/power/state
```

Expected result:

```text
system suspends
RTC alarm fires about 60 seconds later
system resumes
dmesg shows RTC/alarm or wakeup-source activity
```

Debug failed wake:

```sh
dmesg | grep -i rtc
cat /proc/interrupts | grep -i rtc
cat /sys/kernel/debug/wakeup_sources 2>/dev/null | grep -i rtc || true
cat "$RTC/wakealarm"
```

Common causes:

- RTC alarm IRQ is not wired.
- DTS lacks the correct interrupt description.
- Device is not marked as a wake source.
- Driver supports time but not alarm IRQs.
- Suspend state powers off the needed wake domain.

## PWM LED Test Commands
Find the LED class device:

```sh
ls /sys/class/leds/
LED=/sys/class/leds/status:white
cat "$LED/max_brightness"
```

Change brightness:

```sh
echo 0 | sudo tee "$LED/brightness"
sleep 1
echo 32 | sudo tee "$LED/brightness"
sleep 1
echo 255 | sudo tee "$LED/brightness"
```

Expected result:

```text
LED turns off
LED becomes dim
LED becomes bright
```

If brightness is inverted, check PWM polarity, board transistor/inverter wiring, and the consumer binding before changing driver logic.

## Lab-Only PWM Sysfs Debug
Use this only when no kernel consumer owns the channel. If `pwm-leds` has already requested the PWM, manual export should fail or conflict.

Find a PWM chip:

```sh
ls /sys/class/pwm/
cat /sys/class/pwm/pwmchip0/npwm
```

Export and drive channel 0:

```sh
cd /sys/class/pwm/pwmchip0
echo 0 | sudo tee export
echo 20000000 | sudo tee pwm0/period
echo 5000000 | sudo tee pwm0/duty_cycle
echo normal | sudo tee pwm0/polarity
echo 1 | sudo tee pwm0/enable
```

Expected waveform:

```text
period: 20 ms
duty:   5 ms
ratio:  25%
```

Always verify with a scope or logic analyzer. Sysfs confirms requested state, not the physical waveform at the pin.

Cleanup:

```sh
echo 0 | sudo tee /sys/class/pwm/pwmchip0/pwm0/enable
echo 0 | sudo tee /sys/class/pwm/pwmchip0/unexport
```

## Debug Checklist
RTC:

- `dmesg | grep -i rtc`
- `i2cdetect -y <bus>` if safe for the board and bus devices.
- Confirm the RTC address, usually `0x68` for this example.
- Confirm the compatible string matches an enabled driver.
- Check oscillator-stop or battery-low messages.
- Check `/sys/class/rtc/rtcN/hctosys` when boot time is wrong.

PWM:

- `dmesg | grep -i pwm`
- `ls /sys/class/pwm/`
- Check pinctrl debugfs for the pin's active function.
- Confirm the provider clock/reset is enabled by the provider driver.
- Confirm no other consumer owns the channel.
- Measure the output pin with a scope or logic analyzer.

## Cleanup
For a DTB-based test:

1. Restore the previous DTB or remove the overlay from the boot configuration.
2. Reboot.
3. Confirm the test devices disappeared or returned to the previous state:

```sh
ls /sys/class/rtc/
ls /sys/class/leds/
dmesg | grep -E -i 'rtc|pwm|led'
```

For module-based drivers:

```sh
sudo modprobe -r leds_pwm
sudo modprobe -r rtc-pcf8523
```

Module removal may fail with `Module in use` if another subsystem or userspace process still holds the device.

## Error-Path Explanation
Expected failure modes are useful clues:

| Symptom | Likely Cause | Fix |
| --- | --- | --- |
| No `/dev/rtc0` | RTC driver did not bind or probe failed | Check DTS `compatible`, I2C bus, address, power, and `dmesg`. |
| `hwclock` reports invalid time | RTC registers invalid after battery/oscillator loss | Set time, inspect driver handling of validity flags. |
| `wakealarm` missing | Driver or chip lacks alarm support | Check chip data sheet and RTC driver ops. |
| Wakealarm set but no resume | IRQ is not wake-capable or PM wake not configured | Check IRQ wiring, DTS, wakeup sources, suspend state. |
| No `status:white` LED | `pwm-leds` did not bind | Check DTS node, `CONFIG_LEDS_PWM`, and PWM provider probe. |
| PWM export fails with busy | A kernel consumer already owns the PWM | Unbind/remove the consumer only in a lab, or test through the consumer ABI. |
| LED brightness inverted | Polarity or board inverter mismatch | Fix PWM flags/binding or board-specific polarity. |
| Sysfs PWM works but LED does not | Consumer binding/name mismatch | Check `pwms`, provider channel, and class driver logs. |

## Why This Is Not Production-Ready
The example is intentionally generic and board-neutral.

Production work must add:

- DTS validated against the target kernel YAML bindings.
- Correct RTC chip compatible string and required properties.
- Correct interrupt parent, IRQ type, and wakeup wiring.
- Board-specific pinctrl states for PWM output.
- Correct PWM provider specifier cells and polarity flags.
- Electrical validation with real hardware.
- Suspend/resume validation for wakealarm and PWM consumers.
- Manufacturing test coverage for RTC battery/oscillator state.

The production-ready pattern is not "copy this fragment unchanged"; it is "use standard RTC and PWM subsystem bindings, then validate every board-specific detail."
