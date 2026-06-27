# Knowledge Template

Use this for `knowledge/chXX_<topic>.md`.

# Chapter X - <Topic>

## Learning Goal
Short paragraph stating what the reader should understand and be able to do.

## Coverage Notes
Short checklist naming the learning-map topic rows and chapter Must Cover concepts this file covers. If a concept moved to another file or is intentionally out of scope, state that here.

## Problem It Solves
Explain the real system/software problem before naming many APIs.

## Mental Model
Beginner-friendly explanation plus a small flow diagram or table.

## Mechanism
Explain the Linux/POSIX mechanism: objects, ownership, lifetime, and who calls whom.

## Key APIs And Objects
Use tables and bullets. Explain APIs in context, not as a memorization list.

## Lifecycle / Data Flow
Show the order of calls, state changes, blocking behavior, inheritance, or cleanup rules.

## Production Bugs And Debugging
Start from symptoms. Include likely causes, evidence, commands, and fix patterns.
Prefer practical tools such as `strace`, `gdb`, `lsof`, `/proc`, `dmesg`, `ss`, `ipcs`, `lslocks`, `getfacl`, and `perf` when relevant.

## Work Checklist
Concrete checklist for design, code review, production debugging, or Embedded constraints.

## Recognize / Advanced
Keep rare flags, portability details, and deep edge cases here unless they are central.

## Interview Readiness
Summarize what the reader must explain without memorizing and link to the interview doc.

## Final Coverage Check
Before finalizing, verify the Coverage Matrix: mapped rows, must-cover concepts, key APIs/objects, lifecycle/data flow, production bugs, debugging commands, Embedded constraints, and interview links.
