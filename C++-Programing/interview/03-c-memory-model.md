# 03 - C Memory Model: Interview Pack

## How To Use This Pack

For each question:

1. Give the **Short answer** first.
2. Expand with the **Deep explanation** only as far as the interviewer needs.
3. Ground the answer with the **C/C++ code/API anchor**.
4. Add the **Production/debug angle** to demonstrate engineering judgment.
5. Avoid the listed **Common traps**.
6. Use the **Follow-up questions** to test whether the model is complete.

The primary language baseline is C17. C++ appears only where comparison helps
clarify allocation, object construction, RAII, or ownership. Physical sections,
stack frames, and ELF tools are implementation examples rather than universal C
requirements.

## Beginner Questions

### 1. What is the difference between a C object and a memory segment, and how do data and BSS differ?

**Short answer**

A C object is a region of data storage with a type, size, alignment, storage
duration, lifetime, value, and representation. A memory segment such as text,
data, BSS, stack, or heap is an implementation-level organization used by a
toolchain and runtime environment. Initialized data commonly stores initial
bytes for nonzero static-storage objects; BSS commonly describes
zero-initialized static storage without storing every zero byte in the
executable.

**Deep explanation**

The C abstract machine defines objects and the rules for valid access. It does
not require an ELF process, a physical call stack, a section named `.bss`, or a
single heap that grows in a particular direction.

A source-level object may be placed in a section, kept in a register, merged
with another constant, optimized away, or managed by an allocator. Therefore,
reason about language correctness using type, lifetime, alignment, bounds, and
ownership. Use sections and addresses when diagnosing one concrete build.

C requires static-storage objects without explicit initializers to receive zero
initialization. A loader or startup routine commonly implements that rule by
zeroing a BSS-like region. The language guarantees the initial values, not the
existence or spelling of `.data` and `.bss`.

**C/C++ code/API anchor**

```c
static int global_count;
static int initialized_count = 10;

void process(void)
{
    int local_count = 0;
    ++local_count;
}
```

`global_count` and `initialized_count` have static storage duration.
A common ELF implementation places `global_count` in BSS,
`initialized_count` in initialized data, and `local_count` in a stack slot.
None of those physical placements is the defining C rule.

**Production/debug angle**

Use `size`, `nm -S`, `readelf -S -l -s`, and `objdump -h -t` to inspect a real
binary. Never turn one observed address layout into a portability guarantee.

**Common traps**

- Saying every local variable is stored on the stack.
- Saying C requires text, data, BSS, heap, and stack segments.
- Saying BSS is zero-initialized merely because the section happens to be empty.
- Assuming stack always grows downward and heap always grows upward.
- Confusing an object's value with its byte representation.

**Follow-up questions**

- Can an automatic object be held only in a register?
- What language property explains how long an object exists?
- Why can BSS reduce executable file size?
- Which tool would you use to inspect an ELF section table?

### 2. Compare automatic, static, thread, and allocated storage duration.

**Short answer**

Automatic storage commonly follows block execution. Static storage lasts for
the program. Thread storage provides one instance per thread. Allocated storage
lasts from successful allocation until release or replacement.

**Deep explanation**

Storage duration is not the same as scope. A block-scope `static` object has a
locally visible name but persists for the entire program. Static- and
thread-storage objects receive zero initialization when no explicit initializer
provides another value.

Allocated storage is controlled through APIs such as `malloc` and `free`. The
pointer object and the allocation it points to can have different storage
durations. Letting the pointer leave scope does not release the allocation.

**C/C++ code/API anchor**

```c
_Thread_local unsigned int thread_errors;
static int file_state;

void sample(void)
{
    static unsigned int calls;
    int automatic = 0;
    int *allocated = malloc(sizeof *allocated);

    if (allocated != NULL) {
        *allocated = automatic;
        free(allocated);
    }

    ++calls;
}
```

**Production/debug angle**

Persistent state affects reentrancy and test isolation. Allocated state needs an
explicit owner and cleanup path. Thread-local state avoids sharing that object,
but it does not make other shared objects thread-safe.

**Common traps**

- Treating scope and storage duration as synonyms.
- Assuming every `static` name has internal linkage.
- Forgetting that `malloc` storage survives the pointer variable's scope.
- Treating `_Thread_local` as a general synchronization mechanism.

**Follow-up questions**

- What storage duration does a block-scope `static` object have?
- Why can a local pointer going out of scope cause a leak?
- Which storage durations receive implicit zero initialization?

### 3. What is the difference between stack and heap?

