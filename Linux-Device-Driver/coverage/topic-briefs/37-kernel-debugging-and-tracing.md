# Topic Brief - 37 - Kernel Debugging And Tracing

## Output Targets

| Learning Path | Slug | Requested Brief Path | Canonical Brief Path | Knowledge Target | Interview Target | Example Target |
| --- | --- | --- | --- | --- | --- | --- |
| 37 - Kernel Debugging And Tracing | `kernel-debugging-and-tracing` | `coverage/topic-briefs/cc.md` | `coverage/topic-briefs/37-kernel-debugging-and-tracing.md` | `knowledge/37-kernel-debugging-and-tracing.md` | `interview/37-kernel-debugging-and-tracing.md` | `examples/37-kernel-debugging-and-tracing/README.md` |

Note: the user explicitly requested `coverage/topic-briefs/cc.md`; the canonical learning-path brief path is recorded above for the later coverage update.

## Source Coverage

| Source ID | Source Root | File | Status | Key Contribution |
| --- | --- | --- | --- | --- |
| `ldd1-source-root` | ldd1 | `docs/Linux Device Driver Development/` | `searched/mapped/gap` | No dedicated ftrace, tracepoint, dynamic-debug, oops-analysis, or general kernel-debugging chapter exists in book 1. Relevant material is distributed across logging, memory fault, IRQ exception, Device Tree, GPIO, regmap, input, framebuffer, and network chapters. |
| `ldd1-ch02` | ldd1 | `Chapter 2-Device Driver Basis.md` | `read/mapped/covered-adjacent` | Logging baseline: `printk()`, log levels, `pr_*` wrappers, console loglevel, `/proc/sys/kernel/printk`, `dmesg`, dynamic debug mention, and debug-oriented module parameters. Overlaps strongly with `ldd2-ch14` and Notion Chapter 2 Part 3. |
| `ldd1-ch06` | ldd1 | `Chapter 6-The Concept of a Device Tree .md` | `read/mapped/related` | Runtime DT inspection through `/proc/device-tree` / `CONFIG_PROC_DEVICETREE`; useful as a debugging workflow for probe and binding issues, but detailed DT remains topics 10-11. |
| `ldd1-ch09` | ldd1 | `Chapter 9-Regmap API .md` | `read/mapped/related` | Register I/O failure examples and regmap sanity checks; book 2 has the clearer regmap debugfs dump. Use as adjacent register-debug context only. |
| `ldd1-ch11` | ldd1 | `Chapter 11-Kernel Memory Management.md` | `read/mapped/related` | Fault model: virtual memory, page faults, SIGSEGV for userspace, unresolved faults, and page faults in interrupt context leading to fatal behavior. Supports oops/panic reasoning. |
| `ldd1-ch14` | ldd1 | `Chapter 14-Pin Control and GPIO Subsystem.md` | `read/mapped/related` | GPIO and pinctrl debugging through `/sys/kernel/debug/gpio`; useful as an example of subsystem debugfs state inspection. |
| `ldd1-ch15` | ldd1 | `Chapter 15-GPIO Controller Drivers.md` | `read/mapped/related` | Mentions optional GPIO debugfs dump hooks and naming used in debug output; adjacent only. |
| `ldd1-ch16` | ldd1 | `Chapter 16-Advanced IRQ Management.md` | `read/mapped/related` | Exception/trap background, debug exception entry, kernel traps causing panic, IRQ names visible in `/proc/interrupts`, and IRQ-context debugging implications. Full IRQ internals remain topic 15. |
| `ldd1-ch17` | ldd1 | `Chapter 17- Input Devices Drivers.md` | `read/mapped/related` | `dev_dbg()` and `/sys/kernel/debug/gpio` button-state inspection examples. Adjacent subsystem-specific debug workflow. |
| `ldd1-ch21` | ldd1 | `Chapter 21-Framebuffer Drivers.md` | `read/mapped/related` | `pr_debug()` around framebuffer blanking and mode changes; adjacent example of using debug prints in driver operations. |
| `ldd1-ch22` | ldd1 | `Chapter 22-Network Interface Card Drivers.md` | `read/mapped/related` | `dmesg` verification and `printk()` examples during fake NIC load/open; adjacent only because netdev-specific debugging remains topic 30. |
| `ldd2-source-root` | ldd2 | `docs/Linux Device Driver Development 2/` | `searched/mapped/covered` | Contains the primary dedicated kernel-debugging chapter plus subsystem-specific debug material in regmap and V4L2 chapters. |
| `ldd2-ch02` | ldd2 | `Chapter 2-Regmap_API.md` | `read/mapped/covered-adjacent` | `debugfs` regmap register dump under `/sys/kernel/debug/regmap/...`; concrete subsystem debugfs workflow. |
| `ldd2-ch09` | ldd2 | `Chapter 9-Leveraging the V4L2.md` | `read/mapped/covered-adjacent` | V4L2 debugging from userspace: vb2 debug module parameters, `/sys/class/video4linux/video0/dev_debug`, `dmesg` traces of V4L2 ioctls, and `v4l2-compliance`. Useful as a framework-specific debug pattern. |
| `ldd2-ch14` | ldd2 | `Chapter 14-Linux_Kernel_Debugging_Tips.md` | `read/mapped/covered/merged-primary` | Primary source. Covers kernel release context, `printk()`/`pr_*`/`dev_*`, log levels, console loglevel, log buffer sizing, timestamps, ftrace, tracefs/debugfs controls, tracers, function/function_graph tracing, latency tracers, filters, tracepoints/events, per-PID tracing, oops/panic analysis, ftrace dump on oops, `objdump`, and source/assembly correlation. |
| `notion-source-root` | notion | `docs/Linux-Device-Driver-Notion/` | `searched/mapped/gap` | No standalone Notion chapter dedicated to ftrace/tracepoints/oops exists. Notion was searched and not skipped; logging/error and subsystem debug snippets were read separately. |
| `notion-ch01-part4` | notion | `Chapter 1-Part 4 Coding Style and Best Practices .md` | `read/mapped/related` | Logging style context: line length, context-rich messages, and coding-style impact on debuggability. Adjacent only; detailed coding style is topic 04. |
| `notion-ch02-part1` | notion | `Chapter 2-Part 1 User Space vs Kernel Space & Module Concept.md` | `read/mapped/related` | Kernel-space bugs can crash the whole system; user-pointer misuse examples. Useful beginner motivation for why kernel debugging differs from userspace debugging. |
| `notion-ch02-part2` | notion | `Chapter 2-Part 2 Module Dependencies, Loading & Information.md` | `read/mapped/related` | Module load/unload diagnostics, `dmesg`, `modinfo`, module parameters, dangerous forced unload caveat. Adjacent to debugging module lifecycle. |
| `notion-ch02-part3` | notion | `Chapter 2-Part 3 Error Handling & Message Printing.md` | `read/mapped/covered/merged` | Strong overlap with `ldd1-ch02` and `ldd2-ch14`: errno propagation, `ERR_PTR()`/`IS_ERR()`/`PTR_ERR()`, cleanup labels, `printk()`, `pr_*`, `dev_*`, console loglevel, `dmesg`, ring buffer, timestamps, `pr_fmt()`, `pr_debug()`, dynamic debug, printk format specifiers, and logging best practices. |
| `notion-ch02-part4` | notion | `Chapter 2-Part 4 Module Parameters & Building Your First Mod.md` | `read/mapped/related` | Debug-level module parameters, runtime sysfs parameter changes, `modinfo`, and `dmesg` checks. Useful as a contrast to dynamic debug and framework debug knobs. |
| `notion-ch03-part2` | notion | `Chapter 3-Part 2 Data Structures & Synchronization.md` | `read/mapped/related` | `pr_debug()` examples around lock acquisition and busy paths; lockdep struct mention. Deep locking remains topic 06. |
| `notion-ch03-part3` | notion | `Chapter 3-Part 3 Work Queues and Scheduling.md` | `read/mapped/related` | `CONFIG_LOCKDEP` mention in workqueue internals and debug prints in deferred work examples. Adjacent to debugging async behavior. |
| `notion-ch04-part1` | notion | `Chapter 4-Part 1 Character Device Registration.md` | `read/mapped/related` | `dmesg`-based module/device creation checks. Adjacent only. |
| `notion-ch04-part2` | notion | `Chapter 4-Part 2 Basic File Operations.md` | `read/mapped/related` | Crash examples from direct user-pointer access and diagnostic `printk()` in file operations. Adjacent ABI debug context. |
| `notion-ch04-part3` | notion | `Chapter 4-Part 3 Read, Write, and Seek Operations.md` | `read/mapped/related` | Practical `pr_debug()` in read/write/llseek paths; useful for demonstrating high-volume log hygiene. |
| `notion-ch04-part4` | notion | `Chapter 4-Part 4 Advanced Features - Poll and IOCTL.md` | `read/mapped/related` | `pr_debug()` around poll/read/write/ioctl behavior; adjacent ABI debug context. |
| `notion-ch05-part1` | notion | `Chapter 5-Part 1 Platform Bus & Driver Basics.md` | `read/mapped/related` | `dmesg` probe/remove checks for platform devices; adjacent practical workflow. |
| `notion-ch05-part2` | notion | `Chapter 5-Part 2 Probe, Remove & Resource Management.md` | `read/mapped/related` | `dmesg` probe/resource checks; adjacent only. |
| `notion-ch06-part1` | notion | `Chapter 6-Part 1 Device Tree Fundamentals.md` | `read/mapped/related` | Runtime Device Tree debugging, compiled-DT debugging, and debug-console context. Detailed DT remains topics 10-11. |
| `notion-ch07-extra-part2` | notion | `Chapter 7-v4l2_part2.md` | `read/mapped/covered-adjacent` | Dynamic debug commands for V4L2 core/vb2/specific drivers and V4L2 debug command checklist. Overlaps with `ldd2-ch09`; read separately and compared. |
| `notion-ch09-extra-part2` | notion | `Chapter 9-userspace-part2.md` | `read/mapped/covered-adjacent` | V4L2 userspace debug workflow: enable vb2 debug, `dev_debug`, `dmesg`, `v4l2-compliance`, and troubleshooting checklist. Overlaps with `ldd2-ch09`; read separately and compared. |
| `notion-ch14-part1` | notion | `Chapter 14-Part 1 Pin Control Framework.md` | `read/mapped/related` | Pinctrl debugging/verification section; adjacent subsystem-specific state inspection. |
| `notion-ch15-part1` | notion | `Chapter 15-Part 1 GPIO Controller Architecture.md` | `read/mapped/related` | GPIO provider debugfs support and `dev_dbg()` examples. Adjacent only. |
| `notion-ch16-part1` | notion | `Chapter 16-Part 1 IRQ Architecture and Propagation.md` | `read/mapped/related` | "Trace a real interrupt" walkthrough, hwirq-to-virq flow, and `dev_dbg()` mapping examples; useful for trace reasoning, but full IRQ internals remain topic 15. |
| `kernel-doc-dynamic-debug` | external/official | `https://docs.kernel.org/admin-guide/dynamic-debug-howto.html` | `read/mapped/validation` | Validated current dynamic-debug behavior, query language, `/proc/dynamic_debug/control`, and boot/module `dyndbg` caveats. |
| `kernel-doc-ftrace` | external/official | `https://docs.kernel.org/trace/ftrace.html` | `read/mapped/validation` | Validated current ftrace purpose, tracefs location, `/sys/kernel/tracing`, backward-compatible `/sys/kernel/debug/tracing`, and current tracing file model. |
| `kernel-doc-events` | external/official | `https://docs.kernel.org/trace/events.html` | `read/mapped/validation` | Validated event tracing filters, filter expression syntax, event `format` files, and trigger concepts beyond the internal source. |
| `kernel-doc-trace-debugging` | external/official | `https://docs.kernel.org/trace/debugging.html` | `read/mapped/validation` | Validated crash tracing guidance, `ftrace_dump_on_oops`, ring buffer sizing, and persistent tracing buffers. |
| `kernel-doc-driver-debugging` | external/official | `https://docs.kernel.org/process/debugging/driver_development_debugging_guide.html` | `read/mapped/validation` | Validated current driver-debugging tool categories: `printk()`, `trace_printk`, `dev_dbg`, custom tracepoints, ftrace, debugfs, KASAN, UBSAN, lockdep, PSI, and device coredump. |
| `kernel-doc-sysctl-kernel` | external/official | `https://docs.kernel.org/admin-guide/sysctl/kernel.html` | `read/mapped/validation` | Validated current `/proc/sys/kernel/ftrace_dump_on_oops` values including all-CPU, original-CPU, and instance-specific modes. |
| `kernel-doc-sysrq` | external/official | `https://docs.kernel.org/admin-guide/sysrq.html` | `read/mapped/validation` | Validated current SysRq debugging access, `/proc/sys/kernel/sysrq`, and `/proc/sysrq-trigger` behavior. |

