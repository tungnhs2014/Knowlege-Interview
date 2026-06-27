# Part 2. Module Dependencies, Loading & Information

This section explains how modules interact through dependencies, how they are loaded/unloaded, and how to manage module metadata and information.

---

## 2.3 Module Dependencies

### What are Module Dependencies?

**Definition:** A module dependency occurs when Module B uses functions or variables exported by Module A.

**Concept:**

```
Module A                    Module B
├── Exports symbols         ├── Uses symbols from A
│   ├── function_a()       │   ├── calls function_a()
│   └── variable_x         │   └── reads variable_x
│                          │
└── EXPORT_SYMBOL()        └── Depends on Module A
```

**Why dependencies exist:**

- Code reuse (don't duplicate functionality)
- Modular design (each module handles specific tasks)
- Layered architecture (core → subsystem → driver)

### Symbol Export/Import Mechanism

**Exporting symbols from a module:**

```c
/* Module A: gpio_core.c - Provides GPIO services */
#include <linux/module.h>
#include <linux/export.h>

/* Function to be shared with other modules */
int gpio_request(unsigned gpio, const char *label)
{
    /* Implementation */
    pr_info("GPIO %d requested with label: %s\n", gpio, label);
    return 0;
}

/* Export symbol - makes it available to other modules */
EXPORT_SYMBOL(gpio_request);
/* OR for GPL-only modules */
EXPORT_SYMBOL_GPL(gpio_request);

/* Variable export example */
int gpio_count = 32;
EXPORT_SYMBOL(gpio_count);

MODULE_LICENSE("GPL");
```

**Importing (using) symbols in another module:**

```c
/* Module B: gpio_driver.c - Uses GPIO services */
#include <linux/module.h>

/* External symbol declaration (defined in gpio_core.c) */
extern int gpio_request(unsigned gpio, const char *label);
extern int gpio_count;

static int __init gpio_driver_init(void)
{
    int ret;

    /* Use the imported symbol */
    ret = gpio_request(5, "my-driver");
    if (ret)
        return ret;

    pr_info("Total GPIOs available: %d\n", gpio_count);
    return 0;
}

static void __exit gpio_driver_exit(void)
{
    /* Cleanup */
}

module_init(gpio_driver_init);
module_exit(gpio_driver_exit);
MODULE_LICENSE("GPL");
```

**EXPORT_SYMBOL vs EXPORT_SYMBOL_GPL:**

```c
/* Available to ALL modules (any license) */
EXPORT_SYMBOL(function_name);

/* Available ONLY to GPL-compatible modules */
EXPORT_SYMBOL_GPL(function_name);
```

**Comparison:**

```
┌──────────────────┬──────────────┬──────────────────┐
│ Symbol Type      │ License Req  │ Use Case         │
├──────────────────┼──────────────┼──────────────────┤
│ EXPORT_SYMBOL    │ Any          │ Generic APIs     │
│ EXPORT_SYMBOL_GPL│ GPL only     │ Core kernel APIs │
└──────────────────┴──────────────┴──────────────────┘
```

**Example: Real kernel dependency chain**

```
i2c-dev module
    ↓ uses
i2c-core module
    ↓ uses
i2c-imx module (hardware driver)
```

### The depmod Utility

**What is depmod?**`depmod` (dependency module) is a tool that analyzes modules and generates dependency information.

**When depmod runs:**

```bash
# During kernel build
make modules_install
# Automatically runs: depmod -a

# Manually analyze dependencies
sudo depmod -a

# For specific kernel version
sudo depmod -a 6.1.0
```

**What depmod does:**

```
1. Scans /lib/modules/<version>/kernel/
   ↓
2. Reads each .ko file
   ↓
3. Extracts:
   - Symbols the module exports (provides)
   - Symbols the module needs (requires)
   ↓
4. Creates dependency database files:
   - modules.dep (text format)
   - modules.dep.bin (binary format)
   - modules.alias (device aliases)
   - modules.symbols (symbol index)
```

**modules.dep file format:**

```bash
# View dependencies
cat /lib/modules/$(uname -r)/modules.dep | head -5

# Example output:
kernel/drivers/gpio/gpio-mcp23s08.ko: kernel/drivers/gpio/gpio-regmap.ko
kernel/net/bluetooth/bluetooth.ko:
kernel/drivers/usb/storage/usb-storage.ko: kernel/drivers/scsi/scsi_mod.ko

# Format: module.ko: dependency1.ko dependency2.ko ...
```

**Real example - USB storage dependencies:**

```bash
$ modinfo usb-storage | grep depends
depends: scsi_mod

# This means usb-storage needs scsi_mod loaded first
```

**Dependency tree visualization:**

```
usb-storage.ko
    ↓ depends on
scsi_mod.ko
    ↓ depends on
Nothing (base module)

# Loading order:
# 1. scsi_mod.ko (no dependencies)
# 2. usb-storage.ko (after scsi_mod)
```

---

## 2.4 Module Loading and Unloading

### Manual Loading Methods

### Method 1: insmod (Basic, Low-Level)

**Characteristics:**

- Requires full path to .ko file
- Does NOT handle dependencies
- Used during development
- Fast and direct

**Usage:**

```bash
# Load module with full path
sudo insmod /path/to/mydriver.ko

# Example: Load GPIO driver
sudo insmod /lib/modules/$(uname -r)/kernel/drivers/gpio/gpio-mcp23s08.ko

# With parameters
sudo insmod mydriver.ko debug=1 irq=5

```

**When insmod fails due to dependencies:**

```bash
$ sudo insmod /lib/modules/$(uname -r)/kernel/drivers/usb/storage/usb-storage.ko
insmod: ERROR: could not insert module: Unknown symbol in module

# Check dmesg for details
$ dmesg | tail
usb_storage: Unknown symbol scsi_command_size

# Problem: scsi_mod.ko not loaded (dependency missing)
```

**Manual dependency resolution:**

```bash
# Load dependency first
sudo insmod /lib/modules/$(uname -r)/kernel/drivers/scsi/scsi_mod.ko

# Then load the module
sudo insmod /lib/modules/$(uname -r)/kernel/drivers/usb/storage/usb-storage.ko

# Success!
```

### Method 2: modprobe (Smart, High-Level)

**Characteristics:**

- Only needs module name (no path, no .ko)
- Automatically handles dependencies
- Used in production
- Reads modules.dep file

**Usage:**

```bash
# Load module by name
sudo modprobe usb-storage

# What modprobe does internally:
# 1. Reads /lib/modules/$(uname -r)/modules.dep
# 2. Finds usb-storage dependencies (scsi_mod)
# 3. Loads scsi_mod first
# 4. Then loads usb-storage
# 5. All done automatically!

# With parameters
sudo modprobe usb-storage delay_use=0

# Dry-run (show what would be done)
sudo modprobe -n usb-storage
```

**insmod vs modprobe Comparison:**

```
┌─────────────────┬─────────────┬──────────────┐
│ Feature         │ insmod      │ modprobe     │
├─────────────────┼─────────────┼──────────────┤
│ Input           │ Full path   │ Module name  │
│ Dependencies    │ Manual      │ Automatic    │
│ Speed           │ Faster      │ Slightly slow│
│ Use case        │ Development │ Production   │
│ Complexity      │ Simple      │ Intelligent  │
└─────────────────┴─────────────┴──────────────┘
```

**Example scenario:**

```bash
# Developer testing new driver
cd ~/mydriver/
sudo insmod mydriver.ko    # Quick test, no install needed

# Production deployment
sudo cp mydriver.ko /lib/modules/$(uname -r)/extra/
sudo depmod -a             # Update dependencies
sudo modprobe mydriver     # Smart loading with dependencies
```

### Checking Loaded Modules

**lsmod command:**

```bash
# List all loaded modules
lsmod

# Output format:
# Module         Size  Used by
# i2c_dev       20480  0
# i2c_imx       24576  0
# bluetooth    491520  31 btrtl,btintel,btbcm,bnep,btusb,rfcomm

# Columns:
# - Module: Module name
# - Size: Memory used (bytes)
# - Used by: Reference count and dependent modules
```

**Explanation of "Used by" column:**

```bash
Module            Size  Used by
gpio_mcp23s08    16384  2        # Used by 2 modules/processes
i2c_core         65536  5 i2c_dev,i2c_imx,rtc_ds1307,gpio_mcp23s08
                        ↑ ↑
                        │ └─ Modules depending on i2c_core
                        └─ Reference count (5 users)
```

**Alternative: /proc/modules**

```bash
# Same information as lsmod
cat /proc/modules

# Format: name size refcount dependencies state address
# Example:
# i2c_core 65536 5 i2c_dev,i2c_imx,rtc_ds1307 Live 0xbf000000
```

### Module Unloading

### Method 1: rmmod (Basic)

**Characteristics:**

- Removes single module
- Does NOT remove dependencies
- Fails if module in use

**Usage:**

```bash
# Unload module by name
sudo rmmod mydriver

# Force unload (dangerous! can crash system)
sudo rmmod -f mydriver
```

**Common rmmod errors:**

```bash
# Error 1: Module in use
$ sudo rmmod i2c_core
rmmod: ERROR: Module i2c_core is in use by: i2c_dev i2c_imx

# Solution: Unload dependent modules first
$ sudo rmmod i2c_dev i2c_imx
$ sudo rmmod i2c_core

# Error 2: Module doesn't support unloading
$ sudo rmmod some_module
rmmod: ERROR: could not remove module: Operation not permitted

# Reason: CONFIG_MODULE_UNLOAD not enabled in kernel
```

### Method 2: modprobe -r (Smart)

**Characteristics:**

- Removes module AND unused dependencies
- Smart cleanup
- Recommended method

**Usage:**

```bash
# Remove module and unused dependencies
sudo modprobe -r usb-storage

# What it does:
# 1. Unload usb-storage
# 2. Check if scsi_mod still needed by other modules
# 3. If not needed, unload scsi_mod too
# 4. Clean dependency chain
```

**Example comparison:**

```bash
# Scenario: Load usb-storage (loads scsi_mod as dependency)
$ sudo modprobe usb-storage
$ lsmod | grep -E "usb_storage|scsi_mod"
usb_storage    69632  0
scsi_mod      147456  2 usb_storage,sd

# Method 1: rmmod (leaves dependency)
$ sudo rmmod usb_storage
$ lsmod | grep scsi_mod
scsi_mod      147456  1 sd     # Still loaded!

# Method 2: modprobe -r (cleans dependencies)
$ sudo modprobe usb-storage
$ sudo modprobe -r usb-storage
$ lsmod | grep scsi_mod
# (empty - scsi_mod removed if no other users)
```

### Reference Counting

**How kernel tracks module usage:**

```c
/* Inside kernel */
struct module {
    char name[MODULE_NAME_LEN];
    unsigned int refcnt;  /* Reference counter */
    /* ... */
};

/* When module is used */
try_module_get(module);   /* Increment refcnt */

/* When module released */
module_put(module);       /* Decrement refcnt */

/* Can only unload if refcnt == 0 */
```

**Preventing module unload:**

```bash
# Module being used by device
$ sudo rmmod gpio_mcp23s08
rmmod: ERROR: Module gpio_mcp23s08 is in use

# Check reference count
$ lsmod | grep gpio_mcp23s08
gpio_mcp23s08  16384  2     # refcnt = 2, can't unload

# After closing all users
$ lsmod | grep gpio_mcp23s08
gpio_mcp23s08  16384  0     # refcnt = 0, can unload now
$ sudo rmmod gpio_mcp23s08  # Success!
```

### Boot-Time Loading: /etc/modules-load.d/

**Automatic module loading at boot:**

```bash
# Create configuration file
sudo nano /etc/modules-load.d/mymodules.conf

# Add module names (one per line)
# Comments start with #
# This is a comment
i2c-dev
gpio-mcp23s08
rtc-ds1307

# On next boot, these modules load automatically
```

**Multiple configuration files:**

```bash
/etc/modules-load.d/
├── network.conf      # Network drivers
│   ├── r8169
│   └── tg3
├── storage.conf      # Storage drivers
│   ├── usb-storage
│   └── sd_mod
└── custom.conf       # Custom drivers
    └── mydriver
```

### Auto-Loading (Hotplug)

**How auto-loading works:**

```
1. Device plugged in (USB, PCI, etc.)
   ↓
2. Kernel detects new hardware
   ↓
3. Kernel identifies device (vendor ID, product ID)
   ↓
4. Kernel sends uevent to user space
   ↓
5. udev receives uevent
   ↓
6. udev searches modules.alias
   ↓
7. udev finds matching driver
   ↓
8. udev calls modprobe to load driver
   ↓
9. Driver initializes device
   ↓
10. Device ready to use!
```

**modules.alias file:**

```bash
# View alias database
cat /lib/modules/$(uname -r)/modules.alias | head

# Example entries:
alias usb:v0403pFF1C* ftdi_sio
alias pci:v00008086d00001502sv*sd*bc*sc*i* e1000e
alias i2c:mcp23008 gpio-mcp23s08

# Format: alias <bus>:<device_id_pattern> <module_name>
```

**Real-world example:**

```bash
# Plug in USB FTDI device (VID=0x0403, PID=0xFF1C)

# Kernel generates uevent:
# usb:v0403pFF1Cd*dc*dsc*dp*ic*isc*ip*in*

# udev searches modules.alias and finds:
# alias usb:v0403pFF1C* ftdi_sio

# udev automatically loads:
$ sudo modprobe ftdi_sio

# Device now available as /dev/ttyUSB0
```

---

## 2.5 Module Information

### MODULE_* Macros

**Purpose:** Provide metadata about the module for users and the system.

*Common MODULE_ macros:**

```c
#include <linux/module.h>

/* License (MANDATORY!) */
MODULE_LICENSE("GPL");
MODULE_LICENSE("GPL v2");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_LICENSE("Proprietary");

/* Author information */
MODULE_AUTHOR("John Doe <john@example.com>");

/* Description */
MODULE_DESCRIPTION("My GPIO driver for MCP23S08");

/* Version */
MODULE_VERSION("1.0.0");

/* Supported devices (auto-loaded via modules.alias) */
MODULE_ALIAS("i2c:mcp23008");
MODULE_ALIAS("spi:mcp23s08");

/* Module parameters description */
MODULE_PARM_DESC(debug, "Enable debug mode (0=off, 1=on)");
```

### MODULE_LICENSE - Critical!

**Why MODULE_LICENSE matters:**

**1. Symbol Access:**

```c
/* Kernel code */
EXPORT_SYMBOL_GPL(some_internal_api);  /* GPL-only symbol */
EXPORT_SYMBOL(some_public_api);        /* Available to all */

/* GPL module - can use both */
MODULE_LICENSE("GPL");
// Can call: some_internal_api() ✓
// Can call: some_public_api() ✓

/* Proprietary module - limited access */
MODULE_LICENSE("Proprietary");
// Can call: some_internal_api() ✗ ERROR!
// Can call: some_public_api() ✓
```

**2. Kernel Tainting:**

```bash
# Load GPL module
$ sudo insmod my_gpl_driver.ko
# Kernel: Normal operation

# Load proprietary module
$ sudo insmod proprietary.ko
# Kernel: WARNING: Loading module with non-GPL license
# Kernel is now TAINTED!

# Check taint status
$ cat /proc/sys/kernel/tainted
1    # Non-zero = tainted

# What tainting means:
# - Community won't support you
# - Bug reports ignored
# - "You're on your own"
```

**Accepted licenses:**

```c
/* Free/Open Source licenses */
MODULE_LICENSE("GPL");                    /* GNU Public License v2 or later */
MODULE_LICENSE("GPL v2");                 /* GNU Public License v2 only */
MODULE_LICENSE("GPL and additional rights");
MODULE_LICENSE("Dual BSD/GPL");           /* GPL or BSD */
MODULE_LICENSE("Dual MIT/GPL");           /* GPL or MIT */
MODULE_LICENSE("Dual MPL/GPL");           /* GPL or Mozilla */

/* Non-free */
MODULE_LICENSE("Proprietary");            /* Non-free product */
```

### Viewing Module Information: modinfo

**Basic usage:**

```bash
# View module info
modinfo gpio-mcp23s08

# Output:
filename:    /lib/modules/6.1.0/kernel/drivers/gpio/gpio-mcp23s08.ko.xz
license:     GPL
description: Driver for MCP23S08/MCP23S17 I/O expander
author:      Peter Korsgaard <peter.korsgaard@barco.com>
alias:       spi:mcp23s18
alias:       spi:mcp23s17
alias:       i2c:mcp23018
alias:       i2c:mcp23017
depends:     gpio-regmap
intree:      Y
name:        gpio_mcp23s08
vermagic:    6.1.0 SMP preempt mod_unload modversions ARMv7
parm:        debug:Debug mode (bool)
```

**Key information explained:**

```
filename:     Where .ko file is located
license:      Module license type
description:  What the module does
author:       Who wrote it
alias:        Device identifiers for auto-loading
depends:      Module dependencies
intree:       Y=kernel tree, N=out-of-tree
name:         Module name
vermagic:     Kernel version compatibility
parm:         Available parameters
```

**modinfo for specific module path:**

```bash
# Check your compiled module before installing
modinfo ./mydriver.ko

# View only specific fields
modinfo -F license gpio-mcp23s08
# Output: GPL

modinfo -F depends usb-storage
# Output: scsi_mod
```

### Module Information Section (.modinfo)

**ELF section containing metadata:**

```bash
# View .modinfo section directly
objdump -s -j .modinfo mydriver.ko

# Or use strings
strings mydriver.ko | grep -A 5 "license"

# Output shows raw MODULE_* data:
# license=GPL
# author=John Doe <john@example.com>
# description=My driver
# depends=i2c-core
```

*How MODULE_ macros work internally:**

```c
/* MODULE_LICENSE macro expansion */
#define MODULE_LICENSE(license) \
    MODULE_INFO(license, license)

#define MODULE_INFO(tag, info) \
    static const char __UNIQUE_ID(tag)[] \
    __used __section(".modinfo") = __stringify(tag) "=" info

/* Your code: */
MODULE_LICENSE("GPL");

/* After preprocessor: */
static const char __UNIQUE_ID_license_123[] \
    __used __section(".modinfo") = "license" "=" "GPL";
/* This places string "license=GPL" in .modinfo section */
```

### sysfs Module Interface

**Runtime module information via /sys/module/:**

```bash
# Navigate to module sysfs directory
cd /sys/module/gpio_mcp23s08/

# Contents:
ls -la
drwxr-xr-x  holders/      # Modules depending on this
drwxr-xr-x  parameters/   # Module parameters (runtime)
-r--r--r--  refcnt        # Reference count
dr-xr-xr-x  sections/     # Memory sections
-r--r--r--  srcversion    # Source version hash
-r--r--r--  version       # Module version

# View reference count
cat /sys/module/gpio_mcp23s08/refcnt
# Output: 2

# View parameters (covered in detail later)
ls /sys/module/gpio_mcp23s08/parameters/
debug  # Example parameter

# Read parameter value
cat /sys/module/gpio_mcp23s08/parameters/debug
# Output: 0
```

**Checking module dependencies via sysfs:**

```bash
# See what modules depend on i2c-core
ls /sys/module/i2c_core/holders/
i2c_dev  i2c_imx  gpio_mcp23s08

# These modules have loaded i2c-core
# i2c-core cannot be unloaded while these are loaded
```

This concludes Page 2 covering Module Dependencies, Loading, and Information.