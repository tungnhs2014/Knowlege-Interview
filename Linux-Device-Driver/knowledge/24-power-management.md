# 24 - Power Management

## Learning Goal
Understand how Linux lets device drivers save power without breaking correctness. After this topic, you should be able to explain runtime PM, system sleep, wakeup sources, PM callback ordering, and the common races that make suspend/resume bugs painful on real boards.

After this topic you should be able to:

- Explain **runtime PM versus system sleep PM**.
- Add `struct dev_pm_ops` callbacks to a driver.
- Use runtime PM helpers without leaking usage counts.
- Reason about clocks, regulators, pinctrl, DMA, IRQs, workqueues, and userspace during suspend.
- Configure wakeup behavior without confusing wake IRQs with `IRQF_NO_SUSPEND`.
- Debug devices that fail to suspend, fail to resume, or immediately wake the system.

## Why This Matters In Real Work
Power management is where a driver stops being "it probes" and starts being production quality. A device that works during boot can still drain batteries, lock up resume, corrupt DMA, lose register state, or wake the system forever.

You see PM in real driver work when:

- A sensor, touchscreen, camera, codec, modem, display, or PMIC interrupt should sleep when unused.
- A system must pass suspend-to-RAM, hibernation, or suspend-to-idle tests.
- A device must wake the system from an RTC alarm, key press, GPIO, PMIC event, or network event.
- A runtime-suspended device is accessed from `read()`, `ioctl()`, IRQ, workqueue, or subsystem callback.
- Board power sequencing depends on clocks, regulators, reset lines, GPIOs, and pinctrl sleep states.

Without the PM core, every driver would invent private suspend logic. With it, Linux has common ordering, common callback tables, common sysfs controls, and shared rules for parent/child devices and power domains.

## Mental Model
Think of power management as **safe state transitions**. The driver must know when hardware is usable, when it is asleep, who is allowed to wake it, and what must be stopped before power is removed.

Three simple pictures help:

| Model | Simple Meaning | Typical Trigger |
| --- | --- | --- |
| Runtime PM | "Turn this device down while Linux keeps running." | No active users, stream stopped, file closed, autosuspend timeout. |
| System sleep PM | "Move the whole system into a sleep state." | `echo mem > /sys/power/state`, policy daemon, firmware/platform event. |
| Wakeup source | "This device is allowed to wake or keep the system awake." | RTC alarm, key press, GPIO/PMIC IRQ, network wake event. |

Runtime PM is local to a device. System sleep is global and ordered. They interact, but they are not the same thing.

## Core Concepts
Power management works because the kernel tracks device state, device hierarchy, callback ownership, and wakeup policy.

| Concept | Meaning |
| --- | --- |
| Active device | Hardware is powered and safe to access. |
| Runtime suspended device | The individual device is in a low-power state while the system is still awake. |
| Usage count | Runtime PM reference count. Nonzero means the device is needed. |
| Active-child count | Parent devices usually stay active while children are active. |
| Autosuspend | Delay runtime suspend after last use to avoid rapid power cycling. |
| System sleep | Whole-system transition such as freeze, standby, suspend-to-RAM, or hibernation. |
| Wakeup-capable | Hardware and driver can signal wake. |
| Wakeup-enabled | Userspace policy currently permits that device to wake the system. |
| PM domain | Shared power resource grouping devices, often called genpd for generic PM domains. |
| Noirq phase | Late system-sleep phase after ordinary device IRQs are disabled. |

Useful comparisons:

| Runtime PM | System Sleep PM |
| --- | --- |
| Per-device. | Whole-system. |
| Happens while tasks and most kernel services continue. | Freezes userspace and suspends devices in ordered phases. |
| Driven by usage counters and autosuspend. | Driven by system sleep entry and device hierarchy. |
| Common callbacks: `runtime_suspend`, `runtime_resume`, `runtime_idle`. | Common callbacks: `prepare`, `suspend`, `suspend_late`, `suspend_noirq`, `resume_noirq`, `resume_early`, `resume`, `complete`. |
| Usually entered many times during normal operation. | Usually tested less often but must be very reliable. |

