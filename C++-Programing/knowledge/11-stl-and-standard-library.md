# 11 - STL And Standard Library

## 1. Goal

After this lesson, you should be able to:

- explain the STL model: containers, iterators, algorithms, and callables;
- choose between `std::vector`, `std::array`, `std::deque`, `std::list`, and
  `std::forward_list`;
- use `std::stack`, `std::queue`, and `std::priority_queue` correctly;
- choose between `std::map`, `std::set`, `std::unordered_map`, and
  `std::unordered_set`;
- reason about complexity, memory layout, cache locality, and allocation cost;
- explain iterator categories and iterator invalidation;
- use common algorithms such as `std::sort`, `std::find`, `std::transform`,
  `std::accumulate`, `std::lower_bound`, and `std::remove_if`;
- write safe comparators, predicates, functors, and lambdas;
- use file streams with RAII and explicit error handling;
- avoid common STL bugs in production code and interviews.

This lesson uses C++17 for most examples. Some notes mention C++20 vocabulary
types such as `std::span` and APIs such as `contains()`.

Chapter 10, Resource Management In C++, is the prerequisite.

## 2. Why It Matters

The Standard Library is the practical foundation of modern C++.

Without it, many programs drift toward hand-written arrays, linked lists,
manual string buffers, raw file handles, and custom search loops. That usually
creates bugs:

- leaks and double cleanup from manual resource management;
- buffer overflows and missing null terminators;
- slow code from the wrong data structure;
- invalid iterators after container modification;
- broken comparators that make sorting undefined;
- accidental insertion into maps;
- file-reading loops that process stale data.

The Standard Library gives you tested building blocks, but it does not remove
the need for engineering judgment. You still must know:

- who owns the elements;
- whether storage is contiguous or node-based;
- which operations are O(1), O(log n), or O(n);
- which operations invalidate iterators, references, or pointers;
- what preconditions an algorithm requires;
- what happens when file I/O fails.

The central review question is:

> Does this container and algorithm match the access pattern, lifetime, and
> failure behavior of the real program?

## 3. Mental Model

### 3.1 The Four STL Pillars

The STL is built around four ideas:

| Pillar | Meaning | Examples |
| --- | --- | --- |
| Container | Owns and organizes elements | `vector`, `map`, `unordered_set` |
| Iterator | Describes a position or range | `begin()`, `end()`, `it++` |
| Algorithm | Performs an operation over a range | `sort`, `find`, `transform` |
| Callable | Customizes behavior | lambda, comparator, functor |

The powerful idea is separation:

```cpp
std::sort(values.begin(), values.end());
```

`std::sort` does not need to know that `values` is a `std::vector<int>`. It
needs a range with random-access iterators and elements that can be ordered.

### 3.2 Containers Own Elements

Standard containers normally store elements by value:

```cpp
#include <string>
#include <vector>

int main()
{
    std::vector<std::string> names;
    names.push_back("Ada");
    names.push_back("Bjarne");
}
```

The `vector` owns its `std::string` elements. When the `vector` is destroyed,
the strings are destroyed too. This is RAII applied to collections.

If a container stores pointers, it owns only the pointer values unless the
pointer type itself owns:

```cpp
#include <memory>
#include <vector>

struct Sensor {
    int id{};
};

int main()
{
    std::vector<std::unique_ptr<Sensor>> sensors;
    sensors.push_back(std::make_unique<Sensor>(Sensor{42}));
}
```

Here the `vector` owns `unique_ptr` objects, and each `unique_ptr` owns a
`Sensor`.

### 3.3 Iterators Describe Ranges

A range is usually written as `[begin, end)`: begin is included, end is not.

```cpp
#include <iostream>
#include <vector>

int main()
{
    std::vector<int> values{10, 20, 30};

    for (auto it = values.begin(); it != values.end(); ++it) {
        std::cout << *it << '\n';
    }
}
```

`end()` is a sentinel one past the last element. Do not dereference it.

### 3.4 Algorithms Work On Ranges

Algorithms let code say what it means:

```cpp
#include <algorithm>
#include <iostream>
#include <vector>

int main()
{
    std::vector<int> values{4, 1, 3, 2};

    std::sort(values.begin(), values.end());

    if (std::find(values.begin(), values.end(), 3) != values.end()) {
        std::cout << "found\n";
    }
}
```

Prefer a named algorithm when it expresses the intent better than a handwritten
loop.

### 3.5 Complexity Is Part Of Correctness

Big-O is not decoration. It decides whether code survives production data.

| Complexity | Meaning | Example |
| --- | --- | --- |
| O(1) | Constant time | `vector[i]` |
| O(log n) | Grows slowly | `map.find(key)` |
| O(n) | Linear scan | `find(v.begin(), v.end(), x)` |
| O(n log n) | Typical comparison sort | `sort(v.begin(), v.end())` |

Big-O is necessary but not sufficient. Cache locality, allocation, branch
prediction, element size, and profiling also matter. A `std::vector` linear
scan can beat a hash table for small data.

