# Chapter 6 — Threads Core
## Topics: 6.1 Threads Introduction · 6.2 Thread Synchronization
> Sources: TLPI Ch29, Ch30, Ch33 | DevLinux Module 05

---

## 6.1 Threads Introduction

### Problem It Solves

The classical UNIX approach to concurrency is `fork()` — create a child process per task. This breaks down at scale for two reasons:

1. **Sharing is expensive:** parent and child live in separate address spaces; exchanging data requires IPC (pipes, shared memory, sockets)
2. **`fork()` is costly:** even with copy-on-write, duplicating page tables and file descriptor tables takes non-trivial time

Threads solve both: all threads inside a process **share the same virtual address space** — code, heap, globals, open file descriptors. Communication is a read/write to a shared variable. And thread creation is approximately **10× faster** than `fork()` because the kernel (via `clone()`) skips memory duplication entirely.

---

### Concept Overview

A **thread** is an independent execution stream inside a process. Multiple threads run concurrently, sharing most process attributes but maintaining their own execution context (registers, stack, signal mask).

```
Process virtual address space (Linux/x86-64)
────────────────────────────────────────────
0xFFFF…  ┌─────────────────────┐
         │   Kernel space      │
         ├─────────────────────┤
         │  Stack: main thread │ ← per-thread, grows downward
         ├─────────────────────┤
         │  Stack: thread 3    │
         ├─────────────────────┤
         │  Stack: thread 2    │
         ├─────────────────────┤
         │  Stack: thread 1    │
         ├─────────────────────┤
         │  Shared libs /      │ ← shared by ALL threads
         │  shared memory      │
         ├─────────────────────┤
         │  Heap               │ ← shared: dynamic malloc/free
         ├─────────────────────┤
         │  BSS (uninit globals│ ← shared
         ├─────────────────────┤
         │  Data (init globals)│ ← shared
         ├─────────────────────┤
         │  Text (code)        │ ← shared: all threads execute here
0x0000…  └─────────────────────┘
```

**Shared across all threads (process-wide):**
- Virtual address space: text, data, BSS, heap
- Open file descriptors and their positions
- Signal dispositions (`sigaction` table)
- Process ID, parent PID, process group ID, session ID
- Current working directory, umask, root directory
- Resource limits (`RLIMIT_*`), nice value
- POSIX timers, CPU time consumed

**Per-thread (private to each thread):**
- Thread ID (`pthread_t`)
- Stack (local variables + function call chain)
- Signal mask (which signals are currently blocked)
- `errno` — per-thread lvalue via macro expansion
- Alternate signal stack (`sigaltstack`)
- Floating-point environment
- Realtime scheduling policy and priority
- CPU affinity

---

### System Context

- **What it solves:** Enables concurrent execution within a single process, with cheap data sharing compared to multi-process designs.
- **Where in the system:** Threads are scheduled entities at kernel level (NPTL uses a 1:1 model — one kernel scheduling entity per POSIX thread). The kernel sees them as tasks created via `clone()` with shared VM flags.
- **Subsystem interactions:** Threads interact with the pthread library (libpthread / glibc NPTL), the kernel scheduler (CFS), the VM subsystem (shared page tables), and the signal subsystem (per-thread masks, process-wide dispositions).
- **Failure scenarios:** A bug in one thread (NULL dereference, stack overflow, buffer overflow) delivers a signal (SIGSEGV, SIGBUS) to that thread — because the default disposition is terminate, the **entire process is killed**, taking all other threads with it. Failing to `join` or `detach` joinable threads produces thread zombies that accumulate until `pthread_create` fails with `EAGAIN`.

---

### Internal Mechanism

#### Linux implements threads via `clone()`

POSIX threads are not a separate kernel concept. On Linux, `pthread_create()` internally calls `clone()` with a set of sharing flags:

```c
clone(CLONE_VM       /* share virtual memory (page tables) */
    | CLONE_FS       /* share filesystem info: cwd, umask, root */
    | CLONE_FILES    /* share file descriptor table */
    | CLONE_SIGHAND  /* share signal disposition table */
    | CLONE_THREAD   /* same thread group (same PID visible to userspace) */
    | CLONE_SETTLS   /* set thread-local storage (TLS) pointer */
    | ...,
    child_stack, flags, arg, ...);
```

Contrast with `fork()` which uses none of these flags → child gets **copies** (with copy-on-write) of everything above.

**Implication:** Thread creation on Linux is fast because no page table duplication occurs — all threads use the same page directory. The kernel simply allocates a new `task_struct`, a new stack region, and a new TLS block.

#### NPTL (Native POSIX Threads Library)

Linux's current implementation since kernel 2.6 + glibc 2.3.2:
- **1:1 model**: one kernel thread per POSIX thread (no user-space multiplexing)
- Thread is visible in `/proc` as a separate entry under the process's thread group
- `getpid()` returns the same PID for all threads; `gettid()` returns unique kernel TID
- POSIX `pthread_t` is a userspace opaque value (on Linux/NPTL: `unsigned long` cast from a pointer), **not** the same as kernel TID

