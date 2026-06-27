# 16 - POSIX/Linux C API Vs Modern C++

## 1. Goal

After this lesson, you should be able to compare POSIX/Linux user-space C APIs
with Modern C++ facilities in a practical engineering way:

- choose between `pthread_create` and `std::thread`;
- choose between `pthread_mutex_t` and `std::mutex`;
- use `std::lock_guard`, `std::unique_lock`, and `std::condition_variable`
  instead of fragile manual lock/unlock code;
- understand when C++ has no standard replacement for Linux APIs such as
  `fork`, `exec`, `pipe`, `dup2`, `socket`, `epoll`, and `ioctl`;
- wrap file descriptors, sockets, and other POSIX handles with RAII;
- translate POSIX return-code/`errno` errors into C++ APIs without losing
  debugging context;
- avoid common bugs around thread lifetime, partial I/O, signals, `volatile`,
  deadlocks, and resource leaks.

The goal is not "C++ replaces POSIX." The real goal is to know where Modern C++
gives safer standard abstractions and where C++ code still needs POSIX/Linux
APIs behind careful wrappers.

## 2. Why It Matters

Linux software is often mixed by nature:

- low-level services use `open`, `read`, `write`, `close`, `socket`, `poll`,
  and `epoll`;
- process launchers use `fork`, `exec`, `waitpid`, `pipe`, and `dup2`;
- older libraries expose `pthread_*` APIs;
- C++ application code wants RAII, exceptions or `std::error_code`, lambdas,
  standard containers, `std::thread`, `std::chrono`, and `std::filesystem`.

Many production bugs happen at the boundary:

- a file descriptor leaks on an early return;
- a detached thread uses an object that has already been destroyed;
- `pthread_create` receives a pointer to stack data whose lifetime is too
  short;
- `read`, `write`, `send`, or `recv` is assumed to transfer the whole buffer;
- a signal handler calls a function that is not async-signal-safe;
- `volatile` is used as if it fixed a data race;
- a C++ exception crosses a C callback or POSIX thread boundary.

Strong Linux C++ code usually keeps the operating-system boundary explicit, then
wraps ownership and invariants in C++ types.

## 3. Mental Model

Think in three layers.

Layer 1 is the Linux user-space system API. It exposes C functions, integer
file descriptors, `pthread_t`, `pid_t`, `errno`, raw buffers, and flags. This is
where you use headers such as `<unistd.h>`, `<fcntl.h>`, `<pthread.h>`,
`<sys/socket.h>`, `<sys/wait.h>`, `<poll.h>`, `<signal.h>`, and `<time.h>`.

Layer 2 is the C++ standard library. It gives portable abstractions such as
`std::thread`, `std::mutex`, `std::lock_guard`, `std::condition_variable`,
`std::atomic`, `std::chrono`, `std::filesystem`, `std::fstream`, `std::future`,
and `std::async`.

Layer 3 is your application policy. This is where you decide ownership,
lifetime, shutdown, retry rules, logging, and error reporting. A good C++
program often uses POSIX/Linux at layer 1, but exposes layer-3 code through
RAII classes and type-safe interfaces.

The key boundary rule:

- use Modern C++ when it provides the abstraction you need;
- use POSIX/Linux APIs when you need exact Linux behavior;
- wrap owning POSIX resources in C++ RAII as soon as possible.

## 4. Mechanism

### Resource Ownership

POSIX APIs often return handles that must be closed or waited for manually:

- file descriptor: `close(fd)`;
- socket descriptor: `close(sock)`;
- `FILE*`: `fclose(file)`;
- process child: `wait` or `waitpid`;
- pthread mutex/condition variable/semaphore: destroy when appropriate.

C++ RAII ties cleanup to object lifetime. This is especially important when code
has multiple return paths or exceptions.

```cpp
#include <unistd.h>

class Fd {
public:
    explicit Fd(int fd = -1) noexcept : fd_(fd) {}

    ~Fd() {
        if (fd_ >= 0) {
            ::close(fd_);
        }
    }

    Fd(const Fd&) = delete;
    Fd& operator=(const Fd&) = delete;

    Fd(Fd&& other) noexcept : fd_(other.fd_) {
        other.fd_ = -1;
    }

    Fd& operator=(Fd&& other) noexcept {
        if (this != &other) {
            if (fd_ >= 0) {
                ::close(fd_);
            }
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    int get() const noexcept { return fd_; }
    explicit operator bool() const noexcept { return fd_ >= 0; }

private:
    int fd_;
};
```

