# 1.2. Variables, Data Types, Storage & Scope

---

## Table of Contents

1. Variables & Constants
2. Storage Classes
3. Variable Scope & Lifetime
4. Data Types & Modifiers

---

## 1. Variables & Constants

### 1.1 What is a Variable?

**Definition:** A variable is a named memory location that stores a value which can be modified during program execution.

**Purpose:**

- Store and manipulate data
- Make programs flexible and reusable
- Enable dynamic behavior

**Syntax:**

```cpp
type variable_name = initial_value;
```

### 1.2 Variable Declaration vs Initialization

```cpp
#include <iostream>
using namespace std;

int main() {
    // Declaration only (contains garbage value)
    int age;

    // Declaration + Initialization
    int score = 100;  // C-style initialization
    int count(50);    // Constructor initialization
    int total{75};    // Uniform initialization (C++11) - RECOMMENDED

    // Why uniform initialization is better:
    int value{3.14};  // ❌ Compile error: narrowing conversion prevented
    int bad = 3.14;   // ✅ Compiles but data loss occurs (becomes 3)

    return 0;
}
```

**Key Point:** Always initialize variables before use to avoid undefined behavior.

### 1.3 Variable Naming Rules

**Must Follow:**

- Start with letter (a-z, A-Z) or underscore `_`
- Can contain letters, digits (0-9), underscores
- Case-sensitive (`age` ≠ `Age`)
- Cannot use C++ keywords (`int`, `class`, `for`, etc.)

**Best Practices:**

```cpp
// ✅ GOOD - Descriptive names
int studentCount = 50;
double accountBalance = 1500.75;
string userName = "Alice";

// ❌ BAD - Not descriptive
int x = 50;       // What does x represent?
double a = 1500;  // Meaningless name
string s = "Alice";  // Too short

// Naming conventions
int camelCase;       // Variables, functions
int PascalCase;      // Classes
int SCREAMING_CASE;  // Constants/Macros
int snake_case;      // Alternative style
```

### 1.4 Constants

**Definition:** Variables whose values cannot be changed after initialization.

**Purpose:**

- Prevent accidental modification
- Make code more readable (named values instead of magic numbers)
- Enable compiler optimizations

### Method 1: `const` Keyword

```cpp
#include <iostream>
using namespace std;

int main() {
    const double PI = 3.14159265359;  // Mathematical constant
    const int MAX_STUDENTS = 100;     // System limit

    // PI = 3.14;  // ❌ Compile error: cannot modify const

    cout << "Circle area: " << PI * 5 * 5 << endl;
    return 0;
}
```

**When to use:**

- Mathematical constants (PI, E)
- Configuration values
- Array sizes (for fixed-size arrays)

### Method 2: `constexpr` Keyword (C++11)

```cpp
constexpr int factorial(int n) {
    return (n <= 1) ? 1 : n * factorial(n - 1);  // Computed at compile-time
}

int main() {
    constexpr int result = factorial(5);  // Evaluated during compilation
    // const int result2 = factorial(5); // Can be runtime

    int arr[result];  // OK - compile-time constant
    return 0;
}
```

**`const` vs `constexpr`:**

| Feature | `const` | `constexpr` |
| --- | --- | --- |
| **Evaluation** | Runtime or compile-time | Compile-time only |
| **Use case** | Read-only variables | Compile-time constants |
| **With functions** | Not applicable | Must be evaluable at compile-time |
| **Example** | `const int x = getUserInput();` | `constexpr int x = 5 * 3;` |

**When to use:**

- `const`: Runtime values that shouldn't change (configuration, user input)
- `constexpr`: Compile-time computations (array sizes, template arguments)

### Method 3: `#define` Preprocessor (Not Recommended)

```cpp
#define MAX_SIZE 100  // No type safety, just text replacement

int main() {
    int arr[MAX_SIZE];  // Works but prefer constexpr
    return 0;
}
```

**Why avoid `#define`:**

- No type checking
- No scope (global by default)
- Debugging difficulties
- Not namespace-aware

---

## 2. Storage Classes

