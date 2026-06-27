# Part 2. IRQ Chip Integration

This part covers adding interrupt controller functionality to GPIO controllers, including IRQ domain API, chained vs nested interrupts, and the modern gpiolib irqchip API.

---

## 15.6 IRQ Chip Fundamentals

### 15.6.1 GPIO Controller as Interrupt Controller

**Many GPIO controllers can also generate interrupts:**

```
┌────────────────────────────────────────────────┐
│  GPIO Controller with IRQ Support              │
├────────────────────────────────────────────────┤
│                                                │
│  GPIO Controller (e.g., MCP23016)              │
│  ┌──────────────────────────────────────┐      │
│  │  GPIO 0  →  Can generate IRQ 0       │      │
│  │  GPIO 1  →  Can generate IRQ 1       │      │
│  │  GPIO 2  →  Can generate IRQ 2       │      │
│  │  ...                                 │      │
│  │  GPIO 15 →  Can generate IRQ 15      │      │
│  └────────────┬─────────────────────────┘      │
│               │                                │
│               │ Single IRQ line (multiplexed)  │
│               ▼                                │
│  ┌─────────────────────────────────────┐       │
│  │  Parent Interrupt Controller        │       │
│  │  (e.g., SoC GPIO or GIC)            │       │
│  └─────────────────────────────────────┘       │
│                                                │
│  Problem: How to map?                          │
│  • 16 GPIO lines → 16 potential IRQs           │
│  • Only 1 hardware IRQ line to parent          │
│  • Need IRQ multiplexing!                      │
│                                                │
└────────────────────────────────────────────────┘
```

### 15.6.2 IRQ Multiplexing Example

**Real-world scenario:**

```
┌────────────────────────────────────────────────┐
│  IRQ Multiplexing in Action                    │
├────────────────────────────────────────────────┤
│                                                │
│  1. Button connected to GPIO expander pin 8    │
│                                                │
│  2. Button pressed → GPIO 8 changes state      │
│                                                │
│  3. GPIO expander sets internal IRQ flag       │
│                                                │
│  4. Expander pulls INT line to parent LOW      │
│                                                │
│  5. Parent IRQ controller triggers             │
│                                                │
│  6. Parent handler runs                        │
│                                                │
│  7. Parent handler reads expander status reg   │
│     Discovers: GPIO 8 caused interrupt         │
│                                                │
│  8. Parent finds child IRQ for GPIO 8          │
│                                                │
│  9. Child IRQ handler runs (button driver)     │
│                                                │
│  Single hardware IRQ multiplexes 16 GPIO IRQs! │
│                                                │
└────────────────────────────────────────────────┘
```

---

## 15.7 IRQ Domain API

### 15.7.1 Why IRQ Domains?

**IRQ domains solve the mapping problem:**

```
┌────────────────────────────────────────────────┐
│  IRQ Domain: hwirq ↔ virq Mapping              │
├────────────────────────────────────────────────┤
│                                                │
│  Hardware IRQ (hwirq):                         │
│  • Local to the interrupt controller           │
│  • GPIO offset (0-15 for MCP23016)             │
│  • Not unique across system                    │
│                                                │
│  Virtual IRQ (virq):                           │
│  • Global Linux IRQ number                     │
│  • Unique across entire system                 │
│  • What request_irq() uses                     │
│                                                │
│  IRQ Domain provides the mapping:              │
│                                                │
│  GPIO Offset (hwirq)  →  Linux IRQ (virq)      │
│         0             →       120              │
│         1             →       121              │
│         2             →       122              │
│        ...            →       ...              │
│        15             →       135              │
│                                                │
└────────────────────────────────────────────────┘
```

### 15.7.2 IRQ Domain Structures

**Required headers:**

```c
#include <linux/irq.h>
#include <linux/irqdomain.h>
```

**Core structures:**

