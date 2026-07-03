# 38 - Production Driver Checklist And Interview Review Interview Questions

Strong candidates should be able to reason across the whole driver lifecycle: probe, error paths, remove, userspace ABI, locking, DMA, power management, and debugging. The best answers do not recite APIs; they prove ownership, ordering, context, and failure behavior.

## Beginner Questions

### 1. What does "production-ready driver" mean?

**Level:** Beginner

**Question:** A driver probes successfully on your board. Is that enough to call it production-ready?

**Short Answer:** No. Production-ready means the driver survives normal operation, partial failures, remove, suspend/resume, userspace misuse, concurrency, and debugging in the field.

**Deep Explanation:** A bench demo usually tests only the success path. Production review checks every boundary where the kernel can call back into the driver: file operations, IRQs, timers, workqueues, DMA completions, PM callbacks, sysfs, and subsystem callbacks. The driver must acquire resources in a safe order, expose userspace only when ready, unwind partial probe failures, and stop asynchronous activity before freeing state.

**API / Code Anchor:** `probe()`, `remove()`, `struct device`, `devm_*`, `request_threaded_irq()`, `pm_runtime_*()`, subsystem registration APIs such as `register_netdev()`, `input_register_device()`, `iio_device_register()`.

**Production or Debugging Angle:** Test bind/unbind loops, failed probe paths, active userspace during remove, interrupt activity, suspend/resume, and debug logging. A driver that only passes "boot once and read one register" is not reviewed deeply enough.

**Common Traps:**

- Calling success-path-only code "done."
- Ignoring remove because the device is soldered down.
- Assuming `devm_*` handles asynchronous callbacks.
- Treating debugfs output as proof of ABI quality.

**Follow-up Questions:**

- What tests would you run before shipping?
- What failures appear only under repeated bind/unbind?
- Which callbacks can run after the device is visible to userspace?

### 2. What is the difference between module init and probe?

**Level:** Beginner

**Question:** Why should hardware initialization usually be in `probe()` instead of `module_init()`?

**Short Answer:** `module_init()` runs once when the module loads; `probe()` runs for each matched device. Hardware is tied to a device, so hardware setup belongs in `probe()`.

**Deep Explanation:** The Linux driver model separates driver registration from device binding. Module init usually registers a driver with a bus. The bus then matches devices and calls `probe()` for each device. A module may support multiple devices, devices may appear later, and probe may be deferred until suppliers such as clocks or regulators are ready.

**API / Code Anchor:** `module_init()`, `module_exit()`, `module_platform_driver()`, `struct platform_driver`, `.probe`, `.remove`, `MODULE_DEVICE_TABLE()`.

**Production or Debugging Angle:** If hardware access happens in module init, the driver may fail on multi-device systems, hotplug-capable buses, deferred probe, or systems where the device is disabled in firmware.

**Common Traps:**

- Reading MMIO before the platform device exists.
- Assuming one driver module means one hardware instance.
- Forgetting module autoload aliases.
- Using global state when per-device private state is needed.

**Follow-up Questions:**

- What should module init do for a simple platform driver?
- How does the kernel know which module can drive a device?
- Why can `probe()` be called more than once?

### 3. Why is `goto` commonly used for probe error handling?

**Level:** Beginner

**Question:** Kernel style often uses `goto err_*` labels in probe. Why is that considered good practice?

**Short Answer:** It keeps cleanup linear and avoids duplicated, incomplete, or incorrectly ordered cleanup code.

**Deep Explanation:** Probe often acquires resources step by step. If step five fails, steps one through four must be undone in reverse order. `goto` labels create one cleanup path per acquisition stage. This is clearer than deeply nested `if` blocks and less error-prone than repeating cleanup at every failure.

**API / Code Anchor:**

```c
ret = clk_prepare_enable(clk);
if (ret)
	goto err_disable_regulator;

ret = my_register_device(priv);
if (ret)
	goto err_disable_clk;
```

**Production or Debugging Angle:** Bad cleanup paths cause leaks, powered hardware after failed probe, double frees, and later probe failures that are difficult to connect to the original failure.

**Common Traps:**

- Returning immediately after manual resource acquisition.
- Jumping to a label that frees a resource that was never acquired.
- Forgetting PM or DMA cleanup in failure paths.
- Using `devm_*` for everything and forgetting manual hardware disable.

**Follow-up Questions:**

- How do cleanup labels relate to acquisition order?
- Which resources still need manual cleanup with `devm_*`?
- What errno should allocation failure return?

## Mid-Level Questions

