# M03-L01 — Arrays, Double Pointers & Function Pointers

> **Status:** APPROVED.

## 1. Why Advanced Pointer Design Matters

Embedded software often selects behaviour or data without copying it: a diagnostic message is selected from a table, a board variant selects a device descriptor, or an operation is selected through a function pointer. These designs can be small, fast to inspect, and easy to extend, but only when their type, bounds, lifetime, and toolchain assumptions are explicit.

The difficult part is not punctuation such as `*`, `[]`, or `()`. The difficult part is deciding what each object contains, how long it remains valid, and which checks must happen before it is indexed or called. This lesson develops those reasoning habits for pointer-based tables and prepares the learner for Session 05 without supplying either exercise solution.

This lesson is about ISO C99 pointer and declaration semantics plus a narrowly labelled GCC/Clang-family custom-section feature. It is not a complete pointer manual, a callback system, an FSM implementation, an MMIO lesson, or a linker-script tutorial.

## 2. Array and Pointer Mental Model

### 2.1 An array object is not a pointer object

An array object contains a fixed number of contiguous element objects. A pointer object is a separate object whose value can designate another object or function. Their expressions often cooperate, but their types, sizes, assignment rules, and storage are different.

In many expressions, an array expression converts to a pointer to its first element. This is **array-to-pointer conversion**, often informally called decay. It explains why `samples` can be passed to a function expecting `uint8_t *`, but it does not turn the array object into a pointer object.

The conversion does not occur when the array is the operand of `sizeof`, unary `&`, or a string literal used to initialise an array. Therefore, `sizeof(samples)` can observe an actual array's complete size, while `sizeof(p_samples)` observes only the size of a pointer object. The complete array must still be visible at that expression; after array-parameter adjustment, the parameter is a pointer.

`&samples` has type “pointer to array of 3 `uint8_t`” in the example below. It points to the whole array object, not merely to its first element. Incrementing such a pointer, where permitted, advances by an entire three-element array.

### 2.2 Complete standalone C99 example — an array, its first element, and its whole object

```c
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

static void print_samples(const uint8_t data[], size_t count)
{
    size_t index;

    for (index = 0U; index < count; ++index)
    {
        (void)printf("sample[%zu] = %u\n", index, (unsigned int)data[index]);
    }
}

int main(void)
{
    uint8_t samples[] = { 4U, 7U, 9U };
    uint8_t *p_first = samples;
    uint8_t (*p_whole)[3] = &samples;
    size_t array_bytes = sizeof(samples);
    size_t pointer_bytes = sizeof(p_first);

    if ((p_first != &samples[0]) || (p_whole != &samples))
    {
        return 1;
    }

    (void)printf("array bytes: %zu; pointer bytes: %zu\n", array_bytes, pointer_bytes);
    print_samples(samples, sizeof(samples) / sizeof(samples[0]));

    return 0;
}
```

The declaration `void print_samples(const uint8_t data[], size_t count)` is adjusted by C to a pointer parameter. Inside that function, `data` has pointer type, so `sizeof(data)` would not recover the caller's array extent. Passing a count is an interface contract, not optional decoration.

### 2.3 Bounds, one-past positions, and scaled movement

For an array with `count` elements, a pointer to element zero may be advanced to `data + count`. That one-past position is useful as the exclusive endpoint of a half-open range such as `[data, data + count)`. It may be formed and compared in the permitted array-object domain, but it must never be dereferenced.

Pointer addition is scaled by the pointed-to type. Advancing a `uint8_t *` by one addresses the next `uint8_t`; advancing a `device_t *` by one addresses the next `device_t`. No portable program should infer a fixed byte distance from source syntax alone.

Pointer arithmetic, subtraction, and relational comparison need the appropriate common array-object domain. Do not subtract pointers from unrelated objects or use relational comparison to order them. Equality comparison has its own C rules, but it never authorises dereferencing a null, dangling, out-of-range, or one-past pointer.

**Common mistake:** treating `data + count` as the last element. It is the endpoint after the last element. The last valid element, when `count > 0U`, is `data[count - 1U]`.

**Embedded and Linux relevance:** drivers, protocol parsers, and user-space buffers commonly use a pointer plus a count. A half-open range avoids special handling for empty ranges and makes the bound check visible.

