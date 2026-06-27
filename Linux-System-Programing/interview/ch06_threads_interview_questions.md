# Chapter 6 Interview - Threads

> Scope: 6.1-6.5 thread lifecycle, synchronization, thread safety/TSD/TLS, cancellation, stacks/signals/fork/exec/exit, and Linux NPTL debugging.
> Mental model: a process is the resource container; a thread is an execution flow inside it. Threads make sharing memory cheap, but every shared mutable object needs an ownership and synchronization design.

---

## Review Basis

This chapter-level interview file is grounded in the Chapter 6 learning map and the five mechanism-family knowledge files:

| Topic | Knowledge file | Interview focus |
|---|---|---|
| 6.1 lifecycle and threads vs processes | [ch06_threads_core.md](../knowledge/ch06_threads_core.md) | creation order, argument lifetime, join/detach, process-wide exit |
| 6.2 synchronization | [ch06_threads_sync.md](../knowledge/ch06_threads_sync.md) | mutex protocols, condition-variable predicates, deadlock, contention |
| 6.3 thread safety and TLS | [ch06_threads_tls.md](../knowledge/ch06_threads_tls.md) | reentrancy, static buffers, `pthread_once()`, TSD, TLS, per-thread `errno` |
| 6.4 cancellation | [ch06_threads_cancel.md](../knowledge/ch06_threads_cancel.md) | deferred cancellation, cleanup handlers, shutdown design |
| 6.5 further details | [ch06_threads_details.md](../knowledge/ch06_threads_details.md) | stacks, signals, fork/exec/exit, NPTL, Linux debugging evidence |

Mapped training sources: TLPI Chapters 29-33 and DevLinux Module 05 with Exercises 1-7. TLPI is the semantic authority for POSIX pthread behavior. DevLinux is most useful for create/join, mutex, condition-variable, producer-consumer, read-write-lock, and parallel-array practice.

Source and portability caveats:

- **POSIX-vs-Linux:** pthread APIs are POSIX, but `/proc/<pid>/task`, kernel TIDs, `gettid()`, `strace -f -e futex`, `perf sched`, `pidstat -t`, NPTL details, and `pthread_sigqueue()` are Linux/glibc-specific or Linux-focused debugging details.
- **Version-sensitive:** modern Linux/glibc normally uses NPTL with one pthread mapped to one kernel-scheduled task. TLPI mentions older LinuxThreads behavior because it matters historically. Very old vendor Embedded systems should be verified on target.
- **Source gaps:** DevLinux is thin on TSD/TLS destructors, cancellation cleanup depth, signal/thread interactions, fork in multithreaded programs, and NPTL. Use TLPI/POSIX/man-pages for production decisions in those areas.
- **DevLinux caveats:** `volatile` on a mutex-protected counter can mislead; the mutex provides synchronization. Many status-returning pthread functions return `0` or a positive error number, not `-1` with `errno`, but APIs such as `pthread_exit()`, `pthread_self()`, `pthread_equal()`, and `pthread_getspecific()` have different return contracts. Examples using `rand()` in multithreaded code deserve a portability/thread-safety note.

## Coverage Trace

This is the interview coverage matrix for Chapter 6. Each learning-map row and must-cover item appears in Priority A, B, or C; production-critical items are tested through scenarios or comparisons.

| Coverage item | Source | Required interview coverage | Priority coverage | Status |
|---|---|---|---|---|
| 6.1 Threads Introduction | Learning map, TLPI Ch29, DevLinux 05, [ch06_threads_core.md](../knowledge/ch06_threads_core.md) | Thread/process tradeoff, create/join/detach lifecycle, argument/result lifetime, process exit vs thread exit | A1, A2, B1, B2, B3 | Covered |
| 6.2 Thread Synchronization | Learning map, TLPI Ch30, DevLinux 05, [ch06_threads_sync.md](../knowledge/ch06_threads_sync.md) | Mutexes, condition variables, read-write locks, barriers, semaphores as comparison, invariants, memory visibility | A3, A4, A5, A10, B4, B5, B8, B13, B14, B16 | Covered |
| 6.3 Thread Safety & TLS | Learning map, TLPI Ch31, [ch06_threads_tls.md](../knowledge/ch06_threads_tls.md) | Thread-safe vs reentrant, static state, `pthread_once()`, TSD, TLS, per-thread `errno` | A6, B6, B7 | Covered |
| 6.4 Thread Cancellation | Learning map, TLPI Ch32, [ch06_threads_cancel.md](../knowledge/ch06_threads_cancel.md) | `pthread_cancel()`, deferred cancellation, cancellation points, cleanup handlers, join-after-cancel, safe shutdown | A7, B9, B17, C full cancellation point list | Covered |
| 6.5 Threads Further Details | Learning map, TLPI Ch33, [ch06_threads_details.md](../knowledge/ch06_threads_details.md) | Thread stacks, signal masks/dispositions, signal targeting, `fork()`/`exec()`/`exit()`, NPTL, Linux debugging evidence | A8, A9, A11, B10, B11, B12, B18 | Covered |
| Thread lifecycle must cover | Chapter 6 Must Cover | `pthread_create()`, start routine, join, detach, process exit vs thread exit | A2, B1, B3 | Covered |
| Process vs thread must cover | Chapter 6 Must Cover | Shared address space/FDs, per-thread stack/TID/signal mask/`errno`/scheduling details | A1, A8, A9, A11, B2, B12 | Covered |
| Concurrency vs parallelism must cover | Chapter 6 Must Cover | CPU scheduling, blocking calls, context-switch implications, pool sizing | A1, A9 | Covered |
| Synchronization must cover | Chapter 6 Must Cover | Mutexes, condition variables, rwlocks, barriers, semaphores when relevant, visibility, invariants | A3, A4, A5, A10, B4, B5, B8, B13, B14, B16 | Covered |
| Failure/debug must cover | Chapter 6 Must Cover | Races, deadlock, livelock, starvation, lock ordering, timeout/recovery, debugging | A3, A4, A5, A10, B15, B16, Work-Ready Debug Checklist | Covered |
| Thread safety/TLS must cover | Chapter 6 Must Cover | Reentrancy, `pthread_key`, `__thread`, `_Thread_local`/`thread_local`, `pthread_once()` | A6, B6, B7 | Covered |
| Cancellation must cover | Chapter 6 Must Cover | Cleanup handlers, cancellation points, resource ownership, safe shutdown, cancellation state across process-control calls | A7, B9, B17, C full cancellation point list | Covered |
| Signals/stacks/NPTL/Embedded must cover | Chapter 6 Must Cover | Signals with threads, signal targeting, stack sizing, Linux NPTL, Embedded RAM/task constraints | A8, A9, A11, B10, B11, B12, B18 | Covered |

