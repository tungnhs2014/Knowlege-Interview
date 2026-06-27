# 09 - OOP In C++: Examples

## Status

These are **learning-focused C++17 and C17 examples**, not production-ready
libraries, firmware, plugin ABIs, ownership frameworks, or real-time designs.

The suite demonstrates:

- a small abstract C++ interface with a public virtual destructor;
- runtime dispatch through a borrowed base reference;
- dependency injection without dynamic allocation;
- composition through a bounded retry wrapper;
- defined object slicing contrasted with reference-based polymorphism;
- equivalent C runtime dispatch using a context pointer and operation table;
- strict compiler warnings and host-side ASan/UBSan execution.

The examples are single-threaded and use stack or static storage. They do not
model concurrent access, exceptions, dynamic ownership, stable binary
interfaces, hardware timing, cancellation, or unbounded retries.

## Requirements

- GNU Make
- A C++17 compiler
- A C17 compiler
- AddressSanitizer and UndefinedBehaviorSanitizer
- Optional: GDB and `nm`

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
| `polymorphism` | `polymorphism.cpp` | Abstract interface, `override`, borrowed injection, and composition |
| `slicing` | `slicing.cpp` | By-value slicing versus reference-based virtual dispatch |
| `c-dispatch` | `c_dispatch.c` | C17 context pointer and versioned function table |
| `sanitize` | All runnable sources | ASan/UBSan and C++ vptr checks on exercised paths |
| `symbols` | C++ polymorphism executable | Implementation-specific symbol inspection |

## 1. C++ Runtime Polymorphism And Composition

```bash
make polymorphism
./build/polymorphism
```

Expected output:

```text
direct-active=true retry-active=true failure-propagated=true retry-calls=2 result=passed
```

`Sensor` is a small abstract interface. `FixedSensor` and
`FailingOnceSensor` provide substitutable implementations. `RetryingSensor`
also implements `Sensor`, but reuses behavior through composition by borrowing
another sensor.

`Alarm`, `RetryingSensor`, and their collaborators are stack allocated. Virtual
dispatch does not require dynamic allocation.

`SensorReading` keeps status separate from the measurement value, so negative
measurements cannot collide with an error sentinel. `AlarmState` makes a
persistent read failure distinct from an inactive alarm. The example tests that
failure path explicitly; a production system must choose and document whether
sensor failure triggers a fail-safe action, degraded operation, or escalation.

**Ownership warning:** the stored `const Sensor&` members are borrowed. Each
referenced sensor must outlive every object that stores its reference. A
reference does not transfer ownership or extend lifetime.

**Timing warning:** the retry policy performs at most two reads. Production
retry logic must define attempt count, delay, deadline, error propagation, and
whether a read may block.

**Thread-safety warning:** `FailingOnceSensor` changes a `mutable` call counter
inside a `const` function. It is intentionally single-threaded. `mutable` does
not provide synchronization; concurrent calls would require a documented
thread-safety design.

## 2. Object Slicing

```bash
make slicing
./build/slicing
```

Expected output:

```text
by-value=Message by-reference=AlarmMessage result=passed
```

The by-value function deliberately creates a separate `Message` object from an
`AlarmMessage`. This is defined behavior, but the derived part is sliced away.
The by-reference function refers to the original object, so virtual dispatch
selects `AlarmMessage::name`.

This demonstration executes no undefined behavior. In production polymorphic
APIs, pass and store base references or pointers rather than base objects by
value.

## 3. C Function-Table Dispatch

```bash
make c-dispatch
./build/c-dispatch
```

Expected output:

```text
value=2700 null-rejected=true result=passed
```

The C example pairs:

- an explicit implementation context;
- a versioned operation table;
- a wrapper that validates pointers and table version;
- a status code separated from the output value.

The context and table are borrowed. The caller must keep both alive for every
dispatch. C provides no automatic virtual destructor; a stateful implementation
that owns resources needs an explicit, documented cleanup operation.

**Unsafe C API warning:** `void *` does not encode the context type, ownership,
size, or lifetime. A mismatched context and operation table can cause undefined
behavior. A production API needs stronger construction functions, private
representation where possible, version compatibility rules, and complete error
handling.

## Sanitizers

```bash
make sanitize
```

The C++ sanitizer builds enable AddressSanitizer, UndefinedBehaviorSanitizer,
and the compiler's vptr checks. The C build enables ASan and UBSan.

They may expose selected executed defects involving:

- use after lifetime;
- invalid memory access;
- invalid downcasts or virtual calls supported by vptr instrumentation;
- misalignment;
- selected arithmetic undefined behavior.

A clean run does not prove:

- that every borrowed reference or context has a valid lifetime;
- substitutability of every implementation;
- absence of object slicing in untested APIs;
- ABI compatibility across compilers or library versions;
- exception safety;
- race freedom or deadlock freedom;
- real-time timing or target behavior.

ThreadSanitizer is not included because the examples create no threads.

## Debugging

Build unoptimized binaries with debug information:

```bash
make debug
gdb ./build/polymorphism
```

Useful GDB commands:

```text
break FixedSensor::read
break FailingOnceSensor::read
break RetryingSensor::read
run
bt
continue
```

Inspect the slicing example:

```bash
gdb ./build/slicing
```

```text
break name_by_value
break name_by_reference
run
print message
continue
```

Inspect C++ symbols:

```bash
make symbols
```

Virtual-table symbols, class layouts, and mangled names are ABI-specific. They
are debugging evidence for the selected toolchain, not portable language
contracts.

## Safety And Production Notes

- Every example is learning-only and requires project-specific review.
- The normal and sanitizer targets execute no intentional undefined behavior.
- No example uses raw owning `new` or `delete`.
- Borrowed C++ references and C context pointers require externally managed
  lifetimes.
- The suite has no iterators or containers, so iterator invalidation is not
  applicable.
- The suite has no threads or locks, so it demonstrates neither race safety nor
  deadlock handling.
- The suite throws no exceptions. A production error and exception policy must
  still be documented.
- The C table version check demonstrates policy shape, not a complete compatible
  ABI/version-negotiation design.
- Virtual dispatch cost and layout must be measured on the actual target.

## Suggested Experiments

1. Remove `const` from an override and observe the `override` error.
2. Remove `using Base::function` in a small overload hierarchy and inspect name
   hiding.
3. Add a third `Sensor` implementation and reuse the same alarm tests.
4. Return an `Alarm` borrowing a local sensor, run it under ASan, then repair
   the ownership/lifetime design.
5. Add a cleanup callback to the C operation table and document who may call
   it.
6. Inspect symbols or class layout with a second compiler and compare results.
