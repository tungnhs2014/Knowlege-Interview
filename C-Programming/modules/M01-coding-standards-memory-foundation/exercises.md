# M01 Exercises — Coding Standards and Memory Foundation

> **Status:** EXERCISE_APPROVED.

These four build exercises preserve the order, identity, learning objectives, core requirements, test intent, and submission intent of the corresponding DevLinux sessions. They are implementation specifications, not reference solutions. Technical wording is corrected only where needed for ISO C, toolchain, or safety accuracy.

## Common Engineering Baseline

Unless an exercise states a narrower requirement, implement and submit the exercise using C99 and a Makefile with at least `all` and `clean` targets. Compile with `-std=c99 -Wall -Wextra -pedantic -Werror`, or with the project’s documented equivalent when a compiler uses `-Wpedantic`. Use the applicable BARR C style conventions: fixed-width integer types where the interface requires them, braces for control statements, defensive pointer handling, and Doxygen documentation for public functions.

Run `cppcheck` and `clang-tidy`, and resolve relevant warnings and errors.

## Session 01 — Exercise 1

**Source provenance:** `session-01.md`, `Exercise_1 [build]` — *Safe Network Address Parser — IPv4 to `uint32_t`*.

### Scenario

An embedded network component receives an IPv4 address as ASCII text from a configuration file, command-line interface, or web configuration surface. The component must validate that external text before it becomes a numeric address used by the program.

### Objective

Implement a safe conversion from a dotted-decimal IPv4 string to a `uint32_t` result. The numeric representation is generally more compact and is often cheaper to compare than text, but the exact cost depends on the target, compiler, and workload. Correct validation and an explicit error contract are the primary goals.

### Required Interface

```c
int8_t parse_ipv4(const char *ip_str, uint32_t *p_ip_out);
```

### Requirements

- Defensively reject a null input string and a null output pointer.
- Accept exactly four decimal components separated by exactly three dots.
- Accept each component only when its value is in the inclusive range `0` through `255`.
- Reject malformed, incomplete, excessive, signed, or otherwise invalid external input. Do not use the `atoi` family: it does not provide the syntax, range, and error reporting contract required here.
- Return `0` on success and a documented negative error value on invalid input.
- You may document an output-state convention for rejected input as an API-design consideration; this exercise does not prescribe output preservation on failure.
- Use a conversion order that produces the conventional network-order numeric value. Do not rely on the host’s in-memory byte order to define the returned integer value.
- Do not use dynamic allocation for this exercise.

### Required Test Cases / Expected Behavior

| Input and setup | Expected behavior |
| --- | --- |
| `"192.168.1.50"` and a valid output pointer | Return `0`; the result is `0xC0A80132` (`3232235826`). |
| `"0.0.0.0"` and `"255.255.255.255"` | Return `0` and produce the corresponding boundary values. |
| `"256.0.0.1"` | Return a negative error; do not accept an out-of-range component. |
| A string with fewer or more than four components | Return a negative error. |
| A string with a missing component, non-decimal text, a sign, or trailing invalid text | Return a negative error. |
| `NULL` input string | Return a negative error without dereferencing the pointer. |
| `NULL` output pointer | Return a negative error without attempting a write. |

### Engineering Constraints

- Keep parsing bounded by the input terminator and avoid reading beyond it.
- Check a component’s range before narrowing it or incorporating it into the aggregate result.
- Initialize local state before use and make every error path explicit.
- Provide a Doxygen contract that identifies valid inputs, output parameter behavior, and return values.

### Selected Standards/Safety Notes

| Concern | Applicable engineering guidance |
| --- | --- |
| External input and library arguments | MISRA C:2012 Directive 4.11 and 4.14; CERT EXP34-C, ERR33-C, and MSC24-C support validating pointers, arguments, and conversion errors. |
| Bounds and text handling | CERT ARR30-C and STR31-C are relevant when traversing externally supplied text. |
| Integer construction and narrowing | CERT INT32-C and INT31-C support checking component range and conversion before storing or combining values. |
| Dynamic allocation | The source exercise intentionally uses only automatic and fixed-size storage, consistent with the intent of MISRA C:2012 Directive 4.12. |

These selected references identify review concerns; they are not a complete MISRA or CERT compliance claim.

### Acceptance Criteria

- The required interface compiles under the common engineering baseline.
- All required valid and invalid cases are demonstrably handled, including null pointers, component count, component range, and malformed text.
- The successful `192.168.1.50` conversion produces `0xC0A80132`.
- No `atoi`-family conversion is used.
- The implementation does not read outside the supplied string or write outside the valid output object.
- Doxygen documentation and Makefile targets are present, and the required compiler and tool checks complete with relevant warnings and errors resolved.
- The submission makes no unqualified MISRA, CERT, target-performance, or security-compliance claim.

### Submission

