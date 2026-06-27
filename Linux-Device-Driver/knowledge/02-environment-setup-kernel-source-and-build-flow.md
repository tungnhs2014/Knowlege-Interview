# 02 - Environment Setup, Kernel Source, And Build Flow

## Learning Goal

After this chapter, you should be able to prepare a Linux kernel development
environment, choose the correct source and configuration, build a matched set
of kernel artifacts, and diagnose common kernel and external-module build
failures.

By the end, you should be able to:

- Explain the host/target model for native and cross-compilation.
- Treat the kernel source, configuration, toolchain, and generated build state
  as one **build contract**.
- Choose among a mainline, stable, longterm, distribution, or vendor kernel
  tree for a specific target.
- Distinguish a defconfig, the expanded `.config`, and a minimized
  `savedefconfig`.
- Explain how Kconfig selection reaches Kbuild and produces built-in objects or
  loadable modules.
- Use a separate output directory with `O=` consistently.
- Build a kernel image, modules, and Device Tree blobs for native or
  cross-compiled targets.
- Stage modules without accidentally installing target files into the host.
- Build an external module against the matching configured kernel build tree.
- Distinguish kernel source, UAPI headers, a prepared build tree, and a fully
  built tree.
- Investigate architecture, configuration, toolchain, symbol-version,
  deployment, and module-signing failures systematically.

This chapter covers the build environment and artifact flow. Module loading,
parameters, dependencies, and entry/exit behavior belong to topic 03.

## Why This Matters In Real Work

Many failures that look like driver bugs are actually build-contract or
deployment mistakes. A correct source change has no effect if the target boots
an old image, uses an incompatible DTB, or loads a module built for another
kernel.

Common real-world failures include:

- Building against the host kernel instead of the embedded target kernel.
- Reusing one output tree for different architectures or product
  configurations.
- Starting from a defconfig that lacks the target's boot storage, root
  filesystem, console, or required bus support.
- Deploying a new kernel image with old modules or an old DTB.
- Installing target modules under the development host's `/lib/modules`.
- Treating `/usr/include/linux` as the headers needed for a kernel module.
- Seeing a successful compile and assuming the result will boot or load.
- Overwriting the only bootable image before establishing a recovery path.

The engineering goal is not merely "make it compile." It is:

```text
identify -> configure -> build -> inspect -> stage -> deploy -> boot -> verify
```

Every step must refer to the same target and the same build identity.

## Mental Model

A kernel build is a controlled transformation:

```text
source identity
  + downstream patches
  + kernel configuration
  + target architecture
  + compiler and host tools
  + build environment
  + signing/reproducibility inputs
        |
        v
matched target artifacts
  = kernel image + DTBs + modules + metadata
```

The version number alone is not enough to identify that transformation.
`6.x.y` built with two configurations or two vendor patch sets can expose
different symbols, structure layouts, features, and module compatibility.

### Host And Target

Keep two machines conceptually separate even when they happen to be the same
physical computer.

| Role | Responsibility |
| --- | --- |
| Host | Runs Git, Kconfig tools, Make, host utilities, compiler, linker, and packaging scripts |
| Target | Boots the resulting kernel and runs its modules, DTB, root filesystem, and applications |

A **native build** produces code for the same architecture family as the host.
A **cross-build** runs tools on one architecture while producing code for
another.

```text
x86-64 host                         arm64 target
-------------                       ------------
make, bash, flex, bison             bootloader
host C compiler for build tools     kernel Image
arm64 target compiler      ------>  DTB
linker and objcopy                   /lib/modules/<release>/
```

Some kernel build programs run on the host, while kernel objects run on the
target. This is why a cross-build can require both working host tools and a
working target toolchain.

### Keep Four Locations Separate

A clean workspace separates immutable inputs from generated and deployed
files:

```text
work/
|-- linux/              # source checkout
|-- build-arm64/        # generated files, .config, objects, images
|-- module-src/         # external module source
`-- target-rootfs/      # staged target filesystem
```

- The **source tree** contains version-controlled kernel code.
- The **output tree** contains `.config`, generated headers, objects, and final
  build artifacts.
- The **external module tree** contains independently maintained module source.
- The **staging tree** mirrors files that will later enter the target root
  filesystem.

**Production rule:** use a distinct output directory for each target,
architecture, and important configuration. Do not share generated state across
unrelated builds.

## Core Concepts

### Select The Kernel Tree Deliberately

The correct source is the tree used by the target product, not automatically
the newest upstream release.

| Source choice | Appropriate use | Main risk |
| --- | --- | --- |
| Mainline tree | Upstream development and testing future-facing support | Product-specific support may be absent |
| Stable branch | Maintained fixes for a released kernel series | Still may not contain vendor platform changes |
| Longterm branch | Products needing a maintained upstream baseline | Kernel.org and vendor support periods differ |
| Distribution tree | A distribution kernel and its packaging/ABI policy | Patches and config differ from plain upstream |
| Vendor/BSP tree | Hardware requiring downstream SoC or board support | Larger maintenance and rebase burden |
| Release archive | Fixed, easy-to-record released baseline | No Git history unless obtained separately |

Record at least:

- repository or archive origin;
- exact tag and commit ID;
- vendor or product patch series;
- configuration origin;
- toolchain identity and version;
- build script or command line.

Reading version fields in the top-level `Makefile` identifies the declared
kernel version. It does **not** authenticate the source. Verify release
archives using the kernel.org signature process and record immutable Git
commits or verified tags where the project requires it.

### Source, Headers, And Build Trees Are Different

The word "headers" is often used too loosely.

| Item | What it provides | Suitable for external module build? |
| --- | --- | --- |
| Kernel source tree | Source, Kconfig, Kbuild files, internal headers | Only after matching configuration/preparation |
| `/usr/include/linux` | Userspace-facing headers installed for libc/applications | No |
| `make headers_install` output | Sanitized UAPI headers for userspace | No |
| Prepared kernel build tree | `.config`, generated headers, scripts, enough generated state for many module builds | Usually |
| Fully built matching tree | Prepared state plus complete build outputs and `Module.symvers` | Yes; needed for complete symbol-version data |
| Distribution `linux-headers-<release>` package | Distribution-prepared build interface for that exact packaged kernel | Usually, for that exact distribution kernel |

An external module must use the target kernel's build system because Kbuild
supplies much more than include paths:

- generated configuration values;
- architecture and compiler flags;
- generated headers;
- symbol export processing;
- optional symbol-version CRCs;
- module metadata and post-processing.

### Kconfig Chooses Features

Kconfig files define symbols, types, prompts, defaults, dependencies, and help
text. Configuration tools evaluate those rules and write the expanded result
to `.config`.

For a tristate option:

| Value | Meaning | Typical build result |
| --- | --- | --- |
| `y` | Built in | Object eventually links into `vmlinux` |
| `m` | Module | Kbuild produces a `.ko` |
| `n` | Disabled | Object is omitted |

Not every symbol is tristate, and not every visible option can select all
three values. Dependencies can hide an option or constrain its value.

Important configuration forms:

| Form | Purpose |
| --- | --- |
| `defconfig` target | Maintained starting configuration for an architecture or platform |
| `.config` | Full, expanded configuration used by the current build |
| `savedefconfig` output | Minimized choices that differ from defaults |
| Config fragment | Partial set of desired symbols, merged through a project-specific workflow |

**Interview trap:** a defconfig is not simply another name for `.config`.
A defconfig is an input baseline; `.config` is the resolved build
configuration.

### Kbuild Turns Selection Into Objects

Kbuild Makefiles connect configuration symbols to source objects:

```makefile
obj-$(CONFIG_DEMO_SENSOR) += demo_sensor.o
```

The expansion determines ownership:

```text
CONFIG_DEMO_SENSOR=y
  -> obj-y += demo_sensor.o
  -> built into the kernel

CONFIG_DEMO_SENSOR=m
  -> obj-m += demo_sensor.o
  -> built as demo_sensor.ko

CONFIG_DEMO_SENSOR is unset/n
  -> object is not selected
