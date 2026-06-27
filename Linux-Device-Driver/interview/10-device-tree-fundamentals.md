# 10 - Device Tree Fundamentals Interview Questions

Strong candidates can read a DTS snippet, explain what hardware it describes, predict how the kernel will use it, and debug bring-up failures without blaming the driver too early.

## Beginner Questions

### 1. What is Device Tree, and why does embedded Linux use it?
**Short Answer:** Device Tree is a hardware description passed to the kernel, usually as a `.dtb`, so the kernel can support non-discoverable board hardware without hardcoding every board in C.

**Deep Explanation:** Many embedded devices do not enumerate themselves like PCI or USB. The kernel needs to know which devices exist, where their registers are, which IRQs they use, and how they connect to clocks, GPIOs, regulators, and buses. Device Tree moves that board-specific description into data. Drivers remain generic; board wiring lives in `.dts` and `.dtsi` files.

**API / Code Anchor:** Root and device nodes use properties such as `compatible`, `reg`, `interrupts`, `clocks`, `gpios`, and `status`. Platform drivers often match with `struct of_device_id` and `MODULE_DEVICE_TABLE(of, ...)`.

**Production or Debugging Angle:** If the wrong DTB boots, the driver may be perfectly fine but never probe or receive wrong resources.

**Common Traps:** Saying "Device Tree configures the driver" too broadly. DT should describe hardware, not arbitrary driver policy.

**Follow-up Questions:**
- How is Device Tree different from PCI enumeration?
- What kind of hardware information belongs in DT?
- What kind of information should stay out of DT?

### 2. What are `.dts`, `.dtsi`, and `.dtb`?
**Short Answer:** `.dts` is board-level source, `.dtsi` is reusable include source, and `.dtb` is the compiled binary blob passed to the kernel.

**Deep Explanation:** A board `.dts` usually includes one or more SoC or module `.dtsi` files. The `.dtsi` describes common hardware blocks, often disabled by default. The board `.dts` enables and wires the devices actually present. `dtc` compiles source into a `.dtb` that the bootloader gives to the kernel.

**API / Code Anchor:**
```bash
make ARCH=arm64 dtbs
dtc -I dtb -O dts -o /tmp/board.dts board.dtb
```

**Production or Debugging Angle:** Editing the source file is not enough; you must ensure the deployed DTB is rebuilt and loaded by the bootloader.

**Common Traps:** Treating `.dtsi` as board-specific or assuming a changed `.dts` automatically changes the booted DTB.

**Follow-up Questions:**
- Why are SoC peripherals often disabled in `.dtsi`?
- How would you confirm the running board uses the DTB you edited?
- Why might a signed/FIT boot flow complicate DTB deployment?

### 3. What is the role of the `compatible` property?
**Short Answer:** `compatible` identifies the hardware and is used by bindings and drivers to decide how a node should be interpreted and matched.

**Deep Explanation:** A node can have multiple compatible strings, usually from most specific to more generic fallback. The driver match table lists compatible strings it supports. Bindings document which properties are valid for that compatible.

**API / Code Anchor:**
```dts
uart0: serial@10000000 {
    compatible = "vendor,uart-v2", "vendor,uart-v1";
};
```

```c
static const struct of_device_id uart_of_match[] = {
    { .compatible = "vendor,uart-v2" },
    { .compatible = "vendor,uart-v1" },
    { }
};
MODULE_DEVICE_TABLE(of, uart_of_match);
```

**Production or Debugging Angle:** If `compatible` does not match the driver or binding, probe may never run, module autoload may fail, or validation may reject the node.

**Common Traps:** Using a vague compatible such as `"vendor,device"` when hardware revision matters, or forgetting fallback-compatible strings.

**Follow-up Questions:**
- Why is order important in a compatible list?
- Why should new compatible strings be documented?
- What happens if `MODULE_DEVICE_TABLE(of, ...)` is missing?

### 4. What are labels and phandles?
**Short Answer:** A label is a source-level name for a node; a phandle is the compiled reference used when one node points to another.

**Deep Explanation:** In DTS, you write `gpio1: gpio@...` to label a node. Another node can reference it with `<&gpio1 ...>`. The compiler resolves that reference into a phandle value. The extra cells after the phandle are interpreted according to the provider's binding.

**API / Code Anchor:**
```dts
gpio1: gpio@10002000 {
    gpio-controller;
    #gpio-cells = <2>;
};

sensor@40 {
    reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
};
```

**Production or Debugging Angle:** A phandle reference is only meaningful if the provider node exists, is enabled when needed, and declares the correct `#*-cells` property.

