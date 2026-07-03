# 38 - Production Driver Checklist And Interview Review

## Learning Goal

By the end of this chapter, you should be able to review a Linux device driver like an engineer preparing it for real hardware, real userspace, and real maintenance.

You should be able to:

- Explain what "production-ready" means for a driver.
- Walk through probe, error paths, remove, suspend/resume, and runtime PM.
- Identify lifetime, locking, DMA, ABI, and Device Tree risks.
- Debug common bring-up and field failures.
- Answer senior interview questions by reasoning from kernel mechanisms, not memorized slogans.

## Why This Matters In Real Work

A driver can "work once" on a bench and still be unsafe for production. Production bugs usually appear at boundaries: partial probe failure, unbind while userspace is active, suspend/resume, IRQ storms, DMA timeout, missing supplier, or a userspace ABI that cannot be changed later.

In real driver work, review is not just style checking. You are asking:

- Can this driver bind only to hardware it supports?
- Does every acquired resource have a safe release path?
- Can callbacks run after private state is freed?
- Are user-visible interfaces stable and validated?
- Does the driver obey sleep/atomic context rules?
- Does it survive repeated failures, bind/unbind, suspend/resume, and stress?
- Can another engineer debug it six months later?

This final topic ties together the earlier chapters: platform probe/remove, Device Tree, device model, memory, DMA, IRQs, power management, userspace ABI, and debugging.

## Mental Model

Think of a driver as a set of doors into shared state. Probe opens those doors; remove must close them in the right order. Production review is the discipline of finding every door and proving that nothing can enter after the room is being dismantled.

The important doors are:

- bus binding and probe;
- userspace file operations, sysfs attributes, ioctl, poll, mmap;
- subsystem callbacks such as input, IIO, netdev, V4L2, ASoC, RTC, watchdog, PWM;
- IRQ handlers and threaded IRQs;
- timers, workqueues, tasklets, completions, wait queues;
- DMA completion callbacks;
- runtime PM and system sleep callbacks;
- debugfs, tracepoints, and debug controls.

**Production rule:** if code can be called asynchronously, it has a lifetime and teardown story.

## Core Concepts

This chapter is a checklist, but the checklist only works if you understand the mechanism behind each item.

| Area | What To Prove | Typical Failure |
| --- | --- | --- |
| Binding | The driver matches the right device and handles missing suppliers. | Probe loop, wrong hardware, no autoload. |
| Probe | Resources are acquired before the device is exposed. | Userspace calls into half-initialized state. |
| Error path | Partial setup unwinds in reverse order. | Leak, double free, powered hardware after failed probe. |
| Remove | New users are blocked and async callbacks are stopped before memory is freed. | Use-after-free during unbind. |
| Lifetime | Ownership of each object is clear. | Sysfs, IRQ, timer, or DMA callback touches stale data. |
| Locking | Lock type matches execution context. | Sleeping in atomic context, deadlock, data race. |
| ABI | Userspace interface is stable, validated, and appropriate. | Unfixable ioctl/sysfs mistake after release. |
| Device Tree | DT describes hardware, not driver policy. | Wrong polarity, missing clock, bad interrupt, deferred probe. |
| DMA | CPU/device ownership and cache sync are correct. | Silent corruption, stale data, random timeout. |
| Power | Hardware access happens only while powered and clocked. | Works before suspend, fails after resume. |
| Debug | Failures leave enough evidence. | Field bug cannot be diagnosed. |

### Probe vs Module Init

`module_init()` runs once when a module is loaded. `probe()` runs once per matched device.

That distinction matters:

- A module can support multiple devices.
- A device may appear after the module is loaded.
- Probe can be deferred and retried.
- Remove can happen per device while the module remains loaded.

**Interview trap:** putting per-device hardware access in module init usually means the driver does not fit the Linux driver model.

### Managed Resources vs Manual Teardown

`devm_*` helpers free resources when the device is detached, but they do not stop the world.

Use `devm_*` for:

- memory tied to device lifetime: `devm_kzalloc()`;
- MMIO mapping: `devm_platform_ioremap_resource()` or `devm_ioremap_resource()`;
- IRQ request: `devm_request_threaded_irq()`;
- GPIOs, clocks, regulators, resets, pinctrl, and many subsystem registration helpers where available.

Still handle manually:

- unregistering externally visible objects in the correct order;
- stopping DMA, timers, workqueues, tasklets, and hardware activity;
- balancing runtime PM usage counts;
- disabling interrupts or hardware sources before state can disappear;
- making remove safe while userspace still holds file descriptors.

**Production rule:** `devm_*` helps cleanup; it does not prove lifetime safety.

## Kernel Mechanism

The kernel driver core binds devices and drivers through bus-specific match logic. Once a match is found, the bus calls the driver's `probe()` callback. The driver's job is to turn a hardware description into a registered kernel object that a subsystem or userspace can use.

The usual object relationships are:

```text
firmware / bus description
        |
        v
struct device / bus object
        |
        v
driver probe()
        |
        +--> private state
        +--> resources: MMIO, IRQ, DMA, clocks, regulators, GPIOs
        +--> kernel objects: cdev, net_device, input_dev, iio_dev, video_device, ...
        +--> userspace ABI: /dev, sysfs, subsystem nodes
        +--> async activity: IRQ, work, timers, DMA callbacks, PM
```

After a subsystem registration succeeds, callbacks may happen immediately. For example:

- `open()` may run after `device_create()` or subsystem registration.
- An IRQ may fire after `request_irq()`.
- A sysfs `show()` callback may run after attributes are created.
- Runtime PM may suspend the device after the usage count reaches zero.
- A DMA completion may run after the transaction is submitted.

That is why registration order matters. Expose the device only when state is ready. Tear it down by removing visibility first, then stopping asynchronous producers, then freeing resources.

## Key Structs And APIs

These APIs are not a memorization list. They are landmarks for review.

### Binding And Driver Model

| Struct / API | Why It Matters |
| --- | --- |
| `struct device` | The core object for lifetime, parent/child relation, PM state, firmware node, and driver data. |
| `struct device_driver` | Holds driver callbacks, match tables, PM ops, and attribute groups. |
| `struct platform_driver`, `struct i2c_driver`, `struct spi_driver`, `struct pci_driver` | Bus-specific driver objects with probe/remove callbacks. |
| `module_platform_driver()`, `module_i2c_driver()`, `module_spi_driver()`, `module_pci_driver()` | Register/unregister boilerplate for simple modules. |
| `MODULE_DEVICE_TABLE()` | Exposes match aliases for module autoloading. |
| `dev_set_drvdata()`, `dev_get_drvdata()`, `platform_set_drvdata()` | Connect callbacks back to private state. |

### Resources

| API | Review Question |
| --- | --- |
| `platform_get_resource()` | Does the driver get MMIO resources from firmware instead of hardcoding? |
| `platform_get_irq()` | Are IRQ errors handled, including deferral where relevant? |
| `devm_platform_ioremap_resource()` | Are MMIO resources requested and mapped safely? |
| `devm_clk_get()`, `clk_prepare_enable()`, `clk_disable_unprepare()` | Are clocks acquired and enabled in the right order? |
| `devm_regulator_get()`, `regulator_enable()`, `regulator_disable()` | Are supplies optional/mandatory as binding says? |
| `devm_gpiod_get()` | Is GPIO polarity handled through descriptors and DT flags? |
| `devm_pinctrl_get_select_default()` | Are default and sleep pin states considered? |

### Error Handling

| API / Pattern | Review Question |
| --- | --- |
| `return -ENOMEM`, `-EINVAL`, `-ENODEV`, `-EBUSY`, `-EIO` | Does the error code describe the real failure? |
| `-EPROBE_DEFER` | Is a missing supplier handled as "try later" instead of permanent failure? |
| `ERR_PTR()`, `IS_ERR()`, `PTR_ERR()` | Are pointer-returning helpers checked correctly? |
| `dev_err_probe()` | Are probe errors, especially deferrals, logged consistently for modern kernels? |
| `goto err_*` cleanup labels | Does partial setup unwind in reverse order? |

