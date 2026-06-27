# 13 - Pin Control And GPIO Consumer APIs

## Learning Goal
After this topic, you should be able to explain how a driver gets the right pins and GPIO lines for a board, use descriptor-based GPIO APIs safely, handle active-low polarity, switch pinctrl states for power management, and debug common pin/GPIO bring-up failures.

The main skill is not memorizing every helper. It is knowing **which subsystem owns which job**.

## Why This Matters In Real Work
Many embedded devices are not self-discovering. A sensor, display, modem, Wi-Fi module, codec, or FPGA may need reset GPIOs, enable GPIOs, interrupt GPIOs, and board-specific pin muxing before it can work.

This topic shows up when:

- A device probes, but reset never toggles because the DT property name is wrong.
- A GPIO works on one board but is inverted on another because `GPIO_ACTIVE_LOW` was ignored.
- A button IRQ handler sleeps accidentally because the GPIO comes from an I2C expander.
- Suspend current is high because pins stay in the active state during sleep.
- A driver hard-codes SoC pad registers and becomes impossible to reuse.

**Production rule:** consumer drivers should use pinctrl and GPIO consumer APIs, not poke mux registers or global GPIO numbers directly.

## Mental Model
Think of a physical SoC pin as a configurable pad. The pin can be routed to one function at a time, and it also has electrical settings.

```
Physical pad
  -> pinmux chooses function: UART, I2C, SPI, PWM, GPIO, ...
  -> pinconf chooses electrical behavior: bias, drive strength, open-drain, debounce, ...
  -> GPIO consumer API uses the line if the selected function is GPIO
```

Pinctrl and GPIO are related, but they answer different questions:

| Question | Subsystem |
| --- | --- |
| Which hardware function is connected to this pad? | pinctrl / pinmux |
| What pull-up, drive strength, slew rate, or sleep state should the pad use? | pinctrl / pinconf |
| Does my driver own this digital line? | gpiolib consumer API |
| Is logical 1 active or inactive for this device signal? | GPIO descriptor flags |
| Can this input GPIO become an IRQ? | GPIO plus IRQ integration |

## Core Concepts
Pinctrl describes **pad routing and electrical state**. GPIO describes **digital line ownership and value**.

| Concept | Meaning | Example |
| --- | --- | --- |
| Pin | Physical SoC pad or package line | `GPIO2_7`, `MX6QDL_PAD_SD3_DAT1` |
| Pinmux | Selects which function a pin performs | UART RX vs GPIO input |
| Pinconf | Configures electrical behavior | pull-up, open-drain, drive strength |
| Pin controller | Hardware block plus Linux driver that applies mux/config settings | i.MX IOMUXC, AM335x pinmux, STM32 pinctrl |
| Pinctrl state | Named group of pin settings for a device | `default`, `init`, `sleep`, `idle` |
| GPIO line | Digital input/output line, often one possible pin function | reset, enable, interrupt, LED |
| GPIO descriptor | Opaque `struct gpio_desc *` used by modern consumers | returned by `devm_gpiod_get()` |
| Active-low | Logical active state is physical low voltage | reset asserted by driving low |

### Pinctrl vs GPIO
A common beginner mistake is treating pinctrl and GPIO as the same thing.

| Scenario | Need pinctrl? | Need GPIO API? |
| --- | --- | --- |
| UART pins must be muxed to UART TX/RX | yes | no |
| Reset line must be driven high/low by driver | usually yes, if pad must be muxed as GPIO | yes |
| I2C pins need open-drain and pull-ups | yes | no, unless bit-banging |
| Button input generates interrupt | usually yes | yes |
| Sleep state should reduce leakage | yes | maybe |

## Kernel Mechanism
The board description provides two separate kinds of information: pin state references and GPIO consumer references.