**Common Traps:** Thinking the label is the runtime device name, or assuming every phandle specifier has the same number of arguments.

**Follow-up Questions:**
- What does `#gpio-cells = <2>` mean?
- How are `clocks = <&clk 15>` and `reset-gpios = <&gpio 7 1>` similar?
- What is an alias, and how is it different from a label?

## Mid-Level Questions

### 5. How do you parse the `reg` property?
**Short Answer:** You parse `reg` according to the parent node's `#address-cells` and `#size-cells`; the meaning also depends on the bus type.

**Deep Explanation:** `reg` is not always `<base size>`. For MMIO devices under a `simple-bus`, it usually describes register base and size. For I2C children, it is the slave address. For SPI children, it is the chip-select index. The parent node defines how many 32-bit cells make up the address and size.

**API / Code Anchor:**
```dts
soc {
    #address-cells = <1>;
    #size-cells = <1>;

    serial@10000000 {
        reg = <0x10000000 0x1000>;
    };
};

&i2c1 {
    #address-cells = <1>;
    #size-cells = <0>;

    eeprom@50 {
        reg = <0x50>;
    };
};
```

For a platform device, driver code may use:
```c
res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
base = devm_ioremap_resource(&pdev->dev, res);
```

**Production or Debugging Angle:** A wrong parent cell count can make every child `reg` look wrong even if each child node seems reasonable.

**Common Traps:** Treating `#address-cells` and `#size-cells` as properties of the node itself instead of its children.

**Follow-up Questions:**
- What does `#size-cells = <0>` imply?
- Why is SPI `reg = <1>` not a memory address?
- What should the unit address in the node name match?

### 6. How are interrupts described in Device Tree?
**Short Answer:** Interrupt controllers declare `interrupt-controller` and `#interrupt-cells`; interrupt consumers specify `interrupt-parent` and `interrupts`, or use `interrupts-extended`.

**Deep Explanation:** The interrupt controller binding defines the meaning of each interrupt cell. For example, ARM GIC-style specifiers often encode interrupt type, interrupt number, and trigger flags. If a device omits `interrupt-parent`, it may inherit it from an ancestor.

**API / Code Anchor:**
```dts
intc: interrupt-controller@10001000 {
    interrupt-controller;
    #interrupt-cells = <3>;
};

uart0: serial@10000000 {
    interrupt-parent = <&intc>;
    interrupts = <0 26 IRQ_TYPE_LEVEL_HIGH>;
};
```

Driver-side platform helpers:
```c
irq = platform_get_irq(pdev, 0);
ret = devm_request_irq(&pdev->dev, irq, handler, 0, dev_name(&pdev->dev), priv);
```

**Production or Debugging Angle:** If IRQs never fire, check the controller binding, trigger flags, inherited `interrupt-parent`, and `/proc/interrupts`.

**Common Traps:** Copying an interrupt specifier from another controller and assuming the cells have the same meaning.

**Follow-up Questions:**
- When would you use `interrupts-extended`?
- What is `#interrupt-cells`?
- Why can a trigger flag bug look like a driver interrupt-handler bug?

### 7. What are named resources and why do they matter?
**Short Answer:** Named resources pair resource lists with name lists, such as `reg-names`, `interrupt-names`, `clock-names`, and `dma-names`, so drivers do not depend only on fragile ordering.

**Deep Explanation:** Many devices have multiple memory ranges, IRQs, clocks, or DMA channels. If the driver assumes index 0 is "rx" and index 1 is "tx", a DTS ordering mistake can cause subtle bugs. Names make the relationship explicit.

**API / Code Anchor:**
```dts
device@80000000 {
    reg = <0x80000000 0x1000>, <0x80001000 0x100>;
    reg-names = "control", "status";

    interrupts = <0 56 IRQ_TYPE_LEVEL_HIGH>, <0 57 IRQ_TYPE_LEVEL_HIGH>;
    interrupt-names = "rx", "tx";

    clocks = <&clk 1>, <&clk 2>;
    clock-names = "core", "bus";
};
```

```c
res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "control");
irq = platform_get_irq_byname(pdev, "rx");
clk = devm_clk_get(&pdev->dev, "core");
```

**Production or Debugging Angle:** Named resources make board review and driver maintenance easier, especially when hardware variants add resources.

**Common Traps:** Reordering `reg` without reordering `reg-names`, or using a driver lookup name that does not exactly match the DTS string.

**Follow-up Questions:**
- What happens if `clock-names` has fewer entries than `clocks`?
- Why are names especially useful for multiple IRQs?
- Can a binding require named resources?

