# 2.6. Lambda Expressions (C++11)

---

## Table of Contents

1. Lambda Fundamentals
2. Capture Modes
3. Generic Lambdas (C++14)
4. Lambda with STL
5. Lambda vs Alternatives

---

## 1. Lambda Fundamentals

### 1.1 What are Lambda Expressions?

**Lambda:** Anonymous function object defined inline, introduced in C++11.

**Purpose:**

- Write short functions where they're used
- Eliminate separate function definitions for simple operations
- Functional programming style
- Callback functions

**Basic Syntax:**

```cpp
[capture](parameters) -> return_type { body }
```

### 1.2 First Lambda Example

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    vector<int> nums = {1, 2, 3, 4, 5};

    // Traditional way - separate function
    // void printNum(int n) { cout << n << " "; }
    // for_each(nums.begin(), nums.end(), printNum);

    // Lambda way - inline anonymous function
    for_each(nums.begin(), nums.end(), [](int n) {
        cout << n << " ";
    });

    return 0;
}
```

**Output:**

```
1 2 3 4 5
```

### 1.3 Lambda Components

```cpp
[capture_clause](parameter_list) mutable -> return_type {
    // function body
}
```

**Components:**

1. **[capture]** - Which variables from outer scope to capture
2. **(parameters)** - Function parameters (like regular function)
3. **mutable** - Optional, allows modifying captured variables
4. **> return_type** - Optional, explicit return type
5. **{ body }** - Function implementation

**Complete Example:**

```cpp
#include <iostream>
using namespace std;

int main() {
    int x = 10, y = 20;

    // [x, y] - capture x and y by value
    // (int a) - parameter
    // -> int - return type
    auto lambda = [x, y](int a) -> int {
        return x + y + a;
    };

    cout << lambda(5) << endl;  // 10 + 20 + 5 = 35

    return 0;
}
```

**Output:**

```
35
```

### 1.4 Return Type Deduction

```cpp
#include <iostream>
using namespace std;

int main() {
    // Compiler deduces return type automatically
    auto add = [](int a, int b) {
        return a + b;  // Deduced as int
    };

    auto divide = [](double a, double b) {
        return a / b;  // Deduced as double
    };

    cout << add(5, 3) << endl;       // 8
    cout << divide(10.0, 3.0) << endl;  // 3.33333

    return 0;
}
```

---

## 2. Capture Modes

### 2.1 No Capture []

**Empty capture:** Lambda cannot access any variables from outer scope.

```cpp
#include <iostream>
using namespace std;

int main() {
    int x = 10;

    auto lambda = []() {
        // cout << x << endl;  // ERROR: x not captured!
        cout << "Hello from lambda!" << endl;
    };

    lambda();
    return 0;
}
```

### 2.2 Capture by Value [=]

**[=]:** Copy all used variables by value.

```cpp
#include <iostream>
using namespace std;

int main() {
    int x = 10, y = 20;

    // Capture all by value (copy)
    auto lambda = [=]() {
        cout << "x: " << x << endl;  // Uses copy of x
        cout << "y: " << y << endl;  // Uses copy of y
    };

    x = 100;  // Change original
    y = 200;

    lambda();  // Still prints original values (10, 20)

    return 0;
}
```

**Output:**

```
x: 10
y: 20
```

### 2.3 Capture by Reference [&]

**[&]:** Capture all used variables by reference.

```cpp
#include <iostream>
using namespace std;

int main() {
    int x = 10, y = 20;

    // Capture all by reference
    auto lambda = [&]() {
        cout << "x: " << x << endl;
        cout << "y: " << y << endl;
        x = 100;  // Modifies original x
        y = 200;  // Modifies original y
    };

    lambda();

    cout << "After lambda:" << endl;
    cout << "x: " << x << endl;  // 100
    cout << "y: " << y << endl;  // 200

    return 0;
}
```

**Output:**

```
x: 10
y: 20
After lambda:
x: 100
y: 200
```

### 2.4 Explicit Captures

```cpp
#include <iostream>
using namespace std;

int main() {
    int a = 1, b = 2, c = 3, d = 4;

    // Capture specific variables
    auto lambda1 = [a, b]() {  // Only a, b by value
        cout << a + b << endl;
    };

    auto lambda2 = [&c, &d]() {  // Only c, d by reference
        c = 30;
        d = 40;
    };

    lambda1();  // 3
    lambda2();

    cout << "c: " << c << ", d: " << d << endl;  // c: 30, d: 40

    return 0;
}
```

**Output:**

```
3
c: 30, d: 40
```

### 2.5 Mixed Captures

```cpp
#include <iostream>
using namespace std;

