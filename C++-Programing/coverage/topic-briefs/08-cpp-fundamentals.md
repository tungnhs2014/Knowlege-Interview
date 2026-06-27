# Topic Brief 08 - C++ Fundamentals

## 1. Canonical Routing

| Field | Validated value |
| --- | --- |
| `learning_path_number` | `08` |
| Title | C++ Fundamentals |
| `slug` | `cpp-fundamentals` |
| Master source | `master-ch08` |
| Required Notion sources | `notion-1-1`, `notion-1-2`, `notion-1-3`, `notion-1-4`, `notion-2-1`, `notion-2-2`, `notion-2-3`, `notion-2-4`, `notion-2-5`, `notion-2-6`, `notion-10-1`, `notion-10-10` |
| Topic Brief | `coverage/topic-briefs/08-cpp-fundamentals.md` |
| Knowledge target | `knowledge/08-cpp-fundamentals.md` |
| Interview target | `interview/08-cpp-fundamentals.md` |
| Example target | `examples/08-cpp-fundamentals/README.md` |

Validation result: the number, title, slug, master source, twelve mapped Notion
sources, and all canonical output paths match `LEARNING_PATH.md`.

This step creates the Topic Brief only. It does not create or modify knowledge,
interview, or example outputs.

## 2. Source Coverage

### Internal Sources Read

| Source label | Path | Coverage contribution |
| --- | --- | --- |
| `master-ch08` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, CH08 | MUST priority, deep depth, CH05 prerequisite, keyword scope, comparisons, C contrast rule, object-lifetime emphasis, and interview focus |
| `guide-section-03` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 3 | Deep treatment for MUST concepts and controlled medium/brief treatment for supporting concepts |
| `guide-section-04` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 4 | Full-chapter structure and required C, C++, embedded, enterprise, bug, debug, interview, practice, and reference angles |
| `guide-section-06` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 6 | Required C versus C++ comparison format |
| `guide-section-07` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 7 | Practical-example selection and depth control |
| `guide-section-09` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 9 | Trusted routing for exact C++ language and engineering guidance |
| `guide-section-10` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 10 | Clear technical English, concise Markdown, compile-oriented examples, and risk warnings |
| `guide-section-11` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 11 | Topic Brief, full lesson, interview pack, examples, and review expectations |
| `guide-section-14` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 14 | Final quality checklist |
| `notion-index` | `docs/C++ Notion/C++ Programming.md` | Notion inventory and chapter identity validation |
| `notion-1-1` | `docs/C++ Notion/Chapter 1-1 Introduction & Environment Setup.md` | C++ purpose, multi-paradigm model, first program, translation stages, compiler flags, and development environment |
| `notion-1-2` | `docs/C++ Notion/Chapter 1-2 Variables, Data Types, Storage & Scope.md` | Initialization, fundamental types, `const`, `constexpr`, scope, storage duration, linkage-related vocabulary, shadowing, and type properties |
| `notion-1-3` | `docs/C++ Notion/Chapter 1-3 Type Conversion & Casting.md` | Implicit conversions, narrowing, C++ cast syntax, usual arithmetic conversions, and cast risks |
| `notion-1-4` | `docs/C++ Notion/Chapter 1-4 Operators, Input Output, Control Flow & Loops.md` | Operators, short-circuiting, stream I/O, branches, loops, range-for, and control-flow bugs |
| `notion-2-1` | `docs/C++ Notion/Chapter 2-1 Function Basics.md` | Declarations, definitions, return values, calls, scope, lifetime, recursion risk, and implementation-oriented call-stack model |
| `notion-2-2` | `docs/C++ Notion/Chapter 2-2 Parameter Passing Techniques.md` | Value, reference, pointer, `const` parameters, defaults, array decay, C varargs, variadic templates, and forwarding |
| `notion-2-3` | `docs/C++ Notion/Chapter 2-3 Function Overloading & Name Mangling.md` | Overload sets, viability, ranking, ambiguity, ABI symbol representation, C language linkage, and mixed C/C++ headers |
| `notion-2-4` | `docs/C++ Notion/Chapter 2-4 Inline Functions.md` | `inline`, macros, ODR concerns, class-body definitions, header definitions, `constexpr`, and inline variables |
| `notion-2-5` | `docs/C++ Notion/Chapter 2-5 Recursion.md` | Base cases, recursive depth, recursion versus iteration, tail calls, stack risk, and algorithm examples |
| `notion-2-6` | `docs/C++ Notion/Chapter 2-6 Lambda Expressions (C++11).md` | Closure objects, captures, `mutable`, init-capture, generic lambdas, algorithm use, function-pointer conversion, and `std::function` |
| `notion-10-1` | `docs/C++ Notion/Chapter 10-1 Namespaces.md` | Namespace organization, using declarations/directives, nested and inline namespaces, unnamed namespaces, aliases, and ADL |
| `notion-10-10` | `docs/C++ Notion/Chapter 10-10 Modern C++ Features.md` | Selected C++11 through C++20 vocabulary including `auto`, `nullptr`, list initialization, `enum class`, `static_assert`, structured bindings, and newer library features |

