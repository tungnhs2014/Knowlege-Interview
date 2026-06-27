# Topic Brief 06 - Advanced C For Embedded

## 1. Canonical Routing

| Field | Validated value |
| --- | --- |
| `learning_path_number` | `06` |
| Title | Advanced C For Embedded |
| `slug` | `advanced-c-for-embedded` |
| Requested topic | Advanced C language and design techniques for portable, testable embedded firmware and middleware |
| Master source | `master-ch06` |
| Required Notion sources | `notion-10-2`, `notion-10-5`, `notion-10-7` |
| Topic Brief | `coverage/topic-briefs/06-advanced-c-for-embedded.md` |
| Knowledge target | `knowledge/06-advanced-c-for-embedded.md` |
| Interview target | `interview/06-advanced-c-for-embedded.md` |
| Example target | `examples/06-advanced-c-for-embedded/README.md` |

Validation result: the number, title, slug, master source, three mapped Notion
sources, and all canonical output paths match `LEARNING_PATH.md`.

## 2. Source Coverage

### Internal Sources Read

| Source label | Path | Coverage contribution |
| --- | --- | --- |
| `master-ch06` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, CH06 | Priority, CH05 dependency, MUST/SHOULD/NICE keyword scope, required comparisons, firmware-oriented expansion rule, and interview focus |
| `guide-section-03` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 3 | Deep treatment required for embedded MUST concepts and controlled depth for SHOULD/NICE concepts |
| `guide-section-04` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 4 | Full-chapter structure and C, C++, embedded, enterprise, bug, debug, interview, and practice angles |
| `guide-section-06` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 6 | Required C/C++ and `volatile`/atomic comparison formats |
| `guide-section-07` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 7 | Practical embedded examples and depth control |
| `guide-section-09` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 9 | Trusted routing to ISO C, CERT C, MISRA, BARR-C, and compiler documentation |
| `guide-section-10` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 10 | Technical-English, Markdown, compile-oriented example, and unsafe-API warning rules |
| `guide-section-11` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 11 | Full lesson, interview pack, example, and review expectations |
| `guide-section-12` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 12 | Required callback, macro, OOP, and `volatile` comparisons |
| `guide-section-14` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 14 | Final quality checklist |
| `notion-index` | `docs/C++ Notion/C++ Programming.md` | Notion chapter inventory and source identity validation |
| `notion-10-2` | `docs/C++ Notion/Chapter 10-2 Preprocessor Directives.md` | Preprocessing stages, includes, object-like and function-like macros, stringification, token pasting, conditional compilation, include guards, predefined macros, pragmas, macro pitfalls, and C++ alternatives |
| `notion-10-5` | `docs/C++ Notion/Chapter 10-5 Callbacks.md` | Function pointers, callback parameters, aliases, stateful callbacks, lifetime hazards, dispatch/event patterns, lambdas, functors, templates, and `std::function` |
| `notion-10-7` | `docs/C++ Notion/Chapter 10-7 Signal Handling.md` | Asynchronous control-flow model, restricted handler context, flag-and-defer pattern, `sig_atomic_t`, signal APIs, and examples requiring correction before reuse |

All three mapped Notion chapter files were read. No mapped Notion source was
skipped.

### External References Consulted

Accessed on 2026-06-13.

