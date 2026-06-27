# 14 - GPIO Controller Drivers And IRQ Integration Interview Questions

Strong candidates can separate GPIO consumers from GPIO providers, explain how gpiolib calls `gpio_chip` callbacks, reason about GPIO IRQ domains, and choose chained or nested IRQ handling based on hardware context.

## Beginner Questions

### 1. What is a GPIO controller driver?
**Short Answer:** A GPIO controller driver is a provider driver that registers a bank of GPIO lines with gpiolib using `struct gpio_chip`.

**Deep Explanation:** Consumer drivers ask for GPIOs using APIs such as `gpiod_get()`. They do not know how the hardware register or I2C expander works. The GPIO controller driver sits underneath gpiolib and implements operations such as setting direction, reading input value, writing output value, and optionally mapping a GPIO line to an IRQ.

**API / Code Anchor:**
```c
struct gpio_chip gc;

gc.base = -1;
gc.ngpio = 16;
gc.can_sleep = true;
gc.direction_input = my_direction_input;
gc.direction_output = my_direction_output;
gc.get = my_get;
gc.set = my_set;

ret = devm_gpiochip_add_data(dev, &gc, priv);
```

**Production or Debugging Angle:** If a GPIO expander does not appear in `gpioinfo` or `/sys/kernel/debug/gpio`, first check whether the provider driver probed and registered its `gpio_chip`.

**Common Traps:** Describing only consumer APIs. `gpiod_get()` is not how a provider exposes GPIO lines; it is how another driver consumes them.

**Follow-up Questions:**
- What is the difference between a GPIO provider and a GPIO consumer?
- Why should `base` usually be `-1`?
- Where do GPIO controller line names show up?

### 2. Why do `gpio_chip` callbacks receive an offset instead of a global GPIO number?
**Short Answer:** The offset is local to the controller, so callbacks can access hardware registers using `0..ngpio-1`.

**Deep Explanation:** Legacy integer GPIO numbers are global and may depend on registration order. A controller's hardware usually knows only local line indexes. Gpiolib converts the consumer's GPIO descriptor or legacy number into a `gpio_chip` plus a local offset before calling provider callbacks.

**API / Code Anchor:**
```c
static int my_get(struct gpio_chip *gc, unsigned int offset)
{
    struct my_gpio *priv = gpiochip_get_data(gc);

    return !!(readl(priv->base + DATA_REG) & BIT(offset));
}
```

**Production or Debugging Angle:** If GPIO 67 maps to a chip with base 64, the callback should operate on offset 3, not 67.

**Common Traps:** Using global GPIO numbers inside provider callbacks. That breaks dynamic numbering and often reads the wrong hardware bit.

**Follow-up Questions:**
- What is `ngpio`?
- Why is dynamic GPIO numbering preferred?
- How does descriptor-based GPIO reduce dependence on global numbers?

### 3. What does `can_sleep` mean in a GPIO controller?
**Short Answer:** `can_sleep` tells the GPIO core and users that this controller's callbacks may sleep.

**Deep Explanation:** SoC GPIO controllers are usually memory-mapped, so reads and writes are quick register accesses and can run in atomic context. I2C/SPI GPIO expanders need bus transfers, which may sleep. A controller driver must set `can_sleep = true` for such hardware so consumers and IRQ code use sleep-safe paths.

**API / Code Anchor:**
```c
gc.can_sleep = true;  /* I2C/SPI expander */
```

**Production or Debugging Angle:** A wrong `can_sleep` value can cause "sleeping function called from invalid context" warnings or broken interrupt handling.

**Common Traps:** Assuming all GPIOs are like SoC GPIOs. The same board signal might come from an I2C expander on another board.

**Follow-up Questions:**
- Which GPIO value APIs should consumers use for sleepable GPIOs?
- Can an I2C transfer run in hard IRQ context?
- How does `can_sleep` influence chained vs nested IRQ handling?

