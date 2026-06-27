# Topic Brief 14 - Concurrency

## 1. Canonical Routing

| Field | Validated value |
| --- | --- |
| `learning_path_number` | `14` |
| Title | Concurrency |
| `slug` | `concurrency` |
| Requested topic | C++ threads, synchronization, atomics, condition variables, semaphores, producer-consumer queues, thread pools, signal-aware shutdown, and POSIX pthread comparisons |
| Master source | `master-ch15` |
| Required Notion sources | `notion-10-8`, `notion-10-9`, `notion-10-7` |
| Topic Brief | `coverage/topic-briefs/14-concurrency.md` |
| Knowledge target | `knowledge/14-concurrency.md` |
| Interview target | `interview/14-concurrency.md` |
| Example target | `examples/14-concurrency/README.md` |

Validation result: the number, title, slug, master source, three mapped Notion
sources, and canonical output paths match `LEARNING_PATH.md`.

This step creates the Topic Brief only. It does not create or modify knowledge,
interview, or example outputs.

## 2. Source Coverage

### Internal Sources Read

| Source label | Path | Coverage contribution |
| --- | --- | --- |
| `master-ch15` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, CH15 | MUST priority, CH12 prerequisite, keywords, required comparisons, correctness-first expansion rule, and interview focus |
| `master-concurrency-checklist` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, concurrency checklist | Required topic checklist: thread, mutex, lock, condition variable, semaphore, atomic, race condition, deadlock, spurious wakeup, producer-consumer, and thread pool |
| `guide-section-03` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 3 | MUST-topic deep output requirements |
| `guide-section-04` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 4 | Full chapter structure for later learner-facing output |
| `guide-quality-rules` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md` and `C++-Programing/.agents/skills/c-cpp/SKILL.md` | Clear technical English, compile-oriented examples, source coverage, comparisons, practical embedded/enterprise framing, and no Linux Device Driver/kernel-driver material |
| `notion-index` | `docs/C++ Notion/C++ Programming.md` | Notion inventory and mapped chapter identity validation |
| `notion-10-8` | `docs/C++ Notion/Chapter 10-8 Multithreading Basics.md` | Thread creation, `join`, `detach`, `joinable`, thread IDs, hardware concurrency, race conditions, mutexes, `std::lock_guard`, `std::unique_lock`, thread-safe classes, atomic counters, scoped locking, and basic best practices |
| `notion-10-9` | `docs/C++ Notion/Chapter 10-9 Multithreading Advanced.md` | Condition variables, predicate waits, spurious wakeups, producer-consumer, timed waits, `std::promise`, `std::future`, `std::async`, `std::packaged_task`, deadlock prevention, `std::lock`, `std::scoped_lock`, thread pools, atomic operations, compare-exchange, atomic flags, spinlocks, and advanced best practices |
| `notion-10-7` | `docs/C++ Notion/Chapter 10-7 Signal Handling.md` | Signals, `std::signal`/`signal`, `raise`, POSIX `sigaction`, signal masks, async-signal-safe functions, `sig_atomic_t`, lock-free atomic flags, graceful shutdown, platform considerations, and signal-handler safety warnings |

All three mapped Notion chapter files were inspected. No mapped Notion source
was skipped.

### External References Consulted

Accessed on 2026-06-27.

| Label | Reference | Validation purpose |
| --- | --- | --- |
| `external-cppreference-thread` | cppreference `std::thread`: <https://en.cppreference.com/w/cpp/thread/thread> | Thread ownership, `joinable`, destructor termination behavior, `join`, `detach`, IDs, and hardware concurrency |
| `external-cppreference-mutex` | cppreference mutex library: <https://en.cppreference.com/w/cpp/thread> | Standard mutex, lock, condition variable, semaphore, future, and atomic vocabulary |
| `external-cppreference-condition-variable` | cppreference `std::condition_variable`: <https://en.cppreference.com/w/cpp/thread/condition_variable> | Predicate wait, notification, mutex association, and spurious-wakeup-safe waiting |
| `external-cppreference-atomic` | cppreference `std::atomic`: <https://en.cppreference.com/w/cpp/atomic/atomic> | Atomic operations, data-race-free access to shared objects, lock-free awareness, and memory-order vocabulary |
| `external-cppreference-semaphore` | cppreference `std::counting_semaphore`: <https://en.cppreference.com/w/cpp/thread/counting_semaphore> | C++20 semaphore behavior and comparison against POSIX `sem_t` |
| `external-posix-pthreads` | Linux man-pages `pthreads(7)`: <https://man7.org/linux/man-pages/man7/pthreads.7.html> | POSIX threading model and API-family comparison |
| `external-posix-pthread-create` | Linux man-pages `pthread_create(3)`: <https://man7.org/linux/man-pages/man3/pthread_create.3.html> | POSIX thread creation, join/detach attributes, and error-code style |
| `external-posix-pthread-mutex` | POSIX man page `pthread_mutex_lock(3p)`: <https://man7.org/linux/man-pages/man3/pthread_mutex_lock.3p.html> | `pthread_mutex_t` lock/unlock behavior and error-code comparison to RAII C++ locks |
| `external-posix-pthread-cond` | POSIX man page `pthread_cond_wait(3p)`: <https://man7.org/linux/man-pages/man3/pthread_cond_wait.3p.html> | Condition wait behavior, mutex release/reacquire, and predicate-loop comparison |
| `external-posix-semaphore` | Linux man-pages `sem_overview(7)`: <https://man7.org/linux/man-pages/man7/sem_overview.7.html> | POSIX unnamed/named semaphore model for comparison with `std::counting_semaphore` |
| `external-posix-signal-safety` | Linux man-pages `signal-safety(7)`: <https://man7.org/linux/man-pages/man7/signal-safety.7.html> | Async-signal-safe function constraints and signal-handler warnings |
| `external-core-guidelines-cp` | C++ Core Guidelines concurrency rules: <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#S-concurrency> | Guideline-level validation for avoiding data races, minimizing shared writable state, using RAII locks, and preferring higher-level synchronization where possible |

### Source Coverage Status

`TOPIC_BRIEF_COMPLETE_WITH_EXTERNAL_VALIDATION`: canonical routing, mapped
master chapter, expansion guide requirements, every mapped Notion source,
cppreference concurrency references, POSIX user-space API references, C++ Core
Guidelines validation, merged concepts, required comparisons, common bugs,
debugging notes, best practices, interview angles, gaps, and output targets are
recorded.

The internal sources provide strong coverage for beginner-to-mid C++
concurrency. External validation is required downstream for exact wording around
thread destructor termination, data races, memory ordering, condition-variable
waiting, POSIX return/error behavior, POSIX semaphore behavior, and
async-signal-safety.

## 3. Priority And Dependencies

- Overall priority: `MUST`.
- Required depth: deep for common primitives; controlled expert overview for
  memory model, lock-free programming, ABA, hazard pointers, epoch reclamation,
  and false sharing.
- Master prerequisite: CH12, Modern C++ And Templates, for move-only types,
  RAII, lambdas, `std::future`, vocabulary types, and template-based standard
  library APIs.
- Practical prerequisites:
  - Object lifetime, ownership, RAII, and destructor behavior.
  - Callable objects, lambdas, references, and move semantics.
  - Basic STL containers such as `std::vector` and `std::queue`.
  - Basic error handling and exception safety, especially for thread startup,
    future propagation, and cleanup paths.
  - C/POSIX return-code discipline for pthread and semaphore APIs.
- Follow-on topics:
  - Topic 16, Design Principles & Design Patterns, for thread-safe service
    design, observer/callback safety, and task dispatchers.
  - Topic 17, Testing Debugging Tooling, for ThreadSanitizer, Helgrind-like
    tools, stress tests, and deadlock diagnostics.
  - Topic 18, Code Review, for concurrency review checklists and API boundary
    assessment.

## 4. Scope And Depth Boundaries

### Deep In This Topic

- Thread basics: process vs thread, shared address space, `std::thread`
  creation, argument passing, lambdas, member functions, multiple threads,
  `join`, `detach`, `joinable`, thread IDs, and hardware concurrency.
- Thread lifetime and ownership: `std::thread` as a move-only owner of an
  execution thread; joining or detaching before destruction; detached-thread
  lifetime hazards.
- Shared state correctness: race condition, data race, critical section,
  happens-before at a practical level, and why unsynchronized read/write of
  shared non-atomic data is not a valid optimization problem.
- Mutexes and RAII locking: `std::mutex`, `std::lock_guard`,
  `std::unique_lock`, `std::scoped_lock`, lock duration, manual lock/unlock
  risks, and exception safety.
- Condition variables: mutex association, wait/notify, predicate waits,
  spurious wakeups, lost-wakeup prevention, timed waits, and producer-consumer
  queues.
- Deadlock/livelock prevention: lock ordering, avoiding callbacks under lock,
  `std::lock`, `std::scoped_lock`, timeouts, and careful lock hierarchy.
- Atomics: `std::atomic`, `atomic_flag`, `fetch_add`, compare-exchange,
  lock-free awareness, `volatile` misuse, and memory-order basics.
- Semaphores: conceptual counting permit model, POSIX `sem_t`, and C++20
  `std::counting_semaphore`.
- Higher-level async facilities: `std::future`, `std::promise`, `std::async`,
  launch policies, `std::packaged_task`, and exception propagation through
  futures.
- Thread pools: task queue, worker lifecycle, stop flag, condition variable,
  shutdown ordering, exception containment, and avoiding unbounded thread
  creation.
- Signal-aware shutdown: `std::signal`/`signal`, `raise`, POSIX `sigaction`,
  `sig_atomic_t`, lock-free atomic flags, async-signal-safe restrictions, and
  main-thread cleanup after a handler only records intent.

### Medium In This Topic

- `std::shared_mutex`, `std::recursive_mutex`, `std::timed_mutex`, and when
  their complexity is justified.
- `std::jthread` and `std::stop_token` as modern C++20 lifecycle helpers, if
  later output wants an optional modern note.
- C11 `_Atomic` and POSIX/GCC atomic-style comparisons only as bridges to C or
  legacy code.
- Performance considerations: contention, lock granularity, false sharing,
  oversubscription, cache effects, and reducing shared mutable state.
- Enterprise API design: explicit thread ownership, cancellation/shutdown
  policy, bounded queues, backpressure, and documented thread-safety contracts.

### Controlled Awareness

- Memory-order deep dive beyond acquire/release/relaxed/sequential consistency.
- Lock-free data structures, ABA problem, hazard pointers, epoch reclamation,
  and memory reclamation strategy.
- Realtime/embedded constraints around blocking, priority inversion, bounded
  latency, and allocation in worker paths.

### Defer Or Exclude

- Full POSIX/Linux API comparison belongs to Topic 16 unless needed for the
  required concurrency comparisons.
- Full debugging/tooling walkthrough belongs to Topic 17.
- Full code review checklist belongs to Topic 18.
- Linux Device Driver, kernel-driver, interrupt-handler, Yocto, GStreamer, or
  unrelated platform material is excluded.

## 5. Merged Concept Map

- Concurrency means multiple execution flows make progress in overlapping time;
  parallelism means they run simultaneously on multiple cores. A program can be
  concurrent without being parallel.
- Threads share memory, so communication is easy but correctness is fragile.
  Every shared mutable object needs a synchronization story: ownership transfer,
  mutex protection, atomic access, message passing, or immutability.
- `std::thread` starts execution immediately. Its object owns the thread handle,
  not the callable's captured object lifetimes. Joining synchronizes completion;
  detaching gives up ownership and usually requires a separately designed
  lifetime/cancellation mechanism.
- A race condition is a behavior-level bug where timing affects the result. A
  data race is a C++ memory-model violation caused by conflicting unsynchronized
  accesses to the same memory location, at least one a write. Data races are
  undefined behavior.
- Mutexes protect invariants, not individual lines. Lock the smallest region
  that preserves the invariant, and release before slow I/O, callbacks, logging
  hooks, or user-provided code when possible.
- RAII locks make mutex release exception-safe. `lock_guard` is the default
  simple lock. `unique_lock` is for deferred/manual/timed locking and condition
  variables. `scoped_lock` is the C++17 default for multiple mutexes.
- Condition variables are for waiting until a shared predicate changes. The
  predicate is the truth; notification is only a hint to recheck it.
- Semaphores count permits. Mutexes guard ownership of a critical section.
  Swapping one for the other without a design reason often creates bugs.
- Atomics are for small shared state and carefully designed lock-free protocols.
  They do not automatically make compound invariants safe.
- `volatile` is not a thread synchronization primitive. It does not provide
  atomicity, inter-thread ordering, or data-race freedom.
- Futures/promises transfer a one-time result or exception. They are useful for
  result-bearing asynchronous work but are not a general task scheduler.
- Thread pools amortize thread creation but introduce queue ownership, shutdown,
  backpressure, exception containment, and task-lifetime responsibilities.
- Signal handlers are not normal callbacks. In a concurrent program they should
  usually set a `volatile sig_atomic_t` flag or a verified lock-free atomic flag
  and let normal code perform cleanup outside the handler.

## 6. Required Comparisons

| Topic | C / POSIX API | Modern C++ API | Guidance |
| --- | --- | --- | --- |
| `pthread` vs `std::thread` | `pthread_create`, `pthread_join`, `pthread_detach`; C function pointer entry point; return-code errors | `std::thread` constructs from any callable; move-only owner; `join`, `detach`, `joinable`; exceptions on construction failures | Prefer `std::thread` in C++ application/library code. Use pthreads for C ABI, existing POSIX code, or POSIX-specific attributes/scheduling |
| `pthread_mutex_t` vs `std::mutex` | Explicit init/destroy/lock/unlock; error-code handling; manual cleanup | RAII wrappers with `lock_guard`, `unique_lock`, `scoped_lock` | Prefer C++ mutex plus RAII locks in C++. Wrap POSIX mutexes if POSIX attributes are required |
| `pthread_cond_t` vs `std::condition_variable` | Wait releases/reacquires a `pthread_mutex_t`; manual predicate discipline | Works with `std::unique_lock<std::mutex>`; predicate overload makes safe waiting idiomatic | In both APIs, always protect and recheck the predicate. Notification alone is not state |
| `sem_t` vs `std::counting_semaphore` | POSIX named/unnamed semaphores; C API and platform-specific lifetime rules | C++20 counting semaphore object with `acquire`, `release`, `try_acquire` forms | Use C++20 semaphore for portable C++ in-process permit counting. Use POSIX semaphores for legacy/POSIX interop or named semaphore requirements |
| `volatile` vs atomic | `volatile` may be used for signal-visible `sig_atomic_t` and memory-mapped I/O in other topics, but not thread synchronization | `std::atomic<T>` gives atomic operations and specified memory ordering | Teach `volatile` as not enough for threads. Use atomics or locks for inter-thread communication |
| mutex vs semaphore | POSIX mutex ownership guards a critical section; semaphore counts resources/permits | `std::mutex` guards invariants; `std::counting_semaphore` controls capacity or events | Use mutex for exclusive access to shared state. Use semaphore for bounded resources, rate/permit control, or producer-consumer capacity |
| `lock_guard` vs `unique_lock` | No direct C RAII equivalent | `lock_guard` is simple scoped locking; `unique_lock` supports deferred/manual/timed locking and condition-variable waits | Prefer `lock_guard` for simple critical sections. Use `unique_lock` only when the extra capability is needed |

## 7. Common Bugs And Corrections

- Forgetting to `join()` or `detach()` a joinable `std::thread`.
  Correction: join in the owning scope, use RAII joiners, or use `std::jthread`
  where appropriate.
- Detaching a thread that uses references or object pointers owned by a shorter
  scope.
  Correction: avoid detach by default; if used, design explicit ownership,
  cancellation, and shutdown.
- Incrementing or reading shared non-atomic data from multiple threads.
  Correction: protect with a mutex or use an atomic when the state is truly a
  single independent value.
- Confusing race condition with data race.
  Correction: explain both; a program can have a race condition without C++ UB,
  but a C++ data race is UB.
- Manual `lock()`/`unlock()` around code that can throw or return early.
  Correction: use RAII locks.
- Holding locks during slow I/O, sleeps, blocking calls, callbacks, or logging
  sinks.
  Correction: copy/move the needed data under lock, release, then do slow work.
- Waiting on a condition variable without a predicate.
  Correction: use `cv.wait(lock, predicate)` or a `while (!predicate)` loop.
- Updating the predicate without holding the associated mutex.
  Correction: mutate predicate state under the same mutex used by waiters, then
  notify.
- Assuming `notify_one()` stores an event.
  Correction: store state in the predicate; notification only wakes waiters.
- Locking multiple mutexes in inconsistent order.
  Correction: define lock hierarchy or use `std::scoped_lock`/`std::lock`.
- Using `std::recursive_mutex` to hide unclear ownership.
  Correction: refactor locking boundaries unless true recursion is required.
- Using atomics for multi-field invariants.
  Correction: use a mutex or redesign around immutable snapshots/message
  passing.
- Using `memory_order_relaxed` where ordering is required.
  Correction: default to sequential consistency while learning; introduce
  acquire/release only with a written invariant.
- Treating `volatile bool done` as a thread-safe stop flag.
  Correction: use `std::atomic<bool>` or condition-variable based shutdown.
- Calling `cout`, `printf`, `malloc`, `new`, `delete`, `mutex.lock()`, or
  complex cleanup from a signal handler.
  Correction: set a `sig_atomic_t`/safe flag or use an async-signal-safe
  operation, then cleanup in normal control flow.
- Letting exceptions escape thread functions or thread-pool tasks.
  Correction: catch at the thread boundary, store in `std::promise`/future, log
  safely, or convert to task failure state.

## 8. Debugging Notes

- Reproduce concurrency bugs with stress loops, high iteration counts, varied
  CPU affinity/scheduling, and randomized timing; one successful run proves
  very little.
- Compile with debug symbols and sanitizers where available:
  `-g -O1 -fsanitize=thread` for data races and many synchronization mistakes.
- Use AddressSanitizer/UBSan too when thread bugs might be lifetime or UB
  defects rather than pure synchronization defects.
- Add structured logs with thread IDs, state transitions, queue sizes, and lock
  ownership boundaries. Avoid logging while holding hot locks unless logging is
  proven safe and bounded.
- For condition variables, log predicate transitions and notifications, not just
  wait/notify calls.
- For deadlocks, inspect thread backtraces, lock order, blocked joins, and waits
  that have no corresponding state transition.
- For atomics, write down the invariant and required ordering. If the invariant
  cannot be stated simply, use a mutex.
- For POSIX APIs, check every return code. Many pthread functions return the
  error number directly instead of setting `errno` in the usual C-library style.
- For signal-related shutdown, test `SIGINT` and `SIGTERM` separately and verify
  cleanup happens outside the handler.

## 9. Best Practices

- Design ownership first: who owns the thread, who owns shared data, who stops
  work, and who joins?
- Prefer no shared mutable state when possible: immutable data, message passing,
  queues, futures, and ownership transfer reduce the synchronization surface.
- Protect invariants, not variables. Document which mutex protects which state.
- Prefer RAII locks and keep critical sections small.
- Use `lock_guard` by default, `unique_lock` for condition variables or flexible
  locking, and `scoped_lock` for multiple mutexes.
- Always wait on condition variables with predicates.
- Prefer bounded queues and explicit backpressure for production thread pools.
- Catch exceptions at thread/task boundaries and propagate them deliberately.
- Use atomics for simple flags/counters and well-reviewed protocols; use mutexes
  for compound state.
- Default to simple memory ordering until performance measurement and a written
  correctness argument justify weaker ordering.
- Avoid detached threads in application code unless the lifetime model is
  explicit and reviewed.
- Never call non-async-signal-safe code from signal handlers.
- Put concurrency APIs behind small, testable abstractions: worker, queue,
  executor, cancellation token, or service lifecycle object.
- Treat performance as secondary to correctness. Optimize only after data-race,
  deadlock, shutdown, and lifetime behavior are sound.

## 10. Interview Angles

- Define concurrency vs parallelism.
- Explain process vs thread memory and failure isolation.
- Explain `std::thread` lifecycle, `join`, `detach`, `joinable`, and destructor
  termination behavior.
- Race condition vs data race, with a counter increment example.
- Why `counter++` is not atomic.
- Mutex purpose and why RAII locking matters.
- `lock_guard` vs `unique_lock` vs `scoped_lock`.
- How condition variables work and why predicate waits are required.
- Spurious wakeup and lost wakeup.
- Producer-consumer queue design.
- Deadlock conditions and practical avoidance strategies.
- Mutex vs semaphore.
- Atomic vs `volatile`.
- Sequential consistency vs acquire/release vs relaxed at a practical level.
- When an atomic counter is appropriate and when a mutex is required.
- `pthread` vs `std::thread`.
- POSIX condition variable vs C++ condition variable.
- `std::async`, `future`, `promise`, and exception propagation.
- Thread pool design: worker loop, task queue, stop flag, condition variable,
  destructor, exception containment, and backpressure.
- Signal-safe shutdown in a multithreaded program.
- Debugging a flaky data race, deadlock, or hung shutdown.

## 11. Practice Targets

- Write a data-race counter example, then fix it once with `std::mutex` and once
  with `std::atomic<int>`.
- Implement a small thread-safe queue using `std::mutex` and
  `std::condition_variable`.
- Add a bounded-capacity queue using a semaphore or condition-variable
  predicates.
- Build a minimal thread pool with worker lifecycle, enqueue, stop, join, and
  exception containment.
- Convert a manual `lock()`/`unlock()` block to RAII locking and show why it is
  exception-safe.
- Demonstrate a deadlock with two mutexes, then fix it with lock ordering and
  `std::scoped_lock`.
- Write a `std::promise`/`std::future` example that propagates an exception.
- Compare a small pthread example with a `std::thread` equivalent.
- Implement signal-aware graceful shutdown that only sets a signal-safe flag in
  the handler and performs cleanup in normal code.
- Run a race example under ThreadSanitizer and record the diagnostic pattern.

## 12. Gaps And External Validation Needs

- Validate exact standard-library wording against cppreference when writing
  learner-facing content for `std::thread` destructor behavior,
  `std::condition_variable` waits, `std::atomic`, memory orders, and
  `std::counting_semaphore`.
- Validate POSIX details against man-pages when comparing pthreads, POSIX
  condition variables, POSIX mutexes, POSIX semaphores, and signals.
- The Notion material uses `std::atomic<bool>` in signal-handler examples with a
  lock-free assertion. Downstream content should present `volatile sig_atomic_t`
  as the portable C signal-handler baseline and mention lock-free atomics only
  with careful qualification.
- The Notion material includes spinlock examples. Downstream content should mark
  spinlocks as advanced/rare and not a beginner production default.
- The Notion material includes a double-checked locking singleton. Downstream
  content should avoid teaching raw-pointer singleton patterns as preferred;
  prefer function-local static initialization or dependency injection when the
  topic requires a thread-safe singleton note.
- Memory-order deep dive, lock-free reclamation, ABA, hazard pointers, and epoch
  reclamation should remain controlled awareness unless the user asks for expert
  expansion.
- No Linux Device Driver/kernel-driver material is needed or allowed for this
  topic brief.

## 13. Suggested Output Targets

- `knowledge/14-concurrency.md`
  - Teach in order: goal, why it matters, mental model, thread lifecycle,
    shared-state correctness, mutex/RAII locks, condition variables,
    semaphores, atomics, futures/async, thread pools, signals/shutdown,
    comparisons, bugs, debugging, best practices, interview readiness, and
    practice.
  - Use small compile-oriented examples.
  - Preserve English technical keywords.
  - Keep source coverage/audit metadata out.
- `interview/14-concurrency.md`
  - Include beginner, mid-level, and senior questions.
  - Each answer should contain short answer, deep explanation, C/C++ API anchor,
    production/debug angle, traps, and follow-ups.
  - Include coding tasks for counter fix, producer-consumer queue, deadlock
    diagnosis, thread pool design, and signal-safe shutdown.
- `examples/14-concurrency/README.md`
  - Include build/run commands and sanitizer commands.
  - Mark learning-only examples such as artificial races, deadlocks, spinlocks,
    and minimal thread pools.
  - Warn about data races, deadlocks, lifetime capture bugs, detached threads,
    exception escape, unsafe signal handlers, and unsafe POSIX return-code
    handling.

