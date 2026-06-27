# Topic Brief 05 - Compound Types In C

## 1. Canonical Routing

| Field | Validated value |
| --- | --- |
| `learning_path_number` | `05` |
| Title | Compound Types In C |
| `slug` | `compound-types-in-c` |
| Requested topic | C arrays, strings, structures, unions, enumerations, aliases, aggregate layout, initialization, and safe use |
| Master source | `master-ch05` |
| Required Notion sources | `notion-3-1`, `notion-3-4`, `notion-3-5`, `notion-3-6` |
| Topic Brief | `coverage/topic-briefs/05-compound-types-in-c.md` |
| Knowledge target | `knowledge/05-compound-types-in-c.md` |
| Interview target | `interview/05-compound-types-in-c.md` |
| Example target | `examples/05-compound-types-in-c/README.md` |

Validation result: the number, title, slug, master source, four mapped Notion
sources, and all canonical output paths match `LEARNING_PATH.md`.

## 2. Source Coverage

### Internal Sources Read

| Source label | Path | Coverage contribution |
| --- | --- | --- |
| `master-ch05` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, CH05 | `MUST / Deep` priority, CH04 prerequisite, keyword scope, mandatory comparisons, memory-layout rule, string-safety rule, and interview focus |
| `guide-section-03` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 3 | MUST-depth requirements and controlled depth for SHOULD/NICE material |
| `guide-section-04` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 4 | Full-chapter structure and required usage angles |
| `guide-section-06` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 6 | Required C versus C++ comparison format |
| `guide-section-07` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 7 | Depth control and practical-example guidance |
| `guide-section-09` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 9 | ISO C, cppreference C, CERT, and embedded reference routing |
| `guide-section-10` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 10 | Technical-English, Markdown, and compile-oriented example rules |
| `guide-section-11` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 11 | Full lesson, interview pack, review guide, and comparison expectations |
| `guide-section-12` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 12 | Required struct, union, enum, string, and array comparisons |
| `guide-section-14` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 14 | Final quality checklist |
| `notion-index` | `docs/C++ Notion/C++ Programming.md` | Notion chapter inventory and source identity validation |
| `notion-3-1` | `docs/C++ Notion/Chapter 3-1 Arrays in C++.md` | Contiguous arrays, initialization, traversal, multidimensional layout, decay, function parameters, character arrays, bugs, and interview prompts |
| `notion-3-4` | `docs/C++ Notion/Chapter 3-4 Strings in C++.md` | Null-terminated strings, common string functions, buffer hazards, input, comparison, C++ string alternatives, lifetime risks, and best practices |
| `notion-3-5` | `docs/C++ Notion/Chapter 3-5 Structures in C++.md` | Aggregate modeling, initialization, nesting, member access, padding, alignment, packing, bit-fields, self-referential structures, and aliases |
| `notion-3-6` | `docs/C++ Notion/Chapter 3-6 Unions, Enumerations, and Type Aliases in C+.md` | Union storage, tagged unions, enum values, aliases, C++ alternatives, low-level use cases, bugs, and interview prompts |

All four mapped Notion chapter files were read. No mapped Notion source was
skipped.

### External References Consulted

| Label | Reference | Validation purpose |
| --- | --- | --- |
| `external-iso-c` | WG14 N3220, public ISO/IEC 9899:2024 working draft: <https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf> | C rules for arrays, string literals, structures, unions, padding, bit-fields, flexible array members, anonymous members, enumerations, aggregate initialization, and library string/I/O functions |
| `external-cppreference-array` | <https://en.cppreference.com/w/c/language/array> | C array declarators, VLAs, parameter adjustment, qualifiers, and decay summary |
| `external-cppreference-string-literal` | <https://en.cppreference.com/w/c/language/string_literal> | C string-literal array type, initialization, storage, and modification rules |
| `external-cppreference-struct` | <https://en.cppreference.com/w/c/language/struct> | C structure members, padding, flexible array members, anonymous structures, and layout summary |
| `external-cppreference-union` | <https://en.cppreference.com/w/c/language/union> | Overlapping storage, active-member considerations, common initial sequences, and union layout |
| `external-cppreference-enum` | <https://en.cppreference.com/w/c/language/enum> | Enumerator types, compatible/underlying types, fixed underlying types in C23, and scope |
| `external-cppreference-strncpy` | <https://en.cppreference.com/w/c/string/byte/strncpy> | Exact bounded-copy behavior, padding, overlap restrictions, and missing-terminator risk |
| `external-cppreference-fgets` | <https://en.cppreference.com/w/c/io/fgets> | Bounded line-input behavior, newline retention, termination, EOF, and error handling |