#### Per-thread `errno`

Traditional `errno` is a global `int`. In a multi-threaded program this would cause races. The fix: `<errno.h>` defines `errno` as a macro expanding to a function call returning a per-thread modifiable lvalue:

```c
/* Actual expansion on glibc */
#define errno (*__errno_location())
/* __errno_location() reads the TLS block of the calling thread */
```

Each thread has its own `errno` slot in its TLS block — no sharing, no locking needed.

---

### Architecture

#### Thread lifecycle state machine

```
pthread_create()
       │
       ▼
   [READY] ←──────────────────┐
       │ scheduler selects     │
       ▼                       │ preempted
   [RUNNING] ─── blocking op ──► [BLOCKED]
       │          (mutex, I/O,         │
       │           cond_wait)          │ resource ready
       │                              └──► [READY]
       │ pthread_exit() / return
       ▼
  [TERMINATED]
       │
       ├── joinable (default): waits as zombie until pthread_join()
       │                       → caller gets return value → resources freed
       │
       └── detached:           resources freed immediately
                               no return value retrievable
```

---

### Execution Flow

#### `pthread_create()` call sequence

```
Caller thread                     Kernel / NPTL
──────────────────                ────────────────────────────────
pthread_create(&tid, attr,
               start_fn, arg)
       │
       ├─ allocate new stack region (mmap)
       ├─ set up TLS block (errno, pthread_t identity)
       ├─ call clone() with CLONE_VM | CLONE_FILES | ...
       │                                 │
       │                         kernel creates task_struct
       │                         schedules new thread
       │                                 │
       ◄── pthread_create returns 0      │
       │   tid is filled                 ▼
       │                         new thread calls start_fn(arg)
       │
       │   [NO ORDERING GUARANTEE]
       │   — caller and new thread race for CPU
```

---

### Core API

#### `pthread_create` — Create a new thread

```c
#include <pthread.h>

int pthread_create(pthread_t *thread,
                   const pthread_attr_t *attr,
                   void *(*start)(void *),
                   void *arg);
/* Returns 0 on success, positive errno value on error
 * NOTE: does NOT set errno — check return value directly */
```

- `thread`: output — filled with the new thread's ID before the call returns
- `attr`: thread attributes (stack size, detach state, scheduling policy); `NULL` = defaults
- `start`: the function the new thread begins executing
- `arg`: single `void *` argument passed to `start`; use a struct for multiple values

#### `pthread_exit` — Terminate the calling thread

```c
void pthread_exit(void *retval);
/* Never returns */
```

- Equivalent to `return retval` from the thread start function
- `retval` must **not** point to the thread's own stack — stack is invalidated after exit
- If called from `main()`: only the main thread terminates; other threads continue running; process stays alive until the last thread exits or any thread calls `exit()`

#### `pthread_join` — Wait for a thread to terminate

```c
int pthread_join(pthread_t thread, void **retval);
/* Returns 0 on success, positive errno value on error */
```

- Blocks until the target thread terminates
- `retval` receives the thread's return value (what it passed to `return` or `pthread_exit`)
- A joinable thread that nobody joins becomes a **thread zombie** — wastes resources, eventually causes `pthread_create` to fail
- Calling `pthread_join` twice on the same TID is undefined behavior (TID may be reused)
- Threads are peers: any thread can join any other; there is no parent–child join hierarchy

#### `pthread_detach` — Detach a thread (no join needed)

```c
int pthread_detach(pthread_t thread);
/* Returns 0 on success, positive errno value on error */
/* A thread can detach itself: pthread_detach(pthread_self()); */
```

- After detach, the system automatically frees resources when the thread terminates
- Cannot join a detached thread; cannot make it joinable again
- Does **not** protect against `exit()` from another thread — detached or not, all threads die when the process exits

#### `pthread_self` / `pthread_equal` — Thread identity

```c
pthread_t pthread_self(void);
/* Returns the calling thread's TID */

int pthread_equal(pthread_t t1, pthread_t t2);
/* Returns nonzero if equal, 0 if different */
/* Use this — never use == on pthread_t (opaque type) */
```

#### Compile flag

```bash
gcc -pthread source.c -o binary
# -pthread: defines _REENTRANT + links -lpthread
```

---

### Example (Code)

#### 1. Basic: create, join, retrieve return value

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

void *thread_func(void *arg)
{
    int id = *((int *)arg);
    printf("Thread %d: TID=%lu\n", id, (unsigned long)pthread_self());
    return (void *)(long)id;   /* cast int→long→void* to avoid UB */
}

