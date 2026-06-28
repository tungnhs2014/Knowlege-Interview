# 17 - Design Principles And Design Patterns By Priority

## 1. Goal

After this lesson, you should be able to design C++ code with principles first
and patterns second:

- explain SOLID, SRP, OCP, LSP, ISP, DIP, DRY, KISS, YAGNI, high cohesion, low
  coupling, and composition over inheritance;
- know when a plain function, class, lambda, template, or `enum class` FSM is
  enough;
- use high-priority patterns only when they solve a real maintainability
  problem;
- implement and compare the MUST patterns: State / FSM, Strategy, Observer,
  Factory Method, Adapter, Facade, and Command;
- recognize the SHOULD patterns: Builder, Decorator, Proxy, Template Method,
  Chain of Responsibility, Mediator, Iterator, Composite, and Prototype;
- recognize the NICE patterns: Visitor, Memento, Flyweight, Bridge, and
  Abstract Factory;
- avoid over-engineering, ownership confusion, object slicing, dangling
  callbacks, and fragile inheritance.

The main idea is simple: design patterns are names for recurring solutions, not
badges to collect. In real C++, the best design is often the smallest design
that keeps ownership, lifetime, invariants, and change points clear.

## 2. Why It Matters

Design problems usually start small:

- a sensor has three operating states;
- a parser supports two formats;
- a logger sends output to multiple destinations;
- an old C API has an inconvenient interface;
- a command needs to be queued, retried, or undone;
- a subsystem requires several setup calls in the right order.

At first, a direct function or `switch` may be perfect. Later, the code may gain
new states, algorithms, listeners, product types, wrappers, or command queues.
That is when principles and patterns help.

Good design reduces the cost of change:

- adding a new behavior should not break old behavior;
- changing one module should not ripple through unrelated modules;
- interfaces should be small enough to understand and mock;
- resource ownership should be visible;
- callbacks should not outlive the objects they touch;
- polymorphic bases should clean up correctly.

Bad design often looks clever early and expensive later:

- too many tiny classes for a simple `switch`;
- inheritance used only to share code;
- callbacks that capture dead stack variables;
- factories returning raw owning pointers;
- a Facade that becomes a hidden god object;
- an Observer list that cannot safely unsubscribe.

## 3. Mental Model

Think in three layers.

Layer 1 is the problem. Ask what changes and what stays stable:

- states change?
- algorithms change?
- listeners come and go?
- object creation varies?
- an external interface is awkward?
- subsystem setup is complex?
- actions need to be stored or replayed?

Layer 2 is the simplest C++ mechanism:

- free function;
- small class with private data;
- `enum class` and `switch`;
- `std::vector`, `std::map`, `std::unordered_map`, or another container;
- lambda or functor;
- template parameter;
- `std::function` only when type erasure and storage are useful;
- abstract interface only when runtime substitution is needed.

Layer 3 is the named pattern, used only when the simple mechanism no longer
keeps the design clear.

Example:

- one callback: use a lambda or function pointer;
- many subscribers with lifetime policy: consider Observer;
- one algorithm parameter: use a function or lambda;
- many interchangeable runtime algorithms: consider Strategy;
- one small state machine: use `enum class` and a transition table;
- state-specific behavior keeps growing: consider State.

This is the "principles before patterns" mindset.

## 4. Mechanism

### Design Principles

Design principles are decision rules. They help you judge whether a type or
module is easy to change safely.

**SRP - Single Responsibility Principle**

A class or module should have one clear reason to change.

Bad shape:

```cpp
class SensorManager {
public:
    void read_sensor();
    void filter_data();
    void save_to_file();
    void send_to_network();
    void draw_ui();
};
```

This class changes for sensor hardware, filtering, storage, networking, and UI.
That is too many reasons.

Better shape:

```cpp
struct Reading {
    double value{};
};

class Sensor {
public:
    Reading read() const;
};

class Filter {
public:
    Reading apply(Reading reading) const;
};

class ReadingStore {
public:
    void save(Reading reading);
};
```

SRP does not mean every function becomes its own class. It means a type has a
cohesive purpose.

**OCP - Open/Closed Principle**

