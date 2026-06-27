# 2.2. Parameter Passing Techniques

---

## Table of Contents

1. Parameter Passing Overview
2. Pass by Value
3. Pass by Reference
4. Pass by Pointer
5. const Parameters
6. Default Arguments
7. Array Parameters
8. Variadic Functions (C-style)
9. Variadic Templates (C++11)
10. Perfect Forwarding (C++11)

---

## 1. Parameter Passing Overview

### 1.1 What is Parameter Passing?

**Definition:** Parameter passing is the mechanism by which arguments are transmitted from the caller to the called function.

**Core Concept:**

- **Parameters:** Variables declared in function definition
- **Arguments:** Actual values passed during function call

```cpp
// WHY: Understanding the terminology
void process(int x) {    // 'x' is parameter
    cout << x << endl;
}

int main() {
    int value = 10;
    process(value);      // 'value' is argument
    return 0;
}
```

### 1.2 Three Main Mechanisms

| Mechanism | Symbol | Copy Made? | Can Modify Original? |
| --- | --- | --- | --- |
| **Pass by Value** | `type param` | ✅ Yes | ❌ No |
| **Pass by Reference** | `type& param` | ❌ No | ✅ Yes |
| **Pass by Pointer** | `type* param` | ❌ No (pointer copied) | ✅ Yes (via dereferencing) |

### 1.3 Choosing the Right Mechanism

**Decision Tree:**

```
Need to modify original?
├─ NO → Is object small (< 16 bytes)?
│      ├─ YES → Pass by value
│      └─ NO  → Pass by const reference
└─ YES → Prefer reference over pointer
         (use pointer only for optional/nullable params)
```

---

## 2. Pass by Value

### 2.1 Definition & Mechanism

**Pass by value:** A copy of the argument is created and passed to the function.

**How it works:**

