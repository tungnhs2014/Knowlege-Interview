# Topic Brief 16 - POSIX/Linux C API Vs Modern C++

## 1. Canonical Routing

| Field | Validated value |
| --- | --- |
| `learning_path_number` | `16` |
| Title | POSIX/Linux C API Vs Modern C++ |
| `slug` | `posix-linux-c-api-vs-modern-cpp` |
| Requested topic | Practical comparison of POSIX/Linux user-space C APIs with modern C++ standard-library facilities and RAII wrappers |
| Master source | `master-ch18` |
| Required Notion sources | `notion-9-1`, `notion-10-7`, `notion-10-8`, `notion-10-9` |
| Topic Brief | `coverage/topic-briefs/16-posix-linux-c-api-vs-modern-cpp.md` |
| Knowledge target | `knowledge/16-posix-linux-c-api-vs-modern-cpp.md` |
| Interview target | `interview/16-posix-linux-c-api-vs-modern-cpp.md` |
| Example target | `examples/16-posix-linux-c-api-vs-modern-cpp/README.md` |

Validation result: the number, title, slug, master source, mapped Notion source
list, and output paths match `LEARNING_PATH.md`.

This step creates the Topic Brief only. It does not create or modify knowledge,
interview, or example outputs.

## 2. Source Coverage

### Internal Sources Read

| Source label | Path | Coverage contribution |
| --- | --- | --- |
| `master-ch18` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, CH18 | MUST priority for Embedded Linux/Linux Software, CH07/CH12/CH15 prerequisites, POSIX/Linux vs Modern C++ comparison matrix, and interview focus |
| `guide-section-03` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 3 | MUST-topic depth requirements |
| `guide-section-06` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 6.2 | Required C/POSIX/Linux vs Modern C++ comparison format |
| `guide-section-07` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 7 | Depth control and practical-example rules |
| `guide-section-14` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 14 | Quality checklist |
| `notion-index` | `docs/C++ Notion/C++ Programming.md` | Notion chapter inventory and identity validation |
| `notion-9-1` | `docs/C++ Notion/Chapter 9-1 File Handling - Basics to Advanced Operations.md` | C++ file streams, modes, binary/text I/O, file-state checks, exceptions, RAII file wrapper, copy/count/log examples, and interview points |
| `notion-10-7` | `docs/C++ Notion/Chapter 10-7 Signal Handling.md` | `signal`, `raise`, POSIX `sigaction`, async-signal-safe rules, `sig_atomic_t`, graceful shutdown, platform notes, and signal best practices |
| `notion-10-8` | `docs/C++ Notion/Chapter 10-8 Multithreading Basics.md` | `std::thread`, join/detach/joinable, lambdas/member functions, race conditions, `std::mutex`, `std::lock_guard`, `std::unique_lock`, thread-safe classes, and `std::atomic` basics |
| `notion-10-9` | `docs/C++ Notion/Chapter 10-9 Multithreading Advanced.md` | `std::condition_variable`, predicates/spurious wakeups, producer-consumer, `std::future`, `std::async`, deadlock prevention, `std::scoped_lock`, thread pool, advanced atomics, and best practices |

All four mapped Notion chapter files were inspected. No mapped Notion source was
skipped.

### External References Consulted

Accessed on 2026-06-27.

