# Part 1. Platform Bus & Driver Basics

Platform drivers are a fundamental concept in Linux kernel driver development. They provide a way to handle devices that are not connected through conventional discoverable buses like PCI, USB, or I2C. This chapter covers everything you need to know about platform drivers.

---

## 5.1 Introduction to Platform Drivers

### What are Platform Drivers?

**Platform drivers** handle devices that are **not on a conventional, discoverable bus**. These are typically:

- SoC (System-on-Chip) integrated peripherals
- Memory-mapped devices
- Devices on non-enumerable buses
- GPIO controllers, timers, UARTs
- Custom hardware controllers

**Key characteristic:** The kernel cannot automatically detect these devices - we must explicitly tell the kernel about them.

### Platform Bus: The Pseudo-Bus

```
Real Buses (Auto-Discovery):          Pseudo Platform Bus (Manual):
┌─────────────────────────┐          ┌──────────────────────────┐
│  PCI Bus                │          │  Platform Bus            │
│  ├─ Device 1 detected   │          │  ├─ UART0 (declared)     │
│  ├─ Device 2 detected   │          │  ├─ GPIO controller      │
│  └─ Device 3 detected   │          │  ├─ Timer                │
│                         │          │  └─ Custom hardware      │
│  USB Bus                │          │                          │
│  ├─ Device A detected   │          │  No auto-detection!      │
│  ├─ Device B detected   │          │  Must be declared        │
│  └─ Device C detected   │          │                          │
└─────────────────────────┘          └──────────────────────────┘

Auto-negotiation ✅                   Manual declaration ✅
Plug and play ✅                      Fixed in design ✅
```

**The platform bus is a pseudo-bus** - it doesn't physically exist. It's a software abstraction that allows the kernel to use the same driver model for non-discoverable devices.

### When to Use Platform Drivers?

**Use platform drivers when:**

- ✅ Device is not on PCI, USB, I2C, SPI, or other standard bus
- ✅ Device is memory-mapped and integrated into the SoC
- ✅ Device location is known at compile/boot time
- ✅ Device cannot be auto-detected

**Examples:**

```
Platform Devices:                NOT Platform Devices:
├─ On-chip UART controllers     ├─ USB webcam (USB bus)
├─ GPIO controllers              ├─ PCI network card (PCI bus)
├─ On-chip timers               ├─ I2C sensor (I2C bus)
├─ Memory controllers           ├─ SPI flash (SPI bus)
├─ Custom FPGA peripherals      └─ SATA drive (SATA bus)
└─ DMA controllers

BUT WAIT! I2C and SPI controllers themselves
are platform devices! The I2C/SPI sensor is then
a client device on that bus.
```

### Platform Driver Architecture

```
                    Platform Driver Model
┌─────────────────────────────────────────────────────────┐
│                    Kernel Space                         │
│                                                         │
│  ┌────────────────────────────────────────────────────┐ │
│  │         Platform Bus Core                          │ │
│  │  (drivers/base/platform.c)                         │ │
│  │                                                    │ │
│  │  ┌──────────────┐        ┌──────────────┐          │ │
│  │  │ Driver List  │        │ Device List  │          │ │
│  │  │              │        │              │          │ │
│  │  │ uart_driver  │        │ uart0        │          │ │
│  │  │ gpio_driver  │        │ uart1        │          │ │
│  │  │ timer_driver │        │ gpio_ctrl    │          │ │
│  │  └──────────────┘        │ timer0       │          │ │
│  │                          └──────────────┘          │ │
│  │                                                    │ │
│  │         Matching Algorithm:                        │ │
│  │         Compare device.name with driver.name       │ │
│  │                    ↓                               │ │
│  │              Match found!                          │ │
│  │                    ↓                               │ │
│  │         Call driver->probe(device)                 │ │
│  └────────────────────────────────────────────────────┘ │
│                                                         │
│  ┌────────────────────────────────────────────────────┐ │
│  │  Your Platform Driver                              │ │
│  │                                                    │ │
│  │  static struct platform_driver my_driver = {       │ │
│  │      .probe  = my_probe,                           │ │
│  │      .remove = my_remove,                          │ │
│  │      .driver = {                                   │ │
│  │          .name = "my-device",                      │ │
│  │      },                                            │ │
│  │  };                                                │ │
│  └────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────┘
```

---

## 5.1.2 Platform Driver Structure

