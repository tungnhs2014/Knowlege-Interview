# 14 - Concurrency Interview Pack

## How To Use This Pack

Concurrency interviews test whether you can reason about correctness under
unpredictable timing. Strong answers should start with the invariant, then name
the synchronization mechanism, then explain the failure mode if that mechanism
is missing.

The most important principle:

> Correctness first, performance second.

## Beginner Questions

### 1. What Is The Difference Between Concurrency And Parallelism?

**Short answer:** Concurrency means multiple tasks are in progress during the
same time period. Parallelism means tasks are physically executing at the same
instant, usually on different CPU cores.

**Deep explanation:** A single-core machine can run concurrent code by switching
between tasks. A multi-core machine can run parallel code by running tasks at
the same time. Concurrency is about program structure and coordination.
Parallelism is about simultaneous execution. In C++, threads may be concurrent,
parallel, or both, depending on the runtime and hardware.

**C/C++ code/API anchor:**

```cpp
#include <iostream>
#include <thread>

void task(int id) {
    std::cout << "task " << id << "\n";
}

int main() {
    std::thread a(task, 1);
    std::thread b(task, 2);
    a.join();
    b.join();
}
```

`std::thread` creates concurrent execution. Whether `a` and `b` run in parallel
depends on scheduling and hardware.

**Production/debug angle:** Do not assume that creating more threads improves
performance. Too many threads can create scheduling overhead, contention, and
harder debugging.

**Traps:**

- Saying concurrency and parallelism are the same.
- Assuming `std::thread::hardware_concurrency()` is a guaranteed thread count.
- Ignoring synchronization just because a test machine has one core.

**Follow-ups:**

- Can concurrent code have a data race on a single-core system?
- Why can adding threads make a program slower?
- How would you decide the worker count for a thread pool?

### 2. What Is The Difference Between A Process And A Thread?

**Short answer:** A process has its own address space. Threads inside one
process share the same address space and resources.

**Deep explanation:** Threads are cheaper to create and communicate through
shared memory, but shared memory also means one thread can corrupt data used by
another. A process provides stronger isolation but needs IPC, files, sockets, or
shared memory for communication.

**C/C++ code/API anchor:** `std::thread` creates a thread inside the current
process. POSIX code may use `pthread_create()` for the same general purpose in
C/POSIX APIs.

```cpp
std::thread worker([] {
    // Runs in the same process and can access shared objects.
});
worker.join();
```

**Production/debug angle:** A crash or memory corruption in one thread can bring
down the whole process. Logging thread IDs and ownership boundaries helps debug
multi-threaded failures.

**Traps:**

- Thinking each thread has separate global variables.
- Passing raw pointers to threads without lifetime guarantees.
- Forgetting that standard containers are not automatically safe for concurrent
  mutation.

**Follow-ups:**

- What memory does a thread have privately?
- When would you prefer processes over threads?
- What does "shared address space" imply for a global variable?

### 3. What Happens If A Joinable `std::thread` Is Destroyed?

**Short answer:** The program calls `std::terminate()`.

**Deep explanation:** A `std::thread` object owns a thread handle. If it is still
joinable at destruction, C++ cannot safely guess whether the thread should be
joined or detached, so it terminates the program. The owner must explicitly call
`join()` or `detach()` before destruction.

**C/C++ code/API anchor:**

```cpp
#include <thread>

void bad() {
    std::thread t([] {});
} // std::terminate()

void good() {
    std::thread t([] {});
    t.join();
}
```

Relevant APIs: `std::thread::joinable()`, `join()`, `detach()`.

**Production/debug angle:** This often appears on exception paths. If code
starts a thread and then throws before `join()`, the destructor can terminate
the process. Use RAII joiners or `std::jthread` where available.

**Traps:**

- Assuming the destructor joins automatically.
- Calling `join()` twice.
- Using `detach()` as a shortcut without a lifetime/shutdown design.

**Follow-ups:**

- What does `joinable()` mean?
- Why is `detach()` risky?
- How would you make thread ownership exception-safe?

### 4. Race Condition vs Data Race?

**Short answer:** A race condition is a timing-dependent behavior bug. A data
race is a specific C++ undefined behavior: conflicting unsynchronized accesses
to the same memory location, with at least one write.

**Deep explanation:** A race condition can happen even if all memory accesses
are protected, for example if two valid operations happen in the wrong business
order. A data race violates the C++ memory model. Once a data race exists, the
compiler and CPU are not required to preserve the behavior you expected.

