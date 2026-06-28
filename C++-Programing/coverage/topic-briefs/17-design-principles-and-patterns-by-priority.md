# Topic Brief 17 - Design Principles And Design Patterns By Priority

## 1. Canonical Routing

| Field | Validated value |
| --- | --- |
| `learning_path_number` | `17` |
| Title | Design Principles And Design Patterns By Priority |
| `slug` | `design-principles-and-patterns-by-priority` |
| Requested topic | Practical C++ design principles and design patterns organized by engineering priority, with simple design first and patterns used only when they reduce complexity |
| Master source | `master-ch16` |
| Required Notion sources | `notion-5-1` to `notion-5-5`, `notion-6-1` to `notion-6-6`, `notion-7-1` to `notion-7-4`, `notion-10-5` |
| Topic Brief | `coverage/topic-briefs/17-design-principles-and-patterns-by-priority.md` |
| Knowledge target | `knowledge/17-design-principles-and-patterns-by-priority.md` |
| Interview target | `interview/17-design-principles-and-patterns-by-priority.md` |
| Example target | `examples/17-design-principles-and-patterns-by-priority/README.md` |

Validation result: the number, title, slug, master source, mapped Notion source
list, and output paths match `LEARNING_PATH.md`.

This step creates the Topic Brief only. It does not create or modify knowledge,
interview, or example outputs.

## 2. Source Coverage

### Internal Sources Read

| Source label | Path | Coverage contribution |
| --- | --- | --- |
| `master-ch16` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, CH16 | Mixed priority, CH09 prerequisite, MUST design principles, pattern priority tiers, required comparisons, AI expansion rule, and interview focus |
| `guide-section-03` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 3 | MUST/SHOULD depth rules and required output coverage dimensions |
| `guide-section-06` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 6 | Comparison table rules when C vs C++ or interface/polymorphism comparisons appear |
| `guide-section-07` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 7 | Depth control and practical example preference |
| `guide-section-08` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 8 | Design pattern expansion rules and priority-specific pattern depth |
| `guide-section-14` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 14 | Quality checklist: priority, dependencies, C/C++ usage, comparisons, common bugs, interviews, no over-engineering |
| `notion-index` | `docs/C++ Notion/C++ Programming.md` | Notion chapter inventory and source identity validation |
| `notion-5-1` | `docs/C++ Notion/Chapter 5-1 Classes, Objects & Constructors.md` | OOP pillars, class vs object, access control, encapsulation, constructors/destructors, `this`, RAII, method chaining, class vs struct interview angles |
| `notion-5-2` | `docs/C++ Notion/Chapter 5-2 Static Members & Friend Functions.md` | Static members/functions, factory-style static functions, singleton caution, friends, friendship rules, `mutable`, encapsulation/coupling tradeoffs |
| `notion-5-3` | `docs/C++ Notion/Chapter 5-3 Abstraction & Abstract Classes.md` | Abstraction, pure virtual functions, abstract classes, C++ interfaces, ISP, virtual destructors, interface contracts |
| `notion-5-4` | `docs/C++ Notion/Chapter 5-4 Inheritance & Polymorphism.md` | Inheritance types, inheritance modes, diamond problem, virtual inheritance, runtime polymorphism, vtable/vptr, `override`, `final`, object slicing |
| `notion-5-5` | `docs/C++ Notion/Chapter 5-5 Operator Overloading.md` | Operator overloading, member vs friend operators, functor `operator()`, const-correct operators, overload pitfalls and readability rules |
| `notion-6-1` | `docs/C++ Notion/Chapter 6-1 STL Introduction & vector Container.md` | STL motivation, generic containers/algorithms, `std::vector`, size/capacity, `reserve`, iterator invalidation, default-container guidance |
| `notion-6-2` | `docs/C++ Notion/Chapter 6-2 Sequence Containers deque, list, forward_list, array.md` | Sequence-container selection, `deque`, `list`, `forward_list`, `std::array`, cache locality, iterator stability, LRU-style design examples |
| `notion-6-3` | `docs/C++ Notion/Chapter 6-3 Container Adapters & Associative Containers.md` | Container Adapter concept, `stack`, `queue`, `priority_queue`, `set`, `map`, multimap/multiset, intent-revealing restricted interfaces |
| `notion-6-4` | `docs/C++ Notion/Chapter 6-4 Unordered Associative Containers.md` | Hash-table containers, ordered vs unordered tradeoffs, custom hash functions, load factor, average vs worst-case complexity |
| `notion-6-5` | `docs/C++ Notion/Chapter 6-5 Iterators - The Bridge Between Containers and Algorithms.md` | Iterator abstraction, iterator categories, adapters, invalidation, algorithm requirements, container/algorithm design contracts |
| `notion-6-6` | `docs/C++ Notion/Chapter 6-6 STL Algorithms & Functors.md` | STL algorithms, predicates, comparators, functors, lambdas, algorithmic reuse, strategy-like callable customization |
| `notion-7-1` | `docs/C++ Notion/Chapter 7-1 Templates - Function Templates & Class Template.md` | Generic programming, templates vs macros, class/function templates, specialization, zero-overhead reuse |
| `notion-7-2` | `docs/C++ Notion/Chapter 7-2 Templates - Variadic Templates & SFINAE.md` | Variadic templates, fold expressions, perfect-forwarding factory ideas, SFINAE constraints, modern alternatives |
| `notion-7-3` | `docs/C++ Notion/Chapter 7-3 Templates - Type Traits, Concepts & Metaprogramming.md` | Type traits, C++20 concepts, compile-time constraints, static polymorphism, constexpr vs TMP, generic design contracts |
| `notion-7-4` | `docs/C++ Notion/Chapter 7-4 Templates - Template Template Parameters & Advanced Topics.md` | Template template parameters, alias templates, deduction guides, dependent names, template friends, advanced generic wrappers |
| `notion-10-5` | `docs/C++ Notion/Chapter 10-5 Callbacks.md` | Function pointers, functors, lambdas, `std::function`, `std::bind`, callback storage, member callbacks, callback patterns, Observer pattern and callback lifetime traps |

