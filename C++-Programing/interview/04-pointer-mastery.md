# 04 - Pointer Mastery: Interview Pack

## How To Use This Pack

For each question:

1. Start with the **Short answer**.
2. Add the **Deep explanation** when the interviewer asks why.
3. Ground the answer with the **C/C++ code/API anchor**.
4. Show engineering judgment through the **Production/debug angle**.
5. Avoid the listed **Common traps**.
6. Use the **Follow-up questions** to test the limits of the model.

The core rule throughout this pack is:

> A pointer is safe to dereference only when it designates a live object of an
> appropriate type, within bounds, with suitable alignment and permitted
> access.

## Beginner Questions

### 1. What is a pointer, and what is the difference between the pointer and the pointee?

**Short answer**

A pointer is an object whose value can designate another object or function, or
represent a null state. The pointer object has its own storage. The pointee is
the separate target accessed by dereferencing the pointer.

**Deep explanation**

For `int *ptr = &value`, there are two objects:

- `value`, an `int`;
- `ptr`, a pointer object storing a value that designates `value`.

`&ptr` is the address of the pointer object, `ptr` is its stored pointer value,
and `*ptr` is the designated `int`.

Copying `ptr` copies the pointer value, not the pointee. The result is another
alias to the same target. Pointer safety depends on lifetime, bounds, type,
alignment, and access permissions, not merely on the stored value being
non-null.

**C/C++ code/API anchor**

```c
int value = 42;
int *ptr = &value;
int *alias = ptr;

*alias = 50; /* value is now 50 */
```

```text
ptr ----+
        +----> value
alias --+
```

**Production/debug angle**

When debugging, inspect the pointer and pointee separately:

```text
p ptr
p *ptr
p &ptr
ptype ptr
```

For every important pointer, identify the owner, aliases, valid extent, and
invalidation events.

**Common traps**

- Saying a pointer is the memory it points to.
- Saying copying a pointer copies the target object.
- Assuming all pointer types have the same size on every implementation.
- Assuming a non-null pointer is valid.
- Treating a pointer as nothing more than an integer address.

**Follow-up questions**

- What does `&ptr` mean?
- Can two pointers designate the same object?
- What conditions must hold before dereference?
- Does raw pointer syntax express ownership?

### 2. Compare null, uninitialized, dangling, and one-past pointers.

**Short answer**

A null pointer designates no object. An uninitialized pointer has an
indeterminate value. A dangling pointer refers to an object whose lifetime has
ended. A one-past pointer marks the boundary after an array and may be used as a
sentinel, but not dereferenced.

**Deep explanation**

These states fail for different reasons:

| State | Meaning | Valid dereference? |
| --- | --- | --- |
| Null | Deliberately designates no target | No |
| Uninitialized | No usable pointer value was established | No |
| Dangling | Former target no longer exists | No |
| One-past | Valid array boundary, not an element | No |

A null check distinguishes null from non-null. It cannot prove that a non-null
pointer is live, in bounds, aligned, or correctly typed.

**C/C++ code/API anchor**

```c
int *null_ptr = NULL;
int *uninitialized;

int values[3] = {1, 2, 3};
int *one_past = values + 3;

int *owner = malloc(sizeof *owner);
int *alias = owner;
free(owner);
owner = NULL;

/* alias is dangling; one_past is not dereferenceable */
```

In modern C++, prefer `nullptr` for the null state.

**Production/debug angle**

AddressSanitizer detects many use-after-free and out-of-bounds accesses.
Compiler warnings and MemorySanitizer can help with uninitialized reads. A
lifetime or ownership review is still required to find stale aliases.

**Common traps**

- Calling every invalid pointer a null pointer.
- Saying dangling pointers always become null automatically.
- Dereferencing a one-past pointer.
- Printing or comparing an uninitialized pointer as if reading it were safe.
- Saying `owner = NULL` repairs every alias.

**Follow-up questions**

- Can a dangling pointer remain non-null?
- Why is one-past useful?
- Can a C++ reference dangle?
- Which tool would you try for use-after-free?

### 3. Why is an array not a pointer, and what is array-to-pointer conversion?

**Short answer**

An array is an object containing a fixed sequence of elements. A pointer is a
separate object storing a pointer value. In many expressions, an array
expression converts to a pointer to its first element, which is why they are
often confused.

**Deep explanation**

Given `int values[4]`, `values` has array type. In most expressions it converts
to `int *` designating `values[0]`. The conversion loses the array extent.

Important non-decay contexts include:

- `sizeof values`;
- `&values`;
- binding to an array reference in C++.

`values`, `&values[0]`, and `&values` can represent the same starting location,
but their types and arithmetic differ. `&values + 1` advances by one complete
array.

**C/C++ code/API anchor**

```c
int values[4] = {10, 20, 30, 40};

int *element_ptr = values;       /* pointer to first int */
int (*array_ptr)[4] = &values;   /* pointer to whole array */

sizeof values;       /* size of all four elements */
sizeof element_ptr;  /* size of the pointer object */
```

Array parameters are adjusted:

```c
void process(int values[10]); /* parameter type is int * */
```

**Production/debug angle**

