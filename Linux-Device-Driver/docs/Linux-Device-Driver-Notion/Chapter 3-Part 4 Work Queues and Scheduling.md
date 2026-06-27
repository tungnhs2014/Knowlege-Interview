# Part 4. Work Queues and Scheduling

## 3.3.2 Tasklets

### What is a Tasklet?

**Tasklet = Softly deferred interrupt work (atomic context, built on softIRQs)**

A tasklet is a bottom-half mechanism built on top of softIRQs, designed for **fast, atomic operations** that cannot sleep.

**Key characteristics:**

- ✅ Runs in **atomic context** (like IRQ handler)
- ✅ **Cannot sleep** or call blocking functions
- ✅ **Very fast** - minimal overhead
- ✅ **Same tasklet never runs concurrently** (exclusive execution)
- ✅ Different tasklets can run on different CPUs
- ⚠️ **Lower priority** than hardware IRQs
- ⚠️ **Legacy mechanism** - workqueues preferred for new code

**Architecture:**

```
Tasklet Execution Model:

Hardware IRQ
    ↓
IRQ Handler (top-half)
    │
    ├─→ Minimal work
    ├─→ Acknowledge hardware
    └─→ Schedule tasklet
            ↓
    ┌─────────────────┐
    │  Tasklet Queue  │
    │  (per-CPU)      │
    └─────────────────┘
            ↓
    Softirq execution
    (IRQs enabled)
            ↓
    Tasklet handler runs
    (atomic context)
```

**Tasklet vs Workqueue:**

```
Tasklet:                           Workqueue:
├─ Atomic context                  ├─ Process context
├─ Cannot sleep                    ├─ Can sleep
├─ Fast execution                  ├─ Can take time
├─ No scheduling                   ├─ Scheduled by kernel
└─ IRQs enabled                    └─ Full preemption
```

### Tasklet Data Structure

```c
/* Defined in include/linux/interrupt.h */
struct tasklet_struct {
    struct tasklet_struct *next;  /* Next tasklet in list */
    unsigned long state;          /* Tasklet state */
    atomic_t count;              /* Reference count (enable/disable) */
    void (*func)(unsigned long);  /* Tasklet handler function */
    unsigned long data;          /* Argument passed to func */
};
```

**State flags:**

```c
#define TASKLET_STATE_SCHED  0  /* Tasklet is scheduled */
#define TASKLET_STATE_RUN    1  /* Tasklet is running */
```

**Important:** The same tasklet cannot run on multiple CPUs simultaneously. If it's already running on one CPU and gets scheduled on another, it will wait.

### Tasklet Initialization

**Method 1: Static Declaration**

```c
/* Declare and initialize tasklet */
void my_tasklet_handler(unsigned long data);

/* Normal priority tasklet */
DECLARE_TASKLET(my_tasklet, my_tasklet_handler, data);

/* Initially disabled tasklet */
DECLARE_TASKLET_DISABLED(my_tasklet, my_tasklet_handler, data);
```

**Method 2: Dynamic Initialization**

```c
struct tasklet_struct my_tasklet;

/* Initialize tasklet */
tasklet_init(&my_tasklet, my_tasklet_handler, data);
```

**Handler function prototype:**

```c
void tasklet_handler(unsigned long data)
{
    /* Your code here - CANNOT SLEEP! */
    /* data is the argument passed during initialization */
}
```

**Example: Basic tasklet setup**

