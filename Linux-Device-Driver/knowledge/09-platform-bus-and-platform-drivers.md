# 09 - Platform Bus And Platform Drivers

## Learning Goal

After this chapter, you should be able to explain why the Linux kernel has a platform bus, how a `platform_device` binds to a `platform_driver`, and how a real driver uses `probe()` to acquire resources, initialize hardware, and register with the right subsystem.

By the end, you should be able to:

- Explain what the **platform bus** is and why it exists for non-discoverable devices.
- Distinguish platform devices from PCI/USB devices and from I2C/SPI client devices.
- Describe how `struct platform_device`, `struct platform_driver`, and the driver model fit together.
- Walk through platform matching: firmware match, ID table match, and name fallback.
- Implement a clean `probe()`/`remove()` flow using resources and `devm_*`.
- Retrieve MMIO, IRQ, DMA, clock, GPIO, and custom configuration from the device object.
- Debug a driver that loads but never probes, probes but cannot find resources, or keeps deferring.
- Recognize interview traps around `probe()`, `module_init()`, platform data, Device Tree, and resource lifetime.

## Why This Matters In Real Work

Most Embedded Linux drivers are not for plug-and-play PCI cards or USB devices. They are for SoC blocks, board-level controllers, FPGA registers, GPIO controllers, timers, watchdogs, display controllers, DMA engines, and other hardware the CPU cannot automatically enumerate.

The platform bus solves a common embedded problem:

- The hardware exists at a known address or is described by firmware.
- The kernel needs a `struct device` for it.
- The driver needs a normal driver-model `probe()` entry point.
- Resources such as MMIO ranges, IRQs, clocks, resets, GPIOs, and DMA channels must be attached to that device.
- The same driver should remain board-agnostic while each board supplies its own hardware description.

Common places you will see platform drivers:

| Hardware / subsystem | Why it is often a platform driver |
| --- | --- |
| UART, I2C, SPI, GPIO, PWM, watchdog controllers | Integrated into the SoC and described by firmware. |
| Display, camera, audio, DMA, crypto engines | MMIO IP blocks with IRQs, clocks, resets, and DMA. |
| FPGA or custom memory-mapped blocks | Not discoverable by a standard enumerable bus. |
| MFD child devices | Parent driver creates child platform devices for sub-functions. |
| Board glue drivers | Small board-specific integration points, often DT-matched. |

**Production rule:** a platform driver should not hardcode board addresses. It should get hardware description from resources, firmware properties, or subsystem helpers.

## Mental Model

The platform bus is a **software bus**. It is not a wire on the board. It is the kernel's way to reuse the normal device-driver model for hardware that cannot announce itself.

Think of the platform bus as a registry with two lists:

```text
platform bus
  devices:
    serial@02020000
    gpio@0209c000
    watchdog@020bc000

  drivers:
    imx-uart driver
    imx-gpio driver
    imx-watchdog driver

matching:
  device description + driver match table
      -> call driver probe(device)
```

Loading a platform driver only registers the driver. It does not prove that a matching device exists.

```text
insmod my_platform_driver.ko
  -> module init or module_platform_driver()
  -> platform_driver_register()
  -> driver appears under /sys/bus/platform/drivers/
  -> if a matching device already exists, probe() runs
  -> if no matching device exists, probe() does not run
```

The important separation:

| Action | What it means |
| --- | --- |
| Register a platform driver | "Here is code that can handle matching devices." |
| Register or describe a platform device | "Here is a hardware instance and its resources." |
| Match succeeds | "Call `probe()` with this specific device instance." |
| `probe()` succeeds | "This driver now owns and manages this device instance." |

**Interview trap:** `probe()` is not a replacement for module initialization. Module init happens once per module load. `probe()` happens once per matched device instance.

## Core Concepts

Platform drivers are easiest to remember by comparing them with bus types that do enumerate hardware.

