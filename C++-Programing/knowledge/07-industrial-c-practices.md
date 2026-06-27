# 07 - Industrial C Practices

## 1. Goal

Industrial C is not a different C language. It is the disciplined use of C in
software that must remain understandable, testable, diagnosable, and reliable
after the original author has moved on.

After this chapter, you should be able to:

- explain why coding standards matter;
- distinguish MISRA C, BARR-C, and SEI CERT C;
- design defensive C APIs with explicit contracts;
- validate inputs before dangerous operations;
- use error codes, `errno`, and `assert` correctly;
- handle strings and formatted I/O with explicit capacities;
- design useful, bounded logging;
- compare compiler warnings, static analysis, and dynamic analysis;
- test hardware-dependent code through narrow interfaces;
- place repeatable quality checks in CI;
- explain what tools can detect and what they cannot prove.

This chapter uses C17 as its language baseline. Tool options and availability
remain compiler- and platform-specific.

## 2. Why It Matters

C gives programmers direct control over memory, representation, and execution.
That control is useful in embedded systems, libraries, protocol stacks, and
performance-sensitive software. It also means that many important contracts are
not enforced automatically.

A function may accept:

- a null pointer;
- a buffer that is too short;
- an invalid enum value;
- a negative value converted to a huge `size_t`;
- a count whose multiplication overflows;
- an object whose lifetime has ended.

The compiler can catch some mistakes. Static analysis can catch others. Tests
and sanitizers can expose defects on executed paths. Code review can reason
about design and domain rules. None of these layers is sufficient by itself.

Industrial practice matters because real failures are rarely isolated syntax
errors. They are usually contract failures:

- nobody defined who owns an allocation;
- a return value was ignored;
- a parser trusted an external length;
- an assertion replaced runtime validation;
- one build configuration was never compiled;
- a warning was globally disabled;
- a test checked only the happy path;
- a clean tool report was mistaken for proof of correctness.

For embedded products, these failures can be especially expensive because the
software may run for years with limited memory, limited observability, and no
safe opportunity for manual recovery.

## 3. Mental Model: Layered Assurance

Think of industrial quality as a stack of evidence:

```text
Requirements and failure policy
            |
Project coding standard
            |
Defensive API and module design
            |
Compiler diagnostics
            |
Static analysis and code review
            |
Unit and integration tests
            |
Sanitizers and dynamic tools
            |
Target verification
            |
Repeatable CI evidence
```

Each layer answers different questions.

- Requirements define what correct behavior means.
- A coding standard restricts risky or inconsistent source practices.
- API design makes contracts visible.
- Warnings identify suspicious translations.
- Static analysis reasons about paths without running the program.
- Tests check expected behavior for selected cases.
- Dynamic tools observe an instrumented execution.
- Target verification checks assumptions that host tools cannot model.
- CI repeats the checks and preserves evidence.

A clean result at one layer does not cancel a failure at another. For example:

- passing tests do not make undefined behavior valid;
- no compiler warning does not prove an input is trusted;
- 100% line coverage does not prove assertions are meaningful;
- a clean sanitizer run says nothing about paths that were never executed.

## 4. Coding Standards

### 4.1 What a coding standard should do

A useful coding standard is an engineering agreement. It should define:

- the supported C language edition;
- accepted compiler extensions;
- warning and static-analysis policy;
- naming and formatting rules;
- type, conversion, and integer policies;
- pointer, ownership, and lifetime rules;
- string and buffer rules;
- restricted or prohibited APIs;
- error-handling conventions;
- concurrency and asynchronous-context rules;
- documentation expectations;
- how deviations are requested, reviewed, and recorded.

Formatting consistency helps reviews, but the highest-value rules prevent real
defects. A rule such as "validate a shift count before shifting" protects
behavior. A rule such as "put one space after a comma" mainly protects
readability. Both can be useful, but they do not have equal risk.

### 4.2 MISRA C, BARR-C, and SEI CERT C

These references overlap, but they serve different purposes.

| Reference | Main emphasis | Practical use |
| --- | --- | --- |
| MISRA C | Controlled language use and compliance process for safety- and security-conscious C | Build a project rule set, enforcement plan, deviation process, and evidence |
| BARR-C | Practical embedded C style, portability, maintainability, and defect reduction | Shape readable module, type, function, variable, and preprocessor conventions |
| SEI CERT C | Secure coding and prevention of concrete vulnerability classes | Review memory, integer, string, I/O, expression, and error-handling risks |

