# 6.6. STL Algorithms & Functors

---

## Table of Contents

1. STL Algorithms Overview
2. Sorting Algorithms
3. Searching Algorithms
4. Modifying Algorithms
5. Numeric Algorithms
6. Functors - Function Objects
7. Lambda Expressions
8. Summary
9. Interview Preparation

---

## 1. STL Algorithms Overview

### 1.1 What are STL Algorithms?

**STL algorithms are generic functions that operate on ranges of elements using iterators.**

Header: `#include <algorithm>`, `#include <numeric>`

**Why Algorithms Exist:**

Without algorithms, you'd write the same loops repeatedly. Algorithms provide tested, optimized implementations that work with any container.

```cpp
// WITHOUT algorithms - manual loop
vector<int> v = {5, 2, 8, 1, 9};
int max_val = v[0];
for(size_t i = 1; i < v.size(); i++) {
    if(v[i] > max_val) max_val = v[i];
}

// WITH algorithms - one line
int max_val = *max_element(v.begin(), v.end())
```

### 1.2 Algorithm Categories

**Categories by operation:**

| Category | Purpose | Examples |
| --- | --- | --- |
| **Non-modifying** | Read elements | find, count, equal |
| **Modifying** | Change elements | copy, transform, remove |
| **Sorting** | Order elements | sort, stable_sort |
| **Binary search** | Search sorted ranges | binary_search, lower_bound |
| **Numeric** | Mathematical operations | accumulate, inner_product |
| **Set operations** | Set theory | union, intersection |

### 1.3 Key Principles

**Range-based:** Algorithms work on ranges `[begin, end)` using iterators.

```cpp
vector<int> v = {1, 2, 3, 4, 5};
// Range: [v.begin(), v.end()) includes all elements
```

**Iterator requirements:** Each algorithm requires specific iterator category.

```cpp
sort(v.begin(), v.end());     // Requires Random Access
find(l.begin(), l.end(), 5);  // Requires Input Iterator
```

**Return values:** Most algorithms return iterators, not values.

```cpp
auto it = find(v.begin(), v.end(), 3);
if(it != v.end()) {
    cout << "Found: " << *it << endl;
}
```

---

## 2. Sorting Algorithms

### 2.1 std::sort - Basic Sorting

**sort sorts elements in ascending order by default (O(n log n) average).**

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    vector<int> v = {5, 2, 8, 1, 9, 3};

    // WHY: Highly optimized introsort (hybrid quicksort/heapsort)
    sort(v.begin(), v.end());

    cout << "Sorted: ";
    for(int x : v) cout << x << " ";
    cout << endl;

    // Sort in descending order
    sort(v.begin(), v.end(), greater<int>());

    cout << "Descending: ";
    for(int x : v) cout << x << " ";

    return 0;
}
```

**Output:**

```
Sorted: 1 2 3 5 8 9
Descending: 9 8 5 3 2 1
```

### 2.2 Custom Comparators

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
using namespace std;

struct Person {
    string name;
    int age;
};

int main() {
    vector<Person> people = {
        {"Alice", 30},
        {"Bob", 25},
        {"Charlie", 35}
    };

    // WHY: Custom sorting criteria
    sort(people.begin(), people.end(),
         [](const Person& a, const Person& b) {
             return a.age < b.age;  // Sort by age
         });

    cout << "Sorted by age:\n";
    for(auto& p : people) {
        cout << p.name << " (" << p.age << ")\n";
    }

    return 0;
}
```

**Output:**

```
Sorted by age:
Bob (25)
Alice (30)
Charlie (35)
```

### 2.3 stable_sort - Preserve Relative Order

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

struct Item {
    string name;
    int priority;
};

int main() {
    vector<Item> items = {
        {"Task1", 2},
        {"Task2", 1},
        {"Task3", 2},
        {"Task4", 1}
    };

    // WHY: stable_sort preserves order for equal elements
    stable_sort(items.begin(), items.end(),
                [](const Item& a, const Item& b) {
                    return a.priority < b.priority;
                });

    cout << "Stable sorted:\n";
    for(auto& item : items) {
        cout << item.name << " (priority " << item.priority << ")\n";
    }

    return 0;
}
```

**Output:**

```
Stable sorted:
Task2 (priority 1)
Task4 (priority 1)
Task1 (priority 2)
Task3 (priority 2)
```

Note: Task2 comes before Task4 (both priority 1) - original order preserved!

### 2.4 partial_sort - Sort First N Elements

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    vector<int> v = {9, 2, 7, 4, 1, 8, 3, 6, 5};

    // WHY: Only need top 3 elements sorted
    partial_sort(v.begin(), v.begin() + 3, v.end());

    cout << "Top 3 smallest: ";
    for(int i = 0; i < 3; i++) {
        cout << v[i] << " ";
    }
    cout << "\nRest (unsorted): ";
    for(int i = 3; i < v.size(); i++) {
        cout << v[i] << " ";
    }

    return 0;
}
```

