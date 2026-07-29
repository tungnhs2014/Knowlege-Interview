# M03 Interview — Advanced Pointers & Embedded Design Patterns

> **Status:** APPROVED.

These questions prioritise the real Session 05 and Session 06 practical scenarios supplied by DevLinux, then assess the approved M03 lessons, followed by focused Embedded C interview extensions within this module's scope. No separate M03 user interview-question pack is currently present in the repository. This is assessment material, not a replacement for either lesson or the approved exercise solutions.

## M03-Q01 — Array Object Versus Pointer Object

### Question

Explain the difference between an array object and a pointer object in this declaration context:

```c
uint16_t samples[4];
uint16_t *p_samples = samples;
```

Why do these objects often work together without becoming the same type of object?

### Expected Answer

`samples` is an array object containing four contiguous `uint16_t` elements; `p_samples` is a separate pointer object whose value initially designates `samples[0]`. In many expressions, `samples` converts to a pointer to its first element, which permits the initialization. The conversion does not make the array assignable or change its array type. Their object sizes, assignment rules, and storage are distinct.

For embedded buffers and Linux user-space APIs alike, an interface normally receives a pointer plus an explicit extent. The pointer is not evidence that the callee can recover the caller's array length.

### What the Interviewer Is Testing

- Whether the candidate distinguishes an object from a commonly useful expression conversion.
- Whether they can describe a pointer-plus-count contract without claiming that an array is “just a pointer.”

### Common Wrong Answers

- “The array variable stores a pointer to its first element.”
- “The two declarations have the same type because their addresses may compare equal after conversion.”
- “A pointer parameter automatically knows the original array capacity.”

### Follow-up Questions

- Can `samples = p_samples;` be a valid assignment? Why not?
- Which API information must accompany `p_samples` if the callee needs to process all four elements?

## M03-Q02 — Array-to-Pointer Conversion and `sizeof`

### Question

Classify this fragment and explain why it cannot calculate the caller's array length:

```c
static size_t byte_count(const uint8_t bytes[8])
{
    return sizeof(bytes);
}
```

Name the important exceptions to array-to-pointer conversion. Also explain the status of a one-past endpoint formed from a valid array range.

### Expected Answer

The fragment is valid C99, but the array parameter is adjusted to `const uint8_t *`; `sizeof(bytes)` therefore yields the size of that pointer object, not eight bytes or the caller's array extent. A correct interface carries the count separately, unless the expression that uses `sizeof(array)` still names a complete array object in the same scope.

Although the fragment is valid ISO C99, GCC and Clang commonly diagnose `sizeof` applied to an array parameter because the expression operates on the adjusted pointer type. Under this repository's strict `-Werror` build policy, such a diagnostic causes the build to fail.

Array-to-pointer conversion does not occur when an array is the operand of `sizeof`, unary `&`, or a string literal used to initialize an array. For an array of `count` elements, `array + count` may be formed as a one-past endpoint in the permitted array domain and compared as an endpoint; dereferencing it is undefined behaviour.

### What the Interviewer Is Testing

- Knowledge of parameter adjustment and the limits of the common `sizeof(array) / sizeof(array[0])` idiom.
- Whether the candidate separates valid endpoint formation from invalid dereference.

### Common Wrong Answers

- “The `[8]` in a parameter preserves the length at runtime.”
- “`sizeof` always reports the original array size.”
- “A one-past pointer is always invalid to form.”
- “A one-past pointer is the final element and may be read.”

### Follow-up Questions

- Where is `sizeof(local_array) / sizeof(local_array[0])` valid?
- Why is a half-open range `[begin, end)` useful for an empty buffer?

## M03-Q03 — Array of Pointers, Pointer to Array, and Pointer to Pointer

### Question

Read these declarations from the identifier outward. What representation and pointer arithmetic does each describe?

```c
int *p_items[3];
int (*p_row)[3];
int **pp_item;
```

### Expected Answer

`p_items` is an array of three `int *` objects. `p_row` is one pointer to an array of three `int`, so advancing it moves by an entire three-`int` row where pointer arithmetic is otherwise valid. `pp_item` is a pointer to an `int *` object; it can be used to update a caller-owned pointer or traverse an actual array of pointers.

