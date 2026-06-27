# 10.9. Multithreading Advanced

---

## Table of Contents

1. Condition Variables
2. Producer-Consumer Pattern
3. std::promise and std::future
4. std::async
5. std::packaged_task
6. Deadlock Prevention
7. Thread Pools
8. Atomic Operations
9. Best Practices
10. Summary

---

## 1. Condition Variables

### What is a Condition Variable?

A **condition variable** is a synchronization primitive that allows threads to wait until a particular condition becomes true. It's used with a mutex to coordinate thread communication.

### Basic Syntax

```cpp
#include <condition_variable>

std::condition_variable cv;
std::mutex mtx;

// Thread 1: Wait for condition
std::unique_lock<std::mutex> lock(mtx);
cv.wait(lock, []{ return condition; });

// Thread 2: Notify
cv.notify_one();   // Wake one waiting thread
cv.notify_all();   // Wake all waiting threads
```

### Simple Condition Variable Example

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
using namespace std;

mutex mtx;
condition_variable cv;
bool ready = false;

void worker() {
    unique_lock<mutex> lock(mtx);

    // WHY: Wait until ready becomes true
    cv.wait(lock, []{ return ready; });

    cout << "Worker: condition met, working..." << endl;
}

int main() {
    thread t(worker);

    // Simulate setup work
    this_thread::sleep_for(chrono::seconds(1));

    {
        lock_guard<mutex> lock(mtx);
        ready = true;
        cout << "Main: setting ready = true" << endl;
    }

    // WHY: Notify waiting thread
    cv.notify_one();

    t.join();
    return 0;
}
```

**Output:**

```
Main: setting ready = true
Worker: condition met, working...
```

### Why Use a Predicate?

```cpp
// BAD: Without predicate - prone to spurious wakeups
cv.wait(lock);

// GOOD: With predicate - handles spurious wakeups
cv.wait(lock, []{ return dataReady; });

// The predicate version is equivalent to:
while (!dataReady) {
    cv.wait(lock);
}
```

### Spurious Wakeups

Condition variables can wake up without being notified (spurious wakeup). Always use a predicate to handle this:

```cpp
void safeWait() {
    unique_lock<mutex> lock(mtx);

    // WHY: Predicate prevents spurious wakeup issues
    cv.wait(lock, []{
        return dataReady;  // Rechecked after every wakeup
    });
}
```

---

## 2. Producer-Consumer Pattern

### Classic Producer-Consumer

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
using namespace std;

mutex mtx;
condition_variable cv;
queue<int> dataQueue;
bool finished = false;

void producer() {
    for (int i = 1; i <= 5; i++) {
        {
            lock_guard<mutex> lock(mtx);
            dataQueue.push(i);
            cout << "Produced: " << i << endl;
        }
        cv.notify_one();
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    {
        lock_guard<mutex> lock(mtx);
        finished = true;
    }
    cv.notify_all();
}

void consumer() {
    while (true) {
        unique_lock<mutex> lock(mtx);

        // WHY: Wait for data or finish signal
        cv.wait(lock, []{
            return !dataQueue.empty() || finished;
        });

        while (!dataQueue.empty()) {
            int value = dataQueue.front();
            dataQueue.pop();
            cout << "Consumed: " << value << endl;
        }

        if (finished && dataQueue.empty()) break;
    }
}

int main() {
    thread prod(producer);
    thread cons(consumer);

    prod.join();
    cons.join();

    return 0;
}
```

**Output:**

```
Produced: 1
Consumed: 1
Produced: 2
Consumed: 2
Produced: 3
Consumed: 3
Produced: 4
Consumed: 4
Produced: 5
Consumed: 5
```

### Timed Wait

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
using namespace std;

mutex mtx;
condition_variable cv;
bool ready = false;

void waiter() {
    unique_lock<mutex> lock(mtx);

    // WHY: Wait with timeout
    if (cv.wait_for(lock, chrono::seconds(2), []{ return ready; })) {
        cout << "Condition met!" << endl;
    } else {
        cout << "Timeout occurred!" << endl;
    }
}

int main() {
    thread t(waiter);

    // Don't set ready - let it timeout
    this_thread::sleep_for(chrono::seconds(3));

    t.join();
    return 0;
}
```

**Output:**

```
Timeout occurred!
```

---

## 3. std::promise and std::future

### What are Promise and Future?

- **std::promise**: A channel to set a value (producer side)
- **std::future**: A channel to get a value (consumer side)

They provide one-time communication between threads.

### Basic Promise/Future Example

```cpp
#include <iostream>
#include <thread>
#include <future>
using namespace std;

