# Topic Template

Use this learner-facing template for `knowledge/NN-<topic>.md`.
Canonical example: `knowledge/07-character-device-drivers.md`.

Audit metadata belongs in `coverage/topic-briefs/NN-<topic>.md`, not in this file.

Style rule:

- Start each major section with one short explanation paragraph.
- Then use bullets, tables, checklists, diagrams, or flow blocks for details.
- Prefer roughly 70% structured bullets/tables/checklists and 30% short paragraphs.
- Bold only important concepts, warnings, production rules, and interview traps.
- Avoid wall-of-text, over-bold text, raw doc duplication, audit tables, and API dumps without mechanism.

# <NN> - <Topic>

## Learning Goal
State what the reader should understand and do after this topic.

## Why This Matters In Real Work
Explain why this topic exists, where it appears in real drivers, and what problems it solves.

## Mental Model
Explain the topic in simple language before naming many APIs.

## Core Concepts
Define the core terms and compare related ideas when that helps memory.

## Kernel Mechanism
Explain the real flow, object relationships, ownership, and lifetime rules.

## Key Structs And APIs
Explain important structs, callbacks, and helper APIs in context, not as a memorization list.

## Lifecycle / Data Flow
Show the order of important calls and state changes.

## Minimal Practical Example
Provide concise code or pseudo-code, explain the important lines, and state whether it is learning-only or production-ready.

## Common Bugs And Debugging
Start from observable symptoms. Explain likely causes, evidence to inspect, kernel logs/commands, sysfs/debugfs/procfs clues, tracing or dynamic debug when relevant, and fix patterns.

## Production Checklist
List what an engineer should verify before review or bring-up.

## Interview Readiness
Summarize what the reader must be able to explain without memorizing and link to `interview/NN-<topic>.md`.

## Kernel Version Notes
Use this only when APIs or behavior are version-sensitive. Keep it short and practical.
