# Topic Brief - 38 - Production Driver Checklist And Interview Review

## Output Targets

| Learning Path | Slug | Coverage Brief | Knowledge Target | Interview Target | Example Target |
| --- | --- | --- | --- | --- | --- |
| 38 - Production Driver Checklist And Interview Review | `production-driver-checklist-and-interview-review` | `coverage/topic-briefs/38-production-driver-checklist-and-interview-review.md` | `knowledge/38-production-driver-checklist-and-interview-review.md` | `interview/38-production-driver-checklist-and-interview-review.md` | Not listed in `LEARNING_PATH.md`; no example expected unless a later step requests one. |

## Source Coverage

| Source ID | Source Root | File | Status | Key Contribution |
| --- | --- | --- | --- | --- |
| `ldd1-source-root` | ldd1 | `docs/Linux Device Driver Development/` | `searched/mapped/covered` | No standalone production-review chapter. Relevant material is distributed across module basics, platform probe/remove, Device Tree, memory, DMA, device model/sysfs, IRQ, regulator/power-like resource usage, framebuffer, and network chapters. |
| `ldd1-ch02` | ldd1 | `Chapter 2-Device Driver Basis.md` | `read/mapped/covered` | Module load/unload, `insmod` vs `modprobe`, module aliases, auto-loading, module reference/unload safety, module skeleton, `module_init()` / `module_exit()`, and why driver initialization/exit is not the same as per-device probe/remove. |
| `ldd1-ch05` | ldd1 | `Chapter 5-Platform Device Drivers.md` | `read/mapped/covered/merged-primary` | Platform-driver probe/remove model, bus matching, `module_platform_driver()`, resources, `platform_get_resource()`, `platform_get_irq()`, old board-file provisioning, and the warning that `platform_driver_probe()` prevents deferred probe. |
| `ldd1-ch06` | ldd1 | `Chapter 6-The Concept of a Device Tree .md` | `read/mapped/covered` | Device Tree mental model, `compatible`, `reg`, phandles, `#address-cells`, `#size-cells`, I2C/SPI/platform address interpretation, `dtc`, runtime DT inspection, and binding-specific hardware description. |
| `ldd1-ch11` | ldd1 | `Chapter 11-Kernel Memory Management.md` | `read/mapped/covered` | Kernel/user address separation, VMAs, page faults, allocation constraints, I/O memory access, `mmap()` background, and devres mention. Supports memory, user-pointer, and lifetime review items. |
| `ldd1-ch12` | ldd1 | `Chapter 12-DMA - Direct Memory Access.md` | `read/mapped/covered/merged-primary` | DMA coherency, coherent vs streaming mappings, `dma_alloc_coherent()`, `dma_map_single()`, `dma_map_sg()`, DMA direction, sync-for-CPU/device APIs, DMAengine transaction lifecycle, completions, and unmap/free cleanup rules. |
| `ldd1-ch13` | ldd1 | `Chapter 13-The Linux Device Model.md` | `read/mapped/covered/merged-primary` | `struct device`, `struct device_driver`, binding, reference-counted `kobject`, `release`, sysfs attributes/groups, default groups, `show()`/`store()` semantics, sysfs removal, and pollable sysfs attributes. |
| `ldd1-ch14` | ldd1 | `Chapter 14-Pin Control and GPIO Subsystem.md` | `read/mapped/related` | Pinctrl state selection, GPIO context rules, GPIO descriptor errors including `-EPROBE_DEFER`, GPIO-to-IRQ, threaded IRQ request, and active polarity. Related to DT/resource/context checklist items. |
| `ldd1-ch16` | ldd1 | `Chapter 16-Advanced IRQ Management.md` | `read/mapped/covered` | IRQ domains, `irq_chip`, `irq_data`, `irq_set_type`, `irq_set_wake`, slow-bus IRQ locking, hwirq-to-virq mapping, chained/nested propagation, and IRQ context caveats. |
| `ldd1-ch21` | ldd1 | `Chapter 21-Framebuffer Drivers.md` | `read/mapped/related` | Concrete subsystem example where probe allocates/registers, remove unregisters/frees, DMA/clock setup is reversed, framebuffer ioctl/mmap/sysfs ABI is exposed, and blanking should stop clocks/power. |
| `ldd1-ch22` | ldd1 | `Chapter 22-Network Interface Card Drivers.md` | `searched/mapped/related` | Network driver source was found as an adjacent production example for registration, IRQ/DMA, and debug bring-up; detailed netdev review remains topic 30. |
| `ldd2-source-root` | ldd2 | `docs/Linux Device Driver Development 2/` | `searched/mapped/covered` | No standalone production-checklist chapter, but book 2 contributes current-style locking, PM, PCI, NVMEM/watchdog ABI, and debugging guidance. |
| `ldd2-ch01` | ldd2 | `Chapter 1-Linux_Kernel_Concepts.md` | `read/mapped/covered/merged-primary` | Locking API, shared objects, spinlocks vs mutexes, interrupt-disabled deadlock pattern, `_irqsave` variants, wait queues, completions, deferred work, and context rules. |
| `ldd2-ch10` | ldd2 | `Chapter 10-Linux_Kernel_Power_Management.md` | `read/mapped/covered/merged-primary` | Runtime PM and system sleep, `struct dev_pm_ops`, `SET_RUNTIME_PM_OPS`, `pm_runtime_get_sync()`, `pm_runtime_put_autosuspend()`, autosuspend delay, usage-count balancing, remove-time PM shutdown, and wakeup testing ideas. |
| `ldd2-ch11` | ldd2 | `Chapter 11-Writing_PCI_Device_Drivers.md` | `read/mapped/covered` | PCI enumeration, IDs, BAR/config space, `struct pci_dev`, `struct pci_driver`, MSI/MSI-X vs INTx, DMA masks/interrupt vector caveats, and production pitfalls around resource enablement. |
| `ldd2-ch12` | ldd2 | `Chapter 12-NVMEM_Framework.md` | `searched/mapped/related` | ABI review example: NVMEM provider/consumer model, sysfs binary entry, old NVRAM ABI caveat, DT cells, read-only policy, and `devm_nvmem_register()`. |
| `ldd2-ch13` | ldd2 | `Chapter 13-Watchdog_Device_Drivers.md` | `searched/mapped/related` | Reliability-oriented subsystem example: watchdog timeout/keepalive, nowayout expectations, user ABI implications, and board safety policy. |
| `ldd2-ch14` | ldd2 | `Chapter 14-Linux_Kernel_Debugging_Tips.md` | `read/mapped/covered/merged-primary` | Kernel release model, logging levels, `pr_*`, `dev_*`, console loglevel, log buffer, timestamps, ftrace, tracefs/debugfs controls, function/function_graph tracing, filters, latency tracers, and oops/postmortem context. |
| `notion-source-root` | notion | `docs/Linux-Device-Driver-Notion/` | `searched/mapped/covered` | No standalone production-checklist page found. Notion was searched and read independently; it contributes explicit best-practice/checklist-style material around error handling, probe/remove, DT, async work, poll/ioctl, IRQs, GPIO, and V4L2 async probing. |
| `notion-ch01-part4` | notion | `Chapter 1-Part 4 Coding Style and Best Practices .md` | `read/mapped/covered` | Coding style, reviewability, readability, maintainability, 8-character tabs, line length, brace rules, and style as a defect-prevention aid. |
| `notion-ch02-part3` | notion | `Chapter 2-Part 3 Error Handling & Message Printing.md` | `read/mapped/covered/merged-primary` | Error-code selection, negative errno convention, propagation to userspace, `ERR_PTR()` family context, `goto` cleanup rationale, kernel logging, and why kernel errors are more dangerous than userspace errors. |
| `notion-ch03-part4` | notion | `Chapter 3-Part 4 Work Queues and Scheduling.md` | `read/mapped/covered` | Tasklet cleanup, spinlock requirement for tasklet/shared data, no sleeping in tasklet/atomic context, and deferred-work best practices. Some tasklet material may need version-sensitive validation because new code often prefers other mechanisms. |
| `notion-ch03-part5` | notion | `Chapter 3-Part 5 Work Queues and Scheduling.md` | `read/mapped/covered` | Timer cleanup, `del_timer()` vs `del_timer_sync()`, hrtimer cancellation, no sleeping in timer callbacks, and remove-path teardown ordering. |
| `notion-ch04-part4` | notion | `Chapter 4-Part 4 Advanced Features - Poll and IOCTL.md` | `read/mapped/covered` | Userspace ABI review items: blocking vs non-blocking I/O, `poll_wait()`, wait queues, readiness masks, ioctl/poll behavior, and why busy-waiting is not acceptable. |
| `notion-ch05-part2` | notion | `Chapter 5-Part 2 Probe, Remove & Resource Management.md` | `read/mapped/covered/merged-primary` | Explicit probe workflow, resource extraction, private data allocation, hardware init, IRQ request, subsystem registration, and reverse-order cleanup with `goto`. |
| `notion-ch05-part3` | notion | `Chapter 5-Part 3 Device Provisioning & Integration.md` | `read/mapped/covered/merged-primary` | Best-practice list: use `devm_*`, proper `goto`, `platform_set_drvdata()`, DT support, release function, avoid hardcoded addresses, do not confuse probe with module init, provide `MODULE_DEVICE_TABLE`, and do not access hardware before probe. |
| `notion-ch06-part3` | notion | `Chapter 6-Part 3 OF APIs and Platform Integration.md` | `read/mapped/covered` | OF headers/APIs, `struct device_node`, property reading, `of_property_read_*`, phandle parsing context, and probe-time DT validation. |
| `notion-ch08-extra-part1` | notion | `Chapter 8-Part 1.md` | `read/mapped/related` | V4L2 async/fwnode checklist showing unordered DT probing, `.bound`, `.complete`, `.unbind`, graph binding, and why deferred/async composition belongs in senior scenarios. Not a duplicate of SPI chapter 8. |
| `notion-ch15-part3` | notion | `Chapter 15-Part 3 Advanced Features and Integration.md` | `read/mapped/covered` | GPIO-controller best practices: resource-managed APIs, mutex-protected register access, register caching, DT integration, sysfs exposure, threaded IRQ, and complete-driver cleanup patterns. |
| `notion-ch16-part3` | notion | `Chapter 16-Part 3 Advanced Topics.md` | `read/mapped/covered` | Threaded IRQ model, top half vs bottom half, `request_threaded_irq()`, `IRQF_ONESHOT`, I2C/SPI-in-threaded-handler rationale, and context-safe interrupt work. |

