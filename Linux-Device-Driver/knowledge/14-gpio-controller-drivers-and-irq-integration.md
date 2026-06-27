# 14 - GPIO Controller Drivers And IRQ Integration

## Learning Goal
After this topic, you should be able to write the shape of a GPIO controller driver, explain how `struct gpio_chip` exposes GPIO lines to the kernel, and choose the right IRQ integration model when those GPIO lines can also generate interrupts.

The main skill is understanding **provider-side GPIO**. Topic 13 taught how a driver consumes GPIOs. This topic teaches how a controller driver provides them.

## Why This Matters In Real Work
Many embedded boards use GPIO banks and GPIO expanders everywhere: SoC GPIO controllers, I2C/SPI expanders, PMIC GPIO blocks, audio-codec GPIOs, FPGA GPIO banks, and reset/interrupt aggregators.

This topic shows up when:

- A new board has an I2C GPIO expander and Linux must expose it as `/dev/gpiochipN`.
- A button or sensor interrupt comes through an expander, not directly from the SoC interrupt controller.
- A controller has 16 GPIO pins but only one parent interrupt line.
- A GPIO interrupt works on an SoC bank but breaks on an I2C expander because the handler sleeps.
- An interrupt storm happens because the GPIO IRQ status bit is never cleared.

**Production rule:** a GPIO controller driver must be honest about hardware context. If register access may sleep, set `can_sleep` and use threaded/nested IRQ handling.

## Mental Model
A GPIO controller driver is a provider. It registers a bank of GPIO lines with gpiolib. Consumers ask for a GPIO by descriptor or legacy number, and gpiolib calls the provider's callbacks.

```text
Consumer driver
  -> gpiod_get(), gpiod_set_value(), gpiod_to_irq()
  -> gpiolib core
  -> struct gpio_chip callbacks
  -> hardware registers or I2C/SPI transactions
```

If the same hardware can generate interrupts, it is also a small interrupt controller:

```text
GPIO expander has 16 lines
  -> each line may be a child IRQ
  -> expander has one parent INT pin
  -> parent IRQ handler reads status bits
  -> dispatches the matching child IRQs
```

## Core Concepts
The GPIO provider side is about translating generic kernel operations into hardware-specific register operations.

| Concept | Meaning | Example |
| --- | --- | --- |
| GPIO controller | Hardware block that provides GPIO lines | SoC GPIO bank, MCP23016 expander |
| `struct gpio_chip` | Kernel object representing one GPIO controller | registered with gpiolib |
| Offset | Line number local to one chip | `0..ngpio-1` |
| Global GPIO number | Legacy system-wide integer | fragile, avoid in new drivers |
| `can_sleep` | Says GPIO callbacks may sleep | true for I2C/SPI expanders |
| GPIO IRQ | A GPIO line exposed as a Linux IRQ | button interrupt on GPIO 8 |
| hwirq | IRQ number local to an interrupt controller | often GPIO offset |
| virq | Linux virtual IRQ number | used by `request_irq()` |
| IRQ domain | Mapping between hwirq and virq | per interrupt controller |
| `irq_chip` | Operations for interrupt masking, ack, type, wake | GPIO IRQ controller ops |

### Provider vs Consumer
A consumer driver uses a GPIO line. A controller driver provides the lines.

| Driver role | Typical APIs |
| --- | --- |
| GPIO consumer | `devm_gpiod_get()`, `gpiod_set_value()`, `gpiod_to_irq()` |
| GPIO controller/provider | `struct gpio_chip`, `devm_gpiochip_add_data()`, GPIO callbacks |
| IRQ-capable GPIO provider | `struct gpio_irq_chip`, `struct irq_chip`, `irq_domain`, `generic_handle_irq()`, `handle_nested_irq()` |

### Chained vs Nested IRQs
The choice depends on whether the GPIO controller can be accessed in hard IRQ context.

