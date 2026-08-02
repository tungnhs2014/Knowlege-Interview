# M01-L02 — Memory Layout & Failure Analysis

> **Status:** `APPROVED`
>
> **Gate:** `LESSON_2_AUTHORING`
>
> **Language baseline:** ISO C99

## 1. Purpose, Scope, and the Three-Layer Memory Model

Reliable memory reasoning starts by separating three different layers that are often mixed together:

```text
ISO C object semantics
→ object-file / executable representation
→ physical target placement and runtime behavior
```

### Layer 1 — ISO C object semantics

ISO C defines concepts such as:

- object type and qualification;
- scope, linkage, storage duration, and lifetime;
- initialization requirements;
- valid pointer use;
- dynamic-allocation contracts;
- defined, implementation-defined, unspecified, and undefined behavior.

ISO C does **not** require sections named `.text`, `.rodata`, `.data`, or `.bss`. It does not require a downward-growing stack, an ELF file, a particular heap implementation, or a specific Flash/RAM map. [ISO C99 §§6.2.4, 6.7.8]

### Layer 2 — Object-file and executable representation

A compiler, assembler, linker, object format, and build configuration commonly represent code and objects using sections and symbols. GNU/ELF environments often use names such as:

```text
.text
.rodata
.data
.bss
```

These names are useful engineering conventions, but they describe one build artifact rather than a universal property of C.

### Layer 3 — Physical placement and runtime behavior

A linker script, loader, startup runtime, operating system, memory map, and target hardware decide where image content is stored and where objects exist while the program runs.

For example:

- code may execute directly from non-volatile memory;
- code may be copied to RAM before execution;
- writable initialized objects may have load bytes in Flash and execution storage in RAM;
- a Linux loader may map executable pages and writable pages into virtual memory;
- a bootloader or runtime may relocate sections.

The same C declaration can therefore produce different section names and different physical placement under another compiler, linker, target, or optimization level.

```c
#include <stdint.h>

static const uint32_t k_protocol_magic = UINT32_C(0x4D303132);
static uint32_t g_retry_limit = 3U;
static uint32_t g_error_count;

void process_packet(void)
{
    uint32_t local_status = 0U;

    (void)local_status;
}
```

At the C-language layer:

- `k_protocol_magic` is a `const`-qualified object with static storage duration;
- `g_retry_limit` and `g_error_count` have static storage duration;
- `local_status` has automatic storage duration.

A conventional build may represent them in `.rodata`, `.data`, `.bss`, and call-related storage. That result must be inspected; it must not be assumed from the source alone.

This lesson owns:

- common executable-section interpretation;
- startup initialization;
- Flash/image and RAM reasoning;
- stack and heap failure analysis;
- GNU Binutils evidence;
- basic GDB investigation;
- optimization-sensitive observations.

It prepares learners for the Session 02 Memory Segment Analyzer and Stack Depth Monitor exercises without reproducing their complete specifications or solutions.

**Must remember:** first identify the C-language meaning, then inspect the generated artifact, and only then reason about physical target placement.

## 2. Common Embedded Startup Model

Objects with static storage duration must have their required initial values before ordinary application code relies on them. ISO C defines the required initialized state; the selected runtime and target decide how that state is established.

A common bare-metal startup flow is:

```text
reset
→ establish processor mode and initial stack state
→ perform required target initialization
→ copy initialized writable data to its runtime location
→ establish zero-initialized storage
→ initialize runtime services as required
→ call main()
```

This is a **common implementation model**, not an ISO C-mandated sequence. Exact ordering and mechanisms depend on:

- processor architecture;
- startup code;
- compiler runtime;
- linker configuration;
- bootloader;
- RTOS;
- operating environment.

Consider:

```c
#include <stdint.h>

static uint32_t g_sample_period_ms = 100U;
static uint32_t g_samples_received;
```

At the C-language layer:

- `g_sample_period_ms` must hold `100U` before normal program execution uses it;
- `g_samples_received` is zero-initialized.

A conventional embedded runtime may:

1. store the initial bytes for `g_sample_period_ms` in the load image;
2. copy those bytes to writable RAM;
3. reserve RAM for `g_samples_received`;
4. clear that RAM range before `main()`.

This model explains why a source declaration can have a valid initial value before application code assigns it.

### Why startup belongs in failure analysis

A program can fail even when the application source appears correct if:

- the initialized-data copy range is wrong;
- the zeroing range is wrong;
- the linker symbols used by startup code are inconsistent;
- RAM initialization is skipped;
- a bootloader and application disagree about placement;
- startup code accesses unavailable memory;
- execution enters `main()` with an invalid stack or runtime state.

A useful diagnostic question is:

```text
Did the C source request the correct initial value,
and did this exact runtime establish it correctly?
```

Do not immediately blame `.data` or `.bss` as abstract concepts. Inspect:

- startup implementation;
- linker map;
- section headers;
- load and execution addresses;
- target memory map;
- the exact binary being executed.

Full startup assembly and linker-script authoring are outside M01.

**Must remember:** C defines the required initialized state; startup code, the linker, and the target establish that state in an implementation-specific way.

