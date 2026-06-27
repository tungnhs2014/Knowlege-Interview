# Part 3. Device Tree Integration

This part covers how to integrate I2C drivers with Device Tree, including DT node declaration, OF-style matching, and complete working examples.

---

## 7.1 I2C Devices in Device Tree

### 7.1.1 Device Tree Overview for I2C

**Key concepts:**

```
┌────────────────────────────────────────────────┐
│  I2C Device Tree Fundamentals                  │
├────────────────────────────────────────────────┤
│                                                │
│  1. I2C devices = Non-memory mapped devices    │
│     - Cannot be accessed directly by CPU       │
│     - Need I2C controller as intermediary      │
│                                                │
│  2. I2C devices are CHILDREN of bus node       │
│     - Parent: I2C controller node              │
│     - Children: I2C slave devices              │
│                                                │
│  3. Address translation                        │
│     - #address-cells = <1>                     │
│     - #size-cells = <0>                        │
│     - reg = I2C slave address only             │
│                                                │
│  4. Matching                                   │
│     - compatible property                      │
│     - Driver's of_match_table                  │
│                                                │
└────────────────────────────────────────────────┘
```

### 7.1.2 I2C Device Tree Hierarchy

**Visual representation:**

```
Device Tree Structure
═════════════════════

/ {
    soc {
        i2c@021a0000 {              ← I2C Controller Node
            #address-cells = <1>;   ← Addresses are 1 cell
            #size-cells = <0>;      ← No size component

            eeprom@50 {             ← I2C Device #1
                compatible = "atmel,24c512";
                reg = <0x50>;       ← I2C address
            };

            rtc@68 {                ← I2C Device #2
                compatible = "nxp,pcf8523";
                reg = <0x68>;       ← I2C address
            };
        };
    };
};

Hardware Topology
═════════════════

SoC I2C Controller @0x021a0000
    │
    ├─ EEPROM (I2C addr 0x50)
    └─ RTC    (I2C addr 0x68)
```

### 7.1.3 Understanding #address-cells and #size-cells

**For I2C devices:**

```c
&i2c1 {
    #address-cells = <1>;  /* Address = 1 cell (32-bit) */
    #size-cells = <0>;     /* No size component */

    device@50 {
        reg = <0x50>;      /* Just address, no size */
    };
};
```

**Why #size-cells = 0?**

```
┌────────────────────────────────────────────────┐
│  Memory-mapped vs Non-memory-mapped            │
├────────────────────────────────────────────────┤
│                                                │
│  Memory-mapped (UART, GPIO):                   │
│    reg = <0x02020000 0x4000>;                  │
│           └─ base    └─ size                   │
│    #size-cells = <1>  (size is relevant)       │
│                                                │
│  I2C/SPI devices:                              │
│    reg = <0x50>;                               │
│           └─ I2C address                       │
│    #size-cells = <0>  (no size concept)        │
│                                                │
│  I2C address doesn't have "size"!              │
│  It's just a 7-bit identifier on the bus       │
│                                                │
└────────────────────────────────────────────────┘
```

---

## 7.2 Declaring I2C Devices in Device Tree

### 7.2.1 Basic I2C Device Declaration

**Minimal device node:**

```c
&i2c2 {
    status = "okay";

    my_device@55 {
        compatible = "vendor,my-device";
        reg = <0x55>;
    };
};
```

**Property breakdown:**

- `my_device@55`: Node name (name@address format)
- `compatible`: Matches driver's of_device_id table
- `reg`: 7-bit I2C slave address (0x55)

### 7.2.2 Real-World Example: EEPROM and RTC

**Complete DT snippet:**

