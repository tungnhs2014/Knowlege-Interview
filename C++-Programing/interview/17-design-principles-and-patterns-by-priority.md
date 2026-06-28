# 17 - Design Principles And Design Patterns By Priority Interview Pack

## How To Use This Pack

These questions test whether a candidate can design C++ code with principles
first and patterns second. Strong answers explain the problem, show the simple
solution, choose a pattern only when it reduces complexity, and discuss
ownership, lifetime, polymorphism, callbacks, iterator invalidation, and
debugging.

Every answer should avoid pattern-name theater. The interviewer is looking for
engineering judgment.

## Beginner Questions

### 1. What does "principles before patterns" mean?

**Short answer:** It means you first ask whether the code is cohesive, simple,
testable, and correctly owned. Only after that do you choose a pattern if the
problem really needs one.

**Deep explanation:** Design principles such as SRP, KISS, YAGNI, high
cohesion, low coupling, and composition over inheritance guide everyday design.
Patterns are named solutions for recurring problems: State for growing
state-specific behavior, Strategy for interchangeable algorithms, Observer for
multiple listeners, Adapter for incompatible interfaces, and so on. A pattern is
useful when it makes change safer. It is harmful when it creates structure for a
future that never arrives.

**C/C++ code/API anchor:**

```cpp
// Simple first: a predicate parameter is enough.
template <typename Predicate>
int count_if_sensor(const int* values, int size, Predicate predicate) {
    int count = 0;
    for (int i = 0; i < size; ++i) {
        if (predicate(values[i])) {
            ++count;
        }
    }
    return count;
}
```

**Production/debug angle:** During review, ask "what change does this design
make easier?" If nobody can answer, the abstraction may be premature.

**Traps:** Saying "use patterns everywhere"; assuming inheritance is more
"designed" than a function or lambda; using a full class hierarchy for one
branch.

**Follow-ups:** When would the predicate above become a Strategy object? What
would make the simple version insufficient?

### 2. Explain SRP, OCP, and ISP with C++ examples.

**Short answer:** SRP means one reason to change. OCP means extend behavior
without repeatedly editing fragile code. ISP means prefer small role-specific
interfaces over large interfaces with unused functions.

**Deep explanation:** SRP keeps classes cohesive. A `Sensor` should not also
own file persistence, network reporting, and UI rendering. OCP is often achieved
with callables, templates, or runtime interfaces so new behavior can be added as
new code. ISP prevents dummy implementations by splitting large interfaces into
roles like `Readable`, `Writable`, `Chargeable`, or `Resettable`.

**C/C++ code/API anchor:**

```cpp
class Readable {
public:
    virtual int read() = 0;
    virtual ~Readable() = default;
};

class Resettable {
public:
    virtual void reset() = 0;
    virtual ~Resettable() = default;
};
```

**Production/debug angle:** Small interfaces are easier to mock in tests and
reduce rebuild/refactor blast radius. They also make invalid states obvious:
objects do not need meaningless "not supported" methods.

**Traps:** Treating SRP as "one method per class"; claiming OCP always requires
inheritance; creating one huge `IDevice` interface.

**Follow-ups:** How does ISP relate to LSP? When is a template parameter a
better OCP extension point than a virtual interface?

### 3. What is composition over inheritance?

**Short answer:** Composition means a type owns or uses another object.
Inheritance means a type promises it is substitutable for a base type. Prefer
composition for behavior reuse; use inheritance for true "is-a" relationships.

**Deep explanation:** Public inheritance creates a contract: any derived object
must work wherever the base is expected. If the relationship is merely "uses
this helper" or "has this policy", composition is safer and less coupled. C++
composition can be a data member, constructor-injected dependency, lambda,
functor, `std::function`, or template policy.

**C/C++ code/API anchor:**

