# Topic Brief 18 - Enterprise And Interview Checklist

## 1. Canonical Routing

| Field | Validated value |
| --- | --- |
| `learning_path_number` | `18` |
| Title | Enterprise And Interview Checklist |
| `slug` | `enterprise-and-interview-checklist` |
| Requested topic | Final enterprise/code-review/interview checklist that consolidates memory, pointers, OOP, STL, Modern C++, concurrency, and design-pattern readiness |
| Master source | `master-ch19` |
| Required Notion sources | All relevant Notion best-practice, common-pitfall, summary, and interview sections |
| External reference need | External guidelines for review checklists |
| Topic Brief | `coverage/topic-briefs/18-enterprise-and-interview-checklist.md` |
| Knowledge target | `knowledge/18-enterprise-and-interview-checklist.md` |
| Interview target | `interview/18-enterprise-and-interview-checklist.md` |
| Example target | `examples/18-enterprise-and-interview-checklist/README.md` |

Validation result: the number, title, slug, master source, mapped Notion source
description, and output paths match `LEARNING_PATH.md`.

This step creates the Topic Brief only. It does not create or modify knowledge,
interview, or example outputs.

## 2. Source Coverage

### Internal Sources Read

| Source label | Path | Coverage contribution |
| --- | --- | --- |
| `master-ch19` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, CH19 | Enterprise checklist priority, checklist depth, all-relevant prerequisite policy, and mandatory checklist groups |
| `guide-section-03` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 3 | MUST depth rule and required dimensions for deep/checklist topics |
| `guide-section-04` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 4 | Full chapter/lesson shape for downstream knowledge output |
| `guide-section-06` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 6 | C vs C++ and POSIX/Linux vs Modern C++ comparison table requirements |
| `guide-section-07` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 7 | Depth control and practical-example guidance |
| `guide-section-08` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 8 | Design-pattern priority rules for final pattern checklist |
| `guide-section-09` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 9 | Trusted-source routing for C, C++, safety, enterprise, and POSIX/user-space references |
| `guide-section-10` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 10 | Output style, code-example, and exclusion rules |
| `guide-section-11` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 11 | Interview pack and code-review guide shape |
| `guide-section-12` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 12 | Required comparison pairs to preserve across final checklist outputs |
| `notion-index` | `docs/C++ Notion/C++ Programming.md` | Notion chapter inventory and source identity validation |

### Relevant Notion Chapter Files Read

Topic 18 is a capstone/checklist topic. Because `LEARNING_PATH.md` maps it to
"all relevant Notion best-practice, common-pitfall, summary, and interview
sections," the relevant set is broad: every Notion chapter that contributes
checklist facts, common bugs, best practices, summaries, debugging workflow, or
interview preparation for the CH19 checklist categories was inspected.

