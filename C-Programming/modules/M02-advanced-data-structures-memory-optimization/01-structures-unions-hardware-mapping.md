# M02-L01 — Structures, Unions & Hardware Mapping

> **Status:** APPROVED.

## 1. Learning Objectives

This lesson prepares you for the four Session 03 exercises without providing their implementations. After completing it, you should be able to:

- use a structure to model related values with different types;
- distinguish a structure's declared member order from its target-specific physical layout;
- use `sizeof` and `offsetof` to inspect a particular build rather than assume a universal layout;
- explain alignment, internal padding, tail padding, and the effect of arrays of structures;
- evaluate member reordering and compiler packing as measured trade-offs;
- explain union shared storage, representation observation, and byte order without treating a union as a serialization format;
- define enumeration constants as bit flags and account for integer promotions in bitwise expressions;
- explain why enumeration size, bit-field layout, and packed layouts depend on the compiler and ABI;
- read a nested compound object through a pointer and recognise the basic meaning of `T **`; and
- describe the peripheral union pattern as a controlled educational representation pattern, not a portable MMIO implementation.

M01 already established fixed-width type rationale, initialization, pointer validation, diagnostics, and the distinction between ISO C semantics and implementation facts. This lesson builds on those foundations rather than reteaching them.

## 2. Why Compound Data Representation Matters in Embedded C

Embedded software often handles a value that is not meaningful in isolation. A sensor sample may include an identifier, timestamp, quality state, and measurement. A configuration item may include a mode, limits, and enable flags. A hardware-oriented design may need to inspect bytes, fields, and a full word while keeping the intended representation visible in code.

Structures, unions, enumerations, and bit-fields give names to those relationships. They improve readability, but they also expose an important boundary: ISO C defines many source-level semantics, while exact bytes, offsets, padding, alignment, and bit placement are often determined by the implementation, ABI, compiler options, and target. Reliable engineers model the data clearly, then measure and verify the built artifact when layout matters.

This is especially important at three boundaries:

- a component interface that passes a structure between translation units;
- an external binary format whose byte order is specified independently of the host CPU; and
- a target-specific register or peripheral convention that is valid only under a documented compiler and hardware contract.

## 3. Structure Fundamentals

### 3.1 A structure is one aggregate object

A `struct` groups named members into one object. The members are declared in a logical order chosen by the designer. They may have different types, which is what makes a structure suitable for a record such as a measurement or configuration item.

```c
#include <stdint.h>

struct measurement
{
    uint8_t channel;
    uint16_t value;
    uint32_t timestamp;
};
```

The structure gives the three values one shared meaning: a `measurement` is not merely three unrelated variables. Member access uses the dot operator for an object and the arrow operator for a pointer to that object.

```c
struct measurement sample = {0U, 0U, 0U};
struct measurement *p_sample = &sample;

sample.channel = 2U;
p_sample->value = 314U;
```

`p_sample->value` is shorthand for `(*p_sample).value`. The pointer must designate a live `struct measurement` object before it is dereferenced; the pointer validation and lifetime rules from M01 still apply.

### 3.2 Logical member order is not a byte-layout promise

For an ordinary structure, member addresses increase in declaration order. The implementation may insert unnamed padding between members and after the final member. It does not insert padding before the first member. Therefore, the source order is a dependable logical order, while exact offsets and total size are observations to obtain from the selected implementation.

Nested compound types make larger models readable without changing that principle.

```c
struct calibration
{
    int16_t offset;
    uint16_t scale;
};

struct channel_config
{
    uint8_t enabled;
    struct calibration calibration;
};
```

The nested `calibration` member is itself a complete structure with its own layout requirements. A change to its type or layout can therefore change the enclosing `channel_config` layout. This is why a structure passed across a binary, ABI, or hardware boundary needs an explicit contract and target verification.

**Embedded/Linux relevance:** structures make device configuration, message state, and component interfaces easier to review. They do not, by themselves, define a network packet format or a hardware register layout.

**Key takeaway:** a structure is a source-level aggregate with ordered members; exact size and offsets are target facts to inspect when they matter.

## 4. Alignment, Padding, and Layout Observation

### 4.1 Alignment is an access requirement

An object type can have an alignment requirement. On a given target, an implementation may require or prefer that an object of a particular type begins at an address divisible by a particular value. The compiler arranges ordinary objects so that their members satisfy the implementation's requirements.

Consider this intentionally mixed-order structure:

```c
struct sample_layout
{
    uint8_t status;
    uint32_t sequence;
    uint16_t count;
};
```

