# 06 - Advanced C For Embedded: Interview Pack

## How To Use This Pack

For each question:

1. Give the **Short answer** first.
2. Expand with the **Deep explanation**.
3. Ground the answer using the **C/C++ code/API anchor**.
4. Demonstrate engineering judgment through the **Production/debug angle**.
5. Avoid the listed **Common traps**.
6. Use the **Follow-up questions** to test the limits of the design.

The practical baseline is C17. Compiler extensions and target-specific behavior
are labeled rather than presented as portable C.

The recurring embedded contracts are:

```text
bit operation    width + unsigned representation + valid shift
MMIO             compiler + architecture + device semantics
callback         signature + context + lifetime + execution context
FSM              valid state + event + transition + action order
HAL              capability + ownership + error + target boundary
external bytes   bounds + width + byte order + encoding
shared state     atomicity + ordering + synchronization
```

## Beginner Questions

### 1. How do you set, clear, toggle, and test bits safely?

**Short answer**

Use unsigned, width-aware masks. Set with `|`, clear with `& ~`, toggle with
`^`, and test with `&`. Validate any runtime bit index before shifting.

**Deep explanation**

For a 32-bit value, a single-bit mask can be created with:

```c
UINT32_C(1) << bit
```

but only when `bit < 32`. A shift count outside the promoted operand width is
undefined behavior. A signed left operand can also make high-bit shifts unsafe.

Testing any selected bit differs from testing all selected bits:

```c
(value & mask) != 0U  /* any selected bit */
(value & mask) == mask /* all selected bits */
```

Integer promotions also matter when applying `~` to types narrower than `int`.

**C/C++ code/API anchor**

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static uint32_t set_bits(uint32_t value, uint32_t mask)
{
    return value | mask;
}

static uint32_t clear_bits(uint32_t value, uint32_t mask)
{
    return value & ~mask;
}

static bool bit_mask_u32(unsigned int bit, uint32_t *out)
{
    if (out == NULL || bit >= 32U) {
        return false;
    }

    *out = UINT32_C(1) << bit;
    return true;
}
```

C++ can use the same operators with fixed-width unsigned types. A scoped enum
can improve field naming, but conversions and mask operators must still be
designed deliberately.

**Production/debug angle**

Test bit `0`, the highest valid bit, and the first invalid bit. Enable shift and
conversion diagnostics, and run UBSan on host-testable code. For registers,
confirm that read-modify-write is permitted by the device specification.

**Common traps**

- Using `1 << bit` without considering signedness.
- Accepting `bit == 32` for a 32-bit operand.
- Confusing logical `&&` with bitwise `&`.
- Treating `value & mask` as an all-bits test.
- Applying ordinary read-modify-write to a special-function register.

**Follow-up questions**

- What happens when `bit` equals the operand width?
- Why can `~` surprise you with `uint8_t`?
- How would you test that all bits in a mask are clear?
- Why might a peripheral provide separate set and clear registers?

### 2. What does `volatile` mean, and is it thread-safe?

**Short answer**

`volatile` makes accesses to a qualified object observable according to the
implementation. It is commonly used for MMIO. It does not make operations
atomic, thread-safe, mutually exclusive, or ordered like a synchronization
primitive.

**Deep explanation**

A compiler normally optimizes ordinary memory accesses based on the abstract
machine. A volatile-qualified access tells it that the access itself matters
and the object can change outside ordinary analysis.

That solves a different problem from concurrency. For:

```c
++counter;
```

the implementation can perform a read, calculation, and write. Two execution
contexts can both read the same old value and lose an update, even if `counter`
is volatile.

`volatile` also does not generally order ordinary memory around an MMIO access
or create a hardware barrier.

**C/C++ code/API anchor**

```c
static volatile unsigned int status_register;

unsigned int read_status(void)
{
    return status_register;
}
```

This is not a safe concurrent counter:

```c
static volatile unsigned int counter;

void increment(void)
{
    ++counter;
}
```

C11 `_Atomic` and C++ `std::atomic` address atomic operations and memory
ordering. MMIO remains governed by compiler and target documentation.

**Production/debug angle**

Identify every writer and reader. Ask whether the problem is MMIO,
interrupt communication, or thread synchronization. Inspect generated code for
the exact toolchain, then use race-detection or target-specific tests for the
actual sharing model.

**Common traps**

- Saying volatile values are always read directly from physical memory.
- Saying volatile makes an increment atomic.
- Using volatile as a lock or memory barrier.
- Applying C++ atomic rules to an MMIO register.
- Casting away volatile to satisfy an API.

**Follow-up questions**

- When is `volatile const` useful?
- What does an atomic type provide that volatile does not?
- Can an atomic type automatically replace a device register declaration?
- What other documentation is needed for MMIO ordering?

### 3. Explain `const`, `restrict`, `static`, and `extern`.

**Short answer**

`const` restricts modification through an access path. `restrict` is a
non-aliasing promise for pointer-based access. File-scope `static` gives
internal linkage; block-scope `static` gives retained storage duration.
`extern` declares an entity defined elsewhere.

**Deep explanation**

These keywords describe different contracts:

- `const` does not guarantee ROM placement or immutability through every alias.
- `restrict` enables optimization only when callers uphold its access
  association; violating the promise is undefined behavior.
- file-scope `static` keeps a name private to one translation unit.
- block-scope `static` creates hidden retained state, which can harm
  reentrancy and test isolation.
- `extern` normally belongs in a header, while exactly one source file supplies
  the definition.

**C/C++ code/API anchor**

```c
#include <stddef.h>

static unsigned int module_errors;
extern unsigned int system_mode;

