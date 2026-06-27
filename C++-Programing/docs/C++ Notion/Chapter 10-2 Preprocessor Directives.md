# 10.2. Preprocessor Directives

---

## Table of Contents

1. Introduction to Preprocessor
2. #include Directive
3. #define Macros
4. Macro Functions
5. #undef Directive
6. Conditional Compilation
7. #pragma Directive
8. Predefined Macros
9. Include Guards
10. Macros vs Functions
11. Common Pitfalls
12. Best Practices
13. Summary

---

## 1. Introduction to Preprocessor

### What is the Preprocessor?

**The preprocessor** is a program that processes source code **before compilation**. It handles all directives that start with `#` (hash symbol).

**Think of it as:** A text processor that runs before the compiler sees your code.

### Preprocessing Stages

```
Source Code (.cpp)
      ↓
Preprocessor (#directives processed)
      ↓
Expanded Code (.i)
      ↓
Compiler (actual compilation)
      ↓
Object Code (.o)
```

### Key Characteristics

1. **Text-based**: Works on text, not syntax
2. **Happens before compilation**: Modifies code before compiler sees it
3. **No semicolons**: Directives don't end with `;`
4. **Line-based**: One directive per line (use `\` for continuation)

### Common Use Cases

- **Include headers**: `#include`
- **Define constants**: `#define`
- **Conditional compilation**: `#ifdef`, `#ifndef`
- **Platform-specific code**: Different code for Windows/Linux
- **Debug vs Release**: Enable/disable debugging code

---

## 2. #include Directive

### What is #include?

**#include** replaces the directive with the entire contents of the specified file.

### Two Forms

**Form 1: Angle Brackets `< >`**

```cpp
#include <iostream>  // WHY: System/standard library headers
#include <vector>
#include <string>
```

**WHY angle brackets:**

- Search in **system directories** (e.g., `/usr/include`)
- For standard library and third-party library headers
- Faster search (predefined paths)

**Form 2: Double Quotes `" "`**

```cpp
#include "myheader.h"  // WHY: User-defined headers
#include "utils.hpp"
#include "../common/config.h"
```

**WHY double quotes:**

- Search in **current directory first**, then system directories
- For project-specific headers
- Can specify relative paths

### Example: #include in Action

**myutils.h:**

```cpp
#ifndef MYUTILS_H
#define MYUTILS_H

#include <iostream>

void printMessage() {
    std::cout << "Hello from myutils!" << std::endl;
}

#endif
```

**main.cpp:**

```cpp
#include <iostream>     // WHY: Standard library
#include "myutils.h"    // WHY: Our custom header

int main() {
    printMessage();  // WHY: Function from myutils.h
    return 0;
}
```

**After preprocessing (simplified):**

```cpp
// Contents of <iostream> inserted here...
// ...

// Contents of "myutils.h" inserted here:
#ifndef MYUTILS_H
#define MYUTILS_H
// (iostream already included above)
void printMessage() {
    std::cout << "Hello from myutils!" << std::endl;
}
#endif

int main() {
    printMessage();
    return 0;
}
```

---

## 3. #define Macros

### Object-like Macros

**Syntax:**

```cpp
#define MACRO_NAME replacement_text
```

**WHY:** Create named constants or short code snippets.

### Basic Examples

```cpp
#include <iostream>
using namespace std;

// WHY: Define constants
#define PI 3.14159
#define MAX_SIZE 100
#define COMPANY_NAME "TechCorp"

// WHY: Define code snippets
#define NEWLINE '\n'
#define PRINT(x) cout << x << endl

int main() {
    double radius = 5.0;

    // WHY: PI is replaced with 3.14159 before compilation
    double area = PI * radius * radius;

    cout << "Area: " << area << NEWLINE;
    cout << "Max size: " << MAX_SIZE << endl;
    cout << "Company: " << COMPANY_NAME << endl;

    return 0;
}
```

**Output:**

```
Area: 78.5397
Max size: 100
Company: TechCorp
```

### Multi-line Macros

**Use backslash `\` for continuation:**

```cpp
#include <iostream>
using namespace std;

// WHY: Multi-line macro using backslash
#define MULTI_LINE_MSG "This is a \
very long message that \
spans multiple lines"

#define SWAP(a, b, type) \
    type temp = a; \
    a = b; \
    b = temp;

int main() {
    cout << MULTI_LINE_MSG << endl;

    int x = 10, y = 20;
    cout << "Before: x=" << x << ", y=" << y << endl;

    // WHY: SWAP macro expands to three statements
    SWAP(x, y, int);

    cout << "After: x=" << x << ", y=" << y << endl;

    return 0;
}
```

**Output:**

```
This is a very long message that spans multiple lines
Before: x=10, y=20
After: x=20, y=10
```

### Stringification (#)

**Operator `#` converts macro parameter to string:**

```cpp
#include <iostream>
using namespace std;

// WHY: # converts parameter to string
#define TO_STRING(x) #x
#define PRINT_VAR(var) cout << #var << " = " << var << endl

int main() {
    int count = 42;
    double price = 99.99;

    // WHY: #var becomes "var" (string literal)
    PRINT_VAR(count);   // Prints: count = 42
    PRINT_VAR(price);   // Prints: price = 99.99

    // WHY: Stringification
    cout << TO_STRING(Hello World) << endl;  // Prints: Hello World

    return 0;
}
```

**Output:**

```
count = 42
price = 99.99
Hello World
```

### Token Pasting (##)

**Operator `##` concatenates tokens:**

```cpp
#include <iostream>
using namespace std;

// WHY: ## concatenates tokens
#define CONCAT(a, b) a##b
#define CREATE_VAR(name, num) int name##num = num

int main() {
    // WHY: CONCAT(Hello, World) becomes HelloWorld
    int HelloWorld = 100;
    cout << CONCAT(Hello, World) << endl;  // Prints: 100

    // WHY: Creates variables x1, x2, x3
    CREATE_VAR(x, 1);  // int x1 = 1;
    CREATE_VAR(x, 2);  // int x2 = 2;
    CREATE_VAR(x, 3);  // int x3 = 3;

    cout << "x1=" << x1 << ", x2=" << x2 << ", x3=" << x3 << endl;

    return 0;
}
```

**Output:**

```
100
x1=1, x2=2, x3=3
```

---

## 4. Macro Functions

### Function-like Macros

**Syntax:**

```cpp
#define MACRO_NAME(params) replacement_text
```

**Important:** No space between `MACRO_NAME` and `(`

### Basic Macro Functions

```cpp
#include <iostream>
using namespace std;

// WHY: Define macro functions
#define SQUARE(x) ((x) * (x))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(x) ((x) < 0 ? -(x) : (x))

int main() {
    cout << "SQUARE(5) = " << SQUARE(5) << endl;
    cout << "MAX(10, 20) = " << MAX(10, 20) << endl;
    cout << "ABS(-15) = " << ABS(-15) << endl;

    // WHY: Can use with any type
    cout << "SQUARE(2.5) = " << SQUARE(2.5) << endl;

    return 0;
}
```

**Output:**

```
SQUARE(5) = 25
MAX(10, 20) = 20
ABS(-15) = 15
SQUARE(2.5) = 6.25
```

### WHY Parentheses Matter

**Without parentheses - WRONG:**

```cpp
#include <iostream>
using namespace std;

// BAD: No parentheses
#define SQUARE_BAD(x) x * x

int main() {
    // WHY: This expands to 5 + 1 * 5 + 1 = 11 (WRONG!)
    int result = SQUARE_BAD(5 + 1);

    cout << "Expected: 36, Got: " << result << endl;

    return 0;
}
```

**Output:**

```
Expected: 36, Got: 11
```

**With parentheses - CORRECT:**

```cpp
#include <iostream>
using namespace std;

// GOOD: Parentheses protect from precedence issues
#define SQUARE_GOOD(x) ((x) * (x))

int main() {
    // WHY: Expands to ((5 + 1) * (5 + 1)) = 36 (CORRECT!)
    int result = SQUARE_GOOD(5 + 1);

    cout << "Result: " << result << endl;

    return 0;
}
```

**Output:**

```
Result: 36
```

### Macro Function Pitfalls

**Problem: Side Effects**

```cpp
#include <iostream>
using namespace std;

#define SQUARE(x) ((x) * (x))

int main() {
    int i = 5;

    // WHY: Expands to ((i++) * (i++))
    // i is incremented TWICE! (undefined behavior)
    int result = SQUARE(i++);

    cout << "i = " << i << ", result = " << result << endl;
    // Unpredictable output!

    return 0;
}
```

**Solution: Use inline functions instead (covered later)**

---

## 5. #undef Directive

### What is #undef?

**#undef** undefines a previously defined macro.

**Syntax:**

```cpp
#undef MACRO_NAME
```

### Basic Example

```cpp
#include <iostream>
using namespace std;

#define VERSION 1

int main() {
    cout << "VERSION: " << VERSION << endl;

    // WHY: Undefine VERSION
    #undef VERSION

    // cout << VERSION << endl;  // ERROR: VERSION not defined!

    // WHY: Can redefine after #undef
    #define VERSION 2
    cout << "New VERSION: " << VERSION << endl;

    return 0;
}
```

**Output:**

```
VERSION: 1
New VERSION: 2
```

### Use Case: Redefining Macros

```cpp
#include <iostream>
using namespace std;

// WHY: Initial definition
#define MAX_USERS 10

void oldSystem() {
    cout << "Old system: MAX_USERS = " << MAX_USERS << endl;
}

#undef MAX_USERS  // WHY: Remove old definition

// WHY: New definition for new system
#define MAX_USERS 100

void newSystem() {
    cout << "New system: MAX_USERS = " << MAX_USERS << endl;
}

int main() {
    oldSystem();
    newSystem();

    return 0;
}
```

**Output:**

```
Old system: MAX_USERS = 10
New system: MAX_USERS = 100
```

---

## 6. Conditional Compilation

### #ifdef and #ifndef

**Check if macro is defined:**

```cpp
#include <iostream>
using namespace std;

#define DEBUG  // WHY: Define DEBUG mode

int main() {
    #ifdef DEBUG
        cout << "Debug mode is ON" << endl;
    #endif

    #ifndef RELEASE
        cout << "Release mode is OFF" << endl;
    #endif

    return 0;
}
```

**Output:**

```
Debug mode is ON
Release mode is OFF
```

### #if, #elif, #else

**Check macro values:**

```cpp
#include <iostream>
using namespace std;

#define VERSION 2  // WHY: Set version number

int main() {
    #if VERSION == 1
        cout << "Version 1.0" << endl;
    #elif VERSION == 2
        cout << "Version 2.0" << endl;
    #else
        cout << "Unknown version" << endl;
    #endif

    return 0;
}
```

**Output:**

```
Version 2.0
```

### Practical Example: Debug vs Release

```cpp
#include <iostream>
using namespace std;

// WHY: Uncomment to enable debug
// #define DEBUG

// WHY: Debug macro logs only in debug mode
#ifdef DEBUG
    #define LOG(msg) cout << "[DEBUG] " << msg << endl
#else
    #define LOG(msg)  // WHY: No-op in release
#endif

void processData(int value) {
    LOG("Processing started");  // WHY: Only prints in debug

    int result = value * 2;

    LOG("Result calculated");

    cout << "Result: " << result << endl;
}

int main() {
    processData(10);
    return 0;
}
```

**Output (DEBUG not defined):**

```
Result: 20
```

**Output (DEBUG defined):**

```
[DEBUG] Processing started
[DEBUG] Result calculated
Result: 20
```

### Platform-Specific Code

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: Different code for different platforms
    #ifdef _WIN32
        cout << "Windows OS" << endl;
    #elif __linux__
        cout << "Linux OS" << endl;
    #elif __APPLE__
        cout << "macOS" << endl;
    #else
        cout << "Unknown OS" << endl;
    #endif

    return 0;
}
```

### defined Operator

**Check if macro is defined inline:**

```cpp
#include <iostream>
using namespace std;

