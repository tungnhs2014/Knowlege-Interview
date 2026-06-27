# Part 2. Probe, Remove & Resource Management

This part dives deep into probe/remove implementation, resource management APIs, and platform data handling - the core of platform driver development.

---

## 5.2 The Probe Function in Detail

### Probe Function Workflow

```
Complete probe() Implementation Flow:

┌─────────────────────────────────────────────────────────┐
│ 1. Device Verification                                  │
│    └─ Verify hardware exists (if possible)              │
└──────────────────────┬──────────────────────────────────┘
                       ↓
┌─────────────────────────────────────────────────────────┐
│ 2. Extract Resources                                    │
│    ├─ Memory regions (platform_get_resource)            │
│    ├─ IRQ numbers (platform_get_irq)                    │
│    ├─ DMA channels                                      │
│    └─ Platform data (dev_get_platdata)                  │
└──────────────────────┬──────────────────────────────────┘
                       ↓
┌─────────────────────────────────────────────────────────┐
│ 3. Memory Mapping                                       │
│    ├─ Request memory region                             │
│    ├─ Map to virtual address (ioremap)                  │
│    └─ Or use devm_ioremap_resource (recommended)        │
└──────────────────────┬──────────────────────────────────┘
                       ↓
┌─────────────────────────────────────────────────────────┐
│ 4. Allocate Private Data                                │
│    ├─ kzalloc() or devm_kzalloc()                       │
│    └─ platform_set_drvdata() to save pointer            │
└──────────────────────┬──────────────────────────────────┘
                       ↓
┌─────────────────────────────────────────────────────────┐
│ 5. Initialize Hardware                                  │
│    ├─ Reset device                                      │
│    ├─ Configure registers                               │
│    ├─ Enable clocks (if needed)                         │
│    └─ Setup DMA (if needed)                             │
└──────────────────────┬──────────────────────────────────┘
                       ↓
┌─────────────────────────────────────────────────────────┐
│ 6. Request Interrupts                                   │
│    └─ request_irq() or devm_request_irq()               │
└──────────────────────┬──────────────────────────────────┘
                       ↓
┌─────────────────────────────────────────────────────────┐
│ 7. Register with Subsystem                              │
│    ├─ Character device: cdev_add()                      │
│    ├─ Input device: input_register_device()             │
│    ├─ Network device: register_netdev()                 │
│    └─ Or other framework                                │
└──────────────────────┬──────────────────────────────────┘
                       ↓
┌─────────────────────────────────────────────────────────┐
│ 8. Return Success                                       │
│    └─ return 0;                                         │
└─────────────────────────────────────────────────────────┘

On ANY error: Clean up and return error code!
```

### Error Handling with goto

**Pattern:** Use goto for cleanup on error

```c
static int my_probe(struct platform_device *pdev)
{
    struct resource *res;
    struct my_device *mydev;
    void __iomem *base;
    int irq;
    int ret;

    /* Step 1: Allocate private data */
    mydev = kzalloc(sizeof(*mydev), GFP_KERNEL);
    if (!mydev) {
        return -ENOMEM;
    }

    /* Step 2: Get memory resource */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        ret = -ENODEV;
        goto err_free_dev;
    }

    /* Step 3: Request and map memory */
    if (!request_mem_region(res->start, resource_size(res), pdev->name)) {
        ret = -EBUSY;
        goto err_free_dev;
    }

    base = ioremap(res->start, resource_size(res));
    if (!base) {
        ret = -ENOMEM;
        goto err_release_mem;
    }

    mydev->base = base;

    /* Step 4: Get IRQ */
    irq = platform_get_irq(pdev, 0);
    if (irq < 0) {
        ret = irq;
        goto err_unmap;
    }

    /* Step 5: Request IRQ */
    ret = request_irq(irq, my_irq_handler, 0, pdev->name, mydev);
    if (ret) {
        goto err_unmap;
    }

    mydev->irq = irq;

    /* Step 6: Initialize device */
    ret = my_device_init(mydev);
    if (ret) {
        goto err_free_irq;
    }

    /* Step 7: Register with framework */
    ret = my_register_device(mydev);
    if (ret) {
        goto err_shutdown_device;
    }

    /* Save private data */
    platform_set_drvdata(pdev, mydev);

    dev_info(&pdev->dev, "Device probed successfully\n");
    return 0;

/* Error handling - cleanup in reverse order */
err_shutdown_device:
    my_device_shutdown(mydev);
err_free_irq:
    free_irq(irq, mydev);
err_unmap:
    iounmap(base);
err_release_mem:
    release_mem_region(res->start, resource_size(res));
err_free_dev:
    kfree(mydev);
    return ret;
}
```

