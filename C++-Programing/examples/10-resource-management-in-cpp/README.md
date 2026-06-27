# 10 - Resource Management In C++: Examples

## Status

These are **learning-focused C++17 examples**, not production-ready resource
libraries, allocators, real-time components, or concurrent ownership systems.

The suite demonstrates:

- a manual Rule-of-Five owner beside a Rule-of-Zero value type;
- a movable, non-copyable RAII wrapper for a fake C-style handle;
- `unique_ptr` ownership transfer and reference-based borrowing;
- a `shared_ptr` ownership edge with a `weak_ptr` back-reference;
- strict warnings and host-side AddressSanitizer/UndefinedBehaviorSanitizer.

The examples use single-threaded host execution. They do not prove timing
bounds, allocation determinism, race freedom, ABI stability, or suitability for
safety-critical software.

## Build And Run

Requirements:

- GNU Make;
- a C++17 compiler;
- AddressSanitizer and UndefinedBehaviorSanitizer support.

Build, run, and sanitize everything:

```bash
make check
```

Build without running:

```bash
make all
```

Run sanitizer builds:

```bash
make sanitize
```

Remove generated files:

```bash
make clean
```

Generated binaries are placed under `build/`.

## Example Map

| Target | Source | Main lesson |
| --- | --- | --- |
| `rule_of_zero` | `rule_of_zero.cpp` | Manual copy/move ownership versus `std::vector` |
| `handle_raii` | `handle_raii.cpp` | Unique handle ownership and exactly-once release |
| `ownership` | `ownership.cpp` | Borrow versus `unique_ptr` transfer |
| `shared_weak` | `shared_weak.cpp` | Shared owner plus non-owning back-reference |

## 1. Rule Of Five Versus Rule Of Zero

```bash
make build/rule_of_zero
./build/rule_of_zero
```

Expected output:

```text
manual-copy-independent=true assignments=true moved-from-empty=true failure-safe=true rule-zero-copy-independent=true result=passed
```

`ManualBuffer` is intentionally educational. It owns a raw array and therefore
must define destruction, deep copying, assignment, and move transfer correctly.
`Buffer` delegates ownership to `std::vector` and needs no custom special
members.

The executable covers copy and move construction, copy and move assignment,
self-assignment, self-move, copying an empty moved-from object, and an injected
copy-allocation failure. The failed assignment leaves the target unchanged.

**Production direction:** prefer the Rule-of-Zero version unless a low-level
resource requirement prevents using a standard owner.

**Exception-safety warning:** copy-and-swap constructs replacement state before
commit. A manual assignment that deletes current storage before allocating the
replacement can leave the object broken if allocation throws.

## 2. Movable C-Style Handle

```bash
make build/handle_raii
./build/handle_raii
```

Expected output:

```text
release-count=1 result=passed
```

`Session` deletes copying, transfers the handle on move, empties the source, and
releases exactly once in its destructor. The fake C API makes release counting
observable.

**Production direction:** use the real API's invalid-handle value and exact
release function. If release can report an error that callers must handle,
provide an explicit `close()` plus a non-throwing destructor fallback.

**Ownership warning:** copying an integer or pointer handle does not duplicate
the underlying resource safely.

## 3. Transfer And Borrow

```bash
make build/ownership
./build/ownership
```

Expected output:

```text
ownership-transferred=true null-rejected=true value=2700 result=passed
```

`inspect(const Sensor&)` borrows. `Device(std::unique_ptr<Sensor>)` accepts
ownership transfer. After `std::move(owner)`, the original `unique_ptr` is
empty and `Device` owns the sensor.

`Device` requires a sensor and rejects an empty `unique_ptr` during
construction. Its `read()` operation can therefore rely on a non-null invariant
rather than dereferencing an optional owner implicitly.

**Lifetime warning:** a pointer returned by `owner.get()` would be a borrow. It
would dangle after transfer if the new owner later released the object.

## 4. Shared Ownership And Observation

```bash
make build/shared_weak
./build/shared_weak
```

Expected output:

```text
expired=true parent-destructions=1 child-destructions=1 result=passed
```

`Parent` strongly owns `Child`; `Child` observes `Parent` through `weak_ptr`.
After the scope, both objects are destroyed and `lock()` reports expiration.

**Cycle warning:** replacing the `weak_ptr` with `shared_ptr` would create a
strong cycle and leak the unreachable graph.

**Concurrency warning:** control-block bookkeeping does not make `Parent` or
`Child` thread-safe. Concurrent state access still requires a documented
synchronization policy.

## Debugging

Build unoptimized binaries:

```bash
make debug
gdb ./build/handle_raii
```

Useful commands:

```text
break Session::Session
break Session::~Session
break Session::operator=
run
bt
continue
```

For `rule_of_zero`, break on `ManualBuffer` copy, move, assignment, and
destruction operations. Compare owner-object addresses with managed-array
addresses.

## Sanitizer Experiments

The normal suite intentionally executes no undefined behavior. For a separate
local experiment, introduce exactly one defect at a time:

- remove source emptying from `ManualBuffer` move construction;
- copy a raw owning pointer into a second owner;
- keep and dereference a pointer returned by `unique_ptr::get()` after reset;
- change the weak back-reference to `shared_ptr` and inspect leak behavior.

Run the modified program under sanitizers, then revert the defect. ASan/UBSan
cover executed paths only; a clean run is not proof of correct ownership.

ThreadSanitizer is omitted because these examples create no threads. Iterator
invalidation is also outside these examples; Chapter 11 covers container
invalidation in depth.
