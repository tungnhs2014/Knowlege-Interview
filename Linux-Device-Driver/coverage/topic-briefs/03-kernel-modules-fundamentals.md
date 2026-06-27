# Topic Brief - 03 - Kernel Modules Fundamentals

## Output Targets
- Knowledge: `knowledge/03-kernel-modules-fundamentals.md`
- Interview: `interview/03-kernel-modules-fundamentals.md`
- Example: `examples/03-kernel-modules-fundamentals/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch01` | `docs/Linux Device Driver Development/Chapter 1-Introduction to Kernel Development .md` | read/mapped/merged | Supporting build context: `make modules`, `modules_install`, `INSTALL_MOD_PATH`, cross-compile variables, and kernel source layout. |
| `ldd1-ch02` | `docs/Linux Device Driver Development/Chapter 2-Device Driver Basis.md` | read/covered/merged | Main book source for module concept, dependencies, `depmod`, `insmod`, `modprobe`, auto-loading, unload/reference count, skeleton, `module_init()`, `module_exit()`, `__init`, `__exit`, `.modinfo`, `MODULE_*`, module parameters, Kbuild, in-tree/out-of-tree builds, and cross-compilation. |
| `ldd1-ch05` | `docs/Linux Device Driver Development/Chapter 5-Platform Device Drivers.md` | read/mapped/merged-adjacent | Adds real-driver context for bus registration helpers such as `module_platform_driver()`, explains `probe()` is not `module_init()`, and shows `MODULE_DEVICE_TABLE()`/`MODULE_ALIAS()` feeding `modules.alias`. |
| `ldd2-ch01` | `docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md` | read/mapped/adjacent | No direct module-fundamentals coverage; its locking/workqueue/IRQ content belongs to later learning-path topics 05, 06, and 15. |
| `ldd2-ch11` | `docs/Linux Device Driver Development 2/Chapter 11-Writing_PCI_Device_Drivers.md` | read/mapped/merged-adjacent | Adds subsystem example of `MODULE_DEVICE_TABLE()`, `modules.alias`, hotplug/udev-driven autoload, and `module_pci_driver()` as a subsystem wrapper around init/exit registration. |
| `ldd2-ch14` | `docs/Linux Device Driver Development 2/Chapter 14-Linux_Kernel_Debugging_Tips.md` | read/mapped/merged-adjacent | Adds debug context for crashes during `insmod`, `init_module`/`cleanup_module` symbols, and `objdump -fS` on `.ko` files. Detailed tracing belongs to topic 37. |
| `notion-ch01-part2` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 2 Kernel Configuration.md` | read/mapped/merged | Clarifies menuconfig module choices (`Y`, `M`, `N`) and important module config options: loadable modules, module unloading, forced unloading, versioning, and source checksums. |
| `notion-ch01-part3` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 3 Building the Kernel.md` | read/mapped/merged | Expanded build/install flow for `make modules`, `modules_install`, target rootfs `INSTALL_MOD_PATH`, native/cross builds, and generated `.ko` placement. |
| `notion-ch01-part4` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 4 Coding Style and Best Practices .md` | read/mapped/merged-adjacent | Include ordering, `static` for internal helpers, `EXPORT_SYMBOL()` for module-visible APIs, load/unload testing checklist, and required module attribution macros. |
| `notion-ch02-part1` | `docs/Linux-Device-Driver-Notion/Chapter 2-Part 1 User Space vs Kernel Space & Module Concept.md` | read/covered/merged | Strong beginner explanation of modules as dynamic kernel extensions, lifecycle, `CONFIG_MODULES`, static vs dynamic drivers, module locations, states, and module-vs-driver distinction. |
| `notion-ch02-part2` | `docs/Linux-Device-Driver-Notion/Chapter 2-Part 2 Module Dependencies, Loading & Information.md` | read/covered/merged | Expanded source for dependencies, `EXPORT_SYMBOL()`/`EXPORT_SYMBOL_GPL()`, `depmod`, `modules.dep`, `insmod` vs `modprobe`, `rmmod` vs `modprobe -r`, refcounts, boot loading, hotplug, `MODULE_*`, `modinfo`, `.modinfo`, and `/sys/module`. |
| `notion-ch02-part3` | `docs/Linux-Device-Driver-Notion/Chapter 2-Part 3 Error Handling & Message Printing.md` | read/mapped/adjacent | Mostly topic 04; useful only for module-load debug symptoms such as out-of-tree/signature taint messages and dynamic debug commands using module names. |
| `notion-ch02-part4` | `docs/Linux-Device-Driver-Notion/Chapter 2-Part 4 Module Parameters & Building Your First Mod.md` | read/covered/merged | Main source for `module_param()`, `MODULE_PARM_DESC()`, supported types, parameter permissions, arrays, load-time parameters, `/etc/modprobe.d`, `/sys/module/.../parameters`, out-of-tree Makefile, in-tree Kconfig/Makefile, install, and troubleshooting. |
| `notion-ch05-part1` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 1 Platform Bus & Driver Basics.md` | read/mapped/merged-adjacent | Beginner-friendly explanation of `module_platform_driver()` expansion, manual registration with `module_init()`/`module_exit()`, `probe()` vs module init, and load/unload behavior for matching devices. |

