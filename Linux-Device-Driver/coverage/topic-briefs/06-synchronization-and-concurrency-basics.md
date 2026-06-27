# Topic Brief - 06 - Synchronization And Concurrency Basics

## Topic Identity
- Learning-path number: `06`
- Slug: `synchronization-and-concurrency-basics`
- Primary scope: mutexes, spinlocks, atomic scalar operations, execution-context
  rules, sleeping versus atomic context, race patterns, deadlocks, lock-aware
  lifetime, and concurrency debugging.
- Related topics: `05-core-kernel-facilities`, `07-character-device-drivers`,
  `08-userspace-abi-design-for-drivers`, `14-gpio-controller-drivers-and-irq-integration`,
  `15-interrupt-management`, `17-spi-device-drivers`, `21-dma-and-dma-mapping`,
  and `37-kernel-debugging-and-tracing`.

## Output Targets
- Knowledge: `knowledge/06-synchronization-and-concurrency-basics.md`
- Interview: `interview/06-synchronization-and-concurrency-basics.md`
- Example: `examples/06-synchronization-and-concurrency-basics/README.md`

## Source Coverage
### Primary Sources
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch03` | `docs/Linux Device Driver Development/Chapter 3-Kernel Facilities and Helper Functions.md` | read/covered/merged | Atomic versus sleepable context, mutex and spinlock mental models, initialization, lock APIs, ownership rules, IRQ-safe spinlock use, and basic lock comparison. |
| `ldd2-ch01` | `docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md` | read/covered/merged | Stronger SMP explanation, spinlock/preemption behavior, IRQ-state variants, mutex ownership, trylock behavior, deadlock examples, and process/IRQ locking patterns. |
| `notion-ch03-part1` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 1 Data Structures & Synchronization.md` | read/mapped/merged-adjacent | Shows an embedded mutex protecting per-device state and reinforces that data structures need an explicit protection rule. List and `container_of()` teaching remains topic 05. |
| `notion-ch03-part2` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 2 Data Structures & Synchronization.md` | read/covered/merged | Expanded beginner explanations, mutex/spinlock examples, variant-selection scenarios, misuse examples, and a comparison matrix. |

### Adjacent Notion Chapter 3 Sources
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `notion-ch03-part3` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 3 Work Queues and Scheduling.md` | read/mapped/merged-adjacent | Process-context work, IRQ/work shared-state locking, synchronous cancellation, self-cancel deadlocks, and teardown ordering. Facility details remain topic 05. |
| `notion-ch03-part4` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 4 Work Queues and Scheduling.md` | read/mapped/merged-adjacent | Atomic tasklet context, per-instance serialization, cross-tasklet races, spinlock use, and stop-rescheduling-before-kill lifetime rules. Tasklets are legacy and belong mainly to topic 15. |
| `notion-ch03-part5` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 5 Work Queues and Scheduling.md` | read/mapped/merged-adjacent | Timer/hrtimer atomic context, spinlock-protected callback state, synchronous cancellation, and callback self-deadlock. Timer APIs remain topic 05. |
| `notion-ch03-part6` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 6 Wait Queues and Sleep Wake Mechanisms.md` | read/mapped/merged-adjacent | Protected condition publication, condition rechecking, wait/wake races, interruptible waits, and lock-plus-wait-queue examples. Wait queue APIs remain topic 05. |
| `notion-ch03-part7` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 7 Wait Queues and Sleep Wake Mechanisms.md` | read/mapped/cross-linked | Readiness state shared by read/write/poll paths and protected by a common locking rule. ABI and poll mechanics remain topics 07-08. |
| `notion-ch03-part8` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 8 Wait Queues and Sleep Wake Mechanisms.md` | read/mapped/merged-adjacent | IRQ-safe completion signaling, sleepable waiters, storage lifetime, timeout handling, and reinitialization races. Completion APIs remain topic 05. |
| `notion-ch03-extra` | `docs/Linux-Device-Driver-Notion/Linux Device Drivers.md` | read/mapped/index-only | Notion navigation confirms the chapter-3 part set; it contains no independent synchronization teaching. |

### Secondary Sources With Transferable Topic-06 Content
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch04` | `docs/Linux Device Driver Development/Chapter 4-Character Device Drivers.md` | read/mapped/supporting | Mutex-protected shared open/user count in a file-operation lifecycle. |
| `ldd1-ch08` | `docs/Linux Device Driver Development/Chapter 8-SPI Device Drivers.md` | read/mapped/merged | `spi_sync()` sleepability, atomic-safe asynchronous submission, callback completion, and transfer-buffer lifetime. |
| `ldd1-ch10` | `docs/Linux Device Driver Development/Chapter 10-IIO Framework.md` | read/mapped/supporting | Hard-IRQ top half versus sleepable threaded bottom half using mutexes and bus I/O. |
| `ldd1-ch11` | `docs/Linux Device Driver Development/Chapter 11-Kernel Memory Management.md` | read/mapped/merged | `GFP_KERNEL` versus `GFP_ATOMIC` as a consequence of execution context, not a general lock-selection shortcut. |
| `ldd1-ch12` | `docs/Linux Device Driver Development/Chapter 12-DMA - Direct Memory Access.md` | read/mapped/merged | Completion ordering, signal-before-wait behavior, and DMA callback-to-waiter synchronization. |
| `ldd1-ch14` | `docs/Linux Device Driver Development/Chapter 14-Pin Control and GPIO Subsystem.md` | read/mapped/merged | `gpio_cansleep()`/`gpiod_cansleep()` and choosing direct versus sleepable GPIO access from a callback context. |
| `ldd1-ch15` | `docs/Linux Device Driver Development/Chapter 15-GPIO Controller Drivers.md` | read/mapped/merged | Provider-side `gpio_chip.can_sleep` contract and threaded handling for sleepable GPIO controllers. |
| `ldd1-ch16` | `docs/Linux Device Driver Development/Chapter 16-Advanced IRQ Management.md` | read/mapped/merged | Atomic chained handlers versus threaded nested handlers for slow buses. |
| `ldd1-ch17` | `docs/Linux Device Driver Development/Chapter 17- Input Devices Drivers.md` | read/mapped/supporting | Allocation may sleep and must not occur in atomic context or while holding a spinlock. |
| `ldd1-ch19` | `docs/Linux Device Driver Development/Chapter 19-PWM Drivers.md` | read/mapped/supporting | Mutex-backed PWM request/free APIs cannot run in atomic context. |
| `ldd1-ch22` | `docs/Linux Device Driver Development/Chapter 22-Network Interface Card Drivers.md` | read/mapped/merged | Sleepable SPI paths use deferred/threaded work and mutexes; MMIO paths can use spinlocks. |
| `ldd2-ch02` | `docs/Linux Device Driver Development 2/Chapter 2-Regmap_API.md` | read/mapped/merged | Regmap selects mutex or spinlock from access sleepability; threaded regmap IRQ paths can use mutexes. |
| `ldd2-ch04` | `docs/Linux Device Driver Development 2/Chapter 4-Common_Clock_Framework.md` | read/mapped/merged | Sleepable `prepare/unprepare` versus spinlock-held, non-sleeping `enable/disable` callbacks. |
| `ldd2-ch07` | `docs/Linux Device Driver Development 2/Chapter 7-Demystifying V4L2 and Video Capture Device Drivers.md` | read/mapped/supporting | Different locks for queue ownership paths and deferral of sleepable bus access from IRQ callbacks. |
| `ldd2-ch10` | `docs/Linux Device Driver Development 2/Chapter 10-Linux_Kernel_Power_Management.md` | read/mapped/supporting | Asynchronous runtime-PM helpers defer work and can therefore be called from atomic context where documented. |
| `ldd2-ch11` | `docs/Linux Device Driver Development 2/Chapter 11-Writing_PCI_Device_Drivers.md` | read/mapped/merged | Multiple interrupt vectors sharing a lock create a local-IRQ deadlock risk requiring an IRQ-disabling spinlock variant. |
| `ldd2-ch13` | `docs/Linux Device Driver Development 2/Chapter 13-Watchdog_Device_Drivers.md` | read/mapped/supporting | `test_and_clear_bit()` as a concrete atomic read-modify-write operation for one-shot status consumption. |
| `ldd2-ch14` | `docs/Linux Device Driver Development 2/Chapter 14-Linux_Kernel_Debugging_Tips.md` | read/mapped/merged | `irqsoff`, `preemptoff`, and combined latency tracers for long atomic critical sections. |
| `notion-ch01-part4` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 4 Coding Style and Best Practices .md` | read/mapped/merged | Explicitly document global lock order, illustrated by device-lock-before-IRQ-lock ordering. |
| `notion-ch04-part2` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 2 Basic File Operations.md` | read/mapped/supporting | Interruptible mutex protection around concurrent character-device buffer access. |
| `notion-ch04-part3` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 3 Read, Write, and Seek Operations.md` | read/mapped/supporting | Serializes shared storage and file-position transitions across read/write/seek paths. |
| `notion-ch04-part4` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 4 Advanced Features - Poll and IOCTL.md` | read/mapped/cross-linked | Spinlock-protected readiness state and mutex-protected control state; detailed ABI mechanics remain topics 07-08. |
| `notion-ch07-extra` | `docs/Linux-Device-Driver-Notion/Chapter 7-v4l2_part2.md` | read/mapped/supporting | Spinlock-protected buffer ownership shared by process callbacks, stop-streaming, and DMA IRQ completion. |
| `notion-ch08-part2` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 2 SPI Transfer Mechanisms and Communication APIs.md` | read/mapped/merged | SPI sync/async context matrix, callback completion, transfer lifetime, and mutex serialization of multi-command transactions. |
| `notion-ch14-part2` | `docs/Linux-Device-Driver-Notion/Chapter 14-Part 2 GPIO Consumer Interfaces.md` | read/mapped/merged | Direct versus `_cansleep` GPIO access and context-selection flow. |
| `notion-ch14-part3` | `docs/Linux-Device-Driver-Notion/Chapter 14-Part 3 Userspace GPIO Access.md` | read/mapped/cross-linked | Notes race-prone semantics as a limitation of the deprecated GPIO sysfs ABI. |
| `notion-ch15-part1` | `docs/Linux-Device-Driver-Notion/Chapter 15-Part 1 GPIO Controller Architecture.md` | read/mapped/merged | Provider `can_sleep` and `irq_not_threaded` contracts constrain legal consumer contexts. |
| `notion-ch15-part2` | `docs/Linux-Device-Driver-Notion/Chapter 15-Part 2 IRQ Chip Integration.md` | read/mapped/merged | Chained atomic flow versus nested threaded flow for sleepable status-register access. |
| `notion-ch15-part3` | `docs/Linux-Device-Driver-Notion/Chapter 15-Part 3 Advanced Features and Integration.md` | read/mapped/supporting | Mutex-protected register read-modify-write with nested IRQ dispatch and `IRQF_ONESHOT`. |
| `notion-ch16-part1` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 1 IRQ Architecture and Propagation.md` | read/mapped/supporting | Lock and context transitions through IRQ core, parent controller, and nested child handler. |
| `notion-ch16-part2` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 2 IRQ Multiplexing - Chained and Nested Inter.md` | read/mapped/merged | Detailed atomic chained flow, threaded nested flow, raw-spinlock use, and teardown constraints. |
| `notion-ch16-part3` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 3 Advanced Topics.md` | read/mapped/merged | Hard/threaded/chained/nested IRQ context decision tree and `IRQF_ONESHOT` concurrency behavior. |

