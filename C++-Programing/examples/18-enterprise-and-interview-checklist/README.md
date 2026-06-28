# 18 - Enterprise And Interview Checklist Examples

This folder is intentionally small. Topic 18 is a checklist/capstone topic, so
the useful example is not a new application; it is a compact review workflow
with one safe compile-ready demo and a few intentionally flawed snippets to
practice reviewing.

## Files

| File | Purpose | Status |
| --- | --- | --- |
| `review_checklist_demo.cpp` | Safe examples of RAII, view lifetime, vector erase, callback capture, and condition-variable predicate | Learning-only, production-style shapes |
| `Makefile` | Build, run, sanitizer, strict-warning, debug, and cleanup commands | Practical |

## Build

From this directory:

```sh
make
```

Build only the demo:

```sh
make review_checklist_demo
```

## Run

```sh
make run
```

Expected output includes:

```text
stable label: ready
values: 1 3 5
callback: done
popped: 42
```

## Sanitizer / Debug Commands

AddressSanitizer + UndefinedBehaviorSanitizer:

```sh
make sanitize
```

Strict warnings as errors:

```sh
make strict
```

Debug build:

```sh
make debug
gdb ./review_checklist_demo_dbg
```

ThreadSanitizer is useful when adapting the example into a real threaded
producer-consumer test:

```sh
g++ -std=c++17 -g -O1 -fsanitize=thread -fno-omit-frame-pointer -pthread review_checklist_demo.cpp -o review_checklist_demo_tsan
./review_checklist_demo_tsan
```

Clean generated files:

```sh
make clean
```

## Review Checklist

Use this order when reading a C/C++ patch:

1. Correctness and undefined behavior.
2. Ownership, lifetime, RAII, and cleanup on failure.
3. Exception safety and error context.
4. Iterator/reference/view invalidation.
5. Callback capture lifetime.
6. Thread-safety, lock order, and condition-variable predicates.
7. API design: owner, borrower, observer, view, optional, error channel.
8. Complexity and container choice.
9. Tests, sanitizer evidence, and debug commands.

## Flawed Snippets For Practice

These snippets are intentionally broken. They are learning-only review prompts;
do not copy them into production.

### Dangling `std::string_view`

```cpp
#include <string>
#include <string_view>

std::string_view bad_label() {
    std::string text = "ready";
    return text; // UB if the caller reads the view
}
```

Review finding: `text` is destroyed when the function returns. Return
`std::string`, return a view to storage that outlives the view, or make the
caller own the string.

### Iterator Invalidation

```cpp
#include <vector>

void remove_negative(std::vector<int>& values) {
    for (auto it = values.begin(); it != values.end(); ++it) {
        if (*it < 0) {
            values.erase(it); // `it` is invalid after erase
        }
    }
}
```

Review finding: use the iterator returned by `erase`, and increment only when
not erasing.

### Dangling Lambda Capture

```cpp
#include <functional>
#include <string>

std::function<void()> bad_callback() {
    std::string message = "done";
    return [&] { (void)message; }; // captures a dead stack object
}
```

Review finding: capture by value when the callback must own the data, or use a
clear object-lifetime/subscription policy.

### Missing Predicate

```cpp
#include <condition_variable>
#include <mutex>

std::condition_variable cv;
std::mutex mutex;

void wait_bad() {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock); // spurious wakeups and missed condition policy
}
```

Review finding: store the condition in shared state protected by the mutex and
wait with a predicate.

## Safety Notes

- `review_checklist_demo.cpp` is learning-only. It uses production-style shapes,
  but it is intentionally compact.
- The flawed snippets demonstrate undefined behavior, iterator invalidation,
  dangling captures, and condition-variable misuse. They are review prompts, not
  runnable examples.
- No example here is a complete production system. Real code still needs tests,
  logging/error policy, input validation, ownership documentation, and
  concurrency stress testing.
- If you add threads or callbacks, document whether callbacks may unsubscribe,
  subscribe, or call back into the subject during notification.
