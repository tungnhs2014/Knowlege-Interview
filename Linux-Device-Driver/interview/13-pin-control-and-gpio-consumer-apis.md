# 13 - Pin Control And GPIO Consumer APIs Interview Questions

Strong candidates can separate pinctrl from GPIO, reason about Device Tree lookup, use descriptor GPIO APIs safely, handle active-low signals, and debug board bring-up without guessing.

## Beginner Questions

### 1. What is the difference between pinctrl and GPIO?
**Short Answer:** Pinctrl configures what a physical pin does and how it behaves electrically. GPIO APIs let a driver own and use a digital input/output line.

**Deep Explanation:** A physical SoC pin can often be UART, SPI, I2C, PWM, GPIO, or another function. Pinctrl selects that mux function and electrical settings such as pull-up, pull-down, drive strength, open-drain, slew rate, and sleep state. GPIO is only one possible pin function. Once the pad is configured as a GPIO line, the GPIO consumer API lets the driver request it, set direction, read/write logical values, and sometimes map it to an IRQ.

**API / Code Anchor:**
```dts
pinctrl-names = "default";
pinctrl-0 = <&button_pins>;
reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
```

```c
reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
```

**Production or Debugging Angle:** If a GPIO descriptor is valid but the pin does not move on a scope, check pinctrl. The line may be requested correctly but the pad may not be muxed to GPIO.

**Common Traps:** Saying "GPIO configuration" when you mean pin muxing. GPIO direction/value and pin mux/config are different layers.

**Follow-up Questions:**
- Can a UART pin need pinctrl without GPIO?
- Can a reset GPIO need both pinctrl and GPIO APIs?
- Where should pad register values live: driver C code or Device Tree?

### 2. What is a GPIO descriptor, and why is it preferred over integer GPIO numbers?
**Short Answer:** A GPIO descriptor is an opaque `struct gpio_desc *` representing a requested GPIO line. It is preferred because it is device-scoped, firmware-aware, and handles flags such as active-low.

**Deep Explanation:** Legacy GPIO APIs used global integer numbers. Those numbers are fragile because they depend on controller registration order and board layout. Descriptor APIs let a driver request a line by function name, such as `"reset"`, from the device's firmware node. The GPIO core resolves the provider, requests ownership, stores flags, and returns a descriptor.

**API / Code Anchor:**
```dts
reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
```

```c
struct gpio_desc *reset;

reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
if (IS_ERR(reset))
    return PTR_ERR(reset);
```

**Production or Debugging Angle:** Descriptor APIs make drivers more portable across boards because the driver asks for "reset", not "GPIO number 39".

**Common Traps:** Hard-coding global GPIO numbers in a new driver, or converting descriptors back to integers just to use legacy APIs.

**Follow-up Questions:**
- What DT property does `gpiod_get(dev, "reset", ...)` look for?
- When might you still see integer GPIO APIs?
- What does `devm_` add here?

### 3. What does `GPIO_ACTIVE_LOW` mean for driver code?
**Short Answer:** It means logical active is physical low. Descriptor APIs use logical values, so `gpiod_set_value(desc, 1)` asserts an active-low signal by driving the pin low.

**Deep Explanation:** A device signal has a logical meaning, such as reset asserted or LED on. The board wiring decides whether that logical active state is high or low voltage. With descriptor APIs, the driver normally uses logical values and lets gpiolib apply the active-low mapping.

**API / Code Anchor:**
```dts
reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
```

```c
gpiod_set_value_cansleep(reset, 1); /* assert reset */
gpiod_set_value_cansleep(reset, 0); /* deassert reset */
```

**Production or Debugging Angle:** If reset looks inverted on the oscilloscope, check whether you are thinking in physical voltage while the API is using logical values.

**Common Traps:** Using raw accessors to "fix" active-low behavior. Most drivers should use logical accessors.

**Follow-up Questions:**
- What does `gpiod_is_active_low()` tell you?
- When would raw GPIO access be valid?
- How would you represent an active-low button in DT?