int main() {
    int x = 10, y = 20, z = 30;

    // [=, &x] - All by value EXCEPT x by reference
    auto lambda1 = [=, &x]() {
        cout << "x (ref): " << x << endl;
        cout << "y (val): " << y << endl;
        cout << "z (val): " << z << endl;
        x = 100;  // Can modify x (reference)
        // y = 200;  // ERROR: y is const (value capture)
    };

    // [&, y] - All by reference EXCEPT y by value
    auto lambda2 = [&, y]() {
        cout << "y (val): " << y << endl;
        x = 50;   // Can modify x (reference)
        z = 70;   // Can modify z (reference)
        // y = 25;  // ERROR: y is const (value capture)
    };

    lambda1();
    cout << "After lambda1, x = " << x << endl;  // 100

    lambda2();
    cout << "After lambda2, x = " << x << ", z = " << z << endl;  // 50, 70

    return 0;
}
```

### 2.6 Capture [this]

**Capture class members:**

```cpp
#include <iostream>
using namespace std;

class Counter {
    int count = 0;

public:
    void increment() {
        // [this] - Capture this pointer
        auto lambda = [this]() {
            count++;  // Access member variable
            cout << "Count: " << count << endl;
        };

        lambda();
    }

    void display() {
        // [*this] - Capture copy of *this (C++17)
        auto lambda = [*this]() {
            cout << "Count copy: " << count << endl;
        };

        lambda();
    }
};

int main() {
    Counter counter;
    counter.increment();  // Count: 1
    counter.increment();  // Count: 2
    counter.display();    // Count copy: 2

    return 0;
}
```

**Output:**

```
Count: 1
Count: 2
Count copy: 2
```

### 2.7 The mutable Keyword

**Problem:** By default, value-captured variables are const.

```cpp
#include <iostream>
using namespace std;

int main() {
    int x = 10;

    // Without mutable - ERROR
    // auto lambda1 = [x]() {
    //     x++;  // ERROR: cannot modify const x
    // };

    // With mutable - OK
    auto lambda2 = [x]() mutable {
        x++;  // OK: can modify copy
        cout << "Inside lambda: " << x << endl;
    };

    lambda2();  // Inside lambda: 11
    lambda2();  // Inside lambda: 12

    cout << "Outside: " << x << endl;  // Still 10 (original unchanged)

    return 0;
}
```

**Output:**

```
Inside lambda: 11
Inside lambda: 12
Outside: 10
```

**Why mutable is needed:**

Lambda generates closure class:

```cpp
// Lambda: [x]() { x++; }
// Equivalent to:
class Lambda {
    int x;  // Captured by value
public:
    Lambda(int _x) : x(_x) {}

    void operator()() const {  // const by default!
        // x++;  // ERROR: can't modify in const function
    }
};

// Lambda: [x]() mutable { x++; }
// Equivalent to:
class Lambda {
    int x;
public:
    Lambda(int _x) : x(_x) {}

    void operator()() {  // NOT const with mutable!
        x++;  // OK
    }
};
```

### 2.8 Capture Initializers (C++14)

```cpp
#include <iostream>
#include <memory>
using namespace std;

int main() {
    // Create new variable in capture clause
    auto lambda1 = [counter = 0]() mutable {
        counter++;
        cout << "Counter: " << counter << endl;
    };

    lambda1();  // Counter: 1
    lambda1();  // Counter: 2
    lambda1();  // Counter: 3

    // Move into lambda
    unique_ptr<int> ptr = make_unique<int>(42);

    auto lambda2 = [p = move(ptr)]() {
        cout << "Value: " << *p << endl;
    };

    lambda2();  // Value: 42
    // ptr is now nullptr (moved)

    return 0;
}
```

**Output:**

```
Counter: 1
Counter: 2
Counter: 3
Value: 42
```

---

## 3. Generic Lambdas (C++14)

### 3.1 Auto Parameters

**Generic lambda:** Use `auto` as parameter type.

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    // Generic lambda - works with any type
    auto print = [](const auto& x) {
        cout << x << endl;
    };

    print(42);              // int
    print(3.14);            // double
    print("Hello");         // const char*
    print(string("World")); // string

    return 0;
}
```

**Output:**

```
42
3.14
Hello
World
```

### 3.2 Generic Operations

```cpp
#include <iostream>
using namespace std;

int main() {
    // Works with any type that supports +
    auto add = [](const auto& a, const auto& b) {
        return a + b;
    };

    cout << add(5, 3) << endl;           // int: 8
    cout << add(2.5, 3.7) << endl;       // double: 6.2
    cout << add(string("Hello "), string("World")) << endl;  // string: Hello World

    return 0;
}
```

**Output:**

```
8
6.2
Hello World
```

### 3.3 Template Lambda (Behind the Scenes)