---

## Priority Map

### Priority A - Production, Debug, And Design Scenarios

These are the questions to master. Answer them as incident/design reviews with evidence, not as API definitions.

| # | Scenario | Must cover |
|---|---|---|
| A1 | Choose threads, processes, or a bounded worker pool | shared state, isolation, stack/task overhead, crash/security boundary, debugging |
| A2 | Review worker lifecycle bugs | stable args, no create-order assumption, join/detach ownership, `main()`/`exit()` behavior |
| A3 | Fix corruption hidden by logging | shared mutable state, mutex/atomic discipline, ownership, race tooling |
| A4 | Debug producer-consumer hangs | predicate loop, `pthread_cond_wait()`, lost wakeups, shutdown broadcast |
| A5 | Diagnose deadlock or contention | lock order, critical-section size, priority inversion, GDB/perf/pidstat evidence |
| A6 | Make legacy C helpers thread-safe | reentrant API, static buffers, `pthread_once()`, TSD/TLS, per-thread `errno` |
| A7 | Design cancellation-safe shutdown | deferred cancellation, cancellation points, cleanup handlers, join after cancel |
| A8 | Handle SIGTERM in a threaded daemon | process-wide disposition, per-thread masks, `sigwait()` pattern, async-signal safety |
| A9 | Investigate high thread count and overhead | `/proc`, `top -H`, GDB, `strace -f`, futex waits, stack memory, pool sizing |
| A10 | Review timeout, livelock, and starvation failures | timed waits, absolute deadline, retry policy, progress evidence, fairness |
| A11 | Debug worker crashes from stack pressure | default stack limits, large locals, recursion, guard-page SIGSEGV, Embedded RAM budget |

### Priority B - Comparisons And Follow-Ups

These test whether you can make trade-offs after the main scenario is solved.

- Joinable vs detached.
- `pthread_t` vs Linux TID.
- `exit()` vs `pthread_exit()` vs returning from a start routine.
- Mutex vs condition variable.
- `pthread_cond_signal()` vs `pthread_cond_broadcast()`.
- Thread-safe vs reentrant.
- TSD vs TLS.
- Mutex vs read-write lock.
- Deferred vs asynchronous cancellation.
- `fork()` after threads exist.
- Default stack vs tuned stack.
- POSIX pthread model vs Linux NPTL evidence.
- Barrier vs condition variable.
- Semaphore vs condition variable.
- Deadlock vs livelock vs starvation.
- Timed wait vs sleep-loop retry.
- Cancellation state/type across `fork()` and `exec()`.
- Signal targeting: process-directed vs thread-directed.

### Priority C - Recognize And Read The Manual

Know what these are and when they matter, but do not spend first-pass interview time memorizing every flag or historical detail.

- Raw futex programming.
- Full cancellation point list.
- Exact `clone()` flags behind NPTL.
- LinuxThreads historical deviations.
- Process-shared mutexes and condition variables.
- Mutex types such as `ERRORCHECK`, `RECURSIVE`, and priority-inheritance attributes.
- Realtime thread scheduling policy details.
- `pthread_sigqueue()` and signal-with-data to a specific Linux thread.

---

## Scenario Questions - Priority A

### A1. A Linux service can handle clients with one process per client, one thread per client, or a bounded thread pool. How would you choose?

**What the interviewer is testing**

They want to see that you understand a thread as an execution flow inside a process resource container, and that you can trade sharing cost against isolation, resource limits, and debugging complexity.

**Strong answer**

Use threads when shared in-memory state is intentional: a shared cache, queue, metrics table, connection registry, or low-latency coordination path. Use processes when crash containment, privilege separation, independent restarts, or running a different program matters more. For a backend or Embedded daemon, prefer a bounded worker pool over unbounded one-thread-per-client creation unless the workload and limits are tightly controlled.

