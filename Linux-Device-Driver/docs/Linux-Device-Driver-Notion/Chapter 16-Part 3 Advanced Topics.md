# Part 3. Advanced Topics

This part covers threaded interrupts, the regmap IRQ API, Device Tree IRQ bindings, and best practices.

---

## 16.9 Threaded Interrupts

### 16.9.1 Understanding Threaded IRQs

**The problem threaded IRQs solve:**

```
┌────────────────────────────────────────────────┐
│  Why Threaded Interrupts?                      │
├────────────────────────────────────────────────┤
│                                                │
│  Traditional IRQ Handler:                      │
│  • Runs in hard IRQ context (atomic)           │
│  • Cannot sleep                                │
│  • Blocks other interrupts                     │
│  • Must be very fast                           │
│  • Can't do much work                          │
│                                                │
│  Problem: What if handler needs to:            │
│  • Access I2C/SPI device (sleeps!)             │
│  • Allocate memory with GFP_KERNEL             │
│  • Take mutex (may sleep)                      │
│  • Do time-consuming work                      │
│                                                │
│  Solution: Threaded IRQs!                      │
│  • Split into top half + bottom half           │
│  • Top half: Fast, atomic, minimal work        │
│  • Bottom half: Thread, can sleep, do work     │
│                                                │
└────────────────────────────────────────────────┘
```

### 16.9.2 Top Half vs Bottom Half

**Two-part execution model:**

```
┌────────────────────────────────────────────────┐
│  Threaded IRQ Execution Model                  │
├────────────────────────────────────────────────┤
│                                                │
│  Hardware interrupt occurs                     │
│         │                                      │
│         ▼                                      │
│  ┌─────────────────────────────┐               │
│  │  Top Half (handler)         │               │
│  │  • Hard IRQ context         │               │
│  │  • Atomic, cannot sleep     │               │
│  │  • Quick acknowledge        │               │
│  │  • Return IRQ_WAKE_THREAD   │               │
│  └─────────────┬───────────────┘               │
│                │                               │
│                │ IRQ_WAKE_THREAD               │
│                ▼                               │
│  ┌─────────────────────────────┐               │
│  │  Schedule Thread            │               │
│  │  • Mark IRQ pending         │               │
│  │  • Wake up IRQ thread       │               │
│  └─────────────┬───────────────┘               │
│                │                               │
│                ▼                               │
│  ┌─────────────────────────────┐               │
│  │  Bottom Half (thread_fn)    │               │
│  │  • Thread context           │               │
│  │  • CAN sleep                │               │
│  │  • Can take mutex           │               │
│  │  • Can do I2C/SPI           │               │
│  │  • Do actual work           │               │
│  │  • Return IRQ_HANDLED       │               │
│  └─────────────────────────────┘               │
│                                                │
└────────────────────────────────────────────────┘
```

### 16.9.3 Using request_threaded_irq()

**Function signature:**

```c
int request_threaded_irq(unsigned int irq,
                         irq_handler_t handler,
                         irq_handler_t thread_fn,
                         unsigned long flags,
                         const char *name,
                         void *dev_id);

/* handler:   Top half (optional, can be NULL) */
/* thread_fn: Bottom half (mandatory) */
```

**Three usage patterns:**

```c
/* Pattern 1: Both top and bottom half */
ret = request_threaded_irq(irq,
                           my_hardirq_handler,      /* Top half */
                           my_threaded_handler,     /* Bottom half */
                           IRQF_ONESHOT,
                           "my-device",
                           priv);

/* Pattern 2: Only threaded handler (most common) */
ret = request_threaded_irq(irq,
                           NULL,                    /* No top half */
                           my_threaded_handler,     /* Bottom half */
                           IRQF_ONESHOT,
                           "my-device",
                           priv);

/* Pattern 3: Only hard IRQ (traditional) */
ret = request_irq(irq,
                  my_hardirq_handler,
                  0,
                  "my-device",
                  priv);
```

### 16.9.4 Complete Example