A project can draw from all three, but it still needs one resolved project
policy. Simply writing "we follow MISRA, CERT, and BARR-C" leaves unanswered:

- Which editions and rules apply?
- Which rules are mandatory?
- Which tools enforce them?
- How are tool findings investigated?
- How are deviations approved?
- How is third-party or generated code handled?

Do not claim MISRA compliance because a tutorial was read or one analyzer
reported no findings. Compliance requires the applicable licensed material,
defined scope, process, competent review, controlled deviations, and evidence.

### 4.3 Deviation is not deletion

Sometimes a rule does not fit a constrained implementation. A deviation should
record:

- the rule and affected scope;
- the technical reason;
- the risk created;
- compensating controls;
- reviewer and approver;
- expiry date or review trigger.

Suppressing a diagnostic without this reasoning hides information. A reviewed
deviation keeps the decision visible.

## 5. Defensive Programming

Defensive programming means enforcing the real contract at the boundary that
owns it. It does not mean adding random null checks everywhere.

Input validation is one part of defensive programming. It checks whether data
belongs to the domain an operation can safely and meaningfully process.

### 5.1 Identify trust boundaries

Untrusted or partially trusted data commonly enters through:

- protocol packets;
- files and persistent storage;
- user or command-line input;
- configuration;
- callbacks from another module;
- public APIs;
- sensors and hardware status;
- inter-process or network communication.

Before using the data, validate:

- pointer nullability;
- buffer length and capacity;
- numeric range;
- enum and state validity;
- relationships among fields;
- arithmetic overflow;
- conversion safety;
- whether the requested operation is valid in the current state.

Validation must happen before indexing, shifting, allocating, copying,
dispatching, or changing persistent state.

### 5.2 Capacity-aware parsing

This function validates every condition before reading:

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    PARSE_OK = 0,
    PARSE_INVALID_ARGUMENT,
    PARSE_SHORT_INPUT,
    PARSE_INVALID_KIND
} ParseStatus;

typedef struct {
    uint8_t kind;
    uint16_t value;
} Message;

static ParseStatus parse_message(
    const uint8_t *input,
    size_t length,
    Message *out_message)
{
    if (input == NULL || out_message == NULL) {
        return PARSE_INVALID_ARGUMENT;
    }

    if (length < 3U) {
        return PARSE_SHORT_INPUT;
    }

    if (input[0] > 2U) {
        return PARSE_INVALID_KIND;
    }

    Message parsed = {
        .kind = input[0],
        .value = (uint16_t)(
            ((uint16_t)input[1] << 8)
            | (uint16_t)input[2])
    };

    *out_message = parsed;
    return PARSE_OK;
}
```

Important design choices:

- the input pointer and length travel together;
- the output pointer is validated;
- the function rejects invalid data before committing output;
- success and failure cases use named values;
- no native structure is cast over external bytes.

Writing to a local `parsed` object first preserves the caller's output when
parsing fails.

### 5.3 Checked allocation arithmetic

This is unsafe:

```c
int *values = malloc(count * sizeof *values);
```

The multiplication can wrap before `malloc` sees the size. Check it first:

```c
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

static int *allocate_ints(size_t count)
{
    if (count == 0U || count > SIZE_MAX / sizeof(int)) {
        return NULL;
    }

    return malloc(count * sizeof(int));
}
```

In C, do not cast the result of `malloc`. Include `<stdlib.h>`, check failure,
and define one clear owner responsible for `free`.

### 5.4 Safe `realloc`

Direct assignment can lose the original allocation:

```c
items = realloc(items, new_size); /* unsafe ownership update */
```

Use a temporary pointer:

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

static bool resize_ints(int **items, size_t count)
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

This example gives a zero count an explicit release-and-null policy instead of
depending on the implementation-sensitive `realloc` zero-size case.

Nulling one pointer after `free` does not fix other aliases. Ownership design is
the real protection against use-after-free and double free.

## 6. Errors, Return Values, And Assertions

### 6.1 Check meaningful return values

If an API can report failure, ignoring its result silently changes the
program's error policy.

Bad:

```c
send_message(message, length);
```

Better:

```c
SendStatus status = send_message(message, length);
if (status != SEND_OK) {
    return status;
}
```

If ignoring a result is intentional, make the decision visible:

```c
(void)flush_best_effort_log();
```

The cast does not make the operation safe. It tells a reviewer that the result
was deliberately ignored and invites a check of the API policy.

### 6.2 Design explicit error codes

Avoid unrelated magic integers:

```c
typedef enum {
    SENSOR_OK = 0,
    SENSOR_INVALID_ARGUMENT,
    SENSOR_TIMEOUT,
    SENSOR_IO_ERROR,
    SENSOR_BAD_STATE
} SensorStatus;
```

A useful error model lets callers decide whether to retry, reject input, enter
a safe state, or report a permanent failure.

Do not partially update outputs unless the contract says so:

```c
#include <limits.h>