**Mechanism**

Threads in one process share address space, heap, globals, mappings, file descriptors, cwd, umask, credentials, signal dispositions, and resource limits. Each thread has its own stack, execution context, `pthread_t`, signal mask, `errno`, TSD/TLS, alternate signal stack, and Linux task identity. On modern Linux NPTL, pthreads are kernel-scheduled tasks, so multiple runnable threads can execute in parallel on different CPUs.

**Pitfalls**

One bad pointer, data race, or process-terminating call can take down every thread. Unbounded thread-per-client designs fail through stack memory pressure, context switching, task limits, lock contention, and slow shutdown. Process-per-client designs provide stronger isolation but pay creation and IPC costs.

**Debug angle**

Start with evidence: `grep '^Threads:' /proc/<pid>/status`, `ls /proc/<pid>/task`, `ps -eLf`, `top -H -p <pid>`, and `gdb -p <pid>` with `info threads` and `thread apply all bt`. If high thread count is the issue, also inspect stack size, cgroup task limits, and context switches with `pidstat -t -w -p <pid> 1` or `perf sched`.

**Follow-up keywords**

`pthread_create()`, worker pool, process isolation, shared address space, file descriptor table, per-thread stack, NPTL, kernel TID, IPC, backpressure.

### A2. A worker service sometimes exits before logs flush, leaks threads, and gives workers the wrong job ID. How would you review and fix the lifecycle?

**What the interviewer is testing**

They are testing practical pthread lifecycle ownership: argument lifetime, scheduling order, join/detach policy, return-value ownership, and process-wide termination behavior.

**Strong answer**

For every `pthread_create()`, define who owns the argument object, who observes completion, and who releases thread resources. Pass stable per-thread argument storage, not `&i` from a changing loop variable. Join threads when shutdown code needs completion or return status. Detach only true fire-and-forget work whose input/output lifetime is independent. Make sure `main()` does not return while workers are expected to continue.

**Mechanism**

`pthread_create()` starts `void *(*start)(void *)` with one `void *` argument, and there is no ordering guarantee after the call. The new thread may run before or after the creator's next statement. Threads are joinable by default. A terminated joinable thread keeps resources until exactly one successful `pthread_join()`. A detached thread releases resources automatically and cannot be joined. Returning from `main()` or calling `exit()` terminates the whole process; returning from a start routine or calling `pthread_exit()` terminates only that thread.

**Pitfalls**

Passing `&i` from a loop makes workers race on the same object. Returning a pointer to a worker's stack is invalid after thread exit. Joining twice, detaching and then joining, or never joining a joinable thread is a lifecycle bug. A worker that calls `exit()` may bypass the intended shutdown path for the whole process.

**Debug angle**

Look at creation sites and argument storage in code review, then confirm with runtime evidence: `/proc/<pid>/status` `Threads`, `/proc/<pid>/task`, `ps -T -p <pid>`, and GDB backtraces. If `pthread_create()` fails, remember it returns a positive error number; check stack size, task limits, memory pressure, and unjoined joinable threads.

**Follow-up keywords**

`pthread_create()`, `pthread_join()`, `pthread_detach()`, `pthread_exit()`, `pthread_self()`, `pthread_equal()`, positive pthread error returns, stable storage, `pthread_attr_setstacksize()`.

### A3. A shared counter, cache, or queue is rarely corrupted under load, but adding logs makes the bug disappear. How do you explain and fix it?

**What the interviewer is testing**

They want race-condition reasoning, not folklore. A strong answer explains why timing changes do not prove correctness and how to design shared-state ownership.

**Strong answer**

Assume an unsynchronized shared mutable object until proven otherwise. Logging changes timing and may accidentally serialize execution, so the bug disappearing is evidence of a race, not a fix. Identify each shared object, name the invariant, and require every read/write path to follow one synchronization rule. Use a mutex for compound state and object lifetimes; use atomics only for simple independent values where the memory-ordering story is clear.

**Mechanism**

`counter++` is a read-modify-write sequence, not automatically atomic at the C level. Queues, caches, lists, and reference counts usually have multi-field invariants. A mutex protects an invariant only if all code paths use the same mutex for every access to that state. Condition variables do not protect data; they only help waiters sleep until protected state may have changed.

**Pitfalls**

`volatile` does not make pthread code synchronized. Using two different locks for one invariant is equivalent to having no clear lock. Holding locks around slow logging/I/O can hide corruption while creating deadlocks and latency. A "mostly read-only" object still needs a safe publication and lifetime protocol.

**Debug angle**

Build with ThreadSanitizer when possible: `gcc -g -O1 -fsanitize=thread -pthread ...`. Use Helgrind/DRD for pthread race checks when TSan is not practical. In GDB, watchpoints can help with narrow corruptions, but code review of ownership and invariants is usually more reliable than trying to reproduce a timing bug.

**Follow-up keywords**

data race, critical section, mutex discipline, invariant, C atomics, per-thread aggregation, sharded locks, ThreadSanitizer, Helgrind, DRD.

### A4. A producer-consumer queue occasionally hangs or consumes from an empty buffer. How would you review the condition-variable design?

**What the interviewer is testing**

They want the condition-variable mental model: the predicate is the real state; the condition variable is only a notification channel.

