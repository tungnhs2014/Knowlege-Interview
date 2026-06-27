# 06 - Advanced C For Embedded

## 1. Goal

By the end of this chapter, you should be able to:

- manipulate bits with width-aware unsigned masks;
- identify undefined and implementation-defined shift behavior;
- explain memory-mapped I/O and the role of `volatile`;
- distinguish `volatile`, atomicity, ordering, and mutual exclusion;
- use `const`, `volatile`, `restrict`, `static`, and `extern` deliberately;
- replace unsafe computational macros with typed functions when possible;
- use conditional compilation without creating an untestable configuration
  maze;
- design callbacks with a function pointer and context pointer;
- build validated command dispatch tables and finite state machines;
- model encapsulation and polymorphism in C with opaque types and operation
  tables;
- design a small, fakeable hardware abstraction layer;
- encode external binary data without relying on native structure layout;
- reason about ring-buffer invariants and synchronization requirements;
- use variadic functions only with explicit type and termination contracts;
- keep asynchronous handlers minimal and defer complex work;
- compare the C techniques with modern C++ alternatives;
- debug generated code, macro expansion, register access, state transitions,
  and lifetime defects;
- answer advanced embedded C interview questions from junior through senior
  level.

This chapter uses C17 as its practical baseline. Compiler extensions and
target-specific behavior are labeled explicitly.

This is a `MUST` topic at `Deep` depth for embedded development and a `SHOULD`
topic for general software. Chapter 05, Compound Types In C, is the
prerequisite.

## 2. Why This Chapter Matters

Basic C tells you how to write expressions, functions, arrays, and structures.
Embedded C asks harder questions:

- Does this source-level write produce the required hardware transaction?
- Can this object change without ordinary program control?
- Is a shared update atomic?
- What happens if a callback runs after its context has died?
- Can an external command index outside a dispatch table?
- Is this state transition valid?
- Can the same control algorithm run in a host test without real hardware?
- Does this packet layout mean the same thing on another compiler or target?

The language cannot answer these questions alone. Correct embedded software
combines several contracts:

| Layer | Example responsibility |
| --- | --- |
| ISO C | Types, expressions, object lifetime, undefined behavior |
| Compiler | Volatile-access interpretation, extensions, generated code |
| ABI | Calling convention, alignment, representation |
| Architecture | Atomic access widths, memory ordering, barriers |
| Device | Register addresses and read/write side effects |
| Product design | Ownership, timing, state, error, and recovery policy |

A common failure is to use one keyword as a substitute for understanding all
layers. For example, `volatile` does not automatically provide:

- atomic read-modify-write;
- thread safety;
- a lock;
- a memory barrier;
- cache maintenance;
- correct peripheral semantics.

The central rule for this chapter is:

> Make every hidden contract visible: width, mask, access semantics, lifetime,
> execution context, state, ownership, byte order, and synchronization.

## 3. Mental Model: Policy, Mechanism, And Boundary

Advanced embedded C becomes easier when code is divided into three roles.

### 3.1 Policy

Policy decides what the system should do.

Examples:

- enter an alarm state after three failed samples;
- reject an invalid command;
- enable a motor only when safety conditions are true;
- drop the newest item when a queue is full.

Policy should usually be deterministic and testable without hardware.

### 3.2 Mechanism

Mechanism performs an operation.

Examples:

- read a register;
- set an output pin;
- enqueue an event;
- invoke a callback;
- encode a 16-bit value into two bytes.

Mechanism has concrete representation, timing, and target constraints.

### 3.3 Boundary

A boundary translates between domains:

```text
external bytes -> validated command -> policy
hardware status -> HAL value        -> policy
policy action   -> HAL operation     -> register access
interrupt event -> deferred event    -> normal control flow
```

Good boundaries:

- validate before indexing;
- convert representation explicitly;
- preserve qualifiers and ownership;
- isolate target-specific code;
- expose errors rather than inventing valid-looking data.

This leads to a useful architecture:

```text
+------------------------------+
| Testable application policy  |
+------------------------------+
              |
              v
+------------------------------+
| Narrow HAL / callback API    |
+------------------------------+
              |
              v
+------------------------------+
| Target-specific mechanism    |
| registers, barriers, timing  |
+------------------------------+
```

## 4. Bitwise Operations And Masks

Embedded programs frequently represent independent Boolean fields inside an
unsigned integer.

```text
bit 7                            bit 0
+---+---+---+---+---+---+---+---+
| R | R | E | M | 0 | 0 | S | P |
+---+---+---+---+---+---+---+---+

R: reserved
E: enabled
M: mode
S: status
P: pending
```

### 4.1 Operators

| Operator | Meaning |
| --- | --- |
| `&` | Bitwise AND |
| `|` | Bitwise inclusive OR |
| `^` | Bitwise exclusive OR |
| `~` | Bitwise complement |
| `<<` | Left shift |
| `>>` | Right shift |

Do not confuse bitwise operators with logical operators:

```c
flags & READY_MASK  /* tests selected bits */
ready && enabled    /* combines truth values */
```

### 4.2 Creating a mask safely

For a 32-bit value:

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static bool bit_mask_u32(unsigned int bit, uint32_t *out_mask)
{
    if (out_mask == NULL || bit >= 32U) {
        return false;
    }

    *out_mask = UINT32_C(1) << bit;
    return true;
}
```

The important details are:

- the left operand is unsigned;
- the requested bit is validated;
- the function reports failure;
- the output is written only for a valid bit.

`UINT32_C(1)` expresses a constant suitable for `uint32_t` operations. A plain
signed `1` can make high-bit shifts unsafe or misleading.

### 4.3 Set, clear, toggle, and test

```c
#include <stdbool.h>
#include <stdint.h>

static uint32_t set_bits(uint32_t value, uint32_t mask)
{
    return value | mask;
}

static uint32_t clear_bits(uint32_t value, uint32_t mask)
{
    return value & ~mask;
}

static uint32_t toggle_bits(uint32_t value, uint32_t mask)
{
    return value ^ mask;
}

static bool any_bits_set(uint32_t value, uint32_t mask)
{
    return (value & mask) != UINT32_C(0);
}

static bool all_bits_set(uint32_t value, uint32_t mask)
{
    return (value & mask) == mask;
}
```

`any_bits_set` and `all_bits_set` answer different questions. For:

```text
value = 1010
mask  = 0011
```

one selected bit is set, but not all selected bits are set.

### 4.4 Integer promotions

Operators generally apply integer promotions before performing the operation.
This matters for types narrower than `int`.

```c
#include <stdint.h>

