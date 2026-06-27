# 2.3. Function Overloading & Name Mangling

---

## Table of Contents

1. Function Overloading Basics
2. Overload Resolution Rules
3. Ambiguous Overloads
4. Name Mangling
5. extern "C" Linkage

---

## 1. Function Overloading Basics

### 1.1 Definition & Purpose

**Function overloading:** Multiple functions with same name but different parameter lists in the same scope.

**Purpose:**

- Intuitive interface for same operation on different types
- Compile-time polymorphism
- Code readability

```cpp
#include <iostream>
using namespace std;

// WHY: Same logical operation (addition) on different types
int add(int a, int b) {
    return a + b;
}

double add(double a, double b) {
    return a + b;
}

string add(const string& a, const string& b) {
    return a + b;
}

int main() {
    cout << add(5, 3) << endl;                              // int version
    cout << add(2.5, 3.7) << endl;                         // double version
    cout << add(string("Hello "), string("World")) << endl; // string version
    return 0;
}
```

**Output:**

```
8
6.2
Hello World
```

### 1.2 Overloading Criteria

**✅ Can overload by:**

**1. Number of parameters**

```cpp
void print(int x);
void print(int x, int y);
void print(int x, int y, int z);
```

**2. Type of parameters**

```cpp
void process(int x);
void process(double x);
void process(const string& x);
```

**3. Order of parameter types**

```cpp
void display(int a, double b);
void display(double a, int b);  // Different signature
```

**❌ Cannot overload by:**

```cpp
// ❌ WRONG: Return type alone doesn't create different signature
int getValue();
double getValue();  // ERROR!

// ❌ WRONG: const on value parameter ignored
void func(int x);
void func(const int x);  // ERROR: Same as above

// ✅ CORRECT: const on reference/pointer creates different signature
void func(int& x);
void func(const int& x);  // OK: Different signatures
```

### 1.3 Real-World Example

```cpp
#include <iostream>
#include <vector>
#include <string>
using namespace std;

class Logger {
public:
    // WHY: Overloading provides uniform logging interface
    void log(int value) {
        cout << "[INT] " << value << endl;
    }

    void log(double value) {
        cout << "[DOUBLE] " << value << endl;
    }

    void log(const string& message) {
        cout << "[STRING] " << message << endl;
    }

    void log(const vector<int>& numbers) {
        cout << "[VECTOR] ";
        for (int n : numbers) cout << n << " ";
        cout << endl;
    }
};

int main() {
    Logger logger;

    logger.log(42);
    logger.log(3.14);
    logger.log("Error occurred");
    logger.log(vector<int>{1, 2, 3});

    return 0;
}
```

**Output:**

```
[INT] 42
[DOUBLE] 3.14
[STRING] Error occurred
[VECTOR] 1 2 3
```

---

## 2. Overload Resolution Rules

### 2.1 The Resolution Process

**Three-step compiler process:**

1. **Candidate Functions** - Find all functions with matching name
2. **Viable Functions** - Filter by convertible arguments
3. **Best Match** - Select using matching hierarchy

### 2.2 Matching Hierarchy

| Rank | Match Type | Description | Example |
| --- | --- | --- | --- |
| **1** | **Exact Match** | Perfect type match | `func(int)` called with `int` |
| **2** | **Promotion** | Integral/floating promotion | `char`→`int`, `float`→`double` |
| **3** | **Standard Conversion** | Built-in type conversions | `int`→`double`, `Derived*`→`Base*` |
| **4** | **User-Defined Conversion** | Constructor/operator | `int`→`Complex` via constructor |

### 2.3 Exact Match

```cpp
#include <iostream>
using namespace std;

void func(int x) {
    cout << "int version: " << x << endl;
}

void func(double x) {
    cout << "double version: " << x << endl;
}

int main() {
    func(10);      // Exact match → int
    func(10.5);    // Exact match → double

    return 0;
}
```

