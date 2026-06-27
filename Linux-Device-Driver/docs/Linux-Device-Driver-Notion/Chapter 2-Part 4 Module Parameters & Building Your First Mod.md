# Part 4. Module Parameters & Building Your First Module

This final section covers module parameters for runtime configuration and the complete module building process, including Makefiles, in-tree vs out-of-tree compilation, and cross-compilation.

---

## 2.9 Module Parameters

### Why Module Parameters?

**Problem:** Hard-coded values in driver require recompilation to change.

**Solution:** Module parameters allow runtime configuration without recompiling.

**Benefits:**

```
Hard-coded Value          Module Parameter
├── Change = Recompile    ├── Change = Reload with new value
├── Different configs     ├── Single module, multiple configs
│   need different builds │   via parameters
├── Testing difficult     ├── Easy testing with different values
└── User inflexible       └── User has control
```

**Use cases:**

- Debug levels (0=off, 1=basic, 2=verbose)
- Hardware addresses
- IRQ numbers
- Buffer sizes
- Timeout values
- Feature enable/disable flags

### Declaring Module Parameters

**Include required header:**

```c
#include <linux/moduleparam.h>
```

**Basic syntax:**

```c
module_param(name, type, perm);
```

**Parameters:**

- `name`: Variable name (must be declared before module_param)
- `type`: Data type of the parameter
- `perm`: File permissions in sysfs

### Supported Parameter Types

```c
/* Integer types */
bool     /* Boolean (0/1, true/false) */
byte     /* unsigned char (0-255) */
short    /* short integer */
ushort   /* unsigned short */
int      /* integer */
uint     /* unsigned integer */
long     /* long integer */
ulong    /* unsigned long */

/* String type */
charp    /* char pointer (string) */

/* Special types */
invbool  /* Inverted boolean (1=false, 0=true) */
```

**Complete example:**

```c
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>

/* Declare parameter variables */
static int debug_level = 0;        /* Default: no debug */
static char *device_name = "mydev"; /* Default device name */
static int irq_number = 5;         /* Default IRQ */
static bool enable_feature = false; /* Default: feature off */
static unsigned int timeout_ms = 1000; /* Default: 1 second */

/* Register parameters with kernel */
module_param(debug_level, int, S_IRUGO | S_IWUSR);
module_param(device_name, charp, S_IRUGO);
module_param(irq_number, int, S_IRUGO | S_IWUSR);
module_param(enable_feature, bool, S_IRUGO | S_IWUSR);
module_param(timeout_ms, uint, S_IRUGO);

/* Describe each parameter */
MODULE_PARM_DESC(debug_level, "Debug output level (0=off, 1=errors, 2=verbose)");
MODULE_PARM_DESC(device_name, "Name of the device");
MODULE_PARM_DESC(irq_number, "IRQ line number");
MODULE_PARM_DESC(enable_feature, "Enable special feature (default: false)");
MODULE_PARM_DESC(timeout_ms, "Timeout in milliseconds (default: 1000)");

static int __init my_init(void)
{
    pr_info("Module loaded with parameters:\n");
    pr_info("  debug_level = %d\n", debug_level);
    pr_info("  device_name = %s\n", device_name);
    pr_info("  irq_number = %d\n", irq_number);
    pr_info("  enable_feature = %s\n", enable_feature ? "true" : "false");
    pr_info("  timeout_ms = %u\n", timeout_ms);

    /* Use parameters */
    if (debug_level > 0)
        pr_info("Debug mode enabled (level %d)\n", debug_level);

    if (enable_feature)
        pr_info("Special feature is enabled\n");

    return 0;
}

static void __exit my_exit(void)
{
    pr_info("Module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Module parameter demonstration");
```

### Permission Values (perm)

**Permission format: S_I[R/W/X][USR/GRP/OTH]**

```c
/* Permission bits explanation */
S_I      /* Prefix */
R        /* Read permission */
W        /* Write permission */
X        /* Execute permission */
USR      /* User (owner) */
GRP      /* Group */
OTH      /* Others */
UGO      /* User, Group, Others (all) */

/* Common combinations */
S_IRUGO           /* Read-only for everyone (0444) */
S_IWUSR | S_IRUSR /* Read-write for owner only (0600) */
S_IRUGO | S_IWUSR /* Read for all, write for owner (0644) */
0                 /* No sysfs file created */
```

