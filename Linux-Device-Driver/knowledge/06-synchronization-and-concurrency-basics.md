# 06 - Synchronization And Concurrency Basics

## Learning Goal

After this topic, you should be able to identify shared driver state, list every
context that can touch it, select a mutex, spinlock, or atomic operation for the
actual constraint, and design init/error/remove paths that cannot race with live
callbacks.

## Why This Matters In Real Work

Most concurrency bugs are not caused by forgetting a lock API. They come from an
incomplete ownership model: one path protects a field, another path does not, an
IRQ uses a sleeping operation, or teardown frees state while a callback still
uses it.

Typical symptoms include:

- counters or queue indices changing unpredictably;
- partially updated state visible to another CPU;
- rare use-after-free during unload or probe failure;
- `BUG: sleeping function called from invalid context`;
- hard lockups, soft lockups, hung tasks, or lockdep splats;
- failures that disappear when extra logging changes timing.

## Mental Model

Treat synchronization as a design exercise with four questions:

1. **What state has an invariant?**
2. **Which execution contexts can access it?**
3. **Can any protected operation sleep?**
4. **How is the object kept alive while those contexts run?**

A lock does not protect code by itself. It protects the data that every accessor
consistently reaches through that lock.

```text
shared state
  -> enumerate readers and writers
  -> identify process, work, timer, softirq, and hard-IRQ contexts
  -> define the invariant and ownership
  -> select the primitive and exact lock scope
  -> define lock order
  -> quiesce users before freeing the state
```

### Race Condition Versus Data Race

- A **race condition** means correctness depends on timing or interleaving.
- A **data race** is concurrent conflicting memory access where at least one
  access is a write and the accesses lack proper synchronization.
- A design can have a race condition without an obvious same-address data race.
  For example, two individually atomic fields can still represent an impossible
  pair of values.

### Critical Section

A critical section is the region in which an invariant is inspected or changed.
Keep it understandable and as small as practical, but do not split one invariant
across multiple lock acquisitions merely to reduce line count.

## Core Concepts

### Primitive Selection At A Glance

Start with the contexts and the state relationship. Lock hold time matters for
latency and contention, but it does not make an otherwise illegal primitive
legal.

| Need | Usual Primitive | Why | Do Not Use It For |
| --- | --- | --- | --- |
| Task-only state; protected work may sleep | `struct mutex` | Waiters may sleep; ownership is explicit | Hard IRQ, softirq, or timer access |
| State shared with an atomic context; protected work cannot sleep | `spinlock_t` | Serializes CPUs without a sleeping wait | Bus I/O, user copies, allocation with `GFP_KERNEL`, long work |
| One independent scalar read-modify-write value | `atomic_t` / `atomic64_t` | Makes that scalar operation indivisible | Multi-field invariants or object ownership |
| Object lifetime references | `refcount_t` or subsystem lifetime API | Expresses reference-count intent and catches misuse | General statistics or unrelated state |
| Rare, mostly read state with specialized requirements | RCU, seqlock, percpu, or another advanced primitive | Can reduce contention for a proven access pattern | A beginner default or an unmeasured optimization |

This gives a practical decision sequence:

```text
Can any accessor run in hard IRQ, softirq, or timer context?
  yes -> protected operations must not sleep; consider a spinlock
  no  -> can the protected operation sleep?
          yes -> use a mutex
          no  -> a mutex is still often simplest unless atomic context requires otherwise

Is the state only one independent scalar operation?
  yes -> an atomic operation may be sufficient
  no  -> protect the whole invariant with one coherent locking rule
```

### Execution Context Comes First

Lock selection begins with where the code can run, not a guessed duration.

| Context | May Sleep? | Typical Driver Examples |
| --- | --- | --- |
| Process context | Yes, unless atomic state was entered | syscall callback, `probe()`, `remove()`, kthread |
| Workqueue callback | Yes | deferred bus I/O, firmware work |
| Threaded IRQ handler | Yes, subject to subsystem rules | I2C/SPI interrupt processing |
| Hard IRQ handler | No | acknowledge device, capture status |
| Softirq/tasklet | No | atomic bottom-half work |
| Conventional timer callback | No | short state update, queue later work |

**A function running in process context can still be atomic temporarily**, for
example while it holds a spinlock or has disabled interrupts/preemption.

Operations that may sleep include:

- taking a mutex that may contend;
- `msleep()` and scheduler waits;
- `copy_to_user()` or `copy_from_user()`, which may fault;
- `kmalloc(..., GFP_KERNEL)`;
- many I2C/SPI, GPIO `_cansleep`, regulator, clock, and PM operations.

### Protect Invariants, Not Variables In Isolation

Suppose `head`, `tail`, and `count` describe one ring buffer. Protecting each
field independently does not preserve their relationship.

```text
Invariant:
  count == number of entries between tail and head

Wrong:
  atomic head + atomic tail + atomic count

Normal design:
  one lock protects the complete queue state transition
```

Write the protection rule near the data:

```c
struct demo_state {
        /* state_lock protects head, tail, count, and stopping. */
        spinlock_t state_lock;
        unsigned int head;
        unsigned int tail;
        unsigned int count;
        bool stopping;
};
```

### Mutex

A mutex is a **sleeping, owner-based lock**. A contending task may sleep until
the owner releases it, so a mutex is the normal choice for shared state reached
only from sleepable task context.

Use a mutex when:

- every accessor runs in sleepable process context;
- the protected operation calls an API that may sleep;
- a userspace operation, kthread, work item, or threaded IRQ shares state;
- there is no hard-IRQ, softirq, or timer accessor to the same lock.

Important rules:

- only the acquiring task may unlock it;
- mutexes are not recursive;
- do not use them in hard IRQ, softirq/tasklet, or timer context;
- initialize them through the API and never copy or reinitialize a live mutex;
- keep the mutex-containing memory alive through the final unlock.

`mutex_lock()` is normal for internal paths that must complete. Use
`mutex_lock_interruptible()` when signal interruption has useful caller-visible
semantics, then propagate or deliberately translate its error.

### Spinlock

A conventional `spinlock_t` protects short, non-sleeping critical sections. On a
non-`PREEMPT_RT` kernel, holding it disables preemption locally; a contending CPU
spins instead of sleeping.

Use a spinlock when:

- a hard IRQ, softirq/tasklet, or timer accesses the state;
- the protected operation cannot sleep;
- process and atomic contexts share a small state transition;
- inter-CPU serialization is required in atomic context.

**Never sleep while holding a conventional spinlock.** Move user copies, bus
transactions, sleepable allocation, and slow computation outside the lock.

If one operation needs both atomic-context state protection and sleepable I/O,
split it deliberately:

```text
spin_lock
  -> reserve work or snapshot/update the shared state
spin_unlock
  -> perform sleepable I/O in process/threaded context
mutex_lock, if the sleepable transaction itself needs serialization
  -> complete the transaction and publish the result
mutex_unlock
```

The split must preserve ownership. Do not unlock, perform I/O, and later assume
the object or queue entry is still valid without a reservation, reference, or
state transition that keeps it yours.

### Spinlock Variant Selection

Choose the variant from all contexts that share the data.

| Shared Contexts | Common Choice | Reason |
| --- | --- | --- |
| Process/task only, non-sleeping section | `spin_lock()` | Inter-CPU exclusion and local preemption control |
| Process plus softirq/timer | `spin_lock_bh()` in process context | Prevent local bottom-half self-deadlock |
| Process plus hard IRQ | `spin_lock_irqsave()` in process context | Preserve IRQ state and prevent local IRQ self-deadlock |
| Hard IRQ plus another CPU/context | `spin_lock()` or IRQ-state variant as design requires | IRQ context is already atomic, but architecture and nesting matter |

Canonical process/IRQ sharing pattern:

```c
unsigned long flags;

spin_lock_irqsave(&d->state_lock, flags);
snapshot = d->status;
d->pending = false;
spin_unlock_irqrestore(&d->state_lock, flags);
```

Do not mechanically use `_irqsave` everywhere. Interrupt disabling increases
latency and does not replace a clear context inventory.

### Atomic Operations

`atomic_t`, `atomic64_t`, and `atomic_long_t` provide atomic operations on one
scalar value, especially read-modify-write operations such as increment,
decrement, exchange, and compare-exchange.

Useful examples:

```c
atomic64_set(&d->irq_count, 0);
atomic64_inc(&d->irq_count);
count = atomic64_read(&d->irq_count);
```

Good candidates:

- independent statistics;
- a simple event or usage counter whose value is self-contained;
- a single state transition designed around compare-exchange.

Bad candidates:

- several fields that must change as one transaction;
- ownership of a list, buffer, or device lifecycle;
- object reference counting with generic atomics;
- an attempt to avoid understanding memory ordering.

For object lifetime, prefer `refcount_t` and its `refcount_*()` APIs. They encode
reference-count intent and add protections that generic atomics do not.

### `READ_ONCE()` And `WRITE_ONCE()`

These helpers force one marked access and constrain compiler transformations.
They are useful in carefully designed lockless protocols and for documenting
concurrent observation.

They do **not** by themselves:

- make `x++` atomic;
- protect a multi-field invariant;
- provide mutual exclusion;
- provide every required CPU ordering guarantee;
- keep an object alive.

For ordinary driver control state, a lock is usually clearer and easier to
verify.

### Atomicity Is Not Memory Ordering

An operation can be indivisible without publishing surrounding data in the
required order. Locks normally provide the ordering needed around the protected
critical section. Custom acquire/release or barrier protocols require a precise
Linux Kernel Memory Model argument and should not be introduced casually.

### Locks Versus Waiting And Signaling

A lock protects shared state while it is inspected or changed. A wait queue or
completion lets one context wait for another context to make progress. These
roles complement each other but are not interchangeable.

| Need | Mechanism |
| --- | --- |
| Keep a predicate and its payload coherent | Mutex, spinlock, atomic protocol, or another documented synchronization rule |
| Sleep until a reusable condition may be true | Wait queue and `wait_event*()` |
| Signal that one asynchronous operation reached a completion point | `struct completion` |

For a wait queue, update the protected state before calling `wake_up*()`. The
waiter must reevaluate the condition because wakeups do not reserve data and
another consumer may win the race.

If the wait condition is inspected without taking its protecting lock, every
concurrent access to that predicate needs a deliberate marked-access or atomic
protocol, commonly `READ_ONCE()` paired with `WRITE_ONCE()`. The waiter still
needs the lock to validate the predicate and reserve or consume its payload.

```text
producer:
  lock -> update payload and ready predicate -> unlock -> wake_up

consumer:
  wait_event(predicate or stopping)
    -> lock -> recheck predicate -> consume/reserve payload -> unlock
```

The wait queue's containing object and the predicate storage must remain alive
until every waiter has stopped using them. Detailed wait queue and completion
APIs are covered in topic 05; the synchronization rule belongs here.

## Kernel Mechanism

### Mutex Contention

```text
Task A acquires mutex
  -> Task B attempts same mutex
  -> Task B may sleep
  -> scheduler runs other work
  -> Task A unlocks
  -> a waiter becomes runnable
```

The API contract matters more than the current internal fields of
`struct mutex`.

### Spinlock Contention

```text
CPU 0 acquires spinlock
  -> local preemption handling follows lock type/configuration
CPU 1 attempts same spinlock
  -> CPU 1 waits according to spinlock implementation
CPU 0 releases
  -> CPU 1 can acquire
```

If process context holds a plain spinlock and a local IRQ preempts it and tries
the same lock, the IRQ cannot make progress and the preempted task cannot release
the lock. The process-side acquisition must prevent that local IRQ path, commonly
with `spin_lock_irqsave()`.

### What The Kernel Objects Represent

Drivers normally embed synchronization objects in per-device state:

```c
struct demo_device {
        struct mutex config_lock;
        spinlock_t state_lock;
        atomic64_t irq_count;
        refcount_t users;
};
```

- `struct mutex` records owner/waiter state needed by the mutex implementation.
- `spinlock_t` is the normal kernel spinlock type; its implementation depends on
  kernel configuration, architecture, and `PREEMPT_RT`.
- `atomic_t` and `atomic64_t` wrap scalar atomic operations; their internal
  representation is not a driver-facing synchronization protocol.
- `refcount_t` is a specialized reference counter for object lifetime.

Treat these types as kernel API objects. Do not inspect their fields, copy them,
zero a live object, or reinitialize one while another context can use it.

### What CPU Locks Do Not Serialize

A mutex or spinlock coordinates participating CPU contexts. It does not
automatically:

- stop a device or DMA engine from changing memory;
- flush posted MMIO writes or replace device accessors and required barriers;
- make a transfer buffer live until asynchronous hardware completion;
- serialize a subsystem operation that requires its own bus or framework lock;
- make an object safe after teardown has removed its last lifetime reference.

Real drivers often need both a CPU locking rule and a hardware ownership or
completion rule.

