# Topic Brief 03 - C Memory Model

## 1. Canonical Routing

| Field | Validated value |
| --- | --- |
| `learning_path_number` | `03` |
| Title | C Memory Model |
| `slug` | `c-memory-model` |
| Requested topic | C objects, storage duration, typical executable memory layout, stack frames, dynamic allocation, alignment, padding, endianness, undefined behavior, memory failures, and debugging |
| Master source | `master-ch03` |
| Required Notion sources | `notion-4-1`, `notion-4-2`, `notion-3-2` |
| Topic Brief | `coverage/topic-briefs/03-c-memory-model.md` |
| Knowledge target | `knowledge/03-c-memory-model.md` |
| Interview target | `interview/03-c-memory-model.md` |
| Example target | `examples/03-c-memory-model/README.md` |

Validation result: the number, title, slug, master source, required Notion
sources, and all canonical output paths match `LEARNING_PATH.md`.

## 2. Source Coverage

### Internal Sources Read

| Source label | Path | Coverage contribution |
| --- | --- | --- |
| `master-ch03` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, CH03 | `MUST / Deep` priority, CH02 prerequisite, keyword scope, debugging requirement, and interview focus |
| `master-ch17` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, CH17 memory comparisons | Required allocation, stack/heap, and manual-cleanup comparisons |
| `master-ch19` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, memory checklist | Enterprise and interview review points |
| `guide-section-03` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 3 | MUST-depth and controlled EXPERT-depth requirements |
| `guide-section-04` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 4 | Full-chapter structure and usage angles |
| `guide-section-06` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 6 | Required C/C++ comparison format |
| `guide-section-07` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 7 | Depth control and practical-example guidance |
| `guide-section-09` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 9 | Trusted C, safety, embedded, and enterprise source routing |
| `guide-section-10` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 10 | Technical-English, Markdown, and compile-oriented example rules |
| `guide-section-11` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 11 | Full lesson, interview pack, review guide, and comparison expectations |
| `guide-section-12` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 12 | Required allocation and cleanup comparison inventory |
| `guide-section-14` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 14 | Final quality checklist |
| `notion-index` | `docs/C++ Notion/C++ Programming.md` | Notion chapter inventory and source identity validation |
| `notion-4-1` | `docs/C++ Notion/Chapter 4-1 Dynamic Memory Basics.md` | Typical process segments, stack/heap comparison, dynamic arrays, contiguous versus fragmented layouts, and allocation failure concepts |
| `notion-4-2` | `docs/C++ Notion/Chapter 4-2 Advanced Memory Management.md` | `malloc`/`calloc`/`realloc`/`free`, allocation-family mismatch, leaks, dangling pointers, double free, wild pointers, ownership practices, and memory debugging tools |
| `notion-3-2` | `docs/C++ Notion/Chapter 3-2 Pointers in C++.md` | Address/dereference mental model, null and invalid pointers, pointer arithmetic, arrays, `void *`, pointer lifetime bugs, and interview prompts |

All three mapped Notion chapter files were read in full. No mapped Notion
source was skipped.

### External References Consulted

| Label | Reference | Validation purpose |
| --- | --- | --- |
| `external-iso-c` | WG14 N3096, public C23 draft: <https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3096.pdf> | Canonical C rules for objects, representations, alignment, storage duration, lifetime, effective type, pointer arithmetic, behavior categories, initialization, and memory-management functions |
| `external-cppreference-object` | <https://en.cppreference.com/w/c/language/object> | Navigable summary of object representation, alignment, effective type, and strict aliasing |
| `external-cppreference-storage-duration` | <https://en.cppreference.com/w/c/language/storage_duration> | Automatic, static, thread, and allocated storage duration; scope and linkage distinctions |
| `external-cppreference-malloc` | <https://en.cppreference.com/w/c/memory/malloc> | Allocation size, failure, alignment, initialization, and zero-size behavior |
| `external-cppreference-calloc` | <https://en.cppreference.com/w/c/memory/calloc> | Byte-zero initialization and its distinction from abstract numeric/pointer initialization guarantees |
| `external-cppreference-realloc` | <https://en.cppreference.com/w/c/memory/realloc> | Move/in-place behavior, preservation limits, failure handling, pointer invalidation, and zero-size version notes |
| `external-cppreference-free` | <https://en.cppreference.com/w/c/memory/free> | Valid arguments, null-pointer behavior, and lifetime termination |
| `external-sei-cert-exp33` | <https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/expressions-exp/exp33-c/> | Uninitialized automatic and dynamically allocated memory |
| `external-sei-cert-mem30` | <https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/memory-management-mem/mem30-c/> | Use-after-free and double-free prevention |
| `external-sei-cert-mem31` | <https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/memory-management-mem/mem31-c/> | Leak prevention and ownership lifetime |
| `external-sei-cert-mem34` | <https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/memory-management-mem/mem34-c/> | Freeing only pointers returned by compatible dynamic allocation functions |
| `external-sei-cert-arr30` | <https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/arrays-arr/arr30-c/> | Out-of-bounds pointer formation, arithmetic, and access |
| `external-sei-cert-mem35` | <https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/memory-management-mem/mem35-c/> | Allocation-size calculation and overflow risks |

