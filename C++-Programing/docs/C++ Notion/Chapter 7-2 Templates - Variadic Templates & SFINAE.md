# 7.2. Templates - Variadic Templates & SFINAE

---

## Table of Contents

1. Variadic Templates
2. Fold Expressions (C++17)
3. SFINAE - Substitution Failure Is Not An Error
4. Summary
5. Interview Preparation

---

## 1. Variadic Templates

### 1.1 What are Variadic Templates?

**Variadic templates** allow functions and classes to accept a variable number of template arguments of different types. Introduced in C++11, they enable powerful generic programming patterns.

```cpp
// WHY: Single function handles any number of arguments
template <typename... Args>
void print(Args... args) {
    // Function body
}

// Usage
print(1, 2.5, "hello", 'A');  // Any number and types
```

**Why Variadic Templates Matter:**

Before C++11, you needed separate overloads for different argument counts:

```cpp
// Old approach - tedious and limited
void print(int a);
void print(int a, int b);
void print(int a, int b, int c);
// ... can't handle infinite arguments!
```

With variadic templates:

```cpp
// Modern approach - handles any number of arguments
template <typename... Args>
void print(Args... args) {
    // Single implementation
}
```

### 1.2 Parameter Packs

**Parameter pack** is a template parameter that accepts zero or more template arguments:

```cpp
#include <iostream>
using namespace std;

// WHY: ... creates parameter pack, ...args expands it
template <typename T>
void print(T value) {
    // WHY: Base case - single value left
    cout << value << endl;
}

template <typename T, typename... Args>
void print(T first, Args... rest) {
    // WHY: Print first, recursively handle rest
    cout << first << " ";
    print(rest...);  // Pack expansion
}

int main() {
    print(1, 2.5, "hello", 'A');
    return 0;
}
```

**Output:**

```
1 2.5 hello A
```

**How It Works:**

1. `print(1, 2.5, "hello", 'A')` → T=int, Args={double, const char*, char}
2. Print 1, call `print(2.5, "hello", 'A')`
3. Print 2.5, call `print("hello", 'A')`
4. Print "hello", call `print('A')`
5. Print 'A' (base case)

**Understanding Parameter Packs:**

There are two types of packs:

1. **Template parameter pack** - `typename... Args` or `class... Args`
2. **Function parameter pack** - `Args... args`

```cpp
template <typename... Types>  // Types is template parameter pack
void function(Types... args) { // args is function parameter pack
    // Use packs here
}
```

### 1.3 sizeof... Operator

Get the number of elements in a parameter pack at compile time:

```cpp
#include <iostream>
using namespace std;

template <typename... Args>
void count_args(Args... args) {
    // WHY: sizeof... returns number of arguments at compile time
    cout << "Number of arguments: " << sizeof...(Args) << endl;
    cout << "Number of arguments: " << sizeof...(args) << endl;  // Same
}

int main() {
    count_args();                    // 0 arguments
    count_args(1, 2, 3);            // 3 arguments
    count_args("a", "b", "c", "d"); // 4 arguments
    return 0;
}
```

**Output:**

```
Number of arguments: 0
Number of arguments: 3
Number of arguments: 4
```

**Why sizeof... is Useful:**

```cpp
template <typename... Args>
void process(Args... args) {
    // WHY: Different behavior based on argument count
    if constexpr (sizeof...(Args) == 0) {
        cout << "No arguments" << endl;
    } else if constexpr (sizeof...(Args) == 1) {
        cout << "One argument" << endl;
    } else {
        cout << sizeof...(Args) << " arguments" << endl;
    }
}
```

### 1.4 Variadic Class Templates

Classes can also use variadic templates:

```cpp
#include <iostream>
using namespace std;

// WHY: Tuple-like class storing any types
template <typename... Types>
class Tuple;

// Base case - empty tuple
template <>
class Tuple<> {
public:
    Tuple() {
        cout << "Empty tuple created" << endl;
    }
};

// Recursive case - inherits from smaller tuple
template <typename Head, typename... Tail>
class Tuple<Head, Tail...> : private Tuple<Tail...> {
private:
    Head head;

public:
    Tuple(Head h, Tail... t) : Tuple<Tail...>(t...), head(h) {}

    Head getHead() const { return head; }
};

int main() {
    Tuple<int, double, string> t(42, 3.14, "hello");
    cout << "First element: " << t.getHead() << endl;

    return 0;
}
```

**Output:**

```
Empty tuple created
Empty tuple created
Empty tuple created
First element: 42
```

### 1.5 Perfect Forwarding with Variadic Templates

