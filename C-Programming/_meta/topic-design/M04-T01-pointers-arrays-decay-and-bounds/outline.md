# M04-T01 — Detailed Outline for Review

## 1. Review state and artifact status

| Field | Status |
| --- | --- |
| Topic lifecycle | `OUTLINE_REVIEW` |
| Lesson | `NOT_STARTED` |
| Examples | `NOT_STARTED` |
| Exercises | `NOT_STARTED` |
| Interview | `NOT_STARTED` |
| Technical review | `NOT_STARTED` |
| Human review | `PENDING` |
| Exercise source coverage | `PARTIAL_EXERCISE_COVERAGE` |

Completion of this outline is not approval. The topic cannot move to `LESSON_DRAFT` without explicit human approval of this outline.

## 2. Topic contract

| Contract item | Planned decision |
| --- | --- |
| Purpose | Enable an engineer to reason about arrays, pointer values, and bounds before writing or reviewing low-level C that indexes, traverses, or passes buffers. |
| Audience | Learners following the Advanced C curriculum for embedded, firmware, and Linux/POSIX user-space work. |
| Required prerequisites | `M01-T02-abstract-machine-and-behavior`; `M01-T03-integer-conversions-and-floating-point`; `M02-T01-scope-linkage-and-storage-duration`. |
| Pre-Training assumption | Basic pointer declaration, dereference syntax, basic arrays, and functions are assumed. This is not an introductory pointer-syntax lesson. |
| Included scope | Pointer object/value/pointee; operational pointer states; arrays and conversion; non-decay contexts; arithmetic/one-past; equality, relational comparison, and subtraction; parameter adjustment and extent contracts; constrained multidimensional arrays; defects and diagnostic tools. |
| Excluded scope | Allocation/ownership (`M03-T03`); `T **`, `void *`, generic APIs (`M04-T02`); function pointers/callbacks (`M04-T03`); aliasing/alignment/object representation (`M04-T04`); complete strings (`M05-T02`); byte parsing (`M05-T03`); pointer provenance pending a later standards decision. |
| Context labels | Primarily **ISO C**. Compiler diagnostics and sanitizers are **compiler/tool** context. Any pointer size, address space, ABI, or MCU claim requires its own **ABI/compiler/target** context. |
| Misconceptions to correct | Arrays are pointers; `sizeof` finds an array’s original size in a function; a pointer contains bounds; one-past is dereferenceable; arbitrary pointers can be subtracted or relationally ordered; `NULL` means every invalid/dangling state; `volatile` makes an access bounded or memory-safe. |
| Industrial relevance | Buffer APIs, drivers that index tables, fixed-size firmware data, multidimensional sample blocks, code review of off-by-one defects, and sanitizer-guided debugging. |

## 3. Measurable learning outcomes

After the approved lesson and verification work, a learner should be able to:

1. Distinguish a pointer object, its stored pointer value, and the object an access is intended to designate.
2. Classify a given pointer use as valid, null, one-past, indeterminate/uninitialised, dangling, or invalid for the stated access—without treating those labels as interchangeable.
3. Explain that an array object is not a pointer object, and identify ordinary expression contexts where array-to-pointer conversion occurs and contexts where it does not.
4. Predict why `sizeof` can describe an array object at its declaration context but cannot recover a caller array’s extent after parameter adjustment.
5. Distinguish equality/inequality from relational ordering, and apply the approved array-domain rules for pointer arithmetic, one-past formation, pointer subtraction, and relational comparison without generalising unrelated-pointer cases.
6. Explain the distinct roles of `size_t` (sizes/counts) and `ptrdiff_t` (the result type of permitted pointer subtraction), including the need to verify subtraction result representability against ISO C17.
7. Specify a pointer-and-length or pointer-and-end contract that makes bounds responsibility visible to a caller and callee.
8. Choose and read a pointer-to-array parameter type for a constrained multidimensional array; explain why a flat pointer and an array of pointers have different meanings.
9. Recognise off-by-one, bounds, uninitialised-pointer, and dangling-use symptoms, then select an appropriate compiler warning, sanitizer, debugger, or review check without claiming the tool proves all correctness.
10. Identify when a question belongs to M03-T03, M04-T02, M04-T03, M04-T04, M05-T02, or M05-T03 rather than extending this lesson.

