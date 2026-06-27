# 08 - Userspace ABI Design For Drivers Interview Questions

Strong candidates should reason about userspace ABI as a long-term contract. They should know how `/dev`, sysfs, ioctl, `poll()`, framework ABIs, and debug-only filesystems fit together, and they should be able to debug real failures without hiding behind vague theory.

## Beginner Questions

### 1. What is a userspace ABI in a driver?
**Level:** Beginner

**Question:** What do we mean when we say a Linux driver exposes a userspace ABI?

**Short Answer:** A userspace ABI is the stable contract that applications use to interact with the driver, such as `/dev` nodes, sysfs files, ioctl numbers, structure layouts, return values, and blocking behavior.

**Deep Explanation:** Driver internals can be refactored, but userspace-visible behavior must remain compatible once applications depend on it. If an app opens `/dev/mydev`, writes to `/sys/class/.../mode`, or calls `ioctl(fd, MYDEV_GET_STATUS, ...)`, those names, commands, meanings, data formats, and error codes become part of the ABI.

**API / Code Anchor:**
```c
open("/dev/mydev", O_RDWR);
ioctl(fd, MYDEV_IOC_GET_CONFIG, &cfg);
cat /sys/class/myclass/mydev/status
```

**Production or Debugging Angle:** Breaking userspace ABI can break deployed products even if the kernel module still builds. Production drivers should document ABI paths, command meanings, units, permissions, and errno behavior.

**Common Traps:**
- Thinking ABI only means C function prototypes.
- Changing ioctl structure layout without compatibility.
- Treating debugfs files as stable production interfaces.

**Follow-up Questions:**
- What parts of a sysfs file are ABI?
- Why is changing an ioctl command number dangerous?
- What does "do not break userspace" mean in driver work?

### 2. How does opening `/dev/mydev` reach the driver?
**Level:** Beginner

**Question:** Explain how userspace `open("/dev/mydev")` ends up calling the driver's `.open()` callback.

**Short Answer:** `/dev/mydev` is a character device node with a major/minor number. VFS finds the matching `struct cdev`, then uses its `struct file_operations` to call `.open()`.

**Deep Explanation:** The driver reserves a `dev_t`, initializes a `struct cdev` with its `file_operations`, and adds it with `cdev_add()`. It usually calls `class_create()` and `device_create()` so devtmpfs/udev creates `/dev/mydev`. When userspace opens the node, VFS sees the device inode, follows `inode->i_cdev`, and calls the matching file operation.

**API / Code Anchor:**
```c
alloc_chrdev_region(&devt, 0, 1, "mydev");
cdev_init(&mydev->cdev, &mydev_fops);
cdev_add(&mydev->cdev, devt, 1);
device_create(class, NULL, devt, NULL, "mydev");

static const struct file_operations mydev_fops = {
	.owner = THIS_MODULE,
	.open = mydev_open,
};
```

**Production or Debugging Angle:** If `/dev/mydev` is missing, check `dmesg`, `/sys/class/<class>/<name>/dev`, udev/devtmpfs, and whether `device_create()` failed.

**Common Traps:**
- Manually creating `/dev` with the wrong major/minor.
- Calling `device_create()` but forgetting `cdev_add()`.
- Assuming `struct inode` and `struct file` are the same thing.

**Follow-up Questions:**
- What is the difference between major and minor?
- Why does `file->private_data` matter?
- What should happen on driver remove while the file is still open?

### 3. When should a driver use sysfs instead of ioctl?
**Level:** Beginner

**Question:** For a simple device setting like `sample_rate`, should you expose it through sysfs or ioctl?

**Short Answer:** If it is a simple property that can be represented as a small value, sysfs is often better. ioctl is better for structured commands or operations that do not fit simple attributes.

**Deep Explanation:** Sysfs represents attributes of kernel objects. A good sysfs file is small, understandable, and usually one value or a simple list. ioctl is a command interface on a file descriptor and is useful for complex configuration, bidirectional structures, resets, calibration commands, or operations tied to one open device instance.

**API / Code Anchor:**
```c
static ssize_t sample_rate_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%u\n", rate);
}

static DEVICE_ATTR_RW(sample_rate);
```

**Production or Debugging Angle:** Sysfs files are easy to inspect with shell tools, but once published their names and meanings are ABI. Keep them simple and validated.

**Common Traps:**
- Creating a mini command language in a sysfs file.
- Using ioctl for every small scalar property.
- Returning success from `store()` after accepting only part of the input.

**Follow-up Questions:**
- Why should sysfs `show()` use `sysfs_emit()`?
- What should `store()` return on success?
- When would ioctl be more appropriate?

## Mid-Level Questions

### 4. How do you safely copy data between userspace and kernel space?
**Level:** Mid

**Question:** Why can't a driver directly dereference a userspace pointer passed to `read()`, `write()`, or ioctl?