**C/C++ code/API anchor:**

```cpp
int counter = 0;

void increment() {
    ++counter; // data race if called by multiple threads without synchronization
}
```

Fix with `std::mutex` or `std::atomic<int>` depending on the invariant.

**Production/debug angle:** Data races may disappear under a debugger and appear
only under optimization or production load. Use `-fsanitize=thread` to catch
many accidental races.

**Traps:**

- Saying "the result is just unpredictable" and missing undefined behavior.
- Calling every timing bug a data race.
- Thinking `volatile` fixes the problem.

**Follow-ups:**

- Is `counter++` atomic?
- Can a program have a race condition without a data race?
- How would ThreadSanitizer help here?

## Mid-Level Questions

### 5. Why Is `counter++` Not Thread-Safe, And How Would You Fix It?

**Short answer:** `counter++` is a read-modify-write sequence. Multiple threads
can interleave those steps. Fix it with a mutex or an atomic if the counter is
an independent value.

**Deep explanation:** The expression appears as one source line, but it needs to
read the old value, compute the new value, and write it back. Without
synchronization, two threads can read the same old value and both write the same
new value. In C++, this is also a data race if `counter` is a non-atomic shared
object.

**C/C++ code/API anchor:**

```cpp
#include <atomic>
#include <mutex>

std::atomic<int> atomic_counter{0};

void atomic_fix() {
    atomic_counter.fetch_add(1);
}

int counter = 0;
std::mutex mtx;

void mutex_fix() {
    std::lock_guard<std::mutex> lock(mtx);
    ++counter;
}
```

**Production/debug angle:** Use an atomic for a simple independent counter. Use
a mutex if the counter is part of a larger invariant, such as queue size plus
checksum plus state.

**Traps:**

- Using `volatile int counter`.
- Using multiple atomics for related fields and assuming the snapshot is
  consistent.
- Using `memory_order_relaxed` without explaining why ordering is not needed.

**Follow-ups:**

- When is `std::atomic<int>` better than a mutex?
- When is a mutex better than atomics?
- What would change if the counter update also modified a `std::vector`?

### 6. Why Should You Use RAII Locks Instead Of Manual `lock()` / `unlock()`?

**Short answer:** RAII locks release the mutex automatically when the lock object
leaves scope, including during exceptions and early returns.

**Deep explanation:** Manual locking creates duplicated cleanup paths. Any
exception, return, or future edit can skip `unlock()`, causing deadlock. RAII
ties mutex ownership to object lifetime. This is the same C++ resource-management
idea used for files, memory, and smart pointers.

**C/C++ code/API anchor:**

```cpp
#include <mutex>

std::mutex mtx;

void update() {
    std::lock_guard<std::mutex> lock(mtx);
    // protected state update
} // unlocks here
```

Use `std::lock_guard` for simple scopes, `std::unique_lock` for condition
variables or flexible locking, and `std::scoped_lock` for multiple mutexes.

**Production/debug angle:** In code review, raw `lock()` / `unlock()` should
trigger a question: why is RAII not enough here?

**Traps:**

- Creating a temporary unnamed lock object that unlocks immediately.
- Holding a lock while calling unknown callbacks.
- Using `unique_lock` everywhere when `lock_guard` is clearer.

**Follow-ups:**

- When do you need `std::unique_lock`?
- Why is `std::scoped_lock` useful?
- What is the exception-safety issue with manual locking?

### 7. How Should A `std::condition_variable` Be Used?

**Short answer:** Protect shared state with a mutex, wait with a predicate, and
notify after changing the state.

**Deep explanation:** A condition variable does not store the condition. It only
wakes waiting threads. The shared predicate, such as `!queue.empty() || done`,
is the real state. Waiting without a predicate is vulnerable to spurious wakeups
and lost-wakeup style bugs.

**C/C++ code/API anchor:**

```cpp
#include <condition_variable>
#include <mutex>
#include <queue>

std::mutex mtx;
std::condition_variable cv;
std::queue<int> q;
bool done = false;

void consumer() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [] { return done || !q.empty(); });
}
```

**Production/debug angle:** Log predicate transitions such as queue size and
shutdown state. Logging only "notify called" is not enough to debug missed
state changes.

**Traps:**

- Calling `cv.wait(lock)` without a predicate.
- Updating the predicate without the same mutex.
- Thinking `notify_one()` stores an event for future waiters.

