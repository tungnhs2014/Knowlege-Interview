# 01 - Build And Compilation Model: Interview Pack

## How To Use This Pack

For each question:

1. Give the **Short answer** first.
2. Expand only when the interviewer asks for mechanism, tradeoffs, or diagnosis.
3. Use the **Code/API anchor** to connect the explanation to real C/C++.
4. Finish with the **Production/debug angle** to show engineering judgment.

The Linux-specific commands use GCC/Clang-style tooling and ELF as a concrete
environment. The language concepts remain relevant across platforms, while
binary formats and tools differ on MSVC/COFF and other systems.

## Beginner Questions

### 1. What happens from a `.c` or `.cpp` file to a running program?

**Short answer**

The source is preprocessed into a translation unit, compiled into assembly or
machine code, assembled into an object file, linked with other objects and
libraries into an executable, and finally loaded with its runtime dependencies
before `main` runs.

**Deep explanation**

- **Preprocessing** handles `#include`, macros, and conditional compilation.
- **Compilation** parses the language, checks types and semantics, optimizes,
  and generates code.
- **Assembly** creates a relocatable object file.
- **Linking** resolves symbols and relocations across objects and libraries.
- **Loading** maps the executable and shared libraries into memory, performs
  runtime binding as required, runs initialization, and transfers control to
  the program entry path that eventually calls `main`.

The compiler driver may run several of these stages with one command, but they
remain distinct mechanisms with different failure modes.

**C/C++ code/API anchor**

```bash
g++ -E main.cpp -o main.ii  # Preprocess
g++ -S main.cpp -o main.s   # Compile to assembly
g++ -c main.cpp -o main.o   # Produce object file
g++ main.o -o app           # Link
./app                       # Load and run
```

**Production/debug angle**

First classify a failure by stage. Syntax errors point to compilation;
`undefined reference` points to linking; "cannot open shared object file"
points to runtime loading.

**Common traps**

- Saying the compiler directly creates a running process.
- Omitting the linker or loader.
- Treating `gcc`/`g++` as only the compiler rather than a driver.
- Assuming a successful link guarantees successful startup.

**Follow-up questions**

- What does an object file contain?
- Which stage processes `#include`?
- Can loading fail after a successful build?

### 2. What is a translation unit?

**Short answer**

A translation unit is one source file after preprocessing, including the
headers and macro expansions visible to that source file.

**Deep explanation**

Each `.c` or `.cpp` file is normally preprocessed and compiled independently.
If both `main.cpp` and `sensor.cpp` include `sensor.hpp`, the header content
participates separately in both translation units. This is why inconsistent
macros, header definitions, and include order can create cross-file problems.

**C/C++ code/API anchor**

```cpp
// main.cpp
#include "sensor.hpp"
```

```bash
g++ -E main.cpp -o main.ii
```

`main.ii` is a useful approximation of the preprocessed translation unit.

**Production/debug angle**

When one source file sees a different class definition, feature macro, or
calling declaration from another, the project may compile separately but fail
at link time or violate the ODR.

**Common traps**

- Calling a header a translation unit.
- Saying the whole project is one translation unit.
- Confusing a translation unit with an object file: the latter is compiled
  binary output.

**Follow-up questions**

- Why can the same header be processed many times?
- How can macros make two translation units disagree?
- How does LTO affect, but not erase, this model?

### 3. What is the difference between a declaration and a definition?

**Short answer**

A declaration introduces an entity and its type. A definition provides the
function body or object storage required by the program.

**Deep explanation**

A function declaration lets the compiler type-check a call before it sees the
function body. The linker later needs a matching definition. An object
declaration using `extern` does not allocate storage, while an object definition
does.

Compatible declarations may appear in multiple translation units. An ordinary
non-inline entity with external linkage normally needs one program-wide
definition.

**C/C++ code/API anchor**

```cpp
int read_sensor();          // Function declaration
extern int sample_count;    // Object declaration

int read_sensor() {         // Function definition
    return 42;
}

int sample_count = 0;       // Object definition
```

