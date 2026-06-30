# 26 - Input Device Drivers Example

## Status
This is a **learning-only** example.

It builds a loadable kernel module that registers a synthetic input device named `ldd-input-demo`. It is useful for learning `struct input_dev`, capability bits, `open()`/`close()`, `input_report_key()`, `input_sync()`, and evdev testing. It is **not production-ready** because it does not bind to real GPIO, I2C, SPI, USB, HID, or firmware-described hardware.

For a real board button, first consider the existing `gpio-keys` or `gpio-keys-polled` driver instead of writing custom code.

## Goal
Use this example to connect the runtime chain:

```text
insmod ldd_input_demo.ko
  -> module creates a synthetic platform_device
  -> platform_driver probe allocates struct input_dev
  -> input core registers an EV_KEY device
  -> evdev exposes /dev/input/eventX
  -> evtest opens the device
  -> input open() schedules synthetic press/release events
  -> userspace receives struct input_event records
```

The example demonstrates:

- `devm_input_allocate_device()`;
- `input_set_capability(input, EV_KEY, key_code)`;
- `input_register_device()`;
- `input->open` and `input->close`;
- `input_report_key()`;
- `input_sync()`;
- `EV_KEY` value `1` for press and `0` for release;
- cleanup with delayed-work cancellation;
- standard evdev userspace ABI under `/dev/input/eventX`.

## Kernel Version Assumptions
Build against the exact kernel headers for the target system:

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

This example was checked against Linux `6.8.0-124-generic` headers and assumes:

- `devm_input_allocate_device()` from `<linux/input.h>`;
- `input_set_capability()` from `<linux/input.h>`;
- `input_register_device()` from `<linux/input.h>`;
- `struct platform_driver.remove = int (*)(struct platform_device *pdev)` on this target header set;
- evdev support enabled in the running kernel;
- a userspace tool such as `evtest` for convenient testing.

Kernel APIs can drift. Older input examples may use `struct input_polled_dev` and `<linux/input-polldev.h>`, but current kernel documentation prefers polling helpers such as `input_setup_polling()` on `struct input_dev`.

## Files
| File | Purpose |
| --- | --- |
| `ldd_input_demo.c` | Learning-only platform module registering a synthetic EV_KEY input device. |
| `Makefile` | Out-of-tree Kbuild wrapper for `ldd_input_demo.ko`. |
| `README.md` | Build, load, test, debug, ABI, cleanup, and production notes. |

No Device Tree file is required for this synthetic module. A realistic board-level button can often be described with `gpio-keys`:

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

## Userspace ABI Impact
When loaded successfully, this example creates standard input/evdev ABI:

- `/dev/input/eventX`
- `/proc/bus/input/devices`
- `/sys/class/input/inputX/`
- optional udev symlinks under `/dev/input/by-path/`

It emits `EV_KEY` events for `key_code`, which defaults to `KEY_ENTER` (`28`). Each synthetic press/release sequence is visible as `struct input_event` records through evdev.

The module also exposes normal module parameters under `/sys/module/ldd_input_demo/parameters/`:

- `key_code`
- `emit_count`
- `interval_ms`

Those parameters are for the learning module only and are not a production userspace ABI.

This example does **not** create:

- custom character devices;
- custom ioctl commands;
- custom sysfs attributes owned by the driver;
- procfs entries;
- debugfs entries.

## Build
From this directory:

```sh
make
```

Expected build artifact:

```text
ldd_input_demo.ko
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

## Load
Watch kernel logs:

```sh
sudo dmesg -w
```

Load with defaults:

```sh
sudo insmod ./ldd_input_demo.ko
```

Expected log shape:

```text
ldd-input-demo ldd-input-demo.0: registered learning-only input demo, key_code=28
input: ldd-input-demo as /devices/platform/ldd-input-demo.0/input/inputX
```

Load with custom behavior:

```sh
sudo insmod ./ldd_input_demo.ko key_code=116 emit_count=2 interval_ms=300
```

`key_code=116` is commonly `KEY_POWER`.

## Find The Event Node
Use `/proc/bus/input/devices`:

```sh
grep -A6 -B2 "ldd-input-demo" /proc/bus/input/devices
```

Expected output shape:

```text
N: Name="ldd-input-demo"
P: Phys=ldd-input-demo/input0
H: Handlers=eventX
B: EV=3
B: KEY=...
```

Or use shell discovery:

```sh
EVENT=$(grep -A6 -B2 "ldd-input-demo" /proc/bus/input/devices |
	awk '/Handlers=/ {
		for (i = 1; i <= NF; i++)
			if ($i ~ /^event[0-9]+$/)
				print "/dev/input/" $i
	}' | head -n1)
