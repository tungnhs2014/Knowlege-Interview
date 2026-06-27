# Part 7. Wait Queues and Sleep/Wake Mechanisms

## 3.4.2 Blocking vs Non-Blocking I/O

### Understanding I/O Modes

**Blocking I/O (default):**

```
User calls read():
├─ If data available → Return immediately
└─ If no data → Sleep until data arrives
                (Process blocked, CPU available for others)
```

**Non-Blocking I/O:**

```
User calls read() with O_NONBLOCK:
├─ If data available → Return data
└─ If no data → Return -EAGAIN immediately
                (Process continues, can do other work)
```

**User space usage:**

```c
/* Blocking mode (default) */
int fd = open("/dev/mydevice", O_RDONLY);
read(fd, buf, size);  /* Blocks until data available */

/* Non-blocking mode */
int fd = open("/dev/mydevice", O_RDONLY | O_NONBLOCK);
ssize_t n = read(fd, buf, size);
if (n == -1 && errno == EAGAIN) {
    /* No data available, try again later */
}
```

### Implementing Non-Blocking I/O in Driver

**Check for O_NONBLOCK flag:**

```c
static ssize_t my_read(struct file *filp, char __user *buf,
                      size_t count, loff_t *f_pos)
{
    struct my_device *dev = filp->private_data;
    int ret;

    /* Check if non-blocking mode */
    if (filp->f_flags & O_NONBLOCK) {
        /* Non-blocking - return immediately if no data */
        if (!data_available(dev))
            return -EAGAIN;  /* Try again later */
    } else {
        /* Blocking - wait for data */
        ret = wait_event_interruptible(dev->read_queue,
                                       data_available(dev));
        if (ret)
            return -ERESTARTSYS;
    }

    /* Read data... */
    return actual_read(dev, buf, count);
}
```

### Complete Example: Non-Blocking Character Device

```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "nonblock_char"
#define BUFFER_SIZE 256

struct nonblock_dev {
    struct cdev cdev;
    dev_t devt;
    struct class *class;

    wait_queue_head_t read_queue;
    char buffer[BUFFER_SIZE];
    int data_len;
    spinlock_t lock;
};

static struct nonblock_dev *nbdev;

static ssize_t nonblock_read(struct file *filp, char __user *buf,
                             size_t count, loff_t *f_pos)
{
    struct nonblock_dev *dev = filp->private_data;
    int bytes_to_read;
    unsigned long flags;
    int ret;

    /* Check for non-blocking mode */
    if (filp->f_flags & O_NONBLOCK) {
        /* Non-blocking - check data immediately */
        spin_lock_irqsave(&dev->lock, flags);
        if (dev->data_len == 0) {
            spin_unlock_irqrestore(&dev->lock, flags);
            return -EAGAIN;  /* No data, return immediately */
        }
        spin_unlock_irqrestore(&dev->lock, flags);
    } else {
        /* Blocking - wait for data */
        ret = wait_event_interruptible(dev->read_queue, dev->data_len > 0);
        if (ret)
            return -ERESTARTSYS;
    }

    /* Read data */
    spin_lock_irqsave(&dev->lock, flags);

    bytes_to_read = min((int)count, dev->data_len);

    spin_unlock_irqrestore(&dev->lock, flags);

    if (copy_to_user(buf, dev->buffer, bytes_to_read))
        return -EFAULT;

    spin_lock_irqsave(&dev->lock, flags);
    dev->data_len -= bytes_to_read;
    memmove(dev->buffer, dev->buffer + bytes_to_read, dev->data_len);
    spin_unlock_irqrestore(&dev->lock, flags);

    return bytes_to_read;
}

static ssize_t nonblock_write(struct file *filp, const char __user *buf,
                              size_t count, loff_t *f_pos)
{
    struct nonblock_dev *dev = filp->private_data;
    int bytes_to_write;
    unsigned long flags;

    spin_lock_irqsave(&dev->lock, flags);

    /* Check available space */
    if (dev->data_len >= BUFFER_SIZE) {
        spin_unlock_irqrestore(&dev->lock, flags);

        if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;  /* Buffer full, non-blocking */
        else
            return -ENOSPC;  /* Buffer full, blocking */
    }

    bytes_to_write = min((int)count, BUFFER_SIZE - dev->data_len);

    spin_unlock_irqrestore(&dev->lock, flags);

    if (copy_from_user(dev->buffer + dev->data_len, buf, bytes_to_write))
        return -EFAULT;

    spin_lock_irqsave(&dev->lock, flags);
    dev->data_len += bytes_to_write;
    spin_unlock_irqrestore(&dev->lock, flags);

    /* Wake up readers */
    wake_up_interruptible(&dev->read_queue);

    return bytes_to_write;
}

static int nonblock_open(struct inode *inode, struct file *filp)
{
    filp->private_data = nbdev;
    printk("Device opened in %s mode\n",
           (filp->f_flags & O_NONBLOCK) ? "non-blocking" : "blocking");
    return 0;
}

static const struct file_operations nonblock_fops = {
    .owner = THIS_MODULE,
    .open = nonblock_open,
    .read = nonblock_read,
    .write = nonblock_write,
};

/* Init and exit functions omitted for brevity - similar to previous example */

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Character device supporting non-blocking I/O");
```

