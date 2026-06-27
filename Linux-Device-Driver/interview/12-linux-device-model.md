# 12 - Linux Device Model Interview Questions

The Linux Device Model tests whether a candidate can reason about how kernel objects become real driver instances, how userspace sees devices, and how lifetime is kept safe. Strong candidates can trace a device from registration to sysfs, bus matching, probe, userspace ABI, remove, and final release.

Good answers should connect concepts to consequences: if a device has a parent, PM ordering changes; if sysfs exposes a callback, lifetime and locking matter; if a module loads but `probe()` does not run, matching probably failed or no device exists.

## Beginner

### 1. What Is The Linux Device Model?

- **Level:** Beginner
- **Question:** What is the Linux Device Model, and why does the kernel need it?
- **Short Answer:** The Linux Device Model is the kernel's common framework for representing devices, drivers, buses, classes, sysfs visibility, and device lifetime.
- **Deep Explanation:** Without a common model, every subsystem would need its own way to describe devices, match drivers, expose information to userspace, and manage cleanup. The device model gives the kernel a shared object graph. A device has a generic `struct device`, can sit on a bus, can bind to a driver, can belong to a class, can appear in sysfs, and can participate in PM and managed resource cleanup.
- **API / Code Anchor:**
  ```c
  struct device {
          struct device *parent;
          struct bus_type *bus;
          struct device_driver *driver;
          struct class *class;
          struct kobject kobj;
          void (*release)(struct device *dev);
  };
  ```
- **Production or Debugging Angle:** When a driver behaves strangely, inspect the device-model view: `/sys/devices`, `/sys/bus/<bus>`, `/sys/class`, and the kernel log. These paths often tell you whether the device exists, whether it bound, and how userspace sees it.
- **Common Traps:**
  - Treating the device model as only sysfs.
  - Thinking each bus invents a totally separate driver system.
  - Ignoring lifetime and reference counting.
  - Confusing physical topology with functional class.
- **Follow-up Questions:**
  - Why is `struct device` embedded in bus-specific device structs?
  - What problem does sysfs solve?
  - Why does the kernel need parent-child device relationships?

### 2. What Is `struct device`?

- **Level:** Beginner
- **Question:** What does `struct device` represent?
- **Short Answer:** `struct device` is the generic kernel representation of one device instance, physical or virtual.
- **Deep Explanation:** Bus-specific objects such as `platform_device`, `i2c_client`, `spi_device`, and many framework objects either embed or point to a generic device object. That generic object stores identity, parent relationship, bus membership, bound driver, class membership, firmware node, PM state, driver data, kobject/sysfs representation, and lifetime hooks.
- **API / Code Anchor:**
  ```c
  struct platform_device {
          const char *name;
          int id;
          struct device dev;
          struct resource *resource;
  };

  dev_info(&pdev->dev, "using the generic device object\n");
  ```
- **Production or Debugging Angle:** Use the right `struct device *` for `dev_*()` logs, `devm_*()` resources, DMA configuration, PM helpers, and sysfs callbacks. Passing the wrong parent or device can break cleanup, logs, PM, and resource ownership.
- **Common Traps:**
  - Treating `struct device` as only a name string.
  - Using global state instead of per-device state.
  - Passing `NULL` as parent when a real parent exists.
  - Forgetting that `struct device` lifetime can outlive driver private state unless handled carefully.
- **Follow-up Questions:**
  - What is stored in `dev->driver`?
  - What does `dev->parent` affect?
  - Why do `devm_*` APIs take `struct device *`?

### 3. Device, Driver, Bus, Class: What Is The Difference?

- **Level:** Beginner
- **Question:** Explain the difference between a device, driver, bus, and class.
- **Short Answer:** A device is one instance, a driver is code that can manage instances, a bus defines matching between devices and drivers, and a class groups devices by user-visible function.
- **Deep Explanation:** A device answers "what exists?" A driver answers "what code handles it?" A bus answers "how do we decide whether they match?" A class answers "what kind of user-visible function does this object provide?" The same physical device can appear under `/sys/devices`, have links under `/sys/bus`, and have a functional class entry under `/sys/class`.
- **API / Code Anchor:**
  ```text
  /sys/devices/...              physical or virtual topology
  /sys/bus/platform/devices/... bus membership
  /sys/bus/platform/drivers/... registered drivers
  /sys/class/input/...          functional class
  ```
