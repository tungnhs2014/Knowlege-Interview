# 09 - OOP In C++

## 1. Goal

Object-oriented programming is a way to organize behavior around types,
contracts, and relationships. In C++, it is one tool among several: procedural,
generic, functional, and value-oriented designs are equally valid.

After this chapter, you should be able to:

- distinguish encapsulation from abstraction;
- design a class that protects an invariant;
- choose among dependency, association, aggregation, composition, and
  inheritance;
- explain when public inheritance represents valid substitutability;
- use pure virtual functions and abstract classes;
- design a small interface-style base class;
- explain compile-time and runtime polymorphism;
- reason about static type, dynamic type, and virtual dispatch;
- explain `vtable` and `vptr` as a common implementation model;
- choose a correct base-class destructor policy;
- use `override` and `final`;
- recognize and prevent object slicing;
- compare inheritance with composition;
- compare C++ virtual dispatch with a C function-pointer table;
- understand the purpose and cost of multiple and virtual inheritance;
- apply interface segregation and dependency inversion without over-engineering;
- debug common hierarchy, lifetime, and dispatch defects.

This lesson uses C++17 for its primary examples and C17 for the explicit C
comparison. Chapter 08, C++ Fundamentals, is the prerequisite.

## 2. Why It Matters

Suppose an alarm can read from either a physical sensor or a test sensor. A
first implementation might create the physical sensor internally:

```cpp
class Alarm {
public:
    bool active() const
    {
        PhysicalSensor sensor;
        return sensor.read_millivolts() >= 2500;
    }
};
```

This class is tightly coupled to one implementation:

- tests cannot easily supply controlled readings;
- replacing the hardware API changes `Alarm`;
- sensor creation policy is hidden inside alarm logic;
- the class mixes two responsibilities.

A small abstraction separates policy from mechanism:

```cpp
enum class SensorStatus {
    ok,
    read_failed
};

struct SensorReading {
    SensorStatus status;
    int millivolts;
};

class Sensor {
public:
    virtual ~Sensor() = default;
    virtual SensorReading read() const = 0;
};

enum class AlarmState {
    inactive,
    active,
    sensor_failure
};

class Alarm {
public:
    explicit Alarm(const Sensor& sensor)
        : sensor_{sensor}
    {
    }

    AlarmState evaluate() const
    {
        const SensorReading reading = sensor_.read();
        if (reading.status != SensorStatus::ok) {
            return AlarmState::sensor_failure;
        }
        return reading.millivolts >= 2500
            ? AlarmState::active
            : AlarmState::inactive;
    }

private:
    const Sensor& sensor_;
};
```

`Alarm` now depends on a contract rather than one hardware implementation. This
can improve testability and change isolation.

The result type separates failure from the measurement domain. A negative
voltage remains a value, while `AlarmState::sensor_failure` prevents a failed
read from silently becoming "alarm inactive." Production code must document
the fail-safe or degraded-mode policy applied to that state.

It also introduces a lifetime requirement: `sensor_` is borrowed, so the
`Sensor` object must outlive the `Alarm`. OOP does not remove lifetime,
ownership, or performance questions. Good OOP makes those questions visible.

## 3. Mental Model: Contract, State, And Relationship

Do not begin an OOP design by drawing an inheritance tree. Begin with three
questions.

### 3.1 What Must Stay True?

An invariant is a condition that must hold whenever an object is available for
normal use.

```cpp
#include <stdexcept>

class Percentage {
public:
    explicit Percentage(int value)
        : value_{value}
    {
        if (value_ < 0 || value_ > 100) {
            throw std::out_of_range{"percentage outside [0, 100]"};
        }
    }

    int value() const
    {
        return value_;
    }

private:
    int value_;
};
```

The class does not expose a setter that could create an invalid value.
Encapsulation is useful because it protects a meaningful rule, not because
private fields are fashionable.

### 3.2 What May Clients Rely On?

A contract describes observable behavior:

- valid inputs;
- results and state changes;
- failure behavior;
- lifetime and ownership requirements;
- thread-safety expectations;
- complexity or timing promises when relevant.

An abstraction exposes the stable part of that contract while hiding details
that may change.

### 3.3 What Relationship Actually Exists?

Ask whether one object:

- temporarily uses another;
- knows another;
- refers to an independently owned part;
- owns a part;
- or is genuinely substitutable for another type.

Only the last question naturally suggests public inheritance.

## 4. Encapsulation And Abstraction

These terms are related but not interchangeable.

### 4.1 Encapsulation

Encapsulation groups state and operations while controlling access to the
representation.

Its practical purpose is to:

- preserve invariants;
- reduce the number of legal state transitions;
- hide representation choices;
- prevent clients from depending on internal details.

Poor encapsulation:

```cpp
struct Account {
    int balance_cents;
};

void withdraw(Account& account, int amount)
{
    account.balance_cents -= amount;
}
```

Nothing prevents a negative amount or an overdraw.

Invariant-oriented design:

```cpp
class Account {
public:
    explicit Account(int initial_cents)
        : balance_cents_{initial_cents}
    {
        if (balance_cents_ < 0) {
            throw std::out_of_range{"negative initial balance"};
        }
    }

    bool withdraw(int amount_cents)
    {
        if (amount_cents <= 0 || amount_cents > balance_cents_) {
            return false;
        }

        balance_cents_ -= amount_cents;
        return true;
    }

    int balance_cents() const
    {
        return balance_cents_;
    }

private:
    int balance_cents_;
};
```

