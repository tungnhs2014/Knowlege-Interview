# 5. Thread

## 5.1. Introduction and Working Principles
### 5.1.1. What is a Thread?
A thread is the smallest unit of execution within a process. A process can have multiple threads, all sharing the same resources but executing independently. Using threads can enhance performance by efficiently utilizing CPU resources and enabling parallel processing.

### 5.1.2. Operating Principles
Similar to processes, threads are created to handle multiple tasks simultaneously (multi-tasking). Key characteristics of threads include:

- A **process** is a program in execution, utilizing system resources.
- A **thread** is a lightweight execution unit within a process and can be scheduled independently.
- On a **multi-core** system, multiple threads can run truly in parallel, increasing computational efficiency.
- If one thread is blocked, other threads within the same process can continue executing.
- Each thread has its own **stack segment**, separate from other threads in the same process.
- **Context switching** occurs when switching between threads to preserve their execution state.
  
<p align="center">
  <img src="https://github.com/user-attachments/assets/dc6703ae-20b2-473b-a5d0-60cc2e149aa4" width="70%" alt="Process and Thread Structure">
</p>

### 5.1.3. Memory Management in Multithreading
In a multithreaded process, memory is organized as follows:
- **Code Segment (Text Segment)**: Stores executable instructions shared among all threads.
- **Data Segment (Initialized & Uninitialized Data - BSS)**: Contains global and static variables accessible to all threads.
- **Heap**: Used for dynamic memory allocation, shared among all threads.
- **Stack**: Each thread has its own stack, used for local variables and function return addresses.

---

## 5.2. Concurrency, Parallelism, and Context Switching
### 5.2.1. Concurrency vs. Parallelism
Understanding the difference between **concurrency** and **parallelism** is crucial when working with threads:

- **Concurrency** refers to handling multiple tasks by switching between them using **context switching**. In a single-core CPU, concurrency creates an illusion of simultaneous execution, but only one task is executed at a time.
- **Parallelism** means executing multiple tasks simultaneously using multiple processing units. For example, Task 1 runs on CPU Core 1 while Task 2 runs on CPU Core 2 at the same time, without requiring context switching between them.

<p align="center">
  <img src="https://github.com/user-attachments/assets/557ec12a-b86d-4300-bca2-ad549a876d4c" width="70%" alt="Concurrency vs Parallelism">
</p>

#### Example:
- **Concurrent execution**: A single-threaded web server handling multiple requests by switching between them rapidly.
- **Parallel execution**: A multi-threaded video processing application encoding different video frames on separate CPU cores.

### 5.2.2. Context Switching
**Context switching** is the process of storing and restoring the state of a thread or process when switching execution between them. It enables multitasking by allowing the CPU to execute multiple threads or processes efficiently.

#### Context Switching Process:
1. **Save the current thread/process state**: Store information such as the program counter (PC), registers, and stack memory.
2. **Load the next thread/process state**: Restore previously saved information of the new thread/process to be executed.
3. **Resume execution**: The CPU continues execution from the saved state of the new thread/process.

#### Reasons for Context Switching:
- **Multitasking**: The CPU needs to switch between processes or threads to optimize execution time.
- **Interrupts**: External events (I/O, system signals) require the CPU to pause execution and handle the event.
- **Synchronization**: When a thread waits for a locked resource (mutex, semaphore), the CPU can switch to another thread.

#### Disadvantages of Context Switching:
- **CPU Overhead**: Storing and restoring thread states consumes processing time.
- **Memory Usage**: Requires space to save multiple thread/process states.

---

## 5.3. Thread States and Lifecycle
A thread can be in different states during its lifecycle:
- **Running**: Actively executing on the CPU.
- **Ready**: Ready to execute but waiting for CPU allocation.
- **Blocked**: Waiting for a resource (I/O, mutex, semaphore).
- **Suspended**: Temporarily paused but can resume execution later.
- **Terminated**: Finished execution or forcefully stopped.

<p align="center">
  <img src="https://github.com/user-attachments/assets/cb2faf27-01e7-42fa-ab5e-3d4a82c1e972" width="70%" alt="Thread Lifecycle">
</p>

