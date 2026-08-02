# M01 Exercises — Coding Standards and Memory Foundation

> **Status:** `EXERCISE_APPROVED`
>
> **Gate:** `EXERCISE_RESTORATION — APPROVED`
>
> **Language baseline:** ISO C99

## 1. Exercise Policy and Source Authority

This file contains exactly four canonical build exercises, in the original DevLinux order:

1. Session 01 — Exercise 1: IPv4 parser
2. Session 01 — Exercise 2: MAC parser
3. Session 02 — Exercise 1: Memory Segment Analyzer
4. Session 02 — Exercise 2: Stack Depth Monitor

The DevLinux session files define each exercise's identity, title, required interface, primary learning objective, required behavior, expected-output intent, and submission structure.

This restoration follows these rules:

- Do not rename, merge, split, omit, replace, or reorder an exercise.
- Do not include a reference implementation or algorithmic solution.
- Preserve source-defined behavior separately from technical corrections.
- Label repository-added validation separately from source requirements.
- Correct technically inaccurate claims without editing the immutable DevLinux source.
- Do not treat compiler, analyzer, or selected-guideline output as proof of correctness or complete compliance.
- Keep exactly one solution artifact for each exercise.

## 2. Common Engineering Baseline

Unless an exercise states a narrower requirement:

- Implement in ISO C99.
- Provide a Makefile with at least `all` and `clean` targets.
- Build with the DevLinux-required warning policy:

```text
-std=c99 -Wall -Wextra -pedantic -Werror
```

For GCC versions that document `-Wpedantic`, the repository may use the equivalent recorded command:

```text
-std=c99 -Wall -Wextra -Wpedantic -Werror
```

Record the compiler and version used for evidence.

Apply the relevant BARR-C practices without treating them as ISO C semantics:

- explicit braces;
- initialized local state;
- fixed-width types where width is part of the interface;
- defensive pointer handling;
- reviewable naming;
- Doxygen documentation for public functions.

Run the DevLinux-required tools:

```text
cppcheck
clang-tidy
```

Record enough tool context to make the result reproducible, including the tool version and relevant configuration or command. Resolve relevant diagnostics. A clean run does not prove functional correctness, MISRA compliance, CERT compliance, portability, or target safety.

### 2.1 Source-reference corrections applied in this file

| Correction | Applied policy |
| --- | --- |
| MISRA Directive 4.14 | The identifier is not verified in the supplied MISRA C:2012 reference. It is recorded as a source inconsistency and is not used as applicable guidance. |
| MISRA Directive 4.11 | It applies to validity of values passed to library functions. General API null and input validation comes from the API contract and applicable language/security guidance. |
| MISRA Directive 4.12 | It is a Required guideline concerning dynamic allocation. A violating design needs the project's authorized deviation when making a compliance claim. |
| MISRA Rule 11.6 | It is not a rule about the ordinary C conversion from `void *` to another object-pointer type. |
| Function pointer and `.text` | Generated function code may be inspected as a code symbol. A function-pointer object is data and must not be described as the function's `.text` bytes. |
| Unrelated pointer subtraction | Portable C pointer subtraction is not defined between unrelated objects. Numeric address magnitudes are permitted here only as explicitly implementation-scoped observations on a supporting implementation. |
| Stack direction and frame size | Neither direction nor a fixed bytes-per-call value is portable or guaranteed. |
| Recursion guard | A configured guard reduces experimental risk but does not prove the real overflow boundary or make recursion compliant with MISRA Rule 17.2. |
| CERT source path | The stale `C_Books/` repository path is not used. Any CERT identifier retained from source provenance must be verified against an official SEI CERT C source before a compliance claim. |

---

# 3. Session 01 — Exercise 1

## Safe Network Address Parser — IPv4 to `uint32_t`

**Canonical source:** `session-01.md`, `Exercise_1 [build]`

## 3.1 Scenario

An embedded networking component receives an IPv4 address as ASCII text from a configuration file, command-line interface, or web configuration surface. The component must validate the external representation before converting it into a numeric value used internally.

A numeric representation can reduce storage and simplify some comparisons. Exact performance depends on the compiler, target, optimization, data flow, and workload. The primary objective is a deterministic validation and error contract, not an unmeasured performance claim.

