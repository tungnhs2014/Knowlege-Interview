# 05 - Core Kernel Facilities

## Learning Goal

After this chapter, you should understand the small kernel building blocks that appear inside almost every nontrivial driver.

By the end, you should be able to:

- Explain object embedding and recover a driver object with `container_of()`.
- Build and safely traverse an intrusive kernel linked list.
- Choose among a timer, high-resolution timer, delayed work, wait queue, and completion.
- Explain which callbacks may sleep and which run in atomic context.
- Initialize, publish, quiesce, and destroy asynchronous driver state safely.
- Describe what a kobject contributes without reaching for raw kobject APIs unnecessarily.
- Debug list corruption, lost wakeups, stuck waits, and timer/work use-after-free bugs.

## Why This Matters In Real Work

Drivers rarely execute as one straight function. Hardware interrupts, timeouts, worker threads, userspace reads, DMA callbacks, and device removal all touch shared driver state at different times.

Core kernel facilities solve recurring parts of that problem:

| Driver need | Facility |
| --- | --- |
| Link many driver objects without wrapper allocations | Kernel list |
| Recover the owner of an embedded callback object | `container_of()` and typed wrappers |
| Run a small callback after a deadline | Timer or hrtimer |
| Defer work to sleepable process context | Workqueue |
| Sleep until a state condition becomes true | Wait queue |
| Wait for one asynchronous operation to finish | Completion |
| Participate in kernel object lifetime and hierarchy | Kobject, usually through `struct device` |

These helpers do not design concurrency for you. **A list is not a lock, a wakeup is not shared-state synchronization, and canceling one callback does not automatically stop every producer.**

## Mental Model

Think of a driver's private structure as the owner of several embedded service objects.

```text
struct my_device
  |
  +-- list node       membership in a driver's collection
  +-- timer           "notify me after this deadline"
  +-- work item       "run this later where sleeping is allowed"
  +-- wait queue      place for tasks waiting on state
  +-- completion      event indicating one operation finished
  +-- subsystem object, often struct device
```

The kernel often calls back with only the embedded member. The driver then recovers the enclosing object:

```text
timer callback receives &dev->timer
  -> from_timer() recovers dev
  -> callback accesses dev state
```

This creates one central lifetime rule:

> **The enclosing object must remain alive while any list membership, callback, waiter, completion producer, or external reference can still reach it.**

## Core Concepts

### Object Embedding And `container_of()`

Kernel APIs commonly embed a generic object inside a subsystem-specific or driver-specific object.

```c
struct sample_device {
        int id;
        struct list_head node;
        struct timer_list timeout;
        struct work_struct work;
};
```

Given `&dev->node`, `container_of()` computes the address of `dev` from the member's offset:

```c
dev = container_of(node_ptr, struct sample_device, node);
```

Important rules:

- The pointer must identify the actual embedded member.
- The containing type and member name must match reality.
- A pointer stored *inside* a pointer member is not the address of that member.
- Prefer typed wrappers when an API provides them, such as `from_timer()` or `to_delayed_work()`.
- `container_of()` does not acquire a reference or prove that the owner is still alive.

### Intrusive Linked Lists

Linux lists are **intrusive**: the payload embeds `struct list_head`; the list does not allocate a separate generic node.

```text
head <-> object A.node <-> object B.node <-> head
```

Benefits:

- No wrapper allocation per entry.
- One object can belong to multiple lists by embedding multiple nodes.
- Generic list code does not need to know the payload type.
- Insertion and deletion are constant-time once the position is known.

Costs:

- Membership and object lifetime are tightly coupled.
- Linked lists have poor cache locality compared with arrays.
- Search remains linear.
- The caller must provide locking or another ownership rule.

| Operation | Meaning |
| --- | --- |
| `LIST_HEAD(name)` | Declare and initialize a standalone list head. |
| `INIT_LIST_HEAD(&head)` | Initialize a list head or reusable node at runtime. |
| `list_add(&obj->node, &head)` | Insert after the head, often stack-like. |
| `list_add_tail(&obj->node, &head)` | Insert before the head, often queue-like. |
| `list_del(&obj->node)` | Unlink and poison links for bug detection. |
| `list_del_init(&obj->node)` | Unlink and reinitialize the node. |
| `list_for_each_entry()` | Iterate payload objects. |
| `list_for_each_entry_safe()` | Iterate while deleting the current entry. |

