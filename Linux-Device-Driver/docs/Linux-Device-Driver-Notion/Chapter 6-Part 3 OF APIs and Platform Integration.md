# Part 3. OF APIs and Platform Integration

This part covers the Open Firmware (OF) APIs used to extract data from Device Tree, platform driver integration, and complete working examples.

---

## 6.3 Device Tree Data Extraction - OF APIs

### 6.3.1 OF API Overview

**What are OF APIs?**

**OF** stands for **Open Firmware** - the original standard that Device Tree came from. In Linux, all Device Tree manipulation functions are prefixed with `of_`.

**Required headers:**

```c
#include <linux/of.h>           /* Core OF APIs */
#include <linux/of_device.h>    /* OF device matching */
#include <linux/of_platform.h>  /* OF platform devices */
#include <linux/of_gpio.h>      /* OF GPIO helpers */
#include <linux/of_irq.h>       /* OF IRQ helpers */
```

**Enabling Device Tree support:**

```c
CONFIG_OF=y  /* Must be enabled in kernel config */
```

### 6.3.2 Core Data Structures

### **struct device_node**

This structure represents a **node** in the Device Tree:

```c
struct device_node {
    const char *name;             /* Node name (e.g., "serial") */
    const char *full_name;        /* Full path (e.g., "/soc/serial@02020000") */
    phandle phandle;              /* Unique handle for this node */

    struct property *properties;  /* List of properties */
    struct property *deadprops;   /* Removed properties */

    struct device_node *parent;   /* Parent node */
    struct device_node *child;    /* First child */
    struct device_node *sibling;  /* Next sibling */

    struct device_node *next;     /* Next device of same type */
    struct device_node *allnext;  /* Next in list of all nodes */

    struct kobject kobj;
    unsigned long _flags;
    void *data;
};
```

**Accessing device_node:**

```c
static int my_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;

    if (!np) {
        dev_err(&pdev->dev, "No device tree node\n");
        return -EINVAL;
    }

    pr_info("Node name: %s\n", np->name);
    pr_info("Full name: %s\n", np->full_name);

    return 0;
}
```

### **struct property**

This structure represents a **property** in a node:

```c
struct property {
    char *name;               /* Property name */
    int length;               /* Value length in bytes */
    void *value;              /* Property value */
    struct property *next;    /* Next property */

#if defined(CONFIG_OF_DYNAMIC)
    unsigned long _flags;
#endif
#if defined(CONFIG_OF_KOBJ)
    struct bin_attribute attr;
#endif
};
```

**Visual representation:**

```
┌─────────────────────────────────────────────────┐
│           Device Tree in Memory                 │
├─────────────────────────────────────────────────┤
│                                                 │
│  device_node: "serial@02020000"                 │
│  ├─ name = "serial"                             │
│  ├─ full_name = "/soc/serial@02020000"          │
│  ├─ phandle = 5                                 │
│  │                                              │
│  └─ properties:                                 │
│     ├─ property: "compatible"                   │
│     │  ├─ name = "compatible"                   │
│     │  ├─ length = 18                           │
│     │  └─ value = "fsl,imx6q-uart\0"            │
│     │                                           │
│     ├─ property: "reg"                          │
│     │  ├─ name = "reg"                          │
│     │  ├─ length = 8                            │
│     │  └─ value = {0x02020000, 0x4000}          │
│     │                                           │
│     └─ property: "status"                       │
│        ├─ name = "status"                       │
│        ├─ length = 5                            │
│        └─ value = "okay\0"                      │
└─────────────────────────────────────────────────┘
```

---

## 6.4 Property Reading APIs

### 6.4.1 Reading String Properties

**API:**

```c
int of_property_read_string(const struct device_node *np,
                            const char *propname,
                            const char **out_string);
```

**Parameters:**

- `np`: Pointer to device node
- `propname`: Property name
- `out_string`: Output pointer to string

**Returns:** 0 on success, negative error code on failure

**Example:**

```c
static int example_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    const char *model, *serial;
    int ret;

    /* Read single string */
    ret = of_property_read_string(np, "model", &model);
    if (ret) {
        dev_err(&pdev->dev, "Missing 'model' property\n");
        return ret;
    }

    dev_info(&pdev->dev, "Model: %s\n", model);

    /* Read optional string (with default) */
    ret = of_property_read_string(np, "serial-number", &serial);
    if (ret) {
        serial = "unknown";  /* Default value */
    }

    dev_info(&pdev->dev, "Serial: %s\n", serial);

    return 0;
}
```

**Reading string arrays:**

```c
int of_property_read_string_array(const struct device_node *np,
                                  const char *propname,
                                  const char **out_strs,
                                  size_t sz);
```

**Example:**

```c
/* DT: clock-names = "core", "bus", "ref"; */

const char *clock_names[3];
int count;

count = of_property_read_string_array(np, "clock-names",
                                      clock_names, 3);
if (count > 0) {
    for (int i = 0; i < count; i++) {
        dev_info(&pdev->dev, "Clock[%d]: %s\n", i, clock_names[i]);
    }
}
```

### 6.4.2 Reading Integer Properties (u32)

**API:**

```c
int of_property_read_u32(const struct device_node *np,
                        const char *propname,
                        u32 *out_value);
```

**Example:**

