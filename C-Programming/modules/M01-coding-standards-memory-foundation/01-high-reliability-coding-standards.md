# M01-L01 — High-Reliability Coding Standards

> **Status:** `DRAFT — HUMAN_REVIEW_PENDING`
>
> **Gate:** `LESSON_1_AUTHORING`
>
> **Language baseline:** ISO C99

## 1. Purpose, Scope, and Learning Model

High-reliability C makes assumptions visible before code relies on them. A useful component does not merely appear to work for one input on one host; it states what it accepts, what it changes, what failure means, and what evidence supports the claim. This discipline matters in firmware, device-facing Linux services, configuration utilities, and every boundary where external data becomes an internal value.

This lesson owns the M01 language and engineering baseline: C99 selection, coding-standard roles, API contracts, conversions, behavior classification, scope and linkage, diagnostics, basic Make, and API documentation. It prepares learners for the Session 01 IPv4 and MAC exercises, but it does not repeat their interfaces, algorithms, acceptance cases, or solutions. Those remain owned by `exercises.md` and `solutions/`; interview assessment remains owned by `interview.md`.

Use the following learning model throughout the lesson:

```text
assumption → written contract → language rule → applicable guidance
→ compiler or analyzer evidence → human review → documented decision
```

The compiler and tools are valuable, but none can infer every protocol rule, target constraint, ownership decision, or project policy. A developer must still define the accepted domain and review whether the implementation preserves it.

**Must remember:** reliable C is not a collection of warning flags. It is a repeatable way to turn implicit assumptions into contracts that another engineer can inspect.

## 2. C Standards and the C99 Baseline

The C standard defines the portable language contract: syntax, types, expressions, statements, translation requirements, and the standard library. A compiler implements one or more language dialects for particular targets and can add extensions. The standard therefore answers “what does this C construct mean?”, while the compiler configuration answers “which dialect and target are we building today?” [ISO C99; GCC, C Dialect Options]

The historical names used in the DevLinux roadmap are useful orientation rather than a standards-history survey:

- **C89/C90** is the older baseline retained by some established products and toolchains.
- **C99** added facilities that are central to this curriculum, including `//` comments, `_Bool` through `<stdbool.h>`, and the integer typedefs in `<stdint.h>`.
- **C11** is a later revision. Its facilities are outside this lesson's baseline unless a later module explicitly introduces them.

M01 uses ISO C99 as its explicit baseline because the roadmap and Session 01 build requirements select it. A compiler default is not a project contract: defaults can change with compiler releases, target packages, or build environments. Record the language mode in the build command and in the Makefile. [BARR-C:2018 §1.1; GCC, C Dialect Options]

```text
gcc -std=c99 -Wall -Wextra -Wpedantic -Werror -c device_config.c
```

For GCC, `-std=c99` selects its ISO C99 mode. It does not prove that every accepted construct is portable C99: GCC documents extensions that can still be accepted in a base-standard mode when they do not conflict with that standard. `-Wpedantic` asks GCC to diagnose many non-ISO constructs relative to the selected `-std` version, but it is still a compiler diagnostic policy, not a complete conformance proof. [GCC, C Dialect Options; GCC, Warning Options]

Hosted and freestanding implementations are another important boundary. A hosted implementation supplies the normal hosted environment and startup model expected by ordinary applications. A freestanding implementation, common in low-level firmware contexts, can provide a smaller library environment and a different startup arrangement. C99 source code may be portable across both only when it uses facilities the selected implementation actually provides. Ask the toolchain documentation which headers, library functions, startup code, and runtime assumptions are available; do not infer them from the `-std=c99` flag alone. [ISO C99 §5.1.2; GCC, C Dialect Options]

Compiler extensions need an explicit decision: identify the extension, isolate it when practical, name the compiler and version that support it, provide a fallback or a portability boundary, and review the target scope. For example, `_Static_assert` is a C11 feature, so it must not quietly enter a C99-only M01 build. Later standards and extensions are not “better C99”; they are different declared baselines.

**Must remember:** select the language revision deliberately, record the compiler command, and label every extension or later-standard feature instead of treating a compiler default as a portable guarantee.

## 3. ISO C, MISRA C, and BARR-C

ISO C, MISRA C, and BARR-C serve different purposes. A project needs all relevant layers to remain distinct so that a style rule is not accidentally taught as a language rule, or a clean build is not mistaken for a safety case.