All sixteen mapped Notion chapter files were inspected. No mapped Notion source
was skipped.

### External References Consulted

Accessed on 2026-06-27.

| Label | Reference | Validation purpose |
| --- | --- | --- |
| `external-refactoring-guru-catalog` | Refactoring Guru Design Patterns Catalog: <https://refactoring.guru/design-patterns/catalog> | Pattern taxonomy and short intent validation for creational, structural, and behavioral pattern grouping |
| `external-refactoring-guru-state` | Refactoring Guru State: <https://refactoring.guru/design-patterns/state> | State pattern intent for object behavior changing with internal state |
| `external-refactoring-guru-strategy` | Refactoring Guru Strategy: <https://refactoring.guru/design-patterns/strategy> | Strategy pattern intent for interchangeable algorithms |
| `external-refactoring-guru-observer` | Refactoring Guru Observer: <https://refactoring.guru/design-patterns/observer> | Observer pattern intent for subscription and notification |
| `external-refactoring-guru-factory-method` | Refactoring Guru Factory Method: <https://refactoring.guru/design-patterns/factory-method> | Factory Method intent for deferring product creation to subclasses/overrides |
| `external-refactoring-guru-adapter` | Refactoring Guru Adapter: <https://refactoring.guru/design-patterns/adapter> | Adapter intent for making incompatible interfaces work together |
| `external-refactoring-guru-facade` | Refactoring Guru Facade: <https://refactoring.guru/design-patterns/facade> | Facade intent for simplified subsystem access |
| `external-refactoring-guru-command` | Refactoring Guru Command: <https://refactoring.guru/design-patterns/command> | Command intent for encapsulating a request/action as an object |

No external standard-behavior references were required for this coverage step.
The external references above are pattern-reference support only, as requested.

### Source Coverage Status

`TOPIC_BRIEF_COMPLETE_WITH_PATTERN_REFERENCE`: canonical routing, master
priority tiers, guide depth/style rules, all mapped Notion files, Refactoring
Guru pattern reference, merged concepts, required comparisons, common bugs,
debugging notes, best practices, interview angles, gaps, and output targets are
recorded.

## 3. Priority And Dependencies

- Overall priority: `Mixed`.
- Required depth:
  - Deep for MUST design principles.
  - Deep for MUST patterns.
  - Medium for SHOULD patterns.
  - Short awareness for NICE patterns.
- Master prerequisite:
  - CH09, OOP In C++, for classes, encapsulation, abstraction, inheritance,
    polymorphism, virtual functions, abstract classes, and interfaces.
