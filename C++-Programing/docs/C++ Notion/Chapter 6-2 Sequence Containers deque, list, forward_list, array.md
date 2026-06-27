# 6.2. Sequence Containers: deque, list, forward_list, array

---

## Table of Contents

1. deque - Double-Ended Queue
2. list - Doubly Linked List
3. forward_list - Singly Linked List
4. array - Fixed-Size Array Wrapper
5. Sequence Container Comparison
6. Summary
7. Interview Preparation

---

## 1. deque - Double-Ended Queue

### 1.1 What is deque?

**deque (pronounced "deck") is a sequence container that allows efficient insertion and deletion at both front and back.**

Header: `#include <deque>`

**Why deque Exists:**

vector is efficient at the back but slow at the front because inserting at the beginning requires shifting all elements:

```cpp
vector<int> v = {1, 2, 3, 4, 5};
v.push_back(6);        // O(1) - Fast!
v.insert(v.begin(), 0); // O(n) - Slow! Must shift all elements
```

deque solves this by providing O(1) operations at both ends:

```cpp
deque<int> d = {1, 2, 3, 4, 5};
d.push_back(6);   // O(1) - Fast!
d.push_front(0);  // O(1) - Also fast!
```

**Memory Structure:**

```
vector: Single contiguous block
┌───┬───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ 4 │ 5 │
└───┴───┴───┴───┴───┘
Must reallocate entire block to grow

deque: Multiple fixed-size chunks
┌─────┐     ┌─────┐     ┌─────┐
│ 1 │ 2│ ↔ │ 3 │ 4│ ↔ │ 5 │  │
└─────┘     └─────┘     └─────┘
Can add chunks at front or back
```

**Key Difference:** deque uses non-contiguous memory in chunks, allowing it to grow at both ends without moving existing elements.

### 1.2 Declaration and Basic Operations

```cpp
#include <iostream>
#include <deque>
using namespace std;

int main() {
    // WHY: Multiple ways to initialize deque

    // 1. Empty deque
    deque<int> d1;

    // 2. With size and default value
    deque<int> d2(5, 10);  // [10, 10, 10, 10, 10]

    // 3. Initializer list (C++11)
    deque<int> d3 = {1, 2, 3, 4, 5};

    // 4. Copy from another deque
    deque<int> d4(d3);

    // WHY: Efficient operations at both ends
    deque<int> d;

    // Add at back - O(1)
    d.push_back(30);
    d.push_back(40);

    // Add at front - O(1)
    d.push_front(20);
    d.push_front(10);

    cout << "Deque: ";
    for(int x : d) cout << x << " ";
    cout << endl;

    // Remove from both ends - O(1)
    d.pop_front();  // Remove 10
    d.pop_back();   // Remove 40

    cout << "After pop: ";
    for(int x : d) cout << x << " ";

    return 0;
}
```

**Output:**

```
Deque: 10 20 30 40
After pop: 20 30
```

### 1.3 Accessing Elements

```cpp
#include <iostream>
#include <deque>
using namespace std;

int main() {
    deque<int> d = {10, 20, 30, 40, 50};

    // WHY: Same access methods as vector

    // Random access - O(1) like vector
    cout << "d[0] = " << d[0] << endl;
    cout << "d.at(2) = " << d.at(2) << endl;

    // Access ends - O(1)
    cout << "Front: " << d.front() << endl;
    cout << "Back: " << d.back() << endl;

    // Iteration works same as vector
    cout << "All elements: ";
    for(int x : d) cout << x << " ";

    return 0;
}
```

**Output:**

```
d[0] = 10
d.at(2) = 30
Front: 10
Back: 50
All elements: 10 20 30 40 50
```

### 1.4 deque vs vector - Key Differences

```cpp
#include <iostream>
#include <deque>
#include <vector>
using namespace std;

int main() {
    deque<int> d;
    vector<int> v;

    // BOTH support: random access - O(1)
    d.push_back(10);
    v.push_back(10);
    cout << "d[0] = " << d[0] << endl;
    cout << "v[0] = " << v[0] << endl;

    // deque has: push_front/pop_front - O(1)
    d.push_front(5);
    d.pop_front();
    cout << "Deque after front ops: " << d.front() << endl;

    // vector doesn't have push_front/pop_front
    // Must use slow insert/erase
    v.insert(v.begin(), 5);  // O(n) - shifts all elements!
    v.erase(v.begin());      // O(n) - shifts all elements!

    // deque does NOT have:
    // d.data();      // ERROR! No contiguous memory
    // d.capacity();  // ERROR! No capacity concept
    // d.reserve();   // ERROR! Cannot pre-allocate

    // vector has all these:
    int* ptr = v.data();  // OK - contiguous memory
    size_t cap = v.capacity();  // OK
    v.reserve(100);  // OK

    return 0;
}
```