void add_samples(
    float *restrict destination,
    const float *restrict left,
    const float *restrict right,
    size_t count);
```

In C++, `const` also participates heavily in member functions, overloads, and
constant evaluation. C++ has no direct general-purpose `restrict` keyword in
the standard language.

**Production/debug angle**

Use file-private functions and objects to enforce module ownership. Review
every `restrict` addition against real callers. Search for writable exported
objects and block-static state that can be accessed reentrantly.

**Common traps**

- Saying `const` means the object cannot change.
- Adding `restrict` as a harmless performance hint.
- Saying every `static` object has internal linkage.
- Defining a writable object in a header.
- Believing internal linkage makes shared state thread-safe.

**Follow-up questions**

- What is the difference between `const uint8_t *` and `uint8_t *const`?
- Can another pointer modify an object seen through a const pointer?
- What happens if restricted ranges overlap?
- How would you replace hidden block-static state?

### 4. Why prefer a `static inline` function to a function-like macro?

**Short answer**

A function provides type checking, scope, normal argument evaluation, and
debuggability. A function-like macro can change precedence and evaluate an
argument multiple times.

**Deep explanation**

Parenthesizing macro parameters repairs some precedence bugs but does not
repair duplicate evaluation:

```c
#define SQUARE(x) ((x) * (x))
```

`SQUARE(index++)` expands the side effect twice and can have undefined
behavior. A function evaluates its argument according to normal function-call
rules.

Macros are not inherently faster. Optimizing compilers can inline functions,
while macro expansion can increase code size.

Macros remain appropriate for preprocessing-only work such as conditional
compilation, token generation, or source-location capture.

**C/C++ code/API anchor**

```c
static inline int square_int(int value)
{
    return value * value;
}
```

Statement macros that are unavoidable should use a controlled single-statement
form:

```c
#define RESET_VALUE(value) \
    do {                   \
        (value) = 0;       \
    } while (0)
```

C++ generally prefers inline functions, templates, and `constexpr` functions.

**Production/debug angle**

Inspect preprocessed output with `cc -E`. Search macro call sites for `++`,
`--`, assignments, function calls, and volatile reads. Compare generated code
before claiming a performance difference.

**Common traps**

- Saying parentheses make every macro safe.
- Including a trailing semicolon in a statement macro.
- Disabling a logging macro in a way that removes required side effects.
- Calling macros type-safe because one call compiled.
- Treating `#pragma once` as portable ISO C.

**Follow-up questions**

- When is a macro still the right tool?
- Why use `do { ... } while (0)`?
- How can a disabled assertion change behavior?
- How does C++ `constexpr` differ from replacement text?

### 5. How do you implement a stateful callback in C?

**Short answer**

Store a typed function pointer together with a `void *context`. The callback
receives the context on invocation. The API must define context lifetime,
ownership, execution context, reentrancy, and unregister behavior.

**Deep explanation**

A plain function pointer represents behavior but cannot carry per-instance
state. The context pointer supplies that state without global variables.

The design is only correct while:

- the function has the exact expected signature;
- the context points to a live object of the expected type;
- registration remains valid;
- the invocation context permits the callback's operations;
- teardown cannot race with invocation.

**C/C++ code/API anchor**

```c
typedef void (*SampleCallback)(void *context, int sample);

typedef struct {
    SampleCallback function;
    void *context;
} SampleListener;

static void notify(const SampleListener *listener, int sample)
{
    if (listener != NULL && listener->function != NULL) {
        listener->function(listener->context, sample);
    }
}
```

C++ can use a lambda or functor for state. `std::function` adds type erasure
and may add indirect-call, storage, or allocation costs.

**Production/debug angle**

Test null callback, valid invocation, unregister, invocation after unregister,
reentrancy, and dead context. Log registration identity outside interrupt
context. Use ASan for host-side stale-context defects.

**Common traps**

- Returning a callback whose context points to a local object.
- Casting an incompatible function pointer.
- Assuming a valid function pointer proves context lifetime.
- Calling blocking or allocating code from an interrupt callback.
- Clearing registration without synchronizing concurrent invocation.

**Follow-up questions**

- Who owns the context?
- Can the callback unregister itself?
- How would you support multiple listeners?
- How would a C++ capturing lambda cross a C ABI?

## Mid-Level Questions

### 6. How would you design access to a memory-mapped register?

**Short answer**

Start from the device and compiler contracts, preserve volatile qualification,
use the required width and alignment, and hide the physical access behind a
narrow operation whose name reflects the register semantics.

**Deep explanation**

A register address does not come from C. Correct access depends on:

- the device address and field definitions;
- legal read and write widths;
- alignment;
- ordinary versus special side effects;
- compiler interpretation of volatile;
- architecture ordering and barrier requirements.

The application should not spread physical addresses and masks throughout
policy code. Vendor definitions or generated headers are usually preferable to
handwritten register maps.

**C/C++ code/API anchor**

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    volatile uint32_t control;
    volatile const uint32_t status;
} TimerRegisters;

enum {
    TIMER_ENABLE = UINT32_C(1) << 0,
    TIMER_EXPIRED = UINT32_C(1) << 0
};

static void timer_enable(TimerRegisters *registers)
{
    registers->control |= TIMER_ENABLE;
}

