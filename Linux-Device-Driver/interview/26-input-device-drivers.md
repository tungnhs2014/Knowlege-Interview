# 26 - Input Device Drivers Interview Questions

Strong candidates can reason from a physical input action to a userspace event. They can explain why the input subsystem exists, how `struct input_dev` is registered, how event types/codes/values work, and where real drivers fail around GPIO polarity, debounce, IRQ context, lifetime, and userspace discovery.

## Beginner

### 1. What problem does the Linux input subsystem solve?

**Level:** Beginner

**Short Answer:**  
It gives keyboards, buttons, mice, touchscreens, and similar devices a standard way to report events to userspace through interfaces such as `/dev/input/eventX`.

**Deep Explanation:**  
Without the input subsystem, every button or keyboard driver would invent a custom character device, sysfs file, or ioctl format. The input core standardizes user-input events: drivers report event type, code, and value; the core routes them to handlers; evdev exposes them as `struct input_event` records. Existing tools and applications can then consume the device without knowing the hardware-specific driver.

**API / Code Anchor:**  
`struct input_dev`, `input_register_device()`, `input_report_key()`, `input_sync()`, `struct input_event`, `/dev/input/eventX`.

**Production or Debugging Angle:**  
Using the input subsystem gives stable userspace behavior and standard tools like `evtest`, `libinput`, and udev integration. A private `/dev/my_button` node creates needless ABI work.

**Common Traps:**  
- Treating a user button as a raw GPIO userspace problem.
- Creating a custom char device for keyboard-like events.
- Thinking `/dev/input/eventX` is created by the driver manually.

**Follow-up Questions:**  
- What is evdev?
- Which devices usually belong in input?
- How is input different from IIO?

### 2. What is `struct input_dev`?

**Level:** Beginner

**Short Answer:**  
`struct input_dev` is the kernel object representing one logical input device.

**Deep Explanation:**  
The driver fills `struct input_dev` with identity, parent device, bus type, supported event capabilities, optional absolute-axis ranges, callbacks, and private data. After `input_register_device()`, the input core can attach handlers such as evdev and expose the device to userspace.

**API / Code Anchor:**  
`devm_input_allocate_device()`, `input->name`, `input->id.bustype`, `input->dev.parent`, `input_register_device()`.

**Production or Debugging Angle:**  
If the input device is registered with missing capability bits or a poor name, userspace may see a confusing or unusable event node.

**Common Traps:**  
- Registering before capabilities are initialized.
- Forgetting `input->dev.parent`.
- Treating `struct input_dev` as the private hardware state instead of storing private state separately.

**Follow-up Questions:**  
- How do you attach private data to an input device?
- What should be initialized before registration?
- When would a driver have more than one input device?

### 3. What are event type, code, and value?

**Level:** Beginner

**Short Answer:**  
The type says the class of event, the code says which key/axis/switch, and the value says the state or amount.

**Deep Explanation:**  
For a button press, the type is usually `EV_KEY`, the code might be `KEY_ENTER`, and the value is `1` for pressed or `0` for released. For a mouse move, the type can be `EV_REL`, the code `REL_X`, and the value the delta. For a touchscreen coordinate, the type can be `EV_ABS`, the code `ABS_X`, and the value the absolute coordinate.

**API / Code Anchor:**  
`EV_KEY`, `EV_REL`, `EV_ABS`, `KEY_*`, `BTN_*`, `REL_*`, `ABS_*`, `struct input_event`.

**Production or Debugging Angle:**  
Wrong type/code choices confuse userspace classification. For example, a touchscreen without proper absolute axes and properties may not behave like a touchscreen.

**Common Traps:**  
- Confusing `KEY_*` and `BTN_*`.
- Reporting absolute positions as relative deltas.
- Thinking key value `1` means "toggle" instead of "currently pressed".

**Follow-up Questions:**  
- What does `EV_KEY` value `2` mean?
- What event type would a mouse wheel use?
- Why do absolute axes need range metadata?

### 4. Why is `input_sync()` required?

**Level:** Beginner

