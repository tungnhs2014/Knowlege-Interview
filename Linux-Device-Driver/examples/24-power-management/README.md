# 24 - Power Management Example

## Status
This is a **learning-only** example.

It does not bind to real hardware or provide a buildable kernel module. Power management is easiest to teach safely with a driver skeleton plus real inspection commands, because a fake module cannot prove that clocks, regulators, IRQ wake, parent devices, or power domains behave correctly on your board.

## Goal
Learn the normal power-management lifecycle for a Linux device driver:

- Enable runtime PM during probe.
- Resume the device before touching registers.
- Mark activity and autosuspend when work is done.
- Quiesce I/O before runtime suspend or system suspend.
- Respect userspace wakeup policy.
- Debug runtime PM and system sleep with sysfs, debugfs, dmesg, and tracefs.

The example centers on a platform-style MMIO device, but the same flow applies to I2C, SPI, MFD child, IIO, media, and audio drivers after substituting subsystem-specific registration and transfer APIs.

## Kernel Version Assumptions
The pseudo-code assumes a modern Linux 6.x-style kernel with:

- `struct dev_pm_ops`
- `pm_runtime_enable()`
- `pm_runtime_disable()`
- `pm_runtime_set_active()`
- `pm_runtime_get_noresume()`
- `pm_runtime_resume_and_get()`
- `pm_runtime_mark_last_busy()`
- `pm_runtime_put_autosuspend()`
- `pm_runtime_use_autosuspend()`
- `pm_runtime_set_autosuspend_delay()`
- `pm_runtime_force_suspend()`
- `pm_runtime_force_resume()`
- `DEFINE_RUNTIME_DEV_PM_OPS()` where available
- `device_init_wakeup()`
- `device_may_wakeup()`
- `enable_irq_wake()`
- `disable_irq_wake()`

Validate helper availability against your target kernel headers before turning this into real code. Older kernels may use `SET_RUNTIME_PM_OPS()` or `DEFINE_SIMPLE_DEV_PM_OPS()` patterns instead of newer convenience macros.

## Files
| File | Purpose |
| --- | --- |
| `README.md` | Runtime PM skeleton, system sleep skeleton, commands, expected logs, and cleanup/error-path notes. |

No `Makefile` or `.c` file is included because this example is intentionally not a loadable fake hardware driver.

## Build
There is nothing to build.

For a real driver derived from this skeleton, build against the exact target kernel:

```bash
uname -r
ls /lib/modules/$(uname -r)/build
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

## Driver Skeleton
This pseudo-code shows the shape of a device that can be runtime suspended after 500 ms of inactivity. It uses one imaginary register block and one optional wake IRQ.

```c
struct demo_pm {
    struct device *dev;
    void __iomem *base;
    int wake_irq;
    bool wake_irq_enabled;
};

static int demo_pm_hw_on(struct device *dev)
{
    /*
     * Production code would enable regulators, prepare/enable clocks,
     * select the default pinctrl state, deassert reset, wait for the
     * device, and restore volatile registers.
     */
    dev_dbg(dev, "hardware on\n");
    return 0;
}

static void demo_pm_hw_off(struct device *dev)
{
    /*
     * Production code would stop DMA, mask device interrupts, select
     * sleep pinctrl state, assert reset if needed, then disable clocks
     * and regulators in reverse order.
     */
    dev_dbg(dev, "hardware off\n");
}

static int demo_pm_runtime_resume(struct device *dev)
{
    return demo_pm_hw_on(dev);
}

static int demo_pm_runtime_suspend(struct device *dev)
{
    /*
     * All users, transfers, work items, timers, and IRQ paths must already
     * be stopped or protected before the hardware is powered down.
     */
    demo_pm_hw_off(dev);
    return 0;
}

static int demo_pm_runtime_idle(struct device *dev)
{
    pm_runtime_mark_last_busy(dev);
    return pm_runtime_autosuspend(dev);
}

