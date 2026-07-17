# M04-T01 — Pointers, Arrays, Decay and Bounds

> **Draft for human review.** This is the canonical lesson draft for M04-T01; it is not approved yet.

## Metadata and scope

| Field | Value |
| --- | --- |
| Topic | `M04-T01-pointers-arrays-decay-and-bounds` |
| Module | M04 — Advanced Pointers and Object Representation |
| Status | `LESSON_DRAFT`; lesson `DRAFT`; human review `PENDING` |
| Prerequisites | M01-T02, M01-T03, M02-T01; Pre-Training pointer and array syntax |
| Context | **ISO C** semantics; **compiler/tool** diagnostics and sanitizers |

This lesson explains how to reason about array objects, pointer values, and explicit bounds contracts.

Out of scope: allocation and ownership (M03-T03); `void *`, double pointers, and generic APIs (M04-T02); function pointers (M04-T03); aliasing/alignment/object representation (M04-T04); strings (M05-T02); parsing (M05-T03); and MMIO/`volatile` (M06-T02). Pointer provenance is intentionally not modelled here.

## Learning objectives

After this lesson, an engineer should be able to:

1. Distinguish a pointer object, pointer value, designated object, array object, extent, and valid operation domain.
2. Explain array-to-pointer conversion and the important contexts where it does not occur.
3. Specify an API contract that carries the required extent explicitly.
4. Apply the array-domain rules for arithmetic, subscripting, and one-past pointers.
5. Choose `size_t` or `ptrdiff_t` for the correct question, and review pointer subtraction and comparison separately.
6. Read a fixed-column multidimensional parameter and reject `T **` as a substitute for an array of arrays.
7. Review common bounds defects using compiler, sanitizer, debugger, test, and contract evidence without treating any one tool as proof.

## 1. Pointer objects, pointer values, arrays, and valid operations

Start with objects and operations, not with printed numeric addresses.

| Term | Meaning |
| --- | --- |
| Pointer object | An object of pointer type that stores a pointer value. |
| Pointer value | The value stored in a pointer object or produced by a pointer expression. |
| Designated object | The object an access expression is intended to reach. |
| Array object | An object with an element type and a fixed extent. |
| Array extent | The number of elements in that particular array object. |
| Valid operation domain | The objects and positions for which a proposed operation is permitted. |

In `*p`, `p` supplies a pointer value and `*p` attempts an access. A non-null value alone does not prove that access valid: the value may be one-past an array, indeterminate, dangling, or otherwise outside the required domain.

Use these states operationally:

| State | Review consequence |
| --- | --- |
| Null | Not a dereference target or arithmetic base. A contract may accept it for a zero-length input. |
| Valid for this access | Designates a suitable live object or array element and satisfies the operation’s preconditions. |
| One-past | May take part only in permitted operations; never dereference it. |
| Indeterminate/uninitialised | Do not read, compare, or dereference it. |
| Dangling | Its former object’s lifetime ended; repair of ownership/lifetime is M03-T03. |

`NULL` is not another name for dangling, indeterminate, or invalid. A null check is only one possible contract check.

## 2. Arrays are not pointers

```c
int samples[4] = {10, 20, 30, 40};
int *element = samples;
int (*whole_array)[4] = &samples;
```

`samples` is an array object of four `int` elements. `element` and `whole_array` are separate pointer objects.

In the initializer for `element`, the array expression converts to a pointer to `samples[0]`. That conversion does not transform `samples` into an `int *` object.

`element` is a pointer to one `int`; permitted `element + 1` advances by one `int` element. `whole_array` is a pointer to an array of four `int`; permitted `whole_array + 1` advances by one complete array-of-four object.

On many targets their initial printed addresses appear alike. Their types and arithmetic meanings are still different. Never use “the array name is a pointer” as a shortcut.

## 3. Array-to-pointer conversion and non-conversion contexts

**ISO C.** In ordinary applicable expression contexts, an array expression converts to a pointer to its initial element. Passing an array to a function is a common case.

```c
#include <stddef.h>

static int sum_values(const int *values, size_t count)
{
    int total = 0;

    for (size_t i = 0U; i < count; ++i) {
        total += values[i];
    }

    return total;
}
```

The pointer argument does not carry `count`; the caller must provide the extent through the API contract.

Three important contexts do not apply this conversion rule:

| Context | Consequence |
| --- | --- |
| `sizeof array` | Observes the array object’s size where that array object is visible. |
| `&array` | Produces a pointer to the complete array, not a pointer to its first element. |
| String literal initialising an array | Initialises an array object; it does not make that array a pointer object. |

```c
int readings[8];
size_t element_count = sizeof readings / sizeof readings[0];
int (*array_address)[8] = &readings;
int *first_element = readings;
char label[] = "ADC";
```