| Bus / device type | How device appears | Driver type | Example |
| --- | --- | --- | --- |
| PCI | Hardware enumeration reads vendor/device IDs | `struct pci_driver` | PCI Ethernet card |
| USB | USB enumeration reads descriptors | `struct usb_driver` | USB webcam |
| I2C client | Board firmware or adapter registration creates client | `struct i2c_driver` | I2C temperature sensor |
| SPI device | Board firmware or controller registration creates SPI device | `struct spi_driver` | SPI flash |
| Platform device | Firmware, board code, MFD core, or test code creates device | `struct platform_driver` | SoC UART controller |

Important distinction:

- An **I2C controller** integrated in the SoC is often a platform device because the controller itself is an MMIO block.
- An **I2C sensor** connected to that controller is not a platform-bus device. It is an I2C client and should use an `i2c_driver`.
- An **SPI controller** may be a platform device.
- An **SPI flash chip** should use the SPI driver model.

### Device Provisioning Methods

The kernel needs a `platform_device` before a platform driver's `probe()` can run.

| Method | Use today? | Notes |
| --- | --- | --- |
| Device Tree / firmware description | Normal for embedded SoCs | Board describes nodes, `compatible`, `reg`, `interrupts`, clocks, resets, GPIOs. |
| ACPI | Common on some non-DT systems | Match through ACPI IDs and firmware resources. |
| MFD-created child devices | Common inside multi-function chips | Parent MFD driver creates platform child devices with resources. |
| Runtime-created test devices | Useful for learning or pseudo devices | Driver or helper module calls platform device creation APIs. |
| Legacy board files / platform data | Mainly old kernels or legacy code | Hardcoded C board files compiled into the kernel. |

**Production rule:** prefer firmware or subsystem-created devices. Runtime-created platform devices are fine for tests and learning, but production hardware description should usually live outside the driver.

## Kernel Mechanism

Platform drivers are built on the Linux device model. The platform bus has a `bus_type`; devices and drivers register with it; the bus match function decides whether a device and driver belong together.

### Object Relationships

```text
struct platform_device
  name
  id
  struct device dev
    of_node / fwnode / parent / drvdata
  resource[]
  num_resources
  id_entry

struct platform_driver
  probe()
  remove()
  shutdown()
  id_table
  struct device_driver driver
    name
    of_match_table
    acpi_match_table
```

`struct platform_device` is the device instance. It says, "there is one UART controller here, with these resources."

`struct platform_driver` is the driver. It says, "I can handle devices with these match strings or IDs."

`struct device` is the generic driver-model object embedded inside the platform device. It is what ties the device into sysfs, parent-child lifetime, power management, devres, firmware nodes, DMA configuration, and driver data.

### Matching Order

The exact kernel code can evolve, but the important practical order is:

```text
device and driver registered on platform bus
  -> driver_override if explicitly set
  -> firmware match, commonly OF / Device Tree
  -> ACPI match where applicable
  -> platform_device_id table
  -> device name == driver name fallback
```

For Device Tree, the match usually looks like:

```dts
uart1: serial@02020000 {
        compatible = "fsl,imx6q-uart", "fsl,imx21-uart";
        reg = <0x02020000 0x4000>;
        interrupts = <0 26 4>;
};
```

Driver side:

```c
static const struct of_device_id my_uart_of_match[] = {
        { .compatible = "fsl,imx6q-uart", .data = &imx6q_data },
        { .compatible = "fsl,imx21-uart", .data = &imx21_data },
        { }
};
MODULE_DEVICE_TABLE(of, my_uart_of_match);

static struct platform_driver my_uart_driver = {
        .probe = my_uart_probe,
        .remove = my_uart_remove,
        .driver = {
                .name = "my-uart",
                .of_match_table = my_uart_of_match,
        },
};
module_platform_driver(my_uart_driver);
```

For legacy or non-DT device creation, platform ID matching may look like:

```c
static const struct platform_device_id my_uart_ids[] = {
        { .name = "my-uart-v1", .driver_data = UART_V1 },
        { .name = "my-uart-v2", .driver_data = UART_V2 },
        { }
};
MODULE_DEVICE_TABLE(platform, my_uart_ids);
```

**Production rule:** keep `MODULE_DEVICE_TABLE()` for match tables that should support module autoloading.

### Resources

A platform device carries hardware resources in a common form:

```c
struct resource {
        resource_size_t start;
        resource_size_t end;
        const char *name;
        unsigned long flags;
        ...
};
```

Common resource types:

| Resource | Meaning | Typical getter |
| --- | --- | --- |
| `IORESOURCE_MEM` | MMIO physical address range | `platform_get_resource()` |
| `IORESOURCE_IRQ` | IRQ line | `platform_get_irq()` |
| `IORESOURCE_DMA` | DMA channel/request line in older style resources | `platform_get_resource()` or DMA engine APIs |
| named memory | Multiple MMIO regions | `platform_get_resource_byname()` |
| named IRQ | Multiple interrupts | `platform_get_irq_byname()` |

With Device Tree, `reg` and `interrupts` are converted into platform resources so the driver can still use platform helpers:

```text
DTS reg property
  -> platform MMIO resource
  -> platform_get_resource(pdev, IORESOURCE_MEM, 0)
  -> devm_ioremap_resource()

DTS interrupts property
  -> platform IRQ resource or IRQ mapping
  -> platform_get_irq(pdev, 0)
  -> devm_request_irq()
```

This is why platform drivers can stay mostly independent of whether a device came from old board files or from Device Tree.

## Key Structs And APIs

These APIs are not a memorization list. Each group maps to a lifecycle question: register the driver, find the device's resources, attach driver state, and clean up correctly.

### Driver Registration

| API | Purpose | Notes |
| --- | --- | --- |
| `platform_driver_register()` | Register a platform driver with the bus. | Driver remains registered for later matches and deferred probe. |
| `platform_driver_unregister()` | Unregister the driver. | Triggers unbind/remove for bound devices. |
| `module_platform_driver(driver)` | Boilerplate macro for simple modules. | Replaces hand-written `module_init()` and `module_exit()`. |
| `platform_driver_probe()` | Register and probe immediately, then do not keep normal deferred behavior. | Use only when device presence and supplier readiness are guaranteed. |

Normal pattern:

```c
static struct platform_driver foo_driver = {
        .probe = foo_probe,
        .remove = foo_remove,
        .driver = {
                .name = "foo",
                .of_match_table = foo_of_match,
        },
};

module_platform_driver(foo_driver);
```

### Device Creation For Learning Or Legacy Code

| API | Purpose |
| --- | --- |
| `platform_device_register()` | Register a statically described `platform_device`. |
| `platform_add_devices()` | Register an array of platform devices. |
| `platform_device_alloc()` | Allocate a platform device dynamically. |
| `platform_device_add_resources()` | Attach resource array to an allocated device. |
| `platform_device_add_data()` | Attach platform data to an allocated device. |
| `platform_device_add()` | Publish the allocated platform device. |
| `platform_device_unregister()` | Remove and release a registered platform device. |
| `platform_device_put()` | Drop an allocated-but-not-added device on error. |

For production board hardware, these are usually hidden behind firmware parsing or subsystem code. You still need to recognize them when reading old code, tests, and MFD internals.

### Resource And MMIO APIs

| API | Use |
| --- | --- |
| `platform_get_resource(pdev, IORESOURCE_MEM, index)` | Get an MMIO resource by type and index. |
| `platform_get_resource_byname()` | Get named MMIO resource. |
| `resource_size(res)` | Compute size from `start` and `end`. |
| `devm_ioremap_resource(&pdev->dev, res)` | Request and map MMIO resource with managed cleanup. |
| `devm_platform_ioremap_resource(pdev, index)` | Common newer shortcut for indexed MMIO resource mapping. |
| `platform_get_irq(pdev, index)` | Get IRQ number by index. |
| `platform_get_irq_byname()` | Get named IRQ. |
| `devm_request_irq()` | Request IRQ with managed cleanup. |
| `devm_request_threaded_irq()` | Request top half plus threaded handler. |