All twelve mapped Notion chapter files were inspected. No mapped Notion source
was skipped.

### External References Consulted

Accessed on 2026-06-13.

| Label | Reference | Validation purpose |
| --- | --- | --- |
| `external-iso-cpp-reference-init` | C++ working draft `[dcl.init.ref]`: <https://eel.is/c++draft/dcl.init.ref> | Reference initialization, required initialization, rebinding prohibition, direct binding, and temporary binding |
| `external-iso-cpp-class` | C++ working draft `[class]`: <https://eel.is/c++draft/class> | Class members, implicit special member functions, copy/move behavior, and class object rules |
| `external-iso-cpp-ctor` | C++ working draft `[class.ctor]`: <https://eel.is/c++draft/class.ctor> | Default, converting, copy, and move constructor rules |
| `external-iso-cpp-base-init` | C++ working draft `[class.base.init]`: <https://eel.is/c++draft/class.base.init> | Member initializer lists, default member initializers, delegating constructors, and initialization order |
| `external-iso-cpp-dtor` | C++ working draft `[class.dtor]`: <https://eel.is/c++draft/class.dtor> | Destructor invocation, destruction order, implicit destructors, and lifetime termination |
| `external-iso-cpp-access` | C++ working draft `[class.access]`: <https://eel.is/c++draft/class.access> | `public`, `protected`, `private`, and the default-access difference between `class` and `struct` |
| `external-iso-cpp-friend` | C++ working draft `[class.friend]`: <https://eel.is/c++draft/class.friend> | Friend access, non-membership, and non-transitive/non-inherited friendship |
| `external-iso-cpp-overload` | C++ working draft `[over.match]`: <https://eel.is/c++draft/over.match> | Candidate, viable, best viable, conversion-sequence, and ambiguity rules |
| `external-iso-cpp-operators` | C++ working draft `[over.oper]`: <https://eel.is/c++draft/over.oper> | Operator-overload restrictions, member/non-member forms, and unchanged precedence/arity |
| `external-iso-cpp-linkage` | C++ working draft `[dcl.link]`: <https://eel.is/c++draft/dcl.link> | C and C++ language linkage without reducing the rule to “turn name mangling off” |
| `external-iso-cpp-elision` | C++ working draft `[class.copy.elision]`: <https://eel.is/c++draft/class.copy.elision> | Permitted copy elision and object-identity/lifetime effects |
| `external-iso-cpp-odr` | C++ working draft `[basic.def.odr]`: <https://eel.is/c++draft/basic.def.odr> | Reachability and multiple-definition requirements for inline entities |
| `external-core-guidelines` | C++ Core Guidelines: <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines> | Initialization, constructor design, parameter passing, resource ownership, interfaces, and class design |

### Source Coverage Status

`TOPIC_BRIEF_COMPLETE_WITH_ROUTING_GAPS`: canonical routing, every mapped
internal source, master requirements, guide requirements, major source
corrections, exact-language validation, downstream quality gates, and output
targets are recorded.

The mapped Notion set does not directly cover several master-CH08 MUST concepts:
class/object fundamentals, constructors, destructors, friend declarations, and
operator overloading. Exact working-draft and Core Guidelines references were
therefore used. A future routing cleanup should consider mapping the relevant
Notion class and operator chapters explicitly.

## 3. Priority And Dependencies

- Overall priority: `MUST`.
- Required depth: Deep.
- Master prerequisite: CH05, Compound Types In C.
- Important prior concepts:
  - C declarations, definitions, scope, storage duration, linkage, and
    translation units;
  - C structs and manual initialization;
  - pointers, arrays, function pointers, and callbacks;
  - C ownership and cleanup conventions;
  - object representation versus object lifetime.
- Follow-on chapters:
  - CH09, OOP in C++;
  - CH10, Resource Management in C++;
  - CH12, Modern C++;
  - CH13, Templates and Generic Programming.

The lesson must not assume that C++ is merely “C with classes.” It should show
how initialization, references, overload resolution, object lifetime, and
automatic construction/destruction change API and program design.

## 4. Scope And Depth Boundaries

### Deep In This Topic

- C versus C++ at the language-mechanism level.
- Namespaces and qualified lookup.
- References, lvalue references, and `const` references.
- Pointer versus reference contracts.
- Class and object fundamentals.
- `class` versus `struct` in C++.
- Access control: `public`, `private`, and introductory `protected`.
- Object initialization and lifetime.
- Default, parameterized, copy, and `explicit` constructors.
- Constructor member initializer lists.
- Default member initializers and delegating constructors.
- Destructors and deterministic cleanup.
- Function overloading and overload resolution.
- Operator overloading fundamentals and constraints.
- C versus C++ language linkage and practical ABI boundaries.
- Static data/function members at a fundamental level.
- Friend functions and friend classes, including their design cost.

### Medium In This Topic

- Fundamental types, initialization forms, narrowing prevention, and scope.
- Function declarations, definitions, signatures, default arguments, and
  return-value design.