### Locking And Async Context

| API / Object | Use |
| --- | --- |
| `struct mutex` | Sleepable mutual exclusion in process/thread context. |
| `spinlock_t` | Atomic/SMP/IRQ-safe protection for short critical sections. |
| `spin_lock_irqsave()` / `spin_unlock_irqrestore()` | Protect data shared with hard IRQ while preserving interrupt state. |
| `request_threaded_irq()` | Move slow work, such as I2C/SPI access, into thread context. |
| `IRQF_ONESHOT` | Keep an interrupt masked while its threaded handler runs. |
| `struct work_struct` | Defer sleepable work to process context. |
| `struct timer_list`, `hrtimer` | Timer callbacks; do not sleep in callbacks. |
| `del_timer_sync()`, `hrtimer_cancel()`, `cancel_work_sync()` | Make teardown wait until callbacks cannot run. |
| `wait_queue_head_t`, `poll_wait()` | Blocking I/O and poll readiness. |
| `struct completion` | Wait for one asynchronous operation to complete. |

### DMA And Memory

| API | Review Question |
| --- | --- |
| `dma_set_mask_and_coherent()` | Can the device address the memory the kernel gives it? |
| `dma_alloc_coherent()` / `dma_free_coherent()` | Is long-lived coherent DMA memory used intentionally? |
| `dma_map_single()` / `dma_unmap_single()` | Is streaming DMA ownership returned to the CPU correctly? |
| `dma_map_sg()` / `dma_unmap_sg()` | Does the driver use the mapped entry count, not the original SG count? |
| `dma_sync_single_for_cpu()` / `dma_sync_single_for_device()` | Is ownership switched safely before unmap? |
| `copy_from_user()` / `copy_to_user()` | Are userspace pointers accessed safely? |
| `readl()` / `writel()` | Is MMIO accessed through accessors, not normal pointer dereference? |

### Power Management

| API / Object | Review Question |
| --- | --- |
| `struct dev_pm_ops` | Are runtime PM and system sleep callbacks registered? |
| `pm_runtime_enable()` / `pm_runtime_disable()` | Is runtime PM state initialized and torn down coherently? |
| `pm_runtime_get_sync()` / `pm_runtime_put()` | Is hardware resumed before access and released after access? |
| `pm_runtime_mark_last_busy()` / `pm_runtime_put_autosuspend()` | Is autosuspend delay used correctly? |
| `device_init_wakeup()`, IRQ wake helpers | Can the device wake the system when required? |

### Debugging

| Tool / API | Use |
| --- | --- |
| `dev_err()`, `dev_warn()`, `dev_info()`, `dev_dbg()` | Device-scoped logs. |
| `dmesg -w` | Live kernel log during probe/remove. |
| Dynamic debug | Enable `dev_dbg()` / `pr_debug()` callsites selectively. |
| ftrace / function graph | Trace call flow and latency. |
| Tracepoints / events | Observe subsystem-defined behavior. |
| debugfs / sysfs / procfs | Inspect state, but know which interfaces are stable ABI. |
| `/proc/interrupts` | Verify IRQ firing and interrupt sharing. |
| lockdep, KASAN, KCSAN, UBSAN | Find locking, memory, race, and undefined-behavior bugs when configured. |

## Lifecycle / Data Flow

The most useful review technique is to draw the lifecycle and walk it forward and backward.

### Normal Probe Flow

```text
driver registered
  -> bus match succeeds
  -> probe()
     -> allocate private state
     -> save drvdata
     -> parse DT / ACPI / resources
     -> map MMIO
     -> acquire clocks, regulators, resets, GPIOs, pinctrl
     -> initialize locks, wait queues, completions, timers, work
     -> enable hardware enough to identify it
     -> request IRQ / DMA resources
     -> enable runtime PM if used
     -> register subsystem object or userspace ABI
     -> return 0
```

