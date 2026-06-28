# 18 - Enterprise And Interview Checklist Interview Pack

## How To Use This Pack

These questions test whether a candidate can use C/C++ knowledge in real code
review, debugging, and production design. Strong answers do not stop at a
definition. They connect each topic to ownership, lifetime, undefined behavior,
RAII, exception safety, thread safety, API design, performance, maintainability,
and debugging evidence.

## Beginner Questions

### 1. How do you review a simple C++ function?

**Short answer:** Start with correctness and undefined behavior, then check
ownership/lifetime, error handling, API clarity, and tests. Style comes after
behavior is safe.

**Deep explanation:** A practical review is risk-ordered. First ask whether the
function handles inputs, boundaries, and failure paths. Then classify every
pointer, reference, iterator, view, and callback as owning, borrowing,
observing, or stored for later. After behavior and lifetime are clear, review
readability, performance assumptions, and maintainability.

**C/C++ code/API anchor:**

```cpp
int at_or_default(const int* values, int size, int index) {
    if (values == nullptr || index < 0 || index >= size) {
        return 0;
    }
    return values[index];
}
```

**Production/debug angle:** Compile with warnings such as `-Wall -Wextra
-Wconversion -Wshadow`, then test null input, negative index, zero size, and
last valid index.

**Traps:** Starting review with formatting; ignoring nullability; assuming
because code compiles it is safe; forgetting negative indexes.

**Follow-ups:** How would you express the same API with `std::span<const int>`?
What tests would you write first?

### 2. Explain stack vs heap and why lifetime matters.

**Short answer:** Stack objects have automatic lifetime tied to scope. Heap
objects live until explicitly freed or until an owning RAII object destroys
them. Bugs happen when a pointer/reference outlives the object.

**Deep explanation:** Lifetime is the period during which an object exists and
may be safely used. A local variable is destroyed when its scope exits. Dynamic
memory allocated manually must be released correctly. In Modern C++, standard
containers and smart pointers are preferred because they tie cleanup to object
lifetime through RAII.

**C/C++ code/API anchor:**

```cpp
int* bad() {
    int value = 42;
    return &value; // dangling pointer
}
```

**Production/debug angle:** AddressSanitizer can expose use-after-scope and
use-after-free bugs. Code review should catch them earlier by asking what object
owns the storage.

**Traps:** Saying "stack is always safe"; returning addresses of locals;
assuming a pointer becomes `nullptr` after the object dies.

**Follow-ups:** How does `std::unique_ptr<int>` change the heap lifetime story?
What is the difference between lifetime and scope?

### 3. What is a dangling pointer, and how do you prevent it?

**Short answer:** A dangling pointer points to storage whose object lifetime has
ended. Prevent it with clear ownership, RAII, and by avoiding returns or stored
callbacks that reference dead objects.

**Deep explanation:** A pointer value is just an address. It does not know
whether the object at that address is still alive. Dereferencing a dangling
pointer is undefined behavior. The same lifetime problem can appear with raw
pointers, references, iterators, `std::string_view`, `std::span`, and lambda
captures.

**C/C++ code/API anchor:**

```cpp
#include <string>
#include <string_view>

std::string_view name_bad() {
    std::string name = "sensor";
    return name; // dangling view
}
```

**Production/debug angle:** ASan helps find many dangling-memory bugs. During
review, trace object lifetime rather than trusting the type name.

**Traps:** Thinking only raw pointers can dangle; storing `c_str()` after the
`std::string` changes or dies; returning `std::span` to a local array.

**Follow-ups:** When is returning `std::string_view` safe? How can a lambda
capture create the same bug?

### 4. Why is `std::vector` often safer than a raw dynamic array?

**Short answer:** `std::vector` owns its memory, tracks size, cleans up
automatically, and integrates with algorithms. A raw dynamic array requires
manual lifetime and size tracking.

**Deep explanation:** Raw arrays decay to pointers and lose size information in
many function calls. Dynamic arrays require matching `new[]`/`delete[]`, and
manual paths can leak. `std::vector` uses RAII and exposes `size()`, iterators,
and bounds-checked `at()`. It still has rules: growth can invalidate pointers,
references, and iterators.

**C/C++ code/API anchor:**

```cpp
#include <vector>

int sum(const std::vector<int>& values) {
    int total = 0;
    for (int value : values) {
        total += value;
    }
    return total;
}
```

