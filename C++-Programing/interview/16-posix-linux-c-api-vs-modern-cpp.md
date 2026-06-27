# 16 - POSIX/Linux C API Vs Modern C++ Interview Pack

## How To Use This Pack

These questions test whether a candidate can distinguish POSIX/Linux user-space APIs from ISO C++ facilities, choose the right abstraction, and design safe C++ wrappers around low-level resources.

Strong answers should mention ownership, lifetime, `errno`, RAII, thread shutdown, signal safety, partial I/O, and what the C++ standard library does not provide.

## Beginner Questions

### 1. Is `fork()` part of C++?

**Short answer:** No. `fork()` is a POSIX/Linux user-space API, not part of the C++ standard library.

**Deep explanation:** C++ standardizes language features and library facilities such as `std::thread`, `std::mutex`, `std::chrono`, `std::filesystem`, and `std::fstream`. POSIX/Linux APIs such as `fork`, `exec`, `waitpid`, `pipe`, `dup2`, `open`, `socket`, `ioctl`, and `epoll` come from the operating-system API. A C++ program on Linux can call them, but that does not make them C++ standard APIs.

**C/C++ code/API anchor:**

```cpp
#include <sys/types.h>
#include <unistd.h>

pid_t pid = fork();
```

**Production/debug angle:** Clarifying the API family matters for portability, build flags, error handling, and testing. POSIX code usually needs syscall-style error checks and `errno` context.

**Traps:** Saying "C++ has fork because I can call it from C++"; assuming a Windows or embedded target has the same API.

**Follow-ups:** What C++ standard API launches a process? Why might a C++ Linux service still use `fork`/`exec`?

### 2. Does the C++ standard library have sockets?

**Short answer:** No common C++ standard socket API exists. On Linux, C++ programs usually use POSIX sockets directly, Boost.Asio, or a project-specific wrapper.

**Deep explanation:** POSIX sockets are file-descriptor-based APIs: `socket`, `bind`, `listen`, `accept`, `connect`, `send`, and `recv`. C++ provides useful tools around them, such as RAII wrappers, `std::string`, `std::vector`, `std::chrono`, and exceptions or `std::error_code`, but the socket operations themselves are not standardized by C++.

**C/C++ code/API anchor:**

```cpp
#include <sys/socket.h>

int fd = socket(AF_INET, SOCK_STREAM, 0);
```

**Production/debug angle:** Socket code must handle partial `send`/`recv`, nonblocking behavior, readiness APIs, and descriptor lifetime. A wrapper should not hide these realities.

**Traps:** Treating `std::iostream` as a socket API; assuming `std::filesystem` or `std::fstream` helps with sockets.

**Follow-ups:** When would you choose Boost.Asio? Why is a socket also commonly treated as a file descriptor on Linux?

### 3. Compare `pthread_create` and `std::thread`.

**Short answer:** `pthread_create` is the POSIX C API for creating threads. `std::thread` is the C++ standard-library thread object that can run functions, lambdas, functors, or member functions.

**Deep explanation:** `pthread_create` uses a C callback shape: `void* (*)(void*)` plus a `void*` argument. That means the programmer owns casting and lifetime discipline. `std::thread` is type-aware and works naturally with C++ callables, but it still represents a real OS thread and must be joined or detached before destruction.

**C/C++ code/API anchor:**

```cpp
#include <pthread.h>
#include <thread>

// POSIX: pthread_create(&tid, nullptr, start_routine, arg);
std::thread t([] { /* work */ });
t.join();
```

**Production/debug angle:** Prefer `std::thread` in C++ unless you need pthread-specific attributes, scheduling, or C ABI integration. Check lifetime of captured data either way.

**Traps:** Passing stack data to a pthread that outlives the stack frame; destroying a joinable `std::thread`; detaching a thread that still references an object being destroyed.

**Follow-ups:** What happens if a `std::thread` destructor runs while the thread is still joinable? When might pthread attributes matter?

### 4. Compare file descriptors and `std::fstream`.

**Short answer:** A file descriptor is a POSIX integer handle used with APIs like `open`, `read`, `write`, `close`, sockets, pipes, `poll`, and `epoll`. `std::fstream` is a C++ stream abstraction for file I/O.

**Deep explanation:** File descriptors are byte-oriented and integrate with OS facilities. They expose partial I/O, `errno`, nonblocking modes, descriptor inheritance, and readiness APIs. `std::fstream` is better for formatted or straightforward file I/O with RAII and stream state, but it does not replace descriptor-level APIs.