static uint8_t invert_u8(uint8_t value)
{
    return (uint8_t)~value;
}
```

`value` is promoted before `~` is applied. The explicit conversion documents
that only the low eight bits are wanted.

Do not scatter casts merely to suppress warnings. First understand:

1. the promoted operand type;
2. the result type;
3. the intended width;
4. whether truncation is part of the design.

### 4.5 Shift rules

The shift count must be nonnegative and smaller than the width or precision of
the promoted left operand.

Unsafe:

```c
uint32_t mask = UINT32_C(1) << bit; /* unsafe if bit >= 32 */
```

Left-shifting a signed value can also produce undefined behavior. Right-shifting
a negative signed value is implementation-defined.

For bit manipulation:

- use unsigned operands;
- validate dynamic shift counts;
- use exact-width types when width is part of the interface;
- test the highest valid bit and the first invalid bit.

### 4.6 Register read-modify-write is not always safe

This familiar expression performs a read, calculation, and write:

```c
register_value |= ENABLE_MASK;
```

That is correct only if the register permits ordinary read-modify-write.
Peripheral registers can have special semantics:

- write one to clear;
- read to clear;
- write-only command bits;
- self-clearing bits;
- reserved bits that must preserve a documented value;
- separate set and clear registers.

For a write-one-to-clear status register, this may clear more flags than
intended:

```c
status_register |= ERROR_MASK; /* potentially wrong peripheral operation */
```

The device specification, not the shape of the C expression, determines the
correct operation.

## 5. Memory-Mapped I/O And `volatile`

### 5.1 Memory-mapped I/O

In a memory-mapped I/O design, certain addresses represent peripheral
registers rather than ordinary RAM.

Conceptually:

```c
#include <stdint.h>

#define TIMER_STATUS_ADDRESS UINT32_C(0x40001000)

static volatile uint32_t *const timer_status =
    (volatile uint32_t *)(uintptr_t)TIMER_STATUS_ADDRESS;
```

This is target-specific code. The address, access width, alignment, pointer
conversion, and register semantics must come from the selected target's
documentation. Prefer vendor-provided definitions or generated register
headers over handwritten addresses.

The declaration has two independent ideas:

```text
volatile uint32_t * const timer_status
         ^          ^
         |          pointer cannot be reassigned
         pointed-to register is volatile
```

### 5.2 What `volatile` means

A volatile-qualified object can be accessed in ways that are observable to the
implementation and can change outside ordinary program analysis.

Typical uses include:

- memory-mapped hardware registers;
- a narrow communication object used by a standard C signal handler;
- implementation-defined low-level interfaces.

`volatile` tells the compiler that accesses matter. It does not describe the
whole hardware transaction.

### 5.3 What `volatile` does not mean

`volatile` does not guarantee:

- atomic access;
- indivisible read-modify-write;
- mutual exclusion;
- inter-thread synchronization;
- ordering of non-volatile memory;
- a CPU or device memory barrier;
- cache coherence;
- a particular instruction sequence on every implementation.

This code still has a data race when ordinary threads access it concurrently:

```c
static volatile unsigned int counter;

void increment(void)
{
    ++counter; /* read, add, write: not an atomic increment */
}
```

### 5.4 `volatile` versus atomic

Suppose two execution contexts update a counter.

```text
Context A: read 7, add 1, write 8
Context B: read 7, add 1, write 8

Expected after two increments: 9
Observed:                     8
```

Declaring the counter `volatile` does not repair the lost update. Depending on
the environment, use:

- C atomics;
- a critical section;
- interrupt masking with a carefully bounded region;
- an RTOS synchronization primitive;
- an architecture-specific atomic operation.

The correct choice depends on who shares the object and the timing contract.

### 5.5 Barriers are separate

A volatile access is not a general barrier for ordinary memory.

```c
payload = prepared_value;
doorbell_register = START_MASK;
```

If the device requires payload memory to be globally visible before the
doorbell write, the architecture may require a documented barrier or cache
operation. Use only the target/compiler primitive specified for that system.

Do not invent a portable barrier by:

- adding more `volatile`;
- inserting an empty loop;
- reading the value back without a documented reason;
- using inline assembly copied from another architecture.

### 5.6 Register wrappers

Keep register semantics behind functions whose names describe the operation.

```c
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    volatile uint32_t control;
    volatile const uint32_t status;
} TimerRegisters;

enum {
    TIMER_CONTROL_ENABLE = UINT32_C(1) << 0,
    TIMER_STATUS_EXPIRED = UINT32_C(1) << 0
};

static void timer_enable(TimerRegisters *registers)
{
    registers->control |= TIMER_CONTROL_ENABLE;
}

static bool timer_has_expired(const TimerRegisters *registers)
{
    return (registers->status & TIMER_STATUS_EXPIRED) != 0U;
}
```

This model is suitable only for ordinary read/write semantics in a fictional
register block. A real device may require different accessors.

`volatile const` on `status` means software reads through this declaration but
does not write through it; the value can still change externally.

## 6. `const`, `restrict`, `static`, And `extern`

These keywords describe different contracts. They are not interchangeable.

### 6.1 `const`

`const` prevents modification through a particular access path.

```c
#include <stddef.h>
#include <stdint.h>

uint32_t checksum(const uint8_t *data, size_t length);
```

The function promises not to modify elements through `data`.

`const` does not automatically mean:

- the object is stored in ROM;
- no other alias can modify the object;
- the object cannot change;
- an access is atomic.

### 6.2 Pointer qualification

```c
#include <stdint.h>

extern uint8_t buffer[];
extern const uint8_t table[];

const uint8_t *p1;       /* pointer to const uint8_t */
uint8_t *const p2 = buffer; /* const pointer to uint8_t */
const uint8_t *const p3 = table;
```

Read declarations from the identifier outward:

- `p1` can point elsewhere, but cannot modify through that path;
- `p2` cannot point elsewhere, but can modify the pointed-to bytes;
- `p3` can do neither.

### 6.3 `restrict`

`restrict` is a promise made for pointer-based access during an execution of a
block. It allows optimization based on the claim that the relevant object is
accessed through the designated restricted pointer association.

```c
#include <stddef.h>

static void add_samples(
    float *restrict destination,
    const float *restrict left,
    const float *restrict right,
    size_t count)
{
    for (size_t index = 0; index < count; ++index) {
        destination[index] = left[index] + right[index];
    }
}
```

The caller must satisfy the non-aliasing contract. Calling with overlapping
ranges that violate the contract causes undefined behavior.

`restrict`:

- is not a runtime overlap check;
- should not be added merely to make a loop faster;
- belongs only on interfaces whose callers can maintain the promise;
- needs tests and documentation even though tests cannot prove all calls valid.

### 6.4 File-scope `static`

At file scope, `static` gives an object or function internal linkage:

```c
static unsigned int retry_count;

static void reset_retry_count(void)
{
    retry_count = 0U;
}
```

This limits the name to one translation unit and helps define module
boundaries. It does not make the object thread-safe.

### 6.5 Block-scope `static`

Inside a function, a static object retains its value for the program lifetime:

```c
unsigned int next_sequence(void)
{
    static unsigned int sequence;
    return sequence++;
}
```

This is simple but creates hidden shared state. Ask:

- Is the function reentrant?
- Can an interrupt or another thread call it?
- Can tests reset the state?
- What happens on unsigned wrap?

An explicit state object is often easier to test:

```c
typedef struct {
    unsigned int sequence;
} Sequencer;

