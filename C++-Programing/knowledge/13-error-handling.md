# 13 - Error Handling

## 1. Goal

After this lesson, you should be able to:

- classify failures as programmer bugs, expected runtime failures, exceptional
  failures, or non-recoverable failures;
- choose between C-style return codes, `errno`, assertions, C++ exceptions,
  `std::optional`, and `std::expected`/Result-style APIs;
- write small C APIs with clear status codes and checked output parameters;
- use `try`, `catch`, `throw`, and standard exception types correctly;
- explain stack unwinding and why RAII is the foundation of exception-safe C++;
- describe the basic, strong, and no-throw exception safety guarantees;
- use `noexcept` for destructors, move operations, swap, and simple no-throw
  functions;
- avoid throwing from destructors and avoid catching exceptions by value;
- handle file I/O errors with stream state flags or stream exceptions;
- explain why some embedded or real-time systems restrict exceptions;
- answer common interview questions about return codes vs exceptions,
  `errno`, `assert`, RAII cleanup, `std::expected`, and `noexcept`.

This is a MUST topic. It connects directly to defensive programming, RAII,
resource ownership, Modern C++, file I/O, embedded constraints, and code review.

## 2. Why It Matters

Most bugs are not in the happy path. They appear when allocation fails, a file is
missing, a parser receives bad data, a device is disconnected, a conversion
fails, a function returns early, or a constructor cannot build a valid object.

Error handling decides what the program does at those moments:

- continue safely;
- report a useful diagnostic;
- recover at the right layer;
- release resources;
- preserve object invariants;
- stop immediately when continuing would be unsafe.

Weak error handling creates code that looks clean until something goes wrong.
Then it leaks memory, ignores failures, corrupts state, hides diagnostics, or
terminates in a place far away from the real bug.

The central review question is:

> If this operation fails here, what remains true, what gets cleaned up, and who
> is responsible for deciding recovery?

Good C and C++ code answer that question explicitly.

## 3. Mental Model

Do not start by asking "Should I throw?" Start by classifying the failure.

| Failure kind | Meaning | Typical mechanism |
| --- | --- | --- |
| Programmer bug | A condition that should never happen if code is correct | `assert`, `static_assert`, tests, fail-fast |
| Expected runtime failure | A normal possibility the caller should handle locally | status code, error enum, `std::optional`, `std::expected` |
| Exceptional runtime failure | Rare or higher-level failure that breaks normal flow | C++ exception |
| Non-recoverable failure | Continuing is unsafe or impossible | terminate, abort, reset, controlled shutdown |

Examples:

- Bad user input is usually an expected runtime failure.
- A missing optional config file may be an expected runtime failure.
- A required config file missing during startup may be exceptional.
- A failed invariant inside an internal data structure is a programmer bug.
- Memory corruption or impossible hardware state may be non-recoverable.

The mechanism is only good if it matches the failure kind and the project
constraints.

## 4. Core Concepts

### 4.1 Error Channel

An error channel is how a function reports failure.

Common channels:

- return value: `0`, `-1`, `bool`, enum, status object;
- output parameter plus status return;
- `errno` set by a C/POSIX-style API;
- exception thrown with `throw`;
- optional value: `std::optional<T>`;
- value-or-error object: `std::expected<T, E>` or project `Result<T, E>`.

The caller should not have to guess which channel is used. A function that both
returns an error code and sometimes throws for the same failure class is hard to
use correctly.

### 4.2 Error Boundary

An error boundary is where low-level failure becomes a higher-level decision.

Examples:

- A parser function returns `ParseStatus`.
- A config loader converts parser errors into a diagnostic message.
- The application startup code decides whether to continue with defaults or
  stop.

Catch exceptions or translate error codes at meaningful boundaries, not at every
line. Catching too early often loses context or hides the failure.

### 4.3 Cleanup Responsibility

Cleanup must not depend on remembering every return path.

