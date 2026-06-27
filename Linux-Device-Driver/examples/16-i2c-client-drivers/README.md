# 16 - I2C Client Drivers Example

This is a **learning-only** I2C client driver example. It demonstrates the shape of a small register-based temperature sensor driver and exposes the result through the standard `hwmon` subsystem.

Do **not** load this on arbitrary hardware. The code assumes a TMP102-like temperature register at I2C address `0x48`. Adapt the compatible string, address, register format, power sequencing, and binding before using it on a real board.

## Goal

Use this example to connect the runtime chain:

```text
i2c-tmp102-demo.dts
  -> I2C child node with compatible + reg
  -> I2C core creates struct i2c_client
  -> i2c_tmp102_demo.c probe()
  -> i2c_check_functionality()
  -> i2c_smbus_read_word_data()
  -> devm_hwmon_device_register_with_info()
  -> /sys/class/hwmon/hwmonX/temp1_input
```

The example demonstrates:

- `struct i2c_driver` with OF and I2C ID match tables;
- modern one-argument I2C `probe()` and `void remove()`;
- `i2c_check_functionality()` before using SMBus word operations;
- per-client private data with `i2c_set_clientdata()`;
- a mutex around I2C register access;
- SMBus read return-value handling;
- device-specific endian and signed-temperature conversion;
- managed registration with the kernel `hwmon` subsystem;
- Device Tree instantiation of an I2C child device.

## Kernel Version Assumptions

Build against the exact kernel headers for the target system:

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

This example assumes a modern kernel with:

- external module builds through Kbuild;
- `struct i2c_driver.probe = int (*)(struct i2c_client *client)`;
- `struct i2c_driver.remove = void (*)(struct i2c_client *client)`;
- `devm_hwmon_device_register_with_info()`;
- SMBus word helpers from `<linux/i2c.h>`.

Older examples often use:

```c
static int probe(struct i2c_client *client,
		 const struct i2c_device_id *id);
static int remove(struct i2c_client *client);
```

Validate callback prototypes against the target kernel headers before adapting this code.

## Files

| File | Purpose |
| --- | --- |
| `i2c_tmp102_demo.c` | Learning-only I2C client module that reads a TMP102-like temperature register and registers a hwmon device. |
| `i2c-tmp102-demo.dts` | Placeholder Device Tree overlay fragment showing an I2C child node with `compatible` and `reg`. |
| `Makefile` | Out-of-tree Kbuild wrapper for `i2c_tmp102_demo.ko`. |
| `README.md` | Build, load, test, debug, cleanup, ABI, and production notes. |

## Userspace ABI Impact

This example creates a standard `hwmon` userspace ABI:

- `/sys/class/hwmon/hwmonX/name`
- `/sys/class/hwmon/hwmonX/temp1_input`

`temp1_input` is read-only and reports millidegrees Celsius. For example, `25000` means 25.000 C.

This example does **not** create:

- a `/dev` node;
- ioctl commands;
- custom sysfs attributes;
- procfs files;
- debugfs files.

Using `hwmon` is intentional: a temperature sensor should normally expose the existing subsystem ABI instead of inventing raw register files for userspace.

## Build

From this directory:

```sh
make
```

Expected build artifacts include:

```text
i2c_tmp102_demo.ko
i2c_tmp102_demo.o
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
dtc -@ -I dts -O dtb -o i2c-tmp102-demo.dtbo i2c-tmp102-demo.dts
dtc -I dtb -O dts -o i2c-tmp102-demo.decompiled.dts i2c-tmp102-demo.dtbo
```

Before applying it to a board, replace:

- `target-path = "/soc/i2c@REPLACE_ME"` with the real I2C controller path;
- `reg = <0x48>` with the real 7-bit I2C address;
- `compatible = "ldd,tmp102-demo"` with the binding used by your adapted driver;
- any missing board resources such as regulators, reset GPIOs, interrupts, or pinctrl states.

The demo compatible string is training-specific on purpose. A real TMP102 board may already be supported by an in-tree driver, and two drivers must not bind to the same I2C client.

## Load And Test

