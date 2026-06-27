# 03 - C Memory Model: Examples

## Status

These are **learning-focused examples**, not production-ready libraries or
application infrastructure. They keep allocation, lifetime, representation,
and diagnostic behavior visible.

The successful examples:

- use C17 and strict warnings;
- check allocation results and allocation-size arithmetic;
- use one clear owner for each allocation;
- preserve the original allocation when `realloc` fails;
- decode external bytes without unaligned structure casts;
- inspect object representation only through `unsigned char`;
- contain no concurrency, exceptions, iterators, races, or deadlocks.

Files under `unsafe/` intentionally execute undefined behavior only through
isolated expected-failure targets. Never copy those defects into production
code.

## Requirements

The complete workflow requires:

- GNU Make;
- GCC or Clang with GCC-compatible warning flags;
- AddressSanitizer and UndefinedBehaviorSanitizer;
- GNU `size`, `nm`, `readelf`, and `objdump` for ELF inspection;
- optionally GDB and Valgrind.

```bash
cc --version
make --version
size --version
nm --version
readelf --version
objdump --version
```

Build, run, and validate everything:

```bash
make check
```

Use another compatible compiler:

```bash
make check CC=clang
```

Remove generated artifacts:

```bash
make clean
```

All generated files are placed under `build/`.

## Example Map

| Target | Demonstrates |
| --- | --- |
| `layout` | Static, automatic, and allocated storage observations |
| `allocation` | Overflow-checked allocation, ownership, and safe `realloc` |
| `protocol` | Alignment, padding, byte representation, and big-endian parsing |
| `inspect` | ELF text/data/BSS sections and symbols |
| `debug` | Unoptimized builds with debug symbols |
| `sanitize` | Safe programs under ASan and UBSan |
| `expected-failures` | ASan-detected heap use-after-free and buffer overflow |

## 1. Observe A Concrete Memory Layout

Build and run:

```bash
make layout
./build/layout
```

The program prints addresses for:

- a zero-initialized file-scope object;
- an initialized file-scope object;
- read-only data;
- an automatic object;
- allocator-managed storage.

These addresses describe one process execution. They do not prove that C
requires a particular segment order or growth direction.

Inspect the executable:

```bash
make inspect
```

Useful manual commands:

```bash
size build/layout
nm -S build/layout
readelf -S -l -s build/layout
objdump -h -t build/layout
```

Look for the initialized global in a data symbol class and the zero-initialized
global in a BSS-like symbol class. Exact names and placement are
toolchain-specific.

## 2. Checked Ownership And Dynamic Growth

```bash
make allocation
./build/allocation
```

The `IntArray` example demonstrates:

- `data == NULL`, `size == 0`, `capacity == 0` as the empty state;
- multiplication-overflow checks before allocation;
- a temporary pointer for `realloc`;
- state updates only after successful growth;
- one destroy function that releases ownership and resets the state.

This is a useful teaching implementation, but a production container also
needs a richer error model, allocator policy, API tests, documented borrowing
rules, and possibly deterministic capacity limits.

**Ownership warning:** every pointer borrowed from `array.data` becomes invalid
after a successful growth operation that reallocates storage. Do not retain
element pointers across `int_array_push`.

## 3. Alignment, Padding, And Protocol Bytes

```bash
make protocol
./build/protocol
```

The program:

- prints `sizeof`, `alignof`, and `offsetof` for a structure;
- inspects a `uint32_t` representation through `unsigned char`;
- reports the host's observed byte order;
- parses a big-endian packet without casting the byte buffer to a structure.

The wire format is:

```text
byte 0..1: payload length, unsigned big-endian 16-bit
byte 2:    message type
byte 3:    flags
byte 4..:  payload
```

The parser validates length before each field and keeps the wire representation
independent of native structure padding, alignment, and endianness.

**Production note:** a real protocol needs explicit versioning, maximum payload
size, error codes, semantic field validation, and fuzz/regression tests.

## 4. Debug Builds

```bash
make debug
gdb ./build/debug-allocation
```

Useful GDB commands:

```text
break int_array_reserve
run
bt
frame 0
info args
info locals
print *array
print new_capacity
```

For a memory address:

```text
p/x array->data
x/32bx array->data
```

Optimized builds can inline functions, reuse storage, and report variables as
optimized out. Reproduce defects both with a debug build and the production
optimization level.

## 5. Sanitizer Builds

Build and run the safe examples with AddressSanitizer and
UndefinedBehaviorSanitizer:

```bash
make sanitize
./build/sanitize-layout
./build/sanitize-allocation
./build/sanitize-protocol
```

Instrumentation flags:

```text
-fsanitize=address,undefined
-fno-omit-frame-pointer
```

Sanitizers do not prove memory safety. They inspect executed paths and do not
replace static analysis, code review, boundary tests, or target validation.

## 6. Intentional Undefined Behavior

Run the isolated expected failures:

```bash
make expected-failures
```

The target passes only when ASan rejects:

1. heap use-after-free;
2. heap buffer overflow.

Diagnostics are saved under:

```text
build/failures/use-after-free.log
build/failures/out-of-bounds.log
```

Run one case manually only when studying the report:

```bash
./build/unsafe-use-after-free
./build/unsafe-out-of-bounds
```

These programs intentionally have undefined behavior. Their output, exit code,
and exact diagnostic text are not language guarantees.

## 7. Valgrind

Run safe examples:

```bash
valgrind --leak-check=full --show-leak-kinds=all ./build/allocation
valgrind --leak-check=full --show-leak-kinds=all ./build/protocol
```

Study an unsafe case with the non-sanitized compiler command if needed:

```bash
cc -std=c17 -g3 -O0 unsafe/use_after_free.c -o build/valgrind-use-after-free
valgrind --track-origins=yes ./build/valgrind-use-after-free
```

Valgrind availability and behavior are platform-specific. It does not replace
testing on an embedded target.

## 8. Safety And Production Notes

- **Undefined behavior:** only `unsafe/` intentionally executes UB, and only
  under expected-failure workflows.
- **Ownership:** `IntArray` owns exactly one allocation. Call
  `int_array_destroy` once when ownership ends.
- **Allocation failure:** successful examples preserve valid state and return
  failure without dereferencing null.
- **Allocation-size overflow:** capacity and byte multiplication are checked
  before allocator calls.
- **Pointer invalidation:** successful `realloc` can invalidate every old
  pointer into the allocation.
- **Structure layout:** no raw structure comparison, hashing, persistence, or
  wire transmission is used.
- **Unsafe C APIs:** no `gets`, unbounded string copy, or unbounded `%s` input is
  used.
- **Exception safety and iterator invalidation:** C exceptions and iterators are
  not applicable. The analogous pointer-invalidation risk is documented.
- **Concurrency:** examples contain no shared concurrent access. Adding threads
  would require a separate synchronization and ownership design.
- **Production readiness:** real code still needs project-specific error
  reporting, tests, static analysis, allocation policy, performance
  measurements, and supported-target validation.

## 9. Suggested Exercises

1. Add `int_array_pop` without shrinking capacity.
2. Add an explicit maximum capacity suitable for an embedded product.
3. Inject allocation failures and verify that `IntArray` remains unchanged.
4. Extend the protocol with a big-endian 32-bit sequence number.
5. Fuzz every truncated packet length from zero through the full message size.
6. Reorder `struct LayoutSample` members and compare size and offsets.
7. Compare `-O0`, `-O2`, and `-Os` binary sizes with `size`.
8. Add compiler stack-usage reports with `-fstack-usage`.