**Output:**

```
d[0] = 10
v[0] = 10
Deque after front ops: 10
```

**Comparison Table:**

| Feature | vector | deque |
| --- | --- | --- |
| Random access [ ] | O(1) ✅ | O(1) ✅ |
| push_back/pop_back | O(1) ✅ | O(1) ✅ |
| push_front/pop_front | ❌ O(n) | O(1) ✅ |
| Memory layout | Contiguous | Chunked |
| data() method | ✅ | ❌ |
| capacity()/reserve() | ✅ | ❌ |
| Iterator invalidation | More aggressive | Less for push/pop |

### 1.5 When to Use deque

**✅ Use deque When:**

- Need efficient insertion/deletion at both front and back
- Implementing queue (FIFO: push_back, pop_front)
- Implementing sliding window algorithms
- Random access required but vector's front operations too slow

**❌ Don't Use deque When:**

- Need contiguous memory for C APIs (use vector)
- Only adding at one end (use vector)
- Need iterator stability (use list)
- Memory fragmentation is concern

**Common Use Cases:**

```cpp
// 1. Queue implementation
deque<int> queue;
queue.push_back(1);      // Enqueue
int front = queue.front();
queue.pop_front();       // Dequeue

// 2. Sliding window
deque<int> window;
void addToWindow(int value, int maxSize) {
    window.push_back(value);
    if(window.size() > maxSize) {
        window.pop_front();  // Remove oldest - O(1)!
    }
}

// 3. Double-ended operations
deque<int> d;
d.push_back(1);
d.push_front(0);   // Efficient at both ends
```

---

## 2. list - Doubly Linked List

### 2.1 What is list?

**list is a sequence container that implements a doubly linked list data structure.**

Header: `#include <list>`

**Why list Exists:**

Both vector and deque have O(n) insertion/deletion in the middle because elements must shift:

```cpp
vector<int> v = {1, 2, 3, 4, 5};
v.insert(v.begin() + 2, 99);  // O(n) - shifts elements 3,4,5
```

list solves this with O(1) insertion/deletion anywhere if you have an iterator:

```cpp
list<int> l = {1, 2, 3, 4, 5};
auto it = next(l.begin(), 2);  // Get iterator to position
l.insert(it, 99);  // O(1) - just rewire pointers!
```

**Memory Structure:**

```
vector/deque: Elements in contiguous or chunked memory
┌───┬───┬───┬───┬───┐
│ 1 │ 2 │ 3 │ 4 │ 5 │
└───┴───┴───┴───┴───┘

list: Each element is a separate node
┌────────┐      ┌────────┐      ┌────────┐
│prev│1│next│ ↔ │prev│2│next│ ↔ │prev│3│next│ ↔ ...
└────────┘      └────────┘      └────────┘
Each node has pointers to previous and next
```

### 2.2 Basic Operations

```cpp
#include <iostream>
#include <list>
using namespace std;

int main() {
    // WHY: Multiple initialization methods
    list<int> l1;
    list<int> l2(5, 10);  // [10, 10, 10, 10, 10]
    list<int> l3 = {1, 2, 3, 4, 5};

    // WHY: Efficient at both ends
    list<int> l;

    l.push_back(30);
    l.push_back(40);
    l.push_front(20);
    l.push_front(10);

    cout << "List: ";
    for(int x : l) cout << x << " ";
    cout << endl;

    // Insert in middle - O(1) with iterator!
    auto it = next(l.begin(), 2);  // Get iterator to 3rd element
    l.insert(it, 25);

    cout << "After insert: ";
    for(int x : l) cout << x << " ";

    return 0;
}
```

**Output:**

```
List: 10 20 30 40
After insert: 10 20 25 30 40
```

### 2.3 No Random Access - Critical Limitation

```cpp
#include <iostream>
#include <list>
using namespace std;

int main() {
    list<int> l = {10, 20, 30, 40, 50};

    // WHY: Linked list cannot do O(1) random access
    // cout << l[2];    // ERROR! No operator[]
    // cout << l.at(2); // ERROR! No at()

    // Must use iterators - O(n) to reach position
    auto it = l.begin();
    advance(it, 2);  // Move iterator 2 positions - O(n)
    cout << "3rd element: " << *it << endl;

    // Helper function 'next' is cleaner
    cout << "3rd element: " << *next(l.begin(), 2) << endl;

    // Can access ends efficiently - O(1)
    cout << "First: " << l.front() << endl;
    cout << "Last: " << l.back() << endl;

    return 0;
}
```

**Output:**

