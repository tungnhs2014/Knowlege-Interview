# 09 - Platform Bus And Platform Drivers Interview Questions

Platform drivers test whether a candidate understands the Linux device model in embedded systems. A strong answer should explain how non-discoverable hardware becomes a `struct device`, how the platform bus matches it to a driver, and how `probe()` safely turns resources into a live subsystem device.

Use these questions to test reasoning, not keyword recall. Good candidates can trace a device from firmware description to `/sys/bus/platform/devices`, through matching, into `probe()`, resource acquisition, interrupt setup, and remove-time cleanup.

## Beginner

### 1. What Is The Platform Bus?

- **Level:** Beginner
- **Question:** What is the Linux platform bus, and why does it exist?
- **Short Answer:** The platform bus is a kernel software bus for devices that are not automatically discoverable by hardware enumeration. It lets the kernel use the normal device-driver model for SoC and board devices described by firmware, board code, MFD core, or test code.
- **Deep Explanation:** PCI and USB devices can announce themselves through bus enumeration. Many embedded devices cannot. A UART controller or GPIO controller inside an SoC does not appear by scanning a physical platform bus. The kernel still needs a `struct device`, resources, matching, sysfs representation, power-management hooks, and a `probe()` callback. The platform bus provides that common framework.
- **API / Code Anchor:**
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
- **Production or Debugging Angle:** If the module loads but `probe()` does not run, check whether a matching device exists under `/sys/bus/platform/devices/` and whether the driver is registered under `/sys/bus/platform/drivers/`.
- **Common Traps:**
  - Saying platform bus is a physical bus.
  - Thinking every non-hotplug device should use a platform driver.
  - Confusing SoC bus controllers with the client devices attached to those controllers.
  - Assuming `insmod` means `probe()` must have run.
- **Follow-up Questions:**
  - How is a platform device created on a Device Tree system?
  - Why does PCI not normally need a platform driver?
  - What appears in `/sys/bus/platform/devices/`?

### 2. Platform Device Versus Platform Driver

- **Level:** Beginner
- **Question:** What is the difference between `struct platform_device` and `struct platform_driver`?
- **Short Answer:** `struct platform_device` represents one device instance and its resources. `struct platform_driver` represents code that can bind to matching device instances and manage them through callbacks such as `probe()` and `remove()`.
- **Deep Explanation:** A platform device says, "there is hardware here." It contains a name, ID, embedded `struct device`, resources, and firmware linkage. A platform driver says, "I can handle hardware of this type." It contains callbacks, a driver name, and match tables. The platform bus compares devices and drivers; when they match, it calls the driver's `probe()` with that specific `platform_device`.
- **API / Code Anchor:**
  ```c
  struct platform_device {
          const char *name;
          int id;
          struct device dev;
          u32 num_resources;
          struct resource *resource;
  };

  struct platform_driver {
          int (*probe)(struct platform_device *);
          int (*remove)(struct platform_device *);
          struct device_driver driver;
          const struct platform_device_id *id_table;
  };
  ```
- **Production or Debugging Angle:** When debugging, identify both sides: the device in `/sys/bus/platform/devices/<device>` and the driver in `/sys/bus/platform/drivers/<driver>`. A driver without a device is just registered code.
- **Common Traps:**
  - Registering only the driver and expecting hardware to appear.
  - Creating a platform device with the wrong name or `compatible`.
  - Using one global private state for multiple device instances.
  - Forgetting that `probe()` receives the device instance.
- **Follow-up Questions:**
  - What field stores resources in `platform_device`?
  - Where is the generic driver-model `struct device`?
  - Why does `probe()` take `struct platform_device *pdev`?

### 3. Why Is `probe()` Not `module_init()`?

