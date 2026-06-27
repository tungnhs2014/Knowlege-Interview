# Part 3. Device Provisioning & Integration

This final part covers how to provision platform devices - from legacy board files to modern Device Tree, plus complete real-world driver examples.

---

## 5.3 Device Provisioning Methods

### Overview: Three Ways to Provision Devices

```
Platform Device Provisioning Methods:

┌────────────────────────────────────────────────────────┐
│ Method 1: Legacy Board Files (Deprecated)              │
│ ─────────────────────────────────────────────────      │
│ • Hardcoded in arch/<arch>/mach-<soc>/board-xxx.c      │
│ • Compiled into kernel                                 │
│ • Not flexible                                         │
│ • Still used in very old kernels                       │
└────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────┐
│ Method 2: Runtime Device Creation                      │
│ ─────────────────────────────────────────────────      │
│ • Created in module code                               │
│ • Flexible for testing/development                     │
│ • Used for pseudo-devices                              │
│ • Examples: We used this in Part 1                     │
└────────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────────┐
│ Method 3: Device Tree (Modern, Recommended)            │
│ ─────────────────────────────────────────────────      │
│ • Separate .dts file, compiled to .dtb                 │
│ • Flexible, board-independent drivers                  │
│ • Industry standard                                    │
│ • Detailed in Chapter 6                                │
└────────────────────────────────────────────────────────┘
```

---

## 5.3.1 Legacy Board Files (For Understanding)

### Board File Structure

**Location:** `arch/<arch>/mach-<soc>/board-<name>.c`

**Example:** ARM board file

```c
/* arch/arm/mach-imx/board-mx6.c (simplified example) */
#include <linux/platform_device.h>
#include <linux/ioport.h>

/* UART resources */
static struct resource uart1_resources[] = {
    {
        .start = 0x02020000,
        .end   = 0x02023FFF,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = 26,
        .end   = 26,
        .flags = IORESOURCE_IRQ,
    },
};

/* UART platform device */
static struct platform_device uart1_device = {
    .name = "imx-uart",
    .id = 0,
    .num_resources = ARRAY_SIZE(uart1_resources),
    .resource = uart1_resources,
};

/* GPIO resources */
static struct resource gpio1_resources[] = {
    {
        .start = 0x0209C000,
        .end   = 0x0209FFFF,
        .flags = IORESOURCE_MEM,
    },
};

/* GPIO platform device */
static struct platform_device gpio1_device = {
    .name = "imx-gpio",
    .id = 0,
    .num_resources = ARRAY_SIZE(gpio1_resources),
    .resource = gpio1_resources,
};

/* Board init function */
static void __init board_init(void)
{
    /* Register platform devices */
    platform_device_register(&uart1_device);
    platform_device_register(&gpio1_device);

    /* More device registrations... */
}
```

### Problems with Board Files

**Why deprecated?**

❌ **Not scalable:**

- Every board needs separate file
- Lots of duplicate code
- Hard to maintain

❌ **Not flexible:**

- Must recompile kernel for changes
- Can't support multiple boards with one kernel

❌ **Driver pollution:**

- Drivers contain board-specific code
- Not portable across platforms

❌ **Kernel bloat:**

- All board files compiled into kernel
- Wastes space for unused boards

**Result:** Linux moved to Device Tree!

---

## 5.3.2 Device Tree Integration

### Device Tree Basics

**Device Tree** separates hardware description from driver code.

```
Traditional Approach:          Device Tree Approach:
┌──────────────────┐          ┌──────────────────┐
│   Kernel Image   │          │   Kernel Image   │
│                  │          │  (board-agnostic)│
│  ┌────────────┐  │          └──────────────────┘
│  │ Driver     │  │                   +
│  └────────────┘  │          ┌──────────────────┐
│  ┌────────────┐  │          │  Device Tree     │
│  │ Board File │  │          │   (.dtb blob)    │
│  │ (HW desc)  │  │          │                  │
│  └────────────┘  │          │  ┌────────────┐  │
└──────────────────┘          │  │ HW desc    │  │
                              │  └────────────┘  │
One kernel per board          └──────────────────┘

                              One kernel, many .dtb
```