## Source Files Read
- `docs/Linux Device Driver Development/Chapter 1-Introduction to Kernel Development .md`
  - Relevant sections: Environment setup, Kernel configuration, Building your kernel.
- `docs/Linux Device Driver Development/Chapter 2-Device Driver Basis.md`
  - Relevant sections: User space and kernel space, The concept of modules, Module dependencies, depmod utility, Module loading and unloading, Manual loading, modprobe and insmod, Auto-loading, Module unload, Driver skeletons, Module entry and exit point, `__init` and `__exit` attributes, Module information, Licensing, Module author(s), Module description, Module parameters, Building your first module, The module's makefile, In the kernel tree, Out of the tree, Building the module.
- `docs/Linux Device Driver Development/Chapter 5-Platform Device Drivers.md`
  - Relevant sections: Platform drivers, `module_platform_driver()`, `probe()` is not a substitute for init, bus-specific `module_*_driver()` macros, Devices/drivers/bus matching, `MODULE_DEVICE_TABLE()`, `MODULE_ALIAS()`.
- `docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md`
  - Relevant check: chapter heading and early technical scope; no direct module-fundamentals content found.
- `docs/Linux Device Driver Development 2/Chapter 11-Writing_PCI_Device_Drivers.md`
  - Relevant sections: `struct pci_device_id`, `MODULE_DEVICE_TABLE()`, `struct pci_driver`, Registering a PCI driver.
- `docs/Linux Device Driver Development 2/Chapter 14-Linux_Kernel_Debugging_Tips.md`
  - Relevant sections: oops context showing `insmod`, Trace dump on oops, using `objdump` on kernel modules, `init_module` and `cleanup_module` disassembly.
- `docs/Linux-Device-Driver-Notion/Chapter 1-Part 2 Kernel Configuration.md`
  - Relevant sections: menuconfig module choices, Essential Kernel Options -> Module Support.
- `docs/Linux-Device-Driver-Notion/Chapter 1-Part 3 Building the Kernel.md`
  - Relevant sections: Understanding the Build System, Build Targets, Native Compilation modules, Cross-Compilation modules, Install Modules, `INSTALL_MOD_PATH`.
- `docs/Linux-Device-Driver-Notion/Chapter 1-Part 4 Coding Style and Best Practices .md`
  - Relevant sections: Header files/include order, Static Functions, Code Quality Checklist.
- `docs/Linux-Device-Driver-Notion/Chapter 2-Part 1 User Space vs Kernel Space & Module Concept.md`
  - Relevant sections: The Concept of Modules, Module Requirement: `CONFIG_MODULES`, Module Architecture, Static vs Dynamic Modules, Module Naming Convention, Where Modules Live, Module States, Module vs Driver.