| Source or policy | Primary question answered | What it contributes in M01 | What it does not prove |
| --- | --- | --- | --- |
| ISO C99 | What does the language require, permit, or leave undefined? | Semantics for types, conversions, expressions, scope, linkage, library contracts, and diagnostics. | A project's coding style, target behavior, or complete safety process. |
| MISRA C:2012 | Which restricted-use guidance and compliance model apply to a selected critical-system project? | Risk-oriented guidelines, categories, tool and review boundaries, compliance and deviation concepts. | That selected examples, warnings, or one tool run establish complete compliance. |
| BARR-C:2018 | Which Embedded C practices improve readability and defect prevention? | C99 baseline, brace and parenthesis discipline, comments, modules, type guidance, and function practices. | ISO C semantics or universal target performance. |
| Project policy | What must this product, team, and target do? | Chosen standard subset, target restrictions, approved tools, review evidence, and documented exceptions. | A replacement for ISO C or the cited standards. |

For example, ISO C gives the following fragment a defined control-flow meaning: only the first statement is controlled by the `if`. The fragment is intentionally incomplete; it illustrates structure rather than a complete program.

```c
if (is_enabled)
    start_device();
    record_start();
```

Consistent braces reduce the chance that indentation or a later edit hides the true controlled region. The corrected pattern makes the intended controlled block explicit:

```c
if (is_enabled)
{
    start_device();
    record_start();
}
```

This is a maintainability and defect-prevention practice, not a change to ISO C semantics. [BARR-C:2018 §1.3]

Likewise, a cast may be legal C while still hiding an unproven range assumption. The engineering question is not “does the cast compile?” but “what source range has been established, and can the destination represent it?” The conversion contract appears in Sections 6 and 7.

**Must remember:** ISO C supplies meaning, MISRA C supplies constrained-use and compliance guidance, BARR-C supplies practical maintainability discipline, and the project chooses how they are applied.

## 4. MISRA Guideline Model

MISRA C:2012 distinguishes a **rule** from a **directive**. A rule has a complete enough description that source-code compliance can, in principle, be checked without additional project information. A directive needs information beyond the source, such as design requirements, build configuration, or system context. Tools can assist with both, but tool support and interpretation vary. [MISRA C:2012 §6.1]

Every MISRA guideline has one category: **Mandatory**, **Required**, or **Advisory**. The category governs the compliance and deviation treatment; it is not a ranking of how serious a defect would be in a particular product. Mandatory guidance cannot be deviated from in a MISRA conformance claim. Required guidance needs compliance or a formal deviation. Advisory guidance should be followed as far as reasonably practical, with the project's documented treatment of any non-compliance. A project may impose a stricter local policy. [MISRA C:2012 §6.2]

Some guidelines are readily checked from source under a known language and target configuration; others need translation-unit, whole-program, or system evidence. That distinction explains why a compiler diagnostic is not automatically a confirmed MISRA violation, and why the absence of a message does not prove compliance. A diagnostic can indicate a real violation, a possible violation, a false diagnosis, or a concern outside the MISRA guideline set. It must be investigated in context. [MISRA C:2012 §§5.3, 6.1]

A proportionate compliance model includes a matrix showing which guidelines apply, how each is checked, which compiler and analysis configurations are used, and where human review is required. A deviation is an authorized, recorded exception with scope, rationale, risk assessment, compensating controls, approval, and a revalidation trigger. This is an introductory model only; it is not a complete compliance-management procedure. [MISRA C:2012 §§5.3–5.5]

| Evidence item | Useful question | Insufficient conclusion to avoid |
| --- | --- | --- |
| Compiler diagnostic | Does this configured compiler identify a language or quality concern in this build? | “No compiler warning proves the code is correct.” |
| Static-analysis message | Does the configured analyzer identify a possible issue under its model? | “One message proves a confirmed guideline violation.” |
| Manual review | Does the code satisfy the stated contract and the applicable guidance in its real context? | “Review can be skipped because a tool was clean.” |
| Compliance matrix and deviations | Is there project-scoped evidence for all applicable guidelines and authorized exceptions? | “Using selected rule numbers creates a complete compliance claim.” |

The supplied DevLinux source mentions **Directive 4.14**, but that identifier is **UNVERIFIED** in the supplied MISRA C:2012 reference. It is therefore not used in this lesson as verified MISRA guidance. The issue is retained as a source-reference finding for the later exercise gate; the general need to validate external input is taught from the API contract and applicable language or security guidance instead. [M01-CR-001]