```c
static int example_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    u32 clock_frequency, timeout;
    int ret;

    /* Read mandatory u32 property */
    ret = of_property_read_u32(np, "clock-frequency", &clock_frequency);
    if (ret) {
        dev_err(&pdev->dev, "Missing clock-frequency property\n");
        return ret;
    }

    dev_info(&pdev->dev, "Clock frequency: %u Hz\n", clock_frequency);

    /* Read optional property with default */
    ret = of_property_read_u32(np, "timeout-ms", &timeout);
    if (ret) {
        timeout = 1000;  /* Default: 1 second */
        dev_info(&pdev->dev, "Using default timeout: %u ms\n", timeout);
    } else {
        dev_info(&pdev->dev, "Timeout: %u ms\n", timeout);
    }

    return 0;
}
```

### 6.4.3 Reading Integer Arrays

**API:**

```c
int of_property_read_u32_array(const struct device_node *np,
                               const char *propname,
                               u32 *out_values,
                               size_t sz);
```

**Parameters:**

- `sz`: Number of u32 values to read

**Example:**

```c
/* DT: operating-points = <1000000 1350000>,
 *                        <800000  1200000>;
 */

u32 op_points[4];
int ret;

ret = of_property_read_u32_array(np, "operating-points", op_points, 4);
if (ret) {
    dev_err(&pdev->dev, "Failed to read operating points\n");
    return ret;
}

dev_info(&pdev->dev, "OPP0: %u kHz @ %u uV\n",
         op_points[0], op_points[1]);
dev_info(&pdev->dev, "OPP1: %u kHz @ %u uV\n",
         op_points[2], op_points[3]);
```

**Reading specific index:**

```c
int of_property_read_u32_index(const struct device_node *np,
                               const char *propname,
                               u32 index,
                               u32 *out_value);
```

**Example:**

```c
/* DT: reg = <0x02020000 0x4000>; */

u32 base_addr, size;

of_property_read_u32_index(np, "reg", 0, &base_addr);  /* 0x02020000 */
of_property_read_u32_index(np, "reg", 1, &size);       /* 0x4000 */

dev_info(&pdev->dev, "Base: 0x%08x, Size: 0x%08x\n", base_addr, size);
```

### 6.4.4 Reading Boolean Properties

**API:**

```c
bool of_property_read_bool(const struct device_node *np,
                          const char *propname);
```

**Returns:** `true` if property exists, `false` otherwise

**Example:**

```c
static int example_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    bool dma_coherent, big_endian, spi_cs_high;

    /* Check boolean properties */
    dma_coherent = of_property_read_bool(np, "dma-coherent");
    big_endian = of_property_read_bool(np, "big-endian");
    spi_cs_high = of_property_read_bool(np, "spi-cs-high");

    dev_info(&pdev->dev, "DMA coherent: %s\n",
             dma_coherent ? "yes" : "no");
    dev_info(&pdev->dev, "Big endian: %s\n",
             big_endian ? "yes" : "no");
    dev_info(&pdev->dev, "SPI CS high: %s\n",
             spi_cs_high ? "yes" : "no");

    /* Configure hardware accordingly */
    if (dma_coherent)
        enable_coherent_dma();

    if (big_endian)
        set_big_endian_mode();

    return 0;
}
```

### 6.4.5 Reading Other Integer Types

**u8 arrays:**

```c
int of_property_read_u8_array(const struct device_node *np,
                              const char *propname,
                              u8 *out_values,
                              size_t sz);
```

**u16 arrays:**

```c
int of_property_read_u16_array(const struct device_node *np,
                               const char *propname,
                               u16 *out_values,
                               size_t sz);
```

**u64 values:**

```c
int of_property_read_u64(const struct device_node *np,
                        const char *propname,
                        u64 *out_value);
```

**Example with MAC address:**

```c
/* DT: local-mac-address = [00 1A 2B 3C 4D 5E]; */

u8 mac_addr[6];
int ret;

ret = of_property_read_u8_array(np, "local-mac-address", mac_addr, 6);
if (!ret) {
    dev_info(&pdev->dev, "MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
             mac_addr[0], mac_addr[1], mac_addr[2],
             mac_addr[3], mac_addr[4], mac_addr[5]);
}
```

### 6.4.6 Complete Property Reading Example

```c
static int comprehensive_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    const char *name, *mode_str;
    u32 freq, speed, dimensions[2];
    u8 mac[6];
    bool enabled, dma_enabled;
    int ret;

    /* String property */
    ret = of_property_read_string(np, "device-name", &name);
    if (!ret)
        dev_info(&pdev->dev, "Name: %s\n", name);

    /* String property with default */
    ret = of_property_read_string(np, "mode", &mode_str);
    mode_str = ret ? "default" : mode_str;

    /* u32 property */
    ret = of_property_read_u32(np, "clock-frequency", &freq);
    if (ret) {
        dev_err(&pdev->dev, "Missing clock-frequency\n");
        return ret;
    }

    /* Optional u32 with default */
    if (of_property_read_u32(np, "speed", &speed))
        speed = 100000;  /* Default */

    /* u32 array */
    ret = of_property_read_u32_array(np, "dimensions", dimensions, 2);
    if (!ret) {
        dev_info(&pdev->dev, "Size: %ux%u\n",
                 dimensions[0], dimensions[1]);
    }

    /* u8 array (MAC address) */
    ret = of_property_read_u8_array(np, "mac-address", mac, 6);
    if (!ret) {
        dev_info(&pdev->dev, "MAC: %pM\n", mac);
    }

    /* Boolean properties */
    enabled = of_property_read_bool(np, "enable");
    dma_enabled = of_property_read_bool(np, "dma-enable");

    dev_info(&pdev->dev, "Enabled: %s, DMA: %s\n",
             enabled ? "yes" : "no",
             dma_enabled ? "yes" : "no");

    return 0;
}
```