**C/C++ code/API anchor:**

```cpp
#include <fcntl.h>
#include <unistd.h>
#include <fstream>

int fd = open("data.txt", O_RDONLY);
std::ifstream file("data.txt");
```

**Production/debug angle:** Use descriptors for sockets, pipes, `dup2`, `ioctl`, `poll`, and `epoll`. Use streams for ordinary text/binary file work where formatting and C++ stream state help.

**Traps:** Forgetting to `close(fd)`; assuming `write` writes all bytes; using `std::fstream` where an API requires a descriptor.

**Follow-ups:** Why should a C++ wrapper around an fd be move-only? What does `O_CLOEXEC` protect against?

## Mid-Level Questions

### 5. Compare `pthread_mutex_lock/unlock` with `std::lock_guard`.

**Short answer:** `pthread_mutex_lock/unlock` are manual POSIX calls. `std::lock_guard` is a C++ RAII wrapper that unlocks automatically when the guard leaves scope.

**Deep explanation:** Manual lock/unlock code is fragile around early returns and exceptions. `std::lock_guard<std::mutex>` makes lock ownership scoped, which is the usual C++ pattern. If pthread-specific mutex attributes are needed, a custom RAII wrapper can provide the same safety around `pthread_mutex_t`.

**C/C++ code/API anchor:**

```cpp
#include <mutex>

std::mutex m;

void update() {
    std::lock_guard<std::mutex> lock(m);
    // protected work
}
```

**Production/debug angle:** RAII locking prevents leaked locks after exceptions and simplifies code review. Deadlocks still require lock ordering or multi-lock helpers such as `std::scoped_lock`.

**Traps:** Manually unlocking on only the success path; holding locks while calling unknown callbacks; locking multiple mutexes in inconsistent order.

**Follow-ups:** When do you need `std::unique_lock` instead of `std::lock_guard`? How would you wrap a `pthread_mutex_t` in C++?

### 6. Why must condition-variable waits use a predicate?

**Short answer:** Because condition variables can wake without the desired condition being true, and notifications can race with wait setup. A predicate rechecks the shared state.

**Deep explanation:** A condition variable is only a notification mechanism; the real condition lives in protected shared state. POSIX `pthread_cond_t` and C++ `std::condition_variable` both require the same mental model: lock the mutex, check the predicate, wait, then check again. C++ makes this convenient with predicate overloads.

**C/C++ code/API anchor:**

```cpp
#include <condition_variable>
#include <mutex>

std::condition_variable cv;
std::mutex m;
bool ready = false;

void wait_ready() {
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [] { return ready; });
}
```

**Production/debug angle:** Bugs here show up as hangs, rare races, or consumers reading empty queues. Debug by inspecting the protected predicate, notification order, and lock ownership.

**Traps:** Calling `cv.wait(lock)` without a loop/predicate; modifying the predicate without holding the mutex; notifying while still relying on stale state.

**Follow-ups:** What is a spurious wakeup? Why does `std::condition_variable` require `std::unique_lock`?

### 7. Compare `open/read/write/close` with C++ streams for error handling.

**Short answer:** POSIX calls return values and set `errno` on failure. C++ streams use stream state flags and can optionally throw exceptions.

**Deep explanation:** `read` and `write` operate on descriptors and can return partial counts, zero, or `-1` with `errno`. Streams abstract buffering and formatting, and the caller checks `is_open`, `fail`, `bad`, or enables exceptions. A wrapper should preserve the operation name, resource, return value, and error context.

**C/C++ code/API anchor:**

```cpp
#include <cerrno>
#include <unistd.h>

ssize_t n = write(fd, data, size);
if (n == -1) {
    int saved = errno;
    (void)saved;
}
```

**Production/debug angle:** Always distinguish EOF, temporary unavailability, signal interruption, partial success, and fatal errors. Use `strace` to see real syscalls.

**Traps:** Assuming `write` writes all bytes; reading `errno` after a successful call; dropping the path/fd from logs.

**Follow-ups:** How would you implement a `write_all` helper? When should a C++ wrapper throw vs return `std::error_code`?

### 8. Why wrap a file descriptor in RAII?

**Short answer:** To make descriptor ownership move-only and guarantee `close` on all exit paths.

**Deep explanation:** A raw `int fd` does not say who owns it. It can leak on early return or exception, and copying it can cause double-close. A C++ RAII wrapper owns exactly one descriptor, closes in the destructor, disables copying, and supports moving.

