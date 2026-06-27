# Topic Brief - 04 - Kernel Logging, Error Handling, And Coding Practice

## Output Targets
- Knowledge: `knowledge/04-kernel-logging-error-handling-and-coding-practice.md`
- Interview: `interview/04-kernel-logging-error-handling-and-coding-practice.md`
- Example: `examples/04-kernel-logging-error-handling-and-coding-practice/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch02` | `docs/Linux Device Driver Development/Chapter 2-Device Driver Basis.md` | read/covered/merged | Main book source for negative errno returns, syscall error propagation to userspace `errno`, cleanup labels with `goto`, `ERR_PTR()`/`IS_ERR()`/`PTR_ERR()`, `printk()` levels, `pr_*` wrappers, console log level, `/proc/sys/kernel/printk`, and atomic-context note for `printk()`. |
| `ldd1-ch05` | `docs/Linux Device Driver Development/Chapter 5-Platform Device Drivers.md` | read/mapped/adjacent | Adjacent source for real bus-driver context and `MODULE_DEVICE_TABLE()`; no direct logging/error-style section, but useful to avoid confusing module-load/probe behavior with generic error handling. |
| `ldd1-ch09` | `docs/Linux Device Driver Development/Chapter 9-Regmap API .md` | read/mapped/merged-adjacent | Concrete driver example of APIs returning `ERR_PTR()` and callers using `IS_ERR()`/`PTR_ERR()` plus `dev_err()` before returning exact errors. Detailed regmap belongs to topic 18. |
| `ldd1-ch11` | `docs/Linux Device Driver Development/Chapter 11-Kernel Memory Management.md` | read/mapped/merged-adjacent | Shows manual cleanup labels versus `devm_*` managed-resource cleanup, `dev_err()` on resource acquisition failure, and the idea that managed allocation changes error-unwind shape. Detailed memory/devres belongs to topic 20. |
| `ldd2-ch01` | `docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md` | read/mapped/adjacent | Contains scattered `pr_info()`, `pr_err()`, `IS_ERR()`/`PTR_ERR()`, and cleanup examples in wait-queue/IRQ sections; no standalone logging/error-style treatment. Use only as later-topic cross-reference. |
| `ldd2-ch02` | `docs/Linux Device Driver Development 2/Chapter 2-Regmap_API.md` | read/mapped/merged-adjacent | Additional modern-ish `ERR_PTR()`/`IS_ERR()`/`PTR_ERR()` and `dev_err()` examples around regmap/devm APIs; detailed regmap and IRQ-chip material belongs to topics 18-19. |
| `ldd2-ch11` | `docs/Linux Device Driver Development 2/Chapter 11-Writing_PCI_Device_Drivers.md` | read/mapped/adjacent | Shows PCI probe error-unwind labels, `dev_warn()`/`dev_err()`, and `WARN_ON_ONCE()` examples; detailed PCI flow belongs to topic 31. |
| `ldd2-ch14` | `docs/Linux Device Driver Development 2/Chapter 14-Linux_Kernel_Debugging_Tips.md` | read/covered/merged | Strong second-book source for kernel logging APIs: `printk()`, log levels, `pr_*`, `pr_fmt()`, `dev_*`, console log level, `dmesg -n`, `loglevel`, `KERN_CONT`/`pr_cont`, circular log buffer, `LOG_BUF_SHIFT`, `log_buf_len`, `CONFIG_PRINTK_TIME`, `/sys/module/printk/parameters/time`, and oops/logging boundaries. |
| `notion-ch01-part4` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 4 Coding Style and Best Practices .md` | read/covered/merged | Main Notion source for coding practice: tabs, line length, brace style, naming, function size, comments, include order, `static`, `checkpatch.pl`, allocation style, `devm_*`, and code-quality checklist. |
| `notion-ch02-part3` | `docs/Linux-Device-Driver-Notion/Chapter 2-Part 3 Error Handling & Message Printing.md` | read/covered/merged | Main Notion source for this topic: beginner mental model, errno selection, negative returns, userspace propagation, cleanup `goto`, `ERR_PTR()` family internals, `dev_*`, `pr_*`, console log level, `dmesg` workflows, ring buffer, timestamps, `pr_fmt()`, dynamic debug, printk format specifiers, and message best practices. |
| `notion-ch04-part2` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 2 Basic File Operations.md` | read/mapped/merged-adjacent | Concrete char-driver examples of syscall-facing errors (`-EFAULT`, `-EBUSY`, `-ENOMEM`, `-ERESTARTSYS`), `printk(KERN_*)`, `IS_ERR()`/`PTR_ERR()` around `class_create()` and `device_create()`, and cleanup labels. Detailed char devices belong to topic 07. |
| `notion-ch04-part3` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 3 Read, Write, and Seek Operations.md` | read/mapped/merged-adjacent | Concrete examples for `pr_debug()`, `pr_err()`, `pr_info()`, bounds-checking with `-EINVAL`, and manual cleanup in init/exit; detailed read/write/llseek belongs to topic 07. |
| `notion-ch05-part1` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 1 Platform Bus & Driver Basics.md` | read/mapped/adjacent | Platform-driver context for `probe()`/`remove()` return conventions and why device-bound logging should prefer `dev_*`; detailed platform flow belongs to topic 09. |
| `notion-ch06-part3` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 3 OF APIs and Platform Integration.md` | read/mapped/adjacent | Rich example usage of `dev_err()`, `dev_warn()`, `dev_info()`, `IS_ERR()`/`PTR_ERR()`, and `devm_*` cleanup in DT/platform integration. Detailed OF APIs belong to topic 11. |
| `notion-ch15-part1` | `docs/Linux-Device-Driver-Notion/Chapter 15-Part 1 GPIO Controller Architecture.md` | read/mapped/adjacent | GPIO controller examples of `dev_dbg()`, `dev_err()`, `dev_info()`, `-EIO`, `-EINVAL`, `-ENOMEM`; detailed GPIO controller work belongs to topic 14. |
| `notion-ch15-part2` | `docs/Linux-Device-Driver-Notion/Chapter 15-Part 2 IRQ Chip Integration.md` | read/mapped/adjacent | IRQ-chip examples of resource failures and `dev_err()`; detailed IRQ/GPIO integration belongs to topics 14-15. |
| `notion-ch15-part3` | `docs/Linux-Device-Driver-Notion/Chapter 15-Part 3 Advanced Features and Integration.md` | read/mapped/adjacent | Additional GPIO integration examples with cleanup labels and `dev_info()`; no unique general logging/error doctrine. |

## Source Files Read
- `docs/Linux Device Driver Development/Chapter 2-Device Driver Basis.md`
  - Relevant sections: Errors and message printing; Error handling; Handling null pointer errors; Message printing - `printk()`; module skeleton examples using `pr_info()`.
- `docs/Linux Device Driver Development/Chapter 5-Platform Device Drivers.md`
  - Relevant check: bus matching and driver/probe context; no standalone logging/error section.
- `docs/Linux Device Driver Development/Chapter 9-Regmap API .md`
  - Relevant sections: SPI initialization; I2C initialization; `regmap_init_*()` returning `ERR_PTR()`; `dev_err()` on initialization failure.
- `docs/Linux Device Driver Development/Chapter 11-Kernel Memory Management.md`
  - Relevant section: Managed device resources/devres, manual `goto` unroll versus `devm_request_irq()` automatic cleanup.
- `docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md`
  - Relevant check: scattered wait-queue/IRQ examples with `pr_info()`, `pr_err()`, `IS_ERR()`/`PTR_ERR()`, cleanup labels.
- `docs/Linux Device Driver Development 2/Chapter 2-Regmap_API.md`
  - Relevant check: regmap/devm examples using `IS_ERR()`/`PTR_ERR()`, `dev_err()`, and negative errno returns.
- `docs/Linux Device Driver Development 2/Chapter 11-Writing_PCI_Device_Drivers.md`
  - Relevant check: PCI error-unwind labels, warnings, and `dev_*` diagnostics.
- `docs/Linux Device Driver Development 2/Chapter 14-Linux_Kernel_Debugging_Tips.md`
  - Relevant sections: Linux kernel development tips; Message printing; Kernel log levels; Kernel log buffer; Adding timing information; Oops and panic analysis boundary; Trace dump on oops boundary.
- `docs/Linux-Device-Driver-Notion/Chapter 1-Part 4 Coding Style and Best Practices .md`
  - Relevant sections: Why Coding Style Matters; Essential Coding Style Rules; Comment Style; Code Organization; Static Functions; Using `checkpatch.pl`; Memory Allocation in Kernel; Best Practices Summary.
- `docs/Linux-Device-Driver-Notion/Chapter 2-Part 3 Error Handling & Message Printing.md`
  - Relevant sections: Error Handling; Standard Error Codes; When to Use Each Error Code; Returning Errors; Error Propagation to User Space; Error Handling with `goto`; Handling Null Pointer Errors; `ERR_PTR()`/`PTR_ERR()`/`IS_ERR()`; Coding Style Rules; Message Printing - `printk()`; `pr_*`; `dev_*`; console log level; viewing kernel messages; ring buffer; timestamps; `pr_fmt()`; `pr_debug()`; formatting; best practices.
- `docs/Linux-Device-Driver-Notion/Chapter 4-Part 2 Basic File Operations.md`
  - Relevant check: file-operation return values, user-copy errors, init cleanup labels, `IS_ERR()`/`PTR_ERR()` around device/class creation.
- `docs/Linux-Device-Driver-Notion/Chapter 4-Part 3 Read, Write, and Seek Operations.md`
  - Relevant check: `pr_debug()`/`pr_err()`/`pr_info()`, `-EINVAL`, cleanup in character-device init/exit.
- `docs/Linux-Device-Driver-Notion/Chapter 5-Part 1 Platform Bus & Driver Basics.md`
  - Relevant check: probe/remove return conventions and device-bound context.
- `docs/Linux-Device-Driver-Notion/Chapter 6-Part 3 OF APIs and Platform Integration.md`
  - Relevant check: numerous examples of device-scoped logging, `IS_ERR()`/`PTR_ERR()`, and managed cleanup in probe.
- `docs/Linux-Device-Driver-Notion/Chapter 15-Part 1 GPIO Controller Architecture.md`
  - Relevant check: GPIO controller `dev_dbg()`/`dev_err()`/`dev_info()` and errno examples.
- `docs/Linux-Device-Driver-Notion/Chapter 15-Part 2 IRQ Chip Integration.md`
  - Relevant check: resource-failure diagnostics and errno returns.
- `docs/Linux-Device-Driver-Notion/Chapter 15-Part 3 Advanced Features and Integration.md`
  - Relevant check: cleanup-label and logging examples.

## Merged Source Notes
- `ldd1-ch02` should be the canonical backbone for the learner-facing chapter's error-handling model: kernel APIs normally return `0` or a positive useful value on success, negative errno on failure, and syscall-facing negative errno is translated into userspace `errno`.
- `notion-ch02-part3` should be merged for beginner clarity and practical examples. It expands `ldd1-ch02` with when to choose common errno values, the userspace propagation diagram, `ERR_PTR()` internals, `dev_*` examples, `dmesg` workflows, dynamic debug commands, and printk formatting traps.
- `ldd2-ch14` should be merged for a more complete logging mechanism: `printk()` versus `pr_*`, `pr_fmt()`, `dev_*` for drivers, console filtering, the circular log buffer, boot/runtime timestamp controls, and the warning that logging is useful but not the whole tracing/debugging story.
- `notion-ch01-part4` should provide the coding-practice half of the topic: kernel style, function design, comments, include order, `static`, `checkpatch.pl`, and code-review readiness.
- Adjacent driver chapters should contribute examples, not scope expansion. Regmap, char-device, DT, platform, GPIO, PCI, and IRQ examples are valuable because they show real `IS_ERR()`/`PTR_ERR()`, cleanup labels, and `dev_err()` patterns, but their subsystem mechanics belong to their own learning-path rows.
- Managed-resource examples should be handled carefully: `devm_*` reduces manual cleanup for device-bound resources, but it does not eliminate all cleanup ordering, non-managed resources, or the need to return exact errors.

## Source Differences
- `ldd1-ch02` says `printk()` is "safe enough" from atomic context and never blocks. Current upstream documentation is more nuanced: `printk()` can be called from any context, but excessive printing in hot paths or with legacy consoles can cause lockup-like problems; use rate-limited/once macros, lower log levels, tracepoints, or deferred printing where appropriate.
- `ldd1-ch02` and Notion both teach `pr_*` as modern wrappers; `ldd2-ch14` adds the key distinction that device drivers should prefer `dev_*` when a `struct device *` is available because messages are tied to the specific device instance.
- `ldd1-ch02` names `pr_warning`; `ldd2-ch14` and current docs also show `pr_warn`, which is the shorter common form to teach.
- `ldd2-ch14` says `KERN_DEBUG` is active only if DEBUG is enabled. Current docs distinguish filtering from compilation: `pr_debug()`/`dev_dbg()` are compiled out by default unless `DEBUG` or `CONFIG_DYNAMIC_DEBUG` is enabled, while raw `printk(KERN_DEBUG ...)` is still a real printk at debug level.
- Notion's `ERR_PTR()` section shows implementation details such as `MAX_ERRNO` and address-space encoding. Learner-facing docs should teach the API contract and mention internals only as intuition; official docs say callers should treat encoded error pointers as opaque.
- Notion includes `%px` and `%pK`; current printk-format docs require care because `%p` hashes pointers by default and raw address printing can leak kernel addresses. Teach `%p`/typed formats first, and mark `%px`/`%pK` as special-case/debug or virtual-file formats.
- Notion and ldd1 use some examples with manual `kfree()` after `devm_kzalloc()` or mixed managed/manual resources. Learner-facing examples should avoid implying a `devm_*` object needs the same manual cleanup as an unmanaged object.
- The sources differ in how broad "coding practice" should be. Keep this topic focused on style that directly affects driver reliability and review: clear returns, error labels, logging quality, function naming, comments, `static`, include order, `checkpatch.pl`, and quiet drivers. Leave deep memory allocation, locking, and subsystem style to later topics.

## Gaps / Uncertainties
- Internal docs do not mention `dev_err_probe()`, which is common in modern probe paths to handle errors, log once with device context, and cooperate with deferred probe. External validation is needed before deciding whether to introduce it in the learner-facing chapter.
- Internal docs do not cover rate-limited and one-shot variants such as `pr_warn_ratelimited()`, `dev_err_ratelimited()`, `pr_info_once()`, or `dev_warn_once()` in detail. These should be externally validated if examples include logging in repeated paths.
- Internal docs do not deeply cover `PTR_ERR_OR_ZERO()`, `IS_ERR_OR_NULL()`, or `%pe` for symbolic error-pointer formatting. These are useful but should be introduced only if the chapter remains beginner-friendly.
- Internal sources do not fully explain deferred probe return values such as `-EPROBE_DEFER`; that belongs primarily to platform/device-model topics but should be cross-linked if `dev_err_probe()` is later used.
- Heavy debugging tools (`ftrace`, tracepoints, crash/oops decoding, kdump, dynamic debug workflows beyond enabling `pr_debug()`/`dev_dbg()`) should be deferred to topic 37 Kernel Debugging And Tracing.
- The source docs are based on older kernels in places. Example code should be checked against the target kernel headers before being treated as build-ready.

## External Validation
- Used official kernel documentation for `printk()` basics:
  - `https://www.kernel.org/doc/html/next/core-api/printk-basics.html`
  - Validates log levels, `pr_*`, `pr_fmt()`, `/proc/sys/kernel/printk`, `dmesg -n`, the kernel log buffer, conditional `pr_debug()`, context notes, and current warnings about excessive logging in hot paths.
