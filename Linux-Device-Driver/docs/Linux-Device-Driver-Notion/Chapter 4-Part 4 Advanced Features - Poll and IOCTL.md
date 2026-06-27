# Part 4. Advanced Features - Poll and IOCTL

This final part covers advanced character device operations: poll/select for efficient I/O multiplexing and IOCTL for custom device commands.

---

## 4.2.10 The poll() Method

### Introduction to poll/select

**The Problem:**

```c
/* Inefficient busy-wait loop */
while (1) {
    ret = read(fd, buf, size);
    if (ret > 0)
        break;  /* Got data! */

    usleep(1000);  /* Wait and try again */
}
/* CPU wasted in loop! */
```

**The Solution: poll/select**

```c
/* Efficient event notification */
struct pollfd fds[1];
fds[0].fd = fd;
fds[0].events = POLLIN;  /* Want to read */

/* Block until data available */
poll(fds, 1, -1);  /* CPU free while waiting */

/* Now read will succeed immediately */
read(fd, buf, size);
```

### How poll() Works

```
User Space                           Kernel Space
──────────────────────────────────   ─────────────────────────────
1. Call poll()                    →
                                     2. Driver's poll() called
                                        - Add to wait queue
                                        - Check current state
                                        - Return mask

                                     3. If not ready:
                                        - Process sleeps
3. Process blocks                 ←

                                     4. When ready:
                                        - IRQ/event occurs
                                        - wake_up() called
4. poll() returns                 ←

5. Check which fd ready
6. Perform I/O
```

### poll() Function Prototype

```c
unsigned int (*poll)(struct file *file, struct poll_table_struct *wait);

/* Parameters:
 * file - File structure
 * wait - poll_table for registering wait queues
 *
 * Returns: Mask of ready conditions (POLLIN, POLLOUT, etc.)
 */
```

### Event Masks

```c
/* Input events */
#define POLLIN      0x0001   /* Data can be read */
#define POLLRDNORM  0x0040   /* Normal data readable */

/* Output events */
#define POLLOUT     0x0004   /* Data can be written */
#define POLLWRNORM  0x0100   /* Normal data writable */

/* Error events */
#define POLLERR     0x0008   /* Error condition */
#define POLLHUP     0x0010   /* Hang up */
#define POLLNVAL    0x0020   /* Invalid request */
```

### poll_wait() Function

```c
void poll_wait(struct file *filp, wait_queue_head_t *queue,
              poll_table *wait);

/* Registers a wait queue with poll_table
 * Does NOT sleep - just registers interest
 */
```

### Basic poll() Implementation

```c
#include <linux/poll.h>

struct my_device {
    struct cdev cdev;
    wait_queue_head_t read_queue;
    wait_queue_head_t write_queue;
    char *buffer;
    size_t data_available;
    size_t buffer_size;
};

static unsigned int my_poll(struct file *file, poll_table *wait)
{
    struct my_device *dev = file->private_data;
    unsigned int mask = 0;

    /* Register wait queues - doesn't block! */
    poll_wait(file, &dev->read_queue, wait);
    poll_wait(file, &dev->write_queue, wait);

    /* Check current device state */

    /* Can read if data available */
    if (dev->data_available > 0)
        mask |= POLLIN | POLLRDNORM;

    /* Can write if buffer not full */
    if (dev->data_available < dev->buffer_size)
        mask |= POLLOUT | POLLWRNORM;

    return mask;
}
```

### Complete poll() Example with Circular Buffer