The wrapper is move-only because two owners closing the same descriptor would be
a bug.

### Error Handling

POSIX APIs usually report failure with a return value and set `errno`.

```cpp
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>

int fd = ::open("data.txt", O_RDONLY);
if (fd == -1) {
    std::cerr << "open failed: " << std::strerror(errno) << '\n';
}
```

C++ wrappers can expose failures as exceptions, `std::error_code`, or a
Result-style type. The important rule is to preserve operation context and the
original error value.

### Thread Lifetime

`std::thread` improves type safety, but it still requires explicit lifetime
management. If a joinable `std::thread` object is destroyed, the program calls
`std::terminate`.

```cpp
#include <iostream>
#include <thread>

int main() {
    std::thread worker([] {
        std::cout << "work\n";
    });

    worker.join();
}
```

Always join, detach, or use an RAII thread owner. Avoid `detach` unless object
lifetime and shutdown are designed carefully.

### Synchronization

Manual locking is fragile:

```cpp
pthread_mutex_lock(&mutex);
// if this code returns early, the mutex may stay locked
pthread_mutex_unlock(&mutex);
```

Scoped locking is safer:

```cpp
#include <mutex>

std::mutex mutex;

void update() {
    std::lock_guard<std::mutex> lock(mutex);
    // protected work
}
```

The destructor unlocks the mutex even when the function returns early or throws.

### Condition Variables

Condition variables must wait for a condition, not merely a notification.
Spurious wakeups are allowed.

```cpp
#include <condition_variable>
#include <mutex>
#include <queue>

std::mutex mutex;
std::condition_variable cv;
std::queue<int> queue;

int pop() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [] { return !queue.empty(); });

    int value = queue.front();
    queue.pop();
    return value;
}
```

The predicate is the real condition. The notification is only a wakeup signal.

### Time

POSIX time APIs often use integer units or `timespec`. C++ uses type-safe
durations and clocks.

```cpp
#include <chrono>
#include <thread>

std::this_thread::sleep_for(std::chrono::milliseconds(100));
```

Use `std::chrono` in C++ code when possible. Use POSIX clocks when you need
exact Linux clock behavior or interoperation with POSIX APIs.

### Signals

Signals are not normal callbacks. A signal handler can interrupt code at almost
any point, so it must do very little.

```cpp
#include <csignal>

volatile sig_atomic_t stop_requested = 0;

extern "C" void on_signal(int) {
    stop_requested = 1;
}

int main() {
    std::signal(SIGINT, on_signal);

    while (!stop_requested) {
        // main loop
    }
}
```

For POSIX programs, prefer `sigaction` for precise signal registration. Keep the
handler tiny: set a `sig_atomic_t` flag or perform another async-signal-safe
operation. Do not log with iostreams, allocate memory, lock a mutex, or throw
from a signal handler.

## 5. C/POSIX/Linux API And Code

### POSIX Thread Creation

`pthread_create` takes a C function pointer and a `void*` context.

```cpp
#include <pthread.h>
#include <iostream>

struct Work {
    int value;
};

void* run(void* arg) {
    auto* work = static_cast<Work*>(arg);
    std::cout << work->value << '\n';
    return nullptr;
}

int main() {
    pthread_t thread{};
    Work work{42};

    if (pthread_create(&thread, nullptr, run, &work) != 0) {
        return 1;
    }

    pthread_join(thread, nullptr);
}
```

This is learning-only code. It is correct here because `work` stays alive until
after `pthread_join`. It would be unsafe to pass a pointer to data that can be
destroyed before the thread finishes.

### Descriptor I/O

`open`, `read`, `write`, and `close` operate on integer file descriptors.

```cpp
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

int main() {
    int fd = ::open("example.txt", O_RDONLY);
    if (fd == -1) {
        std::cerr << "open: " << std::strerror(errno) << '\n';
        return 1;
    }

    char buffer[128]{};
    ssize_t n = ::read(fd, buffer, sizeof(buffer));
    if (n == -1) {
        std::cerr << "read: " << std::strerror(errno) << '\n';
        ::close(fd);
        return 1;
    }

    ssize_t out = ::write(STDOUT_FILENO, buffer, static_cast<size_t>(n));
    if (out == -1) {
        std::cerr << "write: " << std::strerror(errno) << '\n';
        ::close(fd);
        return 1;
    }

    ::close(fd);
}
```

