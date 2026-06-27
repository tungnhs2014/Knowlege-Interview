# 02 - C Fundamentals: Interview Pack

## How To Use This Pack

For each question:

1. Start with the **Short answer**.
2. Expand into the language mechanism only when needed.
3. Use the **C/C++ code/API anchor** to make the answer concrete.
4. Finish with production consequences, diagnostics, and tradeoffs.

The examples use C17 as the practical baseline. Important C23 differences are
called out where they affect an answer.

## Beginner Questions

### 1. What is the difference between declaration, definition, initialization, and assignment?

**Short answer**

A declaration introduces a name and type. A definition creates an object or
provides a function body. Initialization establishes an object's initial value.
Assignment changes the value of an existing modifiable object.

**Deep explanation**

These concepts overlap but are not interchangeable. A declaration can also be a
definition, as in `int count = 0;`. An `extern` object declaration normally
refers to storage defined elsewhere. Initialization occurs as part of an object
definition, while assignment is an expression executed after the object already
exists.

For functions, a prototype declaration allows calls to be type-checked. The
definition supplies the executable body needed by the linker.

**C/C++ code/API anchor**

```c
extern int system_status;       /* declaration */

int system_status = 0;          /* definition + initialization */

int read_status(void);          /* function declaration/prototype */

int read_status(void)           /* function definition */
{
    return system_status;
}

void reset_status(void)
{
    system_status = 0;          /* assignment */
}
```

**Production/debug angle**

Missing definitions usually become linker errors. Duplicate external
definitions can become multiple-definition errors. Treat public declarations as
API contracts and compile all callers against the same header.

**Common traps**

- Saying every declaration allocates storage.
- Calling assignment initialization.
- Putting ordinary external object definitions in a header.
- Declaring a function with a signature different from its definition.

**Follow-up questions**

- Is `int value;` a declaration, definition, or both?
- Does `extern int value;` initialize `value`?
- Why should function declarations live in headers?

### 2. What is the difference between scope, linkage, storage duration, and lifetime?

**Short answer**

Scope controls where a name is visible. Linkage determines whether declarations
refer to the same entity. Storage duration describes how long storage exists.
Lifetime describes when an object exists and may be used according to the
language rules.

**Deep explanation**

A block-scope automatic object normally exists during execution of its block.
A block-scope `static` object has the same narrow name visibility but persists
for the program's execution. A file-scope `static` name has internal linkage,
so it cannot be resolved by name from another translation unit.

The dimensions must be analyzed separately. "Global" is often too vague because
it can mix file scope, static storage duration, and external linkage.

**C/C++ code/API anchor**

```c
static int file_state = 0; /* file scope, internal linkage,
                              static storage duration */

unsigned int next_id(void)
{
    static unsigned int id = 0U; /* block scope, no linkage,
                                    static storage duration */
    int copy = file_state;       /* block scope, automatic storage duration */
    (void)copy;
    return ++id;
}
```

**Production/debug angle**

Persistent objects can hide shared mutable state, reduce reentrancy, complicate
tests, and create data races when code later becomes concurrent. During review,
ask all four questions rather than labeling an object merely "local" or
"global."

**Common traps**

- Assuming block scope implies automatic storage duration.
- Assuming every static-storage object has internal linkage.
- Equating storage duration with a physical stack or data segment.
- Believing a static local is automatically thread-safe to access.

**Follow-up questions**

- What linkage does a block-scope `static` object have?
- How does file-scope `static` affect a function?
- Why can a static local make a function non-reentrant?

### 3. Why must automatic local variables be initialized before use?

**Short answer**

An automatic object without an initializer can have an indeterminate value.
Reading it is unsafe and can produce undefined behavior under the applicable C
rules.

**Deep explanation**

C does not implicitly zero-initialize ordinary automatic local objects. The
compiler can optimize under the assumption that undefined behavior does not
occur, so an uninitialized read is not merely "a random stack value." Its
effects can change with optimization, surrounding code, or compiler version.

Objects with static or thread storage duration receive zero initialization when
no explicit initializer supplies another value.

**C/C++ code/API anchor**

