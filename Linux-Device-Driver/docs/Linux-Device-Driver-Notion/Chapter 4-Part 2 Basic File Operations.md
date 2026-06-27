# Part 2. Basic File Operations

This part covers the fundamental file operations that enable user space applications to interact with your character device. You'll learn how to implement open, release, and data exchange functions.

---

## 4.2 File Operations Overview

### Introduction to file_operations

**The file_operations structure** defines what operations user space can perform on your device file.

```c
/* Defined in include/linux/fs.h */
struct file_operations {
    struct module *owner;
    loff_t (*llseek) (struct file *, loff_t, int);
    ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
    ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
    unsigned int (*poll) (struct file *, struct poll_table_struct *);
    long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
    int (*mmap) (struct file *, struct vm_area_struct *);
    int (*open) (struct inode *, struct file *);
    int (*release) (struct inode *, struct file *);
    int (*fsync) (struct file *, loff_t, loff_t, int datasync);
    int (*fasync) (int, struct file *, int);
    /* ... more operations ... */
};
```

**Mapping to system calls:**

```
User Space          Kernel Space
─────────────────   ─────────────────────────
open()         →    .open()
close()        →    .release()
read()         →    .read()
write()        →    .write()
lseek()        →    .llseek()
ioctl()        →    .unlocked_ioctl()
poll/select()  →    .poll()
mmap()         →    .mmap()
```

**Important notes:**

- ✅ **All methods are optional** - If not implemented, kernel returns appropriate error
- ✅ **NULL method = error code** - e.g., NULL `.write` returns `EINVAL`
- ✅ **Use designated initializers** - `.member = value` syntax

**Example initialization:**

```c
static struct file_operations my_fops = {
    .owner          = THIS_MODULE,
    .open           = my_open,
    .release        = my_release,
    .read           = my_read,
    .write          = my_write,
    .llseek         = my_llseek,
    .unlocked_ioctl = my_ioctl,
};
```

### File Representation in Kernel

**Two key structures:**

**1. struct inode** - Represents file on disk (filesystem level)

```c
/* Simplified inode structure */
struct inode {
    umode_t i_mode;              /* File type and permissions */
    kuid_t i_uid;                /* Owner user ID */
    kgid_t i_gid;                /* Owner group ID */
    dev_t i_rdev;                /* Device number (for device files) */
    loff_t i_size;               /* File size */
    struct timespec i_atime;     /* Access time */
    struct timespec i_mtime;     /* Modification time */
    struct timespec i_ctime;     /* Change time */

    /* For character devices */
    struct cdev *i_cdev;         /* Pointer to cdev structure */

    /* ... many more fields ... */
};
```

**2. struct file** - Represents open file (per file descriptor)

```c
/* Simplified file structure */
struct file {
    struct path f_path;                    /* Path to file */
    struct inode *f_inode;                 /* Associated inode */
    const struct file_operations *f_op;    /* File operations */

    unsigned int f_flags;                  /* File flags (O_RDONLY, etc.) */
    fmode_t f_mode;                        /* File mode (FMODE_READ, etc.) */
    loff_t f_pos;                         /* Current file position */

    void *private_data;                    /* Driver's private data */

    /* ... more fields ... */
};
```

**Relationship:**

```
                One inode, Multiple file descriptors

    Inode (on disk)               File (in memory)
    ┌─────────────┐               ┌──────────────┐
    │ i_rdev      │               │ f_inode  ────┼─→ points to inode
    │ i_cdev  ────┼──┐            │ f_op         │
    │ (Device)    │  │            │ f_pos = 0    │
    └─────────────┘  │            │ private_data │
                     │            └──────────────┘
                     │
                     │            ┌──────────────┐
                     └────────────┼→ f_inode     │
                                  │ f_op         │
                     (Same inode  │ f_pos = 100  │
                      shared by   │ private_data │
                      multiple    └──────────────┘
                      opens)

Example:
    Process A: fd1 = open("/dev/mydevice", O_RDONLY)  → file struct 1
    Process B: fd2 = open("/dev/mydevice", O_RDWR)    → file struct 2

    Both point to SAME inode, but DIFFERENT file structures
    Each has independent f_pos and private_data
```