Pointer-only APIs lose size information and invite out-of-bounds access.
Prefer pointer plus count in C and bounded views such as `std::span` in modern
C++.

**Common traps**

- Saying the array variable stores a pointer.
- Expecting `sizeof` on an array parameter to return the caller's array size.
- Treating `int **` as compatible with a two-dimensional array.
- Assuming `&values` has type `int *`.

**Follow-up questions**

- What is the type of `&values`?
- Why does an array parameter lose its extent?
- Which C++ interface can carry pointer and extent together?
- What does `array_ptr + 1` mean?

### 4. Explain the three important const-pointer declarations.

**Short answer**

`const int *p` is a pointer to const `int`. `int * const p` is a const pointer
to mutable `int`. `const int * const p` makes both the pointer and access to the
pointee const through `p`.

**Deep explanation**

Qualification can apply to the pointed-to type or the pointer object:

| Declaration | Modify pointee through `p`? | Reseat `p`? |
| --- | --- | --- |
| `const int *p` | No | Yes |
| `int * const p` | Yes | No |
| `const int * const p` | No | No |

Top-level const on a pointer passed by value affects only the function's local
pointer copy. Pointee const is the part that communicates a useful read-only
API contract to callers.

**C/C++ code/API anchor**

```c
int first = 10;
int second = 20;

const int *read_only = &first;
read_only = &second;       /* allowed */

int * const fixed = &first;
*fixed = 30;               /* allowed */

const int * const fixed_read_only = &first;
```

Read from the variable name outward.

**Production/debug angle**

Const-correct interfaces reduce accidental writes and make code review easier.
Warnings such as `-Wcast-qual` help identify casts that erase qualification.

**Common traps**

- Treating `const int *` and `int * const` as equivalent.
- Saying const makes the object immutable through every possible alias.
- Casting away const and writing to an originally const object.
- Believing top-level const on a by-value parameter changes the caller.

**Follow-up questions**

- Is `int const *p` different from `const int *p`?
- Can the original non-const object change through another alias?
- What happens if code modifies an originally const object through a cast?
- Which const form belongs in a read-only buffer API?

### 5. Compare pointers and C++ references.

**Short answer**

Pointers can represent null, be reseated, and participate in valid array
arithmetic. References use direct access syntax and usually express a required
alias that cannot be reseated. Both can dangle.

**Deep explanation**

Use a reference when C++ code requires an existing object and reseating is not
part of the design. Use a pointer when absence, reseating, pointer identity,
array traversal, or C interoperability matters.

A reference is not an ownership type and does not guarantee that the target
outlives the reference.

**C/C++ code/API anchor**

```cpp
void required(int &value)
{
    value = 10;
}

void optional(int *value)
{
    if (value != nullptr) {
        *value = 10;
    }
}
```

Unsafe reference return:

```cpp
int &bad()
{
    int local = 42;
    return local; // dangling reference
}
```

**Production/debug angle**

Parameter type should communicate nullability. During review, check lifetime
for both pointers and references, especially returned references and callbacks
capturing references.

**Common traps**

- Saying references cannot dangle.
- Saying a reference necessarily has no storage in the implementation.
- Using a reference when absence is a valid state.
- Using a nullable pointer without documenting whether null is allowed.

**Follow-up questions**

- Can a reference be reseated?
- When is a pointer clearer than a reference?
- How would `std::span` differ from both?
- Does either type express ownership?

## Mid-Level Questions

### 6. What are the exact limits of pointer arithmetic and comparison?

**Short answer**

Pointer arithmetic is defined within one array object and its one-past
boundary. Pointer subtraction and ordered comparison require the relevant
same-array relationship. A one-past pointer may be used as a boundary but not
dereferenced.

**Deep explanation**

For `T array[N]`, valid positions for arithmetic range from `&array[0]` through
`array + N`. Only the first `N` positions designate elements.

`p + n` advances by elements of `T`, not bytes. Subtracting two pointers yields
an element distance only when they belong to the same array domain and the
result is representable as `ptrdiff_t`.

Two separate variables written next to each other in source are not an array.
Numerically adjacent addresses do not create a language-level relationship.

**C/C++ code/API anchor**

```c
#include <stddef.h>

int values[8];
int *first = &values[2];
int *last = &values[7];
ptrdiff_t distance = last - first; /* 5 */

int *end = values + 8;
/* *end is invalid */
```

Use equality for null and identity checks. Do not use `<` to establish a
portable order between unrelated allocations.

**Production/debug angle**

ASan can identify many out-of-bounds accesses, but API design should carry
bounds from the start. Review loops for off-by-one errors and validate offsets
before forming or dereferencing pointers.

**Common traps**

- Saying any pointer can be incremented.
- Dereferencing one-past.
- Subtracting pointers from different allocations.
- Using ordered comparison for arbitrary object addresses.
- Performing arithmetic on standard C `void *`.

**Follow-up questions**

- Is forming an out-of-bounds pointer safe if it is never dereferenced?
- What type holds a pointer difference?
- Why is `values[i]` equivalent to `*(values + i)`?
- How would you represent a circular buffer position safely?

### 7. Why would a C API use `T **`, and what contract must it define?