| Label | Reference | Validation purpose |
| --- | --- | --- |
| `external-man7-pthread-create` | Linux man-pages `pthread_create(3)`: <https://man7.org/linux/man-pages/man3/pthread_create.3.html> | POSIX thread creation signature, `void*` start routine argument, joinable/detached lifecycle, return/error style |
| `external-man7-open` | Linux man-pages `open(2)`: <https://man7.org/linux/man-pages/man2/open.2.html> | File descriptor model, `open` flags, `O_CLOEXEC`, `read/write/close` relationship |
| `external-man7-socket` | Linux man-pages `socket(2)`: <https://man7.org/linux/man-pages/man2/socket.2.html> | Socket creation as file-descriptor API and basis for socket comparison |
| `external-man7-signal-safety` | Linux man-pages `signal-safety(7)`: <https://man7.org/linux/man-pages/man7/signal-safety.7.html> | Async-signal-safe function boundary and signal-handler restrictions |
| `external-man7-sigaction` | Linux man-pages `sigaction(2)`: <https://man7.org/linux/man-pages/man2/sigaction.2.html> | POSIX signal registration and masks/flags behavior |
| `external-man7-fork-exec-wait-pipe-dup2` | Linux man-pages `fork(2)`, `execve(2)`, `waitpid(2)`, `pipe(2)`, `dup2(2)` | Process-control and pipe/redirection behavior |
| `external-man7-io-event-time` | Linux man-pages `read(2)`, `write(2)`, `close(2)`, `ioctl(2)`, `select(2)`, `poll(2)`, `epoll(7)`, `getaddrinfo(3)`, `nanosleep(2)`, `clock_gettime(2)` | Exact POSIX/Linux user-space behavior for I/O, event loops, networking lookup, sleep, and clocks |
| `external-cppreference-thread` | cppreference `std::thread`: <https://en.cppreference.com/w/cpp/thread/thread> | C++ thread object lifecycle and join/detach comparison |
| `external-cppreference-threading-family` | cppreference C++ thread support pages for `std::mutex`, locks, condition variables, semaphores, atomics, `std::chrono`, `std::filesystem`, and `std::fstream` | C++ standard-library counterparts to POSIX APIs |

### Source Coverage Status

`TOPIC_BRIEF_COMPLETE_WITH_POSIX_VALIDATION`: canonical routing, master
comparison matrix, guide comparison rules, all mapped Notion files, POSIX/Linux
user-space references, C++ standard-library references, merged concepts,
required comparisons, common bugs, debugging notes, best practices, interview
angles, gaps, external validation needs, and output targets are recorded.

## 3. Priority And Dependencies

- Overall priority: `MUST` for Embedded Linux / Linux Software.
- Required depth: medium to deep.
- Master prerequisites:
  - CH07, Operating System Concepts, for process, thread, file descriptor, IPC,
    and event-loop background.
  - CH12, Modern C++ Features, for `std::thread`, lambdas, RAII, atomics,
    futures, and chrono.
  - CH15, C Vs C++ Comparison, for C API boundary, ownership, RAII, callbacks,
    error translation, and ABI vocabulary.
- Practical prerequisites:
  - Linux user-space programming basics.
  - Function pointers and `void*` context.
  - Return-code and `errno` error handling.
  - RAII and smart-pointer/custom-deleter patterns.
  - Thread lifetime, synchronization, and data-race basics.

## 4. Scope And Depth Boundaries

### Deep In This Topic

- Threading:
  - `pthread_create` vs `std::thread`.
  - `pthread_join` vs `std::thread::join`.
  - `pthread_detach` vs `std::thread::detach`.
  - `void* arg` vs lambda/functor/member function.
- Synchronization:
  - `pthread_mutex_t` vs `std::mutex`.
  - `pthread_mutex_lock/unlock` vs `std::lock_guard`.
  - `pthread_cond_t` vs `std::condition_variable`.
  - `sem_t` vs `std::counting_semaphore`.
  - POSIX semaphore vs mutex + condition variable.
- Process:
  - `fork`, `exec`, `wait`, `waitpid`, `pipe`, `dup2`.
  - No direct C++ standard process API.
  - C++ RAII wrapper for process/file descriptors.
- File I/O:
  - `open/read/write/close` vs `std::fstream`.
  - `fopen/fread/fwrite/fclose` vs `std::fstream`.
  - file descriptor vs stream.
  - `ioctl` has no C++ standard equivalent.
  - `std::filesystem`.
  - RAII file descriptor wrapper.
- Socket / event loop:
  - `socket/bind/listen/accept/connect`.
  - `send/recv`.
  - `select/poll/epoll`.
  - `getaddrinfo`.
  - No common C++ standard socket API.
  - Boost.Asio or custom wrapper awareness.
- Time:
  - `sleep/usleep/nanosleep` vs `std::this_thread::sleep_for`.
  - `clock_gettime` vs `std::chrono`.
  - manual units vs type-safe duration.
- Atomic / volatile:
  - C11 `_Atomic` vs `std::atomic`.
  - GCC `__sync_*` / `__atomic_*` vs `std::atomic`.
  - `volatile` vs atomic.
