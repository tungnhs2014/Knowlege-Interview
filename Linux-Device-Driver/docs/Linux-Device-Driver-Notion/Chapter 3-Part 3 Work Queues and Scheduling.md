# Part 3. Work Queues and Scheduling

This part covers kernel work deferring mechanisms and timer management - essential tools for handling asynchronous operations and time-based processing in device drivers.

---

## 3.3 Work Queues and Scheduling

### Introduction to Deferred Work

**The Problem: Why Defer Work?**

In kernel programming, certain operations cannot or should not be performed immediately:

```
Interrupt Context Limitations:
├─ Cannot sleep
├─ Cannot call blocking functions
├─ Cannot hold mutex
├─ Must execute very quickly
└─ Limited stack space

Solution: Deferred Work Mechanisms
├─ Workqueues (can sleep, process context)
├─ Tasklets (atomic, no sleep)
└─ Timers (time-based execution)
```

**When to defer work:**

1. **IRQ Handler Split:**
    - Top half (hard IRQ): Minimal work, acknowledge hardware
    - Bottom half (deferred): Heavy processing, data manipulation
2. **Long Operations:**
    - Operations that take > 100 microseconds
    - I/O operations that might sleep
    - Memory allocation with GFP_KERNEL
3. **Time-Based Operations:**
    - Periodic polling
    - Timeout handling
    - Delayed execution

**Linux provides three main mechanisms:**

| Mechanism | Context | Can Sleep? | Use Case |
| --- | --- | --- | --- |
| **Workqueue** | Process | ✅ Yes | Long operations, sleeping allowed |
| **Tasklet** | Atomic | ❌ No | Fast operations, atomic context |
| **Timer** | Atomic | ❌ No | Time-based execution |

---

## 3.3.1 Workqueue API and Usage Patterns

### What is a Workqueue?

**Workqueue = Asynchronous work execution in process context**

A workqueue allows your driver to defer work to be executed later in a **kernel thread** (process context), where sleeping and blocking operations are allowed.

**Key characteristics:**

- ✅ Runs in **process context** (can sleep)
- ✅ Can use **mutex**, **semaphores**
- ✅ Can call **GFP_KERNEL allocations**
- ✅ Can perform **I/O operations**
- ✅ Suitable for **long-running tasks**
- ⚠️ Cannot guarantee **immediate execution**

**Architecture overview:**

```
                    Workqueue Subsystem
┌─────────────────────────────────────────────────────┐
│                                                       │
│  ┌────────────┐         ┌──────────────┐            │
│  │ Your Driver│─submit─→│  Work Queue  │            │
│  └────────────┘         └──────────────┘            │
│                                │                     │
│                         enqueue work                 │
│                                ↓                     │
│                    ┌─────────────────────┐           │
│                    │   Worker Thread     │           │
│                    │   (Kernel Thread)   │           │
│                    │   - Can sleep       │           │
│                    │   - Process context │           │
│                    └─────────────────────┘           │
│                                │                     │
│                        execute work handler          │
│                                ↓                     │
│                    ┌─────────────────────┐           │
│                    │  Your Work Function │           │
│                    └─────────────────────┘           │
└─────────────────────────────────────────────────────┘
```

### Core Data Structures

**1. Work Item Structure:**

```c
/* Defined in include/linux/workqueue.h */
struct work_struct {
    atomic_long_t data;           /* Work item state and flags */
    struct list_head entry;       /* Link in workqueue list */
    work_func_t func;            /* Function to execute */
#ifdef CONFIG_LOCKDEP
    struct lockdep_map lockdep_map;
#endif
};
```

**2. Delayed Work Structure:**

```c
struct delayed_work {
    struct work_struct work;      /* Embedded work structure */
    struct timer_list timer;      /* Timer for delayed execution */
    struct workqueue_struct *wq;  /* Target workqueue */
    int cpu;                      /* CPU to run on */
};
```

**3. Work Function Prototype:**

```c
/* Your work handler signature */
typedef void (*work_func_t)(struct work_struct *work);

/* Example work function */
void my_work_handler(struct work_struct *work)
{
    /* Your code here - can sleep! */
}
```

**Important:** The work function receives `struct work_struct *` as parameter. To access your driver's data structure, use `container_of()`:

