# Part 1. Character Device Registration

Character devices are the most fundamental type of device driver in Linux. They transfer data as a stream of bytes (characters), similar to how serial ports work. This chapter covers everything you need to know to create professional character device drivers.

---

## 4.1 Character Device Basics

### Introduction to Character Devices

**What is a Character Device?**

A character device is a device that transfers data **byte by byte**, in sequential order, like a stream. Unlike block devices (which transfer data in blocks), character devices are accessed as a continuous stream of characters.

**Examples of character devices:**

- Serial ports (UART, RS-232)
- Keyboard and mouse
- Sound cards
- Terminals and consoles
- GPIO pins
- I2C/SPI devices exposed to userspace
- Many sensors and embedded peripherals

**Character vs Block devices:**

```
Character Device:              Block Device:
├─ Stream of bytes            ├─ Blocks of data
├─ Sequential access          ├─ Random access
├─ No caching                 ├─ Cached by kernel
├─ Simple I/O                 ├─ Complex I/O scheduler
└─ Examples: UART, GPIO       └─ Examples: Hard disk, USB storage

User Space Access:
/dev/ttyS0 (serial port)      /dev/sda (hard disk)
/dev/input/mouse0             /dev/mmcblk0 (SD card)
```

### The Big Picture: How Character Devices Work

```
                    Character Device Architecture
┌─────────────────────────────────────────────────────────────┐
│                      USER SPACE                             │
│                                                             │
│  Application                                                │
│      │                                                      │
│      ├─ open("/dev/mydevice", O_RDWR)                       │
│      ├─ read(fd, buffer, size)                              │
│      ├─ write(fd, buffer, size)                             │
│      └─ close(fd)                                           │
│                                                             │
└───────────────────────────┬─────────────────────────────────┘
                            │ System Calls
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                     KERNEL SPACE                            │
│                                                             │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ Virtual File System (VFS)                              │ │
│  │  - Manages /dev/mydevice                               │ │
│  │  - Routes calls to correct driver                      │ │
│  └────────────────────────┬───────────────────────────────┘ │
│                            │                                │
│                            ↓                                │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ Character Device Layer                                 │ │
│  │  ┌──────────────┐    ┌──────────────┐                  │ │
│  │  │ struct cdev  │───→│ file_ops     │                  │ │
│  │  │ Major: 240   │    │ .open        │                  │ │
│  │  │ Minor: 0     │    │ .read        │                  │ │
│  │  └──────────────┘    │ .write       │                  │ │
│  │                      │ .release     │                  │ │
│  │                      └──────────────┘                  │ │
│  └────────────────────────┬───────────────────────────────┘ │
│                           │                                 │
│                           ↓                                 │
│  ┌────────────────────────────────────────────────────────┐ │
│  │ Your Driver Code                                       │ │
│  │  - Device initialization                               │ │
│  │  - Hardware interaction                                │ │
│  │  - Data processing                                     │ │
│  └────────────────────────┬───────────────────────────────┘ │
│                           │                                 │
└───────────────────────────┼─────────────────────────────────┘
                            │
                            ↓
                    ┌────────────────┐
                    │    HARDWARE    │
                    │   Your Device  │
                    └────────────────┘
```

### Core Data Structure: struct cdev

**The cdev structure** represents a character device in the kernel:

```c
/* Defined in include/linux/cdev.h */
struct cdev {
    struct kobject kobj;              /* Kernel object for reference counting */
    struct module *owner;             /* Module that owns this cdev */
    const struct file_operations *ops; /* File operations */
    struct list_head list;            /* List of cdevs */
    dev_t dev;                        /* Device number (major + minor) */
    unsigned int count;               /* Number of devices */
};
```

**Key fields explained:**

| Field | Purpose |
| --- | --- |
| `kobj` | Kernel object for sysfs integration and reference counting |
| `owner` | Pointer to the module (usually `THIS_MODULE`) |
| `ops` | Pointer to file_operations structure (your driver methods) |
| `dev` | Device number (combination of major and minor) |
| `count` | Number of consecutive minor numbers |

---

## 4.1.2 Device Numbers: Major and Minor

### Understanding Device Numbers

**Device numbers identify devices in the kernel.** They consist of two parts:

```
Device Number (dev_t):
┌─────────────────────────────────┐
│ 32-bit unsigned integer         │
├───────────────┬─────────────────┤
│ Major (12bit) │ Minor (20 bit)  │
│ 0-4095        │ 0-1048575       │
└───────────────┴─────────────────┘

Example: /dev/ttyS0
├─ Major: 4  (identifies the driver)
└─ Minor: 64 (identifies the specific device)
```

**Roles:**

- **Major number:** Identifies the **driver** responsible for the device
- **Minor number:** Identifies the **specific device** instance

**Example: Multiple devices, one driver:**

```
Driver "my_uart_driver" (Major: 240)
├─ /dev/uart0  → Major:240, Minor:0
├─ /dev/uart1  → Major:240, Minor:1
├─ /dev/uart2  → Major:240, Minor:2
└─ /dev/uart3  → Major:240, Minor:3

Same driver handles all 4 devices!
Minor number distinguishes which UART.
```

### Viewing Device Numbers

```bash
# List devices with major:minor numbers
ls -l /dev/

crw-rw---- 1 root kmem      1,   1 Dec 13 08:57 mem
crw-rw---- 1 root disk     10, 237 Dec 13 08:57 loop-control
brw-rw---- 1 root disk      7,   0 Dec 13 08:57 loop0
crw-rw-rw- 1 root tty       5,   0 Dec 13 10:23 tty
crw--w---- 1 root tty       4,   0 Dec 13 08:57 tty0
crw--w---- 1 root tty       4,   1 Dec 13 08:57 tty1

# First column indicates file type:
# c = character device
# b = block device
# l = symbolic link
# d = directory

# Format: <major>, <minor>
```

### The dev_t Type

**dev_t** is a typedef for a 32-bit unsigned integer that holds both major and minor:

```c
/* Defined in include/linux/types.h */
typedef u32 dev_t;  /* 32-bit device number */

/* Bit layout (defined in include/linux/kdev_t.h) */
#define MINORBITS    20                          /* 20 bits for minor */
#define MINORMASK    ((1U << MINORBITS) - 1)     /* Mask: 0xFFFFF */

#define MAJOR(dev)   ((unsigned int) ((dev) >> MINORBITS))
#define MINOR(dev)   ((unsigned int) ((dev) & MINORMASK))
#define MKDEV(ma,mi) (((ma) << MINORBITS) | (mi))
```

**Helper macros:**

```c
/* Extract major from dev_t */
int my_major = MAJOR(device_number);

/* Extract minor from dev_t */
int my_minor = MINOR(device_number);

/* Create dev_t from major and minor */
dev_t my_dev = MKDEV(240, 5);  /* Major:240, Minor:5 */
```

**Example: Working with device numbers**

```c
#include <linux/kdev_t.h>

dev_t dev_num = MKDEV(240, 10);  /* Create device number */

printk("Device number: %d:%d\n",
       MAJOR(dev_num),    /* Prints: 240 */
       MINOR(dev_num));   /* Prints: 10 */

/* Converting to/from old format */
printk("Device number (hex): 0x%x\n", dev_num);  /* 0xF0000A */
```

---

## 4.1.3 Device Number Allocation

### Two Methods to Allocate Device Numbers

**Method 1: Static Allocation** (NOT recommended)

You manually choose a major number and register it:

```c
/* DON'T USE THIS unless you have a good reason! */
int register_chrdev_region(dev_t first, unsigned int count,
                           const char *name);

/* Parameters:
 * first  - Starting device number (MKDEV(major, first_minor))
 * count  - Number of consecutive device numbers
 * name   - Name of device/driver
 *
 * Returns: 0 on success, negative error code on failure
 */
```

**Problems with static allocation:**

- ❌ Chosen major might be already used
- ❌ Different machines might have conflicts
- ❌ Hard to maintain
- ❌ Not portable

**Method 2: Dynamic Allocation** (✅ RECOMMENDED)

Let the kernel choose an available major number:

```c
/* RECOMMENDED: Let kernel choose the major */
int alloc_chrdev_region(dev_t *dev, unsigned int firstminor,
                       unsigned int count, const char *name);

/* Parameters:
 * dev         - OUTPUT: Kernel assigns first device number here
 * firstminor  - First minor number you want
 * count       - Number of consecutive minors needed
 * name        - Device/driver name (shows in /proc/devices)
 *
 * Returns: 0 on success, negative error code on failure
 */
```

### Dynamic Allocation Examples

**Example 1: Single device**

