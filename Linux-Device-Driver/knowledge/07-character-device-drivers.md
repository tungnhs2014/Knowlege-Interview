# 07 - Character Device Drivers

## Learning Goal

After this chapter, you should be able to explain how a userspace file such as `/dev/demochar0` reaches kernel driver code, then design a small character driver with correct registration, file callbacks, user-copy handling, cleanup, locking, and ABI behavior.

By the end, you should be able to:

- Explain the path from `open/read/write/lseek/ioctl/poll/close` to `struct file_operations`.
- Register a character device using dynamic `dev_t` allocation, `struct cdev`, `class_create()`, and `device_create()`.
- Use `struct inode`, `struct file`, `file->private_data`, and `container_of()` correctly.
- Implement seekable buffer-style `.read`, `.write`, and `.llseek` callbacks.
- Describe blocking readiness, `O_NONBLOCK`, wait queues, `poll/select`, and ioctl command dispatch.
- Unwind failures in reverse order without leaking `/dev` nodes, `cdev`s, or device numbers.
- Recognize the interview traps around user pointers, partial I/O, `poll_wait()`, ioctl ABI, and cleanup.

## Why This Matters In Real Work

Character drivers are often the first custom Linux driver interface an Embedded Linux developer builds. They are simple enough for bring-up, but serious enough that mistakes become ABI bugs, memory safety bugs, race conditions, or unload failures.

Common real-world uses:

- Board bring-up access to registers, FPGA blocks, firmware mailboxes, or test hardware.
- Factory-test and diagnostic interfaces before a full subsystem driver exists.
- Byte-stream devices such as UART-like endpoints or small FIFOs.
- Finite memory/register windows that userspace reads, writes, and seeks.
- Command/control paths when read/write alone cannot express an operation.

The hard parts are not the API names. The hard parts are:

- **Stable userspace ABI**: node name, permissions, blocking behavior, ioctl numbers, errno, EOF, and partial I/O become promises.
- **Safe user access**: `__user` pointers are untrusted and must go through user-copy helpers.
- **Lifetime discipline**: callbacks may run after `cdev_add()` succeeds, and open file descriptors may outlive simple assumptions.
- **Cleanup correctness**: failure paths must undo only what succeeded, in reverse order.
- **Concurrency**: multiple processes can open the same device and race on shared buffers or hardware state.

## Mental Model

A character driver makes a device look like a file under `/dev`. Userspace uses normal file syscalls, the VFS routes those calls to callbacks in your driver, and your driver translates each callback into buffer or hardware behavior.

Split the path into two phases.

During `open()`, the kernel resolves the device identity:

```text
userspace
  -> open("/dev/demochar0")
  -> VFS
  -> character device inode with dev_t major/minor
  -> registered struct cdev
  -> struct file_operations
  -> .open(inode, file)
  -> file->private_data = driver state
```

After `open()` succeeds, later operations use the already-open file:

```text
userspace
  -> read/write/lseek/ioctl/poll/close(fd)
  -> struct file
  -> file->f_op callback
  -> file->private_data
  -> driver state / hardware
```

Key mental anchors:

| Concept | What to remember |
| --- | --- |
| `/dev/demochar0` | User-visible device node and part of the ABI. |
| `dev_t` | Packed device identity: major + minor. |
| Major number | Selects the registered driver/range. Prefer dynamic allocation. |
| Minor number | Selects the device instance handled by that driver. |
| `struct cdev` | Kernel object that binds device numbers to callbacks. |
| `struct file_operations` | Callback table used by VFS for file syscalls. |
| `struct inode` | Represents the device file identity; has `inode->i_cdev`. |
| `struct file` | Represents one open file description; has `f_pos`, `f_flags`, and `private_data`. |

For a multi-device driver:

- Allocate one major with several minors, or a range of `dev_t` values.
- Embed `struct cdev` inside each per-device object.
- In `.open()`, recover the parent object:

  ```c
  dev = container_of(inode->i_cdev, struct demo_dev, cdev);
  filp->private_data = dev;
  ```

- In later callbacks, use `file->private_data` instead of guessing from the path or relying on unrelated globals.

**Interview trap:** `device_create()` can create `/dev` visibility, but it does not register callbacks. `cdev_add()` is what makes the VFS able to reach your `file_operations`.

## Kernel Mechanism

