# M02 Interview — Advanced Data Structures & Memory Optimization

> **Status:** APPROVED.

These questions are derived primarily from the approved Session 03 and Session 04 exercises, with focused production-review extensions. They assess the M02 lessons; they do not introduce new lesson-scale theory.

## Q1. Why can `sizeof` be larger than the sum of a structure's named members?

### Expected answer

Alignment requirements can cause internal padding between members and tail padding after the final member. `sizeof` and `offsetof` describe the selected compiler, ABI, target, and options; they are not universal constants from the source declaration.

### Strong interview points

Tail padding matters for arrays because every array element must begin at a suitable alignment. A reviewer should measure the build that crosses the relevant module, ABI, or storage boundary rather than manually add member widths.

### Follow-up

- Which evidence would you collect before accepting a layout-sensitive interface?
- Why is `offsetof` unsuitable for a bit-field?

## Q2. Would you always place the largest member first to optimize memory?

### Expected answer

No. Reordering can reduce padding on a selected build, but it is a hypothesis to measure after preserving the logical and external data contract. Reordering a private structure is different from changing a structure used by a binary interface, persistent image, or external format.

### Strong interview points

An engineer compares measured sizes and offsets before and after the change, then checks the target ABI and every affected boundary. A smaller private object is not useful if it silently breaks an interface contract.

### Follow-up

- What target change could make a previously helpful ordering less beneficial?
- Would you approve reordering a structure shared with a separately built library without an ABI review?

## Q3. Would you approve this packed-member access?

```c
struct __attribute__((packed)) record
{
    uint8_t tag;
    uint32_t sequence;
};

uint32_t *p_sequence = &record_instance.sequence;
```

### Expected answer

Not without a documented compiler, ABI, target, and access contract. `__attribute__((packed))` is a compiler extension. The `sequence` member can have an address that does not meet `uint32_t` alignment, so an ordinary `uint32_t *` can be unsafe or inefficient to dereference.

### Strong interview points

Direct member access and taking an ordinary typed member pointer are different cases. The target might handle an unaligned access, expand it, slow it down, or fault; “ARM always HardFaults” is not a valid review conclusion. Use a representation and access method justified by the supported boundary instead of treating packing as a default optimization.

### Follow-up

- What hidden assumption did the pointer declaration introduce?
- How would you obtain a compiler diagnostic for this risk without retaining a warning-producing final build?

## Q4. When should you choose a `struct`, and when is a `union` useful?

### Expected answer

A `struct` stores related members as one aggregate with distinct member storage. A `union` gives members shared storage and is useful for a controlled local representation observation. Neither choice alone defines an external byte format.

### Strong interview points

Writing a whole-word union member and inspecting bytes can show the selected host representation, but it is not portable serialization or protocol decoding. External formats define their own widths, byte order, validity rules, and boundaries.

### Follow-up

- What could change when the same union experiment runs on another target?
- Why is an observed host byte order insufficient to define sensor-payload bytes?

## Q5. Does this permission check enforce every required permission?

```c
if ((user_perms & required_perms) != 0U)
{
    allow_operation();
}
```

### Expected answer

No. It accepts when any requested bit is present. An all-required check compares the masked result with the complete requirement: `(user_perms & required_perms) == required_perms`.

### Strong interview points

Independent flags need distinct power-of-two values. Small integer operands still undergo promotion in bitwise expressions. Enum representation and `sizeof(enum)` are compiler, ABI, target, and option observations; `-fshort-enums` can change interfaces and break ABI compatibility with separately built code using the enum.

### Follow-up

- Which tests distinguish any-required from all-required semantics?
- Would you enable `-fshort-enums` for one component only? Why not?

## Q6. Why is a C bit-field declaration not a universal hardware-register specification?

### Expected answer

Field allocation order, storage-unit boundaries, padding, alignment, and some permitted base types are implementation or toolchain dependent. A declaration whose widths total 32 does not universally place a named field at a particular physical bit.

### Strong interview points

The Session 03 `uint32_t` bit-field spelling itself requires a toolchain contract in C99. A controlled project can verify a selected compiler/ABI layout, but it must not infer that every compiler will match. CMSIS-style device headers commonly use integer-width register objects with named bit positions and masks so software does not rely on bit-field allocation order for exact hardware positions.

### Follow-up

- What build evidence would you require before accepting a controlled bit-field use?
- Why is “CMSIS universally forbids bit-fields” an overclaim?

## Q7. How do host endianness and an external protocol's byte order differ?

### Expected answer

Host endianness describes how one implementation stores a multi-byte object in memory. An external protocol specifies the meaning and order of transmitted bytes independently. A little-endian host can correctly process a big-endian sensor payload by reconstructing the numeric value explicitly.