- Signals:
  - C/C++ `signal`/`raise` vs POSIX `sigaction`.
  - Async-signal-safe handler discipline.
  - `volatile sig_atomic_t` and lock-free atomics for shutdown flags.

### Medium In This Topic

- `std::future`, `std::promise`, `std::async`, and `std::packaged_task` as
  higher-level C++ concurrency alternatives.
- `std::scoped_lock` and deadlock-avoidance patterns.
- Text/binary file stream modes and stream exception settings.
- `std::filesystem` for path and metadata operations, not as a replacement for
  descriptor-level I/O.
- Boost.Asio as a common production C++ networking abstraction, but not a C++
  standard facility.

### Controlled Awareness

- Linux-specific APIs such as `epoll` and `ioctl` are user-space Linux APIs, not
  C++ standard APIs.
- `std::jthread` and stop tokens are useful C++20 additions but not required for
  the core topic unless a downstream lesson chooses C++20 depth.
- `std::counting_semaphore` is C++20; examples should mark the version or use
  mutex/condition-variable alternatives for C++17.
- Exact `errno` restart behavior, signal interruption, nonblocking I/O, and
  close-on-exec races should be handled carefully in examples.

### Defer Or Exclude

- Linux Device Driver, kernel-driver, kernel module, hardware driver, Yocto,
  GStreamer, AUTOSAR, and unrelated platform material are excluded.
- Full C vs C++ language comparison belongs to topic 15.
- Full concurrency fundamentals belong to topic 14.
- Full enterprise checklist belongs to topic 18.
- Full networking tutorial is out of scope; focus on POSIX API vs C++ wrapping
  and architecture decisions.

## 5. Merged Concept Map

- POSIX/Linux APIs are C user-space APIs exposed through headers such as
  `<pthread.h>`, `<unistd.h>`, `<fcntl.h>`, `<sys/socket.h>`, `<sys/wait.h>`,
  `<poll.h>`, `<sys/epoll.h>`, `<signal.h>`, and `<time.h>`.
- Modern C++ standard-library APIs live in headers such as `<thread>`,
  `<mutex>`, `<condition_variable>`, `<semaphore>`, `<atomic>`, `<chrono>`,
  `<filesystem>`, `<fstream>`, and `<future>`.
- C++ does not replace all POSIX/Linux APIs. It standardizes common language and
  library abstractions, but processes, file descriptors, sockets, `ioctl`, and
  Linux event loops still require POSIX/Linux APIs or third-party wrappers.
- The most important production move is wrapping POSIX resources with RAII:
  file descriptors, `FILE*`, sockets, mutex locks, process handles/status, and
  thread joins.
- POSIX APIs usually report errors with return values and `errno`. C++ wrappers
  may translate to exceptions, `std::error_code`, or Result-style values, but
  they must preserve enough operation/context information for debugging.
- Thread and signal APIs are especially easy to misuse because lifetime,
  cancellation, signal masks, async-signal-safety, data races, and deadlocks are
  cross-cutting concerns.
- A strong comparison answer should always clarify:
  - POSIX/Linux API is not core C and not core C++;
  - C++ standard library may provide a safer equivalent for some domains;
  - for domains without a C++ standard equivalent, C++ still uses POSIX wrapped
    in RAII and type-safe interfaces;
  - the common bug and debugging method.

## 6. Required Comparisons To Preserve

Every downstream lesson/interview answer should use this compact table shape
when expanding a pair:

```md
| Topic | C/POSIX/Linux | Modern C++ | Enterprise Usage |
| --- | --- | --- | --- |
```

### Threading

| Pair | POSIX/Linux side | Modern C++ side | Guidance |
| --- | --- | --- | --- |
| `pthread_create` vs `std::thread` | C function, `pthread_t`, attributes, `void* (*)(void*)` start routine and `void* arg` | Type-safe callable construction from function, lambda, functor, or member function | Prefer `std::thread` in C++ unless pthread attributes/scheduling/interop are required |
| `pthread_join` vs `std::thread::join` | Join by `pthread_t`, optional `void*` return value | Member function on owning thread object | Always join or detach before destruction; prefer RAII join wrappers for exception safety |
| `pthread_detach` vs `std::thread::detach` | Detached thread resources release automatically; cannot join | Detached thread continues independently | Avoid detach unless lifetime and shutdown are explicit |
| `void* arg` vs lambda/functor/member function | Manual cast and lifetime discipline | Type-safe captures and callable objects | Use `void*` for POSIX/C callbacks; use lambdas/functors internally with capture lifetime care |

