# 10.10: Modern C++ Features

---

## Table of Contents

1. Overview of Modern C++
2. C++11 Features
3. C++14 Features
4. C++17 Features
5. C++20 Features
6. constexpr
7. Structured Bindings
8. std::optional, std::variant, std::any
9. Ranges and Views
10. Summary

---

## 1. Overview of Modern C++

### Evolution of C++

| Standard | Year | Key Theme |
| --- | --- | --- |
| C++98/03 | 1998/2003 | Original standard |
| C++11 | 2011 | Major modernization |
| C++14 | 2014 | Bug fixes, refinements |
| C++17 | 2017 | Library additions |
| C++20 | 2020 | Concepts, Ranges, Coroutines |
| C++23 | 2023 | Further improvements |

### Why Modern C++?

1. **Safer Code**: Smart pointers, type safety
2. **Cleaner Syntax**: auto, range-based for, lambdas
3. **Better Performance**: Move semantics, constexpr
4. **More Expressive**: Optional, variant, concepts
5. **Easier Concurrency**: Threads, async, futures

---

## 2. C++11 Features

### auto Keyword

```cpp
#include <iostream>
#include <vector>
#include <map>
using namespace std;

int main() {
    // WHY: auto deduces type automatically
    auto i = 42;           // int
    auto d = 3.14;         // double
    auto s = "hello"s;     // std::string (with literal)

    vector<int> vec = {1, 2, 3, 4, 5};

    // WHY: Much cleaner than vector<int>::iterator
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        cout << *it << " ";
    }
    cout << endl;

    map<string, int> ages = {{"Alice", 30}, {"Bob", 25}};

    // WHY: Avoid complex type declarations
    for (const auto& pair : ages) {
        cout << pair.first << ": " << pair.second << endl;
    }

    return 0;
}
```

### Range-Based For Loop

```cpp
#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> numbers = {1, 2, 3, 4, 5};

    // WHY: Cleaner than index-based loops
    for (int n : numbers) {
        cout << n << " ";
    }
    cout << endl;

    // WHY: Reference to modify elements
    for (int& n : numbers) {
        n *= 2;
    }

    // WHY: const reference for read-only
    for (const int& n : numbers) {
        cout << n << " ";
    }
    cout << endl;

    return 0;
}
```

**Output:**

```
1 2 3 4 5
2 4 6 8 10
```

### nullptr

```cpp
#include <iostream>
using namespace std;

void process(int* ptr) {
    if (ptr != nullptr) {
        cout << "Value: " << *ptr << endl;
    } else {
        cout << "Null pointer" << endl;
    }
}

int main() {
    int x = 42;
    int* p1 = &x;
    int* p2 = nullptr;  // WHY: Type-safe null (not 0 or NULL)

    process(p1);
    process(p2);

    return 0;
}
```

### Uniform Initialization

```cpp
#include <iostream>
#include <vector>
using namespace std;

class Point {
public:
    int x, y;
    Point(int x, int y) : x(x), y(y) {}
};

int main() {
    // WHY: Consistent initialization syntax
    int a{5};
    double b{3.14};

    vector<int> vec{1, 2, 3, 4, 5};

    Point p{10, 20};

    // WHY: Prevents narrowing conversions
    // int narrow{3.14};  // ERROR: narrowing

    cout << "a: " << a << ", b: " << b << endl;
    cout << "Point: (" << p.x << ", " << p.y << ")" << endl;

    return 0;
}
```

### Strongly Typed Enums

```cpp
#include <iostream>
using namespace std;

// Old enum (pollutes namespace)
enum OldColor { RED, GREEN, BLUE };

// WHY: enum class provides type safety
enum class Color { Red, Green, Blue };
enum class Size { Small, Medium, Large };

int main() {
    Color c = Color::Red;
    Size s = Size::Large;

    // Color c2 = Red;           // ERROR: must use Color::Red
    // if (c == s) {}            // ERROR: different types
    // int i = Color::Green;     // ERROR: no implicit conversion

    int i = static_cast<int>(Color::Green);  // OK: explicit

    cout << "Color value: " << i << endl;

    return 0;
}
```

### static_assert