**Short answer**

In common implementations, a call stack holds call state and many automatic
objects, while allocator-managed storage holds dynamic allocations until they
are explicitly released. C defines the lifetimes and allocation APIs, not a
mandatory physical stack/heap layout.

**Deep explanation**

A stack frame can contain return state, saved registers, spills, parameters,
and local storage. Optimization can inline calls, eliminate frames, or keep
values in registers.

Dynamic allocation supports runtime-sized and explicitly controlled lifetimes,
but introduces ownership, failure, fragmentation, leak, and use-after-free
risks. "Stack is always faster" is an oversimplification; performance depends on
the allocator, target, workload, optimization, and access pattern.

**C/C++ code/API anchor**

```c
void process(size_t count)
{
    int local_value = 0; /* automatic storage duration */
    int *values = malloc(count * sizeof *values);

    if (values == NULL) {
        return;
    }

    values[0] = local_value;
    free(values);
}
```

In C++, the equivalent production design would commonly prefer a managed
container such as `std::vector<int>` over a raw `new[]` allocation.

**Production/debug angle**

For stack exhaustion, inspect call depth, recursion, large locals, VLAs, and
compiler stack-usage reports. For dynamic storage, use ASan, Valgrind,
allocation-failure tests, and ownership review.

**Common traps**

- Describing stack and heap as C types.
- Claiming stack cannot fragment in every possible implementation.
- Assuming dynamic storage is automatically shared safely between threads.
- Explaining stack overflow only as stack/heap collision.

**Follow-up questions**

- Can the same `struct` type have automatic and allocated instances?
- What causes stack exhaustion?
- When is static allocation preferable in embedded software?

### 4. Compare `malloc`, `calloc`, `realloc`, and `free`.

**Short answer**

`malloc` allocates uninitialized bytes. `calloc` allocates and zeroes bytes.
`realloc` resizes or replaces an allocation and may move it. `free` ends an
allocation's lifetime; `free(NULL)` has no effect.

**Deep explanation**

All allocation results must be checked. `calloc` zeroes the object
representation; do not generalize that into a universal semantic guarantee for
every type.

On `realloc` failure, the old allocation remains valid. On success, use the
returned pointer and stop using the old pointer value and aliases into the
previous allocation. Zero-size requests have version-sensitive history and
should not be used as an ownership protocol.

**C/C++ code/API anchor**

```c
#include <stdint.h>
#include <stdlib.h>

if (count == 0U || new_count == 0U
    || count > SIZE_MAX / sizeof(int)
    || new_count > SIZE_MAX / sizeof(int)) {
    /* This API rejects zero-sized and overflowing requests. */
    return 0;
}

int *data = malloc(count * sizeof *data);
int *zeroed = calloc(count, sizeof *zeroed);

if (data == NULL || zeroed == NULL) {
    free(data);
    free(zeroed);
    return 0;
}

int *new_data = realloc(data, new_count * sizeof *data);
if (new_data == NULL) {
    free(data);
    free(zeroed);
    return 0;
}

data = new_data;
free(data);
free(zeroed);
```

In C, do not cast the `void *` returned by these functions. In C++, prefer C++
ownership facilities rather than using C allocation for class objects.

**Production/debug angle**

Review allocation-size arithmetic before the call, failure behavior after the
call, and the matching release family. Test allocation failure and growth paths,
not only successful first allocation.

**Common traps**

- Reading `malloc` storage before initialization.
- Saying `calloc` constructs arbitrary typed objects.
- Assigning `realloc` directly to the owning pointer.
- Mixing `malloc`/`free` with `new`/`delete`.

**Follow-up questions**

- What happens to the original allocation when `realloc` fails?
- Why is `free(NULL)` useful in cleanup paths?
- Why is casting `malloc` unnecessary in C?

### 5. What are null, wild, and dangling pointers?

**Short answer**

A null pointer deliberately points to no object or function. A wild pointer is
uninitialized. A dangling pointer still contains an address associated with an
object whose lifetime has ended.

**Deep explanation**

A null check proves only that a pointer is not null. It does not prove that the
pointer is live, in bounds, aligned, correctly typed, or safe to dereference.

Wild and dangling pointers can both produce undefined behavior. Resetting one
owner to `NULL` after `free` can prevent reuse through that variable, but other
aliases still dangle.

**C/C++ code/API anchor**

```c
int *wild;                 /* uninitialized */
int *owner = malloc(sizeof *owner);
int *alias = owner;

if (owner != NULL) {
    *owner = 42;
    free(owner);
    owner = NULL;
}

/* alias is dangling even though owner is NULL */
```