```cpp
#include <iostream>
#include <memory>
using namespace std;

// WHY: Factory function forwards arguments perfectly to constructor
template <typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args) {
    // WHY: std::forward preserves value category (lvalue/rvalue)
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

class Widget {
public:
    Widget(int x, double y, string s) {
        cout << "Widget(" << x << ", " << y << ", " << s << ")" << endl;
    }
};

int main() {
    // WHY: Arguments forwarded perfectly to Widget constructor
    auto widget = make_unique<Widget>(42, 3.14, "test");

    string str = "movable";
    auto widget2 = make_unique<Widget>(100, 2.71, move(str));

    return 0;
}
```

**Output:**

```
Widget(42, 3.14, test)
Widget(100, 2.71, movable)
```

**Why Perfect Forwarding Matters:**

```cpp
// WITHOUT perfect forwarding - copies everything
template <typename T, typename... Args>
unique_ptr<T> bad_make(Args... args) {
    return unique_ptr<T>(new T(args...));  // Always copies!
}

// WITH perfect forwarding - preserves value categories
template <typename T, typename... Args>
unique_ptr<T> good_make(Args&&... args) {
    return unique_ptr<T>(new T(forward<Args>(args)...));  // Moves when possible!
}
```

### 1.6 Practical Examples

**Example 1: Sum All Arguments**

```cpp
#include <iostream>
using namespace std;

// WHY: Base case - no more arguments to sum
auto sum() {
    return 0;
}

// WHY: Recursive case - add first to sum of rest
template <typename T, typename... Args>
auto sum(T first, Args... rest) {
    return first + sum(rest...);
}

int main() {
    cout << "Sum: " << sum(1, 2, 3, 4, 5) << endl;           // 15
    cout << "Sum: " << sum(1.5, 2.5, 3.0) << endl;           // 7.0
    cout << "Sum: " << sum(10) << endl;                      // 10
    cout << "Sum: " << sum() << endl;                        // 0

    return 0;
}
```

**Output:**

```
Sum: 15
Sum: 7
Sum: 10
Sum: 0
```

**Example 2: Print with Custom Separator**

```cpp
#include <iostream>
using namespace std;

// WHY: Base case - last element, no separator after
template <typename T>
void print_sep(const string& sep, T value) {
    cout << value << endl;
}

// WHY: Print with separator between elements
template <typename T, typename... Args>
void print_sep(const string& sep, T first, Args... rest) {
    cout << first << sep;
    print_sep(sep, rest...);
}

int main() {
    print_sep(", ", 1, 2, 3, 4, 5);
    print_sep(" | ", "apple", "banana", "cherry");
    print_sep(" -> ", "start", "middle", "end");

    return 0;
}
```

**Output:**

```
1, 2, 3, 4, 5
apple | banana | cherry
start -> middle -> end
```

**Example 3: Variadic Min/Max**

```cpp
#include <iostream>
using namespace std;

// WHY: Base case - single value is min
template <typename T>
T min(T value) {
    return value;
}

// WHY: Find minimum recursively
template <typename T, typename... Args>
T min(T first, Args... rest) {
    T rest_min = min(rest...);
    return (first < rest_min) ? first : rest_min;
}

template <typename T>
T max(T value) {
    return value;
}

template <typename T, typename... Args>
T max(T first, Args... rest) {
    T rest_max = max(rest...);
    return (first > rest_max) ? first : rest_max;
}

int main() {
    cout << "Min: " << min(5, 2, 8, 1, 9) << endl;
    cout << "Max: " << max(5, 2, 8, 1, 9) << endl;

    return 0;
}
```

**Output:**

```
Min: 1
Max: 9
```

---

## 2. Fold Expressions (C++17)

### 2.1 What are Fold Expressions?

**Fold expressions** provide a concise way to apply binary operators to all elements in a parameter pack, eliminating the need for recursive template functions.

**Before C++17 - Required Recursion:**

```cpp
template <typename T>
auto sum(T value) { return value; }

template <typename T, typename... Args>
auto sum(T first, Args... rest) {
    return first + sum(rest...);  // 5-6 lines minimum
}
```

**C++17+ - Fold Expression:**

```cpp
template <typename... Args>
auto sum(Args... args) {
    return (... + args);  // One line!
}
```

### 2.2 Fold Expression Syntax

**Four types of fold expressions:**

```cpp
(... op pack)           // Unary left fold
(pack op ...)           // Unary right fold
(init op ... op pack)   // Binary left fold
(pack op ... op init)   // Binary right fold
```

**Where:**

- `op` is any binary operator (+, -, *, /, &&, ||, ,, etc.)
- `pack` is an unexpanded parameter pack
- `init` is an initial value

### 2.3 Unary Left Fold

**Syntax:** `(... op pack)`

**Expansion:** `((pack1 op pack2) op pack3) op ...`