| Source label | Path | Coverage contribution |
| --- | --- | --- |
| `notion-1-1` | `docs/C++ Notion/Chapter 1-1 Introduction & Environment Setup.md` | Compiler workflow, warnings, debug symbols, standards flags, GDB/tooling setup, interview essentials |
| `notion-1-2` | `docs/C++ Notion/Chapter 1-2 Variables, Data Types, Storage & Scope.md` | Initialization, scope, storage duration, lifetime, data/BSS/stack concepts, interview points |
| `notion-1-3` | `docs/C++ Notion/Chapter 1-3 Type Conversion & Casting.md` | Implicit/explicit conversions, cast risks, best practices, conversion interview traps |
| `notion-1-4` | `docs/C++ Notion/Chapter 1-4 Operators, Input Output, Control Flow & Loops.md` | Operator semantics, control-flow basics, loop mistakes, I/O basics, interview summary |
| `notion-2-1` | `docs/C++ Notion/Chapter 2-1 Function Basics.md` | Function declaration/definition, return values, common mistakes, best practices, lifetime interview points |
| `notion-2-2` | `docs/C++ Notion/Chapter 2-2 Parameter Passing Techniques.md` | Pass-by-value/reference/pointer, const references, pointer pitfalls, parameter-passing interview rules |
| `notion-2-3` | `docs/C++ Notion/Chapter 2-3 Function Overloading & Name Mangling.md` | Overload resolution, name mangling, ABI/interview awareness, debugging symbols |
| `notion-2-4` | `docs/C++ Notion/Chapter 2-4 Inline Functions.md` | Inline semantics, ODR/header concerns, debug tradeoffs, best practices |
| `notion-2-5` | `docs/C++ Notion/Chapter 2-5 Recursion.md` | Recursion risks, stack overflow, tail-call awareness, recursive debugging/interview points |
| `notion-2-6` | `docs/C++ Notion/Chapter 2-6 Lambda Expressions (C++11).md` | Lambda captures, callable objects, lifetime risks, interview essentials |
| `notion-3-1` | `docs/C++ Notion/Chapter 3-1 Arrays in C++.md` | Bounds, decay, C arrays vs `std::array`/`std::vector`, C-string risks, common mistakes |
| `notion-3-2` | `docs/C++ Notion/Chapter 3-2 Pointers in C++.md` | Pointer arithmetic, dangling/wild/null pointers, ownership risks, smart-pointer transition |
| `notion-3-3` | `docs/C++ Notion/Chapter 3-3 References in C+.md` | References, dangling references, reference vs pointer, best practices |
| `notion-3-4` | `docs/C++ Notion/Chapter 3-4 Strings in C++.md` | `std::string`, C-string API boundaries, `c_str()` lifetime, string best practices |
| `notion-3-5` | `docs/C++ Notion/Chapter 3-5 Structures in C++.md` | Structures, padding/alignment, nested structures, aggregate initialization, common mistakes |
| `notion-3-6` | `docs/C++ Notion/Chapter 3-6 Unions, Enumerations, and Type Aliases in C+.md` | Unions, `enum class`, type aliases, active-member risks, summary/interview points |
| `notion-4-1` | `docs/C++ Notion/Chapter 4-1 Dynamic Memory Basics.md` | Stack vs heap, `new`/`delete`, `new[]`/`delete[]`, leaks, double delete, RAII transition |
| `notion-4-2` | `docs/C++ Notion/Chapter 4-2 Advanced Memory Management.md` | Memory errors, memory debugging tools, Valgrind/ASan, best practices, interview questions |
| `notion-5-1` | `docs/C++ Notion/Chapter 5-1 Classes, Objects & Constructors.md` | Constructors/destructors, encapsulation, `this`, RAII-style object lifetime, interview essentials |
| `notion-5-2` | `docs/C++ Notion/Chapter 5-2 Static Members & Friend Functions.md` | Static members/functions, friend tradeoffs, singleton caution, class-level state risks |
| `notion-5-3` | `docs/C++ Notion/Chapter 5-3 Abstraction & Abstract Classes.md` | Abstract classes, interfaces, pure virtual functions, virtual destructor best practices |
| `notion-5-4` | `docs/C++ Notion/Chapter 5-4 Inheritance & Polymorphism.md` | Inheritance, polymorphism, vtable/vptr, virtual destructors, object slicing, composition tradeoffs |
| `notion-5-5` | `docs/C++ Notion/Chapter 5-5 Operator Overloading.md` | Operator overload best practices, common pitfalls, const-correct operators, interview questions |
| `notion-6-1` | `docs/C++ Notion/Chapter 6-1 STL Introduction & vector Container.md` | STL model, `std::vector`, capacity, `reserve`, iterator invalidation, default container guidance |
| `notion-6-2` | `docs/C++ Notion/Chapter 6-2 Sequence Containers deque, list, forward_list, array.md` | Sequence-container selection, complexity, iterator stability, cache locality |
| `notion-6-3` | `docs/C++ Notion/Chapter 6-3 Container Adapters & Associative Containers.md` | `stack`, `queue`, `priority_queue`, `map`, `set`, comparator rules, container choice |
| `notion-6-4` | `docs/C++ Notion/Chapter 6-4 Unordered Associative Containers.md` | Hash containers, load factor, custom hash, average vs worst-case performance |
| `notion-6-5` | `docs/C++ Notion/Chapter 6-5 Iterators - The Bridge Between Containers and Algorithms.md` | Iterator categories, invalidation, safe erase patterns, algorithm contracts |
| `notion-6-6` | `docs/C++ Notion/Chapter 6-6 STL Algorithms & Functors.md` | Algorithms, functors, predicates, lambdas, comparator/capture traps |
| `notion-7-1` | `docs/C++ Notion/Chapter 7-1 Templates - Function Templates & Class Template.md` | Function/class templates, specialization, template vs macro guidance |
| `notion-7-2` | `docs/C++ Notion/Chapter 7-2 Templates - Variadic Templates & SFINAE.md` | Variadic templates, fold expressions, SFINAE, template complexity cautions |
| `notion-7-3` | `docs/C++ Notion/Chapter 7-3 Templates - Type Traits, Concepts & Metaprogramming.md` | Type traits, concepts, compile-time constraints, static polymorphism best practices |
| `notion-7-4` | `docs/C++ Notion/Chapter 7-4 Templates - Template Template Parameters & Advanced Topics.md` | Advanced template patterns, dependent names, template pitfalls and interview awareness |
| `notion-8-1` | `docs/C++ Notion/Chapter 8-1 Exception Handling - Basics & Standard Exception.md` | Exception basics, standard exceptions, exceptions vs return codes, RAII cleanup |
| `notion-8-2` | `docs/C++ Notion/Chapter 8-2 Exception Handling - Exception Safety & RAII.md` | Exception-safety guarantees, RAII, cleanup under exceptions |
| `notion-8-3` | `docs/C++ Notion/Chapter 8-3 Exception Handling - noexcept, Stack Unwinding.md` | `noexcept`, stack unwinding, destructor rules, best practices, interview prep |
| `notion-9-1` | `docs/C++ Notion/Chapter 9-1 File Handling - Basics to Advanced Operations.md` | File stream states, binary/text files, RAII file handling, file error checks |
| `notion-9-2` | `docs/C++ Notion/Chapter 9-2 File Handling - Interview Questions.md` | File I/O interview scenarios, stream error handling, RAII wrapper discussion |
| `notion-10-1` | `docs/C++ Notion/Chapter 10-1 Namespaces.md` | Namespace hygiene, header `using namespace` pitfalls, ADL/name collision awareness |
| `notion-10-2` | `docs/C++ Notion/Chapter 10-2 Preprocessor Directives.md` | Macro pitfalls, include guards, debug macros, macro vs safer C++ alternatives |
| `notion-10-3` | `docs/C++ Notion/Chapter 10-3 Type Casting.md` | C-style cast risks, `static_cast`, `dynamic_cast`, `const_cast`, `reinterpret_cast`, cast best practices |
| `notion-10-4` | `docs/C++ Notion/Chapter 10-4 Smart Pointers.md` | RAII, `unique_ptr`, `shared_ptr`, `weak_ptr`, ownership, cycles, smart-pointer pitfalls |
| `notion-10-5` | `docs/C++ Notion/Chapter 10-5 Callbacks.md` | Function pointers, lambdas, `std::function`, `std::bind`, callback lifetime and Observer issues |
| `notion-10-6` | `docs/C++ Notion/Chapter 10-6 Move Semantics.md` | Move constructor/assignment, moved-from state, perfect forwarding, Rule of Five |
| `notion-10-7` | `docs/C++ Notion/Chapter 10-7 Signal Handling.md` | Signal safety basics, minimal handlers, atomic flags, signal-handling best practices |
| `notion-10-8` | `docs/C++ Notion/Chapter 10-8 Multithreading Basics.md` | `std::thread`, mutexes, RAII locking, races, thread lifecycle, basic best practices |
| `notion-10-9` | `docs/C++ Notion/Chapter 10-9 Multithreading Advanced.md` | Condition variables, deadlock avoidance, async/future, atomics, advanced best practices |
| `notion-10-10` | `docs/C++ Notion/Chapter 10-10 Modern C++ Features.md` | Modern feature summary: `auto`, range-for, `constexpr`, `optional`, `variant`, `string_view`, `span` |

