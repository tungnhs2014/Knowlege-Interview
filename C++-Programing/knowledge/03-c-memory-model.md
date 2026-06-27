# 03 - C Memory Model

## 1. Goal

By the end of this chapter, you should be able to:

- describe a C object in terms of type, size, alignment, storage duration,
  lifetime, value, and object representation;
- separate the C abstract machine from a typical process memory layout;
- explain text, read-only data, initialized data, BSS, stack frames, and
  allocator-managed storage;
- distinguish automatic, static, thread, and allocated storage duration;
- use `malloc`, `calloc`, `realloc`, and `free` correctly;
- define ownership and cleanup responsibilities;
- explain alignment, structure padding, and endianness;
- distinguish undefined, unspecified, and implementation-defined behavior;
- recognize memory leaks, dangling and wild pointers, double free,
  use-after-free, buffer overflow, and stack overflow;
- debug memory failures with warnings, sanitizers, Valgrind, GDB, `size`, `nm`,
  `readelf`, and `objdump`.

This chapter uses C17 as its practical baseline. Version-sensitive behavior,
especially zero-size allocation, is called out where relevant.

This is a `MUST` topic at `Deep` depth. Chapter 02, C Fundamentals, is the
prerequisite.

## 2. Why This Matters

C gives programs direct control over data representation and lifetime. That
control is useful, but the compiler cannot protect the program from every
memory error:

- reading an object before initialization;
- writing beyond an array;
- using a pointer after its target's lifetime has ended;
- losing the only pointer that owns an allocation;
- freeing the same allocation twice;
- calculating the wrong allocation size;
- accessing data through an incorrectly aligned or incompatible pointer;
- assuming native byte order matches a file or network protocol.

Memory failures are difficult because the symptom may appear far from the
cause. A parser can overwrite allocator metadata, while the process crashes
only during a later `free`. A use-after-free may pass thousands of tests until
the allocator reuses the released block.

For embedded software, the memory model affects RAM and ROM budgets, per-task
stack budgets, deterministic startup, allocation policy, protocol handling,
and long-running reliability.

For enterprise software, it affects API ownership contracts, security review,
portability, sanitizer policy, and production crash diagnosis.

The central rule is:

> Start with type, storage duration, lifetime, alignment, bounds, and ownership.
> Physical placement on a stack or heap is an implementation-level question.

## 3. Mental Model: Objects Before Segments

### 3.1 What is a C object?

A C `object` is a region of data storage whose contents can represent values.

```c
int temperature = 25;
```

Reason about `temperature` using these questions:

| Question | Answer |
| --- | --- |
| What is its type? | `int` |
| What is its size? | `sizeof temperature` bytes |
| What alignment does it require? | The implementation's alignment for `int` |
| What is its storage duration? | Determined by where it is declared |
| When does its lifetime begin and end? | Determined by its storage mechanism |
| What is its value? | `25` |
| What is its object representation? | The bytes representing `25` |

A value is an abstract meaning. An object representation is the sequence of
bytes used to represent that value. Two implementations can represent the same
value with different byte order or other representation details.

### 3.2 Four dimensions that must remain separate

| Concept | Question answered |
| --- | --- |
| `scope` | Where is the name visible in source code? |
| `linkage` | Do declarations in different scopes or translation units denote the same entity? |
| `storage duration` | How long does the object's storage exist? |
| `lifetime` | When does the object exist and permit valid access? |

```c
static int global_count;

void sample(void)
{
    static int call_count;
    int value = 42;

    ++global_count;
    ++call_count;
    (void)value;
}
```

- `global_count` has file scope, internal linkage, and static storage duration.
- `call_count` has block scope, no linkage, and static storage duration.
- `value` has block scope, no linkage, and automatic storage duration.

`call_count` has a locally visible name but persists for the entire program.
Short scope does not imply short lifetime.

### 3.3 Abstract machine versus implementation

The C abstract machine defines objects, types, values, representations,
alignment, storage duration, lifetime, and the behavior of expressions.

A compiler, ABI, linker, executable format, and operating environment commonly
implement concepts such as:

- `.text`;
- read-only data;
- `.data`;
- `.bss`;
- a call stack and stack frames;
- allocator-managed memory mappings.

C does not require those exact segments. It also does not require the stack to
grow downward or dynamic storage to grow upward.

## 4. Storage Duration And Lifetime

### 4.1 Automatic storage duration

Objects declared inside a block commonly have automatic storage duration:

```c
void process(void)
{
    int sample = 0;
    unsigned char buffer[64] = {0};

    (void)sample;
    (void)buffer;
}
```

Their lifetime normally follows block execution. The compiler may place an
object in a stack slot, keep it in a register, or remove it if optimization
proves that no physical storage is needed.

Do not claim that every local variable lives on a physical stack.

When an automatic object's lifetime ends, a pointer to that object can no
longer be used to access it:

```c
int *bad_pointer(void)
{
    int local = 42;
    return &local; /* wrong: local's lifetime ends on return */
}
```

### 4.2 Static storage duration

File-scope objects and block-scope `static` objects have static storage
duration:

```c
int global_uninitialized;
int global_initialized = 10;

void count_calls(void)
{
    static unsigned long calls;
    ++calls;
}
```

These objects exist for the entire program execution. Without an explicit
initializer, they receive language-defined zero initialization:

```c
static int count;       /* initial value: 0 */
static int *owner;      /* initial value: null pointer */
static double ratio;    /* initial value: 0.0 */
```

Zero initialization is a language guarantee. Placement in a section named
`.bss` is a common implementation technique.

### 4.3 Thread storage duration

C11 provides thread storage duration through `_Thread_local`:

```c
_Thread_local unsigned int error_count;
```

Each thread has a separate instance. Thread support and TLS layout depend on
the implementation. Synchronization, atomics, and concurrent memory ordering
belong to Chapter 14.

### 4.4 Allocated storage duration

Memory returned by an allocation function has allocated storage duration:

```c
int *values = malloc(10U * sizeof *values);
```

If allocation succeeds:

- `values` is a pointer object, commonly with automatic storage duration;
- its target is storage for ten `int` objects;
- the allocation's lifetime ends when `free` releases it or a successful
  `realloc` replaces it.

The pointer and its target are different objects with potentially different
storage durations:

```text
values (pointer object)
    |
    +----> allocated storage for ten int values
```

When `values` leaves scope, its target is not automatically released. If no
other pointer retains the allocation address, the program has a memory leak.

## 5. Typical Process Memory Layout

A hosted ELF executable commonly uses this conceptual layout:

```text
machine instructions                  -> text
string literals and constant tables   -> read-only data
initialized static-storage objects    -> data
zero-initialized static objects       -> BSS-like region
dynamic allocations                   -> allocator-managed mappings
function-call state                   -> call stack
```

This is a model for investigating one implementation, not a requirement of C.

### 5.1 Text and read-only data

Machine instructions commonly reside in executable, read-only text mappings.
String literals and constant tables commonly reside in read-only data.

```c
static const char message[] = "sensor ready";
```

The `const` qualifier does not by itself guarantee placement in physically
read-only memory. Placement and page protection are implementation decisions.

String literals must not be modified:

```c
char *text = "ready";
text[0] = 'R'; /* undefined behavior */
```

Use a pointer-to-const when mutation is not intended:

```c
const char *text = "ready";
```

### 5.2 Initialized data and BSS

Static-storage objects with nonzero initializers commonly occupy initialized
data:

```c
int retry_limit = 3;
static unsigned int mode = 1U;
```

Zero-initialized static-storage objects commonly occupy a BSS-like region:

```c
int error_count;
static unsigned char cache[4096];
```

BSS allows an executable to describe a large zeroed region without storing all
zero bytes in the file. A loader or startup routine prepares that storage.

The language guarantees the initial value, not a section named `.bss`.

### 5.3 Stack and stack frames

Many ABIs use a call stack for:

- return state;
- saved registers;
- spilled values;
- some arguments;
- some automatic objects;
- alignment padding.

```text
caller frame
----------------
return state
saved registers
arguments/spills
local storage
----------------
callee frame
```

Optimization can inline functions, keep values in registers, reuse stack
slots, remove objects, and make debugger variables unavailable. A stack frame
is therefore an ABI/compiler model, not a semantic rule of C.

### 5.4 Allocator-managed storage and the "heap"

The `malloc` family obtains storage from an allocator. The allocator can use
multiple operating-system mechanisms and mappings, not one mandatory,
contiguous heap segment.

The word "heap" is convenient, but the precise C phrase is:

> storage returned by a memory allocation function

### 5.5 Inspecting a concrete executable

```c
#include <stdio.h>
#include <stdlib.h>

int global_zero;
int global_value = 7;
static const char label[] = "memory";

int main(void)
{
    int automatic = 11;
    int *allocated = malloc(sizeof *allocated);

    if (allocated == NULL) {
        return EXIT_FAILURE;
    }

    *allocated = 13;

    printf("&global_zero  = %p\n", (void *)&global_zero);
    printf("&global_value = %p\n", (void *)&global_value);
    printf("label         = %p\n", (void *)label);
    printf("&automatic    = %p\n", (void *)&automatic);
    printf("allocated     = %p\n", (void *)allocated);

    free(allocated);
    return EXIT_SUCCESS;
}
```

Build and inspect:

```bash
cc -std=c17 -Wall -Wextra -Wpedantic -g memory_layout.c -o memory_layout
size memory_layout
nm -S memory_layout
readelf -S -l -s memory_layout
objdump -h -t memory_layout
```

The observed addresses and sections describe only that executable and
platform.

## 6. Dynamic Memory APIs In C

The standard allocation functions are declared in `<stdlib.h>`:

```c
void *malloc(size_t size);
void *calloc(size_t count, size_t size);
void *realloc(void *ptr, size_t new_size);
void free(void *ptr);
```

In C, `void *` converts to an object-pointer type without a cast:

```c
int *values = malloc(10U * sizeof *values);
```

Avoid casting `malloc` results in C. A cast repeats the type and, in old code,
can hide a missing function declaration.

