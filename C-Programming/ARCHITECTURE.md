# C Programming Curriculum Architecture

## 1. Purpose

This document defines the repository-level architecture for the standalone DevLinux-centered C curriculum. It governs curriculum scope, module structure, source authority, artifact ownership, technical-claim discipline, and human review gates.

The curriculum contains ten modules. Each module contains exactly two canonical lesson Markdown files and the module-level supporting artifacts defined below. Curriculum work proceeds module by module and through explicit human-review gates; completing a file or command never authorizes the next gate.

This architecture does not authorize creation or revision of a module artifact by itself. An explicit task and the preceding human approval are required.

## 2. Canonical Repository Model

The canonical module shape is:

```text
C-Programming/
├── ARCHITECTURE.md
├── docs/
│   └── C Advanced/
└── modules/
    └── Mxx-<module-slug>/
        ├── README.md
        ├── 01-<lesson-slug>.md
        ├── 02-<lesson-slug>.md
        ├── exercises.md
        ├── solutions/
        └── interview.md
```

Repository invariants:

- There are exactly ten canonical modules, identified as M01 through M10.
- Every module has exactly two canonical lesson Markdown files, numbered `01` and `02`.
- Exercises, solutions, and interview material are module-level artifacts rather than lesson directories.
- A module `README.md` maps both lessons and all corresponding DevLinux exercise sources before content authoring begins.
- Supporting build files may exist within individual solution directories, but they are not additional curriculum lessons.
- Prerequisite or beginner material may be referenced as entry preparation, but it does not become an additional canonical module or lesson.

M01, M02, and M03 currently demonstrate this physical structure. M04 through M10 must adopt the same structure when their module bootstrap is explicitly authorized; this document does not create them.

## 3. Canonical Module and Lesson Map

The detailed DevLinux roadmap is the primary authority for the sequence below.

| Module | Canonical module title | Lesson 01 | Lesson 02 | DevLinux scope |
| --- | --- | --- | --- | --- |
| M01 | Coding Standards & Memory Foundation | High-Reliability Coding Standards | Memory Layout & Failure Analysis | Week 1, Days 1–2 |
| M02 | Advanced Data Structures & Memory Optimization | Structures, Unions & Hardware Mapping | Object-Oriented C & Portability | Week 2, Days 3–4 |
| M03 | Advanced Pointers & Embedded Design Patterns | Arrays, Double Pointers & Function Pointers | Callbacks, Function Pointers & FSM | Week 3, Days 5–6 |
| M04 | Standard I/O, Variadic Functions & Secure Standard I/O | Variadic Functions, Macros & Building a Logging Module | I/O Safety & Security Hacking | Week 4, Days 7–8 |
| M05 | Low-Level Hardware Control & File Systems | Low-Level Hardware Interfacing | Fundamentals of Low-Level I/O & System Calls | Week 5, Days 9–10 |
| M06 | Process & Thread Management | Threading Models & History | Modern C11 Threading & Synchronization | Week 6, Days 11–12 |
| M07 | Build Systems, Unit Testing & String/Memory Handling Algorithms | Build Systems (CMake) & Test-Driven Development | String Memory Architecture, Advanced Manipulation & Standard Algorithms | Week 7, Days 13–14 |
| M08 | Robust Network Sockets & Numerical Processing | Industrial Network Socket Programming | Numerical Computations, Precision & Network Serialization | Week 8, Days 15–16 |
| M09 | Advanced Debugging & Profiling | Defensive Programming & Static Analysis | Advanced Error Handling Tools & Profiling | Week 9, Days 17–18 |
| M10 | Industrial Mock Project | Mock Project Implementation | Project Review & Standards Audit | Week 10, Days 19–20 |

The compact roadmap incorrectly repeats `Threading Models & History` as M05 Lesson 02. The detailed roadmap identifies Week 5 Day 10 as `Fundamentals of Low-Level I/O & System Calls`; that detailed entry controls the canonical map. The inconsistency must remain recorded in the relevant module correction register rather than being silently hidden.

## 4. Language and Execution-Context Baseline