static int demo_pm_suspend(struct device *dev)
{
    struct demo_pm *priv = dev_get_drvdata(dev);
    int ret;

    /*
     * Stop subsystem activity here: block new opens/requests, stop queues,
     * cancel or flush delayed work, stop streaming, and ensure no path can
     * touch registers without first runtime-resuming the device.
     */
    if (priv->wake_irq >= 0 && device_may_wakeup(dev)) {
        ret = enable_irq_wake(priv->wake_irq);
        if (ret)
            return ret;
        priv->wake_irq_enabled = true;
    }

    ret = pm_runtime_force_suspend(dev);
    if (ret && priv->wake_irq_enabled) {
        disable_irq_wake(priv->wake_irq);
        priv->wake_irq_enabled = false;
    }

    return ret;
}

static int demo_pm_resume(struct device *dev)
{
    struct demo_pm *priv = dev_get_drvdata(dev);
    int ret;

    ret = pm_runtime_force_resume(dev);
    if (ret)
        return ret;

    if (priv->wake_irq_enabled) {
        disable_irq_wake(priv->wake_irq);
        priv->wake_irq_enabled = false;
    }

    /*
     * Re-enable subsystem activity after hardware state is coherent.
     */
    return 0;
}

static const struct dev_pm_ops demo_pm_ops = {
    RUNTIME_PM_OPS(demo_pm_runtime_suspend,
                   demo_pm_runtime_resume,
                   demo_pm_runtime_idle)
    SYSTEM_SLEEP_PM_OPS(demo_pm_suspend, demo_pm_resume)
};

static int demo_pm_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct demo_pm *priv;
    int ret;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->dev = dev;
    priv->base = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(priv->base))
        return PTR_ERR(priv->base);

    priv->wake_irq = platform_get_irq_optional(pdev, 0);
    if (priv->wake_irq == -ENXIO)
        priv->wake_irq = -1;
    else if (priv->wake_irq < 0)
        return priv->wake_irq;

    platform_set_drvdata(pdev, priv);
    device_init_wakeup(dev, priv->wake_irq >= 0);

    ret = demo_pm_hw_on(dev);
    if (ret)
        return ret;

    pm_runtime_get_noresume(dev);
    pm_runtime_set_active(dev);
    pm_runtime_set_autosuspend_delay(dev, 500);
    pm_runtime_use_autosuspend(dev);
    pm_runtime_enable(dev);

    /*
     * Register with the real subsystem only after hardware and PM state are
     * coherent. After this point, user paths must call
     * pm_runtime_resume_and_get() before register access.
     */
    dev_info(dev, "runtime PM enabled\n");

    pm_runtime_mark_last_busy(dev);
    pm_runtime_put_autosuspend(dev);
    return 0;
}

