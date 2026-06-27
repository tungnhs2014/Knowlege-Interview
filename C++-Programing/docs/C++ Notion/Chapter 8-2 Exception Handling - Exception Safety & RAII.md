# 8.2. Exception Handling - Exception Safety & RAII

---

## Table of Contents

1. Exception Safety Guarantees
2. RAII and Exception Safety
3. noexcept Specifier - Part 1
4. Summary

**Note:** This is Part 8.2aof Exception Handling. Continue to Part 8.3 for noexcept details, Stack Unwinding, and Best Practices.

---

## 1. Exception Safety Guarantees

### 1.1 What is Exception Safety?

**Exception safety** ensures that code maintains correctness, consistency, and resource integrity even when exceptions are thrown.

**The Problem:**

```cpp
// ❌ NOT exception-safe - resource leak
void processData() {
    int* data = new int[1000];
    processArray(data);  // If this throws, memory leaks!
    delete[] data;
}
```

**Four Levels of Exception Safety:**

| Level | Guarantee | Description |
| --- | --- | --- |
| **No Guarantee** | None | Undefined state, leaks possible |
| **Basic Guarantee** | Valid state | No leaks, objects in valid state |
| **Strong Guarantee** | Transactional | Complete or unchanged |
| **No-throw Guarantee** | Never throws | Guaranteed success |

### 1.2 No Guarantee (Level 0)

**DO NOT WRITE CODE WITH NO GUARANTEE**

```cpp
// ❌ NO GUARANTEE - BAD CODE
class BadClass {
    int* data;
    int size;
public:
    BadClass(int s) : size(s) {
        data = new int[size];
    }

    ~BadClass() {
        delete[] data;
    }

    void resize(int newSize) {
        // ❌ DANGER: If allocation fails, old data lost!
        delete[] data;
        data = new int[newSize];  // May throw!
        size = newSize;
    }
};
```

**Problems:**

- Old data deleted before new allocation succeeds
- If `new` throws, object is in invalid state
- Destructor will try to delete invalid pointer

### 1.3 Basic Guarantee (Level 1)

**Basic Guarantee:** Program remains in valid state, no resource leaks

```cpp
#include <iostream>
#include <vector>
using namespace std;

class BasicSafe {
    vector<int> data;  // WHY: vector handles its own memory
    int counter;

public:
    BasicSafe() : counter(0) {}

    // WHY: Basic guarantee - no leaks, valid state
    void addData(int value) {
        data.push_back(value);  // May throw
        ++counter;  // If push_back succeeds
    }

    // WHY: Even if exception, no leaks
    // Object remains valid (counter may be inconsistent with data.size())
};

int main() {
    BasicSafe obj;

    try {
        for (int i = 0; i < 10; ++i) {
            obj.addData(i);
        }
    } catch (const exception& e) {
        // Object is still valid, no leaks
        cout << "Exception: " << e.what() << endl;
    }

    return 0;
}
```

**Characteristics:**

- ✅ No resource leaks
- ✅ Object in valid state
- ❌ Data may be modified (counter inconsistent)
- ❌ Cannot rollback changes

### 1.4 Strong Guarantee (Level 2)

**Strong Guarantee:** Operation either succeeds completely or has no effect (transactional)

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

class StrongSafe {
    vector<int> data;
    int sum;

    void updateSum() {
        // WHY: Calculate sum from data
        sum = 0;
        for (int val : data) {
            sum += val;
        }
    }

public:
    StrongSafe() : sum(0) {}

    // WHY: Strong guarantee using copy-and-swap
    void addData(int value) {
        vector<int> temp = data;  // 1. Copy (may throw)
        temp.push_back(value);     // 2. Modify copy (may throw)

        // WHY: If we reach here, everything succeeded
        data.swap(temp);           // 3. Swap (no throw)
        updateSum();               // 4. Update (no throw)

        // WHY: Either all changes applied or none
    }

    int getSum() const { return sum; }
    size_t size() const { return data.size(); }
};

