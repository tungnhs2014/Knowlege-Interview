# M02-L02 — Object-Oriented C & Portability

> **Status:** APPROVED.

## 1. Learning Objectives

This lesson prepares you for the three Session 04 exercises: the Polymorphic Display Driver, the Object Pool Allocator (including its memory-layout inspection), and the Endian-Safe Protocol Parser. It develops the design reasoning needed for those exercises without supplying their completed implementations. After completing it, you should be able to:

- explain practical object-oriented C as an explicit design pattern built from ordinary C types, functions, and translation units;
- separate a module's public interface from its private data by using an incomplete, typed opaque structure;
- explain why a caller may hold and pass a pointer to an incomplete type but may not access its members or determine its size;
- distinguish a typed opaque handle from `void *`, and explain why completing the same structure type in its implementation does not require a cast;
- model a small interface as a structure of compatible function pointers and validate required interface members before an indirect call;
- trace the dependency flow from application code through an interface to a selected implementation, including a simple dummy implementation;
- compare static and dynamically created hidden configuration state at a high level, including lifetime, ownership, and failure trade-offs;
- explain fixed-capacity pools in terms of capacity, slot ownership, release, exhaustion, bounded work, and memory budget;
- validate pool membership using equality against known slot addresses rather than relational comparisons on an externally supplied pointer;
- apply M01's startup and `.bss`/`.data` mental model to a static pool without presenting a toolchain convention as an ISO C guarantee; and
- decode externally specified bytes independently of host byte order while separating alignment, effective-type/aliasing, and endianness concerns.

## 2. Why Portable Embedded C Needs Stable Interfaces

An embedded application often starts before every board, display, or sensor implementation is available. It may also need to run on a host during early development and later on more than one target. If application code directly depends on every concrete device configuration and every platform-specific operation, each hardware change spreads through the program. That makes review, replacement, and testing unnecessarily expensive.

A stable interface narrows that dependency. The application describes the operation it needs; an implementation supplies the operation for one environment. The boundary gives reviewers a concrete place to ask four useful questions:

1. What data and operations may the application use?
2. Which names, structures, and storage remain private to the implementation?
3. What must be valid before an operation is called?
4. Which parts are ISO C, and which depend on a compiler, target, or external format?

This is not a claim that C becomes C++. C can express object-like encapsulation and interface-based dispatch explicitly without requiring C++ language mechanisms. The resulting RAM, ROM, runtime, and indirect-call costs depend on the selected design, compiler, target, and build. A function-pointer table may be an excellent portability boundary in one product and unnecessary indirection in another; measure the chosen design on the supported target.

**Embedded/Linux relevance:** the same source-level application flow can select a console implementation on a development host and a display-driver implementation in firmware. The interface is useful only when its contract remains small, clear, and verified.

**What to remember:** portability begins with a stable contract, not with an assumption that every platform implements storage or hardware the same way.

## 3. Object-Like Design in C Is an Explicit Pattern

In C, an object-like component normally combines three ideas:

- a structure represents related state;
- functions operate on that state according to a documented contract; and
- a module owns the private representation and exposes only the needed declarations.

For a display component, configuration data such as a baud rate belongs with the display implementation, while the application needs operations such as initialization or drawing. The design pattern is built from C language features; C does not add inheritance, virtual-table ABI rules, exceptions, templates, or a hidden object model.

This distinction matters during review. A `struct` does not automatically enforce encapsulation, and a function pointer does not automatically make a design portable. The module boundary, the compatible types, the documented preconditions, and the selected implementation create the engineering property.

```text
data + operations + module ownership
                ↓
an explicit object-like C component
```

Keep the component focused. Session 04 needs a small display interface with two operations, not a catalogue of design patterns. More elaborate callback, command-dispatch, and state-machine architectures belong to M03.

## 4. Encapsulation with an Incomplete, Typed Opaque Type

A public header can declare a structure tag without disclosing the members:

```c
/* display.h */
#include <stdint.h>

typedef struct display_config_s display_config_t;

display_config_t *console_config_create(uint32_t baud_rate);
```

