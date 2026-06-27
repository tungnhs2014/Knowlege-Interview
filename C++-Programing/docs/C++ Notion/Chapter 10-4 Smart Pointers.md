# 10.4. Smart Pointers

---

## Table of Contents

1. Introduction to Smart Pointers
2. Problems with Raw Pointers
3. RAII Principle
4. std::unique_ptr
5. std::shared_ptr
6. std::weak_ptr
7. make_unique and make_shared
8. Custom Deleters
9. Smart Pointer Comparison
10. Common Pitfalls
11. Best Practices
12. Summary

---

## 1. Introduction to Smart Pointers

### What are Smart Pointers?

**Smart pointers** are class templates that behave like pointers but provide automatic memory management. They ensure that dynamically allocated memory is properly released when no longer needed.

**Think of them as:** Pointers with built-in "self-destruct" capability - they automatically clean up after themselves.

### Why Do We Need Smart Pointers?

1. **Prevent Memory Leaks**: Automatic deallocation
2. **Avoid Dangling Pointers**: Clear ownership semantics
3. **Exception Safety**: Resources released even if exceptions occur
4. **RAII Compliance**: Resource management tied to object lifetime
5. **Simpler Code**: No need for explicit `delete`

### The `<memory>` Header

All smart pointers are defined in the `<memory>` header:

```cpp
#include <memory>

// Three types of smart pointers:
std::unique_ptr<T>  // Exclusive ownership
std::shared_ptr<T>  // Shared ownership
std::weak_ptr<T>    // Non-owning reference
```

### History

- **C++98**: `auto_ptr` (deprecated, removed in C++17)
- **C++11**: `unique_ptr`, `shared_ptr`, `weak_ptr` introduced
- **C++14**: `make_unique` added

---

## 2. Problems with Raw Pointers

### Problem 1: Memory Leaks

```cpp
#include <iostream>
using namespace std;

void memoryLeakExample() {
    // WHY BAD: Memory allocated but never freed
    int* ptr = new int(42);

    // Some code that might throw exception or return early
    if (true) {
        return;  // Memory leak! ptr never deleted
    }

    delete ptr;  // Never reached
}

int main() {
    // WHY: This loop creates infinite memory leak
    while (false) {  // Disabled for safety
        int* leak = new int;
        // Forgot to delete!
    }

    cout << "Memory leaks are silent killers!" << endl;
    return 0;
}
```

### Problem 2: Dangling Pointers

```cpp
#include <iostream>
using namespace std;

int main() {
    int* ptr = new int(100);
    int* ptr2 = ptr;  // Both point to same memory

    delete ptr;  // Memory freed

    // WHY BAD: ptr2 is now dangling!
    // cout << *ptr2 << endl;  // Undefined behavior!

    cout << "ptr2 is now a dangling pointer!" << endl;
    return 0;
}
```

### Problem 3: Double Delete

```cpp
#include <iostream>
using namespace std;

int main() {
    int* ptr = new int(50);
    int* ptr2 = ptr;

    delete ptr;
    // delete ptr2;  // CRASH! Double delete - undefined behavior

    cout << "Double delete causes crashes!" << endl;
    return 0;
}
```

### Problem 4: Exception Safety

```cpp
#include <iostream>
#include <stdexcept>
using namespace std;

void riskyFunction() {
    int* ptr = new int(42);

    // WHY BAD: If exception thrown, memory leaks
    throw runtime_error("Something went wrong!");

    delete ptr;  // Never reached!
}

int main() {
    try {
        riskyFunction();
    } catch (const exception& e) {
        cout << "Exception: " << e.what() << endl;
        cout << "Memory was leaked!" << endl;
    }
    return 0;
}
```

---

## 3. RAII Principle

### What is RAII?

**RAII (Resource Acquisition Is Initialization)** is a C++ programming idiom where resource lifetime is tied to object lifetime.

**Key Concept:** Acquire resources in constructor, release in destructor.

### How RAII Works

```cpp
#include <iostream>
using namespace std;

// WHY: RAII wrapper for integer pointer
class IntWrapper {
    int* ptr;
public:
    // Constructor acquires resource
    IntWrapper(int value) : ptr(new int(value)) {
        cout << "Resource acquired: " << *ptr << endl;
    }

    // Destructor releases resource
    ~IntWrapper() {
        cout << "Resource released: " << *ptr << endl;
        delete ptr;
    }

    int getValue() const { return *ptr; }
    void setValue(int v) { *ptr = v; }
};

int main() {
    {
        IntWrapper wrapper(42);
        cout << "Value: " << wrapper.getValue() << endl;
        // WHY: Destructor called automatically when wrapper goes out of scope
    }

    cout << "After scope - resource already cleaned up!" << endl;
    return 0;
}
```

