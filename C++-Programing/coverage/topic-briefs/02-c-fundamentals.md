# Topic Brief 02 - C Fundamentals

## 1. Canonical Routing

| Field | Validated value |
| --- | --- |
| `learning_path_number` | `02` |
| Title | C Fundamentals |
| `slug` | `c-fundamentals` |
| Requested topic | Core C program structure, types, objects, scope, storage duration, conversions, operators, control flow, functions, and basic standard I/O |
| Master source | `master-ch02` |
| Required Notion sources | `notion-1-2`, `notion-1-3`, `notion-1-4` |
| Topic Brief | `coverage/topic-briefs/02-c-fundamentals.md` |
| Knowledge target | `knowledge/02-c-fundamentals.md` |
| Interview target | `interview/02-c-fundamentals.md` |
| Example target | `examples/02-c-fundamentals/README.md` |

Validation result: the number, title, slug, master source, required Notion
sources, and all canonical output paths match `LEARNING_PATH.md`.

## 2. Source Coverage

### Internal Sources Read

| Source label | Path | Coverage contribution |
| --- | --- | --- |
| `master-ch02` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, CH02 | `MUST / Deep` priority, CH01 prerequisite, keyword scope, embedded-risk rule, and interview focus |
| `guide-section-03` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 3 | MUST-depth requirements |
| `guide-section-04` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 4 | Full-chapter structure |
| `guide-section-06` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 6 | Required C/C++ comparison format for `const`, `static`, `extern`, and `volatile` |
| `guide-section-07` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 7 | Depth control and practical-example guidance |
| `guide-section-09` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 9 | Trusted C, embedded, safety, and enterprise source routing |
| `guide-section-10` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 10 | English-first style and compile-oriented example rules |
| `guide-section-11` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 11 | Full lesson, interview, review, and comparison output expectations |
| `guide-section-12` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 12 | Required comparison inventory |
| `guide-section-14` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 14 | Final quality checklist |
| `notion-index` | `docs/C++ Notion/C++ Programming.md` | Notion chapter inventory and source identity validation |
| `notion-1-2` | `docs/C++ Notion/Chapter 1-2 Variables, Data Types, Storage & Scope.md` | Variables, initialization, constants, storage-class concepts, scope, lifetime, fundamental types, signedness, and interview prompts |
| `notion-1-3` | `docs/C++ Notion/Chapter 1-3 Type Conversion & Casting.md` | Implicit conversion, explicit conversion, narrowing risks, integer promotion intent, mixed arithmetic, and conversion-related interview prompts |
| `notion-1-4` | `docs/C++ Notion/Chapter 1-4 Operators, Input Output, Control Flow & Loops.md` | Operators, precedence, short-circuiting, branches, loops, `break`, `continue`, `goto`, and practical control-flow examples |

All three mapped Notion chapter files were read in full. No mapped Notion
source was skipped.

### External References Consulted

| Label | Reference | Validation purpose |
| --- | --- | --- |
| `external-iso-c` | WG14 N3220, ISO/IEC 9899:2024 working draft: <https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf> | Exact C rules for hosted/freestanding execution, scope, linkage, storage duration, types, conversions, expressions, declarations, initialization, statements, functions, and the standard library |
| `external-cppreference-conversions` | <https://en.cppreference.com/w/c/language/conversion> | Navigable summary of integer promotions, usual arithmetic conversions, assignment conversions, and floating/integer conversion behavior |
| `external-cppreference-integers` | <https://en.cppreference.com/w/c/types/integer> | Exact-width, least-width, fast-width, maximum-width, and pointer-capable integer typedef availability |
| `external-cppreference-arithmetic` | <https://en.cppreference.com/w/c/language/operator_arithmetic> | Arithmetic, bitwise, shift, signed-overflow, and unsigned-modulo behavior |
| `external-cppreference-main` | <https://en.cppreference.com/w/c/language/main_function> | Hosted `main` forms and the implementation-defined entry point of a freestanding program |

External validation is required for this topic. The mapped Notion chapters are
C++ lessons and do not precisely cover C declarations, C storage-class rules,
C casts, C standard I/O, C function definitions, hosted versus freestanding
startup, or the exact conversion and overflow rules.

### Coverage Status