```c
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/gpio/consumer.h>

struct sensor_device {
    struct i2c_client *client;
    struct gpio_desc *irq_gpio;
    int irq;
    struct work_struct work;
};

/*
 * Top half: Hard IRQ handler (optional)
 * Fast, atomic, just acknowledge
 */
static irqreturn_t sensor_hardirq(int irq, void *dev_id)
{
    struct sensor_device *sensor = dev_id;

    /* Quick check if this is our interrupt */
    /* Could read a status register if very fast */

    pr_debug("Sensor IRQ: Top half\n");

    /* Wake up thread */
    return IRQ_WAKE_THREAD;
}

/*
 * Bottom half: Threaded handler
 * Runs in thread context, can sleep
 */
static irqreturn_t sensor_threaded_handler(int irq, void *dev_id)
{
    struct sensor_device *sensor = dev_id;
    u8 data[16];
    int ret;

    pr_debug("Sensor IRQ: Thread handler\n");

    /*
     * Can do I2C access here (sleeps!)
     * Can take mutexes
     * Can do time-consuming work
     */

    /* Read data from sensor via I2C */
    ret = i2c_smbus_read_i2c_block_data(sensor->client,
                                         0x00,
                                         sizeof(data),
                                         data);
    if (ret < 0) {
        dev_err(&sensor->client->dev,
                "Failed to read sensor data\n");
        return IRQ_HANDLED;
    }

    /* Process data */
    pr_info("Sensor data: %02x %02x %02x %02x...\n",
            data[0], data[1], data[2], data[3]);

    /* Can sleep if needed */
    msleep(10);

    /* Report to input subsystem, etc. */

    return IRQ_HANDLED;
}

static int sensor_probe(struct i2c_client *client,
                        const struct i2c_device_id *id)
{
    struct sensor_device *sensor;
    int ret;

    sensor = devm_kzalloc(&client->dev,
                          sizeof(*sensor),
                          GFP_KERNEL);
    if (!sensor)
        return -ENOMEM;

    sensor->client = client;
    i2c_set_clientdata(client, sensor);

    /* Get IRQ GPIO from DT */
    sensor->irq_gpio = devm_gpiod_get(&client->dev,
                                      "interrupt",
                                      GPIOD_IN);
    if (IS_ERR(sensor->irq_gpio))
        return PTR_ERR(sensor->irq_gpio);

    /* Get IRQ number */
    sensor->irq = gpiod_to_irq(sensor->irq_gpio);
    if (sensor->irq < 0)
        return sensor->irq;

    /*
     * Request threaded IRQ with IRQF_ONESHOT
     * IRQF_ONESHOT: Keep IRQ masked until thread completes
     */
    ret = devm_request_threaded_irq(&client->dev,
                                     sensor->irq,
                                     sensor_hardirq,
                                     sensor_threaded_handler,
                                     IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                                     "sensor-irq",
                                     sensor);
    if (ret) {
        dev_err(&client->dev,
                "Failed to request IRQ %d\n",
                sensor->irq);
        return ret;
    }

    dev_info(&client->dev,
             "Threaded IRQ handler registered on IRQ %d\n",
             sensor->irq);

    return 0;
}

static const struct of_device_id sensor_of_match[] = {
    { .compatible = "vendor,sensor" },
    { }
};
MODULE_DEVICE_TABLE(of, sensor_of_match);

static const struct i2c_device_id sensor_id[] = {
    { "sensor", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct i2c_driver sensor_driver = {
    .driver = {
        .name = "sensor",
        .of_match_table = sensor_of_match,
    },
    .probe = sensor_probe,
    .id_table = sensor_id,
};

module_i2c_driver(sensor_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Sensor Driver with Threaded IRQ");
```

---

## 16.10 Regmap IRQ API

### 16.10.1 Overview

**What is regmap IRQ API?**

```
┌────────────────────────────────────────────────┐
│  Regmap IRQ API - High-Level IRQ Helper        │
├────────────────────────────────────────────────┤
│                                                │
│  Purpose:                                      │
│  • Simplify IRQ chip implementation            │
│  • Automatic register handling                 │
│  • Built-in threading support                  │
│  • Perfect for MFD (Multi-Function Devices)    │
│                                                │
│  Use case:                                     │
│  Device with multiple interrupt sources:       │
│  • Status register shows which IRQs active     │
│  • Mask register to enable/disable IRQs        │
│  • Type register for edge/level config         │
│                                                │
│  Examples:                                     │
│  • PMIC (Power Management ICs)                 │
│  • Audio codecs with multiple sources          │
│  • GPIO expanders with IRQ                     │
│  • MFD chips (combine multiple functions)      │
│                                                │
└────────────────────────────────────────────────┘
```

### 16.10.2 Core Structures