### 4. What does it mean for a GPIO controller to also be an interrupt controller?
**Short Answer:** It means GPIO lines can generate child IRQs, and the controller maps those local GPIO IRQs into Linux IRQ numbers.

**Deep Explanation:** A GPIO expander may have 16 input lines but only one physical INT output to the SoC. When any enabled GPIO line changes, the expander asserts the parent interrupt. The GPIO controller driver reads a status register, finds which GPIO lines caused the event, maps each local line to a Linux virq, and dispatches the child IRQ.

**API / Code Anchor:**
```c
child = irq_find_mapping(gc->irq.domain, offset);
handle_nested_irq(child);
```

**Production or Debugging Angle:** `/proc/interrupts` may show both the parent IRQ line and child IRQ handlers. Debug both levels.

**Common Traps:** Thinking the single parent IRQ is the same as every GPIO child IRQ. It is only the upstream signal.

**Follow-up Questions:**
- What is an hwirq?
- What is a virq?
- Why does a GPIO IRQ controller need an IRQ domain?

## Mid-Level Questions

### 5. What are the required pieces of a minimal GPIO controller driver?
**Short Answer:** Private data, hardware access, `struct gpio_chip`, direction callbacks, get/set callbacks, correct `ngpio`, correct `can_sleep`, and registration with gpiolib.

**Deep Explanation:** The driver must translate generic GPIO operations into hardware-specific operations. At probe, it allocates private state, maps registers or initializes bus/regmap access, initializes locks and caches, fills `gpio_chip`, then calls `gpiochip_add_data()` or the managed variant. Runtime operations enter through callbacks.

**API / Code Anchor:**
```c
gc.direction_input = my_direction_input;
gc.direction_output = my_direction_output;
gc.get = my_get;
gc.set = my_set;
ret = devm_gpiochip_add_data(dev, &gc, priv);
```

**Production or Debugging Angle:** Optional callbacks matter in real drivers: `get_direction`, `set_multiple`, `set_config`, debounce, line names, and IRQ support can affect correctness and performance.

**Common Traps:** Omitting locking around read-modify-write registers or setting output direction before setting a safe output value.

**Follow-up Questions:**
- How do you retrieve private data inside a callback?
- When would you implement `set_multiple()`?
- Why should output value often be programmed before output direction?

### 6. How does a GPIO line become a Linux IRQ?
**Short Answer:** The GPIO controller maps the GPIO line's local hwirq, often the GPIO offset, into a Linux virq using an IRQ domain.

**Deep Explanation:** Linux IRQ users request virq numbers, not controller-local hwirqs. The GPIO IRQ provider creates or uses an IRQ domain. When a consumer calls `gpiod_to_irq()`, gpiolib or the provider's `.to_irq` path returns the Linux IRQ mapped from that GPIO line.

**API / Code Anchor:**
```c
irq = gpiod_to_irq(desc);          /* consumer side */

/* provider/manual style */
return irq_create_mapping(domain, offset);
```

**Production or Debugging Angle:** If `gpiod_to_irq()` returns a negative error, check whether the provider registered IRQ support and whether that line is actually IRQ-capable.

**Common Traps:** Assuming every GPIO supports IRQ. Some controllers have only a subset of interrupt-capable lines.

**Follow-up Questions:**
- What does `irq_find_mapping()` do?
- Why is a linear IRQ domain common for GPIO banks?
- How should partial IRQ-capable banks be represented?

### 7. Compare chained and nested GPIO IRQ handling.
**Short Answer:** Chained handling is for fast non-sleeping MMIO controllers and dispatches child IRQs in hard IRQ context. Nested handling is for sleepable bus controllers and dispatches child IRQs from a threaded parent handler.

**Deep Explanation:** A chained parent handler is installed as part of interrupt-controller plumbing and calls `generic_handle_irq()` for active children. It cannot sleep. A nested GPIO expander requests its parent IRQ with `request_threaded_irq()` and uses `handle_nested_irq()` after reading status over I2C/SPI in thread context.