**Output:**

```
int version: 10
double version: 10.5
```

### 2.4 Promotion

**Integral promotions:**

- `bool`, `char`, `signed char`, `unsigned char` → `int`
- `short`, `unsigned short` → `int`

**Floating promotions:**

- `float` → `double`

```cpp
#include <iostream>
using namespace std;

void func(int x) {
    cout << "int version: " << x << endl;
}

void func(double x) {
    cout << "double version: " << x << endl;
}

int main() {
    char c = 'A';
    func(c);       // Promotion: char → int

    short s = 100;
    func(s);       // Promotion: short → int

    float f = 3.14f;
    func(f);       // Promotion: float → double

    return 0;
}
```

**Output:**

```
int version: 65
int version: 100
double version: 3.14
```

### 2.5 Standard Conversion

```cpp
#include <iostream>
using namespace std;

void func(long x) {
    cout << "long version: " << x << endl;
}

void func(double x) {
    cout << "double version: " << x << endl;
}

int main() {
    int i = 10;
    func(i);       // Standard conversion: int → long

    return 0;
}
```

**Output:**

```
long version: 10
```

### 2.6 Resolution Example - Step by Step

```cpp
#include <iostream>
using namespace std;

void process(int x) {
    cout << "process(int): " << x << endl;
}

void process(double x) {
    cout << "process(double): " << x << endl;
}

void process(int x, int y) {
    cout << "process(int, int): " << x << ", " << y << endl;
}

int main() {
    // Step-by-step resolution

    // Call 1: process(5)
    // Candidates: All 3 functions named 'process'
    // Viable: process(int), process(double) [process(int,int) needs 2 args]
    // Best: process(int) - Exact match
    process(5);

    // Call 2: process(3.14)
    // Candidates: All 3 functions
    // Viable: process(int), process(double)
    // Best: process(double) - Exact match beats conversion
    process(3.14);

    // Call 3: process(1, 2)
    // Candidates: All 3 functions
    // Viable: only process(int, int)
    // Best: process(int, int) - Only viable
    process(1, 2);

    return 0;
}
```

**Output:**

```
process(int): 5
process(double): 3.14
process(int, int): 1, 2
```

---

## 3. Ambiguous Overloads

### 3.1 What is Ambiguity?

**Definition:** Compiler cannot determine which overload to call because multiple functions are equally good matches.

### 3.2 Common Causes

**Cause 1: Multiple Standard Conversions**

```cpp
#include <iostream>
using namespace std;

void func(int x) {
    cout << "int" << endl;
}

void func(double x) {
    cout << "double" << endl;
}

int main() {
    long value = 100;
    // func(value);  // ERROR: Ambiguous!
                     // int and double equally viable via conversion

    // Solution: Explicit cast
    func(static_cast<int>(value));     // OK
    func(static_cast<double>(value));  // OK

    return 0;
}
```

**Cause 2: Default Arguments**

```cpp
void process(int x, int y = 0);
void process(int x);              // Ambiguous with process(x, 0)

int main() {
    // process(5);  // ERROR: Ambiguous!
                    // Could call process(5) or process(5, 0)
    return 0;
}
```

**Cause 3: Implicit Conversions**

```cpp
#include <iostream>
using namespace std;

class Complex {
public:
    // WHY: Implicit conversion from int
    Complex(int real) : real(real), imag(0) {}
    int real, imag;
};

void func(Complex c) {
    cout << "Complex" << endl;
}

void func(int x) {
    cout << "int" << endl;
}

int main() {
    func(10);  // OK: Exact match for int beats user-defined conversion

    return 0;
}
```

**Output:**

```
int
```

### 3.3 Resolving Ambiguity

**Solution 1: Explicit Casting**

```cpp
void func(int x);
void func(double x);

int main() {
    long val = 100;
    func(static_cast<int>(val));  // Explicit choice
    return 0;
}
```

**Solution 2: Rename Functions**

