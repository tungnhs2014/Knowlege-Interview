# 3.2. Pointers in C++

---

## Table of Contents

1. What are Pointers?
2. Pointer Declaration and Initialization
3. Address-of and Dereference Operators
4. Pointer Types and Sizes
5. Special Types of Pointers
6. Pointer Arithmetic
7. Pointers and const
8. Pointers and Arrays
9. Pointer to Pointer (Double Pointer)
10. Function Pointers
11. Pointers vs References
12. Smart Pointers Overview
13. Common Pitfalls and Best Practices

---

## 1. What are Pointers?

### 1.1 Definition

**A pointer is a variable that stores the memory address of another variable, rather than storing a direct value itself.** Pointers enable direct memory access and manipulation, making them fundamental to system-level programming and efficient data structure implementation.

**Core Concept:**

```cpp
#include <iostream>
using namespace std;

int main() {
    int value = 42;

    // WHY: Pointer stores ADDRESS, not value
    int* ptr = &value;  // ptr holds address of value

    cout << "Value: " << value << endl;
    cout << "Address of value: " << &value << endl;
    cout << "Pointer stores: " << ptr << endl;
    cout << "Value via pointer: " << *ptr << endl;

    return 0;
}
```

**Output:**

```
Value: 42
Address of value: 0x7ffd12345678
Pointer stores: 0x7ffd12345678
Value via pointer: 42
```

### 1.2 Why Pointers Exist

**Purpose:**

1. **Direct Memory Access**
    - Manipulate hardware at low level
    - Efficient data modification
2. **Dynamic Memory Management**
    - Allocate memory at runtime
    - Create flexible data structures
3. **Efficient Parameter Passing**
    - Pass large objects without copying
    - Modify function arguments directly
4. **Data Structure Implementation**
    - Linked lists, trees, graphs
    - Polymorphism via base class pointers

**Real-World Applications:**

- Operating system development
- Device drivers
- Game engines (performance critical)
- Embedded systems
- Database systems

### 1.3 How Pointers Work Internally

**Memory Model:**

```
Variable:  int value = 42;
Memory:    [Address: 0x1000] → [Value: 42]

Pointer:   int* ptr = &value;
Memory:    [Address: 0x2000] → [Value: 0x1000]  (stores address)

Dereference: *ptr
Action: Go to address 0x1000, read value → 42
```

**Visualization:**

```cpp
#include <iostream>
using namespace std;

int main() {
    int x = 100;
    int* ptr = &x;

    // WHY: Demonstrate memory relationships
    cout << "x value: " << x << endl;
    cout << "x address: " << &x << endl;
    cout << "ptr value (address it holds): " << ptr << endl;
    cout << "ptr address: " << &ptr << endl;
    cout << "*ptr (dereferenced): " << *ptr << endl;

    return 0;
}
```

---

## 2. Pointer Declaration and Initialization

### 2.1 Declaration Syntax

**Syntax:**

```cpp
data_type* pointer_name;
```

**Examples:**

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: Declare pointers for different types
    int* intPtr;        // Pointer to integer
    char* charPtr;      // Pointer to character
    double* dblPtr;     // Pointer to double
    float* floatPtr;    // Pointer to float

    // WHY: Pointer to pointer
    int** ptrPtr;       // Pointer to pointer to int

    return 0;
}
```

**Important:** `*` is part of type, but placement varies:

```cpp
int* ptr1;    // Preferred: * with type
int *ptr2;    // Also valid: * with name
int * ptr3;   // Valid but unusual
```

**Multiple declarations:**

```cpp
// ⚠️ CAUTION: Only first is pointer!
int* ptr1, var1;    // ptr1 is pointer, var1 is int

// ✅ CORRECT: Declare separately or with *
int* ptr1, *ptr2;   // Both are pointers
```

### 2.2 Pointer Initialization

**Method 1: Initialize with address**

```cpp
#include <iostream>
using namespace std;

int main() {
    int value = 10;

    // WHY: Initialize pointer with variable address
    int* ptr = &value;

    cout << "Pointer initialized to: " << ptr << endl;
    cout << "Points to value: " << *ptr << endl;

    return 0;
}
```

**Method 2: Initialize with nullptr (C++11)**

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: nullptr represents "points to nothing"
    int* ptr1 = nullptr;    // Modern C++11
    int* ptr2 = NULL;       // Old style (C legacy)
    int* ptr3 = 0;          // Old style (integer 0)

    // WHY: Check before dereferencing
    if (ptr1 != nullptr) {
        cout << *ptr1 << endl;  // Won't execute
    } else {
        cout << "Pointer is null" << endl;
    }

    return 0;
}
```

**Method 3: Uninitialized (dangerous!)**