**Safe traversal means structurally safe deletion during that loop. It does not make concurrent access safe.**

### Timers, Hrtimers, And Delayed Work

All three arrange future execution, but their precision and callback context differ.

| Facility | Time model | Callback context | May sleep? | Typical use |
| --- | --- | --- | --- | --- |
| `struct timer_list` | Jiffies, coarse timeout | Timer/softirq context | No | Protocol timeout, watchdog check, deferred trigger |
| `struct hrtimer` | `ktime_t`, high resolution | Atomic timer context in normal use | No | Precision-sensitive deadlines |
| `struct delayed_work` | Work scheduled after jiffies delay | Workqueue process context | Yes | Delayed operation that needs a mutex, allocation, or sleeping I/O |

A timer deadline means "not before this time," not "execute at this exact instant." CPU load, interrupt masking, and worker availability add latency.

Use standard timers for ordinary kernel timeouts. Use hrtimers only when the precision is required. Use delayed work when the delayed callback must sleep.

### Workqueues

A workqueue runs deferred functions using kernel worker threads. The modern implementation is concurrency-managed: worker pools are shared and the workqueue layer controls concurrency.

Key ideas:

- `work_struct` represents one unit of work, not a new thread.
- A pending work item is normally coalesced; repeatedly queueing the same pending item does not create an unbounded number of executions.
- Work callbacks run in process context and may sleep, subject to the queue's contract.
- The system workqueues suit ordinary short work.
- Allocate a private workqueue only for a real ordering, concurrency, reclaim, freezing, latency, or isolation requirement.
- `alloc_ordered_workqueue()` is useful when work items must execute one at a time in queue order.

### Wait Queues

A wait queue supports **condition-based waiting**. The wait queue stores sleeping tasks; the driver's state determines whether they may proceed.

```text
consumer: while condition is false -> sleep on wait queue
producer: update protected state -> wake eligible waiters
consumer: run again -> reevaluate condition
```

The condition may become true many times. `wake_up()` does not evaluate it on behalf of the caller; the `wait_event*()` machinery reevaluates it when a waiter runs.

Rules:

- Publish the new state before calling `wake_up*()`.
- Protect the condition and related data with a suitable lock, atomic operation, or documented memory-ordering scheme.
- Use an interruptible or killable wait for syscall-facing paths where appropriate.
- Check every interruptible wait's return value before using data.
- Waiting is legal only from a sleepable context.

### Completions

A completion represents an asynchronous operation reaching a completion point.

```text
submit operation
  -> waiter calls wait_for_completion*()
  -> IRQ, DMA callback, or worker calls complete()
  -> waiter continues
```

Unlike a wait queue, a completion already owns its event state:

- `complete()` contributes one consumable completion event.
- One waiter consumes one event.
- `complete_all()` releases all current and future waiters until the object is reinitialized.
- `reinit_completion()` resets the state but is safe only when no racing completion event can be lost.
- The completion object must outlive every waiter and every possible caller of `complete*()`.

### Kobject Overview

A `struct kobject` is a low-level building block for kernel object identity, hierarchy, reference counting, release, and sysfs representation.

Related concepts:

| Object | Role |
| --- | --- |
| `struct kobject` | Name, parent relationship, reference-counted identity. |
| `struct kobj_type` | Object type operations, especially final release behavior and sysfs operations. |
| `struct kset` | Collection of related kobjects with common organization. |
| `struct device` | Higher-level device-model object that embeds a kobject. |

Normal drivers should usually use `struct device`, bus/class APIs, and subsystem get/put helpers. Raw kobjects require a correct release callback and carefully designed ownership; they are covered in depth in topic 12.

## Kernel Mechanism

These facilities look unrelated at API level, but they share object embedding, state transitions, and strict lifetime requirements.

### Lists And Owner Recovery

A list head is a circular sentinel. An empty head points to itself. Each embedded node carries links, while iteration macros recover the containing payload with the `container_of()` pattern.

```text
list_for_each_entry(pos, &head, node)
  -> walk list_head links
  -> compute enclosing object for each node
  -> expose typed object as pos
```

List operations update pointers only. They do not:

- Allocate or free payload objects.
- Prevent double insertion or deletion.
- Serialize concurrent readers and writers.
- Keep an object alive after a reader drops its lock.

### Timer Dispatch

`timer_setup()` associates a callback with an embedded `struct timer_list`. `mod_timer()` adds or changes its expiry, commonly expressed as `jiffies + msecs_to_jiffies(ms)`.

```text
initialize timer
  -> arm or modify expiry
  -> timer subsystem marks it pending
  -> deadline passes
  -> callback runs in atomic context
  -> callback returns or rearms timer
```

Use `time_before()`, `time_after()`, and related helpers for wrap-safe jiffies comparisons. Do not compare time with naive arithmetic such as `if (jiffies > deadline)` when wraparound matters.

An hrtimer uses nanosecond-based `ktime_t` values and time-ordered internal structures. Its callback returns `HRTIMER_NORESTART` or `HRTIMER_RESTART`; a periodic callback commonly advances its deadline with `hrtimer_forward_now()`.

### Workqueue Dispatch

`INIT_WORK()` binds a callback to one work item. Queueing makes the item pending; a worker later clears or changes that state and invokes the callback.

```text
IRQ/timer/process queues work
  -> work becomes pending
  -> worker pool selects a worker
  -> callback runs in process context
  -> callback finishes or arranges later work
```

The distinctions matter during teardown:

| Operation | Purpose |
| --- | --- |
| `cancel_work_sync()` | Cancel pending work and wait for a running callback. |
| `cancel_delayed_work_sync()` | Cancel delayed work and wait for completion. |
| `flush_work()` | Wait for a particular queued work item to finish; does not cancel it. |
| `flush_workqueue()` | Wait for work queued on an owned queue before the flush point. |
| `destroy_workqueue()` | Drain and destroy a workqueue allocated by the caller. |

Do not synchronously cancel or flush a work item from a context that creates a dependency on that same work finishing.

### Wait/Wake Dispatch

The `wait_event*()` family repeatedly checks a C condition around scheduler sleep. The macro handles queueing the current task and changing task state, but the driver still owns the shared-state protocol.

For `wait_event_interruptible()`:

```text
condition true       -> return 0
signal arrives first -> return -ERESTARTSYS
```

For interruptible timeout variants:

```text
positive -> condition became true, usually remaining jiffies
0        -> timeout
negative -> interrupted by signal
```

Exact return contracts differ by variant, so check the chosen API rather than treating all wait helpers alike.

### Completion Dispatch

Internally, a completion combines waiters with completion-event state. This prevents the classic timing problem where `complete()` happens just before the waiter starts waiting: the event remains available for the waiter to consume.

`complete()` may be called from atomic or IRQ context. Waiting functions sleep and therefore require process context. Timeout and interruptible variants must have their returns checked.

## Key Structs And APIs

The useful way to learn these APIs is by role: initialize, activate, observe, and quiesce.

### Embedding And Lists

| Role | APIs |
| --- | --- |
| Recover owner | `container_of()`, `list_entry()`, `from_timer()`, `to_delayed_work()` |
| Initialize | `LIST_HEAD()`, `INIT_LIST_HEAD()` |
| Insert | `list_add()`, `list_add_tail()` |
| Inspect | `list_empty()`, `list_first_entry()` |
| Traverse | `list_for_each_entry()`, `list_for_each_entry_safe()` |
| Remove | `list_del()`, `list_del_init()` |

### Timers

| Role | Standard timer | High-resolution timer |
| --- | --- | --- |
| Object | `struct timer_list` | `struct hrtimer` |
| Initialize | `timer_setup()` | `hrtimer_setup()` on kernels that provide it; otherwise `hrtimer_init()` plus callback assignment |
| Start/update | `mod_timer()` | `hrtimer_start()` |
| Check | `timer_pending()` | `hrtimer_active()` |
| Stop | `timer_delete_sync()` or `timer_shutdown_sync()` | `hrtimer_cancel()` |
| Rearm | `mod_timer()` | `hrtimer_forward*()` and return `HRTIMER_RESTART` |
| Time helpers | `jiffies`, `msecs_to_jiffies()`, `time_after()` | `ktime_t`, `ms_to_ktime()`, `ns_to_ktime()` |