static SensorStatus convert_sample(
    int raw,
    unsigned int scale,
    int *out_value)
{
    if (out_value == NULL
        || scale == 0U
        || scale > (unsigned int)INT_MAX) {
        return SENSOR_INVALID_ARGUMENT;
    }

    int converted = raw / (int)scale;
    *out_value = converted;
    return SENSOR_OK;
}
```

### 6.3 Correct use of `errno`

`errno` is not a universal last-error variable. It matters only when an API's
contract says that failure sets it.

Correct pattern:

```c
#include <errno.h>
#include <stdio.h>

typedef enum {
    FILE_OPEN_OK = 0,
    FILE_OPEN_NOT_FOUND,
    FILE_OPEN_PERMISSION,
    FILE_OPEN_OTHER
} FileOpenStatus;

static FileOpenStatus open_input(
    const char *path,
    FILE **out_file)
{
    if (path == NULL || out_file == NULL) {
        return FILE_OPEN_OTHER;
    }

    FILE *file = fopen(path, "rb");
    if (file != NULL) {
        *out_file = file;
        return FILE_OPEN_OK;
    }

    int saved_errno = errno;

    if (saved_errno == ENOENT) {
        return FILE_OPEN_NOT_FOUND;
    }
    if (saved_errno == EACCES) {
        return FILE_OPEN_PERMISSION;
    }
    return FILE_OPEN_OTHER;
}
```

Key rules:

- check the function's primary failure result first;
- save `errno` before another library call;
- do not treat nonzero `errno` alone as failure;
- translate platform errors at module boundaries when callers need stable
  domain errors.

Setting `errno` to zero is useful only when a called API requires it to
distinguish a valid result from an error. `fopen` already has a clear primary
failure result, so this wrapper does not reset `errno` first.

### 6.4 `assert` versus runtime validation

Use `assert` for internal programmer invariants:

```c
#include <assert.h>
#include <stddef.h>

static int first_value_nonempty(
    const int *values,
    size_t count)
{
    assert(values != NULL);
    assert(count > 0U);
    (void)count;

    return values[0];
}
```

This function is suitable only after a trusted caller has established the
preconditions. Public or untrusted boundaries need runtime checks:

```c
static bool try_first_value(
    const int *values,
    size_t count,
    int *out_value)
{
    if (values == NULL || out_value == NULL || count == 0U) {
        return false;
    }

    *out_value = first_value_nonempty(values, count);
    return true;
}
```

Never put required side effects in an assertion:

```c
assert(initialize_device()); /* wrong */
```

When `NDEBUG` is defined, the call can disappear. Compute first:

```c
bool initialized = initialize_device();
assert(initialized);

if (!initialized) {
    return DEVICE_INIT_FAILED;
}
```

The runtime branch is still required if failure is possible in a supported
deployment.

Use `_Static_assert` for compile-time invariants:

```c
enum { COMMAND_COUNT = 4 };

_Static_assert(COMMAND_COUNT > 0,
               "The command table must not be empty");
```

## 7. Safe Strings And Secure I/O

### 7.1 A string API needs a capacity contract

A C string is a character sequence terminated by `'\0'`. Therefore a
destination capacity of `N` can hold at most `N - 1` ordinary characters.

This bounded copy reports whether the entire source fit:

```c
#include <stdbool.h>
#include <stddef.h>

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

The function always terminates a valid nonzero-capacity destination. It returns
`false` when truncation occurs. That makes truncation a caller-visible policy
decision.

### 7.2 Why `strncpy` is not a universal safe copy

`strncpy` has fixed-field semantics:

- it may leave the destination unterminated;
- it pads the remaining destination with zero bytes when the source is short.

Those behaviors can be useful for a fixed-width record format, but they do not
make `strncpy` a drop-in safe `strcpy`.

Avoid unbounded functions such as `strcpy`, `strcat`, and `sprintf` at trust
boundaries unless sufficient capacity is proven by construction. `gets` was
removed from the C standard because it cannot be used safely.

### 7.3 Check `snprintf`

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

