# 12 - Linux Device Model

## Learning Goal

After this chapter, you should understand how Linux represents devices, drivers, buses, classes, sysfs objects, and device lifetime in one common model.

By the end, you should be able to:

- Explain what the **Linux Device Model** is and why `struct device` is everywhere.
- Distinguish **device**, **driver**, **bus**, **class**, **subsystem**, and **kobject**.
- Trace how a device and driver bind through a bus match callback.
- Understand why `/sys/devices`, `/sys/bus`, and `/sys/class` show different views of the same system.
- Use `class_create()` and `device_create()` correctly for character-device helper nodes.
- Explain how sysfs attributes connect userspace files to kernel objects.
- Reason about lifetime, reference counts, release callbacks, `devm_*`, and remove paths.
- Debug devices that exist but do not bind, bind but do not expose `/dev`, or expose broken sysfs attributes.
- Handle kernel-version caveats around class and sysfs helper APIs.

## Why This Matters In Real Work

Almost every real Linux driver touches the device model, even if you never write a custom bus. Platform, I2C, SPI, PCI, USB, input, RTC, regulator, clock, GPIO, IIO, V4L2, and network drivers all sit on top of the same driver core ideas.

The device model solves practical driver problems:

| Problem | Device-model answer |
| --- | --- |
| How does the kernel know which driver owns which device? | Devices and drivers register on a bus; the bus performs matching and binding. |
| How does userspace discover hardware? | Registered objects appear through sysfs and uevents. |
| How does a char driver get `/dev/mydev`? | `device_create()` creates a class device with a `dev` attribute; userspace can create the node. |
| How does the kernel order suspend/resume? | Devices have parent/child relationships and PM callbacks. |
| How are resources cleaned up on remove? | Resource-managed allocations attach cleanup actions to `struct device`. |
| How do generic frameworks support many drivers? | Drivers embed generic objects and recover private objects with `container_of()`. |

**Production rule:** if your driver has a real hardware instance, userspace ABI, PM behavior, managed resources, or subsystem registration, you are already relying on the Linux Device Model.

## Mental Model

The Linux Device Model is the kernel's object graph for devices. It gives every device a common identity, a place in the hierarchy, a way to bind to a driver, a way to expose state to userspace, and a lifetime rule.

Think of one physical UART controller:

```text
Physical view:
  /sys/devices/platform/soc/2020000.serial

Bus view:
  /sys/bus/platform/devices/2020000.serial
  /sys/bus/platform/drivers/imx-uart

Class view:
  /sys/class/tty/ttymxc0

Userspace node:
  /dev/ttymxc0
```

These are not four separate devices. They are different views of related kernel objects.

The core idea:

```text
struct device
  -> belongs to a parent hierarchy
  -> sits on a bus
  -> may bind to a driver
  -> may belong to a class
  -> has a kobject, sysfs identity, and reference-counted lifetime
  -> owns managed resources through devres
```

## Core Concepts

The device model becomes much easier once the major nouns are separated.

| Concept | What it means | Common path |
| --- | --- | --- |
| Device | One hardware or virtual instance represented by `struct device`. | `/sys/devices/...` |
| Driver | Code that can manage matching devices. | `/sys/bus/<bus>/drivers/...` |
| Bus | Matching domain that connects devices to drivers. | `/sys/bus/<bus>/` |
| Class | Functional grouping visible to userspace. | `/sys/class/<class>/` |
| Subsystem/framework | Kernel layer that provides common behavior for a device family. | Varies: input, RTC, IIO, regulator, etc. |
| kobject | Low-level object used for reference counting and sysfs representation. | Usually hidden inside higher-level structs. |
| sysfs attribute | A file exposing one piece of kernel state or control. | `/sys/.../<attribute>` |
| uevent | Kernel notification to userspace that an object was added, removed, or changed. | Consumed by udev/mdev/systemd-udevd policy. |

### Device vs Driver vs Bus

This is the most important comparison.

| Question | Device | Driver | Bus |
| --- | --- | --- | --- |
| What does it represent? | One instance. | Code that handles instances. | Matching and organization domain. |
| Example | `2020000.serial` | `imx-uart` | `platform` |
| Key struct | `struct device` or bus-specific wrapper | `struct device_driver` or bus-specific wrapper | `struct bus_type` |
| Main job | Carry identity, resources, parent, lifetime. | Probe, operate, remove. | Match devices and drivers. |