**Must remember:** an array can commonly supply a pointer to its first element, but it retains array identity where C does not apply the conversion.

## 3. Arrays of Pointers

### 3.1 The representation

`T *items[N]` declares `items` as an array containing `N` pointer objects, each capable of designating a `T`. It does **not** declare one contiguous array of `N` `T` objects. The pointed-to objects may be elsewhere, may have different storage durations, and may even be absent if a table entry is null.

This representation is useful for a table of string pointers, a selection of pre-existing descriptors, or a board-specific catalogue. It saves copying when the table should refer to existing data, but it introduces two contracts: the index must be valid and the selected pointee must remain alive for the duration of use.

### 3.2 Const distinctions for string-pointer tables

| Declaration | What may change through this name? | Typical meaning |
| --- | --- | --- |
| `const char *p` | `p` may point elsewhere; characters may not be modified through `p`. | Read-only text reference. |
| `char *const p` | `p` itself cannot point elsewhere; characters may be modified if the referenced object is modifiable. | Fixed pointer to mutable characters. |
| `const char *const p` | Neither the pointer nor characters may be modified through `p`. | Fixed read-only text reference. |

For a table, `const char *const labels[]` means each table entry is a fixed pointer, and each entry points to characters that cannot be modified through the table. It says nothing by itself about the physical memory section where a toolchain places either the table or the text.

### 3.3 Complete standalone C99 example — selecting a checked label

```c
#include <stddef.h>
#include <stdio.h>

static int print_mode_label(size_t mode_index)
{
    static const char *const s_mode_labels[] = {
        "standby",
        "measure",
        "service"
    };
    const char *p_label;
    size_t label_count = sizeof(s_mode_labels) / sizeof(s_mode_labels[0]);

    if (mode_index >= label_count)
    {
        return -1;
    }

    p_label = s_mode_labels[mode_index];
    if (p_label == NULL)
    {
        return -1;
    }

    (void)puts(p_label);
    return 0;
}

int main(void)
{
    if (print_mode_label(1U) != 0)
    {
        return 1;
    }

    return (print_mode_label(3U) == -1) ? 0 : 1;
}
```

The example checks the index before indexing the table and checks the selected pointer before use. The second check creates a deliberate policy for tables that may later contain optional entries. A table whose contract guarantees every entry is populated may document that fact, but the contract must remain true as the table evolves.

Ownership is separate from const qualification. A non-owning table merely borrows references; it must not free, overwrite, or outlive the objects it references. An owning table has a different lifetime and cleanup contract. This lesson uses non-owning references only.

## 4. Arrays of Structure Pointers

An array of complete structures stores every structure inline:

```c
device_t devices[3];
```

An array of structure pointers stores addresses instead:

```c
device_t *devices[3];
```

The first representation has one contiguous set of `device_t` objects and one lifetime for that array. The second has an array of three pointer objects and may designate three independent objects. It can select variants or share descriptors without copying them, at the cost of indirection and lifetime reasoning.

### 4.1 Complete standalone C99 example — a descriptor table

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct
{
    uint8_t channel_id;
    bool enabled;
} channel_descriptor_t;

static const channel_descriptor_t s_temperature_channel = { 1U, true };
static const channel_descriptor_t s_pressure_channel = { 2U, false };

static const channel_descriptor_t *const s_channels[] = {
    &s_temperature_channel,
    &s_pressure_channel,
    NULL
};

static bool get_enabled_channel_id(size_t index, uint8_t *p_channel_id)
{
    const channel_descriptor_t *p_channel;
    size_t channel_count = sizeof(s_channels) / sizeof(s_channels[0]);

    if ((p_channel_id == NULL) || (index >= channel_count))
    {
        return false;
    }

    p_channel = s_channels[index];
    if ((p_channel == NULL) || !p_channel->enabled)
    {
        return false;
    }

    *p_channel_id = p_channel->channel_id;
    return true;
}

