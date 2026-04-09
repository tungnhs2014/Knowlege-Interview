# Advanced Shared Libraries - `dlopen()`, `dlsym()`, Symbol Visibility, Version Scripts, `LD_PRELOAD`, and `LD_DEBUG`

## Problem It Solves

Shared-library fundamentals explain how libraries are built and loaded automatically at process start. Real systems often need more control than that default model provides.

Typical higher-level requirements include:

- loading a module only when a feature is needed;
- implementing a plug-in architecture;
- allowing a library to call back into the main program;
- preventing internal symbols from leaking into the public ABI;
- keeping old programs working after incompatible changes to a library interface;
- temporarily overriding functions for testing or instrumentation;
- understanding why the dynamic linker loaded a particular library or bound a symbol in an unexpected way.

The advanced shared-library features in this chapter solve those problems.

## Concept Overview

This topic centers on explicit interaction with the dynamic linker and on tighter control of a library's exported ABI.

The main concepts are:

- **dynamic loading** with `dlopen()`, `dlsym()`, `dlclose()`, and `dlerror()`;
- **run-time symbol lookup** using library handles and pseudohandles such as `RTLD_DEFAULT` and `RTLD_NEXT`;
- **symbol visibility control** using `static`, the GCC `visibility("hidden")` attribute, and linker version scripts;
- **symbol versioning** so one shared library can provide multiple versions of the same interface;
- **automatic initialization and finalization** using constructor and destructor functions;
- **library preloading** with `LD_PRELOAD`;
- **dynamic-linker tracing** with `LD_DEBUG`.

Together, these mechanisms let a program control when libraries are loaded, which symbols are visible, how symbol binding works, and how to debug the linker's behavior.

## System Context

Advanced shared-library behavior sits across several layers of the Linux user-space runtime:

- the executable and its ELF metadata;
- the dynamic linker;
- shared libraries and their dependency trees;
- the compiler and linker used to build those libraries;
- environment variables that influence run-time loading behavior.

Relevant components include:

- `libdl`, which provides the `dlopen()` API;
- `ld-linux.so`, the dynamic linker;
- shared objects loaded automatically at program start or explicitly at run time;
- linker options such as `--export-dynamic` and `--version-script`;
- environment variables such as `LD_PRELOAD`, `LD_BIND_NOW`, and `LD_DEBUG`.

This topic is especially important for:

- plug-in systems;
- host applications that load optional modules;
- ABI design and maintenance;
- function interposition and testing;
- debugging deployment and symbol-resolution problems.

## Architecture

## 1) Dynamic loading with the `dlopen()` API

The dynamic linker normally loads all libraries listed in an executable's dependency list when the process starts. The `dlopen()` API allows additional shared libraries to be loaded later at run time.

Core functions:

- `dlopen(libfilename, flags)` loads a shared library and returns a handle;
- `dlsym(handle, symbol)` finds the address of a symbol;
- `dlclose(handle)` closes a previously opened library handle;
- `dlerror()` returns the most recent error string;
- `dladdr(addr, &info)` reports information about a loaded address.

Programs using this API on Linux must link with:

```bash
gcc ... -ldl
```

`dlopen()` behavior:

- if `libfilename` contains `/`, it is treated as a pathname;
- otherwise the dynamic linker searches for the library using its normal search rules;
- if the library depends on other libraries, those libraries are also loaded recursively.

Each library handle has a reference count:

- repeated `dlopen()` calls on the same library return the same handle;
- the library is loaded only once;
- each `dlopen()` increments the count;
- each `dlclose()` decrements the count;
- unloading happens only when the count reaches 0 and the library is no longer needed by other loaded libraries.

## 2) `dlopen()` flags and binding strategy

`flags` for `dlopen()` must include exactly one of:

- `RTLD_LAZY`: resolve undefined function symbols only when needed during execution;
- `RTLD_NOW`: resolve all undefined symbols before `dlopen()` returns.

Important implications:

- `RTLD_LAZY` usually opens a library faster;
- `RTLD_NOW` fails earlier if there are unresolved symbols;
- variable references are resolved immediately even with `RTLD_LAZY`.

Additional visibility-related flags:

- `RTLD_GLOBAL`: make symbols in the library and its dependency tree available for resolving references in subsequently loaded libraries and for broader `dlsym()` lookup;
- `RTLD_LOCAL`: keep those symbols unavailable to subsequently loaded libraries; this is the Linux default.

Linux/glibc-specific flags:

- `RTLD_NODELETE`: do not unload the library during `dlclose()`, even if the reference count reaches 0;
- `RTLD_NOLOAD`: do not load the library, but return its handle if it is already loaded;
- `RTLD_DEEPBIND`: prefer the library's own definitions when resolving symbols referenced by that library.

Special case:

- `dlopen(NULL, flags)` returns a handle for the main program, allowing `dlsym()` to search the main executable followed by other globally visible libraries.

## 3) Symbol lookup with `dlsym()`

