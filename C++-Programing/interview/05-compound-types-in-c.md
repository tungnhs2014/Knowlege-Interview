# 05 - Compound Types In C: Interview Pack

## How To Use This Pack

For each question:

1. Give the **Short answer** first.
2. Expand with the **Deep explanation** when the interviewer asks why.
3. Ground the answer with the **C/C++ code/API anchor**.
4. Show engineering judgment through the **Production/debug angle**.
5. Avoid the listed **Common traps**.
6. Use the **Follow-up questions** to test the limits of the model.

The practical baseline is C17. C23 and C++ features are labeled explicitly.

The recurring contracts are:

```text
array       element type + extent
C string    capacity + length + null terminator
struct      members + alignment + layout assumptions
union       tag + active representation
enum        valid values + unknown-value policy
binary data widths + byte order + bounds + encoding
```

## Beginner Questions

### 1. What is the difference between an array and a pointer?

**Short answer**

An array is an object containing a fixed number of contiguous elements. A
pointer is a separate scalar object holding a pointer value. In most
expressions, an array converts to a pointer to its first element, but that
conversion does not make the types identical.

**Deep explanation**

For `int values[4]`, the array owns four `int` elements and its type includes
the extent. In most expressions, `values` converts to `int *` pointing at
`values[0]`. The conversion loses the extent.

Important contexts where the usual conversion does not occur include
`sizeof values` and unary `&values`.

`values`, `&values[0]`, and `&values` can refer to the same starting location,
but their types and arithmetic differ. `&values + 1` advances by one complete
array.

**C/C++ code/API anchor**

```c
int values[4] = {10, 20, 30, 40};

int *element = values;
int (*whole)[4] = &values;

size_t array_bytes = sizeof values;
size_t pointer_bytes = sizeof element;
```

In C++, `std::array<int, 4>` preserves fixed extent as a copyable value type,
while `std::vector<int>` owns dynamically sized contiguous storage.

**Production/debug angle**

Pointer-only APIs cannot recover the caller's array extent. In C, use
pointer-plus-count. In modern C++, consider `std::span` for a bounded,
non-owning range.

When debugging, inspect both type and value:

```text
ptype values
ptype &values
p sizeof(values)
```

**Common traps**

- Saying the array variable stores a pointer.
- Saying `sizeof(array)` always returns pointer size.
- Assuming `&values` has type `int *`.
- Treating an array parameter's written bound as runtime size metadata.

**Follow-up questions**

- What is the type of `&values`?
- What does `whole + 1` mean?
- When does array-to-pointer conversion not occur?
- How would you design a bounded C array API?

### 2. Why does `"Hello"` require six `char` elements?

**Short answer**

It contains five visible characters plus the terminating null character
`'\0'`, which marks the end of a C string.

**Deep explanation**

A C string is a convention, not a distinct language type. It is a sequence of
characters terminated by a zero character.

```text
[H][e][l][l][o][\0]
```

A character array can exist without a terminator, but then it is not a valid C
string for APIs such as `strlen`, `strcmp`, or `printf("%s", ...)`.

Capacity and string length are different:

- capacity: total elements in the array;
- length: characters before the first `'\0'`;
- required size: length plus one terminator.

**C/C++ code/API anchor**

```c
char text[] = "Hello";

sizeof text;  /* 6 */
strlen(text); /* 5 */
```

This C declaration contains no terminator:

```c
char bytes[5] = "Hello";
```

It is a character array, but not a valid C string.

In C++, `std::string` stores its logical length, while `std::string_view`
stores a pointer and length without owning the characters.

**Production/debug angle**

Off-by-one string bugs often come from allocating `length` bytes instead of
`length + 1`. With suspicious input, inspect at most the known capacity; do not
call `strlen` merely to discover whether the buffer is terminated.

AddressSanitizer can detect many scans or writes that leave the valid object.

**Common traps**

- Saying every `char` array is a C string.
- Forgetting that `strlen` excludes the terminator.
- Assuming `sizeof(char *)` reveals string capacity.
- Calling an unterminated array "an empty string."

**Follow-up questions**

- What is the difference between length and capacity?
- Is `char bytes[5] = "Hello";` a valid C string?
- What precondition does `strlen` require?
- Can a binary payload safely be passed to `%s`?

### 3. Compare a structure and a union.

**Short answer**

A structure gives each member separate storage, so all members can hold values
simultaneously. Union members overlap in storage, so the program must know which
representation is logically active.

**Deep explanation**

Structure size includes storage for all members plus possible internal and tail
padding. Union size and alignment are sufficient for its members, and every
member begins in overlapping storage.

A union does not remember which member was written. For variant data, a
separate enum tag should identify the valid interpretation.

**C/C++ code/API anchor**

```c
struct Measurement {
    int id;
    double value;
};

union Number {
    int integer;
    double real;
};
```

For a C tagged union:

```c
typedef enum {
    NUMBER_INTEGER,
    NUMBER_REAL
} NumberKind;

typedef struct {
    NumberKind kind;
    union {
        int integer;
        double real;
    } data;
} TaggedNumber;
```

Modern C++ commonly uses `std::variant<int, double>` for a managed,
type-checked alternative set.

**Production/debug angle**

