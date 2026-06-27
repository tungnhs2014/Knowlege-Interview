# 15 - Interrupt Management Interview Questions

Strong candidates can trace an interrupt from hardware to a driver handler, explain hard IRQ context rules, choose the right bottom-half mechanism, and debug missing interrupts or interrupt storms without guessing.

## Beginner Questions

### 1. What is an interrupt, and why do drivers use interrupts instead of polling?
**Short Answer:** An interrupt is a hardware notification that an event happened. Drivers use it to avoid repeatedly polling a device for status.

**Deep Explanation:** Polling means the CPU asks the device again and again whether anything changed. Interrupts invert that model: the device signals the CPU only when attention is needed. Linux routes that signal through interrupt-controller and IRQ-core code until the registered driver handler runs.

**API / Code Anchor:**
```c
irq = platform_get_irq(pdev, 0);
ret = devm_request_irq(&pdev->dev, irq, my_irq_handler,
                       0, dev_name(&pdev->dev), priv);
```

**Production or Debugging Angle:** Polling can be useful during early bring-up, but production drivers usually prefer interrupts for responsiveness and power. If interrupt handling fails, temporarily polling the status register can prove whether the device event is happening at all.

**Common Traps:** Saying interrupts are always faster. Bad interrupt handling can be worse than polling if it causes storms, long hard IRQ handlers, or missed clears.

**Follow-up Questions:**
- When is polling still acceptable?
- What is the first file you check to see whether an IRQ is firing?
- Why can interrupts help low-power systems?

### 2. What happens from a device interrupt to your driver handler?
**Short Answer:** Hardware signals an interrupt controller, the CPU enters the IRQ path, the IRQ core maps the hardware source to a Linux IRQ, then calls registered handlers.

**Deep Explanation:** The driver does not handle a raw CPU exception directly. The root interrupt controller identifies a hardware IRQ source. An IRQ domain translates controller-local hwirq numbering into a Linux IRQ number. The generic IRQ core finds the IRQ descriptor, runs the flow handler, and invokes the registered `irqaction` handler.

**API / Code Anchor:**
```text
hardware event
  -> interrupt controller
  -> IRQ domain: hwirq -> Linux IRQ
  -> irq_desc flow handler
  -> irqaction handler
```

**Production or Debugging Angle:** If `/proc/interrupts` never increments, debug upstream first: DT wiring, pinmux, interrupt controller, mask registers, and trigger polarity.

**Common Traps:** Treating hardware IRQ numbers as the same thing as Linux IRQ numbers. They are often different once IRQ domains are involved.

**Follow-up Questions:**
- What is an IRQ domain?
- What is a flow handler?
- Why can two controllers both have hwirq 0 without conflict?

### 3. What can and cannot be done in a hard IRQ handler?
**Short Answer:** A hard IRQ handler must be fast and non-sleeping. It can read/ack registers and save minimal state, but it cannot sleep, take mutexes, or do slow bus I/O.

**Deep Explanation:** Hard IRQ context is atomic. It interrupted other code and runs with severe scheduling restrictions. Blocking there can deadlock or trigger kernel warnings. Slow work should be moved to a threaded IRQ handler or workqueue.

**API / Code Anchor:**
```c
static irqreturn_t my_irq(int irq, void *dev_id)
{
    u32 status = readl(base + STATUS_REG);

    if (!status)
        return IRQ_NONE;

    writel(status, base + CLEAR_REG);
    return IRQ_HANDLED;
}
```

**Production or Debugging Angle:** A warning such as "sleeping function called from invalid context" often means a hard IRQ handler called I2C/SPI/regmap sleeping APIs, `mutex_lock()`, `msleep()`, or allocated with `GFP_KERNEL`.

**Common Traps:** Assuming "small" I2C reads are safe in hard IRQ context. They are not safe if the bus operation may sleep.

**Follow-up Questions:**
- Which lock type is allowed in hard IRQ context?
- Can you call `copy_to_user()` from a hard IRQ?
- Where should I2C register reads happen?

