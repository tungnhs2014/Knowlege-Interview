# Part 1. GPIO Controller Architecture

This part covers GPIO controller driver architecture, the gpio_chip structure, callback implementation, and a complete working driver for the MCP23016 I2C GPIO expander.

---

## 15.1 GPIO Controller Overview

### 15.1.1 What is a GPIO Controller?

**A GPIO controller is a hardware device that provides GPIO lines:**

```
┌────────────────────────────────────────────────┐
│  GPIO Controller (Hardware Block)              │
├────────────────────────────────────────────────┤
│                                                │
│  Examples:                                     │
│  • SoC Internal GPIO (memory-mapped)           │
│    - i.MX6 GPIO1, GPIO2, GPIO3...              │
│    - AM335x GPIO0, GPIO1, GPIO2, GPIO3         │
│    - STM32 GPIOA, GPIOB, GPIOC...              │
│                                                │
│  • I2C/SPI GPIO Expanders                      │
│    - MCP23016/MCP23017 (I2C)                   │
│    - MCP23S08/MCP23S17 (SPI)                   │
│    - PCF8574/PCF8575 (I2C)                     │
│                                                │
│  • Other GPIO Expanders                        │
│    - PCA9555 (I2C)                             │
│    - MAX7310/MAX7311 (I2C)                     │
│                                                │
└────────────────────────────────────────────────┘
```

### 15.1.2 GPIO Controller Driver Responsibilities

**A GPIO controller driver must provide:**

```
┌────────────────────────────────────────────────┐
│  GPIO Controller Driver Requirements           │
├────────────────────────────────────────────────┤
│                                                │
│  1. Direction Control:                         │
│     • Set GPIO as input                        │
│     • Set GPIO as output with initial value    │
│     • Get current direction                    │
│                                                │
│  2. Value Access:                              │
│     • Read GPIO state (for inputs)             │
│     • Write GPIO state (for outputs)           │
│     • Support atomic access if needed          │
│                                                │
│  3. IRQ Support (optional):                    │
│     • Map GPIO to IRQ number                   │
│     • Handle interrupt controller features     │
│                                                │
│  4. Additional Features (optional):            │
│     • Debouncing configuration                 │
│     • Pull-up/pull-down control                │
│     • Drive strength settings                  │
│     • Debugfs support                          │
│                                                │
│  5. Sleep Awareness:                           │
│     • Indicate if access may sleep             │
│     • Important for bus-based controllers      │
│                                                │
└────────────────────────────────────────────────┘
```

### 15.1.3 Architecture Overview

```
┌────────────────────────────────────────────────┐
│  GPIO Controller Driver Architecture           │
├────────────────────────────────────────────────┤
│                                                │
│  Consumer Drivers (from Chapter 14)            │
│  ┌──────────────────────────────────────┐      │
│  │ gpio_request(), gpiod_get()          │      │
│  │ gpio_direction_input/output()        │      │
│  │ gpio_get/set_value()                 │      │
│  └─────────────┬────────────────────────┘      │
│                │                               │
│                │ Consumer API                  │
│                ▼                               │
│  ┌─────────────────────────────────────┐       │
│  │  GPIO Core (gpiolib)                │       │
│  │  drivers/gpio/gpiolib.c             │       │
│  │                                     │       │
│  │  - GPIO allocation/tracking         │       │
│  │  - Number mapping                   │       │
│  │  - Sysfs interface                  │       │
│  │  - Consumer API implementation      │       │
│  └─────────────┬───────────────────────┘       │
│                │                               │
│                │ Calls controller callbacks    │
│                ▼                               │
│  ┌─────────────────────────────────────┐       │
│  │  GPIO Controller Driver             │       │
│  │  (This chapter!)                    │       │
│  │                                     │       │
│  │  struct gpio_chip {                 │       │
│  │    .request                         │       │
│  │    .free                            │       │
│  │    .direction_input                 │       │
│  │    .direction_output                │       │
│  │    .get                             │       │
│  │    .set                             │       │
│  │    .to_irq                          │       │
│  │    ...                              │       │
│  │  }                                  │       │
│  └─────────────┬───────────────────────┘       │
│                │                               │
│                │ Hardware access               │
│                ▼                               │
│  ┌─────────────────────────────────────┐       │
│  │  GPIO Controller Hardware           │       │
│  │  (Memory-mapped or Bus)             │       │
│  └─────────────────────────────────────┘       │
│                                                │
└────────────────────────────────────────────────┘
```

