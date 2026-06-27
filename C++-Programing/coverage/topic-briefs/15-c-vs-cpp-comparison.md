# Topic Brief 15 - C Vs C++ Comparison

## 1. Canonical Routing

| Field | Validated value |
| --- | --- |
| `learning_path_number` | `15` |
| Title | C Vs C++ Comparison |
| `slug` | `c-vs-cpp-comparison` |
| Requested topic | Practical comparison of C and C++ language mechanisms, idioms, safety tradeoffs, ownership models, data modeling, callbacks, memory management, arrays/strings, OOP-style design, and error handling |
| Master source | `master-ch17` |
| Required Notion sources | Related Notion files routed to topic `15`: `notion-1-3`, `notion-2-2`, `notion-2-4`, `notion-2-6`, `notion-3-1`, `notion-3-2`, `notion-3-3`, `notion-3-4`, `notion-3-5`, `notion-3-6`, `notion-4-1`, `notion-6-1`, `notion-6-2`, `notion-8-1`, `notion-10-2`, `notion-10-3`, `notion-10-4`, `notion-10-5` |
| Topic Brief | `coverage/topic-briefs/15-c-vs-cpp-comparison.md` |
| Knowledge target | `knowledge/15-c-vs-cpp-comparison.md` |
| Interview target | `interview/15-c-vs-cpp-comparison.md` |
| Example target | `examples/15-c-vs-cpp-comparison/README.md` |

Validation result: the number, title, slug, master source, output paths, and
source-routing phrase match `LEARNING_PATH.md`. The concrete Notion source list
above is derived from the `LEARNING_PATH.md` Notion Source Inventory rows whose
`Routed Topics` include `15`.

This step creates the Topic Brief only. It does not create or modify knowledge,
interview, or example outputs.

## 2. Source Coverage

### Internal Sources Read