| Label | Reference | Validation purpose |
| --- | --- | --- |
| `external-misra` | MISRA publications and MISRA C information: <https://misra.org.uk/publications/> and <https://misra.org.uk/misra-c/> | Identify MISRA C as safety-oriented guidance for embedded and standalone C; route later safety claims to the licensed guideline and project compliance process |
| `external-barr-c` | BARR-C:2018, Embedded C Coding Standard: <https://barrgroup.com/embedded-c-coding-standard> | Practical embedded rules for modules, headers, data types, functions, preprocessor use, variables, portability, maintainability, and automated enforcement |
| `external-sei-cert-pre00` | CERT C PRE00-C: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/recommendations/preprocessor-pre/pre00-c/> | Prefer inline or static functions when they can replace function-like macros; validate multiple-evaluation and scope hazards |
| `external-sei-cert-pre31` | CERT C PRE31-C, linked from PRE00-C | Avoid side effects in arguments to unsafe macros |
| `external-sei-cert-int13` | CERT C INT13-C, linked from INT34-C | Prefer unsigned operands for bitwise operations |
| `external-sei-cert-int34` | CERT C INT34-C: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/integers-int/int34-c/> | Validate shift-count ranges, signed-shift hazards, integer promotions, and undefined or implementation-defined behavior |
| `external-sei-cert-sig30` | CERT C SIG30-C: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/rules/signals-sig/sig30-c/> | Validate restricted signal-handler operations and the flag-and-defer pattern; correct unsafe mapped examples |
| `external-gcc-volatile` | GCC, Volatiles: <https://gcc.gnu.org/onlinedocs/gcc/Volatiles.html> | Compiler interpretation of volatile accesses; confirm that `volatile` is not a general memory barrier and does not order ordinary memory |
| `external-gcc-cpp` | GCC, The C Preprocessor, Macro Pitfalls: <https://gcc.gnu.org/onlinedocs/cpp/Macro-Pitfalls.html> | Operator precedence, swallowed semicolons, duplicate side effects, argument prescan, and other macro-expansion traps |
| `external-clang-ubsan` | Clang, UndefinedBehaviorSanitizer: <https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html> | Validate executable checks for invalid shifts, signed overflow, misalignment, null dereference, and bounds defects |
| `external-clang-diagnostics` | Clang, Diagnostics Reference: <https://clang.llvm.org/docs/DiagnosticsReference.html> | Route compiler-version-specific warning names and macro/qualifier diagnostics |

### External Source Boundaries

- MISRA C is licensed guidance. The future lesson may explain compliance
  concepts and cite official publications, but must not reproduce proprietary
  rule text or imply certification from reading a summary.
- BARR-C:2018 is useful practical guidance, not the C language standard and not
  a substitute for target, compiler, hardware, or product requirements.
- CERT C provides secure-coding rules and recommendations, not hardware-specific
  register semantics.
- Compiler documentation describes one implementation. GCC or Clang behavior
  must not be presented as universal ISO C behavior.
- Register access, barriers, interrupt rules, and special attributes ultimately
  require the selected MCU architecture manual, compiler manual, ABI, linker
  documentation, and vendor HAL/register definitions.

### Coverage Status

`CHAPTER_OUTPUTS_CREATED_AND_REVIEWED`: canonical routing, all mapped internal
sources, master priority, guide requirements, and the principal embedded-C
safety gaps have been addressed. External safety and compiler references were
consulted where the C++-first mapped chapters were insufficient or inaccurate.
The learner-facing knowledge, interview, and example outputs have been created
and reviewed against this brief.

## 3. Priority And Dependencies

- Priority: `MUST` for embedded software; `SHOULD` for general software.
- Depth: Deep for embedded use.
- Prerequisite: CH05, Compound Types In C.
- Required prior model: integer types and conversions, arrays, structures,
  unions, enums, pointers, function declarations, storage duration, linkage,
  object lifetime, alignment, and undefined behavior.
- Follow-on topic: CH07, Industrial C Practices.
- Expansion rule: every major keyword must be tied to a real firmware or
  middleware need such as register access, hardware abstraction, callbacks,
  state machines, protocol handling, or bounded buffering.

## 4. Scope And Depth Boundaries

### Deep In This Topic

- Bitwise operators, masks, unsigned arithmetic, integer promotions, safe shift
  counts, and set/clear/toggle/test operations.
- Memory-mapped I/O mental model and target-defined register access.
- Correct and incorrect uses of `volatile`; contrast with atomicity, ordering,
  synchronization, barriers, and critical sections.
- `const`, `volatile`, `const volatile`, `restrict`, `static`, and `extern` in
  embedded interfaces.
- Preprocessor phases, macros, conditional compilation, include guards,
  configuration boundaries, and compile-time checks.
- C callbacks using function pointer plus context pointer, lifetime contracts,
  dispatch tables, and function pointer tables.
