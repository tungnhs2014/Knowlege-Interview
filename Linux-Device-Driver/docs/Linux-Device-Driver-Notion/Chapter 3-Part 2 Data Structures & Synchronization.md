# Part 2. Data Structures & Synchronization

---

## 3.2 Synchronization Mechanisms

### Introduction to Kernel Locking

**Why synchronization is critical:**

When multiple execution contexts access shared resources concurrently, we need synchronization to prevent:

- **Race conditions:** Two threads modifying same data simultaneously
- **Data corruption:** Inconsistent state due to interleaved operations
- **Deadlocks:** Circular waiting for resources

**Execution contexts that need synchronization:**

```
Multiple CPUs (SMP)        → Need synchronization
Multiple processes         → Need synchronization
Process + IRQ handler      → Need synchronization
Multiple IRQ handlers      → Need synchronization
Softirq + process          → Need synchronization
```

**Kernel provides two main primitives:**

1. **Mutex** - For process context (can sleep)
2. **Spinlock** - For atomic context (cannot sleep)

---

## 3.2.1 Mutex (Mutual Exclusion)

### What is a Mutex?

**Mutex = Lock held by a TASK**

```c
/* Defined in include/linux/mutex.h */
struct mutex {
    atomic_long_t owner;          /* Task that owns mutex */
    spinlock_t wait_lock;         /* Protects wait_list */
    struct list_head wait_list;   /* Waiters put to sleep here */
#ifdef CONFIG_MUTEX_SPIN_ON_OWNER
    struct optimistic_spin_queue osq;  /* Spinners */
#endif
    /* Debug fields omitted */
};
```

**Key characteristics:**

- ✅ Can sleep (scheduler can switch tasks)
- ✅ Built on top of spinlocks
- ✅ Waiters are put into sleep queue
- ✅ Suitable for long critical sections
- ❌ Cannot be used in interrupt context

**How it works:**

```
Task A tries to lock mutex:
├─ If unlocked → Acquire immediately, continue
└─ If locked by Task B:
    ├─ Task A is removed from run queue
    ├─ Task A is put into wait_list (sleep)
    ├─ Scheduler runs another task (Task C)
    └─ When Task B unlocks:
        ├─ Wake up Task A (first in wait_list)
        └─ Task A gets mutex and continues
```

### Mutex API

**Initialization:**

```c
/* Method 1: Static initialization */
static DEFINE_MUTEX(my_mutex);

/* Expands to: */
#define DEFINE_MUTEX(mutexname) \
    struct mutex mutexname = __MUTEX_INITIALIZER(mutexname)

/* Method 2: Dynamic initialization */
struct mutex my_mutex;
mutex_init(&my_mutex);
```

**Locking:**

```c
/* Lock (sleep if already locked) - NOT RECOMMENDED */
void mutex_lock(struct mutex *lock);

/* Lock (interruptible by signals) - RECOMMENDED */
int mutex_lock_interruptible(struct mutex *lock);

/* Lock (interruptible by kill signals only) */
int mutex_lock_killable(struct mutex *lock);

/* Try to lock (return immediately) */
int mutex_trylock(struct mutex *lock);
```

**Unlocking:**

```c
void mutex_unlock(struct mutex *lock);
```

**Checking status:**

```c
/* Returns true if locked */
int mutex_is_locked(struct mutex *lock);
```

### When to Use Which Lock Function

**mutex_lock() - Use ONLY when:**

- You GUARANTEE mutex will be released
- You're NOT in interrupt context
- Signal handling is NOT important

```c
/* Example: Kernel thread that cannot be interrupted */
static int kernel_worker_thread(void *data)
{
    struct my_device *dev = data;

    while (!kthread_should_stop()) {
        mutex_lock(&dev->mutex);  /* OK here */
        process_data(dev);
        mutex_unlock(&dev->mutex);
        msleep(100);
    }
    return 0;
}
```

**mutex_lock_interruptible() - Use TYPICALLY (RECOMMENDED):**

- In device driver file operations
- When user can send signals (Ctrl+C)
- When you want to handle errors gracefully