**Production/debug angle**

A declaration/definition mismatch may produce a link error whose symbol spelling
reveals a different parameter type, namespace, qualifier, or language linkage.

**Common traps**

- Saying every declaration is a definition.
- Assuming `extern int count;` allocates storage.
- Believing a declaration alone is enough for the final executable.

**Follow-up questions**

- Is `int value;` a declaration or definition?
- Why can a function call compile without the function body?
- What happens if the signature differs between declaration and definition?

### 4. Why does `undefined reference` occur?

**Short answer**

The compiler accepted a declaration, but the linker could not find a matching
definition in the object files and libraries supplied to the link.

**Deep explanation**

Typical causes include:

- the defining object file was omitted;
- the required library was omitted;
- a static library appears before the object that needs it;
- the declaration and definition produce different symbols;
- C++ expects a mangled symbol for a C function;
- C++ objects are final-linked without the required C++ runtime libraries.

**C/C++ code/API anchor**

```cpp
int read_sensor();

int main() {
    return read_sensor();
}
```

```bash
g++ -c main.cpp -o main.o  # Succeeds
g++ main.o -o app          # undefined reference to read_sensor()
```

Inspect the required symbol:

```bash
nm -C main.o
```

**Production/debug angle**

Do not start by adding random libraries. Identify the exact unresolved symbol,
then locate the object or library that should define it with `nm -C` or
`readelf -Ws`.

**Common traps**

- Calling it a syntax error.
- Assuming another `#include` always fixes it.
- Ignoring namespace, qualifiers, and language linkage.

**Follow-up questions**

- How can static-library order cause this error?
- How would you inspect whether a library contains the symbol?
- Why can missing `extern "C"` create this symptom?

### 5. Why does `multiple definition` occur?

**Short answer**

More than one linked object provides an external definition that must be unique.

**Deep explanation**

A common cause is defining an ordinary non-inline function or global object in
a header included by several source files. Each translation unit produces its
own external definition, and the linker detects duplicates. Other causes include
linking the same object twice or defining a global object in several source
files instead of declaring it with `extern`.

**C/C++ code/API anchor**

```cpp
// bad.hpp
int read_sensor() {  // Unsafe ordinary external definition in a header
    return 42;
}
```

Fix by leaving only a declaration in the header:

```cpp
// sensor.hpp
int read_sensor();
```

```cpp
// sensor.cpp
int read_sensor() {
    return 42;
}
```

**Production/debug angle**

Use the duplicate symbol name to find every defining object:

```bash
nm -C file1.o file2.o
```

Also inspect the build graph for the same source or object being added twice.

**Common traps**

- Believing include guards prevent cross-translation-unit duplicates.
- Making every duplicate function `static` without considering intended API
  visibility.
- Using `inline` only to silence a linker error without checking ODR semantics.

**Follow-up questions**

- When is a function definition valid in a header?
- What does internal linkage change?
- Why might an ODR violation produce no linker error?

## Mid-Level Questions

### 6. What is stored in an object file, and how would you inspect it?

**Short answer**

An object file contains relocatable machine code, data sections, symbols, and
relocation records. On ELF systems, use `nm`, `readelf`, and `objdump`.

**Deep explanation**

The object file records code and data whose final addresses are not yet known.
Defined symbols identify entities this object supplies. Undefined symbols
identify dependencies. Relocation records tell the linker which encoded
addresses or references must be adjusted when sections receive final locations.

**C/C++ code/API anchor**

```bash
nm -C sensor.o          # Defined and undefined symbols
readelf -S sensor.o     # Sections
readelf -Ws sensor.o    # Symbol table
readelf -r sensor.o     # Relocations
objdump -dr sensor.o    # Disassembly with relocations
```

**Production/debug angle**

For `undefined reference`, compare the caller's `U` symbol with the provider's
defined symbol. For size issues, inspect sections and use a linker map to find
large contributors.

**Common traps**

