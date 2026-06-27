# Part 2. IRQ Multiplexing - Chained and Nested Interrupts

This part covers IRQ multiplexing in detail, including chained interrupts, nested interrupts, and complete implementation examples for both approaches.

---

## 16.5 Chained vs Nested Interrupts

### 16.5.1 The Two Approaches

**Understanding the fundamental difference:**

```
┌────────────────────────────────────────────────┐
│  Chained vs Nested Interrupts                  │
├────────────────────────────────────────────────┤
│                                                │
│  CHAINED INTERRUPTS:                           │
│  ┌──────────────────────────────────────┐      │
│  │ Use case: Fast controllers           │      │
│  │ • Memory-mapped GPIO (SoC internal)  │      │
│  │ • Direct register access             │      │
│  │ • Never sleeps                       │      │
│  │                                      │      │
│  │ Execution model:                     │      │
│  │ Parent Handler (Hard IRQ context)    │      │
│  │   ├─ Read status register            │      │
│  │   ├─ Find active interrupts          │      │
│  │   ├─ Call generic_handle_irq()       │      │
│  │   └─ Child handler runs INLINE       │      │ 
│  │       (still in hard IRQ context!)   │      │
│  │                                      │      │
│  │ Key function: generic_handle_irq()   │      │
│  └──────────────────────────────────────┘      │
│                                                │
│  NESTED INTERRUPTS:                            │
│  ┌──────────────────────────────────────┐      │
│  │ Use case: Slow controllers           │      │
│  │ • I2C/SPI GPIO expanders             │      │
│  │ • Bus access required                │      │
│  │ • May sleep!                         │      │
│  │                                      │      │
│  │ Execution model:                     │      │
│  │ Parent Handler (Thread context)      │      │
│  │   ├─ Read status via I2C (sleeps!)   │      │
│  │   ├─ Find active interrupts          │      │
│  │   ├─ Call handle_nested_irq()        │      │
│  │   └─ Child handler runs in THREAD    │      │
│  │       (can sleep, can be preempted)  │      │
│  │                                      │      │
│  │ Key function: handle_nested_irq()    │      │
│  └──────────────────────────────────────┘      │
│                                                │
└────────────────────────────────────────────────┘
```

### 16.5.2 Detailed Comparison

```
┌──────────────────────────────────────────────────────────────┐
│  Chained vs Nested: Complete Comparison                      │
├──────────────────┬──────────────────┬────────────────────────┤
│  Feature         │  Chained         │  Nested                │
├──────────────────┼──────────────────┼────────────────────────┤
│  Controller type │  Memory-mapped   │  Bus-based (I2C/SPI)   │
│  Access speed    │  Fast (MMIO)     │  Slow (I2C)            │
│  Can sleep?      │  NO              │  YES                   │
│  Context         │  Hard IRQ        │  Thread                │
│  Preemptible?    │  NO              │  YES                   │
│  IRQs enabled?   │  Disabled        │  Enabled               │
│  Handler call    │  generic_handle  │  handle_nested_irq     │
│  Registration    │  irq_set_chained │  request_threaded_irq  │
│  Parent setup    │  _handler_and_   │  IRQF_ONESHOT          │
│                  │  data()          │                        │
│  Child config    │  Standard        │  IRQ_NESTED_THREAD     │
│  Performance     │  Very fast       │  Slower                │
│  Latency         │  Low             │  Higher                │
│  RT-safe         │  Blocks RT       │  RT-friendly           │
│  Examples        │  SoC GPIO        │  I2C expander          │
│                  │  i.MX6 GPIO      │  MCP23016              │
│                  │  AM335x GPIO     │  PCF8574               │
└──────────────────┴──────────────────┴────────────────────────┘
```

---

## 16.6 Implementing Chained Interrupts

### 16.6.1 Chained Interrupt Flow

**Complete execution flow:**