**Short answer**

A C API uses `T **` when it must modify the caller's `T *`, commonly to return
an allocated object or replace an existing pointer. The API must define
nullability, output state on failure, ownership transfer, extent, and release
function.

**Deep explanation**

C passes a pointer argument by value. A function receiving `T *` can modify the
pointee but cannot replace the caller's pointer object. Passing `&caller_ptr`
as `T **` gives the function access to that object.

`T **` is not automatically a two-dimensional array. It may designate one
pointer object, the first element of an array of pointers, or another
contract-specific layout.

**C/C++ code/API anchor**

```c
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

int create_values(size_t count, int **out_values)
{
    if (out_values == NULL) {
        return -1;
    }

    *out_values = NULL;

    if (count == 0U || count > SIZE_MAX / sizeof **out_values) {
        return -1;
    }

    int *values = malloc(count * sizeof *values);
    if (values == NULL) {
        return -1;
    }

    *out_values = values;
    return 0;
}
```

**Production/debug angle**

Set outputs to a documented failure value before operations can fail. Review
every exit path for ownership. In modern C++, prefer a return value or
`std::unique_ptr` when that expresses the contract more directly.

**Common traps**

- Passing `T *` and expecting reassignment to affect the caller.
- Leaving `*out` indeterminate on failure.
- Failing to validate `out`.
- Forgetting allocation-size overflow.
- Treating `T **` as compatible with `T (*)[N]`.

**Follow-up questions**

- What should `*out_values` contain after failure?
- Who calls `free` after success?
- How would the C++ API differ?
- When is a result structure clearer than multiple output parameters?

### 8. Design a safe pointer-and-length buffer API.

**Short answer**

The API must carry an extent and define nullability, mutability, capacity,
produced length, overlap behavior, ownership, and failure-state behavior.

**Deep explanation**

A pointer alone does not carry bounds. Input buffers normally use
`const T *data, size_t size`. Output buffers need both capacity and an output
length. The contract must say whether null is accepted when size is zero.

Size validation must occur before pointer arithmetic and access. For text,
specify whether capacity includes the terminator. For overlapping regions,
either forbid overlap or use an API whose semantics support it.

**C/C++ code/API anchor**

```c
int encode_packet(unsigned char *output,
                  size_t capacity,
                  size_t *out_size,
                  const unsigned char *input,
                  size_t input_size);
```

Possible contract:

- `out_size` is required;
- `*out_size` becomes zero on failure;
- `input` may be null only when `input_size == 0`;
- `output` may be null only when `capacity == 0`;
- no more than `capacity` bytes are written;
- input and output must not overlap.

Modern C++:

```cpp
int encode(std::span<std::byte> output,
           std::span<const std::byte> input,
           std::size_t &written);
```

**Production/debug angle**

Fuzz sizes around zero, one, exact capacity, and one beyond capacity. Run ASan
and test integer overflow before allocations or pointer offset calculations.

**Common traps**

- Using a pointer-only API.
- Confusing capacity with current length.
- Checking bounds after writing.
- Forgetting integer overflow in `offset + required`.
- Assuming `std::span` owns or extends the underlying lifetime.

**Follow-up questions**

- Would you permit null with zero length?
- How do you prevent `offset + size` overflow?
- How should partial output be reported?
- When is begin/end clearer than pointer/count?

### 9. Explain function pointers and the callback-plus-context pattern.

**Short answer**

A function pointer designates a function with an exact return and parameter
type. A C callback API often pairs it with `void *context` so one callback can
operate on caller-specific state without globals.

**Deep explanation**

The callback type is part of the contract. Calling through an incompatible
function pointer type is undefined behavior; casting does not make a mismatched
function safe.

The context pointer erases static type information, so the callback and caller
must agree on its type, alignment, lifetime, and ownership. A synchronous
callback can often borrow a local context. A stored or asynchronous callback
needs registration, cancellation, teardown, and concurrency rules.

**C/C++ code/API anchor**

```c
typedef void (*value_callback)(int value, void *context);

void notify(value_callback callback, void *context)
{
    if (callback != NULL) {
        callback(42, context);
    }
}

struct Counter {
    int calls;
};

void count_call(int value, void *context)
{
    struct Counter *counter = context;
    if (counter != NULL) {
        counter->calls += value != 0;
    }
}
```

A noncapturing C++ lambda can convert to a compatible function pointer.
Capturing lambdas require a richer callable representation.

**Production/debug angle**

Many callback bugs are context-lifetime bugs rather than invalid function
pointers. Trace registration, retention, invocation, unregistration, and
destruction order. Use thread sanitizers when callback teardown can race with
invocation.

**Common traps**

- Casting an incompatible function to the expected callback type.
- Retaining a pointer to a local context after the registering call returns.
- Destroying context before unregistering the callback.
- Assuming a valid callback function implies valid context.
- Ignoring reentrancy and execution-context requirements.

**Follow-up questions**

- Can a capturing lambda convert to a function pointer?
- Who owns the context?
- May the callback retain pointers passed during invocation?
- How would you prevent invocation during teardown?

### 10. Compare `T **`, `T (*)[N]`, and a flat `T *` matrix.