```cpp
#include <iostream>
using namespace std;

template<typename T>
class Buffer {
    // WHY: Compile-time check
    static_assert(sizeof(T) <= 64, "Type too large for Buffer");

    T data[100];
public:
    void info() {
        cout << "Buffer size: " << sizeof(data) << " bytes" << endl;
    }
};

int main() {
    Buffer<int> b1;     // OK
    b1.info();

    // Buffer<char[100]> b2;  // ERROR: static_assert fails

    return 0;
}
```

---

## 3. C++14 Features

### Generic Lambdas

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    // WHY: auto parameters make lambda generic
    auto print = [](auto x) {
        cout << x << " ";
    };

    print(42);
    print(3.14);
    print("hello");
    cout << endl;

    // WHY: Generic comparator
    auto compare = [](auto a, auto b) {
        return a < b;
    };

    cout << compare(5, 10) << endl;
    cout << compare(3.14, 2.71) << endl;

    return 0;
}
```

**Output:**

```
42 3.14 hello
1
0
```

### Return Type Deduction

```cpp
#include <iostream>
using namespace std;

// WHY: Compiler deduces return type
auto add(int a, int b) {
    return a + b;  // Returns int
}

auto multiply(double a, double b) {
    return a * b;  // Returns double
}

// WHY: Multiple returns must have same type
auto process(int x) {
    if (x > 0) return x * 2;
    else return x * -1;  // Both return int
}

int main() {
    cout << "add(3, 4) = " << add(3, 4) << endl;
    cout << "multiply(2.5, 4.0) = " << multiply(2.5, 4.0) << endl;
    cout << "process(-5) = " << process(-5) << endl;

    return 0;
}
```

### Binary Literals

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: Binary literals for bit manipulation
    int flags = 0b1010;      // Binary: 10 in decimal
    int mask = 0b11110000;   // Binary: 240 in decimal

    cout << "flags: " << flags << endl;
    cout << "mask: " << mask << endl;

    // WHY: Digit separators for readability
    int million = 1'000'000;
    int binary = 0b1111'0000'1111'0000;

    cout << "million: " << million << endl;
    cout << "binary: " << binary << endl;

    return 0;
}
```

**Output:**

```
flags: 10
mask: 240
million: 1000000
binary: 61680
```

### [[deprecated]] Attribute

```cpp
#include <iostream>
using namespace std;

[[deprecated("Use newFunction() instead")]]
void oldFunction() {
    cout << "Old function" << endl;
}

void newFunction() {
    cout << "New function" << endl;
}

int main() {
    // oldFunction();  // Compiler warning
    newFunction();

    return 0;
}
```

---

## 4. C++17 Features

### if constexpr

```cpp
#include <iostream>
#include <type_traits>
using namespace std;

template<typename T>
auto getValue(T value) {
    // WHY: Compile-time conditional
    if constexpr (is_integral_v<T>) {
        return value * 2;
    } else if constexpr (is_floating_point_v<T>) {
        return value * 1.5;
    } else {
        return value;
    }
}

int main() {
    cout << getValue(10) << endl;      // int: 20
    cout << getValue(10.0) << endl;    // double: 15
    cout << getValue("hi") << endl;    // const char*: hi

    return 0;
}
```

### Fold Expressions

```cpp
#include <iostream>
using namespace std;

// WHY: Sum any number of arguments
template<typename... Args>
auto sum(Args... args) {
    return (args + ...);  // Unary right fold
}

// WHY: Print all arguments
template<typename... Args>
void printAll(Args... args) {
    ((cout << args << " "), ...);  // Fold with comma operator
    cout << endl;
}

// WHY: Check if all are true
template<typename... Args>
bool allTrue(Args... args) {
    return (args && ...);
}

int main() {
    cout << "Sum: " << sum(1, 2, 3, 4, 5) << endl;
    printAll("Hello", 42, 3.14, "World");
    cout << "All true: " << allTrue(true, true, true) << endl;
    cout << "All true: " << allTrue(true, false, true) << endl;

    return 0;
}
```

**Output:**

```
Sum: 15
Hello 42 3.14 World
All true: 1
All true: 0
```

### Inline Variables