## 3.2 Learning objective

Implement a safe IPv4 text parser that:

- accepts the source-defined valid form;
- rejects malformed or out-of-range input;
- avoids undefined behavior;
- writes the documented numeric result;
- exposes a clear success/failure contract.

## 3.3 Required interface

```c
int8_t parse_ipv4(const char *ip_str, uint32_t *p_ip_out);
```

The interface is fixed and must not be renamed or replaced.

## 3.4 Source-preserved requirements

The implementation shall:

- defensively check `ip_str` and `p_ip_out`;
- validate each IPv4 component in the inclusive range `0` through `255`;
- return `0` on success;
- return a negative error value when the format is invalid;
- avoid the `atoi` family;
- use the common C99, BARR-C, Doxygen, Makefile, compiler, `cppcheck`, and `clang-tidy` baseline;
- use no dynamic allocation.

## 3.5 Deterministic format clarification

The DevLinux source requires a valid IPv4 address but does not fully define every lexical edge case. For a deterministic repository contract, accept only:

```text
decimal-component "." decimal-component "." decimal-component "." decimal-component
```

Each component shall:

- contain one or more ASCII decimal digits;
- contain no sign;
- have a numeric value from `0` through `255`.

The complete input shall:

- contain exactly four components;
- contain exactly three dots;
- contain no missing component;
- contain no extra component;
- contain no leading or trailing whitespace;
- contain no trailing characters after the fourth component.

Leading zeroes are accepted unless the implementation documents a stricter repository policy. They remain decimal digits and must not trigger octal interpretation.

This clarification defines the repository's accepted syntax; it is not an additional exercise identity.

## 3.6 Numeric-result contract

For components `A.B.C.D`, construct the returned numeric value as:

```text
(A << 24) | (B << 16) | (C << 8) | D
```

after each component has been validated.

This defines the integer value independently of the host's in-memory byte order. The exercise does not ask the implementation to expose the object's byte representation or to perform socket API conversion.

## 3.7 Output state on failure

The DevLinux source does not define whether `*p_ip_out` is preserved or reset after invalid input.

The implementation shall choose and document one of these contracts:

- output remains unchanged on failure; or
- output is set to a documented sentinel before returning failure.

Tests shall follow the documented contract. Writing a partially parsed result without documenting that behavior is not acceptable.

## 3.8 Source-defined expected behavior

These cases come directly from the source exercise:

| Input/setup | Required result |
| --- | --- |
| `"192.168.1.50"` with a valid output object | Return `0`; output is `0xC0A80132` (`3232235826`) |
| `"256.0.0.1"` with a valid output object | Return a negative error |
| `NULL` input with a valid output object | Return a negative error without dereferencing the input |

The source also requires process exit code `0` when the exercise test program succeeds and nonzero when its self-checks fail.

## 3.9 Additional repository validation

The following cases strengthen the deterministic contract but do not create new DevLinux exercises:

| Case | Expected behavior |
| --- | --- |
| `"0.0.0.0"` | Success; output `0x00000000` |
| `"255.255.255.255"` | Success; output `0xFFFFFFFF` |
| Fewer than four components | Failure |
| More than four components | Failure |
| Empty component such as `"1..2.3"` | Failure |
| Signed component such as `"-1.2.3.4"` or `"+1.2.3.4"` | Failure |
| Non-decimal character | Failure |
| Trailing text or whitespace | Failure |
| Empty string | Failure |
| `NULL` output pointer | Failure without writing |

Additional tests must be labelled as repository validation rather than source-defined expected output.

## 3.10 Safety and standards review notes

- General pointer checks are required by the API contract.
- MISRA Directive 4.11 is relevant only when values are passed to library functions; it is not the universal reason for checking both public API pointers.
- MISRA Directive 4.14 remains unverified and must not be cited as verified guidance.
- MISRA Rule 9.1 is relevant to reading initialized automatic objects.
- MISRA Rule 10.3 applicability depends on the actual expression and destination essential types.
- MISRA Rule 21.7 is relevant if the implementation attempts to use the prohibited conversion functions.
- MISRA Directive 4.12 is consistent with this source exercise's no-dynamic-allocation design, but selected adherence does not establish full compliance.
- Any CERT identifiers inherited from the source are supporting review prompts only and require verification against the official SEI CERT C source.

