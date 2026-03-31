# Chapter 6 — Thread Cancellation
## Topics: 6.4 Thread Cancellation
> Sources: TLPI Ch32 | DevLinux Module 05

---

## 6.4 Thread Cancellation

### Problem It Solves

A thread blocked in a slow network `read()`, a compute thread running indefinitely when the user clicks Cancel — these must be stopped from outside. Naive approaches fail:

- `pthread_kill(tid, SIGKILL)` → kills the **entire process**, not just the thread
- `volatile int stop = 1` → thread blocked in `read()` never checks the flag

`pthread_cancel()` provides **cooperative cancellation**: the requesting thread sends a request; the target thread acts on it only at well-defined safe points, and registers cleanup handlers to release resources before dying.

---

### Concept Overview

Three orthogonal controls govern cancellation behavior:

```
pthread_cancel(tid)
      │
      ▼
  [PENDING REQUEST]
      │
      ├─ State = DISABLE  →  stays pending silently
      │
      └─ State = ENABLE (default)
              │
              ├─ Type = DEFERRED (default)
              │    fires at next cancellation point:
              │    sleep(), read(), write(), pthread_cond_wait()...
              │
              └─ Type = ASYNCHRONOUS
                   fires at ANY machine instruction (almost always wrong)
```

| Attribute | API | Default | Behavior |
|---|---|---|---|
| **State** | `pthread_setcancelstate()` | `PTHREAD_CANCEL_ENABLE` | DISABLE: request stays pending |
| **Type** | `pthread_setcanceltype()` | `PTHREAD_CANCEL_DEFERRED` | ASYNC: cancel at any instruction |
| **Handlers** | `pthread_cleanup_push/pop()` | — | LIFO stack runs before thread dies |

---

### System Context

- **What it solves:** Controlled, resource-safe shutdown of threads blocked in I/O or stuck in compute loops, without leaving mutexes locked or heap memory leaked.
- **Where in the system:** `pthread_cancel()` sends NPTL-internal signal `SIGCANCEL` (usually `SIGRTMIN+2`) to the target thread. On Linux/glibc, every cancellation-point function wraps its underlying syscall with a cancellation flag check inside the glibc wrapper.
- **Subsystem interactions:** Cleanup handlers form a linked list on the **thread's stack** — `pthread_cleanup_push/pop` are macros expanding to `{` / `}` blocks that manage this list. The cancellation check at each cancellation point inspects a per-thread flag set by `pthread_cancel()`.
- **Failure scenarios:** `PTHREAD_CANCEL_DISABLE` while holding locks without a clear exit path → thread never exits, program hangs at shutdown. Async cancellation + `malloc()` → heap corruption (`malloc` holds internal arena lock at cancellation moment). `pthread_cancel()` on an already-joined TID → undefined behavior.

---

### Internal Mechanism

#### Cancellation Check at Cancellation Points

```
Per-thread cancel state (in TCB):
  cancelstate:  ENABLE | DISABLE
  canceltype:   DEFERRED | ASYNCHRONOUS
  pending_flag: 0 | 1

pthread_cancel(tid):
  → sends SIGCANCEL to target thread
  → sets target's pending_flag = 1
  → if target blocked in cancellation-point syscall → syscall interrupted

glibc wrapper for read() (simplified):
  int read(int fd, void *buf, size_t n) {
      if (cancelstate == ENABLE && pending_flag && canceltype == DEFERRED)
          __cancel_thread();   /* run cleanup stack, then pthread_exit */
      return syscall(SYS_read, fd, buf, n);
  }
```

#### Why Async Cancellation is Dangerous

```c
/* Seemingly simple — UNSAFE with PTHREAD_CANCEL_ASYNCHRONOUS */
void *worker(void *arg) {
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    void *buf = malloc(1024);
    /* Cancel can arrive HERE: malloc returned but buf not yet assigned
     * → memory allocated, nobody has pointer → instant leak
     * → malloc's arena mutex may be held → heap permanently corrupt */

    strcpy(buf, "data");
    /* Cancel mid-strcpy → no null-terminator → silent buffer corruption */

    free(buf);
    return NULL;
}
```

SUSv3 guarantees async-cancel-safe for only 3 functions: `pthread_cancel()`, `pthread_setcancelstate()`, `pthread_setcanceltype()`. Nothing else — not `printf`, not `malloc`, not any Pthreads API.

#### Cleanup Handler Stack — LIFO Design

```
Thread acquires resources in order A → B → C:
  pthread_cleanup_push(release_A, a)    ← pushed first (bottom)
  pthread_cleanup_push(release_B, b)
  pthread_cleanup_push(release_C, c)    ← pushed last  (top)

Cancel fires → LIFO execution:
  release_C(c)   ← last in, first out
  release_B(b)
  release_A(a)
  thread terminates → retval = PTHREAD_CANCELED

Design: mirrors RAII destructor ordering
  acquire A→B→C, cleanup C→B→A = correct reverse teardown
```

