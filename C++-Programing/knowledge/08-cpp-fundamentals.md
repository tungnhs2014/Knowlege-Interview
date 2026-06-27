# 08 - C++ Fundamentals

## 1. Goal

C++ fundamentals are not only new syntax added to C. They introduce language
mechanisms for expressing initialization, object lifetime, invariants,
interfaces, and cleanup.

After this chapter, you should be able to:

- explain important differences between C and C++;
- distinguish scope, storage duration, linkage, and lifetime;
- initialize objects deliberately and avoid narrowing;
- choose between values, references, `const` references, and pointers;
- design a small class with a valid invariant;
- explain constructors, member initializer lists, copy construction, and
  destructors;
- compare `struct` and `class` in C++;
- use namespaces without polluting headers;
- reason about function overloading and ambiguity;
- explain basic operator overloading;
- use static members and friendship carefully;
- explain `inline`, name mangling, and `extern "C"` accurately;
- recognize lifetime bugs in returned references and lambda captures;
- compile and debug small multi-file C++ programs.

This chapter uses C++17 as its main baseline. Newer features are mentioned only
when they clarify the direction of modern C++.

## 2. Why It Matters

C code often expresses contracts through comments and naming:

```c
typedef struct {
    int channel;
    int sample_period_ms;
} SensorConfig;

bool sensor_config_init(
    SensorConfig *config,
    int channel,
    int sample_period_ms);
```

The caller must create storage, call the correct initialization function, check
the result, avoid using the object too early, and remember any required cleanup.

C++ can move some of those rules into the type:

```cpp
class SensorConfig {
public:
    SensorConfig(int channel, int sample_period_ms);

    int channel() const { return channel_; }
    int sample_period_ms() const { return sample_period_ms_; }

private:
    int channel_;
    int sample_period_ms_;
};
```

Construction becomes part of creating the object. Private data prevents callers
from directly breaking the invariant. Destruction can automatically release
owned resources when lifetime ends.

These mechanisms matter in production because they reduce the number of states
that reviewers, tests, and maintainers must consider.

## 3. Mental Model: Type, Object, And Lifetime

Keep four ideas separate:

```text
Type
  Defines valid operations and representation requirements.

Storage
  Memory in which an object may exist.

Object
  An entity of a type whose lifetime has begun.

Lifetime
  The period during which the object exists and may be used as that object.
```

Storage alone is not always a live object. Likewise, an address can remain in a
pointer after the object's lifetime ends. The pointer still contains a value,
but dereferencing it is invalid.

For a normal local class object:

```text
storage obtained
      |
members and bases initialized
      |
constructor body executes
      |
object is used
      |
destructor body executes
      |
members and bases destroyed
      |
storage becomes reusable
```

C++ design becomes easier when you ask:

1. What invariant must be true after construction?
2. Who owns each resource?
3. Which references and pointers are only borrowed?
4. What event ends each object's lifetime?
5. What cleanup must happen at that point?

## 4. Translation, Declarations, And Definitions

A C++ program is commonly built from multiple translation units. A translation
unit is approximately one source file after preprocessing.

```text
header declarations
        |
source translation units
        |
object files
        |
linker
        |
program or library
```

A declaration introduces a name and type:

```cpp
int read_sensor(int channel);
```

A definition supplies the function body:

```cpp
int read_sensor(int channel)
{
    return channel * 10;
}
```

Definitions must obey the One Definition Rule (ODR). Some entities, including
properly defined `inline` functions, have specific permission to appear in more
than one translation unit. This is not an exemption from the ODR.

Compile a simple program with explicit warnings and a language version:

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
    -Wold-style-cast -Werror main.cpp -o app