## Source Files Read

- `Linux-Device-Driver/CODEX.md`
- `Linux-Device-Driver/LEARNING_PATH.md`
- `Linux-Device-Driver/.agents/skills/linux-device-driver/SKILL.md`
- `Linux-Device-Driver/.agents/skills/linux-device-driver/references/topic-brief-template.md`
- `Linux-Device-Driver/.codex/agents/lld-topic-explorer.toml`
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 2-Device Driver Basis.md`
  - Message printing: `printk()`, log levels, `pr_*`, `/proc/sys/kernel/printk`, dynamic debug mention, module parameters.
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 6-The Concept of a Device Tree .md`
  - Runtime DT exposure through procfs for debugging.
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 9-Regmap API .md`
  - Register access checks and adjacent register-debug context.
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 11-Kernel Memory Management.md`
  - Faults, unresolved page faults, interrupt-context fault/panic caveats.
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 14-Pin Control and GPIO Subsystem.md`
  - `/sys/kernel/debug/gpio` and GPIO labels.
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 15-GPIO Controller Drivers.md`
  - Optional debugfs dump method and debug naming.
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 16-Advanced IRQ Management.md`
  - Exceptions, traps, panic behavior, debug exception, IRQ names in `/proc/interrupts`.
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 17- Input Devices Drivers.md`
  - `dev_dbg()` and GPIO debugfs checks.
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 21-Framebuffer Drivers.md`
  - `pr_debug()` in framebuffer operation paths.
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 22-Network Interface Card Drivers.md`
  - `dmesg` and `printk()` checks during network driver bring-up.
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 2-Regmap_API.md`
  - Regmap debugfs register dump.
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 9-Leveraging the V4L2.md`
  - V4L2/vb2 debug module parameters, `dev_debug`, ioctl traces, compliance.
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 14-Linux_Kernel_Debugging_Tips.md`
  - Full primary chapter: logging, ftrace, tracepoints, event tracing, filters, per-PID tracing, oops/panic, ftrace dump on oops, objdump.
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 1-Part 4 Coding Style and Best Practices .md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 2-Part 1 User Space vs Kernel Space & Module Concept.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 2-Part 2 Module Dependencies, Loading & Information.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 2-Part 3 Error Handling & Message Printing.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 2-Part 4 Module Parameters & Building Your First Mod.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 3-Part 2 Data Structures & Synchronization.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 3-Part 3 Work Queues and Scheduling.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 4-Part 1 Character Device Registration.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 4-Part 2 Basic File Operations.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 4-Part 3 Read, Write, and Seek Operations.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 4-Part 4 Advanced Features - Poll and IOCTL.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 5-Part 1 Platform Bus & Driver Basics.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 5-Part 2 Probe, Remove & Resource Management.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 6-Part 1 Device Tree Fundamentals.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 7-v4l2_part2.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 9-userspace-part2.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 14-Part 1 Pin Control Framework.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 15-Part 1 GPIO Controller Architecture.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 16-Part 1 IRQ Architecture and Propagation.md`
- Official docs listed in External Validation.