C++ uses `nullptr` for a type-safe null pointer and commonly uses smart pointers
to express ownership. Non-owning raw pointers can still dangle.

**Production/debug angle**

ASan can catch many use-after-free cases. Static analysis and ownership review
are needed to identify aliases and lifetime contracts that a null check cannot
validate.

**Common traps**

- Saying every non-null pointer is valid.
- Believing nulling one alias nulls all aliases.
- Calling a freed pointer null when it still stores the old address.
- Confusing a one-past pointer with a dereferenceable pointer.

**Follow-up questions**

- Can a non-null pointer be invalid?
- How does returning a local address create a dangling pointer?
- What ownership model reduces dangling aliases?

## Mid-Level Questions

### 6. Why should `realloc` use a temporary pointer?

**Short answer**

Because failure returns `NULL` while leaving the original allocation valid.
Direct assignment can lose the only owner and leak the original block.

**Deep explanation**

`realloc` can move an allocation. On success, the returned pointer becomes the
valid owner and the old pointer value must not be used. Any aliases into the old
allocation must also be considered invalid, even if the numeric address appears
unchanged.

A robust growth operation checks capacity arithmetic and byte-size arithmetic
before calling `realloc`, commits state only after success, and initializes new
elements before reading them.

**C/C++ code/API anchor**

```c
if (new_count > SIZE_MAX / sizeof *data) {
    return 0;
}

int *new_data = realloc(data, new_count * sizeof *data);
if (new_data == NULL) {
    /* data still owns the old allocation */
    return 0;
}

data = new_data;
```

C++ containers such as `std::vector` encapsulate reallocation, but their
reallocation can similarly invalidate pointers, references, and iterators.

**Production/debug angle**

Test `realloc` failure with allocator fault injection. Review all aliases and
borrowers before growth. Use ASan to expose stale aliases after a move.

**Common traps**

- Preserving the owner on failure but continuing to use old aliases on success.
- Updating capacity before `realloc` succeeds.
- Forgetting to initialize the newly grown region.
- Using `realloc(p, 0)` as portable shorthand for release.

**Follow-up questions**

- Which state changes should be committed only after success?
- Why can a successful in-place `realloc` still invalidate the old pointer
  value according to the API contract?
- What is the C++ container analogue of pointer invalidation here?

### 7. How can allocation-size arithmetic become a security bug?

**Short answer**

If `count * element_size` or `header_size + payload_size` overflows `size_t`,
the allocator receives a smaller size than intended. Later writes based on the
original count can overflow the allocation.

**Deep explanation**

Unsigned wrap is defined, but the resulting allocation request can violate the
program's safety contract. The check must happen before the overflowing
operation.

For multiplication:

```text
count <= SIZE_MAX / element_size
```

For addition:

```text
payload_size <= SIZE_MAX - header_size
```

The API should also define whether zero elements are valid and whether success
can return a null data pointer for an empty logical container.

**C/C++ code/API anchor**

```c
static int allocate_items(size_t count, struct Item **out)
{
    if (out == NULL || count > SIZE_MAX / sizeof **out) {
        return 0;
    }

    *out = malloc(count * sizeof **out);
    return *out != NULL;
}
```

C++ code is not immune: `new T[count]`, container growth, and byte-buffer
construction still need valid size-domain reasoning.

**Production/debug angle**

Fuzz sizes near zero and `SIZE_MAX`, enable conversion diagnostics, and use
static-analysis rules for allocation sizing. Sanitizers can catch a later
overflow but may not identify the missing precondition as clearly.

**Common traps**

- Checking after multiplication.
- Assuming `calloc` removes every overflow concern.
- Storing a `size_t` result in `int`.
- Treating defined unsigned wrap as acceptable application behavior.

**Follow-up questions**

- How do you check header-plus-payload sizing?
- Why does `sizeof *ptr` reduce maintenance risk?
- Which boundary tests would you require?

### 8. Explain alignment and structure padding.

**Short answer**

Alignment constrains where an object may be stored. Padding consists of bytes
inserted between or after members so each member and each array element meets
its alignment requirements.

**Deep explanation**

Misaligned typed access can be undefined behavior even on hardware that appears
to tolerate it. Member order can change structure size, but reordering can also
change ABI and protocol contracts.

Padding bytes do not necessarily hold stable values. Consequently, raw
structure comparison, hashing, persistence, and network transmission are not
portable by default.

**C/C++ code/API anchor**

