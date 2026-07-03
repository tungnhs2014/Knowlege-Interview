# 37 - Kernel Debugging And Tracing Interview Questions

This chapter tests whether you can choose the right debugging tool, keep instrumentation safe, and reason from logs, traces, and crash output back to driver code.

## Beginner Questions

### 1. What is kernel debugging and tracing?

**Short Answer**

Kernel debugging and tracing are the tools and workflows used to observe kernel behavior, diagnose driver bugs, and analyze crashes using logs, dynamic debug, tracefs/ftrace, tracepoints, debugfs, and oops output.

**Deep Explanation**

Kernel code cannot be debugged like an ordinary userspace process in many real board bring-up situations. A driver bug can crash the whole system, happen in interrupt context, or disappear when timing changes. Logging gives explicit messages from code. Dynamic debug enables hidden debug callsites. Ftrace and tracepoints record runtime flow and timing. Oops analysis helps after a fault has already happened.

**API / Code Anchor**

- `dev_err()`, `dev_warn()`, `dev_info()`, `dev_dbg()`
- `pr_err()`, `pr_warn()`, `pr_info()`, `pr_debug()`
- `/proc/dynamic_debug/control`
- `/sys/kernel/tracing` or `/sys/kernel/debug/tracing`
- `events/<subsystem>/<event>/enable`

**Production or Debugging Angle**

Good driver debugging is layered. Start with existing logs, enable targeted debug output, inspect subsystem state, and only then broaden into function tracing or crash capture.

**Common Traps**

- Treating `printk()` as the only debugging tool.
- Tracing the whole kernel first and drowning in noise.
- Forgetting that debug output can change timing.

**Follow-up Questions**

- When would you choose tracepoints over logs?
- Why can kernel debugging be harder than userspace debugging?
- What evidence would you collect before changing code?

### 2. What is the difference between `printk()`, `pr_*()`, and `dev_*()`?

**Short Answer**

`printk()` is the low-level logging primitive. `pr_*()` wraps printk for general kernel code. `dev_*()` is preferred in drivers because it attaches device context to the message.

**Deep Explanation**

`pr_err()` and friends are convenient for code without a `struct device`. Driver code usually has a device pointer, so `dev_err(dev, ...)` is better because the log includes which device emitted the message. This matters when one driver instance binds to multiple devices. `dev_err_probe()` is especially useful in probe paths because it handles deferred-probe logging cleanly.

**API / Code Anchor**

```c
dev_err(dev, "read failed: %d\n", ret);
dev_dbg(dev, "irq=%d\n", irq);
pr_info("core state changed\n");
```

**Production or Debugging Angle**

Use `dev_*()` for driver messages, choose severity carefully, and avoid noisy `dev_info()` in paths that run repeatedly.

**Common Traps**

- Using raw `printk()` everywhere.
- Logging without device identity.
- Printing repeated hot-path messages at info or warning level.

**Follow-up Questions**

- Why is `dev_err_probe()` useful?
- When is raw `printk()` still reasonable?
- How would you make repeated warnings less noisy?

### 3. Why does `dev_dbg()` not print anything by default?

**Short Answer**

`dev_dbg()` is a debug-level callsite. Depending on kernel config, it may be compiled out, disabled, or controlled by dynamic debug. It usually needs runtime enabling before it emits messages.

**Deep Explanation**

Debug prints are intentionally quiet to avoid log spam and timing perturbation. With dynamic debug support, the callsites can be listed and enabled through `/proc/dynamic_debug/control`. After enabling, output still goes through printk, so it may appear in `dmesg` without appearing on the console.

**API / Code Anchor**

```sh
grep my_driver /proc/dynamic_debug/control
echo 'module my_driver +p' | sudo tee /proc/dynamic_debug/control
dmesg -w
```

**Production or Debugging Angle**

Leave useful `dev_dbg()` statements in code for bring-up, but keep them targeted and meaningful. They are far better than adding temporary noisy `dev_info()` lines.

**Common Traps**

- Watching only the serial console instead of `dmesg`.
- Enabling the wrong module name.
- Assuming the callsite exists without checking the catalog.

**Follow-up Questions**

- How do you enable one function's debug prints?
- What does `+p` mean in dynamic debug?
- Why might enabled debug still not show output?

### 4. What is the difference between console loglevel and the printk ring buffer?

**Short Answer**

The printk ring buffer stores log messages. Console loglevel controls which stored messages are immediately printed to consoles.

**Deep Explanation**

A debug or info message can exist in the ring buffer even if it never appears on the serial console. `dmesg` reads the ring buffer. `/proc/sys/kernel/printk` shows loglevel settings, and `dmesg -n` can change console verbosity.

**API / Code Anchor**