int main(void)
{
    pthread_t threads[2];
    int ids[2] = {1, 2};      /* separate storage — avoid &loop_var race */

    for (int i = 0; i < 2; i++) {
        int s = pthread_create(&threads[i], NULL, thread_func, &ids[i]);
        if (s != 0) {
            fprintf(stderr, "pthread_create: %s\n", strerror(s));
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < 2; i++) {
        void *retval;
        pthread_join(threads[i], &retval);
        printf("Main: thread %d returned %ld\n", i + 1, (long)retval);
    }
    return 0;
}
```

#### 2. Passing multiple arguments via struct; heap-allocated return value

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct {
    int  id;
    int  delay_ms;
    char name[32];
} ThreadArgs;

typedef struct {
    int  processed;
    long tid;
} ThreadResult;

void *worker(void *arg)
{
    ThreadArgs   *a   = (ThreadArgs *)arg;
    ThreadResult *res = malloc(sizeof(*res)); /* heap: survives thread exit */
    if (!res) pthread_exit(NULL);

    usleep(a->delay_ms * 1000);
    printf("Worker %d (%s): done\n", a->id, a->name);

    res->processed = a->id * 10;
    res->tid       = (long)pthread_self();
    return res;        /* caller must free() */
}

int main(void)
{
    pthread_t  tids[3];
    ThreadArgs args[3] = {
        {1, 300, "alpha"},
        {2, 100, "beta"},
        {3, 200, "gamma"},
    };

    for (int i = 0; i < 3; i++)
        pthread_create(&tids[i], NULL, worker, &args[i]);
    /* args[] is on main's stack — valid until pthread_join returns */

    for (int i = 0; i < 3; i++) {
        void *retval;
        pthread_join(tids[i], &retval);
        ThreadResult *res = (ThreadResult *)retval;
        printf("Thread %d: processed=%d tid=%ld\n",
               i + 1, res->processed, res->tid);
        free(res);
    }
    return 0;
}
```

#### 3. Detached thread — fire-and-forget

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void *background_logger(void *arg)
{
    pthread_detach(pthread_self());   /* self-detach; no join needed */
    for (int i = 0; i < 5; i++) {
        printf("[LOG] tick %d\n", i);
        sleep(1);
    }
    return NULL;   /* resources freed automatically */
}

int main(void)
{
    pthread_t logger;
    pthread_create(&logger, NULL, background_logger, NULL);
    printf("Main: logger spawned\n");

    /* pthread_exit(NULL): main exits, logger keeps running
     * return 0         : exit() called → ALL threads killed immediately */
    pthread_exit(NULL);
}
```

#### 4. `pthread_exit` in main — atexit trap

```c
#include <stdio.h>
#include <pthread.h>

void cleanup(void) { printf("atexit: cleanup!\n"); }

void *worker(void *arg) { sleep(2); return NULL; }

int main(void)
{
    pthread_t t;
    atexit(cleanup);
    pthread_create(&t, NULL, worker, NULL);

    /* return 0;          → calls exit() → cleanup() IS called      ✓ */
    pthread_exit(NULL);   /* bypasses exit() → cleanup() NOT called ✗ */
}
```

#### 5. Classic bug: dangling pointer to main's stack

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct { int x; int y; } Point;

void *buggy_worker(void *arg)
{
    sleep(1);          /* main thread exits via pthread_exit() first  */
    Point *p = (Point *)arg;
    /* main's stack is reclaimed → p is a dangling pointer
     * result: undefined behavior (segfault, garbage, or "works") */
    printf("BUG  : x=%d y=%d\n", p->x, p->y);
    return NULL;
}

void *safe_worker(void *arg)
{
    sleep(1);
    Point *p = (Point *)arg;   /* heap — outlives main's stack frame */
    printf("SAFE : x=%d y=%d\n", p->x, p->y);
    free(p);
    return NULL;
}

int main(void)
{
    pthread_t t1, t2;

    /* BUG: stack address passed to thread that outlives main's stack */
    Point stack_pt = {10, 20};
    pthread_create(&t1, NULL, buggy_worker, &stack_pt);

    /* SAFE: heap address passed — valid until free() */
    Point *heap_pt  = malloc(sizeof(Point));
    heap_pt->x = 10; heap_pt->y = 20;
    pthread_create(&t2, NULL, safe_worker, heap_pt);

    pthread_exit(NULL);   /* main exits; stack_pt memory is reclaimed */
}
```

---

### Debugging

#### Detect thread leaks (zombie threads)

```bash
# List all threads of a process
ls /proc/<pid>/task/

# Count threads
cat /proc/<pid>/status | grep Threads

# Valgrind Helgrind — detect misuse of pthread API
valgrind --tool=helgrind ./binary

# ThreadSanitizer — compile-time thread error detector
gcc -fsanitize=thread -g -pthread source.c -o binary
./binary
```

#### Common error patterns

```
pthread_create: Resource temporarily unavailable   (EAGAIN)
→ Too many threads / zombie threads accumulating
→ Check: every joinable thread must be joined or detached

Segmentation fault (core dumped)
→ Thread accessed dangling pointer to another thread's (or main's) stack
→ Check: anything passed as `arg` must outlive the thread

undefined behavior / garbage data
→ Race on loop variable: pthread_create(..., &i) in loop
→ Each thread must get its own copy of loop index
```

#### GDB multi-thread commands

```bash
(gdb) info threads           # list all threads with IDs
(gdb) thread 2               # switch to thread 2
(gdb) thread apply all bt    # backtrace of every thread simultaneously
(gdb) set scheduler-locking on   # freeze other threads while stepping
```

---

### Real-world Usage

| Pattern | Description |
|---|---|
| **Thread pool** | Pre-create N threads, feed work via queue; amortizes creation overhead |
| **Dedicated signal thread** | Block all signals in main before spawning workers; one thread calls `sigwait()` — handles signals synchronously in normal context, avoiding async-signal-safe restrictions |
| **Worker + collector** | Workers write results to heap structs; main thread joins and collects via `pthread_join` `retval` |
| **Detached background tasks** | Logging, heartbeat, stat collection — fire-and-forget, no return value needed |
| **Parallel divide-and-conquer** | Split array into N segments, one thread per segment (e.g., parallel sort, parallel sum) |

---

### Key Takeaways

1. **Linux threads are `clone()` with shared VM flags** — not a separate OS concept; NPTL uses 1:1 kernel thread per POSIX thread.

2. **Shared vs. per-thread**: signal *disposition* is process-wide (policy); signal *mask* is per-thread (filter). `errno` is per-thread via TLS macro expansion.

3. **Every joinable thread must be joined or detached** — a terminated unjoinable thread becomes a zombie. Enough zombies prevent further `pthread_create`.

4. **Return values from threads must live on the heap** — the thread's stack is invalidated on exit; returning a pointer to a local variable is undefined behavior.

5. **`pthread_exit()` in main ≠ `return`/`exit()`**: only the main thread terminates; other threads continue. The process stays alive. `atexit()` handlers are NOT called.

6. **No "join any thread"** — intentional: peer equality means library-internal threads could be accidentally joined, destroying the library's ability to clean up its own threads.

7. **Thread creation is ~10× faster than `fork()`** — no page table duplication, no COW setup, no FD table copy.

8. **Passing `&loop_var` to `pthread_create` is a race** — use a separate array or cast the integer directly: `(void *)(long)i`.

---

## 6.2 Thread Synchronization

### Problem It Solves

Threads share memory — that is the advantage. But shared mutable state accessed concurrently without coordination produces **race conditions**: results that vary between runs depending on CPU scheduling. Even a seemingly simple `glob++` is not atomic — it compiles to load / increment / store, and a preemption between any two of those steps corrupts the result.

Two tools address this:
- **Mutex** — ensures only one thread executes a critical section at a time (mutual exclusion)
- **Condition variable** — allows a thread to sleep efficiently until another thread changes shared state, avoiding CPU-burning busy-wait loops

---

### Concept Overview

**Race condition anatomy:**

```
glob = 2000

Thread 1: loc = glob          (= 2000)
Thread 1: [PREEMPTED] ────────────────────────────────────
          Thread 2: runs 1000 loops → glob = 3000
Thread 1: [RESUMED]
Thread 1: loc++  → loc = 2001
Thread 1: glob = loc          → glob = 2001   ← 1000 increments LOST
```

**Critical section:** a code block that accesses shared resources and must execute atomically — no other thread may interleave while it runs.

**Mutex semantics:**
- At most one thread holds the lock at any time
- Only the owner can unlock
- Other threads that call `lock` block until the mutex is released
- Advisory, not mandatory — all threads must cooperate

**Condition variable semantics:**
- Stateless — holds no information; a signal not caught by a waiting thread is **lost**
- Always paired with a mutex (same mutex for all concurrent waiters on the same CV)
- `wait` atomically unlocks the mutex and sleeps; on wakeup re-locks before returning
- Predicate (shared variable) carries the real state; CV is only the notification mechanism

---

### System Context

- **What it solves:** Prevents data races on shared variables and provides efficient thread coordination without busy-waiting.
- **Where in the system:** Mutexes and CVs live in userspace memory (pthread library structs). For uncontended mutexes, no kernel involvement occurs — only an atomic CAS in userspace. Kernel is entered only on contention via the `futex()` syscall.
- **Subsystem interactions:** Mutexes are built on **futexes** (fast userspace mutexes) — a Linux kernel primitive that parks/unparks threads without a syscall in the common uncontended case. Condition variables are implemented on top of futexes as well. The kernel CFS scheduler decides which unblocked thread runs next.
- **Failure scenarios:** Forgetting to lock before accessing shared state → silent data corruption that manifests non-deterministically. Locking in inconsistent order across threads → deadlock (all involved threads block forever, unkillable without `SIGKILL`). Calling `pthread_cond_wait` with no predicate check → spurious wakeups cause incorrect behavior. Destroying a locked mutex or a CV with active waiters → undefined behavior.

---

### Internal Mechanism

#### Mutex — Futex-based implementation

```
Uncontended (fast path — no syscall):
──────────────────────────────────────
pthread_mutex_lock()
    │
    └─ atomic CAS on a 32-bit word in the mutex struct
       if word was 0 (unlocked) → set to TID, return immediately
       cost: ~10 ns, pure userspace

Contended (slow path — syscall):
──────────────────────────────────
pthread_mutex_lock()
    │
    └─ CAS fails (someone else holds lock)
       → futex(FUTEX_WAIT) syscall: thread suspended into kernel wait queue
       → when owner calls pthread_mutex_unlock():
          atomic release → futex(FUTEX_WAKE): one waiter unblocked → tries CAS
```

**Why mutexes are ~10× faster than semaphores or file locks:** semaphores always require a syscall per lock/unlock. Mutexes only pay syscall cost on contention.

#### Mutex types

| Type | Self-lock behavior | Use case |
|---|---|---|
| `PTHREAD_MUTEX_NORMAL` (default) | Deadlock | Production: lowest overhead |
| `PTHREAD_MUTEX_ERRORCHECK` | Returns `EDEADLK` | Debug: catches all misuse |
| `PTHREAD_MUTEX_RECURSIVE` | Increments lock count | Reentrant code (use sparingly) |

#### Condition variable — atomic unlock + sleep

The critical property of `pthread_cond_wait(&cond, &mutex)` is that unlock and sleep happen **atomically**:

```
pthread_cond_wait(&cond, &mtx):
┌────────────────────────────────────────────────────┐
│ 1. Unlock mtx                                      │ ← atomic pair
│ 2. Suspend thread on cond (no CPU consumed)        │ ← atomic pair
└────────────────────────────────────────────────────┘
   ... thread sleeps until signal/broadcast ...
┌────────────────────────────────────────────────────┐
│ 3. Re-lock mtx, then return                        │
└────────────────────────────────────────────────────┘
```

**Why atomic matters — missed wakeup without it:**

```
Consumer: pthread_mutex_unlock(&mtx)     ← releases lock
Consumer: [PREEMPTED here]
    Producer: lock → avail++ → unlock → pthread_cond_signal()  ← LOST
Consumer: [RESUMED]
Consumer: pthread_cond_wait(...)         ← sleeps forever
```

The atomic pair closes this window completely.

#### Deadlock — four necessary conditions (Coffman)

All four must hold simultaneously:
1. **Mutual exclusion** — resource used by only one thread at a time
2. **Hold and wait** — thread holds resource while waiting for another
3. **No preemption** — resources cannot be forcibly taken
4. **Circular wait** — A waits for B, B waits for A (or longer cycle)

Break any one condition → deadlock impossible. Practical approach: break **circular wait** via **lock ordering**.

---

### Architecture

#### Mutex critical section flow

```
Thread A                            Thread B
────────────────────                ────────────────────
pthread_mutex_lock(&mtx)
  CAS: 0→TID_A ✓ → locked
  │
  critical section                  pthread_mutex_lock(&mtx)
  (access shared var)                 CAS fails → futex(WAIT)
  │                                   [BLOCKED]
pthread_mutex_unlock(&mtx)              │
  CAS: TID_A→0                         │
  futex(WAKE) ──────────────────────► unblocked
                                      CAS: 0→TID_B ✓ → locked
                                      │
                                      critical section
                                      │
                                    pthread_mutex_unlock(&mtx)
```

---

### Execution Flow

#### Producer-consumer with condition variable

```
Producer                            Consumer
────────────────────                ────────────────────
lock(mtx)                           lock(mtx)
avail++                             while (avail == 0)
unlock(mtx)                             cond_wait(cond, mtx)
cond_signal(cond)                         ↑ atomically: unlock+sleep
                                          ↓ on signal: relock
                                    avail--
                                    unlock(mtx)

State machine:
  avail == 0: consumer sleeps on cond
  avail > 0:  consumer runs; if producer waiting on not_full → wake it
```

#### Spurious wakeup — why `while` is mandatory

```
pthread_cond_broadcast() → ALL waiters wake
Waiter 1: recheck predicate → condition met → consumes → condition false again
Waiter 2: recheck predicate → condition FALSE → must sleep again

Without while loop:
  Waiter 2 skips check → operates on empty state → bug

With while loop:
  Waiter 2 rechecks → condition false → goes back to cond_wait → correct
```

Three reasons spurious/redundant wakeups happen:
1. `broadcast` wakes multiple waiters; only one can proceed
2. Loose predicate signals ("maybe something to do")
3. Hardware/OS spurious wakeup (permitted by POSIX on SMP systems)

---

### Core API

#### Mutex

```c
#include <pthread.h>

/* Static initialization (global/static vars only, default attrs) */
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

/* Dynamic initialization (heap, stack, non-default attrs) */
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
/* destroy: mutex must be unlocked; no threads waiting */

/* Lock / unlock */
int pthread_mutex_lock(pthread_mutex_t *mutex);     /* blocks if locked */
int pthread_mutex_trylock(pthread_mutex_t *mutex);  /* returns EBUSY if locked */
int pthread_mutex_unlock(pthread_mutex_t *mutex);
/* All return 0 on success, positive error number on failure */

/* Set mutex type */
pthread_mutexattr_t attr;
pthread_mutexattr_init(&attr);
pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
pthread_mutex_init(&mtx, &attr);
pthread_mutexattr_destroy(&attr);
```

#### Condition Variable

```c
#include <pthread.h>

/* Static initialization */
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

/* Dynamic initialization */
int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
int pthread_cond_destroy(pthread_cond_t *cond);
/* destroy: no threads waiting on it */

/* Wait — ALWAYS use while loop for predicate check */
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                           const struct timespec *abstime); /* returns ETIMEDOUT */

/* Signal */
int pthread_cond_signal(pthread_cond_t *cond);    /* wake ≥1 waiter */
int pthread_cond_broadcast(pthread_cond_t *cond); /* wake all waiters */
/* All return 0 on success, positive error number on failure */
/* Signal/broadcast does NOT need to hold the mutex (but often does) */
```

**Canonical wait pattern — never deviate:**

```c
pthread_mutex_lock(&mtx);
while (!condition_is_true())       /* WHILE, not if */
    pthread_cond_wait(&cond, &mtx);
/* shared state is now in desired state */
pthread_mutex_unlock(&mtx);
```

---

### Example (Code)

#### 1. Race condition demo — with and without mutex

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS    2
#define NUM_ITERATIONS 10000000

static long long counter_unsafe = 0;
static long long counter_safe   = 0;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

void *increment_unsafe(void *arg)
{
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        long long loc = counter_unsafe;  /* load  */
        loc++;                           /* modify — preemption window */
        counter_unsafe = loc;            /* store  */
    }
    return NULL;
}