**Output:**

```
Resource acquired: 42
Value: 42
Resource released: 42
After scope - resource already cleaned up!
```

**Explanation:**

- Smart pointers implement RAII
- When smart pointer goes out of scope, destructor automatically deletes managed object
- No need for explicit `delete`

---

## 4. std::unique_ptr

### What is unique_ptr?

**std::unique_ptr** provides **exclusive ownership** of a dynamically allocated object. Only one `unique_ptr` can own the object at a time.

### Key Characteristics

1. **Exclusive Ownership**: Cannot be copied
2. **Moveable**: Ownership can be transferred with `std::move`
3. **Lightweight**: Same size as raw pointer
4. **Automatic Deletion**: Object deleted when unique_ptr destroyed

### Creating unique_ptr

```cpp
#include <iostream>
#include <memory>
using namespace std;

class Resource {
    int id;
public:
    Resource(int i) : id(i) {
        cout << "Resource " << id << " created" << endl;
    }
    ~Resource() {
        cout << "Resource " << id << " destroyed" << endl;
    }
    void use() { cout << "Using resource " << id << endl; }
};

int main() {
    // WHY: Multiple ways to create unique_ptr

    // Method 1: Constructor with new
    unique_ptr<Resource> ptr1(new Resource(1));

    // Method 2: make_unique (C++14, preferred)
    auto ptr2 = make_unique<Resource>(2);

    // Using the pointers
    ptr1->use();
    ptr2->use();

    // WHY: Both resources automatically destroyed when main() ends
    return 0;
}
```

**Output:**

```
Resource 1 created
Resource 2 created
Using resource 1
Using resource 2
Resource 2 destroyed
Resource 1 destroyed
```

### unique_ptr Cannot Be Copied

```cpp
#include <iostream>
#include <memory>
using namespace std;

int main() {
    unique_ptr<int> ptr1 = make_unique<int>(42);

    // WHY ERROR: unique_ptr cannot be copied!
    // unique_ptr<int> ptr2 = ptr1;  // Compile error!

    // WHY OK: Can be moved
    unique_ptr<int> ptr2 = move(ptr1);

    // ptr1 is now nullptr
    if (ptr1 == nullptr) {
        cout << "ptr1 is now empty" << endl;
    }

    cout << "ptr2 value: " << *ptr2 << endl;

    return 0;
}
```

**Output:**

```
ptr1 is now empty
ptr2 value: 42
```

### unique_ptr Member Functions

```cpp
#include <iostream>
#include <memory>
using namespace std;

int main() {
    auto ptr = make_unique<int>(100);

    // get() - returns raw pointer
    int* raw = ptr.get();
    cout << "Via get(): " << *raw << endl;

    // operator* - dereference
    cout << "Via *ptr: " << *ptr << endl;

    // release() - releases ownership, returns raw pointer
    int* released = ptr.release();
    cout << "Released: " << *released << endl;
    cout << "ptr is null? " << (ptr == nullptr ? "Yes" : "No") << endl;
    delete released;  // Must delete manually now!

    // reset() - replaces managed object
    auto ptr2 = make_unique<int>(200);
    ptr2.reset(new int(300));  // Old object deleted
    cout << "After reset: " << *ptr2 << endl;

    ptr2.reset();  // Deletes object, ptr2 becomes nullptr
    cout << "After reset(): " << (ptr2 == nullptr ? "null" : "valid") << endl;

    return 0;
}
```

**Output:**

```
Via get(): 100
Via *ptr: 100
Released: 100
ptr is null? Yes
After reset: 300
After reset(): null
```

### unique_ptr with Arrays

```cpp
#include <iostream>
#include <memory>
using namespace std;

int main() {
    // WHY: unique_ptr has array specialization
    unique_ptr<int[]> arr = make_unique<int[]>(5);

    // Use like array
    for (int i = 0; i < 5; i++) {
        arr[i] = i * 10;
    }

    cout << "Array contents: ";
    for (int i = 0; i < 5; i++) {
        cout << arr[i] << " ";
    }
    cout << endl;

    // WHY: Automatically calls delete[] not delete
    return 0;
}
```

**Output:**

```
Array contents: 0 10 20 30 40
```