For structures, review alignment, padding, pointer-member ownership, and raw
serialization assumptions. For unions, log the tag and payload mutation sites
and verify that every dispatch checks the tag.

**Common traps**

- Saying a union stores all member values simultaneously.
- Assuming union size is always exactly the largest member size with no
  alignment considerations.
- Reading an arbitrary inactive member as a portable type-punning technique.
- Forgetting that a pointer member copied in a structure remains an alias.

**Follow-up questions**

- Why does a tagged union need a discriminator?
- Can a structure contain a union?
- How does `std::variant` differ from a C union?
- Is raw structure assignment a deep copy?

### 4. What are structure padding and alignment?

**Short answer**

Alignment constrains valid object addresses. Padding is extra storage an
implementation may place between structure members or at the end to satisfy
alignment and array-layout requirements.

**Deep explanation**

Members appear in declaration order, but their offsets need not equal the sum
of previous member sizes. Tail padding can ensure that each element of a
structure array has the required alignment.

For an ABI where `int` has size and alignment 4:

```c
struct Example {
    char code;
    int value;
    char state;
};
```

A common layout is:

```text
offset 0       code
offset 1..3    internal padding
offset 4..7    value
offset 8       state
offset 9..11   tail padding
```

This example is not a universal size guarantee.

**C/C++ code/API anchor**

```c
#include <stddef.h>

size_t size = sizeof(struct Example);
size_t alignment = _Alignof(struct Example);
size_t value_offset = offsetof(struct Example, value);
```

C++ uses `alignof` and also provides layout traits and class-layout rules that
must be considered for nontrivial types.

**Production/debug angle**

Use `sizeof`, `_Alignof`, `offsetof`, `_Static_assert`, compiler layout reports,
and target ABI documentation. Reordering members can reduce padding, but may
break an ABI or external layout.

**Common traps**

- Calculating structure size as only the sum of member sizes.
- Saying padding exists only for performance; some targets require alignment.
- Assuming "largest member first" always gives the minimum size.
- Treating one compiler's layout as a portable wire format.

**Follow-up questions**

- Why is tail padding useful?
- Can padding bytes affect `memcmp`?
- When is member reordering unsafe?
- How would you verify a required offset?

### 5. What is an enum, and can you assume it has the size of `int`?

**Short answer**

An enum defines an enumerated type and named integral constants. In C17, do not
assume every enum has the same size or representation as `int`.

**Deep explanation**

Enumerators default to zero and increment unless explicitly assigned:

```c
enum State {
    STATE_IDLE,
    STATE_RUNNING,
    STATE_ERROR = 10
};
```

C enumerator names enter the enclosing ordinary identifier namespace. C enums
also provide weaker type separation than C++ `enum class`.

For pre-C23 C, the compatible integer type is implementation-defined subject to
the language's representability rules. C23 adds syntax for a fixed underlying
type.

An enum lists intended states, but external integers can still be outside the
declared enumerator set. Validate input at boundaries.

**C/C++ code/API anchor**

```c
typedef enum {
    MODE_OFF = 0,
    MODE_ON = 1
} Mode;

static int mode_from_int(int raw, Mode *out)
{
    if (out == NULL || (raw != MODE_OFF && raw != MODE_ON)) {
        return 0;
    }

    *out = (Mode)raw;
    return 1;
}
```

C++ `enum class Mode : unsigned char` scopes enumerators and prevents implicit
conversion to integers. C23 fixed-underlying-type syntax is a separate feature.

**Production/debug angle**

For persisted or externally transmitted values, define stable numeric codes,
validate unknown values, and specify a forward-compatibility policy. Use
`-Wswitch-enum` where useful, but do not rely on warnings as runtime validation.

**Common traps**

- Assuming `sizeof(enum_type) == sizeof(int)`.
- Saying enum values cannot be invalid at runtime.
- Confusing C23 fixed underlying types with C++ `enum class`.
- Changing externally visible enumerator values without compatibility review.

**Follow-up questions**

- Where are C enumerator names scoped?
- How should a parser handle a future enum value?
- What does C++ `enum class` improve?
- Why can a complete `default` policy still matter?

## Mid-Level Questions

### 6. Why does `sizeof` fail to recover an array's length inside a function?

**Short answer**

An array parameter is adjusted to a pointer parameter. Inside the function,
`sizeof parameter` measures the pointer object, not the caller's array.

**Deep explanation**

These declarations describe the same parameter type:

```c
void process(int values[]);
void process(int values[10]);
void process(int *values);
```

The written bound does not become runtime metadata. The caller's array
expression converts to a pointer to its first element, and the function
receives no extent unless another parameter or protocol provides it.

C's `static` array parameter form can state a minimum caller obligation, but it
still does not create a bounds-checked array parameter.

**C/C++ code/API anchor**

```c
static int sum(const int *values, size_t count)
{
    int total = 0;

    for (size_t i = 0; i < count; ++i) {
        total += values[i];
    }

    return total;
}
```

Minimum-bound contract:

```c
void normalize(float values[static 16]);
```

In C++, a template taking `T (&array)[N]`, `std::array`, or `std::span` can
preserve or carry extent.

**Production/debug angle**

Review every pointer-range API for an explicit count, begin/end pair, sentinel,
or protocol-defined extent. Use ASan and boundary tests, especially for zero,
one, maximum, and one-too-large counts.

