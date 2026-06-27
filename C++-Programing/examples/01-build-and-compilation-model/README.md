# 01 - Build And Compilation Model: Examples

## Status

These are **learning-focused examples**, not production-ready build
infrastructure. They intentionally favor visible compiler and linker steps over
abstraction.

The code itself follows safe basic practices:

- no manual memory management;
- no intentionally executed undefined behavior;
- no shared mutable state, races, or deadlocks;
- no iterators or exception-sensitive resource ownership;
- no unsafe C string APIs.

The examples use GNU Make, GCC/G++, GNU `ar`, and Linux/ELF inspection tools.
Equivalent concepts apply elsewhere, but commands and binary formats differ.

## Requirements

```bash
gcc --version
g++ --version
make --version
ar --version
nm --version
readelf --version
```

Run all successful builds and assert the intentional failures:

```bash
make check
```

Remove generated files:

```bash
make clean
```

All generated artifacts are placed under `build/`.

## Example Map

| Target | Demonstrates |
| --- | --- |
| `stages` | Preprocessing, assembly generation, object generation, linking |
| `separate` | Separate translation units and ordinary linking |
| `static` | Static archive creation and correct library order |
| `shared` | Shared library, `soname`, runtime lookup |
| `mixed` | C implementation called from C++ through `extern "C"` |
| `inline` | One inline header definition used by multiple translation units |
| `inspect` | Symbols, sections, dynamic dependencies, and linker map |
| `debug` | Debug-symbol build |
| `sanitize` | AddressSanitizer and UndefinedBehaviorSanitizer build |
| `expected-failures` | Missing definition, wrong archive order, missing loader path |

## 1. Build Stages

Generate the visible stage outputs:

```bash
make stages
```

Artifacts:

```text
build/stages/main.ii  # Preprocessed translation unit
build/stages/main.s   # Assembly output
build/stages/main.o   # Relocatable object file
build/stages/app      # Linked executable
```

Run:

```bash
./build/stages/app
```

Expected output:

```text
sensor=42
```

The `.ii` file can be large because it contains expanded standard headers.

## 2. Separate Compilation

```bash
make separate
./build/separate/app
```

The relevant commands are conceptually:

```bash
g++ -c cpp/main.cpp -o build/separate/main.o
g++ -c cpp/sensor.cpp -o build/separate/sensor.o
g++ build/separate/main.o build/separate/sensor.o -o build/separate/app
```

Inspect what each object defines and requires:

```bash
nm -C build/separate/main.o
nm -C build/separate/sensor.o
```

Look for an undefined `read_sensor()` in `main.o` and its definition in
`sensor.o`.

## 3. Static Library And Link Order

```bash
make static
./build/static/app
```

The archive is created with:

```bash
ar rcs build/static/libsensor.a build/static/sensor.o
```

Correct order:

```bash
g++ build/static/main.o -Lbuild/static -lsensor -o build/static/app
```

Demonstrate that the reverse order fails on the GNU linker used by this example:

```bash
make expect-static-order-failure
```

The target passes only when the link fails with `undefined reference`. Library
ordering is toolchain behavior, so do not generalize the exact command-line
rule to every linker.

## 4. Shared Library And Runtime Loading

```bash
make shared
LD_LIBRARY_PATH=build/shared ./build/shared/app
```

Inspect the recorded dependency and `soname`:

```bash
readelf -d build/shared/app
readelf -d build/shared/libch01sensor.so.1.0
```

Demonstrate a load-time failure:

```bash
make expect-loader-failure
```

The application linked because `-Lbuild/shared` let the linker find
`libch01sensor.so`. That option does not configure runtime lookup. This example
uses `LD_LIBRARY_PATH` only for controlled learning; production deployment
should use a deliberate packaging and loader-path policy.

## 5. Mixed C And C++

```bash
make mixed
./build/mixed/app
```

The C source is compiled with `gcc`, while the C++ caller and final link use
`g++`:

```bash
gcc -c mixed/sensor_c.c -o build/mixed/sensor_c.o
g++ mixed/main.cpp build/mixed/sensor_c.o -o build/mixed/app
```

Inspect the C symbol and the caller's required symbol:

```bash
nm build/mixed/sensor_c.o
nm -C build/mixed/main.o
```

The shared header guards `extern "C"` with `__cplusplus`, so it remains valid
for both languages.

`extern "C"` specifies language linkage. It does not make C++ classes, STL
types, exceptions, ownership, or allocator boundaries automatically ABI-safe.

## 6. Header-Safe `inline`

```bash
make inline
./build/inline/app
```

Both `left.cpp` and `right.cpp` include the same inline definition from
`math_utils.hpp`. This is valid when all definitions satisfy ODR requirements.

The keyword does not force optimizer inlining. Inspect generated assembly if
you want to study optimization:

```bash
g++ -std=c++17 -O2 -S inline/left.cpp -o build/inline/left-O2.s
```

## 7. Debug And Inspection

Create a debug build and linker map:

```bash
make debug
gdb ./build/debug/app
```

Useful GDB commands:

```text
break main
run
next
step
info functions read_sensor
disassemble read_sensor
```

Inspect binary structure:

```bash
make inspect
```

The target runs:

```bash
nm -C build/separate/app
readelf -S build/separate/app
readelf -Ws build/separate/app
readelf -r build/separate/main.o
readelf -d build/shared/app
```

It also generates `build/separate/app.map`.

## 8. Sanitizers

```bash
make sanitize
./build/sanitize/app
```

This uses:

```text
-fsanitize=address,undefined
-fno-omit-frame-pointer
```

The current examples are not intended to trigger sanitizer findings. The target
shows how to keep sanitizer instrumentation available while changing or
extending examples.

Sanitizers do not diagnose ordinary unresolved symbols, duplicate definitions,
loader search failures, or all ABI/ODR violations. Use the tool that matches the
failure stage.

## 9. Intentional Failure Summary

Run all expected failures:

```bash
make expected-failures
```

The suite checks:

1. Linking `main.o` without a `read_sensor()` definition fails.
2. Placing the static archive before the requiring object fails with this
   linker.
3. Running the shared-library application without a runtime search path fails.

The Makefile captures diagnostics in:

```text
build/failures/missing-definition.log
build/failures/static-order.log
build/failures/loader.log
```

These targets intentionally test failures without leaving `make check` in a
failed state.

## 10. Production Notes

For production builds:

- use a real project build system and dependency tracking;
- make warning, language-standard, target, and ABI flags consistent;
- do not rely globally on `LD_LIBRARY_PATH`;
- control exported shared-library symbols;
- version incompatible shared-library ABIs with a new major `soname`;
- keep C ABI ownership and exception boundaries explicit;
- retain linker maps for size-sensitive embedded images;
- do not silence duplicate-definition or unresolved-symbol errors without
  understanding the ownership and linkage model.
