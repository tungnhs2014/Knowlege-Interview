# Part 5. Work Queues and Scheduling

## 3.3.3 Timer APIs and High-Resolution Timers

### Introduction to Kernel Timers

**Timers = Schedule functions to execute after a specified delay**

The Linux kernel provides two types of timers:

1. **Standard Timers (timer_list)** - Jiffies-based, lower resolution
2. **High-Resolution Timers (hrtimer)** - Nanosecond precision

**Time in Linux Kernel:**

```
Absolute Time (RTC):           Relative Time (Kernel Timers):
├─ Real-world date/time        ├─ Jiffies counter
├─ Hardware RTC chip           ├─ HZ-based ticking
└─ Used for timestamps         └─ Used for scheduling
```

### Understanding Jiffies and HZ

**HZ = Timer interrupt frequency (ticks per second)**

```c
/* Defined in kernel config */
CONFIG_HZ=1000  /* 1000 ticks per second (1ms resolution) */
CONFIG_HZ=250   /* 250 ticks per second (4ms resolution) */
CONFIG_HZ=100   /* 100 ticks per second (10ms resolution) */
```

**Jiffies = Counter incremented at each timer tick**

```c
/* Global variable in kernel */
extern unsigned long volatile jiffies;  /* 32-bit or 64-bit counter */
extern u64 jiffies_64;                  /* Always 64-bit */
```

**How it works:**

```
Time ───────────────────────────────────→
     ↑        ↑        ↑        ↑
     tick     tick     tick     tick
     (HZ=1000 means 1000 ticks per second)

jiffies: 0 ─→ 1 ─→ 2 ─→ 3 ─→ ... ─→ 4294967295 ─→ 0 (wraps)
```

**Time conversion helpers:**

```c
/* Convert milliseconds to jiffies */
unsigned long msecs_to_jiffies(const unsigned int m);

/* Convert microseconds to jiffies */
unsigned long usecs_to_jiffies(const unsigned int u);

/* Convert jiffies to milliseconds */
unsigned int jiffies_to_msecs(const unsigned long j);

/* Convert jiffies to microseconds */
unsigned int jiffies_to_usecs(const unsigned long j);
```

**Example: Time conversion**

```c
unsigned long delay_jiffies;

/* Schedule something 500ms from now */
delay_jiffies = msecs_to_jiffies(500);
printk("500ms = %lu jiffies\n", delay_jiffies);

/* With HZ=1000: 500ms = 500 jiffies */
/* With HZ=250:  500ms = 125 jiffies */
```

### Standard Timers (timer_list)

**Timer structure:**

```c
/* Defined in include/linux/timer.h */
struct timer_list {
    struct hlist_node entry;      /* Hash list entry */
    unsigned long expires;        /* Expiration time in jiffies */
    void (*function)(struct timer_list *);  /* Timer callback */
    u32 flags;                   /* Timer flags */
};
```

**Modern timer API (kernel 4.15+):**

```c
/* Initialize timer with callback function */
void timer_setup(struct timer_list *timer,
                 void (*callback)(struct timer_list *),
                 unsigned int flags);

/* Start/modify timer */
int mod_timer(struct timer_list *timer, unsigned long expires);

/* Add timer (schedule for first time) */
void add_timer(struct timer_list *timer);

/* Delete timer */
int del_timer(struct timer_list *timer);

/* Delete timer (synchronous - wait for completion) */
int del_timer_sync(struct timer_list *timer);

/* Check if timer is pending */
int timer_pending(const struct timer_list *timer);
```

**Timer callback function:**

```c
void timer_callback(struct timer_list *t)
{
    /* This runs in atomic context - cannot sleep! */
    /* Use container_of to get your structure */
    struct my_device *dev = from_timer(dev, t, my_timer);

    /* Your timer handler code */
}
```

**The `from_timer()` macro:**

```c
/* Helper to get container structure from timer */
#define from_timer(var, callback_timer, timer_fieldname) \
    container_of(callback_timer, typeof(*var), timer_fieldname)
```

### Basic Timer Example

