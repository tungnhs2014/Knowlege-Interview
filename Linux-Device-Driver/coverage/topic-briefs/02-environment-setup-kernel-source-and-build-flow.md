# Topic Brief - 02 - Environment Setup, Kernel Source, And Build Flow

## Topic Identity
- Learning-path number: `02`
- Slug: `environment-setup-kernel-source-and-build-flow`
- Primary scope: host and target setup, selecting and obtaining the correct
  kernel tree, build prerequisites, source-tree orientation, Kconfig and
  `.config`, native and cross-compilation, Kbuild outputs, staged installation,
  external-module build prerequisites, and practical build diagnosis.
- Related topics:
  `01-linux-kernel-and-driver-development-overview`,
  `03-kernel-modules-fundamentals`,
  `04-kernel-logging-error-handling-and-coding-practice`,
  `10-device-tree-fundamentals`, and
  `37-kernel-debugging-and-tracing`.

## Output Targets
- Knowledge: `knowledge/02-environment-setup-kernel-source-and-build-flow.md`
- Interview: `interview/02-environment-setup-kernel-source-and-build-flow.md`
- Example:
  `examples/02-environment-setup-kernel-source-and-build-flow/README.md`

## Source Coverage

### Direct Sources
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch01` | `docs/Linux Device Driver Development/Chapter 1-Introduction to Kernel Development .md` | read/covered/merged | Debian-era package setup, source acquisition, source-tree layout, defconfig and `.config`, native versus ARM cross-build commands, image/module/DTB targets, and `INSTALL_MOD_PATH`. |
| `ldd1-ch02` | `docs/Linux Device Driver Development/Chapter 2-Device Driver Basis.md` | read/covered/merged-partial | Kbuild `obj-y`/`obj-m` and `obj-$(CONFIG_*)`, in-tree Kconfig/Makefile integration, external-module wrapper Makefile, distribution kernel headers, `M=`, module outputs, and module cross-compilation. Module loading, parameters, metadata, and lifecycle map primarily to topic 03. |
| `notion-ch01-part1` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 1 Environment Setup and Getting Source Code.md` | read/covered/merged | Host/target mental model, current-looking package/toolchain examples, workspace organization, mainline/stable/longterm/vendor tree choices, Git/tarball/shallow acquisition, source-tree navigation, internal versus UAPI headers, and documentation discovery. |
| `notion-ch01-part2` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 2 Kernel Configuration.md` | read/covered/merged | Kconfig purpose and dependencies, `y`/`m`/`n`, configuration frontends, `ARCH`, `CROSS_COMPILE`, defconfig workflow, `savedefconfig`, search/help, and configuration profiles. |
| `notion-ch01-part3` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 3 Building the Kernel.md` | read/covered/merged | Kbuild flow, native and cross-build sequences, kernel/module/DTB artifacts, target staging and deployment, parallel and incremental builds, verbose output, installation checks, clean targets, and troubleshooting examples. |
| `notion-ch02-part4` | `docs/Linux-Device-Driver-Notion/Chapter 2-Part 4 Module Parameters & Building Your First Mod.md` | read/covered/merged-partial | Independent external-module and in-tree build explanations, single- and multi-file Kbuild syntax, `KERNELDIR`, `M=`, cross-compilation, generated artifacts, installation, and version-mismatch troubleshooting. Parameter declarations and module testing belong to topic 03. |