No relevant Notion chapter file was skipped. The selection intentionally
excludes Linux Device Driver/kernel-driver material; none was used.

### External References Used Or Needed

External references are needed because the target asks for "external guidelines
for review checklists" and because a final checklist benefits from guideline
families rather than only tutorial notes.

| Label | Reference | Validation purpose |
| --- | --- | --- |
| `external-core-guidelines` | C++ Core Guidelines: <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines> | Ownership, RAII, interfaces, resource management, type safety, concurrency, and rule/checklist language |
| `external-sei-cert-cpp` | SEI CERT C++ Coding Standard: <https://wiki.sei.cmu.edu/confluence/pages/viewpage.action?pageId=88046682> | Secure-coding review categories: undefined behavior, resource lifetime, error handling, concurrency, and API misuse |
| `external-sei-cert-c` | SEI CERT C Coding Standard: <https://wiki.sei.cmu.edu/confluence/display/c/SEI+CERT+C+Coding+Standard> | C-side memory, strings, integer, pointer, and library safety checklist support |
| `external-cppreference` | cppreference C++ reference: <https://en.cppreference.com/w/cpp> | Exact API anchors for standard library facilities appearing in the checklist |
| `external-cppreference-c` | cppreference C reference: <https://en.cppreference.com/w/c> | Exact C library/API anchors for C comparison points |

