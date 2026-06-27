# Topic Brief 13 - Error Handling

## 1. Canonical Routing

| Field | Validated value |
| --- | --- |
| `learning_path_number` | `13` |
| Title | Error Handling |
| `slug` | `error-handling` |
| Requested topic | C and C++ error handling: return/status codes, `errno`, assertions, exceptions, exception safety, RAII cleanup, `noexcept`, stack unwinding, file I/O error states, and `std::expected`/Result-style APIs |
| Master source | `master-ch14` |
| Required Notion sources | `notion-8-1`, `notion-8-2`, `notion-8-3`, `notion-9-1` |
| Topic Brief | `coverage/topic-briefs/13-error-handling.md` |
| Knowledge target | `knowledge/13-error-handling.md` |
| Interview target | `interview/13-error-handling.md` |
| Example target | `examples/13-error-handling/README.md` |

Validation result: the number, title, slug, master source, four mapped Notion
sources, and canonical output paths match `LEARNING_PATH.md`.

This step creates the Topic Brief only. It does not create or modify knowledge,
interview, or example outputs.

## 2. Source Coverage

### Internal Sources Read

| Source label | Path | Coverage contribution |
| --- | --- | --- |
| `master-ch14` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, CH14 | MUST priority, CH07/CH12 prerequisites, required keywords, required comparisons, embedded exception restriction rule, and interview focus |
| `master-comparison-rule` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, comparison sections | Required compact C vs C++ vs enterprise/embedded comparison table for error-handling topics |
| `guide-section-03` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 3 | MUST-topic deep output requirements |
| `guide-section-07` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 7 | Depth control and practical-example preference |
| `guide-section-10` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 10 | Clear technical English, concise Markdown, compile-oriented examples, and warnings for unsafe APIs |
| `guide-section-11` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 11 | Lesson type routing for later full lesson, interview pack, examples, and comparison note |
| `guide-section-14` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 14 | Final quality checklist |
| `notion-index` | `docs/C++ Notion/C++ Programming.md` | Notion inventory and mapped chapter identity validation |
| `notion-8-1` | `docs/C++ Notion/Chapter 8-1 Exception Handling - Basics & Standard Exception.md` | Exception purpose, `try`/`catch`/`throw`, propagation, uncaught exceptions, standard exception hierarchy, custom exceptions, catch ordering, catch-all, rethrowing, and catch-by-const-reference |
| `notion-8-2` | `docs/C++ Notion/Chapter 8-2 Exception Handling - Exception Safety & RAII.md` | Exception safety guarantees, copy-and-swap, RAII cleanup, smart pointers, file streams, lock guards, custom RAII wrappers, `noexcept` moves, exceptions vs error codes, and interview questions |
| `notion-8-3` | `docs/C++ Notion/Chapter 8-3 Exception Handling - noexcept, Stack Unwinding.md` | `noexcept`, conditional `noexcept`, stack unwinding, partial construction, destructors during exceptions, destructor throwing hazards, use/avoid exception guidance, and exception-safety checklist |
| `notion-9-1` | `docs/C++ Notion/Chapter 9-1 File Handling - Basics to Advanced Operations.md` | File-stream RAII, checking `is_open()`, `good()`, `eof()`, `fail()`, `bad()`, `clear()`, stream exception masks, and file-operation error-handling interview angles |

All four mapped Notion chapter files were inspected. No mapped Notion source was
skipped.

### External References Consulted

Accessed on 2026-06-27.

| Label | Reference | Validation purpose |
| --- | --- | --- |
| `external-cppreference-exceptions` | cppreference exceptions: <https://www.cppreference.com/cpp/language/exceptions> | Exception object, handler matching, stack unwinding, exception specifications, and termination behavior around exception handling failures |
| `external-cppreference-terminate` | cppreference `std::terminate`: <https://cppreference.com/cpp/error/terminate> | Exact situations where exception handling failure calls `std::terminate`, including uncaught exceptions and exceptions during exception handling |
| `external-cppreference-expected` | cppreference `std::expected`: <https://en.cppreference.com/cpp/utility/expected> | C++23 expected/unexpected vocabulary type constraints and exact library status |
| `external-cppreference-header-expected` | cppreference `<expected>` header: <https://en.cppreference.com/cpp/header/expected> | C++23 header contents: `expected`, `unexpected`, `bad_expected_access`, `unexpect_t` |
| `external-core-guidelines-error-handling` | C++ Core Guidelines, error handling section: <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines> | Guideline-level policy: prefer exceptions and RAII where exceptions are available; use status codes when exceptions are unavailable or inappropriate |

### Source Coverage Status

