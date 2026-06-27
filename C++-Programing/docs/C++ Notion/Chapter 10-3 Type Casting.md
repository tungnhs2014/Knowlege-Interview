# 10.3. Type Casting

---

## Table of Contents

1. Introduction to Type Casting
2. C-Style Casting (Legacy)
3. static_cast
4. dynamic_cast
5. const_cast
6. reinterpret_cast
7. Comparison of Cast Operators
8. Best Practices
9. Common Pitfalls
10. Summary

---

## 1. Introduction to Type Casting

### What is Type Casting?

**Type casting** (or type conversion) is the process of converting a value from one data type to another. It allows programmers to explicitly change how the compiler interprets data.

**Think of it as:** Translating data from one "language" (type) to another.

### Why Do We Need Type Casting?

1. **Interfacing with APIs**: Some functions require specific types
2. **Precision Control**: Convert between int and double for calculations
3. **Polymorphism**: Navigate inheritance hierarchies
4. **Low-level Programming**: Memory manipulation in embedded systems
5. **Legacy Code**: Interface with C libraries

### Types of Conversions in C++

**Implicit Conversion (Automatic):**

```cpp
int a = 10;
double b = a;  // WHY: Compiler automatically converts int to double
```

**Explicit Conversion (Casting):**

```cpp
double x = 3.14;
int y = static_cast<int>(x);  // WHY: Programmer explicitly requests conversion
```

### Why C++ Has 4 Cast Operators?

**C++ introduces 4 specific casts to replace C-style casting:**

| Cast Operator | Purpose |
| --- | --- |
| `static_cast` | Compile-time safe conversions |
| `dynamic_cast` | Runtime-checked polymorphic conversions |
| `const_cast` | Add/remove const qualifier |
| `reinterpret_cast` | Low-level bit reinterpretation |

**WHY 4 casts?**

- **Clarity**: Each cast shows intent
- **Safety**: Compiler can check appropriateness
- **Searchability**: Easy to find and audit casts in code

---

## 2. C-Style Casting (Legacy)

### The Old Way

**Syntax:**

```cpp
(new_type) expression
new_type(expression)  // Function-style
```

### Example: C-Style Cast

```cpp
#include <iostream>
using namespace std;

int main() {
    double pi = 3.14159;

    // WHY: C-style cast - works but hides intent
    int a = (int)pi;              // Cast notation
    int b = int(pi);              // Function notation

    cout << "Original: " << pi << endl;
    cout << "C-style cast (int)pi: " << a << endl;
    cout << "Function-style int(pi): " << b << endl;

    return 0;
}
```

**Output:**

```
Original: 3.14159
C-style cast (int)pi: 3
Function-style int(pi): 3
```

### Problems with C-Style Casts

```cpp
#include <iostream>
using namespace std;

class Base {
public:
    virtual void print() { cout << "Base" << endl; }
};

class Derived : public Base {
public:
    void print() override { cout << "Derived" << endl; }
    void derivedOnly() { cout << "Derived only function" << endl; }
};

class Unrelated {
public:
    void unrelatedFunc() { cout << "Unrelated" << endl; }
};

int main() {
    Base* basePtr = new Derived();

    // WHY BAD: C-style cast hides dangerous operations
    // This looks innocent but could be any of 4 cast types!
    Derived* derivedPtr = (Derived*)basePtr;  // Works, but unsafe syntax

    // WHY VERY BAD: No compile error for completely wrong cast!
    Unrelated* unrelPtr = (Unrelated*)basePtr;  // Compiles but WRONG!
    // unrelPtr->unrelatedFunc();  // Undefined behavior!

    delete basePtr;
    return 0;
}
```

**Why C-style casts are dangerous:**

1. **Hides Intent**: Can't tell what kind of conversion is happening
2. **No Compile-time Check**: Allows dangerous conversions
3. **Hard to Find**: Difficult to search/audit in code
4. **Unpredictable**: Tries multiple casts in sequence

### Cast Sequence of C-Style Cast

When you use `(Type)expression`, the compiler tries these casts in order:

1. `const_cast`
2. `static_cast`
3. `reinterpret_cast`
4. `static_cast` + `const_cast`
5. `reinterpret_cast` + `const_cast`