The primary course baseline is ISO C99 because that is the DevLinux course baseline. A lesson may introduce a later language revision only when its approved module scope requires it. For example, M06 Lesson 02 explicitly introduces C11 threading and synchronization features.

Every exception must:

- name the applicable language revision;
- separate later-standard facilities from the C99 baseline;
- state relevant implementation and library availability constraints; and
- avoid implying that the facility is available in every embedded toolchain.

Compiler extensions, POSIX interfaces, Linux behavior, ABI conventions, object-file behavior, linker behavior, and target hardware behavior are separate contexts. They must be labelled and sourced rather than presented as ISO C guarantees.

## 5. Source Inventory and Authority

### 5.1 Discovered curriculum sources

The repository currently provides these curriculum inputs:

| Source path | Role |
| --- | --- |
| `docs/C Advanced/C Advacne DevLinux.txt` | Detailed ten-week, twenty-day roadmap; primary module and lesson progression. |
| `docs/C Advanced/C advaced outline devlinux.txt` | Compact curriculum outline; corroborating map that contains the recorded M05 Lesson 02 inconsistency. |
| `docs/C Advanced/session-01.md` through `session-07.md` | Current assignment sources in session order. Sessions 01–02 map to M01, 03–04 to M02, 05–06 to M03, and 07 begins M04 coverage. |
| `docs/C Advanced/Full-Embedded-C-Notes.md` | Supplementary legacy notes for topic discovery, migration, comparison, and identifying material that may require review. This file is neither a curriculum authority nor a technical authority; every reused technical claim requires independent verification against an appropriate authoritative source. |
| `docs/C Advanced/references/barr_c_coding_standard_2018.pdf` | BARR-C:2018 style and maintainability reference. |
| `docs/C Advanced/references/MISRA C 2012 Guidelines for the use of.pdf` | MISRA C:2012 safety-guideline, category, compliance, and deviation reference. |

No file named `docs/C Advanced/Lộ-trình-C-Advanced-DevLinux.txt` was discovered. The actual compact-roadmap filename is `docs/C Advanced/C advaced outline devlinux.txt`; authors must use that exact path and must not claim that the missing filename exists.

Sessions after `session-07.md` are not currently present. Their future arrival may extend source mapping and exercise inventory for later modules, but must not silently alter the ten-module/two-lesson structure or previously approved exercise identities.

### 5.2 Source hierarchy

Sources have different responsibilities. Scope authority and technical authority must not be conflated.

1. **DevLinux roadmap and session assignments** decide what is taught: curriculum scope, module order, exercise identity and order, required interfaces, expected behavior, and submission structure.
2. **ISO C and WG14 material** decide language semantics, including defined, implementation-defined, unspecified, and undefined behavior.
3. **MISRA C:2012** verifies applicable safety guidelines, categories, rationale, compliance, and deviation concepts.
4. **BARR-C:2018** verifies coding style, module organization, naming, braces, initialization, functions, macros, and maintainability practices.
5. **Official compiler and tool documentation** verifies GCC, GNU Binutils, GNU Make, GDB, linker, ELF, and other implementation-specific behavior.
6. **CERT C and other engineering references** supplement security and defensive-engineering guidance but do not override DevLinux curriculum scope.
7. **Supplementary repository notes** — `Full-Embedded-C-Notes.md` and other legacy notes are discovery, comparison, and migration inputs only. They do not define curriculum scope or establish technical correctness. Every reused claim must be verified using the appropriate language, standard, toolchain, platform, or security authority.

DevLinux decides **what** the curriculum teaches. Official language, standards, and toolchain references verify **whether and under what context** technical claims are correct. Engineering experience may improve explanations, examples, trade-offs, and practical application, but it never replaces an official definition.

Existing lesson content is migration evidence, not an independent technical authority. No source is copied blindly. Copyrighted standards are referenced and paraphrased; substantial standard text is not reproduced.

### 5.3 Source mapping requirements

Before either lesson in a module is authored, its `README.md` must identify:

- exact source path;
- exact relevant heading or exercise identity;
- intended lesson or artifact use;
- known conflict or ambiguity;
- confidence; and
- required semantic, toolchain, target, or pedagogical verification.