- **Level:** Beginner
- **Question:** Explain the difference between module initialization and a platform driver's `probe()`.
- **Short Answer:** Module initialization registers the driver once. `probe()` runs once for each matching platform device instance.
- **Deep Explanation:** `module_platform_driver()` expands to module init and exit code that calls `platform_driver_register()` and `platform_driver_unregister()`. Registering the driver only makes it available to the bus. The bus calls `probe()` when a matching device exists. If there are three matching devices, `probe()` can run three times. If there are no matching devices, it may not run at all.
- **API / Code Anchor:**
  ```c
  module_platform_driver(foo_driver);

  static int foo_probe(struct platform_device *pdev)
  {
          dev_info(&pdev->dev, "one matched device instance\n");
          return 0;
  }
  ```
- **Production or Debugging Angle:** A log in module init proves the module loaded. A log in `probe()` proves binding happened. Keep these separate when debugging.
- **Common Traps:**
  - Initializing per-device hardware in module init.
  - Using global state because the driver author expects only one device.
  - Assuming `rmmod` cleanup is equivalent to per-device remove cleanup.
  - Forgetting `probe()` can defer and run later.
- **Follow-up Questions:**
  - What happens if two Device Tree nodes match the same driver?
  - Where should MMIO mapping happen?
  - What should module exit do for a simple platform driver module?

### 4. Platform Drivers Versus I2C And SPI Drivers

- **Level:** Beginner
- **Question:** Are I2C and SPI devices platform devices?
- **Short Answer:** SoC I2C and SPI controllers are often platform devices, but the devices attached to those buses are I2C or SPI client devices and should use `i2c_driver` or `spi_driver`.
- **Deep Explanation:** The controller is usually an MMIO block integrated into the SoC. Since it cannot be discovered by PCI/USB-style enumeration, it is commonly represented as a platform device. Once the I2C or SPI controller driver is running, it creates or discovers bus-specific child devices from firmware. An I2C sensor should bind to an I2C driver, not a platform driver.
- **API / Code Anchor:**
  ```text
  SoC MMIO I2C controller
      -> platform_device
      -> platform_driver

  Sensor at I2C address 0x48
      -> i2c_client
      -> i2c_driver
  ```
- **Production or Debugging Angle:** If your driver receives no `probe()`, confirm the device is on the bus you think it is. Check `/sys/bus/platform/devices/`, `/sys/bus/i2c/devices/`, and `/sys/bus/spi/devices/`.
- **Common Traps:**
  - Writing a platform driver for an I2C sensor.
  - Saying all non-discoverable devices are platform devices.
  - Ignoring the controller/client distinction.
  - Matching on the controller node when you meant to match a child node.
- **Follow-up Questions:**
  - What bus type should an SPI flash chip use?
  - What bus type should an SoC SPI controller use?
  - How do child devices appear after a controller probes?

## Mid-level

### 5. How Does Platform Driver Matching Work?

- **Level:** Mid-level
- **Question:** How does the platform bus decide whether a `platform_device` matches a `platform_driver`?
- **Short Answer:** The platform bus match path checks explicit override first, then firmware matches such as Device Tree and ACPI, then the platform ID table, and finally falls back to device name versus driver name.
- **Deep Explanation:** The platform bus has a match callback. On a Device Tree system, the common match is the device node's `compatible` string against the driver's `of_match_table`. On legacy or manually created devices, the device name may match a `platform_device_id` table or the driver's `.driver.name`. Match tables also help module autoloading through `MODULE_DEVICE_TABLE()`.
- **API / Code Anchor:**
  ```c
  static const struct of_device_id foo_of_match[] = {
          { .compatible = "vendor,foo-v1", .data = &foo_v1 },
          { .compatible = "vendor,foo-v2", .data = &foo_v2 },
          { }
  };
  MODULE_DEVICE_TABLE(of, foo_of_match);

  static const struct platform_device_id foo_ids[] = {
          { "foo-v1", FOO_V1 },
          { "foo-v2", FOO_V2 },
          { }
  };
  MODULE_DEVICE_TABLE(platform, foo_ids);
  ```
- **Production or Debugging Angle:** For autoload failures, run `modinfo driver.ko | grep alias` and compare with the device modalias under `/sys/bus/platform/devices/<device>/modalias`.
- **Common Traps:**
  - Missing `MODULE_DEVICE_TABLE()`.
  - Typo in `compatible`.
  - Depending on name fallback in new DT-based code.
  - Assuming platform ID table and OF table are the same thing.
  - Hiding match data in a way that breaks build or autoload expectations.