**Definition:** Storage classes specify the **scope**, **visibility**, **lifetime**, and **location** of variables.

**Purpose:** Control how and where variables are stored in memory.

### 2.1 Types of Storage Classes

| Storage Class | Scope | Lifetime | Default Value | Memory Location |
| --- | --- | --- | --- | --- |
| `auto` | Local | Block execution | Garbage | Stack (RAM) |
| `static` | Local/Global | Program lifetime | 0 | Data segment |
| `extern` | Global | Program lifetime | 0 | Data segment |
| `register` | Local | Block execution | Garbage | CPU register (if available) |
| `mutable` | Class member | Object lifetime | - | With object |
| `thread_local` | Thread-specific | Thread lifetime | 0 | Thread storage |

### 2.2 Auto Storage Class

**Note:** In modern C++ (C++11+), `auto` is used for **type inference**, not storage class.

```cpp
#include <iostream>
using namespace std;

void function() {
    // Old C++ - explicitly auto (no longer valid in C++11+)
    // auto int x = 10;

    int y = 20;       // Implicitly auto (default for local variables)

    // Modern C++11+ usage:
    auto value = 42;        // Compiler deduces type as int
    auto price = 19.99;     // Compiler deduces type as double
    auto name = "Alice";    // Compiler deduces type as const char*

    cout << value << ", " << price << ", " << name << endl;
}
```

**Properties:**

- **Scope:** Local to the block `{}`
- **Lifetime:** Until block exits
- **Default:** Garbage value (must initialize!)
- **Memory:** Stack

**Type Deduction Rules:**

```cpp
auto x = 5;         // int
auto y = 5.0;       // double
auto z = 5.0f;      // float
auto s = "hello";   // const char*
auto v = {1, 2, 3}; // std::initializer_list<int>
```

### 2.3 Static Storage Class

**Purpose:** Preserve variable value between function calls.

```cpp
#include <iostream>
using namespace std;

void counter() {
    static int count = 0;  // Initialized only ONCE
    count++;
    cout << "Count: " << count << endl;
}

int main() {
    counter();  // Count: 1
    counter();  // Count: 2 (value preserved!)
    counter();  // Count: 3
    return 0;
}
```

**Use cases:**

- Counting function calls
- Caching computed values
- Singleton pattern implementation

**Static in different contexts:**

```cpp
// 1. Static local variable
void func() {
    static int x = 0;  // Preserved between calls
    x++;
}

// 2. Static global variable (internal linkage - file scope only)
static int globalVar = 100;  // Not visible to other files

// 3. Static class member (covered in OOP section)
class MyClass {
    static int count;  // Shared across all instances
};
```

**Internal vs External Linkage:**

- **Static global:** Internal linkage (private to file)
- **Non-static global:** External linkage (visible across files)

### 2.4 Extern Storage Class

**Purpose:** Declare a variable defined in another file (external linkage).

```cpp
// file1.cpp
int globalCounter = 0;  // Definition

// file2.cpp
extern int globalCounter;  // Declaration (tells compiler it exists elsewhere)

void increment() {
    globalCounter++;  // Accesses the variable from file1.cpp
}
```

**Use cases:**

- Sharing global variables across multiple files
- Large projects with modular code

**Important:** Declare with `extern`, define without it (only once).

**Example - Multi-file project:**

```cpp
// constants.cpp
int MAX_SIZE = 100;
double PI = 3.14159;

// main.cpp
extern int MAX_SIZE;
extern double PI;

int main() {
    int array[MAX_SIZE];
    double circumference = 2 * PI * 5;
    return 0;
}
```

### 2.5 Register Storage Class

**Purpose:** Suggest to compiler to store variable in CPU register for faster access.

```cpp
#include <iostream>
using namespace std;

int main() {
    register int counter;  // Hint to compiler

    for (counter = 0; counter < 1000000; counter++) {
        // Fast loop iterations (if register is available)
    }

    // Cannot take address of register variable
    // int* ptr = &counter;  // ❌ Compile error

    return 0;
}
```

**Limitations:**