### 5.3.1. Full Thread Lifecycle
The thread lifecycle consists of multiple states and transitions:

1. **Creation (Create)**: A thread starts in the **Ready** state after being created.
2. **State Transitions**:
   - **Ready → Running**: The scheduler selects the thread for execution.
   - **Running → Ready**: The thread is preempted when the scheduler switches to another thread.
   - **Running → Blocked**: The thread waits for a resource.
   - **Blocked → Ready**: The required resource becomes available.
   - **Ready → Suspended**: The thread is temporarily paused.
   - **Suspended → Ready**: The thread resumes execution.
   - **Blocked → Blocked Suspended**: The blocked thread is also suspended.
   - **Blocked Suspended → Blocked**: The thread resumes but remains blocked.
   - **Running → Terminated**: The thread completes execution.

3. **Additional States**:
   - **Ready Suspended**: A ready thread that is paused temporarily.
   - **Blocked Suspended**: A blocked thread that is also suspended.

The Linux operating system manages thread states using the **scheduler** to decide which thread to execute next.

---

## 5.4. Comparison Between Process and Thread

### 5.4.1. Process vs Thread Examples

Suppose we have a **network server** that needs to handle multiple client connections simultaneously. Below are two implementation approaches:

#### Method 1: Using Processes
- When a client connects, the server calls `fork()` to create a new process.
- Each process has its own separate memory space and handles clients independently.
- If one process crashes, other processes remain unaffected.
- However, creating a process consumes more resources due to memory and execution environment duplication.

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void handle_client() {
    printf("Process %d is handling client\n", getpid());
    sleep(2);
    printf("Process %d completed\n", getpid());
}

int main() {
    for (int i = 0; i < 3; i++) {
        pid_t pid = fork();
        if (pid == 0) {  // Child process
            handle_client();
            exit(0);
        }
    }
    return 0;
}
```

#### Method 2: Using Threads
- When a client connects, the server creates a new thread to handle it within the same process.
- Threads share common memory, making client communication easier.
- If one thread crashes, it may affect the entire process.
- Creating threads is faster and consumes fewer resources compared to processes.

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void *handle_client(void *arg) {
    printf("Thread %lu is handling client\n", pthread_self());
    sleep(2);
    printf("Thread %lu completed\n", pthread_self());
    return NULL;
}

int main() {
    pthread_t threads[3];
    for (int i = 0; i < 3; i++) {
        pthread_create(&threads[i], NULL, handle_client, NULL);
    }
    for (int i = 0; i < 3; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}
```

### 5.4.2. Comparison Between Process and Thread
| Criteria | Process | Thread |
|----------|---------|--------|
| **Memory** | Each process has its own separate memory space. | Threads share memory within the same process. |
| **Creation** | More resource-intensive due to memory and execution environment duplication. | Lightweight, as memory duplication is not required. |
| **Inter-Process Communication (IPC)** | Difficult, requires mechanisms like pipes, message queues, shared memory. | Easy, as threads use shared memory. |
| **Performance** | Slower due to process context switching. | Faster due to lower context switching overhead. |
| **Fault Impact** | A process crash does not affect other processes. | A thread crash may bring down the entire process. |
| **ID** | Each process has a unique **Process ID (PID)**. | Each thread has a unique **Thread ID (TID)** within a process. |

### 5.4.3. Thread ID in Multithreading Programming
Each thread in a process has a **Thread ID (TID)** for identification. Key characteristics:

- **PID (Process ID)** is globally unique, while **TID (Thread ID)** is unique only within a process.
- **PID** is an integer, but **TID** may be a structure depending on the system.
- **PID** is easy to print, whereas **TID** requires specific functions to retrieve its value.

#### 5.4.3.1. Retrieving and Comparing Thread ID in POSIX Threads (pthreads)
In pthreads, Thread ID is represented using `pthread_t`. Direct comparison is not allowed; instead, specific functions are used:

- **pthread_self()**: Returns the Thread ID of the calling thread.
- **pthread_equal(thread1, thread2)**: Compares two Thread IDs.

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void *thread_func(void *arg) {
    pthread_t tid = pthread_self();
    printf("Thread ID: %lu\n", tid);
    return (void*)tid;
}

