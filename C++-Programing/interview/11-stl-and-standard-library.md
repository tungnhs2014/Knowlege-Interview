# 11 - STL And Standard Library: Interview Pack

## How To Use This Pack

For each question:

1. give the short answer first;
2. explain the mechanism and tradeoff;
3. anchor the answer in C++ code or a Standard Library API;
4. connect it to production behavior or debugging;
5. name traps explicitly;
6. handle follow-up questions without changing the original claim.

The examples use C++17 unless marked otherwise. Some follow-ups mention C++20
APIs such as `std::span` or `contains()`.

## Beginner Questions

### 1. What are the main STL components, and how do they work together?

**Short answer**

The STL model is built from containers, iterators, algorithms, and callables.
Containers own elements, iterators describe ranges, algorithms operate on
ranges, and callables customize behavior.

**Deep explanation**

A container such as `std::vector<int>` manages a collection of elements. An
iterator is a position abstraction, usually used as a half-open range
`[begin, end)`. An algorithm such as `std::sort` or `std::find` works through
iterators instead of depending on one concrete container type. A callable such
as a lambda, predicate, comparator, or functor tells the algorithm how to test,
order, or transform values.

This separation is why the same `std::find` can search a `vector`, `list`, or
array-like range, while `std::sort` is limited to ranges with random-access
iterators.

**C++ code/API anchor**

```cpp
#include <algorithm>
#include <iostream>
#include <vector>

int main()
{
    std::vector<int> values{4, 1, 3, 2};

    std::sort(values.begin(), values.end()); // container + iterators + algorithm

    auto it = std::find_if(values.begin(), values.end(),
                           [](int x) { return x > 2; }); // callable

    if (it != values.end()) {
        std::cout << *it << '\n';
    }
}
```

**Production/debug angle**

When reviewing STL code, identify the range, the algorithm preconditions, and
the callable contract. Many bugs are not in the algorithm itself; they are in a
wrong range, invalid iterator, broken comparator, or container choice.

**Common traps**

- Saying STL is only "containers."
- Forgetting algorithms operate on ranges, not containers directly.
- Dereferencing `end()`.
- Assuming every container supports every algorithm efficiently.

**Follow-up questions**

- Why does `std::sort` not work on `std::list` iterators?
- What does the range `[begin, end)` mean?
- When is a handwritten loop clearer than an algorithm?

### 2. Why is `std::vector` often the default sequence container?

**Short answer**

`std::vector` is usually the default because it stores elements contiguously,
supports O(1) random access, has amortized O(1) append, works with C APIs via
`data()`, and has excellent cache locality.

**Deep explanation**

`vector` behaves like a dynamic array. Its storage can grow, but growth may
reallocate and move/copy elements into a new contiguous block. That
reallocation cost is why `push_back` is amortized O(1), not always a single
constant-time write.

Contiguous storage is a big practical advantage. Modern CPUs are very good at
walking memory sequentially. A `list` may have cheaper insertion once you
already have the position, but each node may live far from the next one and
require separate allocation.

**C++ code/API anchor**

```cpp
#include <iostream>
#include <vector>

int main()
{
    std::vector<int> samples;
    samples.reserve(1000);

    samples.push_back(10);
    samples.push_back(20);

    std::cout << samples[0] << ' ' << samples.data()[1] << '\n';
}
```

**Production/debug angle**

Use `vector` first, then measure. Log `size()`, `capacity()`, and `data()` when
debugging unexpected invalidation or reallocation. Use `reserve()` when final
size is known.

**Common traps**

- Claiming `vector` never reallocates.
- Assuming `reserve()` creates elements.
- Keeping `data()` or iterators across operations that may reallocate.
- Replacing `vector` with `list` because "insertion is O(1)" without measuring
  search and cache cost.

**Follow-up questions**

- What invalidates `vector` iterators?
- When is `std::deque` better than `vector`?
- Why can a linear `vector` scan beat `unordered_set` for small data?

### 3. Compare C array, `std::array`, and `std::vector`.

**Short answer**

