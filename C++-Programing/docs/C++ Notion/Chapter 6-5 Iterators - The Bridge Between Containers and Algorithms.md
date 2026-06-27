# 6.5. Iterators - The Bridge Between Containers and Algorithms

---

## Table of Contents

1. Iterator Fundamentals
2. Iterator Categories
3. Iterator Operations
4. Iterator Adapters
5. Iterator Invalidation Summary
6. Summary
7. Interview Preparation

---

## 1. Iterator Fundamentals

### 1.1 What are Iterators?

**Iterators are objects that point to elements in containers and allow traversal through the container.**

**Why Iterators Exist:**

Without iterators, every algorithm would need to know the internal structure of every container. Iterators provide a uniform interface so one algorithm works with all containers.

```cpp
// WITHOUT iterators - need different code for each container
void print_vector(vector<int>& v) {
    for(size_t i = 0; i < v.size(); i++) {
        cout << v[i] << " ";
    }
}

void print_list(list<int>& l) {
    // Can't use index! Need different approach
    for(auto node = l.begin(); /* ... */) { }
}

// WITH iterators - same code for all containers
template<typename Iterator>
void print_range(Iterator begin, Iterator end) {
    for(auto it = begin; it != end; ++it) {
        cout << *it << " ";
    }
}
// Works with vector, list, deque, set, map, array, etc.
```

### 1.2 Iterator as Generalized Pointers

**Think of iterators as smart pointers that know how to traverse their container.**

```cpp
#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v = {10, 20, 30, 40, 50};

    // WHY: Iterators behave like pointers

    // Pointer to array
    int arr[] = {10, 20, 30, 40, 50};
    int* ptr = arr;
    cout << *ptr << endl;        // 10 - dereference
    cout << *(ptr + 2) << endl;  // 30 - pointer arithmetic

    // Iterator to vector (similar syntax)
    auto it = v.begin();
    cout << *it << endl;         // 10 - dereference
    cout << *(it + 2) << endl;   // 30 - iterator arithmetic

    return 0;
}
```

**Output:**

```
10
30
10
30
```

### 1.3 Basic Iterator Operations

```cpp
#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v = {10, 20, 30, 40, 50};

    // WHY: Core iterator operations work uniformly

    // Get iterators to begin and end
    auto begin = v.begin();  // Points to first element
    auto end = v.end();      // Points PAST last element

    // Dereference - access element
    cout << "First: " << *begin << endl;

    // Increment - move to next
    ++begin;
    cout << "Second: " << *begin << endl;

    // Comparison - check if equal
    if(begin != end) {
        cout << "Not at end yet" << endl;
    }

    // Iterate through container
    cout << "All elements: ";
    for(auto it = v.begin(); it != v.end(); ++it) {
        cout << *it << " ";
    }

    return 0;
}
```

**Output:**

```
First: 10
Second: 20
Not at end yet
All elements: 10 20 30 40 50
```

---

## 2. Iterator Categories

### 2.1 The Five Iterator Categories

STL defines a hierarchy of iterator capabilities:

```
Iterator Hierarchy (least → most capable):

Input Iterator ────┐
                   ├──→ Forward Iterator ──→ Bidirectional Iterator ──→ Random Access Iterator
Output Iterator ───┘

Each level adds capabilities
```

**Why Categories Matter:**

Algorithms require specific iterator capabilities. `sort` needs random access, `find` only needs input iteration.

### 2.2 Input Iterator

**Input Iterator: Read-only, single-pass, forward-only**

Operations: `*it`, `++it`, `it++`, `==`, `!=`

```cpp
#include <iostream>
#include <iterator>
#include <sstream>
using namespace std;

int main() {
    // WHY: Input iterator for reading stream
    istringstream iss("10 20 30 40 50");

    // istream_iterator is an input iterator
    istream_iterator<int> begin(iss);
    istream_iterator<int> end;  // Default = end of stream

    cout << "Stream contents: ";
    for(auto it = begin; it != end; ++it) {
        cout << *it << " ";
    }

    // CANNOT go backwards
    // --it;  // ERROR! Input iterators are forward-only

    // CANNOT multi-pass
    // Once you've read it, you can't read again

    return 0;
}
```

