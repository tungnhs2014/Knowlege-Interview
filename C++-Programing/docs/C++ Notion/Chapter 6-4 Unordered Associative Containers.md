# 6.4. Unordered Associative Containers

---

## Table of Contents

1. Unordered Containers Overview
2. unordered_set
3. unordered_map
4. Hash Functions
5. Performance and Best Practices
6. Summary
7. Interview Preparation

---

## 1. Unordered Containers Overview

### 1.1 What are Unordered Containers?

**Unordered containers use hash tables to provide O(1) average-time insert, find, and erase operations.**

Header: `#include <unordered_set>`, `#include <unordered_map>`

**Why They Exist:**

Ordered containers (set/map) use Red-Black Trees with O(log n) operations. For large datasets, O(1) average-time can be significantly faster than O(log n).

```
Ordered (set/map):          Unordered (unordered_set/map):
Red-Black Tree              Hash Table

      50                    Buckets:
     /  \                   [0]: → 10 → 20
   30    70                 [1]: → 31
  /  \   /  \               [2]: → 50
 10  40 60  80              [3]: → 73
                            [4]: → 40
O(log n) operations         O(1) average operations
```

### 1.2 Ordered vs Unordered Comparison

| Feature | set/map | unordered_set/map |
| --- | --- | --- |
| Implementation | Red-Black Tree | Hash Table |
| Insert/Find/Erase | O(log n) guaranteed | O(1) average, O(n) worst |
| Iteration order | Sorted | No order |
| Memory overhead | Moderate | Higher (buckets) |
| Use when | Need sorted order | Just need fast lookup |

### 1.3 The Four Unordered Containers

| Container | Description | Allows Duplicates? |
| --- | --- | --- |
| **unordered_set** | Unique elements | No |
| **unordered_multiset** | Elements | Yes |
| **unordered_map** | Key-value pairs | No (keys unique) |
| **unordered_multimap** | Key-value pairs | Yes (duplicate keys) |

---

## 2. unordered_set

### 2.1 Basic Operations

```cpp
#include <iostream>
#include <unordered_set>
using namespace std;

int main() {
    // WHY: O(1) average operations, no sorting overhead
    unordered_set<int> us;

    // Insert - O(1) average
    us.insert(30);
    us.insert(10);
    us.insert(50);
    us.insert(20);
    us.insert(30);  // Duplicate - ignored

    cout << "Size: " << us.size() << endl;  // 4

    // NO guaranteed order
    cout << "Elements: ";
    for(int x : us) cout << x << " ";
    cout << endl;

    // Find - O(1) average
    if(us.find(20) != us.end()) {
        cout << "20 found" << endl;
    }

    // Count - O(1) average
    cout << "Count of 30: " << us.count(30) << endl;

    // Erase - O(1) average
    us.erase(30);
    cout << "After erase: ";
    for(int x : us) cout << x << " ";

    return 0;
}
```

**Output (order varies):**

```
Size: 4
Elements: 20 50 10 30
20 found
Count of 30: 1
After erase: 20 50 10
```

### 2.2 unordered_set vs set

```cpp
#include <iostream>
#include <set>
#include <unordered_set>
#include <chrono>
using namespace std;

int main() {
    const int N = 100000;

    // WHY: Compare performance

    // set - O(log n) operations
    auto start = chrono::high_resolution_clock::now();
    set<int> s;
    for(int i = 0; i < N; i++) {
        s.insert(i);
    }
    for(int i = 0; i < N; i++) {
        s.find(i);
    }
    auto end = chrono::high_resolution_clock::now();
    auto set_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    // unordered_set - O(1) average operations
    start = chrono::high_resolution_clock::now();
    unordered_set<int> us;
    for(int i = 0; i < N; i++) {
        us.insert(i);
    }
    for(int i = 0; i < N; i++) {
        us.find(i);
    }
    end = chrono::high_resolution_clock::now();
    auto unordered_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "set time: " << set_time << "ms" << endl;
    cout << "unordered_set time: " << unordered_time << "ms" << endl;
    cout << "Speedup: " << (double)set_time / unordered_time << "x" << endl;

    return 0;
}
```