int main() {
    pthread_t thread1, thread2;
    void *ret1, *ret2;
    
    pthread_create(&thread1, NULL, thread_func, NULL);
    pthread_create(&thread2, NULL, thread_func, NULL);
    
    pthread_join(thread1, &ret1);
    pthread_join(thread2, &ret2);
    
    if (pthread_equal((pthread_t)ret1, (pthread_t)ret2)) {
        printf("Thread 1 and Thread 2 have the same ID\n");
    } else {
        printf("Thread 1 and Thread 2 have different IDs\n");
    }
    return 0;
}
```

### 5.4.4. When to Use Process or Thread?
| Scenario | Use Process | Use Thread |
|-----------|-------------|------------|
| **Parallel execution on multiple CPU cores** | ✅ | ✅ |
| **Easy data sharing** | ❌ (IPC is complex) | ✅ (Shared memory) |
| **Fault isolation** | ✅ (Process crash does not affect others) | ❌ (Thread crash may impact the whole process) |
| **Fast creation, low resource usage** | ❌ | ✅ |

---

## 5.5. Working with Threads in Linux

### 5.5.1. Creating a New Thread
A program starts execution as a **single-threaded** process, meaning it has only one execution flow. If a process creates additional threads, it becomes a **multi-threaded** process.

Every process has at least one thread, which is called the **main thread**, responsible for executing the `main()` function.

<p align="center">
  <img src="https://github.com/user-attachments/assets/d8124904-927e-4461-96ea-ed8ff9e2a248" width="70%" alt="Thread Creation">
</p>

To create a new thread in Linux, use the `pthread_create()` function:

```c
int pthread_create(pthread_t *threadID, const pthread_attr_t *attr, void *(*start)(void *), void *arg);
```

#### Parameters:
- **threadID**: A pointer to a `pthread_t` variable that stores the ID of the newly created thread.
- **attr**: Specifies thread attributes (usually set to `NULL` for default attributes).
- **start**: A function pointer specifying the function the thread will execute.
- **arg**: A pointer to the argument passed to the thread function (can be `NULL` if no arguments are needed).

#### Example of Creating a Thread:
```c
#include <pthread.h>
#include <stdio.h>

void *print_message(void *msg) {
    printf("%s\n", (char *)msg);
    return NULL;
}

int main() {
    pthread_t thread;
    char *message = "Hello from thread!";
    pthread_create(&thread, NULL, print_message, message);
    pthread_join(thread, NULL);
    return 0;
}
```

### 5.5.2. Terminating a Thread
A thread can be terminated explicitly using `pthread_exit()`, or it will terminate automatically when the function it executes completes.

```c
void pthread_exit(void *retval);
```

#### Parameters:
- **retval**: A pointer to the return value of the thread. The parent thread can retrieve this value using `pthread_join()`.

#### Example of Thread Termination:
```c
#include <pthread.h>
#include <stdio.h>

void *thread_func(void *arg) {
    printf("Thread executing...\n");
    pthread_exit(NULL);
}

int main() {
    pthread_t thread;
    pthread_create(&thread, NULL, thread_func, NULL);
    pthread_join(thread, NULL);
    printf("Thread has terminated.\n");
    return 0;
}
```

In this example:
- The `thread_func()` prints a message and then calls `pthread_exit(NULL)` to terminate.
- The `pthread_join()` function ensures that the main thread waits for the created thread to finish execution before exiting the program.

--- 

## 5.6. Managing Threads

### 5.6.1. Thread Termination Methods
A thread can terminate in multiple ways:
- The thread function completes execution naturally.
- The thread explicitly calls `pthread_exit()`.
- The thread is forcefully canceled using `pthread_cancel()`.
- If any thread calls `exit()`, or if the **main thread** terminates, all remaining threads in the process will be terminated immediately.

### 5.6.2. Using `pthread_exit()`
```c
void pthread_exit(void *retval);
```
This function allows a thread to terminate and return a value that can be collected by another thread using `pthread_join()`.

#### Example:
```c
#include <pthread.h>
#include <stdio.h>

void *thread_func(void *arg) {
    printf("Thread exiting...\n");
    pthread_exit("Thread finished execution");
}