## 3.11 Acceptance criteria

### Source acceptance

- The exact required interface is present.
- The source-defined success and failure cases pass.
- `"192.168.1.50"` produces `0xC0A80132`.
- No `atoi`-family function is used.
- Required Makefile and documentation artifacts are present.

### Repository technical acceptance

- The complete syntax contract is deterministic.
- Component accumulation cannot exceed the validated range.
- No read occurs beyond the terminating null character.
- No write occurs through an invalid output pointer.
- Output-on-failure behavior is documented and tested.
- Compiler and required tool commands are recorded.
- No unsupported MISRA, CERT, security, portability, or performance claim is made.

## 3.12 Submission

Preserve the source submission structure:

```text
Exercise_1/
├── main.c        (required)
├── Makefile      (required — targets: all, clean)
└── *.h           (if any)
```

A header is included only when it declares a genuine public interface.

---

# 4. Session 01 — Exercise 2

## Safe Network Address Parser — MAC Address to `uint8_t[6]`

**Canonical source:** `session-01.md`, `Exercise_2 [build]`

## 4.1 Scenario

An embedded Ethernet component receives a MAC address as ASCII configuration text. The parser must convert valid text into exactly six bytes while rejecting malformed input and preserving all input and output bounds.

## 4.2 Learning objective

Implement a defensive parser that validates:

- pointer arguments;
- hexadecimal digits;
- delimiter placement;
- exact field count;
- exact field width;
- output bounds.

## 4.3 Required interface

```c
int8_t parse_mac(const char *mac_str, uint8_t *p_mac_out);
```

`p_mac_out` designates caller-provided storage for six bytes. The interface is fixed.

## 4.4 Source-preserved requirements

The implementation shall:

- defensively check `mac_str` and `p_mac_out`;
- accept uppercase and lowercase hexadecimal digits;
- validate the placement of `:` or `-` delimiters;
- avoid reading past the input terminator;
- avoid writing beyond the six-byte output object;
- return `0` on success;
- return a negative error value on failure;
- use the common C99, BARR-C, Doxygen, Makefile, compiler, `cppcheck`, and `clang-tidy` baseline.

## 4.5 Deterministic format clarification

Accept either of these complete forms:

```text
HH:HH:HH:HH:HH:HH
HH-HH-HH-HH-HH-HH
```

where each `H` is one ASCII hexadecimal digit.

The complete input shall contain:

- exactly six fields;
- exactly two hexadecimal digits per field;
- exactly five delimiters;
- one consistent delimiter throughout the input;
- no prefix such as `0x`;
- no leading or trailing whitespace;
- no trailing text.

Mixed delimiters are rejected under the repository contract.

## 4.6 Output state on failure

The DevLinux source does not define the state of the six-byte output after failure.

The implementation shall document whether:

- all six output bytes remain unchanged on failure; or
- all six output bytes are reset to a documented value.

A partially written result with no documented contract is not acceptable. A robust implementation may validate into temporary local state before committing all six bytes, but the exercise does not prescribe an algorithm.

## 4.7 Source-defined expected behavior

These cases come directly from the source exercise:

| Input/setup | Required result |
| --- | --- |
| `"00:1A:2B:3C:4D:5E"` | Return `0`; output bytes are `00 1A 2B 3C 4D 5E` |
| `"00-1a-2b-3c-4d-5e"` | Return `0`; lowercase input is accepted |
| `"00:1A:2B:3C:4D"` | Return a negative error |
| `"00:1A:2B:3C:4D:5E:6F"` | Return a negative error |
| `"00:1A:2B:3C:4D:5G"` | Return a negative error |
| `NULL` input with a valid output object | Return a negative error without dereferencing the input |

The source requires exit code `0` when the exercise test program succeeds and nonzero when its self-checks fail.

## 4.8 Additional repository validation

