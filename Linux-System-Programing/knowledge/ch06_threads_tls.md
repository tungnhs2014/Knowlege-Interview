# Chapter 6 - Thread Safety, Reentrancy, TSD, and TLS

> Topics: 6.3 Thread safety, reentrancy, `pthread_once()`, thread-specific data, TLS
> Main sources: TLPI Ch31; DevLinux Module 05
> Source notes: DevLinux is thin on TSD/TLS. TLPI is the main source for thread safety, reentrancy, one-time initialization, TSD destructors, and `__thread` portability.

## Learning Goal

Understand whether a C function can be called by many threads at the same time, and learn the design options for making legacy static-state code safe.

After this file, you should be able to:

- distinguish thread-safe from reentrant;
- spot hidden static/global state bugs;
- use `pthread_once()` for lazy library initialization;
- choose caller-owned storage, TSD, or TLS for per-thread state.

## Coverage Notes

This file covers learning-map row **6.3 Thread Safety & TLS** and the Chapter 6 must-cover items for thread safety, reentrancy, thread-local storage, `pthread_key`, `pthread_once()`, `__thread`, C/C++ thread-local storage concepts, per-thread `errno`, production debugging, and interview readiness.

Moved or split coverage:

- data races, memory visibility, mutex discipline, and atomics boundaries are covered in [ch06_threads_sync.md](ch06_threads_sync.md);
- cancellation cleanup of TSD-owning threads is covered in [ch06_threads_cancel.md](ch06_threads_cancel.md);
- per-thread stack and signal-mask details are covered in [ch06_threads_details.md](ch06_threads_details.md).

## Problem It Solves

Thread safety solves the problem of old C interfaces and library helpers assuming one caller at a time. Once many threads call the same code, hidden static buffers, global cursors, lazy initialization flags, and shared error state can corrupt results unless state ownership is made explicit.

## Mental Model

Thread safety asks: "Can multiple threads call this code concurrently without corrupting state or returning another thread's result?"

```text
unsafe legacy function
    |
    +-- hidden static buffer
    +-- hidden global cursor/state
    +-- global cache without locking
```

Reentrant design moves state ownership to the caller. TSD/TLS gives each thread its own persistent copy when you cannot change the old interface.

| Concept | Meaning | Best use |
|---|---|---|
| thread-safe | concurrent calls are valid | existing APIs, internal locks, libc functions |
| reentrant | no hidden shared mutable state | new APIs and signal-safe thinking |
| TSD | POSIX key -> one value per thread | legacy APIs needing per-thread heap state |
| TLS | per-thread variable syntax | simple per-thread counters/buffers on known toolchains |

## Mechanism

The classic failure is a static return buffer.

```text
Thread A calls format_error() -> gets pointer to static buf
Thread B calls format_error() -> overwrites same buf
Thread A prints pointer      -> sees Thread B's message
```

A mutex around the function may protect the write, but it does not protect the returned pointer after the function unlocks. Better designs are:

- caller provides the output buffer;
- function returns allocated storage with clear ownership;
- each thread gets its own buffer via TSD/TLS.

`errno` is a useful mental anchor. Code uses `errno` like a variable, but in threaded programs each thread has its own value via C library support. Always include `<errno.h>`; do not declare `extern int errno`.

## Key APIs And Objects

| API/object | Role | Production rule |
|---|---|---|
| `pthread_once_t` | one-time init control | initialize statically with `PTHREAD_ONCE_INIT` |
| `pthread_once()` | run init once across all threads | use instead of ad hoc static boolean checks |
| `pthread_key_t` | process-wide TSD key | usually one key per library facility |
| `pthread_key_create()` | create key and optional destructor | key creation is not per-thread allocation |
| `pthread_getspecific()` | get this thread's value for key | `NULL` means no value has been set for this thread |
| `pthread_setspecific()` | set this thread's value for key | value is commonly a heap pointer |
| TSD destructor | cleanup at thread termination | order among different keys is unspecified |
| `__thread` | TLPI's TLS storage specifier | common GCC/glibc extension, not POSIX |
| `_Thread_local` / `thread_local` | standard C/C++ thread-local storage spellings | prefer where the project language standard and toolchain support them |

Many status-returning pthread calls, such as `pthread_once()`, `pthread_key_create()`, and `pthread_setspecific()`, return `0` on success or a positive error number on failure. Other APIs here return different types, such as a pointer from `pthread_getspecific()`, so check each API's contract before writing wrappers.

## Lifecycle / Data Flow

Reentrant API flow:

```text
caller owns buffer/state
    |
    v
function writes into caller-provided storage
    |
    v
caller controls lifetime and synchronization
```

TSD flow for a legacy return-pointer API:

```text
first caller from any thread
    |
    v
pthread_once() creates process-wide key
    |
    v
each thread calls function
    |
    +-- pthread_getspecific(key) returns existing buffer
    |
    +-- or NULL: allocate buffer, pthread_setspecific(key, buffer)
    |
    v
function writes into this thread's buffer
```

Thread termination cleanup:

```text
thread exits
    |
    v
for each key with non-NULL value and destructor
    |
    v
destructor(value) runs
```

TLS flow:

```c
static __thread char scratch[256];
```

Each thread sees its own `scratch`. The storage disappears when that thread terminates, but TLPI's `__thread` does not give you a POSIX destructor hook. C11 `_Thread_local` and C++ `thread_local` are the language-level versions to consider in newer codebases, with the usual toolchain and ABI checks for the target.

## Production Bugs And Debugging

Thread-safety bugs often look like "impossible" string corruption, wrong user/session data, or rare crashes that vanish under logging.

| Symptom | Likely cause | Evidence | Fix |
|---|---|---|---|
| one thread sees another's formatted string | static return buffer | same pointer returned in different threads | caller buffer, TSD, or TLS |
| rare parser state corruption | global/static mutable parser state | `rg 'static'` in helper library | make state explicit per instance |
| double initialization crash | lazy init with unlocked static flag | two threads run init | `pthread_once()` |
| memory leak per worker lifetime | TSD value without destructor | heap grows with thread churn | register destructor |
| slow but correct helper | one big library mutex | contention in profiles/backtraces | lock only shared data or redesign API |
| wrong random sequences | shared `rand()` state or poor API choice | multiple threads call `rand()` | use per-thread PRNG state |

Useful checks:

```bash
rg '\\bstatic\\b' src include
rg '\\b(strtok|asctime|ctime|gmtime|localtime|rand|gethostbyname)\\b' src include
gcc -g -O1 -fsanitize=thread -pthread app.c -o app_tsan
readelf -S ./app | grep -E '\\.tdata|\\.tbss'
gdb -p <pid>
(gdb) thread apply all bt
```

ThreadSanitizer can find many unsynchronized shared-memory accesses, but it will not prove a bad API contract is good. Review ownership and returned pointer lifetimes.

## Work Checklist

When writing or reviewing C helpers:

- [ ] Prefer reentrant APIs with caller-owned state for new code.
- [ ] Avoid returning pointers to static mutable buffers.
- [ ] Search legacy code for static mutable locals and process-wide cursors.
- [ ] Use `pthread_once()` for lazy initialization shared by all threads.
- [ ] Use TSD when an old ABI must return a pointer but needs per-thread storage.
- [ ] Register TSD destructors for heap allocations.
- [ ] Keep TSD destructors independent; their order is unspecified.
- [ ] Use TLS only when the compiler/runtime portability tradeoff is acceptable.
- [ ] Document whether the function is thread-safe, reentrant, or neither.

## Recognize / Advanced

| Detail | Practical meaning |
|---|---|
| POSIX thread-safe functions | most standardized functions are required safe, but important legacy exceptions exist |
| `_r` functions | caller-buffer alternatives; some are obsolete or superseded by better APIs |
| thread-safe but not reentrant | internal locks may make thread calls safe but signal-handler reentry unsafe |
| TSD key limits | POSIX guarantees a minimum; Linux supports more, but keys are finite |
| destructor ordering | do not make one TSD destructor depend on another key's destructor |
| `__thread` support | TLPI notes support depends on compiler, libc, pthread implementation, and kernel era |
| C/C++ TLS spelling | `_Thread_local` and `thread_local` may be more idiomatic in newer code, but existing C/pthread code often still uses `__thread` |

Source gap: DevLinux does not deeply cover TSD, cleanup destructors, `pthread_once()` failure modes, or TLS portability. Treat TLPI/POSIX/man-pages as the authority for production code here.

## Interview Readiness

A strong answer starts with hidden shared state and then offers the least invasive safe design.

Be ready to explain:

- thread-safe vs reentrant;
- why a mutex cannot fully fix a static-return-buffer API;
- how `errno` works in threaded programs;
- why `pthread_once()` is better than a static boolean flag;
- what a TSD key represents versus a TSD value;
- when TLS is simpler than TSD and when it is less portable;
- how you would audit a legacy C helper library for thread-safety bugs.

Interview trap: "Use a mutex" is not always enough. If the API returns hidden shared storage, the caller can still observe another thread's later write.

## Final Coverage Check

Covered: mapped row 6.3; thread-safe vs reentrant; hidden shared state; `pthread_once()`; TSD keys and values; TSD destructors; `__thread`; `_Thread_local`/`thread_local` recognition; per-thread `errno`; production audit/debug workflow; interview framing.

Moved: race synchronization and memory-visibility details to `ch06_threads_sync.md`; cancellation cleanup design to `ch06_threads_cancel.md`; stack/signal per-thread details to `ch06_threads_details.md`.