### 4. What do `IRQ_NONE`, `IRQ_HANDLED`, and `IRQ_WAKE_THREAD` mean?
**Short Answer:** `IRQ_NONE` means "not my interrupt", `IRQ_HANDLED` means handled, and `IRQ_WAKE_THREAD` means wake the threaded IRQ handler.

**Deep Explanation:** Return values guide the IRQ core after your handler runs. On shared lines, a handler must return `IRQ_NONE` if its device did not cause the interrupt, so other handlers can be tried and the IRQ core can detect bad/stuck lines. `IRQ_WAKE_THREAD` only makes sense when a `thread_fn` was registered.

**API / Code Anchor:**
```c
if (!(status & MY_INT_BIT))
    return IRQ_NONE;

if (needs_thread)
    return IRQ_WAKE_THREAD;

return IRQ_HANDLED;
```

**Production or Debugging Angle:** A shared IRQ handler that always returns `IRQ_HANDLED` can hide other devices' problems and confuse stuck-IRQ detection.

**Common Traps:** Returning `IRQ_WAKE_THREAD` without a valid threaded handler. The hard handler should return it only when the thread exists and more work must run there.

**Follow-up Questions:**
- When is `IRQ_NONE` most important?
- What should the threaded handler return?
- What happens if many interrupts are unhandled?

## Mid-Level Questions

### 5. Explain the parameters to `request_irq()`.
**Short Answer:** `request_irq()` takes a Linux IRQ number, handler callback, flags, a name, and a `dev_id` cookie passed back to the handler and used for freeing/shared IRQs.

**Deep Explanation:** The IRQ number is the Linux-visible interrupt. The handler runs when that IRQ fires. Flags describe sharing and trigger behavior. The name appears in diagnostics such as `/proc/interrupts`. `dev_id` should point to private device state; it identifies this handler, especially on shared lines.

**API / Code Anchor:**
```c
int request_irq(unsigned int irq, irq_handler_t handler,
                unsigned long flags, const char *name, void *dev_id);

free_irq(irq, dev_id);
```

**Production or Debugging Angle:** Use a meaningful name and a unique private-data pointer. For managed lifetimes, prefer `devm_request_irq()` but still disable device interrupt generation during remove/suspend.

**Common Traps:** Passing `NULL` as `dev_id` for a shared IRQ, or freeing with a different cookie than the one used to request.

**Follow-up Questions:**
- Why does `dev_id` matter for shared IRQs?
- Where does `name` show up?
- What is the difference between `request_irq()` and `devm_request_irq()`?

### 6. When should you use `request_threaded_irq()`?
**Short Answer:** Use it when interrupt handling has a fast hard part plus slower work, especially if the slow part may sleep.

**Deep Explanation:** `request_threaded_irq()` registers an optional primary hard handler and a `thread_fn`. The primary handler runs in hard IRQ context and can return `IRQ_WAKE_THREAD`. The thread runs in process context, so it can use mutexes, I2C/SPI/regmap access, and longer processing.

**API / Code Anchor:**
```c
ret = devm_request_threaded_irq(dev, irq,
                                my_hardirq,
                                my_thread_fn,
                                IRQF_ONESHOT,
                                "mydev", priv);
```

**Production or Debugging Angle:** Threaded IRQs are usually the right answer for I2C/SPI devices with interrupt pins. They also reduce hard IRQ latency by moving work into a schedulable thread.

**Common Traps:** Doing everything in the thread but forgetting `IRQF_ONESHOT` for a thread-only level IRQ. That can cause repeated interrupts before the thread clears the device.

**Follow-up Questions:**
- What if `handler` is `NULL` and `thread_fn` is not `NULL`?
- What does the primary handler return to wake the thread?
- Can the threaded handler take a mutex?

### 7. What does `IRQF_ONESHOT` do, and why is it important?
**Short Answer:** `IRQF_ONESHOT` keeps the IRQ line masked after the primary handler until the threaded handler finishes.

**Deep Explanation:** Without it, a level-triggered interrupt can be re-enabled before the threaded handler clears the device-level cause. The device may still assert the line, causing immediate retriggering and storms. With `handler = NULL`, Linux uses a default primary handler that wakes the thread, so `IRQF_ONESHOT` is normally required.

