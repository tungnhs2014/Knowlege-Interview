# 15 - C Vs C++ Comparison

## 1. Goal

After this lesson, you should be able to compare C and C++ mechanisms in a way
that is useful for real engineering work:

- choose between C-style and C++-style data modeling;
- understand manual memory management vs RAII;
- compare C arrays/strings with `std::array`, `std::vector`, and `std::string`;
- choose function pointers, lambdas, templates, or `std::function` for
  callbacks;
- replace dangerous macros with inline functions, `constexpr`, or templates;
- understand C-style OOP patterns and C++ polymorphism;
- choose return codes, exceptions, or Result/`std::expected`-style APIs;
- design C/C++ boundaries without leaking ownership, exceptions, or ABI
  assumptions.

This topic is not about proving that C is bad and C++ is good. The real goal is
to know which tool makes ownership, lifetime, invariants, error handling, and
interface boundaries clearest.

## 2. Why It Matters

Many production codebases are mixed:

- C libraries wrapped by C++ services;
- embedded projects using C vendor SDKs with C++ application logic;
- legacy C modules slowly migrated to C++;
- plugin APIs that must keep a stable C ABI;
- performance-sensitive code that needs explicit control but still benefits
  from C++ RAII and type safety.

C gives you direct control. C++ gives you stronger ways to express intent. Bugs
often appear when the two models are mixed without a policy:

- `malloc` memory is released with `delete`;
- a C callback stores a dangling C++ lambda capture;
- a C string is treated as if it had a known size;
- a C API receives a pointer but nobody knows who owns it;
- a C++ exception crosses a C ABI boundary.

Good engineers do not answer "always use C++" or "C is faster". They ask:

> What is the ownership model, lifetime, ABI boundary, failure mode, and
> performance constraint?

## 3. Mental Model

### C Mental Model

C is procedural and explicit:

- data is usually modeled with `struct`, `union`, arrays, and pointers;
- behavior is usually separate functions;
- dynamic memory is manually allocated and freed;
- strings are null-terminated character sequences;
- callbacks are function pointers, often with a `void* user_data` context;
- error handling is usually return codes, `errno`, or output parameters;
- generic code is often written with macros, `void*`, or code generation.

C gives precise control, but the compiler cannot see much intent. A pointer can
mean "owning", "borrowing", "optional", "array", "single object", "C string",
or "opaque handle" unless the interface documents it.

### C++ Mental Model

C++ can still do low-level programming, but it adds stronger abstractions:

- constructors/destructors establish object lifetime;
- classes bind data and behavior;
- references express non-null aliasing;
- RAII ties resource cleanup to scope;
- standard containers own arrays and track size;
- `std::string` owns text;
- smart pointers express ownership;
- lambdas and templates express callable behavior;
- exceptions or Result-style types express failure;
- `enum class`, `std::variant`, and named casts improve type safety.

C++ is not automatically safe. It becomes safer when you use the facilities that
encode intent.

### Boundary Rule

Use C style deliberately at boundaries:

- C ABI;
- vendor SDK;
- embedded C module;
- plugin interface;
- freestanding or low-level environment;
- exact layout or binary compatibility requirement.

Use C++ style internally when it improves invariants:

- RAII for ownership;
- `std::vector` for dynamic arrays;
- `std::string` for owned text;
- `std::array` for fixed-size arrays;
- `std::unique_ptr` for exclusive ownership;
- `enum class` for scoped states;
- `std::variant` for type-safe alternatives;
- lambdas/templates for callbacks.

## 4. Mechanism

### Data And Behavior

In C, data and behavior are usually separate:

```c
typedef struct {
    int id;
    float temperature;
} Sensor;

void sensor_print(const Sensor* sensor);
```

In C++, behavior can live with the data:

```cpp
struct Sensor {
    int id{};
    float temperature{};

    bool is_hot() const {
        return temperature > 80.0f;
    }
};
```

The C++ version can still be a simple passive data type. You do not need a class
hierarchy just because you are using C++.

### Lifetime And Cleanup

C manual cleanup:

```c
char* buffer = malloc(1024);
if (!buffer) {
    return -1;
}

/* work */

free(buffer);
```

C++ RAII cleanup:

```cpp
#include <vector>

std::vector<char> buffer(1024);
// cleanup happens automatically
```

RAII matters because cleanup happens on every exit path: normal return,
exception, or early return.

### Arrays And Strings

C arrays decay to pointers when passed to functions:

```c
void print_values(const int* values, int count);
```

C++ can preserve size in the type:

```cpp
#include <array>
#include <span>

void print_fixed(const std::array<int, 4>& values);
void print_view(std::span<const int> values); // C++20
```

C strings rely on `'\0'`:

```c
const char* name = "Alice";
```

C++ strings carry their size:

```cpp
#include <string>

std::string name = "Alice";
```

### Functions And Callbacks

C callback shape:

```c
typedef void (*callback_t)(int value, void* user_data);

void register_callback(callback_t callback, void* user_data);
```

C++ callback shape:

```cpp
#include <functional>

void register_callback(std::function<void(int)> callback);
```

For hot paths, a template callback can avoid `std::function` overhead:

```cpp
template <class Callback>
void for_each_value(Callback callback) {
    callback(1);
    callback(2);
}
```

### Errors

C error style:

```c
int read_sensor(int* out_value);
```

C++ exception style:

```cpp
int read_sensor(); // may throw
```

C++ explicit Result-style:

```cpp
struct ReadResult {
    bool ok;
    int value;
    int error_code;
};
```

Use the error style that matches the failure: expected local failures often fit
Result/return-code style; rare cross-layer failures may fit exceptions if the
project allows them.

## 5. C/C++ API And Code

### Example 1: Macro vs Inline / `constexpr`

C macro:

```c
#define SQUARE(x) ((x) * (x))
```

Bug:

```c
int i = 3;
int y = SQUARE(++i); /* increments more than once */
```

C++ replacement:

```cpp
#include <iostream>

constexpr int square(int x) {
    return x * x;
}

int main() {
    int i = 3;
    int y = square(++i);
    std::cout << "i=" << i << " y=" << y << "\n";
}
```

Compile:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic macro_replace.cpp -o macro_replace
```

### Example 2: C Dynamic Array vs `std::vector`

C style:

```c
#include <stdlib.h>

int* values = malloc(10 * sizeof *values);
if (!values) {
    return 1;
}

/* work */

free(values);
```

C++ style:

```cpp
#include <iostream>
#include <vector>

int main() {
    std::vector<int> values;
    values.reserve(10);

    for (int i = 0; i < 10; ++i) {
        values.push_back(i);
    }

    std::cout << values.size() << "\n";
}
```

Compile:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic vector_array.cpp -o vector_array
```

### Example 3: C Callback vs Lambda

C-compatible callback:

```cpp
#include <iostream>

using Callback = void (*)(int, const void*);

void run_c_style(Callback callback, const void* user_data) {
    callback(42, user_data);
}

void print_with_prefix(int value, const void* user_data) {
    const char* prefix = static_cast<const char*>(user_data);
    std::cout << prefix << value << "\n";
}

int main() {
    const char* prefix = "value=";
    run_c_style(print_with_prefix, prefix);
}
```

C++ lambda version:

```cpp
#include <iostream>

template <class Callback>
void run_cpp_style(Callback callback) {
    callback(42);
}

int main() {
    int offset = 10;
    run_cpp_style([offset](int value) {
        std::cout << value + offset << "\n";
    });
}
```

Compile:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic callback.cpp -o callback
```

Warning: do not capture by reference if the callback may outlive the referenced
object.

### Example 4: C Tagged Union vs `std::variant`

C style:

```c
typedef enum {
    VALUE_INT,
    VALUE_FLOAT
} ValueKind;

typedef struct {
    ValueKind kind;
    union {
        int i;
        float f;
    } data;
} Value;
```

C++ style:

```cpp
#include <iostream>
#include <variant>

using Value = std::variant<int, float>;

int main() {
    Value value = 3.5f;

    std::visit([](auto v) {
        std::cout << v << "\n";
    }, value);
}
```

Compile:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic variant_value.cpp -o variant_value
```

