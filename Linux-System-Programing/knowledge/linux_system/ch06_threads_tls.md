# Chapter 6 — Thread Safety & TLS
## Topics: 6.3 Thread Safety & TLS
> Sources: TLPI Ch31 | DevLinux Module 05

---

## 6.3 Thread Safety & TLS

### Problem It Solves

Many classic C library functions use **static internal buffers** shared across all callers — `strtok()`, `strerror()`, `gmtime()`, `localtime()`, `crypt()`. In a single-threaded world they work fine. In a multithreaded program, two threads calling `strtok()` concurrently silently corrupt each other's tokenization state.

Two tools solve per-thread private data:

1. **Thread-Specific Data (TSD)** — per-thread private storage slot, identified by a global key, with optional destructor
2. **Thread-Local Storage (TLS) (`__thread`)** — compiler/linker provide a per-thread copy of a variable automatically, with zero runtime overhead

Additionally, library initialization must happen **exactly once** even when many threads race for first use. `pthread_once()` solves this.

---

### Concept Overview

A function is classified at one of three levels of thread safety:

| Level | Description | Mechanism | Example |
|---|---|---|---|
| **Serialized** | Global mutex wraps the whole function | Lock/unlock around entire function body | Wrapping legacy `strtok()` with a global lock |
| **Critical Section** | Mutex protects only the shared state | Lock only accesses to shared data | `malloc()` internals — arena lock |
| **Reentrant** | No shared mutable state; uses only stack or caller-provided buffers | Caller passes buffer, or `__thread` vars | `strtok_r()`, `gmtime_r()`, `strerror_r()` |

Reentrant is the strongest guarantee — works correctly even when interrupted by a signal and called again from the signal handler.

---

### System Context

- **What it solves:** Enables legacy and new library code to operate safely from multiple threads concurrently, without global serialization that would kill throughput.
- **Where in the system:** TSD implemented in glibc/NPTL — `pthread_key_create` registers a global slot; each thread's TCB holds a per-thread pointer array indexed by key. TLS implemented by ELF linker + kernel: `__thread` variables in `.tdata`/`.tbss` ELF sections; the FS register points to the per-thread TLS block.
- **Subsystem interactions:** `pthread_once` uses a futex internally to serialize concurrent initializations. TSD destructors are called by `pthread_exit()` via NPTL cleanup code. `fork()` copies only the calling thread's TLS block — all other threads' TLS data vanishes in the child.
- **Failure scenarios:** Race-prone lazy init of TSD key (without `pthread_once`) crashes under load. A TSD destructor that re-sets the key triggers another destructor round — NPTL stops after `PTHREAD_DESTRUCTOR_ITERATIONS` (4) rounds, leaking remaining data. Calling `longjmp()` from inside `pthread_once`'s init function leaves `once_control` stuck IN_PROGRESS permanently — deadlock for all future callers.

---

### Internal Mechanism

#### `pthread_once` — One-Time Initialization

`pthread_once_t` is a small integer with three states:

```
INIT (0) → IN_PROGRESS (1) → DONE (2)

Thread A sees INIT → CAS INIT→IN_PROGRESS → calls init_routine() → sets DONE
Thread B sees IN_PROGRESS → futex_wait() (sleeps until DONE)
Thread C sees DONE → returns immediately (fast path, no syscall)
```

**Critical danger:** If `init_routine()` exits via `longjmp()` or C++ exception, `once_control` is stuck at IN_PROGRESS forever. Every future caller will `futex_wait()` indefinitely — **permanent deadlock**. `init_routine` must always return normally.

---

#### Thread-Specific Data (TSD)

TSD provides a per-thread private storage slot identified by a global `pthread_key_t`.

```
glibc global array: pthread_keys[]
  Slot 0: { in_use=1, destructor=free }
  Slot 1: { in_use=1, destructor=my_destructor }
  Slot 2: { in_use=0, destructor=NULL }
  ... up to PTHREAD_KEYS_MAX (1024 on Linux)

Per-thread TCB:
  Thread 1 TSD array: [0x1000, NULL, 0x3000, ...]
                         ↑ key 0   ↑ key 1
  Thread 2 TSD array: [0x2000, NULL, 0x4000, ...]
                         ↑ key 0   ↑ key 1
```

`pthread_getspecific(key)` is O(1): `thread_tcb->tsd[key]`.