## Source Files Read
- Primary files were read independently rather than inferred from chapter numbers:
  `ldd1-ch03`, `ldd2-ch01`, `notion-ch03-part1`, and
  `notion-ch03-part2`.
- All Notion chapter-3 continuations were read in full:
  `notion-ch03-part3` through `notion-ch03-part8`, plus the index-only
  `notion-ch03-extra`.
- The secondary files in the coverage table were read at the listed
  synchronization-relevant sections. Their primary subsystem teaching is not
  duplicated into topic 06.
- All 84 files under `ldd1`, `ldd2`, and `notion` were searched and triaged.
  Incidental-only hits were inspected but excluded from topic coverage when
  they only declared an internal lock or counter without teaching a
  transferable rule. Examples include `ldd1-ch13`, `ldd1-ch18`,
  `ldd1-ch20`, `ldd1-ch21`, and `ldd2-ch12`.

## Merged Source Notes
- Use all sources for the core reasoning sequence: identify shared state, list every execution context, decide whether any context is atomic, then choose the smallest protection domain that preserves the state invariant.
- Use `ldd2-ch01` for the clearest explanation of why plain `spin_lock()` is insufficient when the same lock is used by process context and a hard-IRQ handler on the local CPU.
- Use `notion-ch03-part2` for concrete misuse examples, but correct its blanket recommendations and time-based lock-selection rules.
- Use Notion chapter 3 parts 3-8 for applied concurrency relationships rather
  than facility tutorials: callback context, producer/consumer state, readiness
  predicates, signal-before-wait behavior, synchronous cancellation, and
  producer-versus-teardown races.