External validation is required. The mapped Notion material is C++-oriented
and does not precisely define the C abstract machine, allocated storage
duration, effective type, C allocation edge cases, behavior categories, or the
safety rules expected for this chapter.

### Coverage Status

`IMPLEMENTED_AND_VERIFIED`: canonical internal coverage is complete, all mapped
Notion files were read, C-specific language and safety gaps were validated
against primary references, and the learner-facing outputs have been created
and checked.

## 3. Priority And Dependencies

- Priority: `MUST`
- Depth: Deep
- Prerequisite: CH02 C Fundamentals
- Required prior mental model: objects, declarations, initialization, scope,
  linkage, storage duration, expressions, arrays, functions, and basic pointer
  syntax.
- Role in learning path: establish how C reasons about objects and lifetimes
  before CH04 Pointer Mastery and CH05 Compound Types in C.
- Master-specific rule: always include real memory bugs and debugging tools.
- Controlled EXPERT scope: strict aliasing, effective type, cache locality,
  false sharing, and memory barriers must be bounded and clearly separated from
  the core single-threaded C memory lesson.

## 4. Scope And Depth Boundaries

### Deep In This Topic

- C object, value, type, representation, address, alignment, lifetime, and
  storage duration.
- Automatic, static, thread, and allocated storage duration.
- Typical hosted executable/process regions: text, read-only data, initialized
  data, zero-initialized data/BSS, stack mappings, and allocator-managed
  storage.
- Stack frames as an ABI/compiler implementation model, including parameters,
  return state, saved registers, spills, and automatic objects.
- `malloc`, `calloc`, `realloc`, `free`, allocation failure, ownership, and
  overflow-safe size calculation.
- Alignment and padding, including why raw byte comparison or serialization of
  structures is unsafe.
- Endianness, byte inspection, and network byte order at concept level.
- Undefined, unspecified, and implementation-defined behavior.
- Memory leak, dangling pointer, wild/uninitialized pointer, null dereference,
  double free, invalid free, use-after-free, buffer overflow, and stack
  overflow.
- Debugging with compiler warnings, sanitizers, Valgrind where available, GDB,
  and binary-inspection tools.

### Introduce But Defer

- Deep pointer declarators, multi-level pointers, callbacks, and pointer-heavy
  API design: CH04.
- Full structure layout, flexible array members, unions, and portable wire
  formats: CH05.
- Memory-mapped I/O and deep `volatile`: CH06.
- Industrial allocation policies, static analysis governance, MISRA, and
  BARR-C: CH07.
- C++ `new`/`delete`, RAII, smart pointers, containers, and exception safety:
  CH10, with comparison only here.
- Atomic ordering and the concurrent C/C++ memory model: CH14. A memory barrier
  is not a synonym for the C object/storage model.
- Allocator internals, custom pools, NUMA, cache-line tuning, and false sharing:
  controlled advanced follow-up only.

## 5. Merged Concept Map

### C Abstract Machine Versus Process Layout

- Start with the standard concepts: an object is a region of data storage whose
  contents can represent values; each object has a type, size, alignment,
  storage duration, and lifetime.
- Scope and linkage describe identifiers. Storage duration and lifetime
  describe objects. Physical placement in a stack, register, data section, or
  heap is an implementation decision.
- The C standard does not require text/data/BSS/heap/stack segments or fixed
  growth directions. Present them as a common hosted toolchain and OS model,
  then verify a concrete binary with `size`, `nm`, `readelf`, and `objdump`.
- Read-only data may share or separate mappings from executable code depending
  on object format, linker script, loader, and target.