## 3. `.text`, `.rodata`, `.data`, and `.bss`

The following names describe common section roles in many GNU/ELF and embedded toolchains. They are useful, but none is mandated by ISO C.

| Common section | Typical contents | Typical image implication | Typical runtime implication | Important limitation |
| --- | --- | --- | --- | --- |
| `.text` | Executable instructions | Contributes code bytes to the image | May execute in place or after relocation | Section name and placement are toolchain choices |
| `.rodata` | Read-only objects and string literals | Contributes constant bytes to the image | May remain in read-only memory or be mapped/relocated | `const` does not guarantee `.rodata` or Flash |
| `.data` | Writable objects with nonzero initial values | Commonly needs initial-value bytes in the image | Commonly occupies writable RAM | Not every initialized object must use this section |
| `.bss` | Zero-initialized writable storage | Usually does not require one payload byte per zero | Occupies runtime storage and commonly requires zeroing | Explicit `= 0` does not force `.bss` |

### 3.1 `.text`: generated code, not a function-pointer object

A function body is commonly emitted as machine instructions associated with a code section such as `.text`.

```c
static uint32_t increment_count(uint32_t value)
{
    return value + 1U;
}
```

The function's generated instructions may appear as a symbol classified by `nm` or as bytes in a section shown by `objdump`.

Do not confuse:

- the generated code for a function;
- a function designator in C;
- a function pointer object stored somewhere in memory.

A stored function-pointer object is data. It is not itself the machine instructions of the referenced function. Use symbol and section tools such as `nm` and `objdump` to inspect generated code rather than claiming that a function-pointer object “is in `.text`.” [M01-CR-006]

### 3.2 `.rodata`: common read-only representation

```c
static const char k_build_label[] = "M01 memory lesson";
```

Many builds place this object in a read-only section. However:

- `const` is a type-system property;
- it restricts modification through the declared type;
- it does not guarantee a section name;
- it does not guarantee physical Flash placement;
- it does not guarantee that no runtime copy exists;
- an unused object may be removed.

Attempting to modify an object defined with a `const`-qualified type by casting away `const` produces undefined behavior. That language rule is separate from physical placement. [ISO C99 §§6.5.16.1, 6.7.3]

### 3.3 `.data`: initialized writable objects

```c
static uint32_t g_sample_period_ms = 100U;
```

A common bare-metal model stores initial bytes in non-volatile image storage and copies them to writable RAM before `main()`.

This commonly creates two resource effects:

```text
load-image content for the initial value
+
runtime writable storage
```

The exact representation depends on the compiler, linker, image format, and startup model.

### 3.4 `.bss`: zero-initialized writable storage

```c
static uint32_t g_samples_received;
static uint32_t g_samples_rejected = 0U;
```

Both objects are zero-initialized at the C-language level. A conventional toolchain may represent both in BSS-like storage.

A precise explanation is:

- the objects still require runtime storage;
- startup commonly clears the corresponding range;
- the image usually does not store one explicit payload byte for every required zero;
- the image and linker still need enough metadata, addresses, or symbols to describe what must be reserved and cleared.

Therefore, saying “BSS costs zero Flash” is too absolute. A better statement is:

> BSS-like storage commonly avoids carrying an equivalent zero-filled payload in the load image while still consuming runtime storage and requiring image/layout metadata.

[M01-CR-009]

### 3.5 Image size and RAM use are different questions

Consider a large static buffer:

```c
#include <stdint.h>

static uint8_t g_buffer[4096U];
```

A typical build may reserve approximately 4096 bytes of runtime writable storage while avoiding 4096 explicit zero bytes in the image payload.

Changing it to:

```c
static uint8_t g_buffer[4096U] = {1U};
```

can increase load-image content because the object now requires a nonzero initial representation, while its runtime capacity remains approximately the same.

This is a useful expectation to test with the exact toolchain. It is not a guarantee that every linker represents the object identically.

**Must remember:** section names help explain one build; they do not replace the distinction between C semantics, image representation, and runtime storage.

## 4. Flash, Load Memory, Execution Memory, and RAM

The phrase “stored in Flash” often hides two separate questions:

1. Where are the initial bytes stored in the image?
2. Where does the program execute or access the object at runtime?

Embedded linker terminology often distinguishes:

- **load memory address (LMA):** where content exists in the load image;
- **virtual/execution memory address (VMA):** where the section is accessed while the program runs.

Terminology varies by toolchain, but the distinction is fundamental.

### Typical bare-metal possibilities

```text
Case A: Execute in place
Flash image: .text, .rodata
RAM:        .data, .bss, heap, stack

Case B: Relocated code
Flash image: .text load bytes
RAM:         copied .text execution bytes

Case C: Initialized writable data
Flash image: initial .data bytes
RAM:         writable .data execution storage
```

A section name alone does not reveal the complete load/execution relationship.

### Hosted Linux boundary

A Linux executable is normally loaded by the operating system and dynamic loader. File-backed executable or read-only pages, writable mappings, zero-filled mappings, heap growth, stack mappings, shared libraries, relocation, and address-space randomization are operating-environment behaviors.

The useful M01 distinction is:

```text
bare-metal firmware image model
≠
hosted Linux process-loading model
```

Both can use ELF, but physical Flash/RAM intuition from a microcontroller must not be copied directly into a Linux process explanation.

### Questions to ask

For a real target, determine:

- What is the object format?
- What linker script or linker defaults are used?
- What are the load and execution addresses?
- Is code executed in place?
- Which regions are copied or cleared?
- Does a loader perform relocation?
- Are addresses virtual, physical, or both?
- Which document defines the target memory map?

Full linker-script syntax, virtual-memory theory, MMU/MPU configuration, and loader internals are outside M01.

**Must remember:** “in the image,” “in Flash,” “mapped read-only,” “executed from,” and “stored in RAM” are different claims that require different evidence.

## 5. Guided Hello World Binary-Size Baseline

The DevLinux roadmap includes a small binary-size observation before the Session 02 exercises. This is a guided lesson activity, not a fifth canonical exercise and not an additional solution artifact.

### 5.1 Minimal source

```c
#include <stdio.h>

int main(void)
{
    puts("Hello, memory model.");
    return 0;
}
```

### 5.2 Record the build context

Example host command:

```text
gcc -std=c99 -Wall -Wextra -Wpedantic -Werror -O0 hello.c -o hello
```

Record at least:

- compiler name and version;
- target or host architecture;
- language mode;
- optimization level;
- linker mode if relevant;
- exact output file.

### 5.3 Run GNU `size`

```text
size hello
```

In GNU `size`'s default Berkeley-style output, common columns are:

```text
text    data    bss    dec    hex    filename
```

These are summary categories. They must not be interpreted as exact universal section names.

In particular:

- the `text` summary can include read-only data;
- the three columns do not fully explain every ELF section;
- runtime heap and stack use are not reported as future dynamic consumption;
- shared-library and loader behavior can complicate hosted interpretation;
- target-specific image formats may use different accounting.

### 5.4 The correct conclusion

The activity demonstrates:

```text
source
→ exact build command
→ exact binary
→ tool summary
→ bounded interpretation
```

A good observation is:

> Under the recorded compiler, target, options, and binary, GNU `size` reports these summary values.

A bad conclusion is:

> Every C program with this source always requires exactly these text, data, and BSS sizes.

Use `objdump`, `readelf`, or a linker map when the summary is not enough.

**Must remember:** tool output becomes evidence only when the exact artifact and build context are recorded.

## 6. Stack Mental Model

ISO C defines automatic storage duration and function-call semantics, but it does not require a conventional hardware stack or a fixed frame layout.

A useful conceptual model is:

```text
each active ordinary call
→ has execution state
→ may require call-related storage
→ disappears when the invocation returns
```

Example call chain:

```text
main()
  └─ initialize_service()
       └─ parse_configuration()
            └─ validate_field()    ← current invocation
```

When `validate_field()` returns, its invocation is no longer active and execution resumes in `parse_configuration()`.

A debugger often presents each active invocation as a **frame** containing enough information to inspect:

- the current source location;
- arguments;
- local variables;
- the caller relationship;
- saved execution state.

The physical representation is controlled by the ABI and compiler.

### What cannot be assumed

Do not assume:

- the stack grows toward lower addresses;
- the stack grows toward higher addresses;
- every call consumes the same number of bytes;
- every local is stored in memory;
- every function has a visible frame;
- frame pointers are always present;
- inlined functions produce ordinary frames;
- tail-call transformations preserve the apparent call chain.

Optimization, ABI rules, interrupt handling, exception mechanisms, security features, and target architecture can change all of these observations.

### Automatic objects and address observations

```c
#include <stdint.h>

static void observe_call(uint32_t depth)
{
    uint8_t marker = 0U;
    const void *p_marker = &marker;

    (void)p_marker;
    (void)depth;
}
```

The address of `marker` can be printed in a controlled implementation experiment, but one observed address does not prove:

- stack direction;
- exact frame size;
- total stack usage;
- safe remaining capacity.

A compiler can also optimize the object away unless the observation forces an addressable representation.

`uintptr_t` is optional. It exists only when the implementation provides an unsigned integer type capable of representing converted `void *` values. Even where available, converting unrelated object addresses to integers and subtracting them produces an implementation-scoped numeric observation, not portable C pointer-distance semantics. [ISO C99 §7.18.1.4; M01-CR-015]

**Must remember:** use the stack as a logical call-storage model first; treat addresses and frame sizes as evidence from one ABI and one build.

## 7. Stack Overflow and Recursion-Lab Boundaries

A **stack overflow** occurs when the call-related storage required by execution exceeds the stack capacity or configured limit available to the thread, task, or program.

Common causes include:

- unintended infinite recursion;
- excessive bounded recursion;
- deep call chains;
- large automatic arrays;
- nested library calls with significant stack use;
- interrupt nesting or exception paths;
- underestimated task-stack configuration.

Possible symptoms include:

- memory corruption;
- a protection fault;
- corrupted return state;
- an incomplete backtrace;
- failure far from the deepest call;
- inconsistent behavior after optimization or small code changes.

### 7.1 Recursion requires a bound but a bound is not a proof