**Conclusion:** Never use C-style casts in modern C++!

---

## 3. static_cast

### What is static_cast?

**static_cast** is the most commonly used cast operator in C++. It performs **compile-time** type conversion for explicit conversions that are considered safe.

**Syntax:**

```cpp
static_cast<new_type>(expression)
```

### When to Use static_cast

1. **Numeric conversions** (int ↔ double)
2. **Pointer upcasting** (Derived* → Base*)
3. **Pointer downcasting** (when you know the type)
4. **Enum conversions**
5. *void to typed pointer*

### Example 1: Numeric Conversions

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: Convert between numeric types safely
    int intVal = 42;
    double doubleVal = 3.14159;

    // int to double (widening - safe)
    double d = static_cast<double>(intVal);
    cout << "int to double: " << d << endl;

    // double to int (narrowing - truncates)
    int i = static_cast<int>(doubleVal);
    cout << "double to int: " << i << endl;

    // char to int (ASCII value)
    char c = 'A';
    int ascii = static_cast<int>(c);
    cout << "char 'A' to int: " << ascii << endl;

    // int to char
    int num = 66;
    char ch = static_cast<char>(num);
    cout << "int 66 to char: " << ch << endl;

    return 0;
}
```

**Output:**

```
int to double: 42
double to int: 3
char 'A' to int: 65
int 66 to char: B

```

**Explanation:**

- `static_cast` performs safe numeric conversions at compile time
- Truncation warnings may occur for narrowing conversions
- Preferred over implicit conversions for clarity

### Example 2: Pointer Upcasting (Derived → Base)

```cpp
#include <iostream>
using namespace std;

class Animal {
public:
    virtual void speak() { cout << "Animal speaks" << endl; }
    virtual ~Animal() {}
};

class Dog : public Animal {
public:
    void speak() override { cout << "Dog barks" << endl; }
    void fetch() { cout << "Dog fetches" << endl; }
};

int main() {
    Dog dog;

    // WHY: Upcasting is always safe (Derived → Base)
    // Can use implicit conversion, but static_cast is explicit
    Animal* animalPtr = static_cast<Animal*>(&dog);

    animalPtr->speak();  // Calls Dog::speak() due to virtual

    return 0;
}
```

**Output:**

```
Dog barks
```

**Note:** Upcasting is always safe and doesn't strictly require a cast, but `static_cast` makes the intent clear.

### Example 3: Pointer Downcasting (Base → Derived)

```cpp
#include <iostream>
using namespace std;

class Base {
public:
    virtual void print() { cout << "Base" << endl; }
    virtual ~Base() {}
};

class Derived : public Base {
public:
    void print() override { cout << "Derived" << endl; }
    void derivedMethod() { cout << "Derived-specific method" << endl; }
};

int main() {
    // WHY: We KNOW basePtr actually points to a Derived object
    Base* basePtr = new Derived();

    // Downcasting with static_cast (no runtime check!)
    Derived* derivedPtr = static_cast<Derived*>(basePtr);

    derivedPtr->print();
    derivedPtr->derivedMethod();  // Can access Derived methods

    delete basePtr;
    return 0;
}
```

**Output:**

```
Derived
Derived-specific method
```

**Warning:** `static_cast` does NOT check if the cast is valid at runtime. If `basePtr` pointed to an actual `Base` object (not `Derived`), the code would have undefined behavior!

### Example 4: Enum Conversions

```cpp
#include <iostream>
using namespace std;

enum class Color { Red, Green, Blue };
enum OldStyle { Low = 0, Medium = 1, High = 2 };

int main() {
    // WHY: Convert between enum and int
    Color c = Color::Green;

    // enum class to int (requires explicit cast)
    int colorNum = static_cast<int>(c);
    cout << "Color::Green as int: " << colorNum << endl;

    // int to enum class
    Color c2 = static_cast<Color>(2);
    cout << "2 as Color: Blue" << endl;

    // Old-style enum to int (implicit works, but static_cast is clearer)
    OldStyle level = Medium;
    int levelNum = static_cast<int>(level);
    cout << "Medium as int: " << levelNum << endl;

    return 0;
}
```

**Output:**

```
Color::Green as int: 1
2 as Color: Blue
Medium as int: 1
```

### Example 5: void* Conversion

```cpp
#include <iostream>
using namespace std;