### The struct platform_driver

**Definition:**

```c
/* Defined in include/linux/platform_device.h */
struct platform_driver {
    int (*probe)(struct platform_device *);
    int (*remove)(struct platform_device *);
    void (*shutdown)(struct platform_device *);
    int (*suspend)(struct platform_device *, pm_message_t state);
    int (*resume)(struct platform_device *);
    struct device_driver driver;
    const struct platform_device_id *id_table;
    bool prevent_deferred_probe;
};
```

**Key fields explained:**

| Field | Purpose |
| --- | --- |
| `probe` | Called when device matches driver |
| `remove` | Called when device is removed or driver unloaded |
| `shutdown` | Called at system shutdown |
| `suspend` | Power management - suspend device |
| `resume` | Power management - resume device |
| `driver` | Generic driver structure (name, owner, etc.) |
| `id_table` | Device ID table for matching |

### The probe() Function

**Function prototype:**

```c
int (*probe)(struct platform_device *pdev);

/* Parameters:
 * pdev - Platform device that matched this driver
 *
 * Returns:
 *   0       - Success
 *   -ENXIO  - Device not found
 *   -ENOMEM - Out of memory
 *   -EINVAL - Invalid parameters
 */
```

**What probe() should do:**

```
probe() Function Responsibilities:

1. Verify device exists
   └─ Check hardware presence (if possible)

2. Extract resources
   ├─ Memory regions (platform_get_resource)
   ├─ IRQ numbers (platform_get_irq)
   ├─ DMA channels
   └─ Platform data

3. Initialize hardware
   ├─ Map memory (ioremap)
   ├─ Configure device
   ├─ Enable clocks
   └─ Reset device

4. Allocate data structures
   └─ Per-device private data

5. Register with framework
   ├─ Character device (cdev_add)
   ├─ Input device (input_register_device)
   ├─ Network device (register_netdev)
   └─ Or other subsystem

6. Setup interrupts
   └─ request_irq()

Return 0 on success, error code on failure
```

**Important notes:**

- ⚠️ **probe() ≠ module_init()** - probe() called per device, init() called once per module
- ⚠️ probe() can be called multiple times if multiple devices match
- ⚠️ probe() can be deferred if resources not ready

### The remove() Function

**Function prototype:**

```c
int (*remove)(struct platform_device *pdev);

/* Called when:
 * - Device is physically removed (rare for platform devices)
 * - Driver is unloaded (rmmod)
 * - System shutdown
 *
 * Returns: Usually 0
 */
```

**What remove() should do:**

```
remove() Function Responsibilities:

1. Unregister from framework
   └─ Reverse what was done in probe()

2. Free interrupts
   └─ free_irq()

3. Unmap memory
   └─ iounmap()

4. Disable hardware
   ├─ Turn off device
   └─ Disable clocks

5. Free allocated memory
   └─ kfree(), devm_* resources auto-freed

6. Clean up resources
   └─ Release any claimed resources
```

---

## 5.1.3 Platform Device Structure

### The struct platform_device

**Definition:**

```c
/* Defined in include/linux/platform_device.h */
struct platform_device {
    const char *name;              /* Device name for matching */
    int id;                        /* Device instance number */
    bool id_auto;                  /* Auto-assign ID */
    struct device dev;             /* Embedded device structure */
    u32 num_resources;            /* Number of resources */
    struct resource *resource;     /* Array of resources */

    const struct platform_device_id *id_entry;

    /* Managed resources */
    struct mfd_cell *mfd_cell;

    /* arch-specific additions */
    struct pdev_archdata archdata;
};
```

**Key fields:**

| Field | Description |
| --- | --- |
| `name` | Device name - must match driver name for binding |
| `id` | Instance ID (-1 for single instance, ≥0 for multiple) |
| `dev` | Generic device structure |
| `num_resources` | Count of resources in resource array |
| `resource` | Array of struct resource (mem, irq, dma, etc.) |

### Device-Driver Matching

**How matching works:**

```c
/* Matching algorithm (simplified) */

For each platform_device in device_list:
    For each platform_driver in driver_list:

        /* Method 1: Name-based matching */
        if (strcmp(device->name, driver->driver.name) == 0)
            → Match found! Call driver->probe(device)

        /* Method 2: Device Tree matching (covered later) */
        if (device->dev.of_node && driver->driver.of_match_table)
            → Match compatible strings

        /* Method 3: ID table matching */
        if (driver->id_table)
            → Match device->name with id_table entries
```