- `docs/Linux-Device-Driver-Notion/Chapter 2-Part 2 Module Dependencies, Loading & Information.md`
  - Relevant sections: Module Dependencies, Symbol Export/Import Mechanism, `depmod`, Module Loading and Unloading, Checking Loaded Modules, Module Unloading, Reference Counting, Boot-Time Loading, Auto-Loading, Module Information, `MODULE_LICENSE`, `modinfo`, `.modinfo`, sysfs Module Interface.
- `docs/Linux-Device-Driver-Notion/Chapter 2-Part 3 Error Handling & Message Printing.md`
  - Relevant check: message-printing/debug sections include taint messages and dynamic debug by module; main error/logging content belongs to topic 04.
- `docs/Linux-Device-Driver-Notion/Chapter 2-Part 4 Module Parameters & Building Your First Mod.md`
  - Relevant sections: Module Parameters, declaring parameters, supported types, permissions, array parameters, loading modules with parameters, viewing parameters, Building Your First Module, out-of-tree building, Kbuild, testing, cross-compilation, multi-file modules, in-tree building, installation, troubleshooting, best practices.
- `docs/Linux-Device-Driver-Notion/Chapter 5-Part 1 Platform Bus & Driver Basics.md`
  - Relevant sections: `probe()` Function, `remove()` Function, Driver Registration, manual registration, `module_platform_driver()`, complete basic platform driver example, platform device ID table, `MODULE_DEVICE_TABLE(platform, ...)`.

## Merged Source Notes
- `ldd1-ch02` is the compact canonical backbone for this topic. It should define what a module is, why `CONFIG_MODULES` matters, how dependencies are represented by exported symbols, and how `insmod`, `modprobe`, `rmmod`, `modprobe -r`, `lsmod`, `depmod`, `modules.dep`, and `modules.alias` relate.
- Notion chapter 2 parts 1, 2, and 4 should provide most beginner-facing explanations, diagrams, command workflows, and troubleshooting examples. They expand the book with practical `modinfo`, `/proc/modules`, `/sys/module`, `/etc/modules-load.d`, `/etc/modprobe.d`, module parameter, and out-of-tree Makefile material.
- `ldd1-ch01`, Notion chapter 1 part 2, and Notion chapter 1 part 3 should be used only as supporting build/config context. The learner-facing chapter should not become a full kernel-build lesson; detailed setup/build flow belongs to topic 02.
- `ldd2-ch11` should contribute only the generalizable autoloading idea behind `MODULE_DEVICE_TABLE()` and `modules.alias`. PCI specifics belong to topic 31.
- `ldd1-ch05` and `notion-ch05-part1` should contribute only the general real-driver pattern: many subsystem drivers do not hand-write init/exit when their only module-level work is registering/unregistering a bus driver. The learner-facing chapter should mention wrapper macros and the `probe()` vs `module_init()` distinction, then defer platform-driver mechanics to topic 09.
- `ldd2-ch14` should contribute only a small debugging note: module init failures happen in the context of the loading process, often visible as `insmod` in oops output; `objdump` can inspect module sections and symbol offsets. Full crash/tracing workflow belongs to topic 37.
- `notion-ch02-part3` and broader logging/error handling are adjacent. Use only load-time debug clues here; move `pr_*`, `dev_*`, errno, cleanup labels, and dynamic debug details to topic 04.

