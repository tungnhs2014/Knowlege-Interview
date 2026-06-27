# Topic Brief 10 - Resource Management In C++

## 1. Canonical Routing

| Field | Validated value |
| --- | --- |
| `learning_path_number` | `10` |
| Title | Resource Management In C++ |
| `slug` | `resource-management-in-cpp` |
| Requested topic | Ownership, lifetime, RAII, copying, moving, and smart pointers |
| Master source | `master-ch10` |
| Required Notion sources | `notion-4-1`, `notion-4-2`, `notion-8-2`, `notion-8-3`, `notion-10-4`, `notion-10-6` |
| Topic Brief | `coverage/topic-briefs/10-resource-management-in-cpp.md` |
| Knowledge target | `knowledge/10-resource-management-in-cpp.md` |
| Interview target | `interview/10-resource-management-in-cpp.md` |
| Example target | `examples/10-resource-management-in-cpp/README.md` |

Validation result: the number, title, slug, master source, six mapped Notion
sources, and all canonical output paths match `LEARNING_PATH.md`.

This step creates the Topic Brief only. It does not create or modify knowledge,
interview, or example outputs.

## 2. Source Coverage

### Internal Sources Read

| Source label | Path | Coverage contribution |
| --- | --- | --- |
| `master-ch10` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, CH10 | MUST priority, deep depth, CH09 prerequisite, keyword scope, six required comparisons, ownership/cleanup rule, and interview focus |
| `guide-section-03` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 3 | Deep treatment for MUST concepts, medium treatment for SHOULD concepts, and controlled treatment for EXPERT concepts |
| `guide-section-04` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 4 | Full-chapter structure and C, C++, embedded, enterprise, bug, debug, interview, practice, and reference angles |
| `guide-section-06` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 6 | Required C versus C++ allocation and manual-cleanup versus RAII comparisons |
| `guide-section-07` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 7 | Practical examples and depth control |
| `guide-section-09` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 9 | Trusted-source routing for exact C++ and resource-management guidance |
| `guide-section-10` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 10 | Clear technical English, concise Markdown, compile-oriented examples, and safety warnings |
| `guide-section-11` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 11 | Topic Brief, lesson, interview pack, examples, and review expectations |
| `guide-section-14` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 14 | Final quality checklist |
| `notion-index` | `docs/C++ Notion/C++ Programming.md` | Notion inventory and mapped chapter identity validation |
| `notion-4-1` | `docs/C++ Notion/Chapter 4-1 Dynamic Memory Basics.md` | Storage overview, `new`/`delete`, arrays, allocation failure, and manual cleanup hazards |
| `notion-4-2` | `docs/C++ Notion/Chapter 4-2 Advanced Memory Management.md` | C allocation family, allocation-family mismatch, leaks, dangling pointers, double deletion, RAII, placement new, and memory tools |
| `notion-8-2` | `docs/C++ Notion/Chapter 8-2 Exception Handling - Exception Safety & RAII.md` | Exception guarantees, copy-and-swap, RAII during failure, standard guards, custom wrappers, and `noexcept` move motivation |
| `notion-8-3` | `docs/C++ Notion/Chapter 8-3 Exception Handling - noexcept, Stack Unwinding.md` | Stack unwinding, partial construction, destructor failure, exception policy, and interview treatment |
| `notion-10-4` | `docs/C++ Notion/Chapter 10-4 Smart Pointers.md` | `unique_ptr`, `shared_ptr`, `weak_ptr`, factories, custom deleters, cycles, ownership transfer, and common smart-pointer bugs |
| `notion-10-6` | `docs/C++ Notion/Chapter 10-6 Move Semantics.md` | Value categories, rvalue references, move operations, `std::move`, Rule of Five/Zero, forwarding, STL interaction, and moved-from states |

All six mapped Notion chapter files were inspected. No mapped Notion source
was skipped.

### External Reference Consulted

Accessed on 2026-06-14.

| Label | Reference | Validation purpose |
| --- | --- | --- |
| `external-core-guidelines` | C++ Core Guidelines: <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines> | RAII and resource handles; scoped objects; avoiding explicit `malloc`/`free` and naked `new`/`delete`; immediate transfer to a manager; ownership with `unique_ptr`/`shared_ptr`; preferring `unique_ptr`; Rule of Zero; and `noexcept` move guidance |

### Source Coverage Status

`TOPIC_BRIEF_COMPLETE_WITH_MAPPED_SOURCE_GAPS`: routing, all mapped Notion
files, master requirements, guide requirements, major source corrections,
merged concepts, comparisons, bugs, debugging workflow, interview angles,
practice targets, external validation needs, and downstream quality gates are
recorded.

