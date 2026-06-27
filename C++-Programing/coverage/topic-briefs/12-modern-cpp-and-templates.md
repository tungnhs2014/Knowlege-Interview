# Topic Brief 12 - Modern C++ And Templates

## 1. Canonical Routing

| Field | Validated value |
| --- | --- |
| `learning_path_number` | `12` |
| Title | Modern C++ And Templates |
| `slug` | `modern-cpp-and-templates` |
| Requested topic | Modern C++ language features, value categories, smart pointers, move semantics, lambdas, vocabulary types, templates, concepts, SFINAE, and practical generic programming |
| Master source | `master-ch12`, `master-ch13` |
| Required Notion sources | `notion-2-6`, `notion-7-1`, `notion-7-2`, `notion-7-3`, `notion-7-4`, `notion-10-4`, `notion-10-6`, `notion-10-10` |
| Topic Brief | `coverage/topic-briefs/12-modern-cpp-and-templates.md` |
| Knowledge target | `knowledge/12-modern-cpp-and-templates.md` |
| Interview target | `interview/12-modern-cpp-and-templates.md` |
| Example target | `examples/12-modern-cpp-and-templates/README.md` |

Validation result: the number, title, slug, master sources, eight mapped Notion
sources, and all canonical output paths match `LEARNING_PATH.md`.

This step creates the Topic Brief only. It does not create or modify knowledge,
interview, or example outputs.

## 2. Source Coverage

### Internal Sources Read

| Source label | Path | Coverage contribution |
| --- | --- | --- |
| `master-ch12` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, CH12 | Modern C++ priority, CH11 prerequisite, C++11/14/17/20/23 keyword scope, required comparisons, safety/readability rule, and interview focus |
| `master-ch13` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, CH13 | Template priority split, CH12 prerequisite, template/generic-programming keywords, required comparisons, beginner depth boundary, and interview focus |
| `master-ch19-modern-checklist` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, CH19 Modern C++ checklist | Reinforcement for RAII, smart pointers, move semantics, lambdas, `auto`, `constexpr`, `noexcept`, `std::optional`, `std::variant`, `std::string_view`, and `std::span` |
| `guide-section-03` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 3 | MUST/SHOULD/NICE/EXPERT depth control |
| `guide-section-04` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 4 | Full chapter structure and C, C++, embedded, enterprise, bug, debug, interview, practice, and reference angles |
| `guide-section-06` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 6 | Required comparison routing for C vs C++ and POSIX/Linux vs Modern C++ topics |
| `guide-section-09` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 9 | Trusted reference routing for C++ language/library and safety/enterprise guidance |
| `guide-section-10` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 10 | Clear technical English, concise Markdown, compile-oriented examples, and risk warnings |
| `guide-section-11` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 11 | Lesson type rules for full lessons, interview packs, examples, and comparison notes |
| `guide-section-12` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 12 | Required comparison pairs including function pointer/lambda/`std::function` and macro/`constexpr`/template |
| `guide-section-14` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 14 | Final quality checklist |
| `notion-index` | `docs/C++ Notion/C++ Programming.md` | Notion inventory and mapped chapter identity validation |
| `notion-2-6` | `docs/C++ Notion/Chapter 2-6 Lambda Expressions (C++11).md` | Lambda syntax, captures, `this` and `*this`, `mutable`, init-capture, generic lambdas, STL algorithm usage, function-pointer/functor/`std::function` comparison, closure model, and interview questions |
| `notion-7-1` | `docs/C++ Notion/Chapter 7-1 Templates - Function Templates & Class Template.md` | Function/class templates, type deduction, non-type template parameters, class templates, specialization, partial specialization, variable templates, macro comparison, and template interview questions |
| `notion-7-2` | `docs/C++ Notion/Chapter 7-2 Templates - Variadic Templates & SFINAE.md` | Parameter packs, pack expansion, `sizeof...`, variadic classes, perfect forwarding, fold expressions, SFINAE, `std::enable_if`, `std::void_t`, member detection, and SFINAE interview questions |
| `notion-7-3` | `docs/C++ Notion/Chapter 7-3 Templates - Type Traits, Concepts & Metaprogramming.md` | Type traits, type relationships, type transformations, custom traits, `integral_constant`, concepts, `requires`, concept subsumption, concepts vs SFINAE, TMP, `constexpr` as TMP alternative, and interview questions |
| `notion-7-4` | `docs/C++ Notion/Chapter 7-4 Templates - Template Template Parameters & Advanced Topics.md` | Template-template parameters, alias templates, CTAD/deduction guides, dependent names, `typename`, dependent `template`, two-phase lookup, template friends, policy examples, and interview questions |
| `notion-10-4` | `docs/C++ Notion/Chapter 10-4 Smart Pointers.md` | RAII, raw pointer hazards, `std::unique_ptr`, `std::shared_ptr`, `std::weak_ptr`, `make_unique`, `make_shared`, custom deleters, smart pointer comparison, cycles, `get()`, `release()`, `enable_shared_from_this`, and ownership best practices |
| `notion-10-6` | `docs/C++ Notion/Chapter 10-6 Move Semantics.md` | Move semantics, value categories, rvalue references, move constructor/assignment, `std::move`, `noexcept`, Rule of Five/Zero, perfect forwarding, `std::forward`, RVO, and move pitfalls |
| `notion-10-10` | `docs/C++ Notion/Chapter 10-10 Modern C++ Features.md` | Standard-version feature map, `auto`, range-for, `nullptr`, uniform initialization, `enum class`, `static_assert`, generic lambdas, `if constexpr`, fold expressions, inline variables, nested namespaces, `std::string_view`, concepts, `constexpr`, structured bindings, `std::optional`, `std::variant`, `std::any`, ranges, modules, and C++20/23 awareness |