## Kernel Mechanism
The PM core is built into the Linux device model. Every `struct device` carries PM state, hierarchy, wakeup state, and links to bus/class/type/driver callbacks.

The callback owner is selected through the device stack:

1. PM domain, if the device belongs to one.
2. Device type or class callbacks, if present.
3. Bus callbacks, if present.
4. Driver `struct dev_pm_ops` callbacks.

That layering matters because a subsystem may wrap or replace driver callbacks. For example, a bus may need to handle transport-specific suspend before or after the driver powers down hardware.

Runtime PM uses:

- A runtime status such as active or suspended.
- A **usage count** that protects active hardware access.
- An active-child count for parent/child ordering.
- Optional autosuspend timer/work.
- PM callbacks that actually save state, gate clocks, disable regulators, or restore hardware.

System sleep uses:

- Global sleep state selection.
- Task freezer and notifier phases.
- Ordered device suspend and resume.
- Late and noirq phases for ordering-sensitive work.
- Wakeup-source accounting so events can abort or exit sleep.

## Key Structs And APIs
These APIs are worth learning as mechanisms, not as a memory dump. The important question is always: "Who owns the hardware state right now?"

### Core structs

| Struct | Why It Matters |
| --- | --- |
| `struct device` | The PM core's anchor for hierarchy, PM state, wakeup, parent/child ordering, and subsystem ownership. |
| `struct dev_pm_info` | Per-device PM state stored inside `struct device`. Driver code rarely edits it directly. |
| `struct dev_pm_ops` | Callback table for runtime PM and system sleep. |
| `struct wakeup_source` | Tracks wakeup events and prevents premature suspend while events are being handled. |
| `struct dev_pm_domain` | Lets a power domain own PM policy for groups of devices. |

### Runtime PM helpers

| API | Use |
| --- | --- |
| `pm_runtime_enable(dev)` | Allow runtime PM for the device after initial state is coherent. |
| `pm_runtime_disable(dev)` | Stop runtime PM, usually in remove/error paths. |
| `pm_runtime_set_active(dev)` | Tell the PM core hardware is currently active. |
| `pm_runtime_set_suspended(dev)` | Tell the PM core hardware is currently suspended. |
| `pm_runtime_resume_and_get(dev)` | Resume synchronously and increment usage count; returns error without the common `get_sync` leak trap. |
| `pm_runtime_get_sync(dev)` | Older/common helper; on error the usage count may still need balancing. |
| `pm_runtime_put(dev)` | Drop a usage reference and let idle/suspend run asynchronously. |
| `pm_runtime_put_sync(dev)` | Drop a usage reference and run idle/suspend synchronously. |
| `pm_runtime_mark_last_busy(dev)` | Record recent activity before autosuspend. |
| `pm_runtime_put_autosuspend(dev)` | Drop usage reference and use autosuspend delay. |
| `pm_runtime_use_autosuspend(dev)` | Enable autosuspend behavior. |
| `pm_runtime_set_autosuspend_delay(dev, ms)` | Choose autosuspend delay. |
| `pm_runtime_force_suspend(dev)` / `pm_runtime_force_resume(dev)` | Bridge runtime PM state during system sleep for some drivers. |
| `pm_suspend_ignore_children(dev, true)` | Let a parent runtime suspend even when children would normally keep it active. Use carefully. |
| `pm_runtime_irq_safe(dev)` | Allow runtime PM callbacks in atomic context only when callbacks truly do not sleep. |

### Callback helpers

Modern drivers usually put PM callbacks in `struct dev_pm_ops`:

```c
static const struct dev_pm_ops foo_pm_ops = {
        RUNTIME_PM_OPS(foo_runtime_suspend,
                       foo_runtime_resume,
                       foo_runtime_idle)
        SYSTEM_SLEEP_PM_OPS(foo_suspend, foo_resume)
};
```

