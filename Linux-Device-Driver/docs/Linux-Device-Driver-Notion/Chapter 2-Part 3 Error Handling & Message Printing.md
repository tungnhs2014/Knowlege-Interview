# Part 3. Error Handling & Message Printing

This section covers proper error handling techniques and kernel message printing APIs, essential for writing robust and debuggable drivers.

---

## 2.6 Error Handling

### Why Proper Error Handling Matters

**In kernel space, errors have serious consequences:**

- Wrong error code → system makes wrong decisions
- Unhandled error → system crash (kernel panic)
- Memory leak → system becomes unstable over time
- Resource leak → device becomes unusable

**Comparison:**

```
User Space Error              Kernel Space Error
├── Process crashes           ├── System crashes (panic)
├── Can be restarted          ├── Requires reboot
├── Affects one application   ├── Affects entire system
└── Logged to user            └── Logged to kernel buffer
```

### Standard Error Codes

The kernel provides **predefined error codes** that cover almost every error scenario.

**Location of error codes:**

- `include/uapi/asm-generic/errno-base.h` (basic errors)
- `include/uapi/asm-generic/errno.h` (extended errors)

**Common error codes table:**

```
┌─────────┬────────┬──────────────────────────────────────┐
│ Code    │ Value  │ Meaning                              │
├─────────┼────────┼──────────────────────────────────────┤
│ EPERM   │ 1      │ Operation not permitted              │
│ ENOENT  │ 2      │ No such file or directory            │
│ ESRCH   │ 3      │ No such process                      │
│ EINTR   │ 4      │ Interrupted system call              │
│ EIO     │ 5      │ I/O error                            │
│ ENXIO   │ 6      │ No such device or address            │
│ E2BIG   │ 7      │ Argument list too long               │
│ ENOEXEC │ 8      │ Exec format error                    │
│ EBADF   │ 9      │ Bad file number                      │
│ ECHILD  │ 10     │ No child processes                   │
│ EAGAIN  │ 11     │ Try again                            │
│ ENOMEM  │ 12     │ Out of memory                        │
│ EACCES  │ 13     │ Permission denied                    │
│ EFAULT  │ 14     │ Bad address                          │
│ ENOTBLK │ 15     │ Block device required                │
│ EBUSY   │ 16     │ Device or resource busy              │
│ EEXIST  │ 17     │ File exists                          │
│ EXDEV   │ 18     │ Cross-device link                    │
│ ENODEV  │ 19     │ No such device                       │
│ ENOTDIR │ 20     │ Not a directory                      │
│ EISDIR  │ 21     │ Is a directory                       │
│ EINVAL  │ 22     │ Invalid argument                     │
│ ENFILE  │ 23     │ File table overflow                  │
│ EMFILE  │ 24     │ Too many open files                  │
│ ENOTTY  │ 25     │ Not a typewriter (inappropriate)     │
│ ETXTBSY │ 26     │ Text file busy                       │
│ EFBIG   │ 27     │ File too large                       │
│ ENOSPC  │ 28     │ No space left on device              │
│ ESPIPE  │ 29     │ Illegal seek                         │
│ EROFS   │ 30     │ Read-only file system                │
│ EMLINK  │ 31     │ Too many links                       │
│ EPIPE   │ 32     │ Broken pipe                          │
│ EDOM    │ 33     │ Math argument out of domain          │
│ ERANGE  │ 34     │ Math result not representable        │
└─────────┴────────┴──────────────────────────────────────┘
```

### When to Use Each Error Code

**Guidelines for choosing the right error:**