```c
#include <linux/fs.h>
#include <linux/cdev.h>

static dev_t dev_num;

static int __init my_driver_init(void)
{
    int ret;

    /* Request 1 device number dynamically */
    ret = alloc_chrdev_region(&dev_num, 0, 1, "mydevice");
    if (ret < 0) {
        printk(KERN_ERR "Failed to allocate device number\n");
        return ret;
    }

    printk(KERN_INFO "Allocated device number: %d:%d\n",
           MAJOR(dev_num), MINOR(dev_num));

    return 0;
}

static void __exit my_driver_exit(void)
{
    /* Free the device number */
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "Device number released\n");
}

module_init(my_driver_init);
module_exit(my_driver_exit);
```

**Example 2: Multiple devices**

```c
#define NUM_DEVICES 4

static dev_t dev_num;  /* Stores first device number */

static int __init multi_device_init(void)
{
    int ret;

    /* Request 4 consecutive device numbers */
    ret = alloc_chrdev_region(&dev_num, 0, NUM_DEVICES, "uart_devices");
    if (ret < 0) {
        printk(KERN_ERR "Failed to allocate %d device numbers\n",
               NUM_DEVICES);
        return ret;
    }

    /* dev_num now contains the first device number */
    printk(KERN_INFO "Allocated device numbers:\n");
    printk(KERN_INFO "  Major: %d\n", MAJOR(dev_num));
    printk(KERN_INFO "  Minor range: %d-%d\n",
           MINOR(dev_num),
           MINOR(dev_num) + NUM_DEVICES - 1);

    /* Individual device numbers can be calculated */
    for (int i = 0; i < NUM_DEVICES; i++) {
        dev_t this_dev = MKDEV(MAJOR(dev_num), MINOR(dev_num) + i);
        printk(KERN_INFO "  Device %d: %d:%d\n",
               i, MAJOR(this_dev), MINOR(this_dev));
    }

    return 0;
}

static void __exit multi_device_exit(void)
{
    /* Free all device numbers */
    unregister_chrdev_region(dev_num, NUM_DEVICES);
}
```

**Output example:**

```
Allocated device numbers:
  Major: 240
  Minor range: 0-3
  Device 0: 240:0
  Device 1: 240:1
  Device 2: 240:2
  Device 3: 240:3
```

### Freeing Device Numbers

```c
/* Free allocated device numbers */
void unregister_chrdev_region(dev_t first, unsigned int count);

/* Always call this in module cleanup! */
```

**Important notes:**

- ✅ Always free device numbers in `module_exit()`
- ✅ Free the SAME number of devices you allocated
- ✅ Use the SAME starting `dev_t` value

---

## 4.1.4 Character Device Registration

### Registration Flow Overview

**To create a functional character device:**

```
Step-by-step Registration:

1. Allocate device number
   ├─ alloc_chrdev_region()
   └─ Get major:minor

2. Initialize cdev structure
   ├─ cdev_init()
   └─ Set file_operations

3. Add cdev to kernel
   ├─ cdev_add()
   └─ Device now active!

4. Create device class
   ├─ class_create()
   └─ Creates /sys/class/<name>

5. Create device node
   ├─ device_create()
   └─ udev creates /dev/<name>
```

### Step 1: Initialize cdev Structure

```c
/* Initialize a cdev structure */
void cdev_init(struct cdev *cdev, const struct file_operations *fops);

/* Parameters:
 * cdev - Pointer to cdev structure to initialize
 * fops - Pointer to file_operations (your driver methods)
 */
```

**Example:**

```c
#include <linux/cdev.h>
#include <linux/fs.h>

/* Your file operations (we'll implement these later) */
static struct file_operations my_fops = {
    .owner   = THIS_MODULE,
    .open    = my_open,
    .release = my_release,
    .read    = my_read,
    .write   = my_write,
};

static struct cdev my_cdev;

static int __init my_driver_init(void)
{
    /* Initialize cdev structure */
    cdev_init(&my_cdev, &my_fops);
    my_cdev.owner = THIS_MODULE;

    return 0;
}
```

### Step 2: Add cdev to Kernel

```c
/* Register cdev with the kernel */
int cdev_add(struct cdev *p, dev_t dev, unsigned int count);

/* Parameters:
 * p     - Pointer to initialized cdev
 * dev   - First device number
 * count - Number of consecutive minors
 *
 * Returns: 0 on success, negative error code on failure
 */
```

