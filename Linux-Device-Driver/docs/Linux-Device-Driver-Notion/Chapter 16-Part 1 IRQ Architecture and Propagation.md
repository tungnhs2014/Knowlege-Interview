# Part 1. IRQ Architecture and Propagation

This part covers the Linux interrupt architecture, IRQ propagation mechanisms, interrupt controller hierarchy, and the IRQ domain API in detail.

---

## 16.1 Interrupt Architecture Overview

### 16.1.1 What is an Interrupt?

**Interrupt fundamentals:**

```
┌────────────────────────────────────────────────┐
│  Interrupt - Asynchronous Event Notification   │
├────────────────────────────────────────────────┤
│                                                │
│  Definition:                                   │
│  • Signal that causes CPU to stop normal flow  │
│  • Jump to special handler code                │
│  • Handle urgent event                         │
│  • Return to interrupted code                  │
│                                                │
│  Types of interrupts:                          │
│  1. Hardware interrupts (IRQ)                  │
│     - External devices (keyboard, network)     │
│     - Timers                                   │
│     - GPIO state changes                       │
│                                                │
│  2. Software interrupts                        │
│     - System calls                             │
│     - Exceptions (divide by zero)              │
│                                                │
│  Why use interrupts?                           │
│  ✓ Efficient - No polling needed               │
│  ✓ Fast response - Immediate notification      │
│  ✓ Power saving - CPU can sleep until IRQ      │
│                                                │
└────────────────────────────────────────────────┘
```

### 16.1.2 Linux IRQ Architecture

**Three-layer architecture:**

```
┌────────────────────────────────────────────────┐
│  Linux IRQ Management Architecture             │
├────────────────────────────────────────────────┤
│                                                │
│  Layer 1: Device Drivers (Consumers)           │
│  ┌──────────────────────────────────────┐      │
│  │ request_irq()                        │      │
│  │ request_threaded_irq()               │      │
│  │ devm_request_irq()                   │      │
│  │                                      │      │
│  │ Your IRQ handler:                    │      │
│  │ irqreturn_t my_handler(int irq, ...) │      │
│  └──────────────┬───────────────────────┘      │
│                 │                              │
│                 ▼                              │
│  Layer 2: IRQ Core (Generic Layer)             │
│  ┌──────────────────────────────────────┐      │
│  │ - IRQ descriptor management          │      │
│  │ - IRQ number mapping (hwirq→virq)    │      │
│  │ - Flow handlers (level/edge/etc)     │      │
│  │ - Threaded IRQ support               │      │
│  │ - IRQ domain API                     │      │
│  └──────────────┬───────────────────────┘      │
│                 │                              │
│                 ▼                              │
│  Layer 3: Interrupt Controllers (Drivers)      │
│  ┌──────────────────────────────────────┐      │
│  │ GIC (ARM)                            │      │
│  │ GPIO controllers                     │      │
│  │ Custom IRQ controllers               │      │
│  │                                      │      │
│  │ struct irq_chip {                    │      │
│  │   .irq_mask                          │      │
│  │   .irq_unmask                        │      │
│  │   .irq_ack                           │      │
│  │   .irq_set_type                      │      │
│  │ }                                    │      │
│  └──────────────┬───────────────────────┘      │
│                 │                              │
│                 ▼                              │
│  Hardware (Interrupt Controllers & Devices)    │
│                                                │
└────────────────────────────────────────────────┘
```

### 16.1.3 IRQ Numbers: hwirq vs virq

**Understanding the two IRQ numbering schemes:**

```
┌────────────────────────────────────────────────┐
│  Hardware IRQ (hwirq) vs Virtual IRQ (virq)    │
├────────────────────────────────────────────────┤
│                                                │
│  Hardware IRQ (hwirq):                         │
│  • Physical interrupt line number              │
│  • Local to interrupt controller               │
│  • NOT unique across system                    │
│  • Example: GPIO offset 5 on GPIO controller   │
│                                                │
│  Virtual IRQ (virq):                           │
│  • Linux global interrupt number               │
│  • Unique across entire system                 │
│  • What request_irq() uses                     │
│  • Managed by IRQ core                         │
│                                                │
│  Example mapping:                              │
│                                                │
│  GIC IRQ Controller:                           │
│    hwirq 32 → virq 100                         │
│    hwirq 33 → virq 101                         │
│                                                │
│  GPIO1 Controller:                             │
│    hwirq 0 (GPIO1_0) → virq 200                │
│    hwirq 1 (GPIO1_1) → virq 201                │
│                                                │
│  GPIO2 Controller:                             │
│    hwirq 0 (GPIO2_0) → virq 300                │
│    hwirq 1 (GPIO2_1) → virq 301                │
│                                                │
│  Same hwirq (0), different virq (200, 300)!    │
│                                                │
└────────────────────────────────────────────────┘
```