```c
#include <stddef.h>
#include <stdalign.h>

struct Sample {
    char status;
    int value;
    char quality;
};

_Static_assert(alignof(struct Sample) >= alignof(int),
               "unexpected alignment");

size_t value_offset = offsetof(struct Sample, value);
```

C++ has the same layout concerns, plus class-specific ABI rules. A C-compatible
structure is not automatically a stable cross-compiler binary protocol.

**Production/debug angle**

Inspect `sizeof`, `alignof`, and `offsetof` on supported targets. Use compiler
layout reports where available. Decode external bytes field by field rather
than casting a buffer to a structure pointer.

**Common traps**

- Calling padding wasted memory without explaining its alignment purpose.
- Assuming packed structures are automatically safe or efficient.
- Comparing structures with `memcmp`.
- Treating native structure layout as a wire format.

**Follow-up questions**

- What is trailing padding for?
- Why can packed access fail on some targets?
- How would you serialize this structure portably?

### 9. Compare undefined, unspecified, and implementation-defined behavior.

**Short answer**

Undefined behavior imposes no requirements. Unspecified behavior permits one of
several valid outcomes without requiring documentation of each choice.
Implementation-defined behavior requires the implementation to choose and
document an outcome.

**Deep explanation**

Examples of memory-related undefined behavior include out-of-bounds access,
use-after-free, invalid free, misaligned typed access, and many forbidden
uninitialized reads.

Plain-`char` signedness and concrete type sizes are common
implementation-defined properties. Evaluation-order choices can be
unspecified. A constraint violation is another category and normally requires a
diagnostic.

Optimization matters because compilers can assume undefined behavior does not
occur. The symptom can therefore change between `-O0` and `-O2`.

**C/C++ code/API anchor**

```c
int values[4] = {0};
values[4] = 1; /* undefined behavior */
```

```c
#include <limits.h>

/* Whether plain char is signed is implementation-defined. */
int plain_char_is_signed = CHAR_MIN < 0;
```

The same principle applies in C++: undefined behavior is not a catchable error
model.

**Production/debug angle**

Compile with warnings, ASan, and UBSan; reproduce at production optimization;
document implementation-defined assumptions; and remove dependencies on
unspecified evaluation choices.

**Common traps**

- Defining UB as "the program crashes."
- Treating unspecified behavior as undefined behavior.
- Assuming a diagnostic is required for every UB case.
- Calling an optimization-dependent symptom a compiler bug before validating
  the program.

**Follow-up questions**

- Why can UB appear harmless in a debug build?
- Give one implementation-defined memory-related property.
- What is the difference between UB and a constraint violation?

### 10. Why are raw structure comparison and serialization unsafe?

**Short answer**

Because structures can contain padding, implementation-specific layout, native
endianness, and representations that are not stable across builds or targets.

**Deep explanation**

Two structures can have identical member values while padding bytes differ, so
`memcmp` is not a general equality operator. Writing raw bytes also exposes
member offsets, widths, padding, and host byte order.

A portable external format defines field widths, signedness, byte order, bounds,
and versioning independently of the native structure.

**C/C++ code/API anchor**

```c
static int sample_equal(const struct Sample *left,
                        const struct Sample *right)
{
    return left->status == right->status
        && left->value == right->value
        && left->quality == right->quality;
}
```

```c
static uint16_t read_u16_be(const unsigned char bytes[2])
{
    return (uint16_t)(((uint16_t)bytes[0] << 8)
                    | (uint16_t)bytes[1]);
}
```

C++ standard-layout types still do not automatically define a portable file or
network format.

**Production/debug angle**

Use golden protocol vectors, cross-endian tests, truncated-input tests, and
field-level encoders/decoders. Inspect native layout only for ABI diagnostics,
not to infer the external contract.

**Common traps**

- Assuming zero-initializing both structures makes `memcmp` a universal design.
- Casting packet bytes to a structure pointer.
- Ignoring unaligned input.
- Using compiler packing pragmas as the entire serialization strategy.

**Follow-up questions**

- How do padding bytes affect hashing?
- Why is a `uint16_t *` cast of packet data unsafe?
- What must a wire-format specification define?

## Senior Questions

### 11. Explain effective type and strict aliasing for allocated storage.

**Short answer**

Allocated storage initially has no declared type. Typed writes and object copies
establish how it may be accessed. Reading through an incompatible lvalue can
violate effective-type and aliasing rules and produce undefined behavior.

**Deep explanation**