**API / Code Anchor:**
```c
/* chained */
chained_irq_enter(chip, desc);
generic_handle_irq(child);
chained_irq_exit(chip, desc);

/* nested */
devm_request_threaded_irq(dev, parent, NULL, parent_thread,
                          IRQF_ONESHOT, name, priv);
handle_nested_irq(child);
```

**Production or Debugging Angle:** Choose based on whether parent status access may sleep. Wrong choice can produce latency problems or hard IRQ sleep bugs.

**Common Traps:** Using `request_irq()` for a chained parent controller, or using chained handling for an I2C GPIO expander.

**Follow-up Questions:**
- Why does nested handling usually need `IRQF_ONESHOT`?
- Which function dispatches child IRQs in chained mode?
- Which function dispatches child IRQs in nested mode?

### 8. What does `IRQF_ONESHOT` do for a threaded GPIO expander IRQ?
**Short Answer:** It keeps the parent IRQ masked until the threaded handler finishes.

**Deep Explanation:** If the primary handler is `NULL`, the kernel installs a default primary handler that wakes the thread. Without `IRQF_ONESHOT`, a level-triggered interrupt could be unmasked before the thread clears the device status, causing repeated interrupts or storms. Threaded I2C/SPI GPIO expanders usually need this flag.

**API / Code Anchor:**
```c
ret = devm_request_threaded_irq(dev, client->irq,
                                NULL, expander_irq_thread,
                                IRQF_ONESHOT | IRQF_TRIGGER_LOW,
                                dev_name(dev), priv);
```

**Production or Debugging Angle:** If the IRQ count climbs rapidly and the threaded handler cannot clear status in time, check trigger type, `IRQF_ONESHOT`, and hardware ack order.

**Common Traps:** Thinking `IRQF_ONESHOT` is a chained IRQ concept. Chained parent handlers are not requested this way.

**Follow-up Questions:**
- What happens if the device-level status bit is not cleared?
- When is a primary hard IRQ handler needed even with threading?
- Why might shared IRQ lines need consistent flags?

### 9. What should an IRQ-capable GPIO controller node contain in Device Tree?
**Short Answer:** It should declare itself as both a GPIO provider and an interrupt provider, and describe its parent interrupt line.

**Deep Explanation:** `gpio-controller` and `#gpio-cells` tell gpiolib how consumers reference GPIO lines. `interrupt-controller` and `#interrupt-cells` tell the IRQ core that this node can provide child interrupts. `interrupt-parent` and `interrupts` describe how this GPIO controller is connected upstream.

**API / Code Anchor:**
```dts
expander: gpio@20 {
    compatible = "vendor,my-expander";
    reg = <0x20>;
    gpio-controller;
    #gpio-cells = <2>;
    interrupt-controller;
    #interrupt-cells = <2>;
    interrupt-parent = <&gpio4>;
    interrupts = <29 IRQ_TYPE_LEVEL_LOW>;
};
```

**Production or Debugging Angle:** A child device using `interrupt-parent = <&expander>` will not work unless the expander registered an IRQ domain and DT cells match the driver's xlate/domain setup.

**Common Traps:** Adding `interrupts` to the expander but forgetting `interrupt-controller`, or using the wrong number of interrupt cells.

**Follow-up Questions:**
- What does the first interrupt cell usually represent for a GPIO IRQ provider?
- What does the second cell usually represent?
- How does this differ from a `reset-gpios` property?

### 10. How do you debug a GPIO interrupt that never fires?
**Short Answer:** Check the provider registration, DT parent/child interrupt wiring, pinctrl, line direction, IRQ mapping, mask/type registers, and interrupt counters.

**Deep Explanation:** The failure can be at several layers. The pin may not be muxed as GPIO, the controller may not expose IRQ support, `gpiod_to_irq()` may fail, the parent IRQ may not be requested, the line may be masked, or the trigger type may not match the signal.

**API / Code Anchor:**
```bash
cat /sys/kernel/debug/gpio
gpioinfo
cat /proc/interrupts
```

