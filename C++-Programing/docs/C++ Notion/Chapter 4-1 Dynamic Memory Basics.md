# 4.1. Dynamic Memory Basics

---

## Table of Contents

1. Understanding Memory Organization
2. Dynamic Memory Allocation with new/delete
3. Memory Allocation Failure Handling
4. Summary

---

## 1. Understanding Memory Organization

### 1.1 Program Memory Segments

**Purpose**: Understanding where different types of data are stored helps optimize memory usage and prevent errors.

**Why This Matters**: Knowing memory layout is crucial for debugging, optimization, and avoiding errors like stack overflow or heap fragmentation.

**Memory Layout** (from low to high addresses):

```
┌─────────────────────┐ High Address
│       Stack         │ ← Local variables, function calls (grows down)
├─────────────────────┤
│         ↓           │
│                     │
│         ↑           │
├─────────────────────┤
│       Heap          │ ← Dynamic memory (grows up)
├─────────────────────┤
│  BSS (Uninitialized │ ← Uninitialized global/static variables
│   Global/Static)    │
├─────────────────────┤
│  Data (Initialized  │ ← Initialized global/static variables
│   Global/Static)    │
├─────────────────────┤
│   Text (Code)       │ ← Program instructions (read-only)
└─────────────────────┘ Low Address
```

**Memory Segments Explained:**

1. **Text Segment (Code)**:
    - **What**: Contains compiled program instructions
    - **Why**: Read-only to prevent accidental or malicious code modification
    - **When**: Loaded once at program start
    - **Size**: Fixed, determined at compile time
2. **Data Segment**:
    - **What**: Initialized global and static variables
    - **Why**: Exists for entire program lifetime
    - **When**: Allocated at program start
    - **Example**: `int globalVar = 100;`
3. **BSS (Block Started by Symbol)**:
    - **What**: Uninitialized global and static variables
    - **Why**: Automatically initialized to zero, saves space in executable
    - **When**: Allocated at program start
    - **Example**: `int globalVar;` (implicitly 0)
4. **Heap**:
    - **What**: Dynamic memory allocation area
    - **Why**: For runtime-determined sizes, data that outlives functions
    - **When**: Grows during program execution as needed
    - **How**: Managed via new/delete or malloc/free
    - **Direction**: Grows upward (toward higher addresses)
5. **Stack**:
    - **What**: Local variables and function call information
    - **Why**: Automatic management (LIFO), very fast
    - **When**: Grows/shrinks with function calls
    - **How**: Automatically managed by compiler
    - **Direction**: Grows downward (toward lower addresses)

**Code Example:**

```cpp
// memory_segments.cpp
#include <iostream>
using namespace std;

// WHY: Global variables stored in Data segment (entire program lifetime)
int globalInitialized = 100;        // Data segment
int globalUninitialized;            // BSS segment (auto-initialized to 0)
static int staticGlobal = 200;      // Data segment

void demonstrateSegments() {
    // WHY: Local variables stored on Stack (automatic cleanup)
    int localVar = 10;              // Stack

    // WHY: Static local persists but still in Data segment
    static int staticLocal = 20;    // Data segment

    // WHY: Dynamically allocated memory goes on Heap (manual management)
    int* heapVar = new int(30);     // Pointer on stack, value on heap

    cout << "=== Memory Addresses (Hexadecimal) ===" << endl;
    cout << "Global initialized (Data): " << &globalInitialized << endl;
    cout << "Static global (Data):      " << &staticGlobal << endl;
    cout << "Local variable (Stack):    " << &localVar << endl;
    cout << "Heap pointer (Stack):      " << &heapVar << endl;
    cout << "Heap value (Heap):         " << heapVar << endl;

    // WHY: Must manually free heap memory
    delete heapVar;
}

int main() {
    cout << "Program code (Text segment): " << (void*)main << endl;
    cout << endl;
    demonstrateSegments();
    return 0;
}
```

**Output Example:**

```
Program code (Text segment): 0x400000

=== Memory Addresses (Hexadecimal) ===
Global initialized (Data): 0x601040
Static global (Data):      0x601044
Local variable (Stack):    0x7fff5fbff8cc
Heap pointer (Stack):      0x7fff5fbff8c0
Heap value (Heap):         0x1a3b010
```