---

## 15.2 struct gpio_chip - The Core Structure

### 15.2.1 Required Header

```c
#include <linux/gpio/driver.h>
```

### 15.2.2 Complete Structure Definition

```c
struct gpio_chip {
    /* Basic identification */
    const char              *label;
    struct device           *dev;
    struct module           *owner;

    /* GPIO allocation hooks */
    int  (*request)(struct gpio_chip *chip, unsigned offset);
    void (*free)(struct gpio_chip *chip, unsigned offset);

    /* Direction control */
    int (*get_direction)(struct gpio_chip *chip, unsigned offset);
    int (*direction_input)(struct gpio_chip *chip, unsigned offset);
    int (*direction_output)(struct gpio_chip *chip, unsigned offset,
                            int value);

    /* Value access */
    int  (*get)(struct gpio_chip *chip, unsigned offset);
    void (*set)(struct gpio_chip *chip, unsigned offset, int value);
    void (*set_multiple)(struct gpio_chip *chip,
                         unsigned long *mask, unsigned long *bits);

    /* Additional features */
    int (*set_debounce)(struct gpio_chip *chip, unsigned offset,
                        unsigned debounce);
    int (*set_config)(struct gpio_chip *chip, unsigned offset,
                      unsigned long config);

    /* IRQ support */
    int (*to_irq)(struct gpio_chip *chip, unsigned offset);

    /* GPIO range */
    int              base;      /* First GPIO number */
    u16              ngpio;     /* Number of GPIOs */
    const char       *const *names;  /* GPIO names */

    /* Flags */
    bool             can_sleep;
    bool             irq_not_threaded;
    bool             exported;

    /* IRQ chip support (when CONFIG_GPIOLIB_IRQCHIP) */
#ifdef CONFIG_GPIOLIB_IRQCHIP
    struct irq_chip      *irqchip;
    struct irq_domain    *irqdomain;
    unsigned int         irq_base;
    irq_flow_handler_t   irq_handler;
    unsigned int         irq_default_type;
#endif

    /* Device Tree support (when CONFIG_OF_GPIO) */
#if defined(CONFIG_OF_GPIO)
    struct device_node   *of_node;
    int                  of_gpio_n_cells;
    int (*of_xlate)(struct gpio_chip *gc,
                    const struct of_phandle_args *gpiospec,
                    u32 *flags);
#endif
};
```

### 15.2.3 Field Descriptions

**Basic identification:**

```c
const char *label;
```

- Human-readable name for this GPIO controller
- Used in debug output and sysfs
- Example: "mcp23016", "gpio-controller"

```c
struct device *dev;
```

- Parent device (I2C client, platform device, etc.)
- Used for device-managed resources

```c
struct module *owner;
```

- Module that owns this driver
- Usually set to `THIS_MODULE`

**Allocation hooks (optional):**

```c
int (*request)(struct gpio_chip *chip, unsigned offset);
```

- Called when GPIO is requested via `gpio_request()` or `gpiod_get()`
- Can perform chip-specific activation
- Return 0 on success, negative error code on failure
- If NULL, GPIO core uses default handler

```c
void (*free)(struct gpio_chip *chip, unsigned offset);
```

- Called when GPIO is released via `gpio_free()` or `gpiod_put()`
- Performs chip-specific deactivation
- If NULL, GPIO core uses default handler

**Direction control (mandatory):**

```c
int (*get_direction)(struct gpio_chip *chip, unsigned offset);
```

