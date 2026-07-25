# M01-L01 — High-Reliability Coding Standards

> **Status:** APPROVED.

## 1. Learning Objectives

This lesson establishes the coding baseline for the DevLinux Embedded C sequence. It prepares you to write the safe IPv4 and MAC parsing exercises in session-01, but it does not prescribe either parser's algorithm. After completing the lesson, you should be able to:

- identify a project's C language baseline and the compiler mode that enforces it;
- distinguish ISO C from MISRA C:2012 and BARR-C:2018;
- define a small C API with initialization, validation, bounds checks, and an explicit result contract;
- choose fixed-width, native, and size-related integer types for the right reasons;
- recognize several high-risk undefined behaviors and separate them from unspecified behavior;
- explain scope, linkage, and storage duration without treating them as the same concept;
- use baseline compiler diagnostics, a small Makefile, and Doxygen comments to make code easier to review.

The goal is not to memorize a standards catalogue. The goal is to make assumptions visible before an embedded or Linux component receives an unexpected input, is rebuilt with a new compiler, or is maintained by someone other than its original author.

## 2. Why High-Reliability C Matters

C gives direct control over data representation and interfaces. That is valuable in firmware, device-facing Linux programs, and protocol code, but an unchecked output pointer, malformed text, or silent narrowing conversion can turn an ordinary configuration error into an intermittent field defect.

High-reliability C is disciplined C: define the accepted domain, check a condition before relying on it, and record the result contract so callers do not need to guess. The compiler, target, ABI, and project rules may differ, but the questions are constant: what does this function accept, what does it promise, what changes on failure, and how was that result verified?

## 3. C Language Standards and Compiler Modes

A C language standard defines the portable language contract: syntax, types, expressions, statements, translation requirements, and the standard library. A compiler implements one or more language dialects and may offer extensions; it also controls target-specific options that ISO C does not define.

The historical names in this curriculum are useful shorthand:

- **C89/C90** is the ANSI C baseline and its closely corresponding ISO publication. Older embedded products may retain it because their toolchain, vendor libraries, or established code base requires it.
- **C99** added widely used facilities including `<stdint.h>` and remains a common embedded baseline.
- **C11** added `_Static_assert` and other facilities; adopting it does not require using every feature it specifies.

The language mode belongs in the build configuration, not in a developer's memory. With GCC, `-std=c99` requests the C99 ISO dialect and `-std=c11` requests C11. Selecting one makes accepted syntax and standard features reproducible when defaults or compiler versions change.

An ISO dialect option is not a complete policy engine: GCC may still accept some extensions, while `-Wpedantic` requests its pedantic diagnostics relative to `-std`. Record both the language mode and diagnostics, as well as compiler name and version.

Use a newer feature only when the project baseline permits it. For example, `_Static_assert` is a C11 feature, so it is not available to a C99-only build:

```c
#define MAX_PROTOCOL_CHANNELS 8U

_Static_assert(MAX_PROTOCOL_CHANNELS <= 16U,
               "protocol channel count exceeds the interface limit");
```

This assertion protects a compile-time configuration invariant; external data still needs runtime checks.

## 4. ISO C, MISRA C, and BARR-C

These sources answer different questions. **ISO C** defines language meaning, including type conversions and undefined behavior; it does not prescribe a project's braces, naming, or permitted subset. **MISRA C:2012** restricts patterns that are difficult to reason about in critical systems, including risky conversions, unclear control flow, and pointer use. **BARR-C:2018** emphasizes practical defect prevention through readable, consistent, maintainable Embedded C, and harmonizes its style rules with MISRA C:2012.

Neither guideline is an ISO language standard or a substitute for project requirements. A complete compliance claim needs defined scope and process; this lesson teaches habits, not a rule catalogue, deviations, or certification.

```c
if (is_enabled)
    start_device();
    record_start();
```

ISO C gives this code a meaning: only `start_device()` is controlled by the `if`. High-reliability style uses braces because indentation is easy to misread and future edits can silently change the intended control flow.

```c
uint16_t port = (uint16_t)requested_port;
```

