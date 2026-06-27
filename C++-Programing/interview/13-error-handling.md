# 13 - Error Handling: Interview Pack

## How To Use This Pack

For each question:

1. give the short answer first;
2. explain the mechanism and tradeoff;
3. anchor the answer in a C or C++ API/code example;
4. connect it to production behavior or debugging;
5. name traps explicitly;
6. answer follow-ups without weakening the original claim.

The examples use C++17 unless marked otherwise. `std::expected` is C++23; for
earlier standards, discuss a project `Result<T, E>` type.

## Beginner Questions

### 1. What is error handling, and why is it not the same as printing an error message?

**Short answer**

Error handling is the design of how a program detects failure, reports it,
preserves valid state, releases resources, and decides whether to recover,
propagate, or stop. Printing a message is only one possible reporting action.

**Deep explanation**

A failure path must answer several questions: what failed, who owns recovery,
what resources were acquired, what object invariants remain true, and what
diagnostic context is needed. Good code separates detection from policy. A
low-level parser may detect invalid input, but a higher-level command handler
may decide whether to retry, use defaults, reject the command, or stop.

**C/C++ code/API anchor**

```cpp
#include <stdexcept>
#include <string>

int parse_port(const std::string& text)
{
    int port = std::stoi(text);
    if (port <= 0 || port > 65535) {
        throw std::out_of_range("port outside TCP range");
    }
    return port;
}
```

The function reports a failure with a typed exception. The caller decides policy.

**Production/debug angle**

In production logs, "failed" is weak. Useful diagnostics include operation,
input/resource identity, error category, and recovery decision.

**Common traps**

- Treating logging as handling.
- Continuing after an invalid state.
- Catching all exceptions and returning success.
- Reporting an error but leaking resources.

**Follow-up questions**

- Who should decide whether to retry?
- What context should be included in a file-open error?
- How can logging hide bugs if it is used as a substitute for handling?

### 2. Compare C return codes and C++ exceptions.

**Short answer**

Return codes are explicit and predictable, but callers can ignore them.
Exceptions separate normal flow from error flow and propagate automatically, but
they require RAII and a clear exception policy.

**Deep explanation**

C-style APIs usually return `int`, `bool`, or an enum status. This is good for C
ABI boundaries, embedded systems, expected failures, and deterministic control
flow. C++ exceptions are useful for rare failures, constructor failure, and
errors that should cross several stack frames before being handled.

| Topic | C | C++ | Enterprise / Embedded Guidance |
| --- | --- | --- | --- |
| Error report | Return code/status enum | `throw` typed exception | Use explicit status for expected failures and no-exception builds; use exceptions for rare cross-layer failures |
| Propagation | Manual at each call | Automatic up the stack | Catch at meaningful boundaries |
| Cleanup | Manual or cleanup block | RAII during unwinding | RAII is still valuable even when exceptions are disabled |

**C/C++ code/API anchor**

```c
typedef enum { PARSE_OK, PARSE_BAD_DIGIT } ParseStatus;
ParseStatus parse_u8(const char* text, unsigned* out);
```

```cpp
unsigned parse_u8(std::string_view text); // may throw std::invalid_argument
```

**Production/debug angle**

Review the API boundary. If C code calls C++ code, throwing across a C ABI
boundary is usually unacceptable; translate to a status code at the boundary.

**Common traps**

- Saying exceptions are always better.
- Saying return codes are always safer.
- Mixing both mechanisms for the same failure without documentation.
- Ignoring return values.

**Follow-up questions**

- Why are exceptions natural for constructor failure?
- Why might embedded code avoid exceptions?
- How would you wrap a C API in a C++ interface?

### 3. What is `errno`, and when is it valid to read it?

**Short answer**

`errno` is a diagnostic variable used by many C/POSIX-style APIs after they
report failure. Read it only after an API failed and only when that API documents
that `errno` is meaningful.

**Deep explanation**

`errno` is not a universal last-error value. Successful calls do not generally
reset it. Another library call may overwrite it. Therefore, the correct pattern
is: call the API, check the documented failure return, save `errno`
immediately, then report or translate it with context.

**C/C++ code/API anchor**

```c
#include <errno.h>
#include <stdio.h>
#include <string.h>

FILE* file = fopen("config.txt", "r");
if (file == NULL) {
    int saved = errno;
    fprintf(stderr, "open config.txt failed: %s\n", strerror(saved));
}
```

**Production/debug angle**

