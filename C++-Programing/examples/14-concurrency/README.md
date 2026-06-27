# 14 - Concurrency Examples

These examples support the Concurrency lesson. They are intentionally small and
compile-oriented.

## Files

| File | Purpose | Status |
| --- | --- | --- |
| `race_counter.cpp` | Shows a broken shared counter with an intentional data race | Learning-only, intentionally undefined behavior |
| `mutex_atomic_counter.cpp` | Fixes shared counters with `std::mutex` and `std::atomic<int>` | Practical pattern for simple counters |
| `producer_consumer.cpp` | Minimal blocking queue with `std::condition_variable` and shutdown | Practical but minimal; add bounds/timeouts for production |
| `future_exception.cpp` | Propagates worker failure through `std::promise` / `std::future` | Practical pattern |
| `signal_shutdown.cpp` | Signal handler sets only a `sig_atomic_t` flag | Practical baseline, POSIX programs often prefer `sigaction()` |
| `scoped_lock_deadlock_fix.cpp` | Uses `std::scoped_lock` for two mutexes | Practical pattern |

## Build

From this directory:

```sh
make
```

Or build one file manually:

```sh
g++ -std=c++20 -Wall -Wextra -Wpedantic -O2 -pthread producer_consumer.cpp -o producer_consumer
./producer_consumer
```

`-pthread` is required on typical POSIX toolchains for C++ threading support.

## Run Safe Examples

```sh
make run-safe
```

This runs the examples that are meant to behave correctly:

- `mutex_atomic_counter`
- `producer_consumer`
- `future_exception`
- `scoped_lock_deadlock_fix`

Run signal shutdown manually:

```sh
./signal_shutdown
```

Stop it with `Ctrl+C` or from another terminal:

```sh
pkill -TERM signal_shutdown
```

## ThreadSanitizer

Build ThreadSanitizer variants:

```sh
make tsan
```

Run the intentionally broken race:

```sh
./race_counter_tsan
```

Expected lesson: ThreadSanitizer should report a data race. The source program
has undefined behavior by design.

Run fixed examples:

```sh
./mutex_atomic_counter_tsan
./producer_consumer_tsan
```

These should not report the same shared-counter or queue data race.

## AddressSanitizer And UBSan

Some concurrency-looking bugs are actually lifetime or undefined-behavior bugs.
Build sanitizer variants:

```sh
make asan
```

Run:

```sh
./producer_consumer_asan
./future_exception_asan
./signal_shutdown_asan
```

## Warnings And Review Notes

- `race_counter.cpp` is learning-only. It intentionally contains a C++ data
  race, which is undefined behavior.
- Do not use `volatile` to synchronize threads. Use `std::atomic`, a mutex, or a
  condition-variable protocol.
- `producer_consumer.cpp` is practical but minimal. Production queues usually
  need bounded capacity, backpressure, cancellation, metrics, and a documented
  post-close behavior.
- Do not hold locks while running callbacks, slow I/O, blocking filesystem or
  network calls, or task bodies.
- `future_exception.cpp` shows why thread/task boundaries should catch and
  propagate exceptions deliberately.
- `signal_shutdown.cpp` keeps the handler minimal. Do not call `std::cout`,
  `printf`, `new`, `delete`, `malloc`, `free`, or `mutex.lock()` inside a signal
  handler.
- `scoped_lock_deadlock_fix.cpp` shows the fix pattern, not a full deadlock lab.
  In real code, also document lock ownership and lock ordering.

## Clean

```sh
make clean
```