int main() {
    int value = 42;

    // WHY: void* is a generic pointer, must cast to use
    void* voidPtr = &value;  // Implicit conversion to void*

    // void* back to int*
    int* intPtr = static_cast<int*>(voidPtr);

    cout << "Value through int*: " << *intPtr << endl;

    return 0;
}
```

**Output:**

```
Value through int*: 42
```

### What static_cast Cannot Do

```cpp
#include <iostream>
using namespace std;

int main() {
    int num = 42;

    // ERROR: Cannot remove const with static_cast
    const int* constPtr = &num;
    // int* nonConstPtr = static_cast<int*>(constPtr);  // ERROR!

    // ERROR: Cannot cast between unrelated types
    double d = 3.14;
    // int* intPtr = static_cast<int*>(&d);  // ERROR!

    return 0;
}
```

---

## 4. dynamic_cast

### What is dynamic_cast?

**dynamic_cast** is used for **safe downcasting** in inheritance hierarchies. It performs a **runtime check** using RTTI (Run-Time Type Information) to ensure the conversion is valid.

**Syntax:**

```cpp
dynamic_cast<new_type>(expression)
```

### Requirements for dynamic_cast

1. **Polymorphic classes** (at least one virtual function)
2. **Pointers or references** only
3. **RTTI must be enabled** (usually default)

### Behavior

- **Pointer cast fails**: Returns `nullptr`
- **Reference cast fails**: Throws `std::bad_cast` exception

### Example 1: Safe Downcasting with Pointers

```cpp
#include <iostream>
using namespace std;

class Animal {
public:
    // WHY: Must have virtual function for dynamic_cast (polymorphism)
    virtual void speak() { cout << "Animal speaks" << endl; }
    virtual ~Animal() {}
};

class Dog : public Animal {
public:
    void speak() override { cout << "Dog barks" << endl; }
    void fetch() { cout << "Dog fetches ball" << endl; }
};

class Cat : public Animal {
public:
    void speak() override { cout << "Cat meows" << endl; }
    void scratch() { cout << "Cat scratches" << endl; }
};

int main() {
    Animal* animal1 = new Dog();
    Animal* animal2 = new Cat();

    // WHY: Safe downcasting - runtime check!
    Dog* dogPtr = dynamic_cast<Dog*>(animal1);
    if (dogPtr) {
        cout << "Successfully cast to Dog!" << endl;
        dogPtr->fetch();  // Safe to call
    }

    // WHY: This cast will FAIL at runtime
    Dog* wrongDog = dynamic_cast<Dog*>(animal2);  // animal2 is actually a Cat!
    if (wrongDog == nullptr) {
        cout << "Cast to Dog failed - animal2 is not a Dog!" << endl;
    }

    // WHY: Cast to correct type succeeds
    Cat* catPtr = dynamic_cast<Cat*>(animal2);
    if (catPtr) {
        cout << "Successfully cast to Cat!" << endl;
        catPtr->scratch();
    }

    delete animal1;
    delete animal2;
    return 0;
}
```

**Output:**

```
Successfully cast to Dog!
Dog fetches ball
Cast to Dog failed - animal2 is not a Dog!
Successfully cast to Cat!
Cat scratches
```

**Explanation:**

- `dynamic_cast` checks at runtime if the conversion is valid
- Returns `nullptr` if the object isn't actually of the target type
- Safe to use in polymorphic hierarchies

### Example 2: Reference Casting with bad_cast

```cpp
#include <iostream>
#include <typeinfo>  // For bad_cast
using namespace std;

class Base {
public:
    virtual void print() { cout << "Base" << endl; }
    virtual ~Base() {}
};

class Derived : public Base {
public:
    void print() override { cout << "Derived" << endl; }
    void derivedOnly() { cout << "Derived only" << endl; }
};