Always log the operation and resource, not only `strerror(errno)`. "Permission
denied while opening /etc/app/config" is much more useful than "Permission
denied."

**Common traps**

- Reading `errno` after a successful call.
- Reading `errno` long after the failing call.
- Forgetting that not every C function uses `errno`.
- Translating `errno` to a generic exception and losing the file path or
  operation.

**Follow-up questions**

- Why should you save `errno` immediately?
- How is `errno` different from a C++ exception object?
- What should a C++ wrapper preserve when translating `errno`?

### 4. What is the difference between `assert` and runtime error handling?

**Short answer**

`assert` checks programmer assumptions, usually in debug builds. Runtime error
handling reports failures that can happen in real executions, such as bad input,
missing files, device errors, allocation failure, or communication timeouts.

**Deep explanation**

An assertion says "if this fails, the code is wrong." Runtime validation says
"this can fail because the world is imperfect." `assert` may be disabled when
`NDEBUG` is defined, so it must not be the only protection against external
input or recoverable operational failure.

**C/C++ code/API anchor**

```cpp
#include <cassert>
#include <stdexcept>

void set_index(int index, int size)
{
    assert(size >= 0); // internal invariant

    if (index < 0 || index >= size) {
        throw std::out_of_range("index");
    }
}
```

**Production/debug angle**

Use assertions to catch developer bugs early. Use runtime checks for values from
users, files, networks, devices, environment variables, and other processes.

**Common traps**

- Using `assert(file.is_open())` for a real file open.
- Assuming assertions run in release builds.
- Replacing clear error handling with fail-fast checks in recoverable paths.

**Follow-up questions**

- What is `static_assert` for?
- Should invalid user input be handled with `assert`?
- What happens to `assert` when `NDEBUG` is defined?

## Mid-Level Questions

### 5. What happens during stack unwinding?

**Short answer**

When an exception propagates, C++ exits scopes and destroys fully constructed
local objects in reverse construction order until it finds a matching handler or
terminates.

**Deep explanation**

Stack unwinding is the mechanism that makes RAII work with exceptions. If a
function owns resources through local objects, their destructors run even when
control leaves through an exception. If an object constructor throws, the
object's destructor is not called because the object was never fully
constructed, but already constructed members and base subobjects are destroyed.

**C/C++ code/API anchor**

```cpp
#include <iostream>
#include <stdexcept>

struct Trace {
    const char* name;
    ~Trace() { std::cout << "destroy " << name << '\n'; }
};

void f()
{
    Trace a{"a"};
    Trace b{"b"};
    throw std::runtime_error("fail");
}
```

When `f()` throws, `b` is destroyed before `a`.

**Production/debug angle**

If cleanup does not happen during exceptions, inspect ownership. Raw handles,
manual `new`, and manual `lock()` are warning signs; RAII wrappers are the fix.

**Common traps**

- Thinking stack unwinding destroys partially constructed complete objects.
- Assuming destructors run if `std::terminate` is called before unwinding.
- Relying on manual cleanup after a throwing call.

**Follow-up questions**

- What happens to fully constructed members if a constructor throws?
- Why is RAII important for stack unwinding?
- What can prevent normal unwinding?

### 6. What are the basic, strong, and no-throw exception safety guarantees?

**Short answer**

The basic guarantee means no leaks and valid state. The strong guarantee means
the operation either succeeds fully or has no effect. The no-throw guarantee
means the operation does not throw.

**Deep explanation**

Exception safety is about object state after failure. The basic guarantee is the
minimum acceptable production target. The strong guarantee is useful for
transaction-like updates. The no-throw guarantee is required or strongly
preferred for destructors, deallocation, `swap`, and move operations that are
used during rollback or container reallocation.

**C/C++ code/API anchor**

```cpp
#include <stdexcept>
#include <string>
#include <vector>

class Lines {
public:
    void replace_all(std::vector<std::string> next)
    {
        for (const auto& line : next) {
            if (line.empty()) {
                throw std::runtime_error("empty line");
            }
        }
        lines_.swap(next); // commit after throwing work is done
    }

private:
    std::vector<std::string> lines_;
};
```

This aims at the strong guarantee: validate temporary state first, commit last.

**Production/debug angle**

During review, place a mental "throw here" after every allocation, copy, parse,
lock, and write. Check whether invariants and ownership still survive.

**Common traps**

- Saying "exception-safe" when the code only catches exceptions.
- Updating object state before operations that may throw.
- Writing destructors or rollback paths that can throw.

**Follow-up questions**

