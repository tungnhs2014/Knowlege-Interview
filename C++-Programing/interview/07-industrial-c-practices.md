# 07 - Industrial C Practices: Interview Pack

## How To Use This Pack

For each question:

1. Give the short answer first.
2. Explain the mechanism and trade-offs.
3. Anchor the answer in a C or C++ API.
4. Connect it to production and debugging.
5. Name the common traps.
6. Be ready for the follow-up questions.

Strong answers distinguish language guarantees, project policy, tool behavior,
and product requirements.

## Beginner Questions

### 1. Why does a C team need a coding standard?

**Short answer**

A coding standard creates one reviewable set of rules for language use,
interfaces, ownership, errors, tools, and deviations. Its purpose is to reduce
defects and ambiguity, not merely to make formatting uniform.

**Deep explanation**

C permits many correct implementation styles and many dangerous ones. A
project standard defines choices that the language leaves open:

- supported C edition and compiler extensions;
- warning policy;
- integer and conversion rules;
- pointer ownership and lifetime conventions;
- buffer and string contracts;
- restricted APIs;
- error handling;
- concurrency and asynchronous-context rules;
- documentation, testing, and deviation process.

High-value rules should have a rationale. "Validate a shift count before
shifting" prevents undefined behavior. A formatting rule mainly reduces review
friction. Both can help, but they address different risks.

**C/C++ code or API anchor**

```c
typedef enum {
    SENSOR_OK = 0,
    SENSOR_INVALID_ARGUMENT,
    SENSOR_TIMEOUT
} SensorStatus;

SensorStatus sensor_read(
    Sensor *sensor,
    int *out_value);
```

A project standard can require named status codes, documented nullability, and
unchanged output on failure.

**Production and debug angle**

A useful standard makes reviews repeatable and lets CI enforce selected rules.
When a defect occurs, engineers can ask whether the contract, implementation,
review, tool configuration, or deviation process failed.

Public API contracts may be checked through Doxygen documentation using fields
such as `@brief`, `@param`, and `@return`, but generated documentation does not
replace source review or tests.

**Common traps**

- Treating a style formatter as the whole coding standard.
- Copying rules without understanding their rationale.
- Claiming compliance while deviations and third-party code are unmanaged.
- Enforcing rules manually when automation is practical.
- Applying every rule equally to generated, adopted, and first-party code
  without a defined policy.

**Follow-up questions**

- What belongs in a deviation record?
- Which rules would you automate first?
- How would you introduce a standard into legacy code?

### 2. Compare MISRA C, BARR-C, and SEI CERT C.

**Short answer**

MISRA C emphasizes controlled language use and a compliance process for
safety- and security-conscious C. BARR-C provides practical embedded coding
guidance. SEI CERT C focuses on secure coding and concrete vulnerability
classes. They overlap but are not interchangeable.

**Deep explanation**

A project can use ideas from all three through one resolved project policy:

- MISRA C helps restrict risky language constructs and structure compliance
  evidence.
- BARR-C emphasizes readable, portable, maintainable embedded C.
- CERT C explains secure-coding risks involving memory, integers, strings,
  expressions, I/O, and errors.

None replaces product requirements, architecture analysis, compiler
documentation, or testing. A MISRA compliance claim also requires the
applicable licensed guideline, scope, enforcement, investigated diagnostics,
deviations, adopted-code policy, competence, and retained evidence.

**C/C++ code or API anchor**

```c
bool packet_decode(
    const uint8_t *data,
    size_t length,
    Packet *out_packet);
```

All three bodies of guidance can influence this API: explicit-width types,
input validation, capacity, failure reporting, and reviewable control flow.

**Production and debug angle**

Teams should map external guidance into project rules and tool configurations.
When tools disagree, the project rule and language behavior guide triage; a
tool name is not the final argument.

**Common traps**

- Saying MISRA is only formatting.
- Saying one clean analyzer report proves MISRA compliance.
- Reproducing proprietary rule text without the licensed material.
- Combining standards without resolving conflicting or duplicate rules.
- Treating CERT C as a certification scheme.

**Follow-up questions**

- What evidence would you expect in a compliance review?
- Can a project deviate from a rule?
- How would you handle third-party source code?

### 3. What is undefined behavior, and why is it an industrial concern?

**Short answer**

Undefined behavior means the C standard imposes no requirements on the
program's behavior after the invalid operation. It is dangerous because a
compiler may optimize under the assumption that undefined behavior never
occurs.

**Deep explanation**

Examples include:

- out-of-bounds access;
- use-after-free;
- signed integer overflow;
- invalid shift counts;
- null dereference;
- reading an uninitialized automatic object;
- mismatched format arguments;
- invalid allocation-family use;
- data races in applicable language models.

Code that appears to work at `-O0` can change at `-O2`. The right response is to
remove the invalid operation, not to rely on one observed binary.

**C/C++ code or API anchor**