- **Follow-up Questions:**
  - Why is compatible-string matching better for hardware variants?
  - What does the `.data` field in `of_device_id` do?
  - When would name fallback still matter?

### 6. What Should A Good `probe()` Do?

- **Level:** Mid-level
- **Question:** Walk through a typical platform driver's `probe()` function.
- **Short Answer:** `probe()` should allocate per-device state, read match/config data, acquire resources, initialize hardware, store driver data, expose IRQ/subsystem entry points only when callbacks can safely run, and return either `0` or a meaningful negative errno.
- **Deep Explanation:** `probe()` is where the driver turns a generic device description into a live kernel object. It should not assume one global device. It should use `&pdev->dev` for logging and managed resources. It should carefully order initialization so that IRQ handlers and subsystem callbacks cannot observe partially initialized state.
- **API / Code Anchor:**
  ```c
  static int foo_probe(struct platform_device *pdev)
  {
          struct foo *foo;
          struct resource *res;
          int irq, ret;

          foo = devm_kzalloc(&pdev->dev, sizeof(*foo), GFP_KERNEL);
          if (!foo)
                  return -ENOMEM;

          res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
          foo->base = devm_ioremap_resource(&pdev->dev, res);
          if (IS_ERR(foo->base))
                  return PTR_ERR(foo->base);

          irq = platform_get_irq(pdev, 0);
          if (irq < 0)
                  return irq;

          platform_set_drvdata(pdev, foo);

          ret = devm_request_irq(&pdev->dev, irq, foo_irq, 0,
                                 dev_name(&pdev->dev), foo);
          if (ret)
                  return ret;

          return foo_register_with_subsystem(foo);
  }
  ```
- **Production or Debugging Angle:** If resource acquisition can defer, return `-EPROBE_DEFER` unchanged. `dev_err_probe()` is useful for logging supplier-related failures without noisy repeated messages.
- **Common Traps:**
  - Requesting IRQ before state and hardware are ready.
  - Returning `0` after partial initialization failure.
  - Converting every error to `-EINVAL`.
  - Allocating one global object for all devices.
  - Forgetting `platform_set_drvdata()`.
- **Follow-up Questions:**
  - Where should subsystem registration happen?
  - Why is `&pdev->dev` passed to `devm_*` APIs?
  - What can go wrong if an IRQ fires during probe?

### 7. How Are Platform Resources Used?

- **Level:** Mid-level
- **Question:** How does a platform driver get MMIO and IRQ resources?
- **Short Answer:** It gets MMIO resources with `platform_get_resource()` or a named variant, maps them with `devm_ioremap_resource()`, and gets IRQs with `platform_get_irq()` or `platform_get_irq_byname()`.
- **Deep Explanation:** Platform resources abstract hardware description. In old board files, resources were C arrays. In Device Tree, properties like `reg` and `interrupts` are converted into resources or IRQ mappings. The driver can use platform helpers in both cases. For multiple resources, named resources make the driver less dependent on ordering.
- **API / Code Anchor:**
  ```c
  res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "control");
  base = devm_ioremap_resource(&pdev->dev, res);
  if (IS_ERR(base))
          return PTR_ERR(base);

  irq = platform_get_irq_byname(pdev, "rx");
  if (irq < 0)
          return irq;
  ```
- **Production or Debugging Angle:** If MMIO mapping fails, inspect DT `reg`, parent `ranges`, `#address-cells`, and `/proc/iomem`. If IRQ lookup fails, inspect `interrupts`, `interrupt-parent`, and `/proc/interrupts`.
- **Common Traps:**
  - Hardcoding physical addresses in the driver.
  - Using the wrong resource index.
  - Ignoring negative return values from `platform_get_irq()`.
  - Treating optional resources as mandatory without design intent.
  - Calling `ioremap()` without reserving the memory region when a managed resource helper would do both.
- **Follow-up Questions:**
  - Why are named resources useful?
  - What does `resource_size(res)` compute?
  - How does a DT `reg` property become usable by `platform_get_resource()`?