### 6.1 `malloc`

`malloc(size)` requests a block of at least `size` bytes:

```c
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    const size_t count = 5U;
    int *values = malloc(count * sizeof *values);

    if (values == NULL) {
        fputs("allocation failed\n", stderr);
        return EXIT_FAILURE;
    }

    for (size_t i = 0U; i < count; ++i) {
        values[i] = (int)(i * 10U);
    }

    for (size_t i = 0U; i < count; ++i) {
        printf("%d%c", values[i], i + 1U == count ? '\n' : ' ');
    }

    free(values);
    values = NULL;
    return EXIT_SUCCESS;
}
```

The bytes returned by `malloc` are uninitialized. Do not read an element before
storing a valid value in it.

### 6.2 `calloc`

`calloc(count, element_size)` allocates storage and sets every byte to zero:

```c
unsigned int *counters = calloc(channel_count, sizeof *counters);

if (counters == NULL) {
    /* Handle allocation failure. */
}
```

| API | Initial contents |
| --- | --- |
| `malloc` | Uninitialized or indeterminate bytes |
| `calloc` | All bytes set to zero |

Do not generalize byte-zeroing into a claim that all-bits-zero is the semantic
zero or null representation for every possible C type. Initialize according to
the type's contract when that distinction matters.

### 6.3 Safe allocation-size calculation

This multiplication can overflow before `malloc` is called:

```c
int *values = malloc(count * sizeof *values);
```

Check the operation first:

```c
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int allocate_ints(size_t count, int **out)
{
    if (out == NULL) {
        return 0;
    }

    *out = NULL;

    if (count > SIZE_MAX / sizeof **out) {
        return 0;
    }

    *out = malloc(count * sizeof **out);
    return *out != NULL;
}

int main(void)
{
    int *values = NULL;

    if (!allocate_ints(100U, &values)) {
        fputs("cannot allocate values\n", stderr);
        return EXIT_FAILURE;
    }

    values[0] = 42;
    printf("%d\n", values[0]);

    free(values);
    return EXIT_SUCCESS;
}
```

Before computing `count * element_size`, require:

```text
count <= SIZE_MAX / element_size
```

If a layout combines a header and payload, check both multiplication and
addition.

### 6.4 `realloc`

`realloc` can resize a block in place, move it and preserve a prefix, or fail.
On failure, the original allocation remains valid.

Direct assignment is wrong:

```c
values = realloc(values, new_count * sizeof *values);

if (values == NULL) {
    /* The original allocation address has been lost. */
}
```

Use a temporary pointer:

```c
int *new_values = realloc(values, new_count * sizeof *values);

if (new_values == NULL) {
    /* values still owns the original allocation */
    free(values);
    return EXIT_FAILURE;
}

values = new_values;
```

A checked growth helper:

```c
#include <stdint.h>
#include <stdlib.h>

static int grow_int_array(int **data, size_t *capacity)
{
    size_t new_capacity;
    int *new_data;

    if (data == NULL || capacity == NULL) {
        return 0;
    }

    if (*capacity == 0U) {
        new_capacity = 8U;
    } else {
        if (*capacity > SIZE_MAX / 2U) {
            return 0;
        }
        new_capacity = *capacity * 2U;
    }

    if (new_capacity > SIZE_MAX / sizeof **data) {
        return 0;
    }

    new_data = realloc(*data, new_capacity * sizeof **data);
    if (new_data == NULL) {
        return 0;
    }

    *data = new_data;
    *capacity = new_capacity;
    return 1;
}
```

After successful `realloc`:

- use the returned pointer;
- do not use the old pointer value or aliases into the old allocation;
- initialize newly added elements before reading them.

This rule applies even if the numeric address appears unchanged.

### 6.5 `free`

These calls are valid:

```c
free(pointer_returned_by_allocator);
free(NULL);
```

These calls are invalid:

```c
int local = 0;
int *values = malloc(4U * sizeof *values);

free(&local);       /* invalid free */
free(values + 1);   /* invalid free: interior pointer */
free(values);
free(values);       /* double free */
```

After `free(values)`, the allocation's lifetime has ended. Every pointer into
that allocation is dangling:

```c
int *owner = malloc(sizeof *owner);
int *alias = owner;

free(owner);
owner = NULL;

/* alias is still dangling */
```

Resetting one owner variable can prevent accidental reuse of that variable. It
does not repair aliases and is not a substitute for ownership design.

### 6.6 Zero-size allocation

The contracts around `malloc(0)` and `realloc(pointer, 0)` have
version-sensitive and implementation-sensitive history.

Use an explicit production policy:

- handle `count == 0` directly;
- call `free` when the operation means release;
- represent an empty dynamic array as `data = NULL`, `size = 0`, and
  `capacity = 0`;
- do not use zero-size allocation as an ownership protocol.

## 7. Ownership And Cleanup

### 7.1 Ownership

An owner is responsible for:

- keeping the allocation reachable;
- deciding when its lifetime ends;
- calling the correct release function;
- not releasing it while borrowers still use it;
- releasing it no more than once.