**After `cdev_add()`, your device is LIVE!** User space can now open it.

**Example:**

```c
static dev_t dev_num;
static struct cdev my_cdev;

static int __init my_driver_init(void)
{
    int ret;

    /* 1. Allocate device number */
    ret = alloc_chrdev_region(&dev_num, 0, 1, "mydevice");
    if (ret < 0)
        return ret;

    /* 2. Initialize cdev */
    cdev_init(&my_cdev, &my_fops);
    my_cdev.owner = THIS_MODULE;

    /* 3. Add cdev to kernel - DEVICE IS NOW ACTIVE */
    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    printk(KERN_INFO "Character device registered: %d:%d\n",
           MAJOR(dev_num), MINOR(dev_num));

    return 0;
}

static void __exit my_driver_exit(void)
{
    /* Remove cdev from kernel */
    cdev_del(&my_cdev);

    /* Free device number */
    unregister_chrdev_region(dev_num, 1);

    printk(KERN_INFO "Character device unregistered\n");
}
```

### Step 3: Delete cdev

```c
/* Unregister cdev from kernel */
void cdev_del(struct cdev *p);

/* Always call this BEFORE unregister_chrdev_region() */
```

---

## 4.1.5 Device Class and Automatic Device Node Creation

### The Problem: Manual Device Node Creation

**Without device class:**

```bash
# User must manually create device node
sudo mknod /dev/mydevice c 240 0
#                        │  │   └─ Minor number
#                        │  └───── Major number
#                        └──────── 'c' = character device

# Problems:
# - User must know major:minor
# - Manual work required
# - Error-prone
# - Not user-friendly
```

**With device class (automatic):**

```bash
# udev automatically creates /dev/mydevice
# No manual intervention needed!
```

### Device Class Creation

**Device classes** organize related devices and enable automatic device node creation via udev.

```c
/* Create a device class */
struct class *class_create(struct module *owner, const char *name);

/* Parameters:
 * owner - THIS_MODULE
 * name  - Class name (appears in /sys/class/<name>)
 *
 * Returns: Pointer to class structure, or ERR_PTR() on error
 */
```

**Example:**

```c
#include <linux/device.h>

static struct class *my_class;

static int __init my_driver_init(void)
{
    /* Create device class */
    my_class = class_create(THIS_MODULE, "mydevice_class");
    if (IS_ERR(my_class)) {
        printk(KERN_ERR "Failed to create class\n");
        return PTR_ERR(my_class);
    }

    printk(KERN_INFO "Class created: /sys/class/mydevice_class\n");
    return 0;
}

static void __exit my_driver_exit(void)
{
    /* Destroy class */
    class_destroy(my_class);
}
```

### Device Node Creation

```c
/* Create device node */
struct device *device_create(struct class *class,
                            struct device *parent,
                            dev_t devt,
                            void *drvdata,
                            const char *fmt, ...);

/* Parameters:
 * class  - Pointer to device class
 * parent - Parent device (usually NULL)
 * devt   - Device number (from alloc_chrdev_region)
 * drvdata- Private driver data (can be NULL)
 * fmt    - Device name format string (printf-style)
 *
 * Returns: Pointer to created device, or ERR_PTR() on error
 */
```

**What happens:**

1. Creates directory in `/sys/class/<class_name>/<device_name>/`
2. Populates `dev` file with `major:minor`
3. **udev** reads this and creates `/dev/<device_name>`

**Example:**

```c
static struct class *my_class;
static struct device *my_device;
static dev_t dev_num;

static int __init my_driver_init(void)
{
    int ret;

    /* Allocate device number */
    ret = alloc_chrdev_region(&dev_num, 0, 1, "mydevice");
    if (ret < 0)
        return ret;

    /* Create class */
    my_class = class_create(THIS_MODULE, "mydevice_class");
    if (IS_ERR(my_class)) {
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(my_class);
    }

    /* Create device - udev will create /dev/mydevice */
    my_device = device_create(my_class, NULL, dev_num, NULL, "mydevice");
    if (IS_ERR(my_device)) {
        class_destroy(my_class);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(my_device);
    }

    printk(KERN_INFO "Device created: /dev/mydevice\n");
    return 0;
}

static void __exit my_driver_exit(void)
{
    /* Destroy device */
    device_destroy(my_class, dev_num);

    /* Destroy class */
    class_destroy(my_class);

    /* Free device number */
    unregister_chrdev_region(dev_num, 1);
}
```