Compilers use aliasing rules to determine whether differently typed pointers can
refer to the same object. Breaking those rules can produce optimization-
dependent results even when the bytes and addresses look plausible.

Character types may inspect object representations. `memcpy` is the usual tool
for representation transfer when the size and destination contract are valid.
Pointer casts do not grant permission to access arbitrary bytes as another type
and do not repair alignment.

**C/C++ code/API anchor**

Risky type-punning:

```c
float value = 1.0F;
uint32_t bits = *(uint32_t *)&value;
```

Representation transfer:

```c
static uint32_t float_bits(float value)
{
    uint32_t bits;
    _Static_assert(sizeof bits == sizeof value,
                   "representation sizes differ");
    memcpy(&bits, &value, sizeof bits);
    return bits;
}
```

C++ also commonly uses `memcpy`, and newer standards provide additional
facilities for defined bit-level representation transfer.

**Production/debug angle**

Reproduce suspicious behavior at `-O0` and `-O2`, enable strict-aliasing
warnings where useful, inspect generated code, and reduce the defect to a small
case. Do not solve it globally with `-fno-strict-aliasing` without understanding
the code contract.

**Common traps**

- Saying every cast is a conversion of the stored object.
- Assuming hardware support makes the C access defined.
- Ignoring alignment while discussing aliasing.
- Applying `memcpy` without verifying equal sizes and representation intent.

**Follow-up questions**

- Why may character types inspect any object representation?
- How can a typed store affect allocated storage's effective type?
- When is `memcpy` not enough to define a semantic conversion?

### 12. Design a C buffer API with explicit ownership and safe growth.

**Short answer**

Use a state structure with `data`, `size`, and `capacity`; define one owner;
check arithmetic; commit mutations only after allocation success; document
borrowing and invalidation; and provide one release function.

**Deep explanation**

The API contract must specify:

- initialization and empty state;
- ownership and release function;
- whether pointers returned to callers are borrowed;
- which operations invalidate borrowed pointers;
- failure behavior and whether state remains unchanged;
- maximum size and overflow handling;
- thread-safety expectations.

Safe growth computes a valid new capacity, validates byte sizing, calls
`realloc` through a temporary pointer, and updates state only after success.

**C/C++ code/API anchor**

```c
struct Buffer {
    unsigned char *data;
    size_t size;
    size_t capacity;
};

void buffer_destroy(struct Buffer *buffer)
{
    if (buffer != NULL) {
        free(buffer->data);
        buffer->data = NULL;
        buffer->size = 0U;
        buffer->capacity = 0U;
    }
}
```

C++ would normally model this ownership with `std::vector<unsigned char>` or a
dedicated RAII type.

**Production/debug angle**

Use allocator failure injection, invariant assertions, fuzzed append sizes,
ASan, leak checking, and tests that retain a borrowed pointer across growth to
verify the documented invalidation rule.

**Common traps**

- Returning an owning pointer without a release contract.
- Exposing internal capacity as if it were initialized size.
- Updating `capacity` before successful growth.
- Claiming a non-null data pointer proves the whole state is valid.

**Follow-up questions**

- Should failed growth leave the buffer unchanged?
- Which operations invalidate borrowed pointers?
- How would you add a custom allocator without weakening ownership clarity?

### 13. Design a portable binary protocol parser.

**Short answer**

Treat input as bytes, validate remaining length before every access, decode
explicit widths and byte order, check offset arithmetic, and never overlay a
native structure on untrusted input.

**Deep explanation**

The parser must separate wire representation from native objects. Every field
needs a specified width, signedness, endianness, and validity range. Length and
offset arithmetic must be checked before addition or subtraction.

The parser should return structured status, avoid partially committed output on
failure, and avoid retaining pointers after the input owner can release or
reallocate the buffer.

**C/C++ code/API anchor**

```c
static int read_u16_be(const unsigned char *data,
                       size_t size,
                       size_t offset,
                       uint16_t *out)
{
    if (data == NULL || out == NULL || offset > size
        || size - offset < 2U) {
        return 0;
    }

    *out = (uint16_t)(((uint16_t)data[offset] << 8)
                    | (uint16_t)data[offset + 1U]);
    return 1;
}
```

C++ can express the same boundary using spans and value-returning error types,
but alignment and representation rules still apply.

**Production/debug angle**

Use fuzzing, truncated inputs at every byte boundary, maximum lengths,
cross-endian golden vectors, ASan/UBSan, and corpus regression tests. Treat
allocation-size and offset overflow as security defects.

**Common traps**