`TOPIC_BRIEF_COMPLETE_WITH_EXTERNAL_VALIDATION`: canonical routing, mapped
master chapter, guide requirements, every mapped Notion source, cppreference and
C++ Core Guidelines validation, merged concepts, required comparisons, common
bugs, debugging notes, best practices, interview angles, gaps, and output
targets are recorded.

The internal sources provide strong coverage for C++ exception mechanics,
exception safety, RAII, `noexcept`, stack unwinding, and file-stream errors.
Downstream learner-facing output should still validate exact standard wording
against cppreference when describing `std::expected`, stream exception masks,
and `std::terminate` behavior.

## 3. Priority And Dependencies

- Overall priority: `MUST`.
- Required depth: deep explanation with examples, comparisons, common bugs,
  debugging workflow, best practices, interview preparation, practice tasks, and
  trusted references.
- Master prerequisites:
  - CH07, Industrial C Practices, for defensive programming, return-code
    discipline, `errno`, assertions, warnings, sanitizers, and code-review
    habits.
  - CH12, Modern C++ And Templates, for `std::optional`, `std::expected`,
    `noexcept`, move semantics, smart pointers, and vocabulary types.
- Practical prerequisites:
  - RAII, constructors/destructors, object lifetime, ownership, and smart
    pointers.
  - Basic C APIs that report status through return values and `errno`.
  - Standard exception hierarchy and standard library containers/streams.
  - File I/O stream state flags.
- Follow-on topics:
  - Topic 10, Resource Management In C++, for deeper ownership and RAII design.
  - Topic 11, STL And Standard Library, for library error behavior and stream
    state.
  - Topic 14, Concurrency, for exceptions across thread boundaries and
    termination behavior.
  - Topic 15/16, comparison topics, for C API wrappers and POSIX-style errors.

## 4. Scope And Depth Boundaries

### Deep In This Topic

- C-style error reporting: return codes, status codes, error enums, `errno`,
  `perror`, `strerror`, and disciplined caller checks.
- Assertion policy: `assert` for programmer bugs/debug invariants and
  `static_assert` for compile-time constraints; never teach assertions as a
  substitute for recoverable runtime error handling.
- C++ exceptions: `try`, `catch`, `throw`, propagation, handler matching,
  catch-all, rethrowing, custom exceptions, standard exception hierarchy, and
  catch-by-const-reference.
- Exception safety: no guarantee, basic guarantee, strong guarantee, and
  no-throw guarantee.
- RAII cleanup: stack objects, smart pointers, streams, locks, custom wrappers,
  and cleanup during stack unwinding.
- Stack unwinding: reverse-order destruction, partial construction behavior, and
  the danger of throwing destructors during unwinding.
- `noexcept`: semantic promise, optimization/documentation role, conditional
  `noexcept`, move operations, swap, destructors, and `std::terminate` risk when
  the promise is violated.
- File I/O error handling: `is_open()`, stream truthiness, `good()`, `eof()`,
  `fail()`, `bad()`, `clear()`, exception masks, and distinguishing EOF from
  actual read/format/hardware errors.
- Modern explicit-result APIs: `std::optional` for absence without detailed
  reason, `std::expected<T,E>`/`Result<T,E>` for value-or-error APIs, and
  domain-specific error models.

### Medium In This Topic

- Error categories and domain-specific error enums/classes.
- Exception hierarchy design for application domains.
- Constructor failure versus factory-return failure.
- Copy-and-swap and transactional update patterns.
- Embedded/project-policy tradeoffs when exceptions are disabled.
- File APIs that use stream exceptions versus explicit stream-state checks.

### Controlled Awareness

- Monadic error handling with `std::expected` and similar Result types.
- ABI/binary-size and real-time concerns around exceptions.
- Mixed C/C++ boundary wrappers that translate status codes into C++ APIs.
- Exception policy documentation in enterprise codebases.

### Defer Or Exclude

- Full resource-management lesson: route to topic 10.
- Full STL/container behavior lesson: route to topic 11.
- Full concurrency/thread exception handling: route to topic 14.
- Full POSIX/Linux API comparison: route to topic 16.
- Linux Device Driver, kernel-driver, Yocto, GStreamer, AUTOSAR, or unrelated
  platform material.

## 5. Merged Concept Map

- Errors must be classified before choosing a mechanism:
  - Programmer bug/invariant violation: `assert`, `static_assert`, contract-like
    checks, tests, and fail-fast diagnostics.
  - Expected operational failure: return code, status enum, `std::optional`, or
    `std::expected`/Result.
  - Exceptional runtime failure: exception, especially when recovery is handled
    at a higher layer or construction cannot produce a valid object.
  - Non-recoverable failure: terminate, abort, reset, or controlled shutdown
    according to system policy.
