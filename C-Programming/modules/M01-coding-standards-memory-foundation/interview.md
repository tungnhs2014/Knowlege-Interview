# M01 Interview — Coding Standards and Memory Foundation

> **Status:** `DRAFT — HUMAN_REVIEW_PENDING`
>
> **Gate:** `INTERVIEW_AUDIT`
>
> **Language baseline:** ISO C99

## Review Boundary

These questions assess the M01 scope defined by DevLinux Session 01 and Session 02:

- C standards, MISRA C, BARR-C, and compiler diagnostics;
- fixed-width types, scope, linkage, storage duration, and API contracts;
- undefined and unspecified behavior;
- common executable-memory categories and startup behavior;
- stack, heap, memory failures, GDB, binary-inspection tools, and optimization;
- basic Makefile and Doxygen practices.

Answers must distinguish:

```text
ISO C semantics
from
compiler, linker, ABI, loader, operating-system, and target behavior
```

A clean compiler or analyzer result is useful evidence, but it is not by itself proof of functional correctness, portability, target safety, or MISRA compliance.

---

## Q1. Why do Embedded C projects use ISO C, MISRA C, BARR-C, project rules, and compiler diagnostics? How do their roles differ?

### Expected Answer

- ISO C defines the portable language semantics and standard-library contract. It does not define a project-specific coding policy.
- MISRA C:2012 restricts risky C usage for critical or high-integrity projects and defines a compliance model involving scope, checking, and deviations.
- BARR-C:2018 supplies practical Embedded C style and defect-prevention guidance that improves readability, reviewability, and maintainability.
- Project rules select the language version, applicable guideline subset, target restrictions, approved extensions, tools, and evidence required for that product.
- Compiler diagnostics report suspicious or nonconforming constructs for one configured compiler, version, target, language mode, and option set.
- `-Wall` enables a useful compiler-defined warning group; it does not mean every possible warning is enabled.
- A warning-free build does not prove functional correctness, target correctness, portability, or MISRA compliance.

### Strong Interview Answer

“ISO C tells me what the language means. MISRA constrains risky language use within a project compliance process, while BARR-C makes the code consistent and reviewable. The project then defines the actual baseline, target restrictions, allowed extensions, tools, and evidence. Compiler warnings provide early feedback for one configuration, but they are not a compliance certificate. I still verify the API contract, target behavior, tests, and review evidence.”

---

## Q2. How is `uint32_t` different from `unsigned int`, and when should each type be used?

### Expected Answer

- `uint32_t` is an exact-width unsigned integer type with exactly 32 value bits and no padding bits.
- It is available only when the implementation provides a suitable type.
- `unsigned int` is a native unsigned integer type whose width and range are implementation-defined within ISO C requirements.
- Use `uint32_t` when exactly 32 bits are part of a protocol, register abstraction, binary format, serialized representation, or documented API contract.
- Use a native type such as `int` or `unsigned int` for ordinary local arithmetic when no exact external representation is required and its range is sufficient.
- Use `size_t` for object sizes, array capacities, and values produced by `sizeof`.
- Fixed-width types do not establish byte order, validate input, or make narrowing conversions safe.
- Validate the source range before converting to a narrower type.

### Strong Interview Answer

“`uint32_t` communicates that exactly 32 bits are part of the contract, so I use it for protocol fields or APIs that require that representation. `unsigned int` is a native type and its width depends on the implementation, so it is often suitable for ordinary arithmetic when exact width is not contractual. I also verify that the target provides `uint32_t`, and I still prove ranges before conversions.”

---

## Q3. Explain the differences among an automatic local, a static local, a file-scope `static` object, an `extern` declaration, and `register`.

### Expected Answer

- An ordinary local object has block scope, no linkage, and automatic storage duration. A new instance is created for each active block execution.
- A static local object has block scope, no linkage, and static storage duration. One object persists for the entire program execution while its name remains local to the function or block.
- A file-scope `static` object has file scope, internal linkage, and static storage duration. It is private to one translation unit.
- `extern int value;` is normally a declaration referring to an externally linked definition provided elsewhere; it does not allocate a separate object in every translation unit.
- `extern int value = 1;` is a definition because it has an initializer.
- An object with external linkage still has static storage duration; linkage describes name association across translation units, not lifetime.
- `register` is a historical optimization hint for an automatic object. It does not guarantee placement in a CPU register, and C does not permit applying `&` to an object declared `register`.
- Scope, linkage, and storage duration are separate concepts and should be reviewed separately.

### Strong Interview Answer

“An automatic local is per active call. A static local persists across calls but can only be named inside its block. A file-scope static is persistent module-private state with internal linkage. An `extern` declaration usually refers to one definition owned elsewhere, while an initialized `extern` declaration is itself a definition. `register` is only a historical hint and prevents taking the object’s address; modern code normally uses a clear ordinary local and lets the compiler optimize.”