```cpp
#include <iostream>
using namespace std;

// WHY: Clean one-liner instead of recursive template
template <typename... Args>
auto sum(Args... args) {
    return (... + args);
}

int main() {
    cout << sum(1, 2, 3, 4, 5) << endl;  // 15

    return 0;
}
```

**How it expands:**

```cpp
// sum(1, 2, 3, 4) with (... + args)
// → (((1 + 2) + 3) + 4)
```

### 2.4 Unary Right Fold

**Syntax:** `(pack op ...)`

**Expansion:** `pack1 op (pack2 op (pack3 op ...))`

```cpp
#include <iostream>
using namespace std;

template <typename... Args>
void print(Args... args) {
    // WHY: Print all arguments using stream operator
    (cout << ... << args) << endl;
}

int main() {
    print("C++", 17, " ", "fold", " ", "expressions");
    return 0;
}
```

**Output:**

```
C++17 fold expressions
```

**How it expands:**

```cpp
// (cout << ... << args) with args={"C++", 17, " "}
// → cout << "C++" << (17 << " ")
```

### 2.5 Binary Left Fold

**Syntax:** `(init op ... op pack)`

**Expansion:** `(((init op pack1) op pack2) op ...)`

```cpp
#include <iostream>
using namespace std;

template <typename... Args>
auto sum_from(int init, Args... args) {
    // WHY: Start from initial value
    return (init + ... + args);
}

int main() {
    cout << sum_from(10, 1, 2, 3) << endl;  // 16 = 10+1+2+3
    cout << sum_from(100) << endl;          // 100

    return 0;
}
```

**Output:**

```
16
100
```

### 2.6 Binary Right Fold

**Syntax:** `(pack op ... op init)`

**Expansion:** `pack1 op (pack2 op (... op init))`

```cpp
#include <iostream>
using namespace std;

template <typename... Args>
auto subtract_from_100(Args... args) {
    // WHY: Subtract all from 100
    return (args - ... - 100);
}

int main() {
    cout << subtract_from_100(10, 5, 2) << endl;  // 10 - (5 - (2 - 100))

    return 0;
}
```

### 2.7 Common Fold Expression Patterns

**Pattern 1: Printing with Separator**

```cpp
#include <iostream>
using namespace std;

template <typename... Args>
void print_with_space(Args... args) {
    // WHY: Fold over comma operator to print each with space
    ((cout << args << " "), ...) << endl;
}

int main() {
    print_with_space(1, 2.5, "hello", 'A');
    return 0;
}
```

**Output:**

```
1 2.5 hello A
```

**Pattern 2: Calling Function on All Elements**

```cpp
#include <iostream>
#include <vector>
using namespace std;

template <typename... Args>
void push_back_all(vector<int>& vec, Args... args) {
    // WHY: Push each argument to vector
    (vec.push_back(args), ...);
}

int main() {
    vector<int> numbers;
    push_back_all(numbers, 1, 2, 3, 4, 5);

    for (int n : numbers) {
        cout << n << " ";
    }
    cout << endl;

    return 0;
}
```

**Output:**

```
1 2 3 4 5
```

**Pattern 3: Logical Operations**

```cpp
#include <iostream>
using namespace std;

template <typename... Args>
bool all_true(Args... args) {
    // WHY: true only if ALL args are true
    return (... && args);
}

template <typename... Args>
bool any_true(Args... args) {
    // WHY: true if ANY arg is true
    return (... || args);
}

int main() {
    cout << boolalpha;
    cout << "all_true(true, true, true): " << all_true(true, true, true) << endl;
    cout << "all_true(true, false, true): " << all_true(true, false, true) << endl;
    cout << "any_true(false, false, true): " << any_true(false, false, true) << endl;
    cout << "any_true(false, false, false): " << any_true(false, false, false) << endl;

    return 0;
}
```

**Output:**

```
all_true(true, true, true): true
all_true(true, false, true): false
any_true(false, false, true): true
any_true(false, false, false): false
```

### 2.8 Fold Expressions vs Recursion

**Comparison:**

| Aspect | Recursive Template | Fold Expression |
| --- | --- | --- |
| Code Length | ~8-10 lines | ~1 line |
| Readability | Requires recursion knowledge | Intuitive |
| Base Case | Required | Not needed |
| Instantiations | Multiple templates | Single template |
| Compile Time | Slower | Faster |
| Error Messages | Nested errors | Simpler |
| C++ Version | C++11+ | C++17+ |

**Example Comparison:**

```cpp
// Recursive approach (pre-C++17)
template <typename T>
auto sum(T value) {
    return value;
}

template <typename T, typename... Args>
auto sum(T first, Args... rest) {
    return first + sum(rest...);
}

// Fold expression (C++17+)
template <typename... Args>
auto sum(Args... args) {
    return (... + args);
}
```

