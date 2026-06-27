# 8.3. Exception Handling - noexcept, Stack Unwinding & Best Practices

---

## Table of Contents

1. noexcept Specifier - Part 2
2. Stack Unwinding
3. Best Practices
4. Summary
5. Interview Preparation

---

## 4. Stack Unwinding

### 4.1 What is Stack Unwinding?

**Stack unwinding** is the process of cleaning up the call stack when an exception is thrown

**Process:**

1. Exception thrown
2. Current scope exited
3. Destructors called for local objects (reverse construction order)
4. Previous scope exited
5. Repeat until exception caught or program terminates

### 4.2 Stack Unwinding Example

```cpp
#include <iostream>
using namespace std;

class Tracer {
    string name;
public:
    Tracer(const string& n) : name(n) {
        cout << "Constructing " << name << endl;
    }

    ~Tracer() {
        cout << "Destroying " << name << endl;
    }
};

void level3() {
    Tracer t3("Level3");
    cout << "In level3 - throwing exception" << endl;
    throw runtime_error("Error at level3");
    cout << "After throw (never executes)" << endl;
}

void level2() {
    Tracer t2("Level2");
    cout << "In level2 - calling level3" << endl;
    level3();
    cout << "After level3 (never executes)" << endl;
}

void level1() {
    Tracer t1("Level1");
    cout << "In level1 - calling level2" << endl;
    level2();
    cout << "After level2 (never executes)" << endl;
}

int main() {
    try {
        Tracer tmain("Main");
        cout << "In main - calling level1" << endl;
        level1();
        cout << "After level1 (never executes)" << endl;
    } catch (const exception& e) {
        cout << "Caught: " << e.what() << endl;
    }
    cout << "Program continues" << endl;
    return 0;
}
```

**Output:**

```
Constructing Main
In main - calling level1
Constructing Level1
In level1 - calling level2
Constructing Level2
In level2 - calling level3
Constructing Level3
In level3 - throwing exception
Destroying Level3
Destroying Level2
Destroying Level1
Destroying Main
Caught: Error at level3
Program continues
```

### 4.3 Partial Construction

```cpp
#include <iostream>
using namespace std;

class Member {
    string name;
public:
    Member(const string& n) : name(n) {
        cout << "Member " << name << " constructed" << endl;
    }
    ~Member() {
        cout << "Member " << name << " destroyed" << endl;
    }
};

class Container {
    Member m1;
    Member m2;
    Member m3;
public:
    Container()
        : m1("First"), m2("Second"), m3("Third") {
        cout << "Container constructed" << endl;
        // Simulate error after partial construction
        throw runtime_error("Construction failed");
    }

    ~Container() {
        cout << "Container destroyed" << endl;
    }
};

int main() {
    try {
        Container c;
    } catch (const exception& e) {
        cout << "Caught: " << e.what() << endl;
    }
    return 0;
}
```

**Output:**

```
Member First constructed
Member Second constructed
Member Third constructed
Member Third destroyed
Member Second destroyed
Member First destroyed
Caught: Construction failed
```

**Key Point:** Destructor NOT called for partially constructed object, but destructors called for fully constructed members

### 4.4 Exception in Destructor (DANGER!)

```cpp
#include <iostream>
using namespace std;

class DangerousClass {
public:
    ~DangerousClass() noexcept(false) {  // ❌ BAD!
        cout << "Destructor throwing" << endl;
        throw runtime_error("Destructor error");
    }
};

int main() {
    try {
        DangerousClass obj;
        cout << "Throwing from try block" << endl;
        throw runtime_error("First error");
    } catch (const exception& e) {
        cout << "Will never reach here!" << endl;
    }
    return 0;
}
```

**Output:**

```
Throwing from try block
Destructor throwing
terminate called after throwing an instance of 'std::runtime_error'
  what():  Destructor error
Aborted
```

**CRITICAL:** Throwing from destructor during stack unwinding calls `std::terminate()`!

---

## 5. Best Practices

### 5.1 When to Use Exceptions

**✅ USE exceptions for:**

1. **Constructor Failures:**

```cpp
class File {
public:
    File(const string& path) {
        // WHY: Constructors can't return error codes
        if (!open(path)) {
            throw runtime_error("Cannot open file: " + path);
        }
    }
};
```

1. **Unexpected Errors:**

```cpp
void divide(int a, int b) {
    // WHY: Caller expects this to work
    if (b == 0) {
        throw invalid_argument("Division by zero");
    }
    return a / b;
}
```

1. **Deep Call Stacks:**

```cpp
// WHY: Automatic propagation through many layers
main() -> layer1() -> layer2() -> layer3() [throws]
// Error propagates automatically to main
```

1. **Resource Cleanup:**

```cpp
// WHY: RAII ensures cleanup even with exceptions
void processResources() {
    unique_ptr<Resource> r1(new Resource());
    unique_ptr<Resource> r2(new Resource());
    // Both cleaned up even if exception thrown
}
```