```
3rd element: 30
3rd element: 30
First: 10
Last: 50
```

### 2.4 Special list Operations

**list has unique operations not available in vector/deque:**

```cpp
#include <iostream>
#include <list>
using namespace std;

int main() {
    // WHY: Operations unique to linked lists

    // 1. splice - Move elements from one list to another
    list<int> l1 = {1, 3, 5, 7};
    list<int> l2 = {2, 4, 6};

    auto it = next(l1.begin(), 2);  // Position after 3
    l1.splice(it, l2);  // Move ALL of l2 into l1 - O(1)!

    cout << "After splice - l1: ";
    for(int x : l1) cout << x << " ";
    cout << "\nAfter splice - l2 size: " << l2.size() << endl;

    // 2. merge - Merge two sorted lists
    list<int> l3 = {1, 3, 5};
    list<int> l4 = {2, 4, 6};
    l3.merge(l4);  // O(n) - merges sorted lists
    cout << "After merge: ";
    for(int x : l3) cout << x << " ";
    cout << endl;

    // 3. sort - Sorts the list
    list<int> l5 = {5, 2, 8, 1, 9};
    l5.sort();  // O(n log n) - uses merge sort
    cout << "After sort: ";
    for(int x : l5) cout << x << " ";
    cout << endl;

    // 4. reverse - Reverses the list
    l5.reverse();  // O(n) - just rewire pointers
    cout << "After reverse: ";
    for(int x : l5) cout << x << " ";
    cout << endl;

    // 5. remove - Removes all elements with value
    list<int> l6 = {1, 2, 3, 2, 4, 2, 5};
    l6.remove(2);  // Removes all 2's - O(n)
    cout << "After remove(2): ";
    for(int x : l6) cout << x << " ";
    cout << endl;

    // 6. unique - Removes consecutive duplicates
    list<int> l7 = {1, 1, 2, 2, 2, 3, 3, 4};
    l7.unique();  // Keeps only one of each consecutive group
    cout << "After unique: ";
    for(int x : l7) cout << x << " ";

    return 0;
}
```

**Output:**

```
After splice - l1: 1 3 2 4 6 5 7
After splice - l2 size: 0
After merge: 1 2 3 4 5 6
After sort: 1 2 5 8 9
After reverse: 9 8 5 2 1
After remove(2): 1 3 4 5
After unique: 1 2 3 4
```

### 2.5 Iterator Stability - Major Advantage

**list has the strongest iterator stability of all containers.**

```cpp
#include <iostream>
#include <list>
#include <vector>
using namespace std;

int main() {
    // WHY: list iterators remain valid after modifications

    list<int> l = {1, 2, 3, 4, 5};
    auto it_list = next(l.begin(), 2);  // Points to 3

    cout << "Before modifications - *it_list: " << *it_list << endl;

    // Modify list: insert at front, back, middle
    l.insert(l.begin(), 0);  // Insert at front
    l.push_back(6);          // Insert at back
    l.insert(next(l.begin(), 1), 99);  // Insert in middle

    cout << "After modifications - *it_list: " << *it_list << endl;  // Still 3!

    // Compare with vector - iterator invalidation
    vector<int> v = {1, 2, 3, 4, 5};
    auto it_vec = v.begin() + 2;  // Points to 3

    v.insert(v.begin(), 0);  // it_vec now INVALID!
    // cout << *it_vec;  // DANGER! Undefined behavior

    return 0;
}
```

**Output:**

```
Before modifications - *it_list: 3
After modifications - *it_list: 3
```

**Iterator Invalidation Rules:**

| Container | Insert | Erase | push_back | push_front |
| --- | --- | --- | --- | --- |
| **vector** | From insert point → end | From erase point → end | All if realloc | N/A |
| **deque** | All | All | End iterator | All |
| **list** | None | Only erased | None | None |

### 2.6 When to Use list

**✅ Use list When:**

- Frequent insertions/deletions in middle with known position
- Iterator stability is critical
- Need splice operations (moving elements between lists)
- Don't need random access

**❌ Don't Use list When:**

- Need random access by index
- Memory overhead is concern (2 pointers per node)
- Cache performance matters (nodes scattered in memory)
- Only inserting at ends (use deque or vector)

**Performance Characteristics:**

```cpp
list<int> l;

// Fast - O(1) if you have iterator
l.push_front(1);
l.push_back(2);
l.insert(it, 3);  // O(1) with iterator
l.erase(it);      // O(1) with iterator

// Slow - O(n)
// l[5];  // ERROR! No random access
auto it = next(l.begin(), 5);  // O(n) to reach position
```

---

## 3. forward_list - Singly Linked List

### 3.1 What is forward_list?