- Practical prerequisites:
  - constructors/destructors and RAII;
  - references/pointers and object lifetime;
  - virtual functions and virtual destructors;
  - callback mechanisms: function pointer, functor, lambda, `std::function`;
  - STL containers, iterators, algorithms, and iterator invalidation;
  - template basics, concepts/type traits, and static polymorphism;
  - error-handling and exception-safety awareness for command/factory/wrapper
    designs.

## 4. Scope And Depth Boundaries

### Deep In This Topic

- Design principles:
  - SOLID as a vocabulary set.
  - SRP, OCP, LSP, ISP, DIP.
  - DRY, KISS, YAGNI.
  - high cohesion and low coupling.
  - composition over inheritance.
- MUST patterns:
  - State / FSM.
  - Strategy.
  - Observer.
  - Factory Method.
  - Adapter.
  - Facade.
  - Command.
- Required pattern treatment:
  - problem first;
  - simple solution first;
  - pattern solution only when useful;
  - C implementation idea when relevant;
  - C++ implementation idea;
  - embedded/enterprise use case;
  - when not to use;
  - over-engineering mistake;
  - interview questions.

### Medium In This Topic

- SHOULD patterns:
  - Builder.
  - Decorator.
  - Proxy.
  - Template Method.
  - Chain of Responsibility.
  - Mediator.
  - Iterator.
  - Composite.
  - Prototype.
- Supporting C++ mechanisms:
  - abstract interfaces and virtual destructors;
  - static member factory functions;
  - functors, lambdas, and `std::function`;
  - STL container adapters as examples of interface restriction;
  - generic programming and concepts as alternatives to runtime polymorphism.

### Short Awareness In This Topic

- NICE patterns:
  - Visitor.
  - Memento.
  - Flyweight.
  - Bridge.
  - Abstract Factory.
- Template-heavy static design patterns and policy-based design only as
  controlled awareness. Full template machinery belongs mainly to topic 12.

### Defer Or Exclude

- Full OOP mechanics belong to topic 09.
- Full STL/container details belong to topic 11.
- Full template mechanics belong to topic 12.
- Full callback mechanics belong to topic 12 / topic 15 depending on comparison
  angle.
- Full C vs C++ comparison belongs to topic 15.
- Full POSIX/Linux API wrapping belongs to topic 16.
- Linux Device Driver, kernel-driver, Yocto, GStreamer, AUTOSAR, and unrelated
  platform material are excluded.

## 5. Merged Concept Map

- Design principles are decision rules. They help decide whether a class,
  function, interface, dependency, or module boundary is clear enough.
- Design patterns are recurring solutions. They are not goals. Use them only
  when the problem has enough variability, lifetime, coupling, or subsystem
  complexity to justify the structure.
- C++ design should start with the simplest useful mechanism:
  - free function or small class before inheritance;
  - lambda/functor before a full Strategy hierarchy;
  - enum + switch for a small stable FSM before State objects;
  - direct constructor before Factory Method;
  - direct API call before Adapter/Facade.
- Abstraction is about hiding "how" behind a stable "what". In C++ this is
  commonly expressed with classes, pure virtual interfaces, templates,
  callbacks, and STL-style iterator/range contracts.
- Encapsulation and RAII make design principles concrete: private data protects
  invariants, constructors establish valid state, destructors release resources,
  and move/copy policy states ownership.
- Inheritance should model substitutable "is-a" relationships. Composition is
  preferred when one object merely uses another object to do work.
- Runtime polymorphism uses virtual dispatch, interfaces, and base references or
  pointers. It enables plugin-like behavior but requires virtual destructors and
  slicing avoidance.
- Static polymorphism uses templates, overloads, concepts, and type traits. It
  can avoid runtime overhead but may increase compile-time complexity and error
  verbosity.
- STL itself demonstrates design principles:
  - containers own data;
  - iterators decouple algorithms from container internals;
  - algorithms accept callable policies;
  - container adapters restrict interfaces to prevent misuse.
- Callback mechanisms are the bridge between simple callbacks, Observer,
  Strategy, Command, and event-style APIs.

## 6. Required Comparisons To Preserve

Every downstream lesson/interview answer should preserve these comparisons.

### Callback Vs Observer