```c
/* Memory allocation failures */
ptr = kmalloc(size, GFP_KERNEL);
if (!ptr)
    return -ENOMEM;  /* Out of memory */

/* Invalid parameters */
if (speed < 0 || speed > MAX_SPEED)
    return -EINVAL;  /* Invalid argument */

/* Device not responding */
if (device_timeout())
    return -EIO;  /* I/O error */

/* Resource already in use */
if (port_is_busy())
    return -EBUSY;  /* Device or resource busy */

/* Device not found */
if (!device_exists())
    return -ENODEV;  /* No such device */

/* Permission denied */
if (!has_permission())
    return -EACCES;  /* Permission denied */

/* Operation not supported */
if (!supports_feature())
    return -EOPNOTSUPP;  /* Operation not supported */

/* Try again later */
if (would_block())
    return -EAGAIN;  /* Try again */

/* Interrupted by signal */
if (interrupted())
    return -ERESTARTSYS;  /* Restart system call */
```

### Returning Errors

**Standard error return format: negative errno**

```c
/* Correct way - return negative errno */
int my_function(void)
{
    struct device *dev;

    dev = allocate_device();
    if (!dev)
        return -ENOMEM;  /* NEGATIVE error code */

    if (init_device(dev) < 0)
        return -EIO;

    return 0;  /* Success returns 0 */
}

/* Function that uses it */
int ret;
ret = my_function();
if (ret < 0) {
    /* Error occurred, ret contains -errno */
    pr_err("my_function failed with error: %d\n", ret);
    return ret;  /* Propagate error */
}
```

**Error return conventions:**

```c
/* Integer return values */
0       = Success
-ERRNO  = Error (negative)

/* Pointer return values */
valid_ptr    = Success (non-NULL pointer)
NULL         = Error (traditional way, no error info)
ERR_PTR(-ERRNO) = Error (modern way, with error info)

/* Boolean return values */
true  = Success
false = Failure
```

### Error Propagation to User Space

When kernel error reaches user space through system call:

```c
/* Kernel driver code */
static ssize_t my_read(struct file *filp, char __user *buf,
                       size_t count, loff_t *f_pos)
{
    int ret;

    ret = read_from_hardware();
    if (ret < 0)
        return ret;  /* Return -ERRNO */

    /* ... */
    return bytes_read;
}

/* User space code */
#include <errno.h>
#include <string.h>
#include <stdio.h>

int fd = open("/dev/mydevice", O_RDONLY);
char buffer[100];

if (read(fd, buffer, 100) < 0) {
    /* errno is automatically set by C library */
    printf("Read failed: %s\n", strerror(errno));
    /* Output: "Read failed: Input/output error" */
}
```

**How it works:**

```
Kernel Driver                 User Space
    ↓                             ↓
return -EIO;                 read() returns -1
    ↓                             ↓
Kernel syscall handler       C library sets errno = EIO
    ↓                             ↓
Return -1 to user space     strerror(errno) returns "I/O error"
```

### Error Handling with goto

**Why use goto for error handling?**

The kernel coding style recommends `goto` for cleanup. This prevents:

- Deep nesting
- Code duplication
- Missing cleanup steps
- Readability issues

**Bad example (nested if):**

```c
/* DON'T DO THIS - Hard to read and maintain */
int bad_function(void)
{
    struct resource1 *res1;
    struct resource2 *res2;
    struct resource3 *res3;

    res1 = allocate_res1();
    if (res1) {
        res2 = allocate_res2();
        if (res2) {
            res3 = allocate_res3();
            if (res3) {
                if (operation() == 0) {
                    return 0;
                }
                free_res3(res3);
            }
            free_res2(res2);
        }
        free_res1(res1);
    }
    return -ENOMEM;
}
```

**Good example (goto cleanup):**

```c
/* CORRECT - Clean, linear control flow */
int good_function(void)
{
    struct resource1 *res1 = NULL;
    struct resource2 *res2 = NULL;
    struct resource3 *res3 = NULL;
    int ret = 0;

    /* Allocation step 1 */
    res1 = allocate_res1();
    if (!res1) {
        ret = -ENOMEM;
        goto err_res1;  /* Jump to appropriate cleanup */
    }

    /* Allocation step 2 */
    res2 = allocate_res2();
    if (!res2) {
        ret = -ENOMEM;
        goto err_res2;  /* Cleanup res1 */
    }

    /* Allocation step 3 */
    res3 = allocate_res3();
    if (!res3) {
        ret = -ENOMEM;
        goto err_res3;  /* Cleanup res1 and res2 */
    }

    /* Main operation */
    ret = operation();
    if (ret < 0)
        goto err_operation;  /* Cleanup everything */

    return 0;  /* Success - no cleanup needed */

err_operation:
    free_res3(res3);
err_res3:
    free_res2(res2);
err_res2:
    free_res1(res1);
err_res1:
    return ret;
}
```

