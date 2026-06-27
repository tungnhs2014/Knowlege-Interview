# 7.3. Templates - Type Traits, Concepts & Metaprogramming

---

## Table of Contents

1. Type Traits
2. C++20 Concepts
3. Template Metaprogramming
4. Summary
5. Interview Preparation

---

## 1. Type Traits

### 1.1 What are Type Traits?

**Type traits** are templates that provide compile-time information about types. They form the foundation of SFINAE, template metaprogramming, and generic programming in C++.

**Why Type Traits Matter:**

Type traits enable you to:

- Query type properties at compile time
- Conditionally enable/disable code
- Transform types
- Make decisions based on type characteristics

```cpp
#include <iostream>
#include <type_traits>
using namespace std;

template <typename T>
void analyze() {
    if constexpr (is_integral<T>::value) {
        cout << "T is an integral type" << endl;
    } else if constexpr (is_floating_point<T>::value) {
        cout << "T is a floating-point type" << endl;
    } else {
        cout << "T is neither integral nor floating-point" << endl;
    }
}

int main() {
    analyze<int>();     // T is an integral type
    analyze<double>();  // T is a floating-point type
    analyze<string>();  // T is neither...
    return 0;
}
```

### 1.2 Standard Type Trait Categories

**Primary Type Categories:**

```cpp
#include <type_traits>

// Check fundamental types
is_void<T>             // void
is_null_pointer<T>     // nullptr_t
is_integral<T>         // int, char, bool, etc.
is_floating_point<T>   // float, double, long double
is_array<T>            // C-style arrays
is_enum<T>             // Enumeration types
is_union<T>            // Union types
is_class<T>            // Class/struct types
is_function<T>         // Function types
is_pointer<T>          // Pointer types
is_lvalue_reference<T> // Lvalue references
is_rvalue_reference<T> // Rvalue references
is_member_object_pointer<T>    // Pointer to member object
is_member_function_pointer<T>  // Pointer to member function
```

**Composite Type Categories:**

```cpp
is_fundamental<T>      // Arithmetic or void or nullptr_t
is_arithmetic<T>       // Integral or floating-point
is_scalar<T>           // Arithmetic, pointer, enum, nullptr_t
is_object<T>           // Not function, reference, or void
is_compound<T>         // Array, function, pointer, reference, class, union, enum
is_reference<T>        // Lvalue or rvalue reference
is_member_pointer<T>   // Pointer to member
```

**Type Properties:**

```cpp
is_const<T>            // const-qualified
is_volatile<T>         // volatile-qualified
is_trivial<T>          // Trivially copyable and default constructible
is_trivially_copyable<T>       // Can be copied with memcpy
is_standard_layout<T>  // Standard layout type
is_pod<T>              // Plain Old Data (deprecated in C++20)
is_empty<T>            // Empty class (no non-static members)
is_polymorphic<T>      // Has at least one virtual function
is_abstract<T>         // Has at least one pure virtual function
is_final<T>            // Marked with final
is_aggregate<T>        // Aggregate type (C++17)
is_signed<T>           // Signed arithmetic type
is_unsigned<T>         // Unsigned arithmetic type
```

### 1.3 Using Standard Type Traits

```cpp
#include <iostream>
#include <type_traits>
using namespace std;

template <typename T>
void analyze_type() {
    cout << "Type analysis for: " << typeid(T).name() << endl;
    cout << "  is_integral: " << is_integral<T>::value << endl;
    cout << "  is_floating_point: " << is_floating_point<T>::value << endl;
    cout << "  is_pointer: " << is_pointer<T>::value << endl;
    cout << "  is_array: " << is_array<T>::value << endl;
    cout << "  is_const: " << is_const<T>::value << endl;
    cout << "  is_reference: " << is_reference<T>::value << endl;
    cout << endl;
}

int main() {
    analyze_type<int>();
    analyze_type<double>();
    analyze_type<int*>();
    analyze_type<const int>();
    analyze_type<int&>();

    return 0;
}
```

### 1.4 Type Relationships

```cpp
#include <iostream>
#include <type_traits>
using namespace std;

class Base {};
class Derived : public Base {};

int main() {
    // WHY: Check type relationships at compile time
    cout << boolalpha;

    // Type equality
    cout << "is_same<int, int>: "
         << is_same<int, int>::value << endl;
    cout << "is_same<int, double>: "
         << is_same<int, double>::value << endl;

    // Inheritance
    cout << "is_base_of<Base, Derived>: "
         << is_base_of<Base, Derived>::value << endl;
    cout << "is_base_of<Derived, Base>: "
         << is_base_of<Derived, Base>::value << endl;

    // Convertibility
    cout << "is_convertible<int, double>: "
         << is_convertible<int, double>::value << endl;
    cout << "is_convertible<double*, int*>: "
         << is_convertible<double*, int*>::value << endl;

    // Construction
    cout << "is_constructible<string, const char*>: "
         << is_constructible<string, const char*>::value << endl;

    return 0;
}
```