| Comparison | Callback | Observer | Guidance |
| --- | --- | --- | --- |
| Shape | One callable passed to be invoked later | Subject maintains one or more subscribers | Use callback for one hook; use Observer when multiple independent listeners need notifications |
| State | Function pointer has no state; lambda/functor/`std::function` may hold state | Observers often have identity, lifetime, and unsubscribe behavior | Observer needs stronger lifetime management |
| C idea | Function pointer plus `void* user_data` | List of callback slots plus contexts | Keep callback context lifetime explicit |
| C++ idea | Lambda, functor, template callback, or `std::function` | Interface observers, `std::function` list, signal/slot-like wrapper | Prefer simple callback until multiple subscribers or subscription management is real |
| Common bug | Dangling captured reference or invalid context pointer | Not removing destroyed observer; notifying while list mutates | Define ownership, unsubscribe, and reentrancy policy |

### FSM In C Vs State Pattern In C++

| Comparison | C FSM | C++ State Pattern | Guidance |
| --- | --- | --- | --- |
| Shape | `enum` state plus `switch` or transition table | State objects implement behavior through a common interface | Use C FSM for small stable states; use State when state-specific behavior grows and transitions become hard to maintain |
| Extensibility | Add state by editing central switch/table | Add state class and transition logic | State improves OCP when new states are frequent |
| Embedded fit | Excellent for predictable memory and explicit transitions | Useful when object behavior varies, but dynamic allocation should be controlled | In embedded examples, prefer static state objects or value-owned states when possible |
| Common bug | Missing transition/default; duplicated transition logic | Too many tiny classes for a tiny FSM; hidden transitions | Keep transition table/tests visible |

### Strategy Vs State

| Comparison | Strategy | State |
| --- | --- | --- |
| Intent | Swap an algorithm/policy | Change object behavior as internal state changes |
| Who chooses | Client/configuration usually chooses strategy | Context/state transitions choose state |
| Lifetime | Often stable during operation, can be injected | Changes as object progresses |
| C++ form | Interface, template policy, function object, lambda | Interface/state objects or enum+table for simpler FSMs |
| Trap | Overusing virtual classes where a lambda/template is enough | Using State for a tiny switch with no real behavioral complexity |

### Adapter Vs Facade

| Comparison | Adapter | Facade |
| --- | --- | --- |
| Intent | Convert one interface into another expected interface | Provide a simpler interface to a complex subsystem |
| Scope | Usually one incompatible class/API boundary | Several classes/functions/subsystems behind one entry point |
| C++ use | Wrap C API, legacy class, vendor SDK, or mismatched interface | Hide setup/order/error details of a subsystem |
| Trap | Adapter that changes semantics silently | Facade that becomes a huge god object |

### Factory Method Vs Abstract Factory

| Comparison | Factory Method | Abstract Factory |
| --- | --- | --- |
| Intent | Let subclass/override decide one product creation path | Create families of related products |
| Priority here | MUST pattern | NICE awareness pattern |
| C++ use | Virtual `create()` or overridable construction hook; static factory function can be simpler when polymorphic creation is not needed | Interface that creates multiple related interface types |
| Trap | Using full factory hierarchy for one concrete type | Using Abstract Factory when products are not a family |

### Decorator Vs Proxy

| Comparison | Decorator | Proxy |
| --- | --- | --- |
| Intent | Add behavior around an object while preserving interface | Control access to an object while preserving interface |
| Examples | Logging, metrics, compression, validation wrapper | Lazy loading, access control, remote object, caching guard |
| C++ use | Composition over inheritance; wrapper owns or references wrapped interface | Wrapper may delay, check, cache, or forward calls |
| Trap | Deep wrapper stacks that hide behavior | Proxy that surprises users by changing cost, lifetime, or failure mode |

### Inheritance Vs Composition

| Comparison | Inheritance | Composition |
| --- | --- | --- |
| Relationship | "is-a" and substitutable through a base interface | "has-a" or "uses-a" collaborator |
| C++ mechanism | Public inheritance, virtual functions, base references/pointers | Data members, constructor injection, templates, callable policies |
| Strength | Runtime polymorphism and common interface | Lower coupling, easier testing, clearer ownership |
| Risk | Fragile base class, slicing, virtual destructor mistakes, diamond complexity | Too much forwarding or unclear ownership if poorly named |
| Guidance | Use for true substitutability | Prefer by default for behavior reuse |

## 7. Pattern Priority Map

### MUST Patterns