```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "polldev"
#define BUFFER_SIZE 256

/* Circular buffer device */
struct poll_device {
    struct cdev cdev;
    dev_t dev_num;
    struct class *class;
    struct device *device;

    /* Circular buffer */
    char buffer[BUFFER_SIZE];
    unsigned int read_pos;
    unsigned int write_pos;
    unsigned int count;  /* Number of bytes in buffer */

    /* Wait queues */
    wait_queue_head_t read_queue;
    wait_queue_head_t write_queue;

    spinlock_t lock;
};

static struct poll_device poll_dev;

/* Helper: Check if buffer has data */
static inline bool has_data(struct poll_device *dev)
{
    return dev->count > 0;
}

/* Helper: Check if buffer has space */
static inline bool has_space(struct poll_device *dev)
{
    return dev->count < BUFFER_SIZE;
}

static int polldev_open(struct inode *inode, struct file *file)
{
    file->private_data = &poll_dev;
    pr_info("polldev: Device opened\n");
    return 0;
}

static int polldev_release(struct inode *inode, struct file *file)
{
    pr_info("polldev: Device closed\n");
    return 0;
}

static ssize_t polldev_read(struct file *file, char __user *buf,
                           size_t count, loff_t *f_pos)
{
    struct poll_device *dev = file->private_data;
    size_t bytes_read = 0;
    unsigned long flags;
    int ret;

    /* Non-blocking: return immediately if no data */
    if (file->f_flags & O_NONBLOCK) {
        spin_lock_irqsave(&dev->lock, flags);
        if (!has_data(dev)) {
            spin_unlock_irqrestore(&dev->lock, flags);
            return -EAGAIN;
        }
        spin_unlock_irqrestore(&dev->lock, flags);
    } else {
        /* Blocking: wait for data */
        ret = wait_event_interruptible(dev->read_queue, has_data(dev));
        if (ret)
            return -ERESTARTSYS;
    }

    spin_lock_irqsave(&dev->lock, flags);

    /* Read from circular buffer */
    while (count > 0 && dev->count > 0) {
        char ch = dev->buffer[dev->read_pos];

        spin_unlock_irqrestore(&dev->lock, flags);

        if (put_user(ch, buf++)) {
            if (bytes_read == 0)
                bytes_read = -EFAULT;
            goto out_wake;
        }

        spin_lock_irqsave(&dev->lock, flags);

        dev->read_pos = (dev->read_pos + 1) % BUFFER_SIZE;
        dev->count--;
        bytes_read++;
        count--;
    }

    spin_unlock_irqrestore(&dev->lock, flags);

out_wake:
    /* Wake up writers - space now available */
    wake_up_interruptible(&dev->write_queue);

    pr_debug("polldev: Read %zu bytes\n", bytes_read);
    return bytes_read;
}

static ssize_t polldev_write(struct file *file, const char __user *buf,
                            size_t count, loff_t *f_pos)
{
    struct poll_device *dev = file->private_data;
    size_t bytes_written = 0;
    unsigned long flags;
    int ret;

    /* Non-blocking: return immediately if no space */
    if (file->f_flags & O_NONBLOCK) {
        spin_lock_irqsave(&dev->lock, flags);
        if (!has_space(dev)) {
            spin_unlock_irqrestore(&dev->lock, flags);
            return -EAGAIN;
        }
        spin_unlock_irqrestore(&dev->lock, flags);
    } else {
        /* Blocking: wait for space */
        ret = wait_event_interruptible(dev->write_queue, has_space(dev));
        if (ret)
            return -ERESTARTSYS;
    }

    spin_lock_irqsave(&dev->lock, flags);

    /* Write to circular buffer */
    while (count > 0 && dev->count < BUFFER_SIZE) {
        char ch;

        spin_unlock_irqrestore(&dev->lock, flags);

        if (get_user(ch, buf++)) {
            if (bytes_written == 0)
                bytes_written = -EFAULT;
            goto out_wake;
        }

        spin_lock_irqsave(&dev->lock, flags);

        dev->buffer[dev->write_pos] = ch;
        dev->write_pos = (dev->write_pos + 1) % BUFFER_SIZE;
        dev->count++;
        bytes_written++;
        count--;
    }

    spin_unlock_irqrestore(&dev->lock, flags);

out_wake:
    /* Wake up readers - data now available */
    wake_up_interruptible(&dev->read_queue);

    pr_debug("polldev: Wrote %zu bytes\n", bytes_written);
    return bytes_written;
}

/* poll() implementation */
static unsigned int polldev_poll(struct file *file, poll_table *wait)
{
    struct poll_device *dev = file->private_data;
    unsigned int mask = 0;
    unsigned long flags;

    pr_debug("polldev: poll() called\n");

    /* Register wait queues */
    poll_wait(file, &dev->read_queue, wait);
    poll_wait(file, &dev->write_queue, wait);

    /* Check current state */
    spin_lock_irqsave(&dev->lock, flags);

    if (has_data(dev))
        mask |= POLLIN | POLLRDNORM;

    if (has_space(dev))
        mask |= POLLOUT | POLLWRNORM;

    spin_unlock_irqrestore(&dev->lock, flags);

    pr_debug("polldev: poll() returns mask=0x%x\n", mask);

    return mask;
}

static struct file_operations polldev_fops = {
    .owner   = THIS_MODULE,
    .open    = polldev_open,
    .release = polldev_release,
    .read    = polldev_read,
    .write   = polldev_write,
    .poll    = polldev_poll,
};

static int __init polldev_init(void)
{
    int ret;

    /* Initialize */
    spin_lock_init(&poll_dev.lock);
    init_waitqueue_head(&poll_dev.read_queue);
    init_waitqueue_head(&poll_dev.write_queue);
    poll_dev.read_pos = 0;
    poll_dev.write_pos = 0;
    poll_dev.count = 0;

    /* Allocate device number */
    ret = alloc_chrdev_region(&poll_dev.dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0)
        return ret;

    /* Initialize and add cdev */
    cdev_init(&poll_dev.cdev, &polldev_fops);
    poll_dev.cdev.owner = THIS_MODULE;

    ret = cdev_add(&poll_dev.cdev, poll_dev.dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(poll_dev.dev_num, 1);
        return ret;
    }

    /* Create class and device */
    poll_dev.class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(poll_dev.class)) {
        cdev_del(&poll_dev.cdev);
        unregister_chrdev_region(poll_dev.dev_num, 1);
        return PTR_ERR(poll_dev.class);
    }

    poll_dev.device = device_create(poll_dev.class, NULL,
                                   poll_dev.dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(poll_dev.device)) {
        class_destroy(poll_dev.class);
        cdev_del(&poll_dev.cdev);
        unregister_chrdev_region(poll_dev.dev_num, 1);
        return PTR_ERR(poll_dev.device);
    }

    pr_info("polldev: Device created: /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void __exit polldev_exit(void)
{
    device_destroy(poll_dev.class, poll_dev.dev_num);
    class_destroy(poll_dev.class);
    cdev_del(&poll_dev.cdev);
    unregister_chrdev_region(poll_dev.dev_num, 1);
    pr_info("polldev: Module unloaded\n");
}

module_init(polldev_init);
module_exit(polldev_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Character device with poll support");
```

