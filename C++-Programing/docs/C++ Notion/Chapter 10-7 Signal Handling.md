# 10.7. Signal Handling

---

## Table of Contents

1. Introduction to Signals
2. Common Signals
3. The signal() Function
4. The raise() Function
5. sigaction() Function (POSIX)
6. Signal-Safe Functions
7. Graceful Shutdown Pattern
8. Platform Considerations
9. Best Practices
10. Summary

---

## 1. Introduction to Signals

### What are Signals?

**Signals** are software interrupts sent to a process by the operating system or other processes to notify it of events like errors, user interrupts, or termination requests.

**Think of signals as:** Urgent messages that interrupt your program to demand immediate attention.

### Why Do We Need Signal Handling?

1. **Graceful Shutdown**: Clean up resources before terminating
2. **Error Handling**: Respond to crashes like segmentation faults
3. **User Interrupts**: Handle Ctrl+C properly
4. **Inter-Process Communication**: Signals can coordinate processes
5. **Timeout Mechanisms**: Handle alarm signals

### The `<csignal>` Header

```cpp
#include <csignal>  // C++ signal handling header
// or
#include <signal.h>  // C-style header (also works)
```

---

## 2. Common Signals

### Signal List

| Signal | Number | Description | Default Action |
| --- | --- | --- | --- |
| SIGINT | 2 | Interrupt (Ctrl+C) | Terminate |
| SIGTERM | 15 | Termination request | Terminate |
| SIGSEGV | 11 | Segmentation fault | Core dump |
| SIGABRT | 6 | Abort (from abort()) | Core dump |
| SIGFPE | 8 | Floating-point exception | Core dump |
| SIGILL | 4 | Illegal instruction | Core dump |
| SIGHUP | 1 | Hangup (terminal closed) | Terminate |
| SIGALRM | 14 | Alarm clock | Terminate |
| SIGKILL | 9 | Kill (cannot be caught!) | Terminate |
| SIGSTOP | 19 | Stop (cannot be caught!) | Stop |

### Signals in Action

```cpp
#include <iostream>
#include <csignal>
using namespace std;

int main() {
    // Print signal numbers
    cout << "Signal numbers:" << endl;
    cout << "SIGINT  = " << SIGINT << endl;   // Usually 2
    cout << "SIGTERM = " << SIGTERM << endl;  // Usually 15
    cout << "SIGSEGV = " << SIGSEGV << endl;  // Usually 11
    cout << "SIGABRT = " << SIGABRT << endl;  // Usually 6
    cout << "SIGFPE  = " << SIGFPE << endl;   // Usually 8

    return 0;
}
```

**Output:**

```
Signal numbers:
SIGINT  = 2
SIGTERM = 15
SIGSEGV = 11
SIGABRT = 6
SIGFPE  = 8
```

---

## 3. The signal() Function

### Syntax

```cpp
#include <csignal>

void (*signal(int sig, void (*handler)(int)))(int);

// Simplified with typedef:
typedef void (*SignalHandler)(int);
SignalHandler signal(int sig, SignalHandler handler);
```

### Parameters

- `sig`: Signal number to handle
- `handler`: Function pointer, SIG_DFL, or SIG_IGN

### Special Handler Values

| Value | Meaning |
| --- | --- |
| SIG_DFL | Default signal handling |
| SIG_IGN | Ignore the signal |
| Function | Custom handler function |

### Basic Example: Handling SIGINT

```cpp
#include <iostream>
#include <csignal>
#include <cstdlib>
using namespace std;

// WHY: Signal handler function
void signalHandler(int signum) {
    cout << "\nInterrupt signal (" << signum << ") received." << endl;

    // Cleanup and exit
    cout << "Cleaning up and exiting..." << endl;
    exit(signum);
}

int main() {
    // WHY: Register signal handler for SIGINT (Ctrl+C)
    signal(SIGINT, signalHandler);

    cout << "Program running. Press Ctrl+C to interrupt." << endl;

    // Infinite loop (will be interrupted by signal)
    while (true) {
        cout << "Working..." << endl;
        // In real code, use proper sleep function
        for (volatile int i = 0; i < 100000000; i++);
    }

    return 0;
}
```

**Output (when Ctrl+C pressed):**

```
Program running. Press Ctrl+C to interrupt.
Working...
Working...
^C
Interrupt signal (2) received.
Cleaning up and exiting...
```

### Ignoring Signals

