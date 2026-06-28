# 19 - Regmap IRQ And MFD/Syscon Example

This is a **learning-only** example. It does not include a kernel module and is not production-ready. It uses a small DTS file plus command-line inspection to show how a PMIC-like MFD parent, regmap IRQ child resources, and a syscon/`simple-mfd` register block are represented.

Do **not** boot this DTS on real hardware. The compatible strings are training-only, the MMIO addresses are fake, and no real driver is expected to bind.

## Goal

Use this example to connect the design chain:

```text
PMIC-like I2C node
  -> parent owns chip-wide regmap and parent IRQ
  -> parent would register a regmap IRQ chip
  -> parent would create MFD child platform devices
  -> child nodes represent regulator, RTC, and power-key functions

syscon/simple-mfd node
  -> shared MMIO register block
  -> child nodes represent simple consumers
  -> consumers would use syscon phandle/regmap access
```

The example demonstrates:

- a parent I2C device that would be handled by an MFD core driver;
- a parent interrupt line from a GPIO interrupt controller;
- a PMIC node marked as an interrupt controller for child interrupt indexes;
- child nodes that would become child platform devices in a real MFD design;
- a `"syscon", "simple-mfd"` style MMIO block with child nodes;
- why this topic is mostly about object relationships and binding shape, not a tiny standalone module.

## Kernel Version Assumptions

No kernel module is built in this directory.

The concepts assume a Linux 6.x style kernel with:

- regmap IRQ helpers such as `devm_regmap_add_irq_chip()` and `regmap_irq_get_domain()`;
- MFD helpers such as `devm_mfd_add_devices()`;
- syscon helpers such as `syscon_regmap_lookup_by_phandle()`;
- Device Tree compilation with `dtc`.

Regmap IRQ structures and MFD/syscon helper details change across kernel versions. Check the target kernel headers before turning the pseudo-code below into real code.

## Files

| File | Purpose |
| --- | --- |
| `regmap-irq-mfd-syscon-demo.dts` | Learning-only DTS showing a fake PMIC MFD parent and a fake syscon/`simple-mfd` block. |
| `README.md` | Commands, expected output, debug workflow, ABI notes, cleanup, and production caveats. |

There is no `Makefile` because this example intentionally does not include kernel module code.

## Userspace ABI Impact

This example creates **no userspace ABI** by itself.

It does not create:

- a `/dev` node;
- sysfs attributes;
- ioctl commands;
- procfs files;
- debugfs files;
- real regulator, RTC, input, GPIO, or watchdog devices.

On real hardware, the child drivers created by an MFD parent may expose standard subsystem ABIs, for example:

- regulator state under regulator framework interfaces;
- RTC nodes such as `/dev/rtcX`;
- input events such as `/dev/input/eventX`;
- GPIO controller lines through the GPIO character-device ABI.

Those ABIs belong to the child subsystem drivers, not to the MFD parent itself.

## Build

Compile the learning DTS into a DTB:

```sh
cd Linux-Device-Driver/examples/19-regmap-irq-and-mfd-syscon
dtc -@ -I dts -O dtb -o regmap-irq-mfd-syscon-demo.dtb \
    regmap-irq-mfd-syscon-demo.dts
```

Decompile it back for inspection:

```sh
dtc -I dtb -O dts -o regmap-irq-mfd-syscon-demo.roundtrip.dts \
    regmap-irq-mfd-syscon-demo.dtb
```

Expected generated files:

```text
regmap-irq-mfd-syscon-demo.dtb
regmap-irq-mfd-syscon-demo.roundtrip.dts
```

Clean generated files:

```sh
rm -f regmap-irq-mfd-syscon-demo.dtb \
      regmap-irq-mfd-syscon-demo.roundtrip.dts
```

## Load

There is no safe generic load command for this example.

Do **not** apply this DTS as an overlay to a real board. It uses fake providers and fake devices. A real board needs:

- real MMIO addresses;
- real interrupt parents and interrupt specifiers;
- real compatible strings documented by bindings;
- real parent and child drivers;
- board-specific power, reset, pinctrl, and clock descriptions.

## Test And Inspect

Inspect the fake PMIC shape:

```sh
grep -n "training,pmic" regmap-irq-mfd-syscon-demo.roundtrip.dts
grep -n "interrupt-controller" regmap-irq-mfd-syscon-demo.roundtrip.dts
```