- Returns current direction of GPIO
- Return values:
    - 0 = output (GPIOF_DIR_OUT)
    - 1 = input (GPIOF_DIR_IN)
    - Negative = error

```c
int (*direction_input)(struct gpio_chip *chip, unsigned offset);
```

- Configure GPIO as input
- Return 0 on success, negative error code on failure
- **Must be implemented!**

```c
int (*direction_output)(struct gpio_chip *chip, unsigned offset,
                        int value);
```

- Configure GPIO as output with initial value
- `value`: Initial state (0 = low, non-zero = high)
- Return 0 on success, negative error code on failure
- **Must be implemented!**

**Value access (mandatory):**

```c
int (*get)(struct gpio_chip *chip, unsigned offset);
```

- Read current GPIO value
- For outputs: return actual sensed value or cached value
- For inputs: return current state
- Return value: 0 or 1
- **Must be implemented!**

```c
void (*set)(struct gpio_chip *chip, unsigned offset, int value);
```

- Set GPIO output value
- `value`: 0 = low, non-zero = high
- Should only be called for outputs
- **Must be implemented!**

```c
void (*set_multiple)(struct gpio_chip *chip,
                     unsigned long *mask, unsigned long *bits);
```

- Set multiple GPIO values atomically (optional)
- `mask`: Bitmap of GPIOs to modify
- `bits`: Desired values for masked GPIOs
- If NULL, core provides generic implementation

**Additional features (optional):**

```c
int (*set_debounce)(struct gpio_chip *chip, unsigned offset,
                    unsigned debounce);
```

- Set debounce time in milliseconds
- Only useful for inputs
- Return 0 on success, -ENOTSUPP if not supported

```c
int (*set_config)(struct gpio_chip *chip, unsigned offset,
                  unsigned long config);
```

- Generic configuration interface
- Can set pull-up/down, drive strength, etc.
- Config format defined by kernel

**IRQ support (optional):**

```c
int (*to_irq)(struct gpio_chip *chip, unsigned offset);
```

- Map GPIO offset to IRQ number
- Called by `gpio_to_irq()` or `gpiod_to_irq()`
- Return Linux IRQ number (virq) or negative error
- **Must not sleep!**

**GPIO range:**

```c
int base;
```

- First GPIO number for this controller
- Set to -1 for dynamic allocation (recommended)
- Example: If base=64, GPIOs are 64 to (64 + ngpio - 1)

```c
u16 ngpio;
```

- Number of GPIOs this controller provides
- Example: 16 for MCP23016, 32 for typical SoC GPIO bank

```c
const char *const *names;
```

- Optional array of GPIO names
- Size must be `ngpio`
- NULL entries allowed for unnamed GPIOs
- Used in debugfs and sysfs

**Flags:**

```c
bool can_sleep;
```

- **CRITICAL FLAG!**
- Set to `true` if GPIO access may sleep
- True for I2C/SPI expanders (bus access sleeps)
- False for memory-mapped controllers
- Affects which consumer APIs can be used

```c
bool irq_not_threaded;
```

- Set if `can_sleep` is true but IRQs don't need threading
- Rare case

```c
bool exported;
```

- Internal flag, managed by GPIO core

### 15.2.4 Offset vs GPIO Number

**Understanding the relationship:**

```
┌────────────────────────────────────────────────┐
│  GPIO Numbering: Offset vs Global Number       │
├────────────────────────────────────────────────┤
│                                                │
│  GPIO Controller with base=64, ngpio=16:       │
│                                                │
│  Offset (chip-relative):  0  1  2 ... 14  15   │
│                           │  │  │      │   │   │
│                           ▼  ▼  ▼      ▼   ▼   │
│  Global GPIO number:     64 65 66 ... 78  79   │
│                                                │
│  Formula:                                      │
│    global_number = base + offset               │
│    offset = global_number - base               │
│                                                │
│  In callbacks:                                 │
│    • Always receive OFFSET (0 to ngpio-1)      │
│    • Never receive global GPIO number          │
│    • Core handles the conversion               │
│                                                │
└────────────────────────────────────────────────┘
```