`COMPLETE_FOR_CURRENT_SCOPE`: all canonical internal sources were read, the
C-specific gaps were validated against the C standard draft and cppreference C,
and the Topic Brief, knowledge lesson, interview pack, and compile-ready example
suite have been created and reviewed.

## 3. Priority And Dependencies

- Priority: `MUST`
- Depth: Deep
- Prerequisite: CH01 Build And Compilation Model
- Required prior mental model: source files, headers, preprocessing,
  translation units, declarations versus definitions, compilation, linking,
  and basic diagnostics.
- Role in learning path: establish the language mechanics needed by every later
  C topic, especially memory, pointers, compound types, embedded C, and
  industrial C practices.
- Master-specific risk emphasis: type width, signed/unsigned interactions,
  overflow, and uninitialized automatic objects must appear even in beginner
  explanations.

## 4. Scope And Depth Boundaries

### Deep In This Topic

- Hosted C program shape and the role of `main`.
- Statements, expressions, declarations, definitions, and initialization.
- Fundamental arithmetic types, `_Bool`/`bool` version notes, `size_t`, and
  `<stdint.h>`.
- Scope, linkage, storage duration, and object lifetime as separate concepts.
- `auto`, `register`, `static`, `extern`, `typedef`, `const`, and introductory
  `volatile` usage with version-sensitive notes.
- Integer promotions, usual arithmetic conversions, signed/unsigned
  comparisons, narrowing, integer division, and overflow behavior.
- Arithmetic, relational, logical, bitwise, assignment, conditional, and
  `sizeof` operators.
- `if`, `switch`, `for`, `while`, `do while`, `break`, `continue`, and bounded
  treatment of `goto`.
- Function declarations, definitions, parameters, return values, file-local
  `static` functions, and recursion.
- Basic C standard I/O sufficient for examples, including checked return values
  and format-string correctness.

### Introduce But Defer

- Pointers and array decay beyond what is needed to explain `main` arguments,
  string input, and function interfaces: CH04.
- Detailed memory layout, effective type, alignment, padding, and object
  representation: CH03 and CH05.
- Dynamic allocation and ownership: CH03 and CH04.
- Deep `volatile`, memory-mapped I/O, callbacks, and embedded register access:
  CH06.
- Strict aliasing details: CH03/CH04 controlled advanced scope.
- Secure-input APIs and industrial coding standards: CH07.
- C++ cast operators, references, classes, streams, range-based `for`, and
  templates: CH08 and later.

## 5. Merged Concept Map

### Program Shape And Execution

- In a hosted implementation, teach the standard `int main(void)` and
  `int main(int argc, char *argv[])` forms, plus equivalent forms permitted by
  the implementation.
- Reaching the closing brace of `main` returns zero in hosted C.
- A freestanding implementation can use an implementation-defined startup
  function and execution environment. Embedded firmware therefore does not
  universally begin through a hosted `main` contract, even though many
  toolchains still call a user `main` after startup initialization.
- A function declaration communicates a function type; a function definition
  supplies the body. Parameters are local objects initialized from arguments.
- A non-`void` function must return an appropriate value on every reachable
  path, except for the special hosted-`main` rule.

### Statements, Expressions, And Objects

- An expression computes a value, designates an object/function, and/or causes
  side effects. An expression statement is an expression followed by `;`.
- A declaration specifies identifiers and types. A definition additionally
  creates an object or supplies a function body where required.
- An object is a region of data storage with a type, value, storage duration,
  and potentially an identifier and linkage.
- Initialization establishes an initial value as part of a definition;
  assignment modifies an existing object later.
- Automatic objects without an initializer can hold indeterminate values.
  Reading such a value is unsafe and can produce undefined behavior under the
  applicable C rules; the lesson should use the conservative rule: initialize
  before reading.
- Objects with static or thread storage duration are initialized before use,
  with zero initialization when no explicit initializer supplies another value.

### Types And Portability

- The standard guarantees minimum ranges and an ordering of integer conversion
  ranks, not universal byte widths for `short`, `int`, `long`, or `long long`.
- `sizeof(char)` is always `1`, but `CHAR_BIT` defines how many bits are in a
  byte. Do not equate a C byte with eight bits without a platform assumption.