### User Space poll() Test

```c
/* test_poll.c */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <errno.h>

#define DEVICE "/dev/polldev"

int main(void)
{
    int fd;
    struct pollfd fds[1];
    int ret;
    char buf[100];

    printf("=== Poll Test ===\n\n");

    /* Open device */
    fd = open(DEVICE, O_RDWR);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    /* Setup poll structure */
    fds[0].fd = fd;
    fds[0].events = POLLIN | POLLOUT;

    printf("Waiting for events (timeout: 5 seconds)...\n");

    /* Wait for events */
    ret = poll(fds, 1, 5000);  /* 5 second timeout */

    if (ret == -1) {
        perror("poll");
        close(fd);
        return EXIT_FAILURE;
    }

    if (ret == 0) {
        printf("Timeout - no events\n");
    } else {
        printf("Events occurred! revents=0x%x\n", fds[0].revents);

        if (fds[0].revents & POLLOUT) {
            printf("  - Device is writable\n");
            write(fd, "Test data", 9);
            printf("  - Wrote test data\n");
        }

        if (fds[0].revents & POLLIN) {
            printf("  - Device is readable\n");
            ret = read(fd, buf, sizeof(buf));
            printf("  - Read %d bytes\n", ret);
        }
    }

    close(fd);
    return EXIT_SUCCESS;
}
```

