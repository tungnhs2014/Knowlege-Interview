# M02 Exercises — Advanced Data Structures & Memory Optimization

> **Status:** APPROVED.

These seven exercises are the canonical, source-preserving restatement of DevLinux Session 03 and Session 04. Their identities, order, scenarios, required interfaces, learning objectives, and acceptance intent are retained. Technical wording is corrected only where a source statement could be mistaken for a universal ISO C, ABI, compiler, or hardware guarantee.

## Common Engineering Baseline

Apply this baseline to every build exercise unless an exercise states a more specific requirement:

- Compile as C99 with `-std=c99 -Wall -Wextra -pedantic -Werror`.
- Use BARR-C-oriented readable style: fixed-width types where a representation contract requires them, descriptive names, braces for controlled statements, and the `p_` prefix for pointer variables where applicable.
- Validate pointers before the API dereferences them and validate array indexes before use.
- Use Doxygen-style documentation as required by the source exercise. Session 04 explicitly requires all functions and data structures to be fully documented.
- Provide a `Makefile` with at least `all` and `clean` targets.
- Run `cppcheck` and `clang-tidy`, then resolve relevant warnings and errors for the submitted configuration.

A clean compiler or static-analysis result is review evidence only. It does not by itself prove MISRA or CERT compliance, safety, security, or portability to every compiler and target. Treat the selected coding-standard references from the source sessions as focused review guidance, not as a full compliance claim.

## Session 03

### S03-E01 — Exercise 1: Endianness Checker using Union

**Source provenance:** DevLinux `session-03.md`, Exercise_1 [build].

#### Scenario and objective

Build a small endianness observation program using a union. Store `0x11223344` in a `uint32_t` member, inspect the first byte through a four-element `uint8_t` member, and report whether the selected build observes a little-endian or big-endian representation.

#### Required declaration

```c
typedef union
{
    uint32_t full_word;
    uint8_t bytes[4];
} endian_checker_u;
```

#### Requirements

1. Use a union containing exactly one `uint32_t` member and one `uint8_t[4]` member.
2. Initialize the observation with `0x11223344` before reading the byte representation.
3. Inspect and print the first byte in increasing memory-address order.
4. Identify the resulting observation as little-endian or big-endian for the selected implementation.
5. Add a block comment that explains memory-address order, least-significant byte (LSB), most-significant byte (MSB), and why byte order matters when handling network or sensor data.

#### Required behavior and acceptance intent

The program must build cleanly and print the stored value, the first observed byte, and the observed byte order. On a little-endian build, the source's illustrative output shows first byte `0x44`; a different target may legitimately show a big-endian observation. Do not require a little-endian result as the universal acceptance condition.

#### Engineering constraints and review notes

- This union is a controlled observation of the selected implementation's object representation. It is not a portable serialization or protocol-decoding technique.
- Preserve the source's static-analysis and submission expectations: one `main.c` and one `Makefile` for this exercise identity.
- Review initialization, braces, and the absence of uninitialized reads. The selected MISRA/CERT notes are guidance for this narrow code review only.

#### Submission

```text
Session-03 / Exercise_1
├── main.c
└── Makefile
```

### S03-E02 — Exercise 2: Struct Padding / Alignment / Packed Struct Analyzer

**Source provenance:** DevLinux `session-03.md`, Exercise_2 [build].

#### Scenario and objective

Observe how member order, alignment, padding, and a compiler-specific packing request affect one selected build. This exercise deliberately uses native types so that the learner can see an ABI and target layout rather than impose a fixed-width representation contract.

#### Required data models

Create the source-required original mixed-order structure with members of these types and order:

```text
char → int → double → short
```