**Output:**

```
is_same<int, int>: true
is_same<int, double>: false
is_base_of<Base, Derived>: true
is_base_of<Derived, Base>: false
is_convertible<int, double>: true
is_convertible<double*, int*>: false
is_constructible<string, const char*>: true
```

### 1.5 Type Transformations

**Removing Qualifiers:**

```cpp
#include <type_traits>

remove_const<const int>         // → int
remove_volatile<volatile int>   // → int
remove_cv<const volatile int>   // → int
remove_reference<int&>          // → int
remove_reference<int&&>         // → int
remove_pointer<int*>            // → int
remove_extent<int[5]>           // → int (removes one array dimension)
remove_all_extents<int[5][10]>  // → int (removes all dimensions)
```

**Adding Qualifiers:**

```cpp
add_const<int>           // → const int
add_volatile<int>        // → volatile int
add_cv<int>              // → const volatile int
add_lvalue_reference<int>  // → int&
add_rvalue_reference<int>  // → int&&
add_pointer<int>         // → int*
```

**Example Usage:**

```cpp
#include <iostream>
#include <type_traits>
using namespace std;

int main() {
    // WHY: Remove qualifiers to get base type
    using T1 = remove_const_t<const int>;
    using T2 = remove_reference_t<int&>;
    using T3 = remove_pointer_t<int*>;

    cout << boolalpha;
    cout << is_same<T1, int>::value << endl;  // true
    cout << is_same<T2, int>::value << endl;  // true
    cout << is_same<T3, int>::value << endl;  // true

    // WHY: Add qualifiers
    using T4 = add_const_t<int>;
    using T5 = add_pointer_t<int>;

    cout << is_same<T4, const int>::value << endl;  // true
    cout << is_same<T5, int*>::value << endl;       // true

    return 0;
}
```

### 1.6 Custom Type Traits

**Creating Custom Traits:**

```cpp
#include <iostream>
#include <type_traits>
using namespace std;

// WHY: Custom trait to check if type has push_back method
template <typename, typename = void>
struct has_push_back : false_type {};

template <typename T>
struct has_push_back<T,
    void_t<decltype(declval<T>().push_back(declval<typename T::value_type>()))>>
    : true_type {};

#include <vector>
#include <list>
#include <set>

int main() {
    cout << boolalpha;
    cout << "vector has push_back: "
         << has_push_back<vector<int>>::value << endl;
    cout << "list has push_back: "
         << has_push_back<list<int>>::value << endl;
    cout << "set has push_back: "
         << has_push_back<set<int>>::value << endl;

    return 0;
}
```

**Output:**

```
vector has push_back: true
list has push_back: true
set has push_back: false
```

**Pattern for Custom Traits:**

```cpp
// Step 1: Primary template - defaults to false
template <typename T, typename = void>
struct has_member : false_type {};

// Step 2: Specialization that only works if condition is met
template <typename T>
struct has_member<T, void_t</* check expression */>>
    : true_type {};
```

**Example: Detect size() Method**

```cpp
template <typename, typename = void>
struct has_size : false_type {};

template <typename T>
struct has_size<T, void_t<decltype(declval<T>().size())>>
    : true_type {};

// Usage
has_size<vector<int>>::value  // true
has_size<int>::value          // false
```

**Example: Detect Iterator**

```cpp
template <typename, typename = void>
struct is_iterator : false_type {};

template <typename T>
struct is_iterator<T, void_t<
    typename iterator_traits<T>::iterator_category
>> : true_type {};
```

### 1.7 integral_constant Base

**All type traits inherit from integral_constant:**

```cpp
template <typename T, T v>
struct integral_constant {
    static constexpr T value = v;
    using value_type = T;
    using type = integral_constant<T, v>;

    constexpr operator value_type() const noexcept { return value; }
    constexpr value_type operator()() const noexcept { return value; }
};

// Common aliases
using true_type = integral_constant<bool, true>;
using false_type = integral_constant<bool, false>;
```

**Creating Custom Constant Traits:**