### 8. What Are `devm_*` APIs And What Do They Not Solve?

- **Level:** Mid-level
- **Question:** Why are `devm_*` APIs common in platform drivers, and what do they not replace?
- **Short Answer:** `devm_*` APIs attach resources to `struct device` and automatically release them on probe failure or device detach. They simplify cleanup, but they do not replace explicit unregistering, quiescing hardware, canceling work, or reasoning about ordering.
- **Deep Explanation:** Devres keeps a list of release actions associated with the device. If probe fails after a managed allocation or mapping, the driver core can release those resources automatically. This makes probe error paths shorter. However, a subsystem object may still need explicit unregistering before devm frees memory. For example, a net device, input device, char device, workqueue, DMA stream, or runtime PM state may need intentional shutdown order.
- **API / Code Anchor:**
  ```c
  priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
  base = devm_ioremap_resource(&pdev->dev, res);
  ret = devm_request_irq(&pdev->dev, irq, handler, 0, name, priv);
  ```
- **Production or Debugging Angle:** Remove crashes often happen when devm frees memory while some subsystem callback, IRQ, work item, or userspace path can still reach it. Stop external entry points first.
- **Common Traps:**
  - Thinking devm means `remove()` can always be empty.
  - Manually freeing a devm-managed pointer.
  - Registering subsystem objects without unregistering them.
  - Forgetting that cleanup order can matter.
  - Holding resources beyond the device lifetime.
- **Follow-up Questions:**
  - What object owns devm-managed resources?
  - Why can an empty `remove()` still be wrong?
  - Give an example of something that usually needs explicit unregistering.

### 9. Device Tree Integration In A Platform Driver

- **Level:** Mid-level
- **Question:** What Device Tree information does a platform driver normally consume?
- **Short Answer:** It normally matches on `compatible`, gets MMIO from `reg`, IRQs from `interrupts`, and named suppliers such as clocks, resets, GPIOs, regulators, DMA channels, and pinctrl states from their subsystem APIs.
- **Deep Explanation:** Device Tree describes hardware, not driver policy. The platform core creates a device from a node and associates resources with it. The driver should use generic platform and subsystem APIs rather than parsing everything manually. Custom properties can be read when they represent hardware configuration, and match data can represent variant-specific behavior.
- **API / Code Anchor:**
  ```c
  static const struct of_device_id foo_of_match[] = {
          { .compatible = "vendor,foo", .data = &foo_data },
          { }
  };

  data = of_device_get_match_data(&pdev->dev);
  clk = devm_clk_get(&pdev->dev, "core");
  reset = devm_reset_control_get_optional_exclusive(&pdev->dev, NULL);
  gpio = devm_gpiod_get_optional(&pdev->dev, "reset", GPIOD_OUT_HIGH);
  ```
- **Production or Debugging Angle:** If probe fails with supplier errors, inspect phandles and provider nodes. A typo in `clock-names` or `reset-gpios` can look like a driver bug even though the driver code is fine.
- **Common Traps:**
  - Manually decoding `reg` instead of using platform resource helpers.
  - Putting software policy in DT rather than hardware description.
  - Using platform data for new board descriptions.
  - Forgetting `status = "okay"` on nodes that should exist.
  - Going too deep into OF parsing in a basic platform driver.
- **Follow-up Questions:**
  - What should be deferred to a separate Device Tree topic?
  - Why use match data for variants?
  - How would you debug a missing clock supplier?

## Senior

### 10. Debugging Scenario: Module Loads But No Probe

- **Level:** Senior
- **Question:** A platform driver module loads successfully. `lsmod` shows it, but your `probe()` log never appears. How do you debug this?
- **Short Answer:** Separate driver registration from device matching. Confirm the driver is registered, confirm a platform device exists, compare modalias and match tables, check DT node status and compatible strings, and verify module aliases.
- **Deep Explanation:** A successful module load only means module init succeeded. The platform bus calls `probe()` only after a device and driver match. The device may not exist because the DT node is disabled or under a bus that was not populated. The match may fail because `compatible` is wrong, the OF table is missing, the driver is on the wrong bus, or module aliases are absent.
- **API / Code Anchor:**
  ```bash
  ls /sys/bus/platform/drivers/
  ls /sys/bus/platform/devices/
  cat /sys/bus/platform/devices/<dev>/modalias
  modinfo my_driver.ko | grep alias
  find /proc/device-tree -name compatible -print
  dmesg -w
  ```