**Output (typical):**

```
set time: 45ms
unordered_set time: 15ms
Speedup: 3x
```

### 2.3 When to Use unordered_set

**✅ Use unordered_set When:**

- Only need to check existence (no sorting)
- Performance critical (large datasets)
- Insert/find operations dominate
- No need for range queries

**❌ Use set When:**

- Need elements in sorted order
- Need range queries (lower_bound, upper_bound)
- Worst-case guarantees required
- Hash function unavailable for key type

---

## 3. unordered_map

### 3.1 Basic Operations

```cpp
#include <iostream>
#include <unordered_map>
#include <string>
using namespace std;

int main() {
    // WHY: O(1) average lookup by key
    unordered_map<string, int> ages;

    // Insert - O(1) average
    ages["Alice"] = 25;
    ages["Bob"] = 30;
    ages["Charlie"] = 28;
    ages.insert({"David", 35});

    cout << "Size: " << ages.size() << endl;

    // Access - O(1) average
    cout << "Alice's age: " << ages["Alice"] << endl;

    // Find - O(1) average
    auto it = ages.find("Bob");
    if(it != ages.end()) {
        cout << "Bob's age: " << it->second << endl;
    }

    // NO guaranteed order
    cout << "\nAll entries (random order):\n";
    for(auto& pair : ages) {
        cout << pair.first << ": " << pair.second << endl;
    }

    return 0;
}
```

**Output (order varies):**

```
Size: 4
Alice's age: 25
Bob's age: 30

All entries (random order):
David: 35
Bob: 30
Alice: 25
Charlie: 28
```

### 3.2 Real-World Use Case: Cache

```cpp
#include <iostream>
#include <unordered_map>
#include <string>
using namespace std;

class Cache {
    unordered_map<string, string> data;

public:
    // WHY: O(1) cache lookup is critical for performance
    void put(string key, string value) {
        data[key] = value;
        cout << "Cached: " << key << endl;
    }

    bool get(string key, string& value) {
        auto it = data.find(key);
        if(it != data.end()) {
            value = it->second;
            return true;
        }
        return false;
    }
};

int main() {
    Cache cache;

    cache.put("user:1", "Alice");
    cache.put("user:2", "Bob");

    string value;
    if(cache.get("user:1", value)) {
        cout << "Cache hit: " << value << endl;
    }

    if(!cache.get("user:3", value)) {
        cout << "Cache miss: user:3" << endl;
    }

    return 0;
}
```

**Output:**

```
Cached: user:1
Cached: user:2
Cache hit: Alice
Cache miss: user:3
```

### 3.3 unordered_map vs map Performance

```cpp
#include <iostream>
#include <map>
#include <unordered_map>
#include <string>
#include <chrono>
using namespace std;

int main() {
    const int N = 100000;

    // Generate keys
    vector<string> keys;
    for(int i = 0; i < N; i++) {
        keys.push_back("key" + to_string(i));
    }

    // map - O(log n)
    auto start = chrono::high_resolution_clock::now();
    map<string, int> m;
    for(int i = 0; i < N; i++) {
        m[keys[i]] = i;
    }
    for(int i = 0; i < N; i++) {
        auto it = m.find(keys[i]);
    }
    auto end = chrono::high_resolution_clock::now();
    auto map_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    // unordered_map - O(1) average
    start = chrono::high_resolution_clock::now();
    unordered_map<string, int> um;
    for(int i = 0; i < N; i++) {
        um[keys[i]] = i;
    }
    for(int i = 0; i < N; i++) {
        auto it = um.find(keys[i]);
    }
    end = chrono::high_resolution_clock::now();
    auto unordered_time = chrono::duration_cast<chrono::milliseconds>(end - start).count();

    cout << "map time: " << map_time << "ms" << endl;
    cout << "unordered_map time: " << unordered_time << "ms" << endl;
    cout << "Speedup: " << (double)map_time / unordered_time << "x" << endl;

    return 0;
}
```

**Output (typical):**