```c
int read_sample(int available)
{
    int sample;

    if (available) {
        sample = 42;
    }

    return sample; /* unsafe when available == 0 */
}
```

A safer interface makes every path explicit:

```c
#include <stdbool.h>
#include <stddef.h>

bool read_sample(int available, int *sample)
{
    if (!available || sample == NULL) {
        return false;
    }

    *sample = 42;
    return true;
}
```

**Production/debug angle**

Enable `-Wall -Wextra` and use path-sensitive static analysis. Reproduce
optimization-dependent behavior with debug and optimized builds. Memory
sanitizers can help on supported platforms, but initialization by construction
is the primary fix.

**Common traps**

- Describing the value as reliably containing old stack bytes.
- Initializing to zero when zero falsely means "valid data."
- Fixing the symptom while retaining an ambiguous success/failure contract.
- Assuming a debug build that appears stable proves correctness.

**Follow-up questions**

- Which objects are zero-initialized automatically?
- How would you represent "no sample available" without a sentinel collision?
- Can a compiler diagnose every uninitialized read?

### 4. Why is `int` not always the right type, and when should `uint32_t` be used?

**Short answer**

Choose a type from its required value range and interface contract. Use
`uint32_t` when an unsigned exact 32-bit representation is required and the
implementation provides it; do not use it merely because a desktop `int`
happens to be 32 bits.

**Deep explanation**

C guarantees minimum ranges for fundamental integer types, not universal
widths. `sizeof(char)` is always one C byte, but `CHAR_BIT` need not be eight.
Exact-width typedefs such as `uint32_t` are optional because some implementations
cannot provide a type with exactly that width and no padding bits.

Use `size_t` for object sizes, least-width types when at least N bits are
required, and exact-width types for wire formats or hardware-defined fields.

**C/C++ code/API anchor**

```c
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

void print_packet_id(uint32_t packet_id)
{
    printf("packet=%" PRIu32 "\n", packet_id);
}
```

**Production/debug angle**

Document width, signedness, units, and overflow behavior at API boundaries.
Use `<limits.h>` and compile-time assertions where platform assumptions are
required. Validate serialized byte order separately from the host integer type.

**Common traps**

- Assuming `int` is exactly 32 bits.
- Assuming `uint8_t` and `uint32_t` exist everywhere.
- Printing a fixed-width type with an assumed format such as `%u`.
- Using unsigned types for ordinary arithmetic solely because values "should
  never be negative."

**Follow-up questions**

- When would `uint_least32_t` be more portable?
- Why is `size_t` unsigned?
- Does `uint32_t` define byte order?

### 5. Compare `if`, `switch`, `for`, `while`, and `do while`.

**Short answer**

`if` handles general conditions, while `switch` dispatches on integral values.
`for` expresses explicit iteration state, `while` repeats while a condition is
true, and `do while` executes its body at least once.

**Deep explanation**

`switch` applies integer promotions to its controlling expression. Case labels
must be unique integer constant expressions after conversion. Fallthrough is
part of the language and must be intentional.

`for` and `while` are entry-controlled and can execute zero times. `do while`
checks its condition after the body. `break` exits the nearest loop or switch;
`continue` advances the nearest loop according to that loop's mechanics.

**C/C++ code/API anchor**

```c
switch (command) {
    case 0:
        stop();
        break;
    case 1:
        start();
        break;
    default:
        report_invalid_command();
        break;
}

for (size_t index = 0U; index < count; ++index) {
    process(index);
}
```

**Production/debug angle**

Test zero, one, maximum, and one-past-boundary iterations. Enable switch and
fallthrough warnings. Choose a construct for clarity and invariants, not the
claim that `switch` is inherently faster than `if`.

**Common traps**

- Missing `break` unintentionally.
- Writing an unsigned countdown condition that is always true.
- Forgetting to update loop state.
- Assuming `do while` can execute zero times.

**Follow-up questions**

- When is intentional switch fallthrough useful?
- What does `continue` do in a `for` loop?
- How would you prove a loop terminates?

## Mid-Level Questions

### 6. Explain integer promotions and the usual arithmetic conversions.

**Short answer**

Integer promotions first convert narrow integer types where required, usually
to `int` or `unsigned int`. The usual arithmetic conversions then choose a
common real type for operands of many binary arithmetic and comparison
operators.

