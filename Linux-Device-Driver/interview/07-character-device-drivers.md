# 07 - Character Device Drivers Interview Questions

Character drivers are a practical Embedded Linux interview topic because they connect `/dev` nodes, device numbers, VFS callbacks, userspace ABI, user-copy safety, synchronization, and cleanup discipline. A strong answer should explain the path from userspace to driver state, then show how the design behaves under errors, concurrency, and removal.

Use these questions to test understanding, not keyword recall. A good candidate can trace one syscall from userspace to `struct file_operations`, explain what state is owned by `inode`, `file`, `cdev`, and the device object, and predict what happens when allocation, user-copy, blocking I/O, or teardown fails.

## Beginner

### 1. From `/dev` Node To Driver Callback

- **Level:** Beginner
- **Question:** When userspace opens `/dev/demochar0` and later calls `read(fd, buf, len)`, what path reaches the driver?
- **Short Answer:** `open()` resolves the `/dev` inode's `dev_t` to a registered `struct cdev` and its `struct file_operations`. Later `read()` dispatches through the already-open `struct file`, using `file->f_op` and the driver state stored in `file->private_data`.
- **Deep Explanation:** Userspace sees a special file, but the kernel sees a character-device inode. During `open()`, the character-device core uses the inode's major/minor number to find the live `struct cdev`; that `cdev` points to the driver's `file_operations`, so VFS can call `.open()`. A well-structured `.open()` finds the per-device object and stores it in `file->private_data`. After that, `read()`, `write()`, `poll()`, and `ioctl()` do not redo the `/dev` lookup; they dispatch through the open `struct file` and use `file->private_data`.
- **API / Code Anchor:**
  ```c
  static int demo_open(struct inode *inode, struct file *filp)
  {
          struct demo_dev *dev;

          dev = container_of(inode->i_cdev, struct demo_dev, cdev);
          filp->private_data = dev;
          return 0;
  }

  /* Later operations use the already-open struct file. */
  static ssize_t demo_read(struct file *filp, char __user *buf,
                           size_t count, loff_t *ppos)
  {
          struct demo_dev *dev = filp->private_data;
          ...
  }
  ```
- **Production or Debugging Angle:** If callbacks do not run, check that the `/dev` node is a character device, its major/minor match `/sys/class/<class>/<device>/dev`, and `cdev_add()` succeeded. `strace -e openat,read,write` plus `dmesg` callback logs usually shows whether userspace reaches the expected driver.
- **Common Traps:**
  - Thinking `/dev/demochar0` itself stores driver state.
  - Forgetting that `/dev` -> `dev_t` -> `struct cdev` is mainly the open-time lookup.
  - Missing that later syscalls use the already-open `struct file`.
  - Using one global object accidentally when minors should select different devices.
  - Assuming `device_create()` alone registers callbacks.
- **Follow-up Questions:**
  - What object connects a character-device inode to your driver?
  - Why is `file->private_data` normally initialized in `.open()`?
  - How would you prove that `/dev/demochar0` points at the expected major/minor?

### 2. Device Numbers And Minor Routing

- **Level:** Beginner
- **Question:** What are `dev_t`, major number, and minor number, and how do they matter in a multi-device character driver?
- **Short Answer:** `dev_t` stores the device identity. The major number selects the driver or registered range, while the minor number usually selects the instance handled by that driver.
- **Deep Explanation:** Character device files show a major/minor pair in `ls -l /dev`. The kernel stores that pair in `dev_t`. A driver may allocate one major with many consecutive minors, such as `/dev/eep-mem0` through `/dev/eep-mem7`. The minor can index an array, xarray, IDR, or embedded object. The key mental model is that the minor is not decorative; it is often the route from one driver registration to one hardware channel, buffer, bank, or logical endpoint.
- **API / Code Anchor:**
  ```c
  dev_t base;

  ret = alloc_chrdev_region(&base, 0, nr_devs, "demochar");
  if (ret)
          return ret;

  pr_info("major=%u first_minor=%u\n", MAJOR(base), MINOR(base));

  dev_t devt = MKDEV(MAJOR(base), MINOR(base) + index);
  ```
- **Production or Debugging Angle:** Wrong-minor bugs often look like "open succeeds but the wrong hardware responds." Log `iminor(inode)`, the selected device pointer, and the major/minor at registration. Compare them with `ls -l /dev/<node>` and `cat /sys/class/<class>/<device>/dev`.
- **Common Traps:**
  - Hard-coding a static major number without a real deployment reason.
  - Registering fewer minors than the driver later exposes.
  - Creating a manual `/dev` node with the wrong major/minor.
  - Treating the minor as only a printed number instead of an instance selector.
