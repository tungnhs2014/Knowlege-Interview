# Chapter 6 - Threads Further Details

> Topics: 6.5 Stacks, signals, fork/exec/exit, NPTL
> Main sources: TLPI Ch33; DevLinux Module 05
> Source notes: DevLinux is thin on signal/thread interactions, fork in multithreaded programs, and NPTL. TLPI is the primary source for these production caveats.

## Learning Goal

Understand the system-level traps around pthreads: stack sizing, signal delivery, process-control calls, and Linux implementation details.

After this file, you should be able to:

- size and debug thread stacks for Embedded/Linux services;
- design sane SIGTERM/SIGINT handling in a multithreaded daemon;
- avoid `fork()` deadlocks after other threads exist;
- interpret Linux thread evidence from `/proc`, `ps`, GDB, and NPTL behavior.

## Coverage Notes

This file covers learning-map row **6.5 Threads Further Details** and the Chapter 6 must-cover items for thread stacks, signals with threads, NPTL/Linux details, per-thread signal masks, process-wide signal dispositions, `fork()`/`exec()`/`exit()` interactions, blocking syscall behavior under NPTL, scheduling/debug evidence, Embedded stack/RAM constraints, and interview readiness.

Moved or split coverage:

- basic create/join/detach lifecycle and process-vs-thread tradeoffs are covered in [ch06_threads_core.md](ch06_threads_core.md);
- synchronization primitives, memory visibility, barriers, semaphores, livelock, starvation, and timeout/recovery patterns are covered in [ch06_threads_sync.md](ch06_threads_sync.md);
- cancellation cleanup is covered in [ch06_threads_cancel.md](ch06_threads_cancel.md).

## Problem It Solves

The basic pthread lifecycle is not enough for production services. This topic covers the system boundaries where thread code meets stacks, signals, process control, and Linux implementation details, because these are the places where correct-looking worker code can still fail in deployment.

## Mental Model

Threads were added to a UNIX model that already had processes and signals. Some process features remain process-wide; others are per-thread. The hard bugs appear when code assumes the old single-threaded rules still identify one obvious execution context.

```text
process-wide: signal dispositions, address space, FDs, cwd, credentials
per-thread: stack, signal mask, errno, TSD/TLS, kernel task scheduling
```

Signals and `fork()` are the danger zone. They can interact with any thread, but most application invariants are protected by ordinary thread synchronization.

Scheduling is the everyday version of that same boundary: the kernel can pause, resume, or block each thread independently. This is why a design can have good correctness but still fail latency or memory goals.

| State | Beginner meaning | Debug evidence |
|---|---|---|
| runnable | eligible to run when the scheduler picks it | `top -H`, `ps -L`, `perf sched` |
| blocked | sleeping in I/O, futex, condition wait, signal wait, or join | GDB backtrace, `strace -f`, `/proc/<pid>/task/<tid>/wchan` |
| too many runnable threads | CPU time is spent switching and refilling caches | high context-switch rate, scheduler latency |
| too many blocked threads | CPU may be calm while stacks/task slots/RAM are consumed | high thread count, many stack mappings |

## Mechanism

Thread stacks:

- each non-main thread has a fixed-size stack selected at creation;
- defaults vary by architecture, libc, resource limits, and environment;
- large automatic arrays and deep recursion can overflow worker stacks;
- many threads consume virtual address space even before they use all stack pages.

Signals:

- signal disposition/action is process-wide;
- signal mask is per-thread;
- process-directed signals can be delivered to any eligible thread;
- thread-directed signals target one thread;
- pthread APIs are not async-signal-safe for signal handlers.

Process control:

- `exec()` replaces the whole process image; other threads vanish;
- `exit()` or returning from `main()` terminates the whole process;
- `pthread_exit()` terminates only the calling thread;
- after `fork()` in a multithreaded process, the child contains only the calling thread.

NPTL and scheduling:

- modern Linux normally maps each pthread to one kernel-scheduled task;
- a blocking syscall in one thread does not block every thread in the process;
- many runnable threads can still hurt latency through context switches, cache churn, lock contention, and priority interactions;
- many blocked threads can still hurt Embedded systems through stack reservation, task limits, and slower shutdown.

## Key APIs And Objects

| API/object | Role | Production rule |
|---|---|---|
| `pthread_attr_setstacksize()` | set new thread stack size | configure before `pthread_create()` |
| `sysconf(_SC_THREAD_STACK_MIN)` | query minimum supported stack size | minimum is not a good workload size by itself |
| `pthread_sigmask()` | change calling thread's signal mask | use instead of `sigprocmask()` in threaded programs |
| `pthread_kill()` | send signal to thread in same process | target is pthread ID, not another process |
| `sigwait()` | synchronously receive blocked signals | preferred daemon pattern |
| `pthread_atfork()` | register fork handlers | specialized mitigation, not magic safety |
| `sched_getscheduler()` / `sched_getparam()` | inspect scheduling policy and priority | useful when realtime or priority bugs are suspected |
| NPTL | modern Linux pthread implementation | one pthread maps to one kernel-scheduled task |

Linux-specific recognition:

- `/proc/<pid>/task` lists kernel task IDs for threads;
- `/proc/<pid>/task/<tid>/status` shows per-task signal and scheduling/accounting evidence;
- `ps -eLf` or `ps -L` shows thread rows;
- `getconf GNU_LIBPTHREAD_VERSION` can show the glibc pthread implementation.

## Lifecycle / Data Flow

Stack sizing flow:

```text
estimate worker stack need
    |
    v
pthread_attr_init()
    |
    v
pthread_attr_setstacksize()
    |
    v
pthread_create(..., &attr, worker, arg)
    |
    v
pthread_attr_destroy()
```

Dedicated signal thread flow:

```text
main blocks SIGTERM/SIGINT before creating workers
    |
    v
workers inherit blocked signal mask
    |
    v
signal thread waits in sigwait()
    |
    v
SIGTERM arrives
    |
    v
signal thread sets shutdown flag under mutex
    |
    v
broadcast condition variable
    |
    v
workers exit and are joined
```

Fork hazard:

```text
parent: Thread B holds mutex M
parent: Thread A calls fork()
    |
    v
child: only Thread A exists
child: mutex M is copied as locked
child: Thread B does not exist to unlock it
```

Practical rule: in a multithreaded program, use `fork()` only when the child immediately calls `exec()` unless you have a carefully audited atfork design.

## Production Bugs And Debugging

These bugs are often misdiagnosed as "random Linux behavior." Usually the program relied on a single-thread assumption that is no longer true.

| Symptom | Likely cause | Evidence | Fix |
|---|---|---|---|
| worker crashes under load | stack overflow from large locals/recursion | GDB stack, guard-page SIGSEGV | move buffers to heap or set stack size |
| SIGTERM handled by random worker | process-directed signal delivered to eligible thread | logs from unexpected thread | block signals in workers, use `sigwait()` thread |
| deadlock in child after fork | inherited locked mutex with vanished owner | child stuck in pthread lock | `fork()` then `exec()` immediately |
| whole process exits from worker | worker called `exit()` | all threads disappear | return or call `pthread_exit()` |
| cleanup did not run on exec/exit | other threads vanished | missing TSD/cleanup effects | cleanly stop threads before exec/exit when needed |
| high context switches | too many runnable threads | `pidstat -t -w`, `perf sched` | bounded pools, reduce contention, adjust design |
| blocked worker does not stop whole process | expected NPTL behavior | other threads continue while one thread sleeps in syscall/futex | design shared shutdown state instead of assuming process-wide blocking |
| realtime thread starves workers | priority/policy mismatch | high-priority thread remains runnable while lower-priority work stalls | block/yield correctly, lower priority, add watchdog limits |

Useful commands:

```bash
grep '^Threads:' /proc/<pid>/status
ls /proc/<pid>/task
ps -eLf | grep <program>
top -H -p <pid>
pidstat -t -w -p <pid> 1
cat /proc/<pid>/task/<tid>/wchan
gdb -p <pid>
(gdb) info threads
(gdb) thread apply all bt
cat /proc/<pid>/task/<tid>/status | grep -E 'SigBlk|SigPnd|ShdPnd'
strace -f -e futex,clone,execve,exit_group ./app
perf sched record ./app
perf sched latency
getconf GNU_LIBPTHREAD_VERSION
```

For signal bugs, inspect masks and pending sets. For hang bugs, collect all thread backtraces before restarting the service.

## Work Checklist

For Embedded/Linux backend services:

- [ ] Avoid large automatic arrays in thread functions.
- [ ] Size stacks intentionally for high thread counts or constrained memory.
- [ ] Prefer bounded worker pools over one thread per unbounded event.
- [ ] Treat stack memory as a per-thread budget item, not a free default.
- [ ] Block shutdown signals before creating workers.
- [ ] Use one `sigwait()` thread to translate signals into normal synchronized state.
- [ ] Keep pthread calls out of async signal handlers.
- [ ] Avoid `fork()` after creating threads; if needed, child should `exec()` immediately.
- [ ] Treat `exit()` as process-wide termination.
- [ ] Use `/proc/<pid>/task` and GDB all-thread backtraces during incident debugging.
- [ ] Watch realtime priority and priority inversion risks around mutexes.
- [ ] Measure context switches before adding more worker threads to "fix" latency.

## Recognize / Advanced

| Detail | Practical meaning |
|---|---|
| NPTL one-to-one model | blocking syscall in one thread does not block all threads |
| LinuxThreads | obsolete historical implementation with many POSIX deviations |
| internal realtime signals | NPTL reserves some signals; avoid depending on every realtime signal being free |
| `RLIMIT_STACK` | may influence default stacks depending on system/runtime timing |
| alternate signal stack | per-thread; new threads need their own if they must handle on alt stack |
| process-shared mutex/condvar | advanced option for shared memory synchronization |
| realtime scheduling | can help latency but can also starve lower priority threads |
| CPU affinity | can reduce jitter in special systems, but can also hide capacity or create hot spots |

Version-sensitive note: TLPI records older LinuxThreads and early NPTL deviations. For modern glibc/Linux systems, NPTL is the normal implementation, but old vendor Embedded kernels may still carry surprises. Verify on target when behavior matters.

POSIX-vs-Linux note: `pthread_kill()` is POSIX for same-process pthreads; `/proc/<pid>/task`, kernel TIDs, `gettid()`, `pthread_sigqueue()`, and `getconf GNU_LIBPTHREAD_VERSION` are Linux/glibc-specific debugging or extension details.

## Interview Readiness

A strong answer connects thread internals to real service failures: stack overflow, wrong signal receiver, fork deadlock, and process-wide exit.

Be ready to explain:

- why each thread needs a stack and when to tune it;
- signal disposition vs signal mask;
- why a dedicated `sigwait()` thread is usually cleaner than async handlers;
- what happens to other threads on `exec()` and `exit()`;
- why `fork()` in a multithreaded process is dangerous;
- how NPTL maps pthreads to Linux scheduled tasks;
- how blocking calls and context switches affect throughput and latency;
- how to inspect and debug all threads of a live process.

Interview trap: "Only the calling thread forks" is true but incomplete. The copied mutexes, condition variables, and partially updated shared state are the reason the child can deadlock.

## Final Coverage Check

Covered: mapped row 6.5; stack sizing; signal disposition vs mask; process-directed and thread-directed signals; `sigwait()` daemon pattern; pthread async-signal-safety caveat; `fork()` after threads; `exec()` and `exit()` behavior; NPTL one-to-one model; blocking syscall behavior; scheduling/context-switch evidence; Embedded stack/RAM constraints; interview framing.

Moved: basic lifecycle to `ch06_threads_core.md`; synchronization and semaphore/barrier placement to `ch06_threads_sync.md`; cancellation to `ch06_threads_cancel.md`.