No POSIX/Linux man-page deep dive is required for this topic. POSIX/user-space
comparison can be referenced from topic 16 outputs when needed. Linux Device
Driver and kernel-driver references are explicitly out of scope.

### Source Coverage Status

`TOPIC_BRIEF_COMPLETE_WITH_ENTERPRISE_CHECKLIST_REFERENCES`: canonical routing,
master checklist, guide rules, all relevant Notion best-practice/pitfall/summary
and interview sections, external review-guideline references, merged concepts,
comparisons, bugs, debugging notes, best practices, interview angles, gaps, and
target outputs are recorded.

## 3. Priority And Dependencies

- Overall priority: `MUST`.
- Depth: `Checklist`.
- Master prerequisite: all relevant chapters.
- Downstream output should be a structured readiness checklist, not a new deep
  textbook chapter for every item.
- Required checklist groups from `master-ch19`:
  - Memory Checklist.
  - Pointer Checklist.
  - OOP Checklist.
  - STL Checklist.
  - Modern C++ Checklist.
  - Concurrency Checklist.
  - Design Pattern Checklist.

Practical dependencies:

- Build/debug workflow: compiler warnings, debug symbols, sanitizer/Valgrind
  workflow, reproducible build commands.
- C fundamentals: initialization, storage duration, lifetime, casts, arrays,
  pointers, strings, structs/unions/enums, dynamic allocation.
- C++ foundations: classes, constructors/destructors, RAII, copy/move control,
  virtual dispatch, interfaces, exceptions.
- Standard library: containers, iterators, algorithms, smart pointers,
  `std::optional`, `std::variant`, `std::string_view`, `std::span`.
- Concurrency: thread lifetime, mutexes, condition variables, atomics,
  producer-consumer, race/deadlock/spurious wakeup handling.
- Design principles/patterns: principles before patterns, State/FSM, Strategy,
  Observer, Factory Method, Adapter, Facade, Command.

## 4. Scope And Depth Boundaries

### Deep In This Topic

- Final interview/readiness checklist across memory, pointers, OOP, STL,
  Modern C++, concurrency, and design patterns.
- Enterprise code review checklist:
  - correctness;
  - undefined behavior;
  - memory/resource ownership;
  - lifetime;
  - exception safety;
  - thread safety;
  - API design;
  - performance and complexity;
  - maintainability;
  - testing/debugging readiness.
- "Can you explain and debug it?" interview framing for each major area.
- Practical pass/fail signals for real review: owns vs borrows, invalidation,
  cleanup path, lock policy, error propagation, measurable complexity.

### Medium In This Topic

- C vs C++ comparisons needed for final checklist reasoning.
- POSIX/Linux user-space vs Modern C++ reminders only when they affect
  checklist categories such as file I/O or concurrency.
- Tooling summary:
  - compiler warnings;
  - sanitizers;
  - Valgrind/heap tools;
  - GDB/backtraces;
  - static analysis;
  - unit tests and concurrency stress tests.

### Short Awareness In This Topic

