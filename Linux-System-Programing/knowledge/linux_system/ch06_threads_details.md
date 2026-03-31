# Chapter 6 — Threads Further Details
## Topics: 6.5 Threads Further Details
> Sources: TLPI Ch33 | DevLinux Module 05

---

## 6.5 Threads Further Details

### Problem It Solves

Three areas from the traditional UNIX API interact non-trivially with multithreaded programs:

1. **Thread stacks:** Default 2 MB per thread on x86-32, 3 GB user space → max ~1500 threads. Real applications must tune stack sizes.
2. **Signals + threads:** Signal disposition is process-wide but mask is per-thread. Process-directed signals are delivered to an **arbitrary** thread — using signal handlers in multithreaded code has severe safety restrictions. There is one correct pattern.
3. **`fork()` in multithreaded processes:** `fork()` creates a child with only the calling thread. All other threads vanish — their mutexes are inherited **locked** with no owner in the child → deadlock on first mutex use.

---

### Concept Overview

| Area | Key Rule | Consequence |
|---|---|---|
| Thread stacks | 2 MB default on x86-32 | ~1500 non-main threads max; tune with `pthread_attr_setstacksize` |
| Signal disposition | **Process-wide** (shared) | One thread's `sigaction()` affects all threads |
| Signal mask | **Per-thread** | Use `pthread_sigmask()` — never `sigprocmask()` in multithreaded code |
| Signal delivery | Arbitrary thread for process-directed signals | Use dedicated signal thread + `sigwait()` |
| `fork()` in threads | Only calling thread survives in child | Inherited locked mutexes → deadlock in child |
| Safe `fork()` | Immediately follow with `exec()` | exec() discards all inherited mutex state |
| NPTL model | 1:1 — one kernel task per POSIX thread | True SMP parallelism; blocking syscall → 1 thread only |

---

### System Context

- **What it solves:** Practical constraints on stack sizing, safe asynchronous signal handling in multithreaded programs, and correct behavior around `fork()` / `exec()` / `exit()`.
- **Where in the system:** Thread stacks allocated by `pthread_create` via `mmap(MAP_ANONYMOUS|MAP_STACK)`. NPTL reads `RLIMIT_STACK` **during runtime initialization, before `main()`** to determine the per-thread default stack size. Signal mask changes go through the kernel `rt_sigprocmask` syscall per-thread — each thread has an independent mask in the kernel.
- **Subsystem interactions:** `pthread_atfork()` hooks into glibc's `fork()` wrapper. NPTL reserves two real-time signals internally (`SIGCANCEL` = `SIGRTMIN+2`, `SIGSETXID` = `SIGRTMIN+1`) — these are invisible to application code. Each thread is visible as `/proc/[pid]/task/[tid]/` — the kernel scheduler treats each as an independent scheduling unit.
- **Failure scenarios:** Calling `sigprocmask()` in multithreaded code — POSIX says "unspecified behavior". Calling `printf()` from an async signal handler while another thread holds glibc's internal stdio lock → deadlock. `fork()` without immediate `exec()` when any thread might hold a mutex → almost guaranteed deadlock in child.

---

### Internal Mechanism

#### Thread Stacks

```
x86-32 user virtual address space (3 GB usable):
┌─────────────────────────────────────────┐ 0xC000_0000
│ Kernel space                            │
├─────────────────────────────────────────┤ 0xBFFF_FFFF
│ Main thread stack (RLIMIT_STACK, 8 MB)  │ ← grows down from top
├─────────────────────────────────────────┤
│ Thread N   stack  [2 MB fixed]          │
├─────────────────────────────────────────┤
│ Thread N-1 stack  [2 MB fixed]          │
├─────────────────────────────────────────┤
│ ...                                     │
│ Max ≈ (3 GB - 8 MB main stack) / 2 MB  │ ≈ 1500 threads
├─────────────────────────────────────────┤
│ Shared libs, heap, text, data           │
└─────────────────────────────────────────┘ 0x0000_0000
```