**Key differences:**

| Feature | inode | file |
| --- | --- | --- |
| **Represents** | File on disk | Open file instance |
| **Lifetime** | Persistent | Per open() |
| **Instances** | One per file | One per file descriptor |
| **Contains** | Metadata, device info | File position, flags |
| **f_pos** | ❌ No | ✅ Yes (each fd independent) |

---

## 4.2.2 User-Kernel Data Exchange

### The __user Annotation

**What is `__user`?**

`__user` is a **marker** (not enforced by GCC) that indicates a pointer points to **user space** memory.

```c
/* User space pointer */
char __user *user_buffer;

/* Kernel space pointer */
char *kernel_buffer;
```

**Why important?**

```
User Space Memory               Kernel Space Memory
├─ Virtual addresses            ├─ Virtual addresses
├─ Paged (can be swapped)       ├─ Never swapped
├─ May not be mapped            ├─ Always mapped
├─ Can cause page faults        ├─ Direct access safe
└─ CANNOT dereference directly  └─ Can dereference directly

WRONG:
    char __user *ubuf;
    char data = *ubuf;  /* CRASH! Cannot access user memory directly */

CORRECT:
    char __user *ubuf;
    char data;
    copy_from_user(&data, ubuf, 1);  /* Use kernel function */
```

**The `__user` annotation helps:**

- ✅ Static analysis tools (sparse) detect bugs
- ✅ Developers avoid direct dereferencing
- ✅ Code review catches mistakes

### Data Copy Functions

**Bulk data transfer:**

```c
/* Copy FROM user space TO kernel space */
unsigned long copy_from_user(void *to, const void __user *from,
                             unsigned long n);

/* Copy FROM kernel space TO user space */
unsigned long copy_to_user(void __user *to, const void *from,
                          unsigned long n);

/* Parameters:
 * to   - Destination address
 * from - Source address
 * n    - Number of bytes to copy
 *
 * Returns: Number of bytes that COULD NOT be copied
 *          0 = complete success
 *          >0 = partial/failed copy
 */
```

**How they work:**

```c
/* Example: Reading user buffer */
char __user *user_buffer;  /* From user space */
char kernel_buffer[100];   /* In kernel space */
unsigned long ret;

/* Copy from user to kernel */
ret = copy_from_user(kernel_buffer, user_buffer, 100);
if (ret != 0) {
    printk(KERN_ERR "Failed to copy %lu bytes\n", ret);
    return -EFAULT;  /* Bad address */
}

/* Now kernel_buffer contains user data */
/* Process it safely... */
```

**Error handling:**

```c
/* Always check return value! */
if (copy_from_user(kbuf, ubuf, size)) {
    return -EFAULT;  /* Standard error for bad address */
}

/* copy_to_user() can fail if:
 * 1. User buffer is invalid
 * 2. User buffer is not writable
 * 3. Page fault cannot be resolved
 */
if (copy_to_user(ubuf, kbuf, size)) {
    return -EFAULT;
}
```

### Single Value Copy

**For simple types (int, char, etc.):**

```c
/* Get single value from user space */
int get_user(x, ptr);

/* Put single value to user space */
int put_user(x, ptr);

/* Parameters:
 * x   - Kernel variable (get_user) or value (put_user)
 * ptr - User space pointer
 *
 * Returns: 0 on success, -EFAULT on error
 */
```

**Example:**

```c
/* Read single integer from user */
int __user *user_val;
int kernel_val;

if (get_user(kernel_val, user_val)) {
    return -EFAULT;
}

printk("Received value: %d\n", kernel_val);

/* Write single integer to user */
int result = 42;
if (put_user(result, user_val)) {
    return -EFAULT;
}
```

**When to use what:**