#define FEATURE_A
// #define FEATURE_B  // Commented out

int main() {
    // WHY: Use defined() operator
    #if defined(FEATURE_A) && defined(FEATURE_B)
        cout << "Both features enabled" << endl;
    #elif defined(FEATURE_A)
        cout << "Only Feature A enabled" << endl;
    #elif defined(FEATURE_B)
        cout << "Only Feature B enabled" << endl;
    #else
        cout << "No features enabled" << endl;
    #endif

    return 0;
}
```

**Output:**

```
Only Feature A enabled
```

---

## 7. #pragma Directive

### What is #pragma?

**#pragma** provides additional instructions to the compiler (compiler-specific).

**Syntax:**

```cpp
#pragma directive_name

```

### #pragma once

**Most common: Prevent multiple inclusion**

```cpp
// myheader.h
#pragma once  // WHY: Simpler than include guards

void myFunction();
```

**Advantages over include guards:**

- Simpler (one line vs three)
- Less error-prone
- Potentially faster compilation

**Supported by:** GCC, Clang, MSVC (most modern compilers)

### #pragma message

**Print message during compilation:**

```cpp
#pragma message("Compiling with C++17 features")

#ifndef REQUIRED_FEATURE
    #pragma message("Warning: REQUIRED_FEATURE not defined!")