static bool timer_expired(const TimerRegisters *registers)
{
    return (registers->status & TIMER_EXPIRED) != 0U;
}
```

This fictional model assumes ordinary control-register read-modify-write. Real
accessors must follow the selected device.

**Production/debug angle**

Review the device manual, compiler manual, and generated assembly. Test on the
target with watchpoints or trace where suitable. Remember that debugger reads
can trigger read-to-clear behavior.

**Common traps**

- Hard-coding addresses in application logic.
- Assuming a C structure always matches a register map.
- Assuming volatile specifies bus width or ordering.
- Performing read-modify-write without checking field semantics.
- Using host sanitizers as proof of MMIO correctness.

**Follow-up questions**

- Why might a status field be `volatile const`?
- When would a compiler or architecture barrier be needed?
- How would you host-test the policy using this timer?
- What if the control register is write-only?

### 7. Why is read-modify-write dangerous for write-one-to-clear registers?

**Short answer**

Read-modify-write writes back every bit observed as one. On a write-one-to-clear
register, that can clear unrelated pending flags. Write only the intended clear
mask according to the device contract.

**Deep explanation**

Consider a status register:

```text
bit 0: RX pending, write 1 to clear
bit 1: TX pending, write 1 to clear
```

If both are set and software wants to clear only RX:

```c
status |= RX_PENDING;
```

the read obtains both bits as one, OR preserves both, and the write sends ones
for both fields. Both events may be cleared.

The same general concern applies to read-to-clear, self-clearing, command, and
reserved fields.

**C/C++ code/API anchor**

```c
enum {
    STATUS_RX_PENDING = UINT32_C(1) << 0,
    STATUS_TX_PENDING = UINT32_C(1) << 1
};

static void status_clear_rx(volatile uint32_t *status)
{
    *status = STATUS_RX_PENDING;
}
```

This code is correct only for the stated fictional write-one-to-clear contract.

**Production/debug angle**

Name operations after semantics: `clear_pending`, `issue_command`, or
`read_snapshot`, not generic `update_register`. Inject simultaneous events in
target tests and inspect bus transactions where tooling permits.

**Common traps**

- Using `|=` to clear a write-one-to-clear bit.
- Assuming a register read is side-effect free.
- Preserving reserved bits by blindly writing the read value back.
- Reusing one generic bit helper for every peripheral register.

**Follow-up questions**

- Why might separate SET and CLEAR registers exist?
- How do read-to-clear fields change debugging?
- Can a critical section repair wrong register semantics?
- What should an API do with reserved bits?

### 8. Design a callback registration API with safe teardown.

**Short answer**

Store function plus context, define whether they are borrowed, serialize
registration/invocation/unregister when they can overlap, and guarantee that no
callback can run after its context lifetime ends.

**Deep explanation**

The hard part is not assignment; it is lifecycle:

1. registration publishes function and context;
2. invocation obtains a consistent pair;
3. unregister prevents new invocations;
4. in-flight invocation completes or is cancelled;
5. only then may context storage be destroyed.

In a single-threaded synchronous design, this can be simple. In interrupt or
threaded designs, it needs a critical section, atomic protocol, reference
management, or another environment-specific mechanism.

**C/C++ code/API anchor**

```c
typedef void (*EventCallback)(void *context, unsigned int event);

typedef struct {
    EventCallback callback;
    void *context;
} EventSubscription;

static void subscription_clear(EventSubscription *subscription)
{
    subscription->callback = NULL;
    subscription->context = NULL;
}
```

This clearing function alone is not a concurrent teardown protocol.

C++ can capture shared or weak ownership in a lambda, but that introduces its
own lifetime, allocation, and cycle risks.

**Production/debug angle**

Write a state diagram for registration and teardown. Stress unregister against
invocation. Use ASan for host lifetime defects and ThreadSanitizer where the
threading environment is supported.

**Common traps**

- Updating function and context separately while invocation can interleave.
- Destroying context immediately after clearing a pointer.
- Letting callbacks recursively mutate the registration list unexpectedly.
- Holding a lock while invoking arbitrary callback code.
- Ignoring shutdown ordering.

**Follow-up questions**

- Should invocation copy the pair before calling?
- Can unregister block?
- What happens when a callback unregisters itself?
- How would you avoid a C++ ownership cycle?

### 9. How should a command dispatch table validate external commands?

**Short answer**

Decode the external value, validate it before indexing, reject or explicitly
handle unknown commands, and call only a handler with the correct typed
signature.

**Deep explanation**

An enum gives names but does not prove an arbitrary integer is a named
enumerator. Direct indexing is safe only when:

- the command domain is validated;
- the table extent is known;
- the entry is present;
- payload bounds and command-specific rules are checked;
- the callback signature matches exactly.

Sparse command IDs may be clearer with a `switch` or searched table.

**C/C++ code/API anchor**

```c
typedef bool (*CommandHandler)(const uint8_t *payload, size_t length);

static const CommandHandler handlers[] = {
    handle_ping,
    handle_reset
};