```

For a module assembled from multiple source files:

```makefile
obj-m := demo.o
demo-y := demo_core.o demo_bus.o
```

Kbuild recursively enters directories selected by parent Kbuild files. An
in-tree driver therefore usually needs both:

- a Kconfig entry that makes the feature selectable;
- a Kbuild/Makefile entry that connects the symbol to its objects.

### Target Selection Variables

These variables solve different problems:

| Variable | Controls | Example |
| --- | --- | --- |
| `ARCH` | Kbuild target architecture and architecture-specific rules | `arm64`, `arm`, `x86` |
| `CROSS_COMPILE` | Prefix used to find GNU target tools | `aarch64-linux-gnu-` |
| `LLVM=1` | Selects LLVM tools using Kbuild's LLVM support | `clang`, `ld.lld`, LLVM utilities |
| `O=` | Separate output directory for generated build state | `$PWD/build-arm64` |
| `V=1` | Verbose commands for diagnosis | Shows actual compiler/linker commands |
| `-j<N>` | Parallel job count | `-j8` |

`CROSS_COMPILE` is a prefix, not a compiler executable:

```text
CROSS_COMPILE=aarch64-linux-gnu-
                         |
                         +-> aarch64-linux-gnu-gcc
                         +-> aarch64-linux-gnu-ld
                         +-> aarch64-linux-gnu-objcopy
```

Neither `ARCH` nor a plausible toolchain tuple proves that the resulting kernel
matches the board or userspace ABI. The board configuration, source tree, boot
format, and platform support must also agree.

For a normal native build, Kbuild commonly derives the host architecture and
uses the default compiler, so `ARCH` and `CROSS_COMPILE` can be omitted.

### The Kernel Release String

Several similar-looking values answer different questions:

| Value/command | Meaning |
| --- | --- |
| `make kernelversion` | Base version declared by the source tree |
| `make kernelrelease` | Effective release string produced by this build configuration |
| `uname -r` | Release string of the kernel currently running on the machine queried |
| `CONFIG_LOCALVERSION` | Configured suffix contributing to the build's release |

Modules are installed under `/lib/modules/<kernelrelease>/`, and module
compatibility checks use build information related to that target kernel.

**Production rule:** when staging a newly built kernel, use the build's
`make kernelrelease`. Do not substitute the host's `uname -r`.

## Kernel Mechanism

The build system combines Kconfig, distributed Kbuild files, architecture
rules, host-generated tools, target compilation, linking, and post-processing.

### 1. Configuration

A configuration target creates or updates `.config` in the output tree:

```text
Kconfig definitions
  + selected defconfig/product config
  + dependency/default resolution
  + user changes
        |
        v
build/.config
        |
        v
generated configuration headers
```

Useful configuration commands include:

- `make ... defconfig` or a board/SoC-specific defconfig target;
- `make ... olddefconfig` to accept defaults for new symbols non-interactively;
- `make ... oldconfig` to ask about new symbols interactively;
- `make ... menuconfig` or `nconfig` to inspect and edit selections;
- `make ... savedefconfig` to generate a minimized defconfig;
- `make ... help` to discover targets supported by the selected tree and
  architecture.

When moving a product config to a newer kernel, do not merely copy `.config`
and ignore new symbols. Reconcile it using the selected tree's Kconfig tools,
review differences, and retest boot-critical features.

### 2. Dependency Traversal And Compilation

Kbuild evaluates the effective configuration while descending through
Makefiles/Kbuild files:

```text
top-level Makefile
  -> architecture rules
  -> selected directories
  -> selected built-in and modular objects
  -> generated headers and host tools
  -> target compiler and assembler
