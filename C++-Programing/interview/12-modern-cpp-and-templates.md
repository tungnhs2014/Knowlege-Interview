# 12 - Modern C++ And Templates: Interview Pack

## How To Use This Pack

For each question:

1. give the short answer first;
2. explain the mechanism, lifetime rule, or tradeoff;
3. anchor the answer in C++ code or a Standard Library API;
4. connect it to production behavior or debugging;
5. name traps explicitly;
6. answer follow-ups without changing the original claim.

The examples use C++17 unless marked otherwise. C++20 concepts, `std::span`,
`consteval`, and `constinit` are marked. `std::expected` is C++23 awareness.

## Beginner Questions

### 1. What does "Modern C++" mean in real project code?

**Short answer**

Modern C++ means using language and library features that make ownership,
lifetime, type requirements, and compile-time intent explicit. It is not just
using newer syntax for style.

**Deep explanation**

Modern C++ starts with RAII and value semantics, then adds tools such as smart
pointers, lambdas, move semantics, vocabulary types, `constexpr`, templates,
and concepts. The goal is to move important rules from comments into types and
compiler-checked code.

Good Modern C++ code should answer questions like: who owns this object, can
this callback outlive its captures, can this value be absent, can this type be
used with this template, and is this computation required at compile time?

**C++ code/API anchor**

```cpp
#include <memory>
#include <optional>
#include <string>
#include <string_view>

struct Device {
    std::string name;
};

std::unique_ptr<Device> create_device(std::string_view name);
std::optional<Device> find_cached_device(std::string_view name);
void inspect_device(const Device& device);
```

The signatures communicate ownership, optional value, borrowed text, and
borrowed object access.

**Production/debug angle**

During code review, inspect the API before reading the implementation. If the
signature hides ownership or lifetime rules, bugs will appear during refactor,
async callbacks, error paths, or tests with larger data.

**Common traps**

- Saying Modern C++ means "use every new feature."
- Replacing clear code with clever template metaprogramming.
- Using `shared_ptr` everywhere instead of designing ownership.
- Treating `auto` as always better than explicit types.

**Follow-up questions**

- Which Modern C++ feature most directly expresses ownership?
- When can `std::string_view` make an API worse?
- How would you explain Modern C++ to a C programmer?

### 2. Why should you prefer `nullptr` over `NULL` or `0`?

**Short answer**

`nullptr` is a typed null pointer value. `0` is an integer literal, and `NULL`
is commonly a macro that may behave like `0`, so overload resolution can choose
the wrong function.

**Deep explanation**

In C++, overload resolution uses types. Passing `0` may select an integer
overload instead of a pointer overload. `nullptr` has type `std::nullptr_t` and
converts to pointer types but not to ordinary integers.

**C++ code/API anchor**

```cpp
#include <iostream>

void open(int)
{
    std::cout << "integer\n";
}

void open(char*)
{
    std::cout << "pointer\n";
}

int main()
{
    open(0);       // integer
    open(nullptr); // pointer
}
```

**Production/debug angle**

Prefer `nullptr` in APIs and tests so null pointer intent survives overloads,
templates, and refactoring.

**Common traps**

- Saying `NULL` and `nullptr` are the same in C++.
- Passing `0` to overloaded APIs and assuming pointer intent is obvious.
- Confusing null pointer checks with dangling pointer safety.

**Follow-up questions**

- What is the type of `nullptr`?
- Can `nullptr` convert to `bool`?
- Does `nullptr` prevent use-after-free?

### 3. What is a lambda expression, and what is a closure?

**Short answer**

A lambda is an inline unnamed function object. The closure is the compiler
generated object that stores any captured state and provides `operator()`.

**Deep explanation**

The lambda expression creates a unique closure type. If it captures variables
by value, the closure stores copies. If it captures by reference, the closure
stores references or equivalent access to the original objects. The capture
list controls the state, while the parameter list controls the call interface.

Lambdas are common with STL algorithms, callbacks, local policies, and short
one-off behavior.

**C++ code/API anchor**

```cpp
#include <algorithm>
#include <vector>

int main()
{
    std::vector<int> values{4, 1, 3, 2};
    int limit = 2;

    auto count = std::count_if(values.begin(), values.end(),
                               [limit](int value) {
                                   return value > limit;
                               });

    return static_cast<int>(count);
}
```

`limit` is stored in the closure because it is captured by value.

**Production/debug angle**