static bool format_sensor_line(
    char *destination,
    size_t capacity,
    unsigned int sensor_id,
    int value)
{
    if (destination == NULL || capacity == 0U) {
        return false;
    }

    int required = snprintf(
        destination,
        capacity,
        "sensor=%u value=%d",
        sensor_id,
        value);

    if (required < 0) {
        destination[0] = '\0';
        return false;
    }

    return (size_t)required < capacity;
}
```

A nonnegative result equal to or larger than `capacity` means the output was
truncated. Truncation is not automatically success.

### 7.4 Bounded input

Use `fgets` with the destination capacity:

```c
char line[64];

if (fgets(line, sizeof line, stdin) == NULL) {
    /* End-of-file or input error. */
}
```

Then define what to do when a complete line did not fit. A robust parser may:

- reject the input;
- consume the rest of the line;
- report a length error;
- use a larger bounded protocol buffer when the product permits it.

For numeric input, avoid `atoi`, which cannot report useful conversion errors.
Use a checked conversion such as `strtol`, then validate:

- whether any characters were consumed;
- the end pointer;
- range and sign policy;
- the result against the destination type.

## 8. Logging

Logging is an operational interface, not decoration.

### 8.1 Define log levels

A simple policy may use:

- `ERROR`: operation failed and needs attention;
- `WARNING`: abnormal condition with recovery or degradation;
- `INFO`: important lifecycle or state event;
- `DEBUG`: development detail that may be disabled in production.

The exact names matter less than consistent meaning.

### 8.2 Use stable event identity

Human-readable text changes. Stable event IDs help automation:

```c
typedef enum {
    LOG_EVENT_SENSOR_TIMEOUT = 1001,
    LOG_EVENT_INVALID_PACKET = 1002
} LogEvent;
```

A useful record can include:

- level;
- event ID;
- module;
- bounded context;
- timestamp when available;
- relevant state or error code.

### 8.3 Do not hide required behavior in logging

Dangerous:

```c
LOG_DEBUG("status=%d", read_and_clear_status());
```

If the log compiles out, the status read may disappear. Compute first:

```c
int status = read_and_clear_status();
LOG_DEBUG("status=%d", status);
```

### 8.4 Production concerns

Define:

- maximum record size;
- truncation behavior;
- sink-full behavior;
- rate limiting;
- thread and interrupt safety;
- privacy and secret-handling rules;
- whether logging may allocate, block, or fail.

An attacker-controlled string must never become the format:

```c
printf("%s", external_text); /* format is fixed */
```

Do not log passwords, tokens, cryptographic material, or unnecessary personal
data.

## 9. Static And Dynamic Analysis

### 9.1 Compiler warnings

Start with a strict, reproducible build:

```bash
cc -std=c17 -Wall -Wextra -Wpedantic -Werror \
   -Wconversion -Wshadow -Wformat=2 \
   module.c -o module
```

This is an example baseline, not a universal command. Warning names, coverage,
and behavior vary by compiler version and target.

Important points:

- `-Wall` does not mean every warning;
- some diagnostics depend on optimization;
- `-Werror` needs a compiler-upgrade and third-party-code policy;
- broad suppressions can hide unrelated defects;
- a warning-free build is evidence, not proof.

Useful C-oriented diagnostics may include warnings about:

- implicit function declarations;
- missing or non-prototype declarations;
- format mismatches;
- signed/unsigned conversion;
- narrowing conversion;
- shadowed variables;
- incomplete `switch` handling;
- discarded qualifiers;
- suspicious allocation sizes.

Validate each option against the actual toolchain before making it mandatory.

### 9.2 Static analysis

Static analysis examines source or an intermediate representation without
needing one concrete runtime execution.

Common tools include:

- GCC `-fanalyzer`;
- Clang Static Analyzer;
- `clang-tidy`;
- `cppcheck`.

They can identify defects such as:

- null dereference paths;
- leaks and double release;
- uninitialized data;
- dead stores or unreachable logic;
- incorrect API use;
- suspicious conversions;
- bounds and lifetime problems.

Results depend on configuration. The analyzer needs the real:

- include paths;
- preprocessor definitions;
- target assumptions;
- language mode;
- generated headers;
- compile commands.

Do not enable every check and then ignore the noise. Select checks according to
project risk, triage every new finding, and document narrow suppressions.

### 9.3 Dynamic analysis

Dynamic tools observe an execution.

| Tool | Main strength | Important limitation |
| --- | --- | --- |
| AddressSanitizer (ASan) | Out-of-bounds access, use-after-free, invalid free, selected stack lifetime errors | Finds only executed behavior; instrumentation may not fit a target |
| UndefinedBehaviorSanitizer (UBSan) | Invalid shifts, signed overflow, null/misaligned access, bounds, and other selected UB | Does not cover every form of undefined behavior |
| ThreadSanitizer (TSan) | Data races in supported threaded hosted builds | High overhead, platform limits, separate build requirements |
| Valgrind Memcheck | Invalid access, uninitialized-value use, leaks, allocation misuse | Hosted-platform tool with substantial runtime overhead |

Example host sanitizer build:

```bash
cc -std=c17 -O1 -g3 \
   -Wall -Wextra -Wpedantic \
   -fsanitize=address,undefined \
   -fno-omit-frame-pointer \
   module.c -o module-sanitized