```c
struct my_device {
    struct work_struct my_work;
    int device_data;
    /* other fields */
};

void my_work_handler(struct work_struct *work)
{
    /* Get pointer to containing structure */
    struct my_device *dev = container_of(work, struct my_device, my_work);

    /* Now you can access dev->device_data */
    printk("Device data: %d\n", dev->device_data);
}
```

### Workqueue Initialization

**Method 1: Static Initialization (Compile Time)**

```c
/* Declare and initialize work structure statically */
static void my_work_handler(struct work_struct *work);

static DECLARE_WORK(my_work, my_work_handler);

/* For delayed work */
static DECLARE_DELAYED_WORK(my_delayed_work, my_work_handler);
```

**Method 2: Dynamic Initialization (Runtime)**

```c
struct work_struct my_work;
struct delayed_work my_delayed_work;

/* Initialize normal work */
INIT_WORK(&my_work, my_work_handler);

/* Initialize delayed work */
INIT_DELAYED_WORK(&my_delayed_work, my_work_handler);
```

**Example: Driver initialization**

```c
struct my_device {
    struct device *dev;
    struct work_struct tx_work;
    struct delayed_work rx_work;
    spinlock_t lock;
    u8 *buffer;
};

static void tx_work_handler(struct work_struct *work)
{
    struct my_device *mdev = container_of(work, struct my_device, tx_work);
    /* Process transmission */
}

static void rx_work_handler(struct work_struct *work)
{
    struct delayed_work *dwork = to_delayed_work(work);
    struct my_device *mdev = container_of(dwork, struct my_device, rx_work);
    /* Process reception */
}

static int my_probe(struct platform_device *pdev)
{
    struct my_device *mdev;

    mdev = devm_kzalloc(&pdev->dev, sizeof(*mdev), GFP_KERNEL);
    if (!mdev)
        return -ENOMEM;

    /* Initialize work structures */
    INIT_WORK(&mdev->tx_work, tx_work_handler);
    INIT_DELAYED_WORK(&mdev->rx_work, rx_work_handler);

    spin_lock_init(&mdev->lock);

    return 0;
}
```

### Scheduling Work - System Workqueues

**The kernel provides system-wide workqueues** that you can use without creating your own. This is recommended for most drivers.

**System workqueue: `system_wq`**

Defined in `kernel/workqueue.c`:

```c
struct workqueue_struct *system_wq __read_mostly;
```

**Scheduling functions:**

```c
/* Schedule work on system workqueue immediately */
int schedule_work(struct work_struct *work);

/* Schedule work on specific CPU */
int schedule_work_on(int cpu, struct work_struct *work);

/* Schedule delayed work (after delay in jiffies) */
int schedule_delayed_work(struct delayed_work *dwork,
                          unsigned long delay);

/* Schedule delayed work on specific CPU */
int schedule_delayed_work_on(int cpu, struct delayed_work *dwork,
                              unsigned long delay);
```

**Return values:**

- `false` (0): Work was already on queue
- `true` (1): Work was successfully queued

**Time conversion helpers:**

```c
/* Convert milliseconds to jiffies */
unsigned long msecs_to_jiffies(const unsigned int m);

/* Convert microseconds to jiffies */
unsigned long usecs_to_jiffies(const unsigned int u);
```

**Example: Scheduling work from IRQ handler**

```c
struct sensor_device {
    struct work_struct data_work;
    void __iomem *reg_base;
    u32 *data_buffer;
};

/* IRQ handler (atomic context - cannot sleep) */
static irqreturn_t sensor_irq_handler(int irq, void *dev_id)
{
    struct sensor_device *sensor = dev_id;
    u32 status;

    /* Read status register */
    status = readl(sensor->reg_base + SENSOR_STATUS);

    /* Acknowledge interrupt at hardware level */
    writel(status | IRQ_ACK, sensor->reg_base + SENSOR_STATUS);

    /* Schedule work for heavy processing */
    schedule_work(&sensor->data_work);

    return IRQ_HANDLED;
}

/* Work handler (process context - can sleep) */
static void sensor_data_work_handler(struct work_struct *work)
{
    struct sensor_device *sensor = container_of(work,
                                    struct sensor_device, data_work);
    int i;

    /* This can sleep - process context! */
    for (i = 0; i < 100; i++) {
        sensor->data_buffer[i] = readl(sensor->reg_base + DATA_REG);
        /* Can use sleeping functions here */
        msleep(1);  /* OK - we're in process context */
    }

    /* Process data */
    printk("Sensor data collected\n");
}
```