```cpp
#include <iostream>
#include <csignal>
using namespace std;

int main() {
    // WHY: Ignore SIGINT - program won't respond to Ctrl+C
    signal(SIGINT, SIG_IGN);

    cout << "SIGINT is now ignored. Ctrl+C won't work!" << endl;
    cout << "You'll need to kill this program differently." << endl;

    // Note: Use with caution! This makes program hard to stop
    for (int i = 0; i < 5; i++) {
        cout << "Running... " << i << endl;
        // Sleep simulation
        for (volatile int j = 0; j < 100000000; j++);
    }

    return 0;
}
```

### Restoring Default Handler

```cpp
#include <iostream>
#include <csignal>
using namespace std;

void tempHandler(int sig) {
    cout << "Caught once! Restoring default." << endl;

    // WHY: Restore default behavior
    signal(SIGINT, SIG_DFL);
}

int main() {
    signal(SIGINT, tempHandler);

    cout << "First Ctrl+C: custom handler" << endl;
    cout << "Second Ctrl+C: program terminates" << endl;

    while (true) {
        cout << "Running..." << endl;
        for (volatile int i = 0; i < 100000000; i++);
    }

    return 0;
}
```

---

## 4. The raise() Function

### What is raise()?

**raise()** sends a signal to the current process programmatically.

### Syntax

```cpp
int raise(int sig);
// Returns 0 on success, non-zero on failure
```

### Example: Raising Signals

```cpp
#include <iostream>
#include <csignal>
#include <cstdlib>
using namespace std;

void handler(int sig) {
    cout << "Signal " << sig << " caught!" << endl;
}

int main() {
    // Register handlers
    signal(SIGINT, handler);
    signal(SIGTERM, handler);

    cout << "Raising SIGINT..." << endl;
    raise(SIGINT);

    cout << "Raising SIGTERM..." << endl;
    raise(SIGTERM);

    cout << "Done!" << endl;
    return 0;
}
```

**Output:**

```
Raising SIGINT...
Signal 2 caught!
Raising SIGTERM...
Signal 15 caught!
Done!
```

### Self-Termination with Signals

```cpp
#include <iostream>
#include <csignal>
#include <cstdlib>
using namespace std;

void cleanup() {
    cout << "Performing cleanup..." << endl;
    // Release resources here
}

void termHandler(int sig) {
    cout << "Termination signal received." << endl;
    cleanup();
    exit(0);
}

int main() {
    signal(SIGTERM, termHandler);

    cout << "Program started." << endl;

    // Simulate error condition
    bool error = true;
    if (error) {
        cout << "Error detected, terminating..." << endl;
        raise(SIGTERM);  // Self-terminate properly
    }

    return 0;
}
```

---

## 5. sigaction() Function (POSIX)

### Why sigaction() over signal()?

| Feature | signal() | sigaction() |
| --- | --- | --- |
| Standard | C Standard | POSIX |
| Portability | More portable | Unix-like systems |
| Reliability | Less reliable | More reliable |
| Features | Basic | Advanced (masks, flags) |
| Thread-safety | No | Yes |

### Syntax

```cpp
#include <signal.h>

int sigaction(int signum,
              const struct sigaction *act,
              struct sigaction *oldact);

struct sigaction {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t *, void *);
    sigset_t sa_mask;
    int sa_flags;
};
```

### Example: Using sigaction()

```cpp
#include <iostream>
#include <signal.h>
#include <cstring>
#include <unistd.h>
using namespace std;

void handler(int sig) {
    const char* msg = "Signal caught!\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

int main() {
    struct sigaction sa;

    // WHY: Initialize the sigaction structure
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);  // No additional signals blocked
    sa.sa_flags = 0;

    // WHY: Register handler using sigaction
    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        cerr << "Error setting signal handler" << endl;
        return 1;
    }

    cout << "Handler set. Press Ctrl+C." << endl;

    while (true) {
        cout << "Working..." << endl;
        sleep(1);
    }

    return 0;
}
```

### Blocking Additional Signals

```cpp
#include <iostream>
#include <signal.h>
#include <cstring>
#include <unistd.h>
using namespace std;

void handler(int sig) {
    const char* msg = "Handling signal, SIGTERM blocked...\n";
    write(STDOUT_FILENO, msg, strlen(msg));
    sleep(5);  // Simulate long operation
    msg = "Handler done.\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

int main() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;

    // WHY: Block SIGTERM while handling SIGINT
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGTERM);

    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);

    cout << "Press Ctrl+C..." << endl;

    while (true) {
        pause();  // Wait for signal
    }

    return 0;
}
```

---

## 6. Signal-Safe Functions

### The Problem

**Signal handlers interrupt normal execution.** Calling non-reentrant functions from a handler can cause:

- Deadlocks (if function holds a lock)
- Corruption (if function modifies global state)
- Undefined behavior

### Async-Signal-Safe Functions

Only these functions are safe to call from signal handlers:

| Category | Functions |
| --- | --- |
| I/O | write(), read() |
| Process | _exit(), fork(), getpid() |
| Signal | signal(), raise() |
| Time | time() |
| Other | Various low-level syscalls |

### NOT Safe in Signal Handlers

```cpp
// DON'T use these in signal handlers:
cout << "text";           // NOT safe (buffered I/O)
printf("text");           // NOT safe (buffered I/O)
malloc() / new           // NOT safe (memory allocation)
free() / delete          // NOT safe
mutex.lock()             // NOT safe (can deadlock)
```

### Using sig_atomic_t

```cpp
#include <iostream>
#include <csignal>
#include <atomic>
using namespace std;

// WHY: volatile sig_atomic_t is safe for signal handlers
volatile sig_atomic_t gSignalReceived = 0;

void handler(int sig) {
    // WHY: Only set a flag, don't do complex operations
    gSignalReceived = sig;
}

int main() {
    signal(SIGINT, handler);
    signal(SIGTERM, handler);

    cout << "Running. Press Ctrl+C to stop." << endl;

    while (gSignalReceived == 0) {
        cout << "Working..." << endl;
        // Simulate work
        for (volatile int i = 0; i < 100000000; i++);
    }

    // WHY: Handle signal outside the handler
    cout << "\nSignal " << gSignalReceived << " received." << endl;
    cout << "Performing cleanup..." << endl;

    return 0;
}
```

### Using std::atomic (C++11)

```cpp
#include <iostream>
#include <csignal>
#include <atomic>
using namespace std;

// WHY: std::atomic is safe if lock-free
atomic<bool> shutdownRequested(false);

static_assert(atomic<bool>::is_always_lock_free,
              "atomic<bool> must be lock-free for signal safety");

extern "C" void handler(int sig) {
    shutdownRequested.store(true, memory_order_relaxed);
}

int main() {
    signal(SIGINT, handler);

    cout << "Running..." << endl;

    while (!shutdownRequested.load(memory_order_relaxed)) {
        // Do work
        cout << "." << flush;
        for (volatile int i = 0; i < 50000000; i++);
    }

    cout << "\nShutting down gracefully." << endl;
    return 0;
}
```

---

## 7. Graceful Shutdown Pattern

### Complete Example

```cpp
#include <iostream>
#include <csignal>
#include <vector>
#include <fstream>
using namespace std;

// Global flag for shutdown
volatile sig_atomic_t running = 1;

void shutdownHandler(int sig) {
    running = 0;
}

class Application {
    vector<int> data;
    ofstream logFile;

public:
    void initialize() {
        cout << "Initializing..." << endl;
        logFile.open("app.log");
        for (int i = 0; i < 100; i++) {
            data.push_back(i);
        }
        cout << "Initialized with " << data.size() << " items." << endl;
    }

    void run() {
        cout << "Running main loop..." << endl;
        int iteration = 0;

        while (running) {
            // Do work
            logFile << "Iteration " << iteration++ << endl;
            cout << "Working... (" << iteration << ")" << endl;

            // Simulate work
            for (volatile int i = 0; i < 100000000; i++);
        }
    }

    void cleanup() {
        cout << "\nPerforming cleanup..." << endl;

        // Save data
        cout << "Saving " << data.size() << " items..." << endl;
        data.clear();

        // Close files
        if (logFile.is_open()) {
            logFile << "Shutdown complete." << endl;
            logFile.close();
            cout << "Log file closed." << endl;
        }

        cout << "Cleanup complete." << endl;
    }
};

int main() {
    // Setup signal handlers
    signal(SIGINT, shutdownHandler);
    signal(SIGTERM, shutdownHandler);

    Application app;

    app.initialize();
    app.run();
    app.cleanup();

    cout << "Application terminated gracefully." << endl;
    return 0;
}
```

---

## 8. Platform Considerations

### Windows vs POSIX

| Feature | Windows | POSIX (Linux/macOS) |
| --- | --- | --- |
| SIGINT | Partially supported | ✅ Full support |
| SIGTERM | ✅ Supported | ✅ Full support |
| SIGHUP | ❌ Not supported | ✅ Supported |
| SIGKILL | ❌ Not supported | ✅ Supported |
| sigaction() | ❌ Not available | ✅ Available |
| Ctrl+C | New thread | Signal |

### Cross-Platform Example