The mapped notes cover manual allocation, RAII, exception safety, smart
pointers, and move semantics in substantial detail. They do not adequately
cover all master items: Rule of Three as a named design rule, pImpl,
allocator-aware design, `std::pmr`, robust non-memory RAII wrappers, or the
expert allocator topics. Those gaps are routed below.

## 3. Priority And Dependencies

- Overall priority: `MUST`.
- Required depth: Deep.
- Master prerequisite: CH09, OOP In C++.
- Required prior model:
  - object lifetime, constructors, destructors, and member initialization;
  - class invariants and special member functions;
  - pointers, references, arrays, and dynamic allocation in C;
  - polymorphic base destruction;
  - exceptions and ordinary control flow at an introductory level;
  - ownership versus borrowing as an API-design question.
- Follow-on chapters:
  - CH11, STL And Standard Library;
  - CH12/CH13, Modern C++ and templates;
  - CH14, Error Handling;
  - CH15, Concurrency;
  - CH17/CH18, C/C++ and POSIX/Modern C++ comparisons.

Chapter 10 must connect object lifetime to every resource, not just heap
memory. Its center is the ownership model and cleanup path: who owns a resource,
how ownership moves or is shared, what aliases may outlive, and what happens on
normal return, early return, exception, partial construction, and move.

## 4. Scope And Depth Boundaries

### Deep In This Topic

- Resource versus memory: dynamic memory, file handles, locks, sockets, and
  other acquire/release pairs.
- Object lifetime, storage duration, ownership, borrowing, and aliasing.
- RAII: acquire or adopt into a resource-owning object and release in its
  destructor.
- Constructor failure, partial construction, stack unwinding, and no-throw
  cleanup.
- Raw owning pointers and manual `new`/`delete` as mechanisms to recognize and
  review, not the default application-level design.
- Copy constructor and copy assignment.
- Move constructor and move assignment.
- Shallow/memberwise copy versus semantic deep copy.
- Rule of Three, Rule of Five, and Rule of Zero.
- `std::unique_ptr`, `std::shared_ptr`, and `std::weak_ptr`.
- Ownership transfer at function boundaries.
- Custom deleters for C APIs and non-memory resources.
- Shared-ownership cycles and observer lifetime.
- Exception guarantees where they affect resource integrity.
- All six master comparisons.

### Medium In This Topic

- `std::make_unique` and `std::make_shared`, including reasons not to use a
  factory in a specific ownership/deleter/allocation design.
- Copy-and-swap and allocate-before-commit assignment.
- `noexcept` and `std::move_if_noexcept`.
- Valid but unspecified moved-from state.
- Polymorphic ownership and virtual destructor interaction.
- pImpl ownership using `std::unique_ptr`, incomplete types, and out-of-line
  destruction.
- RAII wrappers for a C `FILE*`, user-space file descriptor, and mutex lock.
- Placement new as separate allocation and construction.
- Allocator concept and allocator-aware containers.
- Shared control-block behavior and thread-safety boundaries.

### Controlled Expert Awareness

- Custom allocators.
- Memory pools and arena allocation.
- `std::pmr::memory_resource` and polymorphic allocators.
- Object lifetime, alignment, destruction, and reuse in raw storage.
- Small-object optimization as an implementation/design technique, not a
  universal standard-library guarantee.
- Allocation latency, fragmentation, locality, and bounded-capacity designs.

### Defer Or Exclude

- Full allocator-traits implementation and allocator propagation rules:
  CH12/CH13 or a dedicated advanced allocator lesson.
- Full exception taxonomy and expected-style error handling: CH14.
- Container invalidation details: CH11.
- Atomic reference-count implementation and concurrent object access: CH15.
- Kernel allocators, kernel locks, Linux Device Driver, and kernel-driver
  material.

## 5. Mapped Source Corrections

The Notion chapters are useful source notes but contain simplifications and
overstatements. Downstream outputs must use the following corrected model.

### `notion-4-1`

- The C++ language does not require a five-segment process layout, upward heap
  growth, downward stack growth, fixed address ordering, or collision between
  stack and heap. Present these only as common platform models.
- Automatic storage is not synonymous with "the stack," and dynamic storage is
  not normatively synonymous with "the heap." `new` obtains dynamic storage
  from the free store.
- Runtime size alone does not require manual `new[]`; prefer `std::vector`,
  `std::string`, or another owner type.
