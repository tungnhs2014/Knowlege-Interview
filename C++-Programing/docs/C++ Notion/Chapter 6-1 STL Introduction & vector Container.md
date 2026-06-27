# 6.1. STL Introduction & vector Container

---

## Table of Contents

1. What is STL?
2. STL Components
3. Container Fundamentals
4. vector - Dynamic Array Complete Guide
5. Summary
6. Interview Preparation

---

## 1. What is STL?

### 1.1 Definition

**The Standard Template Library (STL) is a collection of generic template classes and functions that provide efficient, reusable implementations of common data structures and algorithms.**

Designed by Alexander Stepanov and Meng Lee at Hewlett-Packard in the early 1990s, STL became part of the C++ Standard in 1998.

**Why STL Exists:**

Before STL, every C++ programmer had to implement their own data structures - linked lists, dynamic arrays, sorting algorithms. This was time-consuming, error-prone, and resulted in non-standard implementations across projects and teams.

STL solves this fundamental problem by providing professionally optimized, extensively tested implementations that work with any data type through templates. Instead of writing a linked list for integers, another for strings, another for custom objects, you write it once using templates and it works for everything.

**Real-World Impact:**

```cpp
// WITHOUT STL - You must implement everything
class IntVector {
    int* data;
    size_t size, capacity;
    // Need 100+ lines: constructors, destructor, copy, move,
    // resize logic, memory management, error handling...
};

class StringVector {
    string* data;
    size_t size, capacity;
    // Another 100+ lines for same logic, different type...
};

// WITH STL - Professional implementation in one line
#include <vector>
vector<int> intVec;     // Done!
vector<string> strVec;  // Done!
```

### 1.2 Benefits of Using STL

**Performance Benefits:**

STL containers and algorithms are heavily optimized, often at the assembly level. They outperform most hand-written implementations because they've been refined over decades by experts in algorithms and compiler optimization.

**Productivity Benefits:**

Development time decreases dramatically. Instead of spending weeks implementing and debugging data structures, you focus on solving actual business problems. Studies show 5-10x productivity improvement when using STL versus manual implementations.

**Safety Benefits:**

STL code has been tested in millions of applications worldwide. The bugs have been found and fixed. Your custom implementation will have bugs - STL's already been debugged.

**Portability Benefits:**

STL is part of the C++ Standard. Your code works on any platform with any conforming compiler - Windows, Linux, macOS, embedded systems. No third-party library dependencies.

**Maintainability Benefits:**

Every C++ programmer knows STL. When new developers join your team, they immediately understand `vector<int>` but must learn your custom `MyIntArray` class.

### 1.3 Big-O Complexity Notation Primer

Understanding algorithm efficiency is critical for choosing the right STL container and algorithm.

**Common Time Complexities:**

| Notation | Name | Example | What it Means |
| --- | --- | --- | --- |
| **O(1)** | Constant | array[i] | Same time regardless of data size |
| **O(log n)** | Logarithmic | binary_search | Halves problem each step |
| **O(n)** | Linear | find | Time proportional to size |
| **O(n log n)** | Linearithmic | sort | Best general-purpose sorting |
| **O(n²)** | Quadratic | Nested loops | Avoid for large data |

**Why This Matters - Real Example:**

```cpp
vector<int> v(1000000);  // 1 million elements

// O(1) - Constant time, always fast
int x = v[500000];  // ~1 nanosecond (same for any index)

// O(n) - Linear time, scales with size
int sum = 0;
for(int x : v) sum += x;  // ~1 millisecond for 1M elements

// O(n log n) - Efficient sorting
sort(v.begin(), v.end());  // ~20 milliseconds for 1M elements

// O(n²) - Quadratic, TOO SLOW for large data
// Nested loop over 1M elements = 1 trillion operations = ~15 minutes!
```

When choosing STL containers, always consider the operations you'll perform most frequently and their time complexities.

---

## 2. STL Components

### 2.1 The Four Pillars

STL consists of four main components that work together seamlessly:

**1. Containers** - Data structures that store collections of objects:

- Sequence: vector, deque, list, array, forward_list
- Associative: set, map, multiset, multimap
- Unordered: unordered_set, unordered_map
- Adapters: stack, queue, priority_queue

**2. Iterators** - Pointer-like objects that traverse containers:

- Provide uniform access across all containers
- Five categories: Input, Output, Forward, Bidirectional, Random Access
- Enable algorithms to work with any container

**3. Algorithms** - Generic functions that operate on containers:

- Sorting: sort, stable_sort, partial_sort
- Searching: find, binary_search, lower_bound
- Modifying: copy, transform, remove, unique
- Over 100 algorithms in `<algorithm>` and `<numeric>`

**4. Functors** - Objects that can be called like functions:

- Customize algorithm behavior
- Built-in: less, greater, plus, minus
- Custom comparators and predicates
- Lambdas (C++11)

### 2.2 How Components Work Together

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    // WHY: Components work together for powerful solutions

    // CONTAINER: Store data
    vector<int> numbers = {5, 2, 8, 1, 9, 3};

    // ITERATOR: Access data
    auto begin = numbers.begin();
    auto end = numbers.end();

    // ALGORITHM: Process data
    sort(begin, end);  // Uses iterators to sort container

    // FUNCTOR: Customize behavior
    sort(numbers.begin(), numbers.end(), greater<int>());  // Descending

    for(int x : numbers) cout << x << " ";

    return 0;
}
```

**Output:**

```
9 8 5 3 2 1
```

**Key Insight:** Iterators decouple algorithms from containers. The `sort` algorithm doesn't know about vector's internal structure - it just uses iterators. This means one algorithm works with all containers.

---

## 3. Container Fundamentals

### 3.1 Container Categories

STL containers are classified into four categories:

**1. Sequence Containers** - Elements in linear order:

- `vector` - Dynamic array (Part 6.1)
- `deque` - Double-ended queue (Part 6.2)
- `list` - Doubly linked list (Part 6.2)
- `forward_list` - Singly linked list (Part 6.2)
- `array` - Fixed-size array wrapper (Part 6.2)

**2. Associative Containers** - Sorted elements (Red-Black Tree):

- `set`, `multiset` - Unique/duplicate elements (Part 6.3)
- `map`, `multimap` - Key-value pairs (Part 6.3)

**3. Unordered Associative** - Hash table based:

- `unordered_set`, `unordered_map` (Part 6.4)

**4. Container Adapters** - Restricted interfaces:

- `stack`, `queue`, `priority_queue` (Part 6.3)

### 3.2 Common Container Operations

All sequence containers support these operations:

```cpp
#include <vector>
using namespace std;

int main() {
    vector<int> v = {1, 2, 3};

    // WHY: Standard interface across all containers
    bool empty = v.empty();      // Check if empty
    size_t sz = v.size();        // Number of elements
    v.clear();                   // Remove all elements

    auto first = v.begin();      // Iterator to start
    auto last = v.end();         // Iterator past end

    return 0;
}
```

### 3.3 Iterator Invalidation - Critical Concept

**Iterator invalidation** occurs when an iterator becomes unsafe to use after container modifications.

**Why This Matters:**

Using an invalidated iterator causes undefined behavior - crashes, data corruption, or silent bugs that are extremely difficult to debug.

```cpp
#include <vector>
using namespace std;

int main() {
    vector<int> v = {1, 2, 3, 4, 5};

    // WHY: Understanding invalidation prevents crashes
    auto it = v.begin();  // Points to first element

    v.push_back(6);  // MAY invalidate 'it' if reallocation occurs!

    // Accessing 'it' here is DANGEROUS - might crash
    // int x = *it;  // Undefined behavior if invalidated!

    // SAFE: Get fresh iterator after modification
    it = v.begin();
    int x = *it;  // OK

    return 0;
}
```

**Rule of Thumb:** Always refresh iterators after modifying containers, or use indices instead of iterators when making modifications.

We'll cover specific invalidation rules for each container in detail.

---

## 4. vector - Dynamic Array Complete Guide

### 4.1 What is vector?

**vector is a sequence container that implements a dynamic array - it automatically grows and shrinks as elements are added or removed.**

Header: `#include <vector>`

**Why vector Exists:**

C arrays have fixed size determined at compile time:

```cpp
int arr[100];  // Fixed size
// Too small? Can't grow
// Too large? Wastes memory
// Don't know size at compile time? Can't use
```

vector solves all these problems:

```cpp
vector<int> v;  // Starts with zero elements
v.push_back(1); // Grows automatically
v.push_back(2); // No size limit (until memory runs out)
// Efficient, safe, flexible
```

**Memory Layout:**

```
Traditional Array (Fixed):
┌───┬───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ 4 │ 5 │ (Size cannot change)
└───┴───┴───┴───┴───┘

vector (Dynamic):
┌───┬───┬───┐
│ 1 │ 2 │ 3 │ → Can grow → [1][2][3][4][5][6]...
└───┴───┴───┘
Contiguous memory, automatic resizing
```

