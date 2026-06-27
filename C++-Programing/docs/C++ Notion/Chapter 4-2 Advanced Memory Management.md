# 4.2. Advanced Memory Management

---

## Table of Contents

1. C-Style Memory Management
2. Common Memory Errors
3. Memory Management Best Practices
4. Advanced Memory Concepts
5. Summary

---

## 1. C-Style Memory Management

### 1.1 malloc(), calloc(), realloc(), free()

**Purpose**: C-style memory allocation functions (inherited from C).

**Why Learn These?**

- Interfacing with C libraries
- Understanding lower-level memory management
- Some specialized use cases (realloc)
- Legacy code maintenance

**Functions Overview:**

```cpp
#include <cstdlib>  // Or <stdlib.h> for C

void* malloc(size_t size);              // Allocate uninitialized memory
void* calloc(size_t num, size_t size);  // Allocate zero-initialized memory
void* realloc(void* ptr, size_t size);  // Resize allocation
void free(void* ptr);                    // Deallocate memory
```

**malloc() - Memory Allocation:**

```cpp
// malloc_example.cpp
#include <iostream>
#include <cstdlib>  // For malloc, free
using namespace std;

int main() {
    // WHY: malloc returns void*, must cast to desired type
    int* ptr = (int*)malloc(sizeof(int) * 5);

    // WHY: Always check for allocation failure
    if (ptr == nullptr) {
        cerr << "Allocation failed!" << endl;
        return 1;
    }

    // WHY: malloc doesn't initialize - contains garbage
    cout << "Garbage values: ";
    for (int i = 0; i < 5; i++) {
        cout << ptr[i] << " ";
    }
    cout << endl;

    // WHY: Must manually initialize
    for (int i = 0; i < 5; i++) {
        ptr[i] = i * 10;
    }

    cout << "After initialization: ";
    for (int i = 0; i < 5; i++) {
        cout << ptr[i] << " ";
    }
    cout << endl;

    // WHY: Use free() for malloc'd memory (not delete!)
    free(ptr);

    return 0;
}
```

**calloc() - Clear Allocation:**

```cpp
// calloc_example.cpp
#include <iostream>
#include <cstdlib>
using namespace std;

int main() {
    // WHY: calloc initializes all bytes to zero
    // Signature: calloc(number_of_elements, size_of_each)
    int* ptr = (int*)calloc(5, sizeof(int));

    if (ptr == nullptr) {
        cerr << "Allocation failed!" << endl;
        return 1;
    }

    // WHY: All values are zero-initialized
    cout << "Zero-initialized: ";
    for (int i = 0; i < 5; i++) {
        cout << ptr[i] << " ";  // All zeros
    }
    cout << endl;

    // WHY: Performance consideration
    cout << "\nNote: calloc is slightly slower than malloc" << endl;
    cout << "because it writes zeros to memory." << endl;

    free(ptr);

    return 0;
}
```

**realloc() - Resize Allocation:**

```cpp
// realloc_example.cpp
#include <iostream>
#include <cstdlib>
using namespace std;

int main() {
    // Initial allocation
    int size = 5;
    int* ptr = (int*)malloc(sizeof(int) * size);

    // Initialize
    for (int i = 0; i < size; i++) {
        ptr[i] = i + 1;
    }

    cout << "Original array (" << size << " elements): ";
    for (int i = 0; i < size; i++) {
        cout << ptr[i] << " ";
    }
    cout << endl;

    // WHY: Resize to larger size
    int newSize = 10;

    // CRITICAL: Use temporary pointer for realloc
    // If realloc fails, it returns nullptr but original ptr is still valid
    int* temp = (int*)realloc(ptr, sizeof(int) * newSize);

    if (temp == nullptr) {
        cerr << "Reallocation failed!" << endl;
        free(ptr);  // Original ptr still valid, must free
        return 1;
    }

    // WHY: Only update ptr if realloc succeeded
    ptr = temp;

    // WHY: Initialize new elements (realloc doesn't initialize them)
    for (int i = size; i < newSize; i++) {
        ptr[i] = i + 1;
    }

    cout << "Resized array (" << newSize << " elements): ";
    for (int i = 0; i < newSize; i++) {
        cout << ptr[i] << " ";
    }
    cout << endl;

    free(ptr);

    return 0;
}
```

**free() - Deallocation:**

```cpp
// free_example.cpp
#include <iostream>
#include <cstdlib>
using namespace std;

int main() {
    int* ptr = (int*)malloc(sizeof(int) * 10);

    if (ptr) {
        // Use memory...

        // WHY: free() doesn't set pointer to nullptr
        free(ptr);

        // BEST PRACTICE: Set to nullptr after free
        ptr = nullptr;

        // WHY: Calling free(nullptr) is safe (does nothing)
        free(ptr);  // Safe
    }

    return 0;
}
```

