# 10 - Device Tree Fundamentals

## Learning Goal
After this topic, you should be able to read a basic Device Tree source file, explain what each node/property means, reason about `compatible`, `reg`, interrupts, phandles, and provider/consumer relationships, and debug the first layer of DT bring-up problems.

You are not expected to memorize every binding. The goal is to understand the grammar and the repeated patterns so later driver topics feel familiar instead of mysterious.

## Why This Matters In Real Work
Most embedded Linux hardware is **not self-discoverable** like PCI or USB. The kernel cannot magically know which UART, GPIO controller, sensor, regulator, clock, display bridge, or audio codec is present on a board.

Device Tree solves that by moving board-specific hardware description out of driver C code and into data:

- One kernel can boot many boards by receiving a different `.dtb`.
- Drivers stay board-agnostic.
- Board wiring lives in `.dts`/`.dtsi` files.
- Subsystems can discover resources such as MMIO ranges, IRQs, clocks, GPIOs, DMA channels, regulators, and pin states.
- Driver matching can use `compatible` strings instead of hardcoded board files.

In real driver work, a bad Device Tree can look exactly like a bad driver: probe never runs, IRQs do not fire, registers map incorrectly, clocks are missing, GPIO polarity is inverted, or a device silently stays disabled.

## Mental Model
Think of Device Tree as a **hardware map handed to the kernel at boot**. It describes what hardware exists and how it is connected; the driver decides how to operate that hardware.

```
Board hardware
  -> described in .dts/.dtsi
  -> compiled by dtc into .dtb
  -> passed by bootloader to kernel
  -> unflattened into kernel nodes/properties
  -> used for matching, resource discovery, and device creation
```

Important boundaries:

| Device Tree should describe | Device Tree should not describe |
| --- | --- |
| Hardware topology | Driver algorithm choices |
| Register ranges | Kernel implementation policy |
| Interrupt wiring | Runtime user preferences |
| Clock/GPIO/DMA/regulator connections | Arbitrary magic values without binding docs |
| Board-specific enablement | A workaround that belongs in driver code |

Device Tree is declarative: it says "this UART is at this address, has this interrupt, consumes these clocks, and is enabled on this board."

## Core Concepts
Device Tree has a small grammar, but the same few ideas repeat everywhere.

| Concept | Meaning | Example |
| --- | --- | --- |
| `.dts` | Board-level Device Tree source | `imx6q-sabresd.dts` |
| `.dtsi` | Reusable include, often SoC-level | `imx6qdl.dtsi` |
| `.dtb` | Compiled binary blob passed to kernel | `imx6q-sabresd.dtb` |
| Node | A hardware object or grouping | `serial@02020000 { ... };` |
| Property | Data attached to a node | `compatible = "fsl,imx6q-uart";` |
| `compatible` | Hardware identity and driver/binding match key | `"vendor,chip-v2"` |
| `reg` | Address/resource description whose meaning depends on parent bus | `<0x02020000 0x4000>` |
| Label | Source-level name for references | `uart1: serial@02020000` |
| Phandle | Compiled node reference | `<&gpio1 7 GPIO_ACTIVE_LOW>` |
| Binding | Contract describing valid properties for hardware | YAML file under `Documentation/devicetree/bindings/` |

### File Types
`.dtsi` files usually describe shared SoC hardware. Board `.dts` files include them and enable only what is physically wired on that board.

```dts
/* SoC .dtsi */
uart1: serial@02020000 {
    compatible = "fsl,imx6q-uart", "fsl,imx21-uart";
    reg = <0x02020000 0x4000>;
    interrupts = <0 26 IRQ_TYPE_LEVEL_HIGH>;
    status = "disabled";
};

/* Board .dts */
&uart1 {
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_uart1>;
    status = "okay";
};
```

This split keeps common SoC definitions reusable while board files describe board-specific wiring.

### Data Types
Device Tree values are compact but precise.

