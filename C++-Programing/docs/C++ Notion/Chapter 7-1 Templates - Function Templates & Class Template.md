# 7.1. Templates - Function Templates & Class Templates

---

## Table of Contents

1. Introduction to Templates
2. Function Templates
3. Template Type Deduction
4. Non-Type Template Parameters
5. Class Templates
6. Template Specialization
7. Variable Templates (C++14)
8. Summary
9. Interview Preparation

---

## 1. Introduction to Templates

### 1.1 What are Templates?

**Templates are C++'s mechanism for generic programming, allowing code to work with any data type without rewriting for each specific type.**

Templates enable you to write a single function or class that can operate on different data types, improving code reusability and type safety.

### 1.2 Why Templates Exist

**Problem Without Templates:**

```cpp
// WHY: Without templates, we need separate functions for each type
int max_int(int a, int b) {
    return (a > b) ? a : b;
}

double max_double(double a, double b) {
    return (a > b) ? a : b;
}

string max_string(string a, string b) {
    return (a > b) ? a : b;
}

// This is repetitive and hard to maintain!
```

**Solution With Templates:**

```cpp
// WHY: Single template function works for all comparable types
template <typename T>
T max_value(T a, T b) {
    return (a > b) ? a : b;
}

int main() {
    cout << max_value(10, 20) << endl;        // Works with int
    cout << max_value(3.5, 7.2) << endl;      // Works with double
    cout << max_value('A', 'Z') << endl;      // Works with char

    return 0;
}
```

**Output:**

```
20
7.2
Z
```

### 1.3 Templates vs Macros

**Why prefer templates over macros?**

| Feature | Macros | Templates |
| --- | --- | --- |
| **Type Safety** | No type checking | Full type checking |
| **Debugging** | Difficult (text substitution) | Easy (real code) |
| **Scope** | Global only | Respects scope rules |
| **Error Messages** | Cryptic | Clear and helpful |
| **Performance** | Fast (preprocessing) | Fast (inline expansion) |

**Example - Macro Problems:**

```cpp
// WHY: Macros have dangerous side effects
#define MAX(a, b) ((a) > (b) ? (a) : (b))

int x = 5;
int result = MAX(x++, 10);  // x++ evaluated multiple times!
// Result: Undefined behavior

// WHY: Templates are safe
template <typename T>
T max_value(T a, T b) {
    return (a > b) ? a : b;
}

int x = 5;
int result = max_value(x++, 10);  // Safe: x++ evaluated once
```

### 1.4 Benefits of Templates

1. **Code Reusability** - Write once, use with any type
2. **Type Safety** - Compiler catches type errors
3. **Performance** - No runtime overhead (inline expansion)
4. **STL Foundation** - Powers vector, map, algorithms
5. **Compile-Time Computation** - Template metaprogramming

---

## 2. Function Templates

### 2.1 Basic Syntax

```cpp
template <typename T>
return_type function_name(parameters) {
    // Function body using T
}
```

**Components:**

- `template` - Keyword declaring template
- `<typename T>` - Template parameter (T is placeholder)
- `T` - Generic type used in function

### 2.2 typename vs class Keywords

Both keywords work identically in template declarations:

```cpp
// WHY: Both are equivalent - use typename (more intuitive)
template <typename T>  // Modern, preferred
T add(T a, T b) {
    return a + b;
}

template <class T>  // Older style, still valid
T subtract(T a, T b) {
    return a - b;
}
```

**Best Practice:** Use `typename` - it's clearer that T represents a type.

### 2.3 Simple Function Template Example

```cpp
#include <iostream>
using namespace std;

// WHY: Single function works for any type supporting > operator
template <typename T>
T getMax(T a, T b) {
    return (a > b) ? a : b;
}

int main() {
    // WHY: Compiler generates separate function for each type
    cout << "Max of 10, 20: " << getMax(10, 20) << endl;
    cout << "Max of 3.5, 2.1: " << getMax(3.5, 2.1) << endl;
    cout << "Max of 'A', 'Z': " << getMax('A', 'Z') << endl;

    return 0;
}
```

**Output:**

```
Max of 10, 20: 20
Max of 3.5, 2.1: 3.5
Max of 'A', 'Z': Z
```

**How It Works:**