**Example:**

```c
/* Consumer code (uses global GPIO number) */
gpio_request(67, "my-gpio");           /* Global number 67 */
gpio_direction_input(67);

/* Inside controller driver (receives offset) */
static int my_direction_input(struct gpio_chip *chip, unsigned offset)
{
    /* offset = 67 - 64 = 3 */
    pr_info("Setting GPIO offset %u as input\n", offset);

    /* Use offset to access hardware */
    /* DO NOT use global GPIO number here! */

    return 0;
}
```

---

## 15.3 GPIO Controller Registration

### 15.3.1 Registration Functions

```c
/* Register GPIO controller */
int gpiochip_add(struct gpio_chip *chip);
int gpiochip_add_data(struct gpio_chip *chip, void *data);

/* Unregister GPIO controller */
void gpiochip_remove(struct gpio_chip *chip);

/* Resource-managed variants */
int devm_gpiochip_add_data(struct device *dev,
                           struct gpio_chip *chip,
                           void *data);
```

### 15.3.2 Registration Process

```c
static int my_gpio_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
    struct my_gpio *gpio;
    int ret;

    /* Allocate private structure */
    gpio = devm_kzalloc(&client->dev, sizeof(*gpio), GFP_KERNEL);
    if (!gpio)
        return -ENOMEM;

    /* Setup gpio_chip */
    gpio->chip.label = "my-gpio";
    gpio->chip.dev = &client->dev;
    gpio->chip.owner = THIS_MODULE;

    gpio->chip.base = -1;              /* Dynamic allocation */
    gpio->chip.ngpio = 16;             /* 16 GPIOs */
    gpio->chip.can_sleep = true;       /* I2C access sleeps */

    /* Mandatory callbacks */
    gpio->chip.direction_input = my_direction_input;
    gpio->chip.direction_output = my_direction_output;
    gpio->chip.get = my_get_value;
    gpio->chip.set = my_set_value;

    /* Optional callbacks */
    gpio->chip.get_direction = my_get_direction;
    gpio->chip.to_irq = my_to_irq;

    /* Register with GPIO core */
    ret = devm_gpiochip_add_data(&client->dev, &gpio->chip, gpio);
    if (ret) {
        dev_err(&client->dev, "Failed to register GPIO chip\n");
        return ret;
    }

    dev_info(&client->dev, "GPIO chip registered: base=%d, ngpio=%d\n",
             gpio->chip.base, gpio->chip.ngpio);

    return 0;
}
```

### 15.3.3 Getting Private Data

**Using `gpiochip_get_data()`:**

```c
static int my_get_value(struct gpio_chip *chip, unsigned offset)
{
    struct my_gpio *gpio = gpiochip_get_data(chip);

    /* Now we can access private data */
    return read_register(gpio, offset);
}
```

**Using `container_of()` (alternative):**

```c
struct my_gpio {
    struct i2c_client *client;
    struct gpio_chip chip;    /* Embedded structure */
    /* Other private data */
};

static inline struct my_gpio *to_my_gpio(struct gpio_chip *chip)
{
    return container_of(chip, struct my_gpio, chip);
}

static int my_get_value(struct gpio_chip *chip, unsigned offset)
{
    struct my_gpio *gpio = to_my_gpio(chip);

    /* Access private data */
    return i2c_smbus_read_byte_data(gpio->client, offset);
}
```

---

## 15.4 Callback Implementation Guide

### 15.4.1 direction_input Implementation

```c
static int my_direction_input(struct gpio_chip *chip, unsigned offset)
{
    struct my_gpio *gpio = gpiochip_get_data(chip);
    int ret;

    /* Validate offset */
    if (offset >= chip->ngpio)
        return -EINVAL;

    /* Hardware-specific code to set direction */
    ret = my_hw_set_direction(gpio, offset, GPIO_DIR_INPUT);
    if (ret)
        return ret;

    dev_dbg(chip->dev, "GPIO %u set as input\n", offset);

    return 0;
}
```