---

## 6.5 Sub-Node Handling and Iteration

### 6.5.1 Iterating Over Child Nodes

**API:**

```c
#define for_each_child_of_node(parent, child)
```

**Example - Flash partitions:**

```
flash@0 {
    compatible = "jedec,spi-nor";
    reg = <0>;

    partition@0 {
        label = "bootloader";
        reg = <0x000000 0x100000>;  /* 1 MB */
        read-only;
    };

    partition@100000 {
        label = "kernel";
        reg = <0x100000 0x400000>;  /* 4 MB */
    };

    partition@500000 {
        label = "rootfs";
        reg = <0x500000 0xb00000>;  /* 11 MB */
    };
};
```

**Driver code:**

```c
static int flash_probe(struct spi_device *spi)
{
    struct device_node *np = spi->dev.of_node;
    struct device_node *child;

    /* Iterate over child nodes (partitions) */
    for_each_child_of_node(np, child) {
        const char *label;
        u32 offset, size;
        bool read_only;
        int ret;

        /* Read partition label */
        ret = of_property_read_string(child, "label", &label);
        if (ret) {
            dev_err(&spi->dev, "Partition missing label\n");
            continue;
        }

        /* Read partition offset and size */
        ret = of_property_read_u32_index(child, "reg", 0, &offset);
        if (ret)
            continue;

        ret = of_property_read_u32_index(child, "reg", 1, &size);
        if (ret)
            continue;

        /* Check if read-only */
        read_only = of_property_read_bool(child, "read-only");

        dev_info(&spi->dev, "Partition '%s': offset=0x%08x, size=0x%08x, %s\n",
                 label, offset, size,
                 read_only ? "ro" : "rw");

        /* Create MTD partition */
        add_mtd_partition(label, offset, size, read_only);
    }

    return 0;
}
```

### 6.5.2 Other Child Node APIs

**Get first child:**

```c
struct device_node *of_get_next_child(const struct device_node *node,
                                      struct device_node *prev);
```

**Get child by name:**

```c
struct device_node *of_get_child_by_name(const struct device_node *node,
                                         const char *name);
```

**Count children:**

```c
int of_get_child_count(const struct device_node *np);
```

**Example:**

```c
struct device_node *np = pdev->dev.of_node;
struct device_node *child;
int child_count;

/* Count children */
child_count = of_get_child_count(np);
dev_info(&pdev->dev, "Device has %d children\n", child_count);

/* Get child by name */
child = of_get_child_by_name(np, "partition0");
if (child) {
    /* Process partition0 */
    of_node_put(child);  /* Release reference */
}

/* Iterate manually */
child = NULL;
while ((child = of_get_next_child(np, child))) {
    dev_info(&pdev->dev, "Child: %s\n", child->name);
    /* Don't forget of_node_put() when done */
}
```

**Important:** Always call `of_node_put()` when done with a device_node to release the reference!

### 6.5.3 Real-World Example: EEPROM with Partitions

```
eeprom: ee24lc512@55 {
    compatible = "microchip,24xx512";
    reg = <0x55>;
    #address-cells = <1>;
    #size-cells = <1>;

    partition1 {
        label = "private";
        reg = <0 1024>;
        read-only;
    };

    partition2 {
        label = "data";
        reg = <1024 64512>;
    };
};
```

**Complete driver:**

```c
struct eeprom_partition {
    const char *label;
    u32 offset;
    u32 size;
    bool read_only;
};

static int eeprom_probe(struct i2c_client *client,
                       const struct i2c_device_id *id)
{
    struct device_node *np = client->dev.of_node;
    struct device_node *child;
    struct eeprom_partition *partitions;
    int num_partitions, i = 0;

    /* Count partitions */
    num_partitions = of_get_child_count(np);
    if (num_partitions == 0) {
        dev_info(&client->dev, "No partitions defined\n");
        return 0;
    }

    /* Allocate partition array */
    partitions = devm_kcalloc(&client->dev, num_partitions,
                             sizeof(*partitions), GFP_KERNEL);
    if (!partitions)
        return -ENOMEM;

    /* Parse each partition */
    for_each_child_of_node(np, child) {
        of_property_read_string(child, "label", &partitions[i].label);
        of_property_read_u32_index(child, "reg", 0, &partitions[i].offset);
        of_property_read_u32_index(child, "reg", 1, &partitions[i].size);
        partitions[i].read_only = of_property_read_bool(child, "read-only");

        dev_info(&client->dev, "Partition '%s': [0x%04x-0x%04x] %s\n",
                 partitions[i].label,
                 partitions[i].offset,
                 partitions[i].offset + partitions[i].size - 1,
                 partitions[i].read_only ? "ro" : "rw");

        i++;
    }

    return 0;
}
```

---

## 6.6 Phandle Parsing APIs

### 6.6.1 Simple Phandle Parsing

**API:**

```c
struct device_node *of_parse_phandle(const struct device_node *np,
                                     const char *phandle_name,
                                     int index);
```

**Example:**

```
device {
    parent-controller = <&intc>;
    uart-ref = <&uart1>;
};
```

**Driver:**