1. Compiler sees `getMax(10, 20)` - generates `int getMax(int, int)`
2. Compiler sees `getMax(3.5, 2.1)` - generates `double getMax(double, double)`
3. Each type gets its own compiled function (template instantiation)

### 2.4 Multiple Template Parameters

```cpp
#include <iostream>
using namespace std;

// WHY: Different types for different parameters
template <typename T1, typename T2>
void display(T1 a, T2 b) {
    cout << "First: " << a << ", Second: " << b << endl;
}

// WHY: Template can return different type than parameters
template <typename T1, typename T2, typename ReturnType>
ReturnType add(T1 a, T2 b) {
    return static_cast<ReturnType>(a + b);
}

int main() {
    display(10, 3.14);           // T1=int, T2=double
    display("Hello", 'A');       // T1=const char*, T2=char

    // WHY: Explicitly specify return type
    double result = add<int, float, double>(5, 3.7f);
    cout << "Result: " << result << endl;

    return 0;
}
```

**Output:**

```
First: 10, Second: 3.14
First: Hello, Second: A
Result: 8.7
```

### 2.5 Template Instantiation and Constraints

**Template instantiation** is when the compiler generates actual functions from templates. The compiler creates specialized code for each type used.

```cpp
template <typename T>
T square(T x) { return x * x; }

int main() {
    square(5);         // Generates: int square(int)
    square(3.14);      // Generates: double square(double)
    square<int>(5);    // Explicit type specification
    return 0;
}
```

**Important:** Templates only work if all operations are valid for the type. For example, `T / T` requires that type T supports division operator.

---

## 3. Template Type Deduction

### 3.1 Automatic Type Deduction

The compiler automatically deduces template types from function arguments:

```cpp
template <typename T>
void print(T value) { cout << value << endl; }

int main() {
    print(42);        // T = int
    print(3.14);      // T = double
    print("Hello");   // T = const char*
    return 0;
}
```

### 3.2 Explicit Template Arguments

When types cannot be deduced, specify them explicitly:

```cpp
template <typename T>
T convert(int value) {
    return static_cast<T>(value);  // Return type can't be deduced
}

int main() {
    double d = convert<double>(42);  // Must specify return type
    float f = convert<float>(42);
    return 0;
}
```

### 3.3 Type Deduction Rules

```cpp
template <typename T>
void process(T param);      // Pass by value

template <typename T>
void processRef(T& param);  // Pass by reference

int x = 10;
const int cx = 20;

// By value - const removed
process(x);    // T = int
process(cx);   // T = int (const discarded)

// By reference - const preserved
processRef(x);   // T = int, param = int&
processRef(cx);  // T = const int, param = const int&
```

**Key Rules:**

- **By value** - const and reference qualifiers removed
- **By reference** - qualifiers preserved
- **By pointer** - pointer qualifiers preserved

---

## 4. Non-Type Template Parameters

### 4.1 Integer Template Parameters

Templates can accept constant values as parameters:

```cpp
#include <iostream>
using namespace std;

// WHY: Size is known at compile time - no dynamic allocation needed
template <typename T, int SIZE>
class StaticArray {
private:
    T arr[SIZE];

public:
    void set(int index, T value) {
        if (index >= 0 && index < SIZE) {
            arr[index] = value;
        }
    }

    T get(int index) {
        return (index >= 0 && index < SIZE) ? arr[index] : T();
    }

    int size() const {
        return SIZE;
    }
};

int main() {
    // WHY: Each size creates a different class type
    StaticArray<int, 5> intArray;
    StaticArray<double, 10> doubleArray;

    intArray.set(0, 100);
    intArray.set(1, 200);

    cout << "Size: " << intArray.size() << endl;
    cout << "Element 0: " << intArray.get(0) << endl;
    cout << "Element 1: " << intArray.get(1) << endl;

    return 0;
}
```

**Output:**

```
Size: 5
Element 0: 100
Element 1: 200
```

### 4.2 Non-Type Parameter Constraints

**Valid non-type parameters:**

- Integral types (int, char, long, etc.)
- Enum types
- Pointers
- References
- nullptr

**Invalid non-type parameters:**

- Floating-point types (float, double)
- Class types
- void

```cpp
// WHY: Must be compile-time constants
template <int N>  // OK: int
class Fixed {};

template <double D>  // ERROR: double not allowed
class Invalid {};

// WHY: Constant expressions work
const int SIZE = 10;
Fixed<SIZE> obj1;      // OK
Fixed<5 + 3> obj2;     // OK: compile-time expression

int runtime_size = 10;
// Fixed<runtime_size> obj3;  // ERROR: not compile-time constant
```