Then attach them through the embedded `struct device_driver`:

```c
static struct platform_driver foo_driver = {
        .probe = foo_probe,
        .remove_new = foo_remove,
        .driver = {
                .name = "foo",
                .of_match_table = foo_of_match,
                .pm = pm_ptr(&foo_pm_ops),
        },
};
```

### Wakeup helpers

| API | Use |
| --- | --- |
| `device_init_wakeup(dev, true)` | Mark the device wakeup-capable and create policy hooks. |
| `device_can_wakeup(dev)` | Check hardware/driver wakeup capability. |
| `device_may_wakeup(dev)` | Check whether userspace policy currently allows wakeup. |
| `dev_pm_set_wake_irq(dev, irq)` | Associate a managed wake IRQ where suitable. |
| `enable_irq_wake(irq)` / `disable_irq_wake(irq)` | Arm/disarm an IRQ as a system wake source. |
| `pm_wakeup_event(dev, msec)` | Tell PM core that a wakeup event happened and keep system awake briefly. |
| `IRQF_NO_SUSPEND` | Keep an IRQ enabled during suspend. **This is not the same as making it a wake IRQ.** |

### Userspace and debug interfaces

| Interface | Use |
| --- | --- |
| `/sys/power/state` | Available system sleep states such as `freeze`, `standby`, `mem`, `disk`. |
| `/sys/power/mem_sleep` | Meaning of `mem`: often `s2idle`, `shallow`, or `deep`. |
| `/sys/power/disk` | Hibernation mode selection. |
| `/sys/devices/.../power/control` | Runtime PM policy, commonly `auto` or `on`. |
| `/sys/devices/.../power/runtime_status` | Runtime PM status when exposed. |
| `/sys/devices/.../power/autosuspend_delay_ms` | Autosuspend delay when exposed. |
| `/sys/devices/.../power/wakeup` | Per-device wakeup policy. |
| `/sys/kernel/debug/wakeup_sources` | Wakeup-source statistics when debugfs support is enabled. |

## Lifecycle / Data Flow
Power management is mostly about ordering. The exact hardware steps differ, but the shape is common.

### Probe

```text
probe()
  acquire resources: registers, IRQs, clocks, regulators, resets, pinctrl
  initialize hardware enough to know its state
  set runtime PM initial state: active or suspended
  configure autosuspend if useful
  enable runtime PM
  register subsystem/user-visible interfaces
```

Rules:

- Do not register users before the hardware and PM state are coherent.
- If the device is already powered by firmware, decide whether to start as active or explicitly suspend it.
- If probe fails after enabling runtime PM, disable runtime PM and balance usage counts.

### Normal access

```text
driver operation
  pm_runtime_resume_and_get(dev)
  access registers / start transfer / perform operation
  pm_runtime_mark_last_busy(dev)
  pm_runtime_put_autosuspend(dev)
```

Rules:

- Every path that touches powered hardware needs an active device.
- Error paths must drop usage references.
- IRQ/workqueue paths need the same state discipline as file operations or subsystem callbacks.

### Runtime suspend

```text
runtime_suspend()
  block new hardware users
  ensure DMA/streaming/work/timers cannot touch registers
  save volatile device state if power may be lost
  mask or quiesce device interrupts if needed
  select sleep pinctrl state
  disable clocks, regulators, resets, or domain resources in safe order
```

Rules:

- Runtime suspend must not race with an active transfer.
- Do not sleep in atomic context unless the PM path has explicitly been made IRQ-safe and callbacks are non-sleeping.
- Parent power resources must not disappear while children are still active.

### Runtime resume

```text
runtime_resume()
  enable supplies/clocks/domain resources
  wait for hardware readiness
  select default pinctrl state
  restore volatile registers
  clear stale status/interrupts
  allow I/O again
```

Rules:

- Restore state before users can observe the device.
- Clear stale interrupt status before enabling interrupt generation.
- Treat failed resume as a real hardware failure and unwind carefully.

### System suspend and resume