#endif

int main() {
    return 0;
}
```

**During compilation:**

```
note: #pragma message: Compiling with C++17 features
warning: #pragma message: Warning: REQUIRED_FEATURE not defined!
```

### #pragma warning (MSVC)

**Control warnings:**

```cpp
// Disable specific warning
#pragma warning(disable: 4996)  // Disable deprecation warning

// Re-enable warning
#pragma warning(default: 4996)
```

### #pragma pack (MSVC/GCC)

**Control structure alignment:**

```cpp
#include <iostream>
using namespace std;

// WHY: Default alignment
struct Default {
    char c;    // 1 byte + 3 padding
    int i;     // 4 bytes
};

// WHY: Pack with no padding
#pragma pack(push, 1)
struct Packed {
    char c;    // 1 byte
    int i;     // 4 bytes (no padding)
};
#pragma pack(pop)

int main() {
    cout << "Default size: " << sizeof(Default) << endl;
    cout << "Packed size: " << sizeof(Packed) << endl;

    return 0;
}
```

**Output:**

```
Default size: 8
Packed size: 5
```

---

## 8. Predefined Macros

### Standard Predefined Macros

**Always available:**

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: Current source file name
    cout << "File: " << __FILE__ << endl;

    // WHY: Current line number
    cout << "Line: " << __LINE__ << endl;

    // WHY: Compilation date
    cout << "Date: " << __DATE__ << endl;

    // WHY: Compilation time
    cout << "Time: " << __TIME__ << endl;

    // WHY: C++ standard version
    cout << "C++ Standard: " << __cplusplus << endl;

    return 0;
}
```

