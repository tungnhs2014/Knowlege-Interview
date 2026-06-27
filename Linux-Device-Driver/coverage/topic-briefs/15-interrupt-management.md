# Topic Brief - 15 - Interrupt Management

## Output Targets
- Knowledge: `knowledge/15-interrupt-management.md`
- Interview: `interview/15-interrupt-management.md`
- Example: `examples/15-interrupt-management/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch03` | `docs/Linux Device Driver Development/Chapter 3-Kernel Facilities and Helper Functions.md` | read/mapped/covered/merged | Core interrupt-consumer source: `request_irq()`, `free_irq()`, handler prototype/returns, hard-IRQ locking, top half/bottom half split, softirq/tasklet/workqueue/threaded IRQ choices, `IRQF_ONESHOT`. |
| `ldd1-ch06` | `docs/Linux Device Driver Development/Chapter 6-The Concept of a Device Tree .md` | read/mapped/covered | DT IRQ consumer/controller properties, `platform_get_irq()`, `request_irq()`, `interrupt-parent`, `interrupts`, `#interrupt-cells`, GIC example, resource extraction from DT. |
| `ldd1-ch15` | `docs/Linux Device Driver Development/Chapter 15-GPIO Controller Drivers.md` | read/mapped/related | Related GPIO controller context: `gpio_chip.to_irq`, `can_sleep`, IRQ-capable GPIO controllers, `interrupt-controller`/`#interrupt-cells` in GPIO controller nodes. |
| `ldd1-ch16` | `docs/Linux Device Driver Development/Chapter 16-Advanced IRQ Management.md` | read/mapped/covered/merged | Primary advanced IRQ source: interrupt controllers, `irq_chip`, `irq_domain`, hwirq/virq mapping, `irq_desc`, `irqaction`, request propagation, chained vs nested interrupts, GPIO irqchip, DT interrupt-controller binding. |
| `ldd2-ch01` | `docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md` | read/mapped/covered/merged | Updated generic interrupt-management source: trigger flags, `IRQ_NONE`/`IRQ_HANDLED`/`IRQ_WAKE_THREAD`, hard IRQ constraints, `devm_request_irq()`/`devm_free_irq()`, threaded IRQs, `request_any_context_irq()`, workqueue bottom halves, locking rules. |
| `ldd2-ch11` | `docs/Linux Device Driver Development 2/Chapter 11-Writing_PCI_Device_Drivers.md` | read/mapped/related | Bus-specific interrupt extension: PCI INTx vs MSI/MSI-X, shared legacy lines, message-based interrupts, unique MSI vectors, PCI interrupt allocation context. |
| `notion-ch03-part4` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 4 Work Queues and Scheduling.md` | read/mapped/covered/merged | Expanded tasklet/bottom-half source: tasklet model, scheduling, enable/disable/kill lifecycle, tasklet vs workqueue comparison, legacy-status warning. |
| `notion-ch06-part3` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 3 OF APIs and Platform Integration.md` | read/mapped/covered | DT/platform integration example with `platform_get_irq()`, `devm_request_irq()`, IRQ status clear, resource-managed setup, named IRQ resources, DT pitfalls. |
| `notion-ch15-part2` | `docs/Linux-Device-Driver-Notion/Chapter 15-Part 2 IRQ Chip Integration.md` | read/mapped/covered/merged | Overlapping IRQ-chip material: GPIO as interrupt controller, IRQ multiplexing mental model, IRQ domains, chained/nested comparison, legacy and gpiolib irqchip implementation. |
| `notion-ch15-part3` | `docs/Linux-Device-Driver-Notion/Chapter 15-Part 3 Advanced Features and Integration.md` | read/mapped/related | Related complete MCP23016-style GPIO expander with nested IRQ, DT interrupt support, `devm_request_threaded_irq()`, `handle_nested_irq()`, resource-managed patterns. |
| `notion-ch16-part1` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 1 IRQ Architecture and Propagation.md` | read/mapped/covered/merged | Expanded IRQ architecture: three-layer model, hwirq vs virq, full propagation path from hardware through IRQ domain and flow handler, request registration internals, IRQ-domain API. |
| `notion-ch16-part2` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 2 IRQ Multiplexing - Chained and Nested Inter.md` | read/mapped/covered/merged | Detailed chained/nested source: when to use each, `generic_handle_irq()`, `handle_nested_irq()`, `irq_set_chained_handler_and_data()`, `chained_irq_enter()/exit()`, parent/child IRQ examples. |
| `notion-ch16-part3` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 3 Advanced Topics.md` | read/mapped/covered/merged | Threaded IRQ, regmap IRQ, DT IRQ binding, interrupt parsing APIs, approach-selection decision tree, and common pitfalls such as sleeping in hard IRQ and missing ack. |

## Source Files Read
- `ldd1-ch03`: "Work deferring mechanism", "Softirqs and ksoftirqd", "Tasklets", "Work queues", "Kernel interruption mechanism", "Registering an interrupt handler", "Interrupt handler and lock", "Concept of bottom halves", "Tasklets as bottom halves", "Workqueue as bottom halves", "Softirqs as bottom half", "Threaded IRQs", and "Threaded bottom half".
- `ldd1-ch06`: "Handling interrupts", "The interrupt handler", "Interrupt controller code", and "Platform resources and DTs".
- `ldd1-ch15`: "Driver architecture and data structures", `to_irq`, `can_sleep`, "Pin controller guidelines", and "GPIO controllers and the DT".
- `ldd1-ch16`: complete chapter, especially interrupt controller structures, IRQ domain creation/mapping/xlate, `irq_desc`/`irqaction`/`irq_data`, request propagation, chained/nested interrupts, GPIO irqchip case study, gpiolib irqchip helpers, and DT binding.
- `ldd2-ch01`: "Linux kernel interrupt management" through "Locking from within an interrupt handler", including flags/return values, threaded IRQ handlers, `request_any_context_irq()`, workqueue bottom half, and locking rules.
- `ldd2-ch11`: "Interrupt distribution", "PCI legacy INT-X-based interrupts", "Message-based interrupt type - MSI and MSI-X", "MSI mechanism", "MSI-X mechanism", "Legacy INTx emulation", and "Dealing with interrupts".
- `notion-ch03-part4`: complete tasklet source through best practices.
- `notion-ch06-part3`: "Complete Real-World Driver Example" and "Best Practices and Common Pitfalls", plus IRQ-related OF/platform notes.
- `notion-ch15-part2`: complete file.
- `notion-ch15-part3`: DT integration and complete MCP23016 driver sections.
- `notion-ch16-part1`: complete file.
- `notion-ch16-part2`: complete file.
- `notion-ch16-part3`: complete file.

## Merged Source Notes
- The learner-facing mental model should start with why interrupts exist: devices notify the CPU about events without constant polling. The CPU enters an architecture-specific interrupt path, the interrupt controller identifies a hardware source, the IRQ core maps that source to a Linux IRQ, and the registered driver handler handles device-specific work.
- Linux separates controller-local hardware IRQ numbers (`hwirq`) from global Linux IRQ numbers (`virq`/IRQ). Drivers request Linux IRQ numbers; interrupt-controller drivers and IRQ domains translate from local hardware numbering to Linux-visible IRQs.
- `request_irq()` registers a hard-IRQ handler. Current sources agree it is effectively the non-threaded form of `request_threaded_irq()` with `thread_fn = NULL`.
- The handler type is `irqreturn_t (*)(int irq, void *dev_id)`. `IRQ_NONE` means "not my interrupt" and matters for shared lines. `IRQ_HANDLED` means the device interrupt was handled. `IRQ_WAKE_THREAD` wakes the threaded handler registered with `request_threaded_irq()`.
- `dev_id` is not optional bookkeeping. It is passed back to the handler, identifies the action on shared IRQs, and must match when freeing an IRQ. For shared IRQs, each handler must use a unique non-NULL cookie and must check whether its device really asserted the line.
- Hard IRQ context is atomic: do not sleep, do not use mutexes, do not call blocking bus APIs, do not copy to/from userspace, avoid `GFP_KERNEL`, avoid long loops, and keep stack use small. Quick register reads/writes, status capture, device-level ack/clear, and wake/defer decisions belong here.
- Bottom halves exist to move non-urgent work out of the hard IRQ path. The sources cover softirqs, tasklets, workqueues, and threaded IRQs. For modern driver teaching, emphasize threaded IRQs and workqueues; tasklets should be marked legacy even though existing drivers still use them.
- Threaded IRQs split handling into optional primary hard handler plus `thread_fn`. The hard handler does quick validation/ack and returns `IRQ_WAKE_THREAD`; the thread runs in process context and can sleep, take mutexes, perform I2C/SPI/regmap operations, and do longer work.
- If `request_threaded_irq()` is called with `handler = NULL` and `thread_fn != NULL`, the IRQ core installs a default primary handler that wakes the thread. The sources agree that `IRQF_ONESHOT` is required for thread-only handlers so the IRQ remains masked until the thread completes.
- A level-triggered interrupt represents a state and will retrigger if the device-level cause is not cleared. An edge-triggered interrupt represents a transition and can be missed if the line never returns inactive or if shared edge behavior is misdesigned.
- `free_irq()`/`devm_free_irq()` remove the handler and wait for executing handlers to finish. Before freeing, the driver should make the device stop generating interrupts to avoid spurious IRQs during teardown.
- Locking depends on contexts sharing the data. Hard IRQ plus process context requires spinlock variants that disable local interrupts in process context, typically `spin_lock_irqsave()`. Threaded-only/shared process-context data can use mutexes. Softirq/tasklet shared data uses spinlocks or `_bh` variants depending on the other contexts.
- `irq_chip` models interrupt-controller hardware operations: mask, unmask, ack, end-of-interrupt, set type, set wake, set affinity, retrigger, and bus lock/sync unlock for slow controllers.
- `irq_desc` is the IRQ core's descriptor for a Linux IRQ. It holds IRQ data, statistics, a flow handler, and the `irqaction` list. `irqaction` records registered handlers, flags, `dev_id`, and optional thread handler/thread state.
- `irq_domain` maps local `hwirq` numbers to Linux IRQ numbers. `irq_domain_add_linear()` fits small dense controller-local ranges such as GPIO banks; tree mapping fits sparse/large spaces. `irq_domain_ops.map()` binds a virq to an `irq_chip` and flow handler; `.xlate()` decodes firmware/DT interrupt specifiers.
- Chained IRQs are for fast, non-sleeping controllers such as MMIO SoC GPIO blocks. The parent handler is installed with chaining APIs, runs in hard IRQ context, reads a status register, maps active child hwirqs to virqs, and calls `generic_handle_irq()`/domain variants.
- Nested IRQs are for slow or sleepable controllers such as I2C/SPI GPIO expanders. The parent IRQ is requested as threaded, the parent thread reads status over the bus, then calls `handle_nested_irq()` for active child virqs.
- GPIO IRQ material overlaps with topic 14. For topic 15, teach it as an example of interrupt-controller hierarchy and multiplexing, not as a full GPIO-controller lesson.
- DT interrupt binding has two sides. Controller nodes advertise `interrupt-controller` and `#interrupt-cells`; consumer nodes use `interrupt-parent` and `interrupts`. Platform drivers usually obtain Linux IRQ numbers with `platform_get_irq()` or named variants, then request them with `devm_request_irq()` or `devm_request_threaded_irq()`.
- PCI interrupt material is related but bus-specific. Include a short note that PCI can use shared legacy INTx or message-based MSI/MSI-X vectors; defer PCI-specific allocation and vector APIs to topic 31.