**Registration should be near the end** because it makes callbacks possible.

### Error Flow

```text
probe()
  -> step 1 ok
  -> step 2 ok
  -> step 3 fails
  -> undo step 2
  -> undo step 1
  -> return correct negative errno
```

With `devm_*`, some release calls are automatic, but ordering-sensitive actions still need explicit cleanup.

### Remove Flow

```text
remove()
  -> make device invisible to new users
  -> unregister subsystem object / remove ABI
  -> stop hardware from generating new events
  -> synchronize IRQ, timer, work, DMA callbacks
  -> disable runtime PM safely
  -> put hardware into safe state
  -> release manual resources
  -> devres releases managed resources
```

**Remove is not just "free what probe allocated."** It must also prove that nothing can still call into freed state.

### Suspend / Resume Flow

```text
runtime access path
  -> pm_runtime_get_sync()
  -> access hardware
  -> pm_runtime_mark_last_busy()
  -> pm_runtime_put_autosuspend()

system suspend
  -> block or drain active I/O
  -> stop DMA/IRQ activity as needed
  -> save device state
  -> select sleep pinctrl state
  -> disable clocks/regulators as appropriate

system resume
  -> enable supplies/clocks
  -> restore registers
  -> re-enable IRQ/DMA state
  -> allow userspace/subsystem I/O again
```

## Minimal Practical Example

This is **learning-only pseudo-code**. It shows reviewable structure, not a complete driver.

```c
struct mydev {
	struct device *dev;
	void __iomem *regs;
	struct clk *clk;
	struct regulator *vdd;
	int irq;
	struct mutex lock;
	struct work_struct work;
	bool registered;
};

static int mydev_probe(struct platform_device *pdev)
{
	struct mydev *priv;
	int ret;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = &pdev->dev;
	mutex_init(&priv->lock);
	INIT_WORK(&priv->work, mydev_work);
	platform_set_drvdata(pdev, priv);

	priv->regs = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(priv->regs))
		return PTR_ERR(priv->regs);

	priv->vdd = devm_regulator_get(&pdev->dev, "vdd");
	if (IS_ERR(priv->vdd))
		return dev_err_probe(&pdev->dev, PTR_ERR(priv->vdd),
				     "failed to get vdd\n");

	priv->clk = devm_clk_get(&pdev->dev, "core");
	if (IS_ERR(priv->clk))
		return dev_err_probe(&pdev->dev, PTR_ERR(priv->clk),
				     "failed to get core clock\n");

	ret = regulator_enable(priv->vdd);
	if (ret)
		return ret;

	ret = clk_prepare_enable(priv->clk);
	if (ret)
		goto err_disable_vdd;

	ret = mydev_hw_init(priv);
	if (ret)
		goto err_disable_clk;

	priv->irq = platform_get_irq(pdev, 0);
	if (priv->irq < 0) {
		ret = priv->irq;
		goto err_disable_hw;
	}

	ret = devm_request_threaded_irq(&pdev->dev, priv->irq, NULL,
					mydev_irq_thread,
					IRQF_ONESHOT,
					dev_name(&pdev->dev), priv);
	if (ret)
		goto err_disable_hw;

	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	ret = mydev_register_with_subsystem(priv);
	if (ret)
		goto err_disable_pm;

	priv->registered = true;
	return 0;

err_disable_pm:
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
err_disable_hw:
	mydev_hw_stop(priv);
err_disable_clk:
	clk_disable_unprepare(priv->clk);
err_disable_vdd:
	regulator_disable(priv->vdd);
	return ret;
}

static void mydev_remove(struct platform_device *pdev)
{
	struct mydev *priv = platform_get_drvdata(pdev);

	if (priv->registered)
		mydev_unregister_from_subsystem(priv);

	cancel_work_sync(&priv->work);

	pm_runtime_get_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	mydev_hw_stop(priv);
	clk_disable_unprepare(priv->clk);
	regulator_disable(priv->vdd);
}
```

Important review points:

- Private state exists before any callback can use it.
- Hardware is powered before register access.
- Subsystem registration happens last.
- Error labels unwind manually enabled hardware resources.
- Remove unregisters visibility before canceling work and powering down.
- `devm_request_threaded_irq()` frees the IRQ later, but remove still stops hardware and synchronizes work before state becomes invalid.

## Common Bugs And Debugging

Start debugging from the symptom, then map it to lifecycle.

| Symptom | Likely Cause | Evidence | Fix Pattern |
| --- | --- | --- | --- |
| Probe returns `-EINVAL` or `-ENODEV` on a valid board | Bad DT property, wrong `compatible`, missing resource, wrong bus. | `dmesg -w`, `/proc/device-tree`, `of_*` parse logs. | Validate binding, use named resources, return meaningful errno. |
| Probe loops with deferral | Supplier clock/regulator/GPIO/IRQ provider not ready. | `dev_err_probe()` messages, deferred probe debug. | Return `-EPROBE_DEFER`; fix supplier driver or DT dependency. |
| Crash during unbind | IRQ, timer, work, DMA, sysfs, or file op uses freed private data. | Oops stack points into callback after remove. | Unregister first; cancel/synchronize async work before free. |
| Works until suspend/resume | Hardware state, clocks, regulators, pinctrl, or IRQ wake not restored. | Compare register state before/after resume, PM logs. | Implement PM callbacks; restore state in correct order. |
| Random DMA data corruption | Missing sync/unmap, wrong DMA direction, bad SG count, cache ownership bug. | Corruption under load, no obvious oops. | Audit DMA map/sync/unmap and timeout paths. |
| `sleeping function called from invalid context` | Mutex, I2C/SPI access, or `GFP_KERNEL` allocation in atomic context. | Kernel warning names caller and context. | Move work to threaded IRQ/workqueue; use spinlock only for short atomic state. |
| `poll()` wakes repeatedly with no data | Readiness mask does not match protected condition. | Userspace sees busy loop. | Protect condition state; wake after state change; return correct mask. |
| Field logs too noisy | Hot-path `dev_info()` or unconditional debug prints. | Huge dmesg volume, lost useful logs. | Use `dev_dbg()`, dynamic debug, tracepoints, rate limiting. |

Useful commands and checks:

```bash
dmesg -w
cat /proc/interrupts
cat /proc/iomem
find /sys/kernel/debug -maxdepth 2 -type f 2>/dev/null
cat /sys/kernel/debug/gpio 2>/dev/null
cat /sys/kernel/debug/clk/clk_summary 2>/dev/null
cat /sys/kernel/debug/regulator/regulator_summary 2>/dev/null
```

For deeper debugging:

- enable dynamic debug for the driver or subsystem;
- use ftrace `function_graph` with filters around probe/remove/PM callbacks;
- use subsystem trace events when available;
- build with lockdep, KASAN, KCSAN, UBSAN, or fault injection in a debug kernel;
- test bind/unbind, repeated suspend/resume, active userspace close/remove, and error injection.

## Production Checklist

Use this before sending code for review or shipping a board support package.

### 1. Binding And Match

- Driver is on the correct bus.
- Match table covers supported hardware only.
- `MODULE_DEVICE_TABLE()` exists for module autoloading.
- DT/ACPI properties match documented binding.
- Optional properties have sane defaults.
- Supplier absence is handled with `-EPROBE_DEFER` when appropriate.

### 2. Probe

- Private state is zero-initialized and saved in driver data.
- MMIO, IRQ, DMA, clocks, regulators, resets, GPIOs, and pinctrl are acquired through proper helpers.
- Hardware is powered before register access.
- Locks, wait queues, completions, work, and timers are initialized before use.
- Subsystem registration or userspace visibility happens last.
- Probe does not assume only one device unless the hardware really is singleton.

### 3. Error Paths

- Every failure returns a meaningful negative errno.
- Error labels unwind in reverse order.
- Manual enables are manually disabled.
- PM usage count is balanced.
- DMA mappings and channels are released or terminated.
- Logs are useful but not spammy.