`display_config_t` is an alias for `struct display_config_s`, but the type is incomplete in this header: the caller knows that the structure type exists, not its size or fields. A caller may declare, copy, test against `NULL`, compare for equality where appropriate, and pass a `display_config_t *`. It may not create, copy, or determine the size of a complete `display_config_t` object, or write `p_config->baud_rate`, because the member list is not visible.

The implementation translation unit includes the same header and completes the same tagged structure:

```c
/* console_display.c */
#include "display.h"

struct display_config_s
{
    uint32_t baud_rate;
};
```

The definition hides the representation from other translation units while giving this implementation the information needed to use it. This is data hiding through module and header boundaries, not a runtime security mechanism. Code with access to private headers or the implementation can still see the representation, so source access and build policy also matter.

**Common mistake:** putting the complete structure definition in the public header “for convenience.” That lets every caller depend on member names, order, and representation. A later private layout change then becomes an application-wide source and potentially ABI change.

**What to remember:** an opaque type protects a representation by withholding its definition, while a typed pointer keeps the public API specific and checkable.

## 5. An Opaque Type Is Not `void *`

`void *` is a generic object-pointer type. It does not say which object representation a function expects, and it cannot be dereferenced without conversion to an appropriate object-pointer type. A typed opaque handle such as `display_config_t *` is different: it tells the compiler and reader that the argument denotes the display configuration type even when the type remains incomplete at the call site.

The forward declaration and the later definition above name the **same** `struct display_config_s`. Therefore, inside `console_display.c`, a parameter declared as `display_config_t *p_config` already points to the completed structure type. No cast to a different concrete object-pointer type is required before valid member access:

```c
static uint32_t console_baud_rate(const display_config_t *p_config)
{
    return p_config->baud_rate;
}
```

This small example assumes the function's contract has already established that `p_config` is non-null. A public boundary should check its pointers before dereference; a private helper may rely on a documented checked precondition. Casting a pointer would not make a null pointer valid, correct an incompatible object, or restore hidden type information.

This corrects a common misunderstanding in the Session 04 wording: data hiding does not require a cast. A cast is appropriate only when a real conversion is justified by the language and the interface contract; it is not a ritual associated with opaque types.

**Embedded/Linux relevance:** typed opaque handles help prevent accidental mixing of unrelated module states in both firmware and user-space components. They also make a function declaration more reviewable than an untyped `void *` context argument.

## 6. Interface Structures: Operations with Exact Types

An interface table is a structure whose members are function pointers. The Session 04 display boundary is conceptually small:

```c
typedef struct
{
    void (*init)(display_config_t *p_config);
    void (*draw_pixel)(uint16_t x, uint16_t y, uint8_t color);
} i_display_t;
```

The table expresses the operations that application code is allowed to request. It does not expose a display's private configuration layout. An implementation can provide functions and initialize an `i_display_t` value with their addresses.

Every assigned function must have a compatible signature. For example, a function declared as `void console_display_init(display_config_t *p_config)` matches the `init` member. A function with a different parameter list, return type, or pointed-to parameter qualification is not made safe by casting its function pointer. A function-pointer cast can hide a type mismatch from the compiler and create undefined behavior when the call is made.

Before an indirect call, validate both the interface pointer and the specific required member. A small check can make the required contract visible:

```c
#include <stdbool.h>

static bool display_can_draw(const i_display_t *p_display)
{
    return (p_display != NULL) && (p_display->draw_pixel != NULL);
}
```

The caller must also satisfy the contract of the selected operation. For example, an initialization operation may require a valid `display_config_t *`, while a drawing operation may require coordinates within the selected display's documented range. Checking a non-null function pointer does not validate every operation-specific input.

**Common mistake:** validating `p_display` but not `p_display->draw_pixel`, then assuming the indirect call is safe. Each required pointer is an independently required precondition.

**Boundary:** this lesson introduces one declaration form and one small table. Function-pointer arrays, callback registration, command dispatch, and finite-state-machine design remain M03 work.

## 7. Polymorphic Application Flow

Polymorphism in this narrow C design means that application code can invoke the same interface operation through different valid implementations. It does not mean that C supplies a universal runtime type system.