## 4. Proposed lesson structure

This is a hierarchical planning structure, not lesson prose. Every standards-sensitive guarantee below is a verification target, not a statement that has already been verified.

### 4.1 Orientation: contracts, contexts, and limits

- **Purpose:** establish why advanced pointer work begins with objects, bounds, and applicable context rather than address arithmetic.
- **Key concepts:** ISO C versus tool/ABI/target claims; prerequisites; topic boundaries; explicit buffer contracts.
- **Guarantee / behaviour category:** context classification and scope contract; no target-specific guarantee.
- **Misconception addressed:** a numeric-looking address alone establishes safe access.
- **Planned correct / incorrect example:** a documented pointer-plus-length interface contrasted with an undocumented raw pointer interface.
- **Industrial relevance:** reviewable driver, parser, and table APIs.
- **Verification:** lesson review against architecture; later ISO C verification for any semantics claim.
- **Explicit boundary:** no allocation ownership, generic pointer API, callback, or aliasing instruction.

### 4.2 Pointer objects, values, pointees, and operational states

- **Purpose:** make the distinction needed to discuss validity without equating pointer representation with access permission.
- **Key concepts:** pointer object/value/pointee; valid access; null; one-past; indeterminate/uninitialised; dangling; invalid attempted access.
- **Guarantee / behaviour category:** ISO C behavior and lifetime-sensitive reasoning; tool observations are diagnostic only.
- **Misconception addressed:** `NULL` is the same as dangling or invalid, or a non-null value proves access is valid.
- **Planned correct / incorrect example:** initialise and validate a bounded pointer before use versus using an uninitialised or stale pointer (the latter described without allocation mechanics).
- **Industrial relevance:** crash triage and API precondition reviews.
- **Verification:** ISO C for object lifetime/value use; compiler/sanitizer documentation for observable diagnostics.
- **Explicit boundary:** allocation and ownership transitions remain M03-T03; provenance is not taught.

### 4.3 Arrays are objects; array-to-pointer conversion is contextual

- **Purpose:** replace the “array is a pointer” shortcut with a usable object-and-conversion model.
- **Key concepts:** array object, first element, decay in ordinary expressions, `&array`, and pointer type consequences.
- **Guarantee / behaviour category:** ISO C conversion rules.
- **Misconception addressed:** an array declaration creates a pointer, or every appearance of an array expression produces a pointer.
- **Planned correct / incorrect example:** pass/initialise from an array in a conversion context versus compare `sizeof array` and `sizeof pointer` at the declaration site.
- **Industrial relevance:** correct APIs and review of embedded lookup tables.
- **Verification:** ISO C; compile-time type/diagnostic observations where supported.
- **Explicit boundary:** character-array and string-literal lifetime details belong to M05-T02.

### 4.4 Non-decay contexts and extent information

- **Purpose:** identify what remains known where the actual array object is visible and what is lost after interface conversion.
- **Key concepts:** `sizeof`, address-of an array, declared extent, element count calculation at a valid declaration context, and a string literal used to initialise an array as a non-decay initialisation case.
- **Guarantee / behaviour category:** ISO C type and expression semantics.
- **Misconception addressed:** `sizeof` of a pointer gives the caller array size, or a pointer carries a length.
- **Planned correct / incorrect example:** compile-time array-count calculation locally versus the same-looking operation on a function parameter; mention array initialisation from a string literal without teaching string lifetime.
- **Industrial relevance:** array-table sizing and avoiding silent API truncation defects.
- **Verification:** ISO C; compiler warnings where an implementation can diagnose a mismatch.
- **Explicit boundary:** full string-literal and character-array semantics belong to `M05-T02-character-arrays-string-literals-and-lifetime`; macros/generic selection and build-system mechanisms are not taught here.