### Supporting And Inventory Sources
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd2-ch14` | `docs/Linux Device Driver Development 2/Chapter 14-Linux_Kernel_Debugging_Tips.md` | read/mapped/merged-partial | Independently explains the mainline, release-candidate, stable, and longterm trees; identifies official kernel.org repositories; and reinforces selecting an explicit kernel source baseline. Logging, tracing, and oops analysis map to topics 04 and 37. |
| `ldd2-ch01` | `docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md` | read/mapped/out-of-scope | Inspected independently because its chapter number appears relevant. Its actual content is locking, waits, deferred work, and IRQ handling, so it maps to topics 05, 06, and 15 rather than topic 02. |
| `notion-ch01-part4` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 4 Coding Style and Best Practices .md` | read/mapped/out-of-scope | Inspected independently. Its coding style, code organization, memory-allocation, operations-structure, kobject, and reference-counting material maps primarily to topics 01, 04, 05, 12, and 20 rather than topic 02. |
| `notion-ch01-extra` | `docs/Linux-Device-Driver-Notion/Linux Device Drivers.md` | read/mapped/index-only | Confirms the Notion Chapter 1 Parts 1-4 and Chapter 2 Part 4 inventory. It contains navigation metadata, not an independent build explanation. |

## Source Files Read
- `ldd1-ch01` was read in full. Relevant sections are Environment setup,
  Getting the sources, Source organization, Kernel configuration, Building
  your kernel, and Summary. Coding-style and object-model sections map to
  topics 01 and 04.
- `ldd1-ch02` was read independently. The complete file was inspected; the
  directly relevant range begins at Building your first module and includes
  The module's makefile, In the kernel tree, Out of the tree, Building the
  module, cross-compilation, and Summary.
- `notion-ch01-part1` was read in full, including development tools,
  cross-toolchain naming, workspace layout, release-tree choices, all three
  acquisition methods, download verification, source organization, and source
  navigation.
- `notion-ch01-part2` was read in full, including Kconfig semantics,
  configuration frontends, `ARCH`/`CROSS_COMPILE`, defconfigs,
  `savedefconfig`, common options, and use-case profiles.
- `notion-ch01-part3` was read in full, including Kbuild, native and ARM build
  flows, artifacts, installation/deployment, incremental and verbose builds,
  troubleshooting, cleaning, and its example build script.
- `notion-ch02-part4` was read in full rather than treated as a duplicate of
  `ldd1-ch02`. Its module-parameter half maps to topic 03; its Kbuild,
  external-module, multi-file, in-tree, installation, and troubleshooting
  sections contribute here.
- `ldd2-ch14` was read in full. Only its release-process and repository
  passages contribute to topic 02. Its similarly numbered source identity
  does not imply equivalence with any `ldd1` or Notion chapter.
- `ldd2-ch01` was inspected independently rather than inferred from its
  number/title. It contains synchronization, deferred-work, and interrupt
  mechanisms, not environment or Kbuild teaching, and is recorded as out of
  scope.
- `notion-ch01-part4` was read in full. Its coding-style rules, comments,
  organization, `checkpatch.pl`, memory-allocation overview, operations
  structures, kobjects, and reference-counting material map to topics 01, 04,
  05, 12, and 20 rather than topic 02.
- `notion-ch01-extra` was read in full and classified as index-only.
- Filenames and build-related headings/terms were scanned across all three
  source roots. Other matches are incidental subsystem build commands or
  chapter-specific technical requirements; they do not add a general
  environment, configuration, kernel build, or external-module build
  mechanism.

## Merged Source Notes
- Start with a two-machine mental model:
  - The **host** runs editors, Git, Kconfig frontends, compilers, linkers, and
    build scripts.
  - The **target** runs the resulting kernel image, DTBs, modules, and root
    filesystem.
  - A native build has compatible host and target architectures; a
    cross-build selects a target architecture and toolchain explicitly.
- Treat the target's kernel as a build contract, not merely a version number.
  The relevant identity includes the exact source/vendor tree and commit or
  release, configuration, generated headers, compiler/build choices,
  `CONFIG_LOCALVERSION`, symbol/version metadata, and target ABI constraints.
- Merge source acquisition into a decision rather than a memorized command:
  - Use an official release archive for a fixed released baseline.
  - Use the mainline Git tree for upstream development.
  - Use the stable tree for maintained release branches.
  - Use the product's vendor/BSP tree when the target depends on unmerged
    platform support, while recording the downstream commit and patch set.