### unique_ptr in Functions

```cpp
#include <iostream>
#include <memory>
using namespace std;

// WHY: Factory function returning unique_ptr
unique_ptr<string> createMessage(const string& text) {
    return make_unique<string>("Message: " + text);
}

// WHY: Taking ownership (by value, requires move)
void consumeMessage(unique_ptr<string> msg) {
    cout << "Consumed: " << *msg << endl;
    // msg destroyed when function ends
}

// WHY: Borrowing (by reference, no ownership transfer)
void borrowMessage(const unique_ptr<string>& msg) {
    cout << "Borrowed: " << *msg << endl;
    // msg not destroyed, caller keeps ownership
}

int main() {
    auto msg = createMessage("Hello World");

    borrowMessage(msg);  // Borrow, we still own it
    cout << "We still have: " << *msg << endl;

    consumeMessage(move(msg));  // Transfer ownership
    // msg is now nullptr

    return 0;
}
```

**Output:**

```
Borrowed: Message: Hello World
We still have: Message: Hello World
Consumed: Message: Hello World
```

---

## 5. std::shared_ptr

### What is shared_ptr?

**std::shared_ptr** provides **shared ownership** of a dynamically allocated object. Multiple `shared_ptr` instances can own the same object, and the object is deleted only when the last owner is destroyed.

### Key Characteristics

1. **Reference Counting**: Tracks number of owners
2. **Copyable**: Unlike unique_ptr
3. **Thread-safe Reference Count**: Atomic operations
4. **Larger than unique_ptr**: Stores pointer + control block pointer

### Creating shared_ptr

```cpp
#include <iostream>
#include <memory>
using namespace std;

class Data {
    int value;
public:
    Data(int v) : value(v) {
        cout << "Data(" << value << ") created" << endl;
    }
    ~Data() {
        cout << "Data(" << value << ") destroyed" << endl;
    }
    int get() const { return value; }
};

int main() {
    // WHY: Multiple ways to create shared_ptr

    // Method 1: Constructor
    shared_ptr<Data> sp1(new Data(1));

    // Method 2: make_shared (preferred)
    auto sp2 = make_shared<Data>(2);

    cout << "sp1 value: " << sp1->get() << endl;
    cout << "sp2 value: " << sp2->get() << endl;

    return 0;
}
```

**Output:**

```
Data(1) created
Data(2) created
sp1 value: 1
sp2 value: 2
Data(2) destroyed
Data(1) destroyed
```

### Reference Counting with use_count()

```cpp
#include <iostream>
#include <memory>
using namespace std;

int main() {
    auto sp1 = make_shared<int>(42);
    cout << "After creation - count: " << sp1.use_count() << endl;

    {
        // WHY: Copying increases reference count
        shared_ptr<int> sp2 = sp1;
        cout << "After copy - count: " << sp1.use_count() << endl;

        shared_ptr<int> sp3 = sp1;
        cout << "Another copy - count: " << sp1.use_count() << endl;

        // sp2 and sp3 go out of scope here
    }

    cout << "After scope - count: " << sp1.use_count() << endl;

    return 0;
}
```

**Output:**

```
After creation - count: 1
After copy - count: 2
Another copy - count: 3
After scope - count: 1
```

### shared_ptr Member Functions

```cpp
#include <iostream>
#include <memory>
using namespace std;

int main() {
    auto sp = make_shared<int>(100);

    // get() - returns raw pointer
    int* raw = sp.get();
    cout << "Via get(): " << *raw << endl;

    // use_count() - number of owners
    cout << "use_count: " << sp.use_count() << endl;

    // unique() - true if use_count == 1
    cout << "unique: " << (sp.unique() ? "Yes" : "No") << endl;

    // reset() - replaces or clears
    auto sp2 = sp;  // share
    cout << "After share, use_count: " << sp.use_count() << endl;

    sp.reset();  // sp no longer owns, sp2 still does
    cout << "After sp.reset(), sp2 use_count: " << sp2.use_count() << endl;

    // swap() - swaps managed objects
    auto spA = make_shared<int>(1);
    auto spB = make_shared<int>(2);
    swap(spA, spB);
    cout << "After swap: spA=" << *spA << ", spB=" << *spB << endl;

    return 0;
}
```

**Output:**

```
Via get(): 100
use_count: 1
unique: Yes
After share, use_count: 2
After sp.reset(), sp2 use_count: 1
After swap: spA=2, spB=1
```

