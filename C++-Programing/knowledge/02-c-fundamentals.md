# 02 - C Fundamentals

## 1. Goal

By the end of this chapter, you should be able to:

- read and write a small C program using a practical C17 baseline;
- distinguish `declaration`, `definition`, `initialization`, and `assignment`;
- choose data types based on value range and contract instead of assumed width;
- explain `scope`, `linkage`, `storage duration`, and `lifetime`;
- reason about `integer promotion`, the `usual arithmetic conversions`, and
  signed/unsigned bugs;
- use operators, control flow, loops, functions, and basic standard I/O
  correctly;
- recognize risks such as uninitialized reads, signed overflow, invalid shifts,
  format mismatches, and infinite loops;
- debug C fundamentals with compiler warnings, a debugger, and sanitizers.

This chapter uses C17 as the default working baseline. Important C23 differences
are noted where they matter.

## 2. Why This Matters

C has a small surface syntax, but its semantics are not small. Many production
bugs come from incorrect assumptions, not from missing punctuation:

- assuming `int` is always 32-bit;
- mixing signed and unsigned values without tracking conversions;
- reading a local variable before it is initialized;
- assuming signed overflow wraps;
- using `volatile` as if it were a synchronization mechanism;
- using the wrong format specifier in `printf`;
- confusing operator precedence with evaluation order.

In embedded systems, these mistakes can corrupt protocol fields, counters,
timeouts, bit masks, or state machines. In enterprise software, they create
bugs that depend on compiler, optimization level, platform, and boundary input.

A strong C foundation is not about memorizing syntax. It is about knowing what
type the compiler sees, how long an object exists, how expressions are
converted, and which behaviors the language actually guarantees.

## 3. Mental Model

Read a C program through five layers:

1. **Names and declarations**: what names exist, and what types do they have?
2. **Objects and lifetime**: what storage is created, and how long does it
   exist?
3. **Expressions and conversions**: what type is each computation performed in?
4. **Control flow**: which statements execute, and how does the loop or branch
   terminate?
5. **Program boundaries**: which functions are called, and how do values and
   errors move across the interface?

Example:

```c
#include <stdio.h>

static unsigned int sample_count = 0U;

static int read_sample(void)
{
    ++sample_count;
    return 42;
}

int main(void)
{
    const int value = read_sample();
    printf("value=%d, count=%u\n", value, sample_count);
    return 0;
}
```

In this example:

- `sample_count` has file scope, internal linkage, and static storage duration;
- `read_sample` is file-private because it is declared `static`;
- `value` has block scope and automatic storage duration;
- `const` prevents modification through the name `value`;
- `printf` requires format specifiers that match the argument types.

## 4. Program Structure And `main`

In a hosted C implementation, the two common `main` forms are:

```c
int main(void)
{
    return 0;
}
```

```c
int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    return 0;
}
```

`argc` is the number of command-line arguments. `argv` holds pointers to the
argument strings. The pointer and array mechanics behind this will be covered in
Chapter 04.

In hosted C, reaching the closing brace of `main` is equivalent to returning
`0`. Even so, writing `return 0;` is often clearer for learners and reviewers.

In a freestanding environment, the entry point is implementation-defined. Many
embedded toolchains still call a user `main` after startup code runs, but that
is not a universal hosted-language contract.

## 5. Statements, Expressions, And Objects

### 5.1 Expression And Statement

An `expression` computes a value, designates an object or function, or produces
a side effect:

```c
temperature + offset
counter++
is_ready && has_data
```

An `expression statement` is an expression followed by `;`:

```c
counter++;
update_state();
```

A block groups statements inside `{}`:

```c
if (is_ready) {
    process_data();
    ++processed_count;
}
```

### 5.2 Declaration, Definition, Initialization, Assignment

```c
extern int system_mode;  /* declaration */

int system_mode;         /* definition, zero-initialized */

int retry_count = 3;     /* definition + initialization */

void reset(void)
{
    retry_count = 0;     /* assignment */
}
```

| Concept | Meaning |
| --- | --- |
| `declaration` | Introduces a name and type |
| `definition` | Creates the object or provides the function body |
| `initialization` | Supplies the initial value when the object is defined |
| `assignment` | Changes the value of an existing object |