- Casting input to a `struct Header *`.
- Checking total length once but not before each variable-length field.
- Performing `offset + field_size <= size` when the addition can overflow.
- Returning pointers into a buffer whose lifetime is not documented.

**Follow-up questions**

- Why is `size - offset < needed` often safer after proving `offset <= size`?
- How do you parse signed fields portably?
- How would you version the protocol without exposing native layout?

### 14. Define an embedded memory policy for a long-running product.

**Short answer**

Define where allocation is permitted, maximum sizes, failure recovery, stack
budgets, fragmentation constraints, ownership rules, measurement tools, and CI
checks. Do not rely on the slogan that embedded software must never allocate.

**Deep explanation**

Static allocation improves boundedness and startup predictability. Dynamic
allocation may still be acceptable during initialization or controlled runtime
phases when the allocator, maximum live set, fragmentation, latency, and failure
strategy are understood.

Each task needs a stack budget based on worst-case call paths, recursion policy,
large locals, interrupt/context requirements where applicable to the platform,
and measured high-water behavior. Host tools do not replace target validation.

**C/C++ code/API anchor**

Relevant build and review artifacts include:

```bash
cc -fstack-usage -c module.c
size firmware.elf
nm -S firmware.elf
readelf -S -s firmware.elf
```

At the C API level, fixed-capacity buffers and caller-provided storage can make
memory bounds explicit. In C++, fixed-capacity containers or RAII owners can
express equivalent policies.

**Production/debug angle**

Track ROM/RAM section budgets, stack reports, runtime high-water measurements,
allocation failures, fragmentation metrics where applicable, and long-duration
stress tests. Verify map-file changes in CI.

**Common traps**

- Banning dynamic allocation without defining alternatives or capacity limits.
- Allowing allocation without a failure path.
- Using desktop stack sizes to justify target safety.
- Treating sanitizer success on a host as target proof.

**Follow-up questions**

- When is initialization-only allocation reasonable?
- How do you establish a worst-case stack budget?
- What evidence would approve a dynamic allocator for runtime use?

### 15. Why can memory code pass at `-O0` and fail at `-O2`?

**Short answer**

Undefined behavior, uninitialized reads, aliasing violations, lifetime errors,
and out-of-bounds accesses can be exposed or transformed by optimization.
`-O0` does not make invalid code valid.

**Deep explanation**

The optimizer assumes that defined C rules are followed. It can remove
impossible branches, reorder independent operations, keep values in registers,
reuse storage, and exploit aliasing assumptions.

Debug builds also change timing, layout, allocator behavior, and stack shape.
The different symptom is evidence to investigate the program's validity, not
proof that optimization caused the defect.

**C/C++ code/API anchor**

```c
int read_after_free(void)
{
    int *value = malloc(sizeof *value);
    if (value == NULL) {
        return 0;
    }

    *value = 42;
    free(value);
    return *value; /* undefined behavior */
}
```

Another common source is incompatible pointer type-punning that violates strict
aliasing. C++ has the same general optimization/UB interaction.

**Production/debug angle**

Build with debug information at both production and low optimization, run ASan
and UBSan, inspect warnings, reduce the case, check generated code only after
validating semantics, and avoid "fixes" such as adding `volatile`.

**Common traps**

- Blaming the compiler first.
- Disabling optimization as the permanent fix.
- Adding logging and assuming the changed symptom proves a race or timing bug.
- Treating `volatile` as a lifetime, bounds, or aliasing repair.

**Follow-up questions**

- Which sanitizers would you try first?
- Why might GDB show a variable as optimized out?
- What valid compiler assumption is violated by use-after-free?

## Coding Tasks

### Task 1. Implement Overflow-Checked Array Allocation

Implement:

```c
int allocate_int_array(size_t count, int **out);
```

Requirements:

- reject a null output pointer;
- set `*out` to `NULL` before attempting allocation;
- reject multiplication overflow;
- return success only when ownership is transferred;
- use `sizeof **out`;
- compile cleanly as C17.

**Expected solution outline**

```c
int allocate_int_array(size_t count, int **out)
{
    if (out == NULL) {
        return 0;
    }

    *out = NULL;

    if (count == 0U || count > SIZE_MAX / sizeof **out) {
        return 0;
    }

    *out = malloc(count * sizeof **out);
    return *out != NULL;
}
```

**What the interviewer evaluates**

- Arithmetic checked before multiplication.
- Explicit zero-count policy.
- Clear ownership transfer.
- No unnecessary `malloc` cast.
- Correct release expectation: caller uses `free`.

