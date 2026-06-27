# 15 - C Vs C++ Comparison Interview Pack

## How To Use This Pack

Use these questions to test whether a candidate can compare C and C++ as engineering tools, not as slogans. Strong answers should explain ownership, lifetime, type safety, ABI boundaries, error handling, and practical debugging tradeoffs.

Good answers usually say when C-style code is appropriate, when C++ abstractions are better, and what bug each choice is trying to prevent.

## Beginner Questions

### 1. Is C++ just C with classes?

**Short answer:** No. C++ can use C-style mechanisms, but it also has constructors/destructors, references, overloads, templates, exceptions, RAII, standard containers, smart pointers, lambdas, and stronger type-safe abstractions.

**Deep explanation:** C usually separates data from functions and makes ownership/manual cleanup explicit. C++ can still write low-level code, but it lets a type express invariants, cleanup, copying, moving, and error behavior. The important interview point is not "C is unsafe and C++ is safe"; C++ becomes safer when you use the language idioms correctly.

**C/C++ code/API anchor:**

```cpp
#include <string>
#include <vector>

struct Packet {
    std::string source;
    std::vector<unsigned char> payload;
};
```

**Production/debug angle:** In production C++, prefer standard library types for owned strings and dynamic arrays. At C ABI boundaries, translate to plain pointers, sizes, and status codes deliberately.

**Traps:** Writing C++ as C with `malloc`, raw owning pointers, macros, and manual cleanup often keeps C's risks while adding C++ object-lifetime rules.

**Follow-ups:** When would you intentionally expose a C API from C++? What does RAII solve that a `goto cleanup` pattern solves manually in C?

### 2. Compare C `struct`, C++ `struct`, and C++ `class`.

**Short answer:** A C `struct` is mainly an aggregate data layout. A C++ `struct` can also have constructors, methods, operators, and access control. In C++, `struct` and `class` differ mainly by default access: `struct` defaults to `public`, `class` defaults to `private`.

**Deep explanation:** Use `struct` for passive values whose fields are naturally public. Use `class` when the type owns invariants and should hide representation. C-compatible `struct` definitions are still useful for ABI boundaries and shared headers.

**C/C++ code/API anchor:**

```cpp
struct Point {
    int x{};
    int y{};
};

class Counter {
public:
    void increment() { ++value_; }
    int value() const { return value_; }

private:
    int value_{0};
};
```

**Production/debug angle:** If invalid states are possible, prefer a `class` or a `struct` with constructors and validation. Plain public fields are fine when every combination of values is valid.

**Traps:** Assuming `struct` means "C only" in C++; putting invariants in comments instead of constructors/member functions.

**Follow-ups:** Why might an ABI-facing header avoid C++ member functions? What does `extern "C"` solve?

### 3. Compare C arrays, `std::array`, and `std::vector`.

**Short answer:** A C array has fixed size and often decays to a pointer when passed to functions. `std::array<T, N>` is a fixed-size value object that keeps its size in the type. `std::vector<T>` is a dynamic owning array.

**Deep explanation:** C arrays are efficient but easy to misuse because size information is not carried through ordinary pointer parameters. `std::array` is good for fixed-size collections. `std::vector` is the default C++ choice for runtime-sized contiguous storage.

**C/C++ code/API anchor:**

```cpp
#include <array>
#include <vector>

void use_array(const std::array<int, 3>& a);
void use_vector(const std::vector<int>& v);
```

**Production/debug angle:** Audit APIs for pointer-plus-size pairs. For C++ internal code, passing `std::vector`, `std::array`, or spans/views makes size and ownership clearer.

**Traps:** Confusing `reserve()` with `resize()`; keeping iterators/references after `std::vector` reallocation; using `sizeof(param)` on a decayed C array parameter.

**Follow-ups:** When would `std::deque` or `std::list` be better than `std::vector`? What invalidates iterators in a vector?

### 4. Compare C strings, `std::string`, and `std::string_view`.