A declaration can also be a definition. The `extern` declaration above does not
allocate storage for `system_mode`; the definition without `extern` provides the
shared object.

### 5.3 Uninitialized Automatic Objects

```c
int main(void)
{
    int value;
    return value; /* unsafe: value is indeterminate */
}
```

A local automatic object without an initializer has an indeterminate value.
Reading it is unsafe and can trigger undefined behavior under the applicable C
rules.

Practical rule:

> Initialize every automatic object before its first read.

```c
int value = 0;
```

## 6. Fundamental Types

### 6.1 Integer Types

C provides these standard integer families:

```text
signed char
unsigned char
short / unsigned short
int / unsigned int
long / unsigned long
long long / unsigned long long
```

The language guarantees minimum ranges and rank relationships. It does not
guarantee universal byte widths.

```c
#include <limits.h>
#include <stdio.h>

int main(void)
{
    printf("CHAR_BIT=%d\n", CHAR_BIT);
    printf("sizeof(short)=%zu\n", sizeof(short));
    printf("sizeof(int)=%zu\n", sizeof(int));
    printf("sizeof(long)=%zu\n", sizeof(long));
    printf("INT_MIN=%d, INT_MAX=%d\n", INT_MIN, INT_MAX);
    return 0;
}
```

`sizeof(char)` is always `1`, but a C byte is not required to be exactly eight
bits. `CHAR_BIT` tells you how many bits the implementation uses for one byte.

`char`, `signed char`, and `unsigned char` are three distinct types. Whether
plain `char` behaves as signed or unsigned is implementation-defined.

### 6.2 Boolean Type

In C17:

```c
#include <stdbool.h>

bool is_ready = true;
```

`bool`, `true`, and `false` from `<stdbool.h>` map onto C's `_Bool` mechanism.
In C23, `bool`, `true`, and `false` became language-level names.

Do not assume that C `bool` matches C++ `bool` across every ABI boundary. When
designing a public binary interface, define the contract explicitly.

### 6.3 Floating-Point Types

C provides `float`, `double`, and `long double`.

```c
double ratio = 10.0 / 3.0;
```

Not every decimal value can be represented exactly in binary floating-point:

```c
#include <stdio.h>

int main(void)
{
    const double value = 0.1 + 0.2;
    printf("%.17f\n", value);
    return 0;
}
```

Exact equality is often the wrong test for computed floating-point results.
Choose tolerances by domain, not by a random global epsilon.

### 6.4 `size_t`

`size_t` is the unsigned integer type used for object sizes and the result type
of `sizeof`.

```c
#include <stdio.h>

int main(void)
{
    int samples[8] = {0};
    const size_t bytes = sizeof(samples);

    printf("bytes=%zu\n", bytes);
    return 0;
}
```

The correct format specifier for `size_t` is `%zu`, not `%d`.

### 6.5 Fixed-Width Integer Types

The `<stdint.h>` header provides these families:

- exact-width: `uint32_t`, `int16_t`;
- least-width: `uint_least32_t`;
- fast-width: `uint_fast32_t`;
- maximum-width: `uintmax_t`, `intmax_t`;
- pointer-capable: `uintptr_t`, `intptr_t` when supported.

Exact-width types are optional. `uint32_t` exists only if the implementation
has an unsigned integer type with exactly 32 value bits and no padding bits.

```c
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

int main(void)
{
    const uint32_t packet_id = UINT32_C(4000000000);
    printf("packet_id=%" PRIu32 "\n", packet_id);
    return 0;
}
```

Choose types based on the contract:

| Need | Suitable choice |
| --- | --- |
| General signed arithmetic | `int`, `long`, or a type chosen for the required range |
| Object size or count | `size_t` |
| Exactly 32-bit wire field | `uint32_t`, after verifying availability |
| At least 16 bits | `uint_least16_t` |
| A fast type with at least 16 bits | `uint_fast16_t` |

## 7. Scope, Linkage, Storage Duration, And Lifetime

These four concepts are related, but they are not the same.

| Concept | Question |
| --- | --- |
| `scope` | Where is the name visible? |
| `linkage` | Do declarations refer to the same entity? |
| `storage duration` | How long does the storage exist? |
| `lifetime` | When does the object validly exist for use? |

### 7.1 Automatic Local Object