**Short answer**

`T **` is pointer to pointer, often used for pointer replacement or separately
stored row pointers. `T (*)[N]` points to contiguous arrays of `N` elements.
A flat `T *` points into one linear sequence indexed manually. They are not
interchangeable.

**Deep explanation**

A contiguous `T rows[R][N]` converts to `T (*)[N]`. Each increment advances one
complete row. A `T **` matrix normally requires an outer pointer sequence and
separate row pointer values; rows may be scattered or have different lengths.

A flat allocation `T *data` stores `rows * columns` elements contiguously and
uses `data[row * columns + column]`. It has one allocation and one release,
usually providing simpler cleanup and better locality.

**C/C++ code/API anchor**

```c
void print_rows(size_t rows, const int matrix[][4]);

int contiguous[3][4];
print_rows(3U, contiguous);
```

Flat representation:

```c
int *matrix = malloc(rows * columns * sizeof *matrix);
int value = matrix[row * columns + column];
```

Pointer-row representation:

```c
int **rows = malloc(row_count * sizeof *rows);
```

Each row then needs its own allocation and cleanup.

**Production/debug angle**

Prefer contiguous storage unless ragged rows are a real requirement. For
multi-allocation designs, test partial-allocation failure and release every
successfully allocated row.

**Common traps**

- Casting a contiguous 2D array to `T **`.
- Forgetting multiplication overflow for flat storage.
- Leaking rows after partial failure.
- Calling scattered rows cache-equivalent to contiguous storage.
- Using `delete` instead of `delete[]` in raw C++ array code.

**Follow-up questions**

- Which representation needs one allocation?
- How do you pass a contiguous C matrix to a function?
- When are ragged rows useful?
- How would `std::vector<T>` represent a flat C++ matrix?

## Senior Questions

### 11. Why is a non-null pointer still not proof of a valid access?

**Short answer**

Non-null proves only that the pointer does not hold a null pointer value. It may
still dangle, be out of bounds, be misaligned, designate an object of an
incompatible type, lack write permission, or violate an ownership contract.

**Deep explanation**

Pointer validity is operation-specific:

- a one-past pointer is valid for a boundary comparison but not dereference;
- a pointer to const permits reads but not writes through that path;
- a pointer to freed storage is non-null but dangling;
- a converted `void *` needs the correct target type and alignment;
- an observer may designate a live object but still be forbidden to release it.

The language's abstract rules allow optimizers to assume that undefined cases
do not occur. Therefore, "the address looks mapped" is not a correctness proof.

**C/C++ code/API anchor**

```c
int *owner = malloc(sizeof *owner);
int *observer = owner;

free(owner);
owner = NULL;

if (observer != NULL) {
    /* *observer is still undefined behavior */
}
```

Misaligned access:

```c
unsigned char bytes[8];
int *value = (int *)(bytes + 1);
/* dereference may violate alignment and object-access rules */
```

**Production/debug angle**

Build a pointer ledger: creation, owner, borrowers, extent, type/alignment,
invalidation, and release. Use ASan/UBSan and compare optimized builds, but fix
the first invalid operation rather than the final crash.

**Common traps**

- Treating readable virtual memory as a live C/C++ object.
- Checking only null before every dereference.
- Assuming a cast establishes alignment or object lifetime.
- Treating ownership permission as equivalent to address validity.

**Follow-up questions**

- Can a pointer be valid for comparison but invalid for dereference?
- What invalidates a pointer without calling `free`?
- Why can the bug appear only at `-O2`?
- What does a sanitizer fail to prove?

### 12. Explain ownership, borrowing, alias invalidation, and the raw-pointer role in modern C++.

**Short answer**

Ownership determines who controls lifetime and release. A borrowed pointer
provides temporary non-owning access. Releasing or moving the owner can
invalidate every alias. Modern C++ normally uses RAII ownership types and raw
pointers or references for non-owning access.

**Deep explanation**

Raw pointer syntax does not say whether the pointer owns, borrows, is optional,
or is an output destination. The contract must.

A single owner can have many observers. When the owner releases or relocates
the object, observers may dangle. Setting the owner to null changes only that
pointer object.

In C++, `std::unique_ptr` expresses exclusive ownership. `std::shared_ptr`
expresses shared ownership when needed, but can leak through cycles and does
not make concurrent access to the managed object automatically safe.
`std::span`, references, and raw pointers are non-owning views whose underlying
objects must outlive their use.

**C/C++ code/API anchor**

```cpp
#include <memory>

auto owner = std::make_unique<int>(42);
int *observer = owner.get();

owner.reset();
/* observer is now dangling */
```

C opaque owner:

```c
struct Sensor *sensor_create(int id);
int sensor_read(const struct Sensor *sensor, int *out_value);
void sensor_destroy(struct Sensor *sensor);
```

**Production/debug angle**

During review, annotate every pointer as owner, borrowed required, borrowed
optional, output, or opaque handle. Check container reallocation, object moves,
callback retention, and teardown paths for invalidation.

**Common traps**