int main(void)
{
    uint8_t channel_id = 0U;

    if (!get_enabled_channel_id(0U, &channel_id) || (channel_id != 1U))
    {
        return 1;
    }

    if (get_enabled_channel_id(1U, &channel_id) || get_enabled_channel_id(2U, &channel_id))
    {
        return 1;
    }

    (void)printf("selected channel: %u\n", (unsigned int)channel_id);
    return 0;
}
```

The table stores addresses, not descriptors. The descriptor objects have static storage duration, so their lifetime safely exceeds every use of this table. If a table held addresses of automatic objects that had already returned from their defining functions, its entries would be dangling even though the pointer values might still look plausible in a debugger.

The pointed-to type controls mutation. Here, `const channel_descriptor_t *const` prevents mutation through both the table's pointer entries and the selected pointer. If the design needs mutable descriptors, make the mutation and ownership policy explicit rather than removing `const` mechanically.

## 5. Double Pointers: Updating a Caller’s Pointer Deliberately

`T **pp_object` is a pointer to an object whose value is `T *`. It is useful when a function must update the caller's pointer, traverse or modify an array of pointers, or select an existing object through an output parameter. It does not imply dynamic allocation, and adding a pointer level does not solve ownership.

### 5.1 Complete standalone C99 example — selecting an existing device

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct
{
    uint8_t device_id;
} device_t;

static device_t s_primary_device = { 10U };
static device_t s_backup_device = { 20U };

static device_t *const s_devices[] = {
    &s_primary_device,
    &s_backup_device
};

static bool select_device(device_t **pp_device, size_t index)
{
    size_t device_count = sizeof(s_devices) / sizeof(s_devices[0]);

    if (pp_device == NULL)
    {
        return false;
    }

    if (index >= device_count)
    {
        return false;
    }

    if (s_devices[index] == NULL)
    {
        return false;
    }

    *pp_device = s_devices[index];
    return true;
}

int main(void)
{
    device_t *p_selected = NULL;

    if (!select_device(&p_selected, 1U) || (p_selected == NULL))
    {
        return 1;
    }

    (void)printf("selected device: %u\n", (unsigned int)p_selected->device_id);
    return 0;
}
```

The validation order matters. First validate `pp_device`, then validate the index and source entry, and only then assign `*pp_device`. This preserves the caller's output pointer on the failure paths shown above.

`T **` is not a pointer to a two-dimensional array. For example, `int **` cannot replace `int matrix[ROWS][COLS]`, whose rows have a fixed column count and whose decayed type is `int (*)[COLS]`. The declarations describe different memory layouts and different pointer arithmetic.

**Common mistake:** using a double pointer merely because a function processes a two-dimensional array. Use a pointer-to-array parameter when the array's column count is part of the type; use a double pointer only when the object really contains pointers to pointers or when the API must update a pointer object.

## 6. Reading Complex Declarations Systematically

Read a declaration from the identifier outward. First find the declared name, then inspect the nearest binding operator. `[]` and `()` bind more tightly than unary `*`, so parentheses change the result. Translate each layer into plain English before considering how to use it.

| Declaration | Start at the name and read outward | Plain-English type |
| --- | --- | --- |
| `int *p;` | `p` → `*` → `int` | `p` is a pointer to `int`. |
| `int values[4];` | `values` → `[4]` → `int` | `values` is an array of four `int`. |
| `int *p_values[4];` | `p_values` → `[4]` → `*` → `int` | `p_values` is an array of four pointers to `int`. |
| `int (*p_values)[4];` | `p_values` → `*` → `[4]` → `int` | `p_values` is a pointer to an array of four `int`. |
| `void (*handler)(uint32_t);` | `handler` → `*` → `(uint32_t)` → `void` | `handler` is a pointer to a function taking `uint32_t` and returning `void`. |
| `void (*handlers[4])(uint32_t);` | `handlers` → `[4]` → `*` → `(uint32_t)` → `void` | `handlers` is an array of four pointers to compatible functions. |

The contrast between `int *p_values[4]` and `int (*p_values)[4]` is especially important. The first is a table of four pointer objects. The second is one pointer that designates a complete four-element row. Parentheses are not cosmetic; they define the representation.

## 7. Function Pointers: Functions as Selectable Operations

A function has a function type. In many expressions, a function designator converts to a pointer to that function, and a function pointer value can be used to call a compatible function. Both `handler(event_id)` and `(*handler)(event_id)` perform an indirect call when `handler` is a valid function pointer; the first is usually easier to read.

