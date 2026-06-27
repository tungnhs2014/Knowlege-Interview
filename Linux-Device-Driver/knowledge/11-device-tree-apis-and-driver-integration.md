# 11 - Device Tree APIs And Driver Integration

## Learning Goal
After this topic, you should be able to explain how a Device Tree node becomes a probed driver instance, choose the right helper APIs to get resources and properties, use DT match data for hardware variants, and debug common probe failures caused by bad bindings or stale driver code.

You do not need to memorize every `of_*()` function. The useful skill is knowing **which layer should parse which part of the Device Tree**.

## Why This Matters In Real Work
Device Tree fundamentals teach you how hardware is described. Driver integration teaches you how the kernel and driver consume that description during probe.

This matters because many bring-up failures sit exactly at this boundary:

- A driver never probes because `compatible` does not match.
- A module does not autoload because `MODULE_DEVICE_TABLE(of, ...)` is missing.
- Register mapping fails because `reg` is wrong or the wrong helper is used.
- IRQ request fails because the DT interrupt specifier or provider is wrong.
- Probe returns `-EPROBE_DEFER` because a clock, GPIO, regulator, reset, or DMA provider is not ready.
- A driver works on one board but breaks another because it encoded board policy in C instead of using match data and binding-defined properties.

**Production rule:** use Device Tree to describe hardware facts and wiring; use driver code to implement behavior.

## Mental Model
Think of Device Tree integration as a handoff chain. Each layer translates a piece of the hardware description into the kernel object the next layer expects.

```
DTS node
  -> kernel unflattens it into struct device_node
  -> bus code creates a device object
  -> driver core matches compatible against driver tables
  -> probe() receives a struct platform_device / i2c_client / spi_device
  -> driver gets resources through helper APIs
  -> driver initializes hardware and registers with a framework
```

The trap is trying to make the driver parse everything manually. Good drivers let the bus core and subsystem APIs do the boring translation.

| DT data | Preferred driver-side access |
| --- | --- |
| `compatible` | `struct of_device_id`, match-data helpers |
| `reg` for platform MMIO | `platform_get_resource()`, `devm_platform_ioremap_resource()` |
| `interrupts` | `platform_get_irq()`, `platform_get_irq_byname()` |
| `clocks` / `clock-names` | `devm_clk_get()` |
| `reset-gpios` | `devm_gpiod_get(dev, "reset", ...)` |
| `*-supply` | `devm_regulator_get()` |
| custom binding property | typed property helpers, after checking the binding |

## Core Concepts
The same few concepts appear in platform, I2C, SPI, GPIO, clock, regulator, DMA, media, and power-management drivers.

| Concept | Meaning | Why it matters |
| --- | --- | --- |
| OF | Open Firmware heritage of Device Tree APIs; many functions start with `of_` | Explains names such as `of_match_table` |
| Firmware node | Generic firmware-backed node, represented by `struct fwnode_handle` | Lets newer code work across DT, ACPI, and software nodes |
| Match table | Driver table of supported hardware identities | Controls probe and module aliases |
| Match data | Variant-specific C data attached to a compatible string | Avoids random DT properties for driver quirks |
| Platform resource | Kernel object describing MMIO, IRQ, DMA, etc. | Lets drivers use common helpers instead of raw DT parsing |
| Provider/consumer | One node provides a resource; another references it by phandle | Explains clocks, GPIOs, regulators, DMA, resets, NVMEM |
| Named resource | Resource list plus `*-names` property | Avoids index-order bugs |
| Deferred probe | Probe waits because a referenced provider is not ready | Common and normal in real systems |

### OF-Only vs Firmware-Generic APIs
Some drivers only support Device Tree. Others should work with Device Tree, ACPI, or software nodes.

| Style | Typical APIs | Use when |
| --- | --- | --- |
| OF-only | `dev->of_node`, `of_property_read_u32()`, `of_parse_phandle()` | DT-only platform/SoC driver |
| Firmware-generic | `dev_fwnode()`, `device_property_read_u32()`, `fwnode_property_read_u32()` | Driver should not care whether firmware is DT or ACPI |
| Subsystem helper | `devm_clk_get()`, `devm_gpiod_get()`, `devm_regulator_get()` | A framework already owns the parsing rules |

**Rule of thumb:** use subsystem helpers first, firmware-generic property helpers second, raw OF helpers only when you really need OF-specific behavior.