- Pass by value, reference, pointer, and `const` reference.
- Implicit conversions and the four named C++ casts.
- `inline` as an ODR/linkage mechanism, distinct from optimizer inlining.
- Stream I/O and ordinary control flow.
- Basic lambdas and captures.
- `auto`, `nullptr`, range-for, `enum class`, `static_assert`, and structured
  bindings.
- Copy elision as object construction/lifetime behavior.

### Brief Awareness Or Forward Reference

- Rvalue references and perfect forwarding.
- Variadic templates and fold expressions.
- Advanced lambda capture/lifetime cases.
- ADL beyond the basic “associated namespaces/classes participate” model.
- Inline namespaces and ABI versioning.
- `std::optional`, `std::variant`, `std::any`, `std::string_view`, concepts,
  ranges, modules, and three-way comparison.
- Tail-call optimization.

These subjects belong primarily to later Modern C++, template, STL, or
performance chapters. Chapter 08 should introduce only the vocabulary needed
to understand fundamental examples.

### Defer Or Exclude

- Deep inheritance, virtual dispatch, overriding, abstract classes, and dynamic
  polymorphism: CH09.
- Rule of Five, move ownership, smart pointers, and full RAII design: CH10.
- Deep operator-overload API design: later OOP/Modern C++ treatment.
- Template metaprogramming, forwarding-reference mechanics, constraints, and
  concepts: CH13.
- Detailed ABI compatibility promises across compilers or standard-library
  versions.
- Linux Device Driver and kernel-driver material.

## 5. Mapped Source Corrections

Downstream outputs must use the mapped files as teaching input, not as
unquestioned authority.

### `notion-1-1`

- “Machine independent” is too strong. Portable C++ source still depends on the
  implementation, data model, ABI, operating environment, extensions, and
  undefined or implementation-defined behavior.
- C++ permits manual memory management but does not require it. Modern C++
  normally expresses ownership with values, containers, and RAII.
- The preprocessing/compilation/assembly/linking pipeline is a useful toolchain
  model, not a universal language-mandated physical pipeline.
- Compiler and package-install commands are platform- and version-specific.
- “No garbage collector” does not mean every C++ program manually manages raw
  memory.

### `notion-1-2`

- `const` is not simply “runtime” while `constexpr` is “compile time.”
  `const` objects may be usable in constant expressions, while a `constexpr`
  function can execute at runtime.
- A `constexpr` variable requires constant initialization; a `constexpr`
  function call is evaluated at compile time only in a constant-expression
  context.
- Fundamental type sizes are implementation-defined within standard
  constraints. Do not teach fixed byte counts for ordinary integer types.
- Plain `char` is a distinct type whose signedness is implementation-defined;
  it is not synonymous with ASCII or UTF-8.
- `wchar_t` size and encoding are implementation-defined.
- Unsigned types are not automatically the best choice for every size or count;
  mixed signed/unsigned arithmetic is a common bug source.
- “Storage class” is not a useful umbrella for `mutable`, and modern `auto`
  means placeholder type deduction rather than the removed storage-class
  spelling.
- Scope, storage duration, linkage, and lifetime must be taught as related but
  distinct properties.

### `notion-1-3`

- There is no simple universal promotion ladder such as
  `bool -> char -> short -> int -> long -> float -> double`.
- Integral promotions and the usual arithmetic conversions depend on rank,
  signedness, representability, and operand types.
- “Widening” is not automatically value-preserving; integer-to-floating
  conversion can lose precision.
- A narrowing conversion does not always require an explicit cast. Some
  contexts allow it; list initialization rejects many narrowing conversions.
- `static_cast` is not a general guarantee of safety.
- `dynamic_cast` belongs with polymorphism and should be a forward reference,
  not a core Chapter 08 mechanism.
- `reinterpret_cast` does not by itself make type punning, dereferencing, or
  aliasing valid.
- Removing `const` is not itself undefined behavior; modifying an object that
  was originally defined as const through the casted access path is.

### `notion-1-4`

- Do not claim that `switch` is inherently faster than `if`/`else`. Generated
  code depends on values, profile information, optimization, and target.
- `switch` works with integral and enumeration conditions after permitted
  conversions, not merely “integers/chars.”
- Operator precedence tables are reference material; production code should use
  parentheses and decomposition when intent is not obvious.
- Signed overflow, invalid shifts, division by zero, unsequenced effects, and
  out-of-range access need explicit warnings.
- Stream extraction failure must be checked before using parsed values.
- Range-for lifetime and proxy/reference behavior should not be oversimplified.

### `notion-2-1`

- The C++ language does not require a hardware call stack or a specific stack
  frame layout. These are common implementation models useful for debugging.
- Parameters and local objects are not guaranteed to occupy a particular stack
  location; ABI and optimizer choices matter.
- A function signature is more nuanced than only “name plus parameter types.”
  Downstream lessons should focus on what distinguishes overloads and avoid
  claiming that a simplified teaching definition is the full standard model.
- Return type alone cannot distinguish ordinary function overloads.
- Fixed limits such as “three parameters” or “fewer than 50 lines” are review
  heuristics, not language rules.
- Returning a reference requires a lifetime contract. Never return a reference
  to an automatic local object.