- Plain `char`, `signed char`, and `unsigned char` are distinct types. Whether
  plain `char` behaves as signed or unsigned is implementation-defined.
- `_Bool` is the core Boolean type in C99 through C17; `<stdbool.h>` supplies
  convenient `bool`, `true`, and `false` macros in those versions. C23 makes
  `bool`, `true`, and `false` language keywords/predefined constants and
  deprecates the old compatibility header role.
- `size_t` is the unsigned integer type used for sizes and the result of
  `sizeof`; it must be printed with the matching `%zu` conversion.
- `<stdint.h>` exact-width types such as `uint32_t` exist only when the
  implementation provides a type with exactly that width and no padding bits.
  They are not universally guaranteed.
- Use `uint_leastN_t` when at least N bits are required, `uint_fastN_t` when a
  fast type of at least N bits is desired, and `uint32_t` when an external
  format or hardware contract truly requires exactly 32 bits.
- `float`, `double`, and `long double` have implementation-defined
  representations and ranges within standard constraints. Decimal values are
  generally not represented exactly in binary floating-point.

### Scope, Linkage, Storage Duration, And Lifetime

- Scope answers where a name is visible. Storage duration answers how long an
  object exists. Linkage answers whether declarations in different scopes or
  translation units denote the same entity.
- Block-scope ordinary local objects normally have automatic storage duration
  and no linkage.
- File-scope objects and functions have static storage duration where
  applicable; file-scope `static` gives internal linkage.
- A block-scope `static` object has block scope, static storage duration, and no
  linkage. Its value persists across calls, but its name remains local.
- `extern` commonly declares an entity with external linkage; it does not
  allocate a special "extern storage area" or make every declaration a
  definition.
- A file-scope declaration without `static` commonly has external linkage.
  Tentative definitions require careful treatment so learners do not create
  duplicate program-wide state.
- `typedef` creates an alias name for a type; it does not create a distinct new
  type.
- `const` qualifies an object type against modification through that lvalue; in
  C it does not automatically make an integer object an integer constant
  expression.
- `volatile` requires observable accesses according to the abstract machine,
  but it does not provide atomicity, inter-thread synchronization, or a general
  compiler memory barrier.

### Conversions And Arithmetic

- Replace the Notion "one promotion hierarchy" model with the actual stages:
  integer promotions first where required, then the usual arithmetic
  conversions choose a common real type.
- Integer promotions convert integer types of rank no greater than `int` to
  `int` if all values fit, otherwise to `unsigned int`.
- Mixed signed/unsigned arithmetic depends on rank and representable ranges.
  A negative signed operand can therefore convert to a large unsigned value.
- Converting to an unsigned integer type produces the congruent value modulo
  one more than the maximum representable value.
- Converting an out-of-range integer value to a signed integer type is
  implementation-defined or can raise an implementation-defined signal.
- Converting finite floating-point to integer discards the fractional part; if
  the integral result is not representable, behavior is undefined.
- Signed integer overflow is undefined behavior. Unsigned arithmetic wraps
  modulo `2^N` for an N-bit unsigned type.
- Integer division truncates toward zero; remainder follows the identity
  `(a / b) * b + a % b == a` when the quotient is representable.
- Division or remainder by zero is undefined behavior.
- Floating-point comparison and equality need tolerance/domain-aware reasoning,
  not blind exact equality for computed values.

### Operators, Sequencing, And Control Flow

- Precedence and associativity determine parsing; they do not generally
  determine operand evaluation order.
- `&&`, `||`, the conditional operator's selected operand, and the comma
  operator provide sequencing guarantees relevant to safe guard expressions.
- Modifying an object more than once, or modifying and reading it for another
  purpose without sequencing, can cause undefined behavior.
- `=` performs assignment and yields the assigned value; accidental assignment
  in a condition is valid C and should be caught by warnings and style.
- Logical operators produce `int` values `0` or `1`. Any scalar zero is false;
  a nonzero scalar value is true in a controlling expression.
- Bitwise operations should normally use unsigned operands. Signed shifts,
  excessive shift counts, and left shifts whose results are not representable
  create implementation-defined or undefined behavior.
- A shift count that is negative or at least the width of the promoted left
  operand is undefined behavior.
- `switch` performs integral promotions on its controlling expression. Case
  labels must be integer constant expressions and unique after conversion.