This lesson also introduces deviation vocabulary only. Dynamic-allocation-specific analysis belongs to M01-L02, where the relevant lab conflict and runtime failure model are owned. [M01-CR-003]

**Must remember:** a guideline name, a warning, and a clean analysis run are inputs to a compliance process—not substitutes for its scope, configuration, review, and evidence.

## 5. BARR-C Practices Relevant to M01

BARR-C:2018 supplies an Embedded C coding baseline focused on readability, reviewability, and defect prevention. M01 uses the practices that support Session 01 without presenting BARR-C style as ISO C semantics or reproducing the standard. The following map points to the relevant verified sections. [BARR-C:2018 §§1.1, 1.3–1.6, 2.2, 4, 5.2–5.3, 6.2, 7, 8]

| Practice | Engineering purpose | M01 boundary |
| --- | --- | --- |
| C99 baseline and controlled extensions | Keep accepted language features explicit and localize unavoidable target-specific mechanisms. | M01 explains language mode; driver-specific extensions belong to later hardware work. |
| Braces and parentheses | Make control flow and intended expression grouping straightforward to review. | They improve clarity; they do not alter ISO C operator semantics. |
| Explicit casts with range reasoning | Make conversions visible and force a reviewer to ask why the conversion is safe. | A cast never proves the source range. |
| Clear comments and Doxygen-ready public interfaces | Record assumptions, contracts, and maintenance-relevant rationale near the code. | Documentation must agree with the implementation. |
| Modules, headers, and private functions | Limit unnecessary exposure and make public interfaces explicit. | Linkage semantics remain defined by ISO C. |
| Initialization before first use | Prevent indeterminate reads and make the initial program state explicit to reviewers. | Initialization must reflect a valid state; it does not replace later state or input validation. |
| Fixed-width and signed/unsigned discipline | Make representation requirements visible and reduce conversion surprises. | Not every local integer needs a fixed-width typedef. |
| Simple functions, branches, and loops | Make behavior and review paths easier to follow. | A style preference is not a proof of correctness. |

When a value's width is part of an external representation, protocol, register-independent data format, or documented API, an exact-width type is a strong choice if the implementation provides it. When a local loop counter or intermediate calculation has no externally specified width, choose the type from its range and operations rather than applying fixed-width types by reflex. `uint32_t` communicates a 32-bit unsigned representation; it does not validate the value, guarantee a particular byte order, or guarantee speed. [BARR-C:2018 §§5.2–5.3; ISO C99 §7.18]

Naming, comments, braces, and limited module visibility are valuable because humans maintain C over years and across target changes. The most valuable comment explains an assumption, limitation, or reason a future editor cannot infer from the code. Repeating the syntax of an obvious statement is less useful. [BARR-C:2018 §2.2]

**Must remember:** BARR-C practices make assumptions and intent easier to review. They support reliable engineering, but they neither replace ISO C nor establish compliance by themselves. [M01-CR-017]

## 6. Reliable API Contracts and Input Validation

An API contract describes the observable boundary between caller and implementation. Before a function reads a pointer, indexes a buffer, converts text, narrows a value, or changes output state, the contract must answer these questions:

- What preconditions must the caller satisfy?
- Which pointers may be null, and which must refer to valid objects?
- What syntax and range are accepted?
- Must all external input be consumed, or may trailing text remain?
- What result represents success and what result represents failure?
- Does an output object remain unchanged on failure, become a documented sentinel, or have another defined state?
- What units and ownership rules apply, if either is relevant?

The order is important: validate before use. In this complete C99 example, a public setter checks the output pointer and representable range before it writes or narrows the value.

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

The cast changes the type after the range has been established; it does not establish the range itself. This particular contract preserves the valid output object on failure because the dereference occurs only after validation. Other APIs may choose another failure policy, but they must document and implement it consistently.

External text needs more than a numeric conversion call. `atoi()` cannot report where conversion stopped or distinguish malformed text from a valid zero result, so it cannot support a reviewable syntax-and-range contract. `strtol()` provides an end pointer and `errno` range reporting, but its accepted lexical form can include leading whitespace and a sign. A protocol that forbids either must pre-validate its own syntax. [ISO C99 §7.20.1.4]

The following complete, generic C99 function converts a fully consumed decimal configuration field to a bounded `uint16_t`. It is not an IPv4 or MAC implementation.