- Treating an object file as a directly runnable executable.
- Looking only at demangled names and missing exact ABI spelling.
- Assuming every source-level entity produces a visible external symbol after
  optimization.

**Follow-up questions**

- What is a relocation?
- What do `T` and `U` commonly mean in `nm` output?
- Why might a function disappear from the final symbol table?

### 7. Why can static-library order affect linking?

**Short answer**

Traditional Unix-style linkers commonly search archives from left to right and
extract members only when they satisfy currently unresolved symbols.

**Deep explanation**

When `main.o` appears first, it creates an unresolved reference to
`read_sensor`. When `-lsensor` appears afterward, the linker extracts the
archive member that defines it. If the archive appears first, no unresolved
reference may exist yet, so no member is selected; the linker may not
automatically rescan it later.

**C/C++ code/API anchor**

```bash
# Usually correct
g++ main.o -L. -lsensor -o app

# May fail
g++ -L. -lsensor main.o -o app
```

For circular archive dependencies, GNU linkers can use a group:

```bash
g++ main.o -Wl,--start-group -la -lb -Wl,--end-group -o app
```

**Production/debug angle**

Inspect the actual link command generated by the build system. Avoid solving
poor dependency design with repeated libraries unless the reason is understood.

**Common traps**

- Claiming order matters equally for every linker and every input type.
- Confusing `-L` search paths with `-l` library selection.
- Repeating a library blindly instead of recognizing circular dependencies.

**Follow-up questions**

- How does a static archive differ from an object file?
- What problem does `--start-group` solve?
- Would shared-library handling be identical?

### 8. Why can a shared library link successfully but fail at program startup?

**Short answer**

Link-time library discovery and runtime loader discovery are separate. The
linker may find the library through `-L`, while the loader cannot find a
compatible shared object at runtime.

**Deep explanation**

The executable records dependencies such as `DT_NEEDED`. At startup, the loader
searches according to platform rules that can involve `RUNPATH`, older `RPATH`,
environment variables, loader caches, and default directories. Deployment
errors, broken `soname` links, architecture mismatches, or missing transitive
dependencies can all prevent loading.

**C/C++ code/API anchor**

```bash
g++ main.cpp -L./lib -lsensor -o app
readelf -d app
LD_DEBUG=libs ./app
```

Temporary controlled test:

```bash
LD_LIBRARY_PATH=./lib ./app
```

**Production/debug angle**

Check `DT_NEEDED`, `RUNPATH`, deployed symlinks, file architecture, and
transitive dependencies. Treat `LD_LIBRARY_PATH` as a diagnostic tool rather
than an automatic production fix.

**Common traps**

- Saying `-L` configures runtime search.
- Copying a library without preserving its `soname` symlink structure.
- Using `ldd` on an untrusted binary.

**Follow-up questions**

- What is a `soname`?
- What is the difference between `RPATH` and `RUNPATH`?
- How would you debug a transitive dependency failure?

### 9. What is the real meaning of `inline` in modern C++?

**Short answer**

`inline` primarily has language and ODR meaning: it permits qualifying
definitions in multiple translation units. It does not force optimizer
inlining.

**Deep explanation**

An inline function can be defined in a header included by several translation
units, provided all definitions satisfy ODR requirements. The optimizer
independently decides whether to replace a call with the function body. It may
inline a function without the keyword or preserve a call to one declared
`inline`.

Templates are commonly defined in headers so their definitions are available
for instantiation; being a template does not automatically add the `inline`
specifier.

**C/C++ code/API anchor**

```cpp
// math.hpp
#pragma once

inline int clamp_to_zero(int value) {
    return value < 0 ? 0 : value;
}
```

Inspect optimization separately:

```bash
g++ -O2 -S main.cpp -o main.s
```

**Production/debug angle**

Header definitions that depend on inconsistent feature macros can violate the
ODR without a required diagnostic. Keep ABI- and definition-affecting build
configuration consistent.

**Common traps**