### Platform Driver with Device Tree Support

**Adding Device Tree matching:**

```c
#include <linux/of.h>
#include <linux/of_device.h>

/* Device Tree match table */
static const struct of_device_id my_dt_ids[] = {
    { .compatible = "vendor,my-device", },
    { .compatible = "vendor,my-device-v2", },
    { /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, my_dt_ids);

/* Platform driver */
static struct platform_driver my_driver = {
    .probe = my_probe,
    .remove = my_remove,
    .driver = {
        .name = "my-driver",
        .of_match_table = my_dt_ids,  /* Add DT matching */
    },
};
```

### Device Tree Node Example

**In .dts file:**

```
/* my-board.dts */
/ {
    /* ... */

    my_device: mydev@80000000 {
        compatible = "vendor,my-device";
        reg = <0x80000000 0x1000>;   /* Memory region */
        interrupts = <0 56 4>;        /* IRQ */
        clocks = <&clk_peripheral>;
        clock-names = "peripheral";

        /* Custom properties */
        vendor,mode = <2>;
        vendor,speed = <100000>;
    };
};
```

### Extracting Device Tree Data

**The driver extracts resources automatically:**

```c
static int my_probe(struct platform_device *pdev)
{
    struct device_node *np = pdev->dev.of_node;
    struct resource *res;
    void __iomem *base;
    int irq;
    u32 mode, speed;

    /* Check if from Device Tree */
    if (!np) {
        dev_err(&pdev->dev, "No device tree node\n");
        return -EINVAL;
    }

    /* Get memory resource - extracted from "reg" property */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(base))
        return PTR_ERR(base);

    /* Get IRQ - extracted from "interrupts" property */
    irq = platform_get_irq(pdev, 0);
    if (irq < 0)
        return irq;

    /* Read custom properties */
    if (of_property_read_u32(np, "vendor,mode", &mode)) {
        dev_err(&pdev->dev, "Missing vendor,mode property\n");
        return -EINVAL;
    }

    if (of_property_read_u32(np, "vendor,speed", &speed)) {
        speed = 50000;  /* Default value */
    }

    dev_info(&pdev->dev, "Mode: %u, Speed: %u\n", mode, speed);

    /* Continue probe... */
    return 0;
}
```

### Mixed Matching (DT + ID Table)

**Support both Device Tree and legacy:**

```c
/* ID table for legacy */
static const struct platform_device_id my_id_table[] = {
    { .name = "my-device-v1", .driver_data = 1 },
    { .name = "my-device-v2", .driver_data = 2 },
    { /* Sentinel */ }
};
MODULE_DEVICE_TABLE(platform, my_id_table);

/* Device Tree table */
static const struct of_device_id my_dt_ids[] = {
    { .compatible = "vendor,my-device-v1", .data = (void *)1 },
    { .compatible = "vendor,my-device-v2", .data = (void *)2 },
    { /* Sentinel */ }
};
MODULE_DEVICE_TABLE(of, my_dt_ids);

/* Probe function */
static int my_probe(struct platform_device *pdev)
{
    const struct of_device_id *of_id;
    const struct platform_device_id *pdev_id;
    unsigned long driver_data;

    /* Check Device Tree first */
    of_id = of_match_device(my_dt_ids, &pdev->dev);
    if (of_id) {
        driver_data = (unsigned long)of_id->data;
        dev_info(&pdev->dev, "Matched via Device Tree\n");
    } else {
        /* Fall back to platform device ID */
        pdev_id = platform_get_device_id(pdev);
        if (pdev_id) {
            driver_data = pdev_id->driver_data;
            dev_info(&pdev->dev, "Matched via ID table\n");
        } else {
            dev_err(&pdev->dev, "No match found\n");
            return -ENODEV;
        }
    }

    dev_info(&pdev->dev, "Driver data: %lu\n", driver_data);

    /* Continue probe... */
    return 0;
}

/* Platform driver */
static struct platform_driver my_driver = {
    .probe = my_probe,
    .remove = my_remove,
    .driver = {
        .name = "my-driver",
        .of_match_table = of_match_ptr(my_dt_ids),
    },
    .id_table = my_id_table,
};
```