## Source Differences
- `ldd1-ch03` contains older tasklet API examples using `void (*func)(unsigned long)` and `tasklet_init()`/`DECLARE_TASKLET()` forms. Current kernels have moved tasklets toward deprecation and API churn; final examples should prefer workqueues or threaded IRQs unless explicitly teaching legacy code.
- `ldd1-ch03` says a disabled tasklet corresponds to `count = 0` and nonzero means enabled, but the surrounding macro examples show `ATOMIC_INIT(0)` for enabled and `ATOMIC_INIT(1)` for disabled. Use the macro behavior: count zero means enabled, nonzero means disabled.
- `ldd1-ch03` suggests "more than 100 us" as a threshold for deferring IRQ work; `ldd2-ch01` frames the threshold more flexibly and also uses "half a jiffy" as a rough warning. The final lesson should avoid a rigid universal number and teach "minimal hard handler; defer anything slow/sleeping".
- `ldd1-ch16` and Notion Chapter 16 duplicate many IRQ-domain concepts, but Notion adds clearer hwirq/virq diagrams, full propagation flow, implementation steps, DT binding examples, and common pitfalls.
- `notion-ch15-part2` includes a table row that implies `IRQF_ONESHOT` under chained interrupts. Treat that as misleading: chained parent handlers are installed with chained IRQ APIs, while `IRQF_ONESHOT` is for threaded/nested parent handlers requested through `request_threaded_irq()`.
- Several sources show older gpiolib irqchip helper style (`gpiochip_irqchip_add[_nested]()` and `gpiochip_set_*_irqchip()`). Current kernel GPIO docs indicate the old post-registration path is deprecated for chained gpio irqchips and prefer configuring `struct gpio_chip.irq`/`struct gpio_irq_chip` before registration.
- `ldd1-ch16` shows direct calls such as `generic_handle_irq()` from parent chained handlers. Current generic IRQ docs also provide domain-aware helpers such as `generic_handle_domain_irq()`/safe variants; final examples should validate target-kernel choice before code.
- `ldd2-ch11` PCI interrupt material uses "MSI data" explanations that are conceptually useful, but PCI driver implementation APIs are outside this topic and should not expand the Interrupt Management chapter into PCI vector allocation.
- Notion examples are useful for learning flow but often omit production details: precise register semantics, complete error unwinding, current helper prototypes, valid masks, immutable irqchip helper macros, and datasheet-specific status/clear ordering.