| Type | Syntax | Meaning |
| --- | --- | --- |
| String | `"text"` | Human-readable or binding-defined text |
| String list | `"ipg", "per"` | Multiple strings |
| Cell | `<123>` or `<0x1234>` | One 32-bit unsigned value |
| Cell list | `<0x1000 0x100>` | Multiple 32-bit values |
| Byte array | `[00 11 22 33 44 55]` | Raw bytes |
| Boolean | `dma-coherent;` | True if present, false if absent |

### Naming Rules
Good names are not decoration. They prevent review noise and binding failures.

- Node names and property names use lowercase and dashes: `clock-frequency`, `interrupt-parent`.
- Labels commonly use underscores: `pinctrl_uart1`.
- Unit addresses should not include `0x` or leading zeros.
- A node with a unit address should usually have a matching `reg` property.
- Device-specific custom properties should use a vendor prefix, such as `vendor,mode`.

## Kernel Mechanism
At boot, the kernel receives a flattened DTB and builds an in-memory tree. Each node becomes a `struct device_node`; each property becomes a `struct property`.

The kernel uses the tree for several jobs:

- **Platform identification**: choose board/SoC setup based on root `compatible`.
- **Runtime configuration**: read global nodes such as `chosen`, memory, CPUs, and boot arguments.
- **Device population**: create devices from active nodes, commonly under buses such as `simple-bus`.
- **Driver matching**: compare a node's `compatible` strings with a driver's match table.
- **Resource translation**: turn DT properties such as `reg` and `interrupts` into resources drivers can request.

For platform devices, this often becomes:

```
DT node with compatible/reg/interrupts
  -> platform device created
  -> platform driver match table checks compatible
  -> probe() receives struct platform_device
  -> driver gets resources with platform_get_resource(), platform_get_irq(), etc.
```

Topic 11 goes deeper into OF APIs. Here, remember the shape: DT properties become kernel data structures, and driver/subsystem helper APIs interpret them according to bindings.

## Key Structs And APIs
These are the names you should recognize when reading kernel driver code. Do not treat this as a memorization list; each item connects to a mechanism.

| Struct/API | Role |
| --- | --- |
| `struct device_node` | In-kernel representation of one DT node |
| `struct property` | In-kernel representation of one DT property |
| `struct of_device_id` | Driver match table entry for DT `compatible` strings |
| `MODULE_DEVICE_TABLE(of, table)` | Exports OF aliases for module autoloading |
| `pdev->dev.of_node` | DT node associated with a platform device |
| `of_property_read_*()` | Reads custom properties from a node; detailed in topic 11 |
| `of_parse_phandle_with_args()` | Parses provider references such as `clocks`, `gpios`, `dmas`; detailed in topic 11 |
| `platform_get_resource()` | Gets resources produced from properties such as `reg` |
| `platform_get_irq()` | Gets Linux IRQ number from DT interrupt data |
| `dtc` | Compiles/decompiles DTS/DTB |
| `make dtbs` | Kernel build target for DTBs |
| `make dtbs_check` | Validates DTBs against YAML bindings |

### `compatible`
`compatible` identifies the hardware and is used by both bindings and drivers.

```dts
serial@02020000 {
    compatible = "fsl,imx6q-uart", "fsl,imx21-uart";
};
```

Rules of thumb:

- Put the most specific compatible first.
- Add fallback compatibles when hardware is compatible with an older/generic implementation.
- Driver match tables usually list compatible strings in `struct of_device_id`.
- Every new compatible should have binding documentation.

### `status`
`status` controls whether a node is considered enabled.

| Value | Meaning |
| --- | --- |
| `"okay"` | Device is enabled/usable |
| `"disabled"` | Device exists in SoC but is not enabled on this board |
| omitted | Usually treated as enabled |

Common pattern:

- SoC `.dtsi`: peripherals default to `"disabled"`.
- Board `.dts`: enable only wired peripherals with `"okay"`.

## Lifecycle / Data Flow
Device Tree has both a build-time flow and a runtime flow.