### 4.3 Practical Use Case - Fixed Size Buffer

```cpp
#include <iostream>
using namespace std;

// WHY: Buffer size known at compile time - optimal performance
template <typename T, int CAPACITY>
class Buffer {
private:
    T data[CAPACITY];
    int count;

public:
    Buffer() : count(0) {}

    bool push(T value) {
        // WHY: No dynamic allocation - fast stack-based storage
        if (count < CAPACITY) {
            data[count++] = value;
            return true;
        }
        return false;
    }

    int size() const { return count; }
    int capacity() const { return CAPACITY; }
};

int main() {
    // WHY: Different buffer sizes for different needs
    Buffer<int, 100> smallBuffer;
    Buffer<string, 1000> largeBuffer;

    smallBuffer.push(10);
    smallBuffer.push(20);

    cout << "Small buffer: " << smallBuffer.size()
         << "/" << smallBuffer.capacity() << endl;

    return 0;
}
```

**Output:**

```
Small buffer: 2/100
```

---

## 5. Class Templates

### 5.1 Basic Class Template Syntax

```cpp
template <typename T>
class ClassName {
private:
    T member;

public:
    ClassName(T value);
    T getValue();
};
```

### 5.2 Simple Class Template Example

```cpp
#include <iostream>
using namespace std;

// WHY: Generic container works with any type
template <typename T>
class Box {
private:
    T content;

public:
    // Constructor
    Box(T value) : content(value) {}

    // Getter
    T getContent() const {
        return content;
    }

    // Setter
    void setContent(T value) {
        content = value;
    }

    // Display
    void display() const {
        cout << "Content: " << content << endl;
    }
};

int main() {
    // WHY: Each instantiation creates different class type
    Box<int> intBox(42);
    Box<double> doubleBox(3.14);
    Box<string> stringBox("Hello Templates");

    intBox.display();
    doubleBox.display();
    stringBox.display();

    return 0;
}
```

**Output:**

```
Content: 42
Content: 3.14
Content: Hello Templates
```

### 5.3 Defining Member Functions Outside Class

When defining template class methods outside the class, you must repeat the template declaration:

```cpp
#include <iostream>
using namespace std;

template <typename T>
class Calculator {
private:
    T num1, num2;

public:
    Calculator(T n1, T n2);
    T add();
    T multiply();
};

// WHY: Template declaration needed for each method definition
template <typename T>
Calculator<T>::Calculator(T n1, T n2) : num1(n1), num2(n2) {}

template <typename T>
T Calculator<T>::add() {
    return num1 + num2;
}

template <typename T>
T Calculator<T>::multiply() {
    return num1 * num2;
}

int main() {
    Calculator<int> intCalc(10, 5);
    Calculator<double> doubleCalc(10.5, 2.5);

    cout << "Int add: " << intCalc.add() << endl;
    cout << "Double multiply: " << doubleCalc.multiply() << endl;

    return 0;
}
```

**Output:**

```
Int add: 15
Double multiply: 26.25
```

### 5.4 Multiple Template Parameters in Classes

```cpp
#include <iostream>
using namespace std;

// WHY: Different types for key-value storage
template <typename KeyType, typename ValueType>
class Pair {
private:
    KeyType key;
    ValueType value;

public:
    Pair(KeyType k, ValueType v) : key(k), value(v) {}

    KeyType getKey() const { return key; }
    ValueType getValue() const { return value; }

    void display() const {
        cout << "Key: " << key << ", Value: " << value << endl;
    }
};

int main() {
    // WHY: Flexible key-value combinations
    Pair<int, string> student(101, "Alice");
    Pair<string, double> price("Apple", 1.99);
    Pair<char, int> grade('A', 95);

    student.display();
    price.display();
    grade.display();

    return 0;
}
```

**Output:**

```
Key: 101, Value: Alice
Key: Apple, Value: 1.99
Key: A, Value: 95
```

### 5.5 Default Template Arguments