| Case | Expected behavior |
| --- | --- |
| `NULL` output pointer | Failure without writing |
| Empty string | Failure |
| One-digit field | Failure |
| Three-digit field | Failure |
| Mixed `:` and `-` delimiters | Failure |
| Prefix such as `0x00:...` | Failure |
| Leading or trailing whitespace | Failure |
| Trailing text | Failure |
| All-zero and all-`FF` addresses | Success |
| Lowercase and uppercase combinations | Success |

These are labelled repository validation, not additional source exercises.

## 4.9 Safety and standards review notes

- General public-pointer validation follows the interface contract.
- MISRA Directive 4.11 applies only to values passed to library functions.
- MISRA Directive 4.14 remains unverified and is not used as verified guidance.
- MISRA Rule 9.1 is relevant to initialized automatic state.
- MISRA Rule 10.3 must be applied to the submitted conversion expressions, not cited automatically.
- MISRA Rule 14.2 applies only if a submitted `for` loop falls under its conditions.
- MISRA Rule 15.5 is Advisory; an early error return is not an ISO C defect.
- MISRA Rule 21.7 applies if prohibited text-conversion functions are used.
- CERT identifiers listed by the original source remain review prompts pending verification against official SEI CERT C material.

## 4.10 Acceptance criteria

### Source acceptance

- The exact required interface is present.
- Both delimiter forms in the source examples are accepted.
- Uppercase and lowercase hexadecimal digits are handled.
- The source-defined malformed cases are rejected.
- Output writes remain within six bytes.
- Required Makefile and documentation artifacts are present.

### Repository technical acceptance

- The complete lexical format is deterministic.
- Mixed delimiters, wrong field widths, extra characters, and null output are handled.
- Input traversal never advances beyond the terminating null character.
- Output-on-failure behavior is documented and tested.
- Compiler and required tool commands are recorded.
- No unsupported compliance or security claim is made.

## 4.11 Submission

Preserve the source submission structure:

```text
Exercise_2/
├── main.c        (required)
├── Makefile      (required — targets: all, clean)
└── *.h           (if any)
```

A header is included only when it declares a genuine public interface.

---

# 5. Session 02 — Exercise 1

## Memory Segment Analyzer — Map, Measure, and Verify

**Canonical source:** `session-02.md`, `Exercise_1 [build]`

## 5.1 Scenario

A firmware engineer needs evidence about how one selected compiler, linker, object format, target, and runtime represent program code and objects.

The exercise observes common Embedded C categories while preserving the three-layer distinction:

```text
C object semantics
→ generated sections and symbols
→ runtime and physical placement
```

## 5.2 Learning objective

Create one controlled program that:

- declares representative objects;
- prints valid object-address observations;
- obtains a heap object;
- records one automatic local object;
- inspects generated code and symbols with tools;
- explains every result as build-specific evidence.

## 5.3 Required interface

```c
void print_memory_map(void);
```

The interface is fixed.

## 5.4 Source-preserved observation categories

The source exercise requires representative observations for:

- function code;
- a `const` global object;
- a nonzero-initialized global object;
- an uninitialized global object;
- dynamically allocated storage;
- an automatic local object.

It also requires:

- runtime address output;
- numeric address comparison;
- dynamic allocation and release;
- GNU `size` evidence;
- GNU `nm` evidence;
- the common C99, BARR-C, Doxygen, Makefile, compiler, `cppcheck`, and `clang-tidy` baseline.

## 5.5 Technical correction — function code is not a function-pointer object

The source labels a function pointer as `.text`. This is technically inaccurate.

The restored exercise shall instead:

- inspect the relevant function symbol with `nm`;
- optionally inspect code sections with `objdump -h`;
- avoid requiring a function-pointer-to-`void *` conversion for `%p`;
- avoid claiming that a stored function-pointer object is generated function code.

Object addresses may be printed with `%p` by passing the corresponding object pointer converted to `void *`, as required by the selected formatted-output contract.

## 5.6 Required representative objects

The program shall contain at least:

- one `const` global object;
- one nonzero-initialized writable global object;
- one uninitialized writable global object;
- one successfully allocated heap object when allocation succeeds;
- one automatic local object.