```c
/* Example: Device read operation */
static ssize_t my_read(struct file *filp, char __user *buf,
                       size_t count, loff_t *f_pos)
{
    struct my_device *dev = filp->private_data;
    int ret;

    /* Use interruptible version */
    ret = mutex_lock_interruptible(&dev->mutex);
    if (ret)
        return -ERESTARTSYS;  /* Signal received */

    /* Critical section */
    ret = copy_to_user(buf, dev->buffer, count);

    mutex_unlock(&dev->mutex);

    return ret ? -EFAULT : count;
}
```

**mutex_lock_killable() - Use when:**

- You want to handle SIGKILL only
- Other signals should not interrupt

**mutex_trylock() - Use when:**

- You don't want to wait at all
- You have alternative action if lock fails

```c
static void optional_processing(struct my_device *dev)
{
    /* Try to lock, skip if busy */
    if (mutex_trylock(&dev->mutex)) {
        /* Got lock - do processing */
        update_device_state(dev);
        mutex_unlock(&dev->mutex);
    } else {
        /* Could not lock - skip or schedule later */
        pr_debug("Device busy, skipping update\n");
    }
}
```

### Complete Mutex Example

```c
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/delay.h>

/* Shared resource */
static int shared_counter = 0;
static DEFINE_MUTEX(counter_mutex);

/* Thread function 1 */
static int thread1_func(void *data)
{
    int i;

    for (i = 0; i < 1000 && !kthread_should_stop(); i++) {
        /* Lock mutex */
        mutex_lock(&counter_mutex);

        /* Critical section */
        shared_counter++;
        pr_info("Thread1: counter = %d\n", shared_counter);

        /* Unlock mutex */
        mutex_unlock(&counter_mutex);

        msleep(10);
    }

    return 0;
}

/* Thread function 2 */
static int thread2_func(void *data)
{
    int i;

    for (i = 0; i < 1000 && !kthread_should_stop(); i++) {
        /* Lock mutex */
        mutex_lock(&counter_mutex);

        /* Critical section */
        shared_counter--;
        pr_info("Thread2: counter = %d\n", shared_counter);

        /* Unlock mutex */
        mutex_unlock(&counter_mutex);

        msleep(10);
    }

    return 0;
}

static struct task_struct *thread1, *thread2;

static int __init mutex_example_init(void)
{
    pr_info("Starting mutex example\n");

    /* Create threads */
    thread1 = kthread_run(thread1_func, NULL, "thread1");
    thread2 = kthread_run(thread2_func, NULL, "thread2");

    if (IS_ERR(thread1) || IS_ERR(thread2)) {
        pr_err("Failed to create threads\n");
        return -1;
    }

    return 0;
}

static void __exit mutex_example_exit(void)
{
    /* Stop threads */
    if (thread1)
        kthread_stop(thread1);
    if (thread2)
        kthread_stop(thread2);

    pr_info("Final counter value: %d\n", shared_counter);
    pr_info("Mutex example unloaded\n");
}

module_init(mutex_example_init);
module_exit(mutex_example_exit);

MODULE_LICENSE("GPL");
```

### Mutex Rules (MUST FOLLOW!)

**From include/linux/mutex.h:**

```c
/*
 * - only one task can hold the mutex at a time
 * - only the owner can unlock the mutex
 * - multiple unlocks are not permitted
 * - recursive locking is not permitted
 * - a mutex object must be initialized via the API
 * - a mutex object must not be initialized via memset or copying
 * - task may not exit with mutex held
 * - memory areas where held locks reside must not be freed
 * - held mutexes must not be reinitialized
 * - mutexes may not be used in hardware or software interrupt
 *   contexts such as tasklets and timers
 */
```

**Explanation:**

❌ **Wrong:** Multiple unlocks

```c
mutex_lock(&my_mutex);
/* ... */
mutex_unlock(&my_mutex);
mutex_unlock(&my_mutex);  /* WRONG! Double unlock */
```

❌ **Wrong:** Recursive locking

```c
void function_a(void) {
    mutex_lock(&my_mutex);
    function_b();  /* Calls another function */
    mutex_unlock(&my_mutex);
}

void function_b(void) {
    mutex_lock(&my_mutex);  /* DEADLOCK! Same task locks again */
    /* ... */
    mutex_unlock(&my_mutex);
}
```