### Synchronization

| Pair | POSIX/Linux side | Modern C++ side | Guidance |
| --- | --- | --- | --- |
| `pthread_mutex_t` vs `std::mutex` | C object initialized/destroyed with pthread functions | RAII-friendly C++ mutex object | Prefer `std::mutex` in C++ unless pthread-specific attributes are required |
| `pthread_mutex_lock/unlock` vs `std::lock_guard` | Manual lock/unlock, easy to leak lock on early return/error | Scoped lock/unlock through destructor | Prefer scoped locking for exception safety |
| `pthread_cond_t` vs `std::condition_variable` | Condition wait with pthread mutex and predicates managed manually | Wait with `std::unique_lock` and predicate overload | Always wait in a predicate loop; handle spurious wakeups |
| `sem_t` vs `std::counting_semaphore` | POSIX semaphore with init/wait/post/destroy | C++20 counting semaphore | Use C++20 semaphore when available; otherwise use POSIX or mutex+condition variable |
| POSIX semaphore vs mutex + condition variable | Counting resource coordination | Structured condition-based coordination | Prefer condition variable when waiting for a state predicate; semaphore for counted permits |

### Process

| Pair | POSIX/Linux side | Modern C++ side | Guidance |
| --- | --- | --- | --- |
| `fork` | Duplicates current process image | No direct standard C++ process API | Use POSIX; wrap child-management paths carefully |
| `exec` | Replaces process image | No direct standard C++ process API | Use POSIX for launching programs; build safe argument/environment handling |
| `wait` / `waitpid` | Reap child status | No direct standard C++ process API | Wrap child PIDs/status in RAII-like management where possible |
| `pipe` / `dup2` | File-descriptor IPC and redirection | No direct standard C++ equivalent | Use RAII descriptors and close unused ends in both parent and child |
| C++ process/file-descriptor wrapper | Manual fd/PID lifetime | RAII wrapper, move-only ownership, error translation | Prefer move-only wrappers to prevent double-close and leaks |

### File I/O

| Pair | POSIX/Linux side | Modern C++ side | Guidance |
| --- | --- | --- | --- |
| `open/read/write/close` vs `std::fstream` | File descriptor, byte-oriented syscalls, `errno`, partial reads/writes | Stream abstraction, formatting, RAII, stream states/exceptions | Use descriptors for OS integration, nonblocking, sockets, `poll`/`epoll`; use streams for formatted file I/O |
| `fopen/fread/fwrite/fclose` vs `std::fstream` | C buffered `FILE*` API | C++ stream classes | Prefer C++ streams for C++ text/binary file code unless C ABI or existing C library uses `FILE*` |
| file descriptor vs stream | Integer handle to kernel/user-space open file description | High-level C++ object wrapping file operations | Use descriptor wrappers when integrating with POSIX APIs |
| `ioctl` vs C++ | Device/control operation with request codes | No standard C++ equivalent | Use POSIX/Linux API directly behind a small typed wrapper |
| `std::filesystem` | Not a POSIX syscall wrapper for all I/O | Path, metadata, directory traversal, file operations | Use for path/metadata portability; still use POSIX for descriptors and special files |
| RAII file descriptor wrapper | Manual `close` on every path | Destructor closes descriptor; move-only ownership | Required pattern for production C++ POSIX code |

### Socket / Event Loop

| Pair | POSIX/Linux side | Modern C++ side | Guidance |
| --- | --- | --- | --- |
| `socket/bind/listen/accept/connect` | C socket API returning file descriptors | No common C++ standard socket API | Use POSIX directly or Boost.Asio/custom wrapper |
| `send/recv` | Byte-oriented socket I/O, partial operations, `errno` | No standard C++ socket I/O | Wrap retry/partial-send logic and ownership |
| `select/poll/epoll` | Readiness/event APIs, with `epoll` Linux-specific | No standard C++ event loop | Use platform abstraction or event library |
| `getaddrinfo` | Name/service resolution with linked result list | No standard socket resolver | Wrap result lifetime and error handling |