int main() {
    StrongSafe obj;

    try {
        obj.addData(10);
        obj.addData(20);
        obj.addData(30);
        cout << "Sum: " << obj.getSum() << endl;
    } catch (const exception& e) {
        // If any addData() fails, object unchanged
        cout << "Exception: " << e.what() << endl;
        cout << "Object unchanged, sum: " << obj.getSum() << endl;
    }

    return 0;
}
```

**Output:**

```
Sum: 60
```

**Characteristics:**

- ✅ No resource leaks
- ✅ Object in valid state
- ✅ All-or-nothing semantics
- ✅ Can rollback changes
- ❌ May be expensive (copying)

### 1.5 No-throw Guarantee (Level 3)

**No-throw Guarantee:** Operation never throws exceptions

```cpp
#include <iostream>
using namespace std;

class NoThrow {
    int value;

public:
    NoThrow() noexcept : value(0) {}

    // WHY: Destructor must never throw
    ~NoThrow() noexcept {
        // Cleanup code that cannot fail
        value = 0;
    }

    // WHY: Move operations should be noexcept
    NoThrow(NoThrow&& other) noexcept : value(other.value) {
        other.value = 0;
    }

    NoThrow& operator=(NoThrow&& other) noexcept {
        if (this != &other) {
            value = other.value;
            other.value = 0;
        }
        return *this;
    }

    // WHY: Swap should be noexcept
    void swap(NoThrow& other) noexcept {
        int temp = value;
        value = other.value;
        other.value = temp;
    }

    int getValue() const noexcept { return value; }
};
```

**When Required:**

- ✅ **ALWAYS:** Destructors
- ✅ **ALWAYS:** Move constructors
- ✅ **ALWAYS:** Move assignment operators
- ✅ **ALWAYS:** swap operations
- ✅ **USUALLY:** Memory deallocation

### 1.6 Copy-and-Swap Idiom

**The copy-and-swap idiom provides strong exception safety:**

```cpp
#include <iostream>
#include <cstring>
#include <algorithm>
using namespace std;

class String {
    char* data;
    size_t length;

public:
    // Constructor
    String(const char* str = "") {
        length = strlen(str);
        data = new char[length + 1];
        strcpy(data, str);
    }

    // Destructor
    ~String() {
        delete[] data;
    }

    // Copy constructor
    String(const String& other) {
        length = other.length;
        data = new char[length + 1];
        strcpy(data, other.data);
    }

    // WHY: Copy-and-swap for strong exception safety
    String& operator=(const String& other) {
        // Step 1: Create temporary copy (may throw)
        String temp(other);

        // Step 2: Swap with temporary (no throw)
        swap(temp);

        // Step 3: Temporary destroyed, old data freed
        return *this;
    }

    // WHY: No-throw swap
    void swap(String& other) noexcept {
        std::swap(data, other.data);
        std::swap(length, other.length);
    }

    const char* c_str() const { return data; }
};

int main() {
    String s1("Hello");
    String s2("World");

    cout << "Before: s1 = " << s1.c_str() << endl;

    try {
        s1 = s2;  // Strong guarantee: either succeeds or s1 unchanged
        cout << "After: s1 = " << s1.c_str() << endl;
    } catch (...) {
        cout << "Assignment failed, s1 unchanged" << endl;
    }

    return 0;
}
```

**Output:**

```
Before: s1 = Hello
After: s1 = World
```

**How Copy-and-Swap Works:**

```cpp
String& operator=(const String& other) {
    // 1. Create copy (may throw)
    //    - If throws here, *this is unchanged ✓
    String temp(other);

    // 2. Swap (never throws)
    //    - Now *this has new value
    //    - temp has old value
    swap(temp);

    // 3. Return
    //    - temp destroyed
    //    - Old data automatically cleaned up
    return *this;
}
```

**Benefits:**

- ✅ Strong exception safety
- ✅ Handles self-assignment automatically
- ✅ Code reuse (uses copy constructor)
- ✅ Simple and elegant

### 1.7 Exception Safety Matrix

**Minimum exception safety level for different operations:**

| Operation | Minimum Level | Reason |
| --- | --- | --- |
| **Destructor** | No-throw | Called during unwinding |
| **Move operations** | No-throw | Performance, rollback impossible |
| **swap** | No-throw | Used for strong guarantee |
| **Assignment** | Strong | Should be transactional |
| **Constructors** | Basic | Objects must be valid |
| **Regular methods** | Basic | Valid state, no leaks |

---

## 2. RAII and Exception Safety

### 2.1 What is RAII?

**RAII (Resource Acquisition Is Initialization):** Resource lifetime is bound to object lifetime.

**Principle:**

- Acquire resources in constructor
- Release resources in destructor
- Rely on automatic destructor calls

**Without RAII:**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

// ❌ NOT exception-safe
void processFile(const string& filename) {
    ifstream file(filename);

    if (!file.is_open()) {
        throw runtime_error("Cannot open file");
    }

    // Process file...
    string line;
    while (getline(file, line)) {
        if (line.empty()) {
            file.close();  // Manual cleanup
            throw runtime_error("Empty line");
        }
        // Process line...
    }

    file.close();  // Manual cleanup
}
```