```c
static bool bit_mask(
    unsigned int bit,
    uint32_t *out_mask)
{
    if (out_mask == NULL || bit >= 32U) {
        return false;
    }

    *out_mask = UINT32_C(1) << bit;
    return true;
}
```

The range check occurs before the shift.

**Production and debug angle**

Use strict warnings, review, static analysis, UBSan, ASan, boundary tests, and
multiple optimization levels. A sanitizer finding is evidence of a defect, but
a clean run does not prove all paths are valid.

**Common traps**

- Calling undefined behavior merely "unpredictable output."
- Assuming hardware behavior overrides the C language rules.
- Fixing a symptom with `volatile`.
- Trusting only debug builds.
- Assuming unsigned arithmetic is always semantically correct because wrapping
  is defined.

**Follow-up questions**

- Undefined versus implementation-defined behavior?
- Which sanitizer would help with an invalid shift?
- Why can optimization expose a latent defect?

### 4. When should `assert` be used instead of runtime validation?

**Short answer**

Use `assert` for internal invariants whose failure indicates a programming
defect. Use runtime validation for external, expected, or recoverable invalid
conditions.

**Deep explanation**

Assertions can be removed when `NDEBUG` is defined. Therefore:

- required side effects must not be inside `assert`;
- public input validation cannot rely only on `assert`;
- the product needs a policy for assertion failure;
- `_Static_assert` should be used for compile-time invariants.

An internal helper may assert a precondition after a public boundary has checked
it.

**C/C++ code or API anchor**

```c
#include <assert.h>

static int first_value(
    const int *values,
    size_t count)
{
    assert(values != NULL);
    assert(count > 0U);
    (void)count;
    return values[0];
}

bool try_first_value(
    const int *values,
    size_t count,
    int *out_value)
{
    if (values == NULL || out_value == NULL || count == 0U) {
        return false;
    }

    *out_value = first_value(values, count);
    return true;
}
```

**Production and debug angle**

Build and test both assertion-enabled and `NDEBUG` configurations. Capture
enough diagnostic state to investigate invariant failures, while avoiding
unsafe logging from restricted contexts.

**Common traps**

- `assert(initialize_device())`.
- Using `assert` for malformed network input.
- Assuming assertions are always enabled in production.
- Continuing after a corrupted invariant without a defined safe policy.
- Replacing all runtime checks with assertions for performance.

**Follow-up questions**

- What should an embedded product do after an assertion failure?
- When would you use `_Static_assert`?
- Should assertions validate function parameters?

### 5. How do you avoid a C string buffer overflow?

**Short answer**

Carry the destination capacity, validate before writing, reserve one byte for
`'\0'`, use bounded operations, and check whether the result was truncated.

**Deep explanation**

A capacity of `N` can hold at most `N - 1` ordinary characters in a
null-terminated string. "Bounded" does not automatically mean "safe":

- `strncpy` may not terminate;
- `snprintf` can truncate;
- `fgets` may read only part of a line;
- integer conversions need end-pointer and range checks.

The API must define whether truncation is accepted, rejected, or reported
separately.

**C/C++ code or API anchor**

```c
static bool copy_c_string(
    char *destination,
    size_t capacity,
    const char *source)
{
    if (destination == NULL || source == NULL || capacity == 0U) {
        return false;
    }

    size_t index = 0U;
    while (source[index] != '\0' && index + 1U < capacity) {
        destination[index] = source[index];
        ++index;
    }

    destination[index] = '\0';
    return source[index] == '\0';
}
```

**Production and debug angle**

Test zero capacity, empty source, exact fit, one-byte-too-small, large input, and
invalid pointers. Run ASan on host tests and preserve any fuzz input that finds
a failure.

**Common traps**

- Forgetting the terminator.
- Assuming `strncpy` always terminates.
- Ignoring `snprintf`'s result.
- Using external text as a format string.
- Using `atoi` when failure and range matter.

**Follow-up questions**

- How do you detect `snprintf` truncation?
- Why was `gets` removed?
- How would you parse an integer safely?

## Mid-Level Questions

### 6. Explain correct `errno` handling.

**Short answer**

First check the API's documented primary failure result. Only then inspect and
save `errno` if that API defines it. Do not treat a stale nonzero `errno` as a
failure.

**Deep explanation**

`errno` is a library side channel, not a universal process status. A successful
call does not generally have to clear it. Another library call may overwrite
the diagnostic before it is logged or translated.

A module should often translate platform errors into stable domain errors so
callers do not depend directly on hosted implementation values.

**C/C++ code or API anchor**

```c
#include <errno.h>
#include <stdio.h>

typedef enum {
    OPEN_OK = 0,
    OPEN_NOT_FOUND,
    OPEN_PERMISSION,
    OPEN_OTHER
} OpenStatus;

static OpenStatus open_input(
    const char *path,
    FILE **out_file)
{
    if (path == NULL || out_file == NULL) {
        return OPEN_OTHER;
    }

    FILE *file = fopen(path, "rb");
    if (file != NULL) {
        *out_file = file;
        return OPEN_OK;
    }

    int saved_errno = errno;
    if (saved_errno == ENOENT) {
        return OPEN_NOT_FOUND;
    }
    if (saved_errno == EACCES) {
        return OPEN_PERMISSION;
    }
    return OPEN_OTHER;
}
```