All three declarations are valid C99 but model different layouts. In particular, `int **` is not a substitute for a two-dimensional `int rows[R][3]` array, whose decayed row type is `int (*)[3]`.

### What the Interviewer Is Testing

- Ability to read parentheses, `[]`, and `*` as a representation contract.
- Whether a candidate avoids substituting pointer levels for a fixed-column array type.

### Common Wrong Answers

- “All three point to a two-dimensional array.”
- “`p_items` stores three `int` values inline.”
- “A double pointer always implies dynamic allocation.”

### Follow-up Questions

- Which declaration would match a matrix with a compile-time column count of three?
- What lifetime rule applies to objects referenced by an array of pointers?

## M03-Q04 — Double Pointers and Caller-Owned Pointer Updates

### Question

When is a `T **` parameter appropriate? Describe a narrow API that selects an existing device for the caller, including validation order and ownership implications.

### Expected Answer

Use `T **` when the callee must modify a pointer object owned by the caller, for example to select an existing descriptor, update a list head, or return an allocated object under a documented allocation contract. For selection, validate the output-pointer parameter before dereferencing it, then validate the index and selected source object before assigning `*pp_selected`.

Selecting an existing device normally transfers no ownership: the caller receives a borrowed pointer and must not free it. A failed selection should have a documented output policy; preserving the previous valid output is often useful but must be part of the API contract rather than assumed.

### What the Interviewer Is Testing

- Whether the candidate can explain why passing a pointer by value cannot update the caller's pointer.
- Whether they validate the outer pointer before the output assignment and keep lifetime distinct from pointer syntax.

### Common Wrong Answers

- “Use `T **` for every two-dimensional array.”
- “Once the output pointer is non-null, the selected object needs no validation.”
- “A returned pointer is automatically owned by the caller.”

### Follow-up Questions

- How would an API differ if it allocated a new device rather than selected a static descriptor?
- Why can assigning a temporary pointer before all validation create an error-path contract problem?

## M03-Q05 — Const Pointer Declarations

### Question

Explain what may be changed through each declaration:

```c
const char *p_text = "text";
char writable[] = "buffer";
char *const p_buffer = writable;
const char *const p_label = "label";
```

### Expected Answer

With `const char *p_text`, the pointer value may change but characters may not be modified through `p_text`. With `char *const p_buffer`, the pointer value cannot change after initialization, but a modifiable pointed-to character object may be changed. With `const char *const p_label`, neither the pointer value nor characters may be changed through that name.

Qualification controls permitted access through the qualified expression; it is not a statement about ownership, lifetime, or physical memory placement. A `const` object may commonly be emitted in read-only program storage, but ISO C does not guarantee a `.rodata` or Flash location.

### What the Interviewer Is Testing

- Correct right-to-left reading of pointer qualification.
- Separation of C type semantics from linker placement and ownership claims.

### Common Wrong Answers

- “`const char *` means the pointer itself is constant.”
- “`const` proves that the object is in Flash.”
- “A const-qualified reference owns the character storage.”

### Follow-up Questions

- What is the risk of casting away `const` and modifying an object defined with a const-qualified type?
- Why is `const char *const table[]` useful for a table of immutable labels?

## M03-Q06 — Function-Pointer Declarations and Typedefs

### Question

Read this interface and explain the benefit of the typedef:

```c
typedef int (*event_handler_t)(uint8_t event_id);

event_handler_t handlers[3];
```

What must be true of every function placed in `handlers`? Do parameter names affect compatibility?

### Expected Answer

`event_handler_t` names a pointer to a function that accepts one `uint8_t` argument and returns `int`. `handlers` is an array of three such function pointers. Every tabled function must have a compatible return type and parameter list, and the caller must meet the documented argument contract.

The typedef makes the common contract visible in declarations, tables, and diagnostics. Parameter names are documentation only; they do not affect function-type compatibility. The typedef still does not define null policy, callback lifetime, ownership of associated data, or execution context.

### What the Interviewer Is Testing

- Ability to parse function-pointer arrays accurately.
- Understanding that a typedef names a type contract rather than repairing an incompatible implementation.

### Common Wrong Answers