```dts
mydev: device@0 {
    compatible = "vendor,mydev";

    pinctrl-names = "default", "sleep";
    pinctrl-0 = <&mydev_default_pins>;
    pinctrl-1 = <&mydev_sleep_pins>;

    reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
    enable-gpios = <&gpio1 8 GPIO_ACTIVE_HIGH>;
    irq-gpios = <&gpio2 3 GPIO_ACTIVE_LOW>;
};
```

The rough flow is:

```text
DTS pinctrl state nodes
  -> parsed by the pin controller driver
  -> device core binds standard pin states to struct device
  -> default/init states can be selected around probe

DTS *-gpios properties
  -> parsed by gpiolib using the device firmware node
  -> GPIO provider returns a line
  -> consumer receives struct gpio_desc *
  -> descriptor API handles ownership, direction, value, polarity, and sleep rules
```

### Standard Pinctrl States
The kernel recognizes common state names:

| State | Typical use |
| --- | --- |
| `default` | Normal runtime pin configuration |
| `init` | Temporary probe-time configuration before normal runtime |
| `sleep` | Low-power system/runtime suspend pin configuration |
| `idle` | Idle power-management state |

For simple devices, the device core often selects `default` automatically before probe. Drivers usually need explicit pinctrl code only when they switch states themselves, such as during suspend/resume.

## Key Structs And APIs
Use these APIs in context. Do not use this as a raw memorization dump.

### Pinctrl Consumer APIs
Pinctrl consumer APIs let a driver select a named state already described in firmware.

| API / type | Role |
| --- | --- |
| `struct pinctrl` | Handle for a device's pinctrl state collection |
| `struct pinctrl_state` | One named state such as `default` or `sleep` |
| `devm_pinctrl_get(dev)` | Managed get of pinctrl handle |
| `pinctrl_lookup_state(p, "sleep")` | Find a named state |
| `pinctrl_select_state(p, state)` | Apply that state |
| `pinctrl_get_select_default(dev)` | Get and select `default` in one call |

Use explicit pinctrl handling when:

- The driver needs to switch between `default` and `sleep`.
- The hardware requires a special `init` or idle state.
- You are debugging whether a state can be found and selected.

### GPIO Descriptor Consumer APIs
Modern GPIO consumers use descriptors, not global integer numbers.

| API / type | Role |
| --- | --- |
| `struct gpio_desc` | Opaque GPIO line descriptor |
| `devm_gpiod_get(dev, "reset", flags)` | Get `reset-gpios` index 0 |
| `devm_gpiod_get_index(dev, "led", 1, flags)` | Get index 1 from `led-gpios` |
| `devm_gpiod_get_optional(dev, "enable", flags)` | Optional GPIO; `NULL` means absent |
| `gpiod_put(desc)` | Release unmanaged descriptor |
| `gpiod_to_irq(desc)` | Convert an interrupt-capable GPIO to Linux IRQ |

`con_id` maps to the DT property prefix:

| Driver call | DT property searched |
| --- | --- |
| `gpiod_get(dev, "reset", ...)` | `reset-gpios`, also legacy `reset-gpio` |
| `gpiod_get(dev, "enable", ...)` | `enable-gpios`, also legacy `enable-gpio` |
| `gpiod_get_index(dev, "led", 0, ...)` | first entry in `led-gpios` |
| `gpiod_get_index(dev, "led", 1, ...)` | second entry in `led-gpios` |

**Production rule:** prefer `*-gpios` in new bindings. Singular `*-gpio` exists for compatibility, but plural naming is the normal modern style.

### Direction And Initial Value
Set direction and initial output value when requesting the line when possible.

| Flag | Meaning |
| --- | --- |
| `GPIOD_ASIS` | Do not change direction/value |
| `GPIOD_IN` | Configure as input |
| `GPIOD_OUT_LOW` | Configure as output, initial logical 0 |
| `GPIOD_OUT_HIGH` | Configure as output, initial logical 1 |
| `GPIOD_OUT_LOW_OPEN_DRAIN` | Output low, open-drain semantics |
| `GPIOD_OUT_HIGH_OPEN_DRAIN` | Output high, open-drain semantics |