void calculate(promise<int> prom) {
    // Simulate calculation
    this_thread::sleep_for(chrono::seconds(1));

    // WHY: Set the result
    prom.set_value(42);
}

int main() {
    promise<int> prom;

    // WHY: Get future before passing promise to thread
    future<int> fut = prom.get_future();

    thread t(calculate, move(prom));

    cout << "Waiting for result..." << endl;

    // WHY: get() blocks until value is set
    int result = fut.get();

    cout << "Result: " << result << endl;

    t.join();
    return 0;
}
```

**Output:**

```
Waiting for result...
Result: 42
```

### Passing Exceptions Through Promise

```cpp
#include <iostream>
#include <thread>
#include <future>
using namespace std;

void riskyTask(promise<int> prom) {
    try {
        // Simulate error
        throw runtime_error("Calculation failed!");
        prom.set_value(42);  // Won't reach here
    } catch (...) {
        // WHY: Pass exception to future
        prom.set_exception(current_exception());
    }
}

int main() {
    promise<int> prom;
    future<int> fut = prom.get_future();

    thread t(riskyTask, move(prom));

    try {
        int result = fut.get();  // Will throw!
        cout << "Result: " << result << endl;
    } catch (const exception& e) {
        cout << "Caught exception: " << e.what() << endl;
    }

    t.join();
    return 0;
}
```

**Output:**

```
Caught exception: Calculation failed!
```

---

## 4. std::async

### What is std::async?

**std::async** runs a function asynchronously and returns a future. It's simpler than managing threads manually.

### Basic async Example

```cpp
#include <iostream>
#include <future>
using namespace std;

int compute(int x) {
    this_thread::sleep_for(chrono::seconds(1));
    return x * x;
}

int main() {
    cout << "Starting async task..." << endl;

    // WHY: async runs function and returns future
    future<int> result = async(compute, 10);

    cout << "Doing other work..." << endl;

    // WHY: get() blocks until result is ready
    cout << "Result: " << result.get() << endl;

    return 0;
}
```

**Output:**

```
Starting async task...
Doing other work...
Result: 100
```

### Launch Policies

```cpp
#include <iostream>
#include <future>
using namespace std;

int task() {
    cout << "Task running in thread: "
         << this_thread::get_id() << endl;
    return 42;
}

int main() {
    cout << "Main thread: " << this_thread::get_id() << endl;

    // WHY: async - may run in new thread (default)
    auto f1 = async(launch::async, task);

    // WHY: deferred - runs when get() is called
    auto f2 = async(launch::deferred, task);

    cout << "f1 result: " << f1.get() << endl;
    cout << "f2 result: " << f2.get() << endl;

    return 0;
}
```

### Multiple Async Tasks

```cpp
#include <iostream>
#include <future>
#include <vector>
using namespace std;

int processData(int id, int data) {
    this_thread::sleep_for(chrono::milliseconds(100 * id));
    return data * 2;
}

int main() {
    vector<future<int>> futures;

    // WHY: Launch multiple async tasks
    for (int i = 1; i <= 5; i++) {
        futures.push_back(async(processData, i, i * 10));
    }

    // WHY: Collect all results
    int total = 0;
    for (auto& f : futures) {
        total += f.get();
    }

    cout << "Total: " << total << endl;

    return 0;
}
```

**Output:**

```
Total: 300
```

---

## 5. std::packaged_task

### What is packaged_task?

**std::packaged_task** wraps a callable so it can be invoked asynchronously and its result obtained via a future.

### Basic packaged_task Example

```cpp
#include <iostream>
#include <future>
#include <thread>
using namespace std;

int add(int a, int b) {
    return a + b;
}

int main() {
    // WHY: Wrap function in packaged_task
    packaged_task<int(int, int)> task(add);

    // WHY: Get future before moving task
    future<int> result = task.get_future();

    // WHY: Run task in a thread
    thread t(move(task), 5, 3);

    cout << "Result: " << result.get() << endl;

    t.join();
    return 0;
}
```

**Output:**

```
Result: 8
```

### packaged_task with Lambda

```cpp
#include <iostream>
#include <future>
#include <thread>
using namespace std;