- **Follow-up Questions:**
  - Why is dynamic major allocation preferred for most drivers?
  - When might `register_chrdev_region()` still be justified?
  - How would you map minor `N` back to `struct demo_dev *`?

### 3. `struct cdev` And `struct file_operations`

- **Level:** Beginner
- **Question:** What are the roles of `struct cdev` and `struct file_operations`?
- **Short Answer:** `struct cdev` is the registered character-device object for a device-number range. `struct file_operations` is the callback table the VFS uses for file operations on that character device.
- **Deep Explanation:** Reserving a `dev_t` only claims identity. It does not make your driver callable. The driver initializes a `struct cdev`, attaches `file_operations`, and calls `cdev_add()` to publish callbacks to the VFS. A common multi-device design embeds `struct cdev` inside stable per-device state, so `inode->i_cdev` can be converted back to the owning object in `.open()`.
- **API / Code Anchor:**
  ```c
  static const struct file_operations demo_fops = {
          .owner          = THIS_MODULE,
          .open           = demo_open,
          .release        = demo_release,
          .read           = demo_read,
          .write          = demo_write,
          .llseek         = demo_llseek,
          .poll           = demo_poll,
          .unlocked_ioctl = demo_ioctl,
  };

  cdev_init(&dev->cdev, &demo_fops);
  ret = cdev_add(&dev->cdev, dev->devt, 1);
  ```
- **Production or Debugging Angle:** Treat `cdev_add()` as a live boundary. After it succeeds, userspace may enter callbacks if a matching node exists or appears quickly. Initialize locks, buffers, wait queues, and state before this point.
- **Common Traps:**
  - Believing `cdev_init()` registers the device.
  - Calling `cdev_add()` before driver state is ready.
  - Forgetting `.owner = THIS_MODULE` in module-based drivers.
  - Freeing the object that embeds `struct cdev` while callbacks may still use it.
- **Follow-up Questions:**
  - What can happen immediately after `cdev_add()` succeeds?
  - Why embed `struct cdev` instead of keeping it unrelated to device state?
  - What is the difference between reserving a number and registering callbacks?

### 4. `inode`, `file`, And `.release()`

- **Level:** Beginner
- **Question:** What is the difference between `struct inode` and `struct file`, and when does `.release()` run?
- **Short Answer:** `inode` represents the device file identity; `file` represents one open file instance. `.release()` runs when the final reference to that open `struct file` is dropped, not necessarily on every userspace `close()` call.
- **Deep Explanation:** The same inode may be opened many times. Each successful open creates or references a `struct file` that carries state such as `f_pos`, `f_flags`, `f_op`, and `private_data`. File descriptors can be duplicated by `dup()`, inherited by `fork()`, or passed to another process, so multiple descriptors may share one `struct file`. That is why `.release()` is the cleanup point for per-open state, but it is not a simple "one close syscall equals one release callback" rule.
- **API / Code Anchor:**
  ```c
  static int demo_open(struct inode *inode, struct file *filp)
  {
          struct demo_open_ctx *ctx;

          ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
          if (!ctx)
                  return -ENOMEM;

          ctx->dev = container_of(inode->i_cdev, struct demo_dev, cdev);
          filp->private_data = ctx;
          return 0;
  }

  static int demo_release(struct inode *inode, struct file *filp)
  {
          kfree(filp->private_data);
          return 0;
  }
  ```
- **Production or Debugging Angle:** If memory or hardware sessions leak, inspect duplicated descriptors and error paths after partial `.open()` setup. Log open context allocation and release with pointer values when chasing lifetime bugs.
- **Common Traps:**
  - Storing stack memory in `file->private_data`.
  - Freeing per-device state in `.release()` when only per-open state should be freed.
  - Assuming `.release()` runs on every `close()`.
  - Ignoring `file->f_flags`, especially `O_NONBLOCK`.
- **Follow-up Questions:**
  - What state belongs in a per-device object versus a per-open object?
  - How do `dup()` and `fork()` affect `.release()` timing?
  - Why is `inode->i_cdev` usually most useful in `.open()`?

## Mid-Level

### 5. Registration, `/dev` Node Creation, And Failure Unwind