- Used official kernel documentation for printk format specifiers:
  - `https://kernel.org/doc/html/next/core-api/printk-formats.html`
  - Validates pointer-format caveats, typed pointer formats, IP/MAC/resource formatting, and the need to avoid casual raw kernel address exposure.
- Used official kernel documentation for dynamic debug:
  - `https://kernel.org/doc/html/next/admin-guide/dynamic-debug-howto.html`
  - Validates that `pr_debug()`, `dev_dbg()`, `print_hex_dump_debug()`, and `print_hex_dump_bytes()` are cataloged/controllable when dynamic debug is enabled and otherwise off by default unless `DEBUG` is used.
- Used official kernel coding style:
  - `https://kernel.org/doc/html/next/process/coding-style.html`
  - Validates centralized `goto` cleanup guidance, meaningful label names, "one err bugs", simulated error-path testing, clear kernel messages, `dev_*` for device diagnostics, quiet drivers, and `pr_debug()`/`dev_dbg()` behavior.
- Used official kernel API documentation for error pointers:
  - `https://docs.kernel.org/6.11/core-api/kernel-api.html#error-pointers`
  - Validates `ERR_PTR()`, `PTR_ERR()`, `IS_ERR()`, `IS_ERR_OR_NULL()`, `ERR_CAST()`, and `PTR_ERR_OR_ZERO()` API contracts and the warning to treat encoded error pointers as opaque.