A C array is fixed-size raw storage that easily decays to a pointer.
`std::array` is a fixed-size STL-friendly wrapper. `std::vector` is a dynamic
contiguous owner that can grow.

**Deep explanation**

All three can store elements contiguously. The difference is API and lifetime
contract. A C array has no member `size()` once passed to a function by value
because it decays to a pointer. `std::array<T, N>` keeps the size in the type
and provides `size()`, iterators, `at()`, and whole-object assignment.
`std::vector<T>` stores a dynamic number of elements and manages heap-backed
capacity internally.

`std::array` usually lives wherever the object itself lives; do not simplify it
to "always stack allocated." A `vector` object can live anywhere, while its
element storage is dynamically managed.

**C++ code/API anchor**

```cpp
#include <array>
#include <iostream>
#include <vector>

void print_raw(const int* data, std::size_t size)
{
    for (std::size_t i = 0; i < size; ++i) {
        std::cout << data[i] << '\n';
    }
}

int main()
{
    int raw[3]{1, 2, 3};
    std::array<int, 3> fixed{1, 2, 3};
    std::vector<int> dynamic{1, 2, 3};

    print_raw(raw, 3);
    print_raw(fixed.data(), fixed.size());
    print_raw(dynamic.data(), dynamic.size());
}
```

**Production/debug angle**

Prefer `std::array` for fixed-size tables and buffers. Prefer `std::vector` for
runtime-sized collections. At C API boundaries, pass both pointer and size.

**Common traps**

- Passing a C array to a function and believing the size is preserved.
- Using `vector` for fixed-size embedded tables that should not allocate.
- Using `std::array` for huge data as an automatic local object without
  considering storage limits.
- Treating `data()` as valid after `vector` reallocation.

**Follow-up questions**

- Why is `std::span` useful here?
- When would you still use a C array?
- What does `std::array<int, 4>` versus `std::array<int, 8>` imply?

### 4. Explain `size()`, `capacity()`, `reserve()`, and `resize()` for `vector`.

**Short answer**

`size()` is the number of existing elements. `capacity()` is allocated element
space. `reserve()` changes capacity without creating elements. `resize()`
changes the number of elements.

**Deep explanation**

`vector` separates logical size from allocated capacity to avoid reallocating
on every append. If `push_back` would exceed capacity, the vector allocates a
larger block and moves/copies elements into it. The growth factor is
implementation-defined; rely on amortized complexity, not a guaranteed
doubling rule.

`reserve(n)` is a performance and invalidation-control tool. It prevents
reallocation until size grows beyond capacity. It does not make `v[0]` valid
when `size() == 0`. `resize(n)` constructs or destroys elements to make
`size() == n`.

**C++ code/API anchor**

```cpp
#include <iostream>
#include <vector>

int main()
{
    std::vector<int> values;
    values.reserve(4);

    std::cout << values.size() << ' ' << values.capacity() << '\n';

    values.push_back(10);
    values.resize(4);

    std::cout << values.size() << ' ' << values.capacity() << '\n';
}
```

**Production/debug angle**

Use `reserve()` before bulk insertion when the final size is known. Log
capacity changes when debugging invalidated pointers, references, or iterators.

**Common traps**

- Writing `v.reserve(10); v[0] = 1;`.
- Assuming capacity must equal size.
- Depending on a specific growth factor.
- Forgetting `resize()` may construct many elements.

**Follow-up questions**

- Does `reserve()` ever invalidate iterators?
- What is amortized O(1)?
- Why can `resize()` be expensive for non-trivial element types?

## Mid-Level Questions

### 5. Compare `std::vector` and `std::list`.

**Short answer**

Use `vector` by default for contiguous storage, random access, and cache
locality. Use `list` only when stable iterators and cheap insertion/erasure at
an already-known position are truly important.

**Deep explanation**

`vector` stores elements contiguously. Random access is O(1), iteration is
cache-friendly, and appending is amortized O(1). Insert or erase in the middle
can shift elements and invalidate iterators from the change point onward.

`list` stores separate nodes linked together. Inserting or erasing at a known
position is O(1), and other iterators remain valid. But finding that position
is O(n), each node has pointer overhead, and scattered memory often makes
iteration much slower.