**Key Observations:**

- Stack addresses (0x7fff...) are much higher than heap (0x1a3...)
- Stack and heap grow toward each other (collision = stack overflow)
- Code/Data segments have lowest addresses

### 1.2 Stack vs Heap - Complete Comparison

**The Fundamental Question**: When should I use stack vs heap?

**Comparison Table:**

| Aspect | Stack | Heap |
| --- | --- | --- |
| **Allocation** | Automatic (compiler-managed) | Manual (`new`/`malloc`) |
| **Deallocation** | Automatic (scope ends) | Manual (`delete`/`free`) |
| **Speed** | Very fast (pointer increment) | Slower (searching for free blocks) |
| **Size** | Limited (~1-8 MB typical) | Large (limited by available RAM) |
| **Lifetime** | Function/block scope only | Until explicitly freed |
| **Growth Direction** | Downward (high→low addresses) | Upward (low→high addresses) |
| **Fragmentation** | Never fragments | Can fragment over time |
| **Access Pattern** | LIFO (Last In, First Out) | Random access |
| **Thread Safety** | Each thread has own stack | Shared (needs synchronization) |
| **Error Detection** | Stack overflow (immediate crash) | Memory leaks (gradual) |
| **Debugging** | Easier (automatic cleanup) | Harder (manual tracking) |

**When to Use Stack:**

✅ **Use Stack when:**

- Size known at compile time
- Small data (few KB or less)
- Short lifetime (within function)
- Fast allocation/deallocation critical
- Thread-local data needed
- Simple data types or small objects

**Example Stack Usage:**

```cpp
void processData() {
    // WHY: Stack - fixed size, short lifetime, automatic cleanup
    int numbers[100];
    double temp;
    string localName;

    // All automatically destroyed when function returns
}
```

**When to Use Heap:**

✅ **Use Heap when:**

- Size unknown until runtime
- Large data structures (MB+)
- Need to persist beyond function scope
- Passing data between threads
- Variable-length arrays
- Recursive data structures (trees, linked lists)

**Example Heap Usage:**

```cpp
int* createArray(int size) {
    // WHY: Heap - runtime size, needs to persist after function returns
    int* arr = new int[size];
    return arr;  // Can return heap memory
}

int main() {
    int n;
    cin >> n;

    int* data = createArray(n);  // Data persists
    // Use data...
    delete[] data;  // Manual cleanup required
}
```

**Performance Comparison:**

```cpp
// performance_comparison.cpp
#include <iostream>
#include <chrono>
using namespace std;
using namespace std::chrono;

const int ITERATIONS = 1000000;

void stackAllocation() {
    // WHY: Stack allocation is just moving stack pointer (single instruction)
    auto start = high_resolution_clock::now();

    for (int i = 0; i < ITERATIONS; i++) {
        int stackArray[100];  // Very fast
        stackArray[0] = i;
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    cout << "Stack allocation: " << duration.count() << " μs" << endl;
}

void heapAllocation() {
    // WHY: Heap allocation involves:
    // 1. Searching free list for suitable block
    // 2. Splitting/coalescing blocks
    // 3. Updating metadata
    // 4. Managing fragmentation
    auto start = high_resolution_clock::now();

    for (int i = 0; i < ITERATIONS; i++) {
        int* heapArray = new int[100];  // Much slower
        heapArray[0] = i;
        delete[] heapArray;
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    cout << "Heap allocation: " << duration.count() << " μs" << endl;
}

int main() {
    cout << "Performance Test (" << ITERATIONS << " iterations):" << endl;
    cout << "Array size: 100 integers (400 bytes)" << endl;
    cout << endl;

    stackAllocation();
    heapAllocation();

    return 0;
}
```

**Typical Output:**

```
Performance Test (1000000 iterations):
Array size: 100 integers (400 bytes)

Stack allocation: 15 μs
Heap allocation: 32000 μs
```

**Key Insight**: Stack is approximately **2000x faster** than heap for allocation!