**Output:**

```
Top 3 smallest: 1 2 3
Rest (unsorted): 4 9 8 7 6 5
```

---

## 3. Searching Algorithms

### 3.1 find - Linear Search

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    vector<int> v = {10, 20, 30, 40, 50};

    // WHY: Find first occurrence - O(n)
    auto it = find(v.begin(), v.end(), 30);

    if(it != v.end()) {
        cout << "Found 30 at index " << (it - v.begin()) << endl;
    } else {
        cout << "Not found" << endl;
    }

    // find_if with predicate
    auto it2 = find_if(v.begin(), v.end(),
                       [](int x) { return x > 35; });

    if(it2 != v.end()) {
        cout << "First > 35: " << *it2 << endl;
    }

    return 0;
}
```

**Output:**

```
Found 30 at index 2
First > 35: 40
```

### 3.2 binary_search - Fast Search in Sorted Range

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    vector<int> v = {1, 2, 3, 5, 8, 9};  // MUST be sorted!

    // WHY: O(log n) search vs O(n) for find
    bool found = binary_search(v.begin(), v.end(), 5);
    cout << "5 found: " << (found ? "Yes" : "No") << endl;

    found = binary_search(v.begin(), v.end(), 7);
    cout << "7 found: " << (found ? "Yes" : "No") << endl;

    return 0;
}
```

**Output:**

```
5 found: Yes
7 found: No
```

### 3.3 lower_bound and upper_bound

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    vector<int> v = {1, 2, 2, 2, 3, 5, 5, 8};

    // WHY: Find range of equal elements

    // lower_bound: first element >= value
    auto lower = lower_bound(v.begin(), v.end(), 2);
    cout << "Lower bound of 2 at index: " << (lower - v.begin()) << endl;

    // upper_bound: first element > value
    auto upper = upper_bound(v.begin(), v.end(), 2);
    cout << "Upper bound of 2 at index: " << (upper - v.begin()) << endl;

    // Count occurrences
    int count = upper - lower;
    cout << "Count of 2: " << count << endl;

    return 0;
}
```

**Output:**

```
Lower bound of 2 at index: 1
Upper bound of 2 at index: 4
Count of 2: 3
```

### 3.4 count and count_if

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    vector<int> v = {1, 2, 3, 2, 4, 2, 5};

    // WHY: Count occurrences
    int count2 = count(v.begin(), v.end(), 2);
    cout << "Count of 2: " << count2 << endl;

    // count_if with predicate
    int even_count = count_if(v.begin(), v.end(),
                              [](int x) { return x % 2 == 0; });
    cout << "Even numbers: " << even_count << endl;

    return 0;
}
```

**Output:**

```
Count of 2: 3
Even numbers: 4
```

---

## 4. Modifying Algorithms

### 4.1 copy and copy_if

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    vector<int> src = {1, 2, 3, 4, 5};
    vector<int> dest(5);

    // WHY: Copy elements efficiently
    copy(src.begin(), src.end(), dest.begin());

    cout << "Copied: ";
    for(int x : dest) cout << x << " ";
    cout << endl;

    // copy_if: conditional copy
    vector<int> evens;
    copy_if(src.begin(), src.end(), back_inserter(evens),
            [](int x) { return x % 2 == 0; });

    cout << "Even numbers: ";
    for(int x : evens) cout << x << " ";

    return 0;
}
```

**Output:**

```
Copied: 1 2 3 4 5
Even numbers: 2 4
```

### 4.2 transform - Apply Function to Elements

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    vector<int> v = {1, 2, 3, 4, 5};
    vector<int> result(5);

    // WHY: Apply transformation to each element
    transform(v.begin(), v.end(), result.begin(),
              [](int x) { return x * x; });

    cout << "Squares: ";
    for(int x : result) cout << x << " ";
    cout << endl;

    // Binary transform - combine two ranges
    vector<int> v2 = {10, 20, 30, 40, 50};
    transform(v.begin(), v.end(), v2.begin(), result.begin(),
              [](int a, int b) { return a + b; });

    cout << "Sums: ";
    for(int x : result) cout << x << " ";

    return 0;
}
```

