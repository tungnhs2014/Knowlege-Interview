# 04 - Kernel Logging, Error Handling, And Coding Practice Example

This is a **learning-only** Linux kernel module. It does not control hardware, create a `/dev` node, add sysfs attributes, implement ioctl, or expose a stable userspace ABI. It exists to practice logging, negative errno returns, `ERR_PTR()` handling, cleanup labels, and failure-path debugging.

## Goal

Use this example to observe how a small kernel module reports and unwinds failures:

```text
logerr_demo.c
  -> kbuild creates logerr_demo.ko
  -> insmod loads the module with optional failure injection
  -> init allocates resources step by step
  -> a selected step succeeds or fails
  -> logs explain the failure
  -> cleanup labels release only completed allocations
  -> rmmod releases resources on the success path
```

The code demonstrates:

- negative errno returns such as `-ENOMEM`, `-ENODEV`, `-EBUSY`, and `-EIO`;
- a pointer-returning helper using `ERR_PTR()`;
- caller-side `IS_ERR()` and `PTR_ERR()`;
- `pr_info()`, `pr_warn()`, `pr_err()`, and `pr_debug()`;
- `pr_fmt()` for consistent message prefixes;
- cleanup labels in reverse allocation order;
- dynamic debug workflow for `pr_debug()` messages.

## Kernel Version Assumptions

Build against the headers for the exact kernel you will load into:

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

The example uses widely available module APIs:

- `module_init()` / `module_exit()`
- `module_param()`
- `kzalloc()`, `kmalloc()`, `kstrdup()`, `kfree()`
- `ERR_PTR()`, `IS_ERR()`, `PTR_ERR()`
- `pr_*()` logging helpers

`pr_debug()` output depends on kernel configuration. It may require `CONFIG_DYNAMIC_DEBUG`, `DEBUG`, or a dynamic debug rule at runtime.

## Files

| File | Purpose |
| --- | --- |
| `logerr_demo.c` | Learning-only kernel module with failure injection, cleanup labels, and logging. |
| `Makefile` | Out-of-tree Kbuild wrapper. |
| `README.md` | Build, load, test, debug, expected logs, and cleanup notes. |

No DTS file is needed because this module does not bind to a device. No userspace C test is needed because the observable behavior is module load/unload status, module parameters, and kernel logs.

## Build

From this directory:

```sh
make
```

Equivalent explicit command:

```sh
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

For a target kernel build tree:

```sh
make KDIR=/path/to/target/kernel/build \
     ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