int main() {
    Base base;
    Derived derived;

    Base& refToBase = base;
    Base& refToDerived = derived;

    // WHY: Reference casting - throws exception on failure
    try {
        // This will FAIL - refToBase refers to an actual Base
        Derived& d1 = dynamic_cast<Derived&>(refToBase);
        d1.derivedOnly();  // Won't reach here
    } catch (const bad_cast& e) {
        cout << "Caught bad_cast: " << e.what() << endl;
    }

    // This will SUCCEED - refToDerived refers to a Derived
    try {
        Derived& d2 = dynamic_cast<Derived&>(refToDerived);
        d2.derivedOnly();  // This works!
    } catch (const bad_cast& e) {
        cout << "Caught bad_cast: " << e.what() << endl;
    }

    return 0;
}
```

**Output:**

```
Caught bad_cast: std::bad_cast
Derived only
```

### Example 3: Cross-casting in Multiple Inheritance

```cpp
#include <iostream>
using namespace std;

class Printable {
public:
    virtual void print() = 0;
    virtual ~Printable() {}
};

class Serializable {
public:
    virtual void serialize() = 0;
    virtual ~Serializable() {}
};

class Document : public Printable, public Serializable {
public:
    void print() override { cout << "Printing document" << endl; }
    void serialize() override { cout << "Serializing document" << endl; }
};

int main() {
    Document doc;

    // Get pointer to one interface
    Printable* printPtr = &doc;

    // WHY: Cross-cast to sibling interface
    Serializable* serPtr = dynamic_cast<Serializable*>(printPtr);

    if (serPtr) {
        cout << "Cross-cast succeeded!" << endl;
        serPtr->serialize();
    }

    return 0;
}
```

**Output:**

```
Cross-cast succeeded!
Serializing document
```

### dynamic_cast vs static_cast

| Aspect | dynamic_cast | static_cast |
| --- | --- | --- |
| **Check** | Runtime | Compile-time |
| **Speed** | Slower (RTTI) | Faster |
| **Safety** | ✅ Safe | ⚠️ May be unsafe |
| **Failure** | nullptr / bad_cast | Undefined behavior |
| **Requirement** | Virtual functions | None |
| **Use Case** | Unknown type | Known type |

### When NOT to Use dynamic_cast

```cpp
// DON'T: When you know the exact type
Base* ptr = new Derived();
Derived* d = static_cast<Derived*>(ptr);  // Faster, we know the type

// DON'T: Non-polymorphic classes
class NonPoly {};
// dynamic_cast requires virtual functions!

// DON'T: Performance-critical code (consider design changes)
```

---

## 5. const_cast

### What is const_cast?

**const_cast** is the only C++ cast that can add or remove `const` (and `volatile`) qualifiers. No other cast can do this.

**Syntax:**

```cpp
const_cast<new_type>(expression)
```

### When to Use const_cast

1. **Interface with const-incorrect APIs**: Legacy C functions
2. **Overload resolution**: Calling non-const version
3. **Modifying mutable-like data**: When you know it's safe

### Example 1: Removing const for Legacy Functions

```cpp
#include <iostream>
using namespace std;

// WHY: Legacy C function that doesn't accept const (bad design)
void legacyPrint(char* str) {
    cout << "Printing: " << str << endl;
    // Note: This function does NOT modify str
}

int main() {
    const char* message = "Hello, World!";

    // WHY: Need to call legacy function with const string
    // We KNOW legacyPrint doesn't modify the string
    legacyPrint(const_cast<char*>(message));

    return 0;
}
```

**Output:**

```
Printing: Hello, World!
```

### Example 2: const Member Function Modifying Data

```cpp
#include <iostream>
using namespace std;

class Counter {
    int count;
public:
    Counter(int c) : count(c) {}

    // WHY: Const function that needs to modify internal state
    // This is a workaround - prefer using 'mutable' keyword instead
    void increment() const {
        // count++;  // ERROR: Can't modify in const function

        // WHY: const_cast removes const from 'this' pointer
        const_cast<Counter*>(this)->count++;
    }

    int getCount() const { return count; }
};

int main() {
    const Counter c(5);

    c.increment();  // Calling const function
    cout << "Count: " << c.getCount() << endl;

    return 0;
}
```

**Output:**

```
Count: 6

```

**Note:** Using `mutable` keyword is usually better than `const_cast` for this pattern.

### Example 3: Safe vs Unsafe const_cast

```cpp
#include <iostream>
using namespace std;

