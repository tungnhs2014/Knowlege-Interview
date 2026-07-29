# M03-L02 — Callbacks, Function Pointers & FSM

> **Status:** APPROVED.

## 1. Why Runtime Behaviour Selection Matters

Embedded software often needs to select behaviour after the program has been built: notify application code when a software event occurs, choose an operation from a text command, or perform a different action according to the current operating state. Function pointers make this selection explicit in C, while a finite state machine (FSM) makes the selection depend on both stored state and new input.

The value of these designs is not indirect calling by itself. Their value is a reviewable contract: which functions may be called, when registration is permitted, who owns state, how invalid input is rejected, and how the system recovers from invalid state. This lesson develops small synchronous designs that prepare the learner for Session 06 without supplying its completed programs.

This lesson does not define ISR rules, `volatile` or MMIO access, thread safety, atomics, mutexes, signals, event-loop frameworks, or unit-test frameworks. A callback does not imply any of those execution contexts.

## 2. Callback Mental Model

A callback is a compatible function supplied to one component so that the component can invoke it later when an event occurs. In C, the provider stores a function-pointer value and the consumer supplies a function with the declared signature.

```text
callback provider
    → registration
    → stored compatible function pointer
    → later event
    → validation
    → invocation
```

The provider needs a policy for three cases before writing the implementation:

1. What happens when registration receives a null function pointer?
2. May a second registration replace the first, or is it rejected while active?
3. What result tells the caller whether registration or invocation succeeded?

A `void` callback is appropriate when the provider has no useful completion result to process. A status-returning callback lets the provider record or propagate a result, but it also makes the return-value contract part of the interface. Neither choice is automatically safer; choose the smallest contract the provider needs.

### 2.1 Complete standalone C99 example — one synchronous notification callback

```c
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef int (*measurement_reporter_t)(uint32_t measurement);

static measurement_reporter_t s_reporter = NULL;

static int register_reporter(measurement_reporter_t reporter)
{
    if (reporter == NULL)
    {
        return -1;
    }

    if (s_reporter != NULL)
    {
        return -1;
    }

    s_reporter = reporter;
    return 0;
}

static int publish_measurement(uint32_t measurement)
{
    measurement_reporter_t reporter = s_reporter;

    if (reporter == NULL)
    {
        return -1;
    }

    return reporter(measurement);
}

static int print_measurement(uint32_t measurement)
{
    (void)printf("measurement: %u\n", (unsigned int)measurement);
    return 0;
}

int main(void)
{
    if (register_reporter(print_measurement) != 0)
    {
        return 1;
    }

    if (register_reporter(print_measurement) != -1)
    {
        return 1;
    }

    return (publish_measurement(25U) == 0) ? 0 : 1;
}
```

This example deliberately rejects replacement registration. A different module might allow replacement, but it must document whether replacement can occur during operation and what happens to any associated context. The example is synchronous: `publish_measurement()` invokes the stored function before it returns.

## 3. Callback Types, Registration, and Compatibility

A callback typedef defines a function type contract. Registered functions must have compatible return and parameter types. Parameter names aid documentation but do not affect type compatibility; parameter types, parameter count, and return type do.

For example, a function declared as `int report(uint16_t value)` is not compatible with `int (*)(uint32_t)`. Casting that function pointer to silence a diagnostic does not repair the mismatch. Calling through an incompatible function-pointer type can have undefined behaviour.

Use a precise typedef and let the compiler diagnose mismatches:

```c
typedef int (*measurement_reporter_t)(uint32_t measurement);
```

Do not convert function pointers to `void *`, and do not use casts to hide incompatible callback signatures. If the provider needs a different payload, define and document a different callback type.

Registration should report failure when the provider cannot accept the callback: the pointer is null, a callback is already registered under a reject policy, or the module is not in a state that accepts registration. A provider should not silently store an invalid pointer and defer the fault to a later event.

## 4. Callback Invocation, Lifetime, and Execution Context

Callback safety has four separate questions:

1. **Function-pointer validity:** is the stored pointer null, and is its target compatible with the declared type?
2. **Context or data lifetime:** if the callback uses associated data, will that data remain valid until invocation completes?
3. **Module-state lifetime:** will the provider's stored registration state remain coherent for the planned module lifetime?
4. **Execution context:** where and when does invocation occur?

The first example stores only a function pointer, so it has no separate context object. Real APIs may store a function pointer plus a caller-owned context pointer. In that case, the registration contract must say who owns the context, when it may be released, and whether deregistration is required before release.

A callback does not automatically run in an ISR, worker thread, or signal context. This lesson uses only synchronous software-driven calls. Restrictions for ISR invocation, synchronization, atomics, mutexes, and race-condition engineering require an explicit execution contract and belong to later modules.

**Common mistake:** a callback target remains valid but reads a pointer to automatic data that has already gone out of scope. Checking the function pointer for null cannot repair a dangling context or stale data reference.

## 5. Timer Callback Preparation Without a Timer Solution

S06-E01 combines one stored callback with module-owned timer state. The required conceptual sequence is:

```text
register one non-null callback while stopped
    → mark timer running
    → advance ticks
    → invoke the callback at the configured expiry, if still non-null
    → stop the timer
```

The registration guard is meaningful only while the timer is running. Therefore, the second registration attempt in the future exercise must occur **before** the first callback stops the timer. The Session 06 stated requirement and its sample output disagree on this ordering; this lesson preserves the requirement, not the contradictory output order.

The important design points are one registered callback, an explicit running/stopped state, tick-driven invocation, null validation, a stop condition, and defined failure reporting. This is not a complete timer implementation.

## 6. Module-Owned Persistent State

Some modules represent one shared software resource. A file-scope `static` state object makes that state private to one translation unit, while public functions control initialization, use, and observation. This can be simple and deterministic for a single module, but it is not a universal best practice or a thread-safety guarantee.

### 6.1 Complete standalone C99 example — private module configuration

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
    bool initialized;
    uint32_t period_ms;
} sampler_module_state_t;

static sampler_module_state_t s_sampler = { false, 0U };

static bool sampler_initialize(uint32_t period_ms)
{
    if (s_sampler.initialized || (period_ms == 0U))
    {
        return false;
    }

    s_sampler.period_ms = period_ms;
    s_sampler.initialized = true;
    return true;
}

static void sampler_deinitialize(void)
{
    s_sampler.period_ms = 0U;
    s_sampler.initialized = false;
}

static bool sampler_get_period(uint32_t *p_period_ms)
{
    if ((p_period_ms == NULL) || !s_sampler.initialized)
    {
        return false;
    }

    *p_period_ms = s_sampler.period_ms;
    return true;
}

int main(void)
{
    uint32_t period_ms = 0U;

    if (!sampler_initialize(100U) || sampler_initialize(200U))
    {
        return 1;
    }

    if (!sampler_get_period(&period_ms) || (period_ms != 100U))
    {
        return 1;
    }

    sampler_deinitialize();
    return sampler_get_period(&period_ms) ? 1 : 0;
}
```

The state has deterministic lifetime, but external code has no direct access to it. The getter has a defined pre-initialization policy: it reports failure rather than returning a value that could be mistaken for valid configuration. The initialization policy rejects repeated initialization until deinitialization occurs.

This pattern makes state ownership clear, but globally shared module state limits scalability and test isolation. It also provides no concurrency guarantee. Keep it small, private, and governed by a documented lifecycle.

## 7. Command Dispatcher Design

A string-driven command dispatcher binds command text to a compatible handler pointer. It is appropriate when a bounded command vocabulary is received as null-terminated text and the operation set should be reviewable in one table.

The runtime flow is:

```text
input string
    → validate pointer
    → search command table
    → compare exact command name
    → validate selected handler
    → invoke handler
    → return unknown-command result if no match
```

The caller must provide a readable, null-terminated string. A null check prevents passing a null pointer to `strcmp`, but it cannot prove that arbitrary memory is a valid C string. Exact matching avoids accidentally treating a prefix such as `SYNC_NOW` as `SYNC`.

### 7.1 Complete standalone C99 example — a neutral command table

```c
#include <stddef.h>
#include <string.h>