In C, cleanup is usually manual and must be structured carefully.

In C++, cleanup should be owned by objects:

- `std::vector` owns dynamic storage;
- `std::unique_ptr` owns a heap object;
- `std::ifstream` owns a file stream;
- `std::lock_guard` owns a lock;
- a custom RAII wrapper owns a handle.

When an exception propagates, C++ calls destructors for fully constructed local
objects. That is why RAII is not optional in exception-safe C++.

## 5. C Usage

### 5.1 Return Code With Output Parameter

C APIs often return a status code and write the result through an output
parameter.

```c
#include <stdio.h>

typedef enum {
    PARSE_OK = 0,
    PARSE_NULL_INPUT,
    PARSE_BAD_DIGIT,
    PARSE_OVERFLOW
} ParseStatus;

ParseStatus parse_u8(const char* text, unsigned* out)
{
    unsigned value = 0;

    if (text == NULL || out == NULL) {
        return PARSE_NULL_INPUT;
    }

    if (*text == '\0') {
        return PARSE_BAD_DIGIT;
    }

    for (const char* p = text; *p != '\0'; ++p) {
        if (*p < '0' || *p > '9') {
            return PARSE_BAD_DIGIT;
        }

        value = value * 10u + (unsigned)(*p - '0');
        if (value > 255u) {
            return PARSE_OVERFLOW;
        }
    }

    *out = value;
    return PARSE_OK;
}

int main(void)
{
    unsigned value = 0;
    ParseStatus status = parse_u8("123", &value);

    if (status != PARSE_OK) {
        printf("parse failed: %d\n", status);
        return 1;
    }

    printf("value = %u\n", value);
    return 0;
}
```

Build:

```sh
cc -std=c11 -Wall -Wextra -pedantic parse_u8.c -o parse_u8
```

This style is predictable and embedded-friendly. The cost is discipline: every
caller must check the returned status.

### 5.2 `errno`, `perror`, And `strerror`

Many C and POSIX-style APIs report failure through a return value and provide
extra detail through `errno`.

```c
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    FILE* file = fopen("missing.txt", "r");
    if (file == NULL) {
        int saved_errno = errno;
        fprintf(stderr, "open failed: %s\n", strerror(saved_errno));
        return 1;
    }

    fclose(file);
    return 0;
}
```

Build:

```sh
cc -std=c11 -Wall -Wextra -pedantic errno_demo.c -o errno_demo
```

Important rules:

- Check the function's return value first.
- Read `errno` only when that API documents that `errno` is meaningful.
- Save `errno` immediately if later calls may overwrite it.
- Include operation context in logs: path, mode, device, command, or input.

`perror("open failed")` is a quick diagnostic helper, but production code often
needs structured logging and saved error codes.

### 5.3 Manual Cleanup In C

C does not have destructors. If a function owns multiple resources, every error
path must release what was already acquired.

```c
#include <stdio.h>
#include <stdlib.h>

int copy_first_byte(const char* src_path, const char* dst_path)
{
    int result = -1;
    FILE* src = NULL;
    FILE* dst = NULL;
    unsigned char* buffer = NULL;

    src = fopen(src_path, "rb");
    if (src == NULL) {
        goto cleanup;
    }

    dst = fopen(dst_path, "wb");
    if (dst == NULL) {
        goto cleanup;
    }

    buffer = malloc(1);
    if (buffer == NULL) {
        goto cleanup;
    }

    if (fread(buffer, 1, 1, src) != 1) {
        goto cleanup;
    }

    if (fwrite(buffer, 1, 1, dst) != 1) {
        goto cleanup;
    }

    result = 0;

cleanup:
    free(buffer);
    if (dst != NULL) {
        fclose(dst);
    }
    if (src != NULL) {
        fclose(src);
    }
    return result;
}
```

This `goto cleanup` pattern is common in C because it centralizes cleanup. In
C++, prefer RAII objects instead.