- BSS is a conventional object-file/executable section for zero-initialized
  static-storage objects that need not occupy equivalent initialized bytes in
  the file. The language guarantee is initialization, not the existence of a
  section named `.bss`.

### Storage Duration, Lifetime, And Initialization

- Automatic storage duration normally covers block-entry to block-exit
  execution, with variably modified objects receiving rules tied to declaration
  execution. It does not guarantee physical stack storage.
- Static-storage objects exist for the entire program execution and are
  initialized before program startup; absent an explicit initializer, they
  receive language-defined zero initialization.
- Thread-storage objects have one instance per thread where the implementation
  supports C threads.
- Storage returned by C allocation functions has allocated storage duration.
  Its lifetime begins with successful allocation and ends on deallocation or
  reallocation that releases the original region.
- An uninitialized automatic object has an indeterminate representation.
  `malloc` and newly added `realloc` bytes are not initialized. The lesson
  should teach the safe rule: initialize before any typed read.
- `calloc` initializes all bytes to zero. Do not overgeneralize this into a
  portable claim that every possible C type's semantic zero/null value must be
  represented by all-bits-zero.

### Typical Stack Frames And Stack Overflow

- A call stack and stack frame are implementation/ABI models, not C language
  requirements.
- A frame may contain return information, saved registers, spills, parameters,
  and automatic objects, but optimization can keep values in registers, merge
  frames, inline calls, or remove objects entirely.
- Recursion, large automatic arrays, variable length arrays, and deep call
  chains can exhaust finite stack resources.
- Debug stack failures with `gdb backtrace`, `frame`, and `info locals`, while
  accounting for optimization and corrupted unwind information.

### Dynamic Allocation And Ownership

- `malloc(n)` requests at least `n` bytes suitably aligned for ordinary object
  types covered by the standard guarantee and returns either a pointer or a
  null pointer. The bytes are uninitialized.
- `calloc(count, size)` performs two-dimensional sizing and zeroes allocated
  bytes, but multiplication overflow and implementation behavior must be
  handled according to the selected standard/library contract.
- `realloc` may resize in place or allocate elsewhere and copy the preserved
  prefix. On success, the old pointer value must no longer be used; on failure,
  the original allocation remains valid.
- Use a temporary pointer for `realloc`, validate multiplication/addition before
  requesting bytes, and define ownership before allocation.
- `free(NULL)` has no effect. Any other argument must be a currently live
  pointer value obtained from a compatible allocation function, not an interior
  pointer, automatic object, static object, or already freed allocation.
- Zero-size allocation and `realloc(p, 0)` have version-sensitive and
  implementation-sensitive history. Production code should avoid using
  zero-size requests as an ownership protocol.

### Alignment, Padding, And Representation

- Every complete object type has an alignment requirement; an object must be
  stored at a suitably aligned address before being accessed through that type.
- Structure padding can appear between members and at the end so array elements
  remain aligned. Member order can affect size, but reordering can affect ABI,
  protocol, and readability contracts.
- Padding bytes can hold unspecified values. Do not compare structures with
  `memcmp`, hash raw object bytes, or transmit raw structures unless a specific
  representation contract makes that valid.
- Character types may inspect object representation byte by byte. Converting
  arbitrary bytes into another type still requires valid alignment, effective
  type, and representation.
- `_Alignof`/`alignof`, `_Alignas`/`alignas`, `offsetof`, and `sizeof` should
  appear in small inspection examples with version labels.

### Endianness And External Data

- Endianness describes byte order for multi-byte scalar representations; it is
  distinct from bit numbering, structure padding, and protocol field order.
- Little-endian and big-endian are common, but the C language does not mandate
  either.
- Detecting endianness can be demonstrated by inspecting an object's bytes
  through `unsigned char`.
- Network byte order is conventionally big-endian. Protocol parsing should
  decode explicit bytes or use appropriate conversion APIs rather than cast a
  packet buffer to a structure pointer.
- Serialization must define field width, byte order, signedness, alignment, and
  bounds independently of native object layout.

### Behavior Categories

- Undefined behavior imposes no requirements. A crash is only one possible
  manifestation; optimization can remove or transform surrounding code.
- Unspecified behavior permits one of multiple valid possibilities without
  requiring the implementation to document which occurs in a given instance.
- Implementation-defined behavior requires the implementation to choose and
  document a behavior.
- A constraint violation normally requires a diagnostic and is not simply
  another label for undefined behavior.