```c
struct uart_device {
    struct tasklet_struct tx_tasklet;
    struct tasklet_struct rx_tasklet;
    spinlock_t lock;
    u8 *tx_buffer;
    u8 *rx_buffer;
    void __iomem *base;
};

/* TX Tasklet handler */
static void uart_tx_tasklet_handler(unsigned long data)
{
    struct uart_device *uart = (struct uart_device *)data;
    unsigned long flags;

    spin_lock_irqsave(&uart->lock, flags);

    /* Process TX - cannot sleep! */
    if (uart_tx_ready(uart->base)) {
        uart_send_char(uart->base, *uart->tx_buffer++);
    }

    spin_unlock_irqrestore(&uart->lock, flags);
}

/* RX Tasklet handler */
static void uart_rx_tasklet_handler(unsigned long data)
{
    struct uart_device *uart = (struct uart_device *)data;
    unsigned long flags;
    u8 ch;

    spin_lock_irqsave(&uart->lock, flags);

    /* Process RX - cannot sleep! */
    while (uart_rx_ready(uart->base)) {
        ch = uart_read_char(uart->base);
        *uart->rx_buffer++ = ch;
    }

    spin_unlock_irqrestore(&uart->lock, flags);
}

static int uart_probe(struct platform_device *pdev)
{
    struct uart_device *uart;

    uart = devm_kzalloc(&pdev->dev, sizeof(*uart), GFP_KERNEL);
    if (!uart)
        return -ENOMEM;

    spin_lock_init(&uart->lock);

    /* Initialize tasklets */
    tasklet_init(&uart->tx_tasklet, uart_tx_tasklet_handler,
                 (unsigned long)uart);
    tasklet_init(&uart->rx_tasklet, uart_rx_tasklet_handler,
                 (unsigned long)uart);

    platform_set_drvdata(pdev, uart);
    return 0;
}
```

### Scheduling Tasklets

**Scheduling functions:**

```c
/* Schedule normal priority tasklet */
void tasklet_schedule(struct tasklet_struct *t);

/* Schedule high priority tasklet */
void tasklet_hi_schedule(struct tasklet_struct *t);
```

**Priority levels:**

```
High Priority Tasklets (HI_SOFTIRQ):
├─ Run before normal tasklets
├─ Use sparingly (increases system latency)
└─ For truly low-latency requirements

Normal Priority Tasklets (TASKLET_SOFTIRQ):
├─ Run after high priority
├─ Default choice for most drivers
└─ Better system performance
```

**Scheduling behavior:**

- If tasklet already scheduled but not running: **No effect** (runs once)
- If tasklet currently running: **Scheduled again** (runs again after completion)
- Tasklet can reschedule itself

**Example: Scheduling from IRQ handler**

```c
static irqreturn_t uart_irq_handler(int irq, void *dev_id)
{
    struct uart_device *uart = dev_id;
    u32 status;

    /* Read interrupt status */
    status = readl(uart->base + UART_STATUS);

    /* Acknowledge interrupt */
    writel(status, uart->base + UART_STATUS);

    /* Schedule tasklets based on interrupt type */
    if (status & UART_TX_IRQ) {
        tasklet_schedule(&uart->tx_tasklet);
    }

    if (status & UART_RX_IRQ) {
        tasklet_schedule(&uart->rx_tasklet);
    }

    return IRQ_HANDLED;
}
```

**Example: High priority tasklet for critical timing**

```c
struct critical_device {
    struct tasklet_struct critical_tasklet;
    ktime_t timestamp;
};

static void critical_tasklet_handler(unsigned long data)
{
    struct critical_device *dev = (struct critical_device *)data;
    ktime_t now = ktime_get();
    s64 latency_us;

    /* Calculate scheduling latency */
    latency_us = ktime_us_delta(now, dev->timestamp);

    pr_info("Tasklet latency: %lld us\n", latency_us);

    /* Process critical data */
    process_time_critical_data(dev);
}

static irqreturn_t critical_irq_handler(int irq, void *dev_id)
{
    struct critical_device *dev = dev_id;

    /* Record timestamp */
    dev->timestamp = ktime_get();

    /* Schedule high-priority tasklet */
    tasklet_hi_schedule(&dev->critical_tasklet);

    return IRQ_HANDLED;
}
```

### Enabling and Disabling Tasklets

**Control functions:**

```c
/* Disable tasklet (increment count) */
void tasklet_disable(struct tasklet_struct *t);

/* Disable tasklet without waiting */
void tasklet_disable_nosync(struct tasklet_struct *t);

/* Enable tasklet (decrement count) */
void tasklet_enable(struct tasklet_struct *t);
```