- Table-driven finite state machines using explicit states, events,
  transitions, actions, and invalid-transition policy.
- OOP-style C with opaque types, private implementation data, explicit
  lifecycle, operation tables, and HAL interfaces.
- Testability: separate pure policy from register access and inject hardware
  operations through narrow interfaces.

### Medium In This Topic

- Bit-field and packed-structure portability risks.
- Explicit endian conversion and byte-oriented protocol access.
- Register-map modeling and why vendor definitions or generated headers are
  preferred to handwritten guesses.
- Command dispatchers, ring buffers, logger macros, and variadic functions.
- `stdarg.h`, default argument promotions, format/type contracts, and
  `va_start`/`va_arg`/`va_end`.

### Brief Awareness

- `_Static_assert`/`static_assert` in C.
- Carefully constrained `_Generic` or generic macro patterns.
- Intrusive linked-list motivation and ownership cost.

### Defer Or Exclude

- Organization-wide MISRA/CERT compliance governance, deviation records,
  static-analysis policy, and coding-standard rollout: CH07.
- Deep interrupt-controller, startup, linker-script, assembly, cache, DMA,
  or architecture memory-ordering treatment: target-specific follow-on work.
- RTOS API details and scheduler design.
- POSIX process signal handling beyond the small comparison needed to correct
  and contextualize `notion-10-7`.
- C++ concurrency and full atomic memory ordering: CH14.
- Full C++ OOP, RAII, lambdas, and type-erased callback design: later C++
  chapters.
- Linux Device Driver and kernel-driver material is excluded.

## 5. Source Corrections And Merge Decisions

### `notion-10-2` Corrections

- The preprocessor is token-oriented translation processing, not merely an
  unrestricted text substitution engine.
- Macros are not inherently faster than functions. Optimizing compilers can
  inline functions, while macro expansion can increase code size.
- Parenthesizing a function-like macro does not solve duplicate evaluation.
- Multi-statement statement macros require a controlled single-statement form
  such as `do { ... } while (0)` when a macro is truly necessary.
- `#pragma once` is widely supported but not an ISO C replacement for portable
  include guards.
- `#pragma pack` and attributes are implementation-specific and may create
  alignment faults or inefficient access; they do not create a portable wire
  format.
- C constants should normally use `enum`, typed `const` objects where suitable,
  or `static inline` functions rather than untyped replacement macros.

### `notion-10-5` Corrections

- A plain C function pointer cannot carry state by itself, but a callback API can
  carry state portably with a separate `void *context`.
- Callback correctness includes function signature, calling convention,
  context type, context lifetime, registration lifetime, execution context, and
  reentrancy.
- Stored callbacks require explicit unregister/shutdown rules to prevent calls
  through dead function or context storage.
- `std::function` can introduce type-erasure, indirect-call, object-size, and
  possible allocation costs; it is not an automatic embedded replacement for a
  C callback pair.
- Function tables and operation tables should be immutable when possible and
  validated before indirect calls.

### `notion-10-7` Corrections

- Operating-system signals and hardware interrupts are different mechanisms.
  The mapped chapter is a source for asynchronous-context reasoning, not an MCU
  interrupt tutorial.
- Numeric values of signals are implementation-defined and must not be taught
  as portable constants.
- A standard C signal handler has very restricted behavior. I/O streams,
  allocation, deallocation, locking, and general cleanup do not belong inside
  the handler.
- The mapped `sigaction` example calls `strlen` inside its handler and must not
  be reused as signal-safe code.
- POSIX asynchronous-signal-safe functions are a POSIX extension, not the
  strictly conforming ISO C set.
- `volatile sig_atomic_t` supports a narrow signal communication pattern; it
  does not make arbitrary shared data atomic or thread-safe.
- For embedded interrupt service routines, permitted operations come from the
  target architecture, compiler, device, and RTOS contracts. The general design
  lesson is to capture minimal state and defer complex work.

## 6. Merged Concept Map

### Bitwise Operations And Masks

- Use fixed-width unsigned types when register or protocol widths are part of
  the contract.
- Build masks with an unsigned left operand and a validated bit index.
- Account for integer promotions before applying `~`, shifts, or masks to types
  narrower than `int`.