**Deep explanation**

An integer type with rank no greater than `int` promotes to `int` if `int` can
represent all its values; otherwise it promotes to `unsigned int`. Mixed
signed/unsigned operations then follow rules based on rank and representable
range, not a simple "small type to large type" ladder.

This explains why an arithmetic expression can have a type different from both
source declarations and why negative signed values can become large unsigned
values.

**C/C++ code/API anchor**

```c
#include <stdio.h>

int main(void)
{
    unsigned char left = 200U;
    unsigned char right = 20U;

    int sum = left + right; /* commonly both promote to int */
    printf("%d\n", sum);
    return 0;
}
```

**Production/debug angle**

Use conversion and sign warnings, inspect operand types, and reduce a failure to
a small expression. Do not suppress a warning with a cast until range and
domain checks make the conversion valid.

**Common traps**

- Presenting one linear promotion hierarchy.
- Assuming unsigned operands always remain at their declared width.
- Believing a wider type conversion cannot lose precision.
- Ignoring integer promotions before bitwise and shift operations.

**Follow-up questions**

- Can `unsigned short` promote to `int`?
- What type does `unsigned char + unsigned char` have?
- How do floating operands change the common type?

### 7. Why can a signed/unsigned comparison produce an unexpected result?

**Short answer**

The signed operand may be converted to an unsigned type before comparison. A
negative value then becomes a large unsigned value, reversing the intuitive
result.

**Deep explanation**

The exact conversion depends on conversion rank and representable ranges.
`size_t` is unsigned, so comparing it directly with an `int` error code is a
common source of defects. The fix is to separate error-domain validation from
size-domain comparison.

**C/C++ code/API anchor**

```c
#include <stddef.h>

int index_is_valid(int index, size_t count)
{
    return index >= 0 && (size_t)index < count;
}
```

**Production/debug angle**

Compile with `-Wsign-compare` and `-Wsign-conversion`. Review every conversion
between error-bearing signed values and size/count types. Boundary tests must
include `-1`, zero, and the maximum valid index.

**Common traps**

- Casting first and checking negativity afterward.
- Changing every value to unsigned instead of modeling the domain.
- Assuming `(unsigned)-1` is a small negative-like value.
- Treating the warning as compiler noise.

**Follow-up questions**

- What happens when `-1` converts to an unsigned type?
- Should an array index API accept `int` or `size_t`?
- How would you represent both an error and a count?

### 8. Compare signed overflow, unsigned wrap, narrowing, and invalid shifts.

**Short answer**

Signed integer overflow is undefined behavior. Unsigned arithmetic is modulo
arithmetic. Narrowing conversions may alter or lose values. Invalid shift
counts and some signed shifts are undefined or implementation-defined.

**Deep explanation**

Unsigned modulo behavior is defined but can still violate application
requirements. Converting an out-of-range integer to a signed type is
implementation-defined or can raise an implementation-defined signal.
Converting a finite floating value to an integer discards the fractional part,
but behavior is undefined if the resulting integral value is not representable.

A shift count that is negative or at least the width of the promoted left
operand is undefined behavior. Bit manipulation is normally clearer with
unsigned operands and validated counts.

**C/C++ code/API anchor**

```c
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>

bool set_bit(unsigned int *value, unsigned int bit)
{
    const unsigned int width =
        (unsigned int)(sizeof(*value) * CHAR_BIT);

    if (value == NULL || bit >= width) {
        return false;
    }

    *value |= 1U << bit;
    return true;
}
```

**Production/debug angle**

Use UBSan for signed overflow and invalid shifts on supported hosted targets.
Add range checks before arithmetic and conversion. Test maximum values and
boundary shift counts explicitly.

**Common traps**

- Relying on two's-complement hardware to define signed overflow.
- Calling all unsigned wrap harmless.
- Assuming an explicit cast makes a conversion safe.
- Using `1 << bit` with signed `int` for a wide mask.

**Follow-up questions**

- How would you implement checked signed addition?
- Is right shift of a negative signed value portable?
- What happens when a shift count equals the operand width?