static unsigned int sequencer_next(Sequencer *sequencer)
{
    return sequencer->sequence++;
}
```

### 6.6 `extern`

Use `extern` in a header to declare an object defined in exactly one source
file.

```c
/* system_status.h */
#ifndef SYSTEM_STATUS_H
#define SYSTEM_STATUS_H

extern unsigned int system_error_count;

#endif
```

```c
/* system_status.c */
#include "system_status.h"

unsigned int system_error_count;
```

Prefer functions over broadly writable exported objects. A function can
validate access, preserve invariants, and later change the representation.

## 7. Preprocessor And Macro Design

### 7.1 The preprocessor is not the type system

Preprocessing happens before ordinary C semantic analysis. Macro replacement
works on preprocessing tokens and can produce code with surprising meaning.

Useful preprocessor jobs include:

- including headers;
- include guards;
- selecting target implementations;
- rejecting unsupported configurations;
- source-location capture;
- controlled code generation.

### 7.2 Include guards

Portable header:

```c
#ifndef SENSOR_HAL_H
#define SENSOR_HAL_H

int sensor_hal_read(int *out_value);

#endif
```

`#pragma once` is widely supported but is not ISO C. Use it only when the
project's compiler policy permits it.

### 7.3 Macro precedence

Unsafe:

```c
#define SQUARE_BAD(x) x * x
```

`SQUARE_BAD(2 + 3)` expands into:

```c
2 + 3 * 2 + 3
```

Parenthesizing improves precedence:

```c
#define SQUARE_LESS_BAD(x) ((x) * (x))
```

It does not fix duplicate evaluation:

```c
SQUARE_LESS_BAD(index++)
```

This expands the side effect twice and can have undefined behavior.

Prefer a function:

```c
static inline int square_int(int value)
{
    return value * value;
}
```

A function provides:

- type checking;
- normal scope;
- one evaluation of each argument;
- ordinary debugging;
- an address when required.

Whether a compiler actually inlines it is an optimization decision.

### 7.4 Statement-like macros

If a statement macro is genuinely needed, make it behave as one statement:

```c
#define CLEAR_AND_COUNT(value, count) \
    do {                              \
        (value) = 0;                  \
        ++(count);                    \
    } while (0)
```

This form works predictably in:

```c
if (needs_reset) {
    CLEAR_AND_COUNT(value, resets);
} else {
    continue_work();
}
```

The arguments can still have macro-related hazards. A function remains better
when it can express the same operation.

### 7.5 Disabled logging must preserve behavior

Dangerous:

```c
#ifdef ENABLE_LOG
#define LOG_VALUE(expression) log_value(expression)
#else
#define LOG_VALUE(expression)
#endif
```

If code calls:

```c
LOG_VALUE(read_and_clear_status());
```

the release build does not call `read_and_clear_status` at all. Logging should
not own required side effects.

Compute first:

```c
int status = read_and_clear_status();
LOG_VALUE(status);
```

Then define a warning-conscious no-op:

```c
#ifdef ENABLE_LOG
#define LOG_VALUE(value) log_value(value)
#else
#define LOG_VALUE(value) ((void)(value))
#endif
```

### 7.6 Conditional compilation

Keep configuration checks centralized:

```c
#if defined(TARGET_ALPHA) && defined(TARGET_BETA)
#error "Select exactly one target"
#endif

#if !defined(TARGET_ALPHA) && !defined(TARGET_BETA)
#error "No supported target selected"
#endif
```

Prefer selecting whole implementations:

```text
sensor_hal_alpha.c
sensor_hal_beta.c
```

over scattering `#if` through every function. Excessive conditional compilation
creates many programs hidden in one source tree, and each configuration needs
its own build and test coverage.

### 7.7 Compile-time assertions

C11 and later provide `_Static_assert`:

```c
#include <limits.h>

_Static_assert(CHAR_BIT == 8,
               "This protocol implementation requires 8-bit bytes");
```

Assert facts that are actual interface requirements. Do not assert one observed
ABI layout and then call it portable. `uint32_t`, when provided by `<stdint.h>`,
is already an exact-width 32-bit unsigned type. Multiplying its `sizeof` by
`8` would incorrectly assume that every C byte has eight bits; use `CHAR_BIT`
when byte width is the real requirement.

### 7.8 Carefully constrained generic macros

C11 `_Generic` can select a type-specific function while preserving a compact
call site:

```c
static inline int clamp_int(int value, int low, int high)
{
    return value < low ? low : (value > high ? high : value);
}

static inline unsigned int clamp_uint(
    unsigned int value,
    unsigned int low,
    unsigned int high)
{
    return value < low ? low : (value > high ? high : value);
}

#define CLAMP(value, low, high)                                      \
    _Generic((value),                                                \
             int: clamp_int,                                         \
             unsigned int: clamp_uint)((value), (low), (high))
```

The controlling expression of `_Generic` is not evaluated, so `value` is
evaluated only by the selected function call. Keep the pattern narrow:

- list every supported type explicitly;
- route behavior through typed `static inline` functions;
- document conversions required for `low` and `high`;
- let unsupported types fail during compilation;
- prefer one ordinary typed function when generic dispatch adds no value.

Unlike C++ templates, `_Generic` selects among existing C expressions. It does
not generate an implementation for arbitrary types.

## 8. Callbacks In C

### 8.1 Function pointer

A callback separates when an operation occurs from what operation runs.

```c
typedef void (*SampleCallback)(int sample);
```

A function pointer carries behavior but no instance state.

### 8.2 Function pointer plus context

The standard C idiom for stateful callbacks is:

```c
typedef void (*SampleCallback)(void *context, int sample);

typedef struct {
    SampleCallback function;
    void *context;
} SampleListener;
```

Invocation:

```c
static void sample_listener_notify(
    const SampleListener *listener,
    int sample)
{
    if (listener != NULL && listener->function != NULL) {
        listener->function(listener->context, sample);
    }
}
```

Usage:

```c
#include <stdio.h>

typedef struct {
    const char *name;
    unsigned int count;
} SampleStats;

static void record_sample(void *context, int sample)
{
    SampleStats *stats = context;
    ++stats->count;
    printf("%s sample=%d count=%u\n",
           stats->name,
           sample,
           stats->count);
}
```

The callback contract must answer:

- Is `context` borrowed or owned?
- How long must it remain alive?
- Can it be null?
- Is invocation synchronous or deferred?
- Can invocation occur in an interrupt?
- Can the callback register or unregister callbacks?
- Can invocation overlap?

### 8.3 Lifetime failure

Unsafe:

```c
static SampleListener make_listener(void)
{
    SampleStats local = {"temporary", 0U};
    SampleListener listener = {record_sample, &local};
    return listener; /* context points to a dead object */
}
```

The function pointer remains valid, but the context pointer dangles.

### 8.4 Registration and teardown

