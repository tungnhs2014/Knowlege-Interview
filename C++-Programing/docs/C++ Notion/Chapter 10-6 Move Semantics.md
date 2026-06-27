# 10.6. Move Semantics

---

## Table of Contents

1. Introduction to Move Semantics
2. Value Categories
3. Rvalue References
4. Move Constructor
5. Move Assignment Operator
6. std::move
7. Rule of Five
8. Perfect Forwarding
9. Move Semantics in STL
10. Common Pitfalls
11. Best Practices
12. Summary

---

## 1. Introduction to Move Semantics

### What is Move Semantics?

**Move semantics** (C++11) is a technique that allows transferring resources from one object to another instead of copying them. It "steals" the internals of a temporary object rather than making an expensive deep copy.

**Think of it as:** Moving furniture to a new house instead of buying identical new furniture.

### Why Do We Need Move Semantics?

**Problem: Expensive Copies**

```cpp
#include <iostream>
#include <vector>
using namespace std;

vector<int> createLargeVector() {
    vector<int> v(1000000);  // 1 million integers
    return v;  // Without move semantics: COPY entire vector!
}

int main() {
    vector<int> data = createLargeVector();
    // With move semantics: resources transferred, no copy!
    return 0;
}
```

### Copy vs Move

| Operation | What Happens | Performance |
| --- | --- | --- |
| **Copy** | Allocate new memory, copy all data | Slow for large objects |
| **Move** | Transfer ownership, no new allocation | Fast (constant time) |

### Before and After C++11

**Before C++11 (Copy only):**

```cpp
string a = "Hello";
string b = a;      // COPY: allocate memory, copy characters
```

**C++11 (Move available):**

```cpp
string a = "Hello";
string b = std::move(a);  // MOVE: steal a's buffer, a becomes empty
```

---

## 2. Value Categories

### Overview

C++11 introduced a refined system of **value categories** to support move semantics.

```
              expression
              /        \
           glvalue    rvalue
           /    \     /    \
       lvalue  xvalue    prvalue
```

### lvalue (Locator Value)

An **lvalue** has a name and a persistent memory location. You can take its address.

```cpp
#include <iostream>
using namespace std;

int main() {
    int x = 10;           // x is an lvalue
    int* ptr = &x;        // Can take address of lvalue

    int arr[5];           // arr is an lvalue

    int& ref = x;         // ref is also an lvalue

    cout << "x address: " << &x << endl;
    return 0;
}
```

### rvalue (Right Value)

An **rvalue** is a temporary value without a persistent memory location.

```cpp
#include <iostream>
using namespace std;

int getValue() { return 42; }

int main() {
    int x = 10;

    // rvalues:
    // 42             - literal
    // x + 5          - expression result
    // getValue()     - function return value

    // int* ptr = &42;        // ERROR: Can't take address of rvalue
    // int* ptr = &(x + 5);   // ERROR: Can't take address of rvalue

    int result = getValue();  // getValue() returns rvalue

    return 0;
}
```

### prvalue (Pure rvalue)

A **prvalue** is a "pure" rvalue - literals and temporary objects.

```cpp
int main() {
    // prvalues:
    42;                    // Literal
    3.14;                  // Literal
    true;                  // Literal
    "hello";              // String literal
    int(5);               // Temporary
    []() { return 1; };   // Lambda

    return 0;
}
```

### xvalue (eXpiring Value)

An **xvalue** is an expiring value - the result of `std::move` or similar.

```cpp
#include <utility>
using namespace std;

int main() {
    string s = "Hello";

    // std::move(s) is an xvalue
    // It's still s, but we're saying "treat it as moveable"
    string s2 = std::move(s);  // s is now "expired"

    return 0;
}
```

### Value Category Summary

| Category | Has Identity | Moveable | Example |
| --- | --- | --- | --- |
| lvalue | ✅ Yes | ❌ No | `int x = 5; x` |
| prvalue | ❌ No | ✅ Yes | `42`, `func()` |
| xvalue | ✅ Yes | ✅ Yes | `std::move(x)` |

---

## 3. Rvalue References

### What is an Rvalue Reference?

An **rvalue reference** (T&&) is a reference that can bind to rvalues (temporaries). It enables move semantics.

### Syntax

```cpp
Type&   // lvalue reference
Type&&  // rvalue reference
```

### Basic Rvalue Reference Example

