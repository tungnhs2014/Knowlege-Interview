# Part 1. Device Tree Fundamentals

This part introduces the Device Tree mechanism, explaining its syntax, naming conventions, compilation process, and the fundamental concepts of aliases, labels, and phandles.

---

## 6.1 Introduction to Device Tree

### 6.1.1 What is Device Tree?

**Definition:**

A **Device Tree (DT)** is an easy-to-read hardware description file with a JSON-like formatting style. It is a simple tree structure where devices are represented by nodes with their properties.

```
┌─────────────────────────────────────────────────┐
│         Device Tree Concept                     │
├─────────────────────────────────────────────────┤
│                                                 │
│  Hardware Description   ─────▶   OS Kernel     │
│  (What hardware exists)        (How to use it)  │
│                                                 │
│  .dts (Source)          ─────▶   .dtb (Binary) │
│  Human-readable                  Machine-ready  │
│                                                 │
└─────────────────────────────────────────────────┘
```

**From the Linux kernel documentation:**

> "The 'Open Firmware Device Tree', or simply Device Tree (DT), is a data exchange format used for exchanging hardware description data with the software or OS. More specifically, it is a description of hardware that is readable by an operating system so that the operating system doesn't need to hard code details of the machine."
> 

**Key Points:**

- **Hardware description language**: Describes the hardware topology
- **OS-agnostic**: Can be used by U-Boot, FreeBSD, Linux, or any OS
- **Declarative**: Describes "what is" not "how to use"
- **Separates concerns**: Hardware description separate from driver code

### 6.1.2 Why Device Tree?

**The Old Problem:**

```
Traditional Linux (before Device Tree):
┌──────────────────────────────────────┐
│        Kernel Source Code            │
│                                      │
│  ┌────────────────────────────────┐  │
│  │  Board File (arch/arm/mach-*/) │  │
│  │  ─ Device definitions          │  │
│  │  ─ Resource descriptions       │  │
│  │  ─ Hardcoded in C code         │  │
│  └────────────────────────────────┘  │
│                                      │
│  Problems:                           │
│  ❌ One kernel per board             │
│  ❌ Recompile for any HW change      │
│  ❌ Code duplication                 │
│  ❌ Difficult maintenance            │
└──────────────────────────────────────┘
```

**The Device Tree Solution:**

```
Modern Linux (with Device Tree):
┌──────────────────────────────────────┐
│     Kernel (Generic, Board-Agnostic) │
└──────────────────────────────────────┘
              +
┌──────────────────────────────────────┐
│   Device Tree Blob (.dtb)            │
│   ─ Hardware description             │
│   ─ Board-specific                   │
│   ─ Runtime loadable                 │
└──────────────────────────────────────┘

Benefits:
✅ One kernel, multiple boards
✅ No recompilation for HW changes
✅ Cleaner driver code
✅ Better maintainability
```

**Real-world impact:**

Before Device Tree, ARM Linux had **thousands** of board files. After Device Tree adoption, these were eliminated, making the kernel cleaner and more maintainable.

### 6.1.3 Device Tree Design Principles

**From Bootlin training materials:**

The Device Tree follows these fundamental principles:

1. **Describe hardware, not configuration**
    - DT describes "how the hardware is"
    - NOT "how I choose to use the hardware"
2. **OS-agnostic**
    - Same DT for U-Boot, FreeBSD, or Linux
    - No need to change DT when updating OS
3. **Describe integration, not internals**
    - How device/IP block is connected to the system
    - IRQ lines, DMA channels, clocks, reset lines
    - Device driver handles internal details

**Important:**

> "The most important thing to understand is that the DT is simply a data structure that describes the hardware. There is nothing magical about it, and it does not magically make all hardware configuration problems go away."
> 

### 6.1.4 Enabling Device Tree in Linux

**Kernel configuration:**

Device Tree support is enabled with:

```bash
CONFIG_OF=y
```

**Required headers in driver:**

```c
#include <linux/of.h>           /* OF core APIs */
#include <linux/of_device.h>    /* OF device matching */
```

**Checking for Device Tree:**

```c
static int my_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;

    if (!np) {
        dev_err(&pdev->dev, "No device tree node\n");
        return -EINVAL;
    }

    /* Device tree is present, continue... */
    return 0;
}
```

---

## 6.2 Device Tree File Types and Structure

### 6.2.1 File Types Overview

Device Trees come in three forms:

```
┌─────────────────────────────────────────────────┐
│  1. .dts (Device Tree Source)                   │
│     ─ Human-readable text format                │
│     ─ Board-level definitions                   │
│     ─ Includes .dtsi files                      │
├─────────────────────────────────────────────────┤
│  2. .dtsi (Device Tree Source Include)          │
│     ─ SoC-level definitions                     │
│     ─ Like C header files                       │
│     ─ Reusable across boards                    │
├─────────────────────────────────────────────────┤
│  3. .dtb (Device Tree Blob)                     │
│     ─ Compiled binary format                    │
│     ─ Loaded by bootloader                      │
│     ─ Passed to kernel at boot                  │
└─────────────────────────────────────────────────┘
```

**File relationship:**

```
SoC Level (.dtsi)          Board Level (.dts)
┌──────────────────┐       ┌──────────────────┐
│ imx6qdl.dtsi     │       │ myboard.dts      │
│                  │       │                  │
│ - CPU definition │       │ #include         │
│ - Peripherals    │◀──────│  "imx6qdl.dtsi"  │
│ - Default config │       │                  │
│ - status=disabled│       │ - Board specifics│
└──────────────────┘       │ - status=okay    │
                           │ - Custom props   │
                           └──────────────────┘
                                    │
                                    │ dtc (compile)
                                    ▼
                           ┌──────────────────┐
                           │ myboard.dtb      │
                           │ (Binary blob)    │
                           └──────────────────┘
```

### 6.2.2 Overall Device Tree Structure

**Basic tree structure:**

```
/dts-v1/;  /* Version declaration */

/ {
    /* Root node properties */
    model = "Manufacturer Board Name";
    compatible = "vendor,board", "vendor,soc";

    #address-cells = <1>;
    #size-cells = <1>;

    /* Standard nodes */
    cpus {
        /* CPU descriptions */
    };

    memory@0 {
        /* Memory description */
    };

    chosen {
        /* Bootloader parameters */
    };

    /* Device nodes */
    soc {
        /* SoC peripherals */
    };
};
```

**Complete example from STM32MP1:**

```
/dts-v1/;

/ {
    #address-cells = <1>;
    #size-cells = <1>;

    model = "STMicroelectronics STM32MP157C-DK2 Discovery Board";
    compatible = "st,stm32mp157c-dk2", "st,stm32mp157";

    cpus {
        #address-cells = <1>;
        #size-cells = <0>;

        cpu0: cpu@0 {
            compatible = "arm,cortex-a7";
            device_type = "cpu";
            reg = <0>;
            clock-frequency = <650000000>;
        };

        cpu1: cpu@1 {
            compatible = "arm,cortex-a7";
            device_type = "cpu";
            reg = <1>;
            clock-frequency = <650000000>;
        };
    };

    memory@c0000000 {
        device_type = "memory";
        reg = <0xc0000000 0x20000000>;  /* 512 MB */
    };

    chosen {
        bootargs = "";
        stdout-path = "serial0:115200n8";
    };

    /* Interrupt controller */
    intc: interrupt-controller@a0021000 {
        compatible = "arm,cortex-a7-gic";
        #interrupt-cells = <3>;
        interrupt-controller;
        reg = <0xa0021000 0x1000>,
              <0xa0022000 0x2000>;
    };

    /* SoC bus */
    soc {
        compatible = "simple-bus";
        #address-cells = <1>;
        #size-cells = <1>;
        interrupt-parent = <&intc>;
        ranges;

        /* I2C controller */
        i2c1: i2c@40012000 {
            compatible = "st,stm32mp15-i2c";
            reg = <0x40012000 0x400>;
            interrupts = <GIC_SPI 31 IRQ_TYPE_LEVEL_HIGH>;
            clocks = <&rcc I2C1_K>;
            resets = <&rcc I2C1_R>;
            #address-cells = <1>;
            #size-cells = <0>;
            status = "okay";
        };

        /* Ethernet controller */
        ethernet0: ethernet@5800a000 {
            compatible = "st,stm32mp1-dwmac", "snps,dwmac-4.20a";
            reg = <0x5800a000 0x2000>;
            interrupts = <GIC_SPI 61 IRQ_TYPE_LEVEL_HIGH>;
            clocks = <&rcc ETHMAC>;
            clock-names = "stmmaceth";
            status = "okay";
        };
    };
};
```

---

## 6.3 Device Tree Syntax and Data Types

### 6.3.1 Basic Syntax Rules

**Node syntax:**

```
node_label: node-name@unit-address {
    property-name = <value>;
    another-property;
    sub-node {
        /* ... */
    };
};
```

**Comments:**

```
/* This is a C-style comment */
// This is also a comment
node {
    property = <0x123>;  // Inline comment
};
```

### 6.3.2 Data Types