`dlsym()` searches for a named symbol in:

- the library referred to by `handle`;
- the libraries in that library's dependency tree.

The symbol may name:

- a function;
- a variable.

One subtle point is that a successful symbol value may itself be `NULL`. Because `dlsym()` also returns `NULL` when lookup fails, the correct pattern is:

1. call `dlerror()` first to clear any old error state;
2. call `dlsym()`;
3. call `dlerror()` again;
4. treat a non-`NULL` error string as failure.

For function pointers, ISO C does not allow direct assignment from `void *` to a function pointer. TLPI uses this pattern:

```c
*(void **) (&funcp) = dlsym(handle, "symbol_name");
```

This avoids relying on nonportable assumptions about function-pointer conversions.

## 4) `dlsym()` pseudohandles

In addition to a normal library handle, `dlsym()` can use pseudohandles:

- `RTLD_DEFAULT`: search using the default dynamic-linker model, starting with the main program and then proceeding through loaded libraries;
- `RTLD_NEXT`: search for the next occurrence of a symbol after the current library.

`RTLD_NEXT` is particularly useful for wrapper functions and symbol interposition. For example, a custom `malloc()` wrapper can obtain the address of the real `malloc()` by calling:

```c
dlsym(RTLD_NEXT, "malloc");
```

The definitions of these pseudohandles require `_GNU_SOURCE`.

## 5) Accessing symbols in the main program

A dynamically loaded library may need to call a function defined in the main executable. In that case, the main program's global symbols must be made available to the dynamic linker.

This is done by linking the main program with:

```bash
gcc -Wl,--export-dynamic ...
```

Equivalent forms include:

- `-export-dynamic`
- `-rdynamic`
- `-Wl,-E`

This is a key mechanism for host applications that expose callback-style interfaces to dynamically loaded plug-ins.

## 6) Controlling symbol visibility

A well-designed shared library should export only the symbols that are part of its intended ABI.

Why this matters:

- applications may accidentally rely on internal interfaces;
- exported symbols can interpose with symbols in other libraries at run time;
- unnecessary exported symbols enlarge the dynamic symbol table.

Main techniques:

- `static`: keep a symbol private to one source file;
- `__attribute__((visibility("hidden")))`: keep a symbol usable across source files inside the library while hiding it from external users;
- version scripts: precisely define which symbols are global and which are local.

The `static` keyword and hidden visibility also prevent run-time interposition for those symbols.

## 7) Linker version scripts

A version script is a text file read by the linker:

```bash
gcc -Wl,--version-script,myscript.map ...
```

One use of version scripts is visibility control. A typical pattern is:

```text
VER_1 {
    global:
        public_api_1;
        public_api_2;
    local:
        *;
};
```

Meaning:

- explicitly listed symbols are exported;
- everything else is hidden.

This is one of the strongest ways to keep a shared-library ABI clean and intentional.

## 8) Symbol versioning

Symbol versioning allows a single shared library to provide multiple versions of the same symbol. Each executable continues to use the symbol version that was current when that executable was linked.

This makes it possible to introduce an incompatible new implementation while preserving compatibility for existing binaries.

Key pieces:

- a version script containing version nodes and dependencies;
- `.symver` directives that associate implementations with symbol versions;
- one default symbol version marked using `@@`.

Conceptually:

- old programs may bind to `xyz@VER_1`;
- new programs may bind to `xyz@@VER_2`.

This technique is powerful enough that, in extreme cases, it can replace the traditional "new major SONAME for every incompatible change" model. TLPI notes that glibc uses symbol versioning extensively within `libc.so.6`.

## 9) Initialization and finalization functions

Shared libraries can automatically execute code when they are loaded and unloaded.

Modern mechanism:

```c
void __attribute__((constructor)) lib_init(void) { /* ... */ }
void __attribute__((destructor)) lib_fini(void) { /* ... */ }
```

These functions run whether the library is:

- loaded automatically at program start;
- or loaded explicitly via `dlopen()`.

Older mechanism:

- `_init()`
- `_fini()`

TLPI treats `_init()` and `_fini()` as obsolete and recommends constructor/destructor attributes instead.

## Execution Flow

### 1) Run-time loading flow

Typical `dlopen()` flow:

1. decide which library to load;
2. call `dlopen()` with an appropriate binding mode such as `RTLD_NOW` or `RTLD_LAZY`;
3. if successful, obtain symbol addresses using `dlsym()`;
4. call functions or read variables through those addresses;
5. call `dlclose()` when the library is no longer needed.

### 2) Correct error-handling flow for `dlsym()`

The safe flow is:

1. clear old state with `dlerror()`;
2. call `dlsym()`;
3. fetch error text with `dlerror()`;
4. only use the returned address if the second `dlerror()` reports no error.

### 3) Library-host callback flow

When a dynamically loaded library needs symbols from the main executable:

1. link the executable with `--export-dynamic`;
2. `dlopen()` the library;
3. let the library resolve the callback symbol at run time.