ISO C permits the cast, but it neither proves that `requested_port` fits nor reports rejection. The corrected validation-before-cast pattern appears in the next section. Project conventions should make that contract easy to review.

| Reliability concern | Engineering practice |
| --- | --- |
| Indeterminate state | Initialize an object before its first read. |
| Untrusted external input | Define accepted syntax and range; reject anything outside them. |
| Narrowing conversion | Prove the source value fits the destination before casting. |
| Ambiguous control flow | Use braces and simple, explicit branches. |
| Unnecessary module exposure | Keep implementation-only names and state at file scope with internal linkage. |
| Caller uncertainty | Document valid input, output changes, and explicit success or failure behavior. |

## 5. Safe C Engineering Practices

Safe C starts at the boundary between an assumption and an operation that relies on it: state the condition, check it first, then give the caller a usable outcome. These practices underpin session-01.

### 5.1 Initialize before the first read

An automatic local object has no useful default value merely because it was declared. Reading it can produce undefined behavior.

```c
bool output_valid;

if (output_valid)
{
    send_reply();
}
```

`output_valid` has no defined state. Initialize it before any branch reads it:

```c
bool output_valid = false;

if (output_valid)
{
    send_reply();
}
```

The initial value must be meaningful; here `false` means no successful result exists yet.

### 5.2 Validate pointers, ranges, and output behavior

Pointer validation is part of the interface contract. A public configuration API can return failure without writing through an invalid output pointer.

```c
bool configuration_port_set(uint32_t requested_port, uint16_t *p_port)
{
    if ((p_port == NULL) || (requested_port > UINT16_MAX))
    {
        return false;
    }

    *p_port = (uint16_t)requested_port;
    return true;
}
```

Success requires a destination and a representable value. On failure, `*p_port` is untouched because dereference follows the checks. The cast is deliberately last: it changes type but does not validate `70000U` or any other source value.

### 5.3 Check bounds before indexing

Bounds errors commonly begin as a one-character mistake in a loop condition.

```c
uint8_t checksum = 0U;
size_t index;

for (index = 0U; index <= byte_count; ++index)
{
    checksum ^= p_bytes[index];
}
```

When `byte_count` is the number of elements, the last iteration reads outside `0` through `byte_count - 1`. The corrected pointer/count contract treats `NULL` with a nonzero count as invalid and uses a half-open range.

```c
bool checksum_bytes(const uint8_t *p_bytes, size_t byte_count, uint8_t *p_checksum)
{
    size_t index;
    uint8_t checksum = 0U;

    if ((p_checksum == NULL) || ((p_bytes == NULL) && (byte_count != 0U)))
    {
        return false;
    }

    for (index = 0U; index < byte_count; ++index)
    {
        checksum ^= p_bytes[index];
    }

    *p_checksum = checksum;
    return true;
}
```

This function permits `p_bytes == NULL` only for an empty input, because no element is accessed. Whether emptiness is valid is a project decision, but the contract and implementation must agree.

### 5.4 Validate external text before conversion

External text is not a number until the program validates syntax, complete consumption, conversion range, and the application's allowed domain. `atoi()` is unsuitable: it offers no end position and no usable error report separating invalid text from a valid zero result. The generic pattern is to reset `errno`, retain the end pointer, and reject every incomplete or range-failed conversion:

```c
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

bool decimal_text_to_long(const char *p_text, long *p_value)
{
    char *p_end = NULL;
    long value;

    if ((p_text == NULL) || (p_value == NULL))
    {
        return false;
    }

    errno = 0;
    value = strtol(p_text, &p_end, 10);

    if ((p_text == p_end) || (*p_end != '\0') || (errno == ERANGE))
    {
        return false;
    }

    *p_value = value;
    return true;
}
```

`p_text == p_end` means no character was converted; `*p_end != '\0'` means trailing invalid text remains; and `ERANGE` reports a conversion-range failure. `strtol()` follows its own lexical rules, including optional leading whitespace and sign characters, so a strict protocol or configuration format may require additional syntax validation. A caller that needs a smaller or application-bounded destination must validate that additional range before narrowing. The IPv4 and MAC labs apply this sequence without this lesson supplying their algorithms.