## Source Differences
- `ldd1-ch02` says `__init` code is freed after the init function finishes, then clarifies this applies to built-in drivers, not loadable modules. The learner-facing chapter should state this carefully: built-in init memory can be discarded after boot; for loadable modules, section attributes still mark sections but the module remains unloadable/reloadable as a module object.
- `ldd1-ch02` has minor typos such as `.modeinfo` and `modeprobe`; the correct forms are `.modinfo` and `modprobe`.
- Notion examples use `S_IRUGO`, `S_IWUSR`, and related symbolic permission macros for `module_param()`. Modern kernel style increasingly prefers octal permissions such as `0444` and `0644`; this needs validation against the target kernel headers before writing examples.
- Notion part 2 presents a simplified `struct module { unsigned int refcnt; }`; real `struct module` internals are kernel-version-dependent and should not be taught as a stable driver-facing layout. Prefer explaining observable behavior via `lsmod`, `/proc/modules`, `/sys/module/<name>/refcnt`, and ownership/refcount concepts.
- `ldd1-ch02` frames `insmod` as preferred during development and `modprobe` as preferred in production. Notion matches this and adds the deployment flow: install under `/lib/modules/$(uname -r)/...`, run `depmod -a`, then use `modprobe`.
- `ldd1-ch02` introduces auto-loading through vendor/product IDs and hotplug agents. `ldd2-ch11` provides a concrete `MODULE_DEVICE_TABLE()` mechanism for PCI. Notion additionally mentions `MODULE_ALIAS()`. Learner docs should describe the concept generically and defer bus-specific ID tables to bus-driver chapters.
- `ldd1-ch05` has older-looking examples that include `.owner = THIS_MODULE` inside `struct platform_driver.driver`; modern subsystem examples may omit explicit owner assignment when wrapper/register macros handle ownership. Validate against the target kernel before copying code.
- `notion-ch05-part1` explains `module_platform_driver()` as recommended for simple platform drivers and expands it into generated `module_init()`/`module_exit()` functions. This should be framed as a wrapper pattern, not as a replacement for understanding module entry/exit.
- `notion-ch02-part4` covers in-tree Kconfig/Makefile integration. For this topic, keep it brief: `obj-m` builds loadable modules, `obj-y` builds into the kernel, and `tristate` allows `Y/M/N`. Deeper Kconfig work belongs to topic 02 or subsystem chapters.

## Gaps / Uncertainties
- Internal sources do not deeply explain module signing, Secure Boot lockdown, DKMS, or distro packaging. These should be marked as advanced/production deployment topics unless the learner-facing chapter needs a brief warning.
- Internal sources mention `vermagic` through `modinfo`, but do not fully explain ABI compatibility, `CONFIG_MODVERSIONS`, `Module.symvers`, or symbol CRC mismatch. The knowledge doc should include a short practical warning and defer details unless examples require it.
- Internal sources do not fully explain module reference ownership through file operations `.owner = THIS_MODULE`, subsystem registration helpers, or why forced unload is unsafe. Include the principle and defer subsystem-specific ownership details to later driver chapters.
- External validation is needed for current kernel parameter permission style, external module build syntax, default external module install directory, and any example Makefile intended to build on a modern kernel.
- The topic should avoid absorbing topic 04 logging/error handling, topic 05 core facilities, topic 07 character devices, topic 09 platform drivers, topic 31 PCI, and topic 37 debugging/tracing.

## External Validation
- Used official kernel documentation for external module build behavior:
  - `https://docs.kernel.org/kbuild/modules.html`
  - Validates `make -C <kernel_dir> M=$PWD`, `/lib/modules/$(uname -r)/build`, `modules_prepare`, `modules_install`, `INSTALL_MOD_PATH`, `INSTALL_MOD_DIR`, and notes that Linux 6.13 added an alternative `-f` Makefile form.
- Used official kernel documentation for module parameters:
  - `https://docs.kernel.org/admin-guide/kernel-parameters.html`
  - Validates that module parameters can be supplied through module-name-prefixed kernel command-line values or through `modprobe`, and that built-in module parameters must be supplied through the kernel command line.
- Needs target-kernel validation before example code:
  - Check whether symbolic permission macros such as `S_IRUGO` are available or whether examples should use octal modes (`0444`, `0644`).
  - Check `MODULE_LICENSE()` accepted strings in the target kernel's `include/linux/module.h`.
  - Check whether distro kernels compress modules (`.ko.xz`, `.ko.zst`) and how that affects `modinfo` examples.

## Learning Content Brief
- Mental model:
  - A kernel module is a loadable kernel object that extends the running kernel without rebooting. A driver is a hardware-control role; a module is a packaging/loading mechanism.
  - Built-in code is selected with `Y`, loadable code with `M`, disabled code with `N`; loadable support requires `CONFIG_MODULES`, and unloading requires `CONFIG_MODULE_UNLOAD`.