---

## Q4. How do undefined, unspecified, implementation-defined, and build-specific behavior differ in Embedded C?

### Expected Answer

- Undefined behavior means ISO C imposes no requirements on program behavior when execution reaches that construct.
- Common examples include dereferencing a null pointer, accessing outside an object’s bounds, using an invalid object lifetime, and signed integer overflow.
- Undefined behavior is not a normal error result that production code can safely recover from.
- Unspecified behavior means ISO C permits multiple valid outcomes and does not require the implementation to document which one occurs.
- Function-argument evaluation order is often unspecified when the order itself does not create undefined behavior. Correctness must not depend on one permitted order.
- Under the C99 sequence-point model, modifying a scalar object more than once between sequence points, or modifying it and also reading it for an unrelated purpose, can produce undefined behavior.
- Implementation-defined behavior is an ISO C category in which the implementation selects one permitted behavior and documents that choice. Sizes or ranges of some native integer types are examples; it must not be confused with undefined behavior.
- A build-specific observation is not an ISO C behavior category. It depends on the exact compiler and version, target, ABI, optimization options, linker configuration, runtime, and binary.
- Symbol addresses, one-run stack direction, section names, code size, and `nm` or `objdump` classifications are build-specific evidence, not portable C guarantees.
- Compiler changes, optimization, input data, memory layout, or target differences can expose the same undefined behavior differently.
- Prevent it through explicit contracts, pointer and lifetime checks, bounds validation, range proofs, and simple expressions with visible side effects.

### Strong Interview Answer

“Undefined behavior means ISO C imposes no requirements, so I remove the defect rather than treating it as recoverable. Unspecified behavior permits several outcomes without requiring the implementation to document one, so my code must work for every permitted outcome. Implementation-defined behavior permits a documented implementation choice, so I read and record the relevant compiler or target documentation. Build-specific observations such as addresses, section names, code size, and symbol classifications are not ISO C categories; they are evidence only for the recorded toolchain, target, options, and binary. In C99 I keep side effects in simple statements to avoid sequence-point violations.”

---

## Q5. Given `int a = 0;` and `int b = 10;` at file scope, where are these objects commonly represented, and why are BSS-like and initialized-data storage treated differently?

### Expected Answer

- Both objects have static storage duration and must have their required initial values before ordinary program execution.
- In a common bare-metal embedded build, `b` is represented in initialized writable data because its nonzero initial value must be available in the load image and copied or otherwise established in writable runtime storage.
- `a` is commonly represented in BSS-like storage because its zero value can be established by clearing a runtime storage range.
- An explicit zero initializer does not require a separate initialized-data representation.
- BSS-like storage still consumes runtime RAM.
- These section names, copy operations, physical addresses, and startup mechanisms are compiler, linker, executable-format, loader, and target conventions, not ISO C requirements.
- Hosted Linux loading is different from a microcontroller startup copy-and-clear model, even when both artifacts use ELF.

### Strong Interview Answer

“Both objects live for the whole execution. On a typical bare-metal build, `b = 10` needs nonzero initialization content and writable runtime storage, so it is commonly initialized data. `a = 0` is commonly BSS-like: runtime storage is reserved and startup clears it. BSS reduces equivalent zero payload; it does not eliminate RAM use. I verify the exact binary, linker configuration, and target instead of claiming ISO C mandates `.data` or `.bss`.”

---

## Q6. Why can BSS-like storage consume runtime RAM while adding little or no equivalent zero payload to a firmware image?

### Expected Answer

- Zero-initialized static-duration objects still require runtime storage for their full lifetime.
- A bare-metal startup implementation can clear one linker-defined RAM range instead of storing one zero byte in the image for every runtime byte.
- In an ELF hosted environment, a loadable segment can have a larger in-memory size than file-backed size, allowing the loader to provide zero-filled memory without equivalent bytes in the executable file.
- The image or executable still needs metadata or boundaries describing the runtime range.
- Therefore, BSS-like storage can reduce image payload while preserving the full runtime RAM requirement.
- Exact behavior depends on the executable format, linker, startup code, loader, and target.

### Strong Interview Answer

“BSS saves equivalent initialization payload, not runtime capacity. The object still occupies RAM. Bare-metal startup commonly clears a linker-defined range, while an ELF loader can create zero-filled memory beyond the file-backed bytes of a segment. The mechanism differs, but the engineering conclusion is the same: account for both image size and runtime RAM.”

---

## Q7. Where is a `const` global commonly placed, and why does ISO C not guarantee `.rodata` or physical Flash placement?

### Expected Answer