```cpp
#include <iostream>
using namespace std;

int main() {
    // ❌ DANGER: Wild pointer with garbage address
    int* wildPtr;

    // ❌ UNDEFINED BEHAVIOR - DO NOT DO!
    // cout << *wildPtr << endl;

    // ✅ CORRECT: Always initialize!
    int* safePtr = nullptr;

    return 0;
}
```

---

## 3. Address-of and Dereference Operators

### 3.1 Address-of Operator (&)

**Purpose:** Get memory address of a variable

```cpp
#include <iostream>
using namespace std;

int main() {
    int age = 25;
    double salary = 50000.50;
    char grade = 'A';

    // WHY: & operator returns address
    cout << "Address of age: " << &age << endl;
    cout << "Address of salary: " << &salary << endl;
    cout << "Address of grade: " << (void*)&grade << endl;  // Cast for char

    return 0;
}
```

**Output (sample):**

```
Address of age: 0x7ffd12345678
Address of salary: 0x7ffd1234567C
Address of grade: 0x7ffd12345680
```

### 3.2 Dereference Operator (*)

**Purpose:** Access value at pointer's address

```cpp
#include <iostream>
using namespace std;

int main() {
    int score = 95;
    int* ptr = &score;

    // WHY: * dereferences pointer to get value
    cout << "score = " << score << endl;
    cout << "ptr = " << ptr << " (address)" << endl;
    cout << "*ptr = " << *ptr << " (value)" << endl;

    // WHY: Modify via pointer
    *ptr = 100;

    cout << "\nAfter *ptr = 100:" << endl;
    cout << "score = " << score << endl;
    cout << "*ptr = " << *ptr << endl;

    return 0;
}
```

**Output:**

```
score = 95
ptr = 0x7ffd12345678 (address)
*ptr = 95 (value)

After *ptr = 100:
score = 100
*ptr = 100
```

### 3.3 Pointer Reassignment

```cpp
#include <iostream>
using namespace std;

int main() {
    int a = 10, b = 20, c = 30;
    int* ptr = &a;

    cout << "*ptr = " << *ptr << endl;  // 10

    // WHY: Pointer can be reassigned to different address
    ptr = &b;
    cout << "*ptr = " << *ptr << endl;  // 20

    ptr = &c;
    cout << "*ptr = " << *ptr << endl;  // 30

    return 0;
}
```

**Output:**

```
*ptr = 10
*ptr = 20
*ptr = 30
```

---

## 4. Pointer Types and Sizes

### 4.1 Pointer Size

**Key Fact:** Pointer size depends on system architecture, NOT data type

```cpp
#include <iostream>
using namespace std;

int main() {
    int* intPtr;
    char* charPtr;
    double* dblPtr;
    long long* llPtr;

    // WHY: All pointers same size on same system
    cout << "sizeof(int*): " << sizeof(intPtr) << " bytes" << endl;
    cout << "sizeof(char*): " << sizeof(charPtr) << " bytes" << endl;
    cout << "sizeof(double*): " << sizeof(dblPtr) << " bytes" << endl;
    cout << "sizeof(long long*): " << sizeof(llPtr) << " bytes" << endl;

    return 0;
}
```

**Output (64-bit system):**

```
sizeof(int*): 8 bytes
sizeof(char*): 8 bytes
sizeof(double*): 8 bytes
sizeof(long long*): 8 bytes
```

**Output (32-bit system):**

```
sizeof(int*): 4 bytes
sizeof(char*): 4 bytes
sizeof(double*): 4 bytes
sizeof(long long*): 4 bytes
```

**Why?**

- 64-bit system: Addresses are 64 bits (8 bytes)
- 32-bit system: Addresses are 32 bits (4 bytes)
- Pointer stores address regardless of what it points to

### 4.2 Pointer Types Matter for Arithmetic

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[3] = {10, 20, 30};
    char str[3] = {'A', 'B', 'C'};

    int* intPtr = arr;
    char* charPtr = str;

    // WHY: Pointer type determines increment step
    cout << "intPtr: " << (void*)intPtr << endl;
    cout << "intPtr + 1: " << (void*)(intPtr + 1) << endl;  // +4 bytes
    cout << "intPtr + 2: " << (void*)(intPtr + 2) << endl;  // +8 bytes

    cout << "\ncharPtr: " << (void*)charPtr << endl;
    cout << "charPtr + 1: " << (void*)(charPtr + 1) << endl;  // +1 byte
    cout << "charPtr + 2: " << (void*)(charPtr + 2) << endl;  // +2 bytes

    return 0;
}
```

---

## 5. Special Types of Pointers

### 5.1 Wild Pointer

**Definition:** Uninitialized pointer containing garbage address

```cpp
#include <iostream>
using namespace std;