Use `timer_shutdown_sync()` when teardown must permanently prevent a timer from being rearmed, especially when timer and work callbacks can rearm each other. Use the timer API available in the target kernel tree.

### Workqueues

| Role | APIs |
| --- | --- |
| Declare/initialize | `DECLARE_WORK()`, `INIT_WORK()`, `INIT_DELAYED_WORK()` |
| Use system queues | `schedule_work()`, `schedule_delayed_work()` |
| Allocate queue | `alloc_workqueue()`, `alloc_ordered_workqueue()` |
| Queue owned work | `queue_work()`, `queue_delayed_work()` |
| Stop/wait | `cancel_work_sync()`, `cancel_delayed_work_sync()`, `flush_work()` |
| Destroy owned queue | `destroy_workqueue()` |

Common allocation flags include `WQ_UNBOUND`, `WQ_MEM_RECLAIM`, and `WQ_FREEZABLE`, but each changes behavior and should be selected from a concrete requirement.

### Wait Queues And Completions

| Facility | Initialize | Wait | Signal |
| --- | --- | --- | --- |
| Wait queue | `DECLARE_WAIT_QUEUE_HEAD()`, `init_waitqueue_head()` | `wait_event()`, `wait_event_interruptible()`, timeout/killable variants | `wake_up()`, `wake_up_interruptible()`, `wake_up_all()` |
| Completion | `DECLARE_COMPLETION()`, `init_completion()` | `wait_for_completion()`, interruptible/killable/timeout variants | `complete()`, `complete_all()` |

### Kobjects

| Role | APIs / fields |
| --- | --- |
| Reference ownership | `kobject_get()`, `kobject_put()` |
| Hierarchy | `kobj->parent`, normally managed through higher-level APIs |
| Type and release | `struct kobj_type`, `.release` |
| Grouping | `struct kset` |
| Preferred driver level | `struct device` and subsystem-specific registration/get/put APIs |

## Lifecycle / Data Flow

Initialization and removal must be designed together. Publishing an object before every callback facility is initialized creates a race; freeing it before callbacks and references are gone creates a use-after-free.

### Initialization

```text
allocate enclosing driver object
  -> initialize locks and protected state
  -> initialize list nodes, wait queues, completions
  -> initialize timer and work objects
  -> acquire hardware resources
  -> register with subsystem / insert in protected collection
  -> enable IRQs or hardware producers
  -> expose interface to userspace
```

### Runtime Event

```text
hardware/producer event
  -> update state under the chosen synchronization rule
  -> optionally arm timer or queue work
  -> wake condition waiters or complete one operation
  -> consumer checks wait return
  -> consumer safely reads/consumes protected state
```

### Removal

```text
mark device stopping and reject new operations
  -> remove userspace/subsystem entry points as required
  -> disable hardware and IRQ producers
  -> permanently stop timers
  -> cancel or drain owned work
  -> resolve/wake waiters so they cannot hang
  -> unlink from protected lists
  -> drop subsystem/object references
  -> free enclosing object
```

The exact order depends on who can create more work. For example, if a timer queues work, stop the timer before canceling the work. If work can rearm the timer, use a stopping state plus a timer shutdown API that prevents rearming.

## Minimal Practical Example

This **learning-only** example demonstrates object embedding and asynchronous flow. It omits registration, complete locking, module plumbing, hardware error handling, and multi-open lifetime management, so it is not production-ready.