```c
/* IRQ domain */
struct irq_domain {
    struct irq_domain_ops *ops;
    void *host_data;
    /* ... internal fields ... */
};

/* IRQ domain operations */
struct irq_domain_ops {
    int  (*map)(struct irq_domain *d, unsigned int virq,
                irq_hw_number_t hw);
    void (*unmap)(struct irq_domain *d, unsigned int virq);
    int  (*xlate)(struct irq_domain *d, struct device_node *node,
                  const u32 *intspec, unsigned int intsize,
                  unsigned long *out_hwirq,
                  unsigned int *out_type);
};
```

### 15.7.3 Creating an IRQ Domain

**Linear mapping (most common for GPIO):**

```c
struct irq_domain *irq_domain_add_linear(
    struct device_node *of_node,
    unsigned int size,
    const struct irq_domain_ops *ops,
    void *host_data);
```

**Example:**

```c
static const struct irq_domain_ops mcp23016_irq_domain_ops = {
    .map = mcp23016_irq_domain_map,
    .xlate = irq_domain_xlate_twocell,
};

static int mcp23016_probe(struct i2c_client *client)
{
    struct mcp23016 *mcp;

    /* ... previous initialization ... */

    /* Create IRQ domain */
    mcp->irq_domain = irq_domain_add_linear(
        client->dev.of_node,     /* DT node */
        mcp->chip.ngpio,         /* Number of IRQs (16) */
        &mcp23016_irq_domain_ops,
        mcp);                    /* host_data */

    if (!mcp->irq_domain) {
        dev_err(&client->dev, "Failed to create IRQ domain\n");
        return -ENOMEM;
    }

    return 0;
}
```

### 15.7.4 Implementing .map() Callback

**The .map() callback configures each IRQ:**

```c
static int mcp23016_irq_domain_map(struct irq_domain *d,
                                    unsigned int virq,
                                    irq_hw_number_t hw)
{
    struct mcp23016 *mcp = d->host_data;

    /*
     * Set IRQ chip for this virq
     * Use kernel's dummy_irq_chip for simple controllers
     */
    irq_set_chip_and_handler(virq,
                             &dummy_irq_chip,
                             handle_level_irq);

    /* Set chip data (can retrieve in IRQ handler) */
    irq_set_chip_data(virq, mcp);

    /* Nested threaded IRQ (because I2C access sleeps) */
    irq_set_nested_thread(virq, 1);

    return 0;
}
```

**For advanced controllers with custom irq_chip:**

```c
static struct irq_chip mcp23016_irq_chip = {
    .name = "mcp23016",
    .irq_mask = mcp23016_irq_mask,
    .irq_unmask = mcp23016_irq_unmask,
    .irq_bus_lock = mcp23016_irq_bus_lock,
    .irq_bus_sync_unlock = mcp23016_irq_bus_sync_unlock,
    .irq_set_type = mcp23016_irq_set_type,
};

static int mcp23016_irq_domain_map(struct irq_domain *d,
                                    unsigned int virq,
                                    irq_hw_number_t hw)
{
    struct mcp23016 *mcp = d->host_data;

    /* Use our custom IRQ chip */
    irq_set_chip_and_handler(virq,
                             &mcp23016_irq_chip,
                             handle_level_irq);

    irq_set_chip_data(virq, mcp);
    irq_set_nested_thread(virq, 1);

    return 0;
}
```

### 15.7.5 Implementing .xlate() Callback

**xlate translates Device Tree IRQ specifier:**

```c
/*
 * Standard two-cell translation
 * Cell 0: GPIO offset
 * Cell 1: IRQ flags (edge/level, polarity)
 */
static const struct irq_domain_ops mcp23016_irq_domain_ops = {
    .map = mcp23016_irq_domain_map,
    .xlate = irq_domain_xlate_twocell,  /* Kernel-provided helper */
};
```

**Kernel provides standard xlate helpers:**

- `irq_domain_xlate_onecell` - Single cell (just hwirq)
- `irq_domain_xlate_twocell` - Two cells (hwirq + flags)
- `irq_domain_xlate_onetwocell` - One or two cells

### 15.7.6 Creating IRQ Mappings

**Create mapping for a GPIO:**

```c
int irq_create_mapping(struct irq_domain *domain, irq_hw_number_t hwirq);
```

**Find existing mapping:**