## Learning Content Brief
- Mental model:
  - Kernel error handling is a contract. The return value tells the caller exactly whether work succeeded, whether userspace should see an `errno`, and which cleanup path must run.
  - Kernel logging is a shared diagnostic channel, not stdout. Messages enter a ring buffer, may or may not appear on the console, and should help someone debug the exact failing device/path.
  - Kernel coding style is not cosmetic. It keeps review, cleanup, lifetime reasoning, and failure paths understandable in a codebase where mistakes can crash the whole system.
- Core mechanism:
  - Use negative errno for integer-returning failures, `0` for success unless the API returns a positive value such as byte count.
  - Syscall-facing driver callbacks return negative errno; userspace sees `-1` from libc and reads positive `errno`.
  - Functions returning pointers may return `NULL` only when no specific error is needed; use `ERR_PTR(-errno)`, `IS_ERR()`, and `PTR_ERR()` when preserving the failure reason matters.
  - Use cleanup labels to unwind resources in reverse acquisition order. Return directly only when nothing has to be cleaned up.
  - Use `dev_*` when a `struct device *` exists, `pr_*` for module/core messages, and raw `printk()` only when there is a specific reason.
- Important APIs/macros:
  - Error codes: `-ENOMEM`, `-EINVAL`, `-EIO`, `-ENODEV`, `-EBUSY`, `-EFAULT`, `-EAGAIN`, `-ERESTARTSYS`, `-EOPNOTSUPP`, and subsystem-specific errors as appropriate.
  - Error pointers: `ERR_PTR()`, `IS_ERR()`, `PTR_ERR()`, plus optional modern helpers `IS_ERR_OR_NULL()`, `PTR_ERR_OR_ZERO()`, and `ERR_CAST()` after validation.
  - Logging: `printk()`, `KERN_*`, `pr_err()`, `pr_warn()`, `pr_notice()`, `pr_info()`, `pr_debug()`, `pr_fmt()`, `dev_err()`, `dev_warn()`, `dev_info()`, `dev_dbg()`, `pr_cont()`/`KERN_CONT` with caution.
  - Debug controls: `dmesg`, `dmesg -w`, `dmesg -n`, `/proc/sys/kernel/printk`, `loglevel=`, `log_buf_len=`, `CONFIG_PRINTK_TIME`, `/sys/module/printk/parameters/time`, `/sys/kernel/debug/dynamic_debug/control`.
  - Style/review: `scripts/checkpatch.pl`, kernel-doc comments, `static`, `EXPORT_SYMBOL()`, include order, 8-column tabs, clear function names.
