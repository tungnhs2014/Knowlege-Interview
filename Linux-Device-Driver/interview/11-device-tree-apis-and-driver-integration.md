# 11 - Device Tree APIs And Driver Integration Interview Questions

Strong candidates can reason from a DTS node to driver `probe()`, choose the right resource API, explain when to use raw OF parsing versus subsystem helpers, and debug failures without guessing.

## Beginner Questions

### 1. How does a Device Tree node cause a driver probe function to run?
**Short Answer:** The kernel creates a device from the DT node, the bus core compares the node's `compatible` property with the driver's match table, and if they match the driver's `probe()` is called.

**Deep Explanation:** Device Tree describes hardware as nodes and properties. During boot, the kernel unflattens the DTB and populates devices from enabled nodes. For platform devices, the platform bus gets a `struct platform_device`; for I2C and SPI, the relevant bus creates `struct i2c_client` or `struct spi_device`. The driver core then matches that device against registered drivers. OF matching checks `compatible` strings against `.of_match_table`.

**API / Code Anchor:**
```c
static const struct of_device_id my_of_match[] = {
    { .compatible = "vendor,mydev" },
    { }
};
MODULE_DEVICE_TABLE(of, my_of_match);

static struct platform_driver my_driver = {
    .probe = my_probe,
    .driver = {
        .name = "mydev",
        .of_match_table = my_of_match,
    },
};
```

**Production or Debugging Angle:** If `probe()` never runs, check the running DTB, `status`, `compatible`, bus placement, and module aliases before rewriting the driver.

**Common Traps:** Assuming DT directly calls the driver. The bus and driver core are the middle layers.

**Follow-up Questions:**
- What is the role of `MODULE_DEVICE_TABLE(of, ...)`?
- What changes for I2C or SPI devices?
- How would you confirm the running DT node exists?

### 2. What is `struct of_device_id` used for?
**Short Answer:** It declares which DT `compatible` strings a driver supports, and it can attach variant-specific driver data through `.data`.

**Deep Explanation:** Each entry in the table represents one hardware identity the driver can bind to. The `compatible` field is compared with strings in the node. The optional `.data` pointer lets the driver select register layouts, feature flags, operation tables, or quirks without inventing extra DT properties.

**API / Code Anchor:**
```c
static const struct of_device_id uart_match[] = {
    { .compatible = "vendor,uart-v1", .data = &uart_v1_data },
    { .compatible = "vendor,uart-v2", .data = &uart_v2_data },
    { }
};
```

**Production or Debugging Angle:** Match data keeps board files and bindings cleaner. If a difference is tied to hardware identity, it usually belongs in compatible-specific match data, not a random property such as `fifo-size`.

**Common Traps:** Forgetting the empty sentinel entry or using `.data` for board policy instead of hardware variant behavior.

**Follow-up Questions:**
- What kinds of data belong in `.data`?
- How can a driver retrieve match data in probe?
- Why should the compatible string be specific?

### 3. What is the difference between `of_property_read_u32()` and `platform_get_resource()`?
**Short Answer:** `of_property_read_u32()` reads a typed property from a DT node; `platform_get_resource()` retrieves a standard platform resource already translated from DT or board data.

**Deep Explanation:** `reg`, interrupts, and some DMA/platform resources are standard resources. Platform helpers understand how the platform device was populated and return a `struct resource` or Linux IRQ number. `of_property_read_u32()` is for custom binding properties such as `vendor,timeout-ms`, not for manually decoding every standard resource.

**API / Code Anchor:**
```c
base = devm_platform_ioremap_resource(pdev, 0);
irq = platform_get_irq(pdev, 0);
ret = of_property_read_u32(dev->of_node, "vendor,timeout-ms", &timeout);
```

**Production or Debugging Angle:** Manually parsing `reg` in a platform driver often duplicates bus logic and can get address translation wrong.

**Common Traps:** Treating all DT properties as arbitrary values the driver should parse itself.

**Follow-up Questions:**
- What helper would you use for named memory resources?
- What helper consumes the `interrupts` property?
- When is a typed property reader appropriate?

### 4. Why is `MODULE_DEVICE_TABLE(of, table)` important?
**Short Answer:** It exports OF match aliases so userspace/module loading tools can autoload the driver when a matching device appears.

**Deep Explanation:** The driver can still compile and may bind if already loaded, but module autoload depends on generated alias information. `depmod` collects module aliases, and device manager logic can load the right module for a device.

**API / Code Anchor:**
```c
static const struct of_device_id my_of_match[] = {
    { .compatible = "vendor,mydev" },
    { }
};
MODULE_DEVICE_TABLE(of, my_of_match);
```

**Production or Debugging Angle:** If a built-in driver probes but the module version does not autoload, missing module aliases are a prime suspect.