```c
#include <linux/completion.h>
#include <linux/jiffies.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

struct facility_demo {
        struct list_head node;
        struct timer_list timer;
        struct work_struct work;
        wait_queue_head_t readq;
        struct completion finished;
        bool ready;
        bool stopping;
        unsigned int value;
};

static void demo_work_fn(struct work_struct *work)
{
        struct facility_demo *demo =
                container_of(work, struct facility_demo, work);

        if (READ_ONCE(demo->stopping))
                return;

        /* A real driver must synchronize value and ready consistently. */
        demo->value++;
        WRITE_ONCE(demo->ready, true);
        wake_up_interruptible(&demo->readq);
        complete(&demo->finished);
}

static void demo_timer_fn(struct timer_list *timer)
{
        struct facility_demo *demo = from_timer(demo, timer, timer);

        if (!READ_ONCE(demo->stopping))
                schedule_work(&demo->work);
}

static void demo_init(struct facility_demo *demo)
{
        INIT_LIST_HEAD(&demo->node);
        INIT_WORK(&demo->work, demo_work_fn);
        init_waitqueue_head(&demo->readq);
        init_completion(&demo->finished);
        timer_setup(&demo->timer, demo_timer_fn, 0);

        demo->ready = false;
        demo->stopping = false;
        demo->value = 0;

        mod_timer(&demo->timer, jiffies + msecs_to_jiffies(100));
}

static int demo_wait_for_data(struct facility_demo *demo)
{
        int ret;

        ret = wait_event_interruptible(demo->readq,
                        READ_ONCE(demo->ready) ||
                        READ_ONCE(demo->stopping));
        if (ret)
                return ret;
        if (READ_ONCE(demo->stopping))
                return -ENODEV;

        return 0;
}

static void demo_stop(struct facility_demo *demo)
{
        WRITE_ONCE(demo->stopping, true);

        /* Stop the producer before draining the consumer. */
        timer_shutdown_sync(&demo->timer);
        cancel_work_sync(&demo->work);

        /* Let condition waiters observe shutdown instead of hanging. */
        wake_up_interruptible_all(&demo->readq);

        if (!list_empty(&demo->node))
                list_del_init(&demo->node);
}
```

What the example demonstrates:

- `from_timer()` and `container_of()` recover the enclosing object.
- The timer callback performs no sleeping operation; it only queues work.
- Work runs in process context and publishes a condition before wakeup.
- A completion independently records that the work reached a milestone.
- The wait includes the stopping condition, so removal can release sleepers.
- Teardown stops the timer before canceling the work it can produce.

What production code must add:

- One consistent lock or atomic-state design for `value`, `ready`, and consumption.
- A protected rule for list insertion/removal.
- Handling for repeated operations and completion reinitialization.
- Reference/lifetime control for callers that may outlive device removal.
- Error paths mirroring every successful initialization step.
- Compatibility handling if the target kernel predates `timer_shutdown_sync()`.

## Common Bugs And Debugging

Start debugging from the observed state: what is pending, what is running, what condition is false, and who still owns the enclosing object.

| Symptom | Likely causes | Evidence and fix |
| --- | --- | --- |
| List corruption or crash in list macros | Double add/delete, uninitialized node, concurrent mutation, freed linked object | Audit every membership transition; enable list debugging where available; verify lock and lifetime rules. |
| Timer callback crashes after unload | Object freed before synchronous shutdown, callback rearmed during teardown | Stop producers, use synchronous timer shutdown, then free only after callback users are gone. |
| Work runs after remove | Pending/running work not canceled, IRQ or timer can still queue it | Disable all producers first; then `cancel_*_sync()` or drain owned work. |
| Repeated queue requests produce fewer callbacks | Same work item was already pending | Treat queue return/state correctly; store event counts separately if every event matters. |
| Task sleeps forever | Producer stopped, condition never published, wakeup missing, teardown omitted stopping condition | Inspect sleeping task stacks and state transitions; wake only after publishing state; release waiters on removal. |
| Read proceeds with invalid data | Return from interruptible wait ignored | Handle `-ERESTARTSYS`, timeout, and shutdown before touching data. |
| Completion wait unexpectedly returns immediately | Old completion token remains or `complete_all()` was not reinitialized | Define one operation lifecycle; reinitialize only when no completion can race. |
| Completion event disappears | `reinit_completion()` raced with `complete()` | Fix ownership and ordering; do not reset while a producer may signal. |
| Atomic-context warning | Timer/hrtimer callback called a sleeping API or took a mutex | Move sleepable work to a workqueue; use context-appropriate locking. |
| Deadlock in remove | Synchronous cancel/flush waits on work that waits on remove-held lock | Release dependency locks before cancel/flush; map the full wait-for graph. |

Useful debugging tools and techniques:

- Add targeted `pr_debug()` or `dev_dbg()` state transitions: queued, pending, running, stopping, canceled.
- Use dynamic debug to enable those messages without permanently flooding logs.
- Inspect blocked task stacks with SysRq task dumps or crash analysis.
- Use KASAN for use-after-free and out-of-bounds bugs.
- Use lockdep for invalid lock ordering and sleep-in-atomic mistakes.
- Enable kernel debug-object support for timer and work lifecycle misuse when available.
- Use ftrace or tracepoints for timer, workqueue, IRQ, and scheduler ordering when logs perturb timing.
- For lists, search for every add, delete, traversal, free, and lock site rather than inspecting only the crash.

**Logging changes timing.** A race that disappears under heavy `printk()` calls is still a race; use tracing or sparse state logging for timing-sensitive failures.

## Production Checklist

Before review or device bring-up, verify the complete ownership and asynchronous lifecycle.

### Object And List Ownership

- [ ] Every embedded member is initialized before any external path can reach it.
- [ ] Each list has a documented lock, single-owner rule, or other synchronization scheme.
- [ ] Deletion during iteration uses the appropriate safe iterator.
- [ ] No object is freed while linked or reachable by a reader.
- [ ] `container_of()` type and member arguments match the actual embedded object.

### Context And Scheduling

- [ ] Timer and hrtimer callbacks use only atomic-context-safe operations.
- [ ] Sleepable operations are moved to process context, commonly a workqueue.
- [ ] A private workqueue exists only for a concrete behavior requirement.
- [ ] Repeated event counts are not accidentally lost through work coalescing.
- [ ] Time comparisons use conversion and wrap-safe helpers.

### Waiting And Signaling

- [ ] Condition state is published before `wake_up*()`.
- [ ] Condition data uses one coherent locking or memory-ordering design.
- [ ] Interruptible, killable, and timeout return values are handled exactly.
- [ ] Removal makes every potentially indefinite waiter terminate.
- [ ] Completion storage outlives all waiters and completers.
- [ ] `complete()`, `complete_all()`, and `reinit_completion()` match the intended event model.

### Teardown

- [ ] New operations are rejected before teardown dismantles their dependencies.
- [ ] Hardware and IRQ producers are disabled before timer/work consumers are drained.
- [ ] Self-rearming timers are permanently shut down where required.
- [ ] Pending and running work is synchronously canceled or drained.
- [ ] Synchronous teardown is not called while holding a lock needed by the callback.
- [ ] List membership and external references are released before final free.
- [ ] Probe error paths apply the same reverse-order lifetime rules as remove.

### Device Model

- [ ] Higher-level subsystem objects are used instead of raw kobjects where possible.
- [ ] Every acquired reference has a defined matching put.
- [ ] Final release owns the actual memory-free decision.

## Interview Readiness

You should be able to reason through these facilities as one lifetime system, not recite isolated APIs.

Be ready to explain:

- Why object embedding is common and how `container_of()` works.
- Why kernel lists are intrusive and why safe deletion iteration is different from concurrency safety.
- The context and precision differences among timers, hrtimers, and delayed work.
- Why a workqueue is not one dedicated thread per work item.
- The difference among queueing, canceling, flushing, and destroying work.
- Why wait queues are condition-based while completions represent operation events.
- Why ignoring `wait_event_interruptible()` return values can lead to invalid data access.
- How `complete()` can run in IRQ context while `wait_for_completion()` cannot.
- Why `complete()` is consumable and why `complete_all()` needs careful reuse.
- A safe remove order for a driver containing IRQs, a timer, work, waiters, a list node, and external references.
- What kobjects provide and why normal drivers usually work through `struct device`.

Practice with [05-core-kernel-facilities.md](../interview/05-core-kernel-facilities.md) after you can draw the runtime and teardown flows from memory.

## Kernel Version Notes

- `timer_setup()` and callbacks receiving `struct timer_list *` are the modern baseline; old `setup_timer()` callback-data examples are obsolete.
- Current kernels provide `timer_delete_sync()` and `timer_shutdown_sync()`; older trees commonly use `del_timer_sync()`. Check the headers of the exact target kernel.
- Some newer trees provide `hrtimer_setup()` helpers, while older supported kernels use `hrtimer_init()` followed by callback assignment.
- Workqueue internals are concurrency-managed. Avoid learning the obsolete model that every created workqueue necessarily owns one dedicated thread.
- Kobject structure fields are implementation details. Use public lifetime and subsystem APIs rather than copying layouts from older kernel versions.