| Source label | Path | Coverage contribution |
| --- | --- | --- |
| `master-ch17` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, CH17 | MUST priority, CH05/CH08/CH10 prerequisites, required comparison matrix, and compact table format rule |
| `guide-section-03` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 3 | MUST-topic deep output requirements |
| `guide-section-04` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 4 | Full chapter structure for later learner-facing output |
| `guide-section-06` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 6 | Required C vs C++ comparison format and required comparison groups |
| `guide-section-07` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 7 | Depth control and practical-example preference |
| `notion-index` | `docs/C++ Notion/C++ Programming.md` | Notion chapter inventory and identity validation |
| `notion-1-3` | `docs/C++ Notion/Chapter 1-3 Type Conversion & Casting.md` | C-style casts vs C++ named casts, implicit/explicit conversion, narrowing, `static_cast`, `dynamic_cast`, `const_cast`, and `reinterpret_cast` |
| `notion-2-2` | `docs/C++ Notion/Chapter 2-2 Parameter Passing Techniques.md` | Pass by value/reference/pointer, pointer vs reference, array decay, C-style variadic functions, variadic templates, perfect forwarding |
| `notion-2-4` | `docs/C++ Notion/Chapter 2-4 Inline Functions.md` | Inline function vs macro, type safety, side effects, scoping, debugging, ODR, inline variables, `constexpr` functions |
| `notion-2-6` | `docs/C++ Notion/Chapter 2-6 Lambda Expressions (C++11).md` | Lambda syntax, captures, generic lambdas, STL use, lambda vs function pointer/functor/`std::function`, capture lifetime bugs |
| `notion-3-1` | `docs/C++ Notion/Chapter 3-1 Arrays in C++.md` | C-style arrays, array decay, pointer arithmetic, C strings, `std::array`, returning arrays, common array bugs |
| `notion-3-2` | `docs/C++ Notion/Chapter 3-2 Pointers in C++.md` | Raw pointers, pointer arithmetic, `void*`, dangling/wild/null pointers, function pointers, pointer vs reference, smart pointer overview |
| `notion-3-3` | `docs/C++ Notion/Chapter 3-3 References in C+.md` | References as aliases, pointer vs reference, const references, returning references, lvalue/rvalue references, move/forward awareness |
| `notion-3-4` | `docs/C++ Notion/Chapter 3-4 Strings in C++.md` | C strings vs `std::string`, null terminators, `cstring` hazards, `c_str()`, `string_view`, string conversions, parsing |
| `notion-3-5` | `docs/C++ Notion/Chapter 3-5 Structures in C++.md` | C-style struct data modeling, C++ structs with constructors/member functions/access, struct vs class, nested structs, arrays/vectors of structs |
| `notion-3-6` | `docs/C++ Notion/Chapter 3-6 Unions, Enumerations, and Type Aliases in C+.md` | C/C++ unions, enum vs `enum class`, type aliases, `typedef` vs `using`, `std::variant` as safer tagged alternative |
| `notion-4-1` | `docs/C++ Notion/Chapter 4-1 Dynamic Memory Basics.md` | Stack vs heap, `malloc/calloc/realloc/free`, `new/delete`, `new[]/delete[]`, allocation failure, `bad_alloc`, `nothrow`, C vs C++ allocation styles |
| `notion-6-1` | `docs/C++ Notion/Chapter 6-1 STL Introduction & vector Container.md` | STL motivation, `std::vector` vs manual dynamic arrays, size vs capacity, `reserve` vs `resize`, iterator invalidation, dynamic array performance |
| `notion-6-2` | `docs/C++ Notion/Chapter 6-2 Sequence Containers deque, list, forward_list, array.md` | `std::array` vs C array vs `std::vector`, container selection, contiguous vs linked storage, iterator stability and invalidation |
| `notion-8-1` | `docs/C++ Notion/Chapter 8-1 Exception Handling - Basics & Standard Exception.md` | Return codes vs exceptions, throw/catch, standard exceptions, custom exceptions, catch ordering, rethrowing |
| `notion-10-2` | `docs/C++ Notion/Chapter 10-2 Preprocessor Directives.md` | Macros, include guards, conditional compilation, macro pitfalls, macro alternatives, compile-time constants |
| `notion-10-3` | `docs/C++ Notion/Chapter 10-3 Type Casting.md` | C-style cast risks, C++ named casts, cast-selection rules, unsafe reinterpretation, const removal hazards |
| `notion-10-4` | `docs/C++ Notion/Chapter 10-4 Smart Pointers.md` | RAII, `unique_ptr`, `shared_ptr`, `weak_ptr`, `make_unique`, `make_shared`, custom deleters, raw pointer ownership pitfalls |
| `notion-10-5` | `docs/C++ Notion/Chapter 10-5 Callbacks.md` | Function pointer callbacks, functors, lambdas, `std::function`, `std::bind`, member callbacks, observer/event callback patterns and lifetime traps |

All eighteen routed Notion chapter files were inspected. No mapped Notion source
was skipped.

### External References Consulted

Accessed on 2026-06-27.

| Label | Reference | Validation purpose |
| --- | --- | --- |
| `external-cppreference-c` | cppreference C reference: <https://cppreference.com/c> | C language/library index for core concepts, dynamic memory, null-terminated strings, preprocessor, and C standard-version context |
| `external-cppreference-c-language` | cppreference C language: <https://cppreference.com/c/language> | C core language constructs for exact terminology |
| `external-cppreference-cpp-language` | cppreference C++ language: <https://cppreference.com/cpp/language> | C++ core language constructs, classes, constructors/destructors, templates, and standard-version context |
| `external-iso-c` | ISO/IEC 9899:2024 C standard page: <https://www.iso.org/standard/82075.html> | Current official C standard identity and scope |
| `external-iso-cpp` | Standard C++ "The Standard": <https://isocpp.org/std/the-standard> | Current official C++ standard identity and relationship to C++23 / ISO/IEC 14882:2024 |
| `external-core-guidelines` | C++ Core Guidelines: <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines> | Guideline-level validation for interfaces, resource management, memory safety, C-style programming migration, and modern C++ idioms |

### Source Coverage Status

`TOPIC_BRIEF_COMPLETE_WITH_EXTERNAL_VALIDATION`: canonical routing, derived
mapped Notion source list, master comparison matrix, guide comparison rules,
all routed Notion chapter files, external standard/reference/guideline sources,
merged concepts, required comparisons, common bugs, debugging notes, best
practices, interview angles, gaps, and output targets are recorded.

