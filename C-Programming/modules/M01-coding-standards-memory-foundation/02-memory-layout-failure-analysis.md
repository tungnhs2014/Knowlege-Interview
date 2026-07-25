# M01-L02 — Memory Layout & Failure Analysis

> **Status:** APPROVED.

## 1. Learning Objectives

This lesson explains the practical memory model used when reviewing a C program built for an embedded target or Linux host. It prepares you for session-02's **Memory Segment Analyzer** and **Stack Depth Monitor** exercises without supplying their implementations. After completing it, you should be able to:

- distinguish C language semantics from a particular toolchain's sections and runtime-memory layout;
- describe the common roles of `.text`, `.rodata`, `.data`, `.bss`, heap, and stack;
- explain why initialized data is commonly copied to RAM and BSS is commonly zeroed before `main()` on embedded systems;
- reason about stack frames, call depth, recursion, and stack-overflow risk without assuming a universal frame layout or stack direction;
- handle allocation failure, release dynamic memory correctly, and recognize leaks, fragmentation, and bad-pointer failures;
- use GNU `size`, `nm`, and `objdump` to inspect one build without mistaking their output for a portable language guarantee;
- use a small GDB workflow to inspect a crash and its call stack;
- explain the purpose and debugging trade-offs of `-O0`, `-O1`, `-O2`, `-O3`, and `-Os`.

The lesson uses the common Embedded C vocabulary because it is useful for firmware sizing and debugging. The names, placement, and addresses discussed here are implementation observations: they depend on the compiler, linker, run-time library, target, and build options.

## 2. Start with the Right Model: C Semantics versus Build Artifacts

ISO C defines object lifetime, initialization requirements, storage duration, and valid program behavior. It does **not** require a physical `.text`, `.data`, or `.bss` placement, a particular heap implementation, a stack direction, or an object-file format. Those are conventions and implementation choices made by a toolchain, linker configuration, operating environment, and target hardware.

That distinction prevents two common mistakes. The first is treating a familiar memory map as if every C program must have it. The second is dismissing the map as irrelevant because it is not mandated by the language. The practical view is between those extremes: use the map to understand the binary you are building, then verify it with that build's tools and documentation.

For example, these declarations have C-level meanings independent of their eventual placement:

```c
static const uint32_t k_protocol_magic = 0x4D303131U;
static uint32_t g_retry_limit = 3U;
static uint32_t g_error_count;

void process_packet(void)
{
    uint32_t local_status = 0U;

    (void)local_status;
}
```

The first object is read-only through this declaration, the next two have static storage duration, and `local_status` has automatic storage duration. A typical embedded build may place them in sections conventionally called `.rodata`, `.data`, `.bss`, and stack-backed storage respectively. Another build may merge, discard, rename, relocate, or optimize them. Learn the C meaning first; use inspection tools to learn the physical result.

## 3. The Conceptual Memory Layout

The following table is a useful mental model for a conventional compiled C program. It describes common roles, not a layout promised by ISO C.

| Common name | Typical contents | Common Embedded intuition | Important limit |
| --- | --- | --- | --- |
| `.text` | Executable instructions | Often stored in non-volatile program memory | A section name and physical location are linker and target choices. |
| `.rodata` | Read-only constants and string literals | Often stored with program image content | `const` does not guarantee this section or physical read-only memory. |
| `.data` | Nonzero-initialized writable static objects | Initial bytes are commonly stored in the image, then copied to RAM | Not every initialized object must be represented this way. |
| `.bss` | Zero-initialized or uninitialized writable static objects | RAM is reserved and is commonly zeroed during startup | Explicit `= 0` does not force a particular section. |
| Heap | Dynamic-allocation arena, if provided | Grows and shrinks as `malloc()` and `free()` are used | It is a runtime service, not an ISO C object-file section. |
| Stack | Active function-call storage | Supports automatic objects and call state | Direction, layout, and even storage use are implementation-dependent. |