```text
device appears
  -> bus checks registered drivers
  -> match?
  -> bind
  -> probe(device)
```

```text
driver appears
  -> bus checks registered devices
  -> match?
  -> bind
  -> probe(device)
```

**Interview trap:** loading a driver module does not guarantee `probe()` ran. `probe()` runs only after a matching device exists and binding succeeds.

### Physical Topology vs Class View

`/sys/devices` and `/sys/class` answer different questions.

| View | Answers | Example |
| --- | --- | --- |
| `/sys/devices` | Where is this object in the hardware or virtual hierarchy? | device parent/child chain |
| `/sys/bus` | Which bus matched this device and driver? | platform, i2c, spi, pci |
| `/sys/class` | What user-visible function does this object provide? | tty, input, rtc, video4linux |

A class is not usually the physical parent. It is a functional grouping.

### kobject, kset, and kobj_type

Most driver authors should not create raw kobjects for normal devices. Still, you need the mental model because `struct device` contains a kobject.

| Object | Role |
| --- | --- |
| `struct kobject` | Name, parent, kset membership, reference count, sysfs identity. |
| `struct kset` | Group of related kobjects; often appears as a sysfs directory. |
| `struct kobj_type` | Describes release behavior, sysfs operations, and default attributes for a kobject type. |

The higher-level driver core wraps this low-level machinery so drivers normally use `struct device`, classes, buses, and subsystem helpers.

## When To Use These Ideas

You use the device model whenever your code creates, binds, exposes, or removes a device. You do not need to write a custom bus to care about it.

Use device-model thinking when you:

- write a `probe()` and `remove()` path for platform, I2C, SPI, PCI, USB, or another bus;
- pass `&pdev->dev`, `&client->dev`, or `&spi->dev` to `devm_*`, `dev_*()`, DMA, PM, or subsystem APIs;
- create `/dev` nodes for a character driver with `class_create()` and `device_create()`;
- add sysfs attributes or attribute groups;
- debug why a driver loaded but did not bind;
- debug why userspace sees the wrong sysfs path or no `/dev` node;
- reason about suspend/resume order, runtime PM, or wakeup policy;
- review cleanup order and possible use-after-free paths.

Avoid raw device-model code when a subsystem already gives you a safer interface. For example, an input driver should normally register an `input_dev`, an RTC driver should register with the RTC framework, and a sensor driver should usually use IIO. Those frameworks still use the device model underneath.

## Kernel Mechanism

The kernel implements the device model by embedding generic objects inside bus-specific and subsystem-specific objects. This gives the driver core one common way to manage many different device families.

### Embedded Generic Objects

Common pattern:

```c
struct platform_device {
        const char *name;
        int id;
        struct device dev;
        struct resource *resource;
        unsigned int num_resources;
};

struct platform_driver {
        int (*probe)(struct platform_device *pdev);
        void (*remove)(struct platform_device *pdev);
        struct device_driver driver;
        const struct platform_device_id *id_table;
};
```

The platform bus can expose platform-specific callbacks, while the driver core still sees the embedded generic objects:

```text
struct platform_device
  contains struct device

struct platform_driver
  contains struct device_driver
```

When generic callbacks receive a `struct device *`, bus code often uses `container_of()` to recover the bus-specific object:

```c
struct platform_device *pdev = to_platform_device(dev);
struct platform_driver *pdrv = to_platform_driver(drv);
```

This same pattern appears throughout the kernel.

### Matching and Binding

Each bus provides matching rules. For platform devices, matching may use Device Tree compatible strings, ACPI IDs, platform ID tables, or name fallback. For PCI, matching uses PCI IDs. For I2C and SPI, matching uses bus-specific client information and firmware data.

Generic flow:

```text
1. Device registers with a bus.
2. Driver registers with the same bus.
3. Driver core asks the bus: does this driver support this device?
4. If match succeeds, the core binds them.
5. Probe callback runs for that specific device instance.
6. If probe returns 0, the device is live.
7. If remove/unbind/unload happens, remove callback unwinds the live instance.
```

Important consequences:

- Matching is **bus-specific**.
- Probe is **per device instance**.
- A driver can bind to multiple devices.
- A device should not be considered usable until probe succeeds.
- Remove must handle only resources that were successfully made live.