**Output:**

```
Squares: 1 4 9 16 25
Sums: 11 22 33 44 55
```

### 4.3 remove and remove_if (Erase-Remove Idiom)

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    // WHY: remove doesn't actually erase - just moves elements
    auto new_end = remove_if(v.begin(), v.end(),
                             [](int x) { return x % 2 == 0; });

    cout << "After remove_if (before erase):\n";
    cout << "Size: " << v.size() << endl;
    cout << "Elements: ";
    for(int x : v) cout << x << " ";
    cout << endl;

    // Erase-remove idiom: actually remove elements
    v.erase(new_end, v.end());

    cout << "\nAfter erase:\n";
    cout << "Size: " << v.size() << endl;
    cout << "Elements: ";
    for(int x : v) cout << x << " ";

    return 0;
}
```

**Output:**

```
After remove_if (before erase):
Size: 9
Elements: 1 3 5 7 9 6 7 8 9

After erase:
Size: 5
Elements: 1 3 5 7 9
```

### 4.4 fill and generate

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    vector<int> v1(5);

    // WHY: Fill with constant value
    fill(v1.begin(), v1.end(), 42);

    cout << "Filled: ";
    for(int x : v1) cout << x << " ";
    cout << endl;

    // generate: Fill with function result
    vector<int> v2(5);
    int n = 0;
    generate(v2.begin(), v2.end(), [&n]() { return n++; });

    cout << "Generated: ";
    for(int x : v2) cout << x << " ";

    return 0;
}
```

**Output:**

```
Filled: 42 42 42 42 42
Generated: 0 1 2 3 4
```

### 4.5 reverse and rotate

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    vector<int> v = {1, 2, 3, 4, 5};

    // WHY: Reverse in-place
    reverse(v.begin(), v.end());
    cout << "Reversed: ";
    for(int x : v) cout << x << " ";
    cout << endl;

    // rotate: Shift elements
    v = {1, 2, 3, 4, 5};
    rotate(v.begin(), v.begin() + 2, v.end());
    // [1,2,3,4,5] → [3,4,5,1,2]

    cout << "Rotated: ";
    for(int x : v) cout << x << " ";

    return 0;
}
```

**Output:**

```
Reversed: 5 4 3 2 1
Rotated: 3 4 5 1 2
```

---

## 5. Numeric Algorithms

### 5.1 accumulate - Sum and Fold

```cpp
#include <iostream>
#include <vector>
#include <numeric>
using namespace std;

int main() {
    vector<int> v = {1, 2, 3, 4, 5};

    // WHY: Sum all elements
    int sum = accumulate(v.begin(), v.end(), 0);
    cout << "Sum: " << sum << endl;

    // Custom operation - product
    int product = accumulate(v.begin(), v.end(), 1,
                             [](int a, int b) { return a * b; });
    cout << "Product: " << product << endl;

    // Concatenate strings
    vector<string> words = {"Hello", " ", "World"};
    string sentence = accumulate(words.begin(), words.end(), string(""));
    cout << "Sentence: " << sentence << endl;

    return 0;
}
```

**Output:**

```
Sum: 15
Product: 120
Sentence: Hello World
```

### 5.2 inner_product - Dot Product

```cpp
#include <iostream>
#include <vector>
#include <numeric>
using namespace std;

int main() {
    vector<int> v1 = {1, 2, 3};
    vector<int> v2 = {4, 5, 6};

    // WHY: Calculate dot product
    // (1*4) + (2*5) + (3*6) = 4 + 10 + 18 = 32
    int dot = inner_product(v1.begin(), v1.end(), v2.begin(), 0);
    cout << "Dot product: " << dot << endl;

    return 0;
}
```

**Output:**

```
Dot product: 32
```

### 5.3 iota - Generate Sequence

```cpp
#include <iostream>
#include <vector>
#include <numeric>
using namespace std;

int main() {
    vector<int> v(10);

    // WHY: Fill with incrementing sequence
    iota(v.begin(), v.end(), 1);  // Start from 1

    cout << "Sequence: ";
    for(int x : v) cout << x << " ";

    return 0;
}
```

**Output:**

```
Sequence: 1 2 3 4 5 6 7 8 9 10
```

---

## 6. Functors - Function Objects

### 6.1 What are Functors?

**Functors are objects that can be called like functions using operator().**

```cpp
#include <iostream>
using namespace std;

// WHY: Functor is object with operator()
struct Multiply {
    int operator()(int a, int b) const {
        return a * b;
    }
};