**C/C++ code/API anchor:**

```cpp
class Fd {
public:
    explicit Fd(int fd = -1) : fd_(fd) {}
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
    ~Fd() { if (fd_ >= 0) close(fd_); }

private:
    int fd_;
};
```

**Production/debug angle:** FD leaks are visible through `/proc/<pid>/fd`, `lsof`, or `strace`. A move-only wrapper makes many leaks and double-closes unrepresentable.

**Traps:** Letting the compiler generate copy operations; retrying `close` blindly; not handling `-1` as "no descriptor".

**Follow-ups:** How would you implement move constructor and move assignment? Why can `close` error handling be subtle?

### 9. Compare `volatile` with `std::atomic`.

**Short answer:** `volatile` is not a thread synchronization primitive. `std::atomic` provides atomicity and inter-thread synchronization semantics.

**Deep explanation:** `volatile` affects certain compiler optimizations around object access, but it does not make compound operations atomic and does not create a happens-before relationship between threads. Use mutexes for compound invariants and `std::atomic` for simple flags/counters when appropriate.

**C/C++ code/API anchor:**

```cpp
#include <atomic>

std::atomic<bool> stop{false};

void request_stop() {
    stop.store(true, std::memory_order_relaxed);
}
```

**Production/debug angle:** Use ThreadSanitizer to catch data races. If the code needs multiple fields to be consistent together, a mutex is usually clearer than several atomics.

**Traps:** Replacing every shared variable with `volatile`; using relaxed atomics without understanding the ordering need; mixing atomic and non-atomic access to the same object.

**Follow-ups:** When is `memory_order_relaxed` enough? Why might a mutex be better than atomics?

### 10. Compare `sleep/usleep/nanosleep` with `std::this_thread::sleep_for`.

**Short answer:** POSIX sleep APIs use raw time units and can have signal-interruption behavior. `std::this_thread::sleep_for` uses type-safe `std::chrono` durations.

**Deep explanation:** Raw integer time APIs invite unit bugs such as confusing microseconds and milliseconds. `std::chrono` encodes units in the type. POSIX APIs remain useful when exact POSIX signal/restart behavior or specific OS contracts are needed.

**C/C++ code/API anchor:**

```cpp
#include <chrono>
#include <thread>

std::this_thread::sleep_for(std::chrono::milliseconds(100));
```

**Production/debug angle:** For measuring intervals, prefer monotonic clocks such as `std::chrono::steady_clock` or POSIX monotonic clocks, not wall-clock time that can jump.

**Traps:** Using `sleep(1)` in tests and making them slow/flaky; confusing `usleep` units; using wall-clock time for timeout intervals.

**Follow-ups:** What is the difference between wall-clock and monotonic time? Why is `usleep` often avoided in new code?

## Senior Questions

### 11. Design a safe C++ wrapper around POSIX descriptors.

**Short answer:** Use a move-only RAII type that owns one descriptor, closes it in the destructor, exposes explicit `get`/`release` operations, and preserves errors at API boundaries.

**Deep explanation:** POSIX descriptors are small integers with ownership rules outside the type system. A C++ wrapper should prevent accidental copying, support move transfer, handle invalid `-1`, and make ownership transitions explicit. It should also avoid pretending descriptor I/O is always complete or exception-free.

**C/C++ code/API anchor:**

```cpp
#include <unistd.h>

class Fd {
public:
    explicit Fd(int fd = -1) : fd_(fd) {}
    Fd(Fd&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
    Fd(const Fd&) = delete;
    ~Fd() { if (fd_ >= 0) close(fd_); }
    int get() const { return fd_; }

private:
    int fd_;
};
```

**Production/debug angle:** Decide whether operations throw, return `std::error_code`, or return a Result type. Log operation name, fd/path, return value, and `errno`.

**Traps:** Copyable fd wrapper; destructor throwing; forgetting close-on-exec policy; hiding partial I/O.

**Follow-ups:** Should the destructor report `close` errors? How would you handle `dup` or ownership release?

### 12. How would you launch a child process with redirected output?

**Short answer:** Use POSIX APIs such as `pipe`, `fork`, `dup2`, `exec`, and `waitpid`; C++ has no direct standard process API.

