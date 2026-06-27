# 05 - Compound Types In C

## 1. Goal

By the end of this chapter, you should be able to:

- explain how arrays, strings, structures, unions, and enumerations represent
  data in C;
- distinguish an array object from a pointer and explain array-to-pointer
  conversion;
- pass one-dimensional and multidimensional arrays to functions with explicit
  bounds;
- distinguish a character array, a byte buffer, a string literal, and a valid
  null-terminated C string;
- use `strlen`, `strcmp`, `fgets`, and `snprintf` with correct preconditions;
- explain why `strcpy`, `strcat`, `strncpy`, and `gets` require special care or
  avoidance;
- define, initialize, copy, and access nested and self-referential structures;
- calculate structure padding and inspect alignment and member offsets;
- explain how union members overlap and maintain a correct tagged union;
- use enums for states and tags without assuming a fixed size or accepting
  arbitrary external values;
- use `typedef`, designated initializers, compound literals, flexible array
  members, and bit-fields appropriately;
- explain why native structure, union, packed, and bit-field layouts are not
  portable wire formats;
- compare C compound types with `std::array`, `std::vector`, `std::string`,
  `std::string_view`, `enum class`, and `std::variant`;
- debug bounds, termination, padding, alignment, union-tag, and representation
  defects;
- answer compound-type interview questions from junior through senior level.

This chapter uses C17 as its practical baseline. C23 additions are labeled
explicitly.

This is a `MUST` topic at `Deep` depth. Chapter 04, Pointer Mastery, is the
prerequisite.

## 2. Why This Chapter Matters

Most useful data is compound:

- a temperature history is an array;
- a device name is a character buffer or string;
- a sensor sample is a structure;
- a command payload may be a tagged union;
- a device state is an enumeration;
- a variable-length message may use a flexible array member.

The syntax is straightforward. The difficult part is preserving the data
contract:

| Type | Essential contract |
| --- | --- |
| Array | Element type, extent, and valid index range |
| C string | Buffer capacity, current length, and null terminator |
| Structure | Member types, layout assumptions, and object lifetime |
| Union | Which member currently represents the stored value |
| Enum | Which values are valid and how unknown values are handled |
| Binary record | Exact field widths, byte order, alignment, and encoding |

Many serious C defects are broken compound-type contracts:

- writing element `count` of an array containing `count` elements;
- allocating space for text but forgetting the terminating `'\0'`;
- using `sizeof` on an adjusted array parameter;
- transmitting structure padding as if it were protocol data;
- reading a union member that does not match its tag;
- assuming an enum occupies four bytes;
- mapping bit-fields directly onto an externally defined binary format.

For embedded software, these rules affect RAM usage, deterministic buffers,
state machines, protocol parsers, configuration data, and binary interfaces.

For enterprise software, they affect public API design, input validation,
security review, portability, serialization, logging, and maintenance.

The central rule is:

> Keep the data representation together with the information required to
> interpret it safely: extent, capacity, terminator, tag, layout contract, and
> lifetime.

## 3. Mental Model: Shape, Layout, And Interpretation

Compound types answer three different questions.

### 3.1 Shape

Shape describes how values are grouped.

```text
array:   [value][value][value][value]

struct:  [member A][padding?][member B][member C][tail padding?]

union:   [------ shared storage used by one representation ------]
```

- An array repeats one element type.
- A structure combines named member objects that coexist.
- A union provides overlapping alternatives.

### 3.2 Layout

Layout describes where the component objects appear in memory.

- Array elements are contiguous with no padding between elements.
- Structure members follow declaration order, but padding can appear between
  members and at the end.
- Union members begin in overlapping storage, and the union is large and
  aligned enough for its members.

Layout is partly controlled by the language and partly by the implementation's
ABI and alignment rules.

### 3.3 Interpretation

Bytes do not explain themselves. Code needs a valid interpretation:

- an array needs an element type and extent;
- a C string needs a terminating zero character;
- a union needs an active-member invariant, normally represented by a tag;
- serialized bytes need defined widths, byte order, and encoding.

Consider these six bytes:

```text
48 65 6c 6c 6f 00
```

They can represent the C string `"Hello"` only when:

- they belong to a live character array;
- the code is permitted to read them;
- the sixth byte is the terminating `'\0'`;
- the consumer uses a compatible character encoding assumption.

The same memory can be a byte buffer without being treated as text.

## 4. Arrays

### 4.1 An array is an object, not a pointer

```c
#include <stddef.h>
#include <stdio.h>

int main(void)
{
    int samples[] = {21, 22, 24, 23};
    size_t count = sizeof samples / sizeof samples[0];

    printf("count = %zu\n", count);
    return 0;
}
```

`samples` is an array object containing four `int` elements. Its type includes
its extent.

```text
samples
+------+------+------+------+
|  21  |  22  |  24  |  23  |
+------+------+------+------+
   0      1      2      3
```

The valid indices are `0` through `count - 1`.

The expression:

```c
sizeof samples
```

produces the size of the whole array because `samples` has not converted to a
pointer in this context.

### 4.2 Array-to-pointer conversion

In most expressions, an array expression converts to a pointer to its first
element:

```c
int values[] = {10, 20, 30};
int *first = values;
```

After conversion:

```c
first == &values[0]
```

This does not mean that the array is a pointer. Their properties differ:

| Property | Array object | Pointer object |
| --- | --- | --- |
| Owns elements | Yes | No |
| Extent is part of type | Yes | No |
| `sizeof` in defining scope | Whole array | Pointer size |
| Assignable | No | Yes |
| Can be reseated | No | Yes |

Important contexts where an array does not undergo the usual conversion include
use as the operand of `sizeof` and unary `&`.

```c
int values[4] = {0};

int *element_ptr = values;       /* pointer to first int */
int (*array_ptr)[4] = &values;   /* pointer to the whole array */
```

The pointer values refer to the same starting location, but their types and
arithmetic differ:

```text
element_ptr + 1  advances by sizeof(int)
array_ptr + 1    advances by sizeof(int[4])
```

### 4.3 Initialization

```c
int exact[4] = {1, 2, 3, 4};
int partial[4] = {1, 2};       /* {1, 2, 0, 0} */
int zeroed[4] = {0};           /* all elements become zero */
int inferred[] = {5, 6, 7};    /* extent is 3 */
```

An automatic array with no initializer contains indeterminate element values:

```c
int unsafe[4];
```

Do not read those elements before assigning values to them.

Designated initializers can select array elements:

```c
int status_codes[8] = {
    [0] = 200,
    [3] = 404,
    [7] = 500
};
```

Unspecified elements are initialized to zero.

### 4.4 Traversal with an explicit extent

```c
#include <stddef.h>
#include <stdio.h>

static int sum(const int *values, size_t count)
{
    int total = 0;

    for (size_t i = 0; i < count; ++i) {
        total += values[i];
    }

    return total;
}

int main(void)
{
    int values[] = {3, 5, 7, 9};
    size_t count = sizeof values / sizeof values[0];

    printf("%d\n", sum(values, count));
    return 0;
}
```

The function receives a pointer and a count. That pair is the range contract.

Use `size_t` for sizes and element counts. If an algorithm needs to move
backward past zero, design the loop carefully because `size_t` is unsigned.

### 4.5 Array parameters are adjusted to pointers

These declarations describe the same parameter type:

```c
void process(int values[]);
void process(int values[10]);
void process(int *values);
```

Inside the function, `values` is a pointer:

```c
void broken_count(int values[10])
{
    /* Wrong: sizeof values is sizeof(int *), not sizeof(int[10]). */
    size_t count = sizeof values / sizeof values[0];
    (void)count;
}
```

Pass the extent separately:

```c
void process(int *values, size_t count);
```

For a read-only range:

```c
int sum(const int *values, size_t count);
```

C also permits a minimum-bound contract in a parameter:

```c
void normalize(float values[static 16]);
```

At each call, the argument must provide access to at least 16 elements. This is
a caller contract, not automatic runtime bounds checking.

### 4.6 Multidimensional arrays

A two-dimensional array is an array of arrays:

```c
int matrix[2][3] = {
    {1, 2, 3},
    {4, 5, 6}
};
```

Its layout is contiguous and row-major:

```text
matrix[0]              matrix[1]
+---+---+---+          +---+---+---+
| 1 | 2 | 3 |          | 4 | 5 | 6 |
+---+---+---+          +---+---+---+

Complete storage: [1][2][3][4][5][6]
```

The compiler needs the row type to calculate `matrix[row][column]`.

```c
#include <stddef.h>
#include <stdio.h>

enum { COLUMN_COUNT = 3 };

static void print_matrix(
    size_t rows,
    const int matrix[][COLUMN_COUNT])
{
    for (size_t row = 0; row < rows; ++row) {
        for (size_t column = 0; column < COLUMN_COUNT; ++column) {
            printf("%d%c",
                   matrix[row][column],
                   column + 1U == COLUMN_COUNT ? '\n' : ' ');
        }
    }
}

int main(void)
{
    int matrix[2][COLUMN_COUNT] = {
        {1, 2, 3},
        {4, 5, 6}
    };

    print_matrix(2U, matrix);
    return 0;
}
```

The adjusted parameter is a pointer to an array of `COLUMN_COUNT` integers:

```c
const int (*)[COLUMN_COUNT]
```

C99 VLAs can express runtime dimensions:

```c
void clear_matrix(size_t rows, size_t columns,
                  int matrix[rows][columns])
{
    for (size_t row = 0; row < rows; ++row) {
        for (size_t column = 0; column < columns; ++column) {
            matrix[row][column] = 0;
        }
    }
}
```

VLA availability is version- and implementation-sensitive after C11. Check the
project's language baseline and compiler support before making VLAs part of a
public interface.

## 5. Character Arrays And C Strings

### 5.1 Character array versus C string

These are both character arrays:

```c
char bytes[5] = {'H', 'e', 'l', 'l', 'o'};
char text[6] = {'H', 'e', 'l', 'l', 'o', '\0'};
```

Only `text` is a valid C string.

```text
bytes: [H][e][l][l][o]       no terminator
text:  [H][e][l][l][o][\0]   valid C string
```

A C string is not a separate language type. It is a convention:

> A C string is a character sequence whose end is marked by a null character.

String-literal initialization adds the terminator:

```c
char greeting[] = "Hello";   /* six char elements */
```

Keep these quantities distinct:

```text
capacity       total elements in the destination array
length         characters before the first '\0'
required size  length + 1 for the terminating '\0'
```

### 5.2 String literals are not mutable storage

```c
const char *label = "ready";
char editable[] = "ready";
```

- `label` points to a string literal. Do not modify it.
- `editable` is a separate array initialized from the literal and can be
  modified within bounds.

This is wrong:

```c
char *label = "ready";
label[0] = 'R'; /* undefined behavior */
```

The C type system historically permits assigning a string literal to `char *`,
but attempting to modify the literal has undefined behavior. Use `const char *`
for read-only access.

### 5.3 `strlen` measures length, not capacity

```c
#include <stdio.h>
#include <string.h>

int main(void)
{
    char text[16] = "sensor";

    printf("length = %zu\n", strlen(text));
    printf("capacity = %zu\n", sizeof text);
    return 0;
}
```

Output:

```text
length = 6
capacity = 16
```

`strlen` scans until it finds `'\0'`. Its argument must already point to a
valid null-terminated string. It cannot safely discover whether an arbitrary
buffer has a terminator.

### 5.4 Comparing strings

The `==` operator compares pointer values after array-to-pointer conversion:

```c
char first[] = "ok";
char second[] = "ok";

if (first == second) { /* compares addresses, not text */ }
```