**Why Such Huge Difference?**

- **Stack**: Single pointer increment (1 CPU instruction)
- **Heap**: Complex allocation algorithm (hundreds of instructions)

**Real-World Application Example:**

```cpp
// real_world_example.cpp
#include <iostream>
#include <vector>
#include <string>
using namespace std;

class ImageProcessor {
private:
    // WHY: Small metadata on stack (fast access)
    int width, height;
    string format;

    // WHY: Large pixel data on heap (runtime size, persists)
    unsigned char* pixelData;

public:
    ImageProcessor(int w, int h) : width(w), height(h) {
        // WHY: Allocate large data on heap
        int dataSize = width * height * 3;  // RGB
        pixelData = new unsigned char[dataSize];
        cout << "Allocated " << dataSize << " bytes on heap" << endl;
    }

    ~ImageProcessor() {
        // WHY: Manual cleanup for heap data
        delete[] pixelData;
        cout << "Freed heap memory" << endl;
    }

    void process() {
        // WHY: Temporary data on stack (automatic, fast)
        int tempBuffer[256];
        double processingTime;

        // Process image...

        // Stack data automatically cleaned up
    }
};

int main() {
    // WHY: Small objects can go on stack
    ImageProcessor img(1920, 1080);  // Object on stack, pixel data on heap

    img.process();

    // WHY: Destructor automatically called, frees heap memory
    return 0;
}
```

---

## 2. Dynamic Memory Allocation with new/delete

### 2.1 The new Operator - Deep Understanding

**Purpose**: Allocate memory at runtime on the heap.

**What new Does** (3 steps):

1. **Allocates** memory from free store (heap)
2. **Initializes** memory (calls constructor for objects)
3. **Returns** typed pointer to allocated memory

**Why new Exists:**

- Stack size is limited and fixed
- Need runtime-determined sizes
- Data must outlive function scope
- Large data structures need heap

**Basic Syntax:**

```cpp
// Single object allocation
Type* ptr = new Type;           // Default initialization
Type* ptr = new Type(args);     // Constructor with arguments
Type* ptr = new Type{args};     // Uniform initialization (C++11)

// Array allocation
Type* arr = new Type[size];     // Default initialization
Type* arr = new Type[size]{};   // Value initialization (zeros)
Type* arr = new Type[size]{v1, v2, ...};  // List initialization
```

**Complete Example with Theory:**

```cpp
// new_operator_explained.cpp
#include <iostream>
#include <string>
using namespace std;

class Student {
public:
    string name;
    int age;

    // WHY: Default constructor for no-argument initialization
    Student() : name("Unknown"), age(0) {
        cout << "Default constructor: " << name << endl;
    }

    // WHY: Parameterized constructor for specific initialization
    Student(string n, int a) : name(n), age(a) {
        cout << "Parameterized constructor: " << name << endl;
    }

    ~Student() {
        cout << "Destructor: " << name << endl;
    }
};

int main() {
    cout << "=== Single Object Allocation ===" << endl;

    // WHY: new allocates memory AND calls constructor
    int* ptr1 = new int;           // Uninitialized (garbage value)
    int* ptr2 = new int(42);       // Initialized to 42
    int* ptr3 = new int{99};       // C++11 uniform initialization

    cout << "*ptr1 (uninitialized): " << *ptr1 << endl;
    cout << "*ptr2 (initialized): " << *ptr2 << endl;
    cout << "*ptr3 (C++11): " << *ptr3 << endl;

    delete ptr1;
    delete ptr2;
    delete ptr3;

    cout << "\n=== Object with Constructor ===" << endl;

    // WHY: new automatically calls appropriate constructor
    Student* s1 = new Student();              // Default constructor
    Student* s2 = new Student("Alice", 20);   // Parameterized constructor

    cout << s1->name << ", age " << s1->age << endl;
    cout << s2->name << ", age " << s2->age << endl;

    // WHY: delete calls destructor automatically
    delete s1;
    delete s2;

    cout << "\n=== Value Initialization vs Default ===" << endl;

    // WHY: () performs value initialization (zeros for primitives)
    int* zeros = new int[5]();

    // WHY: No () leaves primitives uninitialized (garbage)
    int* garbage = new int[5];

    cout << "Value-initialized: ";
    for (int i = 0; i < 5; i++) cout << zeros[i] << " ";
    cout << endl;

    cout << "Uninitialized: ";
    for (int i = 0; i < 5; i++) cout << garbage[i] << " ";
    cout << endl;

    delete[] zeros;
    delete[] garbage;

    return 0;
}
```

