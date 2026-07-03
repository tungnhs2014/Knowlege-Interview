# 37 - Kernel Debugging And Tracing Example

## Status

This example is **learning-only**. It creates a tiny root device and a timer/workqueue loop so you can practice `dev_info()`, `dev_dbg()`, dynamic debug, ftrace, trace events, and cleanup without real hardware.

It is not production-ready because it is a synthetic workload, has no real device ownership model, and exists only to generate safe debug and trace signals.

## Goal

Practice a realistic kernel debugging workflow:

- Build and load a small module.
- Observe normal `dev_info()` logs.
- Enable hidden `dev_dbg()` logs through dynamic debug.
- Trace specific module functions with ftrace/function graph tracing.
- Trace generic timer/workqueue events.
- Clean up dynamic-debug and tracefs settings afterward.

## Kernel Version Assumptions

- Intended for modern Linux 5.x/6.x kernels with loadable module support.
- Requires kernel headers for the running kernel at `/lib/modules/$(uname -r)/build`.
- Dynamic debug requires kernel support such as `CONFIG_DYNAMIC_DEBUG` or equivalent dynamic-debug configuration.
- Ftrace examples require tracefs and relevant tracing configs such as `CONFIG_FTRACE` and event tracing.
- Current kernels usually expose tracefs at `/sys/kernel/tracing`; older setups may expose it through `/sys/kernel/debug/tracing`.
- `interval_ms` is intentionally read-only after load. Set it with `insmod interval_ms=<value>` so the timer interval is validated before the timer starts.

## Files

```text
examples/37-kernel-debugging-and-tracing/
  README.md
  Makefile
  ldd_debug_trace_demo.c
```

## Build Command

```sh
cd Linux-Device-Driver/examples/37-kernel-debugging-and-tracing
make
```

Expected build artifact:

```text
ldd_debug_trace_demo.ko
```

If the build fails with a missing `build` directory, install the kernel headers for the running kernel or point `KDIR` at a configured kernel build tree:

```sh
make KDIR=/path/to/linux/build
```

## Load Commands

Load the module with a one-second timer:

```sh
sudo insmod ./ldd_debug_trace_demo.ko interval_ms=1000
dmesg | tail -20
```

Expected log shape:

```text
ldd_dbgtrace ldd_dbgtrace: loaded, interval_ms=1000
```

After about five timer ticks:

```text
ldd_dbgtrace ldd_dbgtrace: heartbeat ticks=5
```

The exact prefix can vary by kernel, but the message should include `ldd_dbgtrace` and the heartbeat count.

## Dynamic Debug Test

First check whether the callsites are visible:

```sh
sudo grep ldd_debug_trace_demo /proc/dynamic_debug/control
```

Enable debug prints for this module:

```sh
echo 'module ldd_debug_trace_demo +p' | sudo tee /proc/dynamic_debug/control
dmesg -w
```

Expected debug log shape:

```text
ldd_dbgtrace ldd_dbgtrace: work ran, ticks=6 interval_ms=1000
ldd_dbgtrace ldd_dbgtrace: work ran, ticks=7 interval_ms=1000
```

Disable debug prints when finished:

```sh
echo 'module ldd_debug_trace_demo -p' | sudo tee /proc/dynamic_debug/control
```

If no dynamic-debug callsite appears, the running kernel probably lacks dynamic-debug support for this module build. The module can still be used for ftrace practice.

## Ftrace Function Graph Test

Find tracefs:

```sh
TRACEFS=/sys/kernel/tracing
if [ ! -d "$TRACEFS" ]; then
	TRACEFS=/sys/kernel/debug/tracing
fi
echo "$TRACEFS"
```

Prepare a narrow function graph trace for this module only:

```sh
echo 0 | sudo tee "$TRACEFS/tracing_on"
echo nop | sudo tee "$TRACEFS/current_tracer"
echo > "$TRACEFS/trace"
echo ldd_dbgtrace_work | sudo tee "$TRACEFS/set_ftrace_filter"
echo function_graph | sudo tee "$TRACEFS/current_tracer"
echo 1 | sudo tee "$TRACEFS/tracing_on"
sleep 3
echo 0 | sudo tee "$TRACEFS/tracing_on"
sudo cat "$TRACEFS/trace"
```