```text
system suspend
  freeze userspace and quiesce subsystem activity
  prepare devices
  suspend devices
  run late/noirq suspend phases
  enter platform sleep state

system resume
  leave platform sleep state
  run noirq/early resume phases
  resume devices
  complete devices
  thaw userspace
```

Rules:

- System sleep callbacks must account for devices that were already runtime suspended.
- Only arm wake IRQs when `device_may_wakeup(dev)` is true.
- Avoid firmware requests or userspace-dependent operations after userspace is frozen.
- Use late/noirq callbacks only when ordinary callbacks are too early or too late.

### Remove and unbind

```text
remove()
  unregister users/subsystem interface
  stop new operations and pending work
  runtime resume if hardware must be accessed for shutdown
  disable interrupts/DMA
  disable runtime PM
  power off or leave hardware in defined state
  release resources
```

Rules:

- Remove must not race with autosuspend work.
- User-visible interfaces should disappear before hardware state is destroyed.
- If shutdown needs register access, resume first.

## Minimal Practical Example
This is **learning-only pseudo-code**. It shows the shape of a small platform driver with runtime PM and system sleep. Real drivers must add hardware-specific locking, error handling, clocks, regulators, pinctrl, IRQs, and subsystem rules.

```c
struct foo {
        struct device *dev;
        void __iomem *base;
        struct clk *clk;
        struct regulator *vdd;
        struct mutex lock;
};

static int foo_hw_on(struct foo *foo)
{
        int ret;

        ret = regulator_enable(foo->vdd);
        if (ret)
                return ret;

        ret = clk_prepare_enable(foo->clk);
        if (ret) {
                regulator_disable(foo->vdd);
                return ret;
        }

        /* Restore volatile registers here. */
        return 0;
}

static void foo_hw_off(struct foo *foo)
{
        /* Save/stop hardware before gates go away. */
        clk_disable_unprepare(foo->clk);
        regulator_disable(foo->vdd);
}

static int foo_runtime_resume(struct device *dev)
{
        struct foo *foo = dev_get_drvdata(dev);

        return foo_hw_on(foo);
}

static int foo_runtime_suspend(struct device *dev)
{
        struct foo *foo = dev_get_drvdata(dev);

        foo_hw_off(foo);
        return 0;
}

static int foo_read_status(struct foo *foo, u32 *val)
{
        int ret;

        ret = pm_runtime_resume_and_get(foo->dev);
        if (ret)
                return ret;

        mutex_lock(&foo->lock);
        *val = readl(foo->base + FOO_STATUS);
        mutex_unlock(&foo->lock);

        pm_runtime_mark_last_busy(foo->dev);
        pm_runtime_put_autosuspend(foo->dev);
        return 0;
}

static int foo_suspend(struct device *dev)
{
        struct foo *foo = dev_get_drvdata(dev);

        if (device_may_wakeup(dev))
                enable_irq_wake(foo_irq(foo));

        return pm_runtime_force_suspend(dev);
}

static int foo_resume(struct device *dev)
{
        struct foo *foo = dev_get_drvdata(dev);

        if (device_may_wakeup(dev))
                disable_irq_wake(foo_irq(foo));

        return pm_runtime_force_resume(dev);
}

static const struct dev_pm_ops foo_pm_ops = {
        RUNTIME_PM_OPS(foo_runtime_suspend, foo_runtime_resume, NULL)
        SYSTEM_SLEEP_PM_OPS(foo_suspend, foo_resume)
};

static int foo_probe(struct platform_device *pdev)
{
        struct foo *foo;
        int ret;

        foo = devm_kzalloc(&pdev->dev, sizeof(*foo), GFP_KERNEL);
        if (!foo)
                return -ENOMEM;

        foo->dev = &pdev->dev;
        mutex_init(&foo->lock);
        platform_set_drvdata(pdev, foo);

        /* Acquire MMIO, clocks, regulators, IRQs, and pinctrl here. */

        ret = foo_hw_on(foo);
        if (ret)
                return ret;

        pm_runtime_set_active(&pdev->dev);
        pm_runtime_use_autosuspend(&pdev->dev);
        pm_runtime_set_autosuspend_delay(&pdev->dev, 200);
        pm_runtime_enable(&pdev->dev);

        device_init_wakeup(&pdev->dev, true);

        pm_runtime_mark_last_busy(&pdev->dev);
        pm_runtime_put_autosuspend(&pdev->dev);
        return 0;
}
```