```c
&i2c2 {
    #address-cells = <1>;
    #size-cells = <0>;
    clock-frequency = <100000>;  /* 100 kHz */
    status = "okay";

    /* PCF8523 RTC */
    pcf8523: rtc@68 {
        compatible = "nxp,pcf8523";
        reg = <0x68>;
    };

    /* 24LC512 EEPROM */
    eeprom: ee24lc512@55 {
        compatible = "atmel,24c512";
        reg = <0x55>;
        pagesize = <128>;        /* Custom property */
    };
};
```

**Node naming conventions:**

```
┌────────────────────────────────────────────────┐
│  Node Name Format: name@unit-address           │
├────────────────────────────────────────────────┤
│                                                │
│  pcf8523: rtc@68                               │
│  │        │   │                                │
│  │        │   └─ Unit address (from reg)       │
│  │        └───── Generic name (rtc, eeprom)    │
│  └────────────── Label (for phandle reference) │
│                                                │
│  Rules:                                        │
│  - name: lowercase, generic (rtc, eeprom)      │
│  - @address: hexadecimal, matches reg          │
│  - label: optional, for &references            │
│                                                │
└────────────────────────────────────────────────┘
```

### 7.2.3 Complete i.MX6 Example

**From real hardware (i.MX6Q SoC):**

```c
/* File: arch/arm/boot/dts/imx6qdl.dtsi */
i2c1: i2c@021a0000 {
    #address-cells = <1>;
    #size-cells = <0>;
    compatible = "fsl,imx6q-i2c", "fsl,imx21-i2c";
    reg = <0x021a0000 0x4000>;
    interrupts = <0 36 IRQ_TYPE_LEVEL_HIGH>;
    clocks = <&clks IMX6QDL_CLK_I2C1>;
    status = "disabled";
};

/* File: arch/arm/boot/dts/imx6q-sabresd.dts */
&i2c1 {
    clock-frequency = <100000>;
    pinctrl-names = "default";
    pinctrl-0 = <&pinctrl_i2c1>;
    status = "okay";

    codec: wm8962@1a {
        compatible = "wlf,wm8962";
        reg = <0x1a>;
        clocks = <&clks IMX6QDL_CLK_CKO>;
        DCVDD-supply = <&reg_audio>;
        DBVDD-supply = <&reg_audio>;
        AVDD-supply = <&reg_audio>;
        CPVDD-supply = <&reg_audio>;
        gpio-cfg = <
            0x0000 /* 0:Default */
            0x0000 /* 1:Default */
            0x0013 /* 2:FN_DMICCLK */
            0x0000 /* 3:Default */
            0x8014 /* 4:FN_DMICCDAT */
            0x0000 /* 5:Default */
        >;
    };
};
```

### 7.2.4 I2C Device with IRQ

**Device using interrupt:**

```c
&i2c3 {
    status = "okay";

    touchscreen@38 {
        compatible = "edt,edt-ft5306";
        reg = <0x38>;

        interrupt-parent = <&gpio1>;
        interrupts = <9 IRQ_TYPE_EDGE_FALLING>;

        touchscreen-size-x = <800>;
        touchscreen-size-y = <480>;
    };
};
```

**Interrupt properties:**

- `interrupt-parent`: Phandle to interrupt controller (GPIO controller)
- `interrupts`: `<gpio_pin irq_type>`
- IRQ types: `IRQ_TYPE_EDGE_FALLING`, `IRQ_TYPE_LEVEL_LOW`, etc.

---

## 7.3 OF-Style Driver Matching

### 7.3.1 The of_device_id Structure

**Definition:**

```c
struct of_device_id {
    char compatible[128];
    const void *data;
};
```

**Driver example:**

```c
static const struct of_device_id my_i2c_of_match[] = {
    {
        .compatible = "vendor,my-device",
        .data = NULL,  /* Or pointer to device-specific data */
    },
    {
        .compatible = "vendor,my-device-v2",
        .data = (void *)DEVICE_V2_FLAG,
    },
    { }  /* Sentinel */
};
MODULE_DEVICE_TABLE(of, my_i2c_of_match);
```

### 7.3.2 Complete Driver with DT Support