**Real-world example from a GPIO driver:**

```c
static int gpio_probe(struct platform_device *pdev)
{
    struct gpio_chip *gc;
    struct resource *res;
    void __iomem *base;
    int ret;

    /* Step 1: Allocate memory */
    gc = devm_kzalloc(&pdev->dev, sizeof(*gc), GFP_KERNEL);
    if (!gc)
        return -ENOMEM;  /* Early return OK - nothing to cleanup */

    /* Step 2: Get resource */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        ret = -EINVAL;
        goto err_free_gc;
    }

    /* Step 3: Map memory */
    base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(base)) {
        ret = PTR_ERR(base);
        goto err_free_gc;
    }

    /* Step 4: Register GPIO chip */
    ret = gpiochip_add(gc);
    if (ret)
        goto err_unmap;

    /* Step 5: Request IRQ */
    ret = request_irq(gc->irq, gpio_irq_handler, 0, "gpio", gc);
    if (ret)
        goto err_remove_chip;

    platform_set_drvdata(pdev, gc);
    return 0;  /* Success! */

err_remove_chip:
    gpiochip_remove(gc);
err_unmap:
    iounmap(base);
err_free_gc:
    kfree(gc);
    return ret;
}
```

**goto rules:**
✅ ALWAYS move forward (downward in code)

✅ Use meaningful label names (err_free_mem, err_unmap, etc.)

✅ Cleanup in reverse order of allocation

✅ Each label cleans what was allocated above it

❌ NEVER jump backward

❌ NEVER skip cleanup steps

---

## 2.7 Handling Null Pointer Errors

### The Problem with NULL

Traditional approach - returning NULL pointer:

```c
/* Old style - no error information */
struct device *get_device(int id)
{
    if (id < 0)
        return NULL;  /* Why NULL? Invalid ID? No memory? No device? */

    /* ... */
}

/* Caller doesn't know WHY it failed */
struct device *dev = get_device(5);
if (!dev) {
    /* What went wrong? We don't know! */
    printk("Failed to get device\n");  /* Vague error message */
    return -EINVAL;  /* Guessing the error */
}
```

### The Solution: ERR_PTR/PTR_ERR/IS_ERR

The kernel provides three macros to encode error codes in pointers:

```c
/* Defined in include/linux/err.h */

/* Convert error code to error pointer */
void *ERR_PTR(long error);

/* Check if pointer is an error */
long IS_ERR(const void *ptr);

/* Extract error code from error pointer */
long PTR_ERR(const void *ptr);
```

**How it works internally:**

```c
/* Error pointers use last page of address space */
/* Actual kernel memory never uses these addresses */

#define MAX_ERRNO  4095

#define IS_ERR_VALUE(x) \
    ((unsigned long)(void *)(x) >= (unsigned long)-MAX_ERRNO)

static inline void * __must_check ERR_PTR(long error)
{
    return (void *) error;  /* Encode -errno as pointer */
}

static inline long __must_check PTR_ERR(__force const void *ptr)
{
    return (long) ptr;  /* Decode pointer back to -errno */
}

static inline bool __must_check IS_ERR(__force const void *ptr)
{
    return IS_ERR_VALUE((unsigned long)ptr);
}
```

### Using ERR_PTR

**Complete example:**