**Short Answer:**  
`input_sync()` ends the current event frame and tells userspace the related reports belong together.

**Deep Explanation:**  
Drivers often report multiple values for one physical sample, such as X and Y coordinates or press state plus position. `input_sync()` emits a synchronization event, commonly `EV_SYN / SYN_REPORT`, so evdev consumers know the frame is complete.

**API / Code Anchor:**  
`input_report_key()`, `input_report_abs()`, `input_sync()`, `EV_SYN`, `SYN_REPORT`.

**Production or Debugging Angle:**  
If a driver reports events but never syncs, userspace may not see complete frames or may behave unpredictably.

**Common Traps:**  
- Calling `input_sync()` before all values in a frame are reported.
- Forgetting `input_sync()` in rare error or release paths.
- Calling `input_sync()` repeatedly after every axis when one frame should group them.

**Follow-up Questions:**  
- What should follow reporting X/Y touchscreen coordinates?
- What does `SYN_REPORT` mean?
- Can input core filter unchanged events?

## Mid-level

### 5. Walk through the probe flow for a GPIO button input driver.

**Level:** Mid-level

**Short Answer:**  
Probe allocates state, gets the GPIO, allocates `input_dev`, sets capabilities, registers the input device, maps GPIO to IRQ, and requests a threaded IRQ or configures polling.

**Deep Explanation:**  
The driver first obtains hardware resources and prepares private state. It allocates an input device, sets name, parent, bus type, and key capability, then registers it. The event source must not run until the state it uses is valid. For an IRQ-driven GPIO button, the driver maps the descriptor to an IRQ and requests a handler that reads the GPIO, reports `EV_KEY`, and calls `input_sync()`.

**API / Code Anchor:**  
`devm_gpiod_get()`, `devm_input_allocate_device()`, `input_set_capability()`, `input_register_device()`, `gpiod_to_irq()`, `devm_request_threaded_irq()`.

**Production or Debugging Angle:**  
Ordering bugs are common. If the IRQ can fire before `btn->input` and private state are initialized, the handler may crash. If capability bits are set after registration, userspace may not advertise the key correctly.

**Common Traps:**  
- Calling `gpiod_to_irq(priv->button)` before assigning `priv->button`.
- Requesting IRQ before state is initialized.
- Hardcoding `/dev/input/event0` for tests.

**Follow-up Questions:**  
- Where would you set debounce?
- When would you use `open()` instead of requesting IRQ in probe?
- How do you handle active-low buttons?

### 6. How do you declare what events an input device can emit?

**Level:** Mid-level

**Short Answer:**  
Set capability bits before registration, either with `input_set_capability()` or by setting `evbit`, `keybit`, `relbit`, and `absbit`.

**Deep Explanation:**  
Capabilities are the contract with input core and userspace. For a key, the driver declares `EV_KEY` and the specific key code. For relative movement, it declares `EV_REL` and axes such as `REL_X`. For absolute axes, it declares `EV_ABS` and should also configure axis ranges with `input_set_abs_params()`.

**API / Code Anchor:**  
`input_set_capability(input, EV_KEY, KEY_ENTER)`, `set_bit(EV_KEY, input->evbit)`, `set_bit(KEY_ENTER, input->keybit)`, `input_set_abs_params()`.

**Production or Debugging Angle:**  
`evtest` and `/proc/bus/input/devices` show advertised capabilities. If events are missing there, inspect setup before blaming the IRQ path.

**Common Traps:**  
- Reporting a key code that was never advertised.
- Forgetting min/max for `EV_ABS`.
- Setting only `keybit` but not `evbit` when using manual bit operations.

**Follow-up Questions:**  
- What capabilities does a two-button mouse need?
- How would you describe a 3-axis joystick?
- What does `input_set_capability()` do for you?

### 7. When should you use `gpio-keys` instead of writing a custom input driver?

**Level:** Mid-level

**Short Answer:**  
Use `gpio-keys` for simple GPIO-backed keys or switches that can be described by Device Tree without custom hardware logic.

