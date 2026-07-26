# M01 Interview — Coding Standards and Memory Foundation

> **Status:** INTERVIEW_APPROVED.

## Q1. Why do Embedded C projects use ISO C language rules, MISRA C, BARR-C, and compiler warnings? How do their roles differ?

### Expected Answer

- ISO C defines the portable language semantics and standard-library contract; it does not define a project coding policy.
- MISRA C:2012 constrains risky C usage for high-integrity work and needs a defined scope and compliance process.
- BARR-C:2018 provides practical Embedded C style and defect-prevention conventions that make code easier to review and maintain.
- Compiler warnings report suspicious constructs for ongit e compiler, version, target, and option set; `-Wall` is useful but is not every warning.
- A warning-free build is valuable evidence, but it does not prove functional correctness, target correctness, or MISRA compliance.

### Strong Interview Answer

“I start with ISO C because it defines what the code means. MISRA narrows risky language usage for a safety-oriented project, while BARR-C makes the resulting code consistent and reviewable. Compiler warnings are an early feedback mechanism, not a compliance certificate. I use all four together: language semantics, project rules, readable style, and diagnostics, then still verify the API contract and target behavior.”

## Q2. How is `uint32_t` different from `unsigned int`, and when should each kind of type be used?

### Expected Answer

- `uint32_t` is an exact-width unsigned type and exists only when the implementation supplies a compatible 32-bit type without padding bits.
- `unsigned int` is a native unsigned integer type whose width and range are implementation-dependent within ISO C limits.
- Use `uint32_t` when a protocol field, binary format, serialized representation, or documented API requires exactly 32 bits.
- Use a native type such as `int` or `unsigned int` for local arithmetic when an exact external representation is not part of the contract.
- Type selection does not remove the need for range validation before narrowing or conversion.

### Strong Interview Answer

“`uint32_t` communicates an exact 32-bit representation, so I use it when that width is part of an external contract, such as a protocol value. `unsigned int` is appropriate for ordinary native arithmetic when the exact width is not contractual. I also confirm that the target supplies `uint32_t`; the typedef is not guaranteed on every conceivable C implementation.”

## Q3. Explain the differences among a static local variable, a file-scope static object, and an `extern` declaration. Cover scope, linkage, and storage duration.

### Expected Answer

- A static local has block scope, no linkage, and static storage duration; one object persists across calls while its name remains local to the function.
- A file-scope `static` object has file scope, internal linkage, and static storage duration; it is private to one translation unit.
- An `extern` declaration normally declares an entity defined elsewhere; it does not by itself create a separate object definition.
- An externally linked object has static storage duration, while the scope of an `extern` declaration depends on where that declaration appears.
- These are distinct C concepts; informal “global” or “local memory” labels do not explain their visibility or lifetime precisely.

### Strong Interview Answer

“A static local is persistent state that only one function can name. A file-scope static is persistent module-private state with internal linkage. An `extern` declaration lets another translation unit refer to one external definition. I review scope, linkage, and storage duration separately, because two objects can both live for the program duration but have very different visibility and ownership.”

## Q4. What is undefined behavior, how is it different from unspecified behavior, and why is undefined behavior especially dangerous in embedded software?

### Expected Answer

- Undefined behavior means ISO C imposes no requirements after the construct is evaluated; null dereference, out-of-bounds access, and signed overflow are common examples.
- It is not merely an unexpected output or an error code that production code can safely recover from.
- Unspecified behavior permits more than one valid result without requiring the implementation to document which one occurs, such as ordinary function-argument evaluation order when order does not matter.
- Unsequenced conflicting side effects on the same scalar object can be undefined behavior, not just unspecified ordering.
- Embedded failures can be intermittent because a compiler change, optimization level, input, or target can expose undefined behavior differently.

### Strong Interview Answer

“Undefined behavior is more serious than an unspecified result: the C standard no longer constrains what happens. In firmware that can become a field-only failure after an optimization or toolchain change. I prevent it by validating pointers and bounds, checking ranges before arithmetic or conversion, and splitting side effects into clear statements. Unspecified argument order is different: it is acceptable only when the order cannot affect the result.”

