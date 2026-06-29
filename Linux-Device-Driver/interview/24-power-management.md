# 24 - Power Management Interview Questions

Strong candidates can reason about power state, ordering, and ownership. They do not just recite callback names; they can explain what happens when userspace calls into a runtime-suspended device, why suspend hangs, why a board wakes immediately, and how clocks, regulators, IRQs, DMA, workqueues, and wakeup policy fit together.

## Beginner

### 1. What problem does Linux power management solve for device drivers?

**Level:** Beginner

**Short Answer:**  
It gives drivers a common way to safely reduce device and system power while preserving correct hardware state and wakeup behavior.

**Deep Explanation:**  
Real devices need power rails, clocks, resets, pin states, IRQs, DMA, and register state handled in a specific order. Linux PM provides framework rules and callbacks so drivers can suspend individual devices during runtime, participate in whole-system sleep, and declare which devices may wake the system. The goal is not just "turn power off"; it is "turn power off only when nobody can safely touch the hardware, then restore it before use."

**API / Code Anchor:**  
`struct device`, `struct dev_pm_ops`, `pm_runtime_enable()`, `pm_runtime_resume_and_get()`, `/sys/power/state`.

**Production or Debugging Angle:**  
A driver that probes and works once can still be unusable in production if it drains battery, fails resume, or races register access while powered down.

**Common Traps:**  
- Thinking PM is optional for battery-powered products only.
- Treating suspend as simply disabling a clock.
- Ignoring userspace, IRQ, DMA, and workqueue paths that can still touch hardware.

**Follow-up Questions:**  
- What state must a driver save before power is removed?
- Which paths in a driver can access hardware?
- Why does PM need device ordering?

### 2. What is the difference between runtime PM and system sleep PM?

**Level:** Beginner

**Short Answer:**  
Runtime PM manages one device while the system is still running. System sleep PM coordinates the whole system into a sleep state such as freeze, suspend-to-RAM, or hibernation.

**Deep Explanation:**  
Runtime PM is driven by per-device activity. If no user, stream, transfer, or child needs the device, the PM core may call the driver's runtime suspend callback. System sleep is global: userspace is frozen, devices are suspended in order, interrupts move through late/noirq phases, and the platform enters a sleep state. A runtime-suspended device may still need special handling during system sleep.

**API / Code Anchor:**  
Runtime PM: `runtime_suspend`, `runtime_resume`, `pm_runtime_get*()`, `pm_runtime_put*()`.  
System sleep: `suspend`, `resume`, `freeze`, `thaw`, `poweroff`, `restore`, `suspend_late`, `suspend_noirq`.

**Production or Debugging Angle:**  
If a bug reproduces only after `echo mem > /sys/power/state`, it may be system sleep ordering. If it reproduces after the device is idle for a few seconds, it may be runtime PM or autosuspend.

**Common Traps:**  
- Assuming runtime suspend and system suspend callbacks can always be identical.
- Forgetting system sleep may find the device already runtime suspended.
- Debugging a runtime PM bug by only testing suspend-to-RAM.

**Follow-up Questions:**  
- What is autosuspend?
- Why must system suspend account for runtime-suspended devices?
- How would you disable runtime PM temporarily for debugging?

### 3. What is `struct dev_pm_ops`?

**Level:** Beginner

**Short Answer:**  
`struct dev_pm_ops` is the callback table where a driver provides runtime PM and system sleep handlers.

**Deep Explanation:**  
The PM core uses callback tables associated with the device, bus, class, type, driver, or PM domain. Driver callbacks usually live in `struct dev_pm_ops` and are attached through `.driver.pm`. Runtime callbacks handle per-device idle/active transitions. System sleep callbacks handle global suspend/resume phases.

**API / Code Anchor:**  
```c
static const struct dev_pm_ops foo_pm_ops = {
        RUNTIME_PM_OPS(foo_runtime_suspend, foo_runtime_resume, NULL)
        SYSTEM_SLEEP_PM_OPS(foo_suspend, foo_resume)
};
```