- Lifecycle/data flow:
  - Probe/init path acquires resources step by step; each failure path returns the original error and unwinds already-acquired resources.
  - Runtime callbacks report expected recoverable conditions with the right errno rather than logging every normal event.
  - Remove/exit path releases resources in reverse order and should match the successful initialization path.
  - Logs flow from `printk()`/`pr_*`/`dev_*` into the kernel log buffer; console visibility depends on log level and console log level; userspace reads via `dmesg`, `/dev/kmsg`, or journal tools.
- Example direction:
  - A minimal learning-only module or tiny platform/char-driver-shaped example should intentionally fail several setup steps and demonstrate: exact negative errno returns, one early return with no cleanup, one cleanup-label chain, one `ERR_PTR()` helper, `dev_err()`/`pr_info()` usage, and `dmesg`/dynamic-debug inspection.
  - The example should not become a full char-device or platform-driver lesson; it should focus on failure-path readability and diagnostics.
- Common bugs:
  - Returning the wrong errno or losing the original error by replacing it with `-EINVAL` or `-ENODEV`.
  - Returning positive errno values from kernel code.
  - Forgetting cleanup on one failure path or using one generic `err:` label that frees resources that were never acquired.
  - Calling `PTR_ERR()` on a `NULL` pointer or using `IS_ERR()` when an API returns `NULL` on failure.
  - Logging with no context, wrong severity, no newline, excessive messages in hot paths, or `pr_info()` spam in normal operation.
  - Using `%p`/`%px` casually and leaking kernel addresses.
  - Mixing managed and unmanaged resources without understanding which cleanup is automatic.
  - Ignoring `checkpatch.pl` findings or writing style that hides control flow.