```c
struct device_node *intc_node, *uart_node;

/* Get interrupt controller node */
intc_node = of_parse_phandle(np, "parent-controller", 0);
if (intc_node) {
    pr_info("Interrupt controller: %s\n", intc_node->full_name);
    of_node_put(intc_node);
}

/* Get UART node */
uart_node = of_parse_phandle(np, "uart-ref", 0);
if (uart_node) {
    pr_info("UART reference: %s\n", uart_node->full_name);
    of_node_put(uart_node);
}
```

### 6.6.2 Phandle with Arguments

**The powerful API:**

```c
int of_parse_phandle_with_args(const struct device_node *np,
                               const char *list_name,
                               const char *cells_name,
                               int index,
                               struct of_phandle_args *out_args);
```

**Parameters:**

- `np`: Consumer device node
- `list_name`: Property containing phandle list (e.g., "clocks")
- `cells_name`: Property specifying cell count (e.g., "#clock-cells")
- `index`: Which phandle in the list (0-based)
- `out_args`: Output structure

**struct of_phandle_args:**

```c
#define MAX_PHANDLE_ARGS 16

struct of_phandle_args {
    struct device_node *np;           /* Provider node */
    int args_count;                   /* Number of arguments */
    uint32_t args[MAX_PHANDLE_ARGS];  /* Arguments */
};
```

**Example - Clock specifier:**

```
clk: clock-controller@020c4000 {
    compatible = "fsl,imx6q-ccm";
    reg = <0x020c4000 0x4000>;
    #clock-cells = <1>;
};

device {
    clocks = <&clk 10>, <&clk 20>;
    clock-names = "core", "bus";
};
```

**Parsing in driver:**

```c
struct of_phandle_args clkspec;
int ret;

/* Parse first clock (index 0) */
ret = of_parse_phandle_with_args(np, "clocks", "#clock-cells", 0, &clkspec);
if (ret == 0) {
    dev_info(&pdev->dev, "Clock provider: %s\n", clkspec.np->full_name);
    dev_info(&pdev->dev, "Clock ID: %u\n", clkspec.args[0]);  /* 10 */
    dev_info(&pdev->dev, "Args count: %d\n", clkspec.args_count);  /* 1 */
    of_node_put(clkspec.np);
}

/* Parse second clock (index 1) */
ret = of_parse_phandle_with_args(np, "clocks", "#clock-cells", 1, &clkspec);
if (ret == 0) {
    dev_info(&pdev->dev, "Clock ID: %u\n", clkspec.args[0]);  /* 20 */
    of_node_put(clkspec.np);
}
```

### 6.6.3 Complete Phandle Example

**Device Tree:**

```
gpio1: gpio@0209c000 {
    compatible = "fsl,imx6q-gpio";
    reg = <0x0209c000 0x4000>;
    gpio-controller;
    #gpio-cells = <2>;
};

clk: clock-controller@020c4000 {
    compatible = "fsl,imx6q-ccm";
    reg = <0x020c4000 0x4000>;
    #clock-cells = <1>;
};

mydevice {
    compatible = "vendor,mydevice";

    reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
    enable-gpios = <&gpio1 8 GPIO_ACTIVE_HIGH>;

    clocks = <&clk 15>, <&clk 16>;
    clock-names = "ipg", "per";
};
```

**Complete parsing code:**

```c
static int mydevice_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    struct of_phandle_args gpiospec, clkspec;
    int ret;

    /* Parse reset GPIO */
    ret = of_parse_phandle_with_args(np, "reset-gpios", "#gpio-cells",
                                     0, &gpiospec);
    if (ret == 0) {
        dev_info(&pdev->dev, "Reset GPIO controller: %s\n",
                 gpiospec.np->name);
        dev_info(&pdev->dev, "Reset GPIO pin: %u\n",
                 gpiospec.args[0]);  /* 7 */
        dev_info(&pdev->dev, "Reset GPIO flags: 0x%x\n",
                 gpiospec.args[1]);  /* GPIO_ACTIVE_LOW */
        of_node_put(gpiospec.np);
    }

    /* Parse enable GPIO */
    ret = of_parse_phandle_with_args(np, "enable-gpios", "#gpio-cells",
                                     0, &gpiospec);
    if (ret == 0) {
        dev_info(&pdev->dev, "Enable GPIO pin: %u, flags: 0x%x\n",
                 gpiospec.args[0], gpiospec.args[1]);
        of_node_put(gpiospec.np);
    }

    /* Parse clocks */
    for (int i = 0; i < 2; i++) {
        ret = of_parse_phandle_with_args(np, "clocks", "#clock-cells",
                                         i, &clkspec);
        if (ret == 0) {
            dev_info(&pdev->dev, "Clock[%d] ID: %u\n",
                     i, clkspec.args[0]);
            of_node_put(clkspec.np);
        }
    }

    return 0;
}
```

---

## 6.7 GPIO OF APIs

### 6.7.1 of_get_named_gpio()

**API:**

```c
int of_get_named_gpio(struct device_node *np,
                     const char *propname,
                     int index);
```

**Returns:** GPIO number on success, negative error code on failure

**Example:**

```
device {
    reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
    led-gpios = <&gpio2 15 GPIO_ACTIVE_HIGH>,
                <&gpio2 16 GPIO_ACTIVE_HIGH>;
};
```

**Driver:**