The source-level labels describe intended categories. The final section and symbol classifications shall be verified from the exact artifact rather than asserted from the declarations alone.

## 5.7 Dynamic-allocation contract

The program shall:

- request a small allocation;
- check the result before dereference;
- handle allocation failure deterministically;
- release a successful allocation exactly once;
- not access the allocation after `free()`.

The lab intentionally uses dynamic allocation even though MISRA Directive 4.12 is a Required guideline against its use. The exercise must not be described as MISRA-compliant merely because failure and cleanup are handled.

## 5.8 Technical correction — numeric address magnitudes

The source asks for distances between memory segments using pointer arithmetic. Portable C does not define subtraction between pointers to unrelated objects.

To preserve the observational learning intent:

- use only object-pointer values;
- convert through `void *` where required by the implementation contract;
- use `uintptr_t` only when the implementation provides it;
- calculate direction-independent numeric magnitudes between converted values;
- label each result as an implementation-specific address-magnitude observation;
- do not call the value portable pointer distance, segment size, free space, ordering, or capacity.

The lab assumes an implementation that defines `uintptr_t`. If it does not, the numeric-magnitude part cannot be implemented under the fixed source interface without an explicit platform-specific alternative.

## 5.9 Source-defined output intent

Addresses vary by build and run. The program shall produce clearly labelled observations for:

```text
function-code evidence
const global object
initialized writable global object
uninitialized writable global object
heap object
automatic local object
numeric address magnitudes
```

The source sample's exact addresses and apparent ordering are illustrative only.

The test program returns exit status `0` after successful completion and nonzero after a documented failure such as allocation failure or failed self-check.

## 5.10 Required tool evidence

After building, run:

```text
size <binary>
nm <binary>
```

At minimum, inspect the relevant initialized and uninitialized global symbols.

Append the required commands and relevant output in a clearly labelled comment block at the bottom of `main.c`, preserving the source submission intent.

Also record:

- compiler and version;
- target;
- build flags;
- exact binary;
- relevant linker context.

### Evidence interpretation

- GNU `size` Berkeley columns are accounting summaries.
- Read-only data may be included in the `text` summary.
- `text`, `data`, and `bss` columns are not necessarily literal output-section names.
- `nm` symbol letters depend on object format and platform.
- optimization, stripping, merging, and garbage collection can change symbol evidence.
- neither tool proves physical target placement by itself.

Optional evidence:

```text
objdump -h <binary>
readelf -S <binary>   # ELF only
```

Optional output supports investigation but does not replace the source-required `size` and `nm` evidence.

## 5.11 Additional repository validation

| Case | Expected behavior |
| --- | --- |
| Allocation succeeds | Required object observations are printed; allocation is released |
| Allocation fails | No null dereference; documented failure result |
| Tool evidence collected | Exact commands and relevant output retained |
| Function-code evidence | Obtained from symbols/sections, not invalid `%p` assumptions |
| Numeric magnitudes enabled | Direction-independent and explicitly implementation-scoped |
| Optimization changes symbol output | Difference is documented rather than treated as exercise failure without investigation |

## 5.12 Safety and standards review notes

- MISRA Directive 4.12 conflict is intentional laboratory scope.
- MISRA Rule 9.1 is relevant to initialized automatic objects.
- MISRA Rule 11.4 is relevant to object-pointer/integer conversion review.
- MISRA Rule 11.6 is not the ordinary `malloc()` conversion rule.
- MISRA Rule 22.1 supports deliberate release of dynamically allocated standard-library resources.
- `uintptr_t` is optional under C99.
- `const` does not guarantee `.rodata` or Flash.
- BSS-like storage still consumes runtime storage.
- selected tool evidence is not a compliance assessment.

## 5.13 Acceptance criteria

### Source acceptance

- The exact `print_memory_map()` interface is present.
- All source observation categories are represented.
- Allocation failure is checked.
- Successful allocation is released.
- Required `size` and `nm` evidence is appended as instructed.
- Required Makefile and documentation artifacts are present.

### Repository technical acceptance