- Advanced template/metaprogramming details.
- Deep atomics and memory ordering beyond practical checklist signals.
- Design patterns outside the priority list.
- Platform-specific system API details already handled in topic 16.

### Defer Or Exclude

- Do not reteach every prior topic in full; link checklist items back to earlier
  concepts.
- Do not create a Linux Device Driver/kernel-driver checklist.
- Exclude Yocto, GStreamer, AUTOSAR, cloud, and unrelated platform material.
- Do not turn the checklist into a company-specific coding standard.

## 5. Merged Concept Map

- The final checklist is a readiness tool: it should answer whether the learner
  can explain, write, review, debug, and interview on core C/C++ topics.
- Enterprise quality is not just "code compiles." It requires:
  - clear ownership and lifetime;
  - valid object invariants;
  - defined behavior;
  - predictable error handling;
  - documented concurrency policy;
  - stable interfaces;
  - testable design;
  - measurable performance assumptions.
- Interview readiness combines:
  - short definitions;
  - mechanism-level explanation;
  - code/API examples;
  - common bugs;
  - debugging method;
  - production tradeoffs.
- Code review should flow from high-risk to low-risk:
  - correctness and UB first;
  - resource lifetime and exception safety next;
  - concurrency and API contracts next;
  - performance and maintainability after behavior is sound.
- C and C++ must be compared by ownership model:
  - C often exposes manual lifetime through pointers, buffers, and cleanup
    calls;
  - C++ should prefer RAII, type-safe containers, smart pointers, and standard
    abstractions where suitable.
- Modern C++ features are not automatic quality. `auto`, lambdas,
  `std::optional`, `std::variant`, `std::string_view`, and `std::span` improve
  code only when lifetimes, ownership, and API meaning remain clear.
- Concurrency checklist items must distinguish data races, race conditions,
  deadlocks, missed notifications, spurious wakeups, and atomic misuse.
- Design patterns should be checked as problem/solution fit, not name-dropping.

## 6. Required Checklist Areas

### Memory Checklist

Must preserve:

- stack vs heap;
- data vs BSS;
- alignment;
- padding;
- endianness;
- `malloc` vs `calloc`;
- `malloc` vs `new`;
- `free` vs `delete`;
- `new[]` vs `delete[]`;
- memory leak;
- dangling pointer;
- double free;
- use-after-free;
- buffer overflow;
- shallow copy vs deep copy;
- RAII.

Review focus:

- Is every resource owned by exactly one clear owner or intentionally shared?
- Does every allocation have a matching deallocation mechanism?
- Can exceptions or early returns skip cleanup?
- Are buffers sized and bounds-checked?
- Are layout assumptions portable or explicitly documented?

### Pointer Checklist

Must preserve:

- pointer vs array;
- pointer arithmetic;
- double pointer;
- function pointer;
- `void*`;
- const pointer;
- pointer to const;
- null pointer;
- dangling pointer;
- `nullptr` vs `NULL`.

Review focus:

- Does the pointer own, borrow, observe, or provide output?
- Is pointer arithmetic constrained to the same array/object?
- Are nullability and lifetime documented?
- Is `void*` paired with clear type and context ownership?

### OOP Checklist

Must preserve:

- encapsulation;
- abstraction;
- inheritance;
- polymorphism;
- virtual function;
- vtable/vptr;
- pure virtual function;
- abstract class;
- interface;
- virtual destructor;
- object slicing;
- composition vs inheritance.

Review focus:

- Are invariants protected by class boundaries?
- Does inheritance model substitutable "is-a" behavior?
- Does every polymorphic base with deletion through base pointer have a virtual
  destructor?
- Are objects passed by reference/pointer when polymorphism is required?
- Would composition be simpler than inheritance?

### STL Checklist

Must preserve:

- `vector`;
- `array`;
- `map`;
- `set`;
- `unordered_map`;
- `unordered_set`;
- `stack`;
- `queue`;
- `priority_queue`;
- iterator;
- algorithm;
- comparator;
- iterator invalidation;
- complexity.

Review focus:

- Is the container chosen for access pattern, ordering, stability, and memory
  behavior?
- Are iterator/reference invalidation rules respected after insert/erase/growth?
- Are comparators strict weak orderings?
- Is hash quality sufficient for unordered containers?
- Are standard algorithms used to express intent?

