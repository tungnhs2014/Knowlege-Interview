# M04-T01 — Source Inventory

## 1. Topic identity and review state

| Field | Value |
| --- | --- |
| Topic ID | `M04-T01` |
| Slug | `pointers-arrays-decay-and-bounds` |
| Module | `M04 — Advanced Pointers and Object Representation` |
| Required prerequisites | `M01-T02-abstract-machine-and-behavior`; `M01-T03-integer-conversions-and-floating-point`; `M02-T01-scope-linkage-and-storage-duration` |
| Pre-Training assumed | Basic pointer syntax, basic arrays, functions, and basic command-line build use |
| Topic lifecycle | `OUTLINE_REVIEW` |
| Source inventory artifact | `READY_FOR_HUMAN_REVIEW` |
| Lesson artifact status | `NOT_STARTED` |
| Examples artifact status | `NOT_STARTED` |
| Exercise artifact status | `NOT_STARTED` |
| Interview artifact status | `NOT_STARTED` |
| Technical-review status | `NOT_STARTED` |
| Human review | `PENDING` |
| Exercise source coverage | `PARTIAL_EXERCISE_COVERAGE` |

This is an inventory, not a lesson. Its findings may be used to prepare an outline, but no claim becomes canonical without the required standards, compiler, or target verification.

## 2. Approved scope

The future topic is an advanced treatment of pointer and array semantics. It may cover the following coherent concept family:

- pointer object, pointer value, and pointed-to object;
- operational pointer states: valid, one-past, null, indeterminate/uninitialised, dangling, and invalid access attempts;
- array objects, array-to-pointer conversion (decay), and the contexts where that conversion does not occur;
- distinctions between an array object and a pointer object, including `sizeof` and address/type consequences;
- pointer arithmetic, the one-past position, equality, relational comparison, and subtraction within their applicable array-domain rules;
- function parameter adjustment and the loss of caller array extent; explicit bounds contracts;
- multidimensional arrays and the need for pointer-to-array parameter types;
- defects at boundaries and the appropriate use of compiler diagnostics and memory-safety tools.

This scope does not treat the list as separate lesson chapters. The outline must retain a single semantic progression and must not use examples, exercises, or interview material to expand the scope.

## 3. Explicit exclusions and canonical owners

| Excluded concept | Canonical owner | Inventory disposition |
| --- | --- | --- |
| Dynamic allocation, ownership transfer, cleanup, and allocation failure | `M03-T03-dynamic-memory-ownership-and-cleanup` | Do not use allocation as a prerequisite or main example. A dangling-pointer mention is limited to state recognition and links to this owner. |
| Double pointers, output parameters, `void *`, and generic APIs | `M04-T02-double-pointers-void-pointers-and-generic-apis` | Exclude legacy `T **` API material. |
| Function-pointer declarations and callbacks | `M04-T03-function-pointers-callbacks-and-declarations` | Session 05 Exercise 2 and dispatch tables are not M04-T01 material. |
| Effective type, strict aliasing, alignment, and object representation | `M04-T04-object-representation-alignment-effective-type-and-aliasing` | Do not explain typed aliasing or type-punning here. |
| Complete string lifetime and string APIs | `M05-T02-character-arrays-string-literals-and-lifetime` | String-literal tables may appear only as a bounded-array application, without teaching string semantics. |
| Byte encoding and protocol parsing | `M05-T03-memory-operations-byte-encoding-and-safe-parsing` | Do not cast packet buffers or teach serialization. |
| Pointer provenance | Later standards-verification decision | Do not speculate or present unsettled model details as C17 rules. Record a future verification question only. |

## 4. Source mapping

`High`, `Medium`, and `Low` describe usefulness for discovery and migration, not technical authority. All exact language-semantics claims still require ISO C verification.