**Output:**

```
=== Single Object Allocation ===
*ptr1 (uninitialized): -1234567890
*ptr2 (initialized): 42
*ptr3 (C++11): 99

=== Object with Constructor ===
Default constructor: Unknown
Parameterized constructor: Alice
Unknown, age 0
Alice, age 20
Destructor: Unknown
Destructor: Alice

=== Value Initialization vs Default ===
Value-initialized: 0 0 0 0 0
Uninitialized: -123 456 789 -999 0
```

### 2.2 The delete Operator - Complete Understanding

**Purpose**: Deallocate memory and call destructor.

**What delete Does** (2 steps):

1. **Calls destructor** (for objects only)
2. **Returns memory** to free store

**Why delete is Manual:**

- Compiler can't know when you're done with heap memory
- Flexibility: you control lifetime
- Responsibility: you must remember to delete

**Critical Rules:**

```cpp
// delete_rules.cpp
#include <iostream>
using namespace std;

int main() {
    // RULE 1: Every new needs matching delete
    int* ptr1 = new int(42);
    delete ptr1;  // ✅ Correct pairing

    // RULE 2: Set to nullptr after delete (best practice)
    ptr1 = nullptr;  // ✅ Prevents accidental reuse

    // RULE 3: Deleting nullptr is safe (does nothing)
    delete ptr1;  // ✅ Safe, no operation performed

    // RULE 4: Never delete stack variables
    int stackVar = 10;
    int* ptr2 = &stackVar;
    // delete ptr2;  // ❌ CRASH! Stack memory can't be deleted

    // RULE 5: Never delete same memory twice
    int* ptr3 = new int(99);
    delete ptr3;
    // delete ptr3;  // ❌ DOUBLE DELETION! Undefined behavior

    // WHY: Setting to nullptr prevents double deletion
    ptr3 = nullptr;
    delete ptr3;     // ✅ Now safe (does nothing)

    // RULE 6: Match new[] with delete[], new with delete
    int* single = new int(5);
    int* array = new int[10];

    delete single;    // ✅ Correct
    delete[] array;   // ✅ Correct

    // delete[] single;  // ❌ WRONG! Use delete, not delete[]
    // delete array;     // ❌ WRONG! Use delete[], not delete

    return 0;
}
```

**Common Mistakes and Solutions:**

```cpp
// delete_mistakes.cpp
#include <iostream>
using namespace std;

class Resource {
public:
    int* data;

    Resource() {
        data = new int[100];
        cout << "Resource allocated" << endl;
    }

    ~Resource() {
        delete[] data;
        cout << "Resource freed" << endl;
    }
};

int main() {
    // ❌ MISTAKE 1: Memory leak (forgot to delete)
    {
        int* leak = new int(100);
        cout << "Allocated but forgot to delete!" << endl;
        // Forgot delete! Memory leaked when 'leak' goes out of scope
    }
    cout << "Block ended, leak persists\n" << endl;

    // ❌ MISTAKE 2: Dangling pointer (using after delete)
    {
        int* ptr = new int(42);
        cout << "Before delete: " << *ptr << endl;

        delete ptr;
        cout << "After delete..." << endl;

        // cout << *ptr << endl;  // ❌ DANGLING! Undefined behavior

        // ✅ SOLUTION: Set to nullptr after delete
        ptr = nullptr;
        if (ptr != nullptr) {
            cout << *ptr << endl;
        } else {
            cout << "Pointer is null (safe)" << endl;
        }
    }
    cout << endl;

    // ❌ MISTAKE 3: Deleting pointer to stack
    {
        int x = 50;
        int* ptr = &x;
        // delete ptr;  // ❌ CRASH! x is on stack, can't delete
    }

    // ❌ MISTAKE 4: Mismatched new[]/delete
    {
        int* arr = new int[10];
        // delete arr;    // ❌ WRONG! Should be delete[]
        delete[] arr;     // ✅ CORRECT
    }

    // ✅ CORRECT: RAII pattern (automatic cleanup)
    {
        Resource res;  // Constructor allocates
        // Use resource...
        // Destructor automatically frees when res goes out of scope
    }
    cout << "RAII object destroyed automatically" << endl;

    return 0;
}
```