## Gaps / Uncertainties
- Need target-kernel validation before publishing code examples that use tasklets, direct gpiolib irqchip helper calls, `struct gpio_chip` IRQ fields, or `generic_handle_irq()` variants.
- Internal sources do not deeply cover hierarchical IRQ domains, MSI domains, managed IRQ affinity, per-CPU IRQs, or PREEMPT_RT effects. Mention only as advanced related areas unless validated and scoped.
- Internal sources give limited debugging detail for generic IRQ core tracing. Future docs should validate `/proc/interrupts`, `/proc/irq/<n>/`, ftrace/tracepoints, dynamic debug, and `CONFIG_GENERIC_IRQ_DEBUGFS` usage against current docs.
- Internal sources do not cover `synchronize_irq()`, `disable_irq()`, `disable_irq_nosync()`, `enable_irq()`, or IRQ teardown race patterns in enough detail. These should be externally validated before final production checklist.
- Device-specific examples such as MCP23016 are simplified; real interrupt status, mask, clear, polarity, debounce, and read-to-clear behavior must come from datasheets or in-tree drivers before runnable examples.
- Learning-path boundary: GPIO controller implementation belongs mainly to topic 14; regmap IRQ/MFD belongs to topics 18/19; PCI MSI/MSI-X belongs to topic 31; power/wakeup IRQ policy belongs partly to topic 24.