**Why use goto?**

- ✅ Single cleanup path
- ✅ Prevents duplicate code
- ✅ Easy to maintain
- ✅ Hard to forget cleanup steps
- ✅ Standard kernel pattern

---

## 5.2.2 Resource Management

### Understanding Resources

**Resources** represent hardware properties that the device needs:

```c
/* Defined in include/linux/ioport.h */
struct resource {
    resource_size_t start;     /* Start address/number */
    resource_size_t end;       /* End address/number */
    const char *name;          /* Resource name */
    unsigned long flags;       /* Resource type and flags */
    struct resource *parent;
    struct resource *sibling;
    struct resource *child;
};
```

### Resource Types

```c
/* Resource type flags */
#define IORESOURCE_IO      0x00000100  /* PCI/ISA I/O ports */
#define IORESOURCE_MEM     0x00000200  /* Memory regions */
#define IORESOURCE_REG     0x00000300  /* Register offsets */
#define IORESOURCE_IRQ     0x00000400  /* IRQ line */
#define IORESOURCE_DMA     0x00000800  /* DMA channels */
#define IORESOURCE_BUS     0x00001000  /* Bus */
```

**Resource interpretation:**

| Type | start | end | Meaning |
| --- | --- | --- | --- |
| **MEM** | 0x80000000 | 0x80000FFF | Memory region 4KB at 0x80000000 |
| **IRQ** | 56 | 56 | IRQ number 56 |
| **DMA** | 3 | 3 | DMA channel 3 |
| **IO** | 0x3F8 | 0x3FF | I/O ports 0x3F8-0x3FF |

**For IRQ, DMA, BUS:** start and end must be the same (just a number, not a range)

### platform_get_resource()

```c
struct resource *platform_get_resource(struct platform_device *dev,
                                      unsigned int type,
                                      unsigned int num);

/* Parameters:
 * dev  - Platform device
 * type - Resource type (IORESOURCE_MEM, IORESOURCE_IRQ, etc.)
 * num  - Resource index (0 for first, 1 for second, etc.)
 *
 * Returns: Pointer to resource, or NULL if not found
 */
```

**Example:**

```c
struct resource *mem_res;
struct resource *io_res;

/* Get first memory resource */
mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
if (!mem_res) {
    dev_err(&pdev->dev, "No memory resource\n");
    return -ENODEV;
}

pr_info("Memory resource: 0x%08llx - 0x%08llx\n",
        (unsigned long long)mem_res->start,
        (unsigned long long)mem_res->end);

/* Get second memory resource (if exists) */
mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
if (mem_res) {
    pr_info("Second memory resource: 0x%08llx - 0x%08llx\n",
            (unsigned long long)mem_res->start,
            (unsigned long long)mem_res->end);
}
```

### platform_get_irq()

```c
int platform_get_irq(struct platform_device *dev, unsigned int num);

/* Parameters:
 * dev - Platform device
 * num - IRQ index (0 for first IRQ)
 *
 * Returns: IRQ number on success, negative error code on failure
 */
```

**Example:**

```c
int irq;

irq = platform_get_irq(pdev, 0);
if (irq < 0) {
    dev_err(&pdev->dev, "No IRQ resource\n");
    return irq;
}

pr_info("IRQ number: %d\n", irq);

/* Request IRQ */
ret = request_irq(irq, my_handler, 0, "my-device", priv);
if (ret) {
    dev_err(&pdev->dev, "Failed to request IRQ\n");
    return ret;
}
```

### Memory Mapping

**Steps to access memory-mapped registers:**