### 4. What are `pinctrl-names` and `pinctrl-0`?
**Short Answer:** `pinctrl-names` names a device's pin states, and `pinctrl-N` references the pin configuration nodes for state index `N`.

**Deep Explanation:** A device may need different pin states for normal operation, initialization, sleep, or idle. The DT node maps state names to lists of pin configuration phandles. For example, entry 0 in `pinctrl-names` corresponds to `pinctrl-0`.

**API / Code Anchor:**
```dts
pinctrl-names = "default", "sleep";
pinctrl-0 = <&dev_default_pins>;
pinctrl-1 = <&dev_sleep_pins>;
```

```c
pins_sleep = pinctrl_lookup_state(pinctrl, "sleep");
pinctrl_select_state(pinctrl, pins_sleep);
```

**Production or Debugging Angle:** Bad pinctrl references often show up as probe errors, dead GPIOs, broken suspend/resume, or pins owned by the wrong function in debugfs.

**Common Traps:** Writing `pinctrl-name` instead of `pinctrl-names`, or assuming every driver must manually select `default`.

**Follow-up Questions:**
- Which state is normally used during runtime?
- Why might a driver explicitly select `sleep`?
- Can one state reference multiple pin groups?

## Mid-Level Questions

### 5. How does `gpiod_get(dev, "reset", ...)` find a GPIO in Device Tree?
**Short Answer:** The GPIO core uses the device's firmware node and the connection ID `"reset"` to look for a matching GPIO property such as `reset-gpios`.

**Deep Explanation:** Descriptor lookup is function-name based. A driver passes a consumer ID, and gpiolib combines it with GPIO property suffixes. In DT, modern bindings normally use `<name>-gpios`. The provider phandle and cells identify the GPIO controller, line offset, and flags such as active-low.

**API / Code Anchor:**
```dts
reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
```

```c
reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
```

**Production or Debugging Angle:** A missing `s` can be the whole bug. Check the runtime DT, not only the source file you think was built.

**Common Traps:** Passing `"reset-gpios"` as the `con_id`. The `con_id` is `"reset"`, not the full property name.

**Follow-up Questions:**
- What does `devm_gpiod_get_index(dev, "led", 1, ...)` mean?
- What should an optional GPIO return when absent?
- Why should new bindings prefer `*-gpios`?

### 6. When should you use `gpiod_get_value_cansleep()` instead of `gpiod_get_value()`?
**Short Answer:** Use `_cansleep` accessors when the GPIO provider may sleep, such as an I2C or SPI GPIO expander, and in contexts where sleeping is allowed.

**Deep Explanation:** Internal SoC GPIO controllers are usually memory-mapped and can be accessed without sleeping. GPIO expanders on I2C/SPI need bus transactions, which may sleep. Calling non-`_cansleep` accessors on such lines from atomic context can produce warnings or bugs.

**API / Code Anchor:**
```c
if (gpiod_cansleep(btn))
    value = gpiod_get_value_cansleep(btn);
else
    value = gpiod_get_value(btn);
```

**Production or Debugging Angle:** In IRQ paths, prefer a threaded IRQ handler or defer work if the GPIO may sleep.

**Common Traps:** Testing only on an SoC GPIO and then failing when the same design uses an I2C GPIO expander on another board.

**Follow-up Questions:**
- Can you sleep in a hard IRQ handler?
- How does `request_threaded_irq()` help?
- What does `request_any_context_irq()` try to solve?

### 7. How do you convert a GPIO input to an interrupt?
**Short Answer:** Request the GPIO as input, call `gpiod_to_irq()`, check the return value, then request the IRQ with the right trigger and handler model.

**Deep Explanation:** Some GPIO controllers can also act as interrupt providers. `gpiod_to_irq()` asks the GPIO/IRQ infrastructure for a Linux IRQ number corresponding to that line. Not every GPIO supports IRQ mapping, and the trigger type must match the hardware and DT/binding expectations.

