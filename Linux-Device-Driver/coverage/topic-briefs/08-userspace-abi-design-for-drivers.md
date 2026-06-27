# Topic Brief - 08 - Userspace ABI Design For Drivers

## Output Targets
- Knowledge: `knowledge/08-userspace-abi-design-for-drivers.md`
- Interview: `interview/08-userspace-abi-design-for-drivers.md`
- Example: `examples/08-userspace-abi-design-for-drivers/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch04` | `docs/Linux Device Driver Development/Chapter 4-Character Device Drivers.md` | read/covered/merged | Primary source for `/dev` character-device ABI: `dev_t`, major/minor, `struct cdev`, `class_create()`, `device_create()`, `file_operations`, user-copy, `poll()`, and ioctl command numbering. |
| `ldd1-ch13` | `docs/Linux Device Driver Development/Chapter 13-The Linux Device Model.md` | read/covered/merged | Primary source for sysfs/device-model ABI: kobjects, ksets, attributes, `DEVICE_ATTR`, attribute groups, `/sys` topology, symlinks, and `sysfs_notify()`. |
| `ldd2-ch01` | `docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md` | read/mapped/covered/merged | Supporting source for blocking ABI behavior: interruptible waits inside ioctl/read paths, checking `wait_event_interruptible()` return values, and the rule that IRQ handlers must not copy to/from userspace. |
| `ldd2-ch12` | `docs/Linux Device Driver Development 2/Chapter 12-NVMEM_Framework.md` | read/mapped/covered/merged | Framework ABI example: NVMEM exposes binary sysfs files under `/sys/bus/nvmem/devices/.../nvmem`, distinguishes old deprecated ABI from new ABI, and shows `root_only`/`read_only` policy knobs. |
| `ldd2-ch14` | `docs/Linux Device Driver Development 2/Chapter 14-Linux_Kernel_Debugging_Tips.md` | read/mapped/covered/merged | Debug/control ABI examples through procfs, sysfs, debugfs, and tracefs: `/proc/sys/kernel/printk`, `/sys/module/.../parameters`, `/sys/kernel/debug/tracing`, trace events, and ftrace dump controls. |
| `notion-ch04-part1` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 1 Character Device Registration.md` | read/covered/merged | Clearer device-node creation flow: manual `mknod` versus automatic `udev`, `/sys/class/<class>/<device>/dev`, cleanup order, and multi-device `/dev` naming. |
| `notion-ch04-part2` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 2 Basic File Operations.md` | read/covered/merged | Beginner mental model for syscall-to-`file_operations`, `inode` versus `file`, `__user`, `copy_to_user()`, `copy_from_user()`, and `file->private_data`. |
| `notion-ch04-part3` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 3 Read, Write, and Seek Operations.md` | read/covered/merged | Practical ABI semantics for `read()`, `write()`, `llseek()`, `f_pos`, short I/O, EOF, `-EFAULT`, `-ENOSPC`, validation, and locking. |
| `notion-ch04-part4` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 4 Advanced Features - Poll and IOCTL.md` | read/covered/merged | Expanded `poll()`/`select()` and ioctl design notes, including `O_NONBLOCK`, `-EAGAIN`, wait queues, `_IO*` macros, magic/command validation, and user test flows. |
| `notion-ch05-part3` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 3 Device Provisioning & Integration.md` | read/mapped/covered/merged | Platform-driver integration with character ABI: per-device probe creates `cdev` and `/dev/pcdev-*`; reinforces that ABI nodes belong to discovered device instances. |
| `notion-ch14-part3` | `docs/Linux-Device-Driver-Notion/Chapter 14-Part 3 Userspace GPIO Access.md` | read/mapped/covered/merged | ABI contrast: legacy GPIO sysfs, pollable GPIO value files, kernel GPIO export helpers, and modern `/dev/gpiochipX` character-device/libgpiod ABI. |
| `notion-ch15-part3` | `docs/Linux-Device-Driver-Notion/Chapter 15-Part 3 Advanced Features and Integration.md` | read/mapped/covered/merged | GPIO-controller sysfs visibility: automatic `/sys/class/gpio/gpiochipN` entries with `base`, `label`, and `ngpio`; useful as framework-owned sysfs ABI example. |

## Source Files Read
- Required project guidance:
  - `Linux-Device-Driver/CODEX.md`
  - `Linux-Device-Driver/LEARNING_PATH.md`
  - `Linux-Device-Driver/.agents/skills/linux-device-driver/SKILL.md`
  - `Linux-Device-Driver/.agents/skills/linux-device-driver/references/topic-brief-template.md`
  - `Linux-Device-Driver/.codex/agents/lld-topic-explorer.toml`
- `ldd1-ch04`: full file read.
  - Relevant sections: major/minor and `dev_t`, device number allocation, `struct file_operations`, `inode` versus `file`, `cdev` registration, `class_create()`, `device_create()`, `copy_to_user()`, `copy_from_user()`, `get_user()`, `put_user()`, `open()`, `release()`, `read()`, `write()`, `llseek()`, `poll()`, ioctl command macros, and ioctl userspace header sharing.
- `ldd1-ch13`: full sysfs/device-model sections read.
  - Relevant sections: LDM purpose, `struct bus_type`, driver/device/class/bus attributes, kobjects, ksets, `kobj_type`, `sysfs_ops`, attributes, attribute groups, `/sys` topology, `device_create_file()`, `class_create_file()`, `bus_create_file()`, `driver_create_file()`, and `sysfs_notify()`.
- `ldd2-ch01`: relevant wait/IRQ excerpts read.
  - Relevant sections: interruptible wait bug story involving ioctl, `wait_event_interruptible()` return handling, and interrupt-handler restrictions against sleeping and user-copy.
- `ldd2-ch12`: full file read.
  - Relevant sections: `struct nvmem_config`, `read_only`, `root_only`, `nvmem_register()`, old RTC NVRAM ABI, provider callbacks, DT-defined cells, and `NVMEM in user space`.
- `ldd2-ch14`: relevant logging/debug ABI sections read.
  - Relevant sections: `/proc/sys/kernel/printk`, `/sys/module/printk/parameters/time`, ftrace under `/sys/kernel/debug/tracing`, trace events, process-specific tracing, and `/proc/sys/kernel/ftrace_dump_on_oops`.
- `notion-ch04-part1`: relevant registration/device-node sections read, including beginning mental model and class/device creation sections.
- `notion-ch04-part2`: relevant file-operation and user-copy sections read.
- `notion-ch04-part3`: relevant read/write/seek sections and complete EEPROM-like example read.
- `notion-ch04-part4`: full file read.
- `notion-ch05-part3`: runtime device creation and pseudo character platform-driver example read.
- `notion-ch14-part3`: legacy sysfs GPIO, poll example, kernel export helpers, modern libgpiod character-device interface, and summary read.
- `notion-ch15-part3`: full file read.

## Merged Source Notes
- The topic should be taught as an ABI design decision, not as "many random ways to expose data." The driver author chooses the narrowest stable userspace contract: normal file operations for byte streams, sysfs attributes for small device properties, framework ABIs when a subsystem already exists, ioctl only when structured/custom commands are needed, and debugfs/tracefs only for debugging.
- `ldd1-ch04` and Notion chapter 4 overlap heavily, but both were read and compared. `ldd1-ch04` is the compact mechanism source; Notion chapter 4 gives better learning structure, syscall mapping, non-blocking behavior, test programs, and cleanup examples.
- `ldd1-ch13` supplies the main sysfs mechanism. Merge its low-level kobject explanation only enough to explain why `/sys` mirrors kernel object topology and why device/class/bus/driver attributes are wrappers around `sysfs_create_file()`.
- `ldd2-ch12` is a strong framework-ABI example: a driver registers with NVMEM and the framework exposes a userspace ABI. This supports the production rule "prefer an existing subsystem ABI over inventing a private ioctl/sysfs layout."
- `notion-ch14-part3` and `notion-ch15-part3` show ABI evolution: old GPIO sysfs is easy but deprecated; modern GPIO userspace access is through `/dev/gpiochipX` and libgpiod; GPIO controller sysfs files are still useful for enumeration/inspection.
- `ldd2-ch14` should be merged as "debug interfaces are not production control ABI." `/proc/sys`, module parameters under `/sys/module`, debugfs/tracefs, and ftrace are valuable for debugging and tuning, but should not be confused with stable device control contracts.
- Blocking semantics belong in ABI design. `ldd1-ch04`, Notion part 4, and `ldd2-ch01` together show that `poll()`, `O_NONBLOCK`, wait queues, `-EAGAIN`, `-ERESTARTSYS`, and signal handling must be part of the interface contract.

## Source Differences
- `ldd1-ch04` uses `_IORW` in one macro list; current kernel convention is `_IOWR`. Use `_IOWR` in future learner-facing docs.
- `ldd1-ch04` says ioctl headers may be duplicated between kernel and userspace. Prefer one UAPI-style shared definition when possible; avoid silently drifting copies.
- Older examples use `sprintf()`/`scnprintf()` in sysfs `show()` callbacks. Current kernel docs recommend `sysfs_emit()` or `sysfs_emit_at()` for sysfs output.
- Older examples use direct `sysfs_create_file()` and per-attribute creation. For production drivers, prefer subsystem/device-model wrappers and default attribute groups where appropriate.
- `class_create()` signatures are kernel-version-sensitive; examples with `class_create(THIS_MODULE, name)` may not match current headers.
- `.poll` return type is kernel-version-sensitive: older material uses `unsigned int`; newer kernels commonly use `__poll_t`.
- GPIO sysfs material is useful for legacy understanding, but current kernel documentation marks it deprecated/obsolete for new userspace consumers. Teach it as compatibility knowledge, not a new-design recommendation.
- NVMEM's RTC `nvram_old_abi` example is intentionally about preserving old applications. New drivers should avoid setting old ABI flags unless compatibility requires it.
- `debugfs` and `tracefs` examples are runtime debug interfaces. They may be unavailable, root-only, kernel-config dependent, and unsuitable as stable production ABI.

## Gaps / Uncertainties
- Need current-kernel validation before writing the final example:
  - `class_create()` signature.
  - `.poll` return type and mask constants.
  - Best current ioctl `compat_ioctl` handling for 32-bit userspace on 64-bit kernels.
  - Whether an example should use a plain character device, `miscdevice`, or a subsystem framework to illustrate ABI design.
- The internal sources do not provide a dedicated procfs driver interface example. If procfs is included, keep it brief: system/proc control and debug examples only, and validate with current kernel docs.
- The internal sources do not deeply cover ABI stability policy, UAPI header placement, struct padding/reserved fields, endian/time-size concerns, or "never break userspace." These need external validation before final learner docs.
- `mmap()` is listed in `file_operations` but not deeply explained in the ABI-design sources read. Treat it as a gap or defer deep `mmap()` mechanics to memory/DMA/V4L2 topics.
- Permissions, udev rules, groups, SELinux/AppArmor, and production device-node policy are only lightly covered by internal sources.

## External Validation
- Used: https://docs.kernel.org/driver-api/ioctl.html
  - Validates modern ioctl guidance, `_IO`, `_IOR`, `_IOW`, `_IOWR`, the difficulty of fixing broken ioctl ABI, and `compat_ioctl` concerns for 32-bit userspace on 64-bit kernels.
- Used: https://docs.kernel.org/6.11/userspace-api/ioctl/ioctl-number.html
  - Validates ioctl number uniqueness conventions, the user-perspective meaning of read/write in `_IOR`/`_IOW`, and the warning not to use `sizeof(arg)` as the third macro argument.
- Used: https://docs.kernel.org/6.4/filesystems/sysfs.html
  - Validates sysfs purpose, kobject relationship, `show()`/`store()` behavior, PAGE_SIZE buffer semantics, `sysfs_emit()`, and one-value/simple-values guidance.
- Used: https://docs.kernel.org/next/admin-guide/gpio/sysfs.html
  - Validates that GPIO sysfs is deprecated and that new userspace consumers should use the character-device ABI.
- Used: https://docs.kernel.org/admin-guide/abi.html
  - Validates that kernel ABI documentation distinguishes stable/testing/obsolete interfaces and documents userspace ABI stability.
- Used: https://docs.kernel.org/admin-guide/abi-obsolete.html
  - Validates obsolete GPIO sysfs entries and their replacement by the GPIO character-device ABI.

## Learning Content Brief
- Mental model:
  - A driver userspace ABI is a promise, not just a convenient test hook.
  - `/dev` exposes operations on device instances; `/sys` exposes small properties and topology; framework ABIs expose standardized subsystem behavior; ioctl extends a file descriptor with structured commands; debugfs/tracefs/proc/sys/module parameters help observe or tune the kernel but are not substitutes for a stable production ABI.
- Core mechanism:
  - Character devices connect `/dev/<name>` to `struct file_operations` through `dev_t`, `struct cdev`, VFS, `struct inode`, and `struct file`.
  - `class_create()` and `device_create()` create sysfs class/device entries and provide the `dev` major:minor metadata that lets udev/devtmpfs create the corresponding device node.
  - Sysfs files are attributes of kernel objects. Device, class, bus, and driver attributes wrap a generic `struct attribute` plus `show()`/`store()` callbacks.
  - `poll()` is readiness notification, not data transfer: the driver registers wait queues with `poll_wait()`, returns readiness masks, and wakes waiters when state changes.
  - ioctl is a custom command ABI using fixed command numbers and explicit user-copy for pointer arguments.
- Important structs/APIs:
  - Device nodes: `dev_t`, `MAJOR()`, `MINOR()`, `MKDEV()`, `alloc_chrdev_region()`, `unregister_chrdev_region()`, `struct cdev`, `cdev_init()`, `cdev_add()`, `cdev_del()`, `struct class`, `class_create()`, `class_destroy()`, `device_create()`, `device_destroy()`.
  - File ABI: `struct file_operations`, `open`, `release`, `read`, `write`, `llseek`, `poll`, `unlocked_ioctl`, `mmap`, `struct inode`, `struct file`, `file->private_data`, `copy_to_user()`, `copy_from_user()`, `get_user()`, `put_user()`.
  - Sysfs: `struct kobject`, `struct kobj_type`, `struct kset`, `struct attribute`, `struct attribute_group`, `struct device_attribute`, `DEVICE_ATTR`, `device_create_file()`, `device_remove_file()`, `sysfs_create_group()`, `sysfs_remove_group()`, `sysfs_notify()`, `sysfs_emit()`.
  - Ioctl: `_IO`, `_IOR`, `_IOW`, `_IOWR`, `_IOC_TYPE()`, `_IOC_NR()`, `_IOC_SIZE()`, `access_ok()` where appropriate, `compat_ioctl` where 32-bit/64-bit layout matters.
  - Debug/control examples: `/proc/sys/kernel/printk`, `/sys/module/<module>/parameters/*`, `/sys/kernel/debug/tracing/*`, `/sys/bus/nvmem/devices/*/nvmem`, `/dev/gpiochipX`.
- Lifecycle/data flow:
  - Probe/init allocates driver state, reserves device numbers, initializes and adds `cdev`, creates class/device nodes, and exposes any sysfs attributes/groups after the backing state is valid.
  - Userspace opens the device node; `open()` finds per-device state, usually through `container_of(inode->i_cdev, ...)`, and stores it in `file->private_data`.
  - Reads/writes validate offset, size, permissions, state, and buffer bounds, copy safely across the user/kernel boundary, update `f_pos` where meaningful, and return short counts when appropriate.
  - Blocking operations wait on a condition, handle signals, and wake `poll()`/`read()`/`write()` waiters when state changes.
  - Teardown removes attributes/device nodes, deletes `cdev`, unregisters device numbers, and handles remove-while-open/lifetime rules carefully.
- Design rules:
  - Prefer an existing subsystem ABI when the hardware fits a kernel framework.
  - Use sysfs for small, textual, one-value or simple-list attributes; avoid large binary protocols unless the framework defines a binary attribute ABI.
  - Use read/write for byte streams or memory-like data.
  - Use ioctl for structured commands that do not fit read/write/sysfs, but design structs for long-term compatibility with explicit sizes, reserved fields, stable types, and no raw kernel pointers.
  - Never expose debugfs as required production ABI.
  - Never break userspace once the ABI is shipped; add compatible extensions instead.
- Common bugs:
  - Dereferencing `__user` pointers directly.
  - Forgetting to check `copy_to_user()`/`copy_from_user()` return values.
  - Returning the wrong errno for unsupported ioctl commands; use `-ENOTTY`.
  - Using unstable struct layouts in ioctl without padding/reserved fields.
  - Sleeping or copying to userspace from IRQ context.
  - Failing to check `wait_event_interruptible()` return values.
  - Implementing `poll()` but forgetting to wake the wait queue when state changes.
  - Exporting complex control state through many ad hoc sysfs files instead of a framework or ioctl.
  - Treating GPIO sysfs as the recommended new ABI.
  - Removing sysfs attributes or device nodes while userspace can still race with backing object lifetime.
- Debugging notes:
  - Verify `/dev` node creation with `ls -l /dev/<name>` and `/sys/class/<class>/<name>/dev`.
  - Use `udevadm info`/`udevadm monitor` later if udev rules or permissions are part of the example.
  - Use `strace` to confirm userspace calls, ioctl numbers, `EAGAIN`, `ENOTTY`, and short reads/writes.
  - Use `dmesg`, `dev_dbg()`, dynamic debug, and ftrace/tracefs to observe kernel-side paths.
  - For sysfs, test full read/write behavior, invalid input, permissions, and whether `poll()`/`sysfs_notify()` behaves as expected.
- Production concerns:
  - Document every exported ABI path, command, field, errno, blocking behavior, units, permissions, and versioning expectation.
  - Keep UAPI types fixed-width and userspace-safe.
  - Design for 32-bit userspace on 64-bit kernels when ioctl structs contain pointers, longs, times, or alignment-sensitive fields.
  - Use locking around shared state accessed by file operations, sysfs callbacks, IRQ/workqueue paths, and remove paths.
  - Avoid security surprises: validate all user input, enforce permissions, and do not leak kernel memory or uninitialized padding.
- Interview angles:
  - Explain why userspace cannot call driver functions directly and how `/dev` reaches `file_operations`.
  - Compare sysfs, ioctl, read/write, debugfs, and framework ABIs.
  - Explain why sysfs should expose simple attributes and why large protocols in sysfs are a smell.
  - Walk through ioctl command design and why `_IOW` means userspace writes data to the kernel.
  - Explain `poll()` readiness versus blocking `read()`.
  - Diagnose a crash caused by direct user-pointer dereference or unchecked interruptible wait.
  - Explain why GPIO sysfs is deprecated and what replaces it.
  - Explain why "do not break userspace" matters for driver ABI.