### 9. What does `static` mean in different C contexts, and how does `extern` differ?

**Short answer**

At block scope, `static` gives an object static storage duration. At file scope,
`static` gives an object or function internal linkage. `extern` commonly
declares an entity with external linkage whose definition exists elsewhere.

**Deep explanation**

A static local persists between calls but its name remains local to the block.
A file-scope static helper is private to its translation unit. An `extern`
declaration does not create a separate "extern storage area." The program still
needs the required definition.

Tentative definitions and header placement matter. Public headers should
normally declare external objects, while exactly one source file defines them.

**C/C++ code/API anchor**

```c
/* status.h */
extern int system_status;

/* status.c */
int system_status = 0;

static int normalize_status(int value)
{
    return value < 0 ? 0 : value;
}

unsigned int next_sequence(void)
{
    static unsigned int sequence = 0U;
    return ++sequence;
}
```

**Production/debug angle**

Use symbol inspection or linker diagnostics for duplicate/missing definitions.
Review static locals for reentrancy, test isolation, overflow, and data races.
Prefer explicit state objects when hidden persistence harms API clarity.

**Common traps**

- Saying `static` always means file-private.
- Saying `extern` allocates storage.
- Defining shared mutable state in a header.
- Treating a static local counter as concurrency-safe.

**Follow-up questions**

- Does a file-scope non-static object have static storage duration?
- Can an `extern` declaration include an initializer?
- How would you remove hidden state from `next_sequence`?

### 10. Why can a `printf` or `scanf` format mismatch be undefined behavior?

**Short answer**

Variadic formatted I/O relies on the format string to interpret arguments or
destination pointers. If the specified type does not match the actual promoted
argument or pointed-to type, the function accesses it using the wrong contract,
which can produce undefined behavior.

**Deep explanation**

Default argument promotions apply to variadic arguments, but they do not make
all integer types interchangeable. `size_t` requires `%zu`. Fixed-width integer
types should use `<inttypes.h>` macros because their underlying fundamental
types vary by implementation.

For input, the pointer target type and field width are critical. Unbounded `%s`
can overflow a destination array. Return values must be checked to distinguish
successful conversion, malformed input, and end-of-file.

**C/C++ code/API anchor**

```c
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

void print_values(size_t count, uint32_t id)
{
    printf("count=%zu, id=%" PRIu32 "\n", count, id);
}
```

**Production/debug angle**

Enable `-Wformat=2` and keep format strings literal where practical. Prefer
`fgets` plus validated parsing for line-oriented input. Treat ignored I/O return
values as error-path defects.

**Common traps**

- Printing `size_t` with `%d`.
- Assuming `uint32_t` is always `unsigned int`.
- Passing `double *` to `%f` in `scanf`, which requires `float *`.
- Using unbounded `%s` or the removed `gets` function.

**Follow-up questions**

- Why does `printf("%f", value)` accept a `float` argument?
- What does `scanf` return?
- How would you parse a checked integer from a line?

## Senior Questions

### 11. How would you design an integer-type policy for an embedded protocol parser?

**Short answer**

Separate wire representation, computation, sizes, and error domains. Use
exact-width unsigned types for protocol fields when available, `size_t` for
buffer sizes, appropriately ranged signed types for signed quantities, and
checked conversions at every boundary.

**Deep explanation**

An exact-width host type does not define byte order, alignment, or safe direct
access to packet bytes. Parse bytes deliberately and construct values with
unsigned arithmetic. Validate lengths before reading fields. Convert to wider
calculation types when intermediate arithmetic can exceed field width.

The policy should document units, legal ranges, overflow handling, and whether
modulo arithmetic is intentional. It should also state the C version and target
assumptions.