```

Flags vary by compiler. A project must test its warning policy on every
supported toolchain.

## 5. Scope, Storage Duration, Linkage, And Lifetime

These terms answer different questions.

| Property | Question |
| --- | --- |
| Scope | Where can this name be used? |
| Storage duration | How long does the object's storage last? |
| Linkage | Can declarations in different scopes or translation units denote the same entity? |
| Lifetime | When does the object itself exist? |

Example:

```cpp
namespace telemetry {
int packet_count = 0; // namespace scope, static storage duration

void record_packet()
{
    static int calls = 0; // block scope, static storage duration
    int bytes = 8;        // block scope, automatic storage duration

    ++calls;
    packet_count += bytes;
}
}
```

`calls` is visible only inside `record_packet`, but its value persists across
calls. `bytes` is created each time control reaches its declaration and is
destroyed when its scope exits.

Avoid unnecessary mutable global state. It creates hidden dependencies and
makes tests, concurrency, and initialization order harder to reason about.

## 6. Initialization And Fundamental Values

Initialization gives an object its initial value. Prefer creating an object in a
valid state instead of declaring it and assigning later.

```cpp
int uninitialized; // indeterminate value
int zero{};         // value-initialized to zero
int count{4};       // direct-list-initialized
double voltage{3.3};
```

Reading an indeterminate value of an ordinary local integer is erroneous or
undefined depending on the language version and context. In practical C++17
code, treat it as a serious correctness bug.

### 6.1 List Initialization And Narrowing

List initialization rejects many narrowing conversions:

```cpp
int samples{42};
// int invalid{3.5}; // compile error: narrowing

double measured = 3.5;
int truncated = static_cast<int>(measured); // explicit, still needs range policy
```

`static_cast` communicates intent, but it does not prove that the value is in
range or that truncation is acceptable.

Braces are useful, but they have an important edge case: when a class has a
`std::initializer_list` constructor, brace initialization may prefer it over
other overloads. Learn that mechanism before assuming `{}` and `()` are always
interchangeable.

### 6.2 `const` And `constexpr`

`const` restricts modification through an access path:

```cpp
const int max_attempts = load_configured_limit();
```

The initializer may be known only at runtime.

`constexpr` requires constant-expression capability:

```cpp
constexpr int channels = 4;

constexpr int square(int value)
{
    return value * value;
}

static_assert(square(channels) == 16);
```

A `constexpr` function can also execute at runtime:

```cpp
int runtime_value = 0;
std::cin >> runtime_value;
int result = square(runtime_value);
```

Therefore:

- `const` does not mean “runtime only”;
- `constexpr` does not mean every call is evaluated during compilation.

### 6.3 `auto`, `nullptr`, And `enum class`

`auto` asks the compiler to deduce a type:

```cpp
const int threshold = 10;
auto copy = threshold;        // int
const auto fixed = threshold; // const int
auto& alias = threshold;      // const int&
```

Do not use `auto` when hiding the type makes ownership, units, or conversions
unclear.

Use `nullptr` for a null pointer:

```cpp
int *optional_value = nullptr;
```

Use a scoped enumeration to avoid accidental integer conversion and name
collisions:

```cpp
enum class SensorState {
    idle,
    sampling,
    failed
};
```

## 7. References

A reference is initialized to refer to an existing object or function. After
initialization, it cannot be reseated to refer to a different object.

The `T&` form introduced here is an **lvalue reference**. An lvalue normally
identifies an object with a persistent identity, such as a named variable.

```cpp
int first = 10;
int second = 20;

int& selected = first;
selected = second; // assigns 20 to first; it does not rebind selected
```

References are often implemented using addresses, but “a reference is a const
pointer” is not the language model. References are aliases with their own
initialization and binding rules.

### 7.1 Mutable And Const References

Use `T&` when a function requires an existing object and intentionally mutates
it:

```cpp
void clamp_to_zero(int& value)
{
    if (value < 0) {
        value = 0;
    }
}
```

Use `const T&` for required borrowed read-only access when copying is not the
desired contract:

```cpp
#include <string>

bool is_empty(const std::string& text)
{
    return text.empty();
}
```

A const reference can bind to a temporary in many contexts:

```cpp
const std::string& text = std::string{"ready"};
```

Here, the local reference extends the temporary's lifetime to the reference's
lifetime. Temporary lifetime extension has context-specific rules; do not assume
that storing a reference member or returning a reference extends a temporary.

### 7.2 Pointer Versus Reference

```cpp
void reset_required(Device& device);  // required borrowed object
void reset_optional(Device *device);  // nullable borrowed pointer
```

| Question | Reference | Pointer |
| --- | --- | --- |
| Must be initialized? | Yes | No |
| Can represent no object normally? | No | Yes, with `nullptr` |
| Can be reseated? | No | Yes |
| Is ownership implied? | No | No |
| Supports pointer arithmetic? | No | Yes, when valid |

A pointer or reference can still dangle. Neither owns an object automatically.

### 7.3 Parameter Choice

Use the contract, not a guessed machine cost:

| Form | Typical meaning |
| --- | --- |
| `T value` | Independent value, cheap scalar, or intentional copy/move |
| `T& value` | Required mutable borrowed object |
| `const T& value` | Required read-only borrowed object |
| `T* value` | Nullable/address-oriented borrowed access |
| `const T* value` | Nullable read-only borrowed access |

Passing by value does not promise that bytes are copied onto a hardware stack.
The implementation may use registers, copying, moving, or elision.

## 8. Classes And Objects

A class defines a user-defined type. It can combine state, behavior, access
control, and invariants.

```cpp
#include <stdexcept>