### Locking And Lifetime

A lock serializes accesses to live storage. It does not make the storage live.

Safe teardown generally follows:

```text
mark stopping
  -> stop hardware/IRQ/timer producers
  -> prevent new work from being queued
  -> synchronize or cancel callbacks
  -> wake/finish blocked tasks as required
  -> wait for users or references
  -> remove visibility
  -> free state containing locks
```

## Key Structs And APIs

### Mutex APIs

| API | Purpose |
| --- | --- |
| `DEFINE_MUTEX(name)` | Static declaration and initialization |
| `mutex_init(&lock)` | Initialize an embedded/dynamic mutex |
| `mutex_lock(&lock)` | Acquire, sleeping if required |
| `mutex_lock_interruptible(&lock)` | Acquire or return on a signal |
| `mutex_lock_killable(&lock)` | Acquire or return for fatal signals |
| `mutex_trylock(&lock)` | Attempt immediately; still a mutex-context API |
| `mutex_unlock(&lock)` | Owner releases the mutex |

### Spinlock APIs

| API | Purpose |
| --- | --- |
| `DEFINE_SPINLOCK(name)` | Static declaration and initialization |
| `spin_lock_init(&lock)` | Initialize an embedded/dynamic spinlock |
| `spin_lock()` / `spin_unlock()` | Basic spinlock pair |
| `spin_lock_bh()` / `spin_unlock_bh()` | Disable/enable local bottom halves around lock |
| `spin_lock_irq()` / `spin_unlock_irq()` | Disable/enable local hard IRQs around lock |
| `spin_lock_irqsave()` / `spin_unlock_irqrestore()` | Save, disable, and restore local IRQ state |
| `spin_trylock()` | One immediate acquisition attempt |

Use the exact matching unlock form. The `flags` used by `_irqsave` must be a
local `unsigned long` paired with the corresponding restore.

### Atomic And Reference APIs

| API Family | Purpose |
| --- | --- |
| `atomic_set/read()` | Set/read an `atomic_t` |
| `atomic_inc/dec/add/sub()` | Atomic scalar update |
| `atomic_*_return()` | Update and return the new value |
| `atomic_cmpxchg()` | Conditional scalar transition |
| `atomic64_*()` | 64-bit atomic counterpart |
| `refcount_set/inc/dec_and_test()` | Object reference counting |

Do not use atomic operations on MMIO registers. Use the correct device accessors
and subsystem ordering rules.

### Context And Lock-State Checks

These helpers are mainly assertions and diagnostics, not substitutes for a
designed context contract:

| API | Typical Use |
| --- | --- |
| `lockdep_assert_held(&lock)` | State that a helper requires a caller-held lock |
| `lockdep_assert_not_held(&lock)` | Catch a forbidden call under a lock |
| `might_sleep()` | Warn on debug kernels if a sleepable path is called atomically |
| `WARN_ON_ONCE(condition)` | Report a violated invariant without repeated log floods |

Avoid choosing behavior dynamically from broad checks such as
`in_interrupt()`. A callback's documented context should determine which API it
uses; hidden context-dependent behavior is difficult to review and test.

Assertions turn an ownership rule into executable documentation on debug builds.
They complement, rather than replace, correct synchronization.

## Lifecycle / Data Flow

### Initialization

```text
allocate private state
  -> initialize mutexes, spinlocks, atomics, and refcounts
  -> initialize callback objects
  -> publish driver state
  -> enable hardware/IRQs/producers last
```

No callback should observe an uninitialized lock or partially initialized state.

### Shared-State Update

```text
enter allowed context
  -> acquire the lock selected for all accessors
  -> validate current state
  -> perform the complete invariant transition
  -> release lock
  -> perform slow or sleeping follow-up outside a spinlock
```

### Process Path Sharing State With A Hard IRQ

This common driver flow explains why both the data transition and spinlock
variant matter:

```text
process context
  -> spin_lock_irqsave(state_lock)
  -> reserve a request and publish state visible to the IRQ
  -> spin_unlock_irqrestore(state_lock)
  -> start hardware using the subsystem's required ordering

hard IRQ
  -> acknowledge/capture device status
  -> spin_lock(state_lock)
  -> move the request from active to complete
  -> spin_unlock(state_lock)
  -> wake a waiter or queue sleepable follow-up work

process/threaded context
  -> consume the completed request
  -> perform any bus I/O, user copy, or cleanup that may sleep
```