```cpp
class Logger {
public:
    void log(const char* text);
};

class Controller {
public:
    explicit Controller(Logger& logger) : logger_(logger) {}
    void start() { logger_.log("start"); }

private:
    Logger& logger_; // uses-a, not is-a
};
```

**Production/debug angle:** Composition makes ownership and testing clearer.
Inheritance adds virtual dispatch, slicing risks, and base-class evolution
risks.

**Traps:** Inheriting from `Logger` just to reuse `log`; passing polymorphic
objects by value; forgetting a virtual destructor in a polymorphic base.

**Follow-ups:** Give an example where inheritance is the right choice. How
would you test `Controller` if `Logger` were an interface?

### 4. Why is a virtual destructor important in C++ design?

**Short answer:** If a class is used polymorphically, its destructor should be
virtual so deleting through a base pointer destroys the complete derived object.

**Deep explanation:** Runtime polymorphism usually means using base pointers or
references. If a base class has virtual functions but a non-virtual destructor,
`delete base_ptr;` may not call the derived destructor. That can leak memory,
files, locks, or other RAII resources. A pure interface should almost always
have `virtual ~Base() = default;`.

**C/C++ code/API anchor:**

```cpp
class Strategy {
public:
    virtual int compute(int value) const = 0;
    virtual ~Strategy() = default;
};
```

**Production/debug angle:** In code review, search for classes with virtual
functions and non-virtual destructors. Sanitizers can catch some leaks, but the
design rule is simpler than debugging the failure.

**Traps:** Thinking `override` fixes destructor cleanup; deleting derived
objects through raw base pointers; assuming smart pointers solve a non-virtual
base destructor.

**Follow-ups:** When does a class not need a virtual destructor? How can
`std::unique_ptr<Base>` still be unsafe with the wrong base design?

## Mid-Level Questions

### 5. Compare callback vs Observer.

**Short answer:** A callback is usually one callable invoked later. Observer is
for a subject notifying multiple independent listeners with subscription and
lifetime rules.

**Deep explanation:** In C, a callback is often a function pointer plus
`void* user_data`. In C++, it may be a lambda, functor, template callback, or
`std::function`. Observer becomes useful when many subscribers must be notified,
added, removed, or owned independently. The design must define unsubscribe,
reentrancy, and object lifetime.

**C/C++ code/API anchor:**

```cpp
#include <functional>
#include <utility>
#include <vector>

class Subject {
public:
    using Callback = std::function<void(int)>;

    void add(Callback cb) {
        observers_.push_back(std::move(cb));
    }

    void notify(int value) {
        for (const auto& cb : observers_) {
            cb(value);
        }
    }

private:
    std::vector<Callback> observers_;
};
```

**Production/debug angle:** The simple version above is incomplete for
production: no unsubscribe token, no thread-safety, no mutation policy while
notifying. Debugging often starts with "which callback is still registered?"

**Traps:** Capturing stack references in stored lambdas; destroying an observer
without unregistering; modifying the observer list while iterating it; assuming
`std::function` has no overhead.

**Follow-ups:** Design an unsubscribe token. How would you make notification
safe if callbacks can add/remove observers?

### 6. Compare C FSM with C++ State pattern.

**Short answer:** A C FSM uses an enum plus switch or transition table. C++
State uses state objects behind a common interface. Start with the FSM; use
State when state-specific behavior grows and transitions become hard to manage.

**Deep explanation:** Small stable state machines are often clearer as
`enum class State` and explicit transitions. State pattern improves extension
when each state has substantial behavior and new states are frequent. But it can
over-engineer a tiny controller by scattering transitions across many classes.

**C/C++ code/API anchor:**

```cpp
enum class State { Idle, Active, Error };
enum class Event { Start, Fault, Reset };

State next(State state, Event event) {
    switch (state) {
    case State::Idle:
        return event == Event::Start ? State::Active : State::Idle;
    case State::Active:
        return event == Event::Fault ? State::Error : State::Active;
    case State::Error:
        return event == Event::Reset ? State::Idle : State::Error;
    }
    return State::Error;
}
```

