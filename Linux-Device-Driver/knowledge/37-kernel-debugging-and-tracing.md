# 37 - Kernel Debugging And Tracing

## Learning Goal

By the end of this chapter, you should be able to debug a driver with the right level of visibility:

- Use `dev_*()` / `pr_*()` logging without flooding the system.
- Enable `dev_dbg()` / `pr_debug()` callsites at runtime with dynamic debug.
- Use ftrace and tracepoints to answer timing and control-flow questions.
- Inspect subsystem state through debugfs, sysfs, procfs, and framework tools.
- Read an oops well enough to identify the failing function, context, module, and next command to run.

## Why This Matters

Kernel driver bugs often happen where ordinary userspace debugging is weak:

- during early boot, probe, suspend/resume, or shutdown;
- inside IRQ, timer, workqueue, NAPI, or atomic context;
- only under real hardware timing;
- on a board where the only reliable output is serial console, `dmesg`, tracefs, or pstore.

A driver developer needs a layered toolbox. Logs tell you what your driver chose to report. Dynamic debug lets you turn hidden debug prints on after boot. Tracing tells you what actually ran and when. Oops analysis tells you what happened after the system already failed.

## Mental Model

Think of kernel debugging as layers:

| Layer | Best For | Tools |
| --- | --- | --- |
| Driver messages | Error paths, probe state, resource values | `dev_err()`, `dev_warn()`, `dev_info()`, `dev_dbg()`, `pr_*()` |
| Runtime debug prints | Enabling selected debug messages without recompiling | dynamic debug, `/proc/dynamic_debug/control` |
| Tracing | Timing, function flow, subsystem events, latency | tracefs, ftrace, tracepoints, `trace_pipe` |
| State inspection | Current framework/device state | debugfs, sysfs, procfs, subsystem tools |
| Crash analysis | Faults after the fact | oops log, symbols, `addr2line`, `objdump`, pstore/kdump |

The practical rule is: **start narrow, then broaden**. Turn on the smallest amount of logging or tracing that can answer your question. Broad tracing can change timing, fill buffers, and hide the bug you are chasing.

## Core Concepts

### Logging

Use logging when your driver knows something important enough to report:

- `dev_err()` / `dev_warn()` / `dev_info()` / `dev_dbg()` for device drivers.
- `pr_err()` / `pr_warn()` / `pr_info()` / `pr_debug()` for non-device or core code.
- Raw `printk()` mainly for low-level code or unusual cases.

For drivers, **prefer `dev_*()`** because the message includes the device identity. That matters when the same driver binds multiple devices.

Severity matters:

| Intent | Typical API |
| --- | --- |
| probe cannot continue | `dev_err()` or `dev_err_probe()` |
| recoverable but suspicious | `dev_warn()` |
| useful one-time state | `dev_info()` |
| detailed runtime diagnosis | `dev_dbg()` |
| hot path repeated messages | avoid, rate-limit, or trace instead |

The kernel log ring buffer is not the same thing as the console. A message may be stored in the ring buffer but not printed to the console because `console_loglevel` filters it. Check with `dmesg` before assuming a message did not exist.

Useful commands:

```sh
dmesg -w
dmesg -l err,warn
dmesg --time-format=iso
cat /proc/sys/kernel/printk
sudo dmesg -n 7
```

### Dynamic Debug

`dev_dbg()` and `pr_debug()` are usually quiet. Dynamic debug gives the kernel a runtime catalog of debug print callsites and a query language to enable selected ones.

Common selectors:

| Selector | Meaning |
| --- | --- |
| `module foo` | all dynamic-debug callsites in module `foo` |
| `file drivers/iio/*` | callsites matching a file path |
| `func foo_probe` | callsites in one function |
| `line 100-150` | callsites in a line range |
| `format "rx"` | callsites whose format string matches |

Common operations:

```sh
# See available callsites.
sudo grep -i my_driver /proc/dynamic_debug/control

# Enable prints for one module.
echo 'module my_driver +p' | sudo tee /proc/dynamic_debug/control

# Disable them again.
echo 'module my_driver -p' | sudo tee /proc/dynamic_debug/control
```

If dynamic debug is enabled but you still see nothing, check:

- the callsite is present in `/proc/dynamic_debug/control`;
- your code path actually ran;
- the output is in `dmesg`, not necessarily on the console;
- the kernel was built with the needed dynamic-debug support.

### Ftrace And Tracefs