---

## 5.3.3 Runtime Device Creation

### Creating Devices Programmatically

**Use case:** Testing, pseudo-devices, or dynamic hardware

**Method 1: Simple device registration**

```c
static struct platform_device *my_pdev;

static int __init my_device_init(void)
{
    int ret;

    /* Allocate device */
    my_pdev = platform_device_alloc("my-device", -1);
    if (!my_pdev)
        return -ENOMEM;

    /* Add device to platform bus */
    ret = platform_device_add(my_pdev);
    if (ret) {
        platform_device_put(my_pdev);
        return ret;
    }

    pr_info("Platform device created\n");
    return 0;
}

static void __exit my_device_exit(void)
{
    platform_device_unregister(my_pdev);
}
```

**Method 2: Device with resources**

```c
static struct resource my_resources[] = {
    {
        .start = 0x80000000,
        .end   = 0x80000FFF,
        .flags = IORESOURCE_MEM,
    },
    {
        .start = 56,
        .end   = 56,
        .flags = IORESOURCE_IRQ,
    },
};

static struct platform_device *my_pdev;

static int __init my_device_init(void)
{
    int ret;

    /* Allocate device */
    my_pdev = platform_device_alloc("my-device", -1);
    if (!my_pdev)
        return -ENOMEM;

    /* Add resources */
    ret = platform_device_add_resources(my_pdev, my_resources,
                                       ARRAY_SIZE(my_resources));
    if (ret) {
        platform_device_put(my_pdev);
        return ret;
    }

    /* Add to platform bus */
    ret = platform_device_add(my_pdev);
    if (ret) {
        platform_device_put(my_pdev);
        return ret;
    }

    return 0;
}
```

---

## 5.3.4 Complete Real-World Example

### Pseudo Character Device as Platform Driver

**This example combines:**

- ✅ Platform driver
- ✅ Character device
- ✅ Multiple devices
- ✅ Per-device data
- ✅ File operations

**Device setup module (pcdev_platform_devices.c):**

```c
#include <linux/module.h>
#include <linux/platform_device.h>

#define NUM_DEVICES 4

struct pcdev_platform_data {
    int size;
    int serial_number;
    const char *model;
};

/* Platform data for each device */
struct pcdev_platform_data pcdev_pdata[] = {
    { .size = 512,  .serial_number = 1001, .model = "PCDEV-A1" },
    { .size = 1024, .serial_number = 1002, .model = "PCDEV-B1" },
    { .size = 128,  .serial_number = 1003, .model = "PCDEV-C1" },
    { .size = 256,  .serial_number = 1004, .model = "PCDEV-D1" },
};

/* Release function */
static void pcdev_release(struct device *dev)
{
    pr_info("Device released\n");
}

/* Platform devices */
struct platform_device platform_pcdev_1 = {
    .name = "pseudo-char-device",
    .id = 0,
    .dev = {
        .platform_data = &pcdev_pdata[0],
        .release = pcdev_release,
    },
};

struct platform_device platform_pcdev_2 = {
    .name = "pseudo-char-device",
    .id = 1,
    .dev = {
        .platform_data = &pcdev_pdata[1],
        .release = pcdev_release,
    },
};

struct platform_device platform_pcdev_3 = {
    .name = "pseudo-char-device",
    .id = 2,
    .dev = {
        .platform_data = &pcdev_pdata[2],
        .release = pcdev_release,
    },
};

struct platform_device platform_pcdev_4 = {
    .name = "pseudo-char-device",
    .id = 3,
    .dev = {
        .platform_data = &pcdev_pdata[3],
        .release = pcdev_release,
    },
};

static struct platform_device *platform_pcdevs[] = {
    &platform_pcdev_1,
    &platform_pcdev_2,
    &platform_pcdev_3,
    &platform_pcdev_4,
};

static int __init pcdev_platform_init(void)
{
    /* Register all devices */
    platform_add_devices(platform_pcdevs, ARRAY_SIZE(platform_pcdevs));

    pr_info("Platform devices loaded\n");
    return 0;
}

static void __exit pcdev_platform_exit(void)
{
    platform_device_unregister(&platform_pcdev_1);
    platform_device_unregister(&platform_pcdev_2);
    platform_device_unregister(&platform_pcdev_3);
    platform_device_unregister(&platform_pcdev_4);

    pr_info("Platform devices unloaded\n");
}

module_init(pcdev_platform_init);
module_exit(pcdev_platform_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Platform device setup for pseudo char devices");
```