**Follow-ups:**

- What is a spurious wakeup?
- Why does `wait()` need `std::unique_lock`?
- Should you call `notify_one()` while holding or after releasing the lock?

### 8. Compare `lock_guard`, `unique_lock`, And `scoped_lock`.

**Short answer:** `lock_guard` is simple scoped locking, `unique_lock` is
movable and flexible, and `scoped_lock` can lock multiple mutexes safely.

**Deep explanation:** Prefer the simplest abstraction that expresses ownership.
`lock_guard` is ideal for a short critical section. `unique_lock` supports
deferred locking, manual unlock/relock, timed locking, and condition-variable
waits. `scoped_lock` is useful when multiple mutexes must be acquired together
without creating a lock-order deadlock.

**C/C++ code/API anchor:**

```cpp
std::lock_guard<std::mutex> a(m1);

std::unique_lock<std::mutex> b(m2);
cv.wait(b, [] { return ready; });

std::scoped_lock both(m1, m2);
```

**Production/debug angle:** Over-flexible locking makes ownership harder to
review. If `unique_lock` is used, ask which feature requires it.

**Traps:**

- Using `lock_guard` with condition variables.
- Manually locking multiple mutexes in inconsistent order.
- Using `recursive_mutex` to hide unclear design.

**Follow-ups:**

- Why is `unique_lock` required by `std::condition_variable`?
- How does `scoped_lock` help avoid deadlock?
- When would `timed_mutex` be useful?

### 9. Mutex vs Semaphore?

**Short answer:** A mutex protects exclusive access to shared state. A semaphore
counts available permits.

**Deep explanation:** A mutex is about ownership of a critical section and
usually protects an invariant. A semaphore is about capacity or resource
counting. For example, a mutex protects a queue's internal data, while a
semaphore can represent "N free slots" in a bounded queue.

**C/C++ code/API anchor:**

```cpp
#include <mutex>
#include <semaphore> // C++20

std::mutex queue_mutex;
std::counting_semaphore<10> slots{10};
```

POSIX comparison: `sem_t` uses APIs such as `sem_wait()` and `sem_post()`.

**Production/debug angle:** Choosing the wrong primitive makes code hard to
reason about. If you use a semaphore to protect a multi-field invariant, review
whether a mutex is the clearer design.

**Traps:**

- Treating a binary semaphore as an automatic mutex replacement.
- Forgetting ownership semantics: mutexes have ownership expectations;
  semaphores count permits.
- Forgetting release paths during exceptions or shutdown.

**Follow-ups:**

- How would you use a semaphore in a bounded queue?
- When would `std::counting_semaphore` be preferable to a condition variable?
- How does POSIX `sem_t` differ from C++20 `std::counting_semaphore`?

### 10. Atomic vs `volatile`?

**Short answer:** `std::atomic` provides atomic operations and inter-thread
memory-order semantics. `volatile` is not a thread synchronization primitive.

**Deep explanation:** `volatile` affects certain compiler optimizations around
accesses, but it does not make operations atomic, does not provide ordering
between threads, and does not make a data race safe. `std::atomic` is the C++
tool for thread-safe flags, counters, and carefully designed lock-free
protocols.

**C/C++ code/API anchor:**

```cpp
#include <atomic>

std::atomic<bool> stop{false};

void request_stop() {
    stop.store(true);
}

bool should_stop() {
    return stop.load();
}
```

Signal-handler note: `volatile sig_atomic_t` is the portable C baseline for a
minimal signal flag. That is different from using `volatile` for thread
synchronization.

**Production/debug angle:** A `volatile bool done` used between threads is a
code-review red flag. Replace it with `std::atomic<bool>` or a
condition-variable shutdown protocol.

**Traps:**

- Believing `volatile` makes `counter++` safe.
- Using relaxed atomics without proving ordering is irrelevant.
- Using atomics for compound invariants.

**Follow-ups:**

- What does `memory_order_relaxed` not guarantee?
- When is a mutex simpler than an atomic?
- Why is signal handling a special case?

## Senior Questions

### 11. How Do You Design A Thread-Safe Queue?

**Short answer:** Protect the queue and shutdown state with one mutex, wait with
a condition-variable predicate, define ownership and close semantics, and avoid
holding the lock while processing items.