```c
unsigned int irq_find_mapping(struct irq_domain *domain,
                               irq_hw_number_t hwirq);
```

**Typical usage in .to_irq callback:**

```c
static int mcp23016_to_irq(struct gpio_chip *chip, unsigned offset)
{
    struct mcp23016 *mcp = to_mcp23016(chip);

    /* Create mapping if it doesn't exist, return virq */
    return irq_create_mapping(mcp->irq_domain, offset);
}
```

---

## 15.8 Chained vs Nested Interrupts

### 15.8.1 Chained Interrupts

**Characteristics:**

```
┌────────────────────────────────────────────────┐
│  Chained Interrupts                            │
├────────────────────────────────────────────────┤
│                                                │
│  Use case: Memory-mapped GPIO controllers      │
│  • SoC internal GPIO banks                     │
│  • MMIO register access                        │
│  • Does NOT sleep                              │
│                                                │
│  Execution model:                              │
│  • Parent IRQ handler calls child directly     │
│  • Like function call chain                    │
│  • Runs in hardware IRQ context (atomic)       │
│  • Cannot sleep!                               │
│                                                │
│  Parent IRQ Handler                            │
│  ┌──────────────────────────┐                  │
│  │ 1. Save context          │                  │
│  │ 2. Read status register  │ ← MMIO (fast)    │
│  │ 3. For each active GPIO: │                  │
│  │    call child handler()  │ ← Direct call    │
│  │ 4. Clear interrupt       │                  │
│  │ 5. Restore context       │                  │
│  └──────────────────────────┘                  │
│                                                │
│  Use: generic_handle_irq()                     │
│                                                │
└────────────────────────────────────────────────┘
```

**Example:**

```c
static irqreturn_t my_gpio_irq_handler(int irq, void *data)
{
    struct my_gpio *gpio = data;
    u32 status;
    int i;
    unsigned int child_irq;

    /* Read interrupt status (memory-mapped, fast) */
    status = readl(gpio->base + GPIO_INT_STATUS);

    /* Handle each active GPIO IRQ */
    for (i = 0; i < 32; i++) {
        if (status & (1 << i)) {
            /* Find Linux IRQ number */
            child_irq = irq_find_mapping(gpio->domain, i);

            /* Call child handler directly (chained) */
            generic_handle_irq(child_irq);
        }
    }

    /* Clear interrupt */
    writel(status, gpio->base + GPIO_INT_CLEAR);

    return IRQ_HANDLED;
}
```

### 15.8.2 Nested Interrupts

**Characteristics:**

```
┌────────────────────────────────────────────────┐
│  Nested Interrupts (Threaded)                  │
├────────────────────────────────────────────────┤
│                                                │
│  Use case: Bus-based GPIO controllers          │
│  • I2C GPIO expanders                          │
│  • SPI GPIO expanders                          │
│  • Bus access MAY sleep                        │
│                                                │
│  Execution model:                              │
│  • Parent IRQ handler schedules threads        │
│  • Child handlers run in thread context        │
│  • Can sleep!                                  │
│  • Can be preempted                            │
│                                                │
│  Parent IRQ Handler (threaded)                 │
│  ┌──────────────────────────┐                  │
│  │ 1. Read status via I2C   │ ← May sleep      │
│  │ 2. For each active GPIO: │                  │
│  │    schedule thread       │                  │
│  └──────────────────────────┘                  │
│           │                                    │
│           ▼                                    │
│  Child Handler Thread                          │
│  ┌──────────────────────────┐                  │
│  │ Runs in process context  │                  │
│  │ Can call I2C functions   │                  │
│  │ Can sleep                │                  │
│  └──────────────────────────┘                  │
│                                                │
│  Use: handle_nested_irq()                      │
│                                                │
└────────────────────────────────────────────────┘
```

**Example:**

