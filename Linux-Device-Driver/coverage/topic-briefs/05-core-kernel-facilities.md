# Topic Brief - 05 - Core Kernel Facilities

## Output Targets
- Knowledge: `knowledge/05-core-kernel-facilities.md`
- Interview: `interview/05-core-kernel-facilities.md`
- Example: `examples/05-core-kernel-facilities/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch03` | `docs/Linux Device Driver Development/Chapter 3-Kernel Facilities and Helper Functions.md` | read/covered/merged | Broad primary source for `container_of()`, intrusive circular lists, wait queues, jiffies, standard timers, hrtimers, sleeping/delay choices, and system/custom workqueues. |
| `ldd1-ch12` | `docs/Linux Device Driver Development/Chapter 12-DMA - Direct Memory Access.md` | read/covered/merged | Independent completion treatment and a concrete DMA flow: submit asynchronous work, wait, and signal from the completion callback. |
| `ldd1-ch13` | `docs/Linux Device Driver Development/Chapter 13-The Linux Device Model.md` | read/mapped/merged-overview | Direct source for the requested kobject overview: embedded `struct kobject`, hierarchy, reference counting, `kobj_type`, release callbacks, ksets, and sysfs relationship. Detailed device-model mechanics belong to topic 12. |
| `ldd2-ch01` | `docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md` | read/covered/merged | Strong source for completions, wait queues, interruptible-wait failure handling, work items, worker pools, concurrency-managed workqueues, flags, cancellation, and teardown. |
| `notion-ch01-part4` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 4 Coding Style and Best Practices .md` | read/mapped/merged-overview | Short independent kobject/reference-counting explanation using embedded `device.kobj`, `kobject_get()`, and `kobject_put()`. |
| `notion-ch03-part1` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 1 Data Structures & Synchronization.md` | read/covered/merged | Expanded mental models and examples for `container_of()`, embedded `list_head`, list initialization, insertion, deletion, traversal, and safe deletion traversal. |
| `notion-ch03-part3` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 3 Work Queues and Scheduling.md` | read/covered/merged | Detailed workqueue lifecycle, system versus allocated queues, delayed work, `to_delayed_work()`, queueing, cancellation, flushing, teardown, and driver examples. |
| `notion-ch03-part4` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 4 Work Queues and Scheduling.md` | read/mapped/adjacent | Compares tasklet atomic context with workqueue process context. Tasklet implementation is outside this topic and belongs with interrupt/bottom-half coverage. |
| `notion-ch03-part5` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 5 Work Queues and Scheduling.md` | read/covered/merged | Most modern internal timer source: `timer_setup()`, `from_timer()`, `mod_timer()`, timer teardown, callback context, hrtimers, and timer/workqueue selection. |
| `notion-ch03-part6` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 6 Wait Queues and Sleep Wake Mechanisms.md` | read/covered/merged | Expanded wait-queue flow, initialization, wait/wake variants, timeout and signal returns, condition-before-wakeup ordering, and blocking-I/O examples. |
| `notion-ch03-part7` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 7 Wait Queues and Sleep Wake Mechanisms.md` | read/mapped/merged-adjacent | Shows wait queues as the mechanism behind blocking/nonblocking I/O and `poll_wait()`. Detailed file-operation and userspace ABI behavior belongs to topics 07 and 08. |
| `notion-ch03-part8` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 8 Wait Queues and Sleep Wake Mechanisms.md` | read/covered/merged | Expanded completion API, timeout variants, IRQ-side signaling, DMA example, `reinit_completion()`, and comparison with condition-based wait queues. |

## Source Files Read
- `docs/Linux Device Driver Development/Chapter 3-Kernel Facilities and Helper Functions.md`
  - Relevant sections: Understanding the `container_of` macro; Linked lists; Creating and initializing a list; Adding/deleting/traversing list nodes; The kernel sleeping mechanism; Wait queue; Delay and timer management; Standard timers; Jiffies and HZ; High-resolution timers; Delays and sleep; Work queues; Kernel-global work queue; Dedicated work queue.
- `docs/Linux Device Driver Development/Chapter 12-DMA - Direct Memory Access.md`
  - Relevant section: The concept of completion, including static/dynamic initialization, wait-before-or-after-complete behavior, and DMA callback use.
- `docs/Linux Device Driver Development/Chapter 13-The Linux Device Model.md`
  - Relevant sections: Deep inside LDM; kobject structure; `kobj_type`; ksets; attributes boundary.