```cpp
#include <iostream>
#include <type_traits>
using namespace std;

// WHY: Custom trait based on type size
template <typename T>
struct is_small : integral_constant<bool, (sizeof(T) <= 4)> {};

// WHY: Custom trait for numeric ranges
template <typename T>
struct is_byte_sized : integral_constant<bool, (sizeof(T) == 1)> {};

int main() {
    cout << boolalpha;
    cout << "char is small: " << is_small<char>::value << endl;
    cout << "double is small: " << is_small<double>::value << endl;
    cout << "char is byte-sized: " << is_byte_sized<char>::value << endl;
    cout << "int is byte-sized: " << is_byte_sized<int>::value << endl;

    return 0;
}
```

### 1.8 Practical Type Trait Applications

**Application 1: Conditional Compilation**

```cpp
template <typename T>
class Container {
public:
    void sort() {
        // WHY: Different sort for trivial vs complex types
        if constexpr (is_trivially_copyable<T>::value) {
            // Use fast memcpy-based sort
            cout << "Using fast sort" << endl;
        } else {
            // Use safe move-based sort
            cout << "Using safe sort" << endl;
        }
    }
};
```

**Application 2: Type-Safe Serialization**

```cpp
template <typename T>
enable_if_t<is_arithmetic<T>::value, string>
serialize(T value) {
    return to_string(value);
}

template <typename T>
enable_if_t<is_class<T>::value && has_serialize<T>::value, string>
serialize(const T& obj) {
    return obj.serialize();  // Call member function
}
```

**Application 3: Perfect Decay**

```cpp
template <typename T>
using decay_t = typename remove_cv<
    typename remove_reference<T>::type
>::type;

// Modern C++14 already provides std::decay_t
```

---

## 2. C++20 Concepts

### 2.1 What are Concepts?

**Concepts** provide a way to specify constraints on template parameters, offering clearer syntax and dramatically better error messages than SFINAE.

**Why Concepts Matter:**

Before C++20 (SFINAE):

```cpp
// Hard to read, cryptic errors
template <typename T>
enable_if_t<is_integral<T>::value || is_floating_point<T>::value, T>
multiply(T a, T b) {
    return a * b;
}
```

C++20 (Concepts):

```cpp
// Clear, expressive, great errors
template <typename T>
concept Numeric = integral<T> || floating_point<T>;

template <Numeric T>
T multiply(T a, T b) {
    return a * b;
}
```

### 2.2 Basic Concept Syntax

**Defining a Concept:**

```cpp
template <typename T>
concept ConceptName = /* boolean constraint expression */;
```

**Using a Concept:**

```cpp
// Syntax 1: After template keyword
template <Numeric T>
void func(T value) { }

// Syntax 2: requires clause
template <typename T>
requires Numeric<T>
void func(T value) { }

// Syntax 3: Trailing requires
template <typename T>
void func(T value) requires Numeric<T> { }
```

### 2.3 Standard Library Concepts

**Core Language Concepts:**

```cpp
#include <concepts>

// Type categories
same_as<T, U>           // T and U are same type
derived_from<T, U>      // T derived from U
convertible_to<T, U>    // T convertible to U
common_reference_with<T, U>  // T and U have common reference
common_with<T, U>       // T and U have common type
integral<T>             // Integral type
signed_integral<T>      // Signed integral
unsigned_integral<T>    // Unsigned integral
floating_point<T>       // Floating-point type
```

**Object Concepts:**

```cpp
assignable_from<T, U>   // Can assign U to T
swappable<T>            // Can swap
swappable_with<T, U>    // Can swap T with U
destructible<T>         // Has destructor
constructible_from<T, Args...>  // Can construct from Args
default_initializable<T>        // Can default construct
move_constructible<T>   // Can move construct
copy_constructible<T>   // Can copy construct
```

**Comparison Concepts:**

```cpp
equality_comparable<T>  // Can compare with ==
totally_ordered<T>      // Can compare with <, >, <=, >=, ==, !=
```

### 2.4 Simple Concept Examples

```cpp
#include <iostream>
#include <concepts>
using namespace std;

// WHY: Concept defines requirements for Numeric types
template <typename T>
concept Numeric = integral<T> || floating_point<T>;

// WHY: Only accepts numeric types
template <Numeric T>
T add(T a, T b) {
    return a + b;
}

int main() {
    cout << add(10, 20) << endl;      // OK: int is Numeric
    cout << add(3.14, 2.71) << endl;  // OK: double is Numeric

    // add("hello", "world");  // ERROR: string is not Numeric

    return 0;
}
```

**Output:**

```
30
5.85
```

### 2.5 requires Clause

**Simple Constraint:**

```cpp
template <typename T>
requires integral<T> || floating_point<T>
T multiply(T a, T b) {
    return a * b;
}
```

**Multiple Constraints:**

```cpp
template <typename T>
requires integral<T> && sizeof(T) >= 4
void process(T value) {
    cout << "Processing large integer: " << value << endl;
}
```

**Trailing requires:**

