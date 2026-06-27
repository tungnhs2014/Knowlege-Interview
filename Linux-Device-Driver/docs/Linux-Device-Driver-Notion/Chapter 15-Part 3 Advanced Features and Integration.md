# Part 3. Advanced Features and Integration

This part covers Device Tree integration, sysfs interface, pin controller guidelines, and complete working examples with best practices.

---

## 15.11 Device Tree Integration

### 15.11.1 GPIO Controller Properties

**Required properties:**

```
gpio_controller: mcp23016@20 {
    compatible = "microchip,mcp23016";
    reg = <0x20>;

    /* Required: Marks this as GPIO controller */
    gpio-controller;

    /* Required: Number of cells in GPIO specifier */
    #gpio-cells = <2>;
};
```

**With interrupt support:**

```
gpio_controller: mcp23016@20 {
    compatible = "microchip,mcp23016";
    reg = <0x20>;

    gpio-controller;
    #gpio-cells = <2>;

    interrupt-controller;
    #interrupt-cells = <2>;

    interrupt-parent = <&gpio6>;
    interrupts = <31 IRQ_TYPE_LEVEL_LOW>;
};
```

### 15.11.2 Complete Device Tree Example

```
&i2c1 {
    status = "okay";

    expander: mcp23016@20 {
        compatible = "microchip,mcp23016";
        reg = <0x20>;

        gpio-controller;
        #gpio-cells = <2>;

        interrupt-controller;
        #interrupt-cells = <2>;
        interrupt-parent = <&gpio6>;
        interrupts = <31 IRQ_TYPE_LEVEL_LOW>;

        gpio-line-names =
            "EXP_IO0", "EXP_IO1", "EXP_IO2", "EXP_IO3",
            "EXP_IO4", "EXP_IO5", "EXP_IO6", "EXP_IO7",
            "EXP_IO8", "EXP_IO9", "EXP_IO10", "EXP_IO11",
            "EXP_IO12", "EXP_IO13", "EXP_IO14", "EXP_IO15";
    };
};

leds {
    compatible = "gpio-leds";

    status_led {
        gpios = <&expander 0 GPIO_ACTIVE_HIGH>;
        default-state = "off";
    };
};

gpio-keys {
    compatible = "gpio-keys";

    button1 {
        gpios = <&expander 8 GPIO_ACTIVE_LOW>;
        linux,code = <KEY_F1>;
    };
};
```

---

## 15.12 Sysfs Interface

### 15.12.1 Automatic Creation

```bash
$ ls -la /sys/class/gpio/gpiochip480/
-r--r--r--  1 root root 4096 base
-r--r--r--  1 root root 4096 label
-r--r--r--  1 root root 4096 ngpio

$ cat /sys/class/gpio/gpiochip480/base
480
$ cat /sys/class/gpio/gpiochip480/ngpio
16
```

---

## 15.13 Complete Working Driver