External validation is required because the mapped chapters teach C++ first
and do not establish exact C version rules for VLAs, flexible array members,
compound literals, designated initialization, union member access, bit-fields,
enumeration representation, or C string APIs.

### Coverage Status

`TOPIC_BRIEF_CREATED`: canonical routing and mapped internal source coverage are
complete, and the principal C-only language gaps were identified and checked
against ISO C and cppreference C. Learner-facing knowledge, interview, and
example outputs were intentionally not created in this step.

## 3. Priority And Dependencies

- Priority: `MUST`
- Depth: Deep
- Prerequisite: CH04 Pointer Mastery
- Required prior mental model: objects, addresses, lifetime, alignment,
  pointer arithmetic, array-to-pointer conversion, bounds, and undefined
  behavior.
- Role in learning path: build safe C data representations and bounded
  interfaces before advanced embedded and industrial C.
- Master-specific rule: emphasize memory layout for all aggregates.
- Master-specific string rule: always include capacity, null termination,
  unsafe functions, and safer bounded designs.
- Controlled advanced scope: ABI-specific layout, packed records, bit-field
  ordering, X-macros, and intrusive data structures must remain clearly
  separated from portable core C.

## 4. Scope And Depth Boundaries

### Deep In This Topic

- One-dimensional and multidimensional arrays, element count, contiguous
  storage, initialization, indexing, and function-parameter adjustment.
- Character arrays, string literals, null-terminated strings, length versus
  capacity, bounded input/output, and common `<string.h>` operations.
- Structures, tags, aliases, nesting, self-reference through pointers,
  aggregate assignment, designated initialization, padding, alignment, and
  member offsets.
- Unions, overlapping storage, tagged-union invariants, initialization, active
  representation, and portable alternatives to type punning.
- Enumerations, enumerator values, scope, representation limits, switch use,
  and C23 fixed underlying types as a version note.
- `typedef`, especially named aggregate types and callback aliases.
- Flexible array members, compound literals, bit-fields, and anonymous
  structures/unions at SHOULD depth.
- Bounds, lifetime, object representation, serialization, and ABI risks.
- Debugging with warnings, sanitizers, `sizeof`, `_Alignof`/`alignof`,
  `offsetof`, static assertions, GDB, and byte inspection.

### Introduce But Defer

- Deep pointer declarators and ownership: CH04.
- Register access, `volatile`, MMIO, callbacks, and embedded FSM architecture:
  CH06.
- MISRA, CERT policy enforcement, static-analysis governance, and organization
  coding standards: CH07.
- C++ classes and object-oriented design: CH09.
- C++ ownership and RAII: CH10.
- Full `std::array`, `std::vector`, `std::string`, `std::string_view`, and
  `std::variant` treatment: CH11 and CH12.
- Portable protocol parsing should be illustrated here, but socket or kernel
  implementation material is outside scope.
- Linux Device Driver and kernel-driver material is excluded.

## 5. Merged Concept Map

### Arrays And Multidimensional Layout

- An array is an object containing a fixed number of contiguously allocated
  elements of one element type. It is not a pointer.
- In most expressions an array converts to a pointer to its first element.
  Important exceptions include `sizeof`, unary `&`, and string-literal array
  initialization.
- Array subscripting is defined through pointer arithmetic, so every access
  requires a valid index within the array.
- An array function parameter is adjusted to a pointer parameter. A written
  bound such as `int a[10]` normally does not carry runtime extent.
- C supports qualifiers and `static` in array parameter declarators to express
  stronger caller contracts, but these do not create automatic bounds checks.
- Multidimensional arrays are arrays of arrays. Their nested array layout is
  contiguous, and all dimensions except the outermost must be known or
  represented by VLA parameters for correct row stepping.
- VLAs are a C99 feature. Their availability and exact support are
  version/implementation-sensitive after C11 and must not be inferred from C++
  rules, where standard VLAs are absent.

### C Strings And Character Buffers

- A C string is a character sequence terminated by a null character. A
  character array without a reachable terminator is a byte buffer, not a valid
  C string for terminator-based APIs.