int main() {
    // ❌ WILD POINTER - Contains random address
    int* wildPtr;

    // ❌ UNDEFINED BEHAVIOR - May crash!
    // cout << *wildPtr << endl;

    // ✅ ALWAYS INITIALIZE
    int* safePtr = nullptr;

    return 0;
}
```

**Dangers:**

- Unpredictable behavior
- Segmentation faults
- Memory corruption
- Security vulnerabilities

### 5.2 NULL Pointer

**Definition:** Pointer that explicitly points to nothing

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: nullptr is modern C++11 standard
    int* ptr1 = nullptr;

    // WHY: NULL is C legacy (macro for 0)
    int* ptr2 = NULL;

    // WHY: 0 works but ambiguous (could be integer)
    int* ptr3 = 0;

    // WHY: Check before dereferencing
    if (ptr1 != nullptr) {
        cout << *ptr1 << endl;
    } else {
        cout << "Pointer is null, cannot dereference" << endl;
    }

    return 0;
}
```

**nullptr vs NULL vs 0:**

```cpp
#include <iostream>
using namespace std;

void func(int x) {
    cout << "func(int) called" << endl;
}

void func(int* ptr) {
    cout << "func(int*) called" << endl;
}

int main() {
    func(0);         // Ambiguous! Calls func(int)
    func(NULL);      // May be ambiguous
    func(nullptr);   // ✅ Clearly calls func(int*)

    return 0;
}
```

### 5.3 Void Pointer

**Definition:** Generic pointer that can point to any data type

```cpp
#include <iostream>
using namespace std;

int main() {
    int num = 42;
    double pi = 3.14159;
    char letter = 'X';

    // WHY: void* can hold any address
    void* voidPtr;

    voidPtr = &num;
    cout << "Points to int at: " << voidPtr << endl;

    voidPtr = &pi;
    cout << "Points to double at: " << voidPtr << endl;

    voidPtr = &letter;
    cout << "Points to char at: " << voidPtr << endl;

    // WHY: Must cast before dereferencing
    voidPtr = &num;
    // cout << *voidPtr << endl;  // ❌ ERROR: Cannot dereference void*
    cout << *(static_cast<int*>(voidPtr)) << endl;  // ✅ OK

    return 0;
}
```

**Use Cases:**

- Generic programming (e.g., `memcpy`)
- Callback functions
- Type-erased storage

### 5.4 Dangling Pointer

**Definition:** Pointer to memory that has been deallocated

**Scenario 1: Pointing to local variable**

```cpp
#include <iostream>
using namespace std;

int* getDanglingPointer() {
    int local = 100;

    // ❌ DANGER: Returning address of local variable
    return &local;  // local destroyed after function returns
}

int main() {
    int* ptr = getDanglingPointer();

    // ❌ UNDEFINED BEHAVIOR - ptr is dangling!
    // cout << *ptr << endl;

    return 0;
}
```

**Scenario 2: After delete**

```cpp
#include <iostream>
using namespace std;

int main() {
    int* ptr = new int(50);

    cout << *ptr << endl;  // OK: 50

    delete ptr;  // Memory freed

    // ❌ ptr now dangling - points to freed memory
    // cout << *ptr << endl;  // UNDEFINED BEHAVIOR!

    // ✅ BEST PRACTICE: Set to nullptr after delete
    ptr = nullptr;

    return 0;
}
```

**Prevention:**

- Set pointers to `nullptr` after `delete`
- Don't return addresses of local variables
- Use smart pointers (automatic cleanup)

---

## 6. Pointer Arithmetic

### 6.1 Basic Arithmetic Operations

**Operations Allowed:**

- Increment (++, +=)
- Decrement (--, -=)
- Addition (ptr + n)
- Subtraction (ptr - n)
- Difference (ptr1 - ptr2)
- Comparison (==, !=, <, >, <=, >=)

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[5] = {10, 20, 30, 40, 50};
    int* ptr = arr;

    // WHY: Pointer arithmetic moves by element size
    cout << "ptr: " << ptr << ", *ptr: " << *ptr << endl;

    ptr++;  // Move to next element
    cout << "ptr++: " << ptr << ", *ptr: " << *ptr << endl;

    ptr += 2;  // Move 2 elements forward
    cout << "ptr += 2: " << ptr << ", *ptr: " << *ptr << endl;

    ptr--;  // Move back one element
    cout << "ptr--: " << ptr << ", *ptr: " << *ptr << endl;

    return 0;
}
```

**Output:**

```
ptr: 0x7ffd1000, *ptr: 10
ptr++: 0x7ffd1004, *ptr: 20
ptr += 2: 0x7ffd100C, *ptr: 40
ptr--: 0x7ffd1008, *ptr: 30
```

### 6.2 Pointer Arithmetic with Different Types

```cpp
#include <iostream>
using namespace std;