**Short Answer:** Userspace pointers are untrusted and may be invalid, unmapped, or inaccessible. Drivers must use helpers such as `copy_from_user()`, `copy_to_user()`, `get_user()`, and `put_user()`.

**Deep Explanation:** A pointer marked `__user` points into userspace virtual memory, not normal kernel memory. Direct dereference can fault or create security bugs. User-copy helpers handle access checks and page faults safely from valid process context. Their return values must be checked.

**API / Code Anchor:**
```c
if (copy_from_user(&cfg, (void __user *)arg, sizeof(cfg)))
	return -EFAULT;

if (copy_to_user((void __user *)arg, &status, sizeof(status)))
	return -EFAULT;
```

**Production or Debugging Angle:** If userspace sees `EFAULT`, inspect pointer validity, structure size, command direction, and whether the driver copied the right amount. Use `strace -e ioctl,read,write`.

**Common Traps:**
- Dereferencing `arg` directly in ioctl.
- Forgetting that `copy_to_user()` returns bytes not copied, not zero-on-error only.
- Copying to/from userspace in hard IRQ context.

**Follow-up Questions:**
- What errno should a failed user-copy return?
- When are `get_user()` and `put_user()` useful?
- Why is user-copy unsafe in an interrupt handler?

### 5. How should `poll()` work in a character driver?
**Level:** Mid

**Question:** What does a driver's `.poll()` callback do, and how is it related to blocking `read()`?

**Short Answer:** `.poll()` registers wait queues with `poll_wait()` and returns readiness bits showing whether an operation can proceed now. It should use the same readiness condition as blocking `read()` or `write()`.

**Deep Explanation:** Userspace calls `poll()` or `select()` to wait without busy looping. The driver does not transfer data in `.poll()`. It registers interest in wait queues and returns a mask such as `POLLIN | POLLRDNORM` if data is available. When new data arrives, the driver updates the condition and wakes the queue.

**API / Code Anchor:**
```c
static __poll_t mydev_poll(struct file *file, poll_table *wait)
{
	struct mydev *dev = file->private_data;
	__poll_t mask = 0;

	poll_wait(file, &dev->read_wq, wait);

	if (data_available(dev))
		mask |= POLLIN | POLLRDNORM;

	return mask;
}
```

**Production or Debugging Angle:** If `poll()` never returns, check whether the condition changes and whether `wake_up_interruptible()` is called. If it returns constantly, the driver may be reporting readiness unconditionally.

**Common Traps:**
- Thinking `poll_wait()` itself waits until data arrives.
- Using different conditions in `.poll()` and `.read()`.
- Forgetting to wake the wait queue after changing state.

**Follow-up Questions:**
- What should non-blocking `read()` return when no data is ready?
- Why should the condition be set before waking waiters?
- How would `epoll()` observe the same driver behavior?

### 6. How do you design ioctl command numbers and payloads?
**Level:** Mid

**Question:** What rules should you follow when adding ioctl commands to a driver?

**Short Answer:** Use `_IO`, `_IOR`, `_IOW`, or `_IOWR`; validate magic and command number; use stable UAPI structures with fixed-width types; check user-copy; return `-ENOTTY` for unsupported commands; consider `compat_ioctl`.

**Deep Explanation:** ioctl command numbers encode a command type, command number, direction, and size. The direction is from userspace's point of view: `_IOW` means userspace writes data to the kernel, so the driver copies from userspace. Payload structures become ABI, so avoid raw kernel pointers, `long`, layout ambiguity, and uninitialized padding.

**API / Code Anchor:**
```c
#define MY_IOC_MAGIC 'M'
#define MY_IOC_GET_CONFIG _IOR(MY_IOC_MAGIC, 1, struct my_config)
#define MY_IOC_SET_CONFIG _IOW(MY_IOC_MAGIC, 2, struct my_config)

if (_IOC_TYPE(cmd) != MY_IOC_MAGIC)
	return -ENOTTY;
```

**Production or Debugging Angle:** Use `strace` to decode ioctl failures. If a 32-bit app fails on a 64-bit kernel, inspect structure layout and compatibility handling.

**Common Traps:**
- Using `_IORW` instead of `_IOWR`.
- Passing `sizeof(arg)` as the third macro argument.
- Returning `-EINVAL` instead of `-ENOTTY` for unknown ioctl commands.
- Forgetting reserved fields for future extension.

**Follow-up Questions:**
- Why does `_IOW` usually pair with `copy_from_user()`?
- What makes an ioctl struct unsafe for 32-bit compatibility?
- When can `compat_ptr_ioctl()` be enough?

### 7. Debugging scenario: sysfs write appears to work but device behavior is wrong
**Level:** Mid

