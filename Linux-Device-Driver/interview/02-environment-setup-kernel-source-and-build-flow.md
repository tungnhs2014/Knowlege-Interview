# 02 - Environment Setup, Kernel Source, And Build Flow Interview Questions

This topic tests whether a candidate can create and diagnose a trustworthy kernel build environment. Strong candidates treat a build as a contract among the exact source revision, configuration, toolchain, generated build state, and deployed target artifacts. They do not reduce kernel development to installing a headers package and running `make`.

Good answers distinguish host from target, source trees from output trees, defconfigs from expanded configurations, and successful compilation from target compatibility. They can explain how Kconfig reaches Kbuild, why external modules must enter the matching kernel build system, and how to prove which kernel image, DTB, and module tree are actually running.

## Beginner

### 1. What Are The Host And Target In Kernel Development?

- **Level:** Beginner
- **Question:** Explain the host/target model and the difference between native and cross-compilation.
- **Short Answer:** The host is the machine that runs the editor, build tools, and compiler. The target is the machine that runs the resulting kernel, modules, and hardware description. A native build uses a compiler that produces code for the host-compatible architecture; a cross-build produces code for a different target architecture.
- **Deep Explanation:** The host executes Kbuild, shell tools, Kconfig frontends, code generators, the compiler, assembler, and linker. Some generated programs are host executables and must run during the build, while kernel objects are target binaries and must execute on the target CPU.

  Host and target may both be Linux systems and still require different build contracts. An x86-64 workstation building an arm64 kernel is clearly cross-compiling, but two arm64 systems can also differ in kernel configuration, platform support, ABI details, or deployment layout. The board name alone does not select a compiler or prove compatibility.
- **API / Code Anchor:**
  ```bash
  # Native build: host defaults are commonly sufficient.
  make O=$PWD/out-native defconfig
  make O=$PWD/out-native -j"$(nproc)"

  # GNU arm64 cross-build.
  make O=$PWD/out-arm64 ARCH=arm64 \
       CROSS_COMPILE=aarch64-linux-gnu- defconfig
  make O=$PWD/out-arm64 ARCH=arm64 \
       CROSS_COMPILE=aarch64-linux-gnu- -j"$(nproc)" Image modules dtbs
  ```
  The exact image and DTB targets are architecture- and platform-dependent; use the selected tree's `make help` and architecture documentation.
- **Production or Debugging Angle:** Record the host tools, target architecture, compiler identity, kernel commit, and configuration. When an artifact fails, use `file`, `readelf -h`, and verbose build output to prove which architecture and compiler produced it.
- **Common Traps:**
  - Treating `CROSS_COMPILE` as the compiler executable rather than a tool-name prefix.
  - Assuming the board name lets Kbuild infer `ARCH`.
  - Setting cross-build variables for configuration but omitting them during later build commands.
  - Confusing a target userspace toolchain or sysroot with the kernel build contract.
  - Assuming same-architecture builds are automatically compatible.
- **Follow-up Questions:**
  - Why does a kernel build need both host tools and target tools?
  - Can Clang cross-compile without a GNU-style `CROSS_COMPILE` prefix?
  - How would you verify the architecture of `vmlinux` or a `.ko`?

### 2. How Do You Choose The Correct Kernel Source Tree?

- **Level:** Beginner
- **Question:** When would you use a mainline, stable/longterm, distribution, or vendor kernel tree?
- **Short Answer:** Use the tree that matches the development and deployment goal. Mainline is appropriate for upstream work, stable or longterm for maintained upstream release lines, a distribution tree for that distribution's kernel, and a vendor/BSP tree when the product depends on downstream platform support. In every case, pin an exact tag or commit.
- **Deep Explanation:** A version such as `6.x` is not enough to identify a kernel. Vendor and distribution trees often contain backports, board support, security fixes, and ABI changes that are absent from the upstream tree with a similar version number. An external module built against the wrong tree may compile but fail because structures, exported symbols, configuration, or symbol CRCs differ.

  A release archive provides a fixed released baseline. Git is better when history, patches, rebasing, or upstream contribution matters. Source identity and source authenticity are separate: version fields in the top-level `Makefile` identify what the tree declares, but cryptographic signatures or trusted Git references establish provenance.
- **API / Code Anchor:**
  ```bash
  git remote -v
  git rev-parse HEAD
  git describe --always --dirty
  git status --short

  # Version fields are useful identity clues, not authentication.
  sed -n '1,6p' Makefile
  ```
- **Production or Debugging Angle:** Release metadata should preserve the immutable commit, downstream patches, repository origin, and source verification method. When a target has vendor-only drivers, start from its actual BSP baseline before assuming an upstream release is interchangeable.
- **Common Traps:**
  - Choosing a source tree only because its numeric version resembles `uname -r`.
  - Calling `head Makefile` cryptographic verification.
  - Tracking a moving branch without recording the built commit.
  - Assuming all longterm branches have the same support window.
  - Replacing a vendor tree before identifying its required downstream changes.
- **Follow-up Questions:**
  - What information does `git describe --dirty` add?
  - Why can two kernels with similar version strings expose different APIs?
  - What would you archive with a release tarball to preserve provenance?

### 3. What Is The Difference Between Kconfig, A Defconfig, And `.config`?

- **Level:** Beginner
- **Question:** Explain how Kconfig definitions, defconfigs, and `.config` relate.
- **Short Answer:** Kconfig files define configuration symbols, types, dependencies, defaults, and help. A defconfig is a maintained baseline of selected choices. `.config` is the expanded configuration consumed by a particular build after dependencies and defaults are resolved.
- **Deep Explanation:** Running a defconfig target creates an initial `.config`; it does not build the kernel. Kconfig frontends such as `menuconfig` edit that configuration while enforcing symbol types and dependencies. `olddefconfig` carries an existing configuration forward and accepts defaults for new symbols. `savedefconfig` writes a minimized set of choices that differ from defaults, which is useful for maintaining a compact baseline but is not identical to copying `.config`.

  A symbol may be invisible because its prompt is absent, its dependencies are unmet, or another symbol controls it. Editing `.config` by hand can be overwritten when Kconfig recalculates dependencies, so search/help and the defining Kconfig file are better diagnostic tools.