All eight mapped Notion chapter files were inspected. No mapped Notion source
was skipped.

### External References Consulted

Accessed on 2026-06-27.

| Label | Reference | Validation purpose |
| --- | --- | --- |
| `external-cppreference-constexpr` | cppreference `constexpr`: <https://en.cppreference.com/w/cpp/language/constexpr> | Exact distinction between `constexpr` declarations, constant-expression usability, and version-sensitive behavior |
| `external-cppreference-consteval` | cppreference `consteval`: <https://en.cppreference.com/w/cpp/language/consteval> | C++20 immediate-function behavior |
| `external-cppreference-constinit` | cppreference `constinit`: <https://en.cppreference.com/cpp/language/constinit> | C++20 static/thread storage initialization rule |
| `external-cppreference-lambda` | cppreference lambda expressions: <https://en.cppreference.com/cpp/language/lambda> | Closure object behavior and dangling-reference rule for reference captures and `this` captures |
| `external-cppreference-templates` | cppreference templates: <https://en.cppreference.com/cpp/language/templates> | Template declaration and invalid template-id/SFINAE boundary |
| `external-cppreference-sfinae` | cppreference SFINAE: <https://en.cppreference.com/cpp/language/sfinae> | Substitution failure behavior and partial-specialization SFINAE boundary |
| `external-cppreference-partial-specialization` | cppreference partial specialization: <https://en.cppreference.com/cpp/language/partial_specialization> | Class/variable template partial specialization placement and primary-template relationship |
| `external-cppreference-string-view` | cppreference `std::basic_string_view`: <https://en.cppreference.com/cpp/string/basic_string_view> | Non-owning contiguous character view model and lifetime teaching precision |
| `external-cppreference-optional` | cppreference `std::optional`: <https://cppreference.com/cpp/utility/optional> | Optional value/non-value vocabulary type behavior and C++26 view note awareness |

### Source Coverage Status

`TOPIC_BRIEF_COMPLETE_WITH_EXTERNAL_VALIDATION`: canonical routing, both mapped
master chapters, guide requirements, every mapped Notion source, cppreference
validation needs, merged concepts, comparisons, bugs, debugging workflow,
interview angles, practice targets, and downstream quality gates are recorded.

