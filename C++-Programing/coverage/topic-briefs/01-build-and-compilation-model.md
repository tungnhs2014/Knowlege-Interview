# Topic Brief 01 - Build And Compilation Model

## 1. Canonical Routing

| Field | Validated value |
| --- | --- |
| `learning_path_number` | `01` |
| Title | Build And Compilation Model |
| `slug` | `build-and-compilation-model` |
| Requested topic | Source-to-executable model, translation units, symbols, linking, loading, libraries, C/C++ linkage, and ABI |
| Master source | `master-ch01` |
| Required Notion sources | `notion-1-1`, `notion-2-3`, `notion-2-4`, `notion-10-2` |
| Topic Brief | `coverage/topic-briefs/01-build-and-compilation-model.md` |
| Knowledge target | `knowledge/01-build-and-compilation-model.md` |
| Interview target | `interview/01-build-and-compilation-model.md` |
| Example target | `examples/01-build-and-compilation-model/README.md` |

Validation result: the number, title, slug, master source, Notion routing, and
all canonical output paths match `LEARNING_PATH.md`.

## 2. Source Coverage

### Internal Sources Read

| Source label | Path | Coverage contribution |
| --- | --- | --- |
| `master-ch01` | `docs/MASTER_C_CPP_KNOWLEDGE_INDEX.md`, CH01 | Priority, prerequisites, keyword scope, depth, mandatory C/C++ linkage comparison, and interview focus |
| `guide-section-03` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 3 | MUST-depth requirements |
| `guide-section-04` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 4 | Full-chapter output structure |
| `guide-section-06` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 6 | Required comparison format, including macro vs inline |
| `guide-section-07` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 7 | Depth control and practical-example guidance |
| `guide-section-09` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 9 | Trusted-source routing |
| `guide-section-10` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 10 | English-first style, Markdown structure, and code-example rules |
| `guide-section-11` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 11 | Lesson-type expectations |
| `guide-section-12` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 12 | Required comparison inventory |
| `guide-section-14` | `docs/C_CPP_CHAPTER_EXPANSION_GUIDE.md`, section 14 | Final quality checklist |
| `notion-index` | `docs/C++ Notion/C++ Programming.md` | Notion chapter inventory |
| `notion-1-1` | `docs/C++ Notion/Chapter 1-1 Introduction & Environment Setup.md` | Compiler setup, program structure, four build stages, common flags, compile vs link errors |
| `notion-2-3` | `docs/C++ Notion/Chapter 2-3 Function Overloading & Name Mangling.md` | Symbols, name mangling, inspection tools, C/C++ linkage, `extern "C"`, mixed-language build |
| `notion-2-4` | `docs/C++ Notion/Chapter 2-4 Inline Functions.md` | Header definitions, inline semantics, ODR risks, code size, optimization, macro comparison |
| `notion-10-2` | `docs/C++ Notion/Chapter 10-2 Preprocessor Directives.md` | Preprocessing, includes, macros, conditional compilation, include guards, `#pragma once` |

No mapped Notion source was skipped.

### External References Consulted

| Label | Reference | Validation purpose |
| --- | --- | --- |
| `external-gcc-overall-options` | <https://gcc.gnu.org/onlinedocs/gcc/Overall-Options.html> | Exact GCC phase ordering and meanings of `-E`, `-S`, and `-c` |
| `external-gnu-ld` | <https://sourceware.org/binutils/docs/ld.html> | Symbol resolution, archive search order, shared-library options, `rpath`, `soname`, and linker scripts |
| `external-linux-ld-so` | <https://man7.org/linux/man-pages/man8/ld.so.8.html> | Dynamic loading and runtime shared-library search behavior |
| `external-itanium-cpp-abi` | <https://itanium-cxx-abi.github.io/cxx-abi/abi.html> | C++ ABI scope and common GCC/Clang mangling model |
| `external-cpp-draft-linkage` | <https://eel.is/c++draft/basic.link> | Translation units and language linkage model |
| `external-cpp-draft-odr` | <https://eel.is/c++draft/basic.def.odr> | One Definition Rule requirements across translation units |
| `external-cpp-draft-inline` | <https://eel.is/c++draft/dcl> | Exact semantic role of `inline`, separate from optimizer inlining |

External validation was necessary because the mapped Notion material does not
fully define static archive extraction, runtime loader search, `soname`, ABI
compatibility, or exact ODR/`inline` behavior.

### Coverage Status