- Avoid fixed claims such as stack allocation being 2000 times faster. Cost is
  target-, allocator-, optimization-, and workload-dependent.
- Reading default-initialized scalar storage is undefined behavior; do not
  print or characterize it as a stable "garbage value."
- Mismatching `new`/`delete` and `new[]`/`delete[]` is undefined behavior. Do
  not reduce it to destroying only the first element.
- Assigning `nullptr` after deletion changes one pointer only. It does not
  repair aliases and is not a substitute for an ownership design.
- Allocation failure should be handled at a level that can recover or terminate
  meaningfully. Do not require a local `bad_alloc` catch after every allocation.
- `std::nothrow` changes failure reporting; it does not inherently make an
  allocation suitable for performance-critical or real-time code.

### `notion-4-2`

- In C++, `malloc` returns `void*` and a conversion is required, but manual C
  allocation should normally be isolated behind a typed RAII owner. In C, do
  not cast `malloc` merely as a habit.
- `realloc` may move the allocation; preserve the original pointer until
  success and respect object-lifetime/type restrictions.
- Mixing allocation and deallocation families is undefined behavior, not a
  reliably classifiable destructor skip or allocator crash.
- Nulling a pointer does not prevent double deletion through aliases. A single
  owner type prevents the ownership duplication.
- Null checks do not identify dangling non-null pointers.
- Safe-delete macros obscure ownership and evaluate policy through preprocessor
  code. Do not teach them as modern C++ practice.
- Raw pointers are valid for non-owning access. The issue is raw ownership or
  unclear lifetime, not the raw pointer syntax itself.
- Smart pointers do not make all dangling access impossible; borrowed raw
  pointers/references and `weak_ptr` observations still require lifetime
  checks.
- Placement new requires correctly sized and aligned storage, explicit lifetime
  reasoning, and matching destruction. It must not be presented as ordinary
  construction at an arbitrary hardware address.
- AddressSanitizer is highly useful but does not guarantee zero false positives,
  find every leak, or prove lifetime correctness on unexecuted paths.

### `notion-8-2`

- Exception-safety guarantees apply to operations and observable state. The
  basic guarantee means invariants hold and no resources leak; it must not
  leave related members internally inconsistent.
- The strong guarantee is not required for every assignment or operation. State
  and document the guarantee that the API can provide economically.
- Move operations should be `noexcept` when their implementation really cannot
  throw. "All move constructors and move assignments must be `noexcept`" is too
  absolute.
- Standard containers may prefer copying during relocation when copying is
  available and moving may throw. Exact behavior depends on operation and type
  requirements; avoid promising that every container always copies.
- Destructors should not allow exceptions to escape. A destructor can be
  declared `noexcept(false)`, but throwing during active unwinding terminates;
  the default design should be non-throwing cleanup.
- Copy-and-swap is one strong-guarantee technique, not the mandatory or always
  most efficient assignment implementation.
- RAII protects resource cleanup; it does not automatically make every state
  mutation provide the strong guarantee.

### `notion-8-3`

- Stack unwinding destroys fully constructed automatic objects and fully
  constructed subobjects. The destructor of an object whose construction did
  not complete is not called.
- If an exception exits a `noexcept` function, `std::terminate` is called.
  Avoid broad claims that no cleanup can happen anywhere before termination;
  evaluation and implementation details require precise wording.
- Exceptions versus status/result types is a project and API decision, not a
  universal "libraries use exceptions" rule.
- File-not-found, parse failure, network failure, and similar conditions are
  domain-dependent; they are not intrinsically exceptional or expected.
- RAII works with exceptions, status returns, early returns, and ordinary scope
  exit. It is not dependent on using exceptions.
- Destructors that need failure reporting should offer an explicit operation
  such as `close()` or `commit()` and retain a no-throw best-effort destructor.
- Error codes need not be untyped integers; enums and expected-style result
  types can provide typed, non-ignorable interfaces.

### `notion-10-4`

- `std::unique_ptr<T, D>` is often pointer-sized with an empty deleter, but its
  size is not guaranteed and stateful deleters can increase it.
- `std::shared_ptr` reference-count operations support concurrent access to
  separate smart-pointer objects sharing a control block. They do not make the
  managed object thread-safe.
- `std::weak_ptr` observes a shared-ownership control block and can break
  ownership cycles, but cycle breaking is not its only use.
- `weak_ptr::lock()` is the safe way to obtain temporary shared ownership.
  Checking `expired()` and then acting separately has a race.
- `use_count()` is mainly diagnostic. Do not use it as a synchronization or
  uniqueness decision in concurrent code.