**Production and debug angle**

Log or retain the saved platform value at the boundary where it is meaningful,
but return a domain error to normal callers. Use hosted tracing tools only when
the environment supports them.

**Common traps**

- Checking `errno` before checking the function result.
- Assuming success clears `errno`.
- Calling `perror` or another function before preserving context needed later.
- Exposing raw `errno` as a portable product API.
- Resetting `errno` before every library call without a contract reason.

**Follow-up questions**

- Which functions require setting `errno` to zero before a call?
- Is `errno` thread-local?
- How would you preserve the underlying diagnostic in a domain error?

### 7. Review this `realloc` code and fix it.

```c
bool resize(int **items, size_t count)
{
    *items = realloc(*items, count * sizeof **items);
    return *items != NULL;
}
```

**Short answer**

It can dereference a null `items`, overflow the size multiplication, lose the
original allocation on failure, and leaves zero-count behavior undefined by the
project.

**Deep explanation**

`realloc` preserves the original allocation when a nonzero-size request fails,
but direct assignment overwrites the only owning pointer with null. The
multiplication happens before `realloc` and can wrap. A project should define
zero count explicitly.

**C/C++ code or API anchor**

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

static bool resize(
    int **items,
    size_t count)
{
    if (items == NULL || count > SIZE_MAX / sizeof **items) {
        return false;
    }

    if (count == 0U) {
        free(*items);
        *items = NULL;
        return true;
    }

    void *temporary = realloc(*items, count * sizeof **items);
    if (temporary == NULL) {
        return false;
    }

    *items = temporary;
    return true;
}
```

**Production and debug angle**

Test growth, shrink, zero count, multiplication overflow, allocation failure,
and preservation of old data. Inject allocator failure or wrap allocation
behind a test seam. Run ASan and static analysis.

**Common traps**

- Casting `realloc` in C.
- Nulling one pointer while aliases still dangle.
- Assuming `realloc` initializes grown storage.
- Treating zero-size behavior as portable business logic.
- Forgetting that `count` describes elements, not bytes.

**Follow-up questions**

- What happens to the old allocation on failure?
- How would you preserve the logical element count?
- Would this API be safe with shared aliases?

### 8. Compare compiler warnings, static analysis, and dynamic analysis.

**Short answer**

Compiler warnings diagnose suspicious translation constructs. Static analysis
reasons about source paths without one runtime execution. Dynamic analysis
observes an instrumented execution. They are complementary.

**Deep explanation**

Warnings and analyzers depend on compiler/tool version and configuration.
Static analyzers can inspect unexecuted paths but may produce findings that need
triage. Dynamic tools often provide concrete stack traces, but only for paths
that execute.

Typical dynamic roles:

- AddressSanitizer (ASan): bounds and lifetime errors;
- UndefinedBehaviorSanitizer (UBSan): selected undefined behavior;
- ThreadSanitizer (TSan): hosted data races;
- Valgrind Memcheck: invalid access, uninitialized values, leaks, and allocation
  misuse on supported systems.

Representative static-analysis paths include GCC `-fanalyzer`,
`clang-tidy`, Clang Static Analyzer, and `cppcheck`.

**C/C++ code or API anchor**

```bash
cc -std=c17 -Wall -Wextra -Wpedantic -Werror \
   -Wconversion -Wshadow -Wformat=2 \
   module.c -o module

cc -std=c17 -O1 -g3 \
   -fsanitize=address,undefined \
   -fno-omit-frame-pointer \
   module.c -o module-sanitized
```

**Production and debug angle**

Feed static tools the real include paths, macros, target model, and compile
commands. Archive analyzer and sanitizer logs in CI. Run separate TSan jobs
where supported.

**Common traps**

- Believing `-Wall` enables every warning.
- Enabling every analyzer check and ignoring the noise.
- Treating a clean sanitizer run as proof.
- Running tools with the wrong configuration.
- Promoting every third-party warning to an unmanageable global error.

**Follow-up questions**

- Which tool would you choose for a use-after-free?
- Can ASan detect a data race?
- Why might diagnostics differ between `-O0` and `-O2`?

### 9. How would you test hardware-dependent C code?

**Short answer**

Separate policy from hardware access through a narrow interface. Test policy on
the host with a fake or stub, then use integration and target tests for real
hardware behavior.

**Deep explanation**

Direct register access inside policy code makes failure injection and
deterministic testing difficult. An interface can inject:

- sensor values;
- time;
- transport behavior;
- allocation failure;
- timeout and retry conditions.

Host tests validate logic quickly. They do not replace target validation for
timing, MMIO, interrupts, ABI, memory limits, or electrical behavior.

**C/C++ code or API anchor**

```c
typedef bool (*ReadSensor)(
    void *context,
    int *out_value);

typedef struct {
    ReadSensor read;
    void *context;
} Sensor;

