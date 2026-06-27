# 15 - C Vs C++ Comparison Examples

These examples are intentionally small and compile-oriented. They show how the
same engineering problem changes when written in C style, modern C++ style, and
at a C/C++ ABI boundary.

## Files

| File | Purpose | Status |
| --- | --- | --- |
| `c_manual_array.c` | C dynamic array with explicit allocation, size, callback, and cleanup | Learning-only |
| `cpp_modern_replacements.cpp` | C++ replacements using `std::vector`, `std::string_view`, `std::variant`, lambdas, and RAII | Mostly production-style, still simplified |
| `c_api_boundary.cpp` | C ABI wrapper around C++ internals with opaque handle and exception translation | Production-style boundary shape, simplified implementation |
| `Makefile` | Build, run, warning, and sanitizer commands | Practical |

## Build

From this directory:

```sh
make
```

Build one example:

```sh
make c_manual_array
make cpp_modern_replacements
make c_api_boundary
```

## Run

```sh
make run
```

Or run one binary:

```sh
./c_manual_array
./cpp_modern_replacements
./c_api_boundary
```

## Sanitizer / Debug Commands

Use AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
make sanitize
```

Use warnings as errors:

```sh
make strict
```

Clean generated binaries:

```sh
make clean
```

## What To Notice

### C Manual Array

`c_manual_array.c` is learning-only because it uses manual ownership:

- allocation is explicit with `malloc`;
- cleanup is explicit with `free`;
- size travels separately from the pointer;
- callbacks use a function pointer plus `void* user_data`;
- every failure path must preserve cleanup.

This style is appropriate for C code, C ABI boundaries, or vendor APIs, but it
is easy to break if ownership is not documented.

Warnings:

- Do not pair `malloc` with `delete`.
- Do not pair `new` with `free`.
- Do not read past `size`.
- Do not store a callback context pointer that can dangle.

### C++ Modern Replacements

`cpp_modern_replacements.cpp` shows C++ mechanisms that encode intent:

- `std::vector<int>` owns a dynamic array and tracks size;
- `std::string_view` is a non-owning read-only view;
- `std::variant` replaces a manual tagged union for type-safe alternatives;
- a lambda replaces a simple callback;
- `std::unique_ptr<FILE, decltype(&std::fclose)>` gives a C resource RAII.

Warnings:

- `std::string_view` does not own data. Do not store it unless the referenced
  storage is guaranteed to outlive the view.
- `std::vector` reallocation can invalidate pointers, references, and iterators.
- `std::variant` is safer than a union, but `std::get<T>` can still throw if the
  active alternative is different.

### C API Boundary

`c_api_boundary.cpp` exposes C-shaped functions around C++ internals:

- `extern "C"` prevents C++ name mangling on exported functions;
- `EngineHandle` is opaque to C callers;
- create/destroy functions define ownership;
- exceptions are caught and translated to integer status codes.

Warnings:

- Never let a C++ exception cross a C ABI boundary.
- Do not expose `std::string`, `std::vector`, references, templates, or C++
  classes directly in a C ABI.
- Document who allocates and who releases every object and buffer.

## Practice Changes

1. Add a bounds-checked `array_get` function to `c_manual_array.c`.
2. Replace a `std::function` callback with a function template and compare the
   call-site syntax.
3. Modify `c_api_boundary.cpp` so `engine_run` returns an error for a null input
   string.
4. Intentionally store a `std::string_view` to a temporary string, run the
   sanitizer build, then fix it by storing `std::string`.