- Merge the subsystem examples as evidence that lock choice follows callback and
  access semantics: sleepable SPI/GPIO/regmap operations must move to process or
  threaded context, while short MMIO or IRQ-shared state may require a
  spinlock.
- Preserve the PCI multi-vector example as a distinct IRQ deadlock pattern:
  non-reentrancy of one interrupt source does not prevent another vector from
  interrupting a handler that holds the same per-device lock.
- Preserve the clock-framework split as a strong API-design example:
  `prepare/unprepare` may sleep, while `enable/disable` are non-sleeping and
  execute under a spinlock.
- Add `atomic_t`/`atomic64_t` because the learning-path scope explicitly requires atomic operations. Internal sources mention atomics as lock implementation details but do not teach their API or limitations adequately.
- Use `test_and_clear_bit()` from `ldd2-ch13` as the internal source's clearest
  real atomic read-modify-write example, while avoiding unrelated internal
  `atomic_t` fields in tasklet, memory-management, or framework structures.
- Add `READ_ONCE()`/`WRITE_ONCE()` only as an overview. They constrain individual compiler accesses but do not provide mutual exclusion or, by themselves, publish a multi-field state transition.
- Add lock ordering, lock scope documentation, and assertions such as `lockdep_assert_held()` because internal sources under-teach maintainability and deadlock prevention.
- Merge lifecycle synchronization as a first-class concern: initialize locks
  before publishing callbacks; stop new producers; synchronously drain work,
  timers, tasklets, IRQs, or completions as appropriate; then free the
  lock-containing storage.
