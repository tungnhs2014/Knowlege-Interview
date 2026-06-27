# Topic Brief 09 - OOP In C++

## 1. Canonical Routing

| Field | Validated value |
| --- | --- |
| `learning_path_number` | `09` |
| Title | OOP In C++ |
| `slug` | `oop-in-cpp` |
| Requested topic | Object-oriented design and polymorphism in C++ |
| Master source | `master-ch09` |
| Required Notion sources | `notion-5-1`, `notion-5-2`, `notion-5-3`, `notion-5-4`, `notion-5-5` |
| Topic Brief | `coverage/topic-briefs/09-oop-in-cpp.md` |
| Knowledge target | `knowledge/09-oop-in-cpp.md` |
| Interview target | `interview/09-oop-in-cpp.md` |
| Example target | `examples/09-oop-in-cpp/README.md` |

Validation result: the number, title, slug, master source, five mapped Notion
sources, and all canonical output paths match `LEARNING_PATH.md`.

This step creates the Topic Brief only. It does not create or modify knowledge,
interview, or example outputs.

## 2. Source Coverage

### Internal Sources Read

| Source label | Path | Coverage contribution |
| --- | --- | --- |
| `master-ch09` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, CH09 | MUST priority, deep depth, CH08 prerequisite, keyword scope, required comparisons, anti-over-engineering rule, and interview focus |
| `guide-section-03` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 3 | Deep treatment for MUST concepts, medium treatment for SHOULD concepts, and brief awareness for NICE concepts |
| `guide-section-04` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 4 | Full-chapter structure and required C, C++, embedded, enterprise, bug, debug, interview, practice, and reference angles |
| `guide-section-06` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 6 | Required comparison structure |
| `guide-section-07` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 7 | Practical examples and depth control |
| `guide-section-08` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 8 | Problem-first design guidance and avoidance of pattern overuse |
| `guide-section-09` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 9 | Trusted-source routing for exact C++ behavior and design guidance |
| `guide-section-10` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 10 | Clear technical English, concise Markdown, compile-oriented examples, and risk warnings |
| `guide-section-11` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 11 | Topic Brief, full lesson, interview pack, examples, and review expectations |
| `guide-section-14` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 14 | Final quality checklist |
| `notion-index` | `docs/C++ Notion/C++ Programming.md` | Notion inventory and chapter identity validation |
| `notion-5-1` | `docs/C++ Notion/Chapter 5-1 Classes, Objects & Constructors.md` | OOP vocabulary, classes and objects, access control, encapsulation, constructors, destructors, `this`, copy behavior, and introductory RAII |
| `notion-5-2` | `docs/C++ Notion/Chapter 5-2 Static Members & Friend Functions.md` | Static members, static member functions, factories, singleton example, friendship, member versus non-member design, and `mutable` |
| `notion-5-3` | `docs/C++ Notion/Chapter 5-3 Abstraction & Abstract Classes.md` | Abstraction, pure virtual functions, abstract bases, interface-style classes, interface segregation, and polymorphic destruction |
| `notion-5-4` | `docs/C++ Notion/Chapter 5-4 Inheritance & Polymorphism.md` | Inheritance forms and access, multiple and virtual inheritance, diamond problem, compile-time/runtime polymorphism, virtual dispatch model, `override`, `final`, and object slicing |
| `notion-5-5` | `docs/C++ Notion/Chapter 5-5 Operator Overloading.md` | Operator syntax and restrictions, member/non-member choices, comparisons, stream operators, call/subscript operators, assignment, and semantic pitfalls |

All five mapped Notion chapter files were inspected. No mapped Notion source
was skipped.

### External References Consulted

Accessed on 2026-06-14.

| Label | Reference | Validation purpose |
| --- | --- | --- |
| `external-core-guidelines` | C++ Core Guidelines: <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines> | Class invariants, concrete types versus hierarchies, interface bases, virtual destructors, `override`/`final`, implementation versus interface inheritance, slicing, protected data, multiple inheritance, and composition-first design |
| `external-iso-cpp-delete` | C++ working draft `[expr.delete]`: <https://eel.is/c++draft/expr.delete> | Exact behavior of deleting a derived object through a base pointer and the role of a virtual destructor |
| `external-iso-cpp-abstract` | C++ working draft `[class.abstract]`: <https://eel.is/c++draft/class.abstract> | Pure virtual functions, abstract classes, final overriders, and the fact that a pure virtual function may have an out-of-class definition |
| `external-iso-cpp-virtual` | C++ working draft class hierarchy clauses: <https://eel.is/c++draft/> | Portable virtual-function, inheritance, construction, destruction, and copy-elision terminology without treating a particular ABI layout as the language rule |

