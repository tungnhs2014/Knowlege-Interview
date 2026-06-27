# 11 - STL And Standard Library: Examples

## Status

These are **learning-focused C++17 examples**, not production-ready container
libraries, serializers, allocators, real-time components, or concurrent data
structures.

The suite demonstrates:

- choosing `std::array`, `std::vector`, `std::deque`, and `std::list`;
- using erase-remove safely on `std::vector`;
- choosing `std::map`, `std::unordered_map`, `std::unordered_set`, and
  `std::priority_queue`;
- writing a small custom hash and a priority-queue comparator;
- using `std::sort`, `std::binary_search`, `std::lower_bound`,
  `std::upper_bound`, `std::transform`, and `std::accumulate`;
- using file streams with explicit open/read/write/close checks;
- demonstrating safe iterator-refresh patterns after vector mutation;
- strict warnings, host-side ASan/UBSan, and libstdc++ debug iterator checks.

The examples are single-threaded host programs. They do not prove real-time
latency, allocation determinism, file-system robustness, race freedom, ABI
stability, or suitability for safety-critical software.

## Requirements

- GNU Make
- A C++17 compiler
- AddressSanitizer and UndefinedBehaviorSanitizer support
- Optional: GDB
- Optional: libstdc++ debug iterator mode for `_GLIBCXX_DEBUG`

## Build And Run

From this directory:

```bash
make check
```

Build without running:

```bash
make all
```

Run normal examples:

```bash
make run
```

Run sanitizer builds:

```bash
make sanitize
```

Build unoptimized debug binaries:

```bash
make debug
```

Run the vector invalidation example with libstdc++ debug iterators:

```bash
make debug-iter
```

Remove generated files:

```bash
make clean
```

Generated binaries and temporary input files are placed under `build/`.

## Example Map

| Target | Source | Main lesson |
| --- | --- | --- |
| `container_selection` | `container_selection.cpp` | Sequence-container selection and basic algorithms |
| `erase_remove` | `erase_remove.cpp` | Erase-remove idiom for `std::vector` |
| `associative_lookup` | `associative_lookup.cpp` | Ordered/unordered maps, custom hash, priority queue |
| `algorithms_files` | `algorithms_files.cpp` | Algorithms plus safe file-stream state checks |
| `invalidation_demo` | `invalidation_demo.cpp` | Avoiding stale iterators/pointers after vector mutation |

## 1. Container Selection

```bash
make build/container_selection
./build/container_selection
```

Expected output shape:

```text
array-sum=410 vector-size=4 deque-front=1 list-sorted=true result=passed
```

This example uses:

- `std::array` for fixed-size samples;
- `std::vector` for dynamic contiguous storage with `reserve()`;
- `std::deque` for efficient front/back operations;
- `std::list` for known-position insertion and iterator stability;
- `std::sort`, `std::accumulate`, and `std::is_sorted`.

**Production direction:** prefer `std::vector` unless a measured access pattern
requires another container. `std::list` insertion is O(1) only when you already
have the position; finding the position is still O(n).

**Iterator warning:** `std::vector` can invalidate iterators, references, and
pointers when it reallocates.

## 2. Erase-Remove

```bash
make build/erase_remove
./build/erase_remove
```

Expected output:

```text
42 44 45 0 result=passed
```

`std::remove_if` rearranges elements and returns a new logical end. It does not
change the container size. The following `erase()` call physically removes the
tail.

**Production direction:** this is a good pattern for sequence containers such
as `std::vector`, `std::deque`, and `std::string`. Associative containers need
container-specific erase patterns.

**Invalidation warning:** after `erase()`, old iterators at or after the erased
range are invalid. Do not continue using them.

## 3. Associative Lookup And Priority Queue

```bash
make build/associative_lookup
./build/associative_lookup
```

Expected output shape:

```text
ordered-first=error error-count=2 top-task=urgent result=passed
```

This example shows:

- `std::map` for sorted key iteration;
- `std::unordered_map` for average O(1) counting when ordering is unnecessary;
- `std::unordered_set` with a custom key, equality, and hash;
- `std::priority_queue` with a custom comparator.

**Production direction:** choose `map` when order or range behavior matters.
Choose `unordered_map` when lookup dominates, memory overhead is acceptable,
and hash/equality are correct.

**Hash warning:** if `a == b`, then `hash(a) == hash(b)` must be true.

**Comparator warning:** priority-queue comparator semantics are easy to reverse.
Test with small deterministic data.

## 4. Algorithms And File Streams

```bash
make build/algorithms_files
./build/algorithms_files
```

Expected output:

```text
count=5 sum=25 square-sum=163 twos=2 result=passed
```

The program writes a small input file, reads integers with `while (input >>
value)`, sorts the vector, uses binary-search algorithms, transforms values to
squares, and accumulates totals.

**Production direction:** file I/O is failure-prone. Check open, read, write,
flush, and close paths. Treat text parsing, EOF, format failure, and serious
I/O errors separately.

**File warning:** avoid `while (!eof())`. EOF is normally discovered after a
read attempt fails. Also, raw binary struct dumps are not portable
serialization when padding, endian, pointers, or versioning matter.

## 5. Vector Invalidation

```bash
make build/invalidation_demo
./build/invalidation_demo
```

Expected output shape:

```text
reallocated=<true-or-false> safe-first=1 size-after-erase=2 result=passed
```

The example keeps an index rather than dereferencing an old iterator or pointer
after `push_back()`. Whether the shown run prints `reallocated=true` depends on
the implementation's capacity after construction and `reserve(3)`; the lesson
is to treat any operation that can reallocate as invalidating until you have a
fresh iterator, pointer, or index-based access pattern. It also shows the safe
erase loop:

```cpp
it = values.erase(it);
```

**Production direction:** after a vector operation that may invalidate
iterators, either refresh the iterator or store a stable identifier/index when
that is valid for the problem.

**Undefined-behavior warning:** dereferencing an invalidated iterator or pointer
is undefined behavior. The normal suite does not intentionally execute that
bug.

## Sanitizer And Debug Experiments

The normal suite intentionally executes no undefined behavior:

```bash
make sanitize
```

For a local experiment, introduce exactly one defect at a time, then revert it:

- dereference a pointer saved before `vector::push_back()` when reallocation
  happens;
- use `values[0]` after `values.reserve(10)` while `size() == 0`;
- continue incrementing an iterator after `erase(it)`;
- use `<=` instead of `<` in a `std::sort` comparator;
- replace `while (input >> value)` with `while (!input.eof())`.

Then rerun:

```bash
make sanitize
make debug-iter
```

ASan/UBSan and debug iterators catch many executed mistakes, but a clean run is
not proof of correctness. They do not validate every algorithm precondition,
all file-system failure modes, race freedom, or real-time behavior.

## Debugging

Build debug binaries:

```bash
make debug
gdb ./build/invalidation_demo
```

Useful GDB commands:

```text
break main
run
next
print values.size()
print values.capacity()
print values.data()
continue
```

For unordered containers, inspect:

```text
print fast_counts.size()
print fast_counts.bucket_count()
print fast_counts.load_factor()
```

For file streams, inspect state after each operation:

```text
print input.good()
print input.fail()
print input.bad()
print input.eof()
```

## Review Checklist

- Does the chosen container match the access pattern?
- Is dynamic allocation acceptable for this target and phase?
- Are iterators, references, pointers, `string_view`, or `span` kept across
  invalidating operations?
- Does the comparator model strict weak ordering?
- Are custom hash and equality consistent?
- Are binary-search algorithms used only on correctly ordered ranges?
- Does file I/O check open, parse, write, flush, and close failure paths?
- Is the code single-threaded, or is there a documented synchronization policy?