### 4.2 Declaration and Initialization

```cpp
#include <iostream>
#include <vector>
using namespace std;

int main() {
    // WHY: Multiple initialization methods for different scenarios

    // 1. Empty vector - most common
    vector<int> v1;

    // 2. With size, default initialized (0 for int)
    vector<int> v2(5);  // [0, 0, 0, 0, 0]

    // 3. With size and initial value
    vector<int> v3(5, 10);  // [10, 10, 10, 10, 10]

    // 4. Initializer list (C++11) - very convenient
    vector<int> v4 = {1, 2, 3, 4, 5};
    vector<int> v5{1, 2, 3, 4, 5};  // Also valid

    // 5. Copy from another vector
    vector<int> v6(v4);  // Deep copy
    vector<int> v7 = v4; // Also deep copy

    // 6. From iterator range
    vector<int> v8(v4.begin(), v4.begin() + 3);  // [1, 2, 3]

    // 7. 2D vector (vector of vectors)
    vector<vector<int>> matrix(3, vector<int>(4, 0));  // 3x4 matrix of zeros

    cout << "v4: ";
    for(int x : v4) cout << x << " ";

    return 0;
}
```

**Output:**

```
v4: 1 2 3 4 5
```

### 4.3 Adding Elements

### **push_back() - Add at End (Most Common)**

```cpp
#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v;

    // WHY: push_back is O(1) amortized - very efficient
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);

    cout << "Size: " << v.size() << endl;
    cout << "Elements: ";
    for(int x : v) cout << x << " ";

    return 0;
}
```

**Output:**

```
Size: 3
Elements: 10 20 30
```

### **insert() - Add at Specific Position**

```cpp
#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v = {1, 2, 4, 5};

    // WHY: Insert when order matters, but O(n) due to shifting
    v.insert(v.begin() + 2, 3);  // Insert 3 at index 2

    cout << "After insert: ";
    for(int x : v) cout << x << " ";

    return 0;
}
```

**Output:**

```
After insert: 1 2 3 4 5
```

### **emplace_back() - Construct in Place (C++11)**

```cpp
#include <iostream>
#include <vector>
#include <string>
using namespace std;

struct Person {
    string name;
    int age;

    Person(string n, int a) : name(n), age(a) {
        cout << "Constructing " << name << endl;
    }
};

int main() {
    vector<Person> people;

    // WHY: emplace_back constructs object directly in vector
    // More efficient than push_back for complex objects
    people.emplace_back("Alice", 25);  // Constructs in place

    // push_back would create temp object then copy:
    people.push_back(Person("Bob", 30));  // Less efficient

    return 0;
}
```

**Output:**

```
Constructing Alice
Constructing Bob
```

### 4.4 Accessing Elements

```cpp
#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v = {10, 20, 30, 40, 50};

    // WHY: Different access methods for different needs

    // 1. operator[] - Fast, no bounds checking
    cout << "v[0] = " << v[0] << endl;  // O(1)

    // 2. at() - Safer, throws exception if out of bounds
    try {
        cout << "v.at(1) = " << v.at(1) << endl;  // O(1)
        // v.at(10);  // Would throw out_of_range exception
    } catch(const out_of_range& e) {
        cout << "Error: " << e.what() << endl;
    }

    // 3. front() - First element
    cout << "First: " << v.front() << endl;  // O(1), same as v[0]

    // 4. back() - Last element
    cout << "Last: " << v.back() << endl;  // O(1), same as v[v.size()-1]

    // 5. data() - Pointer to underlying array (C++11)
    int* ptr = v.data();
    cout << "Via pointer: " << ptr[2] << endl;  // Raw array access

    return 0;
}
```

**Output:**

```
v[0] = 10
v.at(1) = 20
First: 10
Last: 50
Via pointer: 30
```

**When to Use Each:**

- `operator[]`: Maximum performance, you're certain index is valid
- `at()`: Need bounds checking, can handle exceptions
- `front()/back()`: Semantic clarity, readable code
- `data()`: Interfacing with C APIs that expect raw arrays

### 4.5 Removing Elements