## Kernel Mechanism
At boot, the kernel receives a flattened DTB and turns it into an in-memory tree. Each enabled hardware node can then be converted into a device on the appropriate bus.

For a simple memory-mapped platform device:

```
mydev@80000000 {
    compatible = "acme,mydev-v2";
    reg = <0x80000000 0x1000>;
    interrupts = <0 56 IRQ_TYPE_LEVEL_HIGH>;
    clocks = <&clk 12>;
    clock-names = "core";
};
```

The rough kernel flow is:

- The DT core stores the node as a `struct device_node`.
- OF/platform code creates a `struct platform_device`.
- `pdev->dev.of_node` points back to the DT node.
- The platform bus calls its match function.
- OF matching compares the node `compatible` list with `driver.of_match_table`.
- If a match happens, `probe()` receives the `struct platform_device`.
- `platform_get_resource()` and `platform_get_irq()` return resources already translated from DT.

For I2C and SPI, the idea is the same, but the bus creates bus-native devices:

| Bus | DT child node means | Probe object | `reg` meaning |
| --- | --- | --- | --- |
| Platform/simple-bus | MMIO or SoC-integrated block | `struct platform_device *pdev` | address and size, interpreted by parent cells |
| I2C | I2C slave/client under an adapter | `struct i2c_client *client` | I2C slave address |
| SPI | SPI peripheral under a controller | `struct spi_device *spi` | chip-select index |

## Key Structs And APIs
These names are worth recognizing because they appear in almost every DT-capable driver.

### Matching
The match table is the bridge between the DT `compatible` string and the driver.

```c
static const struct of_device_id acme_mydev_of_match[] = {
    { .compatible = "acme,mydev-v1", .data = &acme_mydev_v1 },
    { .compatible = "acme,mydev-v2", .data = &acme_mydev_v2 },
    { }
};
MODULE_DEVICE_TABLE(of, acme_mydev_of_match);
```

| API / field | Role |
| --- | --- |
| `struct of_device_id` | One compatible string plus optional `.data` |
| `.driver.of_match_table` | Exposes the table to driver matching |
| `MODULE_DEVICE_TABLE(of, table)` | Generates module aliases for autoload |
| `of_match_device()` | Finds the matching table entry for a `struct device` |
| `device_get_match_data()` | Firmware-generic way to fetch match data where available |

**Use `.data` for hardware variants**: register layout, FIFO depth, feature flags, operation tables, or quirks known by compatible string.

### Standard Resources
For standard platform resources, do not manually walk `reg` or `interrupts`.

| Need | Preferred API |
| --- | --- |
| MMIO resource | `platform_get_resource(pdev, IORESOURCE_MEM, index)` |
| Named MMIO resource | `platform_get_resource_byname(pdev, IORESOURCE_MEM, "control")` |
| Map MMIO | `devm_ioremap_resource(dev, res)` |
| Map platform MMIO directly | `devm_platform_ioremap_resource(pdev, index)` |
| IRQ by index | `platform_get_irq(pdev, index)` |
| IRQ by name | `platform_get_irq_byname(pdev, "rx")` |

Named resources are worth using when the hardware has multiple similar resources:

```dts
reg = <0x80000000 0x1000>, <0x80001000 0x100>;
reg-names = "control", "status";

interrupts = <0 56 IRQ_TYPE_LEVEL_HIGH>, <0 57 IRQ_TYPE_LEVEL_HIGH>;
interrupt-names = "rx", "tx";
```

### Typed Property Readers
Use typed helpers for custom binding properties. They validate size and encoding better than raw property access.

| Property type | API |
| --- | --- |
| Required `u32` | `of_property_read_u32(np, "prop", &val)` |
| Indexed cell | `of_property_read_u32_index(np, "prop", index, &val)` |
| Array | `of_property_read_u32_array(np, "prop", values, count)` |
| Count cells | `of_property_count_u32_elems(np, "prop")` |
| String | `of_property_read_string(np, "prop", &str)` |
| String array | `of_property_read_string_array(np, "prop", strs, count)` |
| Boolean | `of_property_read_bool(np, "prop")` |

Boolean properties are **true by presence**:

```dts
acme,use-dma;
```

```c
priv->use_dma = of_property_read_bool(np, "acme,use-dma");
```