- C-style APIs require disciplined status checking. Ignoring return values or
  reading `errno` after a successful call creates false diagnostics.
- C++ exceptions separate normal control flow from error flow, but require RAII
  and exception-safe invariants.
- RAII is the bridge between exceptions and correctness: cleanup belongs in
  destructors, not repeated at each error branch.
- Exception safety is a property of state transitions, not just "does it catch".
  A function must define what remains true if an exception interrupts it.
- `noexcept` is both a contract and an optimization signal. It is especially
  important for destructors, deallocation, swap, and move operations.
- File-stream error handling is a concrete place where explicit status checks
  and exception-based error handling both appear in real C++ code.

## 6. Required Comparisons

| Topic | C | C++ | Enterprise / Embedded Guidance |
| --- | --- | --- | --- |
| Return code vs exception | Return status through `int`, enum, pointer output, or `errno`; caller must check every call | Throw typed exception and handle at a suitable boundary; propagation is automatic | Use exceptions for rare, exceptional failures when project policy allows. Use explicit status for expected failures, C ABI, hard real-time, or no-exception builds |
| `errno` vs exception | Global/thread-local diagnostic set by many C/POSIX-style APIs; valid only when the API documents it and reports failure | Exception object carries type and message/context; standard exceptions derive from `std::exception` | Do not blindly translate every `errno` into an exception. Preserve operation, path/device, code, and recovery context |
| `assert` vs runtime error | `assert` checks debug-time programmer assumptions and may disappear under `NDEBUG` | Runtime validation reports recoverable failures through exceptions or explicit results | Never use `assert` for user input, file availability, allocation failure, communication errors, or safety-critical runtime checks |
| Exception vs `std::expected` / Result type | C has no standard vocabulary result type; projects use structs/enums/output params | Exceptions are implicit control transfer; `std::expected<T,E>` is explicit value-or-error return in C++23 | Prefer `expected`/Result for expected, local, frequent failures and API clarity. Prefer exceptions for rare failures crossing many stack frames or constructor failure |
| Manual cleanup vs RAII cleanup | Cleanup must be manually repeated on success and each failure path | Destructors release resources automatically on normal return and unwinding | Wrap every owning resource: memory, file descriptors, streams, locks, transactions, and handles. Manual cleanup is a code-review smell |

When to use C style:

- C ABI or mixed C/C++ boundary.
- Embedded or safety profile disables exceptions.
- Failure is expected/frequent and close to the caller.
- Tight loops, hard real-time, or deterministic latency requirements.

When to use C++ style:

- Constructor cannot create a valid object.
- Failure is exceptional and rare.
- Error should propagate across several layers.
- RAII protects every acquired resource.
- Typed exceptions improve recovery decisions.

Common comparison bug: mixing mechanisms without policy, such as returning a
status code but also throwing for the same failure class, or catching an
exception and converting it to `false` while losing the diagnostic context.

## 7. Common Bugs And Corrections

- Ignoring return codes from C-style APIs.
  Correction: check immediately, preserve the failing operation and code, and
  avoid continuing with invalid state.
- Reading `errno` without first confirming the API failed.
  Correction: consult `errno` only when the API contract says it is meaningful.
- Using `assert` for recoverable runtime errors.
  Correction: use runtime validation and an explicit error channel.
- Throwing raw strings, integers, or unrelated types.
  Correction: throw meaningful exception types, usually derived from
  `std::exception` or a project base exception.
- Catching exceptions by value.
  Correction: catch by `const&` to avoid slicing and unnecessary copies.
- Catching `std::exception` before a derived exception.
  Correction: order handlers from most specific to most general.
- Catching and swallowing errors.
  Correction: handle, translate with context, or rethrow; do not silently hide.
- Throwing from a destructor.
  Correction: destructors must not fail outward; log, store status, or expose an
  explicit `close()`/`commit()` that can report failure before destruction.
- Marking a function `noexcept` while it calls throwing code.
  Correction: use `noexcept` only when all operations are known not to throw, or
  catch internally and satisfy the promise.
- Losing exception safety during mutation.
  Correction: acquire/copy first, then commit with no-throw operations such as
  swap.
- Manual cleanup in multiple branches.
  Correction: use RAII wrappers and standard types (`std::vector`,
  `std::unique_ptr`, streams, lock guards).
- Treating EOF as a read error.
  Correction: inspect `eof()`, `fail()`, and `bad()` separately and design loop
  conditions around extraction success.

## 8. Debugging Notes