**C++ code/API anchor**

```cpp
#include <algorithm>
#include <list>
#include <vector>

int main()
{
    std::vector<int> v{3, 1, 2};
    std::sort(v.begin(), v.end());

    std::list<int> l{3, 1, 2};
    l.sort(); // std::sort requires random-access iterators
}
```

**Production/debug angle**

Profile before switching away from `vector`. If a performance issue involves
middle insertion, also measure the cost of finding the insertion position and
the impact of allocation.

**Common traps**

- Saying `list` insertion is O(1) without "given an iterator to the position."
- Ignoring CPU cache locality.
- Using `std::sort` on `list`.
- Assuming stable iterators mean stable erased elements; erased element
  iterators are still invalid.

**Follow-up questions**

- What are good uses for `list::splice()`?
- How does `deque` fit between `vector` and `list`?
- What container would you choose for a fixed-size sensor sample window?

### 6. Compare `std::map` and `std::unordered_map`.

**Short answer**

`std::map` keeps keys ordered and provides O(log n) operations.
`std::unordered_map` uses hashing and provides average O(1) operations, with
bucket memory overhead, rehash invalidation, and O(n) worst-case behavior.

**Deep explanation**

Use `map` when sorted iteration, deterministic key order, range queries, or
predictable logarithmic behavior matter. Use `unordered_map` when key lookup
dominates, order does not matter, memory overhead is acceptable, and hash plus
equality are correct.

`unordered_map` performance depends on hash quality and load factor. Rehashing
can invalidate iterators and create latency spikes. `reserve()` can reduce
rehashing when the expected size is known.

**C++ code/API anchor**

```cpp
#include <map>
#include <string>
#include <unordered_map>

int main()
{
    std::map<std::string, int> ordered;
    std::unordered_map<std::string, int> hashed;

    ordered["error"] += 1;
    hashed.reserve(1000);
    hashed["error"] += 1;
}
```

**Production/debug angle**

For performance issues, inspect `size()`, `bucket_count()`, `load_factor()`,
and `max_load_factor()`. For behavior issues, check whether callers depend on
iteration order.

**Common traps**

- Saying `unordered_map` is always O(1).
- Forgetting hash tables can use more memory.
- Assuming iteration order is stable or sorted.
- Forgetting `operator[]` inserts in both `map` and `unordered_map`.

**Follow-up questions**

- When can `map` beat `unordered_map`?
- What does rehash invalidate?
- How would you handle adversarial input keys?

### 7. What is iterator invalidation, and how do you safely erase while iterating?

**Short answer**

Iterator invalidation means an iterator, pointer, or reference no longer safely
refers to an element after a container operation. For erase loops, use the
iterator returned by `erase()` or an algorithmic removal pattern.

**Deep explanation**

Invalidation rules are container- and operation-specific. For `vector`,
reallocation invalidates all iterators, references, and pointers to elements.
Insertion without reallocation invalidates from the insertion point to `end()`.
Erasure invalidates erased elements and elements after them. For node-based
containers like `list`, insertion generally preserves other iterators, while
erasure invalidates only erased elements.

The safe pattern is to let `erase()` return the next valid iterator.

**C++ code/API anchor**

```cpp
#include <vector>

int main()
{
    std::vector<int> values{1, 2, 3, 4, 5};

    for (auto it = values.begin(); it != values.end(); ) {
        if (*it % 2 == 0) {
            it = values.erase(it);
        } else {
            ++it;
        }
    }
}
```

**Production/debug angle**

Use sanitizers and debug iterator modes such as `_GLIBCXX_DEBUG` in debug
builds. Log `vector::capacity()` and `data()` around mutations when tracking
reallocation-related bugs.

**Common traps**

- Incrementing an iterator after erasing through it.
- Assuming all containers share the same invalidation rules.
- Keeping `vector::data()` across a `push_back`.
- Forgetting `end()` may be invalidated too.

**Follow-up questions**

- How does `unordered_map` rehash affect iterators?
- What does `erase()` return?
- Why is invalidated iterator use undefined behavior?