**How it works:**

```c
struct tasklet_struct {
    atomic_t count;  /* 0 = enabled, >0 = disabled */
};

/* Tasklet runs only when count == 0 */
```

**Disable/enable behavior:**

```
tasklet_disable() calls:
├─ Increment count
├─ Wait for tasklet to complete (if running)
└─ Return (tasklet won't run until enabled)

tasklet_disable_nosync():
├─ Increment count
└─ Return immediately (might still be running!)

tasklet_enable():
├─ Decrement count
└─ If count reaches 0, tasklet can run again
```

**Important:** Tasklets can be disabled multiple times. Each `tasklet_disable()` must be matched with `tasklet_enable()`.

**Example: Temporarily disabling tasklet**

```c
static int device_suspend(struct device *dev)
{
    struct uart_device *uart = dev_get_drvdata(dev);

    /* Disable tasklets during suspend */
    tasklet_disable(&uart->tx_tasklet);
    tasklet_disable(&uart->rx_tasklet);

    /* Now safe to power down device */
    device_power_off(uart);

    return 0;
}

static int device_resume(struct device *dev)
{
    struct uart_device *uart = dev_get_drvdata(dev);

    /* Power on device */
    device_power_on(uart);

    /* Re-enable tasklets */
    tasklet_enable(&uart->tx_tasklet);
    tasklet_enable(&uart->rx_tasklet);

    return 0;
}
```

### Killing Tasklets

```c
/* Kill tasklet (prevent from running, wait if executing) */
void tasklet_kill(struct tasklet_struct *t);
```

**What `tasklet_kill()` does:**

1. Prevent tasklet from being scheduled again
2. Wait for currently executing tasklet to complete
3. Remove from schedule queue

**Important:** If tasklet reschedules itself, you must prevent rescheduling before calling `tasklet_kill()`.

**Example: Proper cleanup**

```c
struct network_device {
    struct tasklet_struct rx_tasklet;
    bool running;
};

static void rx_tasklet_handler(unsigned long data)
{
    struct network_device *ndev = (struct network_device *)data;

    /* Check if still running */
    if (!ndev->running)
        return;

    /* Process packets */
    process_rx_packets(ndev);

    /* Reschedule if more packets and still running */
    if (ndev->running && more_packets_available(ndev)) {
        tasklet_schedule(&ndev->rx_tasklet);
    }
}

static void network_shutdown(struct network_device *ndev)
{
    /* Signal tasklet to stop rescheduling */
    ndev->running = false;

    /* Kill tasklet - waits for completion */
    tasklet_kill(&ndev->rx_tasklet);

    pr_info("Network device shut down\n");
}
```

### Complete Real-World Example: SPI Flash Driver