```
┌────────────────────────────────────────────────┐
│  Chained Interrupt Execution Flow              │
├────────────────────────────────────────────────┤
│                                                │
│  1. Hardware interrupt arrives at GIC          │
│     GIC IRQ 96 fires                           │
│                                                │
│  2. GIC handler executes                       │
│     gic_handle_irq() [atomic context]          │
│                                                │
│  3. Calls GPIO parent handler                  │
│     gpio4_irq_handler() [atomic context]       │
│     • Read GPIO ISR register (FAST)            │
│     • Identify GPIO line 29                    │
│     • Find virq for GPIO 29                    │
│     • Call generic_handle_irq(virq)            │
│                                                │
│  4. Child handler executes IMMEDIATELY         │
│     device_irq_handler() [atomic context]      │
│     • Still in same hard IRQ context!          │
│     • No thread created                        │
│     • Cannot sleep                             │
│                                                │
│  5. Returns through call stack                 │
│     device_handler → GPIO handler → GIC        │
│                                                │
│  All in ONE continuous execution!              │
│  No context switches!                          │
│                                                │
└────────────────────────────────────────────────┘
```

### 16.6.2 Step-by-Step Implementation

**Step 1: Define IRQ domain operations**

```c
static int my_gpio_domain_map(struct irq_domain *d,
                               unsigned int virq,
                               irq_hw_number_t hw)
{
    struct my_gpio *gpio = d->host_data;

    /*
     * For CHAINED interrupts:
     * Set chip and handler
     * Use standard flow handler (handle_level_irq)
     */
    irq_set_chip_and_handler(virq,
                             &my_gpio_irq_chip,
                             handle_level_irq);

    irq_set_chip_data(virq, gpio);

    /* Mark as valid, no probing */
    irq_set_noprobe(virq);

    return 0;
}

static const struct irq_domain_ops my_gpio_domain_ops = {
    .map = my_gpio_domain_map,
    .xlate = irq_domain_xlate_twocell,
};
```

**Step 2: Implement IRQ chip callbacks**

```c
static void my_gpio_irq_mask(struct irq_data *d)
{
    struct my_gpio *gpio = irq_data_get_irq_chip_data(d);
    unsigned int gpio_num = d->hwirq;
    u32 mask;

    /* Disable interrupt for this GPIO */
    mask = readl(gpio->base + GPIO_INT_ENABLE);
    mask &= ~(1 << gpio_num);
    writel(mask, gpio->base + GPIO_INT_ENABLE);
}

static void my_gpio_irq_unmask(struct irq_data *d)
{
    struct my_gpio *gpio = irq_data_get_irq_chip_data(d);
    unsigned int gpio_num = d->hwirq;
    u32 mask;

    /* Enable interrupt for this GPIO */
    mask = readl(gpio->base + GPIO_INT_ENABLE);
    mask |= (1 << gpio_num);
    writel(mask, gpio->base + GPIO_INT_ENABLE);
}

static void my_gpio_irq_ack(struct irq_data *d)
{
    struct my_gpio *gpio = irq_data_get_irq_chip_data(d);
    unsigned int gpio_num = d->hwirq;

    /* Clear interrupt status */
    writel(1 << gpio_num, gpio->base + GPIO_INT_CLEAR);
}

static int my_gpio_irq_set_type(struct irq_data *d, unsigned int type)
{
    struct my_gpio *gpio = irq_data_get_irq_chip_data(d);
    unsigned int gpio_num = d->hwirq;
    u32 val;

    val = readl(gpio->base + GPIO_INT_TYPE);

    switch (type) {
    case IRQ_TYPE_EDGE_RISING:
        val |= (1 << gpio_num);
        break;
    case IRQ_TYPE_EDGE_FALLING:
        val &= ~(1 << gpio_num);
        break;
    case IRQ_TYPE_LEVEL_HIGH:
    case IRQ_TYPE_LEVEL_LOW:
        /* Configure level type */
        break;
    default:
        return -EINVAL;
    }

    writel(val, gpio->base + GPIO_INT_TYPE);

    return 0;
}

static struct irq_chip my_gpio_irq_chip = {
    .name = "my-gpio",
    .irq_mask = my_gpio_irq_mask,
    .irq_unmask = my_gpio_irq_unmask,
    .irq_ack = my_gpio_irq_ack,
    .irq_set_type = my_gpio_irq_set_type,
};
```