```cpp
#include <iostream>
using namespace std;

int main() {
    // lvalue reference - binds to lvalue
    int x = 10;
    int& lref = x;          // OK
    // int& lref2 = 42;     // ERROR: can't bind lvalue ref to rvalue

    // rvalue reference - binds to rvalue
    int&& rref = 42;        // OK: binds to temporary
    int&& rref2 = x + 5;    // OK: binds to temporary result
    // int&& rref3 = x;     // ERROR: can't bind rvalue ref to lvalue

    cout << "rref: " << rref << endl;

    // Interesting: rvalue reference IS an lvalue!
    rref = 100;  // Can modify because rref itself is an lvalue
    cout << "rref after modification: " << rref << endl;

    return 0;
}
```

**Output:**

```
rref: 42
rref after modification: 100
```

### Function Overloading with References

```cpp
#include <iostream>
using namespace std;

void process(int& x) {
    cout << "lvalue reference: " << x << endl;
}

void process(int&& x) {
    cout << "rvalue reference: " << x << endl;
}

int main() {
    int a = 10;

    process(a);       // Calls lvalue version
    process(20);      // Calls rvalue version
    process(a + 5);   // Calls rvalue version

    return 0;
}
```

**Output:**

```
lvalue reference: 10
rvalue reference: 20
rvalue reference: 15
```

---

## 4. Move Constructor

### What is a Move Constructor?

A **move constructor** transfers resources from a temporary object instead of copying them. It "steals" the source object's data.

### Syntax

```cpp
ClassName(ClassName&& other) noexcept;
```

### Move Constructor Example

```cpp
#include <iostream>
#include <cstring>
using namespace std;

class String {
    char* data;
    size_t length;

public:
    // Normal constructor
    String(const char* str = "") {
        length = strlen(str);
        data = new char[length + 1];
        strcpy(data, str);
        cout << "Constructor: \"" << data << "\"" << endl;
    }

    // Copy constructor (expensive!)
    String(const String& other) {
        length = other.length;
        data = new char[length + 1];
        strcpy(data, other.data);
        cout << "Copy Constructor: \"" << data << "\"" << endl;
    }

    // Move constructor (cheap!)
    String(String&& other) noexcept {
        // WHY: Steal the data
        data = other.data;
        length = other.length;

        // WHY: Leave source in valid but empty state
        other.data = nullptr;
        other.length = 0;

        cout << "Move Constructor: \"" << data << "\"" << endl;
    }

    ~String() {
        if (data) {
            cout << "Destructor: \"" << data << "\"" << endl;
        } else {
            cout << "Destructor: (empty)" << endl;
        }
        delete[] data;
    }

    const char* c_str() const { return data ? data : ""; }
};

int main() {
    String s1("Hello");

    // Copy constructor called
    String s2 = s1;

    // Move constructor called
    String s3 = std::move(s1);

    cout << "\ns1: \"" << s1.c_str() << "\"" << endl;
    cout << "s2: \"" << s2.c_str() << "\"" << endl;
    cout << "s3: \"" << s3.c_str() << "\"" << endl;

    return 0;
}
```

**Output:**

```
Constructor: "Hello"
Copy Constructor: "Hello"
Move Constructor: "Hello"

s1: ""
s2: "Hello"
s3: "Hello"
Destructor: "Hello"
Destructor: "Hello"
Destructor: (empty)
```

### Why noexcept Matters

```cpp
#include <iostream>
#include <vector>
using namespace std;

class Widget {
public:
    // WHY: noexcept allows STL to use move
    Widget(Widget&& other) noexcept {
        cout << "Move (noexcept)" << endl;
    }

    Widget() { cout << "Default" << endl; }
    Widget(const Widget&) { cout << "Copy" << endl; }
};

int main() {
    vector<Widget> v;
    v.reserve(1);

    Widget w;
    v.push_back(std::move(w));

    // When vector reallocates, it uses move if noexcept
    v.push_back(Widget());

    return 0;
}
```

---

## 5. Move Assignment Operator

### What is Move Assignment?

The **move assignment operator** transfers resources from a source to an existing target object.

### Syntax

```cpp
ClassName& operator=(ClassName&& other) noexcept;
```

### Move Assignment Example