If `uint32_t` needs stricter alignment than `uint8_t` on the selected target, the compiler can insert bytes after `status` before `sequence`. Those bytes are **internal padding**. It can also append **tail padding** after `count` so that every element in an array of `struct sample_layout` begins at a correctly aligned address.

```text
One possible build, not a portable promise:

status | padding | sequence          | count | tail padding
byte 0 | bytes ? | bytes ? through ? | bytes | bytes ?
```

The question is not whether padding is good or bad. Padding is a layout consequence of the ABI and target access rules. A robust program does not send its raw structure bytes to another system merely because the named members look correct in source.

### 4.2 Measure the build with `sizeof` and `offsetof`

`sizeof` reports the number of bytes occupied by an object or type on the implementation that compiles the program. `offsetof`, defined by `<stddef.h>`, reports the offset of an ordinary named member from the start of a structure type. They are observation tools for Session 03, not promises that another compiler, option set, or target will report the same values.

```c
#include <stddef.h>
#include <stdio.h>

static void report_layout(void)
{
    (void)printf("size: %zu\n", sizeof(struct sample_layout));
    (void)printf("status offset: %zu\n",
                 offsetof(struct sample_layout, status));
    (void)printf("sequence offset: %zu\n",
                 offsetof(struct sample_layout, sequence));
    (void)printf("count offset: %zu\n",
                 offsetof(struct sample_layout, count));
}
```

Do not apply `offsetof` to a bit-field; a bit-field has no address that C allows you to take. Also do not replace a layout observation with a guessed arithmetic sum of member sizes: padding and tail padding are exactly what the observation is intended to reveal.

### 4.3 Member ordering is a heuristic to test

Placing members with stricter alignment requirements before smaller members commonly reduces internal padding. For example, a target may lay out `uint32_t`, then `uint16_t`, then `uint8_t` more compactly than the reverse order. This is a useful starting hypothesis for a memory-constrained component, not a universal proof that “largest member first” is always optimal.

The correct workflow is:

1. Preserve the logical/API requirements of the data model.
2. Propose a reasonable ordering where the representation is private.
3. Measure `sizeof` and `offsetof` with the actual compiler, ABI, and options.
4. Re-check every external format or ABI contract before changing member order.

Changing a private structure may be harmless. Changing a structure shared by a bootloader, a separately built library, a persistent image, or a protocol can break a binary contract even when the source still compiles.

**Common misleading assumption:** “The example is 24 bytes on my host, so it is 24 bytes everywhere.” The correct conclusion is only that it is 24 bytes for that build.

## 5. Packed Structures: A Space–Access Trade-off

### 5.1 What a packed request changes

GCC-family toolchains support `__attribute__((packed))` as a compiler extension. When applied to a structure or union definition, GCC documents it as placing members to minimise memory use. That can remove padding that the ordinary layout would contain.

```c
struct __attribute__((packed)) compact_record
{
    uint8_t tag;
    uint32_t counter;
};
```

This spelling is not ISO C. A different compiler may reject it, provide another spelling such as a pragma, or apply different layout rules. A project that uses packing must identify the compiler extension, supported versions, ABI, target, and the reason for accepting the trade-off.

### 5.2 Packing does not recursively pack nested types

Packing an enclosing aggregate does not recursively repack the internal layout of a nested structure or union. The nested type retains its own layout and size unless that type is itself defined with the relevant compiler-specific packing contract.

```c
struct calibration_pair
{
    uint16_t minimum;
    uint8_t state;
};

struct __attribute__((packed)) compact_snapshot
{
    uint8_t source;
    struct calibration_pair calibration;
};
```

Here, `compact_snapshot` requests close placement of its direct members, but it does not change the internal padding of `calibration_pair`. Use `sizeof(struct calibration_pair)` and `sizeof(struct compact_snapshot)` to observe both types for the actual compiler and ABI. If a boundary requires a packed nested type, that requirement must be stated and verified for the nested definition as well.

### 5.3 A smaller object can create a harder access

In an ordinary structure, the compiler normally places `counter` at an address satisfying its alignment needs. In the packed example, `counter` can begin immediately after `tag`. Taking `&record.counter` produces an `uint32_t *` whose pointed-to address may not satisfy the normal alignment required for `uint32_t`.

Compilers can warn when code takes the address of a packed member because later dereference through that pointer may be unsafe or inefficient. The warning is useful: it tells the reviewer that the member's declared type and its possible address no longer naturally agree. Session 03 deliberately asks learners to observe that tension; its future exercise review must separate that negative diagnostic observation from the canonical warning-free build.