**Production or Debugging Angle:** Separate parent IRQ activity from child IRQ activity. If the parent count increments but the child does not, inspect status decoding and `irq_find_mapping()`. If neither increments, inspect pinctrl and parent IRQ wiring.

**Common Traps:** Staring only at the consumer driver. GPIO IRQ bugs often live in the provider driver or DT.

**Follow-up Questions:**
- What does it mean if the parent IRQ count increments but no child handler runs?
- What does it mean if `gpiod_to_irq()` returns `-EINVAL`?
- How would dynamic debug help here?

## Senior Questions

### 11. How would you design locking for an MMIO GPIO controller versus an I2C GPIO expander?
**Short Answer:** Use spinlock/raw spinlock for MMIO paths that can run in atomic context, and mutexes for sleepable I2C/SPI paths.

**Deep Explanation:** MMIO GPIO callbacks and chained IRQ callbacks may run where sleeping is illegal. They need atomic-safe locking around shared register state and read-modify-write sequences. I2C/SPI expanders sleep during bus transfers, so their runtime callbacks and threaded IRQ handlers can use mutexes to protect caches and serialized register updates.

**API / Code Anchor:**
```c
raw_spin_lock_irqsave(&priv->lock, flags);
/* MMIO read-modify-write */
raw_spin_unlock_irqrestore(&priv->lock, flags);

mutex_lock(&priv->lock);
/* I2C read-modify-write */
mutex_unlock(&priv->lock);
```

**Production or Debugging Angle:** A wrong lock type is often worse than no lock: a mutex in hard IRQ context can explode, while a spinlock held across I2C can deadlock or trigger sleep warnings.

**Common Traps:** Copying the same locking style between SoC GPIO and I2C expander drivers.

**Follow-up Questions:**
- Why do read-modify-write register updates need protection?
- What if a line can be set from process context while an IRQ callback masks it?
- How does `IRQF_ONESHOT` affect races between a primary and threaded handler?

### 12. How should a modern driver use gpiolib irqchip support?
**Short Answer:** Prefer configuring the embedded `struct gpio_irq_chip` in `gpio_chip.irq` before registering the GPIO chip, then let gpiolib set up the IRQ domain and mapping.

**Deep Explanation:** Older examples often open-coded IRQ domains or used direct helper calls such as `gpiochip_irqchip_add_nested()`. Current kernels provide `struct gpio_irq_chip` inside `struct gpio_chip`. The driver sets the irqchip, parent handler or threaded behavior, default type, parent IRQs, and valid mask if needed, then registers the chip.

**API / Code Anchor:**
```c
struct gpio_irq_chip *girq = &priv->gc.irq;

gpio_irq_chip_set_chip(girq, &priv->irqchip);
girq->default_type = IRQ_TYPE_NONE;
girq->handler = handle_bad_irq;
girq->threaded = true;        /* for nested/sleepable controller */

ret = devm_gpiochip_add_data(dev, &priv->gc, priv);
```

**Production or Debugging Angle:** Always check the target kernel headers and nearby in-tree drivers. GPIO irqchip APIs have changed across kernel versions.

**Common Traps:** Mixing stale field names such as direct `irqdomain` access with current `gpio_chip.irq` layout.

**Follow-up Questions:**
- Why might `handle_bad_irq` be a good initial handler?
- What is `valid_mask` for?
- When would you still need a manual IRQ domain?

### 13. When would you use regmap IRQ instead of hand-written GPIO IRQ handling?
**Short Answer:** Use regmap IRQ when interrupt status/mask/ack/type registers fit the regmap IRQ model, especially in PMIC, MFD, or register-map-based GPIO blocks.

**Deep Explanation:** Regmap IRQ can create an IRQ domain, request a threaded parent IRQ, read status registers, mask/unmask sources, and expose child virqs. It is useful when the device is already using regmap and the interrupt layout is register-bank based. For a plain MMIO GPIO controller with custom chained behavior, hand-written or gpiolib irqchip handling may be better.