**API / Code Anchor:**
```c
ret = request_threaded_irq(irq, NULL, my_thread,
                           IRQF_ONESHOT | IRQF_TRIGGER_LOW,
                           "sensor", priv);
```

**Production or Debugging Angle:** If `/proc/interrupts` increments rapidly and the threaded handler barely makes progress, inspect `IRQF_ONESHOT`, trigger type, and device status-clear order.

**Common Traps:** Thinking `IRQF_ONESHOT` clears the device interrupt. It only controls IRQ masking at the IRQ core/controller level; your driver still must clear the hardware condition.

**Follow-up Questions:**
- Why is it especially important for level-triggered IRQs?
- Is it used for chained parent IRQ handlers?
- What must the threaded handler return?

### 8. Compare threaded IRQs and workqueues as bottom halves.
**Short Answer:** Both run in process context and can sleep, but threaded IRQs are tied directly to an IRQ action while workqueues are general deferred work.

**Deep Explanation:** A threaded IRQ is part of the IRQ lifecycle: the primary handler can wake exactly that IRQ's thread, and `IRQF_ONESHOT` coordinates masking until it completes. A workqueue is more general: a hard handler can queue work for later processing, batching, cancellation, or shared worker execution.

**API / Code Anchor:**
```c
/* threaded IRQ */
return IRQ_WAKE_THREAD;

/* workqueue */
INIT_WORK(&priv->work, work_fn);
schedule_work(&priv->work);
```

**Production or Debugging Angle:** Use threaded IRQ when the deferred work is the interrupt handling path itself, especially for slow bus status/clear. Use workqueues when processing can be batched or has its own lifecycle and cancellation model.

**Common Traps:** Calling `schedule_work()` while holding a spinlock for too long is usually okay only if the lock is released quickly; doing the slow work under the spinlock is not.

**Follow-up Questions:**
- Which one can take a mutex?
- How do you cancel work during remove?
- Why are tasklets less favored in new drivers?

### 9. How do you handle a shared interrupt line correctly?
**Short Answer:** Request with `IRQF_SHARED`, pass a unique `dev_id`, check device status first, return `IRQ_NONE` if not yours, and clear only your device's cause.

**Deep Explanation:** On a shared IRQ, the IRQ core walks registered handlers. Each driver must prove whether its device caused the interrupt. The `dev_id` identifies the handler instance and is required for removal from the shared action list.

**API / Code Anchor:**
```c
ret = devm_request_irq(dev, irq, my_irq,
                       IRQF_SHARED, "mydev", priv);

static irqreturn_t my_irq(int irq, void *dev_id)
{
    struct mydev *priv = dev_id;
    u32 status = readl(priv->base + STATUS);

    if (!(status & MY_INT))
        return IRQ_NONE;

    writel(MY_INT, priv->base + CLEAR);
    return IRQ_HANDLED;
}
```

**Production or Debugging Angle:** Shared IRQ flag mismatches can make registration fail. A handler that lies with `IRQ_HANDLED` can hide real interrupt sources and break diagnostics.

**Common Traps:** Assuming `irq` number alone identifies your device. On shared lines, it identifies the line, not the device.

**Follow-up Questions:**
- Why must `dev_id` not be `NULL`?
- What happens if all handlers return `IRQ_NONE` repeatedly?
- How do you debug shared IRQ flag mismatch logs?

### 10. What is an IRQ domain, and why does Linux need it?
**Short Answer:** An IRQ domain maps controller-local hardware IRQ numbers to Linux IRQ numbers.

**Deep Explanation:** Modern systems have many interrupt controllers. Each controller may number its local sources from zero, so hwirq values are not globally unique. Linux uses a separate Linux IRQ number space for requested interrupts, and IRQ domains provide the translation.

**API / Code Anchor:**
```c
domain = irq_domain_add_linear(np, nr_irqs, &ops, priv);
virq = irq_create_mapping(domain, hwirq);
virq = irq_find_mapping(domain, hwirq);
```