echo "$EVENT"
```

Do not hardcode `/dev/input/event0`; event numbers depend on boot order and other input devices.

## Test With evtest
Install `evtest` if needed:

```sh
sudo apt-get install evtest
```

Run:

```sh
sudo evtest "$EVENT"
```

Opening the device calls the driver's `open()` callback. The module then emits `emit_count` press/release pairs. Expected event shape:

```text
Input device name: "ldd-input-demo"
Supported events:
  Event type 0 (EV_SYN)
  Event type 1 (EV_KEY)
    Event code 28 (KEY_ENTER)
Event: type 1 (EV_KEY), code 28 (KEY_ENTER), value 1
Event: type 0 (EV_SYN), code 0 (SYN_REPORT), value 0
Event: type 1 (EV_KEY), code 28 (KEY_ENTER), value 0
Event: type 0 (EV_SYN), code 0 (SYN_REPORT), value 0
```

Close `evtest` with `Ctrl-C`. Reopening the device schedules a new sequence.

## Other Debug Commands
Inspect udev/device properties:

```sh
udevadm info "$EVENT"
```

Check module parameters:

```sh
cat /sys/module/ldd_input_demo/parameters/key_code
cat /sys/module/ldd_input_demo/parameters/emit_count
cat /sys/module/ldd_input_demo/parameters/interval_ms
```

Check kernel logs:

```sh
dmesg | grep -E "ldd-input-demo|input:"
```

Enable dynamic debug for the module if your kernel supports it:

```sh
echo 'module ldd_input_demo +p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

Then reopen the event node with `evtest` and look for debug reports:

```sh
dmesg | grep ldd-input-demo
```

Disable dynamic debug:

```sh
echo 'module ldd_input_demo -p' | sudo tee /sys/kernel/debug/dynamic_debug/control
```

## Unload And Cleanup
Unload:

```sh
sudo rmmod ldd_input_demo
```

Expected log shape:

```text
ldd-input-demo ldd-input-demo.0: removed learning-only input demo
```

Clean build outputs:

```sh
make clean
```

## Cleanup And Error-Path Explanation
The module uses a synthetic `platform_device` and `platform_driver` so probe/remove behave like a real subsystem driver.

Probe path:

- validates `key_code` and `interval_ms`;
- allocates private state with `devm_kzalloc()`;
- allocates the input device with `devm_input_allocate_device()`;
- initializes delayed work and a mutex;
- sets `name`, `phys`, `BUS_HOST`, parent, `open()`, and `close()`;
- declares `EV_KEY` capability with `input_set_capability()`;
- registers with `input_register_device()`.

Runtime path:

- `open()` starts a delayed-work sequence;
- each work item reports press or release with `input_report_key()`;
- each event frame ends with `input_sync()`;
- `close()` stops future work and releases a pressed key if userspace closes early.

Remove path:

- marks the demo inactive;
- clears pending event count;
- calls `cancel_delayed_work_sync()` so no worker uses the input device after remove begins;
- relies on managed resources to release the input device and private memory.

Production code would need stricter hardware-specific cleanup:

- stop the device interrupt source;
- synchronize/free IRQs before freeing state;
- cancel debounce work;
- disable regulators/clocks;
- handle runtime PM and wake IRQ policy;
- preserve ABI compatibility for key codes and Device Tree bindings.

## Why This Is Not Production-Ready
This example intentionally avoids real hardware so it can be built and tested anywhere.

It is missing production requirements:

- no real GPIO, I2C, SPI, USB, HID, or MFD binding;
- no Device Tree binding validation;
- no hardware debounce or delayed-work debounce for real mechanical switches;
- no active-low GPIO handling;
- no IRQ path or GPIO expander sleep-context handling;
- no suspend/resume or wakeup-source policy;
- no udev/seat/access-control product policy.

Use it to learn input core mechanics. For a real board button, start with `gpio-keys`; for a real custom device, adapt the lifecycle but bind to real hardware and validate the ABI with `evtest`, `/proc/bus/input/devices`, suspend/resume, and repeated unload/load tests.