typedef int (*command_handler_t)(void);

typedef struct
{
    const char *p_name;
    command_handler_t handler;
} command_entry_t;

static int run_probe(void)
{
    return 0;
}

static int run_sync(void)
{
    return 0;
}

static const command_entry_t s_commands[] = {
    { "PROBE", run_probe },
    { "SYNC", run_sync }
};

static int dispatch_command(const char *p_received_command)
{
    size_t index;
    size_t command_count = sizeof(s_commands) / sizeof(s_commands[0]);

    if (p_received_command == NULL)
    {
        return -2;
    }

    for (index = 0U; index < command_count; ++index)
    {
        const command_entry_t *p_entry = &s_commands[index];

        if ((p_entry->p_name == NULL) || (p_entry->handler == NULL))
        {
            return -3;
        }

        if (strcmp(p_received_command, p_entry->p_name) == 0)
        {
            return p_entry->handler();
        }
    }

    return -1;
}

int main(void)
{
    if ((dispatch_command("PROBE") != 0) || (dispatch_command("SYNC_NOW") != -1))
    {
        return 1;
    }

    return (dispatch_command(NULL) == -2) ? 0 : 1;
}
```

The table stores immutable text references and compatible handlers. A project must decide how it rejects duplicate names: preferably prohibit them during table review or add explicit startup validation if the table is generated or configurable. Returning an unknown-command status is distinct from a known command whose handler reports its own operational failure.

## 8. FSM Fundamentals

An FSM records a current state and reacts to a new input or event. A handler performs an action and chooses the next state. This makes behaviour reviewable when the same input has different meaning in different operating modes.

```text
current state + input
    → handler/action
    → next state
```

- A **state** is the current mode of behaviour.
- An **input** or **event** is the new condition being processed.
- A **transition** changes the current state to a next state.
- An **action** is work performed while handling a state and input.
- An **invalid state** is a value outside the supported state model or one that has no usable handler.

A state action and a state transition are different. Logging a failed attempt is an action; moving from `CONNECTING` to `ERROR` after a limit is a transition. Keeping them conceptually separate prevents accidental multiple transitions and makes entry/exit behaviour easier to review.

## 9. Switch-Based FSM Baseline

A `switch` is often the clearest implementation for a small FSM. It is not automatically slow, and compilers may optimize a suitable `switch` into efficient branch or jump-table code. A function-pointer FSM is an architectural alternative when the state handlers and state table make extension or review clearer; it is not a universal performance improvement.

### 9.1 Complete standalone C99 example — a small switch baseline

```c
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    CONTROL_IDLE = 0,
    CONTROL_ACTIVE,
    CONTROL_STATE_COUNT
} control_state_t;

static int run_switch_step(control_state_t *p_state, uint8_t input)
{
    if (p_state == NULL)
    {
        return -1;
    }

    switch (*p_state)
    {
        case CONTROL_IDLE:
            if (input == 1U)
            {
                *p_state = CONTROL_ACTIVE;
            }
            break;

        case CONTROL_ACTIVE:
            if (input == 0U)
            {
                *p_state = CONTROL_IDLE;
            }
            break;

        default:
            *p_state = CONTROL_IDLE;
            return -1;
    }

    return 0;
}

int main(void)
{
    control_state_t state = CONTROL_IDLE;

    if ((run_switch_step(&state, 1U) != 0) || (state != CONTROL_ACTIVE))
    {
        return 1;
    }

    return ((run_switch_step(&state, 0U) == 0) && (state == CONTROL_IDLE)) ? 0 : 1;
}
```

This baseline validates the state pointer, has explicit transitions, and recovers an unsupported state to a defined safe state. It provides a behaviour reference before the same style of state logic is placed behind a function-pointer table.

## 10. Function-Pointer FSM

A function-pointer FSM uses an enum state identifier, a state-count sentinel, one uniform handler type, one handler per state, a handler table, and one central dispatcher. The central dispatcher validates the state before indexing and validates the selected handler before invoking it.

### 10.1 Complete standalone C99 example — checked state-table dispatch

```c
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    PHASE_READY = 0,
    PHASE_BUSY,
    PHASE_COUNT
} phase_t;

