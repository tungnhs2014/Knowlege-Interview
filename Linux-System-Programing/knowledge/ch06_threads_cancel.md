# Chapter 6 - Thread Cancellation

> Topics: 6.4 Cancellation and cleanup handlers
> Main sources: TLPI Ch32; DevLinux Module 05
> Source notes: DevLinux covers general thread lifecycle practice but is thin on cancellation cleanup depth. TLPI is the main source for state/type, cancellation points, cleanup handler scope, and portability caveats.

## Learning Goal

Understand POSIX thread cancellation as cooperative termination with resource cleanup, not as a safe way to kill arbitrary code.

After this file, you should be able to:

- explain what `pthread_cancel()` requests and what it does not guarantee;
- design cancellation-safe blocking regions;
- use cleanup handlers for mutexes and heap allocations;
- prefer graceful shutdown flags when cancellation is the wrong tool.

## Coverage Notes

This file covers learning-map row **6.4 Thread Cancellation** and the Chapter 6 must-cover items for cancellation, cleanup handlers, cancellation points, resource ownership, safe shutdown, blocking calls, timeout-aware shutdown design, production debugging, Embedded constraints, and interview readiness.

Moved or split coverage:

- general condition-variable shutdown, timeout/recovery, lock ordering, livelock, and starvation are covered in [ch06_threads_sync.md](ch06_threads_sync.md);
- process-wide `exit()`, `exec()`, `fork()`, and signal interactions are covered in [ch06_threads_details.md](ch06_threads_details.md).

## Problem It Solves

Cancellation solves the narrow problem of asking a thread to stop while it may be blocked in a wait or I/O call. It is not a general-purpose kill switch; production code must still define where termination is allowed and how owned resources are released.

## Mental Model

Cancellation is a request. The target thread decides, through its cancelability state and type, when that request is acted on.

```text
requester: pthread_cancel(target)
    |
    v
target has pending request
    |
    +-- cancellation disabled: keep running
    +-- deferred enabled: stop at a cancellation point
    +-- async enabled: may stop almost anywhere
```

Production rule: use cancellation only where resource ownership is clear. A thread canceled while holding a mutex can deadlock the rest of the process unless cleanup is installed.

## Mechanism

The default mode is **enabled + deferred**. Deferred cancellation is practical because the thread terminates only at defined cancellation points such as blocking calls.

Common cancellation points include:

- I/O and waits: `read()`, `write()`, `accept()`, `connect()`, `select()`, `poll()`;
- sleep/wait APIs: `sleep()`, `nanosleep()`, `waitpid()`;
- pthread waits: `pthread_join()`, `pthread_cond_wait()`, `pthread_cond_timedwait()`;
- explicit check: `pthread_testcancel()`.

Cleanup handlers form a per-thread stack. If cancellation happens, handlers run in reverse push order before the thread terminates.

```text
lock mutex
push cleanup(unlock mutex)
wait at cancellation point
pop cleanup
unlock or cleanup on normal path
```

## Key APIs And Objects

| API/object | Role | Production rule |
|---|---|---|
| `pthread_cancel()` | sends cancellation request | returns before target has necessarily stopped |
| `pthread_setcancelstate()` | enable/disable acting on requests | disable only for small invariant-sensitive regions |
| `pthread_setcanceltype()` | deferred vs asynchronous | prefer deferred almost always |
| `pthread_testcancel()` | explicit cancellation point | use in compute loops |
| `pthread_cleanup_push()` | push cleanup handler | push after acquiring resource, before cancellation point |
| `pthread_cleanup_pop()` | pop cleanup handler | pair in same lexical scope |
| `PTHREAD_CANCELED` | join result for canceled thread | compare join result against this value |

For portable code, pass non-`NULL` storage for the old state/type values. Linux allows `oldstate == NULL` and `oldtype == NULL` in many cases, but POSIX portability is better with real variables.

## Lifecycle / Data Flow

Cancel and join:

```text
Thread A calls pthread_cancel(B)
    |
    v
Thread A continues immediately
    |
    v
Thread B later reaches cancellation point
    |
    v
cleanup handlers run
    |
    v
Thread B terminates with PTHREAD_CANCELED
    |
    v
Thread A or another known joiner calls pthread_join(B, &result)
```

Cancellation-safe wait:

```text
allocate buffer
lock mutex
push cleanup handler that frees buffer and unlocks mutex
while predicate false:
    pthread_cond_wait()
pop cleanup with execute=1 on normal path
```

Compute loop:

```text
while work remains:
    process bounded chunk
    pthread_testcancel()
```

Shutdown alternative:

```text
controller locks mutex
sets shutdown = true
broadcasts condvar
workers wake, observe shutdown, return normally
controller joins workers
```

This is often simpler and more auditable than cancellation.

Timeout-aware shutdown:

```text
controller sets shutdown flag and broadcasts
    |
    v
workers leave normal waits and clean up
    |
    v
controller joins with a service-level deadline
    |
    +-- success: release shared resources
    +-- timeout: capture stacks/logs, stop accepting work, escalate restart policy
```

POSIX `pthread_join()` itself has no portable timeout. If the service needs bounded shutdown, design the worker protocol so workers stop waiting promptly, or use platform-specific join-timeout APIs only behind a portability decision.

## Production Bugs And Debugging

Cancellation bugs are usually resource-lifetime bugs under a different name.

| Symptom | Likely cause | Evidence | Fix |
|---|---|---|---|
| service hangs after cancel | target canceled while holding mutex | backtraces show other threads blocked on same lock | cleanup handler unlocks or avoid cancel there |
| caller frees target's data too early | `pthread_cancel()` treated as completion | target still running in logs/backtrace | join before freeing shared resources |
| shutdown never completes | compute loop has no cancellation point | target stuck in CPU loop | add `pthread_testcancel()` or cooperative flag |
| shutdown exceeds watchdog deadline | no bounded shutdown policy | service manager kills process before cleanup | broadcast shutdown early, collect stacks, escalate predictably |
| random corruption | asynchronous cancellation used | cancellation can hit arbitrary instruction | use deferred cancellation |
| cleanup code not compiled as expected | push/pop not paired lexically | macro expansion/scope errors | keep exact same block structure |
| canceled thread leaked resources | no cleanup handler/TSD strategy | heap/FD leak after cancel path | push cleanup before blocking point |

Useful tools:

```bash
gdb -p <pid>
(gdb) thread apply all bt
strace -f -e trace=futex,read,write,select,poll ./app
grep '^Threads:' /proc/<pid>/status
```

In GDB, look for a thread blocked in a cancellation point and for other threads waiting on locks that canceled code may have owned.

## Work Checklist

Before using cancellation:

- [ ] Ask whether a shutdown flag plus condition-variable broadcast is clearer.
- [ ] Identify every resource the target may own at each cancellation point.
- [ ] Install cleanup handlers before reaching cancellation points while resources are owned.
- [ ] Keep cleanup handlers small and independent.
- [ ] Use deferred cancellation by default.
- [ ] Disable cancellation only for short critical sequences.
- [ ] Add `pthread_testcancel()` to long compute loops if cancellation is required.
- [ ] Join joinable canceled threads before freeing shared data.
- [ ] Define what evidence to collect if shutdown misses its deadline.
- [ ] Keep Embedded watchdog deadlines in mind before relying on unbounded joins.
- [ ] Do not return normal thread values that could be confused with `PTHREAD_CANCELED`.

## Recognize / Advanced

| Detail | Practical meaning |
|---|---|
| cleanup handlers may be macros | pair `push` and `pop` in the same lexical block |
| unpopped handlers and `pthread_exit()` | TLPI notes they run on `pthread_exit()`, but not on simple return |
| implementation cancellation points | glibc may mark extra blocking functions as cancellation points |
| asynchronous cancellation | only safe for very restricted code that owns no resources |
| fork/exec interaction | cancellation type/state details exist; avoid complex designs mixing these mechanisms |

Source gap: DevLinux does not cover the cleanup stack deeply. For production code, verify cancellation points and cleanup behavior from TLPI/POSIX/man-pages.

## Interview Readiness

A strong answer says cancellation is cooperative and then walks through resource cleanup.

Be ready to explain:

- why `pthread_cancel()` does not mean the target is already stopped;
- default cancelability state/type;
- what a cancellation point is;
- why compute-bound threads may ignore cancellation;
- how cleanup handlers prevent mutex leaks and heap leaks;
- why asynchronous cancellation is rarely safe;
- when graceful shutdown is better than cancellation.

Interview trap: "Cancel the worker and free the job" is unsafe. The job may still be in use until the worker reaches cancellation, cleans up, and is joined.

## Final Coverage Check

Covered: mapped row 6.4; `pthread_cancel()` request semantics; default deferred cancellation; cancellation points; cleanup handlers; resource ownership; join-after-cancel; graceful shutdown alternative; timeout-aware shutdown policy; production debugging; Embedded watchdog constraints; interview framing.

Moved: broader synchronization timeout/recovery, livelock, and starvation to `ch06_threads_sync.md`; signals/fork/exec/exit details to `ch06_threads_details.md`.