The character device path has three separate jobs: reserve identity, register callbacks, and publish a userspace-visible node. Keeping those jobs separate makes lifecycle and debugging much easier.

### 1. Reserve Device Numbers

Device numbers identify character device files across the system.

- Dynamic allocation:
  - Use `alloc_chrdev_region(&devt, first_minor, count, name)`.
  - The kernel chooses an available major.
  - This is the usual production choice.
- Static allocation:
  - Use `register_chrdev_region(devt, count, name)`.
  - Only use this when a fixed major/minor is truly required by platform policy.
- Cleanup:
  - Use `unregister_chrdev_region(devt, count)`.

Helpers:

- `MAJOR(devt)` extracts the major.
- `MINOR(devt)` extracts the minor.
- `MKDEV(major, minor)` builds a `dev_t`.
- `imajor(inode)` and `iminor(inode)` read major/minor from an inode.

**Production rule:** prefer dynamic major allocation. Hard-coded majors create conflicts when the same module is loaded on a different system.

### 2. Register `struct cdev`

`struct cdev` connects a `dev_t` range to your `struct file_operations`.

Registration flow:

- Initialize driver state first:
  - buffers;
  - locks;
  - wait queues;
  - counters;
  - per-device objects.
- Initialize the cdev:
  - `cdev_init(&dev->cdev, &demo_fops)`;
  - set `dev->cdev.owner = THIS_MODULE` for module drivers.
- Add it:
  - `cdev_add(&dev->cdev, dev->devt, 1)`.
- Delete it on cleanup:
  - `cdev_del(&dev->cdev)`.

**Important lifetime boundary:** after `cdev_add()` succeeds, callbacks can be reached. Do not call it before the object containing the `cdev` is fully initialized.

### 3. Publish `/dev` Visibility

`class_create()` and `device_create()` publish device information through sysfs. On typical systems, devtmpfs or udev creates `/dev/<name>` from that information.

Publication flow:

- `class_create(...)` creates `/sys/class/<class>/`.
- `device_create(class, parent, devt, drvdata, namefmt, ...)` creates a device entry and emits a uevent.
- `device_destroy(class, devt)` removes the device publication.
- `class_destroy(class)` removes the class.

Debug path:

- `ls -l /dev/demochar0` shows type and major/minor.
- `cat /sys/class/demochar/demochar0/dev` shows the major:minor created by the driver.
- `cat /proc/devices` shows registered character majors.
- `dmesg` should show init, major/minor, open, read/write, ioctl, and cleanup logs.

Kernel-version caveats:

- Modern kernels commonly use `class_create("name")`.
- Older examples may use `class_create(THIS_MODULE, "name")`.
- Modern `.poll` callbacks commonly return `__poll_t`; older examples often use `unsigned int`.
- Verify examples against your target kernel headers before copying signatures.

## Key Structs And APIs

Do not memorize this section as a flat list. Each object exists to answer one question: "What device is this?", "Which callbacks handle it?", "Which open file is this?", or "How do I safely move data?"

### Core Objects

| Object | Role | Common mistake |
| --- | --- | --- |
| `dev_t` | Major/minor device identity. | Assuming allocation creates `/dev` automatically. |
| `struct cdev` | Registered character-device object. | Thinking `cdev_init()` is enough; `cdev_add()` makes it live. |
| `struct file_operations` | VFS callback table. | Adding callbacks without defining ABI behavior. |
| `struct inode` | Device-file identity; `inode->i_cdev` points to the registered cdev. | Storing per-open state here. |
| `struct file` | One open file description with `f_pos`, `f_flags`, `f_mode`, `private_data`. | Forgetting duplicated FDs can share one `struct file`. |
| `struct class` | Sysfs class used for userspace publication policy. | Treating it as callback registration. |
| `struct device` | Published device instance under sysfs. | Forgetting `device_destroy()` on failure paths. |
| `wait_queue_head_t` | Queue used by blocking I/O and `poll()`. | Assuming it wakes itself without `wake_up_interruptible()`. |
| `poll_table` | Poll infrastructure object passed to `.poll`. | Assuming `poll_wait()` sleeps or means ready. |

### Registration APIs

Use these in pairs:

| Acquire / publish | Undo |
| --- | --- |
| `alloc_chrdev_region()` or `register_chrdev_region()` | `unregister_chrdev_region()` |
| `cdev_init()` + `cdev_add()` | `cdev_del()` |
| `class_create()` | `class_destroy()` |
| `device_create()` | `device_destroy()` |