## Q5. Given `int a = 0;` and `int b = 10;` at file scope, where are these static-duration objects commonly represented in an embedded build, and why are `.bss` and `.data` treated differently at startup?

### Expected Answer

- Both objects have static storage duration and must have their required initial values before normal C execution begins.
- In a common embedded build, `b` is represented in `.data`-like writable storage because its nonzero initial value must be available for startup copying into RAM.
- `a` is commonly represented in `.bss`-like storage because its all-zero initial state can be established by startup zeroing; an explicit zero initializer does not force that placement.
- The startup sequence and physical sections are toolchain, linker, and target conventions, not ISO C requirements.
- The practical distinction is image initial-value content plus RAM for `.data`, versus RAM plus zeroing work for `.bss`.

### Strong Interview Answer

“At file scope, both variables live for the full program execution. On a typical embedded target, `b = 10` needs initial-value bytes in the image and writable RAM at runtime, so it is commonly `.data`. `a = 0` is commonly BSS-like: RAM is reserved and startup clears it. I would call this a build convention, then verify the actual artifact rather than claim ISO C mandates those sections.”

## Q6. Why can `.bss` consume runtime RAM while adding little or no equivalent zero payload to a firmware image?

### Expected Answer

- BSS-like objects still require writable RAM for their entire static-storage lifetime.
- Their required initial value is zero, so embedded startup code can clear a RAM range instead of storing an equivalent sequence of zero bytes in the image.
- Startup must know the BSS RAM range to clear, commonly through boundaries or equivalent information provided by the linker/startup environment; this does not require one zero byte in the firmware image for every BSS byte.
- This can reduce image payload without removing the RAM requirement.
- Exact placement and startup implementation remain toolchain and target dependent.

### Strong Interview Answer

“BSS saves image payload, not runtime RAM. The firmware still reserves RAM for the object, but startup can zero the range rather than carry one image byte for every initial zero. That is why I consider both Flash/image size and RAM size when reviewing a memory report.”

## Q7. Where is a `const` global commonly placed, and why must an engineer not claim that ISO C guarantees physical `.rodata` or Flash placement?

### Expected Answer

- A `const` global is commonly emitted into a read-only section such as `.rodata`, often in program-image storage on embedded targets.
- `const` is a C type qualification that restricts modification through that qualified lvalue; it is not a section-placement directive.
- Casting away `const` and attempting to modify an object defined with a const-qualified type has undefined behavior.
- A compiler and linker may merge, relocate, eliminate, or name sections differently according to the build and target.

### Strong Interview Answer

“I would expect a const global to be in read-only program storage in many embedded builds, but I would not promise `.rodata` or Flash from the source alone. ISO C defines the object’s type semantics, not its physical address. If placement matters for a product, I inspect that exact build and its toolchain output.”

## Q8. Compare stack, heap, automatic local storage, and static-local lifetime. What practical risks matter in embedded systems?

### Expected Answer

- Automatic locals exist for a block or function invocation; many implementations use stack-oriented call storage, but the exact layout and direction are implementation dependent.
- A static local retains one object for the entire program execution while keeping block scope inside its function.
- Heap storage is obtained dynamically when the implementation provides an allocator and remains allocated until it is released.
- Deep recursion and large automatic objects can exhaust a constrained stack; heap use can fail, leak, or fragment and may have variable timing.
- Static storage is predictable in lifetime but consumes a fixed budget and may introduce hidden persistent state if its ownership is unclear.

### Strong Interview Answer

“Automatic locals are per call, while a static local persists across calls but remains function-private. Heap storage has a dynamic lifetime controlled by allocation and release. In embedded work I budget all three: deep calls and large locals threaten stack capacity, while heap use needs an OOM and ownership strategy. I do not assume a universal stack direction or frame size from one run.”

## Q9. A recursive program crashes. How would you distinguish stack overflow from a bad/dangling pointer or buffer corruption using the basic GDB workflow taught in M01?

### Expected Answer