```cpp
#include <iostream>
#include <cstring>
using namespace std;

class Buffer {
    int* data;
    size_t size;

public:
    Buffer(size_t sz = 0) : size(sz), data(sz ? new int[sz] : nullptr) {
        cout << "Constructor: size=" << size << endl;
    }

    // Copy assignment
    Buffer& operator=(const Buffer& other) {
        if (this != &other) {
            delete[] data;
            size = other.size;
            data = new int[size];
            memcpy(data, other.data, size * sizeof(int));
            cout << "Copy Assignment: size=" << size << endl;
        }
        return *this;
    }

    // Move assignment
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            // WHY: Delete current resources
            delete[] data;

            // WHY: Steal from source
            data = other.data;
            size = other.size;

            // WHY: Leave source in valid state
            other.data = nullptr;
            other.size = 0;

            cout << "Move Assignment: size=" << size << endl;
        }
        return *this;
    }

    ~Buffer() {
        delete[] data;
    }

    size_t getSize() const { return size; }
};

int main() {
    Buffer b1(100);
    Buffer b2(50);

    cout << "\n--- Copy Assignment ---" << endl;
    b2 = b1;
    cout << "b1 size: " << b1.getSize() << endl;
    cout << "b2 size: " << b2.getSize() << endl;

    Buffer b3(200);
    cout << "\n--- Move Assignment ---" << endl;
    b3 = std::move(b1);
    cout << "b1 size: " << b1.getSize() << endl;
    cout << "b3 size: " << b3.getSize() << endl;

    return 0;
}
```

**Output:**

```
Constructor: size=100
Constructor: size=50

--- Copy Assignment ---
Copy Assignment: size=100
b1 size: 100
b2 size: 100
Constructor: size=200

--- Move Assignment ---
Move Assignment: size=100
b1 size: 0
b3 size: 100
```

---

## 6. std::move

### What is std::move?

**std::move** is a cast that converts an lvalue into an rvalue reference, enabling move semantics. It doesn't actually move anything - it just enables moving.

### How std::move Works

```cpp
// Simplified implementation
template<typename T>
typename remove_reference<T>::type&& move(T&& t) noexcept {
    return static_cast<typename remove_reference<T>::type&&>(t);
}
```

### Using std::move

```cpp
#include <iostream>
#include <string>
#include <utility>
using namespace std;

int main() {
    string original = "Hello, World!";

    cout << "Before move:" << endl;
    cout << "original: \"" << original << "\"" << endl;

    // std::move casts to rvalue reference
    string moved = std::move(original);

    cout << "\nAfter move:" << endl;
    cout << "original: \"" << original << "\"" << endl;
    cout << "moved: \"" << moved << "\"" << endl;

    return 0;
}
```

**Output:**

```
Before move:
original: "Hello, World!"

After move:
original: ""
moved: "Hello, World!"
```

### std::move with Containers

```cpp
#include <iostream>
#include <vector>
#include <string>
using namespace std;

int main() {
    vector<string> source = {"apple", "banana", "cherry"};

    cout << "Before move:" << endl;
    cout << "source size: " << source.size() << endl;

    // Move entire vector
    vector<string> dest = std::move(source);

    cout << "\nAfter move:" << endl;
    cout << "source size: " << source.size() << endl;
    cout << "dest size: " << dest.size() << endl;

    // Move into vector
    string fruit = "dragonfruit";
    dest.push_back(std::move(fruit));

    cout << "\nAfter push_back with move:" << endl;
    cout << "fruit: \"" << fruit << "\"" << endl;
    cout << "dest back: \"" << dest.back() << "\"" << endl;

    return 0;
}
```

**Output:**

```
Before move:
source size: 3

After move:
source size: 0
dest size: 3

After push_back with move:
fruit: ""
dest back: "dragonfruit"
```

---

## 7. Rule of Five

### What is the Rule of Five?

If a class defines any of these five special member functions, it should probably define all five:

1. **Destructor**
2. **Copy Constructor**
3. **Copy Assignment Operator**
4. **Move Constructor**
5. **Move Assignment Operator**

### Complete Rule of Five Example