Device Tree supports several data types:

### **1. Text Strings**

**Syntax:** Enclosed in double quotes

```
node {
    string-property = "a string";
    string-list = "red fish", "blue fish";
    model = "Vendor Board v1.0";
    compatible = "vendor,device", "generic,device";
};
```

**In driver:**

```c
const char *my_string;
int ret;

ret = of_property_read_string(np, "string-property", &my_string);
if (!ret)
    pr_info("String: %s\n", my_string);
```

### **2. Cells (32-bit Unsigned Integers)**

**Syntax:** Enclosed in angle brackets `< >`

```
node {
    one-cell-property = <197>;

    /* Hexadecimal */
    hex-value = <0xdeadbeef>;

    /* Multiple cells - this is a list */
    int-list-property = <0xbeef 123 0xabcd4>;

    /* Each number is a 32-bit integer (uint32) */
    /* There are 3 cells in int-list-property */
};
```

**In driver:**

```c
u32 number;
u32 cells_array[4];

/* Read single cell */
of_property_read_u32(np, "one-cell-property", &number);

/* Read array of cells */
of_property_read_u32_array(np, "int-list-property", cells_array, 3);

pr_info("Number: %u\n", number);
pr_info("Array: [%u, %u, %u]\n",
        cells_array[0], cells_array[1], cells_array[2]);
```

### **3. Boolean Properties**

**Syntax:** Empty property (no value)

```
node {
    boolean-property;        /* Property exists = true */
    /* not-present-property */  /* Property absent = false */
};
```

**In driver:**

```c
bool my_bool;

my_bool = of_property_read_bool(np, "boolean-property");

if (my_bool) {
    /* Boolean is true - property exists */
    dev_info(&pdev->dev, "Feature enabled\n");
} else {
    /* Boolean is false - property doesn't exist */
    dev_info(&pdev->dev, "Feature disabled\n");
}
```

### **4. Byte Arrays**

**Syntax:** Enclosed in square brackets `[ ]`

```
node {
    byte-array-property = [0x01 0x23 0x45 0x67];
    mac-address = [00 11 22 33 44 55];
};
```

### **5. Mixed Properties**

**You can mix different types:**

```
node {
    mixed-list-property = "a string", <0xadbcd45>, <35>, [0x01 0x23 0x45];
    /* String, cell, cell, byte-array */
};
```

### 6.3.3 Complete Data Type Example

```
/dts-v1/;

/ {
    example_device: device@80000000 {
        compatible = "vendor,example-device";
        reg = <0x80000000 0x1000>;

        /* String property */
        device-name = "Example Device";

        /* String list */
        clock-names = "core", "bus", "peripheral";

        /* Single cell (u32) */
        clock-frequency = <100000000>;  /* 100 MHz */

        /* Cell array */
        operating-points = <1000000 1350000>,  /* 1.0 GHz @ 1.35V */
                          <800000  1200000>;   /* 800 MHz @ 1.2V */

        /* Boolean properties */
        dma-coherent;
        big-endian;

        /* Byte array */
        local-mac-address = [00 1A 2B 3C 4D 5E];

        /* Mixed */
        custom-data = "header", <0x12345678>, [AB CD EF];
    };
};
```

**Driver code to extract all properties:**

```c
static int example_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    const char *name;
    u32 freq;
    u32 op_points[4];
    bool coherent, big_endian;

    /* Read string */
    if (of_property_read_string(np, "device-name", &name) == 0)
        dev_info(&pdev->dev, "Device name: %s\n", name);

    /* Read u32 */
    if (of_property_read_u32(np, "clock-frequency", &freq) == 0)
        dev_info(&pdev->dev, "Frequency: %u Hz\n", freq);

    /* Read u32 array */
    if (of_property_read_u32_array(np, "operating-points",
                                   op_points, 4) == 0) {
        dev_info(&pdev->dev, "OP1: %u kHz @ %u uV\n",
                 op_points[0], op_points[1]);
        dev_info(&pdev->dev, "OP2: %u kHz @ %u uV\n",
                 op_points[2], op_points[3]);
    }

    /* Read booleans */
    coherent = of_property_read_bool(np, "dma-coherent");
    big_endian = of_property_read_bool(np, "big-endian");

    dev_info(&pdev->dev, "DMA coherent: %s\n",
             coherent ? "yes" : "no");
    dev_info(&pdev->dev, "Big endian: %s\n",
             big_endian ? "yes" : "no");

    return 0;
}
```

---

## 6.4 Naming Conventions

### 6.4.1 Node Naming Rules

**General format:**