**Problems:**

- Must remember to close in every path
- If exception thrown, file may not close
- Easy to forget cleanup code

**With RAII:**

```cpp
#include <iostream>
#include <fstream>
using namespace std;

// ✅ Exception-safe with RAII
void processFile(const string& filename) {
    // WHY: File opened in constructor
    ifstream file(filename);

    if (!file.is_open()) {
        throw runtime_error("Cannot open file");
    }

    // Process file...
    string line;
    while (getline(file, line)) {
        if (line.empty()) {
            throw runtime_error("Empty line");
            // WHY: file automatically closed when exception thrown!
        }
        // Process line...
    }

    // WHY: file automatically closed when function returns
}
```

**Benefits:**

- ✅ Automatic cleanup in all paths
- ✅ Exception-safe
- ✅ No manual cleanup needed
- ✅ Cannot forget to release

### 2.2 RAII for Memory Management

**Without RAII:**

```cpp
#include <iostream>
using namespace std;

void processData() {
    int* data = new int[100];

    try {
        // Process data...
        if (/* error */) {
            delete[] data;  // Manual cleanup
            throw runtime_error("Error");
        }

        // More processing...
        if (/* another error */) {
            delete[] data;  // Manual cleanup again!
            throw runtime_error("Error");
        }

        delete[] data;  // Manual cleanup

    } catch (...) {
        delete[] data;  // Manual cleanup in catch!
        throw;
    }
}
```

**With RAII (Smart Pointers):**

```cpp
#include <iostream>
#include <memory>
using namespace std;

void processData() {
    // WHY: unique_ptr manages memory automatically
    unique_ptr<int[]> data(new int[100]);

    // Process data...
    if (/* error */) {
        throw runtime_error("Error");
        // WHY: Memory automatically freed!
    }

    // More processing...
    if (/* another error */) {
        throw runtime_error("Error");
        // WHY: Memory automatically freed!
    }

    // WHY: Memory automatically freed when unique_ptr destroyed
}
```

### 2.3 Standard RAII Classes

```cpp
#include <iostream>
#include <memory>
#include <fstream>
#include <mutex>
using namespace std;

void demonstrateRAII() {
    // 1. Smart pointers - automatic memory management
    {
        unique_ptr<int> ptr(new int(42));
        // WHY: Automatically deleted when scope ends
    }

    {
        shared_ptr<int> ptr1 = make_shared<int>(100);
        shared_ptr<int> ptr2 = ptr1;  // Reference count = 2
        // WHY: Deleted when last shared_ptr destroyed
    }

    // 2. File streams - automatic file closing
    {
        ofstream file("data.txt");
        file << "Hello";
        // WHY: Automatically closed when scope ends
    }

    // 3. Lock guards - automatic mutex unlocking
    {
        mutex mtx;
        lock_guard<mutex> lock(mtx);
        // Critical section
        // WHY: Mutex automatically unlocked when scope ends
    }

    // 4. Unique lock - more flexible
    {
        mutex mtx;
        unique_lock<mutex> lock(mtx);
        // Can manually unlock/lock
        lock.unlock();
        // Do something without lock
        lock.lock();
        // WHY: Still automatically unlocked when destroyed
    }
}
```

### 2.4 Custom RAII Wrapper

