# 12 - Modern C++ And Templates

## 1. Goal

After this lesson, you should be able to:

- explain what "Modern C++" means in practical engineering terms;
- use `auto`, `decltype`, `nullptr`, range-based `for`, uniform initialization,
  `enum class`, `override`, `final`, and `static_assert` deliberately;
- write lambdas, choose capture modes, and avoid dangling captures;
- explain lvalue, rvalue, xvalue, prvalue, rvalue reference, `std::move`, and
  `std::forward`;
- design simple move-aware types and know when to prefer the Rule of Zero;
- choose between `std::unique_ptr`, `std::shared_ptr`, and `std::weak_ptr`;
- use `std::optional`, `std::variant`, `std::string_view`, and `std::span`
  with correct lifetime thinking;
- write function templates, class templates, non-type template parameters, and
  variadic templates;
- explain template instantiation, specialization, type deduction, and common
  template linker errors;
- use type traits, SFINAE, and C++20 concepts to express type requirements;
- compare templates with macros and runtime polymorphism;
- debug moved-from objects, ownership cycles, dangling views, lambda lifetime
  bugs, and template diagnostics.

This lesson uses C++17 for most examples. C++20 features such as concepts,
`std::span`, ranges, `consteval`, and `constinit` are marked when introduced.
`std::expected` is C++23 awareness.

Chapter 11, STL And Standard Library, is the main prerequisite. Chapter 10,
Resource Management In C++, is also important for smart pointers, RAII, and
move semantics.

## 2. Why It Matters

Modern C++ is not just "new syntax." It is a way to make important engineering
facts visible in code:

- Who owns this resource?
- Is this callback storing state?
- Is this value copied, moved, borrowed, or viewed?
- Can this function return no value?
- Which alternative type is active?
- Are these template arguments valid?
- Is this computation required at compile time?
- Is this interface static or dynamic polymorphism?

Older C++ often hid these answers behind raw pointers, macros, manual cleanup,
large overload sets, and comments. Modern C++ gives you vocabulary types and
language rules that let the compiler help.

The central review question is:

> Does the type system express the ownership, lifetime, and requirements that
> the code depends on?

If the answer is no, the code may still work on the happy path, but it will be
fragile under refactoring, callbacks, exceptions, larger data, and interviews.

## 3. Mental Model

Modern C++ has six connected ideas.

| Idea | What it makes explicit | Common tools |
| --- | --- | --- |
| Ownership | Who releases a resource | RAII, smart pointers |
| Value category | Can this object be moved from | lvalue/rvalue, `std::move` |
| Local behavior | Code near where it is used | lambdas |
| Vocabulary type | Meaning of an API result | `optional`, `variant`, `string_view`, `span` |
| Compile-time intent | What must be known before runtime | `constexpr`, `static_assert`, templates |
| Type requirements | What operations a generic type must support | traits, SFINAE, concepts |

The best Modern C++ code is usually not the cleverest code. It is code where
the obvious reading is also the correct lifetime and ownership reading.

## 4. Core Language Features

### 4.1 `auto` And `decltype`

`auto` asks the compiler to deduce a variable's type from its initializer.
Use it to reduce noise, especially for iterators and complex template types:

```cpp
#include <iostream>
#include <map>
#include <string>

int main()
{
    std::map<std::string, int> counts{{"error", 2}, {"warn", 5}};

    for (const auto& [name, count] : counts) {
        std::cout << name << ": " << count << '\n';
    }
}
```

The `const auto&` matters. Plain `auto` copies each element. In code review,
check whether `auto` hid ownership, reference, or constness.

`decltype(expr)` gives the declared or expression type. It is common in generic
code, type traits, and return-type deduction:

```cpp
template <typename T, typename U>
auto add(T a, U b) -> decltype(a + b)
{
    return a + b;
}
```

### 4.2 `nullptr`

Prefer `nullptr` over `0` or `NULL`. It has type `std::nullptr_t`, so overload
resolution can distinguish a null pointer from an integer:

```cpp
#include <iostream>

void open(int id)
{
    std::cout << "integer id\n";
}

void open(char* ptr)
{
    std::cout << "pointer\n";
}

int main()
{
    open(0);       // integer id
    open(nullptr); // pointer
}
```

### 4.3 Uniform Initialization

Brace initialization gives one consistent syntax and prevents many narrowing
conversions:

```cpp
#include <vector>

struct Point {
    int x;
    int y;
};

int main()
{
    Point p{10, 20};
    std::vector<int> values{1, 2, 3};

    // int bad{3.14}; // error: narrowing conversion
}
```

Be aware that classes with `std::initializer_list` constructors may prefer that
constructor when braces are used.