### Modern C++ Checklist

Must preserve:

- RAII;
- smart pointer;
- move semantics;
- lambda;
- `auto`;
- `constexpr`;
- `noexcept`;
- `std::optional`;
- `std::variant`;
- `std::string_view`;
- `std::span`.

Review focus:

- Prefer `std::unique_ptr` for exclusive ownership and `std::shared_ptr` only
  for true shared ownership.
- Avoid dangling `std::string_view`, `std::span`, references, and lambda
  captures.
- Leave moved-from objects valid and use them only according to documented
  post-move expectations.
- Use `noexcept` where it is truthful and important for move operations or
  cleanup.
- Use `std::optional` for optional value, `std::variant` for closed alternatives,
  and exceptions/error codes for failures according to API boundary policy.

### Concurrency Checklist

Must preserve:

- thread;
- mutex;
- lock;
- condition variable;
- semaphore;
- atomic;
- race condition;
- deadlock;
- spurious wakeup;
- producer-consumer;
- thread pool.

Review focus:

- Is every shared mutable state protected by a lock or atomic protocol?
- Is lock order documented when multiple mutexes exist?
- Does every `condition_variable::wait` use a predicate loop?
- Are threads joined, detached intentionally, or owned by RAII wrappers?
- Are atomics used for data-race freedom, not as a replacement for full
  invariants requiring locks?

### Design Pattern Checklist

MUST:

- State / FSM;
- Strategy;
- Observer;
- Factory Method;
- Adapter;
- Facade;
- Command.

SHOULD:

- Builder;
- Decorator;
- Proxy;
- Template Method;
- Chain of Responsibility;
- Mediator;
- Iterator;
- Composite;
- Prototype.

NICE:

- Visitor;
- Memento;
- Flyweight;
- Bridge;
- Abstract Factory.

Review focus:

- Is the pattern solving real variation, lifetime, coupling, or subsystem
  complexity?
- Is the simple solution shown or considered first?
- Are ownership, virtual destructors, callback lifetimes, and exception behavior
  clear?
- Is the pattern name consistent with its actual structure?

## 7. Required Comparisons To Preserve

Downstream knowledge and interview outputs should preserve these comparisons
because the expansion guide marks them as core final-checklist reasoning.

### C vs C++ Checklist Comparisons

| Topic | C | C++ | Enterprise / Embedded Guidance |
| --- | --- | --- | --- |
| Resource cleanup | Manual cleanup paths | RAII destructors and smart pointers | Prefer RAII in C++; in C use explicit cleanup labels and ownership comments |
| Dynamic allocation | `malloc`/`calloc`/`realloc`/`free` | `new`/`delete`, containers, smart pointers | Avoid mixing allocation families; prefer containers/smart pointers |
| Arrays | Raw contiguous storage, easy decay | `std::array`, `std::vector`, `std::span` | Track bounds and lifetime; beware dangling views |
| Strings | `char*`, C string functions | `std::string`, `std::string_view` | Avoid buffer overflow; beware `string_view` and `c_str()` lifetime |
| Callbacks | Function pointer plus context pointer | Lambda, functor, `std::function`, interface | Document capture/context lifetime and ownership |
| Error handling | Return codes, `errno`, out parameters | Exceptions, error codes, optional/variant/expected-style | Pick one policy per API boundary; preserve diagnostic detail |
| Polymorphism | Function pointers and structs of operations | Virtual interfaces or templates | Prefer simple mechanisms; check lifetime and virtual destructors |
| Macros | Preprocessor substitution | `constexpr`, inline functions, templates | Use macros only for conditional compilation or unavoidable cases |

### POSIX/User-Space Vs Modern C++ Checklist Comparisons