class SensorConfig {
public:
    explicit SensorConfig(int channel, int sample_period_ms)
        : channel_{channel},
          sample_period_ms_{sample_period_ms}
    {
        if (channel_ < 0 || sample_period_ms_ <= 0) {
            throw std::invalid_argument{"invalid sensor configuration"};
        }
    }

    int channel() const
    {
        return channel_;
    }

    int sample_period_ms() const
    {
        return sample_period_ms_;
    }

private:
    int channel_;
    int sample_period_ms_;
};
```

After successful construction, every `SensorConfig` object has a non-negative
channel and positive period. Callers cannot directly assign invalid values to
private members.

Whether exceptions are enabled is a project policy. Embedded projects that
disable exceptions need a different creation protocol, such as validation
before construction, a factory returning a status/result, or a design whose
input domain cannot produce an invalid object.

### 8.1 Access Specifiers

- `public`: the supported interface available to callers;
- `private`: implementation details available to the class and its friends;
- `protected`: available to the class, friends, and applicable derived classes.

`private` is not secrecy. It is a compile-time mechanism that helps a type own
its invariants.

### 8.2 `struct` Versus `class`

In C++, both can have constructors, destructors, methods, static members,
operators, friends, templates, and inheritance.

The language differences are defaults:

| Declaration | Default member access | Default base access |
| --- | --- | --- |
| `struct` | `public` | `public` |
| `class` | `private` | `private` |

Conventionally:

- use `struct` for a simple passive aggregate with public state;
- use `class` when the type owns an invariant behind an interface.

This is a design convention, not an OOP capability rule.

An aggregate can be initialized member by member with braces:

```cpp
struct Point {
    int x;
    int y;
};

Point origin{0, 0}; // aggregate initialization
```

Whether a type is an aggregate is determined by language rules that have
changed across C++ versions. Do not use “struct” and “aggregate” as synonyms.

## 9. Constructors And Initialization

A constructor initializes an object. It has the class name and no return type.

```cpp
class Counter {
public:
    Counter() = default;               // default constructor
    explicit Counter(int initial)      // parameterized constructor
        : value_{initial}
    {
    }

private:
    int value_{0}; // default member initializer
};
```

### 9.1 Member Initializer Lists

Members are initialized before the constructor body executes:

```cpp
class Sample {
public:
    Sample(int channel, double value)
        : channel_{channel},
          value_{value}
    {
    }

private:
    int channel_;
    double value_;
};
```

This is initialization, not assignment. It is required for references,
`const` members, and members without a default constructor.

### 9.2 Initialization Order

Members initialize in declaration order, not the order written in the
initializer list:

```cpp
class Limits {
public:
    Limits(int minimum, int span)
        : minimum_{minimum},
          maximum_{minimum_ + span}
    {
    }

private:
    int minimum_;
    int maximum_;
};
```

Write initializer lists in declaration order. Enable reorder warnings.

### 9.3 Delegating Constructors

One constructor can delegate to another constructor of the same class:

```cpp
class Timeout {
public:
    Timeout()
        : Timeout{1000}
    {
    }

    explicit Timeout(int milliseconds)
        : milliseconds_{milliseconds}
    {
    }

private:
    int milliseconds_;
};
```

Delegation centralizes initialization policy.

### 9.4 Converting Constructors And `explicit`

A constructor callable with one argument can participate in implicit
conversion:

```cpp
class Milliseconds {
public:
    explicit Milliseconds(int value)
        : value_{value}
    {
    }

private:
    int value_;
};

void sleep_for(Milliseconds duration);