```c
/* Function that returns pointer with error info */
static struct iio_dev *indiodev_setup(struct device *dev, int size)
{
    struct iio_dev *indio_dev;

    /* Validate parameters */
    if (size <= 0)
        return ERR_PTR(-EINVAL);  /* Invalid argument */

    /* Allocate memory */
    indio_dev = devm_iio_device_alloc(dev, size);
    if (!indio_dev)
        return ERR_PTR(-ENOMEM);  /* Out of memory */

    /* Initialize device */
    if (init_device(indio_dev) < 0)
        return ERR_PTR(-EIO);  /* I/O error */

    return indio_dev;  /* Success - return valid pointer */
}

/* Caller function */
static int foo_probe(struct platform_device *pdev)
{
    struct iio_dev *my_indio_dev;
    int ret;

    /* Call function */
    my_indio_dev = indiodev_setup(&pdev->dev, sizeof(struct my_data));

    /* Check for error */
    if (IS_ERR(my_indio_dev)) {
        /* Extract error code */
        ret = PTR_ERR(my_indio_dev);
        dev_err(&pdev->dev, "Setup failed: %d\n", ret);
        return ret;  /* Propagate exact error */
    }

    /* Success - use the pointer */
    dev_info(&pdev->dev, "Device setup successful\n");
    return 0;
}
```

**Comparison: NULL vs ERR_PTR**

```c
/* Method 1: NULL (old way) */
struct device *dev1 = get_device_null(5);
if (!dev1) {
    return -EINVAL;  /* Guessing! Could be -ENOMEM, -ENODEV, etc. */
}

/* Method 2: ERR_PTR (modern way) */
struct device *dev2 = get_device_err_ptr(5);
if (IS_ERR(dev2)) {
    return PTR_ERR(dev2);  /* Exact error code! */
}
```

### Complete Real-World Example

```c
/* I2C device driver probe function */
static int my_i2c_probe(struct i2c_client *client,
                        const struct i2c_device_id *id)
{
    struct my_device *dev;
    struct regmap *regmap;
    int ret;

    /* Allocate device structure */
    dev = devm_kzalloc(&client->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;  /* Early return OK */

    /* Initialize regmap */
    regmap = regmap_init_i2c(client, &my_regmap_config);
    if (IS_ERR(regmap)) {
        ret = PTR_ERR(regmap);
        dev_err(&client->dev, "Failed to init regmap: %d\n", ret);
        return ret;  /* Exact error from regmap_init_i2c */
    }

    dev->regmap = regmap;
    dev->client = client;
    i2c_set_clientdata(client, dev);

    /* Register with subsystem */
    ret = my_register_device(dev);
    if (ret) {
        dev_err(&client->dev, "Failed to register: %d\n", ret);
        goto err_free_regmap;
    }

    return 0;

err_free_regmap:
    regmap_exit(regmap);
    return ret;
}
```

### Coding Style Rules

**From Linux kernel coding style:**

> "If the name of a function is an action or an imperative command, the function should return an error-code integer. If the name is a predicate, the function should return a 'succeeded' boolean."
> 

**Examples:**

```c
/* Action/Command - returns error code */
int add_work(struct work *work);
// Returns: 0 = success, -EBUSY = failure

/* Predicate - returns boolean */
bool pci_dev_present(struct pci_device_id *ids);
// Returns: true = found, false = not found

/* Action - returns error code */
int gpio_request(unsigned gpio, const char *label);
// Returns: 0 = success, -EBUSY/-EINVAL = failure

/* Predicate - returns boolean */
bool gpio_is_valid(int number);
// Returns: true = valid, false = invalid
```

**Function naming matters:**

```c
/* GOOD naming */
int configure_device(...)  /* Action → returns int */
bool device_is_ready(...)  /* Predicate → returns bool */

/* BAD naming */
bool configure_device(...) /* Action but returns bool? Confusing! */
int device_ready(...)      /* Predicate but returns int? Confusing! */
```

---

## 2.8 Message Printing - printk()

### Introduction to printk()

`printk()` is to kernel space what `printf()` is to user space.

**Key differences:**