### 15.4.2 direction_output Implementation

```c
static int my_direction_output(struct gpio_chip *chip,
                                unsigned offset, int value)
{
    struct my_gpio *gpio = gpiochip_get_data(chip);
    int ret;

    /* Validate offset */
    if (offset >= chip->ngpio)
        return -EINVAL;

    /* Set output value first (avoid glitch) */
    ret = my_hw_set_value(gpio, offset, value);
    if (ret)
        return ret;

    /* Then set direction to output */
    ret = my_hw_set_direction(gpio, offset, GPIO_DIR_OUTPUT);
    if (ret)
        return ret;

    dev_dbg(chip->dev, "GPIO %u set as output, value=%d\n",
            offset, value);

    return 0;
}
```

### 15.4.3 get_direction Implementation

```c
static int my_get_direction(struct gpio_chip *chip, unsigned offset)
{
    struct my_gpio *gpio = gpiochip_get_data(chip);
    u8 direction;

    direction = my_hw_get_direction(gpio, offset);

    /* Return 0 for output, 1 for input */
    return (direction == GPIO_DIR_INPUT) ? 1 : 0;
}
```

### 15.4.4 get Value Implementation

```c
static int my_get_value(struct gpio_chip *chip, unsigned offset)
{
    struct my_gpio *gpio = gpiochip_get_data(chip);
    int value;

    value = my_hw_read_value(gpio, offset);

    /* Return 0 or 1 */
    return value ? 1 : 0;
}
```

### 15.4.5 set Value Implementation

```c
static void my_set_value(struct gpio_chip *chip,
                         unsigned offset, int value)
{
    struct my_gpio *gpio = gpiochip_get_data(chip);

    my_hw_write_value(gpio, offset, value);

    /* No return value for set callback */
}
```

### 15.4.6 set_multiple Implementation

```c
static void my_set_multiple(struct gpio_chip *chip,
                            unsigned long *mask,
                            unsigned long *bits)
{
    struct my_gpio *gpio = gpiochip_get_data(chip);
    unsigned int i;
    u16 output_val = 0;

    /* Read current output state */
    output_val = my_hw_read_output_register(gpio);

    /* Modify bits according to mask */
    for (i = 0; i < chip->ngpio; i++) {
        if (test_bit(i, mask)) {
            if (test_bit(i, bits))
                output_val |= (1 << i);    /* Set bit */
            else
                output_val &= ~(1 << i);   /* Clear bit */
        }
    }

    /* Write back atomically */
    my_hw_write_output_register(gpio, output_val);
}
```

---

## 15.5 Complete MCP23016 I2C GPIO Expander Driver

### 15.5.1 Hardware Overview

**MCP23016 Features:**

- 16-bit I/O expander
- I2C interface (address 0x20-0x27)
- Two 8-bit ports (GP0, GP1)
- Configurable pull-ups
- Interrupt support

**Register Map:**

```
┌────────────────────────────────────────────────┐
│  MCP23016 Register Map                         │
├────────────────────────────────────────────────┤
│                                                │
│  Address  Name        Description              │
│  ------   ----------  ----------------------   │
│  0x00     GP0         Port 0 data              │
│  0x01     GP1         Port 1 data              │
│  0x06     IODIR0      Port 0 direction         │
│  0x07     IODIR1      Port 1 direction         │
│  0x0A     IOPOL0      Port 0 input polarity    │
│  0x0B     IOPOL1      Port 1 input polarity    │
│                                                │
│  Direction: 1 = input, 0 = output              │
│  Polarity:  1 = inverted, 0 = normal           │
│                                                │
└────────────────────────────────────────────────┘
```

### 15.5.2 Complete Driver Code