- `docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md`
  - Relevant sections: Waiting, sensing, and blocking; Waiting for completion or state change; Linux kernel wait queues; Workqueues; The kernel shared queue; Workqueues - a new generation; Concurrency-managed workqueues.
- `docs/Linux-Device-Driver-Notion/Chapter 1-Part 4 Coding Style and Best Practices .md`
  - Relevant section: kobject and Reference Counting.
- `docs/Linux-Device-Driver-Notion/Chapter 3-Part 1 Data Structures & Synchronization.md`
  - Relevant sections: Container and Data Structures; Understanding `container_of`; Linked Lists; List API Functions; List Traversal; How `list_for_each_entry` Works; Best Practices.
- `docs/Linux-Device-Driver-Notion/Chapter 3-Part 3 Work Queues and Scheduling.md`
  - Relevant sections: Workqueue API and Usage Patterns; Workqueue Initialization; System Workqueues; Custom Workqueues; Queuing Work; Canceling and Flushing Work; Best Practices.
- `docs/Linux-Device-Driver-Notion/Chapter 3-Part 4 Work Queues and Scheduling.md`
  - Relevant sections: Tasklet vs Workqueue Comparison; When to Use Tasklets.
- `docs/Linux-Device-Driver-Notion/Chapter 3-Part 5 Work Queues and Scheduling.md`
  - Relevant sections: Timer APIs and High-Resolution Timers; Jiffies and HZ; Standard Timers; Timer Deletion; High-Resolution Timers; Timer Comparison; Best Practices.
- `docs/Linux-Device-Driver-Notion/Chapter 3-Part 6 Wait Queues and Sleep Wake Mechanisms.md`
  - Relevant sections: Introduction to Kernel Sleeping; Wait Queue Implementation; `wait_event()` family; `wake_up()` family; Blocking Read; Timeout Example.
- `docs/Linux-Device-Driver-Notion/Chapter 3-Part 7 Wait Queues and Sleep Wake Mechanisms.md`
  - Relevant sections: Blocking vs Non-Blocking I/O; Poll/Select Mechanisms; `poll_wait()` and wait-queue application.
- `docs/Linux-Device-Driver-Notion/Chapter 3-Part 8 Wait Queues and Sleep Wake Mechanisms.md`
  - Relevant sections: Completion Framework; Completion API; DMA Completion Example; Completion vs Wait Queue; Best Practices.
- `docs/Linux-Device-Driver-Notion/Linux Device Drivers.md`
  - Inspected as the Notion navigation/index. It points to Chapter 3 material but contains no additional technical treatment to merge.

## Merged Source Notes
- Use `ldd1-ch03` as the broad structural backbone, but do not copy its old timer and workqueue APIs into future learner-facing code.
- Merge `notion-ch03-part1` for the clearest beginner explanation of intrusive data structures: a driver embeds `list_head` or another subsystem object, then recovers the containing driver object with `container_of()`.
- Merge `notion-ch03-part5` as the preferred internal source for modern timer callback shape: `timer_setup()`, callback receives `struct timer_list *`, and `from_timer()` recovers driver state.
- Merge `ldd2-ch01` and `notion-ch03-part3` for workqueues. The former explains concurrency-managed worker pools and flags; the latter gives clearer initialization, queue, cancellation, flush, and remove-path examples.
- Merge `ldd2-ch01` with `notion-ch03-part6` for wait queues. Preserve the second book's production bug story: ignoring an interruptible wait's return can lead to accessing data whose readiness condition is still false.
- Merge all three independent completion treatments:
  - `ldd1-ch12` supplies the DMA callback mental model.
  - `ldd2-ch01` supplies context rules and return-value handling.
  - `notion-ch03-part8` supplies API organization and completion-versus-wait-queue comparison.
- Keep kobjects conceptual in topic 05: embedded object, parent hierarchy, reference-counted lifetime, release callback, and sysfs relationship. Registration details, attributes, ksets, uevents, and device lifetime belong to topic 12.
- Use `notion-ch03-part4` only to establish the choice boundary: workqueues run in process context and may sleep; tasklets run in atomic context and are not part of this topic's implementation scope.
- Use `notion-ch03-part7` only as an application link. Core wait mechanics belong here; `O_NONBLOCK`, `-EAGAIN`, `.poll`, event masks, and stable userspace behavior belong to topics 07 and 08.