Detailed claim-level references belong in the affected lesson or artifact. A module README provides a concise map, not a duplicate source inventory or lesson.

### 5.4 Source immutability

During normal module authoring and refactoring, the DevLinux roadmap files, DevLinux session assignment files, MISRA C reference PDFs, BARR-C reference PDFs, and other files under `docs/C Advanced/references/` are read-only curriculum inputs. This immutability preserves provenance and review evidence; it does not mean that every technical statement in a source is correct.

Technical corrections are recorded in the affected module artifact, such as a module README correction register, a lesson clarification block, or an `exercises.md` source-inconsistency and correction block. Codex must not modify a source file merely to make it agree with module content.

Any source-file change requires a separate explicit task, a stated reason, human review, and explicit human approval.

## 6. Technical-Claim Policy

All authored artifacts must comply with these rules:

- Do not invent definitions.
- Do not state implementation-dependent behavior as universal C behavior.
- Distinguish ISO C semantics, compiler extensions, ABI behavior, object-file or ELF behavior, linker placement, operating-system behavior, and physical target memory.
- When a result depends on compiler, compiler version, target, ABI, linker script, library, build flags, or optimization, name that dependency.
- Major technical claims must be traceable to a source appropriate to the claim.
- Source inconsistencies must be documented and resolved explicitly; they must not be silently hidden.
- Examples must improve understanding without leaking a complete solution to an assigned exercise.
- Tool output is evidence for the recorded toolchain and build, not a universal language guarantee.
- Selected MISRA or CERT guidance does not establish full project compliance.

### 6.1 Reproducible tool evidence

When compiler, linker, binary-inspection, debugger, static-analysis, sanitizer, or runtime output supports a claim, record enough context to reproduce and interpret that evidence. Where relevant, record the tool and version, compiler and version, target architecture and ABI, language dialect, compiler and linker flags, complete command, inspected object file or binary, relevant output excerpt, and environment assumptions. Do not require metadata that is irrelevant to a simple command; evidence must remain proportionate to the claim.

A tool result proves only the recorded build, binary, target, toolchain, and environment. It must not be presented as a universal ISO C guarantee. Context is particularly necessary for claims about structure size and member offsets, enumeration size, symbol classification, ELF section membership, linker placement, disassembly, optimization effects, stack-frame observations, static-analysis diagnostics, sanitizer output, and runtime benchmarks.

Prohibited absolute claims include, but are not limited to:

- `const` always means Flash;
- zero-initialized objects always live in `.bss`;
- `switch` is always slower than a function-pointer table;
- `volatile` provides thread synchronization;
- packed access is universally safe; and
- an enumeration always occupies four bytes.

When a source assignment contains a technical error or inconsistent requirement, preserve its identity and intent, record the conflict, and apply the smallest technically correct clarification. The correction must be reviewable against both the original source and the relevant technical authority.

## 7. Lesson-Authoring Contract

### 7.1 Required teaching qualities

Every lesson must provide, in an order appropriate to its subject:

- accurate definitions;
- the purpose and problem being solved;
- a mental model or operating mechanism;
- why the concept matters;
- when to use it;
- when not to use it, including limitations and trade-offs;
- Embedded or Linux applications where relevant;
- examples only when they materially improve understanding;
- common mistakes and risks;
- key takeaways; and
- official references and source provenance.

These are quality requirements, not mandatory repeated headings. The lesson structure must follow the nature of the subject, and small concepts must not be forced into a mechanical template.

### 7.2 Topic-appropriate teaching and verification workflows

The author selects a workflow that fits the lesson profile:

| Lesson profile | Appropriate workflow and evidence |
| --- | --- |
| Coding standards | Contract review, compiler diagnostics, static analysis, compliance boundaries, and deviation reasoning. |
| Memory failure analysis | Symptom → hypothesis → evidence → tool → root cause → correction. |
| Structure and memory layout | `sizeof`, `offsetof`, ABI documentation, byte inspection, and representation verification. |
| Architecture and design patterns | Interface contract, ownership, dependency flow, capacity, failure policy, and test scenarios. |
| Pointer and dispatch-table lessons | Declaration reasoning, type compatibility, bounds, lifetime, null guards, and linker evidence when applicable. |
| Callback and FSM lessons | Event trace, state-transition table, invariants, retry ownership, invalid-state recovery, and sequence verification. |