**Short answer:** A C string is a null-terminated character sequence. `std::string` owns its characters and knows its size. `std::string_view` is a non-owning view into existing characters.

**Deep explanation:** C strings are compact and ABI-friendly, but they rely on correct null termination and external lifetime management. `std::string` is the normal C++ owning string. `std::string_view` is useful for read-only parameters and parsing, but it must never outlive the data it views.

**C/C++ code/API anchor:**

```cpp
#include <string>
#include <string_view>

bool starts_with(std::string_view text, std::string_view prefix) {
    return text.substr(0, prefix.size()) == prefix;
}
```

**Production/debug angle:** Prefer `std::string` for ownership and `std::string_view` for temporary read-only access. At C boundaries, use `c_str()` only while the `std::string` remains alive and unchanged.

**Traps:** Missing null terminators, buffer overflow, storing a `string_view` to a temporary `std::string`, and assuming `char*` always means writable memory.

**Follow-ups:** Why can `std::string_view view = std::string("abc");` be dangerous? How do you pass a C++ string to a C API safely?

## Mid-Level Questions

### 5. Compare `malloc/free`, `new/delete`, and RAII.

**Short answer:** `malloc/free` allocate and release raw storage. `new/delete` allocate storage and run constructors/destructors. RAII ties resource cleanup to object lifetime and is the preferred C++ model.

**Deep explanation:** C++ objects often need construction and destruction. `malloc` does not call constructors, and `free` does not call destructors. `new[]` must pair with `delete[]`, and `new` must pair with `delete`. Better C++ code avoids direct owning `new` and uses stack objects, containers, and smart pointers.

**C/C++ code/API anchor:**

```cpp
#include <memory>
#include <vector>

auto p = std::make_unique<int>(42);
std::vector<int> values{1, 2, 3};
```

**Production/debug angle:** Allocation-family mismatch is a classic sanitizer finding. Use ASan/LSan and code review to find `malloc` paired with `delete`, `new` paired with `free`, and raw owning pointer leaks.

**Traps:** Treating `malloc` memory as a constructed C++ object; forgetting `delete[]`; creating multiple `std::shared_ptr` objects from the same raw pointer.

**Follow-ups:** When is a raw pointer acceptable in C++? Why is `std::make_unique` usually better than direct `new`?

### 6. Compare raw pointers, references, `std::unique_ptr`, and `std::shared_ptr`.

**Short answer:** A raw pointer can mean nullable access, non-owning access, or ownership unless documented. A reference means non-null aliasing. `std::unique_ptr` expresses exclusive ownership. `std::shared_ptr` expresses shared ownership.

**Deep explanation:** Modern C++ APIs should make ownership visible in the type. Use references for required non-owning parameters, raw pointers for optional/non-owning access or C boundaries, `unique_ptr` for ownership transfer, and `shared_ptr` only when shared lifetime is real.

**C/C++ code/API anchor:**

```cpp
#include <memory>

void observe(const int& value);          // required, non-owning
void maybe_observe(const int* value);    // optional, non-owning
void take(std::unique_ptr<int> value);   // ownership transfer
```

**Production/debug angle:** Ownership ambiguity causes leaks, double deletes, and use-after-free. API design should make the caller's responsibility obvious.

**Traps:** Using `shared_ptr` as a default; passing raw pointers without documenting ownership; keeping references to destroyed objects.

**Follow-ups:** What problem does `std::weak_ptr` solve? Why should a function rarely accept `shared_ptr` by value unless it shares ownership?

### 7. Compare macros with inline functions, `constexpr`, and templates.

**Short answer:** Macros are preprocessor text substitution. Inline functions, `constexpr`, and templates are typed C++ mechanisms that obey scope, overload rules, and debugging expectations.

**Deep explanation:** Macros remain useful for conditional compilation and include guards, but expression-like macros are fragile. C++ alternatives avoid double evaluation, respect types, and produce better compiler diagnostics.

**C/C++ code/API anchor:**

```cpp
template <class T>
constexpr T square(T x) {
    return x * x;
}
```