./module-sanitized
```

Use a separate TSan build where supported. Sanitizer combinations and runtime
availability vary.

### 9.4 Static analysis versus dynamic analysis

Static analysis can reason about paths that tests never execute, but it can
misunderstand configuration or produce findings that need human review.

Dynamic analysis observes real execution and often gives concrete stack traces,
but it cannot find a defect on a path that was never run.

Use both. Their disagreement is information, not inconvenience.

## 10. Unit Testing And Mocking

### 10.1 Isolate policy from hardware

Hard-to-test design:

```c
bool alarm_required(void)
{
    return read_physical_sensor_register() > 100;
}
```

The policy is tied directly to hardware access.

Testable design:

```c
#include <stdbool.h>

typedef bool (*ReadSensor)(void *context, int *out_value);

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

A host fake:

```c
typedef struct {
    int value;
    bool succeeds;
    unsigned int calls;
} FakeSensor;

static bool fake_sensor_read(
    void *context,
    int *out_value)
{
    FakeSensor *fake = context;
    ++fake->calls;

    if (!fake->succeeds) {
        return false;
    }

    *out_value = fake->value;
    return true;
}
```

The production target supplies a different `read` function. The alarm policy
can now be tested on a host without pretending to emulate physical hardware.

### 10.2 Fake, stub, and mock

| Test double | Purpose | Example |
| --- | --- | --- |
| Stub | Return a canned answer | A clock function always returns `1000` |
| Fake | Provide a lightweight working implementation | In-memory storage replacing flash |
| Mock | Verify expected interactions | Confirm a retry sends exactly three requests |

Prefer state-based tests when outputs and state prove the behavior. Strict mocks
can couple tests to implementation details and make safe refactoring painful.

### 10.3 What to test

For each unit, include:

- normal input;
- minimum and maximum valid values;
- just-outside boundary values;
- null and empty inputs where permitted;
- malformed data;
- repeated calls;
- invalid state transitions;
- allocation or I/O failure;
- timeout and retry;
- partial operation;
- cleanup after failure.

Frameworks such as Unity, CMock, FFF, and Google Test can help. They do not
replace good test cases. If a C module is tested through a C++ runner, preserve
C linkage in the public test interface.

### 10.4 Coverage is not correctness

Line coverage answers whether lines executed. It does not answer:

- whether the expected result was checked;
- whether boundary cases were meaningful;
- whether error paths were forced;
- whether concurrency was correct;
- whether requirements were complete.

Use coverage to find missing execution, then review the quality of the tests.

## 11. Documentation, Build Systems, And CI

### 11.1 Doxygen contracts

Document information that the type system cannot express:

```c
/**
 * @brief Decode one big-endian sensor sample.
 *
 * @param data      Input bytes; must point to at least data_length bytes.
 * @param data_length Number of readable bytes in data.
 * @param out_value Receives the decoded value on success; not modified on
 *                  failure.
 *
 * @return true on success; false for null pointers or short input.
 */
bool decode_sensor_sample(
    const uint8_t *data,
    size_t data_length,
    uint16_t *out_value);
```

Useful contracts include:

- units;
- valid ranges;
- nullability;
- ownership and lifetime;
- output behavior on failure;
- side effects;
- blocking behavior;
- thread, task, or interrupt context;
- reentrancy.

Do not write comments that merely repeat syntax.

### 11.2 Make and CMake

Make describes build rules directly. CMake generates native build systems and
can export `compile_commands.json` for analysis tools.

Neither tool creates quality automatically. A useful build describes:

- C language edition;
- compiler and target options;
- include paths and definitions;
- generated dependencies;
- debug and release variants;
- host-test and target variants;
- sanitizer and analysis targets;
- repeatable test commands.

### 11.3 CI basics

A practical CI pipeline may contain:

```text
1. Clean checkout
2. Strict compile
3. Unit tests
4. Static analysis
5. ASan/UBSan host tests
6. Separate TSan job when relevant
7. Coverage report
8. Documentation checks
9. Target build and selected target tests
10. Retained logs and reports
```

Fast checks usually run on every change. Expensive target tests, broad analysis,
fuzzing, or mutation testing may run on merge or schedule. A delayed check still
needs an owner when it fails.

Record relevant tool versions. A build that cannot be reproduced is weak
evidence.

## 12. Practical Industrial Workflow

Use this workflow when adding or changing a C module.

### Step 1: Define the contract

Write down:

- valid inputs;
- output behavior;
- ownership;
- capacities and units;
- failure categories;
- allowed execution contexts;
- timing or resource constraints.

### Step 2: Design test boundaries

Separate pure policy from:

- hardware access;
- time;
- randomness;
- allocation;
- file or transport I/O;
- global configuration.

### Step 3: Implement defensively

Validate external inputs before use. Preserve outputs on failure where
practical. Keep cleanup ownership explicit.

### Step 4: Compile strictly

Use the selected C edition and project warning baseline. Fix the cause rather
than casting away the symptom.

### Step 5: Analyze statically

Run the configured analyzers with the same macros and include paths as the real
build. Triage findings.

### Step 6: Test behavior and failure

Test boundaries, malformed input, fault injection, and cleanup, not only the
happy path.

### Step 7: Run dynamic tools

Exercise host-testable paths under ASan/UBSan and other relevant tools.

### Step 8: Verify target assumptions

Check target compiler behavior, memory constraints, timing, hardware
integration, and configuration variants separately.

### Step 9: Automate and retain evidence

Put repeatable checks in CI and preserve enough output to diagnose failures.

## 13. Comparisons

### 13.1 Standards and tools

| Topic | What it defines | What it does not prove |
| --- | --- | --- |
| Coding standard | Expected source and engineering process | That the implementation is defect-free |
| Compiler warnings | Suspicious constructs recognized by one compiler | Complete semantic or security analysis |
| Static analyzer | Path-sensitive or rule-based source analysis | Correct configuration or absence of false negatives |
| Unit tests | Behavior for selected isolated cases | Whole-system or target correctness |
| Sanitizers | Runtime defects on executed instrumented paths | Safety of unexecuted paths |
| Target tests | Behavior in the real environment | Complete source-path coverage |

### 13.2 `assert` versus runtime validation

| Aspect | `assert` | Runtime validation |
| --- | --- | --- |
| Main purpose | Detect programmer invariant violations | Handle expected or external invalid conditions |
| May disappear | Yes, with `NDEBUG` | No |
| Suitable for untrusted input | No | Yes |
| Recovery expected | Usually no | Depends on API policy |
| Side effects allowed | Must not be required | Ordinary evaluated code |

### 13.3 Error code versus `errno`

| Aspect | Explicit error code | `errno` |
| --- | --- | --- |
| Ownership | Part of the function's API | Library side channel when documented |
| Stability | Can be domain-specific and stable | Platform/library values |
| When to inspect | Directly after the call | Only after the primary failure result |
| Risk | Ignored or underspecified categories | Stale value or overwritten diagnostic |

### 13.4 C cleanup versus C++ RAII

| Topic | C | C++ |
| --- | --- | --- |
| Cleanup trigger | Explicit control flow | Destructor at object lifetime end |
| Ownership expression | API contract, naming, structure, review | RAII types and ownership types |
| Early return handling | Cleanup path must be written | Automatic for well-designed objects |
| Industrial rule | One owner and complete cleanup paths | Prefer Rule of Zero and RAII |

C cannot use C++ RAII directly, but it should preserve the same clarity about
who owns a resource and when it is released.

A mixed C/C++ product can wrap a C lifecycle in a C++ ownership type. Given a C
interface:

```c
#ifndef DEVICE_H
#define DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Device Device;

Device *device_create(void);
void device_destroy(Device *device);

#ifdef __cplusplus
}
#endif

#endif
```

C++ can bind release to scope:

```cpp
#include <memory>

struct DeviceDeleter {
    void operator()(Device *device) const noexcept
    {
        device_destroy(device);
    }
};

using UniqueDevice = std::unique_ptr<Device, DeviceDeleter>;

UniqueDevice make_device()
{
    return UniqueDevice(device_create());
}
```

The `extern "C"` guards preserve C linkage when the header is included by C++.
The C API still defines allocation and destruction, while the C++ wrapper adds
RAII without mixing allocation families or changing the C ABI.

## 14. Common Bugs

### 14.1 Validation bugs