- No function-pointer object is labelled as generated `.text` code.
- No unsupported function-pointer-to-`void *` `%p` requirement is used.
- Unrelated pointers are not subtracted using C pointer arithmetic.
- Numeric magnitudes are direction-independent and implementation-scoped.
- `uintptr_t` availability is stated as an implementation assumption.
- `size` and `nm` output is interpreted accurately.
- No complete MISRA, portability, physical-placement, or stack/heap-layout claim is made.

## 5.14 Submission

Preserve the source submission structure:

```text
Exercise_1/
├── main.c        (required)
├── Makefile      (required — targets: all, clean)
└── *.h           (if any)
```

Append the required `size` and `nm` commands and relevant output as a labelled comment block at the bottom of `main.c`.

---

# 6. Session 02 — Exercise 2

## Stack Depth Monitor — Measure Stack Usage & Detect Overflow Risk

**Canonical source:** `session-02.md`, `Exercise_2 [build]`

## 6.1 Scenario

An embedded function recurses during a bounded laboratory operation. The program observes call depth and a build-specific address-magnitude proxy, then stops further recursion when a configured threshold is reached.

The threshold is an experiment guard. It is not the verified physical overflow boundary.

## 6.2 Learning objective

Implement the source-required recursive monitor while learning that:

- active recursive calls consume call-related resources;
- stack direction is not universal;
- frame size is not fixed by ISO C;
- an address proxy is not exact stack usage;
- a guard reduces risk but does not prove safety;
- recursion conflicts with MISRA Rule 17.2.

## 6.3 Required interface

```c
int8_t recurse_with_monitor(uint32_t current_depth,
                            uint32_t max_depth,
                            const uintptr_t stack_base_addr,
                            uint32_t stack_limit_bytes);
```

The interface is fixed. The lab assumes that the implementation provides `uintptr_t`.

## 6.4 Source-preserved requirements

The implementation shall:

- retain the recursive design;
- declare an initialized local marker in every reached invocation;
- capture `stack_base_addr` from the converted address of a local object in `main()`;
- print the reached depth;
- print the local marker address;
- print a stack-address magnitude proxy;
- stop before another recursive call when the configured guard is reached;
- return `0` when the requested maximum depth is reached without the guard;
- return `-1` when the guard is reached;
- use the common C99, BARR-C, Doxygen, Makefile, compiler, `cppcheck`, and `clang-tidy` baseline.

## 6.5 Direction-independent magnitude clarification

The source's suggested subtraction assumes a downward-growing stack. The restored exercise shall not make that assumption.

For converted values `base` and `current`, compute a numeric magnitude equivalent to:

```text
larger value - smaller value
```

without performing a wrapping subtraction first.

This value shall be labelled:

```text
observed address magnitude
```

or:

```text
stack-address proxy
```

It shall not be labelled exact stack bytes, exact frame size, remaining stack, or portable stack usage.

## 6.6 Guard and depth behavior

At each invocation:

1. initialize the local marker;
2. obtain its implementation-scoped numeric address representation;
3. calculate and print the magnitude;
4. when the magnitude is greater than or equal to `stack_limit_bytes`, print the warning and return `-1` without recursing again;
5. otherwise, when `current_depth` is greater than or equal to `max_depth`, return `0`;
6. otherwise, invoke the next recursion level.

This ordering preserves the source's guard and base-case intent without assuming that the guard predicts a real overflow.

## 6.7 Source-defined expected-output intent

The source expects output shaped like:

```text
depth
local marker address
observed stack-related magnitude
final success or guard result
```

The displayed 64-byte increments and depth 64 in the source example are illustrative. They are not required values.

Two outcomes are required:

| Condition | Required result |
| --- | --- |
| Requested maximum depth is reached before the guard | Return `0` |
| Observed magnitude reaches or exceeds the configured guard | Warn and return `-1` without another recursive call |

The exercise test program returns exit status `0` when its expected self-check scenario succeeds and nonzero when a self-check fails.

## 6.8 Recursion and safety correction

MISRA C:2012 Rule 17.2 prohibits direct and indirect recursion.

Therefore:

- this exercise intentionally conflicts with the rule;
- the guard does not make recursion compliant;
- the guard does not prove the real overflow limit;
- a clean analyzer run does not create a deviation;
- a production MISRA compliance claim requires the project's authorized treatment;
- production worst-case stack analysis requires target-specific methods outside this lab.