### Time

| Pair | POSIX/Linux side | Modern C++ side | Guidance |
| --- | --- | --- | --- |
| `sleep/usleep/nanosleep` vs `std::this_thread::sleep_for` | Manual units and signal interruption concerns | Type-safe duration objects | Prefer `sleep_for` in C++ unless POSIX-specific signal/restart behavior is needed |
| `clock_gettime` vs `std::chrono` | Specific POSIX clocks and `timespec` | Type-safe clocks/durations/time points | Prefer `std::chrono` for C++ timing; use POSIX when a specific clock/API contract is needed |
| manual units vs type-safe duration | Seconds/microseconds/nanoseconds as integers | `std::chrono::duration` types | Avoid unit bugs with chrono |

### Atomic / Volatile

| Pair | POSIX/Linux side | Modern C++ side | Guidance |
| --- | --- | --- | --- |
| C11 `_Atomic` vs `std::atomic` | C atomic type qualifier and operations | C++ template atomic types and memory orders | Use `std::atomic` in C++ code; understand memory order |
| GCC `__sync_*` / `__atomic_*` vs `std::atomic` | Compiler intrinsics, nonportable or low-level | Standard C++ portable abstraction | Prefer `std::atomic` unless interfacing with low-level/compiler-specific code |
| `volatile` vs atomic | Prevents some compiler optimizations for object access | Provides inter-thread synchronization and atomicity | `volatile` is not a thread synchronization primitive |

### Signals

| Pair | POSIX/Linux side | Modern C++ side | Guidance |
| --- | --- | --- | --- |
| `signal` vs `sigaction` | `signal` is simpler; `sigaction` gives masks/flags and reliable POSIX behavior | C++ can call both through `<csignal>`/`<signal.h>` | Prefer `sigaction` on POSIX/Linux production code |
| async-signal-safe handler vs C++ cleanup | Handler can only do a tiny set of safe operations | C++ cleanup should run after main loop observes a flag | Set a flag or write to a self-pipe; do real cleanup outside handler |
| `volatile sig_atomic_t` vs lock-free atomic flag | C signal-safe flag type | `std::atomic` only if lock-free and used carefully | Keep signal handlers minimal and avoid locks, allocation, streams, and exceptions |

## 7. Common Bugs And Corrections

- Treating POSIX APIs as "the C language" or C++ APIs as replacing all POSIX.
  Correction: clarify POSIX/Linux vs ISO C/C++ standard-library boundaries.
- Forgetting to join or detach a `std::thread`.
  Correction: join/detach before destruction; prefer RAII joining wrappers or
  `std::jthread` when using C++20.
- Detaching threads that access stack objects or destroyed owners.
  Correction: make lifetime explicit or avoid detach.
- Passing a pointer to stack data through `pthread_create` when the object may
  go out of scope.
  Correction: ensure argument storage outlives the thread or use C++ capture by
  value/owned state.
- Manual `pthread_mutex_lock` without guaranteed unlock on every path.
  Correction: use `std::lock_guard`, `std::unique_lock`, or custom RAII wrappers.
- Waiting on condition variables without a predicate.
  Correction: use predicate waits to handle spurious wakeups.
- Assuming `volatile` fixes data races.
  Correction: use mutexes or atomics.
- Ignoring partial `read`, `write`, `send`, or `recv` results.
  Correction: loop until protocol/file requirement is satisfied or handle
  nonblocking/EINTR/EAGAIN policy.
- Leaking file descriptors on early return, exception, or `exec`.
  Correction: use RAII fd wrappers and `O_CLOEXEC`/close-on-exec policy.
- Double-closing descriptors after copy.
  Correction: make descriptor wrappers move-only.
- Using `std::fstream` where descriptor integration is required.
  Correction: use `open`/descriptor API for `select`, `poll`, `epoll`, sockets,
  `dup2`, and `ioctl`.