Use `strcmp` for lexical content comparison:

```c
#include <string.h>

if (strcmp(first, second) == 0) {
    /* equal contents */
}
```

Interpret the sign, not a particular nonzero magnitude:

```text
strcmp(a, b) < 0   a sorts before b
strcmp(a, b) == 0  equal strings
strcmp(a, b) > 0   a sorts after b
```

Both arguments must be valid strings.

### 5.5 Why `strcpy` and `strcat` are dangerous interfaces

`strcpy` does not receive destination capacity:

```c
char destination[8];
strcpy(destination, source);
```

The operation is valid only if:

```text
strlen(source) + 1 <= sizeof destination
```

`strcat` needs enough capacity for both existing text and appended text:

```text
strlen(destination) + strlen(source) + 1 <= capacity
```

If the proof is wrong, the function writes outside the destination, producing
undefined behavior. A crash is only one possible result.

In production code, prefer interfaces that carry capacity and report failure.

### 5.6 `strncpy` is not a general safe replacement

`strncpy(destination, source, count)` has two surprising behaviors:

1. If the source length is at least `count`, the destination is not
   null-terminated by the function.
2. If the source is shorter than `count`, the remaining destination elements
   are filled with zero characters.

This code deliberately reserves one byte for the terminator:

```c
char destination[8];

strncpy(destination, source, sizeof destination - 1U);
destination[sizeof destination - 1U] = '\0';
```

It can be correct for a truncating-copy policy, but the policy must be explicit.
It also scans and pads according to `strncpy` semantics, which may not match the
actual requirement.

Do not label a function safe merely because its name contains `n`.

### 5.7 Bounded formatting with `snprintf`

`snprintf` is useful for text construction because it receives destination
capacity and reports the required output length.

```c
#include <stdio.h>

int main(void)
{
    char message[32];
    int written = snprintf(message, sizeof message,
                           "sensor=%u value=%d", 7U, 42);

    if (written < 0) {
        fputs("formatting error\n", stderr);
        return 1;
    }

    if ((size_t)written >= sizeof message) {
        fputs("message was truncated\n", stderr);
        return 1;
    }

    puts(message);
    return 0;
}
```

For a successful call, the return value is the number of characters that would
have been produced, excluding the terminator. Therefore:

```text
result < 0                 formatting or encoding error
(size_t)result >= capacity output did not fully fit
otherwise                  output fit and is terminated
```

### 5.8 Bounded input with `fgets`

```c
#include <stdio.h>
#include <string.h>

int main(void)
{
    char line[64];

    if (fgets(line, sizeof line, stdin) == NULL) {
        if (ferror(stdin)) {
            fputs("input error\n", stderr);
            return 1;
        }
        return 0; /* end of input */
    }

    line[strcspn(line, "\n")] = '\0';
    printf("input: %s\n", line);
    return 0;
}
```

`fgets` reads at most one fewer character than the supplied count and adds a
terminator when it stores input successfully. A newline is retained if it fits.

A complete input policy may also need to detect a line longer than the buffer
and consume or reject the remaining characters.

`gets` has no capacity argument. It cannot prevent an input-dependent buffer
overflow and was removed from the C standard library in C11. Never use it.

### 5.9 Byte buffers are not automatically strings

Protocol payloads, file blocks, image data, and encrypted data can contain zero
bytes anywhere or no zero byte at all.

```c
unsigned char payload[8];
```

Do not pass arbitrary binary data to `strlen`, `printf("%s", ...)`, `strcpy`,
or another string API. Use an explicit byte count.

## 6. Structures

### 6.1 Grouping related objects

```c
#include <stdint.h>

struct SensorSample {
    uint32_t timestamp_ms;
    int16_t temperature_tenths;
    uint8_t status;
};
```

Each member is a separate object, and all members can hold values
simultaneously.

```c
struct SensorSample sample = {
    .timestamp_ms = 1250U,
    .temperature_tenths = 237,
    .status = 1U
};
```

C designated initializers improve readability and reduce dependence on visual
member counting.

Access direct members with `.`:

```c
sample.status = 2U;
```

Access members through a pointer with `->`:

```c
struct SensorSample *sample_ptr = &sample;
sample_ptr->status = 3U;
```

`sample_ptr->status` means `(*sample_ptr).status`.

### 6.2 Tags and `typedef`

The structure tag is written after `struct`:

```c
struct Point {
    int x;
    int y;
};

struct Point origin = {0, 0};
```

C requires `struct Point` unless an ordinary type alias is introduced:

```c
typedef struct Point Point;

Point target = {.x = 10, .y = 20};
```

A common style defines the tag and alias together:

```c
typedef struct DeviceConfig {
    unsigned int timeout_ms;
    int diagnostics_enabled;
} DeviceConfig;
```

`typedef` creates an alias. It does not create a distinct type.

Avoid hiding pointers:

```c
typedef struct Device *DevicePtr; /* ownership and const become less obvious */
```

Prefer an explicit pointer at the use site:

```c
typedef struct Device Device;
Device *device;
```

### 6.3 Nested structures

```c
typedef struct {
    int year;
    int month;
    int day;
} Date;

typedef struct {
    unsigned int employee_id;
    Date start_date;
} Employee;

Employee engineer = {
    .employee_id = 42U,
    .start_date = {
        .year = 2026,
        .month = 6,
        .day = 13
    }
};
```

Nested aggregates model composition directly. Their nested members also affect
alignment and total structure size.

### 6.4 Structure assignment is a value operation

Structures of compatible type can be assigned and returned by value:

```c
typedef struct {
    int x;
    int y;
} Point;

static Point translated(Point point, int dx, int dy)
{
    point.x += dx;
    point.y += dy;
    return point;
}
```

Structure assignment copies member values. If a member is a pointer, the
pointer value is copied, not the pointed-to allocation:

```c
typedef struct {
    char *data;
    size_t length;
} TextView;
```