```c
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/interrupt.h>

#define FLASH_PAGE_SIZE 256
#define FLASH_CMD_READ  0x03
#define FLASH_CMD_WRITE 0x02

struct spi_flash {
    struct spi_device *spi;
    struct tasklet_struct read_tasklet;
    struct tasklet_struct write_tasklet;

    spinlock_t lock;
    u8 *read_buffer;
    u8 *write_buffer;
    size_t transfer_len;

    int gpio_irq;
    void (*completion_callback)(void *);
    void *callback_data;
};

/* Read tasklet - process read completion */
static void spi_flash_read_tasklet(unsigned long data)
{
    struct spi_flash *flash = (struct spi_flash *)data;
    unsigned long flags;

    spin_lock_irqsave(&flash->lock, flags);

    /* Process read data - cannot sleep! */
    pr_info("Read completed: %zu bytes\n", flash->transfer_len);

    /* Notify completion */
    if (flash->completion_callback) {
        flash->completion_callback(flash->callback_data);
    }

    spin_unlock_irqrestore(&flash->lock, flags);
}

/* Write tasklet - process write completion */
static void spi_flash_write_tasklet(unsigned long data)
{
    struct spi_flash *flash = (struct spi_flash *)data;
    unsigned long flags;

    spin_lock_irqsave(&flash->lock, flags);

    /* Verify write - atomic operation */
    pr_info("Write completed: %zu bytes\n", flash->transfer_len);

    /* Check status */
    u8 status = spi_flash_read_status(flash->spi);
    if (status & FLASH_ERROR_BIT) {
        pr_err("Flash write error detected\n");
    }

    /* Notify completion */
    if (flash->completion_callback) {
        flash->completion_callback(flash->callback_data);
    }

    spin_unlock_irqrestore(&flash->lock, flags);
}

/* GPIO IRQ handler - flash operation completed */
static irqreturn_t spi_flash_irq_handler(int irq, void *dev_id)
{
    struct spi_flash *flash = dev_id;

    /* Determine operation type from hardware */
    u8 op_type = gpio_get_value(flash->gpio_status);

    /* Schedule appropriate tasklet */
    if (op_type == FLASH_OP_READ) {
        tasklet_schedule(&flash->read_tasklet);
    } else if (op_type == FLASH_OP_WRITE) {
        tasklet_schedule(&flash->write_tasklet);
    }

    return IRQ_HANDLED;
}

/* Initiate read operation */
static int spi_flash_read(struct spi_flash *flash, u32 addr,
                         u8 *buf, size_t len)
{
    struct spi_transfer xfer[2];
    struct spi_message msg;
    u8 cmd[4];
    int ret;

    /* Prepare command */
    cmd[0] = FLASH_CMD_READ;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    /* Setup SPI transfer */
    memset(xfer, 0, sizeof(xfer));

    /* Command phase */
    xfer[0].tx_buf = cmd;
    xfer[0].len = 4;

    /* Data phase */
    xfer[1].rx_buf = buf;
    xfer[1].len = len;

    spi_message_init(&msg);
    spi_message_add_tail(&xfer[0], &msg);
    spi_message_add_tail(&xfer[1], &msg);

    /* Store for tasklet */
    flash->read_buffer = buf;
    flash->transfer_len = len;

    /* Execute SPI transaction (synchronous) */
    ret = spi_sync(flash->spi, &msg);
    if (ret) {
        pr_err("SPI read failed: %d\n", ret);
        return ret;
    }

    /* Completion will be signaled via IRQ + tasklet */
    return 0;
}

/* Driver probe */
static int spi_flash_probe(struct spi_device *spi)
{
    struct spi_flash *flash;
    int ret;

    flash = devm_kzalloc(&spi->dev, sizeof(*flash), GFP_KERNEL);
    if (!flash)
        return -ENOMEM;

    flash->spi = spi;
    spin_lock_init(&flash->lock);

    /* Initialize tasklets */
    tasklet_init(&flash->read_tasklet, spi_flash_read_tasklet,
                 (unsigned long)flash);
    tasklet_init(&flash->write_tasklet, spi_flash_write_tasklet,
                 (unsigned long)flash);

    /* Request IRQ */
    flash->gpio_irq = gpiod_to_irq(flash_ready_gpio);
    ret = devm_request_irq(&spi->dev, flash->gpio_irq,
                          spi_flash_irq_handler,
                          IRQF_TRIGGER_RISING,
                          "spi_flash", flash);
    if (ret) {
        dev_err(&spi->dev, "Failed to request IRQ\n");
        return ret;
    }

    spi_set_drvdata(spi, flash);
    dev_info(&spi->dev, "SPI flash probed successfully\n");

    return 0;
}

/* Driver remove */
static int spi_flash_remove(struct spi_device *spi)
{
    struct spi_flash *flash = spi_get_drvdata(spi);

    /* Kill tasklets - wait for completion */
    tasklet_kill(&flash->read_tasklet);
    tasklet_kill(&flash->write_tasklet);

    dev_info(&spi->dev, "SPI flash removed\n");
    return 0;
}

static struct spi_driver spi_flash_driver = {
    .driver = {
        .name = "spi_flash_tasklet",
    },
    .probe = spi_flash_probe,
    .remove = spi_flash_remove,
};

module_spi_driver(spi_flash_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Embedded Linux Expert");
MODULE_DESCRIPTION("SPI Flash driver with tasklet bottom-half");
```