```
1. Get resource           → platform_get_resource(IORESOURCE_MEM)
2. Request region         → request_mem_region()
3. Map to virtual address → ioremap()
4. Access registers       → readl(), writel()
5. Unmap                  → iounmap()
6. Release region         → release_mem_region()
```

**Manual approach:**

```c
struct resource *res;
void __iomem *base;

/* Get memory resource */
res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
if (!res)
    return -ENODEV;

/* Request memory region */
if (!request_mem_region(res->start, resource_size(res), pdev->name)) {
    dev_err(&pdev->dev, "Memory region already in use\n");
    return -EBUSY;
}

/* Map to virtual address */
base = ioremap(res->start, resource_size(res));
if (!base) {
    release_mem_region(res->start, resource_size(res));
    return -ENOMEM;
}

/* Now you can access registers */
writel(0x1234, base + REG_CONTROL);
val = readl(base + REG_STATUS);

/* In remove():
 * iounmap(base);
 * release_mem_region(res->start, resource_size(res));
 */
```

**Managed approach (recommended):**

```c
struct resource *res;
void __iomem *base;

/* Get memory resource */
res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
if (!res)
    return -ENODEV;

/* Request and map in one call - auto cleanup! */
base = devm_ioremap_resource(&pdev->dev, res);
if (IS_ERR(base))
    return PTR_ERR(base);

/* Use it */
writel(0x1234, base + REG_CONTROL);

/* No cleanup needed in remove() - automatic! */
```

### Managed Resources (devm_*)

*The devm_ family automatically frees resources when device is removed.**

**Available managed APIs:**

| Traditional | Managed | Auto-freed |
| --- | --- | --- |
| `kzalloc()` | `devm_kzalloc()` | Yes |
| `kmalloc()` | `devm_kmalloc()` | Yes |
| `ioremap()` | `devm_ioremap()` | Yes |
| `request_irq()` | `devm_request_irq()` | Yes |
| `clk_get()` | `devm_clk_get()` | Yes |
| `regulator_get()` | `devm_regulator_get()` | Yes |
| `gpiod_get()` | `devm_gpiod_get()` | Yes |

*Example using devm_ functions:**

```c
static int my_probe(struct platform_device *pdev)
{
    struct my_device *mydev;
    struct resource *res;
    void __iomem *base;
    int irq, ret;

    /* Allocate with devm - auto freed */
    mydev = devm_kzalloc(&pdev->dev, sizeof(*mydev), GFP_KERNEL);
    if (!mydev)
        return -ENOMEM;

    /* Get and map memory with devm - auto unmapped */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(base))
        return PTR_ERR(base);

    mydev->base = base;

    /* Get IRQ with devm - auto freed */
    irq = platform_get_irq(pdev, 0);
    if (irq < 0)
        return irq;

    ret = devm_request_irq(&pdev->dev, irq, my_handler,
                          0, pdev->name, mydev);
    if (ret)
        return ret;

    /* All resources auto-freed on error or in remove() */

    platform_set_drvdata(pdev, mydev);
    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    /* Nothing to free - all managed! */
    dev_info(&pdev->dev, "Device removed\n");
    return 0;
}
```

**Benefits of devm_*:**

- ✅ Automatic cleanup
- ✅ Simpler code
- ✅ Less error-prone
- ✅ No memory leaks
- ✅ Recommended for new drivers

---

## 5.2.3 Platform Data

### What is Platform Data?

**Platform data** is custom data passed from device to driver - anything that's not a standard resource (MEM, IRQ, DMA).

**Examples:**

- GPIO pin numbers
- Device-specific configuration
- Hardware variant information
- Board-specific parameters

### Platform Data Structure

```c
/* Example: Custom platform data for GPIO device */
struct gpio_platform_data {
    int reset_gpio;
    int power_gpio;
    int led_gpio;
    bool active_low;
    unsigned int debounce_interval;
};
```

### Passing Platform Data (Legacy Method)

**In board file or device registration:**

```c
/* Define platform data */
static struct gpio_platform_data my_gpio_pdata = {
    .reset_gpio = 23,
    .power_gpio = 45,
    .led_gpio = 67,
    .active_low = true,
    .debounce_interval = 100,
};

/* Platform device with platform data */
static struct platform_device my_gpio_device = {
    .name = "my-gpio-controller",
    .id = -1,
    .dev = {
        .platform_data = &my_gpio_pdata,
    },
};
```