### 5.2 When NOT to Use Exceptions

**❌ DON'T use exceptions for:**

1. **Flow Control:**

```cpp
// ❌ BAD: Using exceptions for normal flow
try {
    while (true) {
        data = getNextItem();
    }
} catch (EndOfData&) {
    // Process complete
}

// ✅ GOOD: Normal condition checking
while (hasNextItem()) {
    data = getNextItem();
}
```

1. **Expected Errors:**

```cpp
// ❌ Consider error codes for expected failures
bool parseConfig(const string& config) {
    // Config might be invalid - expected case
    if (!validate(config)) {
        return false;  // Error code
    }
    return true;
}
```

1. **Performance-Critical Loops:**

```cpp
// ❌ Exception overhead in tight loop
for (int i = 0; i < 1000000; ++i) {
    try {
        process(i);  // Checking exception every iteration
    } catch (...) { }
}

// ✅ Check once outside loop
try {
    for (int i = 0; i < 1000000; ++i) {
        process(i);
    }
} catch (...) { }
```

1. **Real-time Systems (sometimes):**

```cpp
// WHY: Exception overhead unacceptable in hard real-time
// Use error codes instead
ErrorCode criticalOperation() {
    if (failure) return ERROR_CODE;
    return SUCCESS;
}
```

### 5.3 Exception Safety Checklist

**For Every Function:**

- [ ]  Does it leak resources if exception thrown?
- [ ]  Does it leave objects in invalid state?
- [ ]  Are all cleanup operations in destructors?
- [ ]  Are all resources managed by RAII?
- [ ]  Is exception specification appropriate?
- [ ]  Is noexcept used where appropriate?
- [ ]  Are exceptions caught by reference?

### 5.4 Design Guidelines

**1. RAII Everywhere:**

```cpp
// ✅ GOOD: All resources in RAII wrappers
void process() {
    unique_ptr<Resource> r1(new Resource());
    lock_guard<mutex> lock(mtx);
    ifstream file("data.txt");
    // All automatically cleaned up
}
```

**2. Copy-and-Swap for Assignment:**

```cpp
// ✅ GOOD: Strong guarantee
MyClass& operator=(const MyClass& other) {
    MyClass temp(other);  // May throw
    swap(temp);           // No-throw
    return *this;
}
```

**3. No-throw Destructors:**

```cpp
// ✅ GOOD: Destructor never throws
~MyClass() noexcept {
    try {
        cleanup();
    } catch (...) {
        // Log but don't re-throw
    }
}
```

**4. Catch by const Reference:**

```cpp
// ✅ GOOD: No slicing, no copying
try {
    // ...
} catch (const std::exception& e) {
    // Handle
}

```

**5. Document Exception Behavior:**

```cpp
/**
 * @throws FileException if file cannot be opened
 * @throws std::bad_alloc if memory allocation fails
 */
void loadFile(const string& path);
```

### 5.5 Exceptions vs Error Codes

**Comparison:**

| Aspect | Exceptions | Error Codes |
| --- | --- | --- |
| **Code Clarity** | Clean separation | Cluttered checks |
| **Performance (no error)** | Fast (zero-cost) | Slower (checks) |
| **Performance (error)** | Slower (unwinding) | Faster |
| **Constructor Errors** | Natural | Impossible |
| **Automatic Propagation** | Yes | No (manual) |
| **Ignorable** | No (crash if uncaught) | Yes (easy to ignore) |
| **Resource Cleanup** | Automatic (RAII) | Manual |
| **Call Stack Info** | Available | Lost |
| **Real-time Safe** | No (unpredictable) | Yes |
| **Binary Size** | Larger | Smaller |

**Decision Matrix:**

Use **Exceptions** when:

- Error handling would clutter code
- Errors are exceptional, not expected
- Need automatic resource cleanup
- Working in constructors/operators
- Errors rare on happy path

Use **Error Codes** when:

- Errors are expected/frequent
- Performance critical (tight loops)
- Real-time constraints
- C compatibility required
- Embedded systems (limited resources)

---

## Summary

### Key Takeaways