typedef void (*phase_handler_t)(phase_t *p_state, uint8_t input);

static void handle_ready(phase_t *p_state, uint8_t input)
{
    if ((p_state != NULL) && (input == 1U))
    {
        *p_state = PHASE_BUSY;
    }
}

static void handle_busy(phase_t *p_state, uint8_t input)
{
    if ((p_state != NULL) && (input == 0U))
    {
        *p_state = PHASE_READY;
    }
}

static phase_handler_t const s_phase_handlers[PHASE_COUNT] = {
    [PHASE_READY] = handle_ready,
    [PHASE_BUSY] = handle_busy
};

static int run_phase_machine(phase_t *p_state, uint8_t input)
{
    phase_handler_t handler;
    size_t state_index;

    if (p_state == NULL)
    {
        return -1;
    }

    if ((*p_state < PHASE_READY) || (*p_state >= PHASE_COUNT))
    {
        *p_state = PHASE_READY;
        return -1;
    }

    state_index = (size_t)*p_state;
    handler = s_phase_handlers[state_index];
    if (handler == NULL)
    {
        *p_state = PHASE_READY;
        return -1;
    }

    handler(p_state, input);
    return 0;
}

int main(void)
{
    phase_t state = PHASE_READY;

    if ((run_phase_machine(&state, 1U) != 0) || (state != PHASE_BUSY))
    {
        return 1;
    }

    state = PHASE_COUNT;
    return ((run_phase_machine(&state, 0U) == -1) && (state == PHASE_READY)) ? 0 : 1;
}
```

The handlers directly update the state through `p_state`; that is one valid transition pattern. The dispatcher validates `p_state`, state bounds, and the selected handler before any indirect call. An invalid state or a null table entry resets to `PHASE_READY` and returns failure, so it never indexes the table with invalid data.

## 11. State Tables and Safe Indexing

Direct enum indexing is safe only when values are intentionally contiguous, begin at the required base, have a count sentinel or separately verified size, and are maintained together with the table. The `PHASE_COUNT` sentinel makes the valid range visible in the example.

For sparse state values or arbitrary externally received integers, use an explicit mapping or a search. Do not blindly cast a negative or arbitrary external integer to `size_t` and index a table. First validate the external representation, then convert it into the internal state domain.

At a minimum, a central state dispatcher checks:

1. `p_state != NULL` before reading or writing it.
2. The current state is within the table's valid range before indexing.
3. The selected handler is non-null before indirect invocation.
4. Input validity when the state model defines a restricted input domain.

## 12. Transitions, Actions, Retry State, and Reset Ownership

Two patterns are valid for transitions:

1. The handler performs its action and updates the state directly through a state pointer.
2. The handler calculates or returns a next state, and the dispatcher validates and commits that state.

The first pattern is compact for a small local FSM. The second centralizes transition validation and can make entry/exit actions clearer. Either pattern needs one visible owner for retry state and its reset policy.

For a connection-style FSM, retry count is part of the FSM state, not an unrelated hidden side effect. Successful connection resets it; a fresh entry into a connection attempt may reset it; reaching the explicit limit produces a deterministic recovery transition. Do not create a design in which one handler must directly reset a `static` local variable owned by another handler.

### 12.1 Complete standalone C99 example — explicit retry-policy ownership

```c
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    RETRY_AGAIN = 0,
    RETRY_RECOVER
} retry_decision_t;

typedef struct
{
    uint8_t failed_attempts;
} retry_context_t;

static void begin_connection_attempt(retry_context_t *p_context)
{
    if (p_context != NULL)
    {
        p_context->failed_attempts = 0U;
    }
}