❌ **Wrong:** Using in interrupt context

```c
static irqreturn_t my_irq_handler(int irq, void *dev_id)
{
    mutex_lock(&my_mutex);  /* WRONG! Cannot sleep in IRQ! */
    /* ... */
    mutex_unlock(&my_mutex);
    return IRQ_HANDLED;
}
```

✅ **Correct:** Basic usage

```c
static ssize_t my_write(struct file *filp, const char __user *buf,
                        size_t count, loff_t *f_pos)
{
    struct my_dev *dev = filp->private_data;
    int ret;

    ret = mutex_lock_interruptible(&dev->mutex);
    if (ret)
        return -ERESTARTSYS;

    /* Process data */
    ret = process_write_data(dev, buf, count);

    mutex_unlock(&dev->mutex);
    return ret;
}
```

---

## 3.2.2 Spinlock

### What is a Spinlock?

**Spinlock = Lock held by a CPU**

**Key characteristics:**

- ✅ Hardware-based locking (atomic operations)
- ✅ No sleeping - spins in busy loop
- ✅ Disables preemption on local CPU
- ✅ Suitable for SHORT critical sections
- ✅ Can be used in interrupt context
- ❌ Wastes CPU time if held too long

**How it works:**

```
CPU A wants to acquire spinlock:
├─ If unlocked:
│   ├─ Disable preemption on CPU A
│   ├─ Acquire lock atomically
│   └─ Enter critical section
└─ If locked by CPU B:
    ├─ CPU A enters busy loop (spinning)
    ├─ CPU A keeps checking lock status
    ├─ CPU A wastes CPU cycles
    └─ When CPU B releases:
        └─ CPU A acquires lock and proceeds
```

**Visual comparison:**

```
Mutex (Task-level):                 Spinlock (CPU-level):
Task A locks → Sleeps               CPU A locks → Runs critical code
Task B tries → Sleeps in queue      CPU B tries → Spins waiting
Scheduler runs Task C               CPU B busy, cannot run other code
Task A unlocks → Wake Task B        CPU A unlocks → CPU B acquires
```

### Spinlock API

**Initialization:**

```c
/* Method 1: Static */
static DEFINE_SPINLOCK(my_spinlock);

/* Method 2: Dynamic */
spinlock_t my_spinlock;
spin_lock_init(&my_spinlock);
```

**Basic locking (RARELY USED DIRECTLY):**

```c
/* Lock - disable preemption */
void spin_lock(spinlock_t *lock);

/* Unlock - enable preemption */
void spin_unlock(spinlock_t *lock);
```

**IRQ-safe variants (COMMONLY USED):**

```c
/* Lock + disable IRQs */
void spin_lock_irq(spinlock_t *lock);
void spin_unlock_irq(spinlock_t *lock);

/* Lock + save and disable IRQs (RECOMMENDED) */
void spin_lock_irqsave(spinlock_t *lock, unsigned long flags);
void spin_unlock_irqrestore(spinlock_t *lock, unsigned long flags);

/* Lock + disable bottom halves */
void spin_lock_bh(spinlock_t *lock);
void spin_unlock_bh(spinlock_t *lock);
```

**Try-lock variant:**

```c
/* Returns 1 if lock acquired, 0 if already locked */
int spin_trylock(spinlock_t *lock);
```

### Understanding Spinlock Variants

**Why different variants exist:**

**Problem scenario:**

```
CPU holds spinlock → Interrupt occurs → IRQ handler tries same spinlock
                  → DEADLOCK (IRQ spins forever!)
```

**Solution hierarchy:**

```
spin_lock()
├─ Disables: Preemption only
├─ Use when: No IRQs access this resource
└─ Risk: Deadlock if IRQ needs same lock

spin_lock_irq()
├─ Disables: Preemption + ALL interrupts
├─ Use when: You KNOW interrupts are enabled
└─ Risk: Might wrongly enable previously disabled IRQs

spin_lock_irqsave() ← SAFEST!
├─ Disables: Preemption + saves IRQ state + disables IRQs
├─ Restores: Previous IRQ state on unlock
├─ Use when: Shared with IRQ handlers (most common)
└─ Risk: None (safest option)

spin_lock_bh()
├─ Disables: Preemption + bottom halves (softirq, tasklet)
├─ Use when: Shared with softirq/tasklet
└─ Risk: Hardware IRQ can still preempt
```