### 3.1 Code and read-only data

The body of a compiled function is commonly emitted as instructions in `.text`. Read-only constants, lookup tables, and string literals are commonly emitted into a read-only section such as `.rodata`. On a microcontroller that executes in place from Flash, both often contribute to program-image size. On a Linux host, the loader and operating system provide a different runtime environment. The source-level lesson remains the same: if an object is defined with a `const`-qualified type, attempting to modify it by casting away `const` has undefined behavior; ISO C does not dictate its physical residence.

```c
static const char k_build_label[] = "M01 memory lesson";
```

This is a good candidate for a read-only section in many toolchains. It may instead be merged with another constant, eliminated if unused, or placed according to a target-specific rule. Do not write code that relies on its address being near a function or another constant.

### 3.2 Writable static data: `.data` and `.bss`

Objects with static storage duration exist for the program's whole execution. C requires them to have their specified initial values when the program begins its normal C execution; omitted initialization produces zero initialization at the language level. How a bare-metal product achieves that state is a startup and linker concern.

```c
static uint32_t g_sample_period_ms = 100U;
static uint32_t g_samples_received;
static uint32_t g_samples_rejected = 0U;
```

In a common Embedded startup design, `g_sample_period_ms` belongs to writable RAM at runtime and needs the initial value `100U`. The firmware image therefore contains initial-value bytes somewhere in non-volatile storage, and early startup copies them into the RAM location. This is the usual intuition behind `.data`.

`g_samples_received` has no explicit initializer and is zero-initialized by the C language. `g_samples_rejected = 0U` has an explicit zero initializer but is commonly treated the same way by embedded toolchains: both may be placed in BSS-like storage. BSS generally consumes RAM and requires startup zeroing, but does not need equivalent payload bytes for each zero in the firmware image. The image still needs metadata or layout information so startup code knows what RAM range to clear.

This distinction matters when reading a size report. Changing a large static buffer from a nonzero initializer to an all-zero initializer can reduce image payload while leaving the RAM requirement essentially unchanged. It is not a language guarantee and must be checked in the built artifact.

The common Embedded Flash/image versus runtime-RAM view makes the central distinction visible:

| Common name or concept | Typical firmware-image / non-volatile content | Typical runtime RAM use |
| --- | --- | --- |
| `.text` | Executable instructions | Usually none when executing in place; some systems relocate code. |
| `.rodata` | Constant and string bytes | Usually none unless the target relocates or copies them. |
| `.data` | Initial-value bytes | Writable objects occupy RAM after startup copies their initial values. |
| `.bss` | No equivalent payload bytes for each initial zero | Writable zero-initialized storage occupies RAM and is commonly cleared at startup. |
| Heap | No bytes for individual future allocations | A runtime allocation arena exists if the implementation provides one. |
| Stack | No bytes for individual future calls | Runtime call storage is reserved or established for active invocations. |

Thus, `.data` commonly consumes both firmware-image bytes for initial values **and** runtime RAM, while `.bss` consumes runtime RAM but normally does not require equivalent zero payload bytes in the image. This is a common embedded-toolchain model, not a physical-placement promise from ISO C.

### 3.3 Heap and stack are runtime concepts

The heap is the storage arena exposed by a dynamic-allocation implementation such as `malloc()` and `free()`. The stack is the call-oriented storage used by many implementations for function invocations and automatic objects. Both names are useful in embedded and Linux engineering, but neither is a section placement mandated by ISO C.

On a small microcontroller, a linker configuration commonly reserves RAM for static data, heap, and stack. On a Linux process, the operating system establishes the runtime environment. In either case, do not infer a permanent relationship between their addresses from a single run. Address values and apparent distances can change with the target, link mode, loader, optimization, configuration, and program input.

## 4. Flash/Image Content, RAM, and Startup Initialization

