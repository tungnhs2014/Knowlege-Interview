# 09 - OOP In C++: Interview Pack

## How To Use This Pack

For each question:

1. Give the short answer first.
2. Explain the language and design mechanism.
3. Anchor the answer in C or C++ code.
4. Connect it to production review or debugging.
5. Identify traps instead of repeating slogans.
6. Handle follow-up questions without weakening the original claim.

The primary baseline is C++17. C17 is used for the explicit
function-pointer-table comparison.

## Beginner Questions

### 1. Compare encapsulation and abstraction.

**Short answer**

Encapsulation controls access to representation and protects invariants.
Abstraction exposes a stable contract while hiding implementation details.

**Deep explanation**

Encapsulation answers: "Who may inspect or change this state?" Private members
and semantic operations can prevent clients from constructing invalid states.

Abstraction answers: "What behavior may clients rely on without knowing how it
is implemented?" A public API, abstract base, or opaque module can provide that
boundary.

They often work together, but they are not synonyms. A concrete class can
encapsulate state without inheritance. A C module with an opaque handle can
provide abstraction without C++ classes.

**C/C++ code or API anchor**

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

    int value() const { return value_; }

private:
    int value_;
};
```

The private representation supports encapsulation. The public constructor and
accessor form the abstraction clients use.

**Production and debug angle**

Review whether private state actually protects an invariant. A class containing
only trivial getters and setters may add ceremony without reducing invalid
states. During debugging, inspect state transitions through semantic operations
rather than patching private data.

**Common traps**

- Saying encapsulation means getters and setters.
- Saying abstraction requires an abstract class.
- Assuming private data automatically creates a good contract.
- Treating security and access control as the same thing.

**Follow-up questions**

- Can a `struct` provide encapsulation?
- How can C provide abstraction?
- When is public data appropriate?

### 2. Explain dependency, association, aggregation, composition, and inheritance.

**Short answer**

They describe different relationships: temporary use, ongoing interaction,
independently owned parts, owned parts, and subtype substitutability.

**Deep explanation**

- A dependency temporarily uses another object.
- An association stores or knows another object without implied ownership.
- Aggregation describes a whole referring to parts that can outlive it.
- Composition makes parts owned by and lifetime-bound to the whole.
- Public inheritance says a derived object satisfies the base contract.

These labels do not by themselves establish ownership. A pointer or reference
can be owning or non-owning only according to the API contract.

**C/C++ code or API anchor**

```cpp
class Alarm {
public:
    explicit Alarm(const Sensor& sensor)
        : sensor_{sensor}
    {
    }

private:
    const Sensor& sensor_; // association/aggregation; borrowed
};

class FilteredSensor {
private:
    Filter filter_; // composition; owned subobject
};
```

**Production and debug angle**

For every stored pointer or reference, identify the owner and required lifetime.
Many "OOP bugs" are actually dangling associations or ambiguous ownership.
Composition gives automatic subobject lifetime, while stored borrowing needs
external lifetime discipline.

**Common traps**

- Equating every member field with composition.
- Assuming a raw pointer means ownership.
- Calling inheritance an ownership relationship.
- Choosing inheritance only to reuse implementation.

**Follow-up questions**

- Can aggregation be represented with a reference?
- How does dependency injection relate to association?
- When should an owned collaborator be stored by value?

### 3. Compare overloading and overriding.

**Short answer**

Overloading selects among same-named functions at compile time. Overriding
provides derived behavior for a base virtual function and can dispatch at
runtime.

**Deep explanation**

Overload resolution forms candidates, finds viable functions, ranks conversion
sequences, and chooses one best match. It is ad hoc compile-time polymorphism.

Overriding requires a derived declaration that matches a base virtual function.
When called through a base pointer or reference, the object's dynamic type
selects the final overrider.

Use `override` so the compiler checks the intended relationship.

**C/C++ code or API anchor**

```cpp
void log(int code);
void log(const char *message); // overload

class Sensor {
public:
    virtual ~Sensor() = default;
    virtual int read() const = 0;
};