```cpp
template <typename T>
T divide(T a, T b) requires floating_point<T> {
    return a / b;
}
```

### 2.6 requires Expression

**Check if operations are valid:**

```cpp
#include <iostream>
#include <concepts>
using namespace std;

// WHY: Concept checks if type has specific operations
template <typename T>
concept Addable = requires(T a, T b) {
    { a + b } -> convertible_to<T>;  // Must support + returning T
};

template <typename T>
concept Multipliable = requires(T a, T b) {
    { a * b } -> convertible_to<T>;
};

// WHY: Combine concepts
template <typename T>
concept Numeric = Addable<T> && Multipliable<T>;

template <Numeric T>
T compute(T a, T b, T c) {
    return a + b * c;
}

int main() {
    cout << compute(2, 3, 4) << endl;        // 14
    cout << compute(2.5, 3.0, 4.0) << endl;  // 14.5
    return 0;
}
```

**Output:**

```
14
14.5
```

**Complex requires Expression:**

```cpp
template <typename T>
concept Container = requires(T t) {
    typename T::value_type;      // Must have value_type
    typename T::iterator;        // Must have iterator
    { t.begin() } -> same_as<typename T::iterator>;
    { t.end() } -> same_as<typename T::iterator>;
    { t.size() } -> convertible_to<size_t>;
};
```

### 2.7 Custom Concepts

**Example 1: Arithmetic Concept**

```cpp
template <typename T>
concept Arithmetic = requires(T a, T b) {
    { a + b } -> same_as<T>;
    { a - b } -> same_as<T>;
    { a * b } -> same_as<T>;
    { a / b } -> same_as<T>;
};

template <Arithmetic T>
T average(T a, T b) {
    return (a + b) / T(2);
}
```

**Example 2: Printable Concept**

```cpp
#include <iostream>
#include <concepts>
using namespace std;

template <typename T>
concept Printable = requires(ostream& os, T value) {
    { os << value } -> convertible_to<ostream&>;
};

template <Printable T>
void print(const T& value) {
    cout << value << endl;
}

int main() {
    print(42);
    print(3.14);
    print("hello");
    return 0;
}
```

**Example 3: HasBegin Concept**

```cpp
template <typename T>
concept HasBegin = requires(T t) {
    t.begin();
    t.end();
};

template <HasBegin Container>
void print_container(const Container& c) {
    for (const auto& elem : c) {
        cout << elem << " ";
    }
    cout << endl;
}
```

### 2.8 Concept Subsumption

**More specific concepts are preferred:**

```cpp
#include <iostream>
#include <concepts>
using namespace std;

template <typename T>
concept Integral = integral<T>;

template <typename T>
concept SignedIntegral = Integral<T> && signed_integral<T>;

// WHY: More specific concept preferred
template <Integral T>
void process(T value) {
    cout << "Processing integral: " << value << endl;
}

// WHY: This is more specific - will be chosen for signed types
template <SignedIntegral T>
void process(T value) {
    cout << "Processing signed integral: " << value << endl;
}

int main() {
    process(42);    // Calls SignedIntegral version
    process(42u);   // Calls Integral version
    return 0;
}
```

**Output:**

```
Processing signed integral: 42
Processing integral: 42
```

### 2.9 Concepts vs SFINAE

**Comparison:**

| Aspect | SFINAE | Concepts |
| --- | --- | --- |
| **Syntax** | Verbose, complex | Clean, readable |
| **Error Messages** | Cryptic, nested | Clear, specific |
| **Intent** | Hidden in traits | Self-documenting |
| **Composition** | Difficult | Easy (&&, ||) |
| **Overload Resolution** | Complex | Subsumption rules |
| **Availability** | C++11+ | C++20+ |
| **Learning Curve** | Steep | Gentle |

**SFINAE Example:**

```cpp
// Complex, hard to understand
template <typename T>
enable_if_t<
    is_integral<T>::value && is_signed<T>::value && sizeof(T) >= 4,
    void
>
process(T value) {
    cout << "Processing large signed integer" << endl;
}
```

**Concepts Example:**

```cpp
// Clear, self-documenting
template <typename T>
concept LargeSignedInteger =
    integral<T> && signed_integral<T> && sizeof(T) >= 4;

template <LargeSignedInteger T>
void process(T value) {
    cout << "Processing large signed integer" << endl;
}
```

### 2.10 When to Use Concepts

**✅ Use Concepts When:**

1. **C++20 or later available**
2. **Complex type constraints** - Multiple requirements
3. **Public APIs** - Clear documentation
4. **Overload resolution** - Need subsumption
5. **Better errors desired** - Help users understand failures

**Example: Library Interface**