**API / Code Anchor:**
```c
button = devm_gpiod_get(dev, "button", GPIOD_IN);
if (IS_ERR(button))
    return PTR_ERR(button);

irq = gpiod_to_irq(button);
if (irq < 0)
    return irq;

ret = devm_request_threaded_irq(dev, irq, NULL, button_irq_thread,
                                IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                                dev_name(dev), priv);
```

**Production or Debugging Angle:** A threaded handler is safer if the handler reads a GPIO that may sleep.

**Common Traps:** Assuming every GPIO can be an IRQ, ignoring a negative `gpiod_to_irq()` return, or doing sleeping I/O in the top half.

**Follow-up Questions:**
- When would `platform_get_irq()` be better than `gpiod_to_irq()`?
- What does `IRQF_ONESHOT` do for threaded IRQs?
- How would active-low affect trigger choice?

### 8. How should a driver handle optional GPIOs?
**Short Answer:** Use `devm_gpiod_get_optional()`, treat `NULL` as absent, and treat `ERR_PTR()` as a real failure.

**Deep Explanation:** Optional GPIOs are common for board variants, such as an optional enable line or reset line. The binding should describe whether the line is optional and what default behavior is assumed when absent. The driver should not convert all failures into "optional absent"; that hides deferred probe and real DT mistakes.

**API / Code Anchor:**
```c
enable = devm_gpiod_get_optional(dev, "enable", GPIOD_OUT_LOW);
if (IS_ERR(enable))
    return dev_err_probe(dev, PTR_ERR(enable),
                         "failed to get enable GPIO\n");

if (enable)
    gpiod_set_value_cansleep(enable, 1);
```

**Production or Debugging Angle:** Preserve `-EPROBE_DEFER`. If the GPIO provider is not ready, the driver should probe again later.

**Common Traps:** Treating `-ENOENT`, `-EPROBE_DEFER`, and `-EINVAL` as the same thing.

**Follow-up Questions:**
- Where should optional behavior be documented?
- Why use `dev_err_probe()` here?
- What initial flag would you choose for an optional reset GPIO?

### 9. What is the safe sequence for a reset GPIO?
**Short Answer:** Request it with the correct initial logical state, wait as required by the datasheet, deassert it with logical values, then wait for the device to become ready.

**Deep Explanation:** Reset polarity is board wiring, so the driver should use logical values through the descriptor API. If the reset line is active-low, `GPIOD_OUT_HIGH` requests the line as asserted. Timing must come from the device datasheet. If an enable regulator/GPIO must be active before reset deassertion, respect that ordering.

**API / Code Anchor:**
```c
reset = devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH);
if (IS_ERR(reset))
    return PTR_ERR(reset);

if (reset) {
    usleep_range(1000, 2000);
    gpiod_set_value_cansleep(reset, 0);
    usleep_range(5000, 10000);
}
```

**Production or Debugging Angle:** Use a scope or logic analyzer to confirm physical timing, but reason in code using logical asserted/deasserted states.

**Common Traps:** Using `mdelay()` without need, using raw values accidentally, or deasserting reset before clocks/regulators are ready.

**Follow-up Questions:**
- Why set the initial value in `devm_gpiod_get()`?
- What if the reset GPIO is absent?
- How do regulators and clocks affect reset ordering?

### 10. How do you debug "my GPIO never changes"?
**Short Answer:** Check the runtime DT property, the GPIO consumer label, pinctrl mux state, active-low polarity, provider registration, and whether another driver owns the line.

**Deep Explanation:** The failure could be at several layers. The driver may request the wrong property name. The GPIO provider may not be registered. The line may be owned by a GPIO hog or another driver. The pin may not be muxed to GPIO. Or the value may change logically but look inverted physically because the line is active-low.

**API / Code Anchor:**
```bash
cat /sys/kernel/debug/gpio
ls /sys/kernel/debug/pinctrl/
hexdump -C /sys/firmware/devicetree/base/.../reset-gpios
```