---

## 16.2 IRQ Propagation Mechanism

### 16.2.1 Complete IRQ Flow

**From hardware interrupt to handler execution:**

```
┌────────────────────────────────────────────────┐
│  Complete IRQ Propagation Flow                 │
├────────────────────────────────────────────────┤
│                                                │
│  Step 1: Hardware Event                        │
│  ┌──────────────────────────────────────┐      │
│  │ Device generates interrupt           │      │
│  │ Signal sent to interrupt controller  │      │
│  └──────────────┬───────────────────────┘      │
│                 │                              │
│                 ▼                              │
│  Step 2: Interrupt Controller                  │
│  ┌──────────────────────────────────────┐      │
│  │ Controller detects interrupt         │      │
│  │ Identifies source (hwirq number)     │      │
│  │ Signals CPU via IRQ line             │      │
│  └──────────────┬───────────────────────┘      │
│                 │                              │
│                 ▼                              │
│  Step 3: CPU Exception                         │
│  ┌──────────────────────────────────────┐      │
│  │ CPU stops current execution          │      │
│  │ Saves state (registers, PC, etc.)    │      │
│  │ Switches to IRQ mode                 │      │
│  │ Jumps to exception vector            │      │
│  └──────────────┬───────────────────────┘      │
│                 │                              │
│                 ▼                              │
│  Step 4: Architecture Handler                  │
│  ┌──────────────────────────────────────┐      │
│  │ Execute arch-specific IRQ entry      │      │
│  │ Save additional context              │      │
│  │ Call handle_arch_irq                 │      │
│  └──────────────┬───────────────────────┘      │
│                 │                              │
│                 ▼                              │
│  Step 5: GIC Handler (gic_handle_irq)          │
│  ┌──────────────────────────────────────┐      │
│  │ Read GIC interrupt acknowledge reg   │      │
│  │ Get hwirq from GIC                   │      │
│  │ Call handle_domain_irq()             │      │
│  └──────────────┬───────────────────────┘      │
│                 │                              │
│                 ▼                              │
│  Step 6: IRQ Domain Translation                │
│  ┌──────────────────────────────────────┐      │
│  │ Translate hwirq → virq               │      │
│  │ Lookup in IRQ domain                 │      │
│  │ Call generic_handle_irq(virq)        │      │
│  └──────────────┬───────────────────────┘      │ 
│                 │                              │
│                 ▼                              │
│  Step 7: Flow Handler                          │
│  ┌──────────────────────────────────────┐      │
│  │ Execute high-level flow handler      │      │
│  │ (handle_level_irq, handle_edge_irq)  │      │
│  │ Mask/ack interrupt as needed         │      │
│  │ Call desc->handle_irq()              │      │
│  └──────────────┬───────────────────────┘      │
│                 │                              │
│                 ▼                              │
│  Step 8: Device Handler                        │
│  ┌──────────────────────────────────────┐      │
│  │ Execute YOUR interrupt handler       │      │
│  │ Handle device-specific work          │      │ 
│  │ Return IRQ_HANDLED or IRQ_NONE       │      │
│  └──────────────────────────────────────┘      │
│                                                │
└────────────────────────────────────────────────┘
```

### 16.2.2 Code Path Example

**Let's trace a real interrupt:**