```c
static int device_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    int reset_gpio, led_red, led_green;
    int ret;

    /* Get reset GPIO */
    reset_gpio = of_get_named_gpio(np, "reset-gpios", 0);
    if (!gpio_is_valid(reset_gpio)) {
        dev_err(&pdev->dev, "Invalid reset GPIO\n");
        return -EINVAL;
    }

    /* Request and configure */
    ret = devm_gpio_request_one(&pdev->dev, reset_gpio,
                               GPIOF_OUT_INIT_LOW, "reset");
    if (ret)
        return ret;

    /* Get LED GPIOs */
    led_red = of_get_named_gpio(np, "led-gpios", 0);
    led_green = of_get_named_gpio(np, "led-gpios", 1);

    if (gpio_is_valid(led_red)) {
        devm_gpio_request_one(&pdev->dev, led_red,
                             GPIOF_OUT_INIT_LOW, "led-red");
    }

    if (gpio_is_valid(led_green)) {
        devm_gpio_request_one(&pdev->dev, led_green,
                             GPIOF_OUT_INIT_LOW, "led-green");
    }

    /* Toggle reset */
    gpio_set_value(reset_gpio, 1);
    msleep(10);
    gpio_set_value(reset_gpio, 0);

    /* Blink LEDs */
    gpio_set_value(led_red, 1);
    msleep(500);
    gpio_set_value(led_red, 0);
    gpio_set_value(led_green, 1);

    return 0;
}
```

### 6.7.2 of_gpio_named_count()

**Count GPIO entries:**

```c
int of_gpio_named_count(struct device_node *np, const char *propname);
```

**Example:**

```c
int num_leds = of_gpio_named_count(np, "led-gpios");

for (int i = 0; i < num_leds; i++) {
    int gpio = of_get_named_gpio(np, "led-gpios", i);
    if (gpio_is_valid(gpio)) {
        devm_gpio_request_one(&pdev->dev, gpio,
                             GPIOF_OUT_INIT_LOW, "led");
    }
}
```

---

## 6.8 Platform Driver Integration with Device Tree

### 6.8.1 struct of_device_id

**The matching structure:**

```c
struct of_device_id {
    char name[32];              /* Deprecated, don't use */
    char type[32];              /* Deprecated, don't use */
    char compatible[128];       /* Match against this string */
    const void *data;           /* Per-device specific data */
};
```

**Only two fields matter:**

- `compatible`: String to match
- `data`: Optional driver-specific data

### 6.8.2 Declaring OF Match Table

**Example:**

```c
static const struct of_device_id my_dt_ids[] = {
    { .compatible = "vendor,device-v1", },
    { .compatible = "vendor,device-v2", },
    { .compatible = "vendor,device-v3", },
    { /* Sentinel - must be last */ }
};
MODULE_DEVICE_TABLE(of, my_dt_ids);
```

**Important:**

- Array must end with empty entry (sentinel)
- `MODULE_DEVICE_TABLE(of, ...)` registers with device matching system

### 6.8.3 Platform Driver with OF Match

**Complete platform driver:**

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>

/* OF match table */
static const struct of_device_id mydriver_dt_ids[] = {
    { .compatible = "vendor,mydevice", },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mydriver_dt_ids);

/* Probe function */
static int mydriver_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;

    if (!np) {
        dev_err(&pdev->dev, "No device tree node\n");
        return -EINVAL;
    }

    dev_info(&pdev->dev, "Probed via device tree\n");
    dev_info(&pdev->dev, "Node: %s\n", np->full_name);

    /* Extract properties and configure device */

    return 0;
}

/* Remove function */
static int mydriver_remove(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "Device removed\n");
    return 0;
}

/* Platform driver structure */
static struct platform_driver mydriver = {
    .probe = mydriver_probe,
    .remove = mydriver_remove,
    .driver = {
        .name = "mydriver",
        .of_match_table = mydriver_dt_ids,  /* OF matching */
    },
};