```c
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

bool configuration_limit_from_text(const char *p_text, uint16_t *p_limit)
{
    char *p_end = NULL;
    long parsed_value;

    if ((p_text == NULL) || (p_limit == NULL))
    {
        return false;
    }

    errno = 0;
    parsed_value = strtol(p_text, &p_end, 10);

    if ((p_end == p_text) || (*p_end != '\0') || (errno == ERANGE) ||
        (parsed_value < 0L) || (parsed_value > 1000L) ||
        (parsed_value > (long)UINT16_MAX))
    {
        return false;
    }

    *p_limit = (uint16_t)parsed_value;
    return true;
}
```

`p_end == p_text` means that no character was converted. A non-null character at `p_end` means trailing text remains. `ERANGE` reports that the conversion result was outside the function's `long` result range. The application check of `0` through `1000` is separate: library conversion success does not automatically mean that a configuration value is acceptable for the product. [ISO C99 §7.20.1.4]

MISRA C:2012 Directive 4.11 concerns the validity of values passed to **library functions**. It is not a universal null-pointer-check directive for every project API. General pointer validation follows the public API contract and the language rules governing each dereference; security guidance can supplement that contract when external input crosses a trust boundary. [MISRA C:2012 Dir 4.11; M01-CR-002]

**Must remember:** define input, output, success, and failure first; validate all of those conditions before the first operation that relies on them.

## 7. Integer Types and Conversion Discipline

The native integer types (`char`, `short`, `int`, `long`, and `long long`) have implementation-dependent widths and ranges within the limits required by ISO C. They remain appropriate when the representation is not part of a contract. When width and signedness are part of an external interface, `<stdint.h>` offers names such as `uint8_t`, `uint16_t`, `uint32_t`, and `int32_t` when the implementation has exact matching types. Exact-width typedefs are optional: an implementation that cannot provide the required exact representation need not define the corresponding name. [ISO C99 §§6.2.5, 7.18]

| Requirement | Appropriate first choice | Reason |
| --- | --- | --- |
| A documented unsigned 16-bit field | `uint16_t` | The width is part of the contract. |
| A signed 32-bit measurement | `int32_t` | Signedness and width are part of the contract. |
| Object size, capacity, or array index | `size_t` | It represents the size of any object on the implementation. |
| Local arithmetic with no specified representation | A native type chosen from the required range | The mathematical range and target behavior matter more than a wire width. |

Integer promotions and usual conversions can evaluate an expression in a wider type than its storage object. For example, when `uint8_t` exists, its complete range is `0` through `255`; C99 requires `int` to represent that range, so a `uint8_t` operand is promoted to `int` in the usual integer promotions. Review the expression's promoted type before assuming that arithmetic or bitwise results remain eight bits. [ISO C99 §6.3.1.1]

Narrowing must be preceded by a source-domain and destination-range proof. This complete example uses an unrelated sensor configuration value and preserves the output on failure.

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool sensor_gain_from_count(int32_t raw_count, uint16_t *p_gain)
{
    if ((p_gain == NULL) || (raw_count < 0) || (raw_count > 1023))
    {
        return false;
    }

    *p_gain = (uint16_t)raw_count;
    return true;
}
```

The source range is established before the cast. A cast can make the conversion explicit for a reviewer, but it cannot make an out-of-range input valid. Do not use unsigned arithmetic merely to hide a negative value or signed/unsigned mismatch; choose types from the permitted domain and inspect conversions deliberately. [BARR-C:2018 §§1.6, 5.2–5.3]

Representation width is also not byte order. A `uint32_t` says nothing about how a multi-byte protocol value is serialized. Detailed serialization and numeric-processing rules belong to M08.

The Session 01 source describes storage and comparison advantages of a numeric representation. The useful lesson is that a validated numeric representation can simplify some interfaces and comparisons. It is not valid to claim that every comparison becomes one instruction, that a string representation will cause a failure at scale, or that a fixed-width type is universally faster. Compiler version, target instruction set, optimization, input distribution, and workload must be measured before making a performance claim. [M01-CR-005]

**Must remember:** choose the type from the contract, prove range before narrowing, and treat both endianness and performance as separate, context-dependent questions.

## 8. C Behavior Taxonomy

Reliable review begins by classifying the language behavior before choosing a correction. ISO C99 distinguishes several categories that are often incorrectly merged together. [ISO C99 §3.4; Annex J]

| Category | Meaning for the engineer | Example review response |
| --- | --- | --- |
| Defined behavior | ISO C specifies the required behavior for the construct and its preconditions hold. | Verify that the documented contract makes the preconditions true. |
| Implementation-defined behavior | The implementation chooses a behavior and must document the choice. | Read the selected compiler or target documentation and record the dependency. |
| Unspecified behavior | ISO C permits more than one outcome and need not document which one occurs for an evaluation. | Do not let correctness depend on a particular allowed order or result. |
| Undefined behavior | ISO C imposes no requirements after the construct is evaluated. | Remove the construct or establish a contract that prevents it. |
| Constraint violation | Translation violates a constraint for which a diagnostic is required. | Treat the diagnostic as mandatory evidence; successful continuation does not make the source portable. |
| Compiler extension | The compiler accepts behavior outside the selected ISO C baseline. | Label, isolate, and review the compiler and target scope. |

The following are deliberately negative fragments. They are not examples to execute or rely on.

```c
uint8_t bytes[4] = {0U, 0U, 0U, 0U};
uint8_t invalid = bytes[4]; /* Out-of-bounds read: undefined behavior. */
```

```c
int retry_count = INT_MAX;
retry_count = retry_count + 1; /* Signed overflow: undefined behavior. */
```

The compiler may diagnose some statically visible cases, but ISO C does not require a diagnostic for every possible undefined behavior. The real prevention mechanism is a contract that proves array bounds, pointer validity, arithmetic range, and object lifetime before the operation occurs. [ISO C99 §5.1.1.3]

Unspecified function-argument evaluation order is not automatically undefined behavior. In this illustrative fragment, either function can be evaluated first; it is acceptable only if the result does not depend on that order.

```c
int read_left_sensor(void);
int read_right_sensor(void);
void record_pair(int left_value, int right_value);