A stored callback needs a shutdown rule:

```c
static void listener_clear(SampleListener *listener)
{
    listener->function = NULL;
    listener->context = NULL;
}
```

Clearing fields is not enough if another execution context can invoke the old
callback concurrently. Registration, invocation, cancellation, and teardown
must share a synchronization and lifetime design.

### 8.5 Execution context

A callback safe in the main loop may be unsafe in an interrupt context.

Potentially unsafe callback operations include:

- allocation;
- formatted I/O;
- blocking;
- locking;
- long computation;
- calling non-reentrant code;
- accessing shared data without synchronization.

Name or document context-specific APIs clearly:

```c
void timer_set_deferred_callback(SampleListener listener);
```

Avoid silently changing a callback from synchronous to asynchronous behavior.

## 9. Dispatch Tables

A dispatch table maps a validated identifier to behavior.

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef bool (*CommandHandler)(const uint8_t *payload, size_t length);

static bool handle_ping(const uint8_t *payload, size_t length)
{
    return payload != NULL || length == 0U;
}

static bool handle_reset(const uint8_t *payload, size_t length)
{
    (void)payload;
    return length == 0U;
}

static const CommandHandler handlers[] = {
    handle_ping,
    handle_reset
};

static bool dispatch_command(
    unsigned int command,
    const uint8_t *payload,
    size_t length)
{
    const size_t count = sizeof handlers / sizeof handlers[0];

    if ((size_t)command >= count || handlers[command] == NULL) {
        return false;
    }

    return handlers[command](payload, length);
}
```

The validation must happen before indexing. An enum improves naming but does
not prove that an external integer is a valid enumerator.

### 9.1 Table versus `switch`

A table is useful when:

- handlers have one uniform signature;
- identifiers form a compact validated range;
- the mapping is mostly data;
- table generation reduces real duplication.

A `switch` is useful when:

- cases need different validation;
- identifiers are sparse;
- control flow is clearer than indirect calls;
- compiler diagnostics for enumerators are valuable;
- code-size or safety analysis favors explicit branches.

Do not use a table merely because it looks more advanced.

## 10. Finite State Machines

An FSM defines:

- a finite set of states;
- a finite set of events;
- valid transitions;
- actions;
- behavior for invalid combinations.

### 10.1 Switch-based FSM

```c
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    STATE_IDLE,
    STATE_RUNNING,
    STATE_FAULT
} State;

typedef enum {
    EVENT_START,
    EVENT_STOP,
    EVENT_FAILURE,
    EVENT_RESET
} Event;

static bool transition(State current, Event event, State *next)
{
    if (next == NULL) {
        return false;
    }

    switch (current) {
    case STATE_IDLE:
        if (event == EVENT_START) {
            *next = STATE_RUNNING;
            return true;
        }
        break;

    case STATE_RUNNING:
        if (event == EVENT_STOP) {
            *next = STATE_IDLE;
            return true;
        }
        if (event == EVENT_FAILURE) {
            *next = STATE_FAULT;
            return true;
        }
        break;

    case STATE_FAULT:
        if (event == EVENT_RESET) {
            *next = STATE_IDLE;
            return true;
        }
        break;

    default:
        break;
    }

    return false;
}
```

This function is pure policy: no register access, logging, or allocation. It is
easy to test exhaustively.

### 10.2 Table-driven FSM

```c
typedef struct {
    State current;
    Event event;
    State next;
} Transition;

static const Transition transitions[] = {
    {STATE_IDLE,    EVENT_START,   STATE_RUNNING},
    {STATE_RUNNING, EVENT_STOP,    STATE_IDLE},
    {STATE_RUNNING, EVENT_FAILURE, STATE_FAULT},
    {STATE_FAULT,   EVENT_RESET,   STATE_IDLE}
};
```

Search the table and return failure when no row matches. A two-dimensional
table indexed directly by enum values can be faster, but it requires:

- validated state and event values;
- a defined invalid entry representation;
- compile-time dimension checks;
- deliberate code-size and data-size tradeoffs.

### 10.3 State and action ordering

If transitions perform actions, define the order:

1. validate event;
2. run current-state exit action;
3. update state;
4. run transition action;
5. run next-state entry action.

Another order can be valid, but ambiguity is not.

### 10.4 Asynchronous events

An FSM does not become thread-safe because its states are enums. If events come
from interrupts, tasks, or threads, separately define:

- event queue ownership;
- queue overflow behavior;
- synchronization;
- whether events can be coalesced;
- ordering and priority;
- shutdown behavior.

## 11. OOP-Style C

C does not have classes, constructors, destructors, access control, or virtual
functions. It can still model related ideas through conventions.

### 11.1 Opaque type

Public header:

```c
#ifndef COUNTER_H
#define COUNTER_H

#include <stdbool.h>

typedef struct Counter Counter;

bool counter_init(Counter *counter, unsigned int limit);
bool counter_increment(Counter *counter);
unsigned int counter_value(const Counter *counter);

#endif
```

An incomplete type hides representation, but callers cannot allocate it by
value because its size is unknown. Common lifecycle designs are:

- caller provides raw storage through a documented factory API;
- module allocates and returns a pointer;
- public fixed-size handle contains private storage with strict checks;
- object is created statically inside the module.

For simple no-allocation embedded code, a public structure can sometimes be
clearer than forcing opacity. Encapsulation should solve a real maintenance
problem.

### 11.2 Operation table

```c
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    bool (*read)(void *context, int *out_value);
    bool (*configure)(void *context, unsigned int rate_hz);
} SensorOps;

typedef struct {
    const SensorOps *ops;
    void *context;
} Sensor;

static bool sensor_read(const Sensor *sensor, int *out_value)
{
    if (sensor == NULL
        || sensor->ops == NULL
        || sensor->ops->read == NULL
        || out_value == NULL) {
        return false;
    }

    return sensor->ops->read(sensor->context, out_value);
}
```

This resembles dynamic dispatch:

- `ops` selects behavior;
- `context` selects instance state;
- wrapper functions validate the interface.

### 11.3 Operation-table risks

- incompatible function signature;
- null required operation;
- dead context object;
- writable table corrupted at runtime;
- version mismatch between caller and implementation;
- oversized interface with unrelated responsibilities;
- unclear ownership and teardown.

Use `const` operation tables where possible:

```c
static const SensorOps fake_sensor_ops = {
    .read = fake_sensor_read,
    .configure = fake_sensor_configure
};
```

### 11.4 No automatic lifetime

In C, lifecycle is explicit:

```c
bool device_init(Device *device);
void device_deinit(Device *device);
```

Every successful initialization path needs a corresponding cleanup policy. C
does not call cleanup automatically when a scope ends.

## 12. Hardware Abstraction Layer Design

A HAL separates application intent from target mechanism.

Bad application code:

```c
*(volatile uint32_t *)UINT32_C(0x40020014) |= UINT32_C(1) << 5;
```

Problems:

- physical address leaks into policy;
- pin meaning is hidden;
- register semantics are assumed;
- host testing is difficult;
- target migration touches application logic.

Narrow interface:

```c
#include <stdbool.h>