int main() {
    int intArr[3];
    char charArr[3];
    double dblArr[3];

    int* intPtr = intArr;
    char* charPtr = charArr;
    double* dblPtr = dblArr;

    // WHY: Increment moves by sizeof(type) bytes
    cout << "intPtr: " << (void*)intPtr << endl;
    cout << "intPtr + 1: " << (void*)(intPtr + 1) << endl;  // +4

    cout << "\ncharPtr: " << (void*)charPtr << endl;
    cout << "charPtr + 1: " << (void*)(charPtr + 1) << endl;  // +1

    cout << "\ndblPtr: " << (void*)dblPtr << endl;
    cout << "dblPtr + 1: " << (void*)(dblPtr + 1) << endl;  // +8

    return 0;
}
```

### 6.3 Pointer Subtraction

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[5] = {10, 20, 30, 40, 50};
    int* ptr1 = &arr[1];  // Points to 20
    int* ptr2 = &arr[4];  // Points to 50

    // WHY: Difference gives number of elements between pointers
    ptrdiff_t diff = ptr2 - ptr1;

    cout << "ptr2 - ptr1 = " << diff << " elements" << endl;
    cout << "Distance in bytes: " << diff * sizeof(int) << endl;

    return 0;
}
```

**Output:**

```
ptr2 - ptr1 = 3 elements
Distance in bytes: 12
```

### 6.4 Pointer Comparison

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[5] = {10, 20, 30, 40, 50};
    int* start = arr;
    int* end = arr + 5;

    // WHY: Iterate using pointer comparison
    for (int* ptr = start; ptr < end; ptr++) {
        cout << *ptr << " ";
    }

    return 0;
}
```

**Output:**

```
10 20 30 40 50
```

---

## 7. Pointers and const

### 7.1 Three Variations

**Type 1: Pointer to const (data is const)**

```cpp
#include <iostream>
using namespace std;

int main() {
    int value = 10;
    int another = 20;

    // WHY: Cannot modify data through pointer
    const int* ptr = &value;

    cout << *ptr << endl;  // OK: Read

    // *ptr = 15;  // ❌ ERROR: Cannot modify data

    ptr = &another;  // ✅ OK: Can reassign pointer
    cout << *ptr << endl;

    return 0;
}
```

**Type 2: Const pointer (address is const)**

```cpp
#include <iostream>
using namespace std;

int main() {
    int value = 10;
    int another = 20;

    // WHY: Cannot reassign pointer
    int* const ptr = &value;

    *ptr = 15;  // ✅ OK: Can modify data
    cout << *ptr << endl;

    // ptr = &another;  // ❌ ERROR: Cannot reassign pointer

    return 0;
}
```

**Type 3: Const pointer to const (both const)**

```cpp
#include <iostream>
using namespace std;

int main() {
    int value = 10;
    int another = 20;

    // WHY: Neither data nor pointer can change
    const int* const ptr = &value;

    cout << *ptr << endl;  // ✅ OK: Read

    // *ptr = 15;        // ❌ ERROR: Cannot modify data
    // ptr = &another;   // ❌ ERROR: Cannot reassign pointer

    return 0;
}
```

### 7.2 Comparison Table

| Declaration | Modify Data? | Reassign Pointer? | Read "Right-to-Left" |
| --- | --- | --- | --- |
| `const int* ptr` | ❌ | ✅ | "ptr is a pointer to const int" |
| `int* const ptr` | ✅ | ❌ | "ptr is a const pointer to int" |
| `const int* const ptr` | ❌ | ❌ | "ptr is a const pointer to const int" |

### 7.3 Practical Example

```cpp
#include <iostream>
using namespace std;

// WHY: Prevent function from modifying data
void displayArray(const int* arr, int size) {
    for (int i = 0; i < size; i++) {
        cout << arr[i] << " ";
        // arr[i] = 0;  // ❌ ERROR: arr is const
    }
    cout << endl;
}

// WHY: Function can modify data
void doubleValues(int* arr, int size) {
    for (int i = 0; i < size; i++) {
        arr[i] *= 2;  // ✅ OK
    }
}

int main() {
    int numbers[] = {1, 2, 3, 4, 5};

    displayArray(numbers, 5);
    doubleValues(numbers, 5);
    displayArray(numbers, 5);

    return 0;
}
```

**Output:**

```
1 2 3 4 5
2 4 6 8 10
```

---

## 8. Pointers and Arrays

### 8.1 Array Name as Pointer

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[5] = {10, 20, 30, 40, 50};

    // WHY: Array name is pointer to first element
    cout << "arr: " << arr << endl;
    cout << "&arr[0]: " << &arr[0] << endl;
    cout << "Same? " << (arr == &arr[0]) << endl;

    // WHY: Both access methods equivalent
    cout << "arr[0]: " << arr[0] << endl;
    cout << "*arr: " << *arr << endl;

    cout << "arr[2]: " << arr[2] << endl;
    cout << "*(arr + 2): " << *(arr + 2) << endl;

    return 0;
}
```

