# Linux Device Driver

English-first learning material for Embedded Linux device driver development,
organized for real work, code review, debugging, and interviews.

## How To Use This Project

1. Start with `CODEX.md` to understand the documentation workflow.
2. Use `LEARNING_PATH.md` to choose the canonical topic order and output slug.
3. Use `docs/` as source material and imported notes.
4. Read distilled lessons in `knowledge/`.
5. Practice interview answers in `interview/`.
6. Build and inspect minimal examples in `examples/`.

## Beginner Prompt

```txt
Use $linux-device-driver.

Topic: Character Device Drivers
Slug: character-device-drivers
Learning Path: 07
Source: relevant Chapter 4 docs

I am a beginner. Run the full learning pipeline.
Read and map ldd1, ldd2, and notion source docs in scope.
Create knowledge, interview, and example outputs.
Review and finalize.
```

## Naming Convention

Use learning-path slugs for new finished outputs:

- `07-character-device-drivers`
- `09-platform-bus-and-platform-drivers`
- `16-i2c-client-drivers`

For each topic, use matching paths:

- `knowledge/NN-<topic>.md`
- `interview/NN-<topic>.md`
- `examples/NN-<topic>/`

Raw source chapter numbers are tracked as metadata such as `ldd1-ch04`,
`ldd2-ch04`, and `notion-ch04-part1`. They are not the primary output naming
scheme.

See `LEARNING_PATH.md` for the full topic order, canonical slugs, and output
path examples.

## Source Coverage And Audit

All learning material is built from three independent source groups:

- `ldd1`: `docs/Linux Device Driver Development/`
- `ldd2`: `docs/Linux Device Driver Development 2/`
- `notion`: `docs/Linux-Device-Driver-Notion/`

Same-number chapters across these groups are not assumed to be the same content.
Notion files must be read and mapped even when they appear to overlap with book
chapters. Do not claim full coverage unless a topic or project coverage matrix
backs that claim.

Coverage and source-merge notes belong in `coverage/`, not at the top of
learner-facing `knowledge/` or `interview/` files.

## Documentation Shape

Each finished topic should cover:

- Why it matters in real work
- Core concept
- Kernel objects and APIs
- Lifecycle and data flow
- Minimal example
- Common bugs
- Debugging
- Production checklist
- Interview Q&A

## Priority Topics

Use `LEARNING_PATH.md` as the source of truth for topic order. Start with the
core foundation topics, then common embedded driver work, subsystem-specific
drivers, and finally debugging plus production/interview review.
