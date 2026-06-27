# 3.3. References in C++

---

## Table of Contents

1. What are References?
2. Reference Declaration and Initialization
3. References vs Pointers
4. const References
5. Pass by Reference
6. Return by Reference
7. Lvalue and Rvalue Concepts
8. Lvalue References
9. Rvalue References (C++11)
10. Reference Collapsing and Forwarding References
11. Common Pitfalls and Best Practices

---

## 1. What are References?

### 1.1 Definition

**A reference is an alias (alternative name) for an existing variable.** Unlike pointers, references provide direct access to the original variable without requiring dereference operators. Once initialized, a reference cannot be changed to refer to a different object.

**Core Concept:**

```cpp
#include <iostream>
using namespace std;

int main() {
    int original = 42;

    // WHY: ref is another name for original
    int& ref = original;

    cout << "original: " << original << endl;
    cout << "ref: " << ref << endl;

    // WHY: Modifying through reference changes original
    ref = 100;

    cout << "After ref = 100:" << endl;
    cout << "original: " << original << endl;
    cout << "ref: " << ref << endl;

    // WHY: Both share same address
    cout << "&original: " << &original << endl;
    cout << "&ref: " << &ref << endl;

    return 0;
}
```

**Output:**

```
original: 42
ref: 42
After ref = 100:
original: 100
ref: 100
&original: 0x7ffd12345678
&ref: 0x7ffd12345678
```

### 1.2 Why References Exist

**Purpose:**

1. **Cleaner Syntax**
    - No pointer dereferencing needed
    - Direct variable access
2. **Efficient Parameter Passing**
    - Pass large objects without copying
    - Modify function arguments
3. **Safe Aliasing**
    - Cannot be null (must refer to existing object)
    - Cannot be reseated (permanent binding)
4. **Operator Overloading**
    - Return references from operators
    - Enable chaining (e.g., `cout << a << b`)

**Real-World Applications:**

- Range-based for loops
- Function parameters for large objects
- Operator overloading ([], =, <<, >>)
- STL container element access

### 1.3 Reference Properties

**Key Characteristics:**

1. **Must be initialized** - Cannot exist without binding
2. **Cannot be null** - Always refers to valid object
3. **Cannot be reseated** - Permanent binding to original object
4. **No separate memory** - Alias, not new variable
5. **Implicit dereferencing** - Direct access syntax

```cpp
#include <iostream>
using namespace std;

int main() {
    int value = 10;

    // ✅ MUST initialize
    int& ref1 = value;

    // ❌ CANNOT declare without initialization
    // int& ref2;  // ERROR!

    // ❌ CANNOT be null
    // int& ref3 = nullptr;  // ERROR!

    // ✅ Cannot be reseated
    int another = 20;
    ref1 = another;  // Copies value, doesn't rebind!

    cout << "value: " << value << endl;     // 20 (value changed)
    cout << "ref1: " << ref1 << endl;       // 20
    cout << "another: " << another << endl; // 20

    return 0;
}
```

---

## 2. Reference Declaration and Initialization

### 2.1 Declaration Syntax

**Syntax:**

```cpp
data_type& reference_name = existing_variable;
```

**Examples:**

```cpp
#include <iostream>
using namespace std;

int main() {
    int num = 100;
    double pi = 3.14159;
    string text = "Hello";

    // WHY: Create references to different types
    int& numRef = num;
    double& piRef = pi;
    string& textRef = text;

    cout << "numRef: " << numRef << endl;
    cout << "piRef: " << piRef << endl;
    cout << "textRef: " << textRef << endl;

    return 0;
}
```

### 2.2 Initialization Rules

**Valid Initialization:**

```cpp
#include <iostream>
using namespace std;

int main() {
    int x = 10;

    // ✅ Reference to variable
    int& ref1 = x;

    // ✅ Reference to reference (becomes reference to x)
    int& ref2 = ref1;

    // ✅ const reference can bind to literal
    const int& ref3 = 42;

    cout << "ref1: " << ref1 << endl;
    cout << "ref2: " << ref2 << endl;
    cout << "ref3: " << ref3 << endl;

    return 0;
}
```

**Invalid Initialization:**

```cpp
int main() {
    // ❌ No initialization
    // int& ref1;  // ERROR: must be initialized

    // ❌ Cannot bind non-const reference to literal
    // int& ref2 = 10;  // ERROR: rvalue to lvalue reference

    // ❌ Cannot bind to different type without const
    // double d = 3.14;
    // int& ref3 = d;  // ERROR: type mismatch

    return 0;
}
```

