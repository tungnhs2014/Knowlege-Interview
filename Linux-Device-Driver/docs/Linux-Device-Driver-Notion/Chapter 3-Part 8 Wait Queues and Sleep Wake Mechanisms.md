# Part 8. Wait Queues and Sleep/Wake Mechanisms

## 3.4.4 Completion Framework

### What is Completion?

**Completion = Wait for a specific event to complete (one-time synchronization)**

```
Completion vs Wait Queue:

Wait Queue:                    Completion:
├─ General purpose            ├─ Specific purpose
├─ Multiple events            ├─ Single event
├─ Reusable condition         ├─ One-shot (complete once)
├─ Any condition              ├─ "Task completed" signal
└─ Flexible                   └─ Simpler for common case
```

**Common use cases:**

- Waiting for DMA transfer to complete
- Waiting for I/O operation to finish
- Synchronizing with kthread termination
- Device initialization completion

**Architecture:**

```c
/* Defined in include/linux/completion.h */
struct completion {
    unsigned int done;           /* Completion counter */
    wait_queue_head_t wait;     /* Wait queue */
};
```

### Completion API

**Initialization:**

```c
/* Static declaration */
static DECLARE_COMPLETION(my_completion);

/* Dynamic initialization */
struct completion my_completion;
init_completion(&my_completion);

/* Re-initialize completion */
reinit_completion(&my_completion);
```

**Waiting for completion:**

```c
/* Wait (uninterruptible) */
void wait_for_completion(struct completion *c);

/* Wait (interruptible) */
int wait_for_completion_interruptible(struct completion *c);

/* Wait (killable) */
int wait_for_completion_killable(struct completion *c);

/* Wait with timeout */
unsigned long wait_for_completion_timeout(struct completion *c,
                                         unsigned long timeout);

/* Wait interruptible with timeout */
long wait_for_completion_interruptible_timeout(struct completion *c,
                                               unsigned long timeout);
```

**Return values:**

| Function | Success | Interrupted | Timeout |
| --- | --- | --- | --- |
| `wait_for_completion_interruptible()` | 0 | -ERESTARTSYS | N/A |
| `wait_for_completion_killable()` | 0 | -ERESTARTSYS | N/A |
| `wait_for_completion_timeout()` | Remaining jiffies (≥1) | N/A | 0 |
| `wait_for_completion_interruptible_timeout()` | Remaining jiffies (≥1) | -ERESTARTSYS | 0 |

**Signaling completion:**

```c
/* Complete - wake up ONE waiter */
void complete(struct completion *c);

/* Complete all - wake up ALL waiters */
void complete_all(struct completion *c);
```

**Important notes:**

1. **`complete()` can be called from any context** (including IRQ handlers)
    - Uses `spin_lock_irqsave()` internally - safe for IRQ context
2. **`wait_for_completion()` can only be called from process context**
    - May sleep
3. **Completion is one-shot by default**
    - After `complete()`, next `wait_for_completion()` returns immediately
    - Use `reinit_completion()` to reuse

### Basic Completion Example

```c
#include <linux/module.h>
#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/delay.h>

static DECLARE_COMPLETION(data_ready);
static struct task_struct *worker_thread;

/* Worker thread - does work and signals completion */
static int worker_fn(void *data)
{
    printk("Worker: starting work...\n");
    msleep(3000);  /* Simulate work */

    printk("Worker: work complete, signaling...\n");
    complete(&data_ready);  /* Signal completion */

    return 0;
}

static int __init completion_example_init(void)
{
    /* Start worker thread */
    worker_thread = kthread_run(worker_fn, NULL, "completion_worker");
    if (IS_ERR(worker_thread))
        return PTR_ERR(worker_thread);

    printk("Main: waiting for worker to complete...\n");

    /* Wait for completion */
    wait_for_completion(&data_ready);

    printk("Main: worker completed!\n");
    return 0;
}

static void __exit completion_example_exit(void)
{
    printk("Module cleanup\n");
}

module_init(completion_example_init);
module_exit(completion_example_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Basic completion example");
```

**Output:**

```
Main: waiting for worker to complete...
Worker: starting work...
Worker: work complete, signaling...
Main: worker completed!
```

### DMA Completion Example

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/completion.h>
#include <linux/interrupt.h>

#define DMA_BUFFER_SIZE 4096

struct dma_device {
    struct device *dev;
    void __iomem *reg_base;

    /* DMA buffers */
    dma_addr_t src_dma;
    dma_addr_t dst_dma;
    void *src_buf;
    void *dst_buf;

    /* Completion */
    struct completion dma_complete;