- validating after reading or writing;
- checking each field but not relationships between fields;
- accepting external integers as valid enum values;
- converting negative values to `size_t`;
- overflowing allocation-size arithmetic;
- silently clamping malformed input.

### 14.2 Memory bugs

- missing allocation failure checks;
- direct `realloc` assignment;
- use-after-free through an alias;
- double free or invalid free;
- reading uninitialized memory;
- assuming nulling one pointer repairs ownership.

### 14.3 String and I/O bugs

- forgetting space for `'\0'`;
- assuming `strncpy` always terminates;
- ignoring `snprintf`, `fgets`, `fread`, or `fwrite` results;
- accepting silent truncation;
- using external input as a format string;
- using `atoi` when errors and ranges matter.

### 14.4 Error-handling bugs

- ignoring a meaningful return value;
- checking `errno` without a primary failure;
- overwriting `errno` before saving it;
- using magic integer errors;
- modifying output before failure;
- using `assert` for a recoverable condition;
- putting required work inside `assert`.

### 14.5 Tool and process bugs

- analyzing with incorrect macros or include paths;
- enabling every check and then ignoring all output;
- suppressing warnings globally;
- compiling only one configuration;
- testing only successful inputs;
- treating coverage percentage as test quality;
- accepting routinely failing CI.

## 15. Debugging And Verification

### 15.1 Reproduce first

Capture:

- exact input;
- configuration and build type;
- compiler and tool version;
- optimization level;
- target or host environment;
- failure logs and stack trace.

Reduce the problem to the smallest reproducible case. Add the reproducer as a
regression test.

### 15.2 Use the right tool

- Compiler warning: suspicious source or translation issue.
- Static analyzer: path, ownership, lifetime, or rule violation.
- ASan: bounds or lifetime corruption.
- UBSan: selected undefined behavior.
- TSan: hosted data race.
- Valgrind: hosted memory and uninitialized-value diagnostics.
- `gdb`: inspect concrete state, stack frames, variables, and memory.
- `strace`: inspect hosted user-space system-call failure.
- `ltrace`: inspect selected hosted library calls.
- `perf`: profile supported hosted performance events.

`strace`, `ltrace`, and `perf` are environment-specific user-space tools, not
core C and not substitutes for target debugging.

### 15.3 Fault injection

Deliberately force:

- allocation failure;
- short read or write;
- timeout;
- invalid checksum;
- malformed length;
- full queue;
- unavailable log sink;
- failed callback or HAL operation.

Failure paths are production paths. If they are never executed in tests, they
remain assumptions.

### 15.4 Fuzzing, mutation testing, and formal methods

These are useful awareness topics, not automatic project requirements.

**Fuzzing** repeatedly generates or mutates inputs. It is useful for parsers and
protocol boundaries when failures are detected through sanitizers, assertions,
invariants, or differential checks.

**Mutation testing** deliberately changes program behavior to see whether tests
fail. Surviving meaningful mutations suggest weak assertions or missing cases.

**Formal verification** proves selected properties from a model and explicit
assumptions. It is valuable for high-risk algorithms and protocols, but it does
not casually prove an entire product correct.

## 16. Best Practices Checklist

- Use one resolved project coding standard.
- Record rationale for high-value rules.
- Review and document deviations.
- Validate untrusted input before use.
- Make capacities, units, ownership, and ranges explicit.
- Check meaningful return values.
- Translate platform errors into stable domain errors where useful.
- Use `assert` only for internal invariants.
- Keep required side effects out of assertions and logging.
- Use bounded string and formatted-I/O operations.
- Define truncation behavior.
- Keep logs bounded, searchable, and privacy-aware.
- Compile under a validated warning baseline.
- Run configured static analysis and triage every new finding.
- Use dynamic tools according to their detection scope.
- Isolate hardware dependencies behind narrow interfaces.
- Test boundaries, failures, transitions, and cleanup.
- Keep tests deterministic.
- Avoid overspecified interaction mocks.
- Build every supported configuration.
- Automate checks and preserve useful diagnostics.

## 17. Interview Readiness

### 17.1 Beginner

**Why does a team need a coding standard?**

It creates consistent, reviewable rules and restricts practices associated with
real defects. A standard is useful only when its scope, rationale, enforcement,
and deviation process are defined.

**When should `assert` be used?**

Use it for internal invariants whose failure indicates a programming defect.
Do not use it as the only validation for external input or recoverable runtime
failure because assertions can be disabled.

**How do you avoid buffer overflow?**

