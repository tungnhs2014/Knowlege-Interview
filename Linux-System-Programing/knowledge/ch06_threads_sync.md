# Chapter 6 - Thread Synchronization

> Topics: 6.2 Mutexes, condition variables, deadlock
> Main sources: TLPI Ch30; DevLinux Module 05, Exercises 2, 3, 5, 6, and 7
> Source notes: DevLinux gives good producer-consumer and counter practice. TLPI is the authority for mutex ownership, condition-variable predicates, undefined behavior, and POSIX caveats.

## Learning Goal

Understand synchronization as a design for shared-state ownership, not as an API decoration added after races appear.

After this file, you should be able to:

- protect shared mutable state with the correct mutex protocol;
- use condition variables without lost wakeups;
- recognize and debug deadlock, livelock, starvation, contention, and shutdown hangs;
- review Embedded/Linux backend code for concurrency failures.

## Coverage Notes

This file covers learning-map row **6.2 Thread Synchronization** and the Chapter 6 must-cover items for mutexes, condition variables, read-write locks, barriers, semaphores when relevant, memory visibility, invariants, race conditions, deadlock, livelock, starvation, lock ordering, timeout/recovery patterns, debugging, and Embedded constraints.

Moved or split coverage:

- POSIX semaphores are introduced here only as a synchronization comparison point. Full semaphore lifecycle, named semaphores, System V/POSIX IPC semaphore APIs, and cross-process ownership are moved to Chapter 7 IPC semaphore coverage.
- C11 atomics are mentioned only as a design boundary. Deep memory-ordering API coverage is out of scope for this pthread chapter unless a later atomics topic is added.

## Problem It Solves

Synchronization solves the problem created by threads sharing one address space: multiple execution flows can observe and modify the same state at the same time. Correct code needs a protocol for who owns the state now, who may wait, and what state change makes progress possible.

## Mental Model

A mutex protects state. A condition variable announces that state **may have changed**. The predicate decides whether a waiting thread can proceed.

```text
shared state: queue.count, queue.closed, queue.buffer
    |
    +-- mutex: who may inspect/change the state now?
    |
    +-- condvar: who should wake up and recheck the state?
    |
    +-- predicate: is the state useful yet?
```

The condition variable is not the condition. The protected variables are the condition.

## Mechanism

A race appears when correctness depends on timing between threads. `counter++` looks small in C, but it can be load, add, and store. Two threads interleaving those steps can lose updates.

Synchronization also creates **memory visibility**. The practical pthread rule is: if one thread updates protected state while holding a mutex and another thread later locks the same mutex, the second thread must see a coherent view of that state. The mutex is not just a gate for the CPU; it is the protocol that makes the invariant visible in a defined order.

Mutex rule:

```text
lock mutex
    read or modify every field in the protected invariant
unlock mutex
```

Condition variable rule:

```text
lock mutex
while predicate is false:
    pthread_cond_wait(cond, mutex)
use or change protected state
unlock mutex
```

`pthread_cond_wait()` atomically unlocks the mutex and sleeps, then relocks the mutex before returning. This closes the classic window where a signal could arrive after a thread checked the predicate but before it actually went to sleep.

Read-write locks are useful when many readers can safely inspect stable state while writers need exclusive access. They are not a free speedup: writer starvation, fairness policy, and upgrade/downgrade design can become harder than a normal mutex.

Barriers solve a different problem: a fixed group of threads must all reach the same phase before any of them continues. They are useful for staged computation, tests, and batch algorithms, but they are a poor fit for variable-size worker pools or shutdown paths where the number of participants changes.

Semaphores are counters used to represent available units of a resource or events. In pthread-only designs, condition variables are usually clearer when the state predicate lives in ordinary shared memory. Semaphores become more central in Chapter 7 because POSIX and System V semaphores can also be IPC synchronization objects.

## Key APIs And Objects