**Platform driver module (pcdev_platform_driver.c):**

```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define MAX_DEVICES 10

/* Platform data structure (must match device) */
struct pcdev_platform_data {
    int size;
    int serial_number;
    const char *model;
};

/* Device private data */
struct pcdev_private_data {
    struct pcdev_platform_data pdata;
    char *buffer;
    struct cdev cdev;
    dev_t dev_num;
};

/* Driver private data */
struct pcdev_driver_private_data {
    int total_devices;
    dev_t device_number_base;
    struct class *class_pcdev;
    struct device *device_pcdev;
};

struct pcdev_driver_private_data pcdev_drv_data;

/* File operations */
loff_t pcdev_lseek(struct file *filp, loff_t offset, int whence)
{
    struct pcdev_private_data *pcdev_data = filp->private_data;
    int max_size = pcdev_data->pdata.size;
    loff_t temp;

    switch (whence) {
    case SEEK_SET:
        if (offset > max_size || offset < 0)
            return -EINVAL;
        filp->f_pos = offset;
        break;
    case SEEK_CUR:
        temp = filp->f_pos + offset;
        if (temp > max_size || temp < 0)
            return -EINVAL;
        filp->f_pos = temp;
        break;
    case SEEK_END:
        temp = max_size + offset;
        if (temp > max_size || temp < 0)
            return -EINVAL;
        filp->f_pos = temp;
        break;
    default:
        return -EINVAL;
    }

    return filp->f_pos;
}

ssize_t pcdev_read(struct file *filp, char __user *buff,
                   size_t count, loff_t *f_pos)
{
    struct pcdev_private_data *pcdev_data = filp->private_data;
    int max_size = pcdev_data->pdata.size;

    /* Adjust count */
    if ((*f_pos + count) > max_size)
        count = max_size - *f_pos;

    /* Copy to user */
    if (copy_to_user(buff, pcdev_data->buffer + *f_pos, count))
        return -EFAULT;

    /* Update position */
    *f_pos += count;

    pr_info("Read %zu bytes, f_pos=%lld\n", count, *f_pos);

    return count;
}

ssize_t pcdev_write(struct file *filp, const char __user *buff,
                    size_t count, loff_t *f_pos)
{
    struct pcdev_private_data *pcdev_data = filp->private_data;
    int max_size = pcdev_data->pdata.size;

    /* Adjust count */
    if ((*f_pos + count) > max_size)
        count = max_size - *f_pos;

    if (!count)
        return -ENOMEM;

    /* Copy from user */
    if (copy_from_user(pcdev_data->buffer + *f_pos, buff, count))
        return -EFAULT;

    /* Update position */
    *f_pos += count;

    pr_info("Wrote %zu bytes, f_pos=%lld\n", count, *f_pos);

    return count;
}

int pcdev_open(struct inode *inode, struct file *filp)
{
    struct pcdev_private_data *pcdev_data;

    /* Get device private data */
    pcdev_data = container_of(inode->i_cdev,
                             struct pcdev_private_data, cdev);

    /* Store in file's private_data */
    filp->private_data = pcdev_data;

    pr_info("Device opened: %s\n", pcdev_data->pdata.model);

    return 0;
}

int pcdev_release(struct inode *inode, struct file *filp)
{
    pr_info("Device closed\n");
    return 0;
}

/* File operations structure */
struct file_operations pcdev_fops = {
    .open = pcdev_open,
    .release = pcdev_release,
    .read = pcdev_read,
    .write = pcdev_write,
    .llseek = pcdev_lseek,
    .owner = THIS_MODULE,
};

/* Platform driver probe */
static int pcdev_platform_driver_probe(struct platform_device *pdev)
{
    struct pcdev_private_data *dev_data;
    struct pcdev_platform_data *pdata;
    int ret;

    pr_info("Device detected! %s\n", pdev->name);

    /* Get platform data */
    pdata = (struct pcdev_platform_data *)dev_get_platdata(&pdev->dev);
    if (!pdata) {
        pr_err("No platform data\n");
        return -EINVAL;
    }

    /* Allocate device private data */
    dev_data = devm_kzalloc(&pdev->dev, sizeof(*dev_data), GFP_KERNEL);
    if (!dev_data)
        return -ENOMEM;

    /* Save platform data */
    dev_data->pdata.size = pdata->size;
    dev_data->pdata.serial_number = pdata->serial_number;
    dev_data->pdata.model = pdata->model;

    pr_info("Device size: %d\n", dev_data->pdata.size);
    pr_info("Serial number: %d\n", dev_data->pdata.serial_number);
    pr_info("Model: %s\n", dev_data->pdata.model);

    /* Allocate device buffer */
    dev_data->buffer = devm_kzalloc(&pdev->dev, dev_data->pdata.size,
                                   GFP_KERNEL);
    if (!dev_data->buffer)
        return -ENOMEM;

    /* Get device number */
    dev_data->dev_num = pcdev_drv_data.device_number_base +
                        pcdev_drv_data.total_devices;

    /* Initialize cdev */
    cdev_init(&dev_data->cdev, &pcdev_fops);
    dev_data->cdev.owner = THIS_MODULE;

    /* Add cdev */
    ret = cdev_add(&dev_data->cdev, dev_data->dev_num, 1);
    if (ret < 0) {
        pr_err("cdev_add failed\n");
        return ret;
    }

    /* Create device */
    pcdev_drv_data.device_pcdev = device_create(
        pcdev_drv_data.class_pcdev,
        NULL,
        dev_data->dev_num,
        NULL,
        "pcdev-%d", pcdev_drv_data.total_devices);

    if (IS_ERR(pcdev_drv_data.device_pcdev)) {
        pr_err("device_create failed\n");
        ret = PTR_ERR(pcdev_drv_data.device_pcdev);
        cdev_del(&dev_data->cdev);
        return ret;
    }

    /* Save device data */
    platform_set_drvdata(pdev, dev_data);

    pcdev_drv_data.total_devices++;

    pr_info("Probe successful\n");
    return 0;
}

/* Platform driver remove */
static int pcdev_platform_driver_remove(struct platform_device *pdev)
{
    struct pcdev_private_data *dev_data = platform_get_drvdata(pdev);

    /* Remove device */
    device_destroy(pcdev_drv_data.class_pcdev, dev_data->dev_num);

    /* Delete cdev */
    cdev_del(&dev_data->cdev);

    pcdev_drv_data.total_devices--;

    pr_info("Device removed\n");
    return 0;
}

/* Platform driver structure */
static struct platform_driver pcdev_platform_driver = {
    .probe = pcdev_platform_driver_probe,
    .remove = pcdev_platform_driver_remove,
    .driver = {
        .name = "pseudo-char-device",
    },
};

/* Module init */
static int __init pcdev_platform_driver_init(void)
{
    int ret;

    /* Allocate device numbers */
    ret = alloc_chrdev_region(&pcdev_drv_data.device_number_base,
                             0, MAX_DEVICES, "pcdevs");
    if (ret < 0) {
        pr_err("alloc_chrdev_region failed\n");
        return ret;
    }

    /* Create class */
    pcdev_drv_data.class_pcdev = class_create(THIS_MODULE, "pcdev_class");
    if (IS_ERR(pcdev_drv_data.class_pcdev)) {
        pr_err("class_create failed\n");
        ret = PTR_ERR(pcdev_drv_data.class_pcdev);
        unregister_chrdev_region(pcdev_drv_data.device_number_base,
                                MAX_DEVICES);
        return ret;
    }

    /* Register platform driver */
    ret = platform_driver_register(&pcdev_platform_driver);
    if (ret) {
        pr_err("platform_driver_register failed\n");
        class_destroy(pcdev_drv_data.class_pcdev);
        unregister_chrdev_region(pcdev_drv_data.device_number_base,
                                MAX_DEVICES);
        return ret;
    }

    pr_info("Platform driver loaded\n");
    return 0;
}

/* Module exit */
static void __exit pcdev_platform_driver_exit(void)
{
    /* Unregister driver */
    platform_driver_unregister(&pcdev_platform_driver);

    /* Destroy class */
    class_destroy(pcdev_drv_data.class_pcdev);

    /* Release device numbers */
    unregister_chrdev_region(pcdev_drv_data.device_number_base,
                            MAX_DEVICES);

    pr_info("Platform driver unloaded\n");
}

module_init(pcdev_platform_driver_init);
module_exit(pcdev_platform_driver_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Pseudo character platform driver");
```