void capture_sensor_pair(void)
{
    record_pair(read_left_sensor(), read_right_sensor());
}
```

C99 uses **sequence-point** terminology for the separate defect in which side effects on the same scalar object are not properly separated. A compact expression that both changes a scalar and uses or changes it again without the required C99 sequencing can have undefined behavior; it is not merely an unspecified argument order. Prefer one clear state change per statement rather than expressions such as `counter = counter++ + 1;`. [ISO C99 §6.5]

Constraint violations require at least one diagnostic for the translation unit. A compiler may continue after a diagnostic, but that continuation is not a portability or correctness guarantee. The selected compiler can also accept extensions, which must be distinguished from ISO C99 behavior. [ISO C99 §5.1.1.3; GCC, Warning Options]

**Must remember:** unspecified behavior means “do not rely on a choice”; undefined behavior means “remove the invalid construct or prevent it by contract.” Use C99 sequence-point language when reviewing sequencing under this baseline.

## 9. Scope, Linkage, Storage Duration, and Lifetime

Engineers often say “visibility” in discussion, but C uses more precise concepts. **Scope** answers where a name can be used in source text. **Linkage** answers whether declarations in different scopes or translation units can name the same entity. **Storage duration** answers how long an object's storage is reserved. **Lifetime** answers when the object exists and may be accessed. These are related, but they are not interchangeable. [ISO C99 §§6.2.1–6.2.4]

| Concept | Question | Typical M01 example |
| --- | --- | --- |
| Block scope | Where can a local name be written? | A variable declared inside one function body. |
| File scope | Where can a file-level name be written? | A declaration above the functions in one source file. |
| Internal linkage | Can another translation unit name this file-level entity? | A file-scope declaration using `static`. |
| External linkage | Can compatible declarations in multiple translation units name the entity? | A public function declared in a header and defined once. |
| Automatic storage duration | How long does a typical local object's storage exist? | During each entry into its block. |
| Static storage duration | How long does the object's storage exist? | For the whole program execution. |

The following is an illustrative three-file component. The fragments are complete only when built together as the named files; they demonstrate interface and linkage rather than a production module design.

```c
/* service_port.h */
#ifndef SERVICE_PORT_H
#define SERVICE_PORT_H

#include <stdint.h>

extern uint32_t g_service_start_count;
void service_port_set(uint16_t requested_port);
uint16_t service_port_get(void);

#endif
```

```c
/* service_port.c */
#include "service_port.h"

uint32_t g_service_start_count;
static uint16_t s_service_port;