- Source authenticity is separate from source version. Reading the top-level
  Makefile identifies a declared version but does not authenticate the tree.
  Release archives should be verified with kernel.org signatures; Git work
  should record and, where appropriate, verify the selected tag/commit.
- Organize source-tree teaching around build ownership:
  - Top-level `Makefile`, architecture Makefiles, distributed Kbuild/Makefiles,
    Kconfig files, `.config`, and `scripts/` cooperate to select and compile
    objects.
  - `arch/` supplies architecture-specific code, image rules, defconfigs, and
    DT build paths.
  - `drivers/` contains subsystem drivers and their local Kconfig/Kbuild files.
  - `include/linux/` is primarily internal kernel API, while `include/uapi/`
    contributes exported userspace ABI.
  - `Documentation/`, `MAINTAINERS`, and neighboring drivers are part of the
    normal development environment, not optional reading.
- Present configuration as:
  `Kconfig definitions + dependencies + selected baseline -> .config`.
  A defconfig is a maintained starting point, while `.config` is the expanded
  build configuration. `make savedefconfig` creates a minimized representation
  of choices that differ from defaults.
- Preserve the useful `y`/`m`/`n` model:
  - `y` links selected code into built-in objects and ultimately the kernel.
  - `m` produces a loadable `.ko` when the symbol is tristate and module support
    and dependencies permit it.
  - `n` omits the selected code.
  Dependencies and symbol type mean not every option supports all three states.
- Teach `ARCH` as Kbuild's target architecture selector and `CROSS_COMPILE` as
  a tool-prefix selector. They are not inferred reliably from the board name.
  Native builds commonly omit them; cross-builds should keep them consistent
  across configuration, kernel, module, and DTB commands.
- Add current compiler context absent from the books: GCC/binutils is not the
  only supported path. Kbuild also supports Clang/LLVM through `LLVM=1`;
  exact support and minimum versions must be checked against the selected
  kernel tree.
- Prefer a separate output directory in the learner-facing workflow:
  `make O=<build-dir> ...`. It keeps generated files and `.config` out of the
  source tree, permits multiple configurations from one source checkout, and
  reduces architecture/configuration contamination. The same `O=` value must
  be used for subsequent commands.
- Merge the full build flow as:
  1. Identify target hardware, boot method, root filesystem, and exact kernel
     baseline.
  2. Install host tools and the appropriate native or cross toolchain.
  3. Obtain and identify the source tree.
  4. Create/select a clean output directory.
  5. Set `ARCH`, compiler selection, and cross prefix consistently.
  6. Load a known defconfig or existing product `.config`.
  7. Reconcile/customize configuration with `olddefconfig`, `menuconfig`, or
     another supported frontend.
  8. Build the architecture's kernel image target plus required modules and
     DTBs.
  9. Stage modules with `INSTALL_MOD_PATH`; preserve the matching
     `/lib/modules/$(make kernelrelease)` tree.
  10. Deploy the image, matching DTB, modules, and any required initramfs or
      bootloader metadata.
  11. Boot and verify the running release, boot log, configuration/artifacts,
      and module compatibility.
- Name output artifacts by role, not one architecture's filename:
  - `vmlinux`: ELF kernel image with symbols, useful for debugging.
  - Architecture boot image: for example `bzImage`, `zImage`, or `Image`.
  - `System.map`: symbol-address map from the build.
  - `.dtb`: compiled hardware description where the platform uses Device Tree.
  - `.ko`: loadable module.
  - `Module.symvers`: exported-symbol and optional modversion CRC metadata.
  - `modules.order` and `modules.builtin*`: module ordering and built-in
    metadata consumed by module tooling.
- Merge in-tree driver build integration from both Chapter 2 sources:
  a Kconfig symbol controls selection and
  `obj-$(CONFIG_DRIVER) += driver.o` connects the selection to Kbuild. Parent
  Makefiles/Kconfig files recurse into the correct subsystem directory.