**Common traps**

- Believing `int values[10]` enforces a ten-element runtime parameter.
- Passing byte size where element count is expected.
- Multiplying by element size twice.
- Trusting a count without validating the underlying object.

**Follow-up questions**

- What does `values[static 16]` promise?
- How would you express a read-only range?
- How does `std::span` improve the C++ interface?
- Can a pointer-plus-count pair still be invalid?

### 7. How should a multidimensional array be passed to a function?

**Short answer**

Preserve the inner array extent so the compiler knows the row stride, or use a
flat buffer with explicit dimensions and checked index calculations.

**Deep explanation**

`int matrix[2][3]` is an array of two arrays, each containing three `int`
elements. When passed to a function, the outer array converts to a pointer to
its first row. The parameter therefore needs the row type.

`int **` is not compatible with `int [2][3]`. An `int **` normally points to
pointers; the matrix contains `int` rows directly.

**C/C++ code/API anchor**

```c
enum { COLUMN_COUNT = 3 };

static void clear_matrix(
    size_t rows,
    int matrix[][COLUMN_COUNT])
{
    for (size_t row = 0; row < rows; ++row) {
        for (size_t column = 0; column < COLUMN_COUNT; ++column) {
            matrix[row][column] = 0;
        }
    }
}
```

C99 VLA form:

```c
void clear_dynamic(
    size_t rows,
    size_t columns,
    int matrix[rows][columns]);
```

In C++, nested `std::array`, a flat `std::vector`, or a multidimensional view
can make ownership and dimensions clearer.

**Production/debug angle**

Validate `rows * columns` for overflow in flat representations. Test row
boundaries and use cache-friendly row-major traversal when the format is
row-major.

**Common traps**

- Replacing a real 2D array parameter with `int **`.
- Omitting the inner extent.
- Confusing element count with byte count.
- Assuming VLA support is identical across all C and C++ toolchains.

**Follow-up questions**

- What is the adjusted type of `int matrix[][3]`?
- When would a flat buffer be preferable?
- How does row-major layout affect traversal?
- Are VLAs standard C++?

### 8. Compare `strcpy`, `strncpy`, `snprintf`, and a bounded text-copy API.

**Short answer**

`strcpy` has no destination-capacity parameter. `strncpy` has padding and
non-termination behavior. `snprintf` receives capacity and reports required
output length. A production API should define capacity, truncation, overlap,
and result semantics explicitly.

**Deep explanation**

`strcpy` is correct only when the caller proves:

```text
strlen(source) + 1 <= destination capacity
```

`strncpy(destination, source, count)`:

- does not append an extra terminator when the source length is at least
  `count`;
- zero-pads the destination when the source is shorter;
- is not a general safe replacement for `strcpy`.

`snprintf` returns the character count that would have been produced, excluding
the terminator. A negative return indicates error; a nonnegative result greater
than or equal to capacity indicates truncation.

**C/C++ code/API anchor**

```c
int written = snprintf(destination, capacity, "%s", source);

if (written < 0) {
    /* formatting or encoding error */
} else if ((size_t)written >= capacity) {
    /* truncated */
} else {
    /* complete, terminated output */
}
```

In C++, prefer `std::string` for owned text and `std::string_view` for a
non-owning read-only view whose lifetime is valid.

**Production/debug angle**

Choose a policy: reject truncation, report required size, or truncate
deliberately. Test empty source, exact fit, one-byte-short capacity, embedded
zero behavior, and overlapping input/output where relevant.

Compiler warnings and ASan can detect many mistakes, but API design should make
the capacity proof reviewable.

**Common traps**

- Calling `strncpy` safe without describing termination.
- Ignoring the `snprintf` return value.
- Treating `sizeof(destination)` as capacity when `destination` is a pointer.
- Assuming overflow must immediately crash.

**Follow-up questions**

- What does `snprintf` return on truncation?
- Why does `strncpy` pad?
- How would you report the required destination size?
- What lifetime risk does `std::string_view` introduce?

### 9. How would you design and validate a tagged union?

**Short answer**

Place an enum tag next to the union, define exactly which member corresponds to
each tag, centralize construction and mutation, validate external tags, and
check the tag before every payload access.

**Deep explanation**

The core invariant is:

```text
tag A -> member A is the valid logical representation
tag B -> member B is the valid logical representation
```

The tag and payload form one logical value and must be updated together. If a
member contains a pointer, the variant also needs an ownership and lifetime
contract.

Copying a tagged union with pointer payloads copies pointer values, not owned
data. Cleanup can depend on the active alternative.

**C/C++ code/API anchor**

```c
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
        struct {
            const char *data;
            size_t length;
        } text;
    } data;
} Value;

static Value value_from_integer(int number)
{
    Value value = {
        .kind = VALUE_INTEGER,
        .data.integer = number
    };
    return value;
}
```

C++ `std::variant<int, double, std::string_view>` tracks the alternative and
offers checked access, but the `string_view` alternative can still dangle.

**Production/debug angle**

Log the tag at parse, mutation, and dispatch boundaries. Fuzz unknown tags,
truncated payloads, and tag/payload mismatches. Use exhaustive switch handling
and explicit failure behavior.

**Common traps**