- “`handlers` is a function returning an array.”
- “A parameter called `event` is incompatible with one called `event_id`.”
- “A typedef makes every function with a similar purpose safe to store.”

### Follow-up Questions

- Is `handler(argument)` different from `(*handler)(argument)` when `handler` is a valid function pointer?
- Which checks belong before an indirect call if table entries may be optional?

## M03-Q07 — Incompatible Function-Pointer Casts

### Question

Review this fragment. Is the conversion a valid repair for the callback contract? Classify the indirect call if it occurs.

```c
typedef void (*alarm_t)(uint32_t tick);

static void legacy_alarm(uint16_t code);

alarm_t alarm = (alarm_t)legacy_alarm;
```

### Expected Answer

The explicit conversion does not repair the signature mismatch. C permits conversion between function-pointer types and conversion back to the original type for comparison, but that does not establish that a function is compatible with `alarm_t`. If `alarm` is called through this incompatible type, the behaviour is undefined.

The correct remediation is to make the registered function match the declared callback type, or to define a separate compatible adapter with a documented conversion/range contract. Do not use a cast merely to suppress a diagnostic.

### What the Interviewer Is Testing

- Whether the candidate recognizes that compilation or a cast is not a compatibility proof.
- Accurate use of the term undefined behaviour for an indirect call through an incompatible function type.

### Common Wrong Answers

- “The cast makes the call safe because both parameters are integers.”
- “It is implementation-defined but safe on embedded targets.”
- “Changing only the parameter name would fix the mismatch.”

### Follow-up Questions

- Which parts of a function declaration determine compatibility?
- When is a separately written adapter preferable to changing the provider's public callback type?

## M03-Q08 — Callback Registration Contract

### Question

Review the contract for a software-timer callback registration API. What must it specify beyond the callback typedef?

### Expected Answer

The contract must define whether a null callback is rejected, ignored, or unregisters a prior callback; whether a second registration replaces or is rejected while an event is active; the exact event or condition that triggers invocation; and how registration or invocation failure is reported. It must also state the lifetime of the registered function and any associated context/data, including whether deregistration is required before that context is released.

For the Session 06 timer scenario, active registration is rejected without overwriting the existing callback, the callback is invoked once at expiry if non-null, and reset clears the active state for later reuse. Those are synchronous software-timer rules, not an ISR or threading contract.

### What the Interviewer Is Testing

- Whether the candidate treats callback registration as a lifecycle and failure contract, not just a function-pointer assignment.
- Awareness of replacement, null, lifetime, and failure-reporting policies.

### Common Wrong Answers

- “Checking for null at the call site is the entire callback contract.”
- “A later registration can silently replace an active callback.”
- “A valid callback function automatically makes its saved context valid.”

### Follow-up Questions

- What evidence proves that an attempted replacement did not overwrite the active timer callback?
- How would a context pointer change the lifetime and deregistration requirements?

## M03-Q09 — Callback Execution Context

### Question

Does using a callback mean that it runs in an ISR or another thread? Explain what an engineer must know before making timing, synchronization, or API-safety claims.

### Expected Answer

No. A callback is a compatible function invoked according to the provider's contract; it does not itself specify ISR, worker-thread, signal, or synchronous execution. The provider or API must state where and when invocation occurs. The approved M03 timer model invokes callbacks synchronously from its tick function.

Before making any timing or safety claim, identify the caller/provider execution context, registration and invocation lifetime, and the specific platform rules. M03 intentionally does not establish a concurrency or ISR implementation model.

### What the Interviewer Is Testing

- Whether the candidate avoids inferring execution context from the word “callback.”
- Ability to keep the interface contract separate from later concurrency or interrupt rules.

### Common Wrong Answers

- “Every embedded callback runs in an interrupt.”
- “Every callback is asynchronous.”
- “A callback registration API is automatically thread-safe.”

### Follow-up Questions

- How would you document a synchronous callback invocation in a driver API?
- Which additional specification would be required before an ISR-specific review?

## M03-Q10 — Dispatch Table Versus `switch`

### Question

Review this bounded operation-table shape. When might it improve architecture, and why is it not universally faster than a `switch`?

```c
typedef void (*operation_t)(uint8_t input);

static void run_start(uint8_t input);
static void run_stop(uint8_t input);

static operation_t const s_operations[] = {
    run_start,
    run_stop
};
```