module_platform_driver(mydriver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Example Device Tree Driver");
```

### 6.8.4 of_match_device()

**Get matching entry:**

```c
const struct of_device_id *of_match_device(
    const struct of_device_id *matches,
    const struct device *dev);
```

**Example:**

```c
static int mydriver_probe(struct platform_device *pdev)
{
    const struct of_device_id *match;

    /* Get which match entry was used */
    match = of_match_device(mydriver_dt_ids, &pdev->dev);
    if (match) {
        dev_info(&pdev->dev, "Matched: %s\n", match->compatible);

        /* Access per-device data if any */
        if (match->data) {
            void *driver_data = (void *)match->data;
            /* Use driver_data */
        }
    } else {
        dev_info(&pdev->dev, "Not matched via device tree\n");
    }

    return 0;
}
```

---

## 6.9 Multi-Hardware Support with Per-Device Data

### 6.9.1 The Problem

Sometimes one driver needs to support multiple hardware variants with different characteristics.

**Example variants:**

- v1: 100 kHz max, 16-byte FIFO, no DMA
- v2: 400 kHz max, 64-byte FIFO, DMA support
- v3: 1 MHz max, 128-byte FIFO, DMA + encryption

### 6.9.2 The Solution: Per-Device Data

**Step 1: Define device-specific data structure:**

```c
struct chip_data {
    unsigned int max_speed;
    unsigned int fifo_size;
    bool has_dma;
    bool has_encryption;
};
```

**Step 2: Create data for each variant:**

```c
static const struct chip_data v1_data = {
    .max_speed = 100000,
    .fifo_size = 16,
    .has_dma = false,
    .has_encryption = false,
};

static const struct chip_data v2_data = {
    .max_speed = 400000,
    .fifo_size = 64,
    .has_dma = true,
    .has_encryption = false,
};

static const struct chip_data v3_data = {
    .max_speed = 1000000,
    .fifo_size = 128,
    .has_dma = true,
    .has_encryption = true,
};
```

**Step 3: Associate data with compatible strings:**

```c
static const struct of_device_id mydriver_dt_ids[] = {
    {
        .compatible = "vendor,chip-v1",
        .data = &v1_data,
    },
    {
        .compatible = "vendor,chip-v2",
        .data = &v2_data,
    },
    {
        .compatible = "vendor,chip-v3",
        .data = &v3_data,
    },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mydriver_dt_ids);
```

**Step 4: Extract data in probe:**

```c
static int mydriver_probe(struct platform_device *pdev)
{
    const struct of_device_id *match;
    const struct chip_data *data;

    /* Get matching entry */
    match = of_match_device(mydriver_dt_ids, &pdev->dev);
    if (!match) {
        dev_err(&pdev->dev, "No device tree match\n");
        return -ENODEV;
    }

    /* Extract device-specific data */
    data = match->data;

    /* Configure based on device variant */
    dev_info(&pdev->dev, "Chip: %s\n", match->compatible);
    dev_info(&pdev->dev, "Max speed: %u Hz\n", data->max_speed);
    dev_info(&pdev->dev, "FIFO size: %u bytes\n", data->fifo_size);
    dev_info(&pdev->dev, "DMA support: %s\n",
             data->has_dma ? "yes" : "no");
    dev_info(&pdev->dev, "Encryption: %s\n",
             data->has_encryption ? "yes" : "no");

    /* Configure hardware accordingly */
    configure_max_speed(data->max_speed);
    configure_fifo_size(data->fifo_size);

    if (data->has_dma)
        enable_dma_support();

    if (data->has_encryption)
        enable_encryption();

    return 0;
}
```

### 6.9.3 Real-World Example: i.MX UART Driver

**From drivers/tty/serial/imx.c:**

```c
/* Device type enumeration */
enum imx_uart_type {
    IMX1_UART,
    IMX21_UART,
    IMX6Q_UART,
};

/* Device-specific data */
struct imx_uart_data {
    unsigned uts_reg;
    enum imx_uart_type devtype;
};

/* Data for each variant */
static struct imx_uart_data imx_uart_devdata[] = {
    [IMX1_UART] = {
        .uts_reg = IMX1_UTS,
        .devtype = IMX1_UART,
    },
    [IMX21_UART] = {
        .uts_reg = IMX21_UTS,
        .devtype = IMX21_UART,
    },
    [IMX6Q_UART] = {
        .uts_reg = IMX21_UTS,
        .devtype = IMX6Q_UART,
    },
};

/* OF match table with data */
static const struct of_device_id imx_uart_dt_ids[] = {
    { .compatible = "fsl,imx6q-uart", .data = &imx_uart_devdata[IMX6Q_UART], },
    { .compatible = "fsl,imx1-uart",  .data = &imx_uart_devdata[IMX1_UART], },
    { .compatible = "fsl,imx21-uart", .data = &imx_uart_devdata[IMX21_UART], },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx_uart_dt_ids);

/* Probe function */
static int imx_probe_dt(struct platform_device *pdev)
{
    const struct of_device_id *of_id;
    const struct imx_uart_data *devdata;

    of_id = of_match_device(imx_uart_dt_ids, &pdev->dev);
    if (!of_id)
        return -ENODEV;

    devdata = of_id->data;

    /* Use device-specific configuration */
    sport->devdata = devdata;

    /* Configure based on UART type */
    switch (devdata->devtype) {
    case IMX1_UART:
        /* IMX1-specific setup */
        break;
    case IMX21_UART:
        /* IMX21-specific setup */
        break;
    case IMX6Q_UART:
        /* IMX6Q-specific setup */
        break;
    }

    return 0;
}
```

---

## 6.10 Backward Compatibility - Non-DT Platforms

### 6.10.1 The of_match_ptr() Macro

**Definition:**

```c
#ifdef CONFIG_OF
#define of_match_ptr(_ptr)  (_ptr)
#else
#define of_match_ptr(_ptr)  NULL
#endif
```

**Usage:**

```c
static struct platform_driver mydriver = {
    .driver = {
        .name = "mydriver",
        .of_match_table = of_match_ptr(mydriver_dt_ids),
        /*                ^^^^^^^^^^^^
         *                Returns mydriver_dt_ids if CONFIG_OF=y
         *                Returns NULL if CONFIG_OF=n
         */
    },
};
```

**Why use it?**

- Avoids compiler warnings when DT disabled
- Keeps code cleaner than #ifdef

### 6.10.2 Supporting Both DT and Platform Data

**Driver that works with or without DT:**

```c
struct my_platform_data {
    unsigned int speed;
    unsigned int mode;
    bool enable_dma;
};

static int mydriver_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    struct my_platform_data *pdata;
    unsigned int speed, mode;
    bool dma;

    if (np) {
        /* Device Tree path */
        dev_info(&pdev->dev, "Probed via Device Tree\n");

        /* Allocate pdata */
        pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
        if (!pdata)
            return -ENOMEM;

        /* Extract from DT */
        of_property_read_u32(np, "speed", &pdata->speed);
        of_property_read_u32(np, "mode", &pdata->mode);
        pdata->enable_dma = of_property_read_bool(np, "enable-dma");

    } else {
        /* Platform data path (legacy) */
        dev_info(&pdev->dev, "Probed via platform data\n");

        pdata = dev_get_platdata(&pdev->dev);
        if (!pdata) {
            dev_err(&pdev->dev, "No platform data\n");
            return -EINVAL;
        }
    }

    /* Use pdata regardless of source */
    dev_info(&pdev->dev, "Speed: %u\n", pdata->speed);
    dev_info(&pdev->dev, "Mode: %u\n", pdata->mode);
    dev_info(&pdev->dev, "DMA: %s\n", pdata->enable_dma ? "yes" : "no");

    return 0;
}
```

---

## 6.11 Complete Real-World Driver Example

**Device Tree node:**

```
mydevice: mydev@80000000 {
    compatible = "company,mydevice-v2";
    reg = <0x80000000 0x1000>;
    interrupts = <0 56 IRQ_TYPE_LEVEL_HIGH>;

    clocks = <&clk IMX6QDL_CLK_IPG>;
    clock-names = "ipg";

    reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;

    /* Custom properties */
    mode = <2>;
    speed = <100000>;
    enable-dma;
};
```

**Complete driver implementation:**

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/io.h>

struct mydev_data {
    void __iomem *base;
    int irq;
    struct clk *clk;
    int reset_gpio;
    u32 mode;
    u32 speed;
    bool dma_enabled;
};

/* Interrupt handler */
static irqreturn_t mydev_irq_handler(int irq, void *dev_id)
{
    struct mydev_data *data = dev_id;
    u32 status;

    /* Read interrupt status */
    status = readl(data->base + STATUS_REG);

    /* Handle interrupt */
    pr_info("Interrupt triggered, status=0x%08x\n", status);

    /* Clear interrupt */
    writel(status, data->base + STATUS_REG);

    return IRQ_HANDLED;
}

/* OF match table */
static const struct of_device_id mydev_dt_ids[] = {
    { .compatible = "company,mydevice-v1", },
    { .compatible = "company,mydevice-v2", },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mydev_dt_ids);

/* Probe function */
static int mydev_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct device_node *np = dev->of_node;
    struct mydev_data *data;
    struct resource *res;
    int ret;

    dev_info(dev, "Probing device\n");

    /* Check for device tree node */
    if (!np) {
        dev_err(dev, "No device tree node\n");
        return -ENODEV;
    }

    /* Allocate private data */
    data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    /* Get and map memory resource */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    data->base = devm_ioremap_resource(dev, res);
    if (IS_ERR(data->base))
        return PTR_ERR(data->base);

    dev_info(dev, "Mapped registers at %pR\n", res);

    /* Get IRQ */
    data->irq = platform_get_irq(pdev, 0);
    if (data->irq < 0) {
        dev_err(dev, "Failed to get IRQ\n");
        return data->irq;
    }

    /* Request IRQ */
    ret = devm_request_irq(dev, data->irq, mydev_irq_handler,
                          0, dev_name(dev), data);
    if (ret) {
        dev_err(dev, "Failed to request IRQ %d\n", data->irq);
        return ret;
    }

    dev_info(dev, "IRQ %d registered\n", data->irq);

    /* Get clock */
    data->clk = devm_clk_get(dev, "ipg");
    if (IS_ERR(data->clk)) {
        dev_err(dev, "Failed to get clock\n");
        return PTR_ERR(data->clk);
    }

    /* Enable clock */
    ret = clk_prepare_enable(data->clk);
    if (ret) {
        dev_err(dev, "Failed to enable clock\n");
        return ret;
    }

    dev_info(dev, "Clock enabled, rate: %lu Hz\n",
             clk_get_rate(data->clk));

    /* Get reset GPIO */
    data->reset_gpio = of_get_named_gpio(np, "reset-gpios", 0);
    if (gpio_is_valid(data->reset_gpio)) {
        ret = devm_gpio_request_one(dev, data->reset_gpio,
                                   GPIOF_OUT_INIT_LOW, "reset");
        if (ret) {
            dev_err(dev, "Failed to request reset GPIO\n");
            goto err_clk;
        }

        /* Assert reset */
        gpio_set_value(data->reset_gpio, 1);
        msleep(10);
        gpio_set_value(data->reset_gpio, 0);

        dev_info(dev, "Reset GPIO %d configured\n", data->reset_gpio);
    }

    /* Read custom properties */
    ret = of_property_read_u32(np, "mode", &data->mode);
    if (ret) {
        dev_warn(dev, "Missing 'mode' property, using default\n");
        data->mode = 0;
    }

    ret = of_property_read_u32(np, "speed", &data->speed);
    if (ret) {
        dev_warn(dev, "Missing 'speed' property, using default\n");
        data->speed = 50000;
    }

    data->dma_enabled = of_property_read_bool(np, "enable-dma");

    dev_info(dev, "Configuration:\n");
    dev_info(dev, "  Mode: %u\n", data->mode);
    dev_info(dev, "  Speed: %u Hz\n", data->speed);
    dev_info(dev, "  DMA: %s\n", data->dma_enabled ? "enabled" : "disabled");

    /* Configure hardware */
    writel(data->mode, data->base + MODE_REG);
    writel(data->speed, data->base + SPEED_REG);

    if (data->dma_enabled)
        writel(DMA_ENABLE, data->base + DMA_CTRL_REG);

    /* Save private data */
    platform_set_drvdata(pdev, data);

    dev_info(dev, "Probe successful\n");
    return 0;

err_clk:
    clk_disable_unprepare(data->clk);
    return ret;
}

/* Remove function */
static int mydev_remove(struct platform_device *pdev)
{
    struct mydev_data *data = platform_get_drvdata(pdev);

    dev_info(&pdev->dev, "Removing device\n");

    /* Disable hardware */
    writel(0, data->base + CTRL_REG);

    /* Disable clock */
    clk_disable_unprepare(data->clk);

    dev_info(&pdev->dev, "Device removed\n");
    return 0;
}

/* Platform driver */
static struct platform_driver mydev_driver = {
    .probe = mydev_probe,
    .remove = mydev_remove,
    .driver = {
        .name = "mydevice",
        .of_match_table = mydev_dt_ids,
    },
};

module_platform_driver(mydev_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Expert");
MODULE_DESCRIPTION("Complete Device Tree Example Driver");
```