Do not replace this reasoning with “ARM always HardFaults on unaligned access.” Whether an unaligned access succeeds, is fixed up, becomes slower, or faults depends on the core, instruction, memory type or attributes, configuration, and the access sequence generated by the compiler. The lesson-level rule is simpler: do not assume that a packed member can safely be used through an ordinary aligned pointer.

### 5.4 When packing is appropriate

Packing is not a default memory optimisation. It can be justified for a specifically defined binary representation, a hardware/vendor ABI with documented layout, or an interoperation boundary that has been measured and tested on every supported target.

**Embedded/Linux relevance:** a small firmware image may make every byte important, while a Linux component may exchange a format with another process or device. In both cases, saving padding is valuable only after the representation contract and access cost are understood.

**Key takeaway:** packed is a compiler-specific layout request, not a portability feature or a blanket optimisation.

## 6. Unions and Shared Storage

### 6.1 One region, several member types

A union contains members that share the same storage region. Its size and alignment must be sufficient for its members, so a union is commonly at least as large and as strictly aligned as its largest/strictest member; trailing padding can still matter. Unlike a structure, its members are not independent stored fields.

```c
union word_bytes
{
    uint32_t word;
    uint8_t bytes[4];
};
```

Writing `word` stores a `uint32_t` representation in the union's shared storage. Inspecting `bytes` is useful as a controlled observation of how the selected implementation represents that word in memory. It does not say what another host will observe.

### 6.2 Representation observation, not a wire-format solution

The union in Session 03 is a compact way to observe the host's byte order. Keep the experiment local: initialize the selected member, inspect the bytes on the actual build, and state the build and target. Do not use a union overlay as a general serializer or deserializer for external data. An external format has its own byte order, field widths, validity rules, and often alignment-independent layout.

For a binary protocol, the more portable engineering question is: “What bytes does the specification require?” The answer should be encoded or decoded explicitly, rather than inferred from the host's union observation.

### 6.3 Nested unions and structures

A union can be nested in a structure, and a structure can be nested in a union. This can express a deliberate representation relationship.

```c
struct status_snapshot
{
    uint8_t source;
    union word_bytes raw;
};
```

The enclosing object still has one normal structure layout, while `raw` offers a shared-storage view within its own member. Measure the combined layout when it crosses an ABI or storage boundary; do not infer it only from the source declarations.

### 6.4 Pointer to a union

A pointer to a union uses the same member-access rule as a pointer to a structure.

```c
union word_bytes observation = {0};
union word_bytes *p_observation = &observation;

p_observation->word = 0x11223344U;
```

`p_observation->word` means `(*p_observation).word`: dereference the valid pointer, then select the union member. This is only a compact pointer-to-compound-object example; M03 develops pointer mastery in depth.

**Key takeaway:** a union makes shared storage explicit. It is useful for a controlled local observation, but it does not eliminate byte-order, layout, or external-format contracts.

## 7. Endianness: Byte Significance and Address Order

Endianness describes the ordering of bytes that represent one multi-byte value in memory. It concerns **byte significance**, not the order in which a decimal value is written in source code.

For the 32-bit value `0x11223344`, one possible memory view is:

```text
Increasing memory addresses →

Little-endian:  0x44  0x33  0x22  0x11
                 least-significant byte      most-significant byte

Big-endian:     0x11  0x22  0x33  0x44
                 most-significant byte       least-significant byte
```

The union experiment can reveal which of these arrangements one host uses for the chosen integer type. It should not be used to decide what bytes an external device sends. A sensor, file format, or network protocol can specify big-endian order even when the CPU that receives it is little-endian.

For externally specified data, use the specification's byte order and explicit byte operations. A small big-endian 16-bit reconstruction illustrates the idea; its caller must already have validated the input extent.

```c
static uint16_t decode_be16(const uint8_t bytes[2])
{
    return ((uint16_t)bytes[0] << 8U) | (uint16_t)bytes[1];
}
```

This code does not depend on the host's in-memory order for `uint16_t`. It is an example of the principle behind Session 04's future protocol parser; that broader parser design belongs to M02-L02 and later serialization work, not to this lesson.

**Common failure:** casting a raw `uint8_t` buffer to a wider integer pointer assumes more than endianness. It can also violate alignment and object-representation assumptions. Explicit byte decoding avoids those representation assumptions.

## 8. Enumerations and Bit Flags