Important lines:

- `pm_runtime_set_active()` matches the fact that `foo_hw_on()` powered hardware during probe.
- `pm_runtime_resume_and_get()` protects register access.
- `pm_runtime_mark_last_busy()` plus `pm_runtime_put_autosuspend()` avoids immediate power cycling.
- `device_may_wakeup()` respects userspace wakeup policy.
- `pm_runtime_force_suspend()` and `pm_runtime_force_resume()` bridge runtime PM state during system sleep.

## Common Bugs And Debugging
PM bugs usually appear as timeouts, bus errors, immediate wakeups, missing wakeups, or resume-only failures. Start from the symptom and ask which state transition was wrong.

### Symptom: register access times out or bus read returns errors

Likely causes:

- Driver touched registers while runtime suspended.
- Missing `pm_runtime_resume_and_get()` in an ioctl, sysfs show, IRQ thread, workqueue, or subsystem callback.
- Parent clock, regulator, or power domain was suspended while child device still needed it.

What to inspect:

- `/sys/devices/.../power/runtime_status`
- `/sys/devices/.../power/control`
- dmesg around the failing access
- Dynamic debug in driver access paths and PM callbacks
- Clock/regulator/debugfs state if enabled

Fix patterns:

- Runtime-resume before every hardware access.
- Hold locks so suspend cannot race active transfers.
- Add device links or parent/child PM handling when resources are shared.

### Symptom: runtime PM usage count never drops

Likely causes:

- Error path after `pm_runtime_get_sync()` did not call a matching put.
- Operation returns early without `pm_runtime_put*()`.
- Open/close or stream start/stop paths are unbalanced.

What to inspect:

- `/sys/devices/.../power/runtime_usage` where available
- All `goto` labels after runtime PM get calls
- Subsystem start/stop symmetry

Fix patterns:

- Prefer `pm_runtime_resume_and_get()` for checked resume paths.
- Use a single cleanup label for operations that took a PM reference.
- Put the device after DMA/IRQ/work has really stopped.

### Symptom: system immediately wakes after suspend

Likely causes:

- Stale interrupt status was not cleared before arming wake.
- Wrong device has wakeup enabled.
- Driver enabled wake IRQ without checking `device_may_wakeup()`.
- Shared IRQ line or PMIC interrupt is still asserted.

What to inspect:

- `/sys/devices/.../power/wakeup`
- `/sys/kernel/debug/wakeup_sources`
- IRQ counts before and after suspend
- PMIC/GPIO status registers
- dmesg with `no_console_suspend` when safe

Fix patterns:

- Clear device status before `enable_irq_wake()`.
- Respect `device_may_wakeup()`.
- Separate "IRQ kept enabled" from "IRQ wakes the system".

### Symptom: resume fails only after deep sleep

Likely causes:

- Registers are volatile across power loss and were not restored.
- Pinctrl sleep/default states are incomplete.
- Regulator or clock sequencing is wrong.
- Driver assumed runtime suspend and system suspend have identical hardware state.

What to inspect:

- `/sys/power/mem_sleep`
- Scope/measure rails and clocks if possible
- Register dump before suspend and after resume
- Pinctrl, regulator, reset, and clock debugfs state

Fix patterns:

- Restore all volatile state in resume.
- Split runtime and system callbacks when the hardware state differs.
- Add needed delays from the datasheet.

### Symptom: suspend hangs

Likely causes:

- Driver waits for userspace or firmware after freezer phase.
- Workqueue/timer/IRQ keeps touching hardware while suspend tries to power down.
- DMA or streaming path never quiesces.
- Lock ordering deadlock between PM callback and normal I/O path.