Compatibility is mandatory. Return type and parameter types must match the table or API's declared function type. A cast can silence a diagnostic but cannot make incompatible calling conventions, parameter interpretation, or return handling safe. Do not convert function pointers to `void *`, and do not assume function pointers and object pointers are interchangeable in ISO C.

### 7.1 Complete standalone C99 example — direct declaration, then a typedef

```c
#include <stdint.h>
#include <stdio.h>

static void report_event(uint32_t event_id)
{
    (void)printf("event %u\n", (unsigned int)event_id);
}

typedef void (*event_handler_t)(uint32_t event_id);

static int invoke_event(event_handler_t handler, uint32_t event_id)
{
    if (handler == NULL)
    {
        return -1;
    }

    handler(event_id);
    return 0;
}

int main(void)
{
    void (*direct_handler)(uint32_t) = report_event;
    event_handler_t named_handler = direct_handler;

    return (invoke_event(named_handler, 42U) == 0) ? 0 : 1;
}
```

The direct declaration expresses the type exactly but is visually dense. The typedef names the contract once and makes later declarations, tables, and reviews easier to read. A useful name still needs documentation: `event_handler_t` does not say whether it may be null, who owns associated context, or which execution context may call it. Those callback-lifecycle details belong to M03-L02.

## 8. Function-Pointer Typedefs as Interface Contracts

Consider this API shape:

```c
typedef status_t (*operation_fn_t)(const request_t *p_request);
```

This is an intentionally incomplete declaration fragment: `status_t` and `request_t` are application types defined elsewhere. It illustrates that each implementation placed in an `operation_fn_t` table must return `status_t` and accept a pointer to a const-qualified `request_t` with the same parameter contract.

A typedef improves interface readability, signature consistency, table declarations, compiler diagnostics, and API review. It is not a generic callback framework and does not remove the need to document parameter meaning, nullability, lifetime, ownership, or execution context. Avoid vague names such as `callback_t` when a precise name can describe the operation.

## 9. Arrays of Function Pointers

`handler_t handlers[N]` is an array of callable entries sharing one compatible function-pointer type. It is useful when commands, operations, or board variants are represented by a bounded, explicit index set.

### 9.1 Complete standalone C99 example — a checked operation table

```c
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    ACTION_ENABLE = 0,
    ACTION_DISABLE,
    ACTION_COUNT
} action_id_t;

typedef int (*action_handler_t)(uint32_t argument);

static int enable_action(uint32_t argument)
{
    return (argument <= 100U) ? 0 : -1;
}

static int disable_action(uint32_t argument)
{
    return (argument == 0U) ? 0 : -1;
}

static action_handler_t const s_action_handlers[ACTION_COUNT] = {
    [ACTION_ENABLE] = enable_action,
    [ACTION_DISABLE] = disable_action
};

static int dispatch_action(action_id_t action, uint32_t argument)
{
    size_t index = (size_t)action;
    size_t handler_count = sizeof(s_action_handlers) / sizeof(s_action_handlers[0]);
    action_handler_t handler;

    if (index >= handler_count)
    {
        return -1;
    }

    handler = s_action_handlers[index];
    if (handler == NULL)
    {
        return -1;
    }

    return handler(argument);
}

int main(void)
{
    if (dispatch_action(ACTION_ENABLE, 50U) != 0)
    {
        return 1;
    }

    if (dispatch_action(ACTION_DISABLE, 1U) != -1)
    {
        return 1;
    }

    return (dispatch_action((action_id_t)ACTION_COUNT, 0U) == -1) ? 0 : 1;
}
```

The table-length expression `sizeof(table) / sizeof(table[0])` is correct only where `table` is still an actual array, as it is inside `dispatch_action`'s translation unit. It is not a way to recover an array length from an adjusted pointer parameter.

An enum is safe as a direct table index only when the design maintains a contiguous, zero-based mapping. Sparse or externally supplied values need an explicit mapping or a search, not blind indexing. A valid range does not prove a table entry is non-null, so validate both before the indirect call.

## 10. Safe Dispatch Tables

The runtime flow for a dispatch table is simple and should remain visible in the code:

```text
input/index
    → validate range
    → obtain table entry
    → validate function pointer
    → invoke a compatible handler
    → process its result
```

The table must remain consistent with the index domain. Adding a new enum member but forgetting its handler, using a sparse enum as a direct index, or changing one handler signature independently turns a readable design into a runtime fault or a misleading compile-time workaround.