Submit `Exercise_1/main.c` and a `Makefile` containing at least `all` and `clean`. Include a header only when the submission has a genuine public interface to declare.

## Session 01 — Exercise 2

**Source provenance:** `session-01.md`, `Exercise_2 [build]` — *Safe Network Address Parser — MAC Address to `uint8_t[6]`*.

### Scenario

An Ethernet interface receives a MAC address as configuration text. The parser must turn a valid textual address into exactly six bytes without accepting malformed input or exceeding either input or output bounds.

### Objective

Implement a defensive conversion from a MAC-address string to six `uint8_t` elements while enforcing hexadecimal syntax, delimiter rules, exact length, and pointer validity.

### Required Interface

```c
int8_t parse_mac(const char *mac_str, uint8_t *p_mac_out);
```

`p_mac_out` designates storage for exactly six output bytes supplied by the caller.

### Requirements

- Defensively reject a null input string and a null output pointer.
- Accept hexadecimal digits `0`–`9`, `a`–`f`, and `A`–`F`.
- Accept the documented MAC forms using either colon delimiters or hyphen delimiters; preserve a consistent delimiter convention within one input.
- Require exactly six byte fields and reject inputs with fewer or more than six byte fields.
- Never write beyond output element `5` and never read beyond the input terminator.
- Return `0` on success and a documented negative error value on invalid input.

### Required Test Cases / Expected Behavior

| Input and setup | Expected behavior |
| --- | --- |
| `"00:1A:2B:3C:4D:5E"` | Return `0`; output bytes are `00 1A 2B 3C 4D 5E`. |
| `"00-1a-2b-3c-4d-5e"` | Return `0`; lowercase hexadecimal input is accepted. |
| An input with only five byte fields | Return a negative error. |
| An input with seven byte fields | Return a negative error without exceeding the six-byte output object. |
| A field containing `G`, another non-hexadecimal character, a missing digit, or an inconsistent delimiter | Return a negative error. |
| `NULL` input string | Return a negative error without dereferencing it. |
| `NULL` output pointer | Return a negative error without writing output. |

### Engineering Constraints

- Treat the input as untrusted external text and validate syntax before storing a byte.
- Keep all index or pointer movement within the string and the six-element destination domain.
- Validate the value of a hexadecimal field before narrowing it to `uint8_t`.
- Use fixed-width types, explicit control flow, defensive pointer checks, braces, and Doxygen documentation consistent with the common baseline.
- Build with the strict compiler flags and run the required `cppcheck` and `clang-tidy` checks.

### Selected Standards/Safety Notes

| Concern | Applicable engineering guidance |
| --- | --- |
| Pointer validity and external input | MISRA C:2012 Directive 4.11 and 4.14; CERT EXP34-C and ERR33-C. |
| Input/output bounds | CERT ARR30-C and ARR38-C are relevant when walking the text and filling the six-byte result. |
| Hexadecimal conversion and narrowing | CERT INT32-C and INT31-C support validation before arithmetic and conversion. |
| Text APIs | CERT MSC24-C supports avoiding text-to-integer routines that do not provide the needed error contract; STR31-C applies if a submitted design copies text. |

These are selected safety-review notes, not a complete compliance assessment.

### Acceptance Criteria

- The required interface and six-byte output contract are documented and compile under C99 with the common baseline flags.
- Both required delimiter forms and both hexadecimal letter cases are accepted.
- Every specified malformed, too-short, too-long, out-of-bounds, and null-pointer case is rejected deterministically.
- Output writes remain within six elements, and input traversal remains within the string terminator.
- The submission contains a Doxygen interface contract and Makefile `all`/`clean` targets, and the required compiler and tool checks complete with relevant warnings and errors resolved.
- The solution does not claim that tool output alone proves MISRA, CERT, security, or target correctness.

### Submission

Submit `Exercise_2/main.c` and a `Makefile` containing at least `all` and `clean`. Include a header only when it declares a genuine public interface.

## Session 02 — Exercise 1

**Source provenance:** `session-02.md`, `Exercise_1 [build]` — *Memory Segment Analyzer — Map, Measure, and Verify*.

### Scenario

A firmware engineer needs evidence about how one selected compiler, linker configuration, target, and runtime build place representative program objects. The exercise uses a small program and GNU inspection tools to compare the firmware image with runtime object-address observations.

### Objective

Create a controlled memory-map observation program and verify its build with `size` and `nm`. `objdump -h` is recommended when actual section-header inspection would add useful evidence. The exercise teaches useful embedded conventions for `.text`, `.rodata`, `.data`, `.bss`, heap, and stack while preserving the distinction between those conventions and ISO C guarantees.

### Required Interface

```c
void print_memory_map(void);
```

### Requirements

