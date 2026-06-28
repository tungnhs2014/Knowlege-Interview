# 19 - Regmap IRQ And MFD/Syscon Interview Questions

Strong candidates can reason from the hardware shape to the Linux design: one physical chip, shared register access, child devices, child IRQs, syscon consumers, and teardown ordering. Good answers explain mechanisms and failure modes, not just "MFD splits devices" or "regmap abstracts registers."

## Beginner Questions

### 1. What problem do Regmap IRQ, MFD, and syscon solve?
**Short Answer:** They handle chips and register blocks that do not fit one simple driver: regmap IRQ turns register interrupt bits into Linux IRQs, MFD splits one physical chip into child devices, and syscon shares miscellaneous MMIO registers through regmap.

**Deep Explanation:** Many embedded devices are multi-function. A PMIC can contain regulators, RTC, GPIO, power-key, watchdog, and interrupt status registers. The parent driver owns the common register access and interrupt controller. MFD creates child platform devices for the separate functions. Regmap IRQ converts status/mask/ack register bits into child virqs. Syscon handles shared SoC control registers that are not one cohesive device.

**API / Code Anchor:**
```c
ret = devm_regmap_add_irq_chip(dev, map, irq, IRQF_ONESHOT,
                               0, &pmic_irq_chip, &irq_data);

ret = devm_mfd_add_devices(dev, PLATFORM_DEVID_NONE,
                           cells, ARRAY_SIZE(cells),
                           NULL, 0, regmap_irq_get_domain(irq_data));
```

**Production or Debugging Angle:** If the design is wrong, child drivers may duplicate parent logic, child IRQs may not resolve, or unrelated drivers may race on shared syscon bits.

**Common Traps:** Saying MFD is just "many drivers in one file." The key idea is parent-owned shared services plus child platform devices.

**Follow-up Questions:**
- What does the parent driver usually own?
- What does a child driver usually own?
- When would syscon be a better fit than a full MFD parent?

### 2. What is Regmap IRQ?
**Short Answer:** Regmap IRQ is a helper layer that implements an IRQ controller whose interrupt state is stored in device registers accessed through regmap.

**Deep Explanation:** Many PMICs and expanders expose interrupt status, mask, ack, wake, and type registers. Instead of writing a custom IRQ domain and threaded handler for every device, the driver describes each interrupt bit with `struct regmap_irq` and the register layout with `struct regmap_irq_chip`. The regmap IRQ core creates an IRQ domain, requests the parent IRQ, reads status through regmap, and dispatches child virqs.

**API / Code Anchor:**
```c
static const struct regmap_irq chip_irqs[] = {
    [IRQ_PWRKEY] = { .reg_offset = 0, .mask = BIT(0) },
};

static const struct regmap_irq_chip irq_chip = {
    .name = "chip-irq",
    .status_base = REG_IRQ_STATUS,
    .mask_base = REG_IRQ_MASK,
    .ack_base = REG_IRQ_STATUS,
    .irqs = chip_irqs,
    .num_irqs = ARRAY_SIZE(chip_irqs),
    .num_regs = 1,
};
```

**Production or Debugging Angle:** It is a strong fit when the hardware interrupt model is register-bit based and register access may sleep.

**Common Traps:** Treating it as only a GPIO helper. It is common in PMIC/MFD-style devices, not only GPIO expanders.

**Follow-up Questions:**
- What are status, mask, and ack registers?
- Why does regmap IRQ need a parent IRQ?
- What is a child virq?

### 3. What is an MFD driver?
**Short Answer:** An MFD driver is a parent/core driver for one physical multi-function device that creates child platform devices for its functional blocks.

**Deep Explanation:** The parent handles common chip services: register access, chip identification, shared interrupts, and child creation. Each child then binds to a normal subsystem driver, such as regulator, RTC, input, GPIO, watchdog, or NVMEM. The MFD core uses `struct mfd_cell` entries to create the child platform devices and pass resources.

**API / Code Anchor:**
```c
static const struct mfd_cell cells[] = {
    { .name = "demo-rtc", .of_compatible = "demo,rtc" },
    { .name = "demo-pwrkey", .of_compatible = "demo,pwrkey",
      .resources = pwrkey_resources,
      .num_resources = ARRAY_SIZE(pwrkey_resources) },
};

ret = devm_mfd_add_devices(dev, PLATFORM_DEVID_NONE,
                           cells, ARRAY_SIZE(cells),
                           NULL, 0, irq_domain);
```