static bool dispatch(
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

C++ alternatives include `std::array` of callables, a `switch`, or
`std::variant`-based decoded commands.

**Production/debug angle**

Fuzz command IDs and payload lengths. Add `_Static_assert` checks where enum
counts and table extents intentionally correspond. Record unknown-command
errors without reading payload as a known format.

**Common traps**

- Indexing before checking the command.
- Assuming a cast to the enum validates the value.
- Filling missing table entries with an unsafe default.
- Casting handlers to one common but incompatible function type.
- Ignoring payload validation because the command ID was valid.

**Follow-up questions**

- When is a `switch` better?
- How would you handle versioned commands?
- Should unknown commands be rejected or skipped?
- How can table generation reduce duplication without hiding behavior?

### 10. How would you implement and test a finite state machine?

**Short answer**

Define explicit state and event domains, one authoritative transition function
or table, a policy for invalid combinations, and an explicit action order. Test
every state/event pair for a small FSM.

**Deep explanation**

An FSM is more than an enum and switch. It must define:

- legal states;
- legal events;
- next state;
- entry, exit, and transition actions;
- invalid transition behavior;
- asynchronous event ordering and queue policy when relevant.

Keep transition selection pure when possible, then invoke hardware actions
through a separate interface. This makes the transition matrix host-testable.

**C/C++ code/API anchor**

```c
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

bool transition(State current, Event event, State *next);
```

C++ can use the same switch/table design. A State Pattern is useful only when
state-specific behavior benefits from object-level encapsulation.

**Production/debug angle**

Generate a test matrix covering every pair, including invalid integer values.
Trace state, event, accepted/rejected result, next state, and action order. Keep
logging outside restricted interrupt contexts.

**Common traps**

- Assuming enum values are always valid.
- Silently ignoring impossible transitions.
- Mixing register access into transition selection.
- Recursively dispatching events without a reentrancy policy.
- Treating the FSM as synchronized because state is one scalar.

**Follow-up questions**

- Switch or table: which would you choose and why?
- How would you represent an invalid table entry?
- What is the event queue overflow policy?
- When does the C++ State Pattern improve maintainability?

### 11. How do opaque types and operation tables implement OOP-style C?

**Short answer**

An opaque type hides representation behind a module API. An operation table
stores typed function pointers, and an instance carries the table plus private
context. Lifecycle and ownership remain explicit.

**Deep explanation**

This approach provides conventions analogous to:

- encapsulation through incomplete types;
- dynamic dispatch through operation tables;
- instance state through context pointers;
- interfaces through wrapper functions.

C still lacks automatic constructors, destructors, access enforcement,
inheritance checking, and RAII. The API must explicitly define initialization,
failure cleanup, deinitialization, allocation ownership, and ABI versioning.

**C/C++ code/API anchor**

```c
typedef struct {
    bool (*read)(void *context, int *out_value);
    bool (*configure)(void *context, unsigned int rate_hz);
} SensorOps;

typedef struct {
    const SensorOps *ops;
    void *context;
} Sensor;
```

C++ can represent this with an abstract base class, templates, or a
type-erased wrapper. RAII can bind cleanup to object lifetime.

**Production/debug angle**

Keep fixed operation tables `const`. Validate required entries through wrapper
functions. Version interfaces crossing independently updated modules. Test with
a fake context and operation table.

**Common traps**

- Exposing private structure fields in the public header.
- Partially initializing an operation table.
- Forgetting context lifetime.
- Creating one oversized interface for unrelated capabilities.
- Claiming C conventions provide automatic RAII.

**Follow-up questions**

- How can callers allocate a truly opaque type without dynamic allocation?
- How would you version the operation table?
- Composition or an inheritance-like parent prefix?
- When is a public structure simpler and better?

### 12. Why should protocol bytes be decoded explicitly?

**Short answer**

External bytes have their own width, byte order, and validity rules. Casting
them to a native structure assumes bounds, alignment, padding, representation,
and endianness that the protocol may not provide.

**Deep explanation**

Packing can alter padding under one compiler, but it does not solve:

- byte order;
- enum size;
- bit-field ordering;
- scalar representation;
- alignment-safe access;
- short-buffer validation;
- protocol versioning.

Explicit decoding checks length before access and constructs ordinary aligned
objects from specified bytes.

**C/C++ code/API anchor**

```c
static bool decode_u16_be(
    const uint8_t *bytes,
    size_t length,
    uint16_t *out)
{
    if (bytes == NULL || out == NULL || length < 2U) {
        return false;
    }

    *out = (uint16_t)(
        ((uint16_t)bytes[0] << 8)
        | (uint16_t)bytes[1]);
    return true;
}
```

C++ can use `std::span<const std::byte>` for the bounded view, but it still
needs explicit format decoding.

**Production/debug angle**

Fuzz short, oversized, unknown-version, and unknown-type inputs. Test known byte
vectors. Run ASan/UBSan on the parser. Keep protocol errors separate from
internal enum values.

**Common traps**

- Adding `packed` and calling the format portable.
- Assuming `uint16_t` implies network byte order.
- Dereferencing before validating total size.
- Treating a successful test on one target as proof.
- Using native bit-fields for wire fields.

**Follow-up questions**

- What risks remain after `memcpy` into an aligned structure?
- How would you decode signed values?
- What should happen to unknown fields?
- When is a packed structure acceptable?

### 13. Is a ring buffer with volatile indices interrupt-safe?

**Short answer**

Not automatically. `volatile` does not provide atomicity or ordering. Safety
depends on the producer/consumer model, index access width, publication order,
interrupt nesting, overflow policy, compiler, and architecture.

**Deep explanation**

Even a single-producer/single-consumer design has a publication contract:

1. producer writes the element;
2. producer publishes the new head;
3. consumer observes the head;
4. consumer reads the initialized element.

The compiler and CPU must not expose the head before the element is ready. The
index operations must also be atomic for the target. A critical section or
atomic protocol may be required.

Using `count` modified by both producer and consumer can add another shared
read-modify-write race.

**C/C++ code/API anchor**

```c
typedef struct {
    uint8_t data[8];
    size_t head;
    size_t tail;
} ByteRing;
```

This declaration alone says nothing about synchronization.

C11 `_Atomic` or C++ `std::atomic` can express atomic indices when supported,
but the full algorithm and memory ordering still need proof.

**Production/debug angle**

Write down exactly who writes each field. Test wraparound, full, empty, and
overflow. Stress producer/consumer timing on target. Inspect code generation
and use race tools for a supported hosted equivalent.

**Common traps**

- Adding volatile to every field.
- Assuming naturally aligned means every compound operation is atomic.
- Leaving full and empty indistinguishable.
- Publishing head before writing data.
- Ignoring event loss when the buffer is full.

**Follow-up questions**

- Can head and tail each have one writer?
- Why can a shared count be problematic?
- What memory ordering is required?
- When is interrupt masking acceptable?

## Senior Questions

### 14. How do you review an MMIO design without mixing abstraction layers?

**Short answer**

Review six separate layers: ISO C behavior, compiler behavior, ABI, architecture
ordering/atomicity, device register semantics, and product-level concurrency
and error policy.

**Deep explanation**

A correct source expression can still be wrong for the device. A useful review
sequence is:

1. **C:** types, qualifiers, conversions, lifetime, undefined behavior;
2. **compiler:** volatile semantics, extensions, emitted accesses;
3. **ABI:** widths, alignment, calling convention;
4. **architecture:** atomic access sizes, barriers, cache behavior;
5. **device:** read/write side effects, reset values, reserved fields;
6. **product:** owners, interrupts, timing, retries, failure handling.

Claims should name their layer. "Volatile guarantees this" is usually too
imprecise.

**C/C++ code/API anchor**

```c
static inline void device_start(DeviceRegisters *registers)
{
    registers->control = DEVICE_CONTROL_START;
}
```

This function can be reviewed only after `DeviceRegisters`,
`DEVICE_CONTROL_START`, and the register's write semantics are defined by the
selected target contract.

**Production/debug angle**

Require links to device and compiler documentation in design evidence. Inspect
assembly for exact builds, then verify on target with trace or bus observation
when risk warrants it. Keep policy tests independent of MMIO.

**Common traps**

- Treating assembly inspection as a portable guarantee.
- Treating the device manual as if it defines C behavior.
- Assuming a barrier fixes wrong register semantics.
- Copying a register model between device revisions.
- Reviewing only the happy-path transaction.

**Follow-up questions**

- Which layer defines atomic access width?
- Which layer defines write-one-to-clear?
- What can a host unit test prove?
- How should target assumptions be documented?

### 15. Compare `volatile`, atomics, critical sections, and barriers.

**Short answer**

`volatile` preserves special accesses, atomics provide indivisible operations
and memory-order semantics, critical sections prevent selected interleavings,
and barriers enforce ordering or visibility rules defined by the architecture
or compiler. They solve different problems.

**Deep explanation**

- Use volatile for target-defined MMIO access and narrow implementation-defined
  asynchronous cases.
- Use atomics for shared objects when the algorithm and platform support them.
- Use critical sections when multiple operations must be protected as one
  invariant or when atomics are unsuitable.
- Use barriers only when the architecture/device contract requires specific
  ordering.

These mechanisms can appear together. For example, an MMIO doorbell can be
volatile while an architecture barrier ensures prior payload writes are
visible.

**C/C++ code/API anchor**

```c
/* Conceptual only: the barrier is target-specific. */
payload_ready = true;
target_release_barrier();
doorbell_register = START_MASK;
```

C11 atomics use `<stdatomic.h>`. C++ uses `std::atomic` and standardized memory
orders. Neither API automatically specifies peripheral behavior.

**Production/debug angle**

Build a happens-before or execution-order diagram. Measure critical-section
latency. Verify lock-free assumptions rather than guessing. Review barriers
against the architecture manual and exact compiler intrinsic.

**Common traps**

- Substituting volatile for every mechanism.
- Adding a barrier without identifying the reordered operations.
- Assuming lock-free means wait-free or interrupt-safe.
- Holding a critical section across blocking or slow operations.
- Using atomics on an MMIO object without platform support.

**Follow-up questions**

- When is relaxed atomic ordering enough?
- Can interrupt masking synchronize two CPU cores?
- What is the cost of a critical section?
- Does a compiler barrier equal a hardware barrier?

### 16. How would you design a versioned, fakeable HAL?

**Short answer**

Expose narrow capability-oriented operations, carry opaque instance context,
make fixed tables immutable, define lifecycle and error contracts, and include
an explicit version/size strategy when independently built components may
evolve.

**Deep explanation**

A robust HAL separates:

- stable public capability;
- target-specific implementation;
- instance state;
- application policy;
- host fake.

Versioning can use a leading version and structure size, capability query, or
separate interface revisions. New optional operations must not be mistaken for
required ones by old callers.

Do not expose raw registers unless raw register access is truly the intended
contract.

**C/C++ code/API anchor**

```c
typedef struct {
    uint32_t version;
    size_t size;
    bool (*read)(void *context, int *out_value);
    bool (*configure)(void *context, unsigned int rate_hz);
} SensorOps;

typedef struct {
    const SensorOps *ops;
    void *context;
} Sensor;
```

C++ can use abstract interfaces, templates, or type erasure. A C ABI is often
chosen when compiler and language boundaries must remain stable.

**Production/debug angle**

Test old caller/new implementation and new caller/old implementation cases.
Reject incompatible sizes or versions before indirect calls. Use fake
operations to inject errors, timeouts, and boundary values.

**Common traps**

- Appending a function pointer and assuming binary compatibility.
- Omitting structure size validation.
- Making the ops table writable.
- Returning raw device-specific errors without a stable policy.
- Building a giant interface that every fake must implement.

**Follow-up questions**

- Version number, size field, or capability query?
- How are optional operations represented?
- Who owns the context?
- How would exceptions be prevented across a C ABI?

### 17. How should asynchronous handlers interact with normal code?

**Short answer**

Capture the minimum required state, acknowledge the source correctly, publish a
small event using an approved mechanism, and defer logging, allocation,
locking, cleanup, and long-running work to normal context.

**Deep explanation**

Hosted signals and hardware interrupts are different, but both can interrupt
normal execution at awkward points. Arbitrary library or application code may
hold locks or be halfway through an invariant.

For standard C signals, only narrowly permitted operations are portable.
`volatile sig_atomic_t` supports a small flag pattern; it does not make complex
shared state safe.

For embedded interrupts, allowed operations come from the architecture,
compiler, device, RTOS, and product timing contracts.

**C/C++ code/API anchor**

```c
#include <signal.h>

static volatile sig_atomic_t stop_requested;

static void handle_stop(int signal_number)
{
    (void)signal_number;
    stop_requested = 1;
}
```

An embedded ISR may enqueue an event using a separately reviewed target/RTOS
primitive. Do not infer ISR rules from POSIX signal APIs.

**Production/debug angle**

Measure worst-case handler duration. Test event bursts and queue overflow.
Record lost/coalesced-event policy. Move diagnostic formatting to deferred
context and use trace designed for asynchronous use.

**Common traps**

- Calling `printf`, allocation, or ordinary locks from a handler.
- Treating a Boolean flag as an event counter.
- Assuming `sig_atomic_t` is a general thread atomic.
- Calling an ordinary callback from an interrupt.
- Forgetting to acknowledge a device source according to its semantics.

**Follow-up questions**

- When is a flag insufficient?
- How do nested interrupts affect the design?
- Which operations are signal-safe in ISO C versus POSIX?
- How would shutdown wait for deferred work?

### 18. How do you apply MISRA, BARR-C, CERT C, and compiler diagnostics responsibly?

**Short answer**

Use them as complementary engineering inputs within a documented project
process. Select applicable rules, automate checking where possible, review
diagnostics, justify deviations, and never claim compliance from a checklist
or summary alone.

**Deep explanation**

The sources have different roles:

- MISRA C provides licensed safety-oriented guidance and a compliance
  framework;
- BARR-C provides practical embedded coding guidance;
- CERT C focuses on secure and robust C rules and recommendations;
- compiler warnings and static analyzers detect implementation-specific
  patterns;
- the language, compiler, target, and device documents remain authoritative for
  exact behavior.

Rules can conflict with legacy APIs, generated code, hardware headers, or
performance constraints. A controlled deviation records why the alternative is
safe and how it is verified.

**C/C++ code/API anchor**

```text
policy -> compiler flags -> static analysis -> review -> tests
       -> deviation record -> evidence -> maintenance ownership
```

Typical checks cover macro side effects, invalid shifts, qualifier removal,
table bounds, dead states, and incompatible callbacks.

**Production/debug angle**

Pin tool versions and configurations. Triage new diagnostics rather than
blanket-suppressing them. Track deviations with owner, scope, rationale,
evidence, and re-review conditions.

**Common traps**

- Quoting proprietary rule text without the licensed source.
- Claiming warning-clean means correct.
- Disabling a warning globally for one false positive.
- Applying rules without a project execution model.
- Treating generated/vendor code exactly like owned application code without a
  documented boundary.

**Follow-up questions**

- What belongs in a deviation record?
- How do you handle vendor headers?
- Which warnings would you enable first?
- How do safety and security guidance differ?

## Coding Tasks

### 19. Coding task: implement checked 32-bit bit operations

**Prompt**

Implement a checked single-bit mask and helpers to set, clear, toggle, and test
one runtime-selected bit in a `uint32_t`.

**Short answer**

Validate the bit index before shifting an unsigned 32-bit one. Return failure
without modifying output for an invalid index.

**Deep explanation**

The API should separate validation from mutation and avoid signed shifts. It
must define null-pointer behavior and whether output remains unchanged on
failure.

**C/C++ code/API anchor**

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static bool mask_u32(unsigned int bit, uint32_t *out_mask)
{
    if (out_mask == NULL || bit >= 32U) {
        return false;
    }

    *out_mask = UINT32_C(1) << bit;
    return true;
}

static bool set_bit_u32(
    uint32_t value,
    unsigned int bit,
    uint32_t *out_value)
{
    uint32_t mask;

    if (out_value == NULL || !mask_u32(bit, &mask)) {
        return false;
    }

    *out_value = value | mask;
    return true;
}
```

The same structure can implement clear, toggle, and test.

**Production/debug angle**

Test indices `0`, `31`, and `32`, all-zero and all-one values, and null output.
Compile with strict conversion and shift warnings and run UBSan.

**Common traps**

- Computing the shift before validating.
- Using signed `1`.
- Modifying output before all validation succeeds.
- Hard-coding 32 without tying it to the selected interface.
- Reusing this helper blindly on special-function registers.

**Follow-up questions**

- How would you generalize by width?
- Should test return status plus Boolean output?
- What changes for a compile-time bit index?
- How would C++ express the result?

### 20. Coding task: implement a validated table-driven FSM

**Prompt**

Implement transitions for `IDLE`, `RUNNING`, and `FAULT` with events `START`,
`STOP`, `FAILURE`, and `RESET`. Reject invalid state/event combinations.

**Short answer**

Use a const transition table, search only valid rows, write `next` only on
success, and define behavior for invalid enum representations.

**Deep explanation**

A searched table avoids direct indexing by untrusted enum values. The function
can remain pure and independent of hardware actions.

**C/C++ code/API anchor**

```c
#include <stdbool.h>
#include <stddef.h>

typedef enum { IDLE, RUNNING, FAULT } State;
typedef enum { START, STOP, FAILURE, RESET } Event;

typedef struct {
    State state;
    Event event;
    State next;
} Transition;

static const Transition transitions[] = {
    {IDLE, START, RUNNING},
    {RUNNING, STOP, IDLE},
    {RUNNING, FAILURE, FAULT},
    {FAULT, RESET, IDLE}
};

static bool next_state(State state, Event event, State *out_next)
{
    if (out_next == NULL) {
        return false;
    }

    for (size_t index = 0U;
         index < sizeof transitions / sizeof transitions[0];
         ++index) {
        if (transitions[index].state == state
            && transitions[index].event == event) {
            *out_next = transitions[index].next;
            return true;
        }
    }

    return false;
}
```

**Production/debug angle**

Generate tests for every named state/event pair plus invalid cast values.
Separate transition lookup from entry/exit actions so each can be verified.

**Common traps**

- Writing `out_next` on failure.
- Assuming enum inputs are valid.
- Adding hardware side effects inside the table search.
- Forgetting repeated or duplicate transitions.
- Ignoring event queue overflow.

**Follow-up questions**

- How would you detect duplicate rows?
- When would a two-dimensional table be better?
- Where should actions run?
- How would you version state persistence?

### 21. Coding task: design a fakeable sensor HAL

**Prompt**

Define a C sensor interface that supports `read` and `configure`, then show how
application code and a host fake use it.

**Short answer**

Use a const typed ops table plus context, wrapper functions that validate
required entries, and a fake context that records calls and returns controlled
results.

**Deep explanation**

The interface should expose sensor capabilities, not register addresses.
Application policy depends on the interface. Target and fake implementations
provide different operation tables.

**C/C++ code/API anchor**

```c
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
    if (sensor == NULL || sensor->ops == NULL
        || sensor->ops->read == NULL || out_value == NULL) {
        return false;
    }

    return sensor->ops->read(sensor->context, out_value);
}
```

**Production/debug angle**

Use the fake to test success, error, boundaries, retries, and state transitions.
Target tests then focus on register and timing behavior. Keep ops tables
immutable.

**Common traps**

- Putting physical addresses in application code.
- Letting the fake duplicate application policy.
- Leaving ownership of context undefined.
- Returning one magic sensor value for failure.
- Making every operation optional without a capability contract.

**Follow-up questions**

- How would initialization and deinitialization work?
- How would asynchronous completion change the interface?
- How would you version the ops table?
- When would static dispatch be preferable?

## Debugging Scenarios

### 22. Debugging scenario: a macro increments an index twice

**Scenario**

```c
#define MAX_VALUE(a, b) ((a) > (b) ? (a) : (b))