```c
#include <stdint.h>

static void observe_depth(uint32_t remaining_depth)
{
    uint8_t marker = 0U;

    (void)marker;

    if (remaining_depth > 0U)
    {
        observe_depth(remaining_depth - 1U);
    }
}
```

The explicit depth argument bounds the number of recursive calls for this input. It does **not** prove:

- the target has enough stack;
- every invocation uses a fixed amount;
- the measured address difference equals stack consumption;
- the guard matches the real overflow boundary;
- recursion is MISRA-compliant.

### 7.2 MISRA Rule 17.2 boundary

MISRA C:2012 Rule 17.2 prohibits recursive function calls. A controlled educational recursion lab therefore conflicts with the rule. A runtime guard does not make recursion compliant.

The correct interpretation is:

- the lab intentionally demonstrates call depth and risk;
- the guard reduces experimental risk;
- the guard is not a proof of stack safety;
- a MISRA compliance claim would require the project's authorized treatment of the violation;
- a required-rule deviation is a formal project decision, not an automatic result of adding a depth check.

[MISRA C:2012 Rule 17.2; M01-CR-011]

### 7.3 Production stack analysis

A production stack budget can require:

- compiler-generated stack-usage reports;
- call-graph analysis;
- RTOS high-water-mark evidence;
- interrupt nesting analysis;
- target ABI documentation;
- worst-case execution paths;
- margin policy;
- controlled measurement on the intended build.

These methods are outside the introductory lab.

### 7.4 Do not find the limit by crashing

Deliberately recursing until a real overflow is destructive and does not produce a reliable universal limit. Corruption may occur before a visible fault, and the failure boundary can change with a small build change.

Use a bounded experiment with conservative limits and document that it is only an observation.

**Must remember:** a recursion guard limits one experiment; it does not prove the true stack boundary or make recursion compliant with Rule 17.2.

## 8. Bad Pointer, Buffer Corruption, and Stack Failure

A crash near stack-related code does not automatically mean stack exhaustion. Pointer and buffer defects can corrupt call state and create similar symptoms.

### 8.1 Distinguish the failure classes

| Failure class | Root cause | Typical example |
| --- | --- | --- |
| Stack exhaustion | Total call-related storage exceeds the available stack | Excessive recursion or large automatic arrays |
| Buffer overflow | Access goes outside one object's bounds | Writing element `capacity` of an array |
| Null-pointer dereference | A null pointer is used as an object address | Dereferencing `NULL` |
| Dangling pointer | Pointer refers to an object whose lifetime ended | Use after `free()` or returning an automatic object's address |
| Uninitialized pointer | Pointer value was never established | Dereferencing an indeterminate pointer |
| Invalid conversion or address | Pointer value does not designate a valid object of the required type | Misused integer-to-pointer conversion |

### 8.2 Buffer overflow example

The following is deliberately invalid:

```c
#include <stdint.h>

void invalid_write(void)
{
    uint8_t samples[4] = {0U, 0U, 0U, 0U};

    samples[4] = 1U; /* Undefined behavior: valid indexes are 0 through 3. */
}
```

A stack-local buffer overflow can damage nearby call-related storage and later produce:

- a corrupted local;
- an invalid return;
- a broken backtrace;
- a crash in another function.

The root cause remains an out-of-bounds write, not necessarily total stack exhaustion.

### 8.3 Evidence-oriented comparison

| Evidence | Stack exhaustion hypothesis | Bad-pointer/buffer hypothesis |
| --- | --- | --- |
| Call depth | Failure correlates with increasing depth | Failure can occur at shallow depth |
| Automatic storage | Larger locals or smaller task stack worsen the issue | No consistent relation to total call storage |
| Stop location | May fail during call/return or after corruption | Often stops at a specific invalid access |
| Backtrace | Repeated/deep frames or damaged unwind state | May show a normal chain to the invalid access |
| Controlled correction | Lower depth or reduced automatic storage changes result | Correcting lifetime, bounds, ownership, or null validation removes failure |

These are clues, not proofs. A buffer overflow can destroy the evidence used to diagnose it.

### 8.4 Correct investigation order

```text
reproduce exact failure
→ record build and input
→ capture stopped location and backtrace
→ inspect pointer, index, capacity, and lifetime
→ inspect call depth and large automatic objects
→ form competing hypotheses
→ make one controlled correction
→ compare against the original evidence
```

Do not change optimization, input, recursion depth, and pointer logic simultaneously. That destroys causal evidence.

**Must remember:** similar symptoms can have different root causes; diagnose from pointer validity, bounds, lifetime, call depth, and controlled evidence together.

## 9. Heap, Dynamic Allocation, Ownership, and Failure Modes

Dynamic allocation obtains storage during execution through services such as `malloc()` and releases it through `free()`. It can support variable capacity and runtime-selected lifetime, but it introduces failure, ownership, timing, fragmentation, and policy concerns.

### 9.1 ISO C allocation contract

```c
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

bool process_block(void)
{
    uint8_t *p_buffer = malloc(128U);

    if (p_buffer == NULL)
    {
        return false;
    }

    /* Process the block. */

    free(p_buffer);
    p_buffer = NULL;
    return true;
}
```

