# 38 - Production Driver Checklist And Interview Review Example

## Status

**Learning-only review exercise.** This example is not production-ready code and does not build a kernel module. It teaches how to review a driver-shaped probe/remove flow and how to run basic local checks on a real target driver.

## Goal

Practice reviewing a driver for production risks before thinking "it probes, so it is done."

You will check:

- probe ordering;
- error-path cleanup;
- remove ordering;
- async callback lifetime;
- userspace ABI impact;
- runtime PM balance;
- debug commands for bring-up.

## Kernel Version Assumptions

- Concepts apply broadly to modern Linux kernels.
- API signatures can differ by kernel version, especially bus `.remove` callbacks, subsystem registration helpers, tasklet usage, and some PM helpers.
- Validate against the target kernel headers before copying patterns into real code.

## Files

- `README.md`: review exercise, commands, expected observations, and cleanup reasoning.
- No `Makefile`: no kernel module is built in this example.
- No `.c` file: the code below is intentionally pseudo-code for review practice.

## Review Exercise

Here is a deliberately incomplete probe/remove sketch. Read it like a reviewer and find the production bugs.

```c
static int demo_probe(struct platform_device *pdev)
{
	struct demo *d;
	int irq;
	int ret;

	d = devm_kzalloc(&pdev->dev, sizeof(*d), GFP_KERNEL);
	if (!d)
		return -ENOMEM;

	platform_set_drvdata(pdev, d);
	INIT_WORK(&d->work, demo_work);

	d->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(d->base))
		return PTR_ERR(d->base);

	ret = demo_register_userspace_node(d);
	if (ret)
		return ret;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return irq;

	ret = devm_request_threaded_irq(&pdev->dev, irq, NULL,
					demo_irq_thread,
					IRQF_ONESHOT,
					dev_name(&pdev->dev), d);
	if (ret)
		return ret;

	ret = demo_hw_start(d);
	if (ret)
		return ret;

	pm_runtime_enable(&pdev->dev);
	return 0;
}

static void demo_remove(struct platform_device *pdev)
{
	struct demo *d = platform_get_drvdata(pdev);

	pm_runtime_disable(&pdev->dev);
	demo_hw_stop(d);
	demo_unregister_userspace_node(d);
}
```

## Expected Findings

The sketch should trigger these review comments:

- Userspace is registered before IRQ request and hardware start complete.
- Several failures after `demo_register_userspace_node()` leak the userspace-visible node.
- `demo_hw_start()` failure does not unregister the userspace node.
- Runtime PM is enabled after hardware start without showing coherent initial PM state.
- Remove disables PM before proving no userspace or IRQ path needs hardware access.
- Remove unregisters userspace after stopping hardware, so userspace may call into a dead device.
- Work is initialized but never canceled with `cancel_work_sync()`.
- IRQ is managed by devres, but hardware interrupt generation is not explicitly stopped before teardown.
- There is no locking rule for shared state touched by userspace, IRQ thread, and work.
- There is no clear error log or `dev_err_probe()`-style reporting for resource failures.

## Improved Shape

This is still pseudo-code, but the ownership order is safer:

```c
static int demo_probe(struct platform_device *pdev)
{
	struct demo *d;
	int irq;
	int ret;

	d = devm_kzalloc(&pdev->dev, sizeof(*d), GFP_KERNEL);
	if (!d)
		return -ENOMEM;

	d->dev = &pdev->dev;
	mutex_init(&d->lock);
	INIT_WORK(&d->work, demo_work);
	platform_set_drvdata(pdev, d);

	d->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(d->base))
		return PTR_ERR(d->base);

	ret = demo_power_on(d);
	if (ret)
		return ret;

	ret = demo_hw_init(d);
	if (ret)
		goto err_power_off;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		ret = irq;
		goto err_hw_stop;
	}

	ret = devm_request_threaded_irq(&pdev->dev, irq, NULL,
					demo_irq_thread,
					IRQF_ONESHOT,
					dev_name(&pdev->dev), d);
	if (ret)
		goto err_hw_stop;

	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	ret = demo_register_userspace_node(d);
	if (ret)
		goto err_disable_pm;

	return 0;

err_disable_pm:
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
err_hw_stop:
	demo_hw_stop(d);
err_power_off:
	demo_power_off(d);
	return ret;
}

static void demo_remove(struct platform_device *pdev)
{
	struct demo *d = platform_get_drvdata(pdev);

	demo_unregister_userspace_node(d);
	demo_mask_device_irqs(d);
	cancel_work_sync(&d->work);

	pm_runtime_get_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	demo_hw_stop(d);
	demo_power_off(d);
}
```

## Build Command

No build command is required because this example does not include kernel module code.

For a real target driver, use the target kernel build system, for example:

```bash
make -C /path/to/linux M=$PWD modules
```

For in-tree driver review, useful compile checks are usually run from the kernel tree:

```bash
make W=1 drivers/path/to/driver.o
make C=1 drivers/path/to/driver.o
```