**Production or Debugging Angle:** MFD keeps subsystem code separate while preserving one parent owner for shared chip state.

**Common Traps:** Confusing MFD child platform devices with V4L2 subdevices or with independent I2C/SPI devices.

**Follow-up Questions:**
- What is `struct mfd_cell`?
- Why do MFD children usually appear as platform devices?
- How does a child driver get its IRQ?

### 4. What is syscon?
**Short Answer:** Syscon is a framework for exposing miscellaneous shared MMIO register blocks as regmaps.

**Deep Explanation:** Some SoC register regions contain unrelated control bits that do not form one cohesive subsystem device. A syscon node describes that MMIO region, and consumers obtain a `struct regmap *` by phandle, node, or compatible lookup. They then use regmap APIs to touch only their owned bits.

**API / Code Anchor:**
```c
map = syscon_regmap_lookup_by_phandle(dev->of_node, "vendor,gpr");
if (IS_ERR(map))
    return PTR_ERR(map);

ret = regmap_update_bits(map, REG_CTRL, MODE_MASK, MODE_HOST);
```

**Production or Debugging Angle:** Syscon is useful, but it needs clear binding and ownership rules because multiple drivers may touch one register block.

**Common Traps:** Using syscon as a shortcut for any random register access. If the block has a real cohesive function, a normal subsystem/provider driver may be better.

**Follow-up Questions:**
- Why is syscon backed by regmap?
- Why is phandle lookup often preferable?
- What can go wrong if two drivers update the same register bits?

## Mid-Level Questions

### 5. Walk through the probe flow of a PMIC MFD parent with child IRQs.
**Short Answer:** The parent creates its regmap, registers a regmap IRQ chip using the parent IRQ, stores the returned IRQ data, and registers MFD child devices using the regmap IRQ domain.

**Deep Explanation:** Probe starts at the physical device, often I2C/SPI/MMIO. The parent allocates private data and initializes register access. If the chip exposes child interrupt bits, the parent registers a regmap IRQ chip. That creates runtime IRQ data and an IRQ domain. The parent then calls `devm_mfd_add_devices()` with child cells and passes `regmap_irq_get_domain(irq_data)` so child IRQ resources can be mapped.

**API / Code Anchor:**
```c
pmic->map = devm_regmap_init_i2c(client, &cfg);
if (IS_ERR(pmic->map))
    return PTR_ERR(pmic->map);

ret = devm_regmap_add_irq_chip(&client->dev, pmic->map,
                               client->irq, IRQF_ONESHOT, 0,
                               &irq_chip, &pmic->irq_data);
if (ret)
    return ret;

return devm_mfd_add_devices(&client->dev, PLATFORM_DEVID_NONE,
                            cells, ARRAY_SIZE(cells),
                            NULL, 0,
                            regmap_irq_get_domain(pmic->irq_data));
```

**Production or Debugging Angle:** The child device should not probe before the parent has registered the shared IRQ domain and common services.

**Common Traps:** Registering MFD children before setting up the IRQ domain, or ignoring the return from `devm_regmap_add_irq_chip()`.

**Follow-up Questions:**
- Where should the parent store `struct regmap_irq_chip_data *`?
- What should happen if parent IRQ registration fails?
- Why is `dev_err_probe()` useful here?

### 6. How does a child IRQ resource become a Linux IRQ?
**Short Answer:** The MFD cell describes an `IORESOURCE_IRQ` using a local child IRQ index, and MFD maps it through the parent IRQ domain, often the domain returned by `regmap_irq_get_domain()`.

**Deep Explanation:** The child resource number is not necessarily a global Linux IRQ number. It is usually a hardware/local interrupt index understood by the parent IRQ domain. When the parent passes an IRQ domain into `devm_mfd_add_devices()`, the MFD core creates a mapped Linux virq for each child IRQ resource. The child driver later calls `platform_get_irq()` or `platform_get_irq_byname()` and receives that virq.