This avoids glitches caused by requesting a GPIO first, then changing direction/value later.

### Logical vs Raw Values
Descriptor GPIO APIs normally use **logical values**.

| DT flag | `gpiod_set_value(desc, 1)` drives | Meaning |
| --- | --- | --- |
| `GPIO_ACTIVE_HIGH` | physical high | active/asserted |
| `GPIO_ACTIVE_LOW` | physical low | active/asserted |

Use raw accessors only when the driver truly needs physical voltage-level control:

- `gpiod_get_raw_value()`
- `gpiod_set_raw_value()`
- `gpiod_get_raw_value_cansleep()`
- `gpiod_set_raw_value_cansleep()`
- `gpiod_direction_output_raw()`

**Interview trap:** if a reset line is `GPIO_ACTIVE_LOW`, then `gpiod_set_value(reset, 1)` asserts reset by driving the physical line low.

### Sleeping vs Atomic GPIO Access
Some GPIO controllers are memory-mapped and can be accessed in atomic context. GPIO expanders behind I2C or SPI may sleep.

| Situation | Use |
| --- | --- |
| Known non-sleeping provider, non-sleeping context | `gpiod_get_value()`, `gpiod_set_value()` |
| Provider may sleep, normal process/thread context | `gpiod_get_value_cansleep()`, `gpiod_set_value_cansleep()` |
| Unsure provider in IRQ path | use threaded IRQ or defer work |

Check with:

```c
if (gpiod_cansleep(desc))
    value = gpiod_get_value_cansleep(desc);
else
    value = gpiod_get_value(desc);
```

For GPIO-backed interrupts, `request_threaded_irq()` or `request_any_context_irq()` can avoid hard-IRQ assumptions.

## Lifecycle / Data Flow
Most GPIO consumer drivers follow a simple probe-to-remove flow.

```text
1. DTS describes pinctrl states and *-gpios properties.
2. Device core creates/binds the device and handles standard pinctrl states.
3. probe():
   - allocate private data
   - get required GPIO descriptors with devm_gpiod_get*()
   - set direction and initial values using GPIOD_* flags
   - optionally get pinctrl states for runtime PM
   - convert input GPIO to IRQ if needed
   - request IRQ, preferably managed
4. runtime:
   - use logical GPIO values
   - use _cansleep variants when needed
   - select pin states during suspend/resume when needed
5. remove/error:
   - managed GPIOs, IRQs, and pinctrl handles are cleaned automatically
   - unmanaged descriptors must be released explicitly
```

### Error Path Rules
GPIO and pinctrl providers can appear later than your consumer. Preserve deferred probe errors.

```c
reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
if (IS_ERR(reset))
    return dev_err_probe(dev, PTR_ERR(reset), "failed to get reset GPIO\n");
```

Use `dev_err_probe()` for resources that may return `-EPROBE_DEFER`; it keeps logs cleaner and still records useful context.

## Minimal Practical Example
This is a learning-oriented skeleton. It is close to production style, but real drivers need hardware-specific timing, locking, runtime PM, and framework registration.

### Device Tree
```dts
mydev_default_pins: mydev-default-pins {
    /* Controller-specific pin configuration goes here. */
};

mydev_sleep_pins: mydev-sleep-pins {
    /* Low-power pin configuration goes here. */
};

mydev {
    compatible = "vendor,mydev";
    pinctrl-names = "default", "sleep";
    pinctrl-0 = <&mydev_default_pins>;
    pinctrl-1 = <&mydev_sleep_pins>;

    reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
    enable-gpios = <&gpio1 8 GPIO_ACTIVE_HIGH>;
    irq-gpios = <&gpio2 3 GPIO_ACTIVE_LOW>;
};
```