**Example: Delayed work for periodic polling**

```c
struct gpio_poll_device {
    struct delayed_work poll_work;
    struct gpio_desc *gpio;
    unsigned int poll_interval_ms;
};

static void gpio_poll_handler(struct work_struct *work)
{
    struct delayed_work *dwork = to_delayed_work(work);
    struct gpio_poll_device *dev = container_of(dwork,
                                    struct gpio_poll_device, poll_work);
    int value;

    /* Read GPIO value */
    value = gpiod_get_value(dev->gpio);
    printk("GPIO value: %d\n", value);

    /* Reschedule work for next poll */
    schedule_delayed_work(&dev->poll_work,
                         msecs_to_jiffies(dev->poll_interval_ms));
}

static int gpio_poll_start(struct gpio_poll_device *dev)
{
    dev->poll_interval_ms = 500;  /* Poll every 500ms */

    /* Start polling */
    schedule_delayed_work(&dev->poll_work,
                         msecs_to_jiffies(dev->poll_interval_ms));
    return 0;
}
```

### Custom Workqueues

**When to create your own workqueue:**

1. **High priority work** - Need dedicated worker threads
2. **CPU-intensive operations** - Don't want to hog system workqueue
3. **Memory reclaim path** - Need guaranteed forward progress
4. **Fine-grained control** - Custom concurrency requirements

**Creating custom workqueues:**

**Legacy API (still supported):**

```c
/* Create single-threaded workqueue (one worker thread) */
struct workqueue_struct *create_singlethread_workqueue(const char *name);

/* Create workqueue with worker per CPU */
struct workqueue_struct *create_workqueue(const char *name);

/* Destroy workqueue */
void destroy_workqueue(struct workqueue_struct *wq);
```

**Modern API (Concurrency-Managed Workqueues):**

```c
/* Allocate workqueue with flags */
struct workqueue_struct *alloc_workqueue(const char *fmt,
                                         unsigned int flags,
                                         int max_active, ...);

/* Allocate ordered workqueue (processes work one-by-one) */
struct workqueue_struct *alloc_ordered_workqueue(const char *fmt,
                                                 unsigned int flags, ...);
```

**Workqueue Flags:**

| Flag | Description | Use Case |
| --- | --- | --- |
| `WQ_UNBOUND` | Not bound to specific CPU | Long operations, scheduler can balance load |
| `WQ_FREEZABLE` | Freeze during system suspend | File systems, prevent corruption |
| `WQ_MEM_RECLAIM` | Guaranteed rescuer thread | Memory reclaim path, avoid deadlock |
| `WQ_HIGHPRI` | High priority execution | Low-latency requirements |
| `WQ_CPU_INTENSIVE` | CPU-intensive work | Crypto, heavy computation |

**Example: Creating custom workqueue**

```c
struct dma_device {
    struct workqueue_struct *dma_wq;
    struct work_struct tx_work;
    struct work_struct rx_work;
};

static int dma_probe(struct platform_device *pdev)
{
    struct dma_device *dma;

    dma = devm_kzalloc(&pdev->dev, sizeof(*dma), GFP_KERNEL);
    if (!dma)
        return -ENOMEM;

    /* Create single-threaded workqueue for DMA operations */
    dma->dma_wq = create_singlethread_workqueue("dma_workqueue");
    if (!dma->dma_wq) {
        dev_err(&pdev->dev, "Failed to create workqueue\n");
        return -ENOMEM;
    }

    INIT_WORK(&dma->tx_work, dma_tx_handler);
    INIT_WORK(&dma->rx_work, dma_rx_handler);

    platform_set_drvdata(pdev, dma);
    return 0;
}

static int dma_remove(struct platform_device *pdev)
{
    struct dma_device *dma = platform_get_drvdata(pdev);

    /* Destroy workqueue - waits for pending work to complete */
    destroy_workqueue(dma->dma_wq);

    return 0;
}
```