### 8. Your driver does not probe. How do you debug the Device Tree side?
**Short Answer:** Verify the running DTB, node presence, `status`, `compatible`, driver match table, module aliases, and platform device population.

**Deep Explanation:** A missing probe can happen before driver code runs. The node may be disabled, absent from the booted DTB, under the wrong parent, have a typo in `compatible`, or not be populated as a device. The module may also lack an exported OF alias.

**API / Code Anchor:**
```bash
cat /sys/firmware/devicetree/base/model
find /sys/firmware/devicetree/base -name '*my-device*'
ls /sys/bus/platform/devices
ls /sys/bus/platform/drivers
dmesg | grep -iE 'of:|probe|defer|device tree'
```

Check driver:
```c
static const struct of_device_id my_of_match[] = {
    { .compatible = "vendor,my-device" },
    { }
};
MODULE_DEVICE_TABLE(of, my_of_match);
```

**Production or Debugging Angle:** Always confirm the bootloader loaded the DTB you changed. Many teams lose hours editing the right source but booting an old blob.

**Common Traps:** Looking only at `dmesg` from the driver when `probe()` never ran, or assuming `dtc` success means the node is valid.

**Follow-up Questions:**
- How do you check whether a node is disabled?
- What does `status` default to if omitted?
- Why might `dtbs_check` catch problems `dtc` misses?

## Senior Questions

### 9. What belongs in a binding, and why is binding compatibility important?
**Short Answer:** A binding documents the hardware contract for a compatible string: required/optional properties, resource formats, child nodes, and constraints. It matters because DTs and bindings are long-lived interfaces between firmware, kernels, bootloaders, and boards.

**Deep Explanation:** A driver can evolve, but a board DT may be used by multiple kernel versions or boot flows. If you rename properties, change meanings, or encode driver policy, you can break old kernels or future maintainability. Good bindings describe hardware facts with common properties when possible.

**API / Code Anchor:** Current upstream bindings are commonly YAML schemas under `Documentation/devicetree/bindings/`. Validation uses:
```bash
make dt_binding_check
make dtbs_check
make DT_SCHEMA_FILES=Documentation/devicetree/bindings/path/to/binding.yaml dtbs_check
```

**Production or Debugging Angle:** Schema validation catches missing required properties, bad types, invalid enum values, and undocumented compatibles before the board reaches bring-up.

**Common Traps:** Inventing `vendor,magic-mode` because it helps the current driver, instead of defining real hardware meaning or using an existing common property.

**Follow-up Questions:**
- Why should DT describe hardware, not Linux driver choices?
- When is a vendor-prefixed property appropriate?
- How do you handle a new hardware revision?

### 10. Explain the provider/consumer pattern across clocks, GPIOs, DMA, and interrupts.
**Short Answer:** Providers declare how references to them are encoded with `#*-cells`; consumers hold phandle lists plus arguments that identify a specific resource from the provider.

**Deep Explanation:** This lets the same syntax describe many resource types. A clock controller can expose multiple clock outputs through `#clock-cells`; a GPIO controller exposes lines and flags through `#gpio-cells`; a DMA controller exposes channel specifiers through `#dma-cells`; an interrupt controller exposes interrupt specifiers through `#interrupt-cells`.

**API / Code Anchor:**
```dts
clk: clock-controller@1000 {
    #clock-cells = <1>;
};

gpio: gpio@2000 {
    gpio-controller;
    #gpio-cells = <2>;
};

dev@3000 {
    clocks = <&clk 4>;
    reset-gpios = <&gpio 7 GPIO_ACTIVE_LOW>;
};
```

Driver/subsystem parsing may use helpers such as `devm_clk_get()`, GPIO descriptor APIs, or lower-level `of_parse_phandle_with_args()`.

**Production or Debugging Angle:** If a provider has the wrong `#*-cells`, consumers may parse every following argument incorrectly.

**Common Traps:** Assuming phandle arguments are globally defined. They are provider-binding-defined.

**Follow-up Questions:**
- Why do consumers often also have `*-names`?
- What does `#clock-cells = <0>` mean?
- Why should drivers usually prefer subsystem helpers over raw OF parsing?

### 11. How do overlays work, and what lifetime trap do they introduce?
**Short Answer:** A Device Tree overlay modifies the live tree, often adding or changing nodes. The lifetime trap is that pointers to overlay nodes/properties must not be kept after the overlay is removed.

**Deep Explanation:** Overlays are useful for add-on hardware or board variants. They can target nodes by label if the base DT was compiled with symbols, usually via `-@`, or by explicit target path. Applying an overlay can cause active device nodes to be populated; removing it can remove those nodes and devices.