- Keep wait queues, completions, timers, and workqueue lifecycle in topic 05. Use them here only when explaining execution context or synchronization relationships.
- Keep detailed interrupt registration and bottom-half design in topic 15. Topic 06 teaches the lock-context decision that later IRQ code relies on.
- Keep read/write/poll ABI mechanics in topics 07-08, GPIO provider mechanics in
  topics 13-14, SPI mechanics in topic 17, and DMA mechanics in topic 21. Topic
  06 retains only their shared-state, context, ordering, and lifetime lessons.

## Source Differences
- Internal sources repeatedly frame a mutex as suitable for "long" sections and a spinlock for sections below fixed values such as 100 microseconds or 1 millisecond. Current teaching should not use invented thresholds. Choose by execution context and whether the protected operation can sleep; then minimize hold time for every lock.
- `ldd1-ch03` and Notion describe `mutex_lock_interruptible()` as generally preferred and plain `mutex_lock()` as discouraged. This is too broad. Use an interruptible acquisition when the caller has meaningful signal/error semantics, especially a userspace-facing path; otherwise `mutex_lock()` is normal.
- The sources imply `mutex_trylock()` can be used anywhere because it does not wait. Current lock-type guidance says a mutex remains a sleeping-lock primitive; do not use trylock as an IRQ-context escape hatch.
- The sources' "spinlock held by a CPU, mutex held by a task" model is useful but incomplete. On `PREEMPT_RT`, `spinlock_t` semantics change; `raw_spinlock_t` remains the strict spinning primitive.
- `ldd1-ch03` recommends `spin_lock_irqsave()` almost universally. The correct variant depends on which contexts can access the protected data. Over-disabling interrupts increases latency and can hide an unclear ownership design.
- Notion's claim that `spin_lock_irqsave()` has no risk is false. It can create latency, deadlock through bad nesting, and sleep-in-atomic bugs, and it does not protect against DMA/device-side races.
- Notion's examples return `-ERESTARTSYS` for every nonzero `mutex_lock_interruptible()` result. The safer generic pattern is to propagate the returned error unless a specific API contract requires translation.
- Notion suggests replacing `GFP_KERNEL` with `GFP_ATOMIC` inside a spinlock as the preferred fix. The first design question should be whether allocation can move outside the critical section; `GFP_ATOMIC` is constrained fallback allocation, not a license for broad work under a spinlock.
- Internal sources show implementation snapshots of `struct mutex`. These fields are not driver API and should not be memorized.
- Internal sources do not adequately distinguish atomic read-modify-write operations from broader state synchronization, memory ordering, or reference counting.
- `notion-ch03-part3` labels `cancel_work_sync()` asynchronous in one place even
  though the API waits for running work; do not preserve that label.