### 8.2 Pointer to Array vs Array of Pointers

**Pointer to Array:**

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[5] = {1, 2, 3, 4, 5};

    // WHY: Pointer to entire array (all 5 elements)
    int (*ptr)[5] = &arr;

    // Access elements
    cout << "First element: " << (*ptr)[0] << endl;
    cout << "Third element: " << (*ptr)[2] << endl;

    return 0;
}
```

**Array of Pointers:**

```cpp
#include <iostream>
using namespace std;

int main() {
    int a = 10, b = 20, c = 30;

    // WHY: Array where each element is a pointer
    int* ptrArr[3] = {&a, &b, &c};

    // Access values
    for (int i = 0; i < 3; i++) {
        cout << "*ptrArr[" << i << "] = " << *ptrArr[i] << endl;
    }

    return 0;
}
```

**Output:**

```
*ptrArr[0] = 10
*ptrArr[1] = 20
*ptrArr[2] = 30
```

### 8.3 Iterating Array with Pointer

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[] = {100, 200, 300, 400, 500};
    int size = 5;

    // WHY: Pointer iteration
    int* ptr = arr;
    for (int i = 0; i < size; i++) {
        cout << *(ptr + i) << " ";
    }
    cout << endl;

    // WHY: Alternative with increment
    ptr = arr;
    while (ptr < arr + size) {
        cout << *ptr << " ";
        ptr++;
    }

    return 0;
}
```

---

## 9. Pointer to Pointer (Double Pointer)

### 9.1 Concept

**Definition:** Pointer that stores address of another pointer

```cpp
#include <iostream>
using namespace std;

int main() {
    int value = 42;
    int* ptr1 = &value;      // Pointer to int
    int** ptr2 = &ptr1;      // Pointer to pointer to int

    // WHY: Three ways to access value
    cout << "value: " << value << endl;
    cout << "*ptr1: " << *ptr1 << endl;
    cout << "**ptr2: " << **ptr2 << endl;

    // WHY: Modify via double pointer
    **ptr2 = 100;
    cout << "\nAfter **ptr2 = 100:" << endl;
    cout << "value: " << value << endl;

    return 0;
}
```

**Output:**

```
value: 42
*ptr1: 42
**ptr2: 42

After **ptr2 = 100:
value: 100
```

### 9.2 Memory Diagram

```
value:   [Address: 0x1000] → [Value: 42]
ptr1:    [Address: 0x2000] → [Value: 0x1000]
ptr2:    [Address: 0x3000] → [Value: 0x2000]

*ptr2 = 0x2000 (address of ptr1)
**ptr2 = 42 (value of value)
```

### 9.3 Use Cases

**Modifying pointer in function:**

```cpp
#include <iostream>
using namespace std;

// WHY: Need pointer-to-pointer to modify original pointer
void allocate(int** ptr, int value) {
    *ptr = new int(value);
}

int main() {
    int* myPtr = nullptr;

    allocate(&myPtr, 99);

    cout << "*myPtr: " << *myPtr << endl;

    delete myPtr;

    return 0;
}
```

**Dynamic 2D array:**

```cpp
#include <iostream>
using namespace std;

int main() {
    int rows = 3, cols = 4;

    // WHY: Array of pointers (each row is a pointer)
    int** matrix = new int*[rows];
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
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            cout << matrix[i][j] << "\t";
        }
        cout << endl;
    }

    // Cleanup
    for (int i = 0; i < rows; i++) {
        delete[] matrix[i];
    }
    delete[] matrix;

    return 0;
}
```

---

## 10. Function Pointers

### 10.1 Basic Function Pointer

```cpp
#include <iostream>
using namespace std;

// WHY: Define functions to point to
int add(int a, int b) {
    return a + b;
}

int multiply(int a, int b) {
    return a * b;
}

int main() {
    // WHY: Declare function pointer
    int (*operation)(int, int);

    // WHY: Point to add function
    operation = add;
    cout << "5 + 3 = " << operation(5, 3) << endl;

    // WHY: Point to multiply function
    operation = multiply;
    cout << "5 * 3 = " << operation(5, 3) << endl;

    return 0;
}
```

**Output:**

```
5 + 3 = 8
5 * 3 = 15
```

### 10.2 Function Pointer Syntax

**Declaration:**

```cpp
return_type (*pointer_name)(parameter_types);
```

**Examples:**

