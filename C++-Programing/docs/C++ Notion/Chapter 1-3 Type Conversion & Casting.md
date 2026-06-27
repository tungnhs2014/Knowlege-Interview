# 1.3. Type Conversion & Casting

---

## Table of Contents

1. Introduction to Type Conversion
2. Implicit Type Conversion
3. Explicit Type Conversion
4. static_cast
5. dynamic_cast
6. const_cast
7. reinterpret_cast
8. Cast Operator Comparison
9. Type Promotion & Demotion

---

## 1. Introduction

**Definition:** Type conversion means converting a value from one data type to another compatible type.

**Purpose:**

- Perform operations on mixed types
- Interface with APIs expecting specific types
- Extract parts of data (e.g., integer part of float)

```cpp
#include <iostream>
using namespace std;

int main() {
    int i = 10;
    char c = 'A';  // ASCII value: 65

    // c is automatically converted to int
    cout << (int)c << endl;  // 65

    // Adding i and c involves type conversion
    int sum = i + c;  // 10 + 65 = 75

    cout << sum;

    return 0;
}
```

---

## 2. Implicit Type Conversion

**Definition:** Automatic conversion performed by the compiler when needed, following a conversion hierarchy.

**Also known as:** Coercion

### 2.1 Type Promotion Hierarchy

```
bool → char → short → int → unsigned int → long → unsigned long → long long → float → double → long double
```

**Rule:** Smaller types are promoted to larger types to prevent data loss.

```cpp
#include <iostream>
using namespace std;

int main() {
    int i = 10;
    char c = 'A';  // ASCII value: 65

    // char promoted to int
    int sum = i + c;  // 10 + 65 = 75
    cout << "sum: " << sum << endl;

    // int promoted to float
    float result = i + 1.5f;  // 11.5
    cout << "result: " << result << endl;

    // int promoted to double
    double d = i / 3.0;  // 3.33333...
    cout << "d: " << d << endl;

    return 0;
}
```

### 2.2 Conversion Rules

```cpp
#include <iostream>
using namespace std;

int main() {
    // 1. Integer promotion
    short s = 10;
    int i = s + 5;  // short → int

    // 2. Floating-point conversion
    float f = 3.14f;
    double d = f + 2.0;  // float → double

    // 3. Mixed arithmetic
    int x = 5;
    double y = 2.5;
    double result = x + y;  // int → double
    cout << "result: " << result << endl;  // 7.5

    // 4. Assignment conversion
    double pi = 3.14159;
    int truncated = pi;  // ⚠️ Narrowing: 3.14159 → 3 (data loss!)
    cout << "truncated: " << truncated << endl;

    return 0;
}
```

### 2.3 When Implicit Conversion Occurs

1. **Operations on mixed types**
    
    ```cpp
    int x = 10;
    double y = x + 2.5;  // int → double
    ```
    
2. **Function arguments**
    
    ```cpp
    void func(double d);
    func(42);  // int → double
    ```
    
3. **Assignments**
    
    ```cpp
    double d = 10;  // int → double
    ```
    

### 2.4 Risks of Implicit Conversion

```cpp
#include <iostream>
using namespace std;

int main() {
    // 1. Data loss - narrowing conversion
    int large = 300;
    char small = large;  // ⚠️ Overflow: only lower 8 bits kept
    cout << "small: " << (int)small << endl;  // 44 (not 300!)

    // 2. Sign loss
    int negative = -1;
    unsigned int positive = negative;  // ⚠️ Becomes very large number
    cout << "positive: " << positive << endl;  // 4294967295

    // 3. Precision loss
    double precise = 3.99999;
    int imprecise = precise;  // ⚠️ Becomes 3
    cout << "imprecise: " << imprecise << endl;

    return 0;
}
```

---

## 3. Explicit Type Conversion

**Definition:** Programmer manually forces conversion using cast operators.

**Also known as:** Type casting

### 3.1 C-Style Casting (Old, Not Recommended)

