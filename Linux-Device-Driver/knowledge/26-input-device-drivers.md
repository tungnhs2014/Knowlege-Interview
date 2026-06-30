# 26 - Input Device Drivers

## Learning Goal
Understand how Linux represents keyboards, buttons, mice, touchscreens, and similar devices through the input subsystem. After this topic, you should be able to design a small input driver, report events correctly, expose `/dev/input/eventX`, and debug common bring-up failures.

After this topic you should be able to:

- Explain **input core**, **evdev**, and `struct input_dev`.
- Choose input instead of a private character device or raw GPIO userspace ABI.
- Declare event capabilities with `EV_KEY`, `EV_REL`, `EV_ABS`, `KEY_*`, `BTN_*`, `REL_*`, and `ABS_*`.
- Report press/release, relative movement, and absolute positions.
- Explain why `input_sync()` matters.
- Build the lifecycle for IRQ-driven and polled input devices.
- Use `gpio-keys` when a custom driver is not needed.
- Debug missing events, inverted polarity, bounce, IRQ races, and userspace node confusion.

## Why This Matters In Real Work
User input is a standard Linux ABI, not something every board driver should invent. A power button, keypad, rotary control, touchscreen interrupt, GPIO key, or PMIC on-key should become input events that existing userspace already understands.

You meet input drivers when:

- A GPIO line represents a button, switch, wake key, or board key.
- A keypad, touch controller, mouse, joystick, or gamepad reports user actions.
- A PMIC/MFD child device exposes an on-key or power key.
- Userspace expects `evtest`, `libinput`, desktop sessions, Android input, or embedded UI software to see events.
- You need a clean ABI under `/dev/input/eventX` instead of a custom `/dev/my_button`.
- You must coordinate GPIO polarity, debounce, IRQ threading, runtime PM, and wakeup behavior.

Without the input subsystem, every key/button driver would repeat event formats, polling behavior, device-node creation, key mapping, userspace discovery, and compatibility work.

## Mental Model
Think of an input driver as a **translator from hardware state changes to standard Linux events**.

```text
button / keyboard / mouse / touchscreen hardware
  -> bus, GPIO, IRQ, or polling code in the driver
  -> input core
  -> evdev handler
  -> /dev/input/eventX
  -> userspace reads struct input_event records
```

For a single GPIO button:

- Hardware changes from released to pressed.
- IRQ thread or poll callback reads the GPIO.
- Driver reports `KEY_ENTER = pressed`.
- Driver calls `input_sync()`.
- Userspace sees an `EV_KEY` event followed by `EV_SYN / SYN_REPORT`.

**A button is not just a GPIO file.** In production, a user-facing button should usually be an input event, not an exported GPIO read by an application.

## Core Concepts
The input subsystem has a small vocabulary. The hard part is using the vocabulary consistently so userspace sees the right device.

| Concept | Meaning |
| --- | --- |
| Input core | Kernel subsystem that receives normalized input events from drivers. |
| Input device | One logical input device represented by `struct input_dev`. |
| Input handler | Kernel consumer of input events. `evdev` is the common userspace-facing handler. |
| evdev | Event interface that exposes `/dev/input/eventX`. |
| Event type | Broad category such as key, relative movement, absolute axis, switch, LED. |
| Event code | Specific key, button, axis, or switch within an event type. |
| Event value | State or amount: key up/down/repeat, axis value, relative delta. |
| Event frame | Group of related reports ending with `input_sync()`. |
| Capability bits | Bitmaps that tell input core and userspace which events the device can emit. |

Common event types:

| Type | Typical Codes | Meaning |
| --- | --- | --- |
| `EV_KEY` | `KEY_ENTER`, `KEY_POWER`, `BTN_LEFT`, `BTN_0` | Key or button press/release/repeat. |
| `EV_REL` | `REL_X`, `REL_Y`, `REL_WHEEL` | Relative movement, such as mouse deltas. |
| `EV_ABS` | `ABS_X`, `ABS_Y`, `ABS_MT_POSITION_X` | Absolute positions, joystick axes, touch coordinates. |
| `EV_SW` | `SW_LID`, `SW_TABLET_MODE` | Persistent switch state. |
| `EV_SYN` | `SYN_REPORT` | Event-frame synchronization. |

