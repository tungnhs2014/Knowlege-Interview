# 01 - Build And Compilation Model

## 1. Goals

After this lesson, you should be able to:

- explain how a `.c` or `.cpp` file becomes a running process;
- distinguish a source file, header file, translation unit, object file, and
  executable;
- distinguish a declaration from a definition;
- explain how the linker resolves symbols across object files and libraries;
- diagnose compile-time, link-time, and load-time failures;
- build and inspect static and shared libraries;
- explain C linkage, C++ name mangling, `extern "C"`, and ABI boundaries;
- avoid common One Definition Rule (ODR) mistakes in headers;
- use `gcc`, `g++`, `nm`, `readelf`, `objdump`, and linker maps during
  debugging.

This is a `MUST` topic with no prerequisites. It is the foundation for every
later topic involving multiple source files, libraries, build systems,
cross-compilation, or mixed C/C++ integration.

## 2. Why It Matters

Many failures are casually called "compiler errors," but they belong to
different stages:

- the compiler may reject invalid syntax or types;
- the linker may fail to find a required definition;
- the dynamic loader may fail to locate a shared library;
- a program may build successfully but still contain an ODR or ABI violation.

Without a build model, developers often try random fixes: adding unrelated
headers, moving definitions into headers, changing library order without
understanding why, or copying shared objects into arbitrary directories. Such
changes may make one configuration pass while breaking another.

In production code, this knowledge helps you:

- organize headers and source files correctly;
- classify diagnostics before investigating them;
- integrate third-party C and C++ libraries;
- control public APIs and binary compatibility;
- reduce exported symbols and unnecessary dependencies;
- inspect code size and memory placement in embedded builds;
- reason about differences between build-time and runtime environments.

## 3. Mental Model: Compile Separately, Link Together

Consider a project with three source files:

```text
main.cpp
sensor.cpp
logger.cpp
```

The compiler does not normally process the entire project as one document.
Each source file is preprocessed and compiled independently:

```text
main.cpp   -> translation unit -> main.o
sensor.cpp -> translation unit -> sensor.o
logger.cpp -> translation unit -> logger.o
```

The linker then combines their object files:

```text
main.o + sensor.o + logger.o + libraries -> executable
```

The central idea is:

> The compiler validates one translation unit at a time. The linker connects
> translation units through symbols.

A source file may compile successfully while calling a function whose body is
not present in that translation unit. The compiler only needs a compatible
declaration. The linker must later find the matching definition.

## 4. From Source Code To A Running Process

The conceptual pipeline is:

```text
source
  -> preprocessing
  -> compilation
  -> assembly
  -> linking
  -> loading
  -> running process
```

Compiler drivers such as `gcc` and `g++` usually orchestrate several internal
tools, so one command can perform multiple stages.

### 4.1 Preprocessing

The preprocessor handles directives such as:

- `#include`;
- `#define`;
- `#if`, `#ifdef`, and `#ifndef`;
- implementation-specific `#pragma` directives.

```cpp
#include "config.hpp"

#ifdef ENABLE_LOGGING
void log_startup();
#endif
```

With GCC or Clang, stop after preprocessing:

```bash
g++ -std=c++17 -E main.cpp -o main.ii
```

For a practical mental model, `#include` inserts the header's preprocessing
tokens at the include location. The resulting file can be very large because
it also contains content from standard and third-party headers.

Typical preprocessing problems include:

- an incorrect include path;
- a missing header;
- a macro changing code unexpectedly;
- different feature macros in different translation units;
- an include-guard collision hiding a header's contents.

### 4.2 Compilation

Compilation performs parsing, semantic analysis, type checking, optimization,
and code generation. Stop after generating assembly:

```bash
g++ -std=c++17 -S main.cpp -o main.s
```

Typical compilation failures include:

- invalid syntax;
- an undeclared name;
- an incompatible argument type;
- ambiguous overload resolution;
- invalid access to a class member.

### 4.3 Assembly

The assembler converts assembly into a relocatable object file:

```bash
g++ -std=c++17 -c main.cpp -o main.o
```

An object file is not yet a complete executable. On an ELF system, it commonly
contains:

- machine code in sections such as `.text`;
- initialized and uninitialized data;
- read-only data;
- a symbol table;
- relocation records;
- symbols defined by this object;
- unresolved symbols expected from other objects or libraries.

### 4.4 Linking

The linker:

- combines object files;
- extracts required members from static archives;
- resolves symbol references;
- applies relocations;
- emits an executable or shared object.