- Keep three quantities distinct: object capacity, current string length, and
  required output size including the terminator.
- `strlen` scans for a terminator and therefore requires a valid terminated
  string; it cannot discover a buffer's capacity.
- `strcpy` and `strcat` have no destination-capacity parameter. They are safe
  only when the caller has already proved sufficient space and non-overlap.
- `strncpy` copies exactly according to its count rules, may pad the
  destination, and may leave it unterminated. It is not a general
  "safe `strcpy`."
- `snprintf` supports capacity-aware formatting, but its return value must be
  checked for error and truncation.
- `fgets` is suitable for bounded line input. Callers must handle EOF/error,
  possible retained newline, and lines longer than the buffer.
- `gets` cannot be bounded by the destination size and was removed from the C
  standard library in C11.
- In C, a string literal creates an array that must not be modified. Use
  `const char *` for read-only access and `char text[] = "..."` for a mutable
  array copy.

### Structures, Tags, And Aliases

- A structure groups simultaneously live member objects, potentially of
  different types. Members appear in declaration order, but padding can occur
  between members and at the end.
- A structure pointer, suitably converted, points to its initial non-bit-field
  member, and there is no unnamed padding at the beginning.
- The total size is not generally the arithmetic sum of member sizes. Use
  `sizeof`, `_Alignof`, and `offsetof` for the actual implementation.
- Padding bytes can hold unspecified values. Raw `memcmp`, hashing, persistent
  storage, and network transmission of structure representations are not
  generally portable.
- Reordering members can reduce padding, but must not silently break ABI,
  memory-map, file-format, or readability contracts.
- C tags occupy a separate tag namespace. Without a typedef, declarations
  normally use `struct Tag`; `typedef struct Tag Tag;` creates an ordinary type
  alias.
- A structure can contain a pointer to its own incomplete type, enabling linked
  lists and trees, but cannot directly contain itself by value.
- Structure assignment copies member values according to the language rules;
  it does not imply portable copying of padding or separately allocated
  pointees.

### Aggregate Initialization And C-Specific Forms

- Positional aggregate initialization follows member or element order, and
  omitted members receive zero initialization.
- C designated initializers select array elements or named members and are not
  limited by the C++20 declaration-order restriction.
- The mapped C++ note incorrectly calls C++20 designated initializers
  order-independent. C++20 requires designators to follow declaration order.
- Compound literals create unnamed objects with a specified type and
  scope-dependent storage duration. They are useful for temporary aggregate
  values and pointer-based APIs but require lifetime awareness.
- A flexible array member is an incomplete array type permitted as the last
  member of a structure with the required preceding named content. Its storage
  must be supplied by a larger enclosing allocation and calculated with
  overflow checks.
- Structure assignment and `sizeof(struct_type)` do not automatically copy or
  count a separately provided flexible-array payload.

### Unions And Tagged Variants

- Union members overlap in storage. The union is large and aligned enough for
  its members, with possible trailing padding.
- Writing one member changes the representation shared with the others. The
  code must know which representation is valid before interpreting it.
- A tagged union combines an enum discriminator with a union payload. Every
  constructor-like function, assignment path, parser, and dispatcher must
  update and check the tag consistently.
- Reading a member other than the one most recently written has
  C-specific, representation-sensitive rules and must not be presented as a
  portable general type-punning technique.
- Prefer `memcpy` into a correctly typed object for representation transfer,
  plus explicit byte decoding for external data.
- Common-initial-sequence and implementation-specific register-overlay
  techniques require narrow, separately validated treatment.

### Enumerations

- An enum defines an enumerated type and named enumeration constants.
  Enumerators default to zero and increment by one unless explicitly assigned.
- C enumerator identifiers enter the enclosing ordinary identifier namespace;
  name collisions therefore matter.
- Do not assume `sizeof(enum_type) == sizeof(int)` or that every enum has one
  universal representation. Representation and compatibility depend on the
  language version, values, and implementation.
- In pre-C23 C, the compatible integer type is implementation-defined subject
  to representability constraints. C23 adds syntax for a fixed underlying type.
- C enums do not provide C++ `enum class` scoping or strong conversion rules.
  Validate external integers before converting or treating them as enum states.
- Use enums for states, modes, result codes, and tags, but define behavior for
  invalid or future values at API and binary boundaries.

### Bit-Fields, Packing, And Binary Layout