**API / Code Anchor:**
```c
static const struct resource pwrkey_res[] = {
    DEFINE_RES_IRQ(DEMO_IRQ_PWRKEY),
};

ret = devm_mfd_add_devices(dev, PLATFORM_DEVID_NONE,
                           cells, ARRAY_SIZE(cells),
                           NULL, 0,
                           regmap_irq_get_domain(irq_data));

irq = platform_get_irq(pdev, 0);
```

**Production or Debugging Angle:** If the child gets `-ENXIO` or no IRQ, inspect the cell resource and whether the parent passed the IRQ domain.

**Common Traps:** Treating `DEFINE_RES_IRQ(DEMO_IRQ_PWRKEY)` as already being the final Linux IRQ number.

**Follow-up Questions:**
- What is the difference between hwirq and virq?
- What does `regmap_irq_get_virq()` return?
- When would a child use `platform_get_irq_byname()`?

### 7. Why does regmap IRQ use threaded/nested interrupt handling?
**Short Answer:** Because many regmap IRQ users are I2C/SPI devices where reading status registers can sleep, so handling must happen in thread context and dispatch child interrupts as nested IRQs.

**Deep Explanation:** Hard IRQ context cannot sleep. A PMIC or GPIO expander over I2C/SPI needs bus transfers to read status, ack events, and update masks. Regmap IRQ requests a threaded parent IRQ and uses nested child handling. Current regmap IRQ code forces `IRQF_ONESHOT` when requesting the parent thread, which keeps the parent line controlled while the thread runs.

**API / Code Anchor:**
```c
ret = devm_regmap_add_irq_chip(dev, map, parent_irq,
                               IRQF_ONESHOT, 0,
                               &chip, &data);
```

**Production or Debugging Angle:** If a driver reads I2C/SPI registers in a hard IRQ handler, it can trigger sleep-in-atomic warnings or lockups.

**Common Traps:** Copying a chained IRQ-controller pattern from an MMIO GPIO controller into an I2C expander.

**Follow-up Questions:**
- What does `IRQF_ONESHOT` protect against?
- When is chained IRQ handling appropriate?
- Why is MMIO different from I2C/SPI here?

### 8. When should you use `simple-mfd`?
**Short Answer:** Use `simple-mfd` when an MMIO parent node only needs its child DT nodes populated as devices and does not need custom parent-driver probing or runtime child discovery.

**Deep Explanation:** `simple-mfd` is a Device Tree binding pattern. It is useful for simple MMIO register blocks where the children are statically described and no parent driver needs to inspect hardware, configure shared state, or decide which children exist. It often appears with `"syscon"` so children can reference the shared regmap.

**API / Code Anchor:**
```dts
snvs: snvs@20cc000 {
    compatible = "fsl,sec-v4.0-mon", "syscon", "simple-mfd";
    reg = <0x020cc000 0x4000>;

    pwrkey {
        compatible = "fsl,sec-v4.0-pwrkey";
        regmap = <&snvs>;
        interrupts = <4>;
    };
};
```

**Production or Debugging Angle:** If child creation depends on chip revision, status registers, or parent setup, write a real MFD parent driver.

**Common Traps:** Using `simple-mfd` for complex PMICs that need parent interrupt setup, regulator constraints, or chip-specific initialization.

**Follow-up Questions:**
- How is `simple-mfd` related to `simple-bus`?
- Why does it often appear with `syscon`?
- What would force you to write a real MFD driver?

### 9. How would you debug a child platform driver that never probes?
**Short Answer:** Check whether the parent called `devm_mfd_add_devices()`, whether child compatible/name matching is correct, whether the child module has aliases, and whether the child device exists under the platform bus.

**Deep Explanation:** MFD children are platform devices. They must be created by the parent and matched against a platform driver. Matching can use the child node compatible string, the MFD cell `.of_compatible`, or platform names depending on the setup. If any of those are wrong, the child exists but no driver binds, or the child is not created at all.

**API / Code Anchor:**
```c
static const struct mfd_cell cells[] = {
    { .name = "demo-pwrkey", .of_compatible = "demo,pmic-pwrkey" },
};

static const struct of_device_id pwrkey_of_match[] = {
    { .compatible = "demo,pmic-pwrkey" },
    { }
};
MODULE_DEVICE_TABLE(of, pwrkey_of_match);
```