### 1.2 new vs malloc - Complete Comparison

**The 10 Critical Differences:**

| # | Aspect | new/delete | malloc/free |
| --- | --- | --- | --- |
| 1 | **Language** | C++ only | C and C++ |
| 2 | **Type** | Operator (keyword) | Function (library) |
| 3 | **Return Type** | Typed pointer (`T*`) | `void*` (needs cast) |
| 4 | **Size** | Automatic (`sizeof(T)`) | Manual (must calculate) |
| 5 | **Initialization** | Calls constructor | No initialization |
| 6 | **Cleanup** | Calls destructor | No destructor call |
| 7 | **Failure** | Throws `bad_alloc` | Returns `nullptr` |
| 8 | **Overloadable** | Yes (can customize) | No |
| 9 | **Resize** | No | Yes (`realloc`) |
| 10 | **Type Safety** | Type-safe | Not type-safe |

**Demonstration Code:**

```cpp
// new_vs_malloc_complete.cpp
#include <iostream>
#include <cstdlib>
using namespace std;

class Resource {
public:
    int* data;

    Resource() {
        data = new int[100];
        cout << "Constructor: Allocated 100 integers" << endl;
    }

    ~Resource() {
        delete[] data;
        cout << "Destructor: Freed 100 integers" << endl;
    }

    void use() {
        data[0] = 42;
        cout << "Using resource: data[0] = " << data[0] << endl;
    }
};

int main() {
    cout << "=== Difference 1-2: Operator vs Function ===" << endl;
    int* ptr1 = new int(10);      // Operator
    int* ptr2 = (int*)malloc(sizeof(int));  // Function call
    delete ptr1;
    free(ptr2);

    cout << "\n=== Difference 3: Return Type ===" << endl;
    // WHY: new returns typed pointer (no cast needed)
    int* arr1 = new int[10];

    // WHY: malloc returns void*, needs cast
    int* arr2 = (int*)malloc(sizeof(int) * 10);

    delete[] arr1;
    free(arr2);

    cout << "\n=== Difference 4: Size Calculation ===" << endl;
    // WHY: new automatically calculates size
    double* d1 = new double[5];  // Compiler knows sizeof(double)

    // WHY: malloc requires manual calculation
    double* d2 = (double*)malloc(sizeof(double) * 5);

    delete[] d1;
    free(d2);

    cout << "\n=== Difference 5-6: Constructor/Destructor ===" << endl;

    cout << "With new (C++ way):" << endl;
    {
        Resource* r1 = new Resource();  // Constructor called
        r1->use();
        delete r1;                      // Destructor called
    }

    cout << "\nWith malloc (C way):" << endl;
    {
        Resource* r2 = (Resource*)malloc(sizeof(Resource));
        // WHY: Constructor NOT called! data pointer is garbage
        // r2->use();  // ❌ DANGEROUS! data not initialized

        cout << "No constructor called - object invalid!" << endl;

        // WHY: Destructor NOT called! Memory leak for data array
        free(r2);
        cout << "No destructor called - leaked 100 integers!" << endl;
    }

    cout << "\n=== Difference 7: Failure Handling ===" << endl;

    // WHY: new throws exception
    cout << "new behavior: ";
    try {
        int* big1 = new int[1000000000000LL];
        delete[] big1;
    } catch (bad_alloc&) {
        cout << "Caught exception" << endl;
    }

    // WHY: malloc returns nullptr
    cout << "malloc behavior: ";
    int* big2 = (int*)malloc(sizeof(int) * 1000000000000LL);
    if (big2 == nullptr) {
        cout << "Got nullptr" << endl;
    }

    return 0;
}
```

**When to Use Which:**

**Use new/delete when:**

- ✅ Working with C++ classes/objects
- ✅ Need constructor/destructor calls
- ✅ Want type safety
- ✅ Writing modern C++ code
- ✅ Using standard library containers

**Use malloc/free when:**

- ✅ Interfacing with C libraries
- ✅ Need `realloc()` functionality
- ✅ Legacy C code
- ❌ Avoid in new C++ projects

### 1.3 Mixing new/free - Why It Fails

**Critical Rule**: **NEVER mix new/delete with malloc/free!**

**Why Mixing is Catastrophic:**