### 8.1 Enumerators give names to states and flags

An enumeration declares named integer constants. For mutually exclusive states, values such as `MODE_IDLE` and `MODE_ACTIVE` improve readability. A different pattern is a set of independent flags, where each named value has one bit set.

```c
enum permission
{
    PERMISSION_READ = (1U << 0),
    PERMISSION_WRITE = (1U << 1),
    PERMISSION_EXECUTE = (1U << 2),
    PERMISSION_DELETE = (1U << 3)
};
```

The power-of-two values can be combined with bitwise OR. A mask can then ask whether every requested permission is present.

```c
uint8_t granted = (uint8_t)(PERMISSION_READ | PERMISSION_WRITE);
uint8_t requested = (uint8_t)(PERMISSION_READ | PERMISSION_WRITE);

if ((uint8_t)(granted & requested) == requested)
{
    /* Every requested flag is present. */
}
```

The cast in this small example makes the intended stored width visible. It is not a substitute for a range contract when values come from external input.

### 8.2 Promotions still apply to small types

Before bitwise operators such as `&`, `|`, and `<<` operate, integer types narrower than `int` undergo the integer promotions. When `uint8_t` exists, it is an exact unsigned 8-bit type, so its complete `0` through `255` range is representable by `int` under the C requirements. A `uint8_t` operand therefore promotes to `int`. The result should be reviewed in that wider type before it is narrowed for storage.

For fixed, small flag values this is usually straightforward, but it explains why an apparently 8-bit expression may be evaluated in a wider type. M01 owns deeper conversion and range analysis; here the practical habit is to choose unsigned constants such as `1U`, use a clearly documented storage type, and check ranges at input boundaries.

### 8.3 Enumeration representation is a build contract

Do not assume `sizeof(enum permission)` is four bytes. The representation of an enum object depends on the compiler, target, ABI, and options. Measure it with `sizeof` for the build that matters.

GCC's `-fshort-enums` option can select a smaller integer representation that covers the enumerators' range. GCC documents that code compiled with this option is not binary compatible with code compiled without it. That matters if an enum appears in a structure, a function interface, a shared object, or an interface to separately built firmware. Treat this option as an ABI decision for the complete boundary, not as an isolated size optimisation.

**Key takeaway:** enum names improve source clarity; enum object size is not a portable protocol or ABI guarantee.

## 9. Bit-Fields: Named Bits with a Layout Contract

A bit-field is a structure or union member declared with a width in bits. It can make a small logical field easier to name.

```c
struct control_bits
{
    unsigned enable : 1;
    unsigned mode : 3;
    unsigned fault : 1;
    unsigned reserved : 27;
};
```

Session 03's design hint uses `uint32_t` as a bit-field base type. In C99, support for bit-field base types beyond the standard permitted integer bit-field types is implementation-dependent. Whether that exact spelling is portable therefore depends on what `uint32_t` denotes and on the implementation/toolchain contract.

The declaration explains the intended logical widths. It does not guarantee that `enable` occupies the least-significant physical bit of a 32-bit register on every compiler and target. Allocation-unit alignment, allocation order, whether fields can straddle allocation units, and some permitted underlying types are implementation-defined or unspecified details. A bit-field also has no ordinary address that the program can take, so it cannot be used with `offsetof` or addressed through a pointer.

This limitation is not a reason to avoid every bit-field. It is a reason to establish a controlled compiler/ABI contract where they are used. The Session 03 example is valuable because it makes the representation question visible. A production interface must verify the actual layout with the toolchain and target rules instead of assuming that the C declaration is a universal hardware map.

CMSIS-style device material commonly supplies integer-width register objects along with named bit positions and masks. That approach keeps field selection explicit and avoids treating C bit-field allocation order as a hardware specification. M05 will cover actual `volatile` register access and target-specific MMIO mechanics; M02 only establishes the representation limitation.

## 10. The Peripheral Union Pattern: Educational, Not Universal

A common teaching pattern places an integer-width whole value beside a bit-field structure in a union:

```c
union control_register_view
{
    uint32_t all;
    struct control_bits bits;
};
```

It lets a learner compare a whole-word view with named fields and ask how the compiler represented the selected target. That is the educational value of Session 03 Exercise 4.

It is not a portable, drop-in peripheral-register definition. The bit-field layout remains implementation dependent, the relationship between the fields and hardware-defined bit positions must be verified, and actual register access has additional target, compiler, and concurrency considerations outside this lesson. A controlled project may choose this pattern after documenting and testing its toolchain contract. M05 owns the decision and implementation of real register access.