What to inspect:

- `pm_test` stages if available
- ftrace power events
- blocked task traces
- lockdep output
- pending work/timer paths

Fix patterns:

- Quiesce subsystem users before hardware power-down.
- Cancel or flush delayed work in the right phase.
- Avoid userspace-dependent operations in system sleep callbacks.

## Production Checklist
Use this before posting or reviewing a driver with PM support.

Probe and remove:

- Hardware state matches runtime PM initial state.
- Runtime PM is disabled on every probe failure path after it is enabled.
- User-visible interfaces are registered only after PM and hardware state are valid.
- Remove unregisters users, cancels work, disables IRQ/DMA, and prevents autosuspend races.

Runtime PM:

- Every register/DMA operation resumes the device first.
- Every successful PM get has a matching put.
- Error paths after failed resume are balanced.
- Autosuspend delay is justified by latency and power behavior.
- Runtime callbacks do not race IRQs, workqueues, timers, streams, or file operations.

System sleep:

- Callbacks handle devices that are already runtime suspended.
- Wake IRQs are armed only when `device_may_wakeup()` is true.
- Stale wake status is cleared before suspend.
- Late/noirq callbacks are used only for real ordering needs.
- Resume restores all state lost during power removal.

Resources:

- Clocks, regulators, resets, pinctrl, GPIOs, and power domains follow datasheet order.
- Parent/child devices cannot suspend in an unsafe order.
- Shared resources are not disabled while other consumers need them.
- Device links or subsystem mechanisms are used where ordering is not naturally represented.

Debug and ABI:

- `/sys/devices/.../power/control` behavior is understood.
- Wakeup policy through `/sys/devices/.../power/wakeup` is respected.
- Debugfs/ftrace/dynamic-debug workflows are documented for bring-up.
- PM changes do not break userspace ABI or expected open/read/ioctl behavior.

## Interview Readiness
For interviews, focus on reasoning through state transitions. Strong answers explain what is powered, who owns access, which callback runs, and how bugs show up.

You should be able to explain:

- Runtime PM versus system sleep.
- Usage count and active-child count.
- Why `pm_runtime_resume_and_get()` is safer than careless `pm_runtime_get_sync()` usage.
- Autosuspend and power/latency tradeoffs.
- `struct dev_pm_ops` callback groups.
- Wakeup-capable versus wakeup-enabled.
- `enable_irq_wake()` versus `IRQF_NO_SUSPEND`.
- How to debug immediate wakeups, missing wakeups, suspend hangs, and resume failures.
- How PM interacts with clocks, regulators, pinctrl, IRQs, DMA, workqueues, and userspace.

See `interview/24-power-management.md` for question practice.

## Kernel Version Notes
PM APIs are stable in concept, but helper style changes over kernel versions. Read the target kernel headers before copying examples.

Practical notes:

- Older material often uses `SIMPLE_DEV_PM_OPS()`. Current kernels mark it deprecated; prefer newer helpers such as `DEFINE_SIMPLE_DEV_PM_OPS()` or explicit `struct dev_pm_ops` patterns supported by your target tree.
- `SET_RUNTIME_PM_OPS()` and `SET_SYSTEM_SLEEP_PM_OPS()` still appear widely, but newer `RUNTIME_PM_OPS()` and `SYSTEM_SLEEP_PM_OPS()` style may be available depending on kernel version.
- `pm_runtime_get_sync()` is common in older drivers, but if it returns an error the usage count handling is easy to get wrong. Prefer `pm_runtime_resume_and_get()` when available.
- Device Tree binding documents have moved from many old `.txt` files to YAML schemas. Validate properties such as `wakeup-source`, `power-domains`, OPP tables, clocks, regulators, and pinctrl states against the binding for your kernel.
- Debug interfaces depend on kernel configuration: debugfs, ftrace events, wakeup-source stats, and runtime PM sysfs attributes may not all be enabled.