- Fallthrough is a language feature; accidental fallthrough is a maintainability
  bug and should be made explicit where supported.
- `for` is suited to explicit iteration state, `while` to condition-driven
  repetition, and `do while` when one execution is required.
- `break` exits the nearest loop or `switch`; `continue` advances the nearest
  loop according to that loop form.
- `goto` is not universally forbidden. Keep it out of normal control flow, but
  acknowledge its disciplined use for centralized cleanup in C when a function
  owns several resources.

### Functions And Recursion

- A prototype gives parameter types and enables argument checking. Avoid
  old-style, non-prototype function declarations.
- C passes arguments by value. To let a function modify caller-owned state, a
  pointer must be passed; detailed pointer mechanics belong to CH04.
- Array parameters are adjusted to pointer types in function declarations, so
  `sizeof` on such a parameter does not recover the caller's array length.
- A file-scope `static` function has internal linkage and is private to the
  translation unit.
- Recursion requires a base case and consumes stack space per active call.
  Embedded use should account for bounded depth and stack budget; tail-call
  optimization is not guaranteed.
- Prefer small functions with explicit contracts, narrow state mutation,
  checked inputs, and return values that communicate success or failure.

### Basic C Standard I/O

- Use `<stdio.h>` functions rather than importing C++ stream material into the
  C lesson.
- `printf`/`scanf` conversion specifications must match the actual promoted
  argument or destination type. A mismatch can cause undefined behavior.
- Check return values from input and output functions.
- Prefer `fgets` plus validated parsing for line-oriented input. Avoid `gets`,
  which was removed from the C standard because it cannot be used safely.
- Bound `%s` input widths when `scanf`-family input is deliberately used.
- Distinguish end-of-file from input failure and malformed input.

## 6. Usage Angles

### C Usage

- Small programs organized around declarations, definitions, functions, and
  explicit return-code handling.
- Fixed-width protocol fields only where the platform provides the required
  exact-width types.
- File-local state and helper functions using `static`, with public declarations
  in headers and one definition of shared state.
- Bit masks, state variables, counters, input validation, and bounded loops.

### C++ Usage

- Use comparisons only to prevent C++ syntax from leaking into C:
  `_Bool`/C `bool` versus C++ `bool`, C casts versus named C++ casts, `printf`
  versus iostreams, C storage-class behavior versus C++ additions, and C
  recursion/function rules versus overloads/references/default arguments.
- Do not teach C++ classes, `constexpr`, `mutable`, namespaces, range-based
  `for`, `static_cast`, `dynamic_cast`, `const_cast`, or `reinterpret_cast` as
  C features.

### Embedded Usage

- Select types from hardware/protocol requirements rather than desktop
  assumptions.
- Treat overflow, signedness, truncation, shift width, and implicit conversion
  warnings as correctness issues.
- Use bounded loops and recursion only with a defensible worst-case stack and
  execution-time analysis.
- Keep `volatile` introductory and explicit: it can model externally changing
  objects, but atomicity and ordering require separate mechanisms.
- Distinguish hosted examples from freestanding startup behavior without
  introducing Linux Device Driver or kernel-driver material.

### Enterprise Usage

- Compile with a strong warning baseline such as
  `-std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion`, adjusting
  policy deliberately for the project and compiler.
- Treat warnings as review inputs; do not silence conversion diagnostics with
  unexplained casts.
- Use clear naming, small scopes, explicit initialization, narrow interfaces,
  and one ownership point for mutable global state.
- Test boundary values, invalid input, integer limits, zero divisors, and loop
  termination conditions.
- Record the language version and implementation assumptions in the build.

## 7. Required Comparisons

