# Chapter 10.2 - Advanced Shared Libraries

> Topics: `dlopen()`, `dlsym()`, `dlclose()`, `dlerror()`, symbol visibility, version scripts, symbol versioning, constructors/destructors, `LD_PRELOAD`, `LD_DEBUG`.
> Main sources: TLPI Ch42; TLPI Ch41 for runtime linker search and symbol-resolution background.
> Production context: used in plugin systems, native module hosts, observability wrappers, ABI-controlled SDKs, runtime instrumentation, and debugging complex C/C++ deployments.

---

## Problem It Solves

Basic shared libraries are loaded automatically when a process starts. Real systems often need more control:

- load a plugin only when a feature is enabled;
- let a host application expose callbacks to loaded modules;
- hide internal helper symbols so applications cannot depend on them;
- keep old binaries working after library internals change;
- override selected functions for testing, tracing, or compatibility;
- explain why a symbol came from one library instead of another.

Advanced shared-library mechanisms are the toolbox for those cases. They are powerful, but they also create production bugs when symbol visibility, load order, or error handling is sloppy.

## Learning Roadmap

| Level | Learn | Goal |
|-------|-------|------|
| Must know | `dlopen()`, `dlsym()`, `dlclose()`, `dlerror()`, `RTLD_NOW`, `RTLD_LAZY`, `-ldl` | Build and debug a simple plugin loader |
| Work useful | `RTLD_LOCAL`, `RTLD_GLOBAL`, `RTLD_NEXT`, `--export-dynamic`, symbol visibility, version scripts, `LD_DEBUG` | Control ABI and debug symbol binding in production |
| Recognize | symbol versioning, constructors/destructors, `RTLD_NODELETE`, `RTLD_DEEPBIND`, `LD_PRELOAD` system-wide behavior | Know what these tools do and when to investigate them |

## Core Vocabulary

| Term | Meaning | Example / note |
|------|---------|----------------|
| Dynamic loading | Loading a shared library after process startup | Plugin loaded with `dlopen()` |
| Library handle | Opaque value returned by `dlopen()` and passed to `dlsym()` | `void *handle` |
| Dependency tree | Libraries required by a loaded library, loaded recursively | Plugin depends on `libssl.so` |
| Reference count | Count of opens for a loaded library handle | Library may stay loaded until all references close |
| `RTLD_LAZY` | Resolve function symbols when first used | Faster open, later failure possible |
| `RTLD_NOW` | Resolve all undefined symbols before `dlopen()` returns | Better fail-fast behavior |
| `RTLD_LOCAL` | Keep symbols unavailable to later-loaded libraries by default | Linux default |
| `RTLD_GLOBAL` | Make symbols available to later-loaded libraries | Useful for plugin dependency chains |
| `dlerror()` | Returns and clears dynamic-loader error state | Must be used around `dlsym()` |
| Pseudohandle | Special handle for broader symbol search | `RTLD_DEFAULT`, `RTLD_NEXT` |
| Symbol visibility | Which symbols are exported outside a shared object | `static`, hidden attribute, version script |
| Version script | Linker script controlling exported symbols and versions | `-Wl,--version-script,api.map` |
| Symbol versioning | Multiple ABI versions of a symbol in one `.so` | `foo@LIB_1`, `foo@@LIB_2` |
| Constructor | Function run when a library is loaded | GCC `constructor` attribute |
| Destructor | Function run when a library is unloaded | GCC `destructor` attribute |
| Interposition | Replacing a symbol by providing an earlier definition | `LD_PRELOAD` wrapper |
| `LD_DEBUG` | Dynamic linker tracing environment variable | `LD_DEBUG=libs,bindings ./app` |

## Concept Overview

The core mental model is that the dynamic linker is not only a startup component. Programs can call into it explicitly:

```text
host process
    |
    v
dlopen("plugin.so")
    |
    v
dynamic linker maps plugin and dependencies
    |
    v
dlsym(handle, "entry_point")
    |
    v
host calls function pointer
```

Advanced shared-library work has two big themes:

- runtime control: when a library is loaded, which symbols are searched, and when failures happen;
- ABI control: which symbols are exported and how compatibility is preserved.

Newbie trap: `dlsym()` returning `NULL` does not always mean failure. A valid symbol may have a null value, so the error state from `dlerror()` must be checked.

## System Context