| Source ID | Exact repo-relative path | Exact heading | Relevant subsection/range | Covered concept | Intended use | Migration labels | Confidence | Verification required | Issues / disposition |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ARCH-M04 | `C-Programming/ARCHITECTURE.md` | `### M04 — Advanced Pointers and Object Representation` | M04-T01 catalog row and §11 prerequisite row | Approved scope, owners, prerequisites | Binding architecture boundary | `KEEP`, `VERIFY` | High | Architecture consistency | Records scope; it is not a language-semantics authority. |
| L04-A | `C++-Programing/knowledge/04-pointer-mastery.md` | `## 3 Mental Model: Pointer Object, Value, And Pointee` | `### 3.1`; `### 3.2` | Pointer object/value/pointee and state vocabulary | Discovery for the mental-model portion | `MOVE_C`, `SPLIT`, `REWRITE`, `VERIFY` | Medium | ISO C; pedagogical review | Validity language must distinguish a pointer value from permission to access an object. |
| L04-B | `C++-Programing/knowledge/04-pointer-mastery.md` | `## 6 Pointers And Arrays` | `### 6.1` through `### 6.3` | Arrays are not pointers; decay and non-decay; parameter adjustment | Core migration input | `KEEP`, `MOVE_C`, `SPLIT`, `VERIFY` | High | ISO C; compiler examples | Must not imply that an array parameter retains the full caller array type or extent. |
| L04-C | `C++-Programing/knowledge/04-pointer-mastery.md` | `## 7 Pointer Arithmetic And One-Past Pointers` | `### 7.1` through `### 7.4` | Element-scaled arithmetic, one-past, subtraction, equality, relational comparison | Core migration input | `KEEP`, `MOVE_C`, `SPLIT`, `VERIFY` | High | ISO C; compiler diagnostics | Verify equality separately from relational ordering; distinguish forming/permitted use of one-past from dereferencing it. |
| L04-D | `C++-Programing/knowledge/04-pointer-mastery.md` | `## 13 Passing Data Through Pointers` | `### 13.4 Bounded buffer interfaces` | Pointer-plus-length contracts | Application pattern only | `MOVE_C`, `SPLIT`, `REWRITE`, `VERIFY` | Medium | ISO C; API-design review | Keep bounds ownership explicit; no claim that a pointer carries bounds. |
| L04-E | `C++-Programing/knowledge/04-pointer-mastery.md` | `## 15 Dynamic Allocation Boundaries`; `## 21 Controlled Advanced Topics` | Dynamic allocation and provenance material | Ownership and provenance | Exclusion evidence | `SPLIT`, `MOVE_C`, `ARCHIVE`, `VERIFY` | Medium | ISO C; later standards review | Allocation belongs to M03-T03; provenance is deliberately not taught here. |
| L05 | `C++-Programing/knowledge/05-compound-types-in-c.md` | `## 4 Arrays` | `### 4.1` through `### 4.6` | Array object, conversion, explicit extent, adjusted parameters, multidimensional arrays | Supporting migration input | `MOVE_C`, `SPLIT`, `REWRITE`, `VERIFY` | High | ISO C; compiler examples | Move semantic ownership to M04-T01; retain strings and compound-type material with their owners. |
| L02-L03 | `C++-Programing/knowledge/02-c-fundamentals.md`; `C++-Programing/knowledge/03-c-memory-model.md` | `## 5 Statements, Expressions, And Objects`; `## 10 Behavior Categories`; `## 11 Common Memory Bugs`; `## 14 Debugging Workflow` | Uninitialised objects, behavior categories, bounds defects, diagnostics | Prerequisite and defect/tool context | `SPLIT`, `MOVE_C`, `VERIFY` | Medium | ISO C; official GCC/Clang/tool docs | Do not repeat M01 behavior taxonomy or M03 ownership/lifetime lessons. |
| B04 | `C++-Programing/coverage/topic-briefs/04-pointer-mastery.md` | `## 4 Scope And Depth Boundaries`; `## 5 Merged Concept Map`; `## 13 Source Corrections And Gaps` | Arrays/decay/bounds and corrections | Conflict and migration evidence | `ARCHIVE`, `SPLIT`, `VERIFY`, `DELETE_DUPLICATE` | Medium | Pedagogical review | Broad brief mixes callbacks, ownership, aliasing, and C++ comparisons; it cannot be copied as a topic. |
| E04 | `C++-Programing/examples/04-pointer-mastery/README.md` | `## 1 Pointer And Array Mechanics`; `## Example Map` | References `basics/pointer_array.c`; `matrix/matrix_layout.c`; `unsafe/one_past_write.c` | Candidate demonstrations: `sizeof`, pointer-to-array, one-past error, matrices | Example inventory index | `MOVE_C`, `SPLIT`, `REWRITE`, `VERIFY` | Medium | ISO C; compiler; sanitizer verification | Index is not evidence that examples build or are correct; actual source files were separately inspected below. |
| E04-A | `C++-Programing/examples/04-pointer-mastery/basics/pointer_array.c` | File-level program; `print_range`; `main` | Lines 4–35 | Half-open traversal, array-to-pointer conversion, pointer-to-array, local `sizeof` contrast, qualifier declarations | Candidate example evidence only | `MOVE_C`, `SPLIT`, `REWRITE`, `VERIFY` | Medium | ISO C; local compiler; sanitizer review | Local GCC C17 with `-Wall -Wextra -pedantic -Werror` built and ran successfully. Its traversal is defined only when `begin` and `end` describe a valid ordered range in the same array; that precondition is not documented. Qualifier semantics are M02-T02 scope, so they are a scope leak for the final example. |
| E04-B | `C++-Programing/examples/04-pointer-mastery/matrix/matrix_layout.c` | File-level program; `print_contiguous`; `print_pointer_array`; `main` | Lines 4–45 | Fixed-column array-of-arrays, pointer-to-array, contrast with an array of pointers | Candidate example evidence only | `MOVE_C`, `SPLIT`, `REWRITE`, `VERIFY` | Medium | ISO C; local compiler; compiler diagnostics | Local GCC C17 with `-Wall -Wextra -pedantic -Werror` built and ran successfully. The fixed-column operations are defined for the supplied compatible arrays; the ragged array-of-pointers path and its non-null/row-length assumptions are beyond this topic’s concise multidimensional scope. No ABI or performance conclusion follows from the run. |
| E04-C | `C++-Programing/examples/04-pointer-mastery/unsafe/one_past_write.c` | File-level program; `main` | Lines 3–10 | Forming a one-past pointer and an intentional one-past write | Isolated negative-example evidence only | `MOVE_C`, `REWRITE`, `VERIFY` | High | ISO C; local compiler; AddressSanitizer verification | Local GCC C17 with `-Wall -Wextra -pedantic -Werror` built; an AddressSanitizer build also succeeded, but execution was intentionally skipped. Forming `values + 4` is the planned one-past case; line 8 dereferences it and is intentionally undefined behavior. It must never be adopted as ordinary example code. |
| I04-I05 | `C++-Programing/interview/04-pointer-mastery.md`; `C++-Programing/interview/05-compound-types-in-c.md` | `### 3. Why is an array not a pointer, and what is array-to-pointer conversion?`; `### 6. What are the exact limits of pointer arithmetic and comparison?`; `### 6. Why does sizeof fail to recover an array's length inside a function?`; `### 7. How should a multidimensional array be passed to a function?` | Named question sections | Assessment-topic discovery and duplication audit | `SPLIT`, `MOVE_C`, `MOVE_COMPARISON`, `DELETE_DUPLICATE`, `ARCHIVE` | Medium | ISO C; pedagogical review | Future interview material assesses the canonical lesson; it must not duplicate it or retain C++ content. |
| D0-D1 | `C++-Programing/docs/C Advanced/C advaced outline devlinux.txt`; `C++-Programing/docs/C Advanced/C Advacne DevLinux.txt` | `Advanced Pointers & Embedded Design Patterns (Practical)`; `Day 5: Arrays of Pointers & Function Pointers.` | Pointer, double-pointer, function-pointer, and ragged-array references | Topic discovery and source separation | `SPLIT`, `MOVE_C`, `MOVE_CPP`, `REWRITE`, `VERIFY` | Low | ISO C; compiler; target; pedagogical review | These plans combine M04-T01, M04-T02, M04-T03, M05-T02, and target-specific placement. |
| S04 | `C++-Programing/docs/C Advanced/session-04.md` | `## Exercise_2 [build]`; `**The Object Pool Allocator**` | Requirements 2–4; Rules; coding-standard references | Static pool membership, bounds validation, pointer range reasoning | Related exercise reinforcement only | `SPLIT`, `MOVE_C`, `REWRITE`, `VERIFY` | Medium | ISO C; compiler; target/ELF; exercise and standards-source review | Primary owner is M03-T04. It is not the canonical M04-T01 exercise; source claims about `.bss`, Flash/RAM, O(1), and named projects require target-specific verification. |
| S05 | `C++-Programing/docs/C Advanced/session-05.md` | `## Exercise_1 [build]` | `Error Message Table (Array of String Pointers)` | Indexed table, explicit invalid-index check, array-of-pointers use | Exercise-source mapping only | `MOVE_C`, `SPLIT`, `REWRITE`, `VERIFY` | Medium | ISO C; compiler; exercise review; standards-source review | Partial coverage only; its string and enum details are not this topic’s full scope. |
| EN | `C++-Programing/docs/C Advanced/Full-Embedded-C-Notes.md` | `### 82. Passing Struct and Array to Functions`; `### 77. Dangling Pointer`; `### 88. Pointers in C — Complete Guide` | Table-of-contents discovery entries | Idea and gap discovery only | `ARCHIVE`, `SPLIT`, `REWRITE`, `VERIFY` | Low | ISO C; compiler; pedagogical review | Non-authoritative notes; not a substitute for missing `note.md`, and not a source to copy. |