- Cannot use `&` (address-of operator)
- Compiler may ignore the hint
- Modern compilers optimize automatically (rarely needed today)

**When to use:** Almost never in modern C++. Compilers are better at register allocation.

### 2.6 Mutable Storage Class

**Purpose:** Allow modification of class member even in `const` objects.

```cpp
#include <iostream>
using namespace std;

class Cache {
public:
    int value;
    mutable int accessCount;  // Can be modified even in const objects

    Cache() : value(10), accessCount(0) {}

    int getValue() const {
        accessCount++;  // OK - mutable allows this
        return value;
    }
};

int main() {
    const Cache cache;  // const object
    cout << cache.getValue() << endl;  // accessCount incremented
    cout << cache.getValue() << endl;  // accessCount = 2
    return 0;
}
```

**Use cases:**

- Logging operations in const methods
- Caching results in const functions
- Mutex locks in const methods (thread safety)

### 2.7 Thread-Local Storage Class (C++11)

**Purpose:** Each thread gets its own copy of the variable.

```cpp
#include <iostream>
#include <thread>
using namespace std;

thread_local int threadID = 0;  // Separate copy per thread

void printID(int id) {
    threadID = id;  // Each thread modifies its own copy
    cout << "Thread " << threadID << endl;
}

int main() {
    thread t1(printID, 1);
    thread t2(printID, 2);

    t1.join();
    t2.join();

    return 0;
}
```

**Use cases:**

- Thread-specific data without mutex overhead
- Performance counters per thread
- Random number generators per thread

---

## 3. Variable Scope & Lifetime

**Definition:**

- **Scope:** The region of code where a variable is accessible by its name.
- **Lifetime:** The duration during which a variable exists in memory.

**Important:** Scope ≠ Lifetime

### 3.1 Types of Scope

### Local Scope (Block Scope)

```cpp
#include <iostream>
using namespace std;

int main() {
    int x = 10;  // Local to main()

    {
        int y = 20;  // Local to this block
        cout << x << ", " << y << endl;  // Both accessible
    }

    // cout << y;  // ❌ Error: y not accessible here

    return 0;
}
```

**Properties:**

- Accessible only within `{ }`
- Created when block starts
- Destroyed when block ends

### Function Scope

```cpp
void myFunction() {
    int localVar = 5;  // Function scope
    // Accessible anywhere within myFunction
}
// localVar not accessible here
```

### Global Scope

```cpp
#include <iostream>
using namespace std;

int globalVar = 100;  // Global variable

void display() {
    cout << globalVar << endl;  // Accessible from any function
}

int main() {
    cout << globalVar << endl;  // Accessible here too
    display();
    return 0;
}
```

**Properties:**

- Accessible from any function in the file
- Lifetime: Entire program execution
- Initialized before `main()` starts

### Class Scope

```cpp
class MyClass {
    int x;  // Class scope (accessible within class)

public:
    void setX(int val) {
        x = val;  // Accessible in member functions
    }
};
```

### Namespace Scope

```cpp
namespace MyNamespace {
    int value = 42;  // Namespace scope
}

int main() {
    cout << MyNamespace::value << endl;  // Access using ::
    return 0;
}
```

### 3.2 Variable Shadowing

```cpp
#include <iostream>
using namespace std;

int x = 10;  // Global

int main() {
    int x = 20;  // Shadows global x

    cout << x << endl;    // 20 (local x)
    cout << ::x << endl;  // 10 (global x, accessed with ::)

    {
        int x = 30;  // Shadows local x
        cout << x << endl;  // 30
    }

    cout << x << endl;  // 20 (back to local x)
    return 0;
}
```

**Best Practice:** Avoid shadowing - it reduces code clarity.

### 3.3 Storage Duration

| Storage Duration | Description | Examples |
| --- | --- | --- |
| **Automatic** | Created when scope entered, destroyed when exited | Local variables |
| **Static** | Exists for program lifetime | Static variables, globals |
| **Dynamic** | Manually controlled with `new`/`delete` | Heap allocations |
| **Thread** | Per-thread lifetime | `thread_local` variables |

