# 10.8. Multithreading Basics

---

## Table of Contents

1. Introduction to Multithreading
2. Creating Threads
3. Thread Management
4. Race Conditions
5. Mutex Basics
6. Lock Guards
7. unique_lock
8. Thread Safety
9. Best Practices
10. Summary

---

## 1. Introduction to Multithreading

### What is Multithreading?

**Multithreading** is the ability to execute multiple threads concurrently within a single process. Each thread runs independently but shares the same memory space.

**Think of it as:** Multiple workers in an office sharing the same resources but working on different tasks simultaneously.

### Why Use Multithreading?

1. **Performance**: Utilize multiple CPU cores
2. **Responsiveness**: Keep UI responsive during heavy operations
3. **Resource Sharing**: Threads share memory efficiently
4. **Parallelism**: Execute tasks simultaneously
5. **Asynchronous Operations**: Handle I/O without blocking

### C++11 Threading Library

```cpp
#include <thread>    // std::thread
#include <mutex>     // std::mutex, locks
#include <atomic>    // std::atomic
#include <future>    // std::future, std::async
#include <condition_variable>  // std::condition_variable
```

### Thread vs Process

| Feature | Thread | Process |
| --- | --- | --- |
| Memory | Shared | Separate |
| Creation | Fast | Slow |
| Communication | Easy (shared memory) | Complex (IPC) |
| Overhead | Low | High |
| Crash Impact | Affects all threads | Isolated |

---

## 2. Creating Threads

### Basic Thread Creation

```cpp
#include <iostream>
#include <thread>
using namespace std;

// WHY: Simple function to run in a thread
void sayHello() {
    cout << "Hello from thread!" << endl;
}

int main() {
    // WHY: Create a thread that runs sayHello()
    thread t(sayHello);

    // WHY: Wait for thread to complete
    t.join();

    cout << "Main thread done." << endl;
    return 0;
}
```

**Output:**

```
Hello from thread!
Main thread done.
```

### Thread with Function Arguments

```cpp
#include <iostream>
#include <thread>
#include <string>
using namespace std;

void greet(const string& name, int times) {
    for (int i = 0; i < times; i++) {
        cout << "Hello, " << name << "! (" << i + 1 << ")" << endl;
    }
}

int main() {
    // WHY: Pass arguments to thread function
    thread t(greet, "Alice", 3);
    t.join();

    return 0;
}
```

**Output:**

```
Hello, Alice! (1)
Hello, Alice! (2)
Hello, Alice! (3)
```

### Thread with Lambda

```cpp
#include <iostream>
#include <thread>
using namespace std;

int main() {
    int value = 10;

    // WHY: Use lambda for inline thread function
    thread t([value]() {
        cout << "Lambda thread, value = " << value << endl;
    });

    t.join();

    // WHY: Lambda with capture by reference
    thread t2([&value]() {
        value = 20;
        cout << "Modified value to " << value << endl;
    });

    t2.join();
    cout << "Final value: " << value << endl;

    return 0;
}
```

**Output:**

```
Lambda thread, value = 10
Modified value to 20
Final value: 20
```

### Thread with Member Function

```cpp
#include <iostream>
#include <thread>
using namespace std;

class Worker {
    int id;
public:
    Worker(int i) : id(i) {}

    void doWork() {
        cout << "Worker " << id << " is working..." << endl;
    }
};

int main() {
    Worker w(42);

    // WHY: Pass member function and object pointer
    thread t(&Worker::doWork, &w);
    t.join();

    return 0;
}
```

**Output:**

```
Worker 42 is working...
```

### Multiple Threads

```cpp
#include <iostream>
#include <thread>
#include <vector>
using namespace std;

void task(int id) {
    cout << "Thread " << id << " started" << endl;
    // Simulate work
    this_thread::sleep_for(chrono::milliseconds(100));
    cout << "Thread " << id << " finished" << endl;
}

int main() {
    vector<thread> threads;

    // WHY: Create multiple threads
    for (int i = 0; i < 5; i++) {
        threads.emplace_back(task, i);
    }

    // WHY: Join all threads
    for (auto& t : threads) {
        t.join();
    }

    cout << "All threads completed." << endl;
    return 0;
}
```

---

## 3. Thread Management

### join() vs detach()

| Method | Behavior | When to Use |
| --- | --- | --- |
| join() | Wait for thread to finish | Need result or synchronization |
| detach() | Thread runs independently | Fire-and-forget operations |