When debugging callback bugs, inspect the capture list before the body. A
correct-looking body can still be unsafe if the closure outlives a referenced
object.

**Common traps**

- Saying a lambda is only syntax sugar for a function pointer.
- Capturing by reference in a lambda that is stored or returned.
- Assuming `[=]` always makes object lifetime safe.
- Forgetting that a stateful lambda cannot convert to a C function pointer.

**Follow-up questions**

- When can a lambda convert to a function pointer?
- What does `mutable` do on a lambda?
- What is a generic lambda?

### 4. Compare capture by value and capture by reference.

**Short answer**

Capture by value copies the captured variable into the closure. Capture by
reference lets the closure access the original object. Value capture is safer
for stored callbacks; reference capture can dangle.

**Deep explanation**

Value capture records the value at lambda creation time. Reference capture
observes later changes to the original object, but the original must outlive
every lambda call. C++ closures do not extend the lifetime of objects captured
by reference. The same lifetime concern applies to capturing `this`.

**C++ code/API anchor**

```cpp
#include <functional>
#include <iostream>

std::function<void()> bad()
{
    int value = 42;
    return [&value] {
        std::cout << value << '\n'; // undefined behavior when called later
    };
}

std::function<void()> good()
{
    int value = 42;
    return [value] {
        std::cout << value << '\n';
    };
}
```

**Production/debug angle**

Stored, returned, async, or cross-thread callbacks should almost never use
broad `[&]` capture. Prefer explicit captures and capture by value or by move
when lifetime is uncertain.

**Common traps**

- Believing `const` reference capture prevents dangling.
- Capturing `this` for async work without lifetime ownership.
- Using `[&]` because it is shorter.
- Mutating value captures without understanding `mutable`.

**Follow-up questions**

- How do you capture a `std::unique_ptr`?
- What does `[*this]` do in C++17?
- Why is broad default capture risky in code review?

### 5. What does `std::move` do?

**Short answer**

`std::move` casts an expression to an xvalue so move-aware overloads can be
selected. It does not move anything by itself.

**Deep explanation**

The actual transfer happens in a move constructor, move assignment operator, or
move-aware function. After moving from an object, the object remains valid, but
its value is generally unspecified unless that type documents more. You can
destroy it or assign a new value, but you should not depend on the old value.

**C++ code/API anchor**

```cpp
#include <string>
#include <utility>

int main()
{
    std::string source = "payload";
    std::string target = std::move(source);

    source = "safe to reuse by assignment";
}
```

**Production/debug angle**

When investigating unexpected copies or moves, log copy/move constructors in a
small reproducer. Also check whether the move operation is `noexcept`; some
containers may copy during reallocation if moving can throw.

**Common traps**

- Saying `std::move` performs the move.
- Reading a moved-from object as if it still had the old contents.
- Moving a `const` object and expecting a real move.
- Writing `return std::move(local);` unnecessarily.

**Follow-up questions**

- What is an xvalue?
- Why does `std::move` on a `const std::string` usually copy?
- When should you use `std::forward` instead?

### 6. What is a template, and why is it safer than a macro?

**Short answer**

A template is a typed compile-time pattern for generating functions or classes.
A macro is preprocessor text substitution. Templates obey C++ type checking,
scope, overload resolution, and diagnostics; macros do not.

**Deep explanation**

Templates let one implementation work with many types while preserving type
safety. The compiler instantiates code for the concrete types used. Macros
substitute tokens before the compiler sees the C++ program, so they can
duplicate evaluation and ignore types.

**C++ code/API anchor**

```cpp
#define MAX_BAD(a, b) ((a) < (b) ? (b) : (a))

template <typename T>
T max_value(T a, T b)
{
    return a < b ? b : a;
}
```

`MAX_BAD(x++, y)` may evaluate `x++` more than once. `max_value(x++, y)`
evaluates arguments once before the function call.

**Production/debug angle**

Use templates for typed generic algorithms and `constexpr` for typed constants.
Keep macros mostly for conditional compilation and unavoidable preprocessor
work.

**Common traps**

- Saying templates are just safer macros.
- Forgetting template definitions usually need to be visible at instantiation.
- Writing unconstrained templates that produce unreadable diagnostics.
- Overusing templates when a simple overload is clearer.

**Follow-up questions**

- What is template instantiation?
- Why do templates often live in headers?
- Can function templates be partially specialized?

## Mid-Level Questions

### 7. Explain lvalue, rvalue, rvalue reference, and forwarding reference.