### 2.3 Dynamic Arrays (new[], delete[])

**Purpose**: Allocate arrays of unknown size at runtime.

**Why Array Syntax Exists:**

- `delete[]` calls destructor for EACH element
- `delete` only calls destructor for FIRST element
- Critical for objects with resources

**Syntax and Rules:**

```cpp
Type* arr = new Type[size];        // Allocate array
// Use array...
delete[] arr;                       // Deallocate array (MUST use [])
arr = nullptr;                      // Best practice
```

**Complete Example:**

```cpp
// dynamic_arrays_complete.cpp
#include <iostream>
using namespace std;

class Element {
public:
    int id;

    Element() : id(0) {
        cout << "Element " << id << " constructed" << endl;
    }

    Element(int i) : id(i) {
        cout << "Element " << id << " constructed" << endl;
    }

    ~Element() {
        cout << "Element " << id << " destroyed" << endl;
    }
};

int main() {
    cout << "=== Runtime-Sized Array ===" << endl;

    int size;
    cout << "Enter array size: ";
    cin >> size;

    // WHY: Size unknown at compile time, must use dynamic allocation
    int* arr = new int[size];

    // Initialize
    for (int i = 0; i < size; i++) {
        arr[i] = i * 10;
    }

    // Display
    cout << "Array: ";
    for (int i = 0; i < size; i++) {
        cout << arr[i] << " ";
    }
    cout << endl;

    // WHY: Must use delete[] for arrays
    delete[] arr;
    arr = nullptr;

    cout << "\n=== Why delete[] Matters ===" << endl;

    // WHY: Each object needs destructor called
    Element* elements = new Element[3];
    elements[0] = Element(1);
    elements[1] = Element(2);
    elements[2] = Element(3);

    cout << "\nDeleting with delete[] (correct):" << endl;
    delete[] elements;  // ✅ Calls destructor for all 3 elements

    // If we used delete instead:
    // delete elements;  // ❌ WRONG! Only destroys first element

    return 0;
}
```

**Array Initialization Methods:**

```cpp
// array_initialization.cpp
#include <iostream>
using namespace std;

int main() {
    // Method 1: Default initialization (garbage values for primitives)
    int* arr1 = new int[5];

    // Method 2: Value initialization (zeros) - C++11
    int* arr2 = new int[5]();  // All zeros

    // Method 3: Brace initialization (zeros) - C++11
    int* arr3 = new int[5]{};  // All zeros

    // Method 4: List initialization with values - C++11
    int* arr4 = new int[5]{1, 2, 3, 4, 5};

    // Method 5: Partial initialization (rest are zeros) - C++11
    int* arr5 = new int[5]{1, 2, 3};  // {1, 2, 3, 0, 0}

    cout << "arr1 (garbage): ";
    for (int i = 0; i < 5; i++) cout << arr1[i] << " ";
    cout << endl;

    cout << "arr2 (zeros with ()): ";
    for (int i = 0; i < 5; i++) cout << arr2[i] << " ";
    cout << endl;

    cout << "arr3 (zeros with {}): ";
    for (int i = 0; i < 5; i++) cout << arr3[i] << " ";
    cout << endl;

    cout << "arr4 (initialized): ";
    for (int i = 0; i < 5; i++) cout << arr4[i] << " ";
    cout << endl;

    cout << "arr5 (partial): ";
    for (int i = 0; i < 5; i++) cout << arr5[i] << " ";
    cout << endl;

    // Clean up
    delete[] arr1;
    delete[] arr2;
    delete[] arr3;
    delete[] arr4;
    delete[] arr5;

    return 0;
}
```