### Driver Skeleton
```c
#include <linux/gpio/consumer.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>

struct mydev {
    struct gpio_desc *reset;
    struct gpio_desc *enable;
    struct gpio_desc *irq_gpio;
    struct pinctrl *pinctrl;
    struct pinctrl_state *pins_default;
    struct pinctrl_state *pins_sleep;
    int irq;
};

static irqreturn_t mydev_irq_thread(int irq, void *data)
{
    struct mydev *priv = data;
    int active;

    active = gpiod_get_value_cansleep(priv->irq_gpio);
    if (active < 0)
        return IRQ_NONE;

    /* Handle device event here. */
    return IRQ_HANDLED;
}

static int mydev_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct mydev *priv;
    int ret;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->reset = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH);
    if (IS_ERR(priv->reset))
        return dev_err_probe(dev, PTR_ERR(priv->reset),
                             "failed to get reset GPIO\n");

    priv->enable = devm_gpiod_get_optional(dev, "enable", GPIOD_OUT_LOW);
    if (IS_ERR(priv->enable))
        return dev_err_probe(dev, PTR_ERR(priv->enable),
                             "failed to get enable GPIO\n");

    priv->irq_gpio = devm_gpiod_get_optional(dev, "irq", GPIOD_IN);
    if (IS_ERR(priv->irq_gpio))
        return dev_err_probe(dev, PTR_ERR(priv->irq_gpio),
                             "failed to get IRQ GPIO\n");

    if (priv->enable)
        gpiod_set_value_cansleep(priv->enable, 1);

    if (priv->reset) {
        gpiod_set_value_cansleep(priv->reset, 1); /* assert */
        usleep_range(1000, 2000);
        gpiod_set_value_cansleep(priv->reset, 0); /* deassert */
        usleep_range(5000, 10000);
    }

    priv->pinctrl = devm_pinctrl_get(dev);
    if (!IS_ERR(priv->pinctrl)) {
        priv->pins_default = pinctrl_lookup_state(priv->pinctrl, "default");
        priv->pins_sleep = pinctrl_lookup_state(priv->pinctrl, "sleep");
    }

    if (priv->irq_gpio) {
        priv->irq = gpiod_to_irq(priv->irq_gpio);
        if (priv->irq < 0)
            return dev_err_probe(dev, priv->irq, "failed to map GPIO IRQ\n");

        ret = devm_request_threaded_irq(dev, priv->irq, NULL,
                                        mydev_irq_thread,
                                        IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                                        dev_name(dev), priv);
        if (ret)
            return dev_err_probe(dev, ret, "failed to request IRQ\n");
    }

    platform_set_drvdata(pdev, priv);
    return 0;
}
```

Important details:

- `devm_gpiod_get_optional()` returns `NULL` when the optional line is absent, and `ERR_PTR(...)` for real errors.
- `GPIOD_OUT_HIGH` and `GPIOD_OUT_LOW` are **logical** values, so active-low is handled automatically.
- `_cansleep` accessors are safe for GPIO expanders behind sleeping buses.
- `gpiod_to_irq()` can fail; not every GPIO line supports IRQs.

## Common Bugs And Debugging
Start from symptoms. Most pin/GPIO problems are wiring, DT property, polarity, or context bugs.

| Symptom | Likely cause | What to check |
| --- | --- | --- |
| `devm_gpiod_get()` returns `-ENOENT` | Wrong property name or missing DT property | `reset-gpios` vs `reset-gpio`, runtime DT |
| Probe keeps deferring | GPIO/pinctrl provider not ready | `dmesg`, provider driver, `dev_err_probe()` output |
| Reset appears inverted | Misunderstood `GPIO_ACTIVE_LOW` logical values | DT flags, `gpiod_is_active_low()` |
| GPIO value access warns about sleeping | Used non-`_cansleep` accessor on sleeping provider | `gpiod_cansleep()`, use threaded IRQ/workqueue |
| IRQ request fails | GPIO cannot map to IRQ or wrong trigger | `gpiod_to_irq()` return, controller binding |
| Device works before suspend but not after resume | Pins left in sleep state or wrong PM ordering | pinctrl state selection, PM callbacks |
| Another driver owns the line | Duplicate GPIO consumer or hog | `/sys/kernel/debug/gpio` |
| Pin never toggles on scope | Pad not muxed to GPIO, wrong pin group, disabled node | pinctrl debugfs, DTS state references |