Trivial getters and setters are not automatically good encapsulation. A setter
such as `set_balance()` would expose representation-oriented mutation instead
of a meaningful account operation.

### 4.2 Abstraction

Abstraction presents essential behavior while hiding implementation.

```cpp
class Logger {
public:
    virtual ~Logger() = default;
    virtual void write(const char *message) = 0;
};
```

Clients know that a logger accepts a message. They do not need to know whether
the implementation stores it in memory, prints it, or sends it elsewhere.

### 4.3 Comparison

| Concept | Main question | Typical mechanism |
| --- | --- | --- |
| Encapsulation | Who may access or change representation? | Private members and controlled operations |
| Abstraction | What stable behavior should clients see? | Public API, abstract base, opaque boundary |

Encapsulation can support abstraction, but neither requires inheritance.

## 5. Class And Object Design

A class defines a type. An object is an instance of that type with state,
identity, and lifetime.

### 5.1 `class` Versus `struct`

In C++, both support:

- member functions;
- constructors and destructors;
- access control;
- inheritance;
- virtual functions;
- templates.

The language differences are defaults:

| Construct | Default member access | Default base access |
| --- | --- | --- |
| `class` | `private` | `private` |
| `struct` | `public` | `public` |

Use a transparent `struct` when members can reasonably vary independently:

```cpp
struct Point {
    int x;
    int y;
};
```

Use a class when the type owns an invariant or requires controlled state
transitions:

```cpp
class NonNegativeCount {
public:
    explicit NonNegativeCount(int value)
        : value_{value < 0 ? 0 : value}
    {
    }

    int value() const
    {
        return value_;
    }

private:
    int value_;
};
```

This is a design convention, not a difference in language capability.

### 5.2 Public Contract And Private Representation

Keep the public surface small. Every public function becomes something clients
may depend on.

Prefer:

```cpp
class Door {
public:
    bool open();
    bool close();
    bool is_open() const;

private:
    bool open_{false};
};
```

over an API that exposes every internal variable through generic setters.

### 5.3 Static Members, Friendship, And `mutable`

A static member belongs to the class rather than each object:

```cpp
class Request {
public:
    Request()
        : id_{next_id_++}
    {
    }

    int id() const
    {
        return id_;
    }

private:
    inline static int next_id_{1};
    int id_;
};
```

Shared mutable static state needs a concurrency and lifetime policy. This
example is single-threaded.

A friend is a non-member or another class granted selected access:

```cpp
class Temperature {
public:
    explicit Temperature(int milli_celsius)
        : milli_celsius_{milli_celsius}
    {
    }

    friend bool operator==(
        Temperature left,
        Temperature right);

private:
    int milli_celsius_;
};

bool operator==(Temperature left, Temperature right)
{
    return left.milli_celsius_ == right.milli_celsius_;
}
```

Friendship is not inherited, transitive, or automatically reciprocal. Use it
only when the collaboration genuinely belongs with the type.

`mutable` permits a selected member to change in a `const` member function. It
can support logical constness for caching or locking, but it does not provide
thread safety.

## 6. Relationships Between Objects

Relationship words describe design intent. They do not automatically define
ownership.

### 6.1 Dependency

One operation temporarily uses another object:

```cpp
class Report {
public:
    void emit(Logger& logger) const
    {
        logger.write("report ready");
    }
};
```

`Report` does not store the logger.

### 6.2 Association

Objects know or interact with one another, but ownership is not implied.

```cpp
class Controller {
public:
    explicit Controller(Sensor& sensor)
        : sensor_{sensor}
    {
    }

private:
    Sensor& sensor_;
};
```

This stored reference is borrowed. The sensor must outlive the controller.

### 6.3 Aggregation

A whole refers to parts whose lifetimes are independent.

```cpp
class Dashboard {
public:
    Dashboard(const Sensor& left, const Sensor& right)
        : left_{left},
          right_{right}
    {
    }

private:
    const Sensor& left_;
    const Sensor& right_;
};
```

The sensors can exist before and after the dashboard.

### 6.4 Composition

A whole owns parts whose lifetimes are tied to the whole:

```cpp
class Filter {
public:
    explicit Filter(int offset)
        : offset_{offset}
    {
    }

    int apply(int value) const
    {
        return value + offset_;
    }

private:
    int offset_;
};

class FilteredSensor {
public:
    explicit FilteredSensor(int offset)
        : filter_{offset}
    {
    }

private:
    Filter filter_;
};
```

`Filter` is constructed and destroyed as part of `FilteredSensor`.

### 6.5 Inheritance

Inheritance defines a base/derived type relationship:

```cpp
class FixedSensor final : public Sensor {
public:
    explicit FixedSensor(int value)
        : value_{value}
    {
    }

    SensorReading read() const override
    {
        return {SensorStatus::ok, value_};
    }

private:
    int value_;
};
```

Public inheritance says that a `FixedSensor` can be used wherever the `Sensor`
contract is expected.

## 7. Inheritance And Substitutability

Public inheritance should model substitutability, not merely code reuse.

If a function accepts a `Sensor&`, every valid derived implementation should:

- satisfy the documented input contract;
- preserve base invariants;
- return results in the promised domain;
- obey the same ownership and lifetime rules;
- not introduce surprising stronger preconditions.

Consider:

```cpp
class Storage {
public:
    virtual ~Storage() = default;
    virtual bool write(const char *data) = 0;
};
```

A derived implementation that accepts only one hidden string value is probably
not a useful substitute for the advertised storage contract. The syntax is
legal, but the design is misleading.

An "`is-a`" sentence is only a first check. Behavioral compatibility matters.

## 8. Runtime Polymorphism

Runtime polymorphism lets a call through a base pointer or reference select
behavior from the object's dynamic type.

### 8.1 Complete Compile-Oriented Example

```cpp
#include <iostream>

enum class SensorStatus {
    ok,
    read_failed
};

struct SensorReading {
    SensorStatus status;
    int millivolts;
};

class Sensor {
public:
    virtual ~Sensor() = default;
    virtual SensorReading read() const = 0;
};

class FixedSensor final : public Sensor {
public:
    explicit FixedSensor(int value)
        : value_{value}
    {
    }

    SensorReading read() const override
    {
        return {SensorStatus::ok, value_};
    }

private:
    int value_;
};

enum class AlarmState {
    inactive,
    active,
    sensor_failure
};

class Alarm {
public:
    Alarm(const Sensor& sensor, int threshold)
        : sensor_{sensor},
          threshold_{threshold}
    {
    }

    AlarmState evaluate() const
    {
        const SensorReading reading = sensor_.read();
        if (reading.status != SensorStatus::ok) {
            return AlarmState::sensor_failure;
        }
        return reading.millivolts >= threshold_
            ? AlarmState::active
            : AlarmState::inactive;
    }

private:
    const Sensor& sensor_;
    int threshold_;
};

int main()
{
    FixedSensor sensor{2700};
    Alarm alarm{sensor, 2500};

    std::cout
        << std::boolalpha
        << (alarm.evaluate() == AlarmState::active)
        << '\n';
}
```

Build:

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
    -Wnon-virtual-dtor -Woverloaded-virtual -Werror \
    sensor.cpp -o sensor
./sensor
```

`Alarm` uses composition/association: it contains a reference to a
collaborator. `FixedSensor` uses inheritance: it implements the `Sensor`
contract.

### 8.2 Static Type And Dynamic Type

```cpp
FixedSensor fixed{1200};
const Sensor& sensor = fixed;
```

- The static type of `sensor` is `const Sensor&`.
- The dynamic type of the referred object is `FixedSensor`.

For a virtual call:

```cpp
sensor.read();
```

the dynamic type determines the final overrider, so
`FixedSensor::read()` executes.

For a non-virtual function, ordinary lookup and the static type determine the
call.

### 8.3 `virtual`, `override`, And `final`

`virtual` introduces virtual dispatch in a base declaration:

```cpp
virtual SensorReading read() const = 0;
```

`override` asks the compiler to verify that a derived declaration overrides a
base virtual:

```cpp
SensorReading read() const override;
```

It catches mistakes such as a missing `const`:

```cpp
// Error: does not override the const base function.
SensorReading read() override;
```

`final` prevents further overriding or inheritance:

```cpp
class FixedSensor final : public Sensor {
    // ...
};
```

Use `override` routinely. Use `final` only when closing the extension point is a
deliberate design decision.

## 9. Virtual Dispatch, `vtable`, And `vptr`

The C++ language specifies the observable behavior of virtual calls. It does
not require a particular object layout.

Most common C++ ABIs implement virtual dispatch with:

- a virtual table, commonly called a `vtable`, containing function addresses or
  related dispatch information;
- one or more hidden virtual pointers, commonly called `vptr`, connecting an
  object to the appropriate table.

A conceptual call looks like:

```text
base reference
      |
dynamic object
      |
implementation-specific vptr
      |
implementation-specific vtable slot
      |
final overrider
```

Do not assume:

- one vtable exists for every class on every implementation;
- the vptr is the first member;
- there is exactly one vptr;
- a vptr has a fixed size;
- every virtual call has a fixed time cost.

Multiple inheritance, virtual inheritance, ABI choice, optimization, and
devirtualization can change the implementation.

Virtual dispatch may add indirection and may inhibit some inlining, but
optimizers can sometimes determine the dynamic type and devirtualize the call.
Measure performance on the real target and workload.

## 10. Pure Virtual Functions, Abstract Classes, And Interfaces

### 10.1 Pure Virtual Function

A pure virtual declaration uses a pure-specifier:

```cpp
virtual SensorReading read() const = 0;
```

A class with a pure virtual final overrider is abstract and cannot be
instantiated.

```cpp
// Sensor sensor; // Error: Sensor is abstract.
```

A derived class remains abstract until it provides final overriders for all
required pure virtual functions.

A pure virtual function can have an out-of-class definition. The pure-specifier
controls abstractness and overriding requirements; it does not universally
forbid a definition. A pure virtual destructor must still have a definition
when destruction requires it.

### 10.2 Abstract Class

An abstract class may contain:

- data members;
- constructors;
- implemented non-virtual functions;
- implemented virtual functions;
- pure virtual functions.

Use an abstract class when a family needs both a polymorphic contract and shared
base behavior or state.

### 10.3 Interface-Style Class

C++ has no `interface` keyword. An interface-style class is a design convention:

```cpp
class Clock {
public:
    virtual ~Clock() = default;
    virtual long long now_milliseconds() const = 0;
};
```

A focused interface normally:

- contains no mutable instance data;
- exposes a small cohesive contract;
- avoids protected data;
- has an explicit destruction policy;
- does not prescribe ownership.

### 10.4 Interface Segregation

Do not force clients or implementations to depend on operations they do not
need.

Too broad:

```cpp
class Device {
public:
    virtual ~Device() = default;
    virtual void read() = 0;
    virtual void print() = 0;
    virtual void persist() = 0;
    virtual void connect_network() = 0;
};
```

Focused interfaces:

```cpp
class Readable {
public:
    virtual ~Readable() = default;
    virtual int read() const = 0;
};