### 5.5 Keep control flow visible

Braces make a code change easier to review because the controlled region is explicit.

```c
if (input_is_valid)
{
    store_configuration();
    notify_application();
}
```

Prefer one purpose per branch and report an error at a defined point. Session-01 parsers must document whether output changes only on success and which status means malformed input.

## 6. Fixed-Width and Portable Integer Types

The native C integer types—`char`, `short`, `int`, `long`, and `long long`—have implementation-defined widths and ranges within the limits required by the standard. That makes them appropriate when the exact representation is not part of a contract, but a bare `int` does not communicate the width of a protocol field or binary interface.

Since C99, `<stdint.h>` can provide exact-width types such as `uint8_t`, `uint16_t`, `uint32_t`, and `int32_t`. An exact-width typedef exists only when the implementation has a matching type of exactly that width with no padding bits. Therefore, code that requires `uint32_t` should state that requirement and build on a target that supplies it; it must not assume that every conceivable C implementation does.

Use this decision model:

| Need | Appropriate first choice | Reason |
| --- | --- | --- |
| A field specified as an unsigned 16-bit quantity | `uint16_t` | Width is part of the external contract. |
| A signed protocol measurement specified as 32 bits | `int32_t` | Signedness and width are both part of the contract. |
| Capacity, object size, or array index | `size_t` | It represents the size of any object on the implementation. |
| Ordinary local arithmetic with no representation requirement | `int` or another native type | The operation, range, and target may matter more than a wire width. |

For session-01, fixed-width output types communicate the intended representation of an IPv4 value and individual MAC octets. They do not validate text, establish a byte order, make every target use eight-bit bytes, or determine the layout of a `struct`. Endianness, serialization, padding, alignment, and hardware register access are separate subjects.

`size_t` deserves particular attention. It is the type produced by `sizeof` and is the natural type for an array length or buffer capacity. Using it for a loop index makes the index and the bound comparable in the same size domain. It does not remove the need to check that the pointer is valid for that many elements.

## 7. Undefined and Unspecified Behavior

Undefined behavior (UB) means that ISO C imposes no requirements on the program after a particular construct is evaluated. The compiler may appear to generate a sensible result in one build, but an optimization level, CPU architecture, input value, or unrelated source edit can expose the defect differently. UB is not a recoverable error code and should never be used as a test of what a target "happens to do."

Three practical examples are enough to establish the habit:

```c
uint8_t octets[4] = {0U, 0U, 0U, 0U};
uint8_t invalid = octets[4]; /* Out-of-bounds read: undefined behavior. */
```

```c
uint16_t *p_port = NULL;
*p_port = 80U; /* Null-pointer dereference: undefined behavior. */
```

```c
int retry_count = INT_MAX;
retry_count = retry_count + 1; /* Signed overflow: undefined behavior. */
```

The practical defence is to validate array bounds and pointers before use, and to check a value's supported range before arithmetic or conversion relies on it. A warning may help reveal a suspicious expression, but it cannot establish that every runtime input follows an API contract.

Unspecified behavior is different. ISO C permits more than one outcome but does not require the implementation to document which outcome will occur in a particular execution. Function-argument evaluation order is a common example:

```c
record_pair(read_first_sensor(), read_second_sensor());
```

Either call may be evaluated first. The expression is acceptable only when order does not matter. If one operation must precede the other, write two statements and make the order obvious.

Do not confuse unspecified argument order with unsequenced conflicting side effects on one scalar object. For example, code that modifies a scalar and also uses or modifies it again without the required sequencing can have undefined behavior, not merely an unspecified order. Avoid compact expressions such as `counter = counter++ + 1;`; use a simple statement for each state change instead. This is a practical review rule, not a request to learn the full sequencing formalism.

## 8. Scope, Linkage, and Storage Duration

These three properties are related but answer different questions. Engineers often use *visibility* informally, but C's precise concepts are scope—where a name can be used in source—and linkage—whether declarations can name the same entity across scopes or translation units. Separating them is essential when code moves from one source file to a multi-file embedded component.