---

## 4.2.11 The IOCTL Interface

### Introduction to IOCTL

**What is IOCTL?**

IOCTL (Input/Output Control) extends file operations with **custom commands** specific to your device.

```
Standard operations:    IOCTL operations:
├─ open()              ├─ RESET_DEVICE
├─ read()              ├─ GET_STATUS
├─ write()             ├─ SET_CONFIG
├─ close()             ├─ CALIBRATE
└─ llseek()            └─ Your custom commands
```

### IOCTL Function Prototype

```c
long (*unlocked_ioctl)(struct file *file, unsigned int cmd,
                      unsigned long arg);

/* Parameters:
 * file - File structure
 * cmd  - IOCTL command number
 * arg  - Command argument (can be value or pointer)
 *
 * Returns: 0 or positive on success, negative error code on failure
 */
```

### Creating IOCTL Commands

**Helper macros:**

```c
/* No data transfer */
_IO(type, nr)

/* Read data (copy_to_user) */
_IOR(type, nr, datatype)

/* Write data (copy_from_user) */
_IOW(type, nr, datatype)

/* Read and write data */
_IORW(type, nr, datatype)

/* Parameters:
 * type     - Magic number (8 bits, 0-255)
 * nr       - Command number (8 bits, 0-255)
 * datatype - Data type (for size calculation)
 */
```

**Example IOCTL header:**

```c
/* my_ioctl.h */
#ifndef MY_IOCTL_H
#define MY_IOCTL_H

#include <linux/ioctl.h>

/* Magic number - choose unique value */
#define MY_IOC_MAGIC 'k'

/* Define commands */
#define MY_IOCTL_RESET      _IO(MY_IOC_MAGIC, 0)
#define MY_IOCTL_GET_SIZE   _IOR(MY_IOC_MAGIC, 1, int)
#define MY_IOCTL_SET_SIZE   _IOW(MY_IOC_MAGIC, 2, int)
#define MY_IOCTL_GET_INFO   _IOR(MY_IOC_MAGIC, 3, struct device_info)
#define MY_IOCTL_SET_CONFIG _IOW(MY_IOC_MAGIC, 4, struct device_config)
#define MY_IOCTL_EXCHANGE   _IORW(MY_IOC_MAGIC, 5, struct exchange_data)

/* Maximum command number */
#define MY_IOC_MAXNR 5

/* Data structures */
struct device_info {
    int version;
    char name[32];
    unsigned int flags;
};

struct device_config {
    int mode;
    int speed;
};

struct exchange_data {
    int input;
    int output;
};

#endif /* MY_IOCTL_H */
```

### IOCTL Implementation