**Production/debug angle:** Log transitions and test invalid events. For
embedded-style state machines, explicit transition tables are easy to inspect
and avoid surprise allocation.

**Traps:** Missing default/invalid transition behavior; duplicating transition
logic; using heap-allocated State objects where static state objects would do;
making transitions invisible.

**Follow-ups:** How would you unit-test the `next` function? When would you
split state behavior into separate classes?

### 7. Compare Strategy and State.

**Short answer:** Strategy swaps an algorithm or policy. State changes object
behavior as the object's internal state changes.

**Deep explanation:** Strategy is usually selected by configuration, caller, or
constructor injection. State is usually selected by the context as events cause
transitions. In C++, both can use interfaces, lambdas, functors, templates, or
`std::function`, so the difference is intent, not syntax.

**C/C++ code/API anchor:**

```cpp
template <typename Checksum>
int verify_packet(const int* data, int size, Checksum checksum) {
    int result = 0;
    for (int i = 0; i < size; ++i) {
        result = checksum(result, data[i]);
    }
    return result;
}
```

**Production/debug angle:** Strategy is easy to test independently. If a runtime
interface is used, ownership should be explicit with references or
`std::unique_ptr`. If templates are used, compile errors and build times become
part of the design cost.

**Traps:** Calling every function pointer a Strategy pattern; using State where
an injected policy is enough; using virtual interfaces in a hot path without a
reason.

**Follow-ups:** When would `std::function` be better than a template parameter?
When would a template parameter be better than `std::function`?

### 8. Explain Factory Method and its ownership risks.

**Short answer:** Factory Method defers object creation to an overridable method
or subclass. In C++, it should return ownership clearly, usually with
`std::unique_ptr` for polymorphic products.

**Deep explanation:** Factory Method is useful when creation varies by subtype,
configuration, or plugin-like extension. It is not needed when a constructor or
simple static function is enough. The main C++ danger is unclear ownership:
returning raw owning pointers makes leaks and double deletes likely.

**C/C++ code/API anchor:**

```cpp
#include <memory>

class Parser {
public:
    virtual void parse(const char* text) = 0;
    virtual ~Parser() = default;
};

class ParserFactory {
public:
    virtual std::unique_ptr<Parser> create() const = 0;
    virtual ~ParserFactory() = default;
};
```

**Production/debug angle:** Use ASan/LSan to catch ownership mistakes, but make
the API express ownership up front. If all callers always create the same
concrete type, remove the hierarchy.

**Traps:** Returning raw `Parser*`; using Abstract Factory for one product;
putting too much logic in the factory; hiding construction failure without error
context.

**Follow-ups:** How would you model creation failure? When is a static factory
function enough?

### 9. Compare Adapter and Facade.

**Short answer:** Adapter converts an incompatible interface into the interface
your code expects. Facade provides a simpler front door to a complex subsystem.

**Deep explanation:** Adapter usually wraps one legacy, C-style, vendor, or
mismatched API. Facade usually coordinates several classes or functions and
hides setup order. Adapter changes interface shape; Facade hides coordination.
Both should preserve ownership, error behavior, and semantics.

**C/C++ code/API anchor:**

```cpp
class TextSink {
public:
    virtual void write_text(const char* text) = 0;
    virtual ~TextSink() = default;
};

class LegacyPrinter {
public:
    void print(const char* text);
};

class PrinterAdapter : public TextSink {
public:
    explicit PrinterAdapter(LegacyPrinter& printer) : printer_(printer) {}
    void write_text(const char* text) override { printer_.print(text); }

private:
    LegacyPrinter& printer_;
};
```

**Production/debug angle:** For Adapter, test error translation and lifetime of
the wrapped object. For Facade, review whether it simplifies real usage or hides
important control.

**Traps:** Adapter that silently changes semantics; Facade that becomes a god
object; losing error details; storing a reference to a wrapped object that dies
too early.