static retry_decision_t record_failed_attempt(retry_context_t *p_context, uint8_t retry_limit)
{
    if ((p_context == NULL) || (retry_limit == 0U))
    {
        return RETRY_RECOVER;
    }

    if (p_context->failed_attempts < retry_limit)
    {
        ++p_context->failed_attempts;
    }

    if (p_context->failed_attempts >= retry_limit)
    {
        p_context->failed_attempts = 0U;
        return RETRY_RECOVER;
    }

    return RETRY_AGAIN;
}

int main(void)
{
    retry_context_t context = { 0U };

    begin_connection_attempt(&context);
    if ((record_failed_attempt(&context, 3U) != RETRY_AGAIN) ||
        (record_failed_attempt(&context, 3U) != RETRY_AGAIN) ||
        (record_failed_attempt(&context, 3U) != RETRY_RECOVER))
    {
        return 1;
    }

    return (context.failed_attempts == 0U) ? 0 : 1;
}
```

This small context keeps the reset capability with the component that owns the counter. It is not a complete connection FSM. A future exercise may select a small explicit context or controlled module-owned helper functions, but its full sequence and output must first resolve the Session 06 source inconsistencies.

## 13. Source-Conflict Handling for Session 06

Three source issues must not be copied into canonical work:

| Source area | Canonical teaching correction |
| --- | --- |
| Timer callback registration | The second registration attempt occurs while the first timer is still running. |
| Traffic-light FSM | A function-pointer refactor preserves the behaviour of the supplied `switch` baseline, not an expected-output sequence that contradicts that transition rule. |
| WiFi FSM | The published input sequence, retry output, and extra automatic step are inconsistent. Teach one internally consistent conceptual retry policy here; define the final exercise sequence only during `exercises.md` authoring. |

## 14. Invalid-State Recovery

An FSM needs a deterministic policy for an invalid state before it uses a state value as a table index:

```text
invalid state
    → record or report the error
    → reset to a defined safe state
    → return failure
    → do not index the table until a valid state is present