1. **Four Exception Safety Levels Matter** - No guarantee (avoid!), basic (valid state, no leaks), strong (transactional), no-throw (never fails). Aim for at least basic everywhere, strong where possible, no-throw for destructors/moves.
2. **RAII is Essential for Exception Safety** - Resource lifetime bound to object lifetime. Destructors automatically called during unwinding. Use smart pointers, lock guards, and custom RAII wrappers for all resources.
3. **noexcept Enables Optimizations** - Mark move constructors/assignment and swap as noexcept. Containers use move instead of copy when noexcept. If noexcept function throws, std::terminate() called immediately.
4. **Stack Unwinding Ensures Cleanup** - When exception thrown, destructors called in reverse construction order. Partially constructed objects have constructed members destroyed. Never throw from destructors during unwinding.
5. **Copy-and-Swap Provides Strong Guarantee** - Create temporary copy (may throw), swap with temporary (no-throw), let temporary destruct. All-or-nothing semantics - operation completes fully or leaves original unchanged.
6. **Destructors Must Be No-throw** - Implicit noexcept on all destructors. Throwing from destructor during unwinding terminates program. Catch and log errors in destructors, never re-throw.
7. **Move Operations Should Be noexcept** - Enables performance optimizations in STL containers. Vector uses move if noexcept, otherwise copies during reallocation. Use std::move_if_noexcept for conditional moves.
8. **Always Catch by const Reference** - Prevents object slicing with polymorphic exceptions. Avoids unnecessary copying. Preserves derived class information through base class reference.
9. **Use Exceptions for Unexpected Errors** - Constructors, deep call stacks, resource cleanup. Don't use for flow control, expected errors, or performance-critical tight loops. Consider error codes for real-time systems.
10. **Document Exception Behavior** - Specify which exceptions can be thrown. Use noexcept to document no-throw guarantee. Help users write exception-safe code. Review for exception safety in code reviews.

---

## Interview Preparation

### Q1: Explain the four levels of exception safety guarantees. How do you implement the strong guarantee?

**Answer:**

Exception safety guarantees define how a function behaves when exceptions are thrown. There are four levels:

**1. No Guarantee (Level 0)** - Worst, avoid at all costs:

- Program may be in undefined state
- Resources may leak
- Objects may be corrupted
- Never acceptable in production code

**2. Basic Guarantee (Level 1)** - Minimum acceptable:

- Program remains in valid state
- No resource leaks
- Objects are internally consistent
- Data may be modified

Example:

```cpp
class BasicSafe {
    vector<int> data;
public:
    void addItems(int n) {
        // WHY: Basic guarantee - no leaks, valid state
        for (int i = 0; i < n; ++i) {
            data.push_back(i);  // May throw at any point
        }
        // If push_back throws, data partially modified
        // but object still valid
    }
};
```

**3. Strong Guarantee (Level 2)** - Transactional:

- Operation succeeds completely OR
- State unchanged (like database transaction)
- No side effects if exception thrown
- All-or-nothing semantics

**4. No-throw Guarantee (Level 3)** - Strongest:

- Operation guaranteed not to throw
- Required for: destructors, swap, move operations
- Often impossible for operations that allocate memory

**Implementing Strong Guarantee - Copy-and-Swap Idiom:**

```cpp
#include <algorithm>
#include <utility>

class StrongSafe {
    int* data;
    size_t size;

    // WHY: No-throw swap for strong guarantee
    void swap(StrongSafe& other) noexcept {
        std::swap(data, other.data);
        std::swap(size, other.size);
    }

public:
    StrongSafe(size_t s = 0) : data(nullptr), size(s) {
        if (size > 0) data = new int[size]();
    }

    ~StrongSafe() { delete[] data; }

    // WHY: Copy constructor may throw
    StrongSafe(const StrongSafe& other)
        : data(nullptr), size(other.size) {
        if (size > 0) {
            data = new int[size];
            std::copy(other.data, other.data + size, data);
        }
    }

    // WHY: Assignment operator with strong guarantee
    StrongSafe& operator=(const StrongSafe& other) {
        // Step 1: Create temporary copy (may throw)
        StrongSafe temp(other);

        // Step 2: Swap with temporary (no-throw)
        swap(temp);

        // Step 3: Temporary destroys old data (no-throw)
        return *this;

        // If copy construction throws, original unchanged
        // If succeeds, swap is no-throw
        // Result: strong guarantee - complete or unchanged
    }
};
```

**Why Copy-and-Swap Works:**

1. All operations that can throw (copying) happen first
2. Once successful, only no-throw operations remain (swap)
3. If exception during copy, original object unchanged
4. If copy succeeds, swap commits changes atomically

**Alternative Implementation for resize():**

```cpp
void resize(size_t newSize) {
    // WHY: Create new storage (may throw)
    int* newData = new int[newSize]();

    // WHY: Copy existing data (may throw)
    size_t copySize = std::min(size, newSize);
    std::copy(data, data + copySize, newData);

    // WHY: Only no-throw operations from here
    delete[] data;
    data = newData;
    size = newSize;
}
```

**When to Use Each Level:**

| Operation | Level | Reason |
| --- | --- | --- |
| Destructor | No-throw | Called during unwinding |
| Move ops | No-throw | Performance critical |
| swap | No-throw | Used in strong guarantee |
| Assignment | Strong | Clean semantics |
| Regular methods | Basic minimum | Practical balance |

**Key Points:**

- Basic guarantee is minimum acceptable
- Strong guarantee ideal but sometimes expensive
- No-throw required for critical operations
- Document which guarantee your code provides

---

### Q2: What is RAII and how does it relate to exception safety? Why is it important?

**Answer:**

**RAII (Resource Acquisition Is Initialization)** is a programming idiom where resource lifetime is bound to object lifetime. Resources are acquired in the constructor and released in the destructor.