**When to Use Each:**

**Use Fold Expressions:**

- Simple binary operation on all elements
- C++17 or later available
- No special case handling needed
- Want cleaner, more maintainable code

**Use Recursion:**

- Different behavior for first/last element
- Complex conditional logic per element
- C++14 or earlier required
- Need fine-grained control

---

## 3. SFINAE - Substitution Failure Is Not An Error

### 3.1 What is SFINAE?

**SFINAE** is a C++ rule that states: if template argument substitution fails during overload resolution, the compiler doesn't report an error but simply removes that overload from consideration.

**The Principle:** "Substitution Failure Is Not An Error"

**Why SFINAE Matters:**

It allows you to write templates that work differently for different types, or disable templates for certain types, all at compile time.

```cpp
// WHY: Different implementation based on type properties
template <typename T>
enable_if_t<is_integral<T>::value, void>
process(T value) {
    cout << "Processing integer: " << value * 2 << endl;
}

template <typename T>
enable_if_t<is_floating_point<T>::value, void>
process(T value) {
    cout << "Processing float: " << value * 1.5 << endl;
}
```

### 3.2 Basic SFINAE with std::enable_if

**std::enable_if Definition:**

```cpp
template <bool B, typename T = void>
struct enable_if {};

// Specialization only exists when B is true
template <typename T>
struct enable_if<true, T> {
    using type = T;
};
```

**How it works:**

- When `B` is `true`, `enable_if<true, T>::type` exists and equals `T`
- When `B` is `false`, `enable_if<false, T>::type` doesn't exist → SFINAE!

**Basic Usage:**

```cpp
#include <iostream>
#include <type_traits>
using namespace std;

// WHY: Enable only for integral types
template <typename T>
typename enable_if<is_integral<T>::value, T>::type
process(T value) {
    cout << "Processing integral: " << value << endl;
    return value * 2;
}

// WHY: Enable only for floating-point types
template <typename T>
typename enable_if<is_floating_point<T>::value, T>::type
process(T value) {
    cout << "Processing floating-point: " << value << endl;
    return value * 1.5;
}

int main() {
    process(10);      // Calls integral version
    process(3.14);    // Calls floating-point version
    // process("hello");  // ERROR: no matching function

    return 0;
}
```

**Output:**

```
Processing integral: 10
Processing floating-point: 3.14
```

### 3.3 std::enable_if_t (C++14)

**C++14 provides cleaner syntax:**

```cpp
// Before C++14
typename enable_if<condition, T>::type

// C++14+
enable_if_t<condition, T>
```

**Example:**

```cpp
#include <iostream>
#include <type_traits>
using namespace std;

// WHY: _t suffix is cleaner - no ::type needed
template <typename T>
enable_if_t<is_integral<T>::value, void>
print(T value) {
    cout << "Integer: " << value << endl;
}

template <typename T>
enable_if_t<is_floating_point<T>::value, void>
print(T value) {
    cout << "Float: " << value << endl;
}

int main() {
    print(42);
    print(3.14);
    return 0;
}
```

**Output:**

```
Integer: 42
Float: 3.14
```

### 3.4 SFINAE Placement Options

**Option 1: Return Type (Classic)**

```cpp
template <typename T>
typename enable_if<is_integral<T>::value, T>::type
func(T value) {
    return value * 2;
}
```

**Option 2: Template Parameter (Cleaner)**

```cpp
template <typename T,
          typename = enable_if_t<is_integral<T>::value>>
void func(T value) {
    cout << value * 2 << endl;
}
```

**Option 3: Function Parameter (Explicit)**

```cpp
template <typename T>
void func(T value, enable_if_t<is_integral<T>::value>* = nullptr) {
    cout << value * 2 << endl;
}
```

**Multiple Overloads with Template Parameter:**

```cpp
#include <iostream>
#include <type_traits>
using namespace std;

// WHY: First overload for integral types
template <typename T,
          typename = enable_if_t<is_integral<T>::value>>
void process(T value) {
    cout << "Integral: " << value * 2 << endl;
}

// WHY: Second overload for floating types
// Dummy parameter makes signatures different
template <typename T,
          typename = enable_if_t<is_floating_point<T>::value>,
          typename = void>
void process(T value) {
    cout << "Float: " << value * 1.5 << endl;
}

int main() {
    process(10);
    process(3.14);
    return 0;
}
```

**Output:**

```
Integral: 20
Float: 4.71
```

### 3.5 Member Function Detection with SFINAE

**Detecting if a type has a specific member:**

