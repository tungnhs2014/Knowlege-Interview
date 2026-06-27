# 15 - Interrupt Management

## Learning Goal
After this topic, you should be able to explain how a hardware event becomes a Linux driver callback, write a basic interrupt handler, choose between hard IRQ, threaded IRQ, and workqueue deferral, and debug common interrupt failures.

The practical goal is not to memorize every IRQ core structure. It is to know **which context your code runs in**, what it may safely do there, and how to move slow work out of the hard interrupt path.

## Why This Matters In Real Work
Most embedded drivers eventually handle interrupts: data-ready pins, RX/TX completion, FIFO thresholds, DMA completion, GPIO buttons, touchscreens, PMIC events, PCI devices, and wakeup sources.

Interrupt bugs are painful because they often look like random latency, missing events, storms, or "sleeping function called from invalid context" warnings.

You need interrupt management when:

- A device has an `interrupts = <...>` property in Device Tree.
- A platform, I2C, SPI, GPIO, PCI, or MFD device signals events asynchronously.
- Polling wastes CPU time, loses responsiveness, or prevents low-power idle.
- A hard IRQ handler needs to wake a thread or schedule work.
- One parent interrupt line multiplexes many child interrupt sources.
- You must debug `/proc/interrupts` counters that never move or move too fast.

**Production rule:** keep hard IRQ handlers short, bounded, and non-sleeping. Anything slow, blocking, or bus-based belongs in a threaded IRQ or workqueue.

## Mental Model
An interrupt is hardware saying: "something happened; look now." Linux does not call your driver directly from raw CPU exception code. The architecture entry path, interrupt controller driver, and generic IRQ core route the event to your registered handler.

```text
Device event
  -> interrupt controller sees a hardware source
  -> CPU enters architecture IRQ path
  -> IRQ domain maps hardware IRQ to Linux IRQ
  -> IRQ core runs a flow handler
  -> your handler runs
  -> optional thread/work handles slower work
```

Think of a hard IRQ handler as a triage step:

- identify whether the interrupt is yours;
- read and save minimal status;
- acknowledge, clear, or mask the source as required by hardware;
- wake a thread or queue work if more processing is needed;
- return quickly.

## Core Concepts
Interrupt management has two sides: **IRQ consumers** request interrupts, and **IRQ providers/controllers** route interrupt sources into Linux IRQ numbers.

| Concept | Meaning | Driver-facing example |
| --- | --- | --- |
| IRQ | Interrupt request visible to Linux | value passed to `request_irq()` |
| hwirq | Hardware IRQ number local to one interrupt controller | GPIO offset 5 in one GPIO bank |
| virq / Linux IRQ | Global Linux IRQ number | what a consumer driver requests |
| IRQ domain | Mapping from hwirq to Linux IRQ | per interrupt controller |
| Hard IRQ / top half | Fast handler in atomic context | `request_irq()` handler |
| Bottom half | Deferred work after hard IRQ | threaded IRQ, workqueue, tasklet |
| Flow handler | IRQ core wrapper for edge/level/simple behavior | `handle_level_irq()` |
| IRQ action | Registered driver handler metadata | created by request APIs |
| IRQ chip | Controller operations | mask, unmask, ack, set type |

### Polling vs Interrupts
Polling asks the device repeatedly whether anything changed. Interrupts let the device notify the CPU when an event happens.

| Approach | Best for | Cost |
| --- | --- | --- |
| Polling | simple bring-up, hardware without IRQ, periodic status | CPU wakeups, latency tradeoff |
| Interrupts | asynchronous events, low latency, low power | harder context, locking, ack rules |

### Hard IRQ vs Threaded IRQ vs Workqueue
The main design choice is where the real work should run.

| Mechanism | Context | Can sleep? | Typical use |
| --- | --- | --- | --- |
| Hard IRQ handler | atomic interrupt context | no | read status, ack, capture data |
| Threaded IRQ handler | process/thread context | yes | I2C/SPI/regmap access, larger processing |
| Workqueue | process context | yes | deferred work that can be batched/cancelled |
| Tasklet / softirq | atomic softirq context | no | legacy fast bottom-half code |