**Step 3: Implement parent IRQ handler**

```c
/*
 * This is the parent hardware IRQ handler
 * Called by GIC when GPIO interrupt fires
 * Runs in HARD IRQ CONTEXT (atomic)
 */
static void my_gpio_irq_handler(struct irq_desc *desc)
{
    struct irq_chip *chip = irq_desc_get_chip(desc);
    struct gpio_chip *gc = irq_desc_get_handler_data(desc);
    struct my_gpio *gpio = gpiochip_get_data(gc);
    u32 status;
    unsigned int i, child_irq;

    /*
     * Enter chained handler
     * This masks and acks the parent IRQ
     */
    chained_irq_enter(chip, desc);

    /* Read interrupt status register (FAST - memory-mapped) */
    status = readl(gpio->base + GPIO_INT_STATUS);

    /* Handle each active GPIO interrupt */
    for (i = 0; i < gpio->ngpio; i++) {
        if (status & (1 << i)) {
            /* Find virtual IRQ for this GPIO */
            child_irq = irq_find_mapping(gpio->domain, i);

            if (child_irq) {
                /*
                 * Call child handler directly (chained)
                 * This runs in same context!
                 */
                generic_handle_irq(child_irq);
            }
        }
    }

    /*
     * Exit chained handler
     * This unmasks the parent IRQ
     */
    chained_irq_exit(chip, desc);
}
```

**Step 4: Register chained handler in probe**

```c
static int my_gpio_probe(struct platform_device *pdev)
{
    struct my_gpio *gpio;
    struct resource *res;
    int parent_irq;
    int ret, i;

    /* Allocate private data */
    gpio = devm_kzalloc(&pdev->dev, sizeof(*gpio), GFP_KERNEL);
    if (!gpio)
        return -ENOMEM;

    /* Get memory resource */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    gpio->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(gpio->base))
        return PTR_ERR(gpio->base);

    /* Get parent IRQ from DT */
    parent_irq = platform_get_irq(pdev, 0);
    if (parent_irq < 0) {
        dev_err(&pdev->dev, "No IRQ resource\n");
        return parent_irq;
    }

    gpio->ngpio = 32;

    /* Setup GPIO chip (not shown here) */
    /* ... gpio_chip initialization ... */

    /* Create IRQ domain */
    gpio->domain = irq_domain_add_linear(
        pdev->dev.of_node,
        gpio->ngpio,
        &my_gpio_domain_ops,
        gpio);

    if (!gpio->domain) {
        dev_err(&pdev->dev, "Failed to create IRQ domain\n");
        return -ENOMEM;
    }

    /* Create IRQ mappings */
    for (i = 0; i < gpio->ngpio; i++) {
        int virq = irq_create_mapping(gpio->domain, i);
        if (!virq) {
            dev_err(&pdev->dev,
                    "Failed to map IRQ for GPIO %d\n", i);
            goto err_domain;
        }
    }

    /*
     * CRITICAL FOR CHAINED:
     * Set chained handler and data
     * This registers parent handler WITHOUT request_irq()
     */
    irq_set_chained_handler_and_data(parent_irq,
                                      my_gpio_irq_handler,
                                      gc);

    /*
     * The parent IRQ is now:
     * - Enabled automatically
     * - Marked as IRQ_NOREQUEST (can't be requested)
     * - Marked as IRQ_NOTHREAD (can't be threaded)
     * - Marked as IRQ_NOPROBE
     */

    dev_info(&pdev->dev,
             "Chained IRQ handler registered for IRQ %d\n",
             parent_irq);

    return 0;

err_domain:
    irq_domain_remove(gpio->domain);
    return ret;
}
```

**Step 5: Cleanup in remove**