```

This is safer than out-of-bounds table access, a null indirect call, or silently continuing with corrupted state. The appropriate reporting action depends on the product's error model; this lesson makes no safety-certification claim.

## 15. Common Failures and Debugging

| Failure | Symptom | Root cause | Correct engineering response |
| --- | --- | --- | --- |
| Calling an unregistered callback | Indirect-call fault or lost event. | Stored function pointer is null. | Define no-callback behaviour and check before invocation. |
| Incompatible callback signature | Cast warnings, corrupted arguments, or undefined behaviour. | Function does not match the callback typedef. | Correct the declaration; do not cast to silence the mismatch. |
| Replacing a callback without policy | Unexpected consumer receives future events. | Registration replacement was implicit. | Reject, replace, or deregister according to a documented lifecycle policy. |
| Callback context/data lifetime error | Callback runs but reads stale data. | Associated context was released or left scope too soon. | Document ownership and deregister before context release when required. |
| Assuming callback means ISR execution | Incorrect synchronization or timing assumptions. | Execution context was inferred from the word callback. | State the execution contract explicitly; use later-module rules for concurrency or ISR work. |
| Hidden shared module state | Tests or callers affect each other unexpectedly. | Private persistent state has no clear initialization lifecycle. | Define initialization, repeated-init, getter, and reset policies. |
| Null command string | Fault inside string comparison. | Null input reached `strcmp`. | Validate the input pointer before any string operation. |
| Duplicate command name | One command shadows another. | Table contains ambiguous exact matches. | Reject duplicates by review or controlled startup validation. |
| Unknown command | No operation is selected. | Input is outside the supported vocabulary. | Return a distinct unknown-command result. |
| Unchecked state index | Out-of-bounds handler lookup. | State was used as an index before range validation. | Validate state before indexing and recover deterministically. |
| Sparse enum used directly | Wrong handler or table fault. | Numeric enum values do not map directly to dense table positions. | Use explicit mapping or search. |
| Null state handler | Fault at indirect call. | Incomplete table or optional entry was invoked. | Check the selected handler and use the defined recovery path. |
| Switch and function table diverge | Refactor changes product behaviour. | Handlers were not derived from the baseline transitions. | Test and review the transition rules against the authoritative baseline. |
| Retry counter not reset | Later attempts fail too early. | Success or fresh entry did not reset retry state. | Assign reset ownership and test the reset transitions. |
| Retry owned by inaccessible static local state | Another state cannot reset it coherently. | State ownership is hidden inside one unrelated handler. | Use a small shared context or module-owned reset helper. |
| Missing invalid-state recovery | Corruption propagates or table access faults. | No defined response before dispatch. | Record/report, reset to safe state, and return failure without indexing. |

When debugging, inspect the received input, current state value, table count, selected handler, callback-registration state, and retry ownership before changing code. Adding a cast, an unchecked default handler, or a global variable usually hides the contract problem rather than solving it.

## 16. Embedded Relevance

These designs appear in software timers, synchronous driver-completion notification, protocol command handling, bootloader command dispatch, device operating modes, connection/retry logic, and UI or control state machines. The same C contracts apply in Linux user-space components that use bounded operation tables and explicit state transitions.

The examples deliberately remain synchronous. Real hardware interrupt handling, memory-mapped register access, threads, mutexes, atomic operations, and production event-loop frameworks require additional context and are excluded from this lesson.

## 17. Exercise Preparation Without Solution Leakage

- **S06-E01:** callback storage, registration guard, invocation, running/stopped timer state, and the corrected double-registration order.
- **S06-E02:** private static module state, initialization policy, getter contract, and controlled deinitialization.
- **S06-E03:** command text table, exact search, compatible handlers, null input, null handler, and unknown-command status.
- **S06-E04:** a clear `switch` baseline and an equivalent checked function-pointer FSM without copying contradictory expected output.
- **S06-E05:** a scalable state table, retry behaviour, reset ownership, invalid-state recovery, and the need to resolve the final source sequence during exercise authoring.

This lesson does not provide completed exercise sources, final exercise function names where avoidable, final expected output, exercise Makefiles, or solution-directory contents.

## 18. Embedded Engineering Checklist

Before approving a callback, command dispatcher, or small FSM design, verify:

1. Every registered or tabled function matches one precise compatible typedef.
2. Registration, replacement, null, and failure-reporting policies are explicit.
3. Stored callbacks, contexts, and module state remain valid for their documented lifetimes.
4. The execution context is stated rather than assumed from the callback pattern.
5. Command input is non-null and null-terminated by caller contract; matching is exact and duplicates are addressed.
6. FSM state values are validated before table indexing, and nullable handlers are checked before invocation.
7. State transitions, actions, retry counters, and reset ownership are separately visible.
8. Invalid state has a deterministic recovery path that does not index the table first.
9. Any `switch` versus function-table choice is justified by clarity and verified target evidence, not a universal speed claim.

## 19. Key Takeaways

- A callback is a compatible function pointer stored for a documented later invocation; it needs registration, null, lifetime, and execution-context contracts.
- Private static module state can be deterministic and simple, but it has lifecycle, sharing, scalability, and concurrency limits.
- Command tables require exact matching, checked input, handler validation, duplicate policy, and a distinct unknown-command result.
- An FSM separates current state, input, action, and transition. A `switch` remains a valid baseline.
- Function-pointer FSMs require a contiguous or explicitly mapped state domain, a count, bounds checks, null-handler checks, and invalid-state recovery.
- Retry state needs one owner and a deliberate reset policy; a static local in one handler is not shared reset infrastructure.

## 20. Further Reading

- [WG14 N1256 — C99 technical corrigenda draft](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf), especially function calls, function declarators, and compatible types.
- [SEI CERT C EXP37-C](https://wiki.sei.cmu.edu/confluence/display/c/EXP37-C.+Call+functions+with+the+correct+number+and+type+of+arguments) for the risk of calling through an incompatible function type.
- [GCC Attributes documentation](https://gcc.gnu.org/onlinedocs/gcc/Attributes.html) for compiler-extension boundaries when a callback or state table is also placed in a controlled target section.
