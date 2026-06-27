# C/C++ Learning Path

This file defines the canonical output order and source routing for C/C++
learning material.

The numbering below is learning-path numbering, not Notion chapter numbering and
not raw master-index chapter numbering. Raw source IDs remain metadata such as
`master-ch04` or `notion-3-2`.

Use these paths for finished output:

- `knowledge/NN-<slug>.md`
- `interview/NN-<slug>.md`
- `examples/NN-<slug>/README.md`
- `coverage/topic-briefs/NN-<slug>.md`

Audit metadata belongs in `coverage/topic-briefs/NN-<slug>.md`, not in
learner-facing `knowledge/` or `interview/` files.

## Source Responsibilities

- `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`: priority, dependencies, keywords,
  required comparisons, interview focus, and final learning order.
- `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`: output templates, depth control,
  lesson types, style rules, and quality checklist.
- `docs/C++ Notion/C++ Programming.md`: Notion index and source inventory.
- `docs/C++ Notion/Chapter *.md`: primary detailed C++ source material.
- External trusted references: used only for gaps, exact language/library
  behavior, C-only topics, safety rules, or POSIX/Linux user-space API
  comparisons.

## Canonical Chapters

| No. | Title | Slug | Master Source | Required Source Routing | Output Paths |
| --- | --- | --- | --- | --- | --- |
| 01 | Build And Compilation Model | `build-and-compilation-model` | `master-ch01` | Notion `1-1`, `2-3`, `2-4`, `10-2`; external compiler/linker docs if needed. | `knowledge/01-build-and-compilation-model.md`, `interview/01-build-and-compilation-model.md`, `examples/01-build-and-compilation-model/README.md`, `coverage/topic-briefs/01-build-and-compilation-model.md` |
| 02 | C Fundamentals | `c-fundamentals` | `master-ch02` | Notion `1-2`, `1-3`, `1-4`; external ISO C/cppreference C for C-specific behavior. | `knowledge/02-c-fundamentals.md`, `interview/02-c-fundamentals.md`, `examples/02-c-fundamentals/README.md`, `coverage/topic-briefs/02-c-fundamentals.md` |
| 03 | C Memory Model | `c-memory-model` | `master-ch03` | Notion `4-1`, `4-2`, `3-2`; external ISO C, cppreference C, and SEI CERT for C memory rules. | `knowledge/03-c-memory-model.md`, `interview/03-c-memory-model.md`, `examples/03-c-memory-model/README.md`, `coverage/topic-briefs/03-c-memory-model.md` |
| 04 | Pointer Mastery | `pointer-mastery` | `master-ch04` | Notion `3-2`, `3-3`, `2-2`, `4-1`, `4-2`; external ISO C/C++ or SEI CERT for undefined behavior details. | `knowledge/04-pointer-mastery.md`, `interview/04-pointer-mastery.md`, `examples/04-pointer-mastery/README.md`, `coverage/topic-briefs/04-pointer-mastery.md` |
| 05 | Compound Types In C | `compound-types-in-c` | `master-ch05` | Notion `3-1`, `3-4`, `3-5`, `3-6`; external ISO C/cppreference C for C-specific struct/union/enum rules. | `knowledge/05-compound-types-in-c.md`, `interview/05-compound-types-in-c.md`, `examples/05-compound-types-in-c/README.md`, `coverage/topic-briefs/05-compound-types-in-c.md` |
| 06 | Advanced C For Embedded | `advanced-c-for-embedded` | `master-ch06` | Notion `10-2`, `10-5`, `10-7`; external MISRA, BARR-C, SEI CERT, and compiler docs. | `knowledge/06-advanced-c-for-embedded.md`, `interview/06-advanced-c-for-embedded.md`, `examples/06-advanced-c-for-embedded/README.md`, `coverage/topic-briefs/06-advanced-c-for-embedded.md` |
| 07 | Industrial C Practices | `industrial-c-practices` | `master-ch07` | Notion `4-2`, `10-2`; external SEI CERT, MISRA, BARR-C, compiler warnings, sanitizers, and static-analysis docs. | `knowledge/07-industrial-c-practices.md`, `interview/07-industrial-c-practices.md`, `examples/07-industrial-c-practices/README.md`, `coverage/topic-briefs/07-industrial-c-practices.md` |
| 08 | C++ Fundamentals | `cpp-fundamentals` | `master-ch08` | Notion `1-1`, `1-2`, `1-3`, `1-4`, `2-1`, `2-2`, `2-3`, `2-4`, `2-5`, `2-6`, `10-1`, `10-10`; cppreference C++ if needed. | `knowledge/08-cpp-fundamentals.md`, `interview/08-cpp-fundamentals.md`, `examples/08-cpp-fundamentals/README.md`, `coverage/topic-briefs/08-cpp-fundamentals.md` |
| 09 | OOP In C++ | `oop-in-cpp` | `master-ch09` | Notion `5-1`, `5-2`, `5-3`, `5-4`, `5-5`; C++ Core Guidelines for design guidance. | `knowledge/09-oop-in-cpp.md`, `interview/09-oop-in-cpp.md`, `examples/09-oop-in-cpp/README.md`, `coverage/topic-briefs/09-oop-in-cpp.md` |
| 10 | Resource Management In C++ | `resource-management-in-cpp` | `master-ch10` | Notion `4-1`, `4-2`, `8-2`, `8-3`, `10-4`, `10-6`; C++ Core Guidelines for RAII/ownership. | `knowledge/10-resource-management-in-cpp.md`, `interview/10-resource-management-in-cpp.md`, `examples/10-resource-management-in-cpp/README.md`, `coverage/topic-briefs/10-resource-management-in-cpp.md` |
| 11 | STL And Standard Library | `stl-and-standard-library` | `master-ch11` | Notion `6-1`, `6-2`, `6-3`, `6-4`, `6-5`, `6-6`, `9-1`; cppreference for exact container/library behavior. | `knowledge/11-stl-and-standard-library.md`, `interview/11-stl-and-standard-library.md`, `examples/11-stl-and-standard-library/README.md`, `coverage/topic-briefs/11-stl-and-standard-library.md` |
| 12 | Modern C++ And Templates | `modern-cpp-and-templates` | `master-ch12`, `master-ch13` | Notion `2-6`, `7-1`, `7-2`, `7-3`, `7-4`, `10-4`, `10-6`, `10-10`; cppreference for C++ standard-version and template behavior. | `knowledge/12-modern-cpp-and-templates.md`, `interview/12-modern-cpp-and-templates.md`, `examples/12-modern-cpp-and-templates/README.md`, `coverage/topic-briefs/12-modern-cpp-and-templates.md` |
| 13 | Error Handling | `error-handling` | `master-ch14` | Notion `8-1`, `8-2`, `8-3`, `9-1`; C++ Core Guidelines and cppreference for exception behavior. | `knowledge/13-error-handling.md`, `interview/13-error-handling.md`, `examples/13-error-handling/README.md`, `coverage/topic-briefs/13-error-handling.md` |
| 14 | Concurrency | `concurrency` | `master-ch15` | Notion `10-8`, `10-9`, `10-7`; cppreference concurrency and POSIX man pages for comparisons. | `knowledge/14-concurrency.md`, `interview/14-concurrency.md`, `examples/14-concurrency/README.md`, `coverage/topic-briefs/14-concurrency.md` |
| 15 | C Vs C++ Comparison | `c-vs-cpp-comparison` | `master-ch17` | Related Notion files by topic; external ISO C/C++, cppreference C/C++, and C++ Core Guidelines. | `knowledge/15-c-vs-cpp-comparison.md`, `interview/15-c-vs-cpp-comparison.md`, `examples/15-c-vs-cpp-comparison/README.md`, `coverage/topic-briefs/15-c-vs-cpp-comparison.md` |
| 16 | POSIX/Linux C API Vs Modern C++ | `posix-linux-c-api-vs-modern-cpp` | `master-ch18` | Notion `9-1`, `10-7`, `10-8`, `10-9`; POSIX/Linux man pages; do not include kernel-driver material. | `knowledge/16-posix-linux-c-api-vs-modern-cpp.md`, `interview/16-posix-linux-c-api-vs-modern-cpp.md`, `examples/16-posix-linux-c-api-vs-modern-cpp/README.md`, `coverage/topic-briefs/16-posix-linux-c-api-vs-modern-cpp.md` |
| 17 | Design Principles And Design Patterns By Priority | `design-principles-and-patterns-by-priority` | `master-ch16` | Notion `5-1` to `5-5`, `6-1` to `6-6`, `7-1` to `7-4`, `10-5`; Refactoring Guru only for design pattern reference. | `knowledge/17-design-principles-and-patterns-by-priority.md`, `interview/17-design-principles-and-patterns-by-priority.md`, `examples/17-design-principles-and-patterns-by-priority/README.md`, `coverage/topic-briefs/17-design-principles-and-patterns-by-priority.md` |
| 18 | Enterprise And Interview Checklist | `enterprise-and-interview-checklist` | `master-ch19` | All relevant Notion best-practice, common-pitfall, summary, and interview sections; external guidelines for review checklists. | `knowledge/18-enterprise-and-interview-checklist.md`, `interview/18-enterprise-and-interview-checklist.md`, `examples/18-enterprise-and-interview-checklist/README.md`, `coverage/topic-briefs/18-enterprise-and-interview-checklist.md` |