static void demo_pm_remove(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct demo_pm *priv = platform_get_drvdata(pdev);
    int ret;

    /*
     * Unregister subsystem users first, then prevent autosuspend work from
     * racing with teardown.
     */
    ret = pm_runtime_resume_and_get(dev);
    if (ret < 0)
        dev_warn(dev, "failed to runtime resume for remove: %d\n", ret);

    pm_runtime_disable(dev);
    if (!ret)
        pm_runtime_put_noidle(dev);

    if (priv->wake_irq_enabled)
        disable_irq_wake(priv->wake_irq);

    device_init_wakeup(dev, false);
    demo_pm_hw_off(dev);
}
```

For real code, include the PM ops in the driver:

```c
static struct platform_driver demo_pm_driver = {
    .probe = demo_pm_probe,
    .remove = demo_pm_remove,
    .driver = {
        .name = "demo_pm",
        .pm = pm_ptr(&demo_pm_ops),
    },
};
```

## Active-Path Pattern
Any path that touches registers should look like this:

```c
static int demo_pm_read_status(struct demo_pm *priv)
{
    struct device *dev = priv->dev;
    u32 status;
    int ret;

    ret = pm_runtime_resume_and_get(dev);
    if (ret)
        return ret;

    status = readl(priv->base + DEMO_STATUS);

    pm_runtime_mark_last_busy(dev);
    pm_runtime_put_autosuspend(dev);

    return status;
}
```

Use `pm_runtime_resume_and_get()` for checked paths because it avoids a common `pm_runtime_get_sync()` trap: on error, `pm_runtime_get_sync()` can leave the usage count incremented unless the caller balances it.

## Load / Run
There is no module to load.

There is also no module to unload. If you turn this skeleton into a real module,
the normal flow would be:

```bash
sudo insmod demo_pm.ko
dmesg | tail -n 30
sudo rmmod demo_pm
dmesg | tail -n 30
```

On a real target with a PM-capable driver, identify the device path:

```bash
find /sys/devices -path '*/power/runtime_status' | head
```

Inspect runtime PM state:

```bash
DEV=/sys/devices/path/to/device
cat "$DEV/power/runtime_status"
cat "$DEV/power/control"
cat "$DEV/power/runtime_usage" 2>/dev/null || true
cat "$DEV/power/autosuspend_delay_ms" 2>/dev/null || true
```

Force the device to stay active for comparison:

```bash
echo on | sudo tee "$DEV/power/control"
```

Allow runtime PM again:

```bash
echo auto | sudo tee "$DEV/power/control"
```

If the device is wakeup-capable:

```bash
cat "$DEV/power/wakeup"
echo enabled | sudo tee "$DEV/power/wakeup"
```

## Quick Test Checklist
Run these on real hardware after choosing `DEV=/sys/devices/path/to/device`.

```bash
cat "$DEV/power/runtime_status"
echo on | sudo tee "$DEV/power/control"
cat "$DEV/power/runtime_status"
echo auto | sudo tee "$DEV/power/control"
sleep 1
cat "$DEV/power/runtime_status"
```

Expected result:

- With `power/control=on`, the device should remain active if runtime PM is supported.
- With `power/control=auto`, the device may autosuspend after its delay if no users are active.
- If it never suspends, inspect usage count, active children, open users, and driver get/put balance.

## System Sleep Debug Workflow
First check supported sleep states:

```bash
cat /sys/power/state
cat /sys/power/mem_sleep 2>/dev/null || true
cat /sys/power/disk 2>/dev/null || true
```

Use `pm_test` to test suspend phases without fully entering the hardware sleep state:

```bash
cat /sys/power/pm_test
echo devices | sudo tee /sys/power/pm_test
echo mem | sudo tee /sys/power/state
echo none | sudo tee /sys/power/pm_test
```

Common `pm_test` values, if supported:

- `freezer`: test task freezing.
- `devices`: test device suspend/resume callbacks.
- `platform`: include platform firmware hooks.
- `processors`: include CPU hotplug.
- `core`: include deeper core suspend path.

Watch logs while testing:

```bash
sudo dmesg -w
```

Useful boot arguments during bring-up:

```text
no_console_suspend initcall_debug
```

Use them for diagnosis, not as a production fix.

## Wakeup Debugging
Mount debugfs if needed:

```bash
sudo mount -t debugfs none /sys/kernel/debug 2>/dev/null || true
```

Inspect wakeup sources:

```bash
cat /sys/kernel/debug/wakeup_sources
find /sys/devices -path '*/power/wakeup' -print
```

If the system immediately wakes from suspend, check:

- Which wakeup source counter increased.
- Whether a level-triggered IRQ was still asserted.
- Whether the driver cleared stale interrupt status before suspend.
- Whether the driver enabled wake only when `device_may_wakeup(dev)` was true.
- Whether the driver confused `IRQF_NO_SUSPEND` with wakeup capability.

## Trace Commands
If tracefs and power events are available:

```bash
sudo mount -t tracefs none /sys/kernel/tracing 2>/dev/null || true
cd /sys/kernel/tracing
echo 0 | sudo tee tracing_on
echo nop | sudo tee current_tracer
echo 1 | sudo tee events/power/suspend_resume/enable
echo 1 | sudo tee tracing_on
echo devices | sudo tee /sys/power/pm_test
echo mem | sudo tee /sys/power/state
echo 0 | sudo tee tracing_on
sudo cat trace
echo none | sudo tee /sys/power/pm_test
```

Event names vary by kernel configuration. List available events with:

```bash
find /sys/kernel/tracing/events -maxdepth 2 -type d | grep -E '/(power|rpm|suspend)'
```

## Expected Output / Logs
For a real driver using this pattern, successful probe might look like:

```text
demo_pm 10000000.demo: runtime PM enabled
```

Runtime suspend/resume with dynamic debug enabled might look like:

```text
demo_pm 10000000.demo: hardware off
demo_pm 10000000.demo: hardware on
```

`runtime_status` usually reports one of:

```text
active
suspended
suspending
resuming
```

Supported system sleep states are board- and kernel-dependent. Example:

```text
freeze mem disk
```

## Cleanup
Return modified sysfs knobs to normal policy after experiments:

```bash
echo auto | sudo tee "$DEV/power/control"
echo none | sudo tee /sys/power/pm_test
```

Disable wakeup again if you enabled it only for testing:

```bash
echo disabled | sudo tee "$DEV/power/wakeup"
```

If trace events were enabled:

```bash
echo 0 | sudo tee /sys/kernel/tracing/tracing_on
echo 0 | sudo tee /sys/kernel/tracing/events/power/suspend_resume/enable
```

## Error-Path Explanation
Important cleanup rules:

- If probe turns hardware on before enabling runtime PM, a later failure must turn the hardware off.
- After `pm_runtime_enable()`, remove and probe-error paths must prevent new users before disabling runtime PM.
- Enabled clocks, regulators, resets, DMA channels, IRQs, wake IRQs, and work items are not magically balanced by runtime PM.
- `pm_runtime_resume_and_get()` must be paired with a put helper after successful access.
- Use `pm_runtime_mark_last_busy()` before `pm_runtime_put_autosuspend()` when autosuspend should wait after the last transaction.
- System suspend must quiesce users and work before callbacks power down hardware.
- Wake IRQ enablement should follow userspace policy via `device_may_wakeup(dev)`.

## Userspace ABI Impact
This example creates no userspace ABI.

The debug workflow uses existing kernel PM ABI:

- `/sys/devices/.../power/control`
- `/sys/devices/.../power/runtime_status`
- `/sys/devices/.../power/runtime_usage`
- `/sys/devices/.../power/autosuspend_delay_ms`
- `/sys/devices/.../power/wakeup`
- `/sys/power/state`
- `/sys/power/mem_sleep`
- `/sys/power/disk`
- `/sys/power/pm_test`
- `/sys/kernel/debug/wakeup_sources`
- `/sys/kernel/tracing/events/power/*`

Do not design production applications around debugfs or tracefs output; those are diagnostics, not stable product ABI.

## Why This Is Not Production-Ready
Production PM code must be tied to a real datasheet, real bindings, and measured board behavior. A production driver would add:

- Real clock, regulator, reset, pinctrl, interconnect, and power-domain handling.
- Clear locking around state touched by file operations, IRQs, workqueues, runtime PM, system PM, and remove.
- Subsystem-specific quiesce and restart logic.
- Correct DMA shutdown and resume ordering.
- Proper wake IRQ request and interrupt-status clearing.
- Handling for parent/child ordering, device links, and runtime-suspended devices during system sleep.
- Testing with `power/control=on`, `power/control=auto`, repeated suspend/resume cycles, wakeup-source tests, unbind/remove races, and real power measurement.