### Sysfs and Attributes

Sysfs exposes kernel objects as directories and attributes as files.

```text
struct device
  -> embedded kobject
  -> sysfs directory
  -> attributes
```

A sysfs attribute is not a normal storage file. It is a callback interface:

```c
static ssize_t enabled_show(struct device *dev,
                            struct device_attribute *attr,
                            char *buf)
{
        struct mydev *priv = dev_get_drvdata(dev);

        return sysfs_emit(buf, "%u\n", priv->enabled);
}

static ssize_t enabled_store(struct device *dev,
                             struct device_attribute *attr,
                             const char *buf,
                             size_t count)
{
        struct mydev *priv = dev_get_drvdata(dev);
        bool value;
        int ret;

        ret = kstrtobool(buf, &value);
        if (ret)
                return ret;

        mutex_lock(&priv->lock);
        priv->enabled = value;
        mutex_unlock(&priv->lock);

        return count;
}

static DEVICE_ATTR_RW(enabled);
```

Good sysfs design:

- one value, or an array of similar values, per file;
- text format, not binary blobs;
- stable names and units;
- clear locking around shared state;
- `show()` returns the number of bytes written;
- `store()` returns `count` on success or a negative errno on failure.

**Production rule:** prefer attribute groups for attributes that should exist immediately when the object is added. Userspace may react to the add uevent quickly.

### Classes and `/dev` Nodes

Character drivers often combine the char-device layer with a device-model class.

```text
alloc_chrdev_region()
  -> reserve major/minor numbers
cdev_add()
  -> register file operations for those numbers
class_create()
  -> create /sys/class/<class>
device_create()
  -> create /sys/class/<class>/<device>
  -> create "dev" attribute containing major:minor
  -> userspace can create /dev/<device>
```

`device_create()` creates a `struct device` registered to a class. It is commonly used by char drivers, but it is not the same thing as `cdev_add()`.

| API | Job |
| --- | --- |
| `cdev_add()` | Connects major/minor to `file_operations`. |
| `class_create()` | Creates a functional class under `/sys/class`. |
| `device_create()` | Creates a class device and exposes its `dev_t`. |
| userspace udev/devtmpfs policy | Creates or manages `/dev/<name>`. |

## Key Structs And APIs

The APIs are easier to remember by purpose rather than by long lists.

### Core Objects

| Struct | Why it matters |
| --- | --- |
| `struct device` | Generic representation of one device instance; owns parent, bus, driver, class, firmware node, kobject, PM state, and devres. |
| `struct device_driver` | Generic driver object embedded by bus-specific driver structs. |
| `struct bus_type` | Represents a bus and its callbacks, especially matching. |
| `struct class` | Groups devices by userspace-visible function. |
| `struct kobject` | Low-level reference-counted sysfs object. |
| `struct attribute_group` | Group of sysfs attributes, often preferred for default attributes. |

### Registration and Lifetime APIs

| API | Use |
| --- | --- |
| `device_initialize()` | Initialize a device object before adding it. |
| `device_add()` | Add an initialized device to the driver core. |
| `device_register()` | Convenience wrapper for initialize plus add. |
| `device_del()` | Remove device from driver core visibility. |
| `put_device()` | Drop a device reference; final put triggers release. |
| `device_unregister()` | Remove device and drop the initial reference. |
| `get_device()` / `put_device()` | Manage references when holding a device beyond immediate scope. |
| `driver_register()` / `driver_unregister()` | Low-level driver registration. |
| `bus_register()` / `bus_unregister()` | Register or unregister a bus type. |

**Production rule:** any manually initialized `struct device` needs a valid `.release` callback. Without it, final lifetime cleanup is broken.

### Class and Char-Device Helper APIs

| API | Use |
| --- | --- |
| `class_create()` | Create a class used with `device_create()`. |
| `class_destroy()` | Destroy a class created by `class_create()`. |
| `device_create()` | Create and register a class device, commonly for char devices. |
| `device_create_with_groups()` | Create a class device with initial attribute groups. |
| `device_destroy()` | Remove a device created by `device_create()`. |

### Sysfs Attribute APIs