## Source Files Read

- `Linux-Device-Driver/CODEX.md`
- `Linux-Device-Driver/LEARNING_PATH.md`
- `Linux-Device-Driver/.agents/skills/linux-device-driver/SKILL.md`
- `Linux-Device-Driver/.agents/skills/linux-device-driver/references/topic-brief-template.md`
- `Linux-Device-Driver/.codex/agents/lld-topic-explorer.toml`
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 2-Device Driver Basis.md`
  - Module loading/unloading, module aliases, module skeleton, module init/exit, unload reference safety.
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 5-Platform Device Drivers.md`
  - Platform driver registration, probe/remove, resource extraction, `module_platform_driver()`, deferred-probe warning.
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 6-The Concept of a Device Tree .md`
  - DT syntax, labels/phandles, `compatible`, `reg`, I2C/SPI/platform addressing, `dtc`, runtime inspection.
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 11-Kernel Memory Management.md`
  - Kernel/user memory, VMA/page-fault concepts, allocation context, I/O memory and devres background.
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 12-DMA - Direct Memory Access.md`
  - Coherent/streaming mapping, cache coherency, DMA directions, sync APIs, DMAengine, completions.
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 13-The Linux Device Model.md`
  - `struct device`, `struct device_driver`, binding, kobject lifetime, sysfs attributes/groups and removal.
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 14-Pin Control and GPIO Subsystem.md`
  - Pinctrl/GPIO context rules, descriptor APIs, `-EPROBE_DEFER`, GPIO IRQ.
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 16-Advanced IRQ Management.md`
  - IRQ domains, `irq_chip`, `irq_data`, hwirq/virq, `irq_set_wake`, slow-bus locking.
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 21-Framebuffer Drivers.md`
  - Probe/register/remove pattern, ioctl/mmap/sysfs ABI, blanking/power-down behavior.
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md`
  - Locking, spinlocks, mutexes, wait queues, completions, deferred work, interrupt-context rules.
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 10-Linux_Kernel_Power_Management.md`
  - Runtime PM, system sleep, `struct dev_pm_ops`, autosuspend, PM usage count, remove-time shutdown.
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 11-Writing_PCI_Device_Drivers.md`
  - PCI IDs, enumeration, `struct pci_dev`, `struct pci_driver`, MSI/MSI-X, resource/DMA context.
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 14-Linux_Kernel_Debugging_Tips.md`
  - Logging, `dev_*`, log levels, ftrace, tracefs, function graph tracing, latency tracers, oops/debugging.
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 1-Part 4 Coding Style and Best Practices .md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 2-Part 3 Error Handling & Message Printing.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 3-Part 4 Work Queues and Scheduling.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 3-Part 5 Work Queues and Scheduling.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 4-Part 4 Advanced Features - Poll and IOCTL.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 5-Part 2 Probe, Remove & Resource Management.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 5-Part 3 Device Provisioning & Integration.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 6-Part 3 OF APIs and Platform Integration.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 8-Part 1.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 15-Part 3 Advanced Features and Integration.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 16-Part 3 Advanced Topics.md`