- Merge external-module build integration around the kernel build directory:
  `make -C <kernel-build-dir> M=$PWD`. `M=` marks the external module source
  directory; Kbuild supplies the target kernel's generated headers, compiler
  flags, configuration, and symbol processing.
- Distinguish source, exported userspace headers, and a prepared kernel build
  tree. Generic headers alone are not enough to reproduce the target kernel's
  module build context. Distribution `linux-headers-*` packages usually
  provide a prepared build tree for that exact distribution kernel.
- Keep `make headers_install` separate from module development: it exports
  sanitized UAPI headers for userspace programs and libc/toolchain consumers;
  it does not prepare a tree for building kernel modules.
- Correct the books' full-build requirement for external modules:
  `modules_prepare` can prepare a configured source/output tree for many
  external builds, but it does not generate `Module.symvers`; a full matching
  kernel build is required when `CONFIG_MODVERSIONS` metadata is needed.
- Keep load/unload commands, module parameters, dependencies, aliases, and
  init/exit lifecycle in topic 03. Topic 02 may build and inspect a `.ko` only
  to explain artifacts and compatibility.
- Keep DTS syntax, bindings, overlays, and detailed `dtc` use in topic 10.
  Topic 02 covers only where DTBs fit into the build/deploy set and why the
  kernel image, DTB, and module tree must come from a compatible build.

## Source Differences
- `ldd1-ch01` and `ldd1-ch02` use Linux 4.1/4.4-era commands and Ubuntu 16.04
  packages. `ldd2-ch14` uses a Linux 4.19 baseline. The Notion material uses
  Linux 6.1 examples and late-2025 prose. Package names, minimum tool versions,
  defconfigs, Kconfig paths, and artifact locations are not timeless.
- The three source groups are not duplicate editions:
  `ldd1-ch01` is a compact setup/build introduction, Notion Chapter 1 is a
  much larger tutorial expansion, and `ldd2-ch14` contributes release-tree
  selection from a debugging chapter.
- Notion's package command is illustrative, not portable. Current requirements
  depend on the selected configuration and kernel version and may include
  `pkg-config`, Perl, Python, `pahole`/`dwarves`, OpenSSL development files,
  Clang/LLVM, Rust tools, or architecture-specific utilities.
- Fixed RAM, disk, source size, module count, build time, compression size,
  and "drivers are 60/61%" figures are snapshots and should not become
  requirements.
- The release-category explanations are useful, but example "current 6.x,"
  LTS lists, support durations, and projected end dates age quickly. Product
  support also depends on distribution or vendor commitments, not kernel.org
  status alone.
- Notion calls checking `head Makefile` source authenticity verification.
  This only reports version fields and is not cryptographic verification.
- The old GitHub clone and Notion `git://` examples are not the only or best
  authority. Prefer official kernel.org HTTPS/Git endpoints and record an
  immutable tag or commit.
- The source-tree directory lists are version snapshots. Directories such as
  `rust/`, architecture names, and documentation paths can appear, move, or
  disappear.
- `ldd1-ch01` implies userspace-facing interfaces do not change. The stable
  userspace ABI principle is important, but individual interfaces have
  documented rules and exceptions; it does not mean every `/proc`, `/sys`, or
  internal kernel interface is immutable.
- Notion says one "must" always set `ARCH` and `CROSS_COMPILE` before
  configuration/building. Native builds normally use host defaults; Clang
  cross-builds may use `LLVM=1` and target options without the same GNU prefix
  pattern.
- `CROSS_COMPILE` is a tool name prefix, not itself a compiler. The tuple's ABI
  labels help identify the toolchain but do not alone prove compatibility with
  the board, kernel baseline, or userspace.
- Notion's statement that `[M]` necessarily reduces memory use is too broad.
  A loaded module occupies memory, and modularity also adds storage, loading,
  dependency, signing, and policy concerns.
- `ldd1-ch02` incorrectly describes the `n` case as using `obj-m`; an unset or
  `n` `CONFIG_*` expansion does not add the object to `obj-y` or `obj-m`.
