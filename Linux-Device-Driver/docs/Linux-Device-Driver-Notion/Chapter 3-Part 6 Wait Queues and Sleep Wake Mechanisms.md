# Part 6. Wait Queues and Sleep/Wake Mechanisms

This part covers kernel mechanisms for blocking, waiting, and synchronization through wait queues, poll/select operations, and the completion framework.

---

## 3.4 Wait Queues and Sleep/Wake

### Introduction to Kernel Sleeping

**The Need for Sleeping:**

In kernel programming, many operations require waiting for events:

- Waiting for data to arrive
- Waiting for hardware to complete operation
- Waiting for resource to become available
- Waiting for specific condition to be true

**Sleep vs Busy-Wait:**

```
Busy-Wait (BAD):                   Sleep (GOOD):
while (!ready)                     wait_event(queue, ready);
    /* Waste CPU */                /* CPU does other work */
└─ Burns CPU cycles                └─ Efficient, CPU available

CPU: 100% usage                    CPU: Available for other tasks
```

**Linux kernel provides elegant sleep/wake mechanisms:**

```
Process Sleeping Flow:
┌──────────────┐
│ Process A    │  wait_event(wq, condition == false)
│ (Running)    │
└──────────────┘
       │
       │ Condition false
       ↓
┌──────────────┐
│ Process A    │  Removed from run queue
│ (Sleeping)   │  Added to wait queue
└──────────────┘  State: TASK_INTERRUPTIBLE
       │
       │ wake_up() called
       │ Condition true
       ↓
┌──────────────┐
│ Process A    │  Back to run queue
│ (Running)    │  State: TASK_RUNNING
└──────────────┘
```

---

## 3.4.1 Wait Queue Implementation

### What is a Wait Queue?

**Wait Queue = List of processes waiting for an event**

```c
/* Defined in include/linux/wait.h */
struct wait_queue_head {
    spinlock_t lock;         /* Protects the list */
    struct list_head head;   /* List of waiting processes */
};

typedef struct wait_queue_head wait_queue_head_t;
```

**How it works:**

```
Wait Queue Structure:
┌─────────────────────────────────┐
│  wait_queue_head_t              │
│  ├─ spinlock_t lock             │
│  └─ list_head head ────┐        │
└─────────────────────────│────────┘
                          │
       ┌──────────────────┘
       ↓
   ┌────────────┐      ┌────────────┐      ┌────────────┐
   │ Process A  │ ───→ │ Process B  │ ───→ │ Process C  │
   │ (Sleeping) │      │ (Sleeping) │      │ (Sleeping) │
   └────────────┘      └────────────┘      └────────────┘
```

### Wait Queue Initialization

**Method 1: Static Declaration**

```c
/* Declare and initialize wait queue statically */
static DECLARE_WAIT_QUEUE_HEAD(my_wait_queue);

/* Expands to: */
wait_queue_head_t my_wait_queue = {
    .lock = __SPIN_LOCK_UNLOCKED(my_wait_queue.lock),
    .head = { &(my_wait_queue.head), &(my_wait_queue.head) }
};
```

**Method 2: Dynamic Initialization**

```c
wait_queue_head_t my_wait_queue;

/* Initialize at runtime */
init_waitqueue_head(&my_wait_queue);
```

**Example: Driver initialization**

```c
struct char_device {
    wait_queue_head_t read_queue;
    wait_queue_head_t write_queue;
    char *buffer;
    int buffer_size;
    int data_available;
};

static int char_probe(struct platform_device *pdev)
{
    struct char_device *cdev;

    cdev = devm_kzalloc(&pdev->dev, sizeof(*cdev), GFP_KERNEL);
    if (!cdev)
        return -ENOMEM;

    /* Initialize wait queues */
    init_waitqueue_head(&cdev->read_queue);
    init_waitqueue_head(&cdev->write_queue);

    cdev->buffer_size = 1024;
    cdev->buffer = devm_kzalloc(&pdev->dev, cdev->buffer_size, GFP_KERNEL);

    platform_set_drvdata(pdev, cdev);
    return 0;
}
```

### Blocking - wait_event() Family

**Core blocking functions:**