- **Production or Debugging Angle:** If a device appears under `/sys/devices` but not under the expected class, it may exist but not have registered with the subsystem that creates the class device. If the driver appears under `/sys/bus/.../drivers` but probe did not run, matching probably failed.
- **Common Traps:**
  - Saying class is the physical parent.
  - Assuming `/sys/class` proves hardware topology.
  - Thinking drivers live under `/sys/devices`.
  - Ignoring the bus when debugging matching.
- **Follow-up Questions:**
  - Where would you check whether a platform driver registered?
  - Where would you check whether a tty device exists?
  - Why can the same device have links in multiple sysfs places?

### 4. Why Is `probe()` Not Module Init?

- **Level:** Beginner
- **Question:** Why is a driver's `probe()` function different from module initialization?
- **Short Answer:** Module init registers driver code once. `probe()` runs once for each matching device instance.
- **Deep Explanation:** Loading a module usually calls a registration helper such as `platform_driver_register()`. That tells the bus, "this driver exists." The bus then compares the driver to devices already registered on that bus. If one or more devices match, `probe()` runs for each. If no matching device exists, module init succeeds but `probe()` does not run.
- **API / Code Anchor:**
  ```c
  static struct platform_driver demo_driver = {
          .probe = demo_probe,
          .remove = demo_remove,
          .driver = { .name = "demo" },
  };

  module_platform_driver(demo_driver);
  ```
- **Production or Debugging Angle:** Put separate log messages in module registration and `probe()` while debugging. If only module registration logs appear, inspect device creation and matching.
- **Common Traps:**
  - Initializing per-device hardware in module init.
  - Expecting one global private object to work for all devices.
  - Assuming `insmod` failure is the only reason probe did not run.
  - Assuming `remove()` is the same as module exit.
- **Follow-up Questions:**
  - What happens if two devices match one driver?
  - Where should MMIO mapping happen?
  - What should module exit do in a simple bus driver module?

## Mid-level

### 5. How Does Device-Driver Binding Work?

- **Level:** Mid-level
- **Question:** Walk through how a device binds to a driver.
- **Short Answer:** A device and driver register on the same bus. The bus match callback decides whether they are compatible. If they match, the driver core binds them and calls the driver's probe callback.
- **Deep Explanation:** Matching is intentionally bus-specific. PCI compares vendor/device IDs. Platform may compare Device Tree compatible strings, ACPI IDs, platform ID tables, or names. I2C and SPI use their own bus rules. The generic driver core coordinates registration and binding, but the bus owns the logic that says "this driver supports this device."
- **API / Code Anchor:**
  ```c
  struct bus_type {
          int (*match)(struct device *dev, struct device_driver *drv);
          int (*probe)(struct device *dev);
          void (*remove)(struct device *dev);
  };
  ```
- **Production or Debugging Angle:** For a platform driver, compare `/sys/bus/platform/devices/<dev>/modalias`, Device Tree `compatible`, and `modinfo <driver>.ko | grep alias`. Also verify the driver appears in `/sys/bus/platform/drivers/`.
- **Common Traps:**
  - Saying the driver itself always decides matching.
  - Assuming all buses match by name.
  - Missing `MODULE_DEVICE_TABLE()` for autoloadable modules.
  - Confusing Device Tree node creation with driver binding.
- **Follow-up Questions:**
  - Why is matching bus-specific?
  - What happens when the driver registers before the device?
  - What happens when the device registers before the driver?

### 6. How Do `class_create()` And `device_create()` Help Character Drivers?

- **Level:** Mid-level
- **Question:** Why do many character drivers call `class_create()` and `device_create()` after `cdev_add()`?
- **Short Answer:** `cdev_add()` connects a major/minor number to file operations. `class_create()` and `device_create()` create a device-model object under `/sys/class` with a `dev` attribute so userspace can create or manage `/dev/<name>`.
- **Deep Explanation:** A `cdev` alone is enough for the kernel to dispatch file operations if a device node exists. But userspace needs a discoverable major/minor pair and a device name. `device_create()` registers a class device and exposes the `dev_t`. On systems with devtmpfs or udev/mdev policy, this leads to a usable `/dev` node.
- **API / Code Anchor:**
  ```c
  ret = alloc_chrdev_region(&devt, 0, 1, "demo");
  cdev_init(&cdev, &demo_fops);
  ret = cdev_add(&cdev, devt, 1);

  cls = class_create("demo");
  dev = device_create(cls, NULL, devt, NULL, "demo0");
  ```