```

Expected artifacts:

```text
logerr_demo.ko
logerr_demo.o
logerr_demo.mod.c
logerr_demo.mod.o
Module.symvers
modules.order
```

## Inspect Before Loading

```sh
modinfo ./logerr_demo.ko
modinfo -F license ./logerr_demo.ko
modinfo -F description ./logerr_demo.ko
modinfo -F vermagic ./logerr_demo.ko
modinfo -p ./logerr_demo.ko
```

Expected parameter shape:

```text
fail_step:Failure injection: 0=success, 1=ERR_PTR helper, 2=after rx alloc, 3=after tx alloc (int)
debug_level:Debug level: 0=quiet, 1=basic debug log (int)
```

If `vermagic` does not match the target kernel, rebuild against the correct headers.

## Load The Success Path

```sh
sudo insmod ./logerr_demo.ko
dmesg | tail -20
lsmod | grep logerr_demo
```

Expected log shape:

```text
logerr_demo: init start fail_step=0 debug_level=0
logerr_demo: loaded successfully name=logerr-demo0
```

Unload:

```sh
sudo rmmod logerr_demo
dmesg | tail -20
```

Expected unload log:

```text
logerr_demo: exit start
logerr_demo: unloaded
```

## Test Error Paths

Each `fail_step` value forces a different failure.

### 1. Error Pointer Failure

```sh
sudo insmod ./logerr_demo.ko fail_step=1
echo $?
dmesg | tail -20
lsmod | grep logerr_demo
```

Expected behavior:

- `insmod` fails.
- The module is not left loaded.
- The helper returns `ERR_PTR(-ENODEV)`.
- Init extracts the exact error using `PTR_ERR()`.
- Cleanup frees only the top-level state allocation.

Expected log shape:

```text
logerr_demo: init start fail_step=1 debug_level=0
logerr_demo: failed to build logical name: -19
```

`-19` is `-ENODEV`.

### 2. Cleanup After First Buffer

```sh
sudo insmod ./logerr_demo.ko fail_step=2
dmesg | tail -20
lsmod | grep logerr_demo
```

Expected behavior:

- `name` and `rx_buf` have been allocated.
- The simulated failure returns `-EBUSY`.
- Cleanup runs `err_free_rx`, then `err_free_name`, then `err_free_state`.

Expected log shape:

```text
logerr_demo: init start fail_step=2 debug_level=0
logerr_demo: simulated busy resource after rx allocation: -16
```

`-16` is `-EBUSY`.

### 3. Cleanup After Second Buffer

```sh
sudo insmod ./logerr_demo.ko fail_step=3
dmesg | tail -20
lsmod | grep logerr_demo
```

Expected behavior:

- `name`, `rx_buf`, and `tx_buf` have been allocated.
- The simulated setup failure returns `-EIO`.
- Cleanup runs `err_free_tx`, `err_free_rx`, `err_free_name`, and `err_free_state`.

Expected log shape:

```text
logerr_demo: init start fail_step=3 debug_level=0
logerr_demo: simulated hardware setup failure: -5
```

`-5` is `-EIO`.

## Enable Debug Messages

The module contains `pr_debug()` messages. They may not print by default.

Try loading with a debug parameter:

```sh
sudo insmod ./logerr_demo.ko debug_level=1
dmesg | tail -20
sudo rmmod logerr_demo
```

If debug lines do not appear, enable dynamic debug if your kernel supports it:

```sh
sudo mount -t debugfs none /sys/kernel/debug 2>/dev/null || true
sudo sh -c "echo 'module logerr_demo +p' > /sys/kernel/debug/dynamic_debug/control"
sudo insmod ./logerr_demo.ko debug_level=1
dmesg | tail -30
sudo rmmod logerr_demo
```

Expected debug log shape when enabled:

```text
logerr_demo: rx buffer allocated for logerr-demo0
logerr_demo: tx buffer allocated for logerr-demo0
logerr_demo: debug breadcrumb: buffers are ready
```

Disable the dynamic debug rule:

```sh
sudo sh -c "echo 'module logerr_demo -p' > /sys/kernel/debug/dynamic_debug/control"
```

## Console Log Level

A message can exist in the kernel log buffer even if it does not appear on the console.

```sh
cat /proc/sys/kernel/printk
dmesg -w
sudo dmesg -n 8
```

Use `dmesg` or `journalctl -k` for evidence before changing driver log severity.

## Userspace ABI Impact

This example has **no stable userspace ABI**:

- no `/dev` node;
- no ioctl commands;
- no custom sysfs attributes;
- no procfs/debugfs files;
- no device tree binding.

It does expose module parameters while loaded:

```text
/sys/module/logerr_demo/parameters/fail_step
/sys/module/logerr_demo/parameters/debug_level
```

These are learning controls only. `fail_step` is read-only (`0444`) because changing it after init would not rerun the failure path. `debug_level` is writable by root (`0644`), but it only affects the demo's internal behavior and should not be treated as a production ABI.

## Cleanup And Error-Path Explanation

The init function acquires resources in this order:

```text
demo state
  -> logical name
  -> rx buffer
  -> tx buffer
```

Cleanup labels release them in reverse order:

```text
err_free_tx
  -> err_free_rx
  -> err_free_name
  -> err_free_state
```

Why the labels are split:

- `fail_step=1` happens before `name` is valid, so it must not free `name`.
- `fail_step=2` happens after `rx_buf`, so it frees `rx_buf`, `name`, and state.
- `fail_step=3` happens after `tx_buf`, so it frees everything.

This avoids the classic "one err bug," where one generic `err:` label frees resources that may not have been allocated.

## Production Readiness

This module is **learning-only**, not production-ready.

It is intentionally useful because it is small, deterministic, and easy to load/unload. Production driver code would additionally need:

- device-scoped `dev_*()` logging once a real `struct device *` exists;
- real hardware/resource acquisition APIs;
- `devm_*` or carefully documented manual ownership;
- concurrency handling for callbacks, timers, workqueues, IRQs, and userspace access;
- rate-limited logs for repeated failures;
- no permanent failure-injection module parameters unless they are explicitly part of a debug policy;
- target-kernel validation and review with `scripts/checkpatch.pl`.

## Cleanup Build Artifacts

Unload the module first if it is loaded:

```sh
sudo rmmod logerr_demo 2>/dev/null || true
```

Remove build artifacts:

```sh
make clean
```