No repository file named `note.md` was found. `C++-Programing/docs/C Advanced/Full-Embedded-C-Notes.md` remains a separate, non-authoritative discovery input; it is not silently treated as `note.md`.

## 5. Duplicate and conflict analysis

| Finding | Sources | Risk | Resolution for M04-T01 |
| --- | --- | --- | --- |
| Pointer, array/decay, and bug explanations are repeated as lesson, brief, examples, and interview answers. | L04, L05, B04, E04, I04-I05 | Competing definitions and an oversized lesson/interview pack | Establish one canonical lesson later; examples demonstrate one claim, and interview material assesses it without restating it. |
| Legacy pointer scope mixes allocation/ownership, `T **`, `void *`, callbacks, aliasing, and C++ comparisons with array semantics. | L04, B04, I04, D0-D1 | Scope leak into M03-T03, M04-T02–T04, M05-T02, and comparison work | Keep only advanced pointer/array/bounds material; link excluded concepts to their canonical owners. |
| “Array name is a pointer” is a common shortcut in broad material. | L04, B04, I04-I05, EN | Incorrect object/type/`sizeof` mental model | State instead that an array object can undergo array-to-pointer conversion in specified expression contexts; verify exact contexts against ISO C. |
| One-past positions, subtraction, equality, and relational comparisons can be simplified as general address arithmetic. | L04, I04, E04 | Out-of-bounds or unrelated-object reasoning | Limit planned claims and examples to their standards-defined array domain; never dereference one-past, never subtract/order arbitrary pointers, and do not describe all equality checks on unrelated pointers as forbidden. |
| A pointer may be described as enough to establish a buffer’s length or access validity. | L04-D, S05 | Hidden bounds contracts and unsafe APIs | Use explicit length/capacity contracts; no pointer is presented as carrying bounds. |
| Session 05 says a two-dimensional character array “wastes memory padding” and implies an array of pointers saves RAM. | S05 | Technically misleading, implementation-dependent storage claim; accidental string lesson | `REWRITE` required. Array elements have no inter-element padding; fixed-width rows can have unused capacity, which is not padding. An array of pointers consumes pointer-sized entries, and relative storage depends on pointer width, string lengths, compiler, linker, and target. ISO C does not guarantee `const` objects or string literals reside in Flash. Retain only the bounds-check input pending target-specific verification and M05-T02 review. |
| Exercise standards references and analyzer expectations are asserted without their controlling documents/build environment. | S05 | Unverifiable MISRA/CERT claims and false build guarantees | Treat as review requirements; verify licensed standards references, tool configuration, compiler, and target before reuse. |