## External Validation
- Used: https://docs.kernel.org/core-api/genericirq.html
  - Validates current exported generic IRQ APIs, `request_threaded_irq()`, `generic_handle_irq()` constraints, synchronization/freeing behavior, affinity helpers, and generic IRQ handling terminology.
- Used: https://docs.kernel.org/core-api/irq/irq-domain.html
  - Validates current irq-domain purpose: separating controller-local hardware IRQs from Linux IRQ numbers, hwirq-to-IRQ mapping, and why GPIO/other cascaded irqchips need domains.
- Used: https://docs.kernel.org/driver-api/gpio/driver.html
  - Validates current gpiolib IRQ-chip guidance, cascaded vs nested threaded GPIO irqchips, `struct gpio_chip.irq`/`struct gpio_irq_chip` setup direction, valid masks, and deprecation of older post-registration helper style.
- External validation still needed before final learner code:
  - Current target-kernel headers for `request_any_context_irq()`, tasklet APIs, gpiolib irqchip setup, and IRQ flow handler helpers.
  - Current in-tree examples for one MMIO chained controller and one I2C/SPI nested controller.
  - Current docs/source for `disable_irq*()`, `synchronize_irq()`, IRQ debugfs, and PREEMPT_RT/threaded interrupt behavior if included.

## Learning Content Brief
- Topic scope:
  - Generic Linux interrupt management for driver authors: IRQ flow, hard IRQ rules, top half/bottom half choices, threaded IRQs, request/free lifecycle, shared IRQs, IRQ domains, interrupt-controller hierarchy, chained/nested IRQs, DT interrupt bindings, common bugs, and debugging.