```c
/* Example: Button press on GPIO expander connected to SoC GPIO */

// 1. Button pressed → GPIO expander detects change
//    Expander pulls INT line LOW

// 2. SoC GPIO controller receives signal on GPIO4_29
//    GIC receives interrupt from GPIO4 controller

// 3. GIC handler executes (in drivers/irqchip/irq-gic.c)
void gic_handle_irq(struct pt_regs *regs)
{
    u32 irqstat, irqnr;

    // Read interrupt acknowledge register
    irqstat = readl_relaxed(gic_cpu_base + GIC_CPU_INTACK);
    irqnr = irqstat & GICC_IAR_INT_ID_MASK;  // hwirq from GIC

    // Translate and handle
    handle_domain_irq(gic_domain, irqnr, regs);
}

// 4. handle_domain_irq() translates hwirq→virq
int handle_domain_irq(struct irq_domain *domain,
                      unsigned int hwirq,
                      struct pt_regs *regs)
{
    struct irq_desc *desc;

    // Translate hwirq to virq using domain
    unsigned int irq = irq_find_mapping(domain, hwirq);

    // Get IRQ descriptor
    desc = irq_to_desc(irq);

    // Call generic handler
    generic_handle_irq_desc(desc);

    return 0;
}

// 5. generic_handle_irq_desc() calls flow handler
static inline void generic_handle_irq_desc(struct irq_desc *desc)
{
    // This calls handle_level_irq, handle_edge_irq, etc.
    desc->handle_irq(desc);
}

// 6. Flow handler (example: handle_level_irq)
void handle_level_irq(struct irq_desc *desc)
{
    raw_spin_lock(&desc->lock);

    // Mask interrupt
    mask_ack_irq(desc);

    // Call registered handler
    handle_irq_event(desc);

    // Unmask interrupt
    cond_unmask_irq(desc);

    raw_spin_unlock(&desc->lock);
}

// 7. handle_irq_event() calls YOUR handler
irqreturn_t handle_irq_event(struct irq_desc *desc)
{
    struct irqaction *action;

    for_each_action_of_desc(desc, action) {
        // This is YOUR handler!
        res = action->handler(irq, action->dev_id);
    }

    return ret;
}

// 8. Your GPIO controller handler executes
static irqreturn_t gpio4_irq_handler(int irq, void *dev_id)
{
    struct gpio_chip *chip = dev_id;
    u32 status;

    // Read GPIO interrupt status
    status = readl(gpio_base + GPIO_ISR);

    // GPIO4_29 is set
    if (status & (1 << 29)) {
        // Find virq for this GPIO
        int virq = irq_find_mapping(chip->irq.domain, 29);

        // Call expander's IRQ handler
        generic_handle_irq(virq);
    }

    return IRQ_HANDLED;
}

// 9. GPIO expander handler executes
static irqreturn_t mcp23016_irq_handler(int irq, void *dev_id)
{
    struct mcp23016 *mcp = dev_id;
    u16 status;

    // Read expander interrupt status (via I2C)
    status = i2c_smbus_read_word_data(mcp->client, INT_STATUS_REG);

    // GPIO 2 caused interrupt (button press)
    if (status & (1 << 2)) {
        int virq = irq_find_mapping(mcp->irq_domain, 2);

        // Call button driver handler
        handle_nested_irq(virq);
    }

    return IRQ_HANDLED;
}

// 10. Button driver handler executes
static irqreturn_t button_irq_handler(int irq, void *dev_id)
{
    struct button_dev *button = dev_id;

    pr_info("Button pressed!\n");

    // Report input event
    input_report_key(button->input, KEY_ENTER, 1);
    input_sync(button->input);

    return IRQ_HANDLED;
}
```

### 16.2.3 IRQ Request and Registration

**How handlers get registered:**

```c
/* Device driver requests interrupt */
int request_irq(unsigned int irq,
                irq_handler_t handler,
                unsigned long flags,
                const char *name,
                void *dev_id);

/* Threaded variant */
int request_threaded_irq(unsigned int irq,
                         irq_handler_t handler,
                         irq_handler_t thread_fn,
                         unsigned long flags,
                         const char *name,
                         void *dev_id);

/* What happens internally: */

// 1. Allocate irqaction structure
struct irqaction *action;
action = kzalloc(sizeof(*action), GFP_KERNEL);

// 2. Fill in parameters
action->handler = handler;
action->thread_fn = thread_fn;
action->flags = flags;
action->name = name;
action->dev_id = dev_id;

// 3. Register with IRQ core
__setup_irq(irq, desc, action);

// 4. Add to descriptor's action list
action->next = desc->action;
desc->action = action;

// 5. Enable IRQ
desc->irq_data.chip->irq_unmask(&desc->irq_data);
```

