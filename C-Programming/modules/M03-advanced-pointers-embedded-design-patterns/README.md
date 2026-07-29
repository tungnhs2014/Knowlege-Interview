# M03 — Advanced Pointers & Embedded Design Patterns

> **Status:** COMPLETE — lessons, exercises, seven solutions, and interview
> material approved.
>
> **Current gate:** M03 complete; ready for M04 bootstrap.

## 1. Module Overview

M03 turns the pointer, declaration, and program-organisation foundations from M01 and M02 into controlled embedded design techniques. Its two canonical lessons cover advanced declarations, pointer-based tables and function pointers first; callbacks, command dispatch, and finite state machines (FSMs) second.

The module is defined by DevLinux Week 3, Days 5–6, and its Session 05–06 exercises. It does not create a general-purpose pointer reference, a complete hardware abstraction layer, or a broad design-pattern catalogue.

The future canonical structure is:

```text
README.md
01-arrays-double-pointers-function-pointers.md
02-callbacks-function-pointers-fsm.md
exercises.md
solutions/
interview.md
```

Only this bootstrap README exists at this gate.

## 2. Locked Source Mapping

| Source path | Exact heading or subsection | Intended use | Confidence | Verification required |
| --- | --- | --- | --- | --- |
| `C-Programming/docs/C Advanced/C Advacne DevLinux.txt` | `Week 3: Advanced Pointers & Embedded Design Patterns (Practical)`; `Day 5: Arrays of Pointers & Function Pointers` | Defines the L01 subject boundary and Session 05 preparation. | High | Pedagogical decomposition and technical verification. |
| `C-Programming/docs/C Advanced/C Advacne DevLinux.txt` | `Day 6: Function Pointers in Finite State Machines (FSM)` | Defines the L02 subject boundary and Session 06 preparation. | High | Pedagogical decomposition and technical verification. |
| `C-Programming/docs/C Advanced/session-05.md` | `Exercise_1 [build]` — `Error Message Table (Array of String Pointers)` | Locked source for S05-E01. | High | Exercise-level technical review before implementation. |
| `C-Programming/docs/C Advanced/session-05.md` | `Exercise_2 [build]` — `UI Menu Dispatcher (Jump Table)` | Locked source for S05-E02. | High | Toolchain and section-placement claims require target/build verification. |
| `C-Programming/docs/C Advanced/session-06.md` | `Exercise_1 [build]` through `Exercise_5 [build]` | Locked sources for S06-E01 through S06-E05, in source order. | High | Resolve documented source inconsistencies before exercise authoring. |
| `C-Programming/docs/C Advanced/Full-Embedded-C-Notes.md` | `1b. Pointers and Arrays — Same Memory, Two Views`; `3. Double Pointer (Pointer to Pointer)`; `19. Function Pointers` | Discovery and legacy-explanation input only. | Low | Canonical explanation must be independently verified and rewritten. |
| `C-Programming/docs/C Advanced/C advaced outline devlinux.txt` | `03 Advanced Pointers & Embedded Design Patterns (Practical)`; `Arrays of Pointers & Function Pointers`; `Function Pointers in Finite State Machines (FSM)` | Confirms the M03 two-lesson progression: Day 5 / L01 before Day 6 / L02. | High | Align lesson authoring with the detailed Week 3 source. |

Session 05 and Session 06 define the actual exercises. Future work must preserve their seven identities, order, learning objectives, and core requirements. It must not invent, merge, split, omit, replace, or silently reorder exercises; only technically necessary wording corrections that preserve intent are permitted.

## 3. Learning Outcomes

After completing the approved module, the learner can:

1. Read advanced C declarations systematically and distinguish arrays, pointers, pointer-to-pointer parameters, and function pointers.
2. Select a clear representation for arrays of pointers and arrays of structure pointers, including their bounds and lifetime contracts.
3. Define type-compatible function-pointer typedefs, tables, and explicit table-index validation.
4. Explain custom input-section requests as controlled compiler/toolchain features and inspect resulting binary sections and symbols appropriately.
5. Design a callback interface with explicit registration, invocation, lifetime, and null-validation contracts.
6. Build small command-dispatch and FSM designs with defined states, uniform handler signatures, checked table access, and a recovery path for invalid state or handler data.

## 4. Canonical Lesson Map

| Lesson | Canonical title | Primary sources | Included scope | Explicit boundary |
| --- | --- | --- | --- | --- |
| M03-L01 | Arrays, Double Pointers & Function Pointers | Week 3 Day 5; Session 05 | Advanced declaration reading; array/pointer relationships; arrays of pointers and structure pointers; double pointers; function-pointer declarations and typedefs; function-pointer arrays; explicit dispatch tables; controlled custom-section requests; section/symbol inspection. | Prepares Session 05 without providing its solutions. It is not a complete pointer manual or a linker-script lesson. |
| M03-L02 | Callbacks, Function Pointers & FSM | Week 3 Day 6; Session 06 | Callback registration, invocation, lifetime, and null validation; simple module interfaces; command tables and string-to-command dispatch; FSM state/handler concepts; state tables; transition/action separation; retry handling; bounds, null-handler, and invalid-state recovery. | Prepares Session 06 without providing its solutions. It does not establish ISR, thread, atomic, or hardware-register rules. |