- The old source says an external module always needs a complete precompiled
  kernel source tree. Official Kbuild supports a prepared configured tree;
  a full build is specifically needed for complete symbol/modversion data in
  cases such as `CONFIG_MODVERSIONS`.
- The source phrase "kernel headers" can mislead learners into passing an
  arbitrary `/usr/include/linux` or source `include/` directory. External
  modules need the matching kernel build/preparation context, commonly exposed
  by `/lib/modules/$(uname -r)/build` for a distribution kernel.
- The source wrapper Makefiles use `PWD := $(shell pwd)` or `$(shell pwd)`.
  Official examples commonly use `$PWD`/`$$PWD`, and modern Kbuild has gained
  additional external-module invocation forms. The core concept is `M=` with
  an absolute external source path, not one frozen wrapper.
- The source's ARM external-module command omits an explicit target and uses a
  potentially mismatched `arm-none-linux-gnueabihf` prefix. Teach the configured
  build directory and selected toolchain together rather than copying that
  exact line.
- Notion's ccache example
  `CROSS_COMPILE="ccache arm-linux-gnueabihf-"` is unsafe as a generic Kbuild
  recipe. Prefer a documented compiler launcher/path setup appropriate to the
  selected GCC or LLVM build.
- Notion reverses/contradicts clean semantics in one cross-build step by
  suggesting `mrproper` keeps `.config`. `make clean` keeps `.config`;
  `mrproper` removes generated configuration and more; `distclean` extends
  `mrproper` with common patch/editor leftovers.
- `make clean-modules` and `NO_DOC=1` are presented without evidence that they
  are supported top-level targets/options for the selected tree. Do not teach
  them without validating `make help` and the tree's Makefiles.
- The advice to clean after routine source/config changes is excessive.
  Kbuild is incremental; preserve reproducibility with separate output
  directories and clean only when diagnosis or a changed build contract
  requires it.
- `make` does not universally mean "Image + modules + dtbs." Default and
  architecture-specific targets vary. Use `make help` and the selected
  architecture documentation.
- ARM does not have one universal boot image. ARM32 platforms may use
  `zImage`, while arm64 commonly builds `Image`; bootloader packaging can add
  other formats.
- `make install` behavior is installation-environment dependent. It does not
  universally create an initramfs or update GRUB. Embedded deployment is often
  board/BSP/bootloader-specific.
- Installing modules under `/lib/modules/$(uname -r)` is wrong when staging a
  newly built kernel that is not currently running. Use the build's
  `kernelrelease` and `INSTALL_MOD_PATH`.
- Source examples use fixed `uname -r` and plain version strings
  interchangeably. `make kernelrelease`, local-version settings, dirty-tree
  suffixes, and distribution release strings can determine the actual module
  directory and vermagic.
- Building a specific directory/object is useful but does not replace final
  linking, `modpost`, whole-build dependency checks, installation, and target
  boot/module tests.
- A successful `file module.ko` architecture check is necessary but not
  sufficient. Kernel release/configuration, symbol CRCs, signatures, compiler
  features, and exported APIs can still make the module unloadable.
- Notion labels in-tree building as "production" and out-of-tree as
  "development." Upstream/in-tree integration is preferred for maintainability,
  but production products can carry external modules and developers can build
  in-tree code; the distinction is integration and maintenance, not lifecycle
  stage alone.
- Notion says not to edit a kernel Makefile directly, then instructs adding an
  in-tree driver to parent Makefiles. The intended rule is to avoid ad hoc
  top-level edits and follow the subsystem's Kconfig/Kbuild integration pattern.

## Gaps / Uncertainties
- Internal sources do not teach out-of-source kernel builds with `O=` or
  `KBUILD_OUTPUT`, although this should be the default practical workflow for
  multiple targets/configurations.
- They do not explain how source directory and output directory differ when
  building an external module against a separately built kernel.
- `oldconfig`, `olddefconfig`, `listnewconfig`, config fragments, and safe
  migration of an existing product config across kernel versions are
  under-covered.