### Useful Debug Views
These are common bring-up tools when available:

- `dmesg`: probe failures, deferred probe, pinctrl select errors.
- `/sys/kernel/debug/gpio`: GPIO chips, requested lines, labels, direction, value.
- `/sys/kernel/debug/pinctrl/`: pin mux owner, pin config, selected states.
- `/sys/firmware/devicetree/base`: runtime DT properties actually passed to the kernel.
- `gpioinfo` / `gpiodetect`: userspace GPIO chardev inspection tools.

### Fix Patterns
Use these checks before blaming the silicon:

- Confirm the runtime DT has the property name your driver requests.
- Confirm the GPIO controller node is enabled and registered.
- Confirm the pad is muxed to GPIO if the driver drives it as GPIO.
- Use logical values unless you have a physical-level reason to use raw accessors.
- Move GPIO work out of hard IRQ context if the provider can sleep.
- Preserve `-EPROBE_DEFER` instead of converting it to `-EINVAL` or `-ENODEV`.

## Production Checklist
Before code review or board bring-up, verify these points.

- **Use descriptor GPIO APIs** for new kernel drivers: `devm_gpiod_get*()`.
- **Use managed resources** unless the driver has a specific lifetime reason not to.
- **Set direction and initial output value at request time** using `GPIOD_*` flags.
- **Use logical values** for active-low lines; avoid raw accessors by default.
- **Use `_cansleep` accessors** when the provider may sleep or when code runs in thread/process context.
- **Do not use global GPIO numbers** in new board-specific driver code.
- **Do not export GPIO sysfs as a new ABI**; sysfs GPIO is obsolete.
- **Prefer existing kernel frameworks** such as `gpio-leds`, `gpio-keys`, `gpio-wdt`, `mmc-pwrseq-simple`, or `gpio-gate-clock` when they match the hardware.
- **Keep pinctrl data in Device Tree and bindings**, not in consumer-driver register writes.
- **Check all errors** from `devm_gpiod_get*()`, `pinctrl_lookup_state()`, `pinctrl_select_state()`, `gpiod_to_irq()`, and IRQ request calls.
- **Handle suspend/resume pin states** if pin leakage, wakeup, or safe output state matters.
- **Document optional GPIO behavior** in the binding and code comments.

## Interview Readiness
You are ready for interviews when you can explain the boundary between pinctrl and GPIO without leaning on API memorization.

Be able to answer:

- What is the difference between pinmux, pinconf, and GPIO?
- Why do modern drivers use `struct gpio_desc *` instead of integer GPIO numbers?
- How does `gpiod_get(dev, "reset", ...)` find `reset-gpios`?
- What does `GPIO_ACTIVE_LOW` change in driver code?
- When do you use `_cansleep` GPIO accessors?
- Why can `gpiod_to_irq()` fail?
- How would you debug a reset GPIO that never toggles?
- Why should a driver often use `gpio-keys`, `gpio-leds`, or another subsystem instead of custom GPIO code?

See `interview/13-pin-control-and-gpio-consumer-apis.md` for structured practice.

## Kernel Version Notes
GPIO and pinctrl APIs are stable concepts, but the recommended interfaces have shifted over time.

- **Descriptor GPIO APIs are preferred** for new kernel consumers. Legacy integer GPIO APIs still appear in old drivers and training material.
- **GPIO sysfs is obsolete** for new userspace consumers. Use GPIO character-device tooling for userspace inspection/control when appropriate.
- GPIO chardev v1 is also obsolete; current userspace ABI guidance points to chardev v2.
- Standard pinctrl state handling includes `default`, `init`, `sleep`, and `idle`; PM state selection should follow current pinctrl PM helper behavior.