**Production or Debugging Angle:**  
If callbacks do not run, check whether `.driver.pm` is wired, whether `CONFIG_PM` affects macro expansion, and whether a bus/subsystem or PM domain owns the effective callbacks.

**Common Traps:**  
- Using legacy `.suspend`/`.resume` platform-driver fields in new code.
- Forgetting that subsystem callbacks may wrap or override driver callbacks.
- Putting hardware access in callbacks without locking against normal I/O.

**Follow-up Questions:**  
- Where is `struct dev_pm_ops` attached in a platform driver?
- What are late and noirq callbacks for?
- Why might a PM domain callback run before a driver callback?

### 4. What is a wakeup source?

**Level:** Beginner

**Short Answer:**  
A wakeup source is a device or event that can keep the system awake or wake it from system sleep.

**Deep Explanation:**  
Wakeup has two sides: hardware capability and userspace policy. A driver can mark a device as wakeup-capable. Userspace can then enable or disable whether that device may wake the system. During system suspend, the driver should arm wake IRQs only when policy allows it.

**API / Code Anchor:**  
`device_init_wakeup()`, `device_can_wakeup()`, `device_may_wakeup()`, `enable_irq_wake()`, `dev_pm_set_wake_irq()`, `/sys/devices/.../power/wakeup`.

**Production or Debugging Angle:**  
For "system wakes immediately" bugs, inspect `/sys/kernel/debug/wakeup_sources`, per-device `power/wakeup`, IRQ counts, and device status registers.

**Common Traps:**  
- Treating wakeup-capable and wakeup-enabled as the same thing.
- Enabling wake IRQs without checking `device_may_wakeup()`.
- Confusing `IRQF_NO_SUSPEND` with wakeup capability.

**Follow-up Questions:**  
- How do you expose wakeup policy to userspace?
- What is the difference between `enable_irq_wake()` and `IRQF_NO_SUSPEND`?
- Why clear interrupt status before arming wake?

## Mid-level

### 5. How does a driver safely access registers when runtime PM is enabled?

**Level:** Mid-level

**Short Answer:**  
Resume the device and take a runtime PM usage reference before register access, then drop the reference after the access, usually with autosuspend.

**Deep Explanation:**  
Runtime PM may power down clocks, regulators, or a power domain when the usage count is zero. A driver must raise the usage count and resume the device before touching registers. After the operation, it marks the device recently busy and releases the reference. Locking must prevent runtime suspend from racing active operations.

**API / Code Anchor:**  
```c
ret = pm_runtime_resume_and_get(dev);
if (ret)
        return ret;

val = readl(base + REG_STATUS);

pm_runtime_mark_last_busy(dev);
pm_runtime_put_autosuspend(dev);
```

**Production or Debugging Angle:**  
Register reads returning timeouts or bus faults after idle often mean a missing runtime PM get in a sysfs, ioctl, IRQ thread, workqueue, or subsystem callback.

**Common Traps:**  
- Touching registers from debugfs/sysfs without runtime resume.
- Taking a PM reference but returning early without a put.
- Assuming IRQ handlers never run while the device is runtime suspended.

**Follow-up Questions:**  
- Why use autosuspend instead of immediate suspend?
- Which paths in your driver need runtime PM protection?
- What lock protects hardware access versus PM callbacks?

### 6. What is the `pm_runtime_get_sync()` error-path trap?

**Level:** Mid-level

**Short Answer:**  
If `pm_runtime_get_sync()` returns an error, the usage count may still have been incremented, so returning immediately can leak a runtime PM reference.

**Deep Explanation:**  
Many older drivers use `pm_runtime_get_sync()` before hardware access. The problem is that the helper increments the usage count before attempting resume. If resume fails, the caller must understand whether and how to balance the count. Modern code often uses `pm_runtime_resume_and_get()` because it handles the failed-resume case more safely for checked paths.