### Detailed Examples

**Example 1: Process context + IRQ handler sharing**

```c
/* Shared data */
static spinlock_t my_lock;
static int shared_counter = 0;

/* Process context (e.g., file operation) */
static ssize_t my_write(struct file *filp, const char __user *buf,
                        size_t count, loff_t *f_pos)
{
    unsigned long flags;

    /* MUST use irqsave - IRQ handler also accesses this */
    spin_lock_irqsave(&my_lock, flags);

    shared_counter++;  /* Critical section */

    spin_unlock_irqrestore(&my_lock, flags);

    return count;
}

/* IRQ handler */
static irqreturn_t my_irq_handler(int irq, void *dev_id)
{
    /* Preemption already disabled in IRQ context */
    /* IRQ line already disabled */
    /* Just use spin_lock/spin_unlock */
    spin_lock(&my_lock);

    shared_counter--;  /* Critical section */

    spin_unlock(&my_lock);

    return IRQ_HANDLED;
}
```

**Why different locks in same code?**

```
Process context:
├─ Can be preempted by IRQ
├─ Needs to disable IRQs
└─ Uses spin_lock_irqsave()

IRQ handler:
├─ Cannot be preempted (IRQ already running)
├─ Its IRQ line already disabled
├─ Only needs to prevent other CPUs
└─ Uses spin_lock() (simpler, faster)
```

**Example 2: Multiple IRQ handlers sharing**

```c
/* Device managing 2 IRQ lines */
static spinlock_t device_lock;

/* IRQ handler 1 */
static irqreturn_t irq1_handler(int irq, void *dev_id)
{
    unsigned long flags;

    /* IRQ1 can be interrupted by IRQ2! */
    /* Must disable all IRQs */
    spin_lock_irqsave(&device_lock, flags);

    /* Access shared device registers */
    update_device_register();

    spin_unlock_irqrestore(&device_lock, flags);

    return IRQ_HANDLED;
}

/* IRQ handler 2 */
static irqreturn_t irq2_handler(int irq, void *dev_id)
{
    unsigned long flags;

    /* Same - must disable all IRQs */
    spin_lock_irqsave(&device_lock, flags);

    /* Access shared device registers */
    update_device_register();

    spin_unlock_irqrestore(&device_lock, flags);

    return IRQ_HANDLED;
}
```

**Example 3: Using spin_trylock**

```c
static void optional_update(struct my_device *dev)
{
    unsigned long flags;

    /* Try to acquire lock */
    if (spin_trylock_irqsave(&dev->lock, flags)) {
        /* Got lock - do update */
        dev->status++;
        spin_unlock_irqrestore(&dev->lock, flags);
        pr_debug("Update completed\n");
    } else {
        /* Lock held by someone else - skip */
        pr_debug("Device busy, skipping update\n");
    }
}
```

### Complete Real-World Example

```c
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

struct my_device {
    void __iomem *reg_base;
    spinlock_t lock;
    int irq;
    u32 buffer_count;
};

/* Process context - file operation */
static ssize_t my_read(struct file *filp, char __user *buf,
                       size_t count, loff_t *f_pos)
{
    struct my_device *dev = filp->private_data;
    unsigned long flags;
    u32 buffer_count;

    /* Lock with IRQ save - shared with IRQ handler */
    spin_lock_irqsave(&dev->lock, flags);
    buffer_count = dev->buffer_count;
    spin_unlock_irqrestore(&dev->lock, flags);

    /* Process data (outside critical section) */
    return copy_to_user(buf, &buffer_count, sizeof(buffer_count));
}

/* IRQ handler */
static irqreturn_t my_irq_handler(int irq, void *dev_id)
{
    struct my_device *dev = dev_id;
    u32 status;

    /* Read device status (outside lock) */
    status = readl(dev->reg_base + REG_STATUS);

    /* Acknowledge IRQ at device (outside lock) */
    writel(status | IRQ_ACK_BIT, dev->reg_base + REG_STATUS);

    /* Only lock for shared data access */
    spin_lock(&dev->lock);
    dev->buffer_count++;
    spin_unlock(&dev->lock);

    return IRQ_HANDLED;
}

static int my_probe(struct platform_device *pdev)
{
    struct my_device *dev;
    int ret;

    dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    /* Initialize spinlock */
    spin_lock_init(&dev->lock);

    /* Request IRQ */
    dev->irq = platform_get_irq(pdev, 0);
    ret = devm_request_irq(&pdev->dev, dev->irq,
                           my_irq_handler, 0,
                           dev_name(&pdev->dev), dev);
    if (ret) {
        dev_err(&pdev->dev, "Failed to request IRQ\n");
        return ret;
    }

    platform_set_drvdata(pdev, dev);
    return 0;
}

static struct platform_driver my_driver = {
    .probe = my_probe,
    .driver = {
        .name = "my-device",
    },
};

module_platform_driver(my_driver);
MODULE_LICENSE("GPL");
```

