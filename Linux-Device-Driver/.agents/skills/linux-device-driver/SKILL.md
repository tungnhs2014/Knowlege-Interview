---
name: linux-device-driver
description: Use when creating, rewriting, reviewing, or organizing Linux device driver learning docs, knowledge base notes, examples, debugging guides, and interview material for beginner-to-developer learning.
---

# Linux Device Driver Skill

Use this skill for documentation under `Linux-Device-Driver`.

## Goal
Build a clear Linux Device Driver knowledge base for learning, real work, and interviews.
The core principle is Understanding > Memorization.

Completeness means both:

- source coverage: every source in scope is read, mapped, merged, or marked as a gap in `coverage/`;
- understanding: learner-facing docs explain concept, mechanism, APIs, lifecycle, bugs, debugging, examples, and interview reasoning.

## Source Groups
Treat these as independent sources:

- `ldd1`: `docs/Linux Device Driver Development/`
- `ldd2`: `docs/Linux Device Driver Development 2/`
- `notion`: `docs/Linux-Device-Driver-Notion/`

Do not skip a source because the chapter number or title looks similar. Notion files may repeat book material but still must be read and compared because they may contain better explanations, examples, or extra details.

## Source Identity Labels
Use these labels in Topic Briefs, coverage tables, and final source sections:

- `ldd1-chNN`
- `ldd2-chNN`
- `notion-chNN-partM`
- `notion-chNN-extra`

Always keep the original path with the label. Same chapter number across different groups does not mean same content.

## Newbie Prompt Contract
The user may provide only:

- Topic
- Slug
- Learning-path number or topic hint
- Optional source docs or chapter hint

Do not require the user to know kernel keywords. Discover concepts, APIs, lifecycle, bugs, examples, and interview angles from the source docs.

## Output Naming
Use learning-path numbering for new finished output:

- `knowledge/NN-<topic>.md`
- `interview/NN-<topic>.md`
- `examples/NN-<topic>/README.md`

Examples:

- `knowledge/07-character-device-drivers.md`
- `interview/07-character-device-drivers.md`
- `examples/07-character-device-drivers/README.md`

Existing `chapter-XX-<slug>` files are legacy material unless explicitly migrated.

Load `LEARNING_PATH.md` before choosing or validating a learning-path number,
topic slug, or canonical output path.

## Default Full Pipeline
1. Inventory: list all source files in scope across `ldd1`, `ldd2`, and `notion`.
2. Source mapping: load `LEARNING_PATH.md`, assign labels, extract headings, and map files to learning-path topics.
3. Coverage: record read/mapped/covered/merged/gap status for every source in scope.
4. Topic Brief: create `coverage/topic-briefs/NN-<topic>.md` with source-grounded facts, source differences, gaps, external validations, and target output paths.
5. Teach: create learner-facing `knowledge/NN-<topic>.md` from the Topic Brief without pasting audit metadata.
6. Interview: create learner-facing `interview/NN-<topic>.md` from the same merged facts without source coverage tables.
7. Example: create or update `examples/NN-<topic>/README.md` when code, DTS, commands, or debug workflow would help.
8. Review: apply the review rubric before finalizing.

## Merge Policy
- Read all overlapping sources before merging.
- Preserve unique details, examples, warnings, and debugging clues from each source.
- Record what is shared, what differs, and what was chosen for the final lesson.
- Do not mark apparent duplicates as covered until their content has been inspected.
- Do not claim 100% coverage unless the coverage table proves every source file in scope was handled.

## External Source Policy
- Use internal docs first.
- Use external sources only when internal docs are incomplete, stale, ambiguous, or version-sensitive.
- Prefer `docs.kernel.org`, kernel source documentation, and kernel.org release information.
- Record external URLs and the exact purpose of the validation.
- Mark kernel-version caveats explicitly.

## References
- First principles: read `references/learning-principles.md`.
- Topic lessons: read `references/topic-template.md`.
- Topic briefs and coverage: read `references/topic-brief-template.md`.
- Review criteria: read `references/review-rubric.md`.
- For interviews: read `references/interview-template.md`.
- For examples: read `references/code-example-template.md`.

## Quality Rules
Prefer practical driver behavior over generic OS theory.
Connect concept -> mechanism -> API -> example -> bug/debug -> interview.
Avoid blind summaries, memorization lists, and unexplained API dumps.
Do not put `Source Coverage`, `Merged Source Notes`, source ID tables, or long audit/gap metadata in learner-facing `knowledge/` or `interview/` files.
For knowledge docs, use short explanation paragraphs followed by bullets, tables, checklists, or flows.
Bold important concepts, warnings, production rules, and interview traps only.
Always mention important structs, APIs, lifecycle, error paths, locking, lifetime, ownership, userspace ABI, debugging, and version caveats when relevant.
Mark simplified learning code clearly when it is not production-ready.
Fail the output if it lacks source trace in `coverage/`, skips Notion, collides on raw chapter numbering, claims completion without coverage proof, or makes learner docs metadata-heavy.