### 4.4 `enum class`

`enum class` avoids namespace pollution and implicit integer conversion:

```cpp
enum class State {
    Idle,
    Running,
    Fault
};

void set_state(State state);

int main()
{
    set_state(State::Running);
    // set_state(1); // error
}
```

This is very useful for protocol states, command types, mode flags, and
embedded state machines.

### 4.5 `override` And `final`

Use `override` whenever overriding a virtual function:

```cpp
struct Driver {
    virtual void start() = 0;
    virtual ~Driver() = default;
};

struct SensorDriver : Driver {
    void start() override;
};
```

If the base signature changes, the compiler catches the mismatch. Use `final`
when a class or virtual function should not be extended further.

### 4.6 `static_assert`

`static_assert` checks assumptions at compile time:

```cpp
#include <cstdint>

struct PacketHeader {
    std::uint16_t id;
    std::uint16_t length;
};

static_assert(sizeof(PacketHeader) == 4,
              "PacketHeader must match wire format");
```

Use it for protocol layouts, fixed capacities, template requirements, and
platform assumptions.

## 5. Lambdas

A lambda is an unnamed function object created inline:

```cpp
#include <algorithm>
#include <iostream>
#include <vector>

int main()
{
    std::vector<int> values{4, 1, 3, 2};

    std::sort(values.begin(), values.end(), [](int a, int b) {
        return a < b;
    });

    for (int value : values) {
        std::cout << value << ' ';
    }
}
```

The syntax is:

```cpp
[capture](parameters) mutable -> return_type {
    body
}
```

Most lambdas omit `mutable` and the explicit return type.

### 5.1 Capture Modes

| Capture | Meaning |
| --- | --- |
| `[]` | Capture nothing |
| `[x]` | Capture `x` by value |
| `[&x]` | Capture `x` by reference |
| `[=]` | Capture used local variables by value |
| `[&]` | Capture used local variables by reference |
| `[=, &x]` | Default by value, but `x` by reference |
| `[&, x]` | Default by reference, but `x` by value |
| `[this]` | Capture the current object pointer |
| `[*this]` | Capture a copy of the current object, C++17 |
| `[p = std::move(ptr)]` | Init-capture, useful for move-only objects |

Prefer explicit captures in production code. They make lifetime review much
easier.

### 5.2 Capture Lifetime Trap

Reference captures do not extend lifetime:

```cpp
#include <functional>
#include <iostream>

std::function<void()> make_bad_callback()
{
    int counter = 0;

    return [&counter] {
        std::cout << counter << '\n'; // undefined behavior later
    };
}
```

`counter` is destroyed when `make_bad_callback()` returns. The callback stores
a reference to a dead object.

Capture by value if the lambda is stored or returned:

```cpp
#include <functional>
#include <iostream>

std::function<void()> make_callback()
{
    int counter = 0;

    return [counter] {
        std::cout << counter << '\n';
    };
}
```

For move-only state, use init-capture:

```cpp
#include <memory>

auto make_task()
{
    auto data = std::make_unique<int>(42);

    return [data = std::move(data)] {
        return *data;
    };
}
```

### 5.3 Lambda vs Function Pointer vs `std::function`

| Tool | State | Typical use | Cost model |
| --- | --- | --- | --- |
| Function pointer | No captured state | C API callback, simple dispatch | Direct indirect call |
| Lambda | May capture state | STL algorithms, local callbacks | Usually zero-overhead object |
| Functor class | Explicit named state | Reusable behavior | Usually zero-overhead object |
| `std::function` | Stores many callable types | Runtime callback slot | Type erasure, possible allocation |

Non-capturing lambdas can convert to function pointers. Capturing lambdas
cannot because they carry state.

Use a template parameter when the callable type is known at compile time:

```cpp
template <typename Callback>
void repeat(int count, Callback callback)
{
    for (int i = 0; i < count; ++i) {
        callback(i);
    }
}
```

Use `std::function` when you need to store different callable types behind one
runtime interface.

## 6. Value Categories And Move Semantics

### 6.1 Lvalue And Rvalue

A simple practical model:

- an lvalue has identity; you can usually take its address;
- an rvalue is a temporary or expiring value;
- an xvalue is an expiring value with identity, commonly produced by
  `std::move`;
- a prvalue is a pure temporary value such as `42` or a returned temporary.

```cpp
#include <string>

std::string make_name()
{
    return "sensor";
}

int main()
{
    std::string a = "camera"; // a is an lvalue
    std::string b = make_name(); // function result is a prvalue
}
```

### 6.2 What `std::move` Does