**Short answer**

An lvalue has identity and can usually be addressed. An rvalue is a temporary
or expiring value. An rvalue reference `T&&` can bind to rvalues. In a deduced
template context, `T&&` can be a forwarding reference that preserves lvalue or
rvalue-ness with `std::forward<T>`.

**Deep explanation**

Move semantics depend on value categories. A named object is an lvalue even if
its type is `T&&`. `std::move(x)` casts `x` to an xvalue. A forwarding
reference appears as `T&&` where `T` is deduced, such as `template <typename T>
void f(T&&)`. It can bind to both lvalues and rvalues, and reference-collapsing
rules preserve the original category.

**C++ code/API anchor**

```cpp
#include <iostream>
#include <utility>

void process(int&)
{
    std::cout << "lvalue\n";
}

void process(int&&)
{
    std::cout << "rvalue\n";
}

template <typename T>
void wrapper(T&& value)
{
    process(std::forward<T>(value));
}

int main()
{
    int x = 1;
    wrapper(x);  // lvalue
    wrapper(2);  // rvalue
}
```

**Production/debug angle**

Forwarding is useful for factory wrappers and generic adapters. Outside those
contexts, `T&&` can make APIs harder to understand. Confirm whether the code is
actually forwarding or just accepting too much.

**Common traps**

- Thinking every `T&&` is a forwarding reference.
- Calling `std::move` inside a forwarding wrapper instead of `std::forward<T>`.
- Forgetting that a named rvalue reference variable is an lvalue.
- Treating moved-from state as predictable.

**Follow-up questions**

- What is reference collapsing?
- Why is `std::forward<T>` conditional?
- When would `const T&&` be useful?

### 8. Why should move constructors often be `noexcept`?

**Short answer**

Move operations should be `noexcept` when they truly cannot throw because
standard containers may prefer copying over moving during reallocation if a
move might throw.

**Deep explanation**

Containers must preserve exception-safety guarantees. During `std::vector`
growth, elements may need to be transferred to new storage. If moving an
element can throw but copying is available and safer for rollback, the
container may copy instead. Marking a correct move operation `noexcept` lets
containers use it confidently.

**C++ code/API anchor**

```cpp
class Buffer {
public:
    Buffer(Buffer&& other) noexcept
        : data_{other.data_}, size_{other.size_}
    {
        other.data_ = nullptr;
        other.size_ = 0;
    }

private:
    int* data_{};
    int size_{};
};
```

**Production/debug angle**

If a container of your type copies unexpectedly, inspect the move constructor
signature and whether member moves can throw. Use logging constructors in a
small example to confirm actual behavior.

**Common traps**

- Adding `noexcept` when the move can actually throw.
- Forgetting move assignment also needs the same reasoning.
- Manually writing move operations when Rule of Zero would be better.
- Assuming `std::move` guarantees no allocation or no throw.

**Follow-up questions**

- What does Rule of Zero mean here?
- How can a member type affect your `noexcept`?
- What exception-safety guarantee is the container trying to protect?

### 9. Compare `unique_ptr`, `shared_ptr`, and `weak_ptr`.

**Short answer**

`unique_ptr` expresses exclusive ownership. `shared_ptr` expresses shared
ownership through a control block. `weak_ptr` observes a shared object without
extending its lifetime and is used to break cycles.

**Deep explanation**

`unique_ptr` is movable but not copyable, so transfer is explicit. `shared_ptr`
copies increase the owner count. The control block tracks shared and weak
owners plus deletion information. `weak_ptr::lock()` returns a `shared_ptr` if
the object is still alive. Reference-count operations are thread-safe, but the
pointed object itself still needs synchronization if accessed concurrently.

**C++ code/API anchor**

```cpp
#include <memory>
#include <vector>

struct Node {
    std::vector<std::shared_ptr<Node>> children;
    std::weak_ptr<Node> parent;
};
```

The parent link is weak to avoid a reference cycle.

**Production/debug angle**

Draw an ownership graph. Mark exclusive owners, shared owners, and observers.
Leaks involving `shared_ptr` often come from cycles, not missing `delete`.

**Common traps**

- Using `shared_ptr` for every nullable parameter.
- Creating two independent `shared_ptr`s from the same raw pointer.
- Storing `get()` beyond owner lifetime.
- Using `use_count()` as production logic.

**Follow-up questions**