## Merged Source Notes

### Debugging Mental Model

- Kernel debugging is different from userspace debugging because a bad kernel pointer, sleeping-in-atomic-context bug, IRQ bug, or resource-lifetime bug can destabilize the whole machine, not only one process.
- The first debugging layer is usually controlled logging:
  - use `dev_*()` for device drivers so messages include the device identity;
  - use `pr_*()` for core/non-device code;
  - reserve raw `printk()` for low-level or special cases;
  - choose severity correctly;
  - keep hot-path logging disabled by default.
- The second layer is runtime-selective visibility:
  - dynamic debug enables `pr_debug()` / `dev_dbg()` callsites by module, file, function, line, format, or class;
  - module parameters and framework debug knobs can be useful but should not become a private ABI or noisy production default;
  - subsystem debugfs/sysfs knobs are often the fastest way to inspect current framework state.
- The third layer is tracing:
  - ftrace/function tracer answers "which kernel functions ran and how long did they take?";
  - trace events answer "which predefined subsystem events happened?";
  - event filters and PID/CPU/function filters reduce noise;
  - trace buffers give pre-crash context if configured before the failure.
- The fourth layer is postmortem analysis:
  - interpret oops fields: fault reason, CPU, PID/comm, taint, PC/IP, LR/caller, stack trace, registers, code bytes, loaded module name;
  - use symbols, `addr2line`, `objdump -S`, and debug info to map offsets to source;
  - use serial console, pstore/ramoops, kdump, or ftrace dump-on-oops when the normal log buffer is lost.