`std::move` does not move anything by itself. It casts an expression to an
xvalue so a move constructor or move assignment can be selected.

```cpp
#include <iostream>
#include <string>
#include <utility>

int main()
{
    std::string source = "large payload";
    std::string target = std::move(source);

    std::cout << target << '\n';

    // source is still valid, but its value is unspecified.
    source = "reused safely";
}
```

After moving from an object, you may destroy it or assign a new value to it.
Do not write code that depends on its old contents.

### 6.3 Move-Aware Type

Manual move operations are needed only for classes that directly own resources.
Most classes should follow the Rule of Zero.

```cpp
#include <algorithm>
#include <cstddef>
#include <utility>

class Buffer {
public:
    explicit Buffer(std::size_t size)
        : size_{size}, data_{size ? new int[size] : nullptr}
    {
    }

    ~Buffer()
    {
        delete[] data_;
    }

    Buffer(const Buffer& other)
        : Buffer(other.size_)
    {
        if (size_ != 0) {
            std::copy(other.data_, other.data_ + size_, data_);
        }
    }

    Buffer& operator=(const Buffer& other)
    {
        if (this != &other) {
            Buffer copy(other);
            swap(copy);
        }
        return *this;
    }

    Buffer(Buffer&& other) noexcept
        : size_{other.size_}, data_{other.data_}
    {
        other.size_ = 0;
        other.data_ = nullptr;
    }

    Buffer& operator=(Buffer&& other) noexcept
    {
        if (this != &other) {
            delete[] data_;
            size_ = other.size_;
            data_ = other.data_;
            other.size_ = 0;
            other.data_ = nullptr;
        }
        return *this;
    }

    void swap(Buffer& other) noexcept
    {
        std::swap(size_, other.size_);
        std::swap(data_, other.data_);
    }

private:
    std::size_t size_{};
    int* data_{};
};
```

In real application code, prefer:

```cpp
#include <vector>

class Buffer {
public:
    explicit Buffer(std::size_t size)
        : data_(size)
    {
    }

private:
    std::vector<int> data_;
};
```

This version follows the Rule of Zero. `std::vector` already knows how to copy,
move, and destroy itself safely.

### 6.4 Why `noexcept` Matters

Containers care whether moving can throw. During reallocation, `std::vector`
may copy elements instead of moving them if moving might throw and copying is
available. Mark move operations `noexcept` when they truly cannot throw.

### 6.5 `std::forward` And Perfect Forwarding

Use `std::forward<T>` only in forwarding-reference wrappers:

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
    int x = 10;
    wrapper(x);  // lvalue
    wrapper(20); // rvalue
}
```

`std::move` always casts to an rvalue. `std::forward<T>` preserves whether the
original argument was an lvalue or rvalue.

## 7. Smart Pointers And Ownership Vocabulary

Smart pointers are Modern C++ vocabulary for dynamic ownership.

| Type | Meaning | Copyable | Main use |
| --- | --- | --- | --- |
| `std::unique_ptr<T>` | Exclusive ownership | No | Default owning pointer |
| `std::shared_ptr<T>` | Shared ownership | Yes | Object has multiple real owners |
| `std::weak_ptr<T>` | Non-owning observer | Yes | Break cycles, observe shared object |

### 7.1 `std::unique_ptr`

```cpp
#include <memory>
#include <string>

struct Sensor {
    std::string name;
};

std::unique_ptr<Sensor> make_sensor()
{
    return std::make_unique<Sensor>(Sensor{"imu"});
}

void consume(std::unique_ptr<Sensor> sensor)
{
    // This function owns sensor now.
}

int main()
{
    auto sensor = make_sensor();
    consume(std::move(sensor));
}
```

Passing a `unique_ptr` by value means ownership transfer. Borrow with `T&`,
`const T&`, or `T*` if the callee should not own.

### 7.2 `std::shared_ptr` And `std::weak_ptr`

Use `shared_ptr` only when shared ownership is part of the design:

```cpp
#include <memory>
#include <vector>

struct Node {
    int id{};
    std::vector<std::shared_ptr<Node>> children;
    std::weak_ptr<Node> parent;
};
```

The child list owns children. The parent pointer is weak to avoid a cycle.

Reference-count updates are thread-safe, but the pointed object is not
magically thread-safe. If several threads modify `*ptr`, protect the object
with synchronization.

### 7.3 Custom Deleters

Smart pointers can adapt C-style resources:

```cpp
#include <cstdio>
#include <memory>

using FilePtr = std::unique_ptr<FILE, int (*)(FILE*)>;

