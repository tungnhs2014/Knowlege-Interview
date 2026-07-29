# M03 Exercises — Advanced Pointers & Embedded Design Patterns

> **Status:** APPROVED.

This exercise set preserves the two Session 05 exercises followed by the five Session 06 exercises. The Session files define the exercise identities, source order, learning objectives, public interfaces, required behaviour, validation intent, and submission identities. This document makes only the technical corrections recorded in the approved M03 source map and lessons.

## Common Engineering Baseline

Apply the following baseline to every exercise unless its stated interface makes an item inapplicable:

- Build as C99 with `-Wall -Wextra -pedantic -Werror`.
- Supply a Makefile with `all` and `clean` targets.
- Use mandatory braces and fixed-width integer types where appropriate.
- Document the required functions and data structures with Doxygen-style comments.
- Validate pointers before dereference and indexes before table access where those operations occur.
- Do not use dynamic allocation; no current M03 source exercise requires it.
- Run `cppcheck` and `clang-tidy` when available, and resolve relevant warnings or errors.

`TOOL UNAVAILABLE` must be reported honestly. A warning-free build or clean static-analysis run is useful evidence for the selected compiler and tool configuration, but it does not prove MISRA, CERT, safety, or universal ISO C compliance. Likewise, compiler, linker, and binary-inspection observations describe the selected build and target configuration; they are not universal ISO C guarantees.

## S05-E01 — Error Message Table

### Objective

Build a checked lookup from a fixed error-code domain to human-readable text. The exercise practises an array of const string pointers, enum-to-table alignment, and invalid-index behaviour.

### Required Interface

Preserve the error-code domain:

```c
ERR_OK = 0,
ERR_TIMEOUT,
ERR_HW_FAIL,
ERR_COUNT
```

Provide a global `const char * const` lookup table and this public function:

```c
const char *get_error_string(uint8_t err_code);
```

### Requirements

- Map the declared error codes to their corresponding text through the lookup table.
- Validate `err_code` before using it as a table index.
- Return `"UNKNOWN_ERROR"` for an invalid code.
- Preserve the source's fixed-width integer, BARR-C-style, Doxygen, C99, Makefile, and static-analysis requirements.
- Do not provide a completed lookup table or function implementation in this specification.

An array of pointers supports variable-length strings and can avoid unused fixed-width row capacity. It does not always consume less memory than a two-dimensional character array: total memory depends on pointer width, string storage and sharing, alignment, compiler, and target.

### Required Validation

- Build with the common engineering baseline.
- Test `ERR_TIMEOUT`.
- Test the invalid code `99`.
- Confirm that neither valid nor invalid input indexes outside the lookup table.

### Expected Behaviour

The required output semantics are that `ERR_TIMEOUT` resolves to `TIMEOUT_ERROR` and code `99` resolves to `UNKNOWN_ERROR`.

### Submission Mapping

- Source submission identity: `Exercise_1/hw_array_of_pointers.c` and `Exercise_1/Makefile`.
- Future solution identity, not created: `session-05-exercise-01-error-message-table`.

## S05-E02 — UI Menu Dispatcher

### Objective

Build a bounded UI operation table that selects one of three compatible menu handlers through a checked function-pointer index. The exercise also introduces a controlled custom-section request and target-build inspection.

### Required Interface

Define a function-pointer typedef that takes `uint8_t` page ID and returns `void`. Provide three handlers for Main/Menu, Settings, and About, a const function-pointer array, and:

```c
void dispatch_ui(uint8_t menu_index);
```

### Requirements

- Dispatch every valid menu and one invalid menu.
- Validate the index before accessing the table.
- Validate the selected function pointer before indirect invocation.
- Ensure all handlers have exactly compatible signatures; do not cast a mismatched handler to fit the table.
- Request the named input section `.my_dispatch_table` for the table using the source-required GCC/Clang-family `__attribute__((section(...)))` extension.
- Preserve the source's C99, BARR-C-style, Doxygen, Makefile, and static-analysis requirements.

An explicit dispatch table is an architectural choice, not a universal performance improvement; a compiler may optimize a suitable dense `switch` into a jump table. `__attribute__((section(...)))` is not ISO C. It requests an input section only: `const` and the attribute do not independently guarantee Flash placement, and final placement depends on the linker script and target memory map.

### Required Validation