The internal sources cover the topic well. Downstream learner-facing output
should still validate exact standard-version behavior against cppreference,
especially for `constexpr` evolution, concepts, SFINAE boundaries, lambda
captures, `std::string_view` lifetime, CTAD, and smart-pointer details.

## 3. Priority And Dependencies

- Overall priority: `MUST` for common Modern C++ features from C++11/14/17,
  value categories, move semantics, lambdas, smart pointers, `constexpr`,
  `noexcept`, `auto`, `nullptr`, `std::optional`, `std::variant`, and
  `std::string_view`.
- Supporting priority: `SHOULD` for C++20/23 basics, structured bindings,
  `if constexpr`, `std::any`, `std::span`, `std::filesystem`, `std::chrono`,
  perfect forwarding, forwarding references, `std::forward`, `consteval`,
  `constinit`, concepts, ranges basics, `std::expected`, type traits, SFINAE
  basics, generic lambdas, and non-type template parameters.
- Template priority: selected basics are `MUST`:
  function templates, class templates, template parameters, instantiation, and
  specialization basics.
- Awareness priority: `NICE` for coroutines, modules, spaceship operator,
  ranges advanced, partial specialization, template-template parameters, tag
  dispatch, CRTP, and policy-based design.
- Controlled advanced priority: `EXPERT` for template metaprogramming,
  expression templates, advanced SFINAE, and compile-time reflection when
  available.
- Master prerequisites:
  - CH11, STL And Standard Library, for containers, iterators, algorithms,
    callables, invalidation, and generic-library usage.
  - CH12 is prerequisite for CH13 template material.
  - CH10, Resource Management In C++, remains a practical dependency for smart
    pointers, move semantics, RAII, and exception safety.
- Required prior model:
  - Object lifetime, stack/heap, constructors/destructors, RAII, and ownership.
  - Copy construction, copy assignment, value semantics, and references.
  - STL containers and algorithms.
  - Function overloading, callable objects, and callbacks.
  - Basic compile/link model for templates in headers.
- Follow-on chapters:
  - CH14 Error Handling for exception safety, `noexcept`, `std::expected`, and
    optional/variant-based APIs.
  - CH15 Concurrency for move-only thread handles, lambdas in thread creation,
    atomics, and lifetime hazards.
  - CH16/17 Design and C vs C++ comparisons.
  - CH18 POSIX/Linux C API vs Modern C++ when wrapping system APIs.

## 4. Scope And Depth Boundaries

### Deep In This Topic

- Modern C++ as a safety/readability/performance upgrade, not just syntax.
- `auto`, `decltype`, `nullptr`, range-for, uniform initialization,
  `enum class`, `override`, `final`, and `static_assert`.
- Lambda syntax, closure objects, captures by value/reference, default captures,
  `this` and `*this`, `mutable`, init-capture, move-only capture, generic
  lambdas, and STL algorithm usage.
- Lambda vs function pointer vs functor vs `std::function`.
- Value categories: lvalue, rvalue, glvalue, prvalue, xvalue.
- Rvalue references, move constructor, move assignment, `std::move`,
  moved-from valid-but-unspecified state, `noexcept`, RVO/copy elision, Rule of
  Five, and Rule of Zero.
- Perfect forwarding, forwarding references, reference collapsing at a practical
  level, and `std::forward`.
- Smart pointers as ownership vocabulary: `unique_ptr`, `shared_ptr`,
  `weak_ptr`, `make_unique`, `make_shared`, custom deleters, cycles, and
  ownership transfer.
- `constexpr`, `if constexpr`, compile-time constants, `consteval`, and
  `constinit` at practical standard-version depth.
- Vocabulary types: `std::optional`, `std::variant`, `std::any`,
  `std::string_view`, and `std::span`.
- Template basics: function templates, class templates, template type
  deduction, template instantiation, non-type template parameters, default
  template arguments, specialization, partial specialization, and variable
  templates.
