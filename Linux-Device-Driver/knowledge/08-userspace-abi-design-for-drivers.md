# 08 - Userspace ABI Design For Drivers

## Learning Goal
After this topic, you should be able to design a clean userspace interface for a driver and explain why you chose `/dev`, sysfs, ioctl, a framework ABI, or a debug-only interface.

You should be able to:

- Explain what a **userspace ABI** is and why it is a long-term promise.
- Choose between `read()`/`write()`, `poll()`, ioctl, sysfs, framework ABIs, procfs, debugfs, and tracefs.
- Implement safe user/kernel data transfer.
- Design ioctl commands and structures without breaking 32-bit/64-bit compatibility.
- Expose simple device properties through sysfs without turning sysfs into a private protocol.
- Debug common ABI failures from both userspace and kernel space.

## Why This Matters In Real Work
Every useful driver needs a way for applications, tools, or system services to interact with it. That interface is not just a test convenience; once userspace depends on it, changing it can break products in the field.

Real driver work often includes:

- Creating `/dev/<name>` nodes for device operations.
- Exposing small properties under `/sys`.
- Using existing subsystem ABIs such as GPIO, input, IIO, V4L2, RTC, watchdog, NVMEM, or ALSA.
- Adding ioctl commands for custom operations.
- Supporting blocking, non-blocking, and `poll()`/`select()` behavior.
- Keeping old userspace working while adding new capability.
- Separating production ABI from debug-only knobs.

**Production rule:** design the ABI before writing too much driver code. Bad ABI decisions are much harder to fix than bad internal function names.

## Mental Model
A userspace ABI is the driver's public contract. Kernel code can be refactored later, but userspace-visible paths, commands, meanings, units, error codes, and structure layouts are promises.

Think of the common interfaces like this:

| Interface | Mental Model | Best For | Avoid For |
| --- | --- | --- | --- |
| `/dev` + `read()`/`write()` | Device behaves like a file or stream | Byte streams, memory-like devices, simple data I/O | Many unrelated control commands |
| `/dev` + `poll()` | Userspace waits for readiness | Event-driven I/O, blocking reads/writes | Transferring data itself |
| ioctl | Custom command channel on a file descriptor | Structured commands, mode changes, querying complex state | Simple scalar properties that sysfs can express |
| sysfs | Small attributes of kernel objects | One value, simple property, topology, enumeration | Large data, binary protocols, frequent streaming |
| Framework ABI | Standard subsystem interface | Common device classes | Hardware that does not fit the subsystem contract |
| procfs | Process/system-oriented information and control | Kernel/system tuning, legacy system controls | New per-device driver ABI |
| debugfs/tracefs | Debugging and tracing | Bring-up, diagnostics, tracing | Required production userspace behavior |

The cleanest design is usually the one that gives userspace the least surprising interface.

## Core Concepts
Userspace ABI design is about selecting the right contract and preserving it.

### ABI Versus API
| Term | Meaning |
| --- | --- |
| API | Interface used by code at build time, such as a C function or header. |
| ABI | Binary or behavioral contract used at runtime, such as ioctl numbers, struct layouts, sysfs file names, `/dev` nodes, return values, and error codes. |

Kernel internal APIs can change between versions. Userspace ABI is expected to remain compatible.

### `/dev` Device Nodes
A character device uses a device node such as `/dev/my_sensor`. Userspace calls normal file syscalls:

- `open()`
- `read()`
- `write()`
- `lseek()`
- `ioctl()`
- `poll()` / `select()` / `epoll()`
- `close()`

The kernel routes those calls to the driver's `struct file_operations`.

### sysfs Attributes
Sysfs exposes kernel object properties as files under `/sys`.

Good sysfs attributes are:

- Small.
- Textual when possible.
- One value or a simple list of similar values.
- Named clearly.
- Stable once published.
- Backed by valid object lifetime.

Bad sysfs attributes are:

- Hidden command languages.
- Large streaming buffers.
- Multi-step protocols with fragile ordering.
- Debug controls required by production applications.

### ioctl
ioctl is useful when the device needs commands that do not map naturally to `read()` or `write()`.

Use ioctl for:

- Reset, mode change, calibration, or command operations.
- Structured query/set operations.
- Operations that must happen on an already-open device instance.

Be careful because ioctl ABI is easy to get wrong:

- Command numbers must be stable.
- Data structures must use userspace-safe fixed-width types.
- Pointers, `long`, `time_t`, padding, and alignment can break 32-bit userspace on 64-bit kernels.
- Unsupported commands should return `-ENOTTY`.