Code should be open for extension but closed for modification. In practice, this
means new behavior should often be added by adding a new type, function, or
policy rather than editing a large fragile block.

Simple C++ extension point:

```cpp
#include <functional>
#include <vector>

int count_if_custom(const std::vector<int>& values,
                    const std::function<bool(int)>& predicate) {
    int count = 0;
    for (int value : values) {
        if (predicate(value)) {
            ++count;
        }
    }
    return count;
}
```

For performance-sensitive code, a template callback avoids `std::function`
overhead:

```cpp
#include <vector>

template <typename Predicate>
int count_if_fast(const std::vector<int>& values, Predicate predicate) {
    int count = 0;
    for (int value : values) {
        if (predicate(value)) {
            ++count;
        }
    }
    return count;
}
```

**LSP - Liskov Substitution Principle**

If `Derived` publicly inherits from `Base`, code that expects `Base` should work
correctly with `Derived`.

Bad design example:

```cpp
class File {
public:
    virtual void write(const char* text) = 0;
    virtual ~File() = default;
};

class ReadOnlyFile : public File {
public:
    void write(const char*) override {
        // Violates expectation: a File should be writable.
    }
};
```

Better design: separate roles.

```cpp
class Readable {
public:
    virtual const char* read() const = 0;
    virtual ~Readable() = default;
};

class Writable {
public:
    virtual void write(const char* text) = 0;
    virtual ~Writable() = default;
};
```

This also supports ISP.

**ISP - Interface Segregation Principle**

Clients should not be forced to depend on functions they do not use.

Bad shape:

```cpp
class Worker {
public:
    virtual void work() = 0;
    virtual void eat() = 0;
    virtual void charge() = 0;
    virtual ~Worker() = default;
};
```

Better shape:

```cpp
class Workable {
public:
    virtual void work() = 0;
    virtual ~Workable() = default;
};

class Chargeable {
public:
    virtual void charge() = 0;
    virtual ~Chargeable() = default;
};
```

Small interfaces reduce dummy methods and make tests easier.

**DIP - Dependency Inversion Principle**

High-level code should depend on abstractions, not concrete details.

```cpp
#include <string>

class Logger {
public:
    virtual void log(const std::string& message) = 0;
    virtual ~Logger() = default;
};

class Controller {
public:
    explicit Controller(Logger& logger) : logger_(logger) {}

    void start() {
        logger_.log("controller started");
    }

private:
    Logger& logger_;
};
```

`Controller` does not know whether logging goes to console, file, memory, or a
test double.

**DRY - Do Not Repeat Yourself**

Avoid duplicated knowledge, not just duplicated lines. If the same transition
rule, validation rule, or ownership rule appears in several places, centralize
it.

**KISS - Keep It Simple**

Prefer the simplest design that makes correct use easy.

**YAGNI - You Aren't Gonna Need It**

Do not build extensibility for imaginary future requirements. Extension points
have costs: more indirection, more tests, more lifetime rules, and more names to
learn.

**High Cohesion And Low Coupling**

Cohesion means the parts of a module belong together. Coupling means how much
one module knows about another. Good C++ design keeps related state and behavior
together, but hides unnecessary implementation details.

**Composition Over Inheritance**

Composition means a class uses another object as a member or collaborator.
Inheritance means a class promises substitutability through a base interface.
Use inheritance for "is-a"; use composition for "has-a" or "uses-a".

### Runtime Polymorphism

Runtime polymorphism uses virtual functions:

```cpp
#include <iostream>

class Sink {
public:
    virtual void write(int value) = 0;
    virtual ~Sink() = default;
};

class ConsoleSink : public Sink {
public:
    void write(int value) override {
        std::cout << value << '\n';
    }
};

void publish(Sink& sink, int value) {
    sink.write(value);
}
```

Rules:

- pass polymorphic objects by reference or pointer, not by value;
- add a virtual destructor to polymorphic bases;
- use `override` on every override;
- design the base interface so derived classes can honestly satisfy it.

### Static Polymorphism

Static polymorphism uses templates, overloads, lambdas, functors, or concepts.
It often gives zero runtime dispatch overhead.