- Related topics:
  - Topic 06: synchronization basics and context rules.
  - Topic 10/11: Device Tree fundamentals and OF/platform resource APIs.
  - Topic 14: GPIO controller IRQ integration details.
  - Topic 19: regmap IRQ and MFD/syscon.
  - Topic 24: wakeup/system suspend interrupt policy.
  - Topic 31: PCI INTx/MSI/MSI-X implementation.
- Beginner mental model:
  - An interrupt is the hardware saying, "something happened; please look now."
  - The driver does not receive a raw CPU exception. It registers a Linux IRQ handler, and the IRQ core plus interrupt-controller drivers route the hardware event to that handler.
  - A hard IRQ handler is the emergency-room triage desk: identify, acknowledge/capture, and defer. Slow work belongs elsewhere.
  - IRQ domains are translation tables: local hardware source number plus controller identity becomes a Linux IRQ number that drivers can request.
- Core mechanism:
  - Hardware asserts an IRQ source.
  - Interrupt controller detects the source and signals the CPU.
  - Architecture entry code saves CPU context and calls the root interrupt handler.
  - Root/controller code maps hwirq to Linux IRQ via an IRQ domain.
  - IRQ core runs a flow handler such as level or edge handling.
  - IRQ core calls one or more registered `irqaction` handlers.
  - Handler returns `IRQ_NONE`, `IRQ_HANDLED`, or `IRQ_WAKE_THREAD`.
  - Optional threaded/bottom-half work finishes the slower part with interrupts re-enabled.
- Important structs/APIs:
  - Consumer request/lifecycle: `request_irq()`, `devm_request_irq()`, `request_threaded_irq()`, `devm_request_threaded_irq()`, `request_any_context_irq()`, `free_irq()`, `devm_free_irq()`.
  - Handler types/returns: `irq_handler_t`, `irqreturn_t`, `IRQ_NONE`, `IRQ_HANDLED`, `IRQ_WAKE_THREAD`, `IRQ_RETVAL()`.
  - Flags/types: `IRQF_SHARED`, `IRQF_ONESHOT`, `IRQF_TRIGGER_RISING`, `IRQF_TRIGGER_FALLING`, `IRQF_TRIGGER_HIGH`, `IRQF_TRIGGER_LOW`, `IRQF_NO_SUSPEND`, `IRQF_NO_THREAD`, `IRQ_TYPE_*`.
  - Deferring: `schedule_work()`, `INIT_WORK()`, `queue_work()`, `cancel_work_sync()`, tasklet APIs for legacy context, threaded IRQ handlers.
  - IRQ core/controller: `struct irq_chip`, `struct irq_domain`, `struct irq_domain_ops`, `struct irq_desc`, `struct irqaction`, `struct irq_data`.
  - IRQ domain: `irq_domain_add_linear()`, `irq_domain_add_tree()`, `irq_create_mapping()`, `irq_find_mapping()`, `irq_domain_xlate_onecell()`, `irq_domain_xlate_twocell()`.
  - Dispatch/controller hierarchy: `generic_handle_irq()`, `generic_handle_domain_irq()`, `handle_nested_irq()`, `irq_set_chained_handler_and_data()`, `chained_irq_enter()`, `chained_irq_exit()`.
  - DT/platform: `platform_get_irq()`, `platform_get_irq_byname()`, `of_irq_get()`, `irq_of_parse_and_map()`, `interrupt-controller`, `#interrupt-cells`, `interrupt-parent`, `interrupts`.
- Lifecycle/data flow:
  - Consumer driver probe: allocate private data -> map registers/resources -> get IRQ from platform/DT/bus -> initialize locks/work/thread state -> request IRQ -> enable/configure device interrupt source -> handle events -> disable source on remove -> free/synchronize implicitly or explicitly.
  - Hard IRQ handler: read status/cause -> reject with `IRQ_NONE` if shared and not ours -> clear/ack or mask as hardware requires -> capture minimal state -> wake thread or queue work if needed -> return.
  - Threaded IRQ: primary handler returns `IRQ_WAKE_THREAD` or default primary handler wakes thread -> IRQ remains masked with `IRQF_ONESHOT` -> `thread_fn` performs sleepable work -> returns `IRQ_HANDLED` -> IRQ can be unmasked.
  - Chained controller: parent hard handler enters chained flow -> reads fast status -> maps active child hwirqs -> calls child IRQ handlers -> exits chained flow.
  - Nested controller: threaded parent handler reads sleepable status -> maps active child hwirqs -> calls `handle_nested_irq()` -> child handlers run in thread context.