```c
#include <linux/regmap.h>
#include <linux/irq.h>

/* Describe a single IRQ */
struct regmap_irq {
    unsigned int reg_offset;  /* Offset in status/mask array */
    unsigned int mask;        /* Bit mask for this IRQ */
    unsigned int type_reg_offset;  /* Type register offset */
    unsigned int type_rising_mask; /* Rising edge mask */
    unsigned int type_falling_mask; /* Falling edge mask */
};

/* Describe IRQ chip */
struct regmap_irq_chip {
    const char *name;

    /* Status registers */
    unsigned int status_base;
    unsigned int num_regs;

    /* Mask registers */
    unsigned int mask_base;
    bool mask_invert;  /* 1 = masked, 0 = unmasked? */

    /* Unmask registers (some chips use separate regs) */
    unsigned int unmask_base;

    /* Type registers */
    unsigned int type_base;
    unsigned int num_type_reg;

    /* IRQ definitions */
    const struct regmap_irq *irqs;
    int num_irqs;

    /* Register stride */
    unsigned int irq_reg_stride;
    unsigned int type_reg_stride;

    /* Init/ack callbacks */
    int (*init_ack_masked)(struct regmap *map,
                           struct regmap_irq_chip_data *data);
    int (*ack_invert)(struct regmap *map,
                      struct regmap_irq_chip_data *data);
};

/* Runtime data (returned by regmap_add_irq_chip) */
struct regmap_irq_chip_data;
```

### 16.10.3 Example Implementation

```c
/* Define individual IRQs */
static const struct regmap_irq max77620_irqs[] = {
    [MAX77620_IRQ_GPIO0] = {
        .reg_offset = 0,
        .mask = MAX77620_IRQ_GPIO0_MASK,
    },
    [MAX77620_IRQ_GPIO1] = {
        .reg_offset = 0,
        .mask = MAX77620_IRQ_GPIO1_MASK,
    },
    [MAX77620_IRQ_GPIO2] = {
        .reg_offset = 0,
        .mask = MAX77620_IRQ_GPIO2_MASK,
    },
    /* ... more IRQs ... */
};

/* Define IRQ chip */
static const struct regmap_irq_chip max77620_irq_chip = {
    .name = "max77620",
    .irqs = max77620_irqs,
    .num_irqs = ARRAY_SIZE(max77620_irqs),
    .num_regs = 1,
    .irq_reg_stride = 1,
    .status_base = MAX77620_REG_IRQ_STATUS,
    .mask_base = MAX77620_REG_IRQ_MASK,
};

/* Use in probe */
static int max77620_probe(struct i2c_client *client)
{
    struct max77620_chip *chip;
    struct regmap_irq_chip_data *irq_data;
    int ret;

    chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
    if (!chip)
        return -ENOMEM;

    /* Create regmap */
    chip->rmap = devm_regmap_init_i2c(client, &max77620_regmap_config);
    if (IS_ERR(chip->rmap))
        return PTR_ERR(chip->rmap);

    /*
     * Register IRQ chip with regmap
     * This:
     * - Creates IRQ domain
     * - Registers threaded handler
     * - Handles all IRQ management
     */
    ret = devm_regmap_add_irq_chip(&client->dev,
                                    chip->rmap,
                                    client->irq,
                                    IRQF_ONESHOT,
                                    0,
                                    &max77620_irq_chip,
                                    &irq_data);
    if (ret) {
        dev_err(&client->dev, "Failed to add IRQ chip\n");
        return ret;
    }

    /* Store IRQ data for child devices */
    chip->irq_data = irq_data;

    /* Register MFD children */
    ret = devm_mfd_add_devices(&client->dev,
                                PLATFORM_DEVID_NONE,
                                max77620_children,
                                ARRAY_SIZE(max77620_children),
                                NULL, 0,
                                regmap_irq_get_domain(irq_data));

    return ret;
}

/* Get virq for a specific IRQ */
static int max77620_get_virq(struct max77620_chip *chip, int irq)
{
    return regmap_irq_get_virq(chip->irq_data, irq);
}
```

---

## 16.11 Device Tree IRQ Binding

### 16.11.1 Interrupt Specifier Format

**Standard two-cell format:**