```cpp
// mixing_danger.cpp
#include <iostream>
#include <cstdlib>
using namespace std;

class Resource {
public:
    int* data;

    Resource() {
        data = new int[100];
        cout << "Constructor: Allocated 100 integers" << endl;
    }

    ~Resource() {
        delete[] data;
        cout << "Destructor: Freed 100 integers" << endl;
    }
};

int main() {
    cout << "=== CORRECT: new + delete ===" << endl;
    {
        Resource* obj1 = new Resource();  // Constructor called
        delete obj1;                       // Destructor called
    }

    cout << "\n=== WRONG: new + free ===" << endl;
    {
        Resource* obj2 = new Resource();  // Constructor called

        // ❌ CRITICAL ERROR: Using free with new
        // free(obj2);
        // DISASTER: Destructor NOT called!
        // - data array (100 integers) LEAKED
        // - Resources not properly released
        // - Heap corruption possible

        // Simulating the problem:
        cout << "If we used free() here:" << endl;
        cout << "- Destructor would NOT be called" << endl;
        cout << "- 100 integers would leak forever" << endl;
        cout << "- File handles, mutexes, etc. would leak" << endl;

        // Correct way:
        delete obj2;  // ✅ Destructor called
    }

    cout << "\n=== WRONG: malloc + delete ===" << endl;
    {
        // ❌ CRITICAL ERROR: Using delete with malloc
        Resource* obj3 = (Resource*)malloc(sizeof(Resource));

        // delete obj3;
        // DISASTER: Tries to call non-existent destructor
        // - Undefined behavior
        // - May crash immediately
        // - May corrupt heap

        cout << "If we used delete here:" << endl;
        cout << "- Would try to call destructor on uninitialized object" << endl;
        cout << "- Undefined behavior (crash or corruption)" << endl;
        cout << "- Constructor was never called!" << endl;

        // Correct way:
        free(obj3);  // ✅ But note: constructor never called anyway
    }

    cout << "\n=== WRONG: new[] + delete ===" << endl;
    {
        int* arr = new int[10];

        // delete arr;  // ❌ WRONG! Should be delete[]
        // Only destroys first element
        // Rest of array leaked

        delete[] arr;  // ✅ Correct
    }

    return 0;
}
```

**The Memory Management Matrix:**

```
✅ CORRECT PAIRS:
new        →  delete
new[]      →  delete[]
malloc()   →  free()
calloc()   →  free()
realloc()  →  free()

❌ NEVER MIX:
new        ↮  free()      (destructor not called)
new[]      ↮  free()      (destructors not called)
malloc()   ↮  delete      (undefined behavior)
malloc()   ↮  delete[]    (undefined behavior)
new[]      ↮  delete      (only first element freed)
new        ↮  delete[]    (undefined behavior)
```

---

## 2. Common Memory Errors

### 2.1 Memory Leaks

**Definition**: Allocated memory that is never freed.

**Why It's Critical:**

- Gradually consumes all available RAM
- Program performance degrades
- Eventually causes allocation failures
- Can crash entire system (server, embedded device)

**Common Causes:**

```cpp
// memory_leaks_complete.cpp
#include <iostream>
using namespace std;

// ❌ LEAK 1: Forgot to delete
void leak1() {
    int* ptr = new int(42);
    cout << *ptr << endl;
    // Forgot delete! Memory leaked when function returns
}

// ❌ LEAK 2: Exception before delete
void leak2() {
    int* ptr = new int[1000];

    // Simulating error
    if (true) {
        throw runtime_error("Error!");
    }

    delete[] ptr;  // Never reached! Memory leaked
}

// ❌ LEAK 3: Reassigning pointer
void leak3() {
    int* ptr = new int(10);

    // Lost reference to first allocation!
    ptr = new int(20);  // Memory leak!

    delete ptr;  // Only deletes second allocation
}

// ❌ LEAK 4: Leak in loop
void leak4() {
    for (int i = 0; i < 1000; i++) {
        int* ptr = new int(i);
        // Forgot delete! Leaks 1000 integers
    }
}

// ❌ LEAK 5: Conditional paths
void leak5(bool condition) {
    int* ptr = new int(99);

    if (condition) {
        return;  // Early return without delete!
    }

    delete ptr;  // Not reached if condition true
}

// ✅ CORRECT: Proper cleanup
void noLeak1() {
    int* ptr = new int(42);
    cout << *ptr << endl;
    delete ptr;  // ✅ Properly freed
}

// ✅ CORRECT: Exception-safe
void noLeak2() {
    int* ptr = nullptr;

    try {
        ptr = new int[1000];
        // Operations...
        throw runtime_error("Error!");
    } catch (...) {
        delete[] ptr;  // ✅ Cleanup in catch
        throw;
    }

    delete[] ptr;
}

// ✅ CORRECT: Delete before reassign
void noLeak3() {
    int* ptr = new int(10);

    delete ptr;  // ✅ Free first
    ptr = new int(20);

    delete ptr;  // ✅ Free second
}

// ✅ CORRECT: Delete in loop
void noLeak4() {
    for (int i = 0; i < 1000; i++) {
        int* ptr = new int(i);
        delete ptr;  // ✅ Freed each iteration
    }
}

// ✅ CORRECT: All paths cleanup
void noLeak5(bool condition) {
    int* ptr = new int(99);

    if (condition) {
        delete ptr;  // ✅ Cleanup before return
        return;
    }

    delete ptr;  // ✅ Cleanup normal path
}

int main() {
    cout << "Demonstrating correct memory management..." << endl;
    noLeak1();
    noLeak3();
    noLeak4();
    noLeak5(true);
    noLeak5(false);
    cout << "All memory properly managed!" << endl;

    return 0;
}
```

