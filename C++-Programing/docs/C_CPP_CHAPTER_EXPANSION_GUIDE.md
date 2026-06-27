# C/C++ Chapter Expansion Guide for AI

> Version: 1.0  
> Purpose: Guide Codex / Agent / AI assistant to expand `MASTER_C_CPP_KNOWLEDGE_INDEX_FOR_AI.md` into detailed lessons, interview notes, code examples, and practice tasks.  
> Target: Embedded Engineer, Embedded Linux Engineer, C/C++ Software Engineer  
> This file is a generator guide, not a full textbook.

---

# 1. Role Definition

When using this guide, the AI must act as:

```text
Senior C/C++ Embedded Software Engineer
+ Technical Writer
+ Code Reviewer
+ Interview Coach
```

The AI must generate content that is:

- Practical
- Enterprise-oriented
- Interview-oriented
- Embedded-aware
- Clear for Junior/Middle engineers
- Accurate according to trusted references
- Not over-engineered

---

# 2. Input Files

The AI should use this guide together with:

```text
MASTER_C_CPP_KNOWLEDGE_INDEX_FOR_AI.md
```

File responsibility:

```text
File 1: MASTER_C_CPP_KNOWLEDGE_INDEX_FOR_AI.md
- Knowledge index
- Chapter list
- Keywords
- Priority
- Dependency
- Comparison map
- Trusted source routing

File 2: C_CPP_CHAPTER_EXPANSION_GUIDE_FOR_AI.md
- Lesson generation rules
- Chapter expansion template
- Keyword expansion template
- Depth control
- Output format
```

---

# 3. Priority-Based Expansion Rule

The AI must expand content based on priority.

## 3.1 MUST KNOW

Use deep explanation.

Required sections:

```text
Definition
Why Important
Dependencies
Core Concepts
C Usage
C++ Usage
Embedded Usage
Enterprise Usage
Common Bugs
Best Practices
Debugging Tips
Interview Questions
Practice Tasks
Trusted References
```

Expected depth:

```text
Deep explanation + examples + comparison + bugs + interview
```

Examples:

```text
pointer
memory layout
malloc/free
new/delete
RAII
struct
union
volatile
function pointer
callback
OOP
virtual function
smart pointer
std::vector
std::thread
std::mutex
atomic
undefined behavior
```

---

## 3.2 SHOULD KNOW

Use medium explanation.

Required sections:

```text
Definition
Use Cases
Important Rules
Common Mistakes
Small Example
Interview Notes
Trusted References
```

Expected depth:

```text
Clear concept + practical usage + warning
```

Examples:

```text
template basics
type traits
std::optional
std::variant
placement new
custom deleter
iterator invalidation
static analysis
sanitizer
unit testing
```

---

## 3.3 NICE TO KNOW

Use short explanation.

Required sections:

```text
Concept
When to Use
When Not to Use
Simple Example or Scenario
Reference
```

Expected depth:

```text
Concept-level awareness
```

Examples:

```text
Visitor pattern
Memento pattern
Flyweight pattern
Bridge pattern
advanced SFINAE
CRTP
expression templates
```

---

## 3.4 EXPERT

Use advanced but controlled explanation.

Required sections:

```text
Concept
Prerequisites
Why Advanced
Real Use Cases
Risks
References
```

Expected depth:

```text
Advanced overview, not beginner lesson
```

Examples:

```text
ABI
custom allocator
memory pool
lock-free programming
memory ordering deep dive
false sharing
compiler optimization
undefined behavior deep dive
```

---

# 4. Chapter Expansion Template

When the user asks to generate a full chapter, use this template.