Ftrace is the kernel's built-in tracing framework. It can trace function entry, function call graphs, latency, and static trace events.

Trace control files live under tracefs. On current kernels the preferred path is usually:

```sh
/sys/kernel/tracing
```

Older material often uses:

```sh
/sys/kernel/debug/tracing
```

That path still works on many systems when debugfs exposes tracefs there. A robust script should detect either.

Important files:

| File | Purpose |
| --- | --- |
| `available_tracers` | which tracers this kernel supports |
| `current_tracer` | selected tracer, such as `nop`, `function`, `function_graph` |
| `tracing_on` | start/stop writing to the trace buffer |
| `trace` | snapshot of the trace buffer |
| `trace_pipe` | live streaming trace output |
| `events/` | tracepoint event hierarchy |
| `available_events` | all available trace events |
| `set_ftrace_filter` | limit function tracing to selected functions |
| `set_ftrace_notrace` | exclude selected functions |
| `set_ftrace_pid` | limit tracing to selected task IDs where supported |
| `tracing_cpumask` | limit tracing to selected CPUs |
| `buffer_size_kb` | per-CPU trace buffer size |

Common tracers:

| Tracer | Use |
| --- | --- |
| `nop` | no function tracer; useful for event-only tracing |
| `function` | function entry tracing |
| `function_graph` | call graph and duration tracing |
| `irqsoff` | long IRQ-disabled sections, if enabled |
| `preemptoff` | long preemption-disabled sections, if enabled |
| `preemptirqsoff` | combined IRQ/preemption latency, if enabled |
| `wakeup` / `wakeup_rt` / `wakeup_dl` | scheduler wakeup latency, if enabled |

### Tracepoints And Events

Tracepoints are static instrumentation points compiled into kernel subsystems. Event tracing exposes them under:

```sh
$TRACEFS/events/<subsystem>/<event>/
```

Each event usually has:

- `enable`: turn the event on or off;
- `filter`: restrict which event records are stored;
- `format`: show event fields available for filters and decoding.

Filters reduce noise:

```sh
echo 'comm == "myapp"' | sudo tee $TRACEFS/events/sched/sched_switch/filter
echo 1 | sudo tee $TRACEFS/events/sched/sched_switch/enable
```

Numeric filters support comparisons such as `==`, `!=`, `<`, `<=`, `>`, `>=`, and bitwise `&`. String filters support exact matches and glob-style matching with `~` on supported fields.

### Debugfs, Sysfs, Procfs, And Subsystem Tools

Many driver problems are not solved by more logs. Sometimes you need to inspect framework state.

Examples:

| Problem | Useful Checks |
| --- | --- |
| Register map looks wrong | `/sys/kernel/debug/regmap/<device>/registers` |
| GPIO or pinmux mismatch | `/sys/kernel/debug/gpio`, pinctrl debugfs |
| Interrupt not firing | `/proc/interrupts`, IRQ name, count, affinity |
| Device Tree mismatch | `/proc/device-tree` or firmware DT sysfs path |
| V4L2 streaming bug | `v4l2-ctl`, `v4l2-compliance`, vb2 debug knobs |
| Module parameter issue | `modinfo`, `/sys/module/<module>/parameters/` |

**Debugfs is not a stable userspace ABI.** It is for debugging and diagnostics. Do not build production applications that depend on debugfs file formats.

### Oops And Panic Analysis

An oops is the kernel saying "something went wrong in kernel mode." A panic is the kernel stopping because it cannot safely continue or was configured to panic on certain failures.

Read an oops from the outside in:

| Field | What To Learn |
| --- | --- |
| reason line | NULL dereference, page fault, invalid opcode, BUG, WARN, use-after-free clue |
| kernel version | exact build you must match with symbols |
| taint flags | whether proprietary/out-of-tree/forced modules changed supportability |
| CPU / PID / Comm | which CPU and task context hit the problem |
| PC/IP or RIP | faulting instruction/function and offset |
| LR/caller | return address on architectures that report it |
| call trace | path to the fault |
| registers | bad pointer values, DMA addresses, state |
| module name | which loadable module owned the faulting code |

For symbolization, keep:

- `vmlinux` with debug info;
- `System.map`;
- the exact `.ko` files loaded on the target;
- kernel config;
- build ID or release string.

Useful commands:

```sh
# For built-in code when debug info exists:
addr2line -e vmlinux 0xffffffff81234567

# For a module, use the symbol+offset from the oops and inspect the module:
objdump -S --line-numbers my_driver.ko | less
```

