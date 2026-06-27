# Linux System Programming

English-first learning material for Linux System Programming, organized for
Embedded Linux, backend/system software work, production debugging, and
scenario-first interviews.

## How To Use This Project

1. Start Codex from this directory.
2. Codex reads `AGENTS.md`, then follows `CODEX.md`.
3. Use `$linux-system-programming` for refactoring, writing, and reviewing docs.
4. Use `LINUX_SYSTEM_LEARNING_MAP.md` to route topics to TLPI and DevLinux sources and to enforce the chapter coverage contract.
5. Every refactor must build a Coverage Matrix before writing and pass coverage review before being treated as final.
6. Keep `prompt.md` as legacy reference only.

## Beginner Prompt

```txt
Use $linux-system-programming.

Chapter: 07
Topic: IPC

I am a beginner. Refactor the knowledge and interview docs using the full pipeline.
Build the Coverage Matrix first and do not finalize while coverage blockers remain.
```

## Generic Refactor Prompt

```txt
Use $linux-system-programming.

Chapter: 06
Topic: Threads

Refactor the full chapter.
First build the Coverage Matrix from LINUX_SYSTEM_LEARNING_MAP.md, mapped sources, current outputs, and the chapter Must Cover list.
Do not finalize while mapped rows, Must Cover concepts, useful existing content, debugging coverage, or interview coverage are missing.
```

## Step-By-Step Pipeline

Explorer:

```txt
Use $linux-system-programming.
Spawn lsp-topic-explorer.

Task:
Analyze Chapter 07 IPC using the learning map.
Return a Topic Brief and Coverage Matrix only.
Do not edit files.
```

Teacher:

```txt
Use $linux-system-programming.
Spawn lsp-teacher.

Task:
Use the Topic Brief to refactor the Chapter 07 knowledge docs.
Preserve the Coverage Matrix and report any remaining coverage gaps.
```

Interviewer:

```txt
Use $linux-system-programming.
Spawn lsp-interviewer.

Task:
Use the Topic Brief and Chapter 07 knowledge docs to refactor the Chapter 07 interview doc.
Trace Priority A/B/C questions back to the Coverage Matrix.
```

Reviewer:

```txt
Use $linux-system-programming.
Spawn lsp-code-reviewer.

Task:
Review the Chapter 07 knowledge and interview outputs.
Treat missing mapped rows, missing Must Cover concepts, and dropped useful existing content as blockers.
Return findings only. Do not edit files.
```

## Output Naming

- Knowledge: `knowledge/chXX_<topic>.md`
- Interview: `interview/chXX_<topic>_interview_questions.md`