```c
static irqreturn_t mcp23016_irq_handler(int irq, void *data)
{
    struct mcp23016 *mcp = data;
    u16 status;
    int i;
    unsigned int child_irq;
    int ret;

    /* Read interrupt status via I2C (may sleep!) */
    ret = mcp23016_read_word(mcp, MCP23016_INT_STATUS, &status);
    if (ret)
        return IRQ_HANDLED;

    /* Handle each active GPIO IRQ */
    for (i = 0; i < mcp->chip.ngpio; i++) {
        if (status & (1 << i)) {
            /* Find Linux IRQ number */
            child_irq = irq_find_mapping(mcp->irq_domain, i);

            /* Schedule threaded handler (nested) */
            handle_nested_irq(child_irq);
        }
    }

    /* Clear interrupts via I2C */
    mcp23016_write_word(mcp, MCP23016_INT_CLEAR, status);

    return IRQ_HANDLED;
}
```

### 15.8.3 Comparison Table

```
┌─────────────────────────────────────────────────────────────┐
│  Chained vs Nested Interrupts                               │
├──────────────────┬──────────────────┬───────────────────────┤
│  Feature         │  Chained         │  Nested               │
├──────────────────┼──────────────────┼───────────────────────┤
│  Controller      │  Memory-mapped   │  Bus-based (I2C/SPI)  │
│  Access speed    │  Fast (MMIO)     │  Slow (I2C)           │
│  Can sleep?      │  NO              │  YES                  │
│  Context         │  Hardware IRQ    │  Thread               │
│  Preemptible?    │  NO              │  YES                  │
│  IRQs enabled?   │  NO              │  YES                  │
│  Handler func    │  generic_handle  │  handle_nested_irq    │
│  Request type    │  IRQF_ONESHOT    │  IRQF_ONESHOT         │
│  Thread flag     │  Not set         │  irq_set_nested_      │
│                  │                  │    thread(virq, 1)    │
└──────────────────┴──────────────────┴───────────────────────┘
```

---

## 15.9 Legacy IRQ Implementation (Manual)

### 15.9.1 Complete Implementation Steps

**Step 1: Add IRQ domain to device structure**

```c
struct mcp23016 {
    struct i2c_client *client;
    struct gpio_chip chip;
    struct irq_domain *irq_domain;  /* Add this */
    u16 reg_output;
    u16 reg_direction;
};
```

**Step 2: Define IRQ domain operations**

```c
static int mcp23016_irq_domain_map(struct irq_domain *d,
                                    unsigned int virq,
                                    irq_hw_number_t hw)
{
    struct mcp23016 *mcp = d->host_data;

    irq_set_chip_and_handler(virq,
                             &dummy_irq_chip,
                             handle_level_irq);

    irq_set_chip_data(virq, mcp);
    irq_set_nested_thread(virq, 1);

    return 0;
}

static const struct irq_domain_ops mcp23016_irq_domain_ops = {
    .map = mcp23016_irq_domain_map,
    .xlate = irq_domain_xlate_twocell,
};
```

**Step 3: Implement .to_irq callback**

```c
static int mcp23016_to_irq(struct gpio_chip *chip, unsigned offset)
{
    struct mcp23016 *mcp = to_mcp23016(chip);

    return irq_create_mapping(mcp->irq_domain, offset);
}
```

**Step 4: Create IRQ domain in probe**

```c
static int mcp23016_probe(struct i2c_client *client,
                          const struct i2c_device_id *id)
{
    struct mcp23016 *mcp;
    int ret;

    /* ... previous initialization ... */

    /* Setup GPIO chip */
    mcp->chip.to_irq = mcp23016_to_irq;  /* Add this */

    /* Register GPIO chip first */
    ret = devm_gpiochip_add_data(&client->dev, &mcp->chip, mcp);
    if (ret)
        return ret;

    /* Create IRQ domain */
    mcp->irq_domain = irq_domain_add_linear(
        client->dev.of_node,
        mcp->chip.ngpio,
        &mcp23016_irq_domain_ops,
        mcp);

    if (!mcp->irq_domain) {
        dev_err(&client->dev, "Failed to create IRQ domain\n");
        return -ENOMEM;
    }

    /* Register parent IRQ handler if provided */
    if (client->irq) {
        ret = devm_request_threaded_irq(&client->dev,
                                         client->irq,
                                         NULL,
                                         mcp23016_irq_handler,
                                         IRQF_ONESHOT | IRQF_TRIGGER_LOW,
                                         dev_name(&client->dev),
                                         mcp);
        if (ret) {
            dev_err(&client->dev, "Failed to request IRQ\n");
            irq_domain_remove(mcp->irq_domain);
            return ret;
        }
    }

    return 0;
}
```

