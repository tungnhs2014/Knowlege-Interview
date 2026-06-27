# 7.4. Templates - Template Template Parameters & Advanced Topics

---

## Table of Contents

1. Template Template Parameters
2. Alias Templates
3. Template Deduction Guides (C++17)
4. Dependent Names
5. Template Friends
6. Summary
7. Interview Preparation

---

## 1. Template Template Parameters

### 1.1 What are Template Template Parameters?

**Template template parameters** allow you to pass a template itself as a template parameter, not just a type. This enables writing generic code that works with any template class.

**Syntax:**

```cpp
template <template <typename> class Container>
class MyClass {
    // Can use Container<T> for any T
};
```

**Why They Matter:**

```cpp
// WITHOUT template template parameters - must specify type
template <typename ContainerType>
class Stack {
    ContainerType data;  // Must be fully instantiated: vector<int>, list<double>
};

// WITH template template parameters - type is flexible
template <template <typename> class Container>
class Stack {
    template <typename T>
    Container<T> data;  // Can use Container with any type
};
```

### 1.2 Basic Syntax

```cpp
#include <iostream>
#include <vector>
#include <list>
using namespace std;

// WHY: Container is a template template parameter
// It's a template that takes one type parameter
template <typename T, template <typename> class Container>
class Stack {
private:
    Container<T> data;

public:
    void push(const T& value) {
        data.push_back(value);
    }

    T top() const {
        return data.back();
    }

    size_t size() const {
        return data.size();
    }
};

int main() {
    // WHY: We pass vector as template parameter (not vector<int>)
    Stack<int, vector> intStack;
    intStack.push(10);
    intStack.push(20);
    cout << "Top: " << intStack.top() << endl;

    // WHY: Same Stack class works with list
    Stack<double, list> doubleStack;
    doubleStack.push(3.14);
    doubleStack.push(2.71);
    cout << "Top: " << doubleStack.top() << endl;

    return 0;
}
```

**Output:**

```
Top: 20
Top: 2.71
```

### 1.3 Handling Default Template Parameters

**Problem:** STL containers have default template parameters (like allocator):

```cpp
template <typename T, typename Allocator = allocator<T>>
class vector;

template <typename T, typename Allocator = allocator<T>>
class list;
```

**Solution:** Template template parameter must match the number of parameters:

```cpp
#include <iostream>
#include <vector>
#include <list>
using namespace std;

// WHY: Must account for allocator parameter
template <typename T,
          template <typename, typename> class Container>
class Stack {
private:
    // WHY: Use allocator<T> as second parameter
    Container<T, allocator<T>> data;

public:
    void push(const T& value) {
        data.push_back(value);
    }

    T top() const {
        return data.back();
    }

    size_t size() const {
        return data.size();
    }
};

int main() {
    Stack<int, vector> intStack;
    intStack.push(42);
    cout << "Top: " << intStack.top() << endl;

    Stack<string, list> stringStack;
    stringStack.push("Hello");
    cout << "Top: " << stringStack.top() << endl;

    return 0;
}
```

**Output:**

```
Top: 42
Top: Hello
```

### 1.4 C++17 Improvement - Template Template Parameters

**C++17 allows matching template template parameters with different numbers of parameters:**

```cpp
#include <iostream>
#include <vector>
#include <set>
using namespace std;

// WHY: variadic template parameter matches any number of params
template <typename T,
          template <typename...> class Container>
class Wrapper {
private:
    Container<T> data;

public:
    void add(const T& value) {
        // WHY: insert works for both vector and set
        if constexpr (requires { data.push_back(value); }) {
            data.push_back(value);
        } else {
            data.insert(value);
        }
    }

    size_t size() const {
        return data.size();
    }
};

int main() {
    // WHY: Works with vector (2 template params: T, Allocator)
    Wrapper<int, vector> w1;
    w1.add(10);
    cout << "Vector size: " << w1.size() << endl;

    // WHY: Works with set (3 template params: T, Compare, Allocator)
    Wrapper<int, set> w2;
    w2.add(20);
    cout << "Set size: " << w2.size() << endl;

    return 0;
}
```

### 1.5 Real-World Example - Generic Printer