**API / Code Anchor:**  
Prefer:
```c
ret = pm_runtime_resume_and_get(dev);
if (ret)
        return ret;
```

Be careful with:
```c
ret = pm_runtime_get_sync(dev);
if (ret < 0)
        return ret; /* likely leaked usage count */
```

**Production or Debugging Angle:**  
A leaked usage count keeps the device permanently active, so power never drops and autosuspend never triggers.

**Common Traps:**  
- Copying old `pm_runtime_get_sync()` snippets without balancing errors.
- Only testing the success path.
- Hiding the leak because the device still "works".

**Follow-up Questions:**  
- How would you find a leaked runtime PM reference?
- What sysfs attribute may expose runtime usage?
- When might old helpers still be used in existing drivers?

### 7. What is autosuspend and when should you use it?

**Level:** Mid-level

**Short Answer:**  
Autosuspend delays runtime suspend after the last use, reducing power-cycling overhead when a device is accessed repeatedly.

**Deep Explanation:**  
Some hardware is expensive to power cycle: regulators need ramp time, PLLs need lock time, sensors need startup time, and buses may be slow. Autosuspend lets the driver drop the usage count but keep the device active for a short delay. If more work arrives soon, the driver avoids a suspend/resume cycle.

**API / Code Anchor:**  
`pm_runtime_use_autosuspend()`, `pm_runtime_set_autosuspend_delay()`, `pm_runtime_mark_last_busy()`, `pm_runtime_put_autosuspend()`.

**Production or Debugging Angle:**  
Autosuspend is a power-versus-latency knob. A delay that saves power on one product may cause bad latency or too much power draw on another.

**Common Traps:**  
- Forgetting `pm_runtime_mark_last_busy()` before `pm_runtime_put_autosuspend()`.
- Setting an arbitrary delay without measurement.
- Using autosuspend to hide missing state restoration.

**Follow-up Questions:**  
- How would you choose an autosuspend delay?
- What happens if userspace sets `power/control=on`?
- Why can autosuspend complicate remove/unbind?

### 8. How should a driver handle wake IRQ setup during suspend?

**Level:** Mid-level

**Short Answer:**  
Declare wakeup capability during probe, respect userspace policy during suspend, clear stale status, and arm the wake IRQ only when `device_may_wakeup()` is true.

**Deep Explanation:**  
Wake IRQs are part of system sleep policy. The driver may know that the hardware can wake the system, but userspace decides whether it currently should. During suspend, the driver must avoid arming disabled wake sources. It must also clear stale device interrupt state so an old event does not immediately wake the system.

**API / Code Anchor:**  
`device_init_wakeup(dev, true)`, `device_may_wakeup(dev)`, `enable_irq_wake(irq)`, `disable_irq_wake(irq)`, `dev_pm_set_wake_irq(dev, irq)`.

**Production or Debugging Angle:**  
For wake bugs, compare `power/wakeup`, IRQ counts, and wakeup-source statistics before and after a suspend attempt.

**Common Traps:**  
- Enabling wake IRQ regardless of policy.
- Forgetting to disable wake IRQ on resume.
- Leaving stale status bits set before suspend.

**Follow-up Questions:**  
- When would you use `dev_pm_set_wake_irq()`?
- Why is `IRQF_NO_SUSPEND` not enough?
- How do shared IRQ lines affect wake debugging?

### 9. How do workqueues, timers, tasklets, and DMA interact with PM callbacks?

**Level:** Mid-level

**Short Answer:**  
They are asynchronous hardware users, so a driver must quiesce or protect them before powering down hardware.

**Deep Explanation:**  
Runtime suspend and system suspend cannot safely remove clocks or power while delayed work, timer callbacks, IRQ bottom halves, tasklets, DMA completions, or streaming code can still access registers. The driver needs clear ownership rules: either cancel/flush activity before suspend or ensure those paths take runtime PM references and obey locking.