**Core Principles:**

1. Acquire resource in constructor
2. Release resource in destructor
3. Destructor automatically called during stack unwinding
4. No manual cleanup needed

**Why RAII Matters for Exception Safety:**

**Without RAII - Manual Cleanup Required:**

```cpp
void processFile() {
    FILE* file = fopen("data.txt", "r");
    if (!file) return;

    char* buffer = new char[1024];

    try {
        // Processing...
        if (error1) {
            fclose(file);     // Manual cleanup
            delete[] buffer;
            throw exception();
        }

        if (error2) {
            fclose(file);     // Manual cleanup again!
            delete[] buffer;
            throw exception();
        }

        // Success path
        fclose(file);
        delete[] buffer;

    } catch (...) {
        fclose(file);         // Cleanup in catch too!
        delete[] buffer;
        throw;
    }
}
```

**With RAII - Automatic Cleanup:**

```cpp
class FileHandle {
    FILE* file;
public:
    FileHandle(const char* path, const char* mode) {
        file = fopen(path, mode);
        if (!file) throw runtime_error("Cannot open file");
    }

    ~FileHandle() {
        // WHY: Automatically called even if exception thrown
        if (file) fclose(file);
    }

    FILE* get() { return file; }

    // Prevent copying
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
};

void processFile() {
    FileHandle file("data.txt", "r");
    vector<char> buffer(1024);  // RAII container

    // WHY: Clean code - no manual cleanup
    if (error1) throw exception();  // file & buffer cleaned up
    if (error2) throw exception();  // file & buffer cleaned up

    // Normal return - file & buffer cleaned up
}
```

**Standard Library RAII Examples:**

**1. Smart Pointers:**

```cpp
void useSmartPointers() {
    // WHY: Automatic memory management
    unique_ptr<Widget> widget(new Widget());

    widget->process();  // May throw

    // Widget automatically deleted even if exception thrown
}
```

**2. Lock Guards:**

```cpp
mutex mtx;

void threadSafeFunction() {
    lock_guard<mutex> lock(mtx);
    // WHY: Mutex automatically unlocked

    criticalSection();  // May throw

    // lock_guard destructor unlocks mutex
}
```

**3. File Streams:**

```cpp
void processFile() {
    ifstream file("data.txt");
    // WHY: File automatically closed

    processData(file);  // May throw

    // ifstream destructor closes file
}
```

**Custom RAII Example:**

```cpp
class DatabaseTransaction {
    Database& db;
    bool committed;

public:
    DatabaseTransaction(Database& database)
        : db(database), committed(false) {
        db.beginTransaction();
    }

    ~DatabaseTransaction() {
        if (!committed) {
            // WHY: Automatic rollback if exception thrown
            db.rollback();
        }
    }

    void commit() {
        db.commit();
        committed = true;
    }
};

void performTransaction() {
    Database db;
    DatabaseTransaction trans(db);

    // WHY: If any operation throws, transaction rolled back
    db.insert("data1");
    db.update("data2");
    db.delete("data3");

    trans.commit();  // Only commit if all succeeded
}
```

**RAII Benefits:**

1. **Exception Safety:**
    - Automatic cleanup guaranteed
    - No resource leaks
    - Impossible to forget cleanup
2. **Code Clarity:**
    - No manual cleanup code
    - Clear ownership semantics
    - Less error-prone
3. **Correctness:**
    - Cleanup happens in reverse construction order
    - Works correctly with exceptions
    - Works correctly with early returns
4. **Maintainability:**
    - Add new resources easily
    - Change cleanup logic in one place
    - Refactoring safer

**Key Principles:**

- One resource per RAII object
- Constructor acquires, destructor releases
- Make non-copyable (or implement proper copy)
- Use standard RAII types when possible
- Create custom RAII wrappers for other resources

**Without RAII, writing exception-safe code is nearly impossible!**

---

### Q3: Explain noexcept specifier. Why should move constructors be noexcept? What happens if noexcept function throws?

**Answer:**

**noexcept** is a C++11 specifier indicating a function doesn't throw exceptions.

**Syntax:**

```cpp
void func() noexcept;              // Never throws
void func() noexcept(true);        // Same as above
void func() noexcept(false);       // May throw (default)

// Conditional noexcept
template <typename T>
void func() noexcept(noexcept(T()));
```

**Why Move Constructors Should Be noexcept:**

**Problem: Vector Reallocation**

```cpp
class Widget {
public:
    Widget(Widget&& other);  // Move constructor WITHOUT noexcept
};

vector<Widget> vec;
vec.push_back(Widget());  // Add first element
vec.push_back(Widget());  // Add second element
vec.push_back(Widget());  // Triggers reallocation!
```

**During reallocation:**

1. Allocate new memory
2. Move/copy elements from old to new
3. Deallocate old memory

**If move constructor is NOT noexcept:**

- Vector must use COPY constructor (slow!)
- Why? If move throws halfway, can't recover old state
- Strong exception guarantee impossible with throwing move