```cpp
#include <iostream>
#include <type_traits>
#include <vector>
using namespace std;

// WHY: Detect if type T has begin() method
template <typename T, typename = void>
struct has_begin : false_type {};

template <typename T>
struct has_begin<T, void_t<decltype(declval<T>().begin())>>
    : true_type {};

// WHY: Use SFINAE to enable only for container types
template <typename T>
enable_if_t<has_begin<T>::value, void>
print_container(const T& container) {
    cout << "Container elements: ";
    for (const auto& elem : container) {
        cout << elem << " ";
    }
    cout << endl;
}

int main() {
    vector<int> vec = {1, 2, 3, 4, 5};
    print_container(vec);  // Works - vector has begin()

    // print_container(42);  // ERROR: int doesn't have begin()

    return 0;
}
```

**Output:**

```
Container elements: 1 2 3 4 5
```

**How Member Detection Works:**

```cpp
// Step 1: Primary template - defaults to false
template <typename T, typename = void>
struct has_begin : false_type {};

// Step 2: Specialization that only works if T has begin()
template <typename T>
struct has_begin<T, void_t<decltype(declval<T>().begin())>>
    : true_type {};

// If T has begin(): specialization selected → true_type
// If T lacks begin(): SFINAE removes specialization → false_type
```

### 3.6 std::void_t Helper

**std::void_t** (C++17) simplifies SFINAE-based member detection:

```cpp
template <typename...>
using void_t = void;
```

**Usage:**

```cpp
#include <iostream>
#include <type_traits>
#include <vector>
#include <list>
using namespace std;

// WHY: void_t helps detect valid expressions
template <typename, typename = void>
struct has_value_type : false_type {};

template <typename T>
struct has_value_type<T, void_t<typename T::value_type>>
    : true_type {};

int main() {
    cout << boolalpha;
    cout << "vector<int> has value_type: "
         << has_value_type<vector<int>>::value << endl;
    cout << "list<double> has value_type: "
         << has_value_type<list<double>>::value << endl;
    cout << "int has value_type: "
         << has_value_type<int>::value << endl;

    return 0;
}
```

**Output:**

```
vector<int> has value_type: true
list<double> has value_type: true
int has value_type: false
```

### 3.7 Practical SFINAE Patterns

**Pattern 1: Safe Division**

```cpp
#include <iostream>
#include <type_traits>
#include <stdexcept>
using namespace std;

// WHY: Only enable for arithmetic types
template <typename T>
enable_if_t<is_arithmetic<T>::value, T>
safe_divide(T a, T b) {
    if (b == 0) {
        throw runtime_error("Division by zero");
    }
    return a / b;
}

int main() {
    cout << safe_divide(10, 2) << endl;
    cout << safe_divide(10.0, 3.0) << endl;

    // safe_divide("hello", "world");  // ERROR: not arithmetic

    try {
        safe_divide(10, 0);
    } catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }

    return 0;
}
```

**Output:**

```
5
3.33333
Error: Division by zero
```

**Pattern 2: Container Size Detection**

```cpp
#include <iostream>
#include <type_traits>
#include <vector>
using namespace std;

// WHY: Detect if type has size() method
template <typename T, typename = void>
struct has_size : false_type {};

template <typename T>
struct has_size<T, void_t<decltype(declval<T>().size())>>
    : true_type {};

// WHY: Get size only for types with size() method
template <typename Container>
auto get_size(const Container& c)
    -> enable_if_t<has_size<Container>::value, size_t> {
    return c.size();
}

int main() {
    vector<int> vec = {1, 2, 3, 4, 5};
    cout << "Vector size: " << get_size(vec) << endl;

    // get_size(42);  // ERROR: int doesn't have size()

    return 0;
}
```

**Output:**

```
Vector size: 5
```

**Pattern 3: Type-Based Serialization**

```cpp
#include <iostream>
#include <type_traits>
#include <string>
using namespace std;

// WHY: Serialize arithmetic types to string
template <typename T>
enable_if_t<is_arithmetic<T>::value, string>
serialize(T value) {
    return to_string(value);
}

// WHY: Serialize strings with quotes
template <typename T>
enable_if_t<is_same<T, string>::value, string>
serialize(const T& value) {
    return "\"" + value + "\"";
}

int main() {
    cout << serialize(42) << endl;
    cout << serialize(3.14) << endl;
    cout << serialize(string("hello")) << endl;

    return 0;
}
```

**Output:**

```
42
3.14
"hello"
```

---

## Summary

### Key Takeaways