| Comparison | Required teaching point |
| --- | --- |
| Declaration vs definition vs initialization vs assignment | Separate introducing an entity/type, creating the entity, establishing its first value, and later modification |
| Scope vs storage duration vs lifetime vs linkage | These dimensions are related but not interchangeable |
| Automatic local vs static local vs file-scope object | Visibility, persistence, initialization, linkage, reentrancy, and shared-state risks |
| Internal linkage vs external linkage | Translation-unit-private implementation versus program-wide symbol identity |
| `static` object vs `static` function | Persistent object lifetime versus translation-unit-private function name |
| `extern` declaration vs definition | Cross-unit declaration versus the one required storage-providing definition |
| Fundamental integer types vs `<stdint.h>` types | Minimum-range portable types versus exact/least/fast width contracts |
| `int` vs `size_t` | General signed arithmetic versus unsigned object-size/count type and conversion hazards |
| Signed vs unsigned arithmetic | Negative-value representation, mixed comparisons, modulo arithmetic, and diagnostics |
| Integer promotion vs usual arithmetic conversions | Promotion of narrow integer types versus common-type selection for mixed operands |
| Implicit conversion vs explicit C cast | Automatic conversion versus an explicit request that still does not prove safety |
| Signed overflow vs unsigned wrap | Undefined behavior versus modulo arithmetic |
| Arithmetic vs logical vs bitwise operators | Numeric calculation, truth-valued control, and bit-level manipulation |
| `&&`/`||` vs `&`/`|` | Short-circuit logical operations versus always-evaluated bitwise operations |
| Precedence vs evaluation order/sequencing | Parsing does not generally specify when operands are evaluated |
| `if`/`else` vs `switch` | General conditions versus integral dispatch and fallthrough behavior |
| `for` vs `while` vs `do while` | Iteration-state clarity, entry testing, and guaranteed first execution |
| Function declaration vs prototype vs definition | Name/type introduction, parameter checking, and implementation body |
| Iteration vs recursion | Explicit loop state versus call-stack growth and bounded-depth requirements |
| `const` in C vs C++ | C read-only qualification and constant-expression limitations versus C++ differences |
| `static`, `extern`, and `volatile` in C vs C++ | Shared concepts with important language-specific rules and additional C++ contexts |
| C cast vs C++ named casts | One broad C syntax versus intent-specific C++ operators |
| `<stdio.h>` vs C++ iostreams | C formatted I/O contracts versus typed stream abstractions; keep C examples in `<stdio.h>` |

## 8. Common Bugs And Failure Modes

- Reading an uninitialized automatic object.
- Assuming `int` is always 32 bits or a byte is always eight bits.
- Assuming `uint8_t` or another exact-width typedef exists on every target.
- Storing a `sizeof` result in `int` or printing `size_t` with `%d`.
- Comparing a negative signed value with an unsigned size/count.
- Relying on signed overflow to wrap.
- Narrowing a value with an implicit conversion or cast without a range check.
- Converting an out-of-range floating value to an integer.
- Dividing by zero or evaluating `INT_MIN / -1`.
- Using signed values for bit masks or relying on right shift of a negative
  signed value.
- Shifting by a negative count or by a count greater than or equal to the
  promoted operand width.
- Writing expressions with unsequenced reads and modifications.
- Confusing `=` with `==` in conditions.
- Using bitwise `&` or `|` where short-circuit `&&` or `||` is required.
- Missing `break` in a `switch` or hiding intentional fallthrough.
- Off-by-one loop bounds, unsigned countdown loops that never terminate, and
  failure to update the loop state.
- Returning no value from a non-`void` function.
- Calling a function without a visible correct prototype.
- Declaring shared state `extern` in one place but defining it zero or multiple
  times incorrectly.
- Treating a static local as thread-safe shared state without synchronization
  analysis.
- Recursing without a reachable base case or bounded depth.
- Mismatching `printf`/`scanf` format strings and argument types.
- Ignoring standard I/O return values or using unbounded string input.
- Treating `volatile` as atomic or thread-safe.
- Importing C++ syntax from the mapped notes into a C source file.

## 9. Debugging Notes

- Compile in a declared language mode and enable warnings:
  `cc -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion`.
- Add `-Wshadow`, `-Wformat=2`, and `-Wswitch-enum` where compatible with the
  project warning policy.
- Use `-Werror` in controlled CI configurations after the warning baseline is
  understood; avoid making third-party headers impossible to consume.
- Build debug variants with `-O0 -g3` and inspect object values, types, and
  control flow in a debugger.
- Use `-fsanitize=undefined,address` on supported hosted targets to catch
  signed overflow, invalid shifts, out-of-bounds access, and related runtime
  faults. Sanitizers do not replace target testing.