- **API / Code Anchor:**
  ```bash
  make O=$PWD/out ARCH=arm64 vendor_defconfig
  make O=$PWD/out ARCH=arm64 menuconfig
  make O=$PWD/out ARCH=arm64 olddefconfig
  make O=$PWD/out ARCH=arm64 savedefconfig

  grep '^CONFIG_MY_DRIVER=' out/.config
  rg '^config MY_DRIVER$' .
  ```
- **Production or Debugging Angle:** Preserve both the maintained baseline and the final expanded `.config` used for a release. If an option will not stay enabled, inspect its Kconfig dependencies and use the frontend's search/help rather than repeatedly forcing the text value.
- **Common Traps:**
  - Calling a defconfig a complete, immutable product configuration.
  - Assuming every symbol is visible in `menuconfig`.
  - Copying a `.config` between kernel releases without reconciling new symbols.
  - Believing `savedefconfig` is a byte-for-byte backup of `.config`.
  - Editing `.config` while pointing later commands at a different `O=` directory.
- **Follow-up Questions:**
  - When would you use `oldconfig` instead of `olddefconfig`?
  - Why can `CONFIG_FOO=y` disappear after a configuration update?
  - What should be retained to reproduce a product configuration?

### 4. How Do `Y`, `M`, And `N` Reach Kbuild?

- **Level:** Beginner
- **Question:** Trace a tristate Kconfig selection into built-in code, a module, or no object.
- **Short Answer:** A tristate symbol resolves to `y`, `m`, or `n`. Kbuild commonly connects it with `obj-$(CONFIG_FOO) += foo.o`: `y` adds the object to built-in output, `m` builds a loadable module, and `n` expands to no active object list.
- **Deep Explanation:** Kconfig controls selection, while Kbuild controls compilation and linking. A symbol can become `m` only if it is tristate, module support is enabled, and dependencies permit modular selection. Built-in objects are combined into the kernel link. Modular objects pass through module linking and `modpost` to produce `.ko` files and metadata.

  Directory recursion follows the same idea. A parent Kbuild file may use `obj-$(CONFIG_SUBSYSTEM) += subsystem/`, and a child file selects individual drivers. A Kconfig prompt without corresponding Kbuild wiring changes configuration but produces no driver object.
- **API / Code Anchor:**
  ```makefile
  # Kconfig
  config DEMO_SENSOR
          tristate "Demo sensor"
          depends on I2C

  # Makefile or Kbuild
  obj-$(CONFIG_DEMO_SENSOR) += demo_sensor.o
  ```
  For a multi-file module:
  ```makefile
  obj-$(CONFIG_DEMO_SENSOR) += demo_sensor.o
  demo_sensor-y := demo_core.o demo_bus.o
  ```
- **Production or Debugging Angle:** If expected code is absent, verify the final `.config`, the parent directory recursion, the object rule, and the build log. Boot-critical storage, filesystem, or root-device support may need `y` because modules are unavailable until the root filesystem or initramfs can load them.
- **Common Traps:**
  - Saying `n` maps to `obj-m`; it maps to no selected object.
  - Adding a Kconfig symbol but forgetting Kbuild integration.
  - Selecting `m` for code required before module loading is possible.
  - Assuming `[M]` always saves runtime memory after the module is loaded.
  - Confusing a source filename with the final composite module name.
- **Follow-up Questions:**
  - What happens if `CONFIG_MODULES=n`?
  - Why might a tristate dependency force a symbol from `y` to `m`?
  - How would you build one module from several source files?

### 5. What Are The Main Kernel Build Artifacts?

- **Level:** Beginner
- **Question:** Explain the roles of `vmlinux`, the architecture boot image, `System.map`, DTBs, `.ko` files, and `Module.symvers`.
- **Short Answer:** `vmlinux` is the linked ELF kernel with symbols; the architecture boot image is the form loaded by the boot chain; `System.map` maps kernel symbols to addresses; DTBs describe hardware on Device Tree systems; `.ko` files are loadable modules; and `Module.symvers` records exported symbols and, when enabled, version CRC information.
- **Deep Explanation:** These files are related but not interchangeable. `vmlinux` is valuable for symbolization and debugging but is often not the image copied to the boot partition. The bootable target may be `Image`, `zImage`, `bzImage`, or a platform-specific packaged form. DTBs are compiled separately and must describe hardware compatibly with the deployed kernel and drivers.

  Module builds also produce metadata such as `modules.order` and `modules.builtin*`. Their consumers use them to understand module order and which features are built into the kernel. Artifact names and locations can change across architectures and kernel versions.
- **API / Code Anchor:**
  ```bash
  file out/vmlinux
  readelf -h out/vmlinux
  make O=$PWD/out ARCH=arm64 kernelrelease
  find out -name '*.ko' -o -name '*.dtb'
  ```
- **Production or Debugging Angle:** Archive unstripped debug information or a matching `vmlinux` for crash analysis, even if the deployed image is stripped or compressed. Deploy and identify the kernel image, DTB, module tree, and relevant firmware as one tested set.
- **Common Traps:**
  - Copying `vmlinux` where the bootloader expects another image format.
  - Assuming all ARM systems use `zImage`.
  - Treating `System.map` as the running kernel itself.
  - Deploying a new image with stale DTBs or modules.
  - Deleting `Module.symvers` before building versioned external modules.
- **Follow-up Questions:**
  - Which artifact would you use to symbolize an oops?
  - Why is `Module.symvers` important with `CONFIG_MODVERSIONS`?
  - How do built-in module metadata files help userspace tooling?

### 6. Why Must An External Module Use Kbuild?