Useful comparisons:

| Use This | When |
| --- | --- |
| `gpio-keys` | Simple GPIO buttons or switches described by Device Tree. |
| Custom input driver | Hardware needs custom register reads, protocols, filtering, or state machine. |
| Raw GPIO userspace | GPIO diagnostics or general-purpose line control, not a stable keyboard/button ABI. |
| IIO | Measurement sensors and ADC/DAC channels. |
| V4L2 | Camera/video sensors and media pipelines. |
| HID | USB/Bluetooth/I2C HID class devices when HID describes the device well. |

## Kernel Mechanism
The input driver owns hardware access. The input core owns event routing and the standard ABI.

The usual object relationship:

```text
platform / I2C / SPI / MFD child device
  -> driver private data
       -> GPIO descriptor, IRQ, regmap/client pointer, locks, PM state
       -> struct input_dev *
  -> input core
       -> handlers such as evdev
       -> /dev/input/eventX
```

The driver normally does this:

- Allocates `struct input_dev`.
- Fills identity fields such as `name`, parent device, and bus type.
- Declares capabilities before registration.
- Sets optional absolute-axis ranges.
- Stores private data with `input_set_drvdata()`.
- Registers with `input_register_device()`.
- Reports events from IRQ, polling, workqueue, or bus callbacks.
- Stops event production before device removal.

The input core does this:

- Tracks users of the input device.
- Calls `open()` when the first input handler opens the device.
- Calls `close()` when the last handler closes it.
- Filters and forwards events to handlers.
- Lets evdev expose the event stream to userspace.

`open()` and `close()` are important in real drivers. They let you keep the hardware off until someone cares:

- Request/enable IRQ only while the device is open.
- Start/stop polling only while in use.
- Power up/down the chip.
- Coordinate runtime PM references.
- Reduce idle power on battery devices.

## Key Structs And APIs
Learn these APIs by where they appear in the flow, not as a memorization dump.

### Core Objects

| Struct / API | Role |
| --- | --- |
| `struct input_dev` | Kernel object representing one input device. |
| `struct input_event` | Userspace event record read from `/dev/input/eventX`. |
| `struct input_absinfo` | Absolute-axis metadata: value, min, max, fuzz, flat, resolution. |
| `devm_input_allocate_device(dev)` | Managed allocation tied to the parent device. |
| `input_allocate_device()` | Unmanaged allocation. |
| `input_register_device(input)` | Register with input core and expose handlers. |
| `input_unregister_device(input)` | Unregister an unmanaged registered input device. |
| `input_free_device(input)` | Free an unmanaged device that was never registered. |

### Identity And Capabilities

| Field / API | Role |
| --- | --- |
| `input->name` | Human-readable device name shown by `evtest` and `/proc/bus/input/devices`. |
| `input->phys` | Optional physical path string. |
| `input->id.bustype` | Bus type such as `BUS_HOST`, `BUS_I2C`, `BUS_SPI`, `BUS_USB`. |
| `input->dev.parent` | Parent `struct device`. |
| `input_set_capability(input, type, code)` | Convenient helper to set event type and code bits. |
| `set_bit(EV_KEY, input->evbit)` | Manual capability bitmap setup. |
| `set_bit(KEY_ENTER, input->keybit)` | Manual key-code bitmap setup. |
| `input_set_abs_params()` | Configure `EV_ABS` axis range and filtering metadata. |
| `input_set_drvdata()` / `input_get_drvdata()` | Attach/retrieve driver private data. |

### Reporting Events

| API | Meaning |
| --- | --- |
| `input_report_key(input, code, value)` | Report key/button release, press, or repeat. |
| `input_report_rel(input, code, value)` | Report relative movement delta. |
| `input_report_abs(input, code, value)` | Report absolute axis value. |
| `input_event(input, type, code, value)` | Generic event reporting helper. |
| `input_sync(input)` | End the current event frame and emit `SYN_REPORT`. |

For `EV_KEY`:

| Value | Meaning |
| --- | --- |
| `0` | Released. |
| `1` | Pressed. |
| `2` | Autorepeat. |

### GPIO And IRQ Front End