`std::variant` tracks the active alternative. A manual union requires you to
keep the tag and active member consistent yourself.

### Example 5: C Handle Wrapped With RAII

```cpp
#include <cstdio>
#include <memory>

using FilePtr = std::unique_ptr<FILE, decltype(&std::fclose)>;

FilePtr open_log(const char* path) {
    return FilePtr(std::fopen(path, "w"), &std::fclose);
}

int main() {
    auto file = open_log("app.log");
    if (!file) {
        return 1;
    }

    std::fputs("hello\n", file.get());
}
```

Compile:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic file_raii.cpp -o file_raii
```

The C API still uses `FILE*`, but the C++ wrapper owns cleanup.

## 6. Practical Usage

### When To Use C Style

Use C style when:

- the interface must be callable from C;
- ABI stability matters more than C++ expressiveness;
- you are working inside an existing C codebase;
- a vendor SDK or platform API requires C callbacks, C strings, or raw buffers;
- deterministic layout and minimal runtime dependencies are required;
- exceptions and RTTI are disabled by project policy.

### When To Use C++ Style

Use C++ style when:

- ownership should be automatic and exception-safe;
- an object has invariants that constructors/destructors can enforce;
- arrays need tracked size and safe growth;
- strings need ownership and length;
- callbacks need captures or object state;
- type safety matters more than raw ABI simplicity;
- generic code should be type-checked instead of macro-expanded.

### C Boundary Wrapper Pattern

At a C/C++ boundary, make ownership explicit:

```cpp
#include <stdexcept>

extern "C" int read_sensor_c(int* out_value);