- **Production or Debugging Angle:** If `/dev/demo0` is missing, check `/sys/class/demo/demo0/dev`. If the sysfs `dev` attribute exists with the right major/minor, the kernel side likely created the class device and the remaining issue may be userspace node creation or permissions.
- **Common Traps:**
  - Thinking `device_create()` registers file operations.
  - Forgetting to check `IS_ERR()` on `class_create()` and `device_create()`.
  - Destroying class before device.
  - Passing the wrong `dev_t`.
- **Follow-up Questions:**
  - What does `cdev_add()` do?
  - What does the `dev` sysfs attribute contain?
  - What cleanup order should be used?

### 7. How Should A Sysfs Attribute Be Implemented?

- **Level:** Mid-level
- **Question:** What makes a good sysfs attribute implementation?
- **Short Answer:** It should expose a simple stable text value, use proper `show()` and `store()` callbacks, validate input, use locking, return correct byte counts, and respect object lifetime.
- **Deep Explanation:** Sysfs is not a general file-storage mechanism. A read calls a kernel `show()` callback. A write calls a `store()` callback. The driver must format output safely, parse input carefully, and protect shared state. The attribute is ABI once userspace depends on it, so names, units, and semantics should be stable.
- **API / Code Anchor:**
  ```c
  static ssize_t enabled_show(struct device *dev,
                              struct device_attribute *attr,
                              char *buf)
  {
          struct foo *foo = dev_get_drvdata(dev);

          return sysfs_emit(buf, "%u\n", foo->enabled);
  }

  static ssize_t enabled_store(struct device *dev,
                               struct device_attribute *attr,
                               const char *buf,
                               size_t count)
  {
          struct foo *foo = dev_get_drvdata(dev);
          bool val;
          int ret = kstrtobool(buf, &val);

          if (ret)
                  return ret;

          mutex_lock(&foo->lock);
          foo->enabled = val;
          mutex_unlock(&foo->lock);

          return count;
  }

  static DEVICE_ATTR_RW(enabled);
  ```
- **Production or Debugging Angle:** If a sysfs read crashes, suspect lifetime or missing driver data. If writes behave oddly, inspect parsing and return value. If userspace races with remove, unregister attributes or subsystem objects before freeing state.
- **Common Traps:**
  - Returning `sizeof(value)` instead of `count` from `store()`.
  - Using `sprintf()` carelessly instead of `sysfs_emit()`.
  - Exposing multiple unrelated values in one file.
  - Missing locks around state accessed by IRQ, workqueue, or file operations.
  - Treating sysfs as debugfs.
- **Follow-up Questions:**
  - Why is sysfs considered ABI?
  - When would you use debugfs instead?
  - Why are attribute groups useful?

### 8. What Does `devm_*` Actually Manage?

- **Level:** Mid-level
- **Question:** How are `devm_*` APIs related to the device model?
- **Short Answer:** `devm_*` resources are attached to a `struct device` and are automatically released when that device is detached or the driver probe path unwinds.
- **Deep Explanation:** The driver core maintains managed resource actions associated with a device. When probe fails or the device is removed, devres releases those resources in an orderly way. This simplifies error paths for memory, MMIO mappings, IRQs, clocks, regulators, GPIOs, and similar resources.
- **API / Code Anchor:**
  ```c
  priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
  base = devm_ioremap_resource(&pdev->dev, res);
  ret = devm_request_irq(&pdev->dev, irq, handler, 0,
                         dev_name(&pdev->dev), priv);
  ```
- **Production or Debugging Angle:** `devm_*` does not unregister everything your driver publishes. If you register a char device, input device, netdev, IIO device, or sysfs ABI manually, you still must unregister it at the right time unless that subsystem provides a managed registration helper.
- **Common Traps:**
  - Thinking `devm_*` replaces `remove()` entirely.
  - Passing the wrong parent device to `devm_*`.
  - Using managed memory for objects that must outlive the device.
  - Destroying state before externally visible callbacks are stopped.
- **Follow-up Questions:**
  - What happens to `devm_kzalloc()` memory when probe fails?
  - Why does `devm_request_irq()` take `struct device *`?
  - What objects might still need explicit unregister?

### 9. Debugging Scenario: Driver Is Registered But Device Is Not Bound