### 4. What does `devm_*` solve, and what does it not solve?

**Level:** Mid

**Question:** A driver uses `devm_kzalloc()` and `devm_request_threaded_irq()`. Is its remove path automatically safe?

**Short Answer:** No. `devm_*` releases resources at device detach, but the driver still must unregister visible objects and stop hardware, work, DMA, timers, and PM activity in a safe order.

**Deep Explanation:** Managed resources are tied to device lifetime. They reduce boilerplate for memory, MMIO, IRQ, GPIO, clock, regulator, and some subsystem resources. But they do not know when your hardware has stopped generating interrupts, when userspace stops calling file operations, or when a DMA callback can no longer run. If a subsystem object remains visible while managed private memory is being released, callbacks can still race with teardown.

**API / Code Anchor:** `devm_kzalloc()`, `devm_request_threaded_irq()`, `devm_platform_ioremap_resource()`, `cancel_work_sync()`, `del_timer_sync()`, `dmaengine_terminate_sync()`, subsystem unregister calls.

**Production or Debugging Angle:** A classic crash is "unbind under IRQ load." The IRQ resource will eventually be freed, but if hardware is still interrupting or work is still queued, the callback may touch stale state.

**Common Traps:**

- Believing `devm_*` replaces remove.
- Registering userspace ABI before state is fully initialized.
- Relying on devres ordering without thinking about callback visibility.
- Mixing manual and managed cleanup without clear ownership.

**Follow-up Questions:**

- What should remove do before managed memory is released?
- When is `devm_request_threaded_irq()` still not enough?
- How do you review devres ordering?

### 5. How should a driver handle `-EPROBE_DEFER`?

**Level:** Mid

**Question:** Probe fails because a regulator or clock is not ready yet. What should the driver return?

**Short Answer:** It should return `-EPROBE_DEFER` if the supplier may appear later, usually by propagating the helper's error or using `dev_err_probe()`.

**Deep Explanation:** Deferred probe is the driver model's way to handle dependency ordering. A consumer driver may probe before its clock, regulator, GPIO controller, interrupt controller, or PHY provider is ready. This is not the same as unsupported hardware. Returning `-ENODEV` or `-EINVAL` can turn an ordering problem into a permanent failure.

**API / Code Anchor:** `devm_clk_get()`, `devm_regulator_get()`, `devm_gpiod_get()`, `platform_get_irq()`, `dev_err_probe(dev, ret, "...")`.

**Production or Debugging Angle:** Deferred probe bugs often look like missing devices after boot. Check `dmesg`, supplier drivers, firmware nodes, and whether the dependency exists and matches the binding.

**Common Traps:**

- Collapsing all probe errors to `-EINVAL`.
- Logging noisy errors for normal deferral.
- Treating optional resources as mandatory.
- Hiding the original errno.

**Follow-up Questions:**

- How do optional resources change error handling?
- Why is `dev_err_probe()` useful?
- What DT mistakes commonly cause deferred probe?

### 6. How do you choose mutex vs spinlock?

**Level:** Mid

**Question:** A driver has shared state accessed by file operations and an interrupt handler. Which lock should it use?

**Short Answer:** It depends on context. Use a mutex only in sleepable context. Use a spinlock, often with IRQ-save variants, for state shared with hard IRQ context. Move slow work to a threaded IRQ or workqueue when a mutex or sleeping I/O is needed.

**Deep Explanation:** Mutexes may sleep, so they are illegal in hard IRQ, timer, tasklet, and other atomic contexts. Spinlocks do not sleep and are suitable for short critical sections. If the interrupt handler needs I2C/SPI access, allocation with `GFP_KERNEL`, or other sleepable work, use a threaded IRQ or schedule work.

**API / Code Anchor:** `struct mutex`, `spinlock_t`, `spin_lock_irqsave()`, `request_threaded_irq()`, `IRQF_ONESHOT`, `schedule_work()`.

**Production or Debugging Angle:** The warning `sleeping function called from invalid context` points to this class of bug. Lockdep can also find inversion and invalid locking patterns.

**Common Traps:**

- Taking a mutex in a hard IRQ or timer callback.
- Sleeping while holding a spinlock.
- Using `spin_lock_irq()` when interrupt state must be preserved.
- Holding a spinlock around slow bus I/O.

**Follow-up Questions:**

- Why does `spin_lock_irqsave()` take a flags variable?
- What can run in a threaded IRQ handler?
- How do you debug a deadlock in remove?

### 7. What must a correct `poll()` implementation do?

**Level:** Mid