- Variadic templates, parameter packs, pack expansion, `sizeof...`, and fold
  expressions.
- Type traits and custom traits.
- SFINAE basics with `std::enable_if`, `_t` aliases, `std::void_t`, detection
  idioms, and practical constraints.
- C++20 concepts, `requires`, requires expressions, standard concepts, custom
  concepts, and concept subsumption.
- Reading template errors and turning compiler diagnostics into constraints.

### Medium In This Topic

- Structured bindings, inline variables, nested namespaces, CTAD, deduction
  guides, alias templates, dependent names, `typename`, dependent `template`,
  and two-phase lookup.
- Template-template parameters and policy-based design as practical but
  controlled advanced material.
- `std::filesystem`, `std::chrono`, ranges basics, and `std::expected` as
  modern vocabulary/API awareness.
- `std::shared_ptr` thread-safety boundary: reference-count operations are safe,
  but the pointed object is not automatically thread-safe.
- `make_shared` allocation/control-block tradeoffs and when custom deleters or
  weak retention concerns change the choice.

### Controlled Awareness

- Coroutines.
- Modules.
- Spaceship operator.
- Advanced ranges and views.
- CRTP.
- Tag dispatch.
- Expression templates.
- Advanced SFINAE.
- Deep template metaprogramming.
- Compile-time reflection when available.

### Defer Or Exclude

- Full design-pattern teaching: route to topic 17.
- Full exception hierarchy and policy: route to topic 13.
- Full concurrency/threading: route to topic 14.
- POSIX/Linux system API wrappers: route to topic 16.
- Kernel-driver, Linux Device Driver, Yocto, GStreamer, AUTOSAR, or unrelated
  platform material.

## 5. Mapped Source Corrections And Precision Notes

- `std::move` does not move by itself. It casts to an xvalue/rvalue reference;
  an actual move happens only if a move constructor/assignment or move-aware
  overload is selected.
- A moved-from standard-library object is valid but its value is generally
  unspecified unless the specific type documents more. Do not teach "empty" as
  a universal guarantee.
- Avoid `return std::move(local);` for ordinary local returns because it can
  inhibit copy elision or natural move selection.
- Move operations should be `noexcept` when they cannot throw; containers such
  as `std::vector` may prefer copying during reallocation if moving can throw.
- `unique_ptr` has array support via `unique_ptr<T[]>`; `shared_ptr<T[]>` has
  standard support in modern C++, but many teaching examples still use a custom
  deleter for older or explicit patterns. Validate version wording downstream.
- `make_shared` is often efficient, but it co-allocates object and control
  block. With lingering `weak_ptr`s, object memory may remain tied to the
  control block lifetime. Mention only when teaching advanced ownership.
- `shared_ptr` reference count changes are thread-safe, but accessing the
  managed object needs its own synchronization.
- `use_count()` is usually a diagnostic aid, not a robust ownership decision in
  concurrent or complex code.
- `string_view` is non-owning. It must not outlive the referenced character
  sequence; returning or storing views requires a clear lifetime owner.
- `optional<T>` represents "maybe a T"; it is not a universal error-reporting
  replacement when error reasons matter.
- `variant` is a type-safe union-like vocabulary type. Teach `std::visit` and
  `holds_alternative` rather than unsafe index assumptions.
- `auto` should not hide important ownership/reference information. Prefer
  `auto&`, `const auto&`, or explicit type when lifetime or ownership clarity
  matters.
- Generic lambdas generate a closure type with a templated call operator. This
  links lambdas directly to templates.
- Lambdas that capture by reference, including `this`, do not extend object
  lifetimes. Async callbacks and stored lambdas must use value capture,
  move-capture, or lifetime-managed ownership.
- Non-capturing lambdas can convert to function pointers. Capturing lambdas
  cannot because they carry state.
- `std::function` type-erases callable objects and may allocate or add indirect
  call overhead. Use it for storage/runtime polymorphism, not as the default for
  every callable parameter.