**forward_list is a sequence container that implements a singly linked list (C++11).**

Header: `#include <forward_list>`

**Why forward_list Exists:**

list uses doubly linked nodes with 2 pointers per node:

```
Doubly linked (list):
┌──────────┐
│prev│data│next│  ← 3 fields per node
└──────────┘
24 bytes on 64-bit system (2 pointers + data)
```

forward_list uses singly linked nodes with 1 pointer:

```
Singly linked (forward_list):
┌────────┐
│data│next│  ← 2 fields per node
└────────┘
16 bytes on 64-bit system (1 pointer + data)
```

**Memory Savings: ~33% less overhead per node**

### 3.2 Basic Operations

```cpp
#include <iostream>
#include <forward_list>
using namespace std;

int main() {
    forward_list<int> fl;

    // WHY: Memory-efficient when only traversing forward

    fl.push_front(30);
    fl.push_front(20);
    fl.push_front(10);

    cout << "Forward list: ";
    for(int x : fl) cout << x << " ";
    cout << endl;

    // Can only access front - O(1)
    cout << "Front: " << fl.front() << endl;

    // No back(), size(), push_back()
    // cout << fl.back();   // ERROR!
    // cout << fl.size();   // ERROR!

    return 0;
}
```

**Output:**

```
Forward list: 10 20 30
Front: 10
```

### 3.3 Limitations vs list

```cpp
#include <iostream>
#include <forward_list>
#include <list>
using namespace std;

int main() {
    forward_list<int> fl = {1, 2, 3, 4, 5};
    list<int> l = {1, 2, 3, 4, 5};

    // WHY: Singly linked = fewer operations

    // forward_list does NOT have:
    // fl.size();      // ERROR! No size() method
    // fl.push_back(); // ERROR! Only push_front
    // fl.back();      // ERROR! Only front()

    // list has all these:
    cout << "list size: " << l.size() << endl;
    l.push_back(6);
    cout << "list back: " << l.back() << endl;

    // Both have push_front:
    fl.push_front(0);
    l.push_front(0);

    return 0;
}
```

**Output:**

```
list size: 5
list back: 5
```

### 3.4 insert_after - Special Method

```cpp
#include <iostream>
#include <forward_list>
using namespace std;

int main() {
    forward_list<int> fl = {1, 2, 4, 5};

    // WHY: Singly linked can only insert AFTER a position

    auto it = fl.begin();  // Points to 1
    ++it;                  // Points to 2

    // insert_after (not insert!)
    fl.insert_after(it, 3);

    cout << "After insert_after: ";
    for(int x : fl) cout << x << " ";

    return 0;
}
```

**Output:**

```
After insert_after: 1 2 3 4 5
```

**Why insert_after instead of insert?**

```
In singly linked list:

Current: [1]→[2]→[4]
              ↑
             it

To insert 3 after 2:
1. Create [3]→[4]
2. Change [2]→[3]
Done! O(1)

To insert 3 BEFORE 2:
Need to find node pointing to 2 = O(n)!
```

### 3.5 forward_list vs list Comparison

| Feature | list | forward_list |
| --- | --- | --- |
| Memory per node | ~24 bytes | ~16 bytes |
| Traversal | Bidirectional | Forward only |
| size() | ✅ O(1) | ❌ |
| push_back() | ✅ O(1) | ❌ |
| back() | ✅ O(1) | ❌ |
| insert_after() | ❌ | ✅ |
| Use case | General linked list | Memory-constrained |

### 3.6 When to Use forward_list

**✅ Use forward_list When:**

- Memory is very constrained
- Only need forward traversal
- Don't need size() method
- Don't need back access

**❌ Use list Instead When:**

- Need bidirectional traversal
- Need size() method
- Need push_back/back()
- Memory overhead acceptable

---

## 4. array - Fixed-Size Array Wrapper

### 4.1 What is array?

**array is a container that wraps a fixed-size array with STL interface (C++11).**

Header: `#include <array>`

**Why array Exists:**

C-style arrays lose size information and have no bounds checking:

```cpp
int arr[5] = {1, 2, 3, 4, 5};
// In same scope: sizeof(arr)/sizeof(int) = 5 works

void func(int arr[]) {
    // sizeof(arr) = pointer size, NOT array size!
    // No bounds checking
    // arr[100] = 0;  // No error, undefined behavior!
}
```

std::array solves these problems:

```cpp
array<int, 5> arr = {1, 2, 3, 4, 5};
// Size is part of type
// arr.size() always works
// arr.at(100) throws exception
```

### 4.2 Declaration and Basic Usage