**Basic template:**

```c
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/of.h>
#include <linux/of_device.h>

/* Device-specific data */
struct my_device {
    struct i2c_client *client;
    /* Add your fields */
};

/* OF match table */
static const struct of_device_id my_of_match[] = {
    { .compatible = "vendor,my-device", },
    { }
};
MODULE_DEVICE_TABLE(of, my_of_match);

/* I2C device ID table (still required for older kernels) */
static const struct i2c_device_id my_id[] = {
    { "my-device", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, my_id);

/* Probe function */
static int my_probe(struct i2c_client *client,
                   const struct i2c_device_id *id)
{
    struct my_device *dev;
    struct device_node *np = client->dev.of_node;
    const struct of_device_id *match;

    /* Check if instantiated from Device Tree */
    if (np) {
        dev_info(&client->dev, "Probed via Device Tree\n");

        /* Get matching entry */
        match = of_match_device(my_of_match, &client->dev);
        if (match) {
            dev_info(&client->dev, "Matched: %s\n", match->compatible);
            /* Use match->data if needed */
        }
    } else {
        dev_info(&client->dev, "Probed via legacy method\n");
        /* Use id->driver_data if needed */
    }

    /* Allocate private data */
    dev = devm_kzalloc(&client->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    dev->client = client;
    i2c_set_clientdata(client, dev);

    /* Device initialization */

    return 0;
}

/* Remove function */
static int my_remove(struct i2c_client *client)
{
    /* Cleanup */
    return 0;
}

/* I2C driver */
static struct i2c_driver my_driver = {
    .driver = {
        .name = "my-driver",
        .of_match_table = of_match_ptr(my_of_match),
    },
    .probe = my_probe,
    .remove = my_remove,
    .id_table = my_id,
};

module_i2c_driver(my_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("I2C Driver with DT Support");
```

### 7.3.3 The of_match_ptr() Macro

**What it does:**

```c
#ifdef CONFIG_OF
#define of_match_ptr(_ptr) (_ptr)
#else
#define of_match_ptr(_ptr) NULL
#endif
```

**Usage:**

```c
static struct i2c_driver my_driver = {
    .driver = {
        .of_match_table = of_match_ptr(my_of_match),
    },
    /* ... */
};
```

**Why use it?**

- If `CONFIG_OF` is disabled, `of_match_ptr()` returns NULL
- Prevents compiler warnings about unused variables
- Makes driver work in non-DT systems

---

## 7.4 Parsing Device Tree Properties

### 7.4.1 Common OF APIs for I2C Drivers

**Reading properties:**

```c
#include <linux/of.h>

/* Read string property */
const char *of_get_property(const struct device_node *np,
                           const char *name, int *lenp);

/* Read 32-bit integer */
int of_property_read_u32(const struct device_node *np,
                        const char *propname, u32 *out_value);

/* Read string */
int of_property_read_string(const struct device_node *np,
                           const char *propname,
                           const char **out_string);

/* Check if property exists */
bool of_property_read_bool(const struct device_node *np,
                          const char *propname);

/* Read array */
int of_property_read_u32_array(const struct device_node *np,
                              const char *propname,
                              u32 *out_values, size_t sz);
```

### 7.4.2 Example: Parsing Custom Properties

**Device Tree:**

```c
&i2c2 {
    my_sensor@48 {
        compatible = "vendor,my-sensor";
        reg = <0x48>;

        /* Custom properties */
        vendor,sample-rate = <100>;
        vendor,enable-feature;
        vendor,calibration = <10 20 30 40>;
        vendor,device-name = "Temperature Sensor";
    };
};
```

**Driver code:**