## How The Kernel Implements It

### Printk Path

Driver logging eventually goes through the printk subsystem. The message is formatted, tagged with severity, stored in the kernel log buffer, and maybe emitted to consoles depending on console loglevel and console availability.

Important details:

- The ring buffer is finite and circular.
- Boot-time logs can be lost if the buffer is too small.
- `log_buf_len=` can increase buffer size on the kernel command line.
- `CONFIG_PRINTK_TIME` and runtime printk parameters can add timestamps.
- High-volume logging can perturb timing even if the call is technically allowed from many contexts.

### Dynamic-Debug Path

With dynamic debug support, `pr_debug()` and `dev_dbg()` callsites are cataloged. Runtime queries update flags for matching callsites. When a callsite is enabled with `+p`, it prints through the normal printk path.

The key idea: **dynamic debug controls whether selected debug callsites emit messages; printk controls where those emitted messages are visible.**

### Ftrace Path

Ftrace instruments selected functions or tracepoints and writes records into per-CPU trace buffers. Tracefs files control:

- what to trace;
- where to filter;
- which CPUs/tasks/functions are in scope;
- whether tracing is currently enabled;
- how much buffer space is available.

For function tracing, `set_ftrace_filter` is one of the most important safety controls. Tracing every function on a busy system can produce too much data and change timing.

### Event-Tracing Path

Trace events are predefined by kernel code. When enabled, each event instance records structured fields into the trace buffer. The `format` file shows what fields exist. The `filter` file lets you store only records that match the expression.

### Crash Capture Path

If the kernel oopses or panics, normal userspace may not be available. Common ways to preserve evidence are:

- serial console;
- pstore/ramoops;
- kdump/crash kernel;
- `ftrace_dump_on_oops`;
- persistent tracing buffers on platforms that support them.

## Key Structs And APIs

There is no single "debugging struct" that every driver instantiates. Debugging uses several kernel facilities.

### Logging APIs

| API | Use |
| --- | --- |
| `dev_err(dev, ...)` | device-scoped error |
| `dev_warn(dev, ...)` | device-scoped warning |
| `dev_info(dev, ...)` | device-scoped informational message |
| `dev_dbg(dev, ...)` | device-scoped debug message, often dynamic-debug controlled |
| `dev_err_probe(dev, err, ...)` | probe error helper, especially useful for `-EPROBE_DEFER` |
| `pr_err()` / `pr_warn()` / `pr_info()` | non-device logging |
| `pr_debug()` | non-device debug print |
| `WARN_ON()` / `WARN_ON_ONCE()` | report suspicious state and continue if possible |
| `dump_stack()` | print a stack trace for temporary diagnosis |

### Formatting Helpers

| Feature | Use |
| --- | --- |
| `pr_fmt(fmt)` | prefix all `pr_*()` messages in a file |
| `%pa` / `%pad` | physical / DMA address formatting |
| `%pM` | MAC address |
| `%pI4` / `%pI6` | IPv4 / IPv6 address |
| `%pe` | symbolic error pointer formatting on kernels that support it |

Do not use floating point formats in kernel logs.

### Tracing Interfaces

| Interface | Use |
| --- | --- |
| `tracefs` | filesystem control plane for ftrace/events |
| `TRACE_EVENT()` | define a kernel tracepoint event in subsystem code |
| `trace_printk()` | temporary trace-buffer print during debugging; do not ship |
| `trace_marker` | userspace marker inserted into trace buffer |
| `set_ftrace_filter` | narrow function tracing |
| `events/<subsystem>/<event>/enable` | enable one tracepoint event |
| `events/<subsystem>/<event>/filter` | filter one tracepoint event |

### Debug State Interfaces

| Interface | Use |
| --- | --- |
| `debugfs_create_*()` | create debug-only files |
| `debugfs_remove_recursive()` | remove debugfs files |
| `/proc/interrupts` | observe IRQ counts and names |
| `/proc/sys/kernel/printk` | console loglevel settings |
| `/proc/dynamic_debug/control` | dynamic-debug catalog and control |
| `/sys/module/<module>/parameters/` | module parameter values |

## Lifecycle / Data Flow

### Normal Driver Debug Session