void service_port_set(uint16_t requested_port)
{
    s_service_port = requested_port;
    ++g_service_start_count;
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
    service_port_set(443U);

    return (service_port_get() == 443U) ? 0 : 1;
}
```

The header contains an `extern` declaration for `g_service_start_count`; the source file provides its one external definition. `s_service_port` has file scope, static storage duration, and internal linkage, so other translation units cannot name it. The setter returns `void` because this example defines no failure condition; inventing a Boolean result that is always `true` would create a meaningless API contract. In production, a narrow accessor is usually preferable to exposing mutable global state; the global exists here only to make declaration and definition visible.

`static` has another effect at block scope: a static local retains block scope but has static storage duration, so one object persists between calls. At file scope, it gives an object or function internal linkage. An `extern` declaration with an initializer is a definition, so the surrounding declaration—not merely the word `extern`—must be reviewed. [ISO C99 §§6.2.2, 6.2.4, 6.7.1]

For the ordinary non-VLA declared objects used in this lesson, lifetime normally follows the object's storage duration. The distinction becomes more visible with dynamically allocated objects because their lifetime ends when the allocated storage is released; allocation lifetime and ownership are owned by M01-L02.

`register` is historical optimization guidance, not a guarantee or request that an object be physically placed in a CPU register. A `register` object has automatic storage duration and no linkage, and C99 restricts applying `&` to an object declared with `register`. Modern compilers make allocation decisions from whole-program and target information; new code normally uses a clear ordinary local instead. [ISO C99 §§6.5.3.2, 6.7.1]

Storage duration describes the C object model. It does not select `.data`, `.bss`, `.rodata`, or any other executable section. Object-file sections, linker placement, and target memory are owned by M01-L02.

**Must remember:** use scope for name reachability, linkage for entity identity across translation units, storage duration for storage existence, and lifetime for valid object access.

## 10. Compiler Dialects and Diagnostics

Diagnostics are early review evidence. They help find suspicious, nonportable, or likely defective code before it becomes an integration or field failure. The Session 01 baseline is:

```text
-std=c99 -Wall -Wextra -Wpedantic -Werror
```

| Option | Role | Boundary |
| --- | --- | --- |
| `-std=c99` | Selects the declared GCC language mode. | It does not remove every accepted extension. |
| `-Wall` | Enables a useful GCC-defined group of warnings. | It is not every warning GCC can issue. |
| `-Wextra` | Enables additional GCC warnings. | The exact set remains compiler and version dependent. |
| `-Wpedantic` | Requests GCC diagnostics relative to the selected ISO dialect. | It is not a complete portability or conformance audit. |
| `-Werror` | Makes enabled warnings build failures under project policy. | It is build policy, not a safety standard or correctness proof. |

This deliberately negative, complete program is intended for a configured diagnostic check, not as a positive example. GCC and Clang commonly diagnose the assignment in the condition under strict warning settings, but the exact message and option are compiler-version dependent.

```c
#include <stdbool.h>

int main(void)
{
    bool network_enabled = false;

    if (network_enabled = true)
    {
        return 0;
    }

    return 1;
}
```

The right response is to inspect intent: perhaps the code meant `if (network_enabled)`, perhaps it needs a comparison, or perhaps an assignment is actually intended and should be made unmistakable. When the intent is only to test the state, the corrected form is:

```c
if (network_enabled)
{
    return 0;
}
```

A cast, pragma, suppression, or warning-disable option is not a root-cause correction.

Warning-free compilation does not prove that all external inputs satisfy the API contract, that a target ABI matches the host, that every configured analysis rule was checked, or that code is MISRA compliant. Static-analysis results are also bounded by the tool, version, configuration, language mode, target model, and what the analysis can infer. A diagnostic is evidence to investigate, not automatically proof of a confirmed guideline violation. [MISRA C:2012 §5.3; GCC, Warning Options]

The review workflow for this lesson is therefore:

```text
API contract → language rule → applicable coding-standard guidance
→ compiler/static-analysis evidence → human review → deviation or documentation where required
```

Record the compiler name, version, target, language mode, and flags when a diagnostic supports an engineering decision. Deep analyzer configuration, rule coverage, and static-analysis workflow belong to M09. [M01-CR-014]

**Must remember:** investigate the source and contract behind a diagnostic. A quiet tool is useful evidence for its recorded configuration, never a universal correctness or compliance claim.

## 11. Makefile Fundamentals

Compilation translates each `.c` file into an object file. Linking combines object files and necessary libraries into an executable. A Makefile records those relationships so that a changed header rebuilds the object files that depend on it rather than relying on a developer to remember every command. [GNU Make Manual, Rule Syntax]

In a Make rule, the **target** is the file or named action to build, the **prerequisites** are its inputs, and the **recipe** is the indented command that produces it. This small two-object example preserves the Day 1 workflow without introducing advanced dependency generation or another build system. Recipe lines below begin with a literal tab.

```make
CC := gcc
CFLAGS := -std=c99 -Wall -Wextra -Wpedantic -Werror
TARGET := device_config

