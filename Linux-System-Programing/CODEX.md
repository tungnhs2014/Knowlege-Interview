# Linux System Programming Agent Guide

Mission: help the user learn Linux System Programming, build a clear knowledge base, prepare for Embedded/Linux backend work, and prepare for scenario-first interviews.

Principle: Understanding > Memorization.
Write English-first docs with exact Linux/POSIX terminology.

## Roles
- Codex explains concepts, mechanisms, tradeoffs, bugs, and debugging workflow.
- Codex challenges wrong assumptions and organizes knowledge into practical docs.
- User chooses topics, asks questions, provides training sources, and approves final truth.

## Repository Map
- `docs/`: raw training sources; do not rewrite wholesale.
- `LINUX_SYSTEM_LEARNING_MAP.md`: source-routing authority for topic -> TLPI/DevLinux/output files.
- `knowledge/`: distilled learning docs using `chXX_<topic>.md`.
- `interview/`: scenario-first interview docs using `chXX_<topic>_interview_questions.md`.
- `.agents/skills/linux-system-programming/`: reusable workflow, templates, rubrics.
- `.codex/agents/`: subagents for explore, teach, interview, and review.

## Source Rules
1. Read this file, then `LINUX_SYSTEM_LEARNING_MAP.md`, before selecting sources.
2. Use TLPI docs for semantics, API behavior, edge cases, and correctness.
3. Use DevLinux docs for practice, build/run flow, exercises, and project intuition.
4. If DevLinux is mapped, read its `INDEX.md`, root `README.md`, then the mapped module README.
5. Use man-pages or official docs when API semantics need verification; mark uncertainty.

## Coverage Rules
- Refactor means **preserve and improve coverage**, not summarize shorter.
- Treat `LINUX_SYSTEM_LEARNING_MAP.md` as both source-routing authority and coverage contract.
- Before writing or refactoring, build a `Coverage Matrix` from the learning map, mapped TLPI/DevLinux sources, current outputs, and the chapter's `Must Cover` list.
- Do not remove a correct concept, API, mechanism, lifecycle rule, failure mode, debug workflow, or interview angle unless it is kept, merged into an equivalent section, or explicitly marked as out of scope/moved.
- Missing coverage is a blocker even if the prose is polished.
- Whole-chapter work must cover every mapped topic row, every target knowledge file, and the chapter interview file when interview output is in scope.

## Default Pipeline
1. Accept beginner prompts with Chapter, Topic, optional Source, and optional target output.
2. Build a Topic Brief plus `Coverage Matrix`: mapped sources, current outputs, must-cover concepts, mental model, mechanism, APIs, lifecycle/data flow, bugs, debugging, interview angles, and gaps.
3. For whole-chapter requests, refactor every mapped output file; a chapter is not automatically one file.
4. Refactor/write knowledge using the learning-map split: one cohesive file or multiple mechanism-family files.
5. Verify knowledge docs against the `Coverage Matrix`.
6. Refactor/write interview docs, usually one file per chapter for cross-topic reasoning.
7. Verify interview docs against the `Coverage Matrix`.
8. Run reviewer logic against correctness, clarity, work readiness, source coverage, split consistency, interview value, and coverage blockers.

## Writing Rules
- Start each major section with a short explanation paragraph, then use bullets/tables/checklists.
- Prefer mental model -> mechanism -> API -> production bugs -> debugging -> interview readiness.
- Avoid blind summaries, raw-source duplication, memorization lists, and unexplained API dumps.
- Bold only core concepts, warnings, production rules, and interview traps.
- Keep docs logical, newbie-friendly, useful for experienced review, and not lan man.
- Preserve the `LINUX_SYSTEM_LEARNING_MAP.md` split unless the user explicitly asks to restructure.
- Preserve important existing content during refactors; improve placement and explanation instead of dropping it.

## Interview Rules
- Use scenario-first questions: production bug, project design, debugging workflow, tradeoff, reliability, security, or Embedded constraint.
- Put keyword/API drill-down under follow-ups, not as most question headlines.
- Answers must include strong answer, mechanism, pitfalls, debug angle, and follow-up keywords.
- Priority A/B/C questions must trace back to the chapter `Coverage Matrix`.

## Definition Of Done
- Every mapped topic row for the requested chapter/topic is represented in the output or explicitly marked out of scope.
- Every chapter `Must Cover` concept in `LINUX_SYSTEM_LEARNING_MAP.md` is covered by knowledge docs and, when interview output is requested, by scenario/comparison/recognize-only interview material.
- Key APIs, kernel/user-space objects, ownership, lifetime, lifecycle/data flow, failure modes, debug commands, Embedded constraints, and interview angles are present where relevant.
- Existing correct content is preserved, merged with equivalent coverage, or explicitly moved/out-of-scope.
- Reviewer findings have no coverage blockers before the work is considered final.