Error helpers:

- `IS_ERR(ptr)` checks error-encoded pointers from APIs such as `class_create()` and `device_create()`.
- `PTR_ERR(ptr)` converts an error-encoded pointer to a negative errno.

### File Operation Callbacks

| Callback | Userspace operation | Driver responsibility |
| --- | --- | --- |
| `.open` | `open()` | Select device, initialize per-open state, set `file->private_data`. |
| `.release` | final close of the open file description | Free per-open state; do not free shared device state blindly. |
| `.read` | `read()` | Copy data to userspace, advance `*ppos`, return actual bytes or EOF/error. |
| `.write` | `write()` | Copy data from userspace, advance `*ppos`, return actual bytes or error. |
| `.llseek` | `lseek()` | Validate and update file position, or disable seeking intentionally. |
| `.poll` | `poll()` / `select()` | Register wait queues and return current readiness mask. |
| `.unlocked_ioctl` | `ioctl()` | Decode commands, validate ABI, copy structured args safely. |
| `.compat_ioctl` | 32-bit userspace on 64-bit kernel | Translate ABI when struct layout or pointer size differs. |

### User-Copy Helpers

User pointers are marked with `__user` because they are not normal kernel pointers. They may be invalid, unmapped, swapped, malicious, or inaccessible in the current context.

Use:

- `copy_from_user(kernel_dst, user_src, n)` for user-to-kernel buffers.
- `copy_to_user(user_dst, kernel_src, n)` for kernel-to-user buffers.
- `get_user(x, user_ptr)` for one scalar read.
- `put_user(x, user_ptr)` for one scalar write.
- `access_ok(user_ptr, size)` only as a validation helper, often in ioctl preflight; it is not a substitute for copy helpers.

Return-value rule:

- `copy_to_user()` and `copy_from_user()` return **bytes not copied**.
- `0` means complete success.
- Nonzero means partial or failed copy.
- If no progress was made, return `-EFAULT`.
- If some bytes were transferred, returning the partial byte count is usually a valid short I/O result.

**Warning:** never dereference a `__user` pointer directly.

### Ioctl Encoding

`ioctl()` is part of the userspace ABI. Treat command numbers like exported function signatures.

Common macros:

| Macro | Direction from userspace perspective | Typical driver action |
| --- | --- | --- |
| `_IO(type, nr)` | No payload | Perform command such as reset. |
| `_IOR(type, nr, datatype)` | User reads data | Driver copies data to userspace. |
| `_IOW(type, nr, datatype)` | User writes data | Driver copies data from userspace. |
| `_IOWR(type, nr, datatype)` | User reads and writes | Driver copies in, updates, copies out. |

Validation helpers:

- `_IOC_TYPE(cmd)` checks command magic.
- `_IOC_NR(cmd)` checks command number.
- `_IOC_DIR(cmd)` checks direction.
- `_IOC_SIZE(cmd)` checks encoded argument size.

**Interview trap:** unsupported ioctl commands should normally return `-ENOTTY`, not `-EINVAL`.

## Lifecycle / Data Flow

A character driver is mostly a lifetime and data-flow problem. The code is safer when acquisition and cleanup are mirror images.

### Module Init / Probe Flow

```text
module init / probe
  -> allocate per-device objects
  -> alloc_chrdev_region()
  -> initialize locks, buffers, wait queues, counters
  -> cdev_init()
  -> cdev_add()             # callbacks can run after this
  -> class_create()
  -> device_create()        # /dev node can appear through devtmpfs/udev
```

Before `cdev_add()`:

- [ ] Per-device object is allocated and will outlive callbacks.
- [ ] `struct cdev` is embedded in or strongly owned by the device object.
- [ ] Locks are initialized.
- [ ] Buffers and counters are initialized.
- [ ] Wait queues are initialized if blocking I/O or `poll()` exists.
- [ ] `struct file_operations` points to valid callbacks.

### Open Flow

```text
open("/dev/demochar0")
  -> VFS resolves the character-device inode
  -> inode identifies dev_t major/minor
  -> character core finds struct cdev
  -> VFS calls .open(inode, file)
  -> driver stores state in file->private_data
```

In `.open()`:

- Use `container_of(inode->i_cdev, struct demo_dev, cdev)` when `cdev` is embedded.
- Optionally check `iminor(inode)` for minor routing.
- Store per-device or per-open state in `file->private_data`.
- Inspect `file->f_flags` for behavior such as `O_NONBLOCK`.
- Allocate per-open context only if the driver needs state unique to this open file description.

Ownership rule:

- Per-device state belongs to init/probe and exit/remove.
- Per-open state belongs to `.open()` and `.release()`.
- **Do not free shared device state in `.release()` just because one process closed a file descriptor.**

### Read / Write / Seek Flow

For a finite seekable memory-buffer style driver:

```text
.read(file, ubuf, count, ppos)
  -> validate *ppos
  -> if *ppos >= data length, return 0 for EOF
  -> clamp count to available bytes
  -> copy_to_user()
  -> advance *ppos by actual bytes copied
  -> return actual bytes copied

.write(file, ubuf, count, ppos)
  -> validate *ppos
  -> if no space, return -ENOSPC
  -> clamp count to available space
  -> copy_from_user()
  -> update length and *ppos
  -> return actual bytes copied

.llseek(file, offset, whence)
  -> calculate new position
  -> reject invalid positions
  -> update file->f_pos
  -> return new position
```

Behavior choices:

- `read()` returning `0` means EOF for a finite seekable buffer.
- Short read/write is a normal successful transfer.
- A stream-like device may ignore `f_pos` and use `no_llseek`.
- A fixed-size buffer can use `fixed_size_llseek()`.
- A temporarily empty stream should usually sleep, return `-EAGAIN` for `O_NONBLOCK`, or report hangup/error according to its ABI; it should not accidentally look like EOF.

### Blocking Readiness And `poll()`

Blocking behavior is an ABI decision. Define what happens when data is not ready or space is not available.

Typical read behavior:

| State | Blocking fd | `O_NONBLOCK` fd |
| --- | --- | --- |
| Data available | Return bytes. | Return bytes. |
| No data yet, device still alive | Sleep interruptibly. | Return `-EAGAIN`. |
| Signal while sleeping | Return `-ERESTARTSYS` or related restart behavior. | Usually not sleeping. |
| End of finite seekable data | Return `0` EOF. | Return `0` EOF. |

`poll()` flow:

```text
.poll(file, wait)
  -> poll_wait(file, &readq, wait)
  -> poll_wait(file, &writeq, wait)
  -> lock and check current state
  -> return POLLIN/POLLRDNORM if readable
  -> return POLLOUT/POLLWRNORM if writable
```

When state changes:

- Data arrives:
  - update state under the right lock;
  - call `wake_up_interruptible(&readq)`.
- Space becomes available:
  - update state;
  - call `wake_up_interruptible(&writeq)`.

**Interview trap:** `poll_wait()` does not sleep and does not mean the device is ready. It only registers the wait queue so the poll core knows where to sleep if your returned mask says "not ready".

### Ioctl Flow

`ioctl()` handles command-style operations that do not fit cleanly into read/write.

```text
.unlocked_ioctl(file, cmd, arg)
  -> validate _IOC_TYPE(cmd)
  -> validate _IOC_NR(cmd)
  -> validate _IOC_SIZE(cmd) for commands with payload
  -> copy_from_user() for _IOW/_IOWR input
  -> perform operation under correct lock
  -> copy_to_user() for _IOR/_IOWR output
  -> return 0, positive result if ABI defines it, or negative errno
```

ABI rules:

- Put ioctl numbers and structs in a shared UAPI-style header when userspace must include them.
- Use fixed-width types such as `__u32`/`__u64` where layout matters.
- Avoid raw userspace pointers inside ioctl structs unless there is a strong reason.
- Reserve fields for future extension when the ABI may grow.
- Consider `.compat_ioctl` for 32-bit userspace on a 64-bit kernel.
- Return `-ENOTTY` for unknown or wrong-magic commands.

### Cleanup / Failure Flow

Cleanup mirrors acquisition. Each error label releases only resources that were successfully acquired earlier.

```text
normal exit / remove after successful init
  -> device_destroy()
  -> class_destroy()
  -> cdev_del()
  -> unregister_chrdev_region()

if device_create() fails
  -> class_destroy()
  -> cdev_del()
  -> unregister_chrdev_region()

if class_create() fails
  -> cdev_del()
  -> unregister_chrdev_region()

if cdev_add() fails
  -> unregister_chrdev_region()
```

Common errno values:

| Errno | Typical use |
| --- | --- |
| `-ENOMEM` | Allocation failed. |
| `-ENODEV` | Device is absent, removed, or no longer available. |
| `-EBUSY` | Exclusive device already open or resource busy. |
| `-EINVAL` | Bad offset, invalid size, invalid mode, malformed argument. |
| `-EFAULT` | Bad userspace pointer or failed user copy with no progress. |
| `-ENOSPC` | No space left in a finite buffer. |
| `-EBADF` | File not opened with required read/write mode. |
| `-EAGAIN` | Nonblocking operation would block. |
| `-ERESTARTSYS` | Interrupted sleep that may be restarted. |
| `-ENOTTY` | Unsupported ioctl command for this device. |

## Minimal Practical Example

This chapter's full learning example lives in `examples/07-character-device-drivers/`. The canonical knowledge point is not the amount of code; it is the shape of the state, registration, callback routing, and cleanup.

**Learning-only example:** fixed RAM buffer character device. It is useful for registration, `file->private_data`, user-copy, file position, and cleanup. It is not production-ready hardware code.

Small state shape:

```c
#define DEMO_NAME "demochar"
#define DEMO_SIZE 128

struct demo_dev {
        dev_t devt;
        struct cdev cdev;
        struct class *class;
        struct device *device;
        struct mutex lock;
        char buf[DEMO_SIZE];
        size_t len;
};
```

Minimal callback table:

```c
static const struct file_operations demo_fops = {
        .owner = THIS_MODULE,
        .open = demo_open,
        .release = demo_release,
        .read = demo_read,
        .write = demo_write,
        .llseek = demo_llseek,
};
```

The important `.open()` pattern:

```c
static int demo_open(struct inode *inode, struct file *filp)
{
        struct demo_dev *d;

        d = container_of(inode->i_cdev, struct demo_dev, cdev);
        filp->private_data = d;
        return 0;
}
```

The important read/write pattern:

- Validate `*ppos`.
- Lock shared buffer state with a mutex.
- Clamp `count` to available data or space.
- Use `copy_to_user()` or `copy_from_user()`.
- Advance `*ppos` by actual bytes copied.
- Return actual bytes transferred, `0` for EOF, or negative errno.

Short excerpt:

```c
not_copied = copy_to_user(ubuf, d->buf + *ppos, n);
done = n - not_copied;
*ppos += done;

if (not_copied && !done)
        return -EFAULT;

return done;
```

Minimal registration shape:

```c
ret = alloc_chrdev_region(&demo.devt, 0, 1, DEMO_NAME);
if (ret)
        return ret;

mutex_init(&demo.lock);

cdev_init(&demo.cdev, &demo_fops);
demo.cdev.owner = THIS_MODULE;

ret = cdev_add(&demo.cdev, demo.devt, 1);
if (ret)
        goto err_unregister;

demo.class = class_create(DEMO_NAME);
if (IS_ERR(demo.class)) {
        ret = PTR_ERR(demo.class);
        goto err_cdev;
}

demo.device = device_create(demo.class, NULL, demo.devt, NULL,
                            DEMO_NAME "%d", MINOR(demo.devt));
if (IS_ERR(demo.device)) {
        ret = PTR_ERR(demo.device);
        goto err_class;
}
```

Minimal cleanup shape:

```c
device_destroy(demo.class, demo.devt);
class_destroy(demo.class);
cdev_del(&demo.cdev);
unregister_chrdev_region(demo.devt, 1);
```

What this example intentionally omits:

- Real hardware registers, IRQs, DMA, runtime PM, and device tree matching.
- Blocking reads/writes and `poll()`.
- `ioctl()` ABI headers and compatibility handling.
- Multi-minor routing.
- Remove-while-open lifetime hardening.

**Production rule:** use this example to learn the lifecycle. Do not ship this pattern unchanged for real hardware without defining ABI, locking, lifetime, blocking, and teardown behavior.

## Common Bugs And Debugging

Start from the symptom, then decide whether the failure is in registration, `/dev` publication, callback routing, data transfer, blocking readiness, ioctl ABI, or cleanup.

### `/dev` Node Missing

Likely causes:

- `class_create()` failed.
- `device_create()` failed.
- devtmpfs or udev is not creating nodes.
- Driver registered `cdev` but skipped userspace publication.
- The node name differs from what userspace expects.

Inspect:

- `dmesg | tail -50`
- `cat /proc/devices`
- `ls -l /sys/class/<class>/`
- `cat /sys/class/<class>/<device>/dev`
- `ls -l /dev/<node>`

Fix pattern:

- Check `IS_ERR()` / `PTR_ERR()` after `class_create()` and `device_create()`.
- Log major/minor after allocation.
- If needed for testing, create the node manually with `mknod` using sysfs major/minor.

### `open()` Fails Or Callback Does Not Run

Likely causes:

- Wrong major/minor in `/dev` node.
- `cdev_add()` failed or was never called.
- `.open()` returned an error.
- Permissions or udev rules deny access.
- Minor routing selects no device.

Inspect:

- `ls -l /dev/<node>` for `c` type and major/minor.
- `/sys/class/<class>/<device>/dev`
- `strace -e openat,read,write,ioctl <program>`
- `pr_info()` in init and `.open()`.

**Interview trap:** a node can exist with the wrong major/minor. Always compare `/dev`, sysfs, and driver logs.

### Read/Write Size Is Wrong

Likely causes:

- Returning requested `count` instead of actual bytes copied.
- Not clamping to available data or capacity.
- Not updating `*ppos`.
- Treating short I/O as an error.
- Misinterpreting user-copy return values.

Debug pattern:

- Log `count`, clamped count, `*ppos`, buffer length, return value, and errno.
- Test with `cat`, `echo`, `dd bs=1 count=N`, and a small C program.
- Verify userspace handles partial reads and writes.

### Kernel Oops During User Access

Likely causes:

- Directly dereferencing a `__user` pointer.
- Saving a user pointer and using it later outside syscall context.
- Copying to/from an invalid userspace address.
- Holding a spinlock across user-copy and faulting.

Fix pattern:

- Use `copy_from_user()`, `copy_to_user()`, `get_user()`, or `put_user()`.
- Check their return values.
- Copy into temporary kernel memory if lock scope or latency matters.
- **Never hold a spinlock across user-copy helpers, waits, or other sleeping paths.**

### Blocking Or Nonblocking Behavior Is Wrong

Likely causes:

- Ignoring `file->f_flags & O_NONBLOCK`.
- Returning `0` for "no data yet" on a stream, causing userspace to think EOF.
- Sleeping while holding a lock needed by the producer.
- Not handling signals from `wait_event_interruptible()`.

Fix pattern:

- For `O_NONBLOCK`, return `-EAGAIN` when the operation would block.
- For blocking mode, wait on a condition that is rechecked under proper synchronization.
- Return `-ERESTARTSYS` when an interruptible wait is interrupted.
- Wake wait queues after changing readiness state.

### `poll()` Never Wakes Or Always Reports Ready

Likely causes:

- Forgot `poll_wait()`.
- Forgot `wake_up_interruptible()` after state changes.
- Returned readiness without checking actual state.
- Checked readiness without locking against producers/consumers.
- Misunderstood `poll_wait()` as the sleep point.

Fix pattern:

- Register all relevant queues in `.poll()`.
- Check state after registering queues.
- Return `POLLIN | POLLRDNORM` only when read will make progress.
- Return `POLLOUT | POLLWRNORM` only when write can make progress.
- Wake the matching queue when state changes.

### `ioctl()` Fails Across Systems

Likely causes:

- Wrong command magic or number.
- Used the wrong macro; the bidirectional macro is `_IOWR`.
- Kernel and userspace headers are out of sync.
- Struct layout differs across 32-bit and 64-bit userspace.
- Missing `.compat_ioctl`.
- Driver returns the wrong default errno.

Fix pattern:

- Validate `_IOC_TYPE`, `_IOC_NR`, `_IOC_DIR`, and `_IOC_SIZE`.
- Use fixed-width fields and reserved padding in ABI structs.
- Return `-ENOTTY` for unknown commands.
- Add explicit tests for invalid command magic, wrong size, bad pointer, and unsupported command.

### Cleanup Leaks Or Unload Fails

Likely causes:

- Failure path skips `device_destroy()`, `class_destroy()`, `cdev_del()`, or `unregister_chrdev_region()`.
- Cleanup destroys resources that were never successfully created.
- Module is still in use because a process holds the device open.
- Shared state is freed while callbacks can still access it.

Debug pattern:

- Load/unload repeatedly.
- Force failures by temporarily returning errors after each init step.
- Check `/dev`, `/sys/class`, `/proc/devices`, and `dmesg` after failed init.
- Use logs at each acquisition and cleanup label.