- Inspect preprocessing and macro-expanded expressions with `cc -E` when an
  operator or declaration behaves unexpectedly.
- Print integer values with type-correct format macros from `<inttypes.h>` when
  working with fixed-width types.
- Reduce conversion bugs to an example that prints operand types, signedness,
  limits from `<limits.h>`, and values before and after conversion.
- Test loops at zero, one, maximum valid count, and one-past-boundary inputs.
- For recursion, log depth in a test build and inspect stack usage using
  compiler stack reports or target tooling where available.
- For format warnings, let the compiler validate literal format strings and do
  not cast arguments merely to suppress a diagnostic.

## 10. Best Practices

- Initialize every automatic object before its first read.
- Choose types from value ranges and interface contracts, not habitual width
  assumptions.
- Use `size_t` for object sizes and array counts while handling signed
  interoperability deliberately.
- Prefer unsigned types for bit manipulation and signed types for ordinary
  arithmetic that can conceptually go below zero.
- Use exact-width integer types only when exact width is part of the external
  contract and verify their availability.
- Keep conversion points visible, range-check narrowing conversions, and avoid
  casts whose only purpose is to silence warnings.
- Parenthesize mixed operator classes for readability, but still understand
  precedence and sequencing.
- Keep conditions free of unintended side effects.
- Make `switch` fallthrough explicit and include a deliberate `default` policy.
- Write loops with clear invariants, bounds, and termination progress.
- Put function prototypes in self-contained headers and definitions in one
  source file.
- Use file-scope `static` for private helpers/state, but minimize mutable global
  and static state.
- Check return values from library calls and functions that can fail.
- Prefer `fgets` plus checked parsing for text input.
- Keep recursion bounded and justified in embedded or safety-sensitive code.
- Document implementation-defined assumptions and test them in the build.
- Explain `volatile` narrowly and never present it as a synchronization tool.

## 11. Interview Angles

### Junior

- What is the difference between declaration, definition, initialization, and
  assignment?
- What are scope and lifetime, and why are they not the same?
- How do `if`, `switch`, `for`, `while`, and `do while` differ?
- What is integer division?
- What is the difference between `=` and `==`?
- What do `break` and `continue` affect?
- What is a function prototype?

### Middle

- Compare an automatic local, static local, file-scope `static`, and `extern`
  object.
- Explain integer promotions and the usual arithmetic conversions.
- Why can `-1 < sizeof array` evaluate unexpectedly?
- Why is signed overflow undefined while unsigned arithmetic wraps?
- When should `uint32_t` be preferred over `int`, and when should it not?
- Explain precedence versus evaluation order.
- Diagnose an infinite unsigned countdown loop.
- Explain why a `printf` format mismatch can be undefined behavior.

### Senior

- Define scope, linkage, storage duration, and lifetime precisely and apply them
  to a multi-file example.
- Design a portable integer-type policy for an embedded protocol parser.
- Review an expression for signedness, promotion, overflow, and sequencing
  hazards.
- Explain hosted versus freestanding execution and how that affects `main`.
- Decide whether recursion is acceptable under a bounded stack budget.
- Explain what `volatile` guarantees and what it does not.
- Propose warning, sanitizer, and boundary-test coverage for a low-level C
  module.

## 12. Practice Tasks

- Basic: write a hosted C program with `int main(void)`, initialize variables of
  several arithmetic types, print them with correct format specifiers, and
  compile with the chapter warning set.
- Basic: classify ten snippets as declaration, definition, initialization,
  assignment, expression, or statement.
- Basic: implement the same bounded counter with `for`, `while`, and
  `do while`, then explain the behavioral difference for a zero limit.
- Intermediate: demonstrate block scope, static local persistence, file-scope
  internal linkage, and one correctly defined `extern` object across two source
  files.
- Intermediate: create and repair a signed/unsigned comparison bug involving
  `size_t`.
- Intermediate: inspect the result types and values of mixed `char`, `short`,
  `int`, unsigned, and floating-point expressions.
- Intermediate: implement checked numeric input with `fgets` and validated
  conversion, including malformed and out-of-range cases.
- Intermediate: implement a flag register using an unsigned integer, named
  masks, set/clear/test operations, and guarded shift counts.