- Build with the common engineering baseline.
- Exercise all three valid dispatches and one invalid index.
- Inspect section evidence with `readelf -S` or `objdump -h`.
- Inspect symbol evidence with `readelf -s`, `objdump -t`, or `nm -S`.
- Do not use `nm` alone as proof of final section placement.

### Expected Behaviour

The valid tests must report Main/Menu, Settings, and About in the source-required order. The invalid index must report the source-required invalid-menu error behaviour.

### Submission Mapping

- Source submission identity: `Exercise_2/hw_jump_table.c` and `Exercise_2/Makefile`.
- Future solution identity, not created: `session-05-exercise-02-ui-menu-dispatcher`.

## S06-E01 — Timer Callback Registration System

### Objective

Implement the source-defined software-timer registration pattern: one stored callback, an explicit running state, tick-driven expiry, and controlled reset/reuse.

### Required Interface

```c
typedef void (*timer_callback_t)(void);

void Timer_Register(uint32_t expire_at_tick,
                    timer_callback_t callback);
void Timer_Tick(void);
void Timer_Reset(void);
bool Timer_IsRunning(void);
```

### Requirements

- Keep private timer state in a static structure inside the implementation file.
- Store expiry tick, current tick, callback, and running state.
- Guard the callback against `NULL` before indirect invocation.
- Fire the callback exactly once at expiry, then stop the timer.
- Reject a registration attempt while a timer is already running without overwriting the active timer.
- Allow reset to cancel active state and permit later registration.
- Preserve the source requirement that the second alarm fires at tick 3 after reset and re-registration.
- Do not imply that this callback executes in an ISR or a thread; this exercise is a software-driven timer model.

### Required Validation

Use this one consistent required test sequence:

1. Register the first alarm for tick 5.
2. Advance ticks and attempt the second registration after tick 2 but before tick 5.
3. Confirm that the active timer rejects the second registration.
4. Confirm that the first callback fires at tick 5 and the timer stops.
5. Reset the timer.
6. Register the second alarm for tick 3 and confirm that it fires at tick 3.

### Expected Behaviour

The rejected second registration occurs while the first timer is running. The source output ordering that shows this rejection after the first timer has stopped is contradictory and is not retained.

### Submission Mapping

- Source submission identity: `Exercise_1/main.c`, `Exercise_1/Makefile`, and optional headers.
- Future solution identity, not created: `session-06-exercise-01-timer-callback-registration`.

## S06-E02 — Single-Instance ADC Driver

### Objective

Implement the source-defined ADC module with private persistent state, a single-initialization policy, public getters, and controlled deinitialization/reuse.

### Required Interface

```c
bool     ADC_Init(uint32_t sample_rate_hz);
void     ADC_DeInit(void);
void     ADC_SetChannel(uint32_t channel);
uint32_t ADC_Read(void);
bool     ADC_IsInitialized(void);
uint32_t ADC_GetChannel(void);
uint32_t ADC_GetSampleRate(void);
```

### Requirements

- Use private static ADC state with initialized flag, active channel, and sample rate.
- Do not use heap allocation.
- Reject double initialization until `ADC_DeInit()` occurs.
- Guard use before initialization according to the source-defined runtime behaviour.
- Provide the specified getters and demonstrate deinitialization followed by clean reuse.
- Preserve the source-required demonstration of initialization, rejected second initialization, getters, channel selection, read behaviour, and deinitialization.
- Include the source-required commented-out private-state access as an external-module illustration, with an accurate explanation of the boundary.

This is a single-instance module with private persistent state. It is not universally the best design and is not inherently thread-safe. File-scope `static` gives internal linkage: code in a separate translation unit cannot name the private state. In the source's single-file submission, the comment illustrates the intended module boundary; a direct access placed in that same implementation file would not itself be a C compile error.

If an implementation introduces optional pointer parameters, it must guard them against `NULL`. Do not add pointer parameters merely to satisfy a generic rule.

### Required Validation

- Build with the common engineering baseline.
- Demonstrate initialization at the source test rate, rejected second initialization, getter results, channel selection, read result, deinitialization, and post-deinitialization status.
- Confirm that external consumers use the public API rather than direct private-state access.

### Expected Behaviour

The source runtime semantics include successful initialization at 44100 Hz on initial channel 0, rejection of a second initialization before deinitialization, a channel-2 read reported as 300 mV, and a deinitialized state reported as not initialized.