```cpp
#include <iostream>
using namespace std;

// WHY: Default type makes template easier to use
template <typename T = int, int SIZE = 10>
class Array {
private:
    T arr[SIZE];
    int count;

public:
    Array() : count(0) {}

    void add(T value) {
        if (count < SIZE) {
            arr[count++] = value;
        }
    }

    void display() const {
        for (int i = 0; i < count; i++) {
            cout << arr[i] << " ";
        }
        cout << endl;
    }
};

int main() {
    // WHY: Can use default types or specify custom ones
    Array<> defaultArray;           // Uses int and size 10
    Array<double> doubleArray;      // Uses double and size 10
    Array<char, 5> smallCharArray;  // Custom type and size

    defaultArray.add(1);
    defaultArray.add(2);
    defaultArray.display();

    return 0;
}
```

**Output:**

```
1 2
```

### 5.6 Real-World Example - Generic Stack

```cpp
#include <iostream>
using namespace std;

// WHY: Type-safe stack implementation for any data type
template <typename T, int MAX_SIZE = 100>
class Stack {
private:
    T data[MAX_SIZE];
    int top;

public:
    Stack() : top(-1) {}

    // WHY: Check if stack is full before push
    bool push(T value) {
        if (top >= MAX_SIZE - 1) {
            cout << "Stack overflow!" << endl;
            return false;
        }
        data[++top] = value;
        return true;
    }

    // WHY: Check if stack is empty before pop
    bool pop(T& value) {
        if (top < 0) {
            cout << "Stack underflow!" << endl;
            return false;
        }
        value = data[top--];
        return true;
    }

    bool isEmpty() const {
        return top < 0;
    }

    bool isFull() const {
        return top >= MAX_SIZE - 1;
    }

    int size() const {
        return top + 1;
    }
};

int main() {
    // WHY: Same stack implementation works for different types
    Stack<int, 5> intStack;
    Stack<string, 3> stringStack;

    // Integer stack
    intStack.push(10);
    intStack.push(20);
    intStack.push(30);

    cout << "Int stack size: " << intStack.size() << endl;

    int value;
    while (intStack.pop(value)) {
        cout << "Popped: " << value << endl;
    }

    // String stack
    stringStack.push("First");
    stringStack.push("Second");

    string str;
    while (stringStack.pop(str)) {
        cout << "Popped: " << str << endl;
    }

    return 0;
}
```

**Output:**

```
Int stack size: 3
Popped: 30
Popped: 20
Popped: 10
Popped: Second
Popped: First
```

---

## 6. Template Specialization

### 6.1 What is Template Specialization?

**Template specialization** allows you to provide a custom implementation for specific types while keeping the generic template for others.

### 6.2 Function Template Specialization

```cpp
#include <iostream>
#include <cstring>
using namespace std;

// WHY: Generic template for all types
template <typename T>
T getMax(T a, T b) {
    cout << "Generic template" << endl;
    return (a > b) ? a : b;
}

// WHY: Specialized version for char* (C-strings need strcmp)
template <>
const char* getMax<const char*>(const char* a, const char* b) {
    cout << "Specialized for const char*" << endl;
    return (strcmp(a, b) > 0) ? a : b;
}

int main() {
    cout << "Max of 10, 20: " << getMax(10, 20) << endl;
    cout << "Max of 3.5, 2.1: " << getMax(3.5, 2.1) << endl;

    const char* str1 = "Hello";
    const char* str2 = "World";
    cout << "Max of strings: " << getMax(str1, str2) << endl;

    return 0;
}
```

**Output:**

```
Generic template
Max of 10, 20: 20
Generic template
Max of 3.5, 2.1: 3.5
Specialized for const char*
Max of strings: World
```

### 6.3 Class Template Specialization

**Full Class Specialization:**

```cpp
#include <iostream>
using namespace std;

// WHY: Generic storage for all types
template <typename T>
class Storage {
private:
    T data;

public:
    Storage(T value) : data(value) {}

    void display() {
        cout << "Generic storage: " << data << endl;
    }
};

// WHY: Specialized storage for bool (could use bit packing)
template <>
class Storage<bool> {
private:
    bool data;

public:
    Storage(bool value) : data(value) {}

    void display() {
        cout << "Specialized bool storage: "
             << (data ? "true" : "false") << endl;
    }
};

int main() {
    Storage<int> intStorage(42);
    Storage<double> doubleStorage(3.14);
    Storage<bool> boolStorage(true);

    intStorage.display();
    doubleStorage.display();
    boolStorage.display();

    return 0;
}
```