### 8. Explain the erase-remove idiom.

**Short answer**

The erase-remove idiom removes values from a sequence container by first using
`std::remove` or `std::remove_if` to move kept elements forward, then calling
the container's `erase()` to physically remove the unwanted tail.

**Deep explanation**

Algorithms work on iterator ranges and generally do not know how to change a
container's size. `std::remove_if` rearranges elements and returns the new
logical end. The container still has the same `size()` until `erase()` is
called.

This pattern is efficient for `vector` because it avoids repeated middle
erases and repeated shifting.

**C++ code/API anchor**

```cpp
#include <algorithm>
#include <vector>

int main()
{
    std::vector<int> values{1, 2, 3, 4, 5, 6};

    values.erase(
        std::remove_if(values.begin(), values.end(),
                       [](int x) { return x % 2 == 0; }),
        values.end());
}
```

For `std::list`, a member function can be clearer:

```cpp
#include <list>

int main()
{
    std::list<int> values{1, 2, 3, 4};
    values.remove_if([](int x) { return x % 2 == 0; });
}
```

**Production/debug angle**

If values still appear after `remove_if`, check whether the second `erase()`
step is missing. For C++20 and newer, also consider container-specific
`std::erase` or `std::erase_if` where appropriate.

**Common traps**

- Thinking `std::remove_if` changes container size.
- Applying erase-remove blindly to associative containers.
- Using a predicate with side effects that make behavior hard to reason about.
- Forgetting iterator invalidation after `erase()`.

**Follow-up questions**

- Why can an algorithm rearrange but not erase?
- What is the complexity advantage over erasing in a loop?
- How does this differ for `list`?

### 9. What makes a comparator or hash function correct?

**Short answer**

A sorting or ordered-container comparator must model strict weak ordering. A
hash function for unordered containers must be consistent with equality: if
`a == b`, then `hash(a) == hash(b)`.

**Deep explanation**

For comparators, `comp(a, a)` must be false, ordering must be asymmetric, and
the relation must be consistent enough for algorithms and ordered containers
to build a valid order. Using `<=` instead of `<` breaks this contract.

For unordered containers, the hash is used to choose a bucket and equality is
used to confirm a match within a bucket. If equal objects can hash differently,
lookups may fail even though `operator==` says the objects are equal.

**C++ code/API anchor**

```cpp
#include <cstddef>
#include <unordered_set>

struct DeviceId {
    int bus{};
    int address{};

    bool operator==(const DeviceId& other) const
    {
        return bus == other.bus && address == other.address;
    }
};

struct DeviceIdHash {
    std::size_t operator()(const DeviceId& id) const
    {
        return std::hash<int>{}(id.bus) ^ (std::hash<int>{}(id.address) << 1);
    }
};

int main()
{
    std::unordered_set<DeviceId, DeviceIdHash> devices;
    devices.insert({1, 42});
}
```

**Production/debug angle**

Test comparators and hashes with duplicate keys, equal objects, near-equal
objects, and deterministic small data sets. For unordered containers, inspect
collision behavior and load factor in performance investigations.

**Common traps**

- Writing `return a <= b;` as a sort comparator.
- Mutating a key after insertion.
- Defining `operator==` but forgetting the matching hash.
- Using a poor hash and assuming average O(1) still holds.

**Follow-up questions**

- What happens if a comparator is not strict?
- Why are keys effectively immutable in `map`?
- How would you test a custom key type?

### 10. Explain `lower_bound`, `upper_bound`, and `binary_search`.

**Short answer**

They are binary-search algorithms over a correctly partitioned range.
`lower_bound` finds the first element not less than a value, `upper_bound`
finds the first element greater than a value, and `binary_search` returns
whether a matching value exists.

**Deep explanation**

In normal use, the range must be sorted with the same comparison used for the
search. More precisely, it must be partitioned consistently with the searched
value. If you call binary-search algorithms on unsorted data, the result is not
meaningful.

`lower_bound` and `upper_bound` are often more useful than `binary_search`
because they give positions for insertion or equal-range counting.

**C++ code/API anchor**