**Output:**

```
Stream contents: 10 20 30 40 50
```

### 2.3 Output Iterator

**Output Iterator: Write-only, single-pass**

Operations: `*it = value`, `++it`, `it++`

```cpp
#include <iostream>
#include <iterator>
#include <vector>
using namespace std;

int main() {
    vector<int> v = {10, 20, 30};

    // WHY: Output iterator for writing to stream
    ostream_iterator<int> out(cout, " ");

    // Write to output
    for(int x : v) {
        *out = x;  // Write-only operation
        ++out;
    }

    return 0;
}
```

**Output:**

```
10 20 30
```

### 2.4 Forward Iterator

**Forward Iterator: Read/write, multi-pass, forward-only**

Operations: Input + Output + multi-pass guarantee

```cpp
#include <iostream>
#include <forward_list>
using namespace std;

int main() {
    forward_list<int> fl = {10, 20, 30, 40, 50};

    // WHY: forward_list provides forward iterators
    auto it = fl.begin();

    // Read
    cout << "First: " << *it << endl;

    // Write
    *it = 15;
    cout << "Modified first: " << *it << endl;

    // Multi-pass - can save iterator and reuse
    auto saved = fl.begin();
    ++it;
    ++it;
    cout << "Third: " << *it << endl;
    cout << "Saved still points to first: " << *saved << endl;

    // CANNOT go backwards
    // --it;  // ERROR! Forward iterators are forward-only

    return 0;
}
```

**Output:**

```
First: 10
Modified first: 15
Third: 30
Saved still points to first: 15
```

### 2.5 Bidirectional Iterator

**Bidirectional Iterator: All forward capabilities + backward movement**

Operations: Forward + `--it`, `it--`

```cpp
#include <iostream>
#include <list>
#include <set>
using namespace std;

int main() {
    list<int> l = {10, 20, 30, 40, 50};

    // WHY: list provides bidirectional iterators
    auto it = l.begin();

    // Forward
    ++it;
    ++it;
    cout << "After 2 increments: " << *it << endl;  // 30

    // Backward
    --it;
    cout << "After 1 decrement: " << *it << endl;   // 20

    // Reverse iteration
    cout << "Reverse: ";
    for(auto rit = l.rbegin(); rit != l.rend(); ++rit) {
        cout << *rit << " ";
    }

    return 0;
}
```

**Output:**

```
After 2 increments: 30
After 1 decrement: 20
Reverse: 50 40 30 20 10
```

### 2.6 Random Access Iterator

**Random Access Iterator: All bidirectional + random access**

Operations: Bidirectional + `it + n`, `it - n`, `it[n]`, `<`, `>`, `<=`, `>=`

```cpp
#include <iostream>
#include <vector>
#include <deque>
using namespace std;

int main() {
    vector<int> v = {10, 20, 30, 40, 50};

    // WHY: vector provides random access iterators
    auto it = v.begin();

    // Jump to any position
    cout << "Element at index 3: " << *(it + 3) << endl;

    // Array-style access
    cout << "Using []: " << it[2] << endl;

    // Distance between iterators
    auto end = v.end();
    cout << "Distance: " << (end - it) << endl;

    // Comparison
    auto mid = it + 2;
    if(it < mid) {
        cout << "it is before mid" << endl;
    }

    return 0;
}
```

**Output:**

```
Element at index 3: 40
Using []: 30
Distance: 5
it is before mid
```

### 2.7 Iterator Category Summary