**Result:**

```bash
# After loading module:
ls -l /dev/mydevice
crw------- 1 root root 240, 0 Dec 13 10:30 /dev/mydevice

ls -l /sys/class/mydevice_class/mydevice/dev
cat /sys/class/mydevice_class/mydevice/dev
240:0
```

---

## 4.1.6 Complete Registration Example

### Basic Character Device Driver

```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DEVICE_NAME "simple_char"
#define CLASS_NAME  "simple_class"

/* Global variables */
static dev_t dev_num;
static struct cdev simple_cdev;
static struct class *simple_class;
static struct device *simple_device;

/* File operation function prototypes */
static int simple_open(struct inode *inode, struct file *file);
static int simple_release(struct inode *inode, struct file *file);
static ssize_t simple_read(struct file *file, char __user *buf,
                          size_t len, loff_t *offset);
static ssize_t simple_write(struct file *file, const char __user *buf,
                           size_t len, loff_t *offset);

/* File operations structure */
static struct file_operations simple_fops = {
    .owner   = THIS_MODULE,
    .open    = simple_open,
    .release = simple_release,
    .read    = simple_read,
    .write   = simple_write,
};

/* Open function */
static int simple_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "simple_char: Device opened\n");
    return 0;
}

/* Release function */
static int simple_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "simple_char: Device closed\n");
    return 0;
}

/* Read function */
static ssize_t simple_read(struct file *file, char __user *buf,
                          size_t len, loff_t *offset)
{
    printk(KERN_INFO "simple_char: Read called\n");
    return 0;  /* EOF */
}

/* Write function */
static ssize_t simple_write(struct file *file, const char __user *buf,
                           size_t len, loff_t *offset)
{
    printk(KERN_INFO "simple_char: Write called with %zu bytes\n", len);
    return len;
}

/* Module initialization */
static int __init simple_char_init(void)
{
    int ret;

    printk(KERN_INFO "simple_char: Initializing module\n");

    /* 1. Allocate device number dynamically */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "simple_char: Failed to allocate device number\n");
        return ret;
    }
    printk(KERN_INFO "simple_char: Allocated device number: %d:%d\n",
           MAJOR(dev_num), MINOR(dev_num));

    /* 2. Initialize cdev structure */
    cdev_init(&simple_cdev, &simple_fops);
    simple_cdev.owner = THIS_MODULE;

    /* 3. Add cdev to kernel */
    ret = cdev_add(&simple_cdev, dev_num, 1);
    if (ret < 0) {
        printk(KERN_ERR "simple_char: Failed to add cdev\n");
        goto fail_cdev_add;
    }
    printk(KERN_INFO "simple_char: Character device added\n");

    /* 4. Create device class */
    simple_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(simple_class)) {
        printk(KERN_ERR "simple_char: Failed to create class\n");
        ret = PTR_ERR(simple_class);
        goto fail_class_create;
    }
    printk(KERN_INFO "simple_char: Class created\n");

    /* 5. Create device node */
    simple_device = device_create(simple_class, NULL, dev_num,
                                 NULL, DEVICE_NAME);
    if (IS_ERR(simple_device)) {
        printk(KERN_ERR "simple_char: Failed to create device\n");
        ret = PTR_ERR(simple_device);
        goto fail_device_create;
    }
    printk(KERN_INFO "simple_char: Device created: /dev/%s\n", DEVICE_NAME);

    return 0;

/* Error handling with goto */
fail_device_create:
    class_destroy(simple_class);
fail_class_create:
    cdev_del(&simple_cdev);
fail_cdev_add:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

/* Module cleanup */
static void __exit simple_char_exit(void)
{
    printk(KERN_INFO "simple_char: Cleaning up module\n");

    /* Destroy device */
    device_destroy(simple_class, dev_num);
    printk(KERN_INFO "simple_char: Device destroyed\n");

    /* Destroy class */
    class_destroy(simple_class);
    printk(KERN_INFO "simple_char: Class destroyed\n");

    /* Remove cdev */
    cdev_del(&simple_cdev);
    printk(KERN_INFO "simple_char: Character device removed\n");

    /* Free device number */
    unregister_chrdev_region(dev_num, 1);
    printk(KERN_INFO "simple_char: Device number freed\n");
}

module_init(simple_char_init);
module_exit(simple_char_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Expert");
MODULE_DESCRIPTION("Simple character device driver");
MODULE_VERSION("1.0");
```

