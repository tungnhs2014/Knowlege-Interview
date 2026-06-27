# 2.4. Inline Functions

---

## Table of Contents

1. What are Inline Functions?
2. Inline vs Macros
3. When Compiler Ignores Inline
4. Implicit Inline
5. Inline Functions in Headers

---

## 1. What are Inline Functions?

### 1.1 Definition & Mechanism

**Inline function:** Function where compiler *attempts* to replace function call with actual function code at compile time.

**How it works:**

```
Normal Function Call:           Inline Function:
┌──────────────────┐           ┌──────────────────┐
│ 1. Push args     │           │ Code inserted    │
│ 2. Save address  │           │ directly at      │
│ 3. Jump to func  │           │ call site        │
│ 4. Execute       │           │ (no jump)        │
│ 5. Return        │           │                  │
│ 6. Pop stack     │           │                  │
└──────────────────┘           └──────────────────┘
   Multiple steps                  Single step
```

**Purpose:**

- Eliminate function call overhead
- Enable further compiler optimizations
- Improve performance for small, frequently called functions

### 1.2 Basic Example

```cpp
#include <iostream>
using namespace std;

// WHY: Small function called frequently - good inline candidate
inline int square(int x) {
    return x * x;
}

// Regular function for comparison
int squareNormal(int x) {
    return x * x;
}

int main() {
    int result = 0;

    // Inline version - code inserted directly
    for (int i = 0; i < 1000000; i++) {
        result += square(i);  // Becomes: result += i * i;
    }

    // Normal version - actual function calls
    result = 0;
    for (int i = 0; i < 1000000; i++) {
        result += squareNormal(i);  // Jump to function, return
    }

    return 0;
}
```

**What compiler does (simplified assembly):**

```
; Inline version:
    mov eax, i
    imul eax, eax    ; result = i * i - directly here!
    add result, eax

; Normal version:
    push i           ; Push argument
    call squareNormal ; Jump to function
    add result, eax   ; Use returned value
```

### 1.3 When to Use Inline

**✅ Good Candidates:**

```cpp
// 1. Small functions (1-3 lines)
inline int max(int a, int b) {
    return (a > b) ? a : b;
}

// 2. Accessor/Getter functions
class Point {
    int x, y;
public:
    inline int getX() const { return x; }
    inline int getY() const { return y; }
};

// 3. Frequently called in tight loops
inline bool isEven(int n) {
    return n % 2 == 0;
}

// 4. Simple calculations
inline double celsiusToFahrenheit(double c) {
    return (c * 9.0 / 5.0) + 32.0;
}
```

**❌ Poor Candidates:**

```cpp
// 1. Large functions
inline void processData(int arr[], int size) {
    // 50 lines of code...
    // Inlining would bloat code size
}

// 2. Recursive functions
inline int factorial(int n) {
    return (n <= 1) ? 1 : n * factorial(n - 1);
    // Cannot inline recursive calls
}

// 3. Functions with loops
inline void printArray(int arr[], int size) {
    for (int i = 0; i < size; i++) {
        cout << arr[i] << " ";
    }
    // Too complex to inline
}

// 4. Virtual functions (runtime binding)
class Base {
    virtual inline void func() { }  // inline ignored!
};
```

### 1.4 Performance Implications

```cpp
#include <iostream>
#include <chrono>
using namespace std;

// WHY: Measure actual performance difference
inline int addInline(int a, int b) {
    return a + b;
}

int addNormal(int a, int b) {
    return a + b;
}

int main() {
    const int ITERATIONS = 100000000;
    int result = 0;

    // Test inline version
    auto start = chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERATIONS; i++) {
        result += addInline(i, i + 1);
    }
    auto end = chrono::high_resolution_clock::now();
    auto duration1 = chrono::duration_cast<chrono::milliseconds>(end - start);

    // Test normal version
    result = 0;
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < ITERATIONS; i++) {
        result += addNormal(i, i + 1);
    }
    end = chrono::high_resolution_clock::now();
    auto duration2 = chrono::duration_cast<chrono::milliseconds>(end - start);

    cout << "Inline: " << duration1.count() << "ms" << endl;
    cout << "Normal: " << duration2.count() << "ms" << endl;
    cout << "Speedup: " << (double)duration2.count() / duration1.count() << "x" << endl;

    return 0;
}
```

**Typical Output:**

```
Inline: 85ms
Normal: 120ms
Speedup: 1.41x
```

---

## 2. Inline vs Macros

### 2.1 What are Macros?

**Macro:** Preprocessor text replacement (before compilation).