The source phrase “stop safely before a real overflow” is corrected to:

> Stop further recursion when the configured proxy threshold is reached, reducing experiment risk without claiming the real overflow boundary.

## 6.9 Input and arithmetic constraints

The implementation shall ensure that:

- `current_depth` and `max_depth` are handled consistently;
- the recursion step cannot wrap the depth counter under the documented contract;
- magnitude calculation does not underflow before direction is selected;
- `stack_limit_bytes` is interpreted as a configured proxy threshold;
- local state is initialized before use.

The exercise does not require a pointer parameter, so generic null-pointer checks must not be invented.

## 6.10 Additional repository validation

Run at least:

| Scenario | Expected behavior |
| --- | --- |
| Conservative guard and small bounded depth | Reaches `max_depth`; returns `0` |
| Deliberately small guard | Returns `-1` before another recursive call |
| Multiple reached levels | Reports each reached depth and local-marker observation |
| Address values increase numerically | Magnitude remains valid under the chosen implementation-scoped method |
| Address values decrease numerically | Magnitude remains valid under the chosen implementation-scoped method |
| Different optimization level | Changed proxy values are documented, not treated as fixed frame-size evidence |

Tests must use conservative limits. Do not deliberately seek a real overflow.

## 6.11 Safety and standards review notes

- MISRA Directive 4.1 is relevant to minimizing runtime failures, but the proxy is not proof of stack safety.
- MISRA Rule 9.1 is relevant to initialized automatic state.
- MISRA Rule 11.4 is relevant to object-pointer/integer conversion.
- MISRA Rule 17.2 directly conflicts with the required recursion.
- MISRA Rule 15.5 is Advisory; multiple explicit completion paths are not ISO C defects.
- `uintptr_t` availability and pointer-to-integer behavior are implementation assumptions.
- tool-clean output is not a compliance or production stack-safety claim.

## 6.12 Acceptance criteria

### Source acceptance

- The exact recursive interface is present.
- Every reached invocation declares and reports a local marker.
- The requested maximum depth produces success when reached before the guard.
- The guard produces `-1` without another recursive call.
- Required Makefile and documentation artifacts are present.

### Repository technical acceptance

- No stack-growth direction is assumed.
- No fixed bytes-per-call result is required.
- No proxy result is described as exact or portable stack usage.
- The Rule 17.2 conflict is explicit.
- The guard is not presented as proof of safety or a compliance exception.
- Conservative tests cover both success and guard outcomes.
- Compiler and required tool commands are recorded.

## 6.13 Submission

Preserve the source submission structure:

```text
Exercise_2/
├── main.c        (required)
├── Makefile      (required — targets: all, clean)
└── *.h           (if any)
```

Keep the work a focused recursive monitoring laboratory. Do not replace it with a non-recursive implementation and do not expand it into a production stack-management framework.

---

# 7. Exercise-Restoration Audit Checklist

The exercise gate may be approved only when all items pass:

- Exactly four canonical exercises remain.
- Original order and titles remain.
- All four required interfaces remain unchanged.
- DevLinux source-defined behavior is visibly separated from repository validation.
- Technical corrections are visibly separated from immutable source claims.
- No full solution or algorithm is included.
- Directive 4.14 is not presented as verified.
- Directive 4.11 is not used as a universal pointer-validation rule.
- Rule 11.6 is not misapplied to ordinary `malloc()` result conversion.
- Function code is not represented by a function-pointer object.
- Unrelated object pointers are not subtracted as portable C distances.
- `uintptr_t` is identified as optional and implementation-dependent.
- Section names and physical placement are not presented as ISO C guarantees.
- Dynamic allocation and recursion conflicts are explicit.
- The recursion guard is not presented as proof of real stack safety.
- Required `size` and `nm` evidence is preserved for Session 02 Exercise 1.
- Exact submission structures are preserved.
- Each exercise continues to map to exactly one solution artifact.
- Compiler and analyzer evidence is scoped to the recorded configuration.
- No complete MISRA, CERT, security, portability, or target-correctness claim is made.