- Updating only the tag or only the payload.
- Assuming the union remembers the active member.
- Forgetting cleanup for an owning pointer alternative.
- Trusting an unvalidated tag from input.

**Follow-up questions**

- How would you copy a tagged union with owned text?
- What should happen for an unknown tag?
- How does `std::variant` improve access?
- Can a correctly tagged pointer payload still be invalid?

### 10. Why is `memcmp` not a general structure equality operation?

**Short answer**

Structures can contain padding bytes with unspecified values, and some member
types can have multiple representations for equivalent values. Semantic
equality should compare members according to their meaning.

**Deep explanation**

The language guarantees member order, but allows internal and tail padding.
Ordinary initialization and assignment establish member values; they do not
make padding a semantic part of the value.

Two structures can have equal members but different padding bytes, causing
`memcmp` to report inequality. Conversely, byte equality may not express the
application's intended equivalence for pointers, case-insensitive text, NaNs,
or normalized fields.

**C/C++ code/API anchor**

```c
typedef struct {
    int id;
    char state;
} Record;

static int record_equal(const Record *left, const Record *right)
{
    return left->id == right->id
        && left->state == right->state;
}
```

In C++, define `operator==` or use defaulted comparison only when member-wise
semantics match the domain.

**Production/debug angle**

Use field-aware equality, hashing, serialization, and logging. If a byte-level
comparison is intentionally required by an ABI, document and test the exact
representation contract rather than relying on ordinary structure semantics.

**Common traps**

- Assuming zero-initialization always makes all future padding deterministic.
- Comparing pointer addresses when pointed-to content equality is required.
- Reusing raw-byte equality as a portable hash or file format.
- Assuming equal floating-point values always imply equal object bytes.

**Follow-up questions**

- Can structure assignment copy member values correctly?
- Why can raw hashing have the same problem?
- When might `memcmp` be acceptable?
- How should pointer members be compared?

### 11. How does a flexible array member work?

**Short answer**

A flexible array member is an incomplete array placed last in a structure. Its
runtime elements occupy extra storage in the same allocation, so allocation,
extent, copying, and ownership must be handled explicitly.

**Deep explanation**

```c
typedef struct {
    uint16_t type;
    size_t length;
    unsigned char payload[];
} Message;
```

`sizeof(Message)` covers the fixed header and any implementation padding, but
not a runtime payload count. The allocation must include additional bytes.

The size calculation must reject overflow. Ordinary structure assignment does
not implement the intended variable-payload copy. A clone operation must
allocate and copy the payload explicitly.

**C/C++ code/API anchor**

```c
static Message *message_create(uint16_t type, size_t length)
{
    if (length > SIZE_MAX - sizeof(Message)) {
        return NULL;
    }

    Message *message = malloc(sizeof(Message) + length);
    if (message == NULL) {
        return NULL;
    }

    message->type = type;
    message->length = length;
    return message;
}
```

C++ generally models this with a class containing `std::vector<std::byte>` or
another owning container rather than a C flexible array member.

**Production/debug angle**

Review multiplication and addition overflow, maximum accepted length,
allocation failure, payload bounds, cloning, and one clear `free` owner. Fuzz
zero, maximum, oversized, and truncated payload lengths.

**Common traps**

- Allocating only `sizeof(Message)` and then writing `payload[0]`.
- Assuming `sizeof(Message)` includes runtime payload bytes.
- Copying only the fixed header.
- Trusting an external length before checking bounds and limits.

**Follow-up questions**

- Why must the flexible member be last?
- How would element types larger than one byte affect size checks?
- How would you clone the object?
- When might this design be unsuitable in embedded software?

## Senior Questions

### 12. Review a design that casts a packet buffer to a structure pointer.

**Short answer**

The cast is unsafe as a general parser because the buffer may be too short,
misaligned, encoded with different byte order, or incompatible with native
padding and type representation. Parse explicit bytes into ordinary objects.

**Deep explanation**

This pattern is suspicious:

```c
const struct Header *header = (const struct Header *)bytes;
```

It assumes:

- enough bytes exist for the native structure;
- the address satisfies structure alignment;
- native member offsets match the external format;
- integer widths match;
- host byte order matches;
- all bit and scalar representations are valid;
- effective-type and aliasing rules permit the access;
- the buffer remains alive.

An external format must define its own representation independent of one
compiler's structure layout.

**C/C++ code/API anchor**

```c
typedef struct {
    uint8_t version;
    uint8_t flags;
    uint16_t length;
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
    out->length =
        (uint16_t)((uint16_t)bytes[2] << 8)
        | (uint16_t)bytes[3];
    return 1;
}
```

C++ can use `std::span<const std::byte>` as the bounded input view, but still
needs explicit decoding.

**Production/debug angle**

Fuzz short buffers, invalid versions, oversized lengths, and unknown flags.
Run ASan/UBSan and test both endian families where supported. Record protocol
version and field-level parse errors rather than dumping native structures.

**Common traps**

- Fixing only padding with a packed attribute.
- Assuming fixed-width integer types also solve endianness.
- Dereferencing an unaligned cast before checking bounds.
- Treating one target's successful test as portability proof.

**Follow-up questions**

- Does `__attribute__((packed))` solve the parser?
- How would you decode a signed field?
- Where should payload-length validation occur?
- What would a C++ bounded API look like?