`W=1` enables extra warnings. `C=1` runs sparse when configured.

## Load / Unload / Test Commands

There is no module to load for this example.

For a real platform, PCI, I2C, or SPI driver, adapt these workflows carefully:

```bash
# Watch probe/remove logs.
dmesg -w

# Check module aliases and parameters.
modinfo <module_name>

# Load and unload when safe for the target system.
sudo modprobe <module_name>
sudo modprobe -r <module_name>

# For bus drivers that support manual bind/unbind, inspect paths first.
ls /sys/bus/platform/drivers/
ls /sys/bus/pci/drivers/
ls /sys/bus/i2c/drivers/
```

Manual bind/unbind is hardware-specific and can disrupt the system. Use only on a disposable development target:

```bash
# Example shape only. Replace both fields with real values from sysfs.
echo <device_name> | sudo tee /sys/bus/platform/drivers/<driver_name>/unbind
echo <device_name> | sudo tee /sys/bus/platform/drivers/<driver_name>/bind
```

## Debug Commands

These commands help collect evidence while reviewing a real driver:

```bash
# Kernel logs.
dmesg -w

# IRQ activity.
cat /proc/interrupts

# MMIO resource reservations.
cat /proc/iomem

# GPIO, clock, and regulator summaries when debugfs is available.
sudo mount -t debugfs none /sys/kernel/debug 2>/dev/null || true
cat /sys/kernel/debug/gpio 2>/dev/null
cat /sys/kernel/debug/clk/clk_summary 2>/dev/null
cat /sys/kernel/debug/regulator/regulator_summary 2>/dev/null

# Runtime PM state for a specific device.
find /sys/devices -path '*/power/runtime_status' -print 2>/dev/null | head
```

Dynamic debug and ftrace are useful when the target kernel enables them:

```bash
# Enable dynamic debug for one module if available.
echo 'module <module_name> +p' | sudo tee /proc/dynamic_debug/control

# Function graph tracing around a specific driver function.
sudo mount -t tracefs nodev /sys/kernel/tracing 2>/dev/null || true
cd /sys/kernel/tracing
echo 0 | sudo tee tracing_on
echo function_graph | sudo tee current_tracer
echo '<driver_function_name>' | sudo tee set_ftrace_filter
echo 1 | sudo tee tracing_on
# Run the operation: probe, unbind, userspace I/O, suspend/resume.
echo 0 | sudo tee tracing_on
sudo cat trace
```

## Expected Output / Logs

For the pseudo-code review, expected output is a written review, not kernel logs. A good answer should mention:

```text
FAIL: userspace node is registered before driver is fully ready.
FAIL: error path after userspace registration leaks visible ABI.
FAIL: remove stops hardware before unregistering userspace.
FAIL: work can run after remove unless cancel_work_sync() is used.
FAIL: runtime PM state and get/put balance are not proven.
```

For a real target driver, useful logs look like:

```text
<driver> <device>: probing
<driver> <device>: using irq <n>
<driver> <device>: registered <subsystem object>
<driver> <device>: removing
<driver> <device>: unregistered <subsystem object>
```

Noisy repeated logs, missing device identity, or errors without resource names are review smells.

## Cleanup / Error-Path Explanation

The improved pseudo-code follows this rule:

```text
Acquire forward:
  private state -> MMIO -> power -> hardware init -> IRQ -> PM -> userspace visibility

Unwind backward:
  userspace visibility -> PM -> hardware stop -> power off

Remove:
  unregister userspace first -> stop new IRQ/work -> synchronize callbacks -> disable PM -> stop hardware
```

Important details:

- `demo_register_userspace_node()` is last because it allows userspace callbacks.
- `demo_unregister_userspace_node()` is first in remove because it blocks new users.
- `cancel_work_sync()` waits for queued work that may touch `struct demo`.
- Runtime PM is disabled after external users are gone and before final hardware shutdown.
- Devres will release managed memory and IRQ resources later, but driver-specific ordering still belongs in remove.

## Userspace ABI Impact

The pseudo-code refers to `demo_register_userspace_node()`, so it has a userspace ABI impact if implemented as a real driver.

Review questions:

- Is this a standard subsystem ABI instead of a private interface?
- If it creates `/dev/...`, what are `open`, `release`, `read`, `write`, `poll`, and `ioctl` semantics?
- If it creates sysfs, are attributes simple, documented, and stable?
- If it creates debugfs, is it clearly debug-only and not required by products?
- What should userspace observe if the device is removed while open?

For production, do not expose a new ABI until its lifetime, permissions, input validation, and compatibility story are clear.

## Why This Is Not Production-Ready

This example is intentionally not production-ready because:

- it uses pseudo-functions instead of real subsystem registration;
- it does not include a binding, DTS, or hardware datasheet assumptions;
- it does not prove open-file lifetime;
- it does not implement PM callbacks;
- it does not handle DMA, mmap, or ioctl details;
- it does not compile against a target kernel.

Its purpose is to train review thinking before applying the same checklist to a real driver.