- **Level:** Beginner
- **Question:** Why is compiling a module with a direct `gcc -I...` command not equivalent to building it through Kbuild?
- **Short Answer:** A kernel module must use the target kernel's generated configuration, architecture flags, compiler options, headers, symbol metadata, and module post-processing. `make -C <kernel-build> M=<module-source> modules` enters that exact Kbuild environment.
- **Deep Explanation:** Kernel headers are configuration-sensitive, and many required headers are generated in the output tree. Kbuild also applies target-specific flags, creates module metadata, runs `modpost`, checks imported symbols, and links the final `.ko`. Manually adding `include/` paths misses this contract and can silently compile with wrong assumptions.

  `-C` changes into the selected kernel source/build entry point. `M=` tells Kbuild that the external module sources live in an absolute external directory. If the kernel uses a separate output directory, the external build must enter that configured output tree or otherwise use the tree's documented source/output relationship.
- **API / Code Anchor:**
  ```makefile
  # External module Kbuild content
  obj-m += demo.o
  ```
  ```bash
  make -C /lib/modules/$(uname -r)/build M="$PWD" modules

  # Cross-build against a target kernel output tree.
  make -C /work/kernel-out M="$PWD" ARCH=arm64 \
       CROSS_COMPILE=aarch64-linux-gnu- modules
  ```
- **Production or Debugging Angle:** On a distribution system, `/lib/modules/$(uname -r)/build` commonly links to a prepared build tree for the running distribution kernel. In product development, point to the exact target kernel output, not the development host's running kernel.
- **Common Traps:**
  - Using `/usr/include/linux` to build a kernel module.
  - Passing only the kernel source `include/` directory to GCC.
  - Building against `/lib/modules/$(uname -r)/build` when `uname -r` belongs to the host, not the target.
  - Omitting the same architecture and compiler choices used for the kernel.
  - Treating a generated `.o` as a complete `.ko`.
- **Follow-up Questions:**
  - What does `modpost` contribute?
  - Where do generated kernel headers normally live with `O=`?
  - Why can a module compile successfully and still fail to load?

## Mid-Level

### 7. Describe A Disciplined Kernel Configure-And-Build Flow

- **Level:** Mid-Level
- **Question:** Walk through a repeatable kernel build using a separate output directory.
- **Short Answer:** Pin the source and toolchain, create a clean output directory, select `ARCH` and compiler mode consistently, seed a known-good configuration, reconcile it, build explicit artifacts, inspect the resulting release, and stage rather than directly install target files.
- **Deep Explanation:** `O=<dir>` keeps generated files, `.config`, and architecture-specific state outside the source checkout. It allows one source tree to support separate product, architecture, or debug builds. Every configuration and build command must use the same output directory; otherwise the developer may inspect one `.config` while compiling another.

  Kbuild is incremental, so routine source changes normally do not require cleaning. A clean output directory is useful for release verification or when stale generated state is suspected. `make clean` preserves more configuration state than `mrproper`; `mrproper` removes `.config` and additional generated files. With `O=`, removing a disposable output directory is often clearer than cleaning the source tree.
- **API / Code Anchor:**
  ```bash
  src=$PWD
  out=$PWD/out/board-a

  make -C "$src" O="$out" ARCH=arm64 \
       CROSS_COMPILE=aarch64-linux-gnu- vendor_defconfig
  make -C "$src" O="$out" ARCH=arm64 \
       CROSS_COMPILE=aarch64-linux-gnu- olddefconfig
  make -C "$src" O="$out" ARCH=arm64 \
       CROSS_COMPILE=aarch64-linux-gnu- -j"$(nproc)" Image modules dtbs
  make -C "$src" O="$out" ARCH=arm64 kernelrelease
  ```
- **Production or Debugging Angle:** Put the complete command line in scripts or CI and retain the final `.config`, build log, tool versions, `kernelrelease`, and artifact hashes. Build as an unprivileged user and reserve privilege for controlled deployment.
- **Common Traps:**
  - Reusing one output directory for different architectures.
  - Forgetting `O=` on one command.
  - Running `mrproper` and unexpectedly deleting `.config`.
  - Cleaning after every edit and hiding incremental-build problems.
  - Assuming plain `make` always produces the image, modules, and DTBs required by the board.
- **Follow-up Questions:**
  - What does `KBUILD_OUTPUT` provide compared with `O=`?
  - When is a clean rebuild justified?
  - How would you maintain debug and production configurations from one source tree?

### 8. Compare `ARCH`, `CROSS_COMPILE`, And `LLVM`

- **Level:** Mid-Level
- **Question:** What does each variable control, and what does none of them guarantee?
- **Short Answer:** `ARCH` selects the kernel's target architecture and architecture-specific build rules. `CROSS_COMPILE` usually supplies a GNU tool prefix such as `aarch64-linux-gnu-`. `LLVM=1` selects the LLVM toolchain family. None alone proves that the chosen compiler, ABI, configuration, or artifacts match the product.
- **Deep Explanation:** `ARCH` affects paths such as `arch/<arch>/`, architecture Kconfig, defconfigs, and image targets. `CROSS_COMPILE` is prepended to tool names, producing commands such as `aarch64-linux-gnu-gcc` and `aarch64-linux-gnu-ld`; it is not a board identifier. LLVM builds use Kbuild's LLVM support and may use Clang target options or additional cross tools depending on the selected kernel and environment.

  Tool minimum versions and optional dependencies vary by kernel revision and configuration. Enabling BTF, signing, Rust, or particular certificate features can introduce tools not needed by a minimal build. The selected tree's build-requirements documentation is authoritative.
- **API / Code Anchor:**
  ```bash
  make O=$PWD/out-gcc ARCH=arm64 \
       CROSS_COMPILE=aarch64-linux-gnu- V=1 Image

  make O=$PWD/out-llvm ARCH=arm64 LLVM=1 V=1 Image

  aarch64-linux-gnu-gcc --version
  clang --version
  scripts/ver_linux
  ```