### 4.5 Function parameter adjustment and bounds contracts

- **Purpose:** connect conversion semantics to concrete caller/callee responsibilities.
- **Key concepts:** adjusted array parameters, pointer-plus-length and begin/end contracts, preconditions, capacity versus used length.
- **Guarantee / behaviour category:** ISO C parameter adjustment plus API contract design.
- **Misconception addressed:** an array parameter preserves the full caller array type/size, or bounds checking is implied by `const`/`volatile`.
- **Planned correct / incorrect example:** bounded traversal supplied with an explicit length versus a parameter that assumes an unknown array size.
- **Industrial relevance:** firmware tables, POSIX buffer APIs, and safety review.
- **Verification:** ISO C; API design and test review.
- **Explicit boundary:** `T **` output and generic interfaces go to M04-T02; string API contracts go to M05-T02.

### 4.6 Pointer arithmetic and the one-past position

- **Purpose:** define traversal terms without turning pointer arithmetic into unrestricted address manipulation.
- **Key concepts:** element scaling, same-array domain, a non-array object treated as a one-element array where the ISO C rule applies, null pointers as invalid arithmetic operands, one-past permitted operations, and subscript notation as pointer arithmetic with the same bounds requirements.
- **Guarantee / behaviour category:** ISO C pointer arithmetic rules; exact C17 wording for non-array objects, null operands, one-past use, and subscript equivalence is a mandatory verification target. Erroneous operations must not be modelled as valid execution.
- **Misconception addressed:** one-past may be dereferenced, null is a usable arithmetic base, or arithmetic outside an array is permitted because addresses are numeric.
- **Planned correct / incorrect example:** a half-open `[begin, end)` traversal that compares but never dereferences `end`, contrasted with a one-past write.
- **Industrial relevance:** loop bounds, DMA-buffer descriptors (with target-specific parts excluded), and off-by-one code review.
- **Verification:** ISO C; sanitizer behavior later verified against official tool documentation.
- **Explicit boundary:** no MMIO/`volatile` semantics or hardware register example.

### 4.7 Pointer subtraction, equality, and relational comparison

- **Purpose:** distinguish three operations that are often collapsed into generic “pointer comparison.”
- **Key concepts:** equality `==`/`!=`; relational `<`, `<=`, `>`, `>=`; same-array subtraction; `ptrdiff_t`; result representability; portable array-range reasoning.
- **Guarantee / behaviour category:** ISO C comparison and subtraction rules, requiring exact C17 verification before final wording. Equality/inequality are planned separately from relational ordering; neither will be reduced to a machine-address story.
- **Misconception addressed:** arbitrary pointers may be subtracted or relationally ordered to discover memory layout, or every equality check on unrelated pointers is automatically forbidden.
- **Planned correct / incorrect example:** distance between two positions of the same array, using `ptrdiff_t`, versus an attempted unrelated subtraction or relational ordering. The standard-defined result domain and representability require verification.
- **Industrial relevance:** range validation and portable review rules.
- **Verification:** ISO C; compiler behavior must not be treated as proof of portability.
- **Explicit boundary:** pointer/integer conversions and provenance remain outside scope; equality edge cases are stated only after ISO C17 verification.

### 4.8 Constrained multidimensional arrays and pointer-to-array types