- A `const` global is commonly emitted into read-only program-image storage or a section such as `.rodata`.
- `const` is a type qualifier that prevents modification through a const-qualified lvalue; it is not a section-placement directive.
- If an object is defined with a const-qualified type, casting away `const` and attempting to modify that object produces undefined behavior.
- The compiler may eliminate an unused object, merge constants, use a different section name, or apply target-specific placement.
- The linker and target memory map determine the final load and execution locations.
- Verify physical placement with the exact object, linker map, section headers, symbols, and target documentation.

### Strong Interview Answer

“I commonly expect a const global in read-only program storage, but the source keyword alone does not guarantee `.rodata` or Flash. ISO C defines type semantics, not physical placement. If placement is a product requirement, I verify the exact linked artifact and target memory map.”

---

## Q8. Compare automatic storage, static storage, and dynamic allocation. What practical risks matter in embedded systems?

### Expected Answer

- Automatic objects have a lifetime tied to block execution. Implementations commonly use stack-oriented call storage, but ISO C does not define a universal stack layout or direction.
- Static-duration objects exist for the entire program execution. They provide predictable lifetime and fixed capacity but permanently consume their memory budget.
- A static local has static duration while retaining block scope.
- Dynamic storage is requested at runtime through an allocator when the implementation supplies one. It remains allocated until released or until the execution environment ends.
- Large automatic objects, deep call chains, recursion, and interrupt nesting can exhaust stack capacity.
- Dynamic allocation can fail, leak, fragment, and have allocator-dependent timing.
- Static state can create hidden coupling, non-reentrancy, and concurrency problems when ownership is unclear.
- Choose storage from lifetime, capacity, ownership, timing, concurrency, and failure requirements rather than from a slogan.

### Strong Interview Answer

“Automatic storage is naturally tied to one invocation, static storage has a program-long lifetime, and dynamic storage has a runtime-controlled lifetime. Each has a budget and a failure model. I review maximum call depth and local size for stack use, ownership and OOM handling for heap use, and coupling or concurrency for persistent static state.”

---

## Q9. A recursive program crashes. How would you distinguish stack exhaustion from a bad pointer or buffer corruption using the basic GDB workflow taught in M01?

### Expected Answer

- Reproduce the failure with the same compiler, target, flags, binary, and input.
- Prefer a diagnostic build with debug information; recognize that optimization changes source-level visibility.
- Use `run` to reproduce the failure and `backtrace` or `where` to capture the call chain.
- Repeated recursive frames, excessive depth, or large automatic objects make stack exhaustion more plausible, but they are clues rather than proof.
- Use `frame`, `info locals`, `info args`, and `print` to inspect depth state, local objects, parameters, and the pointer used at the fault.
- A fault at a null, dangling, or out-of-range dereference points toward pointer or lifetime failure.
- An out-of-bounds write can corrupt call-related storage and later resemble stack exhaustion or a bad return address.
- Make one controlled correction at a time and reproduce again.
- Do not infer stack direction, exact frame size, or remaining stack capacity from a few address values.

### Strong Interview Answer

“I first reproduce the exact binary and capture a backtrace instead of diagnosing from the signal alone. Repeated recursive frames suggest stack pressure; a faulting null or stale dereference suggests a pointer defect. Then I inspect the relevant frame, locals, arguments, and pointer values. A buffer overwrite can imitate either failure, so I treat the backtrace as evidence for a hypothesis, not final proof.”

### Follow-up

**What are common causes of segmentation faults on Embedded Linux?**

Null or dangling-pointer dereference, out-of-bounds access, use-after-free, stack exhaustion, stack-buffer corruption, and corrupted function or data pointers are common causes. The signal identifies an invalid memory access, not the root cause.

---

## Q10. Explain the difference among a memory leak, out-of-memory, and fragmentation. Why may long-running embedded systems restrict dynamic allocation?

### Expected Answer

- A memory leak is allocated storage that remains owned after it is no longer needed, or whose last usable reference was lost before release.
- Out-of-memory means the allocator cannot satisfy a request. `malloc()` reports failure with `NULL`, which must be checked before dereference.
- Fragmentation is allocator-dependent loss of usable capacity because free storage is divided or organized in a form unsuitable for a requested allocation.
- A request can fail even when the reported total free memory appears large enough.
- Every successful allocation needs an owner and a release path for normal completion and every error path.
- Checking `malloc()` for `NULL` improves runtime handling but does not make a design using dynamic allocation compliant with MISRA C:2012 Directive 4.12.
- A MISRA compliance claim for a design that uses dynamic allocation requires the project’s formal authorized treatment.
- Long-running systems may restrict dynamic allocation to improve capacity, timing, and failure predictability.

### Strong Interview Answer

“A leak consumes capacity over time because storage is never released. OOM is a failed request now, while fragmentation means available storage cannot satisfy the requested shape or size. In firmware, those risks can make timing and capacity unpredictable, so I use dynamic allocation only under a documented policy with ownership, failure handling, cleanup, and any required deviation process.”

---

