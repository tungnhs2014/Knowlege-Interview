# Part 1. User Space vs Kernel Space & Module Concepts

This section introduces the fundamental separation between user space and kernel space, explaining how device drivers bridge these two worlds through the module system.

---

## 2.1 User Space vs Kernel Space

### Understanding the Separation

One of the most fundamental concepts in Linux is the **separation between user space and kernel space**. This separation is enforced by the CPU hardware and the operating system.

**The Big Picture:**

```
┌──────────────────────────────────────────┐
│         User Space Applications          │
│    (gedit, firefox, your programs)       │
│         Privilege: Limited               │
│         Mode: User Mode                  │
├──────────────────────────────────────────┤
│         System Call Interface            │  ← Bridge between spaces
├──────────────────────────────────────────┤
│          Kernel Space                    │
│     (device drivers, core kernel)        │
│       Privilege: Full access             │
│       Mode: Privileged/Kernel Mode       │
├──────────────────────────────────────────┤
│           Hardware                       │
└──────────────────────────────────────────┘
```

### Memory Layout Overview

On a 32-bit system with 4GB address space, the memory is typically divided:

```
0xFFFFFFFF  ┌─────────────────────────────┐
            │                             │
            │   Kernel Space (1GB)        │  ← CONFIG_PAGE_OFFSET
            │   - Kernel code             │     (0xC0000000 on x86)
0xC0000000  │   - Kernel data             │     (0x80000000 on ARM)
            │   - Device drivers          │
            ├─────────────────────────────┤
            │                             │
            │   User Space (3GB)          │
            │   - Application code        │
            │   - Application data        │
            │   - Stack, heap             │
            │   - Shared libraries        │
0x00000000  └─────────────────────────────┘
```

**This is called a 3G/1G split:**

- **Lower 3GB (0x00000000 - 0xBFFFFFFF)**: User space
- **Upper 1GB (0xC0000000 - 0xFFFFFFFF)**: Kernel space

**Why this configuration?**

- User applications get 3GB (enough for most applications)
- Kernel gets 1GB (sufficient for kernel operations)
- Kernel mapped in every process (speeds up system calls)

### User Space Explained

**Definition:** User space is where normal applications run with **restricted privileges**.

**Characteristics:**

- **Limited access**: Cannot directly access hardware or kernel memory
- **Protected**: One process cannot interfere with another process
- **Sandboxed**: If application crashes, it won't crash the system
- **Lower priority**: User mode has fewer CPU privileges

**What user space can do:**

```c
/* User space program example */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd;
    char buffer[100];

    /* Open device file - triggers system call */
    fd = open("/dev/mydevice", O_RDWR);

    /* Read from device - triggers system call */
    read(fd, buffer, sizeof(buffer));

    /* Close device - triggers system call */
    close(fd);

    return 0;
}
```

**What user space CANNOT do directly:**

- ❌ Access hardware registers
- ❌ Access kernel memory
- ❌ Execute privileged CPU instructions
- ❌ Access other processes' memory
- ❌ Disable interrupts

### Kernel Space Explained

**Definition:** Kernel space is where the kernel and device drivers run with **full privileges**.

**Characteristics:**

- **Full access**: Can access all memory and hardware
- **Privileged**: Can execute any CPU instruction
- **Shared**: All kernel code shares the same address space
- **Critical**: Bugs here can crash the entire system

**What kernel space can do:**

```c
/* Kernel space driver code */
#include <linux/module.h>
#include <linux/io.h>

/* Direct hardware access */
void __iomem *base;
base = ioremap(0x20200000, 0x1000);  /* Map hardware registers */
writel(0x12345678, base + 0x10);     /* Write to hardware */

/* Access kernel memory */
void *kmem = kmalloc(1024, GFP_KERNEL);  /* Allocate kernel memory */

/* Full system control */
local_irq_disable();  /* Disable interrupts - only kernel can do this! */
```

### CPU Privilege Levels

Modern CPUs implement **privilege levels** (also called protection rings):

**x86 Architecture:**

```
Ring 0 (Most privileged) - Kernel Mode
├── Full hardware access
├── Execute any instruction
└── Access all memory

Ring 3 (Least privileged) - User Mode
├── Limited hardware access
├── Restricted instructions
└── Protected memory
```

**ARM Architecture:**

```
EL0 (Exception Level 0) - User Mode
EL1 (Exception Level 1) - Kernel Mode
EL2 (Exception Level 2) - Hypervisor
EL3 (Exception Level 3) - Secure Monitor
```

**Current privilege level affects:**

- Which memory can be accessed
- Which CPU instructions can be executed
- Which hardware registers can be accessed

### System Call Bridge

**How user space communicates with kernel space:**

User applications cannot directly call kernel functions. They use **system calls** (syscalls) to request kernel services.

**System call process:**

```
User Space Application
    ↓
    call open("/dev/mydevice", O_RDWR)
    ↓
C Library (glibc) wrapper
    ↓
    Software interrupt (syscall instruction)
    ↓
CPU switches to Kernel Mode
    ↓
Kernel syscall handler
    ↓
Driver's open() function
    ↓
Hardware interaction
    ↓
Return to User Mode
    ↓
User Space Application continues
```