**Production/debug angle:** Use the preprocessor output (`-E`) when diagnosing macro bugs. Prefer `constexpr` constants/functions and templates for compile-time logic in C++.

**Traps:** `#define SQUARE(x) x * x` breaks precedence and may evaluate arguments more than once; macros do not respect namespaces.

**Follow-ups:** When is a macro still the right tool? How does `constexpr` differ from `const`?

### 8. Compare function pointers, lambdas, and `std::function`.

**Short answer:** A function pointer points to a plain function with a fixed signature. A lambda is a callable object that may capture state. `std::function` is a type-erased wrapper for storing different callable types with one signature.

**Deep explanation:** C callbacks commonly use a function pointer plus `void* user_data` for context. C++ can use lambdas, functors, templates, or `std::function`. Use templates for zero-overhead generic call sites; use `std::function` when you need runtime storage of callbacks.

**C/C++ code/API anchor:**

```cpp
#include <functional>
#include <vector>

std::function<void(int)> on_value;
on_value = [](int value) { (void)value; };
```

**Production/debug angle:** Callback bugs are often lifetime bugs. Audit captures, registration/unregistration, thread context, and whether callbacks run while locks are held.

**Traps:** Capturing references that outlive the scope; assuming a capturing lambda converts to a C function pointer; using `std::bind` when a lambda is clearer.

**Follow-ups:** How do you adapt a C callback API to call a C++ object? What cost can `std::function` introduce?

### 9. Compare C `enum`/`union` with `enum class` and `std::variant`.

**Short answer:** C enums are unscoped and convert easily to integers. `enum class` is scoped and strongly typed. C unions require the programmer to track the active member. `std::variant` tracks the active alternative type safely.

**Deep explanation:** Tagged unions are common in C, but correctness depends on keeping the tag and active member synchronized. `std::variant` encodes alternatives in the type system and forces explicit handling with `std::visit` or checked access.

**C/C++ code/API anchor:**

```cpp
#include <string>
#include <variant>

enum class Kind { number, text };
using Value = std::variant<int, std::string>;
```

**Production/debug angle:** Prefer `enum class` and `std::variant` inside C++ systems. Use C enum/union layouts when required by ABI, embedded layout, serialization, or vendor interfaces.

**Traps:** Reading the inactive member of a union; relying on implicit integer conversion from enums; forgetting to update a tag after writing a union member.

**Follow-ups:** How would you serialize a `std::variant` to a C protocol? When is a union still appropriate?

### 10. Compare C-style casts with C++ named casts.

**Short answer:** A C-style cast can hide several different operations. C++ named casts make the intent visible: `static_cast`, `const_cast`, `reinterpret_cast`, and `dynamic_cast`.

**Deep explanation:** Named casts make dangerous conversions easier to search and review. `static_cast` is for well-defined conversions, `const_cast` changes const/volatile qualification, `reinterpret_cast` is low-level representation reinterpretation, and `dynamic_cast` checks polymorphic downcasts.

**C/C++ code/API anchor:**

```cpp
double d = 3.14;
int n = static_cast<int>(d);
```

**Production/debug angle:** Treat `reinterpret_cast` and `const_cast` as review hotspots. UBSan and compiler warnings can expose invalid casts, narrowing, misalignment, and undefined behavior.

**Traps:** Using a C-style cast to accidentally remove `const`; assuming `reinterpret_cast` makes object lifetime or aliasing valid.

**Follow-ups:** When is `dynamic_cast` valid? Why can removing `const` and modifying the object be undefined behavior?

## Senior Questions

### 11. How would you design a safe C wrapper around C++ code?

**Short answer:** Expose an `extern "C"` API with opaque handles, plain data types, explicit ownership rules, and status/error reporting. Catch all C++ exceptions before crossing the C boundary.

**Deep explanation:** C and C++ have different ABI, name mangling, object lifetime, and exception behavior. The C API should not expose C++ classes, templates, references, exceptions, or standard library types. Internally, the implementation can use RAII, containers, exceptions, and smart pointers.

**C/C++ code/API anchor:**

