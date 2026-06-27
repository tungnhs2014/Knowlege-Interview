# 14 - Concurrency

## 1. Goal

After this lesson, you should be able to write and review basic concurrent C++
code safely:

- create and manage threads with `std::thread`;
- protect shared state with `std::mutex` and RAII locks;
- coordinate threads with `std::condition_variable` and predicates;
- know when `std::atomic` is enough and when a mutex is required;
- compare Modern C++ primitives with POSIX `pthread` primitives;
- recognize common bugs such as data races, deadlocks, spurious wakeups,
  unsafe detached threads, and unsafe signal handlers.

The big rule for this chapter is simple:

> Correctness first, performance second.

A fast concurrent program that has a data race is not a fast program. It is a
bug that only looks fast until production timing exposes it.

## 2. Why It Matters

Concurrency appears whenever a program must do more than one thing at once:

- a server handles many clients;
- a UI stays responsive while work runs in the background;
- a logger accepts messages from many components;
- a sensor pipeline reads data, filters it, and writes output;
- a worker pool processes tasks from a queue;
- a service shuts down cleanly after receiving `SIGTERM`.

Threads are powerful because they share memory. That is also why they are
dangerous. If two threads can touch the same object at the same time, and one
of them writes, you need a synchronization design.

Without that design, the bug may not reproduce under a debugger. It may pass
tests for weeks. Then it may fail when the CPU, compiler optimization level,
timing, or deployment load changes.

## 3. Mental Model

### Concurrency vs Parallelism

Concurrency means multiple tasks are in progress during the same period of
time. Parallelism means tasks are physically running at the same instant on
different CPU cores.

A single-core system can run concurrent code by switching between threads. A
multi-core system can run parallel code by executing threads simultaneously.

### Process vs Thread

| Concept | Process | Thread |
| --- | --- | --- |
| Memory | Separate address space | Shares process address space |
| Creation cost | Higher | Lower |
| Communication | IPC, files, sockets, shared memory | Direct shared objects |
| Failure isolation | Better | Worse; one bad thread can corrupt the process |
| Main risk | IPC complexity | data race, deadlock, lifetime bugs |

Threads share globals, heap objects, file descriptors, and many runtime
resources. A pointer passed to another thread is still just a pointer. C++ does
not automatically make the pointed-to object live long enough or safe to share.

### Shared Mutable State Needs A Story

For every shared mutable object, choose one:

- one thread owns it and others never touch it;
- ownership moves from one thread to another;
- a mutex protects it;
- an atomic protects a simple independent value;
- a queue/message passes data between threads;
- the data is immutable after construction.

If you cannot say which one applies, the code is not ready.

### Race Condition vs Data Race

A **race condition** is a behavior bug where the result depends on timing.

A **data race** is a C++ memory-model bug: two threads access the same memory
location concurrently, at least one access writes, and there is no proper
synchronization. A C++ data race is undefined behavior.

All data races are serious. Not all race conditions are data races. For example,
two correctly locked threads may still have a timing-dependent business logic
bug.

## 4. Mechanism

### Thread Lifecycle

`std::thread` starts running as soon as it is constructed.

```cpp
#include <iostream>
#include <thread>

void work() {
    std::cout << "worker thread\n";
}

int main() {
    std::thread t(work);
    t.join();

    std::cout << "main thread\n";
}
```

Compile:

```sh
g++ -std=c++17 -Wall -Wextra -pedantic thread_basic.cpp -pthread -o thread_basic
./thread_basic
```

`join()` waits until the thread finishes. After `join()`, the `std::thread`
object no longer owns a running thread.

`detach()` lets the thread run independently. This is dangerous if the detached
thread uses references, pointers, or objects that may be destroyed before the
thread finishes.

Important lifecycle rule:

> If a `std::thread` object is destroyed while it is still joinable, the program
> calls `std::terminate()`.

That rule is harsh on purpose. C++ refuses to guess whether you meant to join or
detach.

### Thread Arguments And Lifetime

Arguments are copied or moved into the new thread by default.

```cpp
#include <iostream>
#include <string>
#include <thread>

void greet(std::string name) {
    std::cout << "Hello, " << name << "\n";
}

int main() {
    std::thread t(greet, "Alice");
    t.join();
}
```

Be careful with references:

```cpp
#include <iostream>
#include <thread>

void increment(int& value) {
    ++value;
}

int main() {
    int x = 41;
    std::thread t(increment, std::ref(x));
    t.join();

    std::cout << x << "\n";
}
```

`std::ref(x)` says "pass this exact object by reference". That is fine only if
`x` lives until the thread is done and the access is synchronized when needed.

### Mutexes Protect Invariants

`std::mutex` provides mutual exclusion. Only one thread can hold the mutex at a
time.

Do not think of a mutex as protecting one line. Think of it as protecting an
invariant.

Example invariant:

> `items` and `total_size` must describe the same queue state.

Every read or write that depends on that invariant must use the same mutex.

### RAII Locks

Manual `lock()` / `unlock()` is fragile:

```cpp
mtx.lock();
do_work();     // if this throws, unlock() is skipped
mtx.unlock();
```

Use RAII instead:

```cpp
{
    std::lock_guard<std::mutex> lock(mtx);
    do_work();
} // automatically unlocks here
```

`std::lock_guard` is the default for simple scoped locking.

`std::unique_lock` is more flexible. Use it when you need:

- condition variable waiting;
- deferred locking;
- manual unlock/relock;
- timed locking;
- movable lock ownership.

`std::scoped_lock` is useful for locking multiple mutexes safely.

### Condition Variables

A `std::condition_variable` lets one thread wait until a condition becomes true.
It is almost always used with:

- a mutex;
- shared state protected by that mutex;
- a predicate that checks the state.

The predicate is the real condition. Notification is only a wake-up signal.

```cpp
cv.wait(lock, [] { return ready; });
```

This form is important because condition variables may have spurious wakeups.
A waiting thread can wake even though no useful state changed. The predicate
form rechecks the state before continuing.

### Atomics

`std::atomic<T>` gives atomic operations on a shared object.

Atomics are good for simple independent state:

- counters;
- flags;
- reference counts;
- carefully designed lock-free protocols.

Atomics are not magic. If the invariant involves multiple values, a mutex is
usually clearer and safer.

`volatile` is not a thread synchronization primitive. It does not make
`counter++` atomic, does not create inter-thread ordering, and does not prevent
C++ data races.

### Semaphores

A mutex answers:

> Who may enter this critical section?

A semaphore answers:

> How many permits are available?

C++20 provides `std::counting_semaphore`. POSIX provides `sem_t`.

Use semaphores for bounded resources, capacity limits, or permit-style
coordination. Use mutexes to protect shared invariants.

### Futures, Promises, And Async

`std::future<T>` represents a result that will be available later.

`std::promise<T>` is the producer side. The worker sets the value or exception.
The waiting side calls `future.get()`.

`std::async` starts work and returns a future. It is useful for simple
result-bearing tasks, but it is not a complete thread-pool design.

### Signals Are Not Normal Callbacks

Signal handlers can interrupt normal code at awkward times. Inside a signal
handler, most C and C++ library calls are unsafe.

Do not do this in a signal handler:

- lock a mutex;
- allocate memory with `new` or `malloc`;
- write to `std::cout`;
- call complex cleanup code;
- touch arbitrary shared objects.

The portable baseline is to set a `volatile sig_atomic_t` flag, then let normal
code perform cleanup.

## 5. C/C++ API And Code

### Example 1: Data Race Bug

This program is intentionally wrong.

```cpp
#include <iostream>
#include <thread>

int counter = 0;

void increment() {
    for (int i = 0; i < 100000; ++i) {
        ++counter; // data race: read-modify-write without synchronization
    }
}

int main() {
    std::thread a(increment);
    std::thread b(increment);

    a.join();
    b.join();

    std::cout << "counter = " << counter << "\n";
}
```

Compile and run:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -pedantic race.cpp -pthread -o race
./race
```

`++counter` looks like one operation in C++, but conceptually it is:

1. read current value;
2. add one;
3. write new value.

Two threads can interleave those steps and lose updates. Worse, because this is
a C++ data race, the behavior is undefined.

### Example 2: Fix With `std::mutex`

```cpp
#include <iostream>
#include <mutex>
#include <thread>
#include <utility>

int counter = 0;
std::mutex counter_mutex;

void increment() {
    for (int i = 0; i < 100000; ++i) {
        std::lock_guard<std::mutex> lock(counter_mutex);
        ++counter;
    }
}