### Logging And Message Buffer

- `printk()` messages have severity levels from emergency to debug. Lower numeric value means higher priority.
- Console output is controlled by `console_loglevel`, while the ring buffer still stores messages that are not immediately printed.
- `cat /proc/sys/kernel/printk` exposes current, default, minimum, and boot-time console loglevel values.
- `dmesg -n <level>` or writing to `/proc/sys/kernel/printk` changes console output, not whether a callsite exists.
- The ring buffer is finite and circular. Boot parameter `log_buf_len=` and config `CONFIG_LOG_BUF_SHIFT` influence log capacity.
- Timestamps can be enabled through `CONFIG_PRINTK_TIME` and runtime `printk.time` parameter where available.
- Notion adds practical detail missing from the primary source:
  - `dmesg -w`, `dmesg -l`, `dmesg --time-format=iso`, and `dmesg -C`;
  - `pr_fmt()` for consistent prefixes;
  - no floating-point formats in kernel logs;
  - kernel-specific format specifiers such as `%pa`, `%pad`, `%pr`, `%pM`, `%pI4`, `%pI6`;
  - avoid sensitive data and excessive hot-path logs.

### Dynamic Debug

- Internal book sources only mention dynamic debug briefly and use older documentation names.
- Current official docs validate that dynamic debug provides a catalog and query language for debug print callsites.
- Useful selectors:
  - `module <name>`
  - `file <path>`
  - `func <function>`
  - `line <range>`
  - `format <string>`
  - class selectors where modules declare debug classes.