```c
void process(void)
{
    int result = 0;
    /* result has block scope and automatic storage duration */
}
```

`result` is visible only inside the block, and its lifetime ends when block
execution ends.

### 7.2 Static Local Object

```c
unsigned int next_sequence(void)
{
    static unsigned int sequence = 0U;
    return ++sequence;
}
```

`sequence` has:

- block scope;
- static storage duration;
- a value that persists across calls;
- no linkage.

This creates hidden shared state. The function is not automatically thread-safe
or reentrant just because the state is hidden inside the function body.

### 7.3 File-Scope `static`

```c
static int current_mode = 0;

static void reset_mode(void)
{
    current_mode = 0;
}
```

At file scope, `static` gives the object or function internal linkage. The name
is linkable only inside the current translation unit.

### 7.4 `extern`

```c
/* status.h */
extern int system_status;
```

```c
/* status.c */
#include "status.h"

int system_status = 0;
```

```c
/* main.c */
#include "status.h"

int main(void)
{
    system_status = 1;
    return 0;
}
```

The header contains the declaration. One source file contains the definition.
Do not place an ordinary object definition in a header, or each translation unit
can end up with its own definition attempt.

### 7.5 `typedef`

`typedef` creates a type alias. It does not create a distinct new type.

```c
typedef unsigned long tick_count_t;

tick_count_t now = 0UL;
```

Aliases can express intent, but they do not create type safety between two
aliases that share the same underlying type.

### 7.6 `const`

```c
const unsigned int retry_limit = 3U;
```

`const` prevents modification through that lvalue. In C, a block-scope
`const int` does not automatically become an integer constant expression:

```c
void example(void)
{
    const int length = 8;
    int values[length]; /* VLA in C17, not a fixed constant-size array */
    (void)values;
}
```

This is an important difference from many C++ contexts.

### 7.7 `volatile`

```c
volatile unsigned int event_flag;
```

`volatile` requires the implementation to perform observable accesses in a way
that respects the abstract machine rules for volatile-qualified objects. It does
not guarantee:

- atomic reads or writes;
- mutual exclusion;
- inter-thread synchronization;
- freedom from data races;
- general memory ordering.

Deeper embedded uses of `volatile` belong in Chapter 06.

## 8. Conversions And Arithmetic

### 8.1 Integer Promotions

Before many arithmetic operations, integer types with rank no greater than
`int` are promoted:

- to `int` if `int` can represent all values of the original type;
- otherwise to `unsigned int`.

```c
#include <stdio.h>

int main(void)
{
    unsigned char left = 200U;
    unsigned char right = 20U;

    /* Both operands are commonly promoted to int before addition. */
    int sum = left + right;
    printf("%d\n", sum);
    return 0;
}
```

Do not model conversions as a simple ladder like
`char -> short -> int -> long -> float`. C uses rules based on rank, signedness,
and representable range.

### 8.2 Usual Arithmetic Conversions

For mixed arithmetic operands, C chooses a common real type for the calculation.

```c
int count = 3;
double scale = 0.5;
double result = count * scale; /* count converts to double */
```

Signed/unsigned interactions are a major bug source:

```c
#include <stdio.h>

int main(void)
{
    int error = -1;
    size_t count = 4U;

    if (error < count) {
        puts("unexpected on common platforms");
    }

    return 0;
}
```

In this comparison, `error` can be converted to an unsigned type and become a
very large value. Warnings such as `-Wsign-compare` and `-Wsign-conversion`
exist for a reason.

Fix the domain, not just the warning:

```c
if (error >= 0 && (size_t)error < count) {
    /* Range check makes the conversion meaningful. */
}
```

### 8.3 Integer Conversions

Conversion to an unsigned integer type produces the congruent value modulo one
more than the maximum representable value.

Conversion to a signed integer type, when the value is not representable, is
implementation-defined or can raise an implementation-defined signal.

```c
unsigned int wrapped = (unsigned int)-1;
```

That conversion is defined modulo arithmetic, but it is usually a poor choice
unless modular arithmetic is the real intent.

### 8.4 Floating-To-Integer Conversion

```c
double voltage = 3.99;
int whole = (int)voltage; /* 3: fractional part is discarded */
```

If the truncated integral result is not representable in the destination
integer type, the behavior is undefined. A cast does not make the conversion
safe; it only makes the intent explicit.