```cpp
#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v = {10, 20, 30, 40, 50};

    // WHY: Different removal methods for different scenarios

    // 1. pop_back() - Remove last element: O(1)
    v.pop_back();
    cout << "After pop_back: ";
    for(int x : v) cout << x << " ";
    cout << endl;

    // 2. erase() - Remove at position: O(n)
    v.erase(v.begin() + 1);  // Remove element at index 1 (value 20)
    cout << "After erase(1): ";
    for(int x : v) cout << x << " ";
    cout << endl;

    // 3. erase range
    v = {10, 20, 30, 40, 50};
    v.erase(v.begin() + 1, v.begin() + 4);  // Remove elements 20, 30, 40
    cout << "After erase range: ";
    for(int x : v) cout << x << " ";
    cout << endl;

    // 4. clear() - Remove all elements: O(n)
    v.clear();
    cout << "After clear, size: " << v.size() << endl;

    return 0;
}
```

**Output:**

```
After pop_back: 10 20 30 40
After erase(1): 10 30 40
After erase range: 10 50
After clear, size: 0
```

### 4.6 Size vs Capacity - THE CRITICAL CONCEPT

**This is the most important concept to master for vector.**

**Definitions:**

- **size()**: Number of elements currently stored
- **capacity()**: Total allocated memory space (may be larger than size)

**Why the Difference Exists:**

When vector grows, it doesn't allocate exactly the space needed. It allocates MORE to avoid frequent reallocations. This is the key to vector's O(1) amortized push_back.

```cpp
#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v;

    cout << "Initial - Size: " << v.size()
         << ", Capacity: " << v.capacity() << endl;

    // WHY: Watch capacity grow geometrically to avoid frequent reallocations
    for(int i = 1; i <= 10; i++) {
        v.push_back(i);
        cout << "After push " << i
             << " - Size: " << v.size()
             << ", Capacity: " << v.capacity() << endl;
    }

    return 0;
}
```

**Output (typical implementation):**

```
Initial - Size: 0, Capacity: 0
After push 1 - Size: 1, Capacity: 1
After push 2 - Size: 2, Capacity: 2
After push 3 - Size: 3, Capacity: 4    ← Doubled!
After push 4 - Size: 4, Capacity: 4
After push 5 - Size: 5, Capacity: 8    ← Doubled again!
After push 6 - Size: 6, Capacity: 8
After push 7 - Size: 7, Capacity: 8
After push 8 - Size: 8, Capacity: 8
After push 9 - Size: 9, Capacity: 16   ← Doubled again!
After push 10 - Size: 10, Capacity: 16
```

**Growth Strategy:**

```
Capacity doubles when size exceeds capacity:

Size  → Capacity
1     → 1
2     → 2
3     → 4  (doubled from 2)
5     → 8  (doubled from 4)
9     → 16 (doubled from 8)
17    → 32 (doubled from 16)

WHY: Ensures push_back is O(1) amortized, not O(n)
```

**Amortized Analysis:**

Without doubling, if capacity grew by 1 each time:

- Inserting n elements requires copying: 1 + 2 + 3 + ... + n = n²/2 elements
- Total: O(n²) for n insertions

With doubling:

- Copying happens at sizes: 1, 2, 4, 8, 16, ..., n
- Total copies: 1 + 2 + 4 + 8 + ... + n ≈ 2n
- Total: O(n) for n insertions → O(1) amortized per insertion

### 4.7 reserve() vs resize() - Often Confused

```cpp
#include <iostream>
#include <vector>
using namespace std;

int main() {
    // WHY: reserve() allocates memory, resize() creates elements

    // reserve() - Pre-allocate capacity
    vector<int> v1;
    v1.reserve(100);
    cout << "After reserve(100):" << endl;
    cout << "  Size: " << v1.size() << endl;        // 0 - no elements created
    cout << "  Capacity: " << v1.capacity() << endl; // 100
    // v1[0] = 5;  // ERROR! No elements exist yet

    // resize() - Change actual size, create elements
    vector<int> v2;
    v2.resize(100);
    cout << "\nAfter resize(100):" << endl;
    cout << "  Size: " << v2.size() << endl;        // 100 - elements created
    cout << "  Capacity: " << v2.capacity() << endl; // 100
    v2[0] = 5;  // OK - element exists
    cout << "  v2[0] = " << v2[0] << endl;
    cout << "  v2[50] = " << v2[50] << endl;        // 0 (default constructed)

    // resize() with value
    vector<int> v3;
    v3.resize(5, 42);  // Create 5 elements, all initialized to 42
    cout << "\nAfter resize(5, 42): ";
    for(int x : v3) cout << x << " ";

    return 0;
}
```

**Output:**