| Category | Containers | Can Read | Can Write | Multi-pass | Forward | Backward | Random |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **Input** | istream | ✅ | ❌ | ❌ | ✅ | ❌ | ❌ |
| **Output** | ostream | ❌ | ✅ | ❌ | ✅ | ❌ | ❌ |
| **Forward** | forward_list | ✅ | ✅ | ✅ | ✅ | ❌ | ❌ |
| **Bidirectional** | list, set, map | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| **Random Access** | vector, deque, array | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |

---

## 3. Iterator Operations

### 3.1 Common Iterator Functions

```cpp
#include <iostream>
#include <vector>
#include <iterator>
using namespace std;

int main() {
    vector<int> v = {10, 20, 30, 40, 50};

    // WHY: Helper functions for iterator manipulation

    auto it = v.begin();

    // advance(it, n) - Move iterator n steps
    advance(it, 2);  // Move 2 steps forward
    cout << "After advance(2): " << *it << endl;  // 30

    // distance(first, last) - Count steps between iterators
    int dist = distance(v.begin(), v.end());
    cout << "Distance begin to end: " << dist << endl;  // 5

    // next(it, n) - Return iterator n steps ahead (doesn't modify it)
    auto next_it = next(v.begin(), 3);
    cout << "Next(3) from begin: " << *next_it << endl;  // 40

    // prev(it, n) - Return iterator n steps back
    auto prev_it = prev(v.end(), 2);
    cout << "Prev(2) from end: " << *prev_it << endl;  // 40

    return 0;
}
```

**Output:**

```
After advance(2): 30
Distance begin to end: 5
Next(3) from begin: 40
Prev(2) from end: 40
```

### 3.2 begin() and end() Variants

```cpp
#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v = {10, 20, 30, 40, 50};

    // WHY: Different iterator types for different needs

    // begin() / end() - normal iterators
    auto it = v.begin();
    *it = 15;  // Can modify

    // cbegin() / cend() - const iterators (C++11)
    auto cit = v.cbegin();
    // *cit = 25;  // ERROR! Cannot modify
    cout << *cit << endl;  // Can read

    // rbegin() / rend() - reverse iterators
    cout << "Reverse: ";
    for(auto rit = v.rbegin(); rit != v.rend(); ++rit) {
        cout << *rit << " ";
    }
    cout << endl;

    // crbegin() / crend() - const reverse iterators (C++11)
    auto crit = v.crbegin();
    cout << "Last element: " << *crit << endl;

    return 0;
}
```

**Output:**

```
15
Reverse: 50 40 30 20 15
Last element: 50
```

### 3.3 Iterator Arithmetic (Random Access Only)

```cpp
#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v = {10, 20, 30, 40, 50};

    // WHY: Random access iterators support arithmetic

    auto it = v.begin();

    // Addition
    auto it2 = it + 3;
    cout << "begin + 3: " << *it2 << endl;  // 40

    // Subtraction
    auto it3 = v.end() - 2;
    cout << "end - 2: " << *it3 << endl;  // 40

    // Increment / Decrement
    ++it;
    cout << "After ++: " << *it << endl;  // 20
    --it;
    cout << "After --: " << *it << endl;  // 10

    // Compound assignment
    it += 2;
    cout << "After += 2: " << *it << endl;  // 30
    it -= 1;
    cout << "After -= 1: " << *it << endl;  // 20

    // Array-style access
    cout << "it[2]: " << it[2] << endl;  // 40

    return 0;
}
```

**Output:**

```
begin + 3: 40
end - 2: 40
After ++: 20
After --: 10
After += 2: 30
After -= 1: 20
it[2]: 40
```

---

## 4. Iterator Adapters

### 4.1 Reverse Iterators