**C/C++ code/API anchor**

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool read_u16_be(
    const unsigned char *buffer,
    size_t length,
    uint16_t *value)
{
    if (buffer == NULL || value == NULL || length < 2U) {
        return false;
    }

    *value = (uint16_t)(
        ((uint16_t)buffer[0] << 8) |
        (uint16_t)buffer[1]);
    return true;
}
```

**Production/debug angle**

Test truncated packets, maximum field values, malformed lengths, and all byte
orders. Compile with conversion warnings and sanitizers on the host, then test
on the target. Use golden byte sequences rather than host-struct casts.

**Common traps**

- Casting a packet pointer to a struct pointer.
- Confusing fixed width with network byte order.
- Performing length arithmetic without overflow checks.
- Returning a count and an error through one ambiguous unsigned sentinel.

**Follow-up questions**

- How would you parse a signed 16-bit field?
- What changes if `uint16_t` is unavailable?
- How would you validate `offset + field_length` safely?

### 12. Explain precedence, evaluation order, sequencing, and short-circuiting.

**Short answer**

Precedence and associativity determine how tokens are grouped. Evaluation order
determines when subexpressions run. Sequencing constrains side-effect ordering.
`&&` and `||` short-circuit and sequence the right operand after the left when
the right operand is evaluated.

**Deep explanation**

Higher precedence does not imply earlier execution. Function-call operands and
many binary operator operands do not have a fixed left-to-right order in C.
If an object is modified and also read for another purpose without required
sequencing, behavior can be undefined.

Short-circuiting is useful for guards, but hiding significant side effects in
conditions can reduce maintainability.

**C/C++ code/API anchor**

```c
if (buffer != NULL && length > 0U && buffer[0] == 0xAAU) {
    process(buffer, length);
}
```

Unsafe:

```c
int index = 0;
int value = index++ + index; /* undefined behavior */
```

**Production/debug angle**

Split side-effecting subexpressions into named statements. Enable warnings, but
do not expect the compiler to diagnose every unsequenced operation. Review
macros especially carefully because expansion can duplicate side effects.

**Common traps**

- Saying multiplication's operands execute before addition's operands.
- Assuming function arguments evaluate left to right.
- Using `&` where a null-guard requires `&&`.
- Writing compact expressions whose correctness depends on side-effect timing.

**Follow-up questions**

- Does the comma separator between function arguments sequence them?
- What does the comma operator guarantee?
- Is `i = i + 1` well-defined?

### 13. What does `volatile` guarantee, and why is it not a concurrency primitive?

**Short answer**

`volatile` affects observable accesses to a volatile-qualified object according
to the C abstract machine. It does not guarantee atomicity, mutual exclusion,
inter-thread synchronization, memory ordering, or race freedom.

**Deep explanation**

`volatile` can be relevant when an object changes outside ordinary program flow,
such as implementation-defined hardware access or signal-related state under
specific rules. It prevents certain access elimination or combination, but two
threads performing conflicting non-atomic accesses still have a data race.

Concurrency requires atomics or synchronization mechanisms designed for that
purpose. Hardware access also needs target-specific ordering and width rules
beyond the qualifier itself.

**C/C++ code/API anchor**

```c
volatile unsigned int event_flag;