typedef struct {
    bool (*set_output)(void *context, unsigned int channel, bool active);
    bool (*read_input)(void *context, unsigned int channel, bool *out_active);
} DigitalIoOps;

typedef struct {
    const DigitalIoOps *ops;
    void *context;
} DigitalIo;
```

Application policy:

```c
static bool update_alarm(
    const DigitalIo *io,
    bool alarm_required)
{
    if (io == NULL
        || io->ops == NULL
        || io->ops->set_output == NULL) {
        return false;
    }

    return io->ops->set_output(io->context, 2U, alarm_required);
}
```

A host test can provide fake operations that record calls. Target code can
provide operations that access documented registers.

### 12.1 HAL design rules

- Model capabilities, not raw registers, unless raw access is the true API.
- Keep interfaces small.
- Make units explicit: hertz, milliseconds, bytes, channels.
- Define valid ranges.
- Define synchronous versus asynchronous behavior.
- Define timeout and error behavior.
- Define ownership and lifetime.
- Avoid returning a magic value that could also be valid data.
- Keep device-specific register semantics inside the target implementation.

### 12.2 Do not over-abstract

A HAL with hundreds of function pointers can be harder to understand than two
target source files. Add indirection when it provides:

- multiple implementations;
- host testing;
- stable module boundaries;
- product variants;
- controlled dependency direction.

## 13. Endianness And External Representation

External data is a sequence of bytes with a specified format. It is not a
native C structure.

### 13.1 Decode explicitly

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static bool decode_u16_be(
    const uint8_t *bytes,
    size_t length,
    uint16_t *out_value)
{
    if (bytes == NULL || out_value == NULL || length < 2U) {
        return false;
    }

    *out_value = (uint16_t)(
        ((uint16_t)bytes[0] << 8)
        | (uint16_t)bytes[1]);
    return true;
}
```

The conversions make width and shift behavior visible.

### 13.2 Encode explicitly

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static bool encode_u16_be(
    uint16_t value,
    uint8_t *bytes,
    size_t capacity)
{
    if (bytes == NULL || capacity < 2U) {
        return false;
    }

    bytes[0] = (uint8_t)(value >> 8);
    bytes[1] = (uint8_t)(value & UINT16_C(0x00ff));
    return true;
}
```

### 13.3 Bit-fields are not portable register maps

A bit-field names subfields inside an implementation-chosen storage layout:

```c
struct StatusBits {
    unsigned int ready : 1;
    unsigned int mode : 2;
    unsigned int error : 1;
};
```

This can be useful for compact implementation-local state, but the declaration
is not a portable description of wire bytes or hardware register bits.
Important details are implementation-defined or otherwise
implementation-dependent:

- allocation order within a storage unit;
- whether a field can cross a storage-unit boundary;
- structure size, padding, and alignment;
- supported base types and the signedness of a plain `int` bit-field;
- access behavior for `volatile` bit-fields under the compiler and target ABI.

A bit-field has no address, so code cannot take `&status.ready`. Updating one
field may also compile to a read-modify-write of its storage unit. That is
dangerous for registers with write-one-to-clear bits, concurrent writers, or
other read/write side effects.

For protocols and most register interfaces, explicit masks make positions and
operations reviewable:

```c
#include <stdint.h>

#define STATUS_READY_MASK (UINT32_C(1) << 0)
#define STATUS_MODE_SHIFT 1U
#define STATUS_MODE_MASK (UINT32_C(3) << STATUS_MODE_SHIFT)

static uint32_t status_with_mode(uint32_t status, unsigned int mode)
{
    uint32_t encoded = ((uint32_t)mode << STATUS_MODE_SHIFT)
                     & STATUS_MODE_MASK;
    return (status & ~STATUS_MODE_MASK) | encoded;
}
```

Validate `mode` at the API boundary when truncation would hide an error. Use
bit-fields only when layout is intentionally local or when a specific
compiler/ABI contract is documented, tested, and accepted as a portability
constraint.

### 13.4 Why packed structures do not solve serialization

A packed structure can still depend on:

- compiler syntax;
- member representation;
- byte order;
- bit-field allocation;
- enum size;
- unaligned-access support;
- ABI details.

Unsafe design:

```c
const PacketHeader *header = (const PacketHeader *)bytes;
```

The buffer may be too short, misaligned, or encoded differently. Explicit
decoding validates length and representation.

## 14. Ring Buffers

A ring buffer reuses fixed storage by wrapping indices.

```text
storage: [A][B][C][ ][ ]
          ^       ^
         tail    head
```

Required design decisions:

- element type;
- capacity;
- head/tail meaning;
- full versus empty representation;
- overwrite, reject, or drop policy;
- producer and consumer execution contexts;
- synchronization;
- ownership of stored elements.

### 14.1 Single-threaded ring buffer

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { RING_CAPACITY = 8 };

typedef struct {
    uint8_t data[RING_CAPACITY];
    size_t head;
    size_t tail;
    size_t count;
} ByteRing;

static bool byte_ring_push(ByteRing *ring, uint8_t value)
{
    if (ring == NULL || ring->count == RING_CAPACITY) {
        return false;
    }

    ring->data[ring->head] = value;
    ring->head = (ring->head + 1U) % RING_CAPACITY;
    ++ring->count;
    return true;
}

static bool byte_ring_pop(ByteRing *ring, uint8_t *out_value)
{
    if (ring == NULL || out_value == NULL || ring->count == 0U) {
        return false;
    }

    *out_value = ring->data[ring->tail];
    ring->tail = (ring->tail + 1U) % RING_CAPACITY;
    --ring->count;
    return true;
}
```

This is not thread-safe or interrupt-safe. `volatile` fields would not make the
compound operations atomic.

### 14.2 Synchronization comes from the execution model

For one interrupt producer and one main-loop consumer, a specialized design may
be possible, but correctness depends on:

- index access atomicity on the target;
- ordering between element writes and index publication;
- compiler and architecture rules;
- whether nested interrupts exist;
- overflow policy.

Use a reviewed target-specific design, atomics, or critical sections rather
than adding `volatile` and hoping.

### 14.3 Intrusive linked-list awareness

An intrusive list stores its link inside the owning object instead of
allocating a separate node:

```c
typedef struct ListNode {
    struct ListNode *next;
} ListNode;

typedef struct {
    int priority;
    ListNode pending_link;
} WorkItem;
```

This avoids per-node allocation and can make memory use deterministic. The
trade-off is a tighter lifetime and ownership contract: the list points inside
each `WorkItem`, so that object must stay alive and at a stable address while
linked.

Production rules include:

- initialize each link before first use;
- do not insert the same link into two lists at once;
- unlink an object before its storage expires or moves;
- define whether removal during iteration is allowed;
- keep list mutation inside the synchronization model;
- avoid non-portable owner-recovery macros unless the project has a reviewed,
  documented implementation.