all: $(TARGET)

$(TARGET): main.o device_config.o
	$(CC) $^ -o $@

main.o: main.c device_config.h
	$(CC) $(CFLAGS) -c $< -o $@

device_config.o: device_config.c device_config.h
	$(CC) $(CFLAGS) -c $< -o $@

docs:
	doxygen Doxyfile

clean:
	rm -f $(TARGET) main.o device_config.o

.PHONY: all clean docs
```

`all` is the normal entry point. `$@` names the current target, `$^` is the complete prerequisite list, and `$<` is the first prerequisite. The explicit header dependencies make a change to `device_config.h` rebuild both affected object files. `clean` removes generated outputs; the optional `docs` target is appropriate only because the module uses Doxygen.

This is a host-oriented example. A cross build needs an explicitly selected compiler, target flags, linker settings, and libraries, not an assumption that host `gcc` represents the firmware environment. Do not add CMake, CI, unit-test frameworks, or automatic dependency-generation machinery to this introductory lesson.

**Must remember:** Make records build relationships. Keep compiler flags and header dependencies explicit so the command that produced an artifact can be reviewed and repeated.

## 12. Doxygen and API Documentation

An API contract tells a caller more than a function name and parameter types can express. It should state valid input, parameter direction, success and failure behavior, output behavior, units where relevant, and ownership only when ownership is actually transferred. The declaration, implementation, test expectations, and generated documentation must agree. [BARR-C:2018 §2.2; Doxygen Manual, Special Commands]

This is an intentionally incomplete public-declaration fragment. It demonstrates the documentation contract; the corresponding implementation is not part of this lesson.

```c
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Convert a validated decimal setting into a service limit.
 *
 * @param[in] p_text Null-terminated text accepted by the documented format.
 * @param[out] p_limit Output location written only when conversion succeeds.
 * @return true on success; false for invalid input, range failure, or null output.
 */