```cpp
#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v = {10, 20, 30, 40, 50};

    // WHY: Reverse iterators traverse backwards

    // Normal iteration
    cout << "Forward: ";
    for(auto it = v.begin(); it != v.end(); ++it) {
        cout << *it << " ";
    }
    cout << endl;

    // Reverse iteration
    cout << "Reverse: ";
    for(auto it = v.rbegin(); it != v.rend(); ++it) {
        cout << *it << " ";
    }
    cout << endl;

    // Modify using reverse iterator
    auto rit = v.rbegin();
    *rit = 55;
    cout << "After modify last: " << v.back() << endl;

    return 0;
}
```

**Output:**

```
Forward: 10 20 30 40 50
Reverse: 50 40 30 20 10
After modify last: 55
```

### 4.2 Insert Iterators

```cpp
#include <iostream>
#include <vector>
#include <iterator>
#include <algorithm>
using namespace std;

int main() {
    vector<int> v1 = {1, 2, 3};
    vector<int> v2 = {10, 20, 30};

    // WHY: Insert iterators for adding elements

    // back_insert_iterator - inserts at back
    copy(v2.begin(), v2.end(), back_inserter(v1));

    cout << "After back_insert: ";
    for(int x : v1) cout << x << " ";
    cout << endl;

    // front_insert_iterator - inserts at front (deque, list)
    // insert_iterator - inserts at specific position

    return 0;
}
```

**Output:**

```
After back_insert: 1 2 3 10 20 30
```

### 4.3 Stream Iterators

```cpp
#include <iostream>
#include <iterator>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    // WHY: Stream iterators bridge I/O and algorithms

    // istream_iterator - read from input
    cout << "Enter numbers (Ctrl+D to end): ";
    istream_iterator<int> in_iter(cin);
    istream_iterator<int> eof;

    vector<int> v(in_iter, eof);

    // ostream_iterator - write to output
    cout << "You entered: ";
    ostream_iterator<int> out_iter(cout, " ");
    copy(v.begin(), v.end(), out_iter);

    return 0;
}
```

**Input:**

```
10 20 30
```

**Output:**

```
Enter numbers (Ctrl+D to end): You entered: 10 20 30
```

---

## 5. Iterator Invalidation Summary

### 5.1 Invalidation by Container

**Understanding when iterators become invalid is critical for avoiding crashes.**

| Container | Insert | Erase | push_back | push_front |
| --- | --- | --- | --- | --- |
| **vector** | From insert point → end | From erase point → end | All if realloc | N/A |
| **deque** | All | All | End iterator | All |
| **list** | None | Only erased | None | None |
| **forward_list** | None | Only erased | N/A | None |
| **set/map** | None | Only erased | N/A | N/A |
| **unordered** | All if rehash | Only erased | All if rehash | N/A |

### 5.2 Safe Iterator Patterns

```cpp
#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    // WHY: Safe patterns avoid invalidation issues

    // ❌ BAD: Iterator invalidated by erase
    // for(auto it = v.begin(); it != v.end(); ++it) {
    //     if(*it % 2 == 0) {
    //         v.erase(it);  // Invalidates it!
    //         // ++it will crash
    //     }
    // }

    // ✅ GOOD: Use erase's return value
    for(auto it = v.begin(); it != v.end(); ) {
        if(*it % 2 == 0) {
            it = v.erase(it);  // erase returns next valid iterator
        } else {
            ++it;
        }
    }

    cout << "Odd numbers: ";
    for(int x : v) cout << x << " ";

    return 0;
}
```

**Output:**

```
Odd numbers: 1 3 5 7 9
```

### 5.3 Iterator Invalidation Best Practices

```cpp
// ✅ GOOD PRACTICES:

// 1. Use return value of modifying operations
it = container.erase(it);
it = container.insert(it, value);

// 2. Refresh iterators after modifications
auto it = container.begin();
container.push_back(x);
it = container.begin();  // Refresh!

// 3. Use indices for vector when modifying
for(size_t i = 0; i < v.size(); ) {
    if(should_remove(v[i])) {
        v.erase(v.begin() + i);
    } else {
        ++i;
    }
}

// 4. Use algorithms that handle invalidation
v.erase(remove_if(v.begin(), v.end(), predicate), v.end());

// 5. For list, iterators stay valid (except erased)
for(auto it = l.begin(); it != l.end(); ++it) {
    if(condition) {
        auto temp = it;
        ++it;
        l.erase(temp);  // Safe for list!
        --it;
    }
}
```