```
After reserve(100):
  Size: 0
  Capacity: 100

After resize(100):
  Size: 100
  Capacity: 100
  v2[0] = 5
  v2[50] = 0

After resize(5, 42): 42 42 42 42 42
```

**Comparison Table:**

| Operation | Changes Size? | Creates Elements? | Use When |
| --- | --- | --- | --- |
| **reserve(n)** | No | No | Know final size, will use push_back |
| **resize(n)** | Yes | Yes | Need elements now, can use v[i] |

**Best Practice Example:**

```cpp
// ✅ GOOD: Know size in advance, use reserve
vector<int> v;
v.reserve(1000);  // One allocation
for(int i = 0; i < 1000; i++) {
    v.push_back(i);  // No reallocations!
}

// ✅ ALSO GOOD: Need elements immediately, use resize
vector<int> scores(100);  // Create 100 elements
for(int i = 0; i < 100; i++) {
    scores[i] = getScore(i);  // Direct assignment
}

// ❌ BAD: Frequent reallocations
vector<int> bad;
for(int i = 0; i < 1000000; i++) {
    bad.push_back(i);  // Many reallocations!
}
```

### 4.8 Iterators and Traversal

```cpp
#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v = {10, 20, 30, 40, 50};

    // WHY: Multiple iteration styles for different scenarios

    // 1. Range-based for (C++11) - Simplest, most readable
    cout << "Range-for: ";
    for(int x : v) cout << x << " ";
    cout << endl;

    // 2. Iterator - Standard STL way
    cout << "Iterator: ";
    for(vector<int>::iterator it = v.begin(); it != v.end(); ++it) {
        cout << *it << " ";
    }
    cout << endl;

    // 3. Auto keyword (C++11) - Clean and concise
    cout << "Auto iterator: ";
    for(auto it = v.begin(); it != v.end(); ++it) {
        cout << *it << " ";
    }
    cout << endl;

    // 4. Index-based - When you need index
    cout << "Index-based: ";
    for(size_t i = 0; i < v.size(); i++) {
        cout << v[i] << " ";
    }
    cout << endl;

    // 5. Reverse iterator - Backward traversal
    cout << "Reverse: ";
    for(auto it = v.rbegin(); it != v.rend(); ++it) {
        cout << *it << " ";
    }
    cout << endl;

    // 6. Const iterator - Read-only access
    const vector<int>& cv = v;
    cout << "Const iterator: ";
    for(auto it = cv.cbegin(); it != cv.cend(); ++it) {
        cout << *it << " ";
        // *it = 100;  // ERROR! Cannot modify through const_iterator
    }

    return 0;
}
```

**Output:**

```
Range-for: 10 20 30 40 50
Iterator: 10 20 30 40 50
Auto iterator: 10 20 30 40 50
Index-based: 10 20 30 40 50
Reverse: 50 40 30 20 10
Const iterator: 10 20 30 40 50
```

### 4.9 Iterator Invalidation Rules for vector

**Critical for avoiding crashes and undefined behavior.**

```cpp
#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v = {1, 2, 3, 4, 5};

    // CASE 1: push_back invalidation
    auto it1 = v.begin();
    cout << "Before push_back: " << *it1 << endl;

    v.push_back(6);  // May invalidate if reallocation occurs
    // cout << *it1;  // DANGER! Might be invalid

    it1 = v.begin();  // Refresh iterator - SAFE
    cout << "After refresh: " << *it1 << endl;

    // CASE 2: insert invalidation
    v = {1, 2, 3, 4, 5};
    auto it2 = v.begin() + 3;  // Points to 4
    cout << "Before insert: " << *it2 << endl;

    v.insert(v.begin() + 1, 99);  // Insert 99 at index 1
    // cout << *it2;  // INVALID! Elements shifted

    // CASE 3: erase invalidation
    v = {1, 2, 3, 4, 5};
    auto it3 = v.begin() + 3;  // Points to 4
    cout << "Before erase: " << *it3 << endl;

    v.erase(v.begin() + 1);  // Erase element at index 1
    // cout << *it3;  // INVALID! Elements shifted

    // SAFE PATTERN: Use return value of erase
    v = {1, 2, 3, 4, 5};
    auto it = v.begin();
    while(it != v.end()) {
        if(*it % 2 == 0) {
            it = v.erase(it);  // erase returns next valid iterator
        } else {
            ++it;
        }
    }
    cout << "After safe erase: ";
    for(int x : v) cout << x << " ";

    return 0;
}
```

**Output:**

```
Before push_back: 1
After refresh: 1
Before insert: 4
Before erase: 4
After safe erase: 1 3 5
```