### shared_ptr in Data Structures

```cpp
#include <iostream>
#include <memory>
#include <vector>
using namespace std;

class Node {
public:
    int data;
    vector<shared_ptr<Node>> children;

    Node(int d) : data(d) {
        cout << "Node " << data << " created" << endl;
    }
    ~Node() {
        cout << "Node " << data << " destroyed" << endl;
    }

    void addChild(shared_ptr<Node> child) {
        children.push_back(child);
    }
};

int main() {
    // WHY: shared_ptr useful when object has multiple owners
    auto root = make_shared<Node>(1);
    auto child1 = make_shared<Node>(2);
    auto child2 = make_shared<Node>(3);

    root->addChild(child1);
    root->addChild(child2);

    cout << "Root has " << root->children.size() << " children" << endl;
    cout << "child1 use_count: " << child1.use_count() << endl;  // 2 (our copy + root's copy)

    // WHY: All nodes properly destroyed when main ends
    return 0;
}
```

**Output:**

```
Node 1 created
Node 2 created
Node 3 created
Root has 2 children
child1 use_count: 2
Node 1 destroyed
Node 2 destroyed
Node 3 destroyed
```

---

## 6. std::weak_ptr

### What is weak_ptr?

**std::weak_ptr** provides a **non-owning reference** to an object managed by `shared_ptr`. It doesn't affect the reference count and doesn't prevent the object from being deleted.

### Why Do We Need weak_ptr?

**To break circular references!** When two objects have `shared_ptr` to each other, neither can be deleted (memory leak).

### The Circular Reference Problem

```cpp
#include <iostream>
#include <memory>
using namespace std;

// BAD: Using shared_ptr causes circular reference
class PersonBad {
public:
    string name;
    shared_ptr<PersonBad> partner;  // Problem!

    PersonBad(string n) : name(n) {
        cout << name << " created" << endl;
    }
    ~PersonBad() {
        cout << name << " destroyed" << endl;
    }
};

void circularProblem() {
    auto alice = make_shared<PersonBad>("Alice");
    auto bob = make_shared<PersonBad>("Bob");

    alice->partner = bob;  // Alice -> Bob
    bob->partner = alice;  // Bob -> Alice (circular!)

    cout << "Alice use_count: " << alice.use_count() << endl;  // 2
    cout << "Bob use_count: " << bob.use_count() << endl;      // 2

    // When function ends:
    // alice goes out of scope, but Bob still points to Alice
    // bob goes out of scope, but Alice still points to Bob
    // Neither is destroyed! MEMORY LEAK!
}

int main() {
    cout << "=== Circular Reference Problem ===" << endl;
    circularProblem();
    cout << "Notice: No 'destroyed' messages - MEMORY LEAK!" << endl;
    return 0;
}
```

**Output:**

```
=== Circular Reference Problem ===
Alice created
Bob created
Alice use_count: 2
Bob use_count: 2
Notice: No 'destroyed' messages - MEMORY LEAK!
```

### Solution: weak_ptr

```cpp
#include <iostream>
#include <memory>
using namespace std;

// GOOD: Using weak_ptr breaks circular reference
class PersonGood {
public:
    string name;
    weak_ptr<PersonGood> partner;  // Solution!

    PersonGood(string n) : name(n) {
        cout << name << " created" << endl;
    }
    ~PersonGood() {
        cout << name << " destroyed" << endl;
    }

    void showPartner() {
        // WHY: Must lock() to access weak_ptr
        if (auto sp = partner.lock()) {
            cout << name << "'s partner is " << sp->name << endl;
        } else {
            cout << name << " has no partner (or partner deleted)" << endl;
        }
    }
};

void fixedWithWeakPtr() {
    auto alice = make_shared<PersonGood>("Alice");
    auto bob = make_shared<PersonGood>("Bob");

    alice->partner = bob;  // weak_ptr doesn't increase count
    bob->partner = alice;

    cout << "Alice use_count: " << alice.use_count() << endl;  // 1
    cout << "Bob use_count: " << bob.use_count() << endl;      // 1

    alice->showPartner();
    bob->showPartner();

    // Now both will be properly destroyed!
}

int main() {
    cout << "=== Fixed with weak_ptr ===" << endl;
    fixedWithWeakPtr();
    cout << "Both properly destroyed!" << endl;
    return 0;
}
```

**Output:**

