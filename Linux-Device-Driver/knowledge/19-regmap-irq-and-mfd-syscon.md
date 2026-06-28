# 19 - Regmap IRQ And MFD/Syscon

## Learning Goal
After this topic, you should be able to explain how Linux handles chips that are "one physical device, many logical devices", how regmap IRQ turns interrupt status bits into Linux IRQs, how MFD creates child platform devices, and how syscon exposes shared MMIO register blocks through regmap.

The practical goal is to look at a PMIC, GPIO expander, RTC/power-key block, or SoC system-controller register region and decide:

- should this be one driver, an MFD parent with child drivers, a syscon consumer, or `simple-mfd`;
- where the parent regmap lives;
- how child IRQs are created and passed to child drivers;
- what can go wrong during probe, interrupt handling, Device Tree matching, and teardown.

## Why This Matters In Real Work
Many embedded chips do not fit neatly into one subsystem. A PMIC may contain regulators, an RTC, a watchdog, a power key, GPIO lines, and interrupt status registers. A SoC "system controller" block may contain unrelated reset, boot-mode, power, or mux bits in one MMIO range.

Linux solves these shapes with a few cooperating pieces:

- **Regmap IRQ** handles register-backed interrupt controllers.
- **MFD** splits one physical chip into child platform devices.
- **Syscon** exposes miscellaneous shared MMIO registers as a regmap.
- **`simple-mfd`** lets simple MMIO parent nodes spawn child devices without a custom parent driver.

You use this topic when:

- one chip has multiple independent functions owned by different subsystems;
- child devices need IRQs produced by bits in parent status registers;
- a parent driver owns shared register access and child creation;
- several drivers need access to a miscellaneous MMIO register block;
- a Device Tree node contains subnodes that should become real platform devices.

Keep this separate from topic 18: topic 18 is core regmap access and cache. This topic is about **turning shared register blocks into Linux devices and IRQs**.

## Mental Model
Think of a PMIC as an apartment building.

- The physical chip is the building.
- The MFD parent driver is the building manager.
- The regmap is the shared key system for all register doors.
- Regmap IRQ is the front-desk alarm panel that says which apartment called.
- Child drivers are tenants: regulator, RTC, GPIO, power key, watchdog.
- Syscon is a shared utility closet: not a single tenant, but several drivers may need specific switches inside it.

In kernel terms:

```text
physical chip or MMIO block
  -> parent driver creates regmap
  -> parent registers regmap IRQ chip, if the chip has interrupt status bits
  -> parent registers MFD child platform devices
  -> child drivers bind normally and request resources
```

For a syscon:

```text
DT node with compatible "...", "syscon"
  -> syscon framework maps MMIO region as a regmap
  -> consumers find it by phandle/node/compatible
  -> consumers update their owned bits through regmap
```

**Interview trap:** MFD "subdevice" means a child platform device created by the MFD core. It is not the same thing as a V4L2 subdevice.

## Core Concepts
These concepts are separate but often appear together in one driver.

| Concept | What It Does | Typical Use |
| --- | --- | --- |
| Regmap | Common register access layer | parent chip register access |
| Regmap IRQ | Generic IRQ controller built from register bits | PMIC IRQ status/mask/ack registers |
| MFD | Creates child platform devices from one parent chip | PMIC regulator/RTC/GPIO/watchdog children |
| Syscon | Shared MMIO register block exposed as regmap | SoC general-purpose registers |
| `simple-mfd` | DT-only child population for simple MMIO parents | syscon-like block with simple child nodes |
| IRQ domain | Maps child hardware IRQ indexes to Linux virqs | child IRQ resources |

### Regmap IRQ vs Manual IRQ Domain vs GPIOLIB IRQ
Choose based on the hardware shape.

| Hardware Shape | Better Tool |
| --- | --- |
| GPIO controller with MMIO IRQ registers and fast hard-IRQ-safe access | gpiolib irqchip or manual chained IRQ |
| GPIO expander over I2C/SPI where register access sleeps | nested/threaded IRQ, often regmap IRQ |
| PMIC/MFD with status/mask/ack/type registers | regmap IRQ |
| Very unusual interrupt controller with special flow | custom IRQ domain/irqchip |
| Plain device with one interrupt line, not an IRQ controller | `devm_request_threaded_irq()` |

