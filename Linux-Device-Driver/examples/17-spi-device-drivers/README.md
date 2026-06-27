# 17 - SPI Device Drivers Example

This is a **learning-only** SPI device driver example. It demonstrates the shape of a small register-based SPI peripheral driver and exposes one temperature value through the standard `hwmon` subsystem.

Do **not** load this on arbitrary hardware. The code assumes a training-specific SPI register protocol:

- register `0x00`: chip ID, expected value `0x5a`;
- register `0x01`: signed 8-bit temperature in degrees Celsius;
- read command format: `0x80 | register`.

## Goal

Use this example to connect the runtime chain:

```text
spi-temp-demo.dts
  -> SPI child node with compatible + reg + spi-max-frequency
  -> SPI core creates struct spi_device
  -> spi_temp_demo.c probe()
  -> spi_setup()
  -> spi_write_then_read()
  -> devm_hwmon_device_register_with_info()
  -> /sys/class/hwmon/hwmonX/temp1_input
```

The example demonstrates:

- `struct spi_driver` with OF and SPI ID match tables;
- modern `struct spi_driver.remove = void (*)(struct spi_device *spi)`;
- Device Tree instantiation of a SPI child device;
- `reg = <0>` as chip-select index, not an MMIO address;
- probe-time SPI configuration with `spi_setup()`;
- per-device private data with `spi_set_drvdata()`;
- a mutex around command/response transfers and shared buffers;
- `spi_write_then_read()` for a simple command-then-read register protocol;
- managed registration with the kernel `hwmon` subsystem;
- expected error handling for setup, ID read, initial read, and subsystem registration.

## Kernel Version Assumptions

Build against the exact kernel headers for the target system:

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

This example was written against the Linux 6.x SPI driver style and assumes:

- external module builds through Kbuild;
- `struct spi_driver.probe = int (*)(struct spi_device *spi)`;
- `struct spi_driver.remove = void (*)(struct spi_device *spi)`;
- `devm_hwmon_device_register_with_info()`;
- `spi_write_then_read()` from `<linux/spi/spi.h>`.

Older books and code may use `spi_master` terminology for controller-side concepts. Modern kernel headers and docs generally use `spi_controller`. Validate callback signatures and `struct spi_transfer` fields against the target kernel headers before adapting this code.

## Files

| File | Purpose |
| --- | --- |
| `spi_temp_demo.c` | Learning-only SPI device module that reads a tiny training register protocol and registers a hwmon device. |
| `spi-temp-demo.dts` | Placeholder Device Tree overlay fragment showing a SPI child node with `compatible`, `reg`, and `spi-max-frequency`. |
| `Makefile` | Out-of-tree Kbuild wrapper for `spi_temp_demo.ko`. |
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

Using `hwmon` is intentional: a temperature-like device should normally expose an existing subsystem ABI instead of inventing raw SPI register files for userspace.

## Build

From this directory:

```sh
make
```

Expected build artifacts include:

```text
spi_temp_demo.ko
spi_temp_demo.o
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
dtc -@ -I dts -O dtb -o spi-temp-demo.dtbo spi-temp-demo.dts
dtc -I dtb -O dts -o spi-temp-demo.decompiled.dts spi-temp-demo.dtbo
```

Before applying it to a board, replace:

- `target-path = "/soc/spi@REPLACE_ME"` with the real SPI controller path;
- `reg = <0>` with the real chip-select index;
- `spi-max-frequency = <1000000>` with a datasheet-safe clock;
- `compatible = "ldd,spi-temp-demo"` with the binding used by your adapted driver;
- any missing board resources such as regulators, reset GPIOs, interrupts, clocks, or pinctrl states.

The demo compatible string is training-specific on purpose. A real board may already have an in-tree driver for its SPI chip, and two drivers must not bind to the same SPI device.

## Load And Test

Use a target board with a known safe training device or emulator that implements the register protocol above.

Watch logs:

```sh
sudo dmesg -w
```

Load the module:

```sh
sudo insmod ./spi_temp_demo.ko
```

Expected probe log shape:

```text
spi_temp_demo spi0.0: registered hwmon temperature sensor, temp=<n> mC
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
sudo rmmod spi_temp_demo
```

Expected remove log shape:

```text
spi_temp_demo spi0.0: removed hwmon temperature sensor
```

## Manual Binding Notes

Prefer Device Tree for real embedded-board integration. SPI devices are not safely discoverable, so the kernel needs board or firmware description.