**Interview trap:** "bottom half" does not always mean "can sleep." Tasklets and softirqs are deferred, but still atomic. Workqueues and threaded IRQs run in process context.

### Level vs Edge Interrupts
Trigger type changes how bugs appear.

| Type | Meaning | Common failure |
| --- | --- | --- |
| Level-triggered | line represents a state | storm if status stays asserted |
| Edge-triggered | line represents a transition | missed event if edge never occurs |

For level interrupts, your driver must clear the device-level condition, not just return from the handler.

## Kernel Mechanism
The generic IRQ layer gives Linux one common interrupt model across different CPU architectures and interrupt controllers.

The full path is roughly:

```text
1. Device asserts an interrupt source.
2. Interrupt controller records the hardware source.
3. CPU enters architecture-specific IRQ entry code.
4. Root interrupt handler reads controller state.
5. IRQ domain translates hwirq -> Linux IRQ.
6. IRQ core finds struct irq_desc.
7. Flow handler performs edge/level bookkeeping.
8. IRQ core calls registered struct irqaction handler(s).
9. Handler returns IRQ_NONE, IRQ_HANDLED, or IRQ_WAKE_THREAD.
10. Optional threaded handler or workqueue performs slow work.
```

The driver usually interacts with this mechanism at two levels:

- consumer level: get a Linux IRQ number and request it;
- controller level: implement `irq_chip`/`irq_domain` when your hardware provides IRQs to other devices.

### Shared IRQ Lines
Some IRQ lines can be shared by multiple devices. On a shared line, the IRQ core calls each registered handler.

Your handler must:

- read device status;
- return `IRQ_NONE` if the interrupt was not from your device;
- return `IRQ_HANDLED` only after handling your device's cause;
- pass a unique, non-NULL `dev_id`;
- use the same `dev_id` when freeing the IRQ.

### Chained And Nested Interrupts
Interrupt controllers can be stacked. A GPIO controller may have many child IRQs but one parent IRQ line.

```text
Sensor IRQ
  -> GPIO expander pin 2
  -> expander INT pin
  -> SoC GPIO parent IRQ
  -> GIC/root interrupt controller
  -> Linux IRQ core
```

| Model | Use when | Parent handler | Child dispatch |
| --- | --- | --- | --- |
| Chained | controller access is fast and non-sleeping | hard IRQ | `generic_handle_irq()` or domain helper |
| Nested | controller access may sleep, such as I2C/SPI | threaded IRQ | `handle_nested_irq()` |

GPIO-controller IRQ implementation itself belongs mostly to topic 14. For this topic, the important lesson is the context rule: **sleepable controllers need threaded/nested handling.**

## Key Structs And APIs
These names matter because they show where ownership and context live.

### Consumer-Side APIs
Use these when your driver consumes an IRQ from platform data, Device Tree, ACPI, PCI, GPIO, or another bus.

| API | Role |
| --- | --- |
| `platform_get_irq(pdev, index)` | Get a Linux IRQ from platform resources or DT |
| `platform_get_irq_byname()` | Get a named IRQ resource |
| `request_irq()` | Register a hard IRQ handler |
| `devm_request_irq()` | Managed hard IRQ registration |
| `request_threaded_irq()` | Register primary hard handler plus threaded handler |
| `devm_request_threaded_irq()` | Managed threaded IRQ registration |
| `request_any_context_irq()` | Let IRQ core choose hard or nested/threaded context |
| `free_irq()` | Remove handler and wait for running hard handlers |
| `devm_free_irq()` | Manually free a devm-requested IRQ early |

### Handler Return Values
The return value tells the IRQ core what happened.

| Return | Meaning |
| --- | --- |
| `IRQ_NONE` | Not my interrupt, usually for shared lines |
| `IRQ_HANDLED` | My device caused it and I handled it |
| `IRQ_WAKE_THREAD` | Wake the registered threaded handler |
| `IRQ_RETVAL(x)` | Helper mapping true/false to handled/none |