Default stack sizes:
- x86-32: **2 MB** per non-main thread
- IA-64: **32 MB** per thread
- Minimum: `sysconf(_SC_THREAD_STACK_MIN)` = **16384** on Linux

**RLIMIT_STACK + NPTL interaction:** If `RLIMIT_STACK` is set to a non-unlimited value before program start, NPTL uses it as the default per-thread stack size. Setting it inside `main()` with `setrlimit()` is **too late** — NPTL reads it before `main()` runs. Set via `ulimit -s` in the shell before executing the program.

---

#### Signals + Threads — Attribute Mapping

| Signal Attribute | Scope | Implication |
|---|---|---|
| Disposition (`sigaction`) | **Process-wide** | All threads share the same handler table |
| Signal mask (`pthread_sigmask`) | **Per-thread** | Each thread blocks/unblocks independently |
| Pending signals | **Both** | `sigpending()` returns union of per-process + per-thread pending |
| Alternate signal stack (`sigaltstack`) | **Per-thread** | New thread starts with no alternate stack |

**Signal delivery rules:**

| Signal Source | Delivered To |
|---|---|
| Hardware exception (SIGBUS, SIGFPE, SIGILL, SIGSEGV) | Thread that caused the fault |
| Write to broken pipe (`SIGPIPE`) | Thread that called `write()` |
| `pthread_kill()` / `pthread_sigqueue()` | Specified target thread |
| All other signals (SIGINT, SIGTERM, SIGALRM, ...) | **Any one arbitrary thread that doesn't block it** |

**Why `sigprocmask()` is wrong in multithreaded code:**
`sigprocmask()` was designed for M:1 threading — it sets the mask for the kernel scheduling entity (the whole process / KSE), not the POSIX thread abstraction. POSIX says its behavior in multithreaded programs is "unspecified". On Linux/NPTL they happen to call the same underlying syscall, but code using `sigprocmask()` is not portable to M:N implementations and signals intent incorrectly to readers.

**The safe pattern — dedicated signal thread:**
```
Step 1: main(): sigfillset → pthread_sigmask(SIG_BLOCK, ALL)
        blocks ALL signals BEFORE spawning any thread

Step 2: Spawn worker threads
        → inherit blocked mask → never receive any signal directly

Step 3: Spawn ONE dedicated signal thread
        → loops on sigwait(&handled_set, &sig)
        → receives signals SYNCHRONOUSLY (no async delivery)
        → can freely call pthread functions, mutex operations, printf
        → none of the async-signal-safe restrictions apply
```

**Two threads calling `sigwait()` on the same set:** POSIX says "indeterminate" — the kernel picks one. On Linux: one waiter receives the signal, the other continues blocking. This is **not** a load-balancing mechanism — use exactly **one** dedicated signal thread per signal set.

---

#### Threads and Process Control

**`fork()` child — inherited state:**
```
Parent at time of fork():
  Thread MAIN   [running, calls fork()]
  Thread W1     [holds mutex_M, mid-update of shared_data]
  Thread W2     [blocked waiting for mutex_M]

fork() creates child:
  Thread MAIN   [clone of parent MAIN]     ← survives
  Thread W1     → VANISHED in child        ← no cleanup handler, no TSD destructor
  Thread W2     → VANISHED in child        ← no cleanup handler, no TSD destructor
  mutex_M       → LOCKED (state copied at fork moment, no owner in child)
  shared_data   → half-updated (W1 was mid-operation)

Child calls pthread_mutex_lock(&mutex_M):
  mutex is LOCKED, former owner TID (W1) does not exist
  → futex(WAIT) indefinitely → DEADLOCK
```

**`pthread_atfork()` handler ordering:**