Intrusive lists are useful infrastructure awareness, not a default replacement
for a fixed array, queue, or ring buffer.

## 15. Variadic Functions

Variadic functions accept a variable number of arguments:

```c
int log_values(const char *label, size_t count, ...);
```

They use `<stdarg.h>`:

```c
#include <stdarg.h>
#include <stddef.h>

static int sum_ints(size_t count, ...)
{
    va_list arguments;
    va_start(arguments, count);

    int sum = 0;
    for (size_t index = 0; index < count; ++index) {
        sum += va_arg(arguments, int);
    }

    va_end(arguments);
    return sum;
}
```

### 15.1 Type contract

The callee does not discover argument types automatically. The protocol must
provide them through:

- a format string;
- a count and one known promoted type;
- tags;
- a sentinel;
- another explicit schema.

Default argument promotions matter:

- `float` is passed as `double`;
- integer types narrower than `int` are promoted to `int` or `unsigned int`.

Reading with the wrong `va_arg` type is undefined behavior.

### 15.2 Prefer typed designs

Instead of:

```c
configure(device, OPTION_RATE, 1000, OPTION_MODE, 2, OPTION_END);
```

prefer:

```c
typedef struct {
    unsigned int rate_hz;
    unsigned int mode;
} DeviceConfig;

bool device_configure(Device *device, const DeviceConfig *config);
```

The structure is type-checked, reviewable, extensible through versioning, and
easy to test.

## 16. Asynchronous Handlers: Capture And Defer

Hosted C signals and embedded interrupts are different mechanisms. They share
one important design lesson:

> An asynchronous handler can interrupt code at an inconvenient point, so keep
> it minimal and defer complex work.

### 16.1 Standard C signal pattern

```c
#include <signal.h>

static volatile sig_atomic_t stop_requested;

static void handle_stop(int signal_number)
{
    (void)signal_number;
    stop_requested = 1;
}
```

Normal control flow observes the flag and performs cleanup outside the handler.

Do not assume a standard signal handler may safely:

- call `printf`;
- allocate or free memory;
- lock a mutex;
- use arbitrary library functions;
- modify arbitrary shared objects.

Exact permitted behavior depends on the C version and implementation. POSIX
defines additional asynchronous-signal-safe operations, but those are not the
strictly portable ISO C set.

### 16.2 Embedded interrupt context

For an interrupt service routine, consult:

- target architecture documentation;
- compiler interrupt-function rules;
- device peripheral semantics;
- RTOS interrupt-safe API rules;
- project timing requirements.

Typical design:

```text
interrupt:
    capture minimum status
    acknowledge source correctly
    publish or enqueue event
    return

normal context:
    validate event
    update FSM
    log, allocate, or perform longer work
```

A Boolean flag can lose repeated events. If event count matters, use a counter
or queue with a synchronization design appropriate to the target.

## 17. Practical Integrated Design

Consider a temperature monitor:

1. A target layer reads a sensor.
2. A HAL exposes `read_temperature`.
3. Policy validates the sample.
4. An FSM moves among `NORMAL`, `WARNING`, and `FAULT`.
5. A callback reports state changes.
6. A command dispatcher accepts configuration updates.
7. Protocol functions decode multi-byte values explicitly.

### 17.1 Policy object

```c
typedef enum {
    MONITOR_NORMAL,
    MONITOR_WARNING,
    MONITOR_FAULT
} MonitorState;

typedef struct {
    MonitorState state;
    int warning_threshold;
    unsigned int failed_reads;
    SampleListener state_listener;
} Monitor;
```

### 17.2 Step function

```c
static bool monitor_step(Monitor *monitor, bool read_ok, int temperature)
{
    if (monitor == NULL) {
        return false;
    }

    MonitorState previous = monitor->state;

    if (!read_ok) {
        ++monitor->failed_reads;
        if (monitor->failed_reads >= 3U) {
            monitor->state = MONITOR_FAULT;
        }
    } else {
        monitor->failed_reads = 0U;
        monitor->state =
            temperature >= monitor->warning_threshold
                ? MONITOR_WARNING
                : MONITOR_NORMAL;
    }

    if (monitor->state != previous) {
        sample_listener_notify(
            &monitor->state_listener,
            (int)monitor->state);
    }

    return true;
}
```

This function contains policy, not physical register access. Tests can cover:

- threshold boundary;
- three consecutive failures;
- successful recovery;
- callback only on state change;
- null monitor;
- callback with and without a registered function.

The HAL and target layers can be tested separately.

## 18. C And C++ Comparisons

### 18.1 `volatile` versus atomic

| Topic | `volatile` | Atomic |
| --- | --- | --- |
| Primary purpose | Observable special accesses | Atomic operations and synchronization |
| MMIO | Common, with compiler/target rules | Usually not a substitute for device register access |
| Atomic increment | No | Yes, with the correct atomic operation |
| Orders ordinary memory | Not generally | According to atomic memory-order rules |
| Removes need for locks | No | Only for operations/designs correctly expressed atomically |

### 18.2 Macro versus inline function versus `constexpr`

| Topic | C macro | C `static inline` | C++ `constexpr` function |
| --- | --- | --- | --- |
| Type checking | No | Yes | Yes |
| Argument evaluation | Can repeat | Once per argument | Normal function semantics |
| Scope | Preprocessor name | C scope/linkage | C++ scope/overload rules |
| Compile-time use | Token replacement | Sometimes optimized, not necessarily constant expression | Can participate in constant evaluation |
| Best use | Configuration, token/source generation | Typed small operation | Typed compile-time/runtime computation |

### 18.3 Function pointer versus lambda and `std::function`

| Topic | C callback pair | C++ lambda/template | `std::function` |
| --- | --- | --- | --- |
| State | `void *context` | Captures | Stored callable state |
| Type safety | Explicit casts/context discipline | Strongly typed captures | Signature checked, type erased |
| ABI boundary | Good for C ABI | C++ only | C++ library ABI |
| Allocation | None required by idiom | Often none | May allocate depending on callable/implementation |
| Timing visibility | Explicit indirect call | Often optimizable through templates | Indirect type-erased call |

Use the smallest mechanism that meets lifetime, ABI, code-size, and timing
requirements.

### 18.4 FSM in C versus State Pattern in C++

| Topic | C switch/table FSM | C++ State Pattern |
| --- | --- | --- |
| State representation | Enum and data | Objects/types |
| Dispatch | Switch or table | Usually virtual or variant-based |
| Allocation | Not required | Not inherently required, but designs vary |
| Visibility | Transition map can be centralized | Behavior can be distributed by state class |
| Best fit | Small explicit deterministic FSM | Large state-specific behavior needing encapsulation |

Start with explicit states and transitions. Use a pattern only when it makes
change and review easier.

### 18.5 OOP in C versus OOP in C++