**User space test - non-blocking read:**

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main() {
    int fd;
    char buf[100];
    ssize_t n;
    int retry = 0;

    /* Open in non-blocking mode */
    fd = open("/dev/nonblock_char", O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    while (retry < 5) {
        n = read(fd, buf, sizeof(buf));

        if (n > 0) {
            printf("Read %zd bytes: %.*s\n", n, (int)n, buf);
            break;
        } else if (n == -1 && errno == EAGAIN) {
            printf("No data yet, retrying...\n");
            sleep(1);
            retry++;
        } else {
            perror("read");
            break;
        }
    }

    close(fd);
    return 0;
}
```

---

## 3.4.3 Poll/Select Mechanisms

### Introduction to Poll/Select

**Problem with blocking/non-blocking alone:**

```
Blocking:     wait forever (inefficient)
Non-blocking: busy loop checking (wastes CPU)

Solution:     poll/select (efficient waiting on multiple files)
```

**poll() system call allows:**

- Wait for I/O on multiple file descriptors
- Timeout support
- Efficient - no busy looping
- Event notification (read ready, write ready, error)

**How poll works:**

```
User Space:                         Kernel Space:
┌─────────────────┐                ┌──────────────────┐
│ User calls      │                │ Driver's poll    │
│ poll(fds, ...)  │───────────────→│ method called    │
└─────────────────┘                └──────────────────┘
        │                                   │
        │                            poll_wait() adds
        │                            process to wait queue
        │                                   │
        │ Process sleeps                    │
        │ until event                       │
        │                                   │
        ↓                                   ↓
┌─────────────────┐                ┌──────────────────┐
│ Event occurs!   │←───────────────│ wake_up()        │
│ poll() returns  │                │ called           │
└─────────────────┘                └──────────────────┘
```

### Poll Method Implementation

**Function signature:**

```c
unsigned int (*poll)(struct file *filp, struct poll_table_struct *wait);
```

**Required header:**

```c
#include <linux/poll.h>
```

**Core function:**

```c
void poll_wait(struct file *filp, wait_queue_head_t *queue,
               poll_table *wait);
```

**Return value - event mask:**

```c
/* Readable events */
#define POLLIN      0x0001  /* Data can be read */
#define POLLRDNORM  0x0040  /* Normal data readable */
#define POLLRDBAND  0x0080  /* Priority data readable */

/* Writable events */
#define POLLOUT     0x0004  /* Data can be written */
#define POLLWRNORM  0x0100  /* Normal data writable */
#define POLLWRBAND  0x0200  /* Priority data writable */

/* Error events */
#define POLLERR     0x0008  /* Error condition */
#define POLLHUP     0x0010  /* Hang up */
#define POLLNVAL    0x0020  /* Invalid request */
```

### Basic Poll Implementation

```c
static unsigned int my_poll(struct file *filp, poll_table *wait)
{
    struct my_device *dev = filp->private_data;
    unsigned int mask = 0;

    /* Register wait queues */
    poll_wait(filp, &dev->read_queue, wait);
    poll_wait(filp, &dev->write_queue, wait);

    /* Check conditions and set appropriate flags */
    if (data_available_for_read(dev))
        mask |= POLLIN | POLLRDNORM;  /* Readable */

    if (space_available_for_write(dev))
        mask |= POLLOUT | POLLWRNORM;  /* Writable */

    return mask;
}
```

### Complete Poll Example

```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "poll_char"
#define BUFFER_SIZE 256

struct poll_dev {
    struct cdev cdev;
    dev_t devt;
    struct class *class;

    wait_queue_head_t read_queue;
    wait_queue_head_t write_queue;

    char buffer[BUFFER_SIZE];
    int data_len;
    spinlock_t lock;
};

static struct poll_dev *pdev;

static ssize_t poll_read(struct file *filp, char __user *buf,
                        size_t count, loff_t *f_pos)
{
    struct poll_dev *dev = filp->private_data;
    int bytes_read;
    unsigned long flags;
    int ret;

    /* Wait for data (if blocking mode) */
    if (!(filp->f_flags & O_NONBLOCK)) {
        ret = wait_event_interruptible(dev->read_queue, dev->data_len > 0);
        if (ret)
            return -ERESTARTSYS;
    }

    spin_lock_irqsave(&dev->lock, flags);

    if (dev->data_len == 0) {
        spin_unlock_irqrestore(&dev->lock, flags);
        return (filp->f_flags & O_NONBLOCK) ? -EAGAIN : 0;
    }

    bytes_read = min((int)count, dev->data_len);

    spin_unlock_irqrestore(&dev->lock, flags);

    if (copy_to_user(buf, dev->buffer, bytes_read))
        return -EFAULT;

    spin_lock_irqsave(&dev->lock, flags);
    dev->data_len -= bytes_read;
    memmove(dev->buffer, dev->buffer + bytes_read, dev->data_len);
    spin_unlock_irqrestore(&dev->lock, flags);

    /* Wake up writers */
    wake_up_interruptible(&dev->write_queue);

    return bytes_read;
}

static ssize_t poll_write(struct file *filp, const char __user *buf,
                         size_t count, loff_t *f_pos)
{
    struct poll_dev *dev = filp->private_data;
    int bytes_written;
    unsigned long flags;

    spin_lock_irqsave(&dev->lock, flags);

    if (dev->data_len >= BUFFER_SIZE) {
        spin_unlock_irqrestore(&dev->lock, flags);
        return (filp->f_flags & O_NONBLOCK) ? -EAGAIN : -ENOSPC;
    }

    bytes_written = min((int)count, BUFFER_SIZE - dev->data_len);

    spin_unlock_irqrestore(&dev->lock, flags);

    if (copy_from_user(dev->buffer + dev->data_len, buf, bytes_written))
        return -EFAULT;

    spin_lock_irqsave(&dev->lock, flags);
    dev->data_len += bytes_written;
    spin_unlock_irqrestore(&dev->lock, flags);

    /* Wake up readers */
    wake_up_interruptible(&dev->read_queue);

    return bytes_written;
}

/* Poll method - key function for poll/select */
static unsigned int poll_poll(struct file *filp, poll_table *wait)
{
    struct poll_dev *dev = filp->private_data;
    unsigned int mask = 0;
    unsigned long flags;

    /* Add our wait queues to poll table */
    poll_wait(filp, &dev->read_queue, wait);
    poll_wait(filp, &dev->write_queue, wait);

    /* Check current state and set event flags */
    spin_lock_irqsave(&dev->lock, flags);

    /* Can read if data available */
    if (dev->data_len > 0)
        mask |= POLLIN | POLLRDNORM;

    /* Can write if space available */
    if (dev->data_len < BUFFER_SIZE)
        mask |= POLLOUT | POLLWRNORM;

    spin_unlock_irqrestore(&dev->lock, flags);

    return mask;
}

static int poll_open(struct inode *inode, struct file *filp)
{
    filp->private_data = pdev;
    return 0;
}

static const struct file_operations poll_fops = {
    .owner = THIS_MODULE,
    .open = poll_open,
    .read = poll_read,
    .write = poll_write,
    .poll = poll_poll,  /* Poll method */
};

static int __init poll_init(void)
{
    int ret;

    pdev = kzalloc(sizeof(*pdev), GFP_KERNEL);
    if (!pdev)
        return -ENOMEM;

    /* Initialize wait queues */
    init_waitqueue_head(&pdev->read_queue);
    init_waitqueue_head(&pdev->write_queue);
    spin_lock_init(&pdev->lock);

    /* Allocate device number */
    ret = alloc_chrdev_region(&pdev->devt, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        kfree(pdev);
        return ret;
    }

    /* Initialize and add cdev */
    cdev_init(&pdev->cdev, &poll_fops);
    ret = cdev_add(&pdev->cdev, pdev->devt, 1);
    if (ret < 0) {
        unregister_chrdev_region(pdev->devt, 1);
        kfree(pdev);
        return ret;
    }

    /* Create device class and node */
    pdev->class = class_create(THIS_MODULE, DEVICE_NAME);
    device_create(pdev->class, NULL, pdev->devt, NULL, DEVICE_NAME);

    printk("Poll char device registered: /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void __exit poll_exit(void)
{
    device_destroy(pdev->class, pdev->devt);
    class_destroy(pdev->class);
    cdev_del(&pdev->cdev);
    unregister_chrdev_region(pdev->devt, 1);
    kfree(pdev);
}

module_init(poll_init);
module_exit(poll_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Character device with poll support");
```

### User Space Poll Usage

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

int main() {
    int fd;
    struct pollfd fds[1];
    int ret;
    char buf[100];

    /* Open device */
    fd = open("/dev/poll_char", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    /* Setup poll structure */
    fds[0].fd = fd;
    fds[0].events = POLLIN | POLLOUT;  /* Monitor read and write */

    printf("Waiting for events...\n");

    /* Wait for events (timeout: 5 seconds) */
    ret = poll(fds, 1, 5000);

    if (ret == -1) {
        perror("poll");
    } else if (ret == 0) {
        printf("Timeout occurred!\n");
    } else {
        /* Check which events occurred */
        if (fds[0].revents & POLLIN) {
            printf("Data available for reading\n");
            read(fd, buf, sizeof(buf));
        }

        if (fds[0].revents & POLLOUT) {
            printf("Ready for writing\n");
            write(fd, "Hello", 5);
        }

        if (fds[0].revents & POLLERR) {
            printf("Error condition\n");
        }

        if (fds[0].revents & POLLHUP) {
            printf("Hang up\n");
        }
    }

    close(fd);
    return 0;
}
```

### Select System Call Example

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>

int main() {
    int fd;
    fd_set readfds, writefds;
    struct timeval tv;
    int ret;
    char buf[100];

    fd = open("/dev/poll_char", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    /* Clear and set file descriptor sets */
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(fd, &readfds);
    FD_SET(fd, &writefds);

    /* Set timeout: 5 seconds */
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    printf("Waiting with select()...\n");

    /* Wait for events */
    ret = select(fd + 1, &readfds, &writefds, NULL, &tv);

    if (ret == -1) {
        perror("select");
    } else if (ret == 0) {
        printf("Timeout!\n");
    } else {
        /* Check which operations are ready */
        if (FD_ISSET(fd, &readfds)) {
            printf("Ready for reading\n");
            read(fd, buf, sizeof(buf));
        }

        if (FD_ISSET(fd, &writefds)) {
            printf("Ready for writing\n");
            write(fd, "Test", 4);
        }
    }

    close(fd);
    return 0;
}
```

---