- Set: `value |= mask`.
- Clear: `value &= ~mask`, with the complement converted or constrained to the
  intended width.
- Toggle: `value ^= mask`.
- Test any selected bit: `(value & mask) != 0U`.
- Test all selected bits: `(value & mask) == mask`.
- Reject negative or out-of-range shift counts.
- Do not apply read-modify-write blindly to registers with write-one-to-clear,
  read-to-clear, reserved, self-clearing, or command semantics.

### Register Access And Memory-Mapped I/O

- A register address and layout come from the device specification, not from C.
- A volatile-qualified lvalue requests observable accesses according to the
  implementation's rules.
- `volatile` does not guarantee bus width, atomicity, mutual exclusion,
  cross-core visibility, ordering of ordinary memory, or a hardware barrier.
- Preserve `volatile` qualification through pointers and APIs.
- Use exact-width register types only when the implementation and device define
  them appropriately.
- Hide target addresses behind vendor headers or a narrow register/HAL layer.
- Keep policy code independent of physical addresses so it can run in host
  tests.
- Use target/compiler barrier primitives only when required by the architecture
  and documented by the selected toolchain.

### Qualifiers, Storage Duration, And Linkage

- `const` prevents modification through a particular access path; it does not
  necessarily place data in ROM.
- `volatile const` is suitable for read-only-from-software objects that can
  change externally, such as some status registers.
- `restrict` is a caller promise about pointer-based access. Violating that
  promise creates undefined behavior; it is an optimization contract, not a
  runtime check.
- File-scope `static` gives internal linkage to functions and objects.
- Block-scope `static` gives static storage duration and retained state, which
  affects reentrancy and test isolation.
- `extern` declares an object or function defined elsewhere; headers should
  declare, and one source file should normally define, shared objects.
- Avoid unnecessary writable global state. Make shared state ownership and
  asynchronous access explicit.

### Macros And Conditional Compilation

- Use include guards for portable headers.
- Keep build configuration centralized and validate unsupported combinations
  with `#error`.
- Prefer `static inline` functions for type checking and single evaluation.
- Keep function-like macro arguments free of side effects.
- Parenthesize parameters and complete expression replacements.
- Use `do { ... } while (0)` for statement-like macros.
- Use stringification and token pasting only when code generation or diagnostic
  naming genuinely requires preprocessing.
- Logger/assertion macros must avoid changing release-build behavior when
  disabled and should preserve type checking where possible.
- Inspect preprocessed output when conditional compilation or expansion is
  surprising.

### Callbacks And Dispatch Tables

- Represent a stateful C callback as a function pointer plus context pointer.
- Document whether invocation is synchronous, deferred, interrupt-context, or
  task-context.
- Define whether registration copies state, borrows state, or transfers
  ownership.
- Validate optional callback pointers before use.
- Use `const` operation tables when operations are fixed.
- Keep operation signatures uniform where a table is indexed by command or
  state.
- Validate external command/state values before using them as table indices.
- Do not cast incompatible function pointer types to silence diagnostics.

### Finite State Machines

- Model state and event domains explicitly with enums.
- Define one authoritative transition function or transition table.
- Separate transition selection from side-effecting actions where practical.
- Specify invalid state/event handling rather than silently indexing a table.
- Validate state before table access; C enums can hold values outside named
  enumerators.
- Keep entry, exit, and transition action ordering explicit.
- For asynchronous events, define queue ownership, overflow policy, and
  synchronization separately from the FSM itself.

### OOP-Style C And HAL Design

- Use an opaque incomplete type to hide private representation.
- Provide explicit create/init/use/deinit/destroy lifecycle operations.
- Put polymorphic operations in a typed ops table and store instance-specific
  state in a context/private-data object.
- Make ownership and nullability visible in function contracts.
- Prefer narrow capability-oriented interfaces over a single oversized ops
  table.
- Keep hardware-facing operations behind a HAL boundary while leaving
  algorithms host-testable.