**Follow-ups:** How would you make `PrinterAdapter` own the printer instead?
What is a sign that a Facade has become too large?

### 10. Debugging scenario: what is wrong with this Observer code?

```cpp
#include <functional>

std::function<void()> make_callback() {
    int local = 42;
    return [&local] {
        // uses local later
    };
}
```

**Short answer:** The lambda captures `local` by reference, but `local` is
destroyed when `make_callback` returns. Calling the stored callback later uses a
dangling reference.

**Deep explanation:** Stored callbacks extend the lifetime of the callable, not
the lifetime of referenced objects. Capturing by reference is safe only when the
callback cannot outlive the referenced object. Observer and Command designs are
especially vulnerable because callbacks are often stored for later.

**C/C++ code/API anchor:**

```cpp
std::function<void()> make_safe_callback() {
    int local = 42;
    return [local] {
        // uses a copied value
    };
}
```

**Production/debug angle:** ASan may catch some use-after-scope cases. Code
review should flag stored lambdas with reference captures. For object callbacks,
use explicit ownership or safe handles.

**Traps:** Assuming `std::function` copies everything captured by reference;
capturing `this` when the object can be destroyed; fixing every case with
`std::shared_ptr` without considering cycles.

**Follow-ups:** How would you avoid cycles with `std::weak_ptr`? How would you
design an unsubscribe token?

## Senior Questions

### 11. How do you decide between runtime polymorphism and static polymorphism?

**Short answer:** Use runtime polymorphism when the concrete behavior must vary
at runtime behind a stable interface. Use static polymorphism when types are
known at compile time and you want inlining, simpler ownership, or zero virtual
dispatch.

**Deep explanation:** Runtime polymorphism uses abstract classes, virtual
functions, base references/pointers, and usually RAII ownership such as
`std::unique_ptr<Base>`. It is good for plugins, dynamic configuration, and
stable ABI-like boundaries. Static polymorphism uses templates, overloads,
concepts, lambdas, and functors. It is good for STL-style algorithms and
performance-sensitive policies, but can increase compile-time cost and error
complexity.

**C/C++ code/API anchor:**

```cpp
template <typename Writer>
void publish(Writer& writer, int value) {
    writer.write(value); // static polymorphism
}

class Writer {
public:
    virtual void write(int value) = 0; // runtime polymorphism
    virtual ~Writer() = default;
};
```

**Production/debug angle:** Runtime polymorphism is easier to inspect in a
debugger at object boundaries, but virtual call stacks can hide concrete types.
Templates can generate faster code, but errors and build times matter.

**Traps:** Assuming virtual dispatch is always too slow; assuming templates are
always better; ignoring object slicing; forgetting virtual destructors; using
templates where runtime substitution is required.

**Follow-ups:** How would C++20 Concepts improve the static version? How would
you benchmark the virtual-call overhead meaningfully?

### 12. How would you design an Observer for production?

**Short answer:** Define subscription ownership, unsubscribe behavior,
notification order, reentrancy policy, error handling, and thread-safety policy.
A vector of callbacks is only the learning version.

**Deep explanation:** Observer becomes hard because subject and observer
lifetimes are independent. A production design often returns a subscription
token whose destructor unregisters, or requires explicit `unsubscribe`.
Callbacks may remove themselves during notification, so iteration must tolerate
mutation or defer changes. If callbacks cross threads, synchronization and
deadlock avoidance become design requirements.

**C/C++ code/API anchor:**

```cpp
class Subscription {
public:
    Subscription() = default;
    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;
    Subscription(Subscription&&) noexcept = default;
    Subscription& operator=(Subscription&&) noexcept = default;
    ~Subscription(); // unregisters if still active
};
```

**Production/debug angle:** Log subscription IDs, assert valid unsubscribe
paths, and stress-test callbacks that add/remove observers. If concurrent, use
TSan and avoid calling user code while holding locks when possible.