## Source Differences
- `ldd1-ch03` uses pre-4.15 timer interfaces and layouts: `setup_timer()`, `init_timer()`, callback data arguments, and an obsolete `struct timer_list` representation. Notion part 5 uses the externally validated modern baseline: `timer_setup()` and `from_timer()`.
- `ldd1-ch03` and parts of `ldd2-ch01` describe legacy workqueue topology and use `create_workqueue()`/`create_singlethread_workqueue()`. The learner-facing chapter should start from system workqueues and concurrency-managed `alloc_workqueue()`/`alloc_ordered_workqueue()` concepts.
- The books mention `flush_scheduled_work()`. Flushing the entire shared system workqueue is overbroad; teardown should normally cancel or flush work owned by the driver.
- `ldd1-ch13` exposes old kobject internals such as `sysfs_dirent` and `kobj_type.default_attrs`. These layouts are not stable API and must not be taught as current structure definitions.
- `ldd1-ch03` and Notion part 1 claim that `container_of()` does not work with pointer or array members. This is false as a general rule. The supplied pointer must point to the actual embedded member and have a compatible type; a pointer member's pointee is not the address of the pointer member itself.
- `ldd1-ch03` contains contradictory tasklet enable/count wording. Tasklets are adjacent to this topic, so future output should avoid inheriting that explanation.
- Notion part 6 says `wake_up()` reevaluates the caller's condition. More precisely, `wake_up*()` wakes eligible waiters; the `wait_event*()` loop reevaluates its condition when the task runs.
- Notion part 8 describes completion as simply one-shot and shows repeated waits returning after one `complete()`. A normal `complete()` contributes a consumable completion event; `complete_all()` has different persistent-until-reinitialized behavior.
- Notion part 5 contains a use-after-free in its basic timer exit example: it dereferences `global_dev` after `kfree(global_dev)`.
- Notion part 5's hrtimer PWM example risks using a GPIO accessor that may sleep from atomic timer context and should not substitute for the PWM framework.
- Several Notion wait-queue examples read readiness state outside the lock used to update it, omit explicit memory-ordering discussion, or split buffer state checks from copies in race-prone ways. Treat them as conceptual, not production-ready.
- Notion part 3 labels `cancel_work_sync()` as asynchronous in one API comment even though the accompanying explanation correctly describes it as synchronous.
- List, timer, wait-queue, completion, kobject, and workqueue structure layouts shown by sources are implementation snapshots. Teach API contracts, ownership, context, and lifecycle rather than memorizing fields.

## Gaps / Uncertainties
- Internal sources do not explain list concurrency. Lists provide structure, not synchronization; topic 05 should state that callers need an appropriate lock or ownership rule, while lock selection belongs to topic 06.
- Internal sources do not adequately cover list corruption diagnostics, poison values, debug-list configuration, or RCU list variants.
- Current timer teardown was externally validated, including the distinction between synchronous deletion and shutdown that prevents later rearming. Exact helper availability still depends on the supported target kernel.
- Current workqueue architecture, queueing, cancellation, flushing, destruction, and flag semantics were externally validated. Advanced CPU affinity/locality and `max_active` tuning remain outside this basic topic.
- Wait-queue sources do not deeply cover exclusive waiters, `wait_event*_lock_irq()` variants, memory barriers, or the exact race-free publication pattern for a condition changed by IRQ/process contexts.
- Completion token consumption, `complete_all()`, safe reinitialization, stack lifetime, and waiter/completer lifetime were externally validated.
- Kobject sources do not adequately warn beginners that drivers should usually use subsystem wrappers (`struct device`, bus/class APIs, and their get/put helpers) instead of creating raw kobjects.
- Internal examples underplay lifetime coupling: list nodes, timer callbacks, work items, wait queues, and completions often live inside the same driver-private object and must be quiesced before that object is freed.
- Deep locking and atomic-versus-sleeping context belongs to topic 06; interrupt bottom halves to topic 15; DMA details to topic 21; device model/sysfs to topic 12; blocking I/O and poll ABI to topics 07 and 08; advanced tracing to topic 37.