class FixedSensor final : public Sensor {
public:
    int read() const override { return 42; } // override
};
```

**Production and debug angle**

An unexpected overload usually involves conversions or hidden candidates. An
unexpected virtual call usually involves static type, dynamic type, signature
mismatch, slicing, or construction/destruction state. Enable
`-Woverloaded-virtual` and use `override`.

**Common traps**

- Saying overloading is runtime polymorphism.
- Believing return type alone can overload a function.
- Omitting `const` and accidentally failing to override.
- Assuming same spelling means overriding.

**Follow-up questions**

- What is name hiding?
- Can a private virtual function be overridden?
- Does `override` introduce virtual dispatch?

### 4. What are a pure virtual function, abstract class, and interface?

**Short answer**

A pure virtual function has a pure-specifier. A class with a pure virtual final
overrider is abstract. An interface is a C++ design convention, usually a small
abstract class containing contract-only virtual operations.

**Deep explanation**

An abstract class cannot be instantiated but may have constructors, data,
implemented functions, and shared behavior. A derived class remains abstract
until all required pure virtual functions have final overriders.

C++ has no `interface` keyword. An interface-style base normally has no mutable
instance data, keeps its contract cohesive, and declares a deliberate
destructor policy.

A pure virtual function may have an out-of-class definition. A pure virtual
destructor needs a definition when destruction uses it.

**C/C++ code or API anchor**

```cpp
class Logger {
public:
    virtual ~Logger() = default;
    virtual void write(const char *message) = 0;
};
```

**Production and debug angle**

Use interfaces at real variation or test boundaries, not for every class.
Review interface size, ownership neutrality, exception policy, thread safety,
and whether all implementations can honor the contract.

**Common traps**

- Saying an abstract class contains only pure virtual functions.
- Saying a pure virtual function can never have a definition.
- Assuming every abstract class needs a public virtual destructor.
- Treating interface as a C++ keyword.

**Follow-up questions**

- When would an abstract class contain state?
- What is a protected non-virtual destructor for?
- How does interface segregation improve this design?

### 5. What is object slicing?

**Short answer**

Slicing occurs when a derived object is copied into a base object by value. The
new object contains only the base subobject.

**Deep explanation**

Virtual dispatch depends on the dynamic type of the object being called. After
slicing, the destination is a separate base object whose dynamic type is the
base type. Derived data and derived behavior are not present in that object.

Slicing commonly occurs in function parameters, returns, assignments, and
containers of base objects.

**C/C++ code or API anchor**

```cpp
class Message {
public:
    virtual ~Message() = default;
    virtual const char *name() const { return "Message"; }
};

class AlarmMessage final : public Message {
public:
    const char *name() const override { return "AlarmMessage"; }
};

void bad(Message message);             // slices
void good(const Message& message);     // preserves dynamic type
```

**Production and debug angle**

Search APIs for polymorphic base parameters and containers passed by value.
Break at the call site and inspect whether a new base object is constructed.
Making a base abstract helps, but reference/pointer API design is still
required.

**Common traps**

- Saying a base pointer slices the object.
- Believing virtual functions prevent slicing.
- Assuming an abstract base prevents every slicing pattern.
- Storing derived objects in `std::vector<Base>`.

**Follow-up questions**

- How would you store heterogeneous objects?
- Can slicing happen during assignment?
- Why does a reference preserve polymorphism?

## Mid-Level Questions

### 6. Explain static type, dynamic type, and virtual dispatch.

**Short answer**

The static type is known from the expression at compile time. The dynamic type
is the most-derived live object. A virtual call through a base pointer or
reference selects the final overrider from the dynamic type.

**Deep explanation**

The static type controls ordinary lookup, visible members, conversions, access,
and default arguments. For virtual functions, runtime dispatch then selects the
final overrider associated with the dynamic object.

The C++ language specifies this behavior without requiring a particular object
layout. Most ABIs implement it using virtual tables and hidden virtual pointers.

**C/C++ code or API anchor**

```cpp
FixedSensor fixed{2700};
const Sensor& sensor = fixed;