Copying `TextView` creates another alias to the same character data. It does not
duplicate the characters or transfer ownership automatically.

### 6.5 Self-referential structures

A structure cannot contain itself directly because its size would be infinite:

```c
/* Invalid:
struct Node {
    int value;
    struct Node next;
};
*/
```

It can contain a pointer to its own incomplete type:

```c
typedef struct Node {
    int value;
    struct Node *next;
} Node;
```

This is the basis of linked lists, trees, and intrusive data structures.

The pointer does not express ownership by itself. The data structure must define:

- who allocates each node;
- who releases it;
- whether nodes may belong to multiple containers;
- whether removal ends the node's lifetime;
- which operations invalidate iterators or stored pointers.

### 6.6 Compound literals

A compound literal creates an unnamed object of a specified type:

```c
typedef struct {
    int x;
    int y;
} Point;

static int squared_distance(Point point)
{
    return point.x * point.x + point.y * point.y;
}

int result = squared_distance((Point){.x = 3, .y = 4});
```

Compound literals are useful for:

- temporary aggregate arguments;
- default configuration values;
- table entries;
- returning or assigning concise structured values.

Their lifetime depends on scope. Do not retain a pointer to a block-scope
compound literal after leaving that scope.

## 7. Structure Layout, Padding, And Alignment

### 7.1 Why `sizeof(struct)` can exceed member sizes

Consider a common ABI where `int` has size and alignment 4:

```c
struct Example {
    char code;
    int value;
    char state;
};
```

A possible layout is:

```text
offset 0       code       1 byte
offset 1..3    padding    3 bytes
offset 4..7    value      4 bytes
offset 8       state      1 byte
offset 9..11   padding    3 bytes
total                    12 bytes
```

The first padding aligns `value`. Tail padding allows consecutive array
elements to keep every `value` correctly aligned.

This is a typical example, not a universal size guarantee.

### 7.2 Inspecting layout

```c
#include <stddef.h>
#include <stdio.h>

struct Example {
    char code;
    int value;
    char state;
};

int main(void)
{
    printf("size      = %zu\n", sizeof(struct Example));
    printf("alignment = %zu\n", _Alignof(struct Example));
    printf("code      = %zu\n", offsetof(struct Example, code));
    printf("value     = %zu\n", offsetof(struct Example, value));
    printf("state     = %zu\n", offsetof(struct Example, state));
    return 0;
}
```

Use `_Static_assert` for an intentional implementation contract:

```c
_Static_assert(offsetof(struct Example, value) % _Alignof(int) == 0,
               "value is not correctly aligned");
```

A passing assertion proves the property for the current compilation
environment. It does not make the layout portable to every target.

### 7.3 Member ordering

Reordering members can reduce padding:

```c
struct A {
    char first;
    int count;
    char second;
};

struct B {
    int count;
    char first;
    char second;
};
```

`struct B` is often smaller, but "largest member first" is a heuristic, not a
language guarantee.

Do not reorder public or persisted records casually. Member order may be part
of:

- an ABI;
- a foreign-function interface;
- a shared-memory agreement;
- generated code;
- a documented file format;
- hardware or compiler-specific mapping.

### 7.4 Padding bytes are not semantic data

Padding bytes can have unspecified values. Therefore this is not a general
structure equality test:

```c
memcmp(&left, &right, sizeof left) == 0
```

Two structures can have equal member values while padding bytes differ.

Compare members:

```c
static int sample_equal(
    const struct SensorSample *left,
    const struct SensorSample *right)
{
    return left->timestamp_ms == right->timestamp_ms
        && left->temperature_tenths == right->temperature_tenths
        && left->status == right->status;
}
```

For the same reason, do not hash or serialize raw structure memory unless a
specific, validated representation contract permits it.

## 8. Unions And Tagged Unions

### 8.1 Overlapping storage

```c
union NumericValue {
    int integer;
    float real;
};
```

The members overlap:

```text
+---------------------------+
| integer representation    |
| or                        |
| real representation       |
+---------------------------+
```

The union is large and aligned enough for its members. It may include trailing
padding.

Writing one member changes the shared representation:

```c
union NumericValue value = {.integer = 42};
value.real = 3.5F;
```

After the second assignment, the program's logical value is the `real`
alternative. A union does not automatically remember that fact.

### 8.2 The tagged-union invariant

Use an enum tag to record the active alternative:

```c
#include <stdio.h>

typedef enum {
    VALUE_INTEGER,
    VALUE_REAL,
    VALUE_TEXT
} ValueKind;

typedef struct {
    ValueKind kind;
    union {
        int integer;
        double real;
        const char *text;
    } data;
} Value;

static void print_value(const Value *value)
{
    switch (value->kind) {
    case VALUE_INTEGER:
        printf("%d\n", value->data.integer);
        break;
    case VALUE_REAL:
        printf("%.2f\n", value->data.real);
        break;
    case VALUE_TEXT:
        printf("%s\n", value->data.text);
        break;
    default:
        fputs("invalid value kind\n", stderr);
        break;
    }
}

int main(void)
{
    Value value = {
        .kind = VALUE_INTEGER,
        .data.integer = 42
    };

    print_value(&value);
    return 0;
}
```

The invariant is:

```text
kind == VALUE_INTEGER  -> read data.integer
kind == VALUE_REAL     -> read data.real
kind == VALUE_TEXT     -> read data.text
```

Every creation, mutation, parser, copy, and dispatch path must preserve this
relationship.

Centralize construction:

```c
static Value value_from_real(double real)
{
    Value value = {
        .kind = VALUE_REAL,
        .data.real = real
    };
    return value;
}
```

### 8.3 Do not use unions as a portable type-punning shortcut

Code sometimes writes one union member and reads another to inspect a bit
representation. The exact consequences are language- and
representation-sensitive.

For representation transfer, prefer `memcpy`:

```c
#include <stdint.h>
#include <string.h>

static uint32_t float_bits(float value)
{
    uint32_t bits;
    _Static_assert(sizeof bits == sizeof value,
                   "float and uint32_t sizes differ");
    memcpy(&bits, &value, sizeof bits);
    return bits;
}
```

This copies bytes without accessing the `float` object through an incompatible
`uint32_t` lvalue. The meaning of the resulting bits still depends on the
implementation's floating-point representation and byte order.

For files or protocols, decode the specified bytes explicitly instead of
depending on a native union layout.

## 9. Enumerations

### 9.1 Named integral states

```c
typedef enum {
    DEVICE_IDLE,
    DEVICE_STARTING,
    DEVICE_RUNNING,
    DEVICE_ERROR
} DeviceState;
```

Without explicit values, the first enumerator is zero and each subsequent
enumerator increases by one.

Explicit values are useful for stable application-level codes:

```c
typedef enum {
    RESULT_OK = 0,
    RESULT_INVALID_ARGUMENT = 10,
    RESULT_TIMEOUT = 20
} Result;
```

Do not assign protocol or persistent numeric values casually. Once external
software depends on them, changing a value can break compatibility.

### 9.2 Enums improve readability, not complete validation

An enum documents a valid set of states:

```c
static const char *state_name(DeviceState state)
{
    switch (state) {
    case DEVICE_IDLE:
        return "idle";
    case DEVICE_STARTING:
        return "starting";
    case DEVICE_RUNNING:
        return "running";
    case DEVICE_ERROR:
        return "error";
    default:
        return "unknown";
    }
}
```

Data from a file, network message, device, database, or foreign API can contain
an integer that does not correspond to a declared enumerator. Validate at the
boundary:

```c
static int device_state_from_u8(unsigned int raw, DeviceState *out)
{
    if (out == NULL) {
        return 0;
    }

    switch (raw) {
    case DEVICE_IDLE:
    case DEVICE_STARTING:
    case DEVICE_RUNNING:
    case DEVICE_ERROR:
        *out = (DeviceState)raw;
        return 1;
    default:
        return 0;
    }
}
```

### 9.3 Enum size and representation

Do not assume:

```text
sizeof(DeviceState) == sizeof(int)
```

For C17, the compatible integer type is selected by the implementation subject
to the language rules and the enumerator values.

C23 adds syntax for specifying a fixed underlying type:

```c
/* C23 */
enum DeviceMode : unsigned char {
    MODE_OFF,
    MODE_ON
};
```

Use this only when the selected C23 compiler and project baseline support it.
It is not C17 syntax and is not the same feature as C++ `enum class`.

## 10. Flexible Array Members

A flexible array member represents trailing elements stored in the same
allocation as a structure header:

```c
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    uint16_t type;
    size_t length;
    unsigned char payload[];
} Message;
```

The flexible array member:

- must appear last;
- has incomplete array type;
- does not contribute an ordinary fixed element count to `sizeof(Message)`;
- needs a larger allocation when payload elements are required.

### 10.1 Checked allocation

```c
static Message *message_create(uint16_t type, size_t payload_length)
{
    if (payload_length > SIZE_MAX - sizeof(Message)) {
        return NULL;
    }

    Message *message = malloc(sizeof(Message) + payload_length);
    if (message == NULL) {
        return NULL;
    }

    message->type = type;
    message->length = payload_length;
    return message;
}
```

The caller must release the complete allocation with `free`.

For more complicated element types, calculate:

```text
header size + element_count * element_size
```

with overflow checks for both multiplication and addition.

### 10.2 Copying

Ordinary structure assignment does not define a high-level copy operation for
the separately sized payload:

```c
*destination = *source; /* copies fixed members, not the intended payload */
```

A message-copy function must:

- validate the source length;
- allocate sufficient total storage;
- copy the fixed fields;
- copy exactly `length` payload bytes;
- define failure ownership clearly.

Flexible array members are useful, but the size and ownership contract must be
more explicit than for an ordinary fixed-size structure.

## 11. Bit-Fields, Packed Structures, And Binary Data

### 11.1 Bit-fields

```c
struct Flags {
    unsigned int ready : 1;
    unsigned int error : 1;
    unsigned int mode  : 2;
};
```

Bit-fields can compact flags, but important details depend on the
implementation:

- allocation order within a storage unit;
- whether a field crosses a storage-unit boundary;
- alignment;
- the signedness of a plain `int` bit-field;
- padding between allocation units.

You cannot take the address of a bit-field:

```c
/* Invalid: &flags.ready */
```

Use bit-fields for implementation-local compact state only when the layout
tradeoff is understood.

### 11.2 Masks and shifts for specified bit positions

For an externally specified byte, explicit masks are clearer:

```c
#include <stdint.h>

enum {
    STATUS_READY_MASK = 1U << 0,
    STATUS_ERROR_MASK = 1U << 1,
    STATUS_MODE_MASK  = 3U << 2
};

static unsigned int status_mode(uint8_t status)
{
    return (status & STATUS_MODE_MASK) >> 2;
}
```

The external specification must still define the byte order and bit numbering,
but the C source no longer depends on compiler bit-field allocation.

### 11.3 Packed structures

`#pragma pack` and attributes such as `__attribute__((packed))` are compiler
extensions. They can:

- change ABI layout;
- produce misaligned members;
- require multiple machine operations;
- fault on targets that reject unaligned access;
- behave differently between toolchains.

Use them only when:

- a concrete compiler and target ABI are selected;
- the external layout is documented;
- access strategy is validated;
- static assertions verify expected offsets and size;
- tests run on the actual target.

Packing is not a substitute for serialization.

### 11.4 Explicit serialization

Suppose a format defines:

```text
byte 0      version
byte 1      flags
byte 2..3   payload length, big-endian
```

Decode the bytes:

```c
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t version;
    uint8_t flags;
    uint16_t payload_length;
} Header;

static int decode_header(
    const unsigned char *bytes,
    size_t size,
    Header *out)
{
    if (bytes == NULL || out == NULL || size < 4U) {
        return 0;
    }

    out->version = bytes[0];
    out->flags = bytes[1];
    out->payload_length =
        (uint16_t)((uint16_t)bytes[2] << 8)
        | (uint16_t)bytes[3];
    return 1;
}
```

This code defines bounds and byte order explicitly. It does not depend on
structure padding, host endianness, or alignment.

## 12. Practical Usage

### 12.1 Sensor history

Use an array plus an explicit used count:

```c
enum { HISTORY_CAPACITY = 16 };

typedef struct {
    int values[HISTORY_CAPACITY];
    size_t used;
} SensorHistory;

static int history_push(SensorHistory *history, int value)
{
    if (history == NULL || history->used >= HISTORY_CAPACITY) {
        return 0;
    }

    history->values[history->used] = value;
    ++history->used;
    return 1;
}
```

The invariant is:

```text
0 <= used <= HISTORY_CAPACITY
```

Only elements `[0, used)` contain logical history entries.

### 12.2 Finite-state machine

```c
typedef enum {
    STATE_IDLE,
    STATE_ACTIVE,
    STATE_FAULT
} State;

typedef enum {
    EVENT_START,
    EVENT_STOP,
    EVENT_FAILURE
} Event;

static State next_state(State current, Event event)
{
    switch (current) {
    case STATE_IDLE:
        return event == EVENT_START ? STATE_ACTIVE : STATE_IDLE;
    case STATE_ACTIVE:
        if (event == EVENT_STOP) {
            return STATE_IDLE;
        }
        return event == EVENT_FAILURE ? STATE_FAULT : STATE_ACTIVE;
    case STATE_FAULT:
        return STATE_FAULT;
    default:
        return STATE_FAULT;
    }
}
```

The enums make the transition domain visible. The `default` path contains
invalid or unexpected values.

### 12.3 Variant command argument

```c
typedef enum {
    ARGUMENT_NONE,
    ARGUMENT_INTEGER,
    ARGUMENT_TEXT
} ArgumentKind;

typedef struct {
    ArgumentKind kind;
    union {
        int integer;
        struct {
            const char *data;
            size_t length;
        } text;
    } value;
} CommandArgument;
```

The text alternative is a non-owning pointer-length view. Its source must
outlive every use of the argument. This is a lifetime contract in addition to
the union-tag contract.

## 13. C And C++ Comparisons

### 13.1 Arrays

| Topic | C | C++ | Practical direction |
| --- | --- | --- | --- |
| Fixed array | `T[N]` | `T[N]` or `std::array<T, N>` | Prefer `std::array` for value semantics in modern C++ |
| Dynamic array | Manual allocation and count | `std::vector<T>` | Prefer `std::vector` for owning dynamic sequences |
| Non-owning range | Pointer plus count | `std::span<T>` | Carry extent with the view |
| Bounds check | Manual | Manual for `[]`; checked APIs such as `.at()` where available | Validate untrusted indices |

`std::array` preserves fixed extent as an object that can be copied and returned.
`std::vector` owns resizable contiguous storage. Neither removes the need to
understand pointer invalidation and bounds.

### 13.2 Strings

| Topic | C string | `std::string` | `std::string_view` |
| --- | --- | --- | --- |
| Representation | Character sequence ending in `'\0'` | Owning string object | Non-owning pointer-length view |
| Capacity | Managed by caller | Managed by object | No owned capacity |
| Length | Usually found by scanning or tracked separately | Stored by object | Stored by view |
| Ownership | External contract | Owns its characters | Does not own |
| Main risk | Overflow or missing terminator | Invalidated references after modification | Dangling view |

Use `std::string` as the normal owning text type in C++. Use
`std::string_view` for temporary read-only views when lifetime is guaranteed.
Use C strings at C interfaces and where the surrounding C design explicitly
manages capacity.

### 13.3 Structures

| Topic | C `struct` | C++ `struct` |
| --- | --- | --- |
| Member data | Yes | Yes |
| Member functions | No | Yes |
| Constructors/destructors | No | Yes |
| Access control | No class-style access control | Public by default |
| Tag keyword at use | Normally required unless aliased | Not required |
| Typical role | Data aggregate and explicit C API state | Public aggregate or class-like type |

C initialization and cleanup normally use functions:

```c
int device_init(struct Device *device);
void device_deinit(struct Device *device);
```

C++ can bind initialization and cleanup to object lifetime through constructors,
destructors, and RAII.

### 13.4 Unions

| Topic | C union | C++ union | `std::variant` |
| --- | --- | --- | --- |
| Storage | Overlapping | Overlapping | Managed alternative storage |
| Active alternative | Manual convention/tag | Manual plus C++ lifetime rules | Tracked by the type |
| Checked access | No | No | Yes |
| General application variants | Tagged union | Possible but delicate | Preferred in modern C++ |

Understanding C tagged unions remains important for C APIs, embedded code,
binary parsers, and cross-language interfaces.

### 13.5 Enumerations

| Topic | C enum | C++ `enum class` |
| --- | --- | --- |
| Enumerator scope | Enclosing scope | Enumeration scope |
| Integral conversion | Weaker separation | No implicit conversion to integer |
| Name collision | Possible | Reduced through scoping |
| Fixed underlying type | C23 | C++11 and later |

`enum class` improves type safety, but external values still need validation.

### 13.6 Aliases

```c
typedef unsigned long Counter;
```

```cpp
using Counter = unsigned long;
```

C++ `using` has clearer left-to-right syntax and supports alias templates.
Neither spelling creates a new distinct type.

## 14. Common Bugs

### 14.1 Out-of-bounds array access

```c
int values[4] = {0};
values[4] = 10; /* undefined behavior */
```

The last valid index is `3`.

### 14.2 Off-by-one string capacity

```c
char text[5] = "Hello"; /* valid C array, but no room for '\0' */
```