**Deep Explanation:**  
`gpio-keys` already handles common GPIO button behavior: GPIO or interrupt-based input, key code mapping, debounce interval, wakeup source, labels, autorepeat, and evdev exposure. If the hardware is just "this GPIO means KEY_POWER" or "this GPIO means KEY_ENTER", a custom driver adds risk without adding value.

**API / Code Anchor:**  
Device Tree: `compatible = "gpio-keys"`, child `gpios`, `linux,code`, `debounce-interval`, `wakeup-source`, `autorepeat`.

**Production or Debugging Angle:**  
Generic bindings are easier to maintain and review. Custom drivers are justified when there is a chip protocol, matrix scanning, register state machine, special filtering, firmware interaction, or multiple complex functions.

**Common Traps:**  
- Writing custom code for a simple board button.
- Forgetting `debounce-interval`.
- Using raw GPIO userspace access as the product ABI.

**Follow-up Questions:**  
- What is `gpio-keys-polled`?
- How do you represent a wakeup power key?
- When is a custom keypad driver justified?

### 8. Why might a GPIO button need a threaded IRQ handler?

**Level:** Mid-level

**Short Answer:**  
Because the GPIO provider may sleep, especially when the button is behind an I2C/SPI GPIO expander.

**Deep Explanation:**  
Hard IRQ context cannot sleep. A memory-mapped SoC GPIO read may be safe in hard IRQ, but an expander read may require I2C or SPI transfers. A threaded IRQ lets the handler run in process context, where sleeping bus access and `_cansleep` GPIO reads are valid. `IRQF_ONESHOT` keeps the IRQ masked until the thread completes.

**API / Code Anchor:**  
`gpiod_get_value_cansleep()`, `devm_request_threaded_irq(dev, irq, NULL, thread_fn, IRQF_ONESHOT | IRQF_TRIGGER_..., name, data)`, `request_any_context_irq()`.

**Production or Debugging Angle:**  
If a driver works with SoC GPIO but fails with a GPIO expander, check whether it used non-sleeping GPIO access in hard IRQ context.

**Common Traps:**  
- Calling I2C/SPI/regmap reads in a hard IRQ handler.
- Omitting `IRQF_ONESHOT` for threaded handlers on level-triggered or expander-backed IRQs.
- Assuming all GPIOs are memory-mapped SoC GPIOs.

**Follow-up Questions:**  
- What does `can_sleep` mean for a GPIO provider?
- What happens if a threaded IRQ never clears hardware status?
- Why might nested GPIO IRQs be involved?

### 9. Compare IRQ-driven input and polled input.

**Level:** Mid-level

**Short Answer:**  
IRQ-driven input reacts to hardware state changes. Polled input checks hardware periodically when no reliable interrupt exists.

**Deep Explanation:**  
IRQ-driven input is usually lower latency and lower power because the device wakes the CPU only on changes. Polled input is simpler for hardware without IRQs but consumes periodic CPU/power and can miss very short transitions if the interval is too slow. Current kernels support polling through helpers attached to `struct input_dev`.

**API / Code Anchor:**  
IRQ: `gpiod_to_irq()`, `devm_request_threaded_irq()`.  
Polling: `input_setup_polling()`, `input_set_poll_interval()`.

**Production or Debugging Angle:**  
Polling interval is a product decision: too slow feels laggy; too fast wastes power and may amplify bounce.

**Common Traps:**  
- Using legacy `input_polled_dev` examples blindly on current kernels.
- Polling even though hardware has a reliable IRQ.
- Forgetting to stop polling during remove or low-power states.

**Follow-up Questions:**  
- What replaced many old `input_polled_dev` examples?
- How would debounce differ in polling versus IRQ mode?
- When is polling acceptable?

### 10. How does userspace read input events?

**Level:** Mid-level

**Short Answer:**  
Userspace opens `/dev/input/eventX` and reads `struct input_event` records, often using `poll()`, `select()`, `libevdev`, `libinput`, or tools such as `evtest`.