```c
static int my_probe(struct i2c_client *client,
                   const struct i2c_device_id *id)
{
    struct device_node *np = client->dev.of_node;
    u32 sample_rate;
    u32 calibration[4];
    const char *dev_name;
    int ret;

    if (!np) {
        dev_err(&client->dev, "No Device Tree node\n");
        return -ENODEV;
    }

    /* Read sample rate */
    ret = of_property_read_u32(np, "vendor,sample-rate", &sample_rate);
    if (ret) {
        dev_warn(&client->dev, "No sample-rate, using default\n");
        sample_rate = 50;  /* Default */
    }
    dev_info(&client->dev, "Sample rate: %u Hz\n", sample_rate);

    /* Check boolean property */
    if (of_property_read_bool(np, "vendor,enable-feature")) {
        dev_info(&client->dev, "Feature enabled\n");
        /* Enable feature */
    }

    /* Read array */
    ret = of_property_read_u32_array(np, "vendor,calibration",
                                    calibration, 4);
    if (ret) {
        dev_warn(&client->dev, "No calibration data\n");
    } else {
        dev_info(&client->dev, "Calibration: %u %u %u %u\n",
                calibration[0], calibration[1],
                calibration[2], calibration[3]);
    }

    /* Read string */
    ret = of_property_read_string(np, "vendor,device-name", &dev_name);
    if (ret == 0) {
        dev_info(&client->dev, "Device name: %s\n", dev_name);
    }

    return 0;
}
```

---

## 7.5 Complete Working Driver Example

### 7.5.1 GPIO Expander with Full DT Support

**Device Tree:**

```c
&i2c1 {
    clock-frequency = <400000>;
    status = "okay";

    gpio_exp: mcp23017@20 {
        compatible = "microchip,mcp23017";
        reg = <0x20>;
        gpio-controller;
        #gpio-cells = <2>;

        interrupt-parent = <&gpio4>;
        interrupts = <5 IRQ_TYPE_LEVEL_LOW>;
        interrupt-controller;
        #interrupt-cells = <2>;
    };
};
```

**Complete driver:**