- Chapter examples should classify actual cases: out-of-bounds access,
  use-after-free, invalid free, misaligned typed access, and many uninitialized
  reads as undefined; object sizes, plain-`char` signedness, and some conversion
  results as implementation-defined; selected evaluation choices as
  unspecified.

### Effective Type And Aliasing

- Allocated storage initially has no declared type. Writes through a non-
  character lvalue, or copies from an existing object with `memcpy`, influence
  the effective type used for later access.
- Access through an incompatible lvalue can violate the aliasing rules and
  produce undefined behavior even when addresses and byte counts appear valid.
- Character types are the standard route for examining object representation.
- Teach this as controlled advanced material: use normal typed objects and
  `memcpy` for byte transfer; do not use pointer casts as a general
  serialization or type-punning mechanism.

## 6. Usage Angles

### C Usage

- Explicit object lifetime, allocation ownership, checked size calculation,
  array bounds, and one cleanup policy per allocation.
- Use `sizeof *ptr` rather than repeating the pointed-to type in allocation
  expressions.
- Prefer array/count or pointer/end interfaces whose bounds are explicit.
- Use `memcpy` for representation transfer where the contract permits it, not
  incompatible typed dereferences.

### C++ Usage

- Keep C++ content comparative: `malloc`/`free` versus `new`/`delete`, manual
  cleanup versus RAII, raw allocated arrays versus `std::vector`, and C null
  pointers versus `nullptr`.
- Do not copy C++ constructors, destructors, exceptions, smart pointers,
  references, or placement `new` into the C lesson as C mechanisms.
- The C++ comparison should motivate later resource-management chapters rather
  than turn this topic into a C++ memory-management lesson.

### Embedded Usage

- Static allocation may be preferred when bounded memory, deterministic startup,
  and analyzable failure behavior matter.
- Dynamic allocation policy depends on product lifetime, allocator guarantees,
  fragmentation tolerance, recovery strategy, and safety requirements; avoid
  absolute claims that embedded systems never allocate dynamically.
- Inspect linker map files and section sizes to understand ROM/RAM placement,
  while keeping linker-script details subordinate to the language lesson.
- Budget stack usage for every task/thread and worst-case call path.
- Parse protocol bytes explicitly and document endianness; do not use raw
  structure overlays as a portable wire format.
- No Linux Device Driver or kernel-driver material belongs in this topic.

### Enterprise Usage

- Make ownership visible in APIs and code review: who allocates, who frees, with
  which function, and on every exit path.
- Centralize cleanup in disciplined C patterns or small owner abstractions.
- Treat allocation-size overflow, invalid lifetime assumptions, structure
  representation dependencies, and unchecked bounds as security issues.
- Run warning-clean builds, sanitizers, static analysis, leak checks, and
  long-running stress tests in CI where supported.
- Record ABI, alignment, endianness, compiler, optimization, and language-
  version assumptions when code depends on them.

## 7. Required Comparisons

| Comparison | Required teaching point |
| --- | --- |
| C abstract object model vs process memory map | Standard guarantees versus common ELF/OS/toolchain implementation |
| Scope vs storage duration vs lifetime vs linkage | Name visibility/identity versus object existence |
| Automatic vs static vs thread vs allocated storage duration | Start/end of lifetime, initialization, sharing, and cleanup |
| Text vs read-only data vs data vs BSS | Common section purpose, file footprint, permissions, and why none is a universal C requirement |
| Stack vs heap/allocated storage | Automatic call-oriented implementation model versus allocator-controlled lifetime; avoid equating either with a language guarantee |
| Stack object vs heap object | Same C type can have different storage duration and ownership without becoming a different kind of value |
| `malloc` vs `calloc` | Uninitialized bytes versus zeroed bytes, one size versus count/element-size interface |
| `malloc` vs `realloc` | New allocation versus resize that can move and invalidate the old pointer on success |
| Direct `realloc` assignment vs temporary pointer | Lost-allocation leak on failure versus preserving ownership |
| `malloc`/`calloc`/`realloc`/`free` vs `new`/`delete` | C byte allocation and explicit cleanup versus C++ object construction/destruction |
| Manual cleanup vs RAII | Exit-path auditing in C versus scope-bound C++ ownership |
| Raw allocated array vs `std::vector` | Explicit byte/count/cleanup contract versus managed C++ container |
| Alignment vs padding | Object-address requirement versus inserted bytes in aggregate layout |
| Object value vs object representation | Abstract value versus bytes, padding, and possible trap/invalid representations |
| Little-endian vs big-endian vs network byte order | Native representation choices versus external protocol convention |
| Undefined vs unspecified vs implementation-defined | No requirements versus permitted choices versus documented implementation choice |
| Null pointer vs wild pointer vs dangling pointer | Deliberate no-object value versus indeterminate pointer versus pointer whose target lifetime ended |
| Memory leak vs use-after-free vs double free | Lost ownership versus invalid access versus repeated deallocation |
| Buffer overflow vs stack overflow | Out-of-bounds access versus exhaustion of finite call-stack resources |
| `-O0` vs optimized debugging | Easier source correspondence versus production-like transformations and UB exposure |