- When should a function take `shared_ptr` by value?
- Why does `weak_ptr::expired()` not safely give access?
- What problem does `enable_shared_from_this` solve?

### 10. Compare `std::optional`, nullable pointer, and `std::expected`.

**Short answer**

`std::optional<T>` owns either a `T` or no value. A nullable pointer may mean
optional access to an object elsewhere. `std::expected<T, E>` in C++23 carries
either a value or an error reason.

**Deep explanation**

Use `optional` when absence is part of normal control flow and no detailed
error is needed. Use a pointer when object identity or borrowing matters. Use
an error-carrying result when callers must know why an operation failed.

**C++ code/API anchor**

```cpp
#include <charconv>
#include <optional>
#include <string_view>
#include <system_error>

std::optional<int> parse_positive(std::string_view text)
{
    int value = 0;
    const char* first = text.data();
    const char* last = first + text.size();
    const auto [ptr, ec] = std::from_chars(first, last, value);

    if (ec != std::errc{} || ptr != last || value <= 0) {
        return std::nullopt;
    }

    return value;
}
```

**Production/debug angle**

Review whether "not found" and "failed" are the same state. If operations
need diagnostics, logging, retry decisions, or user messages, `optional` may
be too weak.

**Common traps**

- Calling `.value()` without checking.
- Using `optional<T&>` as if it were a standard C++ vocabulary type.
- Returning `optional` when error details matter.
- Using nullable raw pointers for ownership transfer.

**Follow-up questions**

- When is `T*` better than `optional<T>`?
- How would you model parse failure with an error message?
- What happens if `optional::value()` is called when empty?

### 11. Compare `std::variant` with a union.

**Short answer**

`std::variant` is a type-safe union-like vocabulary type. It tracks the active
alternative and manages object lifetime. A union requires manual discipline
unless restricted to trivial low-level patterns.

**Deep explanation**

In C and low-level C++, a tagged union stores one of several alternatives, but
the programmer must maintain the tag and lifetime rules. `std::variant`
combines storage, active alternative tracking, and safe visitation. It is a
good fit for closed sets such as command events, parser tokens, or state
payloads.

**C++ code/API anchor**

```cpp
#include <iostream>
#include <string>
#include <variant>

using Event = std::variant<int, double, std::string>;

void handle(const Event& event)
{
    std::visit([](const auto& value) {
        std::cout << value << '\n';
    }, event);
}
```

**Production/debug angle**

Use `std::visit` to keep handling exhaustive and type-directed. Debug wrong
alternative access by checking `index()` or `std::holds_alternative<T>()`.

**Common traps**

- Calling `std::get<T>` for the wrong active alternative.
- Using `variant` for an open plugin-style hierarchy where virtual dispatch is
  cleaner.
- Assuming `variant` has no size overhead.
- Forgetting exception paths such as `valueless_by_exception` in advanced code.

**Follow-up questions**

- When is runtime polymorphism better than `variant`?
- How does `std::visit` differ from `switch` on a manual tag?
- What makes `variant` safer than a C union?

### 12. Why can `std::string_view` and `std::span` dangle?

**Short answer**

`std::string_view` and `std::span` are non-owning views. They refer to data
owned somewhere else. If the owner is destroyed, moved, or reallocated, the
view can dangle.

**Deep explanation**

These types are excellent parameter types because they avoid copying and work
with multiple sources. They are dangerous as stored members or return values
unless the owner lifetime is part of the API contract. They behave like
pointer-plus-size, not like owning containers.

**C++ code/API anchor**

```cpp
#include <string>
#include <string_view>

std::string_view bad()
{
    std::string local = "temporary";
    return local; // dangling view
}

void print(std::string_view text); // good parameter shape
```

**Production/debug angle**

For view-related bugs, find the owner. Check operations that reallocate:
`std::string` mutation, `std::vector` growth, temporary creation, and returning
substrings from local objects.

**Common traps**

- Returning `string_view` to a local `std::string`.
- Storing `span` to a temporary vector.
- Treating a view as an owning cache entry.
- Assuming `const` prevents lifetime bugs.

**Follow-up questions**

- When should an API return `std::string` instead?
- Is `string_view` null-terminated?
- Why is `span` useful at C API boundaries?

### 13. Explain template instantiation and why template definitions often live in headers.

**Short answer**

Template instantiation is when the compiler generates concrete code from a
template for specific arguments. The compiler usually needs the full template
definition visible at the point of instantiation, so templates often live in
headers.