- **Production or Debugging Angle:** If the driver is built as a module and should autoload, `MODULE_DEVICE_TABLE(of, ...)` or the correct platform ID alias matters. If the device exists but has no `driver` symlink, binding did not happen.
- **Common Traps:**
  - Only checking `lsmod`.
  - Ignoring `/sys/bus/platform/devices/`.
  - Looking for an I2C/SPI client under platform bus.
  - Assuming a disabled DT node still creates a device.
  - Forgetting exact string matching for `compatible`.
- **Follow-up Questions:**
  - What does `modalias` tell you?
  - How can you manually bind a driver for debugging?
  - What would you check if the device is missing entirely?

### 11. Debugging Scenario: Probe Defers Forever

- **Level:** Senior
- **Question:** Your platform driver's `probe()` keeps returning `-EPROBE_DEFER`. What does that mean, and how do you handle it?
- **Short Answer:** A required supplier resource is not ready yet, such as a clock, regulator, GPIO controller, IRQ domain, reset controller, PHY, or parent device. Preserve the defer error, use good diagnostics, and fix the missing or failing supplier.
- **Deep Explanation:** Deferred probe is normal in the driver model. It lets a consumer driver retry after its suppliers become available. The bug is not the defer itself; the bug is hiding it, converting it to another error, using `platform_driver_probe()` where deferral is needed, or never enabling the supplier driver. Good code returns the supplier error and logs with `dev_err_probe()` when useful.
- **API / Code Anchor:**
  ```c
  clk = devm_clk_get(&pdev->dev, "core");
  if (IS_ERR(clk))
          return dev_err_probe(&pdev->dev, PTR_ERR(clk),
                               "failed to get core clock\n");
  ```
- **Production or Debugging Angle:** Check `dmesg`, `devices_deferred` under debugfs if available, and DT phandles. Verify supplier drivers are enabled in the kernel config and that provider nodes are not disabled.
- **Common Traps:**
  - Turning `-EPROBE_DEFER` into `-EINVAL`.
  - Treating every supplier as optional.
  - Using `platform_driver_probe()` for supplier-dependent devices.
  - Printing noisy errors on every defer retry.
  - Debugging only the consumer driver while the supplier failed earlier.
- **Follow-up Questions:**
  - Why is `platform_driver_probe()` dangerous here?
  - What kinds of resources commonly defer?
  - How does `dev_err_probe()` help?

### 12. Remove-Time Lifetime And Concurrency

- **Level:** Senior
- **Question:** What can go wrong in `remove()` for a platform driver, even if all allocations used `devm_*`?
- **Short Answer:** External activity can still reach driver state. IRQs, threaded handlers, workqueues, timers, DMA, runtime PM callbacks, and subsystem entry points must be stopped or unregistered in a safe order before managed resources disappear.
- **Deep Explanation:** Devm automates resource release but does not know your driver's semantic lifetime. If an input device, network device, character device, IIO device, or media device is still registered, callbacks can still arrive. If hardware IRQs or work items are still active, they may dereference private state while remove is unwinding. `remove()` must quiesce the device, unregister visibility, cancel asynchronous work, and then allow managed cleanup.
- **API / Code Anchor:**
  ```c
  static int foo_remove(struct platform_device *pdev)
  {
          struct foo *foo = platform_get_drvdata(pdev);

          foo_unregister_subsystem(foo);
          disable_device_interrupts(foo);
          cancel_work_sync(&foo->work);
          foo_hw_stop(foo);

          return 0; /* devm resources release after remove */
  }
  ```