- The difference among `make kernelversion`, `make kernelrelease`,
  `uname -r`, `CONFIG_LOCALVERSION`, and distribution release strings is
  missing and is central to module installation/debugging.
- Toolchain compatibility is simplified to package installation. The lesson
  needs to distinguish host tools, target compiler/binutils, supported minimum
  versions, GCC versus LLVM, and optional tools activated by configuration.
- Reproducible-build inputs are absent: exact source commit, `.config`,
  toolchain versions, build scripts/container, environment, timestamps,
  signing keys, and deployed artifact hashes.
- The sources do not adequately cover module signing, Secure Boot, compressed
  modules, or distribution/vendor policies that can reject an otherwise
  correctly built module. Topic 02 should introduce these as compatibility
  checks and defer policy depth.
- There is little guidance on preserving a known-good boot entry, serial
  console, recovery path, or rollback before deploying a custom kernel.
- Initramfs generation and bootloader update are treated as automatic in some
  examples. Their ownership must be made explicit and platform-specific.
- The exact DTB target names and locations are architecture/version-sensitive.
  Detailed Device Tree compilation and validation belong to topic 10.
- The sources omit common build diagnostics such as capturing the first real
  error in parallel output, rebuilding with `V=1`, checking
  `scripts/ver_linux`, inspecting `.config`, using `make help`, and confirming
  the compiler shown in the log.
- External-module diagnosis should connect `vermagic`, `Module.symvers`,
  `CONFIG_MODVERSIONS`, unresolved symbols, `modpost`, target `dmesg`, and
  `modinfo` without duplicating topic 03's load/dependency lifecycle.
- Buildroot, Yocto/OpenEmbedded, distro packaging, CI, QEMU, and vendor SDK
  flows are absent or only implied. Topic 02 should explain how their generated
  source/build/sysroot paths map to the same Kbuild concepts without becoming
  a build-system-specific tutorial.

## External Validation
Official sources were checked on June 6, 2026.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/process/changes.html` | Validated that build prerequisites and minimum versions are kernel-version/configuration-sensitive, including GCC/LLVM, Make, binutils, flex, bison, Python, OpenSSL, and `pahole`. |
| `https://docs.kernel.org/admin-guide/README.html` | Validated the basic configure/build/install flow, `O=` consistency, config migration, `V=1`, and the platform/distribution-dependent nature of `make install`. |
| `https://docs.kernel.org/kbuild/kconfig.html` | Validated Kconfig frontend/search behavior and the need to handle new symbols when carrying configurations across releases. |
| `https://docs.kernel.org/kbuild/kbuild.html` | Validated core Kbuild variables, output metadata, compiler selection context, and separate output-directory concepts. |
| `https://docs.kernel.org/kbuild/makefiles.html` | Validated `obj-y`, `obj-m`, `obj-$(CONFIG_*)`, composite objects, recursive directories, and Kbuild clean integration. |
| `https://docs.kernel.org/kbuild/modules.html` | Validated external-module `M=` builds, matching build-tree requirements, `modules_prepare`, the `Module.symvers` limitation, module installation paths, and multi-file module syntax. |
| `https://docs.kernel.org/kbuild/headers_install.html` | Validated that `headers_install` exports userspace-facing UAPI headers and is not the external-module build-header workflow. |
| `https://docs.kernel.org/kbuild/llvm.html` | Validated current Clang/LLVM build support and why the lesson must not present GNU `CROSS_COMPILE` as the only cross-build model. |
| `https://docs.kernel.org/kbuild/reproducible-builds.html` | Validated timestamp, build user/host, path, initramfs, and signing-key inputs that affect reproducibility. |
| `https://www.kernel.org/releases.html` | Validated release categories and confirmed that active longterm versions and projected EOL dates are time-sensitive. |
| `https://www.kernel.org/signature.html` | Validated cryptographic verification of kernel.org release archives and corrected the source's Makefile-version check being described as authenticity verification. |