## 3. Priority And Dependencies

- Overall priority: `MUST`.
- Required depth: deep for every listed comparison pair.
- Master prerequisites:
  - CH05, Compound Types In C, for arrays, structs, unions, enums, and type
    aliases.
  - CH08, C++ Fundamentals, for references, overloads, lambdas, namespaces,
    standard library basics, and modern C++ syntax.
  - CH10, Resource Management In C++, for RAII, smart pointers, ownership,
    stack/heap lifetime, and exception safety.
- Practical prerequisites:
  - Pointer lifetime, array decay, null-terminated strings, and manual memory.
  - Basic classes/constructors/destructors.
  - Function pointers and callbacks.
  - Exception and return-code error handling.
  - Standard containers, especially `std::array`, `std::vector`, and
    `std::string`.

## 4. Scope And Depth Boundaries

### Deep In This Topic

- Data modeling:
  - C `struct` vs C++ `struct`.
  - C++ `struct` vs C++ `class`.
  - C `union` vs C++ `union` vs `std::variant`.
  - C `enum` vs C++ `enum class`.
  - `typedef` vs `using`.
- Memory and ownership:
  - `malloc` vs `calloc`.
  - `malloc/calloc/realloc/free` vs `new/delete`.
  - `free` vs `delete`.
  - `new[]/delete[]` vs `std::vector`.
  - Manual cleanup vs RAII.
  - Raw pointer vs smart pointer.
  - Stack object vs heap object.
  - Shallow copy vs deep copy.
- Strings and arrays:
  - C string vs `std::string`.
  - `char*` vs `std::string_view`.
  - C array vs `std::array`.
  - C dynamic array vs `std::vector`.
- Functions and callbacks:
  - Function pointer vs lambda.
  - Function pointer vs `std::function`.
  - C callback vs C++ observer/callback object.
  - Macro vs inline function.
  - Macro vs `constexpr`.
  - Macro vs template.
- OOP/design:
  - OOP in C vs OOP in C++.
  - Function pointer table vs virtual function.
  - Ops table vs interface.
  - Inheritance vs composition.
  - Virtual dispatch vs static polymorphism.
- Error handling:
  - Return code vs exception.
  - `errno` vs exception.
  - `assert` vs exception.
  - Exception vs `std::expected` / Result type.

### Medium In This Topic

- Cast comparisons: C-style cast vs C++ named casts.
- Pass-by-pointer vs pass-by-reference.
- C variadic functions vs variadic templates.
- `std::vector` vs `std::deque` / `std::list` only when dynamic-array
  tradeoffs need context.
- `std::bind` vs lambda.
- `std::shared_ptr` vs `std::unique_ptr` only as ownership comparison support.

### Controlled Awareness

- ABI and name-mangling consequences across C and C++ boundaries.
- Exact ISO wording for compatibility and object lifetime.
- C23 features that reduce some historical C/C++ gaps, such as `nullptr`, only
  as standard-version notes.
- Performance microdetails of inlining, virtual dispatch, `std::function`,
  and allocator behavior.

### Defer Or Exclude

- Full POSIX/Linux API comparison: route to topic 16.
- Full concurrency comparison: route to topic 14.
- Full design-pattern lesson: route to topic 17.
- Full enterprise review checklist: route to topic 18.
- Linux Device Driver, kernel-driver, Yocto, GStreamer, AUTOSAR, or unrelated
  platform material is excluded.

## 5. Merged Concept Map

- C is explicit and procedural: data and functions are usually separate, memory
  ownership is manual, callbacks are function pointers plus context pointers,
  and errors are usually return/status codes.
- C++ can express the same low-level operations but adds stronger abstraction:
  constructors/destructors, references, overloads, RAII, templates, exceptions,
  standard containers, smart pointers, lambdas, and type-safe enums.
- C++ is not automatically safer. It becomes safer when code uses RAII, scoped
  ownership, standard containers, `std::string`, `std::array`, `std::vector`,
  `enum class`, named casts, and clear interfaces.