**Production or Debugging Angle:** Always check the DT actually loaded by the bootloader. Editing a DTS source file does nothing if the board boots an old DTB.

**Common Traps:** Looking only at driver code and ignoring mux state or bootloader DT packaging.

**Follow-up Questions:**
- What does `/sys/kernel/debug/gpio` show?
- What would pinctrl debugfs tell you?
- How would you detect a wrong active-low flag?

## Senior Questions

### 11. When should you rely on automatic `default` pinctrl selection, and when should your driver explicitly manage pin states?
**Short Answer:** Rely on automatic `default` selection for simple devices. Explicitly manage states when the driver needs runtime, suspend/resume, `init`, `sleep`, or `idle` transitions.

**Deep Explanation:** The device core binds standard pinctrl states to devices and can select the normal state around probe. A simple UART or sensor driver may not need any pinctrl code. But if the driver must reduce leakage in suspend, switch pins to safe outputs, enter an idle mux, or recover from hardware modes, it should get a pinctrl handle, look up states, and select them at the right lifecycle point.

**API / Code Anchor:**
```c
priv->pinctrl = devm_pinctrl_get(dev);
priv->pins_default = pinctrl_lookup_state(priv->pinctrl, "default");
priv->pins_sleep = pinctrl_lookup_state(priv->pinctrl, "sleep");

pinctrl_select_state(priv->pinctrl, priv->pins_sleep);
```

**Production or Debugging Angle:** Sleep pin states can affect power, wakeup reliability, and bus safety. Selecting sleep too early can break wake IRQs or leave hardware in an unsafe state.

**Common Traps:** Manually selecting `default` in every driver as ritual, or forgetting to restore `default` on resume.

**Follow-up Questions:**
- How do `init` and `default` differ?
- Which callbacks should switch to `sleep`?
- What could go wrong with wakeup GPIOs in sleep state?

### 12. Why should a senior engineer prefer existing GPIO-backed framework drivers when possible?
**Short Answer:** Framework drivers encode subsystem behavior, userspace ABI, power management, and binding conventions better than ad hoc GPIO bit-banging.

**Deep Explanation:** Many GPIO-controlled functions already have kernel models: `gpio-leds`, `gpio-keys`, `gpio-wdt`, `mmc-pwrseq-simple`, `gpio-gate-clock`, regulators, resets, and more. If the hardware is really a watchdog, key, LED, power sequence, or clock gate, the correct abstraction is the subsystem, not a custom character driver or userspace script toggling a GPIO.

**API / Code Anchor:**
```dts
watchdog {
    compatible = "linux,wdt-gpio";
    gpios = <&gpio3 9 GPIO_ACTIVE_LOW>;
    hw_algo = "toggle";
    hw_margin_ms = <1600>;
};
```

**Production or Debugging Angle:** Framework use improves integration with power management, standard userspace tools, bindings, and maintenance.

**Common Traps:** Creating a private sysfs/debugfs knob for something that already has a stable subsystem ABI.

**Follow-up Questions:**
- When is direct GPIO control justified?
- Why is userspace GPIO not a good watchdog implementation?
- What framework would you use for buttons?

### 13. How do active-low and open-drain semantics affect maintainability?
**Short Answer:** They belong in hardware description and descriptor flags, so driver code can operate on logical device state instead of board-specific voltage levels.

**Deep Explanation:** Board A may wire reset active-low and board B may use an inverter or different polarity. If the driver uses logical values, the same code can assert reset or turn a signal on without knowing physical voltage. Open-drain/source behavior similarly describes line driving constraints and should be expressed through bindings and GPIO flags when supported.

**API / Code Anchor:**
```c
reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
gpiod_set_value_cansleep(reset, 0); /* deassert reset logically */
```

**Production or Debugging Angle:** Logical values make reviews easier: `1` means asserted/active, not necessarily high voltage.