### Source Coverage Status

`TOPIC_BRIEF_COMPLETE_WITH_MAPPED_SOURCE_GAPS`: canonical routing, every mapped
Notion source, master requirements, guide requirements, major source
corrections, design validation, comparison requirements, gaps, and downstream
quality gates are recorded.

The mapped sources cover class mechanics and inheritance in substantial detail.
They do not fully cover several master requirements: OOP techniques in C,
composition/aggregation/association/dependency as separate relationships,
function-pointer-table comparison, dependency inversion, mixins, and CRTP.
Those gaps are explicitly routed below.

## 3. Priority And Dependencies

- Overall priority: `MUST`.
- Required depth: Deep.
- Master prerequisite: CH08, C++ Fundamentals.
- Required prior model:
  - class and object fundamentals;
  - constructors, destructors, initialization order, and object lifetime;
  - references, pointers, `const`, overload resolution, and operator basics;
  - manual C callbacks and function pointers;
  - ownership versus borrowing;
  - translation units, headers, linkage, and ABI awareness.
- Follow-on chapters:
  - CH10, Resource Management In C++;
  - CH12/CH13, Modern C++ and templates;
  - CH16, Design Principles and Patterns;
  - CH17, C versus C++ comparison.

Chapter 09 must use CH08 mechanics but should not duplicate the full
constructors, operators, linkage, or basic class lesson. Its center is design:
how to express stable behavior, preserve invariants, select relationships, and
use static or dynamic polymorphism safely.

## 4. Scope And Depth Boundaries

### Deep In This Topic

- Encapsulation as control of invariants, not automatic getter/setter
  generation.
- Abstraction as a stable contract that hides changeable implementation.
- Public inheritance as substitutability and an `is-a` relationship.
- Runtime polymorphism through virtual functions and base
  pointers/references.
- Pure virtual functions, abstract classes, and interface-style base classes.
- Polymorphic destruction and virtual destructor policy.
- `override` and `final`.
- Object slicing and prevention.
- Composition, aggregation, association, and dependency.
- Inheritance versus composition, including reasons to prefer composition.
- Overloading versus overriding.
- Abstract class versus interface-style class.
- Virtual dispatch versus a C function-pointer table.
- OOP-style design in C versus language-supported OOP in C++.

### Medium In This Topic

- Compile-time polymorphism through overloads, operator overloads, and a
  controlled introduction to templates.
- Multiple inheritance for distinct interfaces.
- Virtual inheritance and the diamond problem.
- Interface Segregation Principle.
- Dependency Inversion Principle.
- Static members, friendship, and `mutable` only where they affect class
  design.
- Operator overloading as type-interface design rather than syntax inventory.
- Construction and destruction order across base and derived objects.
- Name hiding and overload-set recovery with `using`.
- Ownership of polymorphic objects at an API-design level.

### Brief Awareness

- Mixin classes.
- CRTP as one static-polymorphism technique.
- Covariant return types.
- RTTI and `dynamic_cast` only as hierarchy-navigation tools, not the default
  dispatch mechanism.
- Devirtualization as an optimizer opportunity, not a correctness contract.
- ABI-specific virtual table inspection as a debugging exercise.

### Defer Or Exclude

- Deep Rule of Five, smart-pointer ownership, custom deleters, and exception
  guarantees: CH10.
- Full template mechanics, concepts, policy classes, and expression templates:
  CH12/CH13.
- Broad design-pattern catalogues: CH16.
- Concurrency-safe caches, singleton initialization, and shared mutable state:
  later concurrency/design treatment.
- Linux Device Driver and kernel-driver material.

## 5. Mapped Source Corrections

Downstream outputs must treat the Notion chapters as source notes requiring
technical review, not as text to reproduce unchanged.

### `notion-5-1`

- A class is not merely a blueprint analogy. It defines a type, member set,
  access rules, special-member behavior, and object invariants.
- `class` and `struct` differ in default member access and default base access.
  Both support constructors, methods, inheritance, templates, and virtual
  functions. "Use struct for POD" is outdated as a general rule; prefer
  `struct` for transparent data without a cross-member invariant and `class`
  for invariant-owning types.
- A C++ `struct` with methods, constructors, references, or non-C-compatible
  members is not automatically C-compatible.