**Deep explanation**

A template declaration alone is not enough for the compiler to generate
`add<int>` or `FixedBuffer<std::string, 8>`. If the definition is hidden in a
`.cpp` file and another translation unit uses a new instantiation, compilation
may pass but linking can fail. Explicit instantiation is possible, but it must
be intentional and list the needed types.

**C++ code/API anchor**

```cpp
// header
template <typename T>
T add(T a, T b)
{
    return a + b;
}
```

This header-only shape lets each translation unit instantiate the needed
specializations.

**Production/debug angle**

If you see "undefined reference to `T func<T>(...)`" for a template, check
whether the definition is visible or explicitly instantiated.

**Common traps**

- Treating templates like ordinary non-template functions in `.cpp` files.
- Forgetting that every instantiation can increase code size.
- Assuming unused template code is compiled like normal code.
- Hiding constraints until a distant instantiation fails.

**Follow-up questions**

- What is explicit instantiation?
- How can templates affect build time?
- Why can template errors be long?

### 14. What is SFINAE, and how do concepts improve on it?

**Short answer**

SFINAE means "Substitution Failure Is Not An Error." Invalid template
substitution can remove a candidate from overload resolution. C++20 concepts
express such requirements directly with clearer syntax and usually better
diagnostics.

**Deep explanation**

SFINAE enabled generic libraries to choose overloads based on type properties
before concepts existed. It works, but `std::enable_if`, `std::void_t`, and
detection idioms can be verbose. Concepts put the requirement at the API
boundary and give the compiler a named constraint to report.

**C++ code/API anchor**

```cpp
#include <concepts>

template <typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template <Numeric T>
T twice(T value)
{
    return value + value;
}
```

Pre-C++20 code might use `std::enable_if_t<std::is_arithmetic_v<T>, T>`.

**Production/debug angle**

Use concepts for new C++20 public templates. Use SFINAE when supporting older
standards or implementing reusable detection traits. For debugging, identify
the failed requirement first, not the last line of a long instantiation trace.

**Common traps**

- Saying SFINAE ignores all template errors.
- Hiding requirements inside function bodies.
- Using concepts in a project locked to C++17.
- Replacing simple overloads with complex constraints.

**Follow-up questions**

- What is `std::void_t` used for?
- What is a requires expression?
- When would you still use SFINAE?

## Senior Questions

### 15. Design a callback API that avoids dangling captures and ownership cycles.

**Short answer**

Separate callable storage from object ownership. Use explicit captures, avoid
reference captures for stored callbacks, and use `weak_ptr` when a callback
observes an object managed by shared ownership without keeping it alive
forever.

**Deep explanation**

Callbacks often outlive the function that registers them. If a callback
captures references or `this`, it can call into destroyed objects. Capturing a
`shared_ptr` can prevent destruction, but bidirectional callback ownership can
create cycles. A common robust pattern is: owner is managed by `shared_ptr`,
callback captures `weak_ptr`, and `lock()` is used at invocation time.

**C++ code/API anchor**

```cpp
#include <functional>
#include <memory>
#include <vector>

class Dispatcher {
public:
    using Callback = std::function<void(int)>;

    void add(Callback callback)
    {
        callbacks_.push_back(std::move(callback));
    }

    void emit(int value)
    {
        for (auto& callback : callbacks_) {
            callback(value);
        }
    }

private:
    std::vector<Callback> callbacks_;
};

class Client : public std::enable_shared_from_this<Client> {
public:
    void subscribe(Dispatcher& dispatcher)
    {
        std::weak_ptr<Client> self = shared_from_this();
        dispatcher.add([self](int value) {
            if (auto locked = self.lock()) {
                locked->on_event(value);
            }
        });
    }

private:
    void on_event(int);
};
```

**Production/debug angle**

When debugging callback crashes or leaks, draw both ownership and callback
graphs. Ask whether the dispatcher owns the callback, whether the callback
owns the object, and whether the object owns the dispatcher.

**Common traps**

- Capturing `[this]` into a dispatcher that outlives the object.
- Capturing `shared_ptr` everywhere and creating cycles.
- Using `std::function` in performance-critical hot paths without measuring.
- Calling `shared_from_this()` before the object is owned by `shared_ptr`.

**Follow-up questions**

- When would a templated callback parameter be better than `std::function`?
- How would you unregister callbacks safely?
- What changes if callbacks run on another thread?

### 16. When would you choose templates over runtime polymorphism, and when not?