- Useful flags:
  - `+p` enable printing;
  - `-p` disable printing;
  - additional flags can include metadata depending on kernel support.
- Output still goes through printk rules. If dynamic debug is enabled but nothing appears on console, check `dmesg`, console loglevel, and whether the callsite exists in the dynamic-debug catalog.

### Ftrace / Tracefs

- `ldd2-ch14` is v4.19-oriented and uses `/sys/kernel/debug/tracing` as the main path.
- Current official docs state the primary tracefs mount is `/sys/kernel/tracing`; `/sys/kernel/debug/tracing` remains a backward-compatible path when debugfs is mounted.
- Important files:
  - `available_tracers`
  - `current_tracer`
  - `tracing_on`
  - `trace`
  - `trace_pipe`
  - `events/`
  - `available_events`
  - `set_ftrace_filter`
  - `set_ftrace_notrace`
  - `set_ftrace_pid`
  - `tracing_cpumask`
  - `buffer_size_kb`
  - `tracing_thresh`
- Tracers to teach:
  - `nop`: event-only or trace_printk-oriented baseline.
  - `function`: function-entry tracing.
  - `function_graph`: call graph and duration tracing.
  - `irqsoff`, `preemptoff`, `preemptirqsoff`: latency tracers when available.
  - `wakeup`, `wakeup_rt`, `wakeup_dl`: scheduler wakeup latency tracers when available.
- Ftrace overhead depends heavily on tracer, filter scope, CPU mask, and event volume. The lesson should teach "narrow first, then broaden".

### Tracepoints And Events

- Tracepoints are statically placed instrumentation sites in kernel code; event tracing exposes many of them through tracefs under `events/<subsystem>/<event>/`.
- `ldd2-ch14` shows timer event tracing with:
  - disable tracing;
  - clear trace;
  - set `current_tracer` to `nop`;
  - enable an event group;
  - run workload;
  - disable tracing;
  - inspect `trace`.
- Official event docs add current filter syntax:
  - numeric comparisons: `==`, `!=`, `<`, `<=`, `>`, `>=`, `&`;
  - string comparisons: `==`, `!=`, `~`;
  - filters are written to individual event `filter` files or subsystem-level filter files;
  - malformed filters show errors in the `filter` file.
- Later learner docs should cover `trace_marker` / `trace_printk()` carefully:
  - `trace_printk()` is useful for temporary debugging, but should not ship in production driver code.

### Crash / Oops Analysis

- Oops output is architecture-dependent but recurring clues include:
  - reason line such as NULL dereference, invalid opcode, page fault, use-after-free, BUG/WARN;
  - CPU, PID, command, taint flags, kernel version;
  - PC/IP and caller/LR offsets;
  - stack trace;
  - register dump;
  - code bytes around faulting instruction;
  - module name in brackets.
- If symbols are missing or not decoded, `CONFIG_KALLSYMS`, `System.map`, `vmlinux`, module `.ko`, and debug info determine how well addresses map to source.
- `objdump -S module.ko` and `addr2line` can map symbol+offset to source/assembly if built with debug info.
- `ftrace_dump_on_oops` can dump the tracing ring buffer on oops/panic. Current docs validate modes beyond the old 0/1 view:
  - `0`: disabled;
  - `1`: all CPUs;
  - `2` / `orig_cpu`: CPU that triggered the oops;
  - trace instance modes.
- Shrink or size trace buffers intentionally before crash capture, especially on serial consoles.

### Subsystem Debug Workflows

- Regmap:
  - mount debugfs;
  - inspect `/sys/kernel/debug/regmap/<bus-device>/registers`;
  - useful for checking cached/actual register state and access tables.
- V4L2:
  - enable vb2 debug module parameters;
  - enable `/sys/class/video4linux/videoX/dev_debug`;
  - use `v4l2-ctl` and `v4l2-compliance`;
  - compare ioctl flow, buffer queueing, streamon/streamoff, and driver logs.
- GPIO/pinctrl:
  - inspect `/sys/kernel/debug/gpio` and pinctrl debugfs where available;
  - compare expected labels, directions, active-low interpretation, and pinmux state.
- IRQ:
  - inspect `/proc/interrupts`, IRQ names, counts, affinity, and whether counts move when hardware fires.