## 8. Common Bugs And Failure Modes

- Reading an uninitialized automatic object or uninitialized allocated bytes.
- Treating a typical virtual address diagram as a portable C guarantee.
- Assuming every automatic local is stored on a stack or every dynamic
  allocation comes from one contiguous "heap" segment.
- Assuming stack and heap always grow toward each other.
- Dereferencing a null, wild, dangling, misaligned, or out-of-bounds pointer.
- Forming pointer values outside an array object and its one-past position.
- Dereferencing a one-past pointer.
- Returning or storing a pointer to an automatic object after its lifetime ends.
- Losing the only pointer to allocated storage by overwriting it.
- Assigning `realloc` directly to the owning pointer and leaking the original
  allocation when the call fails.
- Using the old pointer after a successful `realloc`, even if the address appears
  unchanged.
- Failing to initialize the newly added portion after a growing `realloc`.
- Integer overflow in `count * sizeof(element)` or header-plus-payload sizing.
- Allocating `sizeof(pointer)` instead of `sizeof(*pointer)` or the object size.
- Freeing an interior pointer, stack object, static object, string literal, or
  pointer from a different allocation family.
- Double free and use-after-free, including through aliases that were not reset.
- Assuming setting one pointer to `NULL` invalidates all aliases.
- Assuming `calloc` creates a valid semantic zero for every possible object
  representation.
- Comparing structures with `memcmp` despite padding bytes.
- Serializing or hashing raw structures with native padding and endianness.
- Casting an unaligned byte buffer to a structure or scalar pointer.
- Violating effective-type/strict-aliasing rules through incompatible pointer
  casts.
- Writing past an allocation, off-by-one terminator bugs, and length/capacity
  confusion.
- Deep recursion, large automatic arrays, or VLAs causing stack exhaustion.
- Optimized builds exposing UB that seemed harmless under `-O0`.

## 9. Debugging Notes

- Compile a diagnostic build with a declared standard and strong warnings, for
  example:
  `cc -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wformat=2 -g3`.
- Use AddressSanitizer for out-of-bounds access, use-after-free, double free,
  and many invalid-free defects:
  `-fsanitize=address -fno-omit-frame-pointer`.
- Use UndefinedBehaviorSanitizer for supported misalignment, object-size,
  pointer-overflow, and related checks: `-fsanitize=undefined`.
- Use MemorySanitizer with a supported Clang environment for uninitialized
  reads; ASan does not generally detect all uninitialized-value use.
- Use LeakSanitizer or Valgrind for leak tracing on supported hosted targets.
  Tool availability and allocator interposition vary by platform.
- Reproduce crashes at both `-O0` and the production optimization level.
  Undefined behavior and optimized variable location can change symptoms.
- In GDB, start with `bt`, `frame N`, `info locals`, `info args`, `p/x`,
  `x/Nbx`, and watchpoints. A corrupted stack can make the backtrace itself
  unreliable.
- Inspect executable/object layout with `size`, `nm -S`, `readelf -S -l -s`,
  and `objdump -h -t -d`.
- Generate and inspect a linker map on embedded builds to account for ROM, RAM,
  zero-initialized sections, retained data, and stack/heap reservations.
- Use compiler stack-usage reports such as GCC/Clang `-fstack-usage` where
  supported, then combine static estimates with worst-case call-path analysis.
- Log allocation sizes, ownership transitions, and failure paths in a reduced
  reproducer; do not print or dereference a pointer after its lifetime ends.
- Treat sanitizers and Valgrind as hosted diagnostics, not proof that a
  freestanding target is memory-safe.

## 10. Best Practices