- Which guarantee should destructors provide?
- Why can copy-and-swap help assignment?
- When is the strong guarantee too expensive?

### 7. Why should exceptions be caught by `const&`?

**Short answer**

Catch exceptions by `const&` to avoid slicing, preserve dynamic type, avoid
extra copies, and still allow polymorphic handling through `std::exception`.

**Deep explanation**

If you catch a derived exception by value as `std::exception`, the derived part
can be sliced away. Catching by reference preserves the actual exception object.
Use specific handlers first, then general handlers.

**C/C++ code/API anchor**

```cpp
try {
    throw std::runtime_error("database unavailable");
} catch (const std::runtime_error& e) {
    // specific recovery
} catch (const std::exception& e) {
    // general fallback
}
```

**Production/debug angle**

Slicing loses diagnostic behavior and category. It can turn a recoverable
domain error into a vague base exception.

**Common traps**

- Catching `std::exception e` by value.
- Catching `std::exception` before `std::runtime_error`.
- Catching `...` and hiding the problem.
- Throwing raw strings or integers instead of meaningful exception types.

**Follow-up questions**

- Why should catch blocks be ordered specific to general?
- When is `catch (...)` appropriate?
- Should exception objects be mutable in catch blocks?

### 8. What is `noexcept`, and what happens if a `noexcept` function throws?

**Short answer**

`noexcept` is a promise that a function will not let an exception escape. If
that promise is broken, the program calls `std::terminate`.

**Deep explanation**

`noexcept` documents a no-throw guarantee and enables optimizations. It is
especially important for destructors, deallocation, `swap`, and move operations.
But it must be true. A function that allocates, constructs strings, writes to a
throwing stream, or calls unknown callbacks usually cannot be blindly marked
`noexcept`.

**C/C++ code/API anchor**

```cpp
struct Handle {
    Handle(Handle&& other) noexcept : id(other.id)
    {
        other.id = -1;
    }

    void swap(Handle& other) noexcept
    {
        int temp = id;
        id = other.id;
        other.id = temp;
    }

    int id{-1};
};
```

**Production/debug angle**

If a program mysteriously terminates instead of reaching a `catch`, check for a
throw escaping from a `noexcept` function or destructor. Break on
`std::terminate` in the debugger.

**Common traps**

- Treating `noexcept` as an optimization hint only.
- Marking a function `noexcept` because failure is unlikely.
- Forgetting that a function can throw indirectly through allocation or called
  code.
- Assuming a nearby `catch` can catch a violation of `noexcept`.

**Follow-up questions**

- Why should move constructors often be `noexcept`?
- What is conditional `noexcept`?
- Should destructors be `noexcept`?

### 9. How should file I/O errors be handled with C++ streams?

**Short answer**

Check stream state or enable stream exceptions deliberately. Distinguish normal
EOF from format failure and serious I/O failure.

**Deep explanation**

C++ streams normally report errors through state flags. `eof()` means end of
file was reached. `fail()` means an operation failed, often due to format
failure or EOF. `bad()` indicates a serious I/O error. You can enable exceptions
with `exceptions()`, but enabling exceptions for EOF often makes normal read
loops awkward.

**C/C++ code/API anchor**

```cpp
#include <fstream>
#include <iostream>
#include <string>

void read_lines(const std::string& path)
{
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot open " + path);
    }

    std::string line;
    while (std::getline(input, line)) {
        // process line
    }

    if (input.bad()) {
        throw std::runtime_error("I/O error while reading " + path);
    }
}
```

**Production/debug angle**

When debugging stream failures, inspect `good()`, `eof()`, `fail()`, and
`bad()` before calling `clear()`. Clearing flags does not fix the underlying
problem.

**Common traps**

- Treating EOF as an error.
- Assuming file open succeeded.
- Calling `clear()` and retrying without fixing the cause.
- Enabling stream exceptions without thinking about `eofbit`.

**Follow-up questions**

- What is the difference between `fail()` and `bad()`?
- When would you enable stream exceptions?
- Why is `std::ifstream` an RAII object?

## Senior Questions

### 10. How would you design an error policy for a mixed C and C++ module?

**Short answer**

Use explicit status codes at the C ABI boundary, use RAII inside C++ for
cleanup, and translate errors at well-defined boundaries without losing context.

**Deep explanation**

C code cannot safely receive C++ exceptions through a plain C ABI. A good design
keeps the boundary explicit: C-facing functions return status codes and output
parameters; C++ internals may use exceptions or Result types according to the
module policy. Translation should preserve the operation, low-level code,
resource identity, and domain category.