static bool alarm_required(
    const Sensor *sensor,
    bool *out_required)
{
    if (sensor == NULL
        || sensor->read == NULL
        || out_required == NULL) {
        return false;
    }

    int value = 0;
    if (!sensor->read(sensor->context, &value)) {
        return false;
    }

    *out_required = value > 100;
    return true;
}
```

**Production and debug angle**

Test success, threshold boundaries, failure, repeated calls, timeout, and
invalid interface state. Keep target-specific access in one reviewed
implementation.

Unity can provide a small C unit-test framework; CMock or FFF can help build
test doubles. Google Test can drive C modules from C++ when the C interface
preserves C linkage. Framework choice does not replace good test cases.

**Common traps**

- Building a complete hardware simulator for a small policy test.
- Mocking private implementation details.
- Claiming host tests validate hardware timing.
- Letting the fake's behavior differ from the documented interface.
- Passing a fake context whose lifetime has ended.

**Follow-up questions**

- Fake versus mock?
- How would you inject time?
- What must still be tested on the target?

### 10. Design a safe formatted-output API.

**Short answer**

Pass destination capacity, use `snprintf`, inspect its result, guarantee a
defined destination state, and make truncation visible to the caller.

**Deep explanation**

`snprintf` returns the number of characters that would have been written,
excluding the terminator, when formatting succeeds. A nonnegative result greater
than or equal to capacity indicates truncation.

The API should distinguish invalid arguments, formatting error, truncation, and
success when callers need different policies.

**C/C++ code or API anchor**

```c
#include <stddef.h>
#include <stdio.h>

typedef enum {
    FORMAT_OK = 0,
    FORMAT_INVALID_ARGUMENT,
    FORMAT_ERROR,
    FORMAT_TRUNCATED
} FormatStatus;