| Layer | Role |
|-------|------|
| Host executable | May load plugins and optionally export callbacks |
| Dynamic linker | Maps requested libraries, resolves symbols, tracks references |
| Shared object | Provides exported symbols and may have dependencies |
| `libdl` | User-space API for dynamic loading on Linux |
| ELF symbol table | Controls visible dynamic symbols |
| Environment | `LD_PRELOAD`, `LD_DEBUG`, `LD_BIND_NOW` can alter or trace loading |
| Build system | Uses `-fPIC`, `-shared`, `-ldl`, `--export-dynamic`, version scripts |

These mechanisms interact with chapter 10.1 runtime search rules, process startup, and security rules for privileged executables. For security, variables such as `LD_PRELOAD` and `LD_DEBUG` are ignored for set-user-ID and set-group-ID programs.

## Architecture

```text
main executable
    |
    | dlopen()
    v
plugin.so
    |
    | DT_NEEDED
    v
dependency libraries

dynamic linker tracks:
    loaded objects
    reference counts
    symbol visibility
    relocation and binding state
```

Important design boundary:

- a plugin ABI should be small, stable, and explicit;
- internal helper symbols should not be exported accidentally;
- error reporting should happen at load/lookup boundaries, not much later in unrelated code.

## Execution Flow

### Flow 1: Plugin Load

```text
choose plugin path
    |
    v
dlopen(path, RTLD_NOW | RTLD_LOCAL)
    |
    v
resolve entry point with dlsym()
    |
    v
validate function table or ABI version
    |
    v
call plugin
    |
    v
dlclose() during shutdown
```

### Flow 2: Correct `dlsym()` Error Handling

```text
dlerror()          # clear old error
    |
    v
dlsym()
    |
    v
dlerror()          # check lookup result
    |
    v
use pointer only if error == NULL
```

### Flow 3: Host Callback Export

```text
link host with --export-dynamic
    |
    v
host dlopen() plugin
    |
    v
plugin resolves host callback symbol
    |
    v
plugin calls back into host
```

### Flow 4: Visibility Control

```text
decide public ABI
    |
    v
mark helpers static/hidden
    |
    v
apply version script with local: *
    |
    v
inspect exports with readelf -Ws --dyn-syms
```

### Flow 5: Interposition Debugging

```text
symbol behaves unexpectedly
    |
    v
LD_DEBUG=bindings,symbols
    |
    v
inspect lookup order and binding target
    |
    v
fix visibility, link order, or preload wrapper
```

## 10.2 API / Topic Sections

### 10.2.1 `dlopen()`

Signature:

```c
#include <dlfcn.h>

void *dlopen(const char *filename, int flags);
```

Use it to load a shared library into the calling process. If `filename` contains `/`, it is treated as a path. Otherwise, the dynamic linker uses normal library search rules.

Choose binding behavior:

- `RTLD_NOW`: fail early if required symbols cannot be resolved;
- `RTLD_LAZY`: defer function-symbol resolution until call time.

Production default: prefer `RTLD_NOW` for plugins unless startup cost is a measured problem. Fail-fast bugs are easier to diagnose than late unresolved-symbol crashes.

### 10.2.2 `dlsym()` and `dlerror()`

Signature:

```c
void *dlsym(void *handle, const char *symbol);
char *dlerror(void);
```

Use `dlsym()` to find a function or variable by name. Always use the `dlerror()` pattern because `NULL` can be a valid symbol value.

For function pointers, TLPI uses:

```c
*(void **) (&funcp) = dlsym(handle, "symbol_name");
```

This avoids relying on a direct `void *` to function-pointer assignment.

### 10.2.3 `dlclose()` and Unload Semantics

Signature:

```c
int dlclose(void *handle);
```

`dlclose()` decrements the reference count. The library is unloaded only when the count reaches zero and no other loaded object needs it. Process exit implicitly closes loaded libraries.

Pitfall: do not assume a destructor runs immediately after every `dlclose()`. Other references or flags such as `RTLD_NODELETE` can keep code and state resident.

### 10.2.4 `RTLD_LOCAL`, `RTLD_GLOBAL`, and Pseudohandles

Use `RTLD_LOCAL` to keep plugin symbols private unless there is a strong reason to expose them. Use `RTLD_GLOBAL` when later-loaded libraries must resolve against symbols from this library.

Pseudohandles:

- `RTLD_DEFAULT`: search the default global symbol scope;
- `RTLD_NEXT`: find the next definition after the current object, often for wrappers.