FilePtr open_log(const char* path)
{
    return FilePtr{std::fopen(path, "w"), std::fclose};
}
```

The pointer owns the `FILE*`, and `fclose` runs automatically.

## 8. Modern Vocabulary Types

### 8.1 `std::optional`

Use `std::optional<T>` when a function may return no value and no extra error
reason is needed:

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

Check before using:

```cpp
auto value = parse_positive("42");
if (value) {
    // use *value
}
```

If callers need to know why parsing failed, use an error type, an exception, or
`std::expected` in C++23.

### 8.2 `std::variant`

Use `std::variant` for a closed set of possible types:

```cpp
#include <iostream>
#include <string>
#include <variant>

using Event = std::variant<int, double, std::string>;

int main()
{
    Event event = std::string{"ready"};

    std::visit([](const auto& value) {
        std::cout << value << '\n';
    }, event);
}
```

`std::variant` is a type-safe union-like type. It tracks which alternative is
active and manages object lifetime.

### 8.3 `std::string_view`

`std::string_view` is a non-owning view of characters:

```cpp
#include <iostream>
#include <string_view>

void print_name(std::string_view name)
{
    std::cout << name << '\n';
}

int main()
{
    print_name("sensor");
}
```

It is excellent as a parameter type when the caller owns the text. It is risky
as a stored member or return value unless the owner clearly outlives the view.

Bad:

```cpp
#include <string>
#include <string_view>

std::string_view bad()
{
    std::string local = "dead soon";
    return local; // dangling view
}
```

### 8.4 `std::span` (C++20)

`std::span<T>` is a non-owning view of contiguous elements:

```cpp
#include <span>
#include <vector>

int sum(std::span<const int> values)
{
    int total = 0;
    for (int value : values) {
        total += value;
    }
    return total;
}

int main()
{
    std::vector<int> data{1, 2, 3};
    return sum(data);
}
```

Like `string_view`, `span` does not own. The referenced array or vector must
outlive the span.

## 9. Compile-Time Programming

### 9.1 `constexpr`

`constexpr` means a variable or function can be used in constant-expression
contexts when its inputs also allow compile-time evaluation:

```cpp
constexpr int packet_size(int payload)
{
    return 4 + payload;
}

static_assert(packet_size(8) == 12);
```

Use `constexpr` for typed constants and simple compile-time computations.

### 9.2 `if constexpr`

`if constexpr` chooses a branch at compile time:

```cpp
#include <iostream>
#include <type_traits>

template <typename T>
void print_kind(const T& value)
{
    if constexpr (std::is_integral_v<T>) {
        std::cout << "integer: " << value << '\n';
    } else {
        std::cout << "other\n";
    }
}
```

Branches not selected do not need to be valid for the current `T`, as long as
they are syntactically valid templates.

### 9.3 `consteval` And `constinit` (C++20)

`consteval` means every evaluated call must produce a compile-time result:

```cpp
consteval int square(int value)
{
    return value * value;
}

constexpr int size = square(8);
```

`constinit` requires static or thread-local storage to be initialized during
static initialization:

```cpp
constinit int startup_counter = 0;
```

`constinit` does not mean the variable is immutable. It is about initialization
timing, not constness.

### 9.4 Template Metaprogramming vs `constexpr`

Prefer `constexpr` functions for value computations:

```cpp
constexpr int factorial(int n)
{
    return n <= 1 ? 1 : n * factorial(n - 1);
}
```

Use template metaprogramming mainly for type-level work:

```cpp
#include <type_traits>

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;
```

Deep template metaprogramming is powerful, but it increases compile time and
can produce difficult diagnostics. Reach for it when it simplifies a real type
problem, not to look advanced.

## 10. Templates

Templates are C++'s main mechanism for generic programming. A template is a
compile-time pattern used to generate type-specific code.

### 10.1 Function Templates

```cpp
template <typename T>
T max_value(T a, T b)
{
    return a < b ? b : a;
}

int main()
{
    int i = max_value(3, 7);
    double d = max_value(2.5, 1.5);
}
```

The compiler instantiates separate functions for the types used.

### 10.2 Class Templates

```cpp
#include <array>
#include <cstddef>
#include <stdexcept>

template <typename T, std::size_t Capacity>
class FixedBuffer {
public:
    bool push(const T& value)
    {
        if (size_ == Capacity) {
            return false;
        }
        data_[size_++] = value;
        return true;
    }

    const T& at(std::size_t index) const
    {
        if (index >= size_) {
            throw std::out_of_range("FixedBuffer index");
        }
        return data_[index];
    }

    std::size_t size() const
    {
        return size_;
    }

private:
    std::array<T, Capacity> data_{};
    std::size_t size_{};
};