**Question:** Userspace reports that `poll()` wakes continuously even though no data is available. What should you inspect?

**Short Answer:** Inspect whether the driver's poll mask matches the protected readiness condition and whether wakeups happen only after state changes.

**Deep Explanation:** `poll()` should register wait queues with `poll_wait()`, then check current state and return readiness bits such as `POLLIN` or `POLLOUT`. It should not return readable unless a following non-blocking `read()` can make progress. State updates and wakeups must be protected consistently, or userspace may busy-loop.

**API / Code Anchor:** `poll_wait()`, `wait_queue_head_t`, `wake_up_interruptible()`, `POLLIN`, `POLLOUT`, `O_NONBLOCK`, `-EAGAIN`.

**Production or Debugging Angle:** Bad poll behavior creates CPU load and makes applications fragile. Test blocking and non-blocking reads, poll timeout, concurrent writer/reader, and device removal while polling.

**Common Traps:**

- Returning readiness unconditionally.
- Waking before updating the condition.
- Not holding the lock while checking shared state.
- Returning `0` from non-blocking read instead of `-EAGAIN` when no data exists.

**Follow-up Questions:**

- What should `read()` do after `poll()` says readable?
- How should remove wake blocked readers?
- Why is busy-waiting in userspace a driver smell?

## Senior Questions

### 8. How do you make remove safe while userspace still has the device open?

**Level:** Senior

**Question:** A char-like driver can be unbound while a process still has `/dev/mydev` open. What must the driver prove?

**Short Answer:** It must prevent new users, detach from hardware safely, keep open-file state valid until last close, and ensure callbacks do not touch freed memory.

**Deep Explanation:** Removing a device is not the same as closing all file descriptors. Userspace may keep a file open after the device is unregistered. The driver must separate device visibility from object lifetime. It may need reference counts, a `dying` flag, wait queue wakeups, and per-open state that remains valid until `.release()`. Hardware-backed operations after remove should fail cleanly, usually with `-ENODEV` or `-EIO`, not crash.

**API / Code Anchor:** `cdev_del()`, `device_destroy()`, `.open`, `.release`, `kref`, `refcount_t`, `get_device()`, `put_device()`, wait queues, `wake_up_all()`, `cancel_work_sync()`.

**Production or Debugging Angle:** Test by opening the device, unbinding the driver, then calling read/write/ioctl/close from userspace. Watch for use-after-free with KASAN and for blocked processes that never wake.

**Common Traps:**

- Freeing private state directly in remove while file operations still reference it.
- Forgetting to wake blocking reads during remove.
- Letting ioctl access registers after hardware is powered off.
- Assuming `rmmod` will fail just because userspace has the device open.

**Follow-up Questions:**

- Where should the final free happen?
- What should blocked readers observe during remove?
- How would this differ for `net_device` or `video_device`?

### 9. How do you review DMA correctness?

**Level:** Senior

**Question:** A driver receives correct DMA data on one board but corrupt data on another. What review path do you follow?

**Short Answer:** Check DMA mask, mapping type, direction, cache ownership, sync/unmap rules, scatter-gather mapped count, timeout paths, and whether the CPU accesses buffers while the device owns them.

**Deep Explanation:** DMA correctness depends on the DMA API contract. Coherent memory can be shared more simply but is expensive and usually long-lived. Streaming mappings require ownership transfer. After `dma_map_*()`, the device owns the buffer for the mapped direction. The CPU must not read/write it until ownership is returned by sync or unmap. Non-coherent architectures expose these bugs more often than coherent desktops.

**API / Code Anchor:** `dma_set_mask_and_coherent()`, `dma_alloc_coherent()`, `dma_map_single()`, `dma_unmap_single()`, `dma_map_sg()`, `dma_sync_single_for_cpu()`, `dma_sync_single_for_device()`, `dmaengine_terminate_sync()`.

**Production or Debugging Angle:** Silent corruption is worse than a crash. Reproduce with stress, different cacheline alignment, non-coherent hardware, and DMA debug options when available. Inspect whether error and timeout paths unmap and terminate correctly.

**Common Traps:**

- Always using `DMA_BIDIRECTIONAL`.
- Programming hardware with the original SG count instead of the mapped count.
- Reading a streaming RX buffer before unmap/sync.
- Freeing a DMA buffer while hardware can still write to it.

**Follow-up Questions:**

- When would you choose coherent memory?
- What does `dma_sync_single_for_cpu()` mean?
- How do suspend/remove paths interact with active DMA?

### 10. Why can suspend/resume break a driver that works after boot?