### 2.3 Reference to Array

```cpp
#include <iostream>
using namespace std;

int main() {
    int arr[5] = {1, 2, 3, 4, 5};

    // WHY: Reference to entire array preserves size
    int (&arrRef)[5] = arr;

    // Access via reference
    for (int i = 0; i < 5; i++) {
        cout << arrRef[i] << " ";
    }
    cout << endl;

    // WHY: Size information preserved
    cout << "sizeof(arr): " << sizeof(arr) << endl;
    cout << "sizeof(arrRef): " << sizeof(arrRef) << endl;

    return 0;
}
```

**Output:**

```
1 2 3 4 5
sizeof(arr): 20
sizeof(arrRef): 20
```

---

## 3. References vs Pointers

### 3.1 Comparison Table

| Feature | Reference | Pointer |
| --- | --- | --- |
| **Syntax** | `int& ref = var;` | `int* ptr = &var;` |
| **Access** | Direct (`ref`) | Dereference (`*ptr`) |
| **Nullability** | Cannot be null | Can be `nullptr` |
| **Reassignment** | Cannot be reseated | Can point elsewhere |
| **Initialization** | Must initialize | Can declare first |
| **Arithmetic** | No arithmetic | Pointer arithmetic |
| **Memory** | No separate storage | Has own address |
| **Safety** | Safer (always valid) | Can dangle or be null |

### 3.2 Code Comparison

```cpp
#include <iostream>
using namespace std;

int main() {
    int value = 42;
    int another = 100;

    // REFERENCE
    int& ref = value;
    cout << "ref: " << ref << endl;        // Direct access
    ref = 50;                              // Modify original
    // ref = another;                      // Copies value, doesn't rebind

    // POINTER
    int* ptr = &value;
    cout << "*ptr: " << *ptr << endl;      // Need dereference
    *ptr = 60;                             // Modify original
    ptr = &another;                        // ✅ Can reassign
    ptr = nullptr;                         // ✅ Can be null

    return 0;
}
```

### 3.3 When to Use Each

**Use References When:**

- Parameter cannot be null (guaranteed valid)
- Don't need to reassign
- Want cleaner syntax
- Implementing operator overloading
- Range-based for loops

**Use Pointers When:**

- Need to represent "optional" (nullptr)
- Need to reassign to different objects
- Need pointer arithmetic (arrays)
- Data structures (linked lists, trees)
- C library compatibility

---

## 4. const References

### 4.1 const Reference Basics

```cpp
#include <iostream>
using namespace std;

int main() {
    int value = 10;

    // WHY: const reference prevents modification
    const int& ref = value;

    cout << "ref: " << ref << endl;  // ✅ Read OK

    // ref = 20;  // ❌ ERROR: cannot modify through const reference

    value = 30;  // ✅ Can modify original
    cout << "ref: " << ref << endl;  // 30 (sees change)

    return 0;
}
```

### 4.2 const Reference to Temporary

**Key Feature:** const references can bind to rvalues (temporaries)

```cpp
#include <iostream>
using namespace std;

int main() {
    // ❌ Non-const reference to rvalue - ERROR
    // int& ref1 = 42;

    // ✅ const reference extends temporary's lifetime
    const int& ref2 = 42;
    const int& ref3 = 10 + 20;

    cout << "ref2: " << ref2 << endl;
    cout << "ref3: " << ref3 << endl;

    // WHY: Common in function parameters
    const string& ref4 = string("Temporary");
    cout << "ref4: " << ref4 << endl;

    return 0;
}
```

### 4.3 const Reference in Functions

```cpp
#include <iostream>
#include <string>
using namespace std;

// WHY: Efficient + safe (no copy, no modify)
void displayMessage(const string& msg) {
    cout << msg << endl;
    // msg = "Modified";  // ❌ ERROR
}

// WHY: Can accept both lvalues and rvalues
int sum(const int& a, const int& b) {
    return a + b;
}

int main() {
    string greeting = "Hello";

    displayMessage(greeting);           // lvalue
    displayMessage("World");            // rvalue (temporary)
    displayMessage(string("C++"));      // rvalue

    int x = 5;
    cout << sum(x, 10) << endl;         // lvalue + rvalue
    cout << sum(3, 7) << endl;          // rvalue + rvalue

    return 0;
}
```

---

## 5. Pass by Reference

### 5.1 Basic Pass by Reference