- **Level:** Mid
- **Question:** Describe a safe initialization and cleanup sequence for a character driver that creates `/dev/demochar0`.
- **Short Answer:** Allocate `dev_t`, initialize state, initialize and add `cdev`, create a class, create the device node publication, then unwind in reverse order on failure or unload.
- **Deep Explanation:** `alloc_chrdev_region()` reserves identity. State initialization prepares locks, buffers, wait queues, and device structures. `cdev_init()` binds callbacks, and `cdev_add()` makes them live. `class_create()` creates a sysfs class, while `device_create()` publishes a device under that class so devtmpfs or udev can create `/dev/demochar0`. Cleanup should mirror acquisition: `device_destroy()`, `class_destroy()`, `cdev_del()`, then `unregister_chrdev_region()`.
- **API / Code Anchor:**
  ```c
  ret = alloc_chrdev_region(&devt, 0, 1, "demochar");
  if (ret)
          return ret;

  mutex_init(&dev->lock);
  init_waitqueue_head(&dev->readq);

  cdev_init(&dev->cdev, &demo_fops);
  ret = cdev_add(&dev->cdev, devt, 1);
  if (ret)
          goto err_unreg;

  cls = class_create("demochar");
  if (IS_ERR(cls)) {
          ret = PTR_ERR(cls);
          goto err_cdev;
  }

  device = device_create(cls, NULL, devt, NULL, "demochar0");
  if (IS_ERR(device)) {
          ret = PTR_ERR(device);
          goto err_class;
  }

  return 0;

  err_class:
          class_destroy(cls);
  err_cdev:
          cdev_del(&dev->cdev);
  err_unreg:
          unregister_chrdev_region(devt, 1);
          return ret;
  ```
- **Production or Debugging Angle:** For a missing `/dev` node, check `dmesg`, `/proc/devices`, `/sys/class/<class>/`, `udevadm monitor`, and the return values from `class_create()` and `device_create()`. Kernel examples differ by version: modern kernels commonly use `class_create("name")`, while older code may show `class_create(THIS_MODULE, "name")`.
- **Common Traps:**
  - Forgetting `IS_ERR()` / `PTR_ERR()` for class or device creation.
  - Destroying resources that were never successfully created.
  - Missing `unregister_chrdev_region()` in an error label.
  - Creating the `/dev` publication before the driver state can handle opens.
- **Follow-up Questions:**
  - If `device_create()` fails, which cleanup labels should run?
  - Why should failure cleanup be reverse acquisition order?
  - What is the difference between `cdev_add()` and `device_create()`?

### 6. Read, Write, File Position, And Short I/O

- **Level:** Mid
- **Question:** How should `.read()`, `.write()`, and `.llseek()` handle file position, buffer limits, and partial transfers?
- **Short Answer:** They should validate the position, clamp the requested count to available data or space, copy safely to or from userspace, update `*ppos`, and return the actual bytes transferred. Short I/O is a normal successful result.
- **Deep Explanation:** A finite RAM buffer behaves differently from a stream. For a seekable buffer, `*ppos` matters and reading past the end should return `0` for EOF. For a stream or FIFO-like device, no data may mean sleep or `-EAGAIN`, not EOF. Writes may accept fewer bytes than requested because of buffer capacity or hardware backpressure. `.llseek()` should define meaningful seeking, use helpers such as `fixed_size_llseek()`, or reject seeking with `no_llseek` for streams.
- **API / Code Anchor:**
  ```c
  mutex_lock(&dev->lock);

  if (*ppos < 0)
          ret = -EINVAL;
  else if (*ppos >= dev->len)
          ret = 0;
  else {
          n = min(count, dev->len - (size_t)*ppos);

          /*
           * Holding a mutex across user-copy is allowed, but not always
           * ideal. If the data is protected by a spinlock, copy through a
           * temporary kernel buffer instead.
           */
          not_copied = copy_to_user(ubuf, dev->buf + *ppos, n);
          done = n - not_copied;
          *ppos += done;

          if (not_copied && !done)
                  ret = -EFAULT;
          else
                  ret = done;
  }

  mutex_unlock(&dev->lock);
  return ret;
  ```
- **Production or Debugging Angle:** Repeated reads returning the same bytes usually mean `*ppos` was not advanced. Use `strace`, `dd`, `cat`, and callback logs for `count`, `*ppos`, returned bytes, and errno. User programs must handle short reads and writes too.
- **Common Traps:**
  - Returning the requested count when fewer bytes were copied.
  - Forgetting to update `*ppos`.
  - Treating all short transfers as errors.
  - Returning `0` for a temporarily empty blocking stream, causing userspace to think EOF.
  - Making a hardware stream seekable when positions have no real meaning.