**Strong answer**

I would find the protected predicates, such as `count > 0`, `count < capacity`, and `shutdown`. Every wait must be inside `while (!predicate)` while holding the mutex. Producers and shutdown code must update predicate state under the same mutex, then signal or broadcast. Shutdown should be part of the predicate and usually requires `pthread_cond_broadcast()` so every sleeping worker can recheck state and exit.

**Mechanism**

`pthread_cond_wait(&cv, &mutex)` atomically unlocks the mutex and sleeps, then relocks it before returning. Wakeup does not prove the predicate is true: POSIX allows spurious wakeups, a broadcast can wake many waiters, and another thread may consume the item before this waiter reacquires the mutex. Condition variables do not remember old signals as durable event counts.

**Pitfalls**

Using `if` around `pthread_cond_wait()` can consume from an empty queue. Signaling without changing protected state loses the design. Waiting on one condition variable with different mutexes is a serious bug. Forgetting shutdown broadcast leaves sleepers stuck forever.

**Debug angle**

Attach GDB and run `thread apply all bt`; identify threads in `pthread_cond_wait()` and inspect predicate variables, not just signal calls. Use `strace -f -e futex -p <pid>` to confirm futex waits. Add invariant checks for queue count, capacity, closed/shutdown flags, and use `pthread_cond_timedwait()` temporarily when bounded diagnostic waits help.

**Follow-up keywords**

predicate loop, `pthread_cond_wait()`, `pthread_cond_signal()`, `pthread_cond_broadcast()`, spurious wakeup, lost wakeup, bounded queue, shutdown flag.

### A5. A service freezes under load and many threads are around locks. How do you debug deadlock or lock contention?

**What the interviewer is testing**

They are looking for an incident workflow: distinguish deadlock, contention, priority inversion, CPU saturation, and I/O waits using evidence.

**Strong answer**

First determine whether the system is deadlocked, slow from contention, livelocked, starved, CPU saturated, or blocked on I/O. For deadlock, collect all thread stacks and reconstruct which thread holds which lock and which lock it waits for. Fix with a global lock order, smaller critical sections, and no callbacks or slow I/O while holding locks. For contention or starvation, measure hot locks and reduce shared bottlenecks with per-thread aggregation, sharding, fairer queueing, priority-inheritance mutexes where the target needs them, or a smaller worker pool.

**Mechanism**

A classic deadlock is circular wait: Thread A holds M1 and waits for M2 while Thread B holds M2 and waits for M1. Livelock means threads keep running and changing state but no useful operation commits. Starvation means one class of work rarely gets the resource because scheduling, priority, lock fairness, or queue policy keeps favoring others. Mutexes are cooperative protocols; the kernel blocks waiters but does not understand your object invariants. On realtime or priority-sensitive systems, priority inversion can occur when a high-priority thread waits on a mutex held by a lower-priority thread that cannot run soon enough.

**Pitfalls**

Recursive mutexes can hide broken ownership. `trylock` loops can become busy waiting or livelock. Timeouts can be useful evidence, but treating a timeout as correctness can create retry storms. More locks can make a design less safe without a lock-order rule. Holding locks across logging, allocation, file/network I/O, unknown callbacks, or signal-handler paths creates unpredictable latency and reentrancy risk.

**Debug angle**

Use `gdb -p <pid>`, `info threads`, and `thread apply all bt` as the first snapshot. Use `top -H`, `pidstat -t -w -p <pid> 1`, `strace -f -e futex -p <pid>`, and `perf sched` to separate sleeping lock waits from runnable CPU pressure. If using `pthread_cond_timedwait()` or lock-acquisition timeouts for diagnostics, log the predicate, owner, queue length, and recovery action. In tests, use ThreadSanitizer or Helgrind/DRD for race and lock-order clues.

**Follow-up keywords**

deadlock, circular wait, livelock, starvation, lock ordering, lock convoy, futex wait, timeout recovery, priority inversion, `PTHREAD_PRIO_INHERIT`, critical-section length, `perf sched`.

### A6. A legacy C helper returns a pointer to a static buffer and now runs in many worker threads. How would you make it safe?

**What the interviewer is testing**

They want thread-safety versus reentrancy judgment, especially around old C APIs with hidden static/global state.

**Strong answer**

For new code, change the API to be reentrant: the caller passes a buffer and size, or owns an explicit context object. If the ABI cannot change, use TSD so each thread gets its own persistent buffer, initialized with `pthread_once()` and freed by a TSD destructor. TLS can be simpler for fixed per-thread variables on known compiler/runtime targets, but it is less POSIX-portable than TSD.

**Mechanism**

A static local buffer is shared by all threads. A mutex can protect the write inside the function, but once the function returns and unlocks, another thread can overwrite the same buffer before the first caller uses it. TSD uses a process-wide key with a separate value per thread. TLS such as `__thread` or `_Thread_local` gives direct per-thread storage. In pthread programs, `errno` is per-thread through the C library, so one thread's failed call does not overwrite another thread's `errno`.

**Pitfalls**

"Thread-safe" does not mean async-signal-safe. Returning stack storage is always wrong after the function returns. One big internal lock may be correct but can turn the library into a global bottleneck. TSD destructors have ordering caveats; do not make one key's destructor depend on another. `__thread` is common on Linux/glibc but not POSIX.