- Validate operation table version, required entries, and context before use.
- OOP-style C provides encapsulation and dynamic dispatch conventions, but no
  automatic construction, destruction, inheritance checking, or RAII.

### Endianness, Packing, And Representation

- Decode and encode external byte sequences explicitly.
- Do not cast byte buffers to register or protocol structures.
- Native bit-field order, padding, alignment, enum size, and packed access are
  implementation- or target-specific.
- Packed structures can produce unaligned member addresses and do not solve
  byte-order conversion.
- Treat reserved bits and unknown protocol values defensively.

### Ring Buffers And Variadic Interfaces

- A ring buffer needs storage capacity, head/tail invariants, full/empty policy,
  wrap behavior, and ownership of producer/consumer operations.
- `volatile` indices alone do not make a ring buffer race-free.
- Choose critical sections or atomics according to the actual execution model.
- Variadic functions lose normal type checking after the fixed parameters.
- Default argument promotions must match the type requested by `va_arg`.
- Every successful `va_start` path must reach `va_end`.
- Prefer counted arrays, tagged values, or typed wrappers when the argument
  domain is known.

## 7. Required Comparisons

| Topic | C | C++ | Embedded / enterprise direction |
| --- | --- | --- | --- |
| `volatile` vs atomic | Observable implementation-defined accesses; useful for MMIO and narrow asynchronous cases | `volatile` still is not synchronization; `std::atomic` provides atomic operations and memory-order semantics | Use device/compiler rules for MMIO and atomics/critical sections for concurrency |
| Macro vs inline function | Macro is preprocessing without type checking or single-evaluation guarantees; `static inline` is a typed function | Prefer inline/`constexpr` functions and templates for typed computation | Retain macros for conditional compilation, source-location capture, token generation, and narrowly justified statement wrappers |
| Macro vs `constexpr` | C macro or C23/C11 compile-time facilities depending on need | `constexpr` participates in language typing, scope, overloads, and constant evaluation | Do not claim a macro is a typed compile-time constant |
| Function pointer vs lambda/`std::function` | Function pointer plus context supports explicit, ABI-friendly callbacks | Lambdas carry state; templates can avoid type erasure; `std::function` offers runtime polymorphism with possible overhead | Prefer the smallest mechanism that satisfies state, lifetime, ABI, allocation, and timing constraints |
| FSM in C vs State Pattern in C++ | Enum plus switch/table gives explicit data and control flow | Classes and virtual dispatch can encapsulate state behavior | Use a table/switch until object-based decomposition clearly improves changeability |
| OOP in C vs OOP in C++ | Opaque objects, lifecycle functions, context, and ops tables by convention | Language-supported classes, constructors/destructors, inheritance, virtual dispatch, and RAII | C is suitable for stable ABI/HAL boundaries; C++ offers stronger lifecycle and type guarantees where allowed |
| `const` in C vs C++ | Access-path qualification and API contract | Similar base meaning plus stronger use in classes, overloads, and constant evaluation ecosystem | Do not equate `const` with physical ROM placement |
| Dispatch table vs switch | Table supports data-driven uniform handlers; switch makes control flow explicit | Same choices plus callable objects and variants | Select for validation clarity, code size, timing, and ease of review rather than fashion |
| Packed record vs explicit encoding | Compiler-specific layout shortcut | Same ABI risks | Prefer explicit byte encoding for protocols and persistence |

## 8. C, C++, Embedded, And Enterprise Usage

### C Usage

- Bit masks and fixed-width integer operations.
- MMIO wrappers and target-defined volatile register objects.
- Internal linkage, external declarations, and immutable module interfaces.
- Function pointer/context callbacks.
- Table-driven command dispatch and FSMs.
- Opaque handles and operation tables.
- Bounded ring buffers and explicit serialization.

### C++ Usage

- `constexpr` constants/functions instead of computational macros.
- Lambdas or templates for typed callbacks.
- `std::function` only where type erasure and its resource/timing costs are
  acceptable.
- Classes and RAII for lifecycle management.
- `enum class`, `std::array`, `std::span`, and `std::variant` where the project
  baseline permits them.
- Atomic types for concurrency; `volatile` remains a separate MMIO concern.