Multiple libraries can register handlers. Ordering ensures no deadlock:
```c
pthread_atfork(prepare_A, parent_A, child_A);  /* registered first */
pthread_atfork(prepare_B, parent_B, child_B);  /* registered second */

fork():
  prepare:          B() → A()   ← LIFO (latest registered runs first)
  parent and child: A() → B()   ← FIFO (release in registration order)
```

LIFO for `prepare` mirrors nested locking: the most recently initialized library (which may depend on earlier-initialized ones) acquires its locks first. `parent`/`child` FIFO releases in the correct dependency order.

**`exec()` from a thread:**
- All threads **vanish immediately** — no cleanup handlers, no TSD destructors run
- All mutexes and memory vanish (overwritten by new program)
- Fresh program starts clean: no inherited mutex state

**`exit()` from any thread:**
- All threads terminate immediately
- `atexit()` handlers DO run (process-wide)
- Cleanup handlers and TSD destructors do **NOT** run for other threads

---

#### NPTL — 1:1 Threading Model

| Model | Kernel Sees | Advantage | Disadvantage |
|---|---|---|---|
| **M:1** (user-level) | 1 process | Fast thread ops, no syscall | Blocking syscall → entire process blocks |
| **1:1** (NPTL, Linux) | N kernel tasks | True SMP parallelism; blocking → 1 thread only | Syscall overhead per thread creation |
| **M:N** (two-level) | M KSEs for N threads | Best of both (theory) | Extremely complex; signal semantics broken in practice |

Linux NPTL (kernel ≥ 2.6 + glibc ≥ 2.3.2): one `clone()` system call per POSIX thread → each thread visible in `/proc/[pid]/task/[tid]/`. Verify: `getconf GNU_LIBPTHREAD_VERSION`.

---

### Architecture

```
Dedicated signal thread pattern — structural view:

Process virtual address space:
┌─────────────────────────────────────────────────────────┐
│  Thread: main                                           │
│    1. sigfillset → pthread_sigmask(SIG_BLOCK, ALL, NULL)│ ← blocks ALL first
│    2. pthread_create(&w1/w2/w3)  workers inherit mask   │
│    3. pthread_create(&sig_tid)   signal thread          │
├─────────────────────────────────────────────────────────┤
│  Thread: worker_1   signal mask: ALL BLOCKED            │
│  Thread: worker_2   signal mask: ALL BLOCKED            │ ← never receive signals
│  Thread: worker_3   signal mask: ALL BLOCKED            │
├─────────────────────────────────────────────────────────┤
│  Thread: signal_thread                                  │
│    sigwait(&handled, &sig)  ← SYNCHRONOUS receive       │ ← only handler
│    on SIGINT/SIGTERM:                                   │
│      lock(mtx) → shutdown=1 → broadcast(cond) → return │
└─────────────────────────────────────────────────────────┘

Signal arrives (e.g. SIGINT via Ctrl+C):
  All threads have it blocked in kernel mask EXCEPT sigwait's internal unblock
  → signal_thread.sigwait() returns with sig = SIGINT
  → handled synchronously, no async delivery, no async-signal-safe restrictions
```

---

### Execution Flow

```
fork() mutex deadlock — step by step:

Parent state (3 threads active):
  MAIN           [running]
  W1             [holds mutex_M, inside critical section]  ← fork() called here
  W2             [blocked in pthread_mutex_lock(&mutex_M)]

fork() syscall:
  Kernel: duplicate only MAIN's thread context
  Child:  MAIN   [clone of parent MAIN at fork() call site]
          W1     → simply gone (no cleanup, no unlock)
          W2     → simply gone
          mutex_M→ inherited with state = LOCKED, owner_tid = W1's TID
                   W1's TID does not exist in child

Child continues after fork():
  pthread_mutex_lock(&mutex_M)
  → kernel sees mutex LOCKED
  → futex(WAIT, …) → sleeps waiting for W1 to unlock
  → W1 does not exist → nobody will ever unlock
  → THREAD DEADLOCKED (no timeout by default)

Safe pattern that avoids deadlock:
  fork()
  → child: execlp("newprog", ...) immediately
  → exec() overwrites ALL memory: mutex_M gone, fresh program, clean state ✓
```