```cpp
#include <iostream>
using namespace std;

// WHY: Can be defined in header without ODR violation
struct Config {
    inline static int maxSize = 100;
    inline static string name = "MyApp";
};

int main() {
    cout << "Max size: " << Config::maxSize << endl;
    cout << "Name: " << Config::name << endl;

    Config::maxSize = 200;
    cout << "New max size: " << Config::maxSize << endl;

    return 0;
}
```

### Nested Namespaces

```cpp
#include <iostream>
using namespace std;

// C++17: Nested namespace definition
namespace Company::Project::Module {
    void doSomething() {
        cout << "Doing something in Module" << endl;
    }
}

// Equivalent to (pre-C++17):
// namespace Company { namespace Project { namespace Module { ... }}}

int main() {
    Company::Project::Module::doSomething();
    return 0;
}
```

### std::string_view

```cpp
#include <iostream>
#include <string>
#include <string_view>
using namespace std;

// WHY: Avoid copying strings
void print(string_view sv) {
    cout << "Length: " << sv.length() << ", Content: " << sv << endl;
}

int main() {
    string str = "Hello, World!";
    const char* cstr = "C-style string";

    // WHY: Works with any string-like data without copying
    print(str);
    print(cstr);
    print("Literal");
    print(str.substr(0, 5));

    // WHY: Efficient substring
    string_view sv = str;
    string_view sub = sv.substr(7, 5);  // "World" - no allocation!
    cout << "Substring: " << sub << endl;

    return 0;
}
```

---

## 5. C++20 Features

### Concepts

```cpp
#include <iostream>
#include <concepts>
using namespace std;

// WHY: Constrain template parameters
template<typename T>
concept Numeric = integral<T> || floating_point<T>;

template<Numeric T>
T add(T a, T b) {
    return a + b;
}

// WHY: Alternative syntax
template<typename T>
requires Numeric<T>
T multiply(T a, T b) {
    return a * b;
}

int main() {
    cout << add(5, 3) << endl;       // OK: int is Numeric
    cout << add(2.5, 3.5) << endl;   // OK: double is Numeric
    // add("a", "b");                // ERROR: string not Numeric

    cout << multiply(4, 5) << endl;

    return 0;
}
```

### Three-Way Comparison (Spaceship Operator)

```cpp
#include <iostream>
#include <compare>
using namespace std;

struct Point {
    int x, y;

    // WHY: Single operator provides <, <=, >, >=, ==, !=
    auto operator<=>(const Point&) const = default;
};

int main() {
    Point p1{1, 2};
    Point p2{1, 3};
    Point p3{1, 2};

    cout << "p1 < p2: " << (p1 < p2) << endl;
    cout << "p1 == p3: " << (p1 == p3) << endl;
    cout << "p2 > p1: " << (p2 > p1) << endl;

    return 0;
}
```

### Designated Initializers

```cpp
#include <iostream>
using namespace std;

struct Config {
    int width = 800;
    int height = 600;
    bool fullscreen = false;
    string title = "Window";
};

int main() {
    // WHY: Initialize by member name (order must match!)
    Config c1{.width = 1920, .height = 1080};
    Config c2{.fullscreen = true, .title = "Game"};

    cout << "c1: " << c1.width << "x" << c1.height << endl;
    cout << "c2: " << c2.title << ", fullscreen: " << c2.fullscreen << endl;

    return 0;
}
```

### Modules (Preview)

```cpp
// WHY: Modules replace #include with faster compilation
// This is a preview - actual syntax varies by compiler

// math.ixx (module interface)
// export module math;
// export int add(int a, int b) { return a + b; }

// main.cpp
// import math;
// int result = add(3, 4);

// Benefits:
// - Faster compilation
// - No header/source separation needed
// - No macro leakage
// - Better encapsulation
```

---

## 6. constexpr

### Basic constexpr