- Device Tree:
  - inspect `/proc/device-tree` or sysfs firmware/devicetree paths when enabled;
  - compare live DT with expected binding and probe logs.

## Source Differences

- `ldd2-ch14` uses `/sys/kernel/debug/tracing` as the main ftrace path. Current official docs prefer `/sys/kernel/tracing`; `/sys/kernel/debug/tracing` is backward-compatible when debugfs auto-mounts tracefs.
- `ldd1-ch02` and `ldd2-ch14` say `pr_debug()` / `pr_devel()` are empty unless `DEBUG` is enabled. Current dynamic-debug behavior is more nuanced: with `CONFIG_DYNAMIC_DEBUG`, `pr_debug()` / `dev_dbg()` callsites can be cataloged and enabled at runtime.
- `ldd1-ch02` says `printk()` never blocks and is safe enough in atomic context. Learner docs should be careful: logging from atomic/hard-IRQ context is common but high-volume console logging can still perturb timing, flood buffers, or interact with modern printk implementation details. Use rate limiting and tracepoints for hot paths.
- `ldd2-ch14` includes release-process material. Keep only the debug-relevant parts in topic 37; detailed kernel-source and release model belongs mostly to topics 01-02.
- Notion Chapter 2 Part 3 is stronger than book sources for beginner-friendly logging examples and formatting best practices, but it does not cover ftrace/oops deeply.
- V4L2 sources overlap:
  - `ldd2-ch09` provides the older book workflow and concrete output;
  - `notion-ch07-extra-part2` adds dynamic-debug commands for V4L2/vb2/source files;
  - `notion-ch09-extra-part2` adds a checklist.
  These are examples of subsystem debugging, not the core of topic 37.
- There is no Notion equivalent of `ldd2-ch14`; Notion material is distributed and should not be treated as a duplicate.
- Same-number chapter collision was avoided:
  - `ldd1-ch14` is pinctrl/GPIO;
  - `ldd2-ch14` is kernel debugging;
  - `notion-ch14-part1` is pinctrl;
  These were read/mapped separately and are not equivalent.

## Gaps / Uncertainties

- Internal sources do not deeply cover current dynamic debug classes, boot-time `dyndbg=` syntax, module-specific `<module>.dyndbg=`, or the difference between `/proc/dynamic_debug/control` and debugfs dynamic-debug paths on different kernels.
- Internal sources do not deeply cover:
  - `trace-cmd` / KernelShark;
  - perf/ftrace relationship;
  - bpftrace/eBPF tracing;
  - kprobes, kretprobes, fprobes, uprobes;
  - function graph return values on newer kernels;
  - histogram triggers and synthetic events;
  - pstore/ramoops details;
  - kdump/crash setup;
  - KGDB/KDB;
  - KASAN, KMSAN, KCSAN, UBSAN, lockdep, kmemleak, DEBUG_ATOMIC_SLEEP, PROVE_LOCKING;
  - hung task, soft lockup, hard lockup, RCU stall diagnostics.
- Internal examples include intentionally crashing oops modules. Learner-facing example should avoid shipping a crash module unless explicitly marked dangerous and optional; a safer example can demonstrate dynamic debug, tracefs, and trace events without panicking the system.
- The final example should not require privileged destructive actions. If using SysRq or forced crashes, mark commands as optional/dangerous and prefer non-crashing trace workflows.
- The originally requested brief path was noncanonical (`cc.md`). The canonical learning-path brief now exists at `coverage/topic-briefs/37-kernel-debugging-and-tracing.md`; keep `cc.md` only as the original requested alias if historical trace is needed.

## External Validation

External validation was used because `ldd2-ch14` is v4.19-era, tracing paths changed around tracefs, and dynamic debug behavior is version-sensitive.

| External Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/admin-guide/dynamic-debug-howto.html` | Current dynamic debug query language, catalog, control file, and output/loglevel caveat. |
| `https://docs.kernel.org/trace/ftrace.html` | Current ftrace role, tracefs path `/sys/kernel/tracing`, backward-compatible debugfs path, tracer controls, and event-tracing framing. |
| `https://docs.kernel.org/trace/events.html` | Current event filtering syntax, event format files, subsystem filters, and trigger concepts. |
| `https://docs.kernel.org/trace/debugging.html` | Current crash-debug tracing guidance, `ftrace_dump_on_oops`, `trace_buf_size`, and persistent buffers. |
| `https://docs.kernel.org/process/debugging/driver_development_debugging_guide.html` | Current driver-debugging tool taxonomy: `printk()`, `trace_printk`, `dev_dbg`, ftrace, debugfs, KASAN, UBSAN, lockdep, PSI, and device coredump. |
| `https://docs.kernel.org/admin-guide/sysctl/kernel.html` | Current `/proc/sys/kernel/ftrace_dump_on_oops` modes and related kernel debug sysctls. |
| `https://docs.kernel.org/admin-guide/sysrq.html` | Current SysRq enablement and `/proc/sysrq-trigger` behavior. |