## 4. Core Mechanism

### 4.1 Value Semantics

Standard containers are value types:

```cpp
#include <vector>

int main()
{
    std::vector<int> a{1, 2, 3};
    std::vector<int> b = a;  // copies elements

    b.push_back(4);
    // a is still {1, 2, 3}
}
```

Copying a container copies its elements. Moving a container transfers its
resources when the implementation can do so. After a move, the source object is
valid but its value is unspecified unless the type documents more.

### 4.2 Contiguous Versus Node-Based Storage

Container layout strongly affects performance:

| Layout | Containers | Consequence |
| --- | --- | --- |
| Contiguous | `vector`, `array` | Fast iteration, C API interop, invalidation on reallocation |
| Segmented | `deque` | Fast ends, random access, not one contiguous buffer |
| Node-based | `list`, `map`, `unordered_map` | Stable nodes, more allocation, poorer cache locality |

This is why `vector` is the default sequence container. It is boring in the
best way: simple layout, fast iteration, and predictable access.

### 4.3 Iterator Categories

Algorithms require different iterator capabilities:

| Category | Capability | Example |
| --- | --- | --- |
| Input | Read forward once | stream iterators |
| Output | Write forward once | output stream iterator |
| Forward | Multi-pass forward | `forward_list` |
| Bidirectional | Forward and backward | `list`, `map`, `set` |
| Random access | Jump by offset | `vector`, `deque`, `array` |
| Contiguous | Random access over contiguous memory | `vector`, `array`, raw arrays |

`std::sort` requires random-access iterators. That is why this does not compile:

```cpp
#include <algorithm>
#include <list>

int main()
{
    std::list<int> values{3, 1, 2};
    // std::sort(values.begin(), values.end()); // error: not random access
    values.sort();                              // list-specific sort
}
```

### 4.4 Iterator Invalidation

Iterator invalidation means an iterator, pointer, or reference becomes unsafe
after a container operation.

Using an invalidated iterator is undefined behavior.

For `std::vector`, remember these rules:

| Operation | Invalidation |
| --- | --- |
| Read-only operation | Does not invalidate |
| `reserve()` | Invalidates all if capacity changes |
| `push_back()` / `emplace_back()` | Invalidates all if reallocation happens; otherwise invalidates `end()` |
| `insert()` / `emplace()` | Invalidates all if reallocation happens; otherwise from insertion point to `end()` |
| `erase()` | Invalidates erased elements and everything after them |
| `clear()` / assignment | Invalidates all |

Safe erase loop:

```cpp
#include <vector>

int main()
{
    std::vector<int> values{1, 2, 3, 4, 5, 6};

    for (auto it = values.begin(); it != values.end(); ) {
        if (*it % 2 == 0) {
            it = values.erase(it);  // returns next valid iterator
        } else {
            ++it;
        }
    }
}
```

The shape of this loop matters. Incrementing `it` after erasing through it
uses an invalid iterator.

## 5. Sequence Containers

### 5.1 `std::vector`

`std::vector<T>` is a dynamic contiguous array.

Use it when:

- you need fast iteration;
- you need O(1) random access;
- you append mostly at the end;
- you need `data()` for a C-style API;
- you want the default container and have no measured reason to choose another.

```cpp
#include <iostream>
#include <vector>

int main()
{
    std::vector<int> samples;
    samples.reserve(4);      // capacity only; no elements created

    samples.push_back(10);
    samples.push_back(20);
    samples.push_back(30);

    std::cout << "size=" << samples.size()
              << " capacity=" << samples.capacity() << '\n';

    std::cout << samples[1] << '\n';     // unchecked
    std::cout << samples.at(1) << '\n';  // checked; may throw
}
```

`size()` is the number of elements. `capacity()` is allocated space. `reserve()`
changes capacity. `resize()` changes size.

Do not do this:

```cpp
#include <vector>

int main()
{
    std::vector<int> values;
    values.reserve(10);
    // values[0] = 42; // undefined behavior: no element exists
}
```

Use `resize()` if you need elements immediately:

```cpp
#include <vector>

int main()
{
    std::vector<int> values;
    values.resize(10);
    values[0] = 42;
}
```

### 5.2 `std::array`

`std::array<T, N>` is a fixed-size array with STL-friendly behavior.

```cpp
#include <array>
#include <iostream>

int main()
{
    std::array<int, 4> pins{1, 2, 3, 4};

    for (int pin : pins) {
        std::cout << pin << '\n';
    }
}
```

The size is part of the type: `std::array<int, 4>` and `std::array<int, 8>` are
different types.

Use `std::array` for fixed-size buffers, lookup tables, and embedded-style
collections where dynamic allocation is not wanted.

### 5.3 `std::deque`

`std::deque<T>` is a double-ended queue. It supports efficient insertion and
removal at both front and back.