```
node-name[@unit-address]
```

**Rules from Linux conventions:**

```
Node Names:
✓ Should begin with: 'a'-'z', 'A'-'Z'
✓ Use dash "-" NOT underscore "_"
✓ Unit address: no "0x" prefix
✓ Unit address: no leading zeros
✓ Max length: 31 characters

Examples:
✅ uart@02020000
✅ i2c@021a0000
✅ ethernet@5800a000
✅ cpu@0

❌ uart_device (underscore)
❌ uart@0x02020000 (0x prefix)
❌ uart@0002020000 (leading zeros)
```

**Node name requirements:**

- **`<name>`**: String up to 31 characters
- **`@<unit-address>`**: Optional, depends on whether node is addressable
- **`<unit-address>`**: Primary address to access the device

**Examples:**

```
/* I2C expander at address 0x20 */
expander@20 {
    compatible = "microchip,mcp23017";
    reg = <0x20>;
};

/* I2C controller at MMIO 0x021a0000 */
i2c@021a0000 {
    compatible = "fsl,imx6q-i2c", "fsl,imx21-i2c";
    reg = <0x021a0000 0x4000>;
};

/* CPU without unit address (indexed by reg) */
cpu@0 {
    compatible = "arm,cortex-a7";
    device_type = "cpu";
    reg = <0>;
};
```

### 6.4.2 Property Naming Rules

**From Linux conventions:**

```
Property Names:
✓ Should be lowercase
✓ Begin with: 'a'-'z'
✓ Use dash "-" NOT underscore "_"

Examples:
✅ clock-frequency
✅ interrupt-parent
✅ dma-names

❌ Clock_Frequency (uppercase, underscore)
❌ InterruptParent (camelCase)
```

### 6.4.3 Label Naming Rules

**Labels are used for references:**

```
Label Names:
✓ Should begin with: 'a'-'z', 'A'-'Z'
✓ Should be lowercase (convention)
✓ Use underscore "_" NOT dash "-"

Examples:
✅ uart1
✅ i2c_bus
✅ intc

❌ uart-1 (dash in label)
❌ UART1 (uppercase by convention)
```

**Example with labels:**

```
/* Define labels */
uart1: serial@02020000 {
    compatible = "fsl,imx6q-uart";
    reg = <0x02020000 0x4000>;
};

gpio1: gpio@0209c000 {
    compatible = "fsl,imx6q-gpio";
    reg = <0x0209c000 0x4000>;
};

/* Reference labels */
some_device {
    uart = <&uart1>;
    reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
};
```

### 6.4.4 Complete Naming Example

```
/dts-v1/;

/ {
    model = "My Custom Board";
    compatible = "myvendor,myboard";

    /* Labels use underscores */
    main_uart: serial@02020000 {
        /* Properties use dashes */
        compatible = "fsl,imx6q-uart";
        reg = <0x02020000 0x4000>;
        interrupt-parent = <&intc>;
        interrupts = <0 26 IRQ_TYPE_LEVEL_HIGH>;
        clock-names = "ipg", "per";
        dma-names = "rx", "tx";
        /* Boolean - no value */
        dma-coherent;
    };

    /* Node names use dashes, with @address */
    i2c-controller@021a0000 {
        compatible = "fsl,imx6q-i2c";
        reg = <0x021a0000 0x4000>;
        #address-cells = <1>;
        #size-cells = <0>;

        /* I2C device - address in node name */
        eeprom@50 {
            compatible = "atmel,24c02";
            reg = <0x50>;
            pagesize = <16>;
        };
    };
};
```

---

## 6.5 Aliases, Labels, and Phandles

These three concepts are fundamental to Device Tree and often confuse beginners. Let's clarify each one.

### 6.5.1 Understanding Labels

**What is a label?**

A **label** is a way to tag a node so it can be identified by a unique name. Think of it as giving a node a nickname for easy reference.

```
label_name: node@address {
    /* node properties */
};
```

**How labels work:**

```
Source Code (.dts):          After Compilation (.dtb):
label: node { }      ──▶     phandle = <unique_32bit_value>
```

The DT compiler converts each label into a unique 32-bit value called a **phandle**.

**Example:**

```
aliases {
    ethernet0 = &fec;
    gpio0 = &gpio1;
    gpio1 = &gpio2;
    mmc0 = &usdhc1;
};

/* Labels defined here */
gpio1: gpio@0209c000 {
    compatible = "fsl,imx6q-gpio", "fsl,imx35-gpio";
    reg = <0x0209c000 0x4000>;
    gpio-controller;
    #gpio-cells = <2>;
};

fec: ethernet@02188000 {
    compatible = "fsl,imx6q-fec";
    reg = <0x02188000 0x4000>;
};

/* Using label reference */
node_label: nodename@reg {
    gpios = <&gpio1 7 GPIO_ACTIVE_HIGH>;
    /*       ^^^^^
     *       Reference to gpio1 label above
     */
};
```