This tiny example checks for failure. Production code should wrap `fd` in RAII
and handle partial `write` results.

### Partial Write Loop

`write` can write fewer bytes than requested. Robust code loops until all bytes
are written or an unrecoverable error occurs.

```cpp
#include <cerrno>
#include <cstddef>
#include <unistd.h>

bool write_all(int fd, const char* data, std::size_t size) {
    std::size_t written = 0;

    while (written < size) {
        ssize_t n = ::write(fd, data + written, size - written);
        if (n > 0) {
            written += static_cast<std::size_t>(n);
            continue;
        }

        if (n == -1 && errno == EINTR) {
            continue;
        }

        return false;
    }

    return true;
}
```

This same idea applies to `send` and often to `read`/`recv` protocols that need
a fixed number of bytes.

## 6. Modern C++ API And Code

### `std::thread` With Lambda Capture

```cpp
#include <iostream>
#include <thread>

int main() {
    int value = 42;

    std::thread worker([value] {
        std::cout << value << '\n';
    });

    worker.join();
}
```

The lambda capture is type-safe. Capture by value when the thread may outlive
the current scope. Capture by reference only when lifetime is guaranteed.

### `std::mutex` And `std::lock_guard`

```cpp
#include <mutex>
#include <vector>

class SafeLog {
public:
    void add(int value) {
        std::lock_guard<std::mutex> lock(mutex_);
        values_.push_back(value);
    }

private:
    std::mutex mutex_;
    std::vector<int> values_;
};
```

This is exception-safe with respect to unlocking: if `push_back` throws, the
lock is still released.

### `std::filesystem`

`std::filesystem` handles paths and metadata. It does not replace descriptor
I/O, sockets, `epoll`, or `ioctl`.

```cpp
#include <filesystem>
#include <iostream>

int main() {
    std::filesystem::path path{"example.txt"};

    if (std::filesystem::exists(path)) {
        std::cout << path << " has "
                  << std::filesystem::file_size(path)
                  << " bytes\n";
    }
}
```

Use `std::filesystem` for portable path work. Use POSIX descriptors when you
need flags such as `O_CLOEXEC`, descriptor passing, `poll`, `epoll`, or
descriptor-level control.

### Futures And Async Work

`std::future`, `std::promise`, `std::async`, and `std::packaged_task` can model
one-shot asynchronous results.

```cpp
#include <future>
#include <iostream>

int compute() {
    return 42;
}

int main() {
    std::future<int> result = std::async(std::launch::async, compute);
    std::cout << result.get() << '\n';
}
```

Use this for result-oriented asynchronous work. It is not a replacement for
Linux process management, sockets, or event loops.

## 7. Practical Usage

Use POSIX/Linux APIs directly when you need:

- process control: `fork`, `exec`, `wait`, `waitpid`;
- pipe and redirection mechanics: `pipe`, `dup2`, descriptor inheritance;
- descriptor-level I/O: `open`, `read`, `write`, `close`, `fcntl`;
- socket APIs: `socket`, `bind`, `listen`, `accept`, `connect`, `send`, `recv`;
- event loops: `select`, `poll`, `epoll`;
- device or descriptor control: `ioctl`;
- exact signal behavior: `sigaction`, signal masks, async-signal-safe design;
- exact Linux clock behavior: `clock_gettime`, `nanosleep`.

Use Modern C++ when it gives a good standard abstraction:

- `std::thread` for ordinary C++ thread creation;
- `std::mutex`, `std::lock_guard`, `std::unique_lock`, `std::scoped_lock` for
  scoped synchronization;
- `std::condition_variable` for predicate-based waiting;
- `std::atomic` for simple atomic state and counters;
- `std::chrono` for type-safe durations and clocks;
- `std::filesystem` for paths and metadata;
- `std::fstream` for portable stream-style file I/O;
- `std::future`, `std::promise`, and `std::async` for one-shot async results.

Use a wrapper when a POSIX resource crosses into application code. The wrapper
should define:

- who owns the resource;
- whether it can be copied or only moved;
- how cleanup happens;
- what errors look like;
- whether methods are thread-safe;
- whether operations can block;
- how shutdown is requested.

