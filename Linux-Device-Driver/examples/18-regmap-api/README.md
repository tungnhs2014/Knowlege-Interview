# 18 - Regmap API Example

This is a **learning-only** regmap-backed I2C driver example. It demonstrates how a small register-oriented device can use `struct regmap_config`, `devm_regmap_init_i2c()`, access-policy callbacks, register cache, `regmap_read()`, and `regmap_update_bits()`.

Do **not** load this on arbitrary hardware. The code assumes a training-specific I2C register protocol:

- register `0x00`: chip ID, expected value `0x5a`;
- register `0x01`: control register, bit 0 enables measurements, bits `[2:1]` select sample rate;
- register `0x02`: signed 8-bit temperature in degrees Celsius;
- register `0x03`: clear-on-read event register, used only to demonstrate `precious_reg`.

## Goal

Use this example to connect the runtime chain:

```text
regmap-temp-demo.dts
  -> I2C child node with compatible + reg address
  -> I2C core creates struct i2c_client
  -> regmap_temp_demo.c probe()
  -> devm_regmap_init_i2c()
  -> regmap_read(REG_ID)
  -> regmap_update_bits(REG_CTRL, mask, value)
  -> devm_hwmon_device_register_with_info()
  -> /sys/class/hwmon/hwmonX/temp1_input
```

The example demonstrates:

- `struct regmap_config` with 8-bit register addresses and 8-bit values;
- `readable_reg`, `writeable_reg`, `volatile_reg`, and `precious_reg`;
- `REGCACHE_RBTREE` with a reset default for the control register;
- `devm_regmap_init_i2c()` and managed lifetime;
- `regmap_read()` for chip ID and temperature;
- `regmap_update_bits()` for read/modify/write control bits;
- standard `hwmon` userspace ABI instead of custom raw register files;
- probe error paths for adapter capability, regmap init, ID read, configuration, first data read, and subsystem registration.

## Kernel Version Assumptions

Build against the exact kernel headers for the target system:

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

This example was written for Linux 6.x style external modules and assumes:

- `struct i2c_driver.probe = int (*)(struct i2c_client *client)`;
- `struct i2c_driver.remove = void (*)(struct i2c_client *client)`;
- `devm_regmap_init_i2c()` from `<linux/regmap.h>`;
- `devm_hwmon_device_register_with_info()` from `<linux/hwmon.h>`;
- `FIELD_PREP()` from `<linux/bitfield.h>`;
- `REGCACHE_RBTREE` support in the target kernel.

Older kernels may use older I2C remove/probe signatures or different regmap fields. Validate against the target kernel's `<linux/i2c.h>` and `<linux/regmap.h>` before adapting the code.

## Files

| File | Purpose |
| --- | --- |
| `regmap_temp_demo.c` | Learning-only I2C client module using regmap and registering a hwmon temperature input. |
| `regmap-temp-demo.dts` | Placeholder Device Tree overlay fragment for an I2C child node. |
| `Makefile` | Out-of-tree Kbuild wrapper for `regmap_temp_demo.ko`. |
| `README.md` | Build, load, test, debug, cleanup, ABI, and production notes. |

## Userspace ABI Impact

This example creates a standard `hwmon` userspace ABI when the driver successfully probes:

- `/sys/class/hwmon/hwmonX/name`
- `/sys/class/hwmon/hwmonX/temp1_input`

`temp1_input` is read-only and reports millidegrees Celsius. For example, `25000` means 25.000 C.

This example does **not** create:

- a `/dev` node;
- ioctl commands;
- custom sysfs attributes;
- procfs files;
- debugfs files owned by this driver.

Regmap itself may expose debugfs entries under `/sys/kernel/debug/regmap/` if the target kernel is configured for regmap debugfs support. Treat those as debugging interfaces, not product ABI.

## Build

From this directory:

```sh
make
```

Expected build artifacts include:

```text
regmap_temp_demo.ko
regmap_temp_demo.o
Module.symvers
modules.order
```

For a cross-compiled target kernel:

```sh
make KDIR=/path/to/target/kernel/build \
     ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

Clean generated files:

```sh
make clean
```

## Device Tree Setup

Compile the placeholder overlay for inspection:

```sh
dtc -@ -I dts -O dtb -o regmap-temp-demo.dtbo regmap-temp-demo.dts
dtc -I dtb -O dts -o regmap-temp-demo.decompiled.dts regmap-temp-demo.dtbo
```

Before applying it to a board, replace:

- `target-path = "/soc/i2c@REPLACE_ME"` with the real I2C controller path;
- `reg = <0x48>` with the training device's real I2C address;
- `compatible = "ldd,regmap-temp-demo"` only if the attached device implements this training protocol;
- any missing board resources such as regulators, reset GPIOs, clocks, interrupts, or pinctrl states.

The compatible string is training-specific on purpose. A real board may already have an in-tree driver for its sensor, and two drivers must not bind to the same I2C device.

## Load And Test

Use a target board with a known safe training device or emulator that implements the register protocol above.

Watch logs:

```sh
sudo dmesg -w
```

Load the module:

```sh
sudo insmod ./regmap_temp_demo.ko
```

Expected probe log shape:

```text
regmap_temp_demo 1-0048: registered regmap hwmon demo, temp=<n> mC
```

Find the hwmon device:

```sh
for h in /sys/class/hwmon/hwmon*; do
    printf '%s: ' "$h"
    cat "$h/name"
