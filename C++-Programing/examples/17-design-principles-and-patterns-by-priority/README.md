# 17 - Design Principles And Design Patterns By Priority Examples

These examples are intentionally small and compile-oriented. They show the
priority rule for this topic: start with simple C++ mechanisms, then use a
pattern only when it makes ownership, lifetime, variation, or subsystem
coordination clearer.

## Files

| File | Purpose | Status |
| --- | --- | --- |
| `fsm_vs_state.cpp` | Compares a simple `enum class` FSM with a small State-pattern version | Learning-only comparison |
| `strategy_callbacks.cpp` | Shows Strategy with a lambda/template and with a runtime interface | Production-style shapes, simplified |
| `observer_subscription.cpp` | Observer with subscription IDs and explicit unsubscribe | Learning-only lifecycle model |
| `factory_adapter_facade.cpp` | Factory Method, Adapter, and Facade in one small parser/logger scenario | Production-style ownership shape, simplified |
| `command_ownership.cpp` | Command queue with safe value captures and notes about reference-capture risk | Learning-only command queue |
| `Makefile` | Build, run, sanitizer, strict-warning, and cleanup commands | Practical |

## Build

From this directory:

```sh
make
```

Build one example:

```sh
make fsm_vs_state
make strategy_callbacks
make observer_subscription
make factory_adapter_facade
make command_ownership
```

## Run

```sh
make run
```

Or run one binary:

```sh
./fsm_vs_state
./strategy_callbacks
./observer_subscription
./factory_adapter_facade
./command_ownership
```

## Sanitizer / Debug Commands

AddressSanitizer + UndefinedBehaviorSanitizer:

```sh
make sanitize
```

Warnings as errors:

```sh
make strict
```

Clean generated files:

```sh
make clean
```

If you adapt these examples and add threads to Observer or Command queues, add
ThreadSanitizer:

```sh
g++ -std=c++17 -g -O1 -fsanitize=thread -fno-omit-frame-pointer -pthread your_file.cpp -o your_file_tsan
```

## Safety Notes

- These are learning examples. They intentionally keep behavior small so the
  design tradeoff is visible.
- Prefer the `enum class` FSM when states and transitions are small and stable.
  The State pattern version is useful only when state-specific behavior grows.
- The runtime Strategy example owns its strategy with `std::unique_ptr`. Do not
  return or store raw owning pointers from factories or strategy providers.
- `observer_subscription.cpp` demonstrates explicit unsubscribe. A production
  Observer usually needs a stronger subscription token, reentrancy policy, and
  thread-safety policy.
- `command_ownership.cpp` captures command data by value. Storing lambdas that
  capture stack variables by reference can create dangling references.
- `factory_adapter_facade.cpp` keeps Adapter and Facade thin. Do not hide
  important error behavior or turn a Facade into a god object.
- Iterator invalidation matters if you store iterators to `std::vector` or erase
  observers while notifying. Define a mutation policy before adding that feature.
- No example here is thread-safe. If you add concurrency, document lock order and
  avoid calling user callbacks while holding locks unless the policy is explicit.

## Practice Changes

1. Add an invalid-event counter to `fsm_vs_state.cpp` and decide whether it
   belongs in the FSM or a separate monitoring object.
2. Add a CRC-like checksum strategy to `strategy_callbacks.cpp`.
3. Change `observer_subscription.cpp` so an observer can unsubscribe during
   notification, then document the iteration policy.
4. Add a second parser type to `factory_adapter_facade.cpp` and explain when the
   factory becomes worthwhile.
5. Add undo commands to `command_ownership.cpp` using value-owned state.
6. Replace one runtime interface with a template policy and compare readability.