Debugging is optional and lesson-dependent. Testing, performance analysis, and any other workflow are also included only when the approved subject genuinely requires them. Authors must not invent a debugging, testing, performance, or tool section merely to satisfy a template.

### 7.3 Length and scope policy

There is no numeric length target. Technical correctness and completeness take priority over brevity. A lesson must be deep enough to teach its approved scope to a newcomer while remaining useful as a refresher for an experienced engineer.

Authors remove repetition, trivia, unnecessary history, and later-module scope creep. Necessary depth must not be removed merely to shorten a lesson. Focus means that every included section serves the approved lesson objective; it does not mean that the lesson must be brief.

## 8. Artifact Responsibility Boundaries

| Artifact | Owns | Must not become |
| --- | --- | --- |
| `README.md` | Module purpose, prerequisites, exact DevLinux source mapping, two-lesson map, exercise inventory, scope boundaries, correction register, and module status. | A duplicate theory lesson or a solution guide. |
| `01-<lesson-slug>.md` and `02-<lesson-slug>.md` | Theory, definitions, mental models, mechanisms, reasons, use cases, limitations, focused examples, applicable verification methods, key takeaways, references, and provenance. | Interview question sets, full assignment text, complete solutions, or answer templates such as “Strong Interview Answer.” |
| `exercises.md` | DevLinux assignment identity, source-faithful DevLinux problem statement and requirements, design hints, acceptance criteria, expected output, and submission structure. | A rewritten replacement assignment, solution manual, or lesson. |
| `solutions/` | Exactly one solution entry for every exercise identity, in `exercises.md` order, plus build files, required runtime validation, and necessary tool evidence. | A second lesson, multiple competing canonical answers, or an implementation for an unapproved exercise. |
| `interview.md` | Interview questions, expected answers, interviewer intent, common incorrect answers, and reasonable follow-up questions. | A duplicated lesson, a new source of lesson-scale theory, or a solution leak. |

Within `exercises.md`, repository-authored additions must be visibly separated under these labels:

- **Canonical Technical Clarifications** — technically necessary clarification that preserves the source objective and core requirements;
- **Source Inconsistencies and Corrections** — a documented contradiction or technical defect and its reviewed resolution; and
- **Additional Repository Validation** — extra validation that is explicitly identified as repository-added rather than attributed to DevLinux.

DevLinux exercises must retain their actual identities, source order, interfaces, intended behavior, core requirements, expected output, and submission structure. Do not invent, merge, split, omit, replace, or silently rewrite them. A technical correction is permitted only when necessary for correctness and must not change the original learning objective or core requirements.

Each exercise has exactly one corresponding solution entry. Solution identification and ordering follow `exercises.md`. A solution demonstrates one reviewed implementation path; it does not claim to be the only valid design and does not repeat the full lesson.

Interview authoring prioritizes, in order, real questions supplied by the user, DevLinux-derived questions, and common Embedded C industry questions within the current module scope. Important WHY, HOW, code-review, and debugging extensions are permitted only within that scope and must not introduce new lesson-scale theory.

C++ comparisons are allowed only when they directly clarify a C concept or boundary. They must not introduce lesson-scale C++ content such as RAII, constructors or destructors, virtual dispatch, templates, the STL, C++ concurrency, or unrelated object-oriented C++ design.

## 9. Human-Reviewed Module Workflow

Every module follows these repository-level gates:

1. **Source mapping** — inspect roadmap, sessions, existing artifacts, and appropriate authoritative references; lock lesson scope and exercise provenance.
2. **Lesson outline** — define a focused, source-backed plan for both lessons and stop for human review.
3. **Lesson 1 authoring** — author only the first lesson using its topic-appropriate teaching and verification workflow.
4. **Human review and approval** — correct Lesson 1 until the user explicitly approves it.
5. **Lesson 2 authoring** — author only the second lesson after Lesson 1 approval.
6. **Human review and approval** — correct Lesson 2 until the user explicitly approves it.
7. **`exercises.md` restoration and review** — restore the mapped DevLinux assignments, separate corrections and repository validation, and stop for explicit approval.
8. **Solution consistency validation** — ensure exactly one solution per approved exercise, matching identity, order, interface, behavior, build, and validation requirements.
9. **`interview.md` audit** — verify technical accuracy, module scope, provenance priority, progression, and absence of lesson or solution duplication.
10. **Final module audit** — validate structure, statuses, source coverage, builds and tools where applicable, artifact consistency, cleanup, and repository scope.

Each gate stops for human review before the next major artifact is authored. Silence, elapsed time, completion of a prior command, or a successful validation is not approval. Corrections remain at the current gate until the user explicitly approves progression.

The same repository-level gates apply to every module, but lesson-internal teaching structure and verification evidence remain topic-dependent as defined in Section 7.2.

## 10. Status and Progression Rules

Each module README records the current gate and the approval state of its two lessons, exercises, solutions, interview material, and final audit. At minimum, artifacts distinguish:

- `NOT_STARTED`;
- `DRAFT` or the artifact-specific draft status;
- `HUMAN_REVIEW_PENDING`;
- `REVISION_REQUIRED`;
- `APPROVED`;
- `REFACTORING`; and
- `COMPLETE` for a module only after its final audit passes.

`DRAFT` means authored, not approved. `APPROVED` requires explicit human confirmation. A module becomes `COMPLETE` only when both lessons, its source-preserved exercise set, all one-to-one solutions, its interview artifact, and its final module audit are approved.

`REVISION_REQUIRED` means human review found blockers that must be corrected before approval. The artifact remains at its current gate, identified blockers must be recorded, and progression to the next major artifact is prohibited. Successful tool execution alone does not clear this status; explicit human approval is required after correction.

`REFACTORING` means a previously approved or complete artifact or module has entered an explicitly authorized revision cycle. Prior validation remains historical evidence and previous work is not erased, but the artifact or module must not continue to be represented as `COMPLETE` while its approved baseline is being revised. The new review cycle must pass every applicable gate before completion is restored.

Normal artifact progression is:

```text
NOT_STARTED
→ DRAFT
→ HUMAN_REVIEW_PENDING
→ REVISION_REQUIRED, when blockers exist
→ APPROVED
```

Reopened work follows:

```text
COMPLETE or APPROVED
→ REFACTORING
→ DRAFT / HUMAN_REVIEW_PENDING
→ APPROVED
→ COMPLETE after the final audit
```

Not every artifact must use every status mechanically; it uses the status appropriate to its actual lifecycle.

Approval of one artifact does not authorize unrelated work. Module progression never changes the source identity or numbering of an approved exercise.

## 11. Architecture Acceptance Checklist

The architecture is valid only when all of the following remain true:

- M01 through M10 are present in the canonical map.
- Each module maps exactly two canonical lesson Markdown files.
- The primary baseline is C99, and later-standard material is explicit and module-scoped.
- Module artifacts follow the canonical repository model without lesson-specific directories.
- Source scope and technical authority remain separate.
- Supplementary repository notes remain non-authoritative and require independent claim verification.
- Curriculum source files and reference PDFs remain immutable during normal authoring and refactoring.
- Artifact responsibilities do not overlap.
- Exercises preserve DevLinux identities and expose repository clarifications transparently.
- Every exercise maps to exactly one solution in the same order.
- Interview material remains Embedded C-centered, with only narrow C++ comparisons that clarify a C boundary.
- Lesson quality is judged by focused technical completeness rather than a numeric size target.
- Debugging and other verification workflows are included only when relevant to the lesson.
- Reopened approved work is marked `REFACTORING`, and review blockers are marked `REVISION_REQUIRED`.
- Tool-derived claims retain proportionate, reproducible context and remain scoped to the recorded environment.
- Every major gate requires explicit human approval.
- Source conflicts, unavailable source files, compiler dependencies, and target assumptions are recorded rather than hidden.

Changes to the module count, lesson count, module order, lesson mapping, or exercise identity require explicit human architecture approval before implementation.