```cpp
extern "C" {
struct EngineHandle;

int engine_create(EngineHandle** out);
void engine_destroy(EngineHandle* handle);
int engine_run(EngineHandle* handle, const char* input);
}
```

**Production/debug angle:** Document which function allocates and which function releases. Convert exceptions to status codes and logs. Test failure paths, null input, double destroy defenses, and ownership transfer.

**Traps:** Throwing through a C callback or C ABI; returning pointers to C++ objects with unclear lifetime; exposing `std::string` or `std::vector` in a C ABI.

**Follow-ups:** How would you return an error message safely? What does an opaque handle buy you?

### 12. Compare OOP in C with OOP in C++.

**Short answer:** C can emulate OOP using structs, function pointer tables, and explicit context pointers. C++ provides classes, constructors/destructors, virtual functions, access control, and templates.

**Deep explanation:** C-style OOP is common in low-level libraries because it preserves C ABI and explicit control. C++ OOP makes ownership and invariants easier to package with data. C++ also offers static polymorphism through templates when runtime dispatch is unnecessary.

**C/C++ code/API anchor:**

```cpp
struct Device {
    virtual ~Device() = default;
    virtual int read() = 0;
};

int poll(Device& device) {
    return device.read();
}
```

**Production/debug angle:** Choose virtual dispatch when runtime substitution is needed. Choose templates/static polymorphism when types are known at compile time and the interface should be optimized away.

**Traps:** Missing virtual destructors in polymorphic base classes; overusing inheritance where composition would be simpler; copying C function-table patterns into C++ without RAII.

**Follow-ups:** When is composition better than inheritance? How does a C ops table compare to a C++ interface?

### 13. Explain shallow copy vs deep copy and the Rule of Zero/Five.

**Short answer:** Shallow copy duplicates pointer values; deep copy duplicates owned resources. In C++, prefer the Rule of Zero by storing resources in RAII members. If a type manually manages a resource, define or delete the five special member functions intentionally.

**Deep explanation:** C structs with raw pointers commonly need manual copy/destroy functions. C++ can encode cleanup in destructors and ownership in members like `std::vector`, `std::string`, and smart pointers. Manual ownership requires careful copy constructor, copy assignment, move constructor, move assignment, and destructor decisions.

**C/C++ code/API anchor:**

```cpp
#include <string>
#include <vector>

struct User {
    std::string name;
    std::vector<int> scores;
}; // Rule of Zero
```

**Production/debug angle:** Copy bugs appear as double free, leaked memory, or use-after-free. Sanitizers and ownership-focused code review are useful here.

**Traps:** Copying a raw owning pointer; defining a destructor but forgetting copy/move behavior; assuming `memcpy` is valid for non-trivial C++ objects.

**Follow-ups:** When would you delete copy operations? Why is `std::unique_ptr` move-only?

### 14. Compare return codes, `errno`, exceptions, and Result-style APIs.

**Short answer:** C often uses return codes and sometimes `errno`. C++ can use exceptions for exceptional failures and Result-style types for explicit expected failures. `assert` is for programmer bugs, not runtime error handling.

**Deep explanation:** Return codes are ABI-stable and predictable but easy to ignore. Exceptions separate normal flow from failure flow, but they require exception-safe cleanup and must not cross C boundaries. Result-style APIs make errors explicit in the type and are useful when failure is common or part of normal control flow.

**C/C++ code/API anchor:**

```cpp
#include <optional>
#include <string>

std::optional<int> parse_int(std::string_view text);
```

**Production/debug angle:** Use RAII so cleanup happens on every exit path, including exceptions. At module boundaries, define a clear error policy and translate between exceptions, status codes, and logs.

**Traps:** Ignoring return codes; reading `errno` after a function that did not promise to set it; using `assert` for invalid user input; throwing through C ABI.

**Follow-ups:** When would you avoid exceptions? What does "exception safe" mean for a function that modifies state?

### 15. How do you decide between C style and C++ style in embedded or enterprise code?