---

## 6.12 Best Practices and Common Pitfalls

### 6.12.1 DO's ✅

**1. Always check for device tree node:**

```c
struct device_node *np = pdev->dev.of_node;
if (!np) {
    dev_err(&pdev->dev, "No device tree node\n");
    return -EINVAL;
}
```

*2. Use devm_ functions:**

```c
/* Preferred */
base = devm_ioremap_resource(&pdev->dev, res);
clk = devm_clk_get(&pdev->dev, "ipg");
ret = devm_request_irq(&pdev->dev, irq, handler, 0, name, data);

/* Automatic cleanup on error or removal */
```

**3. Provide default values for optional properties:**

```c
u32 timeout;

if (of_property_read_u32(np, "timeout-ms", &timeout))
    timeout = 1000;  /* Default: 1 second */
```

**4. Use named resources:**

```c
/* More readable */
res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "control");
irq = platform_get_irq_byname(pdev, "tx");
```

**5. Call of_node_put():**

```c
struct device_node *child;

child = of_parse_phandle(np, "parent", 0);
if (child) {
    /* Use child */
    of_node_put(child);  /* Release reference */
}
```

### 6.12.2 DON'Ts ❌

**1. Don't assume DT is always present:**

```c
/* BAD */
of_property_read_u32(np, "speed", &speed);

/* GOOD */
if (of_property_read_u32(np, "speed", &speed))
    speed = default_speed;
```

