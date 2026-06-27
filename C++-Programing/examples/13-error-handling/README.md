# 13 - Error Handling Examples

## Status

These are learning-focused examples for `13-error-handling`. They are small,
compile-oriented demonstrations, not production-ready parsers, logging systems,
file libraries, embedded HALs, or safety-critical components.

The suite demonstrates:

- C status/error enum handling and careful `errno` use;
- C++ exception propagation with RAII cleanup;
- basic strong exception-safety structure using validate-then-commit;
- why `noexcept` matters for move operations;
- file/stream state debugging with `good()`, `eof()`, `fail()`, and `bad()`;
- a tiny C++17 Result-style API as a pre-C++23 stand-in for `std::expected`.

## Requirements

- GNU Make
- A C17 compiler such as `cc`, `gcc`, or `clang`
- A C++17 compiler such as `g++` or `clang++`
- Optional: AddressSanitizer/UndefinedBehaviorSanitizer support
- Optional: `gdb` or `lldb` for debugging

## Build And Run

From this directory:

```sh
make all
make run
make sanitize
make check
make debug
make clean
```

Generated binaries are placed under `build/`.

Use another compiler when useful:

```sh
make clean
make CXX=clang++ CC=clang check
```

## Example Map

| Target | Source | Focus | Learning-only notes |
| --- | --- | --- | --- |
| `c_status_errno` | `c_status_errno.c` | C status enum, output parameter, `errno` after failed `fopen` | Uses one tiny parser and a deliberately missing file |
| `raii_exception_safety` | `raii_exception_safety.cpp` | stack unwinding, RAII cleanup, strong guarantee shape | Uses logging resources, not real file/device handles |
| `noexcept_move` | `noexcept_move.cpp` | `noexcept` move and `std::vector` reallocation choice | Shows observable copy/move counters, not a benchmark |
| `stream_states` | `stream_states.cpp` | EOF vs format failure vs open failure | Uses `std::istringstream` plus a missing file |
| `result_style` | `result_style.cpp` | explicit value-or-error API before C++23 `std::expected` | Minimal Result type, not a complete production abstraction |

## 1. C Status And `errno`

Run:

```sh
make build/c_status_errno
./build/c_status_errno
```

Expected shape:

```text
parse-42=ok value=42
parse-999=overflow
open-status=failed errno=... text=...
result=passed
```

Production direction:

- Use domain-specific error enums instead of magic integers.
- Write output parameters only after validation succeeds.
- Read `errno` only after an API documents failure through `errno`.
- Save `errno` immediately before calling other library functions.

Relevant risks:

- C output parameters can be left unmodified or stale on failure unless the API
  contract is explicit.
- `errno` is not a universal last-error value.
- `strerror()` is convenient for learning; production logging may need
  thread-safe or structured alternatives depending on platform policy.

## 2. RAII And Exception Safety

Run:

```sh
make build/raii_exception_safety
./build/raii_exception_safety
```

Expected shape:

```text
acquire file
acquire buffer
release buffer
release file
caught=simulated parse failure
replace-failed=empty config line
alive=0 size-after-failure=2 result=passed
```

Production direction:

- Put every owning resource into an RAII object.
- Validate or build replacement state before mutating the original object.
- Keep destructors no-throw.
- Use explicit `close()`, `commit()`, or `flush()` when failure must be reported.

Relevant risks:

- Manual cleanup after a throwing call is skipped.
- A destructor that throws during stack unwinding can call `std::terminate`.
- This example prints from destructors for visibility; production destructors
  should avoid throwing logging paths.

## 3. `noexcept` Move Behavior

Run:

```sh
make build/noexcept_move
./build/noexcept_move
```

Expected shape:

```text
throwing-move copies=1 moves=0
nothrow-move copies=0 moves=1
result=passed
```

Exact counts are tied to this tiny example and library behavior, but the design
lesson is stable: containers can prefer no-throw moves during reallocation.

Production direction:

- Mark move constructors and move assignment `noexcept` only when they truly
  cannot throw.
- Use `static_assert` or traits when a template depends on no-throw movement.
- Remember that `noexcept` violations call `std::terminate`; they are not caught
  by nearby `catch` blocks.

Relevant risks:

- A function marked `noexcept` must not let allocation, logging, callbacks, or
  other throwing operations escape.
- No race/deadlock behavior is demonstrated here; the examples are
  single-threaded.

## 4. Stream State Debugging

Run:

```sh
make build/stream_states
./build/stream_states
```

Expected shape:

```text
valid-after-read good=0 eof=1 fail=1 bad=0
invalid-error=format error before EOF
invalid-after-error good=0 eof=0 fail=1 bad=0
missing-opened=0 fail=1
sum=60 format-error=1 result=passed
```

Production direction:

- Use extraction success as the loop condition.
- Treat EOF as normal when EOF is the expected loop end.
- Check `bad()` for serious I/O failure.
- Add path, line number, and operation context to real diagnostics.

Relevant risks:

- `while (!stream.eof())` can process stale data after a failed read.
- Calling `clear()` removes flags but does not fix the underlying cause.
- Enabling stream exceptions for EOF can make normal loops awkward unless EOF is
  truly exceptional.

## 5. Result-Style API

Run:

```sh
make build/result_style
./build/result_style
```

Expected shape:

```text
ok-value=77
bad-error=overflow
unchecked-access-rejected=1
result=passed
```

Production direction:

- Use `std::expected<T, E>` in C++23 when available and approved.
- Use a project `Result<T, E>` in C++17/C++20 when expected failures need
  explicit reasons.
- Use `std::optional<T>` only when absence is enough information.
- Keep exception and Result policies consistent at API boundaries.

Relevant risks:

- This `Result` is intentionally tiny. It assumes default-constructible `T` and
  `E`, throws `std::logic_error` on wrong accessor use, lacks monadic helpers,
  and is not a general library type.
- Do not hide diagnostics inside logs and return only `false`.

## Debugging

Build unoptimized debug binaries:

```sh
make debug
```

Example GDB sessions:

```sh
gdb ./build/raii_exception_safety
(gdb) break throwing_work
(gdb) catch throw
(gdb) run
(gdb) bt
```

```sh
gdb ./build/stream_states
(gdb) break sum_numbers
(gdb) run
(gdb) print input.rdstate()
```

For unexpected termination in your own experiments, break on `std::terminate`
and inspect whether an exception escaped a `noexcept` function or destructor.

## Sanitizer Experiments

Run the clean suite under AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
make sanitize
```

The checked examples should pass without sanitizer findings. To study failures,
introduce one defect at a time in a local experiment:

- move the output assignment in `parse_u8()` before validation succeeds;
- replace RAII cleanup with raw `new`/`delete` and throw before `delete`;
- mark a throwing function `noexcept` and observe termination;
- change a stream loop to `while (!input.eof())` and inspect stale data;
- call `Result::value()` without checking `has_value()` and observe the
  explicit `std::logic_error`.

ASan/UBSan only catch defects on executed paths. A clean sanitizer run is not a
proof of complete exception safety, complete error-policy coverage, race freedom,
deadlock freedom, logical API correctness, or production readiness.