```cpp
// Clear API contract
template <typename T>
concept Serializable = requires(T t) {
    { t.serialize() } -> convertible_to<string>;
    { T::deserialize(string()) } -> same_as<T>;
};

template <Serializable T>
void save(const T& obj, const string& filename) {
    // Implementation
}
```

**❌ Don't Use Concepts When:**

1. **C++17 or earlier required**
2. **Simple SFINAE sufficient** - Single type check
3. **Backward compatibility critical**

---

## 3. Template Metaprogramming

### 3.1 What is Template Metaprogramming?

**Template metaprogramming (TMP)** uses templates to perform computations at compile time. The compiler acts as an interpreter for a functional programming language embedded in C++.

**Key Characteristics:**

- All computation happens at compile time
- Zero runtime overhead
- Uses recursion (no loops)
- Functional programming style
- Can manipulate both types and values

### 3.2 Compile-Time Factorial

```cpp
#include <iostream>
using namespace std;

// WHY: Recursive template computes factorial at compile time
template <int N>
struct Factorial {
    static constexpr int value = N * Factorial<N - 1>::value;
};

// WHY: Base case stops recursion
template <>
struct Factorial<0> {
    static constexpr int value = 1;
};

int main() {
    // WHY: Computed at compile time - no runtime cost
    cout << "Factorial(5) = " << Factorial<5>::value << endl;
    cout << "Factorial(10) = " << Factorial<10>::value << endl;

    // Use in array size (proves compile-time)
    int arr[Factorial<5>::value];  // Array of size 120

    return 0;
}
```

**Output:**

```
Factorial(5) = 120
Factorial(10) = 3628800
```

### 3.3 Compile-Time Power

```cpp
#include <iostream>
using namespace std;

// WHY: Calculate Base^Exp at compile time
template <int Base, int Exp>
struct Power {
    static constexpr int value = Base * Power<Base, Exp - 1>::value;
};

template <int Base>
struct Power<Base, 0> {
    static constexpr int value = 1;
};

int main() {
    cout << "2^8 = " << Power<2, 8>::value << endl;
    cout << "3^4 = " << Power<3, 4>::value << endl;
    cout << "10^3 = " << Power<10, 3>::value << endl;

    return 0;
}
```

**Output:**

```
2^8 = 256
3^4 = 81
10^3 = 1000
```

### 3.4 Type Lists

```cpp
#include <iostream>
#include <type_traits>
using namespace std;

// WHY: Store types at compile time
template <typename... Types>
struct TypeList {};

// WHY: Get length of type list
template <typename List>
struct Length;

template <typename... Types>
struct Length<TypeList<Types...>> {
    static constexpr size_t value = sizeof...(Types);
};

// WHY: Check if type is in list
template <typename T, typename List>
struct Contains;

template <typename T, typename... Types>
struct Contains<T, TypeList<Types...>>
    : integral_constant<bool, (is_same<T, Types>::value || ...)> {};

int main() {
    using MyList = TypeList<int, double, char, string>;

    cout << "List length: " << Length<MyList>::value << endl;
    cout << boolalpha;
    cout << "Contains int: " << Contains<int, MyList>::value << endl;
    cout << "Contains float: " << Contains<float, MyList>::value << endl;

    return 0;
}
```

**Output:**

```
List length: 4
Contains int: true
Contains float: false
```

### 3.5 Compile-Time Fibonacci

```cpp
#include <iostream>
using namespace std;

template <int N>
struct Fibonacci {
    static constexpr int value =
        Fibonacci<N-1>::value + Fibonacci<N-2>::value;
};

template <>
struct Fibonacci<0> {
    static constexpr int value = 0;
};

template <>
struct Fibonacci<1> {
    static constexpr int value = 1;
};

int main() {
    cout << "Fib(10) = " << Fibonacci<10>::value << endl;
    cout << "Fib(15) = " << Fibonacci<15>::value << endl;

    return 0;
}
```

**Output:**

```
Fib(10) = 55
Fib(15) = 610
```

### 3.6 Modern Alternative - constexpr

**constexpr functions** provide a cleaner way to achieve compile-time computation:

```cpp
#include <iostream>
using namespace std;

// WHY: constexpr is cleaner and easier to read
constexpr int factorial(int n) {
    return (n <= 1) ? 1 : n * factorial(n - 1);
}

constexpr int power(int base, int exp) {
    int result = 1;
    for (int i = 0; i < exp; ++i) {
        result *= base;
    }
    return result;
}

constexpr int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n-1) + fibonacci(n-2);
}

int main() {
    // WHY: All computed at compile time
    constexpr int f5 = factorial(5);
    constexpr int p28 = power(2, 8);
    constexpr int fib10 = fibonacci(10);

    cout << "Factorial(5) = " << f5 << endl;
    cout << "2^8 = " << p28 << endl;
    cout << "Fib(10) = " << fib10 << endl;

    // Prove it's compile-time
    int arr[f5];  // Array size must be compile-time constant

    return 0;
}
```