int selected = MAX_VALUE(values[index++], limit);
```

**Short answer**

The selected macro argument appears in both the comparison and result
expression, so `index++` can execute more than once. Replace the macro with a
typed function and keep side effects outside such interfaces.

**Deep explanation**

Parentheses preserve grouping but do not impose single evaluation. Depending on
the comparison result, one argument can be evaluated once and the other twice.
This changes `index`, can access the wrong element, and may go out of bounds.

**C/C++ code/API anchor**

```c
static inline int max_int(int left, int right)
{
    return left > right ? left : right;
}

int value = values[index];
++index;
int selected = max_int(value, limit);
```

C++ can use `std::max`, but passing side-effecting expressions still makes code
harder to reason about because argument evaluation order and intent matter.

**Production/debug angle**

Inspect preprocessed output with `cc -E`. Enable macro-related static-analysis
checks. Add boundary tests where `index` points at the final valid element.

**Common traps**

- Adding more parentheses and declaring it fixed.
- Blaming optimizer behavior.
- Assuming both macro arguments execute exactly once.
- Replacing it with another unsafe generic macro.
- Keeping required side effects inside logging or assertion arguments.

**Follow-up questions**

- Can `_Generic` make the macro single-evaluation?
- Why is a temporary clearer?
- What would UBSan or ASan detect?
- When is token generation still worth a macro?

### 23. Debugging scenario: an optimized build misses ring-buffer data

**Scenario**

An interrupt-like producer writes an element and advances `head`. A consumer
occasionally observes the new `head` but stale element data. Both indices were
declared `volatile`.

**Short answer**

Volatile does not establish the required publication ordering. The algorithm
needs a target-approved atomic/ordering or critical-section protocol, and index
atomicity must also be verified.

**Deep explanation**

The intended order is:

```text
write element -> publish head -> observe head -> read element
```

The language and architecture need an ordering mechanism that makes this
relationship true. Volatile access alone is not a C concurrency guarantee.

The design must also define one writer per index, wraparound, full/empty state,
and overflow.

**C/C++ code/API anchor**

C11 atomics can express a hosted single-producer/single-consumer publication
protocol:

```c
#include <stddef.h>
#include <stdatomic.h>

