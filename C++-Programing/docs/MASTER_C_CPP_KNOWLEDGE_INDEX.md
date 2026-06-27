# MASTER C/C++ KNOWLEDGE INDEX FOR AI

> Purpose: Token-efficient C/C++ knowledge index for Codex / AI Agent / Skill routing.  
> Target: Embedded Engineer, Embedded Linux Engineer, C/C++ Software Engineer.  
> Scope: C/C++ language, memory, OOP, STL, Modern C++, POSIX/Linux comparison, interview and enterprise keyword routing.  
> Out of scope: Linux driver, Yocto, Buildroot, GStreamer, V4L2, AUTOSAR, cloud, project implementation.

---

## 0. How AI Should Use This File

This file is a **knowledge index**, not a full textbook.

AI must use it to:

- Identify required knowledge keywords.
- Determine learning priority.
- Expand only the requested topic/chapter.
- Avoid over-explaining low-priority topics.
- Compare C vs C++ when relevant.
- Compare POSIX/Linux C API vs Modern C++ when relevant.
- Route to trusted references when more detail is needed.

AI must not:

- Treat every keyword as equally important.
- Generate advanced topics before prerequisites.
- Mix this C/C++ roadmap with Linux driver, Yocto, GStreamer, or AUTOSAR.
- Force design patterns where simple design is enough.

---

## 1. Priority System

### MUST

Understand deeply. Required for real work, debugging, code review, and interviews.

### SHOULD

Understand well enough to use, read, and explain.

### NICE

Know concept, use-case, and when to search deeper.

### EXPERT

Learn deeply only for Senior/Architect/library/performance-critical work.

---

## 2. Standard Expansion Template

When the user asks AI to expand any keyword/chapter, use this template:

```md
## <Keyword / Chapter>

### Priority
MUST / SHOULD / NICE / EXPERT

### Prerequisites
...

### Definition
...

### Why It Matters
...

### C Usage
...

### C++ Usage
...

### Embedded Usage
...

### Enterprise Usage
...

### Common Bugs
...

### Best Practices
...

### Interview Focus
...

### Practice Tasks
...

### Trusted References
...
```

Use the full template for MUST topics. Use shorter explanation for SHOULD/NICE topics.

---

## 3. Trusted Reference Routing

Use these references when expanding details:

- cppreference: C/C++ language and standard library reference.
- ISO C / ISO C++ standards: canonical language behavior.
- C++ Core Guidelines: resource management, interfaces, memory, concurrency, RAII.
- SEI CERT C/C++: secure coding, undefined behavior, memory safety.
- MISRA C / MISRA C++: safety-critical and automotive embedded rules.
- BARR-C: embedded C coding practices.
- Refactoring Guru: design patterns and refactoring.
- GeeksForGeeks: beginner-friendly syntax, examples, common interview topic list.
- Compiler docs: GCC/Clang warnings, sanitizers, optimization flags.
- POSIX/Linux man pages: pthread, process, file I/O, socket, select/poll/epoll.

Reference rule:

- For exact language/library behavior: prefer cppreference / ISO.
- For secure coding: prefer SEI CERT.
- For safety-critical embedded rules: prefer MISRA / BARR-C.
- For patterns: prefer Refactoring Guru.
- For POSIX/Linux API: prefer man pages.
- For beginner explanation: GeeksForGeeks is acceptable, but verify with cppreference for precision.

---

## 4. Chapter Dependency Map

```text
CH01 Build & Compilation Model
  -> CH02 C Fundamentals
  -> CH03 C Memory Model
  -> CH04 Pointer Mastery
  -> CH05 Compound Types in C
  -> CH06 Advanced C for Embedded
  -> CH07 Industrial C Practices
  -> CH08 C++ Fundamentals
  -> CH09 OOP in C++
  -> CH10 C++ Resource Management
  -> CH11 STL & Standard Library
  -> CH12 Modern C++
  -> CH13 Templates & Generic Programming
  -> CH14 Error Handling
  -> CH15 Concurrency
  -> CH16 Design Principles & Patterns
  -> CH17 C vs C++ Comparison
  -> CH18 POSIX/Linux C API vs Modern C++
  -> CH19 Enterprise & Interview Checklist
```

---

# CH01 — Build & Compilation Model

Priority: MUST  
Depth: Medium → Deep  
Prerequisites: None

## Keywords