| API | Role |
| --- | --- |
| `devm_gpiod_get(dev, "button", GPIOD_IN)` | Request a GPIO input by descriptor. |
| `gpiod_get_value_cansleep(desc)` | Read GPIO safely when provider may sleep. |
| `gpiod_set_debounce(desc, usec)` | Ask GPIO controller for hardware debounce if supported. |
| `gpiod_to_irq(desc)` | Convert GPIO line to Linux IRQ number. |
| `devm_request_threaded_irq()` | Request IRQ with managed cleanup. |
| `request_any_context_irq()` | Request handler when IRQ context may depend on provider. |
| `IRQF_TRIGGER_RISING` / `IRQF_TRIGGER_FALLING` | Edge-trigger flags for press/release changes. |
| `IRQF_ONESHOT` | Keep IRQ masked until threaded handler completes. |

### Polling APIs

Current kernels prefer polling support on a normal `struct input_dev`:

| API | Role |
| --- | --- |
| `input_setup_polling(input, poll_fn)` | Attach a poll callback to an input device. |
| `input_set_poll_interval(input, ms)` | Set default poll interval. |

Older code may use `struct input_polled_dev` and `linux/input-polldev.h`. Treat that as legacy unless your target kernel still uses it. Some kernel documentation or downstream trees may mention extra poll-interval bound helpers; verify those against your target headers before using them.

## Lifecycle / Data Flow
Input drivers are easiest to reason about as two flows: registration and event reporting.

### Probe Flow

```text
probe()
  allocate private state
  get GPIO / IRQ / bus resources
  allocate struct input_dev
  fill name, parent, bus type
  declare capabilities
  configure absolute axes if needed
  set driver data and open/close callbacks
  register input device
  request IRQ or configure polling
```

Ordering matters:

- Initialize all capability bits before `input_register_device()`.
- Initialize private state before IRQs can fire.
- If using unmanaged allocation, free only pre-registration failures with `input_free_device()`.
- After successful unmanaged registration, `input_unregister_device()` owns the cleanup path.
- With devm allocation, still reason carefully about IRQ handlers that may run after remove begins.

### IRQ-Driven Event Flow

```text
button edge
  -> IRQ thread runs
  -> read GPIO state with context-safe accessor
  -> convert physical state to logical pressed/released
  -> input_report_key(input, KEY_ENTER, pressed)
  -> input_sync(input)
  -> evdev queues struct input_event records
  -> userspace reads /dev/input/eventX
```

For GPIO expanders on I2C/SPI, avoid slow bus reads in hard IRQ context. Use a threaded IRQ, workqueue, or context-safe helper.

### Polled Event Flow

```text
poll timer fires
  -> poll callback reads hardware
  -> compare current state with previous state if needed
  -> report changed state
  -> input_sync()
```

Polling is for hardware without a reliable interrupt. It costs power and CPU time, so set intervals intentionally.

### Userspace Flow

```text
driver registers input_dev
  -> input core creates input device
  -> evdev creates /dev/input/eventX
  -> udev may create /dev/input/by-path/... symlink
  -> app uses read(), poll(), select(), libevdev, libinput, or evtest
```

Never assume the device is always `/dev/input/event0`. Discover it by name, path, udev properties, or `/proc/bus/input/devices`.

## Minimal Practical Example
This is **learning-only pseudo-code** for a GPIO button input driver. It shows the right shape; production code still needs board binding, wakeup policy, debounce validation, and real error-path testing.