### 2.4 Dynamic 2D Arrays

**Purpose**: Allocate multi-dimensional arrays at runtime.

**Two Approaches:**

**Method 1: Array of Pointers (Traditional)**

```cpp
// 2d_array_method1.cpp
#include <iostream>
using namespace std;

int main() {
    int rows = 3, cols = 4;

    // WHY: Step 1 - Allocate array of row pointers
    int** matrix = new int*[rows];

    // WHY: Step 2 - Allocate each row separately
    for (int i = 0; i < rows; i++) {
        matrix[i] = new int[cols];
    }

    // Initialize
    int value = 1;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            matrix[i][j] = value++;
        }
    }

    // Display
    cout << "Matrix (Method 1: Array of Pointers):" << endl;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            cout << matrix[i][j] << "\t";
        }
        cout << endl;
    }

    // WHY: Deallocate in REVERSE order (rows first, then array)
    for (int i = 0; i < rows; i++) {
        delete[] matrix[i];  // Free each row
    }
    delete[] matrix;  // Free array of pointers

    return 0;
}
```

**Method 2: Single Contiguous Block (Better Performance)**

```cpp
// 2d_array_method2.cpp
#include <iostream>
using namespace std;

int main() {
    int rows = 3, cols = 4;

    // WHY: Single allocation = better cache locality, faster access
    int* matrix = new int[rows * cols];

    // Initialize
    int value = 1;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            // WHY: Manual 2D to 1D index conversion
            matrix[i * cols + j] = value++;
        }
    }

    // Display
    cout << "Matrix (Method 2: Contiguous Block):" << endl;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            cout << matrix[i * cols + j] << "\t";
        }
        cout << endl;
    }

    // WHY: Single deallocation (simpler, faster)
    delete[] matrix;

    return 0;
}
```

**Method Comparison:**

| Aspect | Array of Pointers | Contiguous Block |
| --- | --- | --- |
| **Allocations** | rows + 1 | 1 |
| **Deallocations** | rows + 1 | 1 |
| **Memory Layout** | Scattered rows | Sequential |
| **Cache Performance** | Worse | Better |
| **Access Syntax** | `matrix[i][j]` | `matrix[i*cols+j]` |
| **Flexibility** | Variable row sizes | Fixed row sizes |
| **Overhead** | Extra pointers | Minimal |
| **Recommended** | Rarely | Usually |

### 2.5 Dynamic Structures

**Purpose**: Allocate struct/class objects at runtime.

```cpp
// dynamic_structures.cpp
#include <iostream>
#include <string>
using namespace std;

struct Student {
    string name;
    int age;
    double gpa;

    void display() const {
        cout << "Name: " << name << ", Age: " << age
             << ", GPA: " << gpa << endl;
    }
};

int main() {
    cout << "=== Single Structure ===" << endl;

    // WHY: Single structure on heap
    Student* s1 = new Student{"Alice", 20, 3.8};

    // WHY: Use -> operator for pointer access
    s1->display();

    delete s1;

    cout << "\n=== Array of Structures ===" << endl;

    // WHY: Array of structures (contiguous memory)
    int count = 3;
    Student* students = new Student[count];

    // Initialize
    students[0] = {"Bob", 21, 3.5};
    students[1] = {"Charlie", 19, 3.9};
    students[2] = {"David", 22, 3.7};

    // Display
    for (int i = 0; i < count; i++) {
        cout << "Student " << (i + 1) << ": ";
        students[i].display();  // . operator (array element is object)
    }

    delete[] students;

    cout << "\n=== Array of Pointers to Structures ===" << endl;

    // WHY: More flexible - can point to different objects
    Student** studPtrs = new Student*[count];

    // WHY: Each pointer allocated separately
    for (int i = 0; i < count; i++) {
        studPtrs[i] = new Student;
        studPtrs[i]->name = "Student" + to_string(i + 1);
        studPtrs[i]->age = 18 + i;
        studPtrs[i]->gpa = 3.0 + i * 0.2;
    }

    // Display
    for (int i = 0; i < count; i++) {
        studPtrs[i]->display();
    }

    // WHY: Must delete in reverse order
    for (int i = 0; i < count; i++) {
        delete studPtrs[i];  // Free each object
    }
    delete[] studPtrs;  // Free pointer array

    return 0;
}
```