### Build-Time Flow
```
SoC .dtsi + board .dts + dt-bindings headers
  -> dtc
  -> .dtb
  -> boot image / boot partition / FIT image / firmware package
```

Validation flow:

```
make dtbs
make dt_binding_check
make dtbs_check
```

`dtc` catches syntax problems. `dtbs_check` catches binding/schema problems, which are often the ones that hurt driver bring-up.

### Runtime Flow
```
Bootloader loads kernel + DTB
  -> kernel unflattens DTB
  -> active nodes are populated as devices
  -> bus/driver matching checks compatible
  -> probe() runs
  -> driver reads resources/properties
  -> subsystem APIs consume clocks/GPIOs/IRQs/DMA/regulators
```

Debugging flow:

```
Is the right DTB loaded?
  -> check /sys/firmware/devicetree/base/model
Is the node present?
  -> inspect /sys/firmware/devicetree/base/...
Is status okay?
  -> check status property
Does compatible match the driver?
  -> compare DT compatible with of_device_id table
Are resources valid?
  -> check reg, interrupts, clocks, gpios, pinctrl
Did schema validation pass?
  -> run dtbs_check
```

## Addressing, Resources, And Phandles
This is where most beginner bugs live, so slow down here. `reg` does not have one universal meaning; the parent bus defines how to read it.

### `#address-cells` And `#size-cells`
These properties describe how to parse child addresses.

**Important rule:** a node's `#address-cells` and `#size-cells` affect its children, not itself.

| Parent bus style | Typical cells | Child `reg` means |
| --- | --- | --- |
| MMIO/simple-bus | `#address-cells = <1>`, `#size-cells = <1>` | `<base size>` |
| 64-bit address bus | `#address-cells = <2>`, `#size-cells = <2>` | `<addr_hi addr_lo size_hi size_lo>` |
| I2C bus | `#address-cells = <1>`, `#size-cells = <0>` | `<i2c_slave_address>` |
| SPI bus | `#address-cells = <1>`, `#size-cells = <0>` | `<chip_select_index>` |

### MMIO Example
```dts
soc {
    compatible = "simple-bus";
    #address-cells = <1>;
    #size-cells = <1>;
    ranges;

    uart0: serial@10000000 {
        compatible = "vendor,uart";
        reg = <0x10000000 0x1000>;
        interrupts = <0 10 IRQ_TYPE_LEVEL_HIGH>;
    };
};
```

For a platform driver, `reg` can become an `IORESOURCE_MEM`, and `interrupts` can become an IRQ resource.

### I2C vs SPI `reg`
This comparison is a classic interview and bring-up trap.

| Bus | Example | Meaning |
| --- | --- | --- |
| I2C | `eeprom@50 { reg = <0x50>; };` | I2C slave address `0x50` |
| SPI | `flash@1 { reg = <1>; };` | Chip-select index `1` |
| MMIO | `serial@10000000 { reg = <0x10000000 0x1000>; };` | Register base and size |

### Provider / Consumer Pattern
Many resources are described by phandles.

```dts
clk: clock-controller@020c4000 {
    compatible = "vendor,clock-controller";
    reg = <0x020c4000 0x4000>;
    #clock-cells = <1>;
};

gpio1: gpio@0209c000 {
    compatible = "vendor,gpio";
    reg = <0x0209c000 0x4000>;
    gpio-controller;
    #gpio-cells = <2>;
};

device@80000000 {
    compatible = "vendor,my-device";
    reg = <0x80000000 0x1000>;
    clocks = <&clk 15>;
    clock-names = "core";
    reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
};
```

Pattern:

- Provider node declares how many cells are needed: `#clock-cells`, `#gpio-cells`, `#dma-cells`, `#interrupt-cells`.
- Consumer property references provider with `<&provider args...>`.
- The meaning of `args` is defined by the provider binding.
- Name lists such as `clock-names` make driver lookup order-independent.

### Interrupts
Interrupt description has a controller side and a consumer side.