```c
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio/driver.h>
#include <linux/slab.h>
#include <linux/of.h>

/* MCP23016 Register addresses */
#define MCP23016_GP0        0x00
#define MCP23016_GP1        0x01
#define MCP23016_IODIR0     0x06
#define MCP23016_IODIR1     0x07

#define MCP23016_GPIO_NUM   16

/**
 * struct mcp23016 - MCP23016 device structure
 * @client: I2C client
 * @chip: GPIO chip structure
 * @reg_output: Cached output register values
 * @reg_direction: Cached direction register values
 */
struct mcp23016 {
    struct i2c_client *client;
    struct gpio_chip chip;
    u16 reg_output;
    u16 reg_direction;
};

/*
 * Helper function to convert gpio_chip to mcp23016
 */
static inline struct mcp23016 *to_mcp23016(struct gpio_chip *gc)
{
    return container_of(gc, struct mcp23016, chip);
}

/*
 * Low-level I2C read/write functions
 */
static int mcp23016_read_reg(struct mcp23016 *mcp, u8 reg, u8 *val)
{
    int ret;

    ret = i2c_smbus_read_byte_data(mcp->client, reg);
    if (ret < 0) {
        dev_err(&mcp->client->dev,
                "Failed to read register 0x%02x\n", reg);
        return ret;
    }

    *val = ret;
    return 0;
}

static int mcp23016_write_reg(struct mcp23016 *mcp, u8 reg, u8 val)
{
    int ret;

    ret = i2c_smbus_write_byte_data(mcp->client, reg, val);
    if (ret < 0) {
        dev_err(&mcp->client->dev,
                "Failed to write register 0x%02x\n", reg);
        return ret;
    }

    return 0;
}

/*
 * Read 16-bit value (GP0 + GP1)
 */
static int mcp23016_read_word(struct mcp23016 *mcp, u8 reg, u16 *val)
{
    u8 val_lo, val_hi;
    int ret;

    ret = mcp23016_read_reg(mcp, reg, &val_lo);
    if (ret)
        return ret;

    ret = mcp23016_read_reg(mcp, reg + 1, &val_hi);
    if (ret)
        return ret;

    *val = (u16)val_lo | ((u16)val_hi << 8);
    return 0;
}

/*
 * Write 16-bit value (GP0 + GP1)
 */
static int mcp23016_write_word(struct mcp23016 *mcp, u8 reg, u16 val)
{
    int ret;

    ret = mcp23016_write_reg(mcp, reg, (u8)(val & 0xFF));
    if (ret)
        return ret;

    ret = mcp23016_write_reg(mcp, reg + 1, (u8)(val >> 8));
    if (ret)
        return ret;

    return 0;
}

/*
 * GPIO chip callbacks
 */

static int mcp23016_get_direction(struct gpio_chip *gc, unsigned offset)
{
    struct mcp23016 *mcp = to_mcp23016(gc);

    /* Return 1 for input, 0 for output */
    return (mcp->reg_direction & (1 << offset)) ? 1 : 0;
}

static int mcp23016_direction_input(struct gpio_chip *gc, unsigned offset)
{
    struct mcp23016 *mcp = to_mcp23016(gc);
    int ret;

    dev_dbg(gc->dev, "Set GPIO %u as input\n", offset);

    /* Set bit in direction register (1 = input) */
    mcp->reg_direction |= (1 << offset);

    /* Write to hardware */
    ret = mcp23016_write_word(mcp, MCP23016_IODIR0, mcp->reg_direction);
    if (ret)
        return ret;

    return 0;
}

static int mcp23016_direction_output(struct gpio_chip *gc,
                                      unsigned offset, int value)
{
    struct mcp23016 *mcp = to_mcp23016(gc);
    int ret;

    dev_dbg(gc->dev, "Set GPIO %u as output, value=%d\n", offset, value);

    /* Set output value first (avoid glitch) */
    if (value)
        mcp->reg_output |= (1 << offset);
    else
        mcp->reg_output &= ~(1 << offset);

    ret = mcp23016_write_word(mcp, MCP23016_GP0, mcp->reg_output);
    if (ret)
        return ret;

    /* Clear bit in direction register (0 = output) */
    mcp->reg_direction &= ~(1 << offset);

    ret = mcp23016_write_word(mcp, MCP23016_IODIR0, mcp->reg_direction);
    if (ret)
        return ret;

    return 0;
}

static int mcp23016_get_value(struct gpio_chip *gc, unsigned offset)
{
    struct mcp23016 *mcp = to_mcp23016(gc);
    u16 value;
    int ret;

    /* Read current port values */
    ret = mcp23016_read_word(mcp, MCP23016_GP0, &value);
    if (ret)
        return 0;  /* Return 0 on error */

    return (value & (1 << offset)) ? 1 : 0;
}

static void mcp23016_set_value(struct gpio_chip *gc,
                                unsigned offset, int value)
{
    struct mcp23016 *mcp = to_mcp23016(gc);

    /* Update cached output value */
    if (value)
        mcp->reg_output |= (1 << offset);
    else
        mcp->reg_output &= ~(1 << offset);

    /* Write to hardware */
    mcp23016_write_word(mcp, MCP23016_GP0, mcp->reg_output);
}

static void mcp23016_set_multiple(struct gpio_chip *gc,
                                   unsigned long *mask,
                                   unsigned long *bits)
{
    struct mcp23016 *mcp = to_mcp23016(gc);
    unsigned int i;

    /* Update output register based on mask */
    for (i = 0; i < gc->ngpio; i++) {
        if (test_bit(i, mask)) {
            if (test_bit(i, bits))
                mcp->reg_output |= (1 << i);
            else
                mcp->reg_output &= ~(1 << i);
        }
    }

    /* Write atomically */
    mcp23016_write_word(mcp, MCP23016_GP0, mcp->reg_output);
}

/*
 * I2C driver probe
 */
static int mcp23016_probe(struct i2c_client *client,
                          const struct i2c_device_id *id)
{
    struct mcp23016 *mcp;
    int ret;

    dev_info(&client->dev, "Probing MCP23016 GPIO expander\n");

    /* Check I2C functionality */
    if (!i2c_check_functionality(client->adapter,
                                  I2C_FUNC_SMBUS_BYTE_DATA)) {
        dev_err(&client->dev, "I2C adapter doesn't support required functions\n");
        return -EIO;
    }

    /* Allocate device structure */
    mcp = devm_kzalloc(&client->dev, sizeof(*mcp), GFP_KERNEL);
    if (!mcp)
        return -ENOMEM;

    mcp->client = client;
    i2c_set_clientdata(client, mcp);

    /* Read current register values */
    ret = mcp23016_read_word(mcp, MCP23016_GP0, &mcp->reg_output);
    if (ret) {
        dev_err(&client->dev, "Failed to read output registers\n");
        return ret;
    }

    ret = mcp23016_read_word(mcp, MCP23016_IODIR0, &mcp->reg_direction);
    if (ret) {
        dev_err(&client->dev, "Failed to read direction registers\n");
        return ret;
    }

    /* Setup GPIO chip */
    mcp->chip.label = client->name;
    mcp->chip.dev = &client->dev;
    mcp->chip.owner = THIS_MODULE;

    mcp->chip.base = -1;                    /* Dynamic allocation */
    mcp->chip.ngpio = MCP23016_GPIO_NUM;    /* 16 GPIOs */
    mcp->chip.can_sleep = true;              /* I2C access may sleep */

    /* Assign callbacks */
    mcp->chip.get_direction = mcp23016_get_direction;
    mcp->chip.direction_input = mcp23016_direction_input;
    mcp->chip.direction_output = mcp23016_direction_output;
    mcp->chip.get = mcp23016_get_value;
    mcp->chip.set = mcp23016_set_value;
    mcp->chip.set_multiple = mcp23016_set_multiple;

    /* Device Tree support */
    mcp->chip.of_node = client->dev.of_node;

    /* Register GPIO chip */
    ret = devm_gpiochip_add_data(&client->dev, &mcp->chip, mcp);
    if (ret) {
        dev_err(&client->dev, "Failed to register GPIO chip: %d\n", ret);
        return ret;
    }

    dev_info(&client->dev,
             "MCP23016 registered: base=%d, ngpio=%d\n",
             mcp->chip.base, mcp->chip.ngpio);

    return 0;
}

/*
 * I2C driver remove
 */
static int mcp23016_remove(struct i2c_client *client)
{
    dev_info(&client->dev, "Removing MCP23016 GPIO expander\n");

    /* devm_gpiochip_add_data() handles cleanup automatically */

    return 0;
}

/*
 * Device Tree match table
 */
static const struct of_device_id mcp23016_of_match[] = {
    { .compatible = "microchip,mcp23016" },
    { }
};
MODULE_DEVICE_TABLE(of, mcp23016_of_match);

/*
 * I2C device ID table
 */
static const struct i2c_device_id mcp23016_id[] = {
    { "mcp23016", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, mcp23016_id);

/*
 * I2C driver structure
 */
static struct i2c_driver mcp23016_driver = {
    .driver = {
        .name = "mcp23016",
        .of_match_table = mcp23016_of_match,
    },
    .probe = mcp23016_probe,
    .remove = mcp23016_remove,
    .id_table = mcp23016_id,
};

module_i2c_driver(mcp23016_driver);

MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("MCP23016 I2C GPIO Expander Driver");
MODULE_LICENSE("GPL");
```