```c
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio/driver.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_device.h>

/* MCP23017 Registers */
#define MCP23017_IODIRA     0x00
#define MCP23017_IODIRB     0x01
#define MCP23017_GPIOA      0x12
#define MCP23017_GPIOB      0x13

/* Driver private data */
struct mcp23017_chip {
    struct i2c_client *client;
    struct gpio_chip gpio_chip;
    struct mutex lock;
    u8 reg_output[2];  /* Cached output register values */
};

/* Read GPIO register */
static int mcp23017_read_reg(struct mcp23017_chip *chip, u8 reg, u8 *val)
{
    int ret;

    ret = i2c_smbus_read_byte_data(chip->client, reg);
    if (ret < 0)
        return ret;

    *val = ret;
    return 0;
}

/* Write GPIO register */
static int mcp23017_write_reg(struct mcp23017_chip *chip, u8 reg, u8 val)
{
    return i2c_smbus_write_byte_data(chip->client, reg, val);
}

/* GPIO get value */
static int mcp23017_gpio_get(struct gpio_chip *gc, unsigned offset)
{
    struct mcp23017_chip *chip = gpiochip_get_data(gc);
    u8 reg, val;
    int ret;

    /* Determine which register (A or B) */
    reg = (offset < 8) ? MCP23017_GPIOA : MCP23017_GPIOB;
    offset %= 8;

    mutex_lock(&chip->lock);
    ret = mcp23017_read_reg(chip, reg, &val);
    mutex_unlock(&chip->lock);

    if (ret < 0)
        return ret;

    return !!(val & BIT(offset));
}

/* GPIO set value */
static void mcp23017_gpio_set(struct gpio_chip *gc,
                             unsigned offset, int value)
{
    struct mcp23017_chip *chip = gpiochip_get_data(gc);
    u8 reg, *cached;
    int bank;

    bank = offset / 8;
    reg = (bank == 0) ? MCP23017_GPIOA : MCP23017_GPIOB;
    cached = &chip->reg_output[bank];
    offset %= 8;

    mutex_lock(&chip->lock);

    if (value)
        *cached |= BIT(offset);
    else
        *cached &= ~BIT(offset);

    mcp23017_write_reg(chip, reg, *cached);

    mutex_unlock(&chip->lock);
}

/* GPIO direction input */
static int mcp23017_gpio_direction_input(struct gpio_chip *gc,
                                        unsigned offset)
{
    struct mcp23017_chip *chip = gpiochip_get_data(gc);
    u8 reg, val;
    int ret, bank;

    bank = offset / 8;
    reg = (bank == 0) ? MCP23017_IODIRA : MCP23017_IODIRB;
    offset %= 8;

    mutex_lock(&chip->lock);

    ret = mcp23017_read_reg(chip, reg, &val);
    if (ret < 0)
        goto out;

    val |= BIT(offset);  /* Set bit = input */
    ret = mcp23017_write_reg(chip, reg, val);

out:
    mutex_unlock(&chip->lock);
    return ret;
}

/* GPIO direction output */
static int mcp23017_gpio_direction_output(struct gpio_chip *gc,
                                         unsigned offset, int value)
{
    struct mcp23017_chip *chip = gpiochip_get_data(gc);
    u8 reg, val;
    int ret, bank;

    /* Set output value first */
    mcp23017_gpio_set(gc, offset, value);

    bank = offset / 8;
    reg = (bank == 0) ? MCP23017_IODIRA : MCP23017_IODIRB;
    offset %= 8;

    mutex_lock(&chip->lock);

    ret = mcp23017_read_reg(chip, reg, &val);
    if (ret < 0)
        goto out;

    val &= ~BIT(offset);  /* Clear bit = output */
    ret = mcp23017_write_reg(chip, reg, val);

out:
    mutex_unlock(&chip->lock);
    return ret;
}

/* Probe function */
static int mcp23017_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
    struct mcp23017_chip *chip;
    struct gpio_chip *gc;
    struct device_node *np = client->dev.of_node;
    int ret;

    /* Check functionality */
    if (!i2c_check_functionality(client->adapter,
                                I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "SMBUS Byte Data not supported\n");
        return -ENODEV;
    }

    /* Allocate chip structure */
    chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
    if (!chip)
        return -ENOMEM;

    chip->client = client;
    mutex_init(&chip->lock);

    /* Setup GPIO chip */
    gc = &chip->gpio_chip;
    gc->label = client->name;
    gc->parent = &client->dev;
    gc->owner = THIS_MODULE;
    gc->base = -1;  /* Dynamic allocation */
    gc->ngpio = 16;  /* MCP23017 has 16 GPIOs */
    gc->can_sleep = true;  /* I2C operations can sleep */

    gc->get = mcp23017_gpio_get;
    gc->set = mcp23017_gpio_set;
    gc->direction_input = mcp23017_gpio_direction_input;
    gc->direction_output = mcp23017_gpio_direction_output;

    /* Device Tree support */
    if (np) {
        gc->of_node = np;

        /* Check if node has gpio-controller property */
        if (!of_property_read_bool(np, "gpio-controller")) {
            dev_warn(&client->dev, "Missing gpio-controller property\n");
        }
    }

    /* Initialize device - set all as inputs */
    ret = mcp23017_write_reg(chip, MCP23017_IODIRA, 0xFF);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to initialize port A\n");
        return ret;
    }

    ret = mcp23017_write_reg(chip, MCP23017_IODIRB, 0xFF);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to initialize port B\n");
        return ret;
    }

    /* Register GPIO chip */
    ret = devm_gpiochip_add_data(&client->dev, gc, chip);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to register GPIO chip: %d\n", ret);
        return ret;
    }

    i2c_set_clientdata(client, chip);

    dev_info(&client->dev, "MCP23017 GPIO expander registered, base=%d\n",
             gc->base);

    return 0;
}

/* Remove function */
static int mcp23017_remove(struct i2c_client *client)
{
    struct mcp23017_chip *chip = i2c_get_clientdata(client);

    /* GPIO chip is removed automatically (devm_*) */
    mutex_destroy(&chip->lock);

    return 0;
}

/* OF match table */
static const struct of_device_id mcp23017_of_match[] = {
    { .compatible = "microchip,mcp23017", },
    { }
};
MODULE_DEVICE_TABLE(of, mcp23017_of_match);

/* I2C device ID table */
static const struct i2c_device_id mcp23017_id[] = {
    { "mcp23017", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, mcp23017_id);

/* I2C driver */
static struct i2c_driver mcp23017_driver = {
    .driver = {
        .name = "mcp23017",
        .of_match_table = of_match_ptr(mcp23017_of_match),
    },
    .probe = mcp23017_probe,
    .remove = mcp23017_remove,
    .id_table = mcp23017_id,
};

module_i2c_driver(mcp23017_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("MCP23017 GPIO Expander Driver");
```