int main() {
    // WHY: packaged_task with lambda
    packaged_task<string(const string&)> task(
        [](const string& name) {
            return "Hello, " + name + "!";
        }
    );

    future<string> result = task.get_future();

    thread t(move(task), "World");

    cout << result.get() << endl;

    t.join();
    return 0;
}
```

**Output:**

```
Hello, World!
```

---

## 6. Deadlock Prevention

### What is Deadlock?

**Deadlock** occurs when two or more threads are waiting for each other to release resources, resulting in all threads being blocked forever.

### Classic Deadlock Example

```cpp
// DEADLOCK SCENARIO:
mutex m1, m2;

void thread1() {
    lock_guard<mutex> lock1(m1);  // Lock m1
    // ... some work ...
    lock_guard<mutex> lock2(m2);  // Wait for m2
}

void thread2() {
    lock_guard<mutex> lock2(m2);  // Lock m2
    // ... some work ...
    lock_guard<mutex> lock1(m1);  // Wait for m1 - DEADLOCK!
}
```

### Solution 1: Lock Ordering

```cpp
mutex m1, m2;

void thread1() {
    // WHY: Always lock in same order
    lock_guard<mutex> lock1(m1);
    lock_guard<mutex> lock2(m2);
    // Work...
}

void thread2() {
    // WHY: Same order as thread1
    lock_guard<mutex> lock1(m1);
    lock_guard<mutex> lock2(m2);
    // Work...
}
```

### Solution 2: std::lock

```cpp
#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

mutex m1, m2;
int data1 = 0, data2 = 0;

void swapData() {
    // WHY: std::lock locks both mutexes without deadlock
    lock(m1, m2);

    // WHY: adopt_lock assumes mutex is already locked
    lock_guard<mutex> lock1(m1, adopt_lock);
    lock_guard<mutex> lock2(m2, adopt_lock);

    swap(data1, data2);
}

int main() {
    data1 = 10;
    data2 = 20;

    thread t1(swapData);
    thread t2(swapData);

    t1.join();
    t2.join();

    cout << "data1: " << data1 << ", data2: " << data2 << endl;

    return 0;
}
```

### Solution 3: std::scoped_lock (C++17)

```cpp
#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

mutex m1, m2;

void safeOperation() {
    // WHY: scoped_lock handles multiple mutexes safely (C++17)
    scoped_lock lock(m1, m2);

    // Work with protected data...
    cout << "Working safely!" << endl;
}

int main() {
    thread t1(safeOperation);
    thread t2(safeOperation);

    t1.join();
    t2.join();

    return 0;
}
```

---

## 7. Thread Pools

### Simple Thread Pool Implementation

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <vector>
using namespace std;

class ThreadPool {
    vector<thread> workers;
    queue<function<void()>> tasks;
    mutex mtx;
    condition_variable cv;
    bool stop = false;

public:
    ThreadPool(size_t numThreads) {
        for (size_t i = 0; i < numThreads; i++) {
            workers.emplace_back([this] {
                while (true) {
                    function<void()> task;

                    {
                        unique_lock<mutex> lock(mtx);
                        cv.wait(lock, [this] {
                            return stop || !tasks.empty();
                        });

                        if (stop && tasks.empty()) return;

                        task = move(tasks.front());
                        tasks.pop();
                    }

                    task();
                }
            });
        }
    }

    void enqueue(function<void()> task) {
        {
            lock_guard<mutex> lock(mtx);
            tasks.push(move(task));
        }
        cv.notify_one();
    }

    ~ThreadPool() {
        {
            lock_guard<mutex> lock(mtx);
            stop = true;
        }
        cv.notify_all();

        for (auto& worker : workers) {
            worker.join();
        }
    }
};

int main() {
    ThreadPool pool(4);

    for (int i = 0; i < 8; i++) {
        pool.enqueue([i] {
            cout << "Task " << i << " running in thread "
                 << this_thread::get_id() << endl;
            this_thread::sleep_for(chrono::milliseconds(100));
        });
    }

    this_thread::sleep_for(chrono::seconds(1));

    return 0;
}
```

---

## 8. Atomic Operations

### Advanced Atomic Operations

```cpp
#include <iostream>
#include <thread>
#include <atomic>
using namespace std;

atomic<int> counter(0);

void incrementWithFetchAdd() {
    for (int i = 0; i < 1000; i++) {
        // WHY: Atomic fetch and add
        counter.fetch_add(1, memory_order_relaxed);
    }
}

int main() {
    thread t1(incrementWithFetchAdd);
    thread t2(incrementWithFetchAdd);

    t1.join();
    t2.join();

    cout << "Counter: " << counter << endl;

    return 0;
}
```