Before `main()` can safely use normal C objects, an embedded system must establish the C runtime environment. The exact reset sequence is target and toolchain specific. CMSIS startup templates, for example, hand control from a reset handler through system initialization to the C/C++ runtime, while other toolchains or operating systems use different entry code.

The common bare-metal sequence is conceptually:

```text
reset
→ establish an initial stack
→ perform target and clock setup as required
→ copy initialized writable data from image storage to RAM
→ zero BSS-like RAM ranges
→ enter the C runtime / call main()
```

The sequence explains a recurring embedded question: *why can a global variable have an initial value before my code assigns it?* The source declaration expresses the required value; startup and runtime code establish it before ordinary application execution. If startup copy or zero ranges are wrong, static objects can have incorrect values even though the C source is correct.

Consider the two declarations below:

```c
static uint8_t g_mode = 2U;
static uint8_t g_fault_flags;
```

`g_mode` needs `2U` in writable RAM, so its initial byte is commonly represented in the program image and copied during startup. `g_fault_flags` must begin as zero, so a common startup implementation reserves RAM and clears it. This is why `.data` is often associated with an initial-value image and `.bss` with zeroing work rather than duplicated zero payload. The terms describe a conventional implementation, not a physical requirement of the C standard.

Startup behavior is an important diagnostic boundary. If a bare-metal program fails before `main()`, or static state begins unexpectedly nonzero, inspect the board support package, compiler runtime, startup file, linker configuration, and target reset behavior. This lesson does not teach linker-script syntax; use the relevant toolchain and device documentation when a product needs changes there.

## 5. Stack Frames, Call Depth, and Recursion

Every active function invocation needs execution state. A debugger commonly presents that state as a **stack frame** containing the current location, a route back to the caller, arguments, and accessible local variables. The exact physical frame layout is not a C-language contract. A compiler can keep a local in a register, omit a frame, reuse storage, inline a call, or transform a call when optimization permits.

For a newcomer, the safe conceptual model is: each ordinary call adds an active invocation; returning removes it. If `main()` calls `configure()`, which calls `parse_record()`, then a failure in `parse_record()` has a call chain that GDB can usually display as three frames. A deep call chain consumes more call-related storage and increases the risk of exhausting a fixed embedded stack.

```text
Active calls while parse_record() runs:

main()
  └─ configure()
       └─ parse_record()     ← current innermost invocation

When parse_record() returns:

main()
  └─ configure()             ← resumes with parse_record() no longer active
```

This is a logical call-chain diagram, not a physical frame layout or an assertion about numerical stack-growth direction.

Recursion is a direct form of unbounded call depth unless the input or guard provides a strict bound. Each recursive invocation has its own automatic objects. This small fragment illustrates the observation without implementing the session-02 monitor:

```c
static void observe_depth(uint32_t remaining_depth)
{
    uint8_t marker = 0U;
    const void *p_marker = &marker;

    (void)p_marker;

    if (remaining_depth != 0U)
    {
        observe_depth(remaining_depth - 1U);
    }
}
```

The address of `marker` may appear to move by a regular amount on one debug build. Do not turn that observation into a stack-size calculation or a portability claim. The stack may grow in either numerical direction, frame size may vary by call path and optimization, and the compiler can change whether the marker has a stable memory address at all. Session-02 uses controlled measurement to develop this intuition; a production stack budget needs target-specific worst-case analysis and margins.

### 5.1 Stack overflow is not the same as a buffer overflow

A **stack overflow** occurs when call-related storage exceeds the stack region or limit available to the program or task. Common causes include accidental infinite recursion, excessive recursion depth, large automatic arrays, and unexpectedly deep call paths. It can corrupt adjacent memory or cause a fault, and its symptom may appear far from the actual call that exhausted the stack.

A **buffer overflow** is an out-of-bounds read or write on one object, such as an array. It can occur on the stack, heap, or static storage. If a stack-local array overflows, it may corrupt the current frame and can resemble a stack overflow, but the root cause is an indexing error rather than total stack exhaustion.