// Static type: const Sensor&
// Dynamic type: FixedSensor
const SensorReading reading = sensor.read();
```

**Production and debug angle**

When the wrong function appears to run, check for slicing, a non-virtual base
declaration, signature mismatch, construction/destruction context, or a call
made on an object rather than through the expected base reference. Use debugger
breakpoints on both base and derived functions.

**Common traps**

- Claiming every member call uses the dynamic type.
- Confusing pointer type with object dynamic type.
- Treating vtables as standard-mandated layout.
- Assuming virtual dispatch works on a sliced base object.

**Follow-up questions**

- What is a final overrider?
- Can a virtual call be devirtualized?
- What happens during base construction?

### 7. What are `vtable` and `vptr`, and what may you safely claim about them?

**Short answer**

They are common ABI mechanisms for virtual dispatch, not mandatory C++ language
objects with portable layouts.

**Deep explanation**

A typical implementation stores dispatch information in a class-related table
and places one or more hidden pointers in polymorphic objects. A call follows
implementation-defined metadata to the final overrider.

Multiple inheritance, virtual inheritance, thunks, RTTI, ABI rules, and
optimization can make the layout more complicated than "one table per class and
one pointer as the first member."

The portable claim is that virtual calls exhibit language-defined dispatch
behavior. Table count, pointer placement, object size, and slot order are ABI
details.

**C/C++ code or API anchor**

```cpp
class Base {
public:
    virtual ~Base() = default;
    virtual void run() = 0;
};
```

Useful implementation inspection commands may include:

```bash
nm object.o | c++filt
```

**Production and debug angle**

Inspect symbols and layouts only for the selected compiler, target, and ABI.
Never serialize raw polymorphic object bytes or expose class layout as a stable
C ABI. Measure actual dispatch cost rather than quoting fixed nanoseconds.

**Common traps**

- Saying C++ requires a vtable.
- Assuming the vptr is always first or pointer-sized.
- Claiming virtual calls cannot be inlined.
- Treating one compiler's layout as portable.

**Follow-up questions**

- What is devirtualization?
- How can multiple inheritance affect layout?
- Why are C++ class ABIs fragile across toolchains?

### 8. Why does a polymorphic base need a destructor policy?

**Short answer**

The base must state whether clients may destroy derived objects through it. Use
a public virtual destructor for polymorphic deletion, or a protected
non-virtual destructor to forbid such deletion.

**Deep explanation**

If a derived object is deleted through a pointer to a base whose destructor is
not suitably virtual, the behavior is undefined under the relevant deletion
rules. It is not merely a guaranteed base-destructor call followed by a leak.

A virtual destructor participates in dynamic dispatch and destroys the complete
object in derived-to-base order. A protected non-virtual destructor can be
appropriate for a non-owning interface where clients may call virtual functions
but may not delete through the base.

**C/C++ code or API anchor**

```cpp
class OwningInterface {
public:
    virtual ~OwningInterface() = default;
    virtual void run() = 0;
};

class NonOwningInterface {
public:
    virtual void run() = 0;

protected:
    ~NonOwningInterface() = default;
};
```

**Production and debug angle**

Enable `-Wnon-virtual-dtor`. Review factories, containers, callbacks, and C
boundaries for who destroys each implementation. ASan/UBSan may expose an
executed failure, but the design must be correct independently of sanitizer
coverage.

**Common traps**

- Making every destructor virtual.
- Making a non-owning interface publicly deletable.
- Describing the defect only as a memory leak.
- Assuming a pure virtual destructor needs no definition.

**Follow-up questions**

- Should a virtual destructor be pure?
- What does `= default` provide?
- How should a heterogeneous owning collection be represented?

### 9. Debug this override and name-hiding problem.

**Prompt**

```cpp
class Base {
public:
    virtual void configure(int) const;
    void configure(double) const;
};

class Derived : public Base {
public:
    void configure(int);
    void configure(const char *) const;
};
```

Why does `Derived::configure(int)` not override, and why might
`derived.configure(2.5)` not call the base overload?

**Short answer**

The derived `int` overload lacks `const`, so it has a different signature. The
derived declarations also hide the base overload set during ordinary lookup.

**Deep explanation**

Overriding requires a compatible declaration, including cv/ref qualifiers.
Adding `override` turns the silent mismatch into a compiler error.

Name lookup finds declarations named `configure` in `Derived`, so base
overloads are hidden even when their parameter types differ. A `using`
declaration can reintroduce the base overload set.

**C/C++ code or API anchor**

```cpp
class Derived : public Base {
public:
    using Base::configure;