**Production/debug angle:** Review code that stores `values.data()` or iterators
across `push_back`, `insert`, `erase`, `resize`, or `reserve`.

**Traps:** Claiming `std::vector` prevents all out-of-bounds bugs; using
`operator[]` without validating indexes; keeping stale iterators.

**Follow-ups:** When would `std::array` be better? When would `std::span` be
useful?

## Mid-Level Questions

### 5. Compare `malloc/free`, `new/delete`, and RAII.

**Short answer:** `malloc/free` allocate raw storage in C style. `new/delete`
construct and destroy C++ objects. RAII is the preferred C++ style: object
lifetime automatically controls resource cleanup.

**Deep explanation:** `malloc` does not call constructors and `free` does not
call destructors. `new` constructs an object and `delete` destroys it, but both
still require manual matching. RAII avoids scattered cleanup by putting the
resource in an object whose destructor releases it. `std::vector`,
`std::string`, `std::unique_ptr`, file streams, and lock guards are common RAII
types.

**C/C++ code/API anchor:**

```cpp
#include <memory>

auto value = std::make_unique<int>(42); // cleanup is automatic
```

**Production/debug angle:** In review, reject mixed families such as
`malloc` with `delete` or `new` with `free`. Use ASan/LSan or Valgrind to catch
leaks and invalid frees, but design the API so ownership is clear first.

**Traps:** Using `malloc` for C++ objects with constructors; forgetting
`delete[]`; returning raw owning pointers; assuming smart pointers fix wrong
ownership design.

**Follow-ups:** When is `std::unique_ptr<T[]>` acceptable? What is the Rule of
Zero?

### 6. Explain shallow copy vs deep copy and the Rule of Three/Five/Zero.

**Short answer:** A shallow copy copies pointer values; a deep copy duplicates
the owned resource. If a class manually owns a resource, it must define or
disable copy/move/destruction correctly. Prefer Rule of Zero by using RAII
members.

**Deep explanation:** The default copy constructor copies members. If a member
is a raw owning pointer, two objects can end up owning the same allocation,
causing double delete. Rule of Three covers destructor, copy constructor, and
copy assignment. Rule of Five adds move constructor and move assignment. Rule of
Zero says to avoid writing any of them by storing resources in standard RAII
types.

**C/C++ code/API anchor:**

```cpp
#include <vector>

class Buffer {
public:
    explicit Buffer(int n) : data_(static_cast<std::size_t>(n)) {}

private:
    std::vector<int> data_; // Rule of Zero
};
```

**Production/debug angle:** Review every class with a raw pointer, file handle,
mutex, socket-like handle, or manually allocated buffer. Ask whether copy and
move semantics are correct and exception-safe.

**Traps:** Writing a destructor but forgetting copy operations; copying
`std::unique_ptr`; implementing move but leaving moved-from objects invalid;
manual memory where `std::vector` is enough.

**Follow-ups:** How would you make a class move-only? Why is copy-and-swap often
used for assignment?

### 7. Why does a polymorphic base class need a virtual destructor?

**Short answer:** If a derived object may be deleted through a base pointer, the
base destructor must be virtual so the derived destructor runs.

**Deep explanation:** Runtime polymorphism uses base pointers or references to
operate on derived objects. If deletion happens through a base pointer without a
virtual destructor, cleanup is incomplete and behavior is undefined for this
common ownership pattern. A base class with virtual functions usually needs a
virtual destructor.

**C/C++ code/API anchor:**

```cpp
#include <memory>

class Sink {
public:
    virtual void write(const char*) = 0;
    virtual ~Sink() = default;
};

void use(std::unique_ptr<Sink> sink) {
    sink->write("ok");
}
```

**Production/debug angle:** During review, search for virtual functions and
check destructor visibility. Sanitizers may expose leaks or partial cleanup, but
the design rule is simple enough to catch by inspection.

**Traps:** Adding virtual functions but not a virtual destructor; passing
derived objects by value and slicing them; using inheritance for code reuse
instead of substitutability.

**Follow-ups:** What is object slicing? When is protected non-virtual destructor
reasonable?

### 8. Explain iterator invalidation in `std::vector`.

**Short answer:** Operations that change a vector's storage or erase elements
can invalidate iterators, references, and pointers into the vector.