**API / Code Anchor:**
```c
ret = devm_regmap_add_irq_chip(dev, regmap, parent_irq,
                               IRQF_ONESHOT, 0,
                               &chip_irq_desc, &irq_data);

virq = regmap_irq_get_virq(irq_data, line);
```

**Production or Debugging Angle:** Regmap IRQ is threaded, so it naturally fits sleepable register access. It also reduces boilerplate but only when the hardware model matches.

**Common Traps:** Forcing regmap IRQ onto hardware with unusual interrupt semantics, or forgetting that this topic is not a replacement for understanding IRQ domains.

**Follow-up Questions:**
- Why is regmap IRQ common in MFD drivers?
- How do child devices receive IRQ resources from an MFD parent?
- What are `status_base`, `mask_base`, and `ack_base` used for?

### 14. Design a GPIO controller where only some lines can generate IRQs. What must you consider?
**Short Answer:** Expose all GPIO lines as GPIOs, but mark only IRQ-capable lines valid for IRQ mapping.

**Deep Explanation:** Some controllers provide GPIO functionality on every line but IRQ functionality only on a subset. The driver must not let consumers obtain IRQs for unsupported lines. Modern gpiolib irqchip support provides valid-mask mechanisms for this kind of partial IRQ support.

**API / Code Anchor:**
```c
girq = &gc->irq;
girq->need_valid_mask = true;
/* clear invalid bits in girq->valid_mask before final IRQ setup */
```

**Production or Debugging Angle:** Without a valid mask, a consumer may successfully request an IRQ for a line that hardware can never signal, producing a silent bring-up failure.

**Common Traps:** Assuming line offset equals interrupt source for every line. That 1:1 mapping is common, not guaranteed.

**Follow-up Questions:**
- How should the binding document IRQ-capable lines?
- What should `gpiod_to_irq()` return for an unsupported line?
- How would you test every line during bring-up?

### 15. A board has an interrupt storm from an I2C GPIO expander. How do you investigate?
**Short Answer:** Check whether the parent IRQ is threaded with `IRQF_ONESHOT`, whether the expander status is cleared correctly, whether trigger polarity matches hardware, and whether mask/unmask callbacks are correct.

**Deep Explanation:** An I2C GPIO expander usually asserts one parent interrupt until its internal cause is handled. If the threaded handler returns without clearing the status or if the level polarity is wrong, the parent interrupt immediately fires again. If mask/unmask is wrong, disabled child IRQs may still trigger. If the handler sleeps in the wrong context, the system may warn or stall.

**API / Code Anchor:**
```c
ret = devm_request_threaded_irq(dev, parent_irq, NULL, exp_irq_thread,
                                IRQF_ONESHOT | IRQF_TRIGGER_LOW,
                                "expander", priv);

pending = exp_read_irq_status(priv);
/* dispatch children */
exp_clear_irq_status(priv, pending);
```

**Production or Debugging Angle:** Compare `/proc/interrupts` before and after enabling one line. Add dynamic debug for status reads, clears, mask/unmask, and `irq_set_type`.

**Common Traps:** Clearing the wrong register, clearing after unmask when hardware requires the reverse, or using edge trigger for a level-low INT line.

**Follow-up Questions:**
- What if the parent count increases but no child count increases?
- What if the child handler runs but the parent stays asserted?
- How would you prove whether the problem is DT trigger type or driver ack logic?

### 16. What are the most important version-sensitive GPIO irqchip issues?
**Short Answer:** Older examples use direct helper calls and old field names; current kernels prefer `gpio_chip.irq` / `struct gpio_irq_chip` setup before chip registration.

**Deep Explanation:** The concepts are stable: `gpio_chip`, `irq_chip`, IRQ domains, chained vs nested handling. But the exact gpiolib helper interface has evolved. A production driver should be written against the target kernel's headers and nearby in-tree drivers, not blindly copied from old notes.