```text
Application drawing logic
          │ uses only i_display_t
          ▼
Display interface (init, draw_pixel)
       ┌──┴─────────────────┐
       ▼                    ▼
Console implementation   Dummy implementation
host-visible output      records safe test activity
```

The application depends on the interface declaration, not on console-specific state or dummy-specific counters. Replacing one valid implementation with another does not require the application function to learn the new private layout. The implementation depends on the same public interface so the compiler can verify the assigned function signatures.

This is HAL-style decoupling at a small, source-level boundary: application intent flows to an interface, then to a selected implementation. It is not a complete hardware architecture and does not access registers. Real memory-mapped I/O, `volatile`, and register read-modify-write contracts belong to M05.

**What to remember:** unchanged application code is useful evidence of a successful replacement only if both implementations genuinely satisfy the same interface contract.

## 8. A Simple Dummy Is a Useful Test Double

A simple dummy implementation lets application logic run before the physical display is ready. It can accept a valid call and increment a private call counter instead of writing a pixel. The application can then demonstrate that it requested four drawing operations without requiring a device.

The dummy is valuable for two limited reasons:

1. it confirms that the application depends on the interface rather than on concrete hardware state; and
2. it gives a controlled observation of application behavior.

It is not a claim that a call count proves visual correctness, timing, electrical behavior, or target-driver correctness. It is also not a framework-based mocking lesson: no expectation API, test-driven-development workflow, or external test framework is needed here. Those practices belong to M07.

## 9. Hidden Configuration State: Static or Dynamically Created

An opaque configuration factory can choose an implementation strategy without exposing it to the application. Two broad choices are common:

| Strategy | Useful property | Contract to make explicit | Main trade-off |
| --- | --- | --- | --- |
| Static hidden configuration | Fixed storage with no heap-allocation failure; capacity, reuse, and initialization/failure policy remain explicit | Capacity, lifetime, reuse, and whether a later call changes the same object | A fixed number of configurations and less per-instance flexibility |
| Dynamically created hidden configuration | Separate configuration instances when the project permits dynamic allocation | Allocation-failure result, owner, release function, and lifetime | Heap policy, fragmentation, timing, and cleanup obligations |

Many embedded projects restrict or prohibit dynamic allocation according to their safety, timing, lifetime, or certification policy. That is a project decision, not a universal rule of C. A design should not return an opaque pointer without documenting who owns it, how long it stays valid, and—if allocation is used—how a caller releases it.

**Common mistake:** assuming an opaque pointer alone establishes lifetime safety. It hides fields, but it cannot prevent an implementation from returning a pointer whose storage has expired or a caller from retaining a handle after the contract says it is no longer valid.

## 10. Memory Optimization Means Predictability First

For a constrained product, “optimization” is not merely minimizing `sizeof` in a source file. The design must fit the selected target's RAM and image budget, provide a bounded worst case where the system requires one, and retain enough margin for call stacks and other components. A smaller allocation strategy that fails under peak load is not an optimization.

Start with quantities that can be reviewed:

- maximum number of simultaneously live objects;
- bytes per object for the actual target build;
- metadata required to manage the objects;
- lifetime and ownership of each object;
- behavior when capacity is exhausted; and
- evidence from the selected compiler, linker, target, and workload.

M01 established that `sizeof` and section reports are observations for one build. Apply that habit here: a capacity calculation must account for the actual type layout and bookkeeping, not just the fields a reader expects to see. Measure before and after a design change instead of assuming that a particular technique saves RAM, ROM, or execution time.

## 11. Fixed-Capacity Object Pools

An object pool reserves a known set of same-sized slots and manages which slots are free or in use. For the Session 04 packet exercise, the mental model is a static array of packet objects plus separate occupancy metadata:

```c
static network_packet_t s_pool[POOL_CAPACITY];
static bool s_in_use[POOL_CAPACITY];
```

An allocation operation searches for an available slot, records that the slot is in use, and returns its address. A release operation returns an owned slot to the available state. When every slot is in use, allocation fails in a documented way; exhaustion is an expected runtime condition, not an exceptional reason to read or write beyond the array.

The ownership contract is central:

```text
allocation succeeds → caller temporarily owns one slot
caller uses slot    → only within the pool API's documented lifetime
release succeeds    → slot is available to the pool again
after release       → caller must not continue to use that storage as owned
pool full           → caller follows the documented failure path
```

A pool is not a general replacement for every allocation need. It works best when object size and maximum simultaneous count are known. If an object has a different size, it needs a different representation or a separately designed pool; silently treating a fixed slot as arbitrary storage defeats the pool's type and capacity contract.

## 12. Complexity, Bounds, and Fragmentation

If allocation scans slots from index zero to `POOL_CAPACITY - 1`, its algorithmic complexity is O(N) in the capacity N. It is inaccurate to call that scan O(1) merely because one particular build uses a small constant such as five slots. A compile-time fixed N can still provide a known worst-case number of checks, which is often useful for an embedded timing budget. Bounded work and O(1) complexity are different claims.

A uniform fixed-size pool avoids **external fragmentation within that pool**: releasing one slot does not leave a hole that prevents a same-sized later slot request. The pool can nevertheless be exhausted, reserve unused capacity, or waste space internally when its slot size is larger than the payload an operation needs. Therefore, “no fragmentation” is too broad without the pool boundary and object-size assumption.

General-purpose `malloc`/`free` may be appropriate under a documented project policy, but mixed sizes and lifetimes can make allocation timing and fragmentation harder to predict. The lesson's design question is narrower: when a maximum number of same-sized packets is known, can a fixed-capacity pool make resource usage and failure behavior easier to review?

## 13. Safe Pool Pointer Validation

A release API must handle `NULL` according to its documented contract and must avoid accepting a pointer to an object that is not one of its slots. Do not establish membership with relational comparisons or pointer subtraction between an externally supplied pointer and pool elements: those operations have C rules that do not make arbitrary unrelated pointers a portable array-range test.

For a valid object-pointer value supplied by a caller, a simple portable teaching approach is to compare it for equality with each known slot address:

```c
for (index = 0U; index < POOL_CAPACITY; ++index)
{
    if (p_packet == &s_pool[index])
    {
        /* This is the matching slot. */
    }
}
```

The comparison checks exact slot identity. It does not validate a pointer that has no valid C pointer value, repair a dangling pointer, or define the rest of the release algorithm. The API contract must still say whether double release is an error and what state metadata is required. The completed `packet_free()` implementation is intentionally left to the exercise.

**What to remember:** validate the pointer value required by the API, then compare identity against known pool elements. Do not invent a numerical address range or use unrelated-pointer ordering as proof of membership.

## 14. Object Pools and the M01 Memory Model

Static-storage-duration pool arrays have required zero-initialization semantics when no explicit nonzero initializer is provided. In common embedded builds, zero-initialized pool storage and occupancy flags are represented in BSS-like runtime RAM; startup clears the RAM range before application code begins. This normally avoids storing an equivalent zero payload byte for every BSS byte in the firmware image. The startup environment needs the BSS RAM range or equivalent information, but that does not mean the image must contain one zero byte for each BSS byte.

These are common startup and toolchain conventions, not physical placement rules mandated by ISO C. For example, both declarations below require zero initialization as C semantics:

```c
static network_packet_t s_pool_a[POOL_CAPACITY];
static network_packet_t s_pool_b[POOL_CAPACITY] = {0};
```

Many toolchains place either representation in BSS-like storage; a toolchain may choose differently. The right way to answer the Session 04 Part B questions is to inspect the selected build and state its compiler, linker, and target context. Likewise, function bodies commonly contribute to text-like code storage, and writable static state commonly needs runtime RAM, but exact section names and physical Flash/RAM placement remain target and linker decisions.

Use the generic budget instead of assuming a universal number:

```text
pool RAM budget = capacity × sizeof(element)
                + occupancy/bookkeeping storage
                + implementation and alignment effects
```

Increasing capacity changes both the array contribution and any metadata indexed by capacity. Calculate the target build's values for the exercise; this lesson deliberately does not supply the Part B numerical answer.

## 15. Portability Is More Than Successful Compilation