**C/C++ code/API anchor**

```cpp
extern "C" int load_config_c(const char* path)
{
    try {
        load_config_cpp(path); // may throw internally
        return 0;
    } catch (const std::exception&) {
        return -1; // real code should map to a meaningful error enum
    }
}
```

**Production/debug angle**

The boundary should be the only place that converts exceptions into C status
codes. Scattered conversions make logs inconsistent and destroy diagnostics.

**Common traps**

- Letting C++ exceptions cross C callbacks or C ABI boundaries.
- Returning `-1` for every failure with no diagnostic path.
- Translating `errno` without preserving operation and resource.
- Using exceptions internally while raw resources are not protected by RAII.

**Follow-up questions**

- How would you expose detailed error information to C callers?
- Where would logging happen?
- How would this change in a no-exception C++ build?

### 11. How do you choose between exception, `std::optional`, and `std::expected`?

**Short answer**

Use `std::optional` when absence alone is enough. Use `std::expected<T, E>` or a
Result type when failure is expected and the caller needs a reason. Use
exceptions for rare or cross-layer failures, especially when construction cannot
produce a valid object.

**Deep explanation**

These mechanisms express different contracts. `optional<T>` says "maybe a T."
`expected<T, E>` says "either T or a specific E." Exceptions say "normal flow is
interrupted; handle this at an appropriate boundary." Good API design makes the
caller responsibility visible.

**C/C++ code/API anchor**

```cpp
#include <optional>
#include <string>

std::optional<int> find_cached_port(const std::string& name);
```

```cpp
// C++23
#include <expected>

enum class ParseError { Empty, BadDigit, Overflow };
std::expected<unsigned, ParseError> parse_u8(std::string_view text);
```

**Production/debug angle**

Use Result-style APIs when the error is part of normal business logic or device
state. Use exceptions when every intermediate layer would otherwise write
boilerplate just to pass the error upward.

**Common traps**

- Returning `std::optional` when the caller needs the failure reason.
- Using exceptions for normal loop control.
- Using `std::expected` everywhere and making rare fatal failures noisy.
- Forgetting `std::expected` is C++23.

**Follow-up questions**

- How would you represent parse failure in C++17?
- Should a missing cache entry be an exception?
- Should constructor failure use `expected`?

### 12. What makes a destructor dangerous in error handling?

**Short answer**

A destructor is dangerous if it can throw outward, because destructors run during
cleanup and stack unwinding. Throwing during unwinding can call
`std::terminate`.

**Deep explanation**

Destructors should release resources and preserve program stability. If cleanup
can fail in a meaningful way, expose an explicit `close()`, `commit()`, or
`flush()` operation that reports the error before destruction. The destructor
can then perform best-effort no-throw cleanup.

**C/C++ code/API anchor**

```cpp
class Transaction {
public:
    void commit(); // may throw or return status

    ~Transaction() noexcept
    {
        if (!committed_) {
            rollback_best_effort(); // must not throw
        }
    }

private:
    void rollback_best_effort() noexcept;
    bool committed_{false};
};
```

**Production/debug angle**

If a program terminates while handling a different exception, inspect
destructors of local objects on the unwinding path. Logging in a destructor must
also avoid throwing.

**Common traps**

- Throwing from `~T()`.
- Calling throwing code from a destructor without catching internally.
- Assuming destructor failure can be handled like normal API failure.
- Hiding important close/flush failures by only doing them in the destructor.

**Follow-up questions**

- Should `close()` throw or return status?
- What should a destructor do if best-effort cleanup fails?
- What happens if a destructor throws during stack unwinding?

### 13. How would you review a function for exception safety?

**Short answer**

Identify every operation that may fail, every resource acquired, every state
mutation, and every invariant. Then decide whether the function provides no,
basic, strong, or no-throw guarantee.

**Deep explanation**

Exception safety review is not about adding `try/catch` everywhere. It is about
state transitions. If the function mutates state before an allocation, copy,
parse, lock, or callback that may throw, the object may be left inconsistent.
Prefer acquire/validate/build first, then commit with no-throw operations.

**C/C++ code/API anchor**

```cpp
#include <utility>
#include <vector>

struct Record {
    int id;
};

class Store {
public:
    void add_record(Record r)
    {
        auto next = records_;          // may throw, object unchanged
        next.push_back(std::move(r));  // may throw, object unchanged
        records_.swap(next);           // commit
    }

private:
    std::vector<Record> records_;
};
```