- Never create independent `shared_ptr` control blocks for the same raw
  pointer. Use one ownership origin and `std::enable_shared_from_this` where an
  object must safely produce a shared owner for itself.
- Passing `shared_ptr` by value is correct when the function intentionally
  shares or retains ownership. Use a reference or raw/reference view when it
  only observes.
- Returning `unique_ptr` and converting it to `shared_ptr` at the receiving
  boundary is a valid way to preserve the most restrictive ownership choice.
  Do not forbid it.
- `make_shared` commonly combines allocations, but retained `weak_ptr`
  instances may keep the combined storage allocated after object destruction.
- Smart pointers manage ownership, not bounds, iterator validity, protocol
  state, or arbitrary borrowed aliases.

### `notion-10-6`

- Move semantics does not always "steal internals," avoid allocation, or run in
  constant time. A move operation is type-defined.
- An lvalue is not simply "an expression with a name," and an rvalue is not
  simply "a temporary without an address." Teach identity and value-category
  rules without relying on those shortcuts.
- A named rvalue-reference variable is an lvalue expression.
- `std::move` is a cast that permits move overload selection; it performs no
  transfer by itself.
- Moving from a `const` object commonly selects copying because typical move
  constructors require a non-const rvalue reference.
- A moved-from standard-library object is generally valid but has an
  unspecified state unless its type documents a stronger postcondition. It may
  be assigned to, destroyed, and used by operations whose preconditions hold.
  "Never use after move" is too absolute.
- Prefer the standard term forwarding reference rather than "universal
  reference."
- Avoid `std::move` on a local return value when it can inhibit NRVO. This is a
  performance/design concern, not a blanket statement that all return moves are
  wrong.
- Defining one ownership-related special member can suppress or change
  generation of others. The Rule of Five is a review heuristic; explicitly
  defaulting or deleting the correct operations is often better than blindly
  implementing all five.
- Rule of Zero is the preferred destination: compose standard owners and let
  their special members express the class semantics.

## 6. Merged Concept Map

### Resource And Lifetime Model

- A resource is something that must be released or returned: memory, file,
  descriptor, socket, lock, transaction, registration, or library handle.
- Storage duration, object lifetime, and ownership are separate concepts.
- Ownership is responsibility for ending a resource lifetime.
- Borrowing grants temporary access without cleanup responsibility.
- Aliasing means multiple handles can refer to one object; aliases do not
  automatically share ownership.
- Every owning API must define acquisition, release, transfer, failure, and
  null/empty state.

### RAII

- A resource handle is an object whose invariant records ownership.
- Acquisition or adoption establishes the invariant.
- The destructor releases the resource exactly once.
- Scope exit handles normal return, early return, and exception unwinding.
- Member destruction in reverse construction order composes cleanup.
- Constructors that fail do not produce an object; already constructed members
  clean themselves up.
- Destructors should not emit failures. Provide an explicit operation when the
  caller must observe close/flush/commit errors.

### Copy, Move, And Special Members

- Compiler-generated copying is memberwise; whether it is semantically shallow
  or deep depends on member types and the class contract.
- A raw owning pointer copied memberwise creates duplicate ownership and likely
  double deletion.
- Deep copy creates an independent resource when value semantics require it.
- Move transfers or reconstructs the resource relationship and leaves the
  source valid under its documented moved-from contract.
- Rule of Three: custom destructor, copy constructor, or copy assignment often
  implies reviewing all three.
- Rule of Five: adding move construction and move assignment extends that
  review for movable resource owners.
- Rule of Zero: standard value and owner members should make custom special
  members unnecessary.
- Copy and move operations may be defaulted, deleted, or implemented according
  to value, unique-owner, or shared-owner semantics.

### Ownership Types

- Value member/container: simplest ownership; prefer when practical.
- `std::unique_ptr<T>`: exclusive, movable ownership and default heap-owner
  choice.
- `std::shared_ptr<T>`: explicit shared lifetime through one control block.
- `std::weak_ptr<T>`: non-owning observation of a shared-lifetime object.
- Raw pointer/reference: normally a non-owning view; lifetime must be stated.
- Custom deleter: adapts the owner to `fclose`, a C destroy function, descriptor
  close, or another release API.

### Exception And Failure Safety

- No guarantee: state/resource integrity may be lost; reject in production
  interfaces.
- Basic guarantee: invariants remain valid and no resources leak.
- Strong guarantee: operation succeeds or observable state remains unchanged.
- No-throw guarantee: operation does not emit an exception.
- RAII supplies cleanup safety but the surrounding operation must still define
  its state guarantee.
