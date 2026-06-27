---
name: c-cpp
description: Use when creating, rewriting, reviewing, or organizing C/C++ learning docs, knowledge base notes, compile-oriented examples, comparisons, code review guides, and interview material for C/C++ software, embedded, Linux/system user-space, and enterprise engineering.
---

# C/C++ Skill

Use this skill for documentation under `C++ Programing`.

## Goal

Build a clear C/C++ knowledge base for learning, real work, code review, and
interviews. The core principle is Understanding > Memorization.

Completeness means both:

- source coverage: every mapped source in scope is read, merged, or marked as a
  gap in `coverage/`;
- understanding: learner-facing docs explain concept, mechanism, C/C++ API/code,
  practical usage, bugs, debugging, best practices, and interview reasoning.

## Required Sources

Always route work through these files:

- `LEARNING_PATH.md`: canonical topic number, slug, output paths, and mapped
  source files.
- `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`: priority, prerequisites, keywords,
  comparison rules, and interview focus.
- `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`: lesson type, depth, output structure,
  style, and quality checklist.
- `docs/C++ Notion/`: primary detailed C++ source material.

`docs/C++ Notion/C++ Programming.md` is the Notion index. The chapter files are
the actual detailed lesson sources.

## Newbie Prompt Contract

The user may provide only:

- topic or keyword;
- comparison request;
- interview request;
- code review request;
- optional source or chapter hint.

Do not require the user to know C/C++ keywords. Discover prerequisites,
comparisons, examples, common bugs, debugging angles, and interview traps from
the mapped sources.

## Source Routing

1. Load `LEARNING_PATH.md` before choosing a number, slug, output path, or mapped
   source list.
2. Read the mapped master chapter to determine priority and dependency depth.
3. Read the expansion guide section for the requested output type:
   quick note, full lesson, interview pack, code review guide, or comparison
   note.
4. For C++ topics, read every mapped Notion chapter in scope before writing.
5. For C-only, embedded C, industrial C, safety, secure coding, or exact
   standard/library behavior, use trusted external references and record them in
   the topic brief.
6. Never use Linux Device Driver or kernel-driver material for this skill unless
   the user explicitly asks for that separate domain.

## Output Naming

Use learning-path numbering for new finished output:

- `knowledge/NN-<slug>.md`
- `interview/NN-<slug>.md`
- `examples/NN-<slug>/README.md`
- `coverage/topic-briefs/NN-<slug>.md`

Raw Notion chapter numbers are source metadata only.

## Default Full Pipeline

1. Topic routing: identify learning-path number, slug, master chapter, mapped
   Notion chapters, external references likely needed, and output paths.
2. Source reading: inspect all mapped Notion files in scope plus master and guide
   requirements.
3. Topic brief: create `coverage/topic-briefs/NN-<slug>.md` with source files
   read, source coverage status, priority, dependencies, concepts, comparisons,
   bugs, debugging notes, practice tasks, interview angles, gaps, external
   references, and target output paths.
4. Teach: create learner-facing `knowledge/NN-<slug>.md` without audit tables.
5. Interview: create `interview/NN-<slug>.md` with beginner, mid-level, and
   senior questions and answers.
6. Example: create or update `examples/NN-<slug>/README.md` when compile-ready
   code, sanitizer usage, or debugging commands help.
7. Review: apply the C/C++ quality rules before finalizing.

## Lesson Type Rules

- Quick note: definition, key points, common mistakes, interview note.
- Full lesson: goal, concept, examples, comparison, common bugs, best practices,
  debugging, interview, practice, summary.
- Interview pack: must-know concepts, common questions, model answers, tricky
  questions, coding tasks, debugging questions.
- Code review guide: correctness, memory safety, undefined behavior, thread
  safety, API design, C/C++ best practices, performance, maintainability, and
  suggested refactor.
- Comparison note: short conclusion, comparison table, when to use, common bugs,
  interview answer.

## Quality Rules

- Follow priority depth from the master index:
  MUST -> deep, SHOULD -> medium, NICE -> short, EXPERT -> controlled advanced.
- Include required comparisons from the master index when applicable.
- Use clear technical English by default and preserve exact C/C++ terminology.
- Keep examples minimal, compile-oriented, practical, and clearly commented.
- Warn about unsafe C APIs, manual memory management, undefined behavior,
  exception-safety holes, iterator invalidation, data races, deadlocks, and
  ownership ambiguity when relevant.
- Prefer practical embedded/enterprise examples: HAL interface, logger, FSM,
  callback, ring buffer, thread-safe queue, RAII file descriptor wrapper,
  protocol parser, sensor interface, and command dispatcher.
- Do not overuse design patterns. Explain the problem first and use a pattern
  only when it improves maintainability.
- Fail output that skips mapped Notion sources, claims completion without source
  coverage, ignores priority depth, or mixes in unrelated Linux driver/domain
  content.