```cpp
#define SQUARE(x) ((x) * (x))

int main() {
    int result = SQUARE(5);  // Replaced with: ((5) * (5))
    return 0;
}
```

### 2.2 Detailed Comparison

| Aspect | Inline Functions | Macros |
| --- | --- | --- |
| **Processing** | Compiler (type-checked) | Preprocessor (text substitution) |
| **Type Safety** | ✅ Yes - full type checking | ❌ No - no type info |
| **Debugging** | ✅ Can step through | ❌ Difficult - expanded text |
| **Scoping** | ✅ Follows C++ scope rules | ❌ No scope, just replacement |
| **Side Effects** | ✅ Arguments evaluated once | ❌ Can evaluate multiple times |
| **Return Type** | ✅ Explicit, type-safe | ❌ No return type |
| **Namespace** | ✅ Can be in namespace/class | ❌ Global only |
| **Recursion** | ✅ Can recurse (won't inline) | ❌ Cannot recurse |
| **Overloading** | ✅ Can overload | ❌ Cannot overload |

### 2.3 Problem 1: No Type Safety

```cpp
#include <iostream>
using namespace std;

// Macro - no type checking
#define SQUARE_MACRO(x) ((x) * (x))

// Inline - type-safe
inline int square(int x) {
    return x * x;
}

int main() {
    // Macro accepts anything!
    cout << SQUARE_MACRO(5) << endl;        // OK: 25
    cout << SQUARE_MACRO(2.5) << endl;      // "Works": 6.25
    cout << SQUARE_MACRO("hello") << endl;  // COMPILES! Runtime error

    // Inline enforces types
    cout << square(5) << endl;              // OK: 25
    // cout << square("hello") << endl;     // ERROR: type mismatch

    return 0;
}
```

### 2.4 Problem 2: Side Effects

```cpp
#include <iostream>
using namespace std;

// WHY: Macro evaluates arguments MULTIPLE times
#define SQUARE_MACRO(x) ((x) * (x))

inline int square(int x) {
    return x * x;
}

int main() {
    int a = 5;

    // Macro: increments a TWICE!
    cout << SQUARE_MACRO(a++) << endl;  // ((a++) * (a++))
    cout << "a after macro: " << a << endl;  // a = 7 (!!)

    a = 5;
    // Inline: increments a ONCE
    cout << square(a++) << endl;
    cout << "a after inline: " << a << endl;  // a = 6 (correct)

    return 0;
}
```

**Output:**

```
25
a after macro: 7
25
a after inline: 6
```

**Why macro fails:**

```cpp
SQUARE_MACRO(a++)
// Expands to: ((a++) * (a++))
// Step 1: a = 5, return 5, then a = 6
// Step 2: a = 6, return 6, then a = 7
// Result: 5 * 6 = 30 (WRONG!)
```

### 2.5 Problem 3: Debugging

```cpp
#define MAX_MACRO(a, b) ((a) > (b) ? (a) : (b))

inline int max_inline(int a, int b) {
    return (a > b) ? a : b;
}

int main() {
    int x = 10, y = 20;

    // Macro - cannot set breakpoint or step through
    int m1 = MAX_MACRO(x, y);  // Just text expansion

    // Inline - can debug normally
    int m2 = max_inline(x, y);  // Real function call in debug

    return 0;
}
```

### 2.6 Problem 4: Scoping

```cpp
#include <iostream>
using namespace std;

namespace Math {
    // Inline - properly scoped
    inline int square(int x) {
        return x * x;
    }
}

// Macro - always global!
#define SQUARE(x) ((x) * (x))

class Calculator {
public:
    // Can have inline member function
    inline int square(int x) {
        return x * x;
    }

    // CANNOT have macro as member
    // #define SQUARE(x) ((x) * (x))  // Makes no sense!
};

int main() {
    cout << Math::square(5) << endl;  // OK: namespace

    Calculator calc;
    cout << calc.square(5) << endl;   // OK: member function

    cout << SQUARE(5) << endl;        // OK: but global only

    return 0;
}
```

### 2.7 When to Use Each

**Use Inline Functions:**

```cpp
// ✅ Type-safe operations
inline double abs(double x) {
    return (x < 0) ? -x : x;
}

// ✅ Small frequently called functions
inline bool isEmpty(const string& s) {
    return s.empty();
}

// ✅ When you need overloading
inline int max(int a, int b);
inline double max(double a, double b);
```

**Use Macros:**

```cpp
// ✅ Conditional compilation
#ifdef DEBUG
    #define LOG(msg) cout << msg << endl
#else
    #define LOG(msg)  // No-op in release
#endif

// ✅ Platform-specific code
#ifdef _WIN32
    #define PATH_SEP '\\'
#else
    #define PATH_SEP '/'
#endif

// ✅ Include guards
#ifndef MY_HEADER_H
#define MY_HEADER_H
// ...
#endif
```

**Comparison Summary:**

```cpp
// ❌ DON'T: Use macros for functions
#define ADD(a, b) ((a) + (b))

// ✅ DO: Use inline functions
inline int add(int a, int b) {
    return a + b;
}

// Or even better: Use constexpr for compile-time evaluation
constexpr int add(int a, int b) {
    return a + b;
}
```

---

## 3. When Compiler Ignores Inline

### 3.1 Important Truth

**inline is a SUGGESTION, not a command!**

Compiler is free to:

- Ignore `inline` keyword (don't inline the function)
- Inline functions WITHOUT `inline` keyword (if beneficial)

### 3.2 Reasons Compiler Ignores Inline

**1. Function Too Large**

```cpp
// WHY: Too complex to inline
inline void processComplexData(int data[], int size) {
    // 100+ lines of code
    // Multiple loops
    // Complex logic
    // Compiler says: "NO! Too big!"
}
```

**2. Recursive Functions**

```cpp
// WHY: Cannot inline infinite recursion
inline int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);  // Recursive - cannot fully inline
}

// Compiler may inline the first call, but not recursive calls
```

**3. Functions with Loops**

```cpp
// WHY: Loop makes function non-trivial
inline void printNumbers(int n) {
    for (int i = 0; i < n; i++) {
        cout << i << " ";
    }
}
// Compiler likely ignores inline request
```

**4. Virtual Functions**

```cpp
class Base {
public:
    // WHY: Virtual = runtime binding, inline = compile-time
    virtual inline void display() {  // inline ignored!
        cout << "Base" << endl;
    }
};

class Derived : public Base {
public:
    virtual inline void display() {  // inline ignored!
        cout << "Derived" << endl;
    }
};

int main() {
    Base* ptr = new Derived();
    ptr->display();  // Runtime binding - cannot inline!

    // However, this CAN be inlined:
    Derived obj;
    obj.display();  // Compile-time known type - can inline

    delete ptr;
    return 0;
}
```

**5. Functions with Static Variables**

```cpp
// WHY: Static variable needs single storage location
inline int counter() {
    static int count = 0;  // Must exist in ONE place
    return ++count;
}
// Compiler may not inline to preserve static variable semantics
```

**6. Functions with switch/goto**

```cpp
// WHY: Complex control flow
inline int complexSwitch(int x) {
    switch (x) {
        case 1: return 10;
        case 2: return 20;
        // ... 50 more cases
        default: return 0;
    }
}
// Compiler likely won't inline
```

**7. Taking Function Address**

```cpp
inline int add(int a, int b) {
    return a + b;
}

int main() {
    // WHY: Taking address forces outline version
    int (*funcPtr)(int, int) = &add;  // Must have actual function!

    int result1 = add(5, 3);      // Can be inlined
    int result2 = funcPtr(5, 3);  // Cannot be inlined (pointer call)

    return 0;
}
```

### 3.3 Compiler Optimization Levels

```cpp
inline int square(int x) {
    return x * x;
}

// Compile with different optimization levels:
// g++ -O0 code.cpp  → inline probably ignored
// g++ -O1 code.cpp  → inline might be considered
// g++ -O2 code.cpp  → inline likely honored
// g++ -O3 code.cpp  → aggressive inlining
```

### 3.4 Force Inline (Compiler-Specific)

```cpp
// GCC/Clang
__attribute__((always_inline)) inline int square(int x) {
    return x * x;
}

// MSVC
__forceinline int square(int x) {
    return x * x;
}

// WARNING: Forcing inline can hurt performance!
// Trust the compiler's judgment in most cases
```

---

## 4. Implicit Inline

### 4.1 What is Implicit Inline?

**Functions defined inside class are automatically inline.**

```cpp
class MyClass {
public:
    // WHY: Defined inside class = implicitly inline
    int getValue() const {
        return value;  // Automatically inline!
    }

    void setValue(int v) {
        value = v;     // Automatically inline!
    }

private:
    int value;
};

// Equivalent to:
class MyClass {
public:
    inline int getValue() const {
        return value;
    }

    inline void setValue(int v) {
        value = v;
    }

private:
    int value;
};
```

### 4.2 Member Functions Defined Outside

```cpp
class MyClass {
public:
    int getValue() const;        // Declaration only - NOT inline
    void setValue(int v);        // Declaration only - NOT inline

private:
    int value;
};

// Definitions outside - need explicit inline keyword
inline int MyClass::getValue() const {
    return value;
}

inline void MyClass::setValue(int v) {
    value = v;
}
```

### 4.3 Real-World Example

```cpp
#include <iostream>
using namespace std;

class Point {
private:
    double x, y;

public:
    Point(double x = 0, double y = 0) : x(x), y(y) {}  // Implicit inline

    // Implicit inline - small accessors
    double getX() const { return x; }
    double getY() const { return y; }

    void setX(double newX) { x = newX; }
    void setY(double newY) { y = newY; }

    // Implicit inline - simple calculation
    double distanceFromOrigin() const {
        return sqrt(x * x + y * y);
    }

    // WHY: Defined outside - NOT inline unless specified
    void print() const;
};

// Definition outside class - need inline keyword
inline void Point::print() const {
    cout << "(" << x << ", " << y << ")" << endl;
}

int main() {
    Point p(3.0, 4.0);

    cout << "X: " << p.getX() << endl;  // Inlined
    cout << "Y: " << p.getY() << endl;  // Inlined
    cout << "Distance: " << p.distanceFromOrigin() << endl;  // Inlined
    p.print();  // May be inlined

    return 0;
}
```

### 4.4 Template Functions

```cpp
// WHY: Template functions are implicitly inline
template<typename T>
T max(T a, T b) {
    return (a > b) ? a : b;  // Automatically inline
}

// Equivalent to:
template<typename T>
inline T max(T a, T b) {
    return (a > b) ? a : b;
}
```

### 4.5 constexpr Functions (C++11)

```cpp
// WHY: constexpr functions are implicitly inline
constexpr int square(int x) {
    return x * x;  // Implicitly inline
}

// Can be used at compile-time
constexpr int result = square(5);  // Computed at compile-time
int arr[square(10)];               // Array size computed at compile-time
```

---

## 5. Inline Functions in Headers

### 5.1 The One Definition Rule (ODR)

**Problem:** Normal functions can only be defined once across all source files.

```cpp
// math.h
int add(int a, int b) {  // Definition in header
    return a + b;
}

// file1.cpp
#include "math.h"  // Gets definition of add()

// file2.cpp
#include "math.h"  // Gets SAME definition of add()

// Linker: ERROR! add() defined twice!
```

**Solution:** Inline functions are EXEMPT from ODR!

```cpp
// math.h
inline int add(int a, int b) {  // inline keyword
    return a + b;
}

// file1.cpp
#include "math.h"  // Gets inline add()

// file2.cpp
#include "math.h"  // Gets inline add()

// Linker: OK! Inline functions can have identical definitions
```

### 5.2 Why Inline Functions Must Be in Headers

```cpp
// ❌ WRONG: Inline in .cpp file
// math.cpp
inline int add(int a, int b) {
    return a + b;
}

// main.cpp
int add(int a, int b);  // Declaration
int main() {
    int result = add(5, 3);  // Compiler can't inline - no definition!
    return 0;
}
```

```cpp
// ✅ CORRECT: Inline in header
// math.h
inline int add(int a, int b) {
    return a + b;  // Definition visible to all files
}

// main.cpp
#include "math.h"
int main() {
    int result = add(5, 3);  // Compiler CAN inline - has definition!
    return 0;
}
```

### 5.3 Complete Header Example

```cpp
// Point.h
#ifndef POINT_H
#define POINT_H

#include <cmath>

class Point {
private:
    double x, y;

public:
    // WHY: Small functions defined in header for inlining
    Point(double x = 0, double y = 0) : x(x), y(y) {}

    double getX() const { return x; }
    double getY() const { return y; }

    void setX(double newX) { x = newX; }
    void setY(double newY) { y = newY; }

    double distanceFromOrigin() const {
        return std::sqrt(x * x + y * y);
    }

    // Large function - declaration only
    void complexOperation();
};

// Only if truly needed for inline
inline void Point::complexOperation() {
    // Implementation here if must be inline
}

#endif
```

```cpp
// Point.cpp
#include "Point.h"

// WHY: Large functions defined in .cpp (not inline)
void Point::complexOperation() {
    // Many lines of code...
    // Better as non-inline to avoid code bloat
}
```

### 5.4 Inline Variables (C++17)

```cpp
// config.h
#ifndef CONFIG_H
#define CONFIG_H

// Before C++17: Problem with global variables
extern const int MAX_SIZE;  // Declaration

// After C++17: inline variables
inline const int MAX_SIZE = 100;  // Definition in header - OK!

class Config {
public:
    inline static const int BUFFER_SIZE = 1024;  // C++17: OK in header
};

#endif
```

### 5.5 Best Practices

```cpp
// ✅ DO: Put small inline functions in headers
// utils.h
inline int square(int x) {
    return x * x;
}

// ✅ DO: Keep inline functions small (1-5 lines)
inline bool isEven(int n) {
    return n % 2 == 0;
}

// ❌ DON'T: Inline large functions in headers
inline void processLargeData(/*...*/) {
    // 100 lines of code...
    // This bloats EVERY file that includes this header!
}

// ✅ DO: Use include guards or #pragma once
#ifndef UTILS_H
#define UTILS_H
// or
#pragma once

// ✅ DO: Consider constexpr for compile-time evaluation
constexpr int factorial(int n) {
    return (n <= 1) ? 1 : n * factorial(n - 1);
}
```

---

## Summary

### Key Takeaways

1. **Inline functions** - Compiler suggestion to replace call with code; reduces overhead
2. **inline vs macros** - Inline: type-safe, debuggable, scoped; Macros: text replacement, error-prone
3. **Compiler ignores inline** - Large functions, recursion, loops, virtual functions, taking address
4. **Implicit inline** - Functions defined inside class body automatically inline
5. **Inline in headers** - Must be in headers for inlining; exempt from One Definition Rule
6. **When to use** - Small (1-5 lines), frequently called, simple logic, accessors/getters
7. **When not to use** - Large functions (code bloat), complex logic, rarely called
8. **Modern alternatives** - `constexpr` for compile-time; let compiler auto-inline with `O2`
9. **Optimization levels** - `O0` ignores inline, `O2`/`O3` honor and auto-inline
10. **Template/constexpr** - Implicitly inline; must be in headers

### Interview Essential Points

**Q: What is an inline function and how does it work?**
A: Inline function is a compiler suggestion to replace function call with function body at compile time, eliminating call overhead (push args, jump, return). Compiler inserts code directly at call site instead of generating actual call. Only beneficial for small, frequently called functions. Compiler may ignore inline request for large/complex functions.

**Q: What are the advantages of inline functions over macros?**
A: Inline functions provide: (1) Type safety - full type checking unlike text substitution, (2) Debuggable - can set breakpoints and step through, (3) Proper scoping - can be in namespaces/classes, (4) No side effects - arguments evaluated once, not multiple times like macros, (5) Can be overloaded. Macros only useful for conditional compilation and platform-specific code.

**Q: When does the compiler ignore the inline keyword?**
A: Compiler ignores inline for: (1) Large functions (too much code to inline), (2) Recursive functions (can't inline infinite recursion), (3) Functions with loops (complex logic), (4) Virtual functions (runtime binding conflicts with compile-time inlining), (5) Functions with static variables (need single storage), (6) When function address is taken (must have outline version), (7) Low optimization levels like `-O0`.

**Q: What is implicit inline in C++?**
A: Functions defined inside class body are automatically inline without explicit keyword. Example: getters/setters in class definition. Member functions defined outside class need explicit `inline` keyword. Template functions and `constexpr` functions are also implicitly inline.

**Q: Why must inline functions be defined in header files?**
A: For compiler to inline function, it needs complete definition at call site. If definition is in .cpp file, other translation units only see declaration, preventing inlining. Inline functions are exempt from One Definition Rule (ODR), allowing identical definitions in multiple translation units. Linker merges them into single function.

**Q: What's the difference between inline and constexpr?**
A: `constexpr` is stronger than `inline`: (1) constexpr can be evaluated at compile-time, inline only eliminates runtime call, (2) constexpr is implicitly inline, (3) constexpr has restrictions on what code is allowed, inline has no such limits, (4) constexpr results can be used in compile-time contexts like array sizes and template arguments.

**Q: Can virtual functions be inlined?**
A: Not through virtual function pointers/references (runtime binding). But can be inlined for direct object calls where type is known at compile time. Example: `obj.virtualFunc()` can be inlined if compiler knows concrete type, but `ptr->virtualFunc()` cannot because actual function is determined at runtime.

**Q: What happens if you have different definitions of an inline function?**
A: Undefined behavior. ODR exemption for inline functions requires all definitions to be IDENTICAL. If different, linker might use any version arbitrarily. Modern linkers may detect this and error, but behavior is not guaranteed. Always ensure inline function definitions are identical across all translation units by putting them in headers.

---