- **Level:** Mid-level
- **Question:** A platform driver module loads and appears under `/sys/bus/platform/drivers/foo`, but its `probe()` never runs. How do you debug it?
- **Short Answer:** Verify that a matching device exists on the platform bus, compare its modalias or Device Tree compatible string with the driver's match table, check module aliases, and inspect dmesg for deferred probe or registration errors.
- **Deep Explanation:** Driver registration only adds the driver to the bus driver list. Probe requires a matching device. On Device Tree systems, the node must be enabled, instantiated as a platform device, and have a compatible string that matches the driver's `of_match_table`. If the driver relies on module autoloading, `MODULE_DEVICE_TABLE()` must create aliases.
- **API / Code Anchor:**
  ```bash
  ls /sys/bus/platform/devices/
  ls /sys/bus/platform/drivers/foo/
  cat /sys/bus/platform/devices/<dev>/modalias
  modinfo foo.ko | grep alias
  dmesg | grep -i foo
  ```
- **Production or Debugging Angle:** If the device exists and matches but probe still does not run, check bind/unbind state, driver override, supplier dependencies, and whether another driver already bound to the device.
- **Common Traps:**
  - Looking only at `lsmod`.
  - Checking `/sys/class` before binding happened.
  - Ignoring disabled Device Tree nodes.
  - Assuming name fallback should work for DT devices.
  - Losing the original `-EPROBE_DEFER` error.
- **Follow-up Questions:**
  - What is a modalias?
  - How does `MODULE_DEVICE_TABLE(of, ...)` help?
  - What does `-EPROBE_DEFER` mean?

## Senior

### 10. Why Is The Device Release Callback So Important?

- **Level:** Senior
- **Question:** Why must manually created `struct device` objects have a release callback?
- **Short Answer:** The release callback is where final memory cleanup happens after the last reference to the device is dropped. Without it, device lifetime is undefined and the kernel may warn or leak memory.
- **Deep Explanation:** Device removal and final object destruction are not the same event. `device_del()` removes visibility and binding, but references may still exist from sysfs, iterators, open users, child objects, or other kernel users. The driver core uses reference counting. When the final reference is dropped, the release callback runs. That is the safe point to free the object containing the `struct device`.
- **API / Code Anchor:**
  ```c
  struct foo_device {
          struct device dev;
          /* private fields */
  };

  static void foo_release(struct device *dev)
  {
          struct foo_device *foo = container_of(dev, struct foo_device, dev);

          kfree(foo);
  }

  foo->dev.release = foo_release;
  ```
- **Production or Debugging Angle:** A common senior-level bug is freeing the container immediately after `device_unregister()`, while another reference path can still reach it. The release callback prevents that by making final free reference-count driven.
- **Common Traps:**
  - Freeing the device container in remove instead of release.
  - Setting `.release = NULL` for test code.
  - Assuming sysfs removal means no references exist.
  - Forgetting child devices can hold parent references.
- **Follow-up Questions:**
  - What is the difference between `device_del()` and `put_device()`?
  - When should you use `get_device()`?
  - Why can release run later than remove?

### 11. How Can Sysfs Race With Device Removal?

- **Level:** Senior
- **Question:** What race conditions can happen between sysfs callbacks and driver removal?
- **Short Answer:** A sysfs `show()` or `store()` callback can access driver state while remove is tearing it down unless the driver unregisters attributes or subsystem objects in the right order and protects state with lifetime and locking rules.
- **Deep Explanation:** Sysfs exposes callbacks into your driver. Userspace can read or write while the device is active, and remove can happen because of unbind, hot unplug, module unload, or failed probe cleanup. If private data is freed before callbacks are impossible, callbacks can use freed memory. If hardware is disabled while a sysfs operation still expects it, the callback may hang or read invalid state.
- **API / Code Anchor:**
  ```c
  static void foo_remove(struct platform_device *pdev)
  {
          struct foo *foo = platform_get_drvdata(pdev);

          device_remove_file(&pdev->dev, &dev_attr_enabled);
          cancel_work_sync(&foo->work);
          /* now tear down state not managed elsewhere */
  }
  ```
- **Production or Debugging Angle:** Prefer managed or grouped attribute creation when it fits, but still reason about callback state. For complex devices, use a state flag, mutex, runtime PM references, or subsystem unregister ordering so callbacks cannot operate on dead hardware.
- **Common Traps:**
  - Removing locks too early.
  - Assuming sysfs callbacks are serialized with remove.
  - Using raw pointers without checking device state.
  - Publishing attributes before private data is initialized.
  - Forgetting work scheduled from `store()`.
