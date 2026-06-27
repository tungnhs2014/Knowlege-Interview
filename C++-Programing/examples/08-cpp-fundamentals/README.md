# 08 - C++ Fundamentals: Examples

## Status

These are **learning-focused C++17 and C17 examples**, not production-ready
libraries, ABI contracts, resource wrappers, or embedded firmware.

The suite demonstrates:

- a class invariant established by an `explicit` constructor;
- member initializer lists and const member functions;
- references, overload resolution, namespaces, and a small operator overload;
- safe value capture in a stateful lambda;
- deterministic destruction at scope exit;
- a C caller linked to a private C++ implementation through an opaque handle;
- strict compiler warnings and host-side ASan/UBSan execution.

The examples are deliberately small and single-threaded. They do not model a
complete error policy, exception-free embedded configuration, ABI versioning,
real-time constraints, or concurrent ownership.

## Requirements

- GNU Make
- A C++17 compiler
- A C17 compiler
- AddressSanitizer and UndefinedBehaviorSanitizer for `make sanitize`
- Optional: GDB, `nm`, and `c++filt`

Build, run, and sanitize every example:

```bash
make check
```

Build without running:

```bash
make all
```

Run the sanitizer builds only:

```bash
make sanitize
```

Remove generated files:

```bash
make clean
```

Generated files are placed under `build/`.

## Example Map

| Target | Source | Main lesson |
| --- | --- | --- |
| `fundamentals` | `fundamentals.cpp` | Class invariant, initialization, references, overloads, namespace, lambda, and operator |
| `lifetime` | `lifetime.cpp` | Scope-based destruction, borrowed reference member, deleted copying, and safe value capture |
| `c-api` | `c_api/counter.h`, `.cpp`, and `client.c` | Opaque handle, `extern "C"`, explicit ownership, and real C/C++ linking |
| `sanitize` | All runnable sources | ASan/UBSan execution of the exercised host paths |
| `symbols` | C++ C-API implementation object | Inspect external symbols and demangled names |

## 1. Class And Value Fundamentals

```bash
make fundamentals
./build/fundamentals
```

Expected output:

```text
alarms=2 invalid-rejected=true overflow-rejected=true result=passed
```

`fundamentals.cpp` demonstrates:

- a private invariant checked during construction;
- an `explicit` constructor;
- a `const` reference parameter;
- overloads for `Reading` and `int`;
- namespace qualification;
- a lambda used with `std::count_if`;
- a small units-like `operator+` with checked signed arithmetic;
- compile-time checking with `static_assert`.

**Exception-safety warning:** the example uses `std::out_of_range` to reject an
invalid reading and `std::overflow_error` to reject a sum outside the `int`
range. A project that disables exceptions needs a documented alternative such
as pre-validation or a checked factory/result. Do not allow exceptions to cross
a C ABI.

**Iterator warning:** the lambda does not mutate `readings`, so its iterators
remain valid. Container growth, erasure, or replacement during iteration could
invalidate iterators or references depending on the container and operation.

## 2. Lifetime And Lambda Capture

```bash
make lifetime
./build/lifetime
```

Expected output:

```text
active-after-scope=0 callback-state=2 result=passed
```

`ScopeCounter` borrows an external counter through an `int&`. The counter must
outlive every `ScopeCounter` that refers to it. Copying is deleted because a
copy would change the intended construction/destruction accounting.

The callback captures its state by value:

```cpp
return [count]() mutable {
    return ++count;
};
```

That closure owns its copy of `count`. Returning a lambda that captured the
local by reference would create a dangling reference and undefined behavior.
The suite discusses that unsafe version but does not execute it.

**Ownership warning:** a reference member is borrowed, not owned. Production
types must make the required owner lifetime clear in their API.

## 3. C API Backed By C++

```bash
make c-api
./build/c-api
```

Expected output:

```text
counter-value=5 overflow-rejected=true result=passed
```

The C header declares an incomplete `CounterHandle`. C code cannot access its
representation. The C++ implementation owns the definition and allocation.

The caller contract is:

- `counter_create` initializes `*out_handle` to null before allocation;
- successful creation transfers one owning handle to the caller;
- operation functions borrow the handle for the call;
- `counter_increment` returns `COUNTER_OVERFLOW` and leaves the value unchanged
  when the current value is `INT_MAX`;
- `counter_destroy` releases the handle and accepts null;
- the caller must not use the handle after destruction;
- the caller must destroy each successful handle exactly once.

**Ownership and ABI warning:** this is a teaching boundary, not a stable
production ABI. A real API needs versioning, visibility, calling-convention,
allocator/runtime, thread-safety, and error-detail policies. Do not expose C++
classes, exceptions, templates, or standard-library containers directly to C.

**Allocation warning:** the implementation uses `new (std::nothrow)` and
`delete`. Allocation and release must remain in a compatible implementation
module/runtime. Embedded projects may prohibit dynamic allocation and use
caller-provided storage or a fixed pool instead.

## Symbol Inspection

Build the C++ implementation object and inspect its symbols:

```bash
make symbols
```

The C-linkage functions should appear with C-compatible external names on common
toolchains. Exact symbol spelling, object format, and ABI behavior are
implementation-specific. `extern "C"` is a language-linkage declaration, not a
universal binary-compatibility guarantee.

## Sanitizers

```bash
make sanitize
```

The sanitizer builds enable AddressSanitizer and UndefinedBehaviorSanitizer.
They can expose selected executed host-side defects involving:

- invalid memory access;
- use after lifetime;
- allocation/deallocation misuse;
- alignment;
- invalid shifts and selected arithmetic undefined behavior.

A clean sanitizer run does not prove:

- that unexecuted paths are correct;
- that every borrow has a valid lifetime;
- exception safety under every failure;
- ABI compatibility across compilers or runtimes;
- absence of iterator invalidation in modified code;
- race freedom or deadlock freedom;
- target timing or embedded constraints.

ThreadSanitizer is not included because the examples create no threads.

## Debugging

Build unoptimized binaries with debug information:

```bash
make debug
gdb ./build/lifetime
```

Useful GDB commands:

```text
break ScopeCounter::ScopeCounter
break ScopeCounter::~ScopeCounter
run
print active_count
continue
```

Debug the C/C++ boundary:

```bash
gdb ./build/c-api
```

Useful commands:

```text
break counter_create
break counter_destroy
run
print out_handle
continue
```

For overload or linkage problems:

1. list visible declarations;
2. verify parameter types and language linkage;
3. compile C sources as C and C++ sources as C++;
4. inspect object symbols;
5. verify all required object files are linked.

## Safety And Production Notes

- Every example is learning-only and requires product-specific review.
- The suite executes no intentional undefined behavior.
- The C++ class example throws on invalid construction; exception policy is
  project-specific.
- The opaque C handle is uniquely owned by its caller after successful creation.
- The C API uses raw pointers because C has no references; nullability and
  ownership are explicitly documented.
- The lifetime example stores a borrowed reference whose owner must outlive it.
- The vector example performs no mutation during iteration. Future mutations
  must account for iterator and reference invalidation.
- No thread or lock is created. The examples prove neither race freedom nor
  deadlock prevention.
- The static and dynamic analysis results apply only to the exercised host
  configuration.
- No unsafe C string API is used.

## Production Checklist

- Define the supported C and C++ editions and compiler versions.
- Document construction failure and exception policy.
- State ownership, borrowing, nullability, and lifetime for every pointer or
  reference.
- Keep objects valid after successful construction.
- Design copy and move behavior before owning a resource.
- Keep operator overloads unsurprising.
- Avoid global using directives in headers.
- Treat `inline` as an ODR mechanism, not a speed promise.
- Keep C ABI types and functions narrow and versioned.
- Prevent exceptions from crossing C boundaries.
- Test invalid inputs, allocation failure, cleanup, and repeated operations.
- Run warnings and sanitizers on meaningful host tests.
- Add synchronization before sharing mutable state across threads.
- Verify target memory, timing, ABI, and toolchain assumptions separately.