// sleep_for(100);              // rejected
sleep_for(Milliseconds{100});   // intent is explicit
```

Mark converting constructors `explicit` unless implicit conversion is a clear,
unsurprising part of the type's meaning.

### 9.5 Copy Construction And Copy Elision

A copy constructor creates an object from another object of the same type:

```cpp
class Sequence {
public:
    explicit Sequence(int value)
        : value_{value}
    {
    }

private:
    int value_;
};

Sequence first{7};
Sequence second{first};
```

When every member supports copying, the compiler can often generate correct
memberwise copy behavior.

Avoid tests that require an exact number of copy/move constructor calls. C++
permits copy elision, and C++17 guarantees elision in important prvalue cases.
Test the resulting value and ownership, not incidental constructor counts.

## 10. Destructors And Deterministic Cleanup

A destructor runs when an object's lifetime ends in the applicable context:

```cpp
#include <cstdio>

class File {
public:
    explicit File(const char *path)
        : handle_{std::fopen(path, "rb")}
    {
    }

    ~File()
    {
        if (handle_ != nullptr) {
            std::fclose(handle_);
        }
    }

    bool is_open() const
    {
        return handle_ != nullptr;
    }

private:
    std::FILE *handle_;
};
```

This demonstrates deterministic cleanup, but it is not yet production-ready:
the implicitly generated copy operations would copy the raw handle and could
cause double close. Resource-owning copy/move design belongs to the resource
management chapter.

For ordinary local objects, destruction happens when scope exits, including
early returns and exception unwinding. Members are destroyed in reverse
declaration order after the destructor body.

### 10.1 C Cleanup Versus C++ Destruction

C:

```c
FILE *file = fopen(path, "rb");
if (file == NULL) {
    return ERROR_OPEN;
}

if (!process(file)) {
    fclose(file);
    return ERROR_PROCESS;
}

fclose(file);
```

C++ ties cleanup to object lifetime. That can remove repeated cleanup branches,
but only when ownership and copy/move behavior are correctly designed.

## 11. Functions And Overloading

Function overloading allows multiple functions with the same name when their
parameter lists form distinct overloads:

```cpp
#include <string>

std::string format(int value)
{
    return std::to_string(value);
}

std::string format(double value)
{
    return std::to_string(value);
}
```

The compiler:

1. forms a candidate set;
2. removes candidates that are not viable;
3. ranks required conversion sequences;
4. requires one best viable function.

An exact match normally ranks better than a promotion, which normally ranks
better than a general standard conversion. The full rules include references,
list initialization, templates, user-defined conversions, and tie-breakers.

### 11.1 Ambiguity

```cpp
void report(int);
void report(double);

long value = 1;
// report(value); // may be ambiguous: both require standard conversion
```

Do not automatically repair every ambiguous API with casts at call sites.
Consider distinct names or a type-specific overload when the intended meaning
is not naturally obvious.

Return type alone cannot distinguish ordinary function overloads:

```cpp
// int read();
// double read(); // invalid: differs only by return type
```

### 11.2 Overloading Versus Overriding

- Overloading: multiple declarations share a name and differ in parameters.
- Overriding: a derived class replaces virtual behavior from a base class.

Overriding belongs to the next OOP chapter.

### 11.3 C Naming Convention

C cannot overload functions, so APIs use separate names:

```c
int parse_i32(const char *text, int *out_value);
int parse_f64(const char *text, double *out_value);
```

C++ can use one coherent operation name:

```cpp
bool parse(const char *text, int& out_value);
bool parse(const char *text, double& out_value);
```

Use overloading only when the functions represent the same conceptual
operation.

## 12. Operator Overloading

An overloaded operator is a function selected using operator syntax.

```cpp
#include <limits>
#include <stdexcept>

struct Millivolts {
    int value;
};

constexpr Millivolts operator+(
    Millivolts left,
    Millivolts right)
{
    if ((right.value > 0
            && left.value > std::numeric_limits<int>::max() - right.value)
        || (right.value < 0
            && left.value < std::numeric_limits<int>::min() - right.value)) {
        throw std::overflow_error{"millivolt addition overflow"};
    }

    return Millivolts{left.value + right.value};
}
```

Usage:

```cpp
constexpr Millivolts first{1200};
constexpr Millivolts second{300};
static_assert((first + second).value == 1500);
```

The range checks happen before addition, so they do not themselves overflow.
An out-of-range runtime sum throws `std::overflow_error`; an out-of-range
constant expression fails constant evaluation. Exception-free projects should
use a checked result type or a separate `try_add` API instead.

Operator overloading cannot:

- create a new operator token;
- change precedence or associativity;
- change the operator's operand count;
- redefine behavior when every operand has a built-in type.

Use an operator only when its meaning matches normal expectations. `+` should
not modify a database, send a packet, or perform hidden logging.

## 13. Static Members And Friendship

### 13.1 Static Members

A static data member belongs to the class rather than to each object:

```cpp
class Connection {
public:
    Connection()
    {
        ++active_count_;
    }