**Follow-up extensions**

- Return a status enum distinguishing invalid input, overflow, and allocation
  failure.
- Allocate a header plus payload safely.
- Inject allocator failure in tests.

### Task 2. Implement Safe Dynamic-Array Growth

Given:

```c
struct IntArray {
    int *data;
    size_t size;
    size_t capacity;
};
```

Implement a function that ensures capacity for at least `required` elements.

**Requirements**

- preserve the original array on failure;
- validate capacity and byte arithmetic;
- use a temporary `realloc` pointer;
- update `capacity` only after success;
- document borrowed-pointer invalidation.

**Evaluation points**

- Correct handling of zero initial capacity.
- No infinite growth loop or overflow.
- State invariants remain valid on failure.
- Old aliases are treated as invalid after successful growth.
- Newly allocated capacity is not confused with initialized size.

**Follow-up extensions**

- Add amortized growth without exceeding a maximum capacity.
- Replace `realloc` with an explicit allocate/copy/free strategy.
- Compare the contract with `std::vector::reserve`.

### Task 3. Parse A Big-Endian Header

Parse this wire format:

```text
byte 0..1: payload length, unsigned big-endian 16-bit
byte 2:    message type
byte 3:    flags
```

Requirements:

- reject null pointers;
- require at least four input bytes;
- decode without structure casts;
- return output only on success;
- reject payload lengths inconsistent with the available buffer.

**Evaluation points**

- Bounds checked before every indexed access.
- Explicit endianness conversion.
- No alignment assumptions.
- Correct relationship between header size, payload length, and total size.

**Follow-up extensions**

- Add a 32-bit sequence number.
- Fuzz all truncated lengths.
- Return a view into the payload with a documented input lifetime.

### Task 4. Compare Structures Without Raw Bytes

Given a structure containing an integer, a character, and a Boolean-like field:

- implement semantic equality;
- print `sizeof`, `alignof`, and member offsets;
- explain why `memcmp` is not the general solution;
- design a stable serialized representation.

**Evaluation points**

- Field-level equality.
- Awareness of internal and trailing padding.
- Explicit field widths and byte order in serialization.
- No assumption that native `bool` or padding has a stable ABI representation.

## Debugging Scenarios

### Scenario 1. `realloc` Causes A Leak

```c
buffer->data = realloc(buffer->data, new_capacity);
if (buffer->data == NULL) {
    return 0;
}
buffer->capacity = new_capacity;
```

**Expected diagnosis**

On failure, direct assignment overwrites the only pointer to the original
allocation. The old block remains allocated but unreachable.

**Expected repair**

```c
unsigned char *new_data = realloc(buffer->data, new_capacity);
if (new_data == NULL) {
    return 0;
}

buffer->data = new_data;
buffer->capacity = new_capacity;
```

The complete repair must also validate size arithmetic and document that
successful growth invalidates borrowed pointers.

**Useful diagnostics**

- Allocator fault injection.
- LeakSanitizer or Valgrind.
- ASan for stale aliases after successful movement.

**Trap**

Setting `buffer->data = NULL` after failure does not recover the lost allocation.

### Scenario 2. A Packet Parser Crashes On Some Architectures

```c
uint32_t length = *(const uint32_t *)(packet + 1);
```

**Expected diagnosis**

`packet + 1` may be misaligned. The cast also assumes native endianness and a
compatible effective type, and the code has not shown a four-byte bounds check.

**Expected repair**

Validate remaining input and decode bytes explicitly, or copy into a correctly
aligned object and perform the required byte-order conversion under a valid
representation contract.

**Useful diagnostics**

- UBSan alignment checks.
- Cross-architecture tests.
- Truncated-input fuzzing.
- GDB byte inspection with `x/Nbx`.

**Trap**

Adding a packing attribute to an unrelated structure does not make this access
portable.

### Scenario 3. Equality Fails For Apparently Identical Structures

```c
if (memcmp(&left, &right, sizeof left) != 0) {
    report_difference();
}
```

**Expected diagnosis**

Padding bytes can differ even when all member values are equal. Some object
representations may also have multiple valid encodings for the same semantic
value.

**Expected repair**

Compare each semantic field. For serialization or hashing, encode an explicit
canonical representation.

**Useful diagnostics**

- Print member offsets.
- Inspect bytes through `unsigned char`.
- Initialize and mutate members through different code paths to reproduce
  padding differences.

**Trap**

Zeroing every structure before use may make one codebase's comparison appear to
work, but it does not turn raw native layout into a stable general contract.

