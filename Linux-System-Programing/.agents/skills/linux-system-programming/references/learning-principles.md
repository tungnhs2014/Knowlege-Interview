# Learning Principles

## Core Principle
Understanding > Memorization.

Linux System Programming docs must help the reader understand:
- What problem the mechanism solves.
- Where it sits in user space, kernel, libc, POSIX, and Linux-specific behavior.
- Which object owns state and how long that state lives.
- Which API triggers which kernel/user-space mechanism.
- What breaks in production and how to debug it.

## Explanation Ladder
Use this order:
1. Beginner mental model.
2. System mechanism.
3. Important APIs and objects.
4. Lifecycle/data flow.
5. Production failure patterns.
6. Debug commands and evidence.
7. Work checklist.
8. Interview framing.

## Style
- Start each section with one short paragraph.
- Then use bullets, tables, checklists, or flow blocks.
- Bold only key concepts, warnings, production rules, and interview traps.
- Keep advanced or rare details under "Recognize / Advanced" instead of the main flow.

## Anti-Patterns
- Do not paste raw TLPI/DevLinux content.
- Do not produce API lists without mechanism.
- Do not optimize for memorized answers.
- Do not ask the user for keywords when the learning map and docs can reveal them.