    Connection(const Connection&) = delete;
    Connection& operator=(const Connection&) = delete;

    ~Connection()
    {
        --active_count_;
    }

    static int active_count()
    {
        return active_count_;
    }

private:
    inline static int active_count_{0}; // C++17
};
```

This example is not thread-safe. Concurrent construction/destruction would need
appropriate synchronization.

A static member function has no `this` pointer and can directly access only
static members.

### 13.2 Friend Functions And Friend Classes

Friendship grants selected code access to private/protected members:

```cpp
class Temperature {
public:
    explicit Temperature(int milli_celsius)
        : milli_celsius_{milli_celsius}
    {
    }

    friend bool operator==(
        const Temperature& left,
        const Temperature& right);

private:
    int milli_celsius_;
};

bool operator==(
    const Temperature& left,
    const Temperature& right)
{
    return left.milli_celsius_ == right.milli_celsius_;
}
```

A friend function is not a member function. Friendship is not inherited and is
not transitive.

A friend class grants every member of that class access. Use it sparingly;
broad friendship can couple implementations and bypass a weak public interface.

## 14. Namespaces And Name Lookup

Namespaces group names and prevent collisions:

```cpp
namespace telemetry {
struct Packet {
    int id;
};

void send(const Packet& packet)
{
    // Send packet.
}
}
```

Use explicit qualification:

```cpp
telemetry::Packet packet{42};
telemetry::send(packet);
```

A using declaration imports one name:

```cpp
using telemetry::Packet;
Packet packet{42};
```

A using directive makes all names from a namespace available to unqualified
lookup:

```cpp
using namespace telemetry;
```

Do not put broad using directives in headers. Every including translation unit
would inherit those lookup effects and possible collisions.

### 14.1 Unnamed Namespaces

An unnamed namespace gives names translation-unit-local linkage:

```cpp
namespace {
int normalize_channel(int channel)
{
    return channel < 0 ? 0 : channel;
}
}
```

### 14.2 Argument-Dependent Lookup

For an unqualified function call, argument-dependent lookup (ADL) also considers
namespaces and classes associated with argument types:

```cpp
telemetry::Packet packet{42};
send(packet); // ADL can find telemetry::send
```

ADL is useful for operators and customization functions, but it can make name
lookup surprising. Qualify calls while debugging selection problems.

## 15. Lambdas As Closure Objects

A lambda expression creates an object of a unique unnamed closure type:

```cpp
int threshold = 10;

auto is_high = [threshold](int value) {
    return value > threshold;
};
```

`threshold` is captured by value. The closure owns its copy.

Reference capture borrows:

```cpp
int count = 0;

auto record = [&count]() {
    ++count;
};
```

The closure must not outlive `count`. Capturing `this` similarly borrows the
object through a pointer; deferred callbacks can dangle.

A non-capturing lambda can convert to a compatible function pointer:

```cpp
using Transform = int (*)(int);

Transform double_value = [](int value) {
    return value * 2;
};
```

Advanced generic lambdas, move captures, `std::function`, and callback ownership
belong to later chapters.

## 16. `inline`, Name Mangling, And C Linkage

### 16.1 What `inline` Means

`inline` primarily supplies language-level ODR behavior for entities defined in
headers:

```cpp
inline int clamp_low(int value)
{
    return value < 0 ? 0 : value;
}
```

The definition must be reachable where required, and multiple translation-unit
definitions must satisfy the ODR.

Optimizer inlining is separate:

- a compiler may inline a function without the keyword;
- a compiler may generate a normal call for a function declared `inline`.

Do not use `inline` as a performance promise.

### 16.2 Name Mangling

C++ implementations usually encode information such as namespaces, class
membership, and parameter types into external symbol names. This supports ABI
distinction between overloads after the language has already resolved which
function a call means.

Overload resolution is a C++ language rule. Name mangling is an
implementation-specific ABI technique, not the source-language mechanism that
decides the overload.

### 16.3 `extern "C"`

Use C language linkage when a header is shared by C and C++:

```c
#ifndef SENSOR_API_H
#define SENSOR_API_H