- Bit-fields can compact flags, but allocation unit, ordering, alignment,
  signedness of plain `int` bit-fields, and cross-unit behavior include
  implementation-defined or unspecified aspects.
- A bit-field is not an addressable standalone object; code cannot take its
  address.
- Bit-fields and packed structures are poor default wire-format definitions.
  Field widths alone do not define byte order, bit numbering, padding, or ABI.
- `#pragma pack` and compiler packed attributes are implementation extensions,
  not portable C language mechanisms. They can produce misaligned accesses,
  performance costs, or target faults.
- For protocols and persisted data, decode explicit bytes into ordinary C
  objects and use fixed-width integer types where the external specification
  truly defines those widths.

### Type Aliases

- `typedef` creates an alias, not a distinct type.
- Aliases improve readability for aggregate tags and complex function-pointer
  declarations.
- Pointer aliases can hide indirection and const placement, so names must not
  obscure ownership or mutability.
- Prefer semantic names such as `sensor_id_t` only when the abstraction has a
  real contract; do not use aliases merely to disguise primitive types.

## 6. Required Comparisons

| Comparison | Required conclusion |
| --- | --- |
| C array vs `std::array` vs `std::vector` | C arrays have fixed extent and usually decay; `std::array` preserves fixed size as a value type; `std::vector` owns dynamic contiguous storage |
| Character array vs C string | Every C string needs a terminator, but not every character array is a string |
| C string vs `std::string` vs `std::string_view` | C strings use caller-managed capacity and sentinel termination; `std::string` owns dynamic text; `std::string_view` is a non-owning pointer-length view with lifetime risk |
| Array object vs pointer | The array owns elements and has an extent; a pointer is a separate scalar object and carries no bound |
| `sizeof(array)` vs `sizeof(array_parameter)` | The first can yield the whole array size in its defining scope; the adjusted parameter yields pointer size |
| Structure vs array | A structure groups named, potentially heterogeneous members with padding; an array contains same-type elements at regular contiguous strides |
| `struct` in C vs `struct` in C++ | C structures are data aggregates without member functions or access control and normally require the tag keyword unless aliased; C++ structures are class types with public defaults |
| Structure vs union | Structure members have distinct storage and coexist; union members overlap and require active-representation discipline |
| C union vs C++ union vs `std::variant` | C uses manual tag/invariant management; C++ unions add object-lifetime constraints; `std::variant` manages the active alternative and provides checked access |
| C enum vs C++ `enum class` | C enumerators use enclosing scope and weaker type separation; `enum class` scopes names and prevents implicit integral conversion |
| `typedef` vs C++ `using` | Both can alias ordinary types in C++; `using` has clearer syntax and supports alias templates, while C uses `typedef` |
| Native struct/bit-field overlay vs explicit serialization | Native layout depends on implementation and ABI; explicit encoding defines widths, order, bounds, and endianness |
| `strcpy`/`strcat` vs bounded design | Unbounded APIs require external proof of capacity; bounded APIs carry capacity and report truncation/failure |
| `strncpy` vs `snprintf` for text copy | `strncpy` has padding and termination traps; `snprintf` offers clearer capacity/truncation reporting but still requires return-value checks |

## 7. Usage Angles

### C Usage

- Fixed arrays with explicit element counts.
- Pointer-plus-count and buffer-plus-capacity interfaces.
- Character buffers for bounded text, protocol fields, and C library APIs.
- Structures for records, sensor samples, configuration, parser results, and
  API return bundles.
- Tagged unions for variant messages, command arguments, and parser tokens.
- Enums for state machines, modes, error codes, and union discriminators.
- Flexible array members for single-allocation header-plus-payload objects.
- `typedef` for aggregate and callback readability.

### C++ Usage

- Keep C++ material comparative: `std::array`, `std::vector`, `std::string`,
  `std::string_view`, `enum class`, `using`, and `std::variant`.
- Do not import constructors, destructors, methods, references, exceptions, or
  class access rules into the C mechanism sections.
- Use C++ comparisons to expose ownership, extent, lifetime, and type-safety
  improvements, not to replace understanding of C layout and contracts.

### Embedded Usage

- Fixed sensor sample arrays, lookup tables, ring-buffer storage, configuration
  records, FSM enums, and tagged command payloads are suitable examples.
- Validate every binary layout against the compiler, target ABI, endianness,
  alignment rules, and external specification.
