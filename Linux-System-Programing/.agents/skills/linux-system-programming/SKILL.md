---
name: linux-system-programming
description: Use when creating, refactoring, reviewing, or organizing Linux System Programming knowledge docs, scenario-first interview docs, examples, and learning roadmaps for Embedded Linux, backend, and system software work.
---

# Linux System Programming Skill

Use this skill for documentation under `Linux-System-Programing`.

## Goal
Build a clear Linux System Programming knowledge base for learning, real work, and interviews.
The core principle is Understanding > Memorization.

## Newbie Prompt Contract
The user may provide only:
- Chapter
- Topic
- Optional source hint
- Optional output type: knowledge, interview, example/project, or review

Do not require the user to know API keywords. Use `LINUX_SYSTEM_LEARNING_MAP.md` to discover source docs, existing outputs, and expected filenames.
If the user gives only a whole chapter, refactor all mapped topics and all existing output files for that chapter.
If a chapter maps to multiple knowledge files, preserve that split instead of merging unless the user asks.

## Default Full Pipeline
1. Discover: read the learning map, mapped source docs, and current outputs; create a Topic Brief.
2. Coverage Gate: create a Coverage Matrix from the learning map, chapter Must Cover list, source docs, and existing outputs.
3. Teach: refactor/write `knowledge/chXX_<topic>.md` while preserving the Coverage Matrix.
4. Interview: refactor/write `interview/chXX_<topic>_interview_questions.md` while tracing questions back to the Coverage Matrix.
5. Example/project: reference or improve DevLinux exercises/projects when useful.
6. Review: apply the review rubric and fail the work if coverage blockers remain.

## References
- First principles: read `references/learning-principles.md`.
- Source routing: read `references/source-selection.md`.
- Coverage gate: read `references/coverage-gate.md`.
- Knowledge docs: read `references/knowledge-template.md`.
- Interview docs: read `references/interview-template.md`.
- Review criteria: read `references/review-rubric.md`.

## Quality Rules
Prefer practical Linux behavior over generic OS theory.
Connect mental model -> mechanism -> API -> production bug -> debugging -> interview.
Use short paragraphs plus bullets, tables, checklists, and flow diagrams.
Avoid raw-source duplication, API dumps, and memorized interview slogans.
Mark source gaps, version-sensitive behavior, POSIX/Linux-specific behavior, and uncertainty.
Refactor means preserve and improve coverage, not summarize shorter.
Every write/refactor/review task must use a Coverage Matrix; missing must-cover concepts are blockers.