static_assert(sizeof(FixedBuffer<int, 4>) >= sizeof(int) * 4);
```

`Capacity` is a non-type template parameter. It is known at compile time, so
`FixedBuffer<int, 4>` and `FixedBuffer<int, 8>` are different types.

### 10.3 Type Deduction

Template type deduction follows rules:

- by value drops top-level `const` and references;
- by reference preserves constness;
- arrays and functions may decay depending on the parameter form.

```cpp
template <typename T>
void by_value(T value);

template <typename T>
void by_ref(const T& value);
```

Use `const T&` for read-only generic borrowing and `T&&` only when you intend
to forward or distinguish value categories.

### 10.4 Template Definitions And Linker Errors

Template definitions usually need to be visible at the point where they are
instantiated. This is why many templates live in headers.

If you put only this in a header:

```cpp
template <typename T>
T add(T a, T b);
```

and define the template only in a `.cpp` file, another translation unit may
compile but fail to link. Either keep the definition in the header or use
explicit instantiation intentionally.

### 10.5 Specialization

Specialization provides custom behavior for specific template arguments:

```cpp
#include <cstring>

template <typename T>
bool less_than(T a, T b)
{
    return a < b;
}

template <>
bool less_than<const char*>(const char* a, const char* b)
{
    return std::strcmp(a, b) < 0;
}
```

Function templates cannot be partially specialized. Use overloads, class
template partial specialization, traits, or concepts depending on the problem.

## 11. Variadic Templates And Fold Expressions

Variadic templates accept a variable number of template arguments.

```cpp
#include <iostream>

template <typename... Args>
void print_all(const Args&... args)
{
    ((std::cout << args << ' '), ...);
    std::cout << '\n';
}

int main()
{
    print_all("id", 42, "ready");
}
```

`Args...` is a template parameter pack. `args...` is a function parameter pack.
The fold expression applies the comma operation to each argument.

Before C++17, many variadic algorithms used recursive template functions. In
C++17 and later, use fold expressions for simple repeated operations.

Perfect forwarding often combines variadic templates with `std::forward`:

```cpp
#include <memory>
#include <utility>

template <typename T, typename... Args>
std::unique_ptr<T> make_owned(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}
```

## 12. Type Traits, SFINAE, And Concepts

### 12.1 Type Traits

Type traits answer questions about types at compile time:

```cpp
#include <iostream>
#include <type_traits>

template <typename T>
void describe()
{
    if constexpr (std::is_integral_v<T>) {
        std::cout << "integral\n";
    } else if constexpr (std::is_floating_point_v<T>) {
        std::cout << "floating point\n";
    } else {
        std::cout << "other\n";
    }
}
```

Traits also transform types:

```cpp
#include <type_traits>

template <typename T>
using plain_t = std::remove_cv_t<std::remove_reference_t<T>>;
```

### 12.2 SFINAE

SFINAE means "Substitution Failure Is Not An Error." During template overload
resolution, if substituting a type into a template makes that candidate invalid,
the compiler can remove that candidate instead of failing immediately.

```cpp
#include <iostream>
#include <type_traits>

