# Topic Brief - 26 - Input Device Drivers

## Output Targets
- Knowledge: `knowledge/26-input-device-drivers.md`
- Interview: `interview/26-input-device-drivers.md`
- Example: `examples/26-input-device-drivers/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch17` | `docs/Linux Device Driver Development/Chapter 17- Input Devices Drivers.md` | read/mapped/covered/merged | Primary source for input core: `struct input_dev`, event capability bitmaps, `EV_KEY`/`EV_REL`/`EV_ABS`, `input_set_abs_params()`, allocation/register/unregister APIs, open/close callbacks, polled versus IRQ-driven input, `input_report_*()`, `input_sync()`, `/dev/input/eventX`, `struct input_event`, userspace `select()` example, `evtest`, `udevadm`, `/proc/interrupts`, and GPIO debug checks. |
| `ldd1-ch03` | `docs/Linux Device Driver Development/Chapter 3-Kernel Facilities and Helper Functions.md` | read/mapped/supporting-covered | Threaded IRQ keyboard/keypad example using `input_report_key()` and `input_sync()` in a threaded bottom half; reinforces `IRQF_ONESHOT`, sleeping bus access, and input event reporting from interrupt context. Main IRQ/bottom-half theory remains topic 15. |
| `ldd1-ch14` | `docs/Linux Device Driver Development/Chapter 14-Pin Control and GPIO Subsystem.md` | read/mapped/covered-adjacent | GPIO button mechanics feeding input drivers: input direction, `gpio_to_irq()`, `gpiod_to_irq()`, `gpiod_set_debounce()`, `gpiod_cansleep()`, `_cansleep` accessors, DT interrupt mapping, sysfs GPIO polling. Used to explain button hardware, debounce, active-low, and why final input docs should prefer input ABI over ad hoc GPIO userspace. |
| `ldd1-ch15` | `docs/Linux Device Driver Development/Chapter 15-GPIO Controller Drivers.md` | read/mapped/related | Provider-side GPIO details that affect input buttons: `gpio_chip.set_debounce`, `gpio_chip.to_irq`, `can_sleep`, irqchip integration, and DT GPIO/interrupt-controller cells. Full GPIO provider material remains topic 14. |
| `ldd1-ch16` | `docs/Linux Device Driver Development/Chapter 16-Advanced IRQ Management.md` | read/mapped/related | IRQ-controller and IRQ-multiplexing background for buttons behind GPIO expanders; supports nested/chained GPIO IRQ explanation. Full IRQ-domain handling remains topic 15. |
| `ldd1-ch13` | `docs/Linux Device Driver Development/Chapter 13-The Linux Device Model.md` | read/mapped/incidental | Names mice/keyboards as input-device class examples; useful only for tying input devices to device model/sysfs. Full device-model details remain topic 12. |
| `ldd2-source-root` | `docs/Linux Device Driver Development 2/` | searched/mapped/gap | No dedicated book-2 input-framework chapter found. Relevant input hits are IRQ-button and MFD on-key/keypad examples, recorded below. |
| `ldd2-ch01` | `docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md` | read/mapped/covered-adjacent | GPIO button IRQ example using `request_any_context_irq()`, `gpiod_get()`, `gpiod_to_irq()`, `input_report_key()`, and `input_sync()`. Adds warning that GPIO may come from an I2C/SPI expander, so IRQ context may vary. |
| `ldd2-ch03` | `docs/Linux Device Driver Development 2/Chapter 3-MFD_Subsystem_and_Syscon_API.md` | read/mapped/supporting-covered | MFD input subdevice examples: ADP5520 includes keypad functionality; DA9062/DA9063 `onkey` child device maps MFD IRQ resources to `drivers/input/misc/da9063_onkey.c`; SNVS power key node uses `linux,keycode = <KEY_POWER>` and `wakeup-source`. |
| `notion-source-root` | `docs/Linux-Device-Driver-Notion/` | searched/mapped/gap | No standalone Notion input-device-driver chapter found. Notion has distributed GPIO, pinctrl, IRQ, platform, and userspace snippets relevant to input drivers. |
| `notion-ch01-part1` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 1 Environment Setup and Getting Source Code.md` | read/mapped/incidental | Kernel source-tree map lists `drivers/input/` for keyboards, mice, and touchscreens. |
| `notion-ch04-part1` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 1 Character Device Registration.md` | read/mapped/incidental | Shows `/dev/input/mouse0` as a character-device style userspace node; useful to clarify that input drivers should use input core/evdev rather than private `cdev` registration. |
| `notion-ch05-part1` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 1 Platform Bus & Driver Basics.md` | read/mapped/related | Probe checklist includes registering with frameworks such as `input_register_device()` and then setting up interrupts. Used for platform-bus lifecycle framing. |
| `notion-ch05-part2` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 2 Probe, Remove & Resource Management.md` | read/mapped/related | Probe/remove flow includes subsystem registration and platform data with `debounce_interval`; useful for legacy/platform-data contrast. |
| `notion-ch14-part1` | `docs/Linux-Device-Driver-Notion/Chapter 14-Part 1 Pin Control Framework.md` | read/mapped/covered-adjacent | Pinctrl details for GPIO buttons: input mode, pull-up, debounce, Schmitt trigger, `gpio-keys` with `pinctrl-0`, `linux,code`, and active-low GPIO. |
| `notion-ch14-part2` | `docs/Linux-Device-Driver-Notion/Chapter 14-Part 2 GPIO Consumer Interfaces.md` | read/mapped/covered-adjacent | GPIO consumer button mechanics: `gpiod_set_debounce()`, `gpiod_to_irq()`, active-low logic, `_cansleep`, IRQ request patterns, and LED/button examples. Used for input-driver GPIO resource handling and common traps. |
| `notion-ch14-part3` | `docs/Linux-Device-Driver-Notion/Chapter 14-Part 3 Userspace GPIO Access.md` | read/mapped/related | Userspace GPIO sysfs/libgpiod button examples; used as contrast with input/evdev ABI. Full GPIO userspace belongs to GPIO/userspace ABI topics. |
| `notion-ch15-part1` | `docs/Linux-Device-Driver-Notion/Chapter 15-Part 1 GPIO Controller Architecture.md` | read/mapped/related | Provider-side `set_debounce`, `to_irq`, and `gpio-keys` DT examples for GPIO expanders; explains why input-button behavior depends on GPIO provider capabilities. |
| `notion-ch15-part2` | `docs/Linux-Device-Driver-Notion/Chapter 15-Part 2 IRQ Chip Integration.md` | read/mapped/related | GPIO expander IRQ multiplexing example where a child button handler runs after parent IRQ demux. Full irqchip integration remains topics 14-15. |
| `notion-ch15-part3` | `docs/Linux-Device-Driver-Notion/Chapter 15-Part 3 Advanced Features and Integration.md` | read/mapped/covered-adjacent | `gpio-keys` DT example with `compatible = "gpio-keys"`, GPIO expander line, and `linux,code = <KEY_F1>`. Useful for no-custom-driver cases. |
| `notion-ch16-part1` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 1 IRQ Architecture and Propagation.md` | read/mapped/covered-adjacent | End-to-end button interrupt path through GPIO expander, GIC, nested IRQ, and input reporting via `input_report_key(button->input, KEY_ENTER, 1)` plus `input_sync()`. |
| `notion-ch16-part3` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 3 Advanced Topics.md` | read/mapped/related | Advanced GPIO expander integration and `gpio-keys` example with `wakeup-source`-style button context; useful for interrupt/wakeup caveats. |