An API contract should state ownership explicitly:

```c
/*
 * On success, returns an owned buffer through *out_data.
 * The caller must release it with free().
 */
int read_payload(unsigned char **out_data, size_t *out_size);
```

An API that returns a raw pointer without saying who releases it has an
incomplete contract.

### 7.2 One cleanup path

C has no automatic destructor mechanism. A disciplined cleanup path can make
all exits reviewable:

```c
#include <stdio.h>
#include <stdlib.h>

static int process_buffers(size_t count)
{
    int result = 0;
    int *input = NULL;
    int *output = NULL;

    input = malloc(count * sizeof *input);
    if (input == NULL) {
        goto cleanup;
    }

    output = calloc(count, sizeof *output);
    if (output == NULL) {
        goto cleanup;
    }

    input[0] = 21;
    output[0] = input[0] * 2;
    printf("%d\n", output[0]);
    result = 1;

cleanup:
    free(output);
    free(input);
    return result;
}

int main(void)
{
    return process_buffers(16U) ? EXIT_SUCCESS : EXIT_FAILURE;
}
```

Because `free(NULL)` is a no-op, owner variables initialized to `NULL` can be
released unconditionally. Here, `goto cleanup` has one narrow purpose:
forward-only transfer to resource cleanup.

## 8. Alignment, Padding, And Representation

### 8.1 Alignment

Every complete object type has an alignment requirement. An object's address
must satisfy that requirement before access through an lvalue of that type.

```c
#include <stdalign.h>
#include <stdio.h>

int main(void)
{
    printf("alignof(char)   = %zu\n", alignof(char));
    printf("alignof(int)    = %zu\n", alignof(int));
    printf("alignof(double) = %zu\n", alignof(double));
    return 0;
}
```

`malloc` returns storage with alignment covered by the standard allocation
guarantee. An arbitrary byte offset may destroy alignment:

```c
unsigned char buffer[16];
int *value = (int *)(buffer + 1);
*value = 42; /* possible misalignment and other violations */
```

A cast does not create alignment.

### 8.2 Structure padding

A compiler may insert padding between members and after the final member:

```c
#include <stddef.h>
#include <stdio.h>

struct Sample {
    char status;
    int value;
    char quality;
};

int main(void)
{
    printf("sizeof(struct Sample) = %zu\n", sizeof(struct Sample));
    printf("status  offset = %zu\n", offsetof(struct Sample, status));
    printf("value   offset = %zu\n", offsetof(struct Sample, value));
    printf("quality offset = %zu\n", offsetof(struct Sample, quality));
    return 0;
}
```

A common layout is:

```text
status | padding | value | quality | trailing padding
```

Trailing padding helps each element of a structure array begin at a correctly
aligned address.

### 8.3 Why raw structure comparison is unsafe

This is not a general equality operation:

```c
if (memcmp(&left, &right, sizeof left) == 0) {
    /* Assume equal. */
}
```

Two structures can have equal member values but different padding bytes.
Compare fields:

```c
static int sample_equal(const struct Sample *left,
                        const struct Sample *right)
{
    return left->status == right->status
        && left->value == right->value
        && left->quality == right->quality;
}
```

For the same reason, do not use native structure bytes as a file format,
network format, stable hash, or portable serialization by default.

### 8.4 Inspecting object representation

Character types can inspect an object's representation byte by byte:

```c
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    uint32_t value = UINT32_C(0x01020304);
    const unsigned char *bytes = (const unsigned char *)&value;

    for (size_t i = 0U; i < sizeof value; ++i) {
        printf("%02X%c", bytes[i],
               i + 1U == sizeof value ? '\n' : ' ');
    }

    return 0;
}
```

This permission does not make arbitrary casts between unrelated pointer types
valid.

## 9. Endianness And External Data

For a 32-bit value written as `0x01020304`, if the lowest-addressed byte is:

- `04`, the representation is little-endian;
- `01`, the representation is big-endian.

C does not mandate either byte order.

```c
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    const uint16_t value = UINT16_C(0x0102);
    const unsigned char *bytes = (const unsigned char *)&value;

    if (bytes[0] == 0x02U) {
        puts("little-endian");
    } else if (bytes[0] == 0x01U) {
        puts("big-endian");
    } else {
        puts("another representation");
    }

    return 0;
}
```

Decode external formats explicitly. For a big-endian 16-bit field:

```c
#include <stdint.h>

static uint16_t read_u16_be(const unsigned char bytes[2])
{
    return (uint16_t)(((uint16_t)bytes[0] << 8)
                    | (uint16_t)bytes[1]);
}
```

Do not replace explicit decoding with:

```c
uint16_t value = *(const uint16_t *)packet;
```

That expression can violate alignment, bounds, effective-type, aliasing, and
host-endianness requirements.

Network byte order is conventionally big-endian. Familiar conversion
functions are platform APIs, not ISO C APIs.

## 10. Behavior Categories

### 10.1 Undefined behavior

For undefined behavior, the C standard imposes no requirements:

```c
int values[4] = {0};
values[4] = 10; /* out of bounds: undefined behavior */
```

UB does not mean "the program always crashes." It may appear to work, corrupt
data, change under optimization, or allow the compiler to remove and transform
surrounding code.

Common memory-related UB includes:

- out-of-bounds access;
- null dereference;
- use-after-free;
- double or invalid free;
- misaligned typed access;
- forbidden reads of indeterminate representations;
- incompatible-type access that violates aliasing rules.

### 10.2 Unspecified behavior

Unspecified behavior allows one of multiple valid outcomes without requiring
the implementation to document the choice for each occurrence. Correct code
must not depend on one particular choice.

### 10.3 Implementation-defined behavior

Implementation-defined behavior requires the implementation to choose and
document an outcome. Examples include whether plain `char` is signed and the
concrete widths of several integer types.

Depending on implementation-defined behavior can be valid when the assumption
is documented and verified.

### 10.4 Constraint violations

A constraint violation normally requires a diagnostic. It is not simply
another name for undefined behavior.

| Category | Implementation obligation |
| --- | --- |
| Undefined | No runtime-behavior requirement |
| Unspecified | Choose from permitted outcomes; no per-instance documentation required |
| Implementation-defined | Choose and document an outcome |
| Constraint violation | Produce a diagnostic as required by the language rules |

## 11. Common Memory Bugs

### 11.1 Uninitialized and wild pointers

```c
int result;
printf("%d\n", result); /* unsafe read */

int *pointer;
*pointer = 42;          /* undefined behavior */
```

Initialize objects before reading them:

```c
int result = 0;
int *pointer = NULL;
```

A non-null pointer is not automatically live, in bounds, aligned, or correctly
typed.

### 11.2 Dangling pointers and use-after-free

```c
int *value = malloc(sizeof *value);
if (value != NULL) {
    *value = 42;
    free(value);
    printf("%d\n", *value); /* use-after-free */
}
```

A pointer also dangles when an automatic target leaves scope.

### 11.3 Memory leaks

```c
void leak(void)
{
    int *value = malloc(sizeof *value);
    if (value == NULL) {
        return;
    }

    *value = 42;
    /* Missing free. */
}
```

A leak is allocated storage that is no longer released when it is no longer
needed. Long-running leaks can exhaust resources.

### 11.4 Lost allocation through `realloc`

```c
data = realloc(data, new_size);
if (data == NULL) {
    /* No pointer remains for releasing the old allocation. */
}
```

Keep the original owner until `realloc` succeeds.

### 11.5 Double and invalid free

```c
free(data);
free(data); /* undefined behavior */

int local;
free(&local); /* undefined behavior */
```

Release only a live allocation through its matching allocation family.

### 11.6 Buffer overflow

```c
int values[4] = {0};

for (size_t i = 0U; i <= 4U; ++i) {
    values[i] = (int)i; /* i == 4 is out of bounds */
}
```

The valid loop condition is `i < 4U`. A one-past pointer can serve as a
sentinel in the correct array context, but it cannot be dereferenced.

### 11.7 Allocation-size overflow

```c
struct Item *items = malloc(count * sizeof *items);
```

If the `size_t` multiplication wraps, the allocator receives a smaller request
than intended. Later writes based on `count` overflow the allocation.

### 11.8 Stack overflow

Common causes include:

- unbounded or excessively deep recursion;
- very large automatic arrays;
- input-sized VLAs without a bound;
- a call chain that exceeds its stack budget.

Stack overflow is not universally explained by a collision between one stack
and one heap. Address-space layout and guard mechanisms are platform-specific.

## 12. Key Comparisons

### 12.1 Stack and allocated storage

| Aspect | Typical call stack | Allocated storage |
| --- | --- | --- |
| C guarantee | No required physical stack | API and lifetime defined by the C library |
| Typical lifetime | Follows block and call execution | Allocation success through release |
| Cleanup | Follows control flow | Explicit `free` |
| Failure | Stack exhaustion | Allocation returns `NULL` |
| Main risks | Deep calls and large locals | Leaks, UAF, invalid free, fragmentation |

A local and dynamically allocated object can have the same C type. Storage
duration and ownership differ; the value is not a different kind of type.

### 12.2 Data and BSS

| Aspect | Data | BSS-like region |
| --- | --- | --- |
| Typical object | Static-storage, nonzero initialized | Static-storage, zero-initialized |
| File contents | Initial bytes commonly stored | Size/metadata commonly sufficient |
| Language guarantee | Initial value | Initial value |
| Section name/layout | Implementation detail | Implementation detail |

### 12.3 Allocation functions

| API | Purpose | Key rule |
| --- | --- | --- |
| `malloc` | Create a byte block | Contents are uninitialized |
| `calloc` | Create a zeroed byte block | Every byte is zero |
| `realloc` | Resize or replace an allocation | May move; use a temporary pointer |
| `free` | End an allocation's lifetime | `free(NULL)` is a no-op |

### 12.4 Pointer states

| State | Meaning |
| --- | --- |
| Null pointer | Deliberately points to no object or function |
| Wild pointer | Pointer object has not been initialized |
| Dangling pointer | Target lifetime has ended |