Carry capacity with every buffer, validate lengths before access, reserve space
for a string terminator, check arithmetic overflow, use bounded operations, and
inspect their return values.

### 17.2 Mid-Level

**Static analysis versus dynamic analysis?**

Static analysis reasons about source paths without executing them and can find
issues on untested paths. Dynamic analysis observes concrete instrumented
executions and gives strong runtime evidence for paths that run. They have
different blind spots and should be combined.

**How do you use `errno` correctly?**

First detect failure using the function's documented primary result. Then save
`errno` immediately if that API defines it, before another library call. Do not
treat a stale nonzero value as failure.

**How do you test hardware-dependent code?**

Move hardware operations behind a narrow interface. Test policy and parsing on
the host with fakes or stubs, then use integration and target tests for real
timing, MMIO, interrupt, and hardware behavior.

### 17.3 Senior

**What evidence supports a MISRA compliance claim?**

The applicable licensed guideline, defined scope, project rule set, enforcement
and tool strategy, investigated diagnostics, documented deviations, adopted
code policy, competent review, and retained evidence. One analyzer report is
not enough.

**How would you build a risk-based CI matrix?**

Map each important risk to evidence: strict compiler builds, configuration
variants, static analysis, unit and integration tests, sanitizer jobs, target
builds/tests, and scheduled fuzz or mutation work where justified. Record tool
versions and retain diagnosable artifacts.

**How do you handle legacy findings?**

Baseline them with ownership and risk classification, prevent new findings,
fix high-risk defects first, and reduce the baseline deliberately. Do not
globally suppress the entire history and call it clean.

## 18. Practice Tasks

### Basic

1. Compile a small C module with strict warnings and fix each diagnostic without
   blanket suppression.
2. Implement and test a bounded string copy for empty, exact-fit, truncated, and
   zero-capacity cases.
3. Write one internal `assert` and one runtime check for external input.
4. Wrap a failing standard-library call and translate `errno` into a domain
   error enum.
5. Review a logger call and move required side effects outside the log argument.

### Intermediate

1. Implement a dynamic array resize function with checked multiplication,
   temporary `realloc`, and documented zero-count behavior.
2. Build a fake sensor interface and test success, timeout, invalid input, and
   repeated calls.
3. Add normal, ASan/UBSan, and static-analysis targets to a small C project.
4. Design a bounded logger with levels, event IDs, truncation policy, and
   sink-full behavior.
5. Add Doxygen contracts for ownership, capacities, units, return values, and
   failure behavior.
6. Review test coverage and add assertions for unverified error paths.

### Advanced

1. Draft a project coding-standard profile that resolves selected MISRA,
   BARR-C, CERT C, compiler, and local rules.
2. Create a deviation record containing scope, rationale, risk, compensating
   controls, approval, and review trigger.
3. Design a CI matrix for host, target, compiler, optimization, analysis, and
   sanitizer configurations.
4. Fuzz a byte-oriented parser and preserve every discovered failure as a
   regression test.
5. Run mutation testing on a validation module and strengthen tests for
   meaningful surviving mutations.
6. Review a legacy module and create a risk-ranked plan for warnings, analyzer
   findings, ownership defects, and unsafe string APIs.

## 19. Summary

- Industrial C quality comes from layered evidence, not one standard or tool.
- A coding standard needs scope, rationale, enforcement, deviations, and
  ownership.
- MISRA C, BARR-C, and CERT C overlap but are not interchangeable.
- Validate external data before dangerous operations.
- Make capacities, ownership, ranges, and failure behavior explicit in APIs.
- Use named error codes and handle `errno` only according to an API contract.
- Use `assert` for programmer invariants, not recoverable external failures.
- Bounded strings still require checked results and a truncation policy.
- Logging must be bounded, safe, privacy-aware, and independent of required
  behavior.
- Warnings, static analysis, tests, sanitizers, and target verification have
  different strengths and blind spots.
- Isolate hardware access so ordinary policy can be tested on a host.
- CI should repeat meaningful checks and retain enough evidence to diagnose
  failures.

## 20. Reference Notes

- ISO C defines language and library behavior. This chapter uses C17 as its
  practical baseline.
- MISRA C guidance and compliance material require the applicable licensed
  publications for a real compliance program.
- BARR-C provides practical embedded C coding guidance.
- SEI CERT C provides secure-coding rules and rationale.
- GCC and Clang documentation define compiler-specific warnings, analyzers, and
  sanitizer options.
- Cppcheck, Valgrind, Doxygen, Make, CMake, and test frameworks should be used
  according to their official version-specific documentation.