- Calling non-async-signal-safe functions from a signal handler.
  Correction: set a `sig_atomic_t`/lock-free flag or write to a pipe; do real
  work outside the handler.
- Throwing exceptions across C/POSIX callback boundaries.
  Correction: catch at the boundary and translate to error codes.
- Confusing wall-clock time with monotonic time.
  Correction: use monotonic clocks or `std::chrono::steady_clock` for intervals.

## 8. Debugging Notes

- Build warnings:
  - `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`.
  - Add `-pthread` when using POSIX threads or C++ threads on typical Linux
    toolchains.
- Sanitizers:
  - AddressSanitizer/LeakSanitizer for descriptor-wrapper ownership bugs only
    indirectly; use fd leak checks and process tracing for descriptors.
  - ThreadSanitizer for data races in C++/pthread code.
  - UBSan for undefined behavior in mixed low-level code.
- Runtime tracing:
  - `strace` for `open/read/write/close`, `fork/exec/wait`, sockets, `poll`, and
    `epoll` behavior.
  - `lsof` or `/proc/<pid>/fd` for file-descriptor leaks.
  - `gdb` for deadlock backtraces and stuck threads.
  - `pstack`/`gdb thread apply all bt` style workflows for blocked threads.
- Concurrency debugging:
  - Check thread join/detach ownership.
  - Check mutex lock ordering.
  - Check condition-variable predicates.
  - Check callbacks under locks and shutdown ordering.
- Signal debugging:
  - Audit handler body for async-signal-safe operations only.
  - Check signal masks and `sigaction` flags.
  - Avoid `std::cout`, `malloc`, `new`, locks, and exceptions in handlers.
- I/O debugging:
  - Always log operation, path/fd, return value, and `errno` context.
  - Handle partial I/O and retry policy deliberately.
  - Distinguish EOF, temporary unavailability, interruption, and fatal error.

## 9. Best Practices

- Use POSIX/Linux APIs directly when the domain is truly process control,
  descriptor-level I/O, sockets, event loops, `ioctl`, or exact Linux behavior.
- Use Modern C++ standard APIs when they express the same intent portably:
  `std::thread`, `std::mutex`, `std::lock_guard`, `std::condition_variable`,
  `std::atomic`, `std::chrono`, `std::filesystem`, and `std::fstream`.
- Wrap every owning POSIX resource in a move-only RAII type:
  file descriptor, socket, `DIR*`, `FILE*`, mutex-like handles where applicable,
  and child-process lifecycle helpers.
- Do not expose raw ownership ambiguously. Name functions `take`, `borrow`,
  `release`, `close`, or similar when ownership changes.
- Prefer `std::lock_guard` for simple scoped locking and `std::unique_lock` for
  condition variables or flexible locking.
- Prefer `std::scoped_lock` or `std::lock` for multiple mutexes.
- Use `std::atomic` for simple shared flags/counters; use mutexes for compound
  invariants.
- Use `std::chrono` durations instead of raw integer time units in C++ APIs.
- Use `std::filesystem` for path and metadata work, not as a replacement for
  sockets, pipes, nonblocking descriptors, or `ioctl`.
- Keep signal handlers tiny and async-signal-safe.
- Translate errors at boundaries while preserving `errno`, operation name, and
  resource identity.
- Clarify C++ standard version requirements: C++17 for `std::filesystem` and
  `std::scoped_lock`; C++20 for `std::counting_semaphore` and `std::jthread`.

## 10. Interview Angles

- Is `fork()` part of C++? No; it is POSIX/Linux user-space API.
- Does the C++ standard library have sockets? No common standard socket API.
- Why still use `ioctl` in C++? Some OS/device/control operations have no C++
  standard equivalent.