```cpp
#include <iostream>

template <typename Sink>
void publish_fast(Sink& sink, int value) {
    sink.write(value);
}

struct ConsoleSink {
    void write(int value) {
        std::cout << value << '\n';
    }
};
```

Use static polymorphism when types are known at compile time and you do not need
runtime plugin-style substitution.

### Callables

C++ has several callable forms:

- function pointer;
- functor with `operator()`;
- lambda;
- template callback parameter;
- `std::function` for stored type-erased callbacks.

```cpp
#include <algorithm>
#include <iostream>
#include <vector>

int main() {
    std::vector<int> values{1, 2, 3, 4};

    std::for_each(values.begin(), values.end(), [](int value) {
        std::cout << value << '\n';
    });
}
```

This is the foundation for Strategy, Observer, and Command.

## 5. C/C++ API And Code

### C-Style FSM

For small stable state machines, a C-style FSM is often clearer than a class
hierarchy.

```c
#include <stdio.h>

typedef enum {
    STATE_IDLE,
    STATE_ACTIVE,
    STATE_ERROR
} State;

State next_state(State state, int event) {
    switch (state) {
    case STATE_IDLE:
        return event == 1 ? STATE_ACTIVE : STATE_IDLE;
    case STATE_ACTIVE:
        return event == 9 ? STATE_ERROR : STATE_ACTIVE;
    case STATE_ERROR:
        return event == 0 ? STATE_IDLE : STATE_ERROR;
    }
    return STATE_ERROR;
}

int main(void) {
    State state = STATE_IDLE;
    state = next_state(state, 1);
    printf("%d\n", state);
}
```

Build:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic fsm.c -o fsm
```

### C-Style Strategy Callback

C callbacks commonly use a function pointer plus optional context.

```c
#include <stddef.h>
#include <stdio.h>

typedef int (*ScoreFn)(int value, void* user_data);

int count_matching(const int* values, size_t size,
                   ScoreFn score, void* user_data) {
    int count = 0;
    for (size_t i = 0; i < size; ++i) {
        if (score(values[i], user_data)) {
            ++count;
        }
    }
    return count;
}

int above_limit(int value, void* user_data) {
    int limit = *(int*)user_data;
    return value > limit;
}

int main(void) {
    int values[] = {1, 4, 7, 9};
    int limit = 5;
    printf("%d\n", count_matching(values, 4, above_limit, &limit));
}
```

Risk: the caller must ensure `user_data` remains valid for the whole callback
use. If a callback is stored for later, stack context can dangle.

## 6. C++ API And Code

### Strategy With A Lambda

For many Strategy cases, a lambda is enough.

```cpp
#include <iostream>
#include <vector>

template <typename Predicate>
int count_matching(const std::vector<int>& values, Predicate predicate) {
    int count = 0;
    for (int value : values) {
        if (predicate(value)) {
            ++count;
        }
    }
    return count;
}

int main() {
    std::vector<int> values{1, 4, 7, 9};
    int limit = 5;

    int count = count_matching(values, [limit](int value) {
        return value > limit;
    });

    std::cout << count << '\n';
}
```

Build:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic strategy_lambda.cpp -o strategy_lambda
```

### Strategy With Runtime Interface

Use a runtime interface when the concrete algorithm must be selected and stored
at runtime.

```cpp
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

class Checksum {
public:
    virtual int compute(const std::vector<int>& bytes) const = 0;
    virtual ~Checksum() = default;
};

class SumChecksum : public Checksum {
public:
    int compute(const std::vector<int>& bytes) const override {
        int sum = 0;
        for (int b : bytes) {
            sum += b;
        }
        return sum;
    }
};

class Device {
public:
    explicit Device(std::unique_ptr<Checksum> checksum)
        : checksum_(std::move(checksum)) {}

    int verify(const std::vector<int>& packet) const {
        return checksum_->compute(packet);
    }

private:
    std::unique_ptr<Checksum> checksum_;
};

int main() {
    Device device(std::make_unique<SumChecksum>());
    std::cout << device.verify({1, 2, 3}) << '\n';
}
```

Ownership is explicit: `Device` owns the `Checksum` strategy.

### Observer With Stored Callbacks

