# 01 - Linux Kernel And Driver Development Overview Example

This is a **learning-only** pair of Linux kernel modules. It demonstrates that
loading driver code, registering a driver, matching a device, running
`probe()`, and removing a device are separate lifecycle events.

It does not control physical hardware and is not a template for a production
platform driver.

## Goal

Observe this sequence directly:

```text
load overview_demo_driver.ko
  -> platform driver registers
  -> no matching device exists
  -> probe does not run

load overview_demo_device.ko
  -> synthetic platform device registers
  -> platform bus matches device and driver
  -> probe allocates per-device state
  -> probe completes

unload overview_demo_device
  -> device unregisters
  -> remove runs
  -> managed cleanup runs

unload overview_demo_driver
  -> driver unregisters
```

The example reinforces four chapter-01 rules:

- A driver is a role; a module is a packaging and loading mechanism.
- Successful module loading does not prove that a device matched or probed.
- Per-device initialization belongs in `probe()`, not module initialization.
- Cleanup must follow ownership and lifecycle ordering.

## Kernel Version Assumptions

Build against the headers for the exact target kernel:

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

The example was build-checked against Ubuntu
`6.8.0-124-generic` headers. It uses the Linux 6.8-era
`struct platform_driver.remove_new` callback. Other kernel trees may use a
different removal callback field or signature, so treat the target tree's
headers as authoritative.

The modules create a synthetic platform device at runtime. They require module
loading and unloading support and sufficient privilege to use `insmod` and
`rmmod`.

## Files

| File | Purpose |
| --- | --- |
| `overview_demo_driver.c` | Registers the platform driver and implements probe, remove, managed cleanup, and forced probe failure. |
| `overview_demo_device.c` | Registers one synthetic matching platform device. |
| `Makefile` | Builds both modules with the target kernel's Kbuild system. |
| `README.md` | Build, load, test, debug, and cleanup workflow. |

No DTS is needed because the device module creates the device instance
dynamically.

## Build

From this directory:

```sh
make
```

Equivalent explicit command:

```sh
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

For a different prepared target kernel tree:

```sh
make KDIR=/path/to/target/kernel/build \
     ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

Expected primary artifacts:

```text
overview_demo_driver.ko
overview_demo_device.ko
```

Inspect their identities before loading:

```sh
modinfo ./overview_demo_driver.ko
modinfo ./overview_demo_device.ko
modinfo -F vermagic ./overview_demo_driver.ko
```

## Test Driver Registration Without Probe

Start a separate log view if convenient:

```sh
sudo dmesg -w
```

Load only the driver:

```sh
sudo insmod ./overview_demo_driver.ko
lsmod | grep overview_demo
ls /sys/bus/platform/drivers/lld-overview-demo
```

Expected log:

```text
overview_demo_driver: registering driver
```

There should be no `probe entered` message yet. This proves that module loading
and driver registration can succeed while zero devices are bound.

## Test Matching And Probe

Load the synthetic device:

```sh
sudo insmod ./overview_demo_device.ko
dmesg | tail -20
ls /sys/bus/platform/drivers/lld-overview-demo
```

Expected log shape:

```text
lld-overview-demo lld-overview-demo.0.auto: probe entered
lld-overview-demo lld-overview-demo.0.auto: probe complete: instance=1
overview_demo_device: registered lld-overview-demo.0
```

The automatically assigned numeric ID may differ. `PLATFORM_DEVID_AUTO` makes
the kernel device name `lld-overview-demo.<id>.auto`, which is the name used by
sysfs and the `dev_*()` log prefix. The device module's own `pr_info()` line
prints `pdev->name` and `pdev->id` separately, so that line intentionally omits
the `.auto` suffix.

Confirm binding:

```sh
device=$(find /sys/bus/platform/devices -maxdepth 1 \
    -name 'lld-overview-demo.*' -printf '%f\n' | head -1)
readlink "/sys/bus/platform/devices/$device/driver"
```

The link should resolve to the
`/sys/bus/platform/drivers/lld-overview-demo` directory.

## Test Remove And Managed Cleanup

Unregister the device while the driver remains loaded:

```sh
sudo rmmod overview_demo_device
dmesg | tail -20
lsmod | grep overview_demo
```

Expected log shape:

```text
overview_demo_device: unregistering lld-overview-demo.0
lld-overview-demo lld-overview-demo.0.auto: remove: instance=1
lld-overview-demo lld-overview-demo.0.auto: managed cleanup: instance=1
```

The driver module should still be loaded. Unload it afterward:

```sh
sudo rmmod overview_demo_driver
dmesg | tail -20
```

Expected log:

```text
overview_demo_driver: unregistering driver
```

## Test The Probe Error Path

Load the driver with forced probe failure, then create the device:

```sh
sudo insmod ./overview_demo_driver.ko fail_probe=1
sudo insmod ./overview_demo_device.ko
dmesg | tail -30
```

Expected log shape:

```text
overview_demo_driver: registering driver
lld-overview-demo lld-overview-demo.0.auto: probe entered
lld-overview-demo lld-overview-demo.0.auto: forced probe failure
lld-overview-demo lld-overview-demo.0.auto: managed cleanup: instance=1
overview_demo_device: registered lld-overview-demo.0
```

Both modules can be loaded even though binding failed. Confirm that the device
has no driver link:

```sh
device=$(find /sys/bus/platform/devices -maxdepth 1 \
    -name 'lld-overview-demo.*' -printf '%f\n' | head -1)
test ! -L "/sys/bus/platform/devices/$device/driver"
echo $?
```

An exit status of `0` means the device exists but is not bound.

Retry probe without reloading either module:

```sh
echo 0 | sudo tee /sys/module/overview_demo_driver/parameters/fail_probe
echo "$device" | sudo tee /sys/bus/platform/drivers_probe
dmesg | tail -20
readlink "/sys/bus/platform/devices/$device/driver"
```

Probe should now complete and the driver link should appear.

Clean up in dependency order:

```sh
sudo rmmod overview_demo_device
sudo rmmod overview_demo_driver
```

## Debug

Useful checks:

```sh
dmesg | tail -50
journalctl -k -n 50
lsmod | grep overview_demo
modinfo -F vermagic ./overview_demo_driver.ko
ls /sys/bus/platform/devices | grep lld-overview-demo
ls /sys/bus/platform/drivers/lld-overview-demo
```

Interpret failures by lifecycle stage:

| Observation | Likely stage |
| --- | --- |
| Build fails | Source, API, configuration, or header mismatch |
| `insmod` reports invalid module format | Wrong target kernel, configuration, architecture, or signing policy |
| Driver loads but no probe log appears | No matching device exists yet |
| Device exists but has no `driver` link | Matching or probe failed |
| Probe enters but does not complete | Initialization or forced failure path |
| Remove or cleanup log is missing | Teardown did not reach the expected stage |

## Cleanup And Error Paths

The driver owns per-device state through device-managed allocation:

```text
probe
  -> devm_kzalloc()
  -> devm_add_action_or_reset()
  -> optional forced failure
       -> managed cleanup runs
       -> probe returns -EIO
  -> success

device removal
  -> remove callback runs
  -> managed cleanup runs during detach
```

The synthetic device module owns the `struct platform_device` it registers:

```text
module init -> platform_device_register_data()
module exit -> platform_device_unregister()
```

Production drivers must additionally order hardware shutdown, IRQ
synchronization, work cancellation, subsystem unregistration, open-user
lifetime, power management, and failure recovery according to their actual
bus and subsystem contracts.

## Userspace ABI Impact

This example creates no device node, ioctl, custom sysfs attribute, procfs
entry, or debugfs entry. It therefore defines **no supported userspace ABI**.

The objects visible under `/sys/bus/platform/` and the writable module
parameter are training and debugging surfaces for this example, not a product
interface. A production driver should expose functionality through the
appropriate kernel subsystem and its documented ABI.

## Why This Is Not Production-Ready

- It creates a synthetic device instead of discovering or describing hardware.
- It performs no MMIO, IRQ, DMA, clock, reset, regulator, GPIO, or power work.
- It registers no functional subsystem object.
- It uses logs to make lifecycle stages visible.
- Its forced failure parameter exists only for teaching cleanup behavior.
- It was compile-checked, not tested on physical hardware.

Continue with topic 02 for full environment and build-flow setup, topic 03 for
module mechanics, and topic 09 for platform-driver design in depth.