**Deep explanation:** A queue is not just `push` and `pop`. It needs a lifecycle
policy. What happens when consumers wait and the queue closes? Is capacity
bounded? Are producers blocked, rejected, or timed out when full? Who owns the
data? The invariant might be: all access to `items_` and `closed_` happens under
`mtx_`.

**C/C++ code/API anchor:**

```cpp
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

class Queue {
public:
    void push(int value) {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (closed_) return;
            items_.push(value);
        }
        cv_.notify_one();
    }

    std::optional<int> pop() {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [&] { return closed_ || !items_.empty(); });

        if (items_.empty()) return std::nullopt;
        int value = items_.front();
        items_.pop();
        return value;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            closed_ = true;
        }
        cv_.notify_all();
    }

private:
    std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<int> items_;
    bool closed_ = false;
};
```

**Production/debug angle:** Production queues often need bounded capacity,
timeouts, metrics, backpressure, and explicit error returns. Debug by logging
queue size, `closed_`, wait entry, and wake reasons.

**Traps:**

- Waiting on `!items_.empty()` but forgetting shutdown.
- Returning references to internal queue elements after unlocking.
- Calling callbacks while holding the queue lock.

**Follow-ups:**

- How would you make this queue bounded?
- How would you support cancellation?
- Should `push()` throw, block, or return `false` after close?

### 12. How Would You Design A Minimal Thread Pool?

**Short answer:** Use worker threads, a protected task queue, a condition
variable, a stop flag, clean shutdown, and exception containment at the task
boundary.

**Deep explanation:** A thread pool amortizes thread creation, but it introduces
ownership and lifecycle problems. The destructor must stop accepting work, wake
workers, and join them. Tasks can throw, so each worker should catch exceptions
or the pool should define a future/result mechanism. Production designs also
need bounded queues and backpressure.

**C/C++ code/API anchor:**

```cpp
// Core worker loop shape:
while (true) {
    std::function<void()> task;
    {
        std::unique_lock<std::mutex> lock(mtx_);
        cv_.wait(lock, [&] { return stopping_ || !tasks_.empty(); });
        if (stopping_ && tasks_.empty()) return;
        task = std::move(tasks_.front());
        tasks_.pop();
    }

    try {
        task();
    } catch (...) {
        // record/report according to pool policy
    }
}
```

**Production/debug angle:** Debugging a hung thread pool usually means checking
which workers are waiting, whether `stopping_` was set, whether `notify_all()`
was called, and whether a task is blocked while holding a shared lock.

**Traps:**

- Letting task exceptions escape worker threads.
- Destroying the pool while producers still enqueue work.
- Unbounded queues that hide overload until memory grows.
- Holding the queue lock while running tasks.

**Follow-ups:**

- How would you return task results?
- How would you implement backpressure?
- How would you stop long-running tasks?

### 13. How Do You Avoid Deadlock In Code That Locks Multiple Mutexes?

**Short answer:** Use a consistent lock order or lock multiple mutexes with
`std::scoped_lock` / `std::lock`; avoid calling unknown code while holding locks.

**Deep explanation:** Deadlock occurs when threads wait forever on each other.
The classic two-lock case is thread A holds `m1` and waits for `m2`, while
thread B holds `m2` and waits for `m1`. A lock hierarchy prevents cycles.
`std::scoped_lock` helps when multiple mutexes must be acquired together.

**C/C++ code/API anchor:**

```cpp
#include <mutex>

std::mutex m1;
std::mutex m2;

void safe() {
    std::scoped_lock lock(m1, m2);
    // work with both protected resources
}
```

**Production/debug angle:** For a deadlock, collect all thread backtraces and
ask: what lock is each thread holding, and what lock is it waiting for?

**Traps:**

- Locking in different orders in different functions.
- Waiting on a condition while holding unrelated locks.
- Invoking callbacks under lock.
- Using `recursive_mutex` to avoid designing ownership.

**Follow-ups:**

- What are the classic deadlock conditions?
- How would you debug a production deadlock?
- When might a timeout help, and when does it only hide the bug?

### 14. Explain Acquire/Release, Relaxed, And Sequential Consistency At A Practical Level.

**Short answer:** Sequential consistency is the simplest default ordering.
Acquire/release coordinates publication and consumption of data. Relaxed gives
atomicity without ordering for other data.