```bash
g++ main.o sensor.o logger.o -o app
```

Typical link failures include:

- `undefined reference`;
- `multiple definition`;
- a missing library;
- an incompatible object format or target architecture;
- incorrect static-library ordering.

### 4.5 Loading

When you run:

```bash
./app
```

the operating system and runtime loader must:

- map the executable into memory;
- locate and map required shared libraries;
- perform runtime relocations and symbol binding as needed;
- run startup and initialization code;
- eventually transfer control to `main`.

A successful link therefore does not guarantee successful startup:

```text
error while loading shared libraries: libsensor.so.1:
cannot open shared object file: No such file or directory
```

The exact binary format and loader differ by platform. This lesson uses
GCC/Clang-style commands and Linux/ELF as a concrete environment.

## 5. Source Files, Headers, And Translation Units

| Concept | Meaning |
| --- | --- |
| Source file | A `.c` or `.cpp` file passed to the compiler |
| Header file | A file included to share declarations and suitable definitions |
| Translation unit | One source file after preprocessing |
| Object file | Relocatable machine code and metadata produced from a translation unit |

A header is not normally linked as an independent object file. Its contents
become part of every translation unit that includes it.

### Separate-compilation example

```cpp
// sensor.hpp
#ifndef SENSOR_HPP
#define SENSOR_HPP

int read_sensor();

#endif
```

```cpp
// sensor.cpp
#include "sensor.hpp"

int read_sensor() {
    return 42;
}
```

```cpp
// main.cpp
#include "sensor.hpp"

#include <iostream>

int main() {
    std::cout << read_sensor() << '\n';
}
```

Compile each translation unit and then link:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -c sensor.cpp -o sensor.o
g++ -std=c++17 -Wall -Wextra -Wpedantic -c main.cpp -o main.o
g++ main.o sensor.o -o app
./app
```

## 6. Declarations And Definitions

A declaration introduces an entity and its type:

```cpp
int read_sensor();
extern int sample_count;
```

A definition supplies a function body or object storage:

```cpp
int read_sensor() {
    return 42;
}

int sample_count = 0;
```

Compatible declarations may appear in multiple translation units. An ordinary
non-inline function or object with external linkage normally requires exactly
one definition in the program.

### Missing definition

```cpp
// main.cpp
int read_sensor();  // Declaration only

int main() {
    return read_sensor();
}
```

```bash
g++ -c main.cpp -o main.o  # Succeeds
g++ main.o -o app          # Fails: undefined reference
```

The compiler can validate the call from the declaration. The linker cannot find
a definition for the required symbol.

### Duplicate definition

```cpp
// bad.hpp
int read_sensor() {  // External definition in a header
    return 42;
}
```

If two source files include `bad.hpp`, both object files usually define the same
external symbol. The linker then reports `multiple definition`.

## 7. Linkage And Symbols

Linkage answers whether uses of a name in different scopes or translation units
refer to the same entity.

### External linkage

An ordinary namespace-scope function normally has external linkage:

```cpp
int read_sensor() {
    return 42;
}
```

Other translation units may refer to it through a compatible declaration.

### Internal linkage

At file or namespace scope, `static` gives a function or object internal
linkage:

```cpp
static int normalize(int value) {
    return value < 0 ? 0 : value;
}
```

Each translation unit may have its own independent `normalize` without causing
a duplicate external symbol.

In C++, an unnamed namespace is commonly used for implementation details:

```cpp
namespace {

int normalize(int value) {
    return value < 0 ? 0 : value;
}

}  // namespace
```

### Inspecting symbols

```bash
nm sensor.o
nm -C sensor.o
readelf -Ws sensor.o
```

Common `nm` markers include:

- `T`: a globally defined text/code symbol;
- `U`: an undefined symbol that must be resolved;
- lowercase markers such as `t`: commonly a local symbol.

Start with two questions:

1. Which symbols does this object define?
2. Which symbols does this object require?

## 8. Headers, Include Guards, `inline`, And ODR

### Include guards

Portable include guard:

```cpp
#ifndef SENSOR_HPP
#define SENSOR_HPP

int read_sensor();

#endif
```

Widely supported alternative:

```cpp
#pragma once

int read_sensor();
```

| Property | Include guard | `#pragma once` |
| --- | --- | --- |
| Standard status | Portable C/C++ mechanism | Widely supported extension |
| Main risk | Typo or guard-name collision | Toolchain/filesystem edge cases |
| Size | Three directives | One directive |