```cpp
#include <algorithm>
#include <iostream>
#include <vector>

int main()
{
    std::vector<int> values{1, 2, 2, 2, 5, 9};

    auto first = std::lower_bound(values.begin(), values.end(), 2);
    auto last = std::upper_bound(values.begin(), values.end(), 2);

    std::cout << "count=" << (last - first) << '\n';
}
```

**Production/debug angle**

If binary search misses values that are visibly present, check whether the data
is sorted with the same comparator. Also check iterator category and the cost
of distance/arithmetic on non-random-access ranges.

**Common traps**

- Running binary search before sorting.
- Sorting with one comparator and searching with another.
- Assuming `binary_search` gives the position.
- Using these algorithms where a hash table would be simpler and order is not
  needed.

**Follow-up questions**

- How would you count duplicates in a sorted vector?
- Why might sorted vector plus binary search beat `set`?
- What does "partitioned range" mean?

## Senior Questions

### 11. When can a `vector` linear scan beat `unordered_set` lookup?

**Short answer**

For small data sets, compact trivial keys, hot cache, or one-shot searches, a
linear scan over contiguous `vector` storage can beat `unordered_set` despite
O(n) complexity because constant factors and cache locality dominate.

**Deep explanation**

`unordered_set` uses hashing, bucket lookup, equality checks, and often
node-based storage. That can mean extra memory reads and cache misses. A
`vector` scan is simple sequential memory access, and CPUs optimize that very
well.

As data grows or lookup count dominates, hash-based lookup may win. The right
answer depends on size, key cost, hash quality, allocation behavior, mutation
rate, and whether ordering or stable references matter.

**C++ code/API anchor**

```cpp
#include <algorithm>
#include <unordered_set>
#include <vector>

bool contains_vector(const std::vector<int>& values, int key)
{
    return std::find(values.begin(), values.end(), key) != values.end();
}

bool contains_set(const std::unordered_set<int>& values, int key)
{
    return values.find(key) != values.end();
}
```

**Production/debug angle**

Benchmark realistic workloads. Include construction time, memory use, lookup
count, hit/miss ratio, key distribution, and target hardware. Do not optimize
only the asymptotic lookup formula.

**Common traps**

- Treating Big-O as the full performance story.
- Ignoring construction and allocation cost.
- Benchmarking unrealistic data.
- Forgetting memory-constrained embedded systems may prefer compact storage.

**Follow-up questions**

- What would you measure in the benchmark?
- How does `reserve()` affect `unordered_set`?
- When would sorted `vector` plus `lower_bound` be a good compromise?

### 12. How would you design an API that accepts a sequence without taking ownership?

**Short answer**

Use a borrowed range representation: `std::span` for contiguous data in C++20,
iterator pairs or templates for generic ranges in C++17, and `std::string_view`
for borrowed text. Do not take `std::vector` by value unless you need a copy.

**Deep explanation**

An API should expose the contract. If the function only reads contiguous data,
`std::span<const T>` is a clean non-owning view. In C++17, iterator pairs or
`const std::vector<T>&` are common, but `const vector&` unnecessarily restricts
the caller to one container type.

For text, `std::string_view` avoids copying but has lifetime risk. For generic
containers, a template taking iterators can avoid ownership and avoid a
concrete container dependency.

**C++ code/API anchor**

```cpp
#include <numeric>
#include <span>   // C++20
#include <vector>

int checksum(std::span<const int> samples)
{
    return std::accumulate(samples.begin(), samples.end(), 0);
}

int main()
{
    std::vector<int> values{1, 2, 3};
    return checksum(values);
}
```

C++17 iterator-pair style:

```cpp
#include <numeric>
#include <vector>

template <typename Iterator>
int checksum(Iterator first, Iterator last)
{
    return std::accumulate(first, last, 0);
}

int main()
{
    std::vector<int> values{1, 2, 3};
    return checksum(values.begin(), values.end());
}
```

**Production/debug angle**

Document lifetime expectations. A view or iterator pair must not outlive the
underlying container, and the container must not be mutated in ways that
invalidate the view while it is used.

**Common traps**