- Reproduce the fault with the recorded build and input, then use `run` and `backtrace` to capture the active call chain.
- Repeated recursive frames, excessive depth, or large automatic objects make stack exhaustion more plausible; they are clues, not proof.
- Use `frame`, `info locals`, and `print` to inspect the selected frame’s depth, local state, and any pointer used at the stop location.
- A crash directly at a null, stale, or out-of-range dereference points toward a bad pointer; an out-of-bounds write can corrupt a frame and resemble either failure.
- Make one controlled correction at a time: lower the approved recursion depth or automatic storage, or repair pointer lifetime, null validation, ownership, or bounds.
- Do not infer stack direction, frame size, or a universal diagnosis from address values or an incomplete backtrace.

### Strong Interview Answer

“I first capture a backtrace rather than guessing from the crash. Repeated frames and growing call depth suggest stack pressure; a stop at a null or stale-pointer dereference suggests a pointer defect. I inspect the relevant frame and locals, then change one variable at a time—such as a bounded recursion limit or a pointer-lifetime fix. A buffer overwrite can imitate either symptom, so the evidence is diagnostic, not conclusive.”

### Follow-up

**What are common causes of segmentation faults on Embedded Linux?**

Null or dangling-pointer dereference, out-of-bounds access, and corrupted control or data objects are common causes. A stack-exhaustion or stack-buffer corruption failure can also surface as a segmentation fault. Start with the crash location and basic GDB call chain; do not classify the cause solely from the signal.

## Q10. Explain the difference among a memory leak, out-of-memory, and fragmentation. Why may long-running embedded systems restrict dynamic allocation?

### Expected Answer

- A leak is storage that remains allocated after it is no longer needed, or whose ownership/reference was lost before release.
- Out-of-memory means an allocation request cannot be satisfied; `malloc()` commonly reports it with `NULL` and the result must be checked before use.
- Fragmentation means free memory is divided into unsuitable noncontiguous blocks, so a request can fail even when total free memory appears sufficient.
- Every successful allocation needs a clear owner and a release path for both normal and error outcomes.
- Long-running embedded systems may restrict dynamic allocation to improve predictability of capacity, timing, and failure behavior.

### Strong Interview Answer

“A leak consumes capacity over time because allocated storage is never released. OOM is a failed request now, while fragmentation means the total free space may exist but not as one suitable block. In long-running firmware those risks make timing and capacity less predictable, so I use dynamic allocation only under a clear policy with ownership, null handling, and cleanup on every path.”

## Q11. What do `size`, `nm`, and `objdump` provide, and how would you use them when investigating an embedded executable?

### Expected Answer

- `size` provides a footprint summary; GNU Berkeley `text`, `data`, and `bss` fields are accounting categories, not necessarily literal section names.
- `nm` lists symbols and their classifications for the specific binary; common letters such as `T`, `R`, `D`, and `B` are build and object-format observations.
- `objdump -h` can inspect actual object or section-header information when the summary is not detailed enough.
- Run the tools on the exact binary built with the compiler, flags, linker configuration, and target under investigation.
- Use the results to form and verify a build-specific hypothesis, not to claim an ISO C placement rule or infer heap/stack placement from static symbols.

### Strong Interview Answer

“I use `size` first for a footprint trend, but I do not read its Berkeley columns as literal section names. I use `nm` to inspect named symbols in that binary, then `objdump -h` if I need section-header detail. The result is evidence about one build, so I record the build context and avoid turning it into a language-level memory-map claim.”

## Q12. What are the practical differences among `-O0`, `-O2`, and `-Os`? Why can source-level debugging and observed stack/local behavior change under optimization?

### Expected Answer

- `-O0` is commonly useful for initial source-level debugging, while `-O2` enables a broader optimization set and `-Os` prefers code-size reduction.
- The option names express compiler intent; they are not a universal ranking where a higher level is always faster or better.
- For defined programs, optimization preserves the C abstract machine’s required observable behavior, not identical instructions, timing, code layout, addresses, or frame appearance.
- Inlining, register allocation, elimination, and call transformation can change debugger visibility and apparent local or stack behavior.
- Compare code size, timing, RAM effect, and functional behavior on the intended compiler and target rather than trusting an option number.

### Strong Interview Answer

“I use `-O0` to make an initial debugging session easier, `-O2` as a common production comparison point, and `-Os` when code size is important. None is automatically best. Optimized code can inline calls, move locals into registers, or change frame appearance while preserving required behavior for defined code, so I measure the actual target and do not treat changed addresses or missing locals as a source-level defect by themselves.”