    void configure(int) const override;
    void configure(const char *) const;
};
```

**Production and debug angle**

Compile with `-Woverloaded-virtual` and warnings as errors. In review, compare
the complete signatures, not only names and parameter spellings. Add tests
through both `Base&` and `Derived&`.

**Common traps**

- Removing `const` from the base to make the error disappear.
- Assuming overloads in a base remain visible automatically.
- Believing `override` changes runtime behavior.
- Adding unsafe casts at each call site.

**Follow-up questions**

- Can a private base virtual be overridden?
- What are ref-qualified virtual functions?
- How do default arguments interact with overriding?

### 10. Why are virtual calls from constructors and destructors dangerous?

**Short answer**

During construction and destruction, dispatch does not behave as if the fully
formed most-derived object were available.

**Deep explanation**

Base construction happens before derived construction. While a base constructor
runs, derived members and invariants are not established. During base
destruction, the derived part has already been destroyed.

Virtual calls in those phases dispatch within the currently constructed or
destroyed class context. Designs that expect derived overrides can observe
incomplete state or simply call a different function than intended. Calling a
pure virtual function from such a context is especially hazardous.

**C/C++ code or API anchor**

```cpp
class Base {
public:
    Base()
    {
        initialize(); // does not safely invoke future Derived behavior
    }

    virtual void initialize()
    {
    }
};
```

Prefer:

```cpp
Derived object;
object.initialize();
```

or a factory that constructs first and invokes post-construction validation.

**Production and debug angle**

Break on base and derived constructors, destructors, and virtual functions.
Inspect which subobjects are live at each call. Avoid two-phase initialization
unless its failure and partially initialized states are designed explicitly.

**Common traps**

- Assuming virtual means most-derived dispatch at every moment.
- Calling an overridable cleanup hook from a base destructor.
- Accessing derived state before construction.
- Hiding constructor failures in an `init()` convention.

**Follow-up questions**

- What is construction order?
- Can a constructor call a non-virtual helper?
- When is a factory preferable?

### 11. Compare C++ virtual dispatch with a C function-pointer table.

**Short answer**

Both can provide runtime behavior selection. C++ makes the object context and
override relationship language-supported; C represents both explicitly.

**Deep explanation**

A C operation table stores compatible function pointers. An object pairs that
table with a context pointer. The caller explicitly passes the context.

C++ virtual functions use an implicit `this` object and compiler-checked
overriding. The implementation commonly uses table-like ABI machinery, but its
layout is not programmer-defined.

C remains useful for C ABI boundaries and C-only systems. It requires explicit
initialization, null handling, cleanup, table versioning, and context lifetime.

**C/C++ code or API anchor**

```c
enum sensor_status {
    SENSOR_OK = 0,
    SENSOR_INVALID_ARGUMENT,
    SENSOR_READ_FAILED
};

struct sensor_ops {
    enum sensor_status (*read)(
        const void *context,
        int *out_value);
};