- Replacing every raw pointer with `shared_ptr`.
- Saying smart pointers eliminate all leaks and dangling observers.
- Returning an owning raw pointer without a release contract.
- Letting a view outlive its container.
- Confusing control-block thread safety with managed-object thread safety.

**Follow-up questions**

- When is `unique_ptr` preferable to `shared_ptr`?
- Is `std::span` an ownership type?
- What invalidates pointers into `std::vector`?
- How do you represent ownership in a C API?

### 13. Explain alignment, incompatible typed access, strict aliasing, and `void *`.

**Short answer**

`void *` erases static pointed-to type but not the requirements for a live
object, correct alignment, sufficient storage, valid bounds, and permitted
typed access. Dereferencing a misaligned or incompatibly typed pointer can be
undefined behavior.

**Deep explanation**

C permits implicit conversion between `void *` and object pointer types. C++
requires explicit conversion from `void *` back to a typed pointer. Neither
language treats the conversion as proof that an object of the target type
exists at that address.

Strict aliasing rules constrain which lvalue types may access an object's stored
value. Character types may inspect object representation. For transferring
representation between declared objects, `memcpy` is the conservative tool.

**C/C++ code/API anchor**

Unsafe:

```c
float value = 1.0F;
uint32_t bits = *(uint32_t *)&value;
```

Representation copy:

```c
#include <stdint.h>
#include <string.h>

float value = 1.0F;
uint32_t bits;

_Static_assert(sizeof bits == sizeof value,
               "example requires equal-sized types");
memcpy(&bits, &value, sizeof bits);
```

The resulting representation remains implementation-dependent.

**Production/debug angle**

Enable `-Wcast-align`, `-Wcast-qual`, useful strict-aliasing warnings, and
UBSan. Review packet parsers and byte-buffer casts carefully, especially on
targets with stricter alignment than the developer workstation.

**Common traps**

- Saying `void *` can be dereferenced directly.
- Believing a cast creates an object or fixes alignment.
- Using pointer casts as portable serialization.
- Assuming all targets tolerate unaligned access.
- Treating object and function pointers as interchangeable.

**Follow-up questions**

- Why is `memcpy` preferable for representation transfer?
- Can `unsigned char *` inspect object bytes?
- What does placement construction add in advanced C++?
- Why can code work on x86 and fail elsewhere?

### 14. What does `restrict` mean in C, and how can violating it cause undefined behavior?

**Short answer**

`restrict` is an access-association promise that lets the implementation assume
certain accesses do not alias during an execution. If the caller violates the
required access relationship, behavior can be undefined.

**Deep explanation**

`restrict` is not a runtime check and does not mean that no pointer with the
same numeric address exists anywhere. It constrains how objects are accessed
through restricted pointer associations during the relevant execution.

The optimizer may reorder, vectorize, or combine memory operations based on the
promise. Overlapping input and output regions can break that promise.

**C/C++ code/API anchor**

```c
void add_arrays(size_t count,
                int * restrict output,
                const int * restrict left,
                const int * restrict right)
{
    for (size_t i = 0; i < count; ++i) {
        output[i] = left[i] + right[i];
    }
}
```

The API must document which overlap patterns are forbidden.

**Production/debug angle**

Use `restrict` only when profiling shows value and the caller contract is
realistic. Test overlap cases separately and compare optimized behavior. A bug
that appears only with vectorization may be a contract violation rather than a
compiler defect.

**Common traps**

- Describing `restrict` as "this pointer is unique."
- Adding it without updating the public contract.
- Assuming debug builds validate the promise.
- Applying it mechanically to every pointer parameter.
- Treating C++ compiler extensions as identical to standard C semantics.

**Follow-up questions**

- Is `restrict` checked at runtime?
- Can two restricted pointers ever hold the same value?
- Why does optimization expose violations?
- What alternative API avoids overlap ambiguity?

### 15. Explain pointer provenance and why arbitrary pointer/integer round trips are risky.

**Short answer**

Pointer provenance models associate a pointer with the object or allocation
from which it originated, not only a numeric address. Arbitrary integer
arithmetic and reconstruction may fail to preserve a pointer that the language
permits for object access.

**Deep explanation**

The simple machine model says a pointer is an address. Modern optimization,
segmented or capability architectures, and language object rules require a
richer model.

Integer types such as `uintptr_t`, when provided, support implementation-defined
pointer representation conversions for specific purposes, but do not make
arbitrary arithmetic a portable way to manufacture valid object access.

The practical rule is to derive pointers through valid language operations,
keep arithmetic within the originating object, and use implementation
documentation for allocators, runtimes, serialization, or hardware-specific
address handling.

**C/C++ code/API anchor**

```c
#include <stdint.h>

uintptr_t raw = (uintptr_t)ptr;
/* arbitrary arithmetic on raw is not a portable object-navigation API */
void *restored = (void *)raw;
```

Integer conversion can also lose information on implementations without a
suitable integer type or with metadata-bearing pointers.

**Production/debug angle**

Treat pointer tagging, custom allocators, shared-memory encodings, and persisted
addresses as platform-specific designs. Document ABI, compiler, architecture,
and optimization assumptions and test them explicitly.

**Common traps**

- Saying `uintptr_t` guarantees every transformed integer becomes a valid
  pointer.