External validation still useful before final example:

- Check the local target kernel for `CONFIG_DYNAMIC_DEBUG`, `CONFIG_FUNCTION_TRACER`, `CONFIG_FUNCTION_GRAPH_TRACER`, `CONFIG_TRACEPOINTS`, `CONFIG_TRACEFS`, `CONFIG_KALLSYMS`, `CONFIG_KASAN`, `CONFIG_KCSAN`, `CONFIG_LOCKDEP`, and `CONFIG_MAGIC_SYSRQ`.
- Check whether local tracefs is mounted at `/sys/kernel/tracing`, `/sys/kernel/debug/tracing`, or both.
- Check local dynamic-debug control path and supported flags/classes before writing runnable commands.

## Learning Content Brief

### What To Teach

- What kernel debugging and tracing are:
  - logging for explicit driver messages;
  - dynamic debug for runtime-selective debug callsites;
  - ftrace/tracepoints for runtime behavior and timing;
  - debugfs/sysfs/procfs for subsystem state;
  - oops/panic analysis for failures after the fact.
- Why they exist:
  - kernel code cannot rely on normal userspace debuggers in many embedded situations;
  - bugs may happen in IRQ context, atomic context, boot, suspend/resume, or hardware timing windows;
  - a driver must often be debugged on a remote board with only serial console, `dmesg`, and tracefs.
- When to use each tool:
  - `dev_err()` / `dev_err_probe()` for probe failures;
  - `dev_dbg()` / dynamic debug for optional details;
  - ftrace function_graph for latency and call-flow questions;
  - trace events for subsystem behavior without recompilation;
  - subsystem debugfs for current state snapshots;
  - oops decoding when the machine already failed;
  - sanitizers/lockdep when reproducing classes of memory/locking bugs.
- How the kernel implements it:
  - printk ring buffer and console loglevel;
  - dynamic-debug catalog of callsites;
  - tracefs control files, per-CPU ring buffers, tracers, event enable files, filters, triggers;
  - debugfs entries exposed by subsystems;
  - symbolization through kallsyms/System.map/vmlinux/module debug info.

### Key APIs / Files / Commands

- Kernel logging APIs:
  - `printk()`
  - `pr_emerg()`, `pr_alert()`, `pr_crit()`, `pr_err()`, `pr_warn()`, `pr_notice()`, `pr_info()`, `pr_debug()`
  - `dev_emerg()`, `dev_alert()`, `dev_crit()`, `dev_err()`, `dev_warn()`, `dev_notice()`, `dev_info()`, `dev_dbg()`
  - `dev_err_probe()`
  - `pr_fmt()`
  - `print_hex_dump_debug()`
  - `trace_printk()` as temporary debug only
- Error/debug helpers adjacent to logging:
  - `IS_ERR()`, `PTR_ERR()`, `ERR_PTR()`, `IS_ERR_OR_NULL()`
  - `WARN_ON()`, `WARN_ON_ONCE()`, `WARN_ONCE()`, `BUG_ON()` only with strong caveats
  - `dump_stack()`
  - `might_sleep()` and debug configs such as `CONFIG_DEBUG_ATOMIC_SLEEP`
- Runtime files:
  - `/proc/sys/kernel/printk`
  - `/sys/module/printk/parameters/time`
  - `/proc/dynamic_debug/control`
  - `/sys/kernel/debug/dynamic_debug/control` on systems exposing debugfs path
  - `/sys/kernel/tracing/`
  - `/sys/kernel/debug/tracing/`
  - `/proc/interrupts`
  - `/sys/kernel/debug/regmap/.../registers`
  - `/sys/kernel/debug/gpio`
  - `/sys/class/video4linux/videoX/dev_debug`
  - `/proc/sys/kernel/ftrace_dump_on_oops`
  - `/proc/sysrq-trigger`
  - `/sys/fs/pstore` when pstore is configured
- ftrace files:
  - `available_tracers`
  - `current_tracer`
  - `tracing_on`
  - `trace`
  - `trace_pipe`
  - `set_ftrace_filter`
  - `set_ftrace_notrace`
  - `set_ftrace_pid`
  - `tracing_cpumask`
  - `events/*/*/enable`
  - `events/*/*/filter`
  - `buffer_size_kb`
  - `tracing_thresh`

### Lifecycle / Workflow

1. Reproduce the symptom with the smallest workload.
2. Capture baseline:
   - kernel version/config;
   - module load/unload logs;
   - `dmesg -w`;
   - relevant `/proc`, sysfs, debugfs state.