### Scenario 4. Release Build Returns A Value After `free`

```c
int get_value(void)
{
    int *value = malloc(sizeof *value);
    if (value == NULL) {
        return -1;
    }

    *value = 42;
    free(value);
    return *value;
}
```

**Expected diagnosis**

The return expression dereferences a dangling pointer. The behavior is
undefined and can change with optimization and allocator reuse.

**Expected repair**

Copy the value before release:

```c
int result = *value;
free(value);
return result;
```

Or avoid dynamic allocation entirely for this lifetime.

**Useful diagnostics**

- AddressSanitizer.
- Compare `-O0` and `-O2` only as a symptom investigation.
- GDB watchpoints and allocator backtraces where available.

**Trap**

Adding `value = NULL` after `free` does not permit reading the released object.

### Scenario 5. Large Input Corrupts A Small Allocation

```c
size_t bytes = count * sizeof(struct Record);
struct Record *records = malloc(bytes);

for (size_t i = 0U; i < count; ++i) {
    records[i] = input[i];
}
```

**Expected diagnosis**

The multiplication can wrap, producing a small allocation followed by writes
for the original large `count`. The code also fails to check `malloc`.

**Expected repair**

Reject `count > SIZE_MAX / sizeof *records`, check allocation success, and
validate that `input` contains at least `count` records.

**Useful diagnostics**

- Boundary tests near `SIZE_MAX`.
- ASan for the resulting overflow.
- Static analysis for allocation-size arithmetic.
- Fuzzing count and input length together.

**Trap**

Changing `bytes` to a wider-looking typedef does not help if `size_t` is already
the allocation domain and the multiplication is still unchecked.

### Scenario 6. Recursive Code Overflows A Task Stack

```c
void walk(const struct Node *node)
{
    unsigned char scratch[1024];
    scratch[0] = 0U;

    if (node != NULL) {
        walk(node->next);
    }
}
```

**Expected diagnosis**

Each active call can consume stack for call state and `scratch`. An unbounded or
unexpectedly long chain can exhaust the task's finite stack.

**Expected repair**

Bound and validate depth, reduce per-frame storage, use an iterative traversal
with explicitly bounded storage, or allocate scratch storage at a controlled
higher level.

**Useful diagnostics**

- `-fstack-usage` where supported.
- GDB `bt`, `frame`, and `info locals`.
- Target stack high-water measurement.
- Worst-case call-tree analysis.

**Trap**

Increasing the stack size without bounding input depth only moves the failure
threshold.

## Rapid-Fire Checks

- Does C require a physical stack? **No.**
- Does every local variable occupy stack memory? **No.**
- Are static-storage objects zero-initialized without explicit initializers?
  **Yes.**
- Does `malloc` initialize its bytes? **No.**
- Does `calloc` set every allocated byte to zero? **Yes.**
- Is byte-zeroing a universal constructor for every C type? **No.**
- Does `free(NULL)` have an effect? **No.**
- Is a non-null pointer necessarily valid? **No.**
- Can one-past pointers be dereferenced? **No.**
- Does successful `realloc` preserve the validity of old pointer values?
  **No.**
- Is `memcmp` a general structure equality operation? **No.**
- Does C mandate little-endian or big-endian representation? **No.**
- Does a cast repair alignment? **No.**
- Does UB always crash? **No.**
- Does ASan detect every uninitialized read? **No.**
- Is `volatile` a fix for use-after-free or concurrency ordering? **No.**
- Can dynamic allocation be acceptable in embedded software? **Yes, when its
  bounds, allocator, failure behavior, and policy are controlled.**

## Final Review Checklist

Before an interview, be able to:

- explain objects, representations, alignment, storage duration, and lifetime;
- distinguish the C abstract machine from a concrete process layout;
- compare automatic, static, thread, and allocated storage;
- explain text, read-only data, data, BSS, stack frames, and allocator storage
  without presenting them as universal C requirements;
- use `malloc`, `calloc`, `realloc`, and `free` with checked sizing and failure
  paths;
- define ownership, borrowing, invalidation, and release contracts;
- diagnose null, wild, dangling, one-past, misaligned, and out-of-bounds
  pointers;
- explain padding, endianness, effective type, and strict aliasing;
- distinguish undefined, unspecified, and implementation-defined behavior;
- investigate memory defects with warnings, ASan, UBSan, Valgrind, GDB, and
  binary-inspection tools;
- design portable parsers and embedded memory policies;
- explain why optimized builds expose invalid memory assumptions.