- **Follow-up Questions:**
  - Should sysfs callbacks sleep?
  - When should `sysfs_notify()` be used?
  - How would you protect a sysfs register access during runtime suspend?

### 12. How Does The Device Model Affect Power Management?

- **Level:** Senior
- **Question:** Why does the device model matter for suspend, resume, runtime PM, and wakeup?
- **Short Answer:** PM uses device relationships, bus/class/type/driver callbacks, power domains, and per-device PM state to order and control power transitions.
- **Deep Explanation:** A device has a parent, bus, driver, class/type relationships, optional PM domain, and `dev_pm_info`. During system sleep and resume, the PM core walks device relationships and invokes callbacks in defined phases. Runtime PM also operates on a device object. Wakeup capability is represented on the device and exposed through power-related sysfs attributes when enabled.
- **API / Code Anchor:**
  ```c
  static const struct dev_pm_ops foo_pm_ops = {
          SET_RUNTIME_PM_OPS(foo_runtime_suspend,
                             foo_runtime_resume,
                             NULL)
          SET_SYSTEM_SLEEP_PM_OPS(foo_suspend, foo_resume)
  };

  static struct platform_driver foo_driver = {
          .driver = {
                  .name = "foo",
                  .pm = &foo_pm_ops,
          },
  };
  ```
- **Production or Debugging Angle:** Wrong parent pointers can produce wrong suspend ordering. Missing runtime PM references can power off hardware during sysfs, file, or IRQ paths. Wakeup-capable hardware needs both hardware support and userspace wakeup policy.
- **Common Traps:**
  - Treating PM callbacks as isolated driver functions.
  - Ignoring parent-child order.
  - Confusing wakeup capability with wakeup enabled policy.
  - Forgetting power domains can provide callbacks too.
  - Accessing registers while runtime suspended.
- **Follow-up Questions:**
  - Why does `device_init_wakeup()` create sysfs policy knobs?
  - What is the difference between runtime PM and system sleep?
  - How can parent relationships affect suspend order?

### 13. When Should You Create Raw kobjects?

- **Level:** Senior
- **Question:** Should normal device drivers create raw `struct kobject` objects?
- **Short Answer:** Usually no. Normal drivers should use existing device-model, subsystem, class, or bus APIs. Raw kobjects are for lower-level infrastructure or special cases.
- **Deep Explanation:** `struct device` already embeds a kobject and integrates with sysfs, lifetime, PM, uevents, and devres. Subsystems provide safer wrappers with documented ABI rules. Creating raw kobjects means you own reference counting, release behavior, sysfs operations, and ABI design. That is easy to get wrong and often unnecessary.
- **API / Code Anchor:**
  ```c
  /* Prefer this for a device attribute */
  static DEVICE_ATTR_RO(status);
  device_create_file(dev, &dev_attr_status);

  /* Raw kobject APIs exist, but are rarely what a normal driver needs */
  kobject_init_and_add(kobj, ktype, parent, "name");
  kobject_put(kobj);
  ```
- **Production or Debugging Angle:** If you are tempted to create `/sys/kernel/mydriver`, ask whether the data belongs in the device's sysfs directory, debugfs, configfs, a subsystem ABI, or a class device. The answer is rarely raw kobject sysfs.
- **Common Traps:**
  - Creating custom sysfs trees for per-device data.
  - Forgetting a kobject release method.
  - Exposing debug data as stable sysfs ABI.
  - Duplicating functionality already provided by a subsystem.
- **Follow-up Questions:**
  - When is debugfs more appropriate than sysfs?
  - What does `kobj_type` provide?
  - Why does `container_of()` appear in kobject code?

### 14. How Would You Design A Maintainable Device-Model ABI?

- **Level:** Senior
- **Question:** You need to expose a device setting to userspace. How do you decide whether to use sysfs, ioctl, debugfs, or an existing subsystem API?
- **Short Answer:** Use an existing subsystem ABI when one exists. Use sysfs for simple stable per-device attributes. Use ioctl for complex command-style device-specific ABI when appropriate. Use debugfs only for debugging data with no stability guarantee.
- **Deep Explanation:** Sysfs is best for simple text attributes: one value, stable units, predictable permissions. Existing subsystems already define standard ABI for common device families, such as input, RTC, IIO, regulator, network, V4L2, and GPIO. Debugfs is useful for diagnostics but should not be required by production userspace. Ioctl can handle structured commands but creates compatibility and ABI maintenance burden.
- **API / Code Anchor:**
  ```text
  Simple stable setting:
    /sys/class/.../enable

  Standard sensor data:
    IIO sysfs/buffer ABI

  Debug-only register dump:
    debugfs

  Device-specific command ABI:
    ioctl, with stable structs and compat handling
  ```