### 6.5.2 Understanding Phandles

**What is a phandle?**

A **phandle** (pointer handle) is a **32-bit value** associated with a node that uniquely identifies it. This allows one node to reference another node.

**Syntax:** `<&label_name>`

The `&` operator is like in C programming - it obtains the "address" (phandle) of a node.

```
┌─────────────────────────────────────────────┐
│  &label  →  Get phandle of labeled node     │
│                                             │
│  Like C:  &variable → Get address           │
└─────────────────────────────────────────────┘
```

**Example:**

```
/* Define nodes with labels */
intc: interrupt-controller@a0021000 {
    compatible = "arm,cortex-a7-gic";
    interrupt-controller;
    #interrupt-cells = <3>;
    reg = <0xa0021000 0x1000>;
    phandle = <1>;  /* DT compiler assigns this */
};

uart1: serial@02020000 {
    compatible = "fsl,imx6q-uart";
    reg = <0x02020000 0x4000>;

    /* Reference intc via phandle */
    interrupt-parent = <&intc>;
    /*                  ^^^^^^
     *                  Becomes: interrupt-parent = <1>;
     */

    interrupts = <0 26 IRQ_TYPE_LEVEL_HIGH>;
};
```

**How it works internally:**

```
/* What you write: */
device {
    property = <&mylabel>;
};

mylabel: other-device {
    /* ... */
};

/* What DT compiler creates: */
device {
    property = <0x00000005>;  /* phandle value */
};

other-device {
    phandle = <0x00000005>;
    /* ... */
};
```

### 6.5.3 Understanding Aliases

**What is an alias?**

The **aliases** node is a quick lookup table - an index of nodes. It allows finding nodes by short, friendly names instead of full paths.

```
aliases {
    alias-name = &node_label;
};
```

**Purpose:**

Without aliases, you'd need to traverse the entire tree to find a node. Aliases provide O(1) lookup time.

```
Finding a node:
Without alias: Walk entire tree  →  O(n) time
With alias:    Direct lookup     →  O(1) time
```

**Example:**

```
aliases {
    ethernet0 = &fec;
    ethernet1 = &fec2;
    gpio0 = &gpio1;
    gpio1 = &gpio2;
    i2c0 = &i2c1;
    i2c1 = &i2c2;
    mmc0 = &usdhc1;
    mmc1 = &usdhc2;
    serial0 = &uart1;
    serial1 = &uart2;
};

uart1: serial@02020000 {
    compatible = "fsl,imx6q-uart";
    reg = <0x02020000 0x4000>;
};

uart2: serial@021e8000 {
    compatible = "fsl,imx6q-uart";
    reg = <0x021e8000 0x4000>;
};
```

**Using aliases in driver:**

```c
struct device_node *np;

/* Find node by alias */
np = of_find_node_by_alias(NULL, "serial0");
if (np) {
    pr_info("Found uart1 via alias 'serial0'\n");
    /* np now points to uart1 node */
}
```

**Aliases vs Labels - The Difference:**

```
┌──────────────────────────────────────────────────┐
│ Labels:                                          │
│  - Used in DT source (.dts)                      │
│  - For referencing nodes from other nodes        │
│  - Converted to phandles at compile time         │
│  - Example: gpios = <&gpio1 5 0>;                │
├──────────────────────────────────────────────────┤
│ Aliases:                                         │
│  - Used by Linux kernel at runtime               │
│  - For finding nodes quickly by name             │
│  - NOT directly used in DT source                │
│  - Example: of_find_node_by_alias(NULL, "i2c0")  │
└──────────────────────────────────────────────────┘
```

### 6.5.4 Complete Example: Labels, Phandles, and Aliases