**Testing the complete system:**

```bash
# 1. Load platform devices
sudo insmod pcdev_platform_devices.ko
# dmesg: Platform devices loaded

# 2. Load platform driver
sudo insmod pcdev_platform_driver.ko
# dmesg:
# Platform driver loaded
# Device detected! pseudo-char-device
# Device size: 512
# Serial number: 1001
# Model: PCDEV-A1
# Probe successful
# (Repeated for all 4 devices)

# 3. Check created devices
ls -l /dev/pcdev-*
# crw------- 1 root root 237, 0 ... /dev/pcdev-0
# crw------- 1 root root 237, 1 ... /dev/pcdev-1
# crw------- 1 root root 237, 2 ... /dev/pcdev-2
# crw------- 1 root root 237, 3 ... /dev/pcdev-3

# 4. Test device
echo "Hello platform driver" | sudo tee /dev/pcdev-0
sudo cat /dev/pcdev-0
# Hello platform driver

# 5. Unload
sudo rmmod pcdev_platform_driver
sudo rmmod pcdev_platform_devices
```

---

## 5.3.5 Best Practices

### DO's ✅

1. *Use devm_ functions*
    
    ```c
    base = devm_ioremap_resource(&pdev->dev, res);
    ```
    
2. **Proper error handling with goto**
    
    ```c
    if (error)
        goto err_cleanup;
    ```
    