**Debug angle**

Search for hidden mutable state: `rg '\\bstatic\\b'`, globals, lazy-init flags, and legacy APIs like `strtok`, `asctime`, `ctime`, `gmtime`, `localtime`, `rand`, or `gethostbyname` depending on platform and replacement availability. Race tools can catch concurrent writes, but API contract review is the main tool because static-buffer bugs often produce valid-looking wrong strings.

**Follow-up keywords**

thread-safe, reentrant, caller-owned buffer, static storage, `pthread_once()`, `pthread_key_create()`, `pthread_getspecific()`, `pthread_setspecific()`, TSD destructor, `__thread`, `_Thread_local`, per-thread `errno`.

### A7. A shutdown path uses `pthread_cancel()`, and after a release the service sometimes deadlocks. What went wrong and how should cancellation be designed?

**What the interviewer is testing**

They want cancellation treated as cooperative termination with cleanup, not as a safe kill-thread primitive.

**Strong answer**

Assume a thread was canceled while owning a mutex, heap object, file descriptor, or partially updated invariant. Prefer cooperative shutdown: set a shutdown flag under a mutex, broadcast condition variables, and join workers. If cancellation is needed for blocking calls, use deferred cancellation, install cleanup handlers for every resource held across cancellation points, and join the target before freeing shared data.

**Mechanism**

`pthread_cancel()` queues a request and returns; it does not wait for the target to stop. Default cancelability is enabled and deferred, so the target acts on the request at cancellation points such as `pthread_cond_wait()`, `pthread_join()`, blocking I/O, sleep/wait calls, or `pthread_testcancel()`. Cleanup handlers run in reverse push order during cancellation. If cancellation occurs while blocked in `pthread_cond_wait()`, the mutex is reacquired before cleanup handlers run, allowing an unlock cleanup handler to work.

**Pitfalls**

Asynchronous cancellation can stop a thread while inside `malloc()`, a library lock, a critical section, or a partial state update. Forgetting cleanup handlers leaves locks held and memory/FDs leaked. Treating `pthread_cancel()` as completion causes use-after-free. Detached canceled threads hide whether cleanup occurred.

**Debug angle**

Inspect all stacks for threads blocked in `pthread_mutex_lock()`, `pthread_cond_wait()`, or I/O. Search for cancellation points inside locked regions. Check that `pthread_cleanup_push()` and `pthread_cleanup_pop()` are paired in the same lexical scope because they may be macros. Join canceled joinable threads and check for `PTHREAD_CANCELED`.

**Follow-up keywords**

`pthread_cancel()`, deferred cancellation, asynchronous cancellation, cancellation point, `pthread_testcancel()`, `pthread_cleanup_push()`, `pthread_cleanup_pop()`, `PTHREAD_CANCELED`, cooperative shutdown.

### A8. A threaded daemon receives SIGTERM, but shutdown runs in a random worker and sometimes deadlocks. How should signals be handled?

**What the interviewer is testing**

They want the interaction between UNIX signals and pthreads: process-wide dispositions, per-thread masks, eligible-thread delivery, and async-signal safety.

**Strong answer**

Do not perform complex shutdown from an asynchronous signal handler. At startup, block SIGTERM/SIGINT in the main thread before creating workers so workers inherit the blocked mask. Create one dedicated signal thread that waits using `sigwait()`, `sigwaitinfo()`, or Linux `signalfd()`. That thread can update normal shared state under a mutex, broadcast condition variables, and let workers exit cleanly.

**Mechanism**

Signal dispositions are process-wide: one thread installing a handler affects the whole process. Signal masks are per-thread. A process-directed signal such as `kill(pid, SIGTERM)` can be delivered to any eligible thread that does not block it. Pthread mutex/condition APIs are not async-signal-safe, so calling them from a signal handler can deadlock or corrupt state.

Some signals are naturally thread-directed: hardware exceptions such as `SIGSEGV`, `SIGBUS`, `SIGFPE`, and `SIGILL` are generated in the faulting thread, and `SIGPIPE` is generated for the thread that wrote to the broken pipe. Signals sent with `kill()` to a process, terminal-generated signals, and many timer or software-event signals are process-directed. `pthread_kill()` targets one thread in the same process.

**Pitfalls**

Inconsistent worker masks make delivery unpredictable. A handler that takes locks or calls non-async-signal-safe library code can deadlock if the interrupted thread already held an internal or application lock. Waking only one worker during shutdown can leave other workers sleeping forever.

**Debug angle**

Check signal masks with `/proc/<pid>/task/<tid>/status` fields such as `SigBlk`, `SigPnd`, and `ShdPnd`. Use `strace -f -e signal -p <pid>` to observe delivery. In GDB, identify which thread handled the signal and whether the handler path touched locks, malloc, stdio, or pthread condition variables.

**Follow-up keywords**

`pthread_sigmask()`, `sigwait()`, `sigwaitinfo()`, `signalfd()`, `pthread_kill()`, process-directed signal, thread-directed signal, signal disposition, signal mask, `SIGPIPE`, hardware exception signal, `signal-safety(7)`.

### A9. A production process has hundreds of threads, high context-switch overhead, and unclear CPU usage. How would you diagnose it?

