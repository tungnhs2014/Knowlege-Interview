# Part 2. GPIO Consumer Interfaces

This part covers the GPIO subsystem, including both the legacy integer-based interface and the modern descriptor-based interface, with complete driver examples.

---

## 14.1 GPIO Subsystem Overview

### 14.1.1 What is GPIO?

**From different perspectives:**

```
┌────────────────────────────────────────────────┐
│  GPIO - General Purpose Input/Output           │
├────────────────────────────────────────────────┤
│                                                │
│  Hardware View:                                │
│  - GPIO is a MODE/FUNCTION of a pin            │
│  - One of several functions a pin can have     │
│  - Configured via pin controller               │
│                                                │
│  Software View:                                │
│  - Digital line with two possible states       │
│  - Can be INPUT or OUTPUT                      │
│  - Can read/write HIGH (1) or LOW (0)          │
│                                                │
│  Common Uses:                                  │
│  - Read buttons/switches                       │
│  - Control LEDs                                │
│  - Chip select signals                         │
│  - Reset lines                                 │
│  - Interrupt inputs                            │
│                                                │
└────────────────────────────────────────────────┘
```

### 14.1.2 GPIO Operations

**What you can do with GPIO:**

```
┌────────────────────────────────────────────────┐
│  GPIO Operations                               │
├────────────────────────────────────────────────┤
│                                                │
│  1. Claim Ownership:                           │
│     - Request GPIO from kernel                 │
│     - Prevents conflicts with other drivers    │
│                                                │
│  2. Set Direction:                             │
│     - INPUT: Read external signals             │
│     - OUTPUT: Drive signal high/low            │
│                                                │
│  3. Read/Write Values:                         │
│     - INPUT: Read current state (0 or 1)       │
│     - OUTPUT: Set state (0 or 1)               │
│                                                │
│  4. Configure Properties:                      │
│     - Debounce interval (for inputs)           │
│     - Initial value (for outputs)              │
│     - Active high/low polarity                 │
│                                                │
│  5. Interrupt Handling:                        │
│     - Map GPIO to IRQ number                   │
│     - Configure trigger (edge/level)           │
│     - Register interrupt handler               │
│                                                │
│  6. Release:                                   │
│     - Free GPIO when done                      │
│                                                │
└────────────────────────────────────────────────┘
```

### 14.1.3 Two GPIO Interfaces

**The kernel provides TWO ways to use GPIO:**

```
┌────────────────────────────────────────────────┐
│  GPIO Interface Evolution                      │
├────────────────────────────────────────────────┤
│                                                │
│  Legacy Interface (DEPRECATED):                │
│  ┌──────────────────────────────────────┐      │
│  │ Integer-Based GPIO Interface         │      │
│  │                                      │      │
│  │ - GPIO identified by integer number  │      │
│  │ - Global numbering across system     │      │
│  │ - Simple but limited                 │      │
│  │ - Still widely used in old code      │      │
│  │                                      │      │
│  │ Header: <linux/gpio.h>               │      │
│  │ Functions: gpio_request(), etc.      │      │
│  └──────────────────────────────────────┘      │
│                                                │
│  Modern Interface (RECOMMENDED):               │
│  ┌──────────────────────────────────────┐      │
│  │ Descriptor-Based GPIO Interface      │      │
│  │                                      │      │
│  │ - GPIO described by struct gpio_desc │      │
│  │ - Device Tree integration            │      │
│  │ - Better resource management         │      │
│  │ - Polarity handling (ACTIVE_LOW)     │      │
│  │                                      │      │
│  │ Header: <linux/gpio/consumer.h>      │      │
│  │ Functions: gpiod_get(), etc.         │      │
│  └──────────────────────────────────────┘      │
│                                                │
└────────────────────────────────────────────────┘
```

---

## 14.2 Legacy Integer-Based GPIO Interface

### 14.2.1 Required Header

```c
#include <linux/gpio.h>
```

### 14.2.2 GPIO Numbering