**API / Code Anchor:**  
`cancel_delayed_work_sync()`, `flush_work()`, `del_timer_sync()`, subsystem stream stop callbacks, DMA terminate APIs, runtime PM get/put around worker hardware access.

**Production or Debugging Angle:**  
Suspend hangs and resume crashes often happen because an async path touched hardware after clocks were disabled or waited forever for DMA that was never stopped.

**Common Traps:**  
- Canceling work too late.
- Flushing work while holding a lock that the work function needs.
- Assuming the task freezer stops all kernel work automatically.

**Follow-up Questions:**  
- What is `WQ_FREEZABLE`?
- Should every workqueue be freezable?
- How do you avoid deadlock while flushing work in suspend?

## Senior

### 10. How would you design PM for a driver that has open file operations, an IRQ thread, DMA, and runtime autosuspend?

**Level:** Senior

**Short Answer:**  
Define a single hardware-access contract: all paths that touch hardware must either hold a runtime PM reference or run only while a protected active state is guaranteed, and suspend paths must quiesce DMA/IRQ/work before disabling resources.

**Deep Explanation:**  
The hard part is not adding callbacks; it is making all concurrent paths agree on state. File operations may start transfers, IRQ threads may complete them, DMA may continue independently, and autosuspend may run after the usage count drops. A robust design has a device lock, state bits for running/stopping/suspended, runtime PM get/put around user operations, orderly stream stop, IRQ masking where needed, DMA termination before power-down, and remove paths that prevent new users before resources disappear.

**API / Code Anchor:**  
`pm_runtime_resume_and_get()`, `pm_runtime_put_autosuspend()`, `mutex`, `spinlock_t` for IRQ state where appropriate, `disable_irq()`, DMA terminate/synchronize APIs, `pm_runtime_disable()`.

**Production or Debugging Angle:**  
Review all hardware access sites, not just PM callbacks. Search for `readl`, `writel`, regmap calls, DMA start/stop, IRQ thread code, work functions, and subsystem callbacks.

**Common Traps:**  
- Protecting file operations but forgetting debugfs/sysfs.
- Letting autosuspend race with remove.
- Powering down while IRQ status can still cause an IRQ thread to run.

**Follow-up Questions:**  
- What lock protects PM state?
- Which callbacks can sleep?
- How would you test remove while autosuspend is pending?

### 11. A system suspends, then immediately wakes. How do you debug it?

**Level:** Senior

**Short Answer:**  
Identify the wake source, confirm policy, inspect IRQ/status bits, and verify the driver arms wake only for intentional events.

**Deep Explanation:**  
Immediate wake usually means a device generated or reported a wake event as soon as suspend armed wake sources. Causes include uncleared interrupt status, noisy GPIO, PMIC nested IRQ, wrong wake policy, shared IRQ confusion, or using `IRQF_NO_SUSPEND` incorrectly. Debugging should move from global evidence to device-specific registers.

**API / Code Anchor:**  
`/sys/kernel/debug/wakeup_sources`, `/sys/devices/.../power/wakeup`, `enable_irq_wake()`, `disable_irq_wake()`, `pm_wakeup_event()`, `device_may_wakeup()`.

**Production or Debugging Angle:**  
Collect dmesg with suspend timestamps, compare IRQ counts before and after suspend, inspect PMIC/GPIO status registers, and disable candidate wake sources one at a time through sysfs policy where possible.

**Common Traps:**  
- Assuming the last printed driver is the wake source.
- Disabling all wake sources and declaring success.
- Forgetting nested PMIC/GPIO interrupt controllers may hide the real child source.

**Follow-up Questions:**  
- How do you distinguish a wake IRQ from an IRQ kept enabled during suspend?
- What evidence would prove stale status caused the wake?
- What should the driver do in suspend before enabling wake?

### 12. A device works after boot but fails after resume from deep sleep. What do you suspect?

**Level:** Senior

**Short Answer:**  
Suspect lost hardware state, wrong power sequencing, missing pinctrl/default restore, stale IRQ state, or callbacks that assume runtime suspend and deep system sleep are equivalent.