| API / macro | Use |
| --- | --- |
| `DEVICE_ATTR_RO()` / `DEVICE_ATTR_WO()` / `DEVICE_ATTR_RW()` | Declare device attributes. |
| `device_create_file()` / `device_remove_file()` | Add or remove one device attribute. |
| `sysfs_create_group()` / `sysfs_remove_group()` | Add or remove a group of attributes. |
| `sysfs_notify()` | Wake userspace polling a sysfs attribute after a value changes. |
| `sysfs_emit()` / `sysfs_emit_at()` | Format data safely in `show()` callbacks. |

### Driver Data APIs

Private state usually hangs from the device.

```c
struct mydev {
        void __iomem *base;
        struct mutex lock;
};

platform_set_drvdata(pdev, priv);
priv = platform_get_drvdata(pdev);
```

For generic device callbacks:

```c
dev_set_drvdata(dev, priv);
priv = dev_get_drvdata(dev);
```

### Managed Resource APIs

Resource-managed APIs attach cleanup to `struct device`.

| Managed API | Traditional idea |
| --- | --- |
| `devm_kzalloc()` | allocate private memory |
| `devm_ioremap_resource()` | request and map MMIO |
| `devm_request_irq()` | request IRQ |
| `devm_clk_get()` | get clock |
| `devm_regulator_get()` | get regulator |
| `devm_gpiod_get()` | get GPIO descriptor |

`devm_*` reduces error-path boilerplate, but it does not remove the need to unregister subsystem objects in the correct order.

## Lifecycle / Data Flow

The device model is mostly about order. Bugs often happen when a driver exposes an object before it is ready or frees data while someone can still reach it.

### Device and Driver Binding Flow

```text
Device side:
  allocate device-specific object
  initialize embedded struct device
  set parent, bus, release, firmware node, groups
  device_add() or device_register()
  core creates sysfs object and emits add event
  bus checks registered drivers

Driver side:
  initialize bus-specific driver object
  set name, match table, probe, remove, PM callbacks
  register driver
  bus checks registered devices

Binding:
  bus match callback returns true
  core binds device and driver
  probe(dev) runs
  probe stores private data and registers subsystem objects
```

### Probe Flow

```text
probe(device)
  -> allocate per-device state
  -> read firmware or match data
  -> get resources
  -> initialize locks and state
  -> map MMIO / request IRQ / enable clocks as needed
  -> store driver data
  -> register subsystem-visible object
  -> create optional ABI files or groups
  -> return 0
```

Rules:

- Initialize private data before any callback can reach it.
- Store driver data before sysfs, IRQ, or subsystem callbacks need it.
- Register userspace-visible objects late in probe.
- Return the original meaningful errno where possible.

### Remove Flow

```text
remove(device)
  -> stop new userspace/subsystem entry points
  -> unregister subsystem-visible objects
  -> disable hardware activity
  -> cancel work/timers/async callbacks
  -> release non-managed resources
  -> devm resources release after detach
  -> final object memory freed only after references drain
```

Rules:

- Do not free private data while sysfs callbacks, file operations, IRQ handlers, workqueues, or references can still access it.
- Unregister externally visible objects before tearing down the state they call into.
- For manually registered devices, final memory release belongs in the device release callback.

### Char Device Helper Flow

```text
alloc_chrdev_region(&devt, ...)
cdev_init(&cdev, &fops)
cdev_add(&cdev, devt, count)
class_create("my_class")
device_create(class, parent, devt, drvdata, "mydev%d", id)

userspace:
  sees sysfs device with "dev" major:minor
  creates or manages /dev/mydev0 depending on system policy

cleanup:
device_destroy(class, devt)
class_destroy(class)
cdev_del(&cdev)
unregister_chrdev_region(devt, count)
```

## Minimal Practical Example

This example is learning-only. It shows the relationship between `cdev`, class device, sysfs, and `/dev`; it omits real hardware, locking around device state, and full multi-instance design.