### Spinlock Rules

**✅ DO:**

- Keep critical sections SHORT (microseconds, not milliseconds)
- Use appropriate variant (irqsave for IRQ sharing)
- Always unlock in same function that locked
- Release lock before calling functions that might sleep
- Use mutex if critical section > 100 microseconds

**❌ DON'T:**

- Sleep while holding spinlock (no msleep, kmalloc with GFP_KERNEL, mutex_lock, etc.)
- Hold spinlock for long time (wastes CPU)
- Call functions that might sleep
- Access user space memory (might page fault = sleep)
- Acquire same spinlock recursively (deadlock)

**Examples of DON'Ts:**

```c
/* WRONG! Sleeping while holding spinlock */
spin_lock(&my_lock);
msleep(100);  /* WRONG! Sleeps */
spin_unlock(&my_lock);

/* WRONG! Memory allocation might sleep */
spin_lock(&my_lock);
ptr = kmalloc(size, GFP_KERNEL);  /* WRONG! Might sleep */
spin_unlock(&my_lock);

/* CORRECT! Use atomic allocation */
spin_lock(&my_lock);
ptr = kmalloc(size, GFP_ATOMIC);  /* OK! Won't sleep */
spin_unlock(&my_lock);

/* WRONG! Accessing user space */
spin_lock(&my_lock);
copy_to_user(buf, data, len);  /* WRONG! Might page fault */
spin_unlock(&my_lock);
```

---

## 3.2.3 Spinlock vs Mutex Comparison

**Decision matrix:**

```
┌────────────────────┬─────────────┬────────────┐
│ Scenario           │ Use         │ Reason     │
├────────────────────┼─────────────┼────────────┤
│ IRQ context        │ Spinlock    │ Can't sleep│
│ Process context    │ Mutex       │ Can sleep  │
│ Short critical (<1│ Spinlock    │ No overhead│
│   ms)              │             │            │
│ Long critical (>1  │ Mutex       │ Don't waste│
│   ms)              │             │ CPU        │
│ Shared with IRQ    │ Spinlock    │ IRQ can't  │
│                    │             │ sleep      │
│ Process only       │ Mutex       │ Better for │
│                    │             │ SMP        │
│ Need to sleep in   │ Mutex       │ Spinlock   │
│ critical           │             │ forbids it │
│ Softirq/tasklet    │ Spinlock_bh │ BH context │
└────────────────────┴─────────────┴────────────┘
```

**Feature comparison:**

```
┌──────────────────┬──────────┬──────────┐
│ Feature          │ Mutex    │ Spinlock │
├──────────────────┼──────────┼──────────┤
│ Sleep allowed    │ Yes      │ No       │
│ IRQ context      │ No       │ Yes      │
│ Overhead         │ High     │ Low      │
│ Wait behavior    │ Sleep    │ Busy-wait│
│ CPU usage        │ Efficient│ Wasteful │
│ Lock holder      │ Task     │ CPU      │
│ Preemption       │ Allowed  │ Disabled │
│ Deadlock risk    │ Medium   │ High     │
│ Use case         │ Long ops │ Short ops│
└──────────────────┴──────────┴──────────┘
```