Prefer managed helpers when they fit. They reduce error-path noise and attach cleanup to `struct device`.

### Per-Device State APIs

Every `probe()` call needs its own private state. Do not use one global state object unless the hardware is truly singleton and the design is intentional.

```c
struct foo {
        struct device *dev;
        void __iomem *base;
        int irq;
        spinlock_t lock;
};
```

Useful helpers:

| API | Purpose |
| --- | --- |
| `devm_kzalloc(&pdev->dev, size, GFP_KERNEL)` | Allocate state tied to device lifetime. |
| `platform_set_drvdata(pdev, data)` | Store driver state in `pdev->dev.driver_data`. |
| `platform_get_drvdata(pdev)` | Retrieve state in `remove()` or callbacks. |
| `dev_get_drvdata(dev)` / `dev_set_drvdata(dev, data)` | Generic forms for `struct device`. |

### Firmware And Match Data APIs

| API / field | Purpose |
| --- | --- |
| `struct of_device_id` | Device Tree compatible match table. |
| `.of_match_table` | Field in embedded `struct device_driver`. |
| `of_match_device()` | Find the OF table entry that matched this device. |
| `.data` in match entry | Per-compatible driver data. |
| `struct platform_device_id` | Platform ID table for non-DT/name-based devices. |
| `platform_get_device_id()` | Get matching platform ID table entry. |
| `dev_get_platdata()` | Get legacy platform data. |

**Production rule:** use match data for hardware variants instead of scattering `if (compatible == "...")` throughout the driver.

## Lifecycle / Data Flow

A platform driver's lifecycle is the story of one device instance binding to one driver instance.

```text
1. Firmware, board code, MFD core, or test code creates a platform_device
   - name / firmware node
   - resources
   - parent device
   - optional platform data

2. Driver registers a platform_driver
   - probe/remove callbacks
   - driver name
   - match tables

3. Platform bus matches device and driver
   - OF / ACPI / ID table / name fallback

4. Kernel calls probe(pdev)
   - allocate private state
   - get match data
   - get MMIO resource and map registers
   - get IRQs, clocks, resets, GPIOs, regulators, DMA
   - initialize locks, workqueues, hardware
   - platform_set_drvdata()
   - request IRQs
   - register with a subsystem, such as input, netdev, tty, IIO, watchdog
   - return 0

5. Device is live
   - subsystem callbacks, IRQ handlers, workqueues, runtime PM, userspace entry points

6. Remove / unbind / driver unload
   - unregister from subsystem
   - stop hardware and DMA
   - disable IRQ activity and cancel work
   - release explicit resources
   - devm resources are released by device core
```

### Probe Ordering

A practical `probe()` order:

1. Get `struct device *dev = &pdev->dev`.
2. Allocate private state with `devm_kzalloc()`.
3. Save `dev` and initialize locks/completions/work structures.
4. Read match data or legacy platform data.
5. Map registers.
6. Get clocks, resets, regulators, GPIO descriptors, DMA channels.
7. Bring hardware to a known state.
8. Store private data with `platform_set_drvdata()` once callbacks can safely find it.
9. Request IRQs only when the handler can safely run.
10. Register with the target kernel subsystem.
11. Return success.

The exact order may change by subsystem. The key is that each later step can safely observe what earlier steps made visible.

**Warning:** requesting an IRQ before state and hardware are ready can let the handler run against partially initialized data.

### Remove Ordering

A practical `remove()` order:

1. Get private state using `platform_get_drvdata()`.
2. Stop userspace-visible or subsystem-visible entry points.
3. Disable hardware interrupts and DMA.
4. Cancel workqueues/timers/threaded activity.
5. Put hardware into idle or reset state.
6. Unregister subsystem objects if not devm-managed, or if ordering matters.
7. Let devm resources unwind.