**Example matching:**

```c
/* Platform device */
struct platform_device uart0_device = {
    .name = "my-uart",
    .id = 0,
    /* ... */
};

/* Platform driver */
static struct platform_driver uart_driver = {
    .driver = {
        .name = "my-uart",  /* Must match device name! */
    },
    /* ... */
};

/* Result: Match! probe() will be called */
```

---

## 5.1.4 Driver Registration

### Method 1: Manual Registration (Old Style)

```c
static int __init my_driver_init(void)
{
    return platform_driver_register(&my_driver);
}

static void __exit my_driver_exit(void)
{
    platform_driver_unregister(&my_driver);
}

module_init(my_driver_init);
module_exit(my_driver_exit);
```

### Method 2: Using module_platform_driver (Recommended)

```c
/* Single line registration! */
module_platform_driver(my_driver);

/* This macro expands to:
 *
 * static int __init my_driver_init(void) {
 *     return platform_driver_register(&my_driver);
 * }
 *
 * static void __exit my_driver_exit(void) {
 *     platform_driver_unregister(&my_driver);
 * }
 *
 * module_init(my_driver_init);
 * module_exit(my_driver_exit);
 */
```

**Benefits of module_platform_driver:**

- ✅ Less boilerplate code
- ✅ Cleaner, more readable
- ✅ Standard pattern
- ✅ One macro replaces ~10 lines

### Registration Functions

**platform_driver_register():**

```c
int platform_driver_register(struct platform_driver *drv);

/* Registers driver with platform bus
 * Driver stays registered and probe() can be called
 * whenever a matching device appears
 *
 * Returns: 0 on success, negative error code on failure
 */
```

**platform_driver_probe():**

```c
int platform_driver_probe(struct platform_driver *drv,
                         int (*probe)(struct platform_device *));

/* Registers driver and immediately probes
 *
 * Differences from platform_driver_register():
 * - Probe happens immediately
 * - Driver is NOT kept in driver list
 * - Prevents deferred probing
 * - probe() can be in __init section (saves memory)
 *
 * Use when: Device is guaranteed to be present at boot
 */
```

**Example:**

```c
static int __init my_probe(struct platform_device *pdev)
{
    /* This function can be in __init section */
    /* It will be freed after boot completes */
    return 0;
}

static struct platform_driver my_driver = {
    .driver = {
        .name = "my-device",
    },
    /* No .probe here when using platform_driver_probe */
};

static int __init my_init(void)
{
    /* Probe immediately, driver not kept registered */
    return platform_driver_probe(&my_driver, my_probe);
}

module_init(my_init);
```

---

## 5.1.5 Complete Basic Platform Driver Example

### Simple Platform Driver

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>

/*
 * Probe function - called when device matches
 */
static int my_pdrv_probe(struct platform_device *pdev)
{
    pr_info("my_pdrv: Probe function called!\n");
    pr_info("my_pdrv: Device name: %s\n", pdev->name);
    pr_info("my_pdrv: Device ID: %d\n", pdev->id);

    /*
     * In real driver, you would:
     * 1. Get resources (memory, IRQ, etc.)
     * 2. Initialize hardware
     * 3. Allocate data structures
     * 4. Register with subsystem
     */

    return 0;  /* Success */
}

/*
 * Remove function - called when device removed or driver unloaded
 */
static int my_pdrv_remove(struct platform_device *pdev)
{
    pr_info("my_pdrv: Remove function called!\n");
    pr_info("my_pdrv: Device name: %s\n", pdev->name);

    /*
     * In real driver, you would:
     * 1. Unregister from subsystem
     * 2. Free IRQ
     * 3. Unmap memory
     * 4. Free allocated memory
     * 5. Disable hardware
     */

    return 0;
}

/*
 * Platform driver structure
 */
static struct platform_driver my_platform_driver = {
    .probe  = my_pdrv_probe,
    .remove = my_pdrv_remove,
    .driver = {
        .name  = "my-platform-device",
        .owner = THIS_MODULE,
    },
};

/*
 * Module registration using macro
 */