- Encapsulation does not mean wrapping every field in trivial getters and
  setters. Public data can be clearer for a true aggregate; invariant-bearing
  state should be private and changed through semantic operations.
- Compiler-generated copying is memberwise. Calling it universally "shallow
  copy" is imprecise because value members copy according to their own copy
  semantics.
- Copy construction on function return is not guaranteed to execute as an
  observable copy; copy elision can remove it, and some elision is mandatory
  from C++17.
- Manual `new[]`/`delete[]` examples require the Rule of Three and exception
  safety. Prefer standard value members and containers in downstream examples.
- One source `String` destructor prints a character buffer after releasing it;
  that is a use-after-lifetime bug and must not be reused.
- RAII means tying resource lifetime to object lifetime. Acquisition need not
  literally occur in every constructor, and Chapter 10 owns the deep resource
  management treatment.

### `notion-5-2`

- A static data member is not guaranteed to live in a specific "data/BSS
  segment"; storage layout is implementation-specific.
- Since C++17, suitable static data members can be declared `inline static` in
  the class definition. Do not teach an out-of-class definition as universally
  mandatory.
- A friend is not a member and friendship is neither inherited, transitive, nor
  automatically reciprocal. Friendship is targeted access, not automatically
  an encapsulation violation; its coupling cost still requires justification.
- Non-member operators need not be friends when the public interface is
  sufficient. Hidden friends can support argument-dependent lookup while
  keeping the operation associated with the type.
- The raw-pointer singleton example is not production guidance. It has
  thread-safety, lifetime, shutdown-order, testability, and global-state
  problems. Do not teach Singleton as a default OOP technique.
- `mutable` does not make mutation thread-safe. Mutable caches and counters need
  synchronization when objects are shared across threads and must preserve
  logical constness.

### `notion-5-3`

- A pure virtual function is declared with a pure-specifier. It can have a
  separate definition; a pure virtual destructor must have a definition if it
  is odr-used by destruction.
- "Interface" is a design convention in C++, not a language keyword or formally
  separate type category. An interface-style base normally has no mutable data
  and exposes pure virtual behavior.
- Not every abstract class must have a public virtual destructor. A base meant
  for polymorphic deletion needs a virtual destructor; a base that forbids
  deletion through the interface can use a protected non-virtual destructor.
- Deleting a derived object through a base pointer whose base destructor is not
  virtual is undefined behavior under the relevant conditions, not merely a
  predictable call to the base destructor followed by a leak.
- Prefer `virtual ~Interface() = default` when public polymorphic destruction is
  intended.
- Protected data in interface or hierarchy bases weakens invariant control.
  Prefer private data plus protected/public operations, or no data in pure
  interface bases.
- Raw owning arrays of base pointers distract from interface design. Downstream
  code should use stack lifetimes, references, or ownership types introduced
  with an explicit CH10 forward reference.

### `notion-5-4`

- The C++ language specifies virtual behavior, not a mandatory `vtable`/`vptr`
  implementation. Vtables are the dominant implementation model and useful for
  interviews/debugging, but their existence, count, placement, and layout are
  ABI details.
- A virtual pointer is not guaranteed to be the first object member, exactly
  one pointer, or eight bytes. Multiple and virtual inheritance can require
  more complex layouts.
- Fixed nanosecond costs for direct and virtual calls are not portable facts.
  Cost depends on target, optimizer, predictability, cache behavior, call site,
  and whether devirtualization succeeds. Measure the actual workload.
- Virtual calls can sometimes be inlined after devirtualization. Do not say
  virtual functions cannot be inlined.
- Public inheritance should express substitutability, not merely code reuse.
  "`is-a`" is a useful first test but is not sufficient without behavioral
  compatibility.
- Private inheritance is not ordinary composition. It is a distinct
  implementation technique with different access and conversion behavior and
  should be rare.
- Multiple inheritance is most defensible for multiple independent interfaces.
  Virtual inheritance solves shared-base identity but adds initialization,
  layout, and maintenance complexity.
- `override` should be used on every overriding declaration. `final` should
  communicate a deliberate design constraint; possible optimization is
  secondary.
- Making a base abstract prevents direct base construction but does not by
  itself prevent all accidental slicing patterns. APIs must still use
  references/pointers and appropriate copy policy.

### `notion-5-5`

- Operator overloading is compile-time function selection, but it is not the
  whole model of compile-time polymorphism.