MUST:

- translation unit
- header file
- source file
- declaration vs definition
- preprocessing
- compilation
- assembly
- linking
- loading
- object file
- executable
- symbol
- linker error
- undefined reference
- multiple definition
- static library `.a`
- shared library `.so`
- C linkage
- C++ linkage
- `extern "C"`
- name mangling
- ABI

SHOULD:

- include guard
- `#pragma once`
- inline function in header
- ODR in C++
- static function internal linkage
- visibility
- dynamic loader
- rpath
- soname

EXPERT:

- ABI compatibility
- symbol versioning
- linker script
- LTO
- PGO
- weak symbol

## AI Expansion Rule

For this chapter, explain source-to-executable flow and common build/linking errors. Always compare C linkage vs C++ name mangling when discussing mixed C/C++ projects.

## Interview Focus

- What happens from `.c/.cpp` to executable?
- Declaration vs definition?
- Why undefined reference happens?
- Why multiple definition happens?
- Why C++ needs name mangling?
- What does `extern "C"` do?

---

# CH02 — C Fundamentals

Priority: MUST  
Depth: Deep  
Prerequisites: CH01

## Keywords

MUST:

- `main()`
- statement
- expression
- variable
- declaration
- definition
- initialization
- scope
- lifetime
- storage duration
- data type
- `char`, `short`, `int`, `long`, `long long`
- `float`, `double`
- `_Bool`, `bool`
- `size_t`
- `stdint.h`
- `uint8_t`, `uint16_t`, `uint32_t`, `uint64_t`
- `int8_t`, `int16_t`, `int32_t`, `int64_t`
- arithmetic operators
- relational operators
- logical operators
- bitwise operators
- assignment operators
- conditional operator
- `if`, `else`
- `switch`, `case`, `default`
- `for`, `while`, `do while`
- `break`, `continue`
- function declaration
- function definition
- function parameter
- return value
- `static` function
- recursion

SHOULD:

- integer promotion
- usual arithmetic conversion
- signed vs unsigned comparison
- overflow
- implicit conversion
- explicit cast
- `typedef`
- `const` object
- `volatile` object

NICE:

- sequence point / sequencing
- strict aliasing basics
- implementation-defined type size

## AI Expansion Rule

For beginner lessons, keep syntax simple but always mention real embedded risks: type size, signed/unsigned bug, overflow, uninitialized variable.

## Interview Focus

- Scope vs lifetime.
- `static` local vs global variable.
- `extern` usage.
- Why use `uint32_t` instead of `int`?
- Signed vs unsigned bug.

---

# CH03 — C Memory Model

Priority: MUST  
Depth: Deep  
Prerequisites: CH02

## Keywords

MUST:

- memory layout
- text segment
- read-only data
- data segment
- BSS segment
- stack
- heap
- stack frame
- global variable
- static variable
- local variable
- automatic storage
- static storage
- dynamic storage
- alignment
- padding
- endianness
- little endian
- big endian
- network byte order
- undefined behavior
- unspecified behavior
- implementation-defined behavior
- memory leak
- dangling pointer
- wild pointer
- double free
- use-after-free
- buffer overflow
- stack overflow

SHOULD:

- `size`
- `nm`
- `objdump`
- `readelf`
- `gdb backtrace`
- `gdb frame`
- `gdb info locals`
- optimization level `-O0`, `-O1`, `-O2`, `-O3`, `-Os`

EXPERT:

- strict aliasing
- effective type
- memory barrier
- cache locality
- false sharing

## AI Expansion Rule

For memory topics, always include common bugs and debugging tools. If user asks about `struct`, include padding/alignment. If user asks about network/protocol, include endianness.

## Interview Focus

- Stack vs heap.
- Data vs BSS.
- Why BSS is zero-initialized?
- What is undefined behavior?
- What is memory leak?
- How to debug segmentation fault?

---

# CH04 — Pointer Mastery

Priority: MUST  
Depth: Deep  
Prerequisites: CH03

## Keywords

MUST:

- pointer
- address
- dereference
- null pointer
- `NULL`
- `nullptr`
- pointer arithmetic
- pointer vs array
- array decay
- pointer to pointer
- double pointer
- void pointer
- function pointer
- pointer to array
- array of pointers
- pointer to const
- const pointer
- const pointer to const
- dangling pointer
- wild pointer
- invalid pointer
- use-after-free
- out-of-bounds access