struct sensor {
    const void *context;
    const struct sensor_ops *ops;
};
```

```cpp
class Sensor {
public:
    virtual ~Sensor() = default;
    virtual SensorReading read() const = 0;
};
```

**Production and debug angle**

For C, validate every table and function pointer before use and document who
owns the context. For C++, validate destructor and ABI policy. At mixed-language
boundaries, expose a narrow C API rather than C++ class layout. In both models,
keep operation status separate from output values.

**Common traps**

- Saying C cannot implement polymorphism.
- Assuming a C table supplies automatic cleanup.
- Assuming C++ virtual layout is a stable C ABI.
- Using `void *` without a context-type contract.

**Follow-up questions**

- How would you version an operation table?
- How would C express destruction?
- Which model is easier to use without dynamic allocation?

## Senior Questions

### 12. Review inheritance versus composition for a retrying sensor.

**Prompt**

A developer derives `RetryingSensor` from `PhysicalSensor` only to reuse
`read_hardware()`. Is this a sound hierarchy?

**Short answer**

Probably not. Reuse alone does not establish substitutability. Prefer composing
or injecting a `Sensor` collaborator unless `RetryingSensor` genuinely satisfies
the complete `PhysicalSensor` contract.

**Deep explanation**

Public inheritance exposes the entire base contract and creates coupling to
base behavior, state, protected surface, and future changes. A retry policy is
usually orthogonal behavior that can wrap any sensor.

Composition makes the dependency explicit and permits retrying physical, fake,
remote, or cached sensors without inheriting their representations.

**C/C++ code or API anchor**

```cpp
enum class SensorStatus {
    ok,
    read_failed
};

struct SensorReading {
    SensorStatus status;
    int millivolts;
};

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
    const Sensor& inner_; // borrowed
};
```

**Production and debug angle**

Document retry count, error domain, timing, blocking behavior, and collaborator
lifetime. In real-time code, a hidden retry can violate deadlines. Contract
tests should run against wrapped and unwrapped implementations. Test persistent
failure as well as recovery, and require callers such as an alarm to distinguish
read failure from a valid inactive result.

**Common traps**

- Saying "prefer composition" without explaining the contract.
- Ignoring the borrowed lifetime.
- Hiding unbounded retry behind a cheap-looking call.
- Encoding failure as a measurement sentinel, which can collide with valid data
  or silently select an unsafe policy.
- Creating a decorator hierarchy for one fixed use case.

**Follow-up questions**

- When would inheritance be valid here?
- Should retry policy be compile-time or runtime?
- How would ownership change the member type?

### 13. How do interface segregation and dependency inversion work without a framework?

**Short answer**

Keep contracts focused and make high-level policy receive those contracts from
outside. Ordinary constructor parameters are often sufficient.

**Deep explanation**

Interface segregation prevents clients from depending on unrelated operations.
Dependency inversion separates stable policy from volatile details through an
abstraction owned by the policy boundary.

This does not require a container, service locator, global registry, or
reflection. A small interface and constructor injection can express the design
directly.

**C/C++ code or API anchor**

```cpp
class Clock {
public:
    virtual ~Clock() = default;
    virtual long long now_ms() const = 0;
};

class Scheduler {
public:
    explicit Scheduler(const Clock& clock)
        : clock_{clock}
    {
    }

private:
    const Clock& clock_;
};
```

**Production and debug angle**

Review whether the abstraction represents a real boundary or only adds
indirection. Small interfaces improve fakes and contract tests. Excessive
interfaces increase navigation, build cost, and mock-heavy tests detached from
real behavior.

**Common traps**

- Creating an interface for every concrete class.
- Using a service locator and calling it dependency injection.
- Combining unrelated operations in one "manager" interface.
- Ignoring lifetime because a dependency is injected.

**Follow-up questions**

- Who should own the abstraction?
- When is a template parameter better?
- How would you test every implementation's contract?

### 14. Explain multiple inheritance, virtual inheritance, and the diamond problem.

**Short answer**

Multiple inheritance gives a class multiple bases. A diamond can duplicate a
shared non-virtual base. Virtual inheritance creates one shared virtual base,
with added layout and initialization complexity.

**Deep explanation**

Multiple inheritance is clearest for independent interface bases with little or
no state. If two intermediate classes inherit the same ordinary base, the
most-derived object contains two base subobjects, causing duplication and
ambiguity.

Declaring the intermediate inheritance virtual makes the most-derived object
contain one shared virtual base. The most-derived constructor initializes that
virtual base.

Virtual inheritance solves shared-base identity; it does not prove that the
hierarchy is a good design.

**C/C++ code or API anchor**

```cpp
class Device {
public:
    explicit Device(int id) : id_{id} {}

private:
    int id_;
};

class Input : virtual public Device {
protected:
    Input() : Device{0} {}
};

class Output : virtual public Device {
protected:
    Output() : Device{0} {}
};