### 8.5 Overflow

Signed integer overflow is undefined behavior:

```c
#include <limits.h>

int value = INT_MAX;
value += 1; /* undefined behavior */
```

Unsigned arithmetic is modulo arithmetic:

```c
#include <limits.h>

unsigned int value = UINT_MAX;
value += 1U; /* defined: value becomes 0 */
```

Unsigned wrap is defined, but it can still be an application bug when it hits a
counter, allocation size, or timeout.

Checked signed addition:

```c
#include <limits.h>
#include <stdbool.h>

static bool add_int(int left, int right, int *result)
{
    if ((right > 0 && left > INT_MAX - right) ||
        (right < 0 && left < INT_MIN - right)) {
        return false;
    }

    *result = left + right;
    return true;
}
```

Pointer validity will be covered in depth in Chapter 04. For now, the contract
is that `result` must point to a writable `int`.

### 8.6 Division And Remainder

Integer division truncates toward zero:

```c
int quotient = -7 / 3;  /* -2 */
int remainder = -7 % 3; /* -1 */
```

When the quotient is representable:

```text
(a / b) * b + a % b == a
```

Division or remainder by zero is undefined behavior. `INT_MIN / -1` is also not
representable in `int`, so it must be guarded.

## 9. Operators And Sequencing

### 9.1 Operator Families

| Family | Operators |
| --- | --- |
| Arithmetic | `+ - * / %` |
| Relational | `< <= > >= == !=` |
| Logical | `&& || !` |
| Bitwise | `& | ^ ~ << >>` |
| Assignment | `= += -= *= /= %= &= |= ^= <<= >>=` |
| Conditional | `condition ? true_value : false_value` |
| Size | `sizeof` |

### 9.2 Logical Vs Bitwise

```c
if (pointer != NULL && *pointer > 0) {
    /* RHS is evaluated only when pointer is not NULL. */
}
```

`&&` and `||` short-circuit. `&` and `|` are bitwise operators and evaluate
both operands.

| Operators | Purpose | Short-circuit |
| --- | --- | --- |
| `&&`, `||` | Logical conditions | Yes |
| `&`, `|` | Bit manipulation | No |

### 9.3 Assignment In Conditions

This is valid C:

```c
if (status = read_status()) {
    /* Tests the assigned value, probably unintended. */
}
```

If the assignment is intentional, make that explicit:

```c
if ((status = read_status()) != 0) {
    /* deliberate assignment */
}
```

Warnings are extremely useful for catching accidental `=` where `==` was
intended.

### 9.4 Precedence Is Not Evaluation Order

Expression:

```c
result = left() + right() * scale();
```

Precedence says multiplication groups before addition. It does not guarantee
that `left()`, `right()`, and `scale()` are called left-to-right.

Avoid code that depends on unspecified evaluation order:

```c
int left_value = left();
int right_value = right();
int scale_value = scale();
int result = left_value + right_value * scale_value;
```

### 9.5 Unsequenced Modification

```c
int i = 1;
int value = i++ + i; /* undefined behavior */
```

If an object is modified and also read for another purpose without proper
sequencing, the result can be undefined behavior.

### 9.6 Bitwise Operations And Shifts

Prefer unsigned types for bit manipulation:

```c
#include <stdint.h>

enum {
    STATUS_READY = 1U << 0,
    STATUS_ERROR = 1U << 1,
    STATUS_BUSY  = 1U << 2
};

uint32_t status = 0U;
status |= STATUS_READY;
status &= ~((uint32_t)STATUS_BUSY);

if ((status & STATUS_ERROR) != 0U) {
    /* handle error */
}
```

A negative shift count, or a shift count greater than or equal to the width of
the promoted left operand, is undefined behavior.

```c
#include <limits.h>
#include <stdbool.h>

static bool set_bit(unsigned int *value, unsigned int bit)
{
    const unsigned int width =
        (unsigned int)(sizeof(*value) * CHAR_BIT);

    if (bit >= width) {
        return false;
    }

    *value |= 1U << bit;
    return true;
}
```

Right shift of a negative signed value is implementation-defined. Left shift of
a signed value has additional representability constraints. Use unsigned masks
for bit-level work.

## 10. Control Flow