### `notion-2-2`

- Pass by value is language-level value initialization of the parameter, not a
  promise that bytes are copied onto a stack.
- Passing by value may copy, move, or be elided and may use registers or
  implementation-defined calling conventions.
- `const T&` is not a universal “sweet spot.” Prefer:
  - value for cheap values and when ownership/copy is desired;
  - `const T&` for non-null borrowed access to an existing object;
  - pointer when nullability, C interop, or pointer semantics are part of the
    contract;
  - view types such as `std::span` or `std::string_view` when those later
    facilities fit the API.
- A reference is not guaranteed to be represented as an address of a fixed
  machine size.
- Top-level `const` on a by-value parameter is mainly an implementation detail
  and does not change the caller-facing function type.
- C-style variadic arguments are governed by default argument promotions and
  exact `va_arg` type rules; “works only with POD types” is not an accurate
  complete rule.
- “Universal reference” should be labeled forwarding reference and deferred to
  the template chapter.
- Array parameters written with brackets are adjusted to pointer parameters;
  size information is not retained by that function parameter type.

### `notion-2-3`

- Overload resolution is a language rule. Name mangling is an implementation ABI
  technique used after semantic selection; C++ does not “support overloading
  through name mangling.”
- The exact mangled form is implementation- and ABI-specific.
- Do not promise that a return type is never represented in every mangled name;
  some ABI encodings include return information in specific template cases even
  though return type alone cannot overload ordinary functions.
- `extern "C"` specifies C language linkage. “Disables mangling” is a common
  implementation-level effect, not the complete language rule.
- C language linkage can also affect function types and calling conventions.
- Overload ranking is richer than one four-step hierarchy; reference binding,
  list initialization, templates, constraints, user-defined conversions, and
  tie-breakers matter.
- Access checking occurs after overload resolution selects a candidate.

### `notion-2-4`

- The primary language purpose of `inline` is to permit an entity to be defined
  in multiple translation units under ODR conditions and require its definition
  to be reachable where odr-used.
- `inline` does not require, guarantee, or reliably request call-site
  substitution.
- Optimizers may inline functions without the keyword and decline to inline
  functions that have it.
- Loops, recursion, virtual dispatch, static locals, and taking an address do not
  form a universal list of conditions under which optimization is impossible.
- Inline functions are not “exempt from the ODR.” They have specific ODR
  allowances and still must satisfy definition/token/lookup requirements.
- Function templates are not automatically `inline`; templates are commonly
  defined in headers for instantiation visibility for a different reason.
- A `constexpr` function is implicitly inline.
- A function defined inside a class definition is implicitly inline unless it is
  attached to a named module under the applicable modern rules.

### `notion-2-5`

- Tail-call optimization is not guaranteed by C++ and is not guaranteed merely
  by using `-O2`.
- Recursive code must remain correct without TCO.
- Stack size and maximum recursion depth are environment-specific; do not teach
  a fixed “one million calls” or fixed stack range.
- Recursion is not inherently slower than iteration in every case. Complexity,
  data structures, optimizer behavior, locality, and allocation strategy matter.
- Graph recursion also needs visited-state/cycle reasoning, not only a base case.
- Recursion is supporting material rather than a master-CH08 core concept.

### `notion-2-6`

- A lambda expression creates a closure object of a unique unnamed class type;
  “anonymous inline function” is only an introductory analogy.
- `mutable` causes the generated call operator to be non-const; it does not
  change the original captured objects.
- Reference captures can dangle when the closure outlives captured objects.
- Capturing `this` captures a pointer; asynchronous or deferred invocation can
  outlive the object.
- Capture-defaults can hide lifetime and dependency costs. Prefer explicit
  captures when longevity or ownership matters.
- Conversion to a function pointer applies to non-capturing lambdas under the
  language rules; generic-lambda details require care.
- `std::function` adds type erasure and can add allocation and indirect-call
  overhead. It is not interchangeable with every move-only callable.
- Advanced lambda material belongs later; Chapter 08 needs syntax, closure
  lifetime, and basic callback/algorithm use.

### `notion-10-1`

- A using directive in a header is a strong project-level prohibition because
  it pollutes includers; phrase it as a best practice rather than a grammar rule.
- Unnamed namespaces are the normal C++ mechanism for translation-unit-local
  names, but downstream docs should still explain linkage and visibility
  accurately.
- Inline namespaces support versioned naming and lookup; they do not by
  themselves guarantee ABI compatibility.
- ADL considers associated namespaces and classes for unqualified function
  calls. It is not a general search for variables or arbitrary members.
- Friend functions defined in a class may depend on ADL for discovery and should
  be introduced carefully.
- Extending `namespace std` is generally prohibited except for explicitly
  permitted standard customization cases.

### `notion-10-10`

- The file spans many later chapters and must not set Chapter 08 depth.
- C++20 modules are standardized, not merely a “preview,” though compiler and
  build-system support remains version-specific.
- C++20 designated initializers are restricted to aggregates and declaration
  order; they are not C-compatible in all details.
- `auto` follows deduction rules and can drop references/top-level cv
  qualifiers. It should not be presented as “the compiler knows what I mean.”