**Short answer**

Choose templates for compile-time generic code and zero-overhead customization.
Choose runtime polymorphism when the implementation must vary at runtime, when
you need a stable interface boundary, or when reducing template complexity is
more important than static dispatch.

**Deep explanation**

Templates instantiate code for each concrete type. This can optimize well and
avoid virtual dispatch, but it may increase build time, code size, and error
complexity. Runtime polymorphism uses base classes and virtual functions, which
support dynamic substitution and can hide implementation details behind a
stable ABI-like interface.

**C++ code/API anchor**

```cpp
template <typename Sink>
void write_template(Sink& sink, int value)
{
    sink.write(value);
}

struct Sink {
    virtual void write(int value) = 0;
    virtual ~Sink() = default;
};

void write_virtual(Sink& sink, int value)
{
    sink.write(value);
}
```

**Production/debug angle**

For application code, start simple. Use templates when the type is naturally a
compile-time parameter, such as container element type or small policy. Use
runtime polymorphism for plugin boundaries, runtime-selected drivers, and
interfaces that need separate compilation.

**Common traps**

- Claiming templates are always faster.
- Claiming virtual functions are always too slow.
- Creating template-heavy APIs that make errors unreadable.
- Using inheritance where a simple generic function is enough.

**Follow-up questions**

- How can templates increase binary size?
- What is CRTP, and when is it useful?
- How would concepts improve a template API?

### 17. Explain dependent names and two-phase lookup.

**Short answer**

A dependent name depends on a template parameter, so the compiler may not know
what it refers to until instantiation. `typename` marks a dependent type, and
`template` marks a dependent member template. Two-phase lookup resolves
non-dependent names at template definition and dependent names at
instantiation.

**Deep explanation**

In `Container::iterator`, the compiler cannot know whether `iterator` is a type
or a value until `Container` is known. The language requires `typename` to
disambiguate. Similarly, `obj.template run<int>()` tells the compiler that
`run` is a template, not an expression involving `<`.

**C++ code/API anchor**

```cpp
template <typename Container>
void first(Container& container)
{
    typename Container::iterator it = container.begin();
    (void)it;
}

template <typename T>
void call(T& object)
{
    object.template run<int>();
}
```

**Production/debug angle**

These errors often appear in advanced generic code and can look unrelated to
the real intent. Search for nested types or member templates that depend on
`T`, then add the required disambiguator.

**Common traps**

- Forgetting `typename` before `T::value_type`.
- Forgetting `.template` when calling a dependent member template.
- Assuming name lookup is delayed for every name in a template.
- Overusing dependent-name-heavy code where a simpler API would work.

**Follow-up questions**

- What is a non-dependent name?
- Why does the compiler need help parsing `<`?
- How can concepts reduce dependent-name complexity?

### 18. How would you migrate a SFINAE-heavy API to concepts?

**Short answer**

First name the real requirements, then express them as concepts at the API
boundary. Keep compatibility wrappers or conditional compilation if the library
must support pre-C++20 users.

**Deep explanation**

Do not mechanically replace every `enable_if` with a long `requires` clause.
The value of concepts is naming intent. For example, turn a detection trait
into `SizedRange`, `Serializable`, or `ByteBuffer`. Then constrain overloads
with those names and keep the implementation readable.

**C++ code/API anchor**

```cpp
#include <concepts>
#include <cstddef>

template <typename T>
concept SizedRange = requires(const T& value) {
    value.begin();
    value.end();
    { value.size() } -> std::convertible_to<std::size_t>;
};

template <SizedRange R>
std::size_t count_items(const R& range)
{
    return range.size();
}
```

**Production/debug angle**

Run existing tests with deliberately invalid types and compare diagnostics.
Good migration improves both valid behavior and invalid-code error messages.

**Common traps**

- Creating concepts named after implementation details instead of domain
  requirements.
- Breaking C++17 users without a compatibility plan.
- Over-constraining templates and rejecting valid user types.
- Leaving requirements hidden inside the function body.

**Follow-up questions**

- When would you keep a detection trait?
- What is concept subsumption?
- How do concepts affect overload resolution?

### 19. Explain `constexpr`, `consteval`, and `constinit`.

**Short answer**

`constexpr` means an entity can participate in constant expressions when used
with suitable inputs. `consteval` means a function call must produce a
compile-time result. `constinit` requires static or thread storage variables to
be initialized during static initialization, but it does not make them const.