**Using the GPIO expander in Device Tree:**

```c
/* Other device using the expander's GPIOs */
&uart1 {
    reset-gpios = <&gpio_exp 0 GPIO_ACTIVE_LOW>;
    enable-gpios = <&gpio_exp 1 GPIO_ACTIVE_HIGH>;
};
```

---

## 7.6 Per-Device Data with Device Tree

### 7.6.1 Using .data Field in of_device_id

**Supporting multiple device variants:**

```c
/* Device-specific flags */
#define DEVICE_HAS_EEPROM    BIT(0)
#define DEVICE_HAS_LED       BIT(1)

/* Device-specific data structures */
struct device_variant {
    const char *name;
    unsigned int flags;
    int max_speed;
};

static const struct device_variant variant_v1 = {
    .name = "Device V1",
    .flags = DEVICE_HAS_EEPROM,
    .max_speed = 100000,
};

static const struct device_variant variant_v2 = {
    .name = "Device V2",
    .flags = DEVICE_HAS_EEPROM | DEVICE_HAS_LED,
    .max_speed = 400000,
};

/* OF match table with per-device data */
static const struct of_device_id my_of_match[] = {
    {
        .compatible = "vendor,device-v1",
        .data = &variant_v1,
    },
    {
        .compatible = "vendor,device-v2",
        .data = &variant_v2,
    },
    { }
};
MODULE_DEVICE_TABLE(of, my_of_match);

/* Probe function */
static int my_probe(struct i2c_client *client,
                   const struct i2c_device_id *id)
{
    const struct device_variant *variant;
    const struct of_device_id *match;

    /* Get device variant from DT match */
    match = of_match_device(my_of_match, &client->dev);
    if (match && match->data) {
        variant = match->data;

        dev_info(&client->dev, "Device: %s\n", variant->name);
        dev_info(&client->dev, "Max speed: %d Hz\n", variant->max_speed);

        if (variant->flags & DEVICE_HAS_EEPROM)
            dev_info(&client->dev, "Has EEPROM\n");

        if (variant->flags & DEVICE_HAS_LED)
            dev_info(&client->dev, "Has LED\n");
    }

    return 0;
}
```

---

## 7.7 Backward Compatibility

### 7.7.1 Supporting Both DT and Legacy Methods

**Complete example:**