### Framework ABI
If the kernel already has a subsystem for your device type, prefer that ABI.

Examples:

- GPIO userspace access should use the GPIO character-device ABI, usually through libgpiod, not new GPIO sysfs users.
- NVMEM exposes registered memories through framework-created sysfs binary files.
- Input devices expose events through the input subsystem.
- V4L2 exposes video devices through standardized `VIDIOC_*` ioctls.

**Production rule:** do not invent a private ABI when a maintained kernel subsystem already solves the same user problem.

## Kernel Mechanism
The kernel implements userspace driver ABI by combining VFS, device numbers, the Linux device model, and subsystem-specific code.

### Character Device Path
The `/dev` path is routed through VFS into a registered character device.

```text
userspace app
  open("/dev/mydev")
        |
        v
VFS finds device inode
        |
        v
inode->i_cdev -> struct cdev
        |
        v
struct file_operations
        |
        v
driver .open/.read/.write/.poll/.unlocked_ioctl
```

Important relationships:

- `dev_t` stores major and minor numbers.
- Major identifies the driver/device family.
- Minor identifies a device instance.
- `struct cdev` binds a device number range to `struct file_operations`.
- `struct inode` represents the device file.
- `struct file` represents one open file descriptor.
- `file->private_data` usually points to per-device or per-open driver state.

### Device Node Creation
Modern drivers usually avoid asking users to run `mknod` manually.

Typical flow:

```text
alloc_chrdev_region()
  -> cdev_init()
  -> cdev_add()
  -> class_create()
  -> device_create()
  -> /sys/class/<class>/<device>/dev contains major:minor
  -> devtmpfs/udev creates /dev/<device>
```

`device_create()` also creates device-model state under `/sys`, which userspace tools can inspect.

### Sysfs Path
Sysfs is tied to kobjects and the Linux device model.

```text
struct device
  embeds struct kobject
        |
        v
/sys/devices/... or /sys/class/...
        |
        v
attributes call show()/store()
```

For driver-facing code, common wrappers include:

- `DEVICE_ATTR()`
- `device_create_file()`
- `device_remove_file()`
- attribute groups through `struct attribute_group`
- `sysfs_notify()` for pollable sysfs attributes

Sysfs pins the kobject while callbacks run, but the physical device may still be gone or logically unavailable. The driver must still check device state.

### Blocking And Readiness
Blocking behavior is part of the ABI.

For `read()`:

- If data is available, return data.
- If no data and `O_NONBLOCK` is set, return `-EAGAIN`.
- If no data and blocking mode is used, sleep on a wait queue.
- If interrupted by a signal, return `-ERESTARTSYS` or another appropriate error.

For `poll()`:

- Register wait queues with `poll_wait()`.
- Return readiness bits such as `POLLIN | POLLRDNORM` or `POLLOUT | POLLWRNORM`.
- Wake waiters with `wake_up_interruptible()` when state changes.

`poll_wait()` does not transfer data and does not itself sleep forever. It registers interest and lets the VFS poll machinery manage waiting.

## Key Structs And APIs
These APIs matter because they define what userspace can see and how the kernel routes calls.

### Device Numbers And Character Devices
| API / Struct | Purpose |
| --- | --- |
| `dev_t` | Encodes major and minor numbers. |
| `MAJOR(dev)`, `MINOR(dev)`, `MKDEV(major, minor)` | Convert between `dev_t` and major/minor fields. |
| `alloc_chrdev_region()` | Dynamically reserve device numbers. |
| `unregister_chrdev_region()` | Release reserved device numbers. |
| `struct cdev` | Kernel character-device object. |
| `cdev_init()` | Bind `struct cdev` to `file_operations`. |
| `cdev_add()` | Make the character device visible to VFS. |
| `cdev_del()` | Remove the character device registration. |
| `class_create()` | Create a sysfs class. |
| `device_create()` | Create device-model state and trigger `/dev` node creation. |

### File Operations
| Callback | Userspace Trigger | ABI Meaning |
| --- | --- | --- |
| `.open` | `open()` | Start an access session and attach state to `file->private_data`. |
| `.release` | last `close()` on that file | Clean up per-open state. |
| `.read` | `read()` | Copy data from kernel/device to userspace. |
| `.write` | `write()` | Copy data from userspace to kernel/device. |
| `.llseek` | `lseek()` | Define offset behavior for memory-like devices. |
| `.poll` | `poll()`, `select()`, `epoll()` | Report readiness and register wait queues. |
| `.unlocked_ioctl` | `ioctl()` | Handle custom commands. |
| `.mmap` | `mmap()` | Map device or driver memory into userspace. |