`READY_FOR_EXPANSION`: canonical internal coverage is complete and the main
standard/compiler/linker gaps have identified primary references. This status
records source readiness; the learner-facing chapter has now been created and
verified separately.

## 3. Priority And Dependencies

- Priority: `MUST`
- Depth: Medium to Deep
- Prerequisites: None
- Role in learning path: foundation for every later topic that spans headers,
  multiple source files, libraries, debugging, or mixed C/C++ integration.
- Required teaching flow: mental model first, then build phases, translation
  units and symbols, static/dynamic linking, loading, linkage/ABI, failures,
  tools, and interview reasoning.

## 4. Merged Concept Map

### Core Build Model

- Source file and header file roles; a header is normally included into a
  source file rather than compiled as an independent program unit.
- Preprocessing creates the token stream for one translation unit by handling
  includes, macros, and conditional compilation.
- Compilation proper performs parsing, semantic checks, code generation, and
  optimization; assembly converts assembler input to a relocatable object file.
- Linking combines object files and selected library members, resolves symbol
  references, applies relocations, and emits an executable or shared object.
- Loading maps the executable and shared objects, performs runtime relocations
  and symbol binding as required, initializes the process, and transfers control
  through runtime startup code before `main`.

### Declarations, Definitions, Linkage, And Symbols

- A declaration introduces an entity and its type; a definition supplies the
  entity or function body/storage required by the program.
- Each separately processed source file produces a translation unit and usually
  one object file.
- Object files contain sections, symbol tables, relocation records, defined
  symbols, and unresolved references.
- Distinguish scope, storage duration, language linkage, and name linkage.
- Namespace-scope `static` functions/objects have internal linkage; ordinary
  non-inline function definitions commonly have external linkage.
- `undefined reference` and `multiple definition` are usually link-time
  symptoms, not preprocessing or syntax failures.

### Headers, Inline, And ODR

- Include guards and `#pragma once` prevent repeated inclusion inside one
  translation unit; they do not by themselves prevent the same non-inline
  external definition from appearing in multiple translation units.
- Headers should primarily expose declarations and definitions that are valid
  in multiple translation units, such as templates, class-body definitions,
  `inline` functions, and C++17 inline variables.
- In modern C++, `inline` has an ODR-related semantic role. Optimizer inlining
  is a separate compiler decision and can happen with or without the keyword.
- Multiple permitted definitions must satisfy ODR requirements; describing
  inline functions as simply "exempt from ODR" is inaccurate.

### Libraries And Runtime Loading

- A static library (`.a`) is an archive of object files. The linker extracts
  members needed to satisfy unresolved symbols; command-line order can matter.
- A shared library (`.so`) remains a runtime dependency. Link-time selection and
  runtime discovery are separate concerns.
- `-L` affects link-time library search; runtime discovery can involve
  `DT_RPATH`, `DT_RUNPATH`, `LD_LIBRARY_PATH`, loader cache, and default paths.
- `soname` identifies a shared library ABI generation independently of the
  concrete filename used during development or packaging.
- Visibility controls which symbols are exported from a shared object and helps
  define a deliberate ABI surface.

### C/C++ Linkage And ABI

- C++ name mangling encodes source-level information into linkable symbol names
  according to an implementation ABI; the exact scheme is not portable across
  all compilers and platforms.
- `extern "C"` specifies C language linkage for declarations. It does not make
  C++ implementation code valid C, guarantee a complete cross-language ABI, or
  solve incompatible data layout, calling convention, exception, allocation,
  or compiler-option boundaries.
- Mixed C/C++ headers should guard linkage specifications with `__cplusplus`.
- Stable C-facing APIs should use ABI-friendly types, explicit ownership rules,
  predictable error reporting, and no exceptions across the boundary.
- ABI compatibility includes calling convention, layout, alignment, mangling,
  standard-library/runtime choices, compiler options, and exception/RTTI model.

### Controlled Advanced Scope

- Mention but do not deeply expand symbol versioning, linker scripts, weak
  symbols, LTO, and PGO in the first lesson.
- Linker scripts are relevant to embedded memory placement, but no
  kernel-driver or Linux Device Driver material belongs in this topic.

## 5. Usage Angles

### C Usage

- `.c` plus `.h` organization, declaration/definition separation, internal
  linkage via `static`, external declarations via `extern`, static archives,
  linker ordering, and exposing stable C APIs.
- Embedded firmware builds may use startup objects, cross-compilers, map files,
  section placement, and linker scripts. Keep the discussion at build-system
  and executable-layout level.