### Submission Mapping

- Source submission identity: `Exercise_2/main.c`, `Exercise_2/Makefile`, and optional headers.
- Future solution identity, not created: `session-06-exercise-02-single-instance-adc-driver`.

## S06-E03 — String-Driven Command Dispatcher

### Objective

Implement the source-defined mapping from received command text to compatible command handlers through a static const table.

### Required Interface

```c
typedef void (*cmd_action_t)(void);

typedef struct
{
    const char *p_command_str;
    cmd_action_t action;
} command_entry_t;

void Dispatch_Command(const char *p_received_cmd);
```

### Requirements

- Support exactly these five commands: `LED_ON`, `LED_OFF`, `MOTOR_START`, `MOTOR_STOP`, and `STATUS`.
- Use `strcmp` for exact command matching.
- Use a static const command table.
- Calculate the table count with `sizeof(table) / sizeof(table[0])` while the operand is an actual table array.
- Validate `p_received_cmd` before calling `strcmp`.
- Validate table bounds, compatible handlers, and the selected handler before invocation.
- Exercise all five known commands and one unknown command.
- Preserve the source's C99, BARR-C-style, Doxygen, Makefile, and static-analysis requirements.

`static const` expresses read-only and module intent; it does not universally guarantee Flash placement. The array-size expression is valid only where it names an actual array, not where a table has adjusted to a pointer parameter. Do not provide the completed table or handler implementations here.

### Required Validation

- Build with the common engineering baseline.
- Test each of the five named commands and one unknown command.
- Test null input before any call to `strcmp`.
- Confirm that an unknown command produces behaviour distinct from every known command.

### Expected Behaviour

Each known command produces its source-required action message. The unknown command reports the source-required unknown-command behaviour without invoking a known handler.

### Submission Mapping

- Source submission identity: `Exercise_3/main.c`, `Exercise_3/Makefile`, and optional headers.
- Future solution identity, not created: `session-06-exercise-03-command-dispatcher`.

## S06-E04 — Traffic Light Function-Pointer FSM

### Objective

Refactor the supplied traffic-light `switch` baseline into a checked function-pointer FSM while preserving the baseline's actual behaviour.

### Required Interface

Preserve `traffic_state_t` with `RED`, `GREEN`, `YELLOW`, and `NUM_STATES`; one compatible handler per state; and a static const `TrafficFSM` table. The required handler type is:

```c
typedef void (*traffic_handler_t)(
    uint32_t tick,
    traffic_state_t *p_next_state);
```

Every traffic-state handler must exactly match this signature. It receives the current tick and updates the state through `p_next_state`; incompatible handlers must not be cast into `TrafficFSM`. Preserve the dispatcher interface:

```c
void RunTrafficFSM(uint32_t tick, traffic_state_t *p_state);
```

### Requirements

- Retain the provided `switch` implementation as the behavioural authority.
- Include both the source baseline and refactored behaviour comparison.
- Use a uniform handler signature and no global state variable; pass state through `p_state`.
- Validate `p_state` before use.
- Validate state bounds before table access.
- Validate the selected handler before indirect invocation.
- Simulate ticks 1 through 10.
- Preserve the source's C99, BARR-C-style, Doxygen, Makefile, and static-analysis requirements.

### Required Validation

- Build with the common engineering baseline.
- Run the baseline and refactored FSM with the same ticks 1 through 10.
- Compare the generated behaviours; the refactored FSM must match the actual baseline transition logic.
- Verify that no invalid state indexes `TrafficFSM` and no null handler is called.

### Expected Behaviour

The source `switch` logic is authoritative. It prints RED at tick 9 and then transitions to GREEN, so tick 10 prints GREEN. Do not retain the contradictory source expected output that prints RED at tick 10. The acceptance criterion is behavioural identity with the baseline generated from the supplied transition rules.

`static const` does not independently guarantee Flash placement.

### Submission Mapping

- Source submission identity: `Exercise_4/main.c`, `Exercise_4/Makefile`, and optional headers.
- Future solution identity, not created: `session-06-exercise-04-traffic-light-fsm`.

## S06-E05 — WiFi Scalable FSM

### Objective

Implement the source-defined WiFi state model with checked function-pointer dispatch, a retry limit of three, resettable retry ownership, and deterministic invalid-state recovery.