**Output:**

```
Generic storage: 42
Generic storage: 3.14
Specialized bool storage: true
```

### 6.4 Partial Template Specialization (Classes Only)

Partial specialization allows specializing some template parameters while keeping others generic:

```cpp
#include <iostream>
using namespace std;

// WHY: Generic template for two types
template <typename T1, typename T2>
class Pair {
public:
    void display() {
        cout << "Generic Pair<T1, T2>" << endl;
    }
};

// WHY: Partial specialization - both types same
template <typename T>
class Pair<T, T> {
public:
    void display() {
        cout << "Partial specialization: Pair<T, T>" << endl;
    }
};

// WHY: Partial specialization - second type is int
template <typename T>
class Pair<T, int> {
public:
    void display() {
        cout << "Partial specialization: Pair<T, int>" << endl;
    }
};

int main() {
    Pair<double, string> p1;  // Generic
    Pair<int, int> p2;        // Both same
    Pair<double, int> p3;     // Second is int

    p1.display();
    p2.display();
    p3.display();

    return 0;
}
```

**Output:**

```
Generic Pair<T1, T2>
Partial specialization: Pair<T, T>
Partial specialization: Pair<T, int>
```

**Note:** Function templates cannot be partially specialized, only fully specialized.

### 6.5 When to Use Specialization

**Use template specialization when:**

1. **Type requires different algorithm**
    
    ```cpp
    // Generic comparison uses <
    // char* specialization uses strcmp
    ```
    
2. **Type has special optimizations**
    
    ```cpp
    // bool specialization uses bit packing
    // General types use normal storage
    ```
    
3. **Type has different behavior**
    
    ```cpp
    // Generic pointer arithmetic
    // void* specialization prohibits arithmetic
    ```
    

**Don't overuse specialization:**

- Prefer function overloading for simple cases
- Consider inheritance for complex behaviors
- Keep specialized versions consistent with generic

### 6.6 Specialization vs Overloading

```cpp
#include <iostream>
using namespace std;

// Template
template <typename T>
void process(T value) {
    cout << "Template: " << value << endl;
}

// Full specialization
template <>
void process<int>(int value) {
    cout << "Specialized for int: " << value << endl;
}

// Overloaded function (not template)
void process(double value) {
    cout << "Overloaded function for double: " << value << endl;
}

int main() {
    process(10);      // Specialization
    process(3.14);    // Overloaded function (preferred over template)
    process('A');     // Template

    return 0;
}
```

**Output:**

```
Specialized for int: 10
Overloaded function for double: 3.14
Template: A
```

**Resolution Order:**

1. **Exact match** - Non-template function
2. **Specialization** - Specialized template
3. **Generic template** - Primary template

---

## 7. Variable Templates (C++14)

### 7.1 Basic Syntax and Purpose

**Variable templates** create parameterized constants, typically for type-specific compile-time values:

```cpp
// WHY: Mathematical constant with type-specific precision
template <typename T>
constexpr T pi = T(3.1415926535897932385);

int main() {
    cout << "Pi as float: " << pi<float> << endl;
    cout << "Pi as double: " << pi<double> << endl;
    return 0;
}
```

### 7.2 Practical Applications

**Type Traits Constants:**

```cpp
// WHY: Cleaner syntax for type checking
template <typename T>
constexpr bool is_pointer_v = false;

template <typename T>
constexpr bool is_pointer_v<T*> = true;

// Usage
if constexpr (is_pointer_v<int*>) {
    cout << "Pointer type" << endl;
}
```

**Static Data Members:**

```cpp
class Limits {
public:
    template <typename T>
    static constexpr T max_value = T(100);
};

// Specialization
template <>
constexpr int Limits::max_value<int> = 2147483647;
```

**Why Use Variable Templates:**

- Type-safe compile-time constants
- Cleaner syntax than function calls
- Specialization support
- Zero runtime overhead

---

## Summary

### Key Takeaways