done
```

Read temperature:

```sh
cat /sys/class/hwmon/hwmonX/temp1_input
```

Expected output shape:

```text
25000
```

Unload:

```sh
sudo rmmod regmap_temp_demo
```

Expected remove log shape:

```text
regmap_temp_demo 1-0048: removed regmap hwmon demo
```

## Manual Binding Notes

Prefer Device Tree for real embedded-board integration. I2C devices are not safely discoverable, so the kernel needs board or firmware description.

For lab experiments, some systems allow sysfs driver override and manual binding when an `i2c-BUS-ADDR` device already exists:

```sh
echo regmap_temp_demo | sudo tee /sys/bus/i2c/devices/1-0048/driver_override
echo 1-0048 | sudo tee /sys/bus/i2c/drivers/regmap_temp_demo/bind
```

Unbind after testing:

```sh
echo 1-0048 | sudo tee /sys/bus/i2c/drivers/regmap_temp_demo/unbind
echo "" | sudo tee /sys/bus/i2c/devices/1-0048/driver_override
```

Only do this when you know the selected device is safe to access with the demo protocol.

## Debug Commands

Check I2C device and driver presence:

```sh
ls /sys/bus/i2c/devices/
ls /sys/bus/i2c/drivers/
find /sys/bus/i2c/drivers/regmap_temp_demo -maxdepth 2 -type l -o -type f
```

Check module aliases:

```sh
modinfo ./regmap_temp_demo.ko | grep -E 'alias|description|depends'
```

Inspect logs:

```sh
dmesg | grep -Ei 'regmap_temp_demo|regmap|i2c|hwmon'
```

Enable dynamic debug for this file if the kernel supports it:

```sh
echo 'file regmap_temp_demo.c +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Inspect regmap debugfs entries if present:

```sh
sudo mount -t debugfs none /sys/kernel/debug 2>/dev/null || true
find /sys/kernel/debug/regmap -maxdepth 2 -type f 2>/dev/null
```

Common useful files, when exposed by the target kernel:

```text
/sys/kernel/debug/regmap/<device>/registers
/sys/kernel/debug/regmap/<device>/access
```

Do not blindly dump registers on real hardware until `precious_reg` and volatile policy are correct. This demo marks register `0x03` as precious because it represents a clear-on-read event register.

## Expected Failure Logs

Adapter capability problem:

```text
regmap_temp_demo 1-0048: adapter lacks SMBus byte-data support: -95
```

Regmap setup problem:

```text
regmap_temp_demo 1-0048: failed to initialize regmap: <errno>
```

Wrong address, unpowered chip, held reset, or bus wiring issue:

```text
regmap_temp_demo 1-0048: failed to read chip id: <errno>
```

Device responds, but it is not the expected training device:

```text
regmap_temp_demo 1-0048: unexpected chip id 0x00: -19
```

Control register rejected by policy or bus:

```text
regmap_temp_demo 1-0048: failed to configure control register: <errno>
```

Initial temperature read problem:

```text
regmap_temp_demo 1-0048: failed initial temperature read: <errno>
```

Hwmon registration problem:

```text
regmap_temp_demo 1-0048: failed to register hwmon device: <errno>
```

## Cleanup And Error Paths

The probe path is intentionally ordered:

```text
check I2C adapter supports SMBus byte-data
  -> allocate private data with devm_kzalloc()
  -> initialize regmap with devm_regmap_init_i2c()
  -> save private data with i2c_set_clientdata()
  -> read and validate chip ID with regmap_read()
  -> configure control bits with regmap_update_bits()
  -> perform one real temperature read
  -> register hwmon with devm_hwmon_device_register_with_info()
```

If adapter capability checking fails, no resources were allocated. If allocation fails, probe returns `-ENOMEM`. If regmap initialization fails, there is no map to clean up manually. If chip ID validation, configuration, first data read, or hwmon registration fails, devm-managed resources are released automatically by the driver core.

On remove, the driver logs removal. The hwmon device, private data, and regmap are devm-managed and are released automatically when the I2C client unbinds.

The example relies on regmap's default locking for register operations. A production driver may still need its own mutex around multi-step driver state changes that combine register access with software state.

## Why This Is Not Production-Ready

This example is intentionally small and **learning-only**. A production driver would add:

- a real YAML Device Tree binding and `dtbs_check` coverage;
- exact compatible strings for real chip variants;
- datasheet-correct register definitions and conversion formulas;
- regulator, reset GPIO, clock, pinctrl, and power-on delay handling when required;
- runtime PM and system suspend/resume handling;
- cache dirty/sync sequencing if the device loses register state;
- interrupt handling for alert/data-ready/event lines if present;
- careful review of all volatile and precious registers;
- stronger cleanup ordering if workqueues, IRQs, child devices, or PM callbacks are added;
- hardware tests on the real target board and kernel.

## Common Mistakes This Example Highlights

- using raw I2C helpers for a register-heavy device when regmap would centralize policy;
- forgetting that `regmap_update_bits()` needs a shifted value under the mask;
- caching a hardware-updated temperature register;
- allowing debug register dumps to read a clear-on-read event register;
- using `fast_io` for a sleeping I2C transfer path;
- assuming devm-managed regmap removes the need to unregister runtime users in the right order;
- exposing raw register poking as a permanent userspace ABI when a standard subsystem ABI fits.