```cpp
#include <iostream>
using namespace std;

int main() {
    double x = 3.14;

    // C-style cast (prefix notation)
    int y = (int)x;  // 3

    // Function-style cast
    int z = int(x);  // 3

    cout << "y: " << y << ", z: " << z << endl;

    return 0;
}
```

**Problems with C-style casts:**

- No type safety checks
- Can perform dangerous conversions
- Hard to find in code reviews
- Bypasses const correctness
- Cannot distinguish between different types of conversions

**Modern C++ solution:** Use C++ cast operators instead.

---

## 4. static_cast

**Purpose:** Standard compile-time type conversion between related types.

**Syntax:** `static_cast<new_type>(expression)`

**When to use:**

- Numeric conversions (int ↔ double)
- Pointer conversions up/down class hierarchies (non-polymorphic)
- Explicit conversions that the compiler could do implicitly
- Reversing implicit conversions

```cpp
#include <iostream>
using namespace std;

int main() {
    // 1. Numeric conversions
    double d = 3.14159;
    int i = static_cast<int>(d);  // Explicit truncation: 3
    cout << "i: " << i << endl;

    // 2. Avoid narrowing warnings
    char c = static_cast<char>(65);  // Explicit: 'A'
    cout << "c: " << c << endl;

    // 3. Void pointer conversion
    void* void_ptr = &i;
    int* int_ptr = static_cast<int*>(void_ptr);  // Safe conversion back
    cout << "*int_ptr: " << *int_ptr << endl;

    // 4. Enum to int
    enum Color { RED, GREEN, BLUE };
    Color color = RED;
    int color_value = static_cast<int>(color);  // 0
    cout << "color_value: " << color_value << endl;

    // 5. Float to int (explicit)
    float f = 9.99f;
    int truncated = static_cast<int>(f);  // 9
    cout << "truncated: " << truncated << endl;

    return 0;
}
```

**Safety:** Compile-time checked, relatively safe.

**Cannot do:**

- Remove const (use `const_cast`)
- Downcast polymorphic classes without checks (use `dynamic_cast`)

---

## 5. dynamic_cast

**Purpose:** Runtime type checking for polymorphic classes (classes with virtual functions).

**Syntax:** `dynamic_cast<new_type>(expression)`

**When to use:**

- Downcasting in inheritance hierarchies
- Verify object type at runtime
- Safe conversion of pointers/references

**Requirement:** Both classes must be polymorphic (have at least one virtual function).

```cpp
#include <iostream>
using namespace std;

class Base {
public:
    virtual ~Base() {}  // Must have virtual function
};

class Derived : public Base {
public:
    void derivedMethod() {
        cout << "Derived method called" << endl;
    }
};

int main() {
    // 1. Successful downcast
    Base* base_ptr = new Derived();

    // Safe downcast with runtime check
    Derived* derived_ptr = dynamic_cast<Derived*>(base_ptr);

    if (derived_ptr != nullptr) {
        cout << "Successful downcast" << endl;
        derived_ptr->derivedMethod();
    } else {
        cout << "Downcast failed" << endl;
    }

    // 2. Failed downcast
    Base* base_only = new Base();
    Derived* failed_cast = dynamic_cast<Derived*>(base_only);

    if (failed_cast == nullptr) {
        cout << "Cannot cast Base to Derived" << endl;
    }

    delete base_ptr;
    delete base_only;

    return 0;
}
```

**Output:**

```
Successful downcast
Derived method called
Cannot cast Base to Derived
```

**Return values:**

- **Pointer cast:** Returns `nullptr` if cast fails
- **Reference cast:** Throws `std::bad_cast` exception if cast fails

**Safety:** Very safe - runtime checking prevents invalid casts.

**Cost:** Slight runtime overhead due to type checking.

---

## 6. const_cast

**Purpose:** Add or remove `const` or `volatile` qualifiers.

**Syntax:** `const_cast<new_type>(expression)`

**When to use:**

- Interfacing with legacy C APIs that don't use const
- Temporarily removing const (dangerous!)