- Examples to produce later:
  - Minimal platform MMIO driver using `platform_get_irq()` and `devm_request_irq()` with status read/clear.
  - Threaded I2C sensor interrupt using `devm_request_threaded_irq(..., NULL, thread_fn, IRQF_ONESHOT, ...)`.
  - Workqueue bottom-half example that captures status in hard IRQ and processes buffers in a worker.
  - DT snippets for a GIC-backed platform device and an IRQ-capable GPIO expander.
  - Debug workflow using `/proc/interrupts`, `/proc/irq/<irq>/`, dynamic debug, and trigger/status register inspection.
- Common bugs:
  - Sleeping in hard IRQ context by calling I2C/SPI/regmap sleeping APIs, `mutex_lock()`, `msleep()`, `wait_event()`, or allocating with `GFP_KERNEL`.
  - Forgetting to clear or acknowledge the device-level interrupt source, causing an interrupt storm.
  - Returning `IRQ_HANDLED` on a shared IRQ without checking whether the device caused the interrupt.
  - Returning `IRQ_WAKE_THREAD` with no valid `thread_fn`.
  - Omitting `IRQF_ONESHOT` for a thread-only or slow level-triggered handler.
  - Using the wrong trigger type in DT or request flags, especially level vs edge and active-high vs active-low.
  - Freeing an IRQ while the device can still generate it.
  - Protecting hard-IRQ shared data with mutexes instead of spinlocks.
  - Holding spinlocks across slow work or calling callbacks that may sleep.
  - Treating hwirq numbers as globally unique Linux IRQ numbers.
  - Using chained IRQ handling for a controller accessed over a sleeping bus.
- Debugging notes:
  - Check `/proc/interrupts` to see whether the IRQ counter increments, which CPU handles it, and which handler names are registered.
  - Check `/proc/irq/<irq>/` for affinity and IRQ-specific information where available.
  - Add `dev_dbg()` around handler entry, status value, ack/clear writes, return value decisions, and threaded-handler entry/exit.
  - For storms, inspect level-triggered status clear order, mask/unmask callbacks, wrong polarity, shared IRQ returns, and device source still asserted.
  - For missing IRQs, inspect DT `interrupt-parent`, `interrupts`, pinctrl input configuration, trigger type, mask registers, clock/power state, and whether `platform_get_irq()` returned a valid Linux IRQ.
  - For latency, distinguish time in hard IRQ from time in threaded/workqueue bottom half; use ftrace/tracepoints when validated for the target kernel.
- Production concerns:
  - Keep hard IRQ handlers short, bounded, and non-sleeping.
  - Prefer `devm_request_irq()`/`devm_request_threaded_irq()` when the IRQ lifetime equals device lifetime, but still disable the device source explicitly during remove/suspend paths.
  - Use `IRQF_SHARED` only when hardware and handler logic are designed for shared lines.
  - Choose trigger flags from hardware/DT truth, not guesswork.
  - Use threaded IRQs for slow buses and any handler that may sleep.
  - Use workqueues when deferred work is not strictly tied to IRQ-thread semantics or must be batched/cancelled with driver lifecycle.
  - Treat tasklets as legacy; know them for existing code and interviews, prefer modern alternatives for new drivers.
  - Validate current IRQ/gpiolib APIs against the target kernel before publishing examples.
- Interview angles:
  - Explain hard IRQ context and why it cannot sleep.
  - Compare polling and interrupts.
  - Explain `request_irq()` parameters, especially `dev_id`.
  - Explain `IRQ_NONE` vs `IRQ_HANDLED` vs `IRQ_WAKE_THREAD`.
  - Explain why shared IRQ handlers must check device status.
  - Explain top half vs bottom half and choose between workqueue and threaded IRQ.
  - Explain `IRQF_ONESHOT`.
  - Explain level-triggered vs edge-triggered interrupt bugs.
  - Explain hwirq vs Linux IRQ and why IRQ domains exist.
  - Compare chained and nested interrupts.
  - Debug scenarios: interrupt storm, missing interrupt, sleeping-in-atomic warning, shared IRQ flags mismatch, and IRQ works in polling mode but not interrupt mode.