Important points:

- check the result before dereferencing;
- define who owns a successful allocation;
- release it exactly once when its lifetime ends;
- do not use it after release;
- setting one local pointer to `NULL` does not repair other aliases.

In C, `malloc()` returns `void *`, which converts to an object-pointer type without an explicit cast. Casting the result is not required for this conversion and does not validate the allocation. Session 02's association with MISRA Rule 11.6 is incorrect because that rule concerns casts between `void *` and arithmetic types, not the ordinary C conversion from `void *` to another object-pointer type. The primary exercise correction remains owned by the exercise gate. [ISO C99 §6.3.2.3; M01-CR-004]

### 9.2 Allocation failure

`malloc()` returns a null pointer when it cannot provide the requested storage.

A null result does not reveal the full root cause. Possible reasons include:

- insufficient total free memory;
- no sufficiently large contiguous block;
- allocator policy;
- resource limits;
- address-space limits;
- internal allocator metadata failure;
- target-specific restrictions.

The API must define what the caller does when allocation fails.

### 9.3 Memory leak

A memory leak occurs when allocated storage remains allocated after the program no longer needs it, or when the program loses the ownership information needed to release it.

**Intentionally incorrect negative example — an early error return leaks storage.**

```c
#include <stdbool.h>
#include <stdlib.h>

bool process_input_leaking(bool input_is_valid)
{
    void *p_buffer = malloc(128U);

    if (p_buffer == NULL)
    {
        return false;
    }

    if (!input_is_valid)
    {
        return false; /* Incorrect: p_buffer is still allocated. */
    }

    free(p_buffer);
    return true;
}
```

When `malloc()` succeeds and `input_is_valid` is false, the function loses its only local ownership reference without calling `free()`. The following corrected version releases the allocation on every path after successful allocation.

```c
#include <stdbool.h>
#include <stdlib.h>

bool process_input(bool input_is_valid)
{
    void *p_buffer = malloc(128U);

    if (p_buffer == NULL)
    {
        return false;
    }

    if (!input_is_valid)
    {
        free(p_buffer);
        return false;
    }

    /* Process valid input. */

    free(p_buffer);
    return true;
}
```

Every path after successful allocation must preserve the ownership rule.

Process-exit reclamation does not make leaks acceptable in:

- firmware;
- daemons;
- services;
- long-running tests;
- restartable subsystems;
- repeated library calls.

### 9.4 Fragmentation

Fragmentation can prevent a request even when the sum of free space appears sufficient.

```text
[free 32 B] [used 64 B] [free 32 B]

Total free: 64 B
Request:    48 B
Result:     no single 48-B contiguous block
```

Actual behavior depends on allocator design and workload.

### 9.5 MISRA Directive 4.12 boundary

MISRA C:2012 Directive 4.12 states that dynamic memory allocation shall not be used. It is categorized as Required.

Therefore:

- the educational Session 02 allocation lab intentionally conflicts with the directive;
- the lab does not recommend unrestricted production heap use;
- a project claiming MISRA compliance needs the applicable formal deviation if it chooses dynamic allocation;
- checking for `NULL` makes the code safer but does not make the design compliant with the directive;
- the decision must consider bounded behavior, timing, ownership, fragmentation, failure handling, and project policy.

[MISRA C:2012 Dir 4.12; M01-CR-003]

**Must remember:** dynamic allocation is a design policy plus a runtime contract; successful `malloc()` creates both storage and an ownership obligation.

## 10. Memory-Selection Decision Model

No storage strategy is universally best. Choose based on lifetime, capacity, failure model, concurrency, target resources, and project policy.

| Choice | Good fit | Main risks or limits | Required evidence |
| --- | --- | --- | --- |
| Automatic storage | Small bounded temporary state tied to one invocation | Stack exhaustion, lifetime ends on return | Maximum size, call depth, task-stack budget |
| Static storage | Program-lifetime state, fixed buffers, persistent module state | Global coupling, non-reentrancy, fixed RAM cost | Ownership, initialization, concurrency policy, linker/runtime evidence |
| Dynamic allocation | Runtime-selected capacity or lifetime | OOM, leaks, fragmentation, timing variability, policy conflict | Allocation contract, ownership, failure paths, allocator behavior |
| Read-only object | Immutable tables, labels, configuration constants | Physical placement not guaranteed by `const` alone | Type semantics plus section/linker/target evidence |
| Fixed pool | Bounded object count with predictable storage policy | Capacity exhaustion, pool design complexity | Pool size, allocation/release rules, concurrency behavior |

### Questions to ask before choosing

```text
How long must the object live?
What is the maximum capacity?
Can the capacity be known at build time?
What happens when storage is unavailable?
Must allocation time be bounded?
Can the API be called concurrently?
Who owns release?
Which memory budget is affected?
Which project safety rule applies?
```

### Important boundaries

- Static allocation avoids runtime allocation failure but can still create system-level capacity failure.
- Automatic storage is not “free”; it consumes stack capacity while active.
- `const` does not prove physical read-only placement.
- Dynamic allocation is not always wrong, but it requires an explicit policy and failure model.
- Fixed-pool implementation and allocator architecture belong to M02.