**Deep Explanation:**  
The event record contains timestamp, type, code, and value. Applications can block until events arrive, use nonblocking I/O, or poll file descriptors. In real systems, udev often creates stable symlinks under `/dev/input/by-path/` or `/dev/input/by-id/`, so applications should not hardcode event numbers.

**API / Code Anchor:**  
`struct input_event`, `/dev/input/eventX`, `read()`, `poll()`, `select()`, `evtest`.

**Production or Debugging Angle:**  
If an application opens the wrong event node, the driver may be fine. Always identify by name, path, handlers, or udev properties.

**Common Traps:**  
- Hardcoding `/dev/input/event0`.
- Ignoring permissions or seat/session policy.
- Parsing event values without checking type and code.

**Follow-up Questions:**  
- What command shows input device capabilities?
- How can udev help locate the right device?
- Why might root see events but an application cannot?

## Senior

### 11. How do you design remove/unbind so an IRQ cannot use freed input state?

**Level:** Senior

**Short Answer:**  
Stop new hardware events, synchronize or free the IRQ/poll/work path, then unregister/free input resources according to ownership rules.

**Deep Explanation:**  
Input drivers often crash when remove frees `struct input_dev` or private state while an IRQ thread, delayed work, or poll callback can still report events. The safe shape is: disable device interrupt source, stop polling/work, synchronize IRQ path, unregister input users, and release resources. With devm, cleanup order still matters because managed cleanup alone does not make racing event paths logically safe.

**API / Code Anchor:**  
`disable_irq()`, `synchronize_irq()`, `devm_request_threaded_irq()`, `cancel_delayed_work_sync()`, `input_unregister_device()`, `devm_input_allocate_device()`.

**Production or Debugging Angle:**  
Remove races can appear only under stress: pressing the button while unloading, suspend/resume loops, or hot-unplug. Test repeated bind/unbind with active input events.

**Common Traps:**  
- Unregistering input before stopping IRQs.
- Assuming devm automatically orders all runtime event paths safely.
- Double-freeing an unmanaged input device after successful registration.

**Follow-up Questions:**  
- How does unmanaged `input_allocate_device()` ownership change after registration?
- What if `open()` requested the IRQ?
- How would you test this race?

### 12. How would you debug "IRQ count increases, but `evtest` shows nothing"?

**Level:** Senior

**Short Answer:**  
Check that the driver advertises the event, reports the right type/code/value, calls `input_sync()`, reads the GPIO polarity correctly, and that `evtest` is attached to the right event node.

**Deep Explanation:**  
An increasing IRQ count proves only that an interrupt fires. It does not prove the input event path is correct. The handler may read the wrong state, return before reporting, report an unadvertised key code, forget `input_sync()`, or report on a different input device. Userspace may also be listening to the wrong node.

**API / Code Anchor:**  
`/proc/interrupts`, `/proc/bus/input/devices`, `evtest`, `input_report_key()`, `input_sync()`, `input_set_capability()`.

**Production or Debugging Angle:**  
Use layered evidence: IRQ count, driver logs, raw GPIO state, input capabilities, and actual evdev events. Do not stop at one layer.

**Common Traps:**  
- Looking only at `/proc/interrupts`.
- Forgetting active-low conversion.
- Reporting `KEY_ENTER` without advertising it.
- Running `evtest /dev/input/event0` when the device is `event3`.

**Follow-up Questions:**  
- What would you check in `/proc/bus/input/devices`?
- How can `udevadm info` help?
- What dynamic debug logs would you add?

### 13. How do `open()` and `close()` affect power management?

**Level:** Senior

**Short Answer:**  
They let the driver enable hardware only when an input handler is actually using the device, and disable it when the last user closes it.

**Deep Explanation:**  
The input core tracks users. `open()` runs for the first user; `close()` runs for the last. A driver can request IRQs, enable regulators/clocks, start polling, or take runtime-PM references in `open()`, then undo that in `close()`. For wakeup keys, the policy can be more subtle because the device may need to wake the system even when no normal userspace reader is active.