| Topic | C/POSIX/Linux user-space | Modern C++ | Enterprise Usage |
| --- | --- | --- | --- |
| Thread creation | `pthread_create` | `std::thread`, `std::jthread` when available | Define ownership and join/cancel policy |
| Mutex | `pthread_mutex_t` | `std::mutex`, `std::lock_guard`, `std::unique_lock` | Prefer RAII locking; document lock order |
| Condition variable | `pthread_cond_t` | `std::condition_variable` | Always wait with predicate loop |
| File I/O | `open/read/write/close` | `fstream`, `filesystem`, RAII wrappers | Check errors and close through RAII |
| Time | `clock_gettime`, `nanosleep` | `std::chrono` | Use steady clock for durations/timeouts |
| Atomic | C11 `_Atomic`, compiler builtins | `std::atomic` | Avoid `volatile` for synchronization |

Keep POSIX detail at checklist/reminder depth. Full user-space API teaching
belongs to topic 16.

## 8. Common Bugs To Capture

- Uninitialized variables and uninitialized pointers.
- Out-of-bounds array/vector access.
- Pointer arithmetic outside the same array.
- Returning address/reference/view of a local object.
- Storing `std::string::c_str()` or `std::string_view` past object lifetime.
- Dangling lambda captures by reference.
- Memory leaks from early returns, exceptions, or ownership confusion.
- Double delete, mismatched `new[]`/`delete`, and mixed `malloc`/`delete`.
- Shallow copy of owning pointer/resource.
- Missing virtual destructor in polymorphic base.
- Object slicing by passing derived objects by value.
- Overusing inheritance when composition is enough.
- Invalidated iterators/references after container mutation.
- Incorrect comparator or hash/equality mismatch.
- Assuming unordered containers are always O(1).
- Catching exceptions too broadly or throwing from destructors.
- Incorrect `noexcept`.
- Using `std::shared_ptr` as default ownership.
- Cyclic `shared_ptr` ownership without `weak_ptr`.
- Using `auto` when it hides reference/value or signedness behavior.
- Using `std::move` and then reading moved-from state as if unchanged.
- Data races on shared mutable state.
- Deadlocks from inconsistent lock ordering.
- `condition_variable` wait without predicate.
- Confusing `volatile` with `std::atomic`.
- Calling non-async-signal-safe work from a signal handler.
- Pattern name-dropping without a real design problem.

## 9. Debugging And Review Notes

- Build with warnings enabled and treat serious warnings as bugs:
  `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`.
- Use debug symbols (`-g`) and keep optimized/release builds reproducible.
- Use AddressSanitizer/UndefinedBehaviorSanitizer for memory and UB checks.
- Use ThreadSanitizer for race detection in threaded examples or projects.
- Use Valgrind/heap tooling where sanitizer support is unavailable or as a
  second signal for leaks/use-after-free.
- Use GDB/backtraces to inspect crashes, virtual dispatch call paths, stack
  overflow from recursion, and exception paths.
- For STL bugs, inspect iterator invalidation points and container complexity
  assumptions.
- For concurrency bugs, log thread IDs, lock acquisition order, wait predicates,
  and shared-state ownership.
- For API review, classify every parameter and return value as owner, borrower,
  observer, optional value, error channel, or callback.
- For pattern/design review, ask whether removing the abstraction would make the
  code simpler without losing needed variation.

## 10. Best Practices To Preserve

- Prefer initialization at declaration.
- Prefer scoped lifetimes and RAII over manual cleanup.
- Prefer standard containers and algorithms over raw arrays and manual loops
  when constraints allow.
- Prefer `std::unique_ptr` for ownership; use raw pointers/references for
  non-owning access only when lifetime is obvious.
- Prefer `std::array`, `std::vector`, `std::string`, `std::span`, and
  `std::string_view` with explicit lifetime rules.
- Avoid global mutable state; if unavoidable, document initialization order and
  thread-safety.
- Keep interfaces small and intention-revealing.
- Mark overrides with `override`; use `final` only when it communicates design.
- Use virtual destructors for polymorphic base classes.
- Use `const` to express read-only intent.
- Avoid C-style casts in C++; prefer named casts with narrow scope.
- Keep exception boundaries clear and preserve error context.
- Make destructors non-throwing.
- Define copy/move/destructor behavior for resource-owning classes.
- Use RAII lock wrappers; keep critical sections small.
- Use condition-variable predicates.
- Do not use `volatile` as a threading primitive.
- Test ownership, error paths, boundary values, invalid inputs, and concurrent
  behavior.