### Required Interface

Preserve `WIFI_INIT`, `WIFI_CONNECTING`, `WIFI_CONNECTED`, `WIFI_ERROR`, `WIFI_MAX_STATES`, one compatible `wifi_handler_t` per state, and a static const `WifiFSM` table. The required handler type is:

```c
typedef uint32_t (*wifi_handler_t)(
    uint8_t input,
    WIFI_STATE *p_next_state);
```

Every WiFi state handler must exactly match this signature. `input` is the current FSM input, `p_next_state` must be validated before dereference, and incompatible function-pointer casts are forbidden. Preserve the dispatcher interface:

```c
uint32_t RunStateMachine(uint8_t input, WIFI_STATE *p_state);
```

### Return-Value Contract

Both `wifi_handler_t` handlers and `RunStateMachine()` use this complete return contract:

- `0U`: dispatch completed successfully. This includes a valid state handler processing its input whether or not the state changes.
- `0xFFU`: `p_state` is `NULL`, the current state is invalid, the selected handler is `NULL`, or dispatch cannot be performed safely.

No additional status values are permitted. When `p_state` is non-`NULL`, an invalid state must recover to `WIFI_INIT`. No table access may occur before state validation, and no handler invocation may occur when the selected handler is `NULL`. Normal retry outcomes return `0U`; remaining in `WIFI_CONNECTING` is not a `0xFFU` failure.

### Requirements

- Pass current state through `p_state`; do not use dynamic allocation.
- Validate the state before indexing `WifiFSM`.
- Validate the selected handler before indirect invocation.
- Recover deterministically from an invalid state.
- Keep retry limit at three failures.
- Use one small, module-private, explicitly resettable retry owner. It may be private module state with controlled helpers or another minimal mechanism that preserves the required public FSM interface.
- Do not require a `static` local retry counter inside `wifi_state_connecting()` when another state must reset it.
- Reset retry state on a fresh entry to CONNECTING from INIT or after a link drop, on successful connection, and when three failures move the FSM to ERROR.
- On the next provided input, transition ERROR deterministically to INIT.

### Required Validation

Use the source input sequence exactly:

```text
{0, 0, 1, 0, 0, 0, 0, 1}
```

Validate this canonical trace:

| Step | Current state + input | Required result |
| --- | --- | --- |
| 0 | `WIFI_INIT + 0` | Move to `WIFI_CONNECTING`; reset retry state. |
| 1 | `WIFI_CONNECTING + 0` | Failed attempt 1; remain `WIFI_CONNECTING`. |
| 2 | `WIFI_CONNECTING + 1` | Move to `WIFI_CONNECTED`; reset retry state. |
| 3 | `WIFI_CONNECTED + 0` | Move to `WIFI_CONNECTING`; begin a fresh retry sequence. |
| 4 | `WIFI_CONNECTING + 0` | Failed attempt 1. |
| 5 | `WIFI_CONNECTING + 0` | Failed attempt 2. |
| 6 | `WIFI_CONNECTING + 0` | Failed attempt 3; move to `WIFI_ERROR`; reset retry state. |
| 7 | `WIFI_ERROR + 1` | Move to `WIFI_INIT`. |

Do not add an extra automatic ninth step.

### Expected Behaviour

The required behaviour is the internally consistent trace above. It replaces the contradictory source combination of input sequence, retry output, and extra automatic recovery step while preserving the source state domain, retry limit, interfaces, and learning objective.

`static const` does not independently guarantee Flash placement.

### Submission Mapping

- Source submission identity: `Exercise_5/main.c`, `Exercise_5/Makefile`, and optional headers.
- Future solution identity, not created: `session-06-exercise-05-wifi-scalable-fsm`.

## Source Preservation Record

The exercise identities, source order, required public interfaces, core behaviour, test scenarios, output semantics where consistent, Makefile `all`/`clean` requirement, C99 build requirement, and documentation/static-analysis expectations are preserved. The only corrections are the qualified pointer-table memory statement, custom-section and Flash wording, timer registration ordering, traffic-light tick-10 behaviour, WiFi sequence/retry ownership, and the restriction that binary inspection must not treat `nm` alone as section-placement proof.

No solution algorithms, complete source code, alternative advanced versions, unit-test frameworks, solution directories, or interview material are included.
