# Interview Template

Use this for `interview/chXX_<topic>_interview_questions.md`.

# Chapter X Interview - <Topic>

## Review Basis
List repo knowledge files, TLPI chapters, DevLinux modules, man-pages, and any calibration sources used.

## Coverage Trace
List the learning-map topic rows and chapter Must Cover concepts. Each item must appear in Priority A, B, or C coverage.

## Priority Map
- A: high-probability production/project/debug/design scenarios.
- B: comparison and trade-off questions.
- C: recognize-only APIs, flags, or rare details.

## Scenario Questions
For each Priority A question:

### N. <Scenario question>

**What the interviewer is testing**

**Strong answer**

**Mechanism**

**Pitfalls**

**Debug angle**

**Follow-up keywords**

## Comparison Questions
Use compact but precise answers for Priority B.

## Recognize Only
List Priority C items with one-line purpose and when to read the manual.

## Rules
- Scenario-first: start from bug, project design, debugging workflow, tradeoff, reliability, security, performance, or Embedded constraint.
- Do not make most headlines "What is <API>?".
- Answers must teach reasoning, not memorized slogans.
- Use English by default unless the user explicitly asks otherwise.
- Priority A/B/C coverage must trace back to the Coverage Matrix and chapter coverage contract.
- Missing must-cover concepts are blockers, even when the question set reads well.