### 2.2 Dangling Pointers

**Definition**: Pointer pointing to deallocated or invalid memory.

**Why It's Dangerous:**

- Reading: Returns garbage or crashes
- Writing: Corrupts memory or crashes
- Hardest bugs to find (non-deterministic)
- Can work "sometimes" then crash randomly

**Common Causes:**

```cpp
// dangling_pointers_complete.cpp
#include <iostream>
using namespace std;

// ❌ DANGLING 1: Using after delete
void dangling1() {
    int* ptr = new int(42);
    cout << "Before delete: " << *ptr << endl;

    delete ptr;

    // ❌ DANGLING POINTER!
    // cout << *ptr << endl;  // Undefined behavior!
}

// ❌ DANGLING 2: Returning pointer to local
int* dangling2() {
    int x = 100;
    return &x;  // ❌ x destroyed when function returns
}

// ❌ DANGLING 3: Pointer to temporary
void dangling3() {
    int* ptr;
    {
        int temp = 50;
        ptr = &temp;  // ❌ temp destroyed at block end
    }
    // ptr is now dangling!
    // cout << *ptr << endl;  // Undefined behavior!
}

// ❌ DANGLING 4: Double delete
void dangling4() {
    int* ptr = new int(42);
    delete ptr;
    // delete ptr;  // ❌ Double delete! Undefined behavior
}

// ✅ CORRECT: Set to nullptr
void noDangling1() {
    int* ptr = new int(42);
    cout << *ptr << endl;

    delete ptr;
    ptr = nullptr;  // ✅ No longer dangling

    if (ptr != nullptr) {
        cout << *ptr << endl;
    } else {
        cout << "Pointer is null (safe)" << endl;
    }
}

// ✅ CORRECT: Return heap memory
int* noDangling2() {
    int* ptr = new int(100);  // ✅ On heap, persists
    return ptr;
}

// ✅ CORRECT: Check before delete
void noDangling4() {
    int* ptr = new int(42);

    if (ptr != nullptr) {
        delete ptr;
        ptr = nullptr;
    }

    // Safe to "delete" again
    if (ptr != nullptr) {
        delete ptr;  // Won't execute
    }
}

int main() {
    cout << "=== Safe pointer usage ===" << endl;
    noDangling1();

    int* result = noDangling2();
    cout << "Returned value: " << *result << endl;
    delete result;

    noDangling4();

    return 0;
}
```

### 2.3 Double Deletion

**Definition**: Deleting the same memory twice.

**Why It's Fatal:**

- Corrupts heap metadata
- Usually causes immediate crash
- Can corrupt other allocations
- Exploitable security vulnerability

```cpp
// double_deletion.cpp
#include <iostream>
using namespace std;

int main() {
    cout << "=== Double Deletion Problem ===" << endl;

    int* ptr = new int(42);
    cout << "Value: " << *ptr << endl;

    delete ptr;    // First delete: OK

    // ❌ DOUBLE DELETION!
    // delete ptr;    // CRASH! Corrupts memory

    cout << "After first delete, ptr still holds address: " << ptr << endl;
    cout << "But memory is freed - ptr is dangling!" << endl;

    // ✅ SOLUTION: Set to nullptr
    ptr = nullptr;
    delete ptr;    // ✅ Safe! Deleting nullptr does nothing

    cout << "\n=== Safe Deletion Pattern ===" << endl;

    int* ptr2 = new int(99);

    // Pattern: Always check and nullify
    if (ptr2 != nullptr) {
        delete ptr2;
        ptr2 = nullptr;
    }

    // Safe to call again
    if (ptr2 != nullptr) {
        delete ptr2;  // Won't execute
    }

    cout << "No crashes!" << endl;

    return 0;
}
```

**Safe Deletion Macros:**