```cpp
#include <iostream>
using namespace std;

void legacyFunction(char* str) {  // Old API without const
    cout << str << endl;
}

int main() {
    const char* const_str = "Hello";

    // Remove const to call legacy function
    char* mutable_str = const_cast<char*>(const_str);
    legacyFunction(mutable_str);

    // Adding const (safe)
    int value = 42;
    const int* const_value = const_cast<const int*>(&value);

    // ⚠️ DANGER: Modifying const object is undefined behavior!
    const int truly_const = 100;
    int* dangerous = const_cast<int*>(&truly_const);
    // *dangerous = 200;  // UNDEFINED BEHAVIOR! DO NOT DO THIS!

    return 0;
}
```

**Safety:** Dangerous! Only use when absolutely necessary.

**Critical Warning:** Modifying a truly const object leads to undefined behavior.

**Valid use case:**

```cpp
class MyClass {
    mutable int cache;
public:
    int getValue() const {
        // Instead of const_cast, use mutable
        cache++;  // OK with mutable
        return cache;
    }
};
```

---

## 7. reinterpret_cast

**Purpose:** Low-level reinterpretation of bit patterns (very dangerous!).

**Syntax:** `reinterpret_cast<new_type>(expression)`

**When to use:**

- Converting between unrelated pointer types
- Pointer ↔ integer conversions
- Low-level system programming
- Hardware interfacing

```cpp
#include <iostream>
#include <cstdint>
using namespace std;

int main() {
    // 1. Pointer to integer
    int value = 42;
    int* ptr = &value;

    // Convert pointer to integer
    uintptr_t address = reinterpret_cast<uintptr_t>(ptr);
    cout << "Address: 0x" << hex << address << endl;

    // Convert back
    int* new_ptr = reinterpret_cast<int*>(address);
    cout << dec << "*new_ptr: " << *new_ptr << endl;

    // 2. Reinterpret memory
    float f = 3.14f;
    int* f_as_int = reinterpret_cast<int*>(&f);
    cout << "Float as int bits: " << *f_as_int << endl;

    // 3. Unrelated pointer types
    char c = 'A';
    void* void_ptr = &c;
    int* int_ptr = reinterpret_cast<int*>(void_ptr);
    // Accessing *int_ptr is dangerous!

    return 0;
}
```

**Safety:** Very dangerous! No type checking at all.

**Use cases (rare):**

- Memory-mapped hardware
- Serialization/deserialization
- Cryptography
- Network protocols
- Low-level performance optimization

**Warning:** Platform-dependent, can cause:

- Memory alignment issues
- Undefined behavior
- Crashes

---

## 8. Cast Operator Comparison

| Cast | Safety | Speed | Type Checking | Use Case |
| --- | --- | --- | --- | --- |
| `static_cast` | Medium | Fast | Compile-time | Standard conversions, numeric types |
| `dynamic_cast` | High | Slow | Runtime | Polymorphic downcasting |
| `const_cast` | Low | Fast | None | Remove/add const (dangerous) |
| `reinterpret_cast` | Very Low | Fast | None | Low-level pointer manipulation |

### 8.1 When to Use Which Cast

```cpp
// Use static_cast for:
int i = static_cast<int>(3.14);                    // Numeric conversion
Base* b = static_cast<Base*>(derived_ptr);         // Upcast (safe)

// Use dynamic_cast for:
Derived* d = dynamic_cast<Derived*>(base_ptr);     // Safe downcast

// Use const_cast for:
char* str = const_cast<char*>(const_str);          // Legacy API

// Use reinterpret_cast for:
uintptr_t addr = reinterpret_cast<uintptr_t>(ptr); // Pointer to int
```

### 8.2 Best Practices

1. **Prefer `static_cast` for most conversions**
    - Safest general-purpose cast
    - Compile-time checking
2. **Use `dynamic_cast` for polymorphic hierarchies**
    - Runtime safety
    - Null-check results
3. **Avoid `const_cast` unless interfacing with legacy code**
    - Breaking const correctness is dangerous
    - Consider redesign instead
4. **Avoid `reinterpret_cast` unless doing low-level programming**
    - Only for system/hardware programming
    - Platform-dependent