```c
static int my_gpio_remove(struct platform_device *pdev)
{
    struct my_gpio *gpio = platform_get_drvdata(pdev);
    int parent_irq;

    parent_irq = platform_get_irq(pdev, 0);

    /* Remove chained handler */
    irq_set_chained_handler_and_data(parent_irq, NULL, NULL);

    /* Remove IRQ domain */
    irq_domain_remove(gpio->domain);

    return 0;
}
```

---

## 16.7 Implementing Nested Interrupts

### 16.7.1 Nested Interrupt Flow

**Complete execution flow:**

```
┌────────────────────────────────────────────────┐
│  Nested Interrupt Execution Flow               │
├────────────────────────────────────────────────┤
│                                                │
│  1. Hardware interrupt arrives                 │
│     Expander pulls INT line LOW                │
│                                                │
│  2. GIC handler schedules thread               │
│     request_threaded_irq() creates thread      │
│                                                │
│  3. Parent thread handler runs                 │
│     mcp23016_irq_thread() [THREAD context]     │
│     • Read status via I2C (MAY SLEEP!)         │
│     • Can be preempted                         │
│     • IRQs enabled                             │
│     • Find active GPIO                         │
│     • Call handle_nested_irq(virq)             │
│                                                │
│  4. Child thread handler scheduled             │
│     device_irq_thread() [THREAD context]       │
│     • Runs in separate thread                  │
│     • Can sleep                                │
│     • Can be preempted                         │
│                                                │
│  Multiple threads involved!                    │
│  Context switches occur!                       │
│  More latency but RT-friendly!                 │
│                                                │
└────────────────────────────────────────────────┘
```

### 16.7.2 Step-by-Step Implementation

**Step 1: Define IRQ domain operations**

```c
static int mcp23016_domain_map(struct irq_domain *d,
                                unsigned int virq,
                                irq_hw_number_t hw)
{
    struct mcp23016 *mcp = d->host_data;

    /*
     * For NESTED interrupts:
     * Mark as nested thread - CRITICAL!
     */
    irq_set_chip_data(virq, mcp);

    /* Use simple IRQ chip or custom chip */
    irq_set_chip(virq, &dummy_irq_chip);

    /*
     * Mark as nested thread
     * This tells kernel: don't create separate thread,
     * run in parent's thread context
     */
    irq_set_nested_thread(virq, 1);

    /* Mark as valid */
    irq_set_noprobe(virq);

    return 0;
}

static const struct irq_domain_ops mcp23016_domain_ops = {
    .map = mcp23016_domain_map,
    .xlate = irq_domain_xlate_twocell,
};
```

**Step 2: Implement parent IRQ handler (threaded)**

```c
/*
 * This is the parent threaded IRQ handler
 * Runs in THREAD CONTEXT (can sleep!)
 */
static irqreturn_t mcp23016_irq_thread(int irq, void *dev_id)
{
    struct mcp23016 *mcp = dev_id;
    u16 int_status;
    unsigned int i, child_irq;
    int ret;

    /*
     * Read interrupt status via I2C
     * THIS MAY SLEEP! That's why we need threaded IRQ!
     */
    ret = i2c_smbus_read_word_data(mcp->client, MCP23016_INT_STATUS);
    if (ret < 0) {
        dev_err(&mcp->client->dev,
                "Failed to read interrupt status\n");
        return IRQ_HANDLED;
    }

    int_status = ret;

    /* No active interrupts */
    if (!int_status)
        return IRQ_HANDLED;

    /* Handle each active GPIO interrupt */
    for (i = 0; i < mcp->chip.ngpio; i++) {
        if (int_status & (1 << i)) {
            /* Find virtual IRQ for this GPIO */
            child_irq = irq_find_mapping(mcp->domain, i);

            if (child_irq) {
                /*
                 * Call nested handler
                 * This schedules child handler thread
                 */
                handle_nested_irq(child_irq);
            }
        }
    }

    /*
     * Clear interrupts via I2C
     * Again, may sleep
     */
    i2c_smbus_write_word_data(mcp->client,
                               MCP23016_INT_CLEAR,
                               int_status);

    return IRQ_HANDLED;
}
```