`devm_*` frees resources, but it does not replace design thinking. If userspace can still reach a subsystem object, you must unregister that object before the private state disappears.

## Minimal Practical Example

This is a **learning-only** skeleton for an MMIO platform driver. It shows shape and ordering, not a complete production driver.

```c
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

struct demo_variant {
        u32 fifo_depth;
        bool has_dma;
};

struct demo_dev {
        struct device *dev;
        void __iomem *base;
        int irq;
        const struct demo_variant *variant;
        spinlock_t lock;
};

static const struct demo_variant demo_v1 = {
        .fifo_depth = 16,
        .has_dma = false,
};

static const struct demo_variant demo_v2 = {
        .fifo_depth = 64,
        .has_dma = true,
};

static irqreturn_t demo_irq(int irq, void *data)
{
        struct demo_dev *demo = data;
        unsigned long flags;
        u32 status;

        spin_lock_irqsave(&demo->lock, flags);
        status = readl(demo->base + 0x00);
        writel(status, demo->base + 0x00); /* acknowledge example */
        spin_unlock_irqrestore(&demo->lock, flags);

        return IRQ_HANDLED;
}

static int demo_probe(struct platform_device *pdev)
{
        struct device *dev = &pdev->dev;
        struct demo_dev *demo;
        struct resource *res;
        int ret;

        demo = devm_kzalloc(dev, sizeof(*demo), GFP_KERNEL);
        if (!demo)
                return -ENOMEM;

        demo->dev = dev;
        spin_lock_init(&demo->lock);

        demo->variant = of_device_get_match_data(dev);
        if (!demo->variant)
                return -ENODEV;

        res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        demo->base = devm_ioremap_resource(dev, res);
        if (IS_ERR(demo->base))
                return PTR_ERR(demo->base);

        demo->irq = platform_get_irq(pdev, 0);
        if (demo->irq < 0)
                return demo->irq;

        platform_set_drvdata(pdev, demo);

        ret = devm_request_irq(dev, demo->irq, demo_irq, 0,
                               dev_name(dev), demo);
        if (ret)
                return ret;

        dev_info(dev, "demo platform device probed, fifo=%u dma=%d\n",
                 demo->variant->fifo_depth, demo->variant->has_dma);

        return 0;
}

static int demo_remove(struct platform_device *pdev)
{
        struct demo_dev *demo = platform_get_drvdata(pdev);

        /* Unregister subsystem objects and quiesce hardware here. */
        dev_info(demo->dev, "demo platform device removed\n");
        return 0;
}

static const struct of_device_id demo_of_match[] = {
        { .compatible = "vendor,demo-v1", .data = &demo_v1 },
        { .compatible = "vendor,demo-v2", .data = &demo_v2 },
        { }
};
MODULE_DEVICE_TABLE(of, demo_of_match);

static struct platform_driver demo_driver = {
        .probe = demo_probe,
        .remove = demo_remove,
        .driver = {
                .name = "demo-platform",
                .of_match_table = demo_of_match,
        },
};
module_platform_driver(demo_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Learning-only platform driver skeleton");
```

Matching Device Tree sketch:

```dts
demo@80000000 {
        compatible = "vendor,demo-v2";
        reg = <0x80000000 0x1000>;
        interrupts = <0 56 4>;
};
```

Important lines:

| Line / pattern | Why it matters |
| --- | --- |
| `of_device_get_match_data(dev)` | Selects per-compatible hardware data. |
| `platform_get_resource()` | Gets the MMIO range attached to this device. |
| `devm_ioremap_resource()` | Requests and maps registers, with managed cleanup. |
| `platform_get_irq()` | Gets the IRQ mapped for this device. |
| `platform_set_drvdata()` | Makes private state recoverable in remove and other paths. |
| `devm_request_irq()` | Registers handler after state is initialized. |
| `MODULE_DEVICE_TABLE(of, ...)` | Enables module alias generation for autoloading. |