```
/* Interrupt controller */
gpio_controller: gpio@20 {
    compatible = "vendor,gpio-controller";
    reg = <0x20>;

    gpio-controller;
    #gpio-cells = <2>;

    /* Mark as interrupt controller */
    interrupt-controller;
    #interrupt-cells = <2>;

    /* Parent interrupt connection */
    interrupt-parent = <&intc>;
    interrupts = <96 IRQ_TYPE_LEVEL_HIGH>;
};

/* Consumer device */
sensor {
    compatible = "vendor,sensor";

    /* Use GPIO controller as interrupt parent */
    interrupt-parent = <&gpio_controller>;

    /* Interrupt specifier: <hwirq flags> */
    interrupts = <5 IRQ_TYPE_EDGE_FALLING>;
    /*            │  │                        */
    /*            │  └─ Cell 1: IRQ flags     */
    /*            └──── Cell 0: hwirq (GPIO 5)*/
};
```

**Common IRQ type flags:**

```c
/* From include/dt-bindings/interrupt-controller/irq.h */

#define IRQ_TYPE_NONE           0
#define IRQ_TYPE_EDGE_RISING    1
#define IRQ_TYPE_EDGE_FALLING   2
#define IRQ_TYPE_EDGE_BOTH      (IRQ_TYPE_EDGE_RISING | \
                                 IRQ_TYPE_EDGE_FALLING)
#define IRQ_TYPE_LEVEL_HIGH     4
#define IRQ_TYPE_LEVEL_LOW      8
```

### 16.11.2 Complete DT Example

```
/* SoC interrupt controller (GIC) */
intc: interrupt-controller@a000 {
    compatible = "arm,cortex-a9-gic";
    #interrupt-cells = <3>;
    interrupt-controller;
    reg = <0xa000 0x1000>,
          <0xa100 0x100>;
};

/* SoC GPIO controller */
gpio4: gpio@20a8000 {
    compatible = "fsl,imx6q-gpio";
    reg = <0x020a8000 0x4000>;
    interrupts = <0 74 IRQ_TYPE_LEVEL_HIGH>,
                 <0 75 IRQ_TYPE_LEVEL_HIGH>;
    gpio-controller;
    #gpio-cells = <2>;
    interrupt-controller;
    #interrupt-cells = <2>;
};

/* I2C bus */
i2c1: i2c@21a0000 {
    compatible = "fsl,imx6q-i2c";
    reg = <0x021a0000 0x4000>;
    interrupts = <0 36 IRQ_TYPE_LEVEL_HIGH>;
    #address-cells = <1>;
    #size-cells = <0>;

    /* GPIO expander on I2C */
    expander: mcp23016@20 {
        compatible = "microchip,mcp23016";
        reg = <0x20>;

        gpio-controller;
        #gpio-cells = <2>;

        interrupt-controller;
        #interrupt-cells = <2>;

        /* Connected to GPIO4_29 */
        interrupt-parent = <&gpio4>;
        interrupts = <29 IRQ_TYPE_LEVEL_LOW>;
    };

    /* Sensor on I2C */
    sensor@48 {
        compatible = "vendor,sensor";
        reg = <0x48>;

        /* Use expander GPIO 2 as interrupt */
        interrupt-parent = <&expander>;
        interrupts = <2 IRQ_TYPE_EDGE_FALLING>;
    };
};

/* Device using expander GPIOs */
leds {
    compatible = "gpio-leds";

    led1 {
        gpios = <&expander 0 GPIO_ACTIVE_HIGH>;
        default-state = "off";
    };
};

gpio-keys {
    compatible = "gpio-keys";

    button1 {
        label = "User Button";
        gpios = <&expander 8 GPIO_ACTIVE_LOW>;
        linux,code = <KEY_ENTER>;

        /* Can also specify interrupt directly */
        interrupt-parent = <&expander>;
        interrupts = <8 IRQ_TYPE_EDGE_FALLING>;
    };
};
```

### 16.11.3 Parsing Interrupts in Driver

```c
#include <linux/of_irq.h>

static int my_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    int irq;
    int ret;

    /* Method 1: platform_get_irq() - Most common */
    irq = platform_get_irq(pdev, 0);
    if (irq < 0) {
        dev_err(&pdev->dev, "No IRQ resource\n");
        return irq;
    }

    /* Method 2: irq_of_parse_and_map() */
    irq = irq_of_parse_and_map(np, 0);
    if (!irq) {
        dev_err(&pdev->dev, "Failed to parse IRQ\n");
        return -EINVAL;
    }

    /* Method 3: of_irq_get() */
    irq = of_irq_get(np, 0);
    if (irq < 0) {
        dev_err(&pdev->dev, "Failed to get IRQ\n");
        return irq;
    }

    /* Now use the IRQ */
    ret = devm_request_irq(&pdev->dev, irq,
                           my_irq_handler,
                           0, "my-device", priv);

    return ret;
}
```

