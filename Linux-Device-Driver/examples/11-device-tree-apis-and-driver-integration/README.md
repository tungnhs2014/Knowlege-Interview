# 11 - Device Tree APIs And Driver Integration Example

This is a **learning-only** Device Tree integration example. It shows how a DT-backed platform driver consumes the node that topic 10 only described.

It is minimal but realistic in the driver-side flow: match table, module alias, match data, MMIO lookup, IRQ lookup, typed property reading, optional clock/GPIO consumers, expected probe logs, and cleanup for enabled state.

Do **not** boot the included DTS on a real board. Its addresses, interrupt controller, and GPIO controller are fake placeholders.

## Goal

Use this example to connect the runtime chain:

```text
dt-api-demo.dts
  -> compatible = "acme,dt-api-demo-v2"
  -> platform bus creates a device when a real DTB contains that enabled node
  -> dt_api_demo.ko matches through .of_match_table
  -> probe() gets resources through platform/subsystem helpers
  -> probe() reads only custom binding properties with typed OF helpers
```

The code demonstrates:

- `struct of_device_id` with `.data` match data;
- `MODULE_DEVICE_TABLE(of, ...)` for module aliases;
- `.driver.of_match_table`;
- `of_match_device()` in `probe()`;
- `devm_platform_ioremap_resource()` for `reg`;
- `platform_get_irq()` for `interrupts`;
- `devm_clk_get_optional(dev, "core")` for `clocks` / `clock-names`;
- `devm_gpiod_get_optional(dev, "reset", ...)` for `reset-gpios`;
- `of_property_read_u32()` for a custom property;
- `of_property_read_bool()` for a boolean property;
- `dev_err_probe()` so errors and `-EPROBE_DEFER` stay meaningful;
- `devm_add_action_or_reset()` to undo `clk_prepare_enable()`.

## Kernel Version Assumptions

Build the module against the headers for the exact kernel you will load it into:

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

The example assumes headers with:

- `devm_platform_ioremap_resource()`;
- `devm_clk_get_optional()`;
- `devm_add_action_or_reset()`;
- `struct platform_driver.remove_new`.

Older kernels may need small edits:

- use `platform_get_resource()` plus `devm_ioremap_resource()` if the platform ioremap helper is unavailable;
- use `devm_clk_get()` and treat `-ENOENT` as optional if `devm_clk_get_optional()` is unavailable;
- use `.remove` returning `int` instead of `.remove_new`.

## Files

| File | Purpose |
| --- | --- |
| `dt_api_demo.c` | Learning-only DT-backed platform driver. It is buildable as an out-of-tree module. |
| `dt-api-demo.dts` | Standalone learning DTS snippet showing the node shape the driver expects. |
| `Makefile` | Builds `dt_api_demo.ko` and cleans module plus DTS artifacts. |
| `README.md` | Build, load, inspect, debug, cleanup, and production notes. |

No userspace test program is included because this driver creates no `/dev` node and no sysfs, procfs, debugfs, or ioctl interface.

## Build

From this directory:

```sh
make
```

Expected build artifact:

```text
dt_api_demo.ko
```

For a cross-compiled target kernel:

```sh
make KDIR=/path/to/target/kernel/build \
     ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

Compile the standalone DTS only for inspection:

```sh
dtc -I dts -O dtb -o dt-api-demo.dtb dt-api-demo.dts
dtc -I dtb -O dts -o dt-api-demo.decompiled.dts dt-api-demo.dtb
```

The DTS may produce warnings because its interrupt and GPIO providers are fake. That is acceptable for this learning snippet; production DTS files must be validated with real YAML bindings and `dtbs_check`.

## Load And Inspect

On a normal development machine, loading the module only registers the driver. `probe()` will not run unless the live Device Tree already contains a matching enabled node.

```sh
sudo insmod ./dt_api_demo.ko
dmesg | tail -20
modinfo -F alias ./dt_api_demo.ko
ls /sys/bus/platform/drivers/dt-api-demo
```

Expected alias shape:

```text
of:N*T*Cacme,dt-api-demo-v1
of:N*T*Cacme,dt-api-demo-v1C*
of:N*T*Cacme,dt-api-demo-v2
of:N*T*Cacme,dt-api-demo-v2C*
```

Expected behavior without a matching live DT node:

- the module loads successfully;
- `/sys/bus/platform/drivers/dt-api-demo/` exists;
- no `probe: variant=...` log appears.

On a real target that has a real, enabled node and real provider bindings, expected probe log shape:

```text
dt-api-demo 10010000.demo: probe: variant=v2 fifo=64 irq=<n> sample=2000 ns dma=yes
```

The exact device name and Linux IRQ number depend on the target kernel and interrupt provider.

## Runtime Debug Commands

Use these on a DT-based target:

```sh
dmesg | grep -i dt-api-demo
modinfo ./dt_api_demo.ko
modinfo -F alias ./dt_api_demo.ko
find /sys/bus/platform/devices -maxdepth 1 -name '*demo*' -print
find /sys/bus/platform/drivers/dt-api-demo -maxdepth 1 -print
cat /sys/bus/platform/devices/*demo*/modalias 2>/dev/null
readlink /sys/bus/platform/devices/*demo*/driver 2>/dev/null
```

Inspect the live Device Tree:

```sh
find /sys/firmware/devicetree/base -name compatible | grep -i demo
tr -d '\0' < /sys/firmware/devicetree/base/soc/demo@10010000/compatible
tr -d '\0' < /sys/firmware/devicetree/base/soc/demo@10010000/status
```

If `probe()` does not run:

- check the node exists in the running DTB, not only in source;
- check `status = "okay"`;
- check the `compatible` spelling and vendor prefix;
- check that the node is under a bus that populates platform devices, such as `simple-bus`;
- check `modinfo -F alias` for OF aliases from `MODULE_DEVICE_TABLE(of, ...)`;
- check whether the driver is already built into the kernel instead of loaded as a module.

If probe returns `-EPROBE_DEFER`:

- check clock, GPIO, interrupt, regulator, reset, and other provider nodes;
- check the names used by the driver match DT names, such as `"core"` and `clock-names = "core"`;
- keep the original error code instead of converting it to `-EINVAL`.

## Userspace ABI Impact

There is **no userspace ABI impact**.

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
- live DT contents under `/sys/firmware/devicetree/base` on DT systems.

## Cleanup

Unload the module:

```sh
sudo rmmod dt_api_demo
dmesg | tail -20
```

Clean local build artifacts:

```sh
make clean
```

If you compiled the DTS manually, `make clean` also removes:

```text
dt-api-demo.dtb
dt-api-demo.decompiled.dts
```

## Cleanup And Error Paths

The driver uses managed cleanup for resources whose lifetime is tied to the device:

```text
devm_kzalloc()
devm_platform_ioremap_resource()
devm_clk_get_optional()
devm_gpiod_get_optional()
devm_request_irq()
```

The important non-managed state is the enabled clock:

```text
clk_prepare_enable()
  -> devm_add_action_or_reset(..., clk_disable_unprepare)
```

That means:

- if a later probe step fails, the devm action disables the clock;
- if `remove()` runs, the devm action disables the clock after remove returns;
- MMIO mapping, IRQ request, GPIO descriptor, and allocation are released automatically.

The IRQ handler reads a status register and logs it. A production driver would normally acknowledge or mask the hardware interrupt according to the hardware manual. This example does not do that because the hardware is fake.

## Why This Is Not Production-Ready

This example is **learning-only** because:

- the DTS describes fake hardware;
- there is no YAML binding;
- the interrupt and GPIO providers are placeholders;
- the IRQ handler does not acknowledge real hardware;
- power, reset, runtime PM, register definitions, and subsystem registration are intentionally incomplete;
- the custom properties are examples, not a stable ABI contract.

Production code would add:

- a real binding schema and `dtbs_check`;
- real compatible strings from the hardware/vendor;
- named resources when multiple MMIO regions or IRQs exist;
- real clock, reset, regulator, pinctrl, and power sequencing;
- hardware-specific interrupt acknowledgement;
- subsystem registration, such as IIO, input, V4L2, hwmon, or char device only when appropriate;
- stronger review of property defaults and backward compatibility.

## What To Try Next

1. Remove `MODULE_DEVICE_TABLE(of, dt_api_demo_of_match);`, rebuild, and compare `modinfo -F alias`.
2. Change `compatible` in the DTS to a misspelled vendor string and explain why probe would not run.
3. Rename `reset-gpios` to `reset-gpio` and explain why `devm_gpiod_get_optional(dev, "reset", ...)` no longer finds it.
4. Add `clock-names = "bus";` while the driver asks for `"core"`, then predict the probe error.
5. Replace `of_property_read_u32()` with an ignored return value and explain the uninitialized-data bug it could create.