int main() {
    pthread_t thread;
    void *retval;
    
    pthread_create(&thread, NULL, thread_func, NULL);
    pthread_join(thread, &retval);
    printf("Thread returned: %s\n", (char *)retval);
    
    return 0;
}
```

### 5.6.3. Canceling a Thread with `pthread_cancel()`
```c
int pthread_cancel(pthread_t thread);
```
Sends a cancellation request to a specified thread. The thread must check for cancellation points to terminate cleanly.

#### Example:
```c
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

void *thread_func(void *arg) {
    while (1) {
        printf("Thread running...\n");
        sleep(1);
    }
    return NULL;
}

int main() {
    pthread_t thread;
    pthread_create(&thread, NULL, thread_func, NULL);
    sleep(3);
    printf("Canceling thread...\n");
    pthread_cancel(thread);
    pthread_join(thread, NULL);
    printf("Thread canceled.\n");
    return 0;
}
```

### 5.6.4. Joining Threads with `pthread_join()`
```c
int pthread_join(pthread_t thread, void **retval);
```
This function waits for a specific thread to complete and retrieves its return value, preventing zombie threads.

#### Example:
```c
#include <pthread.h>
#include <stdio.h>

void *thread_func(void *arg) {
    return "Thread finished";
}

int main() {
    pthread_t thread;
    void *retval;
    
    pthread_create(&thread, NULL, thread_func, NULL);
    pthread_join(thread, &retval);
    
    printf("Thread returned: %s\n", (char *)retval);
    return 0;
}
```

### 5.6.5. Detaching a Thread with `pthread_detach()`
```c
int pthread_detach(pthread_t thread);
```
A **detached thread** releases its resources automatically when it terminates, preventing it from becoming a zombie thread. Detached threads cannot be `pthread_join()`.

#### Example:
```c
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

void *thread_func(void *arg) {
    printf("Detached thread running...\n");
    sleep(2);
    printf("Detached thread exiting...\n");
    return NULL;
}

