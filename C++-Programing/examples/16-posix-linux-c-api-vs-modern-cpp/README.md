# 16 - POSIX/Linux C API Vs Modern C++ Examples

These examples are Linux/POSIX user-space examples. They are intentionally
small and focused on comparison, ownership, shutdown, and debugging workflow.

## Files

| File | Purpose | Status |
| --- | --- | --- |
| `fd_raii.cpp` | Move-only RAII wrapper around `open`/`close`, plus a `write_all` loop | Production-style shape, simplified |
| `pthread_vs_thread.cpp` | `pthread_create` with `void*` context compared with `std::thread` lambda | Learning-only comparison |
| `condition_variable_queue.cpp` | `std::condition_variable` producer-consumer queue with a predicate | Production-style pattern, simplified |
| `signal_shutdown.cpp` | `sigaction` handler that only sets a `sig_atomic_t` shutdown flag | Production-style signal rule, simplified |
| `Makefile` | Build, run, sanitizer, ThreadSanitizer, and cleanup commands | Practical |

## Build

From this directory:

```sh
make
```

Build one example:

```sh
make fd_raii
make pthread_vs_thread
make condition_variable_queue
make signal_shutdown
```

## Run

```sh
make run
```

Or run one binary:

```sh
./fd_raii
./pthread_vs_thread
./condition_variable_queue
./signal_shutdown
```

## Sanitizer / Debug Commands

AddressSanitizer + UndefinedBehaviorSanitizer:

```sh
make sanitize
```

Build ThreadSanitizer binaries for thread-focused examples:

```sh
make tsan
```

Run ThreadSanitizer binaries when your local runtime supports it:

```sh
make run-tsan
```

Warnings as errors:

```sh
make strict
```

Clean generated files:

```sh
make clean
```

## Optional Linux Debug Commands

Trace syscalls for descriptor ownership and signal behavior:

```sh
strace -e openat,write,close ./fd_raii
strace -e rt_sigaction,kill ./signal_shutdown
```

Inspect file descriptors while adapting the examples:

```sh
ls -l /proc/$$/fd
```

## Safety Notes

- POSIX APIs such as `open`, `write`, `close`, `pthread_create`, `sigaction`,
  and `raise` are not ISO C++ APIs. C++ can call them on Linux, then wrap them.
- `fd_raii.cpp` is the preferred ownership shape: one move-only owner closes one
  descriptor. Do not make descriptor owners copyable.
- `pthread_vs_thread.cpp` is learning-only. It shows why `void*` arguments need
  lifetime discipline and why C++ lambdas are usually clearer inside C++ code.
- `condition_variable_queue.cpp` uses a predicate. Waiting without a predicate
  can hang or consume from an empty queue after a spurious wakeup.
- `signal_shutdown.cpp` keeps the handler tiny. Do not call `std::cout`,
  `malloc`, `new`, locks, or exceptions inside signal handlers.
- `write_all` handles partial writes and `EINTR`. Nonblocking descriptors also
  need an `EAGAIN`/`EWOULDBLOCK` policy, usually integrated with `poll` or
  `epoll`.
- ThreadSanitizer runtime support depends on the host toolchain and execution
  environment. If `make run-tsan` fails before executing program logic, treat it
  as an environment/toolchain issue and use the normal build plus code review.

## Practice Changes

1. Add move assignment to `Fd` and test move reassignment.
2. Change `pthread_vs_thread.cpp` to pass a stack object to a detached thread,
   explain the lifetime bug, then fix it.
3. Add a shutdown flag to `condition_variable_queue.cpp` and prove the consumer
   exits cleanly.
4. Extend `write_all` to return a structured error instead of throwing.
5. Replace the signal flag with a self-pipe and wait for it with `poll`.