template <typename T>
std::enable_if_t<std::is_integral_v<T>, void>
process(T value)
{
    std::cout << "integer: " << value << '\n';
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, void>
process(T value)
{
    std::cout << "float: " << value << '\n';
}
```

SFINAE is useful in pre-C++20 generic libraries, but it can be verbose and hard
to debug.

### 12.3 Concepts (C++20)

Concepts express template requirements directly:

```cpp
#include <concepts>
#include <iostream>

template <typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template <Numeric T>
T twice(T value)
{
    return value + value;
}

int main()
{
    std::cout << twice(21) << '\n';
}
```

Concepts usually produce clearer compiler errors than SFINAE and document the
API contract where the template is declared.

### 12.4 Custom Concept With `requires`

```cpp
#include <concepts>
#include <iostream>
#include <vector>

template <typename T>
concept SizedRange = requires(const T& value) {
    value.begin();
    value.end();
    { value.size() } -> std::convertible_to<std::size_t>;
};

template <SizedRange Range>
void print_size(const Range& range)
{
    std::cout << range.size() << '\n';
}

int main()
{
    std::vector<int> values{1, 2, 3};
    print_size(values);
}
```

Use concepts for public generic APIs when C++20 is available. Use named traits
or `static_assert` to improve diagnostics in C++17.

## 13. Advanced Template Topics In Controlled Depth

### 13.1 Alias Templates

Alias templates create readable names for template families:

```cpp
#include <memory>
#include <vector>

template <typename T>
using UniqueVector = std::unique_ptr<std::vector<T>>;
```

Use them to simplify complex types or express domain vocabulary.

### 13.2 CTAD And Deduction Guides (C++17)

Class Template Argument Deduction lets the compiler deduce class template
arguments from constructors:

```cpp
#include <utility>
#include <vector>

int main()
{
    std::pair p{1, 2.5};       // std::pair<int, double>
    std::vector v{1, 2, 3};    // std::vector<int>
}
```

Custom deduction guides are useful for library types, but most application code
uses standard CTAD as-is.

### 13.3 Dependent Names

In templates, some names depend on template parameters. The compiler may need
help to know whether a dependent name is a type or a template:

```cpp
template <typename Container>
void first_element(Container& container)
{
    typename Container::iterator it = container.begin();
    (void)it;
}
```

Use `typename` for dependent types. Use the `template` keyword when calling a
dependent member template:

```cpp
template <typename T>
void call(T& object)
{
    object.template run<int>();
}
```

These keywords are noisy, but they point to real parsing ambiguity.

### 13.4 Template Template Parameters, CRTP, And Policy Design

Template-template parameters let you pass a template itself:

```cpp
#include <memory>

template <template <typename> class Ptr>
class ResourceManager {
public:
    Ptr<int> value;
};

int main()
{
    ResourceManager<std::shared_ptr> manager;
    manager.value = std::make_shared<int>(42);
}
```

This can be useful for policy families, but it is often overkill. Prefer a
normal type parameter unless the design really needs a template family.

CRTP and policy-based design are static-polymorphism tools. They can remove
virtual dispatch and encode behavior at compile time, but they also increase
template complexity. Use them when the performance or type-safety benefit is
clear.

## 14. C Usage And C++ Usage

C has different tools for similar problems:

| Problem | C style | Modern C++ style |
| --- | --- | --- |
| Generic code | Macros, `void*`, generated functions | Templates, concepts |
| Callback with state | Function pointer plus `void*` context | Lambda, functor, `std::function` |
| Optional value | Nullable pointer, status code | `std::optional`, `std::expected` |
| Variant data | Tagged union | `std::variant` |
| Resource cleanup | Manual cleanup paths | RAII, smart pointers |
| Constants | `#define`, `const`, enum | `constexpr`, `enum class`, templates |

C is still important for ABI, embedded systems, and POSIX APIs. The point is
not "C bad, C++ good." The point is that C++ can encode more intent in types,
so you should use that when writing C++.

## 15. Practical Usage

### 15.1 Embedded And Systems-Oriented C++

Modern C++ can help embedded and systems code when used with discipline:

- `enum class` for modes and states;
- `constexpr` for register masks, packet sizes, and compile-time tables;
- `static_assert` for layout assumptions;
- fixed-size templates for bounded buffers;
- `unique_ptr` with custom deleters for host-side handles;
- lambdas for local callbacks and test fakes;
- `span` for non-owning buffer parameters in C++20.

Be careful with:

- unbounded dynamic allocation;
- exceptions if the project disables them;
- `shared_ptr` in hot paths;
- template bloat in small firmware images;
- stored lambdas that capture references.

### 15.2 Enterprise C++

In larger codebases, Modern C++ helps APIs communicate intent:

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

From signatures alone:

- `create_device` returns ownership;
- `find_cached_device` may not find a value;
- `inspect_device` borrows a required object;
- `string_view` says the function does not need to own the input text.

That is the real win: fewer ownership and lifetime rules live only in comments.

## 16. Required Comparisons

### 16.1 `constexpr` vs `const` vs Macro

| Feature | Meaning | Type-safe | Scope-aware | Typical use |
| --- | --- | --- | --- | --- |
| Macro | Preprocessor token substitution | No | No | Conditional compilation, include guards |
| `const` | Object cannot be modified through this name | Yes | Yes | Runtime or compile-time read-only object |
| `constexpr` | Usable in constant-expression contexts when valid | Yes | Yes | Compile-time constants and functions |

Prefer `constexpr` for typed constants:

```cpp
constexpr int max_packet_size = 256;
```

Use macros only when you truly need the preprocessor.

### 16.2 Template vs Macro

```cpp
#define MAX(a, b) ((a) < (b) ? (b) : (a))

template <typename T>
T max_value(T a, T b)
{
    return a < b ? b : a;
}
```

Macros can evaluate arguments more than once and ignore types. Templates are
type-checked, scoped, and integrated with overload resolution.

### 16.3 Move vs Copy

| Operation | Meaning | Source after operation |
| --- | --- | --- |
| Copy | Duplicate value/resource | unchanged |
| Move | Transfer resource when possible | valid, value generally unspecified |

Use move for ownership transfer and expensive resources. Do not move just to
look modern.

### 16.4 `std::optional` vs Nullable Pointer

| Tool | Owns value | Can be null/no value | Meaning |
| --- | --- | --- | --- |
| `std::optional<T>` | Yes | Yes | Maybe a value |
| `T*` | Usually no | Yes | Maybe an object elsewhere |
| `std::unique_ptr<T>` | Yes | Yes | Maybe owns dynamic object |

Use `optional<T>` when absence is about value, not object identity or dynamic
ownership.

### 16.5 `std::variant` vs Union

| Tool | Tracks active type | Manages non-trivial lifetime | Typical use |
| --- | --- | --- | --- |
| C/C++ union | Manual | Manual or restricted | Low-level storage, C ABI |
| `std::variant` | Yes | Yes | Type-safe closed alternatives |

Prefer `variant` for application-level alternatives. Use unions only when you
need low-level layout or C compatibility and can enforce the invariant.

### 16.6 `std::string_view` vs `std::string`

| Type | Owns characters | Can dangle | Typical use |
| --- | --- | --- | --- |
| `std::string` | Yes | No, for its own data | Store or return text |
| `std::string_view` | No | Yes | Borrow text parameter or substring |

Return `std::string` when ownership is unclear. Return `string_view` only when
the referenced text clearly outlives the view.

### 16.7 Template vs Runtime Polymorphism

| Aspect | Templates | Virtual functions |
| --- | --- | --- |
| Binding | Compile time | Runtime |
| Cost | No virtual dispatch, possible code bloat | Indirect call, stable object interface |
| Errors | Compile-time instantiation diagnostics | Interface contract through base class |
| Best for | Generic algorithms, containers, zero-overhead customization | Runtime substitution, plugins, ABI boundaries |

Use templates for type-generic code known at compile time. Use runtime
polymorphism when the selected implementation changes at runtime or crosses an
interface boundary.

### 16.8 Concepts vs SFINAE

| Aspect | SFINAE | Concepts |
| --- | --- | --- |
| Standard | C++11 and later patterns | C++20 |
| Syntax | Verbose | Direct |
| Diagnostics | Often cryptic | Usually clearer |
| Best use | Legacy support, detection idioms | Public generic API constraints |

If C++20 is available, prefer concepts for new code.

## 17. Common Bugs

- Capturing local variables by reference in a lambda that is returned, stored,
  or used asynchronously.
- Capturing `this` when the lambda may run after the object is destroyed.
- Using `std::function` in a hot path where a template callable would avoid
  type erasure.
- Assuming `std::move` moved something by itself.
- Reading a moved-from object as if it still had its old value.
- Moving from a `const` object and expecting a real move.
- Writing `return std::move(local);` and blocking natural copy elision.
- Forgetting `noexcept` on move operations.
- Creating two independent `shared_ptr` objects from one raw pointer.
- Creating `shared_ptr` cycles and forgetting `weak_ptr`.
- Storing raw pointers from `smart_ptr.get()` after the owner dies.
- Calling `unique_ptr::release()` and forgetting manual cleanup.
- Returning `std::string_view` to a temporary string.
- Calling `optional.value()` without checking.
- Calling `std::get<T>` on the wrong `variant` alternative.
- Hiding copies with plain `auto`.
- Defining templates only in `.cpp` files and getting linker errors.
- Assuming function templates can be partially specialized.
- Writing unconstrained templates that fail with unreadable errors.
- Forgetting `typename` or dependent `template` in advanced template code.
- Overusing advanced templates when a simple overload or class would be clearer.

## 18. Debugging

For lambda lifetime bugs:

- inspect the capture list first;
- check whether the lambda is stored, returned, copied, or scheduled;
- replace `[&]` with explicit captures;
- capture by value or move if lifetime is not obvious.

For move bugs:

- add logging to copy/move constructors in a small reproduction;
- check whether the type's move constructor is `noexcept`;
- check whether the source object is used after move;
- remove unnecessary `std::move` from return statements.

For smart pointer bugs:

- draw the ownership graph;
- mark `unique_ptr` owners, `shared_ptr` owners, and `weak_ptr` observers;
- look for cycles;
- avoid making ownership decisions based on `use_count()` in real logic.

For template bugs:

- start at the first line of user code that instantiated the template;
- identify the required operation that failed, such as `a < b` or `t.begin()`;
- add `static_assert` or a named concept near the API boundary;
- reduce the problem to a small compile-only example.

Useful tools:

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic example.cpp
clang++ -std=c++20 -fsanitize=address,undefined example.cpp
clang-tidy example.cpp -- -std=c++20
```

Use AddressSanitizer/UndefinedBehaviorSanitizer for lifetime and ownership
mistakes. Use compiler diagnostics and concepts for template constraints.

## 19. Best Practices

- Prefer clear ownership types over comments about ownership.
- Prefer `unique_ptr` for dynamic ownership; use `shared_ptr` only for real
  shared ownership.
- Use `weak_ptr` for back-references and observer links in shared graphs.
- Prefer Rule of Zero. Write special member functions only when directly owning
  a raw resource.
- Mark move operations `noexcept` when correct.
- Use `std::move` only when giving up the current value.
- Use `std::forward<T>` only in forwarding-reference templates.
- Prefer explicit lambda captures.
- Avoid reference captures in stored or async lambdas unless lifetime is proven.
- Use `std::function` for runtime callable storage, not every callback.
- Use `constexpr` instead of macros for typed constants and simple compile-time
  functions.
- Use `optional` for maybe-values and `variant` for closed alternatives.
- Use `string_view` and `span` mainly as parameter types.
- Constrain templates at API boundaries.
- Prefer concepts in C++20 and later.
- Prefer `constexpr` functions over template metaprogramming for value
  computations.
- Keep advanced template techniques rare, documented, and justified.

## 20. Interview Readiness

Be ready to answer these cleanly.

### Junior

- What is Modern C++?
- Why is `nullptr` better than `NULL`?
- What is a lambda?
- What is capture by value vs capture by reference?
- What is a smart pointer?
- What is the difference between `unique_ptr` and `shared_ptr`?
- What does `std::move` do?
- What is a template?
- Why are templates safer than macros?

### Middle

- Explain lvalue, rvalue, and rvalue reference.
- What is a moved-from object allowed to do?
- Why does `noexcept` matter for move operations?
- When do you use `weak_ptr`?
- Why can `string_view` dangle?
- Compare `optional` with nullable pointer.
- Compare `variant` with union.
- Compare lambda, function pointer, and `std::function`.
- Explain template instantiation and type deduction.
- What is SFINAE?
- What are C++20 concepts?

### Senior

- Design a callback API that avoids dangling references and ownership cycles.
- Explain `std::move` vs `std::forward`.
- Diagnose a long template error from an unconstrained template.
- Explain dependent names and two-phase lookup.
- Discuss template code-size and build-time tradeoffs.
- When would you choose runtime polymorphism over templates?
- How would you migrate a SFINAE-heavy API to concepts?
- Explain `constexpr`, `consteval`, and `constinit`.
- Design a `string_view` or `span` API with safe lifetime rules.

## 21. Practice

### Basic

- Replace a `#define MAX(a, b)` macro with a function template.
- Write a `constexpr` function that computes a packet size.
- Sort a vector of structs using a lambda comparator.
- Write a function that takes `std::string_view` and prints a message.

### Intermediate

- Implement `FixedBuffer<T, N>` with `std::array`, `push`, `at`, and
  `static_assert` checks.
- Write a small program that logs copy constructor, move constructor, and move
  assignment calls.
- Build a callback list using lambdas. Show one safe value capture and one
  unsafe reference capture.
- Write a parser returning `std::optional<int>`.
- Use `std::variant` and `std::visit` to handle command events.

### Advanced

- Write `print_all(args...)` once with recursion and once with a fold
  expression.
- Write a C++17 trait that detects `begin()`, `end()`, and `size()`.
- Convert an `std::enable_if` overload pair to C++20 concepts.
- Create a small example where an unconstrained template gives a bad error,
  then improve it with `static_assert` or a concept.
- Compare a template-based strategy with a virtual-interface strategy for a
  small logger or protocol handler.

## 22. Summary

Modern C++ is about making important design facts visible:

- RAII and smart pointers express ownership.
- Lambdas express local behavior and callbacks.
- Move semantics avoid unnecessary resource duplication.
- Vocabulary types express absence, alternatives, and borrowed views.
- `constexpr` and templates move valid work to compile time.
- Concepts express generic requirements directly.

The practical rule is simple:

> Use the modern feature when it makes ownership, lifetime, type requirements,
> or intent clearer. Avoid it when it only makes the code look clever.

## 23. Reference Notes

- `std::span`, concepts, ranges, `consteval`, and `constinit` are C++20.
- `std::expected` is C++23.
- `std::string_view` is C++17 and does not own its characters.
- A moved-from standard-library object is valid, but its value is generally
  unspecified unless the specific type documents more.
- C++ templates are usually defined in headers because template definitions
  must be visible when the compiler instantiates them, unless explicit
  instantiation is used intentionally.