1. Argument value is copied
2. Copy placed on stack (in function's stack frame)
3. Function works with copy
4. Original unchanged

**Memory Diagram:**

```
Caller's Stack Frame:          Function's Stack Frame:
┌──────────────┐              ┌──────────────┐
│  value = 10  │  ─ copy ──>  │   x = 10     │
└──────────────┘              └──────────────┘
   (original)                   (independent copy)
```

### 2.2 Basic Example

```cpp
#include <iostream>
using namespace std;

// WHY: Pass by value protects original from modification
void modify(int num) {
    num = 100;  // Only changes local copy
    cout << "Inside function: " << num << endl;
}

int main() {
    int value = 50;
    cout << "Before: " << value << endl;

    modify(value);  // Pass copy

    cout << "After: " << value << endl;  // Original unchanged

    return 0;
}
```

**Output:**

```
Before: 50
Inside function: 100
After: 50
```

### 2.3 When to Use Pass by Value

**✅ Use Pass by Value:**

- Built-in types (int, double, char, bool)
- Small objects (< 16 bytes typically)
- Want to protect original data
- Function needs own copy to modify

**❌ Avoid Pass by Value:**

- Large objects (arrays, big structs/classes)
- Need to modify original
- Performance-critical code with large data

### 2.4 Performance Implications

**Cost of Copying:**

```cpp
#include <iostream>
#include <vector>
#include <chrono>
using namespace std;

// WHY: Large object pass by value = expensive copy
void processVector(vector<int> data) {  // COPIES entire vector!
    cout << "Size: " << data.size() << endl;
}

void processVectorRef(const vector<int>& data) {  // No copy
    cout << "Size: " << data.size() << endl;
}

int main() {
    vector<int> bigData(1000000, 42);  // 1 million integers

    // Measure pass by value
    auto start = chrono::high_resolution_clock::now();
    processVector(bigData);  // Slow - copies 1M integers
    auto end = chrono::high_resolution_clock::now();
    auto duration1 = chrono::duration_cast<chrono::microseconds>(end - start);

    // Measure pass by reference
    start = chrono::high_resolution_clock::now();
    processVectorRef(bigData);  // Fast - no copy
    end = chrono::high_resolution_clock::now();
    auto duration2 = chrono::duration_cast<chrono::microseconds>(end - start);

    cout << "By value: " << duration1.count() << " μs" << endl;
    cout << "By reference: " << duration2.count() << " μs" << endl;

    return 0;
}
```

**Output (approximate):**

```
Size: 1000000
Size: 1000000
By value: 2500 μs
By reference: 5 μs
```

---

## 3. Pass by Reference

### 3.1 Definition & Mechanism

**Pass by reference:** Function receives direct access to the original variable through a reference.

**How it works:**

1. Reference created (alias to original)
2. No copy made
3. Function works with original
4. Changes affect original

**Memory Diagram:**

```
Caller's Stack Frame:          Function's Stack Frame:
┌──────────────┐              ┌──────────────┐
│  value = 10  │ <─ refers ── │  num (ref)   │
└──────────────┘              └──────────────┘
   (original)                   (alias to original)
```

### 3.2 Basic Example

```cpp
#include <iostream>
using namespace std;

// WHY: Reference allows modification of original
void increment(int& num) {  // & makes it reference
    num++;  // Modifies original variable
    cout << "Inside function: " << num << endl;
}

int main() {
    int value = 10;
    cout << "Before: " << value << endl;

    increment(value);  // Pass reference

    cout << "After: " << value << endl;  // Original changed!

    return 0;
}
```

**Output:**

```
Before: 10
Inside function: 11
After: 11
```

### 3.3 Common Use Cases

**1. Modify Original Value:**

```cpp
void swap(int& a, int& b) {
    int temp = a;
    a = b;
    b = temp;
}

int main() {
    int x = 5, y = 10;
    swap(x, y);
    cout << x << " " << y << endl;  // 10 5
    return 0;
}
```

**2. Return Multiple Values:**

```cpp
#include <iostream>
#include <cmath>
using namespace std;

// WHY: Return multiple values via references
void calculateCircle(double radius, double& area, double& circumference) {
    area = M_PI * radius * radius;
    circumference = 2 * M_PI * radius;
}

int main() {
    double area, circum;
    calculateCircle(5.0, area, circum);

    cout << "Area: " << area << endl;
    cout << "Circumference: " << circum << endl;

    return 0;
}
```

**Output:**

```
Area: 78.5398
Circumference: 31.4159
```

**3. Avoid Expensive Copies:**

```cpp
#include <string>
#include <iostream>
using namespace std;

// WHY: Avoid copying large strings
void processString(const string& text) {  // const ref - no copy, no modify
    cout << "Processing: " << text << endl;
}

int main() {
    string largeText = "Very long text...";
    processString(largeText);  // Fast - no copy made
    return 0;
}
```

### 3.4 Reference Best Practices

**DO:**

```cpp
// ✅ GOOD: const reference for large read-only data
void display(const vector<int>& data);

// ✅ GOOD: non-const reference for modification
void modify(string& text);

// ✅ GOOD: Return reference for chaining
MyClass& setName(const string& name);
```

**DON'T:**

```cpp
// ❌ BAD: Never return reference to local variable!
int& getBadValue() {
    int local = 10;
    return local;  // DANGER: local destroyed after return
}

// ❌ BAD: Reference to temporary
void process(const int&& temp);  // Confusing - use value or const ref
```

---

## 4. Pass by Pointer

### 4.1 Definition & Mechanism

**Pass by pointer:** Function receives the memory address of the argument.

**How it works:**

1. Pointer variable created (stores address)
2. Pointer itself copied (but not pointed-to data)
3. Function can access original via dereferencing
4. Changes via pointer affect original

**Memory Diagram:**

```
Caller's Stack Frame:          Function's Stack Frame:
┌──────────────┐              ┌──────────────┐
│  value = 10  │ <─ points ── │  ptr = 0x100 │
│  (at 0x100)  │              │  (copy of    │
└──────────────┘              │   address)   │
                              └──────────────┘
```

### 4.2 Basic Example

```cpp
#include <iostream>
using namespace std;

// WHY: Pointer allows modification and null checking
void increment(int* ptr) {
    if (ptr != nullptr) {  // Always check for null!
        (*ptr)++;  // Dereference to modify
        cout << "Inside function: " << *ptr << endl;
    }
}

int main() {
    int value = 10;
    cout << "Before: " << value << endl;

    increment(&value);  // Pass address with &

    cout << "After: " << value << endl;

    return 0;
}
```

**Output:**

```
Before: 10
Inside function: 11
After: 11
```

### 4.3 Pointers vs References

**Comparison Table:**

| Feature | Pointer | Reference |
| --- | --- | --- |
| **Syntax** | `type*`, needs `*` to access | `type&`, direct access |
| **Null value** | Can be `nullptr` | Cannot be null |
| **Reassignment** | Can point to different objects | Cannot be reseated |
| **Arithmetic** | Pointer arithmetic allowed | No arithmetic |
| **Use case** | Optional parameters, arrays, polymorphism | Simple parameter passing |

**When to Use Each:**

```cpp
// WHY: Pointer for optional parameters
void processOptional(int* data = nullptr) {
    if (data != nullptr) {
        cout << "Processing: " << *data << endl;
    } else {
        cout << "No data provided" << endl;
    }
}

// WHY: Reference when parameter is required
void processRequired(int& data) {  // Cannot be null
    cout << "Processing: " << data << endl;
}

int main() {
    int value = 10;

    processOptional(&value);  // With data
    processOptional();        // Without data - OK

    processRequired(value);   // Must provide value
    // processRequired();     // ERROR: cannot compile

    return 0;
}
```

### 4.4 Common Pointer Pitfalls

```cpp
// ❌ PITFALL 1: Forgetting to check nullptr
void danger(int* ptr) {
    *ptr = 10;  // CRASH if ptr is nullptr!
}

// ✅ CORRECT: Always check
void safe(int* ptr) {
    if (ptr != nullptr) {
        *ptr = 10;
    }
}

// ❌ PITFALL 2: Dangling pointer
int* getDanglingPointer() {
    int local = 10;
    return &local;  // DANGER: local destroyed!
}

// ✅ CORRECT: Return value or use dynamic allocation
int getValue() {
    int local = 10;
    return local;  // Safe - returns copy
}
```

---

## 5. const Parameters

### 5.1 What is const?

**Definition:** `const` keyword declares that a parameter cannot be modified by the function.

**Purpose:**

- Document intent (read-only)
- Enable compiler optimizations
- Prevent accidental modifications
- Allow passing temporaries to references

### 5.2 const with Different Passing Mechanisms

```cpp
#include <iostream>
#include <string>
using namespace std;

// WHY: const protects parameter from modification

// 1. const value (mostly useless - copy anyway)
void func1(const int value) {
    // value = 10;  // ERROR: cannot modify
    cout << value << endl;  // Can read
}

// 2. const reference (common - avoid copy + protect)
void func2(const string& text) {
    // text = "new";  // ERROR: cannot modify
    cout << text << endl;  // Can read
}

// 3. const pointer to const data (both protected)
void func3(const int* const ptr) {
    // *ptr = 10;    // ERROR: cannot modify data
    // ptr = nullptr; // ERROR: cannot modify pointer
    cout << *ptr << endl;  // Can read
}

int main() {
    int x = 5;
    string str = "Hello";

    func1(x);
    func2(str);
    func3(&x);

    return 0;
}
```

### 5.3 const Reference - The Sweet Spot

**Why const reference is preferred:**

```cpp
#include <vector>
#include <iostream>
using namespace std;

// ❌ BAD: Expensive copy
void display1(vector<int> data) {
    for (int x : data) cout << x << " ";
}

// ❌ BAD: Can accidentally modify
void display2(vector<int>& data) {
    // data.clear();  // Oops! Modified original
    for (int x : data) cout << x << " ";
}

// ✅ GOOD: No copy + protected
void display3(const vector<int>& data) {
    // data.clear();  // ERROR: prevented by const
    for (int x : data) cout << x << " ";
}

int main() {
    vector<int> numbers = {1, 2, 3, 4, 5};

    display1(numbers);  // Slow - copies vector
    display2(numbers);  // Fast but unsafe
    display3(numbers);  // Fast AND safe ✅

    return 0;
}
```

### 5.4 const Pointer Variations

```cpp
// WHY: Four combinations of const with pointers

// 1. Pointer to const data (data protected)
void func1(const int* ptr) {
    // *ptr = 10;     // ERROR
    ptr = nullptr;    // OK
}

// 2. const pointer to data (pointer protected)
void func2(int* const ptr) {
    *ptr = 10;        // OK
    // ptr = nullptr; // ERROR
}

// 3. const pointer to const data (both protected)
void func3(const int* const ptr) {
    // *ptr = 10;     // ERROR
    // ptr = nullptr; // ERROR
}

// 4. Pointer to data (nothing protected)
void func4(int* ptr) {
    *ptr = 10;        // OK
    ptr = nullptr;    // OK
}
```

**Mnemonic:** Read right-to-left

- `const int* ptr` = "ptr is a pointer to a const int"
- `int* const ptr` = "ptr is a const pointer to an int"

---

## 6. Default Arguments

### 6.1 Definition & Purpose

**Definition:** Default arguments are values automatically assigned to parameters if no argument is provided during function call.

**Purpose:**

- Reduce function overloading
- Provide optional parameters
- Simplify function calls
- Maintain backward compatibility

### 6.2 Basic Syntax

```cpp
#include <iostream>
#include <string>
using namespace std;

// WHY: Default arguments make parameters optional
void greet(string name, string greeting = "Hello", char end = '!') {
    cout << greeting << ", " << name << end << endl;
}

int main() {
    greet("Alice");                    // Hello, Alice!
    greet("Bob", "Hi");                // Hi, Bob!
    greet("Charlie", "Welcome", '.');  // Welcome, Charlie.

    return 0;
}
```

**Output:**

```
Hello, Alice!
Hi, Bob!
Welcome, Charlie.
```

### 6.3 Rules for Default Arguments

**Rule 1: Right-to-left assignment**

```cpp
// ✅ CORRECT: Defaults on right
void func1(int a, int b = 10, int c = 20);

// ❌ WRONG: Gap in defaults
void func2(int a, int b = 10, int c);  // ERROR!

// ✅ CORRECT: All after first default must have defaults
void func3(int a, int b = 10, int c = 20, int d = 30);
```

**Rule 2: Declaration vs Definition**

```cpp
// Defaults in DECLARATION (header file)
void process(int x = 10);

// Definition WITHOUT defaults
void process(int x) {
    cout << x << endl;
}

// ❌ WRONG: Defaults in both places
void process(int x = 10) {  // ERROR: redefinition of default argument
    cout << x << endl;
}
```

**Rule 3: Cannot be redefined**

```cpp
void func(int a = 10);   // First declaration
void func(int a = 20);   // ERROR: cannot change default value!
```

### 6.4 Real-World Examples

**Example 1: Configure Options**

```cpp
#include <iostream>
#include <string>
using namespace std;

// WHY: Server configuration with sensible defaults
void startServer(
    int port = 8080,
    string host = "localhost",
    int maxConnections = 100,
    bool useSSL = false
) {
    cout << "Server starting...\n";
    cout << "Host: " << host << ":" << port << "\n";
    cout << "Max connections: " << maxConnections << "\n";
    cout << "SSL: " << (useSSL ? "enabled" : "disabled") << "\n";
}

int main() {
    startServer();                        // All defaults
    startServer(3000);                    // Custom port
    startServer(443, "example.com", 200, true);  // Custom all

    return 0;
}
```

**Output:**

```
Server starting...
Host: localhost:8080
Max connections: 100
SSL: disabled
Server starting...
Host: localhost:3000
Max connections: 100
SSL: disabled
Server starting...
Host: example.com:443
Max connections: 200
SSL: enabled
```

**Example 2: Mathematical Operations**

```cpp
#include <iostream>
#include <cmath>
using namespace std;

// WHY: Power function with default exponent
double power(double base, double exponent = 2.0) {
    return pow(base, exponent);
}

int main() {
    cout << "5^2 = " << power(5) << endl;        // Default: square
    cout << "5^3 = " << power(5, 3) << endl;     // Cube
    cout << "5^0.5 = " << power(5, 0.5) << endl; // Square root

    return 0;
}
```

**Output:**

```
5^2 = 25
5^3 = 125
5^0.5 = 2.23607
```

### 6.5 Default Arguments vs Function Overloading

**When to use defaults:**

```cpp
// ✅ GOOD: Use defaults when behavior is same
void print(string msg, int times = 1) {
    for (int i = 0; i < times; i++) {
        cout << msg << " ";
    }
    cout << endl;
}
```

**When to use overloading:**

```cpp
// ✅ GOOD: Use overloading when logic differs
void print(int value) {
    cout << "Integer: " << value << endl;
}

void print(double value) {
    cout << "Double: " << value << endl;
}
```

---

## 7. Array Parameters

### 7.1 Array Decay to Pointer

**Critical Concept:** Arrays decay to pointers when passed to functions.

```cpp
#include <iostream>
using namespace std;

// WHY: These are IDENTICAL declarations!
void func1(int arr[]);      // Array notation
void func2(int arr[10]);    // Size ignored!
void func3(int* arr);       // Pointer notation

// All three are equivalent to:
void process(int* arr) {
    cout << "Size of arr: " << sizeof(arr) << endl;  // Pointer size (8 bytes on 64-bit)
    // Lost array size information!
}

int main() {
    int numbers[10] = {1, 2, 3, 4, 5};

    cout << "Size of numbers: " << sizeof(numbers) << endl;  // 40 bytes (10 * 4)

    process(numbers);  // Decays to int*

    return 0;
}
```

**Output:**

```
Size of numbers: 40
Size of arr: 8
```

### 7.2 Passing Array Size Separately

```cpp
#include <iostream>
using namespace std;

// WHY: Must pass size separately due to array decay
void printArray(int arr[], int size) {
    for (int i = 0; i < size; i++) {
        cout << arr[i] << " ";
    }
    cout << endl;
}

// Alternative: Use pointer notation (equivalent)
void printArray2(int* arr, int size) {
    for (int i = 0; i < size; i++) {
        cout << arr[i] << " ";
    }
    cout << endl;
}

int main() {
    int numbers[] = {10, 20, 30, 40, 50};
    int size = sizeof(numbers) / sizeof(numbers[0]);  // Calculate size

    printArray(numbers, size);
    printArray2(numbers, size);

    return 0;
}
```

**Output:**

```
10 20 30 40 50
10 20 30 40 50
```

### 7.3 Multi-Dimensional Arrays

```cpp
#include <iostream>
using namespace std;

// WHY: First dimension can be omitted, others must be specified
void print2D(int arr[][3], int rows) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < 3; j++) {
            cout << arr[i][j] << " ";
        }
        cout << endl;
    }
}

// Alternative: Pointer notation
void print2D_ptr(int (*arr)[3], int rows) {  // Pointer to array of 3 ints
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < 3; j++) {
            cout << arr[i][j] << " ";
        }
        cout << endl;
    }
}

int main() {
    int matrix[2][3] = {
        {1, 2, 3},
        {4, 5, 6}
    };

    print2D(matrix, 2);
    cout << "---" << endl;
    print2D_ptr(matrix, 2);

    return 0;
}
```

**Output:**

```
1 2 3
4 5 6
---
1 2 3
4 5 6
```

### 7.4 const Array Parameters

```cpp
#include <iostream>
using namespace std;

// WHY: const prevents accidental modification
void display(const int arr[], int size) {
    for (int i = 0; i < size; i++) {
        cout << arr[i] << " ";
        // arr[i] = 0;  // ERROR: cannot modify const array
    }
    cout << endl;
}

// WHY: Calculate sum without modifying
int sum(const int* arr, int size) {
    int total = 0;
    for (int i = 0; i < size; i++) {
        total += arr[i];
    }
    return total;
}

int main() {
    int numbers[] = {1, 2, 3, 4, 5};
    int size = 5;

    display(numbers, size);
    cout << "Sum: " << sum(numbers, size) << endl;

    return 0;
}
```

**Output:**

```
1 2 3 4 5
Sum: 15
```

### 7.5 Modern C++ Alternatives

```cpp
#include <iostream>
#include <array>
#include <vector>
using namespace std;

// WHY: std::array preserves size information
void printStdArray(const array<int, 5>& arr) {
    for (int x : arr) {
        cout << x << " ";
    }
    cout << endl;
}

// WHY: std::vector for dynamic arrays
void printVector(const vector<int>& vec) {
    for (int x : vec) {
        cout << x << " ";
    }
    cout << endl;
}

int main() {
    array<int, 5> arr = {1, 2, 3, 4, 5};
    vector<int> vec = {10, 20, 30};

    printStdArray(arr);
    printVector(vec);

    return 0;
}
```

**Output:**

```
1 2 3 4 5
10 20 30
```

---

## 8. Variadic Functions (C-style)

### 8.1 What are Variadic Functions?

**Definition:** Functions that accept variable number of arguments.

**Classic Example:** `printf()`

```cpp
printf("%d %s %f", 10, "hello", 3.14);  // Takes 4 arguments
printf("%d", 5);                         // Takes 2 arguments
```

**Purpose:**

- Flexible function interfaces
- Legacy C compatibility
- Generic utility functions

### 8.2 Macros and Types

**Required Header:** `<cstdarg>`

**Essential Components:**

```cpp
#include <cstdarg>  // For variadic macros

va_list args;           // Declare argument list
va_start(args, last);   // Initialize (last = last fixed param)
va_arg(args, type);     // Retrieve next argument of type
va_end(args);           // Clean up
```

### 8.3 Basic Example

```cpp
#include <iostream>
#include <cstdarg>
using namespace std;

// WHY: Sum variable number of integers
// First parameter tells how many values follow
int sum(int count, ...) {
    va_list args;
    va_start(args, count);  // Initialize after 'count'

    int total = 0;
    for (int i = 0; i < count; i++) {
        int value = va_arg(args, int);  // Get next int
        total += value;
    }

    va_end(args);  // Cleanup
    return total;
}

int main() {
    cout << sum(3, 10, 20, 30) << endl;      // 60
    cout << sum(5, 1, 2, 3, 4, 5) << endl;   // 15
    cout << sum(2, 100, 200) << endl;         // 300

    return 0;
}
```

**Output:**

```
60
15
300
```

### 8.4 Real-World Example: Custom Printf

```cpp
#include <iostream>
#include <cstdarg>
#include <cstring>
using namespace std;

// WHY: Type-safe printf-like function
// Format string: 'd'=int, 'f'=double, 'c'=char, 's'=string
void myPrintf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] == '%' && format[i+1] != '\0') {
            i++;  // Skip %
            switch (format[i]) {
                case 'd': {
                    int value = va_arg(args, int);
                    cout << value;
                    break;
                }
                case 'f': {
                    double value = va_arg(args, double);
                    cout << value;
                    break;
                }
                case 'c': {
                    // char promoted to int in va_arg
                    char value = (char)va_arg(args, int);
                    cout << value;
                    break;
                }
                case 's': {
                    const char* value = va_arg(args, const char*);
                    cout << value;
                    break;
                }
            }
        } else {
            cout << format[i];
        }
    }

    va_end(args);
}

int main() {
    myPrintf("Integer: %d, Float: %f, Char: %c, String: %s\n",
             42, 3.14, 'A', "Hello");

    return 0;
}
```

**Output:**

```
Integer: 42, Float: 3.14, Char: A, String: Hello
```

### 8.5 Dangers and Limitations

**⚠️ Problem 1: No Type Safety**

```cpp
// ❌ DANGER: Wrong type extraction
void badFunction(int count, ...) {
    va_list args;
    va_start(args, count);

    // Passed double, but extracting as int!
    int value = va_arg(args, int);  // UNDEFINED BEHAVIOR

    va_end(args);
}

int main() {
    badFunction(1, 3.14);  // CRASH or garbage value!
    return 0;
}
```

**⚠️ Problem 2: Must Know Count**

```cpp
// Need count or sentinel value
int sum(int count, ...);  // Requires count parameter

// OR use sentinel (like NULL in printf)
void printStrings(...) {
    // Must pass NULL at end
    // myFunc("a", "b", "c", NULL);
}
```

**⚠️ Problem 3: Type Promotion**

```cpp
// WHY: Small types promoted to int/double
void func(...) {
    va_list args;
    // va_start...

    // char and short promoted to int
    int c = va_arg(args, int);  // NOT va_arg(args, char)

    // float promoted to double
    double f = va_arg(args, double);  // NOT va_arg(args, float)
}
```

### 8.6 When to Use C-style Variadic

**✅ Use when:**

- Interfacing with C libraries
- Need printf-like functionality
- Legacy code maintenance

**❌ Avoid when:**

- Writing new C++ code (use variadic templates instead)
- Type safety is important
- Working with non-trivial types

---

## 9. Variadic Templates (C++11)

### 9.1 Introduction

**Definition:** Type-safe templates that accept arbitrary number of template parameters.

**Advantages over C-style:**

- ✅ Type safe (compile-time checked)
- ✅ Works with any type (not just POD)
- ✅ No runtime overhead
- ✅ Modern C++ idiom

**Syntax:**

```cpp
template<typename... Args>  // Args is parameter pack
void func(Args... args) {   // args is function parameter pack
    // Use args...
}
```

### 9.2 Basic Example - Recursive Expansion

```cpp
#include <iostream>
using namespace std;

// WHY: Base case - stops recursion
void print() {
    cout << endl;
}

// WHY: Recursive variadic template
template<typename T, typename... Args>
void print(T first, Args... rest) {
    cout << first << " ";
    print(rest...);  // Recursive call with remaining arguments
}

int main() {
    print(1, 2.5, "hello", 'A');
    print("C++", 11, 14, 17, 20);

    return 0;
}
```

**Output:**

```
1 2.5 hello A
C++ 11 14 17 20
```

**How it works:**

```
print(1, 2.5, "hello", 'A')
├─ first=1, rest={2.5, "hello", 'A'}
│  ├─ first=2.5, rest={"hello", 'A'}
│  │  ├─ first="hello", rest={'A'}
│  │  │  ├─ first='A', rest={}
│  │  │  │  └─ print() // base case
```

### 9.3 sizeof... Operator

```cpp
#include <iostream>
using namespace std;

// WHY: Count number of arguments at compile-time
template<typename... Args>
void countArgs(Args... args) {
    cout << "Number of arguments: " << sizeof...(Args) << endl;
    cout << "Number of arguments: " << sizeof...(args) << endl;  // Same
}

int main() {
    countArgs(1, 2, 3);              // 3
    countArgs("a", "b", "c", "d");   // 4
    countArgs();                      // 0

    return 0;
}
```

**Output:**

```
Number of arguments: 3
Number of arguments: 4
Number of arguments: 0
```

### 9.4 Fold Expressions (C++17)

**Modern way to expand parameter packs:**

```cpp
#include <iostream>
using namespace std;

// WHY: Fold expressions eliminate recursion

// Sum using fold expression
template<typename... Args>
auto sum(Args... args) {
    return (args + ...);  // Right fold: (a + (b + (c + d)))
}

// Print using fold expression
template<typename... Args>
void print(Args... args) {
    ((cout << args << " "), ...);  // Left fold with comma operator
    cout << endl;
}

// Logical AND
template<typename... Args>
bool allTrue(Args... args) {
    return (args && ...);  // Right fold: (a && (b && (c && d)))
}

int main() {
    cout << "Sum: " << sum(1, 2, 3, 4, 5) << endl;  // 15

    print(1, 2.5, "hello", 'A');

    cout << "All true: " << allTrue(true, true, true) << endl;      // 1
    cout << "All true: " << allTrue(true, false, true) << endl;     // 0

    return 0;
}
```

**Output:**

```
Sum: 15
1 2.5 hello A
All true: 1
All true: 0
```

### 9.5 Real-World Example: Generic Factory

```cpp
#include <iostream>
#include <memory>
#include <string>
using namespace std;

// WHY: Create objects with arbitrary constructor arguments
template<typename T, typename... Args>
unique_ptr<T> makeUnique(Args&&... args) {
    return unique_ptr<T>(new T(forward<Args>(args)...));
}

class Person {
    string name;
    int age;
public:
    Person(string n, int a) : name(n), age(a) {
        cout << "Person created: " << name << ", " << age << endl;
    }
};

int main() {
    auto p1 = makeUnique<Person>("Alice", 25);
    auto p2 = makeUnique<Person>("Bob", 30);

    auto num = makeUnique<int>(42);
    auto str = makeUnique<string>("Hello");

    return 0;
}
```

**Output:**

```
Person created: Alice, 25
Person created: Bob, 30
```

### 9.6 Variadic Class Templates

```cpp
#include <iostream>
using namespace std;

// WHY: Tuple-like class with variable number of types
template<typename... Types>
class MultiStorage;

// Base case: empty
template<>
class MultiStorage<> {
public:
    static constexpr size_t size = 0;
};

// Recursive case
template<typename Head, typename... Tail>
class MultiStorage<Head, Tail...> {
    Head value;
    MultiStorage<Tail...> rest;

public:
    static constexpr size_t size = sizeof...(Tail) + 1;

    MultiStorage(Head h, Tail... t) : value(h), rest(t...) {}

    Head& getFirst() { return value; }
};

int main() {
    MultiStorage<int, double, string> storage(42, 3.14, "Hello");

    cout << "Size: " << storage.size << endl;
    cout << "First: " << storage.getFirst() << endl;

    return 0;
}
```

**Output:**

```
Size: 3
First: 42

```

---

## 10. Perfect Forwarding (C++11)

### 10.1 The Problem

**Issue:** Losing value category (lvalue/rvalue) when passing arguments through wrapper functions.

```cpp
#include <iostream>
using namespace std;

void process(int& x) {
    cout << "Lvalue: " << x << endl;
}

void process(int&& x) {
    cout << "Rvalue: " << x << endl;
}

// ❌ PROBLEM: wrapper loses value category
template<typename T>
void badWrapper(T arg) {
    process(arg);  // Always lvalue here!
}

int main() {
    int x = 10;
    process(x);      // Lvalue
    process(20);     // Rvalue

    badWrapper(x);   // Lvalue (correct)
    badWrapper(30);  // Lvalue (WRONG! Should be rvalue)

    return 0;
}
```

**Output:**

```
Lvalue: 10
Rvalue: 20
Lvalue: 10
Lvalue: 30  ← WRONG!
```

### 10.2 Solution: Universal References + std::forward

**Universal Reference:** `T&&` in template context can bind to both lvalues and rvalues.

```cpp
#include <iostream>
#include <utility>  // for std::forward
using namespace std;

void process(int& x) {
    cout << "Lvalue: " << x << endl;
}

void process(int&& x) {
    cout << "Rvalue: " << x << endl;
}

// ✅ CORRECT: Perfect forwarding wrapper
template<typename T>
void perfectWrapper(T&& arg) {  // Universal reference
    process(forward<T>(arg));   // Preserve value category
}

int main() {
    int x = 10;

    perfectWrapper(x);   // Lvalue (correct)
    perfectWrapper(30);  // Rvalue (correct!)

    return 0;
}
```

**Output:**

```
Lvalue: 10
Rvalue: 30
```

### 10.3 How Perfect Forwarding Works

**Reference Collapsing Rules:**

```cpp
// Given: template<typename T> void func(T&& param)

int x = 10;
func(x);      // T = int&,  T&& = int& &&  → int&   (lvalue ref)
func(10);     // T = int,   T&& = int&&    → int&&  (rvalue ref)

// Rules:
// & &   → &
// & &&  → &
// && &  → &
// && && → &&
```

**std::forward mechanism:**

```cpp
// Simplified implementation
template<typename T>
T&& forward(remove_reference_t<T>& arg) {
    return static_cast<T&&>(arg);
}

// If T = int&:  returns int& &&  → int&  (lvalue)
// If T = int:   returns int&&    → int&& (rvalue)
```

### 10.4 Variadic Perfect Forwarding

```cpp
#include <iostream>
#include <utility>
#include <memory>
using namespace std;

// WHY: Forward arbitrary number of arguments perfectly
template<typename T, typename... Args>
unique_ptr<T> createObject(Args&&... args) {
    return unique_ptr<T>(new T(forward<Args>(args)...));
}

class Widget {
public:
    Widget(int x, const string& s) {
        cout << "Widget(" << x << ", " << s << ")" << endl;
    }

    Widget(int x, string&& s) {
        cout << "Widget(" << x << ", rvalue string)" << endl;
    }
};

int main() {
    string str = "Hello";

    // Lvalue string
    auto w1 = createObject<Widget>(10, str);

    // Rvalue string
    auto w2 = createObject<Widget>(20, string("World"));

    return 0;
}
```

**Output:**

```
Widget(10, Hello)
Widget(20, rvalue string)
```

### 10.5 Real-World Application

```cpp
#include <iostream>
#include <vector>
#include <utility>
using namespace std;

template<typename T>
class Container {
    vector<T> data;

public:
    // WHY: Perfect forwarding to emplace_back
    template<typename... Args>
    void add(Args&&... args) {
        data.emplace_back(forward<Args>(args)...);
    }

    void display() {
        for (const auto& item : data) {
            cout << item << " ";
        }
        cout << endl;
    }
};

int main() {
    Container<string> cont;

    string str = "Hello";
    cont.add(str);              // Copy lvalue
    cont.add("World");          // Move rvalue
    cont.add(string("C++"));    // Move rvalue

    cont.display();

    return 0;
}
```

**Output:**

```
Hello World C++
```

### 10.6 Common Mistakes

```cpp
// ❌ MISTAKE 1: Using auto&& without forward
template<typename T>
void bad1(T&& arg) {
    auto copy = arg;  // Always copies!
}

// ✅ CORRECT: Use std::move for rvalues
template<typename T>
void good1(T&& arg) {
    auto copy = forward<T>(arg);  // Preserves category
}

// ❌ MISTAKE 2: Forwarding same parameter twice
template<typename T>
void bad2(T&& arg) {
    func1(forward<T>(arg));
    func2(forward<T>(arg));  // DANGER: arg may be moved!
}

// ✅ CORRECT: Only forward once, or copy first
template<typename T>
void good2(T&& arg) {
    auto copy = arg;  // Make copy
    func1(forward<T>(arg));
    func2(copy);
}
```

---

## Summary

### Key Takeaways

1. **Three main passing mechanisms** - By value (copy), by reference (alias), by pointer (address)
2. **Pass by value** - Creates copy, safe but expensive for large objects; use for small types
3. **Pass by reference** - No copy, can modify original; use `const&` for large read-only data
4. **Pass by pointer** - Like reference but nullable; use for optional parameters or C compatibility
5. **const parameters** - Document intent, enable optimizations, prevent modifications; `const&` is sweet spot
6. **Default arguments** - Optional parameters with sensible defaults; must be right-to-left
7. **Array decay** - Arrays become pointers when passed; must pass size separately; prefer `std::array` or `std::vector`
8. **C-style variadic** - Uses `va_list`, `va_start`, `va_arg`, `va_end`; not type-safe, legacy only
9. **Variadic templates** - Type-safe, modern C++ way; use recursion or fold expressions
10. **Perfect forwarding** - Preserve value category with `T&&` and `std::forward`; essential for wrapper functions

### Interview Essential Points

**Q: Explain the difference between pass by value, reference, and pointer.**
A: Pass by value creates a copy of the argument, changes don't affect original but copying is expensive for large objects. Pass by reference creates an alias, no copy made, changes affect original, faster for large objects. Pass by pointer passes address, similar to reference but can be null and reassigned, used for optional parameters.

**Q: When should you use const with parameters?**
A: Use `const` with reference parameters when you want to avoid copying (efficiency) but don't want to modify the original (safety). This is the "const reference" pattern - most common for passing large objects to functions. Also use `const` with pointers when data shouldn't be modified.

**Q: What is array decay in C++?**
A: Array decay is when an array name is automatically converted to a pointer to its first element when passed to a function. This loses size information, which is why you must pass the array size as a separate parameter. In modern C++, prefer `std::array` or `std::vector` which don't decay.

**Q: Explain variadic functions in C++ (both C-style and templates).**
A: C-style variadic functions use `va_list`, `va_start`, `va_arg`, `va_end` macros from `<cstdarg>`. They're not type-safe and only work with POD types. Variadic templates (C++11) are type-safe, work with any type, and use parameter packs. Modern code should use variadic templates with fold expressions (C++17) for cleaner syntax.

**Q: What is perfect forwarding and why is it needed?**
A: Perfect forwarding preserves the value category (lvalue/rvalue) of arguments when passing them through wrapper functions. Without it, rvalue arguments become lvalues when passed through. Implemented using universal references (`T&&` in template context) and `std::forward<T>()`. Essential for factory functions and wrappers like `std::make_unique`.

**Q: What is the sizeof... operator?**
A: `sizeof...` is a compile-time operator that returns the number of elements in a parameter pack. Used in variadic templates to get argument count without runtime overhead. Example: `template<typename... Args> void f(Args... args) { cout << sizeof...(Args); }`

**Q: What are fold expressions and when were they introduced?**
A: Fold expressions (C++17) provide clean syntax for expanding parameter packs. Replace recursive template instantiation with single expression. Four types: unary/binary left/right folds. Example: `return (args + ...);` sums all arguments. More readable and faster to compile than recursive templates.

**Q: Why is const reference preferred over pass by value for large objects?**
A: Const reference avoids expensive copying of large objects while preventing accidental modification. Pass by value copies entire object onto stack, which for large containers like `std::vector` can be thousands of times slower. Const reference passes only address (8 bytes on 64-bit) while guaranteeing read-only access through compiler enforcement.

---