#### `return` Inside push/pop Pair — Silent Bug

```c
pthread_cleanup_push(cleanup, buf);   /* macro expands to: { struct frame...; push(&frame); */

    if (error) return -1;             /* jumps OUT of the { } block */
                                      /* pop() never called → handler NOT called → LEAK */

pthread_cleanup_pop(0);               /* macro expands to: pop(&frame); } */
```

`return` skips the `pop` macro's `}` → handler silently ignored. **The only safe exit paths are `pthread_exit()` or letting code flow naturally to `pthread_cleanup_pop()`**.

#### DISABLE + join: Not Deadlock, but Blocking

```
Thread A: pthread_cancel(B)    → pending flag set, A returns immediately
Thread A: pthread_join(B)      → A BLOCKS, waiting for B to terminate

Thread B: cancelstate = DISABLE → pending flag set but ignored
Thread B: ... does work ...
Thread B: setcancelstate(ENABLE, &old) → pending fires NOW
Thread B: cleanup stack runs → B terminates with PTHREAD_CANCELED
Thread A: join unblocks → retval = PTHREAD_CANCELED ✓

BUT: if B never re-enables AND never returns → A blocks forever
```

---

### Architecture

```
Cancellation — 3-layer architecture:

Layer 1: REQUEST
  pthread_cancel(tid)
    → SIGCANCEL → target TCB: pending_flag = 1
    → returns immediately (async fire-and-forget)

Layer 2: GATE (State × Type)
  ┌──────────────────────────────────────────────────────┐
  │  State = DISABLE  │  pending accumulates, no action   │
  ├──────────────────────────────────────────────────────┤
  │  ENABLE + DEFERRED│  fires at cancellation point only │ ← recommended
  ├──────────────────────────────────────────────────────┤
  │  ENABLE + ASYNC   │  fires at any instruction         │ ← almost never safe
  └──────────────────────────────────────────────────────┘

Layer 3: CLEANUP STACK (LIFO)
  push(handler_N)           ← last registered   ┐
  push(handler_N-1)                              │  LIFO
  push(handler_1)           ← first registered  ↓
                                                 ↓
  Cancel fires:                                  ↓
    handler_N   (last pushed → first called)    ─┘
    handler_N-1
    handler_1
    pthread_exit(PTHREAD_CANCELED)
```

---

### Execution Flow

```
Timeline — cancel request to thread death:

Thread A                            Thread B (target)
─────────────────                   ──────────────────────────────────
pthread_cancel(tid_B)
  sends SIGCANCEL → B
  sets B.pending_flag = 1
  returns immediately (non-blocking)
                                    B continues executing...
pthread_join(tid_B) ──────────────→ pthread_mutex_lock(&mtx)   ← safe
  [BLOCKS]                          doing work...
                                    pthread_mutex_unlock(&mtx)  ← safe
                                    sleep(2)  ←── CANCELLATION POINT
                                      glibc wrapper checks pending_flag
                                      cancelstate == ENABLE
                                      canceltype  == DEFERRED
                                      → run cleanup stack:
                                        handler_2(arg2)  (last pushed, first)
                                        handler_1(arg1)
                                      → pthread_exit(PTHREAD_CANCELED)
  [UNBLOCKS] ←────────────────────── B terminated
retval = PTHREAD_CANCELED
```

---

### Core API

#### Cancellation Request

```c
#include <pthread.h>

int pthread_cancel(pthread_t thread);
/* Sends cancellation request — returns immediately (does NOT wait for thread to die) */
/* Returns 0 on success, positive error number on error */
/* Follow with pthread_join() to wait for termination and check result */
```

#### Cancellation State and Type

```c
int pthread_setcancelstate(int state, int *oldstate);
/* state:    PTHREAD_CANCEL_ENABLE (default) | PTHREAD_CANCEL_DISABLE */
/* oldstate: receives previous state — always specify non-NULL for portability */
/* Returns 0 on success */

int pthread_setcanceltype(int type, int *oldtype);
/* type:    PTHREAD_CANCEL_DEFERRED (default) | PTHREAD_CANCEL_ASYNCHRONOUS */
/* oldtype: receives previous type — always specify non-NULL */
/* Returns 0 on success */
```

#### Manual Cancellation Point

```c
void pthread_testcancel(void);
/* Acts as a cancellation point: if pending + enabled → fires immediately */
/* Otherwise: no-op */
/* Insert in compute-bound loops that have no natural cancellation points */
```

#### Cleanup Handlers