## Notion Source Inventory

`docs/C++ Notion/` contains one index plus 48 chapter files. Every chapter file
below is routed into at least one learning-path topic.

| Source ID | Path | Routed Topics |
| --- | --- | --- |
| `notion-index` | `docs/C++ Notion/C++ Programming.md` | Source inventory for all topics |
| `notion-1-1` | `docs/C++ Notion/Chapter 1-1 Introduction & Environment Setup.md` | 01, 08 |
| `notion-1-2` | `docs/C++ Notion/Chapter 1-2 Variables, Data Types, Storage & Scope.md` | 02, 08 |
| `notion-1-3` | `docs/C++ Notion/Chapter 1-3 Type Conversion & Casting.md` | 02, 08, 15 |
| `notion-1-4` | `docs/C++ Notion/Chapter 1-4 Operators, Input Output, Control Flow & Loops.md` | 02, 08 |
| `notion-2-1` | `docs/C++ Notion/Chapter 2-1 Function Basics.md` | 08 |
| `notion-2-2` | `docs/C++ Notion/Chapter 2-2 Parameter Passing Techniques.md` | 04, 08, 15 |
| `notion-2-3` | `docs/C++ Notion/Chapter 2-3 Function Overloading & Name Mangling.md` | 01, 08 |
| `notion-2-4` | `docs/C++ Notion/Chapter 2-4 Inline Functions.md` | 01, 08, 15 |
| `notion-2-5` | `docs/C++ Notion/Chapter 2-5 Recursion.md` | 08 |
| `notion-2-6` | `docs/C++ Notion/Chapter 2-6 Lambda Expressions (C++11).md` | 08, 12, 15, 17 |
| `notion-3-1` | `docs/C++ Notion/Chapter 3-1 Arrays in C++.md` | 05, 15 |
| `notion-3-2` | `docs/C++ Notion/Chapter 3-2 Pointers in C++.md` | 03, 04, 15 |
| `notion-3-3` | `docs/C++ Notion/Chapter 3-3 References in C+.md` | 04, 08, 15 |
| `notion-3-4` | `docs/C++ Notion/Chapter 3-4 Strings in C++.md` | 05, 11, 15 |
| `notion-3-5` | `docs/C++ Notion/Chapter 3-5 Structures in C++.md` | 05, 09, 15 |
| `notion-3-6` | `docs/C++ Notion/Chapter 3-6 Unions, Enumerations, and Type Aliases in C+.md` | 05, 12, 15 |
| `notion-4-1` | `docs/C++ Notion/Chapter 4-1 Dynamic Memory Basics.md` | 03, 04, 10, 15 |
| `notion-4-2` | `docs/C++ Notion/Chapter 4-2 Advanced Memory Management.md` | 03, 04, 07, 10, 18 |
| `notion-5-1` | `docs/C++ Notion/Chapter 5-1 Classes, Objects & Constructors.md` | 09, 17 |
| `notion-5-2` | `docs/C++ Notion/Chapter 5-2 Static Members & Friend Functions.md` | 09, 17 |
| `notion-5-3` | `docs/C++ Notion/Chapter 5-3 Abstraction & Abstract Classes.md` | 09, 17 |
| `notion-5-4` | `docs/C++ Notion/Chapter 5-4 Inheritance & Polymorphism.md` | 09, 17 |
| `notion-5-5` | `docs/C++ Notion/Chapter 5-5 Operator Overloading.md` | 09, 17 |
| `notion-6-1` | `docs/C++ Notion/Chapter 6-1 STL Introduction & vector Container.md` | 11, 15, 17 |
| `notion-6-2` | `docs/C++ Notion/Chapter 6-2 Sequence Containers deque, list, forward_list, array.md` | 11, 15, 17 |
| `notion-6-3` | `docs/C++ Notion/Chapter 6-3 Container Adapters & Associative Containers.md` | 11, 17 |
| `notion-6-4` | `docs/C++ Notion/Chapter 6-4 Unordered Associative Containers.md` | 11, 17 |
| `notion-6-5` | `docs/C++ Notion/Chapter 6-5 Iterators - The Bridge Between Containers and Algorithms.md` | 11, 17 |
| `notion-6-6` | `docs/C++ Notion/Chapter 6-6 STL Algorithms & Functors.md` | 11, 12, 17 |
| `notion-7-1` | `docs/C++ Notion/Chapter 7-1 Templates - Function Templates & Class Template.md` | 12, 17 |
| `notion-7-2` | `docs/C++ Notion/Chapter 7-2 Templates - Variadic Templates & SFINAE.md` | 12, 17 |
| `notion-7-3` | `docs/C++ Notion/Chapter 7-3 Templates - Type Traits, Concepts & Metaprogramming.md` | 12, 17 |
| `notion-7-4` | `docs/C++ Notion/Chapter 7-4 Templates - Template Template Parameters & Advanced Topics.md` | 12, 17 |
| `notion-8-1` | `docs/C++ Notion/Chapter 8-1 Exception Handling - Basics & Standard Exception.md` | 13, 15 |
| `notion-8-2` | `docs/C++ Notion/Chapter 8-2 Exception Handling - Exception Safety & RAII.md` | 10, 13 |
| `notion-8-3` | `docs/C++ Notion/Chapter 8-3 Exception Handling - noexcept, Stack Unwinding.md` | 10, 13 |
| `notion-9-1` | `docs/C++ Notion/Chapter 9-1 File Handling - Basics to Advanced Operations.md` | 11, 13, 16 |
| `notion-9-2` | `docs/C++ Notion/Chapter 9-2 File Handling - Interview Questions.md` | 11, 13, 18 |
| `notion-10-1` | `docs/C++ Notion/Chapter 10-1 Namespaces.md` | 08, 12 |
| `notion-10-2` | `docs/C++ Notion/Chapter 10-2 Preprocessor Directives.md` | 01, 06, 07, 15 |
| `notion-10-3` | `docs/C++ Notion/Chapter 10-3 Type Casting.md` | 08, 12, 15 |
| `notion-10-4` | `docs/C++ Notion/Chapter 10-4 Smart Pointers.md` | 10, 12, 15 |
| `notion-10-5` | `docs/C++ Notion/Chapter 10-5 Callbacks.md` | 06, 12, 15, 17 |
| `notion-10-6` | `docs/C++ Notion/Chapter 10-6 Move Semantics.md` | 10, 12 |
| `notion-10-7` | `docs/C++ Notion/Chapter 10-7 Signal Handling.md` | 06, 14, 16 |
| `notion-10-8` | `docs/C++ Notion/Chapter 10-8 Multithreading Basics.md` | 14, 16 |
| `notion-10-9` | `docs/C++ Notion/Chapter 10-9 Multithreading Advanced.md` | 14, 16 |
| `notion-10-10` | `docs/C++ Notion/Chapter 10-10 Modern C++ Features.md` | 08, 12 |