- **Follow-up Questions:**
  - When should `.read()` return bytes, `0`, `-EAGAIN`, or sleep?
  - What should `.write()` return if only half the data fits?
  - When would you use `fixed_size_llseek()` versus `no_llseek`?

### 7. User Pointers And Copy Helpers

- **Level:** Mid
- **Question:** Why must a driver use `copy_to_user()`, `copy_from_user()`, `get_user()`, or `put_user()` instead of directly dereferencing userspace pointers?
- **Short Answer:** `__user` pointers are untrusted userspace addresses. The user-copy helpers perform safe access across the kernel/userspace boundary and handle faults.
- **Deep Explanation:** A pointer passed to `.read()`, `.write()`, or `.ioctl()` belongs to userspace. Direct dereference can fault in kernel context or create a security bug. `copy_from_user()` brings data into kernel memory before validation or use. `copy_to_user()` returns data to userspace. For single scalar values, `get_user()` and `put_user()` can be clearer. The important detail is return value semantics: `copy_*_user()` returns the number of bytes not copied, not a negative errno.
- **API / Code Anchor:**
  ```c
  struct demo_cfg cfg;

  if (copy_from_user(&cfg, ucfg, sizeof(cfg)))
          return -EFAULT;

  if (cfg.mode > DEMO_MODE_MAX)
          return -EINVAL;

  if (put_user(dev->status, ustatus))
          return -EFAULT;
  ```
- **Production or Debugging Angle:** User-copy can fault and sleep. Do not hold spinlocks, disable interrupts, or run from atomic context while copying. A common production pattern is copy into a temporary kernel buffer, validate it, then take the device lock for the shortest update.
- **Common Traps:**
  - Treating the return value as bytes copied or as `-errno`.
  - Holding a spinlock across `copy_to_user()` or `copy_from_user()`.
  - Treating `access_ok()` as a replacement for the actual copy helper.
  - Trusting userspace lengths, offsets, enum values, or embedded pointers.
  - Reusing partially copied kernel data without sanitizing it.
  - Mixing up copy direction.
- **Follow-up Questions:**
  - What should the driver return if no bytes could be copied?
  - Why may a mutex be acceptable where a spinlock is not?
  - How would you handle a partially successful copy in a read path?

### 8. Blocking I/O And Nonblocking Mode

- **Level:** Mid
- **Question:** How should a character driver behave when `.read()` has no data or `.write()` has no space?
- **Short Answer:** In blocking mode it should sleep on a wait queue until the condition changes, an error happens, or a signal interrupts the wait. In `O_NONBLOCK` mode it should return `-EAGAIN`.
- **Deep Explanation:** Blocking behavior is part of the userspace ABI. A stream-like device with no data should normally wait if blocking, but return `-EAGAIN` if opened with `O_NONBLOCK`. The driver must test the condition under appropriate locking, sleep with a wait helper that rechecks the condition, and wake the queue when data or space becomes available. Interruptible waits should return `-ERESTARTSYS` when interrupted by a signal.
- **API / Code Anchor:**
  ```c
  /*
   * dev->lock protects data_avail, dead, and the ring state.
   * wait_event_interruptible_lock_irq() sleeps with the lock dropped
   * and returns with the lock held before the condition is consumed.
   */
  spin_lock_irq(&dev->lock);
  if (!demo_read_ready_locked(dev)) {
          if (filp->f_flags & O_NONBLOCK) {
                  spin_unlock_irq(&dev->lock);
                  return -EAGAIN;
          }

          ret = wait_event_interruptible_lock_irq(dev->readq,
                                         demo_read_ready_locked(dev),
                                         dev->lock);
          if (ret) {
                  spin_unlock_irq(&dev->lock);
                  return -ERESTARTSYS;
          }
  }

  if (dev->dead)
          ret = -EIO;
  else
          ret = ring_take_locked(dev, tmp, sizeof(tmp));
  spin_unlock_irq(&dev->lock);
  ```