Code can compile on two systems yet disagree about an external protocol, an enum representation, alignment, byte order, or module lifetime. Portable code makes the relevant contract explicit and verifies target-dependent facts where they matter.

For this lesson, a practical portability checklist is:

| Boundary | Question to answer | Appropriate evidence |
| --- | --- | --- |
| Public C API | Are types, pointer validity, ownership, and failure results documented? | Header review and focused tests |
| Module representation | Can callers avoid depending on private fields and layout? | Incomplete type and translation-unit boundary |
| Interface dispatch | Do implementation functions match each required function-pointer signature? | Compiler type checking and interface review |
| Pool resource use | Does peak capacity fit the target memory budget? | `sizeof`, map/section evidence, and target build |
| External bytes | Does decoding follow the protocol's specified width and byte order? | Known byte vectors and protocol review |

Fixed-width integer types communicate the width of an external value when that width is part of the contract; they do not themselves establish the host's byte order. A host's object representation and a protocol's external byte sequence are separate things.

## 16. Endian-Safe Byte Decoding

An external protocol defines the order and meaning of its bytes. A host's endianness describes how that host represents multi-byte objects in memory. The two must not be conflated. If a protocol says a three-byte length is big-endian, decode the byte significance explicitly rather than loading a host integer and hoping the host order matches.

```c
static uint32_t decode_be24(const uint8_t p_bytes[3])
{
    return ((uint32_t)p_bytes[0] << 16U)
         | ((uint32_t)p_bytes[1] << 8U)
         | (uint32_t)p_bytes[2];
}
```

The example is intentionally a three-byte field, not the Session 04 six-byte payload. It shows the portable reconstruction principle: the first byte is explicitly assigned the most-significant position, so the resulting numeric value does not depend on whether the host is little- or big-endian.

Each byte is converted to `uint32_t` **before** a wide left shift. Small integer types undergo integer promotion in expressions. On a target where `int` is too narrow for a shifted intermediate, relying on the promoted expression can be invalid or produce an unintended result. The explicit unsigned destination-width conversion makes the intended shift domain visible. It does not replace validation that the input pointer and readable extent satisfy the function's contract.

**Common mistake:** treating a raw byte array as though it is already a host `uint32_t`. The byte layout is a protocol fact, not a promise about the host representation.

## 17. Alignment, Effective Type/Aliasing, and Endianness Are Separate Hazards

Casting a `uint8_t *` buffer to a wider integer pointer and dereferencing it can be wrong for more than one reason. Review each hazard separately.

| Hazard | Question | What can go wrong | Better Session 04 approach |
| --- | --- | --- | --- |
| Alignment | Does the byte address satisfy the wider type's alignment requirement? | A wider load may be inefficient, rejected by generated code, or fault on a target that does not support that access. | Read individual bytes. |
| Effective type / aliasing | Is this storage being accessed through an appropriate lvalue type for how it was created? | Access through an incompatible wider lvalue can violate C object-access rules and invalidate compiler assumptions. | Keep the input as bytes and construct the value explicitly. |
| Endianness | Does a valid native load interpret bytes in the protocol's specified order? | A valid load can still produce the wrong value on a host with a different byte order. | Assign byte significance with shifts and OR operations. |

Strict-aliasing assumptions and alignment are not the same mechanism. Aliasing does not itself cause an unaligned hardware fault; the alignment of the selected address and the target's access behavior govern that risk. Conversely, an aligned load is not necessarily a protocol-correct or type-correct load. Explicit byte decoding avoids relying on all three assumptions at once.

## 18. The Fixed-Length Parser Contract

Session 04 specifies this interface:

```c
void parse_sensor_data(const uint8_t *p_buffer,
                       sensor_data_t *p_out_data);
```

The interface has no length argument. It can check whether `p_buffer` and `p_out_data` are null, but it cannot independently establish that `p_buffer` refers to six readable bytes. For that exercise, “`p_buffer` refers to at least six readable bytes” is a caller/function precondition. The caller must ensure that precondition before calling the function; the implementation necessarily relies on it when it reads the six-byte payload.