```
/dts-v1/;

/ {
    /* Aliases for runtime lookup */
    aliases {
        ethernet0 = &fec;
        i2c0 = &i2c1;
        serial0 = &uart1;
    };

    /* Label: intc */
    intc: interrupt-controller@a0021000 {
        compatible = "arm,cortex-a7-gic";
        interrupt-controller;
        #interrupt-cells = <3>;
        reg = <0xa0021000 0x1000>;
    };

    /* Label: clk */
    clk: clock-controller@020c4000 {
        compatible = "fsl,imx6q-ccm";
        reg = <0x020c4000 0x4000>;
        #clock-cells = <1>;
    };

    /* Label: gpio1 */
    gpio1: gpio@0209c000 {
        compatible = "fsl,imx6q-gpio";
        reg = <0x0209c000 0x4000>;
        interrupts = <0 66 IRQ_TYPE_LEVEL_HIGH>;
        gpio-controller;
        #gpio-cells = <2>;
        interrupt-controller;
        #interrupt-cells = <2>;

        /* This node uses phandle to reference intc */
        interrupt-parent = <&intc>;
    };

    /* Label: uart1 */
    uart1: serial@02020000 {
        compatible = "fsl,imx6q-uart";
        reg = <0x02020000 0x4000>;

        /* Phandle references to other nodes */
        interrupt-parent = <&intc>;
        interrupts = <0 26 IRQ_TYPE_LEVEL_HIGH>;

        clocks = <&clk IMX6QDL_CLK_UART_IPG>,
                 <&clk IMX6QDL_CLK_UART_SERIAL>;
        clock-names = "ipg", "per";
    };

    /* Label: fec */
    fec: ethernet@02188000 {
        compatible = "fsl,imx6q-fec";
        reg = <0x02188000 0x4000>;

        /* Multiple phandle references */
        interrupt-parent = <&intc>;
        interrupts = <0 118 IRQ_TYPE_LEVEL_HIGH>,
                     <0 119 IRQ_TYPE_LEVEL_HIGH>;

        clocks = <&clk IMX6QDL_CLK_ENET>,
                 <&clk IMX6QDL_CLK_ENET>,
                 <&clk IMX6QDL_CLK_ENET_REF>;
        clock-names = "ipg", "ahb", "ptp";
    };

    /* Consumer node using GPIO phandle */
    my_device {
        compatible = "vendor,my-device";

        /* Reference gpio1 by phandle */
        reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
        /*             ^^^^^^
         *             This is a phandle reference
         *             Resolved to gpio1 node above
         */
    };
};
```

**Visual representation:**

```
┌─────────────────────────────────────────────────┐
│                   Aliases Node                  │
│  ethernet0 ───────────┐                         │
│  serial0 ─────────┐   │                         │
└───────────────────┼───┼─────────────────────────┘
                    │   │
       ┌────────────┘   └────────────┐
       │                             │
       ▼                             ▼
   uart1: serial@...           fec: ethernet@...
   │                           │
   │ interrupt-parent = <&intc>│ interrupt-parent = <&intc>
   │ clocks = <&clk ...>       │ clocks = <&clk ...>
   │                           │
   └──────┬────────────────────┴───────┐
          │                            │
          ▼                            ▼
    intc: interrupt-...          clk: clock-...
```

---

## 6.6 Device Tree Compilation

### 6.6.1 The Device Tree Compiler (dtc)

**What is dtc?**

The **Device Tree Compiler (dtc)** is the tool that converts human-readable `.dts` files into binary `.dtb` blobs that the kernel can understand.

```
Source (.dts)  ──[dtc]──▶  Binary (.dtb)
Human-readable            Machine-readable
```

**Tool location:**

```bash
# DTC binary
/usr/bin/dtc

# or built with kernel
scripts/dtc/dtc
```

### 6.6.2 Compilation Commands

**Compile single .dts file:**

```bash
# Basic compilation
dtc -I dts -O dtb -o myboard.dtb myboard.dts

# With include path
dtc -I dts -O dtb -o myboard.dtb -i /path/to/includes myboard.dts
```

**Parameters:**

- `I dts`: Input format is Device Tree Source
- `O dtb`: Output format is Device Tree Blob
- `o myboard.dtb`: Output filename
- `i path`: Include path for .dtsi files

**Compile all device trees for architecture:**

```bash
# ARM architecture
ARCH=arm make dtbs

# ARM64
ARCH=arm64 make dtbs

# With cross-compiler
ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make dtbs
```

**Compile specific device tree:**

```bash
# For specific board
ARCH=arm make imx6dl-sabrelite.dtb

# Multiple DTBs
ARCH=arm make imx6q-sabresd.dtb imx6dl-sabresd.dtb
```

### 6.6.3 Decompilation (Reverse Operation)

**Convert .dtb back to .dts:**

```bash
# Basic decompilation
dtc -I dtb -O dts -o output.dts input.dtb

# From specific location
dtc -I dtb -O dts -o /tmp/board.dts arch/arm/boot/dts/imx6dl-sabrelite.dtb
```

**This is useful for:**

- Debugging compiled device trees
- Reverse-engineering hardware
- Understanding what bootloader passed to kernel