## Production Checklist

Use this checklist before code review, board bring-up, or interview discussion.

Registration and publication:

- [ ] Dynamic major allocation is used unless a fixed number is required.
- [ ] `dev_t` range count matches the number of minors exposed.
- [ ] Per-device state is initialized before `cdev_add()`.
- [ ] `class_create()` and `device_create()` return values are checked with `IS_ERR()`.
- [ ] `/dev` node name, permissions, ownership, and udev policy are documented.
- [ ] Cleanup is reverse order: `device_destroy()` -> `class_destroy()` -> `cdev_del()` -> `unregister_chrdev_region()`.

Callback behavior:

- [ ] `.open()` maps the correct minor/device and initializes `file->private_data`.
- [ ] `.release()` frees only per-open resources.
- [ ] `.read()` and `.write()` return actual byte counts and handle short I/O.
- [ ] EOF behavior is defined for finite buffers.
- [ ] Stream empty behavior is defined separately from EOF.
- [ ] `.llseek` uses a helper such as `fixed_size_llseek()` or is disabled with `no_llseek`.
- [ ] `.poll` uses wait queues and real readiness checks.
- [ ] `.unlocked_ioctl` validates command magic, number, direction, and size.

Safety and concurrency:

- [ ] `__user` pointers are never directly dereferenced.
- [ ] User-copy return values are checked correctly.
- [ ] Shared buffers, counters, and readiness flags are protected.
- [ ] Spinlocks are not held across user-copy, waits, or sleeping calls.
- [ ] Blocking paths handle `O_NONBLOCK` and interrupted waits.
- [ ] Remove/unload behavior is clear when file descriptors remain open.

Userspace ABI:

- [ ] Node name and ioctl command numbers are stable.
- [ ] Return values and errno behavior are documented.
- [ ] Partial read/write behavior is tested.
- [ ] Shared ioctl headers are versioned or carefully maintained.
- [ ] ABI structs use fixed-width types and avoid pointer-size surprises.
- [ ] `.compat_ioctl` is considered for 32-bit userspace on 64-bit kernels.

Debuggability:

- [ ] Init logs include major/minor and device name.
- [ ] Failure logs name the failing step.
- [ ] Callback logs include minor, count, position, and command where useful.
- [ ] Tests cover `cat`, `echo`, `dd`, small C read/write programs, `poll/select`, invalid pointers, invalid ioctl commands, and repeated load/unload.

Design sanity:

- [ ] A raw character device is really the right ABI.
- [ ] Existing subsystems have been considered first: input, IIO, GPIO, TTY, RTC, hwmon, V4L2, miscdevice, sysfs/configfs/debugfs/netlink where appropriate.
- [ ] The example code has been checked against target kernel headers, especially `class_create()` signature and `.poll` return type.

**Production rule:** prefer a subsystem framework when the device naturally fits one. A custom char ABI is flexible, but that flexibility becomes a long-term maintenance contract.

## Interview Readiness

For interviews, focus on explaining the mechanism and failure modes. Names matter, but the interviewer is usually testing whether you understand the path, lifetime, and ABI consequences.

Be ready to answer:

- What happens after userspace calls `open("/dev/demochar0")`?
- What is the difference between reserving `dev_t`, adding `cdev`, and creating a `/dev` node?
- Why is dynamic major allocation preferred?
- How does minor number routing work in a multi-device driver?
- What is the difference between `struct inode` and `struct file`?
- Why do drivers store state in `file->private_data`?
- Why is `cdev_add()` a lifetime boundary?
- How do `copy_to_user()` and `copy_from_user()` report failure?
- When should `.read()` return bytes, `0`, `-EAGAIN`, or `-ERESTARTSYS`?
- Why are partial reads/writes valid?
- What does `poll_wait()` actually do?
- How should unsupported ioctl commands fail?
- What cleanup runs if `device_create()` fails?
- Why must you not hold a spinlock across user-copy helpers?

Strong answer shape:

```text
mental model
  -> /dev node maps to dev_t major/minor
  -> dev_t maps to struct cdev
  -> cdev maps to file_operations
  -> .open stores device state in file->private_data
  -> read/write/ioctl/poll operate on that state safely
  -> cleanup unwinds publication and registration in reverse order
```

For structured practice answers, use `interview/07-character-device-drivers.md`.