```cpp
// safe_delete.cpp
#include <iostream>
using namespace std;

// WHY: Macro ensures nullptr assignment
#define SAFE_DELETE(p) \
    do { \
        if (p) { \
            delete (p); \
            (p) = nullptr; \
        } \
    } while(0)

#define SAFE_DELETE_ARRAY(p) \
    do { \
        if (p) { \
            delete[] (p); \
            (p) = nullptr; \
        } \
    } while(0)

int main() {
    int* ptr = new int(42);
    int* arr = new int[10];

    // WHY: Can call multiple times safely
    SAFE_DELETE(ptr);
    SAFE_DELETE(ptr);  // Safe

    SAFE_DELETE_ARRAY(arr);
    SAFE_DELETE_ARRAY(arr);  // Safe

    cout << "All deletions safe!" << endl;

    return 0;
}
```

### 2.4 Wild Pointers

**Definition**: Uninitialized pointer containing random address.

**Why It's Dangerous:**

- Points to random memory location
- Dereferencing crashes or corrupts data
- Hardest to debug (random behavior)

```cpp
// wild_pointers.cpp
#include <iostream>
using namespace std;

int main() {
    // ❌ WILD POINTER! Contains garbage address
    int* wildPtr;

    cout << "Wild pointer contains: " << wildPtr << endl;
    cout << "This is a random address!" << endl;

    // ❌ UNDEFINED BEHAVIOR!
    // *wildPtr = 42;  // May crash or corrupt random memory!
    // cout << *wildPtr << endl;  // DANGEROUS!

    cout << "\n=== Solutions ===" << endl;

    // ✅ SOLUTION 1: Initialize to nullptr
    int* safePtr1 = nullptr;

    if (safePtr1 != nullptr) {
        *safePtr1 = 42;
    } else {
        cout << "Pointer not allocated, safe to check" << endl;
    }

    // ✅ SOLUTION 2: Initialize with allocation
    int* safePtr2 = new int(42);
    cout << "Allocated value: " << *safePtr2 << endl;
    delete safePtr2;
    safePtr2 = nullptr;

    // ✅ SOLUTION 3: Initialize to valid address
    int x = 100;
    int* safePtr3 = &x;
    cout << "Stack value: " << *safePtr3 << endl;

    return 0;
}
```

---

## 3. Memory Management Best Practices

### 3.1 RAII Pattern (Resource Acquisition Is Initialization)

**Core Principle**: Tie resource lifetime to object lifetime.

**Why RAII is Fundamental:**

- Automatic cleanup (no manual delete)
- Exception-safe by design
- Prevents all common memory errors
- Foundation of modern C++

**Basic RAII Wrapper:**

```cpp
// raii_complete.cpp
#include <iostream>
using namespace std;

class IntArray {
private:
    int* data;
    int size;

public:
    // WHY: Constructor acquires resource
    IntArray(int s) : size(s) {
        data = new int[size];
        cout << "✅ Array allocated (" << size << " elements)" << endl;
    }

    // WHY: Destructor releases resource automatically
    ~IntArray() {
        delete[] data;
        cout << "✅ Array deallocated" << endl;
    }

    // WHY: Delete copy to prevent double deletion
    IntArray(const IntArray&) = delete;
    IntArray& operator=(const IntArray&) = delete;

    // WHY: Provide safe access
    int& operator[](int index) {
        return data[index];
    }

    int getSize() const { return size; }
};

int main() {
    cout << "=== RAII Demonstration ===" << endl;

    {
        // WHY: Allocation automatic in constructor
        IntArray arr(5);

        // Use array
        for (int i = 0; i < arr.getSize(); i++) {
            arr[i] = i * 10;
        }

        for (int i = 0; i < arr.getSize(); i++) {
            cout << arr[i] << " ";
        }
        cout << endl;

        // WHY: No manual delete needed!
        // Destructor called automatically here ↓
    }

    cout << "✅ No memory leaks!" << endl;
    cout << "✅ Exception-safe!" << endl;

    return 0;
}
```

**RAII with Exception Safety:**

```cpp
// raii_exception_safe.cpp
#include <iostream>
#include <stdexcept>
using namespace std;

class Buffer {
private:
    char* data;
    int size;

public:
    Buffer(int s) : size(s) {
        data = new char[size];
        cout << "Buffer allocated (" << size << " bytes)" << endl;
    }

    ~Buffer() {
        delete[] data;
        cout << "Buffer deallocated" << endl;
    }

    void process() {
        throw runtime_error("Processing error!");
    }
};

void withoutRAII() {
    cout << "\n=== Without RAII ===" << endl;

    char* buffer = new char[1000];
    cout << "Buffer allocated" << endl;

    try {
        throw runtime_error("Error!");

        delete[] buffer;  // ❌ Never reached! Memory leak!
    } catch (...) {
        // Must manually clean up
        delete[] buffer;
        cout << "Manual cleanup in catch" << endl;
    }
}

void withRAII() {
    cout << "\n=== With RAII ===" << endl;

    try {
        Buffer buf(1000);  // RAII object
        buf.process();     // Throws exception

        // ✅ Destructor called automatically even with exception!
    } catch (...) {
        cout << "Exception caught, buffer already cleaned up!" << endl;
    }
}

int main() {
    withoutRAII();
    withRAII();

    cout << "\n✅ RAII guarantees cleanup!" << endl;

    return 0;
}
```