```cpp
#include <iostream>
using namespace std;

// WHY: Generic RAII wrapper for any resource
template<typename T>
class ResourceGuard {
    T* resource;
    void (*deleter)(T*);

public:
    // Acquire resource in constructor
    ResourceGuard(T* res, void (*del)(T*))
        : resource(res), deleter(del) {
        if (!resource) {
            throw runtime_error("Null resource");
        }
        cout << "Resource acquired" << endl;
    }

    // Release resource in destructor
    ~ResourceGuard() {
        if (resource && deleter) {
            deleter(resource);
            cout << "Resource released" << endl;
        }
    }

    // Prevent copying
    ResourceGuard(const ResourceGuard&) = delete;
    ResourceGuard& operator=(const ResourceGuard&) = delete;

    // Allow move
    ResourceGuard(ResourceGuard&& other) noexcept
        : resource(other.resource), deleter(other.deleter) {
        other.resource = nullptr;
        other.deleter = nullptr;
    }

    T* get() { return resource; }
};

// Example: File handle RAII
void closeFile(FILE** f) {
    if (f && *f) {
        fclose(*f);
        *f = nullptr;
    }
}

int main() {
    try {
        FILE* f = fopen("test.txt", "w");
        ResourceGuard<FILE*> guard(&f, closeFile);

        // Use file...
        fprintf(f, "Hello World");

        // Simulate error
        throw runtime_error("Something went wrong");

    } catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }

    // File automatically closed by guard destructor
    cout << "Program continues" << endl;

    return 0;
}
```

**Output:**

```
Resource acquired
Error: Something went wrong
Resource released
Program continues
```

### 2.5 RAII and Container Exception Safety

**How vector uses move operations:**

```cpp
#include <iostream>
#include <vector>
using namespace std;

class Widget {
    int value;
public:
    Widget(int v) : value(v) {
        cout << "Construct(" << value << ")" << endl;
    }

    ~Widget() {
        cout << "Destruct(" << value << ")" << endl;
    }

    // WHY: If move constructor is noexcept, vector will use it
    Widget(Widget&& other) noexcept : value(other.value) {
        cout << "Move(" << value << ")" << endl;
        other.value = 0;
    }

    // WHY: If not noexcept, vector will copy instead
    Widget(const Widget& other) : value(other.value) {
        cout << "Copy(" << value << ")" << endl;
    }
};

int main() {
    cout << "=== Creating vector ===" << endl;
    vector<Widget> vec;
    vec.reserve(2);  // Capacity = 2

    cout << "\n=== Adding first element ===" << endl;
    vec.push_back(Widget(1));

    cout << "\n=== Adding second element ===" << endl;
    vec.push_back(Widget(2));

    cout << "\n=== Adding third element (reallocation) ===" << endl;
    vec.push_back(Widget(3));  // Capacity exceeded, reallocation needed

    cout << "\n=== End of program ===" << endl;
    return 0;
}
```

**Output (with noexcept move):**

```
=== Creating vector ===

=== Adding first element ===
Construct(1)
Move(1)
Destruct(0)

=== Adding second element ===
Construct(2)
Move(2)
Destruct(0)

=== Adding third element (reallocation) ===
Construct(3)
Move(1)    // WHY: noexcept move used for reallocation
Move(2)
Move(3)
Destruct(0)
Destruct(0)
Destruct(0)

=== End of program ===
Destruct(1)
Destruct(2)
Destruct(3)
```

**WHY noexcept move matters:**

- If move constructor can throw, vector cannot provide strong guarantee
- Vector will use copy constructor instead (slower but safer)
- With noexcept move, vector can safely use move (faster)

---

## 3. noexcept Specifier (C++11)

### 3.1 Basic Syntax

```cpp
#include <iostream>
using namespace std;

// WHY: noexcept means function guarantees not to throw
void safeFunction() noexcept {
    // Code that never throws
}

// WHY: noexcept(true) is same as noexcept
void safeFunction2() noexcept(true) {
    // Code that never throws
}

// WHY: noexcept(false) means may throw (default for most functions)
void mayThrowFunction() noexcept(false) {
    throw runtime_error("Error");
}

// WHY: Check at compile time if expression is noexcept
void conditionalNoexcept() noexcept(noexcept(safeFunction())) {
    safeFunction();
}
```

### 3.2 What Happens if noexcept Function Throws?

```cpp
#include <iostream>
#include <exception>
using namespace std;

void badFunction() noexcept {
    cout << "About to throw from noexcept function" << endl;
    throw runtime_error("This should not happen!");  // ❌ DANGER
    cout << "This never executes" << endl;
}

int main() {
    try {
        badFunction();
    } catch (const exception& e) {
        // WHY: This catch block is NEVER reached!
        cout << "Caught: " << e.what() << endl;
    }

    cout << "Program continues" << endl;  // Never reached
    return 0;
}
```

