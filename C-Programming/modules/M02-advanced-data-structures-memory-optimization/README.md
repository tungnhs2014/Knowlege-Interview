# M02 — Advanced Data Structures & Memory Optimization

> **Current gate:** M02 interview review — human approval pending.

## Module purpose

Build on M01's reliability and memory-foundation practices to model compound data, reason about representation and layout boundaries, and design portable C interfaces with predictable resource use.

## Source mapping

- `docs/C Advanced/C Advacne DevLinux.txt`, Week 2 Day 3 → Lesson 01.
- `docs/C Advanced/C Advacne DevLinux.txt`, Week 2 Day 4 → Lesson 02.
- `docs/C Advanced/session-03.md` → four future module exercises.
- `docs/C Advanced/session-04.md` → three future module exercises; Exercise 2 Part B remains part of Exercise 2.
- `docs/C Advanced/Full-Embedded-C-Notes.md` → supplementary explanation and interview input only; technical claims require verification.

## Canonical lessons

1. `01-structures-unions-hardware-mapping.md` — Structures, Unions & Hardware Mapping
2. `02-object-oriented-c-portability.md` — Object-Oriented C & Portability

## Prerequisite and boundaries

M02 requires the approved M01 baseline: fixed-width type rationale, diagnostic discipline, basic pointer validation, and the `.data`/`.bss` mental model. It may reference those foundations without reteaching them.

Lesson 01 owns compound data modeling, layout observations, alignment and packing risks, enums and bit-fields, endianness as representation, pointers to compound objects, and a small `T **` bridge. M03 owns deep double-pointer work, arrays of pointers, complex declarations, function-pointer mastery, callbacks, command dispatch, and finite-state machines.

Lesson 02 owns opaque types, translation-unit data hiding, struct-based interfaces, controlled polymorphic dispatch, HAL-style decoupling, fixed-capacity pools, resource predictability, and endian-safe byte decoding. M05 owns `volatile`, real memory-mapped I/O access, and hardware-register manipulation in depth.

## Exercise-source and planned-solution mapping

| Source identity | Future solution directory |
| --- | --- |
| S03-E01 — Endianness Checker using Union | `solutions/session-03-exercise-01-endianness-checker/` |
| S03-E02 — Struct Padding / Alignment / Packed Struct Analyzer | `solutions/session-03-exercise-02-struct-padding-analyzer/` |
| S03-E03 — Enum Bitmask Permissions Tester | `solutions/session-03-exercise-03-bitmask-permissions/` |
| S03-E04 — Packed Union + Struct Bit-fields + Peripheral Union Pattern | `solutions/session-03-exercise-04-packed-union-bitfields/` |
| S04-E01 — Polymorphic Display Driver | `solutions/session-04-exercise-01-polymorphic-display-driver/` |
| S04-E02 — Object Pool Allocator, including Part B | `solutions/session-04-exercise-02-object-pool-allocator/` |
| S04-E03 — Endian-Safe Protocol Parser | `solutions/session-04-exercise-03-endian-safe-protocol-parser/` |

The future module exercise set contains exactly seven source exercises in the table's order. No exercise may be invented, merged, split, omitted, replaced, or reordered. Each exercise has exactly one corresponding solution entry.

## Module workflow

M02-L01 is the current review gate. After explicit human approval, proceed only to M02-L02. Do not create exercises, solutions, or interview material until the preceding approved workflow gate authorizes them.