- **Production or Debugging Angle:** Test unbind/bind when possible. Crashes during unbind often reveal lifetime bugs that normal reboot testing hides.
- **Common Traps:**
  - Empty `remove()` because "devm handles everything."
  - Not canceling work scheduled by an IRQ.
  - Unmapping registers before disabling hardware.
  - Freeing devm-managed memory manually.
  - Keeping userspace-visible objects registered too long.
- **Follow-up Questions:**
  - How would you test remove without rebooting?
  - What must happen before private state is freed?
  - Why can subsystem unregister order matter?

### 13. Platform Data Versus Device Tree

- **Level:** Senior
- **Question:** How should a modern platform driver think about legacy platform data versus Device Tree?
- **Short Answer:** Modern embedded drivers should normally use firmware description such as Device Tree or ACPI. Platform data is mainly for legacy board-file systems or compatibility. A driver may support both, but new board configuration should not be hardcoded into the driver.
- **Deep Explanation:** Platform data is a C pointer attached to `pdev->dev.platform_data`. It worked with board files but requires recompiling board-specific kernel code. Device Tree separates board hardware description from driver code and lets one driver support multiple boards. A compatibility driver may check `pdev->dev.of_node` or firmware helpers first, then fall back to `dev_get_platdata()`.
- **API / Code Anchor:**
  ```c
  if (pdev->dev.of_node) {
          data = of_device_get_match_data(&pdev->dev);
  } else {
          pdata = dev_get_platdata(&pdev->dev);
          if (!pdata)
                  return -EINVAL;
  }
  ```
- **Production or Debugging Angle:** When reviewing a new driver, reject hardcoded board constants unless there is a very strong reason. Addresses, IRQs, GPIOs, clocks, and regulators should come from resources or subsystem APIs.
- **Common Traps:**
  - Treating platform data as the modern default.
  - Parsing DT properties in a way that duplicates subsystem helpers.
  - Mixing board policy and driver behavior.
  - Breaking old non-DT users without an intentional compatibility decision.
  - Assuming DT is available in every target kernel configuration.
- **Follow-up Questions:**
  - What belongs in Device Tree and what belongs in driver code?
  - Why are board files considered legacy in most embedded systems?
  - How can one driver support multiple hardware variants cleanly?

### 14. MFD-Created Platform Devices

- **Level:** Senior
- **Question:** How can a platform device be created by another driver, such as an MFD core driver?
- **Short Answer:** An MFD parent driver can describe child functions as cells and create child platform devices with resources, parent linkage, platform data, or compatible strings. Child subsystem drivers then bind as platform drivers.
- **Deep Explanation:** Multi-function devices often expose several logical blocks: RTC, watchdog, GPIO, regulator, on-key, and so on. The parent MFD driver manages shared register access and creates child devices for each function. Those children are commonly platform devices because they are logical subdevices, not separately enumerable PCI/USB/I2C devices. The child driver uses normal platform APIs such as `platform_get_irq_byname()`.
- **API / Code Anchor:**
  ```c
  static const struct mfd_cell chip_cells[] = {
          {
                  .name = "chip-rtc",
                  .of_compatible = "vendor,chip-rtc",
                  .resources = rtc_resources,
                  .num_resources = ARRAY_SIZE(rtc_resources),
          },
  };

  ret = devm_mfd_add_devices(dev, PLATFORM_DEVID_AUTO,
                             chip_cells, ARRAY_SIZE(chip_cells),
                             NULL, 0, NULL);
  ```
- **Production or Debugging Angle:** Child resource failures may originate in the parent MFD resource mapping. Check the parent device, child device nodes, and whether the child compatible string matches the child platform driver.
- **Common Traps:**
  - Confusing MFD subdevices with V4L2 subdevices.
  - Assuming all platform devices come directly from top-level DT nodes.
  - Ignoring parent-child lifetime.
  - Accessing parent shared registers without the intended regmap/locking design.
  - Using name matching when compatible matching is needed for variants.
- **Follow-up Questions:**
  - Why are MFD children often platform devices?
  - How does a child driver get parent data?
  - What resources can an MFD cell provide?

### 15. Design Tradeoff: `platform_driver_probe()`