### 13. When is a packed structure justified, and what risks remain?

**Short answer**

A packed structure is justified only under a concrete compiler, target ABI, and
external layout contract. It can reduce or alter padding, but may create
misaligned members and does not solve byte order, bit numbering, validity, or
versioning.

**Deep explanation**

`#pragma pack` and packed attributes are implementation extensions. Their
meaning, nesting, and ABI effects come from compiler documentation.

Even if member offsets match a format:

- accessing a misaligned member may require special code or fault;
- integer byte order may differ;
- bit-fields remain implementation-sensitive;
- the buffer still needs bounds and lifetime validation;
- compiler changes can affect layout.

Packing should be an explicit integration decision, not an optimization reflex.

**C/C++ code/API anchor**

```c
/* Compiler-specific, not portable C. */
struct __attribute__((packed)) PackedHeader {
    uint8_t version;
    uint32_t length;
};
```

A safer access strategy can copy bytes into an aligned object and decode the
specified representation.

In C++, the same ABI risks remain. `std::bit_cast` does not make a foreign byte
format automatically valid or endian-correct.

**Production/debug angle**

Require:

- compiler and target documentation;
- `sizeof`/`offsetof` static assertions;
- target execution tests;
- safe handling of potentially unaligned members;
- explicit endian conversion;
- a versioned compatibility plan.

**Common traps**

- Saying packing simply removes all padding.
- Taking a packed-member address and passing it as a normally aligned pointer.
- Using packing as portable serialization.
- Ignoring performance and fault behavior on the target.

**Follow-up questions**

- How would you safely read a packed `uint32_t`?
- What does a static assertion prove?
- Why are bit-fields still risky inside a packed record?
- When is explicit serialization preferable?

### 14. Explain C union type punning and a more portable alternative.

**Short answer**

Reading a different union member after writing one relies on C-specific and
representation-sensitive rules. It should not be treated as a universally
portable conversion. Use `memcpy` for object-representation transfer or decode
external bytes explicitly.

**Deep explanation**

A union overlaps storage, but overlapping storage does not mean all member
values are simultaneously meaningful. Reading a different member can expose a
representation whose value and portability depend on the C rules, type
representations, padding, and implementation.

For bit inspection, byte copying avoids accessing an object through an
incompatible lvalue type. It still does not standardize the underlying
floating-point format or byte order.

The C and C++ union lifetime/access rules are not interchangeable. A correct
answer should state the language and version rather than repeating a generic
strict-aliasing slogan.

**C/C++ code/API anchor**

```c
#include <stdint.h>
#include <string.h>

static uint32_t float_bits(float value)
{
    uint32_t bits;
    _Static_assert(sizeof bits == sizeof value,
                   "unexpected representation sizes");
    memcpy(&bits, &value, sizeof bits);
    return bits;
}
```

In C++20, `std::bit_cast<uint32_t>(value)` can express the same-size
representation copy, subject to its type constraints. It still does not perform
endian conversion.

**Production/debug angle**

Use representation tests only for selected platforms and document assumptions.
For protocols, define byte order and decode bytes. Compare optimized and
unoptimized builds when investigating aliasing-sensitive behavior.

**Common traps**

- Saying a union cast numerically converts the value.
- Applying C++ active-member rules directly to C.
- Claiming `memcpy` standardizes float representation.
- Forgetting size and destination-type validity constraints.

**Follow-up questions**

- What does `memcpy` solve and not solve?
- How would endianness affect the observed integer?
- What does `std::bit_cast` guarantee?
- Why might behavior change under optimization?

### 15. How would you design a versioned message API using enums and tagged unions?

**Short answer**

Separate the external byte format from the internal tagged union. Validate
version, type, lengths, and enum values before constructing the internal value.
Define unknown-message behavior and encode fields explicitly.

**Deep explanation**

A robust design has layers:

1. bounded byte input;
2. header decoding;
3. version and size validation;
4. message-type validation;
5. payload-specific decoding;
6. construction of a valid internal tagged union;
7. explicit destruction if alternatives own resources.

Do not cast the input into the internal structure. The internal enum values also
do not need to equal protocol numeric codes.

Forward compatibility requires a policy:

- reject unknown versions;
- skip unknown message types using a validated length;
- preserve opaque payloads;
- negotiate capabilities.

**C/C++ code/API anchor**

```c
typedef enum {
    MESSAGE_TEMPERATURE,
    MESSAGE_STATUS
} MessageKind;

typedef struct {
    MessageKind kind;
    union {
        int temperature_tenths;
        unsigned int status_flags;
    } data;
} DecodedMessage;

int decode_message(
    const unsigned char *bytes,
    size_t size,
    DecodedMessage *out);
```

C++ could return `std::expected<std::variant<Temperature, Status>, ParseError>`
where the selected standard/library supports those facilities.

**Production/debug angle**

Add structured parse errors with byte offset and reason. Fuzz every version,
type, and length combination. Place hard limits on lengths before allocation.
Test round trips only after testing malformed input.

**Common traps**

- Equating protocol enum numbers with internal enum representation.
- Updating the union before all payload validation succeeds.
- Using a default branch that silently accepts malformed data.
- Allocating based on an unchecked external length.

**Follow-up questions**