**Production rule:** I2C/SPI register access can sleep, so a bus-backed interrupt controller must not do register reads from hard IRQ context.

### MFD vs Platform Driver vs `simple-mfd`
Not every multi-register block should become an MFD.

| Design | Use When | Avoid When |
| --- | --- | --- |
| One normal driver | one cohesive function owns the whole block | unrelated subsystems need child drivers |
| MFD parent driver | parent must probe chip, create children, share regmap/IRQs | children are simple DT subnodes needing no parent logic |
| `simple-mfd` | MMIO parent only needs child population | child existence depends on runtime detection or revision |
| Syscon | miscellaneous shared MMIO registers | a real subsystem/provider API would be cleaner |

**Design smell:** syscon is powerful, but if many drivers freely poke the same register block without ownership rules, review and debugging become painful.

## Kernel Mechanism
Regmap IRQ, MFD, and syscon sit on top of standard kernel objects: `struct device`, platform devices, IRQ domains, and regmap.

### Regmap IRQ Internals
The driver describes the interrupt controller in data. The regmap IRQ core does the repetitive work.

```text
struct regmap_irq[]
  -> one entry per child interrupt bit/index

struct regmap_irq_chip
  -> status/mask/ack/wake/type register layout

devm_regmap_add_irq_chip()
  -> allocates runtime regmap_irq_chip_data
  -> creates IRQ domain
  -> requests threaded parent IRQ
  -> maps child hwirqs to virqs
```

When the parent IRQ fires:

```text
hardware asserts parent IRQ
  -> regmap_irq_thread() runs in threaded context
  -> reads status registers through regmap
  -> applies mask/ack/type rules
  -> finds active child interrupt bits
  -> maps child hwirq index to virq
  -> calls nested child IRQ handling
  -> child driver's handler runs
```

This is why regmap IRQ is a good fit for slow bus chips. The parent handler is threaded, so register reads over I2C/SPI are allowed.

### MFD Internals
The MFD parent creates child platform devices from `struct mfd_cell` entries.

```text
parent probe
  -> creates parent regmap
  -> optionally registers regmap IRQ chip
  -> fills mfd_cell array
  -> calls devm_mfd_add_devices()
  -> child platform devices appear
  -> child subsystem drivers probe
```

Each child can receive:

- a name;
- an optional Device Tree compatible string;
- platform data;
- `IORESOURCE_MEM` resources;
- `IORESOURCE_IRQ` resources;
- parent linkage through the device model.

If the parent passes a regmap IRQ domain into `devm_mfd_add_devices()`, child IRQ resources can be translated into Linux virqs.

### Syscon Internals
Syscon is built on regmap for MMIO register blocks.

```text
DT syscon node
  -> has compatible with "syscon" fallback
  -> has reg MMIO range
  -> syscon maps the range
  -> syscon stores/reuses a regmap for that node
  -> consumers obtain the regmap by phandle/node/compatible
```

Common lookup helpers:

- `syscon_node_to_regmap(np)`
- `syscon_regmap_lookup_by_compatible("vendor,soc-gpr")`
- `syscon_regmap_lookup_by_phandle(dev->of_node, "vendor,syscon")`
- `syscon_regmap_lookup_by_phandle_args(...)`
- `syscon_regmap_lookup_by_phandle_optional(...)`

Prefer phandle lookup in normal drivers because it makes the dependency explicit in firmware description.

## Key Structs And APIs
The important APIs are easier to remember by role.

### Regmap IRQ
| API / Struct | Role |
| --- | --- |
| `struct regmap_irq` | describes one child interrupt bit/index |
| `struct regmap_irq_type` | current-kernel trigger-type encoding for one IRQ |
| `struct regmap_irq_chip` | describes status/mask/ack/wake/type register layout |
| `struct regmap_irq_chip_data` | runtime state allocated by regmap IRQ core |
| `devm_regmap_add_irq_chip()` | managed registration of regmap-backed IRQ controller |
| `devm_regmap_add_irq_chip_fwnode()` | fwnode-specific variant for IRQ domain placement |
| `regmap_irq_get_virq(data, irq_index)` | get/create Linux virq for one regmap IRQ index |
| `regmap_irq_get_domain(data)` | return IRQ domain for MFD child resource translation |
| `regmap_irq_chip_get_base(data)` | legacy/base IRQ helper where relevant |
| `regmap_del_irq_chip()` | unmanaged cleanup path |