## 5. Locked Exercise Inventory

| Order | Identifier | Source identity | Canonical owner | Future artifact status |
| --- | --- | --- | --- | --- |
| 1 | S05-E01 | Error Message Table | M03-L01 | NOT_STARTED |
| 2 | S05-E02 | UI Menu Dispatcher | M03-L01 | NOT_STARTED |
| 3 | S06-E01 | Timer Callback Registration System | M03-L02 | NOT_STARTED |
| 4 | S06-E02 | Single-Instance ADC Driver | M03-L02 | NOT_STARTED |
| 5 | S06-E03 | String-Driven Command Dispatcher | M03-L02 | NOT_STARTED |
| 6 | S06-E04 | Traffic Light Function-Pointer FSM | M03-L02 | NOT_STARTED |
| 7 | S06-E05 | WiFi Scalable FSM | M03-L02 | NOT_STARTED |

Session 05 is owned by L01. Session 06 is primarily owned by L02. The inventory above is locked at seven exercises; it is not evidence that the exercises have been reviewed, implemented, or approved.

## 6. Future Solution Mapping

Each exercise will have exactly one corresponding solution entry. Solution identification and order must follow `exercises.md` and the locked inventory above. These future paths are mappings only; none is created at this bootstrap gate.

```text
solutions/
├── session-05-exercise-01-error-message-table/
├── session-05-exercise-02-ui-menu-dispatcher/
├── session-06-exercise-01-timer-callback-registration/
├── session-06-exercise-02-single-instance-adc-driver/
├── session-06-exercise-03-command-dispatcher/
├── session-06-exercise-04-traffic-light-fsm/
└── session-06-exercise-05-wifi-scalable-fsm/
```

## 7. Technical Correction Register

| # | Correction heading | Locked correction |
| --- | --- | --- |
| 1 | `const` intent versus physical placement | `const` expresses read-only intent; `static const` does not universally mean Flash placement. Physical placement depends on compiler, linker script, target, and build configuration. |
| 2 | Custom-section request versus final placement | `__attribute__((section(".my_dispatch_table")))` requests an input section. The linker script and memory map determine final placement. |
| 3 | Binary-inspection evidence | Prefer `readelf -S`, `readelf -s`, `objdump -h`, and `objdump -t` for section/symbol inspection. `nm` and `nm -S` are supplemental evidence. |
| 4 | Dispatch-table performance claim | A compiler may optimise a dense `switch` into a jump table. A function-pointer table is primarily an architectural choice, not a universal speed improvement. |
| 5 | Timer double-registration timing | The canonical second registration attempt occurs while the timer is still running, before `stop`; the Session 06 expected-output ordering conflicts with its stated requirement. |
| 6 | Traffic-light transition authority | The Session 06 transition description and expected output disagree. The future function-pointer FSM follows the source `switch` behaviour, not the contradictory output sequence. |
| 7 | WiFi FSM consistency and retry ownership | The Session 06 inputs, retry expectations, and extra automatic step are inconsistent, and a `static` retry counter cannot be reset directly by another handler. Exercise authoring must establish one internally consistent input sequence, transition rule, and retry-reset mechanism without building a large context framework. |
| 8 | Callback execution context | A callback interface does not imply ISR or thread execution. Concurrent, atomic, ISR, and scheduling constraints require an explicit execution context and belong to later modules. |
| 9 | Function-pointer compatibility | Registered and tabled functions must have compatible function-pointer types. Casts must not be used to hide a mismatch. |
| 10 | FSM table safety | Indexed FSM dispatch requires contiguous valid state values or an explicit mapping, bounds checks, null-handler checks, and invalid-state recovery. |

## 8. Scope Boundaries

| M03 owns | Belongs elsewhere |
| --- | --- |
| Advanced pointers; double pointers; arrays of pointers; function pointers and tables; callback contracts; command dispatch; small FSM designs. | Deep `volatile`, real MMIO, hardware registers, and ISR-specific hardware behaviour belong to M05. Threads, mutexes, atomics, and race handling belong to M06. Unit-test and mocking frameworks belong to M07. Full serialization and networking belong to M08. |

Linker-script coverage is limited to explaining how a controlled custom-section request reaches final placement; M03 does not teach linker-script authoring.

## 9. Authoring Workflow

1. Human approval of this source map.
2. Author M03-L01 only.
3. Human review and explicit approval of M03-L01.
4. Author M03-L02 only.
5. Human review and explicit approval of M03-L02.
6. Author `exercises.md` only from the locked source inventory.
7. Human review and explicit approval of the exercises.
8. Implement the seven solutions in locked order, one authorized exercise at a time.
9. Review and approve the solutions.
10. Author `interview.md` only.
11. Perform the M03 final audit.

Codex stops at every review gate. Completion, silence, or a successful command is not approval, and no later artifact or topic begins automatically.

## 10. Current Status

The source mapping, seven-exercise inventory, future solution mapping, correction register, and module boundaries are ready for human review. No M03 lesson, exercise, solution, or interview artifact has been created. This bootstrap is not approved until the user explicitly approves it.