```cpp
#include <iostream>
using namespace std;

// WHY: Computed at compile time
constexpr int square(int x) {
    return x * x;
}

constexpr int factorial(int n) {
    return (n <= 1) ? 1 : n * factorial(n - 1);
}

int main() {
    // WHY: Computed at compile time
    constexpr int sq = square(5);      // 25
    constexpr int fact = factorial(5); // 120

    // WHY: Can use in array size
    int arr[square(3)];  // Array of 9 elements

    cout << "square(5) = " << sq << endl;
    cout << "factorial(5) = " << fact << endl;
    cout << "Array size: " << sizeof(arr)/sizeof(int) << endl;

    return 0;
}
```

### constexpr Classes (C++14+)

```cpp
#include <iostream>
using namespace std;

class Rectangle {
    int w, h;
public:
    // WHY: constexpr constructor
    constexpr Rectangle(int width, int height)
        : w(width), h(height) {}

    constexpr int area() const { return w * h; }
    constexpr int perimeter() const { return 2 * (w + h); }
};

int main() {
    // WHY: Object created at compile time
    constexpr Rectangle r(10, 20);

    constexpr int a = r.area();       // 200
    constexpr int p = r.perimeter();  // 60

    cout << "Area: " << a << endl;
    cout << "Perimeter: " << p << endl;

    // WHY: Can use in compile-time contexts
    static_assert(r.area() == 200, "Area mismatch");

    return 0;
}
```

### constexpr if (C++17)

```cpp
#include <iostream>
#include <type_traits>
using namespace std;

template<typename T>
string typeDescription() {
    if constexpr (is_integral_v<T>) {
        return "Integer type";
    } else if constexpr (is_floating_point_v<T>) {
        return "Floating point type";
    } else if constexpr (is_pointer_v<T>) {
        return "Pointer type";
    } else {
        return "Other type";
    }
}

int main() {
    cout << "int: " << typeDescription<int>() << endl;
    cout << "double: " << typeDescription<double>() << endl;
    cout << "int*: " << typeDescription<int*>() << endl;
    cout << "string: " << typeDescription<string>() << endl;

    return 0;
}
```

---

## 7. Structured Bindings

### Basic Structured Bindings

```cpp
#include <iostream>
#include <tuple>
#include <utility>
using namespace std;

int main() {
    // WHY: Decompose pair
    pair<int, string> p{42, "answer"};
    auto [num, text] = p;
    cout << num << ": " << text << endl;

    // WHY: Decompose tuple
    tuple<int, double, string> t{1, 3.14, "pi"};
    auto [i, d, s] = t;
    cout << i << ", " << d << ", " << s << endl;

    return 0;
}
```

### Structured Bindings with Structs

```cpp
#include <iostream>
using namespace std;

struct Point {
    int x, y, z;
};

Point getPoint() {
    return {10, 20, 30};
}

int main() {
    Point p{1, 2, 3};

    // WHY: Decompose struct members
    auto [x, y, z] = p;
    cout << "x=" << x << ", y=" << y << ", z=" << z << endl;

    // WHY: With function return
    auto [a, b, c] = getPoint();
    cout << "a=" << a << ", b=" << b << ", c=" << c << endl;

    return 0;
}
```

### Structured Bindings in Loops

```cpp
#include <iostream>
#include <map>
using namespace std;

int main() {
    map<string, int> ages = {
        {"Alice", 30},
        {"Bob", 25},
        {"Charlie", 35}
    };

    // WHY: Cleaner than pair.first/pair.second
    for (const auto& [name, age] : ages) {
        cout << name << " is " << age << " years old" << endl;
    }

    return 0;
}
```

**Output:**

```
Alice is 30 years old
Bob is 25 years old
Charlie is 35 years old
```

---

## 8. std::optional, std::variant, std::any

### std::optional

```cpp
#include <iostream>
#include <optional>
using namespace std;

// WHY: Represent "maybe has a value"
optional<int> divide(int a, int b) {
    if (b == 0) return nullopt;
    return a / b;
}

int main() {
    auto result1 = divide(10, 2);
    auto result2 = divide(10, 0);

    // WHY: Check if value exists
    if (result1.has_value()) {
        cout << "10/2 = " << result1.value() << endl;
    }

    // WHY: Use value_or for default
    cout << "10/0 = " << result2.value_or(-1) << endl;

    // WHY: Conditional with optional
    if (auto val = divide(20, 4)) {
        cout << "20/4 = " << *val << endl;
    }

    return 0;
}
```