- Persisting raw process addresses to disk.
- Treating numeric equality as full object-access validity.
- Teaching an evolving provenance model as one simple universal rule.

**Follow-up questions**

- Why might a capability pointer contain more than an address?
- When is pointer-to-integer conversion useful?
- How should shared-memory data represent references?
- Which assumptions belong in implementation documentation?

## Coding Tasks

### Task 1. Implement A Bounded Search API

**Prompt**

Implement:

```c
int find_value(const int *values,
               size_t count,
               int target,
               size_t *out_index);
```

Return `0` on success and `-1` when the input is invalid or the target is not
found. Define the output state on every path.

**Model solution**

```c
#include <stddef.h>

int find_value(const int *values,
               size_t count,
               int target,
               size_t *out_index)
{
    if (out_index == NULL) {
        return -1;
    }

    *out_index = 0U;

    if (values == NULL && count != 0U) {
        return -1;
    }

    for (size_t i = 0; i < count; ++i) {
        if (values[i] == target) {
            *out_index = i;
            return 0;
        }
    }

    return -1;
}
```

**What a strong candidate explains**

- Pointer and count form one range contract.
- Null with zero count is explicitly accepted here.
- `out_index` is required and initialized before failure.
- No one-past pointer is dereferenced.
- A richer status enum could distinguish invalid input from not found.

**Production/debug angle**

Test zero count, null/zero, null/nonzero, first, middle, last, and missing
targets. Run with ASan and use fuzzed counts only when the caller cannot lie
about the actual allocation extent.

**Common traps**

- Accessing `values[0]` before validating the contract.
- Looping with `i <= count`.
- Leaving `*out_index` unchanged on failure without documenting it.
- Believing the count proves the allocation is actually that large.

**Follow-ups**

- Return a pointer to the found element instead.
- Write a `const int *begin, *end` version.
- Write a C++ `std::span<const int>` version.

### Task 2. Implement A C Allocation Output API

**Prompt**

Implement a function that allocates `count` zero-initialized `int` elements
through `int **out_values`.

**Model solution**

```c
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

int create_zeroed_values(size_t count, int **out_values)
{
    if (out_values == NULL) {
        return -1;
    }

    *out_values = NULL;

    if (count == 0U || count > SIZE_MAX / sizeof **out_values) {
        return -1;
    }

    int *values = calloc(count, sizeof *values);
    if (values == NULL) {
        return -1;
    }

    *out_values = values;
    return 0;
}
```

**What a strong candidate explains**

- `out_values` is validated before dereference.
- The failure state is deterministic.
- Multiplication overflow is rejected.
- Ownership transfers only on success.
- The caller releases the result with `free`.
- This API deliberately rejects zero-size requests.

**Production/debug angle**

Inject allocation failure, verify no leaks, and document allocator pairing.
For C++, prefer `std::vector<int>` or an ownership type rather than exposing a
raw owning pointer.

**Common traps**

- Casting `calloc` in C.
- Assigning into `*out_values` before success without a clear failure state.
- Forgetting overflow.
- Using `delete[]` to release the result.

**Follow-ups**

- Add a paired `destroy_values`.
- Return a structure containing pointer and count.
- Implement the C++ equivalent using `std::vector<int>`.

### Task 3. Implement A Callback Iterator With Context

**Prompt**

Visit each element and invoke a callback with caller-provided context.

**Model solution**

```c
#include <stddef.h>

typedef int (*visit_callback)(int value, void *context);

int visit_values(const int *values,
                 size_t count,
                 visit_callback callback,
                 void *context)
{
    if (callback == NULL || (values == NULL && count != 0U)) {
        return -1;
    }

    for (size_t i = 0; i < count; ++i) {
        if (callback(values[i], context) != 0) {
            return 1; /* stopped by callback */
        }
    }

    return 0;
}
```

**What a strong candidate explains**

- The callback signature is exact.
- `context` may be null if the callback permits it.
- This function is synchronous and does not retain either pointer.
- The return contract distinguishes invalid input from callback-requested stop.

**Production/debug angle**

For retained callbacks, this implementation is insufficient; registration,
unregistration, lifetime, concurrency, and reentrancy rules become mandatory.

**Common traps**

- Retaining a pointer to a caller's local context.
- Calling a null callback.
- Casting incompatible callback functions.
- Ignoring callback failure or early-stop semantics.

**Follow-ups**

- Make the callback receive the element index.
- Add mutable traversal.
- Design an asynchronous registration API and teardown protocol.

### Task 4. Refactor A Raw C++ Range

**Prompt**

Refactor:

```cpp
int sum(const int *values, std::size_t count);
```

to a bounded modern C++ interface.

**Model solution**

```cpp
#include <span>

int sum(std::span<const int> values)
{
    int total = 0;
    for (int value : values) {
        total += value;
    }
    return total;
}
```

**What a strong candidate explains**

- `std::span` carries pointer and extent.
- It is non-owning and does not extend lifetime.
- `const int` prevents mutation through the view.
- Integer overflow in `total` remains a separate concern.

**Production/debug angle**