### 12.5 C allocation and C++ resource management

| Topic | C | C++ | Enterprise / Embedded Usage |
| --- | --- | --- | --- |
| Allocation | `malloc`, `calloc`, and `realloc` obtain raw storage | Containers, smart pointers, and lower-level `new` manage object storage | Select one ownership model per API boundary and document allocation failure |
| Release | `free` ends an allocation obtained from the C allocation family | `delete` matches `new`; `delete[]` matches `new[]` | Never cross allocation families, especially across library or module boundaries |
| Construction and destruction | Allocation does not call constructors or destructors | `new` constructs objects and `delete` destroys them | Prefer APIs that keep construction, ownership, and release in the same abstraction |
| Failure | Allocation functions report failure with a null pointer | Throwing `new` reports failure with `std::bad_alloc`; other APIs have their own contracts | Match the error policy to the project, including exception-disabled embedded builds |
| Dynamic array | Explicit owner structure with data, size, capacity, and cleanup | Prefer `std::vector` to raw `new[]`/`delete[]` | Apply capacity limits and deterministic growth policies where memory is constrained |
| Owning pointer | A raw pointer plus a documented release contract commonly represents ownership | Prefer an RAII owner such as `std::unique_ptr`; raw pointers normally express non-ownership | Make ownership visible in interfaces and prohibit ambiguous transfer |
| Copying an owner | Copying a raw owning pointer is a shallow copy and can cause double free or shared mutation | RAII types define, delete, or control copy; a deep copy duplicates the owned resource | Specify whether resources are non-copyable, uniquely owned, shared, or deeply copied |
| Automatic versus allocated object | Storage duration and ownership differ even when the C type is identical | Prefer automatic RAII objects unless dynamic lifetime or polymorphism is required | Choose based on lifetime, maximum size, stack budget, and failure behavior |

Never mix allocation families:

```text
malloc/calloc/realloc -> free
new                   -> delete
new[]                 -> delete[]
```

Modern C++ normally prefers RAII and standard containers over manual
`new`/`delete`. Chapter 10 covers that model in depth.

A shallow copy duplicates a pointer value but not the pointed-to allocation. If
both copies behave as owners, cleanup can double-free the allocation. A deep
copy allocates a distinct resource and copies its contents. C APIs must define
this behavior explicitly; C++ resource-owning types encode it through their
copy and move operations.

## 13. Embedded And Enterprise Usage

### 13.1 Embedded memory policy

A useful memory policy answers:

- Is dynamic allocation permitted?
- Is it limited to initialization, or allowed during runtime?
- What is the maximum allocation size?
- How is allocation failure handled?
- Is fragmentation measured or bounded?
- What is each task's stack budget?
- What is the worst-case call depth?
- Are linker maps and stack-usage reports checked in CI?

Static allocation is valuable when bounded memory, deterministic startup, and
analyzable failure are required. Dynamic allocation can still be appropriate
when the allocator, product lifetime, fragmentation, ownership, and recovery
strategy are controlled.

### 13.2 Protocol parsers

A safe parser:

- validates remaining input before every read;
- decodes explicit widths and byte order;
- does not cast a packet buffer to a structure pointer;
- does not assume native alignment;
- checks offset and length arithmetic;
- does not retain pointers after the buffer owner releases or reallocates it.

### 13.3 Ownership review

For every owning pointer, ask:

1. Where is the allocation created?
2. Who owns it?
3. Which borrowers retain aliases?
4. Which function releases it?
5. Does cleanup run on every path?
6. Can size arithmetic overflow?
7. Does the lifetime exceed every access?
8. Can `realloc` invalidate aliases?
9. Are boundaries tested with dynamic analysis?

## 14. Debugging Workflow

### 14.1 Start with warnings

```bash
cc -std=c17 \
   -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wformat=2 \
   -g3 program.c -o program
```

Warnings can expose uninitialized use, suspicious conversions, invalid format
strings, returned local addresses, and some bounds defects. Do not add a cast
merely to silence a warning.

### 14.2 AddressSanitizer and UBSan

```bash
cc -std=c17 -g3 -O1 \
   -fsanitize=address,undefined \
   -fno-omit-frame-pointer \
   program.c -o program_asan

./program_asan
```

AddressSanitizer commonly detects out-of-bounds access, use-after-free, double
free, invalid free, and some stack-lifetime bugs. UndefinedBehaviorSanitizer
adds checks for supported cases such as misalignment, invalid shifts, signed
overflow, and object-size violations.

Sanitizer success is not proof of memory safety.

### 14.3 Valgrind

```bash
cc -std=c17 -g3 -O0 program.c -o program

valgrind \
  --leak-check=full \
  --show-leak-kinds=all \
  --track-origins=yes \
  ./program
```

Valgrind is useful on supported hosted platforms, but it is slower than native
execution and does not replace target testing.

### 14.4 GDB

```bash
gdb ./program
```

Useful initial commands:

```gdb
run
bt
frame 0
info args
info locals
p variable
p/x pointer
x/16bx pointer
```