**Common Traps:** Mixing raw and logical access in one driver, or encoding board polarity in C conditionals.

**Follow-up Questions:**
- What is the danger of `gpiod_set_raw_value()`?
- How would you support two boards with opposite reset polarity?
- Where should open-drain be documented?

### 14. How do you design error handling and lifetime for GPIO resources in probe?
**Short Answer:** Use managed GPIO/IRQ/pinctrl helpers where possible, return exact errors, preserve deferred probe, and avoid using descriptors after device teardown.

**Deep Explanation:** GPIO descriptors are owned resources tied to the consumer. Managed helpers release them automatically when the device is removed or probe fails. This reduces cleanup labels. However, the driver must still check errors and order hardware shutdown carefully when outputs affect external devices.

**API / Code Anchor:**
```c
priv->enable = devm_gpiod_get_optional(dev, "enable", GPIOD_OUT_LOW);
if (IS_ERR(priv->enable))
    return dev_err_probe(dev, PTR_ERR(priv->enable),
                         "failed to get enable GPIO\n");
```

**Production or Debugging Angle:** Managed cleanup does not replace hardware sequencing. You may still need remove/shutdown callbacks to put outputs into safe states before resources vanish.

**Common Traps:** Thinking `devm_` means no remove logic is ever needed, or flattening all errors to `-EINVAL`.

**Follow-up Questions:**
- When would unmanaged `gpiod_get()` be appropriate?
- How does `dev_err_probe()` help?
- What should happen to enable/reset GPIOs in shutdown?

### 15. A button IRQ handler reads a GPIO and sometimes triggers "sleeping function called from invalid context". What is wrong?
**Short Answer:** The handler is probably using a GPIO accessor that may sleep from hard IRQ context, likely because the GPIO provider is an I2C/SPI expander.

**Deep Explanation:** GPIO-backed IRQs can come from MMIO SoC GPIO controllers or from sleeping expanders. If the handler runs in hard IRQ context and calls a sleeping bus transaction, the kernel warns. The fix is to use a threaded IRQ handler, `request_any_context_irq()`, or defer GPIO reads to process context, and use `_cansleep` accessors.

**API / Code Anchor:**
```c
ret = devm_request_threaded_irq(dev, irq, NULL, button_thread,
                                IRQF_ONESHOT | IRQF_TRIGGER_FALLING,
                                dev_name(dev), priv);

static irqreturn_t button_thread(int irq, void *data)
{
    state = gpiod_get_value_cansleep(priv->button);
    return IRQ_HANDLED;
}
```

**Production or Debugging Angle:** This bug may not reproduce on the first board if it uses SoC GPIOs. It appears when the same signal moves to a GPIO expander.

**Common Traps:** Assuming "GPIO read is just a register read" on all boards.

**Follow-up Questions:**
- What does `gpiod_cansleep()` report?
- Why is `IRQF_ONESHOT` commonly paired with threaded IRQs?
- Would a workqueue also solve this?

### 16. How would you review a driver that parses `reset-gpios` manually with `of_get_named_gpio()`?
**Short Answer:** For new code, I would ask it to use descriptor APIs such as `devm_gpiod_get()` unless there is a strong compatibility reason.

**Deep Explanation:** Manual integer GPIO parsing bypasses the modern consumer model and often leads to duplicated error handling, explicit request/free logic, weaker polarity handling, and less portable code. Descriptor APIs know how to parse firmware-backed GPIO mappings and keep the driver focused on signal meaning.

**API / Code Anchor:**
```c
/* Prefer this */
reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);

/* Legacy style often seen in old code */
gpio = of_get_named_gpio(np, "reset-gpios", 0);
```

**Production or Debugging Angle:** During review, distinguish old maintained drivers from new code. You may not rewrite stable legacy code without reason, but new drivers should follow current style.

**Common Traps:** Claiming integer APIs never work. The real point is that descriptors are the recommended model for new consumers.