```sh
cat /proc/sys/kernel/printk
dmesg -l err,warn,info
sudo dmesg -n 7
```

**Production or Debugging Angle**

When a message is "missing," check both the callsite and the buffer. Do not increase console verbosity permanently on production systems just to see debug noise.

**Common Traps**

- Thinking console silence means the message was never logged.
- Increasing console loglevel and accidentally changing timing.
- Forgetting the ring buffer is finite and circular.

**Follow-up Questions**

- How can you increase the ring buffer size?
- Why are timestamps useful in kernel logs?
- What happens when the buffer wraps?

## Mid-level Questions

### 5. How do you run a safe ftrace session?

**Short Answer**

Disable tracing, clear the buffer, select a tracer, apply filters, enable only needed events or functions, reproduce the issue, stop tracing, read the buffer, and clean up.

**Deep Explanation**

Ftrace writes to per-CPU trace buffers. If you enable a broad tracer without filters, the data can be huge and the overhead can change behavior. A safe session narrows by function, event, PID, or CPU before enabling tracing.

**API / Code Anchor**

```sh
TRACEFS=/sys/kernel/tracing
[ -d "$TRACEFS" ] || TRACEFS=/sys/kernel/debug/tracing

echo 0 | sudo tee "$TRACEFS/tracing_on"
echo nop | sudo tee "$TRACEFS/current_tracer"
echo > "$TRACEFS/trace"
echo 1 | sudo tee "$TRACEFS/events/irq/irq_handler_entry/enable"
echo 1 | sudo tee "$TRACEFS/tracing_on"
# reproduce
echo 0 | sudo tee "$TRACEFS/tracing_on"
sudo cat "$TRACEFS/trace"
```

**Production or Debugging Angle**

Always leave the system clean: disable events, clear filters, reset `current_tracer` to `nop`, and clear temporary dynamic-debug settings.

**Common Traps**

- Using the wrong tracefs path.
- Forgetting to clear old filters.
- Leaving tracing enabled after the experiment.

**Follow-up Questions**

- What is the difference between `trace` and `trace_pipe`?
- How do you restrict tracing to one function?
- How do you restrict tracing to one CPU?

### 6. Compare function tracing, function graph tracing, and tracepoint events.

**Short Answer**

Function tracing records function entries. Function graph tracing records call nesting and durations. Tracepoint events record structured subsystem events at predefined instrumentation points.

**Deep Explanation**

Use `function` when you need to know whether a function ran. Use `function_graph` when call hierarchy and duration matter. Use tracepoints when the subsystem already exposes meaningful events, such as IRQ, scheduler, block, timer, or networking events. Tracepoints are often lower-noise and more semantically useful than broad function tracing.

**API / Code Anchor**

```sh
echo function > $TRACEFS/current_tracer
echo my_driver_* > $TRACEFS/set_ftrace_filter

echo function_graph > $TRACEFS/current_tracer

echo 1 > $TRACEFS/events/sched/sched_switch/enable
```

**Production or Debugging Angle**

Prefer subsystem tracepoints when they answer the question. They are stable instrumentation points compared with tracing large function sets.

**Common Traps**

- Using `function_graph` broadly on a busy system.
- Ignoring tracepoint `format` files before writing filters.
- Forgetting that not every kernel is built with every tracer.

**Follow-up Questions**

- How do you find supported tracers?
- How do event filters work?
- Why might a tracepoint be better than a temporary log?

### 7. How would you debug a platform driver's probe failure?

**Short Answer**

Start with `dmesg`, use `dev_err_probe()` output, check resource acquisition errors, inspect Device Tree and provider devices, then enable targeted dynamic debug or trace relevant subsystem events.

**Deep Explanation**

Probe failures often come from missing clocks, regulators, GPIOs, IRQs, wrong `compatible`, bad `reg`, or provider drivers not ready. `-EPROBE_DEFER` is not always a bug in the consumer; it may mean dependency ordering. Live Device Tree inspection and subsystem debugfs state help verify what the kernel actually parsed.

**API / Code Anchor**

```c
irq = platform_get_irq(pdev, 0);
if (irq < 0)
	return dev_err_probe(dev, irq, "failed to get irq\n");
```

```sh
dmesg -w
find /proc/device-tree -name '*my-device*'
cat /proc/interrupts
```

**Production or Debugging Angle**

Do not hide probe failures behind generic `dev_err(dev, "failed\n")`. Include the resource name and error code, and let deferred probe remain visible but not noisy.

**Common Traps**

- Treating every `-EPROBE_DEFER` as a fatal driver bug.
- Debugging source DTS while the board booted a different DTB.
- Forgetting pinctrl or GPIO polarity.