## 8. Comparisons

### Threading

| Topic | C/POSIX/Linux | Modern C++ | Enterprise Usage |
| --- | --- | --- | --- |
| Create a thread | `pthread_create(&thread, attr, start, void_arg)` uses a C callback and `void*` context. | `std::thread(callable, args...)` accepts functions, lambdas, functors, and member functions. | Prefer `std::thread` for C++ code; use pthreads when you need pthread attributes, scheduling, or C-library interop. |
| Join | `pthread_join(thread, &result)` waits for a `pthread_t`. | `thread.join()` waits through the owning object. | Every joinable thread needs a clear join point or a designed detach policy. |
| Detach | `pthread_detach(thread)` releases thread resources when it exits. | `thread.detach()` lets the thread run independently. | Avoid detached threads unless shutdown and object lifetime are explicit. |
| Context passing | `void* arg` requires casts and manual lifetime discipline. | Lambda captures and callable objects are type-safe. | Prefer value captures or shared ownership when lifetime is not obvious. |

### Synchronization

| Topic | C/POSIX/Linux | Modern C++ | Enterprise Usage |
| --- | --- | --- | --- |
| Mutex object | `pthread_mutex_t` with init/destroy and pthread attributes. | `std::mutex` as a C++ object. | Prefer `std::mutex` unless pthread-specific attributes are required. |
| Locking | `pthread_mutex_lock` and `pthread_mutex_unlock` are manual. | `std::lock_guard` and `std::unique_lock` unlock in destructors. | Use scoped locks for exception safety and early-return safety. |
| Multiple locks | Manual ordering is required. | `std::scoped_lock` can lock multiple mutexes without simple ordering mistakes. | Still define lock-order policies for larger systems. |
| Condition variable | `pthread_cond_t` waits with a pthread mutex. | `std::condition_variable` waits with `std::unique_lock<std::mutex>`. | Always wait with a predicate; notifications are not the condition. |
| Semaphore | `sem_t` uses POSIX init/wait/post/destroy. | `std::counting_semaphore` is available in C++20. | Use semaphores for counted permits; use condition variables for state predicates. |

### Process

| Topic | C/POSIX/Linux | Modern C++ | Enterprise Usage |
| --- | --- | --- | --- |
| Create process | `fork` duplicates the current process. | No direct C++ standard process API. | Use POSIX/Linux or a well-designed library wrapper. |
| Replace image | `exec` family loads a new program. | No direct C++ standard equivalent. | Build argument/environment arrays carefully and report exec failures. |
| Wait for child | `wait` and `waitpid` collect status. | No direct C++ standard equivalent. | Avoid unreaped child processes; decode exit status intentionally. |
| Redirection | `pipe` and `dup2` connect descriptors before `exec`. | C++ streams do not model process descriptor inheritance. | Wrap descriptors and close unused ends in both parent and child paths. |

### File I/O

| Topic | C/POSIX/Linux | Modern C++ | Enterprise Usage |
| --- | --- | --- | --- |
| Descriptor I/O | `open`, `read`, `write`, `close` operate on `int` file descriptors. | No direct descriptor wrapper in the C++ standard library. | Use descriptor I/O for flags, nonblocking mode, polling, inheritance, and low-level integration; wrap with RAII. |
| C file API | `fopen`, `fread`, `fwrite`, `fclose` operate on `FILE*`. | `std::fstream`, `std::ifstream`, `std::ofstream` provide stream-style file I/O. | Prefer C++ streams for portable file content work; use POSIX descriptors for OS-level behavior. |
| Paths and metadata | POSIX path functions vary by operation. | `std::filesystem` handles paths, existence, size, iteration, and metadata. | Use `std::filesystem` for path logic, not as a replacement for descriptor I/O. |
| Control operations | `ioctl` sends device/descriptor-specific control commands. | No C++ standard equivalent. | Keep `ioctl` at a narrow boundary and document request codes and ownership rules. |

### Socket And Event Loop