```cpp
// Instead of overloading
void processInt(int x);
void processDouble(double x);
```

**Solution 3: Use Templates**

```cpp
template<typename T>
void process(T value) {
    // Generic handling
}
```

### 3.4 Complex Ambiguity Example

```cpp
#include <iostream>
using namespace std;

void display(int a, double b) {
    cout << "int, double" << endl;
}

void display(double a, int b) {
    cout << "double, int" << endl;
}

int main() {
    display(5, 3.14);     // OK: Exact match int, double
    display(3.14, 5);     // OK: Exact match double, int

    // display(5, 10);    // ERROR: Ambiguous!
                          // Both need conversion for 2nd param

    // Solution:
    display(5, 10.0);     // OK: Explicit double
    display(5.0, 10);     // OK: Explicit double

    return 0;
}
```

**Output:**

```
int, double
double, int
int, double
double, int
```

---

## 4. Name Mangling

### 4.1 What is Name Mangling?

**Definition:** Compiler encodes function names with parameter type information to create unique symbols for linker.

**Purpose:**

- Enable function overloading
- Provide type information to linker
- Prevent naming conflicts

**How it works:**

```
Original: void func(int x)
Mangled:  _Z4funci      (GCC/Clang)
         or ?func@@YAXH@Z (MSVC)
```

### 4.2 Why Name Mangling is Needed

**Without mangling:**

```cpp
void print(int x);
void print(double x);

// Linker sees:
// print
// print    ← Conflict! Same name!
```

**With mangling:**

```cpp
void print(int x);     → _Z5printi
void print(double x);  → _Z5printd

// Linker sees:
// _Z5printi
// _Z5printd  ← Unique names!
```

### 4.3 Mangling Examples (GCC/Clang)

**Pattern:** `_Z` + `length` + `name` + `type_encoding`

```cpp
// Original → Mangled (GCC/Clang)

void func();               // _Z4funcv
void func(int);            // _Z4funci
void func(int, int);       // _Z4funcii
void func(double);         // _Z4funcd
void func(char*);          // _Z4funcPc
void func(const int&);     // _Z4funcRKi

class MyClass {
    void method(int);      // _ZN7MyClass6methodEi
};

namespace NS {
    void func(int);        // _ZN2NS4funcEi
}
```

**Type Encoding:**

```
v = void
i = int
l = long
d = double
f = float
c = char
P = pointer
R = reference
K = const
```

### 4.4 Viewing Mangled Names

**Linux/Mac (nm command):**

```bash
# Compile
g++ example.cpp -c -o example.o

# View symbols
nm example.o

# Output:
# 0000000000000000 T _Z4funci
# 0000000000000010 T _Z4funcd
```

**C++ Demangle Utility:**

```bash
c++filt _Z4funci
# Output: func(int)

c++filt _Z4funcd
# Output: func(double)
```

**Windows (dumpbin):**

```bash
dumpbin /symbols example.obj

# Output:
# ?func@@YAXH@Z  = func(int)
# ?func@@YAXN@Z  = func(double)
```

### 4.5 Name Mangling Example - Complete

```cpp
// example.cpp
#include <iostream>
using namespace std;

void display(int x) {
    cout << "int: " << x << endl;
}

void display(double x) {
    cout << "double: " << x << endl;
}

class Calculator {
public:
    int add(int a, int b) {
        return a + b;
    }

    double add(double a, double b) {
        return a + b;
    }
};

int main() {
    display(10);
    display(3.14);

    Calculator calc;
    calc.add(5, 3);
    calc.add(2.5, 1.5);

    return 0;
}
```

**Compile and inspect:**

```bash
g++ example.cpp -c -o example.o
nm example.o | grep display

# Output (simplified):
# _Z7displayi     # display(int)
# _Z7displayd     # display(double)
```

---

## 5. extern "C" Linkage

### 5.1 The Problem: C++ Calling C