The caller must not pass a span into storage that expires before use. Avoid
storing spans unless the ownership and invalidation model is explicit.

**Common traps**

- Calling `std::span` an owning container.
- Returning a span into a local array.
- Assuming the span remains valid after `std::vector` reallocation.

**Follow-ups**

- Make the sum overflow-aware.
- Accept any contiguous range with templates.
- Compare `span`, iterator pair, and container reference.

## Debugging Scenarios

### Scenario 1. The Owner Is Null, But ASan Reports Use-After-Free

```c
int *owner = malloc(sizeof *owner);
if (owner == NULL) {
    return;
}

int *cache = owner;

*owner = 42;
free(owner);
owner = NULL;

printf("%d\n", *cache);
```

**Short answer**

`cache` is a dangling alias. Resetting `owner` changes only one pointer object.
Dereferencing `cache` is use-after-free.

**Deep explanation**

Both pointers copied the same pointer value. `free` ended the allocation's
lifetime. No alias is automatically updated. The null state of `owner` says
nothing about `cache`.

**C/C++ code/API anchor**

Use one owner and ensure every observer's use ends before release. In modern
C++, an owning `std::unique_ptr` can clarify ownership, but raw observers still
require correct lifetime.

**Production/debug angle**

Use the ASan allocation and free stack traces, then search for all aliases
created before the free. Fix the ownership protocol rather than adding more
null checks.

**Common traps**

- Checking `owner` before using `cache`.
- Setting every visible alias to null manually as the primary design.
- Assuming smart pointers automatically fix escaped raw observers.

**Follow-ups**

- How would you redesign the cache?
- What if the observer is stored in a callback context?

### Scenario 2. A Loop Writes One Element Too Many

```c
void clear(int *values, size_t count)
{
    for (size_t i = 0; i <= count; ++i) {
        values[i] = 0;
    }
}
```

**Short answer**

The condition must be `i < count`. Index `count` is one-past and cannot be
accessed.

**Deep explanation**

For `count` elements, valid indices are zero through `count - 1`. The one-past
position is useful for termination, not dereference. If `count` is `SIZE_MAX`,
the increment also creates a separate wraparound concern.

**C/C++ code/API anchor**

```c
for (size_t i = 0; i < count; ++i) {
    values[i] = 0;
}
```

Validate the pointer/count contract before entering the loop.

**Production/debug angle**

ASan normally reports the first out-of-bounds write. Add boundary tests for
zero, one, and exact capacity.

**Common traps**

- Allocating `count + 1` to hide the bug.
- Assuming the extra write is harmless because padding exists.
- Checking the bound after writing.

**Follow-ups**

- Rewrite with begin/end pointers.
- What changes if null plus zero is part of the API contract?

### Scenario 3. `realloc` Makes An Interior Pointer Stale

```c
int *values = malloc(4U * sizeof *values);
if (values == NULL) {
    return;
}

int *selected = &values[2];

int *resized = realloc(values, 8U * sizeof *values);
if (resized != NULL) {
    values = resized;
    *selected = 7;
}
```

**Short answer**

After successful `realloc`, `selected` must not be used. The allocation may
have moved, and the old allocation's pointer values and aliases are stale.

**Deep explanation**

Even if `realloc` appears to return the same numeric address in one run, code
must follow the successful-resize lifetime rules. Recompute interior positions
from an index after success.

**C/C++ code/API anchor**

```c
size_t selected_index = 2U;
int *resized = realloc(values, 8U * sizeof *values);
if (resized == NULL) {
    free(values);
    return;
}

values = resized;
values[selected_index] = 7;
free(values);
```

**Production/debug angle**

Review all aliases, iterators, and interior pointers across growth operations.
In C++, `std::vector` reallocation has a similar invalidation concern.

**Common traps**

- Updating only the owner pointer.
- Assuming in-place growth is guaranteed.
- Assigning `realloc` directly to `values` and leaking on failure.

**Follow-ups**

- What remains valid when `realloc` fails?
- Which `std::vector` operations invalidate pointers?

### Scenario 4. A Packet Parser Works On One CPU And Fails On Another

```c
uint32_t read_word(const unsigned char *data)
{
    return *(const uint32_t *)data;
}
```

**Short answer**

The cast and dereference may violate alignment, bounds, typed-access, and byte
order requirements. Decode explicit bytes or use `memcpy` plus defined endian
conversion.

**Deep explanation**

`data` may not be aligned for `uint32_t`, may contain fewer than four bytes, and
does not necessarily contain a live `uint32_t` object. Even if the load works,
native byte order may not match the protocol.

**C/C++ code/API anchor**

```c
int read_u32_be(const unsigned char *data,
                size_t size,
                uint32_t *out)
{
    if (data == NULL || out == NULL || size < 4U) {
        return -1;
    }

    *out = ((uint32_t)data[0] << 24)
         | ((uint32_t)data[1] << 16)
         | ((uint32_t)data[2] << 8)
         | (uint32_t)data[3];
    return 0;
}
```

**Production/debug angle**

Run UBSan and test on strict-alignment targets or emulation. Fuzz truncated
inputs. Keep protocol representation independent of native object layout.

**Common traps**