## Inventory Decisions

- Topic 38 is a capstone/review topic, not a new framework. No internal source group contains a single source that matches the full scope.
- The primary merge sources are `ldd1-ch05`, `ldd1-ch12`, `ldd1-ch13`, `ldd2-ch01`, `ldd2-ch10`, `ldd2-ch14`, `notion-ch02-part3`, `notion-ch05-part2`, and `notion-ch05-part3`.
- Subsystem chapters were used as production review examples when they exposed a lifecycle, ABI, resource, PM, DMA, IRQ, or debug pattern. They should not be re-taught in full in topic 38.
- Same-number chapters were not merged by number:
  - `ldd1-ch05` is platform drivers; `ldd2-ch05` is ASoC framework; `notion-ch05-*` is platform driver expansion.
  - `ldd1-ch14` is pinctrl/GPIO consumer APIs; `ldd2-ch14` is kernel debugging; `notion-ch14-*` is pinctrl/GPIO material.
  - Notion has two different Chapter 8 namespaces: SPI files and V4L2 async/media files. The V4L2 async file was treated as `notion-ch08-extra-part1`, not as a duplicate of SPI.
- Notion was not skipped. Its explicit best-practice sections provide clearer checklist wording than the book sources, but its examples can be simplified or version-sensitive.

## Merged Source Notes