```cpp
#include <iostream>
#include <vector>
#include <list>
#include <deque>
using namespace std;

// WHY: Print any container without knowing its type in advance
template <template <typename, typename> class Container,
          typename T>
void print_container(const Container<T, allocator<T>>& c) {
    cout << "Container elements: ";
    for (const auto& elem : c) {
        cout << elem << " ";
    }
    cout << endl;
}

int main() {
    vector<int> vec = {1, 2, 3, 4, 5};
    list<double> lst = {1.1, 2.2, 3.3};
    deque<char> deq = {'a', 'b', 'c'};

    // WHY: Same function works for all containers
    print_container(vec);
    print_container(lst);
    print_container(deq);

    return 0;
}
```

**Output:**

```
Container elements: 1 2 3 4 5
Container elements: 1.1 2.2 3.3
Container elements: a b c
```

### 1.6 Complex Example - Policy-Based Design

```cpp
#include <iostream>
#include <vector>
#include <list>
using namespace std;

// WHY: Storage policies
template <typename T>
class VectorStorage {
public:
    using Container = vector<T>;
};

template <typename T>
class ListStorage {
public:
    using Container = list<T>;
};

// WHY: Template template parameter for storage policy
template <typename T,
          template <typename> class StoragePolicy>
class DataManager {
private:
    typename StoragePolicy<T>::Container data;

public:
    void add(const T& value) {
        data.push_back(value);
    }

    void print() const {
        cout << "Data: ";
        for (const auto& elem : data) {
            cout << elem << " ";
        }
        cout << endl;
    }
};

int main() {
    // WHY: Choose storage policy at compile time
    DataManager<int, VectorStorage> vectorManager;
    vectorManager.add(10);
    vectorManager.add(20);
    vectorManager.print();

    DataManager<int, ListStorage> listManager;
    listManager.add(30);
    listManager.add(40);
    listManager.print();

    return 0;
}
```

**Output:**

```
Data: 10 20
Data: 30 40
```

### 1.7 When to Use Template Template Parameters

**✅ Use When:**

1. **Generic Container Operations** - Work with any container type

```cpp
template <template <typename> class Container>
void process_any_container(Container<int>& c);
```

1. **Policy-Based Design** - Interchangeable policies

```cpp
template <typename T, template <typename> class Policy>
class Component {
    Policy<T> policy;
};
```

1. **Type-Erasing Wrappers** - Hide implementation details

```cpp
template <template <typename...> class SmartPtr>
class ResourceManager;
```

**❌ Don't Use When:**

1. **Type is Already Known** - Use normal type parameter

```cpp
// DON'T: Overcomplicating
template <template <typename> class C>
void func(C<int> x);

// DO: Simple and clear
template <typename T>
void func(vector<T> x);
```

1. **No Need for Multiple Containers** - YAGNI principle
2. **Increases Compilation Time** - Only when benefit justifies cost

---

## 2. Alias Templates

### 2.1 What are Alias Templates?

**Alias templates** create new names for template families, similar to typedef but for templates. Introduced in C++11 with the `using` keyword.

**Why Alias Templates Matter:**

```cpp
// OLD WAY - typedef doesn't work with templates
template <typename T>
typedef vector<T> Vec;  // ERROR: Can't typedef templates!

// NEW WAY - using works with templates
template <typename T>
using Vec = vector<T>;  // OK: Alias template
```

### 2.2 Basic Syntax

```cpp
#include <iostream>
#include <vector>
#include <string>
using namespace std;

// WHY: Create convenient aliases for complex types
template <typename T>
using Vec = vector<T>;

template <typename Key, typename Value>
using Dict = map<Key, Value>;

int main() {
    // WHY: Much cleaner than vector<int>
    Vec<int> numbers = {1, 2, 3, 4, 5};

    // WHY: More readable than map<string, int>
    Dict<string, int> ages = {
        {"Alice", 30},
        {"Bob", 25}
    };

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

### 2.3 Partial Specialization with Alias Templates

```cpp
#include <iostream>
#include <memory>
using namespace std;

// WHY: Simplify smart pointer usage
template <typename T>
using UniquePtr = unique_ptr<T>;

template <typename T>
using SharedPtr = shared_ptr<T>;

// WHY: Create pointer type based on condition
template <typename T, bool Shared>
using Ptr = conditional_t<Shared, SharedPtr<T>, UniquePtr<T>>;