**Deep Explanation:**  
Deep sleep may remove power from rails or domains that stay alive during runtime suspend or suspend-to-idle. Registers may reset, PLLs may lose lock, pinmux may switch to sleep state, and external devices may need datasheet delays. The resume path must rebuild the hardware state before userspace or subsystem operations resume.

**API / Code Anchor:**  
`resume`, `resume_early`, `resume_noirq`, `pm_runtime_force_resume()`, `pinctrl_pm_select_default_state()`, clock/regulator enable APIs, driver register restore helpers.

**Production or Debugging Angle:**  
Compare register dumps before suspend and after resume, check `/sys/power/mem_sleep`, measure rails/clocks, and test `s2idle` versus deep sleep to narrow the power-loss boundary.

**Common Traps:**  
- Testing only `freeze` or `s2idle` and claiming suspend-to-RAM works.
- Restoring registers before clocks or supplies are stable.
- Forgetting to reconfigure wake/status registers after resume.

**Follow-up Questions:**  
- What state is volatile in this hardware?
- Which sleep mode removes the relevant power domain?
- When would you need a noirq resume callback?

### 13. How do parent/child ordering and power domains affect driver PM?

**Level:** Senior

**Short Answer:**  
Parent devices and PM domains represent shared resources. A child must not access hardware after its parent bus, clock, regulator, or domain has been suspended.

**Deep Explanation:**  
The device model uses parent/child relationships and active-child counts so parents usually stay active while children are active. Generic PM domains can group devices behind shared power islands and may provide callbacks that wrap driver callbacks. If the topology is wrong, Linux may suspend a resource provider before a consumer finishes, causing bus errors or resume failures.

**API / Code Anchor:**  
`struct device` parent, active-child runtime PM accounting, `struct dev_pm_domain`, `power-domains` in Device Tree, device links, `pm_suspend_ignore_children()`.

**Production or Debugging Angle:**  
When a child fails only after the bus or domain idles, inspect device hierarchy, DT `power-domains`, clock/regulator providers, and whether device links represent supplier/consumer ordering.

**Common Traps:**  
- Forcing `pm_suspend_ignore_children()` to hide a topology problem.
- Assuming probe order equals PM order.
- Forgetting that PM domains may own the outer PM transition.

**Follow-up Questions:**  
- What is an active-child count?
- When should you add a device link?
- How can a PM domain alter callback ordering?

### 14. What should a system sleep callback do if the device is already runtime suspended?

**Level:** Senior

**Short Answer:**  
It should not blindly re-suspend active hardware state. It must detect or rely on PM helpers to handle the current runtime PM state and only perform the system-sleep-specific work needed.

**Deep Explanation:**  
System sleep may start when a device is active, idle, or already runtime suspended. If the suspend callback assumes active hardware, it may touch powered-off registers. If it assumes suspended hardware, it may skip necessary quiesce work. Some drivers use `pm_runtime_force_suspend()` and `pm_runtime_force_resume()` to bridge runtime PM state into system sleep, but the exact approach depends on hardware and subsystem rules.

**API / Code Anchor:**  
`pm_runtime_status_suspended()` where available, `pm_runtime_force_suspend()`, `pm_runtime_force_resume()`, `runtime_suspend`, `suspend`.

**Production or Debugging Angle:**  
This bug often shows up as suspend-only register faults or resume-only missing state. Test both active-at-suspend and idle-at-suspend cases.

**Common Traps:**  
- Calling runtime suspend logic twice without idempotence.
- Reading registers in system suspend when runtime PM already gated clocks.
- Forgetting wake setup may still be needed even if the device is runtime suspended.

**Follow-up Questions:**  
- When are force suspend/resume helpers appropriate?
- How would you test both active and idle suspend cases?
- What state belongs only to system sleep, not runtime PM?

### 15. How do you review a driver's PM implementation before merging it?