| Topic | C/POSIX/Linux | Modern C++ | Enterprise Usage |
| --- | --- | --- | --- |
| Socket lifecycle | `socket`, `bind`, `listen`, `accept`, `connect`, `close`. | No common C++ standard socket API. | Use POSIX sockets directly or a library wrapper; sockets should be RAII-owned. |
| Data transfer | `send` and `recv` work with buffers and byte counts. | C++ containers can hold buffers but do not replace socket calls. | Handle partial transfer, `EINTR`, nonblocking errors, and protocol framing. |
| Multiplexing | `select`, `poll`, and Linux `epoll`. | No C++ standard event-loop API. | Use `epoll` or a library such as Boost.Asio when architecture benefits from it. |
| Address lookup | `getaddrinfo` returns linked results that must be freed. | No direct C++ standard equivalent. | Wrap `addrinfo*` cleanup and preserve error text for diagnostics. |

### Time

| Topic | C/POSIX/Linux | Modern C++ | Enterprise Usage |
| --- | --- | --- | --- |
| Sleep | `sleep`, `usleep`, `nanosleep` use seconds, microseconds, or `timespec`. | `std::this_thread::sleep_for` uses `std::chrono` durations. | Prefer `std::chrono` for type-safe application waits. |
| Clock | `clock_gettime` exposes specific POSIX clocks. | `std::chrono` exposes standard clocks and durations. | Use monotonic clocks for measuring intervals; avoid wall-clock time for timeouts. |
| Units | Manual integer units are easy to mix up. | `std::chrono::milliseconds`, `seconds`, and other durations encode units. | Convert explicitly at API boundaries. |

### Atomic And Volatile

| Topic | C/POSIX/Linux | Modern C++ | Enterprise Usage |
| --- | --- | --- | --- |
| Atomic object | C11 `_Atomic` and compiler builtins such as `__sync_*` / `__atomic_*`. | `std::atomic<T>` with standard memory ordering. | Prefer `std::atomic` in C++ code; use builtins only for constrained legacy or platform-specific code. |
| Shared flag | Plain `bool` or `volatile bool` does not make thread communication safe. | `std::atomic<bool>` provides race-free atomic access. | Use atomics for simple flags and mutexes for compound invariants. |
| Memory ordering | POSIX synchronization primitives provide ordering through locks and waits. | `std::memory_order` controls atomic ordering. | Use default sequential consistency until there is a measured reason and expert review. |

### Signals

| Topic | C/POSIX/Linux | Modern C++ | Enterprise Usage |
| --- | --- | --- | --- |
| Basic handler | `signal` and `raise` exist in C/C++. | C++ can call them, but normal C++ callbacks are not signal handlers. | Prefer `sigaction` in POSIX programs for precise behavior. |
| Robust registration | `sigaction` controls handler, mask, and flags. | No higher-level C++ standard signal wrapper for POSIX behavior. | Keep signal setup near process lifecycle code. |
| Handler work | Only async-signal-safe operations are allowed. | Throwing, locking, allocation, iostreams, and most library calls are unsafe in handlers. | Set a `volatile sig_atomic_t` flag or use a carefully designed notification mechanism. |

## 9. Common Bugs

- Treating POSIX APIs as "the C language" or assuming C++ replaces all POSIX.
- Forgetting to call `join` or `detach` before a `std::thread` destructor runs.
- Detaching a thread that later uses stack data or a destroyed object.
- Passing a pointer through `pthread_create` without guaranteeing lifetime.
- Calling `pthread_mutex_lock` and returning before `pthread_mutex_unlock`.
- Waiting on a condition variable without a predicate.
- Assuming `volatile` makes shared data race-free.
- Ignoring partial `read`, `write`, `send`, or `recv` results.
- Leaking file descriptors on early return, exceptions, or failed setup.
- Accidentally copying an owning file descriptor wrapper and double-closing.
- Using `std::fstream` where descriptor integration, nonblocking I/O, `poll`, or
  `epoll` is required.
- Calling non-async-signal-safe functions from a signal handler.
- Throwing C++ exceptions across C/POSIX callbacks.
- Measuring timeouts with wall-clock time instead of a monotonic clock.

## 10. Debugging