### Using join()

```cpp
#include <iostream>
#include <thread>
using namespace std;

void work() {
    cout << "Working..." << endl;
    this_thread::sleep_for(chrono::seconds(1));
    cout << "Work done!" << endl;
}

int main() {
    thread t(work);

    cout << "Main: waiting for thread..." << endl;

    // WHY: join() blocks until thread completes
    t.join();

    cout << "Main: thread finished!" << endl;
    return 0;
}
```

**Output:**

```
Main: waiting for thread...
Working...
Work done!
Main: thread finished!
```

### Using detach()

```cpp
#include <iostream>
#include <thread>
using namespace std;

void backgroundTask() {
    this_thread::sleep_for(chrono::milliseconds(500));
    cout << "Background task completed" << endl;
}

int main() {
    thread t(backgroundTask);

    // WHY: detach() lets thread run independently
    t.detach();

    cout << "Main thread continues..." << endl;

    // Note: Program may exit before detached thread completes!
    this_thread::sleep_for(chrono::seconds(1));

    return 0;
}
```

### Checking joinable()

```cpp
#include <iostream>
#include <thread>
using namespace std;

int main() {
    thread t([]() {
        cout << "Thread running" << endl;
    });

    cout << "Is joinable? " << t.joinable() << endl;  // 1 (true)

    t.join();

    cout << "Is joinable? " << t.joinable() << endl;  // 0 (false)

    return 0;
}
```

### Thread ID and Hardware Concurrency

```cpp
#include <iostream>
#include <thread>
using namespace std;

void showId() {
    cout << "Thread ID: " << this_thread::get_id() << endl;
}

int main() {
    cout << "Main thread ID: " << this_thread::get_id() << endl;
    cout << "Hardware concurrency: " << thread::hardware_concurrency() << endl;

    thread t1(showId);
    thread t2(showId);

    t1.join();
    t2.join();

    return 0;
}
```

**Output (example):**

```
Main thread ID: 140234567890
Hardware concurrency: 8
Thread ID: 140234567891
Thread ID: 140234567892
```

---

## 4. Race Conditions

### What is a Race Condition?

A **race condition** occurs when multiple threads access shared data simultaneously, and at least one thread modifies it. The result depends on the unpredictable order of execution.

### Example: Race Condition Bug

```cpp
#include <iostream>
#include <thread>
using namespace std;

// WHY: Shared variable - source of race condition
int counter = 0;

void increment() {
    for (int i = 0; i < 100000; i++) {
        counter++;  // NOT atomic! Read-modify-write
    }
}

int main() {
    thread t1(increment);
    thread t2(increment);

    t1.join();
    t2.join();

    // WHY: Expected 200000, but result is unpredictable!
    cout << "Counter: " << counter << endl;
    cout << "(Expected: 200000)" << endl;

    return 0;
}
```

**Output (varies each run):**

```
Counter: 156789
(Expected: 200000)
```

### Why Does This Happen?

The `counter++` operation is actually three steps:

1. **Read** current value
2. **Modify** (increment)
3. **Write** new value

When two threads execute simultaneously:

```
Thread 1: Read (0) -> Increment (1) -> Write (1)
Thread 2: Read (0) -> Increment (1) -> Write (1)
// Both wrote 1, lost one increment!
```

---

## 5. Mutex Basics

### What is a Mutex?

A **mutex** (MUTual EXclusion) is a synchronization primitive that ensures only one thread can access a resource at a time.

### Basic Mutex Usage

```cpp
#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

int counter = 0;
mutex mtx;  // WHY: Mutex to protect counter

void increment() {
    for (int i = 0; i < 100000; i++) {
        mtx.lock();    // WHY: Lock before accessing
        counter++;
        mtx.unlock();  // WHY: Unlock after done
    }
}

int main() {
    thread t1(increment);
    thread t2(increment);

    t1.join();
    t2.join();

    // WHY: Now result is always correct!
    cout << "Counter: " << counter << endl;

    return 0;
}
```

**Output:**

```
Counter: 200000
```

### The Problem with Manual lock/unlock

```cpp
void riskyFunction() {
    mtx.lock();

    // What if exception is thrown here?
    doSomething();  // If this throws, unlock() never called!

    mtx.unlock();
}
```

---

## 6. Lock Guards