### Core Mental Model

- Production driver review is a search for bugs at boundaries:
  - hardware discovery and binding;
  - resource acquisition and cleanup;
  - lifetime of kernel objects visible to other code or userspace;
  - context rules for sleeping, locking, IRQs, timers, workqueues, and PM;
  - ABI stability and userspace semantics;
  - Device Tree or firmware description correctness;
  - DMA/cache ownership;
  - power state transitions;
  - debug visibility and diagnosability.
- The final chapter should teach a review loop: identify entry points, identify resources, identify externally visible registrations, identify asynchronous producers, identify userspace ABI, then walk every failure/remove/suspend path in reverse.
- The strongest practical rule from overlapping sources: register externally visible objects only after required private state, resources, IRQ/DMA, clocks, regulators, and hardware state are ready; on teardown, first stop new activity and unregister externally visible objects, then stop IRQs/timers/work/DMA, then release resources.

### Probe / Remove / Error Paths

- Probe is per-device, while module init is per-module. Confusing these is a common beginner and interview trap.
- Probe should normally:
  1. allocate private state and save driver data;
  2. parse firmware/resources;
  3. map registers and acquire clocks, regulators, resets, GPIOs, IRQs, DMA channels, and pinctrl states;
  4. initialize locks, wait queues, completions, timers, and work;
  5. verify hardware if possible;
  6. initialize hardware;
  7. register with the subsystem or expose ABI.