**Production or Debugging Angle:** Inspect `/sys/bus/platform/devices`, `dmesg`, and `modinfo` aliases. Confirm the parent node and child nodes match the binding.

**Common Traps:** Fixing the child driver while the real bug is that the parent never created the child device.

**Follow-up Questions:**
- What does `MODULE_DEVICE_TABLE(of, ...)` affect?
- Why can name-only matching be ambiguous?
- How do you inspect platform devices from userspace?

## Senior Questions

### 10. Design a PMIC parent driver with regulators, RTC, GPIO, and power-key children.
**Short Answer:** Put shared chip access, chip ID, regmap, regmap IRQ, and MFD child creation in the parent. Put subsystem-specific behavior in child drivers. Pass child IRQs through the regmap IRQ domain and keep lifetime ordering explicit.

**Deep Explanation:** The parent should bind to the physical bus device and create a single authoritative regmap. It should verify the chip, clear stale interrupt status if required, register a regmap IRQ chip for shared interrupt registers, and create child devices with `devm_mfd_add_devices()`. Regulators, RTC, GPIO, and power-key should be separate subsystem drivers. Child drivers should use platform resources and, when needed, parent-provided data or shared regmap access under clearly defined ownership.

**API / Code Anchor:**
```c
struct pmic {
    struct regmap *map;
    struct regmap_irq_chip_data *irq_data;
};

devm_regmap_add_irq_chip(dev, map, parent_irq, IRQF_ONESHOT,
                         0, &pmic_irq_chip, &pmic->irq_data);

devm_mfd_add_devices(dev, PLATFORM_DEVID_NONE,
                     pmic_cells, ARRAY_SIZE(pmic_cells),
                     NULL, 0, regmap_irq_get_domain(pmic->irq_data));
```

**Production or Debugging Angle:** The design should survive probe deferral, partial child registration, suspend/resume, wakeup IRQs, and parent removal.

**Common Traps:** Letting every child create its own independent bus access, or putting regulator/input/RTC logic all inside the parent driver.

**Follow-up Questions:**
- Which child should own wakeup policy for a power key?
- How would you handle chip variants?
- How do you prevent children from accessing parent resources after teardown?

### 11. A board has an interrupt storm immediately after enabling a regmap IRQ chip. What do you inspect?
**Short Answer:** Inspect status/ack semantics, stale pending bits, mask polarity, register base/stride fields, and whether the child handler clears the real source condition.

**Deep Explanation:** Regmap IRQ only behaves correctly if the chip description matches hardware. If ack bits are wrong, the same status remains pending. If mask polarity is inverted, enabling may actually unmask everything. If a level interrupt source remains active, the parent IRQ reasserts immediately. If stale status was not cleared before registration, the first enable can flood the system.

**API / Code Anchor:**
```c
static const struct regmap_irq_chip irq_chip = {
    .status_base = REG_STATUS,
    .mask_base = REG_MASK,
    .ack_base = REG_STATUS,
    .mask_invert = true, /* only if datasheet says so */
    .num_regs = 1,
};
```

**Production or Debugging Angle:** Use safe register tracing/debugfs only after marking side-effect registers correctly. Check `/proc/interrupts` to see parent and child counts.

**Common Traps:** Blaming the child driver before proving the parent status bit is acknowledged or the level source is deasserted.

**Follow-up Questions:**
- What is write-one-to-clear?
- Why can level IRQs re-fire?
- How can debug register reads make this worse?

### 12. What are the lifetime risks in MFD parent/child teardown?
**Short Answer:** Children may still use parent regmap, IRQ data, or dummy clients while the parent is being removed, so child-visible activity must stop before shared resources disappear.

**Deep Explanation:** MFD children are separate devices, but they depend on parent-owned services. If a work item, IRQ handler, sysfs callback, or subsystem operation in a child runs after the parent regmap or secondary I2C client is gone, the result can be use-after-free or bus errors. Devm helps release resources, but it does not replace logical ordering. Remove children, stop IRQ/work paths, and unregister manually-created secondary clients in the right order.

**API / Code Anchor:**
```c
/* unmanaged shape */
mfd_remove_devices(parent_dev);
i2c_unregister_device(chip->subdev1_client);
i2c_unregister_device(chip->subdev2_client);
```