Both mechanisms primarily prevent repeated processing inside one translation
unit. They do not make an ordinary external definition safe to place in a
header included by several translation units.

### One Definition Rule

The One Definition Rule specifies which entities require one program-wide
definition and which may have multiple equivalent definitions.

A header-safe inline function:

```cpp
// math_utils.hpp
#pragma once

inline int clamp_to_zero(int value) {
    return value < 0 ? 0 : value;
}
```

The language allows this definition to appear in multiple translation units,
provided the ODR requirements are satisfied.

Important distinction:

> The `inline` keyword does not command the optimizer to replace a call with
> the function body.

Compilers may optimize a function call without the keyword, and may preserve a
call to a function declared `inline`. The keyword's essential modern role is in
language and ODR semantics.

Templates are commonly defined in headers because their definitions must be
available when specializations are instantiated. A function template is not
automatically declared `inline` merely because it is a template.

### Configuration-dependent ODR bug

```cpp
// config.hpp
#pragma once

inline int buffer_size() {
#ifdef LARGE_BUFFER
    return 1024;
#else
    return 128;
#endif
}
```

If `LARGE_BUFFER` differs across translation units, their definitions are not
equivalent. The program may violate the ODR without a required diagnostic.

## 9. Macro, `inline`, And `constexpr`

```cpp
#define SQUARE_MACRO(x) ((x) * (x))

inline int square(int x) {
    return x * x;
}

constexpr int square_constexpr(int x) {
    return x * x;
}
```

| Property | Macro | `inline` function | `constexpr` function |
| --- | --- | --- | --- |
| Mechanism | Token substitution | Typed function | Typed function eligible for constant evaluation |
| Type checking | No | Yes | Yes |
| Scope | Preprocessor namespace | Normal language scope | Normal language scope |
| Argument evaluation | May be repeated | Once per call | Once; may occur at compile time |
| Primary use | Conditional compilation and token operations | Header-safe function definition | Compile-time and runtime calculation |

Never pass an expression with side effects to a macro that may evaluate its
argument more than once:

```cpp
int value = 3;
// SQUARE_MACRO(value++);  // Dangerous: value++ appears twice after expansion
```

In C++, prefer a function, template, or `constexpr` function unless you
specifically need preprocessing behavior.

## 10. Static Libraries

A static library such as `.a` is an archive of object files:

```bash
g++ -c sensor.cpp -o sensor.o
ar rcs libsensor.a sensor.o

g++ -c main.cpp -o main.o
g++ main.o -L. -lsensor -o app
```

The linker typically extracts only archive members needed to satisfy unresolved
symbols.

### Why order can matter

Traditional Unix-style linkers commonly process inputs from left to right:

```bash
# The object creates an unresolved reference; the library resolves it.
g++ main.o -L. -lsensor -o app

# May fail because the library was searched before main.o created the need.
g++ -L. -lsensor main.o -o app
```

Modern toolchains offer grouping and other options for circular dependencies,
but correct dependency order is the simplest default.

| Static-library property | Consequence |
| --- | --- |
| Selected code is copied into link output | The `.a` file is normally not needed at runtime |
| Application contains its selected library code | Updating the library requires relinking |
| Several executables may contain the same code | Total deployed size can increase |
| Runtime dependency is reduced | Deployment is simpler than with shared libraries |

## 11. Shared Libraries And Runtime Loading

Linux/ELF example:

```bash
g++ -std=c++17 -fPIC -c sensor.cpp -o sensor.o
g++ -shared sensor.o -Wl,-soname,libsensor.so.1 -o libsensor.so.1.0

ln -s libsensor.so.1.0 libsensor.so.1
ln -s libsensor.so.1 libsensor.so

g++ main.cpp -L. -lsensor -o app
```

The application can link successfully while the runtime loader still cannot
find the library:

```bash
LD_LIBRARY_PATH=. ./app
```

`LD_LIBRARY_PATH` is useful for controlled testing, but it should not be the
default production deployment strategy.

### Link-time and runtime search are different

- `-L/path` tells the linker where to search during the build;
- `DT_RUNPATH` or older `DT_RPATH` entries influence runtime lookup;
- `LD_LIBRARY_PATH` can influence lookup for a process environment;
- the loader cache and default directories provide system-level lookup paths.

Inspect dynamic metadata:

```bash
readelf -d app
readelf -d libsensor.so.1.0
```

