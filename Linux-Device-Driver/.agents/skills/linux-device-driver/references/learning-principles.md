# Learning Principles

## Core Principle
Understanding > Memorization.

A good Linux Device Driver lesson helps the user understand the system:

- What problem the subsystem solves.
- What objects the kernel uses to model it.
- Who owns each object and how long it lives.
- Which callback/API is called by whom and when.
- How userspace, kernel core, driver code, and hardware interact.
- What breaks in real work and how to debug it.

Completeness is not summary length. Completeness means the project output is both source-covered and understandable:

- source-covered: `ldd1`, `ldd2`, and `notion` sources in scope were read, mapped, compared, and recorded in `coverage/`;
- understandable: the learner-facing docs teach mental model, kernel mechanism, APIs, lifecycle, examples, bugs, debugging, production concerns, and interview reasoning.

## Explanation Ladder
Use this order:

1. Beginner mental model.
2. Why the topic exists and when to use it.
3. Kernel mechanism.
4. Important structs and APIs.
5. Data/lifecycle flow.
6. Minimal example.
7. Common bugs and debugging.
8. Production checklist.
9. Interview framing.
10. Short kernel-version notes when needed.

## Source Discipline
- Treat `ldd1`, `ldd2`, and `notion` as independent sources.
- Do not skip apparent duplicates until they are read and compared.
- Keep source chapter numbers as metadata only.
- Use learning-path numbering for final output.
- Keep audit metadata in `coverage/`, not in learner-facing `knowledge/` or `interview/`.
- Mark stale APIs and version-sensitive behavior instead of hiding uncertainty.

## Anti-Patterns
- Do not paste raw source docs as a summary.
- Do not produce API lists without mechanism.
- Do not optimize for memorized interview answers.
- Do not hide uncertainty; mark version-sensitive behavior or missing source evidence.
- Do not ask the user for kernel keywords when the docs can reveal them.
- Do not claim 100% coverage without a coverage matrix.
- Do not collapse same-number chapters from different source groups into one source identity.
- Do not put source coverage tables or merge notes at the top of learner-facing docs.