- Stream operators must be non-members because the stream is the left operand;
  they are not required to be friends when public operations suffice.
- Overloaded `&&` and `||` do not provide the built-in short-circuit semantics.
  The comma and address-of operators also have surprising behavior and should
  rarely be overloaded.
- Prefix increment is often preferable when the old value is unnecessary, but
  fixed operation-count and nanosecond claims are not portable. Optimization
  can remove copies for simple types.
- A hand-written assignment operator that deletes old storage before allocating
  replacement storage lacks the strong exception guarantee and can leave a
  dangling member if allocation throws. Prefer value members, copy-and-swap, or
  allocate-before-commit designs.
- Self-assignment checks are not universally required. Correct copy-and-swap or
  memberwise assignment can already be self-assignment safe.
- Arithmetic examples must define overflow, division-by-zero, invalid-state,
  and narrowing policies. Signed overflow is undefined behavior.
- An operator should preserve familiar semantics and complexity expectations.
  Use a named function when operator syntax would hide surprising work or side
  effects.

## 6. Merged Concept Map

### Object And Class Design

- Class as a type with a public contract and hidden representation.
- Object as a value with identity, state, lifetime, and invariants.
- Encapsulation protects invariants and limits change propagation.
- Abstraction defines what clients can rely on while hiding implementation.
- Constructors establish valid initial state; destructors terminate lifetime
  and release owned resources.
- `const` member functions express observable read-only operations.
- Static members represent class-level state or operations, but shared mutable
  state needs explicit lifetime and synchronization policy.
- Friendship is a narrow collaboration tool, not a substitute for interface
  design.

### Relationships

- Dependency: one operation temporarily uses another type.
- Association: objects know or interact with one another without implied
  ownership.
- Aggregation: a whole refers to parts whose lifetimes are independent.
- Composition: a whole owns parts whose lifetimes are bound to the whole.
- Inheritance: a derived type participates in a base-type contract.
- Public inheritance is appropriate only when clients can safely substitute a
  derived object wherever the base contract is expected.
- Composition over inheritance: prefer delegation and contained collaborators
  when subtype substitution is not the actual requirement.

Ownership must be stated separately from the UML-style relationship name.
Pointers and references do not inherently identify ownership.

### Polymorphism

- Ad hoc compile-time polymorphism: function and operator overload resolution.
- Parametric/static polymorphism: templates and CRTP, introduced briefly.
- Runtime/subtype polymorphism: virtual dispatch through a base
  pointer/reference.
- Dynamic dispatch chooses the final overrider from the dynamic type.
- Non-virtual calls are selected from the static type and ordinary lookup.
- C can model runtime polymorphism explicitly with a state pointer plus a table
  of function pointers.

### Hierarchy Design

- Abstract bases define contracts and may provide shared implementation.
- Interface-style bases should be small, cohesive, and ownership-neutral.
- `override` lets the compiler verify overriding intent.
- `final` closes an override point or hierarchy intentionally.
- Public virtual destructors support polymorphic ownership/deletion.
- Protected non-virtual destructors can forbid deletion through a non-owning
  interface.
- Object slicing loses the derived portion when copying into a base object by
  value.
- Multiple inheritance is controlled primarily to distinct interfaces.
- Virtual inheritance handles one shared virtual base in a diamond but is not a
  reason to create a diamond.

### Design Principles

- Prefer concrete value types when runtime substitution is unnecessary.
- Prefer composition when behavior can be delegated to a contained strategy or
  collaborator.
- Keep interfaces small and cohesive.
- Depend on abstractions at policy boundaries, with dependencies supplied from
  outside rather than created invisibly inside high-level logic.
- Avoid protected data and broad friendship.
- Avoid speculative virtual functions and hierarchy depth.
- Separate ownership from polymorphism.

## 7. C Usage

C has no language-level classes, inheritance, access control, constructors,
destructors, or virtual functions. It can still implement object-oriented
techniques explicitly:

- a `struct` stores object state;
- functions receive an explicit object pointer;
- opaque incomplete structs hide representation across an API boundary;
- init/deinit or create/destroy functions manage lifetime;
- a table of function pointers provides runtime dispatch;
- a `void *` or typed context pointer carries implementation state;
- composition is represented by embedded structs or pointers to collaborators.

Required C teaching model:

```c
struct device_ops {
    int (*start)(void *context);
    void (*stop)(void *context);
};

struct device {
    void *context;
    const struct device_ops *ops;
};
```

The lesson must explain the contract costs:

- no compiler-enforced private representation unless opaque handles are used;
- manual initialization and cleanup;
- explicit null checks and status propagation;
- function-pointer signature correctness;
- context lifetime and ownership;
- ABI/versioning of operation tables;
- no automatic virtual destructor equivalent.

This is user-space/language-level C design. It must not become a kernel-driver
lesson.

## 8. C++ Usage

Downstream C++ examples should center on one small, practical family such as a
sensor source, logger sink, protocol transport, command handler, or storage
backend.

Required mechanisms:

- concrete value type with a maintained invariant;
- small abstract interface;
- two implementations selected through a base reference;
- `override` on every overrider;
- virtual or protected destructor policy made explicit;
- composition to inject a collaborator;
- no raw owning `new`/`delete` in primary examples;
- one intentional slicing example shown but not executed as production code;
- one C function-table equivalent for comparison.

A suitable core shape:

```cpp
class Sensor {
public:
    virtual ~Sensor() = default;
    virtual int read_millivolts() const = 0;
};

class Alarm {
public:
    explicit Alarm(const Sensor& sensor) : sensor_{sensor} {}
    bool active() const;

private:
    const Sensor& sensor_; // borrowed; Sensor must outlive Alarm
};
```

The lesson must state that the reference is borrowed and that lifetime is not
encoded as ownership.

## 9. Embedded And Enterprise Usage

### Embedded Usage

- HAL-independent sensor, clock, storage, transport, and watchdog interfaces.
- Static allocation and externally owned implementations where dynamic
  allocation is prohibited.
- Function-table C interfaces for C ABI boundaries or highly constrained
  firmware.
- Virtual dispatch only where configurability and testability justify its
  target-specific cost.
- No virtual call, allocation, lock, or exception hidden in timing-critical
  paths without an explicit execution-time policy.
- Interface injection for host tests and hardware fakes.
- Fixed-capacity objects and deterministic destruction.
- Link-time or compile-time implementation selection when runtime substitution
  is unnecessary.

### Enterprise Usage

- Stable service interfaces and dependency injection.
- Plugin or backend boundaries with explicit ABI/version constraints.
- Test doubles through small interfaces.
- Concrete value types for ordinary domain data.
- Composition for policy/configuration variability.
- Ownership represented by values or smart pointers, with CH10 providing the
  deep treatment.
- Avoidance of fragile base classes, broad protected state, deep hierarchies,
  and global singleton dependencies.
- API review focused on substitutability, exception guarantees, thread-safety,
  and lifetime.

## 10. Required Comparisons

### OOP In C Versus OOP In C++

| Topic | C | C++ | Review question |
| --- | --- | --- | --- |
| State | `struct` | class/struct object | Who maintains the invariant? |
| Encapsulation | opaque handle and module boundary | access control plus module boundary | Can clients corrupt representation? |
| Method call | function plus explicit object/context | member or non-member function | Is coupling to representation necessary? |
| Runtime dispatch | function-pointer table | virtual function | Is ABI/layout or language portability required? |
| Lifetime | init/deinit or create/destroy | constructor/destructor and RAII | Who owns cleanup? |
| Subtyping | convention and compatible operation table | public inheritance and virtual contract | Is substitutability enforced and tested? |
| Failure | status/error output | status, expected-style result, or exception policy | Can failure cross the boundary safely? |

### Inheritance Versus Composition

| Question | Inheritance | Composition |
| --- | --- | --- |
| Relationship | `is-a`/substitutability | `has-a` or delegates-to |
| Coupling | Tight to base contract and protected surface | Usually narrower collaborator contract |
| Runtime variation | Virtual override | Replace contained/injected strategy |
| Reuse | Interface and possibly implementation | Implementation through delegation |
| Main risk | Fragile hierarchy and invalid substitution | Boilerplate forwarding and lifetime policy |
| Default | Use for genuine hierarchy | Prefer for flexible reuse |

### Overloading Versus Overriding

| Aspect | Overloading | Overriding |
| --- | --- | --- |
| Selection | Compile-time overload resolution | Runtime virtual dispatch when called polymorphically |
| Scope | Same name, different parameter lists/candidates | Derived declaration matching a base virtual |
| Keyword | No special keyword | Use `override` |
| Common bug | Ambiguous or surprising conversion | Signature mismatch/name hiding |
| Polymorphism category | Compile-time/ad hoc | Runtime/subtype |

### Abstract Class Versus Interface-Style Class