static FormatStatus format_sample(
    char *destination,
    size_t capacity,
    unsigned int id,
    int value)
{
    if (destination == NULL || capacity == 0U) {
        return FORMAT_INVALID_ARGUMENT;
    }

    int required = snprintf(
        destination,
        capacity,
        "id=%u value=%d",
        id,
        value);

    if (required < 0) {
        destination[0] = '\0';
        return FORMAT_ERROR;
    }

    if ((size_t)required >= capacity) {
        return FORMAT_TRUNCATED;
    }

    return FORMAT_OK;
}
```

**Production and debug angle**

Test exact fit, one byte short, zero capacity, large values, and any supported
encoding conditions. Decide whether truncated logs may be emitted or must be
rejected.

**Common traps**

- Ignoring the result.
- Assuming truncation is always harmless.
- Passing attacker-controlled text as the format.
- Forgetting capacity zero.
- Mismatching variadic argument types and format specifiers.

**Follow-up questions**

- Is the destination terminated after truncation?
- How would C++ express the result?
- Would you allocate a larger buffer and retry?

### 11. Why can high line coverage coexist with weak tests?

**Short answer**

Line coverage shows that lines executed. It does not show that outputs,
boundaries, failure policies, or requirements were meaningfully checked.

**Deep explanation**

A test can execute a branch without asserting its result. It can miss:

- just-outside boundary values;
- partial failures;
- cleanup after failure;
- invalid state transitions;
- repeated-call behavior;
- race conditions;
- incorrect requirements.

Branch and mutation information can reveal additional weaknesses, but human
review of test intent remains necessary.

**C/C++ code or API anchor**

```c
void weak_test(void)
{
    char output[8];
    (void)copy_c_string(output, sizeof output, "too-long-input");
    /* The line executed, but truncation was never checked. */
}
```

Better tests assert the return status and destination policy.

**Production and debug angle**

Use coverage to locate unexecuted risk, fault injection to reach failures, and
mutation testing selectively to assess whether tests notice changed behavior.
Preserve production failures as regression cases.

**Common traps**

- Treating a percentage as a quality target independent of risk.
- Adding tests that execute code without assertions.
- Excluding failure paths because they are hard to trigger.
- Mocking so much that real integration contracts are never tested.
- Optimizing for coverage rather than defect detection.

**Follow-up questions**

- What does mutation testing add?
- Which coverage metric would you inspect for a parser?
- How would you test allocation failure?

## Senior Questions

### 12. Design a coding-standard adoption and deviation process.

**Short answer**

Select a language and rule baseline, tailor it to product risk, define automated
and manual enforcement, manage deviations and adopted code, assign ownership,
and retain reviewable evidence.

**Deep explanation**

A practical rollout includes:

1. Define scope, products, language editions, compilers, and supported
   configurations.
2. Build one project standard from applicable MISRA, BARR-C, CERT C, compiler,
   and local rules.
3. Classify rules by risk and enforcement method.
4. Configure warnings and analyzers with the real build model.
5. Establish code-review checklists and training.
6. Define deviation records and approvers.
7. Set policy for generated, adopted, and third-party code.
8. Baseline legacy findings with owners and reduction plans.
9. Gate new findings in CI.
10. Review rules and tool versions periodically.

**C/C++ code or API anchor**

```text
Rule ID: PROJECT-STR-004
Scope: Public text-formatting APIs
Requirement: Destination capacity and truncation status are explicit
Enforcement: API review + static checks + boundary tests
Deviation: Requires risk, compensating control, owner, and review date
```

**Production and debug angle**

Measure whether the process finds and prevents defects. Track recurring
deviations, noisy checks, escaped defects, and configurations that lack
evidence.

**Common traps**

- Importing a standard without tailoring or training.
- Requiring zero legacy findings immediately and causing mass suppression.
- Treating tool output as the standard itself.
- Allowing permanent deviations with no review trigger.
- Ignoring build variants and third-party boundaries.

**Follow-up questions**

- How do you prevent new debt while reducing a legacy baseline?
- Who approves deviations?
- How do compiler upgrades affect the process?

### 13. What evidence is needed before claiming MISRA compliance?

**Short answer**

A credible claim needs the applicable licensed guideline, defined scope,
adopted rule set, enforcement and tool strategy, investigated messages,
documented deviations, adopted-code handling, competent review, and retained
project evidence.

**Deep explanation**

Compliance is not equivalent to:

- compiling without warnings;
- running one static analyzer;
- reading a public summary;
- having no unsuppressed messages.

The project must explain which guidelines apply, how each is enforced, how tool
limitations are handled, and why deviations do not create unacceptable risk.
Tool qualification and regulated safety evidence are separate domain concerns.

**C/C++ code or API anchor**

```text
Compliance evidence set:
- Guideline edition and project scope
- Guideline enforcement plan
- Tool versions and configurations
- Analysis reports and reviewed dispositions
- Deviation records
- Adopted-code records
- Review and test evidence
```

**Production and debug angle**

Evidence should be reproducible for the released configuration. If a finding is
suppressed, an auditor or maintainer should still be able to understand the
technical decision.

**Common traps**

- Saying "MISRA certified code" without defining the claim.
- Treating proprietary rule text casually.
- Counting analyzer messages without reviewing tool coverage.
- Omitting generated or third-party code from scope silently.
- Confusing a clean report with product safety.

**Follow-up questions**

- What is a deviation permit?
- Can two tools enforce the same rule differently?
- What changes when a compiler version changes?

### 14. Build a risk-based CI matrix for an industrial C product.

**Short answer**

Map each risk to a repeatable job: strict builds, configuration variants,
static analysis, unit and integration tests, sanitizer builds, target builds
and tests, plus scheduled fuzzing or mutation testing where justified.

**Deep explanation**

Example matrix:

| Job | Purpose | Frequency |
| --- | --- | --- |
| GCC C17 strict debug build | Language and warning baseline | Every change |
| Production optimization build | Release configuration | Every change |
| Configuration matrix | Hidden `#if` programs | Every change or merge |
| Static analysis | Path and rule defects | Every change or merge |
| Unit tests | Isolated behavior and failures | Every change |
| ASan/UBSan | Host memory and selected UB | Every change |
| TSan | Hosted data-race checks | Merge or scheduled |
| Target build/tests | ABI, integration, timing, hardware | Merge/release |
| Fuzzing | Parser and input-boundary discovery | Scheduled |
| Mutation testing | Test-suite sensitivity | Scheduled |

Jobs should record tool versions and retain logs, reports, binaries, and failing
inputs needed for diagnosis.

Make or CMake can expose distinct normal, analysis, sanitizer, host-test, and
target configurations. CMake can generate `compile_commands.json` for tools
that need the exact compilation model.

**C/C++ code or API anchor**

```bash
cc -std=c17 -O2 -Wall -Wextra -Wpedantic -Werror \
   -Wconversion -Wshadow -Wformat=2 \
   src/*.c -o product

cc -std=c17 -O1 -g3 \
   -fsanitize=address,undefined \
   -fno-omit-frame-pointer \
   tests/*.c src/policy.c -o tests-sanitized
```

**Production and debug angle**

Fast high-signal jobs should block changes early. Expensive jobs can run later,
but failures still need ownership and a reproducible artifact.

**Common traps**

- One debug compiler configuration only.
- Running sanitizer jobs without meaningful tests.
- Letting scheduled failures remain unowned.
- Using the same binary for incompatible sanitizers.
- Failing third-party code under a policy designed for first-party code without
  a boundary strategy.

**Follow-up questions**

- What would you block on every pull request?
- How do you handle flaky target tests?
- Which artifacts would you retain?

### 15. How would you design a stable error model across C modules and a POSIX boundary?

**Short answer**

Use domain-specific status codes inside the product, translate platform or
library failures at the boundary, preserve useful diagnostics separately, and
define output and retry behavior for each category.

**Deep explanation**

A stable model should distinguish conditions callers handle differently:

- invalid input;
- timeout or transient unavailable state;
- resource exhaustion;
- permanent I/O failure;
- bad internal state;
- integrity failure.