**Deep explanation:** The parent creates a pipe, forks, the child redirects one pipe end onto `STDOUT_FILENO` with `dup2`, closes unused descriptors, then calls `exec`. The parent closes its unused ends, reads output, and reaps the child with `waitpid`. C++ should wrap descriptors and process status to avoid leaks.

**C/C++ code/API anchor:**

```cpp
#include <sys/wait.h>
#include <unistd.h>

int pipefd[2];
pipe(pipefd);
pid_t pid = fork();
// child: dup2(pipefd[1], STDOUT_FILENO); exec...
// parent: read(pipefd[0], ...); waitpid(pid, &status, 0);
```

**Production/debug angle:** This code is descriptor-lifetime heavy. Use `strace` to verify `pipe`, `dup2`, `close`, `exec`, and `waitpid` order.

**Traps:** Not closing unused pipe ends; leaking descriptors into the child; calling complex C++ code between `fork` and `exec` in a multithreaded process.

**Follow-ups:** Why does `O_CLOEXEC` matter? What must the child do if `exec` fails?

### 13. How should a multithreaded Linux service handle `SIGTERM` safely?

**Short answer:** Install a POSIX `sigaction` handler that does only async-signal-safe work, such as setting a `volatile sig_atomic_t` flag or writing to a pipe. Let normal threads perform cleanup.

**Deep explanation:** Signal handlers interrupt execution and cannot safely call most C++ library code, including iostreams, allocation, mutex locks, or exceptions. The handler should request shutdown, and the main loop or event loop should observe that request and coordinate thread joins, resource cleanup, and descriptor closure.

**C/C++ code/API anchor:**

```cpp
#include <csignal>

volatile sig_atomic_t stop_requested = 0;

extern "C" void on_signal(int) {
    stop_requested = 1;
}
```

**Production/debug angle:** Audit the handler body against async-signal-safe rules. Debug shutdown hangs by checking signal masks, blocked threads, join order, and locks held during callbacks.

**Traps:** Calling `std::cout`, `new`, `delete`, `std::mutex::lock`, or throwing from a signal handler; trying to catch `SIGKILL`; doing full cleanup inside the handler.

**Follow-ups:** Why might a self-pipe be useful? When can a lock-free `std::atomic` flag be acceptable?

### 14. Compare `select`, `poll`, and `epoll` at a design level.

**Short answer:** They are readiness/event APIs for descriptors. `select` is older and limited, `poll` improves descriptor-list handling, and `epoll` is Linux-specific and designed for scalable event loops.

**Deep explanation:** None of these is a C++ standard API. They operate on file descriptors, which includes sockets, pipes, and other OS resources. C++ code often wraps them behind an event-loop abstraction or uses a library such as Boost.Asio. The wrapper must still expose readiness, nonblocking I/O, and partial operations correctly.

**C/C++ code/API anchor:**

```cpp
#include <poll.h>

pollfd pfd{};
pfd.fd = fd;
pfd.events = POLLIN;
int n = poll(&pfd, 1, 1000);
```

**Production/debug angle:** Use `strace` and logging around fd registration, readiness events, and retry loops. Watch for busy loops when errors or EOF are mishandled.

**Traps:** Assuming readiness means the entire message is available; forgetting nonblocking mode policy; treating `epoll` as portable.

**Follow-ups:** Why does C++ not standardize `epoll`? What would your event-loop wrapper expose?

### 15. When should C++ code still use pthreads directly?

**Short answer:** Use pthreads directly when you need POSIX-specific features, integration with a C API, existing pthread-based infrastructure, or platform-specific thread attributes not exposed by `std::thread`.

**Deep explanation:** `std::thread` is the default C++ abstraction for portable thread creation, but pthreads expose POSIX-specific controls and C ABI compatibility. In mixed codebases, you may need both: pthread APIs at the boundary and C++ RAII/type-safe wrappers internally.

**C/C++ code/API anchor:**

```cpp
#include <pthread.h>

pthread_t tid{};
// pthread_create(&tid, &attrs, start_routine, arg);
```

**Production/debug angle:** Make one ownership policy: who creates, who joins/detaches, what state the thread may access, and how shutdown happens. Avoid mixing raw pthread lifecycle management with C++ exceptions without wrappers.

**Traps:** Assuming pthread and `std::thread` lifetimes are interchangeable; ignoring pthread return codes; losing type safety through `void*` arguments.

**Follow-ups:** How would you wrap `pthread_t` in C++? What pthread attributes might a system program care about?

### 16. How do you choose between exceptions, `errno`, `std::error_code`, and Result-style APIs for POSIX wrappers?