int main() {
    // SAFE: Variable was not originally const
    int x = 10;
    const int* constPtr = &x;  // Pointer to const, but x is not const

    int* modifiablePtr = const_cast<int*>(constPtr);
    *modifiablePtr = 20;  // SAFE: x was never truly const

    cout << "x after modification: " << x << endl;

    // UNSAFE: Variable IS originally const
    const int y = 100;
    int* badPtr = const_cast<int*>(&y);
    // *badPtr = 200;  // UNDEFINED BEHAVIOR!
    // y is actually const, modifying it is UB

    return 0;
}
```

**Output:**

```
x after modification: 20
```

**Critical Rule:** Modifying a value that was originally declared `const` is **undefined behavior**, even after using `const_cast`.

### Example 4: Adding const

```cpp
#include <iostream>
using namespace std;

void processReadOnly(const int* ptr) {
    cout << "Processing read-only: " << *ptr << endl;
}

int main() {
    int value = 42;
    int* ptr = &value;

    // WHY: Adding const to call read-only function
    const int* constPtr = const_cast<const int*>(ptr);
    processReadOnly(constPtr);

    // Note: Adding const doesn't require const_cast (implicit works)
    processReadOnly(ptr);  // This also works!

    return 0;
}
```

**Output:**

```
Processing read-only: 42
Processing read-only: 42
```

### Example 5: Removing volatile

```cpp
#include <iostream>
#include <typeinfo>
using namespace std;

int main() {
    volatile int sensorValue = 100;

    // WHY: const_cast can also handle volatile
    const volatile int* cvPtr = &sensorValue;

    cout << "Original type: " << typeid(cvPtr).name() << endl;

    // Remove volatile (and const)
    int* normalPtr = const_cast<int*>(cvPtr);

    cout << "After const_cast: " << typeid(normalPtr).name() << endl;

    return 0;
}
```

### const_cast Type Safety

```cpp
#include <iostream>
using namespace std;

int main() {
    int a = 40;
    const int* constIntPtr = &a;

    // WHY: const_cast preserves type - only changes const
    int* intPtr = const_cast<int*>(constIntPtr);  // OK

    // ERROR: Cannot change type with const_cast
    // char* charPtr = const_cast<char*>(constIntPtr);  // ERROR!

    cout << "Value: " << *intPtr << endl;

    return 0;
}
```

---

## 6. reinterpret_cast

### What is reinterpret_cast?

**reinterpret_cast** is the most powerful and dangerous cast. It performs low-level bit pattern reinterpretation without any type checking.

**Syntax:**

```cpp
reinterpret_cast<new_type>(expression)
```

### Characteristics

- **No runtime cost** (except for some pointer conversions)
- **No type safety**
- **Platform-specific** and non-portable
- **Compile-time directive** - just tells compiler to treat bits differently

### When to Use reinterpret_cast

1. **Pointer to integer** conversion
2. **Unrelated pointer types** conversion
3. **Hardware addresses** (embedded systems)
4. **Serialization/deserialization**
5. **C-style opaque handles**

### Example 1: Pointer to Integer

```cpp
#include <iostream>
#include <cstdint>  // For uintptr_t
using namespace std;

int main() {
    int value = 42;
    int* ptr = &value;

    // WHY: Store pointer value as integer (for debugging, hashing, etc.)
    uintptr_t address = reinterpret_cast<uintptr_t>(ptr);

    cout << "Pointer address: " << ptr << endl;
    cout << "As integer: " << hex << address << dec << endl;

    // WHY: Convert back to pointer
    int* recoveredPtr = reinterpret_cast<int*>(address);
    cout << "Recovered value: " << *recoveredPtr << endl;

    return 0;
}
```

**Output:**

```
Pointer address: 0x7ffd5c3b9a5c
As integer: 7ffd5c3b9a5c
Recovered value: 42
```

### Example 2: Inspecting Memory Layout

```cpp
#include <iostream>
using namespace std;

struct Data {
    int a;
    int b;
    char c;
};