| Command | Purpose |
| --- | --- |
| `bt` | Show the backtrace |
| `frame N` | Select a stack frame |
| `info args` | Show arguments |
| `info locals` | Show available local variables |
| `p/x` | Print in hexadecimal |
| `x/16bx` | Examine sixteen bytes |

A corrupted stack can make the backtrace itself unreliable.

### 14.5 Compare optimization levels

```bash
cc -std=c17 -g3 -O0 program.c -o program_O0
cc -std=c17 -g3 -O2 program.c -o program_O2
```

`-O0` offers easier source-level debugging. `-O2` is closer to production
transformations. A program containing UB can behave differently because the
optimizer assumes that UB does not occur.

### 14.6 Inspect binary layout

```bash
size program
nm -S program
readelf -S -l -s program
objdump -h -t -d program
```

- `size` summarizes text, data, and BSS.
- `nm` displays symbols and sizes.
- `readelf` displays ELF sections, segments, and symbols.
- `objdump` displays headers, symbols, and disassembly.

For embedded builds, linker maps and compiler stack-usage reports are usually
more useful than desktop virtual addresses.

### 14.7 Reduce and test boundaries

Create a minimal reproducer and test:

- zero and one elements;
- the maximum valid count;
- arithmetic near `SIZE_MAX`;
- allocation failure;
- grow and shrink paths;
- every early return;
- parser input one byte too short;
- repeated allocate/free cycles.

## 15. Controlled Advanced Topics

### 15.1 Effective type and strict aliasing

Allocated storage initially has no declared type. Typed stores and copies from
existing objects influence how it may later be accessed.

This is not a general-purpose type conversion:

```c
float value = 1.0F;
uint32_t bits = *(uint32_t *)&value; /* aliasing and alignment risk */
```

Transfer the representation with `memcpy`:

```c
#include <stdint.h>
#include <string.h>

static uint32_t float_bits(float value)
{
    uint32_t bits;
    _Static_assert(sizeof bits == sizeof value,
                   "float and uint32_t sizes must match");
    memcpy(&bits, &value, sizeof bits);
    return bits;
}
```

Practical rules:

- use normally typed objects;
- use character types to inspect bytes;
- use `memcpy` for representation transfer when the size contract permits it;
- do not type-pun through unrelated pointer types.

### 15.2 Cache locality

Contiguous data commonly has better cache locality than scattered allocations.
For example, one `rows * columns` allocation is often easier for hardware to
traverse than separately allocated rows.

Performance must still be measured on the real target and workload. Do not
change ownership and readability based only on slogans such as "stack is
faster than heap."

### 15.3 Memory barriers and false sharing

Memory barriers, false sharing, and atomic ordering belong to concurrency and
the hardware memory system. They are not synonyms for object lifetime or
text/data/BSS/stack/heap layout.

These topics belong primarily to Chapter 14. `volatile` is not a replacement
for synchronization or memory ordering.

## 16. Best Practices

- Begin with type, storage duration, lifetime, alignment, bounds, and ownership.
- Initialize automatic objects before their first read.
- Check every allocation result.
- Check multiplication and addition before calculating allocation size.
- Prefer `sizeof *pointer` over repeating the pointed-to type.
- Do not cast `malloc` results in C.
- Initialize owner pointers to `NULL`.
- Use a temporary pointer for `realloc`.
- Do not use zero-size allocation as an ownership convention.
- Release only live allocations through the matching allocation family.
- Make cleanup correct on every success and error path.
- Do not treat non-null as proof that a pointer is valid.
- Do not retain aliases across `free` or successful `realloc`.
- Keep length and capacity invariants explicit.
- Never dereference a one-past pointer.
- Do not compare, hash, or serialize structures as raw bytes by default.
- Decode external data using explicit widths and byte order.
- Use `memcpy` instead of incompatible typed-pointer casts.
- Bound recursion, VLAs, and large automatic arrays.
- Document and verify implementation-defined assumptions.
- Run warnings, sanitizers, leak checks, and static analysis early.
- Test with the production optimization level.
- For embedded software, inspect linker maps and enforce stack budgets.

## 17. Interview Readiness

### 17.1 Stack versus heap

A precise answer:

> C defines storage duration and allocation APIs, not a mandatory physical
> stack/heap layout. In common implementations, a call stack stores call state
> and many automatic objects, while allocator-managed storage lives from
> successful allocation until release. Stack-related risks include deep calls
> and large locals; dynamic-storage risks include leaks, use-after-free,
> invalid free, and fragmentation.

### 17.2 Data versus BSS

> Both commonly contain objects with static storage duration. Data commonly
> stores initial bytes for nonzero-initialized objects. BSS commonly describes
> zero-initialized storage without storing every zero byte in the executable.
> C guarantees the initial value; section names and layout are implementation
> details.

### 17.3 Why is BSS zero-initialized?

The language requires static-storage objects without explicit initializers to
receive zero initialization. A loader or startup routine commonly implements
this by zeroing a BSS-like region.

### 17.4 `malloc` versus `calloc`