```
=== Fixed with weak_ptr ===
Alice created
Bob created
Alice use_count: 1
Bob use_count: 1
Alice's partner is Bob
Bob's partner is Alice
Bob destroyed
Alice destroyed
Both properly destroyed!
```

### weak_ptr Member Functions

```cpp
#include <iostream>
#include <memory>
using namespace std;

int main() {
    auto sp = make_shared<int>(42);
    weak_ptr<int> wp = sp;

    // use_count() - same as shared_ptr's count
    cout << "use_count: " << wp.use_count() << endl;

    // expired() - true if object deleted
    cout << "expired: " << (wp.expired() ? "Yes" : "No") << endl;

    // lock() - returns shared_ptr (or empty if expired)
    if (auto locked = wp.lock()) {
        cout << "Value via lock(): " << *locked << endl;
    }

    // Reset the shared_ptr
    sp.reset();

    cout << "After sp.reset():" << endl;
    cout << "expired: " << (wp.expired() ? "Yes" : "No") << endl;

    if (auto locked = wp.lock()) {
        cout << "Value: " << *locked << endl;
    } else {
        cout << "Object no longer exists" << endl;
    }

    return 0;
}
```

**Output:**

```
use_count: 1
expired: No
Value via lock(): 42
After sp.reset():
expired: Yes
Object no longer exists
```

### weak_ptr Use Cases

```cpp
#include <iostream>
#include <memory>
#include <unordered_map>
using namespace std;

// WHY: Cache that doesn't prevent objects from being deleted
class Cache {
    unordered_map<int, weak_ptr<string>> cache;

public:
    void store(int key, shared_ptr<string> value) {
        cache[key] = value;  // Store as weak_ptr
    }

    shared_ptr<string> get(int key) {
        auto it = cache.find(key);
        if (it != cache.end()) {
            // Try to lock - returns empty if expired
            return it->second.lock();
        }
        return nullptr;
    }
};

int main() {
    Cache cache;

    {
        auto data = make_shared<string>("Important Data");
        cache.store(1, data);

        auto retrieved = cache.get(1);
        if (retrieved) {
            cout << "Retrieved: " << *retrieved << endl;
        }
        // data goes out of scope here
    }

    // Try to get after original was destroyed
    auto retrieved = cache.get(1);
    if (retrieved) {
        cout << "Still available: " << *retrieved << endl;
    } else {
        cout << "Data expired from cache" << endl;
    }

    return 0;
}
```

**Output:**

```
Retrieved: Important Data
Data expired from cache
```

---

## 7. make_unique and make_shared

### Why Prefer make_* Functions?

1. **Exception Safety**: No risk of memory leak if exception thrown
2. **Efficiency**: `make_shared` combines object and control block allocation
3. **Cleaner Code**: No `new` keyword
4. **Type Deduction**: Less typing with `auto`

### make_unique (C++14)

```cpp
#include <iostream>
#include <memory>
using namespace std;

class Widget {
    int id;
public:
    Widget(int i) : id(i) { cout << "Widget " << id << endl; }
    Widget(int i, string s) : id(i) { cout << "Widget " << id << ": " << s << endl; }
};

int main() {
    // WHY: make_unique is preferred over new

    // Single value
    auto p1 = make_unique<int>(42);

    // Object with constructor
    auto p2 = make_unique<Widget>(1);
    auto p3 = make_unique<Widget>(2, "hello");

    // Array (size as argument)
    auto arr = make_unique<int[]>(5);

    cout << "Value: " << *p1 << endl;

    return 0;
}
```

**Output:**

```
Widget 1
Widget 2: hello
Value: 42
```

### make_shared

```cpp
#include <iostream>
#include <memory>
using namespace std;

int main() {
    // WHY: make_shared more efficient than shared_ptr(new T)

    // Preferred
    auto sp1 = make_shared<int>(100);

    // Less efficient (two allocations)
    shared_ptr<int> sp2(new int(200));

    cout << "sp1: " << *sp1 << endl;
    cout << "sp2: " << *sp2 << endl;

    return 0;
}
```

### Exception Safety

```cpp
#include <iostream>
#include <memory>
using namespace std;

void mightThrow() {
    throw runtime_error("Exception!");
}

// BAD: Potential memory leak
void unsafe() {
    // If mightThrow() called between new and shared_ptr construction,
    // memory leaks!
    // processWidget(shared_ptr<Widget>(new Widget), mightThrow());
}

// GOOD: Exception safe
void safe() {
    // make_shared is atomic - no leak possible
    auto widget = make_shared<int>(42);
    // Even if this throws, widget properly cleaned up
}

int main() {
    cout << "make_shared provides exception safety" << endl;
    return 0;
}
```