### 10.1 `if` And `else`

```c
if (temperature > high_limit) {
    state = STATE_HIGH;
} else if (temperature < low_limit) {
    state = STATE_LOW;
} else {
    state = STATE_NORMAL;
}
```

Conditions accept scalar expressions. Zero is false; nonzero is true.

### 10.2 `switch`

```c
switch (command) {
    case 0:
        stop();
        break;

    case 1:
        start();
        break;

    case 2:
    case 3:
        enter_service_mode();
        break;

    default:
        report_invalid_command();
        break;
}
```

Case labels must be unique integer constant expressions after conversion.
Fallthrough can be intentional, but it must be made clear. `switch` is not
automatically faster than `if`/`else`; the compiler chooses the code generation
strategy based on the target and value distribution.

### 10.3 Loops

`for` works well when the iteration state is explicit:

```c
for (size_t index = 0U; index < sample_count; ++index) {
    process_sample(index);
}
```

`while` is a good fit for condition-driven repetition:

```c
while (queue_has_data()) {
    process_next_item();
}
```

`do while` executes at least once:

```c
do {
    status = poll_device();
} while (status == STATUS_RETRY);
```

### 10.4 Unsigned Countdown Trap

Wrong:

```c
for (size_t index = count - 1U; index >= 0U; --index) {
    process(index);
}
```

`index >= 0U` is always true because `size_t` is unsigned. Safe version:

```c
for (size_t index = count; index-- > 0U;) {
    process(index);
}
```

Or use forward iteration if the algorithm allows it.

### 10.5 `break`, `continue`, And `goto`

- `break` exits the nearest loop or `switch`;
- `continue` jumps to the next iteration of the nearest loop;
- `goto` should be avoided in ordinary control flow.

In C, `goto` can still be useful for centralized cleanup:

```c
int run_job(void)
{
    int status = -1;
    FILE *input = fopen("input.txt", "r");

    if (input == NULL) {
        return -1;
    }

    FILE *output = fopen("output.txt", "w");
    if (output == NULL) {
        goto cleanup_input;
    }

    status = process_files(input, output);
    fclose(output);

cleanup_input:
    fclose(input);
    return status;
}
```

This is controlled cleanup, not a license for arbitrary jumps.

## 11. Functions

### 11.1 Prototype And Definition

```c
/* declaration with prototype */
int clamp(int value, int minimum, int maximum);

/* definition */
int clamp(int value, int minimum, int maximum)
{
    if (value < minimum) {
        return minimum;
    }

    if (value > maximum) {
        return maximum;
    }

    return value;
}
```

The prototype lets the compiler check argument count and argument types. Avoid
old-style non-prototype declarations.

### 11.2 Pass By Value

C passes arguments by value:

```c
static void increment_copy(int value)
{
    ++value;
}
```

That function does not modify the caller's object. To modify caller-owned
state, pass a pointer:

```c
static void increment(int *value)
{
    if (value != NULL) {
        ++(*value);
    }
}
```

Ownership, pointer validity, and lifetime will be covered in Chapter 04.

### 11.3 File-Private Functions

```c
static int validate_range(int value)
{
    return value >= 0 && value <= 100;
}
```

A file-scope `static` function has internal linkage. This is the standard way
to keep implementation helpers private to the translation unit.

### 11.4 Recursion

```c
static unsigned int factorial(unsigned int value)
{
    if (value <= 1U) {
        return 1U;
    }

    return value * factorial(value - 1U);
}
```

This example has a base case, but it still carries two risks:

- the multiplication can overflow;
- every active call consumes stack space.

Tail-call optimization is not guaranteed by the C standard. In embedded or
safety-sensitive systems, use recursion only when the maximum depth and stack
budget are proven.

## 12. Basic Standard I/O

### 12.1 Output

```c
#include <stdio.h>

int main(void)
{
    int temperature = -5;
    unsigned int retries = 3U;
    size_t bytes = sizeof temperature;

    printf("temperature=%d\n", temperature);
    printf("retries=%u\n", retries);
    printf("bytes=%zu\n", bytes);
    return 0;
}
```

Format mismatches can cause undefined behavior:

```c
size_t count = 10U;
printf("%d\n", count); /* wrong */
```

### 12.2 Checked Input