```c
void pthread_cleanup_push(void (*routine)(void *), void *arg);
/* Push handler onto this thread's cleanup stack */
/* Fires on: cancel, pthread_exit(), or cleanup_pop(1) */
/* May expand to '{' — MUST be balanced with cleanup_pop in same lexical scope */

void pthread_cleanup_pop(int execute);
/* Pop top handler; execute != 0 → call handler before removing */
/* May expand to '}' — MUST match corresponding push */
```

**Handler signature:**
```c
void my_cleanup(void *arg) {
    free(arg);                           /* release heap allocation */
    pthread_mutex_unlock(&some_mutex);   /* release held lock */
}
```

#### Check Cancellation via `pthread_join`

```c
void *retval;
pthread_join(target_tid, &retval);
if (retval == PTHREAD_CANCELED)   /* sentinel: ((void *)-1) */
    printf("thread was canceled\n");
```

---

### Example (Code)

#### Example 1 — Cleanup handlers: LIFO order (mutex + heap)

```c
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static pthread_mutex_t mtx  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;
static int glob = 0;

static void free_buf(void *arg) {
    printf("[cleanup] free(%p)\n", arg);
    free(arg);
}

static void unlock_mtx(void *arg) {
    printf("[cleanup] unlock mutex\n");
    pthread_mutex_unlock((pthread_mutex_t *)arg);
}

static void *worker(void *arg) {
    void *buf = malloc(256);
    if (!buf) return NULL;

    /* Push BEFORE acquiring mutex: ensures handlers cover full cancel window */
    pthread_cleanup_push(free_buf, buf);          /* pushed first (bottom) */

    pthread_mutex_lock(&mtx);
    pthread_cleanup_push(unlock_mtx, &mtx);       /* pushed second (top) */
    /* LIFO: cancel → unlock_mtx first, then free_buf — correct reverse order */

    while (glob == 0)
        pthread_cond_wait(&cond, &mtx);           /* cancellation point */

    printf("[worker] condition met, exiting normally\n");
    pthread_cleanup_pop(1);   /* unlock */
    pthread_cleanup_pop(1);   /* free */
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t thr;
    void *res;

    pthread_create(&thr, NULL, worker, NULL);
    sleep(1);

    if (argc > 1) {
        pthread_mutex_lock(&mtx);
        glob = 1;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mtx);
    } else {
        printf("[main] canceling thread\n");
        pthread_cancel(thr);
    }

    pthread_join(thr, &res);
    printf("[main] thread %s\n",
           res == PTHREAD_CANCELED ? "CANCELED" : "exited normally");
    return 0;
}
/* gcc -pthread ex1_cancel_cleanup.c -o ex1
   ./ex1         → cancel path: LIFO cleanup (unlock then free)
   ./ex1 sig     → normal path: cleanup_pop(1) runs in order */
```

---

#### Example 2 — `pthread_testcancel` in compute loop + DISABLE for critical section

```c
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

/* Compute-bound: no natural cancellation points */
static void *compute_worker(void *arg) {
    uint64_t sum = 0, iter = 0;
    while (1) {
        for (int i = 0; i < 1000000; i++)
            sum += i;
        iter++;

        pthread_testcancel();   /* manual check every 1M iterations */

        if (iter % 10 == 0)
            printf("[compute] iter=%lu\n", iter);
    }
    return NULL;
}

/* Critical section that MUST NOT be interrupted mid-way */
static void *tx_worker(void *arg) {
    int old_state;

    printf("[tx] working, cancellable\n");
    sleep(1);   /* cancel can fire here (cancellation point) */

    /* DISABLE: protect transaction from cancellation */
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &old_state);
    printf("[tx] BEGIN TRANSACTION\n");
    sleep(2);   /* sleep(): normally a cancel point, but DISABLE = safe */
    printf("[tx] COMMIT TRANSACTION\n");

    /* Re-enable: if cancel arrived while DISABLE → fires NOW */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &old_state);
    pthread_testcancel();   /* explicit check: fires immediately if pending */

    printf("[tx] after transaction (only prints if no pending cancel)\n");
    return NULL;
}

int main(void) {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, compute_worker, NULL);
    pthread_create(&t2, NULL, tx_worker, NULL);

    sleep(1);
    printf("[main] sending cancel to both\n");
    pthread_cancel(t1);   /* fires at next testcancel() */
    pthread_cancel(t2);   /* pending during DISABLE window, fires after COMMIT */

    void *r1, *r2;
    pthread_join(t1, &r1);
    pthread_join(t2, &r2);
    printf("[main] compute: %s\n", r1 == PTHREAD_CANCELED ? "CANCELED" : "normal");
    printf("[main] tx:      %s\n", r2 == PTHREAD_CANCELED ? "CANCELED" : "normal");
    return 0;
}
/* gcc -pthread ex2_testcancel.c -o ex2 */
```