### Safe User Access
| API | Direction |
| --- | --- |
| `copy_from_user(kernel, user, n)` | Userspace to kernel. |
| `copy_to_user(user, kernel, n)` | Kernel to userspace. |
| `get_user(x, user_ptr)` | Single scalar from userspace. |
| `put_user(x, user_ptr)` | Single scalar to userspace. |

**Warning:** never directly dereference a `__user` pointer. It may fault, be unmapped, or point to hostile input.

### ioctl Helpers
| Macro / Helper | Purpose |
| --- | --- |
| `_IO(type, nr)` | Command with no data payload. |
| `_IOR(type, nr, type)` | Userspace reads data from kernel. |
| `_IOW(type, nr, type)` | Userspace writes data to kernel. |
| `_IOWR(type, nr, type)` | Bidirectional data transfer. |
| `_IOC_TYPE(cmd)` | Extract command magic/type. |
| `_IOC_NR(cmd)` | Extract command number. |
| `_IOC_SIZE(cmd)` | Extract encoded data size. |
| `compat_ioctl` | 32-bit userspace compatibility path on 64-bit kernels when needed. |

**Trap:** `_IOW` means userspace writes to the kernel, so the driver normally uses `copy_from_user()`.

### Sysfs APIs
| API / Struct | Purpose |
| --- | --- |
| `struct kobject` | Object identity, hierarchy, reference counting, sysfs backing. |
| `struct attribute` | Generic sysfs file metadata. |
| `struct device_attribute` | Device-model wrapper around an attribute plus callbacks. |
| `DEVICE_ATTR()` | Declare a device attribute. |
| `device_create_file()` / `device_remove_file()` | Add/remove device attributes. |
| `struct attribute_group` | Group related attributes. |
| `sysfs_notify()` | Wake userspace polling a sysfs attribute. |
| `sysfs_emit()` | Format sysfs `show()` output safely. |

## Lifecycle / Data Flow
The ABI lifecycle starts before userspace opens the device and continues after userspace closes it.

### Character Device Lifecycle
```text
driver probe/init
  |
  |-- allocate state
  |-- reserve dev_t
  |-- initialize cdev + file_operations
  |-- add cdev
  |-- create class/device node
  |-- expose optional sysfs attributes
  v
userspace sees /dev node and /sys entries
  |
  |-- open()
  |     driver stores state in file->private_data
  |
  |-- read()/write()/ioctl()/poll()
  |     driver validates input, locks state, handles user-copy
  |
  |-- close()
  |     driver release path runs
  v
driver remove/exit
  |
  |-- remove sysfs attributes
  |-- destroy device node
  |-- destroy class
  |-- delete cdev
  |-- unregister device numbers
  |-- free state only when no users remain
```

### Read Path
```text
read(fd, user_buf, count)
  -> .read(file, user_buf, count, &f_pos)
     -> validate file state and count
     -> if no data:
          O_NONBLOCK ? -EAGAIN : wait_event_interruptible()
     -> copy_to_user()
     -> update f_pos if device is seekable
     -> return bytes copied or negative errno
```

### Sysfs Attribute Flow
```text
cat /sys/.../mode
  -> show(dev, attr, buf)
     -> validate device still present
     -> return sysfs_emit(buf, "%u\n", mode)

echo 3 > /sys/.../mode
  -> store(dev, attr, buf, count)
     -> parse and validate
     -> lock state
     -> update mode
     -> return count or negative errno
```

### ioctl Flow
```text
ioctl(fd, MYDEV_SET_CONFIG, &cfg)
  -> .unlocked_ioctl(file, cmd, arg)
     -> validate magic and command number
     -> copy_from_user() config
     -> validate every field
     -> lock state
     -> apply command
     -> return 0 or negative errno
```

## Minimal Practical Example
This example is learning-only. It shows ABI shape, not a complete production driver.

### UAPI Header
Put stable command definitions in a header both kernel and userspace can include.