## 11. Nested Compound Types and Pointers

Nested structures and unions model a hierarchy without forcing the caller to know every physical byte. A pointer to a compound object lets a function inspect or update the same object instead of copying it.

```c
struct packet_header
{
    uint16_t type;
    uint16_t length;
};

struct packet
{
    struct packet_header header;
    uint8_t payload[8];
};

static uint16_t packet_type(const struct packet *p_packet)
{
    return p_packet->header.type;
}
```

The function's contract must require a valid `p_packet`; M01's null-validation rule still applies. The nested access `p_packet->header.type` is a source-level path through the data model, not a guarantee about a wire-format offset. When the data comes from external bytes, validate the bytes and decode the specified representation instead of assuming a raw buffer is a `struct packet`.

## 12. Minimal Double-Pointer Bridge

Day 3 mentions double-pointer dereferencing, so it must appear here. A `T **` is a pointer to a pointer to `T`.

```c
const char *labels[] = {"idle", "run"};
const char **pp_label = labels;
```

Read the expression in small steps:

```text
pp_label   points to the first pointer in labels
*pp_label  is that first pointer, which points to "idle"
**pp_label is the first character of "idle"
```

One controlled advance demonstrates the Day 3 traversal idea without becoming a pointer-arithmetic lesson:

```c
++pp_label;
```

After that advance, `pp_label` points to `labels[1]`, `*pp_label` points to `"run"`, and `**pp_label` is `'r'`.

This bridge is deliberately small. M03 develops arrays of pointers, complex declarations, pointer-to-pointer buffering, ownership through `T **`, and deeper double-pointer reasoning. M02 uses the concept only so that Day 3's source topic is not omitted.

## 13. Common Failure Patterns and Review Questions

| Review question | Misleading conclusion | Better engineering interpretation |
| --- | --- | --- |
| What is the structure size? | The source example fixes the answer. | Measure `sizeof` and offsets for the target build and ABI. |
| Should members be reordered? | Largest first always produces the best layout. | Reorder only after preserving the data contract and measuring the result. |
| Should the structure be packed? | Fewer bytes is automatically safer or faster. | Identify the compiler extension, access hazards, and boundary contract first. |
| Does a union solve byte order? | The host's union observation defines the protocol. | The protocol defines its bytes; decode or encode those bytes explicitly. |
| Is an enum four bytes? | It is four bytes on one host. | Enum representation is implementation and option dependent. |
| Does a bit-field match the datasheet? | Widths in source fix physical bit positions. | Verify compiler/ABI layout; use the project's documented target approach. |
| Is a pointer to a packed member ordinary? | Its declared type guarantees aligned access. | Its address can be insufficiently aligned for that type. |

Before accepting a compound-data design, ask: What does ISO C guarantee? What does this compiler and ABI decide? What does the external or hardware contract require? What evidence proves those answers for the supported target?

## 14. What to Remember

- Structures group related heterogeneous data, but padding and total size are implementation observations.
- Alignment can create internal and tail padding; arrays make tail padding significant.
- `sizeof` and `offsetof` inspect the selected build. They do not predict every target.
- Reordering members is a measured private-layout optimisation, not a universal recipe.
- `__attribute__((packed))` is a compiler extension and can create unaligned-member hazards.
- A union shares storage. It is useful for controlled representation observation, not portable serialization.
- Endianness is host byte order; an external format chooses its own byte order.
- Bit flags use power-of-two values and bitwise masks; small operands still undergo integer promotions.
- Enum size and bit-field layout are implementation-dependent. `-fshort-enums` is an ABI-wide compiler choice.
- A peripheral union pattern explains representation questions, but real MMIO access belongs to M05.
- `T **` means pointer to pointer; M03 owns the advanced forms and applications.

## 15. Further Reading

- [GCC common type attributes: `packed`](https://gcc.gnu.org/onlinedocs/gcc/Common-Attributes.html)
- [GCC code-generation options: `-fshort-enums`](https://gcc.gnu.org/onlinedocs/gcc/Code-Gen-Options.html)
- [cppreference: C structures](https://en.cppreference.com/w/c/language/struct.html), [unions](https://en.cppreference.com/w/c/language/union.html), and [bit-fields](https://en.cppreference.com/w/c/language/bit_field)
- [Arm CMSIS 6 Peripheral Access](https://arm-software.github.io/CMSIS_6/latest/Core/group__peripheral__gr.html)