**Follow-up Questions**

- What does `dev_err_probe()` do for deferred probe?
- How would you confirm the live DT?
- Which tracepoints might help with driver binding?

### 8. How do you debug an interrupt that never seems to fire?

**Short Answer**

Check `/proc/interrupts`, confirm the IRQ number/name, verify pinmux/GPIO/trigger configuration, check hardware status registers, and trace IRQ entry/exit events if counts move but the driver still misbehaves.

**Deep Explanation**

An interrupt problem can be electrical, Device Tree, pinctrl, GPIO controller, IRQ domain, trigger type, handler registration, masking, or status-clear logic. `/proc/interrupts` tells whether the kernel sees interrupts. GPIO and pinctrl debugfs can reveal a wrong pin state. Regmap or MMIO dumps can show whether the device asserted or latched an interrupt.

**API / Code Anchor**

```sh
cat /proc/interrupts
cat /sys/kernel/debug/gpio
echo 1 > $TRACEFS/events/irq/irq_handler_entry/enable
echo 1 > $TRACEFS/events/irq/irq_handler_exit/enable
```

**Production or Debugging Angle**

Give requested IRQs meaningful names. A good IRQ name makes `/proc/interrupts` useful during board bring-up.

**Common Traps**

- Looking only at driver logs while IRQ count never changes.
- Clearing interrupt status too early or too late.
- Using the wrong edge/level trigger.

**Follow-up Questions**

- How can level-triggered IRQs livelock?
- Why does IRQ affinity matter?
- How would threaded IRQs change debugging?

### 9. How do you interpret `my_probe+0x24/0x80 [my_driver]` in an oops?

**Short Answer**

It means the fault happened 0x24 bytes into function `my_probe`, whose compiled size is 0x80 bytes, in module `my_driver`.

**Deep Explanation**

The symbol and offset give the starting point for postmortem analysis. You must match the oops to the exact kernel and module build. With the right `.ko`, `vmlinux`, `System.map`, and debug info, you can map the offset to source or assembly using tools such as `objdump` and `addr2line`.

**API / Code Anchor**

```sh
objdump -S --line-numbers my_driver.ko | less
addr2line -e vmlinux 0xffffffff81234567
```

**Production or Debugging Angle**

Archive symbols for every kernel and module shipped to devices. Without matching symbols, oops logs become much less useful.

**Common Traps**

- Using a rebuilt module that does not match the crashed target.
- Ignoring taint flags.
- Looking only at the faulting line and not the caller-owned lifetime or locking contract.

**Follow-up Questions**

- What files do you archive for crash analysis?
- What does the module name in brackets mean?
- How do taint flags affect debugging?

## Senior Questions

### 10. A board has intermittent 200 ms latency spikes in an IRQ-driven driver. How do you investigate?

**Short Answer**

Use low-noise evidence first: IRQ counts, targeted event tracing, latency tracers if available, function graph tracing on suspected handlers/workqueues only, and lock/sleep debug options. Avoid broad logging in the hot path.

**Deep Explanation**

Latency spikes can come from long IRQ-disabled sections, threaded IRQ scheduling delay, workqueue congestion, locks, slow register access, power-management transitions, or console logging. Broad `dev_info()` in the handler can make the problem worse. Start with tracepoints for IRQ, scheduler, and workqueue behavior; then narrow function tracing to the driver handler or thread.

**API / Code Anchor**

```sh
cat /proc/interrupts
cat $TRACEFS/available_tracers
echo irqsoff > $TRACEFS/current_tracer      # if available
echo my_driver_irq_thread > $TRACEFS/set_ftrace_filter
echo function_graph > $TRACEFS/current_tracer
```

**Production or Debugging Angle**

Instrumentation must not become the bug. For timing-sensitive issues, prefer trace buffers and existing tracepoints over console logging.

**Common Traps**

- Printing from every interrupt.
- Enabling `function_graph` globally on a busy SMP system.
- Ignoring CPU affinity and preemption/IRQ-disabled sections.

**Follow-up Questions**

- How would you distinguish handler time from scheduling delay?
- What kernel debug configs help with sleep-in-atomic or locking bugs?
- How would you size trace buffers?

### 11. The board crashes before userspace starts. What evidence plan do you use?

**Short Answer**

Use serial console, early kernel logs, larger log buffer, pstore/ramoops or kdump if available, and preconfigured ftrace dump-on-oops. Preserve exact symbols for postmortem analysis.

**Deep Explanation**

Before userspace, tools such as `dmesg` and shell commands may not be available. The boot command line and kernel config matter. A serial console can capture early messages. `log_buf_len=` can prevent early messages from wrapping. Pstore/ramoops can preserve logs across reboot. `ftrace_dump_on_oops` can dump trace buffers when the crash happens.