class Console final : public Input, public Output {
public:
    explicit Console(int id) : Device{id} {}
};
```

**Production and debug angle**

Inspect constructor order, pointer adjustment, cast behavior, object size, and
ABI constraints on the actual toolchain. Prefer multiple independent
interfaces or composition when shared state is unnecessary.

**Common traps**

- Saying every multiple inheritance design has a diamond.
- Expecting intermediate constructors to initialize the virtual base finally.
- Ignoring ABI and maintenance complexity.
- Using virtual inheritance only to remove a compiler ambiguity.

**Follow-up questions**

- How does destruction order work?
- Why are interface-only bases simpler?
- Can composition eliminate this diamond?

### 15. Choose among concrete types, virtual functions, templates, CRTP, and a closed variant.

**Short answer**

Choose from the required variability: concrete values for no substitution,
virtual functions for an open runtime set, templates/CRTP for compile-time
variation, and a closed sum type when all alternatives are known.

**Deep explanation**

Virtual dispatch supports adding new implementations without changing ordinary
call sites, but introduces base-interface and ABI considerations.

Templates enable type-specific generation and optimization, but expose
implementation in headers, increase compile coupling, and can grow code size.
CRTP is one static-polymorphism technique, not a universally faster virtual
replacement. A mixin is a small base used to add focused reusable behavior;
CRTP is one way to implement a mixin without runtime dispatch.

A closed variant-style design makes the alternative set explicit and often
works well when operations change more often than types. A concrete value type
is simplest when no variation is required.

**C/C++ code or API anchor**

```cpp
template <typename SensorType>
auto read_sensor(const SensorType& sensor)
{
    return sensor.read();
}
```

```cpp
class Sensor {
public:
    virtual ~Sensor() = default;
    virtual SensorReading read() const = 0;
};
```

**Production and debug angle**

Measure target code size, call frequency, cache behavior, build time, and binary
compatibility. Optimizers may devirtualize calls. Do not choose templates from
a fixed-cost performance slogan.

**Common traps**

- Claiming virtual dispatch is always slow.
- Ignoring template code size and build costs.
- Using CRTP where runtime substitution is required.
- Building a hierarchy for a closed two-case value.

**Follow-up questions**

- What makes an implementation set open or closed?
- When can virtual calls be devirtualized?
- What ABI costs do templates avoid or introduce?

### 16. Debug a dangling injected dependency and design the ownership boundary.

**Prompt**

```cpp
Alarm make_alarm()
{
    FixedSensor sensor{2700};
    return Alarm{sensor, 2500};
}
```

`Alarm` stores `const Sensor&`. Why is the returned object invalid, and how
should the API be redesigned?

**Short answer**

`Alarm` stores a borrowed reference to a local sensor. The sensor dies when
`make_alarm` returns, leaving a dangling reference.

**Deep explanation**

Dependency injection establishes a relationship, not ownership. A reference
member does not extend the referred object's lifetime. Returning the borrower
allows it to outlive its owner.

Valid redesigns depend on policy:

- require the caller to own both objects and construct them in safe order;
- make the alarm own a sensor through an ownership type;
- store a sensor value if polymorphism is unnecessary;
- return an aggregate owner that contains both objects in safe lifetime order.

Ownership types and move semantics are developed in Chapter 10.

**C/C++ code or API anchor**

```cpp
int main()
{
    FixedSensor sensor{2700};
    Alarm alarm{sensor, 2500};
    return alarm.active() ? 0 : 1;
}
```

**Production and debug angle**

ASan may report use-after-scope on an executed path. In GDB, compare the
reference address with the former stack frame. During review, mark stored
references as borrows and verify owner lifetime across returns, callbacks, and
threads.

**Common traps**

- Replacing the reference with a raw pointer without ownership policy.
- Assuming `const` extends lifetime.
- Allocating globally to hide the bug.
- Introducing shared ownership automatically.

**Follow-up questions**

- When should `Alarm` own the sensor?
- Can a reference member be reseated?
- How would a C operation-table context have the same bug?

## Coding Tasks

### Task 1. Design a testable alarm without dynamic allocation.

Create:

- a `Sensor` interface;
- `FixedSensor` and `SequenceSensor` implementations;
- an `Alarm` that borrows a sensor;
- contract tests that run against both implementations.

Required discussion:

- borrowed lifetime;
- virtual destructor policy;
- `override`;
- result/error domain;
- no hidden retry, allocation, or locking.

### Task 2. Implement equivalent C17 runtime dispatch.

Create a `sensor_ops` table, context object, wrapper call, and two
implementations. Validate null pointers and document initialization, ownership,
cleanup, and table-version policy.

### Task 3. Replace inheritance-for-reuse with composition.

Start with a derived class that inherits only to reuse logging or retry code.
Refactor it to contain or borrow a collaborator. Compare coupling, testability,
timing, and lifetime requirements.

## Debugging Scenarios

### Scenario 1. The derived function never runs.

Check:

- base declaration is `virtual`;
- derived signature exactly matches;
- `override` is present;
- call is through a pointer/reference to the original object;
- no slicing occurred;
- call is not inside construction/destruction.

### Scenario 2. `delete base_pointer` crashes or leaks.

Treat a non-virtual polymorphic deletion path as undefined behavior. Review the
base destructor policy, dynamic allocation family, actual ownership, duplicate
deletion, and whether deletion through the base should be permitted at all.

### Scenario 3. A base overload disappeared.

Look for a same-named declaration in the derived class. Reintroduce the base
overload set with `using Base::function` when that is the intended API.

### Scenario 4. A virtual function receives the wrong default value.

Default arguments come from the static type at the call site. Virtual dispatch
selects the function body dynamically. Remove inconsistent defaults or provide
a non-virtual public wrapper around a virtual implementation function.

## Rapid-Fire Review

1. Encapsulation versus abstraction?
2. What invariant does a class protect?
3. What is substitutability?
4. Dependency versus association?
5. Aggregation versus composition?
6. Why is ownership separate from relationship labels?
7. Overloading versus overriding?
8. Compile-time versus runtime polymorphism?
9. What does `override` verify?
10. What can `final` apply to?
11. What makes a class abstract?
12. Can a pure virtual function have a definition?
13. What is an interface in C++?
14. Why might an interface have a protected destructor?
15. What is object slicing?
16. Static type versus dynamic type?
17. What is a final overrider?
18. Are vtables required by the C++ standard?
19. Can a virtual call be inlined?
20. Why avoid virtual calls from constructors?
21. How do default arguments interact with virtual calls?
22. What causes name hiding?
23. Why prefer composition over inheritance?
24. When is multiple inheritance reasonable?
25. Who initializes a virtual base?
26. How does C implement runtime dispatch?
27. What does a context pointer own?
28. What is interface segregation?
29. What is dependency inversion?
30. When might CRTP be appropriate?

## Final Interview Checklist

A strong Chapter 09 candidate can:

- distinguish encapsulation, abstraction, and access control;
- classify dependency, association, aggregation, composition, and inheritance;
- justify public inheritance through substitutability;
- compare overloading and overriding;
- explain static and dynamic type;
- explain virtual dispatch without treating ABI layout as a language rule;
- use pure virtual functions and interface-style bases deliberately;
- choose a public virtual or protected non-virtual destructor;
- diagnose object slicing, hidden overloads, and mismatched overrides;
- explain constructor/destructor dispatch restrictions;
- compare C++ virtual calls with C function-pointer tables;
- prefer composition for implementation reuse;
- apply interface segregation and dependency inversion without ceremony;
- reason about multiple and virtual inheritance;
- choose among concrete values, templates, CRTP, closed alternatives, and
  virtual functions;
- make ownership and borrowed lifetime explicit;
- discuss performance using measurements rather than folklore.

## Reference Notes

- Virtual behavior, abstract classes, deletion, overriding, and inheritance are
  defined by the applicable C++ standard edition.
- Vtable layout, object representation, warnings, sanitizer support,
  devirtualization, and ABI compatibility are implementation-specific.
- The C++ Core Guidelines provide practical hierarchy, interface, destructor,
  slicing, and composition guidance.