| Case | Controller type | Parent handler context | Child dispatch | Can sleep? |
| --- | --- | --- | --- | --- |
| Chained | MMIO SoC GPIO bank | hard IRQ / atomic | `generic_handle_irq()` | no |
| Nested | I2C/SPI expander | threaded IRQ | `handle_nested_irq()` | yes |

**Interview trap:** `IRQF_ONESHOT` belongs to threaded parent IRQ requests. A chained parent IRQ is installed with chaining helpers, not `request_threaded_irq()`.

## Kernel Mechanism
Gpiolib stores a `struct gpio_chip` for each GPIO controller. When a consumer acts on a GPIO line, the core converts the request into a chip-local offset and invokes the controller callback.

For example:

```text
gpiod_direction_output(desc, 1)
  -> desc points to a chip and offset
  -> gpiolib checks ownership/context
  -> chip->direction_output(chip, offset, 1)
  -> controller driver writes hardware register
```

The controller driver usually embeds `struct gpio_chip` inside private data:

```c
struct my_gpio {
    struct gpio_chip gc;
    void __iomem *base;        /* MMIO controller */
    struct mutex lock;         /* for sleepable bus controllers */
    u32 cached_output;
};
```

Callbacks receive an **offset**, not a global GPIO number. If `base = 64` and the consumer uses legacy GPIO 67, the callback receives offset `3`.

For IRQ support, gpiolib and the IRQ core add another layer:

```text
GPIO line offset 8
  -> hwirq 8 in this GPIO controller's IRQ domain
  -> virq 143 in Linux IRQ space
  -> consumer requests virq 143
```

When a parent IRQ fires:

```text
parent IRQ handler
  -> read controller interrupt status
  -> for each active status bit:
       child = irq_find_mapping(domain, offset)
       dispatch child IRQ
  -> clear or ack hardware status
```

## Key Structs And APIs
These are the APIs to understand in context, not memorize as a flat list.

### `struct gpio_chip`
`struct gpio_chip` is the provider object registered with gpiolib.

| Field / callback | Role |
| --- | --- |
| `label` | Diagnostic name shown in debug/sysfs outputs |
| `parent` or older `dev` field | Device owning this chip |
| `base` | Legacy base GPIO number; use `-1` for dynamic allocation |
| `ngpio` | Number of GPIO lines in the controller |
| `names` | Optional line names |
| `can_sleep` | True if callbacks may sleep |
| `.direction_input` | Configure one line as input |
| `.direction_output` | Configure one line as output with initial value |
| `.get` | Read current value |
| `.set` | Set output value |
| `.get_direction` | Report direction if hardware supports it |
| `.set_multiple` | Efficiently update several output lines |
| `.set_config` / `.set_debounce` | Optional electrical/debounce configuration |
| `.to_irq` | Legacy/manual GPIO-to-IRQ mapping hook |

Register with:

```c
ret = devm_gpiochip_add_data(dev, &priv->gc, priv);
```

Retrieve private data in callbacks:

```c
struct my_gpio *priv = gpiochip_get_data(gc);
```

### `struct irq_chip`
`struct irq_chip` describes how the hardware interrupt controller is controlled.

| Callback | Common job |
| --- | --- |
| `.irq_mask` | Disable one child interrupt source |
| `.irq_unmask` | Enable one child interrupt source |
| `.irq_ack` | Acknowledge or clear one interrupt |
| `.irq_set_type` | Program edge/level/polarity |
| `.irq_set_wake` | Enable wakeup from suspend |
| `.irq_bus_lock` / `.irq_bus_sync_unlock` | Protect slow bus register updates |

Simple controllers may use a dummy chip in learning examples, but production drivers should implement real mask/unmask/ack/type operations when hardware supports them.

### IRQ Domains
An IRQ domain maps local hardware IRQs to Linux IRQ numbers.