```c
#include "my_ioctl.h"

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    struct my_device *dev = file->private_data;
    int ret = 0;
    int size;
    struct device_info info;
    struct device_config config;

    /* Check magic number */
    if (_IOC_TYPE(cmd) != MY_IOC_MAGIC) {
        pr_err("my_device: Invalid IOCTL magic\n");
        return -ENOTTY;
    }

    /* Check command number */
    if (_IOC_NR(cmd) > MY_IOC_MAXNR) {
        pr_err("my_device: Invalid IOCTL command\n");
        return -ENOTTY;
    }

    /* Verify user space pointer if needed */
    if (_IOC_DIR(cmd) & _IOC_READ) {
        ret = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
        if (ret)
            return -EFAULT;
    }

    if (_IOC_DIR(cmd) & _IOC_WRITE) {
        ret = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
        if (ret)
            return -EFAULT;
    }

    /* Process command */
    switch (cmd) {
    case MY_IOCTL_RESET:
        pr_info("my_device: RESET command\n");
        /* Reset device */
        memset(dev->buffer, 0, dev->buffer_size);
        break;

    case MY_IOCTL_GET_SIZE:
        pr_info("my_device: GET_SIZE command\n");
        size = dev->buffer_size;
        if (copy_to_user((int __user *)arg, &size, sizeof(size)))
            return -EFAULT;
        break;

    case MY_IOCTL_SET_SIZE:
        pr_info("my_device: SET_SIZE command\n");
        if (copy_from_user(&size, (int __user *)arg, sizeof(size)))
            return -EFAULT;

        if (size < 0 || size > MAX_SIZE)
            return -EINVAL;

        dev->buffer_size = size;
        break;

    case MY_IOCTL_GET_INFO:
        pr_info("my_device: GET_INFO command\n");
        info.version = DRIVER_VERSION;
        strncpy(info.name, DEVICE_NAME, sizeof(info.name));
        info.flags = dev->flags;

        if (copy_to_user((struct device_info __user *)arg,
                        &info, sizeof(info)))
            return -EFAULT;
        break;

    case MY_IOCTL_SET_CONFIG:
        pr_info("my_device: SET_CONFIG command\n");
        if (copy_from_user(&config, (struct device_config __user *)arg,
                          sizeof(config)))
            return -EFAULT;

        /* Validate and apply configuration */
        if (config.mode < 0 || config.mode > 3)
            return -EINVAL;

        dev->mode = config.mode;
        dev->speed = config.speed;
        break;

    default:
        pr_err("my_device: Unknown IOCTL command %u\n", cmd);
        return -ENOTTY;
    }

    return ret;
}
```

### Complete IOCTL Example

```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "ioctldev"
#define BUFFER_SIZE 1024

/* IOCTL definitions */
#define IOCTL_MAGIC 'N'
#define IOCTL_RESET       _IO(IOCTL_MAGIC, 0)
#define IOCTL_GET_SIZE    _IOR(IOCTL_MAGIC, 1, int)
#define IOCTL_CLEAR       _IO(IOCTL_MAGIC, 2)
#define IOCTL_FILL        _IOW(IOCTL_MAGIC, 3, char)
#define IOCTL_GET_STATS   _IOR(IOCTL_MAGIC, 4, struct stats)
#define IOCTL_MAXNR       4

struct stats {
    unsigned long reads;
    unsigned long writes;
    unsigned long ioctls;
};

/* Device structure */
struct ioctl_device {
    struct cdev cdev;
    dev_t dev_num;
    struct class *class;
    struct device *device;

    char buffer[BUFFER_SIZE];
    struct stats stats;
    struct mutex lock;
};

static struct ioctl_device ioctl_dev;

static int ioctldev_open(struct inode *inode, struct file *file)
{
    file->private_data = &ioctl_dev;
    return 0;
}

static int ioctldev_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t ioctldev_read(struct file *file, char __user *buf,
                             size_t count, loff_t *f_pos)
{
    struct ioctl_device *dev = file->private_data;
    size_t bytes_to_read;

    if (*f_pos >= BUFFER_SIZE)
        return 0;

    bytes_to_read = min(count, (size_t)(BUFFER_SIZE - *f_pos));

    if (copy_to_user(buf, dev->buffer + *f_pos, bytes_to_read))
        return -EFAULT;

    *f_pos += bytes_to_read;
    dev->stats.reads += bytes_to_read;

    return bytes_to_read;
}

static ssize_t ioctldev_write(struct file *file, const char __user *buf,
                              size_t count, loff_t *f_pos)
{
    struct ioctl_device *dev = file->private_data;
    size_t bytes_to_write;

    if (*f_pos >= BUFFER_SIZE)
        return -ENOSPC;

    bytes_to_write = min(count, (size_t)(BUFFER_SIZE - *f_pos));

    if (copy_from_user(dev->buffer + *f_pos, buf, bytes_to_write))
        return -EFAULT;

    *f_pos += bytes_to_write;
    dev->stats.writes += bytes_to_write;

    return bytes_to_write;
}

static long ioctldev_ioctl(struct file *file, unsigned int cmd,
                          unsigned long arg)
{
    struct ioctl_device *dev = file->private_data;
    int size;
    char fill_char;
    struct stats stats_copy;

    pr_info("ioctldev: IOCTL called, cmd=0x%x\n", cmd);

    /* Verify magic number */
    if (_IOC_TYPE(cmd) != IOCTL_MAGIC)
        return -ENOTTY;

    if (_IOC_NR(cmd) > IOCTL_MAXNR)
        return -ENOTTY;

    /* Count IOCTL */
    dev->stats.ioctls++;

    /* Process command */
    switch (cmd) {
    case IOCTL_RESET:
        pr_info("ioctldev: RESET\n");
        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;
        memset(dev->buffer, 0, BUFFER_SIZE);
        memset(&dev->stats, 0, sizeof(dev->stats));
        mutex_unlock(&dev->lock);
        break;

    case IOCTL_GET_SIZE:
        pr_info("ioctldev: GET_SIZE\n");
        size = BUFFER_SIZE;
        if (copy_to_user((int __user *)arg, &size, sizeof(size)))
            return -EFAULT;
        break;

    case IOCTL_CLEAR:
        pr_info("ioctldev: CLEAR\n");
        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;
        memset(dev->buffer, 0, BUFFER_SIZE);
        mutex_unlock(&dev->lock);
        break;

    case IOCTL_FILL:
        pr_info("ioctldev: FILL\n");
        if (copy_from_user(&fill_char, (char __user *)arg, sizeof(fill_char)))
            return -EFAULT;

        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;
        memset(dev->buffer, fill_char, BUFFER_SIZE);
        mutex_unlock(&dev->lock);
        break;

    case IOCTL_GET_STATS:
        pr_info("ioctldev: GET_STATS\n");
        if (mutex_lock_interruptible(&dev->lock))
            return -ERESTARTSYS;
        memcpy(&stats_copy, &dev->stats, sizeof(stats_copy));
        mutex_unlock(&dev->lock);

        if (copy_to_user((struct stats __user *)arg,
                        &stats_copy, sizeof(stats_copy)))
            return -EFAULT;
        break;

    default:
        pr_err("ioctldev: Unknown command 0x%x\n", cmd);
        return -ENOTTY;
    }

    return 0;
}

static struct file_operations ioctldev_fops = {
    .owner          = THIS_MODULE,
    .open           = ioctldev_open,
    .release        = ioctldev_release,
    .read           = ioctldev_read,
    .write          = ioctldev_write,
    .unlocked_ioctl = ioctldev_ioctl,
};

/* Module init/exit omitted for brevity - similar to previous examples */
```