`scanf` has a lot of sharp edges. For line-oriented input, `fgets` plus checked
parsing is often easier to control:

```c
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    char buffer[64];

    if (fgets(buffer, sizeof buffer, stdin) == NULL) {
        fputs("input error\n", stderr);
        return 1;
    }

    errno = 0;
    char *end = NULL;
    long value = strtol(buffer, &end, 10);

    if (errno != 0 || end == buffer ||
        (*end != '\n' && *end != '\0') ||
        value < INT_MIN || value > INT_MAX) {
        fputs("invalid integer\n", stderr);
        return 1;
    }

    printf("value=%d\n", (int)value);
    return 0;
}
```

Never use `gets`; it was removed from the C standard because it cannot limit
input length safely.

## 13. Practical Usage

### 13.1 Small Sensor Classification

```c
#include <stdbool.h>
#include <stdio.h>

typedef enum {
    SENSOR_OK,
    SENSOR_LOW,
    SENSOR_HIGH
} sensor_state_t;

static sensor_state_t classify_sample(
    int sample,
    int minimum,
    int maximum)
{
    if (sample < minimum) {
        return SENSOR_LOW;
    }

    if (sample > maximum) {
        return SENSOR_HIGH;
    }

    return SENSOR_OK;
}

int main(void)
{
    const int sample = 42;
    const sensor_state_t state = classify_sample(sample, 10, 80);
    const bool is_valid = state == SENSOR_OK;

    printf("sample=%d, valid=%s\n",
           sample,
           is_valid ? "true" : "false");
    return is_valid ? 0 : 1;
}
```

Build:

```bash
cc -std=c17 -Wall -Wextra -Wpedantic \
  -Wconversion -Wsign-conversion sensor.c -o sensor
./sensor
```

### 13.2 Flag Operations

```c
#include <stdbool.h>
#include <stdint.h>

enum {
    FLAG_READY = UINT32_C(1) << 0,
    FLAG_ERROR = UINT32_C(1) << 1
};

static bool has_flag(uint32_t value, uint32_t mask)
{
    return (value & mask) != UINT32_C(0);
}
```

This is learning-oriented code. A production API would also define valid mask
contracts, thread-safety expectations, concurrency rules, and ownership of any
shared state.

## 14. C And C++ Comparisons

| Topic | C | C++ | Practical note |
| --- | --- | --- | --- |
| Boolean | `_Bool`; C17 typically uses `<stdbool.h>` | Built-in `bool` | Do not assume ABI identity |
| Cast | `(type)expression` | Named casts such as `static_cast` | A C cast is explicit intent, not proof of safety |
| Input/output | `<stdio.h>`, format strings | iostreams or formatting libraries | In C, a format mismatch can be UB |
| `const` integer | Not automatically an integer constant expression | More constant-expression contexts | In C, an array declaration can become a VLA |
| `static` | Storage duration or internal linkage depending on context | Also has class-member meanings | Always identify the context first |
| `extern` | Usually for cross-file declarations | Similar, plus language-linkage contexts | One shared object still needs one definition |
| `volatile` | Observable access, not synchronization | Also not a replacement for atomics | Use for a narrow, documented purpose |
| Function parameters | Pass by value; use pointers to modify caller state | Value, pointer, or reference | C has no references |
| Function names | No overloading by parameter types | Overloading is supported | C API names must be unique |

This chapter is C-first. The C++ column exists to prevent accidental mixing of
syntax and semantics between the two languages.

## 15. Common Bugs

### 15.1 Uninitialized Read

```c
int status;
if (status == 0) { /* undefined behavior risk */
}
```

Fix: initialize the object or read it only after a guaranteed write.

### 15.2 Signed/Unsigned Comparison

```c
int index = -1;
size_t length = 8U;

if (index < length) {
    /* may unexpectedly be false */
}
```

Fix: validate the nonnegative range before converting.

### 15.3 Signed Overflow

```c
int next = current + increment;
```

Fix: prove the range or check it before the arithmetic.

### 15.4 Invalid Shift

```c
unsigned int mask = 1U << bit;
```

Fix: verify `bit < sizeof(mask) * CHAR_BIT`.

### 15.5 Format Mismatch

```c
uint32_t value = 1U;
printf("%u\n", value);
```