void *increment_safe(void *arg)
{
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        pthread_mutex_lock(&mtx);
        counter_safe++;                  /* entire RMW under lock */
        pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

int main(void)
{
    pthread_t threads[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++)
        pthread_create(&threads[i], NULL, increment_unsafe, NULL);
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);
    printf("Unsafe: expected=%d  actual=%lld  %s\n",
           NUM_THREADS * NUM_ITERATIONS, counter_unsafe,
           counter_unsafe == NUM_THREADS * NUM_ITERATIONS ? "OK" : "RACE");

    for (int i = 0; i < NUM_THREADS; i++)
        pthread_create(&threads[i], NULL, increment_safe, NULL);
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);
    printf("Safe:   expected=%d  actual=%lld  %s\n",
           NUM_THREADS * NUM_ITERATIONS, counter_safe,
           counter_safe == NUM_THREADS * NUM_ITERATIONS ? "OK" : "BUG");

    pthread_mutex_destroy(&mtx);
    return 0;
}
/* Output:
 * Unsafe: expected=20000000  actual=13482531  RACE
 * Safe:   expected=20000000  actual=20000000  OK
 */
```

#### 2. Bounded producer-consumer with condition variables

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE  5
#define NUM_ITEMS    20

typedef struct {
    int             buf[BUFFER_SIZE];
    int             head, tail, count;
    pthread_mutex_t mtx;
    pthread_cond_t  not_full;    /* producer waits when buffer full  */
    pthread_cond_t  not_empty;   /* consumer waits when buffer empty */
} BoundedBuffer;

static BoundedBuffer bb;

void bb_init(BoundedBuffer *b)
{
    b->head = b->tail = b->count = 0;
    pthread_mutex_init(&b->mtx, NULL);
    pthread_cond_init(&b->not_full,  NULL);
    pthread_cond_init(&b->not_empty, NULL);
}

void bb_destroy(BoundedBuffer *b)
{
    pthread_mutex_destroy(&b->mtx);
    pthread_cond_destroy(&b->not_full);
    pthread_cond_destroy(&b->not_empty);
}

void bb_put(BoundedBuffer *b, int item)
{
    pthread_mutex_lock(&b->mtx);
    while (b->count == BUFFER_SIZE) {           /* predicate: full? */
        printf("Producer: buffer full, waiting\n");
        pthread_cond_wait(&b->not_full, &b->mtx);
    }
    b->buf[b->tail] = item;
    b->tail = (b->tail + 1) % BUFFER_SIZE;
    b->count++;
    printf("Producer: put %2d  (count=%d)\n", item, b->count);
    pthread_mutex_unlock(&b->mtx);
    pthread_cond_signal(&b->not_empty); /* signal AFTER unlock — avoids spurious contention */
}

int bb_get(BoundedBuffer *b)
{
    pthread_mutex_lock(&b->mtx);
    while (b->count == 0) {                     /* predicate: empty? */
        printf("Consumer: buffer empty, waiting\n");
        pthread_cond_wait(&b->not_empty, &b->mtx);
    }
    int item  = b->buf[b->head];
    b->head   = (b->head + 1) % BUFFER_SIZE;
    b->count--;
    printf("Consumer: got %2d  (count=%d)\n", item, b->count);
    pthread_mutex_unlock(&b->mtx);
    pthread_cond_signal(&b->not_full);
    return item;
}

void *producer(void *arg)
{
    for (int i = 1; i <= NUM_ITEMS; i++) {
        bb_put(&bb, i);
        usleep(100000);  /* 100 ms */
    }
    return NULL;
}

void *consumer(void *arg)
{
    for (int i = 0; i < NUM_ITEMS; i++) {
        bb_get(&bb);
        usleep(200000);  /* 200 ms — consumer slower than producer */
    }
    return NULL;
}

int main(void)
{
    pthread_t prod, cons;
    bb_init(&bb);
    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);
    bb_destroy(&bb);
    return 0;
}
```