**Step 3: Register threaded handler in probe**

```c
static int mcp23016_probe(struct i2c_client *client,
                          const struct i2c_device_id *id)
{
    struct mcp23016 *mcp;
    int ret, i;

    /* Allocate private data */
    mcp = devm_kzalloc(&client->dev, sizeof(*mcp), GFP_KERNEL);
    if (!mcp)
        return -ENOMEM;

    mcp->client = client;
    i2c_set_clientdata(client, mcp);

    /* Setup GPIO chip (not shown) */
    /* ... */

    /* Create IRQ domain */
    mcp->domain = irq_domain_add_linear(
        client->dev.of_node,
        16,  /* 16 GPIOs */
        &mcp23016_domain_ops,
        mcp);

    if (!mcp->domain) {
        dev_err(&client->dev, "Failed to create IRQ domain\n");
        return -ENOMEM;
    }

    /* Create IRQ mappings */
    for (i = 0; i < 16; i++) {
        int virq = irq_create_mapping(mcp->domain, i);
        if (!virq) {
            dev_err(&client->dev,
                    "Failed to map IRQ %d\n", i);
            goto err_domain;
        }
    }

    /* Check if parent IRQ available */
    if (client->irq) {
        /*
         * CRITICAL FOR NESTED:
         * Use request_threaded_irq()
         * NOT irq_set_chained_handler_and_data()!
         */
        ret = devm_request_threaded_irq(
            &client->dev,
            client->irq,
            NULL,                    /* No hard IRQ handler */
            mcp23016_irq_thread,     /* Thread handler */
            IRQF_ONESHOT |           /* Keep IRQ masked until thread done */
            IRQF_TRIGGER_LOW,        /* Active low */
            "mcp23016",
            mcp);

        if (ret) {
            dev_err(&client->dev,
                    "Failed to request IRQ %d: %d\n",
                    client->irq, ret);
            goto err_domain;
        }

        dev_info(&client->dev,
                 "Nested threaded IRQ handler registered\n");
    }

    return 0;

err_domain:
    irq_domain_remove(mcp->domain);
    return ret;
}
```

**Step 4: No special cleanup needed!**

```c
static int mcp23016_remove(struct i2c_client *client)
{
    struct mcp23016 *mcp = i2c_get_clientdata(client);

    /* devm_request_threaded_irq() handles cleanup */
    /* Just remove domain */
    irq_domain_remove(mcp->domain);

    return 0;
}
```

---

## 16.8 GPIO-IRQ Controller Integration

### 16.8.1 Complete Example: SoC GPIO with Chained IRQ

**Full implementation:**

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio/driver.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/of.h>
#include <linux/io.h>

#define GPIO_DATA       0x00
#define GPIO_DIR        0x04
#define GPIO_INT_EN     0x08
#define GPIO_INT_STAT   0x0C
#define GPIO_INT_CLR    0x10

struct soc_gpio {
    struct gpio_chip chip;
    struct irq_chip irq_chip;
    struct irq_domain *domain;
    void __iomem *base;
    int parent_irq;
    raw_spinlock_t lock;
};

/* GPIO chip callbacks */
static int soc_gpio_direction_input(struct gpio_chip *gc, unsigned offset)
{
    struct soc_gpio *gpio = gpiochip_get_data(gc);
    unsigned long flags;
    u32 val;

    raw_spin_lock_irqsave(&gpio->lock, flags);
    val = readl(gpio->base + GPIO_DIR);
    val |= (1 << offset);  /* 1 = input */
    writel(val, gpio->base + GPIO_DIR);
    raw_spin_unlock_irqrestore(&gpio->lock, flags);

    return 0;
}