**Output:**

```
Factorial(5) = 120
2^8 = 256
Fib(10) = 55
```

### 3.7 When to Use Template Metaprogramming

**Use Template Metaprogramming:**

✅ **Type Manipulation**

```cpp
template <typename T>
using RemovePtr = typename remove_pointer<T>::type;
```

✅ **Type Lists and Traits**

```cpp
template <typename... Types>
struct TypeList {};

template <typename List>
struct First;
```

✅ **SFINAE Patterns**

```cpp
template <typename T, typename = void>
struct has_begin : false_type {};
```

✅ **When constexpr Isn't Sufficient**

```cpp
// Type-level operations
template <typename T>
struct add_pointer {
    using type = T*;
};
```

**Use constexpr Instead:**

✅ **Value Computations**

```cpp
constexpr int factorial(int n) { /* ... */ }
```

✅ **Runtime-Like Logic**

```cpp
constexpr int max(int a, int b) {
    return (a > b) ? a : b;
}
```

✅ **Loops and Iteration**

```cpp
constexpr int sum_array(const int* arr, int size) {
    int sum = 0;
    for (int i = 0; i < size; ++i) sum += arr[i];
    return sum;
}
```

### 3.8 Template Metaprogramming Best Practices

**1. Prefer constexpr When Possible**

```cpp
// ❌ Template metaprogramming - verbose
template <int N>
struct Factorial {
    static constexpr int value = N * Factorial<N-1>::value;
};
template <>
struct Factorial<0> {
    static constexpr int value = 1;
};

// ✅ constexpr - cleaner
constexpr int factorial(int n) {
    return (n <= 1) ? 1 : n * factorial(n-1);
}
```

**2. Document Complex TMP Code**

```cpp
// WHY: Clear documentation is essential
template <typename T>
struct remove_all_pointers {
    using type = T;
};

template <typename T>
struct remove_all_pointers<T*> {
    using type = typename remove_all_pointers<T>::type;
};
```

**3. Use Type Aliases**

```cpp
// WHY: Makes code more readable
template <typename T>
using remove_all_pointers_t =
    typename remove_all_pointers<T>::type;
```

**4. Avoid Deep Recursion**

```cpp
// ❌ Deep recursion can hit compiler limits
// ✅ Use iterative constexpr when possible
```

### 3.9 Real-World TMP Applications

**Application 1: Expression Templates (Math Libraries)**

```cpp
// Lazy evaluation of mathematical expressions
template <typename E>
class VecExpression {
    // Delayed computation
};
```

**Application 2: Policy-Based Design**

```cpp
template <typename LockingPolicy>
class Container {
    LockingPolicy lock_;
    // Compile-time policy selection
};
```

**Application 3: Dimensional Analysis**

```cpp
template <int M, int L, int T>  // Mass, Length, Time
struct Unit {
    // Compile-time unit checking
};
```

---

## Summary

### Key Takeaways

1. **Type Traits Provide Compile-Time Type Information** - Standard library offers extensive type traits for querying type properties, relationships, and transformations. Foundation for SFINAE and generic programming.
2. **Custom Type Traits Enable Member Detection** - Use `void_t` and `decltype(declval<T>().member())` pattern to detect if types have specific members or operations at compile time.
3. **integral_constant is the Base** - All type traits inherit from `integral_constant<bool, value>`, providing uniform interface. Use `true_type` and `false_type` for boolean traits.
4. **C++20 Concepts Revolutionize Template Constraints** - Concepts provide clear syntax for type requirements with dramatically better error messages than SFINAE. Self-documenting code.
5. **requires Clause Specifies Constraints** - Multiple syntaxes available: after template keyword, trailing requires, or in requires expression. Choose based on readability.
6. **Standard Concepts Cover Common Needs** - Library provides concepts for integrals, floating-point, containers, and more. Build custom concepts for domain-specific requirements.
7. **Concept Subsumption Enables Specialization** - More specific concepts automatically preferred in overload resolution. Enables powerful compile-time polymorphism patterns.
8. **Concepts vs SFINAE Trade-offs** - Concepts offer cleaner syntax and better errors but require C++20. SFINAE works in older standards but is verbose and complex.
9. **Template Metaprogramming Performs Compile-Time Computation** - Use templates for value computation and type manipulation at compile time. Zero runtime overhead but increases compilation time.
10. **Prefer constexpr Over TMP for Values** - Modern `constexpr` functions are cleaner, more readable, and easier to debug than template metaprogramming for value computations. Reserve TMP for type-level operations.