```cpp
// Generic lambda:
auto lambda = [](auto x, auto y) {
    return x + y;
};

// Compiler generates (approximately):
class Lambda {
public:
    template<typename T1, typename T2>
    auto operator()(T1 x, T2 y) const {
        return x + y;
    }
};
```

---

## 4. Lambda with STL

### 4.1 std::sort

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

struct Person {
    string name;
    int age;
};

int main() {
    vector<Person> people = {
        {"Alice", 30},
        {"Bob", 25},
        {"Charlie", 35}
    };

    // Sort by age using lambda
    sort(people.begin(), people.end(), [](const Person& a, const Person& b) {
        return a.age < b.age;
    });

    for (const auto& p : people) {
        cout << p.name << ": " << p.age << endl;
    }

    return 0;
}
```

**Output:**

```
Bob: 25
Alice: 30
Charlie: 35
```

### 4.2 std::for_each

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    vector<int> nums = {1, 2, 3, 4, 5};

    // Apply operation to each element
    int sum = 0;
    for_each(nums.begin(), nums.end(), [&sum](int n) {
        sum += n;
    });

    cout << "Sum: " << sum << endl;

    return 0;
}
```

**Output:**

```
Sum: 15
```

### 4.3 std::transform

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    vector<int> nums = {1, 2, 3, 4, 5};
    vector<int> squared(nums.size());

    // Transform each element
    transform(nums.begin(), nums.end(), squared.begin(), [](int n) {
        return n * n;
    });

    for (int n : squared) {
        cout << n << " ";
    }
    cout << endl;

    return 0;
}
```

**Output:**

```
1 4 9 16 25
```

### 4.4 std::count_if / std::find_if

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // Count even numbers
    int even_count = count_if(nums.begin(), nums.end(), [](int n) {
        return n % 2 == 0;
    });

    cout << "Even count: " << even_count << endl;

    // Find first number > 5
    auto it = find_if(nums.begin(), nums.end(), [](int n) {
        return n > 5;
    });

    if (it != nums.end()) {
        cout << "First > 5: " << *it << endl;
    }

    return 0;
}
```

**Output:**

```
Even count: 5
First > 5: 6
```

---

## 5. Lambda vs Alternatives

### 5.1 Lambda vs Function Pointers

**Function Pointer:**

```cpp
void printNum(int n) {
    cout << n << " ";
}

int main() {
    vector<int> nums = {1, 2, 3};
    for_each(nums.begin(), nums.end(), printNum);
}
```

**Lambda:**

```cpp
int main() {
    vector<int> nums = {1, 2, 3};
    for_each(nums.begin(), nums.end(), [](int n) {
        cout << n << " ";
    });
}
```

**Comparison:**

| Aspect | Function Pointer | Lambda |
| --- | --- | --- |
| **Definition** | Separate function | Inline definition |
| **State** | Cannot capture state | Can capture variables |
| **Readability** | Function elsewhere | Code where used |
| **Optimization** | Limited | Better (inline) |
| **Flexibility** | Fixed signature | Generic possible |

### 5.2 Lambda vs Functors

**Functor (Function Object):**

```cpp
class Multiplier {
    int factor;
public:
    Multiplier(int f) : factor(f) {}
    int operator()(int x) const {
        return x * factor;
    }
};

int main() {
    vector<int> nums = {1, 2, 3};
    vector<int> result(3);
    transform(nums.begin(), nums.end(), result.begin(), Multiplier(10));
}
```

**Lambda:**

```cpp
int main() {
    int factor = 10;
    vector<int> nums = {1, 2, 3};
    vector<int> result(3);
    transform(nums.begin(), nums.end(), result.begin(), [factor](int x) {
        return x * factor;
    });
}
```

**Comparison:**

| Aspect | Functor | Lambda |
| --- | --- | --- |
| **Syntax** | Verbose class | Concise inline |
| **State** | Member variables | Captured variables |
| **Reusability** | Can be reused | Typically one-time |
| **Naming** | Explicit name | Anonymous |
| **Clarity** | Can be clearer for complex logic | Better for simple operations |

### 5.3 Stateless vs Stateful Lambdas

**Stateless (no captures):**

```cpp
// Stateless - can convert to function pointer
auto lambda = [](int x) { return x * 2; };

int (*funcPtr)(int) = lambda;  // OK - no state
```

**Stateful (with captures):**

```cpp
int factor = 10;
auto lambda = [factor](int x) { return x * factor; };

// int (*funcPtr)(int) = lambda;  // ERROR - has state!
```

**What compiler generates:**