## Source Files Read
- `ldd1-ch17`: complete chapter. Read input data structures, allocation/registration, polled input subclass, event reporting, userspace interface, full polled GPIO button example, full IRQ-driven GPIO button example, `udevadm`, `evtest`, `/proc/interrupts`, and debugfs GPIO checks.
- `ldd1-ch03`: targeted threaded IRQ section around `pcf8574_kp_irq_handler`, `request_threaded_irq()`, `IRQF_ONESHOT`, `input_report_key()`, and `input_sync()`.
- `ldd1-ch14`: targeted GPIO button, descriptor, debounce, IRQ mapping, DT interrupt mapping, and GPIO sysfs poll sections.
- `ldd1-ch15`: targeted `gpio_chip` provider fields and comments for `direction_input`, `set_debounce`, `to_irq`, `can_sleep`, irqchip support, and DT GPIO/interrupt cells.
- `ldd1-ch16`: targeted IRQ-controller/multiplexing material related to GPIO controllers acting as interrupt controllers for child devices.
- `ldd1-ch13`: targeted device-model introduction naming mice/keyboards as input device class examples.
- `ldd2-ch01`: targeted IRQ management section with GPIO button example and `request_any_context_irq()`.
- `ldd2-ch03`: targeted MFD introduction, DA9062/DA9063 on-key child device resource mapping, and SNVS power-key DT node with `linux,keycode` and `wakeup-source`.
- `notion-ch01-part1`: targeted source-tree map entry for `drivers/input/`.
- `notion-ch04-part1`: targeted `/dev/input/mouse0` device-node example.
- `notion-ch05-part1`: targeted platform-driver probe checklist with `input_register_device()`.
- `notion-ch05-part2`: targeted probe/remove flow and platform-data debounce field.
- `notion-ch14-part1`: targeted pin configuration, GPIO button pull-up/debounce, and `gpio-keys` DT example.
- `notion-ch14-part2`: targeted GPIO consumer flow, descriptor APIs, debounce, `gpiod_to_irq()`, `_cansleep` accessors, and button examples.
- `notion-ch14-part3`: targeted sysfs GPIO and libgpiod userspace button event examples for ABI comparison.
- `notion-ch15-part1`: targeted GPIO controller `set_debounce`, `to_irq`, and `gpio-keys` expander example.
- `notion-ch15-part2`: targeted GPIO expander IRQ demux scenario with child button IRQ.
- `notion-ch15-part3`: targeted `gpio-keys` DT snippet.
- `notion-ch16-part1`: targeted real button IRQ path and input-reporting example.
- `notion-ch16-part3`: targeted advanced GPIO expander and `gpio-keys` snippets.