- **Production or Debugging Angle:** Hangs usually come from missing wakeups, a condition that never becomes true, or sleeping while holding the wrong lock. Debug with state logs before sleeping, logs on wakeup paths, `strace -e read,write,poll`, and signal-interruption tests.
- **Common Traps:**
  - Sleeping in nonblocking mode.
  - Checking a condition once and not rechecking after wakeup.
  - Forgetting to call `wake_up_interruptible()` when state changes.
  - Holding a spinlock or mutex across a wait in a way that prevents the producer from making progress.
  - Returning `0` for "try again later" on a stream.
- **Follow-up Questions:**
  - Why is the wait condition checked again after wakeup?
  - How would you test signal interruption of a blocking read?
  - What is the difference between no data and EOF?

### 9. `poll()` And Event Loop Readiness

- **Level:** Mid
- **Question:** What should a `.poll()` callback do for `poll(2)` or `select(2)` support?
- **Short Answer:** It should register relevant wait queues with `poll_wait()`, check current readiness state, and return readiness bits such as `POLLIN`, `POLLRDNORM`, `POLLOUT`, or `POLLWRNORM`.
- **Deep Explanation:** `poll_wait()` does not sleep in the driver callback and does not declare readiness. It links the caller's poll table to a wait queue so the task can sleep later if nothing is ready. The driver still must inspect current state and return a mask. When data arrives or space becomes available, the same wait queues must be woken. Modern kernels commonly use `__poll_t` as the callback return type; older examples may use `unsigned int`.
- **API / Code Anchor:**
  ```c
  static __poll_t demo_poll(struct file *filp, poll_table *wait)
  {
          struct demo_dev *dev = filp->private_data;
          unsigned long flags;
          __poll_t mask = 0;

          poll_wait(filp, &dev->readq, wait);
          poll_wait(filp, &dev->writeq, wait);

          spin_lock_irqsave(&dev->lock, flags);
          if (dev->data_avail)
                  mask |= POLLIN | POLLRDNORM;
          if (dev->space_avail)
                  mask |= POLLOUT | POLLWRNORM;
          if (dev->dead)
                  mask |= POLLERR | POLLHUP;
          spin_unlock_irqrestore(&dev->lock, flags);

          return mask;
  }
  ```
- **Production or Debugging Angle:** If `poll()` never wakes, confirm that the event path updates state and wakes the queue registered in `.poll()`. If `poll()` spins, the readiness mask may say readable or writable when the following `read()` or `write()` cannot make progress.
- **Common Traps:**
  - Assuming `poll_wait()` means "ready".
  - Forgetting to wake queues after state changes.
  - Returning readiness from stale or unlocked state.
  - Copying an old `.poll` signature without checking the target kernel.
  - Returning `POLLIN` for an empty stream just because a wait queue exists.
- **Follow-up Questions:**
  - What is the difference between registering interest and reporting readiness?
  - Which code path should call `wake_up_interruptible()`?
  - How would you debug `poll()` returning immediately in a tight loop?

### 10. Ioctl Dispatch Mechanics

- **Level:** Mid
- **Question:** In a character driver, what should `.unlocked_ioctl()` do when userspace sends a command with a pointer argument?
- **Short Answer:** It should validate the ioctl family and command, check the expected argument size and direction, copy any input data from userspace, perform the operation under the right lock, copy output data back when required, and return `-ENOTTY` for unsupported commands.
- **Deep Explanation:** `ioctl()` is a syscall escape hatch for operations that do not fit cleanly into byte-stream `read()` and `write()`. The command value encodes a magic/type, command number, direction, and size. The driver should treat that as ABI, not as a random integer. For `_IOW` or `_IOWR`, copy the user structure into a kernel structure first, validate fields such as modes, sizes, offsets, and reserved bits, then update device state. For `_IOR` or `_IOWR`, copy a stable result back to userspace. If the command belongs to another driver or is not supported, `-ENOTTY` is the conventional answer.
- **API / Code Anchor:**
  ```c
  static long demo_ioctl(struct file *filp, unsigned int cmd,
                         unsigned long arg)
  {
          struct demo_dev *dev = filp->private_data;
          void __user *uarg = (void __user *)arg;
          struct demo_cfg cfg;

          if (_IOC_TYPE(cmd) != DEMO_IOC_MAGIC)
                  return -ENOTTY;
          if (_IOC_NR(cmd) > DEMO_IOC_MAXNR)
                  return -ENOTTY;

          switch (cmd) {
          case DEMO_IOC_SET_CFG:
                  if (_IOC_DIR(cmd) != _IOC_WRITE)
                          return -EINVAL;
                  if (_IOC_SIZE(cmd) != sizeof(cfg))
                          return -EINVAL;
                  if (copy_from_user(&cfg, uarg, sizeof(cfg)))
                          return -EFAULT;
                  if (cfg.mode > DEMO_MODE_MAX)
                          return -EINVAL;

                  mutex_lock(&dev->lock);
                  dev->cfg = cfg;
                  mutex_unlock(&dev->lock);
                  return 0;

          default:
                  return -ENOTTY;
          }
  }
  ```