**Destructor chain:** When a thread exits, NPTL iterates all keys with destructors and non-NULL values. If a destructor re-sets the key to non-NULL, NPTL runs another round. Maximum rounds = `PTHREAD_DESTRUCTOR_ITERATIONS` = **4**. After round 4, remaining non-NULL values are abandoned (leaked).

---

#### Thread-Local Storage (TLS) — `__thread` / `_Thread_local`

```c
__thread int tls_counter;         /* GCC/Clang extension */
_Thread_local int tls_counter;   /* C11 standard keyword */
```

How it works:
- **Link time:** `__thread` variables placed in `.tdata` (initialized) or `.tbss` (zero-init) ELF sections — templates
- **Thread creation:** `clone()` with `CLONE_SETTLS` flag → kernel sets **FS segment register** (x86-64) to the new thread's TLS block address
- **Access time:** `mov %fs:offset, %reg` — direct segment-relative access, **no function call, no lock, no dynamic lookup**
- **Performance:** Single instruction, ~10 ns less overhead than TSD per access

```
Thread 1 TCB              Thread 2 TCB
FS → [ counter = 0  ]     FS → [ counter = 0  ]
       ↑ independent              ↑ independent
```

| Feature | TSD | `__thread` |
|---|---|---|
| API | `get/setspecific()` calls | Direct variable access |
| Overhead | ~2 function calls per access | Single instruction |
| Destructor support | Yes | No |
| Dynamic allocation | Yes (pointer to heap) | No (inline in TLS block) |
| Runtime key creation | Yes | No (compile-time only) |

---

#### Reentrant `_r` Functions

| Unsafe | Reentrant | Change |
|---|---|---|
| `strtok(str, delim)` | `strtok_r(str, delim, &saveptr)` | Caller provides `saveptr` |
| `gmtime(timep)` | `gmtime_r(timep, &result)` | Caller provides `struct tm` |
| `localtime(timep)` | `localtime_r(timep, &result)` | Caller provides `struct tm` |
| `strerror(errno)` | `strerror_r(errno, buf, buflen)` | Caller provides buffer |
| `crypt(key, salt)` | `crypt_r(key, salt, &data)` | Caller provides `struct crypt_data` |

---

### Architecture

```
Three layers of per-thread data storage — from compile-time to runtime:

Layer 1: __thread / TLS (compile-time, zero overhead)
  ┌─────────────────────────────────────────────────────────┐
  │  ELF .tdata / .tbss  (template sections)                │
  │      ↓ (at thread creation via CLONE_SETTLS)            │
  │  Thread 1 TLS block  ←── FS register                   │
  │  Thread 2 TLS block  ←── FS register                   │
  │  Direct access: mov %fs:offset, %reg  (1 instruction)  │
  └─────────────────────────────────────────────────────────┘

Layer 2: TSD (runtime, key-indexed, with destructor)
  ┌─────────────────────────────────────────────────────────┐
  │  pthread_keys[1024]  (global, process-wide)             │
  │    key 0: { in_use, destructor=free }                   │
  │    key 1: { in_use, destructor=my_dtor }                │
  │      ↕ indexed by key                                   │
  │  Thread 1 tsd[]:  [ptr_A, ptr_B, NULL, ...]            │
  │  Thread 2 tsd[]:  [ptr_C, ptr_D, NULL, ...]            │
  │  getspecific(key) = tsd[key]  — O(1) array lookup      │
  └─────────────────────────────────────────────────────────┘

Layer 3: pthread_once (one-time init, state machine)
  ┌─────────────────────────────────────────────────────────┐
  │  once_control: INIT(0) → IN_PROGRESS(1) → DONE(2)      │
  │  Futex: contending threads sleep while IN_PROGRESS      │
  │  Fast path: DONE → return immediately (no syscall)      │
  └─────────────────────────────────────────────────────────┘
```

---

### Execution Flow

#### TSD destructor chain at thread exit

```
Thread calls pthread_exit() or returns from start function
         │
         ▼
NPTL cleanup: iterate pthread_keys[] — Round 1
  For each key k where tsd[k] != NULL AND destructor[k] != NULL:
    val    = tsd[k]
    tsd[k] = NULL              ← clear BEFORE calling (re-entrancy safe)
    destructor[k](val)         ← destructor MAY re-set tsd[k] to non-NULL
         │
         ▼
Any tsd[k] non-NULL after Round 1?
  YES → Round 2 (same algorithm)
  ...
  Max PTHREAD_DESTRUCTOR_ITERATIONS = 4 rounds on Linux
         │
         ▼
After 4 rounds: stop — any remaining non-NULL values LEAKED (no more destructor calls)
         │
         ▼
pthread_exit cleanup handlers run → thread terminates
```