## Common Bugs And Debugging

Start from the symptom. Platform-driver bugs are often matching, resource, or lifetime bugs.

### Driver Loads But `probe()` Never Runs

Likely causes:

- No matching platform device exists.
- Device Tree node is disabled with `status = "disabled"`.
- `compatible` string does not match the driver's OF table.
- Missing `MODULE_DEVICE_TABLE()` prevents module autoloading.
- Driver is registered on the wrong bus type.
- You wrote an I2C/SPI client driver as a platform driver or the reverse.

Inspect:

```bash
ls /sys/bus/platform/devices/
ls /sys/bus/platform/drivers/
dmesg | grep -i probe
find /proc/device-tree -name compatible -print
modinfo my_driver.ko | grep alias
cat /lib/modules/$(uname -r)/modules.alias | grep vendor
```

Fix patterns:

- Confirm the device exists under `/sys/bus/platform/devices/`.
- Confirm the driver exists under `/sys/bus/platform/drivers/`.
- Compare the exact `compatible` string.
- Add the correct `MODULE_DEVICE_TABLE(of, table)` or platform ID table.
- Make sure the DT node is enabled.

### `probe()` Runs But MMIO Mapping Fails

Likely causes:

- Missing or malformed `reg` property.
- Wrong `#address-cells` / `#size-cells` in parent bus.
- Resource index mismatch.
- Resource already claimed by another driver.
- Using a hardcoded address instead of the platform resource.

Inspect:

```bash
dmesg
cat /proc/iomem
hexdump -C /proc/device-tree/path/to/node/reg
```

Fix patterns:

- Use `platform_get_resource_byname()` when there are multiple `reg` ranges.
- Use `dev_err_probe()` or clear `dev_err()` logs around resource acquisition.
- Validate the parent bus `ranges` and address cells in DT.

### `platform_get_irq()` Returns A Negative Error

Likely causes:

- Missing `interrupts` or `interrupts-extended`.
- Wrong interrupt parent.
- IRQ domain/controller not ready yet.
- Optional interrupt handled as mandatory.
- Deferred supplier dependency.

Fix patterns:

- Return the negative error directly for mandatory IRQs.
- Treat optional IRQs explicitly with optional helper patterns or documented fallback.
- Preserve `-EPROBE_DEFER`; do not convert every failure to `-EINVAL`.

### Probe Keeps Deferring

Likely causes:

- Clock, regulator, reset controller, GPIO controller, IRQ domain, or PHY provider is not ready.
- Supplier driver is missing or failed.
- Device link ordering is waiting for another device.

Inspect:

```bash
dmesg | grep -i defer
cat /sys/kernel/debug/devices_deferred 2>/dev/null
```

Fix patterns:

- Use `dev_err_probe(dev, ret, "message\n")` for supplier acquisition failures.
- Enable missing supplier drivers.
- Fix DT phandles and provider nodes.
- Avoid `platform_driver_probe()` for devices that may need deferred probe.

### Remove Or Unbind Crashes

Likely causes:

- Workqueue, timer, threaded IRQ, DMA, or userspace-visible object still uses freed state.
- Subsystem object was not unregistered before devm cleanup.
- IRQ can still fire while hardware/state is being torn down.
- Driver frees state manually and devm also frees it.

Fix patterns:

- Quiesce hardware first.
- Unregister subsystem objects before private state disappears.
- Cancel work/timers and synchronize as needed.
- Let devm free devm-managed resources exactly once.

### Useful Debug Commands

```bash
ls -l /sys/bus/platform/devices/
ls -l /sys/bus/platform/drivers/
readlink /sys/bus/platform/devices/<device>/driver
cat /sys/bus/platform/devices/<device>/modalias
dmesg -w
modinfo <driver>.ko
cat /proc/iomem
cat /proc/interrupts
```