Raw `errno` values are useful diagnostic detail but may be platform-specific.
The boundary wrapper should capture them promptly and translate them.

**C/C++ code or API anchor**

```c
typedef enum {
    STORAGE_OK = 0,
    STORAGE_INVALID_ARGUMENT,
    STORAGE_NOT_FOUND,
    STORAGE_PERMISSION,
    STORAGE_RESOURCE_EXHAUSTED,
    STORAGE_IO_ERROR
} StorageStatus;

typedef struct {
    StorageStatus status;
    int platform_error;
} StorageResult;
```

C++ may wrap the same C result in an expected-style type while preserving the C
ABI.

**Production and debug angle**

Logs can retain `platform_error`, path or device identity, and operation phase
without exposing unstable details to ordinary callers. Metrics should group by
stable domain status.

**Common traps**

- Returning raw `errno` everywhere.
- Flattening all failures into `false`.
- Mixing logging and error propagation so one replaces the other.
- Partially modifying outputs on failure without documentation.
- Retrying permanent failures indefinitely.

**Follow-up questions**

- How would you version an error enum in a public ABI?
- Should error text be returned from the API?
- How do you represent partial success?

### 16. Review a production logging design.

**Short answer**

Check level semantics, stable event identity, bounded formatting, privacy,
rate limiting, execution context, sink-full behavior, recursion, and whether
logging changes required program behavior.

**Deep explanation**

Logging is an operational API. A production design should define:

- error, warning, information, and debug semantics;
- stable event IDs;
- maximum record size;
- truncation policy;
- allocation, blocking, and locking behavior;
- thread and interrupt safety;
- rate limits;
- privacy and secret filtering;
- behavior when the sink is unavailable;
- compile-time versus runtime filtering.

**C/C++ code or API anchor**

Unsafe:

```c
LOG_DEBUG("status=%d", read_and_clear_status());
```

Safer:

```c
int status = read_and_clear_status();
LOG_DEBUG_EVENT(
    LOG_EVENT_STATUS_READ,
    "status=%d",
    status);
```

Required side effects remain outside the log expression.

**Production and debug angle**

Test disabled logging, full buffers, repeated failures, malformed external text,
and concurrent calls. Logs should help reproduce failures without leaking
credentials or overwhelming timing and storage budgets.

**Common traps**

- External text used as the format string.
- Logging secrets.
- Unbounded formatting.
- Logging from a restricted asynchronous context without a contract.
- Recursive logging after the logger itself fails.
- Removing required work when debug logs compile out.

**Follow-up questions**

- Compile-time versus runtime filtering?
- How would you rate-limit repeated faults?
- What should happen when the log sink is full?

### 17. How would you manage a large legacy warning and analyzer backlog?

**Short answer**

Prevent new findings, classify the existing backlog by risk, assign ownership,
fix high-risk issues first, and reduce a reviewed baseline over time. Do not
hide the entire backlog with global suppression.

**Deep explanation**

A practical strategy:

1. Freeze tool versions and capture a reproducible baseline.
2. Remove configuration errors and duplicate/noisy checks.
3. Classify findings by memory safety, undefined behavior, security,
   correctness, maintainability, and false positive.
4. Block new findings in changed code.
5. Assign owners and milestones for existing findings.
6. Use narrow suppressions with rationale and review dates.
7. Add regression tests for fixed defects.
8. Track baseline size and escaped defects.

**C/C++ code or API anchor**

```text
Suppression record:
- Tool/check ID
- File and narrow scope
- Technical reason
- Risk assessment
- Compensating test/review
- Owner
- Revisit date
```

**Production and debug angle**

Correlate findings with crash reports, sanitizer failures, and field defects.
This helps prioritize real risk instead of chasing count alone.

**Common traps**

- Turning on `-Werror` for thousands of legacy findings with no migration plan.
- Blanket disabling warning groups.
- Labeling difficult findings as false positives without investigation.
- Fixing warnings with unsafe casts.
- Allowing the baseline to grow.

**Follow-up questions**

- How do you enforce "no new warnings"?
- What gets fixed first?
- When is a suppression justified?

## Coding Tasks

### 18. Coding task: implement a bounded packet decoder.

**Prompt**

Decode a three-byte message:

- byte 0: kind, valid range `0..2`;
- bytes 1 and 2: big-endian `uint16_t`;
- preserve the output object on failure.

**Short answer**

Validate pointers, length, and kind before committing a local parsed object to
the caller.

**Deep explanation**

This task checks boundary ownership, validation order, endian handling, named
errors, and output invariants.

**C/C++ code or API anchor**