**API / Code Anchor**

```text
console=ttyS0,115200 log_buf_len=4M ignore_loglevel
```

```sh
echo 1 | sudo tee /proc/sys/kernel/ftrace_dump_on_oops
```

**Production or Debugging Angle**

Crash capture must be designed before the crash. If the board resets and loses RAM logs, you need persistent storage or an external console.

**Common Traps**

- Waiting until after the crash to enable tracing.
- Losing logs because the buffer wrapped.
- Forgetting that the booted image and symbols must match.

**Follow-up Questions**

- When would you use pstore/ramoops?
- What does `ignore_loglevel` help with?
- What are the risks of dumping huge trace buffers on serial console?

### 12. Why should `trace_printk()` not be submitted in production driver code?

**Short Answer**

`trace_printk()` is for temporary debugging. It adds trace-buffer formatting overhead and is not intended as a permanent driver interface or normal instrumentation method.

**Deep Explanation**

Permanent instrumentation should use meaningful logs, existing tracepoints, or custom tracepoints when justified. `trace_printk()` is quick during local debugging because it avoids console spam and writes to the trace buffer, but it is easy to leave behind and can affect performance or trigger warnings.

**API / Code Anchor**

```c
trace_printk("state=%x\n", state); /* temporary only */
```

Better long-term choices:

```c
dev_dbg(dev, "state=%x\n", state);
/* or subsystem-defined TRACE_EVENT() when real trace instrumentation is needed */
```

**Production or Debugging Angle**

Remove `trace_printk()` before code review. If the information is valuable long term, design a proper tracepoint or debug message.

**Common Traps**

- Treating `trace_printk()` as a faster `dev_dbg()`.
- Leaving temporary tracing in submitted patches.
- Using trace text as a userspace ABI.

**Follow-up Questions**

- When is a custom tracepoint justified?
- How is `trace_marker` different?
- Why can trace-buffer output still perturb timing?

### 13. Is debugfs a production ABI?

**Short Answer**

No. Debugfs is for debugging and diagnostics. Its layout and contents are not stable userspace ABI.

**Deep Explanation**

Drivers and subsystems often expose internal state through debugfs because it is convenient and low-friction. That is useful for bring-up, but applications must not depend on it. Production ABI belongs in the subsystem's supported userspace interface, such as sysfs attributes with documented ABI, netlink, ioctl, configfs, or subsystem-specific device nodes.

**API / Code Anchor**

```c
debugfs_create_u32("state", 0444, priv->debugfs_dir, &priv->state);
debugfs_remove_recursive(priv->debugfs_dir);
```

**Production or Debugging Angle**

Debugfs files should not expose secrets, unsafe hardware controls, or formats that product software depends on. Remove entries during driver teardown.

**Common Traps**

- Shipping scripts that parse debugfs as a stable interface.
- Creating writable debugfs knobs that can break hardware state.
- Forgetting cleanup on probe failure or remove.

**Follow-up Questions**

- When would sysfs be more appropriate?
- How do you clean up debugfs entries?
- What permissions should debugfs files use?

### 14. You enabled dynamic debug and ftrace but still cannot see the bug. What next?

**Short Answer**

Re-check the hypothesis and scope: confirm the code path runs, confirm filters match, inspect subsystem state, collect crash evidence, and enable specialized debug tools such as lockdep, KASAN/KCSAN/KMSAN/UBSAN, kmemleak, or hung-task/lockup detectors when relevant.

**Deep Explanation**

Logs and traces only show what you configured them to show. If the bug is memory corruption, a race, sleeping in atomic context, bad lifetime, DMA coherency, or use-after-free, sanitizers and lock debugging may find the root cause faster than more prints. If the system crashes, postmortem evidence and exact symbols become more important.

**API / Code Anchor**

- `CONFIG_PROVE_LOCKING`
- `CONFIG_DEBUG_ATOMIC_SLEEP`
- `CONFIG_KASAN`
- `CONFIG_KCSAN`
- `CONFIG_KMSAN`
- `CONFIG_UBSAN`
- `CONFIG_DEBUG_KMEMLEAK`
- hung-task and lockup detector configs

**Production or Debugging Angle**

Development kernels should use heavier debug configs than production kernels. Production builds need low overhead but should preserve enough crash evidence for field diagnosis.

**Common Traps**

- Adding more logs instead of testing a better hypothesis.
- Running sanitizers on a workload too different from the failure.
- Ignoring lifetime and ownership because the crash appears in an innocent function.

**Follow-up Questions**

- Which tool would you choose for use-after-free?
- Which tool helps with data races?
- How would you debug a sleeping-in-atomic warning?