1. **Templates Enable Generic Programming** - Write code once, use with any type. Templates are C++'s primary mechanism for generic programming, reducing code duplication while maintaining type safety.
2. **Function Templates Create Generic Functions** - Use `template <typename T>` to create functions that work with any type. The compiler generates specialized versions for each type used.
3. **Type Deduction is Automatic** - In most cases, the compiler can deduce template parameters from function arguments, making templates easy to use without explicit type specification.
4. **Non-Type Parameters Allow Compile-Time Values** - Templates can accept constant values (like array sizes) as parameters, enabling compile-time optimizations and zero runtime overhead.
5. **Class Templates Create Generic Classes** - Template classes like `vector<T>` and `Stack<T>` provide type-safe containers that work with any data type while maintaining full compile-time type checking.
6. **Template Specialization Provides Custom Implementations** - When a type needs special handling, template specialization allows providing a custom implementation while keeping the generic version for other types.
7. **Partial Specialization Offers Flexibility** - Classes can be partially specialized (some parameters fixed, others generic), but functions cannot - they only support full specialization.
8. **Templates vs Macros** - Templates are superior to macros because they provide type safety, better error messages, scope respect, and easier debugging while maintaining similar performance.
9. **Variable Templates Store Type-Parameterized Constants** - C++14 variable templates allow creating families of related constants (like mathematical constants) with type-specific precision.
10. **Templates Are Compile-Time Constructs** - All template instantiation and specialization happens at compile time, resulting in zero runtime overhead but potentially larger executable sizes.

---

## Interview Preparation

### Q1: What are templates in C++ and why are they used? Compare templates with macros.

**Answer:**

Templates are C++'s mechanism for generic programming that allows writing code to work with any data type without rewriting for each specific type. They enable type-safe, reusable code generation at compile time.

**Why Templates Are Used:**

1. **Code Reusability** - Write once, use with multiple types
2. **Type Safety** - Compiler performs full type checking
3. **Performance** - No runtime overhead, inline expansion possible
4. **STL Foundation** - All STL containers and algorithms use templates
5. **Maintainability** - Single implementation to maintain

**Templates vs Macros Comparison:**

| Aspect | Templates | Macros |
| --- | --- | --- |
| Type Checking | Full type safety | No type checking |
| Debugging | Clear error messages | Cryptic errors |
| Scope | Follows C++ scope rules | Global text substitution |
| Side Effects | Safe evaluation | Can evaluate arguments multiple times |
| Code Generation | Type-specific functions | Text replacement |

**Example Demonstrating Difference:**

```cpp
// Macro - dangerous
#define MAX(a, b) ((a) > (b) ? (a) : (b))
int x = 5;
MAX(x++, 10);  // x++ evaluated twice - undefined behavior!

// Template - safe
template <typename T>
T max_value(T a, T b) {
    return (a > b) ? a : b;
}
int x = 5;
max_value(x++, 10);  // x++ evaluated once - safe
```

**Best Practice:** Always prefer templates over macros for type-generic code due to superior type safety, debugging capability, and maintainability.

---

### Q2: Explain template instantiation and type deduction. When must you explicitly specify template arguments?

**Answer:**

**Template Instantiation** is the process where the compiler generates actual functions or classes from template definitions when they are used with specific types.

**Types of Instantiation:**

1. **Implicit Instantiation** - Compiler automatically generates code when template is used
2. **Explicit Instantiation** - Programmer forces instantiation with specific types
3. **Explicit Specialization** - Custom implementation for specific types

**Type Deduction** is the compiler's ability to automatically determine template parameter types from function arguments:

```cpp
template <typename T>
T add(T a, T b) {
    return a + b;
}

// Implicit deduction
add(5, 10);        // T deduced as int
add(3.14, 2.71);   // T deduced as double

// Explicit specification
add<double>(5, 10);  // T explicitly set to double
```

**When Explicit Specification is Required:**

1. **Type Cannot Be Deduced:**

```cpp
template <typename T>
T create() {
    return T();  // No parameters to deduce from
}
create<int>();  // Must specify type
```

1. **Ambiguous Types:**

```cpp
template <typename T>
T max_value(T a, T b) { return a > b ? a : b; }
// max_value(5, 3.14);  // ERROR: int or double?
max_value<double>(5, 3.14);  // OK: both converted to double
```

1. **Return Type Differs:**

```cpp
template <typename Return, typename T>
Return convert(T value) {
    return static_cast<Return>(value);
}
convert<double>(42);  // Return type must be specified
```

1. **Non-Type Parameters:**

```cpp
template <typename T, int SIZE>
class Array {};
Array<int, 10> arr;  // SIZE cannot be deduced
```

**Deduction Rules:**