For lab experiments, some systems allow sysfs driver override and manual binding when a `spiB.C` device already exists:

```sh
echo spi_temp_demo | sudo tee /sys/bus/spi/devices/spi0.0/driver_override
echo spi0.0 | sudo tee /sys/bus/spi/drivers/spi_temp_demo/bind
```

Unbind after testing:

```sh
echo spi0.0 | sudo tee /sys/bus/spi/drivers/spi_temp_demo/unbind
echo "" | sudo tee /sys/bus/spi/devices/spi0.0/driver_override
```

Only do this when you know the selected device is safe to access with the demo protocol.

## Debug Commands

Check controller/device/driver presence:

```sh
ls /sys/bus/spi/devices/
ls /sys/bus/spi/drivers/
find /sys/bus/spi/drivers/spi_temp_demo -maxdepth 2 -type l -o -type f
```

Check module aliases:

```sh
modinfo ./spi_temp_demo.ko | grep -E 'alias|description'
```

Inspect logs:

```sh
dmesg | grep -Ei 'spi_temp_demo|spi|hwmon'
```

Enable dynamic debug for this file if the kernel supports it:

```sh
echo 'file spi_temp_demo.c +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Check SPI tracepoints if present on the target kernel:

```sh
ls /sys/kernel/debug/tracing/events/spi
```

Use a logic analyzer when possible. For this demo, a successful register read should show:

```text
CS asserted
MOSI: 0x80 for chip ID or 0x81 for temperature
SCK toggles for command and response bytes
MISO: 0x5a for ID or signed temperature byte
CS deasserted
```

## Expected Failure Logs

SPI setup problem:

```text
spi_temp_demo spi0.0: spi_setup: <errno>
```

Wrong chip select, wrong mode, unpowered chip, or broken command protocol:

```text
spi_temp_demo spi0.0: read chip id: <errno>
```

Device responds, but it is not the expected training device:

```text
spi_temp_demo spi0.0: unexpected chip id 0x00: -19
```

Initial temperature read problem:

```text
spi_temp_demo spi0.0: initial temperature read: <errno>
```

Hwmon registration problem:

```text
spi_temp_demo spi0.0: register hwmon device: <errno>
```

## Cleanup And Error Paths

The probe path is intentionally ordered:

```text
configure mode/bits/speed
  -> spi_setup()
  -> allocate private data with devm_kzalloc()
  -> initialize mutex and save SPI driver data
  -> read chip ID
  -> perform one real temperature read
  -> register hwmon with devm_hwmon_device_register_with_info()
```

If `spi_setup()` fails, no private resources were allocated. If allocation fails, probe returns `-ENOMEM`. If the ID read fails or the ID value is wrong, the driver returns an error and no userspace hwmon device is exposed. If hwmon registration fails, devm-managed private data is released by the driver core.

On remove, the driver logs removal. The hwmon device and private allocation are devm-managed and are released automatically when the SPI device unbinds.

The mutex protects the shared `tx_buf` and `rx_buf` fields. This matters because hwmon reads can happen multiple times after probe, and a production driver may later add IRQ or workqueue paths that share the same transfer helper.

## What Production Code Would Add

This example is not production-ready. A real driver would add:

- a real YAML Device Tree binding and `dtbs_check` coverage;
- documented compatible strings for exact chip variants;
- regulator, reset GPIO, clock, pinctrl, and power-on delay handling when required;
- datasheet-correct mode, chip-select polarity, max speed, dummy cycles, and timing;
- a real register map and conversion formula for the actual sensor;
- runtime PM and system suspend/resume if the chip can be powered down;
- threaded IRQ handling if the chip has alert/data-ready interrupts;
- regmap if the register map grows beyond a few simple operations;
- stronger cleanup ordering for workqueues, IRQs, async transfers, and PM;
- tests on the real target kernel and board;
- logic analyzer validation during first hardware bring-up.

## Common Mistakes This Example Avoids

- writing a platform driver for a SPI peripheral;
- treating `probe()` as chip-select scanning;
- treating SPI child `reg` as an MMIO address;
- forgetting `spi_setup()` after changing mode, speed, or word size;
- calling `spi_sync()` from hard IRQ context;
- exposing raw SPI register poking as a permanent userspace ABI;
- copying stale `compatible = "spidev"` examples into production Device Tree;
- using shared transfer buffers without a lock;
- ignoring transfer errors during probe.