- **Purpose:** show why a row stride belongs to the parameter type when describing a true multidimensional array.
- **Key concepts:** array-of-arrays layout, fixed-column pointer-to-array parameter type, row width, and contrast with `T **`.
- **Guarantee / behaviour category:** ISO C array and parameter type semantics.
- **Misconception addressed:** `T **` is universally interchangeable with `T [rows][columns]`, or a flat pointer states the same layout.
- **Planned correct / incorrect example:** one fixed-column embedded image/sample-buffer parameter versus an incompatible pointer-to-pointer declaration.
- **Industrial relevance:** a small embedded image/sample-buffer contract.
- **Verification:** ISO C and compiler diagnostics; no ABI, cache-performance, or optimisation claim without a target source.
- **Explicit boundary:** no ragged allocation; `T **` API design is M04-T02; byte layout/aliasing are M04-T04. A short advanced note may name VLA or `static` array parameters only with compiler/toolchain support and ISO C verification; it must not imply automatic runtime bounds checking.

### 4.9 Defects, diagnostics, and verification limits

- **Purpose:** turn the semantics into a disciplined debugging workflow while preserving the difference between a tool report and a language proof.
- **Key concepts:** off-by-one, out-of-bounds subscript, uninitialised pointer, dangling use, warnings, sanitizers, debugger inspection, review checklists.
- **Guarantee / behaviour category:** compiler/tool context; tool coverage is inherently incomplete.
- **Misconception addressed:** no warning or sanitizer report proves an access is valid; `volatile` fixes bounds or memory safety.
- **Planned correct / incorrect example:** intentional bounded test failure observed by a selected sanitizer versus a correct contract test; commands are only planned pending tool verification.
- **Industrial relevance:** triaging production faults and maintaining defensible verification records.
- **Verification:** official GCC/Clang and sanitizer documentation; reviewed build environment.
- **Explicit boundary:** testing framework, fuzzing strategy, and formal standards compliance are M10-T02/M08-T03.

### 4.10 Consolidation: decision checklist and canonical links

- **Purpose:** consolidate a review decision without re-teaching prerequisite material.
- **Key concepts:** object, array domain, extent source, operation, context, owner link.
- **Guarantee / behaviour category:** learner decision process; no new C semantic rule.
- **Misconception addressed:** every pointer question belongs to one broad “pointer mastery” chapter.
- **Planned correct / incorrect example:** classify a small API review case as in-scope or redirect it to the owning topic.
- **Industrial relevance:** prevents duplicated semantics across firmware modules and reviews.
- **Verification:** human lesson review against ownership matrix and granularity guard.
- **Explicit boundary:** no new exercise, interview, or implementation material is embedded here.

## 5. Planned examples (descriptions only)

No example source file is authorised at this stage. The future examples require separate `EXAMPLES_DRAFT` and `EXAMPLES_REVIEW` approval.

| Planned example | Objective | Safe/unsafe classification | Expected compiler mode / flags | Sanitizer or tool target | Scope constraint |
| --- | --- | --- | --- | --- | --- |
| Local `sizeof` plus parameter adjustment | Show local array extent versus adjusted parameter and explicit extent input. | Safe | Proposed C17 baseline; warning flags selected after tool review | Compiler/type inspection | Mention string-literal array initialisation only as a non-decay note; no string lesson. |
| Half-open traversal plus isolated one-past defect | Form and use a one-past end only in permitted operations; isolate a one-past dereference as incorrect. | Mixed: safe traversal; unsafe defect isolated | Proposed C17 baseline with reviewed warnings | AddressSanitizer target, subject to compiler/version verification | No null arithmetic or arbitrary pointer ordering. |
| Same-array subtraction plus “Unrelated subtraction and relational ordering” | Use `ptrdiff_t` for a permitted same-array distance; reject unrelated subtraction or relational ordering as portable reasoning. | Mixed: safe distance; unsafe/non-executed negative case | Proposed C17 baseline | Static review and compiler/test observation | Do not label unrelated equality checks as universally forbidden; exact ISO C wording required. |
| Fixed-column multidimensional pointer-to-array | Show an array-of-arrays parameter with a fixed column count and contrast it with `T **`. | Safe | Proposed C17 baseline | Compiler type check | No ragged allocation, cache, or ABI claim. |
| Explicit bounded-buffer API | Make length or begin/end explicit at the interface. | Safe | Proposed C17 baseline | Unit test and review targets selected later | No dynamic ownership or string API lesson. |