**Common Traps:** Thinking `.of_match_table` alone is enough for module autoload in all cases.

**Follow-up Questions:**
- How would you inspect aliases with `modinfo`?
- Does this matter for built-in drivers?
- What other buses use `MODULE_DEVICE_TABLE()`?

## Mid-Level Questions

### 5. In a platform driver, how do you get MMIO and IRQ resources from Device Tree?
**Short Answer:** Use platform resource helpers: `devm_platform_ioremap_resource()` or `platform_get_resource()` plus `devm_ioremap_resource()` for MMIO, and `platform_get_irq()` or `platform_get_irq_byname()` for IRQs.

**Deep Explanation:** When a DT node is populated as a platform device, the kernel translates properties such as `reg` and `interrupts` into platform resources. The driver should consume those resources through platform APIs. These helpers also work for some non-DT platform-data cases, which keeps the driver less tightly coupled to raw DT parsing.

**API / Code Anchor:**
```c
void __iomem *base;
int irq;

base = devm_platform_ioremap_resource(pdev, 0);
if (IS_ERR(base))
    return PTR_ERR(base);

irq = platform_get_irq(pdev, 0);
if (irq < 0)
    return irq;
```

**Production or Debugging Angle:** Always check negative IRQ returns. Passing a negative value to `request_irq()` is a classic sloppy error path.

**Common Traps:** Ignoring `reg-names`/`interrupt-names` and relying on fragile index order.

**Follow-up Questions:**
- When should you use `platform_get_irq_byname()`?
- Why is `%pR` useful in debug logs?
- What can make `platform_get_irq()` return `-EPROBE_DEFER`?

### 6. How should a driver read optional and mandatory DT properties?
**Short Answer:** Mandatory properties should fail probe with a clear error when missing; optional properties should have binding-documented defaults.

**Deep Explanation:** Typed helpers return 0 on success or a negative error on failure. A mandatory property is part of the hardware contract, so continuing without it is unsafe. Optional properties must be handled deliberately, not accidentally ignored.

**API / Code Anchor:**
```c
ret = of_property_read_u32(np, "vendor,clock-rate", &rate);
if (ret)
    return dev_err_probe(dev, ret, "missing vendor,clock-rate\n");

if (of_property_read_u32(np, "vendor,timeout-ms", &timeout))
    timeout = 100;
```

**Production or Debugging Angle:** Good error messages save bring-up time. A missing mandatory property should point to the exact property name.

**Common Traps:** Reading a property, ignoring the return value, and then using uninitialized data.

**Follow-up Questions:**
- How are boolean properties represented?
- Why are typed helpers better than `of_get_property()`?
- What should a binding say about optional defaults?

### 7. What is `of_parse_phandle_with_args()`, and when should a normal driver avoid it?
**Short Answer:** It parses a phandle plus provider-defined argument cells, but ordinary consumer drivers should usually use subsystem helpers instead.

**Deep Explanation:** Many DT relationships use the same shape: the provider declares `#*-cells`, and the consumer references it with a phandle plus arguments. `of_parse_phandle_with_args()` decodes that raw tuple. However, clocks, GPIOs, regulators, DMA, resets, pinctrl, and NVMEM have subsystem APIs that understand binding details and provider readiness.

**API / Code Anchor:**
```c
ret = of_parse_phandle_with_args(np, "clocks", "#clock-cells", 0, &args);
if (!ret)
    of_node_put(args.np);

clk = devm_clk_get(dev, "core"); /* preferred for clock consumers */
```

**Production or Debugging Angle:** Subsystem helpers preserve `-EPROBE_DEFER` and hide provider-specific parsing details.

**Common Traps:** Raw-parsing a GPIO or clock and bypassing the framework that owns lifetime, polarity, flags, and provider lookup.

**Follow-up Questions:**
- What does `#gpio-cells` mean?
- Who must call `of_node_put()`?
- Why is `devm_gpiod_get()` better than parsing `reset-gpios` yourself?

### 8. How do named resources prevent bugs?
**Short Answer:** They let drivers request resources by purpose, such as `"rx"` or `"control"`, instead of relying only on list order.

**Deep Explanation:** DT lists can contain several memory regions, interrupts, clocks, or DMA channels. If the driver assumes index 0 means one thing and the DTS author orders it differently, the driver may map the wrong register block or request the wrong IRQ. Names make the binding explicit.

**API / Code Anchor:**
```dts
reg = <0x80000000 0x1000>, <0x80001000 0x100>;
reg-names = "control", "status";

interrupts = <0 56 IRQ_TYPE_LEVEL_HIGH>, <0 57 IRQ_TYPE_LEVEL_HIGH>;
interrupt-names = "rx", "tx";
```

```c
res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "control");
irq = platform_get_irq_byname(pdev, "rx");
```