**What the interviewer is testing**

They want Linux work-readiness: using `/proc`, scheduler evidence, stacks, syscall tracing, and design constraints to decide whether the system is CPU-bound, blocked, lock-contended, or over-threaded.

**Strong answer**

First count threads and classify their states. If many threads are runnable and CPUs are saturated, investigate CPU-bound work or over-threading. If many threads sleep in I/O syscalls, look at external dependencies and backpressure. If many sleep in futex/pthread paths, suspect lock contention or deadlock. If context switches are high with low useful work, reduce concurrency, bound queues, shard shared state, or resize the pool.

**Mechanism**

With Linux NPTL, each pthread is a kernel-scheduled task visible under `/proc/<pid>/task`. Each thread has its own stack and scheduling state while sharing process resources. More threads do not guarantee more throughput: CPU-bound pools are often near core count; I/O-bound pools still need resource limits; lock-heavy pools can serialize behind one shared mutex.

**Pitfalls**

High thread count can look like scalability but hide stack memory waste and shutdown complexity. One global lock can make hundreds of workers behave like one worker with more context switches. Large default stacks can matter on Embedded targets or containers. Per-request unbounded threads can become a denial-of-service path.

**Debug angle**

Use `top -H -p <pid>` for per-thread CPU, `ps -eLf` or `ps -T -p <pid>` for thread list/state, `/proc/<pid>/task/<tid>/status` for details, `gdb -p <pid>` followed by `(gdb) thread apply all bt` for blocked stacks, `strace -f -p <pid>` for syscalls, `strace -f -e futex` for lock waits, `perf stat` for context switches, `perf record/report` for hot code, `perf sched` for scheduler behavior, and `pidstat -t -w -p <pid> 1` for per-thread context switching. For stack/RAM evidence, inspect `/proc/<pid>/maps`, `/proc/<pid>/smaps`, or `pmap -x <pid>` and compare configured stack size with worker count.

**Follow-up keywords**

`top -H`, `ps -eLf`, `/proc/<pid>/task`, Linux TID, `gdb info threads`, `thread apply all bt`, `strace -f`, futex, `perf stat`, `perf sched`, `pidstat -t -w`, stack sizing, `/proc/<pid>/maps`, `/proc/<pid>/smaps`, `pmap`.

### A10. A retry-based worker pool never fully hangs, but throughput drops to zero under load. How do you review timeout, livelock, and starvation behavior?

**What the interviewer is testing**

They want to know whether you can distinguish "threads are active" from "the system is making progress," and whether you can design timeout recovery without breaking condition-variable rules.

**Strong answer**

I would identify the protected state and the progress condition first. A timeout should trigger a recovery decision, not a blind retry loop. For condition variables, use `pthread_cond_timedwait()` with a predicate loop and an absolute deadline, then recheck state under the mutex. For livelock, look for retry paths where threads repeatedly release/reacquire resources without committing work. For starvation, look for a class of workers that rarely obtains CPU, a lock, or queue access.

**Mechanism**

Deadlock means blocked threads wait forever. Livelock means threads keep running but keep undoing or deferring each other's progress. Starvation means some work can make progress while other work is unfairly delayed. `pthread_cond_timedwait()` behaves like `pthread_cond_wait()` plus a timeout, but the timeout parameter is an absolute `timespec`, not a relative sleep duration. After return, the predicate must still be checked because wakeups and timeouts do not prove useful state exists.

**Pitfalls**

Replacing waits with `trylock()` loops can turn a blocking bug into CPU burn. Treating `ETIMEDOUT` as "the queue is empty forever" is wrong. Recomputing deadlines incorrectly can create much longer waits than expected. Read-write locks can starve writers on some policies. Backoff without a retry budget or health signal can hide a failure until the watchdog resets the service.

**Debug angle**

Collect stacks and scheduling evidence together: GDB all-thread backtraces, `top -H`, `pidstat -t -w -p <pid> 1`, `perf sched`, and logs around retry decisions. For condition-variable timeouts, log the predicate state, deadline, return code, and recovery action. For starvation, compare queue age, lock wait time, and thread priority/policy.

**Follow-up keywords**

`pthread_cond_timedwait()`, absolute `timespec`, `ETIMEDOUT`, predicate loop, livelock, starvation, retry budget, backoff, fairness, read-write lock starvation, progress invariant.

### A11. An Embedded worker thread crashes only under deep recursion or large requests. How do you debug and fix stack pressure?

**What the interviewer is testing**

They want stack lifetime and resource-budget judgment: each thread has a separate stack, defaults are not universal, and high thread counts turn stack size into a system capacity issue.

**Strong answer**

Treat it as a possible thread-stack overflow until proven otherwise. Inspect the crashing thread's backtrace, large automatic objects, recursion depth, and configured stack size. Move large buffers to heap or a bounded pool, reduce recursion, and set an intentional stack size with pthread attributes only after measuring the workload and target limits. In Embedded systems, tune stack size together with worker-pool size, task limits, guard pages, watchdog deadlines, and worst-case call paths.

**Mechanism**

Non-main pthreads are created with a fixed stack chosen by attributes, libc defaults, architecture, and resource limits. Each thread owns its own call frames and automatic variables. Overflow can hit a guard page and produce SIGSEGV, or on constrained/older targets corrupt adjacent memory if protection is weak. Reducing stack size increases capacity but reduces safety margin; increasing it can reduce the number of workers the system can afford.