    int irq;
};

/* DMA interrupt handler - signals completion */
static irqreturn_t dma_irq_handler(int irq, void *dev_id)
{
    struct dma_device *dma = dev_id;
    u32 status;

    /* Read status */
    status = readl(dma->reg_base + DMA_STATUS_REG);

    /* Check if transfer complete */
    if (status & DMA_COMPLETE_BIT) {
        /* Clear interrupt */
        writel(status, dma->reg_base + DMA_STATUS_REG);

        /* Signal completion - safe from IRQ context */
        complete(&dma->dma_complete);

        return IRQ_HANDLED;
    }

    return IRQ_NONE;
}

/* Initiate DMA transfer */
static int dma_transfer(struct dma_device *dma, size_t len)
{
    unsigned long timeout;
    int ret;

    /* Prepare for new transfer */
    reinit_completion(&dma->dma_complete);

    /* Configure DMA */
    writel(dma->src_dma, dma->reg_base + DMA_SRC_ADDR);
    writel(dma->dst_dma, dma->reg_base + DMA_DST_ADDR);
    writel(len, dma->reg_base + DMA_LENGTH);

    /* Start DMA */
    writel(DMA_START | DMA_IRQ_ENABLE, dma->reg_base + DMA_CONTROL);

    dev_info(dma->dev, "DMA transfer started, waiting for completion...\n");

    /* Wait for completion with 5 second timeout */
    timeout = wait_for_completion_timeout(&dma->dma_complete,
                                         msecs_to_jiffies(5000));

    if (timeout == 0) {
        dev_err(dma->dev, "DMA transfer timeout!\n");
        /* Stop DMA */
        writel(DMA_STOP, dma->reg_base + DMA_CONTROL);
        return -ETIMEDOUT;
    }

    dev_info(dma->dev, "DMA transfer completed in %lu jiffies\n",
             msecs_to_jiffies(5000) - timeout);

    return 0;
}

static int dma_probe(struct platform_device *pdev)
{
    struct dma_device *dma;
    int ret;

    dma = devm_kzalloc(&pdev->dev, sizeof(*dma), GFP_KERNEL);
    if (!dma)
        return -ENOMEM;

    dma->dev = &pdev->dev;

    /* Map registers */
    dma->reg_base = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(dma->reg_base))
        return PTR_ERR(dma->reg_base);

    /* Allocate DMA buffers */
    dma->src_buf = dmam_alloc_coherent(dma->dev, DMA_BUFFER_SIZE,
                                       &dma->src_dma, GFP_KERNEL);
    dma->dst_buf = dmam_alloc_coherent(dma->dev, DMA_BUFFER_SIZE,
                                       &dma->dst_dma, GFP_KERNEL);
    if (!dma->src_buf || !dma->dst_buf)
        return -ENOMEM;

    /* Initialize completion */
    init_completion(&dma->dma_complete);

    /* Request IRQ */
    dma->irq = platform_get_irq(pdev, 0);
    ret = devm_request_irq(&pdev->dev, dma->irq, dma_irq_handler,
                          0, dev_name(&pdev->dev), dma);
    if (ret) {
        dev_err(&pdev->dev, "Failed to request IRQ\n");
        return ret;
    }

    platform_set_drvdata(pdev, dma);

    /* Test transfer */
    memset(dma->src_buf, 0xAA, DMA_BUFFER_SIZE);
    ret = dma_transfer(dma, DMA_BUFFER_SIZE);
    if (ret)
        return ret;

    /* Verify */
    if (memcmp(dma->src_buf, dma->dst_buf, DMA_BUFFER_SIZE) == 0)
        dev_info(&pdev->dev, "DMA test successful!\n");
    else
        dev_err(&pdev->dev, "DMA test failed!\n");

    return 0;
}

static struct platform_driver dma_driver = {
    .probe = dma_probe,
    .driver = {
        .name = "completion_dma",
    },
};