---

## Summary

### Key Takeaways

1. **Iterators are the bridge between containers and algorithms** providing a uniform interface so one algorithm can work with all containers. Without iterators, every algorithm would need container-specific implementations.
2. **The five iterator categories form a capability hierarchy** from least to most powerful: Input (read-only, single-pass), Output (write-only), Forward (read/write, multi-pass), Bidirectional (adds backward movement), Random Access (adds arithmetic and subscripting).
3. **Iterator category determines which algorithms can use it** - sort requires Random Access, find only needs Input. Algorithms are designed to work with the minimum required category, maximizing reusability.
4. **Random Access iterators support pointer-like arithmetic** including it+n, it-n, it[n], and comparisons (<, >, <=, >=). Only vector, deque, and array provide this capability, enabling O(1) element access.
5. **Helper functions simplify iterator manipulation** - advance() moves iterators, distance() counts steps between them, next() and prev() return shifted iterators without modifying the original. These work with all iterator categories.
6. **begin()/end() have const and reverse variants** - cbegin()/cend() for read-only access, rbegin()/rend() for reverse iteration, crbegin()/crend() for const reverse. Use const variants to prevent accidental modification.
7. **Iterator adapters extend functionality** - reverse_iterator traverses backwards, insert_iterator enables insertion during copy, stream_iterator bridges I/O with algorithms. Adapters compose to create powerful combinations.
8. **Iterator invalidation rules vary by container** - vector invalidates aggressively on modifications, list only invalidates erased elements, set/map preserve non-erased iterators. Understanding these rules prevents crashes.
9. **Use erase()'s return value to avoid invalidation** - erase() returns the next valid iterator, enabling safe removal during iteration. This pattern works across all containers that support erase().
10. **Different containers optimize for different access patterns** - vector's random access enables binary search, list's bidirectional enables splice, forward_list's forward-only saves memory. Choose iterator category based on algorithm requirements.

---

## Interview Preparation

### Q1: Explain the five iterator categories. Give examples of containers for each and what operations each category supports.

**Answer:**

The five iterator categories form a capability hierarchy, each adding more operations than the previous:

**Input Iterator** - Read-only, single-pass, forward-only. Can read each element only once before moving to next. Operations: *it (read), ++it, it++, ==, !=. Example: istream_iterator for reading from streams. Cannot go backward or re-read elements. Used by algorithms like find that only need one-pass reading.

**Output Iterator** - Write-only, single-pass. Mirror of input iterator but for writing. Operations: *it = value (write), ++it, it++. Example: ostream_iterator for writing to streams. Cannot read values back. Used by algorithms like copy for output.

**Forward Iterator** - Read/write, multi-pass, forward-only. Combines input and output capabilities with multi-pass guarantee. Operations: All input/output ops + can save and reuse iterators. Example: forward_list::iterator. Can traverse multiple times but only forward. Used by algorithms like replace that need to revisit elements.

**Bidirectional Iterator** - All forward capabilities plus backward movement. Operations: Forward ops + --it, it--. Examples: list::iterator, set::iterator, map::iterator. Can traverse in both directions. Enables reverse iteration with rbegin()/rend(). Used by algorithms like reverse that need backward traversal.

**Random Access Iterator** - All bidirectional capabilities plus random access and arithmetic. Operations: Bidirectional ops + it+n, it-n, it+=n, it-=n, it[n], <, >, <=, >=, it1-it2. Examples: vector::iterator, deque::iterator, array::iterator, raw pointers. Can jump to any position in O(1). Used by algorithms like sort and binary_search that need random access.