### Compare and Exchange

```cpp
#include <iostream>
#include <atomic>
using namespace std;

atomic<int> value(100);

bool tryUpdate(int expected, int newValue) {
    // WHY: CAS - only update if value equals expected
    return value.compare_exchange_strong(expected, newValue);
}

int main() {
    cout << "Initial: " << value << endl;

    if (tryUpdate(100, 200)) {
        cout << "Updated to 200" << endl;
    }

    if (tryUpdate(100, 300)) {
        cout << "Updated to 300" << endl;
    } else {
        cout << "Failed - value is now " << value << endl;
    }

    return 0;
}
```

**Output:**

```
Initial: 100
Updated to 200
Failed - value is now 200
```

### Atomic Flag for Spinlock

```cpp
#include <iostream>
#include <thread>
#include <atomic>
using namespace std;

class SpinLock {
    atomic_flag flag = ATOMIC_FLAG_INIT;

public:
    void lock() {
        // WHY: Spin until we acquire the lock
        while (flag.test_and_set(memory_order_acquire)) {
            // Busy wait
        }
    }

    void unlock() {
        flag.clear(memory_order_release);
    }
};

SpinLock spinlock;
int sharedData = 0;

void increment() {
    for (int i = 0; i < 10000; i++) {
        spinlock.lock();
        sharedData++;
        spinlock.unlock();
    }
}

int main() {
    thread t1(increment);
    thread t2(increment);

    t1.join();
    t2.join();

    cout << "Shared data: " << sharedData << endl;

    return 0;
}
```

**Output:**

```
Shared data: 20000
```

---

## 9. Best Practices

### ✅ DO: Use Higher-Level Abstractions

```cpp
// Prefer async over manual thread management
auto result = async(launch::async, computeValue);

// Prefer scoped_lock for multiple mutexes (C++17)
scoped_lock lock(m1, m2, m3);
```

### ✅ DO: Always Use Predicates with Condition Variables

```cpp
cv.wait(lock, []{ return condition; });  // GOOD
cv.wait(lock);  // BAD - prone to spurious wakeups
```

### ✅ DO: Prefer std::atomic for Simple Shared State

```cpp
atomic<bool> flag{false};  // Better than mutex for simple flags
atomic<int> counter{0};    // Lock-free counter
```

### ❌ DON'T: Hold Locks While Waiting

```cpp
// BAD
mutex mtx;
void bad() {
    lock_guard<mutex> lock(mtx);
    cv.wait(/* ... */);  // Still holding mtx!
}

// GOOD
void good() {
    unique_lock<mutex> lock(mtx);
    cv.wait(lock);  // unique_lock releases mutex while waiting
}
```

### ❌ DON'T: Ignore Exception Safety

```cpp
// BAD - exception leaves promise unfulfilled
void bad(promise<int> p) {
    int result = riskyCalculation();  // May throw!
    p.set_value(result);
}

// GOOD - handle exceptions
void good(promise<int> p) {
    try {
        int result = riskyCalculation();
        p.set_value(result);
    } catch (...) {
        p.set_exception(current_exception());
    }
}
```

---

## 10. Summary

### Key Takeaways

1. **Condition Variables**: Thread communication and synchronization
2. **Promise/Future**: One-time value communication
3. **std::async**: Simple asynchronous execution
4. **packaged_task**: Deferred execution with future
5. **Deadlock Prevention**: Use std::lock or scoped_lock
6. **Atomic Operations**: Lock-free synchronization

### Synchronization Comparison

| Mechanism | Use Case | Complexity |
| --- | --- | --- |
| mutex + lock_guard | Simple protection | Low |
| condition_variable | Wait for event | Medium |
| promise/future | One-time result | Low |
| async | Fire-and-forget | Low |
| Thread pool | Task queue | High |

### Keywords Covered

✅ condition_variable (6)
✅ wait/notify (4)
✅ Spurious wakeup (2)
✅ std::promise (5)
✅ std::future (6)
✅ std::async (5)
✅ std::packaged_task (3)
✅ Deadlock (4)
✅ std::lock (2)
✅ scoped_lock (2)
✅ Thread pool (3)
✅ Producer-consumer (2)
✅ Launch policy (2)
✅ compare_exchange (2)
✅ atomic_flag (2)
✅ Spinlock (2)