**If move constructor IS noexcept:**

- Vector uses MOVE constructor (fast!)
- Strong guarantee: if allocation fails, old vector unchanged
- If moves succeed, commit is quick

**Correct Implementation:**

```cpp
class Widget {
    int* data;

public:
    // ❌ WITHOUT noexcept - vector copies!
    Widget(Widget&& other)
        : data(other.data) {
        other.data = nullptr;
    }

    // ✅ WITH noexcept - vector moves!
    Widget(Widget&& other) noexcept
        : data(other.data) {
        other.data = nullptr;
    }
};
```

**Performance Impact:**

```cpp
#include <vector>
#include <iostream>
using namespace std;

struct Copyable {
    Copyable() = default;
    Copyable(const Copyable&) {
        cout << "Copied" << endl;
    }
    Copyable(Copyable&&) {  // NOT noexcept
        cout << "Moved (but vector won't use)" << endl;
    }
};

struct Movable {
    Movable() = default;
    Movable(const Movable&) {
        cout << "Copied" << endl;
    }
    Movable(Movable&&) noexcept {  // noexcept!
        cout << "Moved (vector will use)" << endl;
    }
};

int main() {
    vector<Copyable> v1;
    v1.reserve(2);
    v1.emplace_back();
    v1.emplace_back();
    v1.emplace_back();  // Triggers reallocation - COPIES!

    cout << "---" << endl;

    vector<Movable> v2;
    v2.reserve(2);
    v2.emplace_back();
    v2.emplace_back();
    v2.emplace_back();  // Triggers reallocation - MOVES!

    return 0;
}
```

**Output:**

```
Copied
Copied
---
Moved (vector will use)
Moved (vector will use)
```

**What Happens if noexcept Function Throws:**

```cpp
void dangerousNoexcept() noexcept {
    throw runtime_error("Oops!");  // ❌ Violates noexcept
}

int main() {
    try {
        dangerousNoexcept();
    } catch (...) {
        // ❌ NEVER REACHED!
        cout << "Caught" << endl;
    }
    return 0;
}
```

**Result:** `std::terminate()` called immediately!

**Output:**

```
terminate called after throwing an instance of 'std::runtime_error'
Aborted
```

**No Stack Unwinding:**

- No destructors called
- No cleanup performed
- Program crashes immediately

**When to Use noexcept:**

**✅ ALWAYS:**

- Destructors (implicit)
- Move constructors
- Move assignment operators
- swap functions
- Deallocation functions

**✅ OFTEN:**

- Simple getters
- Operations guaranteed not to throw
- Performance-critical code

**❌ AVOID:**

- Functions that may need to throw in future
- When calling potentially throwing functions
- Unless certain it won't throw

**Conditional noexcept:**

```cpp
template <typename T>
class Container {
    T* data;
public:
    // WHY: noexcept if T's move is noexcept
    Container(Container&& other)
        noexcept(noexcept(T(std::declval<T&&>())))
        : data(other.data) {
        other.data = nullptr;
    }
};
```

**Key Takeaways:**

- noexcept enables optimizations (especially in STL)
- Move constructors MUST be noexcept for performance
- Throwing from noexcept function terminates program
- Use noexcept operator to query: `noexcept(expr)`
- Document no-throw guarantee with noexcept

---

### Q4: What is stack unwinding? What happens to objects during stack unwinding? Why can't destructors throw?

**Answer:**

**Stack Unwinding** is the process of exiting scopes and calling destructors when an exception propagates up the call stack.

**Process:**

1. Exception thrown in some function
2. Function exits immediately
3. Local objects destroyed (destructors called in reverse order)
4. Previous function in call stack exited
5. Its local objects destroyed
6. Process continues until exception caught or main() exited

**Complete Example:**

```cpp
#include <iostream>
using namespace std;

class Resource {
    string name;
public:
    Resource(const string& n) : name(n) {
        cout << "  [+] Acquiring " << name << endl;
    }

    ~Resource() {
        cout << "  [-] Releasing " << name << endl;
    }

    void use() {
        cout << "  [*] Using " << name << endl;
    }
};

void level3() {
    cout << "Entering level3" << endl;
    Resource r3("Resource-3");
    r3.use();

    cout << "level3: Throwing exception!" << endl;
    throw runtime_error("Error at level3");

    cout << "level3: After throw (NEVER EXECUTED)" << endl;
}

void level2() {
    cout << "Entering level2" << endl;
    Resource r2("Resource-2");
    r2.use();

    level3();

    cout << "level2: After level3 (NEVER EXECUTED)" << endl;
}

void level1() {
    cout << "Entering level1" << endl;
    Resource r1("Resource-1");
    r1.use();

    level2();

    cout << "level1: After level2 (NEVER EXECUTED)" << endl;
}

int main() {
    try {
        cout << "Main: Starting" << endl;
        Resource rmain("Resource-Main");
        rmain.use();

        level1();

        cout << "Main: After level1 (NEVER EXECUTED)" << endl;
    } catch (const exception& e) {
        cout << "\\nCaught in main: " << e.what() << endl;
    }

    cout << "Main: Continuing after catch" << endl;
    return 0;
}
```