In C, this creates a five-element character array containing the visible
characters only. It is not a valid C string and must not be passed to
terminator-based string functions.

For a valid C string, use:

```c
char text[6] = "Hello";
```

### 14.3 `sizeof` after parameter adjustment

```c
void process(int values[])
{
    size_t count = sizeof values / sizeof values[0]; /* wrong */
}
```

Pass `count` explicitly.

### 14.4 Treating binary bytes as a string

```c
printf("%s\n", payload); /* wrong unless payload is a valid string */
```

Use a byte count and an appropriate binary format.

### 14.5 Ignoring `snprintf` truncation

```c
snprintf(buffer, sizeof buffer, "%s:%s", first, second);
/* Wrong if the application assumes the complete output always fit. */
```

Check the return value.

### 14.6 Comparing structures with `memcmp`

```c
if (memcmp(&left, &right, sizeof left) == 0) {
    /* Not a general semantic equality test. */
}
```

Compare members.

### 14.7 Raw structure serialization

```c
fwrite(&header, sizeof header, 1U, file);
```

This writes native padding, endianness, widths, and representations. It is not
a portable format unless all those properties are explicitly part of a
controlled ABI.

### 14.8 Stale union tag

```c
value.kind = VALUE_INTEGER;
value.data.real = 3.5; /* tag and payload disagree */
```

Centralize creation and mutation.

### 14.9 Assuming enum size

```c
_Static_assert(sizeof(DeviceState) == 4, "unexpected enum size");
```

This is valid only as a deliberate selected-platform contract, not as a general
C expectation.

### 14.10 Bit-field protocol mapping

```c
struct HeaderBits {
    unsigned int version : 4;
    unsigned int type : 4;
};
```

This does not by itself define which field occupies the high or low four bits
of an external byte.

### 14.11 Flexible-array under-allocation

```c
Message *message = malloc(sizeof *message); /* no payload storage */
message->payload[0] = 1U;                   /* invalid */
```

Allocate and validate the complete required size.

## 15. Debugging Compound-Type Failures

### 15.1 Start with strong diagnostics

For C17 with GCC or Clang:

```bash
cc -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
   -Wformat=2 -g source.c -o program
```

Useful compiler-specific warnings may include:

```text
-Warray-bounds
-Wstringop-overflow
-Wstringop-truncation
-Wformat-truncation
-Wswitch-enum
-Wpadded
```

Warning names and behavior differ between compilers. Enable them selectively
and verify them in the project's toolchain.

### 15.2 AddressSanitizer

```bash
cc -std=c17 -g -O1 -fno-omit-frame-pointer \
   -fsanitize=address source.c -o program
./program
```

AddressSanitizer commonly detects:

- stack, heap, and global out-of-bounds access;
- many off-by-one terminator writes;
- use-after-free in dynamically owned aggregate data;
- some string scans that leave valid objects.

It cannot prove that every array or string contract is correct.

### 15.3 UndefinedBehaviorSanitizer

```bash
cc -std=c17 -g -O1 -fno-omit-frame-pointer \
   -fsanitize=undefined source.c -o program
./program
```

UBSan can detect supported cases involving:

- misaligned access;
- invalid shifts;
- object-size assumptions;
- some invalid enum values and related operations.

Tool coverage depends on compiler and platform.

### 15.4 Inspecting structure layout

Use:

```c
sizeof(Type)
_Alignof(Type)
offsetof(Type, member)
```

At runtime, inspect member addresses:

```c
printf("%p\n", (void *)&object);
printf("%p\n", (void *)&object.member);
```

In GDB:

```text
ptype object
p sizeof(object)
p &object.member
x/16bx &object
```

Do not interpret padding bytes as member values.

### 15.5 Debugging strings

When termination is uncertain:

1. identify the destination capacity;
2. identify the intended length;
3. inspect at most `capacity` bytes;
4. find the first missing or misplaced `'\0'`;
5. locate the write that violated the invariant.

Do not call `strlen` on the suspect buffer merely to debug whether it is
terminated. That can repeat the invalid scan.

### 15.6 Debugging tagged unions

Log or inspect:

- the tag;
- the code path that last changed the tag;
- the code path that last wrote the payload;
- the selected switch branch;
- ownership and lifetime of pointer members.

A union access failure often begins earlier, when one update changes only the
tag or only the payload.

### 15.7 Fuzzing

Parsers and command dispatchers should be tested with:

- empty input;
- one-byte-short input;
- maximum-size input;
- unknown enum and union tag values;
- unterminated text;
- embedded zero bytes;
- inconsistent length fields;
- oversized flexible-array payload lengths;
- truncated multibyte fields.

## 16. Best Practices Checklist

### Arrays

- Keep extent with every array interface.
- Use `size_t` for sizes and counts.
- Validate every untrusted index before access.
- Use `sizeof array / sizeof array[0]` only while the expression is an array.
- Preserve multidimensional row shape in the parameter type.
- Initialize elements before reading them.

### Strings

- Track capacity, length, and termination separately.
- Reserve one element for `'\0'`.
- Use `const char *` for read-only string-literal access.
- Prefer `fgets` for bounded line input.
- Prefer `snprintf` for bounded formatting and check its return value.
- Never use `gets`.
- Do not treat `strncpy` as an automatically safe copy.
- Keep binary buffers out of terminator-based string APIs.

### Structures

- Initialize every member.
- Use designated initializers where they improve clarity.
- Inspect layout rather than guessing it.
- Compare members semantically.
- Serialize fields explicitly.
- Reorder members only after measuring and reviewing ABI effects.
- Make pointer-member ownership and lifetime explicit.

### Unions And Enums

- Pair each union with a reliable tag when alternatives represent different
  logical types.
- Update tag and payload together.
- Validate tags and enum values from external sources.
- Include a deliberate unknown/default policy.
- Use `memcpy` or explicit byte conversion instead of casual union type
  punning.