#ifdef __cplusplus
extern "C" {
#endif

int sensor_read(int channel, int *out_value);

#ifdef __cplusplus
}
#endif

#endif
```

`extern "C"` is commonly reflected as an unmangled C-compatible symbol and may
affect function type/calling convention according to the implementation. It
does not make arbitrary C++ classes, templates, exceptions, or standard-library
types safe to expose through a C ABI.

Keep C boundaries narrow and explicit.

## 17. Practical Design Example

The following C++17 example combines a namespace, class invariant, explicit
constructor, const member function, reference parameter, overload, and lambda:

```cpp
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace sensing {

class Reading {
public:
    explicit Reading(int milli_celsius)
        : milli_celsius_{milli_celsius}
    {
        if (milli_celsius_ < -50000 || milli_celsius_ > 150000) {
            throw std::out_of_range{"temperature outside supported range"};
        }
    }

    int milli_celsius() const
    {
        return milli_celsius_;
    }

private:
    int milli_celsius_;
};

bool is_alarm(const Reading& reading)
{
    return reading.milli_celsius() >= 80000;
}

bool is_alarm(int milli_celsius)
{
    return is_alarm(Reading{milli_celsius});
}

}

int main()
{
    std::vector<sensing::Reading> readings{
        sensing::Reading{25000},
        sensing::Reading{82000}
    };

    const auto alarms = std::count_if(
        readings.begin(),
        readings.end(),
        [](const sensing::Reading& reading) {
            return sensing::is_alarm(reading);
        });

    std::cout << "alarms=" << alarms << '\n';
}
```

Build and run:

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
    -Wold-style-cast -Werror reading.cpp -o reading
./reading
```

Expected output:

```text
alarms=1
```

This is a learning example. A production design must choose a project-specific
error policy, units type, logging policy, exception policy, and test strategy.

## 18. Comparisons

### 18.1 C vs C++

| Topic | C | C++ |
| --- | --- | --- |
| Data abstraction | `struct` plus functions and conventions | Classes, access control, member functions, constructors |
| Required alias parameter | Pointer plus contract | Reference |
| Optional parameter | Pointer, often `NULL` | Pointer, commonly `nullptr` |
| Initialization | Initializer or explicit init function | Initialization syntax and constructors |
| Cleanup | Explicit cleanup function/control flow | Destructor tied to lifetime |
| Multiple typed operations | Different names | Function overloading |
| Name organization | Prefix conventions | Namespaces |
| Callback with state | Function pointer plus context | Lambda/closure or callable object |

C++ mechanisms reduce some manual coordination, but they do not remove the need
for ownership, lifetime, error, and ABI design.

### 18.2 Constructor And C Init Function

| Aspect | C init function | C++ constructor |
| --- | --- | --- |
| Invocation | Explicit ordinary call | Part of object initialization |
| Overloading | Different function names | Constructor overloads |
| Failure | Return status/output contract | Exception, factory/result, or constrained valid input |
| Invariant | Convention and API discipline | Can be enforced behind private data |

### 18.3 Destructor And C Cleanup

| Aspect | C cleanup | C++ destructor |
| --- | --- | --- |
| Trigger | Explicit call | Object-lifetime end |
| Early return | Every path must call cleanup | Automatic for live local objects |
| Copy risk | Contract-specific | Owning classes must design copy/move behavior |
| Timing | Explicit point | Deterministic from lifetime rules |

### 18.4 Overloading And C Names

| Aspect | C | C++ |
| --- | --- | --- |
| Source names | `parse_i32`, `parse_f64` | `parse` overload set |
| Selection | Caller selects name | Compiler performs overload resolution |
| Binary symbol | C ABI/toolchain convention | C++ ABI-specific encoding is common |
| Best use | Stable explicit C interface | One coherent operation for several types |

## 19. Common Bugs

### Initialization And Lifetime

- reading an uninitialized fundamental object;
- using a pointer or reference after the referred object dies;
- returning a reference or pointer to a local object;
- assuming initializer-list order controls member initialization;
- storing a reference to a temporary without understanding lifetime rules;
- manually releasing a resource and then releasing it again in a destructor;
- assuming exact constructor call counts despite copy elision.