## External Validation
| Source | Validation Performed |
| --- | --- |
| `https://docs.kernel.org/core-api/list.html` | Validated the intrusive circular-list model, `container_of()` relationship, initialization, add/delete/traversal helpers, safe deletion traversal, poor cache locality, and the requirement for caller-provided concurrency control. |
| `https://docs.kernel.org/core-api/workqueue.html` | Validated concurrency-managed worker pools, system versus allocated workqueues, queueing/coalescing behavior, workqueue flags, cancellation, flushing, destruction, and producer-first teardown reasoning. |
| `https://docs.kernel.org/scheduler/completion.html` | Validated completion event retention, `complete()` token consumption, `complete_all()`, safe `reinit_completion()` ordering, timeout/interruptible waits, atomic-context signaling, and storage lifetime. |
| `https://docs.kernel.org/core-api/kobject.html` | Validated kobject embedding, ktypes/ksets, reference ownership, `kobject_get()`/`kobject_put()`, release callbacks, hierarchy/sysfs relation, and the preference for subsystem-specific wrappers. |
| `https://docs.kernel.org/driver-api/basics.html` | Validated current timer, hrtimer, wait-queue, completion, workqueue, delay, and context-sensitive helper contracts, including synchronous timer deletion/shutdown and wait return semantics. |
| Linux `6.8.0-124-generic` target headers | Confirmed `timer_setup()`, `timer_delete_sync()`, `timer_shutdown_sync()`, `wake_up_interruptible_all()`, and the example's callback signatures. This is local target-header validation, not an external source. |

Remaining caveats:

- Older supported kernels may use `del_timer_sync()` and may not provide newer setup/shutdown helpers; validate exact target headers.
- Deep wait-queue barrier patterns, exclusive waiters, RCU list variants, advanced workqueue tuning, and raw kobject registration remain intentionally deferred.

## Learning Content Brief
- Learning-path identity:
  - Number: `05`
  - Slug: `core-kernel-facilities`
  - Primary scope: lists, `container_of()`, kobjects overview, timers, workqueues, completions, and wait queues.
  - Related topics: 06 Synchronization, 07 Character Devices, 08 Userspace ABI, 12 Linux Device Model, 15 Interrupt Management, 21 DMA, and 37 Debugging/Tracing.
- Beginner mental model:
  - These facilities are reusable kernel building blocks for embedding objects, organizing many instances, scheduling later work, waiting efficiently, and keeping objects alive.
  - `container_of()` answers "which driver object owns this embedded member?"
  - A kernel list links embedded `list_head` members rather than allocating generic wrapper nodes.
  - A timer says "run this callback no earlier than a time"; a workqueue says "run this function later in process context."
  - A wait queue waits for a condition that can become true repeatedly; a completion signals that a particular unit of asynchronous work has finished.
  - A kobject is the low-level identity/lifetime/hierarchy component behind much of the device model, not normally the first API a driver should expose directly.
- Core mechanisms and APIs:
  - Object embedding: `container_of()`, subsystem-specific wrappers such as `from_timer()`, and `to_delayed_work()`.
  - Lists: `struct list_head`, `LIST_HEAD()`, `INIT_LIST_HEAD()`, `list_add()`, `list_add_tail()`, `list_del()`, `list_del_init()`, `list_empty()`, `list_for_each_entry()`, and `list_for_each_entry_safe()`.
  - Timers: `struct timer_list`, `timer_setup()`, `mod_timer()`, `timer_pending()`, target-appropriate synchronous deletion/shutdown helpers, `jiffies`, `msecs_to_jiffies()`, and wrap-safe time comparison helpers.
  - Hrtimers as an overview: `struct hrtimer`, `hrtimer_init()`, `hrtimer_start()`, `hrtimer_cancel()`, `hrtimer_forward*()`, `HRTIMER_NORESTART`, and `HRTIMER_RESTART`.
  - Workqueues: `struct work_struct`, `struct delayed_work`, `DECLARE_WORK()`, `INIT_WORK()`, `INIT_DELAYED_WORK()`, `schedule_work()`, `schedule_delayed_work()`, `queue_work()`, `queue_delayed_work()`, `alloc_workqueue()`, `alloc_ordered_workqueue()`, `cancel_work_sync()`, `cancel_delayed_work_sync()`, `flush_work()`, `flush_workqueue()`, and `destroy_workqueue()`.
  - Wait queues: `wait_queue_head_t`, `DECLARE_WAIT_QUEUE_HEAD()`, `init_waitqueue_head()`, `wait_event*()` variants, `wake_up*()` variants, timeout conversion, and mandatory return checking for interruptible waits.
  - Completions: `struct completion`, `DECLARE_COMPLETION()`, `init_completion()`, `reinit_completion()`, `wait_for_completion*()`, `complete()`, and `complete_all()`.
  - Kobject overview: `struct kobject`, `struct kobj_type`, `struct kset`, parent hierarchy, get/put references, and release callback; detailed API deferred to topic 12.
- Lifecycle/data flow:
  - Embed list/timer/work/wait/completion/subsystem members in a driver-private object.
  - Initialize every embedded facility before publishing the object or enabling IRQ/hardware paths that can use it.
  - Producers update protected state, then signal with `wake_up*()` or `complete()`.
  - Waiters sleep only in process context, check return values, and recheck or consume the expected state/event.
  - Timer and work callbacks recover the containing object, do context-appropriate work, and avoid touching state after teardown starts.
  - Remove/error paths stop new producers, disable hardware/IRQs as needed, synchronously quiesce timers and work, wake or resolve waiters where required, unlink list nodes, release references, then free the containing object.