**Question:** Userspace runs `echo 1000 > /sys/class/mydev/mydev0/rate`; the write succeeds, but the hardware does not change rate. How do you debug it?

**Short Answer:** Check that `store()` parses and validates the input, updates the real device state under lock, returns the correct count only after success, and is connected to the right device instance.

**Deep Explanation:** Sysfs `store()` receives a buffer from userspace and should parse the complete value, validate ranges/units, lock state, apply the change to the backing device, and return `count` on success. If it only updates a cached variable, uses the wrong device pointer, or returns success after a failed hardware write, userspace will believe the change worked.

**API / Code Anchor:**
```c
static ssize_t rate_store(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct mydev *m = dev_get_drvdata(dev);
	unsigned int rate;

	if (kstrtouint(buf, 0, &rate))
		return -EINVAL;
	if (!valid_rate(rate))
		return -EINVAL;

	return apply_rate(m, rate) ? -EIO : count;
}
```

**Production or Debugging Angle:** Verify with `cat` after writing, add `dev_dbg()` around parse/apply paths, test invalid values, and confirm `dev_get_drvdata()` points to the expected instance.

**Common Traps:**
- Returning `count` before hardware programming succeeds.
- Accepting invalid values silently.
- Using global state in a multi-device driver.
- Forgetting locking around shared state.

**Follow-up Questions:**
- Should sysfs attributes expose units in the filename or value?
- How do you make a sysfs attribute pollable?
- Why should sysfs not be used as a complex binary protocol?

## Senior Questions

### 8. How do you choose between a private ABI and a framework ABI?
**Level:** Senior

**Question:** Your hardware can read small non-volatile memory cells and userspace needs to inspect them. Should you create a private char device with ioctls?

**Short Answer:** Usually no. If the hardware fits NVMEM, register it with the NVMEM framework and use the framework's provider/consumer and userspace ABI.

**Deep Explanation:** Frameworks exist to avoid every driver inventing a slightly different ABI for the same class of hardware. NVMEM can expose registered storage through standard sysfs binary files and can also provide cells to kernel consumers. A private ioctl ABI increases maintenance, documentation, compatibility, and tooling burden.

**API / Code Anchor:**
```c
struct nvmem_config cfg = {
	.dev = &client->dev,
	.name = "my-eeprom",
	.size = size,
	.word_size = 1,
	.stride = 1,
	.reg_read = my_nvmem_read,
	.reg_write = my_nvmem_write,
};

devm_nvmem_register(&client->dev, &cfg);
```

**Production or Debugging Angle:** Framework ABIs usually come with standard userspace tools, kernel documentation, tests, and review expectations. They also make the driver easier for other kernel code to consume.

**Common Traps:**
- Inventing a private ABI because it seems faster during bring-up.
- Ignoring read-only/root-only policy.
- Exposing old ABI paths only because old sample code did.
- Assuming framework ABI removes the need for validation.

**Follow-up Questions:**
- Give examples of other framework ABIs.
- Why is GPIO sysfs not recommended for new userspace?
- When might a private char device still be justified?

### 9. How do you handle lifetime and remove-while-open?
**Level:** Senior

**Question:** A userspace process opens `/dev/mydev`, then the hardware is unplugged or the driver is removed. What must the ABI and driver handle?

**Short Answer:** The driver must prevent use-after-free, mark the device unavailable, wake blocked users, return sensible errors, and free state only after all open references are gone.

**Deep Explanation:** Destroying the `/dev` node stops new opens but does not automatically make existing `struct file` objects disappear. Existing callbacks may still run through `file->private_data`. The driver needs lifetime management: reference counts, locks, a removed/dead flag, orderly sysfs removal, wait queue wakeups, and cleanup after last close.

**API / Code Anchor:**
```c
mutex_lock(&dev->lock);
dev->removed = true;
mutex_unlock(&dev->lock);

wake_up_interruptible(&dev->read_wq);
device_destroy(class, devt);
cdev_del(&dev->cdev);
```

**Production or Debugging Angle:** Test unload/remove while a process is blocked in `read()` or `poll()`. Watch for use-after-free, stuck tasks, and callbacks accessing freed hardware mappings.

**Common Traps:**
- Freeing device state immediately in remove while files are still open.
- Forgetting to wake blocked readers/writers.
- Leaving sysfs attributes active after backing state is invalid.
- Returning success after hardware is gone.

**Follow-up Questions:**
- Where would you place reference increments/decrements?
- What should blocked `read()` return after remove?
- How does sysfs object lifetime differ from physical device availability?

### 10. Debugging scenario: killing userspace crashes the kernel
**Level:** Senior

**Question:** A thread is blocked in a driver ioctl. When the process receives a signal or is killed, the kernel later crashes. What bug pattern do you suspect?

**Short Answer:** The driver likely used `wait_event_interruptible()` but failed to check its return value or re-check the condition before accessing data.