### Embedded Usage

- Peripheral status/control access through a documented register layer.
- HAL interfaces selected by product or target configuration.
- Interrupt-to-main-loop event handoff with minimal asynchronous work.
- Sensor/actuator FSMs with explicit invalid-event behavior.
- Fixed-capacity command queues and ring buffers.
- Compile-time feature selection and target checks.
- Host-testable policy code with fake operation tables.

### Enterprise Usage

- Portable C libraries with stable ABI boundaries.
- Plugin/callback interfaces with explicit context lifetime.
- Configuration matrices that fail unsupported combinations at build time.
- Static analysis, warning-clean builds, target matrix builds, and reviewable
  deviation records.
- Protocol parsers that decode bytes explicitly rather than relying on native
  layout.

## 9. Common Bugs And Review Risks

### Bitwise And Register Bugs

- Shifting a signed `1` into or beyond the sign bit.
- Shift count equal to or larger than the promoted operand width.
- `~mask` affecting promoted high bits unexpectedly.
- Mixing logical `&&`/`||` with bitwise `&`/`|`.
- Using decimal magic numbers instead of named masks.
- Performing unsafe read-modify-write on write-one-to-clear or reserved bits.
- Assuming one C access equals one bus transaction of the required width.
- Casting away `volatile`.
- Treating `volatile` as atomic, a lock, or a memory barrier.

### Macro And Build Bugs

- Multiple evaluation of arguments with side effects.
- Missing parentheses and precedence changes.
- Multi-statement macro breaking `if`/`else`.
- Macro name collisions or leaking configuration globally.
- Different translation units compiled with inconsistent feature macros.
- Disabled logging/assertion macros changing evaluation or side effects.
- Packing pragmas not restored after use.
- Header definitions causing multiple-definition or hidden-state defects.

### Callback, FSM, And OOP Bugs

- Calling a null or incompatible function pointer.
- Dangling context pointer after unregister, shutdown, or stack return.
- Callback invoked in an execution context it was not designed for.
- Reentrant callback mutating registration or dispatch storage.
- Unvalidated external value used as a table index.
- Tag/state updated separately from payload or action.
- Missing default/invalid-transition behavior.
- Partial operation table initialization.
- Mixing allocation families or omitting deinitialization.
- Opaque object representation leaking into public headers.

### Representation And Buffer Bugs

- Casting protocol bytes to a packed/native structure.
- Assuming bit-field order or enum width.
- Ignoring endianness.
- Ring-buffer head/tail wrap or full/empty ambiguity.
- Believing volatile indices solve producer/consumer races.
- Reading a variadic argument with the wrong promoted type.
- Missing `va_end` or an absent/malformed sentinel.

### Asynchronous-Context Bugs

- Logging, allocating, freeing, locking, or performing cleanup in a signal or
  interrupt handler without an explicit platform guarantee.
- Sharing complex state through `volatile`.
- Calling non-reentrant callbacks from an interrupt.
- Losing events by using one Boolean flag when event count matters.

## 10. Debugging And Verification Notes

- Compile with strict warnings in the selected C mode and treat warning policy
  as toolchain-specific.
- Inspect preprocessed output with `cc -E` and macro definitions with compiler
  preprocessing options.
- Generate assembly with `-S` or inspect disassembly to confirm actual register
  load/store width and count; do not infer hardware ordering from C source.
- Use `-Wconversion`, `-Wsign-conversion` where supported, `-Wshift-count-*`,
  `-Wswitch-enum`, `-Wcast-qual`, `-Wmissing-prototypes`, `-Wstrict-prototypes`,
  and macro-related diagnostics after validating compiler support.
- Use UBSan on host-testable code for invalid shifts, signed overflow,
  misalignment, null access, and bounds defects.
- Use ASan for host-side lifetime and buffer tests; sanitizers normally do not
  model physical MMIO.
- Add `_Static_assert` checks for widths, mask ranges, table lengths, and
  required layout facts that are truly part of the selected ABI.
- Unit-test bit operations with zero, top-bit, all-bits, and invalid-index
  cases.