- Templates are not macros. They are typed compile-time code generation with
  overload resolution, scope, diagnostics, and instantiation rules.
- Function templates cannot be partially specialized. Use overloading, class
  template partial specialization, traits, or concepts depending on intent.
- Non-type template parameter rules changed across standards; validate exact
  C++11/14/17/20/23 allowances before making precise claims.
- Template definitions usually need to be visible at the point of
  instantiation; downstream lessons should connect this to header-only
  template code and explicit instantiation.
- SFINAE is an overload/specialization selection rule, not a general "ignore all
  template errors" mechanism.
- Prefer concepts over SFINAE when C++20 is available and the project standard
  allows it. Keep SFINAE for pre-C++20 support and existing library patterns.
- Prefer `constexpr` functions for value computations. Reserve template
  metaprogramming for type-level work or cases where `constexpr` is not enough.
- Template-template parameters are useful for policy/template-family
  abstraction but can overcomplicate ordinary generic code.
- Dependent names require `typename` for dependent types and `template` for
  dependent member templates; this is a common source of confusing compiler
  errors.

## 6. Merged Concepts For Downstream Lesson

### Modern C++ Mental Model

- Modern C++ makes ownership, lifetime, type requirements, and compile-time
  intent explicit.
- The core teaching thread should be:
  - express ownership with RAII/smart pointers;
  - avoid unnecessary copies with move semantics;
  - express local behavior with lambdas;
  - express absence/alternatives/views with vocabulary types;
  - express generic algorithms with templates and concepts;
  - keep advanced compile-time machinery readable and justified.

### C Usage

- C equivalents are mostly manual or convention-based:
  - macros for generic-looking code;
  - function pointers plus `void*` context for callbacks;
  - tagged unions for variants;
  - nullable pointers and status codes for optional/error cases;
  - explicit allocation/free and cleanup labels for resource ownership;
  - hand-written type-specific functions instead of templates.
- The downstream C sections should compare mechanisms, not imply C is obsolete.
  C remains relevant for ABI, embedded, and POSIX APIs.

### C++ Usage

- Use RAII and smart pointers for owning resources.
- Use references, raw pointers, `span`, and `string_view` for non-owning access
  only when lifetime is obvious.
- Use lambdas for local callbacks and STL algorithms.
- Use templates for type-safe generic code.
- Use concepts or traits to document and enforce type requirements.
- Use `constexpr`/`consteval`/`constinit` to express compile-time intent where
  appropriate.

### Embedded Usage

- `constexpr`, `enum class`, `static_assert`, non-type template parameters, and
  fixed-size template buffers help encode hardware/protocol limits at compile
  time.
- Move-only RAII wrappers can own handles, buffers, or file descriptors in
  host-side embedded tools and Linux user-space components.
- Avoid unbounded dynamic allocation on constrained targets unless the project
  permits it.
- Prefer static polymorphism/templates only when code size and compile-time
  impact are acceptable.
- Lambdas are useful for callbacks, FSM actions, and test fakes, but stored or
  async callbacks need lifetime discipline.

### Enterprise Usage

- Prefer expressive vocabulary types and ownership-aware APIs:
  `unique_ptr` for transfer, references/raw pointers for borrowing,
  `optional` for no-value, `variant` for closed alternatives, `string_view` for
  borrowed text, `span` for borrowed contiguous buffers.
- Use concepts or named traits to improve diagnostics and API contracts.
- Avoid clever template metaprogramming in application code unless it materially
  reduces duplication or improves safety.
- Keep modern features tied to project standard level and compiler support.
- Add tests for lifetime, ownership transfer, move-only behavior, template
  constraints, and error paths.

## 7. Required Comparisons