**Pitfalls**

Do not assume the desktop default stack matches the target. Do not hide stack overflow by making every stack huge. Large local arrays in worker functions are easy to miss in code review. A pointer to another thread's stack may be technically addressable inside one process, but it is usually a lifetime and ownership bug.

**Debug angle**

Use GDB to inspect the crashing thread and call depth, then review locals and recursion. Check `/proc/<pid>/maps` for stack mappings where useful, `ulimit -s`/resource limits for the environment, and creation code for `pthread_attr_setstacksize()`. On target, add stack high-water instrumentation if the Linux/Embedded platform or project runtime supports it, and reproduce with production-like request size and worker count.

**Follow-up keywords**

`pthread_attr_init()`, `pthread_attr_setstacksize()`, `pthread_attr_destroy()`, `sysconf(_SC_THREAD_STACK_MIN)`, guard page, large automatic array, recursion, Embedded RAM budget, worker-pool sizing.

---

## Comparison Questions - Priority B

### B1. When should a thread be joinable, and when should it be detached?

Use joinable when another thread needs completion, return status, error propagation, or orderly shutdown. Use detached only when no one will join it, no return value is needed, and all input/output lifetimes are independent. Every created thread needs an explicit join-or-detach ownership plan.

### B2. Why is `pthread_t` not the same as a Linux kernel TID?

`pthread_t` is the POSIX thread identity and is opaque; portable code uses `pthread_self()` and `pthread_equal()`. A Linux TID is a kernel task ID visible in `/proc/<pid>/task` and tools like `top -H`; it is excellent debugging evidence but not portable pthread identity.

### B3. What happens if `main()` returns while workers are running?

Returning from `main()` is process termination, so all threads are terminated. If only the main thread should end while others continue, it can call `pthread_exit(NULL)`, but production services usually join workers during a deliberate shutdown.

### B4. What is the difference between a mutex and a condition variable?

A mutex protects shared state and invariants. A condition variable lets a thread sleep until protected state may have changed. The condition variable is not the condition; the predicate stored in shared state is the condition.

### B5. When should `pthread_cond_signal()` become `pthread_cond_broadcast()`?

Use signal when one equivalent waiter can make progress for one state change. Use broadcast when all waiters must recheck state, predicates differ, resources became widely available, or shutdown is being announced.

### B6. What is the difference between thread-safe and reentrant?

Thread-safe means concurrent calls are valid, often through internal locking. Reentrant means the function avoids hidden shared mutable state and usually uses caller-owned state. Reentrant APIs are usually cleaner for new C interfaces and easier to reason about under concurrency.

### B7. When would you use TSD instead of TLS?

Use TSD for POSIX runtime keys, dynamic per-thread heap values, and destructors at thread exit. Use TLS for simple direct per-thread variables when compiler/libc/runtime portability and destructor limitations are acceptable.

### B8. When is a read-write lock better than a mutex?

A read-write lock can help when reads greatly outnumber writes and read critical sections are long enough to benefit from concurrent readers. Use `pthread_rwlock_init()`/`pthread_rwlock_destroy()` for lifecycle, `pthread_rwlock_rdlock()` for shared reader access, `pthread_rwlock_wrlock()` for exclusive writer access, and `pthread_rwlock_unlock()` for both modes. It can hurt when writes are frequent, critical sections are tiny, fairness matters, writer starvation is possible on the target, or the extra complexity hides design bugs.

### B9. Why is asynchronous cancellation rarely acceptable?

It may stop a thread almost anywhere: while holding a mutex, inside a library internal lock, or halfway through updating shared state. Deferred cancellation at known cancellation points plus cleanup handlers is much easier to make correct.

### B10. What should a child do after `fork()` in a multithreaded parent?

Usually call `exec()` immediately. After `fork()`, only the calling thread exists in the child, but mutexes and global state are copied as they were. A mutex held by a vanished thread may remain locked forever in the child.

### B11. When would you tune thread stack size?

Tune stack size when workers use deep recursion, large automatic arrays, high thread counts, or constrained Embedded/container memory. Prefer moving large buffers to heap unless stack allocation is deliberate and measured. On Linux/NPTL, `RLIMIT_STACK` can influence default thread stack size only when set before program execution; changing it with `setrlimit()` inside `main()` is too late for that default decision.

### B12. What does NPTL change for practical debugging?

NPTL is the modern Linux one-to-one pthread implementation: each pthread maps to a kernel-scheduled task. That makes threads visible in `/proc/<pid>/task`, `ps -L`, `top -H`, GDB, scheduler tools, and syscall traces. Treat this as Linux implementation evidence, not portable POSIX API behavior.

### B13. When is a barrier better than a condition variable?

Use a barrier when a fixed set of threads must all reach the same phase before any proceeds, such as phased batch computation. Use a condition variable when progress depends on changing shared state, variable participants, shutdown, or producer-consumer predicates. Barriers are fragile if workers can exit, be canceled, or join dynamically.

### B14. When would you choose a semaphore instead of a condition variable?