**Short answer:** POSIX calls use return values and `errno`. A C++ wrapper can translate to exceptions, `std::error_code`, or Result-style values, but it must preserve operation context and boundary policy.

**Deep explanation:** Exceptions are convenient for rare failures and RAII cleanup, but not every codebase allows them. `std::error_code` and Result-style APIs make failure explicit and are often better for expected operational failures such as timeout, EOF, or would-block. At C/POSIX boundaries, never lose `errno`, the operation name, and the relevant path/fd.

**C/C++ code/API anchor:**

```cpp
#include <cerrno>
#include <system_error>

if (fd == -1) {
    throw std::system_error(errno, std::generic_category(), "open");
}
```

**Production/debug angle:** Good error design makes logs actionable: operation, resource, errno value/category, and whether retry is allowed.

**Traps:** Throwing from destructors; reading stale `errno`; mapping every POSIX failure to one vague exception message.

**Follow-ups:** Which failures should be ordinary return values? How would you represent EOF vs EAGAIN vs fatal error?

## Coding Tasks

### Task 1. Write a move-only RAII fd wrapper.

**Prompt:** Implement a minimal `Fd` class that closes in the destructor and cannot double-close after moves.

**Short answer:** Store the descriptor, delete copy operations, implement move operations, and set moved-from objects to `-1`.

**Deep explanation:** The wrapper encodes descriptor ownership. Copying would create two owners for one descriptor, so only moving is allowed.

**C/C++ code/API anchor:**

```cpp
class Fd {
public:
    explicit Fd(int fd = -1) : fd_(fd) {}
    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;
    Fd(Fd&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
    ~Fd() { if (fd_ >= 0) close(fd_); }
private:
    int fd_;
};
```

**Production/debug angle:** Extend with move assignment, `get`, `release`, and close-error policy. Use `/proc/<pid>/fd` or `lsof` to inspect leaks.

**Traps:** Generated copy constructor; throwing from destructor; closing `-1`.

**Follow-ups:** How would you implement move assignment safely? Should `release()` close the fd?

### Task 2. Convert `pthread_create` with `void* arg` to `std::thread`.

**Prompt:** A pthread routine receives a pointer to a config struct. Rewrite the idea using `std::thread`.

**Short answer:** Prefer a lambda or callable object that captures owned or safely referenced state.

**Deep explanation:** `std::thread` avoids manual `void*` casting and lets C++ types travel through the callable interface. Lifetime still matters: reference captures must not outlive the referenced object.

**C/C++ code/API anchor:**

```cpp
Config cfg{/*...*/};
std::thread t([cfg] {
    run_worker(cfg);
});
t.join();
```

**Production/debug angle:** Capture by value for simple immutable config; use shared ownership or explicit shutdown for long-lived worker state.

**Traps:** Capturing stack objects by reference and detaching; forgetting to join; swallowing exceptions inside the thread.

**Follow-ups:** How would you report exceptions from the worker? When would `std::async` be simpler?

### Task 3. Fix a condition-variable wait bug.

**Prompt:**

```cpp
std::unique_lock<std::mutex> lock(m);
cv.wait(lock);
consume(queue.front());
```

**Short answer:** Wait with a predicate that verifies the queue is not empty or shutdown was requested.

**Deep explanation:** A wait can wake spuriously or due to an unrelated notification. The consumer must check shared state while holding the mutex.

**C/C++ code/API anchor:**

```cpp
cv.wait(lock, [&] {
    return !queue.empty() || stopped;
});
```

**Production/debug angle:** This prevents rare empty-queue crashes and shutdown hangs in producer-consumer code.

**Traps:** Checking `queue.empty()` outside the lock; forgetting a shutdown predicate; notifying without changing state.

**Follow-ups:** Should `consume` run while holding the mutex? Why or why not?

### Task 4. Implement a partial-write loop.

**Prompt:** `write(fd, buf, len)` may write fewer than `len` bytes. Sketch a safe loop.

**Short answer:** Loop until all bytes are written or a real error/temporary condition is handled by policy.

**Deep explanation:** POSIX byte I/O can complete partially. Production code must advance the buffer pointer by the returned count and handle `EINTR`, `EAGAIN`, or fatal errors intentionally.

**C/C++ code/API anchor:**

```cpp
while (sent < len) {
    ssize_t n = write(fd, data + sent, len - sent);
    if (n > 0) {
        sent += static_cast<size_t>(n);
    } else {
        // handle errno / EOF-like policy
    }
}
```