## Coverage Rules

- A topic is not final until its topic brief lists every mapped internal source
  read and any external reference used.
- If a mapped source is not read, the topic brief must mark the output as a draft
  and explain the gap.
- If the requested keyword spans multiple rows, choose the primary row and list
  related rows in the topic brief. Template-specific requests route primarily to
  topic 12, with topic 17 as a related design-pattern/principle row when
  applicable.
- If a new topic does not fit any row, mark it as a gap instead of inventing a
  new number without updating this file.
- Never claim `100% covered`, `complete`, or `fully checked` unless source
  inventory and topic brief coverage prove the claim.

## Agent Operating Rules

Agent operating rules are not learner-facing C/C++ chapters. They live in:

- `CODEX.md`
- `AGENTS.md`
- `.agents/skills/c-cpp/SKILL.md`
- `.codex/agents/*.toml`
- `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md` section `master-ch20`
- `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`

Do not create learner-facing output paths such as
`knowledge/19-ai-codex-agent-rules.md`, `interview/19-ai-codex-agent-rules.md`,
or `examples/19-ai-codex-agent-rules/README.md`. `master-ch20` is operational
guidance for agents, not a canonical learning-path topic.

## Dry-Run Acceptance Scenarios

- Pointer mastery must read Notion `3-2`, `3-3`, `2-2`, `4-1`, `4-2`, plus
  `master-ch04`.
- RAII and smart pointers must read Notion `8-2`, `8-3`, `10-4`, `10-6`, plus
  `master-ch10`.
- `std::thread` vs `pthread` must read Notion `10-8`, `10-9`, `master-ch18`,
  and POSIX references; it must not use Linux kernel-driver material.