**Integer-based interface uses global GPIO numbers:**

```
┌────────────────────────────────────────────────┐
│  GPIO Numbering Example (AM335x)               │
├────────────────────────────────────────────────┤
│                                                │
│  GPIO Controller 0 (GPIO0):                    │
│  GPIO numbers: 0-31                            │
│  ├── GPIO0_0  = 0                              │
│  ├── GPIO0_1  = 1                              │
│  ├── ...                                       │
│  └── GPIO0_31 = 31                             │
│                                                │
│  GPIO Controller 1 (GPIO1):                    │
│  GPIO numbers: 32-63                           │
│  ├── GPIO1_0  = 32                             │
│  ├── GPIO1_1  = 33                             │
│  ├── ...                                       │
│  └── GPIO1_31 = 63                             │
│                                                │
│  GPIO Controller 2 (GPIO2):                    │
│  GPIO numbers: 64-95                           │
│  ├── GPIO2_0  = 64                             │
│  ├── GPIO2_1  = 65                             │
│  ├── ...                                       │
│  └── GPIO2_31 = 95                             │
│                                                │
│  GPIO Controller 3 (GPIO3):                    │
│  GPIO numbers: 96-127                          │
│  ├── GPIO3_0  = 96                             │
│  ├── ...                                       │
│  └── GPIO3_31 = 127                            │
│                                                │
│  Formula: GPIO_NUMBER = (BANK * 32) + OFFSET   │
│  Example: GPIO1_21 = (1 * 32) + 21 = 53        │
│                                                │
└────────────────────────────────────────────────┘
```

### 14.2.3 Core Functions

**1. Check if GPIO is valid:**

```c
bool gpio_is_valid(int number);
```

**2. Request GPIO (claim ownership):**

```c
int gpio_request(unsigned gpio, const char *label);
```

**3. Free GPIO (release ownership):**

```c
void gpio_free(unsigned gpio);
```

**4. Set direction:**

```c
int gpio_direction_input(unsigned gpio);
int gpio_direction_output(unsigned gpio, int value);
```

**5. Get/Set value:**

```c
int gpio_get_value(unsigned gpio);
void gpio_set_value(unsigned gpio, int value);
```

**6. Set debounce (if supported):**

```c
int gpio_set_debounce(unsigned gpio, unsigned debounce);
```

**7. GPIO to IRQ mapping:**

```c
int gpio_to_irq(unsigned gpio);
```

### 14.2.4 Value Access - Context Matters!

**Non-sleeping context (atomic):**

```c
/* Can be called from interrupt context */
int gpio_get_value(unsigned gpio);
void gpio_set_value(unsigned gpio, int value);
```

**Sleeping context (may sleep):**

```c
/* Cannot be called from interrupt context */
int gpio_get_value_cansleep(unsigned gpio);
void gpio_set_value_cansleep(unsigned gpio, int value);
```

**When to use each:**

```c
/* Check if GPIO access may sleep */
int gpio_cansleep(unsigned gpio);

/* Example usage: */
if (gpio_cansleep(gpio)) {
    /* Use _cansleep variants */
    val = gpio_get_value_cansleep(gpio);
} else {
    /* Use regular variants */
    val = gpio_get_value(gpio);
}
```

**Why the distinction?**

```
┌────────────────────────────────────────────────┐
│  GPIO Access and Sleep                         │
├────────────────────────────────────────────────┤
│                                                │
│  Memory-mapped GPIO (SoC internal):            │
│  - Direct register access                      │
│  - Never sleeps                                │
│  - Use gpio_get_value()                        │
│  - Example: AM335x internal GPIOs              │
│                                                │
│  I2C/SPI GPIO Expander:                        │
│  - Bus transaction required                    │
│  - May sleep                                   │
│  - Use gpio_get_value_cansleep()               │
│  - Example: MCP23017 I2C expander              │
│                                                │
└────────────────────────────────────────────────┘
```

### 14.2.5 Complete Driver Example - LED & Button

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