### C++ Usage

- Multiple translation units, overloaded symbols, name mangling, ODR, inline
  functions/variables, templates in headers, visibility, C interop, and ABI
  compatibility.
- Explain that choosing `g++` for the final link normally supplies the C++
  runtime libraries that a raw `gcc` link may omit.

### Embedded Usage

- Cross-compilation from host to target, reproducible compiler flags, startup
  and library selection, code-size inspection, map-file reading, and controlled
  linker-script awareness.
- Build-time feature macros should be explicit and traceable; avoid hidden
  configuration differences between translation units.

### Enterprise Usage

- Public/private header boundaries, minimal exported symbols, ABI versioning,
  deterministic builds, warning policies, dependency tracking, and CI builds
  that compile all supported configurations.
- Library contracts must specify compiler/standard/runtime assumptions and
  ownership/error boundaries.

## 6. Required Comparisons

| Comparison | Required teaching point |
| --- | --- |
| Declaration vs definition | Type/interface introduction versus entity body or storage; connect mistakes to compiler and linker diagnostics |
| Preprocessing vs compilation vs assembly vs linking vs loading | Identify input, output, responsibility, and typical failure class at each phase |
| Header file vs source file vs translation unit | Physical files are not equivalent to the post-preprocessing translation unit |
| Compile error vs linker error vs loader error | Syntax/type/semantic failure versus unresolved/duplicate symbols versus missing/incompatible runtime dependency |
| Static library vs shared library | Archive extraction and binary inclusion versus runtime dependency, ABI, deployment, and loader search |
| Internal vs external linkage | Translation-unit-private names versus cross-unit symbol resolution |
| C linkage vs C++ linkage/name mangling | Stable C symbol naming versus implementation-defined C++ ABI encoding |
| `inline` keyword vs optimizer inlining | ODR semantics versus code-generation optimization |
| Include guard vs `#pragma once` | Standard macro technique versus widely supported implementation extension |
| Macro vs inline function vs `constexpr` | Text substitution versus typed function/ODR semantics versus constant-evaluation capability |
| `-L` vs `rpath`/`RUNPATH` | Link-time search path versus runtime loader search metadata |

## 7. Common Bugs And Failure Modes

- Header defines a non-inline external function or variable, causing `multiple
  definition` when included by several translation units.
- A function is declared but its defining object file or library is absent,
  producing `undefined reference`.
- Declaration and definition differ in namespace, parameter type, qualifiers,
  calling convention, or language linkage, so symbols do not match.
- A C header lacks the `__cplusplus`/`extern "C"` wrapper when consumed by C++.
- Static library appears before the object that needs it, so archive members are
  not extracted by a one-pass Unix-style linker search.
- C++ objects are linked with the C driver without the required C++ runtime
  libraries.
- Debug and release objects, incompatible architecture objects, or incompatible
  ABIs are mixed in one link.
- Shared library is found at link time but not by the runtime loader.
- Incorrect `rpath`/`RUNPATH`, `soname`, or symlink packaging loads the wrong ABI
  generation.
- Build macros or ABI-affecting flags differ across translation units.
- Header guard identifiers collide, silently suppressing an unrelated header.
- Macro side effects, missing parentheses, or name collisions alter the
  preprocessed program.
- ODR violation has no required diagnostic and manifests as build-dependent or
  optimization-dependent behavior.
- ABI boundary passes STL types, exceptions, ownership, or allocator-dependent
  objects across incompatible modules.

## 8. Debugging Notes

- Stop after each stage: `gcc/g++ -E`, `-S`, and `-c`.
- Enable verbose driver output with `-v` or inspect commands with `-###` where
  supported.
- Inspect symbols with `nm -C`, `readelf -Ws`, and `objdump -t`; use `c++filt`
  for isolated mangled names.
- Inspect sections and relocations with `readelf -S`, `readelf -r`, and
  `objdump -dr`.
- Trace linker input and resolution with a linker map (`-Wl,-Map,<file>`),
  `-Wl,--trace`, or the platform equivalent.
- Inspect dynamic dependencies and search metadata with `readelf -d`; use
  `ldd` only as a diagnostic convenience on trusted binaries.
- Diagnose runtime loading with `LD_DEBUG=libs,bindings` in a controlled
  environment.
- Compare exact compiler commands, target triples, standard versions, PIC/PIE
  settings, visibility, and ABI-affecting flags across all objects.