---

### Core API

#### Thread Stack Attributes

```c
#include <pthread.h>
#include <unistd.h>

/* Set stack size in thread attribute object before pthread_create */
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
/* stacksize: must be >= sysconf(_SC_THREAD_STACK_MIN); page-aligned recommended */
/* Returns 0, or EINVAL if too small */

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);

/* Control both stack address and size (reduces portability) */
int pthread_attr_setstack(pthread_attr_t *attr, void *stackaddr, size_t stacksize);

/* System minimum stack size */
long min = sysconf(_SC_THREAD_STACK_MIN);   /* typically 16384 bytes */

/* Query actual attributes of a running thread (Linux-specific) */
int pthread_getattr_np(pthread_t thread, pthread_attr_t *attr);
/* Returns 0; caller must call pthread_attr_destroy() when done */
```

#### Signal Functions for Threads

```c
#include <signal.h>

/* Modify calling thread's signal mask (use instead of sigprocmask in multithreaded) */
int pthread_sigmask(int how, const sigset_t *set, sigset_t *oldset);
/* how: SIG_BLOCK | SIG_UNBLOCK | SIG_SETMASK */
/* Returns 0 on success */

/* Send signal to specific thread in same process */
int pthread_kill(pthread_t thread, int sig);
/* sig=0: test thread existence without sending any signal */
/* Returns 0 on success */

/* Synchronously receive a signal from set (blocks until one arrives) */
int sigwait(const sigset_t *set, int *sig);
/* PRECONDITION: all signals in set MUST be BLOCKED in the calling thread */
/* sig: OUTPUT — receives signal number that was received */
/* Returns 0 on success */
```

#### Fork Handlers

```c
#include <pthread.h>

int pthread_atfork(void (*prepare)(void),  /* in parent, BEFORE fork — LIFO order */
                   void (*parent)(void),   /* in parent, AFTER fork  — FIFO order */
                   void (*child)(void));   /* in child,  AFTER fork  — FIFO order */
/* Returns 0 on success */
/* prepare: lock relevant mutexes */
/* parent/child: unlock those mutexes */
```

---

### Example (Code)

#### Example 1 — Stack sizing: deep recursion vs lightweight threads

```c
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

/* Simulates deep call stack: 1 KB per frame */
static int recursive_work(int depth) {
    char frame_data[1024];
    frame_data[0] = (char)depth;
    if (depth <= 0) return 0;
    return depth + recursive_work(depth - 1);
}

static void *heavy_worker(void *arg) {
    /* 500 frames × 1 KB = 500 KB — fits in 8 MB, not in default 2 MB */
    printf("[heavy] result = %d\n", recursive_work(500));
    return NULL;
}

static void *light_worker(void *arg) {
    int id = *(int *)arg;
    printf("[light %d] done\n", id);
    return NULL;
}

int main(void) {
    long min_stack = sysconf(_SC_THREAD_STACK_MIN);
    printf("System min stack: %ld bytes\n", min_stack);

    /* 8 MB stack for deep recursion */
    pthread_t heavy;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 8 * 1024 * 1024);
    pthread_create(&heavy, &attr, heavy_worker, NULL);
    pthread_attr_destroy(&attr);

    /* 10 lightweight threads, minimal stack */
    pthread_t lights[10];
    int ids[10];
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, min_stack * 4);   /* 64 KB */
    for (int i = 0; i < 10; i++) {
        ids[i] = i;
        pthread_create(&lights[i], &attr, light_worker, &ids[i]);
    }
    pthread_attr_destroy(&attr);

    /* Query actual stack of running thread */
    pthread_attr_t qattr;
    size_t actual;
    pthread_getattr_np(heavy, &qattr);
    pthread_attr_getstacksize(&qattr, &actual);
    printf("[main] heavy thread actual stack: %.1f MB\n", actual / (1024.0*1024));
    pthread_attr_destroy(&qattr);

    pthread_join(heavy, NULL);
    for (int i = 0; i < 10; i++)
        pthread_join(lights[i], NULL);
    return 0;
}
/* gcc -pthread ex1_stack.c -o ex1 */
```