The `sizeof` formula is valid only when the operand is an actual array in that context. Applying it to a pointer cannot recover a pointed-to array’s extent.

Full string-literal storage, lifetime, and modification rules belong to M05-T02.

## 4. Function parameter adjustment and explicit bounds contracts

This declaration:

```c
void process(int values[16]);
```

does not pass an `int[16]` object by value. In a parameter declaration, C adjusts an array-of-type parameter to a pointer-to-type parameter. The `16` does not automatically enforce a runtime bound.

Therefore, inside the function, `sizeof(values)` is the size of the adjusted pointer parameter on that implementation, not the caller array’s size.

### Pointer plus count

Use a precondition when a function cannot report an invalid argument separately:

```c
#include <stdbool.h>
#include <stddef.h>

static bool has_sample(const int *values, size_t count, int target)
{
    /* Precondition:
     * values points to at least count accessible int elements.
     */
    for (size_t i = 0U; i < count; ++i) {
        if (values[i] == target) {
            return true;
        }
    }

    return false;
}
```

The API must state its null/zero-length policy. For example, it may accept `values == NULL` only with `count == 0`, or it may forbid null for every call. Do not silently translate a violated precondition into “not found”; full error modelling belongs to M08-T01.

### Other useful contracts

| Contract | What it makes explicit |
| --- | --- |
| Pointer + element count | The number of accessible elements. |
| Pointer + capacity + used length | Space available versus elements currently meaningful. |
| Half-open `[begin, end)` | A start and the exclusive endpoint of one array range. |

For an append operation, a common precondition is `used_length < capacity`. Review count and byte-size calculations for overflow before relying on them.

A half-open traversal is valid only when `begin` and `end` describe the same array domain:

```c
#include <stdbool.h>

static bool contains_value(const int *begin, const int *end, int target)
{
    while (begin != end) {
        if (*begin == target) {
            return true;
        }
        ++begin;
    }

    return false;
}
```

The loop compares `end`; it never dereferences it.

## 5. Pointer arithmetic, subscripting, and one-past

Pointer arithmetic is scaled by the pointed-to element type. Permitted `p + 1` for `T *p` identifies the next `T` element; it is not specified as adding one byte.

The rules are defined in terms of an array-object domain. For these rules, a pointer to a standalone object behaves as a pointer to the first element of an array of length one. This does not create a larger array around the object.

Before writing `p + n` or `p[n]`, identify the initial element, the array extent, and the intended index. Then show that the resulting position remains an element of that array or its permitted one-past position. This reasoning is portable; guessing from an address layout is not.

If `p` points to the final element of an array, `p + 1` may form a one-past pointer. It can be used as an exclusive endpoint in permitted operations, but it does not designate an element.

```c
/* Incorrect — undefined behavior: a one-past pointer is not dereferenceable. */
int values[4] = {0};
int *one_past = values + 4;
*one_past = 42;
```

Formation and dereference are different operations. Null pointers establish no array domain and are not valid arithmetic bases.

```c
/* Incorrect — undefined behavior. */
int *p = NULL;
int *next = p + 1;
```

For the relevant operand types, `p[i]` means `*((p) + (i))`. Subscript notation therefore inherits the same array-domain and bounds requirements as explicit pointer arithmetic.

```c
/* Incorrect — undefined behavior: valid indices are 0 through 3. */
int values[4] = {0};
values[4] = 1;
```

Numeric-looking addresses do not justify arithmetic outside these rules.

## 6. `size_t`, `ptrdiff_t`, subtraction, and comparison

Include `<stddef.h>` for both types.

| Need | Type |
| --- | --- |
| Object size, element count, non-negative index | `size_t` |
| Signed distance between permitted positions in one array | `ptrdiff_t` |

Pointer subtraction is defined only when both operands point to elements of the same array object, or to its permitted one-past position. The result is an element distance, not a byte count.

```c
#include <stddef.h>

static ptrdiff_t distance_in_same_array(const int *left, const int *right)
{
    /* Precondition: left and right are positions in one array domain. */
    return left - right;
}
```

`ptrdiff_t` is signed. The difference must be representable in `ptrdiff_t`; otherwise behavior is undefined. Do not store a possibly negative difference in `size_t`.

### Equality and inequality

`==` and `!=` test equality under the standard’s pointer-equality rules. They are useful for checking a documented half-open endpoint.

Pointer equality has additional standards-defined edge cases. Do not infer those rules only from printed addresses.

### Relational comparison

`<`, `<=`, `>`, and `>=` describe ordering only for relationships defined by the standard, such as positions in the same array. They are not a universal portable ordering for arbitrary objects.