**Must remember:** choose storage from lifetime, capacity, failure, ownership, and timing requirements—not from a slogan such as “never use heap” or “static is always safe.”

## 11. Binary-Inspection Tools and Their Limits

Use each tool to answer a specific question about a specific artifact.

| Tool | Primary question | Typical evidence | What it cannot prove by itself |
| --- | --- | --- | --- |
| `size` | What high-level footprint summary does this artifact report? | Berkeley-style `text`, `data`, `bss` summaries | Exact section placement, runtime heap/stack use, physical addresses |
| `nm` | Which symbols exist and how are they commonly classified? | Symbol names, addresses, sizes, type letters | Complete section layout, runtime dynamic storage, portable classification |
| `objdump -h` | Which section headers and sizes exist in this object? | Actual section names, sizes, flags | Final physical target placement without linker/target context |
| `readelf` | What ELF headers, sections, symbols, and program headers exist? | ELF-specific structural evidence | Non-ELF formats or complete board-level memory behavior |
| Linker map | How did the linker allocate input sections and symbols? | Output sections, addresses, contributions, symbols | Runtime allocator behavior or all loader/runtime changes |

### 11.1 GNU `size`

```text
size firmware.elf
```

Use it for:

- baseline footprint;
- regression comparison;
- optimization comparison;
- quick image/RAM category review.

Limitations:

- Berkeley column names are summaries;
- read-only data can contribute to `text`;
- `bss` is not “zero RAM”;
- heap and stack future usage are not represented as ordinary allocated runtime consumption.

### 11.2 GNU `nm`

```text
nm -S firmware.elf
```

Common GNU symbol letters can include:

- `T`/`t`: code-related symbols;
- `R`/`r`: read-only data;
- `D`/`d`: initialized writable data;
- `B`/`b`: BSS-like storage.

Exact meaning depends on object format and platform. Optimization, stripping, merging, and garbage collection can remove or transform symbols.

Use `nm` to inspect a named symbol, not to infer the entire physical memory map.

### 11.3 GNU `objdump`

```text
objdump -h firmware.elf
```

This shows section headers and is useful when the `size` summaries are too coarse.

Additional commands can disassemble code or display symbols, but full disassembly analysis is outside M01.

### 11.4 GNU `readelf`

```text
readelf -S firmware.elf
readelf -l firmware.elf
```

For ELF artifacts:

- `-S` inspects section headers;
- `-l` inspects program headers/segments used by a loader.

This helps distinguish:

```text
link-time sections
from
load-time segments
```

ELF is not universal. Do not teach `readelf` output as a C-language requirement.

### 11.5 Linker map

A linker map can answer:

- which object file contributed bytes;
- how input sections formed output sections;
- where symbols were assigned;
- why one section grew;
- which region overflowed;
- which content was discarded or retained.

It is often the strongest evidence for final link allocation, but physical behavior still depends on startup and target execution.

### 11.6 Pointer-address limitations

ISO C pointer subtraction is defined only for pointers into the same array object or one past it. Subtracting pointers to unrelated globals, heap objects, and automatic objects is not portable distance measurement. [ISO C99 §6.5.6]

A supporting implementation may convert object pointers to `uintptr_t` for diagnostic output. Such numeric differences are implementation-scoped observations and must not be called portable segment distances. [ISO C99 §7.18.1.4; M01-CR-007, M01-CR-015]

### 11.7 Function-symbol evidence

Do not print a function pointer with `%p` by assuming it converts portably to `void *`. ISO C distinguishes function pointers from object pointers. Use `nm`, `objdump`, or debugger symbol information for function-code evidence. [M01-CR-006]

**Must remember:** select the tool from the question, record the exact artifact, and state what the evidence does not prove.

## 12. Basic GDB Workflow

GDB helps turn a crash or unexpected call chain into a testable hypothesis.

### 12.1 Prepare a diagnostic build

A simple introductory host build can use:

```text
gcc -std=c99 -Wall -Wextra -Wpedantic -Werror -g -O0 demo.c -o demo
```

Record:

- compiler and version;
- target;
- flags;
- exact binary;
- input that reproduces the issue.

`-O0` often makes source-level investigation easier, but it is not automatically the release configuration and can change the failure.

### 12.2 Core commands

```text
gdb ./demo
(gdb) run
(gdb) backtrace
(gdb) frame 0
(gdb) info args
(gdb) info locals
(gdb) print pointer_value
```

| Command | Question answered |
| --- | --- |
| `run` | Can the issue be reproduced under the debugger? |
| `backtrace` | Which call chain reached the stop? |
| `frame N` | What state is visible in one selected invocation? |
| `info args` | What arguments reached this invocation? |
| `info locals` | Which local values are available? |
| `print` | What is the value of a relevant expression? |

### 12.3 Stack-exhaustion hypothesis

Inspect:

- repeated or unexpectedly deep frames;
- current depth values;
- large automatic objects;
- configured stack size;
- whether reducing a bounded test depth changes the result.

### 12.4 Bad-pointer hypothesis

Inspect:

- nullness;
- pointer value;
- owner;
- allocation/release history;
- index and capacity;
- object lifetime;
- the exact dereference.

### 12.5 Evidence limits

GDB evidence can be limited by:

- optimization;
- inlining;
- omitted frame pointers;
- stripped symbols;
- corrupted stack state;
- undefined behavior;
- debugger/target limitations.

An incomplete backtrace does not prove that stack exhaustion did not occur. A plausible pointer value does not prove that it designates a live object.

**Must remember:** GDB reveals state from one stopped execution; combine it with the source contract, build context, and controlled experiments.

## 13. Optimization Levels and Evidence

Optimization changes generated code while preserving the required observable behavior of a program whose execution is defined under the language rules.

It can change:

- instruction selection;
- inlining;
- function boundaries;
- local-variable visibility;
- frame layout;
- stack use;
- section sizes;
- symbol availability;
- code and data addresses;
- execution timing.

It must not change the required observable behavior of a defined program, but it can expose existing undefined behavior because the compiler may assume the program obeys the language rules.

### 13.1 GCC level meanings are intent, not rankings

| Option | General intent | Engineering boundary |
| --- | --- | --- |
| `-O0` | Disable most optimization; favor compile speed and straightforward debugging | Not representative of release size or timing |
| `-O1` | Enable a basic optimization set | Can already alter frames and variable visibility |
| `-O2` | Enable a broader optimization set | Not guaranteed to be fastest or smallest |
| `-O3` | Add more aggressive transformations | Can improve or worsen size and workload performance |
| `-Os` | Optimize with code-size goals | Smaller code is not guaranteed for every program/target |

Exact enabled passes depend on:

- GCC version;
- target;
- language;
- other options.

### 13.2 Correct comparison workflow

```text
same source
→ same target
→ record compiler version
→ change one optimization option
→ build exact artifacts
→ compare size
→ run representative target measurements
→ inspect changed sections/symbols
→ document conclusion
```

Do not compare one `-O0` host build against a different `-Os` embedded build and attribute every difference to optimization.

### 13.3 Optimization and undefined behavior

Optimization does not create undefined behavior in an otherwise defined program.

A better model is:

```text
source already contains undefined behavior
→ optimizer assumes language rules hold
→ generated behavior differs from debug expectations
→ defect becomes visible
```

The correction is to remove the undefined behavior, not to declare optimization unsafe.

### 13.4 Debugging boundary

An optimized build may show:

- `<optimized out>`;
- reordered instructions;
- inlined calls;
- missing frames;
- unexpected variable locations.

These effects limit debugger observability; they do not mean that the C object model changed.

[M01-CR-013]

**Must remember:** optimization options are hypotheses about generated code; validate size, timing, stack, and debug effects on the intended compiler and target.

## 14. Failure-Analysis Workflow

Use a disciplined workflow instead of jumping from one symptom to one conclusion.

```text
symptom
→ reproducible case
→ competing hypotheses
→ required evidence
→ appropriate tool
→ controlled experiment
→ root cause
→ corrective action
→ verification
```

### 14.1 Step 1 — State the symptom precisely

Weak:

> Memory is broken.

Strong:

> The process faults after approximately 420 repeated requests in build X with input Y; RSS grows continuously and each request performs one dynamic allocation.

### 14.2 Step 2 — Preserve the context

Record:

- source revision;
- compiler and version;
- target;
- build flags;
- binary;
- input;
- environment;
- task or process limits;
- relevant logs.

### 14.3 Step 3 — Create competing hypotheses

Example for a recursive crash:

- stack capacity exhausted;
- buffer overwrite corrupted return state;
- dangling pointer used during a deep call;
- optimization exposed existing UB;
- task stack configured incorrectly.

### 14.4 Step 4 — Select evidence by hypothesis

| Hypothesis | Useful evidence |
| --- | --- |
| Stack exhaustion | Backtrace depth, task-stack configuration, large automatic objects, stack-usage evidence |
| Bad pointer | Stopped dereference, owner/lifetime, allocation/release path, index/capacity |
| Leak | Repeated workload, ownership review, allocation/release counts |
| Unexpected section growth | `size`, `nm`, `objdump`, `readelf`, linker map |
| Startup initialization failure | Startup code, linker symbols, section/load evidence, state before `main()` |
| Optimization-sensitive failure | Same source built under controlled options, UB review, changed debugger/tool evidence |

### 14.5 Step 5 — Change one variable

Examples:

- reduce bounded recursion depth without changing pointer logic;
- correct one bounds check without changing optimization;
- add missing cleanup without changing workload;
- rebuild only with a different optimization flag;
- compare only one known section contribution.

### 14.6 Step 6 — Correct the root cause

Possible corrections include:

- enforce capacity before indexing;
- remove recursion or define a verified production bound;
- redesign ownership and cleanup;
- handle allocation failure;
- reduce automatic storage;
- correct linker/startup configuration;
- remove undefined behavior;
- select a more predictable storage policy.

### 14.7 Step 7 — Verify and retain evidence

Verify:

- the original case;
- boundary cases;
- long-running behavior where relevant;
- intended release configuration;
- target-specific constraints.