### Child Nodes And References
Some devices describe sub-blocks, partitions, channels, endpoints, or cells as child nodes.

| API | Role |
| --- | --- |
| `for_each_child_of_node(parent, child)` | Iterate child nodes |
| `of_get_child_by_name(parent, "name")` | Get a named child |
| `of_get_child_count(np)` | Count child nodes |
| `of_node_put(np)` | Drop a reference returned by lookup helpers |

**Lifetime rule:** if an OF helper returns a node with a reference, drop it with `of_node_put()` on every path, including errors.

### Phandle Parsing
Phandles connect consumers to providers.

```dts
clk: clock-controller@10000000 {
    #clock-cells = <1>;
};

mydev@80000000 {
    clocks = <&clk 12>;
    clock-names = "core";
};
```

The raw parser is:

```c
struct of_phandle_args args;

ret = of_parse_phandle_with_args(np, "clocks", "#clock-cells", 0, &args);
if (ret)
    return ret;

/* args.np is provider node, args.args[0] is 12 */
of_node_put(args.np);
```

But most drivers should use:

```c
priv->core_clk = devm_clk_get(dev, "core");
```

**Production rule:** raw phandle parsing is for framework/provider code or unusual bindings. Consumer drivers should prefer subsystem APIs.

## Lifecycle / Data Flow
A DT-backed driver usually follows a predictable probe sequence.

```
1. Driver module registers with platform_driver / i2c_driver / spi_driver
2. Bus core matches device against .of_match_table
3. probe() starts
4. Driver allocates private state
5. Driver gets match data for hardware variant
6. Driver gets resources:
   - MMIO
   - IRQ
   - clocks
   - GPIO descriptors
   - regulators
   - resets
   - DMA channels
7. Driver reads custom binding properties
8. Driver initializes hardware
9. Driver registers with a subsystem
10. On failure, managed resources unwind and manual state is disabled
```

Probe ordering is not guaranteed across independent devices. If your device references a provider that is not ready, the provider API may return `-EPROBE_DEFER`. That is not automatically a bug; it is the kernel saying "try this probe again later."

### Error-Path Pattern
Managed resources reduce cleanup code, but they do not replace all cleanup.

| Resource/state | Typical cleanup |
| --- | --- |
| `devm_kzalloc()` | automatic |
| `devm_platform_ioremap_resource()` | automatic |
| `devm_request_irq()` | automatic |
| `devm_clk_get()` | automatic for the handle |
| `clk_prepare_enable()` | must disable, or use a devm action/helper if available |
| hardware block enabled | driver must put hardware into safe state |
| framework registration | often needs unregister unless devm variant exists |

## Minimal Practical Example
This is a learning-only platform-driver skeleton. It is realistic enough to show the integration pattern, but it omits real register definitions, runtime PM, binding YAML, and hardware-specific sequencing.