### 4. Remove

- New users are blocked by unregistering visible objects first.
- IRQs, timers, workqueues, tasklets, and DMA callbacks are stopped or synchronized.
- Runtime PM is disabled only after users and async callbacks are quiesced.
- Hardware is left in a safe state.
- Remove is safe while userspace still has file descriptors if the ABI allows that.

### 5. Locking And Lifetime

- Every shared field has a protection rule.
- Mutexes are not used in hard IRQ/timer/tasklet context.
- Spinlocked regions are short and do not sleep.
- Lock ordering is consistent.
- Wait conditions are checked under the right lock.
- Reference-counted or externally visible objects have correct release paths.

### 6. Userspace ABI

- Standard subsystem ABI is used when possible.
- Private ioctl/sysfs ABI is justified and documented.
- Input sizes, offsets, indexes, flags, and commands are validated.
- `copy_from_user()` / `copy_to_user()` return values are handled.
- Blocking, non-blocking, and poll behavior are correct.
- Debugfs is not treated as product ABI.

### 7. Device Tree And Resources

- DT describes hardware wiring, not Linux implementation policy.
- `reg` matches parent address/size cells.
- Interrupt type and parent are correct.
- GPIO polarity uses descriptor flags.
- `clock-names`, `reset-names`, `dma-names`, and supply names match binding.
- Pinctrl default and sleep states are considered.
- Binding validation is part of the test plan.

### 8. DMA And Memory

- DMA mask is set where needed.
- Coherent vs streaming mapping choice is justified.
- DMA direction is exact.
- Streaming buffers are not accessed by CPU while device owns them.
- Sync calls are used when ownership changes before unmap.
- SG mapped entry count is used correctly.
- Timeout/remove paths stop hardware before freeing buffers.

### 9. Power Management

- Runtime PM initial state is coherent.
- Every `pm_runtime_get*()` has a matching put path.
- Autosuspend uses `pm_runtime_mark_last_busy()`.
- PM callbacks save and restore hardware state.
- Wakeup IRQ behavior is defined and tested.
- Active I/O during suspend is handled.

### 10. Debug And Test

- Probe/remove logs identify the device.
- Error logs include enough resource context.
- Dynamic debug or tracepoints can expose normal debug flow.
- Tests include:
  - load/unload or bind/unbind loop;
  - failed probe path if possible;
  - userspace open/read/write/ioctl/poll while removing;
  - interrupt storm or no-interrupt case;
  - DMA timeout or fault case;
  - suspend/resume loop;
  - subsystem compliance tools where available.

## Interview Readiness

For interviews, practice explaining the reason behind each checklist item.

You should be able to answer:

- Why is probe per-device but module init per-module?
- Why is `goto` cleanup normal in kernel code?
- What does `devm_*` solve, and what does it not solve?
- Why register a subsystem object near the end of probe?
- What can still run after remove starts?
- How do you choose mutex vs spinlock?
- Why is `-EPROBE_DEFER` different from `-ENODEV`?
- Why can DMA corrupt data without crashing?
- Why is debugfs not a stable ABI?
- How would you debug a driver that fails only after suspend/resume?

See `interview/38-production-driver-checklist-and-interview-review.md` for scenario-based questions.

## Kernel Version Notes

Several details in production review are kernel-version sensitive. Always check the target kernel tree when turning this checklist into patches.

- Some bus `.remove` callbacks changed from returning `int` to `void` in newer kernels.
- `dev_err_probe()` is preferred in many modern probe paths, especially around deferred probe.
- Device Tree bindings are now commonly YAML-schema based; validate with the target kernel's binding workflow.
- Tracefs is commonly mounted at `/sys/kernel/tracing`; older material often uses `/sys/kernel/debug/tracing`.
- Tasklets still exist in many kernels, but new designs often prefer threaded IRQs, workqueues, hrtimers, or subsystem-specific mechanisms.
- PCI MSI/MSI-X helper APIs and subsystem registration helpers have evolved; use the APIs present in your target kernel.