- **By value** - const and reference qualifiers removed
- **By reference** - qualifiers preserved
- **By pointer** - pointer qualifiers preserved

---

### Q3: What is template specialization? Explain the difference between full and partial specialization with examples.

**Answer:**

**Template Specialization** provides a way to define custom implementations for specific types while maintaining the generic template for other types. It's useful when certain types require different algorithms or optimizations.

**Full Specialization** - Completely specialized implementation for a specific type:

```cpp
// Generic template
template <typename T>
class Storage {
    T data;
public:
    void process() {
        cout << "Generic processing" << endl;
    }
};

// Full specialization for char*
template <>
class Storage<char*> {
    char* data;
public:
    void process() {
        cout << "Special string processing" << endl;
    }
};
```

**Full Function Template Specialization:**

```cpp
template <typename T>
T getMax(T a, T b) {
    return (a > b) ? a : b;
}

// Specialized for const char* (needs strcmp)
template <>
const char* getMax<const char*>(const char* a, const char* b) {
    return (strcmp(a, b) > 0) ? a : b;
}
```

**Partial Specialization** - Some template parameters fixed, others remain generic (classes only):

```cpp
// Primary template
template <typename T1, typename T2>
class Pair {
    void display() { cout << "Generic Pair" << endl; }
};

// Partial specialization - both types same
template <typename T>
class Pair<T, T> {
    void display() { cout << "Same type Pair" << endl; }
};

// Partial specialization - second type is pointer
template <typename T, typename U>
class Pair<T, U*> {
    void display() { cout << "Second is pointer" << endl; }
};
```

**Key Differences:**

| Aspect | Full Specialization | Partial Specialization |
| --- | --- | --- |
| Template Parameters | All specified | Some specified, some generic |
| Functions | Supported | NOT supported |
| Classes | Supported | Supported |
| Flexibility | Fixed for one type | Pattern matching on types |

**Important Notes:**

1. **Functions Cannot Be Partially Specialized** - Use function overloading instead
2. **Specialization Must Be in Same Namespace** - As primary template
3. **More Specialized Versions Preferred** - Compiler picks most specific match

**When to Use:**

- **Full**: Type needs completely different implementation
- **Partial**: Type pattern needs different behavior (all pointers, all const, etc.)

---

### Q4: Explain non-type template parameters. What types can be used as non-type parameters and what are their constraints?

**Answer:**

**Non-type template parameters** are template parameters that are constant values rather than types. They allow passing compile-time constants to templates, enabling optimizations and zero-runtime-overhead abstractions.

**Syntax:**

```cpp
template <typename T, int SIZE>
class Array {
    T data[SIZE];  // SIZE is compile-time constant
};
```

**Valid Non-Type Parameter Types:**

1. **Integral Types:**

```cpp
template <int N>           // int
template <unsigned U>      // unsigned int
template <long L>          // long
template <char C>          // char
template <bool B>          // bool
class Example {};
```

1. **Enumeration Types:**

```cpp
enum Color { RED, GREEN, BLUE };
template <Color C>
class ColoredBox {};
```

1. **Pointer Types:**

```cpp
template <int* ptr>        // Pointer to int
template <char* str>       // Pointer to char
class PointerTemplate {};
```

1. **Reference Types:**

```cpp
template <int& ref>
class ReferenceTemplate {};
```

1. **nullptr_t:**

```cpp
template <decltype(nullptr) N>
class NullTemplate {};
```

**Invalid Non-Type Parameter Types:**

```cpp
template <float F>       // ERROR: floating-point not allowed
template <double D>      // ERROR: floating-point not allowed
template <string S>      // ERROR: class types not allowed
template <void>          // ERROR: void not allowed
```

**Constraints:**

1. **Must Be Compile-Time Constants:**

```cpp
const int SIZE = 10;
Array<int, SIZE> arr1;          // OK
Array<int, 5 + 3> arr2;         // OK: compile-time expression

int runtime_size = 10;
// Array<int, runtime_size> arr3;  // ERROR: not compile-time
```

1. **Addresses Must Have External Linkage:**

```cpp
int global = 0;
template <int* ptr>
class PtrTemplate {};

PtrTemplate<&global> obj;  // OK: global has external linkage

void func() {
    int local = 0;
    // PtrTemplate<&local> obj2;  // ERROR: local address
}
```

**Practical Uses:**

