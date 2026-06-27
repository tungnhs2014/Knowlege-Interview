# Coverage Gate

Use this gate for every Linux System Programming write, refactor, and review task.

## Principle
Refactor means preserve and improve coverage, not summarize shorter.

Polished prose is not enough. A doc is incomplete if it drops an important concept, API, mechanism, lifecycle rule, failure mode, debug workflow, Embedded constraint, or interview angle that is required by the learning map, mapped sources, or useful existing output.

## Coverage Matrix
Before writing, create a compact matrix with these columns:

| Item | Source | Required coverage | Target output | Status |
|------|--------|-------------------|---------------|--------|
| Mapped topic row or must-cover concept | Learning map, TLPI, DevLinux, existing output | Mechanism/API/lifecycle/failure/debug/interview | Knowledge/interview filename or section | Covered/Moved/Out of scope/Missing |

Use the matrix to decide what to preserve, merge, move, or add.

## Required Inputs
- `CODEX.md`.
- `LINUX_SYSTEM_LEARNING_MAP.md`, including the chapter `Must Cover` list.
- Mapped TLPI docs for correctness.
- Mapped DevLinux docs for practical workflow when present.
- Existing knowledge and interview outputs for the requested chapter/topic.

## Anti-Loss Rules
- Do not delete correct existing content unless equivalent coverage remains.
- If a concept moves, name the new target section or file.
- If a concept is out of scope, say why.
- If sources disagree or the behavior is version-sensitive, mark uncertainty and verify with man-pages or official docs when needed.
- For whole-chapter work, cover every mapped row and every target output file.

## Blockers
Treat these as review blockers:
- Any learning-map topic row is missing from output.
- Any chapter `Must Cover` item is missing.
- A mechanism is named but ownership, lifetime, or data/control flow is not explained.
- APIs are listed without their role in the mechanism.
- Failure modes or debugging commands are absent for production-relevant topics.
- Interview material does not test important concepts through scenario, comparison, or recognize-only coverage.
- Refactor drops useful existing content without replacement, move note, or out-of-scope note.

## Final Check
Before finalizing, answer:
- What sources were read?
- Which mapped rows are covered?
- Which must-cover concepts are covered?
- Which existing concepts were moved or merged?
- Are any gaps, uncertainty, or out-of-scope items clearly marked?