- Compare `pthread_create` with `std::thread`.
- Compare `pthread_mutex_lock/unlock` with `std::lock_guard`.
- Why is `std::thread` destructor dangerous if the thread is still joinable?
- When is `pthread` still needed in a C++ program?
- Why wrap a file descriptor in an RAII class?
- `open/read/write/close` vs `std::fstream`: when is each right?
- Why can `write` return a partial count?
- Why should `O_CLOEXEC` matter in process-launching code?
- How do `pipe` and `dup2` support shell-style redirection?
- Why does `std::filesystem` not replace descriptor-level APIs?
- `select` vs `poll` vs `epoll` at a design level.
- Why use Boost.Asio or a custom wrapper for sockets?
- `sleep/usleep/nanosleep` vs `std::this_thread::sleep_for`.
- `clock_gettime` vs `std::chrono::steady_clock`.
- Why is `volatile` not enough for thread synchronization?
- What is async-signal-safety?
- Why prefer `sigaction` over `signal` on POSIX/Linux?
- How would you shut down a multithreaded server on `SIGTERM` safely?

## 11. Practice Targets

- Write a move-only RAII `Fd` wrapper around `open`/`close`; test move and
  double-close prevention.
- Compare a `pthread_create` example using `void* arg` with a `std::thread`
  lambda version.
- Convert manual `pthread_mutex_lock/unlock` code into `std::lock_guard`.
- Write a `std::condition_variable` producer-consumer queue with a predicate.
- Demonstrate a partial-write loop for a file descriptor or socket.
- Wrap `getaddrinfo` result lifetime in RAII.
- Write a tiny `fork` + `exec` + `pipe` + `dup2` launcher and list which file
  descriptors must be closed in parent and child.
- Implement a graceful shutdown flag set by `sigaction` and consumed by the main
  event loop.
- Compare `nanosleep` and `std::this_thread::sleep_for` in a small timing
  example.
- Demonstrate why `volatile bool` races and replace it with `std::atomic<bool>`.

## 12. Gaps And External Validation Needs

- Exact Linux API behavior varies by syscall, flags, blocking mode, signal
  interruption, and glibc/kernel version. Downstream code examples should check
  the relevant man page for each specific API used.
- `std::counting_semaphore` and `std::jthread` require C++20; C++17 examples
  should use `std::condition_variable`, `std::thread`, and custom RAII joiners.
- `std::filesystem` availability and behavior depend on compiler/library
  version and platform permissions/filesystem semantics.
- `close` error handling is subtle; examples should avoid retrying `close` on
  the same descriptor blindly and should document error policy.
- Async-signal-safety is exacting; downstream examples must not call iostreams,
  allocation, locks, or exceptions inside signal handlers.
- Socket and `epoll` examples can grow quickly; keep topic examples minimal and
  focused on comparison, RAII, and debugging workflow.
- No Linux Device Driver/kernel-driver material is needed or allowed for this
  topic.

## 13. Suggested Output Targets

- `knowledge/16-posix-linux-c-api-vs-modern-cpp.md`
  - Teach in order: goal, why this matters, mental model, POSIX/Linux API
    categories, modern C++ equivalents, RAII wrappers, required comparison
    tables, common bugs, debugging, best practices, interview readiness, and
    practice.
  - Use the required table shape:
    `| Topic | C/POSIX/Linux | Modern C++ | Enterprise Usage |`.
  - Preserve the boundary rule: POSIX/Linux APIs are not core C++; C++ often
    wraps them rather than replacing them.
  - Keep source coverage/audit metadata out.
- `interview/16-posix-linux-c-api-vs-modern-cpp.md`
  - Include beginner, mid-level, and senior questions.
  - Each answer should include short answer, deep explanation, C/POSIX/Linux
    API anchor, Modern C++ API anchor, production/debug angle, traps, and
    follow-ups.
  - Include coding/debugging tasks for RAII fd wrapper, pthread-to-thread
    conversion, condition-variable predicate bug, partial write loop, and
    signal-safe shutdown.
- `examples/16-posix-linux-c-api-vs-modern-cpp/README.md`
  - Include compile-ready examples only if useful:
    - RAII file descriptor wrapper;
    - `pthread_create` vs `std::thread`;
    - `pthread_mutex_lock/unlock` vs `std::lock_guard`;
    - `open/read/write/close` vs `std::fstream`;
    - `sigaction` graceful shutdown flag;
    - optional pipe/dup2 mini-example.
  - Add build commands with `-pthread` and sanitizer/debug commands, especially
    ThreadSanitizer where practical.
  - Mark POSIX/Linux-only examples clearly.