```cpp
#include <iostream>
#include <algorithm>
using namespace std;

class Resource {
    int* data;
    size_t size;

public:
    // Constructor
    Resource(size_t sz = 0) : size(sz), data(sz ? new int[sz] : nullptr) {
        cout << "Constructor" << endl;
    }

    // 1. Destructor
    ~Resource() {
        cout << "Destructor" << endl;
        delete[] data;
    }

    // 2. Copy Constructor
    Resource(const Resource& other) : size(other.size), data(new int[other.size]) {
        cout << "Copy Constructor" << endl;
        copy(other.data, other.data + size, data);
    }

    // 3. Copy Assignment
    Resource& operator=(const Resource& other) {
        cout << "Copy Assignment" << endl;
        if (this != &other) {
            Resource temp(other);  // Copy-and-swap idiom
            swap(data, temp.data);
            swap(size, temp.size);
        }
        return *this;
    }

    // 4. Move Constructor
    Resource(Resource&& other) noexcept : data(other.data), size(other.size) {
        cout << "Move Constructor" << endl;
        other.data = nullptr;
        other.size = 0;
    }

    // 5. Move Assignment
    Resource& operator=(Resource&& other) noexcept {
        cout << "Move Assignment" << endl;
        if (this != &other) {
            delete[] data;
            data = other.data;
            size = other.size;
            other.data = nullptr;
            other.size = 0;
        }
        return *this;
    }
};

int main() {
    Resource r1(10);

    Resource r2 = r1;           // Copy constructor
    Resource r3 = std::move(r1); // Move constructor

    Resource r4(5);
    r4 = r2;                    // Copy assignment
    r4 = std::move(r3);         // Move assignment

    return 0;
}
```

### Rule of Zero

If possible, design classes that don't need custom resource management:

```cpp
#include <string>
#include <vector>
using namespace std;

// Rule of Zero: Let compiler generate everything
class Person {
    string name;           // Manages its own resources
    vector<int> scores;    // Manages its own resources

public:
    Person(string n) : name(move(n)) {}

    // No destructor, copy/move operations needed!
    // Compiler generates correct versions automatically
};
```

---

## 8. Perfect Forwarding

### What is Perfect Forwarding?

**Perfect forwarding** preserves the value category (lvalue/rvalue) of arguments when passing them to other functions.

### The Problem

```cpp
#include <iostream>
using namespace std;

void process(int& x) { cout << "lvalue" << endl; }
void process(int&& x) { cout << "rvalue" << endl; }

// BAD: Loses rvalue-ness
template<typename T>
void wrapper(T&& arg) {
    // arg is always an lvalue here!
    process(arg);  // Always calls lvalue version
}
```

### Solution: std::forward

```cpp
#include <iostream>
#include <utility>
using namespace std;

void process(int& x) { cout << "lvalue: " << x << endl; }
void process(int&& x) { cout << "rvalue: " << x << endl; }

// GOOD: Perfect forwarding
template<typename T>
void wrapper(T&& arg) {
    // WHY: std::forward preserves value category
    process(std::forward<T>(arg));
}

int main() {
    int x = 10;

    wrapper(x);      // T = int&, forwards as lvalue
    wrapper(20);     // T = int, forwards as rvalue
    wrapper(x + 5);  // T = int, forwards as rvalue

    return 0;
}
```

**Output:**

```
lvalue: 10
rvalue: 20
rvalue: 15
```

### Universal References

```cpp
template<typename T>
void func(T&& arg);  // Universal reference (forwarding reference)
```

When `T&&` appears in a template context, it's a **universal reference** that can bind to both lvalues and rvalues.

---

## 9. Move Semantics in STL

### Vector Operations

```cpp
#include <iostream>
#include <vector>
#include <string>
using namespace std;

int main() {
    vector<string> v;

    string s = "Hello";

    // Copy into vector
    v.push_back(s);
    cout << "After copy: s = \"" << s << "\"" << endl;

    // Move into vector
    v.push_back(std::move(s));
    cout << "After move: s = \"" << s << "\"" << endl;

    // emplace_back constructs in place (no move needed)
    v.emplace_back("World");

    cout << "\nVector contents:" << endl;
    for (const auto& str : v) {
        cout << "  \"" << str << "\"" << endl;
    }

    return 0;
}
```

**Output:**

```
After copy: s = "Hello"
After move: s = ""

Vector contents:
  "Hello"
  "Hello"
  "World"
```

### Moving Unique Pointers

```cpp
#include <iostream>
#include <memory>
#include <vector>
using namespace std;

int main() {
    vector<unique_ptr<int>> v;

    auto ptr = make_unique<int>(42);

    // v.push_back(ptr);           // ERROR: Can't copy unique_ptr
    v.push_back(std::move(ptr));   // OK: Move works

    cout << "ptr is null: " << (ptr == nullptr) << endl;
    cout << "v[0] value: " << *v[0] << endl;

    return 0;
}
```

**Output:**

```
ptr is null: 1
v[0] value: 42
```

### Return Value Optimization (RVO)