| Property | Question answered |
| --- | --- |
| **Scope** | Where in the source text can this identifier be used? |
| **Linkage** | Can declarations in different scopes or translation units name the same entity? |
| **Storage duration** | For how long does an object's storage exist during execution? |

An ordinary local variable has **block scope**: its name is usable from its declaration to the end of the containing block. It also has **automatic storage duration** by default: a new instance exists for each entry into that block or function call.

```c
bool port_is_allowed(uint16_t port)
{
    bool allowed = (port <= 1024U); /* Block scope, automatic duration. */

    return allowed;
}
```

The name `allowed` is not visible to callers or to another function. Its automatic storage ends when this call leaves the block. A later call gets a distinct automatic object.

At file scope, a declaration without `static` normally has external linkage. It can be declared from another translation unit and is normally defined once in the whole program. A translation unit is broadly one source file after its included headers have been processed. Headers declare the interface; source files provide the definitions and private implementation details.

The following small component demonstrates the model.

```c
/* service_port.h */
#ifndef SERVICE_PORT_H
#define SERVICE_PORT_H

#include <stdbool.h>
#include <stdint.h>

extern uint32_t g_service_start_count; /* Declaration, not storage. */

bool service_port_set(uint16_t requested_port);
uint16_t service_port_get(void);

#endif
```

```c
/* service_port.c */
#include "service_port.h"

uint32_t g_service_start_count; /* The one external definition. */
static uint16_t s_service_port; /* Private name and state in this file. */

bool service_port_set(uint16_t requested_port)
{
    s_service_port = requested_port;
    ++g_service_start_count;
    return true;
}

uint16_t service_port_get(void)
{
    return s_service_port;
}
```

```c
/* main.c */
#include "service_port.h"

int main(void)
{
    if (!service_port_set(443U))
    {
        return 1;
    }

    return (service_port_get() == 443U) ? 0 : 1;
}
```

`main.c` can call the public functions and refer to the declared external object because the header gives it compatible declarations. It cannot refer to `s_service_port`: the file-scope `static` declaration gives that name internal linkage, so it is private to `service_port.c`. In production code, a public accessor is usually preferable to exposing mutable global state; the global is shown here to make the declaration/definition distinction visible.

## 9. `static`, `extern`, and `register` in Practice

The word `static` has different effects at block scope and file scope. A **static local** keeps block scope—only the function can use its name—but has static storage duration, so one object persists for the program's execution and retains its value between calls.

```c
uint32_t next_trace_number(void)
{
    static uint32_t trace_number = 0U;

    ++trace_number;
    return trace_number;
}
```

This is not the same as the file-scope `static uint16_t s_service_port;` in the preceding example. Both objects persist, but the local static name is visible only inside `next_trace_number()`, whereas the file-scope static name can be used throughout `service_port.c` and has internal linkage there.

`extern` is commonly used for an external declaration, such as `extern uint32_t g_service_start_count;` in the header. It tells the compiler that the object is defined elsewhere. The line `uint32_t g_service_start_count;` in exactly one source file provides storage and is the definition. Repeating the header declaration in many translation units is normal; providing competing definitions is not.

`register` is historical knowledge. It is an optimization hint, not a guarantee or a request that an object be physically placed in a CPU register. Modern compilers make allocation decisions from a much broader view of the program. A `register` object has automatic storage duration and no linkage, and C prohibits taking its address with `&`. New code should normally use a clear ordinary local and let the compiler optimize it.

| Declaration form | Scope | Linkage | Storage duration | Practical meaning |
| --- | --- | --- | --- | --- |
| `uint16_t port;` inside a function | Block | None | Automatic | Fresh local object for a call. |
| `static uint16_t port;` inside a function | Block | None | Static | One persistent object, private to that function. |
| `static uint16_t port;` at file scope | File | Internal | Static | One persistent object, private to one translation unit. |
| `extern uint16_t port;` | Declaration scope | Usually external | Static | Declaration of an object defined elsewhere. |
| `uint16_t port;` at file scope | File | External by default | Static | External definition, normally provided once. |
| `register uint16_t port;` inside a function | Block | None | Automatic | Historical optimization hint; address cannot be taken. |