```cpp
#include <iostream>
using namespace std;

// WHY: Modify original variable
void increment(int& num) {
    num++;
}

// WHY: Swap without returning
void swap(int& a, int& b) {
    int temp = a;
    a = b;
    b = temp;
}

int main() {
    int value = 10;
    cout << "Before: " << value << endl;

    increment(value);
    cout << "After increment: " << value << endl;

    int x = 5, y = 15;
    cout << "Before swap: x=" << x << ", y=" << y << endl;
    swap(x, y);
    cout << "After swap: x=" << x << ", y=" << y << endl;

    return 0;
}
```

**Output:**

```
Before: 10
After increment: 11
Before swap: x=5, y=15
After swap: x=15, y=5
```

### 5.2 Efficient Parameter Passing

```cpp
#include <iostream>
#include <vector>
using namespace std;

// ❌ INEFFICIENT: Copies entire vector
void displayBad(vector<int> vec) {
    for (int val : vec) {
        cout << val << " ";
    }
    cout << endl;
}

// ✅ EFFICIENT: No copy, read-only
void displayGood(const vector<int>& vec) {
    for (int val : vec) {
        cout << val << " ";
    }
    cout << endl;
}

int main() {
    vector<int> data(1000000, 42);  // 1 million elements

    // displayBad(data);   // Slow: copies 1 million ints
    displayGood(data);     // Fast: passes reference only

    return 0;
}
```

### 5.3 Multiple Return Values Pattern

```cpp
#include <iostream>
using namespace std;

// WHY: Return multiple values via references
bool divideWithRemainder(int dividend, int divisor,
                         int& quotient, int& remainder) {
    if (divisor == 0) {
        return false;
    }

    quotient = dividend / divisor;
    remainder = dividend % divisor;
    return true;
}

int main() {
    int q, r;

    if (divideWithRemainder(17, 5, q, r)) {
        cout << "Quotient: " << q << endl;
        cout << "Remainder: " << r << endl;
    }

    return 0;
}
```

---

## 6. Return by Reference

### 6.1 Basic Return by Reference

```cpp
#include <iostream>
using namespace std;

int globalValue = 100;

// WHY: Return reference to global variable
int& getGlobal() {
    return globalValue;
}

int main() {
    cout << "globalValue: " << globalValue << endl;

    // WHY: Can assign to returned reference
    getGlobal() = 200;

    cout << "globalValue: " << globalValue << endl;

    return 0;
}
```

**Output:**

```
globalValue: 100
globalValue: 200
```

### 6.2 Dangerous: Returning Reference to Local

```cpp
#include <iostream>
using namespace std;

// ❌ DANGER: Returns reference to destroyed local variable
int& getBadReference() {
    int local = 42;
    return local;  // DANGLING REFERENCE!
}

int main() {
    // int& ref = getBadReference();  // UNDEFINED BEHAVIOR!
    // cout << ref << endl;            // May crash or garbage

    return 0;
}
```

### 6.3 Operator Overloading with References

```cpp
#include <iostream>
using namespace std;

class Counter {
    int count;

public:
    Counter() : count(0) {}

    // WHY: Return reference for chaining
    Counter& operator++() {  // Prefix
        count++;
        return *this;
    }

    Counter& operator=(int value) {
        count = value;
        return *this;
    }

    int getCount() const { return count; }
};

int main() {
    Counter c;

    // WHY: Chaining works due to reference return
    ++c;
    ++++c;  // Increment twice

    cout << "Count: " << c.getCount() << endl;

    return 0;
}
```

---

## 7. Lvalue and Rvalue Concepts

### 7.1 Definitions

**Lvalue (Locator Value):**

- Has a specific memory location
- Can appear on left side of assignment
- Has a name or can be referenced
- Persists beyond single expression

**Rvalue (Right Value):**

- Temporary value
- Cannot appear on left side of assignment
- No name (anonymous)
- Temporary, expires after expression

```cpp
#include <iostream>
using namespace std;

int main() {
    // LVALUES
    int x = 10;           // x is lvalue
    int* ptr = &x;        // Can take address
    x = 20;               // Can assign to it

    // RVALUES
    int y = 5 + 3;        // (5 + 3) is rvalue
    // int* p = &(5 + 3); // ❌ ERROR: cannot take address
    // (5 + 3) = 10;      // ❌ ERROR: cannot assign

    return 0;
}
```

### 7.2 Identifying Lvalues and Rvalues