### 15.5.3 Device Tree Binding

```
/* I2C bus node */
&i2c1 {
    status = "okay";

    /* MCP23016 GPIO expander */
    gpio_expander: mcp23016@20 {
        compatible = "microchip,mcp23016";
        reg = <0x20>;

        gpio-controller;
        #gpio-cells = <2>;

        /* Optional: specify GPIO names */
        gpio-line-names =
            "EXP_GPIO0", "EXP_GPIO1", "EXP_GPIO2", "EXP_GPIO3",
            "EXP_GPIO4", "EXP_GPIO5", "EXP_GPIO6", "EXP_GPIO7",
            "EXP_GPIO8", "EXP_GPIO9", "EXP_GPIO10", "EXP_GPIO11",
            "EXP_GPIO12", "EXP_GPIO13", "EXP_GPIO14", "EXP_GPIO15";
    };
};

/* Consumer device using expander GPIOs */
leds {
    compatible = "gpio-leds";

    led1 {
        label = "expander-led1";
        gpios = <&gpio_expander 0 GPIO_ACTIVE_HIGH>;
        default-state = "off";
    };

    led2 {
        label = "expander-led2";
        gpios = <&gpio_expander 1 GPIO_ACTIVE_HIGH>;
        default-state = "off";
    };
};

buttons {
    compatible = "gpio-keys";

    button1 {
        label = "Expander Button 1";
        gpios = <&gpio_expander 8 GPIO_ACTIVE_LOW>;
        linux,code = <KEY_1>;
    };
};
```

---

## Summary

In this part, we covered:

**Key Concepts:**

1. **GPIO Controller Overview**
    - What is a GPIO controller
    - Controller responsibilities
    - Architecture overview
2. **struct gpio_chip Structure**
    - Complete field-by-field description
    - Mandatory vs optional callbacks
    - Flags and their meanings
    - Offset vs global GPIO numbering
3. **Controller Registration**
    - gpiochip_add functions
    - Resource-managed variants
    - Getting private data
4. **Callback Implementation**
    - direction_input/output
    - get/set value
    - get_direction
    - set_multiple
5. **Complete Working Driver**
    - MCP23016 I2C GPIO expander
    - Full implementation (~400 lines)
    - I2C communication
    - Register caching
    - Device Tree integration