```c
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio/driver.h>
#include <linux/interrupt.h>

#define MCP23016_GP0        0x00
#define MCP23016_GP1        0x01
#define MCP23016_IODIR0     0x06
#define MCP23016_IODIR1     0x07
#define MCP23016_GPIO_NUM   16

struct mcp23016 {
    struct i2c_client *client;
    struct gpio_chip chip;
    struct mutex lock;
    u16 reg_output;
    u16 reg_direction;
};

/* Helper functions */
static inline struct mcp23016 *to_mcp23016(struct gpio_chip *gc)
{
    return container_of(gc, struct mcp23016, chip);
}

static int mcp23016_read_word(struct mcp23016 *mcp, u8 reg, u16 *val)
{
    u8 lo, hi;
    int ret;

    ret = i2c_smbus_read_byte_data(mcp->client, reg);
    if (ret < 0) return ret;
    lo = ret;

    ret = i2c_smbus_read_byte_data(mcp->client, reg + 1);
    if (ret < 0) return ret;
    hi = ret;

    *val = (u16)lo | ((u16)hi << 8);
    return 0;
}

static int mcp23016_write_word(struct mcp23016 *mcp, u8 reg, u16 val)
{
    int ret;
    ret = i2c_smbus_write_byte_data(mcp->client, reg, val & 0xFF);
    if (ret) return ret;
    return i2c_smbus_write_byte_data(mcp->client, reg + 1, val >> 8);
}

/* GPIO callbacks */
static int mcp23016_direction_input(struct gpio_chip *gc, unsigned offset)
{
    struct mcp23016 *mcp = to_mcp23016(gc);
    int ret;

    mutex_lock(&mcp->lock);
    mcp->reg_direction |= (1 << offset);
    ret = mcp23016_write_word(mcp, MCP23016_IODIR0, mcp->reg_direction);
    mutex_unlock(&mcp->lock);

    return ret;
}

static int mcp23016_direction_output(struct gpio_chip *gc,
                                      unsigned offset, int value)
{
    struct mcp23016 *mcp = to_mcp23016(gc);
    int ret;

    mutex_lock(&mcp->lock);

    if (value)
        mcp->reg_output |= (1 << offset);
    else
        mcp->reg_output &= ~(1 << offset);

    ret = mcp23016_write_word(mcp, MCP23016_GP0, mcp->reg_output);
    if (ret)
        goto out;

    mcp->reg_direction &= ~(1 << offset);
    ret = mcp23016_write_word(mcp, MCP23016_IODIR0, mcp->reg_direction);

out:
    mutex_unlock(&mcp->lock);
    return ret;
}

static int mcp23016_get_value(struct gpio_chip *gc, unsigned offset)
{
    struct mcp23016 *mcp = to_mcp23016(gc);
    u16 value;

    if (mcp23016_read_word(mcp, MCP23016_GP0, &value))
        return 0;

    return (value & (1 << offset)) ? 1 : 0;
}

static void mcp23016_set_value(struct gpio_chip *gc,
                                unsigned offset, int value)
{
    struct mcp23016 *mcp = to_mcp23016(gc);

    mutex_lock(&mcp->lock);

    if (value)
        mcp->reg_output |= (1 << offset);
    else
        mcp->reg_output &= ~(1 << offset);

    mcp23016_write_word(mcp, MCP23016_GP0, mcp->reg_output);

    mutex_unlock(&mcp->lock);
}

/* IRQ handler */
static irqreturn_t mcp23016_irq(int irq, void *data)
{
    struct mcp23016 *mcp = data;
    u16 status;
    unsigned int i, child_irq;

    if (mcp23016_read_word(mcp, 0x08, &status))
        return IRQ_HANDLED;

    for (i = 0; i < mcp->chip.ngpio; i++) {
        if (status & (1 << i)) {
            child_irq = irq_find_mapping(mcp->chip.irq.domain, i);
            if (child_irq)
                handle_nested_irq(child_irq);
        }
    }

    return IRQ_HANDLED;
}

/* Probe */
static int mcp23016_probe(struct i2c_client *client,
                          const struct i2c_device_id *id)
{
    struct mcp23016 *mcp;
    int ret;

    mcp = devm_kzalloc(&client->dev, sizeof(*mcp), GFP_KERNEL);
    if (!mcp)
        return -ENOMEM;

    mcp->client = client;
    mutex_init(&mcp->lock);

    mcp23016_read_word(mcp, MCP23016_GP0, &mcp->reg_output);
    mcp23016_read_word(mcp, MCP23016_IODIR0, &mcp->reg_direction);

    mcp->chip.label = "mcp23016";
    mcp->chip.dev = &client->dev;
    mcp->chip.owner = THIS_MODULE;
    mcp->chip.base = -1;
    mcp->chip.ngpio = MCP23016_GPIO_NUM;
    mcp->chip.can_sleep = true;

    mcp->chip.direction_input = mcp23016_direction_input;
    mcp->chip.direction_output = mcp23016_direction_output;
    mcp->chip.get = mcp23016_get_value;
    mcp->chip.set = mcp23016_set_value;

    mcp->chip.of_node = client->dev.of_node;

    ret = devm_gpiochip_add_data(&client->dev, &mcp->chip, mcp);
    if (ret)
        return ret;

    if (client->irq) {
        ret = gpiochip_irqchip_add_nested(&mcp->chip,
                                          &dummy_irq_chip,
                                          0,
                                          handle_level_irq,
                                          IRQ_TYPE_NONE);
        if (ret)
            return ret;

        ret = devm_request_threaded_irq(&client->dev,
                                         client->irq,
                                         NULL,
                                         mcp23016_irq,
                                         IRQF_ONESHOT | IRQF_TRIGGER_LOW,
                                         "mcp23016",
                                         mcp);
        if (ret)
            return ret;

        gpiochip_set_nested_irqchip(&mcp->chip,
                                     &dummy_irq_chip,
                                     client->irq);
    }

    dev_info(&client->dev, "MCP23016 registered: base=%d\n",
             mcp->chip.base);

    return 0;
}

static const struct of_device_id mcp23016_of_match[] = {
    { .compatible = "microchip,mcp23016" },
    { }
};
MODULE_DEVICE_TABLE(of, mcp23016_of_match);

static const struct i2c_device_id mcp23016_id[] = {
    { "mcp23016", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, mcp23016_id);

static struct i2c_driver mcp23016_driver = {
    .driver = {
        .name = "mcp23016",
        .of_match_table = mcp23016_of_match,
    },
    .probe = mcp23016_probe,
    .id_table = mcp23016_id,
};

module_i2c_driver(mcp23016_driver);

MODULE_AUTHOR("Embedded Linux Expert");
MODULE_DESCRIPTION("MCP23016 GPIO Controller Driver");
MODULE_LICENSE("GPL");
```

---

## 15.14 Best Practices

### 15.14.1 Use Resource-Managed Functions

```c
/* Prefer devm_* */
devm_kzalloc()
devm_gpiochip_add_data()
devm_request_threaded_irq()
```

### 15.14.2 Protect Shared Resources

```c
struct my_gpio {
    struct mutex lock;  /* Protect register access */
};

mutex_lock(&gpio->lock);
/* Read-modify-write */
mutex_unlock(&gpio->lock);
```

### 15.14.3 Cache Registers

```c
/* Avoid redundant I2C/SPI transactions */
struct mcp23016 {
    u16 cached_output;
    u16 cached_direction;
};
```

---

## Summary

**Chapter 15 Complete!**

You learned:

- GPIO controller architecture
- struct gpio_chip implementation
- IRQ chip integration (legacy & modern)
- Device Tree integration
- Sysfs interface
- Complete working MCP23016 driver
- Best practices