- **Production or Debugging Angle:** Debug ioctl failures with `strace -e ioctl` to see the command number and errno, then add driver logs for `_IOC_TYPE(cmd)`, `_IOC_NR(cmd)`, `_IOC_DIR(cmd)`, and `_IOC_SIZE(cmd)`. In production, keep ioctl numbers and payload structures in a shared UAPI-style header so userspace and kernel agree exactly.
- **Common Traps:**
  - Trusting `arg` as a kernel pointer.
  - Forgetting that `_IOR` and `_IOW` names are from the userspace point of view.
  - Returning `-EINVAL` for commands that this driver does not implement.
  - Updating device state before validating all fields from userspace.
  - Defining ioctl structs with native `long` or raw pointers without compatibility planning.
- **Follow-up Questions:**
  - Why is `-ENOTTY` normally better than `-EINVAL` for an unknown ioctl?
  - What does `_IOC_SIZE(cmd)` protect against, and what does it not protect against?
  - When would `.compat_ioctl` be required?

## Senior

### 11. IOCTL ABI Design

- **Level:** Senior
- **Question:** How do you design `ioctl` commands so they are maintainable userspace ABI instead of temporary hacks?
- **Short Answer:** Put commands in a shared UAPI header, use `_IO`, `_IOR`, `_IOW`, or `_IOWR`, validate command type and arguments, use fixed-width types, return `-ENOTTY` for unsupported commands, and consider `.compat_ioctl` for 32-bit userspace on 64-bit kernels.
- **Deep Explanation:** `ioctl` is useful for control operations that do not fit naturally into byte-stream read/write semantics. But once shipped, command numbers and structure layouts are ABI. Use a unique magic, stable command numbers, explicit fixed-width fields such as `__u32`, reserved fields for extension, and clear direction macros. `_IOR` means kernel writes data to userspace, `_IOW` means userspace writes data to kernel, and `_IOWR` means bidirectional. The macro is `_IOWR`, not `_IORW`. Unsupported commands conventionally return `-ENOTTY`.
- **API / Code Anchor:**
  ```c
  #define DEMO_IOC_MAGIC      'd'
  #define DEMO_IOC_RESET      _IO(DEMO_IOC_MAGIC, 0)
  #define DEMO_IOC_GET_STATUS _IOR(DEMO_IOC_MAGIC, 1, struct demo_status)
  #define DEMO_IOC_SET_CFG    _IOW(DEMO_IOC_MAGIC, 2, struct demo_cfg)
  #define DEMO_IOC_XFER       _IOWR(DEMO_IOC_MAGIC, 3, struct demo_xfer)

  static long demo_ioctl(struct file *filp, unsigned int cmd,
                         unsigned long arg)
  {
          void __user *uarg = (void __user *)arg;

          if (_IOC_TYPE(cmd) != DEMO_IOC_MAGIC)
                  return -ENOTTY;

          switch (cmd) {
          case DEMO_IOC_RESET:
                  return demo_reset(filp->private_data);
          default:
                  return -ENOTTY;
          }
  }
  ```
- **Production or Debugging Angle:** Use `strace -e ioctl`, ABI tests, and a single shared header for kernel and userspace builds. Avoid raw pointers, native `long`, and layout-sensitive structs unless compatibility handling is designed. Permissions, ownership, udev rules, and error codes are part of the ABI users rely on.
- **Common Traps:**
  - Using `_IORW` instead of `_IOWR`.
  - Returning `-EINVAL` for unknown ioctl commands instead of `-ENOTTY`.
  - Changing a shipped structure layout without versioning.
  - Forgetting `.compat_ioctl` when 32-bit applications must run on a 64-bit kernel.
  - Trusting userspace structures before range and reserved-field validation.
- **Follow-up Questions:**
  - When is ioctl better than sysfs, configfs, netlink, or read/write?
  - What makes an ioctl structure 32-bit/64-bit compatible?
  - How would you evolve an ioctl ABI without breaking old applications?

