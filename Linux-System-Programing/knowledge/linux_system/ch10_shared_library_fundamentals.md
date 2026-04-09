# Shared Library Fundamentals - `.so`, `soname`, `ldconfig`, PIC, and Run-Time Linking

## Problem It Solves

When multiple programs reuse the same functionality, copying the same object code into every executable wastes disk space, wastes memory, and makes upgrades painful.

Static linking has a simple model:

- object code is copied into the executable at build time;
- each program carries its own copy;
- if the library changes, dependent executables usually need to be relinked.

Shared libraries solve these problems by moving reusable code into a shared object file that is loaded at run time:

- executables become smaller;
- multiple processes can share the same library code in memory;
- compatible library upgrades can be installed without relinking every application.

This topic is fundamental because it explains how Linux programs are built, deployed, upgraded, and debugged in real systems.

## Concept Overview

A library is reusable precompiled code. On Linux, the two most important library models are:

- **static library**: archive file, usually `libname.a`
- **shared library**: shared object file, usually `libname.so`

Static linking:

- linker extracts required object modules from the archive;
- code becomes part of the executable.

Shared linking:

- executable records dependency information instead of copying the code;
- the dynamic linker loads required shared libraries at run time.

Key shared-library concepts:

- **PIC** (`-fPIC`): position-independent code needed for shared libraries
- **real name**: actual library file name, e.g. `libdemo.so.1.0.2`
- **soname**: ABI-level name embedded into executables, e.g. `libdemo.so.1`
- **linker name**: name used at link time, e.g. `libdemo.so`
- **dynamic dependency list**: libraries recorded in the executable (`DT_NEEDED`)
- **dynamic linker**: loader that resolves and loads shared libraries at run time

## System Context

Shared libraries sit at the intersection of:

- compilation and linking;
- executable format (ELF);
- process start-up;
- deployment and system administration;
- ABI compatibility and long-term maintenance.

Relevant components in the system:

- `gcc` / `ld`: create executables and shared objects
- ELF metadata: `DT_NEEDED`, `DT_SONAME`, `DT_RPATH`, `DT_RUNPATH`
- dynamic linker: `ld-linux.so`
- library cache: `/etc/ld.so.cache`
- library directories: `/lib`, `/usr/lib`, `/usr/local/lib`, and paths from `/etc/ld.so.conf`
- tools: `ldd`, `objdump`, `readelf`, `nm`, `ldconfig`

This topic is also directly connected to:

- deployment practices;
- plugin systems;
- build systems;
- binary compatibility;
- security-sensitive runtime loading behavior.

## Architecture

## 1) Static library architecture

A static library is an archive of object files.

Typical flow:

1. compile source into `.o`
2. package `.o` files using `ar`
3. linker extracts only required object modules when building the executable

Example:

```bash
gcc -c mod1.c -o mod1.o
gcc -c mod2.c -o mod2.o
ar rcs libdemo.a mod1.o mod2.o
gcc -o prog prog.c -L. -ldemo
```

Strengths:

- self-contained executable
- simple deployment
- fewer runtime dependencies

Weaknesses:

- larger executable
- duplicated code in memory across processes
- harder upgrade path

## 2) Shared library architecture

A shared library is an ELF shared object built with `-shared`, typically from object files compiled with `-fPIC`.

Example:

```bash
gcc -g -c -fPIC -Wall mod1.c mod2.c mod3.c
gcc -g -shared -o libfoo.so mod1.o mod2.o mod3.o
```

At link time:

- executable records dependency on the shared library;
- object code is not copied into the executable.

At run time:

- dynamic linker resolves the dependency;
- library is loaded into memory if necessary;
- multiple processes can share the library code pages.

Important nuance:

- shared library **code** can be shared across processes;
- shared library **global/static variables** are not shared between processes.

## 3) Why PIC matters

Shared libraries must be loadable at different virtual addresses at run time.  
This is why position-independent code is important.

`-fPIC` changes how the compiler emits code for:

- global and static data access;
- external symbol access;
- function addresses;
- string constant references.

Without `-fPIC`:

- some architectures may fail to build or use shared libraries correctly;
- on Linux/x86-32, it may still work, but code pages needing text relocation lose some sharing benefits.

Quick checks:

```bash
readelf -s mod1.o | grep _GLOBAL_OFFSET_TABLE_
readelf -d libfoo.so | grep TEXTREL
```

`TEXTREL` is a warning sign that some module was not compiled as proper PIC.

## 4) Real name, soname, and linker name

Shared-library versioning relies on three names:

### Real name

The actual file, including major and minor version:

```text
libdemo.so.1.0.2
```

### Soname

The ABI-level name, usually including only the major version:

```text
libdemo.so.1
```

This is the name typically embedded in the executable at link time.

### Linker name

The development-time name used with `-l`:

```text
libdemo.so
```

Typical symlink structure:

```text
libdemo.so      -> libdemo.so.1
libdemo.so.1    -> libdemo.so.1.0.2
libdemo.so.1.0.2
```

Why this matters:

- executables depend on the soname;
- soname points to latest compatible minor version in a major series;
- incompatible major versions can coexist cleanly.

## 5) `ldconfig` and system installation

Production libraries are usually installed in standard library directories rather than loaded through ad hoc environment variables.

`ldconfig` does two key things:

1. rebuilds `/etc/ld.so.cache`
2. creates or updates soname symlinks to the newest minor version within each major version

It searches:

- directories listed in `/etc/ld.so.conf`
- `/lib`
- `/usr/lib`

It should be run whenever:

- a shared library is installed;
- a library is upgraded;
- a library is removed;
- the configured library search directories change.