**Production or Debugging Angle:** GPIO controllers, PMICs, MFDs, and cascaded interrupt controllers commonly need IRQ domains. If a child device cannot request its interrupt, check whether the provider registered the domain and mappings.

**Common Traps:** Treating hwirq `5` from GPIO1 and hwirq `5` from GPIO2 as the same interrupt. They are local to different domains.

**Follow-up Questions:**
- What does `.xlate` do?
- When is a linear domain appropriate?
- What is the difference between `irq_create_mapping()` and `irq_find_mapping()`?

## Senior Questions

### 11. Compare chained and nested interrupts.
**Short Answer:** Chained interrupts dispatch child IRQs directly in hard IRQ context for fast controllers. Nested interrupts dispatch child IRQs from a threaded parent context for sleepable controllers.

**Deep Explanation:** Chained handlers are typical for MMIO SoC GPIO controllers. The parent handler cannot sleep, so it reads a fast status register and calls child handlers using generic IRQ dispatch. Nested handlers are typical for I2C/SPI expanders. The parent IRQ is requested as threaded; the parent thread reads status over the bus and calls `handle_nested_irq()` for children.

**API / Code Anchor:**
```c
/* chained */
irq_set_chained_handler_and_data(parent, parent_handler, data);
chained_irq_enter(chip, desc);
generic_handle_irq(child);
chained_irq_exit(chip, desc);

/* nested */
devm_request_threaded_irq(dev, parent, NULL, parent_thread,
                          IRQF_ONESHOT, name, priv);
handle_nested_irq(child);
```

**Production or Debugging Angle:** The choice is about whether parent-controller status access may sleep. Choosing chained for an I2C expander is a context bug. Choosing nested for fast MMIO may work but adds latency and complexity.

**Common Traps:** Saying "nested" means normal CPU nested interrupts. In this driver context, it means IRQ child handling is nested under a threaded parent model.

**Follow-up Questions:**
- Why is `generic_handle_irq()` not suitable after an I2C status read in hard IRQ context?
- Which model is more RT-friendly?
- How do Device Tree `interrupt-parent` relationships show the hierarchy?

### 12. A level-triggered IRQ causes an interrupt storm. How do you debug it?
**Short Answer:** Check whether the device-level cause remains asserted: wrong trigger type, missing clear/ack, wrong clear order, masked/unmasked incorrectly, or threaded handler not finishing.

**Deep Explanation:** Level interrupts represent a state. If the hardware status bit or physical line is still active when Linux unmasks the IRQ, it fires again immediately. Debugging must inspect both Linux counters and hardware status registers.

**API / Code Anchor:**
```c
status = readl(base + STATUS);
writel(status, base + CLEAR);
after = readl(base + STATUS);
dev_dbg(dev, "irq status before=%x after=%x\n", status, after);
```

**Production or Debugging Angle:** Use `/proc/interrupts` to confirm the storm, then instrument status reads, clear writes, trigger type, and mask/unmask paths. For threaded handlers, confirm `IRQF_ONESHOT` and that the thread returns.

**Common Traps:** Blaming the IRQ core before proving the device-level condition was cleared. The IRQ core cannot clear your hardware status register for you.

**Follow-up Questions:**
- How would this differ for edge-triggered IRQs?
- What if the clear register is write-one-to-clear?
- What if the interrupt line is shared?

### 13. A driver gets "sleeping function called from invalid context" after enabling IRQs. What is your reasoning path?
**Short Answer:** Find what sleepable function runs from hard IRQ or softirq context, then move it to a threaded IRQ or workqueue.

**Deep Explanation:** The stack trace usually shows the offending path. Common offenders are I2C/SPI transfers, regmap over sleeping buses, `mutex_lock()`, `msleep()`, wait queues, and `GFP_KERNEL` allocation. If the device sits behind a slow bus, request a threaded IRQ and do bus access in the thread.

**API / Code Anchor:**
```c
/* wrong in hard IRQ */
i2c_smbus_read_byte_data(client, REG);

/* right in threaded handler */
devm_request_threaded_irq(dev, irq, NULL, irq_thread,
                          IRQF_ONESHOT, name, priv);
```