### Inventory Decisions
- `ldd1-ch17` is the only dedicated internal input-framework source and is primary for topic 26.
- `ldd2` has no dedicated input-device-driver chapter. `ldd2-ch01` and `ldd2-ch03` were read as adjacent/supporting sources, not as same-number or replacement input chapters.
- Notion has no standalone input chapter. Notion same-number chapters around GPIO and IRQ were read independently because they contain `gpio-keys`, debounce, and input-reporting examples that are useful for topic 26.
- `ldd1-ch10` and `ldd1-ch22` had false-positive "input" hits: IIO uses `_input` as an attribute suffix, and networking has `skb->input_dev`. These were not treated as input-subsystem sources.
- `ldd2-ch10`, `ldd2-ch05`, `ldd2-ch06`, `ldd2-ch08`, and `ldd2-ch11` had generic "input" terminology unrelated to Linux input devices and were excluded after search/mapping.
- Related topic boundaries:
  - Generic GPIO consumer APIs remain topic 13.
  - GPIO provider/irqchip details remain topic 14.
  - Generic interrupt management remains topic 15.
  - I2C/SPI bus mechanics remain topics 16-17.
  - Runtime PM and wakeup policy remain topic 24.
  - IIO sensors remain topic 25.
  - Userspace GPIO ABI remains topic 08/13-related material; input drivers should use evdev for user-input events.

## Merged Source Notes
- Input devices are hardware that produce user-input events: keys, buttons, keyboards, mice, touchscreens, touchpads, joysticks, gamepads, rotary controls, and similar devices.
- The mental model is "hardware event to standard event stream":
  - The driver talks to hardware over GPIO, I2C, SPI, USB, MFD child resources, or platform MMIO.
  - The input core receives normalized events such as key press/release, relative movement, or absolute coordinates.
  - Event handlers such as evdev expose the stream to userspace as `/dev/input/eventX`.
- `struct input_dev` is the central kernel object. The driver fills identity, parent device, supported event bitmaps, optional absolute-axis parameters, optional open/close callbacks, optional keymaps, and driver-private data before registration.
- Event capability bitmaps advertise what the device can emit:
  - `evbit`: event types such as `EV_KEY`, `EV_REL`, `EV_ABS`, `EV_SW`, `EV_LED`, `EV_SND`.
  - `keybit`: key/button codes such as `KEY_ENTER`, `KEY_POWER`, `BTN_0`.
  - `relbit`: relative axes such as `REL_X`, `REL_Y`, wheel movement.
  - `absbit`: absolute axes such as `ABS_X`, `ABS_Y`, multitouch coordinates, joystick positions.