**Example: Using modern alloc_workqueue**

```c
struct crypto_device {
    struct workqueue_struct *crypto_wq;
    struct work_struct encrypt_work;
};

static int crypto_init(void)
{
    struct crypto_device *crypto;

    crypto = kzalloc(sizeof(*crypto), GFP_KERNEL);
    if (!crypto)
        return -ENOMEM;

    /* Create high-priority, CPU-intensive workqueue */
    crypto->crypto_wq = alloc_workqueue("crypto_wq",
                                        WQ_HIGHPRI | WQ_CPU_INTENSIVE,
                                        0);  /* max_active = 0 means default */
    if (!crypto->crypto_wq) {
        kfree(crypto);
        return -ENOMEM;
    }

    INIT_WORK(&crypto->encrypt_work, crypto_encrypt_handler);

    return 0;
}
```

### Queuing Work to Custom Workqueues

```c
/* Queue work to specific workqueue */
bool queue_work(struct workqueue_struct *wq, struct work_struct *work);

/* Queue work to specific CPU on workqueue */
bool queue_work_on(int cpu, struct workqueue_struct *wq,
                   struct work_struct *work);

/* Queue delayed work */
bool queue_delayed_work(struct workqueue_struct *wq,
                       struct delayed_work *dwork,
                       unsigned long delay);

/* Queue delayed work on specific CPU */
bool queue_delayed_work_on(int cpu, struct workqueue_struct *wq,
                          struct delayed_work *dwork,
                          unsigned long delay);
```

**Example: Using custom workqueue**

```c
static irqreturn_t device_irq_handler(int irq, void *dev_id)
{
    struct dma_device *dma = dev_id;
    u32 status;

    status = device_read_status(dma);

    if (status & TX_COMPLETE) {
        /* Queue TX work to our custom workqueue */
        queue_work(dma->dma_wq, &dma->tx_work);
    }

    if (status & RX_READY) {
        /* Queue RX work with 10ms delay */
        queue_delayed_work(dma->dma_wq, &dma->rx_work,
                          msecs_to_jiffies(10));
    }

    return IRQ_HANDLED;
}
```

### Canceling and Flushing Work

**Canceling work:**

```c
/* Cancel work (asynchronous) */
bool cancel_work_sync(struct work_struct *work);

/* Cancel delayed work (asynchronous) */
bool cancel_delayed_work(struct delayed_work *dwork);

/* Cancel delayed work (synchronous) */
bool cancel_delayed_work_sync(struct delayed_work *dwork);
```

**Return values:**

- `true`: Work was pending and has been canceled
- `false`: Work was not pending

**Flushing work:**

```c
/* Wait for specific work to complete */
bool flush_work(struct work_struct *work);

/* Flush all work in workqueue */
void flush_workqueue(struct workqueue_struct *wq);

/* Flush system workqueue */
void flush_scheduled_work(void);
```

**Important notes:**

1. **`cancel_work_sync()` and `cancel_delayed_work_sync()`:**
    - **Synchronous** - wait for work to complete
    - **Safe** - guarantee work won't be running after return
    - **Cannot** be called from the work handler itself (deadlock!)
2. **`cancel_delayed_work()`:**
    - **Asynchronous** - return immediately
    - Work might still be executing
    - Must flush workqueue afterward

**Example: Safe work cancellation**

```c
struct poll_device {
    struct delayed_work poll_work;
    struct workqueue_struct *wq;
    bool running;
};

static void poll_work_handler(struct work_struct *work)
{
    struct delayed_work *dwork = to_delayed_work(work);
    struct poll_device *dev = container_of(dwork,
                                struct poll_device, poll_work);

    if (!dev->running)
        return;  /* Stopped */

    /* Do polling work */
    printk("Polling device...\n");

    /* Reschedule if still running */
    if (dev->running) {
        queue_delayed_work(dev->wq, &dev->poll_work, HZ);
    }
}

static void poll_stop(struct poll_device *dev)
{
    /* Signal work to stop */
    dev->running = false;

    /* Cancel work - work won't reschedule after this */
    cancel_delayed_work_sync(&dev->poll_work);

    printk("Polling stopped\n");
}

static void poll_cleanup(struct poll_device *dev)
{
    /* Stop polling */
    poll_stop(dev);

    /* Flush any remaining work */
    flush_workqueue(dev->wq);

    /* Destroy workqueue */
    destroy_workqueue(dev->wq);
}
```