static _Atomic size_t head;

static void publish_head(size_t next)
{
    atomic_store_explicit(&head, next, memory_order_release);
}

static size_t observe_head(void)
{
    return atomic_load_explicit(&head, memory_order_acquire);
}
```

Whether this is appropriate for the embedded target and interrupt context must
be verified.

**Production/debug angle**

Reproduce at optimization levels and inspect assembly. Stress wraparound and
timing. Review target atomic support and interrupt rules. Do not fix the symptom
with delays.

**Common traps**

- Adding volatile to the data array.
- Assuming aligned `size_t` access is atomic on every target.
- Adding a compiler barrier when a hardware barrier is required.
- Using atomics without proving the complete algorithm.
- Ignoring overflow because data usually arrives slowly.

**Follow-up questions**

- Which fields have one writer?
- What if atomics are not lock-free?
- Could a short critical section be simpler?
- How would you detect dropped items?

### 24. Debugging scenario: a callback crashes after initialization returns

**Scenario**

```c
static void configure_timer(Timer *timer)
{
    TimerStats stats = {0U};
    timer_set_callback(timer, record_tick, &stats);
}
```

The timer invokes the callback later.

**Short answer**

The stored context points to `stats`, whose lifetime ends when
`configure_timer` returns. The later callback dereferences a dangling pointer.

**Deep explanation**

Callback function lifetime and context lifetime are separate. A static function
can remain callable while its context is dead.

The fix depends on ownership:

- store state in a longer-lived caller object;
- let the timer own a copy;
- allocate with explicit cleanup if policy permits;
- unregister and drain callbacks before destruction.

**C/C++ code/API anchor**

```c
typedef struct {
    Timer timer;
    TimerStats stats;
} Application;