**Level:** Senior

**Question:** A sensor works after boot but returns `-EIO` after system resume. How do you debug and fix it?

**Short Answer:** Check whether power, clocks, regulators, pinctrl state, interrupts, and device registers are restored in the correct order, and whether runtime PM state matches physical hardware state.

**Deep Explanation:** Suspend may turn off supplies, gate clocks, switch pinctrl states, mask interrupts, reset hardware, or lose register state. Resume must rebuild the hardware state before userspace or subsystem callbacks perform I/O. Runtime PM can add another layer: the device may be logically active or suspended from the PM core's perspective, which must match real hardware.

**API / Code Anchor:** `struct dev_pm_ops`, `.suspend`, `.resume`, `.runtime_suspend`, `.runtime_resume`, `pm_runtime_get_sync()`, `pm_runtime_put_autosuspend()`, `pinctrl_pm_select_default_state()`, `enable_irq_wake()`.

**Production or Debugging Angle:** Run suspend/resume loops while userspace is active. Compare register dumps before suspend and after resume. Use dynamic debug around PM callbacks and check whether wake IRQs fire unexpectedly.

**Common Traps:**

- Restoring registers before enabling clocks.
- Forgetting pinctrl default state on resume.
- Accessing hardware while runtime suspended.
- Balancing PM gets/puts on success path but not error path.

**Follow-up Questions:**

- How does autosuspend affect sysfs reads?
- What must happen before disabling runtime PM in remove?
- How do wakeup-capable IRQs affect suspend?

### 11. How do you decide between sysfs, ioctl, debugfs, tracepoints, and subsystem ABI?

**Level:** Senior

**Question:** A product needs a new control and debug visibility from userspace. Which interface do you choose?

**Short Answer:** Prefer an existing subsystem ABI. Use sysfs for simple stable attributes, ioctl only when structured device-specific commands are justified, tracepoints for observability, and debugfs only for debugging with no stable ABI promise.

**Deep Explanation:** ABI choice is a long-term maintenance decision. Standard subsystem ABIs let existing tools work and reduce custom code. Sysfs should expose simple values and control knobs, not complex protocols. Ioctl can handle structured operations but requires careful command numbering, size validation, compatibility thinking, and documentation. Debugfs is convenient but unstable. Tracepoints are good for observability without creating control ABI.

**API / Code Anchor:** `DEVICE_ATTR_*`, attribute groups, `.unlocked_ioctl`, `.compat_ioctl`, `copy_from_user()`, `copy_to_user()`, debugfs helpers, trace events, subsystem registration APIs.

**Production or Debugging Angle:** Once userspace depends on an ABI, changing it can break products. Review permissions, input validation, compatibility, documentation, and whether debug-only information might expose secrets.

**Common Traps:**

- Creating private ABI when a subsystem already provides one.
- Treating debugfs as a supported product interface.
- Using binary sysfs blobs for complex protocols.
- Not validating ioctl sizes or pointers.

**Follow-up Questions:**

- What belongs in sysfs?
- When is ioctl acceptable?
- How do you support 32-bit userspace on a 64-bit kernel?

### 12. How would you review a probe function for ordering bugs?

**Level:** Senior

**Question:** You are handed a probe function. What order do you inspect first?

**Short Answer:** Find all resource acquisitions, all externally visible registrations, all async activity, and all manual enables. Then verify success, failure, remove, and PM paths undo them safely in the right order.

**Deep Explanation:** Probe review is less about reading top to bottom and more about building an ownership map. Mark when memory is allocated, drvdata is set, MMIO is mapped, power is enabled, IRQs are requested, work/timers are initialized, DMA starts, and userspace/subsystem visibility begins. Then walk each failure label and remove path backward.

**API / Code Anchor:** `devm_kzalloc()`, `platform_set_drvdata()`, `clk_prepare_enable()`, `regulator_enable()`, `request_threaded_irq()`, `pm_runtime_enable()`, `register_*()`, `goto err_*`.

**Production or Debugging Angle:** The most dangerous ordering bug is exposing a device before it is ready or freeing state before every caller is gone. Both may pass simple boot testing.

**Common Traps:**

- Requesting IRQ before state and locks are initialized.
- Registering userspace ABI before hardware is ready.
- Enabling runtime PM before initial state is coherent.
- Missing cleanup for resources enabled after the last `devm_*` call.

**Follow-up Questions:**

- What should be registered last?
- What should be stopped first in remove?
- How do you review a driver that uses both managed and unmanaged resources?