```
printf() (User Space)        printk() (Kernel Space)
├── Outputs to stdout        ├── Outputs to kernel log buffer
├── No log levels            ├── Has 8 log levels
├── Can use floating point   ├── No floating point support
├── Links to C library       ├── Standalone kernel function
└── Process-specific         └── System-wide

```

### The 8 Kernel Log Levels

**Defined in `include/linux/kern_levels.h`:**

```c
#define KERN_SOH      "\001"        /* ASCII Start Of Header */
#define KERN_EMERG    KERN_SOH "0"  /* system is unusable */
#define KERN_ALERT    KERN_SOH "1"  /* action must be taken immediately */
#define KERN_CRIT     KERN_SOH "2"  /* critical conditions */
#define KERN_ERR      KERN_SOH "3"  /* error conditions */
#define KERN_WARNING  KERN_SOH "4"  /* warning conditions */
#define KERN_NOTICE   KERN_SOH "5"  /* normal but significant condition */
#define KERN_INFO     KERN_SOH "6"  /* informational */
#define KERN_DEBUG    KERN_SOH "7"  /* debug-level messages */
```

**Priority vs Value:**

```
Level          Value  Priority  Use Case
KERN_EMERG     0      Highest   System crash imminent
KERN_ALERT     1      ↑         Immediate action needed
KERN_CRIT      2      ↑         Critical hardware/software failure
KERN_ERR       3      ↑         Error conditions
KERN_WARNING   4      ↑         Warning messages
KERN_NOTICE    5      ↑         Significant but normal events
KERN_INFO      6      ↑         Informational messages
KERN_DEBUG     7      Lowest    Debug messages
```

**Lower value = Higher priority = More important**

### Using printk()

**Basic syntax:**

```c
printk(KERN_LEVEL "message format", args...);
```

**Examples:**

```c
/* Emergency - system unusable */
printk(KERN_EMERG "System crash! Hardware failure detected\n");

/* Alert - immediate action required */
printk(KERN_ALERT "Battery critical! System shutting down\n");

/* Critical condition */
printk(KERN_CRIT "Temperature exceeded safe limits\n");

/* Error */
printk(KERN_ERR "Failed to initialize device: error %d\n", ret);

/* Warning */
printk(KERN_WARNING "Device firmware is outdated\n");

/* Notice */
printk(KERN_NOTICE "Network link is up at 1Gbps\n");

/* Info */
printk(KERN_INFO "Driver version 1.2.3 loaded\n");

/* Debug */
printk(KERN_DEBUG "Register value: 0x%08x\n", reg_val);
```

**Note: No comma between log level and message!**

```c
/* CORRECT */
printk(KERN_ERR "Error occurred\n");

/* WRONG - This won't compile correctly */
printk(KERN_ERR, "Error occurred\n");
```

### Modern pr_* Wrapper Macros

**Recommended for new drivers:**

```c
/* Modern wrappers - defined in include/linux/printk.h */
pr_emerg("message");     /* Replaces printk(KERN_EMERG, ...) */
pr_alert("message");     /* Replaces printk(KERN_ALERT, ...) */
pr_crit("message");      /* Replaces printk(KERN_CRIT, ...) */
pr_err("message");       /* Replaces printk(KERN_ERR, ...) */
pr_warning("message");   /* Replaces printk(KERN_WARNING, ...) */
pr_warn("message");      /* Shorter alias for pr_warning */
pr_notice("message");    /* Replaces printk(KERN_NOTICE, ...) */
pr_info("message");      /* Replaces printk(KERN_INFO, ...) */
pr_debug("message");     /* Replaces printk(KERN_DEBUG, ...) */
```

**Example comparison:**

```c
/* Old style */
printk(KERN_ERR "GPIO %d initialization failed\n", gpio);
printk(KERN_INFO "Driver loaded successfully\n");

/* Modern style (RECOMMENDED) */
pr_err("GPIO %d initialization failed\n", gpio);
pr_info("Driver loaded successfully\n");
```

### Device-Specific Printing (dev_* macros)