- Error paths should unwind in reverse order. The Notion sources explicitly recommend `goto` cleanup labels; book sources show the same pattern in subsystem examples.
- `devm_*` APIs reduce boilerplate but do not remove the need to quiesce hardware, unregister subsystem-visible state, stop timers/work/DMA/IRQs, or balance PM usage counts before devres releases memory.
- Use correct errno:
  - `-ENOMEM` for allocation failure;
  - `-EINVAL` for invalid arguments/properties;
  - `-ENODEV` for unsupported or missing device;
  - `-EBUSY` for resource ownership conflicts;
  - `-EIO` for hardware I/O failure;
  - `-EPROBE_DEFER` for unavailable suppliers.
- Later learner docs should include `dev_err_probe()` as a current best practice for probe failures involving deferral, even though it is not emphasized in the internal sources.

### Lifetime And Ownership

- `struct device`, `kobject`, sysfs, classes, subsystem registrations, IRQs, DMA descriptors, timers, work items, file handles, and userspace mappings each add lifetime obligations.
- A `release` callback is not optional for manually registered devices. Notion explicitly calls this out; book 1 explains `release` through kobject/device lifetime.
- Review visible registrations separately from memory allocation:
  - `cdev_add()`, `device_create()`, `input_register_device()`, `register_netdev()`, `iio_device_register()`, `video_register_device()`, `rtc_register_device()`, `devm_snd_soc_register_card()`, and similar calls make the object reachable by other code or userspace.
  - After registration, callbacks may run concurrently with probe tail, remove, suspend, IRQ, or userspace close.
- Teardown must handle "remove while open", "IRQ while remove runs", "timer/work after free", "DMA callback after buffer free", and "sysfs attribute after device state is gone".

### Locking / Context / Async Work

- `ldd2-ch01` supplies the main locking model:
  - mutexes are for sleeping/task context;
  - spinlocks are for atomic/IRQ/SMP contexts;
  - use IRQ-safe variants when data is shared with hard IRQ context;
  - incorrect `_irq` usage can restore interrupt state incorrectly; `_irqsave` preserves prior state.
- Threaded IRQs from Notion Chapter 16 clarify how slow I2C/SPI/register work moves out of hard IRQ context and why `IRQF_ONESHOT` matters.
- Tasklet/timer Notion sources add cleanup rules:
  - no sleeping in tasklet or timer callbacks;
  - use spinlocks for shared tasklet data;
  - use `tasklet_kill()`, `del_timer_sync()`, or `hrtimer_cancel()` during cleanup;
  - never call synchronous timer deletion from the timer callback itself.
- Wait queues, completions, and `poll_wait()` need condition-state correctness: wake after state change, protect condition updates, and return accurate readiness masks.
- Senior review should ask for the context of every callback: process, hard IRQ, threaded IRQ, workqueue, timer, PM, sysfs, ioctl, mmap fault, or DMA completion.

### Userspace ABI

- ABI review means choosing the subsystem ABI first. Private char devices, custom ioctls, sysfs attributes, debugfs files, and procfs entries need stronger justification than a standard subsystem interface.
- Character-device and poll/ioctl sources support these review rules:
  - validate all user sizes, commands, flags, offsets, and indexes;
  - use `copy_from_user()` / `copy_to_user()`;
  - return `-EFAULT`, `-EINVAL`, `-EAGAIN`, `-ERESTARTSYS`, and byte counts correctly;
  - handle blocking and non-blocking behavior;
  - make `poll()` report current readiness and register wait queues;
  - consider compat ioctl for 32-bit userspace on 64-bit kernels when needed.
- Sysfs attributes should be simple, text-oriented, one value or compact group per file, and not a dumping ground for complex ABI. Debugfs/tracefs are debug interfaces, not stable ABI.
- Once exposed, ABI cannot be casually changed. Topic 38 should reinforce ABI as part of production review, not only an implementation detail.

### Device Tree / Firmware Description