- Include representative program objects for a `const` global object, an initialized non-zero global object, an uninitialized global object, dynamically allocated storage, and an automatic local object.
- Observe the addresses of those **object** instances at runtime using the object-pointer form required by the selected implementation’s `%p` contract.
- Inspect function code and symbols with `nm`; `objdump -h` may supplement this with section-header evidence. Do not describe a stored function-pointer object as `.text`, and do not require conversion of a function pointer to `void *` for `%p` output. Function code or its symbol is commonly associated with a text-like section, while a function-pointer object, if one exists, is data.
- Allocate a small heap object for the lab, check allocation failure, and release it on every path after successful allocation.
- Where the lab reports numeric address-difference magnitudes, use `uintptr_t` only for converted **object-pointer** values. `uintptr_t` is optional in C; this lab assumes an implementation that provides a suitable type. Treat reported magnitudes as observations of one build and run, not portable pointer subtraction, ordering, or inter-object distance guarantees.
- Compile under the common baseline and use BARR C style, fixed-width types where applicable, defensive pointers, braces, Doxygen, Makefile targets, and the required tool runs.
- Append the required `size` and `nm` commands and relevant output to a clearly labelled comment block at the bottom of `main.c`, as required by the source exercise. Recording compiler, target, and linker/build context with that evidence is recommended.

### Required Test Cases / Expected Behavior

| Observation or test | Expected behavior |
| --- | --- |
| Program execution after successful allocation | Prints clearly labelled runtime observations for the `const` global object, initialized global object, uninitialized global object, heap object, and stack local object. Actual addresses and ordering are build-specific. |
| Function-code inspection | `nm` identifies the relevant function symbol or code section without printing a function pointer through an object-pointer `%p` conversion. |
| Numeric observation enabled on a supporting implementation | Prints only magnitude-style numeric observations derived from converted object-pointer values; it does not present them as portable pointer distances. |
| Allocation failure path | Reports or returns the documented failure outcome without dereferencing a null allocation result. |
| Allocation success path | Releases the allocated object exactly once before program exit. |
| `size <binary>` | Records the selected format’s size summary. In GNU `size` Berkeley format, `text`, `data`, and `bss` are summaries and `text` can include read-only data; they are not necessarily literal ELF section names. |
| `nm <binary>` | Records the relevant symbols. |
| Optional `objdump -h <binary>` | When used, records actual section headers; use it when the review needs section inspection beyond the `size` summary. |

### Engineering Constraints

- ISO C does not mandate physical `.text`, `.rodata`, `.data`, or `.bss` placement. Embedded toolchain, linker, object format, and startup conventions determine the observed layout.
- A `const` object is commonly placed in a read-only section, but that is not a language guarantee. An initialized non-zero global commonly needs initial-value bytes in the firmware image and runtime RAM; an uninitialized or zero-initialized global commonly needs runtime RAM plus startup zeroing rather than equivalent zero payload bytes in the image.
- Heap and stack are runtime-memory concepts, not ISO C object-file sections. Treat their observed addresses and distances as lab evidence, not an ABI, linker, or language promise.
- The required use of `malloc`/`free` is intentional laboratory scope. It conflicts with the intent of MISRA C:2012 Directive 4.12 and must not be described as MISRA-compliant.

### Selected Standards/Safety Notes

| Concern | Applicable engineering guidance |
| --- | --- |
| Dynamic allocation | MISRA C:2012 Directive 4.12 flags dynamic allocation as a project-policy concern. CERT MEM30-C, MEM31-C, and ERR33-C support checking allocation results and releasing allocated storage. |
| Pointer validity and numeric observation | CERT EXP34-C applies to dereference safety. Pointer-to-integer conversion for this narrow lab is implementation-dependent; do not infer portable pointer arithmetic from a `uintptr_t` value. |
| Initialization and cleanup | MISRA C:2012 Rule 9.1 and Rule 22.1 support initialized local state and deliberate resource release. |
| Evidence scope | `size` and `nm` are required tool evidence for one selected build. Optional `objdump` output can add actual section-header evidence. None establishes an ISO C placement rule or a complete safety-compliance claim. |

### Acceptance Criteria

- `print_memory_map()` is documented, builds under the common baseline, and produces labelled observations for the required object categories.
- The submitted program does not print a function pointer through a function-pointer-to-`void *` conversion and does not label a function-pointer object as `.text`.
- A successful allocation is checked and freed; a failed allocation is handled without invalid access.
- Numeric address observations, when included, use only the stated `uintptr_t` lab assumption and are described as implementation-specific magnitudes rather than portable pointer distances or ordering.
- `size` and `nm` evidence is included with accurate interpretation of GNU `size` summary columns. `objdump -h` may be included when actual section inspection is useful.
- Doxygen and Makefile `all`/`clean` targets are present, and the required compiler and tool checks complete with relevant warnings and errors resolved.
- The submission explicitly avoids a complete MISRA compliance claim for this dynamic-allocation laboratory exercise.

