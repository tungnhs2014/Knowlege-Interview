# 18 - Enterprise And Interview Checklist

## 1. Goal

After this lesson, you should be able to use a C/C++ checklist as a practical
review and interview tool.

You should be able to:

- review C/C++ code for correctness, undefined behavior, memory safety,
  ownership, lifetime, RAII, exception safety, thread safety, API design,
  performance, and maintainability;
- explain the must-know checklist areas: memory, pointers, OOP, STL, Modern C++,
  concurrency, and design patterns;
- compare C-style mechanisms with Modern C++ mechanisms without blindly saying
  "C++ is always safer";
- identify common bugs from small code snippets;
- choose the right debugging tool: compiler warnings, GDB, sanitizers, Valgrind,
  static analysis, unit tests, and stress tests;
- answer interview questions with definition, mechanism, code/API anchor,
  production risk, and debugging method.

This is a capstone lesson. It does not re-teach every previous chapter from
zero. It gives you a way to compress all of them into a review workflow.

## 2. Why It Matters

Enterprise C/C++ code fails in very ordinary ways:

- a pointer outlives the object it points to;
- a function returns a `std::string_view` to a temporary;
- a class owns memory but forgets the Rule of Five or Rule of Zero;
- a polymorphic base class has no virtual destructor;
- a `std::vector` iterator is used after reallocation;
- a callback captures a stack variable by reference and runs later;
- a thread writes shared data without a lock or atomic protocol;
- a condition variable waits without a predicate;
- a design pattern adds complexity but no useful variation.

An interview often tests the same things, just faster. The interviewer wants to
know whether you can move from a keyword to real engineering consequences:

- "What is RAII?" becomes "Will this resource leak if an exception is thrown?"
- "What is polymorphism?" becomes "Can this base pointer delete correctly?"
- "What is `std::atomic`?" becomes "Does this protect the whole invariant?"
- "What is Strategy?" becomes "Is a lambda enough here?"

A checklist is useful because it slows you down in the right places. You do not
review code from top to bottom like a novel. You scan risk areas deliberately.

## 3. Mental Model

Think of the final checklist as three stacked questions.

Question 1: Is the behavior correct?

- Does the code do what it claims?
- Are all inputs handled?
- Are failure paths handled?
- Is there undefined behavior?
- Are boundary cases tested?

Question 2: Is the lifetime clear?

- Who owns each resource?
- Who borrows?
- Who observes?
- Can anything dangle?
- Can exceptions, early returns, or thread interleavings break cleanup?

Question 3: Is the design maintainable?

- Is the API easy to use correctly?
- Are invariants protected?
- Are containers and algorithms chosen for the actual access pattern?
- Is concurrency policy documented?
- Is abstraction solving a real problem?
- Can a reviewer explain the code in a few sentences?

In interviews, use the same model:

1. Define the term.
2. Explain the mechanism.
3. Give a small code/API example.
4. Name the bug.
5. Name the debugging or review method.
6. State the production rule.

That answer shape is much stronger than memorizing isolated definitions.

## 4. Mechanism

### The Review Pass Order

Use this order when reviewing C/C++ code:

1. Correctness and undefined behavior.
2. Memory/resource ownership and lifetime.
3. Error handling and exception safety.
4. Concurrency and synchronization.
5. API design and invariants.
6. STL/container complexity and invalidation.
7. Performance assumptions.
8. Maintainability and pattern fit.
9. Tests and debugging evidence.

Do not start with style nits while the code may have a use-after-free. Review
high-risk behavior first.

### The Ownership Vocabulary

For every pointer, reference, handle, callback, container element, and view,
classify the relationship:

| Relationship | Meaning | C/C++ Shape |
| --- | --- | --- |
| Owns | Responsible for cleanup | RAII object, `std::unique_ptr`, `std::vector`, manual C cleanup owner |
| Borrows | Temporary access, no cleanup | `T&`, `const T&`, non-owning `T*` |
| Observes | May be nullable, no cleanup | `T*`, `std::weak_ptr`, ID/token |
| Shares | Multiple owners intentionally | `std::shared_ptr` when truly needed |
| Views | Non-owning range/string | `std::span`, `std::string_view` |
| Calls later | Stored behavior | function pointer, lambda, `std::function`, callback interface |