The `soname` usually identifies an ABI generation, for example
`libsensor.so.1`. An incompatible ABI release should normally receive a new
major `soname`.

## 12. C Linkage, C++ Name Mangling, And `extern "C"`

C++ supports overloads:

```cpp
void log_value(int value);
void log_value(double value);
```

The linker needs distinct binary symbols for these functions. A C++ compiler
therefore commonly encodes the function name, parameter types, namespace, and
class context into each symbol. This is name mangling.

```bash
nm logger.o
nm -C logger.o
```

The exact mangled spelling is compiler- and ABI-specific. Never design a
portable interface around a hard-coded C++ mangled name.

### Calling a C library from C++

Shared header:

```c
// sensor_c.h
#ifndef SENSOR_C_H
#define SENSOR_C_H

#ifdef __cplusplus
extern "C" {
#endif

int sensor_read(void);

#ifdef __cplusplus
}
#endif

#endif
```

C implementation:

```c
// sensor_c.c
#include "sensor_c.h"

int sensor_read(void) {
    return 42;
}
```

C++ caller:

```cpp
// main.cpp
#include "sensor_c.h"

#include <iostream>

int main() {
    std::cout << sensor_read() << '\n';
}
```

Build:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -c sensor_c.c -o sensor_c.o
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp sensor_c.o -o app
```

`extern "C"` specifies C language linkage when the declaration is processed by
C++. Describing it as "disabling name mangling" is a useful first
approximation, but it is not a complete ABI solution.

It does not:

- turn C++ implementation code into valid C;
- permit C-linkage overloads;
- guarantee compatible structure layout;
- fix ownership or allocator mismatches;
- make C++ exceptions safe across a C boundary;
- guarantee compatibility across arbitrary compiler options and runtimes.

## 13. ABI: The Binary Contract

An API describes how source code uses an interface. An ABI describes how
compiled components interact.

An ABI may define:

- symbol naming;
- calling conventions;
- parameter and return-value passing;
- object size, layout, alignment, and padding;
- virtual table representation;
- exception handling and RTTI conventions;
- standard-library and runtime compatibility;
- compiler options that affect binary interfaces.

Source compatibility does not guarantee binary compatibility. For example,
adding a data member to a public class changes its size and layout even if
existing source calls remain valid.

For a boundary that must remain stable, a narrow C-compatible API is often
easier to control:

```c
typedef struct sensor_handle sensor_handle;

sensor_handle* sensor_create(void);
int sensor_read(sensor_handle* handle, int* output);
void sensor_destroy(sensor_handle* handle);
```

The implementation may be C++, while the public boundary avoids exposing STL
types, exceptions, or class layout.

## 14. Practical Usage

### Embedded builds

- Use the cross-compiler and target triple intended for the hardware.
- Keep ABI-affecting flags consistent across translation units.
- Generate a linker map to inspect sections and large symbols.
- Control startup objects, runtime libraries, and optimization settings.
- Keep feature macros explicit and consistent.
- Treat linker scripts and section placement as an advanced extension of this
  build model, not as kernel-driver material.

Generate a linker map:

```bash
g++ main.o sensor.o -Wl,-Map,app.map -o app
```

### Enterprise libraries

- Separate public headers from private implementation headers.
- Make each header self-contained.
- Export only intentional public symbols.
- Version shared-library ABIs deliberately.
- Document supported compilers, language standards, runtimes, and build flags.
- Build important configurations in CI rather than only the default one.
- Preserve verbose command lines and build logs for reproducible diagnosis.

## 15. Key Comparisons

### Build stages

| Stage | Typical input | Typical output | Typical failure |
| --- | --- | --- | --- |
| Preprocessing | Source and headers | `.i` or `.ii` | Include or macro problem |
| Compilation | Preprocessed source | `.s` or internal IR | Syntax, type, semantic error |
| Assembly | Assembly | `.o` | Target or assembler error |
| Linking | Objects and libraries | Executable or shared object | Missing or duplicate symbol |
| Loading | Executable and shared objects | Running process | Missing or incompatible dependency |

### Static and shared libraries

| Topic | Static `.a` | Shared `.so` |
| --- | --- | --- |
| Link result | Selected object code enters the output | Output records a runtime dependency |
| Deployment | Archive usually not required on target | Compatible shared object required |
| Update | Application must be relinked | Library may be replaced if ABI-compatible |
| Main diagnostics | Symbol resolution and input order | Link resolution, loader search, and ABI |

### C and C++ linkage

| Topic | C | C++ | Engineering guidance |
| --- | --- | --- | --- |
| Overloading | Not supported | Supported | C++ needs distinct symbols for overloads |
| Symbol naming | Usually simpler | Commonly mangled | Exact form depends on ABI |
| Shared header | Plain C declaration | Guarded `extern "C"` declaration | Use `#ifdef __cplusplus` |
| Stable boundary | Often easier to maintain | More compiler/runtime-sensitive | Keep the ABI narrow and ownership explicit |