### 12. Lifetime, Removal, And Open File Descriptors

- **Level:** Senior
- **Question:** What lifetime problems appear when a character device is removed while userspace still has it open?
- **Short Answer:** Existing `struct file` objects may still call driver methods after the `/dev` node is removed. The driver must prevent new opens, keep state alive for old opens, wake sleepers, and free memory only after the final file reference and the removal path both drop their references.
- **Deep Explanation:** With `.owner = THIS_MODULE`, normal `rmmod` should fail while file descriptors are open because the module is referenced through `file_operations`. The harder production case is device removal or unbind while the module remains loaded. `device_destroy()` removes userspace publication and `cdev_del()` prevents new character-device lookups, but neither invalidates file descriptors that already hold a `struct file`. Those callbacks may still use `file->private_data`. A production driver needs reference-counted device objects, a `dead` flag, locks around teardown, and wakeups so blocked operations return.
- **API / Code Anchor:**
  ```c
  struct demo_dev {
          struct cdev cdev;
          struct mutex lock;
          wait_queue_head_t readq;
          wait_queue_head_t writeq;
          struct kref kref;
          bool dead;
  };

  /* init/probe owns the base reference. */
  kref_init(&dev->kref);

  static int demo_open(struct inode *inode, struct file *filp)
  {
          struct demo_dev *dev = container_of(inode->i_cdev,
                                              struct demo_dev, cdev);

          mutex_lock(&dev->lock);
          if (dev->dead) {
                  mutex_unlock(&dev->lock);
                  return -ENODEV;
          }
          kref_get(&dev->kref);
          filp->private_data = dev;
          mutex_unlock(&dev->lock);
          return 0;
  }

  static int demo_release(struct inode *inode, struct file *filp)
  {
          struct demo_dev *dev = filp->private_data;

          kref_put(&dev->kref, demo_dev_release);
          return 0;
  }

  /* remove/unload path */
  mutex_lock(&dev->lock);
  dev->dead = true;
  mutex_unlock(&dev->lock);

  device_destroy(dev->class, dev->devt);
  cdev_del(&dev->cdev);
  wake_up_interruptible(&dev->readq);
  wake_up_interruptible(&dev->writeq);
  kref_put(&dev->kref, demo_dev_release); /* drop init/probe reference */
  ```
- **Production or Debugging Angle:** Test repeated load/unload while a process keeps the device open, blocks in read, polls, duplicates the fd, and exits later. Use KASAN, lockdep, refcount debugging, and callback logs to catch use-after-free and missed wakeups.
- **Common Traps:**
  - Freeing the per-device object immediately after `cdev_del()` while old fds still use it.
  - Forgetting to wake blocking readers/writers during removal.
  - Allowing new opens after teardown starts.
  - Treating `.release()` as module unload cleanup.
  - Ignoring duplicated file descriptors that delay final release.
- **Follow-up Questions:**
  - What should blocking `read()` return after device removal?
  - How would you combine an open count with a `dead` flag?
  - What does `cdev_del()` stop, and what does it not stop?

### 13. Locking, Races, And Lost Wakeups

- **Level:** Senior
- **Question:** How would you review a character driver's locking around shared buffers, blocking reads, writes, and `poll()`?
- **Short Answer:** Identify every shared state variable, protect condition checks and updates consistently, avoid sleeping or user-copy under spinlocks, recheck wait conditions, and make state changes wake the right queues.
- **Deep Explanation:** Character drivers often share ring-buffer counters, hardware state, open counts, flags, and statistics across process context, workqueues, and interrupt paths. A mutex can protect sleepable process-context state. A spinlock may be required for IRQ-shared ring-buffer indices, but user-copy and waits must happen outside it. Blocking paths should avoid lost wakeups by using wait helpers with a condition expression or by checking the condition under the same lock used by producers. `poll()` must read readiness from the same coherent state that read/write use.
- **API / Code Anchor:**
  ```c
  /*
   * The same spinlock protects readiness and ring updates.
   * Producers publish state under this lock, then wake readq.
   */
  spin_lock_irqsave(&dev->lock, flags);
  while (!data_available_locked(dev) && !dev->dead) {
          spin_unlock_irqrestore(&dev->lock, flags);
          ret = wait_event_interruptible(dev->readq,
                                         READ_ONCE(dev->data_avail) ||
                                         READ_ONCE(dev->dead));
          if (ret)
                  return -ERESTARTSYS;
          spin_lock_irqsave(&dev->lock, flags);
  }
  if (dev->dead) {
          spin_unlock_irqrestore(&dev->lock, flags);
          return -EIO;
  }
  n = ring_take(dev, tmp, sizeof(tmp));
  spin_unlock_irqrestore(&dev->lock, flags);

  if (copy_to_user(ubuf, tmp, n))
          return -EFAULT;
  ```