---

## Interview Preparation

### Q1: Compare C++20 Concepts with SFINAE. What are the advantages of Concepts? When would you still use SFINAE?

**Answer:**

**Concepts** and **SFINAE** both constrain template parameters, but Concepts provide a modern, superior approach with significant advantages.

**Key Differences:**

**1. Syntax Clarity:**

```cpp
// SFINAE - verbose and hard to read
template <typename T>
typename enable_if<
    is_integral<T>::value || is_floating_point<T>::value,
    T
>::type
multiply(T a, T b) {
    return a * b;
}

// Concepts - clean and self-documenting
template <typename T>
concept Numeric = integral<T> || floating_point<T>;

template <Numeric T>
T multiply(T a, T b) {
    return a * b;
}
```

**2. Error Messages:**

```cpp
// SFINAE error (cryptic)
template <typename T>
enable_if_t<is_integral<T>::value, T>
process(T value) { return value * 2; }

process(3.14);
// ERROR: no matching function for call to 'process(double)'
//   candidate: template<class T> std::enable_if_t<...>
//   template argument deduction/substitution failed:
//   substitution of 'enable_if<false, double>' failed

// Concepts error (clear)
template <integral T>
T process(T value) { return value * 2; }

process(3.14);
// ERROR: no matching function for call to 'process(double)'
//   candidate requires: integral<double>
//   → the constraint was not satisfied
```

**3. Composition:**

```cpp
// SFINAE - complex composition
template <typename T>
enable_if_t<
    is_arithmetic<T>::value &&
    !is_same<T, bool>::value &&
    sizeof(T) >= 4,
    void
>
process(T value);

// Concepts - intuitive composition
template <typename T>
concept LargeArithmetic =
    arithmetic<T> &&
    !same_as<T, bool> &&
    sizeof(T) >= 4;

template <LargeArithmetic T>
void process(T value);
```

**4. Overload Resolution:**

```cpp
// Concepts support subsumption
template <typename T>
concept Integral = integral<T>;

template <typename T>
concept SignedIntegral = Integral<T> && signed_integral<T>;

// More specific concept automatically preferred
template <Integral T>
void func(T x) { cout << "Integral\n"; }

template <SignedIntegral T>  // More specific
void func(T x) { cout << "Signed\n"; }

func(42);   // Calls SignedIntegral version
func(42u);  // Calls Integral version
```

**Advantages of Concepts:**

| Advantage | Explanation |
| --- | --- |
| **Readability** | Self-documenting, clear intent |
| **Error Messages** | Specific constraint failures |
| **Maintainability** | Easier to understand and modify |
| **Composition** | Natural boolean logic (&&, ||) |
| **Subsumption** | Automatic specialization selection |
| **Teaching** | Gentler learning curve |

**When to Still Use SFINAE:**

✅ **C++17 or Earlier Required**

- Maintaining legacy code
- Targeting older compilers
- Cross-platform compatibility

✅ **Simple Enable/Disable Cases**

```cpp
// Simple SFINAE still acceptable
template <typename T,
          typename = enable_if_t<is_arithmetic<T>::value>>
void func(T value);
```

✅ **Member Detection Traits**

```cpp
// SFINAE pattern for detection
template <typename T, typename = void>
struct has_begin : false_type {};

template <typename T>
struct has_begin<T, void_t<decltype(declval<T>().begin())>>
    : true_type {};
```

✅ **Library Code Supporting Multiple Standards**

```cpp
#if __cplusplus >= 202002L
    // C++20: Use concepts
    template <integral T>
    void func(T value);
#else
    // Pre-C++20: Use SFINAE
    template <typename T, enable_if_t<is_integral<T>::value>* = nullptr>
    void func(T value);
#endif
```

**Best Practice:**

If C++20 is available, always prefer Concepts over SFINAE. They provide all the same functionality with dramatically better developer experience, clearer code, and superior error messages.

---

### Q2: What is template metaprogramming? Compare it with constexpr functions. When should you use each approach?

**Answer:**

**Template Metaprogramming (TMP)** uses templates to perform computations at compile time, treating the compiler as an interpreter for a functional programming language.

**How TMP Works:**

```cpp
// Recursive template - compiler evaluates at compile time
template <int N>
struct Factorial {
    static constexpr int value = N * Factorial<N-1>::value;
};

template <>  // Base case
struct Factorial<0> {
    static constexpr int value = 1;
};

// Usage - computed at compile time
constexpr int result = Factorial<5>::value;  // 120
```

**constexpr Functions:**