**Level:** Senior

**Short Answer:**  
Review state ownership, callback ordering, PM reference balance, hardware access sites, resource sequencing, wakeup policy, remove/error paths, and debugability.

**Deep Explanation:**  
A good PM review starts with the lifecycle: probe, normal I/O, runtime suspend/resume, system suspend/resume, remove, and error paths. Then inspect every hardware access site and ask whether the device is guaranteed active. Check whether clocks/regulators/resets/pinctrl follow datasheet order, whether wakeup policy is respected, and whether subsystem users are quiesced before power-down.

**API / Code Anchor:**  
`struct dev_pm_ops`, `pm_runtime_enable()`, `pm_runtime_disable()`, `pm_runtime_resume_and_get()`, `pm_runtime_put_autosuspend()`, `device_init_wakeup()`, `device_may_wakeup()`, `enable_irq_wake()`.

**Production or Debugging Angle:**  
Ask for tests: runtime PM idle/resume, open/read/write while autosuspend is enabled, suspend while streaming, wakeup enabled/disabled, remove during idle, probe failure injection, and resume from each supported sleep mode.

**Common Traps:**  
- Reviewing only the PM callbacks.
- Missing debugfs/sysfs/hwmon/IIO/V4L2/ALSA paths that access registers.
- Ignoring old helper caveats such as `pm_runtime_get_sync()` error balancing.

**Follow-up Questions:**  
- What tests would you require on real hardware?
- How would you document PM assumptions?
- Which failures should return errors and which should leave the device active?

## Debugging Scenarios

### Scenario A: Runtime PM enabled, but power never drops

**Level:** Mid-level

**Short Answer:**  
Look for a leaked usage count, `power/control=on`, an active child, or a subsystem path that never releases its PM reference.

**Deep Explanation:**  
Runtime PM suspends only when the device is allowed to idle. A nonzero usage count, active child, disabled runtime PM, or userspace policy forcing the device on will keep it active. The bug may be in an error path, stream stop path, open/close path, or old `pm_runtime_get_sync()` failure path.

**API / Code Anchor:**  
`/sys/devices/.../power/control`, `/sys/devices/.../power/runtime_status`, `/sys/devices/.../power/runtime_usage`, `pm_runtime_resume_and_get()`, `pm_runtime_put_autosuspend()`.

**Production or Debugging Angle:**  
Add temporary dynamic debug around every PM get/put and test success plus failure paths.

**Common Traps:**  
- Assuming autosuspend is broken when the device is simply forced on.
- Ignoring children that keep a parent active.
- Forgetting a failed resume can leak a reference with older helper patterns.

**Follow-up Questions:**  
- How do active children affect parent runtime PM?
- What should remove do before disabling runtime PM?
- How do you reproduce a usage leak reliably?

### Scenario B: Suspend hangs after "freezing user space processes"

**Level:** Senior

**Short Answer:**  
Suspect a device callback waiting for userspace, blocked work, a lock inversion, DMA that never quiesces, or a subsystem user that was not stopped before suspend.

**Deep Explanation:**  
After userspace is frozen, callbacks should not depend on userspace responses. Kernel async paths must be stopped or made safe. A driver may deadlock by flushing work while holding a lock the work needs, waiting for firmware/user helper after freeze, or waiting for hardware completion after the clock was already disabled.

**API / Code Anchor:**  
`pm_test`, ftrace power events, dynamic debug, `cancel_delayed_work_sync()`, DMA synchronization/terminate APIs, lockdep.

**Production or Debugging Angle:**  
Use staged PM testing when available, inspect blocked task traces, and add tracepoints around callback entry/exit and async work completion.

**Common Traps:**  
- Assuming the last visible log line names the faulty driver.
- Disabling PM entirely instead of isolating the phase.
- Flushing work in a lock order that can deadlock.

**Follow-up Questions:**  
- What does `pm_test` help isolate?
- Why are firmware requests risky during suspend?
- How would you instrument callback timing?