```
map time: 85ms
unordered_map time: 25ms
Speedup: 3.4x
```

---

## 4. Hash Functions

### 4.1 How Hash Tables Work

**Hash Function:** Converts key to integer (hash code), then to bucket index.

```
Process:
1. Key → Hash Function → Hash Code (large integer)
2. Hash Code % Bucket_Count → Bucket Index (0 to n-1)
3. Store element in that bucket

Example:
Key: "Alice"
Hash("Alice") = 372894621 (hash code)
372894621 % 10 = 1 (bucket index)
Store in bucket 1
```

**Collision:** When two keys hash to same bucket.

```
Hash("Alice") % 10 = 1
Hash("Andrew") % 10 = 1  ← Collision!

Solution: Chaining
Bucket 1: → Alice → Andrew
```

### 4.2 Built-in Hash Functions

```cpp
#include <iostream>
#include <unordered_map>
#include <string>
#include <functional>
using namespace std;

int main() {
    // WHY: C++ provides hash functions for standard types

    hash<int> int_hasher;
    hash<string> string_hasher;
    hash<double> double_hasher;

    cout << "Hash of 42: " << int_hasher(42) << endl;
    cout << "Hash of \"hello\": " << string_hasher("hello") << endl;
    cout << "Hash of 3.14: " << double_hasher(3.14) << endl;

    // Same input → same hash
    cout << "\nConsistency check:" << endl;
    cout << "Hash of 42 again: " << int_hasher(42) << endl;

    return 0;
}
```

**Output:**

```
Hash of 42: 42
Hash of "hello": 15276750567035658801
Hash of 3.14: 4614256656447570610

Consistency check:
Hash of 42 again: 42
```

### 4.3 Custom Hash for Struct

```cpp
#include <iostream>
#include <unordered_set>
#include <string>
using namespace std;

struct Person {
    string name;
    int age;

    bool operator==(const Person& other) const {
        return name == other.name && age == other.age;
    }
};

// WHY: Custom hash function for custom types
struct PersonHash {
    size_t operator()(const Person& p) const {
        // Combine hashes of member variables
        return hash<string>()(p.name) ^ (hash<int>()(p.age) << 1);
    }
};

int main() {
    unordered_set<Person, PersonHash> people;

    people.insert({"Alice", 25});
    people.insert({"Bob", 30});
    people.insert({"Alice", 25});  // Duplicate - ignored

    cout << "Number of unique people: " << people.size() << endl;

    Person search = {"Alice", 25};
    if(people.find(search) != people.end()) {
        cout << "Alice found" << endl;
    }

    return 0;
}
```

**Output:**

```
Number of unique people: 2
Alice found
```

### 4.4 Hash Function Requirements

**Good hash function properties:**

1. **Deterministic:** Same input always produces same hash
2. **Uniform distribution:** Spreads keys evenly across buckets
3. **Fast to compute:** O(1) computation time
4. **Avalanche effect:** Small change in input → large change in hash

**Bad hash example:**

```cpp
// BAD: All strings hash to same value
struct BadHash {
    size_t operator()(const string& s) const {
        return 42;  // All collisions!
    }
};

// GOOD: Uses built-in string hash
unordered_set<string, hash<string>> good_set;

// BAD: Causes all collisions
unordered_set<string, BadHash> bad_set;
```

### 4.5 Load Factor and Rehashing

**Load Factor = Number of Elements / Number of Buckets**

```cpp
#include <iostream>
#include <unordered_set>
using namespace std;

int main() {
    unordered_set<int> us;

    cout << "Initial state:\n";
    cout << "Buckets: " << us.bucket_count() << endl;
    cout << "Load factor: " << us.load_factor() << endl;
    cout << "Max load factor: " << us.max_load_factor() << endl;

    // WHY: Watch rehashing as load factor increases
    for(int i = 0; i < 100; i++) {
        us.insert(i);
        if(i % 20 == 0) {
            cout << "\nAfter " << i << " insertions:\n";
            cout << "Buckets: " << us.bucket_count() << endl;
            cout << "Load factor: " << us.load_factor() << endl;
        }
    }

    return 0;
}
```