```c
#include <stddef.h>
#include <stdint.h>

typedef enum {
    DECODE_OK = 0,
    DECODE_INVALID_ARGUMENT,
    DECODE_SHORT_INPUT,
    DECODE_INVALID_KIND
} DecodeStatus;

typedef struct {
    uint8_t kind;
    uint16_t value;
} Message;

static DecodeStatus decode_message(
    const uint8_t *data,
    size_t length,
    Message *out_message)
{
    if (data == NULL || out_message == NULL) {
        return DECODE_INVALID_ARGUMENT;
    }
    if (length < 3U) {
        return DECODE_SHORT_INPUT;
    }
    if (data[0] > 2U) {
        return DECODE_INVALID_KIND;
    }

    Message parsed = {
        .kind = data[0],
        .value = (uint16_t)(
            ((uint16_t)data[1] << 8)
            | (uint16_t)data[2])
    };

    *out_message = parsed;
    return DECODE_OK;
}
```

**Production and debug angle**

Test null pointers, lengths `0..3`, every valid kind, invalid kinds, and endian
values such as `0x0000`, `0x00ff`, and `0xffff`. Add a fuzz harness with ASan
and UBSan for the parser.

**Common traps**

- Reading before checking length.
- Casting bytes to a native structure.
- Updating output before all validation succeeds.
- Assuming external enum values are valid.
- Shifting a signed `char`.

**Follow-up questions**

- How would you add a payload length?
- How would you version the format?
- What should happen to trailing bytes?

### 19. Coding task: design a fakeable timeout-dependent operation.

**Prompt**

Design a C function that polls a device until ready or until a timeout, while
remaining deterministic in a host unit test.

**Short answer**

Inject the device query and clock operations through a context-bearing
interface. Do not read a global clock directly in the policy.

**Deep explanation**

The task tests dependency injection, deterministic time, error propagation,
loop termination, and fake design.

**C/C++ code or API anchor**

```c
#include <stdbool.h>
#include <stdint.h>

typedef bool (*IsReady)(
    void *context,
    bool *out_ready);

typedef uint32_t (*NowMs)(
    void *context);

typedef struct {
    IsReady is_ready;
    NowMs now_ms;
    void *device_context;
    void *clock_context;
} WaitOps;

typedef enum {
    WAIT_OK = 0,
    WAIT_INVALID_ARGUMENT,
    WAIT_IO_ERROR,
    WAIT_TIMEOUT
} WaitStatus;

static WaitStatus wait_until_ready(
    const WaitOps *ops,
    uint32_t timeout_ms)
{
    if (ops == NULL
        || ops->is_ready == NULL
        || ops->now_ms == NULL) {
        return WAIT_INVALID_ARGUMENT;
    }

    const uint32_t start_ms = ops->now_ms(ops->clock_context);

    for (;;) {
        bool ready = false;
        if (!ops->is_ready(ops->device_context, &ready)) {
            return WAIT_IO_ERROR;
        }
        if (ready) {
            return WAIT_OK;
        }

        const uint32_t now_ms = ops->now_ms(ops->clock_context);
        const uint32_t elapsed_ms = now_ms - start_ms;
        if (elapsed_ms >= timeout_ms) {
            return WAIT_TIMEOUT;
        }
    }
}
```

**Production and debug angle**

Unsigned subtraction makes the elapsed-time comparison work across one
`uint32_t` clock wrap. The contract must require the operation and timeout to
span less than `2^32` ticks. The fake can advance time deterministically and
force I/O failure. Add a maximum-call safety guard in tests to expose a
non-advancing fake.

**Common traps**

- Global time source.
- Busy loop with no target scheduling policy.
- Comparing absolute timestamps with `now >= deadline`.
- Allowing an operation to span the clock's full modulus.
- Fake clock that never advances.

**Follow-up questions**

- Why is unsigned elapsed-time subtraction wrap-safe under the stated bound?
- Should the operation sleep or yield?
- How would you test a timeout that crosses the wrap point?

## Debugging Scenarios

### 20. Debugging: release build fails, debug build passes.

**Scenario**

The program succeeds at `-O0` but corrupts output at `-O2`. No compiler warning
is emitted.

**Short answer**

Suspect undefined behavior, uninitialized data, lifetime errors, aliasing,
out-of-bounds access, or a missing synchronization contract. Reproduce the
exact build and use layered evidence.

**Deep explanation**

Optimization is allowed to assume the C rules are followed. The release build
may expose a latent defect rather than create one. Compare source, generated
code only when useful, and runtime diagnostics without concluding that turning
optimization off is a fix.

**C/C++ code or API anchor**

```bash
cc -std=c17 -O1 -g3 \
   -Wall -Wextra -Wpedantic \
   -fsanitize=address,undefined \
   -fno-omit-frame-pointer \
   failing.c -o failing-sanitized

gdb ./failing-sanitized
```

Also run static analysis with the production macros and include paths.

**Production and debug angle**

Capture the exact input, compiler version, optimization flags, configuration,
and target. Reduce the failure and add a regression test before or with the
fix. Use `gdb` to inspect concrete backtraces, frames, variables, and memory
when the failing environment supports it.

For hosted user-space failures, `strace` may expose system-call errors,
`ltrace` may expose selected library calls, and `perf` may help profile
supported events. They are environment-specific tools, not core C diagnostics.

**Common traps**