- For absolute axes, use `input_set_abs_params(dev, axis, min, max, fuzz, flat)` before registration. The internal `absinfo` data describes current value, range, noise tolerance, flat zone, and related metadata.
- Allocate/register lifecycle from the primary source:
  - Allocate with `input_allocate_device()` or preferably `devm_input_allocate_device()` when tied to a parent device.
  - Fill `name`, `phys` if useful, `id.bustype`, `dev.parent`, event capability bits, axis parameters, callbacks, and private state.
  - Register with `input_register_device()`.
  - On pre-registration failure, use `input_free_device()` for unmanaged devices.
  - After successful registration, use `input_unregister_device()` for unmanaged cleanup; do not also free it as a separate live object unless the API/version pattern requires it.
- `input_register_device()` may sleep and must not be called from interrupt context or while holding a spinlock.
- `open()` and `close()` callbacks are demand-driven resource gates:
  - `open()` is called when the first handler/user opens the input device.
  - `close()` is called when the last handler/user closes it.
  - They are useful for requesting/freeing IRQs, enabling/disabling polling, powering hardware, taking/releasing runtime-PM references, and reducing idle power.
- Event reporting APIs:
  - `input_report_key(dev, code, value)` for `EV_KEY`; value `0` means release, `1` press, and `2` autorepeat.
  - `input_report_rel(dev, code, value)` for relative deltas.
  - `input_report_abs(dev, code, value)` for absolute positions or measurements represented as input axes.
  - `input_sync(dev)` terminates the event frame and emits `SYN_REPORT` to listeners.
- Input core filters some unchanged events; drivers still need correct state reads, debounce, and frame boundaries.
- IRQ-driven input is the common choice when hardware can interrupt on state change. GPIO buttons usually map GPIOs to IRQs with `gpiod_to_irq()` or bus-provided IRQ fields, then request an interrupt handler.
- If the GPIO provider can sleep, such as I2C/SPI expanders, use a threaded IRQ path or context-safe request helper. Sources emphasize `request_threaded_irq(..., IRQF_ONESHOT, ...)` and `request_any_context_irq()` for GPIOs whose provider context is unknown.
- Polled input is for hardware that cannot interrupt. The internal primary source uses old `struct input_polled_dev` and `input_allocate_polled_device()`; current kernel docs prefer `input_setup_polling()` on a normal `struct input_dev`.
- Userspace ABI:
  - A registered evdev device appears under `/dev/input/eventX`, often with `/dev/input/by-path/` or `/dev/input/by-id/` symlinks created by udev.
  - Userspace reads `struct input_event` records containing timestamp, type, code, and value.
  - Applications can use blocking reads, nonblocking reads, `poll()`, or `select()`.
  - `evtest /dev/input/eventX` is the simplest manual validation tool.
  - `udevadm info /dev/input/eventX` shows major/minor, subsystem, path, and input properties.
- Device Tree can avoid custom driver code for simple GPIO buttons:
  - Use `compatible = "gpio-keys"` or, when polling is needed, `compatible = "gpio-keys-polled"`.
  - Each child button normally provides `gpios` or `interrupts`, `label`, `linux,code`, optional `linux,input-type`, optional `debounce-interval`, optional `wakeup-source`, and optional `autorepeat`.
- MFD examples show that input devices can be child functions of a PMIC or companion chip. The MFD core creates child platform devices and IRQ resources; the child input driver registers the evdev-facing input device.
- Wakeup keys such as power keys connect topic 26 with topic 24. The input driver reports `KEY_POWER`, but wake policy and suspend behavior require device PM/wakeup handling.
- `gpio-keys`/input is the correct ABI for real user buttons. Raw GPIO sysfs/libgpiod button examples are useful for GPIO learning, but they are not the preferred final interface for keyboard-like user input.

## Source Differences
- `ldd1-ch17` uses the older `input_polled_dev` API and `#include <linux/input-polldev.h>`. Current kernel input documentation uses `input_setup_polling()`, `input_set_poll_interval()`, and related helpers on `struct input_dev`. Final learner/example docs should teach the current API and mention the old API as legacy/source-history.
- `ldd1-ch17` examples have several extraction or code issues that should not be copied directly:
  - `input_register_polled_device(mcp->poll_dev)` likely meant `ms->poll_dev`.
  - `struct my_strut` typo.
  - `gpiod_get(dev, "reset", GPIOD_IN)` uses an undefined `dev`.
  - IRQ example calls `gpiod_to_irq(priv->btn_gpiod)` before assigning `priv->btn_gpiod = gpiod`.
  - Remove path unregisters/frees input before freeing IRQ; final code should prevent IRQ handlers from racing with a destroyed input device.
  - `input_unregister_device()` and `input_free_device()` are both shown in remove after successful registration; final unmanaged examples must use correct ownership for the target API and avoid double-free patterns.