| Pattern | Core problem | Simple solution first | C++ pattern form | C/embedded idea |
| --- | --- | --- | --- | --- |
| State / FSM | Object behavior depends on current state and transitions | `enum class` + switch/table for small stable state machines | `State` interface with concrete states or table-driven class | `enum` + transition table + function pointers |
| Strategy | Need interchangeable algorithm/policy | Function parameter, lambda, or template policy | Strategy interface, functor, lambda, or `std::function` | Function pointer + context |
| Observer | Multiple consumers need event notification | One callback if only one listener exists | Observer interface or callback list with subscription policy | Array/list of callbacks + contexts |
| Factory Method | Creation varies by subtype/configuration | Constructor or simple static factory when only one creation path exists | Virtual/overridable `create` method returning owning pointer/value | Function pointer factory or switch over product type |
| Adapter | Existing interface does not match required interface | Inline conversion helper if mismatch is tiny | Wrapper class translating calls/types/errors | Thin C wrapper around legacy/vendor API |
| Facade | Subsystem usage requires too many ordered calls | Helper function if only one call site | Small class exposing stable high-level operations | C module exposing simplified API |
| Command | Need to represent an action as data | Direct function/lambda for immediate execution | Command interface, lambda, functor, queueable command object | Function pointer + context + command ID/arguments |

### SHOULD Patterns

| Pattern | Depth target | Main source concepts to reuse |
| --- | --- | --- |
| Builder | Medium | Method chaining from `this`, named construction steps, validation before build |
| Decorator | Medium | Composition, common interface, RAII ownership, wrapper layering |
| Proxy | Medium | Same interface with access/lifetime/caching/remote-control policy |
| Template Method | Medium | Base class algorithm skeleton with virtual customization hooks |
| Chain of Responsibility | Medium | Callback/handler chain, early stop, ordering policy |
| Mediator | Medium | Reduce many-to-many object coupling through a coordinator |
| Iterator | Medium | STL iterator abstraction, categories, invalidation, algorithm contracts |
| Composite | Medium | Tree of objects with common interface; ownership/lifetime clarity |
| Prototype | Medium | Copy/clone behavior, deep copy, virtual `clone`, slicing avoidance |

### NICE Patterns

| Pattern | Awareness target |
| --- | --- |
| Visitor | Double dispatch for adding operations to stable type hierarchies; can be heavy |
| Memento | Snapshot/restore state while preserving encapsulation |
| Flyweight | Share repeated immutable state to reduce memory |
| Bridge | Separate abstraction from implementation hierarchy |
| Abstract Factory | Families of related products; do not confuse with simple Factory Method |

## 8. Common Bugs And Design Smells

- Starting with a pattern before the problem requires it.
- Treating every pattern as equally important.
- Using inheritance for code reuse where composition would be simpler.
- Violating LSP by making derived classes reject valid base-class operations.
- Creating large "fat" interfaces that violate ISP.
- Depending directly on concrete classes where DIP would benefit testing or
  replacement.
- Overusing `friend`, global state, singletons, or static mutable data.
- Forgetting virtual destructors in polymorphic bases.
- Passing polymorphic types by value and causing object slicing.
- Returning raw owning pointers from factories without clear ownership.
- Storing callbacks that capture references to destroyed objects.
- Using `std::function` everywhere in performance-critical paths when a template
  callback or lambda parameter would be simpler and faster.
- Hiding too much behind a Facade until it becomes a god object.
- Building Adapter wrappers that silently change semantics or lose error detail.
- Deep Decorator/Proxy stacks that make debugging call flow difficult.
- Iterator invalidation in designs that store iterators across container
  mutation.
- Template-heavy designs with poor diagnostics where runtime polymorphism or a
  simple function would be clearer.

## 9. Debugging And Review Notes

- Compile examples with:
  - `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`;
  - sanitizers for memory/UB when examples allocate or use polymorphic ownership;
  - TSan only when observer/callback examples become concurrent.
- For design review, ask:
  - What invariant does this class protect?
  - Is this an "is-a" relationship or a "has-a/uses-a" relationship?
  - Does the base class need a virtual destructor?
  - Can this be a value, lambda, template policy, or free function instead of a
    hierarchy?
  - Who owns objects created by Factory Method or Prototype?
  - How does Observer unsubscribe or survive subject destruction?
  - Are callbacks stored beyond the lifetime of their captures?
  - Are iterator/container assumptions documented?
  - Does Adapter preserve error behavior and ownership semantics?
  - Does Facade hide complexity or merely hide important control?