---

## 16.3 Interrupt Controllers Hierarchy

### 16.3.1 Multi-Level Hierarchy

**Real-world example: Button on GPIO expander:**

```
┌────────────────────────────────────────────────┐
│  Three-Level Interrupt Hierarchy               │
├────────────────────────────────────────────────┤
│                                                │
│  Level 1: Root Interrupt Controller (GIC)      │
│  ┌──────────────────────────────────────┐      │
│  │ ARM GIC(Generic Interrupt Controller)│      │
│  │                                      │      │
│  │ IRQ 96 ──────────┐                   │      │
│  │ IRQ 97           │                   │      │
│  │ ...              │                   │      │
│  └───────────────── ┼───────────────────┘      │
│                     │                          │
│                     │ IRQ 96 fires             │
│                     ▼                          │
│  Level 2: GPIO Controller (SoC)                │
│  ┌──────────────────────────────────────┐      │
│  │ GPIO4 Controller                     │      │
│  │                                      │      │
│  │ GPIO4_0                              │      │
│  │ ...                                  │      │
│  │ GPIO4_29 ────────┐                   │      │
│  │ ...              │                   │      │
│  │ GPIO4_31         │                   │      │
│  └──────────────────┼───────────────────┘      │
│                     │                          │
│                     │ INT line                 │
│                     ▼                          │
│  Level 3: GPIO Expander (I2C)                  │
│  ┌──────────────────────────────────────┐      │
│  │ MCP23016 I2C GPIO Expander           │      │
│  │                                      │      │
│  │ GPIO 0                               │      │
│  │ GPIO 1                               │      │
│  │ GPIO 2 ◄──── Button connected        │      │
│  │ ...                                  │      │
│  │ GPIO 15                              │      │
│  └──────────────────────────────────────┘      │
│                                                │
│  IRQ Path: Button → GPIO2 → INT → GPIO4_29     │
│            → GIC IRQ96 → Handlers chain        │
│                                                │
└────────────────────────────────────────────────┘
```

### 16.3.2 Interrupt Controller Chain

**Each level is an interrupt controller:**

```
┌────────────────────────────────────────────────┐
│  Interrupt Controller Characteristics          │
├────────────────────────────────────────────────┤
│                                                │
│  GIC (Level 1 - Root):                         │
│  • Direct connection to CPU                    │
│  • Memory-mapped registers                     │
│  • Fast access (no sleep)                      │
│  • Handles ~200 interrupt sources              │
│  • hwirq: 0-199                                │
│  • virq: Assigned by kernel                    │
│                                                │
│  GPIO Controller (Level 2 - Intermediate):     │
│  • Connected to GIC via one IRQ line           │
│  • Memory-mapped (SoC internal)                │
│  • Fast access (no sleep)                      │
│  • Multiplexes 32 GPIO interrupts              │
│  • hwirq: 0-31 (GPIO offsets)                  │
│  • virq: Assigned by kernel                    │
│  • Acts as both consumer and provider          │
│                                                │
│  GPIO Expander (Level 3 - Leaf):               │
│  • Connected to GPIO via INT pin               │
│  • I2C bus access (SLOW, may sleep!)           │
│  • Multiplexes 16 GPIO interrupts              │
│  • hwirq: 0-15 (GPIO offsets)                  │
│  • virq: Assigned by kernel                    │
│  • Pure provider (no children)                 │
│                                                │
└────────────────────────────────────────────────┘
```

---

## 16.4 IRQ Domain API Deep Dive

### 16.4.1 Why IRQ Domains?

**The problem IRQ domains solve:**

```
┌────────────────────────────────────────────────┐
│  Problem: Same hwirq, Different Controllers    │
├────────────────────────────────────────────────┤
│                                                │
│  Without IRQ domains:                          │
│                                                │
│  GPIO1 Controller:                             │
│    hwirq 0 → What virq?                        │
│    hwirq 1 → What virq?                        │
│                                                │
│  GPIO2 Controller:                             │
│    hwirq 0 → Same virq as GPIO1?? CONFLICT!    │
│    hwirq 1 → Same virq as GPIO1?? CONFLICT!    │
│                                                │
│  Solution: IRQ Domains                         │
│                                                │
│  Domain for GPIO1:                             │
│    hwirq 0 → virq 200                          │
│    hwirq 1 → virq 201                          │
│                                                │
│  Domain for GPIO2:                             │
│    hwirq 0 → virq 300                          │
│    hwirq 1 → virq 301                          │
│                                                │
│  Each controller has its own domain!           │
│  Same hwirq, different virq. No conflicts!     │
│                                                │
└────────────────────────────────────────────────┘
```