- `ldd1-ch17` says "use classic input devices if no IRQ is provided, or fall back to polled" but the logic is inverted by context. Final docs should say IRQ-driven input when an IRQ exists; polling only when no IRQ or no reliable interrupt exists.
- `ldd1-ch14` and Notion GPIO examples sometimes report button-like behavior through raw GPIO sysfs/libgpiod rather than the input subsystem. Final topic 26 should present those as lower-level debugging/comparison paths, not as the production ABI for user buttons.
- `ldd2-ch01` duplicates the GPIO button IRQ pattern from `ldd1-ch17` but adds `request_any_context_irq()` reasoning. It also inherits the same ordering bug around `priv->btn_gpiod`; final docs should correct the order.
- `ldd2-ch03` focuses on MFD/syscon, not input architecture. It should be merged only for MFD child-resource and power-key examples.
- Notion GPIO/IRQ chapters are richer on debounce, active-low semantics, GPIO expander IRQ demux, and pinctrl, but they are not input-core chapters. Merge them as hardware-front-end context.
- Current kernel docs add input-device inhibition and runtime-PM implications around `open()`/`close()`, which are not present in the internal chapter. Final docs should include this as a version/current-behavior note.

## Gaps / Uncertainties
- Internal sources do not deeply explain evdev ioctls, capability discovery, keymap ioctls, event grabbing, repeat controls, or `libevdev`.
- Internal sources do not cover multitouch in detail: slots, tracking IDs, `input_mt_*()` helpers, pointer emulation, `ABS_MT_*`, and synchronization rules.
- Internal sources do not cover force feedback, LEDs/sound output events, switches (`EV_SW`), tablets, gamepads, HID layering, or uinput beyond event basics.
- Internal sources do not cover input properties (`INPUT_PROP_*`) or how userspace classifies touchpads, touchscreens, pointers, and tablets.
- Internal sources do not cover in-kernel input handlers beyond evdev/mousedev/keyboard basics from external docs.
- Internal sources do not provide a clean current minimal module that builds against modern headers. The example step should validate against local kernel headers and avoid legacy `input-polldev.h`.
- Internal sources do not cover security and permissions for `/dev/input/eventX`, udev tags, seat/session access, or container access.
- Internal sources do not cover robust debounce strategies beyond GPIO controller debounce and polling interval; mechanical bounce, IRQ storms, and delayed-work debounce should be validated for example code.
- Internal sources do not cover production suspend/resume and wake handling for power keys in depth; keep detailed PM in topic 24 but include input-facing warnings.
- Internal sources do not cover current YAML binding details for `gpio-keys`, `gpio-keys-polled`, matrix keypads, rotary encoders, or touchscreens beyond snippets.

## External Validation
- Used: `https://docs.kernel.org/input/input.html`
  - Validated current high-level input architecture: device drivers feed events into the input core; event handlers expose them through interfaces such as evdev; evdev is the preferred userspace interface and creates `/dev/input/eventX` nodes.
- Used: `https://docs.kernel.org/input/input-programming.html`
  - Validated current driver-programming flow: allocate `struct input_dev`, set event bitfields, call `input_register_device()`, report with `input_report_key()`/`input_report_rel()`/`input_report_abs()`, call `input_sync()`, use `open()`/`close()` for demand-driven resources, and use current polling helpers such as `input_setup_polling()` instead of legacy `input_polled_dev`.
- Used: `https://docs.kernel.org/input/event-codes.html`
  - Validated current event-code framing, especially `EV_SYN`/`SYN_REPORT`, event type/code/value semantics, and the need for correct userspace interpretation.