Modern C++ provides `constexpr` functions as a cleaner alternative:

```cpp
// Same computation, much cleaner
constexpr int factorial(int n) {
    return (n <= 1) ? 1 : n * factorial(n-1);
}

constexpr int result = factorial(5);  // 120, compile-time
```

**Detailed Comparison:**

| Aspect | Template Metaprogramming | constexpr Functions |
| --- | --- | --- |
| **Syntax** | Complex, template-based | Normal C++ functions |
| **Readability** | Difficult, functional style | Easy, imperative style |
| **Debugging** | Very hard, no debugger | Standard debugging |
| **Error Messages** | Cryptic template errors | Clear function errors |
| **Recursion Limit** | Compiler-specific (~900) | Standard stack limit |
| **Type Computation** | Natural, primary use | Limited support |
| **Value Computation** | Verbose, awkward | Natural, intuitive |
| **C++ Version** | C++98+ | C++11+ (improved in C++14/17/20) |
| **Runtime Use** | Compile-time only | Both compile and runtime |

**When to Use Template Metaprogramming:**

✅ **Type-Level Operations**

```cpp
// Manipulating types at compile time
template <typename T>
struct add_pointer {
    using type = T*;
};

template <typename T>
struct remove_pointer {
    using type = T;
};

template <typename T>
struct remove_pointer<T*> {
    using type = T;
};
```

✅ **Type Lists and Trait Detection**

```cpp
template <typename... Types>
struct TypeList {};

template <typename T, typename = void>
struct has_begin : false_type {};

template <typename T>
struct has_begin<T, void_t<decltype(declval<T>().begin())>>
    : true_type {};
```

✅ **SFINAE Patterns**

```cpp
template <typename T>
enable_if_t<is_integral<T>::value, void>
process(T value);
```

✅ **Compile-Time Code Generation**

```cpp
template <size_t N>
struct UnrollLoop {
    template <typename F>
    static void execute(F&& f) {
        f(N);
        UnrollLoop<N-1>::execute(forward<F>(f));
    }
};
```

**When to Use constexpr:**

✅ **Value Computations**

```cpp
constexpr int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n-1) + fibonacci(n-2);
}
```

✅ **Algorithm Implementation**

```cpp
constexpr int max_element(const int* arr, size_t size) {
    int max_val = arr[0];
    for (size_t i = 1; i < size; ++i) {
        if (arr[i] > max_val) max_val = arr[i];
    }
    return max_val;
}
```

✅ **Runtime and Compile-Time Use**

```cpp
constexpr int power(int base, int exp) {
    int result = 1;
    for (int i = 0; i < exp; ++i) {
        result *= base;
    }
    return result;
}

// Compile-time
constexpr int p1 = power(2, 8);

// Runtime (if needed)
int base = get_user_input();
int p2 = power(base, 3);
```

✅ **Complex Logic with Loops**

```cpp
constexpr bool is_prime(int n) {
    if (n <= 1) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;

    for (int i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}
```

**Modern C++20 constexpr Enhancements:**

```cpp
// C++20: constexpr with dynamic allocation
constexpr vector<int> make_vector(int size) {
    vector<int> v;
    for (int i = 0; i < size; ++i) {
        v.push_back(i * i);
    }
    return v;
}

// C++20: constexpr with try-catch
constexpr int safe_divide(int a, int b) {
    if (b == 0) throw runtime_error("Division by zero");
    return a / b;
}
```

**Best Practices:**

1. **Prefer constexpr for value computations** - Cleaner, more maintainable
2. **Use TMP for type manipulation** - It excels at compile-time type operations
3. **Combine when needed:**

```cpp
template <typename T>
constexpr size_t bit_size() {
    return sizeof(T) * 8;
}
```

1. **Consider compilation time** - Excessive TMP increases build times
2. **Document TMP heavily** - It's hard to read, needs clear comments

**Example Combining Both:**

```cpp
// TMP for type selection
template <bool Condition, typename T, typename F>
struct conditional {
    using type = T;
};

template <typename T, typename F>
struct conditional<false, T, F> {
    using type = F;
};

// constexpr for value computation
template <typename T>
constexpr bool is_large() {
    return sizeof(T) > 4;
}

// Combined usage
template <typename T>
using storage_type = typename conditional<
    is_large<T>(),
    long long,
    int
>::type;
```

**Conclusion:**

Modern C++ strongly favors `constexpr` functions for value computations—they're clearer, easier to write and debug, and provide the same zero-runtime-overhead benefits. Reserve template metaprogramming for type-level operations, traits, and situations where `constexpr` isn't sufficient. The trend is clear: use `constexpr` when possible, TMP when necessary.

---