| Topic | C | C++ |
| --- | --- | --- |
| Encapsulation | Opaque type and module API | Access control |
| Lifecycle | Explicit init/deinit | Constructors, destructors, RAII |
| Dynamic dispatch | Ops table plus context | Virtual functions or variants |
| Inheritance | Convention/composition | Language-supported inheritance |
| Failure cleanup | Manual control flow | RAII can automate scope cleanup |
| ABI | Suitable for stable C boundaries | C++ ABI requires tighter toolchain agreement |

### 18.6 Packed record versus explicit encoding

| Packed/native record | Explicit byte encoding |
| --- | --- |
| Compiler and ABI dependent | Format controls every byte |
| May be unaligned | Works from byte storage |
| Does not solve endianness | Endianness handled explicitly |
| Convenient for a tightly controlled target contract | Preferred for protocols and persistence |

## 19. Common Bugs

### 19.1 Bit and integer bugs

- using signed operands for masks and shifts;
- allowing a shift count outside the valid range;
- forgetting integer promotions around `~`;
- testing any bit when all bits are required;
- using logical operators instead of bitwise operators;
- modifying reserved or write-one-to-clear bits accidentally.

### 19.2 Register and qualifier bugs

- omitting `volatile` from a target-defined MMIO access;
- casting away `volatile`;
- treating `volatile` as synchronization;
- assuming access width or ordering without compiler/target evidence;
- copying a register structure from a different device revision;
- using read-modify-write on a register that forbids it.

### 19.3 Macro and configuration bugs

- missing parentheses;
- duplicate evaluation;
- statement macro breaking `if`/`else`;
- macro names colliding with other headers;
- release logging removing required side effects;
- different translation units using inconsistent feature definitions;
- unbalanced packing pragmas;
- untested build configurations.

### 19.4 Callback and lifetime bugs

- null callback invocation;
- incompatible function pointer cast;
- dangling context;
- callback after unregister or object destruction;
- unexpected reentrancy;
- interrupt-context callback calling blocking code;
- concurrent registration and invocation without synchronization.

### 19.5 FSM and dispatch bugs

- indexing before validation;
- assuming every enum value is named;
- missing invalid transition behavior;
- updating state and action data inconsistently;
- recursive event delivery causing uncontrolled reentrancy;
- queue overflow silently losing safety-relevant events.

### 19.6 HAL and OOP-style C bugs

- leaking target addresses into policy;
- partial operation table;
- unclear object ownership;
- forgetting deinitialization;
- changing ops-table layout without version control;
- giant HAL interfaces that are impossible to fake meaningfully.

### 19.7 Representation and buffer bugs

- casting external bytes to a structure;
- assuming bit-field order;
- ignoring byte order;
- unaligned packed-member access;
- ring-buffer full/empty ambiguity;
- `volatile` ring indices used as fake synchronization;
- incorrect `va_arg` type;
- missing `va_end`.

## 20. Debugging And Verification

### 20.1 Start with strict diagnostics

For hosted C17 examples with GCC or Clang:

```bash
cc -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
   -Wformat=2 -Werror source.c -o program
```

Useful compiler-specific diagnostics may include:

```text
-Wcast-qual
-Wmissing-prototypes
-Wstrict-prototypes
-Wswitch-enum
-Wshift-count-negative
-Wshift-count-overflow
-Wsign-conversion
-Wundef
```

Verify each warning in the selected compiler version before adding it to a
project-wide build.

### 20.2 Inspect preprocessed output

```bash
cc -std=c17 -E source.c > source.i
```

Use this when:

- a macro expands unexpectedly;
- the wrong target implementation is selected;
- an include guard collides;
- configuration differs between translation units.

To inspect predefined macros with GCC or Clang:

```bash
cc -dM -E - < /dev/null
```

### 20.3 Use sanitizers for host-testable code

```bash
cc -std=c17 -g -O1 -fno-omit-frame-pointer \
   -fsanitize=address,undefined source.c -o program
./program
```

UBSan can detect executed invalid shifts, signed overflow, misalignment, and
other undefined behavior. ASan can find bounds and lifetime defects.

Sanitizers generally do not model physical MMIO or prove target timing.

### 20.4 Inspect generated code

Generate assembly:

```bash
cc -std=c17 -O2 -S target_access.c -o target_access.s
```

Or disassemble an object:

```bash
objdump -d target_access.o
```

Use generated code to verify a hypothesis under one exact compiler, version,
flags, and target. It is evidence for that build, not a universal language
guarantee.

### 20.5 Test state spaces

For a small FSM, test every state/event pair:

```text
for each possible state:
    for each possible event:
        verify accepted/rejected
        verify next state
        verify action count and order
```

For masks, test:

- zero mask;
- lowest bit;
- highest valid bit;
- all bits;
- first invalid bit index;
- values with unrelated bits set.

For callbacks, test:

- no callback;
- valid callback and context;
- repeated invocation;
- unregister;
- attempted invocation after unregister;
- reentrant behavior if permitted;
- context lifetime.

### 20.6 Fake hardware

Record operations in a fake:

```c
#include <stdbool.h>

typedef struct {
    unsigned int last_channel;
    bool last_active;
    unsigned int calls;
} FakeDigitalIo;

static bool fake_set_output(
    void *context,
    unsigned int channel,
    bool active)
{
    FakeDigitalIo *fake = context;
    fake->last_channel = channel;
    fake->last_active = active;
    ++fake->calls;
    return true;
}
```

This tests application behavior without pretending that host memory emulates a
real peripheral.

### 20.7 Target debugging

After isolating host-testable policy, target tools may include:

- watchpoints;
- trace;
- register views;
- logic analyzers;
- oscilloscopes;
- bus analyzers;
- timing pins;
- fault-injection hooks.

Always account for debugger side effects. Reading a register in a debugger can
trigger read-to-clear behavior on some devices.

## 21. Best Practices

### Bitwise and MMIO

- Use unsigned, width-aware masks.
- Validate dynamic bit indices.
- Name fields and operations, not magic numbers.
- Derive registers from authoritative target information.
- Keep MMIO in a narrow target layer.
- Model special register semantics with distinct functions.
- Treat `volatile`, atomicity, ordering, and synchronization separately.

### Interfaces and modules

- Use `const` to express read-only access paths.
- Use `restrict` only when callers can uphold the aliasing promise.
- Use file-scope `static` to keep implementation details private.
- Prefer functions over writable `extern` objects.
- Keep ownership, lifetime, nullability, and execution context explicit.

### Macros and configuration

- Prefer typed `static inline` functions for computation.
- Keep macro arguments free of side effects.
- Use statement macros only when necessary and wrap them safely.
- Keep configuration centralized.
- Reject invalid combinations with `#error`.
- Build and test every supported configuration.

### Callbacks, FSMs, and HALs

- Pair callbacks with explicit context.
- Define registration and teardown behavior.
- Validate table indices and required operations.
- Make fixed operation tables `const`.
- Keep transition policy separate from hardware actions.
- Test all small FSM transitions.
- Design HALs around capabilities and testability.
- Avoid abstraction that adds indirection without reducing change cost.