```cpp
void (*funcPtr1)();                  // No parameters, void return
int (*funcPtr2)(int, int);           // Two ints, returns int
double (*funcPtr3)(double);          // One double, returns double
void (*funcPtr4)(int*, char*);       // Pointer parameters
```

### 10.3 Callback Functions

```cpp
#include <iostream>
using namespace std;

// WHY: Generic function accepting callback
void process(int arr[], int size, void (*callback)(int)) {
    for (int i = 0; i < size; i++) {
        callback(arr[i]);
    }
}

// WHY: Different callback implementations
void printSquare(int x) {
    cout << x * x << " ";
}

void printDouble(int x) {
    cout << x * 2 << " ";
}

int main() {
    int numbers[] = {1, 2, 3, 4, 5};

    cout << "Squares: ";
    process(numbers, 5, printSquare);

    cout << "\nDoubles: ";
    process(numbers, 5, printDouble);

    return 0;
}
```

**Output:**

```
Squares: 1 4 9 16 25
Doubles: 2 4 6 8 10
```

### 10.4 Array of Function Pointers

```cpp
#include <iostream>
using namespace std;

int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
int mul(int a, int b) { return a * b; }
int div(int a, int b) { return a / b; }

int main() {
    // WHY: Array of function pointers (calculator)
    int (*operations[4])(int, int) = {add, sub, mul, div};

    int x = 20, y = 5;

    cout << x << " + " << y << " = " << operations[0](x, y) << endl;
    cout << x << " - " << y << " = " << operations[1](x, y) << endl;
    cout << x << " * " << y << " = " << operations[2](x, y) << endl;
    cout << x << " / " << y << " = " << operations[3](x, y) << endl;

    return 0;
}
```

---

## 11. Pointers vs References

### 11.1 Comparison Table

| Aspect | Pointer | Reference |
| --- | --- | --- |
| **Syntax** | `int* ptr = &var;` | `int& ref = var;` |
| **Nullability** | Can be `nullptr` | Cannot be null |
| **Reassignment** | Can point to different variables | Cannot be reseated |
| **Initialization** | Can be uninitialized (dangerous) | Must be initialized |
| **Arithmetic** | Pointer arithmetic allowed | No arithmetic |
| **Memory** | Has own address | Alias (no separate address) |
| **Indirection** | Explicit (`*ptr`) | Implicit (direct access) |
| **Use case** | Optional parameters, dynamic structures | Simple aliasing, parameters |

### 11.2 Code Comparison

```cpp
#include <iostream>
using namespace std;

int main() {
    int value = 10;
    int another = 20;

    // POINTER
    int* ptr = &value;
    cout << "*ptr: " << *ptr << endl;  // Need *

    ptr = &another;  // ✅ Can reassign
    cout << "*ptr: " << *ptr << endl;

    ptr = nullptr;  // ✅ Can be null

    // REFERENCE
    int& ref = value;
    cout << "ref: " << ref << endl;  // Direct access

    // ref = another;  // ❌ Cannot reseat (assigns value)
    ref = another;     // This copies value, doesn't reseat
    cout << "value: " << value << endl;  // value is now 20

    // int& ref2;  // ❌ Must initialize

    return 0;
}
```

### 11.3 When to Use Each

**Use Pointers When:**

- Parameter might be optional (nullptr)
- Need to reassign to different objects
- Working with dynamic memory
- Implementing data structures (linked list, tree)
- Need pointer arithmetic (arrays)
- C library interoperability

**Use References When:**

- Parameter must exist (never null)
- Simple aliasing
- Function return values (avoid copies)
- Range-based for loops
- Operator overloading

---

## 12. Smart Pointers Overview

### 12.1 Problem with Raw Pointers

```cpp
#include <iostream>
using namespace std;

void riskyFunction() {
    int* ptr = new int(42);

    // WHY: If exception thrown or early return, memory leaked!
    // ... complex code ...

    // May never reach here!
    delete ptr;
}
```

### 12.2 Smart Pointer Types (C++11)

**1. unique_ptr - Exclusive ownership**

```cpp
#include <iostream>
#include <memory>
using namespace std;

int main() {
    // WHY: Automatically deleted when out of scope
    unique_ptr<int> ptr1(new int(10));

    // Modern syntax (C++14)
    auto ptr2 = make_unique<int>(20);

    cout << *ptr1 << endl;

    // No need for delete - automatic cleanup!

    return 0;
}
```

**2. shared_ptr - Shared ownership**

```cpp
#include <iostream>
#include <memory>
using namespace std;

int main() {
    shared_ptr<int> ptr1 = make_shared<int>(100);

    {
        shared_ptr<int> ptr2 = ptr1;  // Reference count = 2
        cout << "Use count: " << ptr1.use_count() << endl;
    }  // ptr2 destroyed, count = 1

    cout << "Use count: " << ptr1.use_count() << endl;

    // Automatically deleted when last shared_ptr destroyed

    return 0;
}
```