## 6. C++ Usage

### 6.1 Exceptions: `try`, `catch`, `throw`

C++ exceptions separate normal flow from error flow.

```cpp
#include <iostream>
#include <stdexcept>

double divide(double a, double b)
{
    if (b == 0.0) {
        throw std::invalid_argument("divide by zero");
    }
    return a / b;
}

int main()
{
    try {
        std::cout << divide(10.0, 2.0) << '\n';
        std::cout << divide(10.0, 0.0) << '\n';
    } catch (const std::invalid_argument& e) {
        std::cerr << "invalid input: " << e.what() << '\n';
        return 1;
    }
}
```

Build:

```sh
c++ -std=c++17 -Wall -Wextra -pedantic divide.cpp -o divide
```

Use standard exception types when they fit:

- `std::invalid_argument`: argument value is invalid;
- `std::out_of_range`: index or value outside valid range;
- `std::runtime_error`: runtime condition that prevents operation;
- `std::bad_alloc`: allocation failure from `new`;
- `std::ios_base::failure`: stream failure when stream exceptions are enabled.

Always catch polymorphic exceptions by `const&`:

```cpp
catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
}
```

Catching by value can slice a derived exception object and lose information.

### 6.2 Catch Ordering

Handlers are checked in order. Put specific exceptions before general ones.

```cpp
try {
    throw std::out_of_range("index");
} catch (const std::out_of_range& e) {
    std::cerr << "range error: " << e.what() << '\n';
} catch (const std::logic_error& e) {
    std::cerr << "logic error: " << e.what() << '\n';
} catch (const std::exception& e) {
    std::cerr << "standard exception: " << e.what() << '\n';
}
```

If `std::exception` comes first, derived handlers after it are unreachable for
standard exceptions.

### 6.3 Rethrowing With Context

Sometimes a lower layer cannot decide recovery, but a higher layer needs more
context.

```cpp
#include <fstream>
#include <stdexcept>
#include <string>

std::string read_first_line(const std::string& path)
{
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("cannot open file: " + path);
    }

    std::string line;
    if (!std::getline(file, line)) {
        throw std::runtime_error("cannot read first line: " + path);
    }

    return line;
}
```

Do not catch just to print and continue if the caller still needs to know that
the operation failed. Either handle the problem or let it propagate.

### 6.4 Custom Exception Type

Use a custom exception when the caller can recover differently based on the
category.

```cpp
#include <stdexcept>
#include <string>

class ConfigError : public std::runtime_error {
public:
    explicit ConfigError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

int load_port(const std::string& text)
{
    int port = std::stoi(text);
    if (port <= 0 || port > 65535) {
        throw ConfigError("port is outside valid TCP range");
    }
    return port;
}
```

Keep exception hierarchies simple. A huge hierarchy is usually harder to use
than a small set of meaningful domain categories.

## 7. Stack Unwinding And RAII

When an exception is thrown, C++ searches for a matching handler. As it leaves
each scope, it destroys fully constructed local objects in reverse construction
order. This is stack unwinding.

```cpp
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

class Trace {
public:
    explicit Trace(std::string name) : name_(std::move(name))
    {
        std::cout << "acquire " << name_ << '\n';
    }

    ~Trace()
    {
        std::cout << "release " << name_ << '\n';
    }

private:
    std::string name_;
};

void worker()
{
    Trace a{"A"};
    Trace b{"B"};
    throw std::runtime_error("worker failed");
}

int main()
{
    try {
        Trace main_resource{"main"};
        worker();
    } catch (const std::exception& e) {
        std::cout << "caught: " << e.what() << '\n';
    }
}
```

Build:

```sh
c++ -std=c++17 -Wall -Wextra -pedantic unwind.cpp -o unwind
```

Output shape:

```text
acquire main
acquire A
acquire B
release B
release A
release main
caught: worker failed
```