**Deep explanation:** `std::vector` stores elements contiguously. If it grows
beyond capacity, it reallocates and moves/copies elements to new storage. Old
pointers, references, and iterators then refer to old storage. `erase` also
invalidates iterators at and after the erase point. Safe code uses returned
iterators and does not keep stale addresses.

**C/C++ code/API anchor:**

```cpp
#include <vector>

void remove_zero(std::vector<int>& values) {
    for (auto it = values.begin(); it != values.end();) {
        if (*it == 0) {
            it = values.erase(it);
        } else {
            ++it;
        }
    }
}
```

**Production/debug angle:** Iterator invalidation often appears as intermittent
crashes. Review around `push_back`, `insert`, `erase`, `resize`, `reserve`, and
`rehash` for unordered containers.

**Traps:** Incrementing an iterator after `erase` without using the returned
iterator; storing `data()` then growing the vector; assuming all containers have
the same invalidation rules.

**Follow-ups:** How do `std::list` and `std::map` differ? What does `reserve`
change?

### 9. Debug this callback lifetime bug.

**Short answer:** The lambda captures `message` by reference, but `message` dies
when the factory function returns. The stored callback dangles.

**Deep explanation:** Stored callables are objects with lifetime. A lambda that
captures by reference does not own the referenced object. If the lambda runs
after the referenced object is destroyed, using the reference is undefined
behavior. Capture by value when the callback needs its own copy, or store a
safe owning/shared object with a clear policy.

**C/C++ code/API anchor:**

```cpp
#include <functional>
#include <iostream>
#include <string>

std::function<void()> bad() {
    std::string message = "done";
    return [&] { std::cout << message << '\n'; };
}

std::function<void()> good() {
    std::string message = "done";
    return [message] { std::cout << message << '\n'; };
}
```

**Production/debug angle:** ASan may catch some use-after-scope cases. Review
every stored lambda, `std::function`, callback list, Observer, and Command for
capture lifetime.

**Traps:** Thinking `[&]` is just a shorter syntax; capturing `this` when the
object may be destroyed before callback execution; storing callbacks without
unsubscribe policy.

**Follow-ups:** When is reference capture safe? How would you design an
unsubscribe token?

### 10. Compare error codes and exceptions in a review.

**Short answer:** Error codes make failures explicit at each call site.
Exceptions separate normal flow from error flow and work well with RAII. The API
boundary should choose a consistent policy.

**Deep explanation:** C APIs often return status codes and use out parameters or
`errno`. C++ APIs may throw exceptions for unexpected failures or return
value-based results such as `std::optional` for absence and `std::variant` for
closed alternatives. Exceptions require exception-safe code: resources must be
owned by RAII objects and destructors must not throw.

**C/C++ code/API anchor:**

```cpp
#include <fstream>
#include <stdexcept>

void save(const char* path) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("open failed");
    }
    out << "data\n";
}
```

**Production/debug angle:** Review whether errors preserve diagnostic detail.
Check that cleanup still happens on exceptions and that C API errors are
translated without losing meaning.

**Traps:** Swallowing exceptions; using exceptions for ordinary loop control;
mixing error codes and exceptions randomly; throwing from destructors.

**Follow-ups:** When is `std::optional` not enough for an error? What is the
strong exception guarantee?

## Senior Questions

### 11. How do you structure an enterprise C++ code review checklist?

**Short answer:** Review in risk order: correctness/UB, ownership/lifetime,
exception safety, concurrency, API contract, STL invalidation and complexity,
performance assumptions, maintainability, and tests/tooling.

**Deep explanation:** A senior review is not a style pass. It asks whether the
code can fail safely and be changed safely. The checklist should force explicit
answers about who owns resources, whether invariants are protected, whether
errors preserve context, whether shared state has a synchronization policy, and
whether the abstraction is justified.

**C/C++ code/API anchor:**

```text
Owner? Borrower? View? Callback?
Throws? noexcept? Cleanup path?
Shared state? Mutex/atomic? Lock order?
Container operation? Invalidation? Complexity?
```

**Production/debug angle:** Require evidence: warning-clean build, unit tests
for boundaries and failures, sanitizer runs for memory/UB, ThreadSanitizer or
stress tests for concurrency.

**Traps:** Reviewing only formatting; accepting "works on my machine"; ignoring
failure paths; not checking callback/view lifetimes; reviewing pattern names
instead of problem fit.