bool service_limit_from_text(const char *p_text, uint16_t *p_limit);
```

`@brief` gives the reader a short purpose statement. `@param[in]` identifies data consumed by the function, `@param[out]` identifies an output written by the function, and `@return` records the result contract. A reviewer should be able to compare the comment with the implementation and find the same preconditions, range rules, and failure behavior.

Good comments explain an assumption, unit, limitation, reason, or external reference that the code cannot express by itself. They should not repeat obvious syntax or hide a contradiction with the implementation. Doxygen can make useful interface documentation navigable, but generated output is only as trustworthy as the reviewed source comment. [BARR-C:2018 §2.2]

**Must remember:** document the observable contract—not just the happy path—and keep the comment, declaration, implementation, and caller expectations synchronized.

## 13. Reliability Review Workflow

Use this workflow when reviewing an M01-scale API or defect:

```text
contract → type and range analysis → behavior classification
→ applicable coding-standard guidance → compiler/static-analysis evidence
→ human review → deviation or documentation where required
```

Consider a device configuration setter that receives a 32-bit value, immediately casts it to a 16-bit output, and returns success. The defect is not “a missing cast”; the defect is an incomplete contract and an unproven narrowing conversion.

| Review step | Question | Appropriate outcome |
| --- | --- | --- |
| Contract | Which values are valid, and what happens to output on failure? | State accepted range, null-output policy, success result, and failure behavior. |
| Type and range analysis | Can the source domain fit the destination representation? | Check the range before conversion. |
| Behavior classification | Could the code dereference null, overflow, or use an invalid index? | Prevent any such case before the operation. |
| Guidance | Which selected MISRA or BARR-C practice is relevant? | Apply only the guidance that genuinely matches the construct and context. |
| Tool evidence | What did the recorded compiler or analyzer report? | Investigate the message and the tool's coverage boundary. |
| Human review | Does the implementation meet the product contract and target assumptions? | Correct the cause or document the justified decision. |
| Deviation or documentation | Is a non-compliance genuinely necessary and authorized? | Keep a scoped, reviewed record; do not use suppression as the first remedy. |

This is a design and code-review workflow, not a generic runtime-debugging chapter. It deliberately starts with the intended behavior and the language rules before relying on a tool result. If a deviation is relevant, it follows review and project authorization rather than being created automatically by a warning. [MISRA C:2012 §§5.3–5.4]

**Must remember:** a reliable correction follows evidence back to the contract and language rule. It does not start by silencing the symptom.

## 14. Embedded and Linux Applications

The M01 baseline transfers to both Embedded and Linux work because the boundary questions are the same even when the target, libraries, and deployment environment differ.

| Scenario | Engineering question | Required evidence | M01 boundary |
| --- | --- | --- | --- |
| Protocol-boundary input | What syntax, length, range, and complete-consumption rules make external data valid? | API contract, C99 conversion behavior, and focused tests. | Do not implement a complete assigned parser here. |
| Device-control API | Which values, units, states, and output changes are valid? | Public interface, range proof, and code review. | No MMIO or hardware-register lesson. |
| Sensor-value conversion | Can the source measurement fit the destination field without loss or sign error? | Type ranges, conversion review, and target-aware test values. | Detailed numerical processing belongs to M08. |
| Driver-facing data contract | Which names and state must remain private to one translation unit? | Header/source ownership, linkage review, and documented interface. | No Linux-driver internals. |
| Configuration parsing | Does every accepted text value map to a documented product value? | Syntax policy, conversion error handling, application range, and failure contract. | No full protocol algorithm. |
| Reproducible build | Which compiler, language mode, flags, and dependencies produced this artifact? | Recorded command or Make target and the selected target configuration. | No CMake, CI, or performance conclusion. |

An embedded product often has tighter target and toolchain constraints; a Linux service may have richer libraries and a different deployment path. Neither difference removes the need for explicit contracts, range checks, diagnostics, and review. Conversely, a host build, a fixed-width type, or a warning-free command does not establish target performance, byte order, or hardware behavior without matching evidence. [M01-CR-005]

**Must remember:** apply the same contract-first method across environments, then add the compiler, target, ABI, library, and operational evidence that the specific environment requires.

## 15. Key Takeaways and References

Use this review checklist before accepting an M01-scale change:

- Is the C99 baseline explicit, and are later standards or compiler extensions clearly labelled?
- Does the public contract state valid input, range, output behavior, success, and failure?
- Are bounds, pointer validity, complete input consumption, and conversion range established before use?
- Does every narrowing conversion have an explicit source-domain and destination-range reason?
- Has the code been classified under ISO C before a style rule, tool message, or target convention is discussed?
- Are scope, linkage, storage duration, and lifetime treated as separate concepts?
- Do diagnostics and static-analysis results include their compiler, tool, configuration, and target boundaries?
- Is any guideline claim limited to the selected scope, evidence, and authorized deviations?

M01-L01 stops at the language and engineering baseline. M01-L02 owns executable sections, target-memory interpretation, startup behavior, stack and heap failure analysis, binary tools, and optimization evidence. Later modules own advanced object representation, pointers and dispatch, macros, serialization, static-analysis workflows, and build-system depth.

### Claim-level reference allocation

| Claim family | Primary authority for this lesson | Use in M01-L01 |
| --- | --- | --- |
| C99 language semantics, conversions, behavior taxonomy, scope, linkage, storage duration, and library contracts | ISO/IEC 9899:1999 and applicable public WG14 material | Sections 2, 6–9. |
| MISRA categories, compliance evidence, diagnostics, deviations, and library-function input validity | MISRA C:2012 §§5.3–5.5, §6.1, §6.2, and Directive 4.11 | Sections 3, 4, 6, 10, and 13. Directive 4.14 remains unverified in the supplied reference. |
| C99 baseline, braces, parentheses, casts, comments, modules and headers, naming, initialization, fixed-width and signedness guidance, function practices, conditionals, and loops | BARR-C:2018 §§1.1, 1.3–1.6, 2.2, 4, 5.2–5.3, 6.2, 7, and 8 | Sections 2–7, 9, 11, and 12. |
| GCC language-mode and warning behavior | GCC documentation: *C Dialect Options* and *Warning Options* | Sections 2 and 10. |
| Basic build rules and automatic variables | GNU Make Manual: *Rule Syntax* and automatic-variable documentation | Section 11. |
| API documentation commands | Doxygen Manual: *Special Commands* and code-documentation guidance | Section 12. |

The DevLinux Week 1 Day 1 roadmap and Session 01 define this lesson's learning scope and the later exercise context. `Full-Embedded-C-Notes.md` remains discovery material only and is not an authority for the technical claims above. No stale repository path is used for CERT material; any future CERT reference must be an official, claim-relevant source.