---

#### Example 3 — Thread pool graceful shutdown via cancellation

```c
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define POOL_SIZE 4

static sem_t work_sem;

static void restore_sem(void *arg) {
    /* Cleanup: if canceled while sem_wait() consumed the token, put it back */
    printf("[cleanup] worker restoring semaphore\n");
    sem_post((sem_t *)arg);
}

static void *pool_worker(void *arg) {
    int id = *(int *)arg;
    while (1) {
        printf("[worker %d] waiting for work\n", id);

        /* Push handler BEFORE sem_wait so cancel during wait is handled */
        pthread_cleanup_push(restore_sem, &work_sem);
        sem_wait(&work_sem);                   /* cancellation point */
        pthread_cleanup_pop(0);                /* execute=0: work obtained, no restore */

        printf("[worker %d] processing\n", id);
        sleep(1);
    }
    return NULL;
}

int main(void) {
    pthread_t tids[POOL_SIZE];
    int ids[POOL_SIZE];

    sem_init(&work_sem, 0, 0);
    for (int i = 0; i < POOL_SIZE; i++) {
        ids[i] = i;
        pthread_create(&tids[i], NULL, pool_worker, &ids[i]);
    }

    /* Submit work */
    sleep(1);
    for (int i = 0; i < 3; i++)
        sem_post(&work_sem);
    sleep(2);

    /* Graceful shutdown: cancel all blocked workers */
    printf("[main] shutting down pool\n");
    for (int i = 0; i < POOL_SIZE; i++)
        pthread_cancel(tids[i]);

    for (int i = 0; i < POOL_SIZE; i++) {
        void *res;
        pthread_join(tids[i], &res);
        printf("[main] worker %d: %s\n", i,
               res == PTHREAD_CANCELED ? "CANCELED" : "normal");
    }

    sem_destroy(&work_sem);
    return 0;
}
/* gcc -pthread ex3_pool_shutdown.c -o ex3 */
```

---

### Debugging

```bash
# GDB: break inside cleanup handler to verify it fires on cancel
gdb ./ex1
(gdb) break free_buf
(gdb) break unlock_mtx
(gdb) run                    # in cancel path: both breakpoints should hit

# TSan: detects some cancel-related misuse patterns
gcc -pthread -fsanitize=thread -g ex1.c -o ex1_tsan && ./ex1_tsan

# Helgrind: detects mutex held at thread death (forgot cleanup handler)
valgrind --tool=helgrind ./ex1

# NPTL's internal SIGCANCEL
python3 -c "import signal; print(signal.SIGRTMIN + 2)"   # usually 34
```

**Common mistakes:**

| Bug | Symptom | Fix |
|---|---|---|
| `return` inside push/pop pair | Cleanup handler silently skipped → resource leak | Use `pthread_exit()` or code flow to `pop` |
| Push handler AFTER acquiring resource | Cancel between acquire and push → handler not yet registered | Push handler BEFORE the cancellation point |
| Async cancel + malloc/printf | Heap corruption, stdio deadlock | Use `DEFERRED` only; never `ASYNC` with resources |
| DISABLE, never re-enable | Thread ignores future cancels → hangs at shutdown | Always restore old_state; call `testcancel` after re-enable |
| No `testcancel` in compute loop | Thread ignores cancel indefinitely | Insert `pthread_testcancel()` periodically |

---

### Real-world Usage

- **Thread pool shutdown:** Workers blocked in `sem_wait()` — `pthread_cancel()` on each; cleanup handler releases semaphore and frees current work item.
- **HTTP server request abort:** Client disconnects mid-transfer → cancel handler thread → cleanup closes fd, releases response buffer.
- **Database query timeout:** Watchdog cancels query thread after N seconds; cleanup handlers rollback transaction, release statement handle.
- **GUI background task cancel:** User clicks Cancel → main thread cancels background worker; cleanup restores UI to idle state.

---

### Key Takeaways

| Concept | Rule |
|---|---|
| `pthread_cancel` | Sends request, returns immediately — does NOT wait for thread to die |
| Deferred (default) | Only fires at cancellation points — safe and predictable |
| Asynchronous | Almost never correct — only pure arithmetic with zero resources held |
| Push order matters | Push handler BEFORE acquiring the resource it protects |
| LIFO execution | Last pushed = first called — mirrors reverse acquisition order |
| `return` in push/pop | Handler NOT called — only `pthread_exit()` or natural flow to `pop` works |
| DISABLE + testcancel | Protect critical sections; always restore old state and check after re-enable |
| Joined canceled thread | `retval == PTHREAD_CANCELED` — use to distinguish cancel from normal exit |