#### 3. Deadlock demo — and two fixes

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

static pthread_mutex_t mutex_A = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mutex_B = PTHREAD_MUTEX_INITIALIZER;

/* ── DEADLOCK ─────────────────────────────────────────────── */
void *deadlock_t1(void *arg) {
    pthread_mutex_lock(&mutex_A);
    printf("T1: locked A\n");
    sleep(1);
    pthread_mutex_lock(&mutex_B);   /* blocks: T2 holds B */
    printf("T1: locked B  (never prints)\n");
    pthread_mutex_unlock(&mutex_B);
    pthread_mutex_unlock(&mutex_A);
    return NULL;
}
void *deadlock_t2(void *arg) {
    pthread_mutex_lock(&mutex_B);
    printf("T2: locked B\n");
    sleep(1);
    pthread_mutex_lock(&mutex_A);   /* blocks: T1 holds A */
    printf("T2: locked A  (never prints)\n");
    pthread_mutex_unlock(&mutex_A);
    pthread_mutex_unlock(&mutex_B);
    return NULL;
}

/* ── FIX 1: consistent lock ordering ─────────────────────── */
void *safe_order_t1(void *arg) {
    pthread_mutex_lock(&mutex_A);   /* always A before B */
    pthread_mutex_lock(&mutex_B);
    printf("T1: has A and B\n");
    pthread_mutex_unlock(&mutex_B);
    pthread_mutex_unlock(&mutex_A);
    return NULL;
}
void *safe_order_t2(void *arg) {
    pthread_mutex_lock(&mutex_A);   /* same order */
    pthread_mutex_lock(&mutex_B);
    printf("T2: has A and B\n");
    pthread_mutex_unlock(&mutex_B);
    pthread_mutex_unlock(&mutex_A);
    return NULL;
}