- C style is still valuable for ABI boundaries, embedded C projects, freestanding
  environments, vendor SDKs, hardware-adjacent code, and stable plugin APIs.
- The key design question is not "C bad, C++ good"; it is "which mechanism makes
  ownership, lifetime, invariants, error handling, and ABI constraints clearest?"
- Most migration problems come from mixing models without a policy:
  - `malloc` paired with `delete`.
  - `new` paired with `free`.
  - raw owning pointers mixed with smart pointers.
  - C callbacks storing dangling C++ captures.
  - C strings passed as if they were bounded strings.
  - exceptions crossing C ABI boundaries.
- C++ comparison output should always state:
  - when C style is appropriate;
  - when C++ style is appropriate;
  - the common bug;
  - an interview-quality answer.

## 6. Required Comparisons To Preserve

Every downstream lesson/interview answer should use this compact table shape
when expanding a pair:

```md
| Topic | C | C++ | Enterprise / Embedded Guidance |
| --- | --- | --- | --- |
```

### Data Modeling

| Pair | C side | C++ side | Guidance |
| --- | --- | --- | --- |
| `struct` in C vs `struct` in C++ | Plain aggregate data model; behavior usually separate functions | Can contain constructors, member functions, access control, operators, templates | Use C `struct` for ABI/plain data. Use C++ class/struct when invariants and behavior belong with data |
| `struct` vs `class` in C++ | Not applicable in C | Same except default public for `struct`, default private for `class` | Use `struct` for passive data/value objects; `class` for invariants and encapsulation |
| `union` in C vs C++ vs `std::variant` | Manual tagged union pattern; active member discipline is on programmer | C++ unions have object lifetime concerns; `std::variant` tracks active alternative | Prefer `variant` for type-safe alternatives. Use union only for ABI, embedded layout, or low-level storage |
| `enum` vs `enum class` | Unscoped names and implicit integer conversions | Scoped, strongly typed enumerators | Prefer `enum class` in C++. Use C enum for C ABI or C code |
| `typedef` vs `using` | Traditional alias mechanism | `using` is clearer and supports alias templates | Prefer `using` in C++; use `typedef` in C or C ABI headers |

### Memory And Ownership

| Pair | C side | C++ side | Guidance |
| --- | --- | --- | --- |
| `malloc` vs `calloc` | `malloc` allocates uninitialized bytes; `calloc` zero-initializes array storage | Prefer constructors/containers for objects | Use C allocation only for C APIs/raw storage. Do not treat zero bytes as constructed C++ objects |
| `malloc/calloc/realloc/free` vs `new/delete` | Raw byte allocation and manual release; no constructors/destructors | Object allocation initializes and destroys objects | Never mix families. Prefer RAII containers/smart pointers over both in C++ |
| `free` vs `delete` | Releases C allocation | Destroys C++ object then deallocates | Pair exactly: `malloc/free`, `new/delete`, `new[]/delete[]` |
| `new[]/delete[]` vs `std::vector` | Manual dynamic array lifetime and bounds discipline | Dynamic container with size, capacity, destructor, iterators | Prefer `vector`; use raw arrays only for ABI or carefully bounded low-level code |
| Manual cleanup vs RAII | Cleanup branches at every failure path | Destructor owns cleanup | Prefer RAII for C++ resources: memory, files, locks, handles |
| Raw pointer vs smart pointer | Pointer may represent observe, optional, ownership, array, or C handle | `unique_ptr`/`shared_ptr`/`weak_ptr` encode ownership | Raw pointer should usually mean non-owning. Use smart pointers for ownership |
| Stack object vs heap object | C stack objects and manual heap pointers | Prefer scoped objects; heap when lifetime/size/polymorphism requires | Do not heap-allocate just because "object is large" without measuring or understanding lifetime |
| Shallow copy vs deep copy | Struct copy copies pointer values, not pointed-to allocations | RAII types define copy/move ownership semantics | Rule of Zero/Five prevents double-free and leaks |

### String And Array