### 4) Visibility-management flow

For a stable shared library:

1. decide which symbols are part of the public ABI;
2. keep internal helpers hidden using `static`, hidden visibility, or a version script;
3. inspect exported symbols using tools such as `readelf`;
4. keep future compatibility manageable by not leaking internal interfaces.

### 5) Interposition and debugging flow

When investigating dynamic-linking behavior:

1. use `LD_PRELOAD` to override selected symbols for testing or wrapping;
2. use `LD_DEBUG` with options such as `libs`, `bindings`, or `versions`;
3. direct output to a file with `LD_DEBUG_OUTPUT` when needed.

## Example

## 1) Basic `dlopen()` / `dlsym()` / `dlclose()`

```c
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    void *libHandle;
    void (*funcp)(void);
    const char *err;

    libHandle = dlopen("./libdemo.so", RTLD_LAZY);
    if (libHandle == NULL) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return 1;
    }

    (void) dlerror();  /* Clear any old error */
    *(void **) (&funcp) = dlsym(libHandle, "x1");
    err = dlerror();
    if (err != NULL) {
        fprintf(stderr, "dlsym: %s\n", err);
        dlclose(libHandle);
        return 1;
    }

    if (funcp != NULL)
        (*funcp)();

    dlclose(libHandle);
    return 0;
}
```

Build:

```bash
gcc -g -Wall -o dynload dynload.c -ldl
```

## 2) Wrapper function using `RTLD_NEXT`

```c
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

void *malloc(size_t size) {
    static void *(*real_malloc)(size_t) = NULL;

    if (real_malloc == NULL) {
        *(void **) (&real_malloc) = dlsym(RTLD_NEXT, "malloc");
        if (real_malloc == NULL) {
            fprintf(stderr, "dlsym failed\n");
            exit(1);
        }
    }

    fprintf(stderr, "malloc(%zu)\n", size);
    return real_malloc(size);
}
```

Build and use:

```bash
gcc -g -fPIC -shared -o libwrap.so wrap_malloc.c -ldl
LD_PRELOAD=./libwrap.so ./target_program
```

## 3) Constructor and destructor functions

```c
#include <stdio.h>

void __attribute__((constructor)) lib_init(void) {
    printf("library loaded\n");
}

void __attribute__((destructor)) lib_fini(void) {
    printf("library unloaded\n");
}

void hello(void) {
    printf("hello from library\n");
}
```

Build:

```bash
gcc -g -fPIC -shared -o libhello.so hello.c
```

If a program links this library or loads it via `dlopen()`, the initialization and finalization hooks run automatically.

## Debugging

Common debugging tools and patterns:

- use `dlerror()` immediately after a failed `dlopen()` or `dlsym()`;
- use `LD_DEBUG=libs` to trace library-search behavior;
- use `LD_DEBUG=bindings` or `LD_DEBUG=symbols` to inspect symbol binding;
- use `LD_DEBUG=versions` to inspect version dependencies;
- use `LD_DEBUG_OUTPUT=path` to redirect the tracing output to a file;
- use `readelf --syms --use-dynamic lib.so` to inspect exported symbols;
- use `objdump -t executable | grep symbol` or `readelf -s` to inspect versioned symbol references.

Typical pitfalls:

- forgetting to link with `-ldl`;
- assuming `dlclose()` always unloads a library immediately;
- mishandling `NULL` from `dlsym()` without checking `dlerror()`;
- exporting too many symbols from a library;
- assuming `LD_PRELOAD` or `LD_DEBUG` will work for set-user-ID or set-group-ID executables;
- accidentally exposing internal ABI because no version script or visibility policy was applied.

## Real-world Usage

These mechanisms appear in many practical designs:

- plug-in architectures that load optional modules at run time;
- host applications that expose callback hooks to dynamically loaded modules;
- test and instrumentation libraries that override selected functions via `LD_PRELOAD`;
- long-lived libraries that need to preserve backward compatibility through symbol versioning;
- deployment and debugging workflows that require tracing the dynamic linker.

Even when a project does not explicitly use `dlopen()`, understanding these mechanisms helps explain surprising symbol-binding behavior in complex systems.

## Key Takeaways

- `dlopen()` and related APIs allow libraries to be loaded explicitly at run time instead of only at process start.
- `RTLD_LAZY`, `RTLD_NOW`, `RTLD_LOCAL`, and `RTLD_GLOBAL` strongly influence symbol resolution and failure timing.
- `dlsym()` must be paired with `dlerror()` correctly, because a valid symbol value may be `NULL`.
- Shared libraries should export only their intended ABI; version scripts are a precise way to enforce that rule.
- Symbol versioning allows one library to support old and new binaries with different versions of the same symbol.
- Constructor and destructor functions automate library setup and teardown.
- `LD_PRELOAD` is powerful for testing and interposition, while `LD_DEBUG` is powerful for understanding dynamic-linker behavior.

