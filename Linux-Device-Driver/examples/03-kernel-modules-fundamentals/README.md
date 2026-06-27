# 03 - Kernel Modules Fundamentals Example

This is a **learning-only** Linux kernel module. It does not control hardware and does not create a device node. It exists to practice the smallest useful module workflow: build, inspect metadata, load with parameters, inspect sysfs, trigger an init failure, read logs, and unload cleanly.

## Goal

Use this example to see the module lifecycle directly:

```text
kmodfund.c
  -> kbuild creates kmodfund.ko
  -> modinfo reads MODULE_* and parameter metadata
  -> insmod loads the module and applies parameters
  -> module_init() allocates state and prints logs
  -> /sys/module/kmodfund/ exposes runtime module state
  -> rmmod calls module_exit()
  -> cleanup frees allocated state
```

The code demonstrates:

- `module_init()` and `module_exit()`;
- `__init` and `__exit`;
- `MODULE_LICENSE()`, `MODULE_AUTHOR()`, and `MODULE_DESCRIPTION()`;
- `module_param()`, `module_param_array()`, and `MODULE_PARM_DESC()`;
- parameter visibility under `/sys/module/kmodfund/parameters/`;
- a deliberate init failure path using `fail_after_alloc=1`;
- reverse-order cleanup for allocated memory.

## Kernel Version Assumptions

Build and load this module against the headers for the exact kernel you are running:

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

The example uses octal module-parameter permissions such as `0444` and `0644`. Older tutorials may show symbolic forms such as `S_IRUGO`; check your target kernel before copying old code.

The external module build uses the classic, widely supported Kbuild form:

```sh
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

## Files

| File | Purpose |
| --- | --- |
| `kmodfund.c` | Learning-only module with parameters, metadata, allocation, logs, and cleanup. |
| `Makefile` | Out-of-tree Kbuild wrapper. |
| `README.md` | Build, load, test, debug, and cleanup workflow. |

No DTS snippet is needed because this module does not bind to hardware. No userspace C program is needed because the useful interfaces here are kernel logs, `modinfo`, and `/sys/module`.

## Build

From this directory:

```sh
make
```

Equivalent explicit command:

```sh
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

For a cross-compiled target kernel, pass the target kernel build directory and toolchain variables:

```sh
make KDIR=/path/to/target/kernel/build \
     ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
```

Expected build artifacts include:

```text
kmodfund.ko
kmodfund.o
kmodfund.mod.c
kmodfund.mod.o
Module.symvers
modules.order
```

## Inspect Before Loading

Use `modinfo` to inspect `.modinfo` metadata without loading the module:

```sh
modinfo ./kmodfund.ko
modinfo -F license ./kmodfund.ko
modinfo -F description ./kmodfund.ko
modinfo -F vermagic ./kmodfund.ko
modinfo -p ./kmodfund.ko
```

Expected shape:

```text
license:        GPL
description:    Learning-only module fundamentals demo
vermagic:       <running-kernel-release> ...
parm:           debug_level:Debug level: 0=off, 1=basic, 2=verbose (int)
parm:           device_name:Logical name printed by the demo module (charp)
parm:           fail_after_alloc:Force init failure after allocation (bool)
parm:           values:Optional integer list, up to 4 values (array of int)
```

If `vermagic` clearly does not match the target kernel, rebuild against the correct headers before loading.

## Load

Load with parameters:

```sh
sudo insmod ./kmodfund.ko debug_level=1 device_name=demo0 values=10,20,30
dmesg | tail -20
lsmod | grep kmodfund
```

Expected log shape:

```text
kmodfund: init start name=demo0 debug=1
kmodfund: values[0]=10
kmodfund: values[1]=20
kmodfund: values[2]=30
kmodfund: loaded message="hello from demo0"
```

Check loaded-module state:

```sh
cat /proc/modules | grep kmodfund
ls /sys/module/kmodfund
ls /sys/module/kmodfund/parameters
cat /sys/module/kmodfund/parameters/debug_level
cat /sys/module/kmodfund/parameters/device_name
```