- Saying `inline` is only a performance request.
- Saying inline functions are "exempt from ODR."
- Claiming loops, recursion, virtual calls, or static locals universally forbid
  optimizer inlining.

**Follow-up questions**

- What conditions apply to multiple inline definitions?
- What are C++17 inline variables?
- How can macros create an ODR violation?

### 10. What does `extern "C"` guarantee, and what does it not guarantee?

**Short answer**

It gives declarations C language linkage when compiled as C++. It commonly
provides C-compatible symbol naming, but it is not a complete ABI adapter.

**Deep explanation**

Mixed C/C++ headers use `__cplusplus` so a C compiler sees ordinary C
declarations while a C++ compiler sees a C linkage specification. C linkage
does not permit C++ overloads with the same name.

It does not guarantee:

- compatible structure layout across arbitrary toolchains;
- compatible calling conventions under conflicting options;
- safe ownership or allocator boundaries;
- safe propagation of C++ exceptions through C frames;
- compatibility of STL or C++ class layouts.

**C/C++ code/API anchor**

```c
#ifdef __cplusplus
extern "C" {
#endif

int sensor_read(void);

#ifdef __cplusplus
}
#endif
```

```bash
gcc -c sensor.c -o sensor.o
g++ main.cpp sensor.o -o app
nm sensor.o
```

**Production/debug angle**

Use a narrow boundary with fixed-width or documented C types, explicit
ownership, return-code error handling, and no exceptions crossing the boundary.

**Common traps**

- Saying it converts C++ code into C.
- Saying it guarantees complete binary compatibility.
- Exposing `std::string`, exceptions, or C++ class layout through a stability-
  focused C API.

**Follow-up questions**

- Why use `#ifdef __cplusplus`?
- Can C-linkage functions be overloaded?
- How would you expose a C++ object through an opaque C handle?

## Senior Questions

### 11. What is ABI compatibility, and how can a source-compatible change break it?

**Short answer**

ABI compatibility means separately compiled components still agree on their
binary contract. A source-compatible change can alter layout, symbols, calling
conventions, or runtime assumptions and break existing binaries.

**Deep explanation**

An ABI may cover:

- symbol naming and visibility;
- calling convention and parameter passing;
- object size, alignment, padding, and member offsets;
- virtual table representation;
- exception and RTTI conventions;
- standard-library and runtime compatibility.

Adding a member to a public class, changing a virtual function layout, changing
compiler ABI options, or exposing a different standard-library type can break
clients without causing their source code to become invalid.

**C/C++ code/API anchor**

```cpp
// Public ABI: layout is exposed to clients.
class Sensor {
public:
    virtual int read() const;

private:
    int fd_;
};
```

Safer boundary:

```c
typedef struct sensor_handle sensor_handle;

sensor_handle* sensor_create(void);
int sensor_read(sensor_handle*, int* output);
void sensor_destroy(sensor_handle*);
```

**Production/debug angle**

Version the shared-library ABI deliberately, minimize exported symbols, and run
binary compatibility checks when evolving public libraries. A new incompatible
ABI generally requires a new major `soname`.

**Common traps**

- Equating API compatibility with ABI compatibility.
- Assuming the same compiler name guarantees compatibility.
- Exposing compiler- and runtime-sensitive C++ implementation types.

**Follow-up questions**

- Which class changes break object layout?
- How do visibility and `soname` support ABI management?
- What problem does PImpl solve, and what does it not solve?

### 12. Design a stable C API over a C++ implementation.

**Short answer**

Expose opaque handles and C-compatible functions, keep ownership explicit,
translate exceptions into error codes, and hide all C++ types and layout.

**Deep explanation**

The public header must compile as both C and C++. The C++ implementation owns
the real object behind an incomplete C type. Creation returns a handle;
operations validate inputs and return documented status codes; destruction
releases the same allocation domain. No exception may escape the C boundary.

**C/C++ code/API anchor**

