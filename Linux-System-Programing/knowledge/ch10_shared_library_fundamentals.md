# Chapter 10.1 - Shared Library Fundamentals

> Topics: static libraries, shared libraries, `.so`, PIC, soname, linker name, `ldconfig`, runtime library lookup, ABI compatibility.
> Main sources: TLPI Ch41; DevLinux Module 01, section 1.3 and shared-library exercise.
> Production context: used when building deployable C/C++ services, embedded Linux binaries, SDKs, system libraries, plugin hosts, and products that must upgrade libraries without rebuilding every executable.

---

## Problem It Solves

Many programs reuse the same code. If every executable copies that code into itself, the system pays in three ways:

- disk usage grows because each executable stores another copy;
- memory usage grows because running processes hold separate copies;
- upgrades become painful because every executable must be relinked to pick up a fix.

Linux shared libraries solve this by putting reusable code in a shared object file. At build time, the executable records that it needs the library. At runtime, the dynamic linker finds and maps the library into the process.

For production work, this topic explains why a binary works on one machine but fails on another, why `ldd` shows a different library than expected, why `-fPIC` matters, and when a library upgrade is ABI-safe.

## Learning Roadmap

| Level | Learn | Goal |
|-------|-------|------|
| Must know | Static vs shared linking, `.a` vs `.so`, `-L`, `-l`, `-shared`, `-fPIC`, runtime dynamic linker | Build and run a small shared-library program without guessing |
| Work useful | `soname`, real name, linker name, `ldconfig`, `RPATH`/`RUNPATH`, `$ORIGIN`, `ldd`, `readelf`, `objdump` | Package and debug libraries on real systems |
| Recognize | `TEXTREL`, symbol interposition, `-Bsymbolic`, static fallback, major/minor ABI policy | Understand advanced failures without turning first-pass learning into linker trivia |

## Core Vocabulary

| Term | Meaning | Example / note |
|------|---------|----------------|
| Object file | Compiled machine code that is not yet a final executable | `mod1.o` |
| Static library | Archive of object files copied into the executable at link time | `libdemo.a`, created with `ar rcs` |
| Shared library | ELF shared object loaded at runtime | `libdemo.so` |
| Static linking | Build-time step that combines object files and records or copies dependencies | Every program has this phase |
| Dynamic linking | Runtime step that finds and loads shared libraries | Performed by `ld-linux.so` |
| PIC | Position-independent code that can run at different virtual addresses | Compile library objects with `-fPIC` |
| Real name | Actual shared-library file containing code and full version | `libdemo.so.1.0.2` |
| Soname | ABI-level name embedded in dependents | `libdemo.so.1` |
| Linker name | Development-time name used by `-l` | `libdemo.so` for `-ldemo` |
| `DT_NEEDED` | ELF dependency entry recorded in an executable or shared object | Inspect with `readelf -d` |
| `DT_SONAME` | ELF tag storing the library soname | Inspect with `objdump -p` |
| `RPATH` / `RUNPATH` | Runtime search paths embedded in an ELF file | Prefer controlled paths, often with `$ORIGIN` |
| `LD_LIBRARY_PATH` | Environment variable adding runtime library search directories | Useful for tests, risky for production |
| `ldconfig` | Updates loader cache and soname symlinks | Maintains `/etc/ld.so.cache` |
| ABI | Binary contract between executable and library | Function signatures, exported symbols, struct layout |
| Symbol interposition | A symbol definition earlier in lookup order overrides another | Main executable can override a library symbol |

## Concept Overview

Static libraries make deployment simple because the executable contains the needed code. Shared libraries make systems more maintainable because the executable contains dependency metadata and the code lives in a separate `.so`.

The mental model is:

```text
build time
    source.c -> object.o -> executable records library dependency

runtime
    execve()
        |
        v
    dynamic linker reads ELF dependency metadata
        |
        v
    finds matching .so
        |
        v
    maps library code into process address space
```

Two details matter more than most flags:

- shared-library code must be safe to load at varying virtual addresses, so compile it as PIC;
- the executable should depend on the library soname, not a random filename, so compatible minor upgrades can happen cleanly.

## System Context

Shared libraries sit between compilation, process startup, memory mapping, and deployment.

| Layer | Role |
|-------|------|
| Compiler | Emits object code, usually with `-fPIC` for library objects |
| Static linker | Builds executables and shared objects; records `DT_NEEDED` and `DT_SONAME` metadata |
| ELF | Stores dependency and search-path metadata |
| Dynamic linker | Loads shared libraries into the process at startup |
| VFS / filesystem | Provides library files, symlinks, and installation directories |
| Virtual memory | Maps shared text pages; each process still has its own data variables |
| Packaging / ops | Maintains library versions, cache, and symlinks |

Failure usually appears as startup errors, wrong-version bugs, unresolved symbols, or ABI breakage after an upgrade.

## Architecture

```text
libdemo.so      -> linker name used by gcc -ldemo
libdemo.so.1    -> soname symlink embedded in executables
libdemo.so.1.0.2 -> real file containing ELF code
```