**Output:**

```
File: main.cpp
Line: 7
Date: Dec  4 2025
Time: 14:30:45
C++ Standard: 201703

```

### __cplusplus Values

| Macro Value | C++ Standard |
| --- | --- |
| `199711L` | C++98/C++03 |
| `201103L` | C++11 |
| `201402L` | C++14 |
| `201703L` | C++17 |
| `202002L` | C++20 |

### Platform Detection Macros

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: Platform detection
    #ifdef _WIN32
        cout << "Windows (32 or 64 bit)" << endl;
    #endif

    #ifdef _WIN64
        cout << "Windows 64 bit" << endl;
    #endif

    #ifdef __linux__
        cout << "Linux" << endl;
    #endif

    #ifdef __APPLE__
        cout << "macOS" << endl;
    #endif

    #ifdef __ANDROID__
        cout << "Android" << endl;
    #endif

    return 0;
}
```

### Compiler Detection

```cpp
#include <iostream>
using namespace std;

int main() {
    #ifdef __GNUC__
        cout << "GCC compiler version: "
             << __GNUC__ << "."
             << __GNUC_MINOR__ << endl;
    #endif

    #ifdef _MSC_VER
        cout << "MSVC compiler version: " << _MSC_VER << endl;
    #endif

    #ifdef __clang__
        cout << "Clang compiler" << endl;
    #endif

    return 0;
}
```

### **func** (Function Name)

```cpp
#include <iostream>
using namespace std;

void myFunction() {
    // WHY: Current function name
    cout << "Function: " << __func__ << endl;
    cout << "Line: " << __LINE__ << endl;
}

int main() {
    cout << "Function: " << __func__ << endl;
    myFunction();

    return 0;
}
```

**Output:**

```
Function: main
Function: myFunction
Line: 6
```

---

## 9. Include Guards

### Problem: Multiple Inclusion

**Without guards:**

```cpp
// myheader.h
struct Data {
    int value;
};

// main.cpp
#include "myheader.h"
#include "myheader.h"  // ERROR: Data redefined!
```

### Solution 1: Traditional Include Guards

```cpp
// myheader.h
#ifndef MYHEADER_H  // WHY: Check if not defined
#define MYHEADER_H  // WHY: Define it