- Prefer masks and shifts over bit-fields when exact register or protocol bit
  positions must be portable across compilers.
- Packed layouts require target-specific evidence and safe access strategies.
- Avoid dynamic flexible-array designs where the product's memory policy
  forbids or cannot bound dynamic allocation.
- Do not use Linux Device Driver or kernel-driver material.

### Enterprise Usage

- Make extent, capacity, used length, termination, ownership, and encoding
  explicit at API boundaries.
- Define one invariant for every tagged union and centralize initialization and
  validation.
- Treat raw aggregate serialization, unchecked string functions, enum values
  received from outside the process, and packed ABI assumptions as review
  hotspots.
- Use static assertions for intentional size/offset contracts and runtime
  parsing for untrusted external data.
- Fuzz parsers and variant dispatch, and run sanitizers over buffer-heavy tests.

## 8. Common Bugs And Failure Modes

| Failure | Mechanism | Review or prevention focus |
| --- | --- | --- |
| Array out-of-bounds access | Index or pointer leaves the array domain | Carry an extent and validate before access |
| Off-by-one write | Terminator or final element exceeds capacity | Include terminator in size calculations |
| Lost array extent | Function parameter adjusted to pointer | Pass count/capacity explicitly |
| Wrong 2D parameter type | Row stride does not match the actual array | Preserve inner dimensions or use a flat buffer contract |
| Uninitialized aggregate member | Partial or absent initialization leaves indeterminate values | Initialize the complete aggregate |
| Unterminated character buffer | No reachable `'\0'` for string API | Track capacity and force/verify termination appropriately |
| `strlen` on non-string data | Scan continues outside the valid object | Keep byte buffers and strings distinct |
| `strcpy`/`strcat` overflow | Destination capacity is too small | Prove capacity or redesign around bounded operations |
| Misused `strncpy` | Result is unterminated or unexpectedly padded | Check semantics; use an operation matching the actual contract |
| Ignored `snprintf` result | Truncation or formatting error is missed | Check negative result and required length against capacity |
| `fgets` line handling bug | Newline, EOF, error, or truncated line is ignored | Normalize and validate input state |
| String comparison with `==` | Addresses are compared instead of contents | Use `strcmp` for C strings |
| Structure size assumption | Padding/alignment differs by target or ABI | Use `sizeof`/`offsetof` and document constraints |
| Raw structure equality | Padding bytes differ despite equal members | Compare members semantically |
| Raw structure serialization | Padding, endianness, widths, or representations differ | Encode fields explicitly |
| Misaligned packed-member access | Packed offset violates member alignment | Copy through bytes or use target-supported access |
| Bit-field layout assumption | Allocation order or signedness differs | Use masks/shifts for externally fixed layouts |
| Wrong union member read | Tag and payload disagree | Validate the discriminator before access |
| Stale tagged-union discriminator | Payload changed without updating tag | Centralize construction and mutation |
| Nonportable union type punning | Representation or member-access rule is assumed | Use `memcpy` or explicit decoding |
| Enum size assumption | Implementation chooses a different representation | Avoid ABI assumptions or specify/version-check where supported |
| Invalid external enum value | Integer does not correspond to a defined state | Validate at the boundary and handle unknown values |
| Flexible-array under-allocation | Header-plus-payload arithmetic is wrong or overflows | Check size arithmetic and use `offsetof` where appropriate |
| Flexible-array copy bug | Ordinary struct assignment omits payload semantics | Copy header and payload explicitly |
| Hidden pointer alias | `typedef` disguises indirection or ownership | Use transparent names and declarations |

## 9. Debugging Notes

- Compile C with an explicit baseline and strong warnings, for example:
  `cc -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wformat=2`.
- Add compiler-supported diagnostics such as `-Warray-bounds`,
  `-Wstringop-overflow`, `-Wstringop-truncation`, `-Wformat-truncation`,
  `-Wswitch-enum`, and `-Wpadded` selectively; availability differs between
  GCC and Clang.
- Use AddressSanitizer for stack/heap/global out-of-bounds access and many
  unterminated-string failures:
  `-fsanitize=address -fno-omit-frame-pointer -g`.
- Use UndefinedBehaviorSanitizer for supported alignment, object-size, shift,
  and enum-related checks: `-fsanitize=undefined`.
- Inspect aggregate contracts with `sizeof`, `_Alignof`, `offsetof`, and
  `_Static_assert`.