### Important Flags
Flags describe sharing, trigger type, threading, suspend, and balancing behavior.

| Flag | Meaning |
| --- | --- |
| `IRQF_SHARED` | Multiple handlers may share this line |
| `IRQF_ONESHOT` | Keep IRQ masked until threaded handler finishes |
| `IRQF_TRIGGER_RISING` | Rising edge trigger |
| `IRQF_TRIGGER_FALLING` | Falling edge trigger |
| `IRQF_TRIGGER_HIGH` | Active-high level trigger |
| `IRQF_TRIGGER_LOW` | Active-low level trigger |
| `IRQF_NO_SUSPEND` | Keep IRQ enabled during suspend path |
| `IRQF_NO_THREAD` | Prevent forced threading for this IRQ |

Prefer trigger information from firmware/DT bindings and hardware documentation. Do not guess polarity.

### IRQ Core / Controller Structures
You do not usually allocate these directly in a simple consumer driver, but you should recognize their roles.

| Struct | Role |
| --- | --- |
| `struct irq_desc` | IRQ core descriptor for one Linux IRQ |
| `struct irqaction` | One registered handler/action on an IRQ |
| `struct irq_data` | Per-IRQ data passed to irqchip callbacks |
| `struct irq_chip` | Interrupt-controller operations |
| `struct irq_domain` | hwirq-to-Linux-IRQ mapping |
| `struct irq_domain_ops` | `.map`, `.unmap`, `.xlate` callbacks |

Common controller-side APIs:

- `irq_domain_add_linear()`
- `irq_domain_add_tree()`
- `irq_create_mapping()`
- `irq_find_mapping()`
- `irq_domain_xlate_onecell()`
- `irq_domain_xlate_twocell()`
- `irq_set_chained_handler_and_data()`
- `chained_irq_enter()`
- `chained_irq_exit()`
- `generic_handle_irq()`
- `generic_handle_domain_irq()`
- `handle_nested_irq()`

## Lifecycle / Data Flow
A normal interrupt-consuming platform driver wires IRQs during probe and unwires them during remove or devm cleanup.

### Probe-Time Flow
```text
1. allocate private data
2. map registers and get clocks/resets if needed
3. get IRQ number with platform_get_irq()
4. initialize locks, work structs, or threaded state
5. request IRQ with devm_request_irq() or devm_request_threaded_irq()
6. configure device interrupt mask/type/status registers
7. enable device interrupt source
8. return from probe
```

### Runtime Hard IRQ Flow
```text
1. IRQ fires
2. handler reads device status
3. if shared and not ours, return IRQ_NONE
4. clear/ack/mask hardware as required
5. save minimal state
6. return IRQ_HANDLED or IRQ_WAKE_THREAD
```

### Threaded IRQ Flow
```text
1. primary handler runs in hard IRQ context
2. primary handler returns IRQ_WAKE_THREAD
3. IRQ core wakes thread_fn
4. with IRQF_ONESHOT, IRQ remains masked until thread_fn completes
5. thread_fn performs sleepable work
6. thread_fn returns IRQ_HANDLED
```

### Remove / Shutdown Flow
```text
1. stop hardware from generating new interrupts
2. mask/disable device-level IRQ source
3. cancel or flush deferred work if used
4. free IRQ explicitly or let devm cleanup run
5. unmap/disable other resources
```

**Production rule:** devm cleanup removes the handler, but it does not magically make careless hardware quiet. Disable the device interrupt source in remove/suspend paths.

## Minimal Practical Example
This is learning-only pseudo-code for a simple MMIO platform device. It demonstrates a hard IRQ handler that captures status and wakes a worker for slower processing.