**Production or Debugging Angle:** Named resources make code review easier and reduce silent board-porting bugs.

**Common Traps:** Updating `reg` or `interrupts` order without updating the corresponding `*-names`.

**Follow-up Questions:**
- What APIs use `clock-names`?
- What is the GPIO equivalent of named resources?
- When is index-based lookup acceptable?

### 9. How do GPIOs from Device Tree map to modern driver APIs?
**Short Answer:** DT properties are usually named `<function>-gpios`, and new drivers request descriptors with `gpiod_get()` or `devm_gpiod_get()`.

**Deep Explanation:** A node may define `reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;`. The driver asks for the function name `"reset"`, and gpiolib parses the property, handles active-low semantics, requests the line, and returns a `struct gpio_desc *`. This replaces legacy integer GPIO APIs in new code.

**API / Code Anchor:**
```dts
reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
```

```c
reset = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
if (IS_ERR(reset))
    return dev_err_probe(dev, PTR_ERR(reset), "reset GPIO\n");
```

**Production or Debugging Angle:** Descriptor APIs preserve polarity and lifetime semantics. Integer APIs often lose binding context and are discouraged for new code.

**Common Traps:** Looking for `"reset-gpio"` in code when the DT property is `"reset-gpios"`, or manually inverting active-low lines.

**Follow-up Questions:**
- What does `GPIOD_OUT_LOW` mean?
- How does active-low affect `gpiod_set_value()`?
- When is `devm_gpiod_get_optional()` appropriate?

### 10. Debug scenario: probe returns `-EPROBE_DEFER` while getting a clock. What do you check?
**Short Answer:** Check that the clock provider node exists, is enabled, matches its driver, and that the consumer property and `clock-names` match the driver's request.

**Deep Explanation:** `-EPROBE_DEFER` means the dependency may become available later. For clocks, `devm_clk_get(dev, "core")` depends on `clocks = <...>` and often `clock-names = "core"`. The provider must also probe and register its clocks. Converting the error to `-EINVAL` would break deferred probing.

**API / Code Anchor:**
```c
clk = devm_clk_get(dev, "core");
if (IS_ERR(clk))
    return dev_err_probe(dev, PTR_ERR(clk), "core clock\n");
```

**Production or Debugging Angle:** `dev_err_probe()` keeps logs useful for repeated deferrals and preserves the real errno.

**Common Traps:** Adding sleeps or probe ordering hacks instead of fixing the provider/consumer relationship.

**Follow-up Questions:**
- What other provider APIs may return `-EPROBE_DEFER`?
- How does `clock-names` connect to `devm_clk_get()`?
- Why should drivers preserve `-EPROBE_DEFER`?

## Senior Questions

### 11. When should a hardware difference be represented as match data instead of a DT property?
**Short Answer:** If the difference follows from the hardware compatible string, use match data. If it describes board wiring or a binding-defined hardware parameter, use a DT property.

**Deep Explanation:** Match data is C-side knowledge tied to a hardware variant: register offsets, FIFO depth, feature flags, quirks, or operation callbacks. DT properties should describe facts that can vary by board or are part of the hardware binding: GPIO wiring, clock names, supply phandles, interrupt lines, calibration cells, or documented hardware parameters.

**API / Code Anchor:**
```c
static const struct of_device_id my_match[] = {
    { .compatible = "vendor,ip-v1", .data = &ip_v1_data },
    { .compatible = "vendor,ip-v2", .data = &ip_v2_data },
    { }
};
```

**Production or Debugging Angle:** Random DT knobs create unstable bindings. Once a binding is shipped, it becomes a long-lived contract.

**Common Traps:** Adding `vendor,has-dma` when `"vendor,ip-v2"` already implies DMA support, or hiding board wiring in match data.

**Follow-up Questions:**
- What belongs in a binding schema?
- How would you support two register layouts?
- Why are bindings hard to change later?

### 12. How do OF-only APIs compare with fwnode/device-property APIs?
**Short Answer:** OF APIs are Device Tree-specific; fwnode and device-property APIs abstract firmware properties across DT, ACPI, and software nodes.

**Deep Explanation:** `struct device_node` represents a DT node. `struct fwnode_handle` is a generic firmware-node handle embedded by OF and ACPI representations. If a driver should work only on DT systems, `of_property_read_*()` may be fine. If it should support multiple firmware descriptions, prefer `device_property_read_*()` or `fwnode_property_read_*()` and generic match-data helpers where suitable.

**API / Code Anchor:**
```c
struct fwnode_handle *fwnode = dev_fwnode(dev);
ret = fwnode_property_read_u32(fwnode, "clock-frequency", &freq);

ret = device_property_read_u32(dev, "clock-frequency", &freq);
```

**Production or Debugging Angle:** Media graph and cross-platform drivers increasingly use fwnode APIs to avoid duplicating OF and ACPI parsing paths.