---

## 3. Memory Allocation Failure Handling

### 3.1 bad_alloc Exception

**Purpose**: Handle out-of-memory situations gracefully.

**When Does new Fail?**

- Insufficient available RAM
- System memory limit reached
- Memory fragmentation (no contiguous block large enough)
- Requesting impossibly large allocation

**Default Behavior**: `new` throws `bad_alloc` exception on failure.

**Why Exception Instead of nullptr?**

- Forces error handling (can't ignore)
- Automatic stack unwinding
- Consistent with modern C++ error handling

```cpp
// bad_alloc_exception.cpp
#include <iostream>
#include <new>  // For bad_alloc
using namespace std;

int main() {
    cout << "=== Handling Allocation Failure ===" << endl;

    try {
        // WHY: Attempt to allocate huge amount
        long long hugeSize = 1000000000000LL;  // 1 trillion integers

        cout << "Attempting to allocate "
             << hugeSize * sizeof(int) / (1024*1024*1024)
             << " GB of memory..." << endl;

        int* ptr = new int[hugeSize];  // Will throw bad_alloc

        // WHY: This line never executes if allocation fails
        cout << "Allocation successful!" << endl;
        delete[] ptr;

    } catch (const bad_alloc& e) {
        // WHY: Catch bad_alloc specifically
        cout << "\n❌ Allocation failed!" << endl;
        cout << "Exception: " << e.what() << endl;
        cout << "Program continues safely..." << endl;
    }

    cout << "\nProgram completed successfully." << endl;
    return 0;
}
```

**Output:**

```
=== Handling Allocation Failure ===
Attempting to allocate 3725 GB of memory...

❌ Allocation failed!
Exception: std::bad_alloc
Program continues safely...

Program completed successfully.
```

**Best Practice Pattern:**

```cpp
// exception_pattern.cpp
#include <iostream>
#include <new>
using namespace std;

// WHY: Wrapper function for safe allocation
void* safeAllocate(size_t size) {
    try {
        return new char[size];
    } catch (const bad_alloc& e) {
        cerr << "Memory allocation failed: " << e.what() << endl;
        return nullptr;
    }
}

int main() {
    size_t requestedSize = 1000000;  // 1 MB

    void* ptr = safeAllocate(requestedSize);

    if (ptr != nullptr) {
        cout << "Successfully allocated " << requestedSize << " bytes" << endl;

        // Use memory...

        delete[] static_cast<char*>(ptr);
    } else {
        cout << "Failed to allocate, using alternative strategy..." << endl;
        // Implement fallback (use smaller size, disk storage, etc.)
    }

    return 0;
}
```

### 3.2 nothrow Version of new

**Purpose**: Alternative to exception handling - returns nullptr on failure.

**Syntax:**

```cpp
#include <new>  // For nothrow

Type* ptr = new(nothrow) Type[size];
if (ptr == nullptr) {
    // Handle allocation failure
}

```

**Why nothrow Exists:**

- Avoid exception overhead in performance-critical code
- Simpler error handling (just check for nullptr)
- Compatible with C-style error checking
- Useful in embedded systems (exceptions may be disabled)

**Complete Example:**

```cpp
// nothrow_example.cpp
#include <iostream>
#include <new>  // For nothrow
using namespace std;

int main() {
    cout << "=== Standard new (throws exception) ===" << endl;

    try {
        long long huge = 1000000000000LL;
        int* ptr1 = new int[huge];  // Will throw
        delete[] ptr1;
    } catch (const bad_alloc& e) {
        cout << "Caught exception: " << e.what() << endl;
    }

    cout << "\n=== nothrow new (returns nullptr) ===" << endl;

    long long huge = 1000000000000LL;

    // WHY: nothrow version returns nullptr instead of throwing
    int* ptr2 = new(nothrow) int[huge];

    if (ptr2 == nullptr) {
        cout << "❌ Allocation failed - ptr2 is nullptr" << endl;
        cout << "No exception thrown, program continues normally" << endl;
    } else {
        cout << "✅ Allocation successful" << endl;
        delete[] ptr2;
    }

    cout << "\n=== Practical Usage Pattern ===" << endl;

    // WHY: Try allocating reasonable size with fallback
    int* arr = new(nothrow) int[1000000];  // 1M integers

    if (arr == nullptr) {
        cerr << "Failed to allocate 1M integers, trying smaller size..." << endl;

        // Fallback strategy
        arr = new(nothrow) int[100000];  // 100K integers

        if (arr == nullptr) {
            cerr << "Critical: Cannot allocate minimum required memory" << endl;
            return 1;  // Exit with error code
        }

        cout << "Allocated smaller array (100K integers)" << endl;
    } else {
        cout << "Allocated full array (1M integers)" << endl;
    }

    // Use array...
    for (int i = 0; i < 10; i++) {
        arr[i] = i * 10;
    }

    cout << "First 10 elements: ";
    for (int i = 0; i < 10; i++) {
        cout << arr[i] << " ";
    }
    cout << endl;

    delete[] arr;

    return 0;
}
```

### 3.3 Comparison: Exception vs nothrow

**When to Use Each:**

| Scenario | Use Exception (regular new) | Use nothrow |
| --- | --- | --- |
| **General code** | ✅ Preferred | ❌ |
| **Performance-critical** | ❌ | ✅ |
| **Embedded systems** | ❌ | ✅ |
| **C interop** | ❌ | ✅ |
| **Modern C++** | ✅ | ❌ |
| **Exceptions disabled** | ❌ | ✅ Mandatory |

**Comparison Code:**

```cpp
// comparison.cpp
#include <iostream>
#include <new>
using namespace std;

// Style 1: Exception handling (modern C++)
void modernStyle() {
    try {
        int* data = new int[1000];
        // Use data...
        delete[] data;
    } catch (const bad_alloc&) {
        cerr << "Allocation failed" << endl;
    }
}

// Style 2: nothrow checking (C-style)
void classicStyle() {
    int* data = new(nothrow) int[1000];

    if (!data) {
        cerr << "Allocation failed" << endl;
        return;
    }

    // Use data...
    delete[] data;
}

int main() {
    cout << "Both styles work correctly" << endl;
    modernStyle();
    classicStyle();
    return 0;
}
```

---

## Summary

### Key Takeaways

1. **Memory Segments** - Programs have 5 segments: Text (code), Data (initialized globals), BSS (uninitialized globals), Heap (dynamic), Stack (local vars). Understanding this prevents errors like stack overflow and guides optimization decisions.
2. **Stack vs Heap** - Stack: automatic, fast (~2000x), limited size (~1-8MB), LIFO. Heap: manual, slower, large (RAM-limited), random access. Use stack for small/known-size data, heap for large/runtime-determined data.
3. **new Operator** - Allocates memory on heap AND calls constructors. Syntax: `Type* p = new Type(args)` for objects, `Type* a = new Type[size]` for arrays. Returns typed pointer, throws bad_alloc on failure.
4. **delete Operator** - Calls destructors AND deallocates memory. Critical: pair `new` with `delete`, `new[]` with `delete[]`. Always set to nullptr after delete to prevent dangling pointers.
5. **Dynamic Arrays** - Use `new[]` for runtime-sized arrays. Must use `delete[]` (not `delete`) to call destructors for all elements. Initialize with `()` for zeros or `{}` for list initialization (C++11).
6. **2D Arrays** - Two methods: (1) Array of pointers (flexible but slower), (2) Contiguous block (faster, better cache). Contiguous is preferred for performance: `int* arr = new int[rows*cols]`.
7. **bad_alloc Exception** - Thrown when new fails (out of memory). Always handle in production code with try-catch. Allows graceful recovery or clean shutdown instead of crash.
8. **nothrow Alternative** - `new(nothrow)` returns nullptr instead of throwing. Use for: performance-critical code, embedded systems, C-style error checking. Syntax: `int* p = new(nothrow) int[size]; if (!p) {...}`.

---