1. **Variadic Templates Enable Variable Arguments** - Use `typename... Args` to create functions and classes accepting any number of arguments. Essential for generic programming and perfect forwarding patterns.
2. **Parameter Packs Require Expansion** - Parameter packs (`Args...`) must be expanded with `...` operator. Use recursive patterns with base cases, or leverage fold expressions for simpler code.
3. **sizeof... Gets Pack Size at Compile Time** - The `sizeof...` operator returns the number of elements in a parameter pack, enabling conditional compilation based on argument count.
4. **Perfect Forwarding Preserves Value Categories** - Use `Args&&...` with `std::forward<Args>(args)...` to preserve lvalue/rvalue nature of arguments, essential for wrapper functions and factory patterns.
5. **Fold Expressions Simplify Variadic Code (C++17)** - Replace recursive patterns with concise folds: `(... + args)` is cleaner than recursive templates. Four fold types support different evaluation orders.
6. **SFINAE Controls Template Instantiation** - "Substitution Failure Is Not An Error" enables conditional template instantiation. Failed substitution removes overload from consideration without error.
7. **std::enable_if Implements SFINAE** - Use `enable_if<condition, T>::type` (or `enable_if_t<condition, T>` in C++14+) to conditionally enable templates based on type properties.
8. **Member Detection Uses SFINAE** - Combine `void_t` and `decltype(declval<T>().member())` to detect if types have specific members, enabling type-based dispatch at compile time.
9. **Multiple SFINAE Placement Options** - Place SFINAE in return type, template parameters, or function parameters. Template parameter placement often provides cleanest syntax for overloading.
10. **Fold Expressions vs Recursion Trade-offs** - Folds are cleaner and faster for simple operations. Recursion provides more control for complex patterns. Choose based on C++ version and complexity needs.

---

## Interview Preparation

### Q1: Explain variadic templates and how they work. What is the difference between parameter packs and pack expansion?

**Answer:**

**Variadic templates** allow functions and classes to accept a variable number of template arguments of potentially different types. They were introduced in C++11 and are essential for modern generic programming.

**Parameter Packs:**

A parameter pack is a template parameter that accepts zero or more arguments:

```cpp
template <typename... Args>  // Args is parameter pack
void function(Args... args) {  // args is function parameter pack
    // Use args...
}
```

There are two types:

1. **Template parameter pack** - `typename... Args`
2. **Function parameter pack** - `Args... args`

**Pack Expansion:**

Pack expansion applies a pattern to each element in a pack:

```cpp
template <typename... Args>
void print(Args... args) {
    // Recursive expansion
    print_helper(args...);  // Expands to: print_helper(arg1, arg2, arg3)

    // Fold expansion (C++17)
    ((cout << args << " "), ...);  // Expands each with pattern
}
```

**How Variadic Templates Work - Recursive Pattern:**

```cpp
// Base case - handles last element
template <typename T>
void print(T value) {
    cout << value << endl;
}

// Recursive case - handles first, recurses on rest
template <typename T, typename... Args>
void print(T first, Args... rest) {
    cout << first << " ";
    print(rest...);  // Pack expansion
}

// Call: print(1, 2.5, "hello")
// 1st call: T=int, Args={double, const char*}
//   Print 1, call print(2.5, "hello")
// 2nd call: T=double, Args={const char*}
//   Print 2.5, call print("hello")
// 3rd call: T=const char* (base case)
//   Print "hello"
```

**Key Differences:**

| Aspect | Parameter Pack | Pack Expansion |
| --- | --- | --- |
| What | Template parameter accepting multiple args | Pattern applied to each element |
| Syntax | `typename... Args` | `args...` or fold `(... op args)` |
| Purpose | Declare variadic template | Use pack in code |
| Example | `template <typename... T>` | `f(args...)` or `(args + ...)` |

**sizeof... Operator:**

```cpp
template <typename... Args>
void count(Args... args) {
    cout << sizeof...(Args) << endl;  // Number of types
    cout << sizeof...(args) << endl;  // Number of arguments
}

count(1, 2, 3);  // Prints: 3, 3
```

**Perfect Forwarding with Packs:**

```cpp
template <typename T, typename... Args>
unique_ptr<T> make_unique(Args&&... args) {
    return unique_ptr<T>(new T(forward<Args>(args)...));
}

// forward<Args>(args)... expands to:
// forward<A1>(a1), forward<A2>(a2), forward<A3>(a3)
```

**C++17 Fold Expressions:**

Modern alternative to recursion:

```cpp
// Old way (recursive)
template <typename T>
auto sum(T value) { return value; }

template <typename T, typename... Args>
auto sum(T first, Args... rest) {
    return first + sum(rest...);
}

// New way (fold)
template <typename... Args>
auto sum(Args... args) {
    return (... + args);  // Much simpler!
}
```

---

### Q2: What are fold expressions in C++17? Provide examples of different types and explain when to use them vs recursive templates.

**Answer:**

**Fold expressions** (C++17) provide a concise syntax to apply binary operators to all elements in a parameter pack, eliminating verbose recursive template patterns.