**2. Don't forget error checking:**

```c
/* BAD */
of_property_read_string(np, "name", &name);
pr_info("Name: %s\n", name);

/* GOOD */
if (of_property_read_string(np, "name", &name) == 0)
    pr_info("Name: %s\n", name);
else
    pr_warn("Name not specified\n");
```

**3. Don't hardcode values - use properties:**

```c
/* BAD */
#define MAX_SPEED  100000
speed = MAX_SPEED;

/* GOOD */
of_property_read_u32(np, "max-speed", &speed);
```

**4. Don't forget MODULE_DEVICE_TABLE:**

```c
static const struct of_device_id my_dt_ids[] = {
    { .compatible = "vendor,device", },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, my_dt_ids);  /* DON'T FORGET THIS! */
```

**5. Don't mix DT and non-DT paths carelessly:**

```c
/* BAD - confusing */
if (np)
    speed = get_dt_speed(np);
else
    speed = pdata->speed;
configure_speed(speed);

/* GOOD - clear separation */
if (np) {
    /* Full DT path */
    return probe_with_dt(pdev);
} else {
    /* Full platform data path */
    return probe_with_pdata(pdev);
}
```

### 6.12.3 Common Pitfalls

**Pitfall 1: Forgetting sentinel in match table**

```c
/* WRONG - will crash */
static const struct of_device_id my_ids[] = {
    { .compatible = "vendor,device", },
};

/* CORRECT */
static const struct of_device_id my_ids[] = {
    { .compatible = "vendor,device", },
    { /* sentinel */ }  /* Empty entry required! */
};
```

**Pitfall 2: Not releasing node references**

```c
/* Memory leak */
for_each_child_of_node(parent, child) {
    /* Process child */
    if (error)
        return -EINVAL;  /* WRONG - leaks child reference */
}

/* Correct */
for_each_child_of_node(parent, child) {
    /* Process child */
    if (error) {
        of_node_put(child);  /* Release reference */
        return -EINVAL;
    }
}
of_node_put(child);  /* Also release after loop */
```

**Pitfall 3: Wrong property names**

```c
/* DT uses "reset-gpios" */
reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;

/* Driver looks for wrong name */
gpio = of_get_named_gpio(np, "reset-gpio", 0);  /* WRONG - missing 's' */

/* Correct */
gpio = of_get_named_gpio(np, "reset-gpios", 0);  /* Match DT exactly */
```

---

## Summary

In this final part, we covered:

**OF APIs:**

- Property reading: strings, integers, booleans, arrays
- Sub-node iteration: `for_each_child_of_node()`
- Phandle parsing: `of_parse_phandle_with_args()`
- GPIO helpers: `of_get_named_gpio()`

**Platform Integration:**

- `struct of_device_id` for matching
- `MODULE_DEVICE_TABLE(of, ...)`
- Per-device data for multi-hardware support
- Backward compatibility with `of_match_ptr()`

**Best Practices:**

- Always check for DT node
- Use `devm_*` functions
- Provide defaults for optional properties
- Release node references
- Clear error handling

**Complete Example:**

- Full working driver with all features
- Memory mapping, IRQ, clocks, GPIOs
- Custom property extraction
- Production-ready code

You now have complete knowledge of Device Tree and can write professional Linux device drivers that properly integrate with Device Tree!