int main() {
    // WHY: Unique ownership
    Ptr<int, false> unique = make_unique<int>(42);
    cout << "Unique: " << *unique << endl;

    // WHY: Shared ownership
    Ptr<double, true> shared = make_shared<double>(3.14);
    auto shared2 = shared;  // Can copy
    cout << "Shared: " << *shared << endl;

    return 0;
}
```

**Output:**

```
Unique: 42
Shared: 3.14 
```

### 2.4 Type Traits Aliases

**Standard library uses alias templates extensively:**

```cpp
#include <type_traits>

// WHY: _t suffix provides cleaner syntax
template <typename T>
using remove_const_t = typename remove_const<T>::type;

template <typename T>
using remove_reference_t = typename remove_reference<T>::type;

template <bool B, typename T, typename F>
using conditional_t = typename conditional<B, T, F>::type;

// Usage comparison
typename remove_const<const int>::type x;  // OLD: Verbose
remove_const_t<const int> y;               // NEW: Clean
```

### 2.5 Custom Alias Templates

```cpp
#include <iostream>
#include <vector>
#include <list>
using namespace std;

// WHY: Create domain-specific aliases
template <typename T>
using VertexList = vector<T>;

template <typename T>
using EdgeList = list<pair<T, T>>;

template <typename T>
using Graph = pair<VertexList<T>, EdgeList<T>>;

int main() {
    // WHY: Code reads like domain language
    VertexList<int> vertices = {1, 2, 3, 4};
    EdgeList<int> edges = {{1, 2}, {2, 3}, {3, 4}};

    Graph<int> graph = {vertices, edges};

    cout << "Vertices: " << graph.first.size() << endl;
    cout << "Edges: " << graph.second.size() << endl;

    return 0;
}
```

**Output:**

```
Vertices: 4
Edges: 3
```

### 2.6 Alias Templates with Default Arguments

```cpp
#include <iostream>
#include <vector>
#include <memory>
using namespace std;

// WHY: Provide sensible defaults
template <typename T, typename Allocator = allocator<T>>
using Vector = vector<T, Allocator>;

template <typename T, typename Deleter = default_delete<T>>
using UniquePtr = unique_ptr<T, Deleter>;

// WHY: Custom deleter example
struct FileDeleter {
    void operator()(FILE* file) const {
        if (file) {
            fclose(file);
            cout << "File closed" << endl;
        }
    }
};

int main() {
    // WHY: Use default allocator
    Vector<int> vec = {1, 2, 3};

    // WHY: Use custom deleter
    UniquePtr<FILE, FileDeleter> file(fopen("test.txt", "w"));

    return 0;
}
```

### 2.7 Template Aliases vs Typedef

**Comparison:**

| Feature | typedef | using (Alias Template) |
| --- | --- | --- |
| **With Templates** | ❌ Doesn't work | ✅ Works perfectly |
| **Syntax** | Type first | Type after = |
| **Readability** | Complex for pointers/functions | Always clear |
| **C++ Version** | C++98+ | C++11+ |

**Examples:**

```cpp
// Function pointer typedef - hard to read
typedef void (*FuncPtr)(int, double);

// Function pointer using - clear
using FuncPtr = void(*)(int, double);

// Array typedef - confusing
typedef int IntArray[10];

// Array using - obvious
using IntArray = int[10];

// Template - typedef CAN'T do this
template <typename T>
using Vec = vector<T>
```

---

## 3. Template Deduction Guides (C++17)

### 3.1 What are Deduction Guides?

**Template deduction guides** help the compiler deduce template arguments for class templates, enabling cleaner code without explicit type specifications.

**Before C++17:**

```cpp
vector<int> vec = {1, 2, 3};  // Must specify <int>
pair<int, double> p(1, 2.5);  // Must specify <int, double>
```

**C++17 CTAD (Class Template Argument Deduction):**

```cpp
vector vec = {1, 2, 3};       // Deduces vector<int>
pair p(1, 2.5);               // Deduces pair<int, double>
```

### 3.2 Implicit Deduction Guides

**Compiler automatically creates deduction guides from constructors:**

```cpp
#include <iostream>
using namespace std;