**Rule of Thumb:** If you can take its address (`&`), it's an lvalue

```cpp
#include <iostream>
using namespace std;

int getValue() { return 42; }
int& getReference() {
    static int x = 100;
    return x;
}

int main() {
    int a = 10;

    // LVALUES - has address
    cout << &a << endl;              // ✅ OK
    cout << &getReference() << endl; // ✅ OK (returns lvalue)

    // RVALUES - no address
    // cout << &42 << endl;          // ❌ ERROR
    // cout << &(a + 5) << endl;     // ❌ ERROR
    // cout << &getValue() << endl;  // ❌ ERROR

    return 0;
}
```

### 7.3 Lvalue to Rvalue Conversion

```cpp
#include <iostream>
using namespace std;

int main() {
    int x = 10;  // x is lvalue
    int y = x;   // x (lvalue) converted to rvalue for assignment

    // WHY: Lvalue can be used where rvalue expected
    int z = x + 5;  // x converted to its value (10)

    return 0;
}
```

---

## 8. Lvalue References

### 8.1 Lvalue Reference Basics

```cpp
#include <iostream>
using namespace std;

int main() {
    int value = 42;

    // WHY: Lvalue reference binds to lvalue
    int& lref = value;

    cout << "lref: " << lref << endl;

    lref = 100;  // Modifies original
    cout << "value: " << value << endl;

    // ❌ Cannot bind lvalue reference to rvalue
    // int& ref2 = 42;  // ERROR!

    return 0;
}
```

### 8.2 const Lvalue Reference to Rvalue

```cpp
#include <iostream>
using namespace std;

int main() {
    // ✅ const lvalue reference CAN bind to rvalue
    const int& ref1 = 42;
    const int& ref2 = 10 + 20;
    const int& ref3 = getValue();

    cout << "ref1: " << ref1 << endl;
    cout << "ref2: " << ref2 << endl;

    // WHY: Temporary's lifetime extended
    const string& ref4 = string("Hello") + " World";
    cout << "ref4: " << ref4 << endl;

    return 0;
}
```

---

## 9. Rvalue References (C++11)

### 9.1 Rvalue Reference Syntax

**Syntax:** `type&&` denotes rvalue reference

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: Rvalue reference binds to rvalue
    int&& rref = 42;
    int&& rref2 = 10 + 20;

    cout << "rref: " << rref << endl;
    cout << "rref2: " << rref2 << endl;

    // WHY: Can modify rvalue through reference
    rref = 100;
    cout << "rref: " << rref << endl;

    // ❌ Cannot bind rvalue reference to lvalue
    int x = 5;
    // int&& rref3 = x;  // ERROR!

    return 0;
}
```

### 9.2 Purpose of Rvalue References

**Main Uses:**

1. **Move semantics** - Transfer ownership of resources
2. **Perfect forwarding** - Preserve value categories in templates
3. **Temporary object optimization**

```cpp
#include <iostream>
#include <vector>
using namespace std;

class BigObject {
    vector<int> data;

public:
    // Copy constructor (expensive)
    BigObject(const BigObject& other) : data(other.data) {
        cout << "Copy constructor called" << endl;
    }

    // Move constructor (cheap)
    BigObject(BigObject&& other) noexcept : data(move(other.data)) {
        cout << "Move constructor called" << endl;
    }

    BigObject(int size) : data(size, 0) {}
};

BigObject createObject() {
    return BigObject(1000);  // Returns temporary
}

int main() {
    BigObject obj1 = createObject();  // Move (not copy)

    return 0;
}
```

### 9.3 std::move

**Purpose:** Cast lvalue to rvalue reference

```cpp
#include <iostream>
#include <utility>  // for std::move
#include <vector>
using namespace std;

int main() {
    vector<int> source = {1, 2, 3, 4, 5};

    cout << "source size before move: " << source.size() << endl;

    // WHY: std::move casts source to rvalue reference
    vector<int> dest = move(source);

    cout << "source size after move: " << source.size() << endl;
    cout << "dest size: " << dest.size() << endl;

    // ⚠️ source is now in valid but unspecified state

    return 0;
}
```

**Output:**

```
source size before move: 5
source size after move: 0
dest size: 5
```

### 9.4 Named Rvalue Reference is Lvalue

**Critical Concept:** Rvalue reference variable is itself an lvalue!

```cpp
#include <iostream>
using namespace std;

void process(int& x) {
    cout << "Lvalue version" << endl;
}

void process(int&& x) {
    cout << "Rvalue version" << endl;
}