Important runtime state:

| Component | What it tracks |
|-----------|----------------|
| Executable | `DT_NEEDED` dependencies and optional `RPATH`/`RUNPATH` |
| Shared object | exported symbols, `DT_SONAME`, relocation data, dependency list |
| Dynamic linker | library search, mapping, relocation, symbol binding |
| `/etc/ld.so.cache` | cached mapping from library names to installed paths |
| Symlink chain | which real library satisfies a soname or linker name |

Important nuance: multiple processes can share the read-only code pages of a shared library, but global and static variables from the library are process-private.

## Execution Flow

### Flow 1: Build a Shared Library

```text
source files
    |
    v
gcc -fPIC -c
    |
    v
PIC object files
    |
    v
gcc -shared -Wl,-soname,libdemo.so.1
    |
    v
libdemo.so.1.0.0
```

### Flow 2: Link an Executable

```text
gcc main.o -L./lib -ldemo
    |
    v
linker finds libdemo.so
    |
    v
executable records DT_NEEDED = libdemo.so.1
```

### Flow 3: Run the Program

```text
execve("./app")
    |
    v
kernel starts ELF interpreter / dynamic linker
    |
    v
dynamic linker reads DT_NEEDED
    |
    v
searches for libdemo.so.1
    |
    v
maps library and performs relocation
```

### Flow 4: Production Install

```text
install real file
    |
    v
run ldconfig
    |
    v
soname symlink and /etc/ld.so.cache updated
    |
    v
new processes can find the library
```

### Flow 5: Compatible Upgrade

```text
old: libdemo.so.1.0.1
    |
    v
install compatible libdemo.so.1.0.2
    |
    v
ldconfig points libdemo.so.1 to latest minor version
    |
    v
restarted programs use the new file
```

Already-running processes continue using the old mapped library until they exit.

## 10.1 API / Topic Sections

### 10.1.1 Static Libraries

Use static libraries when you want a self-contained executable or the target environment may not have the needed runtime library.

```bash
gcc -c mod1.c mod2.c
ar rcs libdemo.a mod1.o mod2.o
gcc -o app main.c -L. -ldemo
```

Pitfall: if both `libdemo.a` and `libdemo.so` are available, the linker normally prefers the shared library. Specify the `.a` path or use static-linking options when you really need static.

### 10.1.2 Shared Libraries and PIC

Use shared libraries when multiple programs use the same code, when the code must be upgraded independently, or when building an SDK/system package.

```bash
gcc -fPIC -c mod1.c mod2.c
gcc -shared -o libdemo.so mod1.o mod2.o
```

Production pitfall: non-PIC objects can create text relocations. They may fail on some architectures or reduce memory sharing.

### 10.1.3 Real Name, Soname, and Linker Name

Use the standard naming model for libraries that will be installed or upgraded:

```bash
gcc -fPIC -c demo.c
gcc -shared -Wl,-soname,libdemo.so.1 -o libdemo.so.1.0.0 demo.o
ln -s libdemo.so.1.0.0 libdemo.so.1
ln -s libdemo.so.1 libdemo.so
```

Rule:

- compatible change: keep the same soname, bump the minor real name;
- incompatible ABI change: create a new major soname.

### 10.1.4 Runtime Search

If the dependency string contains `/`, the dynamic linker treats it as a pathname. Otherwise, the usual search order is:

1. `DT_RPATH`, only if no `DT_RUNPATH` exists
2. `LD_LIBRARY_PATH`
3. `DT_RUNPATH`
4. `/etc/ld.so.cache`
5. `/lib`
6. `/usr/lib`

Linux distributions may also use architecture-specific or multiarch directories such as `/lib64` or `/usr/lib/x86_64-linux-gnu`.

Security note: `LD_LIBRARY_PATH` is ignored for set-user-ID and set-group-ID executables.

### 10.1.5 Compatibility and ABI

An update is compatible only if existing binaries can keep calling the library correctly.

Compatible:

- bug fixes;
- performance improvements;
- adding new public functions;
- keeping public structure layout stable.

Incompatible:

- changing function signatures;
- removing exported symbols;
- changing documented semantics in a breaking way;
- changing public structure layout in a way old binaries cannot tolerate.

## Work-Useful Patterns

| Pattern | Why it helps |
|---------|--------------|
| Build library objects with `-fPIC` by default | Avoid architecture-specific failures and `TEXTREL` surprises |
| Use soname and standard symlink chain | Makes upgrades predictable |
| Use `ldconfig` after install/update/remove | Keeps loader cache and soname links consistent |
| Use `$ORIGIN` for app-local libraries | Allows relocatable bundles without global installs |
| Inspect actual ELF metadata before guessing | `readelf` and `objdump` reveal what the binary really records |
| Treat ABI as a contract | Prevents production breakage after library upgrades |

## Advanced / Recognize First