/* GPIO definitions - BeagleBone Black */
#define GPIO_LED_RED    (1 * 32 + 21)   /* GPIO1_21 = P8.13 */
#define GPIO_LED_GREEN  (1 * 32 + 22)   /* GPIO1_22 = P8.19 */
#define GPIO_BTN1       (1 * 32 + 17)   /* GPIO1_17 = P9.23 */
#define GPIO_BTN2       (1 * 32 + 15)   /* GPIO1_15 = P9.24 */

static int irq;

/* Interrupt handler for button press */
static irqreturn_t btn1_pushed_irq_handler(int irq, void *dev_id)
{
    int state_btn2;

    /* Read button 2 state */
    state_btn2 = gpio_get_value(GPIO_BTN2);

    /* Control LEDs based on button state */
    gpio_set_value(GPIO_LED_RED, state_btn2);
    gpio_set_value(GPIO_LED_GREEN, !state_btn2);

    pr_info("Button 1 pressed! BTN2 state: %d\n", state_btn2);

    return IRQ_HANDLED;
}

static int __init gpio_example_init(void)
{
    int ret;

    pr_info("GPIO Example: Initializing...\n");

    /* Validate GPIOs */
    if (!gpio_is_valid(GPIO_LED_RED)) {
        pr_err("Invalid GPIO: Red LED\n");
        return -ENODEV;
    }

    if (!gpio_is_valid(GPIO_LED_GREEN)) {
        pr_err("Invalid GPIO: Green LED\n");
        return -ENODEV;
    }

    /* Request GPIOs */
    ret = gpio_request(GPIO_LED_RED, "red-led");
    if (ret) {
        pr_err("Failed to request GPIO_LED_RED\n");
        return ret;
    }

    ret = gpio_request(GPIO_LED_GREEN, "green-led");
    if (ret) {
        pr_err("Failed to request GPIO_LED_GREEN\n");
        goto err_green;
    }

    ret = gpio_request(GPIO_BTN1, "button-1");
    if (ret) {
        pr_err("Failed to request GPIO_BTN1\n");
        goto err_btn1;
    }

    ret = gpio_request(GPIO_BTN2, "button-2");
    if (ret) {
        pr_err("Failed to request GPIO_BTN2\n");
        goto err_btn2;
    }

    /* Configure button GPIOs as inputs */
    ret = gpio_direction_input(GPIO_BTN1);
    if (ret) {
        pr_err("Failed to set GPIO_BTN1 as input\n");
        goto err_config;
    }

    ret = gpio_direction_input(GPIO_BTN2);
    if (ret) {
        pr_err("Failed to set GPIO_BTN2 as input\n");
        goto err_config;
    }

    /* Optional: Set debounce (if supported by controller) */
    gpio_set_debounce(GPIO_BTN1, 200);  /* 200ms debounce */

    /* Configure LED GPIOs as outputs with initial value LOW */
    ret = gpio_direction_output(GPIO_LED_RED, 0);
    if (ret) {
        pr_err("Failed to set GPIO_LED_RED as output\n");
        goto err_config;
    }

    ret = gpio_direction_output(GPIO_LED_GREEN, 0);
    if (ret) {
        pr_err("Failed to set GPIO_LED_GREEN as output\n");
        goto err_config;
    }

    /* Map GPIO to IRQ */
    irq = gpio_to_irq(GPIO_BTN1);
    if (irq < 0) {
        pr_err("Failed to get IRQ for GPIO_BTN1\n");
        ret = irq;
        goto err_config;
    }

    pr_info("GPIO_BTN1 mapped to IRQ %d\n", irq);

    /* Request IRQ */
    ret = request_threaded_irq(irq, NULL,
                               btn1_pushed_irq_handler,
                               IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                               "gpio-btn1-irq", NULL);
    if (ret) {
        pr_err("Failed to request IRQ\n");
        goto err_config;
    }

    pr_info("GPIO Example: Initialized successfully\n");
    pr_info("  Red LED: GPIO %d\n", GPIO_LED_RED);
    pr_info("  Green LED: GPIO %d\n", GPIO_LED_GREEN);
    pr_info("  Button 1: GPIO %d (IRQ %d)\n", GPIO_BTN1, irq);
    pr_info("  Button 2: GPIO %d\n", GPIO_BTN2);

    return 0;

err_config:
    gpio_free(GPIO_BTN2);
err_btn2:
    gpio_free(GPIO_BTN1);
err_btn1:
    gpio_free(GPIO_LED_GREEN);
err_green:
    gpio_free(GPIO_LED_RED);
    return ret;
}