### Classes

- allowing construction of an invalid state;
- assigning members in the constructor body instead of initializing them;
- forgetting `explicit` on a converting constructor;
- exposing public fields while claiming to protect an invariant;
- adding resource ownership without defining copy/move behavior;
- using friendship to avoid designing a coherent interface.

### Functions And Conversions

- expecting return type to resolve an overload;
- creating ambiguous overloads with symmetric conversions;
- using a cast to hide an API design problem;
- assuming integer-to-floating conversion is always exact;
- mixing signed and unsigned values carelessly;
- using `reinterpret_cast` without valid lifetime, alignment, and aliasing.

### Names And Linkage

- putting `using namespace` in a header;
- diagnosing every link error as name mangling;
- forgetting `extern "C"` guards in a shared C/C++ header;
- exposing implementation-specific C++ types through a stable C ABI;
- giving an inline function inconsistent definitions across translation units.

### Lambdas

- returning or storing a lambda that captures locals by reference;
- scheduling a `[this]` callback after the object may be destroyed;
- using broad `[&]` or `[=]` captures that hide dependencies;
- assuming every lambda converts to a function pointer.

## 20. Debugging And Verification

### 20.1 Start With Strict Compilation

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
    -Wold-style-cast -Werror -O0 -g3 source.cpp -o app
```

Warnings can expose:

- initialization-order mistakes;
- shadowing;
- suspicious conversions;
- old-style casts;
- missing returns;
- unused values and parameters.

### 20.2 Use Sanitizers On Executed Paths

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic -O1 -g3 \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    source.cpp -o app-sanitize
./app-sanitize
```

ASan and UBSan can expose selected lifetime, bounds, alignment, shift, and
undefined-behavior defects on executed paths. A clean run is not proof that
unexecuted paths are correct.

### 20.3 Debug Lifetime

- Break on constructors and destructors.
- Record object addresses only as diagnostic evidence, not as a language model.
- Identify the owner and every borrower.
- Check whether a lambda, callback, reference member, or returned reference
  outlives its source object.
- Remember that copy elision can remove expected copy/move breakpoints.

### 20.4 Debug Overload Resolution

1. List declarations with the selected name.
2. Identify candidates visible through normal lookup and ADL.
3. Remove non-viable candidates.
4. Write the conversion required for each argument.
5. Check `explicit`, list initialization, and member qualification.
6. If selection remains surprising, simplify the API.

### 20.5 Debug Linkage

Inspect symbols where the platform provides suitable tools:

```bash
c++ -std=c++17 -c source.cpp -o source.o
nm source.o
nm source.o | c++filt
```

Check:

- declaration and definition signatures;
- C versus C++ compilation mode;
- language-linkage guards;
- missing object files or libraries;
- target architecture and ABI;
- symbol visibility and link order.

## 21. Best Practices Checklist

- Initialize every object before use.
- Prefer valid construction over an “initialize later” state.
- Keep object lifetime separate from storage and address values.
- Use value semantics by default.
- Use `T&` for required mutable borrowing.
- Use `const T&` for required read-only borrowing when value passing is not the
  desired contract.
- Use pointers when nullability or address semantics are meaningful.
- Document every stored borrow's lifetime requirement.
- Keep class invariants private.
- Use member initializer lists.
- Match initializer-list order to member declaration order.
- Use default member initializers for stable defaults.
- Mark converting constructors `explicit` unless implicit conversion is
  intentionally natural.
- Prefer compiler-generated special members for non-owning value types.
- Design ownership before adding a destructor.
- Overload only one coherent operation.
- Make operator overloads unsurprising.
- Use friendship narrowly.
- Put library code in namespaces.
- Avoid global using directives in headers.
- Treat `inline` as an ODR tool, not a speed request.
- Keep C ABI boundaries narrow and C-compatible.
- Do not depend on optional optimizations for correctness.

## 22. Interview Readiness

### Beginner

**What is the main difference between a pointer and a reference?**

A pointer is an object that can store an address-like value, can be null, and
can be reseated. A reference is initialized as an alias and cannot be reseated.
Neither implies ownership.

**What is the difference between `struct` and `class` in C++?**

Their main capabilities are the same. The language difference is default access:
`struct` defaults to public; `class` defaults to private.