Expected output shape:

```text
compatible = "training,pmic-mfd";
compatible = "training,pmic-regulators";
compatible = "training,pmic-rtc";
compatible = "training,pmic-pwrkey";
interrupt-controller;
```

Inspect the fake syscon/`simple-mfd` shape:

```sh
grep -n "syscon" regmap-irq-mfd-syscon-demo.roundtrip.dts
grep -n "simple-mfd" regmap-irq-mfd-syscon-demo.roundtrip.dts
```

Expected output shape:

```text
compatible = "training,gpr", "syscon", "simple-mfd";
```

## Code Shape In A Real Parent Driver

A real PMIC parent would pair the DTS with code shaped like this:

```c
pmic->map = devm_regmap_init_i2c(client, &pmic_regmap_config);
if (IS_ERR(pmic->map))
    return dev_err_probe(&client->dev, PTR_ERR(pmic->map),
                         "regmap init failed\n");

ret = devm_regmap_add_irq_chip(&client->dev, pmic->map,
                               client->irq, IRQF_ONESHOT, 0,
                               &pmic_irq_chip, &pmic->irq_data);
if (ret)
    return dev_err_probe(&client->dev, ret, "IRQ chip failed\n");

return devm_mfd_add_devices(&client->dev, PLATFORM_DEVID_NONE,
                            pmic_cells, ARRAY_SIZE(pmic_cells),
                            NULL, 0,
                            regmap_irq_get_domain(pmic->irq_data));
```

A real syscon consumer would usually use an explicit phandle:

```c
gpr = syscon_regmap_lookup_by_phandle(dev->of_node, "training,gpr");
if (IS_ERR(gpr))
    return dev_err_probe(dev, PTR_ERR(gpr), "missing syscon\n");

return regmap_update_bits(gpr, CTRL_REG, CTRL_MASK, CTRL_VALUE);
```

## Debug Workflow On Real Hardware

For a real MFD/regmap IRQ driver, debug in layers:

```sh
dmesg | grep -Ei 'mfd|regmap|irq|pmic|syscon'
cat /proc/interrupts
ls /sys/bus/platform/devices
find /sys/kernel/debug/regmap -maxdepth 2 -type f 2>/dev/null
```

Useful checks:

- parent driver probed before child drivers;
- `devm_regmap_add_irq_chip()` did not fail;
- child platform devices exist under `/sys/bus/platform/devices`;
- child `platform_get_irq()` calls return valid IRQ numbers;
- `/proc/interrupts` shows the parent IRQ and child virqs changing as expected;
- syscon phandles point to a node with a valid `reg` range and `"syscon"` fallback.

## Expected Logs In A Real Driver

A real driver might log:

```text
demo-pmic 0-0058: regmap initialized
demo-pmic 0-0058: registered regmap IRQ controller on irq 42
demo-pmic 0-0058: registered 3 MFD child devices
demo-pmic-pwrkey demo-pmic-pwrkey: using IRQ 107
```

If the child IRQ domain was not passed to MFD, a child might fail like:

```text
demo-pmic-pwrkey demo-pmic-pwrkey: failed to get IRQ: -ENXIO
```

## Cleanup And Error Paths

The DTS-only example cleans up generated files with `rm`.

A real driver must handle these cleanup rules:

- if regmap initialization fails, return immediately before registering children;
- if regmap IRQ registration fails, do not create child devices that depend on child IRQs;
- child devices must be removed before parent-owned register access disappears;
- pending child IRQ handlers, work items, and subsystem callbacks must stop before parent teardown;
- manually-created secondary I2C clients must be unregistered in the correct order;
- devm-managed resources help with memory/resource release but do not replace semantic ordering.

## Why This Is Not Production-Ready

This example is learning-only because:

- compatible strings are not real bindings;
- the fake interrupt controller and I2C controller do not exist;
- no YAML binding validation is provided;
- no parent MFD kernel driver is compiled;
- no child subsystem drivers are compiled;
- no hardware IRQ, regmap, or syscon access is exercised.

Production code would add:

- documented bindings and `dtbs_check` validation;
- a real parent driver using target-kernel headers;
- real child drivers or existing subsystem child drivers;
- register definitions from the datasheet;
- interrupt status/mask/ack handling validated on hardware;
- suspend/resume and wakeup handling;
- safe debugfs/ftrace usage for registers with side effects.