### Expected Answer

The fragment is a valid C99 array of compatible function pointers. A dispatch table can centralize a bounded operation set and one common interface, which can make extension and review clearer. Its dispatcher must validate the requested index before table access and validate a nullable selected handler before calling it.

A small `switch` is often the clearest authoritative baseline. It is not inherently slow: a compiler may generate efficient branches or a jump table for a suitable dense switch. Choose the design for clarity and contract fit, then measure the selected compiler, options, target, and workload if performance matters.

### What the Interviewer Is Testing

- Whether a candidate can state architectural trade-offs without a universal performance claim.
- Whether they retain range and null-handler validation in a table design.

### Common Wrong Answers

- “A function-pointer table is always O(1) and therefore always faster.”
- “A `switch` cannot compile to a jump table.”
- “A valid index proves the corresponding handler is non-null.”

### Follow-up Questions

- How does a sparse command or state value change the table design?
- What target-build evidence would support a performance conclusion?

## M03-Q11 — Custom Sections, `const`, and Flash Claims

### Question

Classify this fragment, then describe what it proves and what it does not prove:

```c
typedef void (*ui_handler_t)(uint8_t page_id);
static void draw_menu(uint8_t page_id);

static ui_handler_t const s_menu_handlers[]
    __attribute__((section(".my_dispatch_table"))) = { draw_menu };
```

Which evidence should an engineer collect for a selected build?

### Expected Answer

The declaration is valid for a GCC/Clang-family build that supports the GNU-style `section` attribute; it is a compiler extension, not portable ISO C99. `const` constrains modification through the qualified object, while the attribute requests a named input section. Neither source form alone guarantees final Flash placement.

The shown code is only a declaration fragment. A complete linked program must also provide the definition of `draw_menu()` if the symbol is retained and referenced by the final build.

For the selected target build, inspect section headers with `readelf -S` or `objdump -h`, inspect symbol information with `readelf -s` or `objdump -t`, and confirm final placement through the target linker configuration and memory map. `nm -S` is useful supplementary symbol evidence, not sole placement proof.

### What the Interviewer Is Testing

- Separation of ISO C qualification, compiler extension, ELF/object evidence, and target linker placement.
- Avoidance of unverified RAM/Flash claims from C source alone.

### Common Wrong Answers

- “`const` always places a table in Flash.”
- “The section attribute directly selects the final memory region.”
- “`nm` alone proves final physical placement.”

### Follow-up Questions

- Why can a host executable not prove placement for an embedded target using another linker script?
- What might optimization or section garbage collection do to an unused table?

## M03-Q12 — Safe String-Command Dispatcher Review

### Question

Design or review a bounded string-command dispatcher. What checks and policies must be visible before dispatching an incoming command?

```c
static const command_entry_t s_commands[] = {
    /* command name and compatible handler entries */
};

size_t command_count = sizeof(s_commands) / sizeof(s_commands[0]);
```

### Expected Answer

The size calculation is valid only here, while `s_commands` is an actual array. First validate that the received pointer is non-null; the caller must also meet the separate contract that it points to a readable, null-terminated C string. Search only within the calculated table count and use `strcmp` for exact matching. Before an indirect call, confirm the selected handler is non-null and compatible with the table type.

The table needs a duplicate-name policy, preferably review-time rejection for a fixed table or explicit startup validation for generated data. Unknown input needs an outcome distinct from a known command whose handler later fails. `static const` does not prove a Flash location.

### What the Interviewer Is Testing

- Exact string matching, pointer validity, actual-array size calculation, and function-pointer validation.
- Whether the candidate separates unknown-command handling from handler-level operational failure.

### Common Wrong Answers

- “A null check proves the input is a valid C string.”
- “Prefix comparison is equivalent to an exact command match.”
- “Duplicate command names are harmless because the first one wins.”
- “The same `sizeof` expression works after the table is passed as a function parameter.”

### Follow-up Questions

- What should a dispatcher do if a fixed table entry has a null handler?
- How would you test that `SYNC_NOW` is not accepted as `SYNC`?

## M03-Q13 — FSM Fundamentals and Suitable Use

### Question