```c
struct foo_dev {
    void __iomem *base;
    int irq;
    spinlock_t lock;
    struct work_struct work;
    u32 pending_status;
};

static void foo_work(struct work_struct *work)
{
    struct foo_dev *foo = container_of(work, struct foo_dev, work);
    u32 status;
    unsigned long flags;

    spin_lock_irqsave(&foo->lock, flags);
    status = foo->pending_status;
    foo->pending_status = 0;
    spin_unlock_irqrestore(&foo->lock, flags);

    /*
     * Process data here. This runs in process context, so sleeping
     * operations are allowed if no spinlock is held.
     */
    foo_process_events(foo, status);
}

static irqreturn_t foo_irq(int irq, void *dev_id)
{
    struct foo_dev *foo = dev_id;
    unsigned long flags;
    u32 status;

    status = readl(foo->base + FOO_INT_STATUS);
    if (!status)
        return IRQ_NONE;          /* important for shared lines */

    writel(status, foo->base + FOO_INT_CLEAR);  /* clear device cause */

    spin_lock(&foo->lock);
    foo->pending_status |= status;
    spin_unlock(&foo->lock);

    schedule_work(&foo->work);
    return IRQ_HANDLED;
}

static int foo_probe(struct platform_device *pdev)
{
    struct foo_dev *foo;
    int ret;

    foo = devm_kzalloc(&pdev->dev, sizeof(*foo), GFP_KERNEL);
    if (!foo)
        return -ENOMEM;

    spin_lock_init(&foo->lock);
    INIT_WORK(&foo->work, foo_work);

    foo->base = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(foo->base))
        return PTR_ERR(foo->base);

    foo->irq = platform_get_irq(pdev, 0);
    if (foo->irq < 0)
        return foo->irq;

    ret = devm_request_irq(&pdev->dev, foo->irq, foo_irq,
                           IRQF_SHARED, dev_name(&pdev->dev), foo);
    if (ret)
        return ret;

    platform_set_drvdata(pdev, foo);
    foo_enable_interrupts(foo);
    return 0;
}
```

For an I2C/SPI device, use a threaded IRQ if the handler must access the bus:

```c
static irqreturn_t sensor_irq_thread(int irq, void *dev_id)
{
    struct sensor *s = dev_id;

    mutex_lock(&s->lock);
    sensor_read_status_over_i2c(s);
    sensor_clear_interrupt_over_i2c(s);
    mutex_unlock(&s->lock);

    return IRQ_HANDLED;
}

ret = devm_request_threaded_irq(dev, irq,
                                NULL, sensor_irq_thread,
                                IRQF_ONESHOT | IRQF_TRIGGER_LOW,
                                "sensor", s);
```

This thread-only pattern is common for slow bus devices because the default primary handler only wakes the thread.

## Common Bugs And Debugging
Start debugging from the symptom. Interrupt failures usually leave clues in counters, status registers, logs, and context warnings.

### Symptom: IRQ Counter Never Increments
Likely causes:

- wrong `interrupt-parent` or `interrupts` property;
- wrong pinctrl state, pin not configured as input/IRQ;
- device interrupt source is masked;
- clock or power domain is disabled;
- `platform_get_irq()` failed but error handling ignored it;
- wrong parent GPIO/IRQ controller driver;
- hardware line not connected as expected.

Evidence to inspect:

```bash
cat /proc/interrupts
ls /proc/irq
dmesg | grep -i irq
```

Fix pattern:

- verify DT cells and trigger type;
- confirm probe logs and IRQ number;
- read device interrupt enable/status registers;
- verify pinmux and electrical polarity.

### Symptom: IRQ Counter Increments Too Fast
Likely causes:

- level interrupt status is never cleared;
- wrong active level or edge/level type;
- handler returns `IRQ_HANDLED` without fixing the cause;
- threaded handler lacks `IRQF_ONESHOT`;
- shared IRQ device keeps line asserted;
- mask/unmask or ack order is wrong.

Debug checklist:

- print status before and after clear;
- check whether the line is level or edge triggered;
- check status-clear semantics in the datasheet;
- confirm the threaded handler actually runs;
- confirm no lock prevents the clear path from executing.