```cpp
// Stateless lambda
auto lambda1 = [](int x) { return x * 2; };

// Generates:
class Lambda1 {
public:
    int operator()(int x) const {
        return x * 2;
    }

    // Can convert to function pointer (no state)
    using FuncPtr = int(*)(int);
    operator FuncPtr() const {
        return [](int x) { return x * 2; };
    }
};

// Stateful lambda
int factor = 10;
auto lambda2 = [factor](int x) { return x * factor; };

// Generates:
class Lambda2 {
    int factor;  // Captured variable
public:
    Lambda2(int f) : factor(f) {}

    int operator()(int x) const {
        return x * factor;
    }

    // CANNOT convert to function pointer (has state)
};
```

### 5.4 std::function

**Storing lambdas:**

```cpp
#include <iostream>
#include <functional>
using namespace std;

int main() {
    // std::function can store any callable
    function<int(int, int)> operation;

    // Store lambda
    operation = [](int a, int b) {
        return a + b;
    };
    cout << "Add: " << operation(5, 3) << endl;

    // Store different lambda
    operation = [](int a, int b) {
        return a * b;
    };
    cout << "Multiply: " << operation(5, 3) << endl;

    // Store stateful lambda
    int factor = 2;
    operation = [factor](int a, int b) {
        return (a + b) * factor;
    };
    cout << "Formula: " << operation(5, 3) << endl;

    return 0;
}
```

**Output:**

```
Add: 8
Multiply: 15
Formula: 16
```

---

## Summary

### Key Takeaways

1. **Lambda basics** - Anonymous inline functions; syntax: `[capture](params) { body }`
2. **Capture modes** - `[]` none, `[=]` by value, `[&]` by reference, `[x,&y]` mixed
3. **[this]** - Capture class members; `[*this]` copies object (C++17)
4. **mutable** - Allows modifying value-captured variables; removes const from operator()
5. **Generic lambdas** - Use `auto` parameters (C++14); compiler generates template
6. **Capture initializers** - `[x=expr]` creates new variable (C++14); enables move captures
7. **STL integration** - Perfect for algorithms: sort, for_each, transform, find_if
8. **Stateless lambdas** - No captures; convertible to function pointers
9. **Stateful lambdas** - With captures; cannot convert to function pointers
10. **std::function** - Type-erased wrapper for storing any callable including lambdas

### Interview Essential Points

**Q: What is a lambda expression in C++?**
A: Lambda is anonymous function object defined inline, introduced in C++11. Syntax: `[capture](parameters) -> return_type { body }`. Purpose: write short functions where used, eliminating separate function definitions. Compiler generates closure class with overloaded operator(). Common use: callbacks, STL algorithms, functional programming.

**Q: Explain different capture modes in lambdas.**
A: (1) `[]` - no capture, (2) `[=]` - capture all used variables by value, (3) `[&]` - capture all by reference, (4) `[x]` - capture specific x by value, (5) `[&x]` - capture specific x by reference, (6) `[=,&x]` - all by value except x by reference, (7) `[&,x]` - all by reference except x by value, (8) `[this]` - capture this pointer, (9) `[*this]` - capture copy of *this (C++17).

**Q: What does the mutable keyword do in lambdas?**
A: By default, lambda's operator() is const, making value-captured variables immutable. `mutable` removes this const, allowing modification of captured copies (not originals). Example: `[x]() mutable { x++; }` can modify copy of x. Reference captures don't need mutable since they can already modify originals.

**Q: What are generic lambdas (C++14)?**
A: Lambdas with `auto` parameters that work with any type. Compiler generates template operator(). Example: `[](auto x) { return x * 2; }` works with int, double, etc. Enables writing type-generic code inline without explicit templates.

**Q: Difference between stateless and stateful lambdas?**
A: Stateless: no captures `[]`, no member variables in closure class, convertible to function pointers. Stateful: with captures, has member variables storing captured values, NOT convertible to function pointers (has state that must be preserved). Stateless more efficient but less powerful.

**Q: How do lambdas compare to functors and function pointers?**
A: Lambdas vs Functors: Lambdas more concise, inline; functors verbose but reusable. Lambdas vs Function Pointers: Lambdas can capture state, better optimization, more flexible; function pointers simpler but can't capture. Lambdas best for one-time simple operations, functors for complex reusable logic, function pointers for C compatibility.

**Q: Can lambdas capture move-only types?**
A: Yes, using init-captures (C++14): `[ptr = std::move(unique_ptr)](){}`. Before C++14, had to wrap in shared_ptr or manually create functor. Init-captures create new member in closure type initialized with expression, enabling move semantics.

**Q: What is the closure type generated by lambda?**
A: Compiler generates anonymous class (closure type) with: (1) Member variables for captures, (2) Constructor initializing captures, (3) operator() with lambda body, (4) const operator() by default (mutable removes this). Example: `[x](int y){ return x+y; }` generates class with int member x and operator()(int y).

---