1. Reproduce the failure with minimal extra instrumentation.
2. Check `dmesg` for probe errors, warnings, oopses, and device names.
3. Enable targeted dynamic debug for the suspected module/function.
4. Inspect framework state: IRQ counts, GPIO/pinctrl state, regmap dump, live DT, or subsystem-specific debug knobs.
5. Add ftrace or event tracing when the question becomes "what ran?" or "how long did it take?"
6. Clean up tracing and debug settings after the run.

### Trace Session Flow

1. Find tracefs.
2. Disable tracing.
3. Clear the trace buffer.
4. Select `nop`, `function`, or `function_graph`.
5. Apply filters before enabling tracing.
6. Enable specific events if needed.
7. Run the workload.
8. Stop tracing.
9. Read `trace` or stream `trace_pipe`.
10. Disable events and clear filters.

### Oops Flow

1. Preserve the full oops, preferably from serial, pstore, or kdump.
2. Match the oops to the exact kernel and module build.
3. Identify fault type, context, module, and faulting function offset.
4. Map function+offset to source or assembly.
5. Inspect the surrounding code for lifetime, locking, pointer validation, sleeping context, DMA, and error-path mistakes.
6. Reproduce with narrower tracing or sanitizers if the root cause is still unclear.

## Minimal Practical Example

This is a learning-only workflow. It avoids intentional crashes and uses cleanup steps so the target returns to a quiet state.

### Driver Logging Pattern

```c
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

static int demo_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int irq;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		return dev_err_probe(dev, irq, "failed to get irq\n");

	dev_dbg(dev, "probe started, irq=%d\n", irq);

	/* Temporary diagnostic: use sparingly and remove before submission. */
	if (WARN_ON_ONCE(irq == 0))
		return -EINVAL;

	dev_info(dev, "device ready\n");
	return 0;
}
```

Why this pattern works:

- `dev_err_probe()` handles ordinary probe errors and deferred probe cleanly.
- `dev_dbg()` can stay quiet until dynamic debug enables it.
- `WARN_ON_ONCE()` reports a violated assumption without spamming every call.
- `dev_info()` is reserved for a meaningful one-time state transition.

### Shell Workflow

```sh
#!/bin/sh
set -eu

TRACEFS=/sys/kernel/tracing
if [ ! -d "$TRACEFS" ]; then
	TRACEFS=/sys/kernel/debug/tracing
fi

echo "tracefs: $TRACEFS"

# Enable selected debug prints.
if [ -w /proc/dynamic_debug/control ]; then
	echo 'module my_driver +p' | sudo tee /proc/dynamic_debug/control
fi

# Prepare a narrow event trace.
echo 0 | sudo tee "$TRACEFS/tracing_on"
echo nop | sudo tee "$TRACEFS/current_tracer"
echo > "$TRACEFS/trace"
echo 1 | sudo tee "$TRACEFS/events/irq/irq_handler_entry/enable"
echo 1 | sudo tee "$TRACEFS/events/irq/irq_handler_exit/enable"

# Run the workload that should trigger the device interrupt.
echo 1 | sudo tee "$TRACEFS/tracing_on"
sleep 3
echo 0 | sudo tee "$TRACEFS/tracing_on"

sudo cat "$TRACEFS/trace" | head -100

# Cleanup.
echo 0 | sudo tee "$TRACEFS/events/irq/irq_handler_entry/enable"
echo 0 | sudo tee "$TRACEFS/events/irq/irq_handler_exit/enable"
echo nop | sudo tee "$TRACEFS/current_tracer"
echo > "$TRACEFS/set_ftrace_filter"
echo > "$TRACEFS/trace"
if [ -w /proc/dynamic_debug/control ]; then
	echo 'module my_driver -p' | sudo tee /proc/dynamic_debug/control
fi
```

For a function graph trace, narrow it first:

```sh
echo 0 | sudo tee "$TRACEFS/tracing_on"
echo function_graph | sudo tee "$TRACEFS/current_tracer"
echo my_driver_irq_thread | sudo tee "$TRACEFS/set_ftrace_filter"
echo > "$TRACEFS/trace"
echo 1 | sudo tee "$TRACEFS/tracing_on"
# run workload
echo 0 | sudo tee "$TRACEFS/tracing_on"
sudo cat "$TRACEFS/trace"
```

## Common Bugs And Debugging