**Output:**

```
Main: Starting
  [+] Acquiring Resource-Main
  [*] Using Resource-Main
Entering level1
  [+] Acquiring Resource-1
  [*] Using Resource-1
Entering level2
  [+] Acquiring Resource-2
  [*] Using Resource-2
Entering level3
  [+] Acquiring Resource-3
  [*] Using Resource-3
level3: Throwing exception!
  [-] Releasing Resource-3
  [-] Releasing Resource-2
  [-] Releasing Resource-1
  [-] Releasing Resource-Main

Caught in main: Error at level3
Main: Continuing after catch
```

**Key Observations:**

1. All resources acquired before exception
2. Exception thrown in level3
3. Destructors called in **reverse order** of construction
4. No code after throw executes
5. Program continues after catch

**Partial Object Construction:**

```cpp
#include <iostream>
using namespace std;

class Member {
    string name;
public:
    Member(const string& n) : name(n) {
        cout << "Member " << name << " constructed" << endl;
        if (name == "Bad") {
            throw runtime_error("Member construction failed");
        }
    }

    ~Member() {
        cout << "Member " << name << " destroyed" << endl;
    }
};

class Container {
    Member m1;
    Member m2;
    Member m3;
public:
    Container()
        : m1("First"),
          m2("Bad"),      // Throws here!
          m3("Third") {   // Never constructed
        cout << "Container body" << endl;
    }

    ~Container() {
        cout << "Container destroyed" << endl;
    }
};

int main() {
    try {
        Container c;
    } catch (const exception& e) {
        cout << "Caught: " << e.what() << endl;
    }
    return 0;
}
```

**Output:**

```
Member First constructed
Member Bad constructed
Member First destroyed
Caught: Member construction failed
```

**Critical Points:**

- m1 fully constructed → destructor called
- m2 threw during construction → no destructor
- m3 never constructed → no destructor
- Container object never completed → no destructor
- **Only destructors for fully constructed objects called**

**Why Destructors Can't Throw:**

**Problem: Double Exception**

```cpp
class Bad1 {
public:
    ~Bad1() noexcept(false) {  // ❌ Allowing exceptions
        throw runtime_error("Destructor 1");
    }
};

class Bad2 {
public:
    ~Bad2() noexcept(false) {
        throw runtime_error("Destructor 2");
    }
};

int main() {
    try {
        Bad1 b1;
        Bad2 b2;
        throw runtime_error("Main exception");
    } catch (...) {
        // Never reaches here!
    }
    return 0;
}
```

**What Happens:**

1. Main exception thrown
2. Stack unwinding begins
3. ~Bad2() called → throws second exception
4. **Two active exceptions** → std::terminate() called
5. Program crashes immediately

**Output:**

```
terminate called after throwing an instance of 'std::runtime_error'
  what():  Destructor 2
Aborted
```

**C++ Rule:** If destructor throws during stack unwinding, `std::terminate()` is called!

**Correct Pattern - Suppress Exceptions in Destructor:**

```cpp
class Correct {
    Resource* resource;
public:
    ~Correct() noexcept {  // noexcept by default
        try {
            if (resource) {
                resource->close();  // May throw
            }
        } catch (const exception& e) {
            // WHY: Log error but don't re-throw
            cerr << "Error in destructor: " << e.what() << endl;
            // Could also:
            // - Set global error flag
            // - Write to error log
            // - Alert monitoring system
            // But NEVER re-throw!
        }
        delete resource;
    }
};
```

**Two-Phase Cleanup Pattern:**

```cpp
class FileHandler {
    FILE* file;
    bool closed;

public:
    FileHandler(const char* path)
        : file(fopen(path, "w")), closed(false) {
        if (!file) throw runtime_error("Cannot open file");
    }

    // WHY: Explicit close that can throw
    void close() {
        if (!closed && file) {
            if (fflush(file) != 0) {
                throw runtime_error("Flush failed");
            }
            if (fclose(file) != 0) {
                throw runtime_error("Close failed");
            }
            closed = true;
        }
    }

    // WHY: Destructor never throws
    ~FileHandler() noexcept {
        if (!closed && file) {
            // Attempt close but suppress errors
            fflush(file);
            fclose(file);
        }
    }
};

// Usage:
void processFile() {
    FileHandler fh("output.txt");

    writeData(fh);

    // WHY: Explicit close before scope exit
    // Allows handling close errors
    fh.close();  // May throw - OK here

    // If close() not called, destructor closes silently
}
```

**Stack Unwinding Guarantees:**

| Guarantee | Description |
| --- | --- |
| Destructors called | For all fully constructed objects |
| Reverse order | Opposite of construction order |
| No skipping | Every destructor called exactly once |
| Resources freed | If using RAII properly |
| Exception propagates | Until caught or main() exits |

