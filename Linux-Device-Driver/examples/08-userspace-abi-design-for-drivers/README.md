# 08 - Userspace ABI Design For Drivers Example

This is a **learning-only** Linux kernel module. It creates a tiny ABI surface so you can see how a driver publishes behavior to userspace and why each exported path becomes a contract.

## Goal

Practice a minimal but realistic userspace ABI:

```text
/dev/abidemo0
  -> write() creates one pending event
  -> poll() reports readable when the event exists
  -> read() consumes the event
  -> ioctl() gets/sets structured config

/sys/class/abidemo/abidemo0/mode
  -> exposes one small scalar property
```

The example demonstrates:

- a `/dev` character-device node for device-instance operations;
- `poll_wait()` plus `wake_up_interruptible()` for readiness;
- an ioctl UAPI header shared by kernel code and userspace test code;
- fixed-width ioctl fields, a `version` field, and reserved fields;
- `-ENOTTY` for unsupported ioctl commands;
- a sysfs `mode` attribute using `sysfs_emit()` and `kstrtouint()`;
- reverse-order cleanup for device node, sysfs attribute, class, `cdev`, and device number.

## Kernel Version Assumptions

Validated against local Ubuntu kernel headers for `6.8.0-124-generic`.

This code assumes:

- `class_create(const char *name)`;
- `.poll` returns `__poll_t`;
- sysfs `show()` callbacks can use `sysfs_emit()`.

Older kernels may require `class_create(THIS_MODULE, "abidemo")` or an `unsigned int` `.poll` return type. Build against the headers for the exact kernel that will load the module.

## Files

| File | Purpose |
| --- | --- |
| `abidemo.c` | Learning-only kernel module exposing `/dev`, `poll()`, ioctl, and sysfs. |
| `abidemo_uapi.h` | Shared ioctl command and payload definitions. |
| `abidemo_test.c` | Small userspace test for ioctl, `poll()`, `write()`, and `read()`. |
| `Makefile` | Builds the out-of-tree module and userspace test. |

## Build

From this directory:

```sh
make
```

This expects matching kernel headers at:

```sh
/lib/modules/$(uname -r)/build
```

For a different kernel build tree:

```sh
make KDIR=/path/to/kernel/build
```

## Load

```sh
sudo insmod abidemo.ko
dmesg | tail -20
ls -l /dev/abidemo0
cat /sys/class/abidemo/abidemo0/dev
cat /sys/class/abidemo/abidemo0/mode
```

Expected log shape:

```text
abidemo: loaded node=/dev/abidemo0 sysfs=/sys/class/abidemo/abidemo0/mode
```

If `/dev/abidemo0` is not created automatically, devtmpfs or udev may not be running. For temporary manual testing:

```sh
MAJOR=$(cat /sys/class/abidemo/abidemo0/dev | cut -d: -f1)
sudo mknod /dev/abidemo0 c "$MAJOR" 0
sudo chmod 666 /dev/abidemo0
```

## Test

Run the userspace test:

```sh
sudo ./abidemo_test
```

Expected output shape:

```text
initial config: version=1 mode=0 flags=0
poll before write: ret=0 revents=0x0
write: 10 bytes
poll after write: ret=1 revents=0x1
read: 10 bytes: abi event
```

Test the sysfs property:

```sh
cat /sys/class/abidemo/abidemo0/mode
echo 1 | sudo tee /sys/class/abidemo/abidemo0/mode
cat /sys/class/abidemo/abidemo0/mode
echo 9 | sudo tee /sys/class/abidemo/abidemo0/mode
```

The final command should fail with `Invalid argument` because the ABI only accepts modes `0` and `1`.

You can also test the data path with shell commands:

```sh
printf "hello\n" | sudo tee /dev/abidemo0
sudo dd if=/dev/abidemo0 bs=128 count=1 status=none
```

The read consumes the pending event, so a second non-blocking read from the test program would see `EAGAIN` until another write occurs.

## Debug

Useful commands:

```sh
dmesg | tail -50
sudo strace -e openat,ioctl,poll,read,write ./abidemo_test
ls -l /dev/abidemo0
cat /sys/class/abidemo/abidemo0/dev
cat /sys/class/abidemo/abidemo0/mode
```

Enable dynamic debug for this module if your kernel supports it:

```sh
sudo sh -c 'echo "module abidemo +p" > /sys/kernel/debug/dynamic_debug/control'
dmesg | tail -50
```

## Unload

```sh
sudo rmmod abidemo
dmesg | tail -20
ls -l /dev/abidemo0
```

After unload, `/dev/abidemo0` and `/sys/class/abidemo/abidemo0` should disappear on systems using devtmpfs or udev.

## Cleanup And Error Paths

Initialization acquires resources in this order:

```text
alloc_chrdev_region()
  -> cdev_add()
  -> class_create()
  -> device_create()
  -> device_create_file()
```

Unload and failure paths release resources in reverse order:

```text
device_remove_file()
  -> device_destroy()
  -> class_destroy()
  -> cdev_del()
  -> unregister_chrdev_region()
```

The cleanup labels avoid leaked device numbers, stale `/dev` nodes, sysfs files without backing state, and live VFS callbacks after `cdev` removal.

The module relies on `.owner = THIS_MODULE` to keep the module loaded while the device is open. A production hot-unplug driver still needs stronger per-device lifetime rules because physical hardware can disappear while file descriptors remain open.

## Userspace ABI Impact

This example exports real userspace-visible names and behaviors:

- `/dev/abidemo0` is a device node ABI.
- `/sys/class/abidemo/abidemo0/mode` is a sysfs ABI.
- `ABIDEMO_IOC_*` command numbers and `struct abidemo_config` layout are ioctl ABI.
- `poll()` readiness means "a pending event can be read now."
- `read()` consumes the pending event.
- invalid sysfs mode values return `-EINVAL`.
- unknown ioctl commands return `-ENOTTY`.

In a product driver, changing any of those paths, command numbers, struct fields, return values, or blocking rules can break existing applications.

## Why This Is Not Production-Ready

This module is intentionally small. Real product drivers would add:

- documented UAPI headers installed from an include/uapi-style location;
- careful permission policy through udev rules, groups, or subsystem defaults;
- compatibility review for 32-bit userspace on 64-bit kernels;
- explicit ABI documentation for every ioctl field, errno, unit, and blocking rule;
- remove-while-open handling for real hardware disappearance;
- fault-injection tests for every cleanup label;
- real subsystem integration when the device fits an existing framework.

Use this example to study ABI shape and failure behavior, not as a template for shipping a private ABI when a standard framework already exists.