This is why RAII works. If a resource is owned by an object, the destructor
releases it even during exception propagation.

### 7.1 Partial Construction

If a constructor throws, the object's destructor is not called because the
object was never fully constructed. But destructors are called for fully
constructed members and base classes.

That means each member should manage its own resource. Do not wait for the outer
class destructor to clean up resources acquired by earlier members.

### 7.2 Destructor During Exception

Destructors must not throw outward.

If a destructor throws while another exception is already being processed, the
program calls `std::terminate`. Even outside active unwinding, throwing from a
destructor makes cleanup unpredictable and dangerous.

Prefer this:

```cpp
class Session {
public:
    void close()
    {
        // Can report failure explicitly.
        closed_ = true;
    }

    ~Session() noexcept
    {
        if (!closed_) {
            // Best-effort cleanup only. Do not throw.
        }
    }

private:
    bool closed_{false};
};
```

Use explicit operations such as `close()`, `commit()`, or `flush()` when failure
must be reported. Let the destructor perform best-effort no-throw cleanup.

## 8. Exception Safety Guarantees

Exception safety answers this question:

> If an exception interrupts this operation, what state is left behind?

| Guarantee | Meaning | Example expectation |
| --- | --- | --- |
| No guarantee | State may be invalid or resources may leak | Not acceptable for production code |
| Basic guarantee | No leaks; objects remain valid | Data may be partially changed |
| Strong guarantee | Operation succeeds completely or has no effect | Transaction-like update |
| No-throw guarantee | Operation does not throw | Destructors, swap, deallocation, many moves |

### 8.1 Basic Guarantee

The basic guarantee means the object remains usable and no resources leak.

```cpp
#include <vector>

class LogBuffer {
public:
    void append(int code)
    {
        records_.push_back(code); // May throw, but vector stays valid.
    }

private:
    std::vector<int> records_;
};
```

If `push_back` throws, `records_` is still a valid `std::vector`. The operation
may not complete, but the object is not corrupted.

### 8.2 Strong Guarantee

The strong guarantee means all-or-nothing behavior.

```cpp
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

class ConfigLines {
public:
    void replace_all(std::vector<std::string> next)
    {
        validate(next);      // May throw before changing this object.
        lines_.swap(next);   // No-throw for vector swap in normal allocator use.
    }

private:
    static void validate(const std::vector<std::string>& lines)
    {
        for (const auto& line : lines) {
            if (line.empty()) {
                throw std::runtime_error("empty config line");
            }
        }
    }

    std::vector<std::string> lines_;
};
```

The mutation happens only after validation succeeds. This pattern is common:

1. Build or validate a temporary state.
2. Perform operations that might throw before touching the original object.
3. Commit with a no-throw operation such as `swap`.

### 8.3 No-Throw Guarantee

Some functions should never throw:

- destructors;
- deallocation functions;
- `swap`;
- move constructors and move assignment when they only transfer ownership;
- simple observers such as `size()` or `empty()`.

No-throw functions are important for cleanup, rollback, and standard-library
optimizations.

## 9. `noexcept`

`noexcept` says a function promises not to let an exception escape.

```cpp
class Handle {
public:
    Handle() = default;

    Handle(Handle&& other) noexcept
        : id_(other.id_)
    {
        other.id_ = -1;
    }

    Handle& operator=(Handle&& other) noexcept
    {
        if (this != &other) {
            id_ = other.id_;
            other.id_ = -1;
        }
        return *this;
    }

    void swap(Handle& other) noexcept
    {
        int tmp = id_;
        id_ = other.id_;
        other.id_ = tmp;
    }

    int id() const noexcept
    {
        return id_;
    }

private:
    int id_{-1};
};
```

If a `noexcept` function throws, the exception is not caught by a nearby
`catch`. The program calls `std::terminate`.

Use `noexcept` when the promise is true. Do not add it just because the function
"should not fail" if it calls code that may throw.