static void application_init(Application *application)
{
    timer_set_callback(
        &application->timer,
        record_tick,
        &application->stats);
}
```

The `Application` must outlive all timer callbacks.

**Production/debug angle**

ASan can catch many host-side uses after scope. Trace registration and teardown.
Test delayed invocation during shutdown and callback cancellation.

**Common traps**

- Making `stats` static without considering reentrancy or multiple instances.
- Copying only the context pointer.
- Clearing the callback after destroying context.
- Assuming one successful run proves lifetime.
- Fixing C lifetime with an undocumented heap allocation.

**Follow-up questions**

- Who should own callback state?
- How is in-flight invocation drained?
- What if the callback runs in an interrupt?
- How would C++ RAII improve teardown?

### 25. Debugging scenario: a packed packet works on one target only

**Scenario**

```c
struct __attribute__((packed)) Header {
    uint8_t version;
    uint16_t length;
};

const struct Header *header = (const struct Header *)bytes;
```

**Short answer**

The code still assumes compiler-specific packing, valid alignment/access,
native byte order, enough bytes, and compatible representation. Decode fields
explicitly after validating bounds.

**Deep explanation**

Packing addresses only part of layout for one compiler. The target can fault or
generate expensive code for unaligned members. The protocol's `length` byte
order can differ from the host. The buffer can also be shorter than the
structure.

**C/C++ code/API anchor**

```c
if (size < 3U) {
    return false;
}