## Q11. What do `size`, `nm`, `objdump`, `readelf`, and a linker map provide, and what can they not prove by themselves?

### Expected Answer

- `size` gives a high-level footprint summary. GNU Berkeley `text`, `data`, and `bss` columns are accounting categories, not guaranteed literal section names.
- `nm` lists symbols and build-specific classifications. Letters such as `T`, `R`, `D`, and `B` depend on the object format, platform, optimization, stripping, merging, and garbage collection.
- `objdump -h` inspects section headers for the selected object.
- `readelf -S` and `readelf -l` inspect ELF sections and loadable program segments; ELF is not universal.
- A linker map shows how input sections and symbols contributed to final linked regions and is often the strongest link-allocation evidence.
- These tools do not by themselves prove runtime heap consumption, stack high-water use, memory leaks, physical board placement, or portable C semantics.
- Run them on the exact binary built with the recorded compiler, target, flags, linker configuration, and optimization level.

### Strong Interview Answer

“I use `size` for a quick footprint trend, `nm` for named symbols, `objdump` or `readelf` for structural detail, and the linker map for final link allocation. Each result describes one artifact and one build context. I do not use static binary tools to claim runtime heap or stack usage, a leak, or an ISO C placement rule.”

---

## Q12. What are the practical differences among `-O0`, `-O1`, `-O2`, `-O3`, and `-Os`? Why can debugging and observed stack or local behavior change?

### Expected Answer

- Optimization options select compiler-defined transformation sets and priorities. Their exact contents depend on the compiler and version.
- `-O0` disables most optimization and is commonly easier for initial source-level debugging.
- `-O1`, `-O2`, and `-O3` enable progressively different compiler-selected optimization groups, but a higher number is not guaranteed to be faster or better for every program or target.
- `-Os` prioritizes code-size reduction and commonly enables optimizations considered suitable for that goal.
- For a defined C program, optimization must preserve required observable behavior, not identical instructions, addresses, timing, code layout, stack frames, or debugger presentation.
- Inlining, tail-call transformation, register allocation, constant propagation, elimination, and code motion can change the visible call chain and local-variable availability.
- Undefined behavior may appear differently under optimization because the language imposes no required result.
- Compare code size, timing, RAM effects, and functional behavior on the intended compiler and target.

### Strong Interview Answer

“Optimization levels are compiler configurations, not a universal quality ranking. `-O0` is often easier to debug, `-O2` is a common production comparison point, `-O3` may trade size for more aggressive transformations, and `-Os` prioritizes size. Optimized code can inline calls, remove locals, or keep values in registers while preserving defined observable behavior. I measure the actual target instead of assuming the option name predicts the result.”

---

## Q13. Why use a Makefile and Doxygen in a small Embedded C module, and what do they not prove?

### Expected Answer

- A Makefile records a repeatable build relationship among source files, compiler, language mode, warning policy, dependencies, output, and cleanup.
- Basic targets such as `all` and `clean` make the lab reproducible and remove generated artifacts.
- Make decides whether commands need to run from declared dependencies and timestamps; it does not understand C semantics or prove a correct dependency graph automatically.
- Doxygen-style documentation records public API purpose, parameters, return values, valid input, failure behavior, output changes, and implementation assumptions.
- Documentation must describe the actual implementation and caller contract; generated HTML does not make an undocumented or incorrect API safe.
- Neither Make nor Doxygen proves functional correctness, MISRA compliance, runtime safety, or target suitability.

### Strong Interview Answer

“A Makefile turns the build command into a reproducible project artifact, and Doxygen turns the public API contract into reviewable documentation. They reduce manual variation and maintenance cost, but they are not correctness tools by themselves. I still verify dependencies, compiler configuration, implementation behavior, tests, and target evidence.”

---

## Interview Review Checklist

A strong answer should:

- separate ISO C requirements from common toolchain or target behavior;
- state assumptions and implementation dependencies;
- avoid claiming that warnings or selected rules establish complete MISRA compliance;
- distinguish image payload from runtime memory;
- avoid assuming a universal stack direction or frame size;
- treat `size`, `nm`, debugger output, and optimization results as evidence for one recorded build;
- explain failure mechanisms using contracts, ownership, range, bounds, and lifetime;
- remain concise enough to deliver verbally while still giving the interviewer a correct engineering model.

## Source Basis

- DevLinux Week 1, Session 01, and Session 02 define the M01 interview scope and teaching order.
- ISO C99 defines the language semantics.
- MISRA C:2012 defines its guideline and compliance model.
- BARR-C:2018 supplies the relevant Embedded C style and maintainability guidance.
- GNU GCC, Make, Binutils, GDB, Doxygen, ELF, linker, runtime, and target documentation define tool-specific behavior.
- `Full-Embedded-C-Notes.md` is discovery material only and is not authoritative for placement, startup, stack, or compliance claims.