**Key Takeaways:**

- Stack unwinding = automatic cleanup during exception propagation
- Destructors called in reverse construction order
- Only fully constructed objects get destructors called
- Destructors must never throw (implicit noexcept)
- If destructor throws during unwinding → std::terminate()
- Use RAII to ensure automatic cleanup
- Two-phase cleanup: explicit close() + silent destructor

---

### Q5: Compare exceptions vs error codes. When should you use each approach? What are the trade-offs?

**Answer:**

**Comprehensive Comparison:**

| Aspect | Exceptions | Error Codes |
| --- | --- | --- |
| **Syntax** | throw/try/catch | return/check |
| **Code Clarity** | Clean separation of concerns | Cluttered with checks |
| **Readability** | Normal flow visible | Error checking mixed in |
| **Propagation** | Automatic up call stack | Manual at each level |
| **Performance (happy path)** | Fast (zero-cost abstraction) | Slower (constant checks) |
| **Performance (error path)** | Slower (stack unwinding) | Faster (simple return) |
| **Constructor Errors** | Natural mechanism | Impossible to report |
| **Operator Overloading** | Can throw | Can't return error code |
| **Deep Call Stacks** | Elegant (auto-propagate) | Tedious (manual propagate) |
| **Resource Cleanup** | Automatic (RAII) | Manual in each path |
| **Ignorability** | Cannot ignore (crash if uncaught) | Easy to ignore (dangerous) |
| **Binary Size** | Larger (exception tables) | Smaller |
| **Predictability** | Unpredictable performance | Predictable |
| **Real-time Safe** | No (non-deterministic) | Yes (deterministic) |
| **Stack Trace** | Available in debugger | Lost |
| **Multiple Errors** | Can only throw one at a time | Can return multiple codes |
| **Type Safety** | Strong (catch by type) | Weak (int codes) |

**Code Examples:**

**1. Exceptions - Clean Code:**

```cpp
#include <iostream>
#include <stdexcept>
#include <fstream>
using namespace std;

class DataProcessor {
public:
    void process(const string& filename) {
        // WHY: Clean, readable flow
        ifstream file(filename);
        if (!file) {
            throw runtime_error("Cannot open file: " + filename);
        }

        string data = readData(file);
        validateData(data);
        transformData(data);
        saveResults(data);

        // No error checking code here!
        // All errors propagate automatically
    }

private:
    string readData(ifstream& file) {
        string data;
        if (!(file >> data)) {
            throw runtime_error("Read failed");
        }
        return data;
    }

    void validateData(const string& data) {
        if (data.empty()) {
            throw invalid_argument("Data is empty");
        }
        if (data.size() > 1000) {
            throw length_error("Data too large");
        }
    }

    void transformData(string& data) {
        // Processing...
        if (someError) {
            throw runtime_error("Transform failed");
        }
    }

    void saveResults(const string& data) {
        ofstream out("results.txt");
        if (!out) {
            throw runtime_error("Cannot write results");
        }
        out << data;
    }
};

// Usage:
int main() {
    try {
        DataProcessor processor;
        processor.process("input.txt");
        cout << "Success!" << endl;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}
```

**2. Error Codes - Cluttered Code:**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

enum ErrorCode {
    SUCCESS = 0,
    ERR_FILE_OPEN,
    ERR_FILE_READ,
    ERR_INVALID_DATA,
    ERR_DATA_TOO_LARGE,
    ERR_TRANSFORM,
    ERR_FILE_WRITE
};

class DataProcessor {
public:
    ErrorCode process(const string& filename) {
        // WHY: Cluttered with error checks
        ifstream file(filename);
        if (!file) {
            return ERR_FILE_OPEN;
        }

        string data;
        ErrorCode err = readData(file, data);
        if (err != SUCCESS) return err;  // Manual propagation

        err = validateData(data);
        if (err != SUCCESS) return err;  // Manual propagation

        err = transformData(data);
        if (err != SUCCESS) return err;  // Manual propagation

        err = saveResults(data);
        if (err != SUCCESS) return err;  // Manual propagation

        return SUCCESS;
    }

private:
    ErrorCode readData(ifstream& file, string& data) {
        if (!(file >> data)) {
            return ERR_FILE_READ;
        }
        return SUCCESS;
    }

    ErrorCode validateData(const string& data) {
        if (data.empty()) {
            return ERR_INVALID_DATA;
        }
        if (data.size() > 1000) {
            return ERR_DATA_TOO_LARGE;
        }
        return SUCCESS;
    }

    ErrorCode transformData(string& data) {
        // Processing...
        if (someError) {
            return ERR_TRANSFORM;
        }
        return SUCCESS;
    }

    ErrorCode saveResults(const string& data) {
        ofstream out("results.txt");
        if (!out) {
            return ERR_FILE_WRITE;
        }
        out << data;
        return SUCCESS;
    }
};