- Returning `std::span` or `std::string_view` to a local object.
- Taking `std::shared_ptr` just to observe data.
- Exposing `std::vector&` in public APIs when only a range is needed.
- Storing iterators without defining mutation rules.

**Follow-up questions**

- Why is `std::span` limited to contiguous ranges?
- When is `const std::vector<T>&` still acceptable?
- How do ranges and views change this in newer C++?

### 13. What are the risks of Standard Library containers in embedded or latency-sensitive code?

**Short answer**

The main risks are dynamic allocation, unpredictable reallocation or rehashing,
node-allocation overhead, exception policy mismatch, and unclear maximum
capacity. The STL can still be appropriate when allocation and timing are
controlled.

**Deep explanation**

`std::array` is a strong fit for fixed-size data. `std::vector` can be
acceptable when capacity is reserved during initialization and runtime growth
is bounded. `unordered_map` may be risky in real-time paths because hashing and
rehashing can create latency spikes. Node-based containers can fragment memory
and increase per-element overhead.

The issue is not "STL bad for embedded." The issue is whether each container's
allocation, invalidation, and timing behavior matches the target constraints.

**C++ code/API anchor**

```cpp
#include <array>
#include <numeric>

int main()
{
    std::array<int, 4> samples{101, 103, 102, 104};
    int sum = std::accumulate(samples.begin(), samples.end(), 0);
    return sum == 0 ? 1 : 0;
}
```

Controlled dynamic storage:

```cpp
#include <vector>

int main()
{
    std::vector<int> samples;
    samples.reserve(128); // initialization phase
}
```

**Production/debug angle**

Review maximum sizes, allocation phase, failure policy, and mutation frequency.
Measure worst-case timing, not just average throughput. Inspect `capacity()`,
`bucket_count()`, and `load_factor()` where relevant.

**Common traps**

- Blanket banning STL without analyzing constraints.
- Allowing `vector` growth in a hard real-time loop.
- Using `unordered_map` without controlling rehash.
- Ignoring exception and allocator policies.

**Follow-up questions**

- How would you design a fixed-capacity alternative?
- When would `std::pmr` be relevant?
- What container would you choose for a command lookup table?

### 14. Explain file-stream error handling and why `while (!eof())` is wrong.

**Short answer**

File streams report state through flags such as `good()`, `fail()`, `bad()`,
and `eof()`. `while (!eof())` is wrong because EOF is usually known only after
a read attempt fails, so the loop can process stale data.

**Deep explanation**

Stream extraction should be part of the loop condition. `fail()` can mean a
format error, `eof()` means end-of-file was reached, and `bad()` indicates a
serious I/O error. Writing also needs checks, especially if close or flush
failure matters.

File streams are RAII objects, so destructors close resources, but a destructor
is not a good place to report recoverable close/flush errors. Use explicit
`close()` when the caller must observe failure.

**C++ code/API anchor**

```cpp
#include <fstream>
#include <iostream>

int main()
{
    std::ifstream file("values.txt");
    if (!file) {
        return 1;
    }

    int value{};
    while (file >> value) {
        std::cout << value << '\n';
    }

    if (file.bad()) {
        std::cerr << "I/O error\n";
        return 1;
    }
}
```

**Production/debug angle**

Log whether the failure is open failure, format failure, EOF, or serious I/O
failure. Test missing files, empty files, malformed input, partial writes, and
close/flush failures where data integrity matters.

**Common traps**

- Using `while (!file.eof())`.
- Treating EOF as an error in all contexts.
- Assuming destructor close is enough when errors must be reported.
- Dumping raw structs as portable binary serialization.

**Follow-up questions**

- When would you enable stream exceptions?
- Why can raw binary struct serialization be non-portable?
- How does this compare with POSIX `open/read/write/close`?

## Coding Tasks

### Task 1. Remove invalid readings from a vector.

**Prompt**

Write a function that removes all negative readings from `std::vector<int>`.

**Expected answer**

```cpp
#include <algorithm>
#include <vector>

void remove_invalid(std::vector<int>& readings)
{
    readings.erase(
        std::remove_if(readings.begin(), readings.end(),
                       [](int value) { return value < 0; }),
        readings.end());
}
```