### MFD
| API / Struct | Role |
| --- | --- |
| `struct mfd_cell` | describes one MFD child |
| `mfd_get_cell(pdev)` | child driver can retrieve the cell that created it |
| `devm_mfd_add_devices()` | managed child platform-device creation |
| `mfd_add_devices()` | unmanaged child creation |
| `mfd_remove_devices()` | remove children for unmanaged flows |
| `IORESOURCE_IRQ` | child IRQ resource |
| `IORESOURCE_MEM` | child memory resource |
| `PLATFORM_DEVID_AUTO` | allocate child IDs automatically |
| `PLATFORM_DEVID_NONE` | no instance ID |

### Child Platform Drivers
| API | Role |
| --- | --- |
| `platform_get_irq()` | get unnamed child IRQ resource |
| `platform_get_irq_byname()` | get named child IRQ resource |
| `platform_get_resource()` | get memory or other resource |
| `dev_get_drvdata(pdev->dev.parent)` | get parent private data when appropriate |
| `dev_get_platdata()` | get platform data passed by parent |

### Syscon
| API | Role |
| --- | --- |
| `syscon_node_to_regmap()` | map from explicit node pointer |
| `syscon_regmap_lookup_by_compatible()` | find by compatible string |
| `syscon_regmap_lookup_by_phandle()` | find by phandle property |
| `syscon_regmap_lookup_by_phandle_args()` | find by phandle with arguments |
| `syscon_regmap_lookup_by_phandle_optional()` | optional dependency variant |

## Lifecycle / Data Flow
A PMIC-like MFD with child IRQs follows a predictable flow.

```text
1. Parent device matches
   I2C/SPI/platform probe starts.

2. Parent resources are prepared
   Allocate private data, enable power/clock/reset if needed, create regmap.

3. Parent interrupt controller is described
   Define regmap_irq[] and regmap_irq_chip.

4. Parent IRQ is registered
   devm_regmap_add_irq_chip(parent_dev, map, parent_irq, flags, ...)
   returns regmap_irq_chip_data.

5. Child devices are described
   Define mfd_cell[] with names, compatibles, and resources.

6. Child devices are created
   devm_mfd_add_devices(..., cells, ..., regmap_irq_get_domain(irq_data)).

7. Child drivers bind
   Regulator, RTC, input, GPIO, watchdog, or other subsystem drivers probe.

8. Runtime interrupt happens
   Parent IRQ -> regmap IRQ thread -> child virq -> child handler.

9. Remove/unbind happens
   Stop child-visible activity first, remove children, then release parent IRQ/regmap resources.
```

For syscon:

```text
1. DT describes shared MMIO node with specific compatible plus "syscon".
2. Consumer has a phandle property pointing to the syscon.
3. Consumer probe calls syscon_regmap_lookup_by_phandle().
4. Consumer updates only its owned bits with regmap_update_bits().
5. Ownership and locking rules must be documented if bits are shared.
```

## Minimal Practical Example
This is **learning-only pseudo-code**, not a production-ready driver. It shows the object relationships without pretending every current kernel field and binding detail is complete.

