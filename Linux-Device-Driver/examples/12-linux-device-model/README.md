# 12 - Linux Device Model Example

This is a **learning-only** kernel module. It creates one synthetic platform device, registers one matching platform driver, and exposes one class-device sysfs attribute so you can inspect how the Linux Device Model connects devices, drivers, buses, classes, sysfs, and lifetime.

It does **not** touch real hardware, map fake MMIO, request fake IRQs, or create a custom bus.

## Goal

Use this example to observe three views of one driver-managed instance:

```text
/sys/devices/platform/lld-ldm-demo.0
  -> physical or parent hierarchy view

/sys/bus/platform/devices/lld-ldm-demo.0
/sys/bus/platform/drivers/lld-ldm-demo
  -> bus matching and binding view

/sys/class/ldm_demo/ldm-demo0/enabled
  -> functional class view and userspace sysfs ABI
```

The code demonstrates:

- `struct platform_device` as a bus-specific wrapper around `struct device`;
- `struct platform_driver` registration and platform-bus matching;
- `probe()` as per-device initialization, not module initialization;
- `devm_kzalloc()` tied to `&pdev->dev`;
- `platform_set_drvdata()` and `platform_get_drvdata()`;
- `class_create()` and `device_create_with_groups()`;
- a sysfs attribute group created at class-device add time;
- `sysfs_emit()`, `kstrtobool()`, and locking in sysfs callbacks;
- cleanup order during module unload.

## Kernel Version Assumptions

Build against the headers for the exact kernel that will load the module:

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

This example was checked against local Ubuntu `6.8.0-124-generic` headers and assumes:

- `class_create(const char *name)`;
- `device_create_with_groups(...)`;
- `struct platform_driver.remove_new`;
- sysfs `show()` callbacks may use `sysfs_emit()`.

Older kernels may need `class_create(THIS_MODULE, "ldm_demo")` and the older platform `.remove` callback returning `int`.

## Files

| File | Purpose |
| --- | --- |
| `ldm_demo.c` | Learning-only module that registers a synthetic platform device, matching driver, class device, and `enabled` sysfs attribute. |
| `Makefile` | Out-of-tree Kbuild wrapper. |
| `README.md` | Build, load, test, debug, ABI, and cleanup notes. |

## Build

From this directory:

```sh
make
```

Expected build artifact:

```text
ldm_demo.ko
```

For a target kernel build tree:

```sh
make KDIR=/path/to/kernel/build \
     ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

## Load

Watch logs in one terminal:

```sh
sudo dmesg -w
```

Load the module in another terminal:

```sh
sudo insmod ./ldm_demo.ko
```

Expected log shape:

```text
lld-ldm-demo lld-ldm-demo.0: probe: created /sys/class/ldm_demo/ldm-demo0/enabled
ldm_demo: loaded synthetic platform device and driver
```

## Test

Inspect the device hierarchy, bus binding, and class view:

```sh
find /sys/devices -name 'lld-ldm-demo*'
ls -l /sys/bus/platform/devices/lld-ldm-demo.0
ls -l /sys/bus/platform/drivers/lld-ldm-demo
readlink /sys/bus/platform/devices/lld-ldm-demo.0/driver
ls -l /sys/class/ldm_demo/ldm-demo0
```

Read and write the sysfs attribute:

```sh
cat /sys/class/ldm_demo/ldm-demo0/enabled
echo 1 | sudo tee /sys/class/ldm_demo/ldm-demo0/enabled
cat /sys/class/ldm_demo/ldm-demo0/enabled
echo 0 | sudo tee /sys/class/ldm_demo/ldm-demo0/enabled
```

Expected output shape:

```text
0
1
1
0
```

The extra `1` and `0` lines come from `tee` echoing what it wrote. Invalid values fail:

```sh
echo maybe | sudo tee /sys/class/ldm_demo/ldm-demo0/enabled
```

Expected failure:

```text
tee: /sys/class/ldm_demo/ldm-demo0/enabled: Invalid argument
```

## Debug

Useful commands:

```sh
dmesg | tail -50
lsmod | grep ldm_demo
modinfo ./ldm_demo.ko
modinfo -F alias ./ldm_demo.ko
cat /sys/bus/platform/devices/lld-ldm-demo.0/modalias
readlink /sys/class/ldm_demo/ldm-demo0/device
readlink /sys/bus/platform/devices/lld-ldm-demo.0/driver
```

Expected module alias shape:

```text
platform:lld-ldm-demo
```

If `probe()` does not appear in `dmesg`, check whether the platform device exists and whether the driver is registered:

```sh
ls /sys/bus/platform/devices/ | grep lld-ldm-demo
ls /sys/bus/platform/drivers/ | grep lld-ldm-demo
```

## Unload

```sh
sudo rmmod ldm_demo
dmesg | tail -20
```

Expected log shape:

```text
lld-ldm-demo lld-ldm-demo.0: remove: unregistering class device
ldm_demo: unloaded
```

After unload, these paths should disappear:

```sh
ls /sys/bus/platform/devices/lld-ldm-demo.0
ls /sys/class/ldm_demo/ldm-demo0
```

## Cleanup And Error Paths

Initialization acquires objects in this order:

```text
class_create()
  -> platform_driver_register()
  -> platform_device_register_simple()
  -> platform bus match
  -> probe()
  -> devm_kzalloc()
  -> device_create_with_groups()
```

Unload and failure paths unwind in reverse ownership order:

```text
platform_device_unregister()
  -> driver remove()
  -> device_unregister(class device)
  -> devm-managed memory released
platform_driver_unregister()
class_destroy()
```

If platform-device registration fails, the module unregisters the driver and destroys the class. If class-device creation fails in `probe()`, the probe returns an error and the driver core treats the device as unbound; `devm_kzalloc()` memory is released automatically.

The sysfs attribute is created through an attribute group at device-add time. That avoids the common race where userspace receives an add event before expected attributes exist.

## Userspace ABI Impact

This example exports one userspace-visible ABI:

```text
/sys/class/ldm_demo/ldm-demo0/enabled
```

ABI behavior:

- read returns `0\n` or `1\n`;
- writes accept common boolean strings such as `0`, `1`, `true`, `false`, `y`, `n`;
- invalid writes return `-EINVAL`;
- the value is protected by a mutex because sysfs callbacks can run concurrently.

There is no `/dev` node and no ioctl ABI. In a product driver, the sysfs file name, accepted values, return values, permissions, and semantics become a compatibility contract with userspace.

## Why This Is Not Production-Ready

This module is intentionally small and synthetic. Real drivers would add:

- real hardware discovery from Device Tree, ACPI, PCI, USB, I2C, SPI, or another bus;
- subsystem integration instead of a private class when a standard framework exists;
- documented sysfs ABI under the kernel ABI documentation style;
- stronger remove-while-userspace-accesses-device testing;
- PM callbacks, parent relationships, and resource ownership tied to real hardware;
- fault-injection tests for every init and probe failure path.

Use it to study object relationships and cleanup order, not as a shipping driver template.