```cpp
#include <functional>
#include <iostream>
#include <utility>
#include <vector>

class Subject {
public:
    using Observer = std::function<void(int)>;

    void add_observer(Observer observer) {
        observers_.push_back(std::move(observer));
    }

    void set_value(int value) {
        value_ = value;
        for (const auto& observer : observers_) {
            observer(value_);
        }
    }

private:
    int value_{};
    std::vector<Observer> observers_;
};

int main() {
    Subject subject;
    subject.add_observer([](int value) {
        std::cout << "value = " << value << '\n';
    });

    subject.set_value(42);
}
```

Learning-only warning: this simple version has no unsubscribe handle and no
thread-safety. Production Observer designs must define lifetime,
unsubscription, reentrancy, and concurrency policy.

### Factory Method Returning Ownership

```cpp
#include <iostream>
#include <memory>
#include <string>

class Parser {
public:
    virtual void parse(const std::string& text) = 0;
    virtual ~Parser() = default;
};

class JsonParser : public Parser {
public:
    void parse(const std::string& text) override {
        std::cout << "json: " << text << '\n';
    }
};

class ParserFactory {
public:
    virtual std::unique_ptr<Parser> create() const = 0;
    virtual ~ParserFactory() = default;
};

class JsonParserFactory : public ParserFactory {
public:
    std::unique_ptr<Parser> create() const override {
        return std::make_unique<JsonParser>();
    }
};

int main() {
    JsonParserFactory factory;
    auto parser = factory.create();
    parser->parse("{}");
}
```

If you only have one concrete parser, this hierarchy is probably unnecessary. A
plain constructor or static factory function is simpler.

### Adapter Around An Incompatible Interface

```cpp
#include <iostream>
#include <string>

class TextSink {
public:
    virtual void write_text(const std::string& text) = 0;
    virtual ~TextSink() = default;
};

class LegacyPrinter {
public:
    void print(const char* text) {
        std::cout << text << '\n';
    }
};

class PrinterAdapter : public TextSink {
public:
    explicit PrinterAdapter(LegacyPrinter& printer) : printer_(printer) {}

    void write_text(const std::string& text) override {
        printer_.print(text.c_str());
    }

private:
    LegacyPrinter& printer_;
};
```

The Adapter translates interface shape. It should not silently change semantics
or hide important errors.

### Facade For A Small Subsystem

```cpp
#include <iostream>
#include <string>

class FileSink {
public:
    void open(const std::string& path) {
        std::cout << "open " << path << '\n';
    }

    void write(const std::string& message) {
        std::cout << "file: " << message << '\n';
    }
};

class ConsoleSink {
public:
    void write(const std::string& message) {
        std::cout << "console: " << message << '\n';
    }
};

class LoggerFacade {
public:
    explicit LoggerFacade(const std::string& path) {
        file_.open(path);
    }

    void info(const std::string& message) {
        file_.write(message);
        console_.write(message);
    }

private:
    FileSink file_;
    ConsoleSink console_;
};
```

The Facade hides setup and coordination. It should stay focused; if every
subsystem feature is dumped into it, it becomes a god object.

### Command As A Stored Action

```cpp
#include <functional>
#include <iostream>
#include <utility>
#include <vector>

class CommandQueue {
public:
    void push(std::function<void()> command) {
        commands_.push_back(std::move(command));
    }

    void run_all() {
        for (const auto& command : commands_) {
            command();
        }
        commands_.clear();
    }

private:
    std::vector<std::function<void()>> commands_;
};

int main() {
    int counter = 0;
    CommandQueue queue;

    queue.push([&counter] { ++counter; });
    queue.push([&counter] { counter += 10; });
    queue.run_all();

    std::cout << counter << '\n';
}
```

Warning: this captures `counter` by reference. That is safe only because
`counter` outlives the queue execution. If commands can outlive the current
scope, capture by value or use explicit shared ownership.

## 7. Practical Usage

### Principles In Daily Code Review

Ask these questions before naming a pattern:

- What invariant does this class protect?
- Does this type have one clear reason to change?
- Is this dependency easy to replace in a test?
- Is this public inheritance truly substitutable?
- Can composition express the relationship more clearly?
- Does this callback outlive captured data?
- Who owns objects created by this factory?
- Does this interface force clients to implement unused functions?
- Will a future maintainer understand the simple path first?