### User Space IOCTL Test

```c
/* test_ioctl.c */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

/* Must match driver definitions */
#define IOCTL_MAGIC 'N'
#define IOCTL_RESET       _IO(IOCTL_MAGIC, 0)
#define IOCTL_GET_SIZE    _IOR(IOCTL_MAGIC, 1, int)
#define IOCTL_CLEAR       _IO(IOCTL_MAGIC, 2)
#define IOCTL_FILL        _IOW(IOCTL_MAGIC, 3, char)
#define IOCTL_GET_STATS   _IOR(IOCTL_MAGIC, 4, struct stats)

struct stats {
    unsigned long reads;
    unsigned long writes;
    unsigned long ioctls;
};

int main(void)
{
    int fd;
    int size;
    char fill = 'A';
    struct stats stats;
    char buf[100];

    printf("=== IOCTL Test ===\n\n");

    fd = open("/dev/ioctldev", O_RDWR);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    /* Test 1: Get size */
    printf("Test 1: Get buffer size\n");
    if (ioctl(fd, IOCTL_GET_SIZE, &size) < 0) {
        perror("ioctl GET_SIZE");
    } else {
        printf("  Buffer size: %d bytes\n\n", size);
    }

    /* Test 2: Fill buffer */
    printf("Test 2: Fill buffer with 'A'\n");
    if (ioctl(fd, IOCTL_FILL, &fill) < 0) {
        perror("ioctl FILL");
    } else {
        printf("  Buffer filled\n\n");
    }

    /* Test 3: Read some data */
    printf("Test 3: Read 20 bytes\n");
    memset(buf, 0, sizeof(buf));
    read(fd, buf, 20);
    printf("  Read: \"%.*s\"\n\n", 20, buf);

    /* Test 4: Get statistics */
    printf("Test 4: Get statistics\n");
    if (ioctl(fd, IOCTL_GET_STATS, &stats) < 0) {
        perror("ioctl GET_STATS");
    } else {
        printf("  Reads:  %lu bytes\n", stats.reads);
        printf("  Writes: %lu bytes\n", stats.writes);
        printf("  IOCTLs: %lu calls\n\n", stats.ioctls);
    }

    /* Test 5: Clear buffer */
    printf("Test 5: Clear buffer\n");
    if (ioctl(fd, IOCTL_CLEAR) < 0) {
        perror("ioctl CLEAR");
    } else {
        printf("  Buffer cleared\n\n");
    }

    /* Test 6: Reset everything */
    printf("Test 6: Reset device\n");
    if (ioctl(fd, IOCTL_RESET) < 0) {
        perror("ioctl RESET");
    } else {
        printf("  Device reset\n\n");
    }

    close(fd);
    printf("=== Test Complete ===\n");

    return EXIT_SUCCESS;
}
```