int main() {
    int&& rref = 42;

    // WHY: rref is lvalue (has name)
    process(rref);  // Calls lvalue version!

    // WHY: Cast back to rvalue with std::move
    process(move(rref));  // Calls rvalue version

    return 0;
}
```

---

## 10. Reference Collapsing and Forwarding References

### 10.1 Reference Collapsing Rules

**When combining references in templates:**

```
T&  &   → T&   (lvalue ref + lvalue ref = lvalue ref)
T&  &&  → T&   (lvalue ref + rvalue ref = lvalue ref)
T&& &   → T&   (rvalue ref + lvalue ref = lvalue ref)
T&& &&  → T&&  (rvalue ref + rvalue ref = rvalue ref)
```

**Rule:** Only `&& &&` becomes `&&`, all others become `&`

### 10.2 Forwarding References (Universal References)

**Context:** `T&&` in template is NOT rvalue reference - it's forwarding reference!

```cpp
#include <iostream>
#include <utility>
using namespace std;

template<typename T>
void wrapper(T&& arg) {  // Forwarding reference (not rvalue ref)
    // T deduced based on argument type
}

int main() {
    int x = 10;

    wrapper(x);    // T = int&,  T&& = int& && → int&  (lvalue)
    wrapper(42);   // T = int,   T&& = int&&        (rvalue)

    return 0;
}
```

### 10.3 Perfect Forwarding with std::forward

```cpp
#include <iostream>
#include <utility>
using namespace std;

void process(int& x) {
    cout << "Lvalue: " << x << endl;
}

void process(int&& x) {
    cout << "Rvalue: " << x << endl;
}

// WHY: Preserve value category when forwarding
template<typename T>
void perfectWrapper(T&& arg) {
    process(forward<T>(arg));  // Forwards with correct category
}

int main() {
    int x = 10;

    perfectWrapper(x);   // Calls lvalue version
    perfectWrapper(42);  // Calls rvalue version

    return 0;
}
```

---

## 11. Common Pitfalls and Best Practices

### 11.1 Common Mistakes

**Mistake 1: Returning reference to local**

```cpp
// ❌ WRONG
int& getBad() {
    int local = 10;
    return local;  // Dangling reference!
}
```

**Mistake 2: Thinking reference can be reseated**

```cpp
int a = 10, b = 20;
int& ref = a;
ref = b;  // Copies value, doesn't rebind!
```

**Mistake 3: Forgetting const for pass by reference**

```cpp
// ❌ WRONG: Can accidentally modify
void display(string& str) {
    str = "Modified!";  // Oops!
}

// ✅ CORRECT
void display(const string& str) {
    // str = "Modified!";  // ERROR: safe
}
```

### 11.2 Best Practices

**✅ Use const reference for large read-only parameters**

```cpp
void process(const vector<int>& data) {
    // Efficient + safe
}
```

**✅ Return reference only for class members or globals**

```cpp
class Data {
    int value;
public:
    int& getValue() { return value; }  // OK
};
```

**✅ Use std::move when transferring ownership**

```cpp
vector<int> createData() {
    vector<int> temp = {1, 2, 3};
    return temp;  // Compiler optimizes (RVO)
}