| Pair | C side | C++ side | Guidance |
| --- | --- | --- | --- |
| C string vs `std::string` | Null-terminated `char` sequence; caller manages buffer size | Owns size/capacity and manages memory | Prefer `std::string` internally. Convert at C API boundaries |
| `char*` vs `std::string_view` | Mutable/non-mutable pointer may lack size and ownership clarity | Non-owning view with pointer + length | Use `string_view` for read-only non-owning parameters, but avoid dangling views |
| C array vs `std::array` | Fixed array decays to pointer in function parameters | Fixed-size wrapper that keeps size and STL interface | Prefer `std::array` when size is compile-time and C ABI is not required |
| C dynamic array vs `std::vector` | Pointer + separate size/capacity discipline | Owns dynamic storage and tracks size/capacity | Prefer `vector` for dynamic arrays; use `data()` for C interop |

### Function And Callback

| Pair | C side | C++ side | Guidance |
| --- | --- | --- | --- |
| Function pointer vs lambda | Function address only; no captured state unless separate context pointer | Lambda can capture state; non-capturing lambda can convert to function pointer | Use function pointer for C callbacks. Use lambda for local C++ behavior, with lifetime care |
| Function pointer vs `std::function` | Lightweight but fixed to functions with exact signature | Type-erased callable wrapper stores functions, lambdas, functors | Use templates for hot paths, `std::function` for stored callbacks, function pointer for ABI |
| C callback vs C++ observer/callback object | Usually function pointer plus `void* user_data` | Object/lambda/observer interface can own state and encode lifetime | Avoid dangling captures and callbacks under locks |
| Macro vs inline function | Text substitution, no type checking, multiple evaluation risk | Scoped, typed, debuggable function; compiler may inline | Prefer inline functions for expression-like code |
| Macro vs `constexpr` | Preprocessor constants/text before type checking | Typed compile-time values/functions | Prefer `constexpr` for constants and compile-time computation |
| Macro vs template | Text generation | Type-checked generic code | Prefer templates for generic functions/types; keep macros for conditional compilation |

### OOP And Design

| Pair | C side | C++ side | Guidance |
| --- | --- | --- | --- |
| OOP in C vs OOP in C++ | Struct + function pointer table + manual convention | Classes, constructors, destructors, access control, virtual functions | Use C-style OOP for C ABI/plugin systems; use C++ classes for invariants and ownership |
| Function pointer table vs virtual function | Manual dispatch table | Built-in dynamic dispatch through vtable | Virtuals are clearer in C++ when runtime polymorphism is intended |
| Ops table vs interface | Struct of operations and context pointer | Abstract base class or concept/template interface | Use ops table for C ABI; use interface/concept inside C++ |
| Inheritance vs composition | C usually composes structs manually | C++ supports both inheritance and composition | Prefer composition unless substitutability/polymorphism is truly needed |
| Virtual dispatch vs static polymorphism | Function pointer call | Virtual runtime dispatch or template compile-time dispatch | Use virtual for runtime polymorphism; templates/concepts for compile-time performance and type safety |

### Error Handling

| Pair | C side | C++ side | Guidance |
| --- | --- | --- | --- |
| Return code vs exception | Explicit status checked by every caller | Automatic propagation with typed handlers | Use return/result for expected/local failures and C ABI; exceptions for exceptional failures when project policy allows |
| `errno` vs exception | Thread-local-style diagnostic for selected C/POSIX APIs after failure | Exception object carries type/context | Read `errno` only when API documents it. Preserve context when translating |
| `assert` vs exception | Debug-time invariant check, may disappear under `NDEBUG` | Runtime error channel for recoverable failures | Do not use `assert` for user input or operational failure |
| Exception vs `std::expected` / Result | C has project-specific status structs/enums | Exceptions are implicit propagation; expected/result is explicit value-or-error | Prefer explicit result for expected frequent failures; exceptions for rare cross-layer failures |

## 7. Common Bugs And Corrections

- Mixing allocation families.
  Correction: pair `malloc/free`, `new/delete`, and `new[]/delete[]` exactly;
  prefer RAII wrappers in C++.
- Treating `malloc` memory as a constructed C++ object.
  Correction: use constructors through `new`, containers, or placement-new only
  with explicit lifetime management.
- Losing array size through decay.
  Correction: pass size, use array reference templates, `std::span`, `std::array`,
  or `std::vector`.