```dts
intc: interrupt-controller@a0021000 {
    compatible = "arm,cortex-a7-gic";
    interrupt-controller;
    #interrupt-cells = <3>;
    reg = <0xa0021000 0x1000>;
};

uart0: serial@10000000 {
    compatible = "vendor,uart";
    reg = <0x10000000 0x1000>;
    interrupt-parent = <&intc>;
    interrupts = <0 10 IRQ_TYPE_LEVEL_HIGH>;
};
```

Do not assume every interrupt specifier has the same format. The interrupt controller binding defines the cell meanings.

## Minimal Practical Example
This DTS snippet is **learning-only**. It is realistic enough to explain the mechanism, but not production-ready because a real board must follow exact upstream bindings for the SoC and devices.

```dts
/dts-v1/;

#include <dt-bindings/interrupt-controller/irq.h>
#include <dt-bindings/gpio/gpio.h>

/ {
    model = "Example Board";
    compatible = "example,board-v1";

    #address-cells = <1>;
    #size-cells = <1>;

    aliases {
        serial0 = &uart0;
    };

    chosen {
        stdout-path = &uart0;
    };

    memory@80000000 {
        device_type = "memory";
        reg = <0x80000000 0x10000000>;
    };

    intc: interrupt-controller@10001000 {
        compatible = "example,intc";
        reg = <0x10001000 0x1000>;
        interrupt-controller;
        #interrupt-cells = <2>;
    };

    gpio0: gpio@10002000 {
        compatible = "example,gpio";
        reg = <0x10002000 0x1000>;
        gpio-controller;
        #gpio-cells = <2>;
    };

    soc {
        compatible = "simple-bus";
        #address-cells = <1>;
        #size-cells = <1>;
        ranges;
        interrupt-parent = <&intc>;

        uart0: serial@10000000 {
            compatible = "example,uart-v2", "example,uart-v1";
            reg = <0x10000000 0x1000>;
            interrupts = <10 IRQ_TYPE_LEVEL_HIGH>;
            status = "okay";
        };

        mydev: device@10010000 {
            compatible = "example,my-device";
            reg = <0x10010000 0x1000>;
            interrupts = <11 IRQ_TYPE_LEVEL_HIGH>;
            reset-gpios = <&gpio0 7 GPIO_ACTIVE_LOW>;
            status = "okay";
        };
    };
};
```

What to notice:

- Root node describes board identity, address cells, memory, and boot console.
- `soc` is a `simple-bus`; its children use `<base size>` in `reg`.
- `interrupt-parent = <&intc>` is inherited by children.
- `reset-gpios = <&gpio0 7 GPIO_ACTIVE_LOW>` uses a phandle plus provider-defined arguments.
- `compatible` uses a specific string first and a fallback second.

The matching driver would have an OF match table:

```c
static const struct of_device_id mydev_of_match[] = {
    { .compatible = "example,my-device" },
    { }
};
MODULE_DEVICE_TABLE(of, mydev_of_match);
```

Topic 11 covers the full probe-side resource extraction.

## Common Bugs And Debugging
Start debugging from the symptom. DT bugs are often wiring/data bugs, not C syntax bugs.

| Symptom | Likely DT cause | What to inspect |
| --- | --- | --- |
| Driver `probe()` never runs | Wrong `compatible`, node disabled, wrong DTB loaded | `compatible`, `status`, `/sys/firmware/devicetree/base`, module aliases |
| `platform_get_resource()` fails | Missing/malformed `reg`, wrong parent cells | Parent `#address-cells`/`#size-cells`, node `reg` |
| IRQ request fails or no interrupts fire | Wrong `interrupt-parent`, wrong specifier format, wrong trigger | Controller binding, `interrupts`, `/proc/interrupts` |
| I2C/SPI device not created | Child is under wrong bus, wrong `reg`, controller disabled | Bus node `status`, child node path, bus scan logs |
| GPIO behaves inverted | Wrong active-low/high flag | `*-gpios` flags, binding docs, measured signal |
| Clock/regulator supplier missing | Provider node disabled/missing, wrong phandle/name | `clocks`, `clock-names`, `*-supply`, deferred-probe logs |
| `dtc` succeeds but kernel rejects/ignores data | Binding/schema violation | `make dtbs_check` |