**Invalidation Rules Summary:**

| Operation | Invalidates Iterators |
| --- | --- |
| **push_back/emplace_back** | All if reallocation; none otherwise |
| **insert/emplace** | From insertion point to end |
| **erase** | From erase point to end |
| **pop_back** | end() and to erased element |
| **clear** | All |
| **resize** | All if reallocation |
| **reserve** | All if reallocation |

**Best Practices:**

```cpp
// ✅ GOOD: Refresh iterators after modification
auto it = v.begin();
v.push_back(x);
it = v.begin();  // Safe

// ✅ GOOD: Use return value of erase
it = v.erase(it);

// ✅ GOOD: Use indices instead of iterators when modifying
for(size_t i = 0; i < v.size(); ) {
    if(shouldRemove(v[i])) {
        v.erase(v.begin() + i);  // Don't increment i
    } else {
        ++i;
    }
}

// ❌ BAD: Using iterator after modification
auto it = v.begin();
v.push_back(x);
cout << *it;  // DANGER!
```

### 4.10 Performance Characteristics

```cpp
// vector Performance Summary

vector<int> v;

// ✅ FAST OPERATIONS - O(1)
v.push_back(10);        // O(1) amortized
v.pop_back();           // O(1)
int x = v[5];           // O(1) - random access
int first = v.front();  // O(1)
int last = v.back();    // O(1)
size_t sz = v.size();   // O(1)

// ⚠️ SLOW OPERATIONS - O(n)
v.insert(v.begin(), 1);        // O(n) - shifts all elements
v.erase(v.begin());            // O(n) - shifts all elements
auto it = find(v.begin(), v.end(), 42);  // O(n) - linear search
```

**Performance Tips:**

1. **Use reserve() when size is known:**

```cpp
vector<int> v;
v.reserve(1000000);  // Avoid reallocations
```

1. **Add at end, not beginning:**

```cpp
// ✅ FAST
v.push_back(x);  // O(1)

// ❌ SLOW
v.insert(v.begin(), x);  // O(n)
```

1. **Pass by const reference:**

```cpp
// ✅ GOOD
void process(const vector<int>& v) { }  // No copy

// ❌ BAD
void process(vector<int> v) { }  // Copies entire vector!
```

1. **Use move semantics (C++11):**

```cpp
vector<int> v1 = getLargeVector();
vector<int> v2 = move(v1);  // Move, not copy
```

### 4.11 When to Use vector

**✅ Use vector When:**

- Need random access (O(1) indexing)
- Mostly adding/removing at end
- Default choice for sequence container (90% of cases)
- Need contiguous memory (for C APIs)
- Cache performance matters

**❌ Don't Use vector When:**

- Frequent insertions/deletions in middle
- Frequent insertions at front
- Cannot tolerate reallocation pauses (hard real-time systems)

**Real-World Use Cases:**

```cpp
// 1. Storing collections
vector<string> names;
vector<int> scores;

// 2. Dynamic arrays
vector<double> measurements;
measurements.reserve(10000);

// 3. 2D grids/matrices
vector<vector<int>> grid(rows, vector<int>(cols));

// 4. Buffers
vector<char> buffer(1024);
buffer.resize(actualSize);

// 5. Growing lists
vector<Task> tasks;
tasks.push_back(newTask);
```

---

## Summary

### Key Takeaways

1. **STL eliminates reinventing the wheel** by providing professionally optimized, tested implementations of common data structures and algorithms. Development time decreases by 5-10x compared to manual implementations.
2. **The four STL components work together seamlessly**: Containers store data, Iterators access data, Algorithms process data, and Functors customize behavior. This separation enables one algorithm to work with all containers.
3. **Big-O complexity is critical for performance**. O(1) is constant, O(log n) logarithmic, O(n) linear, O(n log n) linearithmic. Always consider which operations you'll perform most frequently when choosing containers.
4. **vector is the default sequence container** - use it unless you have a specific reason not to. It provides O(1) random access, O(1) amortized push_back, and excellent cache performance due to contiguous memory.
5. **Size vs Capacity is vector's most important concept**. Size is current elements, capacity is allocated space. Vector grows geometrically (typically doubling) to achieve O(1) amortized push_back instead of O(n).
6. **reserve() pre-allocates memory without creating elements** - use it when you know final size to avoid reallocations. resize() actually creates elements - use it when you need elements immediately. These are fundamentally different operations.
7. **Iterator invalidation causes undefined behavior** if not handled correctly. For vector: push_back invalidates all iterators if reallocation occurs, insert/erase invalidate from modification point to end. Always refresh iterators after modifications.
8. **Multiple iteration styles serve different purposes**: Range-based for is simplest, iterators are standard STL, indices are best when modifying during iteration. Choose based on your needs.
9. **Performance matters in production code**. push_back is O(1) amortized but insert at beginning is O(n). Use reserve() for known sizes, pass vectors by const reference to avoid copies, and leverage move semantics in C++11+.
10. **Best practices prevent bugs**: Always use reserve() when size is known, refresh iterators after modifications, use at() for bounds checking during development, prefer emplace_back for complex objects, and default to vector unless you have a proven need for another container.