- How would you support an unknown but skippable message?
- What state should `out` have on failure?
- How would ownership change for a text payload?
- Should decoding modify output incrementally?

### 16. Compare C arrays, strings, structs, unions, and enums with their modern C++ alternatives.

**Short answer**

C exposes representation and lifetime contracts directly. Modern C++ types can
encode ownership, extent, active alternatives, and stronger type separation,
but they still require lifetime, bounds, invalidation, and external-format
reasoning.

**Deep explanation**

| Need | C | Modern C++ |
| --- | --- | --- |
| Fixed sequence | `T[N]` | `std::array<T, N>` |
| Dynamic owning sequence | allocation + count | `std::vector<T>` |
| Non-owning contiguous view | pointer + count | `std::span<T>` |
| Owned text | buffer + capacity policy | `std::string` |
| Read-only text view | pointer + length | `std::string_view` |
| Data aggregate | `struct` | `struct`/`class` |
| Variant value | enum + union | `std::variant` |
| Strong scoped state | C enum plus validation | `enum class` |
| Alias | `typedef` | `using` |

The C++ alternatives improve expressiveness but are not magic:

- `std::span` and `std::string_view` can dangle;
- `std::vector` reallocation can invalidate views and pointers;
- `std::variant` alternatives can contain owning or non-owning resources;
- native C++ object layout is still not a portable wire format.

**C/C++ code/API anchor**

```c
int sum_c(const int *values, size_t count);
```

```cpp
int sum_cpp(std::span<const int> values);
```

```c
typedef struct {
    ValueKind kind;
    union {
        int integer;
        double real;
    } data;
} Value;
```

```cpp
using Value = std::variant<int, double>;
```

**Production/debug angle**

Choose types that make ownership and bounds visible at interfaces. At C/C++
boundaries, define ABI, allocation family, exception behavior, and lifetime
explicitly. Continue using sanitizers, fuzzing, and representation tests.

**Common traps**

- Saying C++ containers eliminate all bounds or lifetime bugs.
- Returning `std::string_view` to destroyed storage.
- Exposing C++ standard-library types directly through a C ABI.
- Assuming `std::variant` is layout-compatible with a C union.

**Follow-up questions**

- When is a C array still appropriate in C++?
- What invalidates a `std::span`?
- How would you expose a C++ implementation through a C API?
- Why is `enum class` not runtime input validation?

## Coding Tasks

### 17. Coding task: implement a bounded C text formatter

**Prompt**

Implement:

```c
typedef enum {
    FORMAT_OK,
    FORMAT_TRUNCATED,
    FORMAT_ERROR
} FormatResult;

FormatResult format_sensor(
    char *destination,
    size_t capacity,
    unsigned int sensor_id,
    int value);
```

The output format is `sensor=<id> value=<value>`. The destination must be a
valid empty string when capacity is nonzero, even on error.

**Short answer**

Validate the pointer/capacity contract, initialize the destination when
possible, call `snprintf`, and classify negative, truncated, and complete
results.

**Deep explanation**

The function must define behavior for:

- `destination == NULL`;
- `capacity == 0`;
- formatting failure;
- exact fit;
- truncation.

`snprintf` reports the required character count excluding the terminator.

**C/C++ code/API anchor**

```c
#include <stddef.h>
#include <stdio.h>

typedef enum {
    FORMAT_OK,
    FORMAT_TRUNCATED,
    FORMAT_ERROR
} FormatResult;

FormatResult format_sensor(
    char *destination,
    size_t capacity,
    unsigned int sensor_id,
    int value)
{
    if (destination == NULL || capacity == 0U) {
        return FORMAT_ERROR;
    }

    destination[0] = '\0';

    int written = snprintf(
        destination,
        capacity,
        "sensor=%u value=%d",
        sensor_id,
        value);

    if (written < 0) {
        destination[0] = '\0';
        return FORMAT_ERROR;
    }

    if ((size_t)written >= capacity) {
        return FORMAT_TRUNCATED;
    }

    return FORMAT_OK;
}
```

C++ would normally return an owning `std::string` or format into a managed
buffer, depending on the selected standard and error policy.

**Production/debug angle**

Test capacity values `0`, `1`, exact required size, and one byte too small.
Compile with `-Wformat=2` and run ASan. Decide whether preserving truncated text
is desirable or whether the API should clear it.

**Common traps**

- Casting a negative return to `size_t` before checking it.
- Using `>` instead of `>=` for truncation.
- Forgetting that required size includes one more byte for `'\0'`.
- Claiming `snprintf` means no return-value handling is needed.

**Follow-up questions**

- How would the API report required capacity?
- Should capacity zero be allowed as a sizing query?
- Should truncated output remain visible?
- How would you unit-test integer extremes?

### 18. Coding task: implement a tagged-union printer

**Prompt**

Print integer, real, and pointer-length text alternatives. Reject unknown tags
and do not assume text is null-terminated.

**Short answer**

Switch on the validated tag and use length-bounded output for the text view.

**Deep explanation**

The text alternative is a byte range, not necessarily a C string. `printf("%s")`
would require a terminator. The implementation must also reject a null data
pointer when length is nonzero.

**C/C++ code/API anchor**

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
        struct {
            const char *data;
            size_t length;
        } text;
    } data;
} Value;