### Pattern Priority In Practice

Use this practical priority order:

1. Apply principles: SRP, KISS, YAGNI, high cohesion, low coupling.
2. Use simple C++ mechanisms: function, class, lambda, template, container.
3. Use MUST patterns when the problem is real and recurring.
4. Use SHOULD patterns when their specific use case appears.
5. Keep NICE patterns as vocabulary until you need them.

### MUST Pattern Use Cases

State / FSM:

- protocol parser state;
- device connection state;
- UI mode state;
- command processing lifecycle.

Strategy:

- checksum algorithm;
- sorting/ranking policy;
- retry policy;
- compression or filtering policy.

Observer:

- telemetry listeners;
- model-view notification;
- event subscription;
- status updates to multiple modules.

Factory Method:

- parser creation by file type;
- plugin object creation by configuration;
- test production of fake implementations.

Adapter:

- legacy C API wrapper;
- vendor API wrapper;
- interface migration layer;
- type/error translation boundary.

Facade:

- logger subsystem;
- communication subsystem setup;
- configuration load/save workflow;
- a small API over several cooperating objects.

Command:

- undo/redo;
- deferred execution;
- command queue;
- retryable operation.

### SHOULD Pattern Use Cases

Builder:

- many optional construction parameters;
- validation before final object creation;
- fluent configuration.

Decorator:

- add logging, metrics, compression, validation, buffering.

Proxy:

- lazy creation, access control, caching, remote-call wrapper.

Template Method:

- fixed algorithm skeleton with overridable steps.

Chain of Responsibility:

- request handlers tried in order;
- validation pipeline;
- message processing chain.

Mediator:

- many objects communicate through a coordinator.

Iterator:

- custom collection traversal;
- hiding container internals behind a stable traversal interface.

Composite:

- tree of objects with a shared interface.

Prototype:

- clone objects without knowing exact concrete type.

### NICE Pattern Awareness

Visitor is useful when you have a stable type hierarchy and want to add
operations, but it can be heavy.

Memento snapshots state for restore/undo while preserving encapsulation.

Flyweight shares repeated immutable state to reduce memory use.

Bridge separates abstraction from implementation when both vary independently.

Abstract Factory creates families of related products. Do not use it when a
simple Factory Method or constructor is enough.

## 8. Comparisons

### Callback Vs Observer

| Topic | Callback | Observer | Practical Guidance |
| --- | --- | --- | --- |
| Shape | One callable passed to be invoked later. | Subject maintains one or more subscribers. | Use callback for one hook; use Observer for multiple independent listeners. |
| State | Function pointer has no state; lambda/functor/`std::function` can hold state. | Observers often have identity and unsubscribe behavior. | Observer needs stronger lifetime management. |
| C style | Function pointer plus `void* user_data`. | List of callback slots plus contexts. | Keep context lifetime explicit. |
| C++ style | Lambda, functor, template callback, or `std::function`. | Interface observers or callback list. | Prefer simple callback until subscription management is real. |
| Trap | Dangling captured reference. | Destroyed observer still registered. | Define ownership and unsubscribe policy. |

### FSM In C Vs State Pattern In C++

| Topic | C FSM | C++ State Pattern | Practical Guidance |
| --- | --- | --- | --- |
| Shape | `enum` state plus `switch` or transition table. | State objects implement behavior through common interface. | Start with FSM for small stable state machines. |
| Extensibility | Add state by editing switch/table. | Add state class and transition logic. | State helps when state-specific behavior keeps growing. |
| Memory | Predictable and explicit. | May involve objects and dynamic allocation unless designed carefully. | Prefer static state objects for constrained systems. |
| Trap | Missing transition. | Too many classes for tiny logic. | Keep transitions visible and tested. |

### Strategy Vs State

| Topic | Strategy | State | Practical Guidance |
| --- | --- | --- | --- |
| Intent | Swap an algorithm or policy. | Change object behavior as internal state changes. | Strategy is chosen from outside; State changes from inside. |
| Lifetime | Often stable during operation. | Changes as the context progresses. | Look at who controls the behavior change. |
| C++ form | Lambda, functor, template, `std::function`, or interface. | `enum class` FSM or State interface. | Use the simplest representation that makes transitions clear. |

