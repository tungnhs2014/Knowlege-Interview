# 13 - Pin Control And GPIO Consumer APIs Example

This is a **learning-only** pinctrl and GPIO consumer example. It is minimal but realistic on the driver side: it uses descriptor GPIO APIs, optional GPIOs, logical active-low values, GPIO-to-IRQ mapping, a threaded IRQ handler, and explicit pinctrl `default`/`sleep` state handling.

Do **not** boot the included DTS on a real board as-is. Its GPIO and pinctrl providers are placeholders.

## Goal

Use this example to connect the runtime chain:

```text
pinctrl-gpio-consumer-demo.dts
  -> compatible = "acme,pinctrl-gpio-consumer-demo"
  -> reset-gpios / enable-gpios / event-gpios describe board wiring
  -> pinctrl-names describes default and sleep pin states
  -> platform bus creates a device when a real DTB contains that enabled node
  -> pinctrl_gpio_consumer_demo.ko matches through .of_match_table
  -> probe() requests GPIO descriptors and maps the event GPIO to an IRQ
```

The code demonstrates:

- `devm_gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH)`;
- logical active-low reset handling;
- `devm_gpiod_get_optional(dev, "enable", GPIOD_OUT_LOW)`;
- `devm_gpiod_get_optional(dev, "event", GPIOD_IN)`;
- `gpiod_to_irq()` plus `devm_request_threaded_irq()`;
- `_cansleep` GPIO access from threaded/process context;
- `devm_pinctrl_get()`, `pinctrl_lookup_state()`, and `pinctrl_select_state()`;
- simple suspend/resume pin state switching;
- safe remove behavior for externally visible output GPIOs;
- `dev_err_probe()` to preserve meaningful errors such as `-EPROBE_DEFER`.

## Kernel Version Assumptions

Build the module against the headers for the exact kernel you will load it into:

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

The example assumes headers with:

- descriptor GPIO consumer APIs in `<linux/gpio/consumer.h>`;
- `devm_gpiod_get_optional()`;
- `devm_pinctrl_get()`;
- `struct platform_driver.remove_new`;
- `dev_err_probe()`.

Older kernels may need small edits:

- use `.remove` returning `int` instead of `.remove_new`;
- replace `dev_err_probe()` with explicit logging and `return ret`;
- check whether your kernel's GPIO and pinctrl helpers have the exact same prototypes.

## Files

| File | Purpose |
| --- | --- |
| `pinctrl_gpio_consumer_demo.c` | Learning-only DT-backed platform driver using pinctrl and GPIO consumer APIs. |
| `pinctrl-gpio-consumer-demo.dts` | Standalone DTS snippet showing the node shape the driver expects. |
| `Makefile` | Builds `pinctrl_gpio_consumer_demo.ko` and cleans module plus DTS artifacts. |
| `README.md` | Build, load, test, debug, cleanup, and production notes. |

No userspace test program is included because the driver creates no `/dev` node and no custom sysfs, procfs, debugfs, or ioctl interface.

## Build

From this directory:

```sh
make
```

Expected build artifact:

```text
pinctrl_gpio_consumer_demo.ko
```

For a cross-compiled target kernel:

```sh
make KDIR=/path/to/target/kernel/build \
     ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

Compile the standalone DTS only for inspection:

```sh
dtc -I dts -O dtb -o pinctrl-gpio-consumer-demo.dtb pinctrl-gpio-consumer-demo.dts
dtc -I dtb -O dts -o pinctrl-gpio-consumer-demo.decompiled.dts pinctrl-gpio-consumer-demo.dtb
```

The DTS may produce warnings because its providers are fake. That is acceptable for this learning snippet; production DTS files must use real SoC bindings and pass schema validation.

## Load And Inspect

On a normal development machine, loading the module only registers the driver. `probe()` will not run unless the live Device Tree already contains a matching enabled node.

```sh
sudo insmod ./pinctrl_gpio_consumer_demo.ko
dmesg | tail -20
modinfo -F alias ./pinctrl_gpio_consumer_demo.ko
ls /sys/bus/platform/drivers/pinctrl-gpio-consumer-demo
```

Expected alias shape:

```text
of:N*T*Cacme,pinctrl-gpio-consumer-demo
of:N*T*Cacme,pinctrl-gpio-consumer-demoC*
```

Expected behavior without a matching live DT node:

- the module loads successfully;
- `/sys/bus/platform/drivers/pinctrl-gpio-consumer-demo/` exists;
- no `probe complete` log appears.

On a real target with a real, enabled node and real providers, expected probe log shape:

```text
pinctrl-gpio-consumer-demo <device>: enable GPIO asserted logically
pinctrl-gpio-consumer-demo <device>: reset GPIO active_low=1
pinctrl-gpio-consumer-demo <device>: reset pulse complete using logical values
pinctrl-gpio-consumer-demo <device>: event GPIO mapped to IRQ <n>
pinctrl-gpio-consumer-demo <device>: probe complete
```

If `reset-gpios`, `enable-gpios`, or `event-gpios` are absent, the driver treats them as optional and continues. That keeps the example flexible, but production bindings should clearly state which lines are required.

## Test On A Real DT Target

Adapt the DTS snippet into your board DTS or overlay:

```dts
pinctrl_gpio_consumer_demo {
	compatible = "acme,pinctrl-gpio-consumer-demo";

	pinctrl-names = "default", "sleep";
	pinctrl-0 = <&demo_default_pins>;
	pinctrl-1 = <&demo_sleep_pins>;

	reset-gpios = <&gpio1 7 GPIO_ACTIVE_LOW>;
	enable-gpios = <&gpio1 8 GPIO_ACTIVE_HIGH>;
	event-gpios = <&gpio2 3 GPIO_ACTIVE_LOW>;
};
```

Then boot the updated DTB, load the module, and inspect:

```sh
sudo insmod ./pinctrl_gpio_consumer_demo.ko
dmesg | grep -i pinctrl-gpio-consumer
find /sys/bus/platform/devices -maxdepth 1 -name '*pinctrl*' -print
readlink /sys/bus/platform/devices/*pinctrl*/driver 2>/dev/null
```

Trigger the `event-gpios` input if it is physically wired to a button or signal source. Expected IRQ log shape:

```text
pinctrl-gpio-consumer-demo <device>: irq: event GPIO logical value=1
```

The exact value depends on polarity, trigger edge, and the signal state when the threaded handler reads the line.

## Runtime Debug Commands

Useful commands on a target:

```sh
dmesg | grep -i pinctrl-gpio-consumer
modinfo ./pinctrl_gpio_consumer_demo.ko
modinfo -F alias ./pinctrl_gpio_consumer_demo.ko
find /sys/bus/platform/drivers/pinctrl-gpio-consumer-demo -maxdepth 1 -print
find /sys/bus/platform/devices -maxdepth 1 -name '*pinctrl*' -print
cat /sys/kernel/debug/gpio 2>/dev/null
find /sys/kernel/debug/pinctrl -maxdepth 2 -type f 2>/dev/null
```

Inspect the live Device Tree:

```sh
find /sys/firmware/devicetree/base -name compatible | grep -i pinctrl-gpio
tr -d '\0' < /sys/firmware/devicetree/base/pinctrl_gpio_consumer_demo/compatible
hexdump -C /sys/firmware/devicetree/base/pinctrl_gpio_consumer_demo/reset-gpios
```

If `probe()` does not run:

- check the node exists in the running DTB, not only in source;
- check the `compatible` spelling;
- check the node is under a bus/root location that creates a platform device;
- check `modinfo -F alias` includes the OF alias;
- check whether the driver is built into the kernel instead of loaded as a module.

If `probe()` defers or fails:

- check GPIO provider nodes are enabled and have registered;
- check pinctrl provider nodes and labels are correct;
- check property names: driver `con_id` `"reset"` expects `reset-gpios`;
- check whether `event-gpios` can actually map to an IRQ;
- check logs from `dev_err_probe()`.

If the signal looks inverted:

- check `GPIO_ACTIVE_LOW` versus `GPIO_ACTIVE_HIGH`;
- remember `gpiod_set_value(desc, 1)` means logical active, not always physical high;
- avoid raw GPIO accessors unless you really need physical voltage control.

If the IRQ path warns about sleeping:

- keep GPIO reads in the threaded handler;
- use `_cansleep` accessors for GPIOs that may be behind I2C/SPI expanders.

## Userspace ABI Impact

There is **no custom userspace ABI impact**.

This example does not create:

- a character device or `/dev` node;
- sysfs attributes;
- procfs files;
- debugfs files;
- ioctl commands;
- netlink or other userspace protocol.

Userspace-visible effects are limited to normal kernel observation surfaces:

- module information from `modinfo`;
- kernel logs in `dmesg`;
- driver/device binding state under `/sys/bus/platform/`;
- GPIO debug information under `/sys/kernel/debug/gpio` when debugfs is mounted;
- pinctrl debug information under `/sys/kernel/debug/pinctrl` when enabled;
- live DT contents under `/sys/firmware/devicetree/base` on DT systems.

Do not use old GPIO sysfs as a new product ABI. For userspace GPIO inspection or board-test tools, prefer the GPIO character-device tools available on your system.

## Cleanup

Unload the module:

```sh
sudo rmmod pinctrl_gpio_consumer_demo
dmesg | tail -20
```

Expected unload log on a probed device:

```text
pinctrl-gpio-consumer-demo <device>: remove: outputs placed in safe logical state
```

Clean local build artifacts:

```sh
make clean
```

If you compiled the DTS manually, `make clean` also removes:

```text
pinctrl-gpio-consumer-demo.dtb
pinctrl-gpio-consumer-demo.decompiled.dts
```

## Cleanup And Error Paths

The driver uses managed cleanup for resources whose lifetime is tied to the device:

```text
devm_kzalloc()
devm_pinctrl_get()
devm_gpiod_get_optional()
devm_request_threaded_irq()
```

That means:

- if a later probe step fails, requested GPIO descriptors and IRQs are released automatically;
- if `remove()` runs, devm resources are released after remove returns;
- `remove()` still places output GPIOs into a safe logical state before descriptors are released.

Important error-path behavior:

- `devm_gpiod_get_optional()` returning `NULL` means the optional GPIO is absent;
- `ERR_PTR(...)` means a real error and is returned with `dev_err_probe()`;
- `gpiod_to_irq()` may fail because not every GPIO line supports IRQ mapping;
- pinctrl `sleep` state absence is allowed in this learning example, but production power-management behavior should be explicit.

## Why This Is Not Production-Ready

This example is **learning-only** because:

- the DTS providers are fake placeholders;
- there is no YAML binding;
- reset, enable, event, and sleep behavior are generic examples, not a real hardware contract;
- IRQ trigger type is hard-coded as falling-edge for demonstration;
- there is no regulator, clock, reset-controller, runtime PM, wakeup-source, or subsystem registration;
- optional GPIO policy is deliberately relaxed for learning.

Production code would add:

- a real binding schema and `dtbs_check`;
- real SoC pinctrl syntax and board-specific pin states;
- clear required-versus-optional GPIO rules;
- datasheet-correct reset/enable timing;
- correct IRQ trigger and wakeup behavior;
- runtime PM and system sleep ordering;
- subsystem integration, such as input, LED, watchdog, IIO, V4L2, or another proper framework when appropriate;
- hardware-specific safety behavior for remove and shutdown.