- For runtime debugging:
  - use `gdb` backtraces to inspect virtual call paths and command/decorator
    stacks;
  - log state transitions in FSM/State examples;
  - assert invalid transitions in learning examples;
  - unit-test each strategy/state/command independently;
  - use sanitizer builds for ownership, lifetime, and slicing-adjacent mistakes.

## 10. Best Practices To Preserve

- Principles before patterns: explain the maintainability problem first.
- Prefer KISS and YAGNI until variability is real.
- Keep classes cohesive: one clear reason to change.
- Keep interfaces small and role-specific.
- Prefer composition over inheritance for behavior reuse.
- Use public inheritance only for substitutable "is-a" relationships.
- Use abstract interfaces for stable runtime polymorphic boundaries.
- Add virtual destructors to polymorphic base classes.
- Prefer RAII and smart pointers for ownership in pattern implementations.
- Prefer lambdas/functors/templates for simple Strategy/Command cases.
- Use `std::function` when callbacks must be type-erased and stored.
- For embedded-style FSMs, prefer explicit transition tables or static state
  objects unless dynamic behavior truly needs heap-backed objects.
- Use `override` on every override and `final` only when the design really
  forbids extension.
- Keep Adapter and Facade wrappers thin, named, and testable.
- Document ownership, lifetime, threading, and error behavior at pattern
  boundaries.

## 11. Interview Angles

- Explain SOLID with one C++ example each, especially SRP, ISP, DIP, and LSP.
- Explain why "composition over inheritance" is a default preference, not an
  absolute ban on inheritance.
- Compare callback vs Observer and identify callback lifetime traps.
- Design an embedded FSM first as an enum/table, then explain when State pattern
  becomes useful.
- Compare Strategy vs State using who chooses the behavior.
- Explain Factory Method use cases and why a plain constructor or static factory
  may be enough.
- Use Adapter to wrap a legacy C or vendor API behind a C++ interface.
- Use Facade to hide subsystem setup while avoiding god-object design.
- Model Command for undo/redo, deferred execution, or command queue.
- Explain why virtual destructors and passing polymorphic objects by reference
  matter.
- Explain when templates/static polymorphism are better than virtual interfaces.
- Explain Decorator vs Proxy and what each wrapper is allowed to change.

## 12. Practice And Example Targets

- Implement a small sensor/controller FSM twice:
  - `enum class State` + transition table;
  - State pattern with static state objects.
- Implement Strategy for checksum selection:
  - lambda/functor version;
  - runtime interface version.
- Implement Observer with subscribe/unsubscribe and safe lifetime comments.
- Implement a Factory Method for parser creation that returns
  `std::unique_ptr<IParser>`.
- Implement an Adapter that wraps a C-style callback API into a C++ class.
- Implement a Facade for a logger subsystem with file sink, console sink, and
  severity filtering.
- Implement Command for undoable configuration changes.
- Demonstrate Decorator vs Proxy with a common `IDataSource` interface.
- Demonstrate Builder only if construction has multiple optional steps and
  validation.
- Include compile commands and sanitizers in later examples when ownership,
  callbacks, or polymorphism are involved.

## 13. Gaps, Constraints, And External Validation Needs

- No mapped Notion files were skipped.
- Refactoring Guru was used only as a pattern-reference source. It should not
  override the repository's C++ priority/depth rules.
- The Notion sources provide many C++ mechanisms but not a full pattern catalog;
  therefore the Topic Brief maps patterns to existing C++ mechanisms and master
  priorities.
- Later learner-facing docs should avoid long source coverage tables and should
  not include audit metadata.
- Later examples should avoid platform-specific material and stay in portable
  C++ unless a C callback/FSM comparison is explicitly useful.
- No Linux Device Driver, kernel-driver, Yocto, GStreamer, AUTOSAR, or unrelated
  platform material should be introduced.

## 14. Output Targets

- Topic Brief: `coverage/topic-briefs/17-design-principles-and-patterns-by-priority.md`.
- Knowledge: `knowledge/17-design-principles-and-patterns-by-priority.md`.
- Interview: `interview/17-design-principles-and-patterns-by-priority.md`.
- Examples: `examples/17-design-principles-and-patterns-by-priority/README.md`.

Recommended next step: create the learner-facing knowledge lesson from this
Topic Brief, keeping audit/source metadata out of the knowledge file.