- `ldd1-ch06`, `notion-ch06-part3`, and platform-driver sources converge on: describe hardware, not Linux driver policy.
- Production DT review items:
  - `compatible` matches driver table and has `MODULE_DEVICE_TABLE()` when autoloading matters;
  - `reg` matches the parent bus address/size cells;
  - interrupts include trigger type and correct parent;
  - clocks, resets, regulators, GPIOs, pinctrl states, and DMA channels use binding-defined names;
  - GPIO polarity is encoded in DT, not inverted ad hoc in the driver;
  - optional properties have documented defaults;
  - graph/endpoint bindings are used for composed devices such as media/audio when appropriate.
- `-EPROBE_DEFER` is not failure in the normal sense; it means a supplier is not ready yet. The final docs should explain how to distinguish real missing hardware from dependency ordering.
- External validation should cover YAML binding and `dtbs_check` because internal sources still mention older `.txt` binding paths in several places.

### Memory, MMIO, DMA

- Memory review items:
  - use the right GFP flags for context;
  - avoid integer overflows in allocation sizes;
  - do not dereference userspace pointers directly;
  - do not access `__iomem` with normal pointers;
  - use resource-managed MMIO helpers where appropriate.
- DMA review items:
  - coherent buffers are simpler but expensive and usually long-lived;
  - streaming mappings require strict CPU/device ownership;
  - direction must be correct; avoid lazy `DMA_BIDIRECTIONAL`;
  - map before device access, unmap after completion, and sync if ownership changes before unmap;
  - for scatter/gather, program the device with the mapped entry count, not blindly the original list count;
  - unmap/free on every error, timeout, remove, and suspend path.
- DMA review should be tied to IRQ and completion review: timeout paths must terminate or recover hardware activity before freeing buffers.

### Power Management

- `ldd2-ch10` is the primary power-management source.
- Production runtime PM review:
  - call `pm_runtime_enable()` only after setting a coherent initial state;
  - balance every get/put path including errors;
  - use autosuspend only after marking last busy;
  - resume before hardware access and idle/put after access;
  - disable runtime PM during remove only after unregistering users and quiescing activity.
- Production system sleep review:
  - stop ongoing I/O safely;
  - mask or configure IRQ wake correctly;
  - save/restore hardware state;
  - restore clocks/regulators/pinctrl in correct order;
  - test repeated suspend/resume under active userspace, not just idle boot.
- Wakeup source behavior belongs in senior interview scenarios because it crosses IRQ, PM, DT, and userspace policy.

### Debugging / Observability

- `ldd2-ch14` supplies the main debug source. Production drivers should be diagnosable without enabling noisy permanent logs.
- Prefer `dev_*()` in drivers so messages include device identity. Use `dev_dbg()` / dynamic debug for normally disabled debug messages.
- Production logs should:
  - report real errors with actionable context;
  - avoid spamming hot paths;
  - not expose secrets;
  - include register/resource identifiers when useful;
  - use subsystem tracing/debugfs where available.
- Debugging checklist:
  - `dmesg -w`, console loglevel, module parameters;
  - dynamic debug for callsites;
  - ftrace/function_graph for call flow and latency;
  - trace events for subsystem behavior;
  - debugfs/sysfs state for subsystem internals;
  - `/proc/interrupts`, `/sys/kernel/debug/gpio`, clock/regulator summaries, PM sysfs, and subsystem-specific tools.
- `trace_printk()` and ad hoc debugfs should be temporary development aids, not production ABI.

### Interview Review Structure

- Beginner interview should test:
  - module init vs probe;
  - what probe/remove do;
  - why `goto` cleanup is normal kernel style;
  - why sleeping in IRQ context is wrong;
  - why not to invent ABI when a subsystem exists.
- Mid-level interview should test:
  - resource unwind after partial probe failure;
  - `devm_*` limits;
  - `-EPROBE_DEFER`;
  - `poll()` / non-blocking I/O;
  - threaded IRQs and workqueues;
  - DMA ownership and sync;
  - runtime PM get/put balance.
- Senior interview should test:
  - remove while userspace holds file descriptors;
  - suspend/resume during active DMA or IRQ storms;
  - userspace ABI design and compatibility;
  - DT binding review and `dtbs_check`;
  - lock hierarchy and context proof;
  - postmortem debugging from an oops or intermittent data corruption;
  - when to reject a driver design even if it "works on my board."