| API | Role |
| --- | --- |
| `irq_domain_add_linear()` | Create a small fixed-size hwirq-to-virq domain |
| `irq_create_mapping(domain, hwirq)` | Create or return a virq for one hwirq |
| `irq_find_mapping(domain, hwirq)` | Find an existing virq |
| `irq_set_chip_data()` | Attach private data to a virq |
| `irq_set_chip_and_handler()` | Assign irqchip and flow handler |
| `irq_set_nested_thread()` | Mark a child IRQ as nested/threaded |

### Gpiolib IRQ Integration
Older examples often show direct helper calls such as:

- `gpiochip_irqchip_add()`
- `gpiochip_irqchip_add_nested()`
- `gpiochip_set_chained_irqchip()`
- `gpiochip_set_nested_irqchip()`

Current kernel documentation prefers filling `struct gpio_irq_chip` embedded in `gpio_chip.irq` before registering the chip, so gpiolib sets up the GPIO and IRQ parts together.

Important `struct gpio_irq_chip` ideas:

| Member | Role |
| --- | --- |
| `chip` | Driver-provided `struct irq_chip` |
| `handler` | Default child flow handler, often `handle_bad_irq` until type is set |
| `default_type` | Usually `IRQ_TYPE_NONE` |
| `parent_handler` | Parent handler for chained/cascaded controllers |
| `parents` / `num_parents` | Parent IRQ line or lines |
| `threaded` | Nested/threaded behavior |
| `need_valid_mask` / `valid_mask` | Mark only some GPIO lines as IRQ-capable |

## Lifecycle / Data Flow
The lifecycle is probe-time registration followed by callback-driven runtime behavior.

### GPIO-Only Controller
```text
1. probe()
2. allocate private data
3. map registers or initialize bus/regmap access
4. initialize locks and cached register state
5. fill struct gpio_chip
6. set gc.base = -1, gc.ngpio, gc.can_sleep
7. set direction/get/set callbacks
8. devm_gpiochip_add_data()
9. consumers can request and use GPIO descriptors
```

### MMIO Controller With Chained IRQ
```text
1. setup gpio_chip
2. setup irq_chip
3. setup gpio_chip.irq / gpio_irq_chip parent information
4. register gpio_chip
5. parent IRQ fires
6. chained parent handler reads MMIO status
7. for each active bit, call generic_handle_irq(child_virq)
8. ack/clear status without sleeping
```

### I2C/SPI Expander With Nested IRQ
```text
1. setup gpio_chip with can_sleep = true
2. setup irq_chip and nested/threaded GPIO IRQ configuration
3. register gpio_chip
4. request parent IRQ with devm_request_threaded_irq()
5. parent thread reads interrupt status over I2C/SPI
6. for each active bit, call handle_nested_irq(child_virq)
7. clear/ack status over the bus
```

## Minimal Practical Example
This is learning-only pseudo-code. It shows the shape of an I2C GPIO expander with nested IRQ support, not a copy-paste production driver.

