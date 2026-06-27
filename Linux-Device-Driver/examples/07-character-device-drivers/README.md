# 07 - Character Device Drivers Example

This is a **learning-only** Linux kernel module. It creates one RAM-backed character device, `/dev/demochar0`, so you can practice the character-device lifecycle without real hardware.

## Goal

Use this example to see how a userspace file operation reaches driver code:

```text
/dev/demochar0
  -> dev_t major/minor
  -> struct cdev
  -> struct file_operations
  -> open/read/write/llseek/release callbacks
```

The code demonstrates:

- dynamic `alloc_chrdev_region()`;
- `cdev_init()`, `cdev_add()`, and `cdev_del()`;
- `class_create()`, `device_create()`, and matching cleanup;
- `.open` and `.release`;
- `.read` and `.write` with `copy_to_user()` and `copy_from_user()`;
- `.llseek` using `fixed_size_llseek()`;
- basic shared-buffer locking with `struct mutex`;
- reverse-order cleanup labels after partial initialization failure.

## Kernel Version Assumptions

This example uses the modern `class_create("demochar")` signature. Older kernels may require:

```c
class_create(THIS_MODULE, "demochar");
```

This example does not implement `.poll`. If you add it later, check your target kernel headers: modern kernels commonly use `__poll_t` as the `.poll` return type, while older examples may show `unsigned int`.

Build and test against the headers for the exact kernel you will load the module into.

## Files

| File | Purpose |
| --- | --- |
| `demochar.c` | Learning-only RAM-backed character driver. |
| `Makefile` | Out-of-tree kernel module build file. |
| `README.md` | Build, load, test, debug, and cleanup notes. |

## Build

From this directory:

```sh
make
```

This expects kernel headers at:

```sh
/lib/modules/$(uname -r)/build
```

For a cross-compiled kernel tree, pass `KDIR` explicitly:

```sh
make KDIR=/path/to/kernel/build
```

## Load

```sh
sudo insmod demochar.ko
dmesg | tail -20
ls -l /dev/demochar0
cat /sys/class/demochar/demochar0/dev
cat /proc/devices | grep demochar
```

Expected log shape:

```text
demochar: loaded major=<N> minor=0 node=/dev/demochar0 size=128
```

If `/dev/demochar0` is not created automatically, check whether devtmpfs or udev is running. For temporary manual testing:

```sh
MAJOR=$(cat /sys/class/demochar/demochar0/dev | cut -d: -f1)
sudo mknod /dev/demochar0 c "$MAJOR" 0
sudo chmod 666 /dev/demochar0
```

## Test

Write and read normal text:

```sh
echo "hello char driver" | sudo tee /dev/demochar0
cat /dev/demochar0
```

Read a smaller chunk:

```sh
dd if=/dev/demochar0 bs=1 count=5 status=none
printf '\n'
```

Overwrite from the start:

```sh
printf "ABCDE" | sudo tee /dev/demochar0
cat /dev/demochar0
```

Test seek behavior:

```sh
dd if=/dev/demochar0 bs=1 skip=2 count=3 status=none
printf '\n'
```

Test the fixed 128-byte capacity:

```sh
head -c 200 /dev/zero | tr '\0' X | sudo tee /dev/demochar0 >/dev/null
wc -c < /dev/demochar0
```

The final command should report at most `128` bytes because `.write` clamps to the fixed buffer size.

## Debug

Useful commands:

```sh
dmesg | tail -50
ls -l /dev/demochar0
cat /sys/class/demochar/demochar0/dev
strace -e openat,read,write,lseek cat /dev/demochar0
```

Enable dynamic debug for this module if your kernel supports it:

```sh
sudo sh -c 'echo "module demochar +p" > /sys/kernel/debug/dynamic_debug/control'
dmesg | tail -50
```

## Unload

```sh
sudo rmmod demochar
dmesg | tail -20
ls -l /dev/demochar0
```

After unload, `/dev/demochar0` should disappear on systems using devtmpfs or udev.

## Cleanup And Error Paths

Successful initialization acquires resources in this order:

```text
alloc_chrdev_region()
  -> cdev_add()
  -> class_create()
  -> device_create()
```

Normal unload releases them in reverse order:

```text
device_destroy()
  -> class_destroy()
  -> cdev_del()
  -> unregister_chrdev_region()
```

Failure labels mirror the same ownership:

- if `device_create()` fails, the code destroys the class, deletes the `cdev`, and unregisters the device number;
- if `class_create()` fails, the code deletes the `cdev` and unregisters the device number;
- if `cdev_add()` fails, the code unregisters the device number.

This shape prevents stale `/dev` nodes, leaked class entries, live callbacks without owned state, and reserved major/minor numbers after a failed load.

## Why This Is Not Production-Ready

This module is intentionally small. Real product drivers need more design work:

- no real hardware registers, IRQs, DMA, runtime PM, or Device Tree matching;
- no blocking I/O, wait queues, or `.poll`;
- no `ioctl` UAPI, fixed-width ABI structs, or `.compat_ioctl`;
- no multi-minor routing;
- no permission or udev policy beyond the default device node;
- no remove-while-open lifetime hardening beyond `.owner = THIS_MODULE`;
- simple mutex locking around the whole user-copy path, which is acceptable for learning but may be too coarse for low-latency hardware paths;
- no fault-injection tests for every cleanup label.

Use this example to understand the lifecycle and callback mechanics. Treat production character devices as userspace ABI contracts, not just as a way to pass bytes.