- **Level:** Senior
- **Question:** When would you use `platform_driver_probe()`, and why is it often the wrong choice?
- **Short Answer:** Use it only when the device is guaranteed to be present and all dependencies are ready at registration time. It is often wrong because it prevents normal later matching and deferred-probe behavior.
- **Deep Explanation:** `platform_driver_register()` keeps the driver registered so future devices can bind and deferred probe can retry. `platform_driver_probe()` is an older/special pattern that probes immediately and can allow init-section probe code in narrow cases. Modern embedded devices frequently depend on suppliers such as clocks, regulators, GPIO controllers, and IRQ domains. Those dependencies may not be ready when the driver registers.
- **API / Code Anchor:**
  ```c
  /* Usual pattern */
  module_platform_driver(foo_driver);

  /* Special case, not the normal default */
  ret = platform_driver_probe(&foo_driver, foo_probe);
  ```
- **Production or Debugging Angle:** If a device works only depending on init ordering, suspect misuse of immediate probing. A driver that needs `-EPROBE_DEFER` should remain registered.
- **Common Traps:**
  - Using `platform_driver_probe()` as a "simpler register."
  - Assuming all platform devices exist at boot.
  - Breaking hot-added or dynamically created platform devices.
  - Losing deferred probe retries.
  - Optimizing memory footprint before correctness.
- **Follow-up Questions:**
  - What happens if a supplier is not ready?
  - Why does ordinary `platform_driver_register()` support later matches?
  - What kind of old or special device might justify immediate probing?

### 16. Debugging Scenario: Resource Exists But Wrong Hardware Responds

- **Level:** Senior
- **Question:** A platform driver's `probe()` succeeds, but register reads return unexpected values or the wrong hardware seems to respond. What do you check?
- **Short Answer:** Check the resource address, size, resource index/name, DT parent address translation, `ranges`, compatible-to-variant data, clock/reset state, and whether another driver owns the same region.
- **Deep Explanation:** Successful mapping only means the kernel accepted a resource. It does not prove the resource describes the intended hardware. Device Tree address cells, parent bus `ranges`, wrong `reg` index, copy-pasted compatible strings, disabled clocks, or missing reset deassertion can all make a mapped region behave wrong. Variant match data may also select the wrong register layout.
- **API / Code Anchor:**
  ```c
  res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "control");
  dev_info(&pdev->dev, "resource %pr\n", res);

  base = devm_ioremap_resource(&pdev->dev, res);
  ```
- **Production or Debugging Angle:** Print `%pr` for resources, compare with the SoC reference manual, inspect `/proc/iomem`, and verify the runtime DT under `/proc/device-tree`. Also confirm required clocks and resets are enabled before touching registers.
- **Common Traps:**
  - Assuming index `0` is always the control register block.
  - Forgetting parent bus address translation.
  - Using compatible data for the wrong SoC revision.
  - Reading registers before enabling clocks.
  - Ignoring `reg-names`.
- **Follow-up Questions:**
  - Why can named resources be safer than indexes?
  - How do clocks affect register access?
  - What does `%pr` print for a resource?

## Common Trap Review

- **Trap:** "The platform bus is physical."  
  **Correction:** It is a software bus in the Linux device model.

- **Trap:** "Module loaded, so probe ran."  
  **Correction:** Module load registers the driver; only matching calls `probe()`.

- **Trap:** "I2C sensor equals platform device."  
  **Correction:** The SoC I2C controller may be platform; the sensor is an I2C client.

- **Trap:** "`devm_*` means no cleanup thinking."  
  **Correction:** Managed resources free memory/mappings/IRQs, but drivers still quiesce hardware and unregister subsystem objects.

- **Trap:** "Hardcoded addresses are fine for one board."  
  **Correction:** Production drivers should get addresses from resources or firmware.

- **Trap:** "`platform_driver_probe()` is just a shorter register function."  
  **Correction:** It changes matching/deferred-probe behavior and should be rare.

- **Trap:** "Device Tree parsing belongs everywhere in the driver."  
  **Correction:** Prefer platform and subsystem helpers for resources and suppliers; parse only real custom hardware properties.