### Strong interview points

For `0x11223344`, a union observation may show `0x44` at the lowest address on one little-endian host. That observation does not authorize emitting or reading protocol bytes in that order. Explicit shifts and OR operations make byte significance visible in the code.

### Follow-up

- What would you expect to change if the same host-observation program ran on a big-endian target?
- Why should a protocol test vector be expressed as bytes rather than as a native structure image?

## Q8. What is wrong with this raw-payload review candidate?

```c
uint32_t timestamp = *(const uint32_t *)&p_buffer[2];
```

### Expected answer

Reject it. The address may not meet `uint32_t` alignment, the storage may not be valid to access through an incompatible wider lvalue under effective-type/aliasing rules, and the host's byte order may differ from the protocol's byte order. These are three separate hazards.

### Strong interview points

Decode the specified bytes explicitly, converting to the intended unsigned destination width before wide shifts. Null validation is necessary but not enough: the fixed Session 04 parser interface has no length argument, so its caller must establish that at least six bytes are readable.

### Follow-up

- Which of the three hazards can remain even if the address happens to be aligned?
- How would you design a different API if the readable extent were not established elsewhere?

## Q9. What does an incomplete typed structure hide, and what can a caller still do?

```c
typedef struct display_config_s display_config_t;
```

### Expected answer

Callers can declare, copy, pass, test against `NULL`, and compare an opaque `display_config_t *` for equality where appropriate. They cannot access members, instantiate the complete object, or determine its size because the representation is hidden.

### Strong interview points

The typed opaque pointer is not `void *`; it preserves API specificity. When the implementation completes the same `struct display_config_s`, its `display_config_t *` already denotes that completed type there. A cast is not inherently required and would not fix an invalid pointer or a broken lifetime contract.

### Follow-up

- Why is defining the complete structure in the public header an encapsulation risk?
- What lifetime and reuse information should a static opaque-configuration factory document?

## Q10. How does a structure of function pointers provide small-scale C polymorphism safely?

```c
typedef struct
{
    void (*init)(display_config_t *p_config);
    void (*draw_pixel)(uint16_t x, uint16_t y, uint8_t color);
} i_display_t;
```

### Expected answer

Application code can depend on these operations rather than on Console or Dummy Display representation. Each implementation function must have an exactly compatible signature; casting an incompatible function pointer is not a valid repair. Before an indirect call, validate both the interface pointer and the specific required member.

### Strong interview points

Initialization calls additionally need the required configuration and `init` member to be valid. The Dummy Display's private count is useful evidence that application code requested four draws, but it is not proof of display hardware behavior.

### Follow-up

- What fails if `p_display` is non-null but `p_display->draw_pixel` is null?
- Why should `draw_rectangle()` avoid depending on a concrete display type?

## Q11. What makes a fixed packet pool predictable, and what does it not guarantee?

### Expected answer

A static pool fixes the number and size of slots. Successful allocation temporarily transfers one slot's ownership to the caller; release returns it to the pool; exhaustion returns `NULL` as an expected condition. A linear search is O(N), while a fixed `N = 5` gives a known bounded number of checks.

### Strong interview points

Release should compare a supplied non-null pointer for equality with known slot addresses rather than use relational comparison or subtract an arbitrary external pointer. A uniform fixed-size pool avoids external fragmentation within that pool, but it can still reserve unused capacity or become full. Zero-initialized static storage commonly appears in BSS-like storage on a selected embedded build; ISO C does not mandate a `.bss` section, and `= {0}` does not universally force `.data`.

### Follow-up

- How should `packet_free()` handle `NULL`, a non-pool pointer, and an already-free slot?
- Why does a capacity increase require both packet-array and occupancy-bookkeeping budgeting?

## Q12. A data component works on GCC/x86 but fails after porting. How would you investigate it?

### Expected answer

First identify the failing boundary and reproduce it with a known byte vector or interface case. Record the compiler, version, flags, target ABI, and optimization configuration. Then measure the target build's sizes and offsets and inspect assumptions about alignment, padding, packing extensions, bit-field layout, enum representation, host byte order, and the externally specified representation.

### Strong interview points

Do not “fix” the failure by adding a cast or treating the host result as proof. Compare actual emitted or received bytes with the external specification, verify compatible build options across ABI boundaries such as `-fshort-enums`, and replace assumptions with a documented target contract or explicit byte reconstruction. Treat section observations such as BSS-like placement as build evidence, not ISO C placement rules.

### Follow-up

- Which single test would you run first if the failure appears only for one sensor payload?
- Which assumption would you challenge first after a packing-related compiler warning?
