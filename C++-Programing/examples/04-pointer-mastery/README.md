# Pointer Mastery Examples

These examples support the learner-facing lesson in
`knowledge/04-pointer-mastery.md`. They are intentionally small and
compile-oriented.

## Status

- `basics/`, `api/`, `matrix/`, and `cpp/` are learning-focused safe examples.
- `unsafe/` contains intentional undefined behavior for sanitizer practice.
- The examples illustrate techniques, but they are not complete
  production-ready libraries.

## Requirements

- A C17 compiler such as GCC or Clang
- A C++20 compiler such as G++ or Clang++
- GNU Make
- AddressSanitizer and UndefinedBehaviorSanitizer support
- Optional: GDB or Valgrind

## Build And Run Everything

From this directory:

```sh
make
make check
```

`make check` builds and runs the safe programs, runs sanitized safe builds,
and confirms that the two intentionally unsafe programs fail with the
expected AddressSanitizer diagnostics.

Use another compiler when needed:

```sh
make clean
make check CC=clang CXX=clang++
```

Remove generated binaries and logs:

```sh
make clean
```

## Example Map

| Target | Source | Main lesson |
|---|---|---|
| `make basics` | `basics/pointer_array.c` | Arrays, pointer arithmetic, pointer-to-array, and `const` placement |
| `make api` | `api/output_callback.c` | Checked `T **` output parameters and callback context |
| `make matrix` | `matrix/matrix_layout.c` | Contiguous matrices versus arrays of row pointers |
| `make cpp` | `cpp/bounded_view.cpp` | Raw observers, `std::span`, `std::vector`, and `std::unique_ptr` |
| `make sanitize` | Safe sources | AddressSanitizer and UndefinedBehaviorSanitizer runs |
| `make expected-failures` | `unsafe/*.c` | Heap use-after-free and one-past-end write detection |

## 1. Pointer And Array Mechanics

Build and run:

```sh
make basics
./build/basics
```

The program contrasts:

- an array object with a pointer variable;
- element traversal using a half-open `[begin, end)` range;
- `int (*)[4]`, a pointer to an array of four `int`;
- pointer-to-`const` and `const` pointer declarations.

The `end` pointer may be formed and compared, but it must not be
dereferenced.

## 2. Output Parameters And Callbacks

Build and run:

```sh
make api
./build/api
```

`create_values` uses `int **out_values` to replace the caller's pointer only
after allocation succeeds. The caller owns the resulting allocation and must
call `free`.

The callback example passes a `void *context` explicitly. Production C APIs
should document:

- whether callback pointers may be null;
- the lifetime and mutability of the context;
- whether callbacks may retain pointers after returning;
- threading and reentrancy rules.

## 3. Matrix Layouts

Build and run:

```sh
make matrix
./build/matrix
```

The example compares:

- a contiguous `int[ROWS][COLS]` accessed through `int (*)[COLS]`;
- an array of row pointers, whose rows may be separate or ragged.

These representations are not interchangeable. A function expecting a
pointer to a fixed-width row needs the column bound in its type.

## 4. Modern C++ Views And Ownership

Build and run:

```sh
make cpp
./build/cpp-view
```

The example uses:

- `std::vector` as an owning dynamic sequence;
- `std::span` as a non-owning bounded view;
- `std::unique_ptr<int>` as an exclusive owner;
- a raw pointer as a temporary observer.

Do not let a `std::span`, iterator, reference, or raw pointer outlive its
source. A `std::vector` reallocation can invalidate all of them. Resetting or
destroying a `std::unique_ptr` invalidates observers of its former object.

RAII provides cleanup if allocation or later operations throw. This small
program does not define an application-level exception policy; production
code still needs one at subsystem boundaries.

## Debugging Commands

Build unoptimized binaries with debug information:

```sh
make debug
gdb ./build/api
```

Useful GDB commands:

```text
break create_values
run
next
print out_values
print *out_values
watch *out_values
continue
```

For the matrix example:

```sh
gdb ./build/matrix
```

```text
break print_contiguous
run
print rows
print rows[1][2]
ptype rows
```

## Sanitizers

Run the safe programs with AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
make sanitize
```

Run only the intentional failures:

```sh
make expected-failures
```

The generated reports are stored in:

```text
build/failures/stale-alias.log
build/failures/one-past-write.log
```

Never copy the unsafe patterns into normal code. Sanitizers improve defect
detection, but they do not make undefined behavior valid or guarantee that
every bad execution will be detected.

Valgrind can provide another check when available:

```sh
make all
valgrind --leak-check=full --track-origins=yes ./build/api
```

## Production Checklist

- Define who owns every allocation and which function releases it.
- Prefer bounded views such as pointer-plus-length or `std::span`.
- Validate null pointers, sizes, multiplication overflow, and API contracts.
- Treat one-past pointers as sentinels only; never dereference them.
- Recompute observers after operations that may invalidate them.
- Prefer RAII owners in C++ and failure-atomic output updates in C.
- Avoid unchecked C string and memory APIs when the destination capacity is
  unknown.
- Document callback lifetime, reentrancy, and concurrency guarantees.

These examples do not start threads, so they demonstrate no race or deadlock
behavior. Sharing pointed-to state between threads requires synchronization;
pointer validity alone says nothing about race freedom.

## Practice

1. Change `create_values` to create a caller-selected arithmetic sequence
   while preserving failure-atomic output behavior.
2. Add a transpose function for the contiguous matrix using a
   pointer-to-array parameter.
3. Deliberately retain a `std::span` across a `std::vector` reallocation,
   predict the failure, and inspect it with AddressSanitizer.
4. Replace the callback's `void *context` with a typed context structure that
   tracks both a sum and a count.