Use a semaphore when a count of available resources or events is the natural state and the protocol does not need a richer predicate under a mutex. Use a condition variable when waiters must inspect structured shared state such as queue count, shutdown flag, generation, or ownership. Full POSIX/System V semaphore lifecycle is Chapter 7 IPC material; in Chapter 6, the interview point is choosing the right synchronization shape.

### B15. How do deadlock, livelock, and starvation differ?

Deadlock means threads are blocked in a circular wait and no one can proceed. Livelock means threads continue running but repeatedly avoid or undo progress, often through retry/backoff logic. Starvation means some threads or work classes make progress while another class waits unfairly long because of lock policy, priority, queue ordering, or read-heavy access.

### B16. What is special about `pthread_cond_timedwait()` compared with a sleep-loop retry?

`pthread_cond_timedwait()` atomically releases the mutex and sleeps like `pthread_cond_wait()`, then reacquires the mutex before returning. Its timeout is an absolute `timespec`, and the caller must still recheck the predicate in a loop. A sleep-loop retry usually creates races, latency, or busy waiting because it separates state checking from the wait protocol.

### B17. What happens to cancellation state across `fork()` and `exec()`?

After `fork()` in a multithreaded process, the child has only the calling thread and inherits that thread's cancelability state and type. After `exec()`, the new program's main thread starts with cancellation enabled and deferred. This matters mostly for code that mixes cancellation with process-control APIs; the safer design is still to keep cancellation simple and `fork()` then `exec()` immediately.

### B18. Which signals are process-directed and which are thread-directed?

Signals sent to the process with `kill()`, terminal-generated signals, and many software/timer signals are process-directed and may be delivered to any eligible unblocked thread. Hardware exceptions such as `SIGSEGV` and `SIGFPE`, and `SIGPIPE` from a broken pipe write, are generated for the thread that caused them. `pthread_kill()` targets one pthread in the same process.

---

## Recognize Only - Priority C

| Topic | What to know |
|---|---|
| Futex | Linux wait/wake primitive used under pthread locks when contended. Use pthread APIs unless implementing synchronization primitives. |
| Process-shared mutex/CV | Pthread sync objects can be configured for shared-memory inter-process synchronization; useful but not first-pass thread interview material. |
| Mutex types | `ERRORCHECK` can catch misuse in debug builds; `RECURSIVE` can hide confused ownership; default/normal misuse may be undefined or implementation-specific. |
| Priority inheritance | Relevant to realtime/Embedded priority inversion; learn attributes and scheduler policy only when the target requires it. |
| Full cancellation point list | Know common blocking calls and `pthread_testcancel()`; read POSIX/man-pages for portability-sensitive code. |
| `pthread_sigqueue()` | Linux-specific signal-with-data to a thread. Learn masks, `sigwait()`, and `pthread_kill()` first. |
| LinuxThreads | Obsolete historical pthread implementation with POSIX deviations; recognize it when reading old docs or maintaining old systems. |
| Exact NPTL `clone()` flags | Useful implementation knowledge, but interviews usually care more about shared vs per-thread state and debugging evidence. |
| CPU affinity | Useful for jitter-sensitive targets, but can hide capacity problems or create hot spots when used casually. |

---

## Work-Ready Debug Checklist

Use this checklist when an interviewer turns the thread topic into an incident.

- Count threads: `grep '^Threads:' /proc/<pid>/status`, `ls /proc/<pid>/task`, `ps -eLf`, `ps -T -p <pid>`.
- Find hot threads: `top -H -p <pid>`, `pidstat -t -w -p <pid> 1`.
- Collect stacks before restart: `gdb -p <pid>`, then `info threads` and `thread apply all bt`.
- Separate wait types: `strace -f -p <pid>` and `strace -f -e futex -p <pid>`.
- Investigate scheduling: `perf stat`, `perf record/report`, `perf sched record`, `perf sched latency`.
- Find races in test: ThreadSanitizer, Helgrind, DRD.
- For condition variables, inspect predicate state (`count`, `ready`, `shutdown`), not just wait/signal calls.
- For signals, inspect masks under `/proc/<pid>/task/<tid>/status`.
- For stack issues, look for large locals, recursion, guard-page SIGSEGV, and configured stack size.
- For shutdown, verify every worker has a stop path, every sleeper wakes, and every joinable thread is joined.

---

## One-Minute Review

- A process is the resource/isolation boundary; a thread is an execution flow inside it.
- Threads share memory and many process resources, so shared mutable state needs a synchronization protocol.
- Each thread has its own stack, signal mask, `errno`, TSD/TLS, and Linux task identity.
- `pthread_create()` gives no ordering guarantee; pass stable argument storage.
- Every joinable thread must be joined or detached.
- Returning from `main()` or calling `exit()` ends the process, not just one thread.
- A mutex protects state; a condition variable only announces that state may have changed.
- Always wait on condition variables with a predicate loop.
- Prefer reentrant caller-owned-buffer APIs over hidden static buffers.
- Prefer cooperative shutdown; use deferred cancellation and cleanup handlers only where cancellation is needed.
- In threaded daemons, block signals in workers and handle shutdown in one signal-waiting thread.
- After `fork()` in a multithreaded process, the child should usually `exec()` immediately.
- On Linux, NPTL makes pthreads visible as tasks for `/proc`, `ps`, `top`, GDB, `strace`, and scheduler tools.