```c
/* mydev_uapi.h */
#ifndef MYDEV_UAPI_H
#define MYDEV_UAPI_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define MYDEV_IOC_MAGIC 'M'

struct mydev_config {
	__u32 size;
	__u32 mode;
	__u32 flags;
	__u32 reserved[5];
};

#define MYDEV_IOC_GET_CONFIG _IOR(MYDEV_IOC_MAGIC, 0x01, struct mydev_config)
#define MYDEV_IOC_SET_CONFIG _IOW(MYDEV_IOC_MAGIC, 0x02, struct mydev_config)

#endif
```

Important choices:

- `__u32` is fixed-width and userspace-safe.
- `reserved[]` leaves room for compatible extension.
- `_IOR` and `_IOW` are from the userspace point of view.

### Driver-Side Sketch
```c
struct mydev {
	struct cdev cdev;
	struct device *dev;
	struct mutex lock;
	wait_queue_head_t read_wq;
	bool data_ready;
	struct mydev_config cfg;
};

static ssize_t mydev_status_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct mydev *m = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%u\n", m->data_ready);
}
static DEVICE_ATTR_RO(mydev_status);

static __poll_t mydev_poll(struct file *file, poll_table *wait)
{
	struct mydev *m = file->private_data;
	__poll_t mask = 0;

	poll_wait(file, &m->read_wq, wait);

	if (READ_ONCE(m->data_ready))
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static long mydev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct mydev *m = file->private_data;
	struct mydev_config cfg;

	if (_IOC_TYPE(cmd) != MYDEV_IOC_MAGIC)
		return -ENOTTY;

	switch (cmd) {
	case MYDEV_IOC_GET_CONFIG:
		mutex_lock(&m->lock);
		cfg = m->cfg;
		mutex_unlock(&m->lock);

		if (copy_to_user((void __user *)arg, &cfg, sizeof(cfg)))
			return -EFAULT;
		return 0;

	case MYDEV_IOC_SET_CONFIG:
		if (copy_from_user(&cfg, (void __user *)arg, sizeof(cfg)))
			return -EFAULT;

		if (cfg.size > 4096 || cfg.reserved[0])
			return -EINVAL;

		mutex_lock(&m->lock);
		m->cfg = cfg;
		mutex_unlock(&m->lock);
		return 0;

	default:
		return -ENOTTY;
	}
}
```

What this demonstrates:

- Sysfs exposes a simple read-only status value.
- `poll()` exposes readiness for event-driven userspace.
- ioctl handles structured configuration.
- Every userspace pointer is accessed through user-copy helpers.
- Unsupported ioctl commands return `-ENOTTY`.

For a production driver, you must also implement full registration, remove-while-open lifetime handling, permissions, locking around all shared state, and compatibility handling when structure layout requires it.

## Common Bugs And Debugging
Start debugging from what userspace observes, then map the symptom to the kernel path.

| Symptom | Likely Cause | What To Check | Fix Pattern |
| --- | --- | --- | --- |
| `/dev/mydev` does not exist | `device_create()` failed, class missing, udev/devtmpfs issue | `dmesg`, `/sys/class/<class>/`, `/sys/class/<class>/<dev>/dev` | Check error paths, create class/device after `cdev_add()`, verify permissions |
| `open()` returns `ENODEV` | `cdev` not registered or stale node | `ls -l /dev/mydev`, major/minor, `dmesg` | Recreate node, fix registration order |
| `read()` returns `EFAULT` | Bad user buffer or wrong `copy_to_user()` usage | `strace`, driver logs | Check user-copy return value and buffer sizes |
| `read()` blocks forever | No wakeup, condition never changes, insufficient event source | wait queue condition, IRQ/work logs | Set condition before waking; call `wake_up_interruptible()` |
| `poll()` never wakes | Driver registered wrong wait queue or forgot wakeup | `strace poll`, driver debug logs | Use same condition in `poll()` and `read()`; wake on state change |
| `poll()` returns immediately forever | Driver always returns ready bits | `.poll` mask logic | Return readiness only when operation can proceed |
| `ioctl()` returns `ENOTTY` | Wrong command number, magic, header mismatch, unsupported command | `strace -e ioctl`, shared UAPI header | Use one stable UAPI header; validate `_IOC_TYPE()`/`_IOC_NR()` |
| 32-bit app fails on 64-bit kernel | ioctl struct has pointer/long/time/alignment issue | `file` on binary, compat path, struct layout | Use fixed-width fields; implement `compat_ioctl` if needed |
| Sysfs write accepts bad values | Missing parse/validation | Try invalid input manually | Use strict parsing and return `-EINVAL` |
| Kernel crash in IRQ | IRQ path slept or touched userspace | stack trace, lockdep, splat | Move user-copy and sleeping work to process/thread/workqueue context |