## 16. Common Failures And Reasoning

### `undefined reference`

Common causes:

- the defining object file was omitted;
- a required `-l<name>` option is missing;
- a static library appears before the object that needs it;
- declaration and definition differ in type, qualifier, or namespace;
- C++ consumes a C declaration without C language linkage;
- C++ object files are final-linked with `gcc` without required C++ runtime
  libraries.

Reasoning process:

1. Did the failure occur during compilation or final linking?
2. Which exact symbol does the calling object require?
3. Which object or library should define it?
4. Does that component define a different spelling because of signature or
   linkage differences?

### `multiple definition`

Common causes:

- an ordinary external function or variable is defined in a header;
- one source or object file was added to the build twice;
- a global object is defined in several source files instead of declared
  `extern`;
- stale and current objects are linked together.

### Shared library found at link time but not load time

Common causes:

- `-L` was configured, but the runtime library was never deployed;
- `RUNPATH` is missing or incorrect;
- `soname` links or package symlinks are incorrect;
- the library has an incompatible architecture;
- the loader finds a different ABI generation.

### ABI mismatch

A module may load but still fail because:

- a public class layout changed;
- compiler or standard-library runtimes differ;
- calling-convention flags differ;
- exceptions or ownership cross an unsafe boundary;
- one module uses incompatible compile-time feature macros.

## 17. Debugging Workflow

### Step 1: Classify the stage

| Failure point | First area to inspect |
| --- | --- |
| `-E` preprocessing | Includes, macros, feature conditions |
| `-c` compilation | Syntax, types, semantics, target options |
| Final link | Symbols, object list, libraries, order, ABI options |
| Program startup | Loader paths, dependency metadata, architecture |
| Runtime | Logic, undefined behavior, undetected ABI mismatch |

### Step 2: Inspect the actual command

```bash
g++ -v main.cpp sensor.cpp -o app
g++ -### main.cpp sensor.cpp -o app
```

Check:

- the compiler driver;
- include paths and macro definitions;
- selected language standard;
- object and library order;
- target architecture;
- optimization, PIC/PIE, visibility, and ABI-related options.

### Step 3: Inspect symbols and relocations

```bash
nm -C main.o
nm -C sensor.o
readelf -Ws main.o
readelf -r main.o
objdump -dr main.o
```

### Step 4: Generate a linker map

```bash
g++ main.o sensor.o -Wl,-Map,app.map -o app
```

A map file helps identify:

- which object or library supplied a symbol;
- where sections were placed;
- which code or data contributes most to binary size.

### Step 5: Inspect dynamic dependencies

```bash
readelf -d app
ldd ./app
LD_DEBUG=libs,bindings ./app
```

Use `ldd` only on trusted binaries. `LD_DEBUG` can produce a large trace, so
enable only the categories relevant to the loader problem.

### Step 6: Build a minimal reproduction

Reduce the problem to two or three translation units. This quickly separates a
language/linkage issue from build-system complexity.

## 18. Best Practices

- [ ] Put interfaces in headers and ordinary non-inline definitions in source
      files.
- [ ] Give every header a unique include guard or use `#pragma once`
      consistently.
- [ ] Make headers self-contained and independent of accidental include order.
- [ ] Do not place `using namespace` directives in public headers.
- [ ] Prefer `constexpr`, functions, or templates over function-like macros.
- [ ] Treat `inline` primarily as a language and ODR tool.
- [ ] Use the appropriate compiler driver for final linking, especially for
      C++.
- [ ] Place static libraries after objects that require them.
- [ ] Never allow C++ exceptions to cross a C ABI boundary.
- [ ] Do not expose ambiguous ownership or STL types through a stability-focused
      ABI.
- [ ] Keep feature macros and ABI-affecting options consistent across
      translation units.
- [ ] Minimize exported symbols and version shared-library ABIs deliberately.
- [ ] Retain linker maps for size-sensitive embedded builds.

## 19. Interview Readiness

### Explain the path from `.cpp` to an executable