*For device drivers, use dev_ variants:**

```c
/* Defined in include/linux/device.h */
dev_emerg(dev, fmt, ...);
dev_alert(dev, fmt, ...);
dev_crit(dev, fmt, ...);
dev_err(dev, fmt, ...);
dev_warn(dev, fmt, ...);
dev_notice(dev, fmt, ...);
dev_info(dev, fmt, ...);
dev_dbg(dev, fmt, ...);
```

**Benefits:**

- Automatically includes device name in message
- Better debugging (know which device caused issue)
- Standard format across all drivers

**Example:**

```c
static int my_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    int ret;

    /* Device-specific messages */
    dev_info(dev, "Probing device\n");

    ret = initialize_hardware(dev);
    if (ret) {
        dev_err(dev, "Hardware init failed: %d\n", ret);
        return ret;
    }

    dev_info(dev, "Device ready\n");
    return 0;
}

/* Output in dmesg:
 * [    1.234567] my-device 1234.gpio: Probing device
 * [    1.234890] my-device 1234.gpio: Device ready
 */
```

### Console Log Level

**How console printing works:**

```
Message Priority < Console Log Level  →  Print to console
Message Priority ≥ Console Log Level  →  Only in log buffer
```

**Check current console log level:**

```bash
$ cat /proc/sys/kernel/printk
4  4  1  7
│  │  │  └─ Boot-time default console log level
│  │  └──── Minimum console log level allowed
│  └─────── Default log level (CONFIG_DEFAULT_MESSAGE_LOGLEVEL)
└────────── Current console log level
```

**Change console log level:**

```bash
# Method 1: Write to /proc
echo 8 > /proc/sys/kernel/printk  # Print ALL messages (0-7)
echo 3 > /proc/sys/kernel/printk  # Print only 0-2 (EMERG, ALERT, CRIT)

# Method 2: Use dmesg
dmesg -n 5  # Set to KERN_NOTICE (print 0-4)
dmesg -n 8  # Print everything

# Method 3: At boot time (kernel command line)
loglevel=5        # Set console log level
quiet             # Same as loglevel=0 (minimal output)
ignore_loglevel   # Same as loglevel=8 (print everything)
```

**Example scenarios:**

```bash
# Scenario 1: Default (level 4)
$ cat /proc/sys/kernel/printk
4 4 1 7

pr_emerg("msg");   # Level 0 < 4 → Printed to console ✓
pr_err("msg");     # Level 3 < 4 → Printed to console ✓
pr_warning("msg"); # Level 4 = 4 → NOT printed to console ✗
pr_info("msg");    # Level 6 > 4 → NOT printed to console ✗

# Scenario 2: Debug mode (level 8)
$ echo 8 > /proc/sys/kernel/printk
pr_debug("msg");   # Level 7 < 8 → Printed to console ✓
pr_info("msg");    # Level 6 < 8 → Printed to console ✓
# ALL messages printed!

# Scenario 3: Quiet mode (level 1)
$ echo 1 > /proc/sys/kernel/printk
pr_emerg("msg");   # Level 0 < 1 → Printed to console ✓
pr_alert("msg");   # Level 1 = 1 → NOT printed to console ✗
pr_err("msg");     # Level 3 > 1 → NOT printed to console ✗
# Only EMERG messages printed!
```

### Viewing Kernel Messages

**Using dmesg command:**

```bash
# View all kernel messages
dmesg

# View last 20 lines
dmesg | tail -20

# View first 10 lines
dmesg | head -10

# Follow new messages (like tail -f)
dmesg -w

# Search for specific messages
dmesg | grep -i "error"
dmesg | grep "my_driver"

# Filter by log level
dmesg -l err          # Show only errors
dmesg -l warn,err     # Show warnings and errors
dmesg -l emerg,alert,crit  # Show critical messages

# Show with timestamps
dmesg -T              # Human-readable timestamps
dmesg --time-format=iso  # ISO 8601 format

# Clear kernel ring buffer (requires root)
sudo dmesg -C
```