### 9.1 Why Move Operations Often Need `noexcept`

Containers such as `std::vector` may choose between moving and copying elements
during reallocation. If moving might throw, the container may copy instead to
preserve exception safety.

```cpp
#include <iostream>
#include <vector>

struct Widget {
    Widget() = default;

    Widget(const Widget&)
    {
        std::cout << "copy\n";
    }

    Widget(Widget&&) noexcept
    {
        std::cout << "move\n";
    }
};

int main()
{
    std::vector<Widget> items;
    items.reserve(1);
    items.emplace_back();
    items.emplace_back(); // Reallocation: existing element can be moved.
}
```

Build:

```sh
c++ -std=c++17 -Wall -Wextra -pedantic noexcept_move.cpp -o noexcept_move
```

The exact output depends on implementation details, but the design lesson is
stable: no-throw moves allow containers to optimize while preserving safety.

### 9.2 Conditional `noexcept`

Templates can make `noexcept` depend on the operations they use.

```cpp
#include <type_traits>
#include <utility>

template <typename T>
void exchange_values(T& a, T& b)
    noexcept(std::is_nothrow_move_constructible_v<T> &&
             std::is_nothrow_move_assignable_v<T>)
{
    T temp(std::move(a));
    a = std::move(b);
    b = std::move(temp);
}
```

This is advanced enough that you should use it only when the correctness or
performance benefit is clear.

## 10. File I/O Error Handling

File streams are RAII objects: they close automatically when destroyed.

```cpp
#include <fstream>
#include <iostream>
#include <string>

int main()
{
    std::ifstream input("data.txt");
    if (!input) {
        std::cerr << "cannot open data.txt\n";
        return 1;
    }

    std::string line;
    while (std::getline(input, line)) {
        std::cout << line << '\n';
    }

    if (input.bad()) {
        std::cerr << "serious I/O error while reading\n";
        return 1;
    }

    if (!input.eof()) {
        std::cerr << "read stopped before EOF\n";
        return 1;
    }
}
```

Stream state flags:

| Flag | Meaning |
| --- | --- |
| `good()` | No error flags are set |
| `eof()` | End of file was reached |
| `fail()` | Operation failed, often format/extraction failure or EOF |
| `bad()` | Serious I/O error, such as low-level stream corruption |

Do not treat every `fail()` as a serious error. A read loop often ends because
it reached EOF. Check `eof()` and `bad()` to understand why extraction stopped.

### 10.1 Stream Exceptions

Streams normally report errors through state flags. You can enable exceptions:

```cpp
#include <fstream>
#include <iostream>

int main()
{
    std::ifstream input;
    input.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
        input.open("data.txt");
        int value = 0;
        input >> value;
        std::cout << value << '\n';
    } catch (const std::ios_base::failure& e) {
        std::cerr << "stream failure: " << e.what() << '\n';
        return 1;
    }
}
```

Be careful with exception masks. Enabling exceptions for EOF can make normal
end-of-file handling awkward unless EOF is truly exceptional in that context.

## 11. Modern Explicit Results

Exceptions are not the only modern C++ answer. Some APIs are clearer when the
error is part of the return type.

### 11.1 `std::optional`

Use `std::optional<T>` when "no value" is enough information.

```cpp
#include <optional>
#include <string>

std::optional<int> find_port(const std::string& key)
{
    if (key == "http") {
        return 80;
    }
    if (key == "https") {
        return 443;
    }
    return std::nullopt;
}
```

`std::optional` is not ideal when the caller needs to know why the value is
missing.

### 11.2 `std::expected` And Result Types

`std::expected<T, E>` is a C++23 vocabulary type for "either a `T` or an `E`".
For earlier standards, many projects use a local `Result<T, E>` type.