## 6) Compatibility and versioning

A library change is compatible only if the public API/ABI stays compatible.

Compatible change:

- keep function signatures and semantics stable;
- do not remove public symbols;
- do not break exported structure layouts;
- bug fixes and performance improvements are usually compatible;
- adding new public functions is usually compatible.

Incompatible change:

- changing function signatures;
- removing public functions/variables;
- changing semantics in a breaking way;
- breaking exported structure layout.

Rule of thumb:

- compatible upgrade -> new **minor** version
- incompatible upgrade -> new **major** version

## 7) Run-time symbol resolution

By default, symbol resolution follows traditional ELF/shared-library rules:

- a global symbol in the main executable can override one in a library;
- if multiple libraries define the same symbol, search order matters.

This is called symbol interposition.

If a shared library should preferentially bind its own internal global symbol references to definitions inside itself, it can be built with:

```bash
-Wl,-Bsymbolic
```

This is an advanced behavior, but important when reasoning about surprising symbol resolution bugs.

## Execution Flow

### Flow A: create and use a shared library

1. compile source to PIC object files
2. link shared object with `-shared`
3. optionally assign `soname`
4. create soname and linker-name symlinks
5. link program against library
6. run program; dynamic linker resolves dependencies and loads libraries

### Flow B: run-time shared library lookup

When a dependency name does **not** contain a slash, the dynamic linker searches in this order:

1. `DT_RPATH` if present and no `DT_RUNPATH`
2. `LD_LIBRARY_PATH`
3. `DT_RUNPATH`
4. `/etc/ld.so.cache`
5. `/lib`
6. `/usr/lib`

Security note:

- `LD_LIBRARY_PATH` is ignored for set-user-ID and set-group-ID executables.

### Flow C: production installation

1. build real library file
2. install to standard library directory
3. run `ldconfig`
4. ensure linker name exists for future builds if needed

### Flow D: nonstandard deployment

If library lives outside standard locations, possible approaches are:

- `LD_LIBRARY_PATH` for quick testing
- `-Wl,-rpath,...` or `RUNPATH`
- `$ORIGIN`-based rpath for relocatable app bundles

Example:

```bash
gcc -Wl,-rpath,'$ORIGIN'/lib ...
```

This is useful for turn-key applications distributed with private libraries.

## Example

### Example 1: Build and use a static library

```bash
gcc -c mod1.c -o mod1.o
gcc -c mod2.c -o mod2.o
ar rcs libdemo.a mod1.o mod2.o
gcc -o prog prog.c -L. -ldemo
```

### Example 2: Build a shared library with soname and standard naming

```bash
gcc -g -c -fPIC -Wall mod1.c mod2.c mod3.c
gcc -g -shared -Wl,-soname,libdemo.so.1 -o libdemo.so.1.0.1 \
    mod1.o mod2.o mod3.o
ln -s libdemo.so.1.0.1 libdemo.so.1
ln -s libdemo.so.1 libdemo.so
gcc -g -Wall -o prog prog.c -L. -ldemo
LD_LIBRARY_PATH=. ./prog
```

### Example 3: Inspect dependencies and metadata

```bash
ldd ./prog
objdump -p ./prog | grep -E 'NEEDED|RPATH|RUNPATH'
objdump -p ./libdemo.so.1.0.1 | grep SONAME
readelf -d ./libdemo.so.1.0.1 | grep -E 'SONAME|RPATH|RUNPATH|TEXTREL'
nm -A /usr/lib/lib*.so 2>/dev/null | grep ' crypt$'
```

## Debugging

### 1) Program fails with "cannot open shared object file"

Usually means:

- library is not in standard search path;
- missing `LD_LIBRARY_PATH` during local testing;
- missing `rpath/runpath`;
- `ldconfig` cache not updated;
- soname symlink is missing or wrong.

### 2) Wrong library version is loaded

Check:

- `ldd program`
- `objdump -p program | grep PATH`
- `readelf -d program`
- library symlink chain (`ls -l libname.so*`)

### 3) PIC/text relocation problems

Look for:

```bash
readelf -d libfoo.so | grep TEXTREL
```

If present, at least one object module may not have been compiled with `-fPIC`.

### 4) ABI breaks after upgrade

Common causes:

- public struct layout changed;
- function signature changed;
- public symbol removed;
- soname/major version not bumped when it should have been.

### 5) Symbol resolution surprises

If behavior changes because same symbol exists in main executable and shared library, inspect symbol interposition and consider whether `-Bsymbolic` is appropriate.

## Real-world Usage

- system libraries such as libc and many standard runtime libraries;
- plugin-capable applications;
- modular products where multiple executables share the same implementation;
- packaged applications that need controlled private library deployment via `rpath` or `$ORIGIN`;
- rolling production updates where compatible minor library upgrades should not force widespread relinking.

For embedded or tightly controlled deployment, static linking can still be useful when:

- runtime dependencies must be minimized;
- deployment must be self-contained;
- library availability on target systems is uncertain.

## Key Takeaways

1. Static libraries copy code into the executable; shared libraries defer code loading to run time.
2. Shared libraries save disk space and RAM, and make compatible upgrades easier.
3. Shared libraries should usually be built with `-fPIC` and `-shared`.
4. The real-name / soname / linker-name model is central to professional Linux library versioning.
5. `ldconfig` is a core operational tool for keeping soname links and the loader cache consistent.
6. Minor version updates are for compatible changes; incompatible ABI changes require a new major version.
7. Runtime loading depends on search rules involving `RPATH`, `RUNPATH`, `LD_LIBRARY_PATH`, `ld.so.cache`, and standard library directories.