```c
#include <linux/clk.h>
#include <linux/gpio/consumer.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

struct acme_variant {
    u32 fifo_size;
    bool has_dma;
};

static const struct acme_variant acme_v1 = {
    .fifo_size = 16,
    .has_dma = false,
};

static const struct acme_variant acme_v2 = {
    .fifo_size = 64,
    .has_dma = true,
};

struct acme_priv {
    void __iomem *base;
    int irq;
    struct clk *core_clk;
    struct gpio_desc *reset_gpiod;
    const struct acme_variant *variant;
    u32 timeout_ms;
};

static const struct of_device_id acme_of_match[] = {
    { .compatible = "acme,mydev-v1", .data = &acme_v1 },
    { .compatible = "acme,mydev-v2", .data = &acme_v2 },
    { }
};
MODULE_DEVICE_TABLE(of, acme_of_match);

static irqreturn_t acme_irq(int irq, void *data)
{
    struct acme_priv *priv = data;

    /* Acknowledge hardware interrupt here. */
    readl(priv->base);
    return IRQ_HANDLED;
}

static int acme_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct acme_priv *priv;
    const struct of_device_id *match;
    int ret;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    match = of_match_device(acme_of_match, dev);
    if (!match)
        return -ENODEV;
    priv->variant = match->data;

    priv->base = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(priv->base))
        return PTR_ERR(priv->base);

    priv->irq = platform_get_irq(pdev, 0);
    if (priv->irq < 0)
        return priv->irq;

    ret = devm_request_irq(dev, priv->irq, acme_irq, 0, dev_name(dev), priv);
    if (ret)
        return ret;

    priv->core_clk = devm_clk_get(dev, "core");
    if (IS_ERR(priv->core_clk))
        return dev_err_probe(dev, PTR_ERR(priv->core_clk), "core clock\n");

    ret = clk_prepare_enable(priv->core_clk);
    if (ret)
        return ret;

    priv->reset_gpiod = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
    if (IS_ERR(priv->reset_gpiod)) {
        ret = dev_err_probe(dev, PTR_ERR(priv->reset_gpiod), "reset GPIO\n");
        goto err_clk;
    }

    if (of_property_read_u32(dev->of_node, "acme,timeout-ms",
                             &priv->timeout_ms))
        priv->timeout_ms = 100;

    platform_set_drvdata(pdev, priv);

    /* Configure hardware using priv->variant and priv->timeout_ms. */
    return 0;

err_clk:
    clk_disable_unprepare(priv->core_clk);
    return ret;
}

static void acme_remove(struct platform_device *pdev)
{
    struct acme_priv *priv = platform_get_drvdata(pdev);

    /* Put hardware into a safe state here. */
    clk_disable_unprepare(priv->core_clk);
}

static struct platform_driver acme_driver = {
    .probe = acme_probe,
    .remove = acme_remove,
    .driver = {
        .name = "acme-mydev",
        .of_match_table = acme_of_match,
    },
};
module_platform_driver(acme_driver);

MODULE_LICENSE("GPL");
```

Matching DT snippet:

```dts
mydev@80000000 {
    compatible = "acme,mydev-v2";
    reg = <0x80000000 0x1000>;
    interrupts = <0 56 IRQ_TYPE_LEVEL_HIGH>;

    clocks = <&clk 12>;
    clock-names = "core";

    reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
    acme,timeout-ms = <250>;
};
```

Important lines:

- `of_device_id.data` carries hardware-variant data.
- `MODULE_DEVICE_TABLE(of, ...)` supports module autoload aliases.
- `devm_platform_ioremap_resource()` consumes `reg`.
- `platform_get_irq()` consumes `interrupts`.
- `devm_clk_get(dev, "core")` consumes `clocks` plus `clock-names`.
- `devm_gpiod_get_optional(dev, "reset", ...)` consumes `reset-gpios`.
- `of_property_read_u32()` reads the device-specific binding property.

## Common Bugs And Debugging
Start from the symptom. DT integration bugs often look like driver bugs until you inspect the binding path.

### Probe Never Runs
Likely causes:

- Node is missing from the running DTB.
- `status = "disabled"` or board `.dts` never enabled the node.
- `compatible` string does not match the driver's OF table.
- Driver module did not autoload because `MODULE_DEVICE_TABLE(of, ...)` is missing.
- Device is on the wrong bus node, such as an I2C chip not placed under the I2C controller.

Evidence to inspect:

```bash
find /sys/firmware/devicetree/base -name compatible
ls /sys/bus/platform/devices/
ls /sys/bus/i2c/devices/
ls /sys/bus/spi/devices/
modinfo my_driver.ko | grep -i alias
dmesg -w
```

Fix pattern:

- Verify the booted DTB, not just the source file.
- Compare `compatible` exactly.
- Ensure the node is enabled and under the correct parent bus.

### Probe Runs But MMIO Mapping Fails
Likely causes:

- `reg` has the wrong address/size.
- Parent `#address-cells` or `#size-cells` is wrong.
- Driver asks for resource index 1 when only index 0 exists.
- Resource names do not match `reg-names`.

Debug clues:

- Print resource with `%pR`.
- Decompile the running DTB.
- Check `reg` against the hardware reference manual and parent bus cells.

Fix pattern:

- Use `devm_platform_ioremap_resource()` for simple indexed MMIO.
- Use `platform_get_resource_byname()` when the binding provides `reg-names`.

### IRQ Request Fails Or IRQ Never Fires
Likely causes:

- `interrupt-parent` points to the wrong controller.
- Interrupt cell format does not match that controller binding.
- Trigger type is wrong.
- Driver ignores negative return from `platform_get_irq()`.
- GPIO interrupt provider is not ready, producing `-EPROBE_DEFER`.

Debug clues:

```bash
cat /proc/interrupts
dmesg | grep -i irq
```