```c
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

struct my_device {
    struct timer_list my_timer;
    int counter;
};

static struct my_device *global_dev;

/* Timer callback - runs in atomic context */
static void my_timer_callback(struct timer_list *t)
{
    struct my_device *dev = from_timer(dev, t, my_timer);

    printk("Timer fired! Counter: %d, jiffies: %lu\n",
           dev->counter++, jiffies);

    /* Reschedule timer for 1 second from now */
    mod_timer(&dev->my_timer, jiffies + HZ);
}

static int __init timer_example_init(void)
{
    global_dev = kzalloc(sizeof(*global_dev), GFP_KERNEL);
    if (!global_dev)
        return -ENOMEM;

    global_dev->counter = 0;

    /* Initialize timer */
    timer_setup(&global_dev->my_timer, my_timer_callback, 0);

    printk("Timer module loaded. Starting timer...\n");

    /* Start timer - fires after 300ms */
    mod_timer(&global_dev->my_timer, jiffies + msecs_to_jiffies(300));

    return 0;
}

static void __exit timer_example_exit(void)
{
    /* Delete timer synchronously */
    del_timer_sync(&global_dev->my_timer);

    kfree(global_dev);
    printk("Timer module unloaded. Total fires: %d\n", global_dev->counter);
}

module_init(timer_example_init);
module_exit(timer_example_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Basic kernel timer example");
```

### Timer Deletion - `del_timer()` vs `del_timer_sync()`

**del_timer():**

- Returns immediately
- Timer might still be running on another CPU
- Returns 1 if timer was pending, 0 otherwise

**del_timer_sync():**

- Waits for timer to complete if running
- Guarantees timer won't fire after return
- **Cannot** be called from timer callback itself (deadlock!)
- **Recommended** for cleanup code

**Example: Safe cleanup**

```c
static int device_remove(struct platform_device *pdev)
{
    struct my_device *dev = platform_get_drvdata(pdev);

    /* Delete timer synchronously - waits for completion */
    del_timer_sync(&dev->periodic_timer);

    /* Now safe to free resources */
    kfree(dev->buffer);

    return 0;
}
```

### Complete Timer Example: Periodic Sensor Polling

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/io.h>

#define POLL_INTERVAL_MS 1000
#define SENSOR_DATA_REG  0x00
#define SENSOR_STATUS_REG 0x04

struct sensor_device {
    void __iomem *reg_base;
    struct timer_list poll_timer;
    spinlock_t lock;
    u32 last_value;
    bool running;
};

/* Timer callback - poll sensor */
static void sensor_poll_timer(struct timer_list *t)
{
    struct sensor_device *sensor = from_timer(sensor, t, poll_timer);
    u32 value, status;
    unsigned long flags;

    /* Read sensor - atomic context, cannot sleep */
    status = readl(sensor->reg_base + SENSOR_STATUS_REG);

    if (status & SENSOR_DATA_READY) {
        value = readl(sensor->reg_base + SENSOR_DATA_REG);

        spin_lock_irqsave(&sensor->lock, flags);
        sensor->last_value = value;
        spin_unlock_irqrestore(&sensor->lock, flags);

        printk("Sensor value: %u\n", value);
    }

    /* Reschedule timer if still running */
    if (sensor->running) {
        mod_timer(&sensor->poll_timer,
                 jiffies + msecs_to_jiffies(POLL_INTERVAL_MS));
    }
}

static int sensor_start_polling(struct sensor_device *sensor)
{
    sensor->running = true;

    /* Start timer */
    mod_timer(&sensor->poll_timer,
             jiffies + msecs_to_jiffies(POLL_INTERVAL_MS));

    printk("Sensor polling started\n");
    return 0;
}

static void sensor_stop_polling(struct sensor_device *sensor)
{
    sensor->running = false;

    /* Stop timer - wait for completion */
    del_timer_sync(&sensor->poll_timer);

    printk("Sensor polling stopped\n");
}

static int sensor_probe(struct platform_device *pdev)
{
    struct sensor_device *sensor;
    struct resource *res;

    sensor = devm_kzalloc(&pdev->dev, sizeof(*sensor), GFP_KERNEL);
    if (!sensor)
        return -ENOMEM;

    /* Map device registers */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    sensor->reg_base = devm_ioremap_resource(&pdev->dev, res);
    if (IS_ERR(sensor->reg_base))
        return PTR_ERR(sensor->reg_base);

    spin_lock_init(&sensor->lock);

    /* Initialize timer */
    timer_setup(&sensor->poll_timer, sensor_poll_timer, 0);

    platform_set_drvdata(pdev, sensor);

    /* Start polling */
    sensor_start_polling(sensor);

    return 0;
}