```c
// sensor_api.h
#ifndef SENSOR_API_H
#define SENSOR_API_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sensor_handle sensor_handle;

sensor_handle* sensor_create(void);
int sensor_read(sensor_handle* handle, int* output);
void sensor_destroy(sensor_handle* handle);

#ifdef __cplusplus
}
#endif

#endif
```

```cpp
// sensor_api.cpp
struct sensor_handle {
    Sensor implementation;
};

extern "C" int sensor_read(sensor_handle* handle, int* output) {
    if (handle == nullptr || output == nullptr) {
        return -1;
    }

    try {
        *output = handle->implementation.read();
        return 0;
    } catch (...) {
        return -2;
    }
}
```

**Production/debug angle**

Document nullability, thread safety, ownership, versioning, allocator rules,
and error meanings. Keep create/destroy in the same module to avoid allocator
and runtime mismatches.

**Common traps**

- Returning a pointer to a C++ class as if its layout were stable.
- Letting exceptions cross the C boundary.
- Allocating in one runtime and freeing in another without a defined contract.
- Omitting API version negotiation for long-lived plugin boundaries.

**Follow-up questions**

- How would you report a detailed error message?
- How would you make the API thread-safe?
- How would you evolve the handle without breaking clients?

### 13. How would you diagnose an optimization-dependent ODR failure?

**Short answer**

Compare preprocessed definitions and build flags across translation units,
inspect symbols and link maps, then reduce the issue to a minimal multi-file
case.

**Deep explanation**

ODR failures can occur when inline functions, templates, class definitions, or
inline variables differ across translation units. Conditional macros, generated
headers, packing settings, or inconsistent compiler options can create these
differences. Because a diagnostic is not required in every case, optimization
or link order may change which behavior appears.

**C/C++ code/API anchor**

```cpp
inline int buffer_size() {
#ifdef LARGE_BUFFER
    return 1024;
#else
    return 128;
#endif
}
```

Compare preprocessing:

```bash
g++ -E -dD file1.cpp -o file1.ii
g++ -E -dD file2.cpp -o file2.ii
```

Inspect commands and symbols:

```bash
g++ -### ...
nm -C file1.o file2.o
```

**Production/debug angle**

Centralize feature configuration, generate one authoritative config header, and
make CI compile important variants cleanly. Avoid per-source ABI-affecting
definitions unless they are deliberate.

**Common traps**

- Expecting the linker to catch every ODR violation.
- Fixing symptoms by disabling optimization.
- Comparing only source files while ignoring preprocessor output and command
  lines.

**Follow-up questions**

- Why can LTO expose or change the manifestation of the problem?
- Which options affect structure layout?
- How would you prevent configuration drift in a build system?

### 14. Compare symbol visibility, `soname`, and symbol versioning.

**Short answer**

Visibility controls what a shared object exports, `soname` identifies an ABI
generation, and symbol versioning can distinguish versions of exported symbols
within library compatibility policies.

**Deep explanation**

- **Visibility** reduces the public symbol surface, relocation work, accidental
  interposition, and ABI commitments.
- **`soname`** is recorded as the runtime dependency identity, commonly using a
  major ABI version such as `libsensor.so.2`.
- **Symbol versioning** allows a library to attach version identities to
  exported symbols and, on supported platforms, preserve old implementations
  while adding new ones.

They solve related but different problems and do not replace semantic API
design or compatibility testing.

**C/C++ code/API anchor**

```bash
g++ -fPIC -fvisibility=hidden -c sensor.cpp -o sensor.o
g++ -shared sensor.o -Wl,-soname,libsensor.so.2 -o libsensor.so.2.0
readelf -Ws libsensor.so.2.0
readelf -d libsensor.so.2.0
```

**Production/debug angle**

Export only documented API symbols, bump the major `soname` for incompatible
ABI changes, and use symbol versioning only with an explicit compatibility
policy and toolchain support.

**Common traps**

- Treating semantic versioning and `soname` as identical.
- Exporting every C++ symbol by default.
- Assuming symbol versioning can repair incompatible object layout.

**Follow-up questions**