```cpp
#include <deque>
#include <iostream>

int main()
{
    std::deque<int> queue;
    queue.push_back(2);
    queue.push_front(1);
    queue.push_back(3);

    for (int value : queue) {
        std::cout << value << '\n';
    }
}
```

`deque` has random access, but it is not one contiguous buffer. Do not use
`deque` when a stable `data()` pointer is required.

### 5.4 `std::list`

`std::list<T>` is a doubly linked list.

Use it only when you really need:

- stable iterators to non-erased elements;
- cheap insertion/erasure when you already have the position;
- operations such as `splice()`, `merge()`, or member `sort()`.

```cpp
#include <iostream>
#include <list>

int main()
{
    std::list<int> values{1, 3, 4};
    auto it = values.begin();
    ++it;                    // points to 3
    values.insert(it, 2);     // cheap once position is known

    for (int value : values) {
        std::cout << value << '\n';
    }
}
```

Finding the position is still O(n). Many real workloads are faster with
`vector` because contiguous memory is friendly to the CPU cache.

### 5.5 `std::forward_list`

`std::forward_list<T>` is a singly linked list.

It saves memory compared with `list`, but it supports only forward traversal
and uses `insert_after()` / `erase_after()` style APIs.

```cpp
#include <forward_list>

int main()
{
    std::forward_list<int> values{1, 3, 4};
    auto before_three = values.begin();
    values.insert_after(before_three, 2);
}
```

Use it only when the memory savings and forward-only traversal match the
problem.

## 6. Container Adapters

Container adapters restrict access to an underlying container.

### 6.1 `std::stack`

`std::stack<T>` is LIFO: last in, first out.

```cpp
#include <iostream>
#include <stack>

int main()
{
    std::stack<int> calls;
    calls.push(10);
    calls.push(20);

    std::cout << calls.top() << '\n'; // 20
    calls.pop();
}
```

Use it for undo stacks, parsing, DFS, and nested state tracking.

### 6.2 `std::queue`

`std::queue<T>` is FIFO: first in, first out.

```cpp
#include <iostream>
#include <queue>

int main()
{
    std::queue<int> tasks;
    tasks.push(1);
    tasks.push(2);

    std::cout << tasks.front() << '\n'; // 1
    tasks.pop();
}
```

Use it for task queues, breadth-first search, and producer/consumer designs.
Thread safety is not automatic; add synchronization when sharing between
threads.

### 6.3 `std::priority_queue`

`std::priority_queue<T>` returns the highest-priority element first.

```cpp
#include <iostream>
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
        return a.priority < b.priority; // larger priority appears at top()
    }
};

int main()
{
    std::priority_queue<Task, std::vector<Task>, LowerPriorityFirst> tasks;
    tasks.push({"normal", 1});
    tasks.push({"urgent", 5});

    std::cout << tasks.top().name << '\n'; // urgent
}
```

Comparator semantics are a common source of confusion. Test priority queues
with small data before trusting them in scheduling code.

## 7. Associative Containers

### 7.1 Ordered Containers

`std::map` stores unique keys with values, ordered by key. `std::set` stores
unique keys only.

```cpp
#include <iostream>
#include <map>
#include <string>

int main()
{
    std::map<std::string, int> counts;
    counts["error"] += 1; // intentional insertion if missing
    counts["warn"] += 1;
    counts["error"] += 1;

    for (const auto& [word, count] : counts) {
        std::cout << word << ": " << count << '\n';
    }
}
```

Ordered containers provide O(log n) lookup, insertion, and erasure. Iteration
is sorted by key.

Be careful with `operator[]`:

```cpp
#include <map>
#include <string>

int main()
{
    std::map<std::string, int> ports;

    if (ports["sensor"] == 0) {
        // This inserted "sensor" with value 0.
    }
}
```

For lookup without insertion, use `find()` or, in C++20, `contains()`:

```cpp
#include <iostream>
#include <map>
#include <string>

int main()
{
    std::map<std::string, int> ports{{"debug", 9000}};

    auto it = ports.find("sensor");
    if (it == ports.end()) {
        std::cout << "missing\n";
    }
}
```

### 7.2 Unordered Containers

`std::unordered_map` and `std::unordered_set` use hash tables.

```cpp
#include <iostream>
#include <string>
#include <unordered_map>

int main()
{
    std::unordered_map<std::string, int> counts;
    counts.reserve(1000);

    ++counts["ok"];
    ++counts["error"];
    ++counts["ok"];

    std::cout << counts.at("ok") << '\n';
}
```

Unordered containers provide average O(1) lookup, insertion, and erasure, but
worst-case O(n) is possible. They also use extra memory for buckets.

Use `unordered_map` when:

- key lookup dominates;
- ordering does not matter;
- hash and equality are correct;
- memory overhead is acceptable;
- rehashing latency is acceptable or controlled.

Use `map` when:

- ordered iteration matters;
- range queries matter;
- predictable O(log n) behavior is preferred;
- hash quality or adversarial input is a concern.