**Deep explanation**

`constexpr` is about possible compile-time evaluation and constant-expression
use. A `constexpr` function can also run at runtime if called with runtime
values. `consteval` is stricter and creates immediate functions. `constinit`
helps avoid static initialization order surprises for static/thread storage,
but the variable can still be mutable unless separately declared `const`.

**C++ code/API anchor**

```cpp
constexpr int square(int x)
{
    return x * x;
}

consteval int required_square(int x)
{
    return x * x;
}

constinit int startup_counter = 0;
```

**Production/debug angle**

Use `constexpr` for typed constants and readable compile-time computations.
Use `consteval` for APIs that must never be runtime. Use `constinit` for static
state whose initialization timing matters.

**Common traps**

- Saying `constexpr` always runs at compile time.
- Saying `constinit` makes a variable immutable.
- Using macros for typed constants that should be `constexpr`.
- Overusing compile-time computation until builds become slow.

**Follow-up questions**

- Can a `constexpr` function be called at runtime?
- Can `constinit` be used on local automatic variables?
- How does `static_assert` relate to constant expressions?

### 20. How do you debug a long template error from an unconstrained template?

**Short answer**

Start at the first user-written instantiation site, identify the missing
operation or type requirement, then add a clearer constraint using
`static_assert`, traits, or concepts.

**Deep explanation**

Template errors are often long because the compiler reports the instantiation
chain. The useful question is usually: what did this template assume about
`T`? Maybe it needed `operator<`, `begin()`, `size()`, `value_type`, or a
non-throwing move. Unconstrained templates let invalid types reach deep
implementation code before failing.

**C++ code/API anchor**

```cpp
#include <concepts>

template <typename T>
concept LessThanComparable = requires(T a, T b) {
    { a < b } -> std::convertible_to<bool>;
};

template <LessThanComparable T>
T min_value(T a, T b)
{
    return b < a ? b : a;
}
```

**Production/debug angle**

Reduce the failing code to a tiny compile-only example. Add a named concept or
`static_assert` at the public template boundary so future failures point to the
contract, not a deep implementation line.

**Common traps**

- Reading only the last line of a template error.
- Adding casts instead of fixing the missing requirement.
- Accepting every type with `auto`/templates and relying on body failures.
- Hiding template definitions so errors become linker problems.

**Follow-up questions**

- How does `static_assert` help in C++17?
- What does a requires expression test?
- When is an overload better than a constraint?

## Coding Tasks

### Task 1. Replace a macro with a template and a `constexpr` value.

**Prompt**

Refactor this code:

```cpp
#define MAX_PACKET 256
#define MAX(a, b) ((a) < (b) ? (b) : (a))
```

**Model answer**

```cpp
constexpr int max_packet = 256;

template <typename T>
T max_value(T a, T b)
{
    return a < b ? b : a;
}
```

**What the interviewer looks for**

- Type safety.
- No duplicate evaluation.
- Awareness that `std::max` may already be the right library answer.
- Explanation that macros still have limited uses for conditional compilation.

**Follow-up questions**

- What happens with mixed types such as `int` and `double`?
- Should the template take by value or `const T&`?
- Why might `std::max` be preferable?

### Task 2. Write a fixed-size buffer with a non-type template parameter.

**Prompt**

Write a `FixedBuffer<T, N>` that can push until full and report `size()`.

**Model answer**

```cpp
#include <array>
#include <cstddef>

template <typename T, std::size_t N>
class FixedBuffer {
public:
    bool push(const T& value)
    {
        if (size_ == N) {
            return false;
        }
        data_[size_++] = value;
        return true;
    }

    std::size_t size() const
    {
        return size_;
    }

private:
    std::array<T, N> data_{};
    std::size_t size_{};
};
```

**What the interviewer looks for**

- `N` is compile-time capacity.
- `FixedBuffer<int, 4>` and `FixedBuffer<int, 8>` are different types.
- No dynamic allocation.
- Clear failure behavior when full.

**Follow-up questions**

- How would you support move-only `T`?
- Should `push` take `T` by value and move into storage?
- How would you expose safe read access?

### Task 3. Convert SFINAE to a C++20 concept.

**Prompt**

Replace an `enable_if` arithmetic-only function with a concept.

**Model answer**

```cpp
#include <concepts>

template <typename T>
concept Arithmetic = std::integral<T> || std::floating_point<T>;

template <Arithmetic T>
T safe_double(T value)
{
    return value + value;
}
```