3. Check obvious binding/lifecycle failures:
   - probe error code;
   - `-EPROBE_DEFER`;
   - missing resources;
   - wrong IRQ/DMA/clock/regulator/DT binding.
4. Enable targeted logging:
   - dynamic debug by module/file/function;
   - framework debug knob if available.
5. Add tracing:
   - event tracing for subsystem events;
   - function_graph with narrow filters for call flow and latency;
   - PID/CPU filters when reproducing from one process.
6. Analyze:
   - timestamps and ordering;
   - return codes;
   - stack traces;
   - event sequence;
   - hardware state snapshots.
7. If the system crashes:
   - capture serial console, pstore, or kdump;
   - decode oops symbol+offset;
   - correlate with trace buffer if `ftrace_dump_on_oops` was enabled.
8. Clean up:
   - disable dynamic debug;
   - clear event enables and filters;
   - restore console loglevel;
   - unmount trace/debugfs if the lab mounted them.

### Practical Example Direction

Recommended example for later step:

- Learning-only README and optional shell snippets, no intentional crashing module by default.
- A small safe module may include:
  - `dev_info()`, `dev_dbg()`, `trace_printk()` demonstration only if clearly marked temporary;
  - module parameter to emit a few events;
  - optional `debugfs` counter/state file if the example wants to teach debugfs lifetime.
- Better minimal example may be README-only:
  - enable dynamic debug for an existing loaded module;
  - trace a safe event group such as `sched`, `irq`, `timer`, `gpio`, or `module`;
  - apply event filters;
  - use `function_graph` with `set_ftrace_filter`;
  - inspect and reset trace state.
- Avoid shipping a module that dereferences NULL unless the example is explicitly "dangerous crash lab" and opt-in.

### Common Bugs / Debugging Traps

- Expecting `dev_dbg()` to print without dynamic debug, `DEBUG`, or loglevel setup.
- Enabling dynamic debug but reading only the console, while messages are in `dmesg`.
- Using `/sys/kernel/debug/tracing` on a system where tracefs is mounted only at `/sys/kernel/tracing`.
- Forgetting to clear ftrace filters/events after a debug session.
- Tracing too broadly and perturbing timing or losing events.
- Printing in hot IRQ/NAPI/timer paths and changing the bug's timing.
- Leaving `trace_printk()` in production code.
- Treating `v4l2-compliance` "Not Supported" lines as failures.
- Reading oops PC/IP offset without considering module load address and symbolization.
- Ignoring taint flags from out-of-tree modules or forced unloads.
- Using `BUG_ON()` for recoverable driver errors.
- Exposing production debugfs files as if they were stable userspace ABI.

### Production Checklist

- Logs:
  - use `dev_*()` for device context;
  - errors include resource name, return code, and enough state to act;
  - normal operation is not noisy;
  - hot-path debug is disabled or rate-limited.
- Dynamic debug:
  - callsites are meaningful;
  - no private debug module parameter duplicates dynamic-debug behavior without a reason;
  - production docs include how to enable targeted debug.
- Traceability:
  - key subsystem tracepoints are known;
  - ftrace examples include cleanup commands;
  - trace buffer sizing is considered for crash capture.
- Crash readiness:
  - serial console, pstore/ramoops, or kdump plan exists for hard failures;
  - debug symbols and exact module `.ko` / `vmlinux` are archived for symbolization.
- Debugfs:
  - debugfs is optional and non-ABI;
  - entries are removed on driver teardown;
  - no secrets or unsafe hardware-mutating knobs are exposed without protection.
- Safety:
  - avoid intentional panics in normal examples/tests;
  - never use debug code to mask a race;
  - verify cleanup after tracing sessions.

### Interview Angles

- Explain the difference between `printk()`, `pr_*()`, and `dev_*()`.
- Explain why `dev_dbg()` may not print and how dynamic debug changes that.
- Explain the difference between console loglevel and the kernel ring buffer.
- Explain ftrace as a tracing framework, not only a function tracer.
- Explain `function` versus `function_graph` versus event tracing.
- Explain tracefs path differences across kernels.
- Explain how to trace only one function, one PID, one CPU, or one event group.
- Explain how to debug a probe failure that only says `-EPROBE_DEFER`.
- Explain how to decode an oops line such as `foo_probe+0x24/0x80 [foo]`.
- Explain why `trace_printk()` should not be left in submitted code.
- Explain why debugfs is not a stable userspace ABI.
- Senior scenario: intermittent IRQ latency; choose between logs, ftrace latency tracers, event filters, and `/proc/interrupts`.
- Senior scenario: board crashes before shell; choose serial console, early printk/boot loglevel, pstore/ramoops, ftrace dump on oops, and kdump according to what survives reboot.