### 3.2 nullptr Best Practice

**Why nullptr is Better:**

```cpp
// nullptr_best_practice.cpp
#include <iostream>
using namespace std;

void func(int x) {
    cout << "Called func(int): " << x << endl;
}

void func(int* ptr) {
    cout << "Called func(int*): " << ptr << endl;
}

int main() {
    cout << "=== NULL vs nullptr ===" << endl;

    // WHY: NULL is typically #define NULL 0
    // This causes ambiguity!
    // func(NULL);  // ❌ Ambiguous! Calls func(int) - WRONG!

    // WHY: nullptr is type-safe
    func(nullptr);  // ✅ Correctly calls func(int*)

    cout << "\n=== Best Practices ===" << endl;

    // ✅ 1. Always initialize to nullptr
    int* ptr1 = nullptr;

    // ✅ 2. Check before use
    if (ptr1 == nullptr) {
        cout << "Pointer is null - safe" << endl;
    }

    // ✅ 3. Safe to delete
    delete ptr1;  // OK (does nothing)

    // ✅ 4. Assign after use
    ptr1 = new int(42);
    delete ptr1;
    ptr1 = nullptr;  // Always reset

    // ✅ 5. Consistent checking
    if (!ptr1) {  // Same as (ptr1 == nullptr)
        cout << "Pointer still null" << endl;
    }

    return 0;
}
```

### 3.3 Smart Pointers Introduction

**Purpose**: Automatic memory management via RAII.

**Three Types** (C++11):

1. `unique_ptr` - Exclusive ownership
2. `shared_ptr` - Shared ownership (reference counting)
3. `weak_ptr` - Non-owning observer

**Quick Preview:**

```cpp
// smart_pointers_preview.cpp
#include <iostream>
#include <memory>  // For smart pointers
using namespace std;

class MyClass {
public:
    int data;

    MyClass(int d) : data(d) {
        cout << "MyClass(" << data << ") constructed" << endl;
    }

    ~MyClass() {
        cout << "MyClass(" << data << ") destroyed" << endl;
    }
};

int main() {
    cout << "=== Raw Pointer (manual) ===" << endl;
    {
        MyClass* raw = new MyClass(1);
        cout << "Data: " << raw->data << endl;
        delete raw;  // Must remember!
    }

    cout << "\n=== unique_ptr (automatic) ===" << endl;
    {
        unique_ptr<MyClass> smart(new MyClass(2));
        // Or better (C++14): auto smart = make_unique<MyClass>(2);

        cout << "Data: " << smart->data << endl;

        // ✅ No delete needed!
    }  // ← Automatic cleanup here!

    cout << "\n=== shared_ptr (reference counting) ===" << endl;
    {
        shared_ptr<MyClass> shared1(new MyClass(3));
        // Or: auto shared1 = make_shared<MyClass>(3);

        {
            shared_ptr<MyClass> shared2 = shared1;
            cout << "Ref count: " << shared1.use_count() << endl;  // 2
        }  // shared2 destroyed

        cout << "Ref count: " << shared1.use_count() << endl;  // 1
    }  // Last reference gone, object destroyed

    cout << "\n✅ No memory leaks with smart pointers!" << endl;

    return 0;
}
```

---

## 4. Advanced Memory Concepts

### 4.1 Placement new

**Purpose**: Construct object at pre-allocated memory address.

**Use Cases:**

- Custom memory pools
- Embedded systems (specific hardware addresses)
- High-performance computing
- Memory-mapped I/O

```cpp
// placement_new_complete.cpp
#include <iostream>
#include <new>  // For placement new
using namespace std;

class Point {
public:
    int x, y;

    Point(int xVal, int yVal) : x(xVal), y(yVal) {
        cout << "Point(" << x << ", " << y << ") constructed" << endl;
    }

    ~Point() {
        cout << "Point(" << x << ", " << y << ") destroyed" << endl;
    }
};

int main() {
    cout << "=== Regular new ===" << endl;
    {
        Point* p1 = new Point(1, 2);
        delete p1;
    }

    cout << "\n=== Placement new ===" << endl;
    {
        // WHY: Pre-allocate raw memory
        void* memory = malloc(sizeof(Point));

        // WHY: Construct in pre-allocated memory
        Point* p2 = new(memory) Point(3, 4);

        // WHY: Must manually call destructor
        p2->~Point();

        // WHY: Free raw memory
        free(memory);
    }

    cout << "\n=== Memory Pool Example ===" << endl;
    {
        // Pre-allocate pool
        char pool[sizeof(Point) * 3];

        // Construct objects in pool
        Point* points[3];
        for (int i = 0; i < 3; i++) {
            points[i] = new(&pool[i * sizeof(Point)]) Point(i, i * 10);
        }

        // Destroy objects
        for (int i = 0; i < 3; i++) {
            points[i]->~Point();
        }
    }

    return 0;
}
```