// Usage:
int main() {
    DataProcessor processor;
    ErrorCode err = processor.process("input.txt");

    // WHY: Must manually check and handle
    if (err != SUCCESS) {
        cerr << "Error code: " << err << endl;
        return 1;
    }

    cout << "Success!" << endl;
    return 0;
}
```

**When to Use Exceptions:**

**✅ USE Exceptions For:**

1. **Constructor Failures:**

```cpp
class Database {
public:
    Database(const string& connectionString) {
        // WHY: Can't return error code from constructor
        if (!connect(connectionString)) {
            throw runtime_error("Connection failed");
        }
    }
};
```

1. **Operator Overloading:**

```cpp
class Matrix {
public:
    Matrix operator+(const Matrix& other) const {
        // WHY: Can't return error code from operator
        if (rows != other.rows) {
            throw invalid_argument("Dimension mismatch");
        }
        return result;
    }
};
```

1. **Deep Call Stacks:**

```cpp
// WHY: Automatic propagation through many layers
main() → service() → business() → data() → database() [error!]
// Exception propagates automatically to main
```

1. **Rare Errors:**

```cpp
void processPayment(double amount) {
    // WHY: Payment usually succeeds
    // Exception for rare failure case
    if (amount < 0) {
        throw invalid_argument("Invalid amount");
    }
    // Happy path continues...
}
```

1. **Library Code:**

```cpp
// WHY: Let user decide how to handle
class Parser {
public:
    Document parse(const string& input) {
        if (!valid(input)) {
            throw parse_error("Invalid syntax");
        }
        return doc;
    }
};
```

**When to Use Error Codes:**

**✅ USE Error Codes For:**

1. **Expected/Frequent Errors:**

```cpp
// WHY: File not found is common, not exceptional
bool openFile(const string& path, FILE*& file) {
    file = fopen(path.c_str(), "r");
    return file != nullptr;
}

// Usage:
FILE* f;
if (!openFile("config.txt", f)) {
    // Use default config
}
```

1. **Performance-Critical Code:**

```cpp
// WHY: Hot loop - exception overhead unacceptable
ErrorCode processPixels(Image& img) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            ErrorCode err = processPixel(x, y);
            if (err != SUCCESS) return err;
        }
    }
    return SUCCESS;
}
```

1. **Real-time Systems:**

```cpp
// WHY: Deterministic timing required
ErrorCode controlLoop() {
    ErrorCode err = readSensors();
    if (err != SUCCESS) return err;

    err = computeControl();
    if (err != SUCCESS) return err;

    return sendActuatorCommands();
}
```

1. **C Compatibility:**

```cpp
extern "C" {
    // WHY: C doesn't have exceptions
    int process_data(const char* input, char* output) {
        if (!input) return -1;
        if (!output) return -2;
        // Process...
        return 0;
    }
}
```

1. **Multiple Error Information:**

```cpp
struct ValidationResult {
    bool success;
    vector<string> errors;
    vector<string> warnings;
};

// WHY: Return multiple issues at once
ValidationResult validate(const Form& form) {
    ValidationResult result;

    if (form.name.empty()) {
        result.errors.push_back("Name required");
    }
    if (form.age < 0) {
        result.errors.push_back("Invalid age");
    }
    if (form.email.find('@') == string::npos) {
        result.warnings.push_back("Email format questionable");
    }

    result.success = result.errors.empty();
    return result;
}
```

**Hybrid Approach:**

```cpp
#include <expected>  // C++23 or third-party

// WHY: Best of both worlds
template <typename T, typename E>
class Result {
    variant<T, E> data;
public:
    bool isOk() const;
    T& value();
    E& error();
};

// Usage:
Result<int, string> divide(int a, int b) {
    if (b == 0) {
        return Error("Division by zero");
    }
    return Ok(a / b);
}

auto result = divide(10, 2);
if (result.isOk()) {
    cout << "Result: " << result.value() << endl;
} else {
    cout << "Error: " << result.error() << endl;
}
```

**Decision Matrix:**

| Scenario | Choice | Reason |
| --- | --- | --- |
| Library API | Exceptions | User decides handling |
| File I/O | Exceptions | Rare, exceptional |
| Parsing | Exceptions | Invalid input exceptional |
| Validation | Error codes | Multiple errors |
| Network | Exceptions | Rare failures |
| Real-time | Error codes | Predictable timing |
| Constructors | Exceptions | No alternative |
| Game loop | Error codes | Performance critical |
| Database | Exceptions | Connection errors rare |
| Sensor reading | Error codes | Expected to fail |

**Key Takeaways:**

1. Exceptions for **exceptional** conditions
2. Error codes for **expected** errors
3. Exceptions in **libraries** (flexible)
4. Error codes in **real-time** systems
5. Exceptions can't be ignored (safer)
6. Error codes predictable (deterministic)
7. Use RAII with exceptions
8. Document your error handling strategy
9. Be consistent within a project
10. Consider hybrid approaches (std::expected)

---