| Comparison | Required teaching angle |
| --- | --- |
| `constexpr` vs `const` vs macro | `const` controls mutability, `constexpr` allows constant-expression use when requirements are met, macros are preprocessor substitution without type/scope safety |
| macro vs template | Macros substitute tokens and can duplicate evaluation; templates are typed, scoped, overload-aware compile-time code generation |
| macro vs `constexpr` vs template | Use `constexpr` for typed constants/functions, templates for type-generic code, macros only for conditional compilation or unavoidable preprocessor work |
| `std::optional` vs nullable pointer | `optional<T>` owns an optional value; nullable pointer represents optional access/ownership depending on API contract |
| `std::variant` vs union | `variant` tracks active alternative and manages object lifetimes; C/C++ unions require manual discipline unless restricted to trivial patterns |
| `std::string_view` vs `std::string` | `string` owns; `string_view` borrows. Prefer view for parameters, avoid storing/returning unless lifetime is guaranteed |
| move vs copy | Copy duplicates value/resource; move transfers resources and leaves source valid but unspecified |
| lambda vs function pointer vs `std::function` | Function pointer is stateless C-compatible callable; lambda is closure object and may capture; `std::function` is type-erased storage with possible overhead |
| template vs runtime polymorphism | Templates give compile-time polymorphism and zero virtual dispatch but can increase compile time/code size; runtime polymorphism supports dynamic substitution and ABI-stable interfaces |
| concepts vs SFINAE | Concepts are clearer C++20 constraints with better diagnostics; SFINAE supports older standards and existing detection idioms |
| CRTP vs virtual function | CRTP is static polymorphism with compile-time binding; virtual functions are runtime polymorphism with dynamic dispatch and stable interface substitution |
| template specialization vs overload | Prefer overloads for function behavior differences; use specialization/partial specialization for type families and trait customization |
| `std::move` vs `std::forward` | `std::move` unconditionally casts to xvalue; `std::forward<T>` conditionally preserves the original value category in forwarding references |
| `shared_ptr` vs `unique_ptr` vs `weak_ptr` | Exclusive ownership, shared ownership, and non-owning observation/breaking cycles respectively |

## 8. Common Bugs And Traps

- Dangling reference capture in a lambda stored beyond the referenced variable's
  lifetime.
- Capturing `this` in async callbacks after the object is destroyed.
- Assuming `[=]` makes all object state safe when member access may still depend
  on `this` behavior depending on standard and capture form.
- Using `std::function` for hot-path callables when a template parameter or
  generic lambda would avoid type-erasure overhead.
- Moving from an object and then reading assumptions about its old value.
- Moving a `const` object and expecting a real move.
- Writing `return std::move(local);` unnecessarily.
- Forgetting `noexcept` on move operations, causing containers to copy instead
  of move.
- Implementing Rule of Five manually when Rule of Zero would be simpler.
- Creating multiple independent `shared_ptr`s from the same raw pointer.
- Storing raw pointers returned by `get()` past owner lifetime.
- Leaking ownership after `unique_ptr::release()`.
- Creating `shared_ptr(this)` instead of using `enable_shared_from_this` when
  the object is already managed by `shared_ptr`.
- Creating reference cycles with `shared_ptr` and forgetting `weak_ptr`.
- Returning or storing `string_view` into a temporary `std::string`.
- Calling `optional::value()` without checking and handling
  `bad_optional_access`.
- Calling `std::get<T>` on a `variant` with the wrong active alternative.
- Using `auto` by value and accidentally copying expensive objects.
- Using `auto` where reference/constness is important but hidden.
- Treating templates as if they are duck-typed runtime code rather than
  compile-time instantiation with requirements.
- Putting template definitions only in `.cpp` files without explicit
  instantiation and causing linker errors.
- Assuming function templates can be partially specialized.
- Writing unconstrained templates that fail with unreadable diagnostics.
- Using SFINAE where concepts would be clearer in C++20 code.
- Forgetting `typename` or dependent `template` in advanced template code.
- Overusing template-template parameters or TMP where a normal type parameter,
  overload, or runtime interface is simpler.
- Creating excessive template instantiations that increase code size or build
  time.

## 9. Debugging Notes