- Returning pointer/reference to local storage.
  Correction: return by value, return owning object, or ensure referenced object
  outlives the caller.
- Buffer overflow and missing null terminator in C strings.
  Correction: track capacity, use bounded functions carefully, or prefer
  `std::string`/`string_view` for C++ interfaces.
- Storing `std::string_view` to a temporary or destroyed string.
  Correction: use `string_view` only when the viewed data lifetime is guaranteed.
- Using macro expressions with side effects.
  Correction: replace with inline functions, `constexpr`, or templates.
- Using C-style casts that hide `const_cast`/`reinterpret_cast` danger.
  Correction: use the narrowest named cast and review each cast.
- Creating multiple `shared_ptr` objects from the same raw pointer.
  Correction: create once with `make_shared` or transfer ownership clearly.
- Using `shared_ptr` where `unique_ptr` ownership is sufficient.
  Correction: default to `unique_ptr`; share only when multiple owners are real.
- Capturing references in callbacks that outlive the captured object.
  Correction: capture by value, use ownership-aware handles, or disconnect
  observers before destruction.
- Throwing exceptions across C ABI boundaries.
  Correction: catch at the boundary and translate to C status/error code.
- Using `assert` as runtime error handling.
  Correction: use return code, result type, or exception for recoverable errors.

## 8. Debugging Notes

- Memory bugs:
  - AddressSanitizer for use-after-free, double delete/free, buffer overflow,
    and stack lifetime bugs.
  - LeakSanitizer or ASan leak mode for ownership leaks.
  - Valgrind-like tools where sanitizers are unavailable.
- Undefined behavior:
  - UBSan for invalid casts, overflow-sensitive code, misalignment, and other UB.
  - Compiler warnings: `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`.
- Allocation mismatch:
  - Search for `malloc`, `calloc`, `realloc`, `free`, `new`, `delete`, and check
    family pairing and ownership transfer.
- Macro bugs:
  - Preprocess with `-E` to inspect macro expansion.
  - Replace expression macros with inline/`constexpr` where possible.
- Container/array bugs:
  - Check array decay, size/capacity confusion, `reserve` vs `resize`, and
    iterator invalidation after `vector` reallocation.
- Callback bugs:
  - Trace callback registration/unregistration, capture lifetimes, thread
    context, and whether callbacks run under locks.
- C/C++ boundary bugs:
  - Verify `extern "C"` where needed, avoid exceptions across C ABI, document
    ownership of pointers, buffers, and returned resources.

## 9. Best Practices

- Prefer C++ standard library types internally: `std::string`, `std::vector`,
  `std::array`, smart pointers, RAII wrappers, lambdas, and named casts.
- Use C style deliberately at C ABI boundaries, embedded C codebases, vendor SDK
  interfaces, or when deterministic low-level layout/control is required.
- Document ownership at every boundary: who allocates, who frees, with which
  function, and whether the pointer may be null.
- Default to stack/scoped objects in C++; heap-allocate only for lifetime,
  dynamic size, polymorphism, or ownership transfer reasons.
- Prefer `using` over `typedef` in C++.
- Prefer `enum class` over unscoped enums in C++.
- Prefer `std::variant` over manual unions when ABI/layout constraints do not
  require a union.
- Prefer `std::array` for fixed-size arrays and `std::vector` for dynamic arrays.
- Prefer `std::string` for owning strings and `std::string_view` for temporary
  read-only views with known lifetime.
- Prefer `constexpr`, inline functions, and templates over expression macros.
- Prefer lambdas/templates for C++ callbacks, `std::function` for stored
  callbacks, and function pointer plus context for C ABI callbacks.
- Keep exceptions inside C++ boundaries; translate at C boundaries.
- Keep comparison content balanced: explain when C style is still correct rather
  than presenting C++ as a universal replacement.

## 10. Interview Angles