Expected trace shape:

```text
ldd_dbgtrace_work() {
  ...
}
```

Some kernels may show only entry/exit timing, CPU IDs, or task names depending on trace options.

## Trace Event Test

Use generic timer and workqueue events to observe the module's timer-driven behavior:

```sh
echo 0 | sudo tee "$TRACEFS/tracing_on"
echo nop | sudo tee "$TRACEFS/current_tracer"
echo > "$TRACEFS/trace"
echo 1 | sudo tee "$TRACEFS/events/timer/timer_expire_entry/enable"
echo 1 | sudo tee "$TRACEFS/events/workqueue/workqueue_execute_start/enable"
echo 1 | sudo tee "$TRACEFS/tracing_on"
sleep 3
echo 0 | sudo tee "$TRACEFS/tracing_on"
sudo grep -E 'ldd_dbgtrace|workqueue|timer' "$TRACEFS/trace" | head -40
```

Expected output varies by kernel, but you should see timer or workqueue records around the period where the module is active.

## Unload Commands

```sh
sudo rmmod ldd_debug_trace_demo
dmesg | tail -20
```

Expected log shape:

```text
ldd_dbgtrace ldd_dbgtrace: unloaded after 12 ticks
```

## Cleanup Commands

Reset dynamic debug:

```sh
if [ -w /proc/dynamic_debug/control ]; then
	echo 'module ldd_debug_trace_demo -p' | sudo tee /proc/dynamic_debug/control
fi
```

Reset tracefs:

```sh
echo 0 | sudo tee "$TRACEFS/tracing_on"
echo 0 | sudo tee "$TRACEFS/events/timer/timer_expire_entry/enable" 2>/dev/null || true
echo 0 | sudo tee "$TRACEFS/events/workqueue/workqueue_execute_start/enable" 2>/dev/null || true
echo nop | sudo tee "$TRACEFS/current_tracer"
echo > "$TRACEFS/set_ftrace_filter"
echo > "$TRACEFS/set_ftrace_notrace"
echo > "$TRACEFS/trace"
```

Remove build artifacts:

```sh
make clean
```

## Cleanup And Error-Path Explanation

The module uses a simple lifetime model:

- `root_device_register()` creates a device context so `dev_*()` logs are realistic.
- `timer_setup()` initializes a timer that periodically queues work.
- `INIT_WORK()` creates a work item that emits `dev_dbg()` and occasional `dev_info()` messages.
- `del_timer_sync()` stops the timer during unload.
- `cancel_work_sync()` waits for any queued work to finish.
- `root_device_unregister()` removes the synthetic device.

The init error path is intentionally small:

- invalid `interval_ms` returns `-EINVAL` before any resource is registered;
- failed `root_device_register()` returns the error pointer value;
- timer and work are only started after the device exists.

This ordering prevents the timer/workqueue path from using a missing `struct device`.

## Userspace ABI Impact

This example does not create device nodes, ioctl commands, sysfs class devices, procfs files, or debugfs files.

It does create normal module-visible state:

- `/sys/module/ldd_debug_trace_demo/parameters/interval_ms`
- a synthetic root device under `/sys/devices/ldd_dbgtrace` on typical systems

The module parameter is read-only after load. Those paths are for learning and debugging only. Do not design product software around this example's synthetic device name or module parameter.

## What Production Code Would Add

A production driver would add:

- a real bus binding and hardware lifetime model;
- clear ownership of IRQs, memory, clocks, regulators, GPIOs, and DMA resources;
- subsystem tracepoints only when they expose useful long-term diagnostics;
- rate limiting for repeated warnings;
- no temporary `dump_stack()` or `trace_printk()`;
- a documented, stable userspace ABI only when userspace must depend on it.