> The source file is preprocessed into a translation unit. The compiler parses,
> validates, optimizes, and generates assembly or machine code. The assembler
> produces a relocatable object file. The linker combines objects and
> libraries, resolves symbols and relocations, and creates an executable. At
> startup, the loader maps the executable and required shared libraries, runs
> initialization, and eventually calls `main`.

### Why does `undefined reference` occur?

The compiler saw a valid declaration, but the linker could not find a matching
definition among the provided objects and libraries. Check missing inputs,
static-library order, signature and namespace differences, and C/C++ language
linkage.

### Why does `multiple definition` occur?

More than one object provides an external definition that must be unique. A
common cause is defining an ordinary function or global object in a header
included by multiple translation units.

### What does `extern "C"` do?

It gives declarations C language linkage when compiled as C++. This commonly
produces symbols compatible with a C library. It does not guarantee data-layout
compatibility, ownership safety, exception safety, or a complete cross-toolchain
ABI.

### Does `inline` force inlining?

No. It has language and ODR meaning, allowing qualifying definitions in
multiple translation units. The optimizer independently decides whether to
replace a call with the function body.

### Senior-level discussion

Be prepared to explain:

- why static archive resolution can depend on input order;
- how `-L`, `RUNPATH`, `LD_LIBRARY_PATH`, and `soname` differ;
- how to expose a stable C API over a C++ implementation;
- which source-compatible changes can still break ABI;
- what symbol visibility, versioning, LTO, PGO, weak symbols, and linker scripts
  are intended to solve.

## 20. Practice

### Basic

1. Build `hello.cpp` into `.ii`, `.s`, `.o`, and an executable using `-E`,
   `-S`, and `-c`.
2. Use `nm` to identify defined and undefined symbols.
3. Create one compile-time, one link-time, and one load-time failure. Record how
   their diagnostics differ.

### Intermediate

1. Split a program into `sensor.hpp`, `sensor.cpp`, and `main.cpp`; compile each
   translation unit separately.
2. Trigger `multiple definition` with a non-inline header definition, then fix
   it by moving the definition into a source file.
3. Create `libsensor.a`, try both library orders, and explain the result.
4. Build a small C library and call it from C++ through a guarded `extern "C"`
   header.

### Advanced

1. Build `libsensor.so.1.0` with a `soname`, then inspect `DT_NEEDED` and
   `RUNPATH`.
2. Deliberately break runtime lookup and diagnose it with `readelf` and
   `LD_DEBUG=libs`.
3. Generate a linker map for a multi-object program and identify its largest
   symbols.
4. Design a C ABI for a C++ sensor implementation without exposing classes,
   STL containers, or exceptions.

## 21. Summary

- Each preprocessed source file forms one translation unit.
- The compiler works primarily on individual translation units; the linker
  connects them through symbols.
- A declaration introduces an entity; a definition supplies its body or
  storage.
- An object file contains code, data, symbols, and relocations, but is not yet a
  complete executable.
- Compilation, linking, and loading are separate stages with different failure
  modes.
- A static library is an archive searched by the linker; a shared library
  remains a runtime dependency.
- `-L` affects link-time lookup and does not replace runtime loader
  configuration.
- C++ name mangling supports overloads, but its spelling depends on the ABI.
- `extern "C"` specifies language linkage; it is not a complete ABI adapter.
- `inline` participates in ODR semantics; optimizer inlining is a separate
  decision.
- Effective build debugging starts by classifying the failing stage and
  inspecting commands, symbols, relocations, and dependencies.

## 22. Reference Notes

- GCC, Overall Options:
  <https://gcc.gnu.org/onlinedocs/gcc/Overall-Options.html>
- GNU `ld`: <https://sourceware.org/binutils/docs/ld.html>
- Linux dynamic linker:
  <https://man7.org/linux/man-pages/man8/ld.so.8.html>
- Itanium C++ ABI:
  <https://itanium-cxx-abi.github.io/cxx-abi/abi.html>
- C++ draft, linkage and ODR:
  <https://eel.is/c++draft/basic.link>,
  <https://eel.is/c++draft/basic.def.odr>

Commands involving `.so`, `readelf`, `LD_LIBRARY_PATH`, `RUNPATH`, and `soname`
use Linux/ELF as the concrete environment. The language concepts involving
translation units, declarations, definitions, ODR, and language linkage remain
central C/C++ knowledge, while tools and binary formats differ on platforms
such as Windows/MSVC.