**Follow-up Questions:**
- What bugs does descriptor GPIO avoid?
- How does active-low handling differ?
- What migration risks would you check?

### 17. How would you debug a suspend/resume failure caused by pin states?
**Short Answer:** Inspect whether the driver or PM core selects `sleep` and restores `default`, then verify pinctrl debugfs, wakeup configuration, and GPIO/pad electrical state.

**Deep Explanation:** Suspend can change mux, bias, output value, and wake behavior. A device may fail after resume because pins remain in `sleep`, because a wake GPIO lost bias, because reset/enable lines changed state, or because the driver selected states in the wrong order relative to clocks/regulators/IRQ wake.

**API / Code Anchor:**
```c
static int my_suspend(struct device *dev)
{
    return pinctrl_select_state(priv->pinctrl, priv->pins_sleep);
}

static int my_resume(struct device *dev)
{
    return pinctrl_select_state(priv->pinctrl, priv->pins_default);
}
```

**Production or Debugging Angle:** Use board measurements when current or wake behavior matters. Pin state bugs can be invisible in driver logs.

**Common Traps:** Treating pinctrl as probe-only, or forgetting that wakeup GPIOs may need a different sleep configuration than ordinary inactive pins.

**Follow-up Questions:**
- How would runtime PM change the design?
- What should happen to interrupt pins in sleep?
- How can pin bias affect wake reliability?

## Debugging Scenarios And Traps

### Scenario A: `devm_gpiod_get(dev, "reset", ...)` returns `-ENOENT`
**Short Answer:** The driver did not find a matching GPIO property for `"reset"`.

**Deep Explanation:** The DT may be missing `reset-gpios`, the property may be named incorrectly, or the board may be booting a different DTB. The driver should not silently continue if reset is mandatory.

**API / Code Anchor:**
```dts
reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
```

```c
reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
```

**Production or Debugging Angle:** Check `/sys/firmware/devicetree/base` to confirm the live tree.

**Common Traps:** Passing `"reset-gpios"` as `con_id`, or making a required reset line optional without binding justification.

**Follow-up Questions:**
- What if the error is `-EPROBE_DEFER`?
- What would `dev_err_probe()` print?
- How do you distinguish absent optional GPIO from real failure?

### Scenario B: The LED turns off when the driver writes `1`
**Short Answer:** The LED is probably active-low, so logical 1 drives the physical line low.

**Deep Explanation:** With descriptor APIs, logical 1 means active. For an active-low LED, active maps to low voltage. The code may be correct even if the physical voltage looks inverted.

**API / Code Anchor:**
```dts
led-gpios = <&gpio2 15 GPIO_ACTIVE_LOW>;
```

```c
gpiod_set_value(led, 1); /* LED on logically */
```

**Production or Debugging Angle:** Confirm the DT flag and board schematic before changing code.

**Common Traps:** Replacing logical access with raw access and breaking other boards.

**Follow-up Questions:**
- How does `gpioinfo` display active-low lines?
- What should code comments say for active-low reset?
- How would you test both polarities?

### Scenario C: GPIO sysfs works in a lab script; should the driver expose it as product ABI?
**Short Answer:** No. GPIO sysfs is obsolete, and drivers should expose real device behavior through the proper subsystem or ABI.

**Deep Explanation:** Userspace GPIO can help early board testing, but it is a poor product interface for hardware that has a kernel driver model. It lacks subsystem semantics and can conflict with kernel ownership. Current userspace GPIO should use chardev tooling when direct userspace line control is appropriate.

**API / Code Anchor:**
```bash
gpioinfo
gpiodetect
```

**Production or Debugging Angle:** For a key, use input; for an LED, use LED class; for a watchdog, use watchdog; for a reset line, usually keep it internal to the driver.

**Common Traps:** Treating debug/testing mechanisms as stable product ABI.

**Follow-up Questions:**
- What is the modern userspace GPIO ABI?
- Why is `gpio-wdt` better than a userspace toggle loop?
- Where should debug-only GPIO information live?