Most C/C++ bugs become easier when you ask, "What is the lifetime contract?"

### The Checklist Categories

Memory:

- stack vs heap;
- data vs BSS;
- alignment, padding, endianness;
- `malloc` vs `calloc`;
- `malloc` vs `new`;
- `free` vs `delete`;
- `new[]` vs `delete[]`;
- memory leak, dangling pointer, double free, use-after-free;
- buffer overflow;
- shallow copy vs deep copy;
- RAII.

Pointers:

- pointer vs array;
- pointer arithmetic;
- double pointer;
- function pointer;
- `void*`;
- const pointer vs pointer to const;
- null pointer;
- dangling pointer;
- `nullptr` vs `NULL`.

OOP:

- encapsulation and abstraction;
- inheritance and polymorphism;
- virtual function, vtable/vptr;
- pure virtual function and abstract class;
- interface;
- virtual destructor;
- object slicing;
- composition vs inheritance.

STL:

- `std::vector`, `std::array`;
- `std::map`, `std::set`;
- `std::unordered_map`, `std::unordered_set`;
- `std::stack`, `std::queue`, `std::priority_queue`;
- iterator and algorithm;
- comparator;
- iterator invalidation;
- complexity.

Modern C++:

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

Concurrency:

- `std::thread`;
- mutex and lock;
- condition variable;
- semaphore;
- atomic;
- race condition;
- deadlock;
- spurious wakeup;
- producer-consumer;
- thread pool.

Design patterns:

- MUST: State / FSM, Strategy, Observer, Factory Method, Adapter, Facade,
  Command;
- SHOULD: Builder, Decorator, Proxy, Template Method, Chain of Responsibility,
  Mediator, Iterator, Composite, Prototype;
- NICE: Visitor, Memento, Flyweight, Bridge, Abstract Factory.

For design patterns, the review question is not "Can I name the pattern?" The
question is "Does this structure solve a real variation, lifetime, coupling, or
coordination problem?"

## 5. C/C++ API And Code Anchors

### Memory And RAII

Manual cleanup is fragile:

```cpp
#include <cstdio>
#include <stdexcept>

void write_bad(const char* path) {
    FILE* file = std::fopen(path, "w");
    if (file == nullptr) {
        throw std::runtime_error("open failed");
    }

    // If code added here throws, fclose may be skipped.
    std::fputs("hello\n", file);
    std::fclose(file);
}
```

RAII ties cleanup to object lifetime:

```cpp
#include <fstream>
#include <stdexcept>

void write_good(const char* path) {
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("open failed");
    }

    file << "hello\n";
} // file closes automatically
```

Review rule: in C++, prefer RAII. In C, use one clear cleanup path and document
who owns each resource.

### Pointer And View Lifetime

This compiles, but it returns a dangling `std::string_view`:

```cpp
#include <string>
#include <string_view>

std::string_view bad_name() {
    std::string name = "sensor";
    return name; // dangling view: name is destroyed
}
```

Safer options:

```cpp
#include <string>
#include <string_view>

std::string make_name() {
    return "sensor";
}

std::string_view stable_name() {
    return "sensor"; // string literal has static storage duration
}
```

Review rule: `std::string_view` and `std::span` are views, not owners. They must
not outlive the data they view.

### OOP And Virtual Destructors

If a base class is used polymorphically and deleted through a base pointer, its
destructor must be virtual:

```cpp
#include <iostream>
#include <memory>

class Sink {
public:
    virtual void write(const char* text) = 0;
    virtual ~Sink() = default;
};

class ConsoleSink final : public Sink {
public:
    void write(const char* text) override {
        std::cout << text << '\n';
    }
};

int main() {
    std::unique_ptr<Sink> sink = std::make_unique<ConsoleSink>();
    sink->write("ok");
}
```

Review rule: when you see a virtual function, check the destructor and check for
object slicing.

### STL Iterator Invalidation

Erasing from a `std::vector` while iterating must use the iterator returned by
`erase`:

```cpp
#include <vector>

void remove_even(std::vector<int>& values) {
    for (auto it = values.begin(); it != values.end();) {
        if (*it % 2 == 0) {
            it = values.erase(it); // returned iterator is the next valid one
        } else {
            ++it;
        }
    }
}
```