int main() {
    std::thread a(increment);
    std::thread b(increment);

    a.join();
    b.join();

    std::cout << "counter = " << counter << "\n";
}
```

Compile:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -pedantic counter_mutex.cpp -pthread -o counter_mutex
./counter_mutex
```

This is correct because every access to `counter` is protected by the same
mutex.

### Example 3: Fix With `std::atomic`

```cpp
#include <atomic>
#include <iostream>
#include <thread>

std::atomic<int> counter{0};

void increment() {
    for (int i = 0; i < 100000; ++i) {
        counter.fetch_add(1);
    }
}

int main() {
    std::thread a(increment);
    std::thread b(increment);

    a.join();
    b.join();

    std::cout << "counter = " << counter.load() << "\n";
}
```

Compile:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -pedantic counter_atomic.cpp -pthread -o counter_atomic
./counter_atomic
```

This is appropriate because the shared state is one independent integer.

If the state were `balance`, `last_update`, and `transaction_id` that must
change together, a single atomic counter would not protect the invariant. Use a
mutex for compound state.

### Example 4: Producer-Consumer With `std::condition_variable`

```cpp
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

std::mutex mtx;
std::condition_variable cv;
std::queue<int> queue;
bool done = false;

void producer() {
    for (int value = 1; value <= 5; ++value) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            queue.push(value);
        }
        cv.notify_one();
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        done = true;
    }
    cv.notify_all();
}

void consumer() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [] {
            return done || !queue.empty();
        });

        while (!queue.empty()) {
            int value = queue.front();
            queue.pop();

            lock.unlock();
            std::cout << "consumed " << value << "\n";
            lock.lock();
        }

        if (done) {
            break;
        }
    }
}

int main() {
    std::thread p(producer);
    std::thread c(consumer);

    p.join();
    c.join();
}
```

Compile:

```sh
g++ -std=c++17 -Wall -Wextra -pedantic producer_consumer.cpp -pthread -o producer_consumer
./producer_consumer
```

Important details:

- `queue` and `done` are protected by `mtx`;
- the wait uses a predicate;
- the consumer unlocks before printing so the lock is not held during I/O;
- `notify_all()` wakes all waiters during shutdown.

### Example 5: Avoid Deadlock With `std::scoped_lock`

```cpp
#include <iostream>
#include <mutex>
#include <thread>
#include <utility>

std::mutex left_mtx;
std::mutex right_mtx;
int left_value = 1;
int right_value = 2;

void swap_values() {
    std::scoped_lock lock(left_mtx, right_mtx);
    std::swap(left_value, right_value);
}

int main() {
    std::thread a(swap_values);
    std::thread b(swap_values);

    a.join();
    b.join();

    std::cout << left_value << " " << right_value << "\n";
}
```

Compile:

```sh
g++ -std=c++17 -Wall -Wextra -pedantic scoped_lock.cpp -pthread -o scoped_lock
./scoped_lock
```

If thread A locks `left_mtx` then waits for `right_mtx`, while thread B locks
`right_mtx` then waits for `left_mtx`, both can block forever. `std::scoped_lock`
locks multiple mutexes using a deadlock-avoidance algorithm.

### Example 6: Future And Exception Propagation

```cpp
#include <exception>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

void worker(std::promise<int> result) {
    try {
        throw std::runtime_error("sensor read failed");
        result.set_value(42);
    } catch (...) {
        result.set_exception(std::current_exception());
    }
}

int main() {
    std::promise<int> promise;
    std::future<int> future = promise.get_future();

    std::thread t(worker, std::move(promise));

    try {
        std::cout << future.get() << "\n";
    } catch (const std::exception& e) {
        std::cout << "worker error: " << e.what() << "\n";
    }

    t.join();
}
```

Compile:

```sh
g++ -std=c++17 -Wall -Wextra -pedantic future_exception.cpp -pthread -o future_exception
./future_exception
```

Do not let exceptions accidentally escape thread entry functions. Catch at the
thread boundary and propagate deliberately.

### Example 7: Signal-Aware Shutdown

This is a small POSIX-friendly pattern. The handler only sets a flag. Cleanup
happens in normal code.

```cpp
#include <csignal>
#include <chrono>
#include <iostream>
#include <thread>

volatile std::sig_atomic_t stop_requested = 0;

extern "C" void handle_signal(int) {
    stop_requested = 1;
}