- Debugging notes:
  - First commands after a failure: `dmesg -w`, `dmesg -l err,warn`, `cat /proc/sys/kernel/printk`, and dynamic debug controls for `pr_debug()`/`dev_dbg()` callsites.
  - Check whether the message exists in the ring buffer even if it did not appear on the console.
  - Use log levels to separate real errors from debug breadcrumbs.
  - For repeated callbacks, prefer rate limiting or dynamic debug rather than unbounded prints.
  - For crashes/oops, collect the log and defer full oops decoding/tracing to topic 37.
- Production concerns:
  - Drivers should be mostly quiet when healthy.
  - Error messages should include the operation, affected resource/register/device, and the return code when useful.
  - Cleanup paths should be testable; simulate allocation/resource failures when possible.
  - Preserve userspace ABI behavior: choose errno values intentionally for file operations and ioctls.
  - Use `devm_*` to simplify device-bound cleanup where appropriate, but understand lifetime and ordering.
  - Follow kernel coding style before review; `checkpatch.pl` is a helper, not a substitute for reading similar in-tree drivers.
- Interview angles:
  - Explain why kernel code returns `-ENOMEM`, not `ENOMEM`.
  - Explain how a driver's `return -EFAULT` becomes userspace `errno == EFAULT`.
  - Explain when to return `NULL` versus `ERR_PTR(-errno)` from a pointer-returning helper.
  - Explain the `IS_ERR()`/`PTR_ERR()` call pattern and a common misuse.
  - Explain why Linux drivers commonly use `goto` for cleanup and what "one err bugs" are.
  - Explain `printk()` log levels and why `pr_info()` may be visible in `dmesg` but not on the console.
  - Explain when to use `dev_err()` instead of `pr_err()`.
  - Explain why `pr_debug()`/`dev_dbg()` may not print and how dynamic debug changes that.
  - Explain why logging in interrupt or high-frequency paths can be dangerous.
  - Explain what `checkpatch.pl` catches and what human review still needs to verify.