### When NOT to Use make_*

```cpp
#include <iostream>
#include <memory>
using namespace std;

// Custom deleter - can't use make_shared
void closeFile(FILE* f) {
    if (f) {
        cout << "Closing file" << endl;
        fclose(f);
    }
}

int main() {
    // Can't use make_shared with custom deleter
    shared_ptr<FILE> file(fopen("test.txt", "w"), closeFile);

    if (file) {
        fprintf(file.get(), "Hello!");
    }

    // Can't use make_unique with custom deleter
    // Must use: unique_ptr<T, Deleter>(ptr, deleter)

    return 0;
}
```

---

## 8. Custom Deleters

### Why Custom Deleters?

- Managing non-memory resources (files, sockets, handles)
- Interfacing with C libraries
- Special cleanup requirements
- Logging destruction

### Custom Deleter with unique_ptr

```cpp
#include <iostream>
#include <memory>
using namespace std;

// WHY: Custom deleter must be part of unique_ptr type
struct FileDeleter {
    void operator()(FILE* f) const {
        if (f) {
            cout << "Closing file with custom deleter" << endl;
            fclose(f);
        }
    }
};

int main() {
    // Method 1: Functor as deleter type
    unique_ptr<FILE, FileDeleter> file1(fopen("test1.txt", "w"));

    // Method 2: Function pointer
    unique_ptr<FILE, void(*)(FILE*)> file2(
        fopen("test2.txt", "w"),
        [](FILE* f) {
            if (f) {
                cout << "Lambda deleter" << endl;
                fclose(f);
            }
        }
    );

    // Method 3: Lambda with decltype
    auto deleter = [](FILE* f) {
        if (f) {
            cout << "Auto lambda deleter" << endl;
            fclose(f);
        }
    };
    unique_ptr<FILE, decltype(deleter)> file3(fopen("test3.txt", "w"), deleter);

    if (file1) fputs("Content 1", file1.get());
    if (file2) fputs("Content 2", file2.get());
    if (file3) fputs("Content 3", file3.get());

    return 0;
}
```

**Output:**

```
Auto lambda deleter
Lambda deleter
Closing file with custom deleter
```

### Custom Deleter with shared_ptr

```cpp
#include <iostream>
#include <memory>
using namespace std;

class Connection {
public:
    int id;
    Connection(int i) : id(i) { cout << "Connection " << id << " opened" << endl; }
};

void closeConnection(Connection* c) {
    if (c) {
        cout << "Connection " << c->id << " closed by deleter" << endl;
        delete c;
    }
}

int main() {
    // WHY: shared_ptr deleter is simpler - just pass to constructor

    // Function pointer deleter
    shared_ptr<Connection> conn1(new Connection(1), closeConnection);

    // Lambda deleter
    shared_ptr<Connection> conn2(new Connection(2), [](Connection* c) {
        cout << "Lambda closing connection " << c->id << endl;
        delete c;
    });

    cout << "Using connections..." << endl;

    return 0;
}
```

**Output:**

```
Connection 1 opened
Connection 2 opened
Using connections...
Lambda closing connection 2
Connection 1 closed by deleter
```

### Deleter for Arrays

```cpp
#include <iostream>
#include <memory>
using namespace std;

int main() {
    // WHY: Default deleter uses delete, not delete[]
    // For arrays, use array specialization or custom deleter

    // Method 1: Array specialization (preferred)
    unique_ptr<int[]> arr1 = make_unique<int[]>(5);

    // Method 2: Custom deleter for shared_ptr (no array specialization)
    shared_ptr<int> arr2(new int[5], [](int* p) {
        cout << "Deleting array with delete[]" << endl;
        delete[] p;
    });

    // Or use default_delete<T[]>
    shared_ptr<int> arr3(new int[5], default_delete<int[]>());

    return 0;
}
```

---

## 9. Smart Pointer Comparison

### Comparison Table

| Feature | unique_ptr | shared_ptr | weak_ptr |
| --- | --- | --- | --- |
| **Ownership** | Exclusive | Shared | None |
| **Copyable** | ❌ No | ✅ Yes | ✅ Yes |
| **Moveable** | ✅ Yes | ✅ Yes | ✅ Yes |
| **Ref Counting** | ❌ No | ✅ Yes | Observes |
| **Size** | 1 pointer | 2 pointers | 2 pointers |
| **Overhead** | Minimal | Moderate | Minimal |
| **Thread-safe** | ❌ No | Ref count only | Ref count only |
| **Custom Deleter** | Template param | Constructor | N/A |