### 7.3 Custom Hash And Equality

For a custom key, equal objects must produce equal hashes.

```cpp
#include <cstddef>
#include <iostream>
#include <string>
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
        std::size_t h1 = std::hash<int>{}(id.bus);
        std::size_t h2 = std::hash<int>{}(id.address);
        return h1 ^ (h2 << 1);
    }
};

int main()
{
    std::unordered_set<DeviceId, DeviceIdHash> devices;
    devices.insert({1, 42});

    std::cout << devices.count({1, 42}) << '\n';
}
```

This hash combiner is enough for a small lesson example. For serious domain
keys, review collision behavior and benchmark realistic data.

## 8. Iterators And Algorithms

### 8.1 Iterator Operations

Common iterator helpers live in `<iterator>`:

```cpp
#include <iostream>
#include <iterator>
#include <list>

int main()
{
    std::list<int> values{10, 20, 30, 40};

    auto it = values.begin();
    std::advance(it, 2);

    std::cout << *it << '\n'; // 30
    std::cout << std::distance(values.begin(), values.end()) << '\n';
}
```

`std::advance()` and `std::distance()` are O(1) for random-access iterators but
O(n) for lists and other non-random-access ranges.

### 8.2 Sorting And Searching

```cpp
#include <algorithm>
#include <iostream>
#include <vector>

int main()
{
    std::vector<int> values{7, 2, 9, 2, 5};

    std::sort(values.begin(), values.end());

    bool has_five = std::binary_search(values.begin(), values.end(), 5);
    auto first_two = std::lower_bound(values.begin(), values.end(), 2);
    auto after_twos = std::upper_bound(values.begin(), values.end(), 2);

    std::cout << std::boolalpha << has_five << '\n';
    std::cout << "count of 2 = " << (after_twos - first_two) << '\n';
}
```

Binary-search algorithms require a range partitioned consistently with the
comparison. In ordinary teaching terms: sort first, then binary search using
the same ordering.

### 8.3 Transform And Accumulate

```cpp
#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>

int main()
{
    std::vector<int> raw{1, 2, 3, 4};
    std::vector<int> squared(raw.size());

    std::transform(raw.begin(), raw.end(), squared.begin(),
                   [](int x) { return x * x; });

    int sum = std::accumulate(squared.begin(), squared.end(), 0);

    std::cout << sum << '\n'; // 30
}
```

`std::transform` maps values. `std::accumulate` folds a range into one value.
For heavy objects such as large strings, think about copying and allocation
inside the fold.

### 8.4 Erase-Remove Idiom

`std::remove_if` does not erase elements from the container. It moves kept
elements to the front and returns a new logical end.

```cpp
#include <algorithm>
#include <iostream>
#include <vector>

int main()
{
    std::vector<int> values{1, 2, 3, 4, 5, 6};

    values.erase(
        std::remove_if(values.begin(), values.end(),
                       [](int x) { return x % 2 == 0; }),
        values.end());

    for (int value : values) {
        std::cout << value << ' ';
    }
}
```

This is usually better than erasing repeatedly from the middle of a vector.

For `std::list`, prefer member functions when they express the operation:

```cpp
#include <list>

int main()
{
    std::list<int> values{1, 2, 3, 4};
    values.remove_if([](int x) { return x % 2 == 0; });
}
```

### 8.5 Comparators, Functors, And Lambdas

A comparator used by `sort`, `map`, or `set` must behave like a strict weak
ordering. In simple terms:

- `comp(a, a)` should be false;
- if `comp(a, b)` is true, `comp(b, a)` should be false;
- the ordering should be consistent.

```cpp
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

struct Person {
    std::string name;
    int age{};
};

int main()
{
    std::vector<Person> people{{"Ada", 36}, {"Bjarne", 35}, {"Grace", 36}};

    std::sort(people.begin(), people.end(),
              [](const Person& a, const Person& b) {
                  if (a.age != b.age) {
                      return a.age < b.age;
                  }
                  return a.name < b.name;
              });

    for (const auto& person : people) {
        std::cout << person.age << ' ' << person.name << '\n';
    }
}
```

Use lambdas for local one-off logic. Use named functors when the logic is
reused, stateful, or important enough to test directly.

Avoid returning lambdas that capture local variables by reference:

```cpp
#include <functional>

std::function<bool(int)> broken(int limit)
{
    return [&](int value) { return value < limit; }; // dangling reference
}
```

Capture by value when the callable may outlive the current scope:

```cpp
#include <functional>

std::function<bool(int)> below(int limit)
{
    return [limit](int value) { return value < limit; };
}
```

## 9. Standard Library Vocabulary Types

### 9.1 `std::string`

`std::string` owns text data. Prefer it over raw `char*` for ordinary C++ text.

```cpp
#include <iostream>
#include <string>

int main()
{
    std::string name = "sensor";
    name += "-left";

    std::cout << name << " size=" << name.size() << '\n';
}
```