Useful commands:

```bash
ls -l /dev/mydev
cat /sys/class/myclass/mydev/dev
udevadm info /dev/mydev
strace -e openat,read,write,ioctl,poll ./app
dmesg -w
cat /proc/sys/kernel/printk
cat /sys/module/<module>/parameters/<param>
ls /sys/kernel/debug/tracing
```

Debugging habits:

- Log the command number and decoded ioctl fields during bring-up.
- Log state transitions around wait queues and wakeups.
- Test `O_NONBLOCK`.
- Test invalid sysfs input.
- Test short reads/writes.
- Test remove/unload while userspace still has the device open.
- Run both 32-bit and 64-bit userspace tests when ioctl structures are sensitive.

## Production Checklist
Use this list before sending a driver ABI for review.

### ABI Choice
- [ ] Existing subsystem ABI was considered first.
- [ ] `/dev`, sysfs, ioctl, procfs, debugfs, and tracefs roles are not mixed up.
- [ ] Debug-only interfaces are not required by production applications.
- [ ] Deprecated ABIs, such as GPIO sysfs for new users, are avoided.

### Device Nodes
- [ ] Device numbers are dynamically allocated unless there is a strong reason otherwise.
- [ ] Device node naming is stable and meaningful.
- [ ] Device permissions and udev rules are considered.
- [ ] Remove path handles open file descriptors safely.

### Sysfs
- [ ] Attributes are simple and documented.
- [ ] `show()` uses `sysfs_emit()` or equivalent current best practice.
- [ ] `store()` validates input and returns `count` only when the whole input is accepted.
- [ ] Attribute lifetime matches backing object lifetime.
- [ ] Writable attributes have clear permissions and side effects.

### ioctl
- [ ] Command numbers use `_IO`, `_IOR`, `_IOW`, or `_IOWR`.
- [ ] Unsupported commands return `-ENOTTY`.
- [ ] UAPI structures use fixed-width integer types.
- [ ] Structures include reserved fields for future extension.
- [ ] No raw kernel pointers or uninitialized padding are exposed.
- [ ] 32-bit userspace compatibility is reviewed.
- [ ] Every command documents input, output, units, blocking behavior, and errors.

### Concurrency And Lifetime
- [ ] Shared state is protected by mutexes/spinlocks appropriate to context.
- [ ] No sleeping or user-copy happens in hard IRQ context.
- [ ] Interruptible waits check return values.
- [ ] `poll()` and blocking I/O use the same state conditions.
- [ ] Remove/unload paths cannot free state still used by open files.

### Testing
- [ ] `strace` output matches expected syscall behavior.
- [ ] Invalid input tests return expected errno.
- [ ] Non-blocking and blocking modes are tested.
- [ ] `poll()`/`select()`/`epoll()` behavior is tested.
- [ ] Sysfs files are tested for read, write, permissions, and invalid values.
- [ ] ABI behavior is documented for application developers.

## Interview Readiness
You are ready for interviews when you can reason from userspace symptoms back to kernel mechanisms.

Be able to explain:

- Why userspace needs a stable ABI and why kernel internals can change more freely.
- How `/dev/mydev` reaches `struct file_operations`.
- Why `__user` pointers need `copy_to_user()` and `copy_from_user()`.
- How `poll()` differs from blocking `read()`.
- When sysfs is better than ioctl, and when ioctl is better than sysfs.
- Why debugfs should not be production ABI.
- How to design ioctl structures for compatibility.
- Why GPIO sysfs is legacy and `/dev/gpiochipX` is the modern GPIO userspace ABI.

Practice with: `interview/08-userspace-abi-design-for-drivers.md`.

## Kernel Version Notes
Some ABI implementation details are kernel-version-sensitive.

- `class_create()` signatures differ across kernel versions. Check the target kernel headers before copying old examples.
- `.poll` examples in older material may return `unsigned int`; newer kernels commonly use `__poll_t`.
- sysfs output should use `sysfs_emit()` / `sysfs_emit_at()` in modern code.
- Use `_IOWR`, not the older mistaken `_IORW` spelling sometimes seen in notes.
- GPIO sysfs is deprecated for new userspace consumers; prefer the GPIO character-device ABI and libgpiod.
- ioctl `compat_ioctl` rules changed over kernel history. Validate current behavior for any ABI used by 32-bit applications on 64-bit kernels.