**Traps:** Holding a mutex while invoking arbitrary callbacks; unregistering
from a vector while iterating it; lifetime cycles through `std::shared_ptr`;
assuming notification order is irrelevant without documenting it.

**Follow-ups:** Would you store `std::function`, raw observer pointers, or
`std::weak_ptr`? How do you avoid deadlocks during notification?

### 13. Coding task: refactor a fat interface using ISP and DIP.

```cpp
class Device {
public:
    virtual int read() = 0;
    virtual void write(int value) = 0;
    virtual void charge() = 0;
    virtual void reset() = 0;
    virtual ~Device() = default;
};
```

**Short answer:** Split the interface into role interfaces such as `Readable`,
`Writable`, `Chargeable`, and `Resettable`, then depend only on the role each
client needs.

**Deep explanation:** The original interface violates ISP because every device
must implement operations it may not support. It also weakens DIP because
clients depend on a broad abstraction rather than the small abstraction they
need. Smaller interfaces improve substitutability, tests, and maintenance.

**C/C++ code/API anchor:**

```cpp
class Readable {
public:
    virtual int read() = 0;
    virtual ~Readable() = default;
};

class Writable {
public:
    virtual void write(int value) = 0;
    virtual ~Writable() = default;
};

class Resettable {
public:
    virtual void reset() = 0;
    virtual ~Resettable() = default;
};

void poll_sensor(Readable& sensor) {
    (void)sensor.read();
}
```

**Production/debug angle:** Smaller interfaces reduce mock size and make
unsupported operations impossible at compile time. Tests can use focused fakes.

**Traps:** Replacing one fat interface with many tiny interfaces that always
travel together; forgetting virtual destructors; using inheritance where a
plain callable would be enough.

**Follow-ups:** When would a concept/template be better than these virtual
interfaces? How would you handle a device that is both `Readable` and
`Writable`?

### 14. Explain object slicing and why it matters for patterns.

**Short answer:** Object slicing happens when a derived object is copied into a
base object by value, losing the derived part. It breaks polymorphic designs
such as Strategy, State, Command, Prototype, and interface-based Decorator.

**Deep explanation:** Runtime polymorphism requires preserving the dynamic
object. Passing by value constructs only the base subobject, so virtual behavior
and derived data are lost. The usual fix is to pass by reference or pointer and
own polymorphic objects through `std::unique_ptr<Base>` when ownership is
needed.

**C/C++ code/API anchor:**

```cpp
class Command {
public:
    virtual void execute() = 0;
    virtual ~Command() = default;
};

void run(Command& command) { // reference avoids slicing
    command.execute();
}
```

**Production/debug angle:** Search for function parameters like `Base b` or
containers like `std::vector<Base>` in polymorphic hierarchies. Prefer
`std::vector<std::unique_ptr<Base>>` or value-based alternatives such as
`std::variant` when the set of types is closed.

**Traps:** Storing derived objects in `std::vector<Base>`; returning `Base` by
value from a factory; copying polymorphic objects without a virtual `clone`
policy.

**Follow-ups:** When would Prototype with `clone()` be appropriate? When would
`std::variant` be better than virtual polymorphism?

### 15. Compare Decorator and Proxy in a code review.

**Short answer:** Decorator adds behavior while preserving an interface. Proxy
controls access while preserving an interface. Both wrap another object, but
their intent differs.

**Deep explanation:** Decorator is useful for logging, metrics, validation,
compression, buffering, or retry wrappers. Proxy is useful for lazy creation,
access control, caching, remote forwarding, or guarding expensive objects.
Both should use composition, make ownership clear, and document any behavior or
cost changes.

**C/C++ code/API anchor:**

```cpp
class DataSource {
public:
    virtual int read() = 0;
    virtual ~DataSource() = default;
};

class LoggingSource : public DataSource {
public:
    explicit LoggingSource(DataSource& inner) : inner_(inner) {}
    int read() override {
        int value = inner_.read();
        // log value
        return value;
    }

private:
    DataSource& inner_;
};
```