```cpp
#include <expected>
#include <string>

enum class ParseError {
    Empty,
    BadDigit,
    Overflow
};

std::expected<unsigned, ParseError> parse_u8_cpp23(const std::string& text)
{
    if (text.empty()) {
        return std::unexpected(ParseError::Empty);
    }

    unsigned value = 0;
    for (char ch : text) {
        if (ch < '0' || ch > '9') {
            return std::unexpected(ParseError::BadDigit);
        }

        value = value * 10u + static_cast<unsigned>(ch - '0');
        if (value > 255u) {
            return std::unexpected(ParseError::Overflow);
        }
    }

    return value;
}
```

Build with a compiler and standard library that support C++23 `std::expected`:

```sh
c++ -std=c++23 -Wall -Wextra -pedantic expected_parse.cpp -o expected_parse
```

Use `std::expected` or Result-style APIs when failures are expected, local, and
important for the caller to inspect. Use exceptions when failures are rare,
should cross several call layers, or occur during construction of an object that
cannot be valid.

## 12. Practical Usage

### 12.1 Constructor Failure

Constructors cannot return a status code. If an object cannot be valid, throwing
is natural in exception-enabled C++.

```cpp
#include <fstream>
#include <stdexcept>
#include <string>

class ConfigFile {
public:
    explicit ConfigFile(const std::string& path) : input_(path)
    {
        if (!input_) {
            throw std::runtime_error("cannot open config: " + path);
        }
    }

private:
    std::ifstream input_;
};
```

If your project disables exceptions, use a factory that returns a status/result
instead of a throwing constructor.

### 12.2 Boundary Translation

A common enterprise pattern is to translate low-level errors once at a boundary.

```cpp
#include <exception>
#include <iostream>
#include <stdexcept>

int run_command()
{
    // Lower layers may throw.
    throw std::runtime_error("sensor config missing");
}

int main()
{
    try {
        return run_command();
    } catch (const std::exception& e) {
        std::cerr << "command failed: " << e.what() << '\n';
        return 1;
    }
}
```

This keeps low-level code clean and centralizes logging, metrics, and exit-code
decisions.

### 12.3 Embedded And Real-Time Projects

Some embedded, automotive, and hard real-time projects restrict or disable
exceptions because of:

- code-size concerns;
- ABI/toolchain constraints;
- latency predictability;
- project coding standards;
- limited runtime support;
- difficulty auditing all exception paths.

That does not mean manual cleanup is good. Even without exceptions, C++ RAII is
still useful for deterministic cleanup on every return path.

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

SensorStatus read_sensor(SensorReading& out) noexcept
{
    // Hardware interaction would happen here.
    out.milli_celsius = 25000;
    return SensorStatus::Ok;
}
```

In such projects, prefer predictable status codes plus RAII wrappers for handles,
locks, buffers, and transactions.

## 13. Comparisons

| Topic | C | C++ | Enterprise / Embedded Guidance |
| --- | --- | --- | --- |
| Return code vs exception | Return `int`, enum, or status object; caller checks every result | Throw typed exception and handle at a boundary | Use status for expected/local failures, C ABI, and no-exception builds. Use exceptions for rare failures that should propagate |
| `errno` vs exception | API reports failure, then `errno` provides extra diagnostic | Exception object carries type and message/context | Preserve low-level code plus operation context when translating |
| `assert` vs runtime error | `assert` checks programmer assumptions and may disappear with `NDEBUG` | Runtime errors are reported through exceptions or explicit result types | Never use `assert` for user input, file availability, device state, or recoverable runtime failures |
| Exception vs `std::expected` | C has project-specific Result structs | Exception is implicit control transfer; `expected` is explicit value-or-error | Use `expected` for expected/frequent/local failures. Use exceptions for rare or cross-layer failures |
| Manual cleanup vs RAII cleanup | Cleanup must be repeated or centralized manually | Destructors clean up on return and stack unwinding | In C++, owning resources should live in RAII types. Manual cleanup is a review warning |

When to use C style:

- C API or ABI boundary;
- expected failure close to caller;
- embedded/no-exception policy;
- hard real-time or deterministic latency requirement;
- very small low-level operations where explicit status is clearer.

When to use C++ exception style:

- constructor cannot build a valid object;
- failure is rare and exceptional;
- error must propagate through several layers;
- RAII protects all acquired resources;
- the recovery decision belongs at a higher boundary.

Common bug: using both styles for the same failure without a policy. For
example, returning `false` for some parse errors but throwing for others makes
callers incomplete by default.

## 14. Common Bugs

### 14.1 Ignoring Return Codes

```c
remove("old.log"); /* Bug: failure ignored. */
```

If removal matters, check the result and report context.

### 14.2 Reading `errno` At The Wrong Time

`errno` is not a general "last error" variable. Read it only after an API reports
failure and documents that `errno` is meaningful.

### 14.3 Using `assert` For Runtime Validation

```cpp
#include <cassert>