**Short answer:** Use C style where ABI, vendor SDKs, deterministic layout, or C-only environments require it. Use C++ style internally when RAII, containers, type safety, and clearer ownership reduce defects.

**Deep explanation:** The best answer is contextual. C APIs are stable and portable across language boundaries. C++ implementations can still expose C-compatible interfaces while using safer internals. The design should make lifetime, allocation ownership, error handling, and data layout explicit.

**C/C++ code/API anchor:**

```cpp
#include <memory>

using FilePtr = std::unique_ptr<FILE, int (*)(FILE*)>;
```

**Production/debug angle:** Keep a boundary policy: who allocates, who frees, what may throw, what is thread-safe, what owns buffers, and what representation is ABI-stable.

**Traps:** Mixing allocation families across boundaries; passing C++ standard library types through C ABI; hiding ownership in raw pointers.

**Follow-ups:** How would you document ownership in a C header? What tradeoff does a C wrapper around C++ introduce?

## Coding Tasks

### Task 1. Fix the allocation-family bug.

**Prompt:**

```cpp
#include <cstdlib>

int* make_values() {
    return static_cast<int*>(std::malloc(3 * sizeof(int)));
}

int main() {
    int* p = make_values();
    delete[] p;
}
```

**Short answer:** Pair `malloc` with `free`, or replace the whole design with `std::vector<int>`.

**Deep explanation:** The bug is not only the final deallocation call; it is the API returning an owning raw pointer with no allocation-family contract. In C++ internal code, returning a standard container makes ownership and size explicit.

**C/C++ code/API anchor:**

```cpp
#include <vector>

std::vector<int> make_values() {
    return {1, 2, 3};
}
```

**Production/debug angle:** ASan can report allocation/deallocation mismatch. The deeper fix is to remove manual ownership from the API.

**Traps:** Replacing `delete[]` with `delete`; forgetting that `malloc` does not construct objects.

**Follow-ups:** How would this change if the memory came from a C library that requires its own release function?

### Task 2. Replace a dangerous macro.

**Prompt:**

```cpp
#define SQUARE(x) x * x
```

**Short answer:** Use a typed inline/`constexpr` function or template.

**Deep explanation:** A function or function template is checked by the compiler, respects scope and namespaces, avoids accidental double evaluation, and gives better diagnostics than preprocessor substitution.

**C/C++ code/API anchor:**

```cpp
template <class T>
constexpr T square(T x) {
    return x * x;
}
```

**Production/debug angle:** The macro fails for `SQUARE(a + b)` and may double-evaluate arguments with side effects.

**Traps:** Writing `#define SQUARE(x) ((x) * (x))` improves precedence but still has double-evaluation risk.

**Follow-ups:** When would a macro be acceptable in a C++ codebase?

### Task 3. Adapt a C callback to a C++ object safely.

**Prompt:** A C library calls `void (*cb)(int, void*)`. Show how a C++ object can receive the callback.

**Short answer:** Use a non-capturing trampoline function and pass the object as `void*` context.

**Deep explanation:** C cannot call a capturing lambda or C++ member function directly through a plain function pointer. The usual bridge is a C-compatible function that casts `void* user_data` back to the owning C++ object. The object must outlive the registration and unregister before destruction.

**C/C++ code/API anchor:**

```cpp
struct Listener {
    void on_value(int value) { (void)value; }
};

extern "C" void trampoline(int value, void* ctx) {
    auto* listener = static_cast<Listener*>(ctx);
    listener->on_value(value);
}
```

**Production/debug angle:** The hard part is lifetime, not the cast. Registration ownership and callback thread context must be documented.

**Traps:** Passing a capturing lambda where a C function pointer is required; letting `ctx` dangle; throwing from the callback into C.

**Follow-ups:** How would you handle errors inside `on_value` without throwing across the C API?

### Task 4. Wrap a C resource with RAII.

**Prompt:** Wrap a `FILE*` so it closes automatically.

**Short answer:** Use a smart pointer with a custom deleter or a small class.