**Production/debug angle:** Wrapper stacks can obscure call paths. In debugger
or logs, include wrapper names and avoid swallowing errors from the wrapped
object.

**Traps:** Deep wrapper chains; reference members outliving wrapped objects;
Proxy surprising callers with lazy I/O or caching; Decorator changing semantics
instead of adding behavior.

**Follow-ups:** How would you make `LoggingSource` own its inner source? How do
you avoid wrapper cycles?

### 16. Senior design task: choose patterns for a packet-processing module.

You need to process packets from several devices. Each packet type has a parser,
validation strategy, stateful connection handling, and optional subscribers for
diagnostics. Which patterns or simple mechanisms would you choose?

**Short answer:** Start simple: use a Factory Method or static factory for
parser creation, Strategy for validation if algorithms vary, an explicit FSM or
State pattern for connection state, and Observer only if multiple diagnostic
subscribers are real.

**Deep explanation:** Parser creation may vary by packet type, so a factory can
centralize creation and ownership with `std::unique_ptr<Parser>`. Validation may
be a lambda/template policy if compile-time or local, or a runtime Strategy if
configured dynamically. Connection state should start as `enum class` plus
transition table; move to State objects if each state grows behavior. Diagnostics
can be a callback for one listener or Observer for many listeners.

**C/C++ code/API anchor:**

```cpp
#include <memory>

class Parser {
public:
    virtual bool parse(const unsigned char* data, int size) = 0;
    virtual ~Parser() = default;
};

class ParserFactory {
public:
    virtual std::unique_ptr<Parser> create(int packet_type) const = 0;
    virtual ~ParserFactory() = default;
};
```

**Production/debug angle:** Define ownership and error paths first. Add logging
for state transitions and parser selection. Use sanitizers for ownership bugs
and unit-test invalid packets, invalid transitions, and subscriber lifetime.

**Traps:** Using every named pattern at once; creating Abstract Factory without
families of related products; making Observer concurrent without a lock policy;
hiding packet errors behind a Facade with no detail.

**Follow-ups:** Which part would you implement first without patterns? What
would make you replace the FSM table with State objects?

## Coding And Debugging Tasks

### Task 1. Implement Strategy twice.

Implement packet checksum selection once with a template/lambda and once with a
runtime `Checksum` interface.

**Expected answer shape:** Explain that the template version is simple and
inline-friendly when the policy is known at compile time. Explain that the
runtime interface is useful when the algorithm is configured dynamically. Both
versions should make ownership and lifetime explicit.

### Task 2. Debug the missing virtual destructor.

Given a base class with virtual methods but non-virtual destructor, explain the
bug and fix it.

**Expected answer shape:** The candidate should state that deleting through the
base pointer may skip the derived destructor. The fix is `virtual ~Base() =
default;`, plus using RAII ownership such as `std::unique_ptr<Base>`.

### Task 3. Design an unsubscribe-safe Observer.

Sketch a `Subject::subscribe` API that returns a token.

**Expected answer shape:** The token should be move-only, should unregister in
its destructor or through explicit cancel, and the design should define what
happens when observers unsubscribe during notification.

### Task 4. Refactor inheritance into composition.

Given a class that inherits from `Logger` only to call `log`, refactor it to
receive `Logger&` or a logging callback.

**Expected answer shape:** The candidate should explain "uses-a" vs "is-a",
show constructor injection, and mention testing with a fake logger.

## Quick Evaluation Rubric

- Beginner: can explain principles, virtual destructors, and composition vs
  inheritance without slogans.
- Mid-level: can choose simple C++ mechanisms before patterns and discuss
  ownership/lifetime.
- Senior: can reason about substitution, slicing, callback lifetime,
  concurrency policy, template vs virtual tradeoffs, and over-engineering.