**Output:**

```
About to throw from noexcept function
terminate called after throwing an instance of 'std::runtime_error'
  what():  This should not happen!
Aborted (core dumped)
```

**WHY std::terminate() is called:**

- noexcept is a promise to the compiler
- If promise broken, program immediately terminates
- No stack unwinding occurs
- Destructors NOT called
- Use noexcept only when you're CERTAIN function won't throw

### 3.3 Move Constructor Must Be noexcept

**Why move constructors should be noexcept:**

```cpp
#include <iostream>
#include <vector>
using namespace std;

class Widget {
    int* data;
public:
    Widget(int value) : data(new int(value)) {
        cout << "Construct(" << *data << ")" << endl;
    }

    ~Widget() {
        cout << "Destruct";
        if (data) cout << "(" << *data << ")";
        cout << endl;
        delete data;
    }

    // Copy constructor
    Widget(const Widget& other) : data(new int(*other.data)) {
        cout << "COPY(" << *data << ")" << endl;
    }

    // WHY: Move constructor WITHOUT noexcept
    Widget(Widget&& other) /* no noexcept */ : data(other.data) {
        cout << "MOVE(" << *data << ")" << endl;
        other.data = nullptr;
    }
};

int main() {
    cout << "Creating vector with capacity 2" << endl;
    vector<Widget> vec;
    vec.reserve(2);

    vec.push_back(Widget(1));
    vec.push_back(Widget(2));

    cout << "\nAdding third element (reallocation needed)" << endl;
    vec.push_back(Widget(3));
    // WHY: Without noexcept, vector uses COPY instead of MOVE!

    return 0;
}
```

**Output (without noexcept):**

```
Creating vector with capacity 2
Construct(1)
MOVE(1)
Destruct
Construct(2)
MOVE(2)
Destruct

Adding third element (reallocation needed)
Construct(3)
COPY(1)    // WHY: Vector uses COPY because move is not noexcept!
COPY(2)
MOVE(3)
Destruct
Destruct
Destruct(1)
Destruct(2)
Destruct(3)
```

**With noexcept move:**

```cpp
// WHY: Move constructor WITH noexcept
Widget(Widget&& other) noexcept : data(other.data) {
    cout << "MOVE(" << *data << ")" << endl;
    other.data = nullptr;
}
```

**Output (with noexcept):**

```
Creating vector with capacity 2
Construct(1)
MOVE(1)
Destruct
Construct(2)
MOVE(2)
Destruct

Adding third element (reallocation needed)
Construct(3)
MOVE(1)    // WHY: Vector uses MOVE because it's noexcept!
MOVE(2)
MOVE(3)
Destruct
Destruct
Destruct
Destruct(1)
Destruct(2)
Destruct(3)
```

**Performance Impact:**

- Copy: Allocate new memory, copy data (slow)
- Move: Transfer pointer ownership (fast)
- Without noexcept move: Vector forced to copy (safe but slow)
- With noexcept move: Vector can move (fast)

---

## Summary

### Key Takeaways from Part 8.2a

1. **Four Exception Safety Levels** - No guarantee (never use), basic (valid state, no leaks), strong (transactional all-or-nothing), no-throw (never throws). Choose appropriate level for each operation.
2. **Copy-and-Swap Idiom for Strong Guarantee** - Create temporary copy (may throw), swap with temporary (no throw), temporary destroys old data. Provides transactional semantics and handles self-assignment.
3. **RAII is Essential for Exception Safety** - Resource lifetime bound to object lifetime. Acquire in constructor, release in destructor. Automatic cleanup in all paths including exceptions.
4. **Standard RAII Classes Save Time** - Use `unique_ptr`, `shared_ptr` for memory, `ifstream`/`ofstream` for files, `lock_guard`/`unique_lock` for mutexes. Don't reinvent the wheel.
5. **Move Constructors Must Be noexcept** - Vector reallocation: if move not noexcept, vector copies (slow) instead of moves (fast). Strong guarantee impossible with throwing move. Always mark move operations noexcept.

---