---

#### Example 2 — Dedicated signal thread (`sigwait` pattern)

```c
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

static int shutdown_flag = 0;
static pthread_mutex_t mtx  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;

static void *signal_thread(void *arg) {
    sigset_t *set = (sigset_t *)arg;
    int sig;
    while (1) {
        sigwait(set, &sig);   /* blocks synchronously until signal arrives */
        printf("[sig_thread] received signal %d\n", sig);
        if (sig == SIGINT || sig == SIGTERM) {
            /* SAFE: we are in a normal thread, not an async handler.
             * Can call pthread functions, printf, malloc — no restrictions. */
            pthread_mutex_lock(&mtx);
            shutdown_flag = 1;
            pthread_cond_broadcast(&cond);
            pthread_mutex_unlock(&mtx);
            return NULL;
        }
        /* Handle SIGHUP, SIGUSR1, etc. safely here */
    }
}

static void *worker(void *arg) {
    int id = *(int *)arg;
    printf("[worker %d] started\n", id);
    pthread_mutex_lock(&mtx);
    while (!shutdown_flag) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1;
        pthread_cond_timedwait(&cond, &mtx, &ts);
    }
    pthread_mutex_unlock(&mtx);
    printf("[worker %d] exiting cleanly\n", id);
    return NULL;
}

int main(void) {
    /* STEP 1: Block ALL signals in main BEFORE creating any thread */
    sigset_t all;
    sigfillset(&all);
    pthread_sigmask(SIG_BLOCK, &all, NULL);

    /* STEP 2: Workers inherit blocked mask — never receive signals directly */
    pthread_t workers[3];
    int ids[3] = {1, 2, 3};
    for (int i = 0; i < 3; i++)
        pthread_create(&workers[i], NULL, worker, &ids[i]);

    /* STEP 3: Create ONE dedicated signal thread */
    sigset_t handled;
    sigemptyset(&handled);
    sigaddset(&handled, SIGINT);
    sigaddset(&handled, SIGTERM);
    sigaddset(&handled, SIGHUP);
    pthread_t sig_tid;
    pthread_create(&sig_tid, NULL, signal_thread, &handled);

    pthread_join(sig_tid, NULL);
    for (int i = 0; i < 3; i++)
        pthread_join(workers[i], NULL);
    printf("[main] clean shutdown\n");
    return 0;
}
/* gcc -pthread ex2_sigwait.c -o ex2
   Run: ./ex2 then Ctrl+C or: kill -TERM $(pidof ex2) */
```

---

#### Example 3 — Safe `fork()` pattern: fork + immediate exec

```c
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static int shared_counter = 0;

/* Worker that holds mutex for periods */
static void *worker(void *arg) {
    while (1) {
        pthread_mutex_lock(&mtx);
        shared_counter++;
        usleep(5000);          /* hold mutex 5 ms — likely held at fork time */
        pthread_mutex_unlock(&mtx);
        usleep(1000);
    }
    return NULL;
}

int main(void) {
    pthread_t tid;
    pthread_create(&tid, NULL, worker, NULL);
    usleep(20000);   /* let worker run and possibly be holding mutex */

    printf("[parent] forking (worker may hold mutex)\n");
    pid_t pid = fork();

    if (pid == 0) {
        /* CHILD: worker thread vanished.
         * mutex_M may be LOCKED — inherited from parent at fork moment.
         *
         * SAFE: exec() immediately.
         *   exec() overwrites all process memory → mutex state gone → clean start.
         *
         * UNSAFE: calling pthread_mutex_lock(&mtx) here
         *   → mutex already locked, no owner → DEADLOCK.
         */
        printf("[child PID=%d] counter snapshot: %d — about to exec\n",
               getpid(), shared_counter);
        execlp("/bin/echo", "echo",
               "[child] replaced by /bin/echo — all mutex state discarded", NULL);
        _exit(127);   /* exec failed */
    }

    /* PARENT: worker thread continues normally */
    int status;
    waitpid(pid, &status, 0);
    printf("[parent] child done. counter now: %d\n", shared_counter);

    pthread_cancel(tid);
    pthread_join(tid, NULL);
    return 0;
}
/* gcc -pthread ex3_fork_exec.c -o ex3 */
```