| Function | Use Case | Performance |
| --- | --- | --- |
| `copy_from_user()` | Arrays, structures, buffers | Optimized for bulk |
| `copy_to_user()` | Arrays, structures, buffers | Optimized for bulk |
| `get_user()` | Single simple value | Fast macro |
| `put_user()` | Single simple value | Fast macro |

---

## 4.2.3 The open() Method

### Function Prototype

```c
int (*open)(struct inode *inode, struct file *file);

/* Parameters:
 * inode - Inode of device file being opened
 * file  - File structure for this open instance
 *
 * Returns: 0 on success, negative error code on failure
 */
```

### What open() Should Do

**Typical tasks:**

1. ✅ **Initialize device** - Wake from low power, configure hardware
2. ✅ **Check permissions** - Validate open flags (O_RDONLY, O_WRONLY, O_RDWR)
3. ✅ **Identify device** - Use minor number to identify which device
4. ✅ **Allocate resources** - Allocate buffers, initialize state
5. ✅ **Setup private_data** - Store per-instance data in `file->private_data`

**Return values:**

```c
return 0;          /* Success */
return -ENOMEM;    /* Out of memory */
return -ENODEV;    /* No such device */
return -EBUSY;     /* Device busy */
return -EINVAL;    /* Invalid argument */
```

### Per-Device Data Pattern

**Problem:** How to access device-specific data from file operations?

**Solution:** Use `container_of()` macro

```c
/* Your device structure embeds cdev */
struct my_device {
    struct cdev cdev;      /* Character device structure */
    int device_id;         /* Device-specific data */
    char *buffer;
    size_t buffer_size;
    /* ... more fields ... */
};

static int my_open(struct inode *inode, struct file *file)
{
    struct my_device *dev;

    /* Get device structure from inode->i_cdev */
    dev = container_of(inode->i_cdev, struct my_device, cdev);

    /* Store in file->private_data for later use */
    file->private_data = dev;

    printk("Device %d opened\n", dev->device_id);
    return 0;
}

/* Now other functions can access device data */
static ssize_t my_read(struct file *file, char __user *buf,
                      size_t len, loff_t *offset)
{
    struct my_device *dev = file->private_data;

    /* Use dev->buffer, dev->device_id, etc. */
    return 0;
}
```

### Checking Open Flags

```c
static int my_open(struct inode *inode, struct file *file)
{
    unsigned int flags = file->f_flags;

    /* Check access mode */
    if ((flags & O_ACCMODE) == O_RDONLY) {
        printk("Opened read-only\n");
    } else if ((flags & O_ACCMODE) == O_WRONLY) {
        printk("Opened write-only\n");
    } else if ((flags & O_ACCMODE) == O_RDWR) {
        printk("Opened read-write\n");
    }

    /* Check other flags */
    if (flags & O_NONBLOCK)
        printk("Non-blocking mode\n");

    if (flags & O_APPEND)
        printk("Append mode\n");

    return 0;
}
```

### Complete open() Example

```c
struct char_device {
    struct cdev cdev;
    int minor;
    char *buffer;
    size_t buffer_size;
    bool device_ready;
    struct mutex lock;
};

static int char_open(struct inode *inode, struct file *file)
{
    struct char_device *dev;
    unsigned int minor;

    /* Get minor number */
    minor = iminor(inode);

    /* Get device structure */
    dev = container_of(inode->i_cdev, struct char_device, cdev);

    /* Verify this is our device */
    if (dev->minor != minor) {
        printk(KERN_ERR "Minor number mismatch\n");
        return -ENODEV;
    }

    /* Check if device is ready */
    if (!dev->device_ready) {
        printk(KERN_ERR "Device not ready\n");
        return -EBUSY;
    }

    /* Allocate buffer on first open */
    if (!dev->buffer) {
        dev->buffer = kzalloc(PAGE_SIZE, GFP_KERNEL);
        if (!dev->buffer) {
            printk(KERN_ERR "Failed to allocate buffer\n");
            return -ENOMEM;
        }
        dev->buffer_size = PAGE_SIZE;
    }

    /* Store device pointer in file's private_data */
    file->private_data = dev;

    /* Reset file position */
    file->f_pos = 0;

    printk(KERN_INFO "Device %d opened (flags: 0x%x)\n",
           minor, file->f_flags);

    return 0;
}
```