`RTLD_DEFAULT` and `RTLD_NEXT` require `_GNU_SOURCE` on glibc.

### 10.2.5 Exporting Host Symbols

If a plugin must call symbols defined by the main executable, link the host with:

```bash
gcc -Wl,--export-dynamic ...
```

Equivalent forms include `-rdynamic`, `-export-dynamic`, and `-Wl,-E`.

Use this intentionally. Exporting every global symbol from the host can make symbol collisions harder to reason about.

### 10.2.6 Symbol Visibility

A shared library should export only its documented ABI.

Good options:

- use `static` for file-local helpers;
- use `__attribute__((visibility("hidden")))` for internal cross-file helpers;
- use a version script to declare public symbols and hide everything else.

Why it matters:

- accidental exports become compatibility promises;
- exported symbols can interpose with other libraries;
- large dynamic symbol tables slow and complicate runtime binding.

### 10.2.7 Version Scripts and Symbol Versioning

A visibility-focused version script:

```text
LIBDEMO_1 {
    global:
        demo_init;
        demo_run;
    local:
        *;
};
```

Build with:

```bash
gcc -shared -Wl,--version-script,libdemo.map -o libdemo.so *.o
```

Symbol versioning goes further: one library can provide old and new versions of the same symbol so old binaries bind to the old ABI and new binaries bind to the new ABI. This is powerful but advanced. Recognize it when debugging libraries such as glibc.

### 10.2.8 Constructors and Destructors

Use GCC attributes for load/unload hooks:

```c
void __attribute__((constructor)) lib_init(void) { }
void __attribute__((destructor)) lib_fini(void) { }
```

They run when the library is loaded or unloaded, whether loading happens automatically or through `dlopen()`.

Avoid heavy logic, uncontrolled thread creation, or surprising side effects in constructors. They run at awkward lifecycle points.

### 10.2.9 `LD_PRELOAD` and `LD_DEBUG`

`LD_PRELOAD` loads specified libraries before normal dependencies, allowing their symbols to interpose first. It is useful for tests and instrumentation.

`LD_DEBUG` traces dynamic-linker behavior:

```bash
LD_DEBUG=libs ./app
LD_DEBUG=bindings,symbols ./app
LD_DEBUG=versions ./app
```

Security note: these environment variables are ignored for set-user-ID and set-group-ID programs.

## Work-Useful Patterns

| Pattern | Why it helps |
|---------|--------------|
| Use `RTLD_NOW | RTLD_LOCAL` for plugins by default | Fail early and avoid leaking plugin symbols globally |
| Define a small plugin ABI | Reduces long-term compatibility burden |
| Validate plugin ABI version after `dlsym()` | Prevents loading incompatible modules |
| Hide all non-public symbols with `local: *` | Keeps ABI clean |
| Use `LD_DEBUG=libs,bindings` for runtime mysteries | Shows real loader behavior instead of assumptions |
| Keep constructors/destructors simple | Avoids hidden startup/shutdown bugs |

## Advanced / Recognize First

| Topic | Know this much |
|-------|----------------|
| `RTLD_NODELETE` | Keeps a library loaded after `dlclose()`, preserving static state |
| `RTLD_NOLOAD` | Checks or promotes flags for an already loaded library |
| `RTLD_DEEPBIND` | Linux/glibc-specific; prefers the library's own symbols for its references |
| `dladdr()` | Maps an address back to library/symbol information |
| `dlvsym()` | Looks up a specific versioned symbol; GNU extension |
| `.symver` | Assembler directive used in symbol versioning |
| `/etc/ld.so.preload` | System-wide preload file; high blast radius |

## Example

### Example 1: Minimal Plugin Loader

`plugin.c`:

```c
#include <stdio.h>

void plugin_run(void) {
    puts("plugin running");
}
```

`host.c`:

```c
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    void *handle;
    void (*plugin_run)(void);
    const char *err;

    handle = dlopen("./plugin.so", RTLD_NOW | RTLD_LOCAL);
    if (handle == NULL) {
        fprintf(stderr, "dlopen: %s\n", dlerror());
        return EXIT_FAILURE;
    }

    (void) dlerror();
    *(void **) (&plugin_run) = dlsym(handle, "plugin_run");
    err = dlerror();
    if (err != NULL) {
        fprintf(stderr, "dlsym: %s\n", err);
        dlclose(handle);
        return EXIT_FAILURE;
    }

    plugin_run();
    dlclose(handle);
    return EXIT_SUCCESS;
}
```