**API / Code Anchor:**
```c
/* current-style idea */
struct gpio_irq_chip *girq = &gc->irq;
gpio_irq_chip_set_chip(girq, irqchip);
ret = devm_gpiochip_add_data(dev, gc, priv);
```

**Production or Debugging Angle:** During review, stale code often shows up as direct access to obsolete fields or helper order that no longer matches the target kernel.

**Common Traps:** Treating all "modern" examples as current. Kernel API drift is real, especially around GPIO irqchip helpers.

**Follow-up Questions:**
- Which in-tree drivers would you inspect before implementation?
- How would you isolate concept learning from target-kernel code accuracy?
- What userspace GPIO interface is preferred over legacy sysfs?

## Debugging Scenarios And Traps

### Scenario A: `gpiod_to_irq()` returns an error for a GPIO line.
**Short Answer:** The provider may not have IRQ support, the line may not be IRQ-capable, or DT/controller setup is incomplete.

**Deep Explanation:** GPIO and IRQ are separate capabilities. A line can be usable as input/output but not as an interrupt. The GPIO provider must expose IRQ mapping through gpiolib/IRQ domain setup, and the DT node must describe interrupt-controller properties when it provides child IRQs.

**API / Code Anchor:**
```c
irq = gpiod_to_irq(desc);
if (irq < 0)
    return irq;
```

**Production or Debugging Angle:** Inspect provider probe logs, `gpioinfo`, `/sys/kernel/debug/gpio`, DT `interrupt-controller`, and any valid-mask logic.

**Common Traps:** Retrying blindly in the consumer driver instead of fixing the provider or DT.

**Follow-up Questions:**
- Can a GPIO output-only line generate interrupts?
- What should the binding say about interrupt support?
- How do you distinguish unsupported IRQ from probe deferral?

### Scenario B: A chained GPIO IRQ handler calls an I2C read and the kernel warns.
**Short Answer:** The driver chose the wrong IRQ model. I2C can sleep, so the parent handler must be threaded/nested, not chained.

**Deep Explanation:** Chained handlers run in hard IRQ context. They are suitable for MMIO status registers, not slow buses. An I2C GPIO expander must request the parent IRQ with a threaded handler and dispatch children with `handle_nested_irq()`.

**API / Code Anchor:**
```c
/* wrong context for I2C */
generic_handle_irq(child);      /* chained path */

/* correct shape for expander */
devm_request_threaded_irq(dev, irq, NULL, thread_fn,
                          IRQF_ONESHOT, name, priv);
handle_nested_irq(child);
```

**Production or Debugging Angle:** Lockdep and "sleeping function called from invalid context" are strong evidence. Fix the architecture, not just the warning.

**Common Traps:** Moving only the I2C read to a workqueue while still pretending the child IRQs are chained.

**Follow-up Questions:**
- Why is `handle_nested_irq()` appropriate here?
- What does `IRQ_NESTED_THREAD` represent?
- Could `request_any_context_irq()` help a consumer of GPIO IRQs?

### Scenario C: GPIO values randomly flip after concurrent users access different lines.
**Short Answer:** The driver probably has an unprotected read-modify-write register sequence or stale cache update.

**Deep Explanation:** Many GPIO controllers pack multiple line states into one register. Setting line 3 may require reading the whole output register, changing one bit, and writing it back. If two contexts do this concurrently without locking, one update can overwrite the other.

**API / Code Anchor:**
```c
mutex_lock(&priv->lock);
cached = update_bit(priv->cached, offset, value);
write_outputs(priv, cached);
mutex_unlock(&priv->lock);
```

**Production or Debugging Angle:** Look for missing locks in `.set`, `.direction_output`, IRQ mask/unmask, and type configuration callbacks.

**Common Traps:** Protecting normal GPIO callbacks but forgetting IRQ callbacks touch the same registers.

**Follow-up Questions:**
- Which lock type would you use for MMIO?
- Which lock type would you use for I2C?
- How can `set_multiple()` reduce race windows and bus traffic?