Review rule: every insert, erase, reallocation, and container growth may affect
iterators, pointers, and references. The exact rule depends on the container.

### Lambda Capture Lifetime

This is a classic delayed-callback bug:

```cpp
#include <functional>
#include <string>

std::function<void()> make_bad_callback() {
    std::string message = "done";
    return [&] {
        // message is already destroyed when callback runs.
        (void)message;
    };
}
```

Capture by value when the callback must own the data:

```cpp
#include <functional>
#include <iostream>
#include <string>

std::function<void()> make_good_callback() {
    std::string message = "done";
    return [message] {
        std::cout << message << '\n';
    };
}
```

Review rule: every stored lambda needs a capture-lifetime check.

### Condition Variable Predicate

`std::condition_variable` can wake spuriously. Always wait with a predicate:

```cpp
#include <condition_variable>
#include <mutex>
#include <queue>

class Queue {
public:
    void push(int value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            values_.push(value);
        }
        cv_.notify_one();
    }

    int pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !values_.empty(); });
        int value = values_.front();
        values_.pop();
        return value;
    }

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<int> values_;
};
```

Review rule: the predicate protects correctness; the notification only wakes a
thread so it can re-check the condition.

## 6. Practical Usage

### A Practical Code Review Workflow

Use this as a review pass:

1. Build:
   - Does it compile with warnings?
   - Are warnings treated seriously?
   - Is the intended C++ standard clear?

2. Behavior:
   - Are inputs validated?
   - Are boundary cases covered?
   - Are errors returned, thrown, or logged consistently?

3. Memory and lifetime:
   - Who owns each allocation/resource?
   - Are raw owning pointers avoided?
   - Can any pointer, reference, iterator, view, or callback dangle?

4. Error handling:
   - Can exceptions skip cleanup?
   - Are destructors non-throwing?
   - Does the API preserve useful diagnostic detail?

5. STL:
   - Is the container appropriate?
   - Are invalidation rules respected?
   - Is complexity acceptable for expected input size?

6. Concurrency:
   - What data is shared?
   - What protects it?
   - Is lock order consistent?
   - Do condition variables use predicates?
   - Are threads joined or otherwise owned?

7. API design:
   - Are ownership and nullability obvious?
   - Are interfaces small?
   - Are invariants protected?
   - Is `const` used to communicate read-only intent?

8. Maintainability:
   - Is abstraction helping?
   - Is a design pattern solving a real problem?
   - Would a simpler function, lambda, container, or class be clearer?

9. Evidence:
   - Are there tests for success, failure, boundaries, and concurrency?
   - Were sanitizer/debug tools run?

### Build And Debug Commands