```c
struct demo_pmic {
    struct device *dev;
    struct regmap *map;
    struct regmap_irq_chip_data *irq_data;
};

enum {
    DEMO_IRQ_PWRKEY,
    DEMO_IRQ_RTC,
};

static const struct regmap_irq demo_irqs[] = {
    [DEMO_IRQ_PWRKEY] = { .reg_offset = 0, .mask = BIT(0) },
    [DEMO_IRQ_RTC]    = { .reg_offset = 0, .mask = BIT(1) },
};

static const struct regmap_irq_chip demo_irq_chip = {
    .name = "demo-pmic-irq",
    .status_base = DEMO_REG_IRQ_STATUS,
    .mask_base = DEMO_REG_IRQ_MASK,
    .ack_base = DEMO_REG_IRQ_STATUS,
    .num_regs = 1,
    .irqs = demo_irqs,
    .num_irqs = ARRAY_SIZE(demo_irqs),
};

static const struct resource pwrkey_resources[] = {
    DEFINE_RES_IRQ(DEMO_IRQ_PWRKEY),
};

static const struct mfd_cell demo_cells[] = {
    {
        .name = "demo-pmic-pwrkey",
        .of_compatible = "demo,pmic-pwrkey",
        .resources = pwrkey_resources,
        .num_resources = ARRAY_SIZE(pwrkey_resources),
    },
    {
        .name = "demo-pmic-rtc",
        .of_compatible = "demo,pmic-rtc",
    },
};

static int demo_pmic_probe(struct i2c_client *client)
{
    struct demo_pmic *pmic;
    int ret;

    pmic = devm_kzalloc(&client->dev, sizeof(*pmic), GFP_KERNEL);
    if (!pmic)
        return -ENOMEM;

    pmic->dev = &client->dev;
    pmic->map = devm_regmap_init_i2c(client, &demo_regmap_config);
    if (IS_ERR(pmic->map))
        return PTR_ERR(pmic->map);

    ret = devm_regmap_add_irq_chip(&client->dev, pmic->map,
                                   client->irq,
                                   IRQF_ONESHOT, 0,
                                   &demo_irq_chip,
                                   &pmic->irq_data);
    if (ret)
        return dev_err_probe(&client->dev, ret, "IRQ chip failed\n");

    i2c_set_clientdata(client, pmic);

    return devm_mfd_add_devices(&client->dev, PLATFORM_DEVID_NONE,
                                demo_cells, ARRAY_SIZE(demo_cells),
                                NULL, 0,
                                regmap_irq_get_domain(pmic->irq_data));
}
```

The important lines are:

- `devm_regmap_init_i2c()` creates the parent register access object.
- `demo_irqs[]` maps child interrupt indexes to register bits.
- `demo_irq_chip` describes status/mask/ack registers.
- `devm_regmap_add_irq_chip()` creates the child IRQ domain and parent threaded IRQ handler.
- `regmap_irq_get_domain()` passes that IRQ domain into MFD child creation.
- `devm_mfd_add_devices()` creates child platform devices.

A child power-key driver would later call:

```c
irq = platform_get_irq(pdev, 0);
ret = devm_request_threaded_irq(&pdev->dev, irq, NULL,
                                demo_pwrkey_irq,
                                IRQF_ONESHOT,
                                dev_name(&pdev->dev), priv);
```

### Syscon Consumer Pseudo-code
This is also learning-only.

```c
struct regmap *gpr;

gpr = syscon_regmap_lookup_by_phandle(dev->of_node, "vendor,gpr");
if (IS_ERR(gpr))
    return dev_err_probe(dev, PTR_ERR(gpr), "missing gpr syscon\n");

return regmap_update_bits(gpr, GPR_CTRL,
                          GPR_CTRL_USB_MODE,
                          FIELD_PREP(GPR_CTRL_USB_MODE, USB_MODE_HOST));
```

Use this pattern only when the binding clearly says the consumer owns those bits.

## Common Bugs And Debugging
Start from symptoms. The failing layer usually tells you where to look.

### Child Driver Gets No IRQ
Likely causes:

- parent never called `devm_regmap_add_irq_chip()`;
- `devm_regmap_add_irq_chip()` failed but the error was ignored;
- parent passed `NULL` instead of `regmap_irq_get_domain()` to `devm_mfd_add_devices()`;
- child `struct mfd_cell` lacks `IORESOURCE_IRQ`;
- child DT compatible did not match the MFD cell and child driver;
- wrong child IRQ index in `DEFINE_RES_IRQ()`.

Evidence to inspect:

- parent probe logs;
- child probe return from `platform_get_irq()` or `platform_get_irq_byname()`;
- `/proc/interrupts` for parent and child IRQ activity;
- sysfs device tree under the parent device;
- dynamic debug in parent MFD and child drivers.

### Interrupt Storm After Probe
Likely causes:

- stale status bits were not acknowledged before enabling IRQs;
- wrong `ack_base`, `status_base`, `mask_base`, stride, or inverted mask flag;
- status register is clear-on-read but policy/config does not match hardware;
- level interrupt remains asserted because the child condition was not cleared;
- missing `IRQF_ONESHOT` or wrong threaded/nested flow.

Fix pattern:

- verify datasheet status/mask/ack semantics;
- clear stale status before enabling child interrupts;
- confirm regmap IRQ chip fields against current headers;
- trace register reads/writes if safe;
- confirm child handler clears the source condition.

### Syscon Lookup Fails
Likely causes:

- missing phandle property;
- phandle points to the wrong node;
- syscon node lacks proper compatible string with `"syscon"` fallback;
- missing `reg` property;
- `CONFIG_MFD_SYSCON` is not enabled;
- driver used compatible lookup where explicit phandle lookup was expected.

Fix pattern:

- inspect the live DT in `/proc/device-tree`;
- check binding docs/YAML;
- use `dev_err_probe()` so probe deferral and error codes are visible;
- prefer explicit phandles over global compatible searches.

### Child Devices Never Probe
Likely causes:

- `devm_mfd_add_devices()` failed;
- child `of_compatible` does not match child node or child driver's OF table;
- `PLATFORM_DEVID_*` choice causes unexpected naming/instance behavior;
- parent node structure does not match binding expectations;
- child driver module is not loaded or lacks `MODULE_DEVICE_TABLE()`.

Evidence to inspect:

- `/sys/bus/platform/devices`;
- `dmesg` for MFD add failure;
- module aliases with `modinfo`;
- child node `compatible` strings;
- parent-child relationship in sysfs.

## Production Checklist
Before review or board bring-up, check the design, not just the code.

**Parent/MFD design:**

- One physical chip really contains multiple subsystem-owned functions.
- Parent owns shared register access, chip-wide IRQ controller, and child creation.
- Child drivers do not duplicate parent register setup.
- Child names and compatible strings match bindings and driver tables.
- Parent teardown stops children before shared resources disappear.

**Regmap IRQ:**

- Parent IRQ is valid and requested by `devm_regmap_add_irq_chip()`.
- `struct regmap_irq[]` indexes match child resources.
- `status_base`, `mask_base`, `ack_base`, `wake_base`, `type_base`, strides, and invert flags match the datasheet.
- Stale status bits are handled before enabling interrupts.
- I2C/SPI accesses happen only in sleepable context.
- Wake/suspend behavior is validated if child IRQs can wake the system.

**MFD resources:**

- `struct mfd_cell` resources are named where names improve clarity.
- IRQ resources use the same numbering expected by `regmap_irq_get_virq()`.
- `devm_mfd_add_devices()` receives the right IRQ domain.
- Child drivers use normal platform resource APIs.
- Multi-address I2C children use current kernel helper APIs and cleanup rules.

**Syscon/simple-mfd:**

- Syscon is justified as shared miscellaneous registers, not as a shortcut around a real subsystem API.
- DT uses a specific compatible plus `"syscon"` where required.
- Consumers use phandles for explicit dependencies.
- Shared-bit ownership is documented.
- `simple-mfd` is used only when children need no runtime parent-driver discovery/setup.

**Debug and maintainability:**

- Use `dev_err_probe()` for probe failures.
- Add useful parent and child probe logs without flooding.
- Validate `/proc/interrupts` and sysfs child creation during bring-up.
- Use regmap debugfs/tracing only when registers are safe to inspect.
- Keep old book-era struct snippets out of production code; compile against target headers.

## Interview Readiness
You should be able to explain the whole chain without memorizing every struct field.

Be ready to answer:

- Why does regmap IRQ exist?
- Why does it use threaded/nested interrupt handling?
- How does a parent IRQ become child virqs?
- What does `regmap_irq_get_domain()` do for MFD children?
- What is the difference between MFD, syscon, and `simple-mfd`?
- How does a child platform driver receive resources from an MFD parent?
- What causes child IRQ mapping failures?
- Why can syscon be a design smell?

See `interview/19-regmap-irq-and-mfd-syscon.md` for practice questions and debugging scenarios.

## Kernel Version Notes
Regmap IRQ and MFD/syscon APIs are version-sensitive enough that examples should be checked against the target kernel headers.

- Current kernels use `struct regmap_irq_type` inside `struct regmap_irq`; older material may show flat trigger-type fields.
- `struct regmap_irq_chip` has grown many fields over time. Do not paste old full struct definitions into docs or code.
- Current kernels provide fwnode variants such as `devm_regmap_add_irq_chip_fwnode()`.
- `struct mfd_cell` has fields not shown in older books, including newer firmware-node and OF matching details.
- Syscon bindings are described by YAML schemas in current kernels; older text files may still explain `simple-mfd`.
- Multi-address I2C MFD examples using old dummy-client helpers should be updated against the target kernel before production use.