- For C-style code, log the failing API, input parameters, return value, and
  saved `errno` value immediately after failure.
- For exception-based code, inspect the dynamic exception type and `what()`;
  add contextual translation at module boundaries.
- Use compiler warnings and static analysis to catch ignored return values,
  unreachable catch blocks, throwing destructors, and incorrect `noexcept`.
- Use sanitizers to expose memory bugs that often masquerade as error-handling
  failures: AddressSanitizer, UndefinedBehaviorSanitizer, and leak checks.
- In debuggers, break on throw/catch or on `std::terminate` to identify the
  original throw site and the boundary where handling failed.
- For file streams, print stream flags (`good`, `eof`, `fail`, `bad`) before
  clearing state; after `clear()`, retry only if the underlying cause is fixed.
- For exception safety, review every operation as "what if this allocation,
  copy, parse, lock, or write throws here?"
- For embedded/no-exception builds, trace status-code propagation and verify
  that callers cannot accidentally ignore fatal errors.

## 9. Best Practices

- Define an error policy per module: exceptions, explicit results, status codes,
  or a boundary translation between them.
- Use RAII for every owned resource before introducing exceptions.
- Maintain at least the basic exception guarantee for production code; provide
  the strong guarantee when mutation should be transactional.
- Keep destructors, deallocation, move operations, and swap operations no-throw
  whenever possible.
- Prefer typed exceptions with clear domain meaning over generic strings.
- Catch exceptions at meaningful boundaries: thread entry points, command
  handlers, request handlers, main loops, and API boundaries.
- Add context while translating errors, but preserve the original failure
  category.
- Use `std::optional` only when absence is enough information.
- Use `std::expected<T,E>`/Result when the caller should inspect and react to
  known failure reasons.
- In embedded systems where exceptions are restricted or disabled, combine
  predictable return/status codes with RAII for cleanup.
- Document thrown exceptions or explicit error variants as part of the API
  contract.

## 10. Interview Angles

- Explain return code vs exception and give cases where each is preferable.
- Why do many embedded or real-time projects avoid exceptions?
- What are the basic, strong, and no-throw exception safety guarantees?
- How does RAII make exception-safe code simpler?
- What happens during stack unwinding?
- What happens if a destructor throws while another exception is active?
- What does `noexcept` mean, and what happens if a `noexcept` function throws?
- Why should move constructors and swap often be `noexcept`?
- Why should exceptions be caught by `const&`?
- How would you design a file-loading API: exception, status code, or
  `std::expected`?
- What is the difference between `assert` and runtime error handling?
- How do stream state flags distinguish EOF, format failure, and serious I/O
  failure?

## 11. Practice Targets For Later Outputs

- Implement a small C-style parser that returns an error enum and output value.
- Wrap a C file handle or device handle in a move-only RAII class.
- Convert a manual-cleanup function into RAII-based exception-safe code.
- Write a `load_config()` API in three styles: exception, `std::optional`, and
  `std::expected`/Result.
- Demonstrate basic versus strong exception guarantee with a container-like
  class.
- Show how `noexcept` on move operations changes `std::vector` reallocation
  behavior.
- Build a file-copy example that checks stream state and reports useful errors.
- Add unit tests for success, expected failure, exceptional failure, and cleanup
  paths.

## 12. Gaps And Uncertainties

- The Notion chapters are C++-heavy. C-only `errno`, `perror`, `strerror`, and
  status-code discipline need external C/POSIX or cppreference C validation
  before writing a C-focused lesson section.
- `std::expected` is C++23. Downstream output must clearly label it by standard
  version and provide a project-local `Result<T,E>` alternative for earlier
  C++ standards.
- Stream exception-mask teaching should be precise: enabling exceptions on
  `eofbit` is often undesirable unless EOF is truly exceptional.
- Some Notion examples are teaching examples and should be refined before
  learner-facing docs, especially examples that use `using namespace std`, raw
  `new`, or simplified custom RAII wrappers.
- Exact behavior of unwinding before `std::terminate` can be implementation- or
  situation-dependent; use cppreference wording carefully in final lessons.

## 13. Suggested Next Outputs

- `knowledge/13-error-handling.md`: full learner-facing lesson with C/C++
  comparison tables, exception-safety examples, RAII cleanup, file-state
  debugging, and embedded/no-exception guidance.
- `interview/13-error-handling.md`: beginner, mid-level, and senior Q&A with
  debugging scenarios and API-design tradeoffs.
- `examples/13-error-handling/README.md`: compile-oriented examples for
  status-code parser, RAII file wrapper, `noexcept` move behavior, and
  `expected`/Result-style config loading.