- `std::string_view` is non-owning and can dangle.
- `std::optional`, `std::variant`, `std::any`, ranges, concepts, and modules are
  forward references for this chapter.
- List initialization prevents many narrowing conversions but has
  `std::initializer_list` preference and overload-resolution edge cases.

## 6. Merged Concept Model

### Foundation Layer

- C++ is a compiled, statically typed, multi-paradigm language.
- Translation units, declarations, definitions, linkage, and the ODR connect
  source files into a program.
- Namespaces organize names but do not create runtime objects.
- Scope, storage duration, linkage, and lifetime answer different questions.

### Initialization And Value Layer

- Prefer initialization over “declare then assign.”
- Distinguish default-, value-, direct-, copy-, and list-initialization only to
  the depth needed to explain behavior and bugs.
- List initialization rejects many narrowing conversions.
- `const` expresses immutability through an access path.
- `constexpr` enables constant-expression use when its requirements are met.
- `auto` deduces a type according to language rules; inspect references and
  cv-qualification deliberately.

### Reference And Parameter Layer

- A reference is initialized to refer to an object/function and cannot later be
  reseated.
- Assignment through a reference assigns to the referred object.
- `T&` commonly expresses required mutable borrowed access.
- `const T&` commonly expresses required read-only borrowed access and can bind
  to temporaries under lifetime rules.
- A pointer supports explicit address semantics and can represent null.
- API choice communicates nullability, ownership, mutability, and lifetime.

### Object And Class Layer

- An object has type, storage, value/state, and lifetime.
- A class defines a user-defined type with data, behavior, access control, and
  invariants.
- In C++, `class` and `struct` have the same major capabilities; their default
  member and base access differ.
- Constructors establish a valid initial state.
- Member initializer lists initialize bases and members before the constructor
  body.
- Members initialize in declaration order, not initializer-list spelling order.
- Destructors run when object lifetime ends in the applicable context and enable
  deterministic cleanup.
- Defaulted special member functions perform memberwise behavior subject to
  deletion and accessibility rules.
- Copy elision can change how many constructor/destructor calls are observed and
  can affect source/target object identity.

### Function And Overload Layer

- A declaration introduces a callable interface; a definition supplies behavior.
- Overloading creates an overload set distinguished by parameter and member
  function properties, not return type alone.
- Overload resolution forms candidates, filters viable functions, ranks
  conversions, and requires a unique best viable function.
- Default arguments are substituted at the call site and can interact badly
  with overload sets.
- Converting constructors should normally be `explicit` unless implicit
  conversion is intentionally part of the type's contract.
- Operator overloads are functions with language-defined syntax and constraints;
  they cannot change precedence, grouping, or operand count.

### Encapsulation And Organization Layer

- `private` supports invariant ownership by restricting naming access.
- `public` defines the supported interface.
- `protected` is mainly relevant to inheritance and should remain introductory.
- Static members belong to the class rather than each object.
- Friendship grants selected access but does not make a function a member and is
  neither inherited nor transitive.
- Namespaces prevent collisions and communicate ownership of APIs.
- Avoid global using directives in headers.

### ABI And Interoperability Layer

- C++ implementations usually encode overload/type information into external
  symbols according to an ABI.
- Mangling format is implementation-specific.
- `extern "C"` requests C language linkage for supported declarations.
- Mixed C/C++ headers should use `#ifdef __cplusplus` guards.
- C-compatible linkage does not make arbitrary C++ classes, exceptions,
  templates, or object layouts portable across a C ABI.

## 7. Practical Usage Angles

### C Usage

- Compare C struct plus init/cleanup functions with a C++ class constructor and
  destructor.
- Compare output pointers with references and returned values.
- Compare type-suffixed C function names with C++ overload sets.
- Compare C callback function pointers with simple non-capturing lambdas.
- Preserve a narrow C ABI when integrating C and C++.

### C++ Usage

- Model valid domain states with constructors and private data.
- Use member initializer lists and declaration-order reasoning.
- Use references to express required borrowed access.
- Use overloads only when operations share one clear semantic concept.
- Use namespaces for project/module ownership.
- Use deterministic destruction to tie cleanup to object lifetime.

### Embedded Usage

- Value types for units, IDs, register descriptions, configuration, and parsed
  protocol fields.
- Constructors that establish valid configuration without hidden dynamic
  allocation.
- Destructors for scoped locks, bus transactions, interrupt masks, or resource
  handles only when timing/context rules permit.
- References for required dependencies whose lifetime is externally guaranteed.
- C-linkage wrappers at firmware libraries or vendor HAL boundaries.
- Avoid exceptions, RTTI, dynamic allocation, or iostreams when project policy
  excludes them; these are policy/toolchain decisions, not universal C++ rules.

### Enterprise Usage

- Public APIs with explicit ownership, nullability, mutation, and lifetime
  contracts.
- Warning-clean builds across supported compilers and language modes.
- ABI stability plans for shared-library boundaries.
- Header discipline: minimal includes, no global using directives, correct
  inline definitions, and stable declarations.