```c
uint8_t samples[4] = {0U, 0U, 0U, 0U};
samples[4] = 1U; /* Buffer overflow: valid indexes are 0 through 3. */
```

The reliable pattern is the one established in M01-L01: carry capacity with a pointer or array, validate the index before access, and establish a maximum recursive depth before recursing. Do not deliberately provoke a real stack overflow to find a limit; it is a destructive and target-dependent experiment.

### 5.2 Diagnosing Stack Overflow vs. Bad Pointer

The two failures can both produce a crash, corrupted local state, or an incomplete backtrace. Distinguish them from evidence rather than from one symptom alone.

| Evidence | More consistent with stack overflow | More consistent with a bad pointer |
| --- | --- | --- |
| Call history | Repeated recursion or unexpectedly deep nested calls | Failure is not tied to increasing call depth. |
| Automatic storage | Large local arrays or reduced available stack make the failure more likely | Changing local storage has no consistent effect. |
| Stop location | Failure occurs while entering or returning through a deep path | GDB stops at a dereference or access using a null, stale, or out-of-range pointer. |
| Backtrace | Many repeated frames, or damaged/truncated frames after deep execution | A coherent call chain reaches the invalid pointer use. |
| Controlled correction | A lower approved depth or smaller automatic storage removes the failure | Correcting pointer lifetime, ownership, null validation, or bounds removes the failure. |

These are diagnostic clues, not universal proofs: corruption can damage a backtrace, and a buffer overwrite can create symptoms of either category. Start with a recorded build and input, capture `backtrace`, select the stopped frame, and inspect depth, automatic objects, and the pointer being dereferenced. Then make one controlled change at a time: reduce the configured call depth or automatic storage within the test guard, or correct the pointer's lifetime, ownership, null check, or bound. Compare the outcome with the original evidence before drawing a conclusion.

## 6. Dynamic Allocation: `malloc()`, `free()`, and Failure Modes

Dynamic allocation obtains storage while the program runs. It is useful when the required lifetime or capacity cannot be fixed at build time, but it creates a resource-management obligation: every successful allocation must have an owner, a defined release point, and a failure path. Some embedded projects prohibit or tightly restrict dynamic allocation because predictability is more important than flexibility; others use it under a documented policy. Follow the project's policy rather than assuming `malloc()` is always available or always appropriate.

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

    /* Read or process the block through p_buffer. */

    free(p_buffer);
    p_buffer = NULL;
    return true;
}
```

In C, `malloc()` returns `void *`, and C permits converting that result to an object-pointer type without an explicit cast. Its narrow purpose is to show the allocation boundary: check the result before use and release a successful allocation when its lifetime ends. An API that returns owned storage needs a validated output pointer and an explicit ownership contract. Never dereference the pointer after `free()`; assigning the local pointer to `NULL` prevents accidental reuse through that local name but cannot repair other aliases.

### 6.1 Out of memory, leaks, and fragmentation

**Out of memory (OOM)** means an allocation request cannot be satisfied. `malloc()` commonly reports this with `NULL`; a program must handle that outcome before it uses the pointer. A failure does not prove that all memory is consumed: the request may be too large, the allocator may have policy constraints, or free memory may not be available as one suitable block.

A **memory leak** occurs when allocated storage remains allocated after it is no longer needed, or after the program loses the ownership or reference needed to release it. A repeating error path that skips `free()` can slowly reduce available memory. This small flow shows allocation, an invalid-input path, and the cleanup that path requires:

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
        /* Returning here without free(p_buffer) would leak the allocation. */
        free(p_buffer);
        return false;
    }

    /* Process the valid input. */
    free(p_buffer);
    return true;
}
```

Both the invalid-input and success paths release the same owned allocation. Make ownership explicit: identify who releases each successful allocation, and ensure all normal and error paths preserve that rule.

**Fragmentation** is a loss of useful allocation capacity caused by free blocks being split into smaller noncontiguous pieces. A program can have sufficient total free memory yet fail a request that needs one contiguous block.

