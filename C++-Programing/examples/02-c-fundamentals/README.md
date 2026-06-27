# 02 - C Fundamentals: Examples

## Status

These are **learning-focused examples**, not production-ready libraries or
application infrastructure. They are intentionally small so each C rule remains
visible.

The successful examples:

- use C17;
- compile with strict warnings;
- initialize automatic objects;
- check fallible input and use type-correct output contracts;
- validate narrowing conversions, arithmetic, and shift counts;
- avoid dynamic allocation and ambiguous ownership;
- contain no concurrency, exceptions, iterators, races, or deadlocks.

The `diagnostics/` and `unsafe/` files are intentionally defective. The
Makefile isolates them and asserts the expected compiler or sanitizer failure.
Do not copy those defects into production code.

## Requirements

The full `make check` workflow requires:

- GNU Make;
- GCC or Clang with GCC-compatible warning options;
- AddressSanitizer and UndefinedBehaviorSanitizer support;
- English diagnostics, enforced by the Makefile with `LC_ALL=C`.

```bash
gcc --version
make --version
```

Use another compatible compiler explicitly when available:

```bash
make check CC=clang
```

Run every successful example and assert every intentional failure:

```bash
make check
```

Remove generated artifacts:

```bash
make clean
```

All generated files are placed under `build/`.

## Example Map

| Target | Demonstrates |
| --- | --- |
| `types` | Integer limits, `size_t`, fixed-width I/O, checked signed addition, signed/unsigned boundary checks |
| `storage` | Header declaration, one `extern` definition, file-scope `static`, static local persistence |
| `control` | `switch`, bounded loops, safe reverse iteration, unsigned bit masks, checked shifts |
| `input` | `fgets`, `strtol`, malformed input rejection, range checks |
| `debug` | Debug symbols and an unoptimized build |
| `sanitize` | AddressSanitizer and UndefinedBehaviorSanitizer instrumentation |
| `expected-failures` | Compiler-detected signedness/format defects, rejected input, and UBSan-detected signed overflow |

## 1. Types And Conversions

Build and run:

```bash
make types
./build/types
```

The example prints platform properties and demonstrates:

- why `CHAR_BIT` and `sizeof` are queried rather than assumed;
- why `%zu` is required for `size_t`;
- why `<inttypes.h>` macros are useful for fixed-width types;
- how to check signed addition before evaluating an overflowing expression;
- how to validate a signed index before converting it to `size_t`.

The code uses `uint32_t` because this host provides it. Exact-width typedefs are
optional in C; a production portability layer must verify required types on
each supported target.

## 2. Scope, Linkage, And Storage Duration

```bash
make storage
./build/storage
```

Files:

```text
storage/status.h
storage/status.c
storage/main.c
```

`status.h` declares one externally linked object and public functions.
`status.c` provides the object definition, keeps a helper function private with
file-scope `static`, and uses a static local sequence counter.

The counter is learning-only shared state. It is not reentrant or thread-safe,
has no reset API, and eventually wraps according to unsigned arithmetic.
Production code should usually make such state explicit in a caller-owned
context object.

## 3. Control Flow And Bit Operations

```bash
make control
./build/control
```

The example includes:

- explicit `switch` handling with `default`;
- forward and reverse `size_t` loops;
- `break` and `continue`;
- checked bit-index validation;
- unsigned masks for bit manipulation.

Avoid this broken unsigned countdown:

```c
for (size_t index = count - 1U; index >= 0U; --index) {
    process(index);
}
```

`index >= 0U` is always true, and `count - 1U` wraps when `count` is zero.

## 4. Checked Text Input

```bash
make input
printf '42\n' | ./build/input
```

Try rejected inputs:

```bash
printf '12x\n' | ./build/input
printf '999999999999999999999999\n' | ./build/input
printf '\n' | ./build/input
```

The program uses `fgets` and `strtol`, then checks:

- input failure;
- an input line that exceeds the buffer;
- whether any digits were consumed;
- trailing characters;
- `errno`;
- the `int` range before narrowing.

It does not use `gets` or unbounded `%s`. For production protocols, define
whitespace, line length, encoding, retry, and error-reporting policies
explicitly.

## 5. Debug Build

```bash
make debug
gdb ./build/debug-types
```

Useful commands:

```text
break main
run
next
step
print value
ptype value
backtrace
```

## 6. Sanitizer Build

```bash
make sanitize
./build/sanitize-types
```

The successful sanitizer program is expected to run without findings.

The intentional undefined-behavior example is checked separately:

```bash
make expect-ubsan-overflow
```

That target passes only when UBSan rejects signed overflow. Never execute
undefined behavior merely to determine whether it is safe. The example exists
only to demonstrate a diagnostic workflow on a supported hosted toolchain.

Sanitizers do not replace:

- compiler warnings;
- static analysis;
- boundary tests;
- review of implementation-defined assumptions;
- testing on the actual embedded target.

## 7. Intentional Compiler Diagnostics

```bash
make expect-sign-warning
make expect-format-warning
```

The targets compile isolated bad examples with selected warnings promoted to
errors. The Makefile uses GCC/Clang warning names and matches English diagnostic
text, so this portion is intentionally toolchain-specific. They demonstrate:

- comparing `int` directly with `size_t`;
- printing `size_t` with `%d`.

The production fixes are domain validation and the correct format specifier,
not unexplained casts.

## 8. Safety Notes

- **Undefined behavior:** `unsafe/signed_overflow.c` intentionally executes
  signed overflow only under the expected-failure target.
- **Ownership/lifetime:** no dynamic allocation is used. Caller-provided output
  pointers are validated before writing.
- **Unsafe C APIs:** `gets` and unbounded string input are not used.
- **Concurrency:** the static sequence example is not thread-safe or reentrant.
  `volatile` would not fix that.
- **Exception safety and iterator invalidation:** not applicable to these C
  examples.
- **Production readiness:** real code still needs project-specific error types,
  logging, test coverage, static analysis, platform validation, and concurrency
  policy.

## 9. Suggested Exercises

1. Extend `checked_add_int` with checked subtraction and multiplication.
2. Change `input/parse_int.c` to accept a caller-provided minimum and maximum.
3. Add clear/toggle/test functions beside `set_bit`.
4. Replace the static sequence counter with a caller-owned context.
5. Add tests for zero, one, minimum, maximum, and one-past-boundary values.
6. Compile with both GCC and Clang and compare diagnostics.