Build:

```bash
gcc -Wall -Wextra -g -fPIC -shared -o plugin.so plugin.c
gcc -Wall -Wextra -g -o host host.c -ldl
./host
```

What it teaches:

- the plugin is not a link-time dependency of the host;
- `RTLD_NOW` reports missing symbols during load;
- `dlsym()` must be paired with `dlerror()`.

### Example 2: Hide Internal Symbols with a Version Script

`api.map`:

```text
PLUGIN_1 {
    global:
        plugin_run;
    local:
        *;
};
```

Build:

```bash
gcc -Wall -Wextra -g -fPIC -c plugin.c
gcc -shared -Wl,--version-script,api.map -o plugin.so plugin.o
readelf --dyn-syms plugin.so
```

What it teaches:

- public ABI is explicit;
- internal symbols do not accidentally become exported.

## Debugging

Useful commands:

```bash
LD_DEBUG=libs ./host
LD_DEBUG=bindings,symbols ./host
LD_DEBUG=versions ./host
LD_DEBUG_OUTPUT=/tmp/lddebug ./host
readelf --dyn-syms ./plugin.so
objdump -T ./plugin.so
ldd ./plugin.so
```

Common bugs:

| Bug | Symptom | Fix / check |
|-----|---------|-------------|
| Forgot `-ldl` | Link fails with unresolved `dlopen`/`dlsym` | Link host with `-ldl` on Linux |
| Wrong path passed to `dlopen()` | `dlopen` cannot find plugin | Use explicit path or configure loader search path |
| Used `RTLD_LAZY` and symbol missing | Failure occurs only when function is first called | Use `RTLD_NOW` during development and for plugins |
| Mishandled `dlsym()` `NULL` | False failure or false success | Clear and check `dlerror()` |
| Plugin cannot see host callback | `undefined symbol` inside plugin | Link host with `--export-dynamic` or pass function table explicitly |
| Too many exported symbols | ABI leaks and symbol collisions | Use `static`, hidden visibility, or version script |
| `LD_PRELOAD` ignored | Wrapper works for normal program but not set-user-ID binary | Expected security behavior |

## Real-world Usage

| Scenario | Practical design |
|----------|------------------|
| Plugin host | Load modules with `dlopen()`, resolve a versioned entry point, pass a function table |
| Observability wrapper | Use `LD_PRELOAD` and `RTLD_NEXT` to intercept selected libc calls |
| ABI-stable SDK | Hide internals and export only documented symbols |
| Production linker mystery | Use `LD_DEBUG=libs,bindings` and inspect `readelf` output |
| Long-lived system library | Use soname discipline; consider symbol versioning only when needed |

## Interview-Relevant Questions

1. What problem does `dlopen()` solve compared with normal shared-library loading?
2. What is the difference between `RTLD_LAZY` and `RTLD_NOW`?
3. Why is `RTLD_NOW` often better for production plugin loading?
4. What does `RTLD_LOCAL` do, and why is it usually safer than `RTLD_GLOBAL`?
5. How should `dlsym()` errors be checked correctly?
6. Why is assigning `dlsym()` directly to a function pointer technically tricky in C?
7. What does `dlclose()` actually guarantee?
8. What are `RTLD_DEFAULT` and `RTLD_NEXT` used for?
9. How can a dynamically loaded library call a function in the main executable?
10. Why should shared libraries hide internal symbols?
11. How does a linker version script help maintain ABI hygiene?
12. What is symbol versioning, and why does glibc use it?
13. When do constructor and destructor functions run?
14. What is `LD_PRELOAD`, and when is it useful?
15. Why are `LD_PRELOAD` and `LD_DEBUG` restricted for set-user-ID programs?
16. How would you debug a plugin that loads but fails on first use?
17. How would you inspect which symbols a `.so` exports?
18. What production risks come from exporting too many symbols?

## Key Takeaways

- `dlopen()` turns the dynamic linker into an explicit runtime API.
- Use `RTLD_NOW | RTLD_LOCAL` unless you have a clear reason not to.
- `dlsym()` requires the `dlerror()` pattern because `NULL` alone is ambiguous.
- A stable library exports a small intentional ABI, not every global helper.
- `LD_PRELOAD` is powerful for interposition; `LD_DEBUG` is powerful for understanding real linker behavior.
- Symbol versioning is important to recognize, but version scripts for visibility are the work-useful first skill.