```c
/* Wait until condition becomes true (uninterruptible) */
wait_event(wq_head, condition);

/* Wait until condition becomes true (interruptible) */
int wait_event_interruptible(wq_head, condition);

/* Wait with timeout (interruptible) */
long wait_event_interruptible_timeout(wq_head, condition, timeout);

/* Wait with timeout (uninterruptible) */
long wait_event_timeout(wq_head, condition, timeout);

/* Wait (killable - only SIGKILL can interrupt) */
int wait_event_killable(wq_head, condition);
```

**How wait_event works:**

```
1. Evaluate condition
   ├─ If TRUE  → Return immediately
   └─ If FALSE → Continue to step 2

2. Change process state
   ├─ TASK_INTERRUPTIBLE (for _interruptible)
   └─ TASK_UNINTERRUPTIBLE (for regular wait_event)

3. Add process to wait queue

4. Call schedule() - give up CPU

5. When woken up:
   ├─ Re-evaluate condition
   ├─ If TRUE → Remove from queue, return
   └─ If FALSE → Go back to sleep
```

**Return values:**

| Function | Return Value |
| --- | --- |
| `wait_event()` | void (always waits) |
| `wait_event_interruptible()` | 0 if condition met, -ERESTARTSYS if interrupted |
| `wait_event_timeout()` | 0 if timeout, remaining jiffies if condition met |
| `wait_event_interruptible_timeout()` | 0 if timeout, -ERESTARTSYS if interrupted, remaining jiffies if completed |

**CRITICAL: Always check return value of _interruptible variants!**

```c
int ret;

/* CORRECT - check return value */
ret = wait_event_interruptible(my_queue, data_ready);
if (ret)
    return -ERESTARTSYS;  /* Signal received */

/* WRONG - ignoring return value */
wait_event_interruptible(my_queue, data_ready);
/* What if signal received? Undefined behavior! */
```

### Waking Up - wake_up() Family

**Core waking functions:**

```c
/* Wake up ONE process from queue */
void wake_up(wait_queue_head_t *q);

/* Wake up ALL processes from queue */
void wake_up_all(wait_queue_head_t *q);

/* Wake up ONE interruptible process */
void wake_up_interruptible(wait_queue_head_t *q);

/* Wake up ALL interruptible processes */
void wake_up_interruptible_all(wait_queue_head_t *q);
```

**What happens when wake_up is called:**

```
wake_up() execution:
1. Re-evaluate condition for each waiter
2. If condition TRUE:
   ├─ Remove process from wait queue
   ├─ Set state to TASK_RUNNING
   └─ Add to run queue (scheduler will run it)
3. If condition FALSE:
   └─ Process stays asleep
```

**Important notes:**

1. **wake_up() does NOT immediately run the process**
    - Just makes it runnable
    - Scheduler decides when to run it
2. **Must update condition BEFORE calling wake_up()**
    
    ```c
    /* CORRECT order */
    data_ready = 1;          /* Update condition */
    wake_up(&my_queue);      /* Then wake up */
    
    /* WRONG order */
    wake_up(&my_queue);      /* Wake up */
    data_ready = 1;          /* Too late! */
    ```
    
3. **Can be called from any context** (including IRQ handlers)

### Basic Wait Queue Example

```c
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>

static DECLARE_WAIT_QUEUE_HEAD(my_wq);
static int condition = 0;
static struct task_struct *worker_thread;

/* Worker thread - signals the event */
static int worker_fn(void *data)
{
    printk("Worker: sleeping for 5 seconds...\n");
    msleep(5000);

    printk("Worker: setting condition and waking up waiters\n");
    condition = 1;              /* Set condition TRUE */
    wake_up_interruptible(&my_wq);  /* Wake up */

    return 0;
}

static int __init wait_example_init(void)
{
    int ret;

    printk("Module init: creating worker thread\n");

    /* Create worker thread */
    worker_thread = kthread_run(worker_fn, NULL, "wait_worker");
    if (IS_ERR(worker_thread))
        return PTR_ERR(worker_thread);

    printk("Module init: waiting for condition...\n");

    /* Wait for condition to become true */
    ret = wait_event_interruptible(my_wq, condition != 0);
    if (ret) {
        printk("Wait interrupted by signal\n");
        return -ERESTARTSYS;
    }

    printk("Module init: condition met, continuing\n");
    return 0;
}

static void __exit wait_example_exit(void)
{
    printk("Module cleanup\n");
}

module_init(wait_example_init);
module_exit(wait_example_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Basic wait queue example");
```