- **Production or Debugging Angle:** ABI mistakes live for years. Before adding sysfs files, define names, units, permissions, error behavior, locking, and compatibility. Do not expose raw hardware details that future hardware cannot support.
- **Common Traps:**
  - Putting debug-only knobs in sysfs.
  - Creating multi-value sysfs files with unclear parsing.
  - Duplicating an existing subsystem ABI.
  - Changing sysfs names after userspace depends on them.
  - Ignoring 32-bit compatibility for ioctl structs.
- **Follow-up Questions:**
  - What makes a sysfs file a stable ABI?
  - When is debugfs unacceptable?
  - Why should hardware register dumps usually not be sysfs ABI?

### 15. Debugging Scenario: Use-After-Free On Module Unload

- **Level:** Senior
- **Question:** A driver unloads and then the kernel reports a use-after-free in a sysfs callback or delayed work item. How do you reason about it?
- **Short Answer:** Something still had a path to driver private data after remove started. Check unregister order, sysfs attribute removal, work/timer cancellation, IRQ shutdown, open file references, and whether final memory is freed only after references drain.
- **Deep Explanation:** Device removal is a publication problem. During probe, the driver publishes entry points: sysfs files, device nodes, IRQ handlers, work, subsystem callbacks. During remove, it must stop those entry points before freeing state. If delayed work or sysfs still points at `priv`, freeing `priv` in remove is unsafe. If `priv` is tied to `devm_kzalloc()`, it remains until managed cleanup, but callbacks still must be stopped before that cleanup occurs.
- **API / Code Anchor:**
  ```c
  static void foo_remove(struct platform_device *pdev)
  {
          struct foo *foo = platform_get_drvdata(pdev);

          device_remove_file(&pdev->dev, &dev_attr_state);
          misc_deregister(&foo->miscdev);
          cancel_delayed_work_sync(&foo->poll_work);
          /* devm resources release after remove path continues */
  }
  ```
- **Production or Debugging Angle:** Use KASAN, lockdep, dynamic debug, and careful dmesg timestamps to find which callback ran after teardown. Then move unregister/cancel earlier or add state/refcount protection.
- **Common Traps:**
  - Cancelling work after freeing its container.
  - Removing `/dev` node after destroying file-operation state.
  - Assuming `devm_*` prevents callback races.
  - Ignoring sysfs operations in progress.
  - Forgetting runtime PM or IRQ handlers can schedule work.
- **Follow-up Questions:**
  - What should be unpublished first in remove?
  - How do open file descriptors affect removal?
  - What tools help find lifetime bugs?

### 16. What Kernel-Version Traps Exist In This Topic?

- **Level:** Senior
- **Question:** What version-sensitive details should you watch for when using older Linux device-model examples?
- **Short Answer:** Check exact API signatures and helper recommendations against the target kernel. Common traps include old `class_create(THIS_MODULE, name)` examples, old sysfs formatting with `sprintf()`, and structure fields that have changed over time.
- **Deep Explanation:** Device-model concepts are stable, but headers evolve. Older books may show fields that no longer exist or callback signatures that changed. Modern kernels document `class_create(const char *name)`. Sysfs `show()` callbacks should use `sysfs_emit()` or `sysfs_emit_at()`. Attribute groups and helper macros are preferred for many modern patterns.
- **API / Code Anchor:**
  ```c
  /* Modern style in current kernels */
  cls = class_create("demo");

  /* Modern sysfs formatting */
  return sysfs_emit(buf, "%u\n", value);
  ```
- **Production or Debugging Angle:** Before copying code, inspect `include/linux/device.h`, relevant subsystem headers, and in-tree drivers for the target kernel. Build warnings around incompatible pointer types often reveal stale examples early.
- **Common Traps:**
  - Copying old struct field snapshots.
  - Assuming examples from one kernel compile on another.
  - Treating sysfs implementation details as stable driver API.
  - Missing new helper macros that simplify correct code.
- **Follow-up Questions:**
  - How would you verify the right `class_create()` signature?
  - Why should docs emphasize roles more than struct fields?
  - What in-tree examples would you inspect before writing new code?