```c
/* Incorrect: unrelated subtraction or relational ordering is not a layout query. */
int left_object = 0;
int right_object = 0;
ptrdiff_t distance = &left_object - &right_object;
bool ordered = &left_object < &right_object;
```

Do not execute this intentionally incorrect code to discover stack or heap layout. Equality and relational comparison are separate questions; not every equality comparison between unrelated objects is automatically forbidden.

## 7. Fixed-column multidimensional arrays

A true multidimensional array is an array of arrays. In the example below, the column count is part of the element type.

```c
#include <stddef.h>
#include <stdint.h>

enum { SAMPLE_COLUMNS = 3 };

static uint16_t row_peak(
    size_t row_count,
    size_t row,
    const uint16_t samples[][SAMPLE_COLUMNS])
{
    /* Precondition:
     * samples points to at least row_count rows.
     * row < row_count.
     */
    (void)row_count; /* Exposes the API contract; no error policy here. */

    uint16_t peak = samples[row][0];

    for (size_t column = 1U; column < SAMPLE_COLUMNS; ++column) {
        if (samples[row][column] > peak) {
            peak = samples[row][column];
        }
    }

    return peak;
}
```

After parameter adjustment, `samples` is a pointer to an array of `SAMPLE_COLUMNS` `uint16_t` elements. That pointer-to-array type carries the row width needed for `samples[row][column]`.

`uint16_t **` is not equivalent: it describes pointer indirection, not this fixed-column array-of-arrays layout. This pattern suits a small image row, sampled-data block, or fixed lookup table; it does not imply ragged allocation, cache optimisation, ABI claims, or protocol layout.

**Optional advanced note.** C also has VLA and `static` array-parameter forms. Their exact semantics and toolchain support require verification before a future example. `static` in an array parameter does not add automatic runtime bounds checking.

## 8. Common defects and practical verification

| Defect | Review question |
| --- | --- |
| Off-by-one or one-past dereference | Is the exclusive end only compared, never dereferenced? |
| Out-of-bounds subscript | What array extent permits this index? |
| Missing/incorrect extent | Where does the caller supply count, capacity, or end? |
| Uninitialised or dangling pointer | Which assignment or lifetime fact makes this access valid? |
| Wrong multidimensional parameter | Does the parameter encode the required fixed column count? |
| Invalid subtraction/order | Which common array domain permits this operation? |

Firmware lookup tables are a common application: validate an externally supplied state or error code against the table extent before evaluating `table[code]`. Bracket syntax does not remove the bounds requirement.

The same membership reasoning appears in fixed pools, while their allocation policy belongs to M03-T04.

| Method | Useful evidence | What it cannot prove |
| --- | --- | --- |
| Compiler warnings | Suspicious constructs and some bounds issues | Complete contract correctness |
| Sanitizers | Runtime defects on executed instrumented paths | Unexecuted paths or all target builds |
| Debugger | Pointer values and failure locations | Intended API bounds contract |
| Boundary tests | Selected edge-case behavior | Complete semantic correctness |
| Static review | Contract, extent, and operation reasoning | Every runtime path by itself |

One useful local command is:

```bash
cc -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -fsyntax-only file.c
```

Warning availability and sanitizer coverage depend on compiler, version, platform, optimization, and configuration. A clean warning or sanitizer run is evidence, not proof. `volatile` does not add bounds, lifetime, atomicity, ordering, or memory safety.

## 9. Engineering checklist and summary

Before accepting pointer-and-array code, ask:

1. What object or array domain is involved, and where does its extent come from?
2. Has array-to-pointer conversion occurred, and is the remaining type the one the code assumes?
3. Is the pointer valid for this operation, or null, one-past, indeterminate, or dangling?
4. Is the operation a dereference, subscript, arithmetic step, subtraction, equality check, or relational comparison?
5. Does it remain in the permitted array domain, and is the API’s count/capacity/end contract explicit?
6. Does the problem belong to another canonical topic rather than being solved with an unsafe shortcut here?

An array object has extent; a pointer value does not carry it. Conversion makes a pointer to the initial element in applicable expressions, but does not make arrays and pointers identical. Reliable C APIs state the valid domain explicitly and use tools to test assumptions rather than to replace them.

## Verification notes

Language semantics were checked against an ISO C17 public draft. Tool statements were checked against official GCC and Clang documentation. Detailed source mapping remains in [`source-inventory.md`](../../../_meta/topic-design/M04-T01-pointers-arrays-decay-and-bounds/source-inventory.md).

Verification pending: VLA and `static` array-parameter forms require a selected C17 toolchain check before a verified example is added.

---

Human review is pending. `lesson.md` remains `LESSON_DRAFT`; no examples, exercises, solutions, interview material, or next-topic work is authorised.
