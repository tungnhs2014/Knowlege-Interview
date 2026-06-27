# 09 - Platform Bus And Platform Drivers Example

This is a **learning-only** platform bus example. It creates one synthetic `platform_device` and one matching `platform_driver` so you can observe driver registration, device registration, matching, `probe()`, `remove()`, `platform_set_drvdata()`, and `devm_kzalloc()` without touching real hardware.

It intentionally does **not** map fake MMIO addresses or request fake IRQs. Mapping random physical addresses is a bad learning habit and can be unsafe on a real machine.

## Goal

Use this example to see the platform bus lifecycle directly:

```text
platform_match_demo_driver.ko
  -> registers struct platform_driver
  -> appears under /sys/bus/platform/drivers/lld-platform-demo
  -> probe does not run until a matching device exists

platform_match_demo_device.ko
  -> creates one runtime platform_device named "lld-platform-demo"
  -> appears under /sys/bus/platform/devices/lld-platform-demo.<id>
  -> platform bus matches device name/id table to driver
  -> driver probe() allocates per-device state and logs platform data
```

The code demonstrates:

- `module_platform_driver()`;
- `struct platform_driver`;
- `struct platform_device_id` and `MODULE_DEVICE_TABLE(platform, ...)`;
- runtime `platform_device_register_data()`;
- `probe()` and `remove_new()`;
- per-device state with `devm_kzalloc()`;
- legacy-style platform data with `dev_get_platdata()`;
- `platform_set_drvdata()` and `platform_get_drvdata()`;
- load-order behavior: driver first or device first both work once both sides exist.

## Kernel Version Assumptions

Build and load these modules against the headers for the exact kernel you are running:

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

This example was written against headers where `struct platform_driver` supports:

```c
void (*remove_new)(struct platform_device *);
```

Older kernels may require the old callback field instead:

```c
static int platform_match_demo_remove(struct platform_device *pdev)
{
        ...
        return 0;
}

static struct platform_driver platform_match_demo_driver = {
        .probe = platform_match_demo_probe,
        .remove = platform_match_demo_remove,
        ...
};
```

The external module build uses the classic Kbuild form:

```sh
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

## Files

| File | Purpose |
| --- | --- |
| `platform_match_demo_driver.c` | Learning-only platform driver. It binds to `lld-platform-demo`, allocates per-device state, stores driver data, and logs probe/remove. |
| `platform_match_demo_device.c` | Learning-only provider module that creates one synthetic runtime `platform_device`. |
| `Makefile` | Out-of-tree Kbuild wrapper for both modules. |
| `README.md` | Build, load, test, debug, cleanup, and DTS notes. |

No userspace test program is needed because this example creates no `/dev` node, sysfs attributes, ioctl ABI, procfs file, or debugfs file. The useful surfaces are kernel logs and platform-bus sysfs.

## Build

From this directory:

```sh
make
```

Expected build artifacts include:

```text
platform_match_demo_driver.ko
platform_match_demo_device.ko
platform_match_demo_driver.o
platform_match_demo_device.o
Module.symvers
modules.order
```

For a cross-compiled target kernel, pass the target build directory and toolchain variables:

```sh
make KDIR=/path/to/target/kernel/build \
     ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

## Load And Test

Use two terminals if you want to watch logs live:

```sh
sudo dmesg -w
```

### Test 1: Load Driver First

Load the driver before any matching device exists:

```sh
sudo insmod ./platform_match_demo_driver.ko
ls /sys/bus/platform/drivers/lld-platform-demo
dmesg | tail -20
```

Expected behavior:

- the driver registers;
- `probe()` does **not** run yet;
- `/sys/bus/platform/drivers/lld-platform-demo/` exists.

Now create the matching platform device:

```sh
sudo insmod ./platform_match_demo_device.ko
dmesg | tail -30
ls /sys/bus/platform/devices/ | grep lld-platform-demo
readlink /sys/bus/platform/devices/lld-platform-demo.*/driver
cat /sys/bus/platform/devices/lld-platform-demo.*/modalias
```

Expected log shape:

```text
platform_match_demo_device: registered lld-platform-demo.<id>
lld-platform-demo lld-platform-demo.<id>: probe: name=lld-platform-demo id=<id> instance=42 label=runtime-created-platform-device
```

The exact numeric `<id>` is assigned by the kernel because the provider uses `PLATFORM_DEVID_AUTO`.

### Test 2: Unregister Device While Driver Is Loaded

Remove only the device provider:

```sh
sudo rmmod platform_match_demo_device
dmesg | tail -30
ls /sys/bus/platform/devices/ | grep lld-platform-demo
```

Expected log shape:

```text
platform_match_demo_device: unregistering lld-platform-demo.<id>
lld-platform-demo lld-platform-demo.<id>: remove: instance=42 label=runtime-created-platform-device
```

The device disappears. The driver remains registered under `/sys/bus/platform/drivers/lld-platform-demo/`.

### Test 3: Load Device First

Start from a clean state:

```sh
sudo rmmod platform_match_demo_driver 2>/dev/null || true
```

Load the device provider first:

```sh
sudo insmod ./platform_match_demo_device.ko
dmesg | tail -20
ls /sys/bus/platform/devices/ | grep lld-platform-demo
```

Expected behavior:

- the platform device exists;
- no probe occurs if the driver is not loaded.

Now load the driver:

```sh
sudo insmod ./platform_match_demo_driver.ko
dmesg | tail -30
```

Expected behavior:

- the platform bus immediately matches the already-present device;
- `probe()` runs.

## Unload

Unload in either order. A clean full unload is:

```sh
sudo rmmod platform_match_demo_device
sudo rmmod platform_match_demo_driver
dmesg | tail -30
```

If you loaded only the driver:

```sh
sudo rmmod platform_match_demo_driver
```

If a module reports `Module ... is not currently loaded`, it just means you already removed it in a previous test.

## Debug

Useful commands:

```sh
dmesg | tail -50
lsmod | grep platform_match_demo
modinfo ./platform_match_demo_driver.ko
modinfo ./platform_match_demo_device.ko
modinfo -F alias ./platform_match_demo_driver.ko
ls /sys/bus/platform/drivers/
ls /sys/bus/platform/devices/
cat /sys/bus/platform/devices/lld-platform-demo.*/modalias
readlink /sys/bus/platform/devices/lld-platform-demo.*/driver
```

Expected alias from the driver module:

```text
platform:lld-platform-demo
```

If `probe()` does not run:

- check that both modules are loaded;
- check that the device name is exactly `lld-platform-demo`;
- check that the driver directory exists under `/sys/bus/platform/drivers/`;
- check that the device directory exists under `/sys/bus/platform/devices/`;
- check `dmesg` for load errors;
- check whether you accidentally loaded only one side of the example.

## Cleanup And Error Paths

The driver module has a small error surface:

```text
platform_match_demo_probe()
  -> devm_kzalloc()
  -> platform_set_drvdata()
  -> return 0
```

If `devm_kzalloc()` fails, `probe()` returns `-ENOMEM` and binding fails. Because the allocation is device-managed, there is no manual free path in this driver.

The device provider module has this lifecycle:

```text
platform_match_demo_device_init()
  -> platform_device_register_data()
  -> save returned platform_device pointer

platform_match_demo_device_exit()
  -> platform_device_unregister()
```

If `platform_device_register_data()` fails, init returns `PTR_ERR()` and the module does not load. If it succeeds, exit unregisters the device. If a driver is bound at that moment, the platform core calls the driver's remove callback before the device disappears.

This example also shows a real lifetime rule: the platform driver does not own the `platform_device` object. The provider owns the synthetic device and unregisters it. The driver owns only the state it allocates while bound.

## Userspace ABI Impact

This example has **no userspace ABI**.

It does not create:

- `/dev` nodes;
- custom sysfs attributes;
- ioctl commands;
- procfs files;
- debugfs files.

The visible effects are kernel logs and normal platform-bus sysfs entries. That keeps the example focused on platform bus matching instead of character-device ABI design.

## Optional DTS Sketch

The runnable example uses `platform_device_register_data()` so it can work on a normal development machine without board firmware changes. A real embedded platform would usually describe the device in Device Tree instead.

Conceptual DTS shape:

```dts
demo@10000000 {
        compatible = "demo,lld-platform-demo";
        reg = <0x10000000 0x1000>;
        interrupts = <0 56 4>;
        status = "okay";
};
```

Matching driver-side sketch:

```c
static const struct of_device_id demo_of_match[] = {
        { .compatible = "demo,lld-platform-demo" },
        { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);

static struct platform_driver demo_driver = {
        .probe = demo_probe,
        .remove_new = demo_remove,
        .driver = {
                .name = "lld-platform-demo",
                .of_match_table = demo_of_match,
        },
};
```

In a real MMIO driver, `probe()` would then use:

```c
base = devm_platform_ioremap_resource(pdev, 0);
irq = platform_get_irq(pdev, 0);
```

Do not add fake `reg` or `interrupts` values to a production DTS. They must come from the SoC manual or board integration data.

## Why This Is Not Production-Ready

This example is intentionally small and learning-only:

- no real hardware;
- no real MMIO mapping;
- no IRQ handling;
- no clocks, resets, regulators, pinctrl, DMA, or runtime PM;
- no subsystem registration;
- no userspace ABI;
- no Device Tree binding schema;
- no suspend/resume or shutdown path;
- no multi-instance resource naming beyond the auto-assigned platform ID.

A production platform driver would usually:

- match through Device Tree, ACPI, or a stable subsystem-created device;
- validate all required resources;
- use named resources when a block has multiple register windows or IRQs;
- preserve `-EPROBE_DEFER` for suppliers;
- initialize hardware before exposing subsystem-visible objects;
- explicitly stop IRQs, workqueues, DMA, runtime PM, and userspace-visible paths in remove;
- include binding documentation or schema for firmware-described hardware.