**Common Traps:** Rewriting a simple DT-only SoC driver for fwnode without benefit, or writing OF-only code in a driver expected to run on ACPI platforms.

**Follow-up Questions:**
- What is `dev_fwnode()`?
- When is `to_of_node()` useful?
- Which subsystems commonly use firmware graph APIs?

### 13. What lifetime mistakes can happen with Device Tree node pointers?
**Short Answer:** Lookup helpers can return referenced `struct device_node *` objects; the driver must release those references with `of_node_put()` on all paths.

**Deep Explanation:** APIs such as `of_parse_phandle()`, `of_parse_phandle_with_args()`, and child lookup helpers can increment node references. If an error path returns without dropping the reference, the driver leaks it. With overlays or dynamic DT changes, stale pointers are even more dangerous.

**API / Code Anchor:**
```c
struct device_node *child;

child = of_get_child_by_name(np, "channels");
if (!child)
    return -ENOENT;

ret = parse_channels(child);
of_node_put(child);
return ret;
```

**Production or Debugging Angle:** In review, every node-returning helper should make you look for the matching put.

**Common Traps:** Returning from inside `for_each_child_of_node()` on error without releasing the current child reference.

**Follow-up Questions:**
- Which APIs return referenced nodes?
- How do overlays make lifetime more subtle?
- How would you structure error handling to avoid leaks?

### 14. Debug scenario: an I2C driver probes but talks to the wrong chip. What DT integration issues would you inspect?
**Short Answer:** Check that the device node is under the correct I2C controller, `reg` is the correct slave address, the compatible matches the intended driver, and board-level mux/pinctrl/power setup is correct.

**Deep Explanation:** I2C devices are children of their controller node, and their `reg` is the I2C address, not a memory address. A wrong parent bus, address conflict, stale compatible, or missing regulator/reset/pinctrl setup can make communication fail even though `probe()` runs.

**API / Code Anchor:**
```dts
&i2c2 {
    status = "okay";

    sensor@48 {
        compatible = "vendor,temp-sensor";
        reg = <0x48>;
    };
};
```

**Production or Debugging Angle:** Use `i2cdetect` carefully, inspect `/sys/bus/i2c/devices/`, check schematics, and verify the booted DTB.

**Common Traps:** Confusing I2C `reg` with platform MMIO `reg`, or placing the child under the wrong adapter.

**Follow-up Questions:**
- How is SPI `reg` different?
- Why can `i2cdetect` be unsafe for some devices?
- What should the driver do if the device ID register does not match?

### 15. How would you review a DT-backed driver for production readiness?
**Short Answer:** I would review matching, bindings, resource acquisition, provider dependencies, error paths, lifetime, debug logs, and whether the driver keeps board policy out of C.

**Deep Explanation:** Production-ready DT integration is not just "probe works." The driver must use documented compatibles, correct module tables, typed property parsing, subsystem helpers, named resources, managed cleanup, and proper handling of provider deferral. It must also avoid stale APIs and keep bindings stable.

**API / Code Anchor:** Review checklist anchors:
```c
MODULE_DEVICE_TABLE(of, table);
devm_platform_ioremap_resource(pdev, 0);
platform_get_irq_byname(pdev, "rx");
devm_clk_get(dev, "core");
devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
dev_err_probe(dev, err, "provider\n");
```

**Production or Debugging Angle:** Good DT integration makes board bring-up explainable. Bad integration creates hidden assumptions that fail on the second board.

**Common Traps:** Accepting arbitrary custom properties without binding documentation, losing `-EPROBE_DEFER`, or leaving manually enabled clocks on after probe failure.

**Follow-up Questions:**
- What would you ask the DTS author to validate?
- How do you decide between indexed and named resources?
- What is one stale API you would flag in new GPIO consumer code?

### 16. Debug scenario: a driver works when built-in but not as a module. What DT-related cause is likely?
**Short Answer:** The driver may be missing module alias metadata, especially `MODULE_DEVICE_TABLE(of, match_table)`.

**Deep Explanation:** A built-in driver is already registered during boot, so it can bind when the device appears. A module must be autoloaded by matching alias information. Without exported OF aliases, userspace may not know which module to load for the DT device.

**API / Code Anchor:**
```bash
modinfo my_driver.ko | grep -i alias
```

```c
MODULE_DEVICE_TABLE(of, my_of_match);
```

**Production or Debugging Angle:** This is a classic difference between "driver code is correct" and "deployment integration is correct."

**Common Traps:** Debugging only the DTS and probe function while ignoring module metadata.

**Follow-up Questions:**
- What other reasons can built-in versus module behavior differ?
- How does `depmod` fit into module autoload?
- Does `MODULE_DEVICE_TABLE(platform, ...)` solve OF matching?