```cpp
#include <iostream>
#include <csignal>
using namespace std;

void handler(int sig) {
    cout << "Signal " << sig << " caught!" << endl;
}

int main() {
    // These work on most platforms
    signal(SIGINT, handler);
    signal(SIGTERM, handler);
    signal(SIGABRT, handler);
    signal(SIGSEGV, handler);
    signal(SIGFPE, handler);

    #ifdef _WIN32
        cout << "Running on Windows" << endl;
    #else
        // POSIX-specific signals
        signal(SIGHUP, handler);
        cout << "Running on POSIX system" << endl;
    #endif

    cout << "Signal handlers registered." << endl;

    // Demonstrate
    raise(SIGINT);

    return 0;
}
```

### Windows Console Handler

```cpp
#ifdef _WIN32
#include <windows.h>

BOOL WINAPI ConsoleHandler(DWORD signal) {
    switch (signal) {
        case CTRL_C_EVENT:
            cout << "Ctrl+C caught!" << endl;
            return TRUE;
        case CTRL_BREAK_EVENT:
            cout << "Ctrl+Break caught!" << endl;
            return TRUE;
        case CTRL_CLOSE_EVENT:
            cout << "Close event caught!" << endl;
            return TRUE;
    }
    return FALSE;
}

int main() {
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    while (true) {
        cout << "Running..." << endl;
        Sleep(1000);
    }

    return 0;
}
#endif
```

---

## 9. Best Practices

### ✅ DO: Keep Handlers Simple

```cpp
volatile sig_atomic_t flag = 0;

void handler(int sig) {
    flag = 1;  // Just set a flag
}

int main() {
    signal(SIGINT, handler);

    while (!flag) {
        // Main loop
    }

    // Handle cleanup in main, not in handler
    cleanup();
    return 0;
}
```

### ✅ DO: Use sig_atomic_t or lock-free atomics

```cpp
volatile sig_atomic_t gSignal = 0;

// Or C++11:
std::atomic<bool> gShutdown{false};
static_assert(std::atomic<bool>::is_always_lock_free);
```

### ✅ DO: Handle Multiple Signals

```cpp
signal(SIGINT, handler);
signal(SIGTERM, handler);
signal(SIGHUP, handler);
```

### ❌ DON'T: Do Complex Operations in Handler

```cpp
void badHandler(int sig) {
    cout << "Signal!";     // BAD: buffered I/O
    delete resource;       // BAD: memory ops
    mutex.lock();          // BAD: can deadlock
    log.write("...");      // BAD: file I/O
}
```

### ❌ DON'T: Ignore SIGTERM in Production

```cpp
// BAD: Process can't be stopped gracefully
signal(SIGTERM, SIG_IGN);  // Don't do this!
```

### ❌ DON'T: Use signal() for Critical Code

```cpp
// BETTER: Use sigaction() on POSIX systems
struct sigaction sa;
sa.sa_handler = handler;
sigemptyset(&sa.sa_mask);
sa.sa_flags = SA_RESTART;  // Restart interrupted syscalls
sigaction(SIGINT, &sa, nullptr);
```

---

## 10. Summary

### Key Takeaways

1. **Signals** are OS interrupts for process notification
2. **signal()** registers handlers (C standard)
3. **sigaction()** is more reliable (POSIX)
4. **raise()** sends signals programmatically
5. **Keep handlers simple** - just set flags
6. **Use sig_atomic_t** for signal-safe variables

### Common Signals

| Signal | Cause | Typical Use |
| --- | --- | --- |
| SIGINT | Ctrl+C | Graceful shutdown |
| SIGTERM | kill command | Termination request |
| SIGSEGV | Bad pointer | Crash debugging |
| SIGABRT | abort() | Assertion failure |

### Signal Handling Pattern

```cpp
volatile sig_atomic_t running = 1;

void handler(int) { running = 0; }

int main() {
    signal(SIGINT, handler);
    signal(SIGTERM, handler);

    while (running) {
        doWork();
    }

    cleanup();
    return 0;
}
```

### Keywords Covered

✅ Signals (5)
✅ Signal handling (4)
✅ signal() (5)
✅ sigaction() (3)
✅ SIGINT (4)
✅ SIGTERM (3)
✅ SIGSEGV (2)
✅ SIGABRT (2)
✅ Signal handler (5)
✅ Async-signal-safe (2)
✅ sig_atomic_t (3)
✅ raise() (3)
✅ SIG_DFL (2)
✅ SIG_IGN (2)
✅ Graceful shutdown (3)
✅ POSIX signals (2)

**Total: 50 keywords/concepts covered**

---