**Step 5: Implement parent IRQ handler**

```c
static irqreturn_t mcp23016_irq_handler(int irq, void *data)
{
    struct mcp23016 *mcp = data;
    u16 int_status;
    unsigned int i, child_irq;
    int ret;

    /* Read interrupt status register */
    ret = mcp23016_read_word(mcp, MCP23016_INT_STATUS, &int_status);
    if (ret) {
        dev_err(&mcp->client->dev, "Failed to read INT status\n");
        return IRQ_HANDLED;
    }

    /* No active interrupts */
    if (!int_status)
        return IRQ_HANDLED;

    /* Process each active interrupt */
    for (i = 0; i < mcp->chip.ngpio; i++) {
        if (int_status & (1 << i)) {
            /* Get virtual IRQ number */
            child_irq = irq_find_mapping(mcp->irq_domain, i);
            if (child_irq) {
                /* Call nested handler */
                handle_nested_irq(child_irq);
            }
        }
    }

    /* Clear interrupts */
    mcp23016_write_word(mcp, MCP23016_INT_CLEAR, int_status);

    return IRQ_HANDLED;
}
```

**Step 6: Cleanup in remove**

```c
static int mcp23016_remove(struct i2c_client *client)
{
    struct mcp23016 *mcp = i2c_get_clientdata(client);

    /* Remove IRQ domain */
    if (mcp->irq_domain)
        irq_domain_remove(mcp->irq_domain);

    /* GPIO chip removed automatically by devm */

    return 0;
}
```

---

## 15.10 Modern gpiolib irqchip API

### 15.10.1 Advantages

**Why use gpiolib irqchip API?**

```
┌────────────────────────────────────────────────┐
│  Modern gpiolib irqchip API Benefits           │
├────────────────────────────────────────────────┤
│                                                │
│  Legacy approach:                              │
│  • Manual IRQ domain creation                  │
│  • Manual xlate setup                          │
│  • Manual .to_irq implementation               │
│  • Manual mapping creation                     │
│  • ~100 lines of boilerplate code              │
│                                                │
│  Modern API:                                   │
│  • Automatic IRQ domain creation               │
│  • Automatic xlate setup (twocell)             │
│  • Automatic .to_irq implementation            │
│  • Automatic mapping creation                  │
│  • ~10 lines of code!                          │
│                                                │
│  Benefits:                                     │
│  ✓ Less code                                   │
│  ✓ Less error-prone                            │
│  ✓ Standardized approach                       │
│  ✓ Maintained by kernel                        │
│                                                │
└────────────────────────────────────────────────┘
```

### 15.10.2 Core Functions

**Adding IRQ chip to GPIO chip:**

```c
int gpiochip_irqchip_add(struct gpio_chip *gpiochip,
                         struct irq_chip *irqchip,
                         unsigned int first_irq,
                         irq_flow_handler_t handler,
                         unsigned int type);
```

**For nested (threaded) IRQs:**

```c
int gpiochip_irqchip_add_nested(struct gpio_chip *gpiochip,
                                struct irq_chip *irqchip,
                                unsigned int first_irq,
                                irq_flow_handler_t handler,
                                unsigned int type);
```

**Connecting to parent IRQ (chained):**

```c
void gpiochip_set_chained_irqchip(struct gpio_chip *gpiochip,
                                   struct irq_chip *irqchip,
                                   unsigned int parent_irq,
                                   irq_flow_handler_t parent_handler);
```

**Connecting to parent IRQ (nested):**

```c
void gpiochip_set_nested_irqchip(struct gpio_chip *gpiochip,
                                  struct irq_chip *irqchip,
                                  unsigned int parent_irq);
```

### 15.10.3 Complete Modern Implementation