- Reproduce failures with two or three small translation units before blaming
  the build system.

## 9. Best Practices

- Put interfaces in headers and ordinary non-inline definitions in source files.
- Make header files self-contained and protected by a unique include guard or
  consistently used `#pragma once`.
- Prefer typed constants, inline functions, templates, or `constexpr` over
  function-like macros when conditional compilation is not required.
- Treat `inline` primarily as a language/ODR tool, not a performance command.
- Use the compiler driver for final links and keep object/library order
  intentional.
- Keep exported shared-library symbols minimal and version the ABI deliberately.
- Use a narrow C-compatible boundary for cross-language or plugin interfaces.
- Never allow C++ exceptions to cross a C ABI boundary.
- Make build configuration and ABI-affecting options consistent and observable.
- Generate and retain map files for size-sensitive embedded builds.
- Teach tools by connecting each one to a concrete question about symbols,
  sections, relocations, or dependencies.

## 10. Interview Angles

### Junior

- Describe the path from `.c`/`.cpp` to executable.
- What is a translation unit?
- Declaration and definition differ how?
- What causes `undefined reference` and `multiple definition`?
- Why use include guards?

### Middle

- Explain symbol resolution across object files and static libraries.
- Why can static library order change a link result?
- Why can a shared library link successfully but fail when the program starts?
- What is the real role of `inline` in C++?
- How do `nm`, `readelf`, and a linker map help isolate a build failure?

### Senior

- Design a stable C API over a C++ implementation.
- Explain what `extern "C"` guarantees and what it does not.
- Define ABI compatibility and list common ways a library upgrade breaks it.
- Compare visibility, `soname`, symbol versioning, and semantic versioning.
- Explain how LTO changes translation-unit optimization without removing ABI or
  ODR obligations.

## 11. Practice Tasks

- Basic: build one program manually through `.i`, `.s`, `.o`, and executable
  outputs, then identify what each stage added or removed.
- Basic: create one compile error, one linker error, and one loader error and
  classify each diagnostic.
- Intermediate: split a function across header and two source files, trigger and
  repair both `undefined reference` and `multiple definition`.
- Intermediate: create a static library and demonstrate why link order matters.
- Intermediate: compile a C library and call it from C++ through a guarded
  `extern "C"` header; inspect symbols before and after.
- Advanced: build a shared library with controlled visibility and `soname`,
  inspect `DT_NEEDED`/`RUNPATH`, and diagnose a deliberate runtime lookup
  failure.
- Advanced embedded: generate a linker map, identify the largest sections and
  symbols, and explain what belongs in a later linker-script deep dive.

## 12. Gaps, Corrections, And Uncertainties

- The Notion compilation overview omits translation units, relocations, loader
  startup, archive extraction rules, and most static/shared-library mechanics.
- The Notion inline chapter overstates performance-oriented rules. Loops,
  recursion, virtual dispatch, static locals, or taking an address do not create
  universal language rules that forbid optimizer inlining.
- "Inline functions are exempt from ODR" must be corrected to: C++ permits
  multiple definitions under specific ODR conditions.
- Template functions are not automatically `inline` merely because they are
  templates. Their definitions are commonly placed in headers so
  specializations can be instantiated, and template ODR rules permit the
  necessary repeated definitions.
- `extern "C"` is broader than "turn off name mangling" as a language model,
  but it is not a complete ABI adapter.
- Exact mangled spelling must always be presented as compiler/ABI-specific.
- The initial learner-facing lesson should focus on ELF/GCC/Clang examples while
  labeling platform-specific behavior; MSVC/COFF differences can be a bounded
  comparison rather than a second full toolchain.
- Linker scripts, weak symbols, symbol versioning, LTO, and PGO remain controlled
  advanced extensions, not core first-pass detail.
- No C-only safety source, POSIX API source, or kernel-driver source is required
  for this topic.

## 13. Output Targets

| Output | Status after this step | Intended scope |
| --- | --- | --- |
| `coverage/topic-briefs/01-build-and-compilation-model.md` | Created | Audit metadata and expansion plan |
| `knowledge/01-build-and-compilation-model.md` | Created | MUST-depth learner-facing lesson |
| `interview/01-build-and-compilation-model.md` | Created | Junior/Middle/Senior interview pack |
| `examples/01-build-and-compilation-model/README.md` | Created | Compile/link/load experiments and inspection commands |

Chapter 01 is complete for the current scope: Topic Brief, knowledge lesson, and
interview pack, plus compile-ready examples.