static int print_value(const Value *value)
{
    if (value == NULL) {
        return 0;
    }

    switch (value->kind) {
    case VALUE_INTEGER:
        printf("%d\n", value->data.integer);
        return 1;
    case VALUE_REAL:
        printf("%.3f\n", value->data.real);
        return 1;
    case VALUE_TEXT:
        if (value->data.text.data == NULL
            && value->data.text.length != 0U) {
            return 0;
        }
        if (value->data.text.length != 0U) {
            fwrite(value->data.text.data,
                   1U,
                   value->data.text.length,
                   stdout);
        }
        fputc('\n', stdout);
        return 1;
    default:
        return 0;
    }
}
```

C++ can use `std::variant<int, double, std::string_view>` and `std::visit`.

**Production/debug angle**

Check I/O errors if output reliability matters. Test unknown tags, empty text,
embedded zero bytes, and dead text storage. The pointer-length pair still has a
lifetime contract.

**Common traps**

- Printing pointer-length text with `%s`.
- Reading the payload before checking the tag.
- Accepting a null pointer with nonzero length.
- Assuming tag validation proves pointer lifetime.

**Follow-up questions**

- How would you return I/O errors?
- How would ownership change for allocated text?
- Can embedded zero bytes be printed by this implementation?
- What would a deep-copy operation require?

### 19. Coding task: allocate and clone a flexible-array message

**Prompt**

Implement creation and cloning for:

```c
typedef struct {
    uint16_t type;
    size_t length;
    unsigned char payload[];
} Message;
```

**Short answer**

Check total-size overflow, allocate one block, initialize the header, and copy
exactly the validated payload length.

**Deep explanation**

The source clone operation needs a trustworthy invariant:

- `source` points to a complete live allocation;
- `source->length` matches accessible payload storage.

An API that receives only `Message *` cannot independently prove the actual
allocation size. Boundary validation must occur when the object is constructed
from untrusted data.

**C/C++ code/API anchor**

```c
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint16_t type;
    size_t length;
    unsigned char payload[];
} Message;

static Message *message_create(
    uint16_t type,
    const unsigned char *payload,
    size_t length)
{
    if ((payload == NULL && length != 0U)
        || length > SIZE_MAX - sizeof(Message)) {
        return NULL;
    }

    Message *message = malloc(sizeof(Message) + length);
    if (message == NULL) {
        return NULL;
    }

    message->type = type;
    message->length = length;
    if (length != 0U) {
        memcpy(message->payload, payload, length);
    }
    return message;
}

static Message *message_clone(const Message *source)
{
    if (source == NULL) {
        return NULL;
    }

    return message_create(
        source->type,
        source->payload,
        source->length);
}
```

C++ would usually model this with a value type containing
`std::vector<std::byte>`.

**Production/debug angle**

Set product-specific maximum payload lengths before allocation. Test zero
length, overflow, allocation failure, and malformed external lengths. Use one
documented `free` owner.

**Common traps**

- Allocating only `sizeof(Message)`.
- Omitting the addition overflow check.
- Calling `memcpy` with a null pointer and nonzero length.
- Believing clone can prove the source allocation's true size.

**Follow-up questions**

- How would you add an element count larger than bytes?
- What output state should creation expose on failure?
- How would you deserialize this safely?
- Would `offsetof(Message, payload)` be useful?

## Debugging Scenarios

### 20. Debugging scenario: AddressSanitizer reports a stack-buffer-overflow in `strlen`

**Scenario**

```c
char code[4] = {'O', 'K', 'A', 'Y'};
printf("%zu\n", strlen(code));
```

**Short answer**

`code` has no null terminator. `strlen` reads beyond the four-element array
while searching for `'\0'`, causing undefined behavior.

**Deep explanation**

The problem begins before `strlen`: the object is a character array but not a C
string. `strlen` has no capacity argument and assumes its input is already
terminated.

The fix depends on intent:

- make a string by adding capacity and a terminator;
- keep it as fixed-width bytes and use the known count.

**C/C++ code/API anchor**

String:

```c
char code[5] = {'O', 'K', 'A', 'Y', '\0'};
printf("%zu\n", strlen(code));
```

Fixed-width bytes:

```c
fwrite(code, 1U, sizeof code, stdout);
```

C++ can represent fixed bytes with `std::array<char, 4>` or a bounded text view
with explicit length.

**Production/debug angle**

Inspect exactly four or five bytes in GDB:

```text
x/5bx code
```

Trace where the buffer's string invariant should have been established. Add
tests for exact capacity and missing terminator.

**Common traps**

- Increasing the array without initializing the new terminator.
- Replacing `strlen` with another terminator-based API.
- Assuming ASan means `strlen` itself is defective.
- Treating random nearby zero bytes as valid termination.

**Follow-up questions**

- Why might the code appear to work without ASan?
- How would you represent a fixed four-byte protocol field?
- Can `strnlen` fully repair a broken API contract?
- Where should termination be established?

### 21. Debugging scenario: a tagged union prints a huge integer after storing a real value

**Scenario**

```c
Value value = {
    .kind = VALUE_INTEGER,
    .data.integer = 42
};