**Production/debug angle:** Log partial counts and `errno`; use nonblocking policy consistently with `poll`/`epoll`.

**Traps:** Treating one successful `write` as complete; ignoring `EINTR`; spinning on `EAGAIN`.

**Follow-ups:** How does this change for nonblocking sockets? What about `send` flags?

### Task 5. Build a signal-safe shutdown request.

**Prompt:** Show a safe signal handler for `SIGTERM`.

**Short answer:** Register with `sigaction`; in the handler, set a `sig_atomic_t` flag or write to a pipe. Do cleanup later.

**Deep explanation:** Signal handlers cannot safely call most C++ runtime/library operations. They should only request shutdown.

**C/C++ code/API anchor:**

```cpp
volatile sig_atomic_t stop_requested = 0;

extern "C" void handle_term(int) {
    stop_requested = 1;
}
```

**Production/debug angle:** The main loop observes the flag, stops accepting work, wakes threads, joins workers, and closes descriptors through RAII.

**Traps:** Using `std::cout` or locking a mutex in the handler; trying to handle `SIGKILL`; throwing from the handler.

**Follow-ups:** How would a self-pipe integrate with `poll` or `epoll`?

## Debugging Scenarios

### Scenario 1. A service slowly leaks file descriptors.

**Short answer:** Look for missing `close`, early returns, exceptions before cleanup, descriptor copies, and missing close-on-exec policy.

**Deep explanation:** Raw descriptors are not self-describing owners. A leak can come from failure paths, copied wrappers, child process inheritance, or forgetting to close unused pipe/socket ends.

**C/C++ code/API anchor:** Inspect `open`, `socket`, `accept`, `pipe`, `dup`, `dup2`, and `close` call sites.

**Production/debug angle:** Use `lsof`, `/proc/<pid>/fd`, and `strace` to identify which descriptors remain open and where they were created.

**Traps:** Only checking the success path; forgetting descriptors inherited through `exec`; making fd wrappers copyable.

**Follow-ups:** How would a move-only RAII wrapper prevent this class of bug?

### Scenario 2. Program terminates when a `std::thread` object is destroyed.

**Short answer:** The `std::thread` was still joinable at destruction, which calls `std::terminate`.

**Deep explanation:** C++ requires explicit lifecycle decision: join or detach. This is similar to pthread lifecycle management but enforced by the `std::thread` destructor.

**C/C++ code/API anchor:** Check `joinable()`, `join()`, `detach()`, RAII joiners, or C++20 `std::jthread`.

**Production/debug angle:** Audit all exception paths after thread creation. A thrown exception before `join` can terminate the program unless a guard owns the join.

**Traps:** Assuming destructor joins automatically; detaching to "fix" terminate while creating lifetime bugs.

**Follow-ups:** How would you write a scope guard for joining? Why might `std::jthread` help?

### Scenario 3. A producer-consumer queue sometimes hangs.

**Short answer:** Suspect missing predicate logic, missed notification, shutdown state not included in the wait condition, or a lock-order bug.

**Deep explanation:** Condition variables do not store events. If the shared predicate does not represent both data availability and shutdown, consumers can wait forever.

**C/C++ code/API anchor:** Check `std::condition_variable::wait(lock, predicate)` or the POSIX equivalent predicate loop.

**Production/debug angle:** Use thread backtraces, logging around predicate changes, and ThreadSanitizer for data races on queue state.

**Traps:** Notifying before changing state; changing state without holding the mutex; waiting without a shutdown condition.

**Follow-ups:** Should `notify_one` happen inside or outside the lock? What tradeoff does that choice have?

### Scenario 4. `SIGTERM` handler deadlocks in production.

**Short answer:** The handler likely called a non-async-signal-safe function such as logging, allocation, or mutex locking.

**Deep explanation:** A signal can interrupt code while it already holds a lock or library internal state. Calling into the same library from the handler can deadlock or corrupt state.

**C/C++ code/API anchor:** Audit handler bodies registered through `signal` or `sigaction`.

**Production/debug angle:** Keep handlers minimal; use a flag or self-pipe. Debug by checking signal masks, thread backtraces, and handler code paths.

**Traps:** Using iostreams in handlers; cleaning up all resources inside the handler; believing `SIGKILL` can be caught.

**Follow-ups:** Which operations are acceptable inside a signal handler? How does a self-pipe avoid this problem?