- `noexcept` must reflect implementation truth. It affects termination behavior
  and can influence container relocation choices.

## 7. C Usage

C uses explicit ownership conventions:

- `malloc`/`calloc`/`realloc` pair with `free`;
- create/destroy APIs pair opaque handles;
- init/deinit APIs manage caller-provided storage;
- cleanup labels centralize release on multi-step failure;
- ownership transfer must be documented in names, comments, and API contracts;
- a wrapper struct can track handle validity but C has no automatic destructor.

Required C comparison points:

- `malloc` allocates raw storage and does not construct C++ objects;
- `realloc` has no direct general C++ object equivalent;
- every intermediate failure path must release already acquired resources;
- use typed status values and preserve the original allocation on failed
  `realloc`;
- do not pass C++ exceptions through a C ABI boundary.

The C treatment is language and user-space API material only. It must not
introduce kernel-driver content.

## 8. C++ Usage

Primary downstream examples should use values and standard owners first:

- `std::vector<std::byte>` or `std::string` instead of owning arrays;
- `std::unique_ptr` for exclusive polymorphic or optional heap ownership;
- `std::shared_ptr` only when independent parties truly co-own lifetime;
- `std::weak_ptr` for shared-lifetime observation;
- references/raw pointers for documented borrows;
- a custom movable RAII wrapper for a non-memory handle;
- `std::lock_guard` or `std::unique_lock` for mutex ownership;
- Rule-of-Zero classes for ordinary application types.

At least one small manual owner may be shown to teach the Rule of Five, but it
must be contrasted immediately with a Rule-of-Zero replacement and tested for
copy, move, self-assignment, and failure safety.

Function signatures must communicate intent:

- `T&` / `const T&`: required borrow;
- `T*` / `const T*`: nullable borrow;
- `std::unique_ptr<T>` by value: ownership transfer;
- `std::shared_ptr<T>` by value: share or retain ownership;
- `const std::unique_ptr<T>&` and `const std::shared_ptr<T>&`: rarely ideal when
  only the pointed-to object is needed; prefer `T&`/`T*`.

## 9. Embedded And Enterprise Usage

### Embedded Usage

- Prefer static storage, automatic storage, fixed-capacity containers, pools,
  or arenas when runtime allocation is prohibited or nondeterministic.
- RAII remains useful without heap allocation: lock guards, interrupt-state
  guards, peripheral-session guards, registrations, and scoped mode changes.
- Do not claim that disabling exceptions removes the need for RAII.
- Allocation policy must define startup-only allocation, failure behavior,
  fragmentation limits, latency, and ownership.
- Avoid `shared_ptr` by default in constrained or real-time paths; justify
  control-block allocation, reference-count operations, and lifetime model.
- A custom deleter can adapt a vendor C handle without exposing manual cleanup
  throughout the application.
- Destructors used in time-critical scopes need bounded, documented cleanup.
- No kernel allocation, kernel lock, or device-driver material.

### Enterprise Usage

- Model ownership in interfaces and code review, not only in comments.
- Prefer Rule-of-Zero domain types and standard containers.
- Use `unique_ptr` for implementation ownership and polymorphic factories.
- Use `shared_ptr` only for genuine shared lifetime, not as a default nullable
  pointer or dependency-injection mechanism.
- Review asynchronous work for captured references and ownership extension.
- Define shutdown order and prevent shared-ownership cycles.
- Keep C-library handles behind narrow RAII adapters.
- Specify exception guarantees and moved-from postconditions where callers need
  them.
- Use pImpl when compile-time isolation or ABI stability justifies the extra
  allocation and indirection.
- Measure allocation rate, peak memory, fragmentation, locality, and
  reference-count contention on the actual workload.

## 10. Required Comparisons

### `malloc/free` Versus `new/delete`

| Aspect | `malloc/free` | `new/delete` | Preferred C++ direction |
| --- | --- | --- | --- |
| API | C library functions | C++ expressions | Standard container or RAII owner |
| Storage | Raw bytes | Storage plus initialization/destruction | Value/resource-handle abstraction |
| Type | `void*` result | Typed result | Encapsulated type |
| Failure | Null result | Usually throws `std::bad_alloc`; nothrow form returns null | Project policy at recovery boundary |
| Resize | `realloc` for suitable C storage | No general object-resize pair | Container operation |
| Pairing | `malloc/calloc/realloc` with `free` | `new` with `delete`, `new[]` with `delete[]` | Avoid naked pairs |
| Mixing | Undefined behavior | Undefined behavior | Never mix families |