### Tasklet Synchronization

**Locking between tasklets:**

```
Same tasklet:
├─ Never runs concurrently (guaranteed by kernel)
└─ No locking needed between instances

Different tasklets:
├─ Can run concurrently on different CPUs
└─ Use spinlock (not mutex!) for shared data
```

**Example: Shared data between tasklets**

```c
struct device_data {
    struct tasklet_struct tasklet_a;
    struct tasklet_struct tasklet_b;
    spinlock_t data_lock;
    int shared_counter;
};

static void tasklet_a_handler(unsigned long data)
{
    struct device_data *dev = (struct device_data *)data;

    /* Must use spinlock - tasklet_b might run on another CPU */
    spin_lock(&dev->data_lock);
    dev->shared_counter++;
    spin_unlock(&dev->data_lock);
}

static void tasklet_b_handler(unsigned long data)
{
    struct device_data *dev = (struct device_data *)data;

    /* Same lock protects shared data */
    spin_lock(&dev->data_lock);
    dev->shared_counter--;
    spin_unlock(&dev->data_lock);
}
```

### Tasklet vs Workqueue Comparison

| Feature | Tasklet | Workqueue |
| --- | --- | --- |
| **Execution Context** | Softirq (atomic) | Process context |
| **Can Sleep?** | ❌ No | ✅ Yes |
| **Can use mutex?** | ❌ No | ✅ Yes |
| **Execution Speed** | ⚡ Very fast | 🐢 Slower (scheduled) |
| **Use GFP_KERNEL?** | ❌ No (GFP_ATOMIC only) | ✅ Yes |
| **Same instance concurrency** | ❌ Never concurrent | ✅ Can run multiple times |
| **Suitable for** | Fast, atomic operations | Long, blocking operations |
| **Current Status** | 📦 Legacy (but still used) | ✅ Preferred for new code |

### When to Use Tasklets

**✅ Use tasklets when:**

- Very fast processing needed (< 100 μs)
- Atomic context required
- No sleeping/blocking needed
- Lower latency than workqueue acceptable
- Maintaining legacy code

**❌ Don't use tasklets when:**

- Need to sleep or block
- Long processing time (> 100 μs)
- Need to allocate memory with GFP_KERNEL
- Writing new driver (use workqueue instead)

**Modern recommendation:** For new drivers, prefer **workqueues** over tasklets. Tasklets are considered legacy, though still widely used in existing drivers.

### Tasklet Best Practices

**✅ DO:**

1. **Keep tasklet handler fast:**
    
    ```c
    void fast_tasklet(unsigned long data) {
        /* Quick operations only */
        process_minimal_data();
    }
    ```
    
2. **Use spinlock for shared data:**
    
    ```c
    spin_lock(&dev->lock);
    shared_data++;
    spin_unlock(&dev->lock);
    ```
    
3. **Kill tasklets in cleanup:**
    
    ```c
    tasklet_kill(&my_tasklet);
    ```
    
4. **Check return from schedule:**
    
    ```c
    tasklet_schedule(&my_tasklet);  /* Safe to call multiple times */
    ```
    

**❌ DON'T:**

1. **Don't sleep in tasklet:**
    
    ```c
    void bad_tasklet(unsigned long data) {
        msleep(10);  /* WRONG! Cannot sleep! */
    }
    ```
    
2. **Don't use mutex:**
    
    ```c
    void bad_tasklet(unsigned long data) {
        mutex_lock(&my_mutex);  /* WRONG! Use spinlock! */
    }
    ```
    
3. **Don't take too long:**
    
    ```c
    void bad_tasklet(unsigned long data) {
        for (i = 0; i < 1000000; i++)  /* WRONG! Too slow! */
            process_data();
    }
    ```
    

---