value.data.real = 3.5;
print_value(&value);
```

**Short answer**

The payload was changed to the real representation, but the tag remained
`VALUE_INTEGER`. The dispatcher reads the wrong union member.

**Deep explanation**

Tag and payload form one invariant. Writing `data.real` replaces the shared
storage representation, while `kind` still directs code to `data.integer`.

The correct design updates both together through a constructor or setter:

```c
value = value_from_real(3.5);
```

**C/C++ code/API anchor**

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

C++ `std::variant` tracks the selected alternative and makes this class of tag
desynchronization harder.

**Production/debug angle**

Set watchpoints on `value.kind` and the union storage. Log tag transitions at
parse and dispatch boundaries. Fuzz sequences of mutations, not only isolated
values.

**Common traps**

- Calling the observed integer "random garbage" without identifying the broken
  invariant.
- Fixing only the print function.
- Assuming a non-null pointer payload would therefore be valid.
- Exposing union members for unrestricted mutation across a large codebase.

**Follow-up questions**

- How would you prevent direct mutation?
- What if one alternative owns allocated memory?
- Should unknown tags be fatal?
- How would you test all transition paths?

### 22. Debugging scenario: a binary header works on one target and fails on another

**Scenario**

```c
struct Header {
    uint8_t version;
    uint32_t length;
};

const struct Header *header = (const struct Header *)bytes;
```

**Short answer**

The code depends on native alignment, padding, endianness, and representation.
The second target exposes one or more assumptions. Decode explicit bytes after
checking bounds.

**Deep explanation**

Likely failures include:

- `length` starts at a padded offset rather than byte 1;
- `bytes` is not aligned for `struct Header`;
- target byte order differs;
- input is shorter than `sizeof(struct Header)`;
- packed layout changes access safety but not byte order.

The format should define byte offsets independently of the native structure.

**C/C++ code/API anchor**

```c
static int decode_header(
    const unsigned char *bytes,
    size_t size,
    struct Header *out)
{
    if (bytes == NULL || out == NULL || size < 5U) {
        return 0;
    }

    out->version = bytes[0];
    out->length =
        ((uint32_t)bytes[1] << 24)
        | ((uint32_t)bytes[2] << 16)
        | ((uint32_t)bytes[3] << 8)
        | (uint32_t)bytes[4];
    return 1;
}
```

C++ still needs explicit decoding; a cast or `std::bit_cast` is not an endian
conversion.

**Production/debug angle**

Compare `sizeof`, `_Alignof`, and `offsetof` on both targets. Inspect input bytes
and run UBSan for alignment where available. Add golden test vectors with
specified byte order.

**Common traps**

- Adding `packed` and declaring the problem solved.
- Reading fields before validating total input size.
- Assuming `uint32_t` implies big-endian storage.
- Blaming optimization without checking representation assumptions.

**Follow-up questions**

- What offset does `offsetof(struct Header, length)` report?
- What risks remain after packing?
- How would you encode the header?
- How should an oversized length be handled?

## Rapid-Fire Traps

### 23. Are these statements correct?

**Short answer**

| Statement | Verdict |
| --- | --- |
| "An array is a pointer." | False |
| "Every character array is a C string." | False |
| "`strncpy` always terminates." | False |
| "`sizeof` on an array parameter returns caller array size." | False |
| "A structure's size is the sum of member sizes." | Not generally |
| "A union remembers the active member." | False |
| "A C enum always occupies four bytes." | False |
| "Packed structures are portable serialization." | False |
| "`typedef` creates a distinct type." | False |
| "`gets` is safe with short expected input." | False |

**Deep explanation**

Every statement drops a required contract: extent, terminator, padding,
active-member tag, implementation-defined representation, or untrusted input
length.

**C/C++ code/API anchor**

Useful anchors are:

```c
sizeof array / sizeof array[0]
strlen(string)
offsetof(Type, member)
_Alignof(Type)
enum + union
snprintf(destination, capacity, ...)
fgets(destination, count, stream)
```

Modern C++ alternatives include `std::array`, `std::vector`, `std::span`,
`std::string`, `std::string_view`, `enum class`, and `std::variant`.

**Production/debug angle**

Turn each slogan into a review question:

- Where is the extent?
- Where is the terminator?
- What is the active tag?
- Which ABI defines this layout?
- How is external input validated?

**Common traps**

- Memorizing the verdict without explaining the mechanism.
- Replacing one unsafe slogan with an absolute C++ safety claim.
- Calling implementation-specific observations language guarantees.

**Follow-up questions**

- Which statement can become true under a documented platform contract?
- Which failures can ASan detect?
- Which failures require ABI documentation?
- Which C++ alternatives remain non-owning?

## Interview Evaluation Checklist

A strong candidate should be able to:

- distinguish array objects from pointer values;
- carry extent and capacity through APIs;
- reason about the null terminator without off-by-one errors;
- explain `strncpy`, `snprintf`, and `fgets` precisely;
- calculate a plausible structure layout for a stated ABI while labeling it as
  implementation-specific;
- reject raw structure equality and serialization as general techniques;
- maintain and debug a tagged-union invariant;
- validate external enum and tag values;
- allocate and copy flexible-array objects with overflow checks;
- explain bit-field and packed-layout portability limits;
- decode binary fields explicitly;
- compare C representations with modern C++ ownership and view types;
- choose warnings, sanitizers, GDB inspection, boundary tests, and fuzzing for
  the failure being investigated.