### 6.6.4 Device Tree Include Mechanism

**.dtsi files (SoC-level) vs .dts files (Board-level):**

```
┌─────────────────────────────────────────────────┐
│  .dtsi (Device Tree Source Include)             │
│  ─ SoC-level definitions                        │
│  ─ Common to multiple boards                    │
│  ─ Peripherals in "disabled" state              │
├─────────────────────────────────────────────────┤
│  .dts (Device Tree Source)                      │
│  ─ Board-level definitions                      │
│  ─ Includes .dtsi files                         │
│  ─ Enables specific peripherals                 │
│  ─ Adds board-specific properties               │
└─────────────────────────────────────────────────┘
```

**Example hierarchy:**

```
/* imx6qdl.dtsi - SoC level (common) */
/ {
    soc {
        uart1: serial@02020000 {
            compatible = "fsl,imx6q-uart";
            reg = <0x02020000 0x4000>;
            interrupts = <0 26 IRQ_TYPE_LEVEL_HIGH>;
            clocks = <&clks IMX6QDL_CLK_UART_IPG>,
                     <&clks IMX6QDL_CLK_UART_SERIAL>;
            clock-names = "ipg", "per";
            status = "disabled";  /* Disabled by default */
        };
    };
};

/* imx6q.dtsi - Quad-core specific */
#include "imx6qdl.dtsi"

/ {
    cpus {
        cpu@0 { /* ... */ };
        cpu@1 { /* ... */ };
        cpu@2 { /* ... */ };
        cpu@3 { /* ... */ };
    };
};

/* imx6q-sabresd.dts - Board specific */
#include "imx6q.dtsi"

/ {
    model = "Freescale i.MX6 Quad SABRE Smart Device Platform";
    compatible = "fsl,imx6q-sabresd", "fsl,imx6q";
};

/* Enable uart1 for this board */
&uart1 {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_uart1>;
};
```

**The `&label` syntax for modification:**

```
/* SoC .dtsi file defines */
uart1: serial@02020000 {
    compatible = "fsl,imx6q-uart";
    reg = <0x02020000 0x4000>;
    status = "disabled";
};

/* Board .dts file modifies */
&uart1 {
    status = "okay";
    /* Add/override properties */
};
```

**This is equivalent to:**

```
#include "soc.dtsi"

/ {
    soc {
        serial@02020000 {
            status = "okay";  /* Override */
        };
    };
};
```

But the `&uart1` syntax is **cleaner and preferred**.

### 6.6.5 C Preprocessor Support

Device Tree files support C preprocessor directives:

```
#include "imx6qdl.dtsi"
#include <dt-bindings/gpio/gpio.h>
#include <dt-bindings/interrupt-controller/irq.h>

#define MY_CUSTOM_VALUE  100

/ {
    device {
        custom-prop = <MY_CUSTOM_VALUE>;
        gpios = <&gpio1 5 GPIO_ACTIVE_LOW>;
        interrupts = <0 26 IRQ_TYPE_LEVEL_HIGH>;
    };
};
```

**Common include files:**

```c
#include <dt-bindings/gpio/gpio.h>
#include <dt-bindings/interrupt-controller/irq.h>
#include <dt-bindings/interrupt-controller/arm-gic.h>
#include <dt-bindings/clock/imx6qdl-clock.h>
#include <dt-bindings/input/input.h>
```

### 6.6.6 Runtime Device Tree Access

**Accessing Device Tree at runtime:**

When `CONFIG_PROC_DEVICETREE=y`, the device tree is exposed in `/proc`:

```bash
# View device tree at runtime
ls -R /proc/device-tree/

# Read properties
cat /proc/device-tree/model
cat /proc/device-tree/compatible
cat /proc/device-tree/chosen/bootargs
```

**Example output:**

```bash
$ cat /proc/device-tree/model
Freescale i.MX6 Quad SABRE Smart Device Platform

$ ls /proc/device-tree/soc/
aips-bus@02000000/  aips-bus@02100000/  compatible  name  ranges
```

**This is useful for:**

- Debugging device tree at runtime
- Verifying bootloader passed correct DTB
- Checking property values without recompiling

### 6.6.7 Device Tree Validation

**Syntax validation (dtc does this automatically):**

```bash
# dtc will report syntax errors
dtc -I dts -O dtb -o board.dtb board.dts

# Example error:
# Error: board.dts:45.1-10 syntax error
```

**Semantic validation (YAML bindings):**

Modern Device Tree validation uses YAML schemas:

```bash
# Check bindings are valid
make dt_binding_check

# Validate device trees against bindings
make dtbs_check

# Check specific binding
make DT_SCHEMA_FILES=Documentation/devicetree/bindings/i2c/i2c-imx.yaml dtbs_check
```

**Important:** `dtc` only does **syntax checking**, not **semantic validation**. YAML bindings provide semantic validation.

---

## 6.7 Complete Examples

### 6.7.1 Minimal Device Tree Example

```
/dts-v1/;

/ {
    model = "Simple Board";
    compatible = "vendor,simple-board";

    #address-cells = <1>;
    #size-cells = <1>;

    chosen {
        bootargs = "console=ttyS0,115200 root=/dev/mmcblk0p2 rootwait";
        stdout-path = &uart0;
    };

    memory@80000000 {
        device_type = "memory";
        reg = <0x80000000 0x10000000>;  /* 256 MB */
    };

    uart0: serial@10000000 {
        compatible = "vendor,uart";
        reg = <0x10000000 0x1000>;
        interrupts = <0 10 4>;
    };
};
```

### 6.7.2 Real-World i.MX6 Example

```
/dts-v1/;

#include "imx6q.dtsi"
#include <dt-bindings/gpio/gpio.h>
#include <dt-bindings/input/input.h>

/ {
    model = "Freescale i.MX6 Quad SABRE Smart Device Platform";
    compatible = "fsl,imx6q-sabresd", "fsl,imx6q";

    chosen {
        stdout-path = &uart1;
    };

    memory@10000000 {
        reg = <0x10000000 0x40000000>;  /* 1 GB */
    };

    regulators {
        compatible = "simple-bus";
        #address-cells = <1>;
        #size-cells = <0>;

        reg_usb_otg_vbus: regulator@0 {
            compatible = "regulator-fixed";
            reg = <0>;
            regulator-name = "usb_otg_vbus";
            regulator-min-microvolt = <5000000>;
            regulator-max-microvolt = <5000000>;
            gpio = <&gpio3 22 GPIO_ACTIVE_HIGH>;
            enable-active-high;
        };
    };

    leds {
        compatible = "gpio-leds";

        led-user {
            label = "user-led";
            gpios = <&gpio1 2 GPIO_ACTIVE_HIGH>;
            linux,default-trigger = "heartbeat";
        };
    };
};

/* UART1 - Debug console */
&uart1 {
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_uart1>;
    status = "okay";
};

/* I2C1 */
&i2c1 {
    clock-frequency = <100000>;
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_i2c1>;
    status = "okay";

    codec: sgtl5000@0a {
        compatible = "fsl,sgtl5000";
        reg = <0x0a>;
        clocks = <&clks IMX6QDL_CLK_CKO>;
        VDDA-supply = <&reg_2p5v>;
        VDDIO-supply = <&reg_3p3v>;
    };
};

/* Ethernet */
&fec {
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_enet>;
    phy-mode = "rgmii";
    phy-handle = <&ethphy>;
    status = "okay";

    mdio {
        #address-cells = <1>;
        #size-cells = <0>;

        ethphy: ethernet-phy@1 {
            reg = <1>;
        };
    };
};

/* USB */
&usbotg {
    vbus-supply = <&reg_usb_otg_vbus>;
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_usbotg>;
    disable-over-current;
    status = "okay";
};

/* SD Card */
&usdhc2 {
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_usdhc2>;
    bus-width = <4>;
    cd-gpios = <&gpio2 2 GPIO_ACTIVE_LOW>;
    vmmc-supply = <&reg_3p3v>;
    status = "okay";
};
```

---

## Summary

In this part, we covered the fundamental concepts of Device Tree:

**Key Takeaways:**

1. **Device Tree Purpose**
    - Separates hardware description from driver code
    - Enables one kernel to support multiple boards
    - OS-agnostic hardware description language
2. **File Types**
    - `.dts`: Board-specific source files
    - `.dtsi`: SoC-level include files
    - `.dtb`: Compiled binary blobs
3. **Data Types**
    - Strings: `"text"`
    - Cells (u32): `<0x1234>`
    - Booleans: empty properties
    - Byte arrays: `[0x01 0x02]`
4. **Naming Conventions**
    - Nodes: `node-name@address`
    - Properties: lowercase with dashes
    - Labels: lowercase with underscores
5. **Key Concepts**
    - **Labels**: Tag nodes for reference
    - **Phandles**: Unique IDs for cross-references
    - **Aliases**: Quick lookup table
6. **Compilation**
    - `dtc`: Device Tree Compiler
    - `make dtbs`: Compile all DTs
    - Can decompile .dtb back to .dts