**Deep explanation:** `std::atomic` operations have memory-order semantics.
Sequential consistency is easiest to reason about and is a good default while
learning. Release on a writer and acquire on a reader can express "data written
before the release becomes visible after the acquire." Relaxed ordering is only
safe when you do not need ordering with other memory, such as a statistics
counter where exact timing of visibility does not matter.

**C/C++ code/API anchor:**

```cpp
#include <atomic>

std::atomic<bool> ready{false};
int payload = 0;

void producer() {
    payload = 42;
    ready.store(true, std::memory_order_release);
}

void consumer() {
    if (ready.load(std::memory_order_acquire)) {
        // sees payload initialized by producer
    }
}
```

**Production/debug angle:** Require a written invariant for non-default memory
ordering. If the team cannot explain the invariant, use a mutex or default
atomic ordering first.

**Traps:**

- Using `memory_order_relaxed` for a ready flag that publishes data.
- Believing atomics make non-atomic related data safe without ordering.
- Over-optimizing before measuring contention.

**Follow-ups:**

- When is relaxed ordering acceptable?
- Why is a mutex often easier to review?
- What is false sharing, and why can atomics still be slow?

### 15. How Should Exceptions Be Handled Across Threads?

**Short answer:** Catch exceptions at the thread or task boundary and propagate
them deliberately, often through `std::promise` / `std::future` or a task result
object.

**Deep explanation:** Exceptions do not automatically jump from a worker thread
to the creator thread. If a thread function lets an exception escape, the
program terminates. A worker must catch exceptions and communicate failure
through a defined channel.

**C/C++ code/API anchor:**

```cpp
#include <exception>
#include <future>
#include <stdexcept>

void worker(std::promise<int> p) {
    try {
        throw std::runtime_error("failed");
    } catch (...) {
        p.set_exception(std::current_exception());
    }
}
```

The caller receives the exception when calling `future.get()`.

**Production/debug angle:** Thread pools need a policy: log and continue, store
failure in a future, stop the service, or mark the task failed. Silent catch-all
blocks are production traps.

**Traps:**

- Assuming a creator thread can catch exceptions thrown inside `std::thread`.
- Letting task exceptions kill workers.
- Losing context when converting exceptions to boolean failure.

**Follow-ups:**

- How does `std::async` propagate exceptions?
- What should a thread pool do when a task throws?
- How would you include task ID/context in the failure?

### 16. How Do You Handle Signals In A Multithreaded Program?

**Short answer:** Keep signal handlers minimal and async-signal-safe. Usually
set a `volatile sig_atomic_t` flag or use a carefully validated lock-free flag,
then do cleanup in normal code.

**Deep explanation:** A signal can interrupt code while it holds locks or is
inside library internals. Calling non-async-signal-safe functions from the
handler can deadlock or corrupt state. Signal handling is not normal callback
execution. POSIX `sigaction()` gives better control than `signal()` on POSIX
systems, but it does not remove async-signal-safety restrictions.

**C/C++ code/API anchor:**

```cpp
#include <csignal>

volatile std::sig_atomic_t stop_requested = 0;

extern "C" void handler(int) {
    stop_requested = 1;
}

int main() {
    std::signal(SIGTERM, handler);
    while (!stop_requested) {
        // normal work
    }
    // cleanup here, not in the handler
}
```

**Production/debug angle:** Test `SIGINT` and `SIGTERM` separately. Verify that
waiting threads wake during shutdown and that cleanup does not happen inside the
handler.

**Traps:**

- Calling `std::cout`, `printf`, `new`, `delete`, or `mutex.lock()` inside a
  signal handler.
- Treating `volatile bool` as thread synchronization.
- Ignoring `SIGTERM` in services that need graceful shutdown.

**Follow-ups:**

- Why is `sig_atomic_t` special?
- When would you prefer `sigaction()`?
- How would you wake a thread blocked on a condition variable during shutdown?

## Coding Tasks

### Task 1. Fix A Broken Counter

**Prompt:** This program prints a wrong result or appears to work by accident.
Fix it two ways: once with a mutex and once with an atomic.

```cpp
int counter = 0;

void inc() {
    for (int i = 0; i < 100000; ++i) {
        ++counter;
    }
}
```

**Expected answer shape:**

- Short answer: non-atomic shared increment is a data race.
- Deep explanation: read-modify-write interleaving and C++ undefined behavior.
- API anchor: `std::lock_guard<std::mutex>` and `std::atomic<int>::fetch_add`.
- Production/debug angle: run with `-fsanitize=thread`.
- Traps: `volatile int`, assuming `++` is atomic, using atomics for compound
  state.