For deeper debugging:

- Add `dev_dbg()` and enable dynamic debug for the driver.
- Trace probe/remove with ftrace or tracepoints where available.
- Check `devices_deferred` when suppliers are involved.
- Compare DT runtime view under `/proc/device-tree` with the DTS source.

## Production Checklist

Before review or hardware bring-up, verify the boring things. That is where most driver bugs hide.

### Matching And Device Description

- The driver is registered on the correct bus.
- DT `compatible` strings match the driver's table exactly.
- `MODULE_DEVICE_TABLE()` exists for module autoloading.
- Legacy platform ID table is present only when needed.
- Hardware variant differences are represented with match data.
- The driver does not hardcode board addresses or IRQ numbers.

### Probe

- `probe()` handles multiple device instances.
- Per-device state is allocated per `pdev`, not as accidental global state.
- Locks, work items, wait queues, and state are initialized before they can be used.
- Mandatory resources return clear errors.
- Optional resources are intentionally optional.
- Supplier failures preserve `-EPROBE_DEFER`.
- IRQs are requested only after handler data is valid.
- Hardware is left in a known state before subsystem registration.

### Resource Management

- MMIO resources use `devm_ioremap_resource()` or a target-kernel equivalent.
- IRQs use managed APIs when suitable.
- Clocks, resets, regulators, GPIOs, pinctrl, DMA, and regmap are acquired through subsystem APIs.
- Explicit cleanup exists for subsystem objects, workqueues, DMA, runtime PM, and anything not fully devm-managed.
- Cleanup ordering is documented by code structure.

### Remove / Unbind

- Userspace-visible or subsystem-visible interfaces are unregistered.
- IRQ activity, DMA, timers, and workqueues are stopped.
- Hardware is disabled or put into a safe state.
- No manual free duplicates a devm-managed free.
- Remove path has been tested with bind/unbind where possible:

  ```bash
  echo <device> | sudo tee /sys/bus/platform/drivers/<driver>/unbind
  echo <device> | sudo tee /sys/bus/platform/drivers/<driver>/bind
  ```

### Debuggability

- Logs use `dev_err()`, `dev_warn()`, `dev_info()`, or `dev_dbg()` with `&pdev->dev`.
- Error paths log enough context to identify missing resources.
- `dev_err_probe()` is used where deferred probe is expected.
- The driver exposes normal subsystem debug interfaces where appropriate.

## Interview Readiness

You are ready for interviews when you can reason from a device description to a live driver instance without guessing API names.

Be able to explain:

- Why platform bus exists.
- Why a platform bus is a software abstraction, not a hardware bus.
- Why SoC I2C/SPI controllers may be platform devices, while I2C/SPI client chips use bus-specific drivers.
- What `platform_device` contains.
- What `platform_driver` contains.
- Why `probe()` may never run even though the module loaded successfully.
- How Device Tree resources become platform resources.
- Why `devm_*` simplifies cleanup but does not remove remove-path thinking.
- Why `platform_driver_probe()` can be wrong when resources may defer.
- How to debug missing match, missing resources, and deferred probe.

See `interview/09-platform-bus-and-platform-drivers.md` for structured questions and traps.

## Kernel Version Notes

Platform-driver fundamentals are stable, but examples should be checked against the target kernel headers.

- `struct platform_driver` remove callback signatures have changed across kernel versions in some trees. Verify whether your target expects `.remove`, `.remove_new`, and whether the callback returns `int` or `void`.
- Modern code may prefer `devm_platform_ioremap_resource()` for simple indexed MMIO mapping. `platform_get_resource()` plus `devm_ioremap_resource()` remains useful for teaching and named-resource flows.
- `of_match_ptr()` is version and build-configuration sensitive. In many modern drivers, keeping the OF match table directly assigned is simpler and avoids hiding match data needed for module aliases or matching.
- `class_create()` caveats matter only if you combine a platform driver with a character-device example; verify the target kernel signature before building.