**Implication:** A destructor that always re-sets its key will cause data to leak after round 4. Design destructors to set the key to NULL without re-registering.

---

### Core API

#### One-Time Initialization

```c
#include <pthread.h>

pthread_once_t once_control = PTHREAD_ONCE_INIT;

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));
/* Calls init_routine exactly once, regardless of how many threads call pthread_once */
/* Returns 0 on success */
/* CRITICAL: init_routine MUST always return normally —
 *           longjmp() or exception leaves once_control stuck at IN_PROGRESS → deadlock */
```

#### Thread-Specific Data

```c
#include <pthread.h>

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
/* destructor: called at thread exit with thread's value if non-NULL; NULL = no destructor */
/* Returns 0 on success; EAGAIN if PTHREAD_KEYS_MAX (1024) exhausted */

void *pthread_getspecific(pthread_key_t key);
/* Returns this thread's value for key; NULL if never set */

int pthread_setspecific(pthread_key_t key, const void *value);
/* Binds value to key for the calling thread only */
/* Returns 0 on success, ENOMEM or EINVAL on error */

int pthread_key_delete(pthread_key_t key);
/* Unregisters key globally; does NOT call destructors */
/* Returns 0 on success */
```

#### Thread-Local Storage

```c
/* No #include needed — compiler keyword */
__thread type varname;              /* GCC/Clang extension */
_Thread_local type varname;        /* C11 standard keyword */

__thread int per_thread_counter = 0;    /* per-thread int, zero-initialized */
static __thread char buf[256];          /* per-thread static buffer */
```

**Constraints:** Must be file-scope (global) or `static` — not automatic (stack). No constructor or destructor support.

#### Reentrant Variants

```c
#include <string.h>

char *strtok_r(char *str, const char *delim, char **saveptr);
/* saveptr: caller-provided state pointer — stores position between calls */

#include <time.h>

struct tm *gmtime_r(const time_t *timep, struct tm *result);
struct tm *localtime_r(const time_t *timep, struct tm *result);

int strerror_r(int errnum, char *buf, size_t buflen);
/* XSI-compliant: returns 0 on success — check _POSIX_C_SOURCE vs _GNU_SOURCE */
```

---

### Example (Code)

#### Example 1 — Thread safety: 3 approaches compared

```c
#include <pthread.h>
#include <stdio.h>
#include <string.h>

/* ── APPROACH 1: Not thread-safe (static buffer shared by all threads) ── */
const char *bad_basename(const char *path) {
    static char buf[256];
    const char *p = strrchr(path, '/');
    strncpy(buf, p ? p + 1 : path, sizeof(buf) - 1);
    return buf;   /* two threads calling simultaneously → corrupt buf */
}

/* ── APPROACH 2: Serialized (global mutex, throughput bottleneck) ── */
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static char shared_buf[256];
const char *mutex_basename(const char *path) {
    const char *p = strrchr(path, '/');
    pthread_mutex_lock(&mtx);
    strncpy(shared_buf, p ? p + 1 : path, sizeof(shared_buf) - 1);
    pthread_mutex_unlock(&mtx);
    return shared_buf;  /* still unsafe after unlock — caller's window is gone */
}

/* ── APPROACH 3: Reentrant (caller provides buffer, zero contention) ── */
char *safe_basename_r(const char *path, char *buf, size_t buflen) {
    const char *p = strrchr(path, '/');
    strncpy(buf, p ? p + 1 : path, buflen - 1);
    buf[buflen - 1] = '\0';
    return buf;
}

static const char *paths[] = {
    "/usr/bin/gcc", "/home/user/main.c", "/var/log/syslog", "/etc/passwd"
};

void *worker(void *arg) {
    int id = *(int *)arg;
    char buf[256];
    /* Each thread has its own buf on the stack — zero contention */
    printf("Thread %d: basename = %s\n", id,
           safe_basename_r(paths[id], buf, sizeof(buf)));
    return NULL;
}

int main(void) {
    pthread_t tids[4];
    int ids[4] = {0, 1, 2, 3};
    for (int i = 0; i < 4; i++)
        pthread_create(&tids[i], NULL, worker, &ids[i]);
    for (int i = 0; i < 4; i++)
        pthread_join(tids[i], NULL);
    return 0;
}
/* gcc -pthread ex1_thread_safety.c -o ex1 */
```