Compile a small review snippet with strict warnings:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -g review.cpp -o review
```

Run with AddressSanitizer and UndefinedBehaviorSanitizer:

```sh
g++ -std=c++17 -Wall -Wextra -g -fsanitize=address,undefined -fno-omit-frame-pointer review.cpp -o review_asan
./review_asan
```

Run with ThreadSanitizer for threaded code:

```sh
g++ -std=c++17 -Wall -Wextra -g -O1 -fsanitize=thread -fno-omit-frame-pointer -pthread review.cpp -o review_tsan
./review_tsan
```

Use GDB for crashes:

```sh
g++ -std=c++17 -g -O0 review.cpp -o review_dbg
gdb ./review_dbg
```

Inside GDB, common commands are:

```text
run
bt
frame 1
print variable_name
```

### Interview Usage

When asked a broad question like "How do you review C++ code?", do not answer
with style rules first. A strong answer starts with risks:

> I start with correctness and undefined behavior, then ownership/lifetime,
> exception safety, concurrency, API contracts, STL invalidation and complexity,
> performance assumptions, and tests. For C++ specifically I look for RAII,
> clear ownership, virtual destructors in polymorphic bases, safe lambda
> captures, and correct synchronization.

That answer shows engineering judgment.

## 7. Comparisons

### C Vs C++ Checklist

| Topic | C | C++ | Practical Guidance |
| --- | --- | --- | --- |
| Cleanup | Manual cleanup calls | RAII destructors | Prefer RAII in C++; in C use one cleanup path and clear ownership comments |
| Allocation | `malloc`, `calloc`, `realloc`, `free` | `new`, `delete`, containers, smart pointers | Do not mix allocation families; prefer containers and smart pointers |
| Arrays | Raw storage and decay | `std::array`, `std::vector`, `std::span` | Know bounds and lifetime; `std::span` does not own |
| Strings | `char*` and C string functions | `std::string`, `std::string_view` | Avoid buffer overflow; watch view lifetime |
| Callbacks | Function pointer plus context | Lambda, functor, `std::function`, interface | Stored callbacks need capture/context lifetime rules |
| Error handling | Return codes, `errno`, out parameters | Exceptions, return codes, `std::optional`, `std::variant` | Pick one policy per API boundary |
| Polymorphism | Function pointers and operation tables | Virtual interfaces or templates | Check virtual destructors and ownership |
| Macros | Preprocessor substitution | `constexpr`, inline functions, templates | Use macros sparingly; prefer typed alternatives |

### POSIX/User-Space C API Vs Modern C++ Reminders

| Topic | C/POSIX User-Space | Modern C++ | Review Guidance |
| --- | --- | --- | --- |
| Thread | `pthread_create` | `std::thread`, `std::jthread` when available | Define join/detach/stop ownership |
| Mutex | `pthread_mutex_t` | `std::mutex`, `std::lock_guard` | Prefer RAII locking |
| Condition variable | `pthread_cond_t` | `std::condition_variable` | Always wait with a predicate |
| File I/O | `open/read/write/close` | `std::fstream`, RAII wrapper | Check errors and guarantee close |
| Time | `clock_gettime`, `nanosleep` | `std::chrono` | Prefer steady clock for durations |
| Atomic | C11 `_Atomic`, compiler builtin | `std::atomic` | Do not use `volatile` for synchronization |

This topic only needs checklist-level reminders. Full POSIX/user-space API
wrapping belongs to the POSIX/Linux C API vs Modern C++ topic.

### Runtime Polymorphism Vs Static Polymorphism

| Choice | Use When | Watch Out |
| --- | --- | --- |
| Virtual interface | Runtime substitution is needed | Virtual destructor, slicing, allocation/lifetime |
| Template/static polymorphism | Type is known at compile time and performance matters | Error verbosity, compile time, over-generalization |
| Lambda/function parameter | One local behavior varies | Capture lifetime |
| `std::function` | You need type-erased storage | Allocation overhead, callable lifetime |

### `std::unique_ptr` Vs `std::shared_ptr`

| Pointer | Meaning | Review Question |
| --- | --- | --- |
| `std::unique_ptr<T>` | Exclusive ownership | Who receives ownership after `std::move`? |
| `std::shared_ptr<T>` | Shared ownership | Is shared ownership truly needed? |
| `std::weak_ptr<T>` | Non-owning observer of shared object | Is it checked with `lock()` before use? |
| raw `T*` / `T&` | Usually non-owning access | Is lifetime guaranteed by the caller? |

## 8. Common Bugs

Memory and pointer bugs:

- uninitialized variables and uninitialized pointers;
- out-of-bounds array or vector access;
- pointer arithmetic outside the same array;
- returning address/reference/view of a local object;
- dangling `std::string_view`, `std::span`, or `c_str()` pointer;
- memory leak from early return or exception;
- double free;
- use-after-free;
- mismatched `new[]` and `delete`;
- mixed `malloc` with `delete` or `new` with `free`;
- shallow copy of an owning pointer.

OOP and design bugs:

- missing virtual destructor;
- object slicing by passing derived objects by value;
- inheritance used where composition is simpler;
- interface too large to implement or mock safely;
- factory returning raw owning pointer;
- pattern name does not match actual structure.

STL and Modern C++ bugs:

- invalidated iterator/reference after container modification;
- comparator that does not provide strict weak ordering;
- poor hash/equality behavior in unordered containers;
- assuming `unordered_map` is always constant time;
- lambda captures by reference but runs later;
- reading a moved-from object as if unchanged;
- incorrect `noexcept`;
- using `std::shared_ptr` as default ownership;
- cyclic `std::shared_ptr` without `std::weak_ptr`.

Concurrency bugs:

- data race on shared mutable state;
- race condition from wrong ordering assumptions;
- deadlock from inconsistent lock order;
- `condition_variable` wait without predicate;
- notifying before state change;
- using `volatile` instead of `std::atomic` or a mutex;
- atomic variable protects one value but not the whole invariant;
- detached thread uses objects that already died.

Error-handling bugs:

- swallowing exceptions without action;
- throwing from destructor;
- losing `errno` or diagnostic context;
- mixing exceptions and error codes without a boundary rule;
- cleanup skipped in C-style code.

## 9. Debugging

### Tool Choice

| Problem | Tool |
| --- | --- |
| Compile warning, suspicious conversion, shadowing | `-Wall -Wextra -Wconversion -Wshadow` |
| Crash or wrong control flow | GDB with `bt`, `frame`, `print` |
| Heap use-after-free, buffer overflow, leak | AddressSanitizer, Valgrind |
| Undefined behavior | UndefinedBehaviorSanitizer |
| Data race | ThreadSanitizer |
| Deadlock or hang | logs with thread IDs, GDB thread backtrace, lock-order review |
| Iterator invalidation | code review around insert/erase/growth points |
| Exception cleanup issue | RAII review and failure-path tests |
| API misuse | unit tests for invalid inputs and boundary cases |

### Debugging Questions To Ask

For memory:

- What object owns this memory?
- When is it destroyed?
- Can another pointer still refer to it?
- Can an exception skip cleanup?

For STL:

- Did an insert, erase, resize, or rehash happen?
- Are old iterators, references, or pointers reused?
- Is the comparator valid?
- Is complexity still acceptable?

For concurrency:

- Which data is shared?
- Which mutex or atomic protects it?
- Is the lock held for every access?
- Is the condition variable predicate correct?
- Is the thread lifetime tied to an object lifetime?

For API design:

- Is null allowed?
- Does the function take ownership?
- Does it store the pointer/reference/callback?
- Can the caller see and handle errors?

## 10. Best Practices

- Initialize variables at declaration.
- Prefer RAII over manual cleanup.
- Prefer standard containers and algorithms over raw arrays and hand-written
  loops when constraints allow.
- Use `std::unique_ptr` for exclusive ownership.
- Use `std::shared_ptr` only for true shared ownership.
- Use raw pointers and references for non-owning access only when lifetime is
  clear.
- Treat `std::span`, `std::string_view`, and iterators as non-owning views.
- Avoid global mutable state. If it is unavoidable, document initialization and
  thread-safety.
- Mark overrides with `override`.
- Give polymorphic base classes virtual destructors.
- Prefer composition over inheritance unless the relationship is substitutable
  "is-a".
- Keep interfaces small and intention-revealing.
- Use `const` to express read-only intent.
- Avoid C-style casts in C++.
- Use `noexcept` only when it is truthful.
- Make destructors non-throwing.
- Define copy/move/destructor behavior for resource-owning classes, or follow
  the Rule of Zero.
- Use RAII lock wrappers such as `std::lock_guard` and `std::unique_lock`.
- Keep critical sections small.
- Always wait on `std::condition_variable` with a predicate.
- Do not use `volatile` for thread synchronization.
- Test success paths, failure paths, boundary values, invalid inputs, and
  concurrent behavior.
- Run sanitizer/debug builds as part of review.
- Use design patterns only after identifying the real design pressure.

## 11. Interview Readiness

### Beginner Readiness

You should be able to answer:

- What is the difference between stack and heap?
- What is a dangling pointer?
- What is the difference between pointer and reference?
- Why is out-of-bounds access undefined behavior?
- What is `nullptr` and why is it preferred over `NULL`?
- What is a class, constructor, destructor, and object?
- Why is `std::vector` safer than a raw dynamic array for many cases?

Strong answer pattern:

> A dangling pointer points to an object whose lifetime has ended. It may still
> contain an address, but dereferencing it is undefined behavior. I avoid it by
> keeping ownership clear, using RAII containers or smart pointers, and not
> returning addresses or references to local variables.

### Mid-Level Readiness

You should be able to answer:

- Compare `malloc/free` with `new/delete` and RAII.
- Explain shallow copy vs deep copy.
- Explain Rule of Three, Rule of Five, and Rule of Zero.
- Why does a polymorphic base class need a virtual destructor?
- What is object slicing?
- Explain iterator invalidation for `std::vector`.
- Compare `std::map` and `std::unordered_map`.
- Explain lambda capture lifetime.
- Compare error codes and exceptions.
- Explain `std::mutex`, RAII locks, and deadlock prevention.

Strong answer pattern:

> `std::vector` invalidates iterators and references when it reallocates. Erase
> also invalidates iterators at and after the erase point. In review, I check
> whether code stores iterators or pointers into a vector across `push_back`,
> `insert`, `erase`, or `reserve`.

### Senior Readiness

You should be able to answer:

- How do you review a resource-owning class?
- How do you evaluate exception safety?
- How do you diagnose a data race?
- When is `std::atomic` enough, and when do you need a mutex?
- How do you design a C API boundary for Modern C++?
- When is `std::shared_ptr` justified?
- How do you avoid cyclic ownership?
- How do you choose between runtime polymorphism, templates, callbacks, and a
  simple function?
- How do you simplify over-engineered design patterns?
- How would you structure an enterprise C++ code review checklist?

Strong answer pattern:

> I review a resource-owning class by checking its invariant, ownership member,
> destructor, copy constructor, copy assignment, move constructor, move
> assignment, and exception safety. If the class can use standard RAII members
> like `std::vector`, `std::string`, or `std::unique_ptr`, I prefer Rule of Zero
> so the compiler-generated operations are correct.

### Red Flags In Interview Answers

- "Smart pointers solve all memory problems."
- "`std::shared_ptr` is safer, so use it everywhere."
- "`volatile` makes code thread-safe."
- "An `unordered_map` is always O(1)."
- "A virtual destructor is optional."
- "A lambda capture by reference is fine because it compiles."
- "Factory means any function that creates an object."
- "I start code review by formatting."

These answers miss mechanism and production risk.

## 12. Practice

### Basic Practice

1. Explain stack vs heap using one local variable and one dynamic allocation.
2. Write a function that safely returns `std::string`, then explain why returning
   `std::string_view` to a local string is wrong.
3. Given a raw pointer parameter, label it as owner, borrower, observer, or
   output parameter.
4. Compile a small file with:

```sh
g++ -std=c++17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -g file.cpp -o file
```

### Intermediate Practice

1. Find the bug:

```cpp
#include <vector>