**C does not support overloading → No name mangling**

```cpp
// C library function (no mangling)
// In C header: stdio.h
int printf(const char* format, ...);
// Symbol name in library: printf (unmangled)
```

**If C++ mangles the name:**

```cpp
// C++ tries to call
int printf(const char* format, ...);

// C++ compiler mangles to: _Z6printfPKcz
// Linker looks for: _Z6printfPKcz
// C library provides: printf
// Result: LINKER ERROR - undefined reference!
```

### 5.2 Solution: extern "C"

**Purpose:** Tells C++ compiler to disable name mangling for C functions.

```cpp
// Tell C++ not to mangle this function
extern "C" {
    int printf(const char* format, ...);
}

int main() {
    printf("Hello World");  // Links correctly!
    return 0;
}
```

**How it works:**

```cpp
extern "C" int printf(...);

// C++ compiler:
// - Does NOT mangle: printf
// - Symbol name: printf
// - Linker finds: printf in C library ✓
```

### 5.3 extern "C" for Multiple Functions

```cpp
// Method 1: Block syntax
extern "C" {
    int printf(const char*, ...);
    void* malloc(size_t);
    void free(void*);
}

// Method 2: Individual declarations
extern "C" int printf(const char*, ...);
extern "C" void* malloc(size_t);
extern "C" void free(void*);
```

### 5.4 Standard C Headers

**All C standard headers use this pattern:**

```cpp
// stdio.h (simplified)
#ifdef __cplusplus
extern "C" {
#endif

int printf(const char* format, ...);
int scanf(const char* format, ...);
// ... more C functions

#ifdef __cplusplus
}
#endif
```

**Why `#ifdef __cplusplus`?**

- C++ compiler defines `__cplusplus`
- C compiler does not
- Makes header usable by both C and C++

### 5.5 Mixing C and C++ Code

**Scenario:** C++ program using C library

```cpp
// math_lib.h (C library header)
#ifdef __cplusplus
extern "C" {
#endif

int calculate(int x, int y);

#ifdef __cplusplus
}
#endif
```

```c
// math_lib.c (C implementation)
int calculate(int x, int y) {
    return x + y;
}
```

```cpp
// main.cpp (C++ program)
#include "math_lib.h"
#include <iostream>

int main() {
    int result = calculate(5, 3);  // Links correctly
    std::cout << result << std::endl;
    return 0;
}
```

**Compilation:**

```bash
gcc -c math_lib.c -o math_lib.o    # C compiler
g++ main.cpp math_lib.o -o program  # C++ linker
```

### 5.6 extern "C" with Function Pointers

```cpp
// WHY: Function pointer to C function
extern "C" typedef void (*CallbackFunc)(int);

extern "C" {
    void registerCallback(CallbackFunc cb);
}

// C++ implementation
extern "C" void myCallback(int value) {
    std::cout << "Callback: " << value << std::endl;
}

int main() {
    registerCallback(myCallback);  // Works!
    return 0;
}
```

### 5.7 Limitations of extern "C"

**❌ Cannot use with:**

- Function overloading
- Default arguments (in some cases)
- Templates
- Classes/namespaces

```cpp
// ❌ WRONG: Can't overload extern "C" functions
extern "C" {
    void func(int x);
    void func(double x);  // ERROR: Can't overload!
}

// ✅ CORRECT: Separate names
extern "C" {
    void funcInt(int x);
    void funcDouble(double x);
}
```

### 5.8 Real-World Example: Plugin System

```cpp
// plugin.h
#ifdef __cplusplus
extern "C" {
#endif

// WHY: Plugin interface must have C linkage for cross-language support
typedef void (*InitFunc)();
typedef void (*CleanupFunc)();

void* loadPlugin(const char* path);
InitFunc getInitFunction(void* plugin);
CleanupFunc getCleanupFunction(void* plugin);

#ifdef __cplusplus
}
#endif
```

