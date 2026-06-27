# 14 - GPIO Controller Drivers And IRQ Integration Example

This is a **learning-only** GPIO controller and IRQ integration example. It does not include a fake kernel module because GPIO controller drivers are hardware-specific and current gpiolib irqchip APIs are version-sensitive. Instead, it gives a minimal realistic Device Tree integration example plus the commands and checks you use when bringing up a real IRQ-capable GPIO expander.

Do **not** apply the included overlay unchanged. Its compatible string and controller phandles are placeholders.

## Goal

Use this example to connect the runtime chain:

```text
gpio-expander-irq-demo.dts
  -> I2C GPIO expander node
  -> gpio-controller + #gpio-cells expose GPIO lines
  -> interrupt-controller + #interrupt-cells expose child IRQs
  -> parent interrupt line connects expander INT pin to a SoC IRQ/GPIO controller
  -> consumers use the expander through gpios and interrupts properties
  -> real expander driver registers struct gpio_chip plus irqchip support
  -> gpiolib and the IRQ core expose /dev/gpiochipN and Linux IRQs
```

The example demonstrates:

- a GPIO controller node that also acts as an interrupt controller;
- `gpio-line-names` for debug-friendly line inspection;
- a `gpio-leds` consumer using expander GPIO line 0;
- a `gpio-keys` consumer using expander GPIO line 8 and child IRQ 8;
- a sensor-like node using expander GPIOs for reset/enable and expander line 9 as an interrupt;
- the debug workflow for parent IRQs, child IRQs, GPIO line ownership, and interrupt storms.

## Kernel Version Assumptions

Build and test against the exact kernel running on the target board:

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

This example assumes a modern kernel with:

- gpiolib GPIO character devices such as `/dev/gpiochipN`;
- GPIO debugfs support when enabled: `/sys/kernel/debug/gpio`;
- standard Device Tree GPIO and IRQ bindings;
- an in-tree or board-provided GPIO expander driver that registers `struct gpio_chip`;
- irqchip support in the expander driver, preferably through current `gpio_chip.irq` / `struct gpio_irq_chip` style setup.

Version-sensitive notes:

- Older GPIO controller examples may use direct helper calls such as `gpiochip_irqchip_add_nested()` or old direct fields such as `irqdomain`.
- Current production drivers should be checked against the target kernel's `include/linux/gpio/driver.h` and nearby in-tree drivers.
- GPIO sysfs is legacy. Use GPIO character-device tools such as `gpiodetect` and `gpioinfo` for userspace inspection.

## Files

| File | Purpose |
| --- | --- |
| `gpio-expander-irq-demo.dts` | Learning-only overlay shape for an IRQ-capable I2C GPIO expander and example consumers. |
| `README.md` | Build, apply, test, debug, cleanup, ABI, and production notes. |

No `Makefile` is included because this example does not include kernel module C code.

## Build

Compile the DTS to a DT overlay blob for inspection:

```sh
dtc -@ -I dts -O dtb \
    -o gpio-expander-irq-demo.dtbo \
    gpio-expander-irq-demo.dts
```

The DTS uses numeric binding values so it can be compiled directly with `dtc`:

| Numeric value | Symbolic meaning |
| --- | --- |
| GPIO flag `0` | `GPIO_ACTIVE_HIGH` |
| GPIO flag `1` | `GPIO_ACTIVE_LOW` |
| IRQ flag `2` | `IRQ_TYPE_EDGE_FALLING` |
| IRQ flag `8` | `IRQ_TYPE_LEVEL_LOW` |
| input code `28` | `KEY_ENTER` |

Decompile it to confirm the generated structure:

```sh
dtc -I dtb -O dts \
    -o gpio-expander-irq-demo.decompiled.dts \
    gpio-expander-irq-demo.dtbo
```

Expected artifact:

```text
gpio-expander-irq-demo.dtbo
```

The DTS compiles as a standalone learning overlay, but it is not schema-valid production hardware description until you replace placeholders with real board labels, a real compatible string, and a binding that matches your actual expander.

## Load / Apply

Overlay loading is board and bootloader specific. Common paths include U-Boot overlay support, distro-specific `/boot` overlay configuration, or configfs overlays when enabled.

Example configfs flow on a target that supports it:

```sh
sudo mount -t configfs none /sys/kernel/config 2>/dev/null || true
sudo mkdir -p /sys/kernel/config/device-tree/overlays/gpio-expander-demo
sudo cp gpio-expander-irq-demo.dtbo \
    /sys/kernel/config/device-tree/overlays/gpio-expander-demo/dtbo
```

If your board does not support runtime overlays, merge the adapted nodes into the board DTS, rebuild the DTB, install it through your normal boot flow, and reboot.

Expected kernel log shape after a real driver binds:

```text
<i2c-bus> 0-0020: registered GPIOs <base> to <base+15>
gpiochip <name>: registered GPIO chip
gpiochip <name>: registered IRQ chip, parent IRQ=<n>
```

Exact logs depend on the real expander driver.

## Test

First confirm that the GPIO provider exists:

```sh
gpiodetect
gpioinfo | grep -A20 -i exp
cat /sys/kernel/debug/gpio 2>/dev/null | grep -A20 -i exp
```

Expected shape:

```text
gpiochipN [<expander label>] (16 lines)
    line 0: "EXP_IO0" ...
    line 8: "EXP_IRQ_BUTTON" ...
    line 9: "EXP_SENSOR_IRQ" ...
```

Then confirm interrupt registration:

```sh
cat /proc/interrupts | grep -Ei 'gpio|expander|button|sensor'
find /proc/irq -maxdepth 2 -type f -name smp_affinity 2>/dev/null | head
```

If the `gpio-keys` consumer is active and input tools are available:

```sh
cat /proc/bus/input/devices | grep -A8 -i expander
```

Trigger the physical button or signal connected to expander line 8. Expected observations:

- parent IRQ counter increments for the SoC line receiving the expander INT pin;
- child IRQ or input event activity appears for the `gpio-keys` device;
- no interrupt storm occurs after the line returns inactive.

## Debug Commands

Use these commands during bring-up:

```sh
dmesg | grep -Ei 'gpio|irq|expander|i2c'
gpiodetect
gpioinfo
cat /sys/kernel/debug/gpio 2>/dev/null
cat /proc/interrupts
find /sys/firmware/devicetree/base -name '*gpio*' -o -name '*expander*'
```

Inspect the live Device Tree node after applying a real overlay:

```sh
find /sys/firmware/devicetree/base -name compatible | grep -i expander
tr -d '\0' < /sys/firmware/devicetree/base/<path-to-expander>/compatible
hexdump -C /sys/firmware/devicetree/base/<path-to-expander>/interrupts
```

Enable dynamic debug for a real driver when available:

```sh
sudo sh -c 'echo "file drivers/gpio/* +p" > /sys/kernel/debug/dynamic_debug/control'
dmesg -w
```

Turn it back down:

```sh
sudo sh -c 'echo "file drivers/gpio/* -p" > /sys/kernel/debug/dynamic_debug/control'
```

## Expected Logs And Symptoms

Successful bring-up usually shows:

```text
i2c <bus>-0020: probing GPIO expander
gpiochip_add_data: registered GPIO chip
request_threaded_irq: parent IRQ <n>
```

If the provider registers but child IRQs do not work:

```text
gpioinfo shows the expander lines
/proc/interrupts parent counter may increment
child device handler does not run
```

Likely causes:

- missing `interrupt-controller` or wrong `#interrupt-cells`;
- driver did not register GPIO irqchip support;
- wrong child `interrupt-parent`;
- child hwirq offset does not match the expander pin;
- valid mask excludes that line.

If the IRQ counter races upward:

```text
/proc/interrupts count increases continuously
system log may show repeated handler messages
```

Likely causes:

- wrong level/edge trigger;
- device status bit not cleared;
- wrong status clear order;
- child IRQ unmasked while parent line is still asserted;
- physical INT line polarity does not match `IRQ_TYPE_*`.

If the kernel warns about sleeping in atomic context:

```text
BUG: sleeping function called from invalid context
```

Likely cause:

- the driver treated an I2C/SPI expander like a chained MMIO GPIO controller. A sleepable expander should use a threaded parent IRQ and nested child IRQ dispatch.

## Cleanup

Remove the runtime overlay if your board used configfs:

```sh
sudo rmdir /sys/kernel/config/device-tree/overlays/gpio-expander-demo
```

If the overlay created devices, expected cleanup effects are:

- consumer devices unbind first or lose their provider;
- the expander device unbinds from its I2C driver;
- the GPIO chip disappears from `gpiodetect` / `gpioinfo`;
- associated IRQ entries disappear or stop changing.

Remove local build artifacts:

```sh
rm -f gpio-expander-irq-demo.dtbo gpio-expander-irq-demo.decompiled.dts
```

## Error Path Explanation

Real GPIO expander drivers should handle failures in this order:

```text
1. allocate private data
2. initialize bus/regmap access
3. initialize locks and register caches
4. fill struct gpio_chip
5. configure gpio_chip.irq / irqchip data if parent IRQ exists
6. register with devm_gpiochip_add_data()
7. let devm-managed resources unwind automatically on probe failure
```

Important error-path rules:

- Preserve `-EPROBE_DEFER` when GPIO, IRQ, regulator, clock, or pinctrl providers are not ready.
- Do not leave parent IRQs enabled if the GPIO chip registration fails.
- Do not expose child IRQs for lines that hardware cannot interrupt.
- Put output GPIOs into a safe logical state on remove when the driver actively drove board signals.
- For bus expanders, protect cached register state with a mutex and keep bus I/O out of hard IRQ context.

## Userspace ABI Impact

There is **no custom userspace ABI** in this example.

The normal kernel-visible effects are:

- a GPIO character device such as `/dev/gpiochipN`;
- line metadata visible through `gpiodetect` and `gpioinfo`;
- optional debugfs visibility through `/sys/kernel/debug/gpio`;
- interrupt counters in `/proc/interrupts`;
- input events if a real `gpio-keys` consumer binds.

Do not design a new product ABI around legacy `/sys/class/gpio`. For production userspace GPIO access, prefer the GPIO character-device ABI and libgpiod tools.

## Why This Is Not Production-Ready

This example is **learning-only**, not production-ready, because:

- the DTS uses placeholder phandles and compatible strings;
- it does not include a real chip binding or schema validation;
- real expander registers, interrupt status clear rules, and valid IRQ masks are chip-specific;
- production code must match the exact kernel's `struct gpio_irq_chip` API;
- production board files must document electrical polarity, interrupt trigger type, debounce, wakeup, and line names accurately.

Production work would add:

- a real binding YAML or reference to an existing binding;
- a real in-tree GPIO expander driver or a carefully reviewed new one;
- schema-validated DTS;
- tested suspend/resume and wakeup behavior;
- hardware-specific interrupt mask/unmask/ack/type handling;
- board-level validation with an oscilloscope or logic analyzer for the INT line.