| Aspect | Abstract class | Interface-style class |
| --- | --- | --- |
| Language category | Class with a pure virtual final overrider | Design convention, usually an abstract class |
| Data | May contain state | Prefer no mutable instance state |
| Implementation | May provide shared behavior | Usually contract-only |
| Constructors | May initialize base state | Often no user-written constructor |
| Main use | Shared contract plus selected implementation | Complete separation of contract and implementation |
| Destruction | Explicit virtual/protected policy | Usually public virtual defaulted destructor |

### Virtual Dispatch Versus Function-Pointer Table

| Aspect | C++ virtual dispatch | C function-pointer table |
| --- | --- | --- |
| Type checking | Compiler checks overrides and calls | Compiler checks signatures, convention supplies hierarchy |
| Object context | Implicit `this` | Explicit context pointer |
| Dispatch syntax | `object.operation()` | `object.ops->operation(object.context)` |
| Layout | Language behavior with ABI-specific implementation | Programmer-defined ABI/layout |
| Extensibility | Derived types override operations | New operation-table instances provide behavior |
| Destruction | Virtual destructor policy | Explicit destroy/deinit operation |
| Boundary fit | Natural inside C++ | Useful for C ABI and C code |

## 11. Common Bugs And Risks

- Deleting through a base pointer without the required virtual destructor:
  undefined behavior.
- Passing or storing a polymorphic base by value: object slicing.
- Missing `override`, allowing a signature mismatch to create a different
  function.
- Calling virtual functions from constructors or destructors expecting
  most-derived dispatch.
- Exposing protected data and allowing derived classes to violate base
  invariants.
- Using inheritance only for code reuse when composition expresses the
  relationship better.
- Base preconditions strengthened or postconditions weakened by a derived
  implementation, breaking substitutability.
- Returning or storing a borrowed reference/pointer beyond the owner's
  lifetime.
- Raw owning pointers in polymorphic containers, causing leaks or double
  deletion.
- Non-owning base destructor made public, permitting accidental deletion.
- Multiple inheritance ambiguity and duplicated non-virtual base subobjects.
- Virtual-base initialization performed in the wrong class; the most-derived
  class initializes virtual bases.
- Name hiding in a derived class removing base overloads from ordinary lookup.
- Default arguments on virtual functions differing between base and derived;
  defaults are selected from the static type.
- Downcasts used to select behavior instead of adding a virtual operation or
  redesigning the interface.
- RTTI disabled or unavailable while code assumes `dynamic_cast`.
- Friend declarations exposing more representation than required.
- Shared static state or mutable caches causing data races.
- Operators violating expected semantics, losing short-circuit behavior, or
  performing unchecked signed arithmetic.
- Hand-written copy/assignment of raw resources violating Rule of Three/Five or
  exception safety.
- Treating vtable layout, object size, or call cost as portable.

## 12. Debugging And Verification Notes

### Compiler Diagnostics

Recommended host-side baseline:

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
    -Wnon-virtual-dtor -Woverloaded-virtual -Werror example.cpp
```

Compiler support and exact warning names vary. Downstream examples must validate
commands on the installed toolchain.

Useful diagnostics:

- non-virtual destructor in a polymorphic base;
- hidden virtual overloads;
- inconsistent override declarations;
- reorder warnings;
- deprecated implicit copy operations;
- shadowing and suspicious conversions.

### Sanitizers

```bash
c++ -std=c++17 -O1 -g3 \
    -fsanitize=address,undefined,vptr \
    -fno-omit-frame-pointer example.cpp