class Printable {
public:
    virtual ~Printable() = default;
    virtual void print() const = 0;
};
```

An implementation can implement only the contracts it genuinely supports.

## 11. Virtual Destructor Policy

Destructor policy communicates whether clients may destroy a derived object
through the base interface.

### 11.1 Public Virtual Destructor

Use a public virtual destructor when polymorphic deletion is part of the
contract:

```cpp
class Sensor {
public:
    virtual ~Sensor() = default;
    virtual SensorReading read() const = 0;
};
```

Deleting a derived object through a base pointer without the required virtual
destructor has undefined behavior. It is not merely a predictable skipped
destructor or memory leak.

Ownership mechanics are covered deeply in Chapter 10. The important Chapter 09
rule is that the base destruction contract must match intended use.

### 11.2 Protected Non-Virtual Destructor

If clients must not destroy through the interface, a protected non-virtual
destructor can enforce that policy:

```cpp
class NonOwningView {
public:
    virtual int read() const = 0;

protected:
    ~NonOwningView() = default;
};
```

This design allows polymorphic calls but rejects `delete base_pointer` from
ordinary client code.

### 11.3 Rule Of Thumb

A polymorphic base should normally have either:

- a public virtual destructor; or
- a protected non-virtual destructor.

Not every destructor in every class should be virtual.

## 12. Object Slicing

Slicing occurs when a derived object is copied into a base object by value. The
new object contains only the base subobject.

```cpp
#include <iostream>

class Message {
public:
    virtual ~Message() = default;

    virtual void print() const
    {
        std::cout << "Message\n";
    }
};

class AlarmMessage final : public Message {
public:
    void print() const override
    {
        std::cout << "AlarmMessage\n";
    }
};

void print_sliced(Message message)
{
    message.print();
}

void print_polymorphic(const Message& message)
{
    message.print();
}
```

Calling `print_sliced(AlarmMessage{})` constructs a separate `Message` object.
Its dynamic type is `Message`, so it prints `Message`.

Calling `print_polymorphic(AlarmMessage{})` refers to the original derived
object and dispatches to `AlarmMessage::print`.

For polymorphic types:

- pass by reference or pointer;
- avoid storing base objects by value;
- consider making hierarchy bases abstract;
- design copy operations deliberately.

Making a base abstract helps prevent direct base objects, but API signatures
still need review.

## 13. Construction, Destruction, And Virtual Calls

Base subobjects construct before derived members and the derived constructor
body. Destruction proceeds in reverse.

```text
base construction
derived members
derived constructor body
normal use
derived destructor body
derived members destroyed
base destruction
```

During base construction, the derived part is not yet fully constructed.
During base destruction, the derived part has already been destroyed.

Therefore, virtual calls from constructors and destructors do not dispatch as
though the complete most-derived object were available.

Avoid designs that require overridable behavior during construction or
destruction. Prefer:

- completing construction first;
- then calling a normal operation;
- or using a factory that constructs and validates before returning.

## 14. Name Hiding And Default Arguments

### 14.1 Name Hiding

A declaration in a derived class can hide all base overloads with the same name:

```cpp
class Base {
public:
    void configure(int);
    void configure(double);
};

class Derived : public Base {
public:
    using Base::configure;
    void configure(const char *);
};
```

Without the `using` declaration, ordinary lookup in `Derived` finds the derived
name and hides the base overload set.

### 14.2 Default Arguments On Virtual Functions

Virtual dispatch selects the function body dynamically, but default arguments
are selected from the static type at the call site.

```cpp
class Base {
public:
    virtual ~Base() = default;
    virtual void report(int level = 1) const;
};

class Derived : public Base {
public:
    void report(int level = 2) const override;
};
```

Calling `report()` through a `Base&` selects `Derived::report` as the function
body but supplies the base declaration's default value `1`.

Avoid different default arguments on overriding declarations.

## 15. Compile-Time Polymorphism

Polymorphism means one interface shape can work with multiple behaviors or
types. Not all polymorphism is virtual.

### 15.1 Function Overloading

```cpp
void send(int code);
void send(const char *text);
```

The compiler selects an overload from argument types and conversion ranking.

### 15.2 Operator Overloading

```cpp
struct Millivolts {
    int value;
};