Use `c_str()` when calling a C API that expects a null-terminated string, and
do not keep the returned pointer across string modifications.

### 9.2 `std::string_view`

`std::string_view` is a non-owning view of text.

```cpp
#include <iostream>
#include <string_view>

void print_label(std::string_view label)
{
    std::cout << label << '\n';
}

int main()
{
    print_label("temperature");
}
```

It does not own characters. A `string_view` can dangle if it outlives the
string or buffer it views.

### 9.3 `std::span`

`std::span<T>` is a non-owning view of contiguous elements. It is C++20.

```cpp
#include <iostream>
#include <span>
#include <vector>

void print_samples(std::span<const int> samples)
{
    for (int sample : samples) {
        std::cout << sample << '\n';
    }
}

int main()
{
    std::vector<int> data{10, 20, 30};
    print_samples(data);
}
```

Use it when a function should accept a contiguous range without owning it.

### 9.4 `std::filesystem` And `std::chrono`

`std::filesystem` gives standard path and filesystem operations. `std::chrono`
gives type-safe time points and durations.

```cpp
#include <chrono>
#include <filesystem>
#include <iostream>

int main()
{
    std::filesystem::path path{"config.txt"};
    std::chrono::milliseconds timeout{250};

    std::cout << path.string() << ' ' << timeout.count() << " ms\n";
}
```

Detailed filesystem race/security topics and POSIX file descriptors belong in
the POSIX/Linux comparison chapter.

## 10. File Streams

File streams are Standard Library RAII wrappers around file-based stream
buffers.

| Type | Purpose |
| --- | --- |
| `std::ifstream` | Read from a file |
| `std::ofstream` | Write to a file |
| `std::fstream` | Read and write |

### 10.1 Text File Reading

```cpp
#include <fstream>
#include <iostream>
#include <string>

int main()
{
    std::ifstream file("input.txt");
    if (!file) {
        std::cerr << "cannot open input.txt\n";
        return 1;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::cout << line << '\n';
    }

    if (file.bad()) {
        std::cerr << "I/O error while reading\n";
        return 1;
    }
}
```

Read in the loop condition. Do not write `while (!file.eof())`.

### 10.2 Writing And Checking Output

```cpp
#include <fstream>
#include <iostream>

int main()
{
    std::ofstream file("output.txt");
    if (!file) {
        std::cerr << "cannot create output.txt\n";
        return 1;
    }

    file << "status=ok\n";

    file.close();
    if (!file) {
        std::cerr << "failed to flush or close output.txt\n";
        return 1;
    }
}
```

Destructors close streams, but explicit `close()` is useful when the program
must observe close/flush failure.

### 10.3 Binary Files Are Not Magic Serialization

Writing raw bytes is low-level I/O:

```cpp
#include <cstdint>
#include <fstream>
#include <iostream>

int main()
{
    std::uint32_t value = 0x12345678;

    std::ofstream file("value.bin", std::ios::binary);
    if (!file) {
        return 1;
    }

    file.write(reinterpret_cast<const char*>(&value), sizeof(value));
    if (!file) {
        std::cerr << "write failed\n";
        return 1;
    }
}
```

Do not treat raw struct dumps as portable serialization when padding,
endianness, pointers, versioning, or non-trivial invariants matter.

## 11. Practical Usage

### 11.1 Container Selection

Start with the access pattern:

| Need | Usually choose |
| --- | --- |
| Dynamic sequence, fast iteration | `std::vector` |
| Fixed-size sequence | `std::array` |
| Push/pop at both ends | `std::deque` |
| Stable iterators and node operations | `std::list` |
| Minimal forward-only node list | `std::forward_list` |
| LIFO behavior | `std::stack` |
| FIFO behavior | `std::queue` |
| Highest-priority first | `std::priority_queue` |
| Sorted key/value lookup | `std::map` |
| Sorted unique membership | `std::set` |
| Fast average key/value lookup | `std::unordered_map` |
| Fast average membership | `std::unordered_set` |

Container choice is a performance and correctness decision, not a style
preference.

### 11.2 Embedded Usage

In embedded C++, STL use depends on target constraints.

Good fits:

- `std::array` for fixed hardware tables;
- `std::vector` with `reserve()` during initialization;
- `std::span` for non-owning access to buffers;
- simple algorithms for parsing, filtering, and lookup-table setup;
- `std::priority_queue` for non-real-time event simulation or scheduling.

Be careful with:

- dynamic allocation after startup;
- `unordered_map` rehashing latency;
- node-based containers with many small allocations;
- exceptions and file streams on platforms where they are disabled or absent.

Example fixed sensor window:

```cpp
#include <array>
#include <numeric>

int main()
{
    std::array<int, 4> samples{101, 103, 102, 104};
    int sum = std::accumulate(samples.begin(), samples.end(), 0);
    int average = sum / static_cast<int>(samples.size());
    return average == 0 ? 1 : 0;
}
```

### 11.3 Enterprise Usage

In enterprise code:

- use standard containers instead of custom ownership-heavy data structures;
- make complexity expectations visible in reviews;
- avoid exposing concrete containers in public APIs unless the container is
  part of the contract;
- pass `std::span`, iterator pairs, or views when the function should not own;
- document comparator, ordering, uniqueness, and hash/equality requirements;
- benchmark before changing containers for performance;
- treat all file I/O as failure-prone.

Example API using a non-owning view:

```cpp
#include <numeric>
#include <span>
#include <vector>

int checksum(std::span<const int> data)
{
    return std::accumulate(data.begin(), data.end(), 0);
}

int main()
{
    std::vector<int> packet{1, 2, 3, 4};
    return checksum(packet);
}
```

## 12. Required Comparisons

### 12.1 C Array Versus `std::array` Versus `std::vector`

| Feature | C array | `std::array` | `std::vector` |
| --- | --- | --- | --- |
| Size | Fixed | Fixed, part of type | Dynamic |
| Owns elements | Yes | Yes | Yes |
| Knows size | Not after decay | Yes: `size()` | Yes: `size()` |
| Bounds-checked access | No | `at()` | `at()` |
| Copyable as whole object | No direct assignment | Yes | Yes |
| Contiguous | Yes | Yes | Yes |
| Can grow | No | No | Yes |
| C API interop | Direct pointer | `data()` | `data()` |

Use C arrays for low-level interop or legacy code. Use `std::array` for fixed
size. Use `std::vector` for dynamic size.

### 12.2 `vector` Versus `list`

| Question | `vector` | `list` |
| --- | --- | --- |
| Random access | O(1) | No |
| Iteration locality | Excellent | Poor |
| Insert at known middle position | O(n) shifting | O(1) node link |
| Find position | O(n) | O(n) |
| Per-element allocation | No | Usually yes |
| Iterator stability | Weaker | Stronger |

Default to `vector`. Choose `list` only when its node stability or special
operations are truly needed.

### 12.3 `map` Versus `unordered_map`

| Question | `map` | `unordered_map` |
| --- | --- | --- |
| Ordering | Sorted by key | No ordering |
| Lookup | O(log n) | O(1) average, O(n) worst |
| Range queries | Natural | Not suitable |
| Memory overhead | Node overhead | Buckets plus nodes |
| Customization | Comparator | Hash and equality |
| Invalidation | Stable except erased elements | Rehash invalidates iterators |

Use `map` when order matters. Use `unordered_map` when lookup dominates and
hash behavior is acceptable.

### 12.4 `set` Versus `unordered_set`

| Question | `set` | `unordered_set` |
| --- | --- | --- |
| Ordering | Sorted by key | No ordering |
| Lookup | O(log n) | O(1) average, O(n) worst |
| Range queries | Natural with ordered iterators | Not suitable |
| Memory overhead | Node overhead | Buckets plus nodes |
| Customization | Comparator | Hash and equality |
| Invalidation | Stable except erased elements | Rehash invalidates iterators |
| Main risk | Comparator must preserve ordering | Hash/equality quality and adversarial keys |

Use `set` when sorted iteration, range operations, or predictable logarithmic
behavior matter. Use `unordered_set` when membership testing dominates, order
does not matter, memory overhead is acceptable, and hash/equality are correct.
For small data sets, a sorted `vector` or even a linear scan can still be
faster because locality and allocation cost matter.

### 12.5 Iterator Versus Raw Pointer

A raw pointer can act like an iterator for a contiguous array:

```cpp
#include <algorithm>
#include <iostream>

int main()
{
    int values[] = {3, 1, 2};
    std::sort(values, values + 3);

    for (int value : values) {
        std::cout << value << '\n';
    }
}
```

But not every iterator is a pointer. A `list` iterator knows how to move
between nodes. A `map` iterator knows ordered traversal. Do not assume pointer
arithmetic works unless the iterator category supports it.

### 12.6 `std::string` Versus `char*`

| Feature | `char*` / C string | `std::string` |
| --- | --- | --- |
| Ownership | Manual or unclear | RAII owner |
| Size | Usually requires scan to `'\0'` | `size()` |
| Mutation | Buffer capacity must be managed | Managed |
| Safety | Easy to overflow or dangle | Safer but still requires references/views discipline |
| C interop | Native | `c_str()` |

Use `std::string` for owned text. Use `std::string_view` for borrowed text.
Use `char*` when working with C APIs or low-level buffers, with explicit
lifetime and size rules.

## 13. Common Bugs

### 13.1 Iterator Invalidated By `vector` Growth

```cpp
#include <vector>

int main()
{
    std::vector<int> values{1, 2, 3};
    auto it = values.begin();

    values.push_back(4); // may reallocate
    // int x = *it;      // undefined behavior if reallocation occurred
}
```

Refresh iterators after operations that may invalidate them.

### 13.2 `reserve()` Mistaken For `resize()`