void wait_for_event(void)
{
    while (event_flag == 0U) {
        /* observable volatile reads, but no thread synchronization */
    }
}
```

**Production/debug angle**

Document why each volatile object is volatile. Inspect generated code only as
supporting evidence, not as the language contract. Use race detectors or
concurrency tests for shared state, and keep platform-specific access wrappers
outside generic business logic.

**Common traps**

- Calling `volatile int lock` a lock.
- Assuming volatile increment is atomic.
- Treating volatile as a portable memory barrier.
- Adding volatile to hide an optimization-sensitive bug.

**Follow-up questions**

- When would C atomics be appropriate?
- Can `volatile` fix a data race?
- Why might hardware access need more than `volatile`?

### 14. When is recursion acceptable in production or embedded C?

**Short answer**

Recursion is acceptable when termination, maximum depth, stack usage, overflow,
and failure behavior are bounded and justified. Otherwise an iterative design
is usually easier to analyze.

**Deep explanation**

Each active call normally consumes stack, although the exact implementation is
not prescribed. A base case proves only logical termination, not a safe maximum
depth. Input-controlled recursion can exhaust stack. Tail-call optimization is
not guaranteed by C.

Recursive arithmetic can also overflow independently of stack depth. Production
acceptance therefore needs both algorithmic and numeric bounds.

**C/C++ code/API anchor**

```c
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool factorial_u32(uint32_t value, uint32_t *result)
{
    uint32_t current = 1U;

    if (result == NULL) {
        return false;
    }

    for (uint32_t factor = 2U; factor <= value; ++factor) {
        if (current > UINT32_MAX / factor) {
            return false;
        }
        current *= factor;
    }

    *result = current;
    return true;
}
```

**Production/debug angle**

Measure or estimate stack use, test maximum depth, and inspect compiler stack
reports where available. Prefer iteration for simple linear recursion. If
recursion remains, encode and test an explicit depth limit.

**Common traps**

- Assuming a base case guarantees safe stack usage.
- Assuming the compiler performs tail-call optimization.
- Ignoring arithmetic overflow in recursive examples.
- Using recursion in an unbounded parser or tree walk without resource limits.

**Follow-up questions**

- How would you bound a recursive tree traversal?
- When is recursion clearer than an explicit stack?
- How do interrupts or small thread stacks affect the decision?

### 15. Design a verification strategy for a low-level C module dominated by integer and control-flow logic.

**Short answer**

Use a declared C standard, strong warnings, static analysis, boundary-focused
tests, sanitizer builds on supported hosts, debug/optimized configurations, and
target tests for implementation assumptions.

**Deep explanation**

No single tool covers the full failure space. Warnings expose suspicious
conversions and formats. Static analysis explores paths. UBSan can detect
runtime signed overflow and invalid shifts. Unit tests establish domain
behavior. Target tests validate widths, ABI assumptions, timing, and
freestanding differences.

The test design should derive from contracts: valid ranges, error paths,
termination, state transitions, and whether wrap is intentional.

**C/C++ code/API anchor**

```bash
cc -std=c17 -Wall -Wextra -Wpedantic \
  -Wconversion -Wsign-conversion -Wshadow -Wformat=2 \
  -O0 -g3 module.c module_test.c -o module-test

cc -std=c17 -Wall -Wextra -Wpedantic \
  -O1 -g -fsanitize=address,undefined \
  -fno-omit-frame-pointer \
  module.c module_test.c -o module-sanitize
```

**Production/debug angle**

Run tests at zero, one, minima, maxima, and one step outside valid ranges.
Exercise malformed input, division guards, maximum shifts, empty loops, and
error returns. Compare debug and optimized builds without changing the contract.

**Common traps**

- Treating a warning-free build as proof of correctness.
- Running sanitizers only on happy paths.
- Assuming host sanitizer success validates the embedded target.
- Enabling `-Werror` before establishing a sustainable warning policy for
  project and third-party code.

**Follow-up questions**

- Which defects can UBSan detect here?
- What would you require in CI versus target hardware tests?
- How would you test implementation-defined assumptions?

## Coding Tasks

### Task 1. Implement Checked Signed Addition

Write:

```c
bool add_int(int left, int right, int *result);
```

Requirements:

- reject a null output pointer;
- detect overflow before performing overflowing arithmetic;
- return `true` only when `*result` is written;
- compile cleanly under C17 warning flags.

**Evaluation points**

- Correct handling of `INT_MIN` and `INT_MAX`.
- No signed overflow in the validation expression itself.
- Clear output contract.
- Tests for positive overflow, negative overflow, and boundary success.

**Follow-up extensions**

- Implement checked multiplication.
- Return a detailed status enum.
- Compare with compiler overflow builtins while keeping a portable fallback.

### Task 2. Parse A Validated Decimal Integer

Read one line from `stdin` and convert it to `int`.

Requirements:

- use `fgets`, not `gets`;
- use `strtol` or an equivalent checked parser;
- reject empty, malformed, trailing-junk, and out-of-range input;
- distinguish input failure from invalid text.

**Evaluation points**

- `errno` handling.
- Correct `end` pointer checks.
- Range validation against `INT_MIN` and `INT_MAX`.
- No unchecked narrowing cast.

**Follow-up extensions**

- Permit surrounding whitespace deliberately.
- Parse a bounded unsigned value.
- Move parsing into a reusable function with no direct I/O.

### Task 3. Implement Safe Bit Operations

Create functions to set, clear, toggle, and test a bit in an `unsigned int`.

Requirements:

- validate the bit index against `sizeof(value) * CHAR_BIT`;
- avoid signed shifts;
- reject null output pointers where applicable;
- document whether mutation occurs on failure.

**Evaluation points**

- Correct boundary checks.
- Use of unsigned constants.
- No shift by the operand width.
- Tests for bit zero and the highest valid bit.

**Follow-up extensions**

- Adapt the API to `uint32_t`.
- Operate on a caller-provided byte array.
- Discuss atomicity if the value becomes shared.

## Debugging Scenarios

### Scenario 1. The Reverse Loop Never Ends

```c
for (size_t index = count - 1U; index >= 0U; --index) {
    process(index);
}
```

**Diagnosis**

`size_t` is unsigned, so `index >= 0U` is always true. The initialization also
wraps when `count` is zero.

**Repair**

```c
for (size_t index = count; index-- > 0U;) {
    process(index);
}
```

**Debug discussion**

Enable sign/type warnings, test `count == 0`, and inspect `index` around zero.
Ask whether reverse iteration is necessary or merely habitual.

### Scenario 2. A Range Check Passes In Debug But Fails In Production

```c
int total = current + increment;