### Extracting Platform Data in Driver

```c
static int my_gpio_probe(struct platform_device *pdev)
{
    struct gpio_platform_data *pdata;

    /* Get platform data */
    pdata = dev_get_platdata(&pdev->dev);
    if (!pdata) {
        dev_err(&pdev->dev, "No platform data\n");
        return -EINVAL;
    }

    /* Use the data */
    pr_info("Reset GPIO: %d\n", pdata->reset_gpio);
    pr_info("Power GPIO: %d\n", pdata->power_gpio);
    pr_info("LED GPIO: %d\n", pdata->led_gpio);
    pr_info("Active low: %d\n", pdata->active_low);

    /* Request GPIOs */
    ret = gpio_request(pdata->reset_gpio, "reset");
    if (ret)
        return ret;

    /* Configure device based on platform data */
    /* ... */

    return 0;
}
```

### Complete Example: Platform Device with Resources and Data

**Device registration:**

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/ioport.h>

/* Platform data structure */
struct my_device_pdata {
    int mode;
    int speed;
    const char *config;
};

/* Platform data */
static struct my_device_pdata my_pdata = {
    .mode = 2,
    .speed = 100000,
    .config = "default-config",
};

/* Resources */
static struct resource my_resources[] = {
    /* Memory region */
    {
        .start = 0x80000000,
        .end   = 0x80000FFF,
        .flags = IORESOURCE_MEM,
        .name  = "control-regs",
    },
    /* Second memory region */
    {
        .start = 0x80001000,
        .end   = 0x80001FFF,
        .flags = IORESOURCE_MEM,
        .name  = "data-buffer",
    },
    /* IRQ */
    {
        .start = 56,
        .end   = 56,
        .flags = IORESOURCE_IRQ,
        .name  = "device-irq",
    },
    /* DMA channel */
    {
        .start = 3,
        .end   = 3,
        .flags = IORESOURCE_DMA,
        .name  = "rx-dma",
    },
};

/* Platform device */
static struct platform_device my_platform_device = {
    .name = "my-complex-device",
    .id = -1,
    .num_resources = ARRAY_SIZE(my_resources),
    .resource = my_resources,
    .dev = {
        .platform_data = &my_pdata,
    },
};

static int __init my_device_init(void)
{
    return platform_device_register(&my_platform_device);
}

static void __exit my_device_exit(void)
{
    platform_device_unregister(&my_platform_device);
}

module_init(my_device_init);
module_exit(my_device_exit);