## 6. Existing exercise-source mapping

### 6.1 Directly relevant source

| Source | Relevant outcomes it can support | Gaps | Assumptions and review items | Solution leakage | Recommended source coverage |
| --- | --- | --- | --- | --- | --- |
| `C++-Programing/docs/C Advanced/session-05.md` — `## Exercise_1 [build]`, `Error Message Table (Array of String Pointers)` | Validate an externally supplied index before table access; relate array indexing to an explicit bound; recognise an array of pointers to string literals. | Does not directly exercise decay/non-decay, parameter adjustment, `sizeof` loss, one-past formation, subtraction/comparison domain, multidimensional pointer-to-array types, or tool-observed defects. | Requests C99, `-Wall -Wextra -pedantic -Werror`, `cppcheck`, `clang-tidy`, Doxygen, and Makefile targets; none of those tool versions, configurations, target, or acceptance environment has been verified in this inventory. Review the BARR-C/MISRA/CERT claims against authoritative/licensed sources. | The optional design hint exposes an enum and table structure, but not a complete submitted program. A future exercise statement must avoid embedding a solution. | `PARTIAL_EXERCISE_COVERAGE` |

`## Exercise_2 [build]` in the same session concerns an array of function pointers, custom-section placement, and dispatch. It is mapped to `M04-T03` (and target/build topics), not to M04-T01.