void set_speed(int speed)
{
    assert(speed >= 0); // Bad if speed comes from user/device input.
}
```

With `NDEBUG`, `assert` may compile away. Use runtime validation for external
inputs.

### 14.4 Catching By Value

```cpp
try {
    throw std::runtime_error("failed");
} catch (std::exception e) { // Bad: slicing risk.
}
```

Use:

```cpp
catch (const std::exception& e) {
}
```

### 14.5 Throwing From A Destructor

```cpp
class Bad {
public:
    ~Bad() noexcept(false)
    {
        throw std::runtime_error("cleanup failed");
    }
};
```

This can terminate the program during stack unwinding. Destructors should not
throw outward.

### 14.6 Marking Unsafe Code `noexcept`

```cpp
void save() noexcept
{
    std::string big;
    big.resize(1'000'000); // May throw std::bad_alloc.
}
```

If an exception escapes, the program terminates. Make the function non-`noexcept`
or catch internally and truly satisfy the promise.

### 14.7 Treating EOF As A Serious Error

Many read loops stop because EOF was reached. Check `bad()` for serious I/O
failure and `eof()` for normal end-of-file.

### 14.8 Losing Context

```cpp
catch (...) {
    throw std::runtime_error("failed");
}
```

This hides the original type and message. Add context carefully, but preserve
the useful diagnostic whenever possible.

## 15. Debugging

For C-style error handling:

- log the operation that failed;
- log the input or resource identifier;
- save `errno` immediately after failure;
- check whether callers ignored a status code;
- compile with warnings that catch unused results when available.

For exception-based C++:

- identify the first throw site, not only the catch site;
- inspect the dynamic exception type and `what()`;
- break on `std::terminate` when the program exits unexpectedly;
- search for throwing destructors and incorrect `noexcept`;
- check whether a catch block swallows the error;
- verify RAII ownership for every acquired resource.

For exception safety:

- ask what happens if allocation throws here;
- ask what happens if copying or parsing throws here;
- check whether partial mutation leaves invariants broken;
- prefer temporary-then-commit designs for strong guarantee;
- use sanitizers to catch memory errors that look like error-handling bugs.

For file I/O:

- print `good()`, `eof()`, `fail()`, and `bad()` before calling `clear()`;
- check open mode and path;
- distinguish format failure from missing file and low-level I/O failure;
- only retry after fixing the underlying reason.

Useful build flags for examples:

```sh
c++ -std=c++17 -Wall -Wextra -Wpedantic -fsanitize=address,undefined file.cpp
```

## 16. Best Practices

- Choose one error policy per API and document it.
- Use `assert` for programmer bugs, not recoverable runtime failures.
- Use status codes or Result types for expected local failures.
- Use exceptions for rare failures that should propagate to a boundary.
- Use RAII before writing exception-based code.
- Keep destructors no-throw.
- Mark move operations and `swap` as `noexcept` when they really cannot throw.
- Catch exceptions by `const&`.
- Order catch blocks from most specific to most general.
- Do not catch just to hide errors.
- Add context at module boundaries.
- Keep custom exception hierarchies small and useful.
- Prefer `std::optional` only when absence is enough information.
- Prefer `std::expected<T, E>` or a project `Result<T, E>` when the caller needs
  a reason.
- In embedded/no-exception code, combine predictable status returns with RAII
  wrappers.
- Test failure paths, not only success paths.

## 17. Interview Readiness

### 17.1 Return Code vs Exception

Return codes are explicit and predictable. They are good for C APIs, embedded
systems, expected failures, and cases where the caller is close to the failure.
The downside is that they are easy to ignore and can clutter normal logic.

Exceptions separate normal flow from error flow and propagate automatically.
They are good when failure is rare, recovery belongs at a higher layer, or a
constructor cannot return a status. They require RAII and an exception policy.

### 17.2 Why Embedded May Avoid Exceptions

Embedded and real-time projects may avoid exceptions for predictable latency,
code size, toolchain support, ABI constraints, or coding-standard rules. In
those projects, use status codes or Result types and still use RAII for cleanup.

### 17.3 What Is Exception Safety?

Exception safety describes what remains true if an exception interrupts an
operation.

- Basic guarantee: valid state, no leaks.
- Strong guarantee: operation succeeds or has no effect.
- No-throw guarantee: operation does not throw.

### 17.4 What Is `noexcept`?

`noexcept` is a promise that a function will not let exceptions escape. It helps
document intent and enables optimizations, especially for move operations. If a
`noexcept` function throws, the program calls `std::terminate`.

### 17.5 What Happens If A Destructor Throws?

If a destructor throws during stack unwinding while another exception is active,
the program terminates. Destructors should catch internally, log if appropriate,
and not throw outward.

### 17.6 Exception vs `std::expected`

An exception is implicit control transfer. `std::expected<T, E>` is explicit in
the function's return type. Use `expected` for expected failures that the caller
should inspect. Use exceptions for rare failures or cross-layer propagation.

## 18. Practice

1. Write a C function `parse_temperature()` that returns an enum status and
   writes the result through an output parameter.
2. Modify the function so it logs `errno` correctly when reading from a file
   fails.
3. Write a C++ `ConfigFile` class that opens a file in the constructor and
   throws `std::runtime_error` when open fails.
4. Convert a manual `new`/`delete` example into `std::vector` or
   `std::unique_ptr`.
5. Write a small class with a `swap()` function and a strong-guarantee
   assignment operation.
6. Create a type with a `noexcept` move constructor and observe how a
   `std::vector` behaves during reallocation.
7. Write a file-reading loop that distinguishes EOF from `bad()` I/O failure.
8. Design `load_config()` in three styles:
   - throwing exception;
   - returning `std::optional<Config>`;
   - returning `std::expected<Config, ConfigError>` or a project Result type.
9. Review an existing function and mark each failure as programmer bug,
   expected failure, exceptional failure, or non-recoverable failure.

## 19. Summary

Error handling is not only about reporting failure. It is about preserving
correctness when the normal path breaks.

Use assertions for bugs, status codes or Result types for expected failures,
exceptions for exceptional cross-layer failures, and termination only when the
program cannot continue safely. In C++, make cleanup automatic with RAII, keep
destructors no-throw, catch exceptions by `const&`, and design each mutating
operation with a clear exception safety guarantee.

The best error-handling code is boring in the best way: the caller can see the
policy, resources are cleaned up automatically, diagnostics keep context, and
failure paths are tested like real code.

## 20. Reference Notes

- `std::expected` is a C++23 library type. For C++17/C++20 projects, use a
  project-local `Result<T, E>` or a third-party equivalent if approved.
- `errno` behavior is API-specific. Do not treat it as a universal last-error
  value.
- Exact exception, `noexcept`, stream, and termination behavior should be checked
  against cppreference or the C++ standard when writing library-level code.