bool operator==(Millivolts left, Millivolts right)
{
    return left.value == right.value;
}
```

Operators should preserve familiar meaning and define invalid-input or
arithmetic policies.

Avoid overloading `&&` or `||` for ordinary domain types because overloaded
forms do not provide the built-in short-circuit semantics.

### 15.3 Templates And CRTP

Templates provide parametric compile-time polymorphism:

```cpp
template <typename SensorType>
SensorReading read_sensor(const SensorType& sensor)
{
    return sensor.read();
}
```

The Curiously Recurring Template Pattern (CRTP) passes a derived type as a base
template parameter:

```cpp
template <typename Derived>
class PrintableMixin {
public:
    void print() const
    {
        static_cast<const Derived&>(*this).print_impl();
    }
};
```

CRTP and mixins can provide static polymorphism or reusable behavior, but they
increase template coupling and belong mainly to later template/design chapters.
Use them only when the trade-off is justified.

### 15.4 Static Versus Runtime Polymorphism

| Aspect | Compile-time polymorphism | Runtime polymorphism |
| --- | --- | --- |
| Mechanisms | Overloads, templates, operators, CRTP | Virtual functions |
| Selection | During compilation | From dynamic type at runtime |
| Type set | Often known at compile time | Can remain open to new derived implementations |
| Coupling | Often header/template coupling | Base interface coupling |
| Cost | Possible code growth and longer builds | Possible indirection and layout cost |
| Main benefit | Optimization and type-specific generation | Runtime substitution and stable call sites |

Choose based on the required variability, not on slogans about speed.

## 16. Composition Over Inheritance

Inheritance is appropriate when a derived type must participate in the base
contract. Composition is usually better for implementation reuse and
configurable behavior.

Inheritance for reuse:

```cpp
class RetryingSensor : public PhysicalSensor {
    // Inherits representation and behavior only to reuse reading code.
};
```

Composition makes the collaboration explicit:

```cpp
class RetryingSensor final : public Sensor {
public:
    explicit RetryingSensor(const Sensor& inner)
        : inner_{inner}
    {
    }

    SensorReading read() const override
    {
        const SensorReading first = inner_.read();
        return first.status == SensorStatus::ok ? first : inner_.read();
    }

private:
    const Sensor& inner_;
};
```

`RetryingSensor` implements the `Sensor` contract and delegates actual reading
to another `Sensor`. The collaborator can be replaced without inheriting its
representation. A second failure remains a failure; the wrapper does not
reinterpret it as an ordinary measurement.

| Question | Inheritance | Composition |
| --- | --- | --- |
| Meaning | `is-a`, substitutable | `has-a` or delegates-to |
| Coupling | Base contract and hierarchy | Collaborator interface |
| Reuse | Interface and possible implementation | Delegation |
| Main risk | Fragile hierarchy | Forwarding and lifetime policy |
| Default | Genuine subtype relationship | Flexible behavior reuse |

## 17. Dependency Inversion

High-level policy should not create and depend directly on volatile low-level
details when a stable abstraction can separate them.

`Alarm` represents policy. `PhysicalSensor` represents one detail. Both meet at
the `Sensor` contract:

```text
Alarm --------> Sensor <-------- PhysicalSensor
 policy         contract          detail
```

The dependency is supplied from outside:

```cpp
PhysicalSensor sensor;
Alarm alarm{sensor, 2500};
```

This does not require a dependency-injection framework or service locator.
Constructor injection and an ordinary reference are often enough.

Do not create an interface for every class. Introduce an abstraction where
variation, testing, boundary isolation, or architectural policy justifies it.

## 18. Multiple Inheritance And The Diamond Problem

### 18.1 Multiple Interfaces

Multiple inheritance is easiest to justify when a class implements independent
interfaces:

```cpp
class Resettable {
public:
    virtual ~Resettable() = default;
    virtual void reset() = 0;
};

class Diagnosable {
public:
    virtual ~Diagnosable() = default;
    virtual int health_code() const = 0;
};

class SensorModule final
    : public Resettable,
      public Diagnosable {
public:
    void reset() override
    {
    }

    int health_code() const override
    {
        return 0;
    }
};
```

The interface bases contain no shared mutable state.

### 18.2 Diamond Problem

A diamond occurs when two intermediate bases inherit from one common base and a
final class inherits from both intermediates:

```text
       Device
       /    \
  Input    Output
       \    /
      Console
```

With ordinary inheritance, `Console` can contain two `Device` base subobjects,
creating duplication and ambiguity.

Virtual inheritance requests one shared virtual base:

```cpp
class Input : virtual public Device {
};

class Output : virtual public Device {
};
```

The most-derived class is responsible for initializing the virtual base.
Virtual inheritance adds layout, initialization, and maintenance complexity.
Before using it, ask whether independent interfaces or composition would
express the design more clearly.

## 19. OOP In C

C has no language-level classes, access control, inheritance, constructors,
destructors, or virtual functions. It can implement similar organization
explicitly.

### 19.1 Function-Pointer Table

```c
#include <stddef.h>

enum sensor_status {
    SENSOR_OK = 0,
    SENSOR_INVALID_ARGUMENT,
    SENSOR_READ_FAILED
};

struct sensor_ops {
    enum sensor_status (*read_millivolts)(
        const void *context,
        int *out_value);
};

struct sensor {
    const void *context;
    const struct sensor_ops *ops;
};