int* bad_pointer() {
    std::vector<int> values{1, 2, 3};
    return values.data();
}
```

Answer: `values` is destroyed when the function returns, so the returned pointer
dangles.

2. Fix this callback:

```cpp
#include <functional>
#include <string>

std::function<void()> bad() {
    std::string text = "done";
    return [&] { (void)text; };
}
```

Hint: capture by value if the callback needs to own the data.

3. Review a class with a raw owning pointer. Decide whether it should use
   `std::unique_ptr`, `std::vector`, `std::string`, or a custom RAII wrapper.

4. Write a `std::condition_variable` wait with a predicate.

### Advanced Practice

1. Review a small module in this order:
   - correctness;
   - ownership/lifetime;
   - exception safety;
   - concurrency;
   - API contract;
   - STL invalidation and complexity;
   - tests.

2. Take a class hierarchy and check:
   - virtual destructor;
   - slicing risk;
   - ownership through base pointer;
   - whether composition would be simpler.

3. Diagnose a race:
   - identify shared mutable state;
   - identify protection;
   - explain whether a mutex or atomic is enough;
   - propose a ThreadSanitizer command.

4. Explain whether a design should use:
   - direct function;
   - lambda;
   - `std::function`;
   - virtual interface;
   - template;
   - named pattern.

## 13. Summary

The final C/C++ checklist is about engineering judgment.

Use it to ask:

- Is the behavior defined and correct?
- Is ownership explicit?
- Can anything dangle?
- Is cleanup exception-safe?
- Are containers and iterators used safely?
- Is concurrency protected by a clear policy?
- Is the API hard to misuse?
- Is the abstraction justified?
- Can I debug this when it fails?
- Can I explain it in an interview with code-level detail?

If you can answer those questions clearly, you are no longer just recognizing
C/C++ keywords. You are thinking like a reviewer and a production engineer.