**Production or Debugging Angle:** Audit async paths: IRQs, delayed work, threaded handlers, sysfs operations, runtime PM callbacks, and subsystem unregister callbacks.

**Common Traps:** Assuming devm automatically cancels all child activity in a semantically safe order.

**Follow-up Questions:**
- What should be unregistered first, parent resources or child devices?
- Why are workqueues relevant here?
- How would runtime PM complicate teardown?

### 13. When is syscon a design smell?
**Short Answer:** Syscon is suspicious when it is used to bypass a proper subsystem/provider API, when ownership of shared bits is unclear, or when many drivers update the same registers independently.

**Deep Explanation:** Syscon is intended for miscellaneous system-control registers that are not cohesive enough to be one functional device. It is not a free-for-all global MMIO accessor. A good syscon user has a binding that names the relationship, owns specific bits, and uses masked updates. If the register block has clear functionality, a real driver or provider framework is usually cleaner.

**API / Code Anchor:**
```c
map = syscon_regmap_lookup_by_phandle(np, "vendor,gpr");
regmap_update_bits(map, REG_MISC, OWNED_BIT_MASK, new_bits);
```

**Production or Debugging Angle:** Reviewers will ask who owns each bit, how concurrent updates are serialized, and whether a better framework exists.

**Common Traps:** Looking up syscon by compatible string globally when a phandle dependency should be explicit.

**Follow-up Questions:**
- Why is `regmap_update_bits()` safer than read/write pairs here?
- What should the binding document?
- How do you avoid two drivers fighting over one register?

### 14. How do current-kernel API changes affect answers about regmap IRQ?
**Short Answer:** The concept is stable, but struct fields and helper variants have changed, so code examples must be checked against the target kernel headers.

**Deep Explanation:** Older material may show `struct regmap_irq` with flat trigger-type fields such as `type_reg_offset`, `type_rising_mask`, and `type_falling_mask`. Current headers use `struct regmap_irq_type` nested inside `struct regmap_irq`. `struct regmap_irq_chip` has also gained fields for more complex hardware. Current kernels also provide fwnode variants for IRQ chip registration.

**API / Code Anchor:**
```c
struct regmap_irq {
    unsigned int reg_offset;
    unsigned int mask;
    struct regmap_irq_type type;
};

int devm_regmap_add_irq_chip_fwnode(struct device *dev,
                                    struct fwnode_handle *fwnode,
                                    struct regmap *map, int irq,
                                    int irq_flags, int irq_base,
                                    const struct regmap_irq_chip *chip,
                                    struct regmap_irq_chip_data **data);
```

**Production or Debugging Angle:** Do not paste old book struct definitions into production code. Include the right headers and compile against the exact kernel target.

**Common Traps:** Giving an answer that is correct for an old v4.x code excerpt but wrong for a current kernel.

**Follow-up Questions:**
- What should you validate before writing an example?
- Which header defines regmap IRQ structures?
- Why do docs often teach concepts instead of every field?

### 15. A syscon consumer fails with an error pointer during probe. How do you debug it?
**Short Answer:** Check the phandle property, target node compatible and `reg`, `CONFIG_MFD_SYSCON`, binding names, and whether the driver handles `-EPROBE_DEFER` or optional dependencies correctly.

**Deep Explanation:** Syscon lookup is firmware-description driven. If the consumer expects a property like `vendor,gpr` but the DT lacks it or points to a non-syscon node, lookup fails. If `CONFIG_MFD_SYSCON` is disabled, helpers return unsupported errors. If the dependency is optional, use the optional helper or handle absence explicitly.

**API / Code Anchor:**
```c
map = syscon_regmap_lookup_by_phandle_optional(np, "vendor,gpr");
if (IS_ERR(map))
    return dev_err_probe(dev, PTR_ERR(map), "syscon lookup failed\n");
```

**Production or Debugging Angle:** Inspect `/proc/device-tree`, binding YAML, and boot logs. Do not hardcode a compatible lookup if the binding requires an explicit phandle.

**Common Traps:** Treating every error as fatal when the syscon is optional, or ignoring an error pointer and later crashing on regmap access.

**Follow-up Questions:**
- When should a dependency be optional?
- What does `dev_err_probe()` improve?
- Why might compatible lookup find the wrong block?