module_platform_driver(my_platform_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Expert");
MODULE_DESCRIPTION("Simple Platform Driver Example");
MODULE_VERSION("1.0");
```

**Testing this driver:**

Since we haven't created a platform device yet, loading this driver alone won't call probe(). We'll see how to create devices in Part 3.

```bash
# Load driver
sudo insmod simple_platform_driver.ko

# Check it registered
ls /sys/bus/platform/drivers/
# You'll see: my-platform-device

# Unload
sudo rmmod simple_platform_driver

# Check dmesg
dmesg | tail
# (No probe messages because no matching device)
```

---

## 5.1.6 Platform Driver with Device Creation

### Creating a Matching Platform Device

**Platform device code:**

```c
#include <linux/module.h>
#include <linux/platform_device.h>

/*
 * Platform device structure
 */
static struct platform_device my_platform_device = {
    .name = "my-platform-device",  /* Must match driver name */
    .id = -1,                       /* -1 = single instance */
};

/*
 * Module init - register device
 */
static int __init my_device_init(void)
{
    int ret;

    pr_info("my_device: Registering platform device\n");

    ret = platform_device_register(&my_platform_device);
    if (ret) {
        pr_err("my_device: Failed to register device\n");
        return ret;
    }

    pr_info("my_device: Platform device registered\n");
    return 0;
}

/*
 * Module exit - unregister device
 */
static void __exit my_device_exit(void)
{
    pr_info("my_device: Unregistering platform device\n");
    platform_device_unregister(&my_platform_device);
}

module_init(my_device_init);
module_exit(my_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Expert");
MODULE_DESCRIPTION("Simple Platform Device");
```

**Testing the complete system:**

```bash
# Load device module first
sudo insmod platform_device.ko
# dmesg shows: "my_device: Platform device registered"

# Load driver module
sudo insmod platform_driver.ko
# dmesg shows:
# "my_pdrv: Probe function called!"
# "my_pdrv: Device name: my-platform-device"
# "my_pdrv: Device ID: -1"

# Unload driver
sudo rmmod platform_driver
# dmesg shows: "my_pdrv: Remove function called!"

# Unload device
sudo rmmod platform_device
```

**What happened:**

```
Step-by-step flow:

1. platform_device.ko loaded
   └─ Device added to platform bus device list

2. platform_driver.ko loaded
   └─ Driver added to platform bus driver list
   └─ Bus core runs matching algorithm
   └─ Match found! (names match)
   └─ driver->probe(device) called
   └─ Probe returns 0 (success)

3. platform_driver.ko unloaded
   └─ driver->remove(device) called
   └─ Driver removed from bus

4. platform_device.ko unloaded
   └─ Device removed from bus
```

---

## 5.1.7 Multiple Devices, One Driver

### Scenario: Managing Multiple Similar Devices

**Common pattern:** One driver managing multiple instances of the same hardware.

**Platform devices:**

```c
/* platform_devices.c - Create 3 devices */
#include <linux/module.h>
#include <linux/platform_device.h>

static struct platform_device *my_devices[3];

static int __init my_devices_init(void)
{
    int i;
    char name[32];

    for (i = 0; i < 3; i++) {
        /* Allocate device */
        my_devices[i] = platform_device_alloc("my-uart", i);
        if (!my_devices[i]) {
            pr_err("Failed to allocate device %d\n", i);
            goto err_alloc;
        }

        /* Register device */
        if (platform_device_add(my_devices[i])) {
            pr_err("Failed to add device %d\n", i);
            platform_device_put(my_devices[i]);
            goto err_alloc;
        }

        pr_info("Registered device: my-uart.%d\n", i);
    }

    return 0;

err_alloc:
    while (--i >= 0) {
        platform_device_unregister(my_devices[i]);
    }
    return -ENOMEM;
}

static void __exit my_devices_exit(void)
{
    int i;

    for (i = 0; i < 3; i++) {
        platform_device_unregister(my_devices[i]);
        pr_info("Unregistered device: my-uart.%d\n", i);
    }
}

module_init(my_devices_init);
module_exit(my_devices_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Multiple platform devices");
```

**Platform driver:**

```c
/* platform_driver.c - One driver for all devices */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

/* Per-device private data */
struct uart_device {
    int id;
    /* Add your device-specific fields here */
};

static int my_uart_probe(struct platform_device *pdev)
{
    struct uart_device *uart;

    pr_info("my_uart: Probing device %s (id=%d)\n",
            pdev->name, pdev->id);

    /* Allocate per-device data */
    uart = kzalloc(sizeof(*uart), GFP_KERNEL);
    if (!uart)
        return -ENOMEM;

    uart->id = pdev->id;

    /* Store private data in device */
    platform_set_drvdata(pdev, uart);

    pr_info("my_uart: Device %d initialized\n", uart->id);

    return 0;
}

static int my_uart_remove(struct platform_device *pdev)
{
    struct uart_device *uart = platform_get_drvdata(pdev);

    pr_info("my_uart: Removing device %d\n", uart->id);

    /* Free private data */
    kfree(uart);

    return 0;
}

static struct platform_driver my_uart_driver = {
    .probe  = my_uart_probe,
    .remove = my_uart_remove,
    .driver = {
        .name = "my-uart",
        .owner = THIS_MODULE,
    },
};

module_platform_driver(my_uart_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("UART platform driver - supports multiple devices");
```

**Testing:**

```bash
# Load devices
sudo insmod platform_devices.ko
# dmesg:
# Registered device: my-uart.0
# Registered device: my-uart.1
# Registered device: my-uart.2

# Load driver
sudo insmod platform_driver.ko
# dmesg:
# my_uart: Probing device my-uart (id=0)
# my_uart: Device 0 initialized
# my_uart: Probing device my-uart (id=1)
# my_uart: Device 1 initialized
# my_uart: Probing device my-uart (id=2)
# my_uart: Device 2 initialized

# Check sysfs
ls /sys/bus/platform/devices/
# my-uart.0  my-uart.1  my-uart.2
```

**Key points:**

- ✅ probe() called **3 times** (once per device)
- ✅ Each call gets different `pdev` with different `id`
- ✅ Use `platform_set_drvdata()` to store per-device data
- ✅ Use `platform_get_drvdata()` to retrieve it

---

## 5.1.8 Platform Device ID Table

### Using ID Table for Multiple Device Types

**When you have different device variants:**

```c
/* Different UART versions */
#define UART_TYPE_V1  0
#define UART_TYPE_V2  1
#define UART_TYPE_V3  2

/* Device-specific data structures */
struct uart_v1_data {
    int max_baud;
    int fifo_size;
};

struct uart_v2_data {
    int max_baud;
    int fifo_size;
    bool has_dma;
};

/* Device type data */
static const struct uart_v1_data v1_data = {
    .max_baud = 115200,
    .fifo_size = 16,
};

static const struct uart_v2_data v2_data = {
    .max_baud = 921600,
    .fifo_size = 64,
    .has_dma = true,
};

/* ID table */
static const struct platform_device_id uart_id_table[] = {
    {
        .name = "uart-v1",
        .driver_data = (kernel_ulong_t)&v1_data,
    },
    {
        .name = "uart-v2",
        .driver_data = (kernel_ulong_t)&v2_data,
    },
    { /* Sentinel */ },
};
MODULE_DEVICE_TABLE(platform, uart_id_table);

/* Driver probe */
static int uart_probe(struct platform_device *pdev)
{
    const struct platform_device_id *id;

    /* Get matching ID entry */
    id = platform_get_device_id(pdev);

    if (!strcmp(id->name, "uart-v1")) {
        struct uart_v1_data *data = (void *)id->driver_data;
        pr_info("UART v1: max_baud=%d, fifo=%d\n",
                data->max_baud, data->fifo_size);
    } else if (!strcmp(id->name, "uart-v2")) {
        struct uart_v2_data *data = (void *)id->driver_data;
        pr_info("UART v2: max_baud=%d, fifo=%d, dma=%d\n",
                data->max_baud, data->fifo_size, data->has_dma);
    }

    return 0;
}

static struct platform_driver uart_driver = {
    .probe = uart_probe,
    .id_table = uart_id_table,  /* Attach ID table */
    .driver = {
        .name = "uart-generic",
    },
};
```

---

## Summary

This part covered the fundamentals of platform drivers:

**Key Concepts:**

- ✅ Platform drivers handle non-discoverable devices
- ✅ Platform bus is a pseudo-bus (software abstraction)
- ✅ `struct platform_driver` - driver structure
- ✅ `struct platform_device` - device structure
- ✅ probe() called when device-driver match occurs
- ✅ remove() called on device removal or driver unload
- ✅ Name-based matching is the simplest method
- ✅ module_platform_driver() macro simplifies registration
- ✅ One driver can handle multiple device instances
- ✅ ID tables support different device variants