static void __exit gpio_example_exit(void)
{
    pr_info("GPIO Example: Cleaning up...\n");

    /* Free IRQ */
    free_irq(irq, NULL);

    /* Turn off LEDs */
    gpio_set_value(GPIO_LED_RED, 0);
    gpio_set_value(GPIO_LED_GREEN, 0);

    /* Free all GPIOs */
    gpio_free(GPIO_LED_RED);
    gpio_free(GPIO_LED_GREEN);
    gpio_free(GPIO_BTN1);
    gpio_free(GPIO_BTN2);

    pr_info("GPIO Example: Cleanup complete\n");
}

module_init(gpio_example_init);
module_exit(gpio_example_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("GPIO Example - Legacy Integer-Based Interface");
```

### 14.2.6 Device Tree Integration (Legacy)

**Required header:**

```c
#include <linux/of_gpio.h>
```

**Functions to extract GPIO numbers from DT:**

```c
/* Get GPIO count in property */
int of_gpio_count(struct device_node *np);
int of_get_named_gpio_count(struct device_node *np,
                             const char *propname);

/* Get GPIO number at index */
int of_get_gpio(struct device_node *np, int index);
int of_get_named_gpio(struct device_node *np,
                      const char *propname, int index);
```

**Device Tree example:**

```
my_device {
    compatible = "vendor,my-device";

    /* Old style: gpios property */
    gpios = <&gpio1 2 0>,   /* INT */
            <&gpio1 5 0>;   /* RST */

    /* New style: named properties */
    led-gpios = <&gpio2 15 GPIO_ACTIVE_HIGH>,  /* red */
                <&gpio2 16 GPIO_ACTIVE_HIGH>;  /* green */

    btn-gpios = <&gpio2 1 GPIO_ACTIVE_LOW>;
};
```

**Driver code:**

```c
static int my_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    int gpio_int, gpio_rst;
    int gpio_red, gpio_green;
    int n_gpios;
    int ret;

    if (!np)
        return -ENOENT;

    /* Method 1: Unnamed property "gpios" */
    n_gpios = of_gpio_count(np);
    pr_info("Found %d GPIOs in 'gpios' property\n", n_gpios);

    gpio_int = of_get_gpio(np, 0);  /* First GPIO */
    gpio_rst = of_get_gpio(np, 1);  /* Second GPIO */

    if (!gpio_is_valid(gpio_int)) {
        dev_err(&pdev->dev, "Invalid INT GPIO\n");
        return -EINVAL;
    }

    if (!gpio_is_valid(gpio_rst)) {
        dev_err(&pdev->dev, "Invalid RST GPIO\n");
        return -EINVAL;
    }

    /* Request GPIOs */
    ret = gpio_request(gpio_int, "int-gpio");
    if (ret)
        return ret;

    ret = gpio_request(gpio_rst, "rst-gpio");
    if (ret) {
        gpio_free(gpio_int);
        return ret;
    }

    /* Configure */
    gpio_direction_input(gpio_int);
    gpio_direction_output(gpio_rst, 1);

    /* Method 2: Named property "led-gpios" */
    gpio_red = of_get_named_gpio(np, "led", 0);
    gpio_green = of_get_named_gpio(np, "led", 1);

    if (gpio_is_valid(gpio_red)) {
        gpio_request(gpio_red, "red-led");
        gpio_direction_output(gpio_red, 0);
    }

    if (gpio_is_valid(gpio_green)) {
        gpio_request(gpio_green, "green-led");
        gpio_direction_output(gpio_green, 0);
    }

    return 0;
}
```

---

## 14.3 Modern Descriptor-Based GPIO Interface

### 14.3.1 Required Header

```c
#include <linux/gpio/consumer.h>
```

### 14.3.2 The gpio_desc Structure

**GPIOs are represented by an opaque structure:**

```c
struct gpio_desc {
    struct gpio_chip *chip;     /* Controller providing this GPIO */
    unsigned long flags;        /* Configuration flags */
    const char *label;          /* Human-readable name */
};
```

**You don't access fields directly - use API functions!**

### 14.3.3 GPIO Descriptor Mapping in Device Tree

**GPIO properties must be named `<name>-gpios` or `<name>-gpio`:**

```
my_device {
    compatible = "vendor,my-device";

    /* Correct naming: */
    led-gpios = <&gpio2 15 GPIO_ACTIVE_HIGH>,    /* Red */
                <&gpio2 16 GPIO_ACTIVE_HIGH>;    /* Green */

    power-gpio = <&gpio1 5 GPIO_ACTIVE_LOW>;

    reset-gpios = <&gpio3 10 GPIO_ACTIVE_HIGH>;

    /* Multiple buttons */
    btn-gpios = <&gpio2 1 GPIO_ACTIVE_LOW>,      /* btn1 */
                <&gpio2 31 GPIO_ACTIVE_LOW>;     /* btn2 */
};
```

**Flags (second cell):**

```c
/* From include/dt-bindings/gpio/gpio.h */