**Follow-ups:** What items are highest risk in legacy C code? What items are
highest risk in template-heavy C++?

### 12. When is `std::atomic` enough, and when do you need a mutex?

**Short answer:** `std::atomic` is enough for independent atomic state such as a
flag or counter. Use a mutex when multiple values must change together as one
invariant.

**Deep explanation:** Atomics prevent data races on individual atomic objects,
but they do not magically protect compound invariants. If code needs to update
`queue`, `size`, and `closed` consistently, a mutex is usually clearer. Atomics
also require memory-order reasoning; for most application-level invariants,
mutexes are easier to review and maintain.

**C/C++ code/API anchor:**

```cpp
#include <atomic>
#include <mutex>
#include <queue>

std::atomic<bool> stop_requested{false}; // independent flag

std::mutex mutex;
std::queue<int> work; // compound invariant: protect with mutex
```

**Production/debug angle:** Use ThreadSanitizer to detect data races, but also
perform invariant review: "Which lock protects this data?" and "Is every access
covered?"

**Traps:** Using `volatile` for synchronization; using atomics for one field
while related non-atomic fields race; assuming sequential consistency fixes bad
design.

**Follow-ups:** Why is `volatile` not a threading primitive? How does a
condition variable relate to a mutex-protected predicate?

### 13. Review this condition-variable code.

**Short answer:** Waiting without a predicate is wrong. Condition variables can
wake spuriously, and notifications can be missed if the condition is not stored
in shared state protected by the mutex.

**Deep explanation:** A condition variable is not the condition itself. It is a
wake-up mechanism. The real condition must be represented by protected shared
state, such as `!queue.empty()` or `ready == true`. The waiting thread must
check the predicate while holding the lock.

**C/C++ code/API anchor:**

```cpp
#include <condition_variable>
#include <mutex>

std::mutex mutex;
std::condition_variable cv;
bool ready = false;

void wait_ready() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [] { return ready; });
}
```

**Production/debug angle:** For hangs, inspect thread backtraces, lock order,
and whether the notifying thread changes the predicate under the same mutex.
ThreadSanitizer detects races, not all logical missed-notification bugs.

**Traps:** `cv.wait(lock)` without predicate; notifying without changing shared
state; holding locks while calling arbitrary user callbacks; inconsistent lock
order.

**Follow-ups:** Should you call `notify_one` while holding the lock? How would
you implement producer-consumer shutdown?

### 14. How do you review an API boundary around C code?

**Short answer:** Make ownership, error handling, lifetime, nullability, and
thread-safety explicit. In C++, wrap resources with RAII and translate errors
without losing detail.

**Deep explanation:** C APIs often use raw pointers, `void*` contexts, status
codes, and manual cleanup functions. A Modern C++ boundary should hide manual
cleanup behind RAII, convert raw buffers to safe views only when lifetime is
clear, and document whether callbacks may be stored or called immediately.

**C/C++ code/API anchor:**

```cpp
#include <cstdio>
#include <memory>

using FilePtr = std::unique_ptr<FILE, int (*)(FILE*)>;

FilePtr open_file(const char* path) {
    return FilePtr(std::fopen(path, "r"), &std::fclose);
}
```

**Production/debug angle:** Test failure paths: null handle, open failure,
partial read/write, callback lifetime, and cleanup on exceptions. Use ASan/UBSan
for memory and invalid usage.

**Traps:** Returning raw owning handles; losing `errno`/diagnostic context;
storing pointers to temporary buffers; wrapping an unsafe API but hiding its
failure mode.

**Follow-ups:** How would you expose a C buffer as `std::span` safely? When
should a wrapper be an Adapter versus a Facade?

### 15. Decide between simple function, lambda, `std::function`, template, and virtual interface.

**Short answer:** Choose the simplest mechanism that expresses the variation.
Use virtual interfaces for runtime substitution, templates for compile-time
polymorphism, lambdas/functions for local customization, and `std::function`
when you need type-erased storage.

**Deep explanation:** Over-engineering happens when every variation becomes a
class hierarchy. Under-engineering happens when a long `switch` or callback
blob hides ownership and lifetime. A senior engineer selects the mechanism that
makes change safe with the least complexity.

**C/C++ code/API anchor:**