**Permission table:**

```
┌───────────────────┬───────┬─────────────────────────────┐
│ Macro             │ Octal │ Meaning                     │
├───────────────────┼───────┼─────────────────────────────┤
│ 0                 │ 0000  │ No sysfs file               │
│ S_IRUSR           │ 0400  │ Owner read-only             │
│ S_IWUSR           │ 0200  │ Owner write-only            │
│ S_IRUSR|S_IWUSR   │ 0600  │ Owner read-write            │
│ S_IRUGO           │ 0444  │ Everyone read-only          │
│ S_IWUGO           │ 0222  │ Everyone write-only         │
│ S_IRUGO|S_IWUSR   │ 0644  │ All read, owner write       │
│ S_IRWXU           │ 0700  │ Owner rwx                   │
│ S_IRWXUGO         │ 0777  │ Everyone rwx (dangerous!)   │
└───────────────────┴───────┴─────────────────────────────┘
```

**Best practices:**
✅ Use `S_IRUGO` (read-only) for most parameters

✅ Use `S_IRUGO | S_IWUSR` only if runtime changes needed

❌ Never use write permissions for everyone (S_IWUGO)

❌ Avoid 0777 (security risk)

### Array Parameters

**For multiple values of same type:**

```c
#include <linux/moduleparam.h>

/* Declare array and count variable */
static int gpio_pins[4] = {0, 1, 2, 3};  /* Default values */
static int gpio_count = 4;                /* Number of elements */

/* Register array parameter */
module_param_array(gpio_pins, int, &gpio_count, S_IRUGO);
MODULE_PARM_DESC(gpio_pins, "GPIO pin numbers (up to 4)");

static int __init my_init(void)
{
    int i;

    pr_info("Using %d GPIO pins:\n", gpio_count);
    for (i = 0; i < gpio_count; i++) {
        pr_info("  GPIO[%d] = %d\n", i, gpio_pins[i]);
    }

    return 0;
}
```

**Syntax:**

```c
module_param_array(name, type, nump, perm);
```

- `name`: Array variable name
- `type`: Type of array elements
- `nump`: Pointer to int that will hold actual number of elements provided
- `perm`: Permissions

### Loading Modules with Parameters

**Method 1: insmod**

```bash
# Single parameter
sudo insmod mymodule.ko debug_level=2

# Multiple parameters
sudo insmod mymodule.ko debug_level=2 device_name="mydevice" irq_number=7

# Array parameters (comma-separated, NO SPACES)
sudo insmod mymodule.ko gpio_pins=10,11,12,13
```

**Method 2: Configuration file `/etc/modprobe.d/mymodule.conf`**

```bash
# Create config file
sudo nano /etc/modprobe.d/mymodule.conf

# Add options line
options mymodule debug_level=2 device_name="mydev" enable_feature=1

# Now just load module (parameters applied automatically)
sudo modprobe mymodule
```

### Viewing Module Parameters

**Before loading (modinfo):**

```bash
$ modinfo mymodule.ko
filename:    /path/to/mymodule.ko
parm:        debug_level:Debug output level (int)
parm:        device_name:Name of the device (charp)
```

**After loading (sysfs):**

```bash
# Read parameter values
$ cat /sys/module/mymodule/parameters/debug_level
2

# Write to parameter (if permission allows)
$ echo 3 > /sys/module/mymodule/parameters/debug_level
```

---

## 2.10 Building Your First Module

### Understanding Kbuild System

**Key concept: obj-<X> pattern**

```makefile
obj-y  += mymodule.o   # Built into kernel (static)
obj-m  += mymodule.o   # Built as loadable module
obj-n  += mymodule.o   # Not built at all
```

### Out-of-Tree Building (Development)

**Directory structure:**

```
mydriver/
├── mydriver.c          # Your driver source
├── mydriver.h          # Driver header (optional)
└── Makefile            # Build script
```

**Complete Makefile:**

```makefile
# Module name (must match .c file)
obj-m := mydriver.o

# Kernel directory (prebuilt kernel source)
KERNELDIR ?= /lib/modules/$(shell uname -r)/build

# Current directory
PWD := $(shell pwd)

# Default target
all: modules

# Build module
modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

# Install module
modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

# Clean build artifacts
clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

# Help
help:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) help

.PHONY: all modules modules_install clean help
```

