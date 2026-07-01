# 28 - Watchdog And NVMEM Frameworks Example

## Status
This is a **learning-only** board-integration example.

It does not build a custom kernel module. That is intentional:

- A fake watchdog module that resets nothing can teach the API shape, but not the real risk: real watchdog expiry reboots the board.
- A fake NVMEM provider can teach callbacks, but not the real risk: EEPROM writes may wear cells and OTP/eFuse writes may be irreversible.
- Real products usually use existing providers such as `gpio-wdt`, `at24`, SoC OTP/eFuse drivers, RTC-backed NVMEM, or vendor watchdog drivers.

Use this example on a disposable target board or lab image, not on your workstation or a field unit.

## Goal
Connect the two runtime chains for topic 28:

```text
External GPIO watchdog:
  DTS node
    -> gpio-wdt driver
    -> watchdog core
    -> /dev/watchdogN and /sys/class/watchdog/watchdogN
    -> hardware reset if keepalive stops

I2C EEPROM as NVMEM provider:
  DTS EEPROM node with cells
    -> at24/NVMEM provider
    -> NVMEM core
    -> /sys/bus/nvmem/devices/<provider>/nvmem
    -> consumers read named cells such as mac-address
```

This example demonstrates:

- A `linux,wdt-gpio` watchdog node.
- A read-only I2C EEPROM-like NVMEM provider with named cells.
- A consumer node that references an NVMEM MAC-address cell.
- Watchdog userspace ABI checks and safe keepalive testing.
- Read-only NVMEM inspection through sysfs.
- Debug commands for missing watchdogs, missing NVMEM providers, and bad cells.

## Kernel Version Assumptions
Build and test against the exact target kernel and board DTS.

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

This example assumes:

- Linux has watchdog core support enabled.
- The GPIO watchdog driver is enabled, commonly `CONFIG_GPIO_WATCHDOG`.
- The target has an external watchdog controlled by one GPIO.
- Linux has NVMEM core support enabled, commonly `CONFIG_NVMEM`.
- The target has an EEPROM provider driver such as `at24` with NVMEM registration support.
- The I2C controller, GPIO controller, pinctrl, and board power rails are already working.
- The target kernel uses current YAML bindings; validate the final DTS against the kernel tree.

No out-of-tree module is built, so no `Makefile` is included.

## Files
| File | Purpose |
| --- | --- |
| `board-snippets.dts` | Learning-only DTS fragments for one GPIO watchdog and one NVMEM EEPROM provider with cells. |
| `README.md` | Integration, test, debug, ABI, cleanup, and production notes. |

## Userspace ABI Impact
If the watchdog node binds successfully, Linux exposes standard watchdog ABI:

- `/dev/watchdog`
- `/dev/watchdogN`
- `/sys/class/watchdog/watchdogN/identity`
- `/sys/class/watchdog/watchdogN/timeout`
- `/sys/class/watchdog/watchdogN/timeleft`
- `/sys/class/watchdog/watchdogN/nowayout`
- `/sys/class/watchdog/watchdogN/state`
- `/sys/class/watchdog/watchdogN/bootstatus`

Opening `/dev/watchdogN` usually starts the watchdog. If keepalive stops, the board may reset.

If the NVMEM provider binds successfully, Linux may expose:

- `/sys/bus/nvmem/devices/`
- `/sys/bus/nvmem/devices/<provider>/nvmem`
- Optional cell-oriented sysfs entries, depending on kernel version/configuration/bindings.

This example does not create private character devices, custom ioctls, procfs, or debugfs files.

## Device Tree Fragment
The fragment is in `board-snippets.dts`.

Adapt these labels to the real board:

- `&gpio3` must be the real GPIO controller connected to the external watchdog.
- `&i2c1` must be the real I2C controller connected to the EEPROM.
- `ethernet0` must be the real network device node that consumes the MAC address.
- EEPROM compatible, size, page size, and address must match the actual chip.
- GPIO polarity and watchdog algorithm must match the external watchdog datasheet.

Keep the NVMEM provider read-only until you have a clear manufacturing/update flow.

## Build / Integrate
Copy the relevant snippets into the board DTS or board overlay in the kernel tree.

Example in-tree DTB build:

```sh
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- dtbs
```

Native target build, if the board supports it:

```sh
make dtbs
```

Deploy the DTB or overlay using the board's normal boot flow, then reboot.

Do not deploy `board-snippets.dts` as a complete board file. It is a fragment.

## Load / Runtime Checks
Check relevant modules or built-in drivers:

```sh
zcat /proc/config.gz 2>/dev/null | grep -E 'CONFIG_GPIO_WATCHDOG|CONFIG_NVMEM|CONFIG_EEPROM_AT24|CONFIG_WATCHDOG'
lsmod | grep -E 'gpio_wdt|at24|nvmem|watchdog'
modinfo gpio_wdt 2>/dev/null | head
modinfo at24 2>/dev/null | head
```

If drivers are modules and not auto-loaded:

```sh
sudo modprobe gpio_wdt
sudo modprobe at24
```

Driver names and module availability are kernel/config specific.

## Expected Logs
Exact logs vary by kernel and board. The shape should look like this:

```text
gpio-wdt watchdog: registered watchdog0
watchdog: watchdog0: watchdog did not stop!
at24 1-0050: 256 byte 24c02 EEPROM, read-only
nvmem ...: registered nvmem device ...
```

Not every kernel prints all of these lines. Use sysfs checks as the primary evidence.

## Watchdog Test Commands
First inspect without opening the device:

```sh
ls -l /dev/watchdog*
ls /sys/class/watchdog/

WDT=/sys/class/watchdog/watchdog0
cat "$WDT/identity"
cat "$WDT/timeout"
cat "$WDT/nowayout"
cat "$WDT/state"
cat "$WDT/bootstatus"
cat "$WDT/timeleft" 2>/dev/null || true
dmesg | grep -i watchdog
```

Safe keepalive test:

```sh
sudo sh -c '
  exec 3>/dev/watchdog0
  i=0
  while [ $i -lt 5 ]; do
    printf "." >&3
    echo "sent keepalive $i"
    sleep 2
    i=$((i + 1))
  done
  printf "V" >&3
'
```

Expected result:

```text
sent keepalive 0
sent keepalive 1
sent keepalive 2
sent keepalive 3
sent keepalive 4
```

If magic close is supported and `nowayout` is `0`, the final `V` asks the kernel to stop the watchdog on close. If `nowayout` is `1`, the watchdog may continue running after the shell exits.

Do not run an expiry test unless you intend to reboot the board:

```sh
# DANGEROUS: this intentionally stops feeding the watchdog.
# sudo sh -c 'exec 3>/dev/watchdog0; sleep 9999'
```

## NVMEM Test Commands
List NVMEM providers:

```sh
ls /sys/bus/nvmem/devices/
dmesg | grep -i -E 'nvmem|at24|eeprom|efuse|otp'
```

Inspect provider names and binary attributes:

```sh
for dev in /sys/bus/nvmem/devices/*; do
  echo "== $dev =="
  ls -l "$dev"
  test -r "$dev/nvmem" && stat -c "size=%s path=%n" "$dev/nvmem"
done
```

Read a small region carefully:

```sh
NVMEM=$(find /sys/bus/nvmem/devices -name nvmem | head -n1)
echo "$NVMEM"
sudo hexdump -C "$NVMEM" | head
```

Expected result:

```text
/sys/bus/nvmem/devices/1-00500/nvmem
00000000  ...
```

If the example EEPROM stores a MAC address at offset `0x00`, the first six bytes should match the board's programmed address. If they are all `00` or all `ff`, treat the value as invalid.

## Debugging
Watchdog debug flow:

```sh
dmesg | grep -i watchdog
ls -l /dev/watchdog*
find /sys/class/watchdog -maxdepth 2 -type f -print
cat /sys/class/watchdog/watchdog0/identity 2>/dev/null
cat /sys/class/watchdog/watchdog0/nowayout 2>/dev/null
cat /sys/class/watchdog/watchdog0/timeleft 2>/dev/null || true
```

Common watchdog failures:

- No `/dev/watchdog0`: node did not bind, driver disabled, bad compatible, missing GPIO.
- Immediate reboot: watchdog already running, timeout too short, no keepalive owner.
- Close still reboots: `nowayout` active or magic close unsupported.
- GPIO watchdog does not toggle: wrong GPIO, wrong polarity, missing pinctrl, wrong `hw_algo`.

NVMEM debug flow:

```sh
dmesg | grep -i -E 'nvmem|at24|eeprom|of:|defer'
ls /sys/bus/nvmem/devices/
grep -R . /sys/bus/nvmem/devices/*/name 2>/dev/null || true
```

Common NVMEM failures:

- No provider: I2C device did not probe, compatible is wrong, address is wrong, driver disabled.
- Consumer gets `-EPROBE_DEFER`: provider has not registered yet.
- Consumer gets `-ENOENT`: `nvmem-cells` phandle or `nvmem-cell-names` is wrong.
- Bad MAC/calibration: cell offset/size wrong, erased storage, endian mismatch, or manufacturing data absent.

## Cleanup
To undo the example:

1. Remove the watchdog and EEPROM/NVMEM snippets from the board DTS or overlay.
2. Rebuild and redeploy the DTB/overlay.
3. Reboot.
4. Confirm nodes disappeared:

```sh
ls /sys/class/watchdog/
ls /sys/bus/nvmem/devices/
dmesg | grep -i -E 'watchdog|nvmem|at24'
```

If the watchdog remains active after cleanup, power-cycle the board and check bootloader watchdog settings. Some external watchdogs cannot be fully disabled once enabled.

## Error-Path Explanation
There is no custom module cleanup path in this example because all runtime objects are owned by in-tree drivers and the device model.

Expected ownership:

- DT node appears.
- Matching driver probes.
- Driver requests GPIO/I2C resources.
- Driver registers with watchdog or NVMEM core.
- Core creates userspace ABI.
- Removing the DT node and rebooting prevents the device from being instantiated.

If probe fails, the framework objects should not appear. If probe defers, the object may appear later after its GPIO, I2C, regulator, clock, or pinctrl dependency is ready.

## Why This Is Not Production-Ready
This is not production-ready as-is because:

- The DTS uses placeholder GPIO, I2C, and Ethernet labels.
- Watchdog timeout and `hw_margin_ms` must be chosen from the real hardware datasheet.
- External watchdog behavior must be validated with a serial console and recovery path.
- EEPROM compatible, size, page size, and write-protect behavior must match the real chip.
- NVMEM writes are intentionally not demonstrated.
- Product security must decide whether raw NVMEM sysfs access is acceptable.
- Final DTS must pass the target kernel's YAML binding checks.

Production work would add:

- Board-specific reset-reason validation.
- Clear watchdog owner policy from bootloader to Linux to userspace.
- Manufacturing policy for programming NVMEM values.
- Validation that MAC/calibration cells are present and sane.
- CI or lab tests for DT binding validation and boot logs.