```text
Current allocation arena: [free 32 B] [used 64 B] [free 32 B]
Total free space: 64 B
New request:     48 B
Result:          cannot use the two 32-B blocks as one contiguous 48-B block
```

Long-running Linux services and embedded systems with bounded RAM both need to consider this risk. A fixed-size buffer, static allocation, or a bounded pool may be a more predictable design, but choosing those designs is a later architecture decision rather than a universal replacement for `malloc()`.

## 7. Null, Bad, and Dangling Pointers

A null pointer is a value that does not designate an object or function. It is useful as an explicit "no object" state, but dereferencing it is undefined behavior. M01-L01 established the basic rule: validate a pointer according to the API contract before dereferencing it.

Not every bad pointer is null. A pointer may be uninitialized, may point outside an object, may refer to an object whose lifetime ended, or may be the result of an invalid conversion. After `free(p_buffer)`, the old value is dangling; after a function returns, a pointer to one of its automatic locals is also dangling. Neither becomes safe merely because its numerical address still looks plausible.

```c
uint8_t *p_buffer = malloc(16U);

if (p_buffer != NULL)
{
    free(p_buffer);
    p_buffer = NULL;
}
```

This pattern makes the local variable safely represent "no current allocation." It does not validate an unrelated pointer, and it does not remove a dangling copy held elsewhere. Good API boundaries minimize aliases and make ownership clear.

When session-02 prints object addresses, use `%p` with an object pointer converted to `void *`, as required by `printf`. Do **not** treat conversion of a function pointer to `void *` as universally portable ISO C behavior. Use `nm` or `objdump` to investigate generated code symbols instead of relying on a function-address cast for the `.text` observation.

## 8. Inspect One Build with `size`, `nm`, and `objdump`

The GNU Binutils tools answer different questions. Use them after a successful build, with the exact binary, compiler options, linker configuration, and target recorded. Their output describes that artifact; it is not a general memory map for every C program.

### 8.1 `size`: a high-level footprint summary

```bash
size memory_demo
```

GNU `size` in its default Berkeley format prints `text`, `data`, and `bss` summary columns. Treat those labels as accounting categories, not literal section names. In particular, GNU Binutils documents that read-only data is counted in the Berkeley `text` column, not the `data` column. Therefore, a larger `text` value does not prove that only executable instructions grew.

Use `size` to compare the same program across deliberate build changes—for example, a baseline build and one built with `-Os`. Record the command and target, then ask what changed and verify with a more detailed tool if necessary. It is not evidence that an object has a particular address or that RAM and image usage are fully explained by three column names.

### 8.2 `nm`: symbols and their usual categories

```bash
nm -S memory_demo | grep -E 'g_sample_period_ms|g_samples_received'
```

On a conventional GNU target, `nm` type letters often help classify a symbol: `T` commonly denotes code, `R` read-only data, `D` initialized data, and `B` BSS. The GNU documentation qualifies those meanings as object-format and system dependent. Symbols can be local, stripped, merged, renamed, or removed by optimization, so absence is an observation requiring investigation, not proof that a source declaration has no storage.

Use `nm` to support the exercise's hypothesis that named static objects are represented where you expect in one build. Do not infer heap or stack placement from `nm`: those are runtime concepts rather than ordinary static symbols in the executable.

### 8.3 `objdump -h`: inspect actual section headers

```bash
objdump -h memory_demo
```

`objdump` displays information about object files; `-h` lists their section headers. It is the right next tool when the actual section names and sizes matter more than the three `size` summaries. Look for familiar names such as `.text`, `.rodata`, `.data`, and `.bss`, then compare them with your source declarations and the toolchain's conventions.

