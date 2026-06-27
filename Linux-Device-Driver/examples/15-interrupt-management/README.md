# 15 - Interrupt Management Example

This is a **learning-only** interrupt management example. It includes a small platform driver that requests one IRQ from Device Tree, handles the fast hard-IRQ part, and defers slower processing either to a threaded IRQ handler or to a workqueue.

Do **not** load this on arbitrary hardware. The driver assumes a tiny MMIO register block with status, clear, enable, and disable registers. You must adapt the DTS and register offsets to real, documented hardware before testing on a board.

## Goal

Use this example to connect the runtime chain:

```text
irq-demo.dts
  -> platform device with reg + interrupts
  -> irq_demo.c probe()
  -> devm_platform_ioremap_resource()
  -> platform_get_irq()
  -> devm_request_threaded_irq() or devm_request_irq()
  -> hard IRQ reads and clears status
  -> threaded IRQ or workqueue processes saved status
  -> remove disables device IRQ source and cancels work
```

The example demonstrates:

- `platform_get_irq()` from firmware resources;
- `devm_request_threaded_irq()` with `IRQF_ONESHOT`;
- `devm_request_irq()` plus `schedule_work()` as an alternate bottom-half path;
- hard IRQ return values: `IRQ_NONE`, `IRQ_HANDLED`, `IRQ_WAKE_THREAD`;
- a private `dev_id` pointer;
- short spinlock-protected pending-status handoff;
- device-level interrupt enable/disable and remove-time cleanup;
- debugging with `dmesg`, `/proc/interrupts`, and platform sysfs.

## Kernel Version Assumptions

Build against the exact kernel headers for the target system:

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

This example assumes a modern kernel with:

- external module builds through Kbuild;
- `devm_platform_ioremap_resource()`;
- `devm_request_irq()` and `devm_request_threaded_irq()`;
- `struct platform_driver.remove_new`.

Older kernels may use the old remove callback signature:

```c
static int irq_demo_remove(struct platform_device *pdev)
{
	...
	return 0;
}

static struct platform_driver irq_demo_driver = {
	.probe = irq_demo_probe,
	.remove = irq_demo_remove,
	...
};
```

Validate current API signatures against the target kernel headers before adapting this for real hardware.

## Files

| File | Purpose |
| --- | --- |
| `irq_demo.c` | Learning-only platform driver that requests an IRQ and demonstrates threaded IRQ vs workqueue deferral. |
| `irq-demo.dts` | Placeholder Device Tree overlay fragment showing `compatible`, `reg`, and `interrupts`. |
| `Makefile` | Out-of-tree Kbuild wrapper for `irq_demo.ko`. |
| `README.md` | Build, load, test, debug, cleanup, ABI, and production notes. |

## Userspace ABI Impact

This example creates **no stable userspace ABI**:

- no `/dev` node;
- no ioctl;
- no custom sysfs attributes;
- no procfs file;
- no debugfs file.

The visible surfaces are normal kernel diagnostics:

- kernel logs from `dev_info()` / `dev_err_probe()`;
- platform bus sysfs entries under `/sys/bus/platform/`;
- interrupt counters in `/proc/interrupts`;
- module parameters under `/sys/module/irq_demo/parameters/`.

## Build

From this directory:

```sh
make
```

Expected build artifacts include:

```text
irq_demo.ko
irq_demo.o
Module.symvers
modules.order
```

For a cross-compiled target kernel:

```sh
make KDIR=/path/to/target/kernel/build \
     ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

Clean generated files:

```sh
make clean
```

## Device Tree Setup

Compile the placeholder DTS for inspection:

```sh
dtc -@ -I dts -O dtb -o irq-demo.dtbo irq-demo.dts
dtc -I dtb -O dts -o irq-demo.decompiled.dts irq-demo.dtbo
```

Before applying it to a board, replace:

- `target-path` with the correct parent bus path;
- `reg = <0x80000000 0x1000>` with a real safe MMIO resource;
- `interrupts = <0 56 4>` with the real interrupt-controller specifier;
- register offsets in `irq_demo.c` if the hardware layout differs.

The demo driver's assumed register block is:

| Offset | Name | Demo assumption |
| --- | --- | --- |
| `0x00` | status | nonzero bits mean pending IRQ causes |
| `0x04` | clear | write-one-to-clear pending bits |
| `0x08` | enable | write bits to enable interrupt sources |
| `0x0c` | disable | write bits to disable interrupt sources |

**Production warning:** real hardware may use read-to-clear, write-zero-to-clear, separate mask registers, level status, edge status, or ordered ack requirements. Follow the datasheet, not this demo layout.

## Load And Test

Use a target board with an adapted Device Tree node and real compatible hardware.

Watch logs:

```sh
sudo dmesg -w
```

Load with threaded IRQ deferral, the default:

```sh
sudo insmod ./irq_demo.ko
```

Expected log shape:

```text
irq-demo <device>: registered IRQ <n> using threaded IRQ deferral
```

Trigger the real hardware interrupt. Expected log shape:

```text
irq-demo <device>: threaded IRQ handled status=0x00000001
```

Check the interrupt counter:

```sh
cat /proc/interrupts | grep -i irq-demo
```

Unload:

```sh
sudo rmmod irq_demo
```

Expected log shape:

```text
irq-demo <device>: removed IRQ demo
```

### Workqueue Mode

Load with workqueue deferral:

```sh
sudo insmod ./irq_demo.ko use_threaded=0
```

Expected log shape:

```text
irq-demo <device>: registered IRQ <n> using workqueue deferral
irq-demo <device>: workqueue handled status=0x00000001
```

Check the live module parameter:

```sh
cat /sys/module/irq_demo/parameters/use_threaded
```

## Debug Commands

Use these while bringing up the adapted driver:

```sh
dmesg | grep -Ei 'irq-demo|irq|interrupt'
cat /proc/interrupts | grep -Ei 'irq-demo|<device-name>|<irq-number>'
ls /sys/bus/platform/devices | grep -i irq
find /sys/bus/platform/drivers/irq-demo -maxdepth 2 -type l -o -type f
```

Inspect the live Device Tree node:

```sh
find /sys/firmware/devicetree/base -name compatible | grep -i irq-demo
tr -d '\0' < /sys/firmware/devicetree/base/<path-to-node>/compatible
hexdump -C /sys/firmware/devicetree/base/<path-to-node>/interrupts
```

Follow logs while triggering:

```sh
sudo dmesg -w
```

## Expected Output And Failure Patterns

Successful probe:

```text
registered IRQ <n> using threaded IRQ deferral
```

No matching device:

```text
insmod succeeds, but probe log does not appear
```

Likely causes:

- adapted DT node was not loaded;
- `compatible` does not match `training,irq-demo`;
- platform device did not instantiate from firmware.

IRQ never fires:

```text
/proc/interrupts counter does not increment
```

Likely causes:

- wrong `interrupts` cells or parent interrupt controller;
- wrong pinmux or interrupt routing;
- device interrupt source not enabled;
- hardware status bit never set;
- trigger polarity mismatch.

IRQ storm:

```text
/proc/interrupts counter increments continuously
```

Likely causes:

- status bit is not really write-one-to-clear;
- level-triggered line remains asserted;
- clear/ack order is wrong for the hardware;
- wrong trigger type in Device Tree;
- hardware source was enabled before stale status was cleared.

Sleeping-in-atomic warning:

```text
BUG: sleeping function called from invalid context
```

This demo should not sleep in the hard handler. If you add I2C/SPI/regmap sleeping operations to `irq_demo_hardirq()`, move them to `irq_demo_thread()` or a workqueue.

## Cleanup And Error Paths

The probe path relies on devm-managed resources:

- `devm_kzalloc()` frees private data after device detach;
- `devm_platform_ioremap_resource()` unmaps MMIO automatically;
- `devm_request_irq()` / `devm_request_threaded_irq()` release the IRQ automatically.

Manual cleanup still matters:

- `irq_demo_remove()` writes the demo disable register so the device stops generating interrupts;
- `cancel_work_sync()` waits for queued workqueue processing to finish;
- threaded IRQ cleanup is handled by devm IRQ teardown, but private data must remain valid until teardown completes.

The request path returns errors directly:

```text
missing reg resource       -> devm_platform_ioremap_resource() error
missing interrupt resource -> platform_get_irq() error
busy or invalid IRQ        -> devm_request_*irq() error
```

## Why This Is Not Production-Ready

This is intentionally **learning-only**.

Production code would add:

- real register definitions from the hardware manual;
- exact status clear and mask/unmask ordering;
- clock, reset, and power-management handling;
- suspend/resume and wakeup policy;
- runtime PM if the block can be clock-gated;
- stronger validation of interrupt trigger type;
- rate-limited logging or tracepoints instead of `dev_info()` on every event;
- per-bit event decoding instead of a generic status print;
- tests on real hardware for storms, missing IRQs, remove races, and suspend/resume.

The important lesson is the shape of a correct IRQ consumer:

```text
get IRQ -> request IRQ -> keep hard handler short -> defer slow work
-> clear hardware cause -> disable source during cleanup
```
