# 07 - Industrial C Practices: Examples

## Status

These are **learning-focused C17 examples**, not production-ready libraries,
firmware, compliance evidence, logging infrastructure, or test frameworks.

The suite demonstrates:

- a capacity-aware string copy with explicit truncation status;
- `errno` capture and translation into a domain error;
- runtime validation before an assertion-backed internal helper;
- checked allocation arithmetic and failure-preserving `realloc`;
- host testing of policy through a fake sensor;
- bounded logging with stable event IDs and visible truncation;
- strict compiler warnings, ASan/UBSan, and GCC `-fanalyzer`.

The examples intentionally remain small and single-threaded. They do not model
physical hardware, interrupts, real-time constraints, persistent logging,
concurrent cancellation, certification, or a complete organization coding
standard.

## Requirements

- GNU Make
- GCC or another C17 compiler for normal builds
- GCC with `-fanalyzer` support for `make analyze`
- AddressSanitizer and UndefinedBehaviorSanitizer for `make sanitize`
- Optional: GDB or Valgrind

Build, run, and sanitize all examples:

```bash
make check
```

Run compiler-integrated static analysis:

```bash
make analyze
```

Build without running:

```bash
make all
```

Remove generated files:

```bash
make clean
```

Generated binaries are placed under `build/`.

## Example Map

| Target | Source | Main lesson |
| --- | --- | --- |
| `strings` | `strings/bounded_copy.c` | Capacity includes the null terminator; truncation is reported |
| `errors` | `errors/errno_translation.c` | Check the primary result, save `errno`, return a domain status |
| `assertions` | `assertions/assert_vs_runtime.c` | Runtime validation protects public input; assertions protect internal invariants |
| `resize` | `analysis/checked_resize.c` | Check multiplication, preserve ownership on failure, define zero-count behavior |
| `fake` | `testing/fake_sensor.c` | Separate policy from hardware and inject failure with a host fake |
| `logger` | `logging/bounded_logger.c` | Bounded formatting, stable event IDs, and side effects outside log arguments |
| `analyze` | All sources | GCC path-sensitive static analysis using the real source configuration |
| `sanitize` | All sources | Host-side ASan and UBSan execution |

## 1. Capacity-Aware String Copy

```bash
make strings
./build/strings
```

The function:

- requires a non-null destination and source;
- requires nonzero destination capacity;
- reserves one byte for `'\0'`;
- always terminates a valid destination;
- returns `false` when truncation occurs.

**Unsafe C API warning:** `strcpy`, `strcat`, and `sprintf` cannot enforce an
unknown destination capacity. `strncpy` is not a universal safe replacement
because it may omit termination and has fixed-field padding semantics.

## 2. `errno` Translation

```bash
make errors
./build/errors
```

The example first checks `fopen`'s primary failure result, then saves `errno`
before another library call can replace the diagnostic. It returns a stable
domain status while retaining the platform error separately.

Do not inspect a stale nonzero `errno` as if it were a universal failure flag.
Real error mappings must be designed for the selected platform and product.

## 3. Assertion Versus Runtime Validation

```bash
make assertions
./build/assertions
make ndebug
```

The public helper rejects invalid input at runtime. The internal helper asserts
the invariant already established by its caller.

`make ndebug` compiles with `NDEBUG` and confirms that required behavior does
not depend on assertion evaluation.

**UB warning:** calling the internal helper directly with a null pointer or
zero count is outside its contract. The example never executes that invalid
path.

## 4. Checked Resize And Ownership

```bash
make resize
./build/resize
```

The resize helper:

- validates the owning pointer;
- checks `count * sizeof(element)` for overflow;
- defines count zero as release-and-null;
- assigns the result only after `realloc` succeeds;
- preserves the original allocation and values when a request is rejected.

**Ownership warning:** one `int **` API does not prevent other aliases from
dangling. Production code must define one owner and restrict concurrent access.
The example is single-threaded and does not prove race freedom.

## 5. Fakeable Sensor Policy

```bash
make fake
./build/fake
```

The alarm policy depends on a function pointer plus context rather than direct
hardware access. The fake tests:

- value above the threshold;
- the exact threshold boundary;
- dependency failure;
- call count.

The fake context is borrowed and must outlive every call. Host tests do not
validate MMIO, interrupt behavior, electrical behavior, or target timing.

## 6. Bounded Logger

```bash
make logger
./build/logger
```

The logger writes a stable level and event ID, appends a bounded message, and
reports truncation. The status-producing operation runs before logging so
compile-time log removal cannot remove required behavior.

**Variadic API warning:** the compiler cannot fully enforce a custom format
protocol. Every format specifier must match its promoted argument type.
Production logging also needs privacy, rate-limit, recursion, synchronization,
sink-full, persistence, and asynchronous-context policies.

## Static Analysis

```bash
make analyze
```

This target compiles each source to a discarded object with GCC `-fanalyzer`
and the same strict C17 warning flags as the normal build. A clean result means
that this compiler/version/configuration did not report a modeled defect. It
does not prove absence of defects. `make check` includes this analyzer pass.

Other projects may add Clang Static Analyzer, `clang-tidy`, or `cppcheck`.
Configure each tool with the real include paths, macros, generated headers,
target model, and compile commands before trusting its report.

## Sanitizers

```bash
make sanitize
```

The sanitizer build enables AddressSanitizer and
UndefinedBehaviorSanitizer. These tools can expose executed host-side bounds,
lifetime, allocation, alignment, shift, and selected undefined-behavior
defects.

They do not prove:

- complete path coverage;
- target MMIO or interrupt correctness;
- timing correctness;
- absence of data races;
- absence of every form of undefined behavior;
- compliance with MISRA C, BARR-C, CERT C, or a project standard.

ThreadSanitizer is not included because the examples create no threads.

## Debugging

Build unoptimized binaries with debug information:

```bash
make debug
gdb ./build/resize
```

Useful commands:

```text
break resize_ints
run
print count
print *items
continue
```

For hosted memory diagnostics where available:

```bash
valgrind --leak-check=full --track-origins=yes ./build/resize
```

Valgrind and GDB behavior is platform-specific. Neither replaces target
integration testing.

## Safety And Production Notes

- All examples are learning-only and require product-specific review before
  reuse.
- The suite executes no intentional undefined behavior.
- Dynamic allocation appears only in `checked_resize.c`; the caller owns the
  allocation and the zero-count path releases it.
- No C++ code, exceptions, containers, or iterators are used. Exception safety
  and iterator invalidation do not apply directly.
- No threads or locks are used. The examples demonstrate neither race freedom
  nor deadlock prevention.
- The fake and logger context/lifetime rules would require synchronization if
  calls could overlap.
- Unsafe C string APIs are discussed but not used.
- Sanitizer and analyzer success is evidence for the exercised build, not a
  correctness or compliance certificate.

## Production Checklist

- Define the supported C edition, compiler versions, extensions, and warnings.
- Document input, output, capacity, ownership, lifetime, and error contracts.
- Validate before indexing, copying, allocating, dispatching, or changing
  state.
- Preserve outputs and ownership on failure unless documented otherwise.
- Define string truncation and logging sink-full behavior.
- Keep required side effects outside assertions and logging.
- Add boundary, negative, failure-injection, and cleanup tests.
- Run static analysis with the real compilation model.
- Run sanitizer jobs on meaningful host tests.
- Verify target timing, memory, hardware, and configuration assumptions
  separately.
- Record tool versions, reviewed suppressions, deviations, and CI evidence.