## Learning Content Brief

### Beginner Mental Model
- A kernel build is a controlled transformation:
  **source identity + configuration + toolchain + build environment -> kernel
  artifacts for one target**.
- A module build is not ordinary application compilation. Kbuild must compile
  it with the selected kernel's generated configuration, flags, headers, and
  symbol metadata.
- The safest workflow keeps source, generated build output, staged target
  rootfs, and deployed artifacts visibly separate.
- The kernel image, DTB, module tree, and configuration are a matched set.
  Mixing artifacts from different builds creates failures that often look like
  driver bugs.

### Core Mechanism
- Kconfig files define symbols, types, prompts, defaults, and dependencies.
- A configuration target expands a baseline into `.config`.
- Kbuild evaluates configuration values while descending through Makefiles and
  Kbuild files.
- Host tools generate intermediate files; the target compiler builds target
  objects; link and post-processing stages produce `vmlinux`, a boot image,
  modules, and metadata.
- Architecture Makefiles decide image and DTB targets.
- `modules_install` stages modules under a directory named by the build's
  kernel release and generates/updates module metadata through the installation
  flow.
- External builds enter the matching kernel build system with `M=<external
  source>` instead of manually supplying kernel include paths and flags.

### Important Files, Variables, And Commands
- Files/directories:
  top-level `Makefile`, `Kconfig`, distributed `Makefile`/`Kbuild`,
  `.config`, `arch/<arch>/configs/`, `include/generated/`,
  `Module.symvers`, `System.map`, `vmlinux`, `.ko`, `.dtb`,
  `/lib/modules/<kernelrelease>/build`.
- Configuration:
  `defconfig`, board/SoC defconfig targets, `menuconfig`, `nconfig`,
  `oldconfig`, `olddefconfig`, `savedefconfig`, `make help`.
- Build selection:
  `ARCH`, `CROSS_COMPILE`, `LLVM`, `O=`/`KBUILD_OUTPUT`, `V=1`, parallel `-j`.
- Module/build integration:
  `obj-y`, `obj-m`, `obj-$(CONFIG_FOO)`, `<module>-y`,
  `M=`, `modules_prepare`, `modules`, `modules_install`.
- Installation/staging:
  `INSTALL_MOD_PATH`, `make kernelrelease`, target boot partition/rootfs,
  initramfs and bootloader tooling where applicable.
- Inspection:
  `file`, `readelf`, `modinfo`, `scripts/ver_linux`, `.config`,
  build logs, `uname -r`, and target `dmesg`.

### Lifecycle / Data Flow
1. Define target board, architecture, boot chain, rootfs, and kernel source
   identity.
2. Provision host dependencies and verify tool versions.
3. Obtain and verify source; record commit/tag and downstream patches.
4. Select a clean output directory and consistent architecture/compiler
   variables.
5. Seed configuration from the target's known-good defconfig or product config.
6. Reconcile dependencies and customize required drivers/features.
7. Build the architecture image, required modules, and required DTBs.
8. Inspect outputs and build release; retain `.config`, logs, and metadata.
9. Stage modules and other rootfs artifacts outside the host root filesystem.
10. Deploy a matched artifact set with a rollback/recovery path.
11. Boot and verify the running release, logs, hardware description, and module
    tree.
12. For an external driver, build against that same source/output contract and
    diagnose `modpost`, vermagic, symbol, signature, and target-load failures.

### Suggested Examples
- Native build versus ARM/arm64 cross-build command skeleton using a separate
  `O=` directory.
- Defconfig -> `olddefconfig`/`menuconfig` -> build -> `kernelrelease` flow.
- One single-file external module and one composite module showing only Kbuild
  mechanics; module behavior remains topic 03.
- In-tree Kconfig plus Makefile connection:
  `config EXAMPLE` and `obj-$(CONFIG_EXAMPLE) += example.o`.
- Staging modules with `INSTALL_MOD_PATH` and checking that the resulting
  directory matches `make kernelrelease`.