int main() {
    Data data = {10, 20, 'X'};

    // WHY: Inspect structure's memory byte by byte
    char* bytePtr = reinterpret_cast<char*>(&data);

    cout << "Memory layout of Data struct:" << endl;
    cout << "Size: " << sizeof(Data) << " bytes" << endl;
    cout << "Bytes: ";

    for (size_t i = 0; i < sizeof(Data); i++) {
        cout << static_cast<int>(static_cast<unsigned char>(bytePtr[i])) << " ";
    }
    cout << endl;

    // WHY: Access first int through char pointer
    int* firstInt = reinterpret_cast<int*>(bytePtr);
    cout << "First int (a): " << *firstInt << endl;

    return 0;
}
```

**Output:**

```
Memory layout of Data struct:
Size: 12 bytes
Bytes: 10 0 0 0 20 0 0 0 88 0 0 0
First int (a): 10
```

### Example 3: Hardware Address Access (Embedded)

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: In embedded systems, hardware registers have fixed addresses
    // This is a simulation - don't run on real addresses!

    // Simulated hardware register address
    // volatile uint32_t* gpioPort = reinterpret_cast<volatile uint32_t*>(0x40020000);

    // For demonstration with safe memory:
    uint32_t simulatedRegister = 0;
    volatile uint32_t* regPtr = &simulatedRegister;

    // Write to "register"
    *regPtr = 0xFF;  // Set all bits in lower byte

    cout << "Register value: 0x" << hex << *regPtr << dec << endl;

    return 0;
}
```

**Output:**

```
Register value: 0xff
```

### Example 4: Casting Between Unrelated Pointers

```cpp
#include <iostream>
using namespace std;

class A {
public:
    void funcA() { cout << "Function A" << endl; }
    int valueA = 100;
};

class B {
public:
    void funcB() { cout << "Function B" << endl; }
    int valueB = 200;
};

int main() {
    A objA;

    // WHY: Dangerous! Treating A as B
    B* bPtr = reinterpret_cast<B*>(&objA);

    // This "works" because memory layout happens to be similar
    // But it's UNDEFINED BEHAVIOR!
    cout << "bPtr->valueB: " << bPtr->valueB << endl;  // Reads A's memory

    return 0;
}
```

**Output:**

```
bPtr->valueB: 100
```

**Warning:** This code has undefined behavior! The output depends on memory layout which may differ between compilers/platforms.

### Example 5: Function Pointer Casting

```cpp
#include <iostream>
using namespace std;

void printMessage() {
    cout << "Hello from printMessage!" << endl;
}

int main() {
    // Get function pointer
    void (*funcPtr)() = printMessage;

    // WHY: Store function pointer as void* (for generic storage)
    void* voidPtr = reinterpret_cast<void*>(funcPtr);

    // WHY: Recover function pointer
    void (*recoveredFunc)() = reinterpret_cast<void(*)()>(voidPtr);

    recoveredFunc();  // Call recovered function

    return 0;
}
```

**Output:**

```
Hello from printMessage!
```

### What reinterpret_cast Cannot Do

```cpp
int main() {
    // CANNOT remove const - use const_cast
    const int x = 10;
    // int* ptr = reinterpret_cast<int*>(&x);  // Still const!

    // CANNOT do real type conversion
    int n = 42;
    // double d = reinterpret_cast<double>(n);  // ERROR!
    // reinterpret_cast works on pointers/references, not values

    return 0;
}
```

---

## 7. Comparison of Cast Operators

### Complete Comparison Table

| Aspect | static_cast | dynamic_cast | const_cast | reinterpret_cast |
| --- | --- | --- | --- | --- |
| **Check Time** | Compile | Runtime | Compile | None |
| **Safety** | Medium | High | Low | Very Low |
| **Speed** | Fast | Slow (RTTI) | Fast | Fast |
| **Numeric Conv.** | ✅ Yes | ❌ No | ❌ No | ❌ No |
| **Pointer Upcast** | ✅ Yes | ✅ Yes | ❌ No | ✅ Yes |
| **Pointer Downcast** | ✅ Unsafe | ✅ Safe | ❌ No | ✅ Unsafe |
| **Remove const** | ❌ No | ❌ No | ✅ Yes | ❌ No |
| **Unrelated Types** | ❌ No | ❌ No | ❌ No | ✅ Yes |
| **Requires Virtual** | ❌ No | ✅ Yes | ❌ No | ❌ No |