### Adapter Vs Facade

| Topic | Adapter | Facade | Practical Guidance |
| --- | --- | --- | --- |
| Intent | Make an incompatible interface fit an expected interface. | Provide a simpler interface to a complex subsystem. | Adapter changes shape; Facade hides coordination. |
| Scope | Usually wraps one API/class. | Usually coordinates several classes/functions. | Keep both thin and testable. |
| Trap | Silently changing semantics. | Becoming a god object. | Preserve errors and keep responsibilities focused. |

### Factory Method Vs Abstract Factory

| Topic | Factory Method | Abstract Factory | Practical Guidance |
| --- | --- | --- | --- |
| Intent | Defer creation of one product type. | Create families of related products. | Factory Method is higher priority here; Abstract Factory is awareness-level. |
| C++ form | Virtual `create()` or overridable creation hook. | Factory interface with multiple create functions. | Use constructors/static factories when polymorphic creation is unnecessary. |
| Trap | Hierarchy for one concrete type. | Family factory without a real product family. | Let the creation problem justify the abstraction. |

### Decorator Vs Proxy

| Topic | Decorator | Proxy | Practical Guidance |
| --- | --- | --- | --- |
| Intent | Add behavior around an object. | Control access to an object. | Decorator enhances; Proxy guards, delays, caches, or forwards. |
| Examples | Logging, metrics, compression. | Lazy loading, access control, remote object. | Both usually use composition. |
| Trap | Wrapper stacks hide behavior. | Proxy changes cost/lifetime unexpectedly. | Document ownership and behavior changes. |

### Inheritance Vs Composition

| Topic | Inheritance | Composition | Practical Guidance |
| --- | --- | --- | --- |
| Relationship | "is-a" and substitutable. | "has-a" or "uses-a". | Prefer composition unless substitutability is real. |
| C++ mechanism | Public inheritance, virtual functions, base references/pointers. | Members, constructor injection, templates, callables. | Composition is usually easier to test and refactor. |
| Risks | Slicing, fragile base, missing virtual destructor, diamond complexity. | Too much forwarding or unclear ownership. | Name ownership and dependencies clearly. |

## 9. Common Bugs

- Starting with a pattern before the problem requires it.
- Teaching or using all patterns with equal depth.
- Using inheritance only to reuse code.
- Violating LSP by making a derived class reject valid base operations.
- Building fat interfaces that force dummy methods.
- Depending directly on concrete classes when testability needs an abstraction.
- Overusing `friend`, global state, singletons, or static mutable data.
- Forgetting a virtual destructor in a polymorphic base class.
- Passing polymorphic objects by value and causing object slicing.
- Returning raw owning pointers from factories.
- Storing callbacks that capture references to destroyed objects.
- Using `std::function` everywhere in hot paths when a template callback would
  be clearer and faster.
- Letting a Facade hide too much and become a god object.
- Building an Adapter that drops error information or changes semantics.
- Creating deep Decorator/Proxy stacks that make call flow hard to debug.
- Keeping iterators after container mutation without checking invalidation
  rules.
- Replacing simple runtime code with template-heavy code that produces poor
  diagnostics and long build times.

## 10. Debugging