---

## 4.3 Advanced Character Driver Features

### 4.3.1 Non-Blocking I/O Summary

**Implementing non-blocking mode:**

```c
static ssize_t my_read(struct file *file, char __user *buf,
                      size_t count, loff_t *f_pos)
{
    struct my_device *dev = file->private_data;

    /* Check O_NONBLOCK flag */
    if (file->f_flags & O_NONBLOCK) {
        /* Non-blocking mode */
        if (!data_available(dev))
            return -EAGAIN;  /* Try again later */
    } else {
        /* Blocking mode */
        wait_event_interruptible(dev->queue, data_available(dev));
    }

    /* Perform read */
    return do_read(dev, buf, count);
}
```

### 4.3.2 Best Practices Summary

**✅ DO:**

1. **Always validate user pointers**
    
    ```c
    if (copy_to_user(ubuf, kbuf, size))
        return -EFAULT;
    ```
    
2. **Check file flags**
    
    ```c
    if (!(file->f_mode & FMODE_READ))
        return -EBADF;
    ```
    
3. **Use proper locking**
    
    ```c
    mutex_lock(&dev->lock);
    /* Access shared data */
    mutex_unlock(&dev->lock);
    ```
    
4. **Handle interrupts**
    
    ```c
    if (mutex_lock_interruptible(&dev->lock))
        return -ERESTARTSYS;
    ```
    

**❌ DON'T:**

1. **Don't dereference user pointers**
    
    ```c
    char val = *user_ptr;  /* WRONG! */
    ```
    
2. **Don't forget to update f_pos**
    
    ```c
    *f_pos += bytes_read;  /* Always update! */
    ```
    
3. **Don't assume request size**
    
    ```c
    /* Don't assume you can read/write full count */
    return min(count, available);
    ```
    

---

## Summary

This completes Chapter 4 on Character Device Drivers!

**Topics Covered:**

**Part 1 - Registration:**

- ✅ Character device basics
- ✅ Major/minor numbers
- ✅ Device number allocation
- ✅ cdev registration
- ✅ Device class and automatic /dev nodes

**Part 2 - Basic File Operations:**

- ✅ file_operations structure
- ✅ open() and release() methods
- ✅ User-kernel data exchange
- ✅ Per-device data management

**Part 3 - Read/Write/Seek:**

- ✅ read() implementation
- ✅ write() implementation
- ✅ llseek() implementation
- ✅ Complete EEPROM example

**Part 4 - Advanced Features:**

- ✅ poll() for I/O multiplexing
- ✅ IOCTL custom commands
- ✅ Non-blocking I/O
- ✅ Best practices

**You now know how to:**

- Create and register character devices
- Implement all major file operations
- Handle user-kernel data exchange safely
- Support poll/select efficiently
- Extend functionality with IOCTL
- Follow kernel coding best practices