```cpp
#include <vector>

int main()
{
    std::vector<int> values;
    values.reserve(8);
    // values[0] = 10; // no element exists
}
```

Use `push_back()` after `reserve()`, or use `resize()` to create elements.

### 13.3 Erasing While Iterating Incorrectly

```cpp
#include <vector>

int main()
{
    std::vector<int> values{1, 2, 3};

    for (auto it = values.begin(); it != values.end(); ++it) {
        if (*it == 2) {
            values.erase(it);
            break; // continuing would use invalid iterator
        }
    }
}
```

Use `it = erase(it)` if the loop continues.

### 13.4 Broken Comparator

```cpp
#include <algorithm>
#include <vector>

int main()
{
    std::vector<int> values{1, 2, 3};

    // Wrong: comp(a, a) is true, not a strict weak ordering.
    // std::sort(values.begin(), values.end(),
    //           [](int a, int b) { return a <= b; });
}
```

Use `<`, not `<=`, for ascending sort.

### 13.5 Binary Search On Unsorted Data

```cpp
#include <algorithm>
#include <vector>

int main()
{
    std::vector<int> values{10, 1, 7};
    // bool found = std::binary_search(values.begin(), values.end(), 7);
    // Wrong precondition: values is not sorted.
}
```

Sort first using the same comparison.

### 13.6 Accidental Map Insertion

```cpp
#include <map>
#include <string>

int main()
{
    std::map<std::string, int> values;
    int count = values["missing"]; // inserts "missing"
    return count;
}
```

Use `find()` or `at()` when insertion is not intended.

### 13.7 Hash And Equality Disagree

If `a == b`, then `hash(a)` must equal `hash(b)`. Violating this breaks
unordered containers.

### 13.8 Dangling `string_view` Or `span`

```cpp
#include <string>
#include <string_view>

std::string_view bad()
{
    std::string local = "temporary";
    return local; // dangling
}
```

Views are borrowed. They must not outlive the viewed object.

### 13.9 `while (!eof())`

```cpp
#include <fstream>
#include <iostream>

int main()
{
    std::ifstream file("input.txt");
    int value{};

    while (!file.eof()) {
        file >> value;              // may fail
        std::cout << value << '\n';  // may print stale value
    }
}
```

Correct:

```cpp
#include <fstream>
#include <iostream>

int main()
{
    std::ifstream file("input.txt");
    int value{};

    while (file >> value) {
        std::cout << value << '\n';
    }
}
```

## 14. Debugging

### 14.1 Strict Build

Use warnings first:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o main
```

Warnings catch many mistaken conversions, unused results, and suspicious
comparators.

### 14.2 Sanitizers

Use sanitizers for lifetime and bounds symptoms:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic \
    -fsanitize=address,undefined -g main.cpp -o main
```

Then run the program normally:

```sh
./main
```

Sanitizers are especially useful for use-after-free, out-of-bounds access, and
undefined behavior around invalid memory. They do not prove the code correct on
paths you did not execute.

### 14.3 Debug Iterator Modes

Some standard-library implementations offer debug iterator checks. With
libstdc++, `_GLIBCXX_DEBUG` can catch many invalid iterator operations:

```sh
g++ -std=c++17 -D_GLIBCXX_DEBUG -g main.cpp -o main
```

Use this for debugging only. It changes ABI and performance characteristics.

### 14.4 Inspect Container State

For `vector` bugs, log:

- `size()`;
- `capacity()`;
- `data()`;
- whether an operation can reallocate.

For unordered-container performance, log:

- `size()`;
- `bucket_count()`;
- `load_factor()`;
- `max_load_factor()`.

For file I/O, inspect:

- `is_open()`;
- `good()`;
- `fail()`;
- `bad()`;
- `eof()`.

### 14.5 Profile Before Changing Containers

If code is slow, do not blindly replace `vector` with `list` or `map` with
`unordered_map`. Measure the real workload.

Micro-benchmarks can lie if they do not match production data size, key
distribution, allocation behavior, and access patterns.

## 15. Best Practices

- Prefer `std::vector` unless a measured access pattern requires another
  container.
- Use `std::array` for fixed-size collections with STL operations.
- Call `reserve()` when the final vector or unordered-container size is known.
- Do not use `reserve()` as if it created elements.
- Use `at()` when bounds failure should be reported; use `operator[]` only when
  the index/key precondition is clear.
- Prefer algorithms when they express intent.
- Use `find()`, `contains()`, `at()`, `try_emplace()`, or `insert_or_assign()`
  to make map insertion intent clear.
- Keep comparators pure, cheap, and strict weak orderings.
- Keep hash and equality consistent.
- Treat iterators, references, pointers, `std::string_view`, and `std::span` as
  borrowed views with lifetime rules.
- Use erase-remove for sequence containers when removing by predicate.
- Use member functions when they are container-specific and clearer, such as
  `list::sort()` or `list::remove_if()`.
- Check file operations. Opening, reading, writing, flushing, and closing can
  fail.