if (total < current) {
    return ERROR_OVERFLOW;
}
```

**Diagnosis**

If signed addition overflows, behavior is already undefined before the check.
The compiler can optimize based on the assumption that valid signed arithmetic
does not overflow.

**Repair**

Check against `INT_MAX - increment` or `INT_MIN - increment` before addition,
with branches that avoid overflow in the check itself.

**Debug discussion**

Run UBSan, compare optimization levels, and add tests around integer limits.
Do not fix this by making optimization assumptions or adding `volatile`.

### Scenario 3. The Program Prints Nonsense For A Correct Value

```c
size_t count = sizeof buffer;
printf("count=%d\n", count);
```

**Diagnosis**

`%d` expects an `int`, but `count` has type `size_t`. Variadic formatted I/O
cannot recover the correct type from the argument.

**Repair**

```c
printf("count=%zu\n", count);
```

**Debug discussion**

Enable `-Wformat=2`, treat format warnings as correctness defects, and use
`<inttypes.h>` macros for fixed-width integer types.

### Scenario 4. An Optimized Build Changes An Expression's Result

```c
int index = 0;
int value = index++ + index;
```

**Diagnosis**

The modification and other access to `index` are unsequenced, producing
undefined behavior.

**Repair**

```c
int index = 0;
int previous = index;
++index;
int value = previous + index;
```

**Debug discussion**

Split side effects into separate statements, use UBSan where supported, and
review macro expansions for duplicated evaluation.

## Rapid-Fire Checks

- Is `sizeof(char)` always one? Yes, one C byte.
- Is one C byte always eight bits? No; inspect `CHAR_BIT`.
- Does `const int n = 8;` always make `n` an integer constant expression in C?
  No.
- Does unsigned overflow have undefined behavior? No; unsigned arithmetic is
  modulo arithmetic, though wrap may still be a logic bug.
- Is signed overflow guaranteed to wrap? No.
- Does a cast prove a narrowing conversion is safe? No.
- Does `volatile` make an access atomic? No.
- Does precedence specify operand evaluation order? No.
- Does `do while` execute at least once? Yes.
- Is `switch` inherently faster than `if`? No.
- Can `uint32_t` be absent? Yes.
- What format prints `size_t`? `%zu`.
- Does C pass arguments by reference? No; C passes by value.
- Is tail-call optimization guaranteed? No.
- Can `goto` be reasonable in C? Yes, for disciplined centralized cleanup.

## Final Review Checklist

Before an interview, be able to:

- distinguish declaration, definition, initialization, and assignment;
- explain scope, linkage, storage duration, and lifetime independently;
- reason through integer promotions and signed/unsigned conversions;
- classify signed overflow, unsigned wrap, narrowing, and shift behavior;
- choose among fundamental, size, least-width, and exact-width integer types;
- explain precedence versus evaluation order and sequencing;
- review loops for bounds, progress, and unsigned underflow;
- use `static`, `extern`, `const`, and `volatile` precisely;
- write type-correct formatted I/O;
- design warning, sanitizer, boundary-test, and target-test coverage.