- **Production or Debugging Angle:** Run concurrent readers/writers, nonblocking I/O, poll loops, signal interruption, and IRQ/event injection. Lockdep catches many invalid lock paths; ftrace and dynamic debug help correlate state changes with wakeups. KCSAN can expose unsynchronized shared-state reads.
- **Common Traps:**
  - Holding a spinlock across `copy_*_user()` or `wait_event_interruptible()`.
  - Checking "no data" without holding the same lock used by the producer.
  - Waking before publishing the state change.
  - Returning `POLLIN` based on a stale counter.
  - Using one lock for everything until latency or deadlocks appear.
- **Follow-up Questions:**
  - When would you choose mutex versus spinlock for a char driver buffer?
  - How do you avoid a lost wakeup between condition check and sleep?
  - What should be copied under lock and what should be copied outside lock?

### 14. ABI, Maintainability, And Interface Choice

- **Level:** Senior
- **Question:** Before adding a custom character device to an Embedded Linux product, what design tradeoffs should you review?
- **Short Answer:** Confirm the interface model is right, define stable userspace behavior, decide permissions and naming, document error semantics, test concurrency and teardown, and compare alternatives such as subsystem drivers, `miscdevice`, sysfs, configfs, netlink, or an existing framework.
- **Deep Explanation:** A character device is powerful but easy to turn into a private, poorly documented ABI. The driver should have a clear model: byte stream, finite buffer, register window, command endpoint, or factory/debug interface. Read/write/seek/poll/ioctl behavior should be deliberate and documented. Permissions, `/dev` naming, udev rules, and compatible ioctl structures matter to applications as much as kernel code. Maintainability also includes failure unwinding, version caveats, test coverage, and whether a smaller framework such as `miscdevice` would remove boilerplate.
- **API / Code Anchor:**
  ```c
  static const struct file_operations demo_fops = {
          .owner          = THIS_MODULE,
          .open           = demo_open,
          .release        = demo_release,
          .read           = demo_read,      /* byte/data ABI */
          .write          = demo_write,     /* input/backpressure ABI */
          .llseek         = no_llseek,      /* stream semantics */
          .poll           = demo_poll,      /* event-loop ABI */
          .unlocked_ioctl = demo_ioctl,     /* command ABI */
          .compat_ioctl   = demo_compat_ioctl,
  };
  ```
- **Production or Debugging Angle:** A production review should include invalid pointers, invalid commands, short I/O, nonblocking mode, poll behavior, interrupted waits, concurrent access, forced init failures, wrong permissions, repeated load/unload, and remove-while-open. Debuggability should include clear logs for major/minor, minor routing, state transitions, and error mapping.
- **Common Traps:**
  - Creating a char device when an existing subsystem ABI would be better.
  - Shipping undocumented ioctl structs and then being unable to change them.
  - Leaving `/dev` permissions as an afterthought.
  - Using old examples without checking modern API prototypes.
  - Treating learning skeletons as production-ready lifetime and locking designs.
- **Follow-up Questions:**
  - When is `miscdevice` enough?
  - Which parts of `/dev` behavior become ABI for applications?
  - What tests would you require before accepting the driver upstream or into a product tree?

## Quick Coverage Checklist

- `/dev` node -> `dev_t` -> `struct cdev` -> `struct file_operations` -> driver callbacks.
- `open()` uses `inode->i_cdev` and initializes `file->private_data`.
- `cdev_add()` is the live boundary; cleanup is reverse order.
- Read/write return actual bytes, handle short I/O, and update `*ppos` when seekable.
- `copy_*_user()` returns bytes not copied, not `-errno`.
- Blocking paths use wait queues; `O_NONBLOCK` returns `-EAGAIN`.
- `poll_wait()` registers wait queues but does not report readiness by itself.
- Ioctl validates command magic, number, size, direction, user copies, `_IOWR`, `-ENOTTY`, fixed-width types, and possible `.compat_ioctl`.
- Removal must handle open files, sleeping tasks, references, and late `.release()`.