Fix pattern:

- Check `interrupts`, `interrupt-parent`, and `interrupt-names`.
- Use `platform_get_irq_byname()` for multiple IRQs.
- Return errors cleanly; do not pass negative IRQ numbers to `request_irq()`.

### Provider Lookup Returns `-EPROBE_DEFER`
Likely causes:

- Clock, regulator, GPIO, DMA, reset, or NVMEM provider has not probed yet.
- Provider node is disabled or has a bad compatible.
- Consumer property has the wrong name or wrong phandle arguments.

Debug clues:

- Use `dev_err_probe()` so repeated deferrals are logged sanely.
- Check provider node `status`.
- Check the consumer property name: `core` in `devm_clk_get(dev, "core")` maps to `clock-names = "core"`.

Fix pattern:

- Return `-EPROBE_DEFER`; do not convert it to `-EINVAL`.
- Fix provider binding/population rather than adding arbitrary delays.

### Custom Property Reads Wrong Data
Likely causes:

- Property is absent but the driver treats it as mandatory.
- Property name differs by vendor prefix or spelling.
- Driver uses the wrong type or count.
- Driver uses unprefixed ad hoc properties.

Fix pattern:

- Use typed helpers.
- Define mandatory versus optional properties.
- Use defaults only for optional properties.
- Add/update binding schema and validate with `dtbs_check`.

## Production Checklist
Use this before sending a DT-backed driver for review or during board bring-up.

- Matching:
  - `compatible` strings are specific and documented.
  - Driver has a sentinel-terminated OF match table.
  - Module driver includes `MODULE_DEVICE_TABLE(of, table)` when it can be built as a module.
  - Variant differences use match data when they are hardware identity differences.
- Resources:
  - MMIO resources are obtained through platform helpers.
  - IRQs are checked for negative errors before request.
  - Multiple resources use `*-names` and by-name helpers.
  - Standard providers are consumed through subsystem APIs.
- Properties:
  - Custom properties are binding-defined, vendor-prefixed when device-specific, and typed.
  - Mandatory properties fail probe with clear errors.
  - Optional properties have documented defaults.
  - Boolean properties are treated as presence/absence.
- Lifetime and error paths:
  - Returned OF node references are released with `of_node_put()`.
  - `-EPROBE_DEFER` is preserved.
  - `devm_*` is used where appropriate.
  - Manually enabled state, such as clocks or hardware blocks, is disabled on failures and remove.
- Debuggability:
  - Probe logs identify the failing provider/resource without becoming noisy.
  - `dev_err_probe()` is used for provider lookup failures.
  - Runtime DT, bus devices, module aliases, and `/proc/interrupts` can be inspected.
- Boundaries:
  - DT describes hardware, not arbitrary Linux policy.
  - Framework-specific details are left to the relevant subsystem binding.

## Interview Readiness
For interviews, you should be able to reason through a failed probe using the DT node, driver match table, and probe code side by side.

Be ready to explain:

- How `compatible` causes `probe()` to run.
- Why `MODULE_DEVICE_TABLE(of, ...)` matters.
- Why `platform_get_resource()` is different from `of_property_read_u32()`.
- How `platform_get_irq()` hides interrupt-tree parsing.
- Why raw phandle parsing is usually not what a consumer driver should do.
- How match data differs from custom DT properties.
- Why integer GPIO APIs are legacy for new code.
- What `-EPROBE_DEFER` means and how to debug it.

See `interview/11-device-tree-apis-and-driver-integration.md` for practice questions.

## Kernel Version Notes
Device Tree APIs are stable, but driver style changes over time.

- New GPIO consumer code should use descriptor APIs such as `gpiod_get()` and `devm_gpiod_get()`, not legacy integer GPIO helpers.
- Platform drivers can use newer wrappers such as `devm_platform_ioremap_resource()` instead of open-coding `platform_get_resource()` plus `devm_ioremap_resource()` when no separate `struct resource *` is needed.
- Some older I2C material required both `.id_table` and `.of_match_table` for probing on older kernels. Modern kernels can match through firmware tables, but many drivers still keep ID tables for module aliases, legacy instantiation, or compatibility.
- For drivers that should support DT and ACPI/software nodes, consider `device_property_read_*()`, `fwnode_property_read_*()`, and match-data helpers instead of OF-only APIs.