| API/object | Role | Production rule |
|---|---|---|
| `pthread_mutex_t` | mutual exclusion object | one mutex should protect one clear invariant or object family |
| `PTHREAD_MUTEX_INITIALIZER` | static default init | only for static storage and default attributes |
| `pthread_mutex_init()` | dynamic/custom init | use for heap/stack mutexes or custom attributes |
| `pthread_mutex_destroy()` | release mutex object resources | only after no thread can lock or unlock it |
| `pthread_mutex_lock()` | acquire ownership | may block; keep critical sections short |
| `pthread_mutex_unlock()` | release ownership | only owner should unlock |
| `pthread_cond_t` | wait/notify object | always pair with a mutex and predicate |
| `PTHREAD_COND_INITIALIZER` | static default condition-variable init | only for static storage and default attributes |
| `pthread_cond_init()` | dynamic/custom condition-variable init | initialize before any waiter/signaler can see it |
| `pthread_cond_destroy()` | release condition-variable object resources | only after no thread can wait, signal, or broadcast |
| `pthread_cond_wait()` | sleep until wakeup, then relock | always in `while`, never naked `if` |
| `pthread_cond_signal()` | wake at least one waiter | good when one equivalent worker can proceed |
| `pthread_cond_broadcast()` | wake all waiters | use for shutdown or multiple predicates |
| `pthread_cond_timedwait()` | bounded wait | useful for timeouts and diagnostics |
| `pthread_rwlock_t` | read-write lock object | only after measuring read-heavy contention |
| `pthread_rwlock_init()` | dynamic/custom rwlock init | initialize before readers or writers can see it |
| `pthread_rwlock_destroy()` | release rwlock object resources | only after no thread can hold or wait on it |
| `pthread_rwlock_rdlock()` | acquire shared/read access | many readers may proceed together |
| `pthread_rwlock_wrlock()` | acquire exclusive/write access | writers need exclusive ownership |
| `pthread_rwlock_unlock()` | release read or write lock | pair with whichever lock mode was acquired |
| `pthread_barrier_t` | phase rendezvous object | use only when the participant count is fixed and known |
| `pthread_barrier_init()` | initialize barrier and count | count must match the number of arrivals expected per phase |
| `pthread_barrier_wait()` | wait until all participants arrive | one waiter receives `PTHREAD_BARRIER_SERIAL_THREAD` |
| `pthread_barrier_destroy()` | release barrier resources | only after no thread can still wait on it |
| POSIX semaphore | counting synchronization primitive | moved to Chapter 7 for full `sem_*` API and IPC lifecycle |

Many status-returning pthread calls in this section return `0` or a positive error number. Store the return value and report that number; do not expect `errno` to be set unless that specific API documents it.

## Lifecycle / Data Flow

Mutex-protected update:

```text
Thread A locks M
Thread A changes state
Thread B tries to lock M and blocks
Thread A unlocks M
Thread B wakes, locks M, sees the new state
```

Condition-variable producer/consumer flow:

```text
producer                              consumer
--------                              --------
lock mutex                            lock mutex
while queue full                      while queue empty and not shutdown
    wait not_full                         wait not_empty
put item                              get item
signal not_empty                      signal not_full
unlock mutex                          unlock mutex
```

Shutdown flow:

```text
controller locks mutex
sets shutdown = true
broadcasts condition variable
unlocks mutex

workers wake
recheck predicate
observe shutdown
exit
```

Timed wait and recovery flow:

```text
worker locks mutex
while queue empty and not shutdown:
    rc = pthread_cond_timedwait(not_empty, mutex, deadline)
    if rc == ETIMEDOUT:
        record diagnostic evidence
        decide: retry, degrade, reconnect, or shutdown
worker rechecks predicate before using state
worker unlocks mutex
```

A timeout is not proof that the predicate is false forever. It is a bounded chance to collect evidence and run a recovery policy while still following the same mutex/predicate rule.

Barrier phase flow:

```text
worker computes local phase N result
pthread_barrier_wait(barrier)
    |
    +-- after all workers arrive, every worker may read phase N results
worker starts phase N + 1
```

Object lifetime:

- initialize mutexes/condition variables before any thread can use them;
- destroy them only after no thread can lock, wait, signal, or broadcast them;
- destroy barriers only after no thread can still be waiting for a phase;
- do not copy an initialized mutex, condition variable, rwlock, or barrier object and then operate on the copy;
- join workers before destroying synchronization objects embedded in shared state.

## Production Bugs And Debugging

Concurrency bugs often disappear under logging because prints change scheduling. Trust evidence from state, locks, and backtraces more than timing guesses.

| Symptom | Likely cause | Evidence | Fix |
|---|---|---|---|
| final counter too small | lost updates | ThreadSanitizer/Helgrind reports race | protect all accesses or use atomics for simple counters |
| consumer sleeps forever | no durable predicate or missed shutdown broadcast | GDB shows wait in `pthread_cond_wait()` | store state under mutex, broadcast on shutdown |
| rare wrong item after wakeup | `if` around `pthread_cond_wait()` | multiple waiters or spurious wakeup path | use `while` and recheck predicate |
| deadlock | inconsistent lock order | `thread apply all bt` shows circular waits | define global lock order |
| livelock | retry loops keep changing state but no thread commits progress | high CPU, logs show repeated backoff/retry | add ordering, bounded backoff, or central arbitration |
| starvation | one class of thread rarely obtains the lock/resource | reader flood, priority mismatch, long tail latency | fairness policy, shorter holds, queueing, priority inheritance where appropriate |
| high CPU with no progress | busy wait instead of condvar | `top -H`, perf samples in polling loop | sleep on condition variable |
| low throughput | lock held around slow I/O/callback | stack traces show work under lock | copy state under lock, do slow work outside |
| timeout loop floods logs | timeout used without recovery policy | repeated `ETIMEDOUT` and no state transition | define retry budget, health signal, or shutdown path |
| phase hang | barrier count does not match live participants | GDB shows workers in `pthread_barrier_wait()` | use fixed participants or replace with condvar state |
| crash during destroy | sync object destroyed while in use | waiters still in backtrace | join/stop all users before destroy |