Use a target board with a known safe TMP102-like device and an adapted Device Tree node.

Watch logs:

```sh
sudo dmesg -w
```

Load the module:

```sh
sudo insmod ./i2c_tmp102_demo.ko
```

Expected probe log shape:

```text
i2c_tmp102_demo 1-0048: registered hwmon temperature sensor, temp=<n> mC
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
sudo rmmod i2c_tmp102_demo
```

Expected remove log shape:

```text
i2c_tmp102_demo 1-0048: removed hwmon temperature sensor
```

### Manual Instantiation For Lab Bring-Up

If your lab kernel allows manual I2C device creation and you are sure the device at `0x48` is safe to access:

```sh
sudo insmod ./i2c_tmp102_demo.ko
echo i2c_tmp102_demo 0x48 | sudo tee /sys/bus/i2c/devices/i2c-1/new_device
cat /sys/class/hwmon/hwmonX/temp1_input
echo 0x48 | sudo tee /sys/bus/i2c/devices/i2c-1/delete_device
sudo rmmod i2c_tmp102_demo
```

Replace `i2c-1` with the real adapter number. Prefer Device Tree for real embedded-board integration.

## Debug Commands

Check adapter and client presence:

```sh
ls /sys/class/i2c-adapter/
ls /sys/bus/i2c/devices/
find /sys/bus/i2c/drivers/i2c_tmp102_demo -maxdepth 2 -type l -o -type f
```

Check module aliases:

```sh
modinfo ./i2c_tmp102_demo.ko | grep -E 'alias|description'
```

Inspect logs:

```sh
dmesg | grep -Ei 'i2c_tmp102_demo|tmp102|hwmon|i2c'
```

Enable dynamic debug for this file if the kernel supports it:

```sh
echo 'file i2c_tmp102_demo.c +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Use I2C userspace tools carefully:

```sh
i2cdetect -l
sudo i2cdetect -y 1
```

**Warning:** generic I2C probing can disturb some chips. Use it only when the board and device are known safe.

## Expected Failure Logs

Wrong adapter capability:

```text
i2c_tmp102_demo 1-0048: adapter lacks SMBus word-data support: -95
```

No ACK, wrong address, unpowered chip, or bus problem:

```text
i2c_tmp102_demo 1-0048: initial temperature read: -121
```

Hwmon registration problem:

```text
i2c_tmp102_demo 1-0048: register hwmon device: <errno>
```

## Cleanup And Error Paths

The probe path is intentionally ordered:

```text
check adapter capability
  -> allocate private data with devm_kzalloc()
  -> initialize mutex and save client data
  -> perform one real register read
  -> register hwmon with devm_hwmon_device_register_with_info()
```

If capability checking fails, no resources were allocated. If allocation fails, probe returns `-ENOMEM`. If the first I2C read fails, the driver returns the transfer errno and no userspace hwmon device is exposed. If hwmon registration fails, devm-managed private data is released by the driver core.

On remove, the driver logs removal. The hwmon device and private allocation are devm-managed and are released automatically when the I2C client unbinds.

## What Production Code Would Add

This example is not production-ready. A real driver would add:

- a real YAML Device Tree binding and `dtbs_check` coverage;
- documented compatible strings for exact chip variants;
- regulator, reset GPIO, clock, and power-on delay handling when required;
- chip ID or variant detection if the hardware provides it;
- correct conversion for every supported resolution/configuration mode;
- suspend/resume and runtime PM if the chip can be powered down;
- interrupt handling in a threaded handler if the chip has alert/ready IRQs;
- regmap if the register map grows beyond a few simple operations;
- stronger locking around multi-register read-modify-write sequences;
- datasheet-based fault handling and limit/alarm attributes;
- tests on the real target kernel and board.

## Common Mistakes This Example Avoids

- treating `probe()` as bus scanning;
- using I2C transfer helpers before checking adapter functionality;
- exposing raw register poking as a permanent userspace ABI;
- ignoring negative SMBus read returns;
- treating a valid temperature value of `0` as failure;
- forgetting endian conversion for an SMBus word read;
- copying stale I2C callback prototypes into a newer kernel.