### 13. Debugging scenario: unbind crashes under IRQ load

**Level:** Senior

**Question:** A driver probes and works. During `echo <dev> > /sys/bus/.../drivers/.../unbind` while interrupts are firing, the kernel oopses in the IRQ thread. What is your likely diagnosis?

**Short Answer:** Remove is racing with an IRQ or IRQ-thread path that still references driver state after it has been unregistered, powered down, or freed.

**Deep Explanation:** Unbind triggers remove, but IRQs can still arrive until the hardware source is disabled and the IRQ is freed or synchronized. A threaded IRQ can also be running while remove tears down state. Work scheduled by the IRQ can outlive both. The fix is to order teardown: unregister visible users, stop hardware interrupt generation, synchronize IRQ/thread/work, then release resources.

**API / Code Anchor:** `disable_irq()`, `free_irq()`, `devm_request_threaded_irq()`, `synchronize_irq()`, `cancel_work_sync()`, `IRQF_ONESHOT`, hardware interrupt mask register.

**Production or Debugging Angle:** Use ftrace function graph around remove and IRQ handler, inspect `/proc/interrupts`, add dynamic debug around interrupt mask/unmask, and reproduce with KASAN/lockdep enabled.

**Common Traps:**

- Assuming devm frees the IRQ early enough for driver-specific ordering.
- Disabling clocks before masking device interrupts.
- Canceling work before preventing new work from being queued.
- Forgetting level-triggered IRQs can refire until the device cause is cleared.

**Follow-up Questions:**

- Where should hardware IRQ masking happen?
- Is `cancel_work_sync()` enough if IRQ can queue more work?
- How does a threaded IRQ differ from a workqueue in teardown?

### 14. Debugging scenario: intermittent `-EPROBE_DEFER` never resolves

**Level:** Senior

**Question:** A driver repeatedly defers probe and the device never appears. How do you approach it?

**Short Answer:** Identify which supplier is missing, verify the firmware binding and provider driver, and make sure the consumer is not treating an optional resource as mandatory.

**Deep Explanation:** Deferral is normal only if a supplier may appear later. If it never resolves, either the supplier driver is missing or failed, the DT/ACPI reference is wrong, the property name does not match the binding, or the consumer requested the wrong resource. A good probe path preserves the original error and logs the resource name.

**API / Code Anchor:** `dev_err_probe()`, `devm_clk_get()`, `devm_regulator_get()`, `devm_gpiod_get()`, `platform_get_irq()`, `of_property_read_*()`.

**Production or Debugging Angle:** Inspect `dmesg`, firmware nodes, provider driver probe logs, clock/regulator/GPIO summaries, and module autoload. Confirm that the supplier node is enabled and compatible strings match.

**Common Traps:**

- Returning deferral for a truly optional property.
- Using the wrong `*-names` string.
- Forgetting `MODULE_DEVICE_TABLE()` for the supplier.
- Hiding the resource name in logs.

**Follow-up Questions:**

- How would you distinguish missing hardware from missing supplier?
- What should optional `devm_*_get_optional()` paths do?
- Why does deferred probe make probe ordering less deterministic?

### 15. What makes a senior driver review different from a checklist pass?

**Level:** Senior

**Question:** If two engineers both have the same checklist, what distinguishes the senior review?

**Short Answer:** A senior review proves interactions and tradeoffs, not just item presence. It asks whether the design remains correct under concurrency, failure, maintenance, ABI stability, and future hardware variation.

**Deep Explanation:** Checklists catch omissions. Senior review catches bad assumptions: "this IRQ cannot fire during remove," "userspace will not keep this open," "this register never resets," "debugfs is fine for production," "DMA is coherent on my board," or "suspend will not happen while streaming." Senior engineers challenge those assumptions and design the driver so the kernel mechanism enforces correctness.

**API / Code Anchor:** Cross-cutting: `struct device`, subsystem registration/unregistration, `kref`/`refcount_t`, PM ops, DMA API, lockdep, KASAN/KCSAN, tracepoints.

**Production or Debugging Angle:** Senior reviews often produce test requirements: fault injection, bind/unbind loops, suspend/resume stress, active userspace teardown, DMA stress, and ABI compatibility checks.

**Common Traps:**

- Treating "works on my board" as evidence.
- Reviewing style but not lifetime.
- Ignoring userspace ABI because it is "internal."
- Shipping without a debug strategy.

**Follow-up Questions:**

- What assumption in this driver would you try to break first?
- Which bug would only show up on another SoC?
- What would make you reject the design even if the code compiles?