- **Production or Debugging Angle:** Use `V=1` to inspect the actual compiler and flags instead of trusting shell variables. Record tool versions and reject accidental compiler changes in CI when reproducibility or qualification matters.
- **Common Traps:**
  - Setting `CROSS_COMPILE` to `...-gcc` instead of the prefix ending in `-`.
  - Assuming one GNU tuple works for every ARM or arm64 target.
  - Mixing GCC-built kernel state with a differently configured LLVM module build without validation.
  - Copying an unsupported `ccache` string into `CROSS_COMPILE`.
  - Ignoring optional host tools activated by configuration.
- **Follow-up Questions:**
  - What evidence in a verbose log proves the selected compiler?
  - Why is the GNU tuple's userspace ABI label not sufficient proof of kernel compatibility?
  - Which configuration features commonly introduce extra build prerequisites?

### 9. Distinguish Kernel Source, UAPI Headers, And A Prepared Build Tree

- **Level:** Mid-Level
- **Question:** Why are a source checkout, `headers_install` output, and a prepared kernel build tree different things?
- **Short Answer:** Source contains kernel code and base headers. `headers_install` exports sanitized userspace ABI headers. A prepared build tree contains the selected `.config`, generated headers, scripts, and build state needed by Kbuild for kernel or external-module compilation.
- **Deep Explanation:** Headers under `include/uapi/` contribute to interfaces visible to userspace and are exported into a sanitized include tree. They intentionally do not expose the complete internal kernel build environment. Internal driver code uses kernel headers plus configuration-generated definitions, architecture headers, compiler settings, and Kbuild processing.

  A distribution's `linux-headers-<release>` package generally provides enough prepared state for external modules targeting that distribution kernel. An arbitrary source tree at the same broad version may not. With a custom kernel, the source and output directories can be separate, so generated headers may live only under the output directory.
- **API / Code Anchor:**
  ```bash
  # Export headers for userspace consumers.
  make O=$PWD/out ARCH=arm64 \
       INSTALL_HDR_PATH=$PWD/stage-uapi headers_install

  # Prepare configured kernel state for many external modules.
  make O=$PWD/out ARCH=arm64 \
       CROSS_COMPILE=aarch64-linux-gnu- modules_prepare
  ```
- **Production or Debugging Angle:** When an external build reports missing generated headers, prove which source and output directories were configured. Do not fix it by adding random include paths; repair the selected Kbuild tree.
- **Common Traps:**
  - Calling `/usr/include/linux` kernel module headers.
  - Expecting `headers_install` to prepare an external-module build.
  - Configuring the source tree but building against an empty output tree.
  - Assuming a headers package for one distribution release matches another.
  - Copying generated headers between unrelated configurations.
- **Follow-up Questions:**
  - Who consumes installed UAPI headers?
  - Which generated files make module compilation configuration-sensitive?
  - How does `/lib/modules/<release>/build` normally help?

### 10. How Do You Integrate A Driver Into The In-Tree Build?

- **Level:** Mid-Level
- **Question:** What changes are normally required to make a new in-tree driver selectable and buildable?
- **Short Answer:** Add a Kconfig definition, connect it through the subsystem's parent Kconfig, add a Kbuild object rule, and ensure the parent directory recurses into the driver's directory. Then test relevant `y`, `m`, and disabled configurations.
- **Deep Explanation:** In-tree integration follows subsystem ownership. The Kconfig symbol expresses dependencies and discoverability; Kbuild maps the resolved value to objects. Parent files may already recurse into the directory, in which case only a local rule is needed. Composite modules list their component objects under a module-specific `-y` or `-objs` variable.

  Dependencies should express compile-time requirements without over-constraining valid configurations. `depends on` controls visibility and legal selection; `select` forces another symbol and can bypass that symbol's dependencies, so it must be used carefully.
- **API / Code Anchor:**
  ```makefile
  # drivers/iio/temperature/Kconfig
  config DEMO_TEMP
          tristate "Demo temperature sensor"
          depends on I2C
          help
            Support the Demo temperature sensor.

  # drivers/iio/temperature/Makefile
  obj-$(CONFIG_DEMO_TEMP) += demo_temp.o
  demo_temp-y := demo_temp_core.o demo_temp_i2c.o
  ```
- **Production or Debugging Angle:** Test compile coverage for built-in, modular, and disabled forms where valid. Also test configurations with dependencies absent; build bots frequently expose incorrect Kconfig assumptions that a developer's full-featured config hides.
- **Common Traps:**
  - Editing the top-level Makefile instead of following subsystem structure.
  - Adding Kconfig without sourcing it from the parent.
  - Using `select` to force symbols with unmet dependencies.
  - Naming a composite module and one component object identically.
  - Testing only the developer's single `.config`.
- **Follow-up Questions:**
  - When should a dependency be `depends on` rather than `select`?
  - How can `COMPILE_TEST` improve coverage?
  - What files would you inspect if a visible symbol produces no object?

### 11. When Is `modules_prepare` Enough?

- **Level:** Mid-Level
- **Question:** Can an external module be built without first completing a full kernel build?
- **Short Answer:** Often yes: a configured tree followed by `modules_prepare` can generate the state needed for many external modules. However, `modules_prepare` does not generate `Module.symvers`; when symbol version CRCs or complete exported-symbol metadata are required, use the matching full kernel build.
- **Deep Explanation:** The blanket statement that every external module needs a complete prebuilt kernel is too strong. Preparation creates generated headers and scripts needed for compilation. But `CONFIG_MODVERSIONS` associates imported and exported symbols with CRCs derived during a full build. Without the correct `Module.symvers`, `modpost` may warn, omit expected version data, or produce a module that the target rejects.

  External modules that depend on symbols exported by another external module need coordinated symbol metadata as well. Kbuild supports building related modules together, and `KBUILD_EXTRA_SYMBOLS` can provide additional symvers files when that design is intentional.
- **API / Code Anchor:**
  ```bash
  make O=$PWD/out ARCH=arm64 vendor_defconfig
  make O=$PWD/out ARCH=arm64 modules_prepare
  test -f out/include/generated/autoconf.h

  make -C "$PWD/out" M=/work/demo-module \
       ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules

  # Required when complete CONFIG_MODVERSIONS metadata matters:
  make O=$PWD/out ARCH=arm64 \
       CROSS_COMPILE=aarch64-linux-gnu- -j8
  test -f out/Module.symvers
  ```