SHOULD:

- callback
- output parameter
- ownership
- borrowed pointer
- pointer aliasing
- `restrict`
- pointer alignment
- opaque pointer
- handle pattern

EXPERT:

- strict aliasing violation
- custom allocator pointer ownership
- pointer provenance

## Must Compare

- pointer vs reference
- pointer vs array
- `NULL` vs `nullptr`
- raw pointer vs smart pointer
- `const int *p` vs `int * const p` vs `const int * const p`

## AI Expansion Rule

For pointer topics, always include diagrams or memory mental model if possible. Always include common crash cases: null dereference, dangling pointer, buffer overflow, use-after-free.

## Interview Focus

- Pointer vs array.
- Double pointer use cases.
- Function pointer syntax.
- Callback in C.
- Const pointer variations.
- Why modifying string literal is undefined behavior?

---

# CH05 — Compound Types in C

Priority: MUST  
Depth: Deep  
Prerequisites: CH04

## Keywords

MUST:

- array
- 1D array
- multi-dimensional array
- string
- null-terminated string
- string literal
- character array
- `strlen`
- `strcpy`
- `strncpy`
- `strcmp`
- `strcat`
- `snprintf`
- `fgets`
- struct
- nested struct
- struct padding
- struct alignment
- union
- enum
- typedef

SHOULD:

- flexible array member
- packed struct
- bit-field
- anonymous struct/union
- designated initializer
- compound literal

NICE:

- X-macro for enum/string mapping
- intrusive data structure in C

## Must Compare

- `struct` in C vs `struct` in C++
- `union` in C vs `union` / `std::variant` in C++
- `enum` in C vs `enum class` in C++
- C string vs `std::string` vs `std::string_view`
- C array vs `std::array` vs `std::vector`

## AI Expansion Rule

For compound types, emphasize memory layout. For string topics, emphasize buffer size, null terminator, unsafe functions, and secure alternatives.

## Interview Focus

- Struct padding size calculation.
- Struct vs union.
- Enum size and type safety.
- C string buffer overflow.
- Why `gets()` is dangerous?

---

# CH06 — Advanced C for Embedded

Priority: MUST for Embedded, SHOULD for general Software  
Depth: Deep for embedded  
Prerequisites: CH05

## Keywords

MUST:

- bitwise operation
- bit mask
- set bit
- clear bit
- toggle bit
- check bit
- register access
- memory-mapped I/O
- `volatile`
- `const`
- `static`
- `extern`
- `restrict`
- macro
- function-like macro
- conditional compilation
- include guard
- callback
- function pointer table
- dispatch table
- finite state machine
- FSM
- enum state
- state transition table
- OOP in C
- HAL abstraction
- ops table
- opaque pointer
- private data

SHOULD:

- bit-field portability
- packed struct risk
- endian conversion
- register map modeling
- command dispatcher
- ring buffer
- logger macro
- variadic function
- `stdarg.h`
- `va_start`, `va_arg`, `va_end`

NICE:

- compile-time assertions in C
- generic C macro patterns
- intrusive linked list

## Must Compare

- `volatile` vs atomic
- macro vs inline function
- macro vs `constexpr` in C++
- function pointer vs `std::function` / lambda
- FSM in C vs State Pattern in C++
- OOP in C vs OOP in C++

## AI Expansion Rule

For embedded C topics, show why the keyword exists in real firmware/middleware: registers, protocol, hardware abstraction, callback, state machine.

## Interview Focus

- Is `volatile` thread-safe?
- How to access hardware register safely?
- How to implement polymorphism in C?
- How to design HAL interface in C?
- How to implement FSM using enum and function pointer table?

---

# CH07 — Industrial C Practices

Priority: SHOULD, some MUST for embedded enterprise  
Depth: Medium  
Prerequisites: CH06

## Keywords

MUST:

- coding standard
- MISRA C
- BARR-C
- SEI CERT C
- undefined behavior avoidance
- defensive programming
- input validation
- return value check
- error code
- `errno`
- `assert`
- logging
- log level
- safe string handling
- secure I/O
- static analysis
- dynamic analysis
- unit test
- mocking

SHOULD:

- Doxygen
- `@brief`, `@param`, `@return`
- `cppcheck`
- `clang-tidy`
- sanitizer
- AddressSanitizer
- UndefinedBehaviorSanitizer
- ThreadSanitizer
- Valgrind
- `gdb`
- `strace`
- `ltrace`
- `perf`
- Makefile
- CMake
- Unity
- CMock
- FFF
- Google Test for C modules
- CI basics

NICE:

- formal verification basics
- fuzzing basics
- mutation testing

## AI Expansion Rule

For industrial practice, keep explanation practical: rule, reason, bug prevented, tool, example.

## Interview Focus

- Why coding standard matters?
- MISRA purpose?
- How to test hardware-dependent code?
- Static analysis vs dynamic analysis?
- How to avoid buffer overflow?

---

# CH08 — C++ Fundamentals

Priority: MUST  
Depth: Deep  
Prerequisites: CH05

## Keywords

MUST:

- C vs C++
- namespace
- reference
- lvalue reference
- const reference
- class
- object
- access specifier
- public
- private
- protected
- constructor
- default constructor
- parameterized constructor
- copy constructor
- destructor
- static member
- friend function
- friend class
- function overloading
- operator overloading
- name mangling

SHOULD:

- default member initializer
- delegating constructor
- explicit constructor
- initializer list
- aggregate initialization
- copy elision

NICE:

- uniform initialization edge cases
- ADL basics

## Must Compare

- pointer vs reference
- struct vs class in C++
- constructor vs init function in C
- destructor vs manual cleanup in C
- function overloading vs C naming convention

## AI Expansion Rule

For C++ fundamentals, always contrast with C when useful. Emphasize object lifetime and initialization.

## Interview Focus

- Reference vs pointer.
- Class vs struct.
- Constructor vs destructor.
- Function overloading vs overriding.
- Why C++ has name mangling?

---

# CH09 — OOP in C++

Priority: MUST  
Depth: Deep  
Prerequisites: CH08

## Keywords

MUST:

- encapsulation
- abstraction
- inheritance
- polymorphism
- compile-time polymorphism
- runtime polymorphism
- virtual function
- vtable
- vptr
- pure virtual function
- abstract class
- interface
- virtual destructor
- override
- final
- object slicing
- composition
- aggregation
- association
- dependency
- composition over inheritance

SHOULD:

- multiple inheritance
- virtual inheritance
- diamond problem
- interface segregation
- dependency inversion

NICE:

- mixin
- CRTP as static polymorphism

## Must Compare

- OOP in C vs OOP in C++
- inheritance vs composition
- overloading vs overriding
- abstract class vs interface
- virtual dispatch vs function pointer table

## AI Expansion Rule

For OOP, avoid over-engineering. Explain problem first, then design, then trade-off.

## Interview Focus

- Encapsulation vs abstraction.
- Why virtual destructor?
- What is vtable/vptr?
- What is object slicing?
- Composition vs inheritance.

---

# CH10 — C++ Resource Management

Priority: MUST  
Depth: Deep  
Prerequisites: CH09

## Keywords

MUST:

- RAII
- object lifetime
- ownership
- constructor acquisition
- destructor release
- copy constructor
- copy assignment
- move constructor
- move assignment
- Rule of 3
- Rule of 5
- Rule of 0
- shallow copy
- deep copy
- `new`
- `delete`
- `new[]`
- `delete[]`
- smart pointer
- `std::unique_ptr`
- `std::shared_ptr`
- `std::weak_ptr`
- custom deleter

SHOULD:

- placement new
- allocator concept
- polymorphic allocator
- `std::make_unique`
- `std::make_shared`
- shared ownership cycle
- pImpl idiom
- RAII wrapper for file descriptor
- RAII wrapper for mutex lock

EXPERT:

- custom allocator
- memory pool
- arena allocator
- small object optimization

## Must Compare

- `malloc/free` vs `new/delete`
- raw pointer vs smart pointer
- `unique_ptr` vs `shared_ptr` vs `weak_ptr`
- manual cleanup vs RAII
- shallow copy vs deep copy
- Rule of 3 vs Rule of 5 vs Rule of 0

## AI Expansion Rule

For resource management, always include ownership model and cleanup path. If user shows code with raw pointer, check for leak/double-free/copy bug.

## Interview Focus

- Why not mix `malloc/free` with `new/delete`?
- Why RAII matters?
- Rule of 3/5/0.
- `unique_ptr` vs `shared_ptr`.
- Why `weak_ptr` exists?

---