| Topic | Know this much |
|-------|----------------|
| `TEXTREL` | Indicates text relocations, often from non-PIC code inside a shared object |
| `-Bsymbolic` | Makes a shared library prefer its own global definitions for internal references |
| `DT_RPATH` vs `DT_RUNPATH` | Different precedence relative to `LD_LIBRARY_PATH`; modern builds usually use `RUNPATH` |
| Static linking | Useful for self-contained or constrained targets, but larger and harder to patch centrally |
| Symbol interposition | Normal ELF behavior where earlier symbols can override later ones |
| `LD_RUN_PATH` | Build-time environment alternative for embedding runtime search paths when `-rpath` is absent |

## Example

### Example 1: Minimal Shared Library

`demo.c`:

```c
#include <stdio.h>

void demo_hello(void) {
    puts("hello from libdemo");
}
```

`main.c`:

```c
void demo_hello(void);

int main(void) {
    demo_hello();
    return 0;
}
```

Build and run:

```bash
gcc -Wall -Wextra -g -fPIC -c demo.c -o demo.o
gcc -shared -Wl,-soname,libdemo.so.1 -o libdemo.so.1.0.0 demo.o
ln -sf libdemo.so.1.0.0 libdemo.so.1
ln -sf libdemo.so.1 libdemo.so
gcc -Wall -Wextra -g -o app main.c -L. -ldemo
LD_LIBRARY_PATH=. ./app
```

What it teaches:

- `-fPIC` builds relocatable library code;
- `-shared` creates the `.so`;
- the executable links through `libdemo.so`, but records the soname.

### Example 2: Inspect What Was Recorded

```bash
readelf -d ./app | grep NEEDED
objdump -p ./libdemo.so.1.0.0 | grep SONAME
ldd ./app
```

What it teaches:

- `readelf` shows the executable dependency list;
- `objdump` shows the library soname;
- `ldd` shows how dependencies resolve on this machine.

## Debugging

Useful commands:

```bash
ldd ./app
readelf -d ./app
objdump -p ./app | grep -E 'NEEDED|RPATH|RUNPATH'
objdump -p ./libdemo.so.1.0.0 | grep SONAME
readelf -d ./libdemo.so.1.0.0 | grep TEXTREL
nm -D ./libdemo.so.1.0.0
ldconfig -p | grep demo
```

Common bugs:

| Bug | Symptom | Fix / check |
|-----|---------|-------------|
| Library not found | `cannot open shared object file` | Check `ldd`, search paths, soname symlink, and `ldconfig` cache |
| Missing soname symlink | Executable needs `libdemo.so.1`, but only real file exists | Create symlink or run `ldconfig` after install |
| Wrong version loaded | Behavior differs across machines | Inspect `ldd`, `RUNPATH`, `LD_LIBRARY_PATH`, package versions |
| Non-PIC object in `.so` | `TEXTREL` appears or link fails | Rebuild all library objects with `-fPIC` |
| ABI break after minor update | Old binary crashes or corrupts data | Restore ABI compatibility or bump major soname |
| Symbol interposition surprise | Library calls unexpected implementation | Inspect exported symbols; consider visibility controls or `-Bsymbolic` carefully |

## Real-world Usage

| Scenario | Practical design |
|----------|------------------|
| Linux distro package | Install real library, run `ldconfig`, expose development linker name in `-dev` package |
| Embedded product | Choose static linking for standalone images or shared libs for updatable components |
| Backend service with private native libs | Use `$ORIGIN` `RUNPATH` so deployment is relocatable |
| SDK/library shipped to customers | Maintain soname discipline and document ABI compatibility |
| Production startup failure | Use `ldd`, `readelf`, and `LD_DEBUG=libs` before changing random paths |

## Interview-Relevant Questions

1. What is the difference between a static library and a shared library?
2. What happens at build time and runtime when an executable uses a shared library?
3. Why should shared-library object files usually be compiled with `-fPIC`?
4. What are real name, soname, and linker name?
5. Why does an executable usually record the soname instead of the real file name?
6. What does `ldconfig` do?
7. Why is `LD_LIBRARY_PATH` acceptable for testing but risky for production?
8. What is the dynamic linker's library search order?
9. How would you debug `cannot open shared object file`?
10. How do `RPATH`, `RUNPATH`, and `$ORIGIN` help deployment?
11. Why can two compatible minor versions share the same soname?
12. What kind of change requires a new major soname?
13. Why can a shared-library upgrade affect a program without relinking it?
14. Do shared-library global variables become shared across processes?
15. How would you inspect an executable's shared-library dependencies?
16. What is a `TEXTREL`, and why is it a warning sign?
17. Why might the main executable override a symbol in a shared library?
18. When would static linking still be useful?

## Key Takeaways

- Static libraries copy code into the executable; shared libraries load code at runtime.
- A professional `.so` build needs PIC, a soname, and a sane symlink/versioning policy.
- The dynamic linker uses ELF metadata, search paths, `/etc/ld.so.cache`, and standard library directories to find libraries.
- `ldconfig`, `ldd`, `readelf`, `objdump`, and `nm` are the first tools to use when debugging library problems.
- ABI compatibility determines whether a minor update is safe or a new major soname is required.