#define GPIO_ACTIVE_HIGH 0
#define GPIO_ACTIVE_LOW  1

#define GPIO_PUSH_PULL   0
#define GPIO_OPEN_DRAIN  (1 << 1)
#define GPIO_OPEN_SOURCE (1 << 2)
```

### 14.3.4 Initialization Flags

**When requesting GPIO, you can specify initial configuration:**

```c
enum gpiod_flags {
    GPIOD_ASIS           = 0,      /* Don't change anything */
    GPIOD_IN             = ...,    /* Configure as input */
    GPIOD_OUT_LOW        = ...,    /* Output, initial value LOW */
    GPIOD_OUT_HIGH       = ...,    /* Output, initial value HIGH */
    GPIOD_OUT_LOW_OPEN_DRAIN  = ...,
    GPIOD_OUT_HIGH_OPEN_DRAIN = ...,
};
```

### 14.3.5 Core Functions

**1. Get GPIO descriptor:**

```c
/* Get single GPIO (index 0) */
struct gpio_desc *gpiod_get(struct device *dev,
                             const char *con_id,
                             enum gpiod_flags flags);

/* Get GPIO by index */
struct gpio_desc *gpiod_get_index(struct device *dev,
                                   const char *con_id,
                                   unsigned int idx,
                                   enum gpiod_flags flags);

/* Optional GPIO (returns NULL if not found) */
struct gpio_desc *gpiod_get_optional(struct device *dev,
                                      const char *con_id,
                                      enum gpiod_flags flags);
```

**2. Release GPIO descriptor:**

```c
void gpiod_put(struct gpio_desc *desc);
```

**3. Set direction:**

```c
int gpiod_direction_input(struct gpio_desc *desc);
int gpiod_direction_output(struct gpio_desc *desc, int value);
int gpiod_direction_output_raw(struct gpio_desc *desc, int value);
```

**4. Get/Set value:**

```c
/* Considers ACTIVE_LOW flag */
int gpiod_get_value(const struct gpio_desc *desc);
void gpiod_set_value(struct gpio_desc *desc, int value);

/* Raw value (ignores ACTIVE_LOW) */
int gpiod_get_raw_value(const struct gpio_desc *desc);
void gpiod_set_raw_value(struct gpio_desc *desc, int value);
```

**5. Sleeping variants:**

```c
int gpiod_cansleep(const struct gpio_desc *desc);