**Output:**

```
10/2 = 5
10/0 = -1
20/4 = 5
```

### std::variant

```cpp
#include <iostream>
#include <variant>
#include <string>
using namespace std;

int main() {
    // WHY: Type-safe union
    variant<int, double, string> v;

    v = 42;
    cout << "int: " << get<int>(v) << endl;

    v = 3.14;
    cout << "double: " << get<double>(v) << endl;

    v = "hello";
    cout << "string: " << get<string>(v) << endl;

    // WHY: Check which type is stored
    cout << "Index: " << v.index() << endl;  // 2 (string)

    // WHY: Safe access with holds_alternative
    if (holds_alternative<string>(v)) {
        cout << "Contains string: " << get<string>(v) << endl;
    }

    return 0;
}
```

### std::any

```cpp
#include <iostream>
#include <any>
#include <string>
using namespace std;

int main() {
    // WHY: Hold any type
    any value;

    value = 42;
    cout << "int: " << any_cast<int>(value) << endl;

    value = 3.14;
    cout << "double: " << any_cast<double>(value) << endl;

    value = string("hello");
    cout << "string: " << any_cast<string>(value) << endl;

    // WHY: Check type
    cout << "Type: " << value.type().name() << endl;

    // WHY: Safe cast with try-catch
    try {
        int i = any_cast<int>(value);  // Will throw!
    } catch (const bad_any_cast& e) {
        cout << "Bad cast: " << e.what() << endl;
    }

    return 0;
}
```

---

## 9. Ranges and Views

### Basic Ranges (C++20)

```cpp
#include <iostream>
#include <vector>
#include <ranges>
#include <algorithm>
using namespace std;

int main() {
    vector<int> numbers = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // WHY: Filter and transform with ranges
    auto result = numbers
        | views::filter([](int n) { return n % 2 == 0; })
        | views::transform([](int n) { return n * n; });

    cout << "Even squares: ";
    for (int n : result) {
        cout << n << " ";
    }
    cout << endl;

    return 0;
}
```

### Range Views

```cpp
#include <iostream>
#include <vector>
#include <ranges>
using namespace std;

int main() {
    vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // WHY: Take first N elements
    cout << "First 5: ";
    for (int n : v | views::take(5)) {
        cout << n << " ";
    }
    cout << endl;

    // WHY: Drop first N elements
    cout << "After 5: ";
    for (int n : v | views::drop(5)) {
        cout << n << " ";
    }
    cout << endl;

    // WHY: Reverse view
    cout << "Reversed: ";
    for (int n : v | views::reverse) {
        cout << n << " ";
    }
    cout << endl;

    return 0;
}
```

---

## 10. Summary

### Features by Standard

| C++11 | C++14 | C++17 | C++20 |
| --- | --- | --- | --- |
| auto | Generic lambdas | Structured bindings | Concepts |
| Range-for | Return type deduction | if constexpr | Ranges |
| nullptr | Binary literals | Fold expressions | Coroutines |
| Lambda | [[deprecated]] | std::optional | Spaceship <=> |
| Move semantics | constexpr relaxed | std::variant | Modules |
| Smart pointers | Variable templates | std::string_view | Designated init |

### When to Use What

| Feature | Use Case |
| --- | --- |
| auto | Reduce type verbosity |
| constexpr | Compile-time computation |
| optional | May or may not have value |
| variant | Type-safe union |
| string_view | Non-owning string reference |
| Concepts | Constrain templates |
| Ranges | Composable algorithms |

### Keywords Covered

✅ auto (5)
✅ constexpr (6)
✅ nullptr (2)
✅ enum class (2)
✅ Range-based for (3)
✅ Uniform initialization (2)
✅ static_assert (2)
✅ Generic lambdas (3)
✅ Binary literals (2)
✅ if constexpr (4)
✅ Fold expressions (3)
✅ Structured bindings (5)
✅ std::optional (4)
✅ std::variant (3)
✅ std::any (2)
✅ std::string_view (3)
✅ Concepts (3)
✅ Ranges (4)
✅ Spaceship operator (2)
✅ Modules (1)

---