```cpp
void example() {
    int auto_var = 10;          // Automatic storage
    static int static_var = 20; // Static storage
    int* dynamic_var = new int(30); // Dynamic storage

    delete dynamic_var;
}
```

---

## 4. Data Types & Modifiers

### 4.1 Primitive Data Types

C++ provides fundamental data types to store different kinds of values.

### Character Types

```cpp
#include <iostream>
using namespace std;

int main() {
    char c = 'A';           // 1 byte, ASCII character
    wchar_t wc = L'あ';     // Wide character (2 or 4 bytes)
    char16_t c16 = u'€';    // UTF-16 character (2 bytes)
    char32_t c32 = U'😀';   // UTF-32 character (4 bytes)

    cout << "char: " << c << " (size: " << sizeof(c) << ")" << endl;
    cout << "wchar_t size: " << sizeof(wc) << " bytes" << endl;
    cout << "char16_t size: " << sizeof(c16) << " bytes" << endl;
    cout << "char32_t size: " << sizeof(c32) << " bytes" << endl;

    return 0;
}
```

**Use cases:**

- `char`: Standard ASCII text
- `wchar_t`: International characters (locale-dependent)
- `char16_t`/`char32_t`: Unicode text (C++11)

### Integer Types

```cpp
#include <iostream>
#include <climits>
using namespace std;

int main() {
    short s = 100;           // 2 bytes
    int i = 10000;           // 4 bytes (typical)
    long l = 100000L;        // 4 or 8 bytes
    long long ll = 10000000000LL;  // 8 bytes (C++11)

    cout << "short: " << sizeof(s) << " bytes" << endl;
    cout << "int: " << sizeof(i) << " bytes" << endl;
    cout << "long: " << sizeof(l) << " bytes" << endl;
    cout << "long long: " << sizeof(ll) << " bytes" << endl;

    // Integer ranges
    cout << "\nInteger Ranges:" << endl;
    cout << "int min: " << INT_MIN << endl;
    cout << "int max: " << INT_MAX << endl;
    cout << "unsigned int max: " << UINT_MAX << endl;

    return 0;
}
```

**Typical Sizes (64-bit systems):**

- `short`: 2 bytes (-32,768 to 32,767)
- `int`: 4 bytes (-2,147,483,648 to 2,147,483,647)
- `long`: 4 or 8 bytes (platform-dependent)
- `long long`: 8 bytes (at least)

### Floating-Point Types

```cpp
#include <iostream>
#include <iomanip>
#include <cfloat>
using namespace std;

int main() {
    float f = 3.14159f;      // 4 bytes, ~7 decimal digits
    double d = 3.14159265359; // 8 bytes, ~15 decimal digits
    long double ld = 3.14159265358979323846L; // 12-16 bytes, ~18+ digits

    cout << fixed << setprecision(20);
    cout << "float:       " << f << " (size: " << sizeof(f) << ")" << endl;
    cout << "double:      " << d << " (size: " << sizeof(d) << ")" << endl;
    cout << "long double: " << ld << " (size: " << sizeof(ld) << ")" << endl;

    // Ranges
    cout << "\nFloat Ranges:" << endl;
    cout << "float min: " << FLT_MIN << endl;
    cout << "float max: " << FLT_MAX << endl;
    cout << "double min: " << DBL_MIN << endl;
    cout << "double max: " << DBL_MAX << endl;

    return 0;
}
```

**Precision comparison:**

- `float`: 1.2e-38 to 3.4e+38 (7 digits precision)
- `double`: 2.3e-308 to 1.7e+308 (15 digits precision)
- `long double`: Extended precision (platform-dependent)

### Boolean Type

```cpp
#include <iostream>
using namespace std;

int main() {
    bool isTrue = true;   // 1 byte
    bool isFalse = false;

    cout << "true: " << isTrue << " (size: " << sizeof(isTrue) << ")" << endl;
    cout << "false: " << isFalse << endl;

    // Boolean context conversions
    cout << "\nBoolean Conversions:" << endl;
    cout << "0 as bool: " << (bool)0 << endl;      // false
    cout << "1 as bool: " << (bool)1 << endl;      // true
    cout << "42 as bool: " << (bool)42 << endl;    // true (any non-zero)
    cout << "-5 as bool: " << (bool)-5 << endl;    // true

    return 0;
}
```