**Common system calls:**

```c
/* File operations */
open()    → sys_open()    → driver's .open()
read()    → sys_read()    → driver's .read()
write()   → sys_write()   → driver's .write()
close()   → sys_close()   → driver's .release()
ioctl()   → sys_ioctl()   → driver's .ioctl()

/* Memory operations */
mmap()    → sys_mmap()    → driver's .mmap()
```

**Example: Reading from device file**

```c
/* User space code */
char buffer[100];
int fd = open("/dev/mydevice", O_RDONLY);  /* System call #1 */
read(fd, buffer, 100);                     /* System call #2 */
close(fd);                                  /* System call #3 */

/* What happens in kernel space: */
// 1. open() → kernel checks permissions → driver's .open() called
// 2. read() → kernel validates buffer → driver's .read() called
// 3. close() → kernel cleanup → driver's .release() called
```

### Why This Separation Matters

**1. Security**

- User applications can't corrupt kernel memory
- One crashing app won't take down the system
- Malicious code cannot directly access hardware

**2. Stability**

```c
/* User space crash */
int *ptr = NULL;
*ptr = 42;  /* Segmentation fault - only kills this process */

/* Kernel space crash */
int *ptr = NULL;
*ptr = 42;  /* Kernel panic - entire system crashes! *
```

**3. Protection**

- Hardware protected from unauthorized access
- Each process isolated from others
- Resources managed centrally by kernel

### Address Space Terminology

**Virtual Address:**

- Address seen by CPU and programs
- Translated by MMU to physical address
- Can be in user space or kernel space

**Physical Address:**

- Actual address in RAM
- Not directly accessible by software
- Managed by kernel through MMU

**Distinguishing addresses:**

```c
/* Given an address, determine if it's kernel or user space */
unsigned long addr = 0xC0123456;

if (addr >= CONFIG_PAGE_OFFSET) {
    /* Kernel space address (>= 0xC0000000 on x86) */
    printk("Kernel address\n");
} else {
    /* User space address (< 0xC0000000) */
    printk("User address\n");
}
```

### Practical Implications for Driver Development

**When writing device drivers:**

✅ **You operate in kernel space:**

- Have full hardware access
- Must be extremely careful (bugs crash system)
- Cannot use standard C library (use kernel APIs)
- Must handle concurrency (interrupts, multiple CPUs)

✅ **You provide interface to user space:**

- Expose device through `/dev` files
- Implement file operations (open, read, write, etc.)
- Copy data safely between kernel and user space

✅ **You must respect the boundary:**

```c
/* WRONG - Direct user pointer dereference */
void bad_read(char __user *user_buffer) {
    *user_buffer = 'A';  /* CRASH! User pointer not valid in kernel */
}

/* CORRECT - Use copy functions */
void good_read(char __user *user_buffer) {
    char data = 'A';
    copy_to_user(user_buffer, &data, 1);  /* Safe kernel→user copy */
}
```

---

## 2.2 The Concept of Modules

### What is a Kernel Module?

A **kernel module** is to the Linux kernel what a **plugin** or **add-on** is to user applications like Firefox or VSCode.

**Key concept:** Modules extend kernel functionality **without rebooting**.

**Analogy:**

```
Firefox Browser          Linux Kernel
    ↓                        ↓
Install ad-blocker      Load driver module
    ↓                        ↓
New functionality       New device support
    ↓                        ↓
No restart needed       No reboot needed!
```

### Why Use Modules?

**Without modules:**

```
Kernel with all drivers built-in
├── Large kernel image (50+ MB)
├── Long boot time
├── Wasted memory (unused drivers loaded)
└── Need reboot to add new driver
```

**With modules:**

```
Small kernel + loadable modules
├── Small kernel image (5-10 MB)
├── Fast boot time
├── Load drivers on-demand
├── Save memory (load only needed drivers)
└── Dynamic loading (no reboot!)
```

### Module Characteristics

**1. Dynamic Loading/Unloading**

```bash
# Load module (add driver to kernel)
sudo insmod mydriver.ko

# Unload module (remove driver from kernel)
sudo rmmod mydriver

# Check loaded modules
lsmod
```

**2. Runs in Kernel Space**

- Module code executes with kernel privileges
- Has full hardware access
- Shares kernel address space
- Bugs can crash the system

**3. Extends Kernel Functionality**

```
Module Types:
├── Device drivers (GPIO, I2C, SPI, USB)
├── Filesystem support (ext4, vfat, nfs)
├── Network protocols (TCP/IP, Bluetooth)
└── System features (virtualization, security)
```

### Module Requirement: CONFIG_MODULES

For the kernel to support loadable modules, it must be compiled with:

```bash
# Check if kernel supports modules
zcat /proc/config.gz | grep CONFIG_MODULES
# Should show: CONFIG_MODULES=y

# In kernel configuration
make menuconfig
    → General setup
        → [*] Enable loadable module support
```