```cpp
// plugin.cpp
#include <dlfcn.h>
#include "plugin.h"

extern "C" {
    void* loadPlugin(const char* path) {
        return dlopen(path, RTLD_LAZY);
    }

    InitFunc getInitFunction(void* plugin) {
        return (InitFunc)dlsym(plugin, "plugin_init");
    }

    CleanupFunc getCleanupFunction(void* plugin) {
        return (CleanupFunc)dlsym(plugin, "plugin_cleanup");
    }
}
```

**Why extern "C" here?**

- Dynamic loading requires unmangled names
- Plugins may be written in C or C++
- Symbol names must be predictable

---

## Summary

### Key Takeaways

1. **Function overloading** - Same name, different parameters; compile-time polymorphism
2. **Overload resolution** - 3-step process: candidates → viable → best match
3. **Matching hierarchy** - Exact match > Promotion > Standard conversion > User-defined
4. **Ambiguous overloads** - Multiple equally-good matches; resolve with explicit casts
5. **Name mangling** - Compiler encodes functions with type info for unique symbols
6. **Mangling scheme** - `_Z` + length + name + type codes (GCC); varies by compiler
7. **extern "C"** - Disables mangling for C compatibility
8. **C headers** - Use `#ifdef __cplusplus` + `extern "C"` block
9. **Mangling limitations** - No overloading in extern "C"; C linkage restrictions
10. **Viewing symbols** - Use `nm`, `c++filt` (Linux), `dumpbin` (Windows)

### Interview Essential Points

**Q: How does C++ support function overloading?**
A: Through name mangling. The compiler encodes function names with parameter type information to create unique symbols. For example, `func(int)` becomes `_Z4funci` and `func(double)` becomes `_Z4funcd`. The linker can then distinguish between overloaded functions because they have different mangled names, even though the source code uses the same function name.

**Q: Explain the overload resolution process.**
A: Compiler uses 3 steps: (1) Find candidate functions with matching name, (2) Filter to viable functions that can be called with given arguments, (3) Select best match using hierarchy: exact match beats promotion beats standard conversion beats user-defined conversion. If multiple functions are equally good, compilation fails with ambiguity error.

**Q: What causes ambiguous overloads?**
A: Multiple functions are equally good matches. Common causes: (1) Multiple standard conversions available, (2) Overlapping default arguments, (3) Implicit conversions creating ties. Example: `func(int)` and `func(double)` called with `long` - both require standard conversion, causing ambiguity.

**Q: Why is extern "C" needed?**
A: C doesn't support function overloading so C functions aren't mangled. When C++ calls C functions, it must disable mangling to match the unmangled names in C libraries. Without extern "C", C++ would look for mangled name `_Z6printfPKcz` instead of `printf`, causing linker errors.

**Q: Can you overload functions in extern "C" block?**
A: No. extern "C" uses C linkage which doesn't support mangling. Since overloading requires mangling to create unique symbols, you cannot overload functions in extern "C" blocks. You must give each function a distinct name.

**Q: How do you view mangled names?**
A: On Linux/Mac: `nm <object-file>` shows symbols; `c++filt <mangled-name>` demangles them. On Windows: `dumpbin /symbols <object-file>`. Can also use compiler flags like `-S` to generate assembly with mangled names visible.

**Q: What information does name mangling encode?**
A: Function name, parameter types (including const/volatile qualifiers), parameter count, namespace, class membership. Does NOT encode return type (since overloading by return type alone is illegal). Format varies by compiler - GCC uses Itanium ABI, MSVC uses its own scheme.

**Q: Give an example of promotion vs standard conversion.**
A: Promotion: `char` to `int`, `float` to `double` - small to larger of same category. Standard conversion: `int` to `double` - across categories, or pointer conversions. Promotion ranks higher than standard conversion in overload resolution. Example: `func(int)` beats `func(double)` for `short` argument (promotion vs conversion).

---