template <typename T>
class Container {
private:
    T value;

public:
    // WHY: Compiler creates deduction guide from constructor
    Container(T val) : value(val) {}

    T get() const { return value; }
};

int main() {
    // WHY: Deduces Container<int>
    Container c1(42);

    // WHY: Deduces Container<double>
    Container c2(3.14);

    // WHY: Deduces Container<const char*>
    Container c3("hello");

    cout << c1.get() << endl;
    cout << c2.get() << endl;
    cout << c3.get() << endl;

    return 0;
}
```

**Output:**

```
42
3.14
hello
```

### 3.3 Explicit Deduction Guides

**Custom deduction guides for complex cases:**

```cpp
#include <iostream>
#include <string>
using namespace std;

template <typename T>
class Wrapper {
private:
    T value;

public:
    Wrapper(T val) : value(val) {}

    T get() const { return value; }
};

// WHY: Deduction guide for C-string → string
Wrapper(const char*) -> Wrapper<string>;

int main() {
    // WHY: Without guide → Wrapper<const char*>
    // With guide → Wrapper<string>
    Wrapper w("hello");

    // WHY: Verify it's string, not const char*
    cout << typeid(decltype(w.get())).name() << endl;

    return 0;
}
```

### 3.4 Real-World Example - Pair Deduction

```cpp
#include <iostream>
using namespace std;

template <typename T1, typename T2>
struct Pair {
    T1 first;
    T2 second;

    Pair(T1 f, T2 s) : first(f), second(s) {}

    void print() const {
        cout << "(" << first << ", " << second << ")" << endl;
    }
};

// WHY: Deduction guide for aggregate initialization
template <typename T1, typename T2>
Pair(T1, T2) -> Pair<T1, T2>;

int main() {
    // WHY: CTAD deduces types
    Pair p1(42, 3.14);           // Pair<int, double>
    Pair p2("hello", 100);       // Pair<const char*, int>
    Pair p3(1.5, 2.5);           // Pair<double, double>

    p1.print();
    p2.print();
    p3.print();

    return 0;
}
```

**Output:**

```
(42, 3.14)
(hello, 100)
(1.5, 2.5)
```

### 3.5 Deduction Guides for Array Decay

```cpp
#include <iostream>
using namespace std;

template <typename T>
class Array {
private:
    T* data;
    size_t size;

public:
    Array(T* arr, size_t sz) : data(arr), size(sz) {}

    void print() const {
        for (size_t i = 0; i < size; ++i) {
            cout << data[i] << " ";
        }
        cout << endl;
    }
};

// WHY: Deduce from array
template <typename T, size_t N>
Array(T(&)[N]) -> Array<T>;

int main() {
    int arr[] = {1, 2, 3, 4, 5};

    // WHY: Deduces Array<int>
    Array a(arr, 5);
    a.print();

    return 0;
}
```

**Output:**

```
1 2 3 4 5
```

---

## 4. Dependent Names

### 4.1 What are Dependent Names?

**Dependent names** are names that depend on template parameters. The compiler cannot fully resolve them until the template is instantiated.

**Why They Matter:**

```cpp
template <typename T>
class MyClass {
    // WHY: Is T::value a type or a value?
    // Compiler doesn't know until T is known
    T::value x;  // Ambiguous!
};
```

### 4.2 typename Keyword for Dependent Types

**Problem:** Compiler assumes dependent names are values, not types.

**Solution:** Use `typename` keyword to tell compiler it's a type.

```cpp
#include <iostream>
#include <vector>
using namespace std;

template <typename T>
class Container {
public:
    // WHY: T::value_type depends on T
    // Must use typename to say it's a type
    using value_type = typename T::value_type;

    void process(const T& container) {
        // WHY: iterator depends on T
        typename T::iterator it = container.begin();
        cout << "First element: " << *it << endl;
    }
};

int main() {
    vector<int> vec = {10, 20, 30};
    Container<vector<int>> c;
    c.process(vec);

    return 0;
}
```

**Output:**

```
First element: 10
```

### 4.3 template Keyword for Dependent Templates

**Problem:** Compiler doesn't know if dependent name is a template.

**Solution:** Use `template` keyword before member template names.

```cpp
#include <iostream>
using namespace std;