int read_sensor_cpp() {
    int value = 0;
    int rc = read_sensor_c(&value);
    if (rc != 0) {
        throw std::runtime_error("read_sensor_c failed");
    }
    return value;
}
```

Do not let exceptions escape through a C ABI. Catch them at the boundary:

```cpp
extern "C" int api_entry() {
    try {
        // C++ implementation
        return 0;
    } catch (...) {
        return -1;
    }
}
```

## 7. Comparisons

### Data Modeling

| Topic | C | C++ | Enterprise / Embedded Guidance |
| --- | --- | --- | --- |
| `struct` in C vs `struct` in C++ | Plain aggregate data; behavior is usually separate functions | Can have constructors, member functions, access control, operators, and templates | Use C `struct` for ABI/plain data. Use C++ `struct` when behavior is simple and data remains mostly public |
| `struct` vs `class` in C++ | No direct C equivalent | Same feature set; `struct` defaults public, `class` defaults private | Use `struct` for passive value objects. Use `class` when invariants require encapsulation |
| `union` vs `std::variant` | Manual active-member discipline, often with a separate tag | `std::variant` tracks active alternative and supports visitation | Use union for ABI/layout constraints. Prefer `std::variant` for type-safe alternatives |
| `enum` vs `enum class` | Unscoped names and implicit integer conversion | Scoped, strongly typed enumerators | Prefer `enum class` in C++. Use C enum for C ABI headers |
| `typedef` vs `using` | Traditional alias syntax | Clearer alias syntax and alias templates | Use `typedef` in C headers. Prefer `using` in C++ |

Common bug: using a C-style union without keeping the tag synchronized with the
active member.

Interview question: Why is `enum class` usually safer than a C-style enum?

### Memory And Ownership

| Topic | C | C++ | Enterprise / Embedded Guidance |
| --- | --- | --- | --- |
| `malloc` vs `calloc` | `malloc` gives uninitialized bytes; `calloc` zero-initializes | C++ objects should be constructed, not merely zeroed | Use C allocation for C buffers/APIs. Do not use `calloc` to construct C++ objects |
| `malloc/calloc/realloc/free` vs `new/delete` | Allocates raw storage; no constructors/destructors | Allocates objects and runs constructors/destructors | Prefer RAII containers/smart pointers. Never mix allocation families |
| `free` vs `delete` | Releases memory from C allocation | Calls destructor then deallocates C++ object | Match exactly: `malloc/free`, `new/delete`, `new[]/delete[]` |
| `new[]/delete[]` vs `std::vector` | Manual dynamic array with manual cleanup | Dynamic array with size, capacity, destructor, iterators | Prefer `std::vector`; use `data()` for C interop |
| Manual cleanup vs RAII | Cleanup must be repeated on every path | Destructors clean automatically on scope exit | RAII is the default C++ resource-management strategy |
| Raw pointer vs smart pointer | Pointer meaning is ambiguous unless documented | `unique_ptr`, `shared_ptr`, `weak_ptr` encode ownership | Raw pointer should usually mean non-owning in modern C++ |
| Stack object vs heap object | Stack and heap both possible, manual heap control | Prefer scoped objects; use heap for lifetime/dynamic size/polymorphism needs | Do not heap-allocate by habit |
| Shallow copy vs deep copy | Copying a struct copies pointer values | RAII types define copy/move behavior | Use Rule of Zero where possible; Rule of Five for owning custom types |

Common bug: `free()` on memory allocated by `new`, or `delete` on memory from
`malloc`.

Interview question: What exactly does `delete` do that `free` does not?

### Strings And Arrays

| Topic | C | C++ | Enterprise / Embedded Guidance |
| --- | --- | --- | --- |
| C string vs `std::string` | Null-terminated `char` buffer; size must be tracked externally | Owns characters and tracks size/capacity | Prefer `std::string` internally; convert with `c_str()` at C boundaries |
| `char*` vs `std::string_view` | Pointer may not carry size or ownership | Non-owning pointer + length view | Use `string_view` for read-only parameters only when lifetime is guaranteed |
| C array vs `std::array` | Fixed array decays to pointer in many contexts | Fixed-size wrapper with `.size()`, iterators, assignment | Prefer `std::array` for fixed-size C++ data |
| C dynamic array vs `std::vector` | Pointer + size/capacity convention | Owns storage and tracks size/capacity | Prefer `std::vector` for dynamic arrays |

Common bug: calling `reserve()` and then indexing into a `std::vector` as if
elements already exist. `reserve()` changes capacity, not size.

Interview question: What is array decay, and why does it lose size information?

### Functions And Callbacks

| Topic | C | C++ | Enterprise / Embedded Guidance |
| --- | --- | --- | --- |
| Function pointer vs lambda | Function address; no captured state by itself | Anonymous function object, can capture state | Use function pointer for C ABI; lambda for local C++ behavior |
| Function pointer vs `std::function` | Lightweight, exact function signature | Type-erased wrapper for functions, lambdas, functors | Use `std::function` for stored callbacks; templates for hot paths |
| C callback vs C++ callback object | Function pointer plus `void* user_data` | Lambda/object/observer can encode state and lifetime | Watch callback lifetime and thread context |
| Macro vs inline function | Text substitution, no type checking, multiple evaluation risk | Typed, scoped, debuggable function | Prefer inline function for expression-like behavior |
| Macro vs `constexpr` | Preprocessor text or constant | Typed compile-time value/function | Prefer `constexpr` for constants and compile-time computation |
| Macro vs template | Text generation | Type-checked generic code | Prefer templates for generic code; keep macros for conditional compilation |

Common bug: storing a callback lambda that captured a local variable by
reference after that local variable is destroyed.

Interview question: When would you choose `std::function` over a template
callback?

### OOP And Design

| Topic | C | C++ | Enterprise / Embedded Guidance |
| --- | --- | --- | --- |
| OOP in C vs OOP in C++ | `struct` plus functions and manual conventions | Classes, constructors, destructors, access control, virtual functions | Use C-style OOP for C ABI/plugin systems; C++ classes for invariants |
| Function pointer table vs virtual function | Manual dispatch table | Built-in runtime dispatch | Virtual functions are clearer when runtime polymorphism is the intent |
| Ops table vs interface | Struct of function pointers and context | Abstract base class or concept/template interface | Use ops table at C boundary; C++ interface internally |
| Inheritance vs composition | Composition through nested structs/pointers | Supports both inheritance and composition | Prefer composition unless substitutability is needed |
| Virtual dispatch vs static polymorphism | Function pointer call | `virtual` runtime dispatch or template compile-time dispatch | Use virtual for runtime choice; templates/concepts for compile-time type safety/performance |

Common bug: using inheritance only for code reuse when composition would be
simpler and less coupled.

Interview question: Compare a C ops table with a C++ abstract interface.

### Error Handling

| Topic | C | C++ | Enterprise / Embedded Guidance |
| --- | --- | --- | --- |
| Return code vs exception | Caller checks every status | Failure propagates through `throw`/`catch` | Use return/Result for expected local failures and C ABI; exceptions for rare cross-layer failures when allowed |
| `errno` vs exception | Diagnostic for selected APIs after failure | Exception object can carry typed context | Read `errno` only when documented; preserve operation context |
| `assert` vs exception | Debug invariant check, may disappear under `NDEBUG` | Runtime error channel for recoverable failures | Do not use `assert` for user input or operational errors |
| Exception vs `std::expected` / Result | Project-specific result structs/enums | `std::expected<T,E>` is explicit value-or-error in C++23 | Use explicit Result for expected/frequent failures; exceptions for exceptional failures |

Common bug: throwing an exception through a C callback or exported C function.

Interview question: Why should exceptions be translated at C ABI boundaries?

### Casts

| Topic | C | C++ | Enterprise / Embedded Guidance |
| --- | --- | --- | --- |
| C-style cast vs named casts | One syntax can mean many things | `static_cast`, `dynamic_cast`, `const_cast`, `reinterpret_cast` expose intent | Prefer named casts so reviewers can see the risk |
| `void*` conversion | Common for generic data and callbacks | Prefer templates, typed interfaces, or specific casts at boundaries | Keep `void*` at C boundaries; restore type as soon as possible |

Common bug: using a C-style cast that silently removes `const` or performs a
dangerous reinterpretation.

Interview question: Why is `reinterpret_cast` a code-review hotspot?

## 8. Common Bugs

- Mixing allocation families: `malloc` with `delete`, or `new` with `free`.
- Forgetting `delete[]` for memory allocated with `new[]`.
- Treating `malloc` memory as if constructors already ran.
- Returning a pointer or reference to a local variable.
- Losing array size through decay and then using `sizeof(ptr)` incorrectly.
- Writing past a C string buffer or forgetting the null terminator.
- Storing a `std::string_view` to a temporary `std::string`.
- Using macro arguments with side effects.
- Using C-style casts to hide unsafe conversions.
- Creating two independent `std::shared_ptr` objects from one raw pointer.
- Using `std::shared_ptr` when ownership is actually exclusive.
- Capturing local variables by reference in long-lived callbacks.
- Throwing exceptions across C ABI boundaries.
- Using `assert` to validate user input or runtime operational failures.

## 9. Debugging

### Memory And Lifetime

Use AddressSanitizer and UBSan:

```sh
g++ -std=c++17 -g -O1 -fsanitize=address,undefined file.cpp -o app_asan
./app_asan
```

Look for:

- use-after-free;
- stack-use-after-return;
- double delete/free;
- buffer overflow;
- invalid cast or misalignment;
- null dereference.

### Allocation-Family Audit

Search mixed code for:

```text
malloc
calloc
realloc
free
new
delete
delete[]
```

For every allocation, answer:

- who owns it?
- who releases it?
- which function releases it?
- can an early return or exception skip cleanup?

### Macro Debugging

Use preprocessing output when a macro behaves strangely:

```sh
g++ -E file.cpp > file.i
```

Then replace expression macros with inline functions, `constexpr`, or templates
where possible.

### Container And Array Bugs

Check:

- array decay;
- `reserve()` vs `resize()`;
- iterator invalidation after `std::vector` reallocation;
- dangling pointers from `vector::data()` after vector growth;
- out-of-bounds access through `operator[]`.

### Callback Bugs

Trace:

- registration and unregistration;
- capture lifetimes;
- whether callback runs synchronously or later;
- whether callback runs under a lock;
- whether callback may outlive the object it references.

## 10. Best Practices

- Use C style deliberately; do not use it accidentally in C++.
- Prefer RAII for every owned resource.
- Prefer `std::vector` over manual dynamic arrays.
- Prefer `std::array` over C arrays when size is fixed and no C ABI is needed.
- Prefer `std::string` for owning text.
- Use `std::string_view` only for non-owning read-only views with clear
  lifetime.
- Prefer `std::unique_ptr` for exclusive ownership.
- Use `std::shared_ptr` only when ownership is truly shared.
- Use raw pointers for non-owning optional references or C interop, and document
  that they do not own.
- Prefer `enum class` over unscoped enums.
- Prefer `std::variant` over manual tagged unions unless layout/ABI requires a
  union.
- Prefer `using` over `typedef` in C++.
- Prefer named casts over C-style casts.
- Prefer inline functions, `constexpr`, and templates over expression macros.
- Prefer lambdas over `std::bind`.
- Use `std::function` for stored callbacks when type erasure is useful.
- Keep exceptions inside C++ boundaries; translate to status codes at C ABI
  boundaries.
- Preserve C compatibility only where it is a real requirement.

## 11. Interview Readiness

### Is C++ Just C With Classes?

No. C++ supports low-level C-like programming, but its main production value is
expressing ownership, lifetime, invariants, generic code, and abstraction more
directly through RAII, classes, templates, standard containers, smart pointers,
lambdas, and exceptions/Result-style APIs.

### When Is C Style Better?

C style is better for C ABI boundaries, embedded C codebases, vendor SDKs,
stable plugin interfaces, or environments where C++ runtime features are not
allowed.

### What Is The Biggest Memory Difference?

C allocation gives raw storage and manual cleanup. C++ object construction and
RAII manage lifetime. The safest C++ design usually avoids raw `new`/`delete`
and uses scoped objects, containers, and smart pointers.

### Why Prefer `std::vector` To `new[]`?

`std::vector` tracks size and capacity, releases memory automatically, supports
copy/move semantics, and integrates with algorithms. `new[]` requires manual
delete, separate size tracking, and careful exception cleanup.

### Why Prefer `enum class`?

`enum class` scopes enumerator names and avoids implicit conversion to integer.
This prevents accidental comparisons or assignments across unrelated enum
domains.

### Function Pointer vs Lambda vs `std::function`?

Use a function pointer for C ABI or simple no-state callbacks. Use a lambda for
local C++ behavior and captures. Use `std::function` when you need to store a
type-erased callable. Use templates when performance matters and the callback
does not need type erasure.

### Return Code vs Exception?

Return codes are explicit and ABI-friendly. Exceptions separate normal flow from
rare failure flow and propagate across layers. Result/`std::expected` style is a
good explicit choice for expected failures.

## 12. Practice

1. Convert a `malloc`/`free` dynamic array to `std::vector<int>`. Explain which
   cleanup and bounds bugs disappear.
2. Write a `SQUARE(x)` macro, call it with `++i`, then replace it with
   `constexpr int square(int)`.
3. Implement a C callback with `void* user_data`, then rewrite it with a C++
   lambda.
4. Create a C tagged union with an enum tag, then rewrite it with
   `std::variant`.
5. Wrap a `FILE*` in `std::unique_ptr<FILE, decltype(&std::fclose)>`.
6. Write a function that accidentally returns a pointer to a local array, then
   fix it with `std::array` or `std::vector`.
7. Demonstrate a dangling `std::string_view`, then fix the ownership model.
8. Design a C API wrapper that catches C++ exceptions and returns an error code.

## 13. Reference Notes

- C and C++ are separate languages with separate standards. They overlap, but
  valid C is not always valid C++, and idiomatic C is not always idiomatic C++.
- `std::expected` is a C++23 library facility. For older standards, projects
  often use a local `Result<T, E>` type or a third-party equivalent.
- `std::string_view` is non-owning. It is not a replacement for `std::string`
  when ownership is required.
- `std::span` is C++20. When unavailable, use pointer-plus-size, iterator pairs,
  or project-specific view types.