void useData() {
    vector<int> source = {1, 2, 3};
    vector<int> dest = move(source);  // Explicit move
}
```

---

## Summary

### Key Takeaways

1. **References are aliases** - Alternative names for existing variables, not separate objects
2. **Must be initialized** - Cannot exist without binding to a variable
3. **Cannot be null** - Always refer to valid object (safer than pointers)
4. **Cannot be reseated** - Permanent binding; assignment copies value
5. **const references bind to rvalues** - Can accept temporaries, extending their lifetime
6. **Lvalue = named, has address** - Variables, array elements, dereferences
7. **Rvalue = temporary, no address** - Literals, expressions, function returns (non-reference)
8. **Rvalue references (&&)** - Enable move semantics and perfect forwarding (C++11)
9. **Named rvalue reference is lvalue** - Variable with type `T&&` is still lvalue
10. **Forwarding references** - `T&&` in templates is universal reference, not rvalue reference

### Interview Essential Points

**Q: What is a reference and how does it differ from a pointer?**

A: A reference is an alias for an existing variable providing direct access without dereference syntax. Key differences: (1) References must be initialized and cannot be null, pointers can be nullptr. (2) References cannot be reseated to different objects, pointers can point elsewhere. (3) References use direct syntax (ref), pointers need dereferencing (*ptr). (4) References have no separate memory address, pointers do. (5) References are safer (guaranteed valid), pointers can dangle. Use references for simple aliasing and non-null parameters; use pointers when you need nullable, reassignable, or arithmetic operations.

**Q: Why can const references bind to rvalues but non-const references cannot?**

A: Non-const lvalue references cannot bind to rvalues because it would allow modifying temporaries that are about to be destroyed, which is meaningless. `int& ref = 42;` is rejected because 42 is temporary. However, const lvalue references can bind to rvalues because they promise not to modify the temporary, making it safe: `const int& ref = 42;`. This extends the temporary's lifetime to match the reference's scope. This feature enables passing temporaries to functions efficiently: `func(string("temp"))` works if parameter is `const string&`.

**Q: Explain lvalue vs rvalue with examples.**

A: Lvalue (locator value) refers to object with specific memory location and name that persists beyond expression. Can take its address and assign to it. Examples: variables (`int x`), array elements (`arr[0]`), dereferenced pointers (`*ptr`), references. Rvalue (right value) is temporary value without persistent memory location. Cannot take address or assign to. Examples: literals (`42`), expressions (`x + y`), function returns by value (`getValue()`), temporaries (`string("temp")`). Key test: if you can write `&expr`, it's lvalue; otherwise rvalue. In `x = 10`, x is lvalue (has address), 10 is rvalue (literal).

**Q: What are rvalue references and why were they added in C++11?**

A: Rvalue references (`T&&`) were added in C++11 to enable move semantics and perfect forwarding. They bind to rvalues (temporaries), allowing functions to distinguish between resources that must be copied (lvalues) vs resources that can be "stolen" (rvalues about to be destroyed). This enables: (1) Move semantics - transfer ownership of resources without copying (move constructor takes `T&&`). (2) Perfect forwarding - preserve lvalue/rvalue nature when forwarding arguments through templates. Example: `vector(vector&& other)` move constructor transfers internal buffer from temporary vector instead of copying millions of elements.

**Q: What is the difference between std::move and std::forward?**

A: `std::move` unconditionally casts its argument to rvalue reference, indicating "I'm done with this object, you can steal its resources." Used when you explicitly want to move from an lvalue: `vector<int> dest = move(source);`. `std::forward<T>` conditionally casts based on template parameter, preserving the original value category. Used in perfect forwarding to maintain whether argument was lvalue or rvalue: `wrapper(T&& arg) { func(forward<T>(arg)); }`. Move always produces rvalue; forward produces lvalue if given lvalue, rvalue if given rvalue. Don't use forward outside templates or move inside perfect forwarding functions.

**Q: What are forwarding references (universal references)?**

A: Forwarding references are `T&&` in template contexts where T is deduced template parameter. Despite the `&&` syntax, they can bind to both lvalues and rvalues: `template<typename T> void f(T&& arg)`. When lvalue passed, T deduces to `int&`, and `int& &&` collapses to `int&`. When rvalue passed, T deduces to `int`, giving `int&&`. This enables perfect forwarding with `std::forward<T>`. Key distinction: `void f(Widget&& x)` is rvalue reference (Widget is concrete type), `template<typename T> void f(T&& x)` is forwarding reference (T is deduced). Only works with type deduction contexts.

**Q: Can you return a reference from a function? When is it safe?**

A: Yes, but only when the referenced object outlives the function call. Safe cases: (1) Returning reference to global/static variable, (2) Returning reference to class member (object must outlive call), (3) Returning function parameter that's a reference. Dangerous: returning reference to local variable causes dangling reference - locals are destroyed when function returns. Example safe: `int& getGlobal() { static int x = 0; return x; }`. Example unsafe: `int& getBad() { int x = 0; return x; }` - returns reference to destroyed variable. Operator overloading often returns references for chaining: `operator=` returns `*this` by reference.

**Q: Why is a named rvalue reference an lvalue?**

A: This is a safety feature preventing accidental double-moves. When you write `int&& rref = 42;`, rref is a variable with a name, making it an lvalue even though its type is rvalue reference. If rref were an rvalue, it could be moved from automatically, potentially multiple times. By making it an lvalue, you must explicitly cast with `std::move(rref)` to move from it, preventing accidental resource theft. General rule: if it has a name, it's an lvalue, regardless of its type. This applies to function parameters too: `void f(T&& x)` - x is lvalue inside f even though parameter type is rvalue reference.

---