### Memory Layout

```
unique_ptr<T>:
+-------------+
| raw pointer | --> [Object]
+-------------+

shared_ptr<T>:
+-------------+     +-----------------+
| raw pointer | --> |    Object       |
+-------------+     +-----------------+
| ctrl block* | --> | ref_count: N    |
+-------------+     | weak_count: M   |
                    | deleter         |
                    +-----------------+

weak_ptr<T>:
+-------------+
| (unused)    |
+-------------+
| ctrl block* | --> (same control block as shared_ptr)
+-------------+
```

### Choosing the Right Smart Pointer

```cpp
#include <iostream>
#include <memory>
using namespace std;

// Use unique_ptr: single owner, no sharing needed
class FileHandler {
    unique_ptr<FILE, decltype(&fclose)> file;
public:
    FileHandler(const char* name) : file(fopen(name, "r"), fclose) {}
};

// Use shared_ptr: multiple owners need the resource
class SharedCache {
    shared_ptr<vector<int>> data;
public:
    SharedCache(shared_ptr<vector<int>> d) : data(d) {}
};

// Use weak_ptr: observer that doesn't prevent deletion
class Observer {
    weak_ptr<int> observed;
public:
    void observe(shared_ptr<int> obj) { observed = obj; }
    void check() {
        if (auto sp = observed.lock()) {
            cout << "Object alive: " << *sp << endl;
        } else {
            cout << "Object gone" << endl;
        }
    }
};

int main() {
    cout << "Choose based on ownership semantics!" << endl;
    return 0;
}
```

---

## 10. Common Pitfalls

### Pitfall 1: Creating shared_ptr from Raw Pointer Multiple Times

```cpp
#include <iostream>
#include <memory>
using namespace std;

int main() {
    int* raw = new int(42);

    // BAD: Two independent shared_ptrs managing same pointer!
    shared_ptr<int> sp1(raw);
    // shared_ptr<int> sp2(raw);  // DISASTER! Double delete!

    // GOOD: Copy or use same shared_ptr
    shared_ptr<int> sp2 = sp1;  // Share ownership properly

    cout << "Both point to: " << *sp1 << endl;

    return 0;
}
```

### Pitfall 2: Circular References with shared_ptr

```cpp
// Already covered in weak_ptr section
// Solution: Use weak_ptr for back-references
```

### Pitfall 3: Using get() and Storing Raw Pointer

```cpp
#include <iostream>
#include <memory>
using namespace std;

int main() {
    int* danglingPtr = nullptr;

    {
        auto sp = make_shared<int>(100);
        danglingPtr = sp.get();  // Get raw pointer
        cout << "Inside scope: " << *danglingPtr << endl;
    }

    // BAD: sp destroyed, danglingPtr is now dangling!
    // cout << *danglingPtr << endl;  // Undefined behavior!

    cout << "Never store raw pointers from get()!" << endl;

    return 0;
}
```

### Pitfall 4: Using this in shared_ptr

```cpp
#include <iostream>
#include <memory>
using namespace std;

class Bad {
public:
    shared_ptr<Bad> getShared() {
        // BAD: Creates new shared_ptr, double-delete!
        // return shared_ptr<Bad>(this);
        return nullptr;
    }
};

// GOOD: Use enable_shared_from_this
class Good : public enable_shared_from_this<Good> {
public:
    shared_ptr<Good> getShared() {
        return shared_from_this();  // Safe!
    }

    void doSomething() {
        auto self = shared_from_this();
        cout << "Safely got shared_ptr to this" << endl;
    }
};

int main() {
    auto g = make_shared<Good>();
    auto g2 = g->getShared();

    cout << "Same object: " << (g.get() == g2.get() ? "Yes" : "No") << endl;
    cout << "use_count: " << g.use_count() << endl;

    return 0;
}
```

**Output:**

```
Same object: Yes
use_count: 2
```

### Pitfall 5: Passing shared_ptr by Value Unnecessarily