**Example dmesg output:**

```
[    0.000000] Linux version 6.1.0 (user@host) (gcc version 11.3.0)
[    1.234567] my_driver: loading out-of-tree module taints kernel.
[    1.234890] my_driver: module verification failed: signature and/or required key missing - tainting kernel
[    2.345678] my_driver: Driver version 1.0.0 initialized
[    2.345901] my_driver: Found device at I2C address 0x27
[    2.346123] my_driver: Device configured successfully
[   10.123456] my_driver: IRQ 42 triggered
[   15.678901] my_driver: Temperature: 45°C
```

**Timestamp format: `[seconds.microseconds]` from boot**

### Kernel Ring Buffer

**Characteristics:**

```
Kernel Log Buffer (Ring Buffer)
├── Fixed size (configurable)
│   └── CONFIG_LOG_BUF_SHIFT
│       ├── 16 → 64KB  (1 << 16)
│       ├── 17 → 128KB (1 << 17)
│       └── 18 → 256KB (1 << 18)
│
├── Circular (wraps around when full)
│   └── Old messages overwritten
│
└── Persistent until reboot
    └── Lost on system restart
```

**Configure buffer size:**

```bash
# At compile time - kernel config
CONFIG_LOG_BUF_SHIFT=17  # 128KB buffer

# At boot time - kernel command line
log_buf_len=1M  # 1MB buffer (must be power of 2)
```

### Adding Timestamps to Messages

**Enable CONFIG_PRINTK_TIME:**

```bash
# In kernel configuration
make menuconfig
  → Kernel hacking
    → printk and dmesg options
      → [*] Show timing information on printks
```

**Result:**

```bash
# Without timestamps
my_driver: Device initialized

# With timestamps (CONFIG_PRINTK_TIME=y)
[    1.234567] my_driver: Device initialized
       ↑
       └─ seconds.microseconds from boot
```

**Runtime control:**

```bash
# Enable timestamps
echo Y > /sys/module/printk/parameters/time

# Disable timestamps
echo N > /sys/module/printk/parameters/time

# Check status
cat /sys/module/printk/parameters/time
```

### Custom Prefix with pr_fmt()

**Make all your messages include module/function name:**

```c
/* At TOP of source file, BEFORE any #include */
#define pr_fmt(fmt) "%s:%s: " fmt, KBUILD_MODNAME, __func__

#include <linux/module.h>
#include <linux/printk.h>

static int my_init(void)
{
    pr_info("Initializing\n");
    /* Output: my_driver:my_init: Initializing */

    pr_err("Failed with code %d\n", -EIO);
    /* Output: my_driver:my_init: Failed with code -5 */

    return 0;
}
```

**More pr_fmt() examples:**

```c
/* Example 1: Module name only */
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
/* Output: my_driver: Message */

/* Example 2: File and line */
#define pr_fmt(fmt) "%s:%d: " fmt, __FILE__, __LINE__
/* Output: driver.c:123: Message */

/* Example 3: Function name only */
#define pr_fmt(fmt) "%s: " fmt, __func__
/* Output: my_function: Message */

/* Example 4: Detailed (module + file + function + line) */
#define pr_fmt(fmt) "[%s] %s:%s:%d: " fmt, \
    KBUILD_MODNAME, __FILE__, __func__, __LINE__
/* Output: [my_driver] driver.c:my_function:123: Message */
```

### Debug Messages (pr_debug)

**pr_debug() behavior:**

```c
/* pr_debug is CONDITIONAL */

/* If DEBUG is defined OR CONFIG_DYNAMIC_DEBUG=y */
pr_debug("Debug message\n");  /* Message is printed */

/* Otherwise */
pr_debug("Debug message\n");  /* Compiled out (no overhead!) */
```

**Enable pr_debug messages:**

```c
/* Method 1: Define DEBUG at top of file */
#define DEBUG
#include <linux/module.h>

/* Method 2: Compile with -DDEBUG */
ccflags-y := -DDEBUG

/* Method 3: Dynamic debug (best for production) */
# Enable at runtime
echo 'module my_driver +p' > /sys/kernel/debug/dynamic_debug/control
```