int main() {
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    std::cout << "running; press Ctrl+C to stop\n";

    while (!stop_requested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "." << std::flush;
    }

    std::cout << "\ncleanup in normal control flow\n";
}
```

Compile:

```sh
g++ -std=c++17 -Wall -Wextra -pedantic signal_shutdown.cpp -pthread -o signal_shutdown
./signal_shutdown
```

In production POSIX code, `sigaction()` is usually preferred over `signal()` for
more explicit behavior and signal mask control.

## 6. Practical Usage

### Thread-Safe Counter Class

```cpp
#include <mutex>

class Counter {
public:
    void increment() {
        std::lock_guard<std::mutex> lock(mtx_);
        ++value_;
    }

    int value() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return value_;
    }

private:
    mutable std::mutex mtx_;
    int value_ = 0;
};
```

The mutex is `mutable` because `value() const` does not change the logical
counter value, but it still needs to lock internally.

### Thread-Safe Queue Shape

Production queues need careful policy:

- Is capacity bounded?
- What happens when the queue is full?
- How does shutdown wake blocked producers and consumers?
- Are tasks allowed to throw?
- Who owns objects placed in the queue?

The queue API should communicate these answers. A vague `push()` / `pop()` API
often hides blocking and shutdown behavior.

### Thread Pool Shape

A minimal thread pool has:

- worker threads;
- a protected task queue;
- a condition variable;
- a stop flag;
- a destructor or `stop()` function that wakes workers and joins them;
- a policy for task exceptions;
- optional backpressure when too many tasks are queued.

Learning-only thread pools are useful, but production thread pools need much
more attention: bounded memory, cancellation, monitoring, error reporting,
shutdown ordering, and task lifetime.

### Embedded And System User-Space Notes

For embedded or system user-space code:

- avoid unbounded thread creation;
- avoid dynamic allocation in hot worker paths when latency matters;
- define shutdown behavior before writing the worker loop;
- avoid holding locks around hardware, filesystem, network, or logging calls;
- account for priority inversion if priorities are used;
- use POSIX APIs only when the portability or platform-specific behavior is
  deliberately required.

This topic is about user-space C/C++ concurrency. It does not require kernel
driver material.

## 7. Comparisons

### `pthread` vs `std::thread`

| Point | `pthread` | `std::thread` |
| --- | --- | --- |
| Language level | C / POSIX | Standard C++ |
| Entry point | C function pointer style | Any callable object |
| Error style | return codes | exceptions for construction errors, member functions for state |
| RAII | manual unless wrapped | object owns thread handle |
| Best use | POSIX-specific C or legacy integration | normal C++ application/library code |

Use `std::thread` for idiomatic C++ unless you need POSIX-specific attributes,
scheduling, or a C ABI boundary.

### `pthread_mutex_t` vs `std::mutex`

| Point | `pthread_mutex_t` | `std::mutex` |
| --- | --- | --- |
| Setup | explicit initialization/destruction | object construction/destruction |
| Locking | `pthread_mutex_lock`, `pthread_mutex_unlock` | `lock`, `unlock`, usually via RAII |
| Error style | return codes | C++ API |
| Typical C++ usage | wrapped behind RAII if needed | direct with `lock_guard`, `unique_lock`, `scoped_lock` |

In C++, raw lock/unlock should be rare. Prefer RAII locks.

### `pthread_cond_t` vs `std::condition_variable`

Both wait on shared state protected by a mutex. Both require a predicate loop.

Modern C++ makes the safe pattern easier:

```cpp
std::unique_lock<std::mutex> lock(mtx);
cv.wait(lock, [] { return ready; });
```

The predicate matters more than the notification. A notification without stored
state can be lost.

### `sem_t` vs `std::counting_semaphore`

| Point | POSIX `sem_t` | C++20 `std::counting_semaphore` |
| --- | --- | --- |
| Scope | POSIX; named or unnamed semaphores | Standard C++ object |
| API | `sem_wait`, `sem_post` | `acquire`, `release` |
| Good for | POSIX interop, named semaphores | portable in-process C++ permit counting |

Use a semaphore when you are counting available permits. Use a mutex when you
are protecting an invariant.

### `volatile` vs `std::atomic`

| Need | `volatile` | `std::atomic` |
| --- | --- | --- |
| Prevent compiler from optimizing away certain accesses | yes, for specific use cases | not its main purpose |
| Atomic read-modify-write | no | yes |
| Inter-thread synchronization | no | yes, with defined memory ordering |
| Thread-safe counter | no | yes |
| Signal flag baseline | `volatile sig_atomic_t` | only with careful lock-free qualification |

For threads, use `std::atomic` or a mutex. Do not use `volatile` as a thread
safety tool.

### Mutex vs Semaphore

| Use case | Choose |
| --- | --- |
| Protect a map, vector, queue, object invariant, or transaction state | mutex |
| Limit concurrent access to N resources | semaphore |
| Signal that queue capacity is available | semaphore or condition variable |
| Make `counter++` safe | atomic or mutex, not semaphore |

### `lock_guard` vs `unique_lock`

| Feature | `std::lock_guard` | `std::unique_lock` |
| --- | --- | --- |
| Simple scoped lock | yes | yes |
| Manual unlock/relock | no | yes |
| Deferred locking | no | yes |
| Condition variable wait | no | yes |
| Lower conceptual overhead | yes | no |

Use the simplest lock that expresses the lifetime clearly.

## 8. Common Bugs

### Bug: Joinable Thread Destroyed

```cpp
void bad() {
    std::thread t([] {});
} // std::terminate()
```

Fix:

```cpp
void good() {
    std::thread t([] {});
    t.join();
}
```

### Bug: Detached Thread Uses Dead Object

```cpp
void bad() {
    std::string message = "hello";
    std::thread([&] {
        std::cout << message << "\n";
    }).detach();
} // message is destroyed while detached thread may still use it
```

Fix by avoiding detach, or by giving the thread independent ownership of the
data and a clear shutdown path.

### Bug: Condition Variable Without Predicate

```cpp
cv.wait(lock); // unsafe pattern
use_data();
```

Fix:

```cpp
cv.wait(lock, [] { return data_ready; });
use_data();
```

### Bug: Holding Lock While Calling Unknown Code

```cpp
void notify_user(const std::function<void()>& callback) {
    std::lock_guard<std::mutex> lock(mtx);
    callback(); // callback may call back into this object and deadlock
}
```

Fix: copy required state under lock, release, then call the callback.

### Bug: Atomics For Compound State

```cpp
std::atomic<int> size;
std::atomic<int> checksum;
```

Two atomics do not automatically create one consistent snapshot. If `size` and
`checksum` must match the same version of data, protect the whole invariant
with a mutex or publish immutable snapshots.

### Bug: Unsafe Signal Handler

```cpp
void handler(int) {
    std::cout << "stopping\n"; // unsafe in signal handler
    global_mutex.lock();       // may deadlock
}
```

Fix:

```cpp
volatile std::sig_atomic_t stop_requested = 0;