struct Data {
    int value;
};

#endif  // WHY: End of guard
```

**How it works:**

1. First inclusion: `MYHEADER_H` not defined → define it → include content
2. Second inclusion: `MYHEADER_H` already defined → skip content

### Solution 2: #pragma once

```cpp
// myheader.h
#pragma once  // WHY: Simpler, modern approach

struct Data {
    int value;
};
```

**Comparison:**

| Aspect | Include Guards | #pragma once |
| --- | --- | --- |
| **Lines** | 3 | 1 |
| **Errors** | Typo in macro name | None |
| **Speed** | Slightly slower | Slightly faster |
| **Standard** | C++ standard | De facto standard |
| **Portability** | 100% portable | 99% portable |

### Complete Header Example

```cpp
// math_utils.h
#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <cmath>

namespace MathUtils {
    const double PI = 3.14159;

    double square(double x) {
        return x * x;
    }

    double circleArea(double radius) {
        return PI * square(radius);
    }
}

#endif
```

---

## 10. Macros vs Functions

### Comparison Table

| Aspect | Macros | Functions |
| --- | --- | --- |
| **Type checking** | ❌ No | ✅ Yes |
| **Debugging** | ❌ Hard | ✅ Easy |
| **Code size** | ❌ Larger (inline) | ✅ Smaller |
| **Speed** | ✅ Faster | ⚠️ Overhead |
| **Side effects** | ❌ Can have | ✅ Predictable |
| **Scope** | ❌ Global | ✅ Namespace |
| **Recursion** | ❌ No | ✅ Yes |

### Example: Macro Problems

```cpp
#include <iostream>
using namespace std;

// BAD: Macro with side effects
#define MAX_MACRO(a, b) ((a) > (b) ? (a) : (b))

// GOOD: Inline function
inline int maxFunction(int a, int b) {
    return (a > b) ? a : b;
}

int main() {
    int x = 5, y = 10;

    // Macro evaluates arguments multiple times!
    int result1 = MAX_MACRO(x++, y++);
    cout << "After macro: x=" << x << ", y=" << y << endl;
    // x and y incremented MORE than once!

    x = 5; y = 10;

    // Function evaluates arguments once
    int result2 = maxFunction(x++, y++);
    cout << "After function: x=" << x << ", y=" << y << endl;
    // x and y incremented exactly once

    return 0;
}
```

### When to Use Macros

**✅ Good uses:**

- Header guards (`#ifndef`/`#pragma once`)
- Conditional compilation (`#ifdef DEBUG`)
- Platform-specific code
- Constant definitions (though `const` is better)

**❌ Avoid:**

- Complex logic
- Anything with side effects
- Type-dependent operations (use templates instead)

### Modern C++ Alternatives

```cpp
#include <iostream>
using namespace std;

// OLD: Macro
#define PI 3.14159
#define MAX(a,b) ((a)>(b)?(a):(b))

// MODERN: const / constexpr
constexpr double PI_MODERN = 3.14159;

// MODERN: inline function
inline int maxModern(int a, int b) {
    return (a > b) ? a : b;
}

// MODERN: template for generic types
template<typename T>
inline T maxTemplate(T a, T b) {
    return (a > b) ? a : b;
}

int main() {
    cout << "PI: " << PI_MODERN << endl;
    cout << "Max: " << maxModern(5, 10) << endl;
    cout << "Max double: " << maxTemplate(5.5, 10.2) << endl;

    return 0;
}
```

---

## 11. Common Pitfalls

### Pitfall 1: Missing Parentheses

```cpp
// BAD
#define MULTIPLY(a, b) a * b

// Result: 2 + 3 * 4 + 5 = 19 (WRONG!)
int result = MULTIPLY(2 + 3, 4 + 5);

// GOOD
#define MULTIPLY_GOOD(a, b) ((a) * (b))

// Result: (2 + 3) * (4 + 5) = 45 (CORRECT!)
int result2 = MULTIPLY_GOOD(2 + 3, 4 + 5);
```

### Pitfall 2: Semicolon in Macro

```cpp
// BAD: Semicolon in macro definition
#define PRINT(x) cout << x << endl;

if (condition)
    PRINT("Message");  // Extra semicolon causes issues!
else
    PRINT("Other");
```

**Solution:**