# CH11 — STL & Standard Library

Priority: MUST for common containers, SHOULD for advanced details  
Depth: Medium → Deep  
Prerequisites: CH10

## Keywords

MUST:

- STL
- container
- iterator
- algorithm
- comparator
- functor
- lambda comparator
- `std::array`
- `std::vector`
- `std::deque`
- `std::list`
- `std::forward_list`
- `std::stack`
- `std::queue`
- `std::priority_queue`
- `std::map`
- `std::set`
- `std::multimap`
- `std::multiset`
- `std::unordered_map`
- `std::unordered_set`
- `std::sort`
- `std::find`
- `std::transform`
- `std::accumulate`
- `std::lower_bound`
- `std::upper_bound`
- `std::binary_search`
- iterator invalidation
- time complexity

SHOULD:

- `std::string`
- `std::string_view`
- `std::span`
- `std::filesystem`
- `std::chrono`
- allocator awareness
- erase-remove idiom
- custom hash
- custom comparator

NICE:

- ranges
- views
- parallel algorithms
- polymorphic memory resource

## Must Compare

- C array vs `std::array` vs `std::vector`
- `vector` vs `list`
- `map` vs `unordered_map`
- `set` vs `unordered_set`
- iterator vs raw pointer
- `std::string` vs `char*`

## AI Expansion Rule

For STL topics, always mention complexity, memory behavior, iterator invalidation, and when not to use.

## Interview Focus

- Why vector is often better than list?
- Map vs unordered_map.
- Iterator invalidation.
- Custom comparator.
- Complexity of insert/search/erase.

---

# CH12 — Modern C++

Priority: MUST for C++11/14/17 basics, SHOULD for C++20/23  
Depth: Medium → Deep  
Prerequisites: CH11

## Keywords

MUST:

- modern C++
- `auto`
- `decltype`
- `constexpr`
- lambda
- lambda capture
- range-based for
- move semantics
- lvalue
- rvalue
- rvalue reference
- `std::move`
- `noexcept`
- `nullptr`
- `override`
- `final`
- smart pointer
- `std::optional`
- `std::variant`
- `std::string_view`

SHOULD:

- structured binding
- `if constexpr`
- `std::any`
- `std::span`
- `std::filesystem`
- `std::chrono`
- perfect forwarding
- forwarding reference
- `std::forward`
- `constexpr` function
- `consteval`
- `constinit`
- concepts
- ranges basics
- `std::expected`

NICE:

- coroutine
- modules
- spaceship operator
- ranges advanced

## Must Compare

- `constexpr` vs `const` vs macro
- `std::optional` vs nullable pointer
- `std::variant` vs union
- `std::string_view` vs `std::string`
- move vs copy
- lambda vs function pointer vs `std::function`

## AI Expansion Rule

For modern C++, explain safety and readability benefit, but also mention lifetime/performance traps.

## Interview Focus

- lvalue vs rvalue.
- What does `std::move` do?
- Lambda capture by value vs reference.
- `constexpr` vs `const`.
- `string_view` lifetime issue.

---

# CH13 — Templates & Generic Programming

Priority: SHOULD, selected basics MUST  
Depth: Medium, advanced topics on demand  
Prerequisites: CH12

## Keywords

MUST:

- function template
- class template
- template parameter
- template instantiation
- template specialization basics

SHOULD:

- non-type template parameter
- variadic template
- type traits
- `std::is_same`
- `std::is_integral`
- `std::enable_if`
- SFINAE basics
- concepts basics
- generic lambda

NICE:

- partial specialization
- template template parameter
- tag dispatch
- CRTP
- policy-based design

EXPERT:

- template metaprogramming
- expression templates
- advanced SFINAE
- compile-time reflection when available

## Must Compare

- template vs macro
- template vs runtime polymorphism
- concepts vs SFINAE
- CRTP vs virtual function

## AI Expansion Rule

Do not over-teach template metaprogramming for beginners. For practical engineers, focus first on function/class templates, type traits basics, and reading template errors.

## Interview Focus

- Why use templates?
- Template specialization.
- What is SFINAE?
- What are concepts?
- Template vs virtual polymorphism.

---

# CH14 — Error Handling

Priority: MUST  
Depth: Deep  
Prerequisites: CH07, CH12

## Keywords

MUST:

- return code
- status code
- error enum
- `errno`
- `perror`
- `strerror`
- `assert`
- `static_assert`
- exception
- `try`
- `catch`
- `throw`
- stack unwinding
- destructor during exception
- RAII cleanup
- `noexcept`
- defensive programming

SHOULD:

- `std::optional`
- `std::expected`
- `Result<T,E>` pattern
- error category
- exception safety guarantee
- basic guarantee
- strong guarantee
- no-throw guarantee

NICE:

- monadic error handling
- domain-specific error model

## Must Compare

- C return code vs C++ exception
- `errno` vs exception
- `assert` vs runtime error
- exception vs `std::expected`
- manual cleanup vs RAII cleanup

## AI Expansion Rule

For embedded contexts, mention that exceptions may be restricted/disabled. Prefer predictable return code + RAII when project standard requires it.

## Interview Focus

- Return code vs exception.
- Why embedded may avoid exception?
- What is exception safety?
- What is `noexcept`?
- What happens if destructor throws?

---

# CH15 — Concurrency

Priority: MUST for multithreaded projects  
Depth: Deep for common primitives, Expert for memory model  
Prerequisites: CH12

## Keywords

MUST:

- thread
- `std::thread`
- join
- detach
- mutex
- `std::mutex`
- `std::lock_guard`
- `std::unique_lock`
- condition variable
- `std::condition_variable`
- predicate wait
- spurious wakeup
- semaphore
- atomic
- `std::atomic`
- race condition
- data race
- deadlock
- livelock
- producer-consumer
- thread-safe queue
- thread pool

SHOULD:

- `std::future`
- `std::promise`
- `std::async`
- `std::shared_mutex`
- `std::recursive_mutex`
- `std::timed_mutex`
- `std::scoped_lock`
- memory ordering basics
- acquire/release
- relaxed ordering
- sequential consistency
- false sharing

EXPERT:

- lock-free programming
- ABA problem
- hazard pointer
- epoch reclamation
- memory model deep dive

## Must Compare

- `pthread` vs `std::thread`
- `pthread_mutex_t` vs `std::mutex`
- `pthread_cond_t` vs `std::condition_variable`
- `sem_t` vs `std::counting_semaphore`
- `volatile` vs atomic
- mutex vs semaphore
- lock_guard vs unique_lock

## AI Expansion Rule

For concurrency, always include correctness first, performance second. Warn about data race, deadlock, spurious wakeup, and volatile misuse.

## Interview Focus

- Race condition vs data race.
- How to avoid deadlock?
- Mutex vs semaphore.
- Atomic vs volatile.
- Spurious wakeup.
- Thread pool design.

---

# CH16 — Design Principles & Design Patterns

Priority: Mixed  
Depth: Based on priority  
Prerequisites: CH09

## Design Principles

MUST:

- SOLID
- SRP
- OCP
- LSP
- ISP
- DIP
- DRY
- KISS
- YAGNI
- high cohesion
- low coupling
- composition over inheritance

## Pattern Priority

### MUST Patterns

Learn deeply because they appear often in embedded, middleware, and C++ production code.

- State / FSM
- Strategy
- Observer
- Factory Method
- Adapter
- Facade
- Command

### SHOULD Patterns

Understand use-case and implementation.

- Builder
- Decorator
- Proxy
- Template Method
- Chain of Responsibility
- Mediator
- Iterator
- Composite
- Prototype

### NICE Patterns

Know concept and when to search deeper.

- Visitor
- Memento
- Flyweight
- Bridge
- Abstract Factory

## Must Compare

- callback vs Observer
- FSM in C vs State Pattern in C++
- Strategy vs State
- Adapter vs Facade
- Factory Method vs Abstract Factory
- Decorator vs Proxy
- inheritance vs composition

## AI Expansion Rule

Do not teach all patterns equally deeply. Explain real problem first. Prefer simple solution first. Use pattern only when it reduces complexity.

## Interview Focus

- State/FSM in embedded.
- Callback vs Observer.
- Factory Method use case.
- Adapter for wrapping C API.
- Facade for hiding subsystem complexity.

---

# CH17 — C vs C++ Comparison Matrix

Priority: MUST  
Depth: Deep for listed pairs  
Prerequisites: CH05, CH08, CH10

## Required Comparisons

AI must compare these pairs when relevant:

### Data Modeling