template <typename T>
class Outer {
public:
    template <typename U>
    void inner(U value) {
        cout << "Inner template: " << value << endl;
    }
};

template <typename T>
void call_inner(T& obj) {
    // WHY: Must use .template to call member template
    obj.template inner<int>(42);
}

int main() {
    Outer<double> obj;
    call_inner(obj);

    return 0;
}
```

**Output:**

```
Inner template: 42
```

### 4.4 Two-Phase Name Lookup

**Phase 1:** Non-dependent names resolved when template defined.

**Phase 2:** Dependent names resolved when template instantiated.

```cpp
#include <iostream>
using namespace std;

void helper() {
    cout << "Global helper" << endl;
}

template <typename T>
class MyClass {
public:
    void func() {
        // WHY: Non-dependent - looked up in phase 1
        helper();
    }

    void func2(T value) {
        // WHY: Dependent on T - looked up in phase 2
        process(value);
    }
};

void process(int x) {
    cout << "Processing int: " << x << endl;
}

int main() {
    MyClass<int> obj;
    obj.func();   // Calls global helper
    obj.func2(42); // Calls process(int)

    return 0;
}
```

**Output:**

```
Global helper
Processing int: 42
```

### 4.5 Common Pitfalls

**Pitfall 1: Forgetting typename**

```cpp
template <typename T>
class MyClass {
    // ERROR: Compiler thinks T::type is a value
    T::type* ptr;

    // CORRECT: Tell compiler it's a type
    typename T::type* ptr;
};
```

**Pitfall 2: Forgetting template keyword**

```cpp
template <typename T>
void func(T& obj) {
    // ERROR: Compiler thinks < is less-than operator
    obj.inner<int>(42);

    // CORRECT: Tell compiler it's a template
    obj.template inner<int>(42);
}
```

---

## 5. Template Friends

### 5.1 Non-Template Friend Functions

```cpp
#include <iostream>
using namespace std;

template <typename T>
class Box {
private:
    T value;

public:
    Box(T val) : value(val) {}

    // WHY: Friend function for this specific Box<T>
    friend void print(const Box<T>& box) {
        cout << "Value: " << box.value << endl;
    }
};

int main() {
    Box<int> intBox(42);
    Box<double> doubleBox(3.14);

    print(intBox);
    print(doubleBox);

    return 0;
}
```

**Output:**

```
Value: 42
Value: 3.14
```

### 5.2 Template Friend Functions

```cpp
#include <iostream>
using namespace std;

// Forward declaration
template <typename T>
class Box;

template <typename T>
void print(const Box<T>& box);

template <typename T>
class Box {
private:
    T value;

public:
    Box(T val) : value(val) {}

    // WHY: Declare specific instantiation as friend
    friend void print<T>(const Box<T>& box);
};

template <typename T>
void print(const Box<T>& box) {
    cout << "Value: " << box.value << endl;
}

int main() {
    Box<int> box(100);
    print(box);

    return 0;
}
```

**Output:**

```
Value: 100
```

### 5.3 Template Friend Classes

```cpp
#include <iostream>
using namespace std;

template <typename T>
class Box {
private:
    T value;

public:
    Box(T val) : value(val) {}

    // WHY: All instantiations of Printer are friends
    template <typename U>
    friend class Printer;
};

template <typename T>
class Printer {
public:
    void print(const Box<T>& box) {
        // WHY: Can access private member
        cout << "Printing: " << box.value << endl;
    }
};

