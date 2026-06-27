# 12 - Modern C++ And Templates Examples

## Status

These are learning-focused C++20 examples for `12-modern-cpp-and-templates`. They are intentionally small and compile-oriented, not production-ready libraries.

Production code should add domain-specific error handling, tests, performance measurements, ABI/build-policy decisions, and concurrency rules where needed.

## Requirements

- A C++20 compiler such as `g++` or `clang++`
- GNU Make
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
make CXX=clang++ check
```

## Example Map

| Target | Focus | Learning-only notes |
| --- | --- | --- |
| `move_logging` | move construction, moved-from objects, `noexcept` moves | Logs behavior instead of benchmarking real workload |
| `lambda_lifetime` | value capture, move-only capture, non-capturing lambda conversion | Avoids executing dangling reference captures |
| `ownership_callbacks` | `shared_ptr`, `weak_ptr`, callback lifetime | Single-threaded only; no event-loop synchronization |
| `vocabulary_types` | `optional`, `variant`, `visit`, `string_view`, `span`, `from_chars` | Uses simple parsing and short-lived views |
| `templates_concepts` | `constexpr`, class/function templates, fold expression, concept constraint | Tiny template examples, not a full container library |

## Move Logging

Run:

```sh
make build/move_logging
./build/move_logging
```

Expected shape:

```text
copies=0 moves=2 reused=true result=passed
```

The exact move count can vary as the example changes, but copies should stay at zero with the current `noexcept` move operations.

Production direction:

- Make move operations `noexcept` when the type can truly honor that guarantee.
- Treat moved-from objects as valid but unspecified; assign a new value or destroy them.
- Do not write `return std::move(local)` for ordinary local returns; let copy elision and move rules work.

Relevant risks:

- A move operation marked `noexcept` must not throw.
- A moved-from object is not a place to read business meaning from.

## Lambda Lifetime

Run:

```sh
make build/lambda_lifetime
./build/lambda_lifetime
```

Expected shape:

```text
counter=2 moved_capture=42 function_pointer=12 result=passed
```

Production direction:

- Capture by value when a lambda escapes the current scope.
- Capture move-only resources explicitly with init capture, such as `[ptr = std::move(ptr)]`.
- Use non-capturing lambdas when a C-style function pointer API requires `T (*)(...)`.

Relevant risks:

- Returning or storing a lambda that captured a local variable by reference can produce undefined behavior.
- Capturing `this` into asynchronous work can dangle unless object lifetime is guaranteed.
- `std::function` is convenient but may allocate and erase type information.

## Ownership Callbacks

Run:

```sh
make build/ownership_callbacks
./build/ownership_callbacks
```

Expected shape:

```text
total_before_destroy=5 expired=true result=passed
```

Production direction:

- Use `weak_ptr` in callbacks when the callback must not extend object lifetime.
- Use `shared_from_this()` only after the object is already owned by a `shared_ptr`.
- Document whether a callback API stores, copies, moves, or invokes callbacks concurrently.

Relevant risks:

- Capturing `shared_ptr` from object to callback and callback back to object can create ownership cycles.
- Keeping a raw pointer from `get()` after the owner resets can dangle.
- This example is single-threaded; it does not prove race, deadlock, or callback ordering behavior.

## Vocabulary Types

Run:

```sh
make build/vocabulary_types
./build/vocabulary_types
```

Expected shape:

```text
parsed=42 rejected=true sum=6 event=fault:E42 result=passed
```

Production direction:

- Use `optional<T>` for maybe-present values, not for hidden error diagnostics.
- Use `variant` plus `visit` when the state is exactly one of several known alternatives.
- Use `string_view` and `span` as non-owning parameters, not as default storage.
- Use `from_chars` for low-level parsing when exceptions and locale behavior are unwanted.

Relevant risks:

- `optional::value()` throws on empty optionals; check or use `value_or` when appropriate.
- `std::get<T>` throws or fails to compile when the active `variant` alternative is not what the code expects.
- A `string_view` can dangle after the original `std::string` or buffer is destroyed.
- A `span` can dangle or become invalid after the original container reallocates.

## Templates Concepts

Run:

```sh
make build/templates_concepts
./build/templates_concepts
```

Expected shape:

```text
buffer_size=3 fold_sum=6 range_size=4 result=passed
```

Production direction:

- Put template definitions in headers unless using explicit instantiation deliberately.
- Prefer `constexpr` and typed constants over preprocessor macros.
- Use concepts to express requirements close to the template interface.
- Watch generated code size and compile time for heavily instantiated templates.

Relevant risks:

- Function templates cannot be partially specialized; use overloads, class-template specialization, or concepts.
- Dependent names may need `typename` or `template` disambiguators.
- Concepts improve diagnostics, but they do not replace tests for runtime semantics.
- Iterator invalidation rules still apply to templated code that mutates containers.

## Debugging

Build debug binaries:

```sh
make debug
```

Example GDB session:

```sh
gdb ./build/vocabulary_types
(gdb) break parse_positive
(gdb) run
(gdb) print text
(gdb) next
```

For template diagnostics, reduce the failing call to the smallest instantiation and inspect the first meaningful compiler error. Later template errors are often consequences of the first bad requirement.

## Sanitizer Experiments

Run the clean suite under sanitizers:

```sh
make sanitize
```

The checked examples should pass without sanitizer findings. To study failures, introduce one defect at a time in a local experiment:

- Return a `std::string_view` to a local `std::string`.
- Store a lambda that captures a local variable by reference, then call it after the variable is gone.
- Replace the `weak_ptr` callback with a `shared_ptr` capture and inspect leak behavior.
- Call `optional::value()` after parsing invalid input.
- Keep a raw pointer from `unique_ptr::get()` or `shared_ptr::get()` after the owner releases or resets.

AddressSanitizer and UndefinedBehaviorSanitizer only catch defects on executed paths. For future threaded examples, use ThreadSanitizer and design explicit lock-order rules before treating race or deadlock behavior as validated.