### Representation and asynchronous code

- Encode and decode external bytes explicitly.
- Do not use packed native structures as a default protocol format.
- Keep asynchronous handlers minimal.
- Defer logging, allocation, cleanup, and long work.
- Choose synchronization from the real execution model.
- Prefer typed structures and arrays over variadic APIs.

## 22. Interview Readiness

### 22.1 Beginner questions

**How do you set, clear, toggle, and test a bit?**

Use unsigned masks:

```c
value |= mask;          /* set */
value &= ~mask;         /* clear */
value ^= mask;          /* toggle */
set = (value & mask) != 0U;
```

Also explain width, integer promotions, and valid shift counts.

**What does `volatile` do?**

It tells the implementation that accesses to a qualified object are observable
and can be affected outside ordinary program analysis. It is commonly used for
MMIO. It does not make compound operations atomic or thread-safe.

**What is the difference between `static` and `extern`?**

At file scope, `static` gives internal linkage. `extern` declares an entity
defined elsewhere. Block-scope `static` instead gives retained static storage
duration.

### 22.2 Mid-level questions

**Why prefer `static inline` over a function-like macro?**

It provides type checking, scope, single argument evaluation, and normal
debugging. A macro is still appropriate for preprocessing-only jobs such as
conditional compilation or source-location capture.

**How do you implement a stateful callback in C?**

Store a typed function pointer plus a `void *context`. Define context lifetime,
ownership, invocation context, reentrancy, registration, and teardown.

**How do you make a HAL testable?**

Keep policy independent of physical addresses, expose a narrow interface, and
provide a fake implementation that records inputs and returns controlled
outputs.

**Why is a packed structure not a portable packet parser?**

Packing does not solve byte order, representation, enum width, bit-field order,
alignment safety, or buffer-length validation.

### 22.3 Senior questions

**How would you review a register access?**

Separate:

1. ISO C type and expression behavior;
2. compiler volatile and extension behavior;
3. ABI alignment and access width;
4. architecture atomicity, ordering, and barriers;
5. device register side effects;
6. product concurrency and error policy.

Then verify the exact generated code and target behavior where required.

**How would you design polymorphism in C?**

Use a typed immutable operation table plus instance context, wrapper functions
that validate required operations, explicit lifecycle, clear ownership, and a
versioning strategy if the ABI crosses independently updated components.

**When would you choose an FSM table over a switch?**

Choose a table for uniform, data-driven, validated transitions. Choose a switch
when case-specific validation and explicit control flow are clearer. Evaluate
code/data size, indirect-call policy, testability, and review requirements.

**Is `volatile` enough for an interrupt-to-main ring buffer?**

No. Correctness also needs atomicity and ordering for element publication and
index updates, plus an overflow policy. The solution depends on the target,
compiler, architecture, nesting model, and synchronization design.

## 23. Practice Tasks

### Basic

1. Implement checked `uint32_t` functions to create, set, clear, toggle, and
   test a bit. Test bit indices `0`, `31`, and `32`.
2. Write a macro and a `static inline` function for minimum. Call each with
   side-effecting arguments and explain why the macro is unsafe.
3. Create a header/source pair that demonstrates an include guard, one `extern`
   declaration, one definition, and file-private functions.
4. Write a `_Static_assert` for a table length and one for a required integer
   width.
5. Implement two-type `_Generic` dispatch through typed `static inline`
   functions and verify that an unsupported type fails during compilation.

### Intermediate

1. Implement callback registration with a function pointer, context pointer,
   notification, and unregister operation. Document lifetime.
2. Implement a command dispatcher with sparse external command IDs. Compare a
   lookup table and a `switch`.
3. Build and exhaustively test the `IDLE`/`RUNNING`/`FAULT` FSM.
4. Encode and decode 16-bit and 32-bit big-endian values.
5. Implement a single-threaded fixed-capacity ring buffer with an explicit
   reject-on-full policy.
6. Implement a fake sensor HAL and test application policy through it.
7. Replace a bit-field-based protocol header with masks, shifts, and explicit
   byte encoding.

### Advanced

1. Model a fictional register with ordinary control bits and write-one-to-clear
   status bits. Provide separate safe APIs and explain why one generic update
   function is wrong.
2. Create two HAL implementations selected at build time. Reject missing or
   conflicting targets with preprocessing checks.
3. Design a versioned operation table for a component shared across separately
   built modules.
4. Add an event queue between an interrupt-like producer and an FSM. Document
   the required atomicity, ordering, overflow, and execution assumptions.
5. Compare generated assembly for a volatile register accessor at `-O0` and
   `-O2`. Record only claims supported by the exact compiler and target.
6. Replace a variadic configuration function with a typed configuration
   structure and compare error detection and maintainability.
7. Specify the ownership, membership, iterator invalidation, and synchronization
   contract for an intrusive work-queue list.

## 24. Summary

- Advanced embedded C is the practice of making low-level contracts explicit.
- Use unsigned, width-aware masks and validate shift counts.
- Register semantics come from the device specification, not from bitwise
  syntax.
- `volatile` supports observable special accesses but is not atomic,
  thread-safe, or a general memory barrier.
- `const`, `restrict`, `static`, and `extern` express different access,
  aliasing, storage, and linkage contracts.
- Prefer typed functions to computational macros.
- Use `_Generic` sparingly to select typed functions, not to hide unsafe macro
  evaluation.
- Keep conditional compilation centralized and test every supported build.
- Stateful C callbacks use a function pointer plus context pointer and require
  a lifetime and execution-context contract.
- Validate values before dispatch-table or FSM-table indexing.
- OOP-style C uses opaque types, explicit lifecycle, and operation tables, but
  provides no automatic RAII.
- A narrow HAL keeps target mechanism separate from testable policy.
- Encode external data explicitly instead of casting bytes to native records.
- Treat bit-field layout as implementation-dependent; prefer explicit masks for
  protocols and most register interfaces.
- Ring-buffer correctness requires invariants and an execution-model-specific
  synchronization design.
- Use intrusive lists only with explicit lifetime, membership, iteration, and
  synchronization rules.
- Variadic functions trade away type checking and should be replaced by typed
  APIs when possible.
- Asynchronous handlers should capture minimal information and defer complex
  work.
- Compiler output and target tests validate one concrete build; they do not
  redefine portable C.

## 25. Reference Notes

- ISO/IEC 9899:2018 (C17) is the practical language baseline for this chapter.
- Exact MMIO, barrier, interrupt, packing, section, and calling-convention
  behavior requires the selected architecture, device, ABI, and compiler
  documentation.
- MISRA C, BARR-C, and SEI CERT C provide safety, portability, and secure-coding
  guidance. A real compliance claim requires the applicable licensed material,
  project process, tooling, evidence, and deviation handling.
- GCC and Clang documentation should be consulted for compiler-specific
  volatile behavior, preprocessing details, warnings, extensions, generated
  code, and sanitizer support.