int main() {
    Box<int> box(42);
    Printer<int> printer;
    printer.print(box);

    return 0;
}
```

**Output:**

```
Printing: 42
```

---

## Summary

### Key Takeaways

1. **Template Template Parameters Enable Generic Container Operations** - Pass template itself (not instantiated type) as parameter. Essential for policy-based design and generic algorithms working with any container type.
2. **Handle STL Container Default Parameters Carefully** - STL containers have multiple template parameters (like allocator). Match parameter count in template template parameters or use variadic templates (C++17+).
3. **Alias Templates Simplify Complex Type Names** - Create readable names for template families using `using`. Much cleaner than typedef and works with templates. Standard library uses _t suffix extensively.
4. **Template Deduction Guides Enable CTAD** - C++17 Class Template Argument Deduction allows omitting template arguments. Compiler generates guides from constructors automatically, or write custom guides for special cases.
5. **Dependent Names Require Special Keywords** - Use `typename` for dependent types and `template` for dependent member templates. Essential for resolving ambiguity in template code.
6. **Two-Phase Name Lookup Separates Resolution** - Non-dependent names resolved at template definition (phase 1), dependent names at instantiation (phase 2). Understanding this prevents subtle bugs.
7. **Template Friends Grant Access to Private Members** - Can make specific function/class instantiations friends, or all instantiations. Useful for operator overloading and testing.
8. **C++17 Improves Template Template Parameter Matching** - Variadic template parameters (`template <typename...>`) match templates with any number of parameters. Makes working with STL containers much easier.
9. **Alias Templates Provide Zero-Cost Abstraction** - Pure compile-time feature, no runtime overhead. Perfect for creating domain-specific vocabulary and simplifying complex nested types.
10. **Deduction Guides Enhance Code Clarity** - Especially useful for containers, pairs, smart pointers. Reduces boilerplate without sacrificing type safety. Follow convention of same name as class.

---

## Interview Preparation

### Q1: What are template template parameters and when would you use them? Explain the syntax for matching STL containers.

**Answer:**

**Template template parameters** allow passing a template itself (not an instantiated type) as a template argument. This enables writing generic code that works with any template class family.

**Basic Syntax:**

```cpp
// Template template parameter
template <typename T, template <typename> class Container>
class Stack {
    Container<T> data;  // Use Container with any type T
};

// Usage
Stack<int, vector> intStack;    // Pass vector as template
Stack<double, list> doubleStack; // Pass list as template
```

**Key Concept:**

```cpp
// WRONG: This passes an instantiated type
template <typename ContainerType>
class Stack {
    ContainerType data;  // Must be vector<int> or list<double>
};

// RIGHT: This passes the template itself
template <typename T, template <typename> class Container>
class Stack {
    Container<T> data;  // Can use Container<int>, Container<double>, etc.
};
```

**Problem with STL Containers:**

STL containers have multiple template parameters with defaults:

```cpp
template <typename T, typename Allocator = allocator<T>>
class vector;

template <typename T, typename Allocator = allocator<T>>
class list;

template <typename Key, typename Compare = less<Key>,
          typename Allocator = allocator<Key>>
class set;
```

**Solution 1: Match All Parameters**

```cpp
// WHY: Must specify two template parameters
template <typename T,
          template <typename, typename> class Container>
class Stack {
    Container<T, allocator<T>> data;  // Explicitly use allocator
public:
    void push(const T& value) {
        data.push_back(value);
    }
};

// Usage
Stack<int, vector> stack;  // Works
```

**Solution 2: C++17 Variadic Template Parameters**

```cpp
// WHY: Matches any number of template parameters
template <typename T,
          template <typename...> class Container>
class Stack {
    Container<T> data;  // Compiler fills in defaults
public:
    void push(const T& value) {
        if constexpr (requires { data.push_back(value); }) {
            data.push_back(value);
        } else {
            data.insert(value);
        }
    }
};

// Works with vector (2 params), list (2 params), set (3 params)
Stack<int, vector> vecStack;
Stack<int, list> listStack;
Stack<int, set> setStack;
```

**When to Use Template Template Parameters:**

**✅ Good Use Cases:**

1. **Generic Container Operations:**

```cpp
template <template <typename, typename> class Container>
void print_any_container(const Container<int, allocator<int>>& c) {
    for (const auto& elem : c) {
        cout << elem << " ";
    }
}

vector<int> vec = {1, 2, 3};
list<int> lst = {4, 5, 6};
print_any_container(vec);  // Works
print_any_container(lst);  // Works
```

1. **Policy-Based Design:**

```cpp
template <typename T, template <typename> class StoragePolicy>
class Database {
    StoragePolicy<T> storage;
public:
    void add(const T& item) {
        storage.store(item);
    }
};

Database<User, MemoryStorage> memDB;
Database<User, DiskStorage> diskDB;
```

1. **Type-Erasing Wrappers:**

```cpp
template <template <typename...> class SmartPtr>
class ResourceManager {
    SmartPtr<Resource> resource;
};