**Four Types of Folds:**

**1. Unary Left Fold: `(... op pack)`**

Expands to: `((pack1 op pack2) op pack3) op ...`

```cpp
template <typename... Args>
auto sum(Args... args) {
    return (... + args);
}

// sum(1, 2, 3, 4) expands to:
// (((1 + 2) + 3) + 4)
```

**2. Unary Right Fold: `(pack op ...)`**

Expands to: `pack1 op (pack2 op (pack3 op ...))`

```cpp
template <typename... Args>
void print(Args... args) {
    (cout << ... << args) << endl;
}

// print("A", "B", "C") expands to:
// cout << "A" << ("B" << "C")
```

**3. Binary Left Fold: `(init op ... op pack)`**

Expands to: `(((init op pack1) op pack2) op ...)`

```cpp
template <typename... Args>
auto sum_from(int init, Args... args) {
    return (init + ... + args);
}

// sum_from(10, 1, 2, 3) expands to:
// ((10 + 1) + 2) + 3 = 16
```

**4. Binary Right Fold: `(pack op ... op init)`**

Expands to: `pack1 op (pack2 op (... op init))`

```cpp
template <typename... Args>
auto concat(Args... args) {
    return (args + ... + string("!"));
}

// concat("A", "B", "C") expands to:
// "A" + ("B" + ("C" + "!"))
```

**Common Fold Patterns:**

**Pattern 1: Comma Operator for Multiple Statements**

```cpp
template <typename... Args>
void print_all(Args... args) {
    ((cout << args << " "), ...);
    cout << endl;
}

// Expands to: ((cout << arg1 << " "), (cout << arg2 << " "), ...)
```

**Pattern 2: Logical Operations**

```cpp
template <typename... Args>
bool all_true(Args... args) {
    return (... && args);  // AND all arguments
}

template <typename... Args>
bool any_true(Args... args) {
    return (... || args);  // OR all arguments
}

all_true(true, true, false);   // false
any_true(false, false, true);  // true
```

**Pattern 3: Function Calls on All Elements**

```cpp
template <typename... Args>
void push_all(vector<int>& v, Args... args) {
    (v.push_back(args), ...);
}

// Calls push_back for each argument
```

**Comparison: Fold vs Recursion**

| Aspect | Recursive Templates | Fold Expressions |
| --- | --- | --- |
| **Code Length** | 6-10 lines | 1 line |
| **Readability** | Requires recursion knowledge | Intuitive operator application |
| **Base Case** | Required | Automatically handled |
| **Compile Time** | Slower (multiple instantiations) | Faster (single instantiation) |
| **Error Messages** | Nested template errors | Simpler errors |
| **Flexibility** | High (custom logic per element) | Limited (same operation) |
| **C++ Version** | C++11+ | C++17+ |

**When to Use Fold Expressions:**

✅ **Use Folds When:**

- Applying same binary operation to all elements
- C++17+ is available
- Want clean, maintainable code
- Simple accumulation or aggregation
- Standard operators (+, -, &&, ||, ,)

```cpp
// Perfect for folds
template <typename... Args>
auto sum(Args... args) { return (... + args); }

template <typename... Args>
void print(Args... args) { ((cout << args << " "), ...); }
```

**When to Use Recursion:**

✅ **Use Recursion When:**

- Different behavior for first/last element
- Complex conditional logic per element
- C++14 or earlier (pre-C++17)
- Need intermediate results
- Custom processing per argument

```cpp
// Better with recursion - different handling for last element
template <typename T>
void print_csv(T value) {
    cout << value << endl;  // Last element, no comma
}

template <typename T, typename... Args>
void print_csv(T first, Args... rest) {
    cout << first << ", ";  // Not last, add comma
    print_csv(rest...);
}

print_csv(1, 2, 3);  // Output: 1, 2, 3
```

**Best Practice:**

Always prefer fold expressions over recursion when they meet your needs—they're clearer, compile faster, and are easier to maintain.

---

### Q3: What is SFINAE? Explain std::enable_if and how it enables conditional template instantiation with practical examples.

**Answer:**

**SFINAE** stands for "Substitution Failure Is Not An Error". It's a C++ language rule that allows the compiler to silently remove template candidates from overload resolution when template parameter substitution fails, rather than causing a compilation error.

**The Core Principle:**

When the compiler substitutes template arguments and encounters an invalid type expression, it doesn't fail compilation—it simply removes that template from the candidate set and tries other overloads.

```cpp
// Example: This doesn't cause error, just removes candidate
template <typename T>
typename T::value_type  // Fails if T has no value_type
func(T t) { return T::value_type(); }

func(42);  // int has no value_type → SFINAE removes this candidate
```