Define current state, input/event, action, transition, and next state in an FSM. When is an FSM preferable to scattered conditionals?

### Expected Answer

The current state is the stored mode that gives an input its meaning. An input or event is the new condition being handled. A handler performs an action and selects a next state; a transition is the change from current to next state. Keeping action and transition conceptually separate makes the behaviour reviewable.

An FSM is useful when the same input has different valid meanings in different modes, when transitions must be explicit, or when retry and recovery paths need a bounded, reviewable model. For a tiny one-shot decision, scattered conditionals may be simpler; an FSM should not be introduced merely for pattern terminology.

### What the Interviewer Is Testing

- Whether the candidate can explain stateful behaviour rather than only draw arrows.
- Ability to choose an FSM for a real state-dependent contract instead of using it as a default abstraction.

### Common Wrong Answers

- “A state is the same thing as an input.”
- “An action must always change state.”
- “Every group of `if` statements should become an FSM.”

### Follow-up Questions

- In a connection controller, which is an action and which is a transition: logging a failed attempt and entering an error state?
- Why is a small switch-based FSM a useful starting point?

## M03-Q14 — Safe Function-Pointer FSM Review

### Question

Review this dispatcher fragment assuming `p_state` itself is non-null. What failure can occur, and what validation/recovery sequence is required before an indirect call?

```c
state_handler_t handler = s_handlers[(size_t)*p_state];
handler(input, p_state);
```

### Expected Answer

If `*p_state` is invalid, casting it to `size_t` does not make it valid and the table access can be out of bounds, which is undefined behaviour. If the selected entry is null, the indirect call is also undefined behaviour. The safe sequence is: validate the state value against a contiguous state domain and count sentinel, or use explicit mapping/search for sparse values; then index; then validate the handler; then invoke the compatible handler.

An invalid non-null state needs deterministic recovery before table indexing. The recovery state and failure result are product contracts; the approved WiFi exercise recovers to `WIFI_INIT` and returns `0xFFU`. When replacing a switch baseline, compare state/action behaviour against the authoritative baseline rather than relying only on output that may contain source contradictions.

### What the Interviewer Is Testing

- State-range validation before conversion/indexing, null-handler checks, and exact handler compatibility.
- Understanding of contiguous enum assumptions, sparse mapping, and behaviour-preserving refactoring.

### Common Wrong Answers

- “An enum cannot hold an invalid value.”
- “Casting an invalid state to `size_t` makes table indexing safe.”
- “A count sentinel alone protects an unchecked table access.”
- “Matching one expected printout proves FSM equivalence.”

### Follow-up Questions

- How would you dispatch a sparse state domain such as values `0`, `10`, and `20`?
- What must happen when the selected handler is null?

## M03-Q15 — Retry-State Ownership in an FSM

### Question

Explain where retry state belongs in a connection FSM and define its reset policy. Why is a static local counter owned only by the CONNECTING handler problematic when another state must reset it?

### Expected Answer

The retry counter belongs to the FSM or module state because it affects transitions across several handlers. Its owner must support deliberate resets on successful connection, on a fresh entry to the retry lifecycle, and when the configured limit drives deterministic recovery. The retry limit itself must be explicit; valid retry processing that remains in CONNECTING is not necessarily a dispatcher failure.

A static local inside only the CONNECTING handler has hidden ownership: INIT, CONNECTED, or ERROR cannot coherently reset it without an additional controlled interface. Use small module-private state with a reset helper, or a small explicit context passed under a documented lifetime contract. Private static state can be appropriate for one simple instance, but it is not inherently thread-safe and does not replace a lifecycle policy.

### What the Interviewer Is Testing

- Whether the candidate treats retry count as state with ownership and lifecycle, not as an incidental hidden variable.
- Ability to distinguish normal retry outcomes from unsafe-dispatch failures.

### Common Wrong Answers

- “A retry counter should never reset until reboot.”
- “A static local is accessible to every state handler in the module.”
- “Remaining in CONNECTING must return the dispatcher's error status.”
- “Module-private static state is automatically thread-safe.”

### Follow-up Questions

- Which trace would prove that a link drop begins a fresh retry sequence?
- When would a small explicit FSM context be preferable to module-private state?