int main() {
    Multiply mult;

    cout << "3 * 4 = " << mult(3, 4) << endl;

    // Can be used like function
    int result = mult(5, 6);
    cout << "5 * 6 = " << result << endl;

    return 0;
}
```

**Output:**

```
3 * 4 = 12
5 * 6 = 30
```

### 6.2 Functors with State

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

// WHY: Functors can maintain state
struct Counter {
    int count = 0;

    bool operator()(int x) {
        count++;
        return x % 2 == 0;
    }
};

int main() {
    vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};

    Counter counter;
    count_if(v.begin(), v.end(), ref(counter));

    cout << "Even numbers found: " << counter.count << endl;

    return 0;
}
```

**Output:**

```
Even numbers found: 4
```

### 6.3 Predefined Functors

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <functional>
using namespace std;

int main() {
    vector<int> v = {5, 2, 8, 1, 9};

    // WHY: STL provides common functors

    // less<int> - ascending order (default)
    sort(v.begin(), v.end(), less<int>());
    cout << "Ascending: ";
    for(int x : v) cout << x << " ";
    cout << endl;

    // greater<int> - descending order
    sort(v.begin(), v.end(), greater<int>());
    cout << "Descending: ";
    for(int x : v) cout << x << " ";
    cout << endl;

    // Other predefined: plus, minus, multiplies, divides
    int sum = accumulate(v.begin(), v.end(), 0, plus<int>());
    cout << "Sum: " << sum << endl;

    return 0;
}
```

**Output:**

```
Ascending: 1 2 5 8 9
Descending: 9 8 5 2 1
Sum: 25
```

---

## 7. Lambda Expressions

### 7.1 Lambda Syntax

**Lambda: Anonymous inline function (C++11)**

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    // WHY: Lambda syntax: [capture](params) -> return_type { body }

    // Simple lambda
    auto add = [](int a, int b) { return a + b; };
    cout << "3 + 4 = " << add(3, 4) << endl;

    // Lambda with algorithm
    vector<int> v = {1, 2, 3, 4, 5};

    for_each(v.begin(), v.end(), [](int x) {
        cout << x * x << " ";
    });

    return 0;
}
```

**Output:**

```
3 + 4 = 7
1 4 9 16 25
```

### 7.2 Capture Modes

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    int threshold = 5;
    vector<int> v = {1, 3, 5, 7, 9};

    // WHY: Different capture modes

    // [=] - Capture by value (copy)
    int count1 = count_if(v.begin(), v.end(), [=](int x) {
        return x > threshold;  // threshold copied
    });
    cout << "Count > threshold (by value): " << count1 << endl;

    // [&] - Capture by reference
    int sum = 0;
    for_each(v.begin(), v.end(), [&](int x) {
        sum += x;  // sum modified
    });
    cout << "Sum (by reference): " << sum << endl;

    // [x, &y] - Specific captures
    int factor = 2;
    int total = 0;
    for_each(v.begin(), v.end(), [factor, &total](int x) {
        total += x * factor;
    });
    cout << "Total (mixed capture): " << total << endl;

    return 0;
}
```

**Output:**

```
Count > threshold (by value): 2
Sum (by reference): 25
Total (mixed capture): 50
```

### 7.3 Mutable Lambdas

```cpp
#include <iostream>
using namespace std;

int main() {
    // WHY: mutable allows modifying captured-by-value variables

    int x = 0;

    auto increment = [x]() mutable {
        x++;  // OK with mutable
        return x;
    };

    cout << increment() << endl;  // 1
    cout << increment() << endl;  // 2
    cout << "Original x: " << x << endl;  // 0 (not modified)

    return 0;
}
```

**Output:**

```
1
2
Original x: 0
```

### 7.4 Lambda Use Cases

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
using namespace std;

int main() {
    vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    // WHY: Lambdas excel at short, one-time functions

    // 1. Filtering
    v.erase(remove_if(v.begin(), v.end(),
                      [](int x) { return x % 2 == 0; }),
            v.end());

    // 2. Transforming
    transform(v.begin(), v.end(), v.begin(),
              [](int x) { return x * 2; });

    // 3. Custom sorting
    sort(v.begin(), v.end(),
         [](int a, int b) { return a > b; });

    cout << "Result: ";
    for(int x : v) cout << x << " ";

    return 0;
}
```

**Output:**

```
Result: 18 10 6 2
```

---

## Summary

### Key Takeaways