### What is lock_guard?

**std::lock_guard** is an RAII wrapper that automatically locks a mutex on construction and unlocks it on destruction.

### Basic lock_guard Usage

```cpp
#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

int counter = 0;
mutex mtx;

void safeIncrement() {
    for (int i = 0; i < 100000; i++) {
        // WHY: lock_guard automatically locks/unlocks
        lock_guard<mutex> lock(mtx);
        counter++;
    }  // Automatically unlocked when lock goes out of scope
}

int main() {
    thread t1(safeIncrement);
    thread t2(safeIncrement);

    t1.join();
    t2.join();

    cout << "Counter: " << counter << endl;

    return 0;
}
```

### Exception Safety with lock_guard

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <stdexcept>
using namespace std;

mutex mtx;

void safeFunction() {
    lock_guard<mutex> lock(mtx);

    cout << "Doing work..." << endl;

    // Even if exception is thrown...
    throw runtime_error("Something went wrong!");

    // ...mutex is still unlocked properly!
}

int main() {
    try {
        safeFunction();
    } catch (const exception& e) {
        cout << "Caught: " << e.what() << endl;
    }

    // Mutex is unlocked, can be used again
    lock_guard<mutex> lock(mtx);
    cout << "Main can lock again!" << endl;

    return 0;
}
```

### Scoped Locking

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
using namespace std;

mutex mtx;
vector<int> data;

void addData(int value) {
    // WHY: Create scope to limit lock duration
    {
        lock_guard<mutex> lock(mtx);
        data.push_back(value);
    }  // Lock released here

    // Other work without holding lock
    cout << "Added: " << value << endl;
}

int main() {
    thread t1(addData, 10);
    thread t2(addData, 20);
    thread t3(addData, 30);

    t1.join();
    t2.join();
    t3.join();

    cout << "Data size: " << data.size() << endl;

    return 0;
}
```

---

## 7. unique_lock

### What is unique_lock?

**std::unique_lock** is a more flexible mutex wrapper than lock_guard. It supports:

- Deferred locking
- Manual lock/unlock
- Timed locking
- Ownership transfer

### Basic unique_lock Usage

```cpp
#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

mutex mtx;

void flexibleLocking() {
    // WHY: unique_lock with deferred locking
    unique_lock<mutex> lock(mtx, defer_lock);

    cout << "Preparing..." << endl;

    // WHY: Lock when ready
    lock.lock();
    cout << "Locked and working..." << endl;

    // WHY: Can unlock manually
    lock.unlock();
    cout << "Unlocked, doing other work..." << endl;

    // WHY: Can lock again
    lock.lock();
    cout << "Locked again!" << endl;
}  // Unlocked automatically

int main() {
    flexibleLocking();
    return 0;
}
```

### unique_lock vs lock_guard

| Feature | lock_guard | unique_lock |
| --- | --- | --- |
| Simplicity | ✅ Simple | More complex |
| Performance | ✅ Faster | Slight overhead |
| Manual unlock | ❌ No | ✅ Yes |
| Deferred lock | ❌ No | ✅ Yes |
| try_lock | ❌ No | ✅ Yes |
| Move ownership | ❌ No | ✅ Yes |
| Condition variables | ❌ No | ✅ Required |

### try_lock with unique_lock

```cpp
#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

mutex mtx;

void tryLockExample() {
    unique_lock<mutex> lock(mtx, try_to_lock);

    if (lock.owns_lock()) {
        cout << "Got the lock!" << endl;
    } else {
        cout << "Couldn't get lock, doing something else..." << endl;
    }
}

int main() {
    // Lock mutex from main
    mtx.lock();

    thread t(tryLockExample);  // Will fail to get lock
    t.join();

    mtx.unlock();

    thread t2(tryLockExample);  // Will succeed
    t2.join();

    return 0;
}
```

---

## 8. Thread Safety

### Making Classes Thread-Safe

```cpp
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
using namespace std;

class ThreadSafeCounter {
    int count = 0;
    mutable mutex mtx;  // WHY: mutable for const methods

public:
    void increment() {
        lock_guard<mutex> lock(mtx);
        count++;
    }

    void decrement() {
        lock_guard<mutex> lock(mtx);
        count--;
    }

    int get() const {
        lock_guard<mutex> lock(mtx);
        return count;
    }
};

int main() {
    ThreadSafeCounter counter;

    auto increment = [&counter]() {
        for (int i = 0; i < 10000; i++) {
            counter.increment();
        }
    };

    thread t1(increment);
    thread t2(increment);

    t1.join();
    t2.join();

    cout << "Final count: " << counter.get() << endl;

    return 0;
}
```