1. **Fixed-Size Containers:**

```cpp
template <typename T, int SIZE>
class StaticArray {
    T data[SIZE];  // No dynamic allocation
};
```

1. **Compile-Time Computation:**

```cpp
template <int N>
struct Factorial {
    static const int value = N * Factorial<N-1>::value;
};
template <>
struct Factorial<0> {
    static const int value = 1;
};
// Factorial<5>::value computed at compile time
```

1. **Algorithm Optimization:**

```cpp
template <typename T, int UNROLL_FACTOR>
void optimized_loop(T* data, int size) {
    // Loop unrolling based on UNROLL_FACTOR
}
```

**Benefits:**

- Zero runtime overhead
- Type safety for constant values
- Enables template metaprogramming
- Compiler optimizations possible

---

### Q5: How do variable templates (C++14) work? Provide practical examples of when and why you would use them.

**Answer:**

**Variable Templates** (introduced in C++14) allow creating parameterized variables, typically used for type-dependent compile-time constants. They provide a cleaner syntax for defining families of related constants.

**Basic Syntax:**

```cpp
template <typename T>
constexpr T pi = T(3.1415926535897932385);

// Usage
double d = pi<double>;   // High precision
float f = pi<float>;     // Lower precision
```

**How They Work:**

Variable templates create a separate variable for each type instantiation. The compiler generates specialized variables at compile time, similar to function and class templates.

**Practical Use Cases:**

**1. Mathematical Constants:**

```cpp
// WHY: Different precision for different types
template <typename T>
constexpr T pi = T(3.1415926535897932385);

template <typename T>
constexpr T e = T(2.718281828459045);

// Usage in calculations
double circle_area = pi<double> * radius * radius;
float approx_area = pi<float> * radius * radius;
```

**2. Type Traits (std::is_pointer_v pattern):**

```cpp
// WHY: Cleaner syntax than accessing ::value
template <typename T>
constexpr bool is_pointer_v = false;

template <typename T>
constexpr bool is_pointer_v<T*> = true;

// Modern usage (C++17)
if constexpr (is_pointer_v<T>) {
    // Handle pointer types
}

// Old style (verbose)
// if (std::is_pointer<T>::value) { }
```

**3. Configuration Constants:**

```cpp
template <typename T>
constexpr int buffer_size = 1024;  // Default

// Specializations for specific types
template <>
constexpr int buffer_size<char> = 512;

template <>
constexpr int buffer_size<double> = 2048;

// WHY: Type-specific buffer allocation
char buffer[buffer_size<char>];
double dbuffer[buffer_size<double>];
```

**4. Static Data Members in Classes:**

```cpp
class Limits {
public:
    template <typename T>
    static constexpr T min_value = T(0);

    template <typename T>
    static constexpr T max_value = T(100);
};

// Specialization for specific types
template <>
constexpr int Limits::max_value<int> = 2147483647;

// Usage
int max_int = Limits::max_value<int>;
double max_double = Limits::max_value<double>;
```

**Why Use Variable Templates:**

1. **Type-Safe Constants:**

```cpp
// Variable template - type safe
template <typename T>
constexpr T zero = T(0);

// Macro - not type safe
#define ZERO 0
```

1. **Cleaner Syntax:**

```cpp
// Variable template
if constexpr (is_integral_v<T>)  // Clean

// Old trait style
if (std::is_integral<T>::value)  // Verbose
```

1. **Specialization Support:**

```cpp
template <typename T>
constexpr size_t alignment = alignof(T);

// Custom alignment for specific type
template <>
constexpr size_t alignment<SpecialType> = 64;
```

**Advantages Over Alternatives:**

| Method | Syntax | Type Safety | Specialization |
| --- | --- | --- | --- |
| Variable Template | `pi<double>` | ✓ | ✓ |
| Macro | `PI` | ✗ | ✗ |
| constexpr Function | `pi<double>()` | ✓ | Limited |
| Static Const | `Class::PI` | ✓ | ✗ |

**Best Practices:**

1. Use `constexpr` with variable templates for compile-time evaluation
2. Provide sensible defaults and specialize when needed
3. Follow naming conventions (lowercase with `_v` suffix for traits)
4. Document precision and range for numeric constants

Variable templates are particularly useful in template metaprogramming, type traits libraries, and situations requiring type-dependent compile-time constants with clean, readable syntax.

---