- Unit tests that observe object invariants and lifetime effects rather than
  constructor-call counts that copy elision may change.

## 8. Required Comparisons

| Comparison | Required conclusion |
| --- | --- |
| C vs C++ | C++ adds language-level references, classes, constructors/destructors, overloading, templates, namespaces, and stronger abstraction mechanisms while retaining low-level control. C compatibility is substantial but not identity. |
| Pointer vs reference | A pointer is an object that stores an address-like value and may represent null; a reference is initialized as an alias and cannot be reseated. Neither automatically owns the referred object. |
| `struct` vs `class` in C++ | They have the same main language capabilities. Default member/base access is public for `struct` and private for `class`; convention usually distinguishes passive data from invariant-owning abstractions. |
| Constructor vs C init function | A constructor participates automatically in object initialization and overload resolution; a C init function is an ordinary function whose invocation and failure protocol are explicit. Both must establish documented invariants. |
| Destructor vs C cleanup function | A destructor is invoked by object-lifetime rules; C cleanup must be called on every owning path. Neither excuses unclear ownership or invalid cleanup context. |
| Function overloading vs C naming convention | C++ resolves one source name against typed candidates; C APIs encode distinctions in separate names such as `parse_i32` and `parse_f64`. |
| Overloading vs overriding | Overloading selects among declarations in an overload set, usually at compile time. Overriding replaces virtual behavior in a derived class and belongs primarily to CH09. |
| Pass by value vs `T&` vs `const T&` vs pointer | Choose according to ownership/copy, required mutability, required borrowing, and nullability; do not choose solely from assumed machine size. |
| Default initialization vs value/list initialization | Initialization syntax can change whether scalars are initialized, whether narrowing is rejected, and which constructor is selected. |
| `const` vs `constexpr` | `const` restricts modification through an access path; `constexpr` declares constant-expression capability/requirements. Compile-time evaluation depends on context and arguments. |
| C-style cast vs named C++ casts | Named casts expose intent and constrain categories of conversion. None makes an invalid lifetime, aliasing, alignment, or downcast safe. |
| `inline` vs optimizer inlining | The keyword has ODR/linkage semantics. Call-site substitution is an optimizer decision independent of the keyword. |
| Function pointer vs lambda | A function pointer has no stored capture state. A lambda is a closure object; only qualifying non-capturing lambdas convert to function pointers. |
| Namespace qualification vs using declaration/directive | Qualification is explicit; a using declaration imports selected names; a using directive makes a namespace's names available to lookup and is dangerous in headers. |
| `extern "C"` vs C++ linkage | They select language linkage properties supported by the implementation; they are not a general binary-compatibility guarantee for arbitrary C++ interfaces. |

## 9. Common Bugs And Review Findings

### Initialization And Lifetime

- Reading an uninitialized fundamental object.
- Assuming `{}` and `()` always select the same constructor.
- Depending on initializer-list spelling order instead of member declaration
  order.
- Binding a reference/member to an object that dies too early.
- Returning a reference or pointer to an automatic local object.
- Calling cleanup manually and then allowing automatic destruction to run again.
- Using an object before construction completes or after destruction starts.
- Testing exact copy/move counts without accounting for elision.

### References And Parameters

- Treating a reference as an owner.
- Hiding mutation behind a non-const reference without API clarity.
- Using a pointer for a required non-null dependency without documenting it.
- Assuming `const T&` extends every temporary lifetime in every context.
- Capturing references in a lambda that escapes the referenced scope.
- Capturing `this` in deferred work without an object-lifetime guarantee.
- Losing array extent through parameter adjustment.
- Mixing signed and unsigned sizes.

### Classes And Constructors

- Leaving members indeterminate.
- Assigning in the constructor body when direct member initialization is needed.
- Omitting `explicit` from a single-argument converting constructor.
- Exposing data publicly while claiming to maintain an invariant.
- Adding a destructor/copy operation without understanding generated special
  members.
- Declaring reference or const members without a viable initialization strategy.
- Overusing friendship to bypass a weak class interface.
- Confusing static class members with per-object state.

### Functions And Overloads

- Overloading functions whose meanings are unrelated.
- Ambiguous calls caused by symmetric conversions or overlapping defaults.
- Expecting return type to resolve an overload.
- Confusing hiding, overloading, and overriding.
- Assuming an inaccessible overload is removed before overload resolution.
- Adding implicit conversions that silently redirect overload selection.
- Designing surprising operator semantics.

### Translation, Linkage, And Headers

- Declaring a C function without C language linkage in C++.
- Putting a global using directive in a header.
- Giving inline definitions different token/lookup meaning across translation
  units.
- Confusing declaration, definition, linkage, and visibility.
- Depending on a compiler's mangled spelling as a portable language rule.
- Exposing compiler-specific C++ ABI types across an intended stable C boundary.

### Control Flow And Conversion

- Assignment in a condition where comparison was intended.
- Signed overflow, invalid shifts, division by zero, and unchecked stream input.
- Narrowing or sign-changing conversion without validation.
- Believing a `static_cast` proves range safety.
- Dereferencing a pointer produced by `reinterpret_cast` without satisfying
  alignment, lifetime, and aliasing rules.