- Example direction:
  - Create one learning-only module centered on a private `struct facility_demo` embedding a list node, timer, work item, wait queue, completion, and state flags.
  - Let the timer do only atomic-safe state capture and queue work.
  - Let work run in process context, update a condition, wake a wait queue, and signal a completion.
  - Demonstrate `container_of()`/`from_timer()`, list insertion/removal, interruptible timeout handling, and strict reverse-order teardown.
  - Keep kobjects conceptual or use an existing higher-level object; do not make raw sysfs/kobject creation the topic-05 example.
- Common bugs:
  - Passing a pointee to `container_of()` instead of the address of the embedded member, or using the wrong containing type/member.
  - Traversing or deleting list nodes without ownership/locking, using non-safe traversal while deleting, double deletion, or freeing a still-linked node.
  - Using direct jiffies arithmetic comparisons that fail across wraparound.
  - Sleeping, taking a mutex, or calling a potentially sleeping GPIO/I/O API in timer/hrtimer context.
  - Freeing driver state before a timer or work callback is synchronously stopped.
  - Assuming queued work runs immediately, assuming repeated queueing creates repeated executions, or deadlocking by synchronously canceling the current work item from itself.
  - Flushing a global workqueue instead of owned work.
  - Ignoring `-ERESTARTSYS`, timeout, or killable-wait returns and accessing data as though the condition succeeded.
  - Updating the wait condition after wakeup, publishing condition state without proper synchronization, or waiting forever during device removal.
  - Reinitializing a completion while another actor can still call `complete()`, misunderstanding `complete()` versus `complete_all()`, or using a completion whose storage has gone out of scope.
  - Calling raw kobject APIs without a correct release/lifetime model or duplicating a subsystem's device-model support.
- Debugging notes:
  - Inspect loaded module logs and callback order with targeted `pr_debug()`/dynamic debug rather than high-rate unconditional prints.
  - For hangs, identify the sleeping task and wait site; distinguish an unfulfilled condition, lost producer, ignored signal, deadlocked teardown, and workqueue dependency cycle.
  - For work/timer use-after-free, verify remove ordering and synchronous cancellation before memory release; use KASAN, lockdep, debug objects, and relevant tracepoints.
  - For list corruption, inspect double add/delete, stale nodes, missing initialization, concurrent traversal, and lifetime; use list-debug facilities where enabled.
  - For delayed execution, distinguish "queued", "pending", "running", "canceled", and "requeued"; log state transitions sparingly.
- Production concerns:
  - Context determines legal operations: timer/hrtimer callbacks are atomic; workqueue callbacks are process context; waiters sleep only from sleepable context.
  - Structure membership does not provide synchronization. Define the lock, owner, reference, or single-thread rule protecting each list and shared condition.
  - Teardown ordering is part of the design, not cleanup polish. Stop producers before consumers and free storage only after all asynchronous users are quiesced.
  - Prefer system workqueues for ordinary short work; allocate a queue only for a concrete ordering, reclaim, freezing, latency, or isolation requirement.
  - Avoid periodic polling when hardware IRQ/event notification exists; avoid hrtimers unless the required precision justifies their constraints.
  - Use subsystem-managed device objects and get/put helpers instead of raw kobjects in normal drivers.
- Interview angles:
  - Explain how `container_of()` works and why embedded objects are common in kernel APIs.
  - Explain why Linux lists are intrusive and how `list_for_each_entry()` recovers the containing structure.
  - Explain `list_add()` versus `list_add_tail()` and why safe traversal is required during deletion.
  - Compare timer, delayed work, and hrtimer by precision, execution context, and ability to sleep.
  - Explain system versus custom workqueues and when `alloc_ordered_workqueue()` matters.
  - Explain the difference among queueing, canceling, flushing, and destroying work.
  - Compare wait queues and completions, including condition-based waiting versus completion-event consumption.
  - Explain why interruptible wait return values must be checked and how ignoring them can crash a driver.
  - Explain whether `complete()` may run in IRQ context and whether `wait_for_completion()` may.
  - Describe a safe remove path for a driver containing an IRQ, timer, work item, wait queue, list node, and completion.
  - Explain what a kobject contributes and why a normal driver usually uses `struct device` rather than raw kobject APIs.