- Unit-test FSMs from transition tables, including every invalid state/event
  pair.
- Test callback registration, missing callback, unregister, repeated callback,
  reentrancy, and dead-context scenarios.
- Replace MMIO with fake register or HAL operations in host tests.
- Compare multiple optimization levels and compilers where volatile or inline
  behavior matters.
- Use static analysis for macro side effects, qualifier removal, dead states,
  table bounds, incompatible callbacks, and concurrency defects.
- On target, use watchpoints, trace, logic analyzers, or peripheral observability
  only after the host-testable logic is isolated.

## 11. Best Practices

- Use unsigned, width-aware masks and validate every dynamic bit index.
- Derive register definitions from authoritative vendor data.
- Centralize MMIO behind a narrow target layer.
- Treat `volatile`, atomicity, ordering, and mutual exclusion as separate
  concerns.
- Prefer `static inline` functions to computational function-like macros.
- Keep configuration macros centralized and reject impossible combinations.
- Make callbacks carry an explicit context and document execution/lifetime.
- Make dispatch tables `const` where possible and validate indices before use.
- Keep FSM transition policy explicit and test the whole transition matrix.
- Hide representation with opaque types and expose explicit lifecycle
  functions.
- Keep HAL interfaces small enough to fake in tests.
- Encode external data byte by byte with explicit width and endianness.
- Avoid dynamic allocation in timing-critical or safety-sensitive paths unless
  the allocation policy is deliberate and bounded.
- Prefer typed/count-based APIs to variadic interfaces.
- Apply MISRA/BARR-C/CERT through a documented project policy, tooling, and
  justified deviations rather than slogan-level compliance.

## 12. Interview Angles

### Beginner

- Show how to set, clear, toggle, and test a bit.
- Explain why masks should normally use unsigned types.
- Explain `const`, `volatile`, `static`, and `extern`.
- Explain why a callback is a function pointer and what a context pointer adds.
- Explain include guards and the main risks of function-like macros.

### Mid-Level

- Explain why `volatile` is not thread-safe or atomic.
- Design a register accessor without spreading physical addresses through the
  application.
- Compare a function-like macro with `static inline`.
- Implement a callback registration API with context and lifetime rules.
- Design a command dispatch table that validates external command IDs.
- Implement a table-driven FSM with invalid-transition handling.
- Explain the hazards of packed structures and bit-fields.
- Review a single-producer/single-consumer ring buffer synchronization plan.

### Senior

- Separate language guarantees, compiler guarantees, architecture ordering, and
  peripheral semantics in an MMIO design review.
- Explain read-modify-write hazards for special-function registers.
- Design a portable HAL with opaque state and versioned operation tables.
- Decide between switch-based, table-driven, and object-based state machines.
- Review callback reentrancy, cancellation, teardown, and asynchronous
  invocation.
- Build a safety-oriented rule and deviation strategy using MISRA, BARR-C,
  CERT C, compiler warnings, and static analysis.
- Explain how to verify generated code without coupling all business logic to
  hardware.
- Identify the unsafe operations in a signal/interrupt-style handler and move
  them into normal control flow.

## 13. Practice And Example Targets

### Basic

- Implement width-safe bit-mask helpers and tests.
- Compare macro and `static inline` implementations with side-effecting inputs.
- Build a header/source pair demonstrating `static`, `extern`, include guards,
  and one-definition ownership.

### Intermediate

- Implement a callback plus `void *context` API with unregister support.
- Implement a validated command-dispatch table.
- Implement an enum-based traffic-light or sensor FSM and exhaustively test its
  transition matrix.
- Implement an explicit big-endian encoder/decoder without packed casts.
- Implement a fixed-capacity ring buffer and document its single-threaded or
  synchronized execution model.

### Advanced

- Create a fakeable GPIO/timer HAL using an opaque handle and const ops table.
- Separate register access from a pure device-control policy and run the policy
  in host tests.
- Review a mock status/control register with write-one-to-clear bits and design
  safe APIs for its distinct semantics.
- Build the same target abstraction with two compile-time configurations and
  fail invalid combinations using preprocessing checks.