**What CONFIG_MODULES enables:**

- Module loading infrastructure
- Symbol export/import system
- Module versioning support
- `insmod`, `rmmod`, `modprobe` utilities

### Module Architecture

**A module consists of:**

```c
#include <linux/module.h>    /* For module macros */
#include <linux/kernel.h>    /* For printk() */
#include <linux/init.h>      /* For __init/__exit */

/* Module entry point (constructor) */
static int __init mymodule_init(void)
{
    printk(KERN_INFO "Module loaded\n");
    /* Initialize resources */
    return 0;  /* 0 = success, negative = error */
}

/* Module exit point (destructor) */
static void __exit mymodule_exit(void)
{
    printk(KERN_INFO "Module unloaded\n");
    /* Cleanup resources */
}

/* Register entry/exit points */
module_init(mymodule_init);
module_exit(mymodule_exit);

/* Module metadata */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("My first kernel module");
```

**Module lifecycle:**

```
┌─────────────────────────────────────────┐
│  Module File (.ko)                      │
│  (On disk: /lib/modules/...)            │
└───────────┬─────────────────────────────┘
            │
            │ insmod/modprobe
            ↓
┌─────────────────────────────────────────┐
│  module_init() called                   │
│  - Allocate resources                   │
│  - Register with subsystem              │
│  - Initialize hardware                  │
└───────────┬─────────────────────────────┘
            │
            │ Module is loaded
            ↓
┌─────────────────────────────────────────┐
│  Module Active in Kernel                │
│  - Serving requests                     │
│  - Handling interrupts                  │
│  - Providing functionality              │
└───────────┬─────────────────────────────┘
            │
            │ rmmod
            ↓
┌─────────────────────────────────────────┐
│  module_exit() called                   │
│  - Free resources                       │
│  - Unregister from subsystem            │
│  - Cleanup                              │
└─────────────────────────────────────────┘
```

### Static vs Dynamic Modules

**Static (Built-in):**

```bash
# In kernel config
<*> GPIO Support  # Built into kernel image

# Result
- Part of vmlinuz/zImage
- Loaded at boot time
- Cannot be unloaded
- Always in memory
```

**Dynamic (Loadable):**

```bash
# In kernel config
<M> GPIO Support  # Compiled as module

# Result
- Separate .ko file
- Loaded on demand
- Can be unloaded
- Saves memory when not needed
```

**Comparison:**

```
┌─────────────┬──────────────┬─────────────┐
│ Feature     │ Built-in (*) │ Module (M)  │
├─────────────┼──────────────┼─────────────┤
│ Load time   │ Boot only    │ Anytime     │
│ Unloadable  │ No           │ Yes         │
│ Memory      │ Always used  │ On-demand   │
│ Boot speed  │ Slower       │ Faster      │
│ Flexibility │ None         │ High        │
└─────────────┴──────────────┴─────────────┘
```

**When to use each:**

**Built-in ([*]) when:**

- Driver needed for boot (root filesystem driver)
- Critical system driver (console)
- Minimal system (embedded, no storage for modules)
- Security (prevent runtime modification)

**Module ([M]) when:**

- Optional hardware support
- Development/testing
- Save memory
- Runtime flexibility needed

### Module Naming Convention

```bash
# Module filenames
mydriver.ko      # .ko = Kernel Object
gpio_chip.ko
i2c_adapter.ko

# Module names (without extension)
mydriver
gpio_chip
i2c_adapter
```

### Where Modules Live

```bash
# Installed modules location
/lib/modules/$(uname -r)/
├── kernel/              # Kernel tree modules
│   ├── drivers/
│   │   ├── gpio/
│   │   │   └── gpio-mcp23s08.ko
│   │   ├── i2c/
│   │   └── spi/
│   ├── fs/
│   └── net/
├── extra/               # Out-of-tree modules
├── modules.dep          # Module dependencies
├── modules.dep.bin      # Binary format
├── modules.alias        # Device aliases
└── modules.symbols      # Exported symbols

```

### Module States

A module can be in several states:

```bash
# Check module state
lsmod
# Output:
# Module         Size  Used by
# mydriver       16384  0        ← Not in use
# i2c_dev        20480  2        ← Used by 2 modules
# snd_hda_intel  49152  3        ← Used by 3 modules

# Module states:
# - Loaded but not in use (Used by = 0)
# - Loaded and in use (Used by > 0)
# - Not loaded (not in lsmod output)
```

### Module vs Driver

**Important distinction:**

**Module:**

- Code packaging/loading mechanism
- Can contain any kernel functionality
- Technical term

**Driver:**

- Code that controls hardware
- Usually packaged as a module
- Functional term

**Relationship:**

```
All drivers can be modules
BUT
Not all modules are drivers!

Examples of non-driver modules:
- Filesystem support (ext4)
- Network protocol (TCP)
- Encryption algorithm
- System call tracer
```

This concludes the first page covering User Space vs Kernel Space and Module Concepts.