- For lambda lifetime bugs, inspect capture lists first. Check whether a lambda
  is stored, returned, dispatched asynchronously, or copied into another owner.
- For move bugs, log or break on copy/move constructors and assignments in a
  small reproducer. Confirm whether `std::move`, RVO, or `noexcept` affects the
  selected path.
- For smart-pointer bugs, draw ownership graphs. Mark exclusive owners,
  shared owners, observers, and back-references.
- Use AddressSanitizer/LeakSanitizer for dangling pointers, double delete,
  leaks, and use-after-free in ownership examples.
- Use compiler warnings such as `-Wall -Wextra -Wpedantic -Wconversion` and
  clang-tidy checks for modernize/readability/performance warnings.
- For template errors, find the first user-written instantiation frame, then
  identify the missing operation/type requirement.
- Use `static_assert` with clear messages to narrow template errors before
  C++20 concepts are available.
- In C++20, prefer named concepts to convert long substitution traces into
  readable failed constraints.
- Use Compiler Explorer or small isolated test files to inspect template
  instantiation, overload selection, and code generation.
- For `string_view`/`span`, verify the owner outlives every view; if not,
  change the API to own (`string`, `vector`) or pass view only as a parameter.

## 10. Best Practices

- Prefer Rule of Zero. Let standard-library members manage resources whenever
  possible.
- Use `unique_ptr` by default for ownership. Use `shared_ptr` only when shared
  ownership is real and documented. Use `weak_ptr` for non-owning observation of
  shared objects and back-references.
- Prefer `make_unique` and `make_shared` except when custom deleters,
  allocation control, or advanced lifetime concerns require direct construction.
- Mark move constructors and move assignment operators `noexcept` when valid.
- Use `std::move` only when you are intentionally giving up the current value.
- Use `std::forward<T>` only in forwarding-reference wrappers.
- Prefer explicit lambda captures over broad `[=]` or `[&]` in production code.
- Capture by value or move for stored/asynchronous lambdas unless reference
  lifetime is proven.
- Use `std::function` for storing heterogeneous callables or runtime callback
  slots; use templates or `auto` callables for zero-overhead generic parameters.
- Use `constexpr` for compile-time values and functions that are still readable
  as normal C++.
- Use `consteval` when a function must be evaluated at compile time.
- Use `constinit` for static/thread storage variables that must avoid dynamic
  initialization surprises.
- Use `optional` for optional values, `variant` for closed alternatives, and
  `expected`/error types when failures need reasons.
- Use `string_view` and `span` mainly as non-owning parameter types with clear
  lifetime rules.
- Constrain templates close to the public API boundary.
- Prefer concepts in C++20+; prefer simple traits/SFINAE only for older standard
  compatibility or reusable detection traits.
- Prefer `constexpr` functions over TMP for value computations.
- Reserve advanced TMP, CRTP, expression templates, and policy-based design for
  cases where they clearly improve performance, correctness, or API clarity.

## 11. Interview Angles

### Junior

- What is Modern C++?
- Why prefer `nullptr` over `NULL` or `0`?
- What problem does `auto` solve, and when can it hurt readability?
- What is a lambda expression?
- Capture by value vs capture by reference.
- What is a smart pointer?
- `unique_ptr` vs `shared_ptr`.
- What does `std::move` do?
- What is a template?
- Why are templates safer than macros?

### Middle

- Explain lvalue, rvalue, and rvalue reference.
- Why should move operations often be `noexcept`?
- What is a moved-from object allowed to do?
- When should you use `weak_ptr`?
- Why is `string_view` dangerous when returned or stored?
- `optional` vs nullable pointer.
- `variant` vs union.
- Lambda vs function pointer vs `std::function`.
- Template instantiation and type deduction.
- Function template overloading vs specialization.
- Variadic templates and fold expressions.
- What is SFINAE, and why is it hard to read?
- What are concepts, and why are they better for template constraints?

### Senior

