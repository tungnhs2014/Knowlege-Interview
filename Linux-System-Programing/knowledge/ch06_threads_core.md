# Chapter 6 - Threads Core

> Topics: 6.1 Thread lifecycle, create/join/detach, threads vs processes
> Main sources: TLPI Ch29; DevLinux Module 05, Exercises 1 and 4
> Source notes: DevLinux is useful for basic create/join practice. TLPI is the semantic authority for pthread return values, join/detach lifetime, `pthread_t` opacity, and Linux-vs-POSIX caveats.

## Learning Goal

Understand a thread as an execution flow inside a process resource container, then design thread lifecycle so worker arguments, results, and cleanup are owned by the right code.

After this file, you should be able to:

- choose threads or processes for an Embedded/Linux backend design;
- create workers with stable argument storage;
- decide joinable vs detached before coding;
- debug leaked, stuck, or prematurely killed threads.

## Coverage Notes

This file covers learning-map row **6.1 Threads Introduction** and the Chapter 6 must-cover items for thread lifecycle, process-vs-thread tradeoffs, shared process resources, per-thread execution state, concurrency vs parallelism, CPU scheduling effects, blocking calls, context-switch implications, and Embedded thread-pool constraints.

Moved or split coverage:

- synchronization primitives, memory visibility, barriers, semaphores, race/deadlock/livelock/starvation, and timeout/recovery patterns are covered in [ch06_threads_sync.md](ch06_threads_sync.md);
- thread safety, reentrancy, TSD, and TLS are covered in [ch06_threads_tls.md](ch06_threads_tls.md);
- cancellation and cleanup handlers are covered in [ch06_threads_cancel.md](ch06_threads_cancel.md);
- thread stacks, signal interactions, `fork()`/`exec()`/`exit()`, and NPTL details are expanded in [ch06_threads_details.md](ch06_threads_details.md).

## Problem It Solves

Threads solve the problem of doing concurrent work inside one process without paying full process isolation and IPC costs. The tradeoff is that lifecycle, argument lifetime, shared memory, and shutdown ownership become part of the design instead of background details.

## Mental Model

A **process is the resource container**. A **thread is an execution flow** inside that container. Threads make memory sharing cheap, but every shared mutable object becomes a correctness problem.

```text
process
    owns address space, heap, globals, FDs, cwd, credentials, limits
    |
    +-- main thread: starts at main()
    +-- worker thread: starts at start(arg)
    +-- worker thread: starts at start(arg)
```

Threads are peers. The thread that creates another thread is not its "parent" in the `fork()` sense; any thread that knows a joinable thread ID may join it.

| Design question | Prefer threads when | Prefer processes when |
|---|---|---|
| Sharing data | shared cache/queue/state is central | isolation is more important |
| Failure impact | one crash can take down the service | one worker crash should be contained |
| Creation cost | many short or repeated workers | slower creation is acceptable |
| Security | same credentials are acceptable | privilege separation matters |
| Embedded constraints | bounded thread pool fits memory | separate watchdog/helper is safer |

## Mechanism

Modern Linux pthreads use **NPTL**, a one-to-one implementation: each POSIX thread maps to a kernel-scheduled task. The pthread API is POSIX; the `/proc` task IDs you see on Linux are implementation/debugging details.

Concurrency and parallelism are related but not identical:

- **concurrency** means the program has multiple flows of control that can make progress over time;
- **parallelism** means multiple flows are actually running at the same instant on different CPUs;
- a single-core system can run concurrent threads by time-slicing, but it cannot run them in parallel;
- a multicore system can run truly parallel threads, but locks, blocking calls, and CPU limits still decide whether useful work progresses.

Scheduling is the kernel choosing which runnable thread executes next. A thread blocked in `read()`, `accept()`, `pthread_mutex_lock()`, or `pthread_cond_wait()` is not consuming a CPU while blocked, but it still owns its stack and thread resources. Too many runnable threads can increase context switches and cache misses; too many blocked threads can exhaust memory, task limits, or shutdown capacity.

Shared by threads in one process:

- address space: text, globals, BSS, heap, mappings;
- open file descriptors and file offsets;
- current working directory, root directory, umask;
- signal dispositions;
- process credentials and resource limits;
- timers and many process-wide kernel resources.

Private per thread:

- stack and call frames;
- `pthread_t` identity;
- signal mask;
- `errno`;
- thread-specific data and TLS;
- alternate signal stack;
- kernel-visible task identity on Linux.

Important caveat: per-thread stacks live in the same virtual address space. A pointer to another thread's stack can be formed, but it is usually a lifetime bug waiting to happen.

## Key APIs And Objects

Many status-returning pthread functions, such as `pthread_create()`, `pthread_join()`, and `pthread_detach()`, return `0` on success or a **positive error number** on failure. Do not handle those calls as `-1` plus `errno`; check each API's return type.

| API/object | Role | Production rule |
|---|---|---|
| `pthread_create()` | starts `start(arg)` in a new thread | `arg` must remain valid until the worker is done with it |
| `pthread_join()` | waits for one known joinable thread | join exactly once, or detach instead |
| `pthread_detach()` | makes resources auto-released at thread exit | detached means no return value and no later join |
| `pthread_exit()` | exits only the calling thread | useful in `main()` if other threads should continue |
| `pthread_self()` | gets calling thread's pthread ID | use for pthread APIs, not Linux `/proc` TID |
| `pthread_equal()` | compares `pthread_t` values | `pthread_t` is opaque; avoid `==` in portable code |
| `pthread_attr_t` | creation attributes | detach state, stack size, scheduling attributes |

Compile Linux pthread code with the compiler's pthread option:

```bash
gcc -Wall -Wextra -g -pthread main.c -o app
```

The `-pthread` option is preferable to just `-lpthread` because it also sets compile-time options expected by the C library/toolchain.

## Lifecycle / Data Flow

Thread creation creates two runnable flows. There is no ordering guarantee after `pthread_create()`; the new thread may run before the creator stores or prints anything after the call.

```text
creator prepares stable argument storage
    |
    v
pthread_create(&tid, attr, start, arg)
    |
    +-- creator continues
    |
    +-- new thread runs start(arg)
            |
            +-- return value
            +-- pthread_exit(value)
            +-- cancellation
            +-- process exit kills it
```

Joinable lifecycle:

```text
created joinable by default
    |
    v
thread terminates
    |
    v
termination status and resources remain
    |
    v
another thread calls pthread_join()
    |
    v
resources released; return value copied to joiner
```

Detached lifecycle:

```text
created detached or later pthread_detach(tid)
    |
    v
thread terminates
    |
    v
resources released automatically
```

Argument and result ownership:

| Pattern | Safe? | Why |
|---|---:|---|
| pass pointer to an array element reserved for that thread | yes | storage remains stable |
| pass pointer to heap object and worker frees it | yes, if documented | one owner is clear |
| pass `&i` from a changing loop variable | no | all workers may observe the same changing object |
| return pointer to heap result and joiner frees it | yes | lifetime survives thread exit |
| return pointer to local stack object | no | stack frame is invalid after return |

## Production Bugs And Debugging

Most thread lifecycle bugs are ownership bugs: an object dies before the worker stops using it, or a thread dies without anyone collecting its state.

| Symptom | Likely cause | Evidence | Fix |
|---|---|---|---|
| workers print same ID | passed `&i` loop variable | log values change with timing | allocate per-thread args |
| crash after join result read | returned stack pointer | GDB shows invalid/corrupt address | return heap or caller-owned storage |
| process exits while workers run | `main()` returned or a worker called `exit()` | last log from main/worker before global exit | join workers; use `pthread_exit()` for one thread |
| thread count grows | joinable threads never joined | `/proc/<pid>/status` `Threads`, `/proc/<pid>/task` | join or detach every thread |
| high context switching | too many runnable workers or lock contention | `pidstat -t -w`, `perf sched`, `top -H` | use bounded pools, reduce shared locks, tune workload |
| many sleeping threads | one-thread-per-client blocked in I/O or locks | `/proc/<pid>/task`, GDB stacks in syscalls/futex waits | use worker pool or event-driven design where appropriate |
| rare race hidden by logging | assumed create/run order | disappears when prints are added | use mutex/condvar for ordering |
| nonportable thread ID logging | treating `pthread_t` as integer | works on one libc, breaks elsewhere | use `pthread_equal()` for logic; Linux TID only for debug |

Useful commands:

```bash
grep '^Threads:' /proc/<pid>/status
ls /proc/<pid>/task
ps -eLf | grep <program>
top -H -p <pid>
pidstat -t -w -p <pid> 1
perf sched record ./app
perf sched latency
gdb -p <pid>
(gdb) info threads
(gdb) thread apply all bt
```

## Work Checklist

Before creating threads:

- [ ] Decide why threads are better than processes for this design.
- [ ] Define shared state and its synchronization owner.
- [ ] Use a bounded worker pool for services instead of unbounded thread creation.
- [ ] Distinguish concurrency needs from true CPU parallelism needs.
- [ ] Check task, stack, memory, and cgroup limits before raising thread counts.
- [ ] Prepare one stable argument object per thread.
- [ ] Decide joinable vs detached before creation.
- [ ] Store every joinable `pthread_t` somewhere owned by shutdown code.
- [ ] Make worker return-value ownership explicit.
- [ ] Ensure no worker calls `exit()` unless the whole process should die.
- [ ] Plan stack size for Embedded targets and avoid large local arrays.

## Recognize / Advanced

These details matter in production reviews, but they should not distract from lifecycle ownership.

| Detail | Practical meaning |
|---|---|
| `pthread_t` reuse | after join/detach termination, an implementation may reuse IDs |
| no "join any" API | design your own state/condition variable if you need that pattern |
| Linux `gettid()` | useful for `/proc/<pid>/task` and logs; not portable pthread identity |
| NPTL | modern Linux implementation; LinuxThreads behavior is historical |
| attributes object lifetime | can be destroyed after `pthread_create()` returns |

Source caveat: TLPI discusses older LinuxThreads deviations because they mattered historically. For current Embedded Linux and backend work, assume NPTL unless you are maintaining very old systems.

## Interview Readiness

A strong answer starts with the container/flow distinction, then moves to lifecycle ownership.

Be ready to explain:

- why threads share memory cheaply but require synchronization;
- what state is process-wide versus per-thread;
- why `pthread_create()` gives no scheduling order guarantee;
- why joinable threads must be joined or detached;
- how you would debug a process with too many or stuck threads;
- when processes are safer than threads.

Interview trap: "Threads are lightweight processes" is not enough. Say what is shared, what is private, and what can break.

## Final Coverage Check

Covered: mapped row 6.1; lifecycle; create/start/join/detach; process-wide exit vs thread exit; shared process resources; per-thread stack, signal mask, `errno`, TSD/TLS, and Linux task evidence; concurrency vs parallelism; scheduling/context-switch implications; Embedded bounded-pool constraints; lifecycle debugging; interview framing.

Moved: synchronization details to `ch06_threads_sync.md`; TLS details to `ch06_threads_tls.md`; cancellation to `ch06_threads_cancel.md`; stacks/signals/NPTL edge cases to `ch06_threads_details.md`.