/* ── FIX 2: try-and-backoff ───────────────────────────────── */
void *trylock_thread(void *arg) {
    int id = *(int *)arg;
    pthread_mutex_t *first  = (id == 1) ? &mutex_A : &mutex_B;
    pthread_mutex_t *second = (id == 1) ? &mutex_B : &mutex_A;
    while (1) {
        pthread_mutex_lock(first);
        if (pthread_mutex_trylock(second) == 0) {
            printf("Thread %d: has both locks\n", id);
            pthread_mutex_unlock(second);
            pthread_mutex_unlock(first);
            return NULL;
        }
        pthread_mutex_unlock(first);  /* release all, retry */
        usleep(1000);                 /* backoff — avoids livelock */
    }
}

int main(void)
{
    pthread_t t1, t2;
    int id1 = 1, id2 = 2;

    printf("=== Lock ordering fix ===\n");
    pthread_create(&t1, NULL, safe_order_t1, NULL);
    pthread_create(&t2, NULL, safe_order_t2, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("=== Try-and-backoff fix ===\n");
    pthread_create(&t1, NULL, trylock_thread, &id1);
    pthread_create(&t2, NULL, trylock_thread, &id2);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    pthread_mutex_destroy(&mutex_A);
    pthread_mutex_destroy(&mutex_B);
    return 0;
}
```

#### 4. Lost wakeup — bug without predicate, fix with predicate

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

static pthread_mutex_t mtx  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;
static int ready = 0;   /* THE predicate — carries state */

/* ── BUG: no predicate, signal fires before anyone waits ─── */
void *buggy_producer(void *arg) {
    sleep(1);
    printf("BUG producer: signaling (no waiter yet)\n");
    pthread_cond_signal(&cond);   /* signal LOST — CV holds no state */
    return NULL;
}
void *buggy_consumer(void *arg) {
    sleep(2);                     /* arrives after signal is gone */
    pthread_mutex_lock(&mtx);
    printf("BUG consumer: waiting (will block forever)\n");
    pthread_cond_wait(&cond, &mtx);   /* never returns */
    pthread_mutex_unlock(&mtx);
    return NULL;
}

/* ── FIX: predicate survives lost signal ─────────────────── */
void *safe_producer(void *arg) {
    sleep(1);
    pthread_mutex_lock(&mtx);
    ready = 1;                    /* set STATE before signaling */
    pthread_mutex_unlock(&mtx);
    printf("SAFE producer: ready=1, signaling\n");
    pthread_cond_signal(&cond);   /* signal may be lost — irrelevant */
    return NULL;
}
void *safe_consumer(void *arg) {
    sleep(2);
    pthread_mutex_lock(&mtx);
    while (ready == 0) {          /* check STATE, not signal count */
        printf("SAFE consumer: ready=0, waiting\n");
        pthread_cond_wait(&cond, &mtx);
    }
    /* if signal was already lost: ready==1 → while skipped → no block */
    printf("SAFE consumer: ready=%d, proceeding\n", ready);
    pthread_mutex_unlock(&mtx);
    return NULL;
}

int main(void)
{
    pthread_t prod, cons;
    printf("=== Safe version (with predicate) ===\n");
    pthread_create(&prod, NULL, safe_producer, NULL);
    pthread_create(&cons, NULL, safe_consumer, NULL);
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);
    pthread_mutex_destroy(&mtx);
    pthread_cond_destroy(&cond);
    return 0;
}
```

#### 5. Dynamic init — mutex embedded in heap-allocated struct

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct Node {
    int              value;
    pthread_mutex_t  lock;     /* cannot use PTHREAD_MUTEX_INITIALIZER here */
    struct Node     *next;
} Node;

Node *node_create(int value)
{
    Node *n = malloc(sizeof(*n));
    if (!n) return NULL;
    n->value = value;
    n->next  = NULL;
    if (pthread_mutex_init(&n->lock, NULL) != 0) { free(n); return NULL; }
    return n;
}

void node_destroy(Node *n)
{
    pthread_mutex_destroy(&n->lock);   /* destroy BEFORE free */
    free(n);
}

void node_set(Node *n, int v)
{
    pthread_mutex_lock(&n->lock);
    n->value = v;
    pthread_mutex_unlock(&n->lock);
}

int node_get(Node *n)
{
    pthread_mutex_lock(&n->lock);
    int v = n->value;
    pthread_mutex_unlock(&n->lock);
    return v;
}

typedef struct { Node *node; int start; } WorkArg;

void *updater(void *arg)
{
    WorkArg *w = (WorkArg *)arg;
    for (int i = 0; i < 1000; i++)
        node_set(w->node, w->start + i);
    return NULL;
}

int main(void)
{
    Node *n = node_create(0);
    pthread_t t1, t2;
    WorkArg a1 = {n, 1000}, a2 = {n, 5000};
    pthread_create(&t1, NULL, updater, &a1);
    pthread_create(&t2, NULL, updater, &a2);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    printf("Final value: %d\n", node_get(n));
    node_destroy(n);
    return 0;
}
```

---

### Debugging

```bash
# ThreadSanitizer — catches races and lock-order violations at runtime
gcc -fsanitize=thread -g -pthread source.c -o binary && ./binary

# Valgrind Helgrind — detects pthread API misuse, races, lock ordering
valgrind --tool=helgrind ./binary

# Valgrind DRD — lighter alternative to Helgrind
valgrind --tool=drd ./binary

# Detect deadlock: process stuck → check all thread stacks
gdb ./binary <pid>
(gdb) thread apply all bt   # look for pthread_mutex_lock in every thread
                              # if all blocked on lock → deadlock

# Check lock contention at scale
perf lock record ./binary && perf lock report
```

**Common error messages and causes:**

```
ThreadSanitizer: data race on ... at ...
→ Shared variable accessed without mutex from two threads

ThreadSanitizer: lock-order-inversion (potential deadlock)
→ Thread A locks M1→M2; Thread B locks M2→M1 → enforce global ordering

Valgrind Helgrind: Mutex is still locked at end of thread
→ Thread exits while holding lock → all other waiters hang forever

Program hangs, all threads in pthread_mutex_lock
→ Deadlock: circular lock dependency
→ Draw a lock dependency graph; find the cycle

pthread_mutex_destroy on a locked mutex
→ Undefined behavior; typically corrupts futex state
→ Ensure all threads release locks before destroy
```

---

### Real-world Usage

| Pattern | Mechanism |
|---|---|
| **Counter / stats aggregation** | Per-thread counters (no sync needed), merge under mutex at report time |
| **Thread-safe queue (work queue)** | Mutex + two CVs (not_full, not_empty); backbone of every thread pool |
| **Read-write lock pattern** | `pthread_rwlock_t` — multiple concurrent readers, exclusive writer; from `<pthread.h>` |
| **One-time initialization** | `pthread_once(&once_ctrl, init_fn)` — guaranteed single execution across threads |
| **Broadcast-based state machine** | One mutex + one CV + enum; broadcast on every state change; each thread checks its predicate |

---

### Key Takeaways

1. **A C increment (`x++`) is NOT atomic** — it compiles to load/modify/store; any preemption between these steps causes a lost update.

2. **Mutex = futex in userspace** — uncontended lock/unlock costs ~10 ns with no syscall; syscall overhead only on contention.

3. **CV + mutex = inseparable pair** — one CV must always be used with the same mutex; mixing causes undefined behavior per POSIX.

4. **`pthread_cond_wait` is atomic unlock + sleep** — this eliminates the missed-wakeup race between checking the predicate and blocking.

5. **Always use `while`, never `if` for predicate** — spurious wakeups are permitted by POSIX; multiple waiters mean the condition may be false when you wake.

6. **Signal without predicate = lost wakeup** — CV carries no state; if no thread is waiting when signal fires, the signal is gone. The predicate (shared variable) is the real state; CV is just the notification.

7. **Deadlock requires circular wait** — break it with a global lock ordering enforced consistently across all code paths.

8. **Signal after unlock is usually better** — avoids the newly-woken thread immediately blocking again on the still-held mutex (spurious lock contention).