**What this tests**

- erase-remove idiom;
- lambda predicate;
- mutation through reference;
- awareness that `remove_if` does not erase by itself.

**Common traps**

- Calling only `std::remove_if`.
- Erasing inside a loop with an invalidated iterator.
- Returning a new vector when in-place mutation was requested.

**Follow-ups**

- How would this change for `std::list<int>`?
- What happens to iterators into `readings` after this function?
- How would you preserve the removed values?

### Task 2. Count words and explain container choice.

**Prompt**

Count word frequencies from a sequence of `std::string`. Choose `map` or
`unordered_map` and justify the choice.

**Expected answer**

```cpp
#include <string>
#include <unordered_map>
#include <vector>

std::unordered_map<std::string, int>
count_words(const std::vector<std::string>& words)
{
    std::unordered_map<std::string, int> counts;
    counts.reserve(words.size());

    for (const auto& word : words) {
        ++counts[word];
    }

    return counts;
}
```

**What this tests**

- `unordered_map` average lookup behavior;
- intentional use of `operator[]` for counting;
- `reserve()` for expected size;
- order versus performance tradeoff.

**Common traps**

- Saying `unordered_map` is always better.
- Forgetting `operator[]` inserts.
- Not considering output order requirements.

**Follow-ups**

- What if output must be sorted alphabetically?
- What if input comes from untrusted users?
- How would you avoid creating empty entries during lookup?

### Task 3. Build a task priority queue.

**Prompt**

Define a `Task` with `name` and `priority`. Higher priority should appear
first.

**Expected answer**

```cpp
#include <queue>
#include <string>
#include <vector>

struct Task {
    std::string name;
    int priority{};
};

struct LowerPriorityFirst {
    bool operator()(const Task& a, const Task& b) const
    {
        return a.priority < b.priority;
    }
};

using TaskQueue =
    std::priority_queue<Task, std::vector<Task>, LowerPriorityFirst>;
```

**What this tests**

- container adapter knowledge;
- comparator semantics;
- underlying container awareness;
- small domain modeling.

**Common traps**

- Reversing comparator semantics.
- Expecting iteration over `priority_queue` in priority order.
- Forgetting that equal priorities need a tie-breaker if deterministic behavior
  matters.

**Follow-ups**

- Add timestamp tie-breaking.
- Why is `priority_queue` not a general sorted container?
- What happens if the comparator reads mutable external state?

## Debugging Questions

### Debug 1. Why does this vector code sometimes crash?

```cpp
#include <iostream>
#include <vector>

int main()
{
    std::vector<int> values{1, 2, 3};
    auto it = values.begin();

    values.push_back(4);

    std::cout << *it << '\n';
}
```

**Short answer**

`push_back` may reallocate the vector, invalidating `it`. Dereferencing it is
undefined behavior.

**Deep explanation**

If size exceeds capacity, `vector` allocates a new block and moves/copies
elements. The old iterator points into old storage. If no reallocation happens,
only `end()` is invalidated for `push_back`, but code cannot rely on accidental
capacity.

**C++ code/API anchor**

```cpp
#include <iostream>
#include <vector>

int main()
{
    std::vector<int> values{1, 2, 3};
    std::size_t index = 0;

    values.push_back(4);
    std::cout << values[index] << '\n';
}
```

**Production/debug angle**

Use indices or refresh iterators after mutation. In debug builds, use
sanitizers and debug iterator modes. Log `capacity()` before and after
`push_back`.

**Common traps**

- Testing once and assuming the iterator is safe.
- Reserving in one code path but not another.
- Keeping references into `vector` inside other containers without a stability
  plan.

**Follow-ups**

- Would `std::list` have the same invalidation behavior?
- What if `values.reserve(10)` is called first?
- Does `push_back` invalidate references when no reallocation happens?

### Debug 2. Why does this file loop print the last value twice?

```cpp
#include <fstream>
#include <iostream>

int main()
{
    std::ifstream file("values.txt");
    int value{};

    while (!file.eof()) {
        file >> value;
        std::cout << value << '\n';
    }
}
```