1. **STL algorithms eliminate repetitive loops** by providing tested, optimized implementations for common operations. One algorithm works with any container through iterators, enabling code reuse across different data structures.
2. **sort is O(n log n) using introsort** (hybrid quicksort/heapsort/insertion sort). Use stable_sort when relative order of equal elements matters. Use partial_sort when only top N elements need sorting for better performance.
3. **Binary search requires sorted data** and provides O(log n) lookup versus O(n) for linear search. Use lower_bound/upper_bound to find insertion points or ranges of equal elements in sorted containers.
4. **The erase-remove idiom is critical for vector** - remove_if moves elements to keep to front and returns iterator, then erase actually removes from that point to end. This is more efficient than erasing in loop which shifts elements repeatedly.
5. **transform applies functions to ranges** enabling functional-style programming. It can operate on one range (unary) or two ranges (binary), making it versatile for element-wise operations.
6. **accumulate is fold operation** that reduces range to single value. Default is sum, but custom binary operation enables product, concatenation, or any reduction operation.
7. **Functors are objects with operator()** that can maintain state between calls, unlike regular functions. This makes them powerful for stateful operations like counting or accumulating within algorithms.
8. **Lambda expressions provide inline anonymous functions** with capture of surrounding scope. Use [=] for value capture (copy), [&] for reference capture (modify), or specific captures like [x, &y] for fine control.
9. **Capture by value creates copies** that don't affect originals even with mutable keyword. Capture by reference allows modification of originals. Choose based on whether you need to modify external state.
10. **Lambdas excel for short, one-time predicates** with algorithms like find_if, count_if, remove_if. They're clearer than separate functor classes for simple operations but less reusable than named functors for complex logic.

---

## Interview Preparation

### Q1: Explain the erase-remove idiom. Why doesn't remove() actually remove elements? How would you remove all even numbers from a vector?

**Answer:**

The erase-remove idiom is a two-step pattern for efficiently removing elements from a sequence container. It's necessary because remove() and remove_if() don't actually erase elements - they only rearrange them.

How remove_if() works:

1. Scans through the range from left to right
2. Moves elements to keep toward the front
3. Returns iterator pointing to the new "logical end"
4. Elements after this iterator are in undefined state (not removed)
5. Container size remains unchanged

Example visualization:

```
Original:     [1, 2, 3, 4, 5, 6, 7, 8]
After remove_if (x%2==0): [1, 3, 5, 7, ?, ?, ?, ?]
                                      ↑
                                  new_end
Size still 8, but only first 4 elements valid
```

Why this design: remove() works with any container using only iterators. It can't actually erase because:

- Erasing requires container-specific knowledge (member functions)
- Algorithms work only with iterators, not containers
- Different containers have different erase mechanisms

The erase-remove idiom:

```cpp
vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};

// Step 1: remove_if rearranges, returns new end
auto new_end = remove_if(v.begin(), v.end(),
                         [](int x) { return x % 2 == 0; });

// Step 2: erase actually removes elements
v.erase(new_end, v.end());

// Now v = {1, 3, 5, 7} with size 4
```

One-liner version:

```cpp
v.erase(remove_if(v.begin(), v.end(),
                  [](int x) { return x % 2 == 0; }),
        v.end());
```

Why efficient: Moves elements only once. Naive loop with erase:

```cpp
// BAD: O(n²) due to repeated shifts
for(auto it = v.begin(); it != v.end(); ) {
    if(*it % 2 == 0) {
        it = v.erase(it);  // Shifts all elements after it
    } else {
        ++it;
    }
}
```

Each erase() shifts all subsequent elements left (O(n)). With n/2 even numbers, total is O(n²).

With erase-remove idiom: remove_if() does one pass moving elements (O(n)), erase() does one deletion at end (O(n)), total O(n).

Alternative for list: list::remove_if() member function actually erases because list knows its own structure:

```cpp
list<int> l = {1, 2, 3, 4, 5, 6, 7, 8};
l.remove_if([](int x) { return x % 2 == 0; });  // Actually erases

```

---

### Q2: Compare functors and lambda expressions. When would you use each? What are the advantages of functors over lambdas?

**Answer:**

Functors and lambdas both provide callable objects for algorithms, but differ in syntax, features, and use cases.

**Syntax comparison:**

Functor:

```cpp
struct IsEven {
    bool operator()(int x) const {
        return x % 2 == 0;
    }
};

count_if(v.begin(), v.end(), IsEven());
```

Lambda:

```cpp
count_if(v.begin(), v.end(), [](int x) { return x % 2 == 0; });
```