```

`-fsanitize=vptr` support and compatibility are compiler/platform specific.
Sanitizers can expose selected invalid downcasts, invalid virtual calls, use
after lifetime, and memory errors on executed paths. They do not prove hierarchy
correctness, substitutability, race freedom, ABI stability, or complete
coverage.

### Debugger

Useful GDB operations:

- break on base and derived constructors/destructors;
- inspect the dynamic type with `print *base_pointer`;
- compare calls through an object, base reference, and base pointer;
- inspect backtraces during virtual dispatch and destruction;
- watch a borrowed collaborator's lifetime;
- stop on sanitizer reports before undefined behavior propagates.

ABI-specific vtable symbols or object layout may be inspected with `nm`,
`c++filt`, debugger RTTI commands, or compiler layout dumps. Such output must be
labeled implementation-specific.

### Tests

- contract tests run against every implementation of an interface;
- destruction test through the intended owner/base type;
- negative test for slicing-prone API signatures;
- test that derived behavior preserves base invariants;
- overflow and invalid-input tests for overloaded operators;
- fake implementation test for dependency injection;
- C operation-table test with null/context/lifetime failures;
- multiple-inheritance construction/destruction order test where used.

## 13. Best Practices

- Start with the problem and required variability; do not begin with a class
  hierarchy.
- Prefer a concrete value type unless runtime substitution is required.
- Use a class to maintain an invariant; use a transparent struct when members
  vary independently.
- Keep representation private when an invariant exists.
- Give operations semantic names instead of producing trivial setters.
- Keep interface bases small, cohesive, and stable.
- Use public inheritance only for substitutable types.
- Prefer composition for implementation reuse and configurable behavior.
- State ownership and borrowing separately from inheritance.
- Use `override` on every overriding declaration.
- Use `final` only for an intentional closed extension point.
- Give a polymorphic base a public virtual destructor when clients may delete
  through it, or a protected non-virtual destructor when they must not.
- Prefer `= default` for an otherwise ordinary virtual destructor.
- Avoid protected data.
- Avoid manual `new`/`delete` in ordinary examples and application code.
- Pass polymorphic objects by reference or pointer, not by value.
- Use multiple inheritance primarily for independent interfaces.
- Keep virtual inheritance rare and documented.
- Prefer virtual dispatch to type switching and downcasts when behavior belongs
  in the hierarchy.
- Prefer compile-time polymorphism only when its coupling, code-size, and build
  trade-offs are justified.
- Keep operator behavior unsurprising, bounded, and consistent.
- Measure virtual-call performance on the real target before optimizing it
  away.

## 14. Interview Angles

### Beginner

- Define encapsulation and abstraction without using them as synonyms.
- What is the actual language difference between `class` and `struct`?
- What is inheritance, and when is public inheritance appropriate?
- Compare overloading and overriding.
- What does `override` check?
- What makes a class abstract?
- Why use a pure virtual function?
- What is object slicing?

### Mid-Level

- Why must a polymorphic ownership base usually have a virtual destructor?
- Explain runtime dispatch using static type, dynamic type, and final overrider.
- Explain vtable/vptr as a common ABI model without claiming it is mandated.
- Compare abstract class and interface-style base.
- Compare inheritance and composition for a configurable device.
- Design a small interface following interface segregation.
- Diagnose a hidden overload or missing-`const` override.
- Explain default arguments on virtual functions.
- Explain construction/destruction dispatch restrictions.
- Compare a C operation table with a C++ virtual interface.

### Senior

- Review a hierarchy for substitutability and fragile-base risks.
- Choose concrete types, templates, variants, composition, or virtual dispatch
  for a given variability problem.
- Design ownership for a heterogeneous collection of polymorphic objects.
- Explain public virtual versus protected non-virtual destructor policy.
- Explain multiple inheritance and virtual inheritance costs and initialization.
- Design a C ABI around a C++ implementation without exposing class layout.
- Evaluate whether a plugin interface is source-stable, ABI-stable, both, or
  neither.
- Discuss dependency inversion without introducing a container or framework.
- Explain why protected data harms hierarchy invariants.
- Discuss devirtualization and performance without fixed-cost folklore.

### Interview Traps

- "Encapsulation means getters and setters."
- "Abstraction and encapsulation are the same."
- "Inheritance is for code reuse."
- "Every class with a destructor needs it to be virtual."
- "Every abstract class must have a public virtual destructor."
- "A pure virtual function cannot have a definition."
- "C++ requires one vtable per class and one vptr as the first member."
- "A virtual call always costs a fixed number of nanoseconds."
- "`override` makes a function virtual for the first time."
- "`final` is mainly a performance keyword."
- "Making the base abstract prevents every form of slicing."
- "Private inheritance is composition."
- "A stream operator must be a friend."
- "C cannot implement polymorphism."

## 15. Practice Tasks

### Basic

1. Refactor a struct with public invalid states into a class with one enforced
   invariant and semantic operations.
2. Write a small abstract `Logger` interface and two implementations.
3. Demonstrate overloading and overriding in separate examples.
4. Compile an incorrect override with and without `override`.
5. Demonstrate object slicing, then correct the function signature.

### Intermediate

1. Implement a sensor interface and inject it into an alarm class through a
   borrowed reference; document the lifetime contract.
2. Implement the same runtime dispatch once with C++ virtual functions and once
   with a C function-pointer table.
3. Replace an inheritance-for-reuse design with composition and compare
   coupling.
4. Build interface contract tests reused by two implementations.
5. Demonstrate a non-virtual destructor bug only under a sanitizer-controlled
   negative target; keep it out of normal execution.
6. Create a multiple-interface class without shared data in the interface
   bases.

### Advanced

1. Design a small polymorphic command pipeline with explicit ownership and no
   raw owning pointers.
2. Review a hierarchy that strengthens preconditions in one derived class and
   redesign it for substitutability.
3. Demonstrate name hiding and recover a base overload set with `using`.
4. Build a diamond hierarchy, explain virtual-base initialization, then propose
   a composition-based alternative.
5. Compare virtual dispatch, `std::variant` visitation, and CRTP for one closed
   versus open set of implementations.
6. Inspect vtable-related symbols or class layout on one compiler and document
   why the result is ABI-specific.

## 16. Gaps And External Validation Needs

### Mapped Source Gaps

- Composition receives only brief mention and is not taught alongside
  aggregation, association, and dependency.
- OOP in C and function-pointer-table dispatch are missing.
- Dependency inversion is absent and interface segregation is covered mainly
  through one example.
- Mixins and CRTP are absent.
- Ownership of polymorphic objects is shown mostly with raw pointers rather
  than modern ownership types.
- Substitutability and behavioral contracts are less developed than syntax and
  hierarchy taxonomy.
- Construction/destruction virtual dispatch and default arguments on virtual
  functions need explicit treatment.
- Name hiding and overload-set recovery need explicit treatment.
- ABI stability across polymorphic boundaries is not covered.

### Exact-Behavior Validation

Downstream knowledge/interview/example work should recheck trusted references
when teaching:

- deleting through a base pointer;
- pure virtual function definitions and pure virtual destructors;
- final overrider rules;
- virtual calls during construction and destruction;
- multiple and virtual inheritance initialization order;
- covariant return types;
- object slicing and generated copy/move operations;
- operator rewrite rules across C++ standard versions;
- RTTI and `dynamic_cast`;
- compiler-specific class layout, vtable symbols, and sanitizer commands.

### Version Baseline

Recommended downstream baseline:

- C++17 for primary lessons and examples;
- C17 for the explicit OOP-in-C comparison;
- C++20 side notes only where they improve comparison/operator vocabulary;
- do not teach ABI layout or optimizer behavior as portable language rules.

## 17. Quality Gate For Later Outputs

- Preserve deep treatment for every master MUST concept.
- State CH08 as the prerequisite.
- Teach in the order: problem, contract, relationship choice, mechanism,
  practical code, trade-offs, bugs, debugging, and interview reasoning.
- Include all five master comparisons.
- Distinguish encapsulation from abstraction.
- Distinguish overload resolution from overriding and runtime dispatch.
- Distinguish abstract class as a language property from interface as a design
  convention.
- Explain vtable/vptr as a common implementation model, not a standard mandate.
- State non-virtual polymorphic deletion as undefined behavior, not merely a
  leak.
- Include public virtual and protected non-virtual destructor policies.
- Explain static and dynamic type when teaching virtual calls and slicing.
- Teach composition, aggregation, association, dependency, and ownership as
  separate dimensions.
- Prefer concrete types and composition before introducing inheritance.
- Use `override` consistently and `final` deliberately.
- Keep multiple/virtual inheritance at medium depth and CRTP/mixins brief.
- Use warning-clean, compile-oriented examples with no raw owning pointers in
  primary code.
- Include sanitizer/debug commands only with capability and portability limits.
- Keep CH10 ownership/RAII depth as a forward reference.
- Keep source and audit tables in this Topic Brief, not learner-facing files.
- Do not use Linux Device Driver or kernel-driver material.

## 18. Output Targets

| Output | Current status | Intended scope |
| --- | --- | --- |
| `coverage/topic-briefs/09-oop-in-cpp.md` | Created | Internal source audit, corrections, merged concepts, comparisons, gaps, external validation, and downstream requirements |
| `knowledge/09-oop-in-cpp.md` | Created | Learner-facing OOP lesson without audit metadata |
| `interview/09-oop-in-cpp.md` | Created | Beginner, mid-level, and senior interview pack |
| `examples/09-oop-in-cpp/README.md` and small source files | Created | Minimal C++17 and C17 polymorphism, slicing, composition, and debugging examples |

Audit metadata must remain under `coverage/` and must not be copied into
learner-facing outputs.