**What the interviewer looks for**

- Requirement is named.
- Constraint is at the API boundary.
- Candidate understands C++20 requirement.
- Candidate avoids overcomplicating simple code.

**Follow-up questions**

- How would this look in C++17?
- Should `bool` count as arithmetic for this API?
- What diagnostic improvement do concepts provide?

## Debugging Scenarios

### Scenario 1. Callback crashes after the registering object is destroyed.

**Prompt**

```cpp
class Widget {
public:
    void register_callback(Dispatcher& dispatcher)
    {
        dispatcher.add([this](int value) {
            handle(value);
        });
    }

private:
    void handle(int);
};
```

**Expected diagnosis**

The lambda captures `this`. If the dispatcher stores the callback and invokes
it after `Widget` is destroyed, the callback dereferences a dangling pointer.

**Fix direction**

Use an explicit lifetime contract. Options include unregistering before
destruction, making the dispatcher not outlive the widget, or using
`weak_ptr`/`shared_from_this` carefully when shared ownership is appropriate.

**Traps**

- Capturing `[=]` does not automatically solve object lifetime.
- Capturing `shared_ptr` may create cycles.
- A null check cannot detect a dangling `this`.

**Follow-up questions**

- How would you test this with AddressSanitizer?
- What if callbacks run on another thread?
- How should callback unregistration be designed?

### Scenario 2. A returned `std::string_view` sometimes prints garbage.

**Prompt**

```cpp
#include <string>
#include <string_view>

std::string_view label()
{
    std::string s = "device";
    return s;
}
```

**Expected diagnosis**

The returned `string_view` points to a local `std::string` that is destroyed
when the function returns. The view dangles.

**Fix direction**

Return `std::string` if the function creates the text. Return `string_view`
only when it views data that outlives the returned view, such as a static
string or a substring of an input whose lifetime is documented.

**Traps**

- `string_view` does not own and does not extend lifetime.
- `const` does not fix dangling lifetime.
- Small string optimization does not make this safe.

**Follow-up questions**

- When is returning `string_view` acceptable?
- Is `string_view` guaranteed null-terminated?
- How does this compare with `std::span`?

### Scenario 3. `std::vector<MyType>` copies during growth instead of moving.

**Prompt**

You log constructors and see copies during vector reallocation even though
`MyType` has a move constructor.

**Expected diagnosis**

The move constructor may not be `noexcept`, so `std::vector` may copy to
preserve exception safety if copying is available.

**Fix direction**

Make the move constructor and move assignment `noexcept` only if their member
operations cannot throw. Prefer Rule of Zero if members already manage
resources safely.

**Traps**

- Adding `noexcept` incorrectly can call `std::terminate` if an exception is
  thrown.
- `std::move` at call sites does not force vector reallocation strategy.
- Copy elision and move logging can confuse simple experiments.

**Follow-up questions**

- What if the type is move-only and move can throw?
- How do member types affect `noexcept`?
- How would you write a small reproducer?

### Scenario 4. A template compiles in one file but fails to link in another.

**Prompt**

```cpp
// add.hpp
template <typename T>
T add(T a, T b);

// add.cpp
template <typename T>
T add(T a, T b)
{
    return a + b;
}
```

Another file calls `add(1, 2)` and gets an undefined reference.

**Expected diagnosis**

The template definition was not visible at the point of instantiation. The
compiler could not generate `add<int>` for the calling translation unit.

**Fix direction**

Put the template definition in the header or explicitly instantiate the needed
specializations in the `.cpp` file.

**Traps**

- Treating function templates exactly like normal functions.
- Explicitly instantiating only one type and later using another.
- Hiding template bodies to reduce compile time without a plan.

**Follow-up questions**

- What is explicit instantiation?
- How do header-only libraries manage this?
- How can templates affect build times?

## Final Review Checklist

- Can the candidate explain ownership and lifetime, not just syntax?
- Can they say what `std::move` actually does?
- Can they identify dangling lambda and view bugs?
- Can they choose between `unique_ptr`, `shared_ptr`, and `weak_ptr`?
- Can they explain `optional`, `variant`, `string_view`, and `span` by
  ownership model?
- Can they compare templates with macros and runtime polymorphism?
- Can they debug a template error by identifying the missing requirement?
- Can they choose concepts over SFINAE when C++20 is available?
- Can they avoid clever advanced templates when a simple design is clearer?