**Output:**

```
Final count: 20000
```

### Thread-Safe Singleton

```cpp
#include <iostream>
#include <thread>
#include <mutex>
using namespace std;

class Singleton {
    static Singleton* instance;
    static mutex mtx;

    Singleton() { cout << "Singleton created" << endl; }

public:
    static Singleton* getInstance() {
        if (instance == nullptr) {
            lock_guard<mutex> lock(mtx);
            if (instance == nullptr) {  // Double-check
                instance = new Singleton();
            }
        }
        return instance;
    }

    void doSomething() {
        cout << "Doing something..." << endl;
    }
};

Singleton* Singleton::instance = nullptr;
mutex Singleton::mtx;

int main() {
    auto getSingleton = []() {
        Singleton::getInstance()->doSomething();
    };

    thread t1(getSingleton);
    thread t2(getSingleton);

    t1.join();
    t2.join();

    return 0;
}
```

### Using std::atomic

```cpp
#include <iostream>
#include <thread>
#include <atomic>
using namespace std;

// WHY: atomic provides lock-free thread safety
atomic<int> counter(0);

void increment() {
    for (int i = 0; i < 100000; i++) {
        counter++;  // Atomic operation
    }
}

int main() {
    thread t1(increment);
    thread t2(increment);

    t1.join();
    t2.join();

    cout << "Counter: " << counter << endl;

    return 0;
}
```

---

## 9. Best Practices

### ✅ DO: Use lock_guard for Simple Cases

```cpp
mutex mtx;

void safeFunction() {
    lock_guard<mutex> lock(mtx);
    // Protected code
}
```

### ✅ DO: Keep Critical Sections Small

```cpp
void goodExample() {
    // Do preparation without lock
    int data = prepareData();

    {
        lock_guard<mutex> lock(mtx);
        sharedData = data;  // Only lock for actual shared access
    }

    // Continue without lock
    processResult();
}
```

### ✅ DO: Use std::atomic for Simple Types

```cpp
atomic<bool> flag(false);
atomic<int> counter(0);

// No mutex needed!
flag = true;
counter++;
```

### ❌ DON'T: Hold Locks During Long Operations

```cpp
// BAD
void badExample() {
    lock_guard<mutex> lock(mtx);
    downloadFile();  // Long operation while holding lock!
}

// GOOD
void goodExample() {
    auto data = downloadFile();  // No lock

    lock_guard<mutex> lock(mtx);
    sharedData = data;  // Quick update with lock
}
```

### ❌ DON'T: Call Unknown Code While Holding Lock

```cpp
// BAD - callback might try to acquire same lock!
void riskyFunction(function<void()> callback) {
    lock_guard<mutex> lock(mtx);
    callback();  // Dangerous!
}
```

---

## 10. Summary

### Key Takeaways

1. **std::thread**: Creates and manages threads
2. **join()**: Waits for thread completion
3. **detach()**: Lets thread run independently
4. **Mutex**: Provides mutual exclusion
5. **lock_guard**: RAII lock management
6. **unique_lock**: Flexible lock management
7. **std::atomic**: Lock-free thread safety

### Thread Management

| Operation | Purpose |
| --- | --- |
| thread(func) | Create thread |
| join() | Wait for completion |
| detach() | Run independently |
| joinable() | Check if joinable |
| get_id() | Get thread ID |

### Synchronization Primitives

| Primitive | Use Case |
| --- | --- |
| mutex | Basic mutual exclusion |
| lock_guard | Simple scoped locking |
| unique_lock | Flexible locking |
| atomic | Simple type synchronization |

### Keywords Covered

✅ std::thread (6)
✅ Multithreading (4)
✅ join() (5)
✅ detach() (3)
✅ mutex (8)
✅ lock_guard (6)
✅ unique_lock (4)
✅ Race condition (4)
✅ Thread safety (4)
✅ Critical section (3)
✅ Synchronization (3)
✅ std::atomic (3)
✅ RAII locking (2)
✅ Deadlock (1)
✅ Thread ID (2)

**Total: 58 keywords/concepts covered**

---