```c
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/module.h>

#define MY_NAME "ldm_demo"

static dev_t my_devt;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;

static int my_open(struct inode *inode, struct file *file)
{
        pr_info(MY_NAME ": open\n");
        return 0;
}

static const struct file_operations my_fops = {
        .owner = THIS_MODULE,
        .open = my_open,
};

static ssize_t answer_show(struct device *dev,
                           struct device_attribute *attr,
                           char *buf)
{
        return sysfs_emit(buf, "%d\n", 42);
}
static DEVICE_ATTR_RO(answer);

static int __init my_init(void)
{
        int ret;

        ret = alloc_chrdev_region(&my_devt, 0, 1, MY_NAME);
        if (ret)
                return ret;

        cdev_init(&my_cdev, &my_fops);
        my_cdev.owner = THIS_MODULE;

        ret = cdev_add(&my_cdev, my_devt, 1);
        if (ret)
                goto err_chrdev;

        my_class = class_create(MY_NAME);
        if (IS_ERR(my_class)) {
                ret = PTR_ERR(my_class);
                goto err_cdev;
        }

        my_device = device_create(my_class, NULL, my_devt, NULL, MY_NAME);
        if (IS_ERR(my_device)) {
                ret = PTR_ERR(my_device);
                goto err_class;
        }

        ret = device_create_file(my_device, &dev_attr_answer);
        if (ret)
                goto err_device;

        return 0;

err_device:
        device_destroy(my_class, my_devt);
err_class:
        class_destroy(my_class);
err_cdev:
        cdev_del(&my_cdev);
err_chrdev:
        unregister_chrdev_region(my_devt, 1);
        return ret;
}

static void __exit my_exit(void)
{
        device_remove_file(my_device, &dev_attr_answer);
        device_destroy(my_class, my_devt);
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(my_devt, 1);
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");
```

What to observe:

```bash
ls -l /sys/class/ldm_demo/ldm_demo/
cat /sys/class/ldm_demo/ldm_demo/dev
cat /sys/class/ldm_demo/ldm_demo/answer
ls -l /dev/ldm_demo
```

Important lines:

- `cdev_add()` makes the major/minor usable by file operations.
- `class_create()` creates the functional class.
- `device_create()` creates the class device and exposes the `dev` attribute.
- `DEVICE_ATTR_RO(answer)` creates a read-only sysfs attribute.
- `sysfs_emit()` formats the sysfs value safely.
- Cleanup runs in reverse order.

For production code:

- prefer attribute groups for add-time attributes;
- use per-device private data instead of globals;
- protect shared state with locks;
- handle open file lifetime before removing underlying state;
- validate the class API signature against your target kernel.

## Common Bugs And Debugging

Start debugging from the visible symptom. Device-model bugs usually leave clues in sysfs, dmesg, module aliases, or missing callbacks.

### Module Loads But `probe()` Does Not Run

Likely causes:

- no matching device exists;
- wrong bus type;
- typo in Device Tree `compatible`;
- missing match table;
- missing `MODULE_DEVICE_TABLE()` for module autoload;
- driver registered but device is deferred or not instantiated.

Commands:

```bash
ls /sys/bus/platform/devices/
ls /sys/bus/platform/drivers/
modinfo my_driver.ko | grep alias
cat /sys/bus/platform/devices/<device>/modalias
dmesg | tail -100
```

Fix pattern:

- confirm the device appears on the expected bus;
- compare modalias or compatible string to driver match table;
- add the correct match table and `MODULE_DEVICE_TABLE()`;
- keep `probe()` logs separate from module-init logs.

### `/dev/mydev` Is Missing

Likely causes:

- `cdev_add()` failed;
- `device_create()` was never called or failed;
- wrong major/minor passed to `device_create()`;
- userspace device manager did not create the node;
- permissions or container environment hide the node.

Commands:

```bash
cat /sys/class/<class>/<device>/dev
ls -l /sys/class/<class>/<device>/
ls -l /dev/<device>
dmesg | tail -100
```

Fix pattern:

- check all error returns;
- unwind in reverse order on failure;
- verify the `dev` sysfs attribute contains the expected major/minor;
- distinguish char-device registration from device-node creation.

### Sysfs Attribute Crashes Or Shows Garbage

Likely causes:

- callback uses freed private data;
- missing locking around shared state;
- `show()` writes past buffer or returns wrong length;
- `store()` accepts malformed input;
- attribute exists before state is initialized or after state is torn down.

Fix pattern:

- use `dev_get_drvdata()` only after driver data is set;
- initialize locks before publishing attributes;
- use `sysfs_emit()`;
- parse with helpers such as `kstrto*()` or `kstrtobool()`;
- remove attributes before destroying state;
- prefer attribute groups for initial attributes.

### Remove Hangs Or Use-After-Free Appears

Likely causes:

- workqueue, timer, IRQ, or sysfs callback still uses private data;
- externally visible subsystem object was not unregistered first;
- final free happens outside the release path;
- missing reference management around long-lived pointers.