`uint32_t` is not required to be `unsigned int`. Use `<inttypes.h>` macros:

```c
printf("%" PRIu32 "\n", value);
```

### 15.6 Accidental Assignment

```c
if (ready = 1) {
}
```

Fix: use `==`, enable warnings, and keep conditions simple.

### 15.7 Missing `break`

```c
switch (state) {
    case STATE_IDLE:
        start();
        /* accidental fallthrough */
    case STATE_RUNNING:
        monitor();
        break;
}
```

Fix: add `break` or mark intentional fallthrough according to your
compiler/language policy.

### 15.8 Unsigned Infinite Loop

```c
for (size_t i = count - 1U; i >= 0U; --i) {
}
```

Fix: use a condition that is valid for unsigned countdown logic.

### 15.9 Static Hidden State

```c
int next_id(void)
{
    static int id = 0;
    return ++id;
}
```

Risks: not reentrant, data races in multi-threaded use, hard-to-reset test
state, and eventual overflow.

### 15.10 Misusing `volatile`

```c
volatile int lock = 0;
```

This is not a mutex and not an atomic lock. Concurrency belongs in Chapter 14.

## 16. Debugging Workflow

### 16.1 Start With Warnings

```bash
cc -std=c17 -Wall -Wextra -Wpedantic \
  -Wconversion -Wsign-conversion -Wshadow -Wformat=2 \
  program.c -o program
```

Do not cast just to silence a warning. Instead ask:

- what is the source type?
- what is the destination type?
- does the value range fit?
- does the signedness change?
- is the conversion part of the API contract?

### 16.2 Debug Build

```bash
cc -std=c17 -O0 -g3 -Wall -Wextra -Wpedantic \
  program.c -o program-debug
gdb ./program-debug
```

Useful GDB commands:

```text
break main
run
next
step
print variable
ptype variable
backtrace
```

### 16.3 Sanitizers

On supported hosted toolchains:

```bash
cc -std=c17 -O1 -g \
  -fsanitize=address,undefined \
  -fno-omit-frame-pointer \
  program.c -o program-sanitize
./program-sanitize
```

UBSan can catch issues such as signed overflow and invalid shifts. ASan becomes
more important once memory-heavy topics arrive later.

Sanitizers do not replace:

- compiler warnings;
- boundary tests;
- static analysis;
- testing on the actual embedded target.

### 16.4 Boundary Testing

For integer-heavy and control-flow-heavy code, test at least:

- zero;
- one;
- minimum valid value;
- maximum valid value;
- one below and one above each boundary;
- negative input when the type or domain allows it;
- empty input and malformed input;
- loop count equal to zero;
- overflow-adjacent values.

## 17. Best Practices

- Initialize automatic objects before the first read.
- Choose types based on value range and interface contract.
- Do not assume `int` is 32-bit or that a byte is 8 bits.
- Use `size_t` for object sizes and counts, but handle signed interoperability
  deliberately.
- Use unsigned types for bit manipulation; do not use unsigned everywhere just
  because a value "should not be negative" if the algorithm needs subtraction or
  error sentinels.
- Use exact-width types only when exact width is part of the contract.
- Range-check before a narrowing conversion.
- Do not rely on signed overflow, negative signed shifts, or unspecified
  evaluation order.
- Keep expressions and conditions simple, with limited side effects.
- Write loops with a clear invariant, bound, and termination path.
- Place function prototypes in self-contained headers.
- Use file-scope `static` for private helpers, but limit mutable global state.
- Check return values from standard-library and fallible functions.
- Prefer `fgets` plus checked parsing for text input.
- Keep recursion bounded and backed by stack analysis in embedded code.
- Do not use `volatile` as an atomic or synchronization primitive.
- Build with an explicit language standard and a strong warning policy.

## 18. Interview Readiness

### 18.1 Scope Vs Lifetime

**Short answer:** `scope` is the region of source code where a name can be
looked up. `lifetime` is the period during which the object validly exists.

**Example:** a static local has block scope but lives for the entire program.

### 18.2 Static Local Vs Global Variable

Static local:

- its name is visible only in the block;
- it has static storage duration;
- it keeps its value across calls.

File-scope object:

- it has file scope;
- it has static storage duration;
- it has external linkage by default, or internal linkage if declared `static`.