Lambda is more concise for simple operations.

**Functor advantages:**

1. State management:

```cpp
struct Counter {
    int count = 0;
    bool operator()(int x) {
        count++;
        return x % 2 == 0;
    }
    int getCount() const { return count; }
};

Counter counter;
count_if(v.begin(), v.end(), ref(counter));
cout << "Checked " << counter.getCount() << " elements" << endl;
```

1. Reusability across translation units:

```cpp
// In header file
struct Comparator {
    bool operator()(int a, int b) const { return a > b; }
};

// Can use in multiple files
sort(v.begin(), v.end(), Comparator());
```

1. Named type for template parameters:

```cpp
template<typename Comparator>
void customSort(vector<int>& v) {
    sort(v.begin(), v.end(), Comparator());
}

customSort<Comparator>(v);  // Clear intent
```

1. Member functions and overloading:

```cpp
struct Processor {
    void operator()(int x) { processInt(x); }
    void operator()(string s) { processString(s); }

    void reset() { /* ... */ }

private:
    void processInt(int x) { /* ... */ }
    void processString(string s) { /* ... */ }
};
```

**Lambda advantages:**

1. Inline definition - no separate class:

```cpp
// No need for separate functor class
sort(v.begin(), v.end(), [](int a, int b) { return a > b; });
```

1. Capture surrounding scope:

```cpp
int threshold = 5;
auto pred = [threshold](int x) { return x > threshold; };
// Threshold automatically captured
```

1. Less boilerplate for one-time use:

```cpp
// Functor requires class definition
// Lambda is one line
```

**When to use each:**

Use lambdas when:

- Operation is simple and used once
- Need to capture local variables
- Code clarity benefits from inline definition
- Working within single function scope

Use functors when:

- Complex logic requiring multiple methods
- Need to reuse across multiple files
- Require explicit state management and access
- Performance-critical (functors can be easier to optimize)
- Teaching/documentation (explicit class is clearer)

**Performance:** Both are typically inlined by compiler with zero overhead. Lambdas might generate slightly more code for complex captures, but difference is negligible in practice.

**Modern C++ trend:** Prefer lambdas for simplicity, use functors when lambda limitations become apparent (need named type, complex state, reuse across files).

Example combining both:

```cpp
// Functor for reusable comparison
struct ByAge {
    bool operator()(const Person& a, const Person& b) const {
        return a.age < b.age;
    }
};

// Lambda for one-time filtering
int min_age = 21;
auto adults = [min_age](const Person& p) { return p.age >= min_age; };

vector<Person> people = /* ... */;
sort(people.begin(), people.end(), ByAge());  // Reusable comparator
people.erase(remove_if(people.begin(), people.end(), adults), people.end());
```

---

### Q3: Explain lambda capture modes. What's the difference between capture by value [=] and by reference [&]? When would each be dangerous?

**Answer:**

Lambda captures allow accessing variables from surrounding scope inside lambda body. The capture mode determines how variables are captured and their lifetime/mutability.

**Capture by value [=]:**

Creates copies of captured variables at lambda creation time.

```cpp
int x = 5;
auto f = [=]() { return x * 2; };  // x copied
x = 10;
cout << f() << endl;  // 10 (original copy was 5)
```

Characteristics:

- Immutable by default (must use mutable keyword to modify)
- Safe from external changes
- Safe from dangling references
- Cost: Copies all captured variables

Danger - large objects:

```cpp
vector<int> huge(1000000);
auto f = [=]() { return huge.size(); };  // Copies entire vector!

// Better:
auto f = [&huge]() { return huge.size(); };  // Reference, no copy
```

**Capture by reference [&]:**

Creates references to original variables.

```cpp
int x = 5;
auto f = [&]() { return x * 2; };  // x by reference
x = 10;
cout << f() << endl;  // 20 (sees updated x)
```

Characteristics:

- Can modify original variable
- Reflects external changes
- No copy cost
- Danger: Dangling references if original dies

Danger - lifetime issues:

```cpp
function<int()> makeCounter() {
    int count = 0;
    return [&]() { return ++count; };  // DANGER! count dies
}

auto counter = makeCounter();
counter();  // Undefined behavior - count is dead
```

Fix with capture by value:

```cpp
function<int()> makeCounter() {
    int count = 0;
    return [count]() mutable { return ++count; };  // OK - copy lives
}
```

**Specific captures:**

Mix and match for fine control:

```cpp
int x = 1, y = 2, z = 3;

[x, &y]  // x by value, y by reference
[=, &y]  // All by value except y by reference
[&, x]   // All by reference except x by value

auto f = [x, &y]() {
    // x cannot be modified (copy)
    // y can be modified (reference to original)
    y = x + 1;
};
```

**Mutable keyword:**

Allows modifying captured-by-value variables:

```cpp
int x = 0;
auto f = [x]() mutable {
    x++;  // OK with mutable
    return x;
};

cout << f() << endl;  // 1
cout << f() << endl;  // 2
cout << x << endl;    // 0 (original unchanged)
```

Without mutable, captured-by-value is const inside lambda.

**Best practices:**

1. Default to specific captures for clarity:

```cpp
// ✅ GOOD - explicit
[threshold](int x) { return x > threshold; }

// ❌ LESS CLEAR - what's captured?
[=](int x) { return x > threshold; }
```

1. Use reference for large objects:

```cpp
vector<int> large(1000000);
// ✅ GOOD
for_each(v.begin(), v.end(), [&large](int x) { large.push_back(x); });
```

1. Avoid capturing by reference in returned lambdas:

```cpp
// ❌ BAD
function<int()> makeFunc() {
    int x = 5;
    return [&x]() { return x; };  // x dies!
}

// ✅ GOOD
function<int()> makeFunc() {
    int x = 5;
    return [x]() { return x; };  // Copy safe
}
```

1. Consider move capture for unique_ptr (C++14):

```cpp
auto ptr = make_unique<int>(42);
auto f = [p = move(ptr)]() { return *p; };  // Move capture
```

---

### Q4: Why can't you use std::sort on a list? What's the relationship between algorithm requirements and iterator categories?

**Answer:**

std::sort requires Random Access iterators but list only provides Bidirectional iterators, causing a compile error.

**Why sort needs Random Access:**

std::sort uses introsort (quicksort + heapsort + insertion sort hybrid). These algorithms fundamentally need:

1. Jump to arbitrary positions for pivot selection
2. Swap non-adjacent elements
3. Compare elements at distance
4. Access middle element in O(1)

With Random Access iterators (vector, deque, array):

```cpp
auto mid = begin + (end - begin) / 2;  // O(1)
swap(*begin, *(end - 1));              // O(1)
```

With Bidirectional iterators (list):

```cpp
auto mid = begin;
advance(mid, distance(begin, end) / 2);  // O(n)!
```

Every pivot access becomes O(n) instead of O(1), making O(n log n) algorithm become O(n² log n) - completely inefficient.

**Algorithm-Iterator contract:**

STL algorithms declare minimum iterator category:

```cpp
// Algorithm signatures (simplified)
template<typename InputIterator>
void find(InputIterator first, InputIterator last);  // Needs Input

template<typename BidirectionalIterator>
void reverse(BidirectionalIterator first, BidirectionalIterator last);  // Needs Bidirectional

template<typename RandomAccessIterator>
void sort(RandomAccessIterator first, RandomAccessIterator last);  // Needs Random Access
```

This creates compile-time checks:

```cpp
list<int> l = {3, 1, 4};
sort(l.begin(), l.end());  // Compile error!
// Error: sort requires random access, list::iterator is bidirectional
```

**Why list has its own sort:**

list::sort() member function uses merge sort optimized for linked lists:

```cpp
list<int> l = {3, 1, 4, 1, 5};
l.sort();  // OK - member function
```

Why merge sort for lists:

- Only needs sequential access (perfect for linked lists)
- Sorts by relinking nodes (no element moves)
- O(n log n) with linked list structure
- Doesn't require random access

**Iterator category hierarchy:**

```
Input ────┐
          ├──→ Forward ──→ Bidirectional ──→ Random Access
Output ───┘
```

Higher categories can do everything lower categories can:

- Random Access can do Bidirectional operations
- Bidirectional can do Forward operations
- Forward can do Input operations

**Algorithm adaptation:**

Some algorithms adapt to iterator category:

```cpp
template<typename Iterator>
void advance(Iterator& it, int n) {
    // For Random Access: O(1)
    if constexpr (is_random_access_iterator<Iterator>)
        it += n;
    // For Bidirectional: O(n)
    else if constexpr (is_bidirectional_iterator<Iterator>)
        while(n--) ++it;
    // For Forward: O(n), only forward
    else
        while(n--) ++it;
}
```

This is why distance() is O(1) for vector but O(n) for list.

**Practical implications:**

1. Container choice affects algorithm availability:

```cpp
// ✅ Works - vector has random access
vector<int> v = {3, 1, 4};
sort(v.begin(), v.end());

// ❌ Doesn't work - list lacks random access
list<int> l = {3, 1, 4};
sort(l.begin(), l.end());  // Compile error

// ✅ Use member function instead
l.sort();
```

1. Choose container based on algorithms needed:
- Need binary_search? Use vector (or sort into vector)
- Need splice? Use list
- Need both? Maybe use vector and sort when needed
1. Algorithm documentation specifies iterator requirements:
- Read it before use
- Compiler enforces at compile-time
- Prevents runtime inefficiency

**Why this design is good:**

Static polymorphism catches mismatches at compile-time rather than silently providing O(n²) algorithm when you expected O(n log n). The compiler error forces you to either:

- Choose appropriate container (vector instead of list)
- Use container-specific algorithm (list::sort())
- Understand performance implications

This is example of C++'s "you don't pay for what you don't use" - algorithms are maximally efficient for their iterator category, and compiler prevents using them with incompatible containers.

---

### Q5: Explain std::accumulate. How would you use it to calculate product instead of sum? How would you concatenate strings?

**Answer:**

std::accumulate is a fold operation that reduces a range to a single value using a binary operation. Header: `<numeric>`

**Basic usage - sum:**

```cpp
vector<int> v = {1, 2, 3, 4, 5};
int sum = accumulate(v.begin(), v.end(), 0);  // 15
```

Signature: `accumulate(begin, end, initial_value, [binary_operation])`

How it works:

1. Start with initial_value
2. For each element: result = binary_op(result, element)
3. Return final result

Default operation is addition (operator+).

**Calculate product:**

Change binary operation from + to *:

```cpp
vector<int> v = {1, 2, 3, 4, 5};
int product = accumulate(v.begin(), v.end(), 1,  // Note: 1 not 0!
                         [](int a, int b) { return a * b; });
// 1 * 1 * 2 * 3 * 4 * 5 = 120
```

Critical: Initial value is 1, not 0 (0 would make everything 0).

Using predefined functor:

```cpp
int product = accumulate(v.begin(), v.end(), 1, multiplies<int>());
```

**Concatenate strings:**

```cpp
vector<string> words = {"Hello", " ", "World", "!"};
string sentence = accumulate(words.begin(), words.end(), string(""));
// "" + "Hello" + " " + "World" + "!" = "Hello World!"
```

Why string("") not ""?

- "" is const char*, doesn't have operator+
- string("") creates actual string object with operator+

**Custom operations:**

Find maximum:

```cpp
vector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};
int max_val = accumulate(v.begin(), v.end(), v[0],
                         [](int a, int b) { return max(a, b); });
```

Count elements matching condition:

```cpp
vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};
int even_count = accumulate(v.begin(), v.end(), 0,
                            [](int count, int x) {
                                return count + (x % 2 == 0 ? 1 : 0);
                            });
```

**Complex example - sum of squares:**

```cpp
vector<int> v = {1, 2, 3, 4, 5};
int sum_of_squares = accumulate(v.begin(), v.end(), 0,
                                [](int sum, int x) {
                                    return sum + x * x;
                                });
// 0 + 1 + 4 + 9 + 16 + 25 = 55
```

**Performance consideration:**

For strings, accumulate can be inefficient:

```cpp
// ❌ O(n²) - creates new string each concatenation
string result = accumulate(words.begin(), words.end(), string(""));

// ✅ O(n) - single allocation
string result;
for(const auto& word : words) result += word;

// ✅ Also O(n) - ostringstream
ostringstream oss;
for(const auto& word : words) oss << word;
string result = oss.str();
```

Each accumulate iteration creates temporary string, copying all previous content.

**Parallel version (C++17):**

```cpp
#include <execution>
int sum = reduce(execution::par, v.begin(), v.end(), 0);
```

reduce is parallel-friendly accumulate (order-independent operations only).

**When to use accumulate:**

✅ Use when:

- Reducing range to single value
- Operation is associative
- Code clarity from functional style

❌ Don't use when:

- Simple sum (use sum = 0; for(x : v) sum += x; for clarity)
- String concatenation (inefficient)
- Complex accumulation logic (explicit loop more readable)

**Alternative: fold expressions (C++17):**

```cpp
template<typename... Args>
auto sum(Args... args) {
    return (args + ...);  // Fold expression
}

int result = sum(1, 2, 3, 4, 5);  // 15
```

But only works with parameter packs, not runtime containers.

accumulate remains the standard way to fold over runtime containers in C++.