### Use Case Decision Tree

```
Need to convert types?
│
├── Numeric conversion (int, double, etc.)?
│   └── Use: static_cast
│
├── Pointer in inheritance hierarchy?
│   ├── Know the exact type?
│   │   └── Use: static_cast (faster)
│   └── Unknown type, need safety?
│       └── Use: dynamic_cast (safer)
│
├── Need to add/remove const?
│   └── Use: const_cast
│
├── Low-level bit manipulation?
│   └── Use: reinterpret_cast
│
└── None of the above?
    └── Reconsider your design!
```

### Example: Choosing the Right Cast

```cpp
#include <iostream>
using namespace std;

class Base {
public:
    virtual ~Base() {}
};

class Derived : public Base {
public:
    void derivedFunc() { cout << "Derived function" << endl; }
};

void processConstData(const int* data) {
    // Need non-const for legacy API
    int* nonConst = const_cast<int*>(data);
    // Use nonConst...
}

int main() {
    // Scenario 1: Numeric conversion
    double d = 3.14;
    int i = static_cast<int>(d);  // ✅ static_cast

    // Scenario 2: Known downcast
    Base* knownDerived = new Derived();
    Derived* d1 = static_cast<Derived*>(knownDerived);  // ✅ static_cast
    d1->derivedFunc();

    // Scenario 3: Unknown type at runtime
    Base* unknownType = new Derived();
    Derived* d2 = dynamic_cast<Derived*>(unknownType);  // ✅ dynamic_cast
    if (d2) {
        d2->derivedFunc();
    }

    // Scenario 4: Remove const
    const int val = 10;
    processConstData(&val);  // Uses const_cast internally

    // Scenario 5: Pointer to integer
    int* ptr = &i;
    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);  // ✅ reinterpret_cast

    delete knownDerived;
    delete unknownType;
    return 0;
}
```

---

## 8. Best Practices

### ✅ DO: Prefer static_cast for Normal Conversions

```cpp
// GOOD: Explicit and safe
double pi = 3.14159;
int truncated = static_cast<int>(pi);

// BAD: C-style cast
int bad = (int)pi;
```

### ✅ DO: Use dynamic_cast When Type is Unknown

```cpp
void processAnimal(Animal* animal) {
    // GOOD: Safe runtime check
    if (Dog* dog = dynamic_cast<Dog*>(animal)) {
        dog->fetch();
    } else if (Cat* cat = dynamic_cast<Cat*>(animal)) {
        cat->scratch();
    }
}
```

### ✅ DO: Check dynamic_cast Results

```cpp
// GOOD: Always check for nullptr
Derived* d = dynamic_cast<Derived*>(basePtr);
if (d != nullptr) {
    d->derivedMethod();
}

// For references, use try-catch
try {
    Derived& d = dynamic_cast<Derived&>(baseRef);
    d.derivedMethod();
} catch (const bad_cast& e) {
    // Handle error
}
```

### ✅ DO: Minimize const_cast Usage

```cpp
// GOOD: Only when absolutely necessary
void legacyApi(char* str);  // Can't change this

void modernWrapper(const string& str) {
    // Document why const_cast is needed
    legacyApi(const_cast<char*>(str.c_str()));
}
```

### ✅ DO: Document reinterpret_cast Usage

```cpp
// GOOD: Clear documentation
// WHY: Converting hardware register address to pointer
// SAFETY: Address 0x40020000 is valid GPIO port on this MCU
volatile uint32_t* gpioPort = reinterpret_cast<volatile uint32_t*>(0x40020000);
```

### ❌ DON'T: Use C-Style Casts

```cpp
// BAD: Hides intent, dangerous
Derived* d = (Derived*)basePtr;

// GOOD: Clear intent
Derived* d = static_cast<Derived*>(basePtr);
// or
Derived* d = dynamic_cast<Derived*>(basePtr);
```

### ❌ DON'T: Modify Originally Const Data

```cpp
const int value = 10;
int* ptr = const_cast<int*>(&value);
*ptr = 20;  // UNDEFINED BEHAVIOR!
```