enum sensor_status sensor_read_millivolts(
    const struct sensor *sensor,
    int *out_value)
{
    if (sensor == NULL
        || sensor->ops == NULL
        || sensor->ops->read_millivolts == NULL
        || out_value == NULL) {
        return SENSOR_INVALID_ARGUMENT;
    }

    return sensor->ops->read_millivolts(sensor->context, out_value);
}

struct fixed_sensor {
    int value;
};

enum sensor_status fixed_sensor_read(
    const void *context,
    int *out_value)
{
    if (context == NULL || out_value == NULL) {
        return SENSOR_INVALID_ARGUMENT;
    }

    const struct fixed_sensor *fixed = context;
    *out_value = fixed->value;
    return SENSOR_OK;
}
```

Usage:

```c
static const struct sensor_ops fixed_ops = {
    .read_millivolts = fixed_sensor_read
};

int main(void)
{
    struct fixed_sensor implementation = {.value = 2700};
    struct sensor interface = {
        .context = &implementation,
        .ops = &fixed_ops
    };

    int value = 0;
    return sensor_read_millivolts(&interface, &value) == SENSOR_OK
        && value == 2700
        ? 0
        : 1;
}
```

Build:

```bash
cc -std=c17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
   -Werror sensor.c -o sensor
./sensor
```

### 19.2 C Contract Costs

The programmer must define:

- context type and lifetime;
- null handling;
- operation-table versioning;
- initialization and cleanup;
- ownership of context and table;
- error-status policy;
- whether calls are thread-safe;
- whether implementations may be replaced at runtime.

Opaque incomplete structs can hide representation across a C module boundary.
Init/deinit or create/destroy functions provide explicit lifetime operations.

### 19.3 C Versus C++

| Topic | C | C++ |
| --- | --- | --- |
| State | `struct` | Class or struct object |
| Encapsulation | Opaque handle/module boundary | Access control plus module boundary |
| Method call | Function with explicit context | Member or non-member function |
| Runtime dispatch | Function-pointer table | Virtual function |
| Object context | Explicit pointer | Implicit `this` |
| Lifetime | Init/deinit or create/destroy | Constructor/destructor |
| Destruction dispatch | Explicit operation | Virtual destructor policy |
| Subtyping | Convention | Public inheritance contract |

C's explicit model can be useful at a C ABI boundary or in constrained
environments. C++ offers stronger language checks and more direct syntax.

## 20. Practical Embedded And Enterprise Usage

### 20.1 Embedded Software

Useful abstraction boundaries include:

- sensor and actuator interfaces;
- clocks and timers;
- storage backends;
- transport channels;
- watchdog services;
- diagnostic sinks;
- host-test fakes.

Runtime polymorphism is not automatically unsuitable for embedded software.
Evaluate:

- target ABI and object-size cost;
- call frequency and timing budget;
- allocation policy;
- exception policy;
- testability benefit;
- whether implementations change at runtime;
- whether templates or link-time selection are simpler.

Static allocation works with virtual dispatch:

```cpp
FixedSensor sensor{2700};
Alarm alarm{sensor, 2500};
```

No dynamic allocation is required.

Do not hide allocation, blocking I/O, locking, or unbounded work behind a
seemingly cheap virtual call in a timing-critical path without documenting it.

### 20.2 Enterprise Software

OOP can support:

- service and repository interfaces;
- pluggable storage or transport backends;
- test doubles;
- stable policy boundaries;
- command handlers;
- serialization strategies.

Production review should ask:

- Is runtime substitution actually required?
- Is the interface cohesive?
- Is ownership explicit?
- Can the hierarchy preserve base behavior?
- Is the ABI boundary stable enough for its deployment model?
- Are exception and thread-safety policies documented?
- Would a concrete value or composition be simpler?

Avoid deep hierarchies, broad protected state, global singleton dependencies,
and interfaces created only for ceremony.

## 21. Required Comparisons

### 21.1 Overloading Versus Overriding

| Aspect | Overloading | Overriding |
| --- | --- | --- |
| Selection | Compile time | Runtime when called polymorphically |
| Form | Same name with different parameter lists | Derived declaration matching a base virtual |
| Keyword | None | `override` recommended |
| Common bug | Ambiguous conversion | Signature mismatch or hiding |
| Category | Ad hoc compile-time polymorphism | Runtime subtype polymorphism |

### 21.2 Abstract Class Versus Interface-Style Class

| Aspect | Abstract class | Interface-style class |
| --- | --- | --- |
| Meaning | Language property caused by pure virtual final overrider | Design convention |
| State | May contain state | Prefer no mutable data |
| Implementation | May share implementation | Usually contract-only |
| Construction | May initialize base state | Often no user-written constructor |
| Main purpose | Contract plus selected common behavior | Decoupled behavior contract |

### 21.3 Virtual Dispatch Versus Function-Pointer Table

| Aspect | C++ virtual dispatch | C function-pointer table |
| --- | --- | --- |
| Context | Implicit `this` | Explicit context pointer |
| Dispatch | `object.operation()` | `object.ops->operation(object.context)` |
| Override checking | Language-supported | Convention plus signature checks |
| Layout | ABI-specific implementation | Programmer-defined |
| Cleanup | Destructor policy | Explicit cleanup operation |
| Boundary | Natural in C++ | Useful for C and C ABI |

Inheritance versus composition and C versus C++ were compared in Sections 16
and 19.

## 22. Common Bugs

### 22.1 Undefined Behavior During Polymorphic Deletion

Deleting a derived object through a base pointer without a suitable virtual
destructor is undefined behavior.

### 22.2 Object Slicing

Passing or storing a polymorphic base by value discards the derived portion.

### 22.3 Accidental Non-Override

A missing `const`, reference qualifier, parameter difference, or typo can
create a different function. Use `override`.

### 22.4 Calling Virtual Functions During Construction

The most-derived object is not fully available during base construction, and
derived behavior must not be expected.

### 22.5 Broken Substitutability

A derived implementation strengthens preconditions, weakens guarantees, or
violates base invariants.

### 22.6 Dangling Stored Borrow

```cpp
Alarm make_alarm()
{
    FixedSensor local{2700};
    return Alarm{local, 2500}; // Returned Alarm would borrow a dead object.
}
```

The reference remains, but the referred object's lifetime ends at function
exit. This is undefined behavior when used.

### 22.7 Protected Data

Derived classes can directly create states that violate assumptions owned by
the base. Prefer private state and protected operations when extension needs
support.

### 22.8 Name Hiding

A derived function hides a base overload set unless it is reintroduced with
`using`.

### 22.9 Different Virtual Default Arguments

The function body is selected dynamically, while the default argument is
selected statically. Keep defaults consistent or avoid them on virtuals.

### 22.10 Downcast-Driven Design

Repeated `dynamic_cast` checks often indicate missing virtual behavior, an
incorrect base contract, or a closed set better represented another way.

### 22.11 Shared Mutable State

Static members and `mutable` caches can cause data races. Neither OOP nor
`mutable` provides synchronization.

### 22.12 Misleading Operators

Operators that perform hidden I/O, mutation, allocation, unchecked signed
arithmetic, or surprising work make APIs harder to review.

### 22.13 Raw Owning Pointers

Manual polymorphic ownership can cause leaks, double deletion, and
exception-safety defects. Chapter 10 develops value and smart-pointer ownership
in depth.

## 23. Debugging

### 23.1 Compiler Warnings

Use a strict host build:

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
    -Wnon-virtual-dtor -Woverloaded-virtual -Werror \
    example.cpp -o example
```