### Symptom: "sleeping function called from invalid context"
Likely causes:

- I2C/SPI/regmap sleeping access in hard IRQ context;
- `mutex_lock()` inside hard IRQ;
- `msleep()` or wait queue call in hard IRQ;
- allocation with `GFP_KERNEL` in hard IRQ;
- chained handler used for sleepable controller.

Fix pattern:

- move work into `request_threaded_irq()` `thread_fn`;
- use workqueues for deferred processing;
- use spinlocks only for short hard-IRQ shared data;
- do not hold spinlocks around sleepable functions.

### Symptom: Shared IRQ Misbehaves
Likely causes:

- missing `IRQF_SHARED`;
- `dev_id` is `NULL` or not unique;
- handler always returns `IRQ_HANDLED`;
- devices sharing the line use incompatible flags;
- hard handler cannot distinguish its device's status.

Fix pattern:

- use a private data pointer as `dev_id`;
- check device status first;
- return `IRQ_NONE` if not yours;
- verify every sharer uses compatible flags.

### Useful Debug Hooks
Use these before inventing complicated theories:

- `/proc/interrupts`: counters, IRQ names, per-CPU handling.
- `/proc/irq/<irq>/`: affinity and IRQ-specific information when available.
- `dev_dbg()` plus dynamic debug: handler entry, status, ack, return path.
- ftrace/tracepoints: latency and handler timing when enabled for the target kernel.
- Device registers: interrupt status, mask, clear, type, polarity, wake.

## Production Checklist
Before review or board bring-up, verify the IRQ path from firmware to hardware clear.

- IRQ number is obtained with checked error handling.
- Trigger type matches hardware and DT binding.
- `dev_id` is unique and non-NULL for shared IRQs.
- Shared handler returns `IRQ_NONE` when the device did not assert the IRQ.
- Hard IRQ handler does not sleep, allocate with `GFP_KERNEL`, use mutexes, or call userspace-copy APIs.
- Slow bus access is in a threaded IRQ or workqueue.
- `IRQF_ONESHOT` is used for thread-only or slow level-triggered handlers.
- Device-level interrupt source is cleared or masked in the correct order.
- Remove/suspend paths disable the device interrupt source before cleanup.
- Deferred work is cancelled or flushed during teardown.
- Locking matches context: spinlocks for hard IRQ sharing, mutexes for threaded/process-only paths.
- `/proc/interrupts` name is meaningful.
- Debug prints can be enabled without flooding normal logs.
- Wakeup and suspend behavior is documented if the IRQ can wake the system.

## Interview Readiness
You are ready for interviews when you can reason from symptom to context, not just recite APIs.

Be able to explain:

- why interrupts exist and when polling is acceptable;
- what hard IRQ context forbids;
- `request_irq()` parameters and why `dev_id` matters;
- `IRQ_NONE`, `IRQ_HANDLED`, and `IRQ_WAKE_THREAD`;
- top half vs bottom half;
- threaded IRQ vs workqueue;
- why `IRQF_ONESHOT` matters;
- level vs edge interrupt bugs;
- hwirq vs Linux IRQ and why IRQ domains exist;
- chained vs nested interrupts;
- how to debug missing IRQs and interrupt storms.

See `interview/15-interrupt-management.md` for scenario-driven practice.

## Kernel Version Notes
Interrupt APIs are stable in concept, but some helper details are version-sensitive.

- `request_irq()` is commonly implemented through the threaded IRQ infrastructure with no `thread_fn`; teach the model, not a fixed internal line number.
- Tasklets are legacy for new driver design. Know them for existing code, but prefer threaded IRQs or workqueues unless maintaining old code.
- GPIO irqchip helper style has evolved. Older examples use post-registration helpers; current GPIO documentation prefers configuring `gpio_chip.irq` / `struct gpio_irq_chip` before chip registration.
- For production examples, validate exact prototypes and helper choices against the target kernel headers and in-tree drivers.