**Deep Explanation:** Interruptible waits can return because a signal arrived, not because the desired condition became true. If the code assumes data is ready after the wait and dereferences a pointer or processes uninitialized state, it can crash. The condition must be rechecked and the interrupted case must return an error or clean up safely.

**API / Code Anchor:**
```c
ret = wait_event_interruptible(dev->wq, dev->data_ready || dev->removed);
if (ret)
	return -ERESTARTSYS;

if (dev->removed)
	return -ENODEV;
if (!dev->data_ready)
	return -EAGAIN;
```

**Production or Debugging Angle:** Reproduce by sending signals to blocked userspace, enable driver logs around waits, and inspect stack traces for invalid state after the wait returns.

**Common Traps:**
- Assuming wakeup means the condition is true.
- Ignoring `wait_event_interruptible()` return.
- Accessing user memory or sleeping from an IRQ handler instead of waking a process-context path.
- Not handling device removal as another wake condition.

**Follow-up Questions:**
- When would you use an uninterruptible wait?
- What is the relationship between `poll()` wakeups and blocking waits?
- Which errno should userspace see after a signal?

### 11. How do you design an ioctl ABI for future extension?
**Level:** Senior

**Question:** You need an ioctl that passes configuration from userspace to the driver. What do you put in the structure so it can evolve safely?

**Short Answer:** Use fixed-width fields, explicit sizes or versions when useful, reserved fields initialized to zero, stable units, no kernel pointers, no ambiguous `long`/time fields, and compatibility handling when layout can differ.

**Deep Explanation:** ioctl payload structures become binary layout ABI. Future kernels may need new fields while old applications still pass old structures. Reserved zero fields let the kernel detect unsupported future flags and add compatible behavior later. Fixed-width types avoid architecture-dependent size changes.

**API / Code Anchor:**
```c
struct mydev_config {
	__u32 size;
	__u32 mode;
	__u32 flags;
	__u32 timeout_ms;
	__u32 reserved[8];
};

#define MYDEV_SET_CONFIG _IOW(MY_IOC_MAGIC, 2, struct mydev_config)
```

**Production or Debugging Angle:** Add tests that pass unknown flags, nonzero reserved fields, invalid sizes, and boundary values. Confirm the kernel returns predictable errors instead of silently accepting undefined behavior.

**Common Traps:**
- Using `long`, pointer-sized fields, or native `time_t`.
- Leaking uninitialized padding back to userspace.
- Reusing command numbers with different meanings.
- Failing to document units and valid ranges.

**Follow-up Questions:**
- How would you handle an application compiled against an older header?
- What makes `compat_ioctl` necessary?
- Why should unsupported flags usually return `-EINVAL` or `-EOPNOTSUPP`?

### 12. Compare sysfs, debugfs, and procfs for driver interfaces
**Level:** Senior

**Question:** A teammate wants to expose runtime device controls in debugfs because it is easy. What do you say?

**Short Answer:** debugfs is fine for debugging, but it should not be required production ABI. Use sysfs, a subsystem ABI, or ioctl for stable device control depending on the shape of the interface.

**Deep Explanation:** debugfs is optional, root-oriented, kernel-config dependent, and not a stable userspace contract. procfs is mainly for process and system-level information or legacy controls. sysfs is device-model ABI for small attributes. For complex commands, use ioctl or an existing subsystem.

**API / Code Anchor:**
```text
/sys/class/...              stable small device attributes
/proc/sys/kernel/printk     system control example
/sys/kernel/debug/tracing   tracing/debug interface
/sys/kernel/debug/...       debug-only driver knobs
```

**Production or Debugging Angle:** During bring-up, debugfs can expose internal counters and state. Before release, decide which controls are required by applications and move them to a stable documented ABI.

**Common Traps:**
- Depending on debugfs in production scripts.
- Exposing secrets or unsafe hardware controls without permission checks.
- Using procfs for new per-device interfaces.
- Treating sysfs as a dumping ground for debug internals.

**Follow-up Questions:**
- What happens if debugfs is not mounted?
- Why are tracefs/ftrace useful during ABI debugging?
- How do module parameters under `/sys/module` differ from per-device sysfs attributes?

## Quick Review Traps
Use these as fast self-checks before an interview.

- `_IOW` means userspace writes; the driver normally copies from userspace.
- Unknown ioctl command: return `-ENOTTY`.
- `copy_to_user()` / `copy_from_user()` return the number of bytes not copied.
- `poll_wait()` registers interest; it is not the readiness condition.
- Sysfs should expose simple attributes, not private command protocols.
- debugfs is not stable production ABI.
- GPIO sysfs is legacy/deprecated for new userspace consumers.
- Existing open file descriptors can outlive `/dev` node removal.
- Interruptible waits must check return values and re-check state.