### 4.2 Memory Debugging Tools

**1. Valgrind (Linux)**

```bash
# Compile with debug symbols
g++ -g program.cpp -o program

# Run with Valgrind
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         ./program
```

**2. AddressSanitizer (GCC/Clang)**

```bash
# Compile with ASan
g++ -fsanitize=address -g program.cpp -o program

# Run normally
./program
```

**3. Custom Memory Tracker:**

```cpp
// custom_tracker.cpp
#include <iostream>
#include <cstdlib>
using namespace std;

static long long totalAllocated = 0;
static int allocationCount = 0;
static int deallocationCount = 0;

void* operator new(size_t size) {
    totalAllocated += size;
    allocationCount++;

    #ifdef DEBUG_MEMORY
    cout << "[NEW] " << size << " bytes" << endl;
    #endif

    return malloc(size);
}

void operator delete(void* ptr) noexcept {
    deallocationCount++;

    #ifdef DEBUG_MEMORY
    cout << "[DELETE]" << endl;
    #endif

    free(ptr);
}

void printMemoryStats() {
    cout << "\n=== Memory Statistics ===" << endl;
    cout << "Allocated: " << totalAllocated << " bytes" << endl;
    cout << "Allocations: " << allocationCount << endl;
    cout << "Deallocations: " << deallocationCount << endl;

    if (allocationCount != deallocationCount) {
        cout << "⚠️  Possible leak: "
             << (allocationCount - deallocationCount)
             << " allocation(s) not freed" << endl;
    } else {
        cout << "✅ All allocations freed" << endl;
    }
}

int main() {
    int* ptr1 = new int(10);
    int* ptr2 = new int[100];

    delete ptr1;
    delete[] ptr2;

    // Intentional leak
    int* leak = new int(999);

    printMemoryStats();

    return 0;
}
```

---

## Summary

### Key Takeaways

1. **C-Style Allocation** - malloc/calloc/realloc/free allocate raw memory without constructors/destructors. malloc is uninitialized, calloc zeros memory. Use only for C interop or when needing realloc. Always cast void* return and check for nullptr.
2. **new vs malloc** - 10 critical differences: new is C++ operator calling constructors, malloc is C function allocating raw bytes. new returns typed pointer, malloc returns void*. new throws bad_alloc, malloc returns nullptr. Never mix - new pairs with delete, malloc pairs with free.
3. **Memory Leaks** - Allocated memory never freed, gradually exhausting RAM. Causes: forgot delete, exception before delete, pointer reassignment, loops without cleanup. Detect with Valgrind, AddressSanitizer, or custom tracking. Prevent with RAII pattern and smart pointers.
4. **Dangling Pointers** - Pointers to freed/invalid memory causing undefined behavior. Causes: use after delete, returning local addresses, pointers to temporaries. Always set to nullptr after delete. Check for nullptr before dereferencing. Use smart pointers to eliminate.
5. **Double Deletion** - Deleting same memory twice corrupts heap. Usually immediate crash. Solution: set to nullptr after delete (deleting nullptr is safe). Use safe deletion macros or smart pointers for automatic prevention.
6. **Wild Pointers** - Uninitialized pointers with random addresses, most dangerous. Always initialize: to nullptr, with new, or to valid address. Never leave pointers uninitialized. Modern compilers may warn but can't prevent.
7. **RAII Pattern** - Tie resource lifetime to object lifetime: acquire in constructor, release in destructor. Provides automatic cleanup, exception safety, prevents leaks. Foundation of modern C++. Examples: unique_ptr, vector, ifstream, lock_guard.
8. **nullptr** - Type-safe null pointer (C++11). Better than NULL (which is 0). Always use nullptr for: initialization, assignment after delete, function parameters, comparisons. Prevents ambiguity in function overloading.
9. **Smart Pointers** - Automatic memory management via RAII. unique_ptr for exclusive ownership (zero overhead), shared_ptr for shared ownership (reference counting), weak_ptr for observation. Eliminates manual memory management errors. Always prefer over raw pointers in modern C++.
10. **Debugging Tools** - Valgrind (Linux, comprehensive leak detection), AddressSanitizer (cross-platform, zero false positives), Visual Studio Memory Profiler (Windows, GUI), custom tracking (override new/delete). Use during development to catch errors early.