```c
static int my_probe(struct i2c_client *client,
                   const struct i2c_device_id *id)
{
    struct device_node *np = client->dev.of_node;
    struct my_device *dev;
    u32 sample_rate = 100;  /* Default */

    dev = devm_kzalloc(&client->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    /* Device Tree path */
    if (np) {
        const struct of_device_id *match;

        dev_info(&client->dev, "Initializing from Device Tree\n");

        /* Get matching entry */
        match = of_match_device(my_of_match, &client->dev);
        if (match && match->data) {
            /* Use match->data for device-specific config */
        }

        /* Read DT properties */
        of_property_read_u32(np, "sample-rate", &sample_rate);

    /* Legacy path (platform_data) */
    } else if (client->dev.platform_data) {
        struct my_platform_data *pdata = client->dev.platform_data;

        dev_info(&client->dev, "Initializing from platform data\n");

        sample_rate = pdata->sample_rate;

    /* No configuration provided */
    } else {
        dev_warn(&client->dev, "No configuration, using defaults\n");
    }

    dev_info(&client->dev, "Sample rate: %u Hz\n", sample_rate);

    /* Continue with initialization */
    dev->client = client;
    i2c_set_clientdata(client, dev);

    return 0;
}
```

---

## 7.8 Complete Driver Checklist

### 7.8.1 Writing I2C Driver with DT Support

**Step-by-step checklist:**

```
✓ 1. Define device-specific data structure
     struct my_device { ... };

✓ 2. Create OF match table
     static const struct of_device_id my_of_match[] = {
         { .compatible = "vendor,device", },
         { }
     };
     MODULE_DEVICE_TABLE(of, my_of_match);

✓ 3. Create I2C device ID table
     static const struct i2c_device_id my_id[] = {
         { "device-name", 0 },
         { }
     };
     MODULE_DEVICE_TABLE(i2c, my_id);

✓ 4. Write probe function
     - Check i2c_check_functionality()
     - Allocate private data with devm_kzalloc()
     - Parse Device Tree properties (if np != NULL)
     - Initialize device
     - Register with kernel framework
     - Store private data with i2c_set_clientdata()

✓ 5. Write remove function
     - Retrieve private data with i2c_get_clientdata()
     - Unregister from kernel framework
     - Cleanup resources

✓ 6. Fill i2c_driver structure
     .driver.name
     .driver.of_match_table = of_match_ptr(my_of_match)
     .probe
     .remove
     .id_table

✓ 7. Register driver with module_i2c_driver()

✓ 8. Add module metadata
     MODULE_LICENSE("GPL");
     MODULE_AUTHOR("...");
     MODULE_DESCRIPTION("...");
```

### 7.8.2 Device Tree Checklist

```
✓ 1. Find I2C controller node
     &i2c1 { ... };

✓ 2. Add device node
     device_name@address {
         compatible = "vendor,device";
         reg = <0xADDRESS>;
         /* Add other properties */
     };

✓ 3. Verify addressing
     - I2C controller has #address-cells = <1>
     - I2C controller has #size-cells = <0>
     - Device reg is 7-bit I2C address

✓ 4. Add required properties
     - compatible (matches driver)
     - reg (I2C address)

✓ 5. Add optional properties
     - interrupts / interrupt-parent
     - Custom vendor properties
     - Supply references

✓ 6. Compile and test
     make dtbs
     Check /proc/device-tree
```

---

## Summary

In this part, we covered:

**Device Tree Fundamentals:**

- I2C devices as DT children
- #address-cells = <1>, #size-cells = <0>
- reg property = I2C slave address
- Node naming: name@address

**OF-Style Matching:**

- `struct of_device_id` table
- `MODULE_DEVICE_TABLE(of, ...)`
- `of_match_device()` in probe
- `of_match_ptr()` macro

**Property Parsing:**

- `of_property_read_u32()`
- `of_property_read_string()`
- `of_property_read_bool()`
- `of_property_read_u32_array()`

**Complete Examples:**

- EEPROM and RTC devices
- GPIO expander with full DT support
- Per-device data with .data field
- Backward compatibility

**Best Practices:**

- Check for Device Tree node (`np != NULL`)
- Provide defaults for missing properties
- Support both DT and legacy methods
- Use devm_* functions
- Proper error handling

**Next Steps:**
You now have all the knowledge to write complete I2C client drivers with Device Tree support. Practice by:

1. Writing drivers for real I2C devices
2. Adding DT support to existing drivers
3. Testing on real hardware