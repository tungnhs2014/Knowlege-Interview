# 25 - IIO Framework Example

## Status
This is a **learning-only** example.

It builds a loadable kernel module that registers a synthetic IIO platform device. It is useful for learning IIO object setup, generated sysfs ABI, `read_raw()`, scan elements, and triggered-buffer plumbing. It is **not production-ready** because it does not bind to real firmware-described hardware and does not read a real ADC or sensor.

## Goal
Use this example to connect the runtime chain:

```text
insmod ldd_iio_demo.ko
  -> module creates a synthetic platform_device
  -> platform_driver probe allocates struct iio_dev
  -> driver describes 3 voltage channels
  -> IIO core creates /sys/bus/iio/devices/iio:deviceX
  -> userspace reads in_voltage*_raw and in_voltage_scale
  -> optional trigger captures binary samples through /dev/iio:deviceX
```

The example demonstrates:

- `devm_iio_device_alloc()` and `iio_priv()`;
- `struct iio_info.read_raw`;
- `struct iio_chan_spec` for indexed voltage channels;
- generated ABI files such as `in_voltage0_raw`;
- `IIO_VAL_INT` and `IIO_VAL_INT_PLUS_MICRO`;
- `iio_buffer_enabled()` returning `-EBUSY` behavior for direct reads during streaming;
- `devm_iio_triggered_buffer_setup()`;
- `iio_pollfunc_store_time`;
- `iio_push_to_buffers_with_timestamp()`;
- `iio_trigger_notify_done()`;
- managed cleanup on probe failure and module unload.

## Kernel Version Assumptions
Build against the exact kernel headers for the target system:

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

This example was checked against Linux `6.8.0-124-generic` headers and assumes:

- `devm_iio_device_alloc()` and `devm_iio_device_register()` from `<linux/iio/iio.h>`;
- `devm_iio_triggered_buffer_setup()` from `<linux/iio/triggered_buffer.h>`;
- `iio_pollfunc_store_time()` and `iio_trigger_notify_done()` from `<linux/iio/trigger_consumer.h>`;
- `struct platform_driver.remove = int (*)(struct platform_device *pdev)` on this target header set;
- classic `struct iio_chan_spec.scan_type.sign` field;
- IIO core support enabled in the target kernel.

Older or newer kernels may differ in IIO helper availability, platform remove signatures, buffer directory names, trigger module names, or scan-type fields. Validate against the target kernel headers before adapting the code.

## Files
| File | Purpose |
| --- | --- |
| `ldd_iio_demo.c` | Learning-only platform module registering a synthetic IIO voltage device. |
| `Makefile` | Out-of-tree Kbuild wrapper for `ldd_iio_demo.ko`. |
| `README.md` | Build, load, test, debug, ABI, cleanup, and production notes. |

No Device Tree file is included because this module intentionally creates its own synthetic platform device for lab use. A production IIO driver should bind to a real I2C, SPI, or platform device described by firmware or board code.

## Userspace ABI Impact
When loaded successfully, this example creates a standard IIO userspace ABI:

- `/sys/bus/iio/devices/iio:deviceX/name`
- `/sys/bus/iio/devices/iio:deviceX/in_voltage0_raw`
- `/sys/bus/iio/devices/iio:deviceX/in_voltage1_raw`
- `/sys/bus/iio/devices/iio:deviceX/in_voltage2_raw`
- `/sys/bus/iio/devices/iio:deviceX/in_voltage_scale`
- `/sys/bus/iio/devices/iio:deviceX/scan_elements/*`
- `/sys/bus/iio/devices/iio:deviceX/buffer/*` or `buffer0/*`, depending on the kernel
- `/sys/bus/iio/devices/iio:deviceX/trigger/current_trigger`
- `/dev/iio:deviceX`

The raw channel values are synthetic 12-bit ADC counts from `0` to `4095`. The scale is approximately `0.000439` V/count, so a userspace learning formula is:

```text
voltage = raw * in_voltage_scale
```

This example does **not** create:

- custom sysfs attributes;
- ioctl commands;
- procfs entries;
- debugfs files owned by this driver.

## Build
From this directory:

```sh
make
```

Expected build artifact:

```text
ldd_iio_demo.ko
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

## Load And Direct-Read Test
Watch kernel logs:

```sh
sudo dmesg -w
```

Load the module:

```sh
sudo insmod ./ldd_iio_demo.ko
```

Expected log shape:

```text
ldd-iio-demo ldd-iio-demo.0: registered learning-only IIO demo
```

Find the IIO device:

```sh
for d in /sys/bus/iio/devices/iio:device*; do
    printf '%s: ' "$d"
    cat "$d/name"
