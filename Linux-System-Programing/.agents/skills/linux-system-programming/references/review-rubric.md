# Review Rubric

Use this before finalizing any Linux System Programming output.

## Correctness
- Linux/POSIX terms, APIs, return values, errno behavior, and lifecycle are accurate.
- TLPI/man-page semantics are respected.
- Linux-specific behavior is not presented as portable POSIX unless true.
- Version-sensitive or uncertain behavior is marked.

## Source Coverage
- `LINUX_SYSTEM_LEARNING_MAP.md` was used.
- The chapter `Must Cover` list was used as a coverage contract.
- A Coverage Matrix exists or can be reconstructed from the output.
- Mapped TLPI docs were read for correctness.
- Mapped DevLinux docs were read for practical workflow when present.
- Existing knowledge/interview outputs were preserved where useful.
- Correct existing content was preserved, merged with equivalent coverage, moved with a clear note, or explicitly marked out of scope.

## Understanding
- Mental model appears before API details.
- Mechanism explains state ownership, lifetime, and data/control flow.
- Newbie readers are not expected to know hidden keywords.
- Every must-cover concept is explained at mechanism level unless it is explicitly recognize-only.

## Work Readiness
- Includes production bugs, debugging commands, failure modes, and review checklist.
- Covers Embedded constraints when relevant: resource limits, power loss, flash wear, daemon behavior, cross-compilation, driver/user-space interaction.

## Interview Readiness
- Interview docs are scenario-first.
- Answers include strong answer, mechanism, pitfalls, debug angle, and follow-up keywords.
- Priority A/B/C is meaningful and not a long keyword dump.
- Priority A/B/C traces to the Coverage Matrix and chapter coverage contract.

## Clarity
- No lan man sections.
- No wall-of-text sections.
- No raw source duplication.
- No unexplained API dumps.
- Bold is used sparingly for key ideas, warnings, and traps.

## Coverage Blockers
- A learning-map topic row is absent from the requested output.
- A chapter Must Cover concept is absent from knowledge or interview coverage.
- A refactor removes useful existing content without equivalent replacement, move note, or out-of-scope note.
- Production-relevant docs lack failure modes or debugging evidence.
- Interview docs miss scenario/comparison/recognize-only coverage for important concepts.