- A `notion-ch03-part5` timer cleanup example dereferences its device pointer
  after `kfree()`. The valid order is synchronous timer shutdown before freeing
  the containing state, with no later dereference.
- `notion-ch03-part6` says `wake_up()` evaluates each waiter's condition. The
  condition belongs to the `wait_event*()` loop and is reevaluated by the
  waiter; the producer must update protected state before waking.
- Several Notion wait-queue and poll examples split condition checks, user
  copies, and state updates without reserving the state. Do not present them as
  concurrency-correct multi-reader/multi-writer designs.
- `notion-ch03-part8` oversimplifies completions as one-shot and incorrectly
  implies one `complete()` permanently satisfies repeated waits. A completion
  event is consumable; `complete_all()` has persistent behavior until
  reinitialization, whose lifetime and concurrency must be controlled.
- Tasklet material is retained only to explain legacy atomic context and
  serialization. New teaching should prefer current threaded IRQ or workqueue
  designs where those fit.
- GPIO examples that perform operations from timer or hrtimer callbacks are
  valid only for accessors guaranteed not to sleep.

## Gaps / Uncertainties
- Internal sources do not directly teach `atomic_t` APIs, ordering suffixes, or why atomic operations cannot preserve multi-variable invariants.
- Memory barriers and the Linux Kernel Memory Model are too broad for a basic chapter. This topic should establish that atomicity and ordering are different, use locks for normal driver state, and defer custom lockless protocols until there is a measured need and a documented proof.
- Advanced primitives such as rwlocks, rwsems, seqlocks, RCU, percpu data, local locks, and lock-free algorithms are outside the core scope. Mention them as later design options, not beginner defaults.
- `PREEMPT_RT` changes `spinlock_t` behavior. Include a practical version note without turning this chapter into real-time locking documentation.
- Internal sources do not cover lockdep, KCSAN, or useful ownership assertions deeply enough.
- Driver-specific hardware serialization may require MMIO ordering, bus locks, runtime-PM coordination, or DMA ownership rules in addition to CPU locks. Those details belong primarily to later subsystem topics.
- Internal sources do not provide a single reliable end-to-end example of
  protecting a wait predicate, its payload, and teardown against multiple
  readers and writers. The learner example should avoid implying that wakeup
  alone provides data synchronization.
- External validation remains necessary for current tasklet deprecation
  guidance, completion semantics, lockdep configuration/use, memory-ordering
  claims, and any `PREEMPT_RT`-specific code advice.