uint8_t version = bytes[0];
uint16_t length = (uint16_t)(
    ((uint16_t)bytes[1] << 8)
    | (uint16_t)bytes[2]);
```

**Production/debug angle**

Test known byte sequences, short buffers, and both endian targets where
available. Run alignment sanitization on hosted builds. Inspect member offsets
only as target evidence, not as protocol definition.

**Common traps**

- Copying the packed object with `memcpy` and forgetting endianness.
- Taking the address of an unaligned packed member.
- Adding a static assertion and calling the parser portable.
- Reading `length` before checking `size`.
- Using native bit-fields for protocol flags.

**Follow-up questions**

- When can packed structures be justified?
- What does `memcpy` solve?
- How should oversized lengths be handled?
- How would C++ represent the bounded input?

## Rapid-Fire Traps

### 26. Are these statements correct?

**Short answer**

| Statement | Verdict |
| --- | --- |
| "`volatile` makes code thread-safe." | False |
| "A macro is always faster than a function." | False |
| "Parentheses make a function-like macro safe." | False |
| "`restrict` is only a performance hint." | False |
| "A C enum value is always one of its named enumerators." | False |
| "A valid callback function proves the context is valid." | False |
| "A packed structure is a portable wire format." | False |
| "Volatile ring-buffer indices provide synchronization." | False |
| "A HAL should expose all registers for flexibility." | Usually false |
| "A signal handler can perform normal cleanup." | Not generally |

**Deep explanation**

Each slogan erases a required contract: atomicity, argument evaluation,
aliasing, value validation, lifetime, representation, ordering, abstraction, or
execution context.

**C/C++ code/API anchor**

Useful anchors are:

```c
UINT32_C(1) << bit
static inline
volatile const
_Atomic
function_pointer(context, event)
enum + validated table
opaque handle + const ops table
va_start / va_arg / va_end
```

C++ alternatives include `constexpr`, lambdas, templates, `std::function`,
classes, RAII, `enum class`, and `std::atomic`.

**Production/debug angle**

Turn every slogan into a review question:

- Which layer guarantees this?
- Who owns this object?
- Which execution contexts access it?
- What value range was validated?
- Which byte order and layout define this data?
- How was the exact build verified?

**Common traps**

- Memorizing verdicts without explaining mechanisms.
- Replacing C misconceptions with absolute C++ safety claims.
- Treating one compiler observation as ISO C.
- Applying ordinary RAM rules to peripheral registers.
- Ignoring failure and teardown paths.

**Follow-up questions**

- Which statements can become true under a narrower documented contract?
- Which defects can sanitizers detect?
- Which require target documentation?
- Which need static analysis or exhaustive tests?

## Interview Evaluation Checklist

A strong candidate should be able to:

- manipulate masks with unsigned, width-aware operations;
- identify invalid shift counts and integer-promotion hazards;
- explain `volatile` without calling it atomic or thread-safe;
- separate C, compiler, architecture, device, and product guarantees;
- review ordinary and special register semantics;
- compare macros with inline and `constexpr` functions;
- explain `const`, `restrict`, `static`, and `extern` precisely;
- design callback/context lifetime and teardown;
- validate dispatch and FSM inputs before table indexing;
- separate pure transition policy from hardware effects;
- design opaque C objects and immutable operation tables;
- create a narrow, fakeable HAL;
- reject packed/native records as general serialization;
- reason about ring-buffer publication and synchronization;
- keep asynchronous handlers minimal;
- use strict warnings, preprocessing output, sanitizers, assembly inspection,
  exhaustive transition tests, and target observability appropriately;
- explain how safety guidance, compiler diagnostics, review, tests, and
  controlled deviations fit together.