**Production/debug angle**

In code review, mark throwing points with comments temporarily. Then ask what
happens to ownership, locks, file handles, counters, and observable state at
each point.

**Common traps**

- Equating exception safety with catching exceptions.
- Updating counters before `vector::push_back`.
- Holding raw resources across throwing calls.
- Forgetting callbacks may throw.

**Follow-up questions**

- What is the minimum acceptable guarantee?
- When is the strong guarantee worth the cost?
- How can RAII help even when exceptions are disabled?

### 14. Why might embedded or hard real-time projects avoid C++ exceptions?

**Short answer**

They may avoid exceptions because of code size, toolchain/runtime support,
latency predictability, ABI constraints, and coding-standard rules. They should
still use disciplined status codes and RAII.

**Deep explanation**

Exception handling can complicate worst-case timing analysis and binary-size
budgets. Some projects also compile with exceptions disabled. In that
environment, APIs should return explicit status enums or Result types. Resource
cleanup should still be automatic through RAII where possible; RAII is not only
for exception-enabled code.

**C/C++ code/API anchor**

```cpp
enum class SensorStatus {
    Ok,
    NotReady,
    CrcError,
    Timeout
};

struct SensorReading {
    int milli_celsius;
};

SensorStatus read_sensor(SensorReading& out) noexcept;
```

**Production/debug angle**

No-exception code must be audited for ignored statuses. Compiler warnings,
static analysis, naming conventions, and tests should make ignored failures
visible.

**Common traps**

- Saying "embedded avoids exceptions" as a universal rule.
- Removing exceptions but keeping manual cleanup bugs.
- Returning `bool` when a domain error enum is needed.
- Marking functions `noexcept` while still using throwing library operations.

**Follow-up questions**

- What cleanup strategy replaces exception unwinding?
- How would you make ignored status codes harder?
- What errors should be fatal in a safety-critical system?

## Coding Tasks

### Task 1. Write a C-style parser API.

**Prompt**

Write `parse_u8(const char* text, unsigned* out)` that accepts decimal values
from `0` to `255`. It must return an enum status for null input, bad digit,
overflow, and success.

**Expected answer shape**

Use an enum such as `ParseStatus`, validate `text` and `out`, accumulate into an
unsigned value, check each digit, detect overflow before writing `*out`, and
write the output only on success.

**C/C++ code/API anchor**

```c
typedef enum {
    PARSE_OK,
    PARSE_NULL_INPUT,
    PARSE_BAD_DIGIT,
    PARSE_OVERFLOW
} ParseStatus;

ParseStatus parse_u8(const char* text, unsigned* out);
```

**Production/debug angle**

This tests whether the candidate can design explicit error channels and avoid
partially written outputs.

**Common traps**

- Writing `*out` before validation succeeds.
- Returning only `true`/`false` when diagnostics matter.
- Accepting an empty string as zero without saying so.
- Ignoring overflow.

**Follow-up questions**

- Should whitespace be accepted?
- Should `+12` be accepted?
- How would the API change for embedded logging?

### Task 2. Refactor manual cleanup to RAII.

**Prompt**

This function leaks if `process()` throws. Refactor it.

```cpp
void run()
{
    int* data = new int[1024];
    process(data);
    delete[] data;
}
```

**Expected answer shape**

Use `std::vector<int>` or `std::unique_ptr<int[]>` so cleanup is owned by an
object.

**C/C++ code/API anchor**

```cpp
#include <vector>

void run()
{
    std::vector<int> data(1024);
    process(data.data());
}
```

**Production/debug angle**

The interviewer is checking whether the candidate reaches for ownership design,
not a local `try/catch` bandage.

**Common traps**

- Adding `catch (...) { delete[] data; }` and forgetting to rethrow.
- Using raw `new` in new C++ code.
- Ignoring whether `process` stores the pointer beyond the call.

**Follow-up questions**

- When would `unique_ptr<int[]>` be better than `vector<int>`?
- What if `process` needs the length?
- What if `process` stores the pointer?

### Task 3. Design `load_config()` in three styles.

**Prompt**

Design APIs for loading a config file using exception, `std::optional`, and
Result/`std::expected` style. Explain when each is appropriate.

**Expected answer shape**

```cpp
Config load_config_or_throw(const std::string& path);
std::optional<Config> find_optional_config(const std::string& path);
Result<Config, ConfigError> load_config_result(const std::string& path);
```

Use exception style for required config or constructor-like failure. Use
`optional` when missing config is acceptable and no reason is needed. Use
Result/`expected` when caller needs error details.