---

## 16.12 Best Practices

### 16.12.1 Choosing the Right Approach

```
┌────────────────────────────────────────────────┐
│  Decision Tree: Which IRQ Method?              │
├────────────────────────────────────────────────┤
│                                                │
│  Q1: Is this an interrupt controller driver?   │
│      NO  → Use request_irq() / request_        │
│             threaded_irq()                     │
│      YES → Continue to Q2                      │
│                                                │
│  Q2: Does hardware access sleep?               │
│      NO  → Chained interrupts                  │
│            • irq_set_chained_handler_and_data()│
│            • generic_handle_irq()              │
│      YES → Nested interrupts                   │
│            • request_threaded_irq()            │
│            • handle_nested_irq()               │
│                                                │
│  Q3: Is this a GPIO chip?                      │
│      YES → Use gpiolib irqchip API             │
│             • gpiochip_irqchip_add[_nested]()  │
│             • gpiochip_set_[chained|nested]    │
│      NO  → Manual IRQ domain                   │
│                                                │
│  Q4: Is this an MFD with status registers?     │
│      YES → Consider regmap IRQ API             │
│             • regmap_add_irq_chip()            │
│      NO  → Use standard approach               │
│                                                │
└────────────────────────────────────────────────┘
```

### 16.12.2 Common Pitfalls

**1. Sleeping in hard IRQ context:**

```c
/* WRONG - Never sleep in hard IRQ! */
static irqreturn_t bad_handler(int irq, void *dev_id)
{
    u8 data;

    /* This sleeps! BUG! */
    data = i2c_smbus_read_byte_data(client, REG);

    return IRQ_HANDLED;
}

/* CORRECT - Use threaded IRQ */
static irqreturn_t good_handler(int irq, void *dev_id)
{
    u8 data;

    /* OK in thread context */
    data = i2c_smbus_read_byte_data(client, REG);

    return IRQ_HANDLED;
}

ret = request_threaded_irq(irq, NULL, good_handler,
                           IRQF_ONESHOT, "device", priv);
```

**2. Forgetting IRQF_ONESHOT:**

```c
/* WRONG - No IRQF_ONESHOT */
request_threaded_irq(irq, NULL, handler, 0, "dev", priv);
/* IRQ will fire repeatedly before thread runs! */

/* CORRECT */
request_threaded_irq(irq, NULL, handler,
                     IRQF_ONESHOT,  /* Keep masked */
                     "dev", priv);
```

**3. Not acknowledging interrupts:**

```c
/* WRONG - Status not cleared */
static irqreturn_t bad_handler(int irq, void *dev_id)
{
    u32 status = readl(base + STATUS_REG);

    /* Handle interrupt... */

    /* Forgot to clear! Will fire again immediately! */
    return IRQ_HANDLED;
}

/* CORRECT */
static irqreturn_t good_handler(int irq, void *dev_id)
{
    u32 status = readl(base + STATUS_REG);

    /* Handle interrupt... */

    /* Clear status */
    writel(status, base + STATUS_CLR_REG);

    return IRQ_HANDLED;
}
```

---

## Summary

**Chapter 16 Complete!**

We covered:

1. **IRQ Architecture**
    - Three-layer architecture
    - hwirq vs virq
    - Complete propagation flow
2. **IRQ Domain API**
    - Why domains needed
    - Creating domains
    - map() and xlate() callbacks
3. **IRQ Multiplexing**
    - Chained interrupts (fast, atomic)
    - Nested interrupts (slow, threaded)
    - Complete implementations
4. **Threaded Interrupts**
    - Top half + bottom half
    - request_threaded_irq()
    - When to use
5. **Regmap IRQ API**
    - High-level helper
    - Perfect for MFD devices
    - Automatic threading
6. **Device Tree**
    - Interrupt specifiers
    - #interrupt-cells
    - Complete examples
7. **Best Practices**
    - Choosing right approach
    - Common pitfalls
    - Debugging tips