3. **Use platform_set/get_drvdata()**
    
    ```c
    platform_set_drvdata(pdev, priv);
    ```
    
4. **Support Device Tree**
    
    ```c
    .of_match_table = of_match_ptr(my_dt_ids),
    ```
    
5. **Provide release function for devices**
    
    ```c
    .release = my_release,
    ```
    

### DON'Ts ❌

1. **Don't forget to free resources**
    - Or use devm_* to auto-free
2. **Don't hardcode addresses in driver**
    - Get from resources or DT
3. **Don't mix up probe() and module_init()**
    - They serve different purposes
4. **Don't forget MODULE_DEVICE_TABLE**
    - Required for auto-loading
5. **Don't access hardware before probe()**
    - Device might not exist yet

---

## Summary

This chapter completed platform device drivers:

**Part 1: Platform Bus Basics**

- ✅ Platform bus concept
- ✅ Platform driver structure
- ✅ Platform device structure
- ✅ Device-driver matching
- ✅ module_platform_driver() macro

**Part 2: Probe/Remove & Resources**

- ✅ Probe function workflow
- ✅ Resource management
- ✅ platform_get_resource() APIs
- ✅ Memory mapping (ioremap)
- ✅ Managed resources (devm_*)
- ✅ Platform data handling
- ✅ Remove function cleanup

**Part 3: Device Provisioning**

- ✅ Legacy board files (deprecated)
- ✅ Device Tree integration
- ✅ Runtime device creation
- ✅ Complete real-world example
- ✅ Best practices

**You now know:**

- How to write platform drivers
- How to manage hardware resources
- How to integrate with Device Tree
- How to create complete driver systems