- In GDB, use `ptype`, `p sizeof(type)`, `p &object.member`,
  `x/Nbx &object`, watchpoints, and tag/payload inspection.
- Print string length and capacity separately. When a terminator is suspect,
  inspect bounded bytes instead of calling another unbounded string function.
- Record the active union tag at every mutation and dispatch point.
- Test layout-sensitive code with all supported compilers, language modes,
  optimization levels, and target ABIs.
- Fuzz parsers with empty, maximum-length, unterminated, truncated, unknown-tag,
  and malformed-enum inputs.
- Treat a sanitizer finding at string/union use as a symptom; the first bad
  write or stale tag may occur earlier.

## 10. Best Practices

- Treat array extent and string capacity as part of the type-level or API
  contract, even when C syntax cannot encode them completely.
- Use `size_t` for object sizes and counts, with checked arithmetic before
  allocation or concatenation.
- Initialize every aggregate and make omitted-member zero initialization
  deliberate.
- Preserve const qualification for read-only arrays and strings.
- Prefer `fgets` for bounded line input and `snprintf` for bounded formatting,
  with complete result handling.
- Avoid `gets`, and do not present `strcpy`, `strcat`, or `strncpy` as safe
  merely because they are standard functions.
- Keep byte buffers separate from null-terminated string abstractions.
- Use `sizeof array / sizeof array[0]` only where the operand is still an array.
- Use `offsetof` and static assertions only for intentional implementation or
  ABI contracts, not as proof of cross-platform portability.
- Compare structures member by member and serialize fields explicitly.
- Maintain a tagged union through a small set of validated operations.
- Validate enum values received from files, networks, devices, or foreign APIs.
- Prefer masks and shifts for externally specified bit layouts.
- Use packed aggregates only with compiler/target documentation and measured
  need.
- Keep aliases readable and avoid hiding pointer ownership, mutability, or
  array extent.

## 11. Interview Angles

### Junior

- Explain why an array is not a pointer and what array decay means.
- Calculate the element count of a local array and explain why the same
  expression fails in an array parameter.
- Explain why `"Hello"` needs six `char` elements.
- Compare a character array with a valid C string.
- Compare structure and union storage.
- Explain default and explicitly assigned enum values.
- Explain why `gets` is dangerous.

### Middle

- Calculate structure size, internal padding, and tail padding for a stated ABI.
- Design a bounded C string-copy or formatting interface.
- Explain why `strncpy` may not terminate its destination.
- Pass a multidimensional array to a function without losing row stride.
- Design and validate a tagged union.
- Explain flexible-array allocation and overflow checks.
- Explain why bit-fields and raw structures are not portable wire formats.
- Diagnose an off-by-one terminator bug with ASan and GDB.

### Senior

- Review a binary protocol parser for bounds, alignment, endianness, enum
  validation, and union-tag consistency.
- Distinguish portable C layout guarantees from ABI/compiler observations.
- Explain union member-access and type-punning risks without applying C++ rules
  to C or vice versa.
- Design a versioned message type using enum tags and explicit serialization.
- Compare C17 enum representation with C23 fixed underlying types.
- Decide when a packed structure is justified and how to access it safely.
- Review a flexible-array-member API for size arithmetic, ownership, copying,
  and lifetime.
- Explain how a future enum value should be handled across an API or persistent
  format.

## 12. Practice Tasks

### Basic

- Print array element addresses and verify the stride using pointer
  subtraction, not integer-cast address arithmetic.
- Implement bounded traversal with pointer-plus-count.
- Read one line with `fgets`, remove a retained newline, and distinguish
  EOF/error from an empty line.
- Define and initialize nested structures using positional and designated
  initializers.

### Intermediate

- Implement a safe text-formatting wrapper around `snprintf` with explicit
  truncation reporting.
- Measure several structure layouts with `sizeof`, `_Alignof`, and `offsetof`,
  then explain every padding byte.
- Implement a tagged union for integer, floating, and bounded-text values.
- Create a flexible-array message object with checked header-plus-payload
  allocation.
- Encode and decode a fixed binary record without casting a byte buffer to a
  structure pointer.

### Advanced

- Fuzz a tagged-union protocol parser and verify unknown-tag behavior.
- Compare normal and packed aggregate access on supported targets and inspect
  compiler output without generalizing the result.
- Build an enum-to-string mapping with an X-macro, then assess readability and
  maintenance tradeoffs.
