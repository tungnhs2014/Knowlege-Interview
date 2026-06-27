# 05 - Compound Types In C: Examples

## Status

These are **learning-focused C17 examples**, not production-ready libraries or
protocol implementations. They keep extent, capacity, layout, tags, ownership,
and byte decoding visible.

The safe programs:

- compile with strict warnings;
- pass array extents and string capacities explicitly;
- check `snprintf` truncation and bounded input;
- inspect structure layout without treating it as portable serialization;
- keep tagged-union construction and access consistent;
- check flexible-array allocation arithmetic and ownership;
- decode external bytes without structure casts;
- contain no exceptions, iterators, concurrency, races, or deadlocks.

Files under `unsafe/` intentionally execute undefined behavior only through
isolated expected-failure targets. Never copy those defects into normal code.

## Requirements

- GNU Make
- GCC or Clang with C17 support
- AddressSanitizer and UndefinedBehaviorSanitizer
- Optional: GDB or Valgrind

Build, run, sanitize, and verify the intentional failures:

```bash
make check
```

Use another compatible compiler:

```bash
make clean
make check CC=clang
```

Remove generated binaries and logs:

```bash
make clean
```

All generated files are placed under `build/`.

## Example Map

| Target | Source | Main lesson |
| --- | --- | --- |
| `arrays` | `arrays/array_matrix.c` | Array versus pointer, explicit extent, pointer-to-array, and 2D row shape |
| `strings` | `strings/bounded_text.c` | Capacity-aware formatting, truncation, `fgets`, length, and termination |
| `layout` | `layout/layout.c` | `sizeof`, `alignof`, `offsetof`, padding, and semantic equality |
| `variant` | `variant/tagged_value.c` | Enum discriminator, union payload, bounded text view, and tag validation |
| `protocol` | `protocol/message.c` | Flexible array ownership, cloning, checked allocation, and explicit big-endian decoding |
| `sanitize` | All safe sources | ASan and UBSan runs |
| `expected-failures` | `unsafe/*.c` | Unterminated-string scan and flexible-array under-allocation |

## 1. Arrays And Matrix Shape

```bash
make arrays
./build/arrays
```

The example demonstrates:

- computing a local array extent before decay;
- a pointer-plus-count range API;
- the difference between `int *` and `int (*)[4]`;
- a two-dimensional array parameter that preserves the column count.

**Production note:** a pointer and count are still only a contract. The count
must describe the live object, and arithmetic must stay in range.

## 2. Bounded Text

```bash
make strings
printf 'sensor name\n' | ./build/strings
```

The formatter distinguishes:

- complete output;
- truncated output;
- formatting or invalid-argument failure.

The input path uses `fgets`, detects whether the whole line fit, removes one
newline, and then calls `strlen` only after a valid terminator exists.

**Unsafe API warning:** this suite does not use `gets`, `strcpy`, or `strcat`.
`strncpy` is also not treated as a general safe-copy function because it can
leave the destination unterminated and can pad the remaining count.

**Production note:** a real line reader must define what to do with the
remaining characters after an overlong line and whether truncation is accepted.

## 3. Structure Layout

```bash
make layout
./build/layout
```

The program prints the actual:

- structure size;
- structure alignment;
- member offsets;
- effect of two member orders on this compiler and ABI.

It compares member values semantically rather than using `memcmp`.

The output describes this build only. It is not proof that every compiler,
target, or ABI uses the same padding.

## 4. Tagged Union

```bash
make variant
./build/variant
```

The example centralizes creation of integer, real, and text alternatives. The
text alternative is pointer-plus-length, so it can print a character sequence
that is not null-terminated.

**Lifetime warning:** the text view does not own its characters. Its source must
remain alive and unchanged for every use. A valid union tag does not prove that
a pointer payload is live.

**Production note:** alternatives owning resources need explicit copy, move-like
transfer, cleanup, and failure rules in C.

## 5. Flexible Array And Protocol Bytes

```bash
make protocol
./build/protocol
```

The program:

- decodes a four-byte header with explicit big-endian operations;
- validates the payload length against the available bytes;
- allocates a header-plus-payload object in one block;
- clones the fixed members and payload;
- releases both allocations with `free`.

**Ownership warning:** each returned `Message *` owns one allocation. Copying
only the fixed structure does not clone the flexible payload.

**Production note:** add product-specific maximum lengths, richer error codes,
version and enum validation, allocation policy, and malformed-input tests.

## Debugging

Build unoptimized programs with debug symbols:

```bash
make debug
gdb ./build/protocol
```

Useful GDB commands for layout:

```text
break print_unordered_layout
run
ptype struct Unordered
p sizeof(struct Unordered)
p _Alignof(struct Unordered)
p &first.value
```

Useful commands for the tagged union:

```text
gdb ./build/variant
break print_value
run
p *value
p value->kind
watch value->kind
```

Useful commands for the flexible array:

```text
gdb ./build/protocol
break message_create
run
p length
p sizeof(Message)
p message
x/16bx message
```

## Sanitizers

Run safe programs under AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
make sanitize
```

Run only the intentional failures:

```bash
make expected-failures
```

Reports are stored in:

```text
build/failures/unterminated.log
build/failures/flexible-underallocation.log
```

The first failure calls `strlen` on a four-byte character array with no null
terminator. The second allocates only the fixed flexible-array header and then
writes a payload element.

Sanitizers inspect executed paths. They do not prove that all bounds, tags,
layouts, ownership rules, or external representations are correct.

Valgrind can provide another hosted check when available:

```bash
make all
valgrind --leak-check=full --track-origins=yes ./build/protocol
```

## Production Checklist

- Carry array extent and string capacity through every interface.
- Reserve and verify space for the string terminator.
- Check `snprintf` results for errors and truncation.
- Distinguish byte buffers from null-terminated strings.
- Initialize every aggregate member.
- Use `sizeof`, `alignof`, and `offsetof` to inspect selected builds.
- Compare structures member by member.
- Keep union tag and payload updates together.
- Validate enum and tag values received from outside the process.
- Check flexible-array size arithmetic and define one clear owner.
- Encode and decode external bytes explicitly.
- Treat bit-fields and packed structures as compiler- and target-specific.

These examples do not use C++ exceptions or containers, so exception safety and
iterator invalidation do not apply directly. They also start no threads, so
they demonstrate no race or deadlock behavior. If the same objects are shared
between threads, synchronization and lifetime rules must be designed
separately.

## Practice

1. Add an `average` function to the array example and define empty-range
   behavior.
2. Change the text formatter to report the required capacity.
3. Add a setter that changes a `Value` alternative while preserving the tag
   invariant.
4. Add a maximum accepted payload length to `message_create`.
5. Add `encode_header` and verify decode/encode round trips using fixed golden
   byte sequences.