**Short answer**

The loop checks EOF before attempting the next read. When extraction fails, the
old value may still be printed.

**Deep explanation**

EOF is usually discovered by trying to read past available input. The read
operation should control the loop. Also, failure can be a format error, not
only EOF.

**C++ code/API anchor**

```cpp
int value{};
while (file >> value) {
    std::cout << value << '\n';
}

if (file.bad()) {
    std::cerr << "I/O error\n";
}
```

**Production/debug angle**

Test empty files, missing files, malformed input, and partial records. Inspect
`fail()`, `bad()`, and `eof()` separately.

**Common traps**

- Treating EOF as the loop condition.
- Ignoring open failure.
- Treating format failure and EOF as the same result.

**Follow-ups**

- How would you report malformed input?
- When should `eof()` be checked?
- What should output code check after writing?

### Debug 3. Why is this sort comparator broken?

```cpp
#include <algorithm>
#include <vector>

int main()
{
    std::vector<int> values{3, 1, 2};

    std::sort(values.begin(), values.end(),
              [](int a, int b) { return a <= b; });
}
```

**Short answer**

The comparator is not a strict weak ordering because `comp(a, a)` returns
true. Use `<`, not `<=`.

**Deep explanation**

Sorting algorithms rely on a consistent ordering relation. If the comparator
contradicts itself, the algorithm's assumptions break and behavior is not
reliable.

**C++ code/API anchor**

```cpp
#include <algorithm>
#include <vector>

int main()
{
    std::vector<int> values{3, 1, 2};

    std::sort(values.begin(), values.end(),
              [](int a, int b) { return a < b; });
}
```

**Production/debug angle**

Comparator bugs often appear as rare ordering failures or hard-to-reproduce
behavior. Test equal elements, reversed input, already-sorted input, and mixed
duplicates.

**Common traps**

- Using `<=` or `>=`.
- Capturing mutable state in the comparator.
- Sorting with one comparator and binary-searching with another.

**Follow-ups**

- What is a tie-breaker for sorting structs?
- How does comparator correctness affect `std::map`?
- What tests would you write for a comparator?

### Debug 4. Why does this lookup modify the map?

```cpp
#include <map>
#include <string>

int main()
{
    std::map<std::string, int> ports;

    if (ports["sensor"] == 0) {
        return 1;
    }
}
```

**Short answer**

`operator[]` inserts a missing key with a value-initialized mapped value.

**Deep explanation**

`map::operator[]` is useful for counting and intentional insertion, but it is
not a pure lookup operation. If the key is absent, it creates an entry so that
it can return a reference to the mapped value.

**C++ code/API anchor**

```cpp
auto it = ports.find("sensor");
if (it == ports.end()) {
    return 1;
}
```

In C++20:

```cpp
if (!ports.contains("sensor")) {
    return 1;
}
```

**Production/debug angle**

Unexpected map growth can cause memory growth, changed behavior, and misleading
metrics. In review, ask whether `operator[]` means "insert if missing" or was
used accidentally.

**Common traps**

- Using `operator[]` for read-only lookup.
- Forgetting the same issue applies to `unordered_map`.
- Using `at()` without handling the missing-key exception.

**Follow-ups**

- When is `operator[]` the right tool?
- Compare `try_emplace` and `insert_or_assign`.
- How would you avoid default construction of expensive mapped values?

## Rapid Review Checklist

- Can you explain containers, iterators, algorithms, and callables together?
- Can you justify `vector` as the default without turning it into a slogan?
- Can you state vector invalidation rules for `push_back`, `insert`, `erase`,
  and `reserve()`?
- Can you compare C array, `std::array`, and `std::vector`?
- Can you compare `vector` and `list` with cache locality and O(n) search?
- Can you compare `map` and `unordered_map` with ordering, memory, rehashing,
  and worst-case behavior?
- Can you write erase-remove from memory and explain why both steps are needed?
- Can you explain strict weak ordering and hash/equality consistency?
- Can you debug `while (!eof())` and stream state mistakes?
- Can you choose containers for embedded or latency-sensitive constraints?