module_platform_driver(dma_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("DMA driver with completion");
```

### Completion vs Wait Queue Comparison

| Feature | Completion | Wait Queue |
| --- | --- | --- |
| **Purpose** | Wait for specific event | General waiting mechanism |
| **Reusability** | One-shot (need reinit) | Reusable |
| **Complexity** | Simpler API | More flexible |
| **Typical Use** | DMA done, init complete | Data available, condition true |
| **Signal from IRQ?** | ✅ Yes (complete() is safe) | ✅ Yes (wake_up() is safe) |
| **Multiple waiters** | ✅ Yes (complete_all()) | ✅ Yes (wake_up_all()) |
| **Condition checking** | ❌ No (just "done" flag) | ✅ Yes (flexible conditions) |

**When to use what:**

```c
/* Use Completion when: */
- Waiting for single event to complete
- One-shot synchronization
- DMA transfer, I/O completion
- Thread termination

Example:
init_completion(&dma_done);
start_dma();
wait_for_completion(&dma_done);  /* DMA complete */

/* Use Wait Queue when: */
- Waiting for complex condition
- Repeated checking needed
- Multiple different conditions
- Producer-consumer pattern

Example:
wait_event_interruptible(queue, data_ready && !buffer_full);
```

### Complete Kthread Synchronization Example

```c
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/completion.h>
#include <linux/delay.h>

struct worker_data {
    struct task_struct *thread;
    struct completion work_done;
    struct completion thread_exit;
    bool stop;
    int result;
};

static int worker_thread(void *data)
{
    struct worker_data *wd = data;
    int i;

    printk("Worker thread started\n");

    for (i = 0; i < 5 && !wd->stop; i++) {
        printk("Worker: processing iteration %d\n", i);
        msleep(1000);

        /* Signal work done */
        complete(&wd->work_done);
    }

    wd->result = i;
    printk("Worker thread exiting\n");

    /* Signal thread exit */
    complete(&wd->thread_exit);

    return 0;
}

static int __init kthread_completion_init(void)
{
    struct worker_data wd;
    int i;

    /* Initialize completions */
    init_completion(&wd.work_done);
    init_completion(&wd.thread_exit);
    wd.stop = false;

    /* Start worker thread */
    wd.thread = kthread_run(worker_thread, &wd, "worker_thread");
    if (IS_ERR(wd.thread))
        return PTR_ERR(wd.thread);

    /* Wait for each work iteration to complete */
    for (i = 0; i < 5; i++) {
        printk("Main: waiting for iteration %d...\n", i);
        wait_for_completion(&wd.work_done);
        printk("Main: iteration %d completed\n", i);

        /* Reinitialize for next iteration */
        reinit_completion(&wd.work_done);
    }

    /* Wait for thread to exit */
    printk("Main: waiting for thread exit...\n");
    wait_for_completion(&wd.thread_exit);

    printk("Main: thread exited with result %d\n", wd.result);

    return 0;
}

static void __exit kthread_completion_exit(void)
{
    printk("Module cleanup\n");
}

module_init(kthread_completion_init);
module_exit(kthread_completion_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Kthread synchronization with completion");
```

### Completion Best Practices

**✅ DO:**

1. **Initialize completion:**
    
    ```c
    init_completion(&my_completion);
    ```
    
2. **Reinitialize for reuse:**
    
    ```c
    reinit_completion(&my_completion);
    ```
    
3. **Check return value of timeout variants:**
    
    ```c
    ret = wait_for_completion_timeout(&comp, HZ*5);
    if (ret == 0)
        /* Timeout */
    ```
    
4. **Use appropriate waiting variant:**
    - `_interruptible` for user-initiated operations
    - `_killable` for critical but killable operations
    - Plain for kernel-internal operations

**❌ DON'T:**

1. **Don't wait in atomic context:**
    
    ```c
    void irq_handler(int irq, void *dev_id) {
        wait_for_completion(&comp);  /* WRONG! IRQ context! */
    }
    ```
    
2. **Don't forget to complete:**
    
    ```c
    /* This will hang forever */
    wait_for_completion(&comp);
    /* Never called complete()! */
    ```
    
3. **Don't reuse without reinit:**
    
    ```c
    complete(&comp);
    wait_for_completion(&comp);  /* Returns immediately */
    wait_for_completion(&comp);  /* Still returns immediately! */
    /* Need reinit_completion()! */
    ```
    

---

## Summary

This chapter covered essential kernel sleeping and synchronization mechanisms:

**Wait Queues:**

- General-purpose sleeping mechanism
- Flexible condition checking
- Interruptible and timeout variants
- Foundation for blocking I/O

**Blocking vs Non-Blocking I/O:**

- O_NONBLOCK flag handling
- EAGAIN return for non-blocking
- Choice between efficiency and latency

**Poll/Select:**

- Efficient multi-file waiting
- Event notification system
- poll_wait() and event masks
- User-friendly I/O multiplexing

**Completion:**

- One-shot event synchronization
- Simple API for common cases
- Safe from IRQ context
- Perfect for DMA and I/O completion

Choose the appropriate mechanism based on your use case:

- Wait queues for complex conditions
- Poll/select for multi-file I/O
- Completion for simple event signaling

---