| Source | Primary owner | M04-T01 relevance | Role | Coverage | Disposition |
| --- | --- | --- | --- | --- | --- |
| `C++-Programing/docs/C Advanced/session-04.md` — `## Exercise_2 [build]`, `The Object Pool Allocator` | `M03-T04-static-allocation-pools-and-determinism` | Same-array membership, bounds validation, and pointer-range reasoning | Related reinforcement only | Partial | Do not import or rewrite it for M04-T01; review its ISO C, target/ELF, build, and MISRA/CERT claims under its primary owner. |

### 6.2 Exercise coverage policy for this topic

- Source coverage and exercise-artifact workflow are independent. The source coverage is `PARTIAL_EXERCISE_COVERAGE`; the exercise artifact remains `NOT_STARTED`.
- `EXERCISE_PENDING` may later block final topic approval if a required exercise artifact is not ready. It is not a source-coverage value.
- Session 05 Exercise 1 is only a possible supplemental bounds exercise after a separate `REWRITE` and verification. It is not sufficient as the primary exercise for the complete topic.
- A future topic-level exercise review must evaluate technical correctness, scope, prerequisites, build requirements, MISRA/CERT claims, compiler and target assumptions, acceptance criteria, and solution leakage before importing any session content.
- No new exercise is designed or created by this inventory.

## 7. Gaps and authorities to consult later

| Gap or question | Required authority category | Planned action |
| --- | --- | --- |
| Exact C17 rules for array-to-pointer conversion and its non-conversion contexts | ISO C17 or corresponding public draft | Verify wording before lesson claims and examples. |
| Exact domains for pointer arithmetic, a non-array object treated as a one-element array where applicable, null operands, one-past use, subscript notation, subtraction, equality, and relational comparison | ISO C17 or corresponding public draft | Verify separately; do not generalise from a particular machine’s addresses. Confirm subtraction result type and representability requirements. |
| Parameter adjustment, multidimensional parameters, qualifiers, VLA parameter syntax, and optional `static` contracts | ISO C17 or corresponding public draft; compiler documentation | Use fixed-column forms in the main lesson. Treat VLA/`static` parameter syntax as a short optional advanced note; do not imply `static` provides automatic runtime bounds checking. |
| Diagnostics and sanitizer behavior for bounds defects | Official GCC and Clang documentation; sanitizer documentation | Select commands and expected observations only after tool/version review. |
| Secure array/pointer guidance | CERT C array and pointer/bounds guidance | Use as supplementary guidance, not as a replacement for ISO C semantics. |
| Pointer sizes, unusual address spaces, and target memory models | ABI, compiler, and MCU/CPU documentation | Do not make platform assumptions in ISO C examples. |
| Pointer provenance | Later standards-verification decision | Keep out of lesson scope unless the architecture is explicitly amended after review. |

## 8. Inventory result and gate

The inventory supports a bounded M04-T01 outline. It does not provide complete exercise coverage, and it identifies standards-sensitive claims requiring later verification. The next permitted deliverable is an outline for human review; neither this inventory nor completion of that outline authorises a lesson.

Human review required.

This topic must remain in OUTLINE_REVIEW.
No lesson, example, exercise, solution, interview, or next-topic work is authorised.