ResourceManager<unique_ptr> uniqueManager;
ResourceManager<shared_ptr> sharedManager;
```

**❌ Avoid When:**

- Type is already known (use normal type parameter)
- Only works with one container type (no need for genericity)
- Increases compilation time without clear benefit

**Complete Example:**

```cpp
#include <iostream>
#include <vector>
#include <list>
#include <deque>
using namespace std;

// WHY: Generic stack works with any sequential container
template <typename T,
          template <typename, typename> class Container = vector>
class Stack {
private:
    Container<T, allocator<T>> data;

public:
    void push(const T& value) {
        data.push_back(value);
    }

    T pop() {
        T value = data.back();
        data.pop_back();
        return value;
    }

    bool empty() const {
        return data.empty();
    }

    size_t size() const {
        return data.size();
    }
};

int main() {
    // Different storage strategies, same interface
    Stack<int, vector> vecStack;
    Stack<int, list> listStack;
    Stack<int, deque> dequeStack;

    vecStack.push(1);
    listStack.push(2);
    dequeStack.push(3);

    cout << "Vector: " << vecStack.pop() << endl;
    cout << "List: " << listStack.pop() << endl;
    cout << "Deque: " << dequeStack.pop() << endl;

    return 0;
}
```

**Key Points:**

- Template template parameters receive templates, not types
- Must match parameter count (or use variadic in C++17+)
- Enables policy-based design and generic algorithms
- STL containers require special handling due to allocator parameter
- Use when you need true template-level genericity

---

### Q2: Explain alias templates, deduction guides, and dependent names. How do they improve modern C++ code?

**Answer:**

These three features significantly improve C++ template code readability, usability, and correctness.

### 1. Alias Templates

**Alias templates** create new names for template families using the `using` keyword (C++11).

**Problem They Solve:**

```cpp
// typedef CAN'T create template aliases
template <typename T>
typedef vector<T> Vec;  // ERROR: Invalid syntax!

// OLD WORKAROUND: Wrapper struct
template <typename T>
struct Vec {
    using type = vector<T>;
};
Vec<int>::type v;  // Ugly!
```

**Modern Solution:**

```cpp
// CLEAN: Direct template alias
template <typename T>
using Vec = vector<T>;

Vec<int> v = {1, 2, 3};  // Much cleaner!
```

**Real-World Benefits:**

**1. Simplify Complex Types:**

```cpp
// Complex nested types become readable
template <typename K, typename V>
using StringMap = unordered_map<string, V,
                                hash<string>,
                                equal_to<string>,
                                allocator<pair<const string, V>>>;

// Usage
StringMap<int> ages;  // vs unordered_map<string, int, ...>
```

**2. Type Traits Made Easier:**

```cpp
// Standard library uses _t suffix
template <typename T>
using remove_const_t = typename remove_const<T>::type;

template <bool B, typename T, typename F>
using conditional_t = typename conditional<B, T, F>::type;

// Compare
typename remove_const<const int>::type x;  // OLD: Verbose
remove_const_t<const int> y;               // NEW: Clean
```

**3. Partial Template Specialization:**

```cpp
template <typename T>
using SharedVec = shared_ptr<vector<T>>;

SharedVec<int> sv = make_shared<vector<int>>();
```

### 2. Template Deduction Guides (C++17)

**Deduction guides** help the compiler deduce template arguments for class templates (CTAD - Class Template Argument Deduction).

**Problem They Solve:**

```cpp
// Pre-C++17: Must specify template arguments
vector<int> vec = {1, 2, 3};
pair<int, double> p(1, 2.5);
mutex<lock_guard> lg(mtx);  // Explicit types everywhere
```

**C++17 Solution:**

```cpp
// Compiler deduces template arguments
vector vec = {1, 2, 3};       // Deduces vector<int>
pair p(1, 2.5);               // Deduces pair<int, double>
lock_guard lg(mtx);           // Deduces lock_guard<mutex>
```

**How It Works:**

**Implicit Guides (Automatic):**

```cpp
template <typename T>
class Container {
public:
    Container(T val);  // Compiler creates guide from constructor
};

// Compiler generates:
// template <typename T>
// Container(T) -> Container<T>;