### 18.3 What Does `extern` Do?

`extern` usually declares that the entity is defined elsewhere and has external
linkage. The program still needs one correct definition under the applicable
rules.

### 18.4 Why `uint32_t` Instead Of `int`?

Use `uint32_t` when the contract requires an unsigned exact 32-bit
representation, such as a wire format or a hardware-defined field, and the
implementation provides that type.

Do not use it just because "32-bit looks clearer" when the algorithm only needs
general integer behavior. In that case, `int`, `uint_least32_t`, or `size_t`
may be a better fit.

### 18.5 Signed Vs Unsigned Bug

In a mixed comparison, the signed operand can be converted to unsigned depending
on rank and representable range. A negative value can then become a huge
unsigned value and completely change the result of the condition.

### 18.6 What Is Integer Promotion?

Integer types with rank no greater than `int` are promoted to `int` if all
their values fit. Otherwise they are promoted to `unsigned int`, in the
contexts where integer promotions apply.

### 18.7 Why Is Signed Overflow Undefined?

C allows the compiler to assume signed overflow does not happen in a valid
program. That freedom enables optimization. Correct code must prove the range
or guard the operation first. Unsigned arithmetic has modulo semantics, but wrap
can still violate business logic.

### 18.8 Precedence Vs Evaluation Order

Precedence determines how an expression is grouped when parsed. Evaluation order
determines when subexpressions run. Higher precedence does not mean "evaluated
earlier."

### 18.9 What Does `volatile` Guarantee?

It affects observable accesses to the volatile-qualified object. It does not
guarantee atomicity, race freedom, synchronization, or mutual exclusion.

## 19. Practice

### Basic

1. Write a program that prints `CHAR_BIT`, the sizes, and the limits of the
   fundamental integer types.
2. Create one example each for declaration, definition, initialization, and
   assignment.
3. Write the same counter using `for`, `while`, and `do while`.
4. Write a `switch` that classifies a command and handles invalid values.
5. Use `printf` correctly for `int`, `unsigned int`, `size_t`, and `uint32_t`.

### Intermediate

1. Build a two-source-file project that demonstrates one `extern` object and
   file-private `static` helpers.
2. Create a signed/unsigned comparison bug, compile with
   `-Wsign-conversion`, then fix it with explicit domain validation.
3. Implement checked addition for `int`.
4. Write a flag API with set, clear, toggle, and test operations, including
   guarded shift counts.
5. Read an integer with `fgets` and `strtol`, rejecting malformed or
   out-of-range input.
6. Write a safe reverse loop using `size_t`.

### Advanced

1. Review a module that contains an uninitialized read, signed overflow,
   invalid shift, format mismatch, accidental assignment, and missing `break`.
2. Compare recursive and iterative factorial, add overflow checks, then analyze
   stack risk.
3. Design a type policy for a sensor reading, sample count, timestamp, and
   32-bit protocol field.
4. Run warnings and UBSan on intentional failure cases, then explain the
   compiler and runtime diagnostics.

## 20. Summary

- C fundamentals are about the semantics of types, objects, expressions,
  conversions, and control flow, not just syntax.
- `scope`, `linkage`, `storage duration`, and `lifetime` must be kept separate.
- Fundamental integer widths are implementation-defined; exact-width types are
  optional.
- Integer promotions and the usual arithmetic conversions explain many
  signed/unsigned bugs.
- Signed overflow is undefined behavior; unsigned wrap is defined modulo
  arithmetic.
- Precedence does not determine evaluation order.
- Bit manipulation should use unsigned types and validated shift counts.
- Function prototypes, checked return values, and correct format specifiers are
  basic production habits.
- `volatile` does not replace atomics or synchronization.
- Strong warnings, boundary tests, a debugger, and sanitizers turn language
  rules into a practical engineering workflow.

## Reference Notes

- ISO/IEC 9899:2024 working draft, WG14 N3220:
  <https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf>
- C implicit conversions:
  <https://en.cppreference.com/w/c/language/conversion>
- Fixed-width integer types:
  <https://en.cppreference.com/w/c/types/integer>
- C arithmetic operators:
  <https://en.cppreference.com/w/c/language/operator_arithmetic>
- C `main` function:
  <https://en.cppreference.com/w/c/language/main_function>