- How would you mark selected symbols for export?
- What is symbol interposition?
- How do Windows DLL exports differ at a high level?

### 15. How do LTO and PGO change optimization without removing build-model obligations?

**Short answer**

LTO lets optimization operate across translation-unit boundaries, while PGO
uses runtime profile data. Neither removes ODR, ABI, linkage, or deployment
requirements.

**Deep explanation**

With Link-Time Optimization, object files may contain intermediate
representation and the linker plugin enables whole-program analysis. This can
improve inlining, dead-code elimination, and devirtualization. Profile-Guided
Optimization uses execution profiles to guide code layout and optimization
decisions.

Both features require consistent compiler/toolchain support. Public ABI
boundaries still exist, shared libraries still need compatible runtime
deployment, and ODR violations remain invalid even if optimization changes how
they manifest.

**C/C++ code/API anchor**

```bash
g++ -O2 -flto -c main.cpp sensor.cpp
g++ -O2 -flto main.o sensor.o -o app
```

Typical PGO flow:

```bash
g++ -O2 -fprofile-generate app.cpp -o app-gen
./app-gen
g++ -O2 -fprofile-use app.cpp -o app
```

**Production/debug angle**

Keep compile and link flags compatible, validate profile representativeness,
measure binary size and performance, and retain non-LTO debug builds when they
improve diagnosis. Do not treat optimization as correctness repair.

**Common traps**

- Saying LTO merges the program into one C++ translation unit.
- Assuming PGO always improves performance.
- Mixing incompatible LTO objects and toolchains.
- Ignoring reproducibility and stale profile data.

**Follow-up questions**

- What kinds of optimization become easier with LTO?
- How can stale PGO data hurt?
- Why can LTO expose previously hidden ODR problems?

## Coding Tasks

### Task 1. Build A Three-File Program Manually

Create:

- `sensor.hpp` with a declaration;
- `sensor.cpp` with the definition;
- `main.cpp` with a call.

Build `.ii`, `.s`, `.o`, and the final executable using separate commands.

**Expected solution outline**

```bash
g++ -E main.cpp -o main.ii
g++ -S main.cpp -o main.s
g++ -c main.cpp -o main.o
g++ -c sensor.cpp -o sensor.o
g++ main.o sensor.o -o app
```

**What the interviewer evaluates**

- Correct distinction between declaration and definition.
- Understanding that each source forms a translation unit.
- Ability to explain which stage each command stops after.
- Use of warnings such as `-Wall -Wextra -Wpedantic`.

**Extensions**

- Inspect undefined and defined symbols with `nm -C`.
- Generate a linker map.
- Explain what changes when compiling as C.

### Task 2. Repair A Header-Induced Multiple Definition

Given:

```cpp
// math.hpp
int add(int left, int right) {
    return left + right;
}
```

The header is included by `a.cpp` and `b.cpp`.

**Expected solutions**

Preferred ordinary interface:

```cpp
// math.hpp
int add(int left, int right);
```

```cpp
// math.cpp
int add(int left, int right) {
    return left + right;
}
```

Valid header-only alternative:

```cpp
inline int add(int left, int right) {
    return left + right;
}
```

**What the interviewer evaluates**

- Recognition of the external definition in each translation unit.
- Understanding that include guards do not solve this problem.
- Ability to choose between source-file ownership and a deliberate inline
  header definition.

**Extensions**

- Ask why `static` also removes the linker error but changes semantics.
- Ask what happens if inline definitions differ across translation units.

### Task 3. Expose A C-Compatible Sensor API From C++

Design a public header and C++ implementation with:

- an opaque handle;
- create/read/destroy functions;
- explicit error reporting;
- no exception crossing the boundary.

**Expected interface**

```c
#ifndef SENSOR_API_H
#define SENSOR_API_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sensor_handle sensor_handle;

sensor_handle* sensor_create(void);
int sensor_read(sensor_handle* handle, int* output);
void sensor_destroy(sensor_handle* handle);

#ifdef __cplusplus
}
#endif

#endif
```