An explicit function-pointer table can improve architecture: it puts the supported operations and their common interface in one reviewable location, and it can make extension deliberate. It is not universally faster than `switch`. A compiler may translate a dense `switch` into a jump table, so performance must be measured on the selected compiler, options, target, and workload rather than inferred from the source form.

The context passed to a handler also needs an ownership and lifetime contract. This lesson uses arguments that are consumed during the call. Long-lived callbacks and stored contexts are introduced later, in M03-L02.

## 11. Controlled Custom Sections

Session 05 introduces a useful but non-ISO-C toolchain feature: requesting that an object be emitted into a named input section. In GCC and Clang-family builds, a declaration can use the GNU-style `section` attribute.

**Toolchain-specific fragment — not a standalone C99 example:**

```c
static action_handler_t const s_example_handlers[]
    __attribute__((section(".example_dispatch"))) = {
        enable_action,
        disable_action
    };
```

`__attribute__((section(".example_dispatch")))` is a GCC/Clang-family extension, not an ISO C99 feature. It requests a named input section for that object. The linker script subsequently maps input sections into output sections, and the target memory map determines whether the final image places an output section in Flash, RAM, or another region.

`const` expresses a C type constraint; it does not guarantee physical Flash placement. Likewise, a section attribute does not by itself prove final placement. Optimisation, link-time optimisation, and section garbage collection can remove unused objects or change the final image. A portable build needs an explicit fallback or a documented target-specific configuration; the exact solution belongs to the build and target, not to ISO C.

## 12. Inspecting Sections and Symbols

Binary inspection provides evidence about one selected build. It can confirm what the compiler and linker produced, but a host executable is not proof of placement on an embedded target with a different linker script or memory map.

| Command | Primary question answered |
| --- | --- |
| `readelf -S firmware.elf` | Which ELF section headers exist? |
| `readelf -s firmware.elf` | Which symbols exist and what metadata is recorded? |
| `objdump -h firmware.elf` | What section-header summary does the selected object expose? |
| `objdump -t firmware.elf` | What symbol-table view does the selected object expose? |
| `nm -S firmware.elf` | Which symbols and sizes are convenient to review as supplementary evidence? |

Use a short, repeatable workflow:

```text
compile the selected target build
    → inspect section headers with readelf -S or objdump -h
    → inspect symbols with readelf -s or objdump -t
    → use nm -S as supplementary symbol evidence
    → confirm final placement against the target linker script and memory map
```

`nm` is useful for quickly listing symbols but is not the best sole proof of section layout. Section-header and symbol-table output answer different questions, and neither substitutes for the target's linker configuration.

## 13. Common Failures and Debugging

| Failure | Symptom | Root cause | Correct engineering response |
| --- | --- | --- | --- |
| Treating an array as a pointer object | Wrong size or an invalid assignment attempt. | Array conversion was confused with object identity. | Keep the array extent where needed; pass pointer plus count across an interface. |
| Using `sizeof` on an array parameter | A small pointer size appears instead of element storage. | The parameter was adjusted to a pointer. | Pass the count or encode it in a separately documented interface contract. |
| Indexing a pointer table unchecked | Crash, wrong message, or unrelated data. | External or stale index was used before range validation. | Validate the index before forming the table access. |
| Keeping a dangling table entry | Intermittent corruption after a function returns. | The table references an object whose lifetime ended. | Reference static, caller-owned, or otherwise documented live objects only. |
| Confusing `T **` with `T (*)[N]` | Incorrect layout, invalid access, or type diagnostics. | Pointer levels were substituted for an array type. | Model the real representation; use a pointer-to-array for fixed-width rows. |
| Failing to validate `pp_object` | Fault on the output assignment. | The output-pointer object itself was not checked. | Validate `pp_object` before any `*pp_object` access. |
| Assigning incompatible functions to one table | Diagnostic suppression or undefined behaviour at the call. | Handlers do not share one compatible function type. | Use one precise typedef and correct the function declarations; do not cast to hide a mismatch. |
| Calling a null function pointer | Fault at indirect call. | An optional or incomplete table entry was invoked. | Define a null-entry policy and validate the entry before calling. |
| Enum and table-count mismatch | Valid-looking command invokes no intended operation. | The enum changed separately from the table. | Review the enum, count sentinel, table initialisers, and bounds logic together. |
| Declaring dispatch tables always faster | Premature design choice or worse target code. | Source-level form was mistaken for generated code. | Choose the architecture for clarity; profile the actual target build when performance matters. |
| Claiming `const` or `section` guarantees Flash | Incorrect target-memory claim. | ISO type qualifiers and compiler attributes were confused with linker placement. | Label the extension and verify the final target image against the linker map. |
| Using `nm` alone as placement proof | Incomplete evidence during review. | Symbol listing was confused with full section-layout evidence. | Combine section-header inspection, symbol inspection, and target linker/map evidence. |