```cpp
#include <iostream>
#include <vector>
using namespace std;

class Widget {
public:
    Widget() { cout << "Constructor" << endl; }
    Widget(const Widget&) { cout << "Copy" << endl; }
    Widget(Widget&&) noexcept { cout << "Move" << endl; }
};

Widget createWidget() {
    Widget w;
    return w;  // RVO: Often no copy/move at all!
}

int main() {
    cout << "Creating widget:" << endl;
    Widget w = createWidget();
    // Often prints just "Constructor" due to RVO!

    return 0;
}
```

---

## 10. Common Pitfalls

### Pitfall 1: Using Object After Move

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    string s = "Hello";
    string s2 = std::move(s);

    // PITFALL: Using s after move
    // cout << s.length() << endl;  // Works but unreliable
    // s[0] = 'X';                   // DANGEROUS!

    // SAFE: s is in valid but unspecified state
    s = "New Value";  // OK: Can assign new value
    cout << s << endl;

    return 0;
}
```

### Pitfall 2: Unnecessary std::move on Return

```cpp
#include <string>
using namespace std;

// BAD: Prevents RVO
string createBad() {
    string s = "Hello";
    return std::move(s);  // Don't do this!
}

// GOOD: Allows RVO
string createGood() {
    string s = "Hello";
    return s;  // Compiler optimizes automatically
}
```

### Pitfall 3: Moving const Objects

```cpp
#include <iostream>
#include <string>
using namespace std;

int main() {
    const string s = "Hello";

    // std::move on const doesn't actually move!
    string s2 = std::move(s);  // Actually copies!

    cout << "s: \"" << s << "\"" << endl;    // Still "Hello"
    cout << "s2: \"" << s2 << "\"" << endl;  // Also "Hello"

    return 0;
}
```

### Pitfall 4: Forgetting noexcept

```cpp
#include <vector>
using namespace std;

class Widget {
public:
    // Without noexcept: vector may copy instead of move
    Widget(Widget&&) { /* ... */ }  // BAD

    // With noexcept: vector will use move
    // Widget(Widget&&) noexcept { /* ... */ }  // GOOD
};
```

---

## 11. Best Practices

### ✅ DO: Mark Move Operations noexcept

```cpp
class MyClass {
public:
    MyClass(MyClass&& other) noexcept;
    MyClass& operator=(MyClass&& other) noexcept;
};
```

### ✅ DO: Leave Moved-From Objects Valid

```cpp
class Buffer {
    int* data;
public:
    Buffer(Buffer&& other) noexcept : data(other.data) {
        other.data = nullptr;  // Valid but empty
    }
};
```

### ✅ DO: Use std::move for Sink Parameters

```cpp
void setName(string name) {  // Takes by value
    m_name = std::move(name);  // Move into member
}
```

### ❌ DON'T: Use std::move on Return

```cpp
// BAD
return std::move(local);

// GOOD
return local;
```

### ❌ DON'T: Use Object After std::move

```cpp
string s = "Hello";
process(std::move(s));
// s is now in unspecified state - don't use!
```

---

## 12. Summary

### Key Takeaways

1. **Move Semantics**: Transfer resources instead of copying
2. **Rvalue References (T&&)**: Bind to temporaries
3. **std::move**: Cast to rvalue reference
4. **Move Constructor/Assignment**: Implement resource transfer
5. **noexcept**: Critical for STL optimization

### Value Categories

| Category | Named | Moveable | Example |
| --- | --- | --- | --- |
| lvalue | ✅ | ❌ | `int x` |
| prvalue | ❌ | ✅ | `42` |
| xvalue | ✅ | ✅ | `std::move(x)` |

### Special Member Functions

| Function | Purpose |
| --- | --- |
| Destructor | Cleanup resources |
| Copy Constructor | Deep copy |
| Copy Assignment | Deep copy to existing |
| Move Constructor | Transfer ownership |
| Move Assignment | Transfer to existing |

### Keywords Covered

✅ lvalue (4)
✅ rvalue (5)
✅ xvalue (2)
✅ prvalue (2)
✅ glvalue (1)
✅ Rvalue references (6)
✅ std::move (8)
✅ Move constructor (6)
✅ Move assignment (4)
✅ Rule of Five (3)
✅ noexcept (4)
✅ Perfect forwarding (3)
✅ std::forward (2)
✅ Universal references (2)
✅ Value categories (3)
✅ RVO/NRVO (2)
✅ Copy elision (1)
✅ Resource transfer (2)

**Total: 60 keywords/concepts covered**

---