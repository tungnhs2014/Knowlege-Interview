# Review Rubric

Use this before finalizing any topic output.

Review learner-facing docs and audit docs separately.

## Audit Coverage
- `coverage/topic-briefs/NN-<topic>.md` lists source identities such as `ldd1-chNN`, `ldd2-chNN`, and `notion-chNN-partM`.
- Every source file in scope is read, mapped, covered, merged, or marked as a gap.
- Notion material is not skipped because it appears derived from book material.
- Matching chapter numbers across source groups are not treated as equivalent without evidence.
- Any `100% covered`, `complete`, or `fully checked` claim is backed by a coverage table or matrix.

## Learner-Facing Structure
- `knowledge/NN-<topic>.md` does not contain `Source Coverage`, `Merged Source Notes`, source ID tables, or long audit/gap metadata.
- `interview/NN-<topic>.md` does not contain source coverage tables.
- Audit metadata is kept in `coverage/`.
- New finished outputs use learning-path naming: `knowledge/NN-<topic>.md`, `interview/NN-<topic>.md`, `examples/NN-<topic>/`.

## Correctness
- Kernel terms, structs, APIs, callbacks, and lifecycle are accurate.
- Version-sensitive behavior or uncertainty is marked.
- Source docs support the important claims through the Topic Brief.
- External validation is used for stale, ambiguous, or version-sensitive APIs when needed.

## Understanding
- The lesson explains what, why, when, how, and what can go wrong.
- Concepts are ordered from mental model to mechanism to API.
- Newbie readers are not expected to know hidden keywords.
- Experienced readers get enough depth on ownership, lifetime, locking, ABI, error paths, debugging, and tradeoffs.
- Comparisons are included when they help memory or design judgment.

## Work Readiness
- Includes realistic error paths, cleanup, locking, lifetime, ownership, userspace ABI, and debugging when relevant.
- Production checklist is concrete enough for bring-up or code review.
- Examples are clearly marked learning-only or production-ready.
- Kernel-version assumptions are stated for examples and version-sensitive APIs.

## Interview Readiness
- Questions cover beginner, mid-level, and senior levels.
- Answers include short answer, deep explanation, API/code anchor, production/debug angle, traps, and follow-ups.
- The answer teaches reasoning, not memorized slogans.
- Questions are generated from the merged Topic Brief, not from one source only.

## Learning Path Coverage
- The learning path does not omit major source topics such as input drivers, V4L2 async/subdev/media-controller, ASoC machine drivers, or separate RTC/PWM versus watchdog/NVMEM concerns.
- Fail if a topic is so broad that lifecycle, APIs, debugging, and interview coverage become shallow.

Fail the review if learner docs are metadata-heavy, source trace is missing, Notion is skipped, chapter-number collision is present, stale APIs are hidden, or the new doc is weaker than a legacy high-quality baseline.