```cpp
#include <iostream>
#include <array>
#include <algorithm>
using namespace std;

int main() {
    // WHY: Fixed size but with STL benefits

    // Size MUST be compile-time constant
    array<int, 5> arr1 = {1, 2, 3, 4, 5};

    // Partial initialization - rest are 0
    array<int, 5> arr2 = {1, 2};  // {1, 2, 0, 0, 0}

    // All zeros
    array<int, 5> arr3 = {};

    // Access elements
    cout << "arr1[0] = " << arr1[0] << endl;
    cout << "arr1.at(2) = " << arr1.at(2) << endl;

    // Size is always known
    cout << "Size: " << arr1.size() << endl;

    // Can use STL algorithms
    sort(arr1.begin(), arr1.end());

    cout << "Sorted: ";
    for(int x : arr1) cout << x << " ";

    return 0;
}
```

**Output:**

```
arr1[0] = 1
arr1.at(2) = 3
Size: 5
Sorted: 1 2 3 4 5
```

### 4.3 array vs C-style Array

```cpp
#include <iostream>
#include <array>
#include <algorithm>
using namespace std;

int main() {
    // C-style array
    int c_arr[5] = {5, 2, 8, 1, 9};

    // STL array
    array<int, 5> stl_arr = {5, 2, 8, 1, 9};

    // Size information
    cout << "C-array size: " << sizeof(c_arr)/sizeof(int) << endl;
    cout << "STL array size: " << stl_arr.size() << endl;  // Always works!

    // Bounds checking
    // c_arr[10];  // No error, undefined behavior!
    try {
        stl_arr.at(10);  // Throws out_of_range exception
    } catch(const out_of_range& e) {
        cout << "Caught: " << e.what() << endl;
    }

    // Assignment
    // c_arr = {1, 2, 3, 4, 5};  // ERROR!
    stl_arr = {1, 2, 3, 4, 5};  // OK!

    // STL algorithms - cleaner syntax
    sort(c_arr, c_arr + 5);           // Works but error-prone
    sort(stl_arr.begin(), stl_arr.end());  // Clean and safe

    return 0;
}
```

**Output:**

```
C-array size: 5
STL array size: 5
Caught: array::at: __n (which is 10) >= _Nm (which is 5)
```

### 4.4 array vs vector

```cpp
#include <iostream>
#include <array>
#include <vector>
using namespace std;

int main() {
    // WHY: array for compile-time size, vector for runtime

    // array - Fixed at compile time
    array<int, 5> arr = {1, 2, 3, 4, 5};
    cout << "array size: " << arr.size() << endl;
    // arr.push_back(6);  // ERROR! Cannot grow
    // arr.resize(10);    // ERROR! Size is fixed

    // vector - Dynamic at runtime
    vector<int> vec = {1, 2, 3, 4, 5};
    cout << "vector size before: " << vec.size() << endl;
    vec.push_back(6);  // OK! Can grow
    vec.resize(10);    // OK! Can resize
    cout << "vector size after: " << vec.size() << endl;

    // Both support:
    // - Random access: arr[i], vec[i]
    // - Iterators: begin(), end()
    // - STL algorithms

    return 0;
}
```

**Output:**

```
array size: 5
vector size before: 5
vector size after: 10
```

**Comparison:**

| Feature | C-style array | std::array | std::vector |
| --- | --- | --- | --- |
| Size | Compile-time | Compile-time | Runtime |
| Can grow | ❌ | ❌ | ✅ |
| Knows size | ❌* | ✅ | ✅ |
| Bounds checking | ❌ | ✅ at() | ✅ at() |
| STL compatible | Partial | ✅ | ✅ |
| Assignment | ❌ | ✅ | ✅ |
| Memory | Stack | Stack | Heap |
- Only in declaring scope

### 4.5 When to Use array

**✅ Use array When:**

- Size known at compile time
- Fixed size is guaranteed
- Want stack allocation
- Need better performance than vector
- Want type safety over C arrays

**✅ Use vector When:**

- Size unknown at compile time
- Need dynamic growth
- Size changes frequently

**✅ Use C-style Array When:**

- Interfacing with C APIs
- Embedded systems with strict constraints
- Need exact control over memory

**Best Practice:**

```cpp
// ✅ GOOD: array for truly fixed-size data
array<int, 12> monthlyData;  // Months are always 12
array<string, 7> weekDays = {"Mon", "Tue", "Wed", ...};

// ✅ GOOD: vector for dynamic data
vector<int> userScores;  // Unknown number of users

// ❌ AVOID: vector for fixed-size wastes memory
vector<int> weekDays(7);  // Use array<int, 7> instead
```

---

## 5. Sequence Container Comparison

### 5.1 Comprehensive Comparison Table