**How std::enable_if Works:**

```cpp
// Definition
template <bool B, typename T = void>
struct enable_if {};  // Primary template is empty

// Specialization only when B is true
template <typename T>
struct enable_if<true, T> {
    using type = T;  // ::type exists only when B is true
};

// Usage
enable_if<true, int>::type   // → int (type exists)
enable_if<false, int>::type  // → ERROR (no ::type member)
```

**When substitution fails (false condition), SFINAE removes that template from consideration.**

**Basic Usage Pattern:**

```cpp
#include <iostream>
#include <type_traits>
using namespace std;

// Enable only for integral types
template <typename T>
typename enable_if<is_integral<T>::value, T>::type
process(T value) {
    cout << "Processing integer: " << value * 2 << endl;
    return value * 2;
}

// Enable only for floating types
template <typename T>
typename enable_if<is_floating_point<T>::value, T>::type
process(T value) {
    cout << "Processing float: " << value * 1.5 << endl;
    return value * 1.5;
}

int main() {
    process(10);      // Calls integer version
    process(3.14);    // Calls float version
    // process("hi");  // ERROR: no matching function
    return 0;
}
```

**C++14 Improvement: enable_if_t**

```cpp
// Instead of: typename enable_if<condition, T>::type
// Use:        enable_if_t<condition, T>

template <typename T>
enable_if_t<is_integral<T>::value, void>
print(T value) {
    cout << "Integer: " << value << endl;
}
```

**Three SFINAE Placement Patterns:**

**Pattern 1: Return Type (Classic)**

```cpp
template <typename T>
typename enable_if<is_arithmetic<T>::value, T>::type
safe_divide(T a, T b) {
    if (b == 0) throw runtime_error("Division by zero");
    return a / b;
}
```

**Pattern 2: Template Parameter (Cleaner)**

```cpp
template <typename T,
          typename = enable_if_t<is_arithmetic<T>::value>>
void safe_divide(T a, T b) {
    if (b == 0) throw runtime_error("Division by zero");
    cout << a / b << endl;
}
```

**Pattern 3: Function Parameter**

```cpp
template <typename T>
void safe_divide(T a, T b,
                 enable_if_t<is_arithmetic<T>::value>* = nullptr) {
    cout << a / b << endl;
}
```

**Practical Examples:**

**Example 1: Member Function Detection**

```cpp
#include <type_traits>
#include <iostream>
using namespace std;

// Detect if type has begin() method
template <typename T, typename = void>
struct has_begin : false_type {};

template <typename T>
struct has_begin<T, void_t<decltype(declval<T>().begin())>>
    : true_type {};

// Enable only for containers with begin()
template <typename T>
enable_if_t<has_begin<T>::value, void>
print_container(const T& container) {
    cout << "Elements: ";
    for (const auto& elem : container) {
        cout << elem << " ";
    }
    cout << endl;
}

// Usage with vector works, with int fails to compile
```

**Example 2: Type-Based Serialization**

```cpp
// Serialize arithmetic types
template <typename T>
enable_if_t<is_arithmetic<T>::value, string>
serialize(T value) {
    return to_string(value);
}

// Serialize strings
template <typename T>
enable_if_t<is_same<T, string>::value, string>
serialize(const T& value) {
    return "\"" + value + "\"";
}

serialize(42);          // "42"
serialize(3.14);        // "3.14"
serialize("hello"s);    // "\"hello\""
```

**Example 3: Conditional Member Functions**

```cpp
template <typename T>
class Container {
public:
    // Only available for arithmetic types
    template <typename U = T>
    enable_if_t<is_arithmetic<U>::value, T>
    sum() const {
        T total = 0;
        for (const auto& elem : data) {
            total += elem;
        }
        return total;
    }

private:
    vector<T> data;
};

Container<int> ci;
ci.sum();  // OK

Container<string> cs;
// cs.sum();  // ERROR: no member sum() for non-arithmetic
```

**Why SFINAE Matters:**

1. **Compile-time polymorphism** - Different code for different types
2. **Type constraints** - Restrict template instantiation
3. **Member detection** - Check if types have certain members
4. **Overload selection** - Choose implementation based on type properties
5. **Library development** - Write generic code that adapts to capabilities

**Limitations:**

- Verbose and hard to read
- Poor error messages
- Difficult to debug
- **Modern alternative: C++20 Concepts** (cleaner, better errors)

```cpp
// SFINAE (hard to read)
template <typename T>
enable_if_t<is_integral<T>::value || is_floating_point<T>::value, T>
multiply(T a, T b) { return a * b; }

// Concepts (clear)
template <typename T>
requires integral<T> || floating_point<T>
T multiply(T a, T b) { return a * b; }
```

---