```cpp
template <typename Predicate>
int count_if_value(const int* values, int size, Predicate pred) {
    int count = 0;
    for (int i = 0; i < size; ++i) {
        if (pred(values[i])) {
            ++count;
        }
    }
    return count;
}
```

**Production/debug angle:** In review, ask what must vary: algorithm,
implementation, object creation, event subscription, subsystem setup, or stored
command. Then choose Strategy, Factory Method, Observer, Facade, Command, or no
pattern at all.

**Traps:** Using `std::function` for every callback; using inheritance for local
behavior; templates that explode compile errors; pattern name-dropping.

**Follow-ups:** When is Strategy better than a lambda? When does a static
factory become a Factory Method?

### 16. How do you identify an over-engineered design pattern?

**Short answer:** A pattern is over-engineered when it adds classes, virtual
dispatch, ownership complexity, or indirection without solving real variation,
lifetime, coupling, or subsystem coordination.

**Deep explanation:** Patterns should make change safer. If the code has one
state, one algorithm, one listener, or one product type, a direct function,
`enum class`, constructor, or lambda may be clearer. The review should compare
the pattern with the simplest working solution.

**C/C++ code/API anchor:**

```cpp
// One local policy: a lambda is enough.
auto is_valid = [](int value) { return value >= 0 && value <= 100; };
```

**Production/debug angle:** Over-abstraction increases debugging cost. Virtual
call stacks, callback chains, and wrapper layers should earn their place with a
real change point.

**Traps:** Treating design patterns as badges; using Abstract Factory for one
product; Observer without unsubscribe policy; Facade that becomes a god object.

**Follow-ups:** What would make State better than `enum class` plus `switch`?
What would make Observer better than one callback?

## Coding And Debugging Tasks

### Task 1. Find the lifetime bug.

```cpp
#include <string>
#include <string_view>

std::string_view label() {
    std::string text = "ready";
    return text;
}
```

Expected answer: `text` is destroyed when `label` returns, so the
`std::string_view` dangles. Return `std::string`, return a view to static
storage, or make the caller own the string.

### Task 2. Fix erase-while-iterating.

```cpp
#include <vector>

void remove_negative(std::vector<int>& values) {
    for (auto it = values.begin(); it != values.end(); ++it) {
        if (*it < 0) {
            values.erase(it);
        }
    }
}
```

Expected answer: assign the iterator returned by `erase`; only increment when
not erasing.

### Task 3. Review a resource-owning class.

Checklist:

- Does it own a raw resource?
- Does it need destructor, copy constructor, copy assignment, move constructor,
  and move assignment?
- Can it use `std::vector`, `std::string`, `std::unique_ptr`, or another RAII
  member instead?
- Is assignment exception-safe?

Expected answer: prefer Rule of Zero when possible; otherwise define or delete
copy/move operations deliberately.

### Task 4. Debug a data race.

Scenario:

```cpp
#include <thread>

int counter = 0;

void work() {
    for (int i = 0; i < 1000; ++i) {
        ++counter;
    }
}
```

Expected answer: `counter` is shared mutable state accessed without
synchronization. Use a mutex if counter is part of a larger invariant, or
`std::atomic<int>` for an independent counter. Run ThreadSanitizer on a complete
test program.

### Task 5. Design a review checklist answer.

Prompt: "How do you review C++ code in production?"

Expected answer:

- correctness and undefined behavior;
- ownership/lifetime and RAII;
- exception safety;
- concurrency;
- API contracts;
- STL invalidation and complexity;
- performance assumptions;
- maintainability and pattern fit;
- tests and tool evidence.

## Quick Evaluation Rubric

Strong candidates:

- classify ownership and lifetime precisely;
- know that undefined behavior is not "just a crash";
- prefer RAII and Rule of Zero;
- distinguish borrowing, owning, viewing, and storing callbacks;
- check virtual destructors and slicing;
- know iterator invalidation rules at review depth;
- wait on condition variables with predicates;
- do not use `volatile` for thread synchronization;
- choose simple mechanisms before design patterns;
- mention concrete debugging tools and tests.

Weak candidates:

- rely on slogans;
- say `shared_ptr` everywhere;
- treat all pointers as owning;
- ignore failure paths;
- confuse race condition and data race;
- use pattern names without explaining the problem;
- start code review with formatting instead of correctness.