```cpp
// GOOD: No semicolon in definition
#define PRINT(x) cout << x << endl

if (condition)
    PRINT("Message");
else
    PRINT("Other");
```

### Pitfall 3: Using Namespace in Headers with Macros

```cpp
// header.h - BAD!
#define SIZE 100
using namespace std;  // Pollutes all including files!
```

### Pitfall 4: Macro Name Collisions

```cpp
// BAD: Generic names
#define MAX 100    // Conflicts with std::max
#define SIZE 50    // Conflicts with size

// GOOD: Specific names
#define MY_BUFFER_MAX 100
#define MY_ARRAY_SIZE 50
```

---

## 12. Best Practices

### ✅ DO: Use UPPERCASE for Macros

```cpp
#define MAX_SIZE 100       // GOOD: Clear it's a macro
#define PI 3.14159         // GOOD
#define DEBUG_MODE         // GOOD
```

### ✅ DO: Use Unique Names

```cpp
#define MYPROJECT_MAX_USERS 100    // GOOD: Project prefix
#define CONFIG_TIMEOUT_MS 5000      // GOOD: Module prefix
```

### ✅ DO: Prefer const/constexpr over #define

```cpp
// OLD
#define MAX_SIZE 100

// MODERN: Type-safe, scoped
constexpr int MAX_SIZE = 100;
```

### ✅ DO: Use #pragma once

```cpp
// Modern, simple
#pragma once

// ... header content ...
```

### ❌ DON'T: Complex Logic in Macros

```cpp
// BAD
#define COMPLEX(x, y) \
    if ((x) > (y)) { \
        result = (x) * 2; \
    } else { \
        result = (y) * 2; \
    }

// GOOD: Use function
inline int calculate(int x, int y) {
    if (x > y) {
        return x * 2;
    } else {
        return y * 2;
    }
}
```

### ❌ DON'T: Macro Functions with Side Effects

```cpp
// BAD
#define INCREMENT(x) (x++)

// GOOD: Regular function
inline void increment(int& x) {
    x++;
}
```

---

## 13. Summary

### Key Takeaways

1. **Preprocessor Runs Before Compilation**
    - Text-based processing
    - No type checking
    - Directives start with `#`
2. **Main Directives**
    - `#include`: Include files
    - `#define`: Define macros
    - `#ifdef`/`#ifndef`: Conditional compilation
    - `#pragma`: Compiler-specific
    - `#undef`: Undefine macros
3. **Include Guards**
    - Traditional: `#ifndef`/`#define`/`#endif`
    - Modern: `#pragma once`
    - Prevent multiple inclusion
4. **Macros vs Functions**
    - Macros: Fast, no type checking, can have issues
    - Functions: Safe, debuggable, type-checked
    - Prefer functions/templates in modern C++
5. **Best Practices**
    - Use UPPERCASE for macros
    - Parenthesize macro parameters
    - Prefer `const`/`constexpr` over `#define`
    - Use `inline` functions instead of function macros
    - Use `#pragma once` for headers

### Quick Reference

| Directive | Purpose | Example |
| --- | --- | --- |
| `#include` | Include file | `#include <iostream>` |
| `#define` | Define macro | `#define PI 3.14` |
| `#undef` | Undefine macro | `#undef PI` |
| `#ifdef` | If defined | `#ifdef DEBUG` |
| `#ifndef` | If not defined | `#ifndef HEADER_H` |
| `#if` | If condition | `#if VERSION == 2` |
| `#elif` | Else if | `#elif VERSION == 3` |
| `#else` | Else | `#else` |
| `#endif` | End if | `#endif` |
| `#pragma` | Compiler directive | `#pragma once` |

### Predefined Macros

| Macro | Value |
| --- | --- |
| `__FILE__` | Current file name |
| `__LINE__` | Current line number |
| `__DATE__` | Compilation date |
| `__TIME__` | Compilation time |
| `__cplusplus` | C++ standard version |
| `__func__` | Current function name |

### Keywords Covered

✅ Preprocessor (2)
✅ #include directive (2)
✅ #define macros (4)
✅ Macro functions (3)
✅ #undef (1)
✅ Conditional compilation (6)
✅ #pragma once (2)
✅ Include guards (2)
✅ Predefined macros (8)
✅ Macro vs inline (2)

**Total: 32 keywords/concepts covered**

---