- Implement an intrusive singly linked list with explicit ownership and
  container lifetime rules.
- Demonstrate a union type-punning assumption in an isolated test, then replace
  it with `memcpy` or explicit byte conversion.

## 13. Gaps, Corrections, And External Validation Needs

### Notion Corrections Required

- Replace "array name is a pointer" with the array-to-pointer conversion model
  and its exceptions.
- Do not state that every array size must be a compile-time constant in C.
  C has VLA rules that differ by language version and implementation support.
- Do not demonstrate byte spacing by converting pointers to `long long` and
  subtracting integers. Use pointer subtraction within the array.
- Do not claim a local array is necessarily "allocated on the stack." Its
  automatic storage duration is the language rule; physical placement is an
  implementation detail.
- Do not recommend returning a raw dynamically allocated array as the general
  fix for returning a local array. In C, prefer caller-provided storage or a
  clearly owned create/destroy API.
- Qualify fixed pointer sizes, structure sizes, and address examples as
  implementation observations.
- Replace absolute claims that C strings are faster, stack-only, or always
  lower overhead. Storage duration, allocation, workload, and implementation
  determine those properties.
- Do not call `strncpy` safe without explaining non-termination and padding.
- Do not claim an overflowing `strcpy` necessarily crashes; it has undefined
  behavior.
- Correct the C++20 designated-initializer example: member designators must
  follow declaration order.
- Do not state that padding exists only because aligned access is faster.
  Some targets require alignment for valid access, and layout is an ABI choice.
- Do not state that ordering members largest-to-smallest always produces the
  minimum structure size. Measure the actual layout and preserve contracts.
- Do not describe `#pragma pack` as portable C or as simply "removing all
  padding."
- Do not recommend bit-fields as portable hardware-register or protocol
  mappings without compiler/ABI validation.
- Do not use a union color/register overlay as a portable endian-independent
  representation.
- Replace "inactive union member contains garbage" with precise
  representation- and language-rule discussion.
- Do not assume a C enum has the size of `int`.
- Separate C23 fixed-underlying-type syntax from pre-C23 C and from C++
  `enum class`.
- Clarify that `typedef` creates an alias, not a new distinct type.

### C-Only Gaps Requiring External Material

- VLA and variably modified type rules.
- `static`, `restrict`, and qualifiers in array parameters.
- C string-literal type and modification behavior.
- Exact `strlen`, copy, concatenation, comparison, formatting, and line-input
  preconditions.
- C tag namespaces and `typedef struct` conventions.
- C designated initializers and compound literals.
- Flexible array member constraints, allocation, `sizeof`, assignment, and
  payload handling.
- Anonymous structures/unions and version/extension boundaries.
- Union member access, common initial sequences, object representation, and
  type-punning limits.
- Bit-field base types, signedness, addressability, allocation, and ordering.
- Enumeration compatible types and C23 fixed underlying types.
- Compiler-specific packing and ABI behavior.

### Further External Validation Needed During Lesson Generation

- Select a primary teaching baseline, recommended as C17 with clearly labeled
  C23 notes, and compile every example in the corresponding modes.
- Add SEI CERT C references for production guidance on array bounds, string
  handling, object representation, and integer size calculations.
- Validate GCC/Clang warning names and sanitizer commands in the repository
  environment before publishing them as runnable instructions.
- Use compiler and target ABI documentation for any concrete packed-layout,
  bit-field, or foreign-function interface example.
- Use the external protocol specification, not native C layout, for any binary
  message example.
- No POSIX, Linux Device Driver, or kernel-driver reference is needed for the
  core topic.

## 14. Output Targets

| Output | Current status | Intended scope |
| --- | --- | --- |
| `coverage/topic-briefs/05-compound-types-in-c.md` | Created | Source audit, merged concept map, comparisons, bugs, debugging plan, interview angles, gaps, and external validation trace |
| `knowledge/05-compound-types-in-c.md` | Not created in this step | MUST-depth learner-facing C lesson without audit metadata |
| `interview/05-compound-types-in-c.md` | Not created in this step | Junior, middle, and senior interview pack |
| `examples/05-compound-types-in-c/README.md` | Not created in this step | Compile-oriented C examples, layout inspection, sanitizer exercises, and debugging workflow |

Audit metadata must remain in `coverage/` and must not be pasted into
learner-facing documents.