| Feature | vector | deque | list | forward_list | array |
| --- | --- | --- | --- | --- | --- |
| **Header** | `<vector>` | `<deque>` | `<list>` | `<forward_list>` | `<array>` |
| **Structure** | Dynamic array | Chunked array | Doubly linked | Singly linked | Static array |
| **Memory** | Contiguous | Non-contiguous | Non-contiguous | Non-contiguous | Contiguous |
| **Size** | Dynamic | Dynamic | Dynamic | Dynamic | Fixed |
|  |  |  |  |  |  |
| **Random access [ ]** | ✅ O(1) | ✅ O(1) | ❌ | ❌ | ✅ O(1) |
| **push_back()** | ✅ O(1)* | ✅ O(1) | ✅ O(1) | ❌ | ❌ |
| **push_front()** | ❌ | ✅ O(1) | ✅ O(1) | ✅ O(1) | ❌ |
| **Insert middle** | O(n) | O(n) | ✅ O(1)** | ✅ O(1)** | ❌ |
|  |  |  |  |  |  |
| **pop_back()** | ✅ O(1) | ✅ O(1) | ✅ O(1) | ❌ | ❌ |
| **pop_front()** | ❌ | ✅ O(1) | ✅ O(1) | ✅ O(1) | ❌ |
| **Erase middle** | O(n) | O(n) | ✅ O(1)** | ✅ O(1)** | ❌ |
|  |  |  |  |  |  |
| **size()** | ✅ O(1) | ✅ O(1) | ✅ O(1) | ❌ | ✅ O(1) |
| **front()** | ✅ O(1) | ✅ O(1) | ✅ O(1) | ✅ O(1) | ✅ O(1) |
| **back()** | ✅ O(1) | ✅ O(1) | ✅ O(1) | ❌ | ✅ O(1) |
|  |  |  |  |  |  |
| **Iterator Type** | Random | Random | Bidirectional | Forward | Random |
| **Iterator Stability** | Weak | Weak | ✅ Strong | ✅ Strong | Strong |
| **Cache Performance** | ✅ Best | Good | Poor | Poor | ✅ Best |
| **Memory Overhead** | Low | Medium | High | Medium | None |
- Amortized

**If you have iterator

### 5.2 Selection Decision Tree

```
Which Container Should I Use?
│
├─ Size fixed at compile time?
│  YES → array
│
├─ Need frequent middle insert/delete?
│  YES → list or forward_list
│  │
│  ├─ Need backward traversal or size()?
│  │  YES → list
│  └─ NO → forward_list (saves memory)
│
├─ Need frequent front insert/delete?
│  YES → deque
│
└─ DEFAULT → vector (best general purpose)
```

### 5.3 Performance Scenarios

**Scenario 1: Add 1M elements at end**

```cpp
vector:      ✅ Fastest (contiguous, O(1) amortized)
deque:       ✅ Fast (chunked, O(1))
list:        Slower (node allocation overhead)
array:       ❌ Cannot grow
```

**Scenario 2: Add 1M elements at front**

```cpp
vector:      ❌ Very slow (O(n²) total)
deque:       ✅ Fast (O(1) per insert)
list:        ✅ Fast (O(1) per insert)
forward_list: ✅ Fastest (less memory per node)
array:       ❌ Cannot grow
```

**Scenario 3: Random access 1M times**

```cpp
vector:      ✅ Fastest (O(1), cache-friendly)
array:       ✅ Fastest (O(1), cache-friendly)
deque:       ✅ Fast (O(1), less cache-friendly)
list:        ❌ Impossible
forward_list: ❌ Impossible
```

**Scenario 4: Insert/delete in middle 1M times**

```cpp
vector:      ❌ Very slow (O(n) per operation)
deque:       ❌ Very slow (O(n) per operation)
list:        ✅ Fast with iterators (O(1) per operation)
forward_list: ✅ Fast with iterators (O(1), less memory)
array:       ❌ Cannot insert/delete
```

---

## Summary

### Key Takeaways