A fix that only hides the symptom under `-O0` is not a complete correction.

**Must remember:** a tool is chosen after the hypothesis; the result is trusted only within the recorded build and experiment.

## 15. Embedded and Linux Applications

The three-layer model and failure workflow apply to both firmware and Linux systems, but the evidence differs.

| Scenario | Engineering question | Useful evidence | Boundary |
| --- | --- | --- | --- |
| Firmware image budget | Which content increased program-image size? | `size`, section headers, symbols, linker map | Section growth is not automatically runtime RAM growth |
| Firmware RAM budget | Which static, heap, stack, or task resources consume RAM? | Linker map, runtime configuration, allocation policy, stack evidence | `size` summaries alone are insufficient |
| Boot-time initialization | Were writable initialized and zero-initialized objects established correctly? | Startup code, linker symbols, memory map, early debugger inspection | Exact sequence is implementation-specific |
| RTOS task stack | Is the configured stack sufficient for worst-case call paths and interrupts? | Task configuration, call analysis, high-water evidence, target measurements | One recursive address proxy is not proof |
| Long-running Linux service | Is memory growth a cache, retained ownership, fragmentation, or leak? | Workload evidence, ownership review, process metrics, allocator tools | Process exit does not excuse operational leaks |
| Crash triage | Is the failure stack exhaustion, buffer corruption, or invalid lifetime? | Core/debugger state, backtrace, pointer and capacity evidence | One signal or frame is not proof |
| Binary-size regression | Which source or library contribution changed? | Same-build comparison, symbols, sections, map | Compiler/target changes must be controlled |
| Optimization regression | Did size, timing, stack, or observability change? | Same source/target under recorded options | Higher optimization is not universally better |

### Example: Embedded firmware

A large zero-initialized buffer can:

- barely change load-image payload;
- significantly increase runtime RAM;
- reduce available stack/heap space;
- cause a link-region overflow.

The correct evidence can include a linker map and target memory-region definition, not only `size`.

### Example: Linux service

A service can have:

- stable executable section size;
- stable static data;
- growing dynamic allocations during requests.

Binary tools cannot prove or disprove the runtime leak. The ownership path and workload behavior must be inspected.

### Example: Cross-build interpretation

A host `gcc` result is not direct evidence for an ARM firmware image. Use the intended cross compiler, target ABI, linker configuration, and target runtime.

**Must remember:** preserve the same reasoning method across environments, but use evidence from the actual loader, runtime, allocator, ABI, and target.

## 16. Key Takeaways and References

### Review checklist

- Did I separate ISO C semantics, executable representation, and physical placement?
- Did I label every section and memory-map claim as implementation-specific?
- Did I distinguish load-image content from runtime storage?
- Did I explain that BSS-like storage still consumes runtime memory?
- Did I avoid claiming that `const` guarantees Flash or `.rodata`?
- Did I avoid assuming stack direction or fixed frame size?
- Is recursion bounded for the experiment, and is the Rule 17.2 conflict explicit?
- Did I distinguish stack exhaustion from buffer and pointer corruption?
- Does every successful allocation have one clear owner and release path?
- Is allocation failure handled before pointer use?
- Did I identify the project's dynamic-allocation and deviation policy?
- Did I choose `size`, `nm`, `objdump`, `readelf`, or a linker map from the question being asked?
- Did I record the exact binary, compiler, target, and flags?
- Did I treat optimization results as measurements rather than rankings?
- Did I follow symptom → hypothesis → evidence → correction?

### Claim-level reference allocation

| Claim family | Primary authority | Supporting authority |
| --- | --- | --- |
| Storage duration, initialization, pointers, allocation, and language behavior | ISO/IEC 9899:1999 and applicable public WG14 material | Selected implementation documentation |
| Dynamic-allocation restriction and recursion rule | MISRA C:2012 Directive 4.12 and Rule 17.2 | Project compliance/deviation policy |
| Binary-size and symbol evidence | GNU Binutils manuals for `size`, `nm`, `objdump`, and `readelf` | Selected object-format and target documentation |
| Link placement | GNU linker documentation and exact linker map | Target memory map and startup documentation |
| Startup model | Selected compiler runtime, board/architecture startup, and target documentation | ISO C initialization requirements |
| Stack frames and ABI behavior | Selected ABI and compiler documentation | GNU GDB frame documentation |
| Debugger workflow | GNU GDB manual: Running, Backtraces, Frames, Arguments, Locals, and Examining Data | Compiler/debug-symbol documentation |
| Optimization levels | GCC Optimize Options for the selected compiler version | Exact target size and runtime measurements |
| Linux process behavior | Selected Linux loader, runtime, allocator, and process documentation | Exact process evidence |
| Embedded placement and memory budget | Target reference manual, linker/startup configuration, and build artifacts | Board/RTOS documentation |

The DevLinux Week 1 Day 2 roadmap and Session 02 define this lesson's learning scope and exercise context. `Full-Embedded-C-Notes.md` remains discovery material only and is not an authority for claims about section placement, startup behavior, stack direction, or physical memory.

No full Memory Segment Analyzer or Stack Depth Monitor solution is included in this lesson.