**Example: Asynchronous cancellation with flush**

```c
static int device_suspend(struct device *dev)
{
    struct my_device *mdev = dev_get_drvdata(dev);

    /* Cancel delayed work (asynchronous) */
    if (!cancel_delayed_work(&mdev->delayed_work)) {
        /* Work was running or already completed */
        /* Flush to ensure completion */
        flush_workqueue(mdev->wq);
    }

    return 0;
}
```

### Complete Real-World Example: Network Driver Bottom Half

```c
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>

#define RX_BUFFER_SIZE 1024
#define MIN_PACKETS_TO_PROCESS 10

struct net_private {
    struct net_device *ndev;
    void __iomem *reg_base;

    /* Workqueue for RX processing */
    struct workqueue_struct *rx_wq;
    struct work_struct rx_work;

    /* Data */
    spinlock_t lock;
    int rx_packet_count;
    u8 rx_buffer[RX_BUFFER_SIZE];
};

/* Work handler - Process received packets */
static void rx_work_handler(struct work_struct *work)
{
    struct net_private *priv = container_of(work,
                                struct net_private, rx_work);
    int packets_to_process;
    int i;
    unsigned long flags;

    /* Get packet count atomically */
    spin_lock_irqsave(&priv->lock, flags);
    packets_to_process = priv->rx_packet_count;
    priv->rx_packet_count = 0;  /* Reset counter */
    spin_unlock_irqrestore(&priv->lock, flags);

    /* Process packets - can sleep here */
    printk("Processing %d packets\n", packets_to_process);

    for (i = 0; i < packets_to_process; i++) {
        /* Read packet from device (can use sleeping functions) */
        u32 packet_len = readl(priv->reg_base + RX_LEN_REG);

        /* Allocate SKB */
        struct sk_buff *skb = dev_alloc_skb(packet_len);
        if (!skb) {
            printk("Failed to allocate SKB\n");
            continue;
        }

        /* Copy data to SKB */
        memcpy(skb->data, priv->rx_buffer, packet_len);
        skb_put(skb, packet_len);

        /* Pass to network stack */
        skb->dev = priv->ndev;
        skb->protocol = eth_type_trans(skb, priv->ndev);
        netif_rx(skb);

        /* Update statistics */
        priv->ndev->stats.rx_packets++;
        priv->ndev->stats.rx_bytes += packet_len;
    }

    /* Re-enable device interrupts */
    writel(RX_IRQ_ENABLE, priv->reg_base + IRQ_ENABLE_REG);
}

/* IRQ Handler - Top half (hard IRQ) */
static irqreturn_t net_irq_handler(int irq, void *dev_id)
{
    struct net_private *priv = dev_id;
    u32 status;
    unsigned long flags;

    /* Read and acknowledge interrupt */
    status = readl(priv->reg_base + IRQ_STATUS_REG);
    writel(status, priv->reg_base + IRQ_STATUS_REG);

    if (status & RX_IRQ) {
        /* Disable RX interrupts temporarily */
        writel(0, priv->reg_base + IRQ_ENABLE_REG);

        /* Count received packet */
        spin_lock_irqsave(&priv->lock, flags);
        priv->rx_packet_count++;
        spin_unlock_irqrestore(&priv->lock, flags);

        /* Schedule work if we have enough packets */
        if (priv->rx_packet_count >= MIN_PACKETS_TO_PROCESS) {
            queue_work(priv->rx_wq, &priv->rx_work);
        }
    }

    return IRQ_HANDLED;
}

/* Driver probe */
static int net_probe(struct platform_device *pdev)
{
    struct net_private *priv;
    struct net_device *ndev;
    int ret;

    /* Allocate network device */
    ndev = alloc_etherdev(sizeof(struct net_private));
    if (!ndev)
        return -ENOMEM;

    priv = netdev_priv(ndev);
    priv->ndev = ndev;

    /* Create dedicated workqueue */
    priv->rx_wq = create_singlethread_workqueue("net_rx_wq");
    if (!priv->rx_wq) {
        ret = -ENOMEM;
        goto err_free_netdev;
    }

    /* Initialize work */
    INIT_WORK(&priv->rx_work, rx_work_handler);
    spin_lock_init(&priv->lock);

    /* Map registers */
    priv->reg_base = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(priv->reg_base)) {
        ret = PTR_ERR(priv->reg_base);
        goto err_destroy_wq;
    }

    /* Request IRQ */
    ret = devm_request_irq(&pdev->dev, platform_get_irq(pdev, 0),
                          net_irq_handler, 0, dev_name(&pdev->dev), priv);
    if (ret)
        goto err_destroy_wq;

    /* Register network device */
    ret = register_netdev(ndev);
    if (ret)
        goto err_destroy_wq;

    platform_set_drvdata(pdev, ndev);
    dev_info(&pdev->dev, "Network device registered\n");

    return 0;

err_destroy_wq:
    destroy_workqueue(priv->rx_wq);
err_free_netdev:
    free_netdev(ndev);
    return ret;
}

/* Driver remove */
static int net_remove(struct platform_device *pdev)
{
    struct net_device *ndev = platform_get_drvdata(pdev);
    struct net_private *priv = netdev_priv(ndev);

    /* Unregister network device */
    unregister_netdev(ndev);

    /* Cancel pending work */
    cancel_work_sync(&priv->rx_work);

    /* Destroy workqueue - waits for pending work */
    destroy_workqueue(priv->rx_wq);

    /* Free network device */
    free_netdev(ndev);

    dev_info(&pdev->dev, "Network device removed\n");
    return 0;
}

static struct platform_driver net_driver = {
    .probe = net_probe,
    .remove = net_remove,
    .driver = {
        .name = "example_net",
    },
};

module_platform_driver(net_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Expert");
MODULE_DESCRIPTION("Network driver with workqueue bottom half");
```