The process side disables local IRQs while taking `state_lock` so its own IRQ
handler cannot preempt it and spin forever on the same lock. The IRQ handler
must keep its locked work bounded and non-sleeping.

### Error And Remove

```text
stop new entrants
  -> quiesce every producer
  -> cancel/synchronize asynchronous users
  -> acquire locks only for final state unlink/update
  -> verify no references remain
  -> free the enclosing object
```

Do not call a synchronous cancellation helper while holding a lock that the
callback needs; that creates a wait cycle.

Probe failure follows the same reverse-order rule as removal. Unwind only the
resources that became visible, disable or synchronize every producer before
freeing its state, and make partially initialized callbacks impossible.

## Minimal Practical Example

This learning-only pattern uses one mutex for a two-field invariant, one spinlock
for a short scalar update, and one atomic for an independent statistic:

```c
struct demo_state {
        struct mutex pair_lock;
        spinlock_t fast_lock;
        atomic64_t events;
        u64 left;
        u64 right;
        u64 fast_count;
};

static void demo_update(struct demo_state *d)
{
        mutex_lock(&d->pair_lock);
        d->left++;
        d->right++;
        mutex_unlock(&d->pair_lock);

        spin_lock(&d->fast_lock);
        d->fast_count++;
        spin_unlock(&d->fast_lock);

        atomic64_inc(&d->events);
}
```

The full buildable example is in
`examples/06-synchronization-and-concurrency-basics/`.

It intentionally does not include an unlocked racing counter. Demonstrating a
race by shipping undefined concurrent C accesses would obscure the correct
pattern and can produce misleading results.

## Common Bugs And Debugging

### Lost Updates Or Impossible State

Likely causes:

- plain `counter++` from multiple contexts;
- one accessor forgot the lock;
- related fields use different locks;
- atomics were applied independently to a multi-field invariant;
- a lockless reader lacks a valid ordering protocol.

Debug approach:

- list every reader and writer with its execution context;
- annotate each field with its protecting lock;
- search for accesses outside that lock;
- enable KCSAN to sample data races;
- add temporary invariant checks under the protecting lock.

Useful first commands on a debug system:

```sh
dmesg -T | tail -200
journalctl -k -b | tail -200
```

Driver-internal mutexes and spinlocks are diagnosed through stacks, lockdep,
tracing, and targeted instrumentation. `/proc/locks` reports userspace-visible
file locks, not these kernel locking objects.

### Sleeping In Atomic Context

Typical report:

```text
BUG: sleeping function called from invalid context
```

Inspect:

- held spinlocks and IRQ/preemption state in the stack trace;
- mutex, user-copy, allocation, bus, GPIO, PM, or sleep calls below the lock;
- callbacks running in hard IRQ, softirq, or timer context.

Fix by moving sleepable work to process context and moving it outside the
spinlocked section.

### Deadlock Or Hung Task

Common patterns:

- recursive mutex or spinlock acquisition;
- path A takes lock X then Y while path B takes Y then X;
- process context uses plain `spin_lock()` while a local IRQ takes the same lock;
- synchronous cancellation waits for a callback while holding its lock;
- an error path returns without unlocking.

Use:

- `CONFIG_LOCKDEP` and `CONFIG_PROVE_LOCKING`;
- full kernel stack traces and blocked-task owners;
- consistent global lock ordering;
- `lockdep_assert_held()` in helpers that require a caller-held lock.

Capture all task stacks when the machine is still responsive:

```sh
echo w | sudo tee /proc/sysrq-trigger
echo t | sudo tee /proc/sysrq-trigger
```

`w` reports blocked tasks; `t` dumps task stacks. Use these only where SysRq is
enabled and operational policy permits it. Preserve the first lockdep report:
later warnings are often consequences of the first invalid dependency.

### Soft Lockup Or IRQ Latency

Likely causes:

- a spinlock is held across slow loops or I/O;
- excessive work is performed with interrupts disabled;
- contention is high because one global lock protects unrelated state;
- logging or debugging code was added inside an atomic critical section.

Measure first. Split the protected data only when the new ownership rules remain
clear.

On a kernel with tracefs and the relevant tracers enabled:

```sh
sudo sh -c 'echo irqsoff > /sys/kernel/tracing/current_tracer'
sudo sh -c 'echo 1 > /sys/kernel/tracing/tracing_on'
# Reproduce briefly.
sudo sh -c 'echo 0 > /sys/kernel/tracing/tracing_on'
sudo cat /sys/kernel/tracing/trace
```

Use `preemptoff` or `preemptirqsoff` when that better matches the suspected
latency. Keep capture windows short because tracing changes timing and can
produce large output.

### Use-After-Free During Remove

A lock cannot fix this after the object is freed. Audit:

- IRQ and timer synchronization;
- work cancellation and requeue paths;
- open file references and callbacks;
- whether teardown stops producers before freeing state;
- whether the lock itself resides in freed memory.

Use KASAN for lifetime corruption and lockdep/KCSAN for related ordering and race
evidence.

### Tool Selection

| Symptom | Strong First Tool | What It Can Show |
| --- | --- | --- |
| Invalid lock order or recursive dependency | lockdep / `CONFIG_PROVE_LOCKING` | Dependency chain and acquisition stacks |
| Suspected unsynchronized memory access | KCSAN | Sampled conflicting accesses |
| Use-after-free or out-of-bounds access | KASAN | Faulting access and allocation/free stacks |
| Long IRQ-disabled or non-preemptible section | `irqsoff`, `preemptoff`, `preemptirqsoff` tracers | Worst latency path and stack |
| Hung task or deadlock | hung-task report, SysRq task dumps | Blocked stack and wait location |
| Wrong lock ownership contract | lockdep assertions and temporary invariant checks | Earliest local contract violation |

No one tool proves a design race-free. Reproduce under load, inspect the first
failure, and compare every access path against the documented ownership rule.

## Production Checklist

- [ ] Every shared field has a documented owner or protecting primitive.
- [ ] Every reader and writer uses the same rule.
- [ ] All access contexts are listed, including teardown and error paths.
- [ ] No mutex is reachable from hard IRQ, softirq/tasklet, or timer context.
- [ ] No spinlocked section can sleep, fault, or call an unknown sleepable API.
- [ ] The spinlock variant matches the contexts sharing the data.
- [ ] IRQ save/restore pairs use the same local flags value.
- [ ] Multi-field invariants are changed under one coherent protection rule.
- [ ] Atomics are limited to independent scalar state.
- [ ] Object references use `refcount_t` or a subsystem lifetime API.
- [ ] A single global lock has not accidentally serialized unrelated devices.
- [ ] Nested locks have one documented order on every path.
- [ ] Synchronous teardown does not wait while holding a callback-needed lock.
- [ ] Producers are stopped before callbacks are synchronized and state is freed.
- [ ] Lock storage remains alive through the final unlock.
- [ ] CPU locking is paired with the required DMA, MMIO, bus, and hardware ownership rules.
- [ ] Probe-error unwinding quiesces every resource that was already published.
- [ ] Userspace copies and stable ABI behavior are not exposed to partially updated state.
- [ ] Debug builds have been exercised with lockdep; race-prone changes use KCSAN.
- [ ] Hot paths were measured before adding complex lockless synchronization.

## Interview Readiness

You should be able to:

- derive lock choice from execution context and sleepability;
- explain why `_irqsave` prevents a local process/IRQ self-deadlock;
- distinguish atomic scalar operations from invariant protection;
- describe ABBA deadlock and a lock-order policy;
- explain why synchronization and object lifetime are separate;
- walk through process/IRQ shared-state locking and teardown ordering;
- explain why a CPU lock does not by itself synchronize DMA or MMIO;
- debug sleep-in-atomic, lockdep, KCSAN, and teardown-race reports.

Practice with `interview/06-synchronization-and-concurrency-basics.md`.

## Kernel Version Notes

- `spinlock_t` behavior changes on `PREEMPT_RT`; do not assume every RT
  `spinlock_t` is a strict busy-wait lock. `raw_spinlock_t` remains the low-level
  strict spinning primitive. Most device interrupts are forced-threaded on RT;
  code that truly remains in hard-IRQ context must use lock types and APIs legal
  in that context. Do not mechanically replace driver spinlocks with raw
  spinlocks: use them only where strict non-preemptible or IRQ-disabled semantics
  are genuinely required.
- Atomic API ordering variants and scoped lock guards evolve. Validate advanced
  usage against the exact target kernel.
- The fundamental driver rule remains stable: choose synchronization from
  context, state invariants, ordering, and lifetime rather than copied API
  recipes.