### Void Type

```cpp
void printMessage() {  // void return type - no value returned
    cout << "Hello!" << endl;
}

void* genericPointer;  // void pointer - can point to any type
```

### 4.2 Type Modifiers

Modifiers change the meaning of base types.

### Signed vs Unsigned

```cpp
#include <iostream>
#include <climits>
using namespace std;

int main() {
    signed int si = -100;      // Can be negative
    unsigned int ui = 100;     // Only positive

    cout << "signed int range: " << INT_MIN << " to " << INT_MAX << endl;
    cout << "unsigned int range: 0 to " << UINT_MAX << endl;

    // Danger: unsigned overflow
    unsigned int x = 0;
    x--;  // Wraps around!
    cout << "0 - 1 (unsigned): " << x << endl;  // 4294967295

    return 0;
}
```

**Use cases:**

- `unsigned`: Sizes, counts (never negative)
- `signed`: Temperatures, coordinates (can be negative)

### Short and Long

```cpp
short int s = 100;      // 2 bytes
long int l = 100000L;   // 4 or 8 bytes
long long int ll = 10000000000LL;  // At least 8 bytes

// Can be combined
unsigned long long ull = 18446744073709551615ULL;
```

### 4.3 Type Properties

```cpp
#include <iostream>
#include <limits>
using namespace std;

int main() {
    cout << "int min: " << numeric_limits<int>::min() << endl;
    cout << "int max: " << numeric_limits<int>::max() << endl;
    cout << "int digits: " << numeric_limits<int>::digits << endl;
    cout << "int is signed: " << numeric_limits<int>::is_signed << endl;

    cout << "\ndouble min: " << numeric_limits<double>::min() << endl;
    cout << "double max: " << numeric_limits<double>::max() << endl;
    cout << "double digits10: " << numeric_limits<double>::digits10 << endl;

    return 0;
}
```

### 4.4 Type Safety

C++ is **strongly typed**: type must be specified and cannot change.

```cpp
int x = 10;
// x = "hello";  // ❌ Compile error: cannot assign string to int

double d = 3.14;
int i = d;  // ⚠️ Compiles but data loss (narrowing conversion)
```

---

## Summary

### Key Takeaways

1. **Variables** - Named memory locations with specific types
2. **Constants** - Use `const` for runtime, `constexpr` for compile-time
3. **Storage Classes** - Control scope, lifetime, and visibility
    - `auto`: Local variables (type inference in C++11+)
    - `static`: Persistent across calls
    - `extern`: Share across files
    - `register`: CPU register hint (obsolete)
    - `mutable`: Modify in const methods
    - `thread_local`: Per-thread data
4. **Scope** - Region where variable is accessible
5. **Data Types** - Fundamental types for different value kinds
    - Character: `char`, `wchar_t`, `char16_t`, `char32_t`
    - Integer: `short`, `int`, `long`, `long long`
    - Floating: `float`, `double`, `long double`
    - Boolean: `bool`
    - Void: `void`
6. **Type Modifiers** - `signed`, `unsigned`, `short`, `long`

### Interview Points

**Q: Difference between `const` and `constexpr`?**

- `const`: Value cannot change after initialization (runtime or compile-time)
- `constexpr`: Evaluated at compile-time, must be computable at compile time

**Q: When to use `static` keyword?**

- Static local: Preserve value between function calls
- Static global: File-private (internal linkage)
- Static class member: Shared across instances

**Q: What is variable shadowing?**

- Inner scope variable hides outer scope variable with same name
- Access global with `::` operator
- Avoid for code clarity

**Q: Difference between `char` and `wchar_t`?**

- `char`: 1 byte, ASCII/UTF-8
- `wchar_t`: 2-4 bytes, wide characters for international text

---