Do not use raw runtime-address differences as a portable measurement of segment distance. ISO C permits pointer subtraction only within an array object, not between unrelated global, heap, or stack objects. `uintptr_t` is optional; it exists only when the implementation provides a suitable unsigned integer type capable of representing converted object-pointer values. If a lab converts object addresses to `uintptr_t` on a supporting implementation to print an observed numeric difference, record it as a property of that one build and process. It can change across runs and is not a language-level ordering or capacity guarantee.

## 9. Basic GDB Workflow for a Crash or Deep Call Chain

GDB makes the conceptual call stack visible while a program is stopped. Build a diagnostic configuration with debug information; for an introductory investigation, `-g -O0` makes source-level stepping and local-variable inspection easier. This is a debugging configuration, not automatically the correct release configuration.

```text
$ gdb ./memory_demo
(gdb) break observe_depth
(gdb) run
(gdb) backtrace
(gdb) frame 1
(gdb) info locals
(gdb) print remaining_depth
```

- `run` starts the program under GDB.
- `backtrace` (also `bt` or `where`) lists the current frame and its callers. It answers: *how did execution get here?*
- `frame 1` selects a caller frame. `frame 0` returns to the innermost stopped function.
- `info locals` shows local variables accessible in the selected frame.
- `print expression` evaluates and displays a source-level expression, such as a depth variable or pointer value.

For a suspected stack failure, first capture the backtrace, select the relevant frame, inspect depth and local state, then compare the actual call path with the expected bound. For a bad-pointer failure, inspect the pointer value and the contract that should have established it. Do not assume an incomplete backtrace proves no stack problem: optimized code, corrupted call state, and missing debug information can limit what GDB can reconstruct.

## 10. Optimization Levels and What They Change

Optimization changes the generated program while preserving the C abstract machine's required observable behavior for defined programs. It does not promise identical timing, instruction count, code layout, stack-frame appearance, addresses, or debugger visibility. It may reduce code size, reduce execution time, increase either one, remove unused objects, inline functions, alter call structure, or make a local difficult to inspect. It can also expose existing undefined behavior because the compiler is entitled to assume the program obeys the language rules.

For GCC, use these levels as intent labels rather than a universal performance ranking:

| Option | Typical purpose | Debugging and measurement implication |
| --- | --- | --- |
| `-O0` | Disable most optimization passes; prioritize fast compilation and expected source-level debugging behavior. | Useful for first GDB investigations; not a release-performance measurement. |
| `-O1` | Enable a basic optimization set. | Compare generated size and runtime on the target; frame and local-variable views may already change. |
| `-O2` | Enable a broader set of optimizations, generally avoiding deliberate space–speed trade-offs. | A common production comparison point, but not proof of best result for every target. |
| `-O3` | Add more aggressive transformations, including additional loop-oriented work. | May improve a measured workload or increase code size; verify behavior, timing, and footprint. |
| `-Os` | Prefer transformations intended to reduce code size. | Smaller output can be valuable for constrained images, but measure speed and RAM effects too. |

The level number does not mean "always faster than the previous number." GCC enables a target- and configuration-dependent set of passes at each level, and a transformation can help one workload while harming another. Record compiler version and flags, use `size` to inspect output, and measure representative behavior on the intended target. Never use an optimized build's changed addresses or omitted locals as evidence that the source-level memory model changed.

## 11. Session-02 Preparation: What to Observe Safely

Session-02 has two learning goals. The Memory Segment Analyzer asks you to declare representative objects, inspect the result with `size` and `nm` or `objdump`, and compare observations. The Stack Depth Monitor asks you to control recursive depth before reaching a real overflow. The following connections explain why each requirement exists without providing the exercises' solutions.

| Session-02 requirement | Lesson preparation |
| --- | --- |
| Representative code, constant, initialized, zero-initialized, heap, and local objects | Classify their C semantics first, then inspect the particular build's sections and runtime behavior. |
| Address output and apparent distances | Print object pointers correctly, but treat addresses and numeric differences as one-process observations. |
| `size`, `nm`, and `objdump` verification | Use summaries for trends, symbols for named objects, and section headers for actual section inspection. |
| Recursion and a configurable guard | Bound depth before a real overflow; do not assume frame size, stack direction, or a fixed bytes-per-call value. |
| `malloc()` and `free()` | Check allocation before use, release every successful allocation, and avoid use after free. |
| Strict build and debug evidence | Build with the recorded compiler settings; use `-g -O0` for an initial GDB investigation, then compare with the intended build. |