## Source Differences

- There is no direct "Production Driver Checklist" chapter. The final lesson must synthesize from lifecycle, error-handling, concurrency, PM, DMA, ABI, DT, and debug chapters.
- `ldd1` often shows older or more manual resource flows: `request_mem_region()`, `ioremap()`, explicit frees, integer GPIO APIs, and old-style examples. Keep the concepts but prefer current managed/resource helpers in final learner docs where appropriate.
- `ldd2` targets Linux 4.19-era APIs. Core concepts remain valid, but final docs should validate current names/signatures for:
  - `remove()` callback return type changes in some buses;
  - PCI MSI/MSI-X APIs;
  - tracefs path preference;
  - tasklet guidance;
  - current PM helper behavior.
- Notion sometimes uses checklist language and emojis. The technical checklist is useful, but learner-facing docs should use clean repo style and avoid copying the formatting.
- Same-number chapter collision is real:
  - `ldd1-ch14` and `ldd2-ch14` are unrelated topics.
  - `ldd1-ch05`, `ldd2-ch05`, and `notion-ch05-*` are not the same content.
  - Notion Chapter 8 contains both SPI and V4L2 materials; they must not be merged by number.
- Internal sources mention old documentation paths such as `Documentation/.../*.txt`. Final docs should use kernel-version notes and validate against current `docs.kernel.org` / kernel tree paths before making current claims.

## Gaps / Uncertainties

- Internal sources do not provide a unified production checklist; this topic must be built as a capstone synthesis.
- Internal sources do not cover modern automated review tools in one place:
  - `make W=1`, `C=1` sparse, smatch, coccinelle, KASAN/KMSAN/KCSAN/UBSAN, lockdep, fault injection, kselftest, kunit, syzkaller, and subsystem compliance tools should be externally validated before final learner docs.
- Device Tree binding validation should be checked against current YAML binding workflow and `dtbs_check`.
- ABI rules should be externally validated against current kernel documentation for sysfs, ioctl, debugfs, and stable ABI policy.
- Current tasklet guidance needs validation because newer kernel code often prefers threaded IRQs, workqueues, or other mechanisms for new designs.
- The final knowledge doc should avoid retreading every previous chapter. It should link concepts into review checklists and scenarios rather than re-explaining all subsystem internals.

## External Validation

No external sources were used in this topic-brief creation step. External validation is recommended before learner-facing docs for:

- Current kernel coding style and submitting-patches process.
- Current `dev_err_probe()` guidance and driver-core probe deferral behavior.
- Current Device Tree schema/YAML binding and `dtbs_check` workflow.
- Current DMA API documentation, especially streaming mapping ownership and scatter/gather mapped entry count.
- Current runtime PM and system sleep driver API notes.
- Current debug tooling docs: dynamic debug, ftrace/tracefs, event tracing, KASAN/KCSAN/lockdep, and fault injection.
- Current ABI documentation for sysfs, ioctl, debugfs, and stable userspace interfaces.

## Learning Content Brief

### Scope

This chapter should be a final production review and interview-readiness chapter. It should not introduce a new subsystem. It should teach how to inspect any driver and ask: "Can this survive real hardware, real userspace, repeated failures, suspend/resume, concurrency, and maintenance review?"

### Mental Model

A driver becomes production-ready when every externally visible behavior has a matching ownership and teardown story:

- every acquired resource has a release path;
- every registration has an unregister path;
- every async producer is stopped before state is freed;
- every userspace ABI is stable and validated;
- every hardware access happens in a valid power/context state;
- every debug path helps diagnose issues without becoming noise or ABI debt.

### Core Review Checklist

1. Binding and match:
   - bus type is correct;
   - ID/OF/ACPI tables are present;
   - `MODULE_DEVICE_TABLE()` exists when needed;
   - `-EPROBE_DEFER` is handled intentionally.
2. Probe:
   - private state is allocated and initialized;
   - resources are acquired through framework helpers;
   - hardware is verified/configured before registration;
   - externally visible objects are registered last.