1. **deque provides O(1) operations at both ends** unlike vector which is slow at the front. Use deque for queue implementations (push_back to enqueue, pop_front to dequeue) or sliding windows where you need efficient front operations.
2. **deque uses chunked non-contiguous memory** which allows it to grow at both ends without moving elements, but means it doesn't have data(), capacity(), or reserve() methods like vector.
3. **list provides O(1) insertion/deletion anywhere** with an iterator, and has the strongest iterator stability. Insertions and deletions don't invalidate other iterators, only the erased elements.
4. **list has unique operations** like splice (move elements between lists in O(1)), merge (combine sorted lists), and built-in sort that work directly on the linked structure without requiring random access.
5. **list's main disadvantage is no random access** - you cannot use operator[] or at(). To access the nth element requires O(n) traversal using iterators or the advance/next helper functions.
6. **forward_list saves ~33% memory** compared to list by using singly linked nodes instead of doubly linked. Use it when memory is constrained and you only need forward traversal.
7. **forward_list lacks size(), back(), and push_back()** because these operations would be inefficient (O(n)) on a singly linked list. It uses insert_after instead of insert for the same reason.
8. **array wraps fixed-size arrays with STL benefits** - bounds checking with at(), size information that doesn't decay, assignment operator, and compatibility with STL algorithms. Size must be known at compile time.
9. **array vs vector trade-off is fixed vs dynamic** - array has zero overhead and stack allocation but cannot grow, while vector provides flexibility at the cost of heap allocation and small memory overhead for size/capacity tracking.
10. **Container choice impacts performance significantly** - vector is fastest for most use cases due to cache-friendly contiguous memory, but choosing the right container for your access pattern (deque for front operations, list for middle insertions) can provide order-of-magnitude speedups.

---

## Interview Preparation

### Q1: Compare deque and vector. When would you use deque instead of vector? What are the trade-offs?

**Answer:**

deque and vector both provide O(1) random access and O(1) push_back, but deque additionally provides O(1) push_front and pop_front which are O(n) in vector (requiring shifting all elements).

The key difference is memory structure: vector uses a single contiguous block of memory, while deque uses multiple fixed-size chunks linked together. This chunked structure allows deque to grow at both ends without moving existing elements.

Use deque instead of vector when you need efficient operations at the front. Classic use cases are queue implementations (FIFO: push_back to enqueue, pop_front to dequeue) and sliding window algorithms where you frequently remove from the front. For example, maintaining a window of the last N elements requires only O(1) operations with deque (push_back new element, pop_front oldest) versus O(n) with vector.

Trade-offs: deque has slightly worse cache performance than vector because elements aren't in one contiguous block. deque lacks data(), capacity(), and reserve() methods because its chunked structure doesn't have the same memory model as vector. deque also has more complex iterator invalidation rules - push_back/push_front can invalidate iterators but not references/pointers to elements.

Choose vector as default for its superior cache performance and simpler mental model. Only use deque when you actually need the O(1) front operations, which is less common than you might think - most algorithms work fine adding only at the back.

---

### Q2: Explain list's unique operations: splice, merge, sort. Why are these special to list?

**Answer:**

list has several operations that leverage its linked list structure in ways impossible for contiguous containers.

splice() moves elements from one list to another in O(1) time by simply rewiring pointers. For example, l1.splice(position, l2) moves all elements from l2 into l1 at the specified position. In vector, this would require copying all elements (O(n)). splice can also move single elements or ranges between lists with no element copying.

merge() combines two sorted lists into one sorted list in O(n) time. It works like the merge step in merge sort, comparing front elements and linking the smaller one into the result. This is special to list because it requires constant-time front removal and linking, which only doubly linked lists provide efficiently.

sort() sorts the list in O(n log n) using merge sort. You might ask why list has its own sort when std::sort exists - because std::sort requires random access iterators, which list doesn't have. list::sort is optimized for linked lists, sorting by relinking nodes rather than moving values, which can be faster for large objects.

These operations are special because they work directly on the list's pointer structure. splice doesn't copy elements, it just changes next/prev pointers. merge doesn't allocate new nodes, it relinks existing ones. sort doesn't need random access, it uses bottom-up merge sort designed for linked lists.

The common theme is O(1) insertion/removal anywhere - list can efficiently manipulate its structure in ways that would require O(n) element moves in contiguous containers. This makes list ideal for algorithms that frequently reorganize data, like maintaining sorted order with insertions, or implementing LRU caches where elements move between lists.

---

### Q3: When should you use forward_list over list? What are the limitations? Give a practical example.

**Answer:**

Use forward_list when memory is constrained and you only need forward traversal. forward_list uses singly linked nodes with one pointer per node versus list's doubly linked nodes with two pointers per node, saving approximately 33% memory overhead.

On a 64-bit system, each list node is ~24 bytes (8-byte prev pointer + 8-byte data + 8-byte next pointer) while each forward_list node is ~16 bytes (8-byte data + 8-byte next pointer). For a million integers, that's 8MB saved.

Limitations of forward_list:

- No size() method (would require O(n) traversal to count)
- No push_back() or back() (would require O(n) traversal to reach end)
- No bidirectional traversal (can't go backwards)
- Uses insert_after instead of insert (singly linked can only efficiently insert after a position)

Practical example: Implementing a hash table with chaining. Each bucket is a linked list of colliding elements. You only traverse buckets forward when searching, never backwards. You never need to know bucket size (just check if empty). You insert at the front (push_front) or after a found position (insert_after). Using forward_list instead of list for millions of hash table entries saves significant memory.

Another example: Memory-constrained embedded system maintaining a log of recent events. Events are added at the front (most recent) and the list is traversed forward to display or process them. Old events may be removed from anywhere if a size limit is reached. forward_list provides the needed functionality with minimal memory footprint.

When to use list instead: When you need size(), when you need to efficiently access or add at the back, when you need bidirectional iteration (reverse iteration or algorithms that move backwards), or when the memory savings aren't significant enough to justify the API limitations. For most applications, list's convenience outweighs forward_list's memory savings.

---

### Q4: Compare std::array with C-style arrays and std::vector. When should you use each?

**Answer:**

C-style arrays, std::array, and std::vector all store elements in contiguous memory with O(1) random access, but differ in size flexibility and features.

C-style arrays have compile-time fixed size and decay to pointers when passed to functions, losing size information. They have no bounds checking, no size() method (sizeof(arr)/sizeof(arr[0]) only works in declaring scope), and don't support STL operations like assignment or algorithms without manual pointer arithmetic.

std::array is a C++11 wrapper around fixed-size arrays that solves these problems. It maintains size information as part of its type (array<int, 5>), provides size() method, supports bounds-checked access with at(), works with STL algorithms using begin()/end(), and supports assignment. Size must still be known at compile time.

std::vector provides dynamic sizing - it can grow and shrink at runtime. It allocates on the heap rather than stack, has small overhead for tracking size and capacity, and provides push_back, resize, reserve, etc. The dynamic sizing comes with occasional reallocation costs.

Use C-style arrays only when interfacing with C APIs that expect raw arrays, or in embedded systems where you need exact control and cannot afford any overhead. In modern C++, prefer std::array.

Use std::array when size is fixed and known at compile time: days of week (always 7), months (always 12), fixed-size configuration data, or small lookup tables. Benefits are zero runtime overhead over C arrays, stack allocation (faster than heap), and type safety. Example: array<string, 12> monthNames.

Use std::vector when size is unknown, changes at runtime, or you need dynamic growth. This is the vast majority of cases: user input, database query results, growing collections, or any time you can't determine size at compile time.

Performance consideration: array and C-style arrays are stack-allocated (limited stack space, typically 1-8MB) while vector is heap-allocated (limited by system memory). For large datasets, you must use vector regardless of whether size is fixed, as stack allocation would overflow.

---

### Q5: You need to implement a queue, LRU cache, and a sorted list with frequent insertions. Which container would you use for each and why?

**Answer:**

**Queue (FIFO):** Use std::deque. Queue requires O(1) enqueue (add at back) and O(1) dequeue (remove from front). deque provides push_back and pop_front both in O(1), making it perfect. Implementation: deque<T> q; q.push_back(item) to enqueue, q.pop_front() to dequeue.

Don't use vector because removing from front requires shifting all elements (O(n)). Don't use list even though it has O(1) at both ends - deque has better cache performance for sequential access patterns typical in queue operations. The standard library's std::queue adapter is actually implemented using deque by default.

**LRU (Least Recently Used) Cache:** Use std::list combined with std::unordered_map. The cache needs to track access order and quickly move elements to the front on access. list provides O(1) splice to move accessed elements to the front without copying. The unordered_map stores key-to-list-iterator mappings for O(1) lookup.

Implementation: unordered_map<Key, list<Pair>::iterator> for O(1) lookup, list<Pair> for LRU order where each pair contains (key, value). On access: use map to find list iterator, splice that element to front (O(1)), update map. On eviction: remove from back of list (least recent) and from map.

Don't use vector or deque because moving accessed elements to the front requires O(n) shifting. Don't use forward_list because splice requires bidirectional linking. list is uniquely suited because of its O(1) splice and strong iterator stability.

**Sorted list with frequent insertions:** Use std::list with manual position finding, or std::set if you don't need duplicates. If you must maintain a sorted sequence with frequent insertions in the middle, list allows O(1) insertion once you find the position (total O(n) to find + insert).

Implementation with list: iterate to find insertion point (O(n)), insert at that position (O(1)). Total O(n) per insertion. Alternative using set (red-black tree): insert is O(log n), automatically maintains sorted order, but doesn't allow duplicates without using multiset.

Don't use vector/deque because insertion in middle is O(n) for the insertion itself plus O(n) to find position, and elements must shift. If insertions are rare and lookups common, vector might be better due to cache performance and binary search. The choice depends on the ratio of insertions to lookups and whether O(n) list traversal or O(log n) tree traversal is better for your dataset size.