### Suggested Compile-Oriented Example Set

- `bits/bit_masks.c`
- `macros/macro_vs_inline.c`
- `callbacks/callback_context.c`
- `fsm/table_fsm.c`
- `hal/fakeable_hal.c`
- `protocol/endian_codec.c`
- `ring_buffer/ring_buffer.c`
- Optional expected-failure cases for invalid shifts, macro duplicate
  evaluation, and stale callback context.

No example files are created in this step.

## 14. Gaps And External Validation Needed

### Exact C And Toolchain Behavior

- Select the lesson baseline, recommended as C17 with clearly labeled C23
  additions.
- Validate exact C rules for `restrict`, volatile access, inline linkage,
  variadic arguments, signal handlers, and integer promotions against the
  selected ISO C edition or a public working draft.
- Validate every proposed warning against the actual GCC and Clang versions
  available in the repository environment.
- Compile all examples in strict language mode and at multiple optimization
  levels.

### Target-Specific Behavior

- Any concrete MMIO example needs a fictional register model or an explicitly
  selected public target specification.
- Real register addresses, access widths, barriers, interrupt attributes,
  sections, packing, and calling conventions require target and compiler
  documentation.
- Do not claim portable behavior for vendor attributes, pragmas, linker
  sections, or startup mechanisms.
- Hardware register examples must distinguish ordinary read/write,
  write-one-to-clear, read-to-clear, reserved, and self-clearing fields.

### Safety And Compliance

- Later learner-facing material should describe MISRA principles without
  reproducing licensed rule content.
- A project claiming MISRA compliance needs the licensed guideline, compliance
  process, tool qualification decisions, deviation handling, and evidence.
- CH07 should own deeper static-analysis governance and organization-wide
  coding-standard practice.

### Mapped Source Gaps

- None of the mapped Notion chapters teaches bit masks, MMIO, `restrict`, HAL
  design, C opaque types, C operation tables, table-driven FSMs, ring buffers,
  endian conversion, or C variadic functions at the required embedded depth.
- `notion-10-2` is C++-first and contains oversimplified performance and
  portability claims.
- `notion-10-5` is C++-heavy and does not develop the C callback-plus-context
  idiom or embedded execution-context contract deeply enough.
- `notion-10-7` focuses on hosted process signals, contains nonportable signal
  numbers, and includes operations that are unsafe in handlers.
- These gaps require external C, safety, compiler, and target documentation
  during lesson and example generation.

## 15. Quality Gate For Later Outputs

- Preserve `MUST / Deep` depth for embedded readers.
- Connect every major concept to mechanism, code, practical firmware use,
  bugs, debugging, best practice, interview reasoning, and practice.
- Include all six master-required comparisons.
- Keep `volatile`, atomicity, barriers, synchronization, and MMIO semantics
  distinct.
- Keep portable ISO C separate from compiler extension and target contract.
- Correct the mapped macro and signal misconceptions rather than repeating
  them.
- Use compile-oriented, warning-clean examples and mark target-only pseudocode.
- Make ownership, callback lifetime, reentrancy, and execution context explicit.
- Include enterprise/testability implications, not only bare-metal syntax.
- Do not paste this audit metadata into learner-facing documents.
- Do not use Linux Device Driver or kernel-driver material.

## 16. Output Targets

| Output | Current status | Intended scope |
| --- | --- | --- |
| `coverage/topic-briefs/06-advanced-c-for-embedded.md` | Created | Source audit, merged concepts, corrections, comparisons, bugs, debugging plan, interview angles, gaps, and external validation trace |
| `knowledge/06-advanced-c-for-embedded.md` | Created and reviewed | Deep learner-facing embedded C lesson without audit metadata |
| `interview/06-advanced-c-for-embedded.md` | Created and reviewed | Beginner, mid-level, and senior embedded C interview pack |
| `examples/06-advanced-c-for-embedded/README.md` and small `.c` files | Created and reviewed | Compile-oriented bit, macro, callback, FSM, HAL, endian, and ring-buffer examples |

Audit metadata must remain under `coverage/` and must not be copied into
learner-facing documents.