### Raw Pointer Versus Smart Pointer

| Question | Raw pointer/reference | Smart pointer |
| --- | --- | --- |
| Primary meaning | Usually borrow/view | Ownership policy |
| Cleanup | None inherent | Destructor-driven |
| Nullability | Pointer may be null; reference not normally null | Smart pointer may be empty |
| Copy semantics | Copies address | Type-specific ownership semantics |
| Main risk | Dangling/unclear lifetime | Wrong ownership model, cycles, escaped borrow |
| Best use | Parameters, observers, low-level interop | Owning members and transfer boundaries |

### `unique_ptr` Versus `shared_ptr` Versus `weak_ptr`

| Aspect | `unique_ptr` | `shared_ptr` | `weak_ptr` |
| --- | --- | --- | --- |
| Ownership | Exclusive | Shared | None |
| Copy | No | Yes | Yes |
| Move | Yes | Yes | Yes |
| Lifetime effect | Sole owner | Extends until last owner | Does not extend |
| Extra state | Deleter may affect size | Control block and reference counts | Observes control block |
| Typical use | Default dynamic owner, factory result | Genuine independent co-owners | Cache/observer/back-reference |
| Main hazard | Escaped borrow, incorrect `release()` | Cycles, overhead, false sharing of ownership | TOCTOU if not using `lock()` |

### Manual Cleanup Versus RAII

| Aspect | Manual cleanup | RAII |
| --- | --- | --- |
| Normal return | Explicit release | Destructor |
| Early return | Every path must release | Destructor |
| Exception | Catch/cleanup logic required | Stack unwinding destroys owners |
| Composition | Cleanup order coded manually | Reverse member/local construction order |
| Review burden | Path-sensitive | Invariant and owner-type focused |
| Main failure | Leak, double release, forgotten path | Incorrect wrapper contract or escaped borrow |

### Shallow Versus Deep Copy

| Aspect | Shallow/memberwise handle copy | Deep semantic copy |
| --- | --- | --- |
| Resource | Same address/handle may be duplicated | Independent resource created |
| Ownership | Unsafe for raw unique ownership | Suitable for value semantics |
| Cost | Often lower | Allocation/copy may be expensive |
| Aliasing | Preserved | Removed |
| Failure policy | May create double ownership | Copy must preserve source and guarantees |
| Alternative | Delete copy or use shared ownership deliberately | Use value members/containers |

### Rule Of Three Versus Rule Of Five Versus Rule Of Zero

| Rule | Trigger | Required review | Preferred use |
| --- | --- | --- | --- |
| Rule of Three | Custom destructor/copy operation in pre-move ownership design | Destructor, copy constructor, copy assignment | Legacy/manual copyable owner |
| Rule of Five | Resource owner participates in move semantics | Three above plus move constructor and move assignment | Low-level owner that cannot use standard member owners |
| Rule of Zero | Members already manage their resources | Defaulted compiler behavior and class semantics | Default application design |

## 11. Common Bugs And Review Risks

- Leak on early return, exception, or partial acquisition.
- Double delete/free/close caused by duplicated ownership.
- Use-after-free through a raw alias after an owner resets or moves.
- Mismatched allocation/deallocation family.
- `new[]`/`delete` or `new`/`delete[]` mismatch.
- Memberwise copy of a raw unique owner.
- Copy assignment that releases current state before replacement succeeds.
- Move operation that leaves two apparent owners or an invalid source.
- Incorrect self-move/self-assignment handling.
- Throwing cleanup or destructor.
- False `noexcept` causing `std::terminate`.
- `shared_ptr` cycle.
- Multiple control blocks for one object.
- Constructing `shared_ptr(this)` instead of using a valid ownership origin.
- Calling `release()` and losing the returned raw ownership.
- Keeping a pointer returned by `get()` beyond the owner lifetime.
- Treating `weak_ptr::expired()` as a lifetime lock.
- Assuming shared ownership makes the object thread-safe.
- Unbounded allocation or destruction in a real-time path.
- Incorrect alignment, missing destruction, or lifetime misuse with placement
  new.
- C `realloc` overwriting the only pointer before checking failure.
- pImpl destructor instantiated where the implementation type is incomplete.
- Custom deleter calling the wrong release function or lacking required state.

## 12. Debugging And Verification Notes

### Compiler And Static Analysis

- Enable strict warnings and review special-member generation/deletion.
- Use clang-tidy checks such as `modernize-make-unique`,
  `modernize-use-override`, `cppcoreguidelines-owning-memory`, and relevant
  bug-prone checks where available.