- Adding only a bounds check.
- Using a packed structure overlay as a universal fix.
- Assuming x86 behavior is portable.
- Forgetting endian conversion.

**Follow-ups**

- When would `memcpy` be appropriate?
- Why does `void *` not solve this?

### Scenario 5. Callback Crashes During Shutdown

```c
struct Session {
    int active;
};

void on_event(int event, void *context)
{
    struct Session *session = context;
    session->active = event != 0;
}
```

The registration system may invoke `on_event` after the `Session` object has
been destroyed.

**Short answer**

The callback function is valid, but its retained context is dangling. Shutdown
must stop or unregister future invocations before destroying context, with
synchronization if invocation can race.

**Deep explanation**

The callback contract needs explicit retention and teardown semantics. Merely
setting a local session pointer to null does not change the context stored by
the registration system.

**C/C++ code/API anchor**

Possible lifecycle:

```text
create context
register(callback, context)
run
unregister and wait for in-flight callbacks
destroy context
```

In C++, ownership might be coordinated with RAII, but shared ownership should
not be introduced without considering cycles and shutdown behavior.

**Production/debug angle**

Use ASan for lifetime failure and ThreadSanitizer for teardown races. Log
registration IDs, context addresses, invocation starts/ends, and destruction.

**Common traps**

- Destroying context before unregistering.
- Assuming unregister means no callback is currently in flight.
- Solving every callback lifetime with `shared_ptr`.
- Ignoring reentrancy during unregister.

**Follow-ups**

- How would you wait for in-flight callbacks?
- What if unregister is called from inside the callback?

### Scenario 6. Release Build Misbehaves After Type Punning

```c
float value = 1.0F;
uint32_t *bits = (uint32_t *)&value;

if (*bits == 0x3f800000U) {
    /* ... */
}
```

**Short answer**

The access uses an incompatible pointer type and can violate strict aliasing.
Optimization may expose the undefined behavior. Use `memcpy` for representation
transfer and do not assume a universal floating-point representation.

**Deep explanation**

The numeric address can be identical while the typed access is still invalid.
The optimizer may assume `float *` and `uint32_t *` do not alias in this way.
The expected bit pattern also assumes a representation and byte order.

**C/C++ code/API anchor**

```c
uint32_t bits;
_Static_assert(sizeof bits == sizeof value,
               "example requires equal-sized types");
memcpy(&bits, &value, sizeof bits);
```

**Production/debug angle**

Compare `-O0` and `-O2`, enable useful strict-aliasing warnings and UBSan, and
search for cast-based type punning in serialization, hashing, and parsers.

**Common traps**

- Blaming the optimizer.
- Disabling optimization as the final fix.
- Assuming a union or pointer cast has identical rules across C and C++.
- Treating copied representation as a portable network format.

**Follow-ups**

- What can character pointers inspect?
- How does `std::bit_cast` relate in modern C++?

## Rapid-Fire Checks

1. **Can a pointer be non-null and dangling?** Yes.
2. **Can a reference dangle?** Yes.
3. **Is an array a pointer?** No; it often converts to one.
4. **Can one-past be dereferenced?** No.
5. **What does `int (*p)[4]` mean?** Pointer to array of four `int`.
6. **What does `int *p[4]` mean?** Array of four pointers to `int`.
7. **Does `T **` mean a contiguous 2D array?** No.
8. **Can `void *` be dereferenced directly?** No.
9. **Does a cast fix alignment?** No.
10. **Does `ptr = NULL` repair aliases?** No.
11. **Can raw pointers express ownership by syntax alone?** No.
12. **What pairs with `malloc`?** `free`.
13. **What pairs with `new[]`?** `delete[]`.
14. **Can unrelated pointers be portably ordered with `<`?** Do not rely on it
    as a generic address-ordering mechanism.
15. **What does `const int *p` protect?** Modification through `p`.
16. **What does `int * const p` protect?** Reseating `p`.
17. **Can a capturing lambda become a plain function pointer?** Not generally.
18. **Is `std::span` owning?** No.
19. **Does `shared_ptr` make the managed object thread-safe?** No.
20. **Is `restrict` a runtime check?** No.

## Final Review Checklist

A strong Pointer Mastery candidate can:

- separate pointer object, pointer value, and pointee;
- explain why non-null is not sufficient;
- distinguish null, uninitialized, dangling, invalid, and one-past pointers;
- explain array decay without saying an array is a pointer;
- state the same-array limits of arithmetic, subtraction, and ordering;
- parse pointer-to-array, array-of-pointers, double-pointer, and function-pointer
  declarations;
- explain all three const-pointer forms;
- design pointer/count and output-parameter contracts;
- explain ownership, borrowing, invalidation, and allocation-family pairing;
- design callback-plus-context lifetime and teardown rules;
- compare raw pointer, reference, `std::span`, and smart-pointer roles;
- recognize alignment, typed-access, strict-aliasing, and `restrict` risks;
- discuss provenance conservatively;
- debug with warnings, ASan, UBSan, Valgrind, GDB, and optimized builds;
- avoid presenting undefined behavior as an inevitable crash;
- avoid unrelated kernel-driver or platform-specific material.