### 16.4.2 IRQ Domain Core Structures

**Required headers:**

```c
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/interrupt.h>
```

**Main structures:**

```c
/* IRQ domain - maps hwirq to virq for a controller */
struct irq_domain {
    struct list_head link;
    const char *name;
    const struct irq_domain_ops *ops;
    void *host_data;
    unsigned int hwirq_max;
    /* Internal fields... */
};

/* IRQ domain operations */
struct irq_domain_ops {
    /* Called when creating mapping */
    int (*map)(struct irq_domain *d,
               unsigned int virq,
               irq_hw_number_t hw);

    /* Called when removing mapping */
    void (*unmap)(struct irq_domain *d,
                  unsigned int virq);

    /* Translate DT interrupt specifier to hwirq */
    int (*xlate)(struct irq_domain *d,
                 struct device_node *node,
                 const u32 *intspec,
                 unsigned int intsize,
                 unsigned long *out_hwirq,
                 unsigned int *out_type);
};

/* IRQ descriptor - one per virtual IRQ */
struct irq_desc {
    struct irq_data irq_data;
    struct irqaction *action;
    irq_flow_handler_t handle_irq;
    /* Many more fields... */
};

/* IRQ chip - controller operations */
struct irq_chip {
    const char *name;
    void (*irq_mask)(struct irq_data *data);
    void (*irq_unmask)(struct irq_data *data);
    void (*irq_ack)(struct irq_data *data);
    int  (*irq_set_type)(struct irq_data *data, unsigned int flow_type);
    /* More callbacks... */
};
```

### 16.4.3 Creating IRQ Domains

**Three domain types:**

```c
/* 1. Linear mapping (most common for GPIO) */
struct irq_domain *irq_domain_add_linear(
    struct device_node *of_node,
    unsigned int size,
    const struct irq_domain_ops *ops,
    void *host_data);

/* 2. Tree mapping (sparse hwirq) */
struct irq_domain *irq_domain_add_tree(
    struct device_node *of_node,
    const struct irq_domain_ops *ops,
    void *host_data);

/* 3. Legacy mapping (fixed virq numbers) */
struct irq_domain *irq_domain_add_legacy(
    struct device_node *of_node,
    unsigned int size,
    unsigned int first_irq,
    irq_hw_number_t first_hwirq,
    const struct irq_domain_ops *ops,
    void *host_data);

/* Simple wrapper (uses linear or legacy) */
struct irq_domain *irq_domain_add_simple(
    struct device_node *of_node,
    unsigned int size,
    unsigned int first_irq,
    const struct irq_domain_ops *ops,
    void *host_data);
```

**Example: Creating linear domain:**

```c
struct my_irq_chip {
    struct irq_chip chip;
    struct irq_domain *domain;
    void __iomem *base;
    int nr_irqs;
};

static int my_probe(struct platform_device *pdev)
{
    struct my_irq_chip *priv;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->nr_irqs = 16;

    /* Create linear domain for 16 IRQs */
    priv->domain = irq_domain_add_linear(
        pdev->dev.of_node,          /* DT node */
        priv->nr_irqs,              /* Number of IRQs */
        &my_domain_ops,             /* Operations */
        priv);                      /* Private data */

    if (!priv->domain) {
        dev_err(&pdev->dev, "Failed to create IRQ domain\n");
        return -ENOMEM;
    }

    dev_info(&pdev->dev, "IRQ domain created for %d IRQs\n",
             priv->nr_irqs);

    return 0;
}
```

### 16.4.4 Implementing domain->map()

**The map() callback configures each IRQ:**

```c
static int my_domain_map(struct irq_domain *d,
                         unsigned int virq,
                         irq_hw_number_t hw)
{
    struct my_irq_chip *priv = d->host_data;

    /* Set IRQ chip for this virq */
    irq_set_chip_and_handler(virq,
                             &priv->chip,
                             handle_level_irq);

    /* Set chip data (can retrieve in handler) */
    irq_set_chip_data(virq, priv);

    /* Mark as valid */
    irq_set_noprobe(virq);

    return 0;
}
```