### ❌ DON'T: Use reinterpret_cast for Type Conversions

```cpp
// BAD: Wrong use of reinterpret_cast
double d = 3.14;
// int i = reinterpret_cast<int>(d);  // ERROR!

// GOOD: Use static_cast
int i = static_cast<int>(d);
```

---

## 9. Common Pitfalls

### Pitfall 1: Unchecked static_cast Downcast

```cpp
#include <iostream>
using namespace std;

class Base { public: virtual ~Base() {} };
class Derived : public Base { public: int data = 42; };

int main() {
    Base* actualBase = new Base();  // NOT a Derived!

    // PITFALL: static_cast doesn't check!
    Derived* wrongCast = static_cast<Derived*>(actualBase);

    // UNDEFINED BEHAVIOR: Accessing non-existent data
    // cout << wrongCast->data << endl;  // CRASH or garbage!

    delete actualBase;
    return 0;
}
```

**Solution:** Use `dynamic_cast` when type is uncertain.

### Pitfall 2: Forgetting dynamic_cast Requirements

```cpp
class NonPolymorphic {
    // No virtual functions!
};

class DerivedNP : public NonPolymorphic {};

int main() {
    NonPolymorphic* ptr = new DerivedNP();

    // ERROR: dynamic_cast requires polymorphic type!
    // DerivedNP* d = dynamic_cast<DerivedNP*>(ptr);

    delete ptr;
    return 0;
}
```

**Solution:** Add at least one virtual function (typically destructor).

### Pitfall 3: Assuming reinterpret_cast is Portable

```cpp
#include <iostream>
using namespace std;

int main() {
    float f = 3.14f;

    // PITFALL: Behavior is implementation-defined!
    int* intPtr = reinterpret_cast<int*>(&f);
    cout << *intPtr << endl;  // Different results on different systems!

    return 0;
}
```

**Solution:** Avoid relying on reinterpret_cast for portable code.

### Pitfall 4: Modifying Const Objects

```cpp
#include <iostream>
using namespace std;

int main() {
    const int CONSTANT = 100;

    int* hackPtr = const_cast<int*>(&CONSTANT);
    *hackPtr = 200;  // UNDEFINED BEHAVIOR!

    // Compiler may have optimized using original value
    cout << CONSTANT << endl;  // Might still print 100!

    return 0;
}
```

---

## 10. Summary

### Key Takeaways

1. **static_cast**: For compile-time safe conversions
    - Numeric conversions
    - Known type casts in hierarchies
    - Most common cast
2. **dynamic_cast**: For runtime-safe polymorphic conversions
    - Unknown types in hierarchies
    - Returns nullptr or throws bad_cast
    - Requires virtual functions
3. **const_cast**: For const/volatile manipulation
    - Only cast that can remove const
    - Use sparingly
    - Don't modify originally const objects
4. **reinterpret_cast**: For low-level bit manipulation
    - Most dangerous
    - Platform-specific
    - Use only when necessary
5. **C-Style Casts**: Never use in modern C++
    - Hides intent
    - Unsafe
    - Hard to search

### Quick Reference

| Need | Use | Example |
| --- | --- | --- |
| int → double | `static_cast` | `static_cast<double>(i)` |
| Base* → Derived* (known) | `static_cast` | `static_cast<Derived*>(b)` |
| Base* → Derived* (unknown) | `dynamic_cast` | `dynamic_cast<Derived*>(b)` |
| Remove const | `const_cast` | `const_cast<int*>(cp)` |
| Pointer → int | `reinterpret_cast` | `reinterpret_cast<uintptr_t>(p)` |

### Keywords Covered

✅ Type casting (2)
✅ Implicit conversion (1)
✅ Explicit conversion (1)
✅ C-style cast (3)
✅ static_cast (5)
✅ dynamic_cast (5)
✅ const_cast (4)
✅ reinterpret_cast (4)
✅ RTTI (2)
✅ bad_cast exception (2)
✅ Upcasting (2)
✅ Downcasting (3)
✅ Polymorphic types (2)
✅ Type safety (2)
✅ Compile-time casting (2)
✅ Runtime casting (2)

---