## 10. Compiler Diagnostics

Compiler diagnostics are an early review tool. They point to constructs that are suspicious, nonportable, or likely to contain a mistake. A practical GCC or Clang baseline for the session-01 level is:

```text
-std=c99 -Wall -Wextra -Wpedantic -Werror
```

- `-std=c99` selects the project language baseline.
- `-Wall` enables a useful, compiler-defined group of warnings; it is not every possible warning.
- `-Wextra` enables further useful warnings beyond that baseline.
- `-Wpedantic` requests the compiler's pedantic diagnostics relative to the selected language mode.
- `-Werror` makes enabled warnings fail the build under project policy, so a new warning must be consciously investigated.

Consider an accidental assignment in a condition:

```c
bool network_enabled = false;

if (network_enabled = true)
{
    start_network();
}
```

Many compiler configurations diagnose this because an assignment used as a truth value is often an error. The correct response is to inspect intent: perhaps the condition should be `if (network_enabled)`, perhaps it needs a comparison, or perhaps an assignment is genuinely intended and must be made unmistakable. Suppressing a warning without answering that question only hides evidence.

Unused variables and misleading indentation are two further examples of defects or maintenance hazards that common warning sets can reveal. A declaration that is never used may indicate an unfinished error path; an unbraced `if` with deceptive indentation may indicate that the wrong statement is guarded. Exact diagnostics depend on compiler, version, language mode, optimization, and enabled options.

Warning-free compilation is valuable evidence, but it is not proof that a program is bug-free, MISRA-compliant, safe for every target, or correct for every external input. Compilers cannot infer all protocol rules, deployment assumptions, or runtime data. Static-analysis frameworks, sanitizers, and deeper dynamic analysis belong to later curriculum scope.

## 11. Makefile Fundamentals

A compiler normally performs two visible stages. **Compilation** translates each `.c` source file into an object file such as `.o`. **Linking** combines object files and required libraries into an executable. A Makefile records those relationships so that `make` rebuilds only what is out of date.

In a Make rule, the **target** is the file or named action to make, **prerequisites** are the inputs on which it depends, and the indented **recipe** is the command that produces it. If a prerequisite is newer than its target, `make` runs the recipe again. This is dependency-based rebuilding, not simply a shorter way to type compiler commands.

```make
CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -Wpedantic -Werror
TARGET := network_config

all: $(TARGET)

$(TARGET): main.o network_config.o
	$(CC) $^ -o $@

main.o: main.c network_config.h
	$(CC) $(CFLAGS) -c $< -o $@

network_config.o: network_config.c network_config.h
	$(CC) $(CFLAGS) -c $< -o $@

docs:
	doxygen Doxyfile

clean:
	rm -f $(TARGET) main.o network_config.o

.PHONY: all clean docs
```

The `all` target is the normal entry point: running `make` builds `network_config`. The executable depends on both object files; each object explicitly depends on its source file and the header it includes. If `network_config.h` changes, both objects are rebuilt. The `clean` target removes generated files, and `docs` runs the documented Doxygen configuration.

Three GNU make automatic variables keep the recipes accurate:

- `$@` is the target currently being built, such as `main.o` or `network_config`.
- `$^` is the complete prerequisite list, useful when linking both object files.
- `$<` is the first prerequisite, useful for compiling one source file into one object file.

Recipe lines must begin with a tab in this basic Makefile syntax. Do not introduce CMake, generated dependency files, or advanced Make features here; the purpose is to understand the reliable compile–link–rebuild cycle used by the early labs.

## 12. Doxygen and API Contracts

An API contract tells a caller more than a function name and parameter types can express. It records valid inputs, output behavior, failure behavior, units where they matter, and ownership only when ownership is relevant. In parser-facing code, the most useful questions are usually: may this pointer be `NULL`, is an empty input allowed, is the output changed on failure, and how does the caller detect failure?