- `struct` in C vs `struct` in C++
- `struct` vs `class` in C++
- `union` in C vs `union` in C++ vs `std::variant`
- `enum` in C vs `enum class` in C++
- `typedef` in C vs `using` in C++

### Memory

- `malloc` vs `calloc`
- `malloc/calloc/realloc/free` vs `new/delete`
- `free` vs `delete`
- `new[]/delete[]` vs `std::vector`
- manual cleanup vs RAII
- raw pointer vs smart pointer
- stack object vs heap object
- shallow copy vs deep copy

### String / Array

- C string vs `std::string`
- `char*` vs `std::string_view`
- C array vs `std::array`
- C dynamic array vs `std::vector`

### Function / Callback

- function pointer vs lambda
- function pointer vs `std::function`
- C callback vs C++ observer/callback object
- macro vs inline function
- macro vs `constexpr`
- macro vs template

### OOP / Design

- OOP in C vs OOP in C++
- function pointer table vs virtual function
- ops table vs interface
- inheritance vs composition
- virtual dispatch vs static polymorphism

### Error Handling

- return code vs exception
- `errno` vs exception
- `assert` vs exception
- exception vs `std::expected` / Result type

## AI Expansion Rule

When explaining any comparison, use this compact table:

```md
| Topic | C | C++ | Enterprise / Embedded Guidance |
|---|---|---|---|
```

Then add:

- When to use C style.
- When to use C++ style.
- Common bug.
- Interview question.

---

# CH18 — POSIX/Linux C API vs Modern C++

Priority: MUST for Embedded Linux / Linux Software  
Depth: Medium → Deep  
Prerequisites: CH07, CH12, CH15

## Required Comparisons

### Threading

- `pthread_create` vs `std::thread`
- `pthread_join` vs `std::thread::join`
- `pthread_detach` vs `std::thread::detach`
- `void* arg` vs lambda/functor/member function

### Synchronization

- `pthread_mutex_t` vs `std::mutex`
- `pthread_mutex_lock/unlock` vs `std::lock_guard`
- `pthread_cond_t` vs `std::condition_variable`
- `sem_t` vs `std::counting_semaphore`
- POSIX semaphore vs mutex + condition_variable

### Process

- `fork`
- `exec`
- `wait`
- `waitpid`
- `pipe`
- `dup2`
- No direct C++ standard process API
- C++ RAII wrapper for process/file descriptor

### File I/O

- `open/read/write/close` vs `std::fstream`
- `fopen/fread/fwrite/fclose` vs `std::fstream`
- file descriptor vs stream
- `ioctl` has no C++ standard equivalent
- `std::filesystem`
- RAII file descriptor wrapper

### Socket / Event Loop

- `socket/bind/listen/accept/connect`
- `send/recv`
- `select/poll/epoll`
- `getaddrinfo`
- No common C++ standard socket API
- Boost.Asio / custom wrapper

### Time

- `sleep/usleep/nanosleep` vs `std::this_thread::sleep_for`
- `clock_gettime` vs `std::chrono`
- manual unit vs type-safe duration

### Atomic / Volatile

- C11 `_Atomic` vs `std::atomic`
- GCC `__sync_*`, `__atomic_*` vs `std::atomic`
- `volatile` vs atomic

## AI Expansion Rule

Always clarify: pthread/fork/socket/open/ioctl are POSIX/Linux APIs, not core C language. `std::thread`, `std::mutex`, `std::chrono`, `std::filesystem` are C++ standard library APIs. C++ on Linux often still uses POSIX APIs wrapped with RAII.

## Interview Focus

- Is `fork()` part of C++?
- Does C++ standard library have sockets?
- Why still use `ioctl` in C++?
- Why wrap file descriptor in RAII class?
- `pthread` vs `std::thread`.
- `volatile` vs `std::atomic`.

---

# CH19 — Enterprise & Interview Checklist

Priority: MUST  
Depth: Checklist  
Prerequisites: All relevant chapters

## Memory Checklist

MUST:

- stack vs heap
- data vs BSS
- alignment
- padding
- endianness
- `malloc` vs `calloc`
- `malloc` vs `new`
- `free` vs `delete`
- `new[]` vs `delete[]`
- memory leak
- dangling pointer
- double free
- use-after-free
- buffer overflow
- shallow copy vs deep copy
- RAII

## Pointer Checklist

MUST:

- pointer vs array
- pointer arithmetic
- double pointer
- function pointer
- void pointer
- const pointer
- pointer to const
- null pointer
- dangling pointer
- `nullptr` vs `NULL`

## OOP Checklist

MUST:

- encapsulation
- abstraction
- inheritance
- polymorphism
- virtual function
- vtable/vptr
- pure virtual function
- abstract class
- interface
- virtual destructor
- object slicing
- composition vs inheritance

## STL Checklist

MUST:

- `vector`
- `array`
- `map`
- `set`
- `unordered_map`
- `unordered_set`
- `stack`
- `queue`
- `priority_queue`
- iterator
- algorithm
- comparator
- iterator invalidation
- complexity

## Modern C++ Checklist

MUST:

- RAII
- smart pointer
- move semantics
- lambda
- `auto`
- `constexpr`
- `noexcept`
- `std::optional`
- `std::variant`
- `std::string_view`
- `std::span`

## Concurrency Checklist

MUST:

- thread
- mutex
- lock
- condition variable
- semaphore
- atomic
- race condition
- deadlock
- spurious wakeup
- producer-consumer
- thread pool

## Design Pattern Checklist

MUST:

- State / FSM
- Strategy
- Observer
- Factory Method
- Adapter
- Facade
- Command

SHOULD:

- Builder
- Decorator
- Proxy
- Template Method
- Chain of Responsibility
- Mediator
- Iterator
- Composite
- Prototype

NICE:

- Visitor
- Memento
- Flyweight
- Bridge
- Abstract Factory

---

# CH20 — AI Codex / Agent Rules

Priority: MUST  
Depth: Operational rules

## Agent Role

Act as:

```text
Senior C/C++ Embedded Software Engineer
+ Technical Writer
+ Code Reviewer
+ Interview Coach
```

## Rule 1 — Follow Priority

```text
MUST    -> deep explanation + examples + bugs + best practices + interview
SHOULD  -> concept + usage + short examples + trade-offs
NICE    -> overview + use-case + when to learn more
EXPERT  -> advanced note + warning + references
```

## Rule 2 — Expand Only What Is Needed

Do not expand every keyword. Expand only:

- user-requested keyword
- prerequisite needed for understanding
- comparison needed for correctness
- bug/best practice needed for real code

## Rule 3 — Compare When Required

Always compare when topic involves:

```text
struct, union, enum, array, string, pointer,
malloc, calloc, realloc, free, new, delete,
callback, function pointer, lambda, std::function,
macro, inline, constexpr, template,
thread, mutex, condition_variable, semaphore,
atomic, volatile, file I/O, socket, error handling,
RAII, smart pointer, OOP in C, OOP in C++
```

## Rule 4 — Prefer Practical Embedded/Enterprise Examples

Preferred examples:

- device abstraction
- HAL interface
- logger
- FSM
- callback
- ring buffer
- thread-safe queue
- RAII file descriptor wrapper
- protocol packet parser
- sensor interface
- command dispatcher

Avoid over-academic examples unless requested.

## Rule 5 — Do Not Overuse Design Patterns

For design patterns:

- explain problem first
- show simple solution first
- apply pattern only if it improves maintainability
- mention trade-offs
- do not teach all patterns equally deeply

## Rule 6 — Keep Language and Platform Separate

This file covers:

- C
- C++
- STL
- Modern C++
- C/POSIX/Linux comparison needed by C++ engineers

Separate files should cover:

- Linux System Programming
- Embedded Linux
- Linux Device Driver
- GStreamer / V4L2
- AUTOSAR Adaptive
- System Design for Embedded

---

# Final Learning Order

```text
01. Build and compilation model
02. C fundamentals
03. C memory model
04. Pointer mastery
05. Compound types in C
06. Advanced C for embedded
07. Industrial C practices
08. C++ fundamentals
09. OOP in C++
10. Resource management in C++
11. STL and standard library
12. Modern C++
13. Error handling
14. Concurrency
15. C vs C++ comparison
16. POSIX/Linux C API vs Modern C++ comparison
17. Design principles and design patterns by priority
18. Enterprise and interview checklist
19. AI Codex / Agent rules
```

---

# Final Principle

```text
Do not learn everything equally deeply.
Learn deeply what affects real code, debugging, maintenance, and interviews.
Learn enough for what appears occasionally.
Know concepts for rare or expert-level topics.
Use trusted references when expanding details.
```