**Production or Debugging Angle:** Do not "fix" this by using random atomic variants if the real operation fundamentally sleeps. Change the execution context.

**Common Traps:** Replacing a mutex with a spinlock around I2C access. That makes the bug worse because I2C can still sleep and now a spinlock is held.

**Follow-up Questions:**
- How do you tell from a stack trace that you are in IRQ context?
- Can regmap sleep?
- When would `request_any_context_irq()` help?

### 14. How do you design locking between a hard IRQ handler and userspace/sysfs/process context?
**Short Answer:** Use spinlock variants that disable local interrupts in process context, usually `spin_lock_irqsave()`, and keep the locked section short.

**Deep Explanation:** If process context holds a plain spinlock and the same CPU takes an IRQ whose handler tries to take the same lock, deadlock can occur. Disabling local IRQs while taking the lock in process context prevents that local IRQ from interrupting the critical section. The hard IRQ handler can use `spin_lock()` if only protecting against other CPUs, but `irqsave` is often simpler and safer for shared paths.

**API / Code Anchor:**
```c
unsigned long flags;

spin_lock_irqsave(&priv->lock, flags);
priv->shared_state = value;
spin_unlock_irqrestore(&priv->lock, flags);
```

**Production or Debugging Angle:** Never hold this spinlock across sleeping operations. For threaded-only data paths, a mutex may be better.

**Common Traps:** Using mutexes for data touched by a hard IRQ handler, or holding a spinlock while calling callbacks that can sleep.

**Follow-up Questions:**
- What if the data is shared only between threaded IRQ and process context?
- What lock is appropriate for softirq/tasklet sharing?
- Why does lock ordering matter during teardown?

### 15. How should an IRQ-consuming driver's remove path be designed?
**Short Answer:** Stop the hardware from generating interrupts, cancel deferred work, then release the IRQ or let devm cleanup release it.

**Deep Explanation:** Managed IRQ cleanup removes the handler at detach, but the device can still assert its interrupt line if you leave it enabled. Remove/suspend paths must mask or disable the device-level source first, then synchronize with work or threaded handling before freeing dependent resources.

**API / Code Anchor:**
```c
static void foo_remove(struct platform_device *pdev)
{
    struct foo *foo = platform_get_drvdata(pdev);

    foo_disable_interrupts(foo);
    cancel_work_sync(&foo->work);
    /* devm_request_irq() cleanup runs after remove */
}
```

**Production or Debugging Angle:** Use `free_irq()`/`devm_free_irq()` explicitly only if the IRQ must be released before normal device cleanup. Make sure private data remains valid until handlers and deferred work are done.

**Common Traps:** Freeing MMIO/register resources while an IRQ handler or workqueue can still access them.

**Follow-up Questions:**
- What does `free_irq()` wait for?
- Does it wait for all deferred work you scheduled yourself?
- What should happen in suspend if this IRQ is not a wake source?

### 16. How do PCI MSI/MSI-X interrupts relate to generic interrupt management?
**Short Answer:** MSI/MSI-X are bus-specific ways for PCI devices to signal interrupts, but Linux still presents allocated vectors as IRQs that drivers request and handle.

**Deep Explanation:** Legacy PCI INTx uses shared interrupt lines. MSI/MSI-X use in-band PCIe messages rather than physical interrupt pins, usually giving devices unique vectors and reducing shared-line ambiguity. Once allocated by PCI APIs, the driver's handler logic still follows normal IRQ rules: short hard handler, proper return values, and deferral when needed.

**API / Code Anchor:**
```text
PCI-specific allocation -> Linux IRQ/vector -> request handler -> same IRQ rules
```

**Production or Debugging Angle:** PCI vector allocation and MSI-X queue affinity belong to PCI driver design, but handler correctness is still generic interrupt-management knowledge.

**Common Traps:** Thinking MSI message data is arbitrary payload for the driver. It is used by the platform/chipset to deliver the interrupt, not as driver event data.

**Follow-up Questions:**
- Why are legacy INTx lines harder to share?
- Why is MSI-X useful for high-performance devices?
- Where should PCI-specific interrupt allocation be studied next?