**What is a constructor?**

A constructor initializes an object and should establish its invariant. It has
the class name and no return type.

**What is a destructor?**

A destructor runs when object lifetime ends in the applicable context. It is
used to release resources owned by the object.

### Mid-Level

**Why use a member initializer list?**

Bases and members are initialized before the constructor body. Initializer lists
perform that initialization directly and are required for references, const
members, and non-default-constructible members.

**Why is `inline` not mainly a performance keyword?**

It gives an entity specific ODR behavior that supports header definitions.
Call-site inlining is an independent optimizer decision.

**How does overload resolution work?**

The compiler forms candidates, filters viable functions, ranks conversion
sequences, and selects one best viable function. If no unique best function
exists, the call is ill-formed.

**Why can a lambda capture dangle?**

A reference capture or `this` capture borrows an existing object. If the closure
outlives that object, later invocation accesses an expired lifetime.

### Senior

**How would you expose a C++ implementation to C?**

Use a narrow C header with `extern "C"` guards, C-compatible values, opaque
handles, explicit ownership functions, and an error policy that does not let
C++ exceptions cross the boundary.

**What changes when a class owns a resource?**

Copy, move, destruction, failure behavior, and ownership transfer must be
designed together. A raw owning handle plus compiler-generated copying is often
incorrect.

**Why is name mangling not the reason overload resolution works?**

Overload resolution is defined by the C++ language and chooses the function
semantically. Mangling is an implementation ABI technique for representing
distinct external entities to the linker.

### Common Interview Traps

- “A reference is just a const pointer.”
- “`const T&` is always the fastest parameter.”
- “A constructor allocates memory.”
- “A destructor always frees heap memory.”
- “`inline` guarantees no function call.”
- “C++ supports overloading because of mangling.”
- “`extern "C"` makes classes compatible with C.”
- “`constexpr` always runs at compile time.”
- “`struct` cannot have private members.”

## 23. Practice

### Basic

1. Write a `Percentage` class that accepts only values from 0 through 100.
2. Implement functions taking `int`, `int&`, `const int&`, and `int*`; explain
   each contract.
3. Demonstrate namespace qualification and a selective using declaration.
4. Write a list-initialization example that rejects narrowing.
5. Overload `print` for `int` and `double`, then predict calls with `char`,
   `long`, and `float`.

### Intermediate

1. Build a `SensorConfig` with default member initializers, an explicit
   constructor, validation, and const accessors.
2. Enable `-Wreorder` and deliberately create, then repair, an initialization
   order warning.
3. Write a C header used by one `.c` implementation and one `.cpp` caller.
4. Create a lambda that safely owns its captured state.
5. Implement `operator==` for a small units type.
6. Compare a C init/cleanup API with a C++ constructor/destructor wrapper.

### Advanced

1. Diagnose and redesign an ambiguous overload set.
2. Use one inline header function from two translation units and explain its ODR
   requirements.
3. Build an opaque C handle backed by a private C++ class.
4. Review a resource-owning class for copy and double-release defects.
5. Demonstrate copy elision without depending on constructor call counts.
6. Design a callback API and document the lifetime of every captured or stored
   object.

## 24. Summary

- C++ fundamentals center on initialization, object lifetime, interfaces, and
  deterministic cleanup.
- References express required aliases but do not own objects.
- Classes can protect invariants through constructors and access control.
- `struct` and `class` differ mainly in default access.
- Members initialize in declaration order.
- Destructors connect cleanup to lifetime, but owning types still need correct
  copy/move design.
- Overload resolution is a language process; name mangling is ABI machinery.
- Operator overloads should preserve familiar meaning.
- Namespaces organize names; avoid broad using directives in headers.
- Lambdas are closure objects whose captures have lifetime consequences.
- `inline` supplies ODR semantics and does not guarantee optimization.
- `extern "C"` supports language linkage at a narrow C boundary, not arbitrary
  C++ ABI portability.

## 25. Reference Notes

- The ISO C++ working draft defines reference initialization, classes,
  constructors, destructors, overload resolution, language linkage, copy
  elision, and ODR rules.
- The C++ Core Guidelines provide practical guidance for initialization,
  parameter passing, class design, ownership, and interfaces.
- Compiler warnings, sanitizer availability, symbol tools, ABI details, and
  module behavior are implementation- and version-specific.