extern "C" void handler(int) {
    stop_requested = 1;
}
```

## 9. Debugging

### Use ThreadSanitizer

For many data races:

```sh
g++ -std=c++17 -g -O1 -fsanitize=thread race.cpp -pthread -o race_tsan
./race_tsan
```

ThreadSanitizer is not a proof of correctness, but it is very good at exposing
many accidental data races.

### Also Use ASan And UBSan

Some "thread bugs" are really lifetime bugs:

- use-after-free;
- stack reference escaping to another thread;
- invalid iterator use;
- double delete;
- undefined behavior that appears only under timing pressure.

Useful debug build:

```sh
g++ -std=c++17 -g -O1 -fsanitize=address,undefined file.cpp -pthread -o app_asan
./app_asan
```

### Debugging Deadlocks

Look for:

- inconsistent lock ordering;
- a thread waiting while still holding another lock;
- callbacks called under lock;
- blocked `join()` from the wrong thread;
- condition variable waits whose predicate never changes;
- shutdown paths that forget to notify waiters.

When a program hangs, inspect all thread backtraces. The question is:

> What is each thread waiting for, and who was supposed to make that condition
> true?

### Debugging Condition Variables

Log state transitions, not only notifications:

```text
queue_size=0 done=false -> consumer waits
queue_size=1 done=false -> producer notifies
queue_size=0 done=true  -> shutdown notifies all
```

If logs only say "notify_one called", you still do not know whether the
predicate became true.

### Debugging Atomics

Write the invariant in English. For example:

> When `ready == true`, the consumer must see the initialized payload.

That invariant requires ordering. A relaxed atomic flag is probably not enough.
If you cannot explain the memory order, use a mutex first.

## 10. Best Practices

- Design thread ownership before writing code.
- Prefer no shared mutable state when practical.
- Document which mutex protects which data.
- Protect invariants, not individual statements.
- Use `std::lock_guard` for simple critical sections.
- Use `std::unique_lock` for condition variables or flexible locking.
- Use `std::scoped_lock` for multiple mutexes.
- Always wait on condition variables with predicates.
- Keep critical sections small.
- Do not hold locks while doing slow I/O, sleeping, logging, or calling unknown
  callbacks.
- Avoid detached threads unless the lifetime model is explicit and reviewed.
- Catch exceptions at thread and task boundaries.
- Use `std::atomic` for simple flags/counters, not compound invariants.
- Treat `memory_order_relaxed` as advanced until you can prove it is correct.
- Prefer bounded queues and clear shutdown behavior in production worker pools.
- Never use `volatile` as a thread synchronization tool.
- Keep signal handlers minimal and async-signal-safe.
- Test with sanitizers and stress runs.

## 11. Interview Readiness

You should be able to answer these clearly.

### What Happens If A Joinable `std::thread` Is Destroyed?

Short answer: the program calls `std::terminate()`.

Why: C++ requires you to choose ownership behavior explicitly. You must call
`join()` or `detach()` before the `std::thread` destructor runs.

### Race Condition vs Data Race?

A race condition is a timing-dependent behavior bug.

A data race is a specific C++ memory-model violation: unsynchronized conflicting
accesses to the same memory location, with at least one write. A data race is
undefined behavior.

### Why Use `lock_guard`?

`std::lock_guard` makes mutex release automatic through RAII. It prevents bugs
where an exception or early return skips `unlock()`.

### Why Must Condition Variables Use Predicates?

Because wakeups can be spurious, and notifications are not stored state. The
predicate says whether the program condition is actually true.

### Mutex vs Semaphore?

A mutex protects exclusive access to shared state. A semaphore counts permits.
Use a mutex for object invariants; use a semaphore for capacity or resource
counting.

### Atomic vs `volatile`?

`std::atomic` provides atomic operations and defined inter-thread behavior.
`volatile` does not make operations atomic and does not synchronize threads.

### How Do You Avoid Deadlock?

Use consistent lock ordering, avoid holding locks while calling unknown code,
prefer `std::scoped_lock` for multiple mutexes, keep critical sections small,
and design shutdown paths so waiters are notified.

### How Would You Design A Thread Pool?

A basic thread pool has workers, a task queue, a mutex, a condition variable, a
stop flag, a destructor or stop function that wakes and joins workers, exception
handling at task boundaries, and a backpressure policy for production use.

### What Is Safe In A Signal Handler?

Very little. Usually set a `volatile sig_atomic_t` flag, or use a carefully
validated async-signal-safe operation, then perform cleanup in normal code.

## 12. Practice

1. Write the broken `counter++` race example. Run it several times. Then run it
   with ThreadSanitizer.
2. Fix the counter once with `std::mutex` and once with `std::atomic<int>`.
   Explain why both fixes work for this specific case.
3. Implement a `ThreadSafeQueue<int>` with `push`, blocking `pop`, and
   `close`. Use `std::condition_variable` with predicates.
4. Create a two-mutex deadlock intentionally, then fix it using consistent lock
   order and `std::scoped_lock`.
5. Write a worker function that catches exceptions and sends them through
   `std::promise` / `std::future`.
6. Build a tiny learning-only thread pool with two workers and a queue of
   `std::function<void()>` tasks. Add a clean shutdown path.
7. Add a signal-aware shutdown flag using `volatile sig_atomic_t`. Confirm the
   handler does not log, allocate, or lock.
8. Take a small class with shared state and document which mutex protects which
   data. If the answer is unclear, redesign the class.

## 13. Reference Notes

- `std::thread`, `std::mutex`, `std::condition_variable`, `std::atomic`,
  `std::future`, and `std::counting_semaphore` are Standard C++ library
  facilities.
- POSIX APIs such as `pthread_create`, `pthread_mutex_lock`,
  `pthread_cond_wait`, and `sem_wait` are useful for C/POSIX interop and
  platform-specific behavior, but C++ code should usually wrap them or prefer
  standard C++ primitives.
- For signal handling, prefer minimal handlers. POSIX `sigaction()` gives more
  control than `signal()` on POSIX systems, but handler safety rules still
  apply.