**What the interviewer evaluates**

- Correct `extern "C"` guarding.
- Explicit ownership and null handling.
- Exception translation.
- Avoidance of STL and class layout in the public ABI.

**Extensions**

- Add API versioning.
- Define thread-safety guarantees.
- Return detailed error text without transferring ambiguous ownership.

## Debugging Scenarios

### Scenario 1. The Function Exists, But The Linker Cannot Find It

C implementation:

```c
int sensor_read(void) {
    return 42;
}
```

C++ declaration:

```cpp
int sensor_read();
```

Build:

```bash
gcc -c sensor.c -o sensor.o
g++ main.cpp sensor.o -o app
```

The link fails.

**Expected diagnosis**

The C++ declaration has C++ language linkage, so the caller expects a mangled
C++ symbol. The C object defines a C symbol.

**Expected fix**

Use a shared header:

```c
#ifdef __cplusplus
extern "C" {
#endif

int sensor_read(void);

#ifdef __cplusplus
}
#endif
```

**Useful commands**

```bash
nm sensor.o
nm -C main.o
```

**Trap**

Adding another library path does not solve a symbol spelling mismatch.

### Scenario 2. The Shared Library Was Found During Build, But Not At Runtime

Build:

```bash
g++ main.cpp -L./build/lib -lsensor -o app
```

Runtime:

```text
error while loading shared libraries: libsensor.so.1:
cannot open shared object file
```

**Expected diagnosis**

`-L` affected the linker's search only. The runtime loader cannot locate the
recorded dependency or a transitive dependency.

**Useful commands**

```bash
readelf -d app
LD_DEBUG=libs ./app
```

**Expected production fixes**

- install/package the library in an intended loader path;
- configure an appropriate relative `RUNPATH` when justified;
- preserve correct `soname` symlinks;
- verify transitive dependencies and architecture.

**Trap**

Using global `LD_LIBRARY_PATH` as a permanent deployment policy can select
unexpected libraries.

### Scenario 3. Release Builds Behave Differently Across Source Files

Header:

```cpp
inline int buffer_size() {
#ifdef LARGE_BUFFER
    return 1024;
#else
    return 128;
#endif
}
```

Only some source files receive `-DLARGE_BUFFER`.

**Expected diagnosis**

Translation units contain different inline definitions, violating ODR
requirements. The linker may not diagnose it, and optimization can change the
observed behavior.

**Useful commands**

```bash
g++ -E -dD file1.cpp -o file1.ii
g++ -E -dD file2.cpp -o file2.ii
g++ -### ...
```

**Expected fix**

Use one authoritative build configuration and ensure definition-affecting
macros are consistent across all relevant translation units.

**Trap**

Disabling optimization may hide the symptom without fixing the invalid program.

## Rapid-Fire Checks

- Is a header file automatically a translation unit? **No.**
- Can a source file compile while its called function has no definition?
  **Yes, if a compatible declaration is visible; linking will still require the
  definition.**
- Does an include guard prevent duplicate external definitions across
  translation units? **No.**
- Does `inline` force call-site substitution? **No.**
- Are template functions automatically declared `inline`? **No.**
- Does `extern "C"` make C++ code valid C? **No.**
- Does `-L` configure runtime shared-library search? **No.**
- Can an executable link successfully and still fail before `main`? **Yes.**
- Can an ODR violation exist without a diagnostic? **Yes.**
- Should C++ exceptions cross a C ABI boundary? **No.**

## Final Review Checklist

Before an interview, make sure you can explain without memorized slogans:

- the complete source-to-process pipeline;
- translation units and separate compilation;
- declarations, definitions, linkage, symbols, and relocations;
- `undefined reference` and `multiple definition`;
- static archive extraction and order;
- shared-library runtime search and `soname`;
- the ODR role of `inline`;
- C linkage, name mangling, and `extern "C"` limitations;
- API compatibility versus ABI compatibility;
- a tool-driven debugging workflow using `nm`, `readelf`, `objdump`, and linker
  maps.