int gpiod_get_value_cansleep(const struct gpio_desc *desc);
void gpiod_set_value_cansleep(struct gpio_desc *desc, int value);

int gpiod_get_raw_value_cansleep(const struct gpio_desc *desc);
void gpiod_set_raw_value_cansleep(struct gpio_desc *desc, int value);
```

**6. Other functions:**

```c
int gpiod_set_debounce(struct gpio_desc *desc, unsigned debounce);
int gpiod_is_active_low(const struct gpio_desc *desc);
int gpiod_to_irq(const struct gpio_desc *desc);
int gpiod_get_direction(struct gpio_desc *desc);
```

### 14.3.6 Complete Driver Example - LED & Button

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/of.h>

/*
 * Device Tree:
 *
 * gpio_device {
 *     compatible = "vendor,gpio-descriptor-sample";
 *
 *     led-gpios = <&gpio2 15 GPIO_ACTIVE_HIGH>,  // red
 *                 <&gpio2 16 GPIO_ACTIVE_HIGH>;  // green
 *
 *     btn-gpios = <&gpio2 1 GPIO_ACTIVE_LOW>,    // btn1
 *                 <&gpio2 31 GPIO_ACTIVE_LOW>;   // btn2
 * };
 */

struct gpio_device {
    struct gpio_desc *red;
    struct gpio_desc *green;
    struct gpio_desc *btn1;
    struct gpio_desc *btn2;
    int irq;
};

static irqreturn_t btn1_irq_handler(int irq, void *dev_id)
{
    struct gpio_device *priv = dev_id;
    int state;

    /* Read button 2 state */
    state = gpiod_get_value(priv->btn2);

    /* Control LEDs - polarity handled automatically! */
    gpiod_set_value(priv->red, state);
    gpiod_set_value(priv->green, !state);

    pr_info("Button 1 interrupt! BTN2 state: %d\n", state);

    return IRQ_HANDLED;
}

static int gpio_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct gpio_device *priv;
    int ret;

    dev_info(dev, "Probing GPIO device\n");

    /* Allocate private data */
    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    /*
     * Get GPIO descriptors with initial configuration
     *
     * gpiod_get_index() combines:
     * 1. Getting descriptor
     * 2. Setting direction
     * 3. Setting initial value
     */

    /* Get LED GPIOs - configure as output with initial LOW */
    priv->red = gpiod_get_index(dev, "led", 0, GPIOD_OUT_LOW);
    if (IS_ERR(priv->red)) {
        dev_err(dev, "Failed to get red LED GPIO\n");
        return PTR_ERR(priv->red);
    }

    priv->green = gpiod_get_index(dev, "led", 1, GPIOD_OUT_LOW);
    if (IS_ERR(priv->green)) {
        ret = PTR_ERR(priv->green);
        goto err_green;
    }

    /* Get button GPIOs - configure as input */
    priv->btn1 = gpiod_get_index(dev, "btn", 0, GPIOD_IN);
    if (IS_ERR(priv->btn1)) {
        ret = PTR_ERR(priv->btn1);
        goto err_btn1;
    }

    priv->btn2 = gpiod_get_index(dev, "btn", 1, GPIOD_IN);
    if (IS_ERR(priv->btn2)) {
        ret = PTR_ERR(priv->btn2);
        goto err_btn2;
    }

    /* Optional: Set debounce if controller supports it */
    ret = gpiod_set_debounce(priv->btn1, 200);
    if (ret)
        dev_warn(dev, "Debounce not supported\n");

    /* Map GPIO to IRQ */
    priv->irq = gpiod_to_irq(priv->btn1);
    if (priv->irq < 0) {
        ret = priv->irq;
        dev_err(dev, "Failed to get IRQ\n");
        goto err_irq;
    }

    /* Request IRQ */
    ret = request_threaded_irq(priv->irq, NULL,
                               btn1_irq_handler,
                               IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                               "gpio-btn1", priv);
    if (ret) {
        dev_err(dev, "Failed to request IRQ\n");
        goto err_irq;
    }

    platform_set_drvdata(pdev, priv);

    dev_info(dev, "GPIO device probed successfully\n");
    dev_info(dev, "  Red LED active_low: %d\n",
             gpiod_is_active_low(priv->red));
    dev_info(dev, "  Button 1 IRQ: %d\n", priv->irq);

    return 0;

err_irq:
    gpiod_put(priv->btn2);
err_btn2:
    gpiod_put(priv->btn1);
err_btn1:
    gpiod_put(priv->green);
err_green:
    gpiod_put(priv->red);
    return ret;
}

static int gpio_remove(struct platform_device *pdev)
{
    struct gpio_device *priv = platform_get_drvdata(pdev);

    dev_info(&pdev->dev, "Removing GPIO device\n");

    /* Free IRQ */
    free_irq(priv->irq, priv);

    /* Turn off LEDs */
    gpiod_set_value(priv->red, 0);
    gpiod_set_value(priv->green, 0);

    /* Release all GPIOs */
    gpiod_put(priv->red);
    gpiod_put(priv->green);
    gpiod_put(priv->btn1);
    gpiod_put(priv->btn2);

    return 0;
}

static const struct of_device_id gpio_of_match[] = {
    { .compatible = "vendor,gpio-descriptor-sample" },
    { }
};
MODULE_DEVICE_TABLE(of, gpio_of_match);

static struct platform_driver gpio_driver = {
    .probe = gpio_probe,
    .remove = gpio_remove,
    .driver = {
        .name = "gpio-descriptor-sample",
        .of_match_table = gpio_of_match,
    },
};

module_platform_driver(gpio_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("GPIO Descriptor-Based Interface Example");
```