int main() {
    pthread_t thread;
    
    pthread_create(&thread, NULL, thread_func, NULL);
    pthread_detach(thread);
    
    printf("Main thread finished. Detached thread will continue execution.\n");
    sleep(3);
    return 0;
}
```

## 5.7. Thread Synchronization
### 5.7.1. Introduction to Thread Synchronization
One of the greatest strengths of multithreading is the ability to share data between threads through global variables. However, this shared access introduces synchronization challenges that must be carefully managed.

When multiple threads attempt to modify the same variable simultaneously or when one thread tries to read a variable while another is modifying it, race conditions can occur. These race conditions lead to unpredictable program behavior, data corruption, and difficult-to-debug issues.

Thread synchronization provides mechanisms to coordinate thread execution and access to shared resources, ensuring data consistency and program correctness.

### 5.7.2. Atomic vs. Nonatomic Operations
#### Atomic Operations
An atomic operation is one that appears to occur instantaneously from the perspective of other threads. Atomic operations guarantee that:
- Only one thread can access a shared resource at any given time
- The operation completes entirely without interruption
- Other threads cannot observe partial results

Atomic operations are thread-safe but may come with performance costs.

#### Nonatomic Operations
Nonatomic operations allow multiple threads to access a shared resource simultaneously. While faster than atomic operations, they are not thread-safe and can lead to:
- **Race conditions:** A race condition occurs when multiple threads access and modify a shared variable at the same time, leading to unpredictable results.
    ```c
    #include <stdio.h>
    #include <pthread.h>

    int counter = 0;  // Shared resource

    void* increment(void* arg) {
        for (int i = 0; i < 1000000; i++) {
            counter++;  // Nonatomic operation
        }
        return NULL;
    }

    int main() {
        pthread_t t1, t2;
        pthread_create(&t1, NULL, increment, NULL);
        pthread_create(&t2, NULL, increment, NULL);
        
        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
        
        printf("Final counter value: %d\n", counter);
        return 0;
    }
    ```
    **Issue:**
    - Since counter++ is not protected, both threads might read the same value, increment it, and write it back, leading to lost updates (data loss).
    - The expected result is 2000000, but the actual result may be lower.

- **Data corruption:** If one thread writes data while another reads it at the same time, the data may become corrupt or inconsistent.

Most operations in C/C++ are nonatomic by default unless explicitly synchronized.

### 5.7.3. Critical Sections
A critical section refers to a segment of code that accesses shared resources and must be executed atomically. During the execution of a critical section:
- Only one thread can execute the code at a time
- The execution cannot be interrupted by another thread accessing the same shared resource
- The section typically starts with acquiring a lock and ends with releasing it

By properly identifying and protecting critical sections, we can ensure thread safety while minimizing performance impact.

### 5.7.4. Mutex (Mutual Exclusion)

A mutex is a synchronization primitive that ensures exclusive access to a shared resource. It acts as a lock that prevents multiple threads from simultaneously executing critical sections.

#### Mutex Implementation Steps

1. **Initialize the mutex**
2. **Lock the mutex** before entering the critical section
3. **Access shared resources** within the critical section
4. **Unlock the mutex** after exiting the critical section

#### Mutex Allocation

Mutexes in POSIX threads are represented by the `pthread_mutex_t` type and must be initialized before use. There are two ways to allocate mutexes:

##### Static Allocation
```c
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
```

##### Dynamic Allocation
```c
pthread_mutex_t mtx;
pthread_mutex_init(&mtx, NULL);  // NULL for default attributes
```

When a dynamically allocated mutex is no longer needed, it should be destroyed:
```c
pthread_mutex_destroy(&mtx);
```

#### Locking and Unlocking a Mutex

After initialization, a mutex is in an unlocked state. To control access to critical sections:

##### Lock a Mutex
```c
int pthread_mutex_lock(pthread_mutex_t *mutex);
```
- If the mutex is unlocked, the function locks it and returns immediately
- If the mutex is already locked by another thread, the calling thread blocks until the mutex becomes available
- Returns 0 on success, negative value on failure

##### Unlock a Mutex
```c
int pthread_mutex_unlock(pthread_mutex_t *mutex);
```
- Releases the lock on the mutex
- Returns 0 on success, negative value on failure

#### Example: Protecting a Counter with Mutex

Here's a simple example demonstrating mutex usage to protect a shared counter:

```c
#include <stdio.h>
#include <pthread.h>

#define NUM_THREADS 5
#define COUNT_MAX 10000

// Shared variable
int counter = 0;
pthread_mutex_t counter_mutex = PTHREAD_MUTEX_INITIALIZER;

void* increment_counter(void* arg) {
    for (int i = 0; i < COUNT_MAX; i++) {
        // Lock mutex before accessing shared resource
        pthread_mutex_lock(&counter_mutex);
        
        // Critical section
        counter++;
        
        // Unlock mutex after accessing shared resource
        pthread_mutex_unlock(&counter_mutex);
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    
    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, increment_counter, NULL);
    }
    
    // Join threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("Expected value: %d\n", NUM_THREADS * COUNT_MAX);
    printf("Actual value: %d\n", counter);
    
    return 0;
}
```

Without the mutex, the final counter value would be unpredictable and likely less than the expected value due to race conditions.

#### Mutex Deadlocks
A deadlock occurs when two or more threads are waiting for each other to release a mutex (mutual exclusion lock), resulting in a permanent blocking state where no progress is made. Deadlocks happen when threads hold some locks while waiting for others, creating a cycle of dependencies.

##### Causes of Deadlocks
1. **Self-deadlock**: A thread locks a mutex and then tries to lock the same mutex again without unlocking it first. Since a regular mutex is non-recursive, the second lock attempt blocks indefinitely, leading to a deadlock.
```c
#include <stdio.h>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void* thread_func(void* arg) {
    pthread_mutex_lock(&mutex);  // First lock
    printf("Thread acquired lock\n");

    pthread_mutex_lock(&mutex);  // Second lock (deadlock)
    printf("This will never print\n");

    pthread_mutex_unlock(&mutex);
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int main() {
    pthread_t thread;
    pthread_create(&thread, NULL, thread_func, NULL);
    pthread_join(thread, NULL);
    return 0;
}
```
- **Issue:** The thread locks mutex, then locks it again without unlocking, causing self-deadlock.
- **Solution:** 
  + Use recursive mutexes (PTHREAD_MUTEX_RECURSIVE) if re-locking is necessary.
  + Always ensure that a mutex is unlocked before trying to lock it again.


2. **Circular wait**: A circular wait occurs when multiple threads hold different mutexes and wait for each other to release them. This creates a deadlock cycle where no thread can proceed.
```c
#include <stdio.h>
#include <pthread.h>