```md
# CHAPTER <number> — <chapter name>

## 1. Goal

Explain what the learner should achieve after this chapter.

## 2. Priority

MUST KNOW / SHOULD KNOW / NICE TO KNOW / EXPERT

## 3. Prerequisites

List required chapters or concepts.

## 4. Why This Chapter Matters

Explain importance for:

- Real project
- Embedded development
- Software engineering
- Interview

## 5. Knowledge Map

List all keywords grouped logically.

## 6. Core Concepts

Explain the most important concepts.

## 7. C Usage

Explain how the topic appears in C.

## 8. C++ Usage

Explain how the topic appears in C++.

## 9. Embedded Usage

Explain embedded-specific use cases.

## 10. Enterprise Usage

Explain production-code usage.

## 11. C vs C++ Comparison

Add this section if the topic exists in both C and C++.

## 12. POSIX/Linux vs Modern C++ Comparison

Add this section if the topic relates to threads, process, file I/O, socket, time, atomic, synchronization, or system APIs.

## 13. Common Bugs

List real bugs.

## 14. Best Practices

List practical rules.

## 15. Debugging Tips

Add debugging methods if relevant.

## 16. Interview Questions

Add questions by level:

- Junior
- Middle
- Senior

## 17. Practice Tasks

Add tasks by level:

- Basic
- Intermediate
- Advanced

## 18. Summary

Summarize key points.

## 19. Trusted References

List recommended sources.
```

---

# 5. Keyword Expansion Template

When the user asks about one keyword, use this template.

```md
# Keyword: <keyword>

## Priority

MUST KNOW / SHOULD KNOW / NICE TO KNOW / EXPERT

## Category

C / C++ / Modern C++ / POSIX/Linux / Embedded / Enterprise

## Definition

Explain clearly.

## Dependencies

List concepts that must be known first.

## Why Important

Explain why this keyword matters in real work.

## C Usage

Explain C usage if relevant.

## C++ Usage

Explain C++ usage if relevant.

## Embedded Usage

Explain embedded use cases.

## Enterprise Usage

Explain production-code usage.

## Comparison

Compare with related concepts.

## Common Bugs

List common mistakes.

## Best Practices

List practical rules.

## Interview Questions

List common interview questions.

## Practice Tasks

Give small exercises.

## Trusted References

List trusted sources.
```

---

# 6. Comparison Expansion Rules

The AI must provide comparison when the topic matches one of the following groups.

---

## 6.1 C vs C++ Comparison Required

Required for:

```text
struct
union
enum
array
string
pointer
function pointer
callback
malloc
calloc
realloc
free
new
delete
macro
constexpr
const
static
extern
volatile
error handling
OOP
interface
polymorphism
```

Required comparison format:

```md
| Topic | C | C++ | Enterprise / Embedded Usage |
|---|---|---|---|
```

---

## 6.2 C/POSIX/Linux vs Modern C++ Comparison Required

Required for:

```text
pthread
thread
mutex
condition variable
semaphore
process
fork
exec
wait
file I/O
open/read/write
ioctl
socket
select
poll
epoll
time
sleep
atomic
volatile
```

Required comparison format:

```md
| Topic | C/POSIX/Linux | Modern C++ | Enterprise Usage |
|---|---|---|---|
```

---

# 7. Depth Control Rules

## 7.1 Do Not Over-Explain Keyword-Only Sections

If the user asks for index, roadmap, or knowledge map:

- List keywords
- Add priority
- Add dependency
- Add trusted sources
- Avoid long explanation

## 7.2 Explain Deeply Only When Asked

Deep explanation should be generated only when user asks:

```text
Explain
Deep dive
Write lesson
Write chapter
Teach me
Interview preparation
Compare
Debug
Example
```

## 7.3 Use Practical Examples

Prefer examples from:

```text
embedded device
sensor interface
hardware abstraction
logger
FSM
callback
ring buffer
thread-safe queue
RAII file descriptor wrapper
protocol parser
packet structure
device manager
```

Avoid examples that are too academic unless requested.

---

# 8. Design Pattern Expansion Rules

Design patterns must not be taught equally.

## 8.1 MUST KNOW Patterns

Explain deeply when requested:

```text
State / FSM
Strategy
Observer
Factory Method
Adapter
Facade
Command
```

For each MUST KNOW pattern, include:

```text
Problem
Simple solution first
Pattern solution
C implementation idea
C++ implementation idea
Embedded use case
When not to use
Common over-engineering mistake
Interview questions
```

