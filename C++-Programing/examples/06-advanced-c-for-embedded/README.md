# 06 - Advanced C For Embedded: Examples

## Status

These are **learning-focused C17 examples**, not production-ready firmware,
drivers, protocol libraries, or synchronization primitives.

The programs demonstrate:

- checked, unsigned bit-mask operations;
- duplicate argument evaluation in an unsafe macro without executing undefined
  behavior;
- callback plus borrowed context lifetime;
- a validated table-driven FSM;
- a fakeable HAL using a const operation table;
- explicit big-endian encoding and decoding;
- a fixed-capacity, single-threaded ring buffer.

The suite intentionally does not access physical addresses. Real MMIO,
`volatile` behavior, barriers, interrupt functions, register side effects, and
access widths require the selected compiler, architecture, device, and product
documentation.

## Requirements

- GNU Make
- GCC or Clang with C17 support
- AddressSanitizer and UndefinedBehaviorSanitizer
- Optional: GDB, `objdump`, or another target-appropriate debugger

Build, run, and sanitize every example:

```bash
make check
```

Use Clang:

```bash
make clean
make check CC=clang
```

Remove generated files:

```bash
make clean
```

Generated files are placed under `build/`.

## Example Map

| Target | Source | Main lesson |
| --- | --- | --- |
| `bits` | `bits/bit_masks.c` | Validate shifts and use width-aware unsigned masks |
| `macros` | `macros/macro_vs_inline.c` | A macro may evaluate one argument twice; an inline function does not |
| `callbacks` | `callbacks/callback_context.c` | Function pointer, borrowed context, invocation, and unregister |
| `fsm` | `fsm/table_fsm.c` | Validated transitions and explicit rejection |
| `hal` | `hal/fakeable_hal.c` | Capability interface, const ops table, context, and host fake |
| `protocol` | `protocol/endian_codec.c` | Bounds checks and explicit big-endian representation |
| `ring` | `ring_buffer/ring_buffer.c` | Full/empty policy and wraparound in one execution context |
| `sanitize` | All sources | Host-side ASan and UBSan execution |

## 1. Checked Bit Masks

```bash
make bits
./build/bits
```

The example accepts bit indices `0` through `31`, rejects `32`, and uses
`UINT32_C(1)` before shifting.

**UB warning:** a runtime shift count equal to or greater than the promoted
operand width is undefined behavior. Signed left shifts also have additional
risks.

**Register warning:** the helper operates on ordinary values. Do not apply
generic read-modify-write helpers to write-one-to-clear, read-to-clear,
write-only, reserved, or self-clearing register fields.

## 2. Macro Versus Inline Function

```bash
make macros
./build/macros
```

Expected observation:

```text
macro-result=10 calls=2
function-result=10 calls=1
```

The macro evaluates `next_value()` once for its comparison and again for the
selected result. The program demonstrates duplicated evaluation safely; it
does not use an expression such as `MAX_UNSAFE(index++, limit)`, whose expanded
side effects can be undefined or otherwise incorrect.

Inspect preprocessed output:

```bash
make preprocess
less build/macro_vs_inline.i
```

Prefer `static inline` functions for typed computations. Keep macros for jobs
that genuinely require preprocessing.

## 3. Callback And Context

```bash
make callbacks
./build/callbacks
```

The `SampleListener` borrows `SampleStats`. The context must remain alive for
every invocation.

The example is synchronous and single-threaded. `listener_clear` demonstrates
an unregister state but is not a concurrent cancellation protocol.

**Ownership/lifetime warning:** clearing a stored pointer does not make it safe
to destroy the context while another execution context may already be invoking
the callback.

## 4. Table-Driven FSM

```bash
make fsm
./build/fsm
```

The FSM:

- searches a const transition table;
- validates by matching before changing state;
- rejects `STOP` while idle;
- keeps transition policy free of hardware operations.

Production FSMs must also define action ordering, invalid enum representations,
event queue ownership, overflow policy, reentrancy, and synchronization.

## 5. Fakeable HAL