When debugging an indirect call, start with the concrete evidence: the received index, the table length, the selected entry, the exact handler type, and the lifetime of any referenced context. Do not begin by adding casts or by assuming the function pointer itself is defective.

## 14. Embedded and Linux Relevance

The same bounded-table ideas appear in error or status message lookup, device descriptor catalogues, protocol-operation selection, bootloader command dispatch, driver-operation tables, and board-specific implementation selection. On embedded targets, controlled sections can also support build-time metadata placement when the compiler, linker, and memory map are reviewed together.

Linux user-space programs use the same C type and lifetime rules for tables of data and operations. This lesson intentionally does not analyse Linux kernel `file_operations`, real memory-mapped I/O, interrupt callbacks, or threaded dispatch; those subjects require additional platform and execution-context contracts.

## 15. Exercise Preparation Without Solution Leakage

For **S05-E01**, this lesson provides the prerequisites to reason about an array of const string pointers, a checked index-to-string lookup, and defined invalid-input behaviour. It does not provide the exercise's finished table, function names, or required output.

For **S05-E02**, this lesson provides the prerequisites to define one compatible function-pointer typedef, build and validate an indexed table, request a named input section, and inspect the resulting selected binary. It does not provide the completed dispatcher, its exact menu table, expected output, Makefile, or solution layout.

## 16. Embedded Engineering Checklist

Before approving a pointer-based table or dispatch design, verify:

1. The representation matches the requirement: complete objects, pointers to objects, pointer to array, or function pointers.
2. Every index is validated before table access, and sparse values use an explicit mapping.
3. Every table entry has a documented null policy and every indirect function call checks nullable entries.
4. Referenced objects and contexts remain alive for the full time the table can use them.
5. Function declarations are compatible with the one documented typedef; no function-pointer cast hides a mismatch.
6. Table-size calculations use `sizeof` only while the expression still names an actual array.
7. A custom section is labelled as a compiler extension and final placement is verified for the intended target build.
8. A performance claim is based on target measurements, not an assumption about `switch` or table code generation.

## 17. Key Takeaways

- Arrays and pointers interact closely but are different object types; array conversion is context-dependent.
- An array of pointers stores references, so bounds, pointee lifetime, null policy, and ownership must be explicit.
- `T **` updates or traverses pointer objects; it is neither a two-dimensional array nor automatic dynamic allocation.
- Parentheses in declarations define the representation. Read from the name outward and account for binding.
- Function-pointer tables require compatible signatures, checked indices, non-null entries, and a consistent index-to-table mapping.
- GCC/Clang `section` attributes request input sections only. The linker script and target memory map determine final placement.
- Section and symbol tools provide evidence for a particular build; they do not replace target linker and memory-map review.

## 18. Further Reading

- [WG14 N1256 — C99 technical corrigenda draft](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf), especially expressions, declarators, array parameters, and function calls.
- [GCC Attributes documentation](https://gcc.gnu.org/onlinedocs/gcc/Attributes.html) and [GCC Common Variable Attributes](https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html) for the GCC-specific `section` attribute.
- [GNU Binutils `readelf` documentation](https://sourceware.org/binutils/docs/binutils/readelf.html) and [GNU Binutils `objdump` documentation](https://sourceware.org/binutils/docs/binutils/objdump.html) for the selected-build inspection commands.
- [SEI CERT C EXP37-C](https://wiki.sei.cmu.edu/confluence/display/c/EXP37-C.+Call+functions+with+the+correct+number+and+type+of+arguments) for the risk of calling functions through incompatible types.