**Output:**

```
Module init: creating worker thread
Module init: waiting for condition...
Worker: sleeping for 5 seconds...
Worker: setting condition and waking up waiters
Module init: condition met, continuing
```

### Complete Real-World Example: Character Device with Blocking Read

```c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "blocking_char"
#define BUFFER_SIZE 1024

struct blocking_dev {
    struct cdev cdev;
    dev_t devt;
    struct class *class;

    wait_queue_head_t read_queue;
    wait_queue_head_t write_queue;

    char buffer[BUFFER_SIZE];
    int read_pos;
    int write_pos;
    int data_size;

    spinlock_t lock;
};

static struct blocking_dev *bdev;

/* Calculate available data */
static int data_available(struct blocking_dev *dev)
{
    return dev->data_size > 0;
}

/* Calculate free space */
static int space_available(struct blocking_dev *dev)
{
    return dev->data_size < BUFFER_SIZE;
}

/* Read operation - blocks until data available */
static ssize_t blocking_read(struct file *filp, char __user *buf,
                            size_t count, loff_t *f_pos)
{
    struct blocking_dev *dev = filp->private_data;
    int bytes_read = 0;
    unsigned long flags;
    int ret;

    /* Wait for data to be available */
    ret = wait_event_interruptible(dev->read_queue,
                                   data_available(dev));
    if (ret)
        return -ERESTARTSYS;  /* Interrupted by signal */

    /* Lock and read data */
    spin_lock_irqsave(&dev->lock, flags);

    /* Calculate how much to read */
    bytes_read = min((int)count, dev->data_size);

    /* Handle circular buffer wrap */
    if (dev->read_pos + bytes_read > BUFFER_SIZE) {
        int first_part = BUFFER_SIZE - dev->read_pos;
        int second_part = bytes_read - first_part;

        spin_unlock_irqrestore(&dev->lock, flags);

        /* Copy first part */
        if (copy_to_user(buf, &dev->buffer[dev->read_pos], first_part))
            return -EFAULT;

        /* Copy second part */
        if (copy_to_user(buf + first_part, &dev->buffer[0], second_part))
            return -EFAULT;

        spin_lock_irqsave(&dev->lock, flags);
        dev->read_pos = second_part;
    } else {
        spin_unlock_irqrestore(&dev->lock, flags);

        /* Simple case - no wrap */
        if (copy_to_user(buf, &dev->buffer[dev->read_pos], bytes_read))
            return -EFAULT;

        spin_lock_irqsave(&dev->lock, flags);
        dev->read_pos = (dev->read_pos + bytes_read) % BUFFER_SIZE;
    }

    dev->data_size -= bytes_read;

    spin_unlock_irqrestore(&dev->lock, flags);

    /* Wake up writers - space now available */
    wake_up_interruptible(&dev->write_queue);

    return bytes_read;
}

/* Write operation - blocks until space available */
static ssize_t blocking_write(struct file *filp, const char __user *buf,
                             size_t count, loff_t *f_pos)
{
    struct blocking_dev *dev = filp->private_data;
    int bytes_written = 0;
    unsigned long flags;
    int ret;

    /* Wait for space to be available */
    ret = wait_event_interruptible(dev->write_queue,
                                   space_available(dev));
    if (ret)
        return -ERESTARTSYS;  /* Interrupted by signal */

    /* Lock and write data */
    spin_lock_irqsave(&dev->lock, flags);

    /* Calculate how much to write */
    bytes_written = min((int)count, BUFFER_SIZE - dev->data_size);

    /* Handle circular buffer wrap */
    if (dev->write_pos + bytes_written > BUFFER_SIZE) {
        int first_part = BUFFER_SIZE - dev->write_pos;
        int second_part = bytes_written - first_part;

        spin_unlock_irqrestore(&dev->lock, flags);

        /* Copy first part */
        if (copy_from_user(&dev->buffer[dev->write_pos], buf, first_part))
            return -EFAULT;

        /* Copy second part */
        if (copy_from_user(&dev->buffer[0], buf + first_part, second_part))
            return -EFAULT;

        spin_lock_irqsave(&dev->lock, flags);
        dev->write_pos = second_part;
    } else {
        spin_unlock_irqrestore(&dev->lock, flags);

        /* Simple case - no wrap */
        if (copy_from_user(&dev->buffer[dev->write_pos], buf, bytes_written))
            return -EFAULT;

        spin_lock_irqsave(&dev->lock, flags);
        dev->write_pos = (dev->write_pos + bytes_written) % BUFFER_SIZE;
    }

    dev->data_size += bytes_written;

    spin_unlock_irqrestore(&dev->lock, flags);

    /* Wake up readers - data now available */
    wake_up_interruptible(&dev->read_queue);

    return bytes_written;
}

static int blocking_open(struct inode *inode, struct file *filp)
{
    filp->private_data = bdev;
    return 0;
}

static int blocking_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations blocking_fops = {
    .owner = THIS_MODULE,
    .open = blocking_open,
    .release = blocking_release,
    .read = blocking_read,
    .write = blocking_write,
};

static int __init blocking_init(void)
{
    int ret;

    bdev = kzalloc(sizeof(*bdev), GFP_KERNEL);
    if (!bdev)
        return -ENOMEM;

    /* Initialize wait queues */
    init_waitqueue_head(&bdev->read_queue);
    init_waitqueue_head(&bdev->write_queue);
    spin_lock_init(&bdev->lock);

    /* Allocate device number */
    ret = alloc_chrdev_region(&bdev->devt, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        kfree(bdev);
        return ret;
    }

    /* Initialize and add cdev */
    cdev_init(&bdev->cdev, &blocking_fops);
    ret = cdev_add(&bdev->cdev, bdev->devt, 1);
    if (ret < 0) {
        unregister_chrdev_region(bdev->devt, 1);
        kfree(bdev);
        return ret;
    }

    /* Create device class */
    bdev->class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(bdev->class)) {
        cdev_del(&bdev->cdev);
        unregister_chrdev_region(bdev->devt, 1);
        kfree(bdev);
        return PTR_ERR(bdev->class);
    }

    /* Create device node */
    device_create(bdev->class, NULL, bdev->devt, NULL, DEVICE_NAME);

    printk("Blocking char device registered: /dev/%s\n", DEVICE_NAME);
    return 0;
}

static void __exit blocking_exit(void)
{
    device_destroy(bdev->class, bdev->devt);
    class_destroy(bdev->class);
    cdev_del(&bdev->cdev);
    unregister_chrdev_region(bdev->devt, 1);
    kfree(bdev);

    printk("Blocking char device unregistered\n");
}

module_init(blocking_init);
module_exit(blocking_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Character device with blocking read/write");
```