**Production/debug angle**

This tests API design, not syntax. The candidate should discuss diagnostics,
expected failure, and recovery ownership.

**Common traps**

- Returning `optional` while logging internally and hiding the reason.
- Throwing for an optional cache miss.
- Using three styles in one API without policy.

**Follow-up questions**

- How would you represent parse error vs missing file?
- What changes in C++23?
- Where should logging happen?

## Debugging Questions

### Debug 1. Why does this program terminate instead of reaching `catch`?

```cpp
#include <stdexcept>

void save() noexcept
{
    throw std::runtime_error("disk full");
}

int main()
{
    try {
        save();
    } catch (...) {
        return 1;
    }
}
```

**Short answer**

`save()` is marked `noexcept`. If an exception escapes it, the program calls
`std::terminate`; the nearby `catch` is not reached.

**Deep explanation**

`noexcept` is a promise in the function type/contract. Violating it is not a
normal throw that callers can recover from. The fix is either remove `noexcept`
or catch internally and satisfy the no-throw guarantee.

**C/C++ code/API anchor**

```cpp
void save();          // may throw
void cleanup() noexcept; // must not throw outward
```

**Production/debug angle**

Break on `std::terminate`, then inspect the stack for `noexcept` functions and
destructors.

**Common traps**

- Assuming `try/catch` catches all exceptions regardless of `noexcept`.
- Marking operations `noexcept` because they are "supposed to work."
- Calling throwing logging code in a `noexcept` function.

**Follow-up questions**

- Should file save normally be `noexcept`?
- What should destructors do with cleanup failures?
- How would you test this path?

### Debug 2. What is wrong with this destructor?

```cpp
class FileWriter {
public:
    ~FileWriter() noexcept(false)
    {
        flush(); // may throw
    }

private:
    void flush();
};
```

**Short answer**

The destructor can throw. If it runs during stack unwinding and `flush()` throws,
the program can terminate.

**Deep explanation**

Destructors are cleanup paths. They should not report ordinary recoverable
failure by throwing outward. Provide an explicit `flush()` or `close()` function
that callers invoke before destruction. The destructor should do best-effort
cleanup and catch internally if needed.

**C/C++ code/API anchor**

```cpp
class FileWriter {
public:
    void close(); // reports failure

    ~FileWriter() noexcept
    {
        try {
            close_best_effort();
        } catch (...) {
            // log only if logging cannot throw, then suppress
        }
    }
};
```

**Production/debug angle**

Crashes during unrelated exception handling often come from destructors on the
unwinding path. Audit destructors for throwing functions.

**Common traps**

- Moving important error reporting exclusively into the destructor.
- Calling throwing loggers from destructors.
- Believing `noexcept(false)` makes destructor throwing safe.

**Follow-up questions**

- Should `close()` return status or throw?
- How would you guarantee logging does not throw?
- What should tests verify?

### Debug 3. Why is this file-reading loop suspicious?

```cpp
std::ifstream input("data.txt");
std::string line;

while (!input.eof()) {
    std::getline(input, line);
    process(line);
}
```

**Short answer**

The loop checks EOF before attempting the read. It may process stale data after
a failed read. The loop should test the extraction operation itself.

**Deep explanation**

EOF is usually discovered by trying to read past available data. The correct
pattern is `while (std::getline(input, line))`. After the loop, inspect `bad()`
to detect serious I/O errors.

**C/C++ code/API anchor**

```cpp
while (std::getline(input, line)) {
    process(line);
}

if (input.bad()) {
    throw std::runtime_error("I/O error while reading data.txt");
}
```

**Production/debug angle**

This bug often appears as duplicate processing of the last line or processing an
empty/stale record after a failed extraction.

**Common traps**

- Treating EOF as an error.
- Ignoring `bad()`.
- Forgetting to check that the file opened.

**Follow-up questions**

- When is `fail()` expected?
- Should EOF throw an exception?
- How would you report the line number on parse failure?

## Final Checklist For Candidates

- Can you classify the failure before choosing the mechanism?
- Can you compare return codes, `errno`, assertions, exceptions, `optional`, and
  `expected` without slogans?
- Can you explain stack unwinding and RAII cleanup?
- Can you name the exception safety guarantee your code provides?
- Can you justify every `noexcept`?
- Can you keep destructors no-throw?
- Can you design an embedded/no-exception alternative?
- Can you debug ignored return codes, stream flags, throwing destructors, and
  unexpected `std::terminate`?