- Follow-ups: ask when each fix is appropriate.

### Task 2. Implement A Blocking Queue With Shutdown

**Prompt:** Implement `push`, `pop`, and `close` for a queue shared between one
producer and multiple consumers.

**Expected answer shape:**

- Short answer: use one mutex, one condition variable, a queue, and a closed
  flag.
- Deep explanation: `pop` waits for `closed || !queue.empty()`.
- API anchor: `std::unique_lock`, `std::condition_variable::wait(lock,
  predicate)`, `notify_one`, `notify_all`.
- Production/debug angle: define post-close `push` behavior and log predicate
  transitions.
- Traps: no predicate, no shutdown wakeup, returning references after unlock.
- Follow-ups: bounded capacity, timeout, cancellation, exception safety.

### Task 3. Diagnose A Deadlock

**Prompt:** Two functions lock `m1` then `m2` and `m2` then `m1`. Explain the
bug and fix it.

**Expected answer shape:**

- Short answer: inconsistent lock order can deadlock.
- Deep explanation: each thread can hold one mutex while waiting for the other.
- API anchor: `std::scoped_lock lock(m1, m2);` or a global lock hierarchy.
- Production/debug angle: inspect thread backtraces and lock ownership.
- Traps: adding sleeps, using `recursive_mutex`, or adding timeouts without
  fixing the cycle.
- Follow-ups: callbacks under lock, lock ordering documentation, testing.

### Task 4. Make Signal Shutdown Safe

**Prompt:** A handler logs to `std::cout`, locks a mutex, and deletes global
objects. Redesign it.

**Expected answer shape:**

- Short answer: handler should only record shutdown intent using signal-safe
  operations.
- Deep explanation: signals interrupt normal execution and non-async-signal-safe
  functions can deadlock or corrupt state.
- API anchor: `volatile std::sig_atomic_t`, `std::signal` or POSIX
  `sigaction()`.
- Production/debug angle: main loop observes the flag, wakes workers through
  normal synchronization, then joins threads.
- Traps: complex cleanup in handler, `volatile bool`, ignoring `SIGTERM`.
- Follow-ups: how to wake condition-variable waiters, how to test shutdown.

## Debugging Scenarios

### Scenario 1. The Program Hangs During Shutdown

**Short answer:** Look for blocked workers, condition-variable waits whose
predicate never changes, missing `notify_all()`, and joins waiting on threads
that cannot exit.

**Deep explanation:** Shutdown is a state transition. The stop flag, task queue,
and worker wait predicate must agree. If `stopping_` is set without notifying
workers, they may sleep forever. If workers do not check `stopping_`, join may
wait forever.

**C/C++ code/API anchor:** `std::condition_variable`, `notify_all()`,
`std::thread::join()`, worker loop predicate `stopping_ || !tasks_.empty()`.

**Production/debug angle:** Capture all thread stacks. Log stop flag changes,
queue size, worker state, and notification points.

**Traps:**

- Logging only "shutdown called" without worker state.
- Holding the queue lock while joining workers.
- Forgetting that a task itself may be blocked.

**Follow-ups:**

- How should a thread pool destructor be ordered?
- Should shutdown drain tasks or cancel them?
- How would you add timeout diagnostics?

### Scenario 2. ThreadSanitizer Reports A Data Race On A Stop Flag

**Short answer:** Replace the shared non-atomic flag with `std::atomic<bool>` or
protect it with the same mutex used by the wait predicate.

**Deep explanation:** A stop flag read by workers and written by another thread
is shared mutable state. If it is a plain `bool` and not protected, it is a data
race. If workers sleep on a condition variable, the flag should usually be under
the same mutex as the predicate.

**C/C++ code/API anchor:**

```cpp
std::atomic<bool> stop{false};
// or:
std::mutex mtx;
bool stop = false; // only read/write while holding mtx
```

**Production/debug angle:** Fix the synchronization model, not only the warning.
If a condition variable is involved, ensure `notify_all()` happens after the
state change.

**Traps:**

- Changing `bool` to `volatile bool`.
- Using atomics and condition variables together without a clear predicate.
- Ignoring the warning because the program "usually exits".

**Follow-ups:**

- Which fix fits a worker pool better?
- What ordering does an atomic stop flag need?
- How would you test this after the fix?