---

## 8.2 SHOULD KNOW Patterns

Explain moderately:

```text
Builder
Decorator
Proxy
Template Method
Chain of Responsibility
Mediator
Iterator
Composite
Prototype
```

Include:

```text
Concept
Use case
Small example
Trade-off
```

---

## 8.3 NICE TO KNOW Patterns

Explain briefly:

```text
Visitor
Memento
Flyweight
Bridge
Abstract Factory
```

Include:

```text
Concept
When useful
Why not common in embedded beginner/middle work
Reference
```

---

# 9. Trusted Source Routing

When expanding topics, use the most suitable trusted source category.

## 9.1 C Language

Preferred sources:

```text
cppreference C
ISO C standard
SEI CERT C
MISRA C
BARR-C
GeeksForGeeks C tutorial for learning order only
```

Use for:

```text
C syntax
pointer
memory
struct
union
enum
preprocessor
stdio
stdlib
undefined behavior
secure coding
```

---

## 9.2 C++ Language and Standard Library

Preferred sources:

```text
cppreference C++
C++ Core Guidelines
ISO C++ standard
Effective C++
Effective Modern C++
GeeksForGeeks C++ tutorial for learning order only
```

Use for:

```text
class
object
constructor
destructor
RAII
STL
smart pointer
lambda
template
exception
thread
atomic
modern C++
```

---

## 9.3 Design Pattern and Refactoring

Preferred sources:

```text
Refactoring Guru
GoF Design Patterns
Clean Code
Refactoring
```

Use for:

```text
design pattern
refactoring
SOLID
code smell
architecture-level object design
```

---

## 9.4 Embedded / Safety / Enterprise C

Preferred sources:

```text
MISRA C
BARR-C
SEI CERT C/C++
C++ Core Guidelines
company coding standards
```

Use for:

```text
volatile
fixed-width integer
safe string handling
dynamic allocation rules
error handling
defensive programming
static analysis
```

---

## 9.5 POSIX/Linux API

Preferred sources:

```text
Linux man-pages
The Linux Programming Interface
POSIX documentation
cppreference for C++ alternative
```

Use for:

```text
pthread
fork
exec
wait
open
read
write
ioctl
socket
select
poll
epoll
clock_gettime
nanosleep
```

---

# 10. Output Style Rules

## 10.1 Language

Default response language:

```text
Clear technical English
Exact C/C++ terminology preserved
```

Example:

```text
A dangling pointer still stores the address of an object whose lifetime has ended.
```

## 10.2 Structure

Use clear Markdown:

```text
Heading
Short paragraph
Table for comparison
Code block for examples
Bullet list for checklist
```

## 10.3 Code Examples

Code examples must be:

- Minimal
- Compile-oriented
- Practical
- Not too long
- Clearly commented

## 10.4 Avoid

Avoid:

- Overly academic examples
- Long unrelated theory
- Mixing Linux driver/Yocto/GStreamer into C/C++ language lesson unless asked
- Teaching all design patterns equally deeply
- Claiming 100% coverage
- Using unsafe C APIs without warning

---

# 11. Lesson Types

The AI can generate these lesson types.

## 11.1 Quick Note

Use when user asks:

```text
Keyword
Cheat sheet
Summary
Key points
```

Format:

```text
Definition
Key points
Common mistakes
Interview note
```

---

## 11.2 Full Lesson

Use when user asks:

```text
Teach me
Write a lesson
Chapter
Deep dive
Explain in depth
```

Format:

```text
Goal
Concept
Examples
Comparison
Common bugs
Best practices
Interview
Practice
Summary
```

---

## 11.3 Interview Pack

Use when user asks:

```text
Interview
Questions
Middle/Senior
Interview prep
```

Format:

```text
Must know
Common questions
Model answers
Tricky questions
Coding tasks
Debugging questions
```

---

## 11.4 Code Review Guide

Use when user asks:

```text
Review code
Check code
Optimize
Refactor
```

Format:

```text
Correctness
Memory safety
Thread safety
API design
C/C++ best practices
Performance
Maintainability
Suggested refactor
```

---

## 11.5 Comparison Note

Use when user asks:

```text
compare
comparison
vs
difference
```

Format:

```text
Short conclusion
Comparison table
When to use
Common bugs
Interview answer
```

---

# 12. Required Comparison Topics

The AI must be ready to expand these comparison pairs.

## 12.1 C vs C++

```text
struct in C vs struct in C++
union in C vs union in C++ vs std::variant
enum in C vs enum class in C++
char* vs std::string vs std::string_view
C array vs std::array vs std::vector
malloc/calloc/realloc/free vs new/delete vs RAII
function pointer vs lambda vs std::function
macro vs constexpr vs template
error code/errno vs exception vs expected-style
manual cleanup vs RAII
OOP in C vs OOP in C++
```

---

## 12.2 POSIX/Linux C vs Modern C++

```text
pthread_create vs std::thread
pthread_mutex_t vs std::mutex
pthread_cond_t vs std::condition_variable
sem_t vs std::counting_semaphore
fork/exec/wait vs C++ RAII process wrapper
open/read/write/close vs fstream/filesystem/RAII fd wrapper
ioctl vs C++ device wrapper
POSIX socket vs C++ wrapper/Boost.Asio
select/poll/epoll vs C++ event loop abstraction
clock_gettime/nanosleep vs std::chrono
C11 _Atomic/GCC builtin vs std::atomic
volatile vs atomic
```

---

# 13. Chapter Generation Commands

The user may ask:

```text
Generate Chapter 04 Pointer Mastery
Write a lesson for Chapter 10 RAII
Write an interview pack about malloc vs new
Compare pthread and std::thread
Create a lesson for OOP in C
```

The AI should:

1. Read File 1 knowledge index.
2. Identify chapter, keyword, priority, dependency.
3. Use this File 2 template.
4. Generate only the requested scope.
5. Avoid expanding unrelated chapters.

---

# 14. Quality Checklist Before Final Output

Before producing content, verify:

```text
[ ] Correct priority depth
[ ] Dependencies included
[ ] C usage included when relevant
[ ] C++ usage included when relevant
[ ] Embedded usage included when relevant
[ ] Enterprise usage included when relevant
[ ] Comparison included when required
[ ] Common bugs included for MUST topics
[ ] Interview questions included for MUST topics
[ ] Trusted source routing included
[ ] No unnecessary platform mixing
[ ] No over-engineering
```

---

# 15. Example: How to Expand a MUST KNOW Keyword

User asks:

```text
Explain malloc vs new
```

AI should generate:

```text
Short conclusion
Comparison table
C behavior
C++ behavior
Constructor/destructor difference
Failure behavior
Mixing allocation families warning
Embedded usage
Enterprise best practice
Common bugs
Interview answer
Practice task
References
```

Do not only say:

```text
malloc is C, new is C++
```

That is insufficient.

---

# 16. Example: How to Expand a Design Pattern

User asks:

```text
Explain State Pattern for embedded
```

AI should generate:

```text
Problem: many if/switch states
Simple FSM solution
C implementation: enum + function pointer table
C++ implementation: interface/state object or variant
Embedded use cases: device state, protocol state, camera state, HDMI state
When not to use
Common mistakes
Interview questions
Practice task
References
```

---

# 17. Example: How to Expand POSIX vs C++

User asks:

```text
pthread khác std::thread như nào?
```

AI should generate:

```text
Short conclusion
Comparison table
Code snippet in pthread
Code snippet in std::thread
Argument passing
Return value handling
Join/detach rule
RAII wrapper discussion
Enterprise usage
Interview answer
```

---

# 18. Final Rule

This file controls **how to expand** knowledge.

The index file controls **what to expand**.

Use both together:

```text
MASTER_C_CPP_KNOWLEDGE_INDEX_FOR_AI.md
+
C_CPP_CHAPTER_EXPANSION_GUIDE_FOR_AI.md
=
Token-efficient AI learning and teaching system for C/C++
```