**Output (typical):**

```
Initial state:
Buckets: 8
Load factor: 0
Max load factor: 1

After 0 insertions:
Buckets: 8
Load factor: 0.125

After 20 insertions:
Buckets: 23
Load factor: 0.913043

After 40 insertions:
Buckets: 53
Load factor: 0.773585
```

**Rehashing:** When load factor exceeds max_load_factor (default 1.0), the hash table automatically increases bucket count and redistributes all elements. This is expensive (O(n)) but rare.

---

## 5. Performance and Best Practices

### 5.1 Time Complexity Summary

| Operation | Average | Worst Case |
| --- | --- | --- |
| Insert | O(1) | O(n) |
| Find | O(1) | O(n) |
| Erase | O(1) | O(n) |
| Rehash | - | O(n) |

**Worst case happens when:**

- All elements hash to same bucket (poor hash function)
- Rehashing required (load factor exceeded)

### 5.2 When to Use Unordered vs Ordered

```cpp
// Scenario 1: Existence checking
// ✅ Use unordered_set - 3x faster
unordered_set<int> visited;
if(visited.find(node) != visited.end()) { }

// Scenario 2: Need sorted iteration
// ✅ Use set - maintains order
set<int> sorted_data;
for(int x : sorted_data) { }  // Ascending order

// Scenario 3: Range queries
// ✅ Use set - has lower_bound/upper_bound
set<int> s;
auto it = s.lower_bound(50);  // First element >= 50

// Scenario 4: Frequency counting
// ✅ Use unordered_map - faster lookups
unordered_map<string, int> freq;
freq[word]++;

// Scenario 5: Real-time system
// ✅ Use set - guaranteed O(log n)
set<int> realtime;  // No worst-case O(n)
```

### 5.3 Memory Considerations

```cpp
#include <iostream>
#include <set>
#include <unordered_set>
using namespace std;

int main() {
    const int N = 10000;

    set<int> s;
    unordered_set<int> us;

    for(int i = 0; i < N; i++) {
        s.insert(i);
        us.insert(i);
    }

    // WHY: unordered containers use more memory
    cout << "set elements: " << s.size() << endl;
    cout << "unordered_set elements: " << us.size() << endl;
    cout << "unordered_set buckets: " << us.bucket_count() << endl;

    // unordered_set has bucket overhead
    // Rough memory: elements + buckets

    return 0;
}
```

**Output:**

```
set elements: 10000
unordered_set elements: 10000
unordered_set buckets: 12289
```

**Memory comparison:**

- set: ~48 bytes per element (pointers + color bit)
- unordered_set: ~8 bytes per element + bucket array overhead

### 5.4 Reserve for Known Size

```cpp
#include <iostream>
#include <unordered_set>
using namespace std;

int main() {
    const int N = 100000;

    // WHY: Reserve to avoid multiple rehashing
    unordered_set<int> us;
    us.reserve(N);  // Pre-allocate buckets

    cout << "Reserved buckets: " << us.bucket_count() << endl;

    // Now insertions won't trigger rehashing
    for(int i = 0; i < N; i++) {
        us.insert(i);
    }

    cout << "After insertions, buckets: " << us.bucket_count() << endl;

    return 0;
}
```

**Output:**

```
Reserved buckets: 100003
After insertions, buckets: 100003
```

### 5.5 Best Practices Summary

**✅ DO:**

- Use unordered containers for pure existence/lookup
- Reserve capacity if size known
- Use good hash functions (built-in for standard types)
- Profile before optimizing

**❌ DON'T:**

- Use unordered when you need sorted order
- Assume O(1) is always faster than O(log n)
- Use with types lacking good hash functions
- Rely on iteration order

---

## Summary

### Key Takeaways