**User space test program:**

```c
/* reader.c */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd;
    char buf[100];
    ssize_t n;

    fd = open("/dev/blocking_char", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("Waiting for data...\n");
    n = read(fd, buf, sizeof(buf));  /* Blocks here! */

    if (n > 0) {
        printf("Read %zd bytes: %.*s\n", n, (int)n, buf);
    }

    close(fd);
    return 0;
}

/* writer.c */
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main() {
    int fd;
    const char *msg = "Hello from writer!";

    sleep(3);  /* Give reader time to start */

    fd = open("/dev/blocking_char", O_WRONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    printf("Writing data...\n");
    write(fd, msg, strlen(msg));

    close(fd);
    return 0;
}
```

### Wait Queue with Timeout Example

```c
#include <linux/module.h>
#include <linux/wait.h>
#include <linux/jiffies.h>

static DECLARE_WAIT_QUEUE_HEAD(timeout_wq);
static int data_ready = 0;

static int __init timeout_example_init(void)
{
    long ret;
    unsigned long timeout_jiffies = msecs_to_jiffies(5000);  /* 5 seconds */

    printk("Waiting for data with 5 second timeout...\n");

    /* Wait with timeout */
    ret = wait_event_interruptible_timeout(timeout_wq,
                                          data_ready != 0,
                                          timeout_jiffies);

    if (ret == 0) {
        printk("Timeout occurred! Data not ready\n");
        return -ETIMEDOUT;
    } else if (ret == -ERESTARTSYS) {
        printk("Interrupted by signal\n");
        return -ERESTARTSYS;
    } else {
        printk("Data ready! Remaining time: %ld jiffies\n", ret);
        return 0;
    }
}

module_init(timeout_example_init);
MODULE_LICENSE("GPL");
```

---