Doxygen turns structured source comments into navigable API documentation according to a `Doxyfile` configuration. A project can generate the documentation through the `docs` Make target shown above. The comment is part of interface design, not decoration.

```c
/**
 * @brief Convert one validated decimal configuration value to a service port.
 *
 * @param[in] p_text Null-terminated input text; it must name a supported value.
 * @param[out] p_port Output location written only when the conversion succeeds.
 * @return true on success; false for invalid input, range failure, or null output.
 */
bool configuration_port_from_text(const char *p_text, uint16_t *p_port);
```

`@brief` gives the reader a quick purpose statement. `@param[in]` identifies data consumed by the function, while `@param[out]` describes an output written by the function. `@return` records the result contract. The prose must agree with the implementation: if an output is sometimes modified on failure, the comment must say so; if a value has a unit such as milliseconds, include it; and if a function transfers ownership, state that explicitly.

## 13. Common Failure Patterns and Debugging

Early debugging should begin by checking the contract that the code was meant to implement, not by guessing at the final symptom. The following patterns recur in configuration and protocol code.

| Symptom or review finding | First engineering question |
| --- | --- |
| A crash occurs while storing a result | Was the output pointer checked before dereference, and is its lifetime valid? |
| A value changes unexpectedly after conversion | What are the source and destination ranges, and was validation performed before the cast? |
| Malformed input reaches an array access | What is the buffer capacity, and is every index proven to be inside it? |
| A build works locally but not in CI or on the target | Which compiler, version, `-std` mode, flags, headers, and target assumptions differ? |
| A module's state is changed from an unexpected file | Should that name have internal linkage, or should access go through a narrow API? |
| The caller cannot tell whether an operation succeeded | Does the API document and return an explicit success or failure result? |

Apply diagnostics before debugging runtime behavior: rebuild with the recorded warning policy, read every diagnostic, and correct the underlying cause rather than merely adding a cast or suppression. Then review the data path from external text to validated value to destination object. This sequence exposes many errors before they become target-specific failures.

## 14. Key Takeaways

- Select and record a C language baseline. `-std=c99` and `-std=c11` are deliberate compatibility choices, not cosmetic flags.
- ISO C defines language semantics; MISRA C:2012 restricts risky usage for critical systems; BARR-C:2018 promotes consistent, maintainable Embedded C practice.
- Initialize state, validate pointers and external input, check bounds and ranges before use, and convert only after the destination can represent the value.
- A cast documents a conversion but does not make an invalid source value valid. `atoi()` cannot support a robust text-to-integer error contract.
- Fixed-width types express representation requirements; `size_t` expresses object-size and indexing requirements. Neither solves byte order or structure layout alone.
- Out-of-bounds access, null dereference, and signed overflow are undefined behavior. Do not rely on a build that appears to work.
- Scope, linkage, and storage duration are distinct. In particular, a static local is not the same thing as a file-scope static, and an `extern` declaration is not a definition.
- Treat compiler warnings as actionable evidence. A warning-free build is necessary engineering evidence, not a correctness or compliance proof.
- Use `make` to encode the compile–link dependencies and Doxygen to make interface contracts visible to callers.

M01-L02 next examines where program objects reside and how memory failures are analysed. It owns memory sections, stack frames, allocation, and related failure analysis; this lesson deliberately stops at the coding baseline that helps prevent avoidable errors before those topics begin.

## 15. Further Reading

- [GCC C Dialect Options](https://gcc.gnu.org/onlinedocs/gcc/C-Dialect-Options.html)
- [GCC Warning Options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- [MISRA C](https://misra.org.uk/)
- [Barr Group Embedded C Coding Standard (BARR-C:2018)](https://barrgroup.com/embedded-systems/books/embedded-c-coding-standard1)
- [SEI CERT C Coding Standard](https://wiki.sei.cmu.edu/confluence/display/c)
- [GNU make Manual](https://www.gnu.org/software/make/manual/)
- [Doxygen: Documenting the Code](https://www.doxygen.nl/manual/docblocks.html)
- [cppreference: C Storage-Class Specifiers](https://en.cppreference.com/w/c/language/storage_class_specifiers.html)