**Makefile explained:**

```makefile
obj-m := mydriver.o
# Tells kbuild to build mydriver.ko from mydriver.c

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
# Location of prebuilt kernel source

$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
# -C $(KERNELDIR): Change to kernel directory
# M=$(PWD): Tell kbuild where external module source is
```

### Building the Module

```bash
# 1. Navigate to module directory
cd ~/mydriver/

# 2. Build module
make

# 3. List generated files
ls -la
# mydriver.c          # Source file
# mydriver.ko         # Kernel module (LOADABLE!)
# mydriver.o          # Object file
# Module.symvers      # Symbol versions
# modules.order       # Build order
```

### Testing the Module

```bash
# 1. Load module
sudo insmod mydriver.ko

# 2. Verify loaded
lsmod | grep mydriver

# 3. Check kernel log
dmesg | tail

# 4. View module info
modinfo mydriver.ko

# 5. Unload module
sudo rmmod mydriver
```

### Cross-Compilation

**For ARM architecture:**

```makefile
# Makefile with cross-compilation support

ARCH ?= arm
CROSS_COMPILE ?= arm-linux-gnueabihf-

KERNELDIR ?= /path/to/arm/kernel/source

obj-m := mydriver.o
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(PWD) clean

.PHONY: all clean
```

**Building for ARM:**

```bash
# Method 1: Pass on command line
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-

# Method 2: Export as environment variables
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
make

# Verify ARM binary
file mydriver.ko
# mydriver.ko: ELF 32-bit LSB relocatable, ARM
```

### Multi-File Modules

**Directory structure:**

```
mydriver/
├── main.c       # Main module code
├── device.c     # Device handling
├── interrupt.c  # Interrupt handling
└── Makefile
```

**Makefile for multi-file module:**

```makefile
# Module name
obj-m := mydriver.o

# Source files (without .c extension)
mydriver-objs := main.o device.o interrupt.o

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

.PHONY: all clean
```

### In-Tree Building (Production)

**Steps:**

**1. Choose location in kernel tree:**

```bash
# Example: Character device
cd /path/to/kernel/source
mkdir drivers/char/mydriver
cp ~/mydriver/mydriver.c drivers/char/mydriver/
```

**2. Create Kconfig:**

```bash
# drivers/char/mydriver/Kconfig

config MYDRIVER
    tristate "My Custom Character Driver"
    default m
    help
      This is my custom character device driver.

      Say Y to build into kernel.
      Say M to build as module.
      Say N to disable.
```

**3. Create local Makefile:**

```bash
# drivers/char/mydriver/Makefile

obj-$(CONFIG_MYDRIVER) += mydriver.o
```

**4. Update parent Kconfig:**

```bash
# drivers/char/Kconfig
source "drivers/char/mydriver/Kconfig"
```

**5. Update parent Makefile:**

```bash
# drivers/char/Makefile
obj-$(CONFIG_MYDRIVER) += mydriver/
```

**6. Configure kernel:**

```bash
make menuconfig
# Navigate to: Device Drivers → Character devices → My Driver
# Press M (module) or Y (built-in)
```

**7. Build:**

```bash
make modules -j$(nproc)
sudo make modules_install
```

### Installation

```bash
# Install module
sudo make modules_install

# Update module dependencies
sudo depmod -a

# Now can use modprobe
sudo modprobe mydriver
```

### Troubleshooting

**Problem: "No such file or directory"**

```bash
# Solution: Install kernel headers
sudo apt-get install linux-headers-$(uname -r)
```

**Problem: "disagrees about version of symbol"**

```bash
# Solution: Rebuild against correct kernel
make clean
make KERNELDIR=/lib/modules/$(uname -r)/build
```

### Best Practices

**DO:**
✅ Start with out-of-tree for development

✅ Use meaningful names

✅ Test thoroughly

✅ Document parameters

✅ Check dmesg logs

✅ Use version control

**DON'T:**
❌ Edit kernel Makefile directly

❌ Load modules built for different kernel

❌ Skip cleaning between rebuilds

❌ Ignore compiler warnings

This completes Chapter 2: Device Driver Basis!