Change the writable parameter:

```sh
echo 2 | sudo tee /sys/module/kmodfund/parameters/debug_level
cat /sys/module/kmodfund/parameters/debug_level
```

`device_name`, `fail_after_alloc`, and `values` are read-only in sysfs because this example gives them `0444` permissions. `debug_level` uses `0644`, so root can change it while the module is loaded.

## Unload

```sh
sudo rmmod kmodfund
dmesg | tail -20
lsmod | grep kmodfund
```

Expected log shape:

```text
kmodfund: exit start
kmodfund: unloaded
```

After unload, `/sys/module/kmodfund/` should disappear.

## Test The Error Path

Use the `fail_after_alloc` parameter to force init to fail after memory allocation:

```sh
sudo insmod ./kmodfund.ko fail_after_alloc=1
echo $?
dmesg | tail -20
lsmod | grep kmodfund
```

Expected behavior:

- `insmod` fails.
- `dmesg` shows the simulated failure.
- `lsmod` does not show `kmodfund`.
- the init error path frees `demo_buf` before returning `-EINVAL`.

Expected log shape:

```text
kmodfund: init start name=kmodfund0 debug=0
kmodfund: simulated init failure after allocation
```

This tests a real module habit: every init step that acquires something must have an error path that releases it if a later step fails.

## Debug

Useful commands:

```sh
dmesg | tail -50
journalctl -k -n 50
modinfo ./kmodfund.ko
modinfo -p ./kmodfund.ko
modinfo -F vermagic ./kmodfund.ko
lsmod | grep kmodfund
cat /proc/modules | grep kmodfund
ls /sys/module/kmodfund
ls /sys/module/kmodfund/parameters
```

If load fails with `Invalid module format`:

```sh
uname -r
modinfo -F vermagic ./kmodfund.ko
file ./kmodfund.ko
```

Likely causes are wrong kernel headers, wrong architecture, or a target/host build mix-up.

If load fails with `Unknown symbol`:

```sh
dmesg | tail -50
modinfo -F depends ./kmodfund.ko
```

This example should not have external module dependencies, so an unknown-symbol error usually means it was built against the wrong or incomplete kernel build tree.

## Cleanup And Error Paths

Successful initialization acquires one resource:

```text
kzalloc()
```

Normal unload releases it:

```text
kfree()
```

The simulated failure path mirrors that ownership:

```text
kzalloc()
  -> fail_after_alloc requested
  -> kfree()
  -> return -EINVAL
```

This example is intentionally smaller than a real driver, but the pattern scales:

```text
init:  acquire A -> acquire B -> acquire C
error: release C? -> release B? -> release A?
exit:  release C -> release B -> release A
```

The key rule is to release only what was successfully acquired, and to release it in reverse order.

## Userspace ABI Impact

This module does not create a device node, ioctl, procfs file, debugfs file, or stable userspace API.

It does expose module parameters through sysfs while loaded:

```text
/sys/module/kmodfund/parameters/debug_level
/sys/module/kmodfund/parameters/device_name
/sys/module/kmodfund/parameters/fail_after_alloc
/sys/module/kmodfund/parameters/values
```

For this learning example, those files are just teaching aids. In a product driver, module parameter names, meanings, valid ranges, and write permissions become operational behavior that scripts and support teams may depend on.

## Why This Is Not Production-Ready

This module is **learning-only**.

Production code would need more design work:

- no real hardware binding, `probe()`, `remove()`, IRQs, DMA, runtime PM, or Device Tree matching;
- no subsystem registration or `MODULE_DEVICE_TABLE()` alias for autoloading;
- no validation range for `debug_level` or `values`;
- no locking around runtime parameter changes because the values are not used after init in a critical path;
- no module signing, packaging, DKMS, or target-rootfs install workflow;
- no fault-injection coverage beyond one deliberate init failure.

Use this example to practice module mechanics. Use later subsystem examples to learn real driver binding and device lifetimes.