```c
static int mcp23016_probe(struct i2c_client *client,
                          const struct i2c_device_id *id)
{
    struct mcp23016 *mcp;
    int ret;

    /* ... standard initialization ... */

    /* Register GPIO chip first */
    ret = devm_gpiochip_add_data(&client->dev, &mcp->chip, mcp);
    if (ret) {
        dev_err(&client->dev, "Failed to register GPIO chip\n");
        return ret;
    }

    /* Add IRQ chip support if parent IRQ available */
    if (client->irq) {
        /*
         * Add nested IRQ chip to GPIO chip
         * This automatically:
         * - Creates IRQ domain
         * - Sets up .to_irq callback
         * - Creates IRQ mappings
         * - Configures twocell xlate
         */
        ret = gpiochip_irqchip_add_nested(&mcp->chip,
                                          &dummy_irq_chip,
                                          0,              /* first_irq */
                                          handle_level_irq,
                                          IRQ_TYPE_NONE);
        if (ret) {
            dev_err(&client->dev, "Failed to add irqchip\n");
            return ret;
        }

        /* Request parent IRQ */
        ret = devm_request_threaded_irq(&client->dev,
                                         client->irq,
                                         NULL,
                                         mcp23016_irq_handler,
                                         IRQF_ONESHOT | IRQF_TRIGGER_LOW,
                                         dev_name(&client->dev),
                                         mcp);
        if (ret) {
            dev_err(&client->dev, "Failed to request IRQ\n");
            return ret;
        }

        /*
         * Set nested irqchip
         * Links parent IRQ to this GPIO IRQ chip
         */
        gpiochip_set_nested_irqchip(&mcp->chip,
                                     &dummy_irq_chip,
                                     client->irq);

        dev_info(&client->dev,
                 "IRQ chip registered, parent IRQ=%d\n",
                 client->irq);
    }

    return 0;
}
```

**That's it! The API handles everything else!**

### 15.10.4 Using Custom irq_chip

**Define custom IRQ chip:**

```c
static struct irq_chip mcp23016_irq_chip = {
    .name = "mcp23016",
    .irq_mask = mcp23016_irq_mask,
    .irq_unmask = mcp23016_irq_unmask,
    .irq_bus_lock = mcp23016_irq_bus_lock,
    .irq_bus_sync_unlock = mcp23016_irq_bus_sync_unlock,
    .irq_set_type = mcp23016_irq_set_type,
};

/* Implement callbacks */
static void mcp23016_irq_mask(struct irq_data *data)
{
    struct mcp23016 *mcp = irq_data_get_irq_chip_data(data);
    unsigned int gpio = data->hwirq;

    /* Disable interrupt for this GPIO */
    mcp->irq_mask &= ~(1 << gpio);
}

static void mcp23016_irq_unmask(struct irq_data *data)
{
    struct mcp23016 *mcp = irq_data_get_irq_chip_data(data);
    unsigned int gpio = data->hwirq;

    /* Enable interrupt for this GPIO */
    mcp->irq_mask |= (1 << gpio);
}

/* Use in probe */
ret = gpiochip_irqchip_add_nested(&mcp->chip,
                                  &mcp23016_irq_chip,  /* Custom chip */
                                  0,
                                  handle_level_irq,
                                  IRQ_TYPE_NONE);
```

---

## Summary

In this part, we covered:

**Key Concepts:**

1. **IRQ Chip Fundamentals**
    - GPIO controllers as interrupt controllers
    - IRQ multiplexing concept
    - Hardware vs virtual IRQ numbers
2. **IRQ Domain API**
    - Why IRQ domains are needed
    - Creating IRQ domains
    - Implementing .map() and .xlate() callbacks
    - Creating and finding IRQ mappings
3. **Chained vs Nested Interrupts**
    - Memory-mapped (chained) controllers
    - Bus-based (nested) controllers
    - When to use which approach
    - Implementation differences
4. **Legacy Implementation**
    - Manual IRQ domain creation
    - Complete step-by-step guide
    - Parent IRQ handler implementation
5. **Modern gpiolib irqchip API**
    - Automatic IRQ infrastructure
    - Simplified code (~10 lines vs ~100)
    - Chained and nested variants
    - Custom irq_chip support