```cpp
#include <iostream>
#include <memory>
using namespace std;

// BAD: Copies shared_ptr, increments/decrements ref count
void processValue(shared_ptr<int> sp) {
    cout << "Value: " << *sp << endl;
}

// GOOD: Reference if you don't need ownership
void processRef(const shared_ptr<int>& sp) {
    cout << "Value: " << *sp << endl;
}

// GOOD: Raw pointer if you just need access
void processRaw(int* p) {
    cout << "Value: " << *p << endl;
}

int main() {
    auto sp = make_shared<int>(42);

    processValue(sp);  // Copies shared_ptr
    processRef(sp);    // No copy
    processRaw(sp.get());  // Just pointer access

    return 0;
}
```

---

## 11. Best Practices

### ✅ DO: Prefer unique_ptr as Default Choice

```cpp
// GOOD: unique_ptr when single ownership is sufficient
auto resource = make_unique<Resource>();
```

### ✅ DO: Use make_unique and make_shared

```cpp
// GOOD: Exception safe, efficient
auto up = make_unique<Widget>(42);
auto sp = make_shared<Widget>(42);

// AVOID: Direct new
// unique_ptr<Widget> up(new Widget(42));
```

### ✅ DO: Use weak_ptr to Break Cycles

```cpp
class Node {
    shared_ptr<Node> next;  // Forward reference - shared
    weak_ptr<Node> prev;    // Back reference - weak
};
```

### ✅ DO: Pass by const Reference When Not Taking Ownership

```cpp
void read(const shared_ptr<Data>& data) {
    // Just reading, don't need ownership
    cout << data->value;
}
```

### ❌ DON'T: Mix Raw and Smart Pointers

```cpp
// BAD: Confusing ownership
int* raw = new int(42);
auto sp = make_shared<int>(42);
raw = sp.get();  // Who owns what now?
```

### ❌ DON'T: Use shared_ptr When unique_ptr Suffices

```cpp
// BAD: Unnecessary overhead
shared_ptr<Widget> widget = make_shared<Widget>();

// GOOD: Simpler, more efficient
unique_ptr<Widget> widget = make_unique<Widget>();
```

### ❌ DON'T: Return unique_ptr and Store as shared_ptr

```cpp
// Factory returns unique_ptr
unique_ptr<Widget> createWidget();

// GOOD: Keep as unique_ptr if possible
auto widget = createWidget();

// OK: Convert if sharing needed
shared_ptr<Widget> shared = createWidget();  // Implicit conversion
```

---

## 12. Summary

### Key Takeaways

1. **Smart Pointers Prevent Memory Issues**
    - Automatic deallocation
    - RAII compliance
    - Exception safety
2. **Three Types for Different Needs**
    - `unique_ptr`: Exclusive ownership (default choice)
    - `shared_ptr`: Shared ownership (when truly needed)
    - `weak_ptr`: Non-owning reference (break cycles)
3. *Use make_ Functions*
    - `make_unique` (C++14) for unique_ptr
    - `make_shared` for shared_ptr
    - Exception safe and efficient
4. **Avoid Raw Pointers**
    - Use smart pointers for ownership
    - Raw pointers only for non-owning access
5. **Watch for Pitfalls**
    - Circular references (use weak_ptr)
    - Creating multiple shared_ptr from raw pointer
    - Dangling raw pointers from get()

### Quick Reference

| Task | Solution |
| --- | --- |
| Single owner | `unique_ptr<T>` |
| Multiple owners | `shared_ptr<T>` |
| Non-owning reference | `weak_ptr<T>` |
| Create unique_ptr | `make_unique<T>(args)` |
| Create shared_ptr | `make_shared<T>(args)` |
| Transfer ownership | `std::move(ptr)` |
| Check if valid | `if (ptr)` or `ptr != nullptr` |
| Get raw pointer | `ptr.get()` |
| Release ownership | `ptr.release()` (unique only) |
| Reset/replace | `ptr.reset()` or `ptr.reset(new T)` |
| Check ref count | `ptr.use_count()` |
| Lock weak_ptr | `weak.lock()` |

### Keywords Covered

✅ Smart pointers (3)
✅ unique_ptr (8)
✅ shared_ptr (8)
✅ weak_ptr (6)
✅ RAII (4)
✅ Automatic memory management (2)
✅ Exclusive ownership (3)
✅ Shared ownership (3)
✅ Reference counting (4)
✅ make_unique (3)
✅ make_shared (3)
✅ Custom deleters (4)
✅ Circular references (3)
✅ Memory leaks prevention (2)
✅ Dangling pointers (2)
✅ use_count (2)
✅ lock() (2)
✅ expired() (1)
✅ reset() (2)
✅ get() (2)
✅ release() (1)
✅ enable_shared_from_this (1)

---