### Submission

Submit `Exercise_1/main.c` and a `Makefile` with at least `all` and `clean`. Include a header only when it serves a real interface. Append the required `size` and `nm` commands and output as a labelled comment block at the bottom of `main.c`; optional `objdump -h` evidence may be included for section inspection.

## Session 02 — Exercise 2

**Source provenance:** `session-02.md`, `Exercise_2 [build]` — *Stack Depth Monitor — Measure Stack Usage & Detect Overflow Risk*.

### Scenario

An embedded function recurses to process a bounded operation. The program must observe call depth and a stack-use proxy during one controlled run, then stop before a configured risk threshold is crossed.

### Objective

Implement a recursive stack-depth monitor that measures a build-specific stack-address magnitude, enforces a configured limit before further recursion, and demonstrates why bounded call depth matters in embedded systems.

### Required Interface

```c
int8_t recurse_with_monitor(uint32_t current_depth,
                            uint32_t max_depth,
                            const uintptr_t stack_base_addr,
                            uint32_t stack_limit_bytes);
```

For this lab, `stack_base_addr` is captured from the converted address of a local object in `main()`. The lab assumes that the implementation provides `uintptr_t`.

### Requirements

- Preserve the recursive design: each invocation declares a local marker and invokes the next level only when the configured depth and guard conditions permit it.
- At each level, observe the local marker address and report the current depth plus a stack-use magnitude derived from the numeric difference between converted object-pointer values.
- Calculate and report a magnitude without assuming that the stack grows downward or that frames have a fixed physical layout.
- Before another recursive call, compare the current observed magnitude with `stack_limit_bytes`. When the guard is reached or exceeded, emit the documented warning and return `-1` without recursing further.
- Return `0` when the requested maximum depth is reached without crossing the guard.
- Initialize state, check all control inputs, use explicit returns, and retain the common C99, BARR C, Doxygen, Makefile, compiler, `cppcheck`, and `clang-tidy` baseline.
- Measurements are observations from one build and run. They are not portable guarantees of frame size, stack direction, or total stack capacity.

### Required Test Cases / Expected Behavior

| Input or condition | Expected behavior |
| --- | --- |
| A conservative limit and bounded `max_depth` | Prints each reached depth and its observed magnitude, reaches the requested maximum depth, and returns `0`. |
| A deliberately small configured limit | When the observed magnitude reaches or exceeds the guard, prints the documented warning and returns `-1` without another recursive call. |
| Multiple reached levels | Each reached invocation reports a local-marker observation; the direction of address change is not assumed. |

### Engineering Constraints

- Recursion is intentionally required to demonstrate call depth. A guard can reduce the risk of uncontrolled growth, but it does not make recursion compliant with a project or rule set that prohibits recursion, including MISRA C:2012 Rule 17.2.
- Do not assume a downward-growing stack, fixed frame layout, a portable relationship between numeric addresses and stack bytes, or an exact future overflow point.

### Selected Standards/Safety Notes

| Concern | Applicable engineering guidance |
| --- | --- |
| Runtime resource failure | MISRA C:2012 Directive 4.1 supports identifying and controlling stack-exhaustion risk. |
| Recursion | MISRA C:2012 Rule 17.2 prohibits recursion. This exercise intentionally demonstrates the risk and is not a compliance exception by itself. |
| Initialization and control flow | MISRA C:2012 Rule 9.1 and Rule 15.5 support initialized state and explicit completion paths. |
| Pointer and integer observations | Converted object-pointer values are a narrow implementation-specific lab technique; do not treat them as portable pointer arithmetic. |
| Call depth and automatic objects | CERT MEM05-C, EXP33-C, and EXP34-C are relevant to bounded automatic storage, initialization, and safe dereference. |

These notes identify the safety discussion required by the source session. They do not claim that a guard, a clean tool run, or the lab program satisfies a complete MISRA or CERT profile.

### Acceptance Criteria

- The required recursive interface and its input/return contract are documented and compile under the common baseline.
- The program retains recursion, reports every reached depth, and uses a magnitude calculation that does not assume stack growth direction.
- A safe bounded run reaches its requested depth and returns `0`.
- A constrained-limit run warns and returns `-1` when the current observed magnitude reaches or exceeds the configured guard, without another recursive call.
- Input validation and arithmetic handling prevent uncontrolled recursion for the documented control inputs.
- The submission identifies the measurement as a one-build/run observation.
- The submission makes no claim that the guard proves stack safety, produces a portable frame-size measurement, or makes recursion MISRA-compliant.

### Submission

Submit `Exercise_2/main.c` and a `Makefile` with at least `all` and `clean`. Include a header only when it declares a real public interface. Keep the work a focused recursive monitoring laboratory; do not substitute a non-recursive implementation or expand it into a production stack-management system.