1. **Unordered containers use hash tables for O(1) average operations** versus O(log n) for ordered containers. This provides 2-4x speedup for large datasets when insertion and lookup dominate, but comes with higher memory overhead for bucket arrays.
2. **Hash functions must be deterministic and provide uniform distribution** to avoid collisions. C++ provides built-in hash functions for standard types (int, string, etc.) but custom types require custom hash functions and equality operators.
3. **Load factor triggers automatic rehashing** when it exceeds max_load_factor (default 1.0). Rehashing is expensive (O(n)) but infrequent. Use reserve() when final size is known to avoid multiple rehashes.
4. **Worst case is O(n) when all elements collide** in same bucket due to poor hash function. This is why real-time systems often prefer set/map with guaranteed O(log n) over unordered containers' average O(1) but worst O(n).
5. **unordered_set/map trade memory for speed** using more memory than set/map due to bucket arrays, but provide faster operations when hash function is good. Memory overhead is bucket_count * pointer_size.
6. **Choose unordered when you only need existence checking or lookups** without caring about order. Choose ordered containers when you need sorted iteration, range queries (lower_bound/upper_bound), or guaranteed performance.
7. **Custom types need operator== and hash function** to work with unordered containers. Combine member hashes using XOR and bit shifts: `hash<T1>()(a) ^ (hash<T2>()(b) << 1)`.
8. **unordered_map's operator[] creates entries** just like map, while at() throws exceptions. Always check with find() or count() before using operator[] to avoid unintended insertions.
9. **Profile before optimizing** - O(log n) with good cache locality can beat O(1) average with poor cache behavior for small datasets. Modern CPUs heavily optimize sequential access, benefiting set/map's tree structure.
10. **multiset/multimap variants allow duplicates** while maintaining same performance characteristics. Use unordered_multiset when you need duplicate tracking without caring about order.

---

## Interview Preparation

### Q1: Explain how hash tables work. What happens when two keys hash to the same bucket? How does rehashing work?

**Answer:**

Hash tables store elements in an array of buckets using a hash function to determine which bucket stores each element. The process has three steps:

1. Hash function converts key to large integer (hash code)
2. Hash code modulo bucket count gives bucket index (0 to n-1)
3. Element stored in that bucket

Example: If hash("Alice") = 372894621 and we have 10 buckets, then 372894621 % 10 = 1, so "Alice" goes in bucket 1.

When two keys hash to the same bucket (collision), C++ unordered containers use chaining - each bucket is actually a linked list. Multiple elements in same bucket are stored as nodes in the list. Finding an element in a bucket with k elements requires O(k) time - this is why worst case is O(n) if all elements collide into one bucket.

Good hash functions minimize collisions by distributing keys uniformly across buckets. Properties of good hash functions: deterministic (same input always produces same output), uniform distribution, fast computation (O(1)), and avalanche effect (small input change causes large hash change).

Rehashing occurs when load factor (elements / buckets) exceeds max_load_factor (default 1.0). The process:

1. Allocate new bucket array (typically double current size)
2. Recompute hash for every element with new bucket count
3. Move all elements to new buckets
4. Delete old bucket array

This is expensive (O(n)) but infrequent. With geometric growth (doubling), inserting n elements requires O(n) total rehashing time, making it O(1) amortized per insertion.

You can avoid rehashing by calling reserve(n) before insertions if you know the final size - this allocates enough buckets upfront so load factor won't exceed threshold during insertions.

---

### Q2: When would you use unordered_map instead of map? What are the trade-offs? Give specific examples.

**Answer:**

Use unordered_map when you only need fast key-based lookup without caring about key order. Use map when you need sorted keys or range queries.

unordered_map advantages:

- O(1) average insert/find/erase vs map's O(log n)
- Typically 2-4x faster for large datasets
- Better for pure lookup workloads

unordered_map disadvantages:

- O(n) worst case if hash function is poor
- No guaranteed order (can't iterate sorted)
- No range queries (no lower_bound/upper_bound)
- Higher memory overhead (bucket arrays)
- Requires good hash function for key type

Specific examples:

Use unordered_map:

```cpp
// 1. Caching - speed critical, no order needed
unordered_map<string, Response> cache;

// 2. Word frequency counting
unordered_map<string, int> wordCount;
for(string word : words) wordCount[word]++;

// 3. Database indexing - ID to record
unordered_map<int, User> userDB;
User u = userDB[userId];  // O(1) average
```

Use map:

```cpp
// 1. Leaderboard - need sorted order
map<int, string> scores;  // Sorted by score
for(auto& [score, name] : scores) { }  // Ascending order

// 2. Range queries - find all in range
map<int, Item> items;
auto it = items.lower_bound(100);  // Items with key >= 100

// 3. Real-time systems - need guaranteed performance
map<Event, Handler> handlers;  // O(log n) guaranteed, not O(n) worst case
```

Performance comparison: For 1 million string keys, unordered_map insert+find takes ~25ms while map takes ~85ms (3.4x speedup). But for 100 keys, map might be faster due to better cache locality despite higher complexity.

Rule of thumb: Start with unordered_map for performance. Switch to map only if you need sorted iteration, range queries, or guaranteed worst-case performance. Profile your specific use case - small datasets or poor hash functions can make map faster in practice.

---

### Q3: How would you create a custom hash function for a struct with multiple members? What's the importance of operator==?

**Answer:**

To use a custom struct with unordered containers, you must provide both a hash function and operator==. The hash function determines which bucket stores the element, while operator== resolves collisions within that bucket.

Example with Person struct:

```cpp
struct Person {
    string name;
    int age;
    string city;

    // WHY: operator== required to distinguish elements in same bucket
    bool operator==(const Person& other) const {
        return name == other.name &&
               age == other.age &&
               city == other.city;
    }
};

// WHY: Hash function determines bucket placement
struct PersonHash {
    size_t operator()(const Person& p) const {
        // Combine member hashes using XOR and bit shifts
        size_t h1 = hash<string>()(p.name);
        size_t h2 = hash<int>()(p.age);
        size_t h3 = hash<string>()(p.city);

        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

unordered_set<Person, PersonHash> people;
```

Why operator== is critical: When you search for an element, the hash function computes the bucket. But that bucket might contain multiple elements (collisions). operator== is used to find the exact element within the bucket by comparing each element in the bucket's linked list.

If you define hash but not operator==, compilation fails because the container can't determine equality. If you define operator== but not hash, compilation also fails because the container doesn't know how to bucket elements.

Best practices for combining hashes:

- XOR (^) combines hash bits
- Bit shifts (<<) prevent similar values from canceling out in XOR
- Use different shift amounts for different members
- For more complex combinations, consider boost::hash_combine

Alternative using std::hash specialization:

```cpp
namespace std {
    template<>
    struct hash<Person> {
        size_t operator()(const Person& p) const {
            return hash<string>()(p.name) ^
                   (hash<int>()(p.age) << 1) ^
                   (hash<string>()(p.city) << 2);
        }
    };
}

// Now can use without explicit hash template parameter
unordered_set<Person> people;
```

Common mistake: Defining operator== but forgetting hash, or vice versa. Both are required because they serve different purposes in the hash table's two-level lookup process.

---

### Q4: Explain load factor and rehashing. How would you optimize performance when you know the final size of your unordered container?

**Answer:**

Load factor is the ratio of elements to buckets (elements / buckets). It measures how full the hash table is. Higher load factor means more collisions and slower operations. Lower load factor means wasted space but fewer collisions.

C++ unordered containers maintain max_load_factor (default 1.0). When inserting an element would exceed this threshold, automatic rehashing occurs:

1. Allocate new bucket array (typically double size)
2. For each existing element:
    - Compute new bucket index with new bucket count
    - Move element to new bucket
3. Delete old bucket array

This rehashing is O(n) and expensive, but infrequent due to geometric growth. With doubling strategy, inserting n elements triggers log(n) rehashes with total cost O(n), making it O(1) amortized per insertion.

Example showing rehashing:

```cpp
unordered_set<int> us;
cout << "Buckets: " << us.bucket_count() << endl;  // 8

for(int i = 0; i < 100; i++) {
    us.insert(i);
    // Rehashing happens around i=8, 16, 32, 64
}

cout << "Final buckets: " << us.bucket_count() << endl;  // ~200
// Had to rehash ~5 times to reach 200 buckets
```

Performance optimization when size is known:

Use reserve(n) before insertions to pre-allocate buckets:

```cpp
const int N = 1000000;

// BAD: Multiple expensive rehashes
unordered_set<int> bad;
for(int i = 0; i < N; i++) {
    bad.insert(i);  // Triggers ~20 rehashes
}

// GOOD: One allocation, no rehashing
unordered_set<int> good;
good.reserve(N);  // Allocates ~1000000 buckets
for(int i = 0; i < N; i++) {
    good.insert(i);  // No rehashing!
}
```

reserve(n) calculates required buckets as n / max_load_factor, so reserve(1000) with default max_load_factor of 1.0 allocates at least 1000 buckets.

Performance impact: For 1 million insertions, not reserving takes ~50ms while reserving takes ~25ms (2x speedup). The speedup increases with dataset size because rehashing cost is O(n) each time.

Additional optimization: If you know elements won't be added after initialization, you can use max_load_factor(higher_value) to reduce memory usage by allowing more collisions:

```cpp
us.max_load_factor(2.0);  // Allow twice as many elements per bucket
// Halves bucket count but doubles average chain length
```

This trades speed for memory - acceptable for read-heavy workloads after initialization completes.

---

### Q5: Compare the performance characteristics of vector with linear search vs unordered_set. When would each be faster in practice?

**Answer:**

Theoretically, unordered_set's O(1) average lookup beats vector's O(n) linear search. But practical performance depends on several factors: dataset size, access patterns, cache behavior, and CPU architecture.

Time complexity comparison:

- vector linear search: O(n)
- unordered_set search: O(1) average, O(n) worst

But modern CPUs are optimized for sequential memory access, giving vector a cache locality advantage. Cache lines load 64 bytes at once, so searching a vector of integers loads 16 elements per cache miss, while unordered_set's scattered nodes cause cache misses for each element.

Crossover point analysis:

For small datasets (<100 elements):

```cpp
vector<int> v = {1, 2, 3, ..., 50};
find(v.begin(), v.end(), 25);  // ~20ns with good cache locality

unordered_set<int> us = {1, 2, 3, ..., 50};
us.find(25);  // ~30ns due to hash computation + potential cache miss
```

Vector wins due to cache effects dominating asymptotic complexity.

For medium datasets (100-10,000 elements):

- vector: ~100-10,000ns (linear in size)
- unordered_set: ~30-50ns (constant)
Unordered_set starts winning around 200-300 elements.

For large datasets (>10,000 elements):

- vector: >10,000ns (grows linearly)
- unordered_set: ~30-50ns (stays constant)
Unordered_set is clear winner.

Other considerations:

Memory usage:

- vector: Exact size, no overhead
- unordered_set: ~2x overhead (buckets + pointers)

For memory-constrained systems, vector might be preferred even with worse complexity.

Access patterns matter:

```cpp
// Many searches - use unordered_set
for(int i = 0; i < 1000000; i++) {
    us.find(random_key);  // O(1) each = O(n) total
}

// Few searches - vector acceptable
for(int i = 0; i < 10; i++) {
    find(v.begin(), v.end(), key);  // O(n) each but only 10 times
}
```

Sorted vector with binary_search can be middle ground:

```cpp
vector<int> sorted_v = {1, 2, 3, ..., 10000};
sort(sorted_v.begin(), sorted_v.end());
binary_search(sorted_v.begin(), sorted_v.end(), key);  // O(log n)
```

Provides O(log n) search with vector's cache locality, beating unordered_set for datasets of 100-1000 elements in many benchmarks.

Decision framework:

- <100 elements: Use vector (cache locality wins)
- 100-1000 elements: Profile your use case (could go either way)
- 1000 elements + frequent searches: Use unordered_set
- Need sorted: Use set or sorted vector
- Memory critical: Use vector
- Real-time guarantees: Use set (no worst-case O(n))

Always profile your specific use case - theoretical complexity doesn't always predict real performance with modern CPU caches and branch predictors.