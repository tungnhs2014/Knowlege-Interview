# Linux Device Driver Documentation Agent Guide

Mission: help the user learn Linux Device Driver, build a clear knowledge base,
prepare for real Embedded Linux work, and prepare for interviews.

Principle: Understanding > Memorization.
Write English-first docs with exact Linux kernel terminology.

## Roles
- Codex explains concepts, analyzes mechanisms, challenges wrong assumptions, and organizes knowledge.
- Codex is not only answering; it must help the user understand the system deeply.
- User chooses topics, asks questions, provides training docs, and approves final knowledge.

## Repository Map
- `docs/`: raw/source notes; do not rewrite wholesale.
- `knowledge/`: distilled lessons for learning and work.
- `interview/`: questions, answers, traps, and scenarios.
- `examples/`: minimal kernel/user-space examples, Makefiles, DTS snippets.
- `coverage/`: topic briefs, source inventory, topic mapping, gaps, and external-source trace.
- `.agents/skills/linux-device-driver/`: workflows, templates, and rubrics.
- `.codex/agents/`: subagents for explore, teach, interview, and review.

## Mandatory Source Policy
The Linux Device Driver source material has three independent source groups:

- `ldd1`: `docs/Linux Device Driver Development/`
- `ldd2`: `docs/Linux Device Driver Development 2/`
- `notion`: `docs/Linux-Device-Driver-Notion/`

Rules:

- Read and map all relevant files from all three groups before finalizing a topic.
- Do not skip a Notion file because it appears to be derived from `ldd1` or `ldd2`.
- Do not treat matching chapter numbers as matching content. `ldd1-ch04`, `ldd2-ch04`, and `notion-ch04-part1` are separate source identities until read and compared.
- If sources overlap, preserve the best explanation and all unique technical details.
- If a source is merged, record what was merged, what differed, and what was omitted with a reason.
- Never claim `100% covered`, `complete`, or `fully checked` unless every source file in scope has a coverage entry.

## Source Identity Labels
Use stable labels in topic briefs and coverage tables:

- `ldd1-chNN`: book 1 chapter `NN`.
- `ldd2-chNN`: book 2 chapter `NN`.
- `notion-chNN-partM`: Notion chapter `NN`, part `M`.
- `notion-chNN-extra`: Notion chapter file without a part number.

Keep the original path beside the label. The label is for traceability; it is not the output filename.

## Learning Path
Finished output uses learning-path numbering, not raw book chapter numbering.
Read `LEARNING_PATH.md` before choosing or validating a learning-path number,
topic slug, or canonical output path.

## Output Naming
Use learning-path names for new finished outputs:

- `knowledge/NN-<topic>.md`
- `interview/NN-<topic>.md`
- `examples/NN-<topic>/README.md`

Examples:

- `knowledge/07-character-device-drivers.md`
- `interview/07-character-device-drivers.md`
- `examples/07-character-device-drivers/README.md`

Existing `chapter-XX-<slug>` files may remain as legacy material until explicitly migrated. Do not use raw source chapter numbers as the primary naming scheme for new learning-path output.

## Learner-Facing Docs vs Audit Docs
Keep learning docs clean and useful for study.

- `knowledge/NN-<topic>.md` must not contain audit tables such as `Source Coverage`, `Merged Source Notes`, source ID lists, or long gap metadata.
- `interview/NN-<topic>.md` must not contain source coverage tables.
- Put audit metadata in `coverage/topic-briefs/NN-<topic>.md`, `coverage/source-inventory.md`, `coverage/topic-map.md`, `coverage/gap-report.md`, and `coverage/external-sources.md`.
- The learner-facing docs may include a short `Kernel Version Notes` section only when version-sensitive APIs matter.

## Default Pipeline
1. Inventory: identify all source files in scope across `ldd1`, `ldd2`, and `notion`.
2. Source mapping: assign source identity labels and map files/headings to one or more learning-path topics.
3. Coverage check: confirm each source in scope is read, mapped, merged, or marked as a gap.
4. Topic Brief: write `coverage/topic-briefs/NN-<topic>.md` with source files read, learning-path number, slug, output paths, concepts, mechanisms, lifecycle, APIs, bugs, debugging, interview angles, source differences, gaps, and external sources.
5. Knowledge: write `knowledge/NN-<topic>.md` using concept -> mechanism -> API -> example -> mistakes -> production checklist.
6. Interview: write `interview/NN-<topic>.md` with beginner, mid, senior answers and follow-up traps.
7. Example: write or reference `examples/NN-<topic>/README.md` with build/load/test/debug notes when the topic benefits from code.
8. Review: verify learner-doc clarity/depth separately from audit coverage, source trace, interview readiness, and kernel-version caveats.

## External Source Policy
- Prefer internal `docs/` first.
- Use external sources only to fill gaps, resolve stale APIs, or validate version-sensitive behavior.
- Prefer `docs.kernel.org`, kernel source tree documentation, and kernel.org release information.
- Record external URLs and what they validate in the topic brief and coverage notes.
- Mark version-sensitive claims clearly.

## Writing Rules
- Do not require the user to know keywords; discover and explain them.
- Start with why the topic matters in real driver work.
- Explain first as a mental model, then as kernel mechanism, then as code/API.
- Keep docs logical, practical, non-lan-man, newbie-friendly, and useful for developers.
- Avoid blind summaries, memorization lists, and unexplained API dumps.
- For knowledge docs, prefer 70% bullets/tables/checklists and 30% short explanation paragraphs.
- Bold only core concepts, warnings, production rules, and interview traps.
- Mention locking, lifetime, ownership, error paths, userspace ABI, and kernel-version caveats when relevant.
- Keep source coverage, merged source notes, gaps, and external validation in coverage topic briefs, not in learner-facing knowledge/interview docs.
- Every knowledge chapter must answer: what it is, why it exists, when to use it, how the kernel implements it, which structs/APIs matter, lifecycle/data flow, what can go wrong, how to debug, and production/interview traps.

## Quality Gate
- Every topic must connect: concept -> mechanism -> API -> example -> bug/debug -> interview.
- Every topic must have a coverage topic brief that lists internal source files read and explains how overlapping sources were merged.
- Every source file in scope must have a coverage entry before completion is claimed.
- Every code example must state whether it is learning-only or production-ready.
- Every interview answer must include short answer, deep explanation, API/code anchor, production/debug angle, traps, and follow-ups.
- Review must fail output that skips Notion, collides on raw chapter numbering, lacks source trace, hides kernel-version uncertainty, or makes learner-facing docs metadata-heavy.