### Interview Essential Questions

**Q1: Explain the difference between `new`/`delete` and `malloc()`/`free()`. Why should you never mix them?**

A: `new`/`delete` are C++ operators that allocate memory AND call constructors/destructors, provide type safety, throw exceptions on failure, return typed pointers. `malloc()`/`free()` are C functions that only allocate raw bytes without constructors/destructors, return void* requiring casts, return NULL on failure.

Mixing causes catastrophic failures: `free()` on `new`-allocated memory skips destructors, leaking resources (file handles, nested allocations). `delete` on `malloc()`-allocated memory attempts calling non-existent destructor, causing undefined behavior. Using `delete[]` with `new` or vice versa corrupts memory. For objects with resources, mixing guarantees resource leaks. Always pair: new↔delete, new[]↔delete[], malloc↔free.

**Q2: What is a memory leak? How do you detect and prevent them?**

A: Memory leak occurs when dynamically allocated memory is never freed, permanently reducing available RAM. Causes gradual performance degradation, eventual allocation failures, system crashes. Critical in long-running servers, embedded systems.

Detection: Valgrind (`valgrind --leak-check=full`), AddressSanitizer (`g++ -fsanitize=address`), Visual Studio Memory Profiler, custom tracking (override new/delete). Tools show leak locations, allocation sizes, call stacks.

Prevention: (1) Always pair every new with delete, (2) Use RAII pattern - tie resource lifetime to object, (3) Prefer smart pointers (unique_ptr, shared_ptr) for automatic cleanup, (4) Delete in catch blocks if using manual management, (5) Use modern containers (vector, string) that manage memory, (6) Avoid raw pointers in modern C++, (7) Enable compiler warnings (-Wall -Wextra).

**Q3: Explain the RAII pattern and why it's important.**

A: RAII (Resource Acquisition Is Initialization) ties resource lifetime to object lifetime. Resources acquired in constructor, released in destructor. When object goes out of scope, destructor automatically called.

Benefits: (1) Automatic cleanup - no manual delete needed, (2) Exception-safe - destructor called during stack unwinding even if exception thrown, (3) Prevents memory leaks - impossible to forget cleanup, (4) Eliminates need to remember cleanup in every code path, (5) Makes code maintainable - clear ownership semantics.

Examples in standard library: unique_ptr/shared_ptr (memory), vector/string (dynamic arrays), ifstream/ofstream (file handles), lock_guard (mutexes). RAII is foundation of modern C++ resource management, eliminates entire categories of bugs. Best practice: always use RAII over manual resource management.

**Q4: What is a dangling pointer and how do you avoid it?**

A: Dangling pointer points to memory that's been deallocated or is otherwise invalid. Reading returns garbage or crashes, writing corrupts memory, causes undefined behavior extremely difficult to debug.

Causes: (1) Using pointer after delete, (2) Returning address of local variable (destroyed when function returns), (3) Pointer to object that went out of scope, (4) Double deletion.

Prevention: (1) Always set pointers to nullptr after delete, (2) Check for nullptr before dereferencing, (3) Never return pointers to local variables - return heap-allocated or static, (4) Use references instead of pointers when possible, (5) Prefer smart pointers (unique_ptr, shared_ptr) that automatically manage lifetime. Pattern: `delete ptr; ptr = nullptr;` makes subsequent deletions safe and nullptr checks effective. With smart pointers, dangling pointers are nearly impossible.

**Q5: Compare `new(nothrow)` with regular `new`. When should you use each?**

A: Regular `new` throws bad_alloc exception on failure, requires try-catch handling, automatic stack unwinding, modern C++ style, small exception overhead. `new(nothrow)` returns nullptr on failure, requires manual if-checking, no exception overhead, C-style, explicit control flow.

Use regular new when: (1) General application code, (2) Exceptions fit error handling strategy, (3) Calling from exception-using code, (4) Want automatic cleanup via stack unwinding. Use new(nothrow) when: (1) Performance-critical code where exception overhead matters, (2) Embedded systems without exception support, (3) Interfacing with C code, (4) Want simple if-null checking, (5) Writing allocation wrappers.

Example comparison: `int* p1 = new(nothrow) int[size]; if (!p1) return nullptr;` vs `try { int* p2 = new int[size]; } catch (bad_alloc&) { return nullptr; }`. Modern C++ generally prefers regular new with exceptions for consistency, but nothrow valid in specific scenarios. Best practice: use smart pointers to avoid manual allocation entirely.

---