**Makefile:**

```makefile
obj-m := simple_char.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean

load:
	sudo insmod simple_char.ko

unload:
	sudo rmmod simple_char

test:
	sudo dmesg | tail -20
```

**Testing:**

```bash
# Build module
make

# Load module
sudo insmod simple_char.ko

# Check device created
ls -l /dev/simple_char
crw------- 1 root root 240, 0 Dec 13 10:45 /dev/simple_char

# Check kernel messages
dmesg | tail
# Output:
# simple_char: Initializing module
# simple_char: Allocated device number: 240:0
# simple_char: Character device added
# simple_char: Class created
# simple_char: Device created: /dev/simple_char

# Test device
sudo cat /dev/simple_char
# Kernel log: simple_char: Device opened
# Kernel log: simple_char: Read called
# Kernel log: simple_char: Device closed

# Unload module
sudo rmmod simple_char

# Verify cleanup
ls /dev/simple_char
# ls: cannot access '/dev/simple_char': No such file or directory
```

---

## 4.1.7 Multi-Device Driver Example

### Supporting Multiple Devices

**Scenario:** One driver managing multiple similar devices (e.g., 4 UART ports)

```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>

#define DEVICE_NAME "uart"
#define CLASS_NAME  "uart_class"
#define NUM_DEVICES 4

/* Per-device data structure */
struct uart_device {
    struct cdev cdev;
    int minor;
    char name[20];
};

/* Global variables */
static dev_t dev_num;                        /* First device number */
static struct class *uart_class;
static struct uart_device uart_devs[NUM_DEVICES];

/* File operations */
static int uart_open(struct inode *inode, struct file *file)
{
    struct uart_device *uart;

    /* Get device-specific data using container_of */
    uart = container_of(inode->i_cdev, struct uart_device, cdev);

    /* Store in file's private_data for later use */
    file->private_data = uart;

    printk(KERN_INFO "uart: Device %s opened\n", uart->name);
    return 0;
}

static int uart_release(struct inode *inode, struct file *file)
{
    struct uart_device *uart = file->private_data;
    printk(KERN_INFO "uart: Device %s closed\n", uart->name);
    return 0;
}

static ssize_t uart_read(struct file *file, char __user *buf,
                        size_t len, loff_t *offset)
{
    struct uart_device *uart = file->private_data;
    printk(KERN_INFO "uart: Read from %s\n", uart->name);
    return 0;
}

static ssize_t uart_write(struct file *file, const char __user *buf,
                         size_t len, loff_t *offset)
{
    struct uart_device *uart = file->private_data;
    printk(KERN_INFO "uart: Write to %s: %zu bytes\n", uart->name, len);
    return len;
}

static struct file_operations uart_fops = {
    .owner   = THIS_MODULE,
    .open    = uart_open,
    .release = uart_release,
    .read    = uart_read,
    .write   = uart_write,
};

/* Module initialization */
static int __init uart_driver_init(void)
{
    int ret, i;
    dev_t curr_dev;

    printk(KERN_INFO "uart: Initializing multi-device driver\n");

    /* 1. Allocate device numbers for all devices */
    ret = alloc_chrdev_region(&dev_num, 0, NUM_DEVICES, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "uart: Failed to allocate device numbers\n");
        return ret;
    }

    printk(KERN_INFO "uart: Allocated major: %d, minors: %d-%d\n",
           MAJOR(dev_num), MINOR(dev_num),
           MINOR(dev_num) + NUM_DEVICES - 1);

    /* 2. Create device class */
    uart_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(uart_class)) {
        printk(KERN_ERR "uart: Failed to create class\n");
        ret = PTR_ERR(uart_class);
        goto fail_class;
    }

    /* 3. Initialize and register each device */
    for (i = 0; i < NUM_DEVICES; i++) {
        /* Calculate device number for this device */
        curr_dev = MKDEV(MAJOR(dev_num), MINOR(dev_num) + i);

        /* Initialize device structure */
        uart_devs[i].minor = i;
        snprintf(uart_devs[i].name, sizeof(uart_devs[i].name),
                "%s%d", DEVICE_NAME, i);

        /* Initialize cdev */
        cdev_init(&uart_devs[i].cdev, &uart_fops);
        uart_devs[i].cdev.owner = THIS_MODULE;

        /* Add cdev to kernel */
        ret = cdev_add(&uart_devs[i].cdev, curr_dev, 1);
        if (ret < 0) {
            printk(KERN_ERR "uart: Failed to add cdev for device %d\n", i);
            goto fail_cdev_add;
        }

        /* Create device node */
        if (IS_ERR(device_create(uart_class, NULL, curr_dev, NULL,
                                uart_devs[i].name))) {
            printk(KERN_ERR "uart: Failed to create device %d\n", i);
            cdev_del(&uart_devs[i].cdev);
            goto fail_cdev_add;
        }

        printk(KERN_INFO "uart: Device created: /dev/%s (%d:%d)\n",
               uart_devs[i].name, MAJOR(curr_dev), MINOR(curr_dev));
    }

    return 0;

fail_cdev_add:
    /* Cleanup devices created so far */
    for (i--; i >= 0; i--) {
        curr_dev = MKDEV(MAJOR(dev_num), MINOR(dev_num) + i);
        device_destroy(uart_class, curr_dev);
        cdev_del(&uart_devs[i].cdev);
    }
    class_destroy(uart_class);
fail_class:
    unregister_chrdev_region(dev_num, NUM_DEVICES);
    return ret;
}

/* Module cleanup */
static void __exit uart_driver_exit(void)
{
    int i;
    dev_t curr_dev;

    printk(KERN_INFO "uart: Cleaning up driver\n");

    /* Destroy all devices */
    for (i = 0; i < NUM_DEVICES; i++) {
        curr_dev = MKDEV(MAJOR(dev_num), MINOR(dev_num) + i);
        device_destroy(uart_class, curr_dev);
        cdev_del(&uart_devs[i].cdev);
        printk(KERN_INFO "uart: Device %s removed\n", uart_devs[i].name);
    }

    /* Destroy class */
    class_destroy(uart_class);

    /* Free device numbers */
    unregister_chrdev_region(dev_num, NUM_DEVICES);

    printk(KERN_INFO "uart: Driver cleanup complete\n");
}

module_init(uart_driver_init);
module_exit(uart_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Expert");
MODULE_DESCRIPTION("Multi-device UART driver example");
```