Debug commands and clues:

```bash
# Runtime DT view
ls /sys/firmware/devicetree/base
cat /sys/firmware/devicetree/base/model

# Optional on some kernels/configurations
ls /proc/device-tree

# Decompile a DTB for inspection
dtc -I dtb -O dts -o /tmp/board.dts board.dtb

# Build and validate DTBs in a kernel tree
make ARCH=arm64 dtbs
make ARCH=arm64 dtbs_check
make DT_SCHEMA_FILES=Documentation/devicetree/bindings/i2c/i2c-imx.yaml dtbs_check

# Platform matching clues
ls /sys/bus/platform/devices
ls /sys/bus/platform/drivers
dmesg | grep -iE "probe|defer|of:|device tree|dtb"
```

Fix patterns:

- Confirm the bootloader loaded the DTB you edited.
- Validate against bindings, not only `dtc`.
- Check the parent bus cells before interpreting `reg`.
- Prefer named resources when a device has multiple resources.
- Keep SoC definitions in `.dtsi`, board wiring in `.dts`.
- Do not invent properties until you check existing bindings.

## Production Checklist
Use this before sending DTS or driver integration for review.

- [ ] The board file uses the correct root `model` and `compatible`.
- [ ] New `compatible` strings are documented in bindings.
- [ ] Properties follow existing common bindings when possible.
- [ ] Custom properties use a vendor prefix.
- [ ] Node names, unit addresses, property order, and labels follow current DTS style.
- [ ] Unit address matches `reg`.
- [ ] Parent `#address-cells` and `#size-cells` correctly describe child `reg`.
- [ ] SoC `.dtsi` devices default to disabled unless always present/usable.
- [ ] Board `.dts` enables only physically wired devices.
- [ ] Multiple resources have matching `*-names`.
- [ ] Interrupt specifiers match the interrupt controller binding.
- [ ] GPIO flags reflect electrical reality, especially active-low signals.
- [ ] Clocks, regulators, resets, pinctrl, DMA, and GPIO providers are enabled and referenced correctly.
- [ ] `dtc`, `dt_binding_check`, and `dtbs_check` pass where applicable.
- [ ] Runtime DT inspection confirms the expected DTB is loaded.
- [ ] Changes do not break binding compatibility for older kernels/bootloaders/users of the DT.

## Interview Readiness
For interviews, be ready to explain the mechanism rather than recite property names.

You should be able to:

- Explain why Device Tree exists for embedded Linux.
- Compare `.dts`, `.dtsi`, and `.dtb`.
- Explain node/property syntax and basic data types.
- Explain `compatible` and why specific-to-generic ordering matters.
- Parse `reg` for MMIO, I2C, and SPI examples.
- Explain why `#address-cells` and `#size-cells` are parent properties.
- Explain labels, phandles, aliases, and provider/consumer specifiers.
- Explain interrupt controller versus interrupt consumer properties.
- Debug "probe never runs" and "resource not found" scenarios.
- Explain why `dtc` alone is not enough for production validation.

Use `interview/10-device-tree-fundamentals.md` to practice structured answers.

## Kernel Version Notes
Device Tree basics are stable, but tooling and documentation style have evolved.

- Many older docs and examples reference `.txt` binding files. Current upstream bindings are commonly YAML schemas.
- Use `dt_binding_check` and `dtbs_check` for semantic validation; `dtc` mostly catches syntax/structural issues.
- Runtime DT may be visible through `/sys/firmware/devicetree/base`; `/proc/device-tree` depends on kernel configuration.
- Overlay support is kernel/config/platform-specific. For label-based overlays, the base DT generally needs symbol information from compiling with `-@`.