## External Validation
| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/locking/locktypes.html` | Validated sleeping, CPU-local, and spinning lock categories; nesting rules; owner semantics; spinlock suffix behavior; and `PREEMPT_RT` caveats. |
| `https://docs.kernel.org/core-api/real-time/differences.html` | Validated forced-threaded interrupt behavior, RT `spinlock_t` semantics, and the role of `raw_spinlock_t` in true hard-IRQ or non-preemptible paths. |
| `https://docs.kernel.org/locking/mutex-design.html` | Validated mutex ownership, sleeping-lock behavior, non-recursion, lifetime rules, and implementation caveats. |
| `https://docs.kernel.org/kernel-hacking/locking.html` | Validated context-based mutex/spinlock selection, IRQ-safe spinlock patterns, trylock caveats, deadlock examples, and the rule to document which data each lock protects. |
| `https://docs.kernel.org/core-api/wrappers/atomic_t.html` | Validated `atomic_t`/`atomic64_t` purpose, atomic read-modify-write operations, ordering variants, and the warning that atomic non-RMW access alone is usually unnecessary. |
| `https://docs.kernel.org/core-api/refcount-vs-atomic.html` | Validated that object lifetime counters should normally use `refcount_t` rather than generic `atomic_t`, with different safety and ordering semantics. |
| `https://docs.kernel.org/dev-tools/kcsan.html` | Validated KCSAN as a sampling-based kernel data-race detector, its marked-access awareness, and its limitations. |

The official kernel documentation above was rechecked on June 6, 2026. Local
target-header validation previously used Linux `6.8.0-124-generic` headers for
mutex, spinlock, atomic, refcount, and lockdep APIs; local headers are not an
external source and do not replace version-specific upstream validation.

## Learning Content Brief
- Mental model:
  - A race exists when correctness depends on an uncontrolled interleaving.
  - Protect data and invariants, not arbitrary lines of code.
  - A lock's scope must be documented: which fields it protects and which contexts may acquire it.
- Context inventory:
  - Process context can normally sleep.
  - Hard IRQ, softirq/tasklet, and conventional timer callbacks cannot sleep.
  - Workqueue and threaded-IRQ callbacks run in process context, but their data can still race with atomic contexts.
- Mutex:
  - Default for task-only shared state and operations that may sleep.
  - `DEFINE_MUTEX()`, `mutex_init()`, `mutex_lock()`, `mutex_lock_interruptible()`, `mutex_lock_killable()`, `mutex_trylock()`, and `mutex_unlock()`.
  - Owner-only unlock, no recursion, no use in interrupt context, and lock storage must remain alive.
- Spinlock:
  - For short, non-sleeping critical sections that can be reached from atomic context or require inter-CPU serialization without sleeping.
  - `DEFINE_SPINLOCK()`, `spin_lock_init()`, basic, `_bh`, `_irq`, and `_irqsave` variants.
  - Select the variant from the highest-priority context sharing the data; pair save/restore correctly.
- Atomics:
  - Use for a genuinely independent scalar read-modify-write state such as a statistic or simple counter.
  - Do not use one atomic per field to claim a multi-field invariant is protected.
  - Use `refcount_t` for object references.
- Race patterns:
  - check-then-act, read-modify-write, split state publication, readiness
    predicate versus payload, signal-before/after-wait, lifetime races,
    producer-versus-teardown, and inconsistent lock coverage.
- Deadlocks:
  - recursive acquisition, ABBA ordering, sleeping under a spinlock,
    self-cancel/self-delete, synchronous cancellation while holding a
    callback-needed lock, cross-vector IRQ nesting, and process/IRQ
    self-deadlock from the wrong spinlock variant.
- Lifecycle:
  - initialize locks before publishing callbacks; stop self-rearming producers;
    prevent new submissions; synchronously drain callbacks/waiters; free
    lock-containing storage last.
- Debugging:
  - lockdep and `CONFIG_PROVE_LOCKING`, KCSAN, `might_sleep()` reports,
    `irqsoff`/`preemptoff` tracers, hung-task/soft-lockup reports, stack traces,
    lock statistics where appropriate, and ownership assertions.
- Example:
  - Learning-only module with two kthreads incrementing equivalent counters under a mutex, spinlock, and `atomic64_t`.
  - A mutex-protected two-field invariant demonstrates why atomics on independent fields are insufficient.
  - Completion-based startup result, timeout/error cleanup, parameter validation, and clean module teardown.