### 14.3.7 Resource-Managed Variants

*Using devm_ for automatic cleanup:**

```c
struct gpio_desc *devm_gpiod_get(struct device *dev,
                                  const char *con_id,
                                  enum gpiod_flags flags);

struct gpio_desc *devm_gpiod_get_index(struct device *dev,
                                        const char *con_id,
                                        unsigned int idx,
                                        enum gpiod_flags flags);

struct gpio_desc *devm_gpiod_get_optional(struct device *dev,
                                           const char *con_id,
                                           enum gpiod_flags flags);
```

**Automatic cleanup on driver detach!**

```c
static int gpio_probe(struct platform_device *pdev)
{
    struct gpio_desc *led;

    /* Automatically freed on remove/error! */
    led = devm_gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
    if (IS_ERR(led))
        return PTR_ERR(led);

    /* Use GPIO... */

    /* No need to call gpiod_put() in remove! */

    return 0;
}
```

### 14.3.8 Improved Driver with devm_*

```c
static int gpio_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct gpio_device *priv;
    int ret;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    /* All these are automatically freed on error/remove! */
    priv->red = devm_gpiod_get_index(dev, "led", 0, GPIOD_OUT_LOW);
    if (IS_ERR(priv->red))
        return PTR_ERR(priv->red);

    priv->green = devm_gpiod_get_index(dev, "led", 1, GPIOD_OUT_LOW);
    if (IS_ERR(priv->green))
        return PTR_ERR(priv->green);

    priv->btn1 = devm_gpiod_get_index(dev, "btn", 0, GPIOD_IN);
    if (IS_ERR(priv->btn1))
        return PTR_ERR(priv->btn1);

    priv->btn2 = devm_gpiod_get_index(dev, "btn", 1, GPIOD_IN);
    if (IS_ERR(priv->btn2))
        return PTR_ERR(priv->btn2);

    priv->irq = gpiod_to_irq(priv->btn1);
    if (priv->irq < 0)
        return priv->irq;

    ret = devm_request_threaded_irq(dev, priv->irq, NULL,
                                     btn1_irq_handler,
                                     IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                                     "gpio-btn1", priv);
    if (ret)
        return ret;

    platform_set_drvdata(pdev, priv);

    return 0;
}

static int gpio_remove(struct platform_device *pdev)
{
    /* Everything cleaned up automatically! */
    dev_info(&pdev->dev, "Driver removed\n");
    return 0;
}
```