- Avoid raw binary struct serialization for portable file formats.
- Use `std::filesystem::path` for modern path handling.
- Keep advanced features such as custom allocators, `std::pmr`, ranges, and
  parallel algorithms at the right depth unless the project truly needs them.

## 16. Interview Readiness

### Beginner

You should be able to answer:

- What are the main STL components?
- What is the difference between `size()` and `capacity()`?
- What is the difference between `reserve()` and `resize()`?
- Why is `std::vector` usually the default container?
- What does `std::sort` require?
- Why is `while (!eof())` wrong?

Model answer for `reserve()` versus `resize()`:

> `reserve()` changes capacity so future insertions can avoid reallocations. It
> does not create elements. `resize()` changes the number of elements by
> constructing or destroying elements.

### Mid-Level

You should be able to answer:

- Compare `vector` and `list`.
- Compare `map` and `unordered_map`.
- Explain iterator invalidation for `vector`.
- Explain the erase-remove idiom.
- Explain why `std::sort` cannot sort a `std::list`.
- Explain how custom hash and equality work.

Model answer for `vector` versus `list`:

> `vector` stores elements contiguously, so random access and iteration are
> fast and cache-friendly. Insertion in the middle can shift elements and
> invalidate iterators. `list` stores nodes separately, so insertion or erasure
> at a known position is cheap and other iterators remain stable, but finding
> the position is O(n), each node costs allocation and pointer overhead, and
> iteration is often slower due to poor locality.

### Senior

You should be able to answer:

- When can a linear scan over `vector` beat `unordered_set` lookup?
- What is the risk of storing iterators across mutations?
- How do comparator bugs break ordered containers or sorting?
- How would you design an API that accepts data without taking ownership?
- How do you control allocation and latency in embedded or real-time-adjacent
  C++ code?
- Why is raw binary serialization of structs dangerous?

Model answer for `map` versus `unordered_map`:

> `map` keeps keys ordered and gives O(log n) operations, so it supports sorted
> iteration and range-style access. `unordered_map` uses hashing and gives
> average O(1) operations, but no ordering, higher bucket memory overhead,
> rehash invalidation, and O(n) worst-case behavior if hashing is poor or
> adversarial. I choose based on ordering needs, lookup profile, memory,
> latency, and key/hash quality.

### Common Interview Traps

- Saying `unordered_map` is always O(1).
- Saying `list` is faster because insertion is O(1) without mentioning O(n)
  search and cache misses.
- Saying `reserve()` creates elements.
- Forgetting that `map::operator[]` inserts.
- Forgetting that `std::remove_if` does not erase.
- Using `<=` in a sort comparator.
- Assuming all iterators are raw pointers.
- Ignoring file stream failure states.

## 17. Practice

### Basic

- Store fixed pin numbers in `std::array` and print them with a range-for loop.
- Push samples into a `std::vector`, then print `size()` and `capacity()` after
  each push.
- Sort a vector of integers and use `std::binary_search`.
- Read integers from a text file using `while (file >> value)`.

### Intermediate

- Remove all failed sensor readings from a `vector` using erase-remove.
- Implement word counting with both `std::map` and `std::unordered_map`; compare
  output order.
- Build a `std::priority_queue` of tasks with priority and timestamp.
- Write a custom hash for a `DeviceId` struct and test duplicate insertion.
- Parse a CSV file line by line and report malformed rows.

### Advanced

- Benchmark `vector` linear search versus `unordered_set` lookup for small,
  medium, and large data sets.
- Design a command dispatcher using `unordered_map<std::string, std::function>`
  and explain ownership/lifetime of captured state.
- Write tests that intentionally trigger iterator invalidation under
  `_GLIBCXX_DEBUG`.
- Compare container choices for an LRU cache, event queue, routing table, and
  fixed sensor sample window.
- Build a small log analyzer using file streams, `vector`, `sort`, `find_if`,
  and `unordered_map`.

## 18. Summary

The Standard Library is not just a bag of convenient types. It is a design
system:

- containers own and organize values;
- iterators describe ranges;
- algorithms express operations;
- callables customize behavior;
- complexity and invalidation rules define correctness boundaries.

Use `std::vector` by default, but choose containers based on access pattern,
memory layout, ordering, iterator stability, and allocation behavior. Use
algorithms to express intent, but respect their preconditions. Treat views and
iterators as borrowed access. Treat file I/O as failure-prone.

The best STL code is usually simple, explicit about ownership, clear about
preconditions, and boring enough to survive maintenance.

## 19. Reference Notes

- Exact container invalidation rules are operation-specific. Check
  cppreference or the C++ standard when writing reference material or reviewing
  subtle code.
- `std::span` is C++20. `std::filesystem` is C++17. `std::string_view` is
  C++17.
- `std::vector` growth factor is implementation-defined. Rely on amortized
  complexity, not on a particular doubling rule.
- Standard containers do not make element access thread-safe. Separate
  synchronization is required for shared mutable access.
