# Topic Brief - 13 - Pin Control And GPIO Consumer APIs

## Output Targets
- Knowledge: `knowledge/13-pin-control-and-gpio-consumer-apis.md`
- Interview: `interview/13-pin-control-and-gpio-consumer-apis.md`
- Example: `examples/13-pin-control-and-gpio-consumer-apis/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch06` | `docs/Linux Device Driver Development/Chapter 6-The Concept of a Device Tree .md` | read/mapped/merged | Supporting source for labels/phandles and GPIO specifier examples in Device Tree. Full DT fundamentals remain covered by topics 10 and 11. |
| `ldd1-ch14` | `docs/Linux Device Driver Development/Chapter 14-Pin Control and GPIO Subsystem.md` | read/mapped/covered/merged | Primary book source for pinmux/pinconf, `pinctrl-names`/`pinctrl-N`, pinctrl consumer APIs, default/init state behavior, legacy integer GPIO, descriptor GPIO, active-low mapping, `_cansleep` accessors, GPIO-to-IRQ mapping, DT GPIO naming, and sysfs export. |
| `ldd2-ch01` | `docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md` | read/mapped/merged | Adds the GPIO IRQ context rule: a GPIO IRQ may come from an MMIO controller or a sleeping I2C/SPI expander, so `request_any_context_irq()` can avoid assuming hard-IRQ context. |
| `ldd2-ch02` | `docs/Linux Device Driver Development 2/Chapter 2-Regmap_API.md` | read/mapped/deferred | Confirms how `gpiod_to_irq()` relies on GPIO-controller IRQ integration. Detailed `gpio_chip`/irqchip implementation belongs to topic 14 and topic 15. |
| `ldd2-ch04` | `docs/Linux Device Driver Development 2/Chapter 4-Common_Clock_Framework.md` | read/mapped/merged | Provides real DT consumer examples combining `pinctrl-names`, `pinctrl-0`, `reset-gpios`, `powerdown-gpios`, and GPIO-backed clock/power-sequence helpers. |
| `ldd2-ch08` | `docs/Linux Device Driver Development 2/Chapter 8-Integrat with V4L2.md` | read/mapped/deferred | Shows GPIO availability as one cause of probe deferral in a real subsystem. Detailed V4L2 async behavior belongs to topics 32-34. |
| `ldd2-ch13` | `docs/Linux Device Driver Development 2/Chapter 13-Watchdog_Device_Drivers.md` | read/mapped/merged | Adds a framework-consumer example: use the kernel `gpio-wdt` driver and watchdog ABI instead of bit-banging GPIOs from userspace. Detailed watchdog coverage belongs to topic 28. |
| `notion-ch06-part3` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 3 OF APIs and Platform Integration.md` | read/mapped/merged | Supporting source for `reset-gpios`/`enable-gpios`, `of_parse_phandle_with_args()`, legacy `of_get_named_gpio()`, `of_gpio_named_count()`, and DT property-name pitfalls. |
| `notion-ch14-part1` | `docs/Linux-Device-Driver-Notion/Chapter 14-Part 1 Pin Control Framework.md` | read/mapped/covered/merged | Primary Notion source for beginner pinctrl mental model, pinmux hardware, pinconf electrical settings, controller/consumer DT nodes, default/sleep/idle states, devm pinctrl APIs, and suspend/resume state switching. |
| `notion-ch14-part2` | `docs/Linux-Device-Driver-Notion/Chapter 14-Part 2 GPIO Consumer Interfaces.md` | read/mapped/covered/merged | Primary Notion source for GPIO consumer operations, legacy versus descriptor APIs, `devm_gpiod_get*()`, optional GPIOs, active-low logical values, raw access, debounce, `gpiod_to_irq()`, and cleanup patterns. |
| `notion-ch14-part3` | `docs/Linux-Device-Driver-Notion/Chapter 14-Part 3 Userspace GPIO Access.md` | read/mapped/deferred/merged | Read as an apparent overlap with `ldd1-ch14` sysfs content. Keep only current-ABI caveats and kernel-export context; userspace GPIO details belong to topic 08 or a later example note. |

## Source Files Read
- `ldd1-ch06`: relevant phandle/GPIO example around labels and `gpios = <&gpio1 ...>`.
- `ldd1-ch14`: full file read.
  - Relevant sections: `The pin control subsystem`, `Pinctrl and the device tree`, pinctrl consumer API examples, automatic default/init state behavior, `The GPIO subsystem`, legacy integer API, descriptor GPIO API, GPIO DT mappings, GPIO-to-IRQ mapping, sysfs GPIO, kernel export helpers.
- `ldd2-ch01`: relevant IRQ-context section read.
  - Relevant section: `request_any_context_irq()` example using `gpiod_get()`, `gpiod_to_irq()`, and GPIO-backed IRQ lines that may or may not sleep.
- `ldd2-ch02`: relevant `irqchip and gpiolib API - new generation` excerpt read.
  - Used only to explain that `gpiod_to_irq()` is backed by GPIO-controller IRQ-domain support; implementation is deferred.
- `ldd2-ch04`: relevant DT examples read.
  - Relevant sections: camera `pinctrl-0`, `reset-gpios`, `powerdown-gpios`; GPIO gate clock and `mmc-pwrseq-simple` examples using `enable-gpios`/`reset-gpios`.
- `ldd2-ch08`: relevant V4L2 async probe-defer excerpt read.
  - Used only as a production caveat that GPIO dependencies can trigger `-EPROBE_DEFER`.
- `ldd2-ch13`: relevant `GPIO-based watchdogs` section read.
  - Used as a framework-selection warning: prefer existing subsystem drivers over userspace GPIO bit-banging.
- `notion-ch06-part3`: relevant sections read.
  - Relevant sections: `Phandle Parsing APIs`, `GPIO OF APIs`, complete real-world driver reset GPIO example, `Wrong property names` pitfall.
- `notion-ch14-part1`: full file read.
  - Relevant sections: pin fundamentals, pinmux, pinconf, pin controller, Linux pinctrl architecture, DT node structure, controller/configuration/consumer nodes, BeagleBone LCD pinctrl/GPIO example, pinctrl consumer APIs, resource-managed versions, automatic default state selection, sleep/default suspend/resume example.
- `notion-ch14-part2`: full file read.
  - Relevant sections: GPIO subsystem overview, legacy integer interface, descriptor-based interface, DT mappings, initialization flags, resource-managed APIs, conversion between interfaces, polarity handling, raw access, best practices.
- `notion-ch14-part3`: full file read.
  - Relevant sections: sysfs GPIO deprecation, `/sys/class/gpio` layout, poll/edge behavior, kernel export functions, libgpiod/chardev overview.

## Merged Source Notes
- Use `ldd1-ch14` as the canonical book spine because it combines pinctrl and GPIO consumers in one chapter and explicitly distinguishes legacy integer GPIO from descriptor GPIO.
- Use `notion-ch14-part1` to improve the mental model: a physical SoC pin has one active mux function at a time, while pin configuration controls electrical behavior such as bias, drive strength, slew rate, open-drain, Schmitt trigger, and debounce.
- Use `notion-ch14-part2` to modernize the GPIO consumer section with `devm_gpiod_get*()`, optional GPIOs, raw versus logical values, and clearer active-low examples.
- Keep legacy integer APIs from `ldd1-ch14` and `notion-ch14-part2` as migration/reading-old-code material, not the recommended path for new drivers.
- Merge `ldd1-ch14` and Notion pinctrl state material into this rule: most simple drivers rely on automatic `default` selection, while drivers with power states or runtime mux changes explicitly look up and select named states.
- Merge `ldd2-ch01` with GPIO IRQ content: consumer code that turns a GPIO descriptor into an IRQ must respect whether the provider can sleep; threaded or any-context IRQ handling is often safer when the GPIO provider may be an expander.
- Merge `ldd2-ch04` and `ldd2-ch13` as practical examples that GPIO consumers are often hidden behind existing frameworks (`gpio-gate-clock`, `mmc-pwrseq-simple`, `gpio-wdt`, camera reset/powerdown GPIOs). A production driver should prefer framework bindings/drivers when they fit.
- Treat `notion-ch14-part3` as an apparent duplicate/extension of `ldd1-ch14` sysfs material. It adds libgpiod/chardev context, but current upstream docs show chardev v2 as the current userspace ABI; detailed userspace GPIO should not dominate topic 13.

## Source Differences
- `ldd1-ch14` says descriptor GPIO is the "new and recommended" API; Notion repeats this and adds `devm_gpiod_get*()`/optional variants. Final learner docs should recommend descriptor and managed APIs for new kernel drivers.
- `ldd1-ch14` includes several typo-like or stale snippets:
  - `pinctrl-name` should be presented as `pinctrl-names`.
  - `gpiog_get()` is clearly intended to be `gpiod_get()`.
  - Example descriptor code requests `btn1`/`btn2` using `"led"` instead of `"btn"`; do not reproduce this bug in learner docs.
  - Some snippets use `struct device_node *np = &pdev->dev.of_node`, which should be treated cautiously; final code should use current idioms.
- `ldd1-ch14` and `notion-ch06-part3` teach `of_get_named_gpio()` and integer GPIO. Current docs should frame this as legacy support, with descriptor APIs preferred.
- `ldd1-ch14` covers GPIO sysfs as normal functionality; Notion marks it deprecated. External validation confirms sysfs GPIO is obsolete and maintained only for old users.
- `notion-ch14-part3` presents libgpiod APIs based on older `struct gpiod_line` style. External validation confirms the kernel chardev v2 ABI is current; any final userspace note should avoid hard-coding stale libgpiod version-specific APIs unless version-qualified.
- `ldd1-ch14` says the pinctrl core selects `init` before probe and switches to `default` after probe. External kernel docs confirm the standard states but add that `sleep`/`idle` are selected through PM helpers, which should be reflected in final content.
- `ldd2-ch02` explains GPIO irqchip internals and `gpiod_to_irq()` implementation. This is useful background but belongs mainly to topic 14 and topic 15, not this consumer-focused chapter.

## Gaps / Uncertainties
- Need current-kernel validation before writing final code examples for exact prototypes and return semantics of:
  - `devm_pinctrl_get_select_default()` and related pinctrl helpers;
  - `devm_gpiod_get_optional()` returning `NULL` versus `ERR_PTR()` cases;
  - `gpiod_set_debounce()` units and provider support behavior;
  - `gpiod_get_value()` versus `gpiod_get_value_cansleep()` warnings and context restrictions.
- Need decide how much userspace GPIO belongs in topic 13 versus topic 08. The target scope mentions GPIO consumer APIs, but the learning path for topic 08 already owns userspace ABI design.
- Need external binding examples for modern GPIO property naming, active-low flags, open-drain/source flags, and whether singular `-gpio` should be described as allowed but discouraged compared with `-gpios`.
- Need avoid over-teaching pin controller driver implementation. Provider-side pinctrl and `gpio_chip` implementation should be separated from this consumer topic.
- Need an example target later that likely combines:
  - a platform driver using `devm_gpiod_get_optional()` for reset/enable GPIOs;
  - `pinctrl-names = "default", "sleep"` with suspend/resume state selection;
  - a DTS snippet with `reset-gpios = <... GPIO_ACTIVE_LOW>`.

## External Validation
- Used: https://docs.kernel.org/driver-api/pin-control.html
  - Validates standard pinctrl states: `default`, `init`, `sleep`, and `idle`; `default` is selected before probe, `init` can be selected before probe and `default` after probe, and PM helpers select `sleep`/`idle`/`default` states for power management.
- Used: https://docs.kernel.org/driver-api/gpio/consumer.html
  - Validates GPIO descriptor consumer API direction/value semantics, logical value behavior, active-low handling, raw accessors, `_cansleep` variants, `gpiod_is_active_low()`, and moderation around raw physical-level handling.
- Used: https://docs.kernel.org/next/admin-guide/gpio/sysfs.html
  - Validates that the sysfs GPIO ABI is deprecated/obsolete and should not be used by new userspace consumers.
- Used: https://docs.kernel.org/userspace-api/gpio/chardev.html
  - Validates that the GPIO character device userspace API v2 is the current userspace ABI and that userspace should not bit-bang hardware that has a proper kernel driver/subsystem.
- Used: https://docs.kernel.org/userspace-api/gpio/chardev_v1.html
  - Validates that GPIO chardev v1 is itself obsolete, so final docs should avoid presenting old libgpiod/chardev assumptions as timeless.

## Learning Content Brief
- Mental model:
  - A SoC pin is a physical pad. Pinctrl decides what function that pad performs and what electrical characteristics it has.
  - GPIO is one possible function of a pin: a simple digital line that can be input, output, or sometimes an interrupt source.
  - Pinctrl answers "is this pad routed/configured correctly for my device?" GPIO consumer APIs answer "how does my driver own and use this logical line?"
- Core mechanism:
  - Board DTS references pin configuration nodes through `pinctrl-names` and `pinctrl-N`.
  - Pin configuration nodes are controller-specific and usually live under the SoC pin controller node.
  - A consumer device references GPIO lines with properties such as `reset-gpios`, `enable-gpios`, `led-gpios`, or `btn-gpios`.
  - The GPIO core maps those firmware descriptions to `struct gpio_desc` descriptors for the requesting device.
- Important structs/APIs:
  - Pinctrl: `struct pinctrl`, `struct pinctrl_state`, `pinctrl_get()`, `devm_pinctrl_get()`, `pinctrl_lookup_state()`, `pinctrl_select_state()`, `pinctrl_put()`, `pinctrl_get_select()`, `pinctrl_get_select_default()`, `devm_pinctrl_get_select_default()`.
  - GPIO descriptor consumer: `struct gpio_desc`, `gpiod_get()`, `gpiod_get_index()`, `gpiod_get_optional()`, `devm_gpiod_get()`, `devm_gpiod_get_index()`, `devm_gpiod_get_optional()`, `gpiod_put()`.
  - GPIO direction/value: `GPIOD_ASIS`, `GPIOD_IN`, `GPIOD_OUT_LOW`, `GPIOD_OUT_HIGH`, open-drain variants, `gpiod_direction_input()`, `gpiod_direction_output()`, `gpiod_get_value()`, `gpiod_set_value()`, `gpiod_get_value_cansleep()`, `gpiod_set_value_cansleep()`, raw variants.
  - Other GPIO helpers: `gpiod_cansleep()`, `gpiod_set_debounce()`, `gpiod_is_active_low()`, `gpiod_to_irq()`, `gpiod_get_direction()`.
  - Legacy only: `gpio_request()`, `gpio_free()`, `gpio_direction_input()`, `gpio_direction_output()`, `gpio_get_value()`, `gpio_set_value()`, `gpio_to_irq()`, `of_get_named_gpio()`, `devm_gpio_request_one()`.
- Lifecycle/data flow:
  - DTS declares pin states and GPIO consumer properties.
  - Device core binds standard pinctrl states around probe when present.
  - Probe allocates driver private data and obtains GPIO descriptors, preferably with `devm_*`.
  - Direction and initial output value should be set atomically at request time when possible.
  - Runtime reads/writes use logical descriptor accessors, choosing `_cansleep` variants when the provider may sleep.
  - Suspend/resume can select `sleep` and `default` pin states through PM-aware paths.
  - Remove/error paths release unmanaged GPIO descriptors, IRQs, and pinctrl handles; managed resources simplify this.
- Examples to build later:
  - DTS: `pinctrl-names = "default", "sleep";`, `pinctrl-0 = <&dev_default_pins>;`, `pinctrl-1 = <&dev_sleep_pins>;`, `reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;`, `enable-gpios = <&gpio1 8 GPIO_ACTIVE_HIGH>;`.
  - Driver: platform `probe()` with `devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH)`, reset pulse using logical values, optional enable GPIO, and `gpiod_to_irq()` for an input line if appropriate.
  - PM: look up sleep/default pinctrl states once and select them during suspend/resume if explicit control is needed.
- Common bugs:
  - Configuring GPIO direction/value before the pad is muxed to GPIO.
  - Mixing physical voltage thinking with logical GPIO values on `GPIO_ACTIVE_LOW` lines.
  - Using raw GPIO accessors when logical access is correct.
  - Calling non-`_cansleep` GPIO accessors on an I2C/SPI expander path from atomic context.
  - Forgetting that `gpiod_get(dev, "reset", ...)` looks for `reset-gpios`/`reset-gpio`.
  - Copying stale legacy examples with wrong property names or mismatched `con_id`.
  - Ignoring `-EPROBE_DEFER` from GPIO/pinctrl providers.
  - Assuming `gpiod_to_irq()` makes every GPIO usable as an IRQ.
  - Reimplementing a GPIO-driven function that already has a subsystem driver or binding.
- Debugging notes:
  - Check kernel logs for pinctrl lookup/select failures and deferred probes.
  - Use `/sys/kernel/debug/gpio` to see requested lines, labels, directions, and consumers when debugfs is available.
  - Use pinctrl debugfs entries when enabled to inspect pin mux ownership and selected states.
  - Check the runtime DT under `/sys/firmware/devicetree/base` to confirm property names and flags.
  - For userspace inspection, prefer GPIO chardev tools such as `gpioinfo`/`gpiodetect` over old sysfs workflows when available.
- Production concerns:
  - Prefer descriptor GPIO APIs and `devm_*` managed allocation in new drivers.
  - Keep pinctrl data in DTS/bindings, not hard-coded register pokes in consumer drivers.
  - Treat active-low and open-drain as hardware description; let the descriptor API handle logical values.
  - Prefer existing kernel frameworks (`gpio-leds`, `gpio-keys`, `gpio-wdt`, `mmc-pwrseq-simple`, `gpio-gate-clock`) over custom GPIO manipulation when they model the hardware.
  - Keep userspace GPIO for board testing, manufacturing tools, and simple external control, not for devices that need proper kernel subsystem integration.
- Interview angles:
  - Explain pinmux versus pinconf versus GPIO.
  - Explain why pinctrl and GPIO are separate subsystems even though GPIO often uses pins.
  - Explain `pinctrl-names`/`pinctrl-0` and default/sleep state behavior.
  - Explain descriptor GPIO lookup from `reset-gpios` using `gpiod_get(dev, "reset", ...)`.
  - Explain logical versus raw GPIO values for active-low lines.
  - Explain when to use `_cansleep` accessors.
  - Explain how to turn a GPIO input into an IRQ and what can go wrong.
  - Explain why integer GPIO numbers and sysfs GPIO are legacy.
  - Explain how you would debug a reset GPIO that appears inverted or never toggles.
  - Explain why a senior driver should choose a framework binding instead of bit-banging GPIOs.