- Advanced: review a small C module containing uninitialized reads, signed
  overflow, unsafe shifts, accidental assignment, switch fallthrough, and a
  format mismatch; use warnings and UBSan to find the defects.
- Advanced embedded: choose integer types for sensor readings, counters,
  timestamps, and a wire protocol, documenting range, width, overflow, and
  conversion assumptions without using driver/kernel material.
- Advanced: compare iterative and recursive implementations, measure or inspect
  maximum recursion depth, and justify the production choice.

## 13. Gaps, Corrections, And External Validation Needs

### Notion Corrections Required

- The Notion sources are C++ lessons. Their `iostream`, `std::string`,
  namespaces, classes, references, range-based `for`, `constexpr`, `mutable`,
  `thread_local` presentation, and named C++ casts must not be copied into the
  C lesson as C syntax.
- The claimed linear promotion hierarchy is inaccurate. C uses integer
  promotions plus the usual arithmetic conversions, with rank, signedness, and
  representable range controlling the result.
- "Smaller to larger prevents data loss" is too broad. Width alone does not
  define promotion safety, and integer-to-floating conversion can lose
  precision.
- Converting `300` to `char` is not portably guaranteed to produce `44`;
  exact results depend on `CHAR_BIT`, plain-`char` signedness, and the
  implementation's out-of-range signed conversion behavior.
- Fundamental type sizes shown in the notes are typical platform examples, not
  C guarantees.
- Plain `char` is not synonymous with ASCII or UTF-8, and C does not require
  ASCII.
- A local automatic object should be described as having an indeterminate
  initial value, not simply "garbage on the stack." Storage location is an
  implementation detail.
- Storage class, scope, linkage, storage duration, and physical memory segment
  must not be collapsed into one table.
- `extern` does not imply zero initialization by itself; the actual object
  definition and storage duration determine initialization.
- `static` at block scope and `static` at file scope share spelling but affect
  different dimensions.
- Bit shifts are not generally safe substitutes for multiplication/division.
  Signedness, representability, negative values, and shift counts matter.
- "Prefer pre-increment because it is faster" is not a meaningful rule for
  scalar integers in modern optimizing C compilers.
- `switch` is not inherently faster than `if`/`else`; code generation depends
  on values, target, optimization, and compiler decisions.
- Operator precedence does not imply evaluation order.
- `goto` should be taught as restricted, not absolutely forbidden, because
  centralized cleanup is an established C use case.

### Internal Coverage Gaps Filled Externally

- CH02 requires function declarations, definitions, parameters, return values,
  static functions, and recursion, but no mapped Notion function chapter is
  assigned to this topic.
- The mapped I/O material is C++ stream-based and does not cover `<stdio.h>`,
  format contracts, checked input, or C-specific input hazards.
- The mapped sources do not distinguish hosted and freestanding C execution.
- Exact C behavior for integer promotions, mixed signedness, overflow, shifts,
  storage duration, and exact-width typedef availability requires the C
  standard or an equivalent precise C reference.

### Validation Notes

- The knowledge lesson uses C17 as a broadly deployed baseline and labels C23
  differences explicitly. If another baseline is chosen, all `_Bool`/`bool`,
  `auto`, attributes, and library statements must be rechecked.
- Compiler-warning examples are validated with a GCC-compatible compiler.
  Warning flags and diagnostic wording remain implementation-specific.
- Sanitizer examples are hosted-toolchain diagnostics, not guarantees for
  freestanding embedded targets.
- Safety-standard rules are not required for this fundamentals brief. Deeper
  MISRA, CERT C, and BARR-C guidance belongs primarily to CH06 and CH07.
- No POSIX source and no Linux Device Driver or kernel-driver source is required
  or permitted for this topic.

## 14. Output Targets

| Output | Status after this step | Intended scope |
| --- | --- | --- |
| `coverage/topic-briefs/02-c-fundamentals.md` | Created | Source audit, corrections, expansion plan, and external validation trace |
| `knowledge/02-c-fundamentals.md` | Created and reviewed | MUST-depth learner-facing C lesson |
| `interview/02-c-fundamentals.md` | Created and reviewed | Junior/middle/senior interview pack |
| `examples/02-c-fundamentals/README.md` | Created and verified | Compile-ready C examples and diagnostic exercises |

Chapter 02 is complete for the current scope.