```bash
make hal
./build/hal
```

The application-facing `Sensor` contains:

- a pointer to an immutable operation table;
- an instance context pointer.

The fake records configuration and reads without simulating physical
registers. A real target implementation would replace the fake operations
while preserving the capability-level interface.

**Production note:** define initialization, deinitialization, context ownership,
error categories, timing, versioning, and optional-operation policy.

## 6. Explicit Endian Codec

```bash
make protocol
./build/protocol
```

The decoder checks input length before reading and constructs a `uint16_t`
explicitly from big-endian bytes. The encoder performs the reverse operation.

Do not replace this with a cast to a native or packed structure. Packing does
not solve byte order, short input, scalar representation, versioning, or
possibly unaligned access.

## 7. Single-Threaded Ring Buffer

```bash
make ring
./build/ring
```

This implementation uses:

- fixed storage;
- `head`, `tail`, and `count`;
- reject-on-full behavior;
- FIFO pop order.

It is deliberately **not thread-safe or interrupt-safe**.

**Race warning:** adding `volatile` to the indices would not provide atomicity,
publication ordering, or mutual exclusion. A producer/consumer design needs a
reviewed synchronization protocol based on the real target and execution
model.

## Debugging

Build unoptimized binaries with debug information:

```bash
make debug
gdb ./build/fsm
```

Useful FSM commands:

```text
break next_state
run
p current
p event
p *out_next
```

Inspect callback state:

```text
gdb ./build/callbacks
break listener_notify
run
p *listener
p *(SampleStats *)listener->context
```

Inspect generated assembly for one exact compiler/build:

```bash
make assembly
less build/bit_masks.s
```

Generated assembly is evidence for that compiler, version, flags, and target.
It is not a portable C guarantee.

## Sanitizers

Run all examples under ASan and UBSan:

```bash
make sanitize
```

Sanitizers help detect executed host-side bounds, lifetime, alignment, and
undefined-behavior defects. They do not prove:

- physical register correctness;
- bus transaction width;
- hardware ordering;
- interrupt safety;
- absence of data races;
- timing correctness;
- complete input coverage.

ThreadSanitizer is not used because these examples create no threads and the
ring buffer explicitly supports only one execution context.

## Safety Notes

- No example uses dynamic allocation, so allocation ownership is not exercised.
- Callback state is borrowed and must outlive invocation.
- No C++ exceptions or iterators are used, so exception safety and iterator
  invalidation do not apply directly.
- No locks or threads are created, so these examples do not demonstrate race
  freedom or deadlock prevention.
- No intentionally unsafe binary is executed by `make check`.
- The macro example exposes duplicated evaluation without invoking UB.
- Real asynchronous handlers must avoid operations forbidden by the selected
  signal, interrupt, compiler, device, and RTOS contracts.

## Production Checklist

- Validate dynamic shift counts before shifting.
- Use unsigned types for masks and bitwise operations.
- Review each peripheral register's exact read/write semantics.
- Keep MMIO and target extensions behind narrow target modules.
- Treat `volatile`, atomicity, ordering, and synchronization separately.
- Prefer typed inline functions to computational macros.
- Define callback context ownership, lifetime, reentrancy, and teardown.
- Validate command, state, and event values before table indexing.
- Keep operation tables immutable when possible.
- Separate hardware mechanism from host-testable policy.
- Decode and encode external bytes explicitly.
- Define ring-buffer full, empty, overflow, ownership, and synchronization
  policies.
- Compile and test every supported configuration and target toolchain.

## Practice

1. Add checked clear, toggle, any-set, and all-set operations to `bits`.
2. Add a safe statement-like logging macro using `do { ... } while (0)` and
   inspect its expansion.
3. Add callback replacement and document whether invocation can be reentrant.
4. Exhaustively test every FSM state/event combination.
5. Add failure injection to the fake sensor and test retry policy.
6. Extend the protocol codec with a 32-bit field and golden byte vectors.
7. Change the ring buffer to overwrite-oldest behavior and document the new
   contract.