Why hierarchy matters: Algorithms are written to require minimum iterator category. find() only needs Input iterator, so it works with all categories. sort() needs Random Access, so it only works with vector/deque/array. This design maximizes algorithm reusability while maintaining efficiency.

Container choice affects algorithm availability: Can't use sort() directly on list because list provides only Bidirectional iterators. Must use list::sort() member function instead. Understanding iterator categories helps you choose the right container for your algorithms.

---

### Q2: What is iterator invalidation? Explain the invalidation rules for vector, list, and set. How do you safely remove elements while iterating?

**Answer:**

Iterator invalidation occurs when an iterator becomes unsafe to use after container modifications. Using invalidated iterators causes undefined behavior - crashes, data corruption, or silent bugs.

**vector invalidation rules:**

Insert/push_back: Invalidates all iterators if reallocation occurs (capacity increases). If no reallocation, invalidates from insertion point to end because elements shift right. You can't know in advance if reallocation will occur, so assume invalidation.

Erase/pop_back: Invalidates from erase point to end because elements shift left to fill gap. pop_back only invalidates end() and iterator to removed element.

Why so aggressive: vector's contiguous memory means any insertion/deletion requires shifting elements or reallocating entire array.

**list invalidation rules:**

Insert: Never invalidates any iterators. New node is linked in without affecting other nodes.

Erase: Only invalidates iterators to erased elements. All other iterators remain valid.

Why so stable: list's node-based structure means modifications only affect direct neighbors via pointer updates, not other nodes.

**set/map invalidation rules:**

Insert: Never invalidates any iterators (except if causing rehash in unordered variants).

Erase: Only invalidates iterators to erased elements.

Why stable: Red-Black Tree structure preserves all non-erased nodes in same locations.

**Safe removal patterns:**

Pattern 1 - Use erase's return value (all containers):

```cpp
for(auto it = container.begin(); it != container.end(); ) {
    if(should_remove(*it)) {
        it = container.erase(it);  // erase returns next valid iterator
    } else {
        ++it;
    }
}
```

Pattern 2 - Use indices for vector:

```cpp
for(size_t i = 0; i < v.size(); ) {
    if(should_remove(v[i])) {
        v.erase(v.begin() + i);  // Don't increment i
    } else {
        ++i;
    }
}
```

Pattern 3 - Remove-erase idiom (most efficient for vector):

```cpp
v.erase(remove_if(v.begin(), v.end(), predicate), v.end());
// remove_if moves elements to keep to front, returns iterator to first "removed"
// erase actually removes from that point to end
```

Pattern 4 - List-specific (exploits strong iterator stability):

```cpp
for(auto it = l.begin(); it != l.end(); ) {
    if(should_remove(*it)) {
        it = l.erase(it);  // Safe, returns next
    } else {
        ++it;
    }
}
```

Common mistakes: Incrementing iterator after erase (uses invalidated iterator), not using erase's return value, assuming all containers have same invalidation rules.

---

### Q3: Explain advance(), distance(), next(), and prev(). How do they work differently for different iterator categories?

**Answer:**

These functions provide uniform operations across all iterator categories, but implementation differs based on iterator capabilities.

**advance(it, n)** - Moves iterator n positions (modifies it in place):

For Random Access iterators (vector, deque, array):

```cpp
auto it = v.begin();
advance(it, 5);  // Implemented as: it += 5 (O(1))
```

For Bidirectional/Forward iterators (list, set):

```cpp
auto it = l.begin();
advance(it, 5);  // Implemented as: for loop with 5 increments (O(n))
```

Why different: Random access iterators support arithmetic directly (O(1)), others must increment in loop (O(n)). The function hides this difference, providing consistent interface.

**distance(first, last)** - Counts elements between iterators:

For Random Access:

```cpp
int n = distance(v.begin(), v.end());  // Implemented as: last - first (O(1))
```

For others:

```cpp
int n = distance(l.begin(), l.end());  // Loop counting increments (O(n))
```

Return value is negative if first > last for Random Access iterators.

**next(it, n=1)** - Returns iterator n positions ahead (doesn't modify it):

```cpp
auto it = next(v.begin(), 3);  // Returns iterator to 4th element
// Original v.begin() unchanged
```

Implementation: Creates copy of it, calls advance(copy, n), returns copy. Convenient for getting offset iterator without modifying original.

**prev(it, n=1)** - Returns iterator n positions back:

```cpp
auto it = prev(v.end(), 2);  // Returns iterator to 2nd-to-last element
```

Requires at least Bidirectional iterator (can't go backward with Forward iterator).

**Practical differences:**

For vector (Random Access):

```cpp
advance(it, 1000000);  // O(1) - single arithmetic operation
int d = distance(v.begin(), v.end());  // O(1) - subtraction
```

For list (Bidirectional):

```cpp
advance(it, 1000000);  // O(1,000,000) - 1 million increments!
int d = distance(l.begin(), l.end());  // O(n) - must traverse entire list
```

This is why list doesn't have random access - operations that look constant-time are actually linear-time.

**Best practices:**

Use next/prev instead of manual arithmetic when possible:

```cpp
// ✅ GOOD - works with all iterators
auto it = next(container.begin(), 5);

// ❌ WORKS but only for Random Access
auto it = container.begin() + 5;
```

Avoid distance() in loops with non-Random Access iterators:

```cpp
// ❌ BAD - O(n²) for list!
for(int i = 0; i < distance(l.begin(), l.end()); i++) { }

// ✅ GOOD - O(n)
for(auto it = l.begin(); it != l.end(); ++it) { }
```

These functions enable generic code that adapts to iterator capabilities, letting you write algorithms that work with any container while maintaining best possible performance.

---

### Q4: What are reverse iterators? How do rbegin() and rend() work? Give an example of when you'd use them.

**Answer:**

Reverse iterators are iterator adapters that traverse containers backward. They make backward traversal look like forward traversal by reversing the meaning of ++ and --.

**How they work:**

rbegin() returns reverse_iterator pointing to the last element. rend() returns reverse_iterator pointing before the first element (one position before begin()).

```cpp
vector<int> v = {10, 20, 30, 40, 50};

// Normal iteration: begin → end
// [10] [20] [30] [40] [50] |
//   ↑                       ↑
// begin()                 end()

// Reverse iteration: rbegin → rend
// | [10] [20] [30] [40] [50]
//  ↑                       ↑
// rend()               rbegin()
```

Key insight: ++rit moves backward through container. This lets you use standard algorithms backward:

```cpp
vector<int> v = {10, 20, 30, 40, 50};

// Forward
for(auto it = v.begin(); it != v.end(); ++it) {
    cout << *it << " ";  // 10 20 30 40 50
}

// Reverse - same loop structure!
for(auto it = v.rbegin(); it != v.rend(); ++it) {
    cout << *it << " ";  // 50 40 30 20 10
}
```

**Practical use cases:**

1. Process elements in reverse order:

```cpp
// Print in reverse
copy(v.rbegin(), v.rend(), ostream_iterator<int>(cout, " "));
```

1. Find last occurrence:

```cpp
// Find last even number
auto rit = find_if(v.rbegin(), v.rend(), [](int x) { return x % 2 == 0; });
if(rit != v.rend()) {
    cout << "Last even: " << *rit << endl;
}
```

1. Reverse algorithms:

```cpp
// Sort in descending order using reverse iterators
sort(v.rbegin(), v.rend());  // Now v = {50, 40, 30, 20, 10}
```

1. Process from end to beginning while modifying:

```cpp
// Remove duplicates from end
for(auto rit = v.rbegin(); rit != v.rend(); ) {
    if(some_condition(*rit)) {
        // base() converts reverse_iterator to regular iterator
        rit = reverse_iterator(v.erase((++rit).base()));
    } else {
        ++rit;
    }
}
```

**base() conversion:**

Reverse iterators have base() method to get underlying regular iterator. But there's a quirk - reverse_iterator points to element before its base:

```cpp
auto rit = v.rbegin();  // Points to last element
auto it = rit.base();   // Points PAST last element (== v.end())

// This is why conversions need adjustment
```

**When to use:**

Use reverse iterators when you need to traverse backward but want to use standard algorithms or loop patterns. More readable than manual backward traversal with --it.

Don't use when simple reverse() suffices:

```cpp
// ❌ Overkill
vector<int> result;
copy(v.rbegin(), v.rend(), back_inserter(result));

// ✅ Simpler
reverse(v.begin(), v.end());
```

Reverse iterators work with any Bidirectional or Random Access container (list, vector, deque, set, map) but not Forward-only containers (forward_list).

---

### Q5: Why can't you use std::sort with list? How would you sort a list? What does this tell you about algorithm requirements?

**Answer:**

You can't use std::sort with list because sort requires Random Access iterators but list only provides Bidirectional iterators.

**Why sort needs Random Access:**

std::sort uses introsort algorithm (hybrid of quicksort, heapsort, and insertion sort). These algorithms fundamentally require:

- Jumping to arbitrary positions (pivot selection in quicksort)
- Comparing non-adjacent elements
- Swapping elements at distance

All these require O(1) random access. With Bidirectional iterators:

```cpp
list<int> l = {5, 2, 8, 1, 9};
auto it = l.begin();

// To access 5th element (needed for sorting):
advance(it, 4);  // O(n) - must traverse 4 nodes

// Quicksort partition needs to jump around:
// O(n) to access pivot, O(n) to access each element for comparison
// Makes O(n log n) algorithm become O(n² log n)!
```

**How to sort list:**

Use list::sort() member function:

```cpp
list<int> l = {5, 2, 8, 1, 9};
l.sort();  // Uses merge sort optimized for linked lists
```

list::sort() uses merge sort which:

- Works efficiently with linked lists (O(n log n))
- Only needs sequential access
- Doesn't require random access
- Sorts by relinking nodes, not moving values

Why member function instead of algorithm: list knows its internal structure and can optimize. It can rearrange nodes by changing pointers instead of moving values.

**Generic sorting options:**

If you must use std::sort (maybe need custom comparator library code expects std::sort):

```cpp
// 1. Copy to vector, sort, copy back
list<int> l = {5, 2, 8, 1, 9};
vector<int> v(l.begin(), l.end());
sort(v.begin(), v.end());
l.assign(v.begin(), v.end());
// Downside: O(n) extra space, O(n) copy overhead
```

**What this teaches about algorithm requirements:**

Algorithms declare minimum iterator category they need:

- find: Input iterator (single-pass read)
- reverse: Bidirectional iterator (needs backward movement)
- sort: Random Access iterator (needs jumping)

This creates compile-time contract:

```cpp
list<int> l;
sort(l.begin(), l.end());  // Compile error!
// Error: sort requires random access, list provides bidirectional
```

Iterator categories enable static polymorphism - compiler selects appropriate algorithm implementation based on iterator category. For example, distance() compiles to:

- Subtraction for Random Access: O(1)
- Counting loop for Bidirectional: O(n)

This is why choosing right container matters: list is wrong if you need sorting with std::sort. vector is wrong if you need O(1) insertion in middle. Iterator categories force you to think about algorithm requirements upfront.

**Practical implications:**

When designing APIs, specify iterator category requirements in comments/concepts. When choosing containers, consider which algorithms you'll use. When performance matters, match container capabilities to algorithm requirements - don't use list if you need binary_search, don't use vector if you need splice.

The compile error is actually helpful - it prevents you from accidentally using O(n²) algorithm when you expected O(n log n).