Compile examples with strong warnings:

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow main.cpp -o main
```

Use sanitizers when examples involve ownership, polymorphism, callbacks, or
containers:

```bash
g++ -std=c++17 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer main.cpp -o main_san
```

Use ThreadSanitizer only when Observer/callback designs become concurrent:

```bash
g++ -std=c++17 -g -O1 -fsanitize=thread -fno-omit-frame-pointer main.cpp -o main_tsan
```

Design-debug checklist:

- Log state transitions in FSM/State code.
- Unit-test each Strategy independently.
- Test Observer unsubscribe and subject destruction.
- Test Factory Method ownership with sanitizers.
- Test Adapter error translation.
- Check Facade responsibilities during review.
- Use `gdb` backtraces to inspect virtual call paths and Command/Decorator
  stacks.
- Search for polymorphic pass-by-value.
- Search for base classes with virtual functions but non-virtual destructors.
- Search for lambdas stored beyond the lifetime of captured references.

## 11. Best Practices

- Principles before patterns.
- Use KISS and YAGNI until variability is real.
- Prefer high cohesion and low coupling.
- Keep interfaces small and role-specific.
- Prefer composition over inheritance for behavior reuse.
- Use public inheritance only for true substitutability.
- Use abstract interfaces for stable runtime boundaries.
- Use `override` on every override.
- Use `final` only when the design intentionally forbids extension.
- Add virtual destructors to polymorphic bases.
- Prefer RAII and smart pointers for ownership.
- Prefer `std::unique_ptr` for exclusive factory ownership.
- Prefer lambdas, functors, or templates for simple Strategy and Command cases.
- Use `std::function` when callbacks must be stored type-erased.
- Keep Adapter and Facade wrappers thin and testable.
- Define Observer unsubscription and lifetime rules.
- For small FSMs, prefer explicit `enum class` states and transition tables
  before State objects.
- Document ownership, lifetime, threading, and error behavior at pattern
  boundaries.

## 12. Interview Readiness

A strong design-pattern interview answer has this shape:

1. Start with the problem.
2. Show the simple solution.
3. Explain when the pattern becomes useful.
4. Name the C++ mechanism: interface, lambda, template, container, RAII, or
   smart pointer.
5. Name the common bug.
6. Explain when not to use it.

Example answer for Strategy:

> Strategy is for interchangeable algorithms. In C++ I first try a lambda,
> functor, or template parameter. If the algorithm must be chosen and stored at
> runtime, I may use an interface such as `Checksum` with implementations like
> `SumChecksum` or `CrcChecksum`. I would not create a Strategy hierarchy for
> one algorithm or for a simple local branch.

Example answer for Observer:

> A callback is usually one callable. Observer is useful when one subject must
> notify multiple independent listeners. In C++ this might be an observer
> interface or a list of `std::function` callbacks. The important production
> issue is lifetime: observers need unsubscribe rules, and stored lambdas must
> not capture dead references.

Example answer for composition over inheritance:

> Inheritance means substitutability: a `Derived` must be usable wherever a
> `Base` is expected. Composition means a type owns or uses another object to do
> part of its work. I prefer composition for behavior reuse because it has lower
> coupling and clearer ownership. I use inheritance when I need runtime
> polymorphism through a stable interface.

Common senior-level traps:

- claiming every pattern improves code;
- ignoring ownership in Factory Method or Prototype;
- forgetting virtual destructors;
- confusing Strategy and State;
- confusing Adapter and Facade;
- using Observer without unsubscribe policy;
- using Abstract Factory when Factory Method or a constructor is enough;
- choosing runtime polymorphism when a template policy is simpler;
- choosing templates when runtime substitution is required.

## 13. Practice

1. Implement a small connection FSM with `enum class State` and a transition
   function. Add tests for invalid events.
2. Rewrite the FSM using State objects. Then decide which version is clearer.
3. Implement a checksum Strategy with a lambda version and an interface version.
4. Implement an Observer that returns an unsubscribe token.
5. Implement a Factory Method that returns `std::unique_ptr<Parser>`.
6. Implement an Adapter around a legacy function that uses `const char*`.
7. Implement a Logger Facade over console and file-like sinks.
8. Implement a Command queue with `std::function<void()>`, then identify capture
   lifetime risks.
9. Compare Decorator and Proxy with a shared `IDataSource` interface.
10. Refactor a fat interface into smaller role interfaces.
11. Find one inheritance relationship in old code and decide whether composition
    would be clearer.
12. For each MUST pattern, write one sentence for "when not to use it."

## 14. Summary

Design principles help you decide whether code is easy to change. Design
patterns give names to recurring solutions, but they are useful only when they
solve a real problem.

For C++, the practical path is:

1. protect invariants with cohesive classes and RAII;
2. keep dependencies small and explicit;
3. prefer composition for reuse;
4. use runtime polymorphism only when substitutability is real;
5. use templates/lambdas when compile-time customization is enough;
6. apply high-priority patterns carefully;
7. keep lower-priority patterns as vocabulary until the problem calls for them.

The best design is not the one with the most pattern names. It is the one where
the next correct change is obvious.