**API / Code Anchor:** DTS overlay concepts:
```dts
/dts-v1/;
/plugin/;

&i2c1 {
    sensor@40 {
        compatible = "vendor,sensor";
        reg = <0x40>;
    };
};
```

**Production or Debugging Angle:** Overlay support depends on kernel configuration, platform policy, and deployment mechanism. Live removal is much more subtle than static boot-time DT.

**Common Traps:** Treating overlay nodes as permanent, or assuming label targets work when the base DTB was not compiled with symbol information.

**Follow-up Questions:**
- What is the difference between label target and target-path overlays?
- Why can overlay removal affect device lifetime?
- When would you avoid runtime overlays in production?

### 12. A board has an I2C sensor node, but no I2C device appears. What do you check?
**Short Answer:** Check the I2C controller node is enabled and has correct bus cells, the sensor is a child of that controller, `reg` is the slave address, the compatible matches a driver, and pinctrl/clocks/regulators for the controller are valid.

**Deep Explanation:** I2C children are instantiated by the I2C subsystem from children under the adapter/controller node. If the child is under the wrong parent, has a wrong `reg`, or the controller is disabled, no client will appear. If the client appears but driver binding fails, check `compatible` and module aliases.

**API / Code Anchor:**
```dts
&i2c1 {
    #address-cells = <1>;
    #size-cells = <0>;
    status = "okay";

    sensor@40 {
        compatible = "vendor,temp-sensor";
        reg = <0x40>;
    };
};
```

Debug:
```bash
ls /sys/bus/i2c/devices
dmesg | grep -i i2c
cat /sys/firmware/devicetree/base/.../i2c*/status
```

**Production or Debugging Angle:** A valid-looking child node does nothing if the parent bus is disabled or not probed.

**Common Traps:** Putting I2C clients under `/soc` as platform devices, or treating `reg = <0x40>` as an MMIO address.

**Follow-up Questions:**
- How is SPI child `reg` different from I2C child `reg`?
- Why does pinctrl matter for an I2C controller?
- How would `dtbs_check` help here?

### 13. What is your review checklist for a new DTS node in a production kernel?
**Short Answer:** Verify binding documentation, naming/style, `compatible`, address cells, resources, provider references, `status`, validation, and runtime deployment.

**Deep Explanation:** DTS review is both syntax and hardware correctness. The node must follow an existing or new binding, describe real hardware, and interoperate with the driver and subsystem helpers. You also need to verify the boot flow actually loads the resulting DTB.

**API / Code Anchor:** Typical review commands:
```bash
make ARCH=arm64 dtbs
make ARCH=arm64 dtbs_check
make DT_SCHEMA_FILES=Documentation/devicetree/bindings/... dtbs_check
```

Runtime checks:
```bash
cat /sys/firmware/devicetree/base/model
ls /sys/bus/platform/devices
dmesg | grep -iE 'probe|defer|dtb|of:'
```

**Production or Debugging Angle:** Review should catch both "will not boot/probe" issues and long-term binding compatibility issues.

**Common Traps:** Reviewing only the driver patch and ignoring the DTS/binding patch, or accepting a node that passes `dtc` but fails schema validation.

**Follow-up Questions:**
- Why should unit address match `reg`?
- Why are `*-names` important for multiple resources?
- What is the risk of changing an existing binding property?

### 14. How do you explain Device Tree to a junior engineer without oversimplifying it?
**Short Answer:** Device Tree is a structured hardware map: nodes are hardware blocks, properties describe identity/resources/wiring, and phandles connect consumers to providers.

**Deep Explanation:** The simple model is useful, but you must add the precise rules: bindings define meanings, parent buses define address parsing, `compatible` drives matching, and validation requires schemas. DT does not make hardware dynamic or automatically correct; it only gives the kernel data to interpret.

**API / Code Anchor:** A teaching snippet should include:
```dts
device@1000 {
    compatible = "vendor,device";
    reg = <0x1000 0x100>;
    interrupts = <0 5 IRQ_TYPE_LEVEL_HIGH>;
    reset-gpios = <&gpio0 3 GPIO_ACTIVE_LOW>;
    status = "okay";
};
```

**Production or Debugging Angle:** Good teaching prevents real bugs: wrong `reg`, wrong compatible, wrong active-low flag, and blindly inventing properties.

**Common Traps:** Saying "Device Tree is JSON for hardware" and stopping there. That analogy hides bindings, phandles, bus-specific parsing, and kernel population.

**Follow-up Questions:**
- What three properties would you explain first?
- Which concept usually confuses beginners most?
- How would you test that the explanation worked?