- Design an ownership-aware plugin/callback API using lambdas and smart
  pointers without cycles.
- Explain `std::move` vs `std::forward` and forwarding references.
- Diagnose a template error from an unconstrained generic function.
- Explain two-phase lookup and dependent names.
- Explain code-size/build-time tradeoffs of templates.
- When would you choose runtime polymorphism over templates?
- When would CRTP be appropriate, and when would it be overengineering?
- How would you migrate a SFINAE-heavy API to C++20 concepts while preserving
  backward compatibility?
- How do `constexpr`, `consteval`, and `constinit` differ?
- How do you design a `string_view` or `span` API that cannot dangle in normal
  use?

## 12. Practice Targets For Later Outputs

- Refactor a macro `MAX` and macro constant into a function template and
  `constexpr` value.
- Write a small `FixedBuffer<T, N>` with a non-type template parameter and
  `static_assert`.
- Implement a move-only RAII wrapper for a simple C handle or file pointer.
- Demonstrate copy vs move behavior with logging constructors and `std::vector`
  reallocation.
- Build a callback registry using lambdas; show safe value capture and unsafe
  reference capture.
- Compare `std::function<void(int)>` callback storage with a templated callback
  parameter.
- Write a parser function returning `std::optional<int>` and then extend to an
  error-carrying result.
- Use `std::variant` plus `std::visit` for a small command/event type.
- Implement a `print_all` variadic function first with recursion, then with a
  C++17 fold expression.
- Write a trait/concept that detects `begin()`, `end()`, and `size()`.
- Convert an `enable_if` overload pair into concepts.
- Create examples that intentionally fail with clear `static_assert` or concept
  diagnostics.

## 13. Gaps And External Validation Needs

- Validate exact standard-version availability for `std::span`,
  `std::expected`, ranges, modules, spaceship operator, `consteval`, and
  `constinit` during downstream lesson writing.
- Validate non-type template parameter rules by standard version before giving
  exact allowed-type lists.
- Validate `constexpr` evolution by standard version, especially C++14 relaxed
  rules and C++20 dynamic allocation/exception limitations.
- Validate `shared_ptr` array support and `make_shared` array overload wording
  before teaching arrays with smart pointers.
- Validate CTAD/deduction guide examples against the intended compiler standard.
- `std::expected` is in C++23; downstream examples should gate it or present it
  as modern awareness rather than baseline C++17.
- `std::span` is C++20; if the course target is C++17, present alternatives or
  use awareness-only coverage.
- Ranges and modules should remain awareness-level unless the downstream topic
  explicitly asks for C++20+ deep coverage.

## 14. Suggested Next Outputs

- `knowledge/12-modern-cpp-and-templates.md`
  - Full lesson.
  - Deep coverage for Modern C++ essentials and template basics.
  - Medium coverage for C++20 vocabulary and constraints.
  - Controlled awareness for advanced TMP, ranges, modules, coroutines, CRTP,
    and expression templates.
- `interview/12-modern-cpp-and-templates.md`
  - Beginner, middle, and senior questions.
  - Include debugging scenarios for dangling lambda captures, moved-from
    objects, ownership cycles, template substitution errors, and
    `string_view` lifetime.
- `examples/12-modern-cpp-and-templates/README.md`
  - Compile-ready examples for move logging, lambda captures, smart-pointer
    ownership, `optional`/`variant`, `string_view` lifetime, variadic folds,
    and concepts vs SFINAE.

## 15. Quality Gate For Downstream Work

- Use priority depth from `master-ch12` and `master-ch13`.
- Include all required comparisons from section 7 of this brief.
- Keep exact standard/version behavior precise and cite cppreference where
  needed.
- Connect concept -> mechanism -> code/API -> practical use -> bugs/debugging
  -> best practices -> interview readiness.
- Keep C-only and POSIX/Linux comparisons scoped to language/API contrast; do
  not import Linux Device Driver or kernel-driver material.
- Do not paste this audit metadata into learner-facing knowledge or interview
  files.