- Used: `https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/input/gpio-keys.yaml`
  - Validated current `gpio-keys`/`gpio-keys-polled` binding direction: `compatible` values, `autorepeat`, `poll-interval`, child key nodes, `gpios`/`interrupts`, `linux,code`, debounce, and wakeup properties.
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/include/uapi/linux/input-event-codes.h`
  - Validated current UAPI location for event type and key/button/axis codes.
- Validation performed for current outputs:
  - Local target kernel: `6.8.0-124-generic`.
  - Local headers expose `devm_input_allocate_device()`, `input_setup_polling()`, `input_set_poll_interval()`, `input_set_capability()`, `input_set_abs_params()`, `input_set_drvdata()`, and `devm_request_threaded_irq()`.
  - Local headers do not expose `input_set_poll_interval_min()` or `input_set_poll_interval_max()`; learner docs should avoid presenting those as current validated APIs for this target.
  - Example module `examples/26-input-device-drivers/ldd_input_demo.c` builds with `make` against `/lib/modules/6.8.0-124-generic/build`.
  - Kernel `checkpatch.pl --no-tree --strict --file` reports `0 errors, 0 warnings, 0 checks` for `ldd_input_demo.c`.
  - The example uses devm allocation/request patterns and does not require an explicit `.remove()` cleanup path for the registered learning-only input device on normal module removal.
- Remaining production/hardware caveats:
  - Runtime `insmod`, `/proc/bus/input/devices`, `evtest`, `libinput debug-events`, and `/dev/input/eventX` permission checks were not run because they require loading a kernel module and interacting with the host input subsystem.
  - No DTS file is shipped for the current example, so local `gpio-keys.yaml` schema validation and `dtc` checks are not applicable to this output.
  - Real GPIO IRQ, hardware debounce, wakeup, suspend/resume, and seat/session policy still need board-specific validation for production drivers.

## Learning Content Brief
- Learning path number: `26`.
- Slug: `input-device-drivers`.
- Topic scope:
  - Input core, `struct input_dev`, event types/codes, capability bitmaps, key/relative/absolute reporting, open/close lifecycle, IRQ-driven and polled input, GPIO buttons, `gpio-keys`, userspace evdev, debugging, cleanup/lifetime, and interview reasoning.
  - Keep raw GPIO mechanics in topics 13-14, generic IRQ internals in topic 15, bus mechanics in topics 16-17, PM/wakeup depth in topic 24, and IIO sensors in topic 25.
- Beginner mental model:
  - An input driver should not invent a private `/dev/my_button` ABI for keyboards, buttons, mice, touchscreens, and similar controls.
  - It reports standardized events to the input core; the input core and evdev turn those into `/dev/input/eventX` records that existing Linux userspace already understands.
  - A button event is not "read a GPIO file"; it is "detect state change, report key code plus press/release value, then end the frame with `input_sync()`".
- Core mechanism:
  - Probe obtains hardware resources, allocates `struct input_dev`, declares capabilities, registers with input core, and arms IRQ/polling according to the hardware.
  - Hardware event path reads the state, reports one or more events, and calls `input_sync()`.
  - Evdev receives the frame and userspace reads `struct input_event` records from `/dev/input/eventX`.
  - Optional `open()`/`close()` callbacks start/stop hardware only while a handler is using the device.
- Important structs/APIs:
  - Core: `struct input_dev`, `struct input_event`, `struct input_absinfo`.
  - Allocation/lifetime: `input_allocate_device()`, `devm_input_allocate_device()`, `input_register_device()`, `input_unregister_device()`, `input_free_device()`.
  - Capability setup: `set_bit()`, `input_set_capability()`, `input_set_abs_params()`, `input_set_drvdata()`, `input_get_drvdata()`.
  - Reporting: `input_report_key()`, `input_report_rel()`, `input_report_abs()`, `input_event()`, `input_sync()`.
  - Polling/current API after validation: `input_setup_polling()`, `input_set_poll_interval()`.
  - GPIO/IRQ front end: `devm_gpiod_get()`, `gpiod_get_value_cansleep()`, `gpiod_set_debounce()`, `gpiod_to_irq()`, `devm_request_threaded_irq()`, `request_any_context_irq()`, `IRQF_TRIGGER_*`, `IRQF_ONESHOT`.
  - DT/ABI constants: `EV_KEY`, `EV_REL`, `EV_ABS`, `EV_SW`, `EV_SYN`, `SYN_REPORT`, `KEY_*`, `BTN_*`, `REL_*`, `ABS_*`, `BUS_*`.
- Lifecycle/data flow:
  - Probe: allocate private state; get GPIO/IRQ/regmap/bus resources; allocate input device; set parent/name/phys/bus ID; declare capabilities; configure absolute axes; set open/close and private data; register input device; request IRQ or setup polling.
  - Open: enable regulators/clocks/device mode, request/start IRQ or polling if done lazily, take runtime-PM reference if needed.
  - Event: IRQ thread or poll callback reads hardware using context-safe accessors, handles debounce/state changes, calls `input_report_*()`, then `input_sync()`.
  - Userspace: udev creates `/dev/input/eventX`; userspace uses `evtest`, `libinput`, or direct `read()`/`poll()` on `struct input_event`.
  - Close/remove: stop new events, synchronize/free IRQ or polling work, unregister input device, release resources, and avoid handlers using freed `input_dev`.
- Practical examples for later:
  - Learning-only platform GPIO button input driver using `devm_input_allocate_device()`, descriptor GPIO, `devm_request_threaded_irq()`, `input_report_key()`, and `input_sync()`.
  - DTS-only `gpio-keys` example showing when no custom driver is needed.
  - Optional polling example using current `input_setup_polling()` rather than legacy `input_polled_dev`.
  - Userspace test script or README commands using `evtest`, `udevadm info`, `/proc/bus/input/devices`, and `/proc/interrupts`.
- Common bugs:
  - Forgetting to set capability bits before registration, so events are dropped or invisible.
  - Reporting events but forgetting `input_sync()`.
  - Using raw GPIO userspace ABI for keyboard-like buttons instead of `gpio-keys`/input.
  - Doing sleeping I2C/SPI/GPIO-expander reads in hard IRQ context.
  - Ignoring active-low semantics and reporting inverted press/release values.
  - No debounce, causing duplicate press/release storms.
  - Requesting IRQ before input object/private state is fully initialized, or freeing input device before IRQ is stopped.
  - Double-freeing after `input_register_device()` failure/success due misunderstanding ownership.
  - Using legacy `input_polled_dev` on kernels where `linux/input-polldev.h` no longer exists.
  - Hardcoding `/dev/input/event0` in docs/tests instead of discovering the node by name/path.
  - Choosing `EV_ABS` without setting min/max/fuzz/flat, causing poor userspace behavior.
  - Confusing `KEY_*` keyboard codes with `BTN_*` button/pointer codes.
- Debugging notes:
  - Check `dmesg` for `input: <name> as ...`.
  - Inspect `/proc/bus/input/devices` for name, handlers, event node, and capability bits.
  - Use `udevadm info /dev/input/eventX` to identify path/properties.
  - Use `evtest /dev/input/eventX` to see event type/code/value and confirm press/release.
  - Use `cat /proc/interrupts | grep <driver>` to confirm IRQ firing.
  - Use GPIO debugfs or gpioinfo/gpioget carefully to verify raw line state and polarity.
  - If no event appears, check capability bits, IRQ trigger flags, active-low polarity, debounce, `input_sync()`, and whether userspace opened the right event node.
  - If the system wakes unexpectedly or fails to wake, check `wakeup-source`, IRQ wake enablement, `open()`/`close()` behavior, and suspend path.
- Production concerns:
  - Prefer existing generic drivers and bindings such as `gpio-keys`, `gpio-keys-polled`, matrix keypad, rotary encoder, HID, or touchscreen-specific drivers before writing custom code.
  - Keep userspace ABI standard: evdev for user input, not custom char devices or ad hoc sysfs.
  - Use devm where it matches the parent lifetime, but understand input-device registration ownership.
  - Define exact event semantics in the binding and driver: key code, event type, active polarity, debounce, autorepeat, wakeup behavior, and polling interval.
  - Use threaded IRQs or work when hardware access can sleep.
  - Make remove/suspend paths stop event production before freeing resources.
  - Validate with real userspace: evtest/libinput, udev properties, permissions, seat handling, suspend/resume, and repeated press/release stress.
- Interview angles:
  - Why does Linux have an input subsystem instead of every keyboard/button driver creating a custom char device?
  - What is `struct input_dev`, and what must be initialized before registration?
  - How do `evbit`, `keybit`, `relbit`, and `absbit` affect userspace visibility?
  - Why is `input_sync()` required?
  - Explain `EV_KEY` values `0`, `1`, and `2`.
  - Compare `EV_REL` and `EV_ABS`, and explain why absolute axes need min/max/fuzz/flat.
  - Explain the lifecycle of a GPIO button input driver from probe to `/dev/input/eventX`.
  - When should you use `gpio-keys` instead of writing a driver?
  - Why might a GPIO button IRQ need a threaded handler?
  - What changed around polled input APIs, and how would you handle legacy `input_polled_dev` examples?
  - How do you debug "button IRQ fires but evtest shows nothing"?
  - How do input open/close callbacks interact with power management and wakeup?