Compile with warnings and thread support:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -pthread main.cpp -o main
```

Use sanitizers for memory and undefined behavior:

```bash
g++ -std=c++17 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -pthread main.cpp -o main_asan
```

Use ThreadSanitizer for data races:

```bash
g++ -std=c++17 -g -O1 -fsanitize=thread -fno-omit-frame-pointer -pthread main.cpp -o main_tsan
```

Useful Linux debugging tools:

- `strace` to inspect system calls and failing `errno` values;
- `lsof` to find leaked open files and sockets;
- `/proc/<pid>/fd` to inspect live file descriptors;
- `gdb` to inspect crashes, deadlocks, and thread state.

When debugging this topic, check:

- does every thread have a join, detach, or RAII owner?
- does every lock have scoped ownership?
- do condition-variable waits use predicates?
- are all descriptor owners move-only?
- are partial I/O results handled?
- is `errno` read before another system/library call overwrites it?
- are signal handlers limited to async-signal-safe actions?
- are timeouts based on monotonic time where appropriate?

## 11. Best Practices

- Prefer C++ standard-library facilities when they express the same operation:
  `std::thread`, `std::mutex`, `std::lock_guard`,
  `std::condition_variable`, `std::atomic`, `std::chrono`,
  `std::filesystem`, and `std::fstream`.
- Use POSIX/Linux APIs when the operation is inherently OS-level: process
  management, descriptor I/O, sockets, event loops, `ioctl`, and exact signal
  behavior.
- Convert raw owning handles into move-only RAII wrappers immediately.
- Make wrapper copy/move behavior explicit. Owning descriptors should usually
  be move-only.
- Preserve `errno` or translate it into `std::error_code` before doing other
  work.
- Prefer scoped locks over manual lock/unlock pairs.
- Use condition-variable predicates.
- Use `std::atomic` for simple shared flags; use mutexes for compound state.
- Use `std::chrono` types instead of raw integer time units.
- Keep signal handlers tiny.
- Do not let C++ exceptions escape through C callbacks, POSIX thread entry
  points, or signal handlers.
- Document blocking behavior and shutdown behavior for wrappers around sockets,
  pipes, and threads.

## 12. Interview Readiness

A strong answer usually has this shape:

1. Name which side is POSIX/Linux and which side is C++ standard library.
2. Explain what C++ improves: type safety, RAII, scoped locking, safer lifetime
   expression, or portable abstraction.
3. Explain what C++ does not replace: process APIs, descriptor APIs, sockets,
   event loops, `ioctl`, and exact signal behavior.
4. Give the main production bug.
5. Give the debugging or best-practice response.

Example answer:

> `pthread_create` is a POSIX C API that starts a thread from a
> `void* (*)(void*)` function. `std::thread` is the C++ standard-library wrapper
> for creating a thread from a type-safe callable such as a lambda. In C++ I
> normally prefer `std::thread`, then use RAII or a clear join policy so the
> thread cannot be destroyed while joinable. I still use pthread APIs when I
> need pthread-specific attributes or C library interop.

Common interview traps:

- saying POSIX is the same thing as C;
- saying Modern C++ eliminates the need for Linux APIs;
- forgetting that `std::thread` destruction can terminate the program;
- forgetting that `std::filesystem` is not descriptor-level I/O;
- claiming `volatile` solves data races;
- ignoring partial socket or descriptor I/O;
- doing complex work inside a signal handler.

## 13. Practice

1. Write a move-only `Fd` wrapper with destructor, move constructor,
   move assignment, `get`, `release`, and `reset`.
2. Convert a small `pthread_create` example that passes `void*` context into a
   `std::thread` lambda version.
3. Write a producer-consumer queue using `std::mutex`,
   `std::condition_variable`, and a predicate.
4. Implement `write_all(int fd, std::span<const std::byte>)` or a C++17
   equivalent using pointer and size.
5. Write a tiny `fork` + `pipe` + `dup2` launcher that captures child stdout.
6. Replace a shared `volatile bool done` thread flag with `std::atomic<bool>`.
7. Install a `sigaction` handler that only requests shutdown, then let the main
   loop perform cleanup outside the handler.
8. Compare a `std::fstream` file copy with an `open`/`read`/`write` copy and
   explain when each version is appropriate.

## 14. Summary

Modern C++ makes Linux user-space programming safer when it is used to express
ownership, lifetime, synchronization, type-safe time, and error policy. POSIX
and Linux APIs still matter because the operating system exposes processes,
descriptors, sockets, event loops, signals, and many control operations through
C interfaces.

The professional pattern is simple: use the right POSIX/Linux API at the
boundary, then wrap ownership and invariants in small C++ types so ordinary
application code is harder to misuse.