**Deep explanation:** `FILE*` is a C resource that must be released with `fclose`. A RAII wrapper makes this rule part of the type so cleanup happens on all exit paths.

**C/C++ code/API anchor:**

```cpp
#include <cstdio>
#include <memory>

using File = std::unique_ptr<FILE, int (*)(FILE*)>;

File open_file(const char* path, const char* mode) {
    return File(std::fopen(path, mode), std::fclose);
}
```

**Production/debug angle:** RAII makes early returns and exceptions safe because cleanup is tied to scope exit.

**Traps:** Calling `fclose` on `nullptr` without checking API guarantees; copying an owning wrapper; ignoring failed `fopen`.

**Follow-ups:** How would you report an open failure? Why is `unique_ptr` appropriate here?

### Task 5. Convert a tagged union idea to `std::variant`.

**Prompt:** Replace a manual "kind plus union" value that stores either an `int` or text.

**Short answer:** Use `std::variant<int, std::string>` and handle alternatives explicitly.

**Deep explanation:** A manual tagged union relies on the programmer to keep the tag and active member synchronized. `std::variant` tracks the active alternative and makes handling type-safe.

**C/C++ code/API anchor:**

```cpp
#include <iostream>
#include <string>
#include <variant>

using Value = std::variant<int, std::string>;

void print(const Value& value) {
    std::visit([](const auto& x) { std::cout << x << '\n'; }, value);
}
```

**Production/debug angle:** `std::variant` prevents reading an inactive union member and makes new alternatives visible at compile time.

**Traps:** Assuming `std::get<T>` cannot fail; forgetting that ABI or wire formats may still require an explicit tag.

**Follow-ups:** How would you convert this to a C ABI representation?

## Debugging Scenarios

### Scenario 1. ASan reports allocation/deallocation mismatch.

**Short answer:** Find where the object was allocated and pair it with the matching release mechanism.

**Deep explanation:** `malloc/calloc/realloc` pair with `free`. `new` pairs with `delete`. `new[]` pairs with `delete[]`. Vendor C APIs may require a dedicated destroy function.

**C/C++ code/API anchor:** Search for `malloc`, `calloc`, `realloc`, `free`, `new`, `delete`, and ownership-transfer functions.

**Production/debug angle:** Prefer containers and RAII wrappers so the allocation family is not repeated at every call site.

**Traps:** Fixing only the crashing site while leaving ownership ambiguous in the API.

**Follow-ups:** How would you redesign the function signature to prevent this mismatch?

### Scenario 2. A stored `std::string_view` prints garbage.

**Short answer:** The view probably outlived the string or buffer it referenced.

**Deep explanation:** `std::string_view` does not own data. It is safe as a parameter or short-lived parser view, but unsafe as stored state unless the referenced storage lifetime is guaranteed.

**C/C++ code/API anchor:**

```cpp
#include <string>
#include <string_view>

std::string_view bad() {
    std::string local = "temporary";
    return local;
}
```

**Production/debug angle:** Review stored views and callback captures. Prefer `std::string` when the object must own text.

**Traps:** Assuming `string_view` extends lifetime like `const std::string&`; building a view from a temporary.

**Follow-ups:** When is `std::string_view` the ideal parameter type?

### Scenario 3. A callback crashes after the owner object is destroyed.

**Short answer:** The callback context or captured reference is dangling.

**Deep explanation:** C callbacks and C++ lambdas often store a pointer, reference, or `this`. If the callback can run after the target object is destroyed, unregister it or use a lifetime-aware design.

**C/C++ code/API anchor:** Check callback registration, unregistration, capture lists, `void* user_data`, `std::function` storage, and destructor order.

**Production/debug angle:** Add clear ownership rules: who stores the callback, when it may run, which thread calls it, and whether callbacks run under locks.

**Traps:** Capturing `[&]` into a long-lived callback; throwing through a C callback; unregistering after the library may already be invoking the callback.

**Follow-ups:** How would `weak_ptr` help for asynchronous C++ callbacks? Why might it not solve a pure C callback by itself?