**API / Code Anchor:**  
`input->open`, `input->close`, `pm_runtime_get_sync()` or `pm_runtime_resume_and_get()`, `pm_runtime_put_autosuspend()`, wake IRQ helpers.

**Production or Debugging Angle:**  
Power bugs often look like "first event after idle is missing" or "button cannot wake from suspend." Trace whether hardware was powered and IRQ wake was configured.

**Common Traps:**  
- Requesting IRQ in `open()` but freeing state in remove without closing/synchronizing.
- Powering down a wake-capable key incorrectly.
- Assuming an input device is always open.

**Follow-up Questions:**  
- When is it better to request IRQ in probe?
- How would you handle `wakeup-source`?
- What happens if userspace never opens the event node?

### 14. How do you choose event codes for a product button?

**Level:** Senior

**Short Answer:**  
Choose codes that match the semantic action userspace expects, and document them in Device Tree or driver data.

**Deep Explanation:**  
The code is ABI. `KEY_POWER`, `KEY_ENTER`, `KEY_VOLUMEUP`, `BTN_LEFT`, and `BTN_0` communicate different meanings to userspace. Desktop, embedded UI, Android, and systemd/logind may react differently. A board "user button" might be `KEY_PROG1` or a product-specific key, while a real power button should be `KEY_POWER` and may also be a wake source.

**API / Code Anchor:**  
`linux,code = <KEY_POWER>`, `input_set_capability(input, EV_KEY, KEY_POWER)`, `include/uapi/linux/input-event-codes.h`.

**Production or Debugging Angle:**  
Changing a key code after release can break applications. Treat it like a userspace ABI change.

**Common Traps:**  
- Using `BTN_0` for everything.
- Choosing a key code because it "works in evtest" rather than matching product semantics.
- Forgetting autorepeat policy for keys that should repeat.

**Follow-up Questions:**  
- When would you use `KEY_PROG1`?
- What is the difference between `KEY_*` and `BTN_*`?
- How do you expose this choice in Device Tree?

### 15. How do debounce and active-low handling interact?

**Level:** Senior

**Short Answer:**  
Debounce filters unstable transitions; active-low maps electrical level to logical state. Both must be correct before reporting press/release values.

**Deep Explanation:**  
Mechanical buttons bounce electrically, so an edge-triggered IRQ can produce multiple transitions for one press. Active-low buttons also invert physical voltage: pressed may be electrical low but logical true. Descriptor GPIO APIs can account for polarity if Device Tree is correct. Debounce can be provided by pinctrl/GPIO hardware, `gpio-keys` `debounce-interval`, polling interval, or delayed-work filtering.

**API / Code Anchor:**  
`GPIO_ACTIVE_LOW`, `gpiod_get_value_cansleep()`, `gpiod_set_debounce()`, `debounce-interval`, `input_report_key()`.

**Production or Debugging Angle:**  
Bounce and polarity bugs show up as duplicate presses, missing releases, or inverted UI behavior. Test with both `evtest` and raw GPIO visibility during bring-up.

**Common Traps:**  
- Reading raw electrical level and treating high as pressed.
- Ignoring `gpiod_set_debounce()` failure on controllers that do not support it.
- Using only falling-edge IRQ and never reporting release.

**Follow-up Questions:**  
- How would you debounce without hardware support?
- Which edge flags should a press/release button use?
- What does `gpio-keys` provide here?

### 16. What are the risks of using raw GPIO sysfs or libgpiod for product buttons?

**Level:** Senior

**Short Answer:**  
It bypasses the standard input ABI and pushes key mapping, debounce, permissions, wakeup, and event semantics into userspace.

**Deep Explanation:**  
Raw GPIO access is useful for diagnostics or generic line control. But a product button is a semantic input event. Userspace should not need to know which GPIO line, polarity, debounce time, or IRQ edge represents `KEY_POWER`. The kernel should expose a stable event through input, often with `gpio-keys`.

**API / Code Anchor:**  
Input: `gpio-keys`, `input_report_key()`, `/dev/input/eventX`.  
GPIO userspace: `/dev/gpiochipX`, sysfs GPIO on older systems.