```

Incremental builds use dependency information to rebuild affected outputs.
A routine source edit should not require `make clean`.

### 3. Linking And Post-Processing

The exact stages depend on architecture and configuration, but common outputs
include:

| Artifact | Role |
| --- | --- |
| `vmlinux` | ELF kernel image with symbols; valuable for debugging and analysis |
| Architecture boot image | Bootable/packaged form such as `bzImage`, `zImage`, or `Image` |
| `System.map` | Symbol-address map for the built kernel |
| `.dtb` | Compiled Device Tree hardware description where used |
| `.ko` | Loadable kernel module |
| `Module.symvers` | Exported symbols and optional modversion CRC information |
| `modules.order` | Module build ordering metadata |
| `modules.builtin*` | Metadata describing code built into the kernel |

There is no universal architecture-independent boot image filename. Use
`make help`, architecture documentation, and the board/BSP deployment flow.

### 4. Module Staging

`modules_install` places built modules under:

```text
<INSTALL_MOD_PATH>/lib/modules/<kernelrelease>/
```

This is a staging operation when `INSTALL_MOD_PATH` points to a target rootfs
directory. The resulting tree may contain module files, dependency metadata,
symbol/alias indexes, and links back to build/source locations depending on
the installation environment.

Do not assume `make install` will correctly:

- install an embedded boot image;
- generate the required initramfs;
- update a bootloader;
- choose the correct boot slot;
- preserve a rollback image.

Those responsibilities are distribution-, BSP-, and product-specific.

### 5. External Module Entry Into Kbuild

An external module enters the configured kernel build system with:

```bash
make -C <kernel-build-dir> M=<absolute-module-source-dir> modules
```

- `-C` changes Make's working directory to the kernel build directory.
- `M=` identifies the external module source directory.
- `modules` requests external-module compilation and post-processing.

When the kernel uses a separate output directory, point `-C` at the prepared
or fully built **output tree**, not merely at the clean source directory.

`make modules_prepare` prepares many generated files needed for external
modules, but it does not generate `Module.symvers`. If
`CONFIG_MODVERSIONS=y` or complete exported-symbol version data is required,
perform a full matching kernel build.

## Key Structs And APIs

This topic has no central runtime structure comparable to `struct device`.
Its important APIs are the Kconfig/Kbuild files, configuration state,
variables, targets, and generated metadata that define the build contract.

### Source-Tree Landmarks

| Path | Why it matters |
| --- | --- |
| Top-level `Makefile` | Kernel version fields and top-level build orchestration |
| `Kconfig` and distributed Kconfig files | Feature definitions, dependencies, defaults, and help |
| Distributed `Makefile`/`Kbuild` files | Object selection and recursive build rules |
| `arch/<arch>/` | Architecture code, image rules, defconfigs, and often DT build paths |
| `drivers/` | Driver subsystems and neighboring implementation examples |
| `include/linux/` | Internal kernel interfaces |
| `include/uapi/` | Interfaces intended for export to userspace |
| `include/generated/` in output | Build-generated headers; do not hand-edit |
| `scripts/` | Build, configuration, checking, and development utilities |
| `Documentation/` | Build process, subsystem, API, and contribution guidance |
| `MAINTAINERS` | Ownership and relevant source/document paths |

### Clean Targets

Cleaning targets have different blast radii:

| Target | Effect |
| --- | --- |
| `clean` | Removes most generated build files but normally preserves `.config` |
| `mrproper` | Removes generated configuration and more generated state |
| `distclean` | Extends `mrproper` with common patch/editor leftovers |

Prefer a new output directory when changing architecture or establishing a
known-clean build. Reflexive cleaning makes builds slower and can hide whether
the normal incremental workflow is correct.

### Build-System Wrappers

Yocto/OpenEmbedded, Buildroot, distribution packaging, vendor SDKs, and CI
systems add orchestration, but the underlying questions remain:

- Which kernel source and commit are being used?
- Where is the actual Kbuild output directory?
- Which `.config` is effective?
- Which compiler and target architecture are selected?
- Where are modules staged?
- Which image, DTB, and module tree reach the target?

Use the wrapper's supported tasks and environment rather than bypassing it,
but map those tasks back to these Kbuild concepts when debugging.

## Lifecycle / Data Flow

A robust build and deployment flow follows one explicit build identity from
source selection to target verification.

### 1. Define The Target

Record:

- board and SoC;
- CPU architecture;
- bootloader and expected image format;
- Device Tree or other firmware description;
- root filesystem and boot-critical storage/filesystem drivers;
- console or serial recovery path;
- expected kernel source and product branch.

### 2. Provision The Host

Install and verify:

- Make and a supported C compiler;
- binutils or LLVM tools;
- shell and basic build utilities;
- Kconfig dependencies such as flex and bison where required;
- optional tools enabled by the chosen configuration, such as OpenSSL
  development files or `pahole`;
- the target cross-toolchain for a cross-build.

Use the selected kernel tree's build-requirements documentation as the
authority. Required tools and minimum versions change over time and with
configuration.

### 3. Obtain And Identify Source

- Fetch from an authoritative project location.
- Verify archives or tags according to project policy.
- Check out an immutable commit.
- Apply and record downstream patches.
- Keep the working tree status visible during release builds.

### 4. Create A Dedicated Output Directory

Choose one output directory and use the same `O=` value for configuration,
build, inspection, and installation commands.

### 5. Establish Configuration

Start from:

- the product's known-good `.config`;
- a maintained board/SoC defconfig;
- a distribution configuration;
- or a documented generic defconfig for initial exploration.

Then reconcile dependencies and review boot-critical options.

### 6. Build The Required Artifact Set

Build the architecture's expected kernel image plus the modules and DTBs the
target needs. The exact image/DTB target is platform-specific.

### 7. Inspect And Preserve Outputs

Retain:

- effective `.config`;
- source commit and patch identity;
- toolchain versions;
- `make kernelrelease`;
- build log;
- `vmlinux`, image, DTBs, modules, and useful maps/metadata;
- checksums or release manifest.

### 8. Stage, Then Deploy

Install modules into a staging root, not the host root. Deploy the image, DTB,
module tree, initramfs, and bootloader metadata as one compatible release.

### 9. Boot And Verify

On the target, verify:

```bash
uname -r
cat /proc/version
dmesg | head -100
```

Also confirm:

- the intended boot entry or slot was selected;
- the expected DTB was loaded;
- `/lib/modules/$(uname -r)` exists when modules are used;
- console, root storage, filesystem, networking, and required devices work;
- no module format, symbol, signature, or firmware errors appear.

### 10. Build External Drivers Against The Same Contract

Use the matching output tree, target architecture/toolchain choices, symbol
metadata, and installation release. A locally successful build against the
host kernel says nothing about compatibility with an unrelated target.

## Minimal Practical Examples

These examples demonstrate Kbuild mechanics. Target names, defconfigs,
toolchain prefixes, dependency packages, bootloader commands, and deployment
locations must be adapted to the selected kernel and board.

### Native Out-Of-Source Build

**Example status: learning-only command skeleton, not production-ready.**

```bash
SRC=$HOME/work/linux
OUT=$HOME/work/build-native