- Teach storage duration and lifetime before physical segments.
- Initialize automatic objects and allocated storage before typed reads.
- Check every allocation result before use.
- Validate size arithmetic before calling an allocator.
- Prefer `sizeof *ptr` and avoid casts on `malloc` family results in C.
- Define ownership at API boundaries and keep allocation/deallocation at the
  same abstraction level where practical.
- Use one allocation family consistently and document the matching release
  function.
- Use a temporary pointer for `realloc`.
- Avoid zero-size allocation as a control-flow or ownership convention.
- Free memory on every applicable exit path, then invalidate the owner; remember
  that aliases can still dangle.
- Prefer bounded, explicit array interfaces and preserve length/capacity
  invariants.
- Do not derive wire formats, file formats, hashes, or equality from raw
  structure bytes.
- Use `memcpy` and explicit decoding rather than incompatible or unaligned typed
  pointer casts.
- Make endianness conversion explicit at external-data boundaries.
- Minimize large automatic objects and unbounded recursion.
- Document implementation-defined dependencies and verify them with compile-
  time assertions or target tests where appropriate.
- Run dynamic analysis and static analysis early, not only after a crash.

## 11. Interview Angles

### Junior

- Compare stack and heap without claiming they are required C language
  segments.
- What is the difference between data and BSS?
- Why are static-storage objects zero-initialized?
- Compare `malloc`, `calloc`, `realloc`, and `free`.
- What are a memory leak, dangling pointer, and wild pointer?
- Why is `free(NULL)` safe?
- What is a buffer overflow?

### Middle

- Explain scope, storage duration, lifetime, and physical storage separately.
- Why should `realloc` normally use a temporary pointer?
- What happens to the original pointer on `realloc` success and failure?
- How can allocation-size arithmetic overflow?
- Explain alignment and structure padding.
- Why is `memcmp` not a general structure equality operation?
- Compare undefined, unspecified, and implementation-defined behavior.
- Diagnose a segmentation fault with GDB and sanitizers.

### Senior

- Explain the C object model versus an ELF process layout and identify which
  claims are standard guarantees.
- Review an allocator-owning API for lifetime, aliasing, failure, and cleanup
  contracts.
- Explain effective type and strict aliasing without relying on folklore.
- Design a portable binary protocol parser that handles alignment, bounds,
  widths, and endianness.
- Define an embedded memory policy covering static allocation, bounded dynamic
  allocation, stack budgets, fragmentation, and failure recovery.
- Explain why a program can pass tests at `-O0` and fail at `-O2` because of
  undefined behavior.
- Choose and justify warnings, sanitizers, static analysis, leak testing, and
  target-side diagnostics for a production C module.

## 12. Practice Tasks

- Basic: declare automatic, static local, file-scope initialized, and file-scope
  zero-initialized objects; inspect addresses and binary sections without
  claiming the observations are universal.
- Basic: implement checked `malloc`/`calloc` allocation and one cleanup path.
- Basic: demonstrate `free(NULL)` and explain why a second `free` of a non-null
  freed pointer is different.
- Intermediate: implement an overflow-checked dynamic array growth function
  using a temporary pointer for `realloc`.
- Intermediate: print `sizeof`, `alignof`, and `offsetof` for several structures,
  then explain internal and trailing padding.
- Intermediate: inspect a 32-bit integer through `unsigned char` to identify the
  target byte order.
- Intermediate: introduce leak, use-after-free, double-free, and out-of-bounds
  defects in isolated programs and classify which tool detects each.
- Advanced: compare section and symbol reports from `size`, `nm`, `readelf`, and
  `objdump` for initialized and zero-initialized globals.
- Advanced: parse a byte buffer into explicit integer fields without structure
  overlay, unaligned loads, or host-endian assumptions.
- Advanced embedded: produce a stack budget for a bounded call tree and large
  local buffers using compiler reports and map-file evidence.
- Advanced: create a strict-aliasing violation, observe optimization-sensitive
  behavior, and repair it using a defined representation-transfer method.

## 13. Gaps, Corrections, And External Validation Needs

### Notion Corrections Required

- The mapped chapters are C++ lessons. `new`, `delete`, constructors,
  destructors, exceptions, references, `nullptr`, smart pointers, placement
  `new`, and RAII are comparison material, not C mechanisms.
- In C, `malloc` returns `void *` that converts to an object-pointer type without
  a cast. The Notion rule that it "must be cast" applies to C++, not C.