pthread_mutex_t mutexA = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexB = PTHREAD_MUTEX_INITIALIZER;

void* thread1_func(void* arg) {
    pthread_mutex_lock(&mutexA);
    printf("Thread 1 acquired Mutex A\n");

    sleep(1);  // Simulate some work

    pthread_mutex_lock(&mutexB);  // Waiting for mutexB (held by Thread 2)
    printf("Thread 1 acquired Mutex B\n");

    pthread_mutex_unlock(&mutexB);
    pthread_mutex_unlock(&mutexA);
    return NULL;
}

void* thread2_func(void* arg) {
    pthread_mutex_lock(&mutexB);
    printf("Thread 2 acquired Mutex B\n");

    sleep(1);  // Simulate some work

    pthread_mutex_lock(&mutexA);  // Waiting for mutexA (held by Thread 1)
    printf("Thread 2 acquired Mutex A\n");

    pthread_mutex_unlock(&mutexA);
    pthread_mutex_unlock(&mutexB);
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, thread1_func, NULL);
    pthread_create(&thread2, NULL, thread2_func, NULL);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    return 0;
}
```
- **Issue:** 
    + Thread 1 locks Mutex A and waits for Mutex B.
    + Thread 2 locks Mutex B and waits for Mutex A.
    + Both threads wait forever, causing a deadlock.
- **Solution**:
    +  Always lock mutexes in a fixed order (e.g., always lock mutexA before mutexB).
    +  Avoid holding multiple locks at the same time when possible.
    +  Use timeouts with pthread_mutex_timedlock() to detect deadlocks.

##### To avoid deadlocks:
- Always unlock mutexes in the reverse order they were locked
- Use timeouts with `pthread_mutex_timedlock()`
```c
struct timespec timeout;
clock_gettime(CLOCK_REALTIME, &timeout);
timeout.tv_sec += 2; // Timeout after 2 seconds

if (pthread_mutex_timedlock(&mutex, &timeout) == 0) {
    // Critical section
    pthread_mutex_unlock(&mutex);
} else {
    printf("Mutex lock timeout! Avoiding deadlock.\n");
}
```
- Consider using recursive mutexes for special cases
- Maintain consistent lock ordering across the program

### 5.7.5. Condition Variables

While mutexes control access to shared resources, condition variables allow threads to synchronize based on the value of data. They enable a thread to block until notified by another thread that a specific condition has occurred.

#### Purpose of Condition Variables

- Mutexes prevent simultaneous access to shared variables
- Condition variables notify threads about changes to shared variables
- Condition variables allow threads to block until receiving a notification

Condition variables are always used with mutexes and typically manage producer-consumer scenarios, waiting for specific events, or implementing advanced synchronization patterns.

#### Allocation of Condition Variables

Condition variables in POSIX threads are represented by the `pthread_cond_t` type and must be initialized before use:

##### Static Allocation
```c
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
```

##### Dynamic Allocation
```c
pthread_cond_t cond;
pthread_cond_init(&cond, NULL);  // NULL for default attributes
```

When a dynamically allocated condition variable is no longer needed, it should be destroyed:
```c
pthread_cond_destroy(&cond);
```

#### Signaling and Waiting on Condition Variables

Condition variables support three primary operations:

##### Wait for a Condition
```c
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
```
- Atomically releases the mutex and blocks the thread until the condition is signaled
- When signaled, the thread reacquires the mutex before returning
- Always call with the mutex locked

##### Signal a Condition to One Thread
```c
int pthread_cond_signal(pthread_cond_t *cond);
```
- Wakes up at least one thread waiting on the condition variable
- If no threads are waiting, the signal has no effect

##### Signal a Condition to All Threads
```c
int pthread_cond_broadcast(pthread_cond_t *cond);
```
- Wakes up all threads waiting on the condition variable
- If no threads are waiting, the broadcast has no effect

#### Example: Producer-Consumer with Condition Variables

Here's an implementation of a bounded buffer using mutexes and condition variables:

```c
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 5
#define NUM_ITEMS 20