---

## Interview Preparation

### Q1: What is STL? Why was it created? Explain its main components and how they work together.

**Answer:**

STL is the Standard Template Library, a collection of generic template classes and functions providing efficient implementations of common data structures and algorithms. Created by Alexander Stepanov and Meng Lee at HP in the early 1990s, it became part of C++ Standard in 1998.

Before STL, every programmer reimplemented basic data structures like linked lists and dynamic arrays for each project and each type. This was time-consuming, error-prone, and resulted in non-standard implementations. STL solved this by providing professionally optimized, extensively tested implementations that work with any type through templates. This eliminated code duplication and improved productivity by 5-10x.

The four main components work together: (1) Containers store data (vector, list, map, set, etc.), (2) Iterators provide uniform access to container elements through pointer-like objects, (3) Algorithms process data using iterators (sort, find, transform), (4) Functors customize algorithm behavior through callable objects.

The key innovation is that iterators decouple algorithms from containers. A sort algorithm doesn't know about vector's internal structure - it just uses iterators. This means one algorithm works with all containers, and one container works with all algorithms. For example, sort() works identically on vector, deque, or array because they all provide random access iterators.

Benefits include type safety through templates, excellent performance from expert optimization, portability across all platforms, code reusability, and standard interfaces familiar to all C++ developers.

---

### Q2: Explain size vs capacity in vector. What happens during reallocation? Why does vector grow geometrically?

**Answer:**

In vector, size is the number of elements currently stored, while capacity is the total allocated memory space. Capacity is always greater than or equal to size. This separation exists because vector pre-allocates extra space to avoid frequent reallocations when growing.

When size exceeds capacity during push_back, reallocation occurs: (1) vector allocates a new larger array (typically doubling capacity), (2) all existing elements are copied/moved to the new array, (3) old array is deallocated. This reallocation invalidates all iterators, references, and pointers to elements.

Vector grows geometrically (doubling) to achieve O(1) amortized time for push_back. Here's why: If vector grew by a constant amount (like +10), inserting n elements would require n reallocations, each copying all existing elements. Total copies would be 1 + 2 + 3 + ... + n = n²/2, giving O(n²) total time.

With geometric growth (doubling), reallocations happen at sizes 1, 2, 4, 8, 16, ..., n. Total copies are 1 + 2 + 4 + 8 + ... + n ≈ 2n, giving O(n) total time for n insertions. This means O(1) amortized per insertion.

Example: Growing from capacity 0 to 1000 with doubling requires only ~10 reallocations (1→2→4→8→16→32→64→128→256→512→1024), copying ~2000 total elements. With constant growth of +10, it would require ~100 reallocations, copying ~50,000 total elements.

This is why reserve() is crucial when you know the final size - it eliminates all reallocations, turning O(n) with reallocations into O(n) without, and avoiding iterator invalidation.

---

### Q3: Compare reserve() and resize(). When would you use each? Include code examples.

**Answer:**

reserve(n) and resize(n) are fundamentally different operations that beginners often confuse.

reserve(n) pre-allocates capacity for n elements WITHOUT creating them. It changes capacity but not size. No elements are constructed. After reserve(100), size is still 0 but capacity is 100. You cannot access elements with operator[] - they don't exist yet. Use reserve when you know you'll add n elements with push_back and want to avoid reallocations.

resize(n) changes the actual number of elements. It changes both size and capacity. Elements are default-constructed (0 for int). After resize(100), both size and capacity are 100, and you can immediately use operator[] to access all elements. Use resize when you need elements to exist now.

Example use cases:

```cpp
// Use reserve: Know final size, will use push_back
vector<int> v1;
v1.reserve(1000);  // One allocation, size still 0
for(int i = 0; i < 1000; i++) {
    v1.push_back(i);  // No reallocations occur
}

// Use resize: Need elements immediately
vector<int> scores(100);  // Creates 100 elements initialized to 0
for(int i = 0; i < 100; i++) {
    scores[i] = calculateScore(i);  // Direct assignment works
}

// Use resize with value: Initialize all elements
vector<int> buffer;
buffer.resize(1024, -1);  // Create 1024 elements, all set to -1
```

Performance difference: If you're adding 1 million elements, calling reserve(1000000) then 1 million push_backs requires one allocation. Without reserve, push_back causes ~20 reallocations (due to doubling), each copying all existing elements - significantly slower for large vectors.

Common mistake: Calling reserve then using operator[] instead of push_back. reserve doesn't create elements, so accessing them is undefined behavior.

---

### Q4: Explain iterator invalidation in vector. What are the rules? How do you safely modify vector during iteration?

**Answer:**

Iterator invalidation occurs when an iterator becomes unsafe to use after container modifications. Using invalidated iterators causes undefined behavior - crashes, data corruption, or silent bugs.

Vector invalidation rules:

push_back/emplace_back: Invalidates all iterators IF reallocation occurs (capacity increases). If no reallocation, iterators remain valid. You can't know in advance, so assume invalidation.

insert/emplace: Invalidates iterators from the insertion point to end, even without reallocation, because elements shift right. Elements before insertion point are safe.

erase: Invalidates iterators from the erase point to end because elements shift left. Elements before erase point are safe.

reserve/resize/clear: Invalidate all iterators if reallocation occurs.

Why this matters: Consider removing even numbers from a vector. This common pattern is buggy:

```cpp
// ❌ WRONG - iterator invalidated after erase
for(auto it = v.begin(); it != v.end(); ++it) {
    if(*it % 2 == 0) {
        v.erase(it);  // Invalidates it!
        // ++it in next loop iteration uses invalid iterator - crash!
    }
}
```

Safe patterns:

```cpp
// ✅ Pattern 1: Use erase's return value
for(auto it = v.begin(); it != v.end(); ) {
    if(*it % 2 == 0) {
        it = v.erase(it);  // erase returns next valid iterator
    } else {
        ++it;
    }
}

// ✅ Pattern 2: Use indices, not iterators
for(size_t i = 0; i < v.size(); ) {
    if(v[i] % 2 == 0) {
        v.erase(v.begin() + i);  // Don't increment i
    } else {
        ++i;
    }
}

// ✅ Pattern 3: Remove-erase idiom (most efficient)
v.erase(remove_if(v.begin(), v.end(),
        [](int x) { return x % 2 == 0; }),
        v.end());
```

Best practice: Refresh iterators after any modification, or use algorithms like remove_if that handle invalidation correctly.

---

### Q5: When should you use vector? When should you NOT use vector? Compare its performance characteristics.

**Answer:**

Use vector as the default sequence container - it's the right choice in 90% of cases. Vector provides O(1) random access through operator[], O(1) amortized push_back at the end, and excellent cache performance due to contiguous memory layout.

Use vector when:

- You need random access (accessing elements by index)
- Most operations are adding/removing at the end
- You want the best cache performance (critical for modern CPUs)
- You need contiguous memory for C API compatibility
- Default choice unless you have a specific reason otherwise

Don't use vector when:

- Frequent insertions/deletions in the middle (O(n) due to shifting)
- Frequent insertions at the front (O(n) for push_front simulation)
- Cannot tolerate occasional reallocation pauses in hard real-time systems
- Need guaranteed iterator stability across all modifications

Performance characteristics:

Fast operations (O(1)):

- Random access: v[i], v.at(i)
- Add/remove at end: push_back, pop_back
- Access ends: front(), back()
- Query: size(), empty(), capacity()

Slow operations (O(n)):

- Insert/delete at front or middle (must shift elements)
- Search: find() requires linear scan
- Reallocation when capacity exceeded

Comparison with alternatives:

- vs deque: vector faster for random access and end operations, but deque provides O(1) push_front
- vs list: vector much faster for access and iteration (cache-friendly), but list provides O(1) insert/delete anywhere with iterator
- vs array: vector is dynamic vs array's fixed size, minimal overhead difference

Best practices: Use reserve() when final size is known to avoid reallocations. Pass by const reference to avoid copying. Use move semantics for large vectors. Prefer push_back over insert at middle. Consider deque if you need frequent front operations, or list if you need frequent middle operations, but start with vector and only change if profiling shows it's a bottleneck.