---

#### Example 2 — TSD: per-thread `strerror` implementation

```c
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static pthread_key_t  errstr_key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static void free_buffer(void *buf) {
    free(buf);   /* TSD destructor: called automatically when thread exits */
}

static void create_key(void) {
    pthread_key_create(&errstr_key, free_buffer);
}

const char *my_strerror(int err) {
    /* pthread_once guarantees create_key() runs exactly once */
    pthread_once(&key_once, create_key);

    char *buf = pthread_getspecific(errstr_key);
    if (buf == NULL) {
        buf = malloc(256);
        if (!buf) return "out of memory";
        pthread_setspecific(errstr_key, buf);  /* bind to this thread */
    }
    snprintf(buf, 256, "Error %d: %s", err, strerror(err));
    return buf;   /* buf is private to this thread — no race */
}

void *worker(void *arg) {
    int fake_errno = *(int *)arg;
    /* Each thread gets its own buf — returns pointer to its own TSD slot */
    printf("Thread %lu: %s\n", (unsigned long)pthread_self(),
           my_strerror(fake_errno));
    return NULL;
    /* Thread exits here → free_buffer(buf) called automatically */
}

int main(void) {
    pthread_t t1, t2, t3;
    int e1 = ENOENT, e2 = EACCES, e3 = EINVAL;
    pthread_create(&t1, NULL, worker, &e1);
    pthread_create(&t2, NULL, worker, &e2);
    pthread_create(&t3, NULL, worker, &e3);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    printf("Main: %s\n", my_strerror(EPERM));
    return 0;
}
/* gcc -pthread ex2_tsd_strerror.c -o ex2 */
```

---

#### Example 3 — `pthread_once` + TSD destructor chain demo

```c
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 5

static pthread_key_t  tsd_key;
static pthread_once_t once   = PTHREAD_ONCE_INIT;
static int            init_count = 0;

static void tsd_destructor(void *val) {
    printf("  [dtor] thread %lu freeing val=%d\n",
           (unsigned long)pthread_self(), *(int *)val);
    free(val);
}

static void init_key(void) {
    init_count++;
    printf("[init_key] called %d time(s) — MUST be exactly 1\n", init_count);
    pthread_key_create(&tsd_key, tsd_destructor);
}

void *worker(void *arg) {
    int id = *(int *)arg;

    /* 5 threads race — only 1 will execute init_key() */
    pthread_once(&once, init_key);

    int *val = malloc(sizeof(int));
    *val = id * 100;
    pthread_setspecific(tsd_key, val);
    printf("[worker %d] TSD value = %d\n", id, *val);
    return NULL;
    /* → tsd_destructor(val) called automatically */
}

int main(void) {
    pthread_t tids[NUM_THREADS];
    int ids[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        pthread_create(&tids[i], NULL, worker, &ids[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(tids[i], NULL);

    printf("init_key was called %d time(s)\n", init_count);
    return 0;
}
/* gcc -pthread ex3_once_tsd.c -o ex3 */
```

---

#### Example 4 — `__thread` TLS + `fork()` inheritance

```c
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

__thread int tls_counter = 0;   /* each thread gets its own independent copy */

void *worker(void *arg) {
    int id = *(int *)arg;
    tls_counter = id * 10;   /* writes ONLY this thread's copy */
    printf("[thread %d] tls_counter = %d\n", id, tls_counter);

    if (id == 1) {
        pid_t pid = fork();
        if (pid == 0) {
            /* Child: only this thread survives. tls_counter = 10 (from thread 1).
             * Threads 2, 3 vanished — their TLS blocks gone. */
            printf("[child ] tls_counter = %d (inherited from forking thread only)\n",
                   tls_counter);
            tls_counter = 999;
            printf("[child ] after modify = %d\n", tls_counter);
            _exit(0);
        }
        wait(NULL);
        /* Parent thread 1: tls_counter unchanged at 10 */
        printf("[thread %d] (parent) after fork = %d\n", id, tls_counter);
    }
    return NULL;
}

int main(void) {
    pthread_t t1, t2, t3;
    int id1 = 1, id2 = 2, id3 = 3;
    pthread_create(&t1, NULL, worker, &id1);
    pthread_create(&t2, NULL, worker, &id2);
    pthread_create(&t3, NULL, worker, &id3);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    return 0;
}
/* gcc -pthread ex4_tls_fork.c -o ex4 */
```