---

## 4.2.4 The release() Method

### Function Prototype

```c
int (*release)(struct inode *inode, struct file *file);

/* Called when last reference to file is closed
 * (when f_count reaches 0)
 *
 * Returns: 0 on success, negative error code on failure
 */
```

### What release() Should Do

**Typical tasks:**

1. ✅ **Cleanup resources** - Free allocated memory
2. ✅ **Reset device state** - Return to default/low power state
3. ✅ **Undo open()** - Reverse everything open() did
4. ✅ **Free private data** - If allocated dynamically

**Important distinction:**

```
close() vs release():

User calls close(fd):
├─ Decrements file->f_count
├─ If f_count > 0: Nothing happens (other fds still open)
└─ If f_count == 0: driver's release() called

Example:
    fd1 = open("/dev/mydev", O_RDWR);  /* f_count = 1 */
    fd2 = dup(fd1);                     /* f_count = 2 */

    close(fd1);  /* f_count = 1, release() NOT called */
    close(fd2);  /* f_count = 0, release() CALLED! */
```

### Complete release() Example

```c
static int char_release(struct inode *inode, struct file *file)
{
    struct char_device *dev = file->private_data;

    if (!dev) {
        printk(KERN_WARNING "release: No private data\n");
        return 0;
    }

    printk(KERN_INFO "Device %d released\n", dev->minor);

    /* Note: We don't free dev->buffer here because:
     * 1. Other processes might still have device open
     * 2. Buffer is reused across opens for efficiency
     *
     * Buffer is freed in module_exit() instead
     */

    /* Clear private data */
    file->private_data = NULL;

    return 0;
}
```

### When to Free Resources

**Guidelines:**

```c
/* Allocated in module_init() → Free in module_exit() */
static int __init my_init(void) {
    dev->permanent_buffer = kmalloc(SIZE, GFP_KERNEL);
}
static void __exit my_exit(void) {
    kfree(dev->permanent_buffer);
}

/* Allocated in open() → Free in release() */
static int my_open(struct inode *i, struct file *f) {
    struct temp_data *td = kzalloc(sizeof(*td), GFP_KERNEL);
    f->private_data = td;
}
static int my_release(struct inode *i, struct file *f) {
    kfree(f->private_data);
}
```

---

## 4.2.5 Complete Basic Driver Example

### Pseudo Character Device (pcd)

This example implements a simple pseudo character device with in-memory buffer:

```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "pcd"
#define CLASS_NAME  "pcd_class"
#define BUFFER_SIZE 512

/* Device structure */
struct pcd_device {
    struct cdev cdev;
    dev_t dev_num;
    struct class *class;
    struct device *device;

    char buffer[BUFFER_SIZE];  /* Device memory */
    struct mutex lock;         /* Protect buffer access */
};

static struct pcd_device pcd_dev;

/*
 * Open function
 */
static int pcd_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "pcd: open() called\n");

    /* Store device pointer in private_data */
    file->private_data = &pcd_dev;

    /* Log access mode */
    switch (file->f_flags & O_ACCMODE) {
    case O_RDONLY:
        printk(KERN_INFO "pcd: Opened read-only\n");
        break;
    case O_WRONLY:
        printk(KERN_INFO "pcd: Opened write-only\n");
        break;
    case O_RDWR:
        printk(KERN_INFO "pcd: Opened read-write\n");
        break;
    }

    return 0;
}

/*
 * Release function
 */
static int pcd_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "pcd: release() called\n");
    return 0;
}

/*
 * Read function - copy data from device to user
 */
static ssize_t pcd_read(struct file *file, char __user *buf,
                       size_t count, loff_t *f_pos)
{
    struct pcd_device *dev = file->private_data;
    size_t bytes_to_read;

    printk(KERN_INFO "pcd: read() called: count=%zu, f_pos=%lld\n",
           count, *f_pos);

    /* Check for EOF */
    if (*f_pos >= BUFFER_SIZE) {
        printk(KERN_INFO "pcd: EOF reached\n");
        return 0;  /* EOF */
    }

    /* Calculate how many bytes to read */
    bytes_to_read = min(count, (size_t)(BUFFER_SIZE - *f_pos));

    /* Lock buffer access */
    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    /* Copy to user space */
    if (copy_to_user(buf, dev->buffer + *f_pos, bytes_to_read)) {
        mutex_unlock(&dev->lock);
        return -EFAULT;
    }

    mutex_unlock(&dev->lock);

    /* Update file position */
    *f_pos += bytes_to_read;

    printk(KERN_INFO "pcd: Read %zu bytes, new f_pos=%lld\n",
           bytes_to_read, *f_pos);

    return bytes_to_read;
}

/*
 * Write function - copy data from user to device
 */
static ssize_t pcd_write(struct file *file, const char __user *buf,
                        size_t count, loff_t *f_pos)
{
    struct pcd_device *dev = file->private_data;
    size_t bytes_to_write;

    printk(KERN_INFO "pcd: write() called: count=%zu, f_pos=%lld\n",
           count, *f_pos);

    /* Check if buffer is full */
    if (*f_pos >= BUFFER_SIZE) {
        printk(KERN_ERR "pcd: Buffer full\n");
        return -ENOSPC;  /* No space left */
    }

    /* Calculate how many bytes to write */
    bytes_to_write = min(count, (size_t)(BUFFER_SIZE - *f_pos));

    /* Lock buffer access */
    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;

    /* Copy from user space */
    if (copy_from_user(dev->buffer + *f_pos, buf, bytes_to_write)) {
        mutex_unlock(&dev->lock);
        return -EFAULT;
    }

    mutex_unlock(&dev->lock);

    /* Update file position */
    *f_pos += bytes_to_write;

    printk(KERN_INFO "pcd: Wrote %zu bytes, new f_pos=%lld\n",
           bytes_to_write, *f_pos);

    return bytes_to_write;
}

/* File operations structure */
static struct file_operations pcd_fops = {
    .owner   = THIS_MODULE,
    .open    = pcd_open,
    .release = pcd_release,
    .read    = pcd_read,
    .write   = pcd_write,
};

/*
 * Module initialization
 */
static int __init pcd_init(void)
{
    int ret;

    printk(KERN_INFO "pcd: Initializing module\n");

    /* Initialize mutex */
    mutex_init(&pcd_dev.lock);

    /* Zero out buffer */
    memset(pcd_dev.buffer, 0, BUFFER_SIZE);

    /* Allocate device number */
    ret = alloc_chrdev_region(&pcd_dev.dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "pcd: Failed to allocate device number\n");
        return ret;
    }
    printk(KERN_INFO "pcd: Device number: %d:%d\n",
           MAJOR(pcd_dev.dev_num), MINOR(pcd_dev.dev_num));

    /* Initialize cdev */
    cdev_init(&pcd_dev.cdev, &pcd_fops);
    pcd_dev.cdev.owner = THIS_MODULE;

    /* Add cdev */
    ret = cdev_add(&pcd_dev.cdev, pcd_dev.dev_num, 1);
    if (ret < 0) {
        printk(KERN_ERR "pcd: Failed to add cdev\n");
        goto fail_cdev_add;
    }

    /* Create class */
    pcd_dev.class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(pcd_dev.class)) {
        printk(KERN_ERR "pcd: Failed to create class\n");
        ret = PTR_ERR(pcd_dev.class);
        goto fail_class_create;
    }

    /* Create device */
    pcd_dev.device = device_create(pcd_dev.class, NULL, pcd_dev.dev_num,
                                  NULL, DEVICE_NAME);
    if (IS_ERR(pcd_dev.device)) {
        printk(KERN_ERR "pcd: Failed to create device\n");
        ret = PTR_ERR(pcd_dev.device);
        goto fail_device_create;
    }

    printk(KERN_INFO "pcd: Device created: /dev/%s\n", DEVICE_NAME);
    return 0;

fail_device_create:
    class_destroy(pcd_dev.class);
fail_class_create:
    cdev_del(&pcd_dev.cdev);
fail_cdev_add:
    unregister_chrdev_region(pcd_dev.dev_num, 1);
    return ret;
}

/*
 * Module cleanup
 */
static void __exit pcd_exit(void)
{
    printk(KERN_INFO "pcd: Cleaning up module\n");

    device_destroy(pcd_dev.class, pcd_dev.dev_num);
    class_destroy(pcd_dev.class);
    cdev_del(&pcd_dev.cdev);
    unregister_chrdev_region(pcd_dev.dev_num, 1);

    printk(KERN_INFO "pcd: Module cleanup complete\n");
}

module_init(pcd_init);
module_exit(pcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Expert");
MODULE_DESCRIPTION("Pseudo Character Device Driver");
MODULE_VERSION("1.0");
```