MODULE_LICENSE("GPL");
```

**Driver that uses resources and data:**

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/interrupt.h>

/* Platform data structure (must match device) */
struct my_device_pdata {
    int mode;
    int speed;
    const char *config;
};

/* Driver private data */
struct my_device {
    void __iomem *ctrl_base;
    void __iomem *data_base;
    int irq;
    int dma_chan;
    struct my_device_pdata *pdata;
};

static irqreturn_t my_irq_handler(int irq, void *dev_id)
{
    pr_info("IRQ %d triggered\n", irq);
    return IRQ_HANDLED;
}

static int my_probe(struct platform_device *pdev)
{
    struct my_device *mydev;
    struct my_device_pdata *pdata;
    struct resource *res;
    int ret;

    dev_info(&pdev->dev, "Probing device\n");

    /* Allocate private data */
    mydev = devm_kzalloc(&pdev->dev, sizeof(*mydev), GFP_KERNEL);
    if (!mydev)
        return -ENOMEM;

    /* Get platform data */
    pdata = dev_get_platdata(&pdev->dev);
    if (!pdata) {
        dev_err(&pdev->dev, "No platform data\n");
        return -EINVAL;
    }

    mydev->pdata = pdata;

    dev_info(&pdev->dev, "Platform data: mode=%d, speed=%d, config=%s\n",
             pdata->mode, pdata->speed, pdata->config);

    /* Get first memory resource (control registers) */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    mydev->ctrl_base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(mydev->ctrl_base))
        return PTR_ERR(mydev->ctrl_base);

    dev_info(&pdev->dev, "Control regs mapped: 0x%px\n", mydev->ctrl_base);

    /* Get second memory resource (data buffer) */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    mydev->data_base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(mydev->data_base))
        return PTR_ERR(mydev->data_base);

    dev_info(&pdev->dev, "Data buffer mapped: 0x%px\n", mydev->data_base);

    /* Get IRQ */
    mydev->irq = platform_get_irq(pdev, 0);
    if (mydev->irq < 0)
        return mydev->irq;

    dev_info(&pdev->dev, "IRQ: %d\n", mydev->irq);

    /* Request IRQ */
    ret = devm_request_irq(&pdev->dev, mydev->irq, my_irq_handler,
                          0, pdev->name, mydev);
    if (ret) {
        dev_err(&pdev->dev, "Failed to request IRQ\n");
        return ret;
    }

    /* Get DMA channel */
    res = platform_get_resource(pdev, IORESOURCE_DMA, 0);
    if (res) {
        mydev->dma_chan = res->start;
        dev_info(&pdev->dev, "DMA channel: %d\n", mydev->dma_chan);
    }

    /* Save private data */
    platform_set_drvdata(pdev, mydev);

    dev_info(&pdev->dev, "Device probed successfully\n");
    return 0;
}

static int my_remove(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "Device removed\n");
    /* All resources auto-freed by devm_* */
    return 0;
}

static struct platform_driver my_driver = {
    .probe = my_probe,
    .remove = my_remove,
    .driver = {
        .name = "my-complex-device",
        .owner = THIS_MODULE,
    },
};

module_platform_driver(my_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Platform driver with resources and data");
```

**Testing:**

```bash
# Load device
sudo insmod complex_device.ko

# Load driver
sudo insmod complex_driver.ko

# Check dmesg
dmesg | tail -15
# Output:
# Probing device
# Platform data: mode=2, speed=100000, config=default-config
# Control regs mapped: 0xffff...
# Data buffer mapped: 0xffff...
# IRQ: 56
# DMA channel: 3
# Device probed successfully
```

---

## 5.2.4 The Remove Function

### Remove Function Implementation

```c
static int my_remove(struct platform_device *pdev)
{
    struct my_device *mydev = platform_get_drvdata(pdev);

    dev_info(&pdev->dev, "Removing device\n");

    /* 1. Unregister from subsystem/framework */
    my_unregister_from_framework(mydev);

    /* 2. Stop hardware */
    my_device_shutdown(mydev);

    /* 3. Free IRQ (if not using devm_request_irq) */
    if (mydev->irq >= 0)
        free_irq(mydev->irq, mydev);

    /* 4. Unmap memory (if not using devm_ioremap) */
    if (mydev->base)
        iounmap(mydev->base);

    /* 5. Release memory region (if not using devm_) */
    if (mydev->mem_res)
        release_mem_region(mydev->mem_res->start,
                          resource_size(mydev->mem_res));

    /* 6. Free allocated memory (if not using devm_kzalloc) */
    kfree(mydev);

    dev_info(&pdev->dev, "Device removed successfully\n");
    return 0;
}
```

*With devm_ functions:**

```c
static int my_remove(struct platform_device *pdev)
{
    struct my_device *mydev = platform_get_drvdata(pdev);

    /* 1. Unregister from framework */
    my_unregister_from_framework(mydev);

    /* 2. Stop hardware */
    my_device_shutdown(mydev);

    /* That's it! IRQ, memory, allocations auto-freed */

    dev_info(&pdev->dev, "Device removed\n");
    return 0;
}
```

---

## Summary

This part covered probe/remove and resource management:

**Key Topics:**

- ✅ Probe function complete workflow
- ✅ Error handling with goto pattern
- ✅ Resource types (MEM, IRQ, DMA, IO, BUS, REG)
- ✅ platform_get_resource() API
- ✅ platform_get_irq() helper
- ✅ Memory mapping (ioremap)
- ✅ Managed resources (devm_*)
- ✅ Platform data structures
- ✅ dev_get_platdata() extraction
- ✅ platform_set/get_drvdata()
- ✅ Remove function cleanup
- ✅ Complete working examples