- Run static analysis and sanitizer/debug builds in review workflows.
- Treat design patterns as vocabulary after the design problem is identified.

## 11. Interview Angles

### Beginner

- Explain stack vs heap and when each object is destroyed.
- Explain pointer vs reference.
- Explain array decay and why out-of-bounds access is undefined behavior.
- Explain `nullptr` vs `NULL`.
- Explain class vs object, encapsulation, constructor, destructor.
- Explain `std::vector` vs raw array.
- Explain `std::unique_ptr` in one sentence.

### Mid-Level

- Compare `malloc/free` with `new/delete` and RAII.
- Explain shallow copy vs deep copy and the Rule of Three/Five/Zero.
- Explain virtual destructor and object slicing.
- Explain iterator invalidation for `vector`, `list`, and associative
  containers.
- Compare `std::map` and `std::unordered_map`.
- Explain lambda captures and dangling capture bugs.
- Compare error codes and exceptions.
- Explain `std::mutex`, RAII locks, and deadlock prevention.
- Compare Strategy, Observer, and Command by problem shape.

### Senior

- Review a class for ownership, exception safety, and copy/move correctness.
- Diagnose a data race or missed notification from code.
- Decide between runtime polymorphism, templates, callbacks, and simple
  functions.
- Design an API boundary around C code or a legacy subsystem.
- Explain when `shared_ptr` is justified and how to avoid cycles.
- Explain `std::atomic` vs mutex at invariant level.
- Explain how to structure an enterprise code-review checklist.
- Identify over-engineered design patterns and simplify them.

## 12. Practice And Output Targets

### Knowledge Output Target

`knowledge/18-enterprise-and-interview-checklist.md` should be a learner-facing
checklist lesson, not an audit table. It should teach in this order:

1. goal;
2. why the checklist matters;
3. mental model for enterprise/interview readiness;
4. mechanism of using a checklist in code review;
5. C/C++ API/code anchors;
6. practical review workflow;
7. comparisons;
8. bugs;
9. debugging;
10. best practices;
11. interview readiness;
12. practice.

### Interview Output Target

`interview/18-enterprise-and-interview-checklist.md` should include:

- beginner, mid-level, and senior checklist questions;
- model answers with short answer, deep explanation, API/code anchor,
  production/debug angle, traps, and follow-ups;
- review scenarios and debugging questions;
- "red flag" answer patterns for interviews.

### Example Output Target

This topic may not need a large compile-ready example because it is a checklist
capstone. If examples are created, prefer a compact `README.md` with:

- review checklist command snippets;
- sanitizer/debug command examples;
- one or two tiny intentionally flawed snippets for review practice;
- no large new C++ program unless it materially improves the checklist.

## 13. Gaps, Uncertainties, And External Validation Needs

- `LEARNING_PATH.md` does not enumerate exact Notion chapter IDs for topic 18;
  it says "all relevant" best-practice/common-pitfall/summary/interview
  sections. This brief resolves that by inspecting the full Notion chapter set
  and selecting all chapters that contribute checklist facts.
- The Notion source set is C++-oriented. C-specific enterprise checklist rules
  should be validated against SEI CERT C and cppreference C when downstream
  output expands C memory/string/pointer safety.
- The C++ checklist should use C++ Core Guidelines and SEI CERT C++ as external
  validation references for review language, but should not quote or reproduce
  them wholesale.
- No Linux Device Driver/kernel-driver material is needed or allowed.

## 14. Quality Checklist For Downstream Outputs

- Use checklist format, not a giant duplicate textbook.
- Preserve all CH19 checklist categories.
- Include C vs C++ comparison where checklist topics cross language boundaries.
- Include POSIX/user-space vs Modern C++ comparison only as review reminders.
- Include memory safety, UB, ownership/lifetime, RAII, exception safety,
  concurrency, API design, performance, maintainability, and interview value.
- Keep source coverage/audit metadata out of learner-facing docs.
- Mark learning-only examples and warn about unsafe APIs, UB, iterator
  invalidation, dangling views/captures, races, deadlocks, and exception-safety
  risks.
- Do not include Linux Device Driver/kernel-driver, Yocto, GStreamer, AUTOSAR,
  cloud, or unrelated platform material.