| Symptom | Likely Cause | What To Check |
| --- | --- | --- |
| `dev_dbg()` never prints | dynamic debug not enabled, code path not hit, or callsite absent | `/proc/dynamic_debug/control`, `dmesg`, module name |
| Message appears in `dmesg` but not serial console | console loglevel too low | `/proc/sys/kernel/printk`, `dmesg -n` |
| Trace buffer is enormous and useless | tracing too broad | filters, CPU mask, PID filter, event selection |
| Bug disappears when logging is added | timing-sensitive race or hot-path perturbation | tracepoints, lockdep, KCSAN, smaller instrumentation |
| IRQ expected but count does not move | wrong IRQ mapping, disabled line, pinmux issue | `/proc/interrupts`, DT, pinctrl/GPIO debugfs |
| Probe fails with `-EPROBE_DEFER` | dependency not ready yet | `dev_err_probe()`, provider driver, regulator/clock/GPIO logs |
| Oops shows `foo+0x24/0x80 [foo]` | module fault at offset into function | exact `.ko`, `objdump -S`, debug info |
| Tracefs path missing | tracefs/debugfs not mounted or config disabled | mount tracefs, kernel config |
| `v4l2-compliance` prints "Not Supported" | optional ioctl/feature absent | distinguish optional skip from real failure |
| Debugfs data missing | kernel config disabled or driver did not create entries | `CONFIG_DEBUG_FS`, driver probe path |

Common traps:

- **Leaving `trace_printk()` in production code.** It is for temporary debugging.
- **Using debugfs as a stable ABI.** Use sysfs, configfs, netlink, ioctl, or subsystem ABI for production interfaces.
- **Using `BUG_ON()` for recoverable driver errors.** Prefer normal error returns or `WARN_ON_ONCE()` for diagnostics.
- **Logging secrets or raw user data.** Kernel logs are often collected broadly.
- **Forgetting cleanup.** Disable events, reset tracers, clear filters, and turn dynamic debug back off.
- **Ignoring taint flags.** They explain whether out-of-tree, forced, or proprietary code affected the crash.

## Production Checklist

Before shipping a driver:

- Use `dev_*()` messages with clear device context.
- Use `dev_err_probe()` for probe errors that may defer.
- Keep `dev_info()` sparse; avoid repeated informational logs.
- Put detailed messages behind `dev_dbg()` or tracepoints.
- Rate-limit unavoidable repeated warnings.
- Do not submit `trace_printk()`, temporary `dump_stack()`, or noisy debug parameters.
- Make debugfs optional and remove entries on teardown.
- Never treat debugfs file formats as userspace ABI.
- Document useful dynamic-debug and trace commands for bring-up teams.
- Preserve exact kernel/module symbols for postmortem analysis.
- Enable appropriate debug configs during development: lockdep, KASAN/KCSAN/KMSAN/UBSAN where feasible, kmemleak, hung-task and lockup detectors.
- Have a crash-capture plan for real boards: serial console, pstore/ramoops, kdump, or ftrace dump-on-oops.
- Verify that tracing and logs do not mask timing-sensitive bugs.
- Clean up all tracefs and dynamic-debug settings after debug sessions.

## Interview Readiness

You should be ready to answer:

- Why `dev_dbg()` may not print even when code executes.
- The difference between console loglevel and the printk ring buffer.
- When to use logs, dynamic debug, ftrace, tracepoints, and debugfs.
- How to find tracefs on old and current systems.
- How to narrow ftrace to one function, PID, CPU, or event.
- Why broad tracing can change the behavior being debugged.
- How to read `function+offset/size [module]` in an oops.
- Why `trace_printk()` and debugfs should not ship as production interfaces.
- How to debug a probe failure, missing IRQ, or crash before userspace starts.

## Kernel Version Notes

- Current kernels normally use `/sys/kernel/tracing` for tracefs. Older examples often use `/sys/kernel/debug/tracing`; many systems still expose that compatibility path.
- `pr_debug()` / `dev_dbg()` behavior depends on kernel config. With dynamic debug enabled, callsites can often be enabled at runtime instead of requiring a rebuild with `DEBUG`.
- Old descriptions that say `printk()` "never blocks" are too simple for production reasoning. Logging from atomic or IRQ context is common, but heavy console logging can still perturb timing and flood buffers.
- `ftrace_dump_on_oops` supports multiple modes on current kernels, including all-CPU and original-CPU dumps, and may support trace-instance-specific modes.
- Trace events, fields, filters, and debugfs layouts can vary by kernel version and config. Always confirm with the target's `available_events`, `format` files, `available_tracers`, and kernel config.