// Shared buffer
int buffer[BUFFER_SIZE];
int count = 0;  // Number of items in the buffer
int in = 0;     // Index for the producer
int out = 0;    // Index for the consumer

// Synchronization objects
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;

void* producer(void* arg) {
    for (int i = 0; i < NUM_ITEMS; i++) {
        // Produce an item
        int item = i;
        
        // Lock the mutex
        pthread_mutex_lock(&mutex);
        
        // Wait until buffer is not full
        while (count == BUFFER_SIZE) {
            printf("Producer: buffer full, waiting...\n");
            pthread_cond_wait(&not_full, &mutex);
        }
        
        // Add item to buffer
        buffer[in] = item;
        in = (in + 1) % BUFFER_SIZE;
        count++;
        
        printf("Producer: added item %d, count = %d\n", item, count);
        
        // Signal that buffer is not empty
        pthread_cond_signal(&not_empty);
        
        // Unlock the mutex
        pthread_mutex_unlock(&mutex);
        
        // Simulate variable production time
        usleep(rand() % 100000);
    }
    return NULL;
}

void* consumer(void* arg) {
    for (int i = 0; i < NUM_ITEMS; i++) {
        // Lock the mutex
        pthread_mutex_lock(&mutex);
        
        // Wait until buffer is not empty
        while (count == 0) {
            printf("Consumer: buffer empty, waiting...\n");
            pthread_cond_wait(&not_empty, &mutex);
        }
        
        // Remove item from buffer
        int item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;
        
        printf("Consumer: removed item %d, count = %d\n", item, count);
        
        // Signal that buffer is not full
        pthread_cond_signal(&not_full);
        
        // Unlock the mutex
        pthread_mutex_unlock(&mutex);
        
        // Simulate variable consumption time
        usleep(rand() % 200000);
    }
    return NULL;
}

int main() {
    pthread_t producer_thread, consumer_thread;
    
    // Create threads
    pthread_create(&producer_thread, NULL, producer, NULL);
    pthread_create(&consumer_thread, NULL, consumer, NULL);
    
    // Join threads
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
    
    return 0;
}
```

In this example:
1. The producer waits on the `not_full` condition when the buffer is full
2. The consumer waits on the `not_empty` condition when the buffer is empty
3. Each thread signals the appropriate condition after modifying the buffer
4. The mutex ensures that buffer operations are atomic

### 5.7.6. Read-Write Locks

Read-write locks (rwlocks) are a synchronization mechanism that allows multiple threads to read shared data concurrently while ensuring exclusive access when a thread needs to modify the data.

#### Advantages of Read-Write Locks

- **Improved concurrency**: Multiple readers can access data simultaneously
- **Efficient for read-heavy workloads**: Applications with many more reads than writes benefit significantly
- **Same safety guarantees**: Still provides exclusive access when needed for writes

#### Read-Write Lock Allocation

Read-write locks in POSIX threads are represented by the `pthread_rwlock_t` type:

##### Static Allocation
```c
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
```

##### Dynamic Allocation
```c
pthread_rwlock_t rwlock;
pthread_rwlock_init(&rwlock, NULL);  // NULL for default attributes
```

When a dynamically allocated read-write lock is no longer needed, it should be destroyed:
```c
pthread_rwlock_destroy(&rwlock);
```

#### Locking and Unlocking Read-Write Locks

Read-write locks support three primary operations:

##### Acquire Read Lock
```c
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
```
- Acquires a read lock, allowing other readers but blocking writers
- If a writer holds the lock or is waiting, this call blocks
- Multiple threads can hold read locks simultaneously

##### Acquire Write Lock
```c
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
```
- Acquires a write lock, blocking both readers and other writers
- If any threads hold read locks, this call blocks
- Only one thread can hold a write lock at a time

##### Release Lock (Both Read and Write)
```c
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
```
- Releases either a read or write lock
- If writers are waiting and this was the last read lock, a waiting writer will be granted the lock

#### Example: Shared Dictionary with Read-Write Locks

Here's an example of a simple dictionary implementation using read-write locks:

```c
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENTRIES 100
#define NUM_READERS 5
#define NUM_WRITERS 2
#define NUM_OPERATIONS 1000