```c
struct demo_button {
	struct device *dev;
	struct input_dev *input;
	struct gpio_desc *button;
	int irq;
};

static irqreturn_t demo_button_irq(int irq, void *data)
{
	struct demo_button *btn = data;
	int pressed;

	pressed = gpiod_get_value_cansleep(btn->button);
	if (pressed < 0)
		return IRQ_HANDLED;

	input_report_key(btn->input, KEY_ENTER, pressed);
	input_sync(btn->input);

	return IRQ_HANDLED;
}

static int demo_button_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct demo_button *btn;
	struct input_dev *input;
	int ret;

	btn = devm_kzalloc(dev, sizeof(*btn), GFP_KERNEL);
	if (!btn)
		return -ENOMEM;

	btn->button = devm_gpiod_get(dev, "button", GPIOD_IN);
	if (IS_ERR(btn->button))
		return PTR_ERR(btn->button);

	/* Optional: ignore -ENOTSUPP if the GPIO controller cannot debounce. */
	gpiod_set_debounce(btn->button, 20000);

	input = devm_input_allocate_device(dev);
	if (!input)
		return -ENOMEM;

	btn->dev = dev;
	btn->input = input;

	input->name = "demo-gpio-button";
	input->id.bustype = BUS_HOST;
	input->dev.parent = dev;

	input_set_capability(input, EV_KEY, KEY_ENTER);
	input_set_drvdata(input, btn);

	ret = input_register_device(input);
	if (ret)
		return ret;

	btn->irq = gpiod_to_irq(btn->button);
	if (btn->irq < 0)
		return btn->irq;

	ret = devm_request_threaded_irq(dev, btn->irq, NULL, demo_button_irq,
					IRQF_TRIGGER_RISING |
					IRQF_TRIGGER_FALLING |
					IRQF_ONESHOT,
					"demo-gpio-button", btn);
	if (ret)
		return ret;

	platform_set_drvdata(pdev, btn);
	return 0;
}
```

Important lines:

- `devm_input_allocate_device()` allocates the input object.
- `input_set_capability(input, EV_KEY, KEY_ENTER)` declares what userspace may see.
- `input_register_device()` exposes the device to input handlers such as evdev.
- `gpiod_get_value_cansleep()` is used because a GPIO provider may sleep.
- `input_report_key()` reports the state.
- `input_sync()` finishes the event frame.

For a simple board button, prefer a Device Tree `gpio-keys` node when it is enough:

```dts
gpio-keys {
	compatible = "gpio-keys";
	autorepeat;

	user_button {
		label = "User Button";
		gpios = <&gpio5 9 GPIO_ACTIVE_LOW>;
		linux,code = <KEY_ENTER>;
		debounce-interval = <20>;
		wakeup-source;
	};
};
```

That uses an existing kernel driver and avoids custom code.

## Common Bugs And Debugging
Debug input problems from the symptom outward: device node, capabilities, IRQ/polling, GPIO state, then driver lifetime.

### Symptom: no `/dev/input/eventX`

Likely causes:

- `input_register_device()` failed.
- Driver did not bind.
- Kernel lacks evdev support.
- Device is registered but udev permissions/symlinks differ.

Check:

```bash
dmesg | grep -i input
cat /proc/bus/input/devices
ls -l /dev/input/
udevadm info /dev/input/eventX
```

Fix patterns:

- Check probe return path.
- Verify compatible string and module autoload.
- Confirm `input->name` and parent device are set before registration.

### Symptom: event node exists, but `evtest` shows no key events

Likely causes:

- Capability bits were not set.
- IRQ is not firing.
- Poll callback is not configured.
- Driver reports events but forgets `input_sync()`.
- The application opened the wrong event node.

Check:

```bash
cat /proc/bus/input/devices
evtest /dev/input/eventX
cat /proc/interrupts | grep -i button
```

Fix patterns:

- Use `input_set_capability(input, EV_KEY, KEY_...)`.
- Add `input_sync()` after related reports.
- Discover the device by name instead of hardcoding `event0`.

### Symptom: press and release are inverted

Likely causes:

- Active-low GPIO was treated as physical value instead of logical value.
- Device Tree polarity is wrong.
- Driver used raw GPIO APIs inconsistently.

Check:

```bash
gpioinfo
gpioget <chip> <line>
cat /sys/kernel/debug/gpio
evtest /dev/input/eventX
```

Fix patterns:

- Use descriptor APIs so active-low can be handled logically.
- Validate `GPIO_ACTIVE_LOW` in Device Tree.
- Decide whether `pressed` means logical pressed, not electrical high.

### Symptom: one press produces many events

Likely causes:

- Mechanical bounce.
- Both edges trigger without debounce.
- Poll interval too short.
- IRQ storm due wrong trigger type or uncleared controller status.

Fix patterns:

- Use `gpiod_set_debounce()` if the controller supports it.
- Use `debounce-interval` in `gpio-keys`.
- Add delayed-work debounce when hardware support is missing.
- Verify IRQ status clear for GPIO expanders.

### Symptom: crash or use-after-free during unload/remove

Likely causes:

- IRQ handler runs after `input_dev` or private data was freed.
- Remove unregisters input before stopping event source.
- Work/polling is not canceled.
- Unmanaged input device is double-freed.

Fix patterns:

- Stop hardware event generation first.
- Synchronize/free IRQ before freeing state if not devm-managed in a safe order.
- Cancel delayed work and polling paths.
- Understand ownership after `input_register_device()`.

### Symptom: works on SoC GPIO, fails on I2C/SPI expander

Likely causes:

- Driver reads GPIO in hard IRQ context.
- Provider has `can_sleep`.
- Missing threaded IRQ or `IRQF_ONESHOT`.
- Nested GPIO IRQ demux is not configured correctly.

Fix patterns:

- Use `devm_request_threaded_irq()` with a `NULL` top half and threaded handler.
- Use `_cansleep` GPIO accessors.
- Check provider `to_irq()` / irqchip integration.

## Production Checklist
Before review or bring-up, walk through this list.

Subsystem choice:

- Use `gpio-keys` or `gpio-keys-polled` for simple GPIO buttons.
- Use HID for HID-described devices.
- Use input for user actions, not IIO-style measurements.
- Avoid private char devices for keyboard/button/touch events.

Device description:

- Device Tree has correct `compatible`, `gpios` or `interrupts`, `linux,code`, polarity, debounce, and wakeup properties.
- Key codes match the actual user-visible function.
- `KEY_*` versus `BTN_*` choice is intentional.
- `EV_REL` versus `EV_ABS` choice is intentional.
- Absolute axes have min/max/fuzz/flat/resolution where appropriate.

Driver lifecycle:

- All capabilities are set before `input_register_device()`.
- IRQ cannot fire before private state and `input_dev` are valid.
- `open()`/`close()` power and IRQ behavior is deliberate.
- Remove path stops IRQ/poll/work before freeing resources.
- Runtime PM and wakeup behavior are tested if relevant.

Concurrency:

- Sleeping bus/GPIO access is not done in hard IRQ context.
- Threaded IRQs use `IRQF_ONESHOT` when needed.
- Shared state has appropriate locking.
- Debounce and state-change logic cannot report stale or duplicate events unexpectedly.

Userspace ABI:

- Device appears in `/proc/bus/input/devices`.
- `evtest` reports expected event type/code/value.
- Udev properties and permissions are acceptable for the product.
- Tests do not hardcode unstable event numbers.

Debuggability:

- `dev_*()` logs identify probe failures and IRQ setup failures.
- IRQ count can be checked in `/proc/interrupts`.
- GPIO polarity can be checked with GPIO debug tools during bring-up.
- Dynamic debug or tracepoints can be enabled where useful.

## Interview Readiness
You are ready for interviews when you can explain the whole path from a physical press to userspace.

Be able to answer:

- Why input exists instead of custom character devices.
- What `struct input_dev` represents.
- How capability bits affect userspace-visible events.
- Why `input_sync()` is needed.
- How a GPIO button becomes `/dev/input/eventX`.
- When to use `gpio-keys`.
- Why GPIO expander buttons often need threaded IRQ handling.
- How to debug "IRQ fires but `evtest` shows nothing".
- How to avoid remove/unload races.

See `interview/26-input-device-drivers.md` for structured practice questions.

## Kernel Version Notes
The core input concepts are stable, but some helper APIs and examples drift across kernel versions.

- Older examples may use `struct input_polled_dev`, `input_allocate_polled_device()`, and `linux/input-polldev.h`.
- Current kernel documentation prefers `input_setup_polling()` and poll interval helpers on `struct input_dev`.
- Prefer `devm_input_allocate_device()` for parent-owned lifetimes, but still understand registration and IRQ ordering.
- Check target kernel headers before writing final module code, especially for polling helpers and managed IRQ/input cleanup behavior.
- For simple GPIO buttons, validate current `gpio-keys.yaml` properties such as `debounce-interval`, `wakeup-source`, `autorepeat`, and `poll-interval`.