The exercises are designed to produce evidence, not universal truths. Preserve the command output and state the compiler, target, options, and operating environment that produced it.

## 12. Common Failure Patterns and a Focused Debugging Order

| Symptom | First question | Useful first evidence |
| --- | --- | --- |
| Static data has an unexpected initial value | Did startup establish `.data` and BSS state before application code ran? | Startup/toolchain documentation and the built section report. |
| A recursive path crashes or behaves inconsistently | Is depth bounded, and what actual call chain reached the failure? | GDB `backtrace`, selected-frame locals, and a controlled guard. |
| `malloc()` failure causes a crash | Was `NULL` handled before the pointer was used? | The allocation result, requested size, and the failure path. |
| Memory use grows during repeated work | Does every successful allocation have a release owner on every path? | Allocation/release ownership review and repeatable workload evidence. |
| A buffer write causes a distant crash | Is an index inside the object's valid range? | Capacity, index, and the first out-of-bounds access. |
| Footprint changes unexpectedly after a build flag change | Which sections or symbols changed in this exact artifact? | `size`, `nm`, `objdump -h`, compiler version, and flags. |

Start with the smallest reliable observation: reproduce with the recorded build, identify the failing function or input, inspect the call chain and relevant data, then confirm the physical artifact with the appropriate tool. Do not jump from one numerical address or one `size` column to a conclusion about all targets.

## 13. Key Takeaways

- `.text`, `.rodata`, `.data`, and `.bss` are useful toolchain conventions, not physical placements mandated by ISO C.
- Nonzero initialized writable static data commonly needs image bytes and startup copying to RAM; BSS-like data commonly needs RAM plus startup zeroing. Explicit zero initialization is often placed in BSS, but that remains a toolchain decision.
- `const` data is commonly emitted into a read-only section; it is not an ISO C guarantee of physical placement.
- Heap and stack are runtime-memory concepts. Frame layout, stack direction, object addresses, and apparent inter-region distances are not portable guarantees.
- Bound recursion and validate every array access. A stack overflow and a buffer overflow can both corrupt memory, but they have different root causes.
- Check `malloc()` results, define ownership, free successful allocations, and design for OOM and fragmentation where dynamic allocation is permitted.
- Use `size` for a footprint summary, `nm` for named symbols, and `objdump -h` when actual section inspection matters. GNU `size` Berkeley columns are summaries, and read-only data may be counted under `text`.
- Use GDB's `run`, `backtrace`, `frame`, `info locals`, and `print` to turn a crash into a bounded investigation.
- Treat GCC optimization levels as build choices to measure on the intended compiler and target, never as a universal speed ranking.

## 14. Further Reading

- [GNU Binutils `size` documentation](https://sourceware.org/binutils/docs/binutils/size.html)
- [GNU Binutils `nm` documentation](https://sourceware.org/binutils/docs/binutils/nm.html)
- [GNU Binutils `objdump` documentation](https://sourceware.org/binutils/docs/binutils/objdump.html)
- [GNU GDB: Running Programs](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Running.html), [Backtraces](https://sourceware.org/gdb/current/onlinedocs/gdb.html/Backtrace.html), and [Frame Information](https://sourceware.org/gdb/current/onlinedocs/gdb/Frame-Info.html)
- [GCC Optimize Options](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [CMSIS-Core startup-file documentation](https://arm-software.github.io/CMSIS_6/latest/Core/startup_c_pg.html)
- [Zephyr early boot sequence](https://docs.zephyrproject.org/latest/hardware/porting/arch.html)