### 14.3.9 Conversion Between Interfaces

**You can convert between descriptor and integer:**

```c
/* Descriptor → Integer */
int desc_to_gpio(const struct gpio_desc *desc);

/* Integer → Descriptor */
struct gpio_desc *gpio_to_desc(unsigned gpio);
```

**Example:**

```c
struct gpio_desc *desc;
int gpio_num;

desc = gpiod_get(&pdev->dev, "reset", GPIOD_OUT_HIGH);

/* Get integer number */
gpio_num = desc_to_gpio(desc);
pr_info("GPIO descriptor corresponds to GPIO %d\n", gpio_num);

/* Convert back */
struct gpio_desc *desc2 = gpio_to_desc(gpio_num);
```

---

## 14.4 GPIO Polarity Handling

### 14.4.1 ACTIVE_LOW vs ACTIVE_HIGH

**Descriptor-based interface handles polarity automatically!**

```
┌────────────────────────────────────────────────┐
│  GPIO Polarity Handling                        │
├────────────────────────────────────────────────┤
│                                                │
│  ACTIVE_HIGH (normal):                         │
│  Logical 1 → Physical HIGH voltage             │
│  Logical 0 → Physical LOW voltage              │
│                                                │
│  ACTIVE_LOW (inverted):                        │
│  Logical 1 → Physical LOW voltage              │
│  Logical 0 → Physical HIGH voltage             │
│                                                │
│  Common use: LEDs, buttons with pull-ups       │
│                                                │
└────────────────────────────────────────────────┘
```

**Device Tree:**

```
leds {
    compatible = "gpio-leds";

    led1 {
        label = "red";
        gpios = <&gpio1 5 GPIO_ACTIVE_HIGH>;  /* Normal */
    };

    led2 {
        label = "green";
        gpios = <&gpio1 6 GPIO_ACTIVE_LOW>;   /* Inverted */
    };
};
```

**Driver code:**

```c
struct gpio_desc *led1, *led2;

/* Get descriptors */
led1 = gpiod_get_index(dev, "led", 0, GPIOD_OUT_LOW);
led2 = gpiod_get_index(dev, "led", 1, GPIOD_OUT_LOW);

/* Turn on both LEDs - polarity handled automatically! */
gpiod_set_value(led1, 1);  /* Physical HIGH */
gpiod_set_value(led2, 1);  /* Physical LOW (because ACTIVE_LOW) */

/* Check polarity */
if (gpiod_is_active_low(led2))
    pr_info("LED2 is active low\n");
```

**With raw access (ignores polarity):**

```c
/* Force physical state regardless of ACTIVE_LOW flag */
gpiod_set_raw_value(led2, 1);  /* Physical HIGH */
```

---

## Summary

In this part, we covered:

**Key Concepts:**

1. **GPIO Subsystem Overview**
    - Hardware vs software perspective
    - Basic GPIO operations
    - Two interfaces available
2. **Legacy Integer-Based Interface**
    - Global GPIO numbering
    - gpio_request/free functions
    - Direction and value control
    - GPIO to IRQ mapping
    - Device Tree integration
    - Complete working example
3. **Modern Descriptor-Based Interface**
    - struct gpio_desc representation
    - Device Tree naming convention
    - Initialization flags
    - gpiod_get/put functions
    - Direction and value control
    - Resource-managed variants
    - Complete working example
4. **Context Awareness**
    - Regular vs _cansleep variants
    - When GPIO access may sleep
    - Proper function selection
5. **Polarity Handling**
    - ACTIVE_HIGH vs ACTIVE_LOW
    - Automatic handling in descriptor API
    - Raw value access
6. **Best Practices**
    - Use descriptor-based API for new code
    - Use devm_* for automatic cleanup
    - Handle errors properly
    - Check gpio_cansleep() before access