static int sensor_remove(struct platform_device *pdev)
{
    struct sensor_device *sensor = platform_get_drvdata(pdev);

    /* Stop polling */
    sensor_stop_polling(sensor);

    return 0;
}

static struct platform_driver sensor_driver = {
    .probe = sensor_probe,
    .remove = sensor_remove,
    .driver = {
        .name = "sensor_timer",
    },
};

module_platform_driver(sensor_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Sensor driver with timer-based polling");
```

### High-Resolution Timers (hrtimer)

**Why High-Resolution Timers?**

Standard timers are limited by HZ granularity:

- HZ=1000 → 1ms resolution
- HZ=250 → 4ms resolution

High-resolution timers provide **nanosecond precision** using hardware timers.

**Requirements:**

- Hardware support (most modern CPUs)
- Kernel config: `CONFIG_HIGH_RES_TIMERS=y`

**Check if available:**

```bash
cat /proc/timer_list | grep resolution
# Should show: .resolution: 1 nsecs

zcat /proc/config.gz | grep CONFIG_HIGH_RES_TIMERS
# Should show: CONFIG_HIGH_RES_TIMERS=y
```

**hrtimer structure:**

```c
/* Defined in include/linux/hrtimer.h */
struct hrtimer {
    struct timerqueue_node node;
    ktime_t _softexpires;                    /* Soft expiry time */
    enum hrtimer_restart (*function)(struct hrtimer *);  /* Callback */
    struct hrtimer_clock_base *base;
    u8 state;
    u8 is_rel;
    u8 is_soft;
};
```

**ktime_t = Kernel time type (nanoseconds):**

```c
typedef s64 ktime_t;  /* Nanoseconds since boot */
```

### hrtimer API

**Initialization:**

```c
/* Initialize hrtimer */
void hrtimer_init(struct hrtimer *timer, clockid_t clock_id,
                  enum hrtimer_mode mode);

/* Clock IDs */
#define CLOCK_MONOTONIC  /* Monotonic time (doesn't include suspend) */
#define CLOCK_REALTIME   /* Wall-clock time */
#define CLOCK_BOOTTIME   /* Monotonic including suspend time */

/* Modes */
enum hrtimer_mode {
    HRTIMER_MODE_ABS = 0,  /* Absolute time */
    HRTIMER_MODE_REL = 1,  /* Relative time */
};
```

**Starting timer:**

```c
/* Start hrtimer */
int hrtimer_start(struct hrtimer *timer, ktime_t tim,
                  const enum hrtimer_mode mode);

/* Start with range (allows timer coalescing) */
void hrtimer_start_range_ns(struct hrtimer *timer, ktime_t tim,
                            u64 range_ns, const enum hrtimer_mode mode);
```

**Canceling timer:**

```c
/* Cancel timer (returns 1 if timer was active) */
int hrtimer_cancel(struct hrtimer *timer);

/* Try to cancel (returns -1 if callback running) */
int hrtimer_try_to_cancel(struct hrtimer *timer);
```

**Callback function:**

```c
/* hrtimer callback prototype */
enum hrtimer_restart timer_callback(struct hrtimer *timer)
{
    /* Your code here - atomic context! */

    /* Return value determines if timer restarts */
    return HRTIMER_NORESTART;  /* One-shot timer */
    /* OR */
    return HRTIMER_RESTART;    /* Periodic timer */
}
```

**Time helpers:**

```c
/* Create ktime from components */
ktime_t ktime_set(const s64 secs, const unsigned long nsecs);

/* Get current time */
ktime_t ktime_get(void);          /* CLOCK_MONOTONIC */
ktime_t ktime_get_real(void);     /* CLOCK_REALTIME */
ktime_t ktime_get_boottime(void); /* CLOCK_BOOTTIME */

/* Convert to/from nanoseconds */
s64 ktime_to_ns(const ktime_t kt);
ktime_t ns_to_ktime(u64 ns);

/* Time arithmetic */
ktime_t ktime_add(ktime_t kt1, ktime_t kt2);
ktime_t ktime_add_ns(ktime_t kt, u64 ns);
ktime_t ktime_sub(ktime_t kt1, ktime_t kt2);
s64 ktime_us_delta(ktime_t later, ktime_t earlier);
s64 ktime_ms_delta(ktime_t later, ktime_t earlier);
```

### Basic hrtimer Example

```c
#include <linux/module.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>

static struct hrtimer hr_timer;
static ktime_t ktime_period;
static int counter = 0;

/* hrtimer callback - atomic context */
static enum hrtimer_restart hrtimer_callback(struct hrtimer *timer)
{
    ktime_t now = ktime_get();

    printk("hrtimer fired! Counter: %d, time: %lld ns\n",
           counter++, ktime_to_ns(now));

    /* Restart timer - forward by period */
    hrtimer_forward(timer, now, ktime_period);

    return HRTIMER_RESTART;  /* Keep timer running */
}

static int __init hrtimer_example_init(void)
{
    /* Timer period: 100 milliseconds */
    ktime_period = ktime_set(0, 100 * 1000 * 1000);  /* 100ms in ns */

    /* Initialize hrtimer */
    hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    hr_timer.function = hrtimer_callback;

    /* Start timer */
    hrtimer_start(&hr_timer, ktime_period, HRTIMER_MODE_REL);

    printk("hrtimer module loaded. Timer period: 100ms\n");
    return 0;
}

static void __exit hrtimer_example_exit(void)
{
    /* Cancel timer */
    hrtimer_cancel(&hr_timer);

    printk("hrtimer module unloaded. Total fires: %d\n", counter);
}

module_init(hrtimer_example_init);
module_exit(hrtimer_example_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("High-resolution timer example");
```

### Complete hrtimer Example: Precision PWM Generator

```c
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/gpio/consumer.h>

struct pwm_device {
    struct hrtimer hr_timer;
    struct gpio_desc *gpio;
    ktime_t period_ns;
    ktime_t duty_ns;
    bool gpio_state;
    bool running;
};

/* hrtimer callback - toggle GPIO for PWM */
static enum hrtimer_restart pwm_timer_callback(struct hrtimer *timer)
{
    struct pwm_device *pwm = container_of(timer, struct pwm_device, hr_timer);
    ktime_t now, next_event;

    if (!pwm->running)
        return HRTIMER_NORESTART;

    now = ktime_get();

    /* Toggle GPIO */
    pwm->gpio_state = !pwm->gpio_state;
    gpiod_set_value(pwm->gpio, pwm->gpio_state);

    /* Calculate next event */
    if (pwm->gpio_state) {
        /* GPIO went HIGH - next event is duty_ns away */
        next_event = ktime_add_ns(now, ktime_to_ns(pwm->duty_ns));
    } else {
        /* GPIO went LOW - next event is (period - duty) away */
        next_event = ktime_add_ns(now,
                    ktime_to_ns(pwm->period_ns) - ktime_to_ns(pwm->duty_ns));
    }

    /* Schedule next edge */
    hrtimer_set_expires(&pwm->hr_timer, next_event);

    return HRTIMER_RESTART;
}

/* Set PWM parameters: frequency and duty cycle */
static int pwm_config(struct pwm_device *pwm,
                     unsigned int freq_hz, unsigned int duty_percent)
{
    u64 period_ns, duty_ns;

    if (duty_percent > 100)
        return -EINVAL;

    /* Calculate period in nanoseconds */
    period_ns = NSEC_PER_SEC / freq_hz;
    duty_ns = (period_ns * duty_percent) / 100;

    pwm->period_ns = ns_to_ktime(period_ns);
    pwm->duty_ns = ns_to_ktime(duty_ns);

    printk("PWM configured: freq=%u Hz, duty=%u%%, period=%llu ns, duty=%llu ns\n",
           freq_hz, duty_percent, period_ns, duty_ns);

    return 0;
}

/* Start PWM generation */
static int pwm_enable(struct pwm_device *pwm)
{
    ktime_t now;

    pwm->running = true;
    pwm->gpio_state = false;
    gpiod_set_value(pwm->gpio, 0);

    /* Start timer */
    now = ktime_get();
    hrtimer_start(&pwm->hr_timer, ktime_add_ns(now, ktime_to_ns(pwm->period_ns)),
                  HRTIMER_MODE_ABS);

    printk("PWM enabled\n");
    return 0;
}

/* Stop PWM generation */
static void pwm_disable(struct pwm_device *pwm)
{
    pwm->running = false;

    /* Cancel timer */
    hrtimer_cancel(&pwm->hr_timer);

    /* Set GPIO low */
    gpiod_set_value(pwm->gpio, 0);

    printk("PWM disabled\n");
}

static int pwm_probe(struct platform_device *pdev)
{
    struct pwm_device *pwm;

    pwm = devm_kzalloc(&pdev->dev, sizeof(*pwm), GFP_KERNEL);
    if (!pwm)
        return -ENOMEM;

    /* Get GPIO */
    pwm->gpio = devm_gpiod_get(&pdev->dev, "pwm", GPIOD_OUT_LOW);
    if (IS_ERR(pwm->gpio)) {
        dev_err(&pdev->dev, "Failed to get GPIO\n");
        return PTR_ERR(pwm->gpio);
    }

    /* Initialize hrtimer */
    hrtimer_init(&pwm->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_ABS);
    pwm->hr_timer.function = pwm_timer_callback;

    platform_set_drvdata(pdev, pwm);

    /* Configure: 1kHz, 50% duty cycle */
    pwm_config(pwm, 1000, 50);

    /* Start PWM */
    pwm_enable(pwm);

    dev_info(&pdev->dev, "PWM device initialized\n");
    return 0;
}

static int pwm_remove(struct platform_device *pdev)
{
    struct pwm_device *pwm = platform_get_drvdata(pdev);

    /* Stop PWM */
    pwm_disable(pwm);

    dev_info(&pdev->dev, "PWM device removed\n");
    return 0;
}

static struct platform_driver pwm_driver = {
    .probe = pwm_probe,
    .remove = pwm_remove,
    .driver = {
        .name = "hrtimer_pwm",
    },
};

module_platform_driver(pwm_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("High-resolution timer PWM generator");
```

### Timer Comparison Table

| Feature | Standard Timer | hrtimer |
| --- | --- | --- |
| **Resolution** | 1/HZ (1-10ms) | Nanoseconds |
| **Precision** | Low | Very high |
| **Overhead** | Lower | Higher |
| **Hardware** | Software counter | Hardware timer |
| **Config** | Always available | CONFIG_HIGH_RES_TIMERS |
| **Use Case** | Normal timeouts | Precise timing |
| **Example** | Polling, retries | PWM, audio, precise delays |

### Timer Best Practices

**✅ DO:**

1. **Use appropriate timer type:**
    - Standard timer for normal timeouts (> 10ms)
    - hrtimer for precise timing (< 1ms)
2. **Delete timers in cleanup:**
    
    ```c
    del_timer_sync(&timer);  /* Or hrtimer_cancel() */
    ```
    
3. **Keep timer callbacks short:**
    
    ```c
    void timer_cb(struct timer_list *t) {
        /* Fast operations only - atomic context! */
    }
    ```
    
4. **Use mod_timer() to reschedule:**
    
    ```c
    mod_timer(&timer, jiffies + HZ);
    ```
    

**❌ DON'T:**

1. **Don't sleep in timer callback:**
    
    ```c
    void bad_timer(struct timer_list *t) {
        msleep(10);  /* WRONG! Atomic context! */
    }
    ```
    
2. **Don't call del_timer_sync() from callback:**
    
    ```c
    void bad_timer(struct timer_list *t) {
        del_timer_sync(t);  /* DEADLOCK! */
    }
    ```
    
3. **Don't do heavy processing:**
    
    ```c
    void bad_timer(struct timer_list *t) {
        for (i = 0; i < 1000000; i++)  /* WRONG! Too slow! */
            process();
    }
    ```
    

---

## Summary

This chapter covered three essential work deferring mechanisms:

**Workqueues:**

- Process context execution
- Can sleep and block
- Suitable for long operations
- System-wide or custom queues

**Tasklets:**

- Atomic context execution
- Fast, lightweight
- Cannot sleep
- Built on softIRQs

**Timers:**

- Standard timers (jiffies-based)
- High-resolution timers (nanosecond precision)
- Time-based execution
- Atomic context

Choose the appropriate mechanism based on your requirements: workqueues for blocking operations, tasklets for fast atomic operations, and timers for time-based execution.

---