- Inspect whether a type is copy/move constructible and nothrow movable with
  type traits and `static_assert`.
- Review ownership annotations and naming at C boundaries.

### Sanitizers And Runtime Tools

- AddressSanitizer: use-after-free, double free, invalid access, and some
  lifetime errors on executed paths.
- LeakSanitizer: leaked allocations on supported platforms.
- UndefinedBehaviorSanitizer: selected lifetime/alignment/invalid-operation
  defects.
- Valgrind or platform heap tools: leak and invalid-access investigation where
  supported.
- ThreadSanitizer: races involving the managed object or shared ownership logic;
  reference counting alone does not remove data races.

Suggested host build:

```bash
c++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
    -Wnon-virtual-dtor -Werror -O1 -g3 \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    resource_example.cpp -o resource_example
```

### Debugger And Tests

- Break on constructors, destructors, copy/move operations, deleters, and C
  release functions.
- Inspect owner addresses separately from managed-object addresses.
- Exercise normal return, every early return, acquisition failure, thrown
  operations, move, reset, and destruction order.
- Run copy/move/self-assignment tests for manual owners.
- Test that moved-from objects support only their documented operations.
- Test shared cycles and `weak_ptr::lock()` after expiration.
- Count acquisitions and releases in fakes; require exactly-once release.
- Test close/commit failure separately from destructor fallback.
- Sanitizer success covers executed paths only and is not proof of ownership
  correctness.

## 13. Best Practices

- Prefer values, standard containers, and Rule-of-Zero types.
- Represent ownership explicitly and keep borrowing separate.
- Use one resource handle per independently released resource.
- Prefer scoped objects over dynamic allocation.
- Prefer `std::make_unique` and `std::unique_ptr` for exclusive dynamic
  ownership.
- Introduce `shared_ptr` only after identifying real co-owners.
- Use `weak_ptr::lock()` for temporary access to shared-lifetime objects.
- Keep raw `new`/`delete` inside low-level owner implementations, if present at
  all.
- Immediately place an acquired resource under an RAII owner.
- Match custom deleters to the acquisition API.
- Make cleanup non-throwing and provide explicit error-reporting cleanup where
  needed.
- Mark move operations `noexcept` only when true.
- Leave moved-from objects valid and document stronger postconditions only
  when guaranteed.
- Prefer defaulted/deleted special members over repetitive hand-written code.
- State exception guarantees for mutating operations.
- Avoid hidden allocation and shared ownership in bounded-latency paths.
- Measure rather than assume allocator or smart-pointer performance.

## 14. Interview Angles

### Beginner

- Define RAII and explain why it manages more than memory.
- Compare stack/scoped lifetime with dynamic allocation without relying on a
  fixed process layout.
- Why must allocation and deallocation families match?
- What are ownership and borrowing?
- Compare raw pointers with `unique_ptr`.
- Why is a destructor part of the resource contract?

### Mid-Level

- Explain Rule of Three, Five, and Zero.
- Diagnose a shallow-copy double-delete bug.
- Implement or review copy-and-swap versus allocate-before-commit.
- Explain what `std::move` does and does not do.
- Describe valid but unspecified moved-from state.
- Compare `unique_ptr`, `shared_ptr`, and `weak_ptr`.
- Explain why `weak_ptr::lock()` matters.
- Design a custom deleter for a C resource.
- Explain how RAII behaves during partial construction and exceptions.

### Senior

- Review an API and assign ownership semantics to every parameter and return.
- Explain when shared ownership is justified and how to prevent cycles.
- Discuss `make_shared` allocation/lifetime trade-offs.
- Explain smart-pointer thread-safety boundaries.
- Design a no-throw destructor plus explicit failure-reporting close/commit.
- State basic, strong, and no-throw guarantees for a resource-owning operation.
- Explain when a move should be conditional `noexcept`.
- Design pImpl with `unique_ptr` and an incomplete implementation type.
- Choose among static storage, pool, arena, PMR, and general dynamic allocation
  for an embedded or high-throughput workload.
- Explain why passing `shared_ptr` by value is sometimes correct and sometimes
  unnecessary.

### Interview Traps

- "`std::move` moves the object."
- "A moved-from object cannot be used."
- "`shared_ptr` makes the object thread-safe."
- "`weak_ptr::expired()` safely reserves the object."
- "`unique_ptr` is always exactly one pointer."
- "Always catch `bad_alloc` next to `new`."
- "Set every deleted pointer to null and dangling pointers are solved."
- "Every move operation must be `noexcept` even if it can throw."
- "Rule of Five means hand-write all five functions."
- "RAII is only for heap memory or exceptions."