typedef struct {
    char key[20];
    char value[100];
    int valid;
} DictionaryEntry;

// Dictionary data
DictionaryEntry dictionary[MAX_ENTRIES];
pthread_rwlock_t dict_rwlock = PTHREAD_RWLOCK_INITIALIZER;

// Find an entry by key (returns -1 if not found)
int find_entry(const char* key) {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (dictionary[i].valid && strcmp(dictionary[i].key, key) == 0) {
            return i;
        }
    }
    return -1;
}

// Find an empty slot (returns -1 if full)
int find_empty() {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (!dictionary[i].valid) {
            return i;
        }
    }
    return -1;
}

// Reader thread function
void* reader(void* arg) {
    int id = *(int*)arg;
    char key[20];
    
    for (int i = 0; i < NUM_OPERATIONS; i++) {
        // Generate a random key to look up
        sprintf(key, "key%d", rand() % 50);
        
        // Acquire read lock
        pthread_rwlock_rdlock(&dict_rwlock);
        
        // Look up the key
        int index = find_entry(key);
        if (index >= 0) {
            printf("Reader %d: Found %s = %s\n", id, key, dictionary[index].value);
        } else {
            printf("Reader %d: Key %s not found\n", id, key);
        }
        
        // Release read lock
        pthread_rwlock_unlock(&dict_rwlock);
        
        // Sleep briefly
        usleep(rand() % 10000);
    }
    
    free(arg);
    return NULL;
}

// Writer thread function
void* writer(void* arg) {
    int id = *(int*)arg;
    char key[20];
    char value[100];
    
    for (int i = 0; i < NUM_OPERATIONS / 5; i++) {  // Writers do fewer operations
        // Generate a random key-value pair
        sprintf(key, "key%d", rand() % 50);
        sprintf(value, "value%d-%d", id, rand() % 1000);
        
        // Acquire write lock
        pthread_rwlock_wrlock(&dict_rwlock);
        
        // Check if key exists
        int index = find_entry(key);
        if (index >= 0) {
            // Update existing entry
            strcpy(dictionary[index].value, value);
            printf("Writer %d: Updated %s = %s\n", id, key, value);
        } else {
            // Add new entry
            index = find_empty();
            if (index >= 0) {
                strcpy(dictionary[index].key, key);
                strcpy(dictionary[index].value, value);
                dictionary[index].valid = 1;
                printf("Writer %d: Added %s = %s\n", id, key, value);
            } else {
                printf("Writer %d: Dictionary full, cannot add %s\n", id, key);
            }
        }
        
        // Release write lock
        pthread_rwlock_unlock(&dict_rwlock);
        
        // Sleep longer between writes
        usleep(rand() % 50000);
    }
    
    free(arg);
    return NULL;
}

int main() {
    pthread_t reader_threads[NUM_READERS];
    pthread_t writer_threads[NUM_WRITERS];
    
    // Initialize dictionary
    for (int i = 0; i < MAX_ENTRIES; i++) {
        dictionary[i].valid = 0;
    }
    
    // Create reader threads
    for (int i = 0; i < NUM_READERS; i++) {
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&reader_threads[i], NULL, reader, id);
    }
    
    // Create writer threads
    for (int i = 0; i < NUM_WRITERS; i++) {
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&writer_threads[i], NULL, writer, id);
    }
    
    // Join reader threads
    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(reader_threads[i], NULL);
    }
    
    // Join writer threads
    for (int i = 0; i < NUM_WRITERS; i++) {
        pthread_join(writer_threads[i], NULL);
    }
    
    return 0;
}
```

In this example:
1. Multiple reader threads can look up values simultaneously
2. Writer threads have exclusive access when updating the dictionary
3. The read-write lock efficiently handles concurrent access while maintaining data integrity