**3. weak_ptr - Non-owning observer**

```cpp
#include <iostream>
#include <memory>
using namespace std;

int main() {
    shared_ptr<int> shared = make_shared<int>(50);

    // WHY: weak_ptr doesn't increase reference count
    weak_ptr<int> weak = shared;

    cout << "Shared count: " << shared.use_count() << endl;  // 1

    return 0;
}
```

### 12.3 Why Smart Pointers?

**Benefits:**

- Automatic memory management (RAII)
- No memory leaks
- Exception-safe
- Clear ownership semantics
- Thread-safe (shared_ptr reference counting)

**Note:** Smart pointers covered in detail in Part 9 (Advanced Topics)

---

## 13. Common Pitfalls and Best Practices

### 13.1 Common Mistakes

**Mistake 1: Dereferencing null pointer**

```cpp
// ❌ WRONG
int* ptr = nullptr;
cout << *ptr << endl;  // CRASH!

// ✅ CORRECT
int* ptr = nullptr;
if (ptr != nullptr) {
    cout << *ptr << endl;
}
```

**Mistake 2: Memory leak**

```cpp
// ❌ WRONG
void leak() {
    int* ptr = new int(10);
    // Forgot delete!
}

// ✅ CORRECT
void noLeak() {
    int* ptr = new int(10);
    // ... use ptr ...
    delete ptr;
}
```

**Mistake 3: Dangling pointer after delete**

```cpp
// ❌ WRONG
int* ptr = new int(5);
delete ptr;
cout << *ptr << endl;  // UNDEFINED BEHAVIOR!

// ✅ CORRECT
int* ptr = new int(5);
delete ptr;
ptr = nullptr;  // Set to nullptr after delete
```

**Mistake 4: Pointer arithmetic out of bounds**

```cpp
// ❌ WRONG
int arr[5] = {1, 2, 3, 4, 5};
int* ptr = arr;
ptr += 10;  // Out of bounds!
cout << *ptr << endl;  // UNDEFINED!

// ✅ CORRECT
int arr[5] = {1, 2, 3, 4, 5};
int* ptr = arr;
int* end = arr + 5;
if (ptr < end) {
    cout << *ptr << endl;
}
```

**Mistake 5: Returning pointer to local variable**

```cpp
// ❌ WRONG
int* getPointer() {
    int local = 10;
    return &local;  // Dangling!
}

// ✅ CORRECT
int* getPointer() {
    return new int(10);  // Heap allocation
}
```

### 13.2 Best Practices

**✅ Always initialize pointers**

```cpp
int* ptr = nullptr;  // Safe default
```

**✅ Check before dereferencing**

```cpp
if (ptr != nullptr) {
    *ptr = 10;
}
```

**✅ Set to nullptr after delete**

```cpp
delete ptr;
ptr = nullptr;
```

**✅ Use smart pointers**

```cpp
#include <memory>
auto ptr = make_unique<int>(10);
// Automatic cleanup!
```

**✅ Prefer references for simple cases**

```cpp
void func(int& ref) {  // Simpler than pointer
    ref = 10;
}
```

**✅ Use const for read-only access**

```cpp
void display(const int* ptr) {
    // Cannot modify data
}
```

**✅ Validate pointer arithmetic**

```cpp
if (ptr >= arr && ptr < arr + size) {
    // Safe to access
}
```

---

## Summary

### Key Takeaways

1. **Pointers store addresses** - Variables that hold memory addresses, enabling direct memory manipulation
2. **Address-of (&) and dereference (*)** - & gets address, * accesses value at address
3. **Pointer size is system-dependent** - 8 bytes on 64-bit, 4 bytes on 32-bit, regardless of pointed type
4. **Four special pointer types** - Wild (uninitialized), NULL (nullptr), void (generic), dangling (freed memory)
5. **Pointer arithmetic** - Operations adjust by sizeof(type), enabling efficient array traversal
6. **Three const variations** - Pointer to const, const pointer, const pointer to const
7. **Array-pointer relationship** - Array name decays to pointer to first element
8. **Pointer to pointer** - Double (or multiple) indirection for dynamic structures and modifying pointers
9. **Function pointers** - Store function addresses, enable callbacks and runtime function selection
10. **Smart pointers preferred** - unique_ptr, shared_ptr, weak_ptr provide automatic memory management

### Interview Essential Points

**Q: What is a pointer and why is it important in C++?**