Exact warning support varies by compiler. Useful diagnostics include:

- non-virtual destructor in a polymorphic base;
- hidden virtual overloads;
- signature mismatches caught by `override`;
- initialization-order mistakes;
- suspicious conversions and shadowing.

### 23.2 Sanitizers

```bash
c++ -std=c++17 -O1 -g3 \
    -fsanitize=address,undefined,vptr \
    -fno-omit-frame-pointer \
    example.cpp -o example-sanitize
./example-sanitize
```

`vptr` sanitizer support is compiler- and platform-specific. Sanitizers inspect
executed paths; a clean run does not prove hierarchy correctness, lifetime
safety, substitutability, ABI stability, or race freedom.

### 23.3 GDB

Useful operations:

```text
break FixedSensor::read
break Sensor::~Sensor
run
bt
print *sensor_pointer
ptype *sensor_pointer
```

Use breakpoints on base and derived constructors/destructors to verify order.
Compare calls through an object, base reference, and base pointer.

Virtual table symbols and object layouts can sometimes be inspected with
`nm`, `c++filt`, debugger facilities, or compiler layout dumps. Treat the
results as implementation-specific.

### 23.4 Contract Tests

Run the same behavior tests against every implementation:

```cpp
#include <stdexcept>

void verify_sensor_contract(const Sensor& sensor)
{
    const SensorReading reading = sensor.read();
    if (reading.status != SensorStatus::ok) {
        throw std::logic_error{"sensor contract failed"};
    }
}
```

Real tests should use the project's test framework and error policy. Contract
tests help find derived implementations that are syntactically correct but
behaviorally incompatible. Test valid negative measurements separately from
read failures so a sentinel convention cannot reappear unnoticed.

## 24. Best Practices

- Start from the problem and expected variability.
- Prefer a concrete value type when runtime substitution is unnecessary.
- Use encapsulation to protect invariants, not to generate ceremonial setters.
- Keep public interfaces small and semantic.
- Use public inheritance only for substitutable types.
- Prefer composition for reuse and configurable behavior.
- Distinguish dependency, association, aggregation, composition, and
  inheritance.
- State ownership and borrowing separately from relationship names.
- Keep interface-style bases cohesive and free of mutable data.
- Use `override` on every overriding declaration.
- Use `final` only for an intentional closed extension point.
- Choose a public virtual or protected non-virtual base destructor deliberately.
- Avoid protected data.
- Pass polymorphic objects by reference or pointer, not by value.
- Avoid virtual calls from constructors and destructors.
- Avoid inconsistent default arguments on virtual functions.
- Use multiple inheritance mainly for independent interfaces.
- Keep virtual inheritance rare and documented.
- Prefer virtual behavior to type switches when behavior belongs to the type.
- Avoid broad friendship.
- Do not treat Singleton as a default solution for shared services.
- Keep operator semantics familiar and bounded.
- Separate polymorphism from ownership; design both explicitly.
- Measure virtual-dispatch cost on the real target before optimizing.
- Keep ABI-specific layout claims out of portable contracts.

## 25. Interview Readiness

### Beginner