```c
struct expander {
    struct i2c_client *client;
    struct gpio_chip gc;
    struct irq_chip irqchip;
    struct mutex lock;
    u16 out_cache;
    u16 dir_cache;
};

static int exp_direction_output(struct gpio_chip *gc,
                                unsigned int offset, int value)
{
    struct expander *ex = gpiochip_get_data(gc);
    int ret;

    mutex_lock(&ex->lock);

    if (value)
        ex->out_cache |= BIT(offset);
    else
        ex->out_cache &= ~BIT(offset);

    ret = exp_write_outputs(ex, ex->out_cache);
    if (!ret) {
        ex->dir_cache &= ~BIT(offset);
        ret = exp_write_direction(ex, ex->dir_cache);
    }

    mutex_unlock(&ex->lock);
    return ret;
}

static int exp_get(struct gpio_chip *gc, unsigned int offset)
{
    struct expander *ex = gpiochip_get_data(gc);
    int ret;

    ret = exp_read_inputs(ex);
    if (ret < 0)
        return 0; /* real drivers should consider cached/error policy */

    return !!(ret & BIT(offset));
}

static irqreturn_t exp_irq_thread(int irq, void *data)
{
    struct expander *ex = data;
    u16 pending;
    int bit;

    pending = exp_read_irq_status(ex);      /* I2C: may sleep */

    for (bit = 0; bit < ex->gc.ngpio; bit++) {
        unsigned int child = irq_find_mapping(ex->gc.irq.domain, bit);

        if (!(pending & BIT(bit)))
            continue;

        if (child)
            handle_nested_irq(child);
    }

    exp_clear_irq_status(ex, pending);      /* I2C: may sleep */
    return IRQ_HANDLED;
}

static int exp_probe(struct i2c_client *client)
{
    struct expander *ex;
    struct gpio_irq_chip *girq;
    int ret;

    ex = devm_kzalloc(&client->dev, sizeof(*ex), GFP_KERNEL);
    if (!ex)
        return -ENOMEM;

    mutex_init(&ex->lock);
    ex->client = client;

    ex->gc.label = dev_name(&client->dev);
    ex->gc.parent = &client->dev;
    ex->gc.base = -1;
    ex->gc.ngpio = 16;
    ex->gc.can_sleep = true;
    ex->gc.direction_output = exp_direction_output;
    ex->gc.direction_input = exp_direction_input;
    ex->gc.get = exp_get;
    ex->gc.set = exp_set;

    ex->irqchip.name = "expander";
    ex->irqchip.irq_mask = exp_irq_mask;
    ex->irqchip.irq_unmask = exp_irq_unmask;
    ex->irqchip.irq_set_type = exp_irq_set_type;

    if (client->irq > 0) {
        ret = devm_request_threaded_irq(&client->dev, client->irq,
                                        NULL, exp_irq_thread,
                                        IRQF_ONESHOT,
                                        dev_name(&client->dev), ex);
        if (ret)
            return ret;

        girq = &ex->gc.irq;
        gpio_irq_chip_set_chip(girq, &ex->irqchip);
        girq->handler = handle_bad_irq;
        girq->default_type = IRQ_TYPE_NONE;
        girq->threaded = true;
    }

    return devm_gpiochip_add_data(&client->dev, &ex->gc, ex);
}
```

Important ideas in the example:

- `can_sleep = true` because I2C transfers can sleep.
- Register read-modify-write paths use a mutex.
- The parent IRQ is threaded and uses `IRQF_ONESHOT`.
- The parent thread demultiplexes GPIO interrupt status into child IRQs.
- Current-style code configures `gc.irq` before `devm_gpiochip_add_data()`.

### Device Tree Shape
An IRQ-capable GPIO provider needs both GPIO-provider and interrupt-provider properties.

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