### Workqueue Best Practices

**✅ DO:**

1. **Use system workqueue for simple tasks:**
    
    ```c
    schedule_work(&simple_work);  /* Good for most cases */
    ```
    
2. **Create custom workqueue for:**
    - High-priority operations
    - CPU-intensive work
    - Memory reclaim path
3. **Always cancel work in cleanup:**
    
    ```c
    cancel_work_sync(&my_work);
    destroy_workqueue(my_wq);
    ```
    
4. **Use `container_of()` to access driver data:**
    
    ```c
    struct my_dev *dev = container_of(work, struct my_dev, work);
    ```
    
5. **Check return values:**
    
    ```c
    if (!schedule_work(&my_work))
        pr_debug("Work already queued\n");
    ```
    

**❌ DON'T:**

1. **Don't call `cancel_*_sync()` from work handler:**
    
    ```c
    void work_handler(struct work_struct *work) {
        cancel_work_sync(work);  /* DEADLOCK! */
    }
    ```
    
2. **Don't assume immediate execution:**
    
    ```c
    schedule_work(&my_work);
    /* Work might not run yet! */
    access_work_results();  /* WRONG! */
    
    ```
    
3. **Don't forget to destroy workqueue:**
    
    ```c
    static void cleanup(void) {
        /* Missing destroy_workqueue() = memory leak! */
    }
    ```
    
4. **Don't queue unbounded work:**
    
    ```c
    /* Bad - can flood workqueue */
    for (i = 0; i < 1000000; i++)
        schedule_work(&my_work);
    ```
    

### When to Use Workqueue

**✅ Use workqueue when:**

- Need to sleep or call blocking functions
- Heavy processing required (> 100 μs)
- I/O operations needed
- Memory allocation with `GFP_KERNEL`
- Can tolerate scheduling delays

**❌ Don't use workqueue when:**

- Need guaranteed immediate execution
- In atomic context and cannot defer
- Very time-critical operations
- Simple, fast operations (use tasklets instead)

**Comparison with alternatives:**

| Requirement | Use This |
| --- | --- |
| Can sleep | **Workqueue** |
| Atomic, no sleep | **Tasklet** |
| Time-based execution | **Timer** |
| Must run on specific CPU | **Tasklet** or **per-CPU workqueue** |
| Long operations | **Workqueue** (dedicated) |
| Fast operations | **Tasklet** |

---