> `malloc(size)` returns a block with uninitialized bytes. `calloc(count, size)`
> returns a block whose bytes are all zero. Both require failure handling,
> overflow-aware sizing, and release through `free`.

### 17.5 Why use a temporary pointer with `realloc`?

> On failure, `realloc` returns `NULL` while the original allocation remains
> valid. Direct assignment loses the owner and leaks that allocation. On
> success, ownership moves to the returned pointer and old aliases must not be
> used.

### 17.6 What is a memory leak?

> A leak is allocated storage that is not released when no longer needed,
> commonly because ownership was lost or an error path skipped cleanup.

### 17.7 What is undefined behavior?

> It is behavior for which the standard imposes no requirements. UB is not
> limited to crashes; it may appear to work, corrupt data, or change under
> optimization.

### 17.8 Alignment versus padding

> Alignment constrains an object's address. Padding consists of bytes inserted
> within or after an aggregate to satisfy layout and alignment requirements.
> Padding makes raw structure comparison and serialization non-portable.

### 17.9 How do you debug a segmentation fault?

1. Build with warnings and debug information.
2. Reduce the failing input.
3. Run AddressSanitizer and UBSan.
4. Use GDB to inspect the backtrace, frame, arguments, locals, pointer, and
   bounds.
5. Verify lifetime, ownership, allocation size, indexes, and cleanup.
6. Reproduce at the production optimization level.
7. Add Valgrind or static analysis where appropriate.

### 17.10 Senior-level discussion

Be ready to explain:

- the C object model versus an ELF process layout;
- what an ownership contract must specify;
- how successful `realloc` affects aliases;
- how effective type and strict aliasing affect optimization;
- how a protocol parser avoids alignment and endianness defects;
- how an embedded memory policy balances determinism and dynamic allocation;
- why passing sanitizers does not prove memory safety.

## 18. Practice

### Basic

1. Create initialized and zero-initialized file-scope objects, a static local,
   and an automatic local. Inspect the executable with `size` and `nm -S`.
2. Allocate an array with `malloc`, initialize every element, print it, and
   release it.
3. Replace `malloc` with `calloc` and explain exactly what was zeroed.
4. Demonstrate that `free(NULL)` is safe without performing a double free.
5. Inspect the bytes of a `uint32_t` and report native endianness.

### Intermediate

1. Write an array-allocation helper with multiplication-overflow checking.
2. Implement dynamic-array growth using a temporary pointer with `realloc`.
3. Use `sizeof`, `alignof`, and `offsetof` to explain three structures with
   different member order.
4. Isolate a leak, use-after-free, double free, and buffer overflow. Run ASan
   and Valgrind and record which defects each tool reports.
5. Parse big-endian 16-bit and 32-bit fields with input-length checks.
6. Refactor a function with multiple early returns into one clear cleanup path.

### Advanced

1. Compare `size`, `readelf -S -l -s`, and `objdump -h -t` output for
   initialized and zero-initialized global arrays.
2. Define an ownership contract for a buffer API, including success, failure,
   borrowing, release, and `realloc` behavior.
3. Create a strict-aliasing violation, compare `-O0` and `-O2`, then repair it
   with `memcpy`.
4. Design a binary protocol parser without raw structure overlays.
5. Create a stack budget for a bounded call tree or large local buffers.
6. Write a memory policy for a long-running embedded product covering static
   allocation, runtime allocation, recovery, stack measurement, and CI checks.

## 19. Summary

- C memory reasoning begins with objects, types, alignment, storage duration,
  lifetime, bounds, and ownership.
- Text, data, BSS, stack, and heap are common implementation concepts, not a
  mandatory layout imposed by C.
- An automatic object is not guaranteed to occupy a physical stack slot.
- Static-storage objects exist for the program and receive zero initialization
  when no explicit initializer is present.
- Allocated storage exists from successful allocation until `free` or a
  successful replacing `realloc`.
- `malloc` leaves bytes uninitialized, `calloc` zeroes bytes, `realloc` can
  move, and `free(NULL)` is safe.
- Allocation-size arithmetic must be checked before calling the allocator.
- `realloc` requires a temporary owner, and successful replacement invalidates
  old pointers and aliases.
- Alignment constrains addresses; padding consists of bytes inserted into
  layout.
- Native structure layout is not a file or network format.
- External data requires explicit width, bounds, and endianness handling.
- Undefined behavior is not limited to crashes and can change under
  optimization.
- Clear ownership, complete cleanup, correct bounds, and disciplined tooling
  are the foundation of robust C memory management.

## Reference Notes

- ISO C public draft WG14 N3096, especially the sections covering objects,
  representations, alignment, storage duration, expressions, and memory
  management functions.
- cppreference C pages for objects, storage duration, `malloc`, `calloc`,
  `realloc`, and `free`.
- SEI CERT C rules EXP33-C, ARR30-C, MEM30-C, MEM31-C, MEM34-C, and MEM35-C.
- Compiler and tool commands use a GCC-compatible C17 environment. Flag and
  sanitizer support varies by compiler and target.