**What is the difference between encapsulation and abstraction?**

Encapsulation controls access to representation and protects invariants.
Abstraction exposes a stable contract while hiding implementation details.

**What is overriding?**

Overriding is a derived declaration providing a final overrider for a base
virtual function. Use `override` so the compiler checks the intent.

**What makes a class abstract?**

A class is abstract when it has at least one pure virtual final overrider. It
cannot be instantiated directly.

**What is object slicing?**

Slicing occurs when a derived object is copied into a base object by value. The
new object contains only the base subobject.

### Mid-Level

**Why does a polymorphic base need a virtual destructor?**

If clients may delete a derived object through the base type, virtual
destruction is required for defined behavior and complete destruction.
Alternatively, a protected non-virtual destructor can forbid such deletion.

**How does virtual dispatch work?**

The static type establishes the visible base contract. For a virtual call, the
dynamic type determines the final overrider. Vtables and vptrs are a common ABI
implementation, not a language mandate.

**Why prefer composition over inheritance?**

Composition reuses behavior through a narrow collaborator contract without
coupling to a base representation or claiming subtype substitutability.

**How does C implement runtime polymorphism?**

C commonly stores an explicit state/context pointer and a table of compatible
function pointers. Initialization, cleanup, ownership, and ABI policy remain
explicit programmer responsibilities.

### Senior

**How do you choose between templates and virtual functions?**

Use templates when implementations are known at compile time and type-specific
generation is useful. Use virtual functions when runtime substitution, an open
implementation set, or stable non-template call sites matter. Include code
size, build time, ABI, ownership, and target performance in the decision.

**What makes public inheritance valid?**

The derived type must preserve the base contract: valid base operations remain
valid, promised guarantees remain true, and clients do not need knowledge of
the derived type for correctness.

**When is multiple inheritance acceptable?**

It is clearest for multiple independent interface bases. Shared state and
diamonds require stronger justification; virtual inheritance solves shared-base
identity but introduces complexity.

### Common Traps

- “Encapsulation means getters and setters.”
- “Abstraction and encapsulation are the same.”
- “Inheritance is mainly for code reuse.”
- “Every destructor must be virtual.”
- “Every abstract class needs a public virtual destructor.”
- “A pure virtual function cannot have a definition.”
- “C++ requires one vptr as the first member.”
- “A virtual call has a fixed cost.”
- “`override` introduces virtual dispatch.”
- “`final` is mainly an optimization.”
- “An abstract base prevents every slicing bug.”
- “Private inheritance is composition.”
- “A stream operator must be a friend.”
- “C cannot implement polymorphism.”

## 26. Practice

### Basic

1. Refactor a public data structure into a class that preserves one invariant.
2. Write a `Logger` interface and two stack-allocated implementations.
3. Demonstrate overloading and overriding in separate programs.
4. Remove `const` from an overriding function and observe the `override`
   diagnostic.
5. Demonstrate slicing, then change the function to accept `const Base&`.

### Intermediate

1. Implement the `Sensor` and `Alarm` example with a physical and fake sensor.
2. Implement the same sensor dispatch with a C17 function-pointer table.
3. Replace inheritance-for-reuse with a composed decorator-like sensor.
4. Write one contract test that runs against two sensor implementations.
5. Demonstrate name hiding and repair it with a `using` declaration.
6. Create a class implementing two independent interfaces.

### Advanced

1. Review a hierarchy in which one derived class strengthens a precondition.
   Redesign the contract or type relationship.
2. Design a heterogeneous command pipeline with explicit ownership and no raw
   owning pointers.
3. Build a diamond hierarchy, explain virtual-base initialization, then
   redesign it using interfaces or composition.
4. Compare virtual dispatch, a closed `std::variant` design, and CRTP for one
   concrete problem.
5. Inspect virtual-table-related symbols on one compiler and document why the
   result is ABI-specific.
6. Design a narrow C ABI around a private C++ implementation.

## 27. Summary

- OOP is a design tool for contracts, invariants, and relationships.
- Encapsulation protects representation and invariants; abstraction hides
  implementation behind stable behavior.
- Public inheritance means substitutability, not merely reuse.
- Composition is usually the safer default for behavior reuse.
- Virtual functions provide runtime polymorphism through base references or
  pointers.
- Static type determines the visible contract; dynamic type selects the final
  overrider for virtual calls.
- Vtables and vptrs are common implementation techniques, not portable layout
  guarantees.
- Interface-style classes are a C++ convention built from abstract classes.
- Polymorphic bases need an intentional destructor policy.
- Object slicing occurs when derived objects are copied as base values.
- `override` verifies intent; `final` closes extension deliberately.
- Multiple inheritance is clearest for independent interfaces; virtual
  inheritance adds complexity.
- C can implement explicit polymorphism with context pointers and function
  tables.
- Ownership, borrowing, thread safety, exception behavior, and performance
  remain explicit design concerns.

## 28. Reference Notes

- The ISO C++ language rules define virtual functions, abstract classes,
  overriding, destruction, inheritance, and object behavior.
- The C++ Core Guidelines provide practical guidance for class invariants,
  hierarchy design, interfaces, slicing, virtual destructors, and composition.
- Vtable layout, vptr placement, object size, warning flags, sanitizer support,
  devirtualization, and ABI behavior are implementation-specific.