3. Error paths:
   - each failure returns the right errno;
   - cleanup runs in reverse order;
   - `devm_*` and manual cleanup are not mixed blindly;
   - PM and DMA paths are balanced on failure.
4. Remove:
   - new users are blocked first;
   - subsystem objects are unregistered;
   - IRQ/timer/work/DMA callbacks are quiesced;
   - hardware is left safe;
   - resources are freed after no callback can touch them.
5. Locking/context:
   - mutex/spinlock choice matches callback context;
   - lock order is documented by structure, not comments alone;
   - no sleep in atomic/timer/hard IRQ context;
   - waits and wakeups protect the condition correctly.
6. ABI:
   - standard subsystem ABI is preferred;
   - sysfs/ioctl/poll/mmap semantics are stable and checked;
   - debugfs is not treated as stable ABI;
   - userspace inputs are validated.
7. DT/resources:
   - binding describes hardware;
   - named clocks/regulators/GPIOs/DMA channels are correct;
   - pinctrl and wake IRQ states are covered;
   - overlays/runtime DT are not assumed unless supported.
8. DMA/memory:
   - mapping type fits workload;
   - ownership/sync/unmap rules are correct;
   - timeouts and errors reclaim safely;
   - user memory and MMIO are accessed through proper APIs.
9. Power:
   - runtime PM get/put balance is provable;
   - autosuspend is configured intentionally;
   - system suspend/resume restores state;
   - wakeup behavior is tested.
10. Debug/test:
   - useful `dev_*` logs exist;
   - dynamic debug/tracing paths are known;
   - bind/unbind, probe failure, remove, suspend/resume, stress, and fault-injection tests are planned;
   - subsystem-specific compliance tools are used where available.

### Practical Examples To Include Later

- Pseudo-code for a production probe with mixed managed and manual resources:
  - `devm_kzalloc()`;
  - parse resources;
  - enable regulators/clocks;
  - initialize locks/wait queues;
  - request threaded IRQ;
  - enable runtime PM;
  - register subsystem object last;
  - unwind PM and hardware if registration fails.
- Pseudo-code for remove:
  - unregister subsystem object;
  - disable runtime PM and resume if hardware command is needed;
  - stop IRQ/timers/work/DMA;
  - power off hardware;
  - rely on devres only after callbacks are quiesced.
- Senior debugging scenario:
  - "Probe succeeds, but unbind crashes under IRQ load."
  - Expected reasoning: visible object still accepts users or IRQ/work/timer touches freed state; inspect unregister ordering, `free_irq()`/`disable_irq()`, work cancellation, timer sync, and devres timing.

### Common Bugs / Traps

- Registering a device node before private state and hardware are fully ready.
- Assuming `devm_kzalloc()` prevents use-after-free from asynchronous callbacks.
- Returning `-EINVAL` for every probe failure instead of meaningful errno or `-EPROBE_DEFER`.
- Accessing hardware while runtime suspended.
- Forgetting PM usage-count balance on error.
- Sleeping under spinlock or in hard IRQ/timer context.
- Using mutex in tasklet/hard IRQ context.
- Forgetting `del_timer_sync()`, work cancellation, tasklet kill, or DMA termination in remove.
- Using `dma_map_sg()` return count incorrectly.
- Exposing debugfs as a supported product ABI.
- Hardcoding board addresses, GPIO polarity, or clock names in driver code.
- Breaking userspace ABI while refactoring.

### Interview Angles

- Ask the candidate to review a probe function and identify ordering bugs.
- Ask what changes when `devm_*` is used.
- Ask how to handle a remove while userspace has the device open.
- Ask how to debug `-EPROBE_DEFER` loops.
- Ask how to prove a spinlock/mutex choice is legal.
- Ask how runtime PM interacts with sysfs reads or ioctl paths.
- Ask why a driver works before suspend but fails after resume.
- Ask how DMA cache coherency can corrupt data without obvious crashes.
- Ask how to design ABI for a feature: sysfs, ioctl, netlink, subsystem ABI, debugfs, or tracepoint.
- Ask what tests must pass before sending a driver upstream or shipping it in a product.