### Binary Layout

- Define widths, byte order, encoding, and bounds explicitly.
- Prefer masks and shifts over bit-fields for specified external bits.
- Treat packed structures as compiler- and target-specific.
- Verify intentional ABI contracts with static assertions and target tests.

### Aliases

- Use `typedef` to improve meaning and readability.
- Remember that an alias is not a new distinct type.
- Avoid aliases that hide pointer ownership or const qualification.

## 17. Interview Readiness

### 17.1 Junior questions

**What is the difference between an array and a pointer?**

An array owns a fixed sequence of contiguous elements, and its type includes
the extent. A pointer is a separate scalar object holding a pointer value and
does not carry an extent. An array expression converts to a pointer to its first
element in most expressions, but the types are not identical.

**Why does `"Hello"` require six `char` elements?**

There are five visible characters plus the terminating null character `'\0'`.

**What is the difference between a structure and a union?**

All structure members have distinct storage and can hold values
simultaneously. Union members overlap in storage, so the program must track
which representation is logically active.

**Why is `gets` dangerous?**

It has no destination-capacity argument. Input length can therefore force it to
write beyond the destination. The function was removed from the C standard
library in C11.

### 17.2 Middle-level questions

**Why can `strncpy` produce an unterminated destination?**

If the source has at least `count` characters, `strncpy` copies `count`
characters and does not append an extra terminator. Its count also controls
zero padding when the source is shorter.

**How do you pass a two-dimensional array to a function?**

The function must know the inner array extent so pointer arithmetic can advance
by complete rows:

```c
void process(size_t rows, int matrix[][COLUMN_COUNT]);
```

This parameter is adjusted to a pointer to an array of `COLUMN_COUNT` integers.

**Why can a structure be larger than the sum of its members?**

The implementation can insert internal padding to align members and tail
padding so elements of a structure array remain correctly aligned.

**How do you make a union type-safe in C?**

Place an enum discriminator beside the union, define the tag-to-member
invariant, centralize construction and mutation, and check the tag before every
payload access.

### 17.3 Senior questions

**Why is casting a packet buffer to a structure pointer risky?**

The buffer may be too short or misaligned, and the native structure can contain
padding, host-endian integers, implementation-specific widths, or invalid
representations. Explicit decoding handles bounds and external representation
directly.

**When is a packed structure acceptable?**

Only under a documented compiler, ABI, target, and external layout contract,
with verified offsets, safe access to possibly misaligned members, and target
tests. It should not be the default serialization design.

**What is the main risk of a flexible array member?**

The language type does not carry the runtime payload extent. Allocation,
overflow checks, ownership, copying, and every access must use a separately
maintained length contract.

**How should an API handle future enum values?**

Validate values at boundaries, avoid assuming every integer maps to a known
enumerator, preserve forward compatibility where required, and provide an
explicit unknown or error path rather than entering an impossible state.

## 18. Practice Tasks

### Basic

1. Write a function that returns the minimum value in an `int` array using a
   pointer-plus-count interface. Define behavior for an empty range.
2. Print the size and element count of a local array, then pass it to a
   function and explain why the same `sizeof` expression changes meaning.
3. Read a line with `fgets`, remove one retained newline, and print whether the
   complete line fit.
4. Define a nested `DeviceConfig` structure and initialize it with designated
   initializers.
5. Compare two structures member by member.

### Intermediate

1. Implement a bounded text formatter around `snprintf` that distinguishes
   success, truncation, and formatting error.
2. Measure `sizeof`, `_Alignof`, and `offsetof` for three member orders and
   explain all observed padding.
3. Implement a tagged union containing an integer, a floating-point value, and
   a non-owning text view.
4. Implement a fixed-capacity sensor history with `push`, `clear`, and
   average operations.
5. Decode a four-byte big-endian record without a structure cast.

### Advanced

1. Implement `message_create`, `message_clone`, and `message_destroy` for a
   flexible-array message with overflow-safe size calculations.
2. Fuzz a tagged-union parser with invalid tags, truncated payloads, embedded
   zeros, and maximum lengths.
3. Compare a bit-field representation with masks and shifts across two
   compilers. Record observations without treating them as portable rules.
4. Build an enum-to-string table using an X-macro, then review whether the
   reduced duplication is worth the macro complexity.
5. Implement an intrusive singly linked list and document node ownership,
   insertion, removal, and invalidation rules.

## 19. Summary

- Arrays own fixed contiguous sequences; pointers do not carry array extent.
- Array parameters are adjusted to pointers, so functions need explicit bounds.
- Multidimensional arrays are arrays of arrays, and row shape matters.
- A C string is a null-terminated character sequence, not merely a `char *`.
- Correct string code tracks capacity, length, and the terminator.
- `strcpy` and `strcat` require externally proved capacity; `strncpy` has
  non-termination and padding traps.
- `fgets` and `snprintf` support bounded designs, but callers must process their
  results correctly.
- Structures contain simultaneously live members and can include internal and
  tail padding.
- Raw structure equality and serialization are generally nonportable.
- Union members overlap. A tagged union must keep its discriminator and payload
  consistent.
- C enums improve readability but do not provide C++ `enum class` type safety
  or a universal fixed size.
- Flexible array members require explicit allocation, extent, ownership, and
  copy rules.
- Bit-fields and packed structures depend on compiler and ABI details.
- External binary data should be encoded and decoded explicitly.
- `typedef` improves naming but creates an alias, not a distinct type.

## 20. Reference Notes

- ISO/IEC 9899:2018 (C17) is the practical language baseline for this chapter.
- ISO/IEC 9899:2024 (C23) adds features including fixed underlying types for
  enumerations; C23 syntax is labeled where shown.
- cppreference C provides navigable summaries for arrays, string literals,
  structures, unions, enumerations, string functions, and `fgets`.
- Compiler documentation is required before depending on packed attributes,
  bit-field layout, ABI offsets, or warning availability.