**After loading:**

```bash
ls -l /dev/uart*
crw------- 1 root root 240, 0 Dec 13 11:00 /dev/uart0
crw------- 1 root root 240, 1 Dec 13 11:00 /dev/uart1
crw------- 1 root root 240, 2 Dec 13 11:00 /dev/uart2
crw------- 1 root root 240, 3 Dec 13 11:00 /dev/uart3
```

---

## 4.1.8 Error Handling Best Practices

### Using goto for Cleanup

**Why use goto?** Proper cleanup when initialization fails.

**Pattern:**

```c
static int __init my_driver_init(void)
{
    int ret;

    /* Step 1 */
    ret = allocate_resource1();
    if (ret < 0)
        return ret;

    /* Step 2 */
    ret = allocate_resource2();
    if (ret < 0)
        goto fail_resource2;

    /* Step 3 */
    ret = allocate_resource3();
    if (ret < 0)
        goto fail_resource3;

    return 0;

/* Cleanup in reverse order */
fail_resource3:
    free_resource2();
fail_resource2:
    free_resource1();
    return ret;
}
```

### Checking Pointer Errors

```c
/* Check if pointer is error */
bool IS_ERR(const void *ptr);

/* Extract error code from error pointer */
long PTR_ERR(const void *ptr);

/* Create error pointer from error code */
void *ERR_PTR(long error);
```

**Example:**

```c
struct class *my_class;

my_class = class_create(THIS_MODULE, "myclass");
if (IS_ERR(my_class)) {
    printk(KERN_ERR "Failed to create class: %ld\n",
           PTR_ERR(my_class));
    return PTR_ERR(my_class);
}
```

---

## Summary

This chapter covered character device registration:

**Key Concepts:**

- ✅ Character device basics and `struct cdev`
- ✅ Major/minor number system
- ✅ Dynamic device number allocation
- ✅ cdev initialization and registration
- ✅ Device class for automatic `/dev` node creation
- ✅ Multi-device driver support
- ✅ Error handling patterns