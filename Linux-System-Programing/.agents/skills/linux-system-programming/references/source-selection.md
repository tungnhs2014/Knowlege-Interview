# Source Selection

`LINUX_SYSTEM_LEARNING_MAP.md` is the source-routing authority and coverage contract.

## Required Order
1. Read `CODEX.md`.
2. Read `LINUX_SYSTEM_LEARNING_MAP.md`.
3. Find the chapter/topic row.
4. Read the chapter `Must Cover` list and turn it into a Coverage Matrix.
5. Read mapped TLPI docs from `docs/Linux-Programming-Interface/`.
6. If DevLinux is mapped, read:
   - `docs/Linux-Programming-DevLinux/INDEX.md`
   - `docs/Linux-Programming-DevLinux/README.md`
   - mapped module `README.md`
   - exercise/project files only when useful for examples or workflow.
7. Read existing outputs in `knowledge/` and `interview/` before refactoring.

## Source Roles
- TLPI: correctness, semantics, API behavior, edge cases, design reasoning.
- DevLinux: practical intuition, build/run workflow, exercises, project examples.
- man-pages / official docs: verify exact API semantics when needed.
- Job/interview sources: calibrate interview priority only; never use as technical authority.

## Output Naming
- Knowledge: `knowledge/chXX_<topic>.md`
- Interview: `interview/chXX_<topic>_interview_questions.md`
- Preserve existing names from the learning map unless a rename is explicitly requested.
- Whole-chapter requests must include every mapped output file for that chapter.
- Whole-chapter requests must include every mapped topic row and chapter Must Cover concept in the Coverage Matrix.
- If several topic rows point to the same output file, update that file once using all relevant rows.
- If a chapter has several output files, keep the existing split and make cross-links/coverage consistent.

## Split Policy
- Whole chapter does not mean one knowledge file.
- Use one knowledge file when one mental model explains most of the chapter.
- Use multiple knowledge files when the chapter contains separate mechanism families with different APIs, lifecycles, bugs, and debugging workflow.
- Preserve the learning-map split by default.
- Do not merge existing knowledge files unless the user explicitly asks.
- If restructuring is requested, propose the new split first before renaming or merging files.
- Chapter 7 IPC remains split by family: overview, pipes/FIFOs, System V IPC, and POSIX IPC.
- Interview docs are usually one file per chapter because interviews test cross-topic tradeoffs and scenario reasoning.

## Source Gaps
If a mapped source is missing or does not cover the needed behavior:
- State what is missing.
- Use man-pages or official docs for verification if needed.
- Mark inference clearly.