Useful tools:

```bash
gcc -g -O1 -fsanitize=thread -pthread app.c -o app_tsan
valgrind --tool=helgrind ./app
valgrind --tool=drd ./app
strace -f -e futex ./app
perf sched record ./app
perf sched latency
pidstat -t -w -p <pid> 1
gdb -p <pid>
(gdb) thread apply all bt
```

For condition variables, inspect the predicate variables (`count`, `shutdown`, `ready`), not just whether `signal()` was called.

For livelock and starvation, collect both stacks and scheduling evidence. A stack snapshot shows where threads are; `pidstat`, `perf sched`, and logs around retry decisions show whether the system is making progress.

## Work Checklist

During design/code review:

- [ ] Name the invariant each mutex protects.
- [ ] Require every access to protected fields to use the same mutex.
- [ ] Keep lock hold time short.
- [ ] Do not call unknown callbacks while holding locks.
- [ ] Use `while` around every condition-variable wait.
- [ ] Modify predicate state while holding the mutex.
- [ ] Use `broadcast` for shutdown and state transitions that many waiters must recheck.
- [ ] Define lock order for code that may acquire multiple mutexes.
- [ ] Define timeout behavior: retry, degrade, reconnect, fail the request, or shutdown.
- [ ] Check whether retries can livelock under load.
- [ ] Check whether read-heavy locks or priorities can starve writers or low-priority workers.
- [ ] Use barriers only for fixed-size phase synchronization.
- [ ] Add cancellation cleanup if a cancellation point can run while a mutex is held.
- [ ] Prefer bounded queues and worker pools in Embedded services.

## Recognize / Advanced

| Detail | Practical meaning |
|---|---|
| mutexes are advisory | unsafe code can still touch globals without locking |
| `PTHREAD_MUTEX_DEFAULT` misuse | behavior can be undefined; Linux often behaves like normal mutex |
| `PTHREAD_MUTEX_ERRORCHECK` | useful to catch self-lock/unlock misuse in debug builds |
| recursive mutexes | can hide confused ownership; use sparingly |
| `pthread_mutex_trylock()` | useful for backoff, not routine polling |
| futex | Linux kernel primitive used by pthread locks under contention; not normal app API |
| read-write locks | use `pthread_rwlock_init()`/`pthread_rwlock_destroy()` plus read/write lock/unlock; watch writer starvation and fairness behavior |
| barriers | useful for fixed phase boundaries; fragile when workers can exit or be canceled |
| semaphores | useful for resource counts and IPC; full coverage moved to Chapter 7 semaphore topics |
| atomics | fine for small independent state; not a replacement for protecting invariants |

DevLinux caveat: if an example uses `volatile` on a mutex-protected counter, do not learn that `volatile` makes code thread-safe. The mutex provides synchronization; `volatile` is for different visibility problems and can mislead in pthread code.

DevLinux caveat: examples that use `rand()` in multiple threads deserve a portability note. POSIX historically does not require `rand()` to be a good thread-safe design choice; prefer per-thread PRNG state or documented reentrant alternatives for production.

## Interview Readiness

A strong answer starts from the state predicate and invariant, then names the APIs.

Be ready to explain:

- why `counter++` is not safe just because it is one C expression;
- what a mutex protects;
- why a condition variable must be paired with a predicate;
- exactly what `pthread_cond_wait()` does with the mutex;
- why `while` is required around waits;
- how you would debug producer-consumer hangs and deadlocks;
- how livelock differs from deadlock and starvation;
- when a barrier or semaphore is the wrong primitive;
- how timeouts become recovery policy instead of a sleep-with-error-code;
- how priority inversion or long lock hold time can hurt realtime/Embedded systems.

Interview trap: "Signal wakes the consumer" is incomplete. The correct statement is: signal wakes a waiter so it can recheck protected state.

## Final Coverage Check

Covered: mapped row 6.2; mutexes; condition variables; read-write locks; barriers; semaphore placement; memory visibility; invariants; race conditions; deadlock; livelock; starvation; lock ordering; timeout/recovery patterns; production debugging; Embedded worker-pool and priority concerns; interview framing.

Moved: full POSIX/System V semaphore lifecycle and cross-process semaphore ownership are moved to Chapter 7 IPC semaphore coverage.