- Uninitialized memory should be called indeterminate, not merely "garbage."
  Reading it has type- and standard-version-sensitive rules; the safe coding
  rule is to initialize before reading.
- Text/data/BSS/heap/stack diagrams are common implementations, not the C
  abstract machine. Address order and growth direction are not portable.
- "Local variables are stored on the stack" is too broad. Optimizers and ABIs
  can place or eliminate them differently.
- The stated fixed stack sizes and allocation-speed ratios are examples at
  best, not guarantees or useful universal rules.
- Stack allocation is not always one instruction, and dynamic allocation cost
  is allocator-, workload-, target-, and optimization-dependent.
- "Stack never fragments" and "heap is random access" mix allocation policy
  with data-access semantics and should not be taught as language facts.
- A thread commonly has its own call stack, but dynamic storage being shared
  does not itself define thread safety.
- Stack and heap collision is not the universal explanation for stack overflow.
- `calloc` zeroes bytes; teaching it as universally constructing the semantic
  zero value for every type is too broad.
- `realloc` invalidates the old pointer value on success. The brief must go
  beyond the temporary-pointer failure pattern and forbid post-success use of
  aliases to the old allocation.
- Setting one pointer to `NULL` after `free` does not repair other aliases and
  is not a substitute for ownership design.
- Null checks prevent null dereference only; they do not prove that a non-null
  pointer is live, in bounds, aligned, or correctly typed.
- An array expression usually converts to a pointer to its first element, but an
  array is not itself a pointer and conversion has exceptions.
- Pointer arithmetic and relational comparison require same-array reasoning;
  the mapped pointer chapter states allowed operations too broadly.
- Pointer sizes need not be identical for every pointer type solely because a
  target is described as 32-bit or 64-bit.
- `free`/`delete` mismatches are undefined behavior. Explanations such as
  "delete calls a non-existent destructor" or "delete destroys only the first
  element" are not complete semantic models.
- Avoid safe-delete macros. They can evaluate awkward expressions, hide
  ownership, and reset only one alias.
- Claims such as "ASan has zero false positives," "`unique_ptr` has zero
  overhead," or exception/nothrow performance rules are too absolute and are
  outside this C topic.
- Placement `new` at a hardware address is not a general embedded memory-mapped
  I/O technique and must not be imported into the C lesson.

### Internal Coverage Gaps Filled Externally

- Exact C definitions for object representation, alignment, effective type,
  lifetime, and allocated storage duration.
- Exact `malloc`, `calloc`, `realloc`, and `free` contracts, including alignment,
  pointer invalidation, failure, zero-size requests, and standard-version notes.
- C behavior categories and the consequences of undefined behavior under
  optimization.
- Safety rules for use-after-free, invalid free, leaks, uninitialized memory,
  out-of-bounds pointers, and allocation-size calculation.
- Portable treatment of padding, byte inspection, and aliasing.

### External Validation Still Needed During Lesson Generation

- Choose and state the primary language baseline, recommended as C17 with
  explicit C23 notes, then recheck zero-size allocation and terminology against
  that baseline.
- Validate compiler-specific warning, sanitizer, stack-usage, and binary-tool
  commands on the repository's available GCC/Clang environment.
- Validate endianness/network examples against the selected hosted API scope.
  POSIX conversion functions may be mentioned only when clearly labeled as
  platform APIs.
- Confirm any embedded linker-map or section example against a real target
  toolchain before presenting addresses or section names as concrete evidence.
- Deep memory barriers, atomics, and false sharing require concurrency sources
  and belong primarily to topic 14, not this chapter.
- No Linux Device Driver or kernel-driver source is needed or permitted.

## 14. Output Targets

| Output | Current status | Intended scope |
| --- | --- | --- |
| `coverage/topic-briefs/03-c-memory-model.md` | Created | Source audit, corrections, concept merge, comparisons, debugging plan, interview angles, gaps, and external validation trace |
| `knowledge/03-c-memory-model.md` | Created and verified | MUST-depth learner-facing C lesson without audit metadata |
| `interview/03-c-memory-model.md` | Created and verified | Junior/middle/senior interview pack |
| `examples/03-c-memory-model/README.md` | Created and verified | Compile-ready C examples, binary inspection, sanitizer exercises, and debugging workflow |

The canonical Topic Brief, knowledge lesson, interview pack, and example suite
now exist for this topic.