**Production or Debugging Angle:**  
Input integrates with udev, logind, GUI stacks, libinput, and standard test tools. Raw GPIO often creates permission and portability problems.

**Common Traps:**  
- Treating libgpiod as a replacement for input events.
- Letting applications encode board-specific GPIO numbers.
- Losing wakeup/autorepeat/key semantics.

**Follow-up Questions:**  
- When is libgpiod appropriate?
- How would you migrate a raw GPIO button to `gpio-keys`?
- What userspace pieces consume evdev?

## Debugging Scenarios

### 17. A GPIO button shows press events but no release events. What do you check?

**Level:** Mid-level

**Short Answer:**  
Check IRQ trigger flags, GPIO polarity, handler logic, and whether both press and release states are reported.

**Deep Explanation:**  
If the IRQ triggers only on one edge, the handler may never run on release. If polarity is inverted, release may be reported as press. If the handler always reports value `1`, userspace never sees release. For input keys, userspace expects both `1` and `0` state transitions.

**API / Code Anchor:**  
`IRQF_TRIGGER_RISING`, `IRQF_TRIGGER_FALLING`, `gpiod_get_value_cansleep()`, `input_report_key(input, code, pressed)`, `input_sync()`.

**Production or Debugging Angle:**  
Use `evtest` to inspect value transitions and `/proc/interrupts` to see whether both edges fire.

**Common Traps:**  
- Using only falling edge on active-low hardware.
- Hardcoding `input_report_key(..., 1)`.
- Forgetting Device Tree `GPIO_ACTIVE_LOW`.

**Follow-up Questions:**  
- How would `gpio-keys` represent this?
- What if the controller supports only one edge?
- How would polling handle release detection?

### 18. `input_register_device()` succeeds, but `/proc/bus/input/devices` shows no expected key capability. Why?

**Level:** Mid-level

**Short Answer:**  
The driver likely did not set the correct capability bits before registration.

**Deep Explanation:**  
Capability bits are captured by input core and exposed to userspace. If the driver sets only `input->name` and registers, userspace sees a device but not the expected `EV_KEY`/`KEY_*` capability. Manual bit setup must set both event type and specific code, or the driver should use `input_set_capability()`.

**API / Code Anchor:**  
`input_set_capability(input, EV_KEY, KEY_ENTER)`, `input->evbit`, `input->keybit`, `input_register_device()`.

**Production or Debugging Angle:**  
`/proc/bus/input/devices` is a quick capability audit. It often catches setup bugs before you inspect IRQs.

**Common Traps:**  
- Setting `keybit` after registration.
- Setting only `KEY_ENTER` but not `EV_KEY`.
- Reporting an event code that was never declared.

**Follow-up Questions:**  
- How do you declare relative axes?
- How do you declare absolute axes?
- What does `input_set_capability()` simplify?

## Common Traps Review

### 19. Name five review comments you would leave on a fragile input driver.

**Level:** Senior

**Short Answer:**  
I would check subsystem choice, capability setup, IRQ context, debounce/polarity, and teardown ordering.

**Deep Explanation:**  
A fragile input driver often works on a developer board but fails in production. Review should ask whether the hardware could use `gpio-keys`, whether capabilities are correct before registration, whether the handler can sleep safely, whether active-low and debounce are handled, and whether remove stops events before freeing memory.

**API / Code Anchor:**  
`gpio-keys`, `input_set_capability()`, `gpiod_get_value_cansleep()`, `devm_request_threaded_irq()`, `input_sync()`, `input_unregister_device()`.

**Production or Debugging Angle:**  
These are the issues that cause intermittent field bugs: duplicate events, missed wakeups, unload crashes, wrong key mapping, or applications listening to unstable event nodes.

**Common Traps:**  
- Good-looking code that reports unadvertised events.
- Correct IRQ count but no evdev events.
- Cleanup that assumes devm solves event-path races.

**Follow-up Questions:**  
- Which issue would you prioritize in code review?
- What tests would you ask for?
- How would you decide between custom driver and binding-only `gpio-keys`?