### printk() Formatting

**Supported format specifiers:**

```c
/* Integers */
pr_info("int: %d\n", 123);              /* Signed decimal */
pr_info("uint: %u\n", 123);             /* Unsigned decimal */
pr_info("hex: 0x%x\n", 0xABCD);         /* Hex (lowercase) */
pr_info("HEX: 0x%X\n", 0xABCD);         /* Hex (uppercase) */
pr_info("oct: %o\n", 0755);             /* Octal */

/* Pointers */
pr_info("ptr: %p\n", ptr);              /* Pointer (hashed) */
pr_info("ptr: %px\n", ptr);             /* Pointer (real address) */
pr_info("ptr: %pK\n", ptr);             /* Pointer (hashed if not root) */

/* Strings */
pr_info("str: %s\n", "Hello");          /* String */
pr_info("str: %.10s\n", "Hello");       /* First 10 chars */

/* Character */
pr_info("char: %c\n", 'A');             /* Character */

/* Size types */
pr_info("size: %zu\n", sizeof(int));    /* size_t */
pr_info("ssize: %zd\n", ssize_val);     /* ssize_t */

/* 64-bit integers */
pr_info("ll: %lld\n", long_long_val);   /* long long */
pr_info("llu: %llu\n", ulong_long_val); /* unsigned long long */

/* Kernel-specific (see Documentation/printk-formats.txt) */
pr_info("phys: %pa\n", &phys_addr);     /* phys_addr_t */
pr_info("dma: %pad\n", &dma_addr);      /* dma_addr_t */
pr_info("resource: %pr\n", &resource);  /* struct resource */
pr_info("mac: %pM\n", mac_addr);        /* MAC address */
pr_info("ip4: %pI4\n", &ip4_addr);      /* IPv4 address */
pr_info("ip6: %pI6\n", ip6_addr);       /* IPv6 address */
```

**NO floating point support:**

```c
/* WRONG - Will not compile */
pr_info("float: %f\n", 3.14);  /* ERROR! No %f support */

/* Workaround - use integer math */
int integer_part = 314 / 100;        /* 3 */
int fractional_part = 314 % 100;     /* 14 */
pr_info("value: %d.%02d\n", integer_part, fractional_part);
/* Output: value: 3.14 */
```

### Best Practices

**DO:**
✅ Use appropriate log level for message severity

✅ Use dev_* macros in device drivers

✅ Use pr_* macros instead of printk() directly

✅ Include relevant context in messages

✅ Use pr_fmt() for consistent prefixes

✅ End messages with `\n` (newline)

**DON'T:**
❌ Print in hot paths (interrupt handlers, frequent functions)

❌ Print excessive debug in production

❌ Use floating point formats

❌ Print sensitive data (passwords, keys)

❌ Forget newlines (causes messages to merge)

❌ Use ALL CAPS IN MESSAGES (LOOKS UNPROFESSIONAL)

**Examples:**

```c
/* GOOD - Clear, informative messages */
static int my_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    int ret;

    dev_info(dev, "Probing device (version %s)\n", DRIVER_VERSION);

    ret = init_hardware(dev);
    if (ret) {
        dev_err(dev, "Hardware initialization failed: %d\n", ret);
        return ret;
    }

    dev_dbg(dev, "Registers: CTRL=0x%08x, STATUS=0x%08x\n",
            read_reg(CTRL), read_reg(STATUS));

    dev_info(dev, "Device initialized successfully\n");
    return 0;
}

/* BAD - Poor messages */
printk("error");                    /* No log level, no context */
printk(KERN_ERR "ERROR ERROR");     /* No detail, no newline */
pr_info("val=%d", val);             /* No newline */
dev_err(dev, "FAILED!!!");          /* No useful information */
```

This concludes Page 3 covering Error Handling and Message Printing.