static int soc_gpio_direction_output(struct gpio_chip *gc,
                                      unsigned offset, int value)
{
    struct soc_gpio *gpio = gpiochip_get_data(gc);
    unsigned long flags;
    u32 val;

    raw_spin_lock_irqsave(&gpio->lock, flags);

    /* Set value first */
    val = readl(gpio->base + GPIO_DATA);
    if (value)
        val |= (1 << offset);
    else
        val &= ~(1 << offset);
    writel(val, gpio->base + GPIO_DATA);

    /* Then set direction */
    val = readl(gpio->base + GPIO_DIR);
    val &= ~(1 << offset);  /* 0 = output */
    writel(val, gpio->base + GPIO_DIR);

    raw_spin_unlock_irqrestore(&gpio->lock, flags);

    return 0;
}

static int soc_gpio_get(struct gpio_chip *gc, unsigned offset)
{
    struct soc_gpio *gpio = gpiochip_get_data(gc);
    u32 val;

    val = readl(gpio->base + GPIO_DATA);
    return !!(val & (1 << offset));
}

static void soc_gpio_set(struct gpio_chip *gc, unsigned offset, int value)
{
    struct soc_gpio *gpio = gpiochip_get_data(gc);
    unsigned long flags;
    u32 val;

    raw_spin_lock_irqsave(&gpio->lock, flags);
    val = readl(gpio->base + GPIO_DATA);
    if (value)
        val |= (1 << offset);
    else
        val &= ~(1 << offset);
    writel(val, gpio->base + GPIO_DATA);
    raw_spin_unlock_irqrestore(&gpio->lock, flags);
}

/* IRQ chip callbacks */
static void soc_gpio_irq_mask(struct irq_data *d)
{
    struct soc_gpio *gpio = irq_data_get_irq_chip_data(d);
    unsigned long flags;
    u32 val;

    raw_spin_lock_irqsave(&gpio->lock, flags);
    val = readl(gpio->base + GPIO_INT_EN);
    val &= ~(1 << d->hwirq);
    writel(val, gpio->base + GPIO_INT_EN);
    raw_spin_unlock_irqrestore(&gpio->lock, flags);
}

static void soc_gpio_irq_unmask(struct irq_data *d)
{
    struct soc_gpio *gpio = irq_data_get_irq_chip_data(d);
    unsigned long flags;
    u32 val;

    raw_spin_lock_irqsave(&gpio->lock, flags);
    val = readl(gpio->base + GPIO_INT_EN);
    val |= (1 << d->hwirq);
    writel(val, gpio->base + GPIO_INT_EN);
    raw_spin_unlock_irqrestore(&gpio->lock, flags);
}

static void soc_gpio_irq_ack(struct irq_data *d)
{
    struct soc_gpio *gpio = irq_data_get_irq_chip_data(d);

    writel(1 << d->hwirq, gpio->base + GPIO_INT_CLR);
}

/* IRQ domain operations */
static int soc_gpio_domain_map(struct irq_domain *d,
                                unsigned int virq,
                                irq_hw_number_t hw)
{
    struct soc_gpio *gpio = d->host_data;

    irq_set_chip_and_handler(virq,
                             &gpio->irq_chip,
                             handle_level_irq);
    irq_set_chip_data(virq, gpio);
    irq_set_noprobe(virq);

    return 0;
}

static const struct irq_domain_ops soc_gpio_domain_ops = {
    .map = soc_gpio_domain_map,
    .xlate = irq_domain_xlate_twocell,
};

/* Chained IRQ handler */
static void soc_gpio_irq_handler(struct irq_desc *desc)
{
    struct irq_chip *chip = irq_desc_get_chip(desc);
    struct gpio_chip *gc = irq_desc_get_handler_data(desc);
    struct soc_gpio *gpio = gpiochip_get_data(gc);
    u32 status;
    unsigned int i, child_irq;

    chained_irq_enter(chip, desc);

    status = readl(gpio->base + GPIO_INT_STAT);

    for (i = 0; i < 32; i++) {
        if (status & (1 << i)) {
            child_irq = irq_find_mapping(gpio->domain, i);
            if (child_irq)
                generic_handle_irq(child_irq);
        }
    }

    chained_irq_exit(chip, desc);
}