A: A pointer is a variable that stores the memory address of another variable rather than a direct value. Pointers are crucial in C++ for: (1) Dynamic memory allocation - allocating memory at runtime using new/delete, (2) Efficient parameter passing - passing large objects by address avoids copying, (3) Data structure implementation - linked lists, trees, and graphs require pointers to link nodes, (4) Polymorphism - base class pointers enable runtime polymorphism, (5) Low-level programming - direct memory access for system programming and embedded systems. Pointers provide the flexibility and control that makes C++ suitable for performance-critical applications.

**Q: Explain the difference between NULL, nullptr, and 0 for pointers.**

A: NULL is a C legacy macro (typically defined as 0) used to represent null pointers, but it's actually an integer which can cause ambiguity in function overloading. 0 is a literal integer that implicitly converts to null pointer, but this conversion isn't type-safe. nullptr (C++11) is a keyword of type std::nullptr_t that specifically represents null pointers, providing type safety and no ambiguity. Example: `func(0)` might call `func(int)` instead of `func(int*)`, but `func(nullptr)` always calls the pointer overload. Best practice: Always use nullptr in modern C++.

**Q: What are wild, dangling, and void pointers?**

A: Wild pointer: Uninitialized pointer containing garbage address - dangerous to dereference. Dangling pointer: Points to memory that has been freed (via delete or local variable out of scope) - accessing causes undefined behavior. Void pointer (void*): Generic pointer that can hold address of any type but cannot be directly dereferenced - must cast to specific type first. Wild and dangling pointers are errors to avoid, while void pointers are intentionally used for type-erased storage in generic programming. Prevention: Initialize all pointers to nullptr, set to nullptr after delete, avoid returning addresses of local variables.

**Q: Explain pointer arithmetic and why pointer type matters.**

A: Pointer arithmetic means incrementing/decrementing pointers or adding/subtracting integers. When you do ptr++, the pointer advances by sizeof(pointed-type) bytes, not 1 byte. For int* (4 bytes per int), ptr++ moves 4 bytes forward; for char* (1 byte), ptr++ moves 1 byte forward. This enables natural array traversal: arr[i] is equivalent to *(arr + i). The arithmetic automatically scales by element size, so you think in terms of elements rather than bytes. Pointer type matters because the compiler needs to know how much memory each element occupies to calculate the correct address. This is why void* cannot be incremented - the compiler doesn't know the pointed type's size.

**Q: What's the difference between pointer to array and array of pointers?**

A: Pointer to array (`int (*ptr)[5]`): A single pointer that points to an entire array of 5 integers. The pointer holds one address, and dereferencing gives you the whole array. Used when passing multi-dimensional arrays to functions. Array of pointers (`int* arr[5]`): An array where each of the 5 elements is itself a pointer to int. Each element can point to different locations. Used for dynamic 2D arrays, string arrays, or when elements are scattered in memory. The key difference: first is ONE pointer TO an array, second is AN ARRAY OF pointers.

**Q: Explain pointer to pointer (double pointer) and its use cases.**

A: Pointer to pointer (int**) stores the address of another pointer. If ptr1 points to variable, ptr2 points to ptr1. Dereferencing once (*ptr2) gives ptr1's value (an address), dereferencing twice (**ptr2) gives the final variable's value. Use cases: (1) Modifying a pointer inside a function - passing &ptr allows function to change where original pointer points, (2) Dynamic 2D arrays - array of pointers where each pointer points to a row, (3) Linked data structures - when nodes themselves contain pointers that need modification. Example: Memory allocation functions often take int** to allocate memory and set the original pointer to the allocated address.

**Q: Compare pointers vs references - when to use each?**

A: Pointers: (1) Can be nullptr (optional parameters), (2) can be reassigned to different objects, (3) require explicit dereferencing (*ptr), (4) have their own memory address, (5) allow pointer arithmetic, (6) can be uninitialized (dangerous). References: (1) Must refer to existing object (no null), (2) cannot be reseated once bound, (3) access is direct (no operator needed), (4) are aliases (no separate address), (5) no arithmetic, (6) must be initialized. Use pointers for: dynamic memory, optional parameters, data structures, C interop. Use references for: simple parameter passing, avoiding copies without pointer syntax, return values, guaranteed non-null semantics. Rule of thumb: References for simplicity and safety when possible, pointers when you need their specific capabilities.

**Q: What are the three const pointer variations?**

A: Type 1: Pointer to const (`const int* ptr`) - Cannot modify data through pointer, but pointer can be reassigned. Read as "pointer to const int." Type 2: Const pointer (`int* const ptr`) - Cannot reassign pointer, but can modify data. Read as "const pointer to int." Type 3: Const pointer to const (`const int* const ptr`) - Cannot modify data OR reassign pointer. Read as "const pointer to const int." Mnemonic: Read declaration right-to-left. Common use: `const int*` in function parameters prevents function from modifying passed data, enabling pass-by-pointer efficiency with pass-by-value safety.

---