## 15. Practice Tasks

### Basic

1. Replace a raw owning array with `std::vector`.
2. Convert a raw factory return to `std::unique_ptr`.
3. Mark raw pointer/reference parameters as borrows in an API review.
4. Find every cleanup path in a C function with three acquisitions.

### Intermediate

1. Implement a movable, non-copyable RAII wrapper for a fake C handle.
2. Add a stateful custom deleter and observe `unique_ptr` size.
3. Demonstrate a `shared_ptr` cycle, then break it with `weak_ptr`.
4. Write a deep-copying buffer with a strong-guarantee assignment operation,
   then replace it with a Rule-of-Zero design.
5. Test partial construction where the second owned member fails.

### Advanced

1. Design pImpl ownership with an out-of-line destructor.
2. Compare `make_shared` with separate control-block/object allocation and an
   outstanding `weak_ptr`.
3. Build a fixed-capacity arena-backed example and document destruction order,
   alignment, failure, and reset policy.
4. Review a callback or asynchronous task for ownership extension and dangling
   captures.
5. Specify exception and move guarantees for a resource-owning container-like
   type.

## 16. Gaps And External Validation Needs

### Mapped Source Gaps

- Rule of Three is implied by raw-owner examples but not taught clearly as a
  named rule.
- pImpl is absent.
- Allocator concept and allocator-aware containers are absent.
- `std::pmr` and polymorphic allocators are absent.
- File-descriptor RAII is absent; mapped files use streams or `FILE*`.
- Mutex guards are shown but ownership transfer and lock policy are shallow.
- Custom allocator, memory pool, arena, and small-object optimization receive
  little or no reliable treatment.
- Exact incomplete-type constraints for `unique_ptr` are absent.
- Smart-pointer aliasing constructors and owner identity are absent.

### External Validation Required For Downstream Outputs

- cppreference or ISO C++ for:
  - object lifetime, storage duration, `new`/`delete`, and placement new;
  - implicit generation/deletion of special members;
  - moved-from library-type guarantees;
  - `noexcept`, `move_if_noexcept`, and container relocation behavior;
  - `unique_ptr`, custom deleters, incomplete types, and array specialization;
  - `shared_ptr` control blocks, aliasing, thread-safety, and
    `enable_shared_from_this`;
  - `weak_ptr::lock`, expiration, and owner identity;
  - allocator-aware containers and `std::pmr`.
- C++ Core Guidelines for ownership, RAII, naked allocation, smart-pointer
  parameters, Rule of Zero, and resource-handle design.
- POSIX or platform documentation only if a user-space file-descriptor wrapper
  is used. Keep that example about RAII and cleanup, not driver behavior.
- Compiler sanitizer documentation for exact supported checks and limitations.

## 17. Downstream Output Targets

### Knowledge

`knowledge/10-resource-management-in-cpp.md`

Required order:

1. goal;
2. why it matters;
3. lifetime and ownership mental model;
4. RAII mechanism;
5. copy/move and special members;
6. C/C++ allocation APIs;
7. smart pointers and custom deleters;
8. practical non-memory resource wrappers;
9. embedded and enterprise usage;
10. required comparisons;
11. bugs;
12. debugging;
13. best practices;
14. interview readiness;
15. practice.

### Interview

`interview/10-resource-management-in-cpp.md`

Must include beginner, mid-level, and senior questions, coding tasks, debugging
scenarios, and answers anchored in explicit ownership and cleanup paths.

### Examples

`examples/10-resource-management-in-cpp/README.md`

Useful minimal examples:

- Rule-of-Zero versus manual Rule-of-Five buffer;
- movable non-copyable C-handle wrapper with custom deleter;
- `unique_ptr` transfer and borrow boundaries;
- `shared_ptr` cycle repaired with `weak_ptr`;
- sanitizer-driven leak/use-after-free demonstration kept separate from safe
  normal runs.

### Review Gates

- Every resource has one documented owner or an explicit shared-lifetime model.
- Every borrow has a lifetime precondition.
- Normal, early-return, exception, partial-construction, move, and destruction
  cleanup paths are explained.
- All six master comparisons are present.
- Manual allocation examples are clearly educational and are followed by RAII
  alternatives.
- No false portability claims about stack/heap layout or smart-pointer size.
- No claim that smart pointers prevent every dangling reference.
- No claim that reference counting makes the managed object thread-safe.
- No unconditional `noexcept` advice for operations that can throw.
- No Linux Device Driver or kernel-driver material.