---

#### Example 5 — `strtok_r` vs `strtok` + nested tokenization

```c
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void *safe_worker(void *arg) {
    char str[] = "alpha:beta:gamma:delta";
    char *saveptr;
    char *tok = strtok_r(str, ":", &saveptr);   /* saveptr is on the stack */
    while (tok) {
        printf("[safe %lu] token = %s\n", (unsigned long)pthread_self(), tok);
        usleep(5000);
        tok = strtok_r(NULL, ":", &saveptr);    /* re-entrant: uses our saveptr */
    }

    /* strtok_r also supports nested tokenization — impossible with strtok */
    char outer[] = "a,b|c,d";
    char *sp1, *sp2;
    char *field = strtok_r(outer, "|", &sp1);
    while (field) {
        char tmp[64];
        strncpy(tmp, field, sizeof(tmp) - 1);
        char *sub = strtok_r(tmp, ",", &sp2);
        while (sub) {
            printf("[nested] %s\n", sub);
            sub = strtok_r(NULL, ",", &sp2);
        }
        field = strtok_r(NULL, "|", &sp1);
    }
    return NULL;
}

int main(void) {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, safe_worker, NULL);
    pthread_create(&t2, NULL, safe_worker, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}
/* gcc -pthread ex5_strtok_r.c -o ex5 */
```

---

### Debugging

```bash
# Find unsafe glibc functions your binary calls that have _r equivalents
nm -D ./binary | grep -E " U (strtok|gmtime|localtime|strerror|crypt)$"

# TSan: detects data races on shared buffers
gcc -pthread -fsanitize=thread -g source.c -o binary_tsan

# Helgrind: detects pthread API misuse
valgrind --tool=helgrind ./binary

# Observe pthread_once futex calls
strace -e futex ./binary 2>&1 | head -30

# Inspect TLS template sections in binary
readelf -S ./binary | grep -E "\.tdata|\.tbss"
objdump -d ./binary | grep "%fs:"   # FS-relative memory accesses
```

**Common mistakes:**

| Bug | Symptom | Fix |
|---|---|---|
| Race on TSD key init without `pthread_once` | Crash under load (NULL key) | Always use `pthread_once` for key creation |
| Destructor re-sets key indefinitely | Memory leak after 4 rounds | Design destructor to set key to NULL, not re-register |
| `longjmp` from `init_routine` | All callers deadlock permanently | `init_routine` must always `return` normally |
| `__thread` for heap-allocated data needing cleanup | Memory leak at thread exit | Use TSD with destructor instead of `__thread` |

---

### Real-world Usage

- **glibc `errno`:** `errno` is a macro expanding to `(*__errno_location())` which returns a pointer to the calling thread's errno slot in TLS. Per-thread, zero overhead.
- **OpenSSL:** Uses TSD for per-thread error queues (`ERR_get_error()`). Each thread accumulates its own error stack without any locking.
- **Database connection pools:** `__thread` pointer to current thread's DB connection — zero overhead lookup, no hash table, no lock.
- **Apache / Nginx modules:** `pthread_once` for one-time module initialization; per-thread request context in TSD.
- **Java JNI:** JNI uses TSD to attach/detach per-thread VM metadata without synchronization.

---

### Key Takeaways

| Concept | Rule |
|---|---|
| `pthread_once` | `init_routine` MUST return normally — `longjmp`/exception = permanent deadlock for all future callers |
| TSD destructor limit | Max 4 rounds — destructor that re-sets key after round 4 leaks |
| `__thread` | Zero overhead, no destructor — use for counters/flags; use TSD for heap data needing cleanup |
| `fork()` + TLS | Child inherits only calling thread's TLS — all other threads' TLS data gone |
| `_r` functions | Prefer over unsafe originals — call-stack state, no shared buffers, also signal-reentrant |
| Thread safety ≠ signal reentrant | Mutex-protected function is thread-safe but not signal-safe if handler also locks it |