Container c(42);  // Deduces Container<int>
```

**Explicit Guides (Custom):**

```cpp
template <typename T>
class Wrapper {
public:
    Wrapper(T val);
};

// WHY: Convert C-string to std::string
Wrapper(const char*) -> Wrapper<string>;

Wrapper w1("hello");  // Deduces Wrapper<string>, not Wrapper<const char*>
Wrapper w2(42);       // Deduces Wrapper<int>
```

**Real-World Example:**

```cpp
// mutex and lock_guard
template <typename Mutex>
class lock_guard {
public:
    explicit lock_guard(Mutex& m);
};

mutex mtx;
lock_guard guard(mtx);  // Deduces lock_guard<mutex>
// vs old: lock_guard<mutex> guard(mtx);
```

**Benefits:**

- Less typing, clearer intent
- Matches how built-in types work
- Reduces boilerplate without sacrificing type safety
- Particularly useful for containers, pairs, smart pointers

### 3. Dependent Names

**Dependent names** are names that depend on template parameters and require special keywords (`typename` and `template`) for disambiguation.

**Problem:**

The compiler doesn't know if a dependent name is a type or a value:

```cpp
template <typename T>
void func() {
    T::value_type x;  // Is value_type a type or static member?
    // Compiler assumes it's a value!
}
```

**Solution 1: `typename` for Dependent Types:**

```cpp
template <typename T>
void func(const T& container) {
    // WHY: Tell compiler value_type is a TYPE
    typename T::value_type x = container[0];

    // WHY: iterator is also a dependent TYPE
    typename T::iterator it = container.begin();
}
```

**Solution 2: `template` for Dependent Templates:**

```cpp
template <typename T>
class Outer {
public:
    template <typename U>
    void inner(U value);
};

template <typename T>
void call_member(T& obj) {
    // WHY: Tell compiler inner is a TEMPLATE
    obj.template inner<int>(42);
    //     ^^^^^^^^ Required!
}
```

**When to Use:**

**Use `typename`:**

- Accessing type members through template parameters
- Any nested type that depends on `T`

```cpp
typename T::value_type        // Type from T
typename T::iterator          // Type from T
typename T::const_iterator    // Type from T
```

**Use `template`:**

- Calling member templates through dependent objects
- Accessing template members of template parameters

```cpp
obj.template func<int>()      // Member template
ptr->template func<int>()     // Through pointer
T::template nested<int>()     // Static member template
```

**Two-Phase Name Lookup:**

**Phase 1** (Template Definition): Non-dependent names resolved
**Phase 2** (Instantiation): Dependent names resolved

```cpp
void helper() { cout << "Global\n"; }

template <typename T>
class MyClass {
public:
    void func() {
        helper();  // Phase 1: Found global helper
    }

    void func2(T value) {
        process(value);  // Phase 2: Found when T is known
    }
};

void process(int x) { /* ... */ }
```

### Combined Example - All Three Together:

```cpp
#include <iostream>
#include <vector>
#include <memory>
using namespace std;

// 1. Alias Templates
template <typename T>
using UniqueVec = unique_ptr<vector<T>>;

// 2. Deduction Guide
template <typename T>
class Wrapper {
    T value;
public:
    Wrapper(T v) : value(v) {}
    T get() const { return value; }
};

// Convert C-string to string
Wrapper(const char*) -> Wrapper<string>;

// 3. Dependent Names
template <typename Container>
void print_container(const Container& c) {
    // WHY: typename needed - iterator depends on Container
    typename Container::const_iterator it;

    for (it = c.begin(); it != c.end(); ++it) {
        cout << *it << " ";
    }
    cout << endl;
}

int main() {
    // Alias template usage
    UniqueVec<int> vec = make_unique<vector<int>>(vector{1, 2, 3});

    // Deduction guide usage
    Wrapper w("Hello");  // Deduces Wrapper<string>
    cout << w.get() << endl;

    // Dependent names usage
    vector<int> data = {10, 20, 30};
    print_container(data);

    return 0;
}
```

**Key Improvements:**

1. **Alias Templates**: Reduce verbosity, create domain vocabulary
2. **Deduction Guides**: Eliminate redundant type specifications
3. **Dependent Names**: Enable correct generic programming with clear semantics

Together, these features make modern C++ template code more readable, maintainable, and less error-prone.

---