- Shipping `-O0` as the fix.
- Adding `volatile` randomly.
- Assuming the compiler is wrong before checking UB.
- Debugging a different configuration.
- Ignoring a sanitizer report because the unsanitized build behaves
  differently.

**Follow-up questions**

- Which UB classes are optimization-sensitive?
- When would you inspect assembly?
- How do you verify a suspected compiler defect?

### 21. Debugging: sanitizer job is clean, but production still crashes.

**Scenario**

ASan and UBSan pass in CI, yet a target product crashes after several days.

**Short answer**

A clean sanitizer run covers only enabled checks, executed paths, and the host
environment. Investigate coverage, target differences, concurrency, timing,
resource exhaustion, and unsupported instrumentation gaps.

**Deep explanation**

Possible gaps:

- the failing path was not tested;
- target allocation or layout differs;
- the issue is a race not covered by ASan/UBSan;
- timing changes hide the defect;
- MMIO or interrupt behavior is target-specific;
- long-duration resource exhaustion is absent from short tests;
- a sanitizer ignore list excludes the path.

**C/C++ code or API anchor**

Use separate jobs:

```bash
# Memory and selected UB
cc -fsanitize=address,undefined ...

# Hosted data race checks, where supported
cc -fsanitize=thread ...
```

Add stress, fault-injection, target trace, and long-duration tests according to
the suspected risk.

**Production and debug angle**

Preserve crash state, reset reason, recent stable event IDs, resource counters,
and configuration. Build a minimal reproducer and convert it into the
appropriate host or target regression test.

**Common traps**

- Saying sanitizers have "zero false negatives."
- Assuming host memory behavior equals target behavior.
- Combining incompatible sanitizers in one job.
- Running only happy-path unit tests.
- Ignoring duration and resource pressure.

**Follow-up questions**

- What does TSan detect?
- How would you diagnose resource exhaustion?
- Which target evidence would you capture?

### 22. Debugging: a warning appears only after a compiler upgrade.

**Scenario**

A new compiler version reports hundreds of warnings, including third-party
code.

**Short answer**

Do not suppress everything or blindly fix with casts. Reproduce the old and new
toolchains, classify changed diagnostics, separate first-party and third-party
policy, and create a controlled migration.

**Deep explanation**

Warnings are version-specific. The upgrade may:

- add diagnostics;
- move warnings into a group;
- improve data-flow analysis;
- diagnose code only under optimization;
- change system-header behavior.

Determine which warnings reveal real defects, which require code modernization,
and which are policy or tool-noise issues.

**C/C++ code or API anchor**

```text
Migration:
1. Record old/new compiler versions and flags.
2. Group warnings by diagnostic ID.
3. Fix high-risk UB, format, lifetime, and conversion findings first.
4. Apply narrow third-party boundaries.
5. Baseline remaining reviewed findings.
6. Block new warnings.
```

**Production and debug angle**

Run tests and analyzers after fixes. Unsafe casts may silence the warning while
preserving the bug, so review semantic changes carefully.

**Common traps**

- Global `-w`.
- Removing `-Werror` permanently without a migration plan.
- Editing vendor code directly with no update strategy.
- Treating every new warning as a false positive.
- Measuring success only by warning count.

**Follow-up questions**

- How do you isolate third-party warnings?
- Which warning classes block release immediately?
- How would you prevent baseline growth?

## Rapid-Fire Review Prompts

Use these to practice concise judgment:

1. Why is `strncpy` not a universal safe replacement for `strcpy`?
2. Why can direct assignment from `realloc` leak memory?
3. Why should required work stay outside a log macro?
4. Why is `-Wall` not a complete warning policy?
5. Why does static analysis need the real compile configuration?
6. Why is a fake often less brittle than a strict mock?
7. Why does line coverage not measure assertion quality?
8. Why should a platform error be translated at a module boundary?
9. Why should analyzer suppressions have owners and review dates?
10. What does a fuzzing crash need before the issue is closed?

## Final Interview Checklist

A strong candidate should be able to:

- explain industrial quality as layered evidence;
- compare MISRA C, BARR-C, and CERT C accurately;
- reason about undefined behavior instead of relying on observed output;
- design explicit input, capacity, ownership, and error contracts;
- distinguish `assert`, runtime validation, error codes, and `errno`;
- review strings, formatted I/O, allocation, and `realloc` safely;
- explain warning, analyzer, sanitizer, and test blind spots;
- make hardware-dependent policy host-testable;
- choose fakes, stubs, and mocks deliberately;
- design risk-based CI and compliance evidence;
- debug release-only and target-only failures methodically;
- handle legacy findings without hiding risk;
- explain when fuzzing, mutation testing, or formal verification adds useful
  evidence and what each method still cannot prove.

## Reference Notes

- C17 is the practical language baseline for code anchors in this pack.
- MISRA compliance requires the applicable licensed guidance and a documented
  project process.
- BARR-C and SEI CERT C provide complementary embedded and secure-coding
  guidance.
- Warning, analyzer, sanitizer, and tracing behavior is tool- and
  platform-specific; verify commands against the selected versions.