make -C "$SRC" O="$OUT" defconfig
make -C "$SRC" O="$OUT" olddefconfig
make -C "$SRC" O="$OUT" -j"$(nproc)"
make -C "$SRC" O="$OUT" kernelrelease
```

Important points:

- Every command uses the same `O=`.
- `defconfig` establishes a baseline; it may not support a specific product.
- The default build target is architecture-dependent. Use `make ... help` and
  product documentation when an explicit image target is required.
- `$(nproc)` is convenient for learning; production CI should set a controlled
  job count based on memory and executor limits.

### ARM64 Cross-Build And Staging

**Example status: learning-only command skeleton, not production-ready.**

```bash
SRC=$HOME/work/linux
OUT=$HOME/work/build-arm64
STAGE=$HOME/work/target-rootfs

make -C "$SRC" O="$OUT" \
        ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig

make -C "$SRC" O="$OUT" \
        ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- olddefconfig

make -C "$SRC" O="$OUT" \
        ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
        -j8 Image modules dtbs

release=$(make -s -C "$SRC" O="$OUT" \
        ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- kernelrelease)

make -C "$SRC" O="$OUT" \
        ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- \
        INSTALL_MOD_PATH="$STAGE" modules_install

test -d "$STAGE/lib/modules/$release"
```

Important points:

- `ARCH` and `CROSS_COMPILE` remain consistent from configuration through
  installation.
- `Image modules dtbs` is common for arm64 but is not a universal board build
  recipe.
- The release comes from the build, not from host `uname -r`.
- `INSTALL_MOD_PATH` prevents target modules from entering the host module
  tree.
- Deployment and bootloader update are intentionally absent because they are
  product-specific and potentially destructive.

### LLVM Build Selection

**Example status: learning-only command skeleton, not production-ready.**

```bash
make -C "$SRC" O="$OUT" ARCH=arm64 LLVM=1 defconfig
make -C "$SRC" O="$OUT" ARCH=arm64 LLVM=1 -j8 Image modules dtbs
```

Use the selected kernel's LLVM documentation to determine whether additional
target or tool variables are needed. Confirm supported compiler versions
before assuming that the host's current Clang is suitable.

### External Single-File Module

**Example status: learning-only Kbuild example, not production-ready.**

External module `Makefile`:

```makefile
obj-m := demo_build.o
```

Build it against a matching kernel output tree:

```bash
KOUT=$HOME/work/build-arm64
MODSRC=$HOME/work/module-src

make -C "$KOUT" M="$MODSRC" \
        ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules

file "$MODSRC/demo_build.ko"
modinfo "$MODSRC/demo_build.ko"
```

The Kbuild line is the important part. Do not replace it with a hand-written
compiler command and a collection of `-I` paths.

For a composite module:

```makefile
obj-m := demo_build.o
demo_build-y := demo_core.o demo_bus.o
```

### In-Tree Selection

**Example status: learning-only integration fragment, not production-ready.**

Kconfig:

```text
config DEMO_SENSOR
        tristate "Demo sensor"
        depends on I2C
        help
          Build support for the demo learning sensor.
```

Kbuild/Makefile:

```makefile
obj-$(CONFIG_DEMO_SENSOR) += demo_sensor.o
```

This is not complete product integration. A production change also needs the
correct subsystem location, parent Kconfig/Makefile recursion, dependencies,
help text, source code, documentation/bindings where applicable, review, and
target testing.

### What Production-Ready Automation Adds

**Example status: production expectations, not a drop-in script.**

A production pipeline should:

- pin source commits, patches, config inputs, and toolchain/container versions;
- create a fresh or controlled output directory;
- fail on missing configuration inputs or unexpected config drift;
- capture complete logs and the first real error;
- record `kernelrelease`, compiler versions, artifact hashes, and build
  provenance;
- stage all target files outside the host root;
- sign artifacts according to product policy;
- package the image, DTB, modules, and initramfs as one release;
- deploy to a recoverable slot or preserve a known-good boot entry;
- run target smoke, boot, module, and hardware tests.

## Common Bugs And Debugging

Begin by locating the failing stage. A compiler error, `modpost` error, module
load rejection, and boot failure require different evidence.

### Failure Matrix

| Symptom | Likely cause | Evidence to inspect | Fix pattern |
| --- | --- | --- | --- |
| Compiler executable not found | Wrong `CROSS_COMPILE` prefix or missing toolchain | `command -v`, verbose build command | Install/select correct toolchain and keep prefix consistent |
| Compiler targets wrong architecture | Wrong `ARCH`, compiler, or stale shared output tree | `file`, `readelf -h`, `V=1` | Use correct variables and a fresh architecture-specific output |
| Kconfig option is missing or cannot become `m` | Hidden dependency, wrong symbol type, or modules disabled | Kconfig search/help, `.config`, `CONFIG_MODULES` | Satisfy dependencies or choose supported state |
| Build fails after copying old `.config` | New/renamed symbols or incompatible config migration | First error, `oldconfig`/`olddefconfig`, config diff | Reconcile config in the new tree and review changes |
| Missing generated header | Tree was not configured/prepared or wrong build directory used | `.config`, `include/generated/`, command path | Configure and prepare/build the correct output tree |
| `modpost` reports undefined symbol | Symbol is not exported, provider not selected, or wrong symbol metadata | `Module.symvers`, `.config`, source exports | Enable/build provider, use exported API, or correct build tree |
| CRC/version warning with `CONFIG_MODVERSIONS` | Missing/stale `Module.symvers` | Config and matching full-build output | Perform matching full kernel build and rebuild module |
| `.ko` builds but target says `Invalid module format` | Release/config/toolchain feature mismatch | Target `dmesg`, `modinfo -F vermagic`, `uname -r` | Rebuild against exact target build contract |
| Module reports unknown symbols on target | Dependency absent, symbol unavailable, or version mismatch | Target `dmesg`, `modinfo`, installed module metadata | Install matched dependency set and regenerate indexes |
| Module rejected for signature/key | Signing, Secure Boot, lockdown, or product policy | Target `dmesg`, `modinfo`, platform policy | Sign with trusted key or follow approved policy |
| New driver appears to have no effect | Old image/module booted or driver not selected | Hashes, timestamps, `uname -r`, `.config`, target log | Deploy and verify the intended matched artifact set |
| Kernel boots but devices fail | Old/wrong DTB or modules, missing config, firmware mismatch | Bootloader selection, DT identity, `dmesg` | Deploy compatible image, DTB, modules, and firmware |
| Target cannot mount rootfs | Boot-critical controller/filesystem built as unavailable module | `.config`, initramfs contents, boot log | Build required support in or include it in a working initramfs |

### Preserve The First Error

Parallel builds can interleave messages. The final line is often a consequence
of an earlier failure.

Use a logged build:

```bash
make -C "$SRC" O="$OUT" -j8 2>&1 | tee build.log
```

Then search from the beginning for the first compiler or Kbuild failure. If
needed, rerun the failing target with fewer jobs and verbose output:

```bash
make -C "$SRC" O="$OUT" -j1 V=1 <target>
```

Avoid sharing logs publicly without reviewing paths, configuration, signing
information, and other product-sensitive data.

### Verify The Actual Compiler And Architecture

High-signal checks:

```bash
command -v aarch64-linux-gnu-gcc
aarch64-linux-gnu-gcc --version
make -C "$SRC" O="$OUT" V=1 <target>
file "$OUT/vmlinux"
readelf -h "$OUT/vmlinux"
```

For a module:

```bash
file demo.ko
readelf -h demo.ko
modinfo demo.ko
```

Correct ELF architecture is necessary, but not sufficient. Release,
configuration, symbol CRCs, signatures, and exported APIs can still disagree.

### Inspect Configuration Instead Of Guessing

Useful checks:

```bash
grep '^CONFIG_MODULES=' "$OUT/.config"
grep '^CONFIG_MODVERSIONS=' "$OUT/.config"
grep '^CONFIG_DEMO_SENSOR=' "$OUT/.config"
```

Inside `menuconfig` or `nconfig`, use search and help to find:

- the exact symbol name;
- its type;
- dependency expressions;
- symbols that select or imply it;
- the source Kconfig location.

Do not force-edit `.config` as the normal solution to unmet dependencies.
Kconfig may rewrite invalid selections when configuration is regenerated.

### Compare Build And Running Releases

On the build host:

```bash
make -s -C "$SRC" O="$OUT" kernelrelease
modinfo -F vermagic demo.ko
```

On the target:

```bash
uname -r
dmesg | tail -100
```

A matching visible release string still does not prove identical source,
configuration, or symbol-version metadata, but a mismatch immediately
identifies a deployment problem.

### Diagnose External Module Preparation

Ask these questions in order:

1. Is `-C` pointing to the target kernel's configured output tree?
2. Does that tree contain the expected `.config` and generated headers?
3. Were `ARCH`, compiler selection, and cross prefix kept consistent?
4. Does the module's Kbuild file select the intended objects?
5. Did `modpost` report missing or versioned symbols?
6. Is `Module.symvers` present and from the matching full build?
7. Does `modinfo` show the expected vermagic, dependencies, and signature?
8. What exact reason does the target kernel report in `dmesg`?

Topic 03 continues with module loading and dependency-tool behavior.

### Deployment Debugging Funnel

When a target boots unexpected behavior, verify in this order:

```text
source commit and dirty state
  -> effective .config
  -> compiler shown in build log
  -> kernelrelease and artifact hashes
  -> staged module directory
  -> bootloader image/slot selection
  -> loaded DTB
  -> target uname -r and dmesg
```

This prevents hours of debugging source code that the target is not running.

## Production Checklist

Use this checklist before accepting a kernel build or external-module pipeline
for product work.

Target and recovery:

- [ ] Board, architecture, boot chain, image format, DTB, rootfs, and console
      are explicitly identified.
- [ ] A serial console or equivalent early-boot observation path is available.
- [ ] A known-good boot entry, slot, or documented recovery method is
      preserved.

Source and configuration:

- [ ] Source origin, exact commit/tag, downstream patches, and dirty-tree state
      are recorded.
- [ ] Release archives/tags are verified according to project policy.
- [ ] The config starts from a known-good product or maintained baseline.
- [ ] Config migration and new symbols are reviewed rather than silently
      accepted.
- [ ] Boot-critical storage, filesystem, console, and initramfs requirements
      are tested.
- [ ] The effective `.config` and minimized config input are retained as
      appropriate.

Toolchain and build environment:

- [ ] Host tool and target compiler versions satisfy the selected kernel tree.
- [ ] `ARCH`, `CROSS_COMPILE`, `LLVM`, and output paths are explicit and
      consistent.
- [ ] Each architecture/configuration has an isolated output directory.
- [ ] Builds run unprivileged; privilege is limited to controlled deployment.
- [ ] CI/container/build scripts are pinned and version-controlled.
- [ ] Build logs, `kernelrelease`, tool versions, and artifact checksums are
      retained.

Artifacts and staging:

- [ ] The image, DTBs, modules, config, and symbol/debug metadata come from the
      same build contract.
- [ ] Module staging uses `INSTALL_MOD_PATH` and the build's
      `kernelrelease`.
- [ ] Target files are never installed into the host root by accident.
- [ ] Initramfs generation and bootloader updates are explicit product steps.
- [ ] Module signing, trusted keys, lockdown, compression, and packaging policy
      are handled.
- [ ] Signing keys and other sensitive inputs are protected.

Verification:

- [ ] Artifact architecture and type are inspected before deployment.
- [ ] The target boots the intended image, DTB, and module tree.
- [ ] `uname -r`, boot logs, module logs, and artifact identity agree.
- [ ] Rootfs, console, storage, networking, and required hardware are smoke
      tested.
- [ ] External modules are rebuilt and tested for every supported target
      kernel/configuration combination.
- [ ] Rollback is tested, not merely documented.

Maintenance:

- [ ] Vendor BSP divergence and out-of-tree module patches have named owners.
- [ ] Rebase/upgrade testing includes config migration and internal API changes.
- [ ] Reproducibility-sensitive timestamps, build user/host, paths, embedded
      initramfs, and signing inputs are controlled where required.

## Interview Readiness

You should be able to explain the build flow as a chain of decisions, not as a
memorized list of Make commands.

### Beginner

- A native build targets the host architecture; a cross-build produces target
  code using host-run tools.
- `ARCH` selects Kbuild's target architecture rules.
- `CROSS_COMPILE` selects a GNU target-tool prefix.
- A defconfig seeds configuration; `.config` is the resolved build input.
- Kernel image, DTB, and modules should be treated as a matched set.

### Mid-Level

- Trace `CONFIG_FOO=m` through `obj-$(CONFIG_FOO)` to `foo.ko`.
- Explain why `O=` must remain consistent across configuration and build.
- Distinguish source, UAPI headers, a prepared build tree, and a fully built
  tree.
- Explain why external modules use `make -C ... M=... modules`.
- Explain when `modules_prepare` helps and why it does not create
  `Module.symvers`.
- Explain why `INSTALL_MOD_PATH` and `make kernelrelease` matter in
  cross-development.

### Senior

- Define the full build contract beyond a kernel version string.
- Design a reproducible and recoverable build/deployment pipeline.
- Diagnose a module that compiles but fails with invalid format, unknown
  symbol, CRC disagreement, or signature rejection.
- Explain why a new image with an old DTB or module tree can fail in
  subsystem-specific ways.
- Map a Yocto, Buildroot, distribution, or vendor SDK failure back to source,
  output tree, config, toolchain, staging, and deployment identity.
- Challenge the assumption that `make install` universally updates initramfs
  and bootloader state.

Typical traps:

- "`CROSS_COMPILE` is the cross-compiler."
- "The same kernel version means modules are compatible."
- "`make headers_install` prepares headers for external modules."
- "`modules_prepare` produces all symbol-version metadata."
- "`make` always builds the kernel image, modules, and DTBs."
- "`uname -r` names the kernel I am currently building."
- "If a module compiles, it will load."
- "Cleaning is required after every source change."

For focused questions and answer structures, continue with
`interview/02-environment-setup-kernel-source-and-build-flow.md`.

## Kernel Version Notes

- Build prerequisites, minimum tool versions, supported compilers, Kconfig
  symbols, image targets, and artifact paths vary by kernel version and
  configuration. Use the documentation and `make help` from the selected tree.
- GCC/binutils is not the only supported toolchain path. Current kernels
  support Clang/LLVM through Kbuild, but exact architecture support and minimum
  versions must be checked for the target tree.
- External-module invocation has gained additional documented forms in newer
  kernels. The widely recognized `make -C <build> M=<source> modules` form
  expresses the stable core concept: enter the matching configured Kbuild
  environment.
- `modules_prepare` is useful for many external builds but does not generate
  `Module.symvers`; a full matching build is required when complete symbol
  version information is needed.
- Module signing, Secure Boot/lockdown behavior, compression, and packaging
  differ across distributions and products.
- Mainline, stable, and longterm release status and support dates change over
  time. Confirm current kernel.org and vendor policy when selecting a product
  baseline.