**For different flow types:**

```c
/* Level-triggered (most common) */
irq_set_chip_and_handler(virq, &my_chip, handle_level_irq);

/* Edge-triggered */
irq_set_chip_and_handler(virq, &my_chip, handle_edge_irq);

/* Simple (no masking needed) */
irq_set_chip_and_handler(virq, &my_chip, handle_simple_irq);

/* Nested threaded */
irq_set_chip_and_handler(virq, &my_chip, handle_nested_irq);
```

### 16.4.5 Implementing domain->xlate()

**xlate translates Device Tree interrupt specifier:**

```c
/*
 * Standard two-cell xlate
 * DT format: interrupts = <hwirq flags>;
 */
static int my_domain_xlate(struct irq_domain *d,
                           struct device_node *node,
                           const u32 *intspec,
                           unsigned int intsize,
                           unsigned long *out_hwirq,
                           unsigned int *out_type)
{
    if (intsize < 2)
        return -EINVAL;

    *out_hwirq = intspec[0];  /* First cell: hwirq */
    *out_type = intspec[1];   /* Second cell: flags */

    return 0;
}

/* Domain ops structure */
static const struct irq_domain_ops my_domain_ops = {
    .map = my_domain_map,
    .xlate = my_domain_xlate,
};
```

**Kernel provides standard xlate helpers:**

```c
/* One-cell: just hwirq */
.xlate = irq_domain_xlate_onecell

/* Two-cell: hwirq + flags (most common) */
.xlate = irq_domain_xlate_twocell

/* One or two cells */
.xlate = irq_domain_xlate_onetwocell
```

### 16.4.6 Creating and Finding Mappings

**Create mapping (hwirq → virq):**

```c
unsigned int irq_create_mapping(struct irq_domain *domain,
                                 irq_hw_number_t hwirq);
```

**Find existing mapping:**

```c
unsigned int irq_find_mapping(struct irq_domain *domain,
                               irq_hw_number_t hwirq);
```

**Example usage:**

```c
/* In probe: create mappings */
static int my_probe(struct platform_device *pdev)
{
    struct my_irq_chip *priv;
    unsigned int virq;
    int i;

    /* ... domain creation ... */

    /* Create mapping for each hwirq */
    for (i = 0; i < priv->nr_irqs; i++) {
        virq = irq_create_mapping(priv->domain, i);
        if (!virq) {
            dev_err(&pdev->dev,
                    "Failed to map hwirq %d\n", i);
            return -EINVAL;
        }
        dev_dbg(&pdev->dev,
                "hwirq %d → virq %u\n", i, virq);
    }

    return 0;
}

/* In IRQ handler: find mapping */
static irqreturn_t my_irq_handler(int irq, void *dev_id)
{
    struct my_irq_chip *priv = dev_id;
    u32 status;
    unsigned int virq;
    int i;

    /* Read interrupt status register */
    status = readl(priv->base + IRQ_STATUS);

    /* Handle each active interrupt */
    for (i = 0; i < priv->nr_irqs; i++) {
        if (status & (1 << i)) {
            /* Find virq for this hwirq */
            virq = irq_find_mapping(priv->domain, i);
            if (virq)
                generic_handle_irq(virq);
        }
    }

    return IRQ_HANDLED;
}
```

---

## Summary

In this part, we covered:

**Key Concepts:**

1. **Interrupt Architecture**
    - Three-layer Linux IRQ architecture
    - Device drivers → IRQ core → Controllers
    - Hardware vs virtual IRQ numbers
2. **IRQ Propagation**
    - Complete flow from hardware to handler
    - 10-step propagation mechanism
    - Code path examples with real functions
3. **Interrupt Controller Hierarchy**
    - Multi-level controller chains
    - GIC → GPIO → GPIO Expander example
    - Three levels of multiplexing
4. **IRQ Domain API**
    - Why domains are needed
    - Core structures (irq_domain, irq_domain_ops)
    - Creating domains (linear, tree, legacy)
    - Implementing map() callback
    - Implementing xlate() callback
    - Creating and finding mappings