done
```

Expected device name:

```text
ldd-iio-demo
```

Set a helper variable:

```sh
IIODEV=$(for d in /sys/bus/iio/devices/iio:device*; do
    [ "$(cat "$d/name")" = "ldd-iio-demo" ] && echo "$d"
done | head -n1)
echo "$IIODEV"
```

Read direct sysfs values:

```sh
cat "$IIODEV/in_voltage0_raw"
cat "$IIODEV/in_voltage1_raw"
cat "$IIODEV/in_voltage2_raw"
cat "$IIODEV/in_voltage_scale"
```

Expected output shape:

```text
17
734
1451
0.000439
```

The exact raw values change on each read because the demo increments an internal synthetic sample counter.

## Triggered Buffer Test
This part needs IIO trigger support in the target kernel. Direct reads work without this section.

Load a sysfs trigger provider if available:

```sh
sudo modprobe iio-trig-sysfs
```

Create trigger `sysfstrig0`:

```sh
echo 0 | sudo tee /sys/bus/iio/devices/iio_sysfs_trigger/add_trigger
cat /sys/bus/iio/devices/trigger0/name
```

Assign the trigger:

```sh
echo sysfstrig0 | sudo tee "$IIODEV/trigger/current_trigger"
```

Inspect scan layout:

```sh
ls "$IIODEV/scan_elements"
cat "$IIODEV/scan_elements/in_voltage0_type"
cat "$IIODEV/scan_elements/in_voltage1_type"
cat "$IIODEV/scan_elements/in_voltage2_type"
```

Expected type shape:

```text
le:u12/16>>0
```

Enable all three voltage channels:

```sh
echo 1 | sudo tee "$IIODEV/scan_elements/in_voltage0_en"
echo 1 | sudo tee "$IIODEV/scan_elements/in_voltage1_en"
echo 1 | sudo tee "$IIODEV/scan_elements/in_voltage2_en"
```

Set the buffer length and enable the buffer. Some kernels expose `buffer`, others expose `buffer0`; use the directory that exists:

```sh
BUFDIR="$IIODEV/buffer"
[ -d "$BUFDIR" ] || BUFDIR="$IIODEV/buffer0"

echo 8 | sudo tee "$BUFDIR/length"
echo 1 | sudo tee "$BUFDIR/enable"
```

Fire the trigger a few times:

```sh
for i in 1 2 3 4; do
    echo 1 | sudo tee /sys/bus/iio/devices/trigger0/trigger_now
done
```

Read binary samples. Prefer `tools/iio/generic_buffer` when available because it decodes IIO metadata:

```sh
sudo generic_buffer -n ldd-iio-demo -a -l 4
```

If `generic_buffer` is not installed, inspect bytes only:

```sh
IIOCHR=/dev/$(basename "$IIODEV")
sudo timeout 1 dd if="$IIOCHR" bs=16 count=4 2>/dev/null | hexdump -C
```

Expected byte output shape:

```text
00000000  44 00 11 03 de 05 00 00  ...
```

Each scan contains three little-endian 16-bit synthetic values plus timestamp padding/data when timestamp scan is active.

Disable and detach:

```sh
echo 0 | sudo tee "$BUFDIR/enable"
echo "" | sudo tee "$IIODEV/trigger/current_trigger"
echo 0 | sudo tee /sys/bus/iio/devices/iio_sysfs_trigger/remove_trigger
```

## Debug Commands
Inspect generated ABI:

```sh
find "$IIODEV" -maxdepth 2 -type f | sort
cat "$IIODEV/name"
cat "$IIODEV/in_voltage_scale"
cat "$IIODEV/scan_elements/in_voltage0_index"
cat "$IIODEV/scan_elements/in_voltage0_type"
```

Check whether direct reads are blocked while the buffer is enabled:

```sh
echo 1 | sudo tee "$BUFDIR/enable"
cat "$IIODEV/in_voltage0_raw"
echo 0 | sudo tee "$BUFDIR/enable"
```

Expected failure while enabled:

```text
cat: .../in_voltage0_raw: Device or resource busy
```

Check logs:

```sh
dmesg | tail -50
```

Useful things to verify when buffer capture fails:

- `iio-trig-sysfs` loaded successfully;
- `/sys/bus/iio/devices/iio_sysfs_trigger` exists;
- `trigger/current_trigger` contains `sysfstrig0`;
- scan elements are enabled before `buffer/enable`;
- `buffer/length` is nonzero;
- the correct `buffer` or `buffer0` directory is used;
- `/dev/iio:deviceX` exists and has readable permissions for the test user.

## Cleanup
Disable active capture before unloading:

```sh
echo 0 | sudo tee "$BUFDIR/enable" 2>/dev/null || true
echo "" | sudo tee "$IIODEV/trigger/current_trigger" 2>/dev/null || true
```

Unload:

```sh
sudo rmmod ldd_iio_demo
```

Expected log shape:

```text
ldd-iio-demo ldd-iio-demo.0: removed learning-only IIO demo
```

Clean the build directory:

```sh
make clean
```

## Cleanup And Error-Path Explanation
The module uses managed IIO helpers to keep the example small:

- `devm_iio_device_alloc()` releases the IIO object with the platform device.
- `devm_iio_triggered_buffer_setup()` unwinds buffer setup automatically on probe failure or device removal.
- `devm_iio_device_register()` unregisters the IIO device automatically when the platform device is removed.
- Module exit unregisters the synthetic platform device first, then unregisters the platform driver.

Important ordering points:

- `iio_device_register()` happens after channels, callbacks, and buffer support are initialized.
- The direct read path uses `iio_buffer_enabled()` and returns `-EBUSY` while streaming is active.
- The trigger handler always calls `iio_trigger_notify_done()` after pushing a sample.

Production drivers still need more than this:

- bind to real hardware through Device Tree, ACPI, I2C, SPI, or platform resources;
- validate chip ID and hardware revision;
- use regmap or bus helpers for real register access;
- manage regulators, clocks, reset GPIOs, pinctrl states, and interrupts;
- use runtime PM around one-shot reads and buffered capture;
- handle suspend/resume and restore volatile state;
- define precise locking rules for sysfs, trigger handler, IRQ, PM, and remove paths;
- validate channel ABI and buffer layout against the datasheet;
- test invalid userspace sequences and unload while userspace is active.

## Why This Is Not Production-Ready
This module is intentionally synthetic.

It skips production requirements:

- no real hardware description;
- no register map;
- no electrical validation;
- no IRQ/data-ready line;
- no runtime PM;
- no regulator, clock, reset, or pinctrl handling;
- no real error recovery from bus faults;
- no ABI review for a real datasheet.

Use it to learn the IIO framework mechanics. Use a real sensor driver pattern for production.