Fix pattern:

- unregister user-visible objects first;
- disable IRQs or stop hardware before freeing state;
- cancel work and timers synchronously;
- understand which resources are managed by `devm_*` and which are not;
- use `get_device()`/`put_device()` when holding device pointers beyond immediate callbacks.

### PM Ordering Looks Wrong

Likely causes:

- incorrect parent pointer;
- device registered under the wrong parent;
- class/bus/type/PM-domain callbacks misunderstood;
- wakeup policy configured incorrectly from userspace.

Commands:

```bash
readlink -f /sys/class/<class>/<device>/device
ls /sys/devices/.../power/
cat /sys/devices/.../power/wakeup
```

Fix pattern:

- set parent relationships accurately;
- use the real hardware parent for child devices;
- keep PM callbacks tied to the correct `struct device`;
- handle wakeup capability and wakeup policy separately.

## Production Checklist

Use this before sending a driver for review or before a board bring-up handoff.

### Object Model

- The driver uses the correct bus type: platform, I2C, SPI, PCI, USB, or subsystem-specific.
- Each hardware instance has per-device state, not accidental global state.
- The device parent is accurate.
- Any manually created `struct device` has a valid `.release` callback.
- Device names, class names, and sysfs attribute names are stable and meaningful.

### Matching and Probe

- Match tables are correct and include `MODULE_DEVICE_TABLE()` where module autoload matters.
- `probe()` handles multiple matching devices.
- `probe()` returns meaningful negative errno values.
- `probe()` does not expose callbacks before private state is initialized.
- Driver data is stored before callbacks need it.

### Lifetime and Cleanup

- Cleanup order is the reverse of publish order.
- User-visible subsystem objects are unregistered before state is destroyed.
- IRQs, workqueues, timers, and async callbacks cannot access freed memory.
- `devm_*` is used where it fits device lifetime.
- Non-managed resources still have explicit cleanup.
- Device references are balanced.

### Sysfs and Userspace ABI

- Sysfs attributes are simple text values with documented units.
- `show()` uses `sysfs_emit()` or `sysfs_emit_at()`.
- `store()` validates input and returns `count` on success.
- Shared state in sysfs callbacks is locked.
- Add-time attributes use groups when userspace needs them immediately.
- ABI compatibility is considered before renaming or removing attributes.

### Debug and Maintainability

- Logs use `dev_*()` with the correct `struct device *`.
- Debug instructions include useful sysfs paths.
- Error paths are tested, not only the happy path.
- Kernel-version-sensitive APIs are checked against target headers.
- The driver avoids raw kobjects unless there is a strong reason.

## Interview Readiness

You should be ready to answer the interview set in `interview/12-linux-device-model.md` when you can explain the system without reciting API names.

Be able to reason through:

- why `struct device` exists;
- what a bus match callback does;
- how one driver can bind to multiple device instances;
- why `/sys/devices`, `/sys/bus`, and `/sys/class` are different views;
- how `device_create()` helps char drivers create userspace-visible devices;
- why sysfs callbacks need careful locking and lifetime handling;
- why `.release` matters;
- why `probe()` is not module init;
- what `devm_*` cleans up and what it does not;
- how parent-child topology affects PM and cleanup ordering.

**Interview trap:** a good answer connects mechanism to consequences. For example, do not only say "sysfs uses kobjects"; explain that object lifetime and sysfs visibility are tied together, so callbacks must not outlive the state they access.

## Kernel Version Notes

Some APIs shown in older books and notes changed across kernel versions. Check the target kernel headers before copying examples.

- Modern kernels document `class_create(const char *name)`. Older examples often use `class_create(THIS_MODULE, name)`.
- Prefer `sysfs_emit()` or `sysfs_emit_at()` in sysfs `show()` callbacks. Older examples may use `sprintf()` or `scnprintf()`.
- Structure definitions such as `struct bus_type`, `struct device`, `struct device_driver`, and `struct class` evolve. Teach and rely on their roles, then verify exact fields in your target kernel.
- Prefer modern attribute helper macros such as `DEVICE_ATTR_RO()`, `DEVICE_ATTR_WO()`, and `DEVICE_ATTR_RW()` where available.
- For attributes that must exist at device-add time, prefer attribute groups or creation helpers that install groups during device creation.