- A failure matrix comparing wrong `ARCH`, missing tool, stale `.config`,
  missing generated headers, absent `Module.symvers`, wrong kernel release,
  unresolved symbol, and rejected signature.

### Common Bugs And Failure Modes
- Configuring for one architecture and building for another.
- Using a toolchain prefix that does not exist or does not match the intended
  target.
- Starting from an unsuitable defconfig or silently losing required rootfs,
  console, storage, bus, or filesystem support.
- Reusing one in-tree output across architectures/configurations.
- Installing target modules into the host's `/lib/modules`.
- Deploying a new kernel image with an old DTB or module directory.
- Building an external module against the running host rather than the target
  kernel build.
- Treating source headers or userspace UAPI headers as a prepared module build
  tree.
- Missing `Module.symvers` when symbol versioning is enabled.
- Looking only at the last line of a parallel build and missing the first real
  compiler/configuration error.
- Cleaning reflexively and hiding dependency/reproducibility problems.
- Assuming a successful compile means the image will boot or the module will
  load.
- Overwriting the only known-good boot artifacts without a recovery path.

### Debugging Notes
- Confirm the selected source/commit and compare `make kernelrelease` with the
  deployed/running `uname -r`.
- Check `ARCH`, `CROSS_COMPILE`, `LLVM`, `O=`, tool versions, and the actual
  compiler command via `V=1`.
- Preserve the first error from a non-parallel or logged rebuild; later errors
  may be consequences.
- Inspect `.config` and use Kconfig search/help to understand hidden options and
  unmet dependencies.
- Use `file`/`readelf` to confirm artifact architecture and type.
- For external modules, inspect `modinfo`, vermagic, `Module.symvers`,
  `modpost` diagnostics, exported symbols, signature state, and target `dmesg`.
- Verify all deployed members of the build set, not just the kernel image.

### Production Concerns
- Pin source, patches, configuration, toolchain, build scripts, and artifact
  hashes in version control or release metadata.
- Prefer clean, separate output directories and automated repeatable commands.
- Build as an unprivileged user; require privilege only for controlled
  installation/deployment steps.
- Stage target files into a rootfs/image directory rather than writing to the
  host by default.
- Preserve signing keys and reproducibility inputs securely.
- Validate boot, probe, module load, rootfs access, networking/console, and
  rollback on real target hardware.
- Treat vendor BSP divergence and out-of-tree modules as ongoing maintenance
  liabilities that must be rebased and tested.
- Keep a serial console or equivalent early-boot observability path.

### Interview Angles
- Explain `ARCH` versus `CROSS_COMPILE`; identify what each controls and what
  neither guarantees.
- Explain defconfig versus `.config` versus `savedefconfig`.
- Trace how `CONFIG_FOO=m` reaches `obj-$(CONFIG_FOO)` and produces a `.ko`.
- Explain why external modules use Kbuild and `M=` instead of manually adding
  kernel include paths.
- Distinguish kernel source, kernel headers/UAPI headers, a prepared build
  tree, and a fully built tree.
- Explain when `modules_prepare` is enough and why `CONFIG_MODVERSIONS` can
  require a full build.
- Diagnose a module that compiles successfully but fails to load with invalid
  format, unknown symbol, version disagreement, or signature rejection.
- Explain why `INSTALL_MOD_PATH` matters during cross-development.
- Explain `vmlinux`, the architecture boot image, `System.map`,
  `Module.symvers`, DTBs, and `.ko` files.
- Explain why a matching version string alone does not prove artifact
  compatibility.
- Describe a reproducible and recoverable kernel build/deployment pipeline.
- Challenge the claim that `make install` universally updates the bootloader
  and initramfs.

## Suggested Output Files
- `knowledge/02-environment-setup-kernel-source-and-build-flow.md`
- `interview/02-environment-setup-kernel-source-and-build-flow.md`
- `examples/02-environment-setup-kernel-source-and-build-flow/README.md`