- Core mechanism:
  - `module_init()` registers the function called at load time; `module_exit()` registers cleanup called at unload time.
  - Init returns `0` on success or a negative errno on failure. Exit must undo what init registered or allocated.
  - `__init` and `__exit` mark special sections; explain carefully for built-in vs loadable modules.
  - `MODULE_*` macros write metadata into `.modinfo`; `modinfo` reads it.
- Commands and files:
  - Build/test loop: `make -C /lib/modules/$(uname -r)/build M=$PWD modules`, `sudo insmod ./name.ko`, `lsmod`, `dmesg`, `modinfo ./name.ko`, `sudo rmmod name`.
  - Production-ish loading: install into `/lib/modules/$(uname -r)/...`, run `depmod -a`, use `modprobe name`, configure persistent options with `/etc/modprobe.d`, and boot loading with `/etc/modules-load.d`.
  - Dependency files: `modules.dep`, `modules.dep.bin`, `modules.alias`, `modules.symbols`.
- APIs and macros:
  - Headers: `<linux/module.h>`, `<linux/init.h>`, `<linux/kernel.h>`, `<linux/moduleparam.h>`.
  - Entry/exit: `module_init()`, `module_exit()`, `__init`, `__exit`.
  - Metadata: `MODULE_LICENSE()`, `MODULE_AUTHOR()`, `MODULE_DESCRIPTION()`, `MODULE_VERSION()`, `MODULE_ALIAS()`, `MODULE_DEVICE_TABLE()`, `MODULE_INFO()`.
  - Dependencies: `EXPORT_SYMBOL()`, `EXPORT_SYMBOL_GPL()`.
  - Parameters: `module_param()`, `module_param_array()`, `MODULE_PARM_DESC()`.
  - Subsystem wrappers: mention `module_platform_driver()`, `module_i2c_driver()`, `module_spi_driver()`, `module_pci_driver()`, and similar helpers as examples of the same module entry/exit registration pattern, but teach each subsystem later.
- Lifecycle:
  - Compile `.c` -> `.o` -> `.ko`; load resolves symbols and calls init; active module serves kernel/subsystem requests; unload checks references and calls exit; cleanup releases resources.
  - `insmod` is direct and does not resolve dependencies; `modprobe` uses dependency databases and can apply configured options.
  - `rmmod` removes one module; `modprobe -r` can remove unused dependencies.
- Common bugs:
  - Missing `MODULE_LICENSE()` causes taint and GPL-only symbol access issues.
  - Loading with `insmod` before dependencies produces unknown symbol failures.
  - Building against the wrong kernel produces version/symbol mismatch failures.
  - Forgetting cleanup in exit causes stale registrations, leaks, or crashes on reload.
  - Treating parameter permissions casually can expose unsafe runtime knobs.
  - Forced unload can crash the system if code is still referenced.
- Debugging notes:
  - Use `dmesg` or `journalctl -k` after load/unload failures.
  - Use `modinfo` for metadata, dependencies, parameters, `vermagic`, and aliases.
  - Use `lsmod`, `/proc/modules`, `/sys/module/<name>/`, `/sys/module/<name>/parameters/`, and `/sys/module/<name>/holders/`.
  - For oops during load, `insmod` may appear as the current process; `objdump` can inspect module sections and symbol offsets.
- Production concerns:
  - Prefer `modprobe` after proper install and `depmod`.
  - Keep module metadata accurate and license compatible.
  - Treat module parameters as ABI-like knobs once users rely on them.
  - Build against the exact target kernel headers/config.
  - Be cautious with out-of-tree modules, signing, taint, and kernel-version compatibility.
- Interview angles:
  - Explain `insmod` vs `modprobe`, `rmmod` vs `modprobe -r`.
  - Explain how dependencies are discovered from exported/imported symbols.
  - Explain why a module cannot unload when its reference count is nonzero.
  - Explain what `MODULE_LICENSE("GPL")` changes.
  - Explain how module parameters appear in `modinfo` and `/sys/module`.
  - Explain `obj-m` vs `obj-y`, `Y/M/N`, and built-in vs loadable tradeoffs.
  - Explain why `probe()` is not the same as `module_init()` in real bus drivers.
  - Explain the first commands to run when a module fails to load.