/* Probe */
static int soc_gpio_probe(struct platform_device *pdev)
{
    struct soc_gpio *gpio;
    struct resource *res;
    int ret, i;

    gpio = devm_kzalloc(&pdev->dev, sizeof(*gpio), GFP_KERNEL);
    if (!gpio)
        return -ENOMEM;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    gpio->base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(gpio->base))
        return PTR_ERR(gpio->base);

    gpio->parent_irq = platform_get_irq(pdev, 0);
    if (gpio->parent_irq < 0)
        return gpio->parent_irq;

    raw_spin_lock_init(&gpio->lock);

    /* Setup GPIO chip */
    gpio->chip.label = "soc-gpio";
    gpio->chip.dev = &pdev->dev;
    gpio->chip.owner = THIS_MODULE;
    gpio->chip.base = -1;
    gpio->chip.ngpio = 32;
    gpio->chip.direction_input = soc_gpio_direction_input;
    gpio->chip.direction_output = soc_gpio_direction_output;
    gpio->chip.get = soc_gpio_get;
    gpio->chip.set = soc_gpio_set;
    gpio->chip.of_node = pdev->dev.of_node;

    /* Setup IRQ chip */
    gpio->irq_chip.name = "soc-gpio";
    gpio->irq_chip.irq_mask = soc_gpio_irq_mask;
    gpio->irq_chip.irq_unmask = soc_gpio_irq_unmask;
    gpio->irq_chip.irq_ack = soc_gpio_irq_ack;

    /* Register GPIO chip */
    ret = devm_gpiochip_add_data(&pdev->dev, &gpio->chip, gpio);
    if (ret)
        return ret;

    /* Create IRQ domain */
    gpio->domain = irq_domain_add_linear(
        pdev->dev.of_node,
        32,
        &soc_gpio_domain_ops,
        gpio);

    if (!gpio->domain)
        return -ENOMEM;

    /* Create mappings */
    for (i = 0; i < 32; i++) {
        int virq = irq_create_mapping(gpio->domain, i);
        if (!virq) {
            irq_domain_remove(gpio->domain);
            return -EINVAL;
        }
    }

    /* Set chained handler */
    irq_set_chained_handler_and_data(gpio->parent_irq,
                                      soc_gpio_irq_handler,
                                      &gpio->chip);

    platform_set_drvdata(pdev, gpio);

    dev_info(&pdev->dev,
             "SoC GPIO registered: 32 GPIOs, chained IRQ %d\n",
             gpio->parent_irq);

    return 0;
}

static int soc_gpio_remove(struct platform_device *pdev)
{
    struct soc_gpio *gpio = platform_get_drvdata(pdev);

    irq_set_chained_handler_and_data(gpio->parent_irq, NULL, NULL);
    irq_domain_remove(gpio->domain);

    return 0;
}

static const struct of_device_id soc_gpio_of_match[] = {
    { .compatible = "vendor,soc-gpio" },
    { }
};
MODULE_DEVICE_TABLE(of, soc_gpio_of_match);

static struct platform_driver soc_gpio_driver = {
    .probe = soc_gpio_probe,
    .remove = soc_gpio_remove,
    .driver = {
        .name = "soc-gpio",
        .of_match_table = soc_gpio_of_match,
    },
};

module_platform_driver(soc_gpio_driver);

MODULE_AUTHOR("Embedded Linux Expert");
MODULE_DESCRIPTION("SoC GPIO Controller with Chained IRQ");
MODULE_LICENSE("GPL");
```

---

## Summary

In this part, we covered:

**Key Concepts:**

1. **Chained vs Nested**
    - Fundamental differences
    - When to use each approach
    - Complete comparison table
2. **Chained Interrupts**
    - Fast, atomic execution
    - Memory-mapped controllers
    - generic_handle_irq()
    - irq_set_chained_handler_and_data()
    - Complete implementation
3. **Nested Interrupts**
    - Threaded execution
    - Bus-based controllers (I2C/SPI)
    - handle_nested_irq()
    - request_threaded_irq()
    - Complete implementation
4. **Complete Driver Examples**
    - SoC GPIO with chained IRQ
    - Full ~400 lines implementation
    - All callbacks implemented