A general production parser often accepts an explicit length or capacity when input extent is not established by another controlled boundary. That observation does not change the exercise prototype. It simply identifies the assumption that the fixed-payload API needs its caller to uphold.

**What to remember:** a non-null pointer alone does not prove that a required byte range is readable.

## 19. Common Failure Patterns and Review Questions

| Review question | Misleading conclusion | Better engineering interpretation |
| --- | --- | --- |
| Can the application access opaque configuration fields? | The pointer type is known, so the fields are public. | The caller knows only an incomplete type; only the implementation has the completed representation. |
| Must a completed opaque type be cast before member access? | Opaque means `void *`, so a cast is required. | The implementation completes the same tagged type; no conversion is inherently needed. |
| Is every function-pointer table safe? | The functions were assigned, so an indirect call cannot fail. | Signatures must be compatible, and the required table/member pointers must be valid before calling. |
| Is a fixed pool allocator O(1)? | A small fixed scan is automatically constant time. | A scan is O(N); fixed N gives a known bounded worst-case number of checks. |
| Does a uniform pool eliminate every waste or failure? | There can be no fragmentation or allocation failure. | It avoids external fragmentation within the pool, but may waste capacity and can be full. |
| Does `= {0}` force `.data`? | An explicit initializer fixes the object-file section. | Both forms have zero-initialization semantics; build representation is toolchain-specific. |
| Does a `uint8_t *` to `uint32_t *` cast only involve byte order? | A byte swap is the only concern. | Alignment, effective-type/aliasing, and endianness are separate requirements. |
| Does a non-null parser buffer prove six bytes are available? | A pointer validates its own extent. | The fixed-payload API requires a documented readable-range precondition. |

Use the following focused review order before accepting a Session 04 design:

1. Read the public header and identify every pointer, ownership, and failure contract.
2. Check that the application uses only the public interface, not private representation details.
3. Confirm each indirect-call signature and every required non-null check.
4. Account for peak pool storage, metadata, and full-pool behavior on the selected target.
5. Compare only known pool-slot addresses when validating membership.
6. Decode external bytes according to the protocol specification, not the observed host layout.

## 20. What to Remember

- Object-oriented C is an explicit arrangement of data, functions, headers, and translation-unit ownership; it is not C++ hidden inside C.
- An incomplete typed structure lets callers hold a specific handle while the implementation hides its fields and layout.
- A typed opaque pointer is not `void *`. Completing the same structure type in the implementation does not require a cast before member access.
- A function-pointer interface requires exact compatible signatures and validation of the interface and required member before an indirect call.
- Application → interface → implementation keeps application logic independent of a selected console, dummy, or target-specific implementation.
- A simple dummy can demonstrate decoupling before hardware exists; it does not prove full hardware correctness and is not a mocking-framework lesson.
- A fixed-capacity pool makes capacity, ownership, release, and exhaustion explicit. A linear scan is O(N), even when its fixed capacity makes the work bounded.
- A uniform pool avoids external fragmentation within that pool, but it can still waste capacity or become full.
- Static zero-initialized pool storage commonly uses BSS-like RAM with startup zeroing and normally no equivalent per-byte zero image payload; exact sections and placement are build-specific.
- Budget a pool as element storage plus bookkeeping and implementation effects, then measure the target build.
- External byte order is a protocol contract. Decode byte-by-byte with destination-width casts before wide shifts.
- Alignment, effective type/aliasing, and endianness are three distinct reasons not to treat a byte buffer as a native wider object.
- A fixed-payload parser without a length parameter depends on its caller to provide the documented minimum readable byte range.

## 21. Further Reading

- [ISO C11 draft N1570](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf), for incomplete types, pointer operations, and integer expressions.
- [GCC Optimize Options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html), including the target-dependent optimization set and `-fstrict-aliasing` at common optimization levels.
- [GNU Binutils `size` documentation](https://sourceware.org/binutils/docs/binutils/size.html) and [GNU Binutils `objdump` documentation](https://sourceware.org/binutils/docs/binutils/objdump.html), for inspecting the selected build rather than inferring sections from C source.
- The approved M01-L02 memory-layout lesson, for the startup, section, stack, and allocation foundations used here.