5. **Never use C-style casts in modern C++**
    - Use appropriate C++ cast operators
    - Makes intent clear

---

## 9. Type Promotion & Demotion

### 9.1 Type Promotion (Safe)

**Widening conversion:** Smaller type to larger type - no data loss.

```cpp
#include <iostream>
using namespace std;

int main() {
    // Promotion (safe)
    char c = 'A';      // 1 byte
    int i = c;         // 4 bytes - promoted to int (65)
    cout << "Promoted: " << i << endl;

    float f = 3.14f;   // 4 bytes
    double d = f;      // 8 bytes - promoted to double
    cout << "Promoted: " << d << endl;

    return 0;
}
```

**Hierarchy:**

```
char → short → int → long → long long → float → double → long double
```

### 9.2 Type Demotion (Narrowing, Data Loss)

**Narrowing conversion:** Larger type to smaller type - possible data loss.

```cpp
#include <iostream>
using namespace std;

int main() {
    // Demotion (narrowing, data loss)
    int large = 300;
    char small = static_cast<char>(large);  // Truncates!
    cout << "Demoted: " << (int)small << endl;  // 44 (300 % 256)

    double precise = 3.99;
    int rounded = static_cast<int>(precise);  // Loses decimal
    cout << "Demoted: " << rounded << endl;  // 3

    return 0;
}
```

### 9.3 Usual Arithmetic Conversions

**Rule:** In mixed-type arithmetic, both operands are converted to the "common type".

```cpp
#include <iostream>
#include <typeinfo>
using namespace std;

int main() {
    int x = 10;
    double y = 5.5;

    // x is promoted to double
    auto result = x + y;  // result is double

    cout << "result: " << result << endl;              // 15.5
    cout << "type: " << typeid(result).name() << endl; // double

    return 0;

```

---

## Summary

### Key Takeaways

1. **Implicit Conversion** - Automatic by compiler
    - Follows promotion hierarchy
    - Can cause data loss (narrowing)
    - Prefer explicit casts for clarity
2. **Explicit Conversion** - Manual by programmer
    - Use C++ cast operators, not C-style casts
3. **static_cast** - Standard conversions
    - Numeric conversions
    - Upcasting/downcasting (non-polymorphic)
    - Compile-time checked
4. **dynamic_cast** - Polymorphic downcasting
    - Runtime type checking
    - Returns nullptr on failure (pointers)
    - Throws bad_cast on failure (references)
    - Requires virtual functions
5. **const_cast** - Add/remove const
    - Dangerous - use sparingly
    - Modifying truly const object = undefined behavior
6. **reinterpret_cast** - Low-level reinterpretation
    - Very dangerous
    - No type checking
    - Platform-dependent
    - Only for system programming
7. **Type Promotion** - Safe widening
    - bool → char → short → int → long → float → double
8. **Type Demotion** - Narrowing with data loss
    - Requires explicit cast
    - Be aware of truncation

### Interview Points

**Q: Difference between implicit and explicit conversion?**

- **Implicit:** Automatic by compiler (e.g., int to double)
- **Explicit:** Manual by programmer using cast operators

**Q: What are C++ cast operators?**

- `static_cast`: Standard conversions
- `dynamic_cast`: Runtime polymorphic checking
- `const_cast`: Remove const (dangerous)
- `reinterpret_cast`: Low-level bit reinterpretation (very dangerous)

**Q: When to use dynamic_cast?**

- Downcasting in polymorphic hierarchies
- When runtime type safety is needed
- Classes must have virtual functions

**Q: What is type promotion?**

- Automatic conversion of smaller types to larger types
- Hierarchy: bool → char → int → long → float → double
- Prevents data loss in expressions

**Q: What is narrowing conversion?**

- Converting larger type to smaller type
- Can cause data loss (truncation, overflow)
- Should be explicit with static_cast

**Q: Why avoid C-style casts?**

- No type safety
- Can bypass const correctness
- Hard to search in code
- Unclear intent
- Use C++ cast operators instead

---