---

### Debugging

```bash
# List all threads of a process
ls /proc/$(pidof binary)/task/
cat /proc/$(pidof binary)/status | grep Threads

# Per-thread signal mask (hex bitmask of blocked signals)
cat /proc/$(pidof binary)/task/<TID>/status | grep -E "SigBlk|SigPnd"

# Show threads with ps
ps -eLf | grep binary   # LWP column = kernel TID

# GDB multithreaded
gdb ./binary
(gdb) info threads                   # list all threads
(gdb) thread 3                       # switch to thread 3
(gdb) thread apply all backtrace     # backtrace for every thread

# Verify NPTL version
getconf GNU_LIBPTHREAD_VERSION        # e.g.: NPTL 2.35

# Check stack size limit
ulimit -s                             # in KB (default: 8192 = 8 MB)
ulimit -s 4096                        # set 4 MB before running program
```

---

### Real-world Usage

- **Nginx:** Uses N worker **processes** instead of threads specifically to avoid `fork()` + mutex deadlock. Signal handling done in single-threaded master process via `sigaction()` — safe because it's single-threaded.
- **Go runtime:** M:N goroutine scheduler. All OS threads block all signals; a dedicated goroutine running on a dedicated OS thread handles signals synchronously via equivalent of `sigwait`.
- **JVM:** Uses `pthread_atfork()` in glibc internals to manage JVM-internal mutex state across `Runtime.exec()` (which calls `fork()+exec()`). Without this, child would inherit locked JVM mutex state.
- **PostgreSQL:** Deliberately single-process-per-connection via `fork()` — sidesteps all fork+mutex complications, at the cost of per-connection memory overhead.
- **Redis:** Single-threaded event loop + BIO (Background I/O) threads. BIO uses `pthread_attr_setstacksize` to reduce stack to 32 KB per BIO thread — allows far more BIO threads before virtual memory exhaustion on 32-bit systems.

---

### Key Takeaways

| Concept | Rule |
|---|---|
| Stack default | 2 MB on x86-32; ~1500 non-main threads before virtual memory exhausted |
| `RLIMIT_STACK` | Set via `ulimit -s` BEFORE running program; `setrlimit()` inside `main()` is too late |
| Signal disposition | Process-wide — one `sigaction()` call affects all threads |
| Signal mask | Per-thread — use `pthread_sigmask()`, never `sigprocmask()` in multithreaded code |
| Safe signal pattern | Block all in main first, then `sigwait()` in one dedicated thread |
| Multiple `sigwait` threads | POSIX: indeterminate which receives — use exactly 1 dedicated signal thread |
| `fork()` in threads | Only calling thread survives; inherited locked mutexes → deadlock in child |
| Safe `fork()` rule | `fork()` → `exec()` immediately; do nothing in between in child |
| `exec()` in thread | All threads vanish instantly, no cleanup/TSD runs for them |
| `exit()` from thread | All threads terminate; `atexit()` runs; cleanup/TSD do NOT |
| NPTL | 1:1 model — each POSIX thread = one kernel task = one `/proc/[pid]/task/[tid]/` entry |