- Explain why C++ is not "C with classes" in production design.
- Compare C `struct` and C++ `struct`; compare C++ `struct` and `class`.
- Explain why `enum class` is safer than C-style/unscoped enums.
- Explain why `std::variant` is safer than a manual tagged union.
- Compare `malloc/free` and `new/delete`.
- Why must `delete[]` match `new[]`?
- What is RAII and why does it solve manual cleanup bugs?
- Raw pointer vs `unique_ptr` vs `shared_ptr` vs `weak_ptr`.
- Stack object vs heap object.
- Shallow copy vs deep copy and Rule of Zero/Five.
- C array vs `std::array`; C dynamic array vs `std::vector`.
- Explain array decay and why it loses size.
- C string vs `std::string`; when is `string_view` dangerous?
- Function pointer vs lambda vs `std::function`.
- C callback with `void* user_data` vs C++ observer object.
- Macro vs inline function; macro vs `constexpr`; macro vs template.
- C-style cast vs C++ named casts.
- OOP in C using function tables vs C++ virtual functions/interfaces.
- Inheritance vs composition.
- Runtime polymorphism vs static polymorphism.
- Return code/`errno` vs exceptions vs `std::expected`/Result.
- How to design a C API wrapper in C++ without leaking ownership or exceptions.

## 11. Practice Targets

- Convert a C dynamic array (`malloc` + size + capacity + `realloc`) into a
  `std::vector` version and list which bugs disappear.
- Rewrite a C string buffer parser using `std::string_view` for parsing and
  `std::string` for ownership; identify dangling-view risks.
- Replace a macro like `SQUARE(x)` with an inline/`constexpr` function and show
  how side effects change.
- Convert a function-pointer callback API into:
  - C style: callback + `void* context`;
  - C++ style: lambda/template callback;
  - stored C++ style: `std::function`.
- Compare a manual tagged union implementation with `std::variant`.
- Wrap a C resource handle in a C++ RAII class with a custom deleter.
- Translate a C return-code API to a C++ wrapper that either returns
  `std::expected`/Result or throws, depending on policy.
- Audit a mixed C/C++ file for allocation-family mismatch and ownership
  ambiguity.

## 12. Gaps And External Validation Needs

- Exact ISO compatibility wording between C and C++ should be validated against
  ISO C and ISO C++ references when later output makes normative claims.
- C23 and C++23/26 standard-version differences should be treated carefully;
  avoid implying every compiler supports the newest feature set.
- `std::expected` is C++23; downstream examples must mark version requirements
  or use a local Result type for older standards.
- `std::string_view` is C++17 and non-owning; examples must avoid storing views
  to temporaries.
- Some Notion material teaches raw `new`/`delete` for learning. Downstream C++
  guidance should present it as historical/low-level knowledge and prefer RAII,
  containers, and smart pointers.
- Some Notion material uses broad "always" style recommendations. Downstream
  content should qualify them for C ABI, embedded constraints, freestanding
  environments, and performance-critical boundaries.
- No Linux Device Driver/kernel-driver material is needed or allowed for this
  topic brief.

## 13. Suggested Output Targets

- `knowledge/15-c-vs-cpp-comparison.md`
  - Teach by comparison groups: goal, why this matters, mental model, data
    modeling, memory/ownership, strings/arrays, functions/callbacks,
    macros/templates, OOP/design, error handling, C ABI boundaries, common bugs,
    debugging, best practices, interview readiness, and practice.
  - Use the required compact comparison table format for each pair.
  - Preserve English technical keywords.
  - Keep source coverage/audit metadata out.
- `interview/15-c-vs-cpp-comparison.md`
  - Include beginner, mid-level, and senior questions.
  - Each answer should contain short answer, deep explanation, C/C++ code/API
    anchor, production/debug angle, traps, and follow-ups.
  - Include coding tasks for allocation mismatch, macro replacement, callback
    conversion, RAII wrapper, and C API error translation.
- `examples/15-c-vs-cpp-comparison/README.md`
  - Include compact compile-ready examples only if useful:
    - macro vs inline/`constexpr`;
    - `malloc/free` vs RAII wrapper;
    - C array vs `std::array`/`std::vector`;
    - function pointer callback vs lambda/`std::function`;
    - tagged union vs `std::variant`.
  - Add sanitizer/debug commands for memory and UB demonstrations.
  - Mark learning-only unsafe examples clearly.