- Depending on tail-call optimization for correctness.

## 10. Debugging And Verification Notes

### Compiler Diagnostics

Use strict warnings and a selected language mode:

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
    -Wold-style-cast -Woverloaded-virtual -Werror source.cpp
```

Flags are compiler-specific. A project should validate them on each supported
toolchain rather than copy one list blindly.

### Initialization And Lifetime Debugging

- Reduce the case to one object and print or break on constructors/destructors.
- Remember that copy elision can remove observable copy/move operations.
- Use ASan for executed use-after-scope/use-after-free paths where supported.
- Use UBSan for selected invalid conversions, shifts, alignment, and other
  executed undefined behavior.
- Inspect declaration order when constructor warnings mention reorder.
- Treat a reference as a borrowed lifetime during review.

### Overload Debugging

- List the candidate functions.
- Identify which candidates are viable.
- Write the conversion sequence required by each argument.
- Check list-initialization and `explicit` restrictions.
- Check member cv/ref qualification.
- Add a temporary explicit type or cast only to diagnose; prefer redesign when
  normal calls remain surprising.

### Linkage Debugging

- Compile C sources as C and C++ sources as C++.
- Inspect object symbols with `nm`, `readelf`, `objdump`, `c++filt`, or the
  platform equivalent.
- Verify the header presents identical declarations and correct language-linkage
  guards to both languages.
- Check compiler, target, ABI, architecture, and runtime-library compatibility.
- Do not diagnose every link failure as “name mangling”; missing definitions,
  wrong libraries, signature mismatches, visibility, and link order also matter.

### Namespace And ADL Debugging

- Fully qualify a call to test whether ordinary lookup or ADL changed selection.
- Remove broad using directives while reducing a collision.
- Place operators and customization functions in the namespace associated with
  their user-defined type when ADL is intended.
- Avoid adding declarations to `std` unless the standard explicitly permits it.

## 11. Best Practices

- Initialize every object before use.
- Prefer `{}` when narrowing prevention is important, while understanding
  initializer-list overload preference.
- Keep scope, storage duration, linkage, and lifetime terminology distinct.
- Use value semantics by default.
- Use references for required borrowed access and pointers when null/address
  semantics are part of the contract.
- State lifetime requirements for every borrowed parameter or stored reference.
- Prefer constructors that establish valid invariants.
- Initialize members in member initializer lists and order the list to match
  declaration order.
- Use default member initializers for stable defaults.
- Mark converting constructors `explicit` unless implicit conversion is
  intentional and unsurprising.
- Prefer Rule of Zero types; defer manual ownership to the resource-management
  chapter.
- Keep public interfaces small and represent invariants privately.
- Use friendship narrowly and for a concrete interface reason.
- Overload only operations with one coherent meaning.
- Make operator overloads behave like the corresponding built-in concept.
- Use namespaces in libraries and avoid global using directives in headers.
- Treat `inline` and language linkage as correctness/ODR/ABI topics, not
  performance slogans.
- Keep C ABI boundaries simple: scalar values, fixed-layout records with an
  explicit ABI policy, opaque handles, and C-compatible callbacks.
- Compile examples under at least C++17 unless a feature-specific example
  explicitly selects C++20.

## 12. Interview Angles

### Beginner

- What does C++ add beyond C?
- What is the difference between a declaration and a definition?
- What is a reference, and can it be reseated?
- Compare pointer and reference.
- Compare `struct` and `class` in C++.
- What do `public`, `private`, and `protected` mean?
- What is a constructor? What is a destructor?
- Why use a member initializer list?
- What is function overloading?
- Why should `using namespace std;` be avoided in headers?

### Mid-Level

- Explain object lifetime from initialization through destruction.
- Compare default-, value-, direct-, copy-, and list-initialization.
- Why should converting constructors often be `explicit`?
- In what order are members initialized and destroyed?
- How does overload resolution select a function?
- Why can a private overload still make a call fail?
- Explain `inline` without discussing performance first.
- Explain C language linkage more accurately than “disables mangling.”
- When should an API take a value, reference, const reference, or pointer?
- What can make a lambda capture dangle?
- What does copy elision change?

### Senior

- Design a stable C boundary around a C++ implementation.
- Review a class that stores references and explain its lifetime constraints.
- Explain how adding a destructor or copy operation affects generated special
  members.
- Diagnose an ODR failure involving an inline header function.
- Explain an ambiguous overload using candidate/viable/ranking terminology.
- Discuss ABI risks of exposing STL types, exceptions, RTTI, or compiler-specific
  class layouts across library boundaries.
- Decide whether a passive aggregate should remain a `struct` or become an
  invariant-owning class.
- Review a deferred lambda callback for ownership and lifetime safety.
- Explain why tail-call optimization cannot be a correctness requirement.

### Interview Traps

- “References are const pointers.”
- “A reference can never be null” without discussing invalid construction or
  undefined behavior paths.
- “`struct` is C-style and `class` is object-oriented.”
- “Constructors allocate memory and destructors free memory.”
- “`inline` means the compiler inserts the body.”
- “C++ overloading is provided by name mangling.”
- “`extern "C"` makes any C++ API C-compatible.”
- “`const T&` is always fastest.”
- “`constexpr` always runs at compile time.”
- “`switch` is always faster.”
- “`-O2` guarantees tail recursion.”

## 13. Practice Tasks

### Basic

1. Convert a C `struct` plus init function into a C++ value type with a
   constructor and private invariant.
2. Write functions taking an `int`, `int&`, `const int&`, and `int*`; explain
   the contract of each.
3. Demonstrate a namespace, a using declaration, and explicit qualification.
4. Compile one program that shows list initialization rejecting narrowing.
5. Write two overloads and predict which one is selected for `char`, `int`,
   `long`, and `double` arguments.

### Intermediate

1. Build a `SensorConfig` class with default member initializers, an explicit
   constructor, validation, and read-only accessors.
2. Demonstrate declaration-order initialization with `-Wreorder`.
3. Create a C-compatible header used from one `.c` and one `.cpp` file.
4. Inspect symbols with `nm` and `c++filt`, then explain what is ABI-specific.
5. Write a lambda callback and a test that exposes a dangling reference capture;
   then redesign the lifetime.
6. Compare a C cleanup path with a scoped C++ object whose destructor records
   release.

### Advanced

1. Diagnose an ambiguous overload and redesign the API without requiring casts
   at every call.
2. Create an inline function in a header, use it from two translation units, and
   explain the relevant ODR requirements.
3. Design an opaque-handle C API backed by a private C++ class.
4. Review a class with a user-declared destructor and explain its copy/move
   behavior.
5. Demonstrate permitted copy elision without asserting an exact copy count.
6. Design a small operator overload whose semantics match the built-in
   expectation, then identify an operator overload that would be misleading.

## 14. Gaps And External Validation Needs

### Mapping Gaps

- The mapped list omits the Notion class/object/constructor chapter even though
  these are central master-CH08 MUST concepts.
- It also omits the Notion static/friend and operator-overloading chapters while
  master CH08 names those concepts.
- `notion-2-2`, `notion-2-6`, and `notion-10-10` include substantial later
  material that must not inflate Chapter 08.

Suggested future routing review:

- consider mapping the relevant portions of Notion `5-1` for classes,
  constructors, and objects;
- consider mapping the relevant portions of Notion `5-2` for static members and
  friendship;
- consider mapping the relevant portions of Notion `5-5` for operator
  overloading;
- keep inheritance/polymorphism depth in CH09 even if those source files are
  later added.

### Exact-Behavior Validation

Downstream knowledge/interview/example work should recheck trusted references
when teaching:

- temporary lifetime extension and reference members;
- list-initialization and `std::initializer_list` preference;
- implicit generation/deletion of special member functions;
- copy elision and version differences;
- inline ODR rules, especially with modules;
- overload ranking and user-defined conversions;
- language linkage and target ABI details;
- lambda capture changes across C++ standards;
- compiler-specific warning, symbol-inspection, and sanitizer commands.

### Version Baseline

Recommended downstream baseline:

- C++17 for the main lesson and examples;
- clearly labeled C++20 side notes for concepts, ranges, modules, designated
  initializers, and three-way comparison;
- avoid teaching draft-only or compiler-preview behavior as portable C++.

## 15. Quality Gate For Later Outputs

- Preserve deep treatment for master MUST concepts.
- State CH05 as the prerequisite.
- Teach object lifetime and initialization before syntax-heavy feature lists.
- Include every master comparison.
- Distinguish `struct`/`class` convention from their actual language difference.
- Distinguish overload resolution from symbol mangling.
- Distinguish `inline` language semantics from optimizer inlining.
- Distinguish source portability from ABI/binary portability.
- Explain parameter passing without promising stack placement or reference size.
- Explain references as non-owning aliases with explicit lifetime requirements.
- Include construction order, destruction order, and copy-elision caveats.
- Keep advanced OOP, move ownership, templates, and modern-library types as
  forward references.
- Use minimal compile-oriented C++17 examples.
- Include warning, sanitizer, debugger, symbol, and multi-translation-unit
  verification where relevant.
- Keep audit/source tables in this Topic Brief, not learner-facing documents.
- Do not claim complete ABI portability, guaranteed optimization, or universal
  performance outcomes.
- Do not use Linux Device Driver or kernel-driver material.

## 16. Output Targets

| Output | Current status | Intended scope |
| --- | --- | --- |
| `coverage/topic-briefs/08-cpp-fundamentals.md` | Created | Internal source audit, corrections, merged concepts, comparisons, gaps, external validation, and downstream requirements |
| `knowledge/08-cpp-fundamentals.md` | Created | Learner-facing C++ fundamentals lesson without audit metadata |
| `interview/08-cpp-fundamentals.md` | Created | Beginner, mid-level, and senior interview pack |
| `examples/08-cpp-fundamentals/README.md` | Created | Minimal multi-file, object-lifetime, reference, overload, and C-linkage examples |

Audit metadata must remain under `coverage/` and must not be copied into
learner-facing outputs.