## 6. Planned exercise coverage (objectives only)

| Coverage source / future review | Objectives only | Current status and gap |
| --- | --- | --- |
| Session 05, Exercise 1 | Review an index before using it to select an element from a fixed table; make the table bound explicit. | `PARTIAL_EXERCISE_COVERAGE`; no import decision, solution, or rewritten exercise exists. |
| Session 04, Exercise 2 | Reinforce same-array membership, bounds validation, and pointer-range reasoning in a static object-pool context. | Related reinforcement only; primary owner `M03-T04-static-allocation-pools-and-determinism`; not a canonical M04-T01 exercise. |
| Beginner reinforcement | Determine whether an interface has an explicit extent and identify a decay/non-decay context. | No direct imported source; exercise design deferred. |
| Industrial review | Review a pointer-and-length API and identify an off-by-one or missing-bound contract. | No direct imported source; exercise design deferred. |
| Embedded bounded-buffer/debug case | Relate an observed bounds report to the API’s array-domain contract. | No direct imported source; tool and target assumptions deferred. |

The exercise artifact remains `NOT_STARTED`. Session 05 Exercise 1 is only a possible supplemental bounds exercise after separate rewrite and verification; it is not sufficient as the complete topic’s primary exercise. If a required artifact is later absent, `EXERCISE_PENDING` may block final topic approval; it is not an exercise source-coverage classification.

## 7. Proposed interview coverage (areas only)

Future interview material must be assessment-only and must not restate the lesson. Candidate areas are:

- distinguish array object, pointer object, pointer value, and pointee;
- explain decay/non-decay and parameter adjustment in a code-reading scenario;
- review a half-open range and identify a one-past dereference;
- reject unrelated-pointer subtraction or relational ordering;
- select a multidimensional parameter representation and state its layout contract;
- diagnose a bounds defect using the stated API contract and limited tool evidence;
- redirect ownership, `void *`, callbacks, aliasing, strings, and parsing questions to their canonical owners.

No interview questions, answers, or pack are created by this outline.

## 8. Granularity assessment

| Item | Assessment |
| --- | --- |
| Expected lesson size | Approximately 450–650 lines of primary lesson prose, subject to outline approval. Examples, references, exercises, and interview material are separate artifacts and do not justify a longer lesson. |
| Major concept groups | Ten concise planned sections, with arrays/conversion, bounds, and fixed-column multidimensional layout kept as the semantic core. Orientation and consolidation stay short. |
| Threshold risk | Low to medium: scope expands if pointer states, equality edge cases, or multidimensional variants become a second lesson. |
| Scope-control action | Keep pointer states operational, not a full ownership/lifetime lesson; keep multidimensional material fixed-column only; use one short advanced VLA/`static` note; link owners rather than adding `T **`, callbacks, aliasing, ragged allocation, cache claims, or string content. |
| Split policy | Do not split automatically. If review shows cognitive overload or a projected length above the warning threshold, recommend a split and stop for human decision. |

## 9. Proposed decisions for human review

1. Keep one concise fixed-column pointer-to-array section: array-of-arrays, fixed-column parameter type, contrast with `T **`, and one embedded image/sample-buffer example only.
2. Exclude pointer provenance from the primary lesson; retain it only as a future verification note.
3. Use fixed-column forms in the main lesson. VLA or `static` array parameters are a short optional advanced note, explicitly conditioned on ISO C verification and compiler/toolchain support; `static` is not automatic runtime bounds checking.
4. Retain approximately five planned example groups, with unsafe behavior isolated.
5. Treat Session 05 Exercise 1 as a possible supplemental bounds exercise only after rewrite and verification; it is not the complete topic’s primary exercise.

## 10. Required review gate

Human review required.

This topic must remain in OUTLINE_REVIEW.
No lesson, example, exercise, solution, interview, or next-topic work is authorised.