sensor@48 {
    compatible = "vendor,sensor";
    reg = <0x48>;
    interrupt-parent = <&expander>;
    interrupts = <8 IRQ_TYPE_EDGE_FALLING>;
};
```

## Common Bugs And Debugging
Start with the symptom. GPIO/IRQ bugs often look like "nothing happens", but the failing layer can be pinctrl, DT, gpiolib, IRQ domain mapping, hardware status clear, or locking.

| Symptom | Likely causes | Evidence to inspect |
| --- | --- | --- |
| GPIO chip never appears | probe failed, missing compatible, missing bus device, registration error | `dmesg`, `dev_err_probe()`, `/sys/bus/*/devices` |
| Line exists but value is wrong | wrong offset, active-low confusion, direction wrong, pinmux wrong | `gpioinfo`, `/sys/kernel/debug/gpio`, oscilloscope |
| `gpiod_to_irq()` fails | line not IRQ-capable, missing `interrupt-controller`, bad `#interrupt-cells` | DT, driver IRQ setup, return code |
| Interrupt never fires | parent IRQ not wired, status masked, wrong trigger, missing mapping | `/proc/interrupts`, dynamic debug, DT interrupt properties |
| Interrupt storm | status not cleared, wrong level polarity, unmask before ack, shared IRQ issue | rapidly increasing `/proc/interrupts`, logs, hardware status |
| Sleep warning in IRQ | I2C/SPI access in hard IRQ, wrong chained/nested choice | lockdep, "sleeping function called from invalid context" |
| Race or corrupted register cache | missing mutex/spinlock around read-modify-write | sporadic wrong output values, KCSAN/lockdep clues |

Useful commands and files:

```bash
cat /sys/kernel/debug/gpio
gpiodetect
gpioinfo
cat /proc/interrupts
ls /proc/irq/<irq>/
```

Debugging patterns:

- Add `dev_dbg()` around probe, GPIO callbacks, IRQ mask/unmask, IRQ type setup, parent handler status reads, and mapping lookup.
- Use dynamic debug to enable driver logs without rebuilding.
- Check the parent IRQ count and child IRQ count separately.
- For level-triggered IRQs, confirm the device-level status is cleared before returning.
- For I2C/SPI expanders, confirm every path that touches bus registers runs in process/thread context.

## Production Checklist
Before review or board bring-up, verify the controller as a provider and as an optional IRQ controller.

- Use `base = -1`; do not hard-code global GPIO numbering for new drivers.
- Set `ngpio` correctly and use offsets inside every callback.
- Set `can_sleep` correctly.
- Use `devm_gpiochip_add_data()` when the lifetime matches the device.
- Use `gpiochip_get_data()` or `container_of()` consistently.
- Protect register read-modify-write with the right lock:
  - mutex for sleepable I2C/SPI/regmap paths;
  - spinlock/raw spinlock for MMIO paths used in atomic context.
- Set output value before changing direction to output when hardware allows it, to avoid glitches.
- Implement `get_direction`, `set_multiple`, `set_config`, debounce, line names, and wake support when hardware supports them.
- If only some lines can generate IRQs, expose that with the current valid-mask mechanism.
- For MMIO IRQ controllers, never call sleeping APIs from the chained parent handler.
- For bus expanders, request the parent IRQ as threaded and use `IRQF_ONESHOT`.
- Implement real `irq_chip` mask/unmask/ack/type callbacks for production hardware.
- Validate DT binding properties: `gpio-controller`, `#gpio-cells`, optional `interrupt-controller`, `#interrupt-cells`, parent IRQ wiring, and trigger flags.
- Confirm with `gpioinfo`, `/sys/kernel/debug/gpio`, and `/proc/interrupts` during bring-up.

## Interview Readiness
You should be able to explain the provider side without reciting callback names.

Be ready to answer:

- Why consumers use descriptors while providers implement `gpio_chip`.
- Why GPIO callbacks receive offsets.
- Why `can_sleep` changes which APIs and IRQ model are safe.
- How a single parent IRQ can represent many GPIO child IRQs.
- Why hwirq and virq are different.
- When to use chained vs nested interrupt handling.
- Why a threaded IRQ usually needs `IRQF_ONESHOT`.
- How to debug a GPIO interrupt that never fires or fires forever.

Practice with [14-gpio-controller-drivers-and-irq-integration.md](/home/tungnhs/TungNHS/Knowlege-Interview/Linux-Device-Driver/interview/14-gpio-controller-drivers-and-irq-integration.md).

## Kernel Version Notes
GPIO controller and GPIO irqchip APIs have changed over kernel versions.

- Older examples may show direct `gpio_chip` fields such as `irqdomain` or direct helper calls like `gpiochip_irqchip_add_nested()`.
- Current kernel documentation emphasizes configuring the embedded `struct gpio_irq_chip` through `gpio_chip.irq` before `gpiochip_add_data()` / `devm_gpiochip_add_data()`.
- Check the target kernel headers and in-tree GPIO drivers before writing production code.
- GPIO sysfs is legacy for userspace. Prefer the GPIO character device and libgpiod tools for new userspace workflows.