### Testing the Driver

**User space test program:**

```c
/* test_pcd.c */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define DEVICE "/dev/pcd"

int main(void)
{
    int fd;
    char write_buf[] = "Hello from user space!";
    char read_buf[100];
    ssize_t ret;

    printf("=== Testing PCD Driver ===\n\n");

    /* Open device */
    printf("Opening device %s...\n", DEVICE);
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return EXIT_FAILURE;
    }
    printf("Device opened successfully (fd=%d)\n\n", fd);

    /* Write data */
    printf("Writing: \"%s\"\n", write_buf);
    ret = write(fd, write_buf, strlen(write_buf));
    if (ret < 0) {
        perror("Write failed");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("Wrote %zd bytes\n\n", ret);

    /* Reset file position to beginning */
    lseek(fd, 0, SEEK_SET);

    /* Read data back */
    printf("Reading data back...\n");
    memset(read_buf, 0, sizeof(read_buf));
    ret = read(fd, read_buf, sizeof(read_buf));
    if (ret < 0) {
        perror("Read failed");
        close(fd);
        return EXIT_FAILURE;
    }
    printf("Read %zd bytes: \"%s\"\n\n", ret, read_buf);

    /* Close device */
    printf("Closing device...\n");
    close(fd);
    printf("Device closed\n");

    return EXIT_SUCCESS;
}
```

**Compile and test:**

```bash
# Build driver
make

# Load driver
sudo insmod pcd.ko

# Check device created
ls -l /dev/pcd
# crw------- 1 root root 240, 0 Dec 13 12:00 /dev/pcd

# Compile test program
gcc -o test_pcd test_pcd.c

# Run test (need root for device access)
sudo ./test_pcd
```

**Expected output:**

```
=== Testing PCD Driver ===

Opening device /dev/pcd...
Device opened successfully (fd=3)

Writing: "Hello from user space!"
Wrote 22 bytes

Reading data back...
Read 22 bytes: "Hello from user space!"

Closing device...
Device closed
```

**Kernel log (dmesg):**

```
pcd: Initializing module
pcd: Device number: 240:0
pcd: Device created: /dev/pcd
pcd: open() called
pcd: Opened read-write
pcd: write() called: count=22, f_pos=0
pcd: Wrote 22 bytes, new f_pos=22
pcd: read() called: count=100, f_pos=0
pcd: Read 22 bytes, new f_pos=22
pcd: release() called
```

---

## Summary

This chapter covered basic file operations:

**Key Topics:**

- ✅ file_operations structure and system call mapping
- ✅ inode vs file structures
- ✅ User-kernel data exchange with copy_to_user/copy_from_user
- ✅ __user annotation importance
- ✅ open() method implementation
- ✅ release() method implementation
- ✅ Per-device data pattern with container_of()
- ✅ Complete working driver example