- **Production or Debugging Angle:** Treat missing or stale `Module.symvers` as a build-contract problem, not a warning to suppress. Preserve the symvers file from the exact kernel build used for the target image.
- **Common Traps:**
  - Claiming `modules_prepare` always creates `Module.symvers`.
  - Copying `Module.symvers` from a similar kernel.
  - Ignoring `modpost` warnings about undefined symbols.
  - Building mutually dependent external modules independently without sharing symbol metadata.
  - Assuming successful linking proves target load compatibility.
- **Follow-up Questions:**
  - What does `CONFIG_MODVERSIONS` protect against?
  - How would two external module projects share exported-symbol information?
  - Why might `Module.symvers` exist but still be wrong?

### 12. How Should Kernel Modules Be Staged For A Target?

- **Level:** Mid-Level
- **Question:** Why should a cross-built module installation use `INSTALL_MOD_PATH` and the build's `kernelrelease`?
- **Short Answer:** `INSTALL_MOD_PATH` redirects installation into a target rootfs staging directory instead of the host. Kbuild installs beneath `lib/modules/<kernelrelease>/`, where `<kernelrelease>` must match the kernel build being deployed, not necessarily the host's `uname -r`.
- **Deep Explanation:** The target module directory is derived from the new build's release string, including local-version settings and other release components. During cross-development, `uname -r` reports the host's running kernel and is therefore the wrong directory selector. Staging allows packaging tools or image builders to consume the files without modifying the development workstation.

  Module installation can also involve dependency indexes, signing, stripping, or compression policies. The final rootfs or initramfs must contain the modules needed at the time they are required. Boot-critical modular drivers must be available before mounting the real root filesystem.
- **API / Code Anchor:**
  ```bash
  release=$(make -s O=$PWD/out ARCH=arm64 kernelrelease)

  make O=$PWD/out ARCH=arm64 \
       CROSS_COMPILE=aarch64-linux-gnu- \
       INSTALL_MOD_PATH=$PWD/stage-rootfs modules_install

  find "$PWD/stage-rootfs/lib/modules/$release" -maxdepth 2 -type f
  ```
- **Production or Debugging Angle:** Package or hash the staged module tree with the image and DTBs. Verify the final image, not only the staging directory, because initramfs generation or rootfs packaging may omit, recompress, or sign modules.
- **Common Traps:**
  - Installing target modules into the host's `/lib/modules`.
  - Using `$(uname -r)` for a kernel that has not booted yet.
  - Forgetting that `CONFIG_LOCALVERSION` changes the directory name.
  - Updating the rootfs module tree but not an initramfs that contains old modules.
  - Assuming `modules_install` updates every product-specific boot artifact.
- **Follow-up Questions:**
  - How would you determine the release string before booting the new kernel?
  - When must a module be included in an initramfs?
  - What role does `depmod` play in the final target image?

### 13. How Do You Diagnose A Failed Parallel Kernel Build?

- **Level:** Mid-Level
- **Question:** A `make -j32` build ends with several errors. How do you find the root cause?
- **Short Answer:** Preserve the complete log and find the earliest substantive compiler, Kconfig, tool, or linker error rather than the final cascading `make` failure. Rebuild the failing target with lower parallelism or `-j1` and `V=1` to expose the exact command.
- **Deep Explanation:** Parallel output is interleaved. The last line often reports only that a parent target failed after a child command had already emitted the useful diagnostic. Missing tools can also cause generated-file failures later, while one syntax error can produce many dependent errors.

  Avoid immediately cleaning the tree. First inspect the configuration, selected compiler, source revision, output directory, and command line. A targeted rebuild can be faster, but final verification should still exercise the complete required build because individual object compilation does not run every link or `modpost` check.
- **API / Code Anchor:**
  ```bash
  make O=$PWD/out ARCH=arm64 -j32 Image modules 2>&1 | tee build.log
  rg -n 'error:|fatal:|No such file|not found|undefined reference' build.log

  make O=$PWD/out ARCH=arm64 -j1 V=1 Image
  make O=$PWD/out ARCH=arm64 help
  ```
- **Production or Debugging Angle:** CI should retain full logs and identify the first failing command. Capture environment and tool versions alongside the log so a failure can be reproduced outside the worker.
- **Common Traps:**
  - Reporting only `make: *** ... Error 2`.
  - Deleting the output directory before collecting evidence.
  - Fixing a later generated-file error while ignoring an earlier missing tool.
  - Assuming a specific directory/object build validates final linking.
  - Leaving `V=1` permanently enabled in noisy routine logs instead of using it diagnostically.
- **Follow-up Questions:**
  - Why can lowering parallelism change the visible error order?
  - What information does `V=1` reveal?
  - When would a clean rebuild add useful evidence?

## Senior

### 14. What Defines A Reproducible Kernel Build?

- **Level:** Senior
- **Question:** Which inputs must be controlled to reproduce and audit a production kernel build?
- **Short Answer:** Control the exact source and patches, expanded configuration, toolchain and host tools, build scripts and environment, architecture/compiler variables, generated inputs, signing material, timestamps and identity fields where relevant, and the packaging/deployment steps. Retain hashes and provenance for the resulting artifacts.
- **Deep Explanation:** Source plus `.config` is necessary but not sufficient. Compiler versions can change code generation and accepted options. Build user, host, timestamps, paths, embedded initramfs contents, certificates, and generated files can affect output. Vendor build systems may inject fragments, local versions, command-line options, firmware, or post-processing after Kbuild completes.

  Reproducibility has two useful levels: functional reproducibility, where equivalent artifacts behave the same, and bit-for-bit reproducibility, where controlled inputs produce identical bytes. A product may require both reproducibility and traceability even when protected signing keys or timestamp policies prevent naive byte equality.
- **API / Code Anchor:**
  ```bash
  git rev-parse HEAD
  sha256sum out/.config out/vmlinux out/arch/arm64/boot/Image
  make O=$PWD/out -s kernelrelease
  ${CROSS_COMPILE}gcc --version

  # Common Kbuild identity controls when required by the release process.
  export KBUILD_BUILD_TIMESTAMP='2026-06-06 00:00:00 +0000'
  export KBUILD_BUILD_USER=builder
  export KBUILD_BUILD_HOST=ci
  ```
- **Production or Debugging Angle:** Generate a release manifest that ties source, patches, config, tool versions, build command, release string, signatures, and hashes to the deployed image. Rebuild in a controlled environment and compare both binaries and observable metadata.
- **Common Traps:**
  - Claiming a container alone guarantees reproducibility.
  - Saving only a defconfig and not the expanded release `.config`.
  - Forgetting embedded initramfs or certificate inputs.
  - Using moving package repositories or branches without lock data.
  - Publishing stripped images without retaining matching debug symbols.
- **Follow-up Questions:**
  - Why can absolute build paths affect output?
  - How would you handle private signing keys in a reproducible pipeline?
  - What evidence connects a field failure to one release manifest?

### 15. Why Does A Matching Version String Not Prove Module Compatibility?

- **Level:** Senior
- **Question:** `uname -r` and a module's apparent release match. Why might the module still be rejected or malfunction?
- **Short Answer:** Release text is only one compatibility signal. The module may differ in configuration, exported symbols, symbol-version CRCs, compiler features, architecture details, signing policy, or downstream API patches. The target log and exact build inputs decide the diagnosis.
- **Deep Explanation:** `kernelversion` is the base version declared by the source Makefile, while `kernelrelease` includes local and generated release components used for module directories and metadata. `uname -r` reports the running kernel's release. Matching these strings does not prove that the module used the same `.config`, `Module.symvers`, source commit, or signing trust.

  `vermagic` records selected build characteristics, but it is not a complete ABI proof. With `CONFIG_MODVERSIONS`, imported symbol CRCs add another check. Distribution kernels may also enforce module signatures or maintain a private kABI policy. A forced load bypasses protections and can turn a clear rejection into memory corruption.
- **API / Code Anchor:**
  ```bash
  uname -r
  make O=$PWD/out -s kernelversion
  make O=$PWD/out -s kernelrelease
  modinfo -F vermagic ./demo.ko
  modinfo -F signer ./demo.ko
  modinfo -F sig_id ./demo.ko
  dmesg | tail -100
  ```
- **Production or Debugging Angle:** Diagnose from the target's exact rejection message: version magic disagreement, unknown symbol, symbol-version disagreement, wrong ELF machine, bad signature, or missing key each points to a different layer. Never normalize forced module loading as a deployment strategy.
- **Common Traps:**
  - Treating `kernelversion`, `kernelrelease`, and `uname -r` as synonyms.
  - Assuming vermagic equality proves binary compatibility.
  - Rebuilding only the module after changing exported kernel interfaces.
  - Ignoring target signature enforcement or compressed-module packaging.
  - Using `--force-vermagic` or forced load to hide a build mismatch.
- **Follow-up Questions:**
  - Which setting commonly changes the suffix in `kernelrelease`?
  - How do symbol CRCs improve detection of incompatible imports?
  - What target evidence distinguishes signature rejection from vermagic failure?

### 16. How Do You Safely Migrate A Product Configuration?

- **Level:** Senior
- **Question:** You are moving a product `.config` to a newer kernel release. What process would you use?
- **Short Answer:** Start from the known-good product configuration, run migration in a separate output directory, review every new or changed symbol, compare the final configuration, build the complete artifact set, and test boot-critical and hardware paths. Do not blindly accept defaults and call the result equivalent.
- **Deep Explanation:** Kconfig symbols are added, renamed, split, removed, or given new dependencies over time. `olddefconfig` is useful automation, but accepting defaults can disable required hardware or enable behavior with security, size, power, or ABI consequences. `oldconfig` allows interactive decisions; `listnewconfig` and configuration diffs help review additions.

  Configuration fragments can express policy, but merge order and dependencies matter. The final expanded `.config` remains the build truth. Migration must include image size, rootfs access, console, storage, filesystems, networking, firmware loading, module policy, DT compatibility, and recovery.
- **API / Code Anchor:**
  ```bash
  cp known-good.config out-new/.config
  make O=$PWD/out-new ARCH=arm64 listnewconfig
  make O=$PWD/out-new ARCH=arm64 oldconfig
  make O=$PWD/out-new ARCH=arm64 savedefconfig

  scripts/diffconfig out-old/.config out-new/.config
  ```
- **Production or Debugging Angle:** Review config changes like source changes. Automate required-symbol assertions for boot-critical features and run target boot, suspend, peripheral, and module tests before replacing a qualified baseline.
- **Common Traps:**
  - Running `olddefconfig` without reviewing its decisions.
  - Comparing only minimized defconfigs.
  - Assuming renamed symbols will migrate automatically.
  - Missing a built-in-to-module change on the root-device path.
  - Mixing configuration migration with unrelated toolchain and BSP changes in one untraceable step.
- **Follow-up Questions:**
  - What risks come from accepting defaults for all new symbols?
  - How would you assert that required options remain built in?
  - Why should config migration use a fresh output directory?

### 17. How Do Yocto, Buildroot, Distribution Packaging, And Vendor SDKs Map To Kbuild?

- **Level:** Senior
- **Question:** A product build does not invoke your manual kernel commands directly. How do you reason about its kernel build?
- **Short Answer:** Higher-level build systems fetch and patch a kernel source, construct a configuration, choose toolchains and Kbuild variables, select an output directory, invoke Kbuild, then package and deploy its artifacts. Debug by identifying those concrete inputs and paths rather than bypassing the build system.
- **Deep Explanation:** Yocto/OpenEmbedded recipes, Buildroot packages, distribution packaging, and vendor SDK wrappers add orchestration around the same kernel mechanisms. They may merge config fragments, set local version strings, provide cross-toolchain paths, stage modules into an image, generate initramfs content, sign artifacts, and package DTBs.

  A manual build can be useful to isolate a compiler error, but shipping an artifact built outside the product pipeline risks missing patches, configuration fragments, signing, post-processing, dependency generation, or image integration. The senior task is to translate wrapper behavior back into source identity, output tree, `.config`, Kbuild command, and final package.
- **API / Code Anchor:**
  ```text
  orchestrator
      -> fetch source + apply patches
      -> merge configuration
      -> set ARCH/compiler/O=
      -> invoke Kbuild
      -> stage modules/DTBs/image
      -> sign/package/generate initramfs
      -> assemble deployable image
  ```
  Useful evidence remains:
  ```bash
  git rev-parse HEAD
  grep '^CONFIG_' <actual-kernel-output>/.config
  make -C <source> O=<actual-kernel-output> -s kernelrelease
  ```
- **Production or Debugging Angle:** Capture the wrapper's expanded task log and environment, then reproduce the failing Kbuild command within the same workspace. Submit the fix through the owning recipe or layer so CI and release packaging retain it.
- **Common Traps:**
  - Editing a temporary work directory that the next build regenerates.
  - Installing a manually built `.ko` into a product image without recipe ownership.
  - Inspecting the source-tree `.config` when the real config is in a generated output directory.
  - Assuming the SDK's sysroot is automatically the kernel build tree.
  - Debugging Kbuild while ignoring a later packaging or signing failure.
- **Follow-up Questions:**
  - Where would you look for the final expanded configuration?
  - How can a config fragment appear enabled yet be absent from final `.config`?
  - When is a manual Kbuild reproduction useful?

### 18. Design A Recoverable Production Deployment Pipeline

- **Level:** Senior
- **Question:** How would you deploy a new kernel, DTBs, and modules to remote or hard-to-access targets without turning one bad build into an unrecoverable device?
- **Short Answer:** Deploy a versioned, matched, authenticated artifact set through an atomic or A/B update mechanism; keep a known-good boot path; validate before committing; preserve early-boot logs and rollback controls; and never overwrite the only bootable artifacts during development.
- **Deep Explanation:** A kernel update can fail before normal userspace or networking starts. Recovery therefore cannot depend solely on the new root filesystem. The bootloader or platform update mechanism should distinguish candidate and known-good slots, track boot attempts, and commit only after health checks.

  The update unit must include every coupled artifact: boot image, DTBs, modules, initramfs, firmware where required, bootloader metadata, and signatures. The pipeline should reject mixed release identifiers and hashes. Serial console, pstore, watchdog reset reasons, or another early-boot channel provides evidence when the candidate fails.
- **API / Code Anchor:**
  ```text
  build manifest
      -> sign and stage image + DTBs + modules + initramfs
      -> install candidate slot
      -> boot candidate with attempt counter
      -> verify kernel release and critical devices
      -> mark slot good
      -> otherwise roll back to known-good slot
  ```
  Basic target checks may include:
  ```bash
  uname -r
  cat /proc/cmdline
  dmesg | head -100
  test -d "/lib/modules/$(uname -r)"
  ```
- **Production or Debugging Angle:** Exercise rollback deliberately, including a kernel that cannot mount root, a bad DTB, a missing storage module, and a signature failure. A recovery design that has never been failure-tested is only an assumption.
- **Common Traps:**
  - Assuming `make install` universally updates the bootloader and initramfs.
  - Updating the kernel image without its matching DTB and module tree.
  - Keeping rollback files on storage that the failed kernel cannot access.
  - Declaring success merely because the kernel reached userspace once.
  - Lacking immutable artifact identity in target diagnostics.
- **Follow-up Questions:**
  - Which health checks are required before committing an update?
  - How would you recover from a root-filesystem driver configured as a module but absent from initramfs?
  - What early-boot evidence survives an automatic rollback?

## Debugging Scenarios

### Scenario A: The Build Uses The Wrong Compiler Or Architecture

- **Level:** Mid-Level
- **Question:** An arm64 board image build fails, and the log unexpectedly shows host `gcc`. What do you inspect and correct?
- **Short Answer:** Verify the actual Kbuild command, `ARCH`, compiler mode, cross-tool prefix or LLVM settings, tool availability, and output-directory history. Reconfigure or rebuild in a clean arm64 output directory using one consistent variable set.
- **Deep Explanation:** Shell exports may be missing in a new terminal, overridden by a wrapper, or applied only to one command. A reused output directory may contain configuration generated for another architecture. `V=1` reveals the compiler Kbuild actually invokes. `command -v` and `--version` prove whether the intended tools exist.

  Do not infer success from filenames. Inspect ELF machine fields in generated objects and images. If architecture state was mixed, a fresh output directory is safer than trying to identify every contaminated generated file.
- **API / Code Anchor:**
  ```bash
  command -v aarch64-linux-gnu-gcc
  aarch64-linux-gnu-gcc --version

  make O=$PWD/out-arm64 ARCH=arm64 \
       CROSS_COMPILE=aarch64-linux-gnu- V=1 Image

  file out-arm64/vmlinux
  readelf -h out-arm64/vmlinux | rg 'Class|Machine'
  ```
- **Production or Debugging Angle:** Encode architecture and toolchain in the output-directory name and build script. CI should fail if the verbose compiler identity or resulting ELF machine differs from the target manifest.
- **Common Traps:**
  - Re-exporting variables without checking the actual log.
  - Reusing an x86 output directory for arm64.
  - Setting `CROSS_COMPILE` only during `defconfig`.
  - Treating a missing cross-compiler as a kernel source error.
  - Copying a host-built external module to the target for testing.
- **Follow-up Questions:**
  - Which variable selects `arch/arm64/`?
  - What would change for an LLVM build?
  - Why should the output directory be replaced after mixed-architecture use?

### Scenario B: A Driver Option Is Missing Or Will Not Stay Enabled

- **Level:** Mid-Level
- **Question:** `CONFIG_DEMO_SENSOR=y` is added to `.config`, but after `olddefconfig` it disappears. How do you debug it?
- **Short Answer:** Find the symbol's Kconfig definition, inspect its type and dependencies, use Kconfig search/help, and verify the correct output tree. Enable the prerequisites or choose a legal tristate value instead of forcing `.config`.
- **Deep Explanation:** Kconfig recalculates values according to dependencies. A symbol may be unavailable because its bus, subsystem, architecture, or expert option is disabled. A tristate dependency built as `m` can prevent a dependent driver from becoming `y`. The symbol may also have been renamed or removed in the selected source revision.

  If the symbol remains selected but no object appears, continue into Kbuild: check the corresponding `obj-$(CONFIG_*)` rule and parent recursion. Configuration and compilation are distinct stages.
- **API / Code Anchor:**
  ```bash
  rg -n '^config DEMO_SENSOR$|CONFIG_DEMO_SENSOR' .
  grep '^CONFIG_DEMO_SENSOR' out/.config
  make O=$PWD/out menuconfig

  # In menuconfig, use "/" to search and inspect dependency expressions.
  make O=$PWD/out V=1 drivers/<subsystem>/
  ```
- **Production or Debugging Angle:** Add automated checks for required product symbols after configuration generation. Fail before compilation when a boot-critical symbol is absent or modularized unexpectedly.
- **Common Traps:**
  - Repeatedly editing `.config` by hand.
  - Looking at `source/.config` while building with `O=out`.
  - Confusing an unmet dependency with a compiler bug.
  - Using `select` as a quick workaround for dependency design.
  - Stopping after configuration even though Kbuild wiring is missing.
- **Follow-up Questions:**
  - How does a tristate dependency constrain a dependent symbol?
  - What proves that the selected object was compiled?
  - How would you enforce required configuration in CI?

### Scenario C: An External Module Builds But The Target Rejects It

- **Level:** Senior
- **Question:** A `.ko` builds successfully, but target insertion reports `Invalid module format`. Give a structured diagnosis.
- **Short Answer:** Read the target `dmesg`, then compare architecture, `kernelrelease`, vermagic, source/config identity, `Module.symvers`, compiler mode, and signature policy. Rebuild against the exact target kernel output and redeploy through the target's normal packaging path.
- **Deep Explanation:** The userspace error is generic; the kernel log usually names the actual disagreement. ELF machine mismatch indicates the wrong target compiler. Version magic points toward release/config/tool differences. Symbol-version failures suggest stale or missing `Module.symvers`. Signature messages indicate enforcement or key problems.

  A matching filename or broad version is weak evidence. Confirm the target is running the expected image and that an old module is not being loaded from an initramfs or another module directory. `modinfo` inspects metadata before loading, while `dmesg` reports the loader's decision.
- **API / Code Anchor:**
  ```bash
  dmesg | tail -100
  uname -r
  file ./demo.ko
  modinfo -F vermagic ./demo.ko
  modinfo -F signer ./demo.ko
  readelf -h ./demo.ko

  make -C /work/exact-target-kernel-out M="$PWD" \
       ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules
  ```
- **Production or Debugging Angle:** Package modules by immutable kernel build identity and test load them on the final signed image. Preserve the target log in failure reports; "`insmod` failed" is not enough evidence.
- **Common Traps:**
  - Rebuilding against the host's `/lib/modules/$(uname -r)/build`.
  - Looking only at `modinfo` and not target `dmesg`.
  - Copying a similarly named `Module.symvers`.
  - Bypassing checks with forced loading.
  - Forgetting that the target may load an older module from initramfs.
- **Follow-up Questions:**
  - How would an unknown-symbol failure differ from invalid vermagic?
  - Why can two modules with matching vermagic still be incompatible?
  - What evidence proves which module file the target attempted to load?

### Scenario D: The New Kernel Boots But The Driver Change Is Absent

- **Level:** Senior
- **Question:** The target reports the expected `uname -r`, but behavior still matches the old driver. How do you prove which artifacts and code paths are active?
- **Short Answer:** Verify the changed driver was selected and built, determine whether it is built-in or modular, identify the exact loaded module or built-in image, compare hashes and timestamps only as supporting clues, and verify matching DTB, initramfs, and rootfs contents. Then prove execution with targeted logs or tracing.
- **Deep Explanation:** A release string can remain unchanged across rebuilds. A modular driver may still come from an old initramfs or rootfs path. A built-in driver requires the new kernel image, while a binding or hardware-description change may require a new DTB. The deployed module can also be correct but never bind because the old DTB lacks the device or compatible string.

  Start at build evidence: final `.config`, object/module output, and build log. Continue through packaging manifests and bootloader-selected paths. Finally inspect runtime binding and add a distinctive, temporary device-scoped log or tracepoint to prove the changed path executes.
- **API / Code Anchor:**
  ```bash
  grep '^CONFIG_DEMO_DRIVER=' out/.config
  find out -name '*demo*.ko' -o -name 'built-in.a'
  sha256sum deployed/Image deployed/*.dtb deployed/demo.ko

  uname -r
  cat /proc/cmdline
  modinfo demo
  readlink /sys/bus/<bus>/devices/<device>/driver
  dmesg | rg 'demo|Linux version'
  ```
- **Production or Debugging Angle:** Give every release a manifest and make the target expose the selected slot, image identity, and module package version. Avoid relying only on unchanged release strings or file modification times.
- **Common Traps:**
  - Assuming `uname -r` uniquely identifies one binary build.
  - Replacing a `.ko` when the driver is built in.
  - Updating the rootfs but not the initramfs.
  - Forgetting a stale DTB can prevent matching or provide old properties.
  - Adding broad logging before proving the expected artifact was deployed.
- **Follow-up Questions:**
  - How can you tell whether a driver is built in?
  - Why can the correct module load without `probe()` running?
  - Which artifact identities should a target health report expose?