Create a reordered variant using the same four member types. Create a third variant of the original mixed order with `__attribute__((packed))` (or the project's equivalent packing mechanism) so that the effect can be observed. `__attribute__((packed))` is a compiler extension; identify the selected compiler support when using it.

#### Requirements

1. Print `sizeof` for the original mixed-order structure.
2. Use `offsetof` from `<stddef.h>` to print the offset of every original member. Use the correct `size_t` output format.
3. Reorder the same members to reduce padding on the selected build, then verify the resulting size and offsets with `sizeof` and `offsetof`.
4. Create and inspect the packed version of the original member order.
5. Perform the source-required direct packed-member access observation.
6. Discuss the trade-off between saved representation space and the access restrictions or cost introduced by packing.

#### Temporary packed-member diagnostic experiment

The source intentionally asks you to take the address of a potentially unaligned packed member and observe a diagnostic. Keep that observation separate from the final strict build:

1. In a temporary negative experiment, take the packed-member address and record the diagnostic produced by the selected compiler, if any.
2. Explain why dereferencing an ordinary pointer to a packed member can be unsafe or inefficient on the selected target.
3. Remove or isolate the intentional warning-producing code.
4. Restore the canonical submitted implementation and confirm that its final C99 build is warning-free under `-Werror`.

Do not submit a final configuration that intentionally produces the packed-member warning.

#### Required observations and discussion

- Values such as 24, 16, and 15 bytes are illustrative examples, not acceptance values. Record the actual sizes and offsets for the compiler, ABI, target, and options used.
- Member reordering is a measured optimization; “largest first” is a hypothesis to verify, not a rule that always eliminates padding.
- An unaligned access may be accepted, expanded into several accesses, slowed down, or fault depending on the target core, instruction, memory attributes or configuration, and compiler-generated code. Do not claim that every unaligned ARM access HardFaults.
- Do not apply `offsetof` to a bit-field; this exercise's original structures use ordinary members.

#### Selected safety and portability notes

Review the `size_t` result types from `sizeof` and `offsetof`, the compiler-extension status of packing, and the selected build's ABI assumptions. Packing is neither an ISO C feature nor an automatically safer or better layout.

#### Acceptance criteria

- The original, reordered, and packed variants use the source-required member types.
- The program reports measured sizes and offsets for the selected build.
- The reordered variant is justified by an observed result, not an assumed universal size.
- The packed-member diagnostic is documented as a temporary experiment, not retained in the final strict build.
- The final submitted build, static analysis, Doxygen documentation, and `main.c`/`Makefile` submission identity meet the common baseline.

#### Submission

```text
Session-03 / Exercise_2
├── main.c
└── Makefile
```

### S03-E03 — Exercise 3: Enum Bitmask Permissions Tester

**Source provenance:** DevLinux `session-03.md`, Exercise_3 [build].

#### Scenario and objective

Create a permission bitmask for Read, Write, Execute, and Delete. Use it to determine whether a user possesses **all** required permissions, then inspect the enum representation for the selected build and discuss the ABI implications of compiler options such as `-fshort-enums`.

#### Required declaration and interface

Use distinct power-of-two flag values with unsigned constants:

```c
typedef enum
{
    PERM_READ = (1U << 0),
    PERM_WRITE = (1U << 1),
    PERM_EXECUTE = (1U << 2),
    PERM_DELETE = (1U << 3)
} sys_perms_e;

bool has_permission(uint8_t user_perms, uint8_t required_perms);
```

Do not provide the `has_permission()` implementation in this exercise specification.

#### Requirements

1. Define the four source-required flags as independent power-of-two values.
2. Implement `has_permission(uint8_t user_perms, uint8_t required_perms)` using bitwise AND so that success means **every** required permission is present.
3. Test successful and unsuccessful cases, including a single required permission and a multiple-permission requirement.
4. Print `sizeof(sys_perms_e)` for the selected build.
5. In a block comment, explain why enum representation can differ across compilers, targets, ABIs, and options; explain the purpose and ABI risk of `-fshort-enums` when separately built components use the same enum interface.

#### Required behavior and acceptance intent

Preserve the source's examples: Read and Write can form a user permission set; checking Read succeeds, checking Execute fails, and checking Read-and-Write succeeds. The program must demonstrate both success and failure behavior. A displayed enum size of four bytes is illustrative only; do not require that value.

#### Engineering constraints and review notes

- Small unsigned operands still undergo integer promotion in expressions. Preserve the source's promotion-learning objective and review any narrowing back to `uint8_t`.
- `-fshort-enums` can change enum representation and can break ABI compatibility when components are compiled with inconsistent options. It is a complete interface/build decision, not a local size tweak.
- Keep the source requirements for Doxygen, strict diagnostics, static analysis, `main.c`, and `Makefile`.

#### Acceptance criteria

- Read, Write, Execute, and Delete are represented by explicit bit flags.
- `has_permission()` enforces all-required-permissions semantics and is tested with both granted and denied cases.
- Enum size is reported as a build observation and the `-fshort-enums` ABI risk is explained.
- The final build and analysis meet the common baseline.

#### Submission

```text
Session-03 / Exercise_3
├── main.c
└── Makefile
```

### S03-E04 — Exercise 4: Packed Union + Struct Bit-fields + Peripheral Union Pattern

**Source provenance:** DevLinux `session-03.md`, Exercise_4 [build].

#### Scenario and objective

Explore nested union packing, a hypothetical 32-bit bit-field representation, and a whole-word/bit-field union view. The objective is to make compiler and target layout dependencies visible, not to create a portable memory-mapped I/O implementation.

#### Task 1 — Nested packed union observation

1. Define a union containing a `uint32_t` member and a five-byte array member.
2. Nest the unpacked union inside a packed structure and print the resulting structure size.
3. Apply the selected compiler's packing attribute directly to the union definition, nest that packed union in a packed structure, and print the size again.
4. Explain the observed nested and tail-padding behavior.

The source's 8-byte and 5-byte examples are illustrative observations, not ISO C guarantees. Packing an outer aggregate does not recursively repack the internal representation of a nested aggregate; inspect both types on the selected compiler and ABI.

#### Task 2 — Hypothetical bit-field register

Create a structure representing a hypothetical 32-bit register with fields for `EN:1`, `MODE:3`, `FLAG:1`, and a 27-bit reserved field. Print `sizeof` for the selected build.

The source hint uses `uint32_t` as the bit-field base type. Acceptance of that spelling and the resulting representation are implementation and toolchain dependent; state the compiler context rather than treating it as a universal C99 declaration.

#### Task 3 — Peripheral union pattern

Create a union that contains a whole-word `uint32_t ALL` view and the bit-field view. Demonstrate the source-required actions:

1. Set `EN` using the bit-field view.
2. Clear the complete value through the `ALL` view.

This is a controlled representation experiment. It does not establish a portable hardware-register definition and must not introduce actual `volatile` or MMIO register access, which belongs to M05.

#### Task 4 — Portability discussion

In a block comment, discuss why portable hardware-register mapping cannot rely on C bit-field allocation alone. Cover the relevant compiler, ABI, and target dependencies:

- bit-field allocation order and field position;
- allocation-unit or storage-unit boundaries;
- padding and alignment; and
- representation and byte order where relevant.

Do not assert that a “strict CMSIS standard” universally forbids struct bit-fields. Instead, explain why CMSIS-style device headers commonly use integer-width register objects with named bit positions and masks: those declarations make the selected target's field positions explicit without relying on C bit-field allocation order.

#### Required behavior and acceptance intent

The source expects a comparison between nested unpacked and directly packed union observations, a size observation for the bit-field structure, an `EN` update through the field view, and a clear through `ALL`. Values such as 8, 5, and 4 bytes or an `ALL` value of `0x00000001` are illustrative for a particular implementation, not portable acceptance values.

#### Selected safety and portability notes

Review the compiler-extension status of packing and the implementation-defined or unspecified layout aspects of bit-fields. The source's union guidance is a focused educational review point; it is not a claim of full MISRA or CMSIS compliance.

#### Acceptance criteria

- Both nested-union layouts are created and measured on the selected build.
- The hypothetical bit-field representation and whole-word union view perform the source-required observations.
- The portability discussion identifies the relevant layout dependencies without making an unsupported universal CMSIS claim.
- The final build, static analysis, Doxygen documentation, and submission identity meet the common baseline.

#### Submission

```text
Session-03 / Exercise_4
├── main.c
└── Makefile
```

## Session 04

### S04-E01 — Exercise 1: Polymorphic Display Driver

**Source provenance:** DevLinux `session-04.md`, Exercise_1 [build].

#### Scenario and objective

Build an encapsulated display component with a small hardware-abstraction interface. Application drawing logic must use the interface rather than concrete display state, so that a Console Display and a simple Dummy Display/test double can supply the same operations.

#### Required public types and interface

Use the source-defined incomplete type and interface:

```c
typedef struct display_config_s display_config_t;

typedef struct i_display_s
{
    void (*init)(display_config_t *p_config);
    void (*draw_pixel)(uint16_t x, uint16_t y, uint8_t color);
} i_display_t;
```

Provide the source-required configuration factory:

```c
display_config_t *console_config_create(uint32_t baud_rate);
```

#### Requirements

1. Keep `display_config_t` incomplete in the public header and define `struct display_config_s` only in the console implementation translation unit.
2. Create Console Display and Dummy Display implementations that both satisfy `i_display_t` exactly.
3. Provide an interface instance for each implementation.
4. Implement `draw_rectangle(i_display_t *p_display)` so that it uses the interface only and requests the source-required 2 × 2 rectangle: four pixels beginning at `(0, 0)`.
5. Validate the display interface pointer and each required function-pointer member before indirect use.
6. The Dummy Display must count `draw_pixel` calls, and the main/application must be able to report the final count of four. Keep the counter implementation-owned and do not expose unrelated implementation state. A documented observation accessor, such as `uint32_t dummy_display_get_draw_count(void)`, is recommended, but another documented observation mechanism that preserves encapsulation is acceptable.
7. Use a multi-file implementation with public interface declarations, concrete implementation files, application code, and a `Makefile`.

#### Opaque-type and configuration requirements

If `console_display.c` completes the same `struct display_config_s` declared by the public `display_config_t` typedef, then a `display_config_t *` already denotes that completed type in that translation unit. Do not cast it merely because callers saw an incomplete type; encapsulation does not require a conversion to another object-pointer type.

The source permits static or dynamically allocated configuration state. Static storage is recommended for this basic exercise. If dynamic allocation is selected, handle allocation failure, document ownership, provide a matching release path, and do not intentionally leak the configuration object.

#### Required behavior and acceptance intent

With the Console Display selected, `draw_rectangle()` must preserve these five source-required output lines and their order:

```text
[Console] Drawing pixel at (0,0) with color 1
[Console] Drawing pixel at (1,0) with color 1
[Console] Drawing pixel at (0,1) with color 1
[Console] Drawing pixel at (1,1) with color 1
Dummy display was called 4 times.
```

Minor environment-specific newline formatting is not a semantic failure; coordinates, color, count, and ordering must match. The documented observation mechanism must let the main/application produce the final Dummy count without exposing unrelated implementation state.

#### Engineering constraints and review notes

- Use a simple dummy/test double only. Do not introduce Unity, CMock, FFF, expectation APIs, TDD, or a mocking framework.
- Function signatures assigned to `i_display_t` must match exactly; do not cast an incompatible function pointer to silence a diagnostic.
- This exercise establishes interface-based decoupling. It does not implement real hardware registers, `volatile` access, or a general callback architecture.

#### Acceptance criteria

- The public header hides the concrete configuration fields.
- Both implementations satisfy the same interface and application drawing code depends only on `i_display_t`.
- Defensive function-pointer validation, the four Console calls, and the Dummy count are demonstrated.
- The multi-file submission, documentation, strict build, and static analysis meet the common baseline.

#### Submission

```text
Session-04 / Exercise_1
├── main.c
├── i_display.h
├── console_display.c
├── console_display.h
├── dummy_display.c
├── dummy_display.h
└── Makefile
```

### S04-E02 — Exercise 2: Object Pool Allocator

**Source provenance:** DevLinux `session-04.md`, Exercise_2 [build], including Exercise_2 Part B [review-only].

#### Scenario and objective

Create a fixed-capacity pool for network packets. The exercise makes capacity, allocation, release, full-pool behavior, ownership, and static-storage resource use visible without using the general-purpose heap.

#### Required type, capacity, and interfaces

Use exactly five packet slots and five occupancy states for this source exercise:

```c
#define POOL_SIZE 5

typedef struct
{
    uint32_t id;
    uint8_t payload[64];
} network_packet_t;

network_packet_t *packet_alloc(void);
void packet_free(network_packet_t *p_packet);
```

Do not use `malloc`, `calloc`, `realloc`, or `free`. The pool and occupancy state must have static storage duration. Keep array indexing bounded by `POOL_SIZE`.

#### Requirements

1. Provide storage for exactly five `network_packet_t` objects and exactly five occupancy states.
2. Allocate an available packet, mark its state in use, and return its address; return `NULL` when all five slots are in use.
3. Release a valid owned slot back to the pool.
4. Handle a `NULL` release according to the documented API behavior.
5. Validate that a non-null packet supplied for release is an exact pool-slot address. A suitable portable teaching approach is equality comparison against each known pool-element address; do not use relational comparison or pointer subtraction between an arbitrary supplied pointer and the pool as the membership test.
6. Document ownership after successful allocation, behavior after release, full-pool failure, `NULL` release, and the behavior for a pointer that is not a pool slot. Do not add a new complex error API.

#### Required tests and observable behavior

Demonstrate the source-required sequence:

1. Five allocations succeed.
2. A sixth allocation returns `NULL` and is reported as a full-pool failure.
3. Release one allocated packet.
4. The next allocation succeeds.

The source's success/failure messages are acceptable output guidance. The required acceptance behavior is the allocation state transition, not one mandatory presentation string.

#### Complexity and fragmentation notes

If the allocation routine scans up to N slots, its algorithmic complexity is O(N) in pool capacity. For this exercise, fixed compile-time `N = 5` gives a known upper bound on slot checks; it does not make the generic scan O(1).

A uniform fixed-size pool avoids external fragmentation **within that pool**. It can still have unused reserved capacity, internal waste, or a full-pool failure. Do not claim that it eliminates all fragmentation or that every safety-critical system universally forbids dynamic allocation; this exercise itself prohibits heap allocation to study the fixed-capacity alternative.

#### Selected safety and portability notes

Review null handling, bounded indexing, ownership after release, and the validity of externally supplied pointer values before dereference. The completed `packet_free()` membership algorithm remains learner work.

#### Acceptance criteria

- The packet type, five-slot capacity, static pool, occupancy state, and required interfaces are preserved.
- The required five-success / sixth-`NULL` / release / successful-reuse sequence is demonstrated.
- Membership validation avoids unrelated-pointer ordering or subtraction.
- Ownership and failure behavior are documented without expanding the API.
- The final build, static analysis, Doxygen documentation, and `main.c`/`Makefile` submission identity meet the common baseline.

#### Part B — Memory Layout Inspection [review-only]

Part B remains inside S04-E02. It is not an eighth exercise and does not receive a separate solution identity.

After building the Exercise 2 binary, run the required source command:

```bash
size exercise_2
```

Use the result to answer the following review questions as comments at the top of `main.c`:

1. Explain why the selected embedded toolchain commonly represents zero-initialized static packet and occupancy storage as BSS-like runtime storage. Distinguish that build convention from ISO C's zero-initialization semantics.
2. Change the packet declaration conceptually to `static network_packet_t s_pool[5] = {0};`. Explain the required zero-initialization semantics, then inspect the selected build rather than asserting that `{0}` always produces either `.data` or `.bss`.
3. Explain why function bodies are commonly associated with text-like code storage and how a typical MCU build may place image content and writable runtime storage differently. State that exact physical Flash/RAM placement depends on the target and linker environment.
4. Increase the capacity conceptually from 5 to 50. Using the source-provided `sizeof(network_packet_t) = 68` bytes, calculate the packet-array growth contribution; separately identify the occupancy/bookkeeping growth contribution and explain why the final binary's BSS summary can also reflect alignment and unrelated static symbols.

`nm` or `objdump -h` may provide useful supplementary observations, but they do not replace the source-required `size` command. Do not provide the numerical Part B answer in this exercise specification.

#### Submission

```text
Session-04 / Exercise_2
├── main.c
├── Makefile
└── optional headers
```

### S04-E03 — Exercise 3: Endian-Safe Protocol Parser

**Source provenance:** DevLinux `session-04.md`, Exercise_3 [build].

#### Scenario and objective

Write firmware for a little-endian ARM Cortex-M4 that receives a six-byte payload from an external sensor over SPI. The sensor transmits both fields in big-endian order. Decode the external byte specification independently of the host object's representation, using explicit byte reconstruction rather than treating a raw byte buffer as a native wider integer object.

#### Required type, interface, and payload

Preserve the source-defined type and function prototype:

```c
typedef struct
{
    uint16_t temperature;
    uint32_t timestamp;
} sensor_data_t;

void parse_sensor_data(const uint8_t *p_buffer,
                       sensor_data_t *p_out_data);
```

The input payload is exactly:

```text
{0x01, 0x2C, 0x00, 0x00, 0x1A, 0x0A}
```

Bytes 0–1 encode the unsigned 16-bit big-endian `temperature`; bytes 2–5 encode the unsigned 32-bit big-endian `timestamp`.

#### Requirements

1. Reconstruct both fields with explicit byte shifts and bitwise OR operations.
2. Do not cast the `uint8_t *` input buffer to a `uint16_t *` or `uint32_t *` for loading the fields.
3. Convert a byte to the intended unsigned destination width before a wide left shift where necessary.
4. Check `p_buffer` and `p_out_data` for `NULL` before dereference. For null arguments, return without an invalid dereference and document the resulting output-state behavior; do not change the `void` return type.
5. Document this required precondition: `p_buffer` refers to at least six readable bytes. The interface has no length parameter, so it can check nullness but cannot independently verify that extent.

#### Required behavior and acceptance intent

For the source-required payload, print or otherwise verify these deterministic numeric results:

```text
temperature: 300
timestamp:   6666
```

The result must be independent of whether the host represents multi-byte integers in little-endian or big-endian order.

#### Alignment, aliasing, and portability notes

Keep three hazards separate in the design discussion:

- **Alignment:** the byte address may not satisfy a wider type's alignment requirement.
- **Effective type / aliasing:** access through an incompatible wider lvalue can violate C object-access rules depending on how the storage was created.
- **Endianness:** even a valid native wider load can interpret the protocol bytes in a host order that differs from the external specification.

Do not claim that strict aliasing itself causes an unaligned hardware fault. Explicit byte reconstruction avoids relying on all three assumptions at once.

#### Acceptance criteria

- The type, prototype, six-byte payload, field layout, and expected numeric results are preserved.
- No wider-pointer cast is used for field reconstruction.
- Shift expressions use the intended unsigned destination width where needed.
- Null handling and the six-readable-byte precondition are documented.
- The final build, static analysis, Doxygen documentation, and `main.c`/`Makefile` submission identity meet the common baseline.

#### Submission

```text
Session-04 / Exercise_3
├── main.c
├── Makefile
└── optional headers
```

## Submission and Review Boundary

The canonical inventory is exactly seven source exercise identities in this order:

1. S03-E01 — Endianness Checker using Union
2. S03-E02 — Struct Padding / Alignment / Packed Struct Analyzer
3. S03-E03 — Enum Bitmask Permissions Tester
4. S03-E04 — Packed Union + Struct Bit-fields + Peripheral Union Pattern
5. S04-E01 — Polymorphic Display Driver
6. S04-E02 — Object Pool Allocator, including Part B
7. S04-E03 — Endian-Safe Protocol Parser

No exercise is added, merged, split, omitted, replaced, or reordered. This document supplies exercise requirements and review context only; it contains no completed solution implementation, no answer key, and no separate solution or interview artifact.
