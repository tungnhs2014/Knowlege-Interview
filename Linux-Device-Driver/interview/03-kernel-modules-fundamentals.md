# 03 - Kernel Modules Fundamentals Interview Questions

Kernel modules are a core Embedded Linux interview topic because they reveal whether a candidate understands the difference between building code, loading code, registering with the kernel, handling dependencies, and cleaning up safely. Strong candidates connect commands such as `insmod`, `modprobe`, `lsmod`, and `rmmod` to the kernel mechanisms behind them.

Use these questions to test reasoning, not memorization. A good answer should explain lifecycle, metadata, dependency resolution, parameters, unload safety, and debugging evidence.

## Beginner

### 1. What Is A Kernel Module?

- **Level:** Beginner
- **Question:** What is a Linux kernel module, and how is it different from a driver?
- **Short Answer:** A kernel module is a loadable kernel object, usually a `.ko` file, that extends the running kernel. A driver is code that controls a device or subsystem role; it may be built into the kernel or packaged as a module.
- **Deep Explanation:** Module is a packaging and loading concept. Driver is a functional concept. Many device drivers are loadable modules because that makes development, optional hardware support, and field updates easier. But modules can also provide filesystems, crypto algorithms, tracing helpers, or network protocols. Likewise, a driver needed at boot may be built directly into the kernel image instead of loaded later.
- **API / Code Anchor:**
  ```c
  static int __init demo_init(void) { return 0; }
  static void __exit demo_exit(void) { }

  module_init(demo_init);
  module_exit(demo_exit);

  MODULE_LICENSE("GPL");
  MODULE_DESCRIPTION("Small loadable kernel module");
  ```
- **Production or Debugging Angle:** In bring-up, modules allow quick `make`, `insmod`, `rmmod`, and retry cycles. In production, whether a driver is built-in or modular depends on boot requirements, security policy, and update strategy.
- **Common Traps:**
  - Treating "module" and "driver" as synonyms.
  - Assuming all drivers are unloadable.
  - Forgetting modules run in kernel space and can crash the whole system.
  - Thinking module loading is just userspace dynamic linking.
- **Follow-up Questions:**
  - Give an example of a module that is not a device driver.
  - When would you build a driver into the kernel instead of as a module?
  - What file extension does a loadable kernel module usually have?

### 2. What Do `Y`, `M`, And `N` Mean?

- **Level:** Beginner
- **Question:** In kernel configuration, what do `Y`, `M`, and `N` mean?
- **Short Answer:** `Y` builds the feature into the kernel image, `M` builds it as a loadable module, and `N` disables it.
- **Deep Explanation:** Kconfig options often use a `tristate` value. If a feature is selected as `Y`, the object is linked into the kernel and initialized during boot. If selected as `M`, Kbuild emits a `.ko` module that can be loaded later. If selected as `N`, the code is not built. Loadable modules require the kernel to be built with module support.
- **API / Code Anchor:**
  ```makefile
  obj-y += built_in_feature.o
  obj-m += loadable_feature.o
  obj-$(CONFIG_MY_DRIVER) += my_driver.o
  ```
- **Production or Debugging Angle:** Boot-critical drivers, such as the root filesystem or early console path, often need `Y`. Optional hardware support is often `M`. Minimal or security-sensitive systems may disable module loading entirely.
- **Common Traps:**
  - Choosing `M` for a driver needed before the root filesystem can load modules.
  - Forgetting `CONFIG_MODULES` is required for loadable modules.
  - Assuming `M` means the module is automatically loaded in every deployment.
  - Editing Makefiles without understanding the matching Kconfig symbol.
- **Follow-up Questions:**
  - What kernel option enables loadable module support?
  - Why might an embedded product disable module loading?
  - How does `obj-$(CONFIG_FOO)` relate to Kconfig?

### 3. What Are `module_init()` And `module_exit()`?

- **Level:** Beginner
- **Question:** What are `module_init()` and `module_exit()` used for?
- **Short Answer:** `module_init()` registers the function called when the module loads. `module_exit()` registers the cleanup function called when the module unloads.
- **Deep Explanation:** The init function is the module's entry point. It should initialize state and register the module's functionality with the kernel. It returns `0` on success or a negative errno on failure. The exit function must undo what init successfully did: unregister callbacks, remove userspace-visible interfaces, stop timers/work, free memory, and release resources.
- **API / Code Anchor:**
  ```c
  static int __init my_init(void)
  {
          int ret;

          ret = register_my_feature();
          if (ret)
                  return ret;

          return 0;
  }

  static void __exit my_exit(void)
  {
          unregister_my_feature();
  }

  module_init(my_init);
  module_exit(my_exit);
  ```
- **Production or Debugging Angle:** A reliable driver can be loaded and unloaded repeatedly without leaking state or leaving registrations behind. Repeated load/unload testing is a simple but useful quality check.
- **Common Traps:**
  - Returning positive values or random values from init.
  - Forgetting to unwind partial initialization failures.
  - Freeing resources before unregistering callbacks that may still use them.
  - Assuming `module_exit()` exists for built-in drivers.
- **Follow-up Questions:**
  - What should init return when registration fails?
  - Why does cleanup order usually reverse initialization order?
  - What happens if init fails halfway through setup?

### 4. What Is The Basic Build/Load/Test Loop?

- **Level:** Beginner
- **Question:** What is the typical manual loop for building, loading, inspecting, and unloading a simple external module?
- **Short Answer:** Build with Kbuild, inspect with `modinfo`, load with `insmod`, check `lsmod` and `dmesg`, then unload with `rmmod`.
- **Deep Explanation:** External modules should be built against a prepared or built kernel tree, commonly through `/lib/modules/$(uname -r)/build`. During development, `insmod ./module.ko` is direct and fast. After loading, `lsmod`, `/proc/modules`, `/sys/module/<name>/`, and `dmesg` confirm runtime state. `rmmod name` requests unload by module name, not file path.
- **API / Code Anchor:**
  ```bash
  make -C /lib/modules/$(uname -r)/build M=$PWD modules
  modinfo ./demo_module.ko
  sudo insmod ./demo_module.ko debug=1
  lsmod | grep demo_module
  dmesg | tail
  sudo rmmod demo_module
  ```
- **Production or Debugging Angle:** This loop is for development. Production-style loading normally installs the module into `/lib/modules/$(uname -r)/...`, runs `depmod -a`, and uses `modprobe`.
- **Common Traps:**
  - Building against headers for a different running kernel.
  - Calling `rmmod ./demo_module.ko` instead of `rmmod demo_module`.
  - Forgetting to check `dmesg` after load failure.
  - Assuming `insmod` reads `/etc/modprobe.d` options.
- **Follow-up Questions:**
  - What does `M=$PWD` tell Kbuild?
  - Which command shows embedded module metadata?
  - Why is `dmesg` important after `insmod`?

### 5. What Does `MODULE_LICENSE()` Do?

- **Level:** Beginner
- **Question:** Is `MODULE_LICENSE("GPL")` only documentation?
- **Short Answer:** No. It records metadata, affects kernel tainting, and controls whether the module may use symbols exported with `EXPORT_SYMBOL_GPL()`.
- **Deep Explanation:** `MODULE_LICENSE()` writes license information into the module's `.modinfo` section. User tools can show it with `modinfo`. The kernel also uses license compatibility to decide whether GPL-only exported symbols are available. Missing or proprietary license metadata can taint the kernel, which matters for support and debugging.
- **API / Code Anchor:**
  ```c
  MODULE_LICENSE("GPL");

  /* Provider module */
  EXPORT_SYMBOL(public_helper);
  EXPORT_SYMBOL_GPL(internal_gpl_helper);
  ```
- **Production or Debugging Angle:** If a module fails to load because it uses a GPL-only symbol, check `modinfo -F license ./module.ko` and `dmesg`. Taint messages are also visible in the kernel log.
- **Common Traps:**
  - Omitting `MODULE_LICENSE()` in a learning module.
  - Assuming proprietary modules can use every exported kernel symbol.
  - Ignoring taint when reporting kernel bugs.
  - Treating license strings as arbitrary free text.
- **Follow-up Questions:**
  - What is the difference between `EXPORT_SYMBOL()` and `EXPORT_SYMBOL_GPL()`?
  - How can you inspect a module's license before loading it?
  - What does a tainted kernel imply for debugging support?

## Mid-Level

### 6. Compare `insmod` And `modprobe`

- **Level:** Mid
- **Question:** What is the difference between `insmod` and `modprobe`?
- **Short Answer:** `insmod` directly loads a specific `.ko` file and does not resolve dependencies. `modprobe` loads by module name using module databases, dependencies, aliases, and configuration options.
- **Deep Explanation:** `insmod ./foo.ko` asks the kernel to load exactly that file. If `foo` needs symbols from another module that is not loaded, it fails. `modprobe foo` searches the installed module tree under `/lib/modules/$(uname -r)/`, reads dependency data generated by `depmod`, applies options from `/etc/modprobe.d`, and loads dependencies first.
- **API / Code Anchor:**
  ```bash
  sudo insmod ./foo.ko

  sudo cp foo.ko /lib/modules/$(uname -r)/extra/
  sudo depmod -a
  sudo modprobe foo
  ```
- **Production or Debugging Angle:** Use `insmod` for quick local development. Use `modprobe` for installed modules, dependency handling, aliases, and persistent options.
- **Common Traps:**
  - Expecting `insmod` to load dependencies.
  - Expecting `modprobe foo` to find `./foo.ko` in the current directory.
  - Forgetting `depmod -a` after manual installation.
  - Passing a `.ko` path to `modprobe`.
- **Follow-up Questions:**
  - What files does `depmod` generate?
  - Why might `insmod` work but `modprobe` say "module not found"?
  - How do `/etc/modprobe.d` options reach a module?

### 7. Compare `rmmod` And `modprobe -r`

- **Level:** Mid
- **Question:** What is the difference between `rmmod foo` and `modprobe -r foo`?
- **Short Answer:** `rmmod` removes one loaded module by name. `modprobe -r` removes the target module and may also remove unused dependency modules.
- **Deep Explanation:** `rmmod` is a lower-level tool: it asks the kernel to remove a named module if it is not in use. It does not walk a dependency tree for cleanup. `modprobe -r` understands module dependency information and can remove dependencies that are no longer needed by any other module.
- **API / Code Anchor:**
  ```bash
  sudo rmmod usb_storage
  sudo modprobe -r usb-storage
  lsmod | grep scsi_mod
  ```
- **Production or Debugging Angle:** If unload fails, inspect `lsmod`, `/proc/modules`, and `/sys/module/<name>/holders/`. The kernel refuses unload when references remain.
- **Common Traps:**
  - Using a filename instead of module name.
  - Assuming `rmmod` cleans up dependencies.
  - Treating `rmmod -f` as normal debugging.
  - Ignoring dependent modules in `/sys/module/<name>/holders/`.
- **Follow-up Questions:**
  - What does the `Used by` column in `lsmod` suggest?
  - Why can an unused-looking module still refuse unload?
  - Why is forced unload dangerous?

### 8. How Are Module Dependencies Discovered?

- **Level:** Mid
- **Question:** How does Linux know that one module depends on another?
- **Short Answer:** Dependencies come from imported and exported symbols. `depmod` scans installed modules, records what they export and require, and writes dependency databases used by `modprobe`.
- **Deep Explanation:** A provider module exports functions or variables using `EXPORT_SYMBOL()` or `EXPORT_SYMBOL_GPL()`. A consumer module references those symbols. During module post-processing and dependency generation, symbol relationships are recorded. At load time, unresolved symbols must already exist in the kernel or in loaded provider modules. `modprobe` can load providers first because `depmod` indexed the installed module tree.
- **API / Code Anchor:**
  ```c
  int sensor_core_read(int reg);
  EXPORT_SYMBOL_GPL(sensor_core_read);
  ```

  ```bash
  sudo depmod -a
  modinfo -F depends ./consumer.ko
  cat /lib/modules/$(uname -r)/modules.dep | grep consumer
  ```
- **Production or Debugging Angle:** Unknown-symbol failures are often solved by checking `dmesg`, verifying the provider module is installed and loaded, running `depmod -a`, and checking license compatibility for GPL-only symbols.
- **Common Traps:**
  - Declaring `extern` and assuming that makes a symbol available.
  - Forgetting to export a provider symbol.
  - Using `EXPORT_SYMBOL_GPL()` from a non-GPL-compatible module.
  - Copying a module into the module tree but not running `depmod`.
- **Follow-up Questions:**
  - What does `EXPORT_SYMBOL_GPL()` add beyond `EXPORT_SYMBOL()`?
  - Why does `insmod` fail even if `modprobe` would succeed?
  - Where would you look for unknown-symbol details?

### 9. How Do Module Parameters Work?

- **Level:** Mid
- **Question:** How do module parameters get declared, passed, inspected, and changed?
- **Short Answer:** Declare them with `module_param()` or `module_param_array()`, describe them with `MODULE_PARM_DESC()`, pass values at load time, and inspect them through `modinfo` or `/sys/module/<name>/parameters/` when permissions allow.
- **Deep Explanation:** Module parameters are variables registered with the module parameter infrastructure. Values passed by `insmod` or `modprobe` are parsed before the module init function runs. If the parameter has nonzero permissions, the kernel exposes it through sysfs under `/sys/module/<name>/parameters/`. Writable permissions allow runtime changes, but only if the driver is designed to handle such changes safely.
- **API / Code Anchor:**
  ```c
  static int debug;
  static char *name = "demo0";
  static int gpios[4];
  static int gpio_count;

  module_param(debug, int, 0644);
  MODULE_PARM_DESC(debug, "Debug level");

  module_param(name, charp, 0444);
  MODULE_PARM_DESC(name, "Logical name");

  module_param_array(gpios, int, &gpio_count, 0444);
  MODULE_PARM_DESC(gpios, "GPIO numbers");
  ```

  ```bash
  sudo insmod demo.ko debug=1 name=mydev gpios=10,11
  modinfo -p ./demo.ko
  cat /sys/module/demo/parameters/debug
  ```
- **Production or Debugging Angle:** Parameters become operational knobs. Changing a buffer size, IRQ number, or debug flag at runtime can be unsafe unless locking, validation, and reconfiguration rules are explicit.
- **Common Traps:**
  - Using writable sysfs permissions casually.
  - Expecting a permission of `0` to create a sysfs parameter file.
  - Forgetting `MODULE_PARM_DESC()`, making `modinfo` less useful.
  - Copying older `S_IRUGO` examples into a newer kernel without checking headers.
- **Follow-up Questions:**
  - How do built-in driver parameters get passed?
  - What does the `perm` argument control?
  - Why might a parameter appear in `modinfo` but not be writable in sysfs?

### 10. What Does `modinfo` Show?

- **Level:** Mid
- **Question:** What information can `modinfo` show, and where does it come from?
- **Short Answer:** `modinfo` shows metadata from the module's `.modinfo` section, such as license, author, description, aliases, dependencies, parameters, name, and `vermagic`.
- **Deep Explanation:** The module build process records metadata strings from `MODULE_*` macros and generated build information. `modinfo` can inspect a module file before it is loaded, which makes it useful for checking expected parameters, kernel version compatibility, dependencies, aliases, and license.
- **API / Code Anchor:**
  ```bash
  modinfo ./demo_module.ko
  modinfo -F license ./demo_module.ko
  modinfo -F depends ./demo_module.ko
  modinfo -F vermagic ./demo_module.ko
  modinfo -p ./demo_module.ko
  ```
- **Production or Debugging Angle:** For load failures, compare `modinfo -F vermagic ./foo.ko` with `uname -r` and target kernel configuration. Also inspect `depends` and `parm` before trying more invasive debugging.
- **Common Traps:**
  - Assuming `modinfo` requires the module to be loaded.
  - Ignoring `vermagic` in invalid-format failures.
  - Confusing module filename with module name.
  - Forgetting distro modules may be compressed as `.ko.xz` or `.ko.zst`.
- **Follow-up Questions:**
  - Which `MODULE_*` macro affects the `license` field?
  - Why is `vermagic` useful?
  - What command shows only module parameters?

## Senior

### 11. Debug `Unknown symbol` During Load

- **Level:** Senior
- **Question:** `insmod ./foo.ko` fails with `Unknown symbol bar_helper`. Walk through your debugging process.
- **Short Answer:** Check `dmesg` for the exact symbol error, identify which module should export it, verify that provider is built/installed/loaded, check `modinfo -F depends`, license compatibility, and kernel version/config match. Prefer installed `modprobe` flow when dependencies are expected.
- **Deep Explanation:** Unknown symbol means the loader could not resolve a referenced kernel symbol. Causes include a missing provider module, provider not exporting the symbol, stale `depmod` database, loading with `insmod` instead of `modprobe`, wrong kernel headers/config, or using a GPL-only symbol from a non-GPL-compatible module.
- **API / Code Anchor:**
  ```bash
  dmesg | tail -50
  modinfo -F depends ./foo.ko
  modinfo -F license ./foo.ko
  modinfo -F vermagic ./foo.ko
  grep bar_helper /proc/kallsyms
  lsmod | grep provider
  ```
- **Production or Debugging Angle:** On a product image, verify the module is installed under the correct `/lib/modules/<kernel_release>/` tree and run `depmod -a`. On cross-compiled targets, verify `ARCH`, `CROSS_COMPILE`, and the target kernel build directory.
- **Common Traps:**
  - Trying random load orders without checking `dmesg`.
  - Forgetting that `insmod` does not load dependencies.
  - Missing license-related GPL-only symbol failures.
  - Rebuilding against host headers instead of target headers.
- **Follow-up Questions:**
  - How would `modprobe` behave differently here?
  - What if the provider symbol exists in source but not in `/proc/kallsyms`?
  - What role does `Module.symvers` play for external modules?

### 12. Why Is Forced Unload Dangerous?

- **Level:** Senior
- **Question:** Why is `rmmod -f` dangerous, and what should you investigate before considering it?
- **Short Answer:** Forced unload can remove module code while something still references it, leading to dangling function pointers, use-after-free, oops, or data corruption. Investigate the references instead.
- **Deep Explanation:** A loaded module may have open files, active subsystem callbacks, exported symbols used by other modules, timers, workqueues, IRQ handlers, sysfs callbacks, or device instances. The normal reference count and holder checks exist to prevent code removal while execution paths can still enter the module. Forced unload bypasses safety.
- **API / Code Anchor:**
  ```bash
  lsmod | grep foo
  cat /proc/modules | grep foo
  cat /sys/module/foo/refcnt
  ls /sys/module/foo/holders
  lsof /dev/foo0
  ```
- **Production or Debugging Angle:** A busy module is a lifetime bug or an expected active use. Fix the user, dependency, or unregister path. In driver code, cancel timers/work, free IRQs, unregister devices, and prevent new entry before freeing state.
- **Common Traps:**
  - Treating forced unload as a normal development shortcut.
  - Ignoring open device nodes or bound devices.
  - Forgetting delayed work, timers, or threaded IRQs can run after unload starts.
  - Freeing state before unregistering entry points.
- **Follow-up Questions:**
  - What can increase a module reference count?
  - How do `/sys/module/<name>/holders/` and `lsmod` differ?
  - Which resources must be stopped before module code is removed?

### 13. Explain Automatic Module Loading

- **Level:** Senior
- **Question:** How does automatic module loading work when matching hardware appears?
- **Short Answer:** The kernel reports device identity through a modalias/uevent, userspace receives it, `modprobe` searches `modules.alias`, then loads the matching module and its dependencies.
- **Deep Explanation:** Bus drivers know how to describe device identity: PCI IDs, USB IDs, OF compatible strings, platform ID tables, and similar match data. Driver code exposes match tables with `MODULE_DEVICE_TABLE()` or aliases with `MODULE_ALIAS()`. During installation, `depmod` scans module metadata and builds `modules.alias`. When a device appears, udev or another hotplug manager asks `modprobe` to load a matching module.
- **API / Code Anchor:**
  ```c
  static const struct platform_device_id demo_ids[] = {
          { .name = "demo-uart" },
          { }
  };
  MODULE_DEVICE_TABLE(platform, demo_ids);
  ```

  ```bash
  grep demo-uart /lib/modules/$(uname -r)/modules.alias
  udevadm monitor --kernel --property
  modinfo -F alias ./demo_driver.ko
  ```
- **Production or Debugging Angle:** If a driver loads manually but not automatically, inspect the modalias, `MODULE_DEVICE_TABLE()`, compatible string or ID table, `depmod` output, and whether the module is installed in the target module tree.
- **Common Traps:**
  - Assuming a module name alone is enough for hardware autoload.
  - Forgetting `depmod` after installing a new module.
  - Using the wrong bus ID table or missing `MODULE_DEVICE_TABLE()`.
  - Mixing up module load with device binding.
- **Follow-up Questions:**
  - What file maps aliases to module names?
  - How can you inspect a module's aliases?
  - Why can a module load without its `probe()` running?

### 14. Why Is `probe()` Not `module_init()`?

- **Level:** Senior
- **Question:** In a real bus driver, why is `probe()` not the same thing as `module_init()`?
- **Short Answer:** `module_init()` runs once when the module loads and usually registers the driver with a subsystem. `probe()` runs later, once per matching device, when the subsystem binds a device to that driver.
- **Deep Explanation:** The module lifecycle and device lifecycle are different. A platform, I2C, SPI, PCI, or USB module may load successfully even when no matching device exists. Its init path registers a driver object. The bus core performs matching. For each matched device, the bus calls `probe()` to allocate per-device state, map resources, request IRQs, and register framework interfaces.
- **API / Code Anchor:**
  ```c
  static struct platform_driver demo_driver = {
          .probe = demo_probe,
          .remove = demo_remove,
          .driver = {
                  .name = "demo",
          },
  };

  module_platform_driver(demo_driver);
  ```
- **Production or Debugging Angle:** If `insmod` succeeds but hardware is not initialized, do not assume init failed. Check whether device matching happened. Inspect device tree, ACPI tables, platform device names, ID tables, bus sysfs, and `dmesg`.
- **Common Traps:**
  - Initializing hardware directly in module init when the driver should support multiple devices.
  - Assuming one module means one device.
  - Forgetting `probe()` may run multiple times.
  - Forgetting `remove()` is per device, while `module_exit()` is per module.
- **Follow-up Questions:**
  - Why do wrappers like `module_platform_driver()` exist?
  - What happens if three devices match one driver?
  - How can a module be loaded but no device bound?

### 15. Debug `Invalid module format`

- **Level:** Senior
- **Question:** `insmod ./foo.ko` fails with `Invalid module format`. What are likely causes and checks?
- **Short Answer:** The module was probably built for a different kernel version, configuration, architecture, or symbol-version setup. Check `dmesg`, `modinfo -F vermagic`, `uname -r`, and the build tree used.
- **Deep Explanation:** Kernel modules are tightly coupled to the kernel they are built against. The loader checks version/config information such as `vermagic`; with symbol versioning, symbol CRCs can also matter. Cross-compiled modules must match target architecture and target kernel, not the build host.
- **API / Code Anchor:**
  ```bash
  dmesg | tail -50
  modinfo -F vermagic ./foo.ko
  uname -r
  file ./foo.ko
  make -C /path/to/target/kernel/build M=$PWD modules
  ```
- **Production or Debugging Angle:** On embedded targets, this often means the developer built against host `/lib/modules/$(uname -r)/build` instead of the target kernel build directory. In CI, record kernel release, config, compiler, and module artifact provenance.
- **Common Traps:**
  - Comparing only version strings while ignoring config and architecture.
  - Using host headers for target modules.
  - Forgetting `CONFIG_MODVERSIONS` and `Module.symvers` implications.
  - Loading an ARM module on x86 or vice versa.
- **Follow-up Questions:**
  - What does `vermagic` include?
  - How would you build an external module for an ARM target?
  - Why might two kernels with the same release string still reject a module?

### 16. What Makes A Good Production Module?

- **Level:** Senior
- **Question:** What production concerns matter for out-of-tree or product-shipped modules?
- **Short Answer:** Build against the exact target kernel, provide correct metadata, handle dependencies through `depmod`/`modprobe`, respect signing and security policy, avoid unsafe parameters, and guarantee clean lifecycle/unload behavior.
- **Deep Explanation:** Production modules are operational artifacts, not just code. The build must match the target kernel ABI and configuration. The install path and dependency databases must be correct. Metadata and aliases must support diagnostics and autoload. Security policy may require signed modules. Runtime parameters and unload behavior become part of supportability.
- **API / Code Anchor:**
  ```bash
  modinfo ./foo.ko
  modinfo -F vermagic ./foo.ko
  modinfo -F depends ./foo.ko
  sudo depmod -a
  sudo modprobe foo
  cat /proc/sys/kernel/tainted
  ```
- **Production or Debugging Angle:** A support bundle for module issues should include `uname -a`, kernel config or release metadata, `modinfo`, `lsmod`, `dmesg`, `/sys/module/<name>/`, install path, and signing/lockdown state.
- **Common Traps:**
  - Shipping modules without a reproducible target-kernel build.
  - Treating module parameters as private after users depend on them.
  - Ignoring taint and signing warnings.
  - Not testing repeated load/unload and failure paths.
- **Follow-up Questions:**
  - What information would you ask a field engineer to collect?
  - How does Secure Boot affect module loading?
  - Why is DKMS useful on some distributions but not a universal embedded solution?

## Debugging Scenarios

### Scenario A: `modprobe foo` Cannot Find A Module That You Just Built

- **Level:** Mid
- **Question:** `foo.ko` exists in your source directory, but `sudo modprobe foo` says the module was not found. Why?
- **Short Answer:** `modprobe` does not search the current directory. The module must be installed under `/lib/modules/$(uname -r)/...` and indexed with `depmod`.
- **Deep Explanation:** `modprobe` loads by module name using the installed module tree. A local build artifact is visible to `insmod ./foo.ko`, but not to `modprobe foo` until installed and indexed.
- **API / Code Anchor:**
  ```bash
  sudo cp foo.ko /lib/modules/$(uname -r)/extra/
  sudo depmod -a
  sudo modprobe foo
  ```
- **Production or Debugging Angle:** On target root filesystems, use `INSTALL_MOD_PATH` during `modules_install` so modules land in the image, not on the host.
- **Common Traps:**
  - Running `modprobe ./foo.ko`.
  - Forgetting `depmod -a`.
  - Installing into the host module tree while building for a target rootfs.
- **Follow-up Questions:**
  - What is the difference between module path and module name?
  - Which command should you use for a local uninstalled `.ko`?
  - How do you install modules into a target rootfs?

### Scenario B: Parameter Does Not Appear In Sysfs

- **Level:** Mid
- **Question:** `modinfo -p foo.ko` shows a parameter, but `/sys/module/foo/parameters/` does not contain it after load. What should you check?
- **Short Answer:** Check whether the module is loaded, the runtime module name, and the parameter permission. A permission of `0` can keep the sysfs file from being created.
- **Deep Explanation:** `MODULE_PARM_DESC()` and parameter metadata can appear in `.modinfo`, but sysfs exposure depends on the `perm` argument to `module_param()`. The parameter may also appear under a module name that differs from the filename formatting.
- **API / Code Anchor:**
  ```c
  module_param(debug, int, 0);     /* no sysfs parameter file */
  module_param(level, int, 0644);  /* sysfs-visible */
  ```

  ```bash
  lsmod | grep foo
  modinfo -p ./foo.ko
  ls /sys/module/foo/parameters
  ```
- **Production or Debugging Angle:** Writable runtime parameters need locking and validation. Read-only load-time configuration is often safer for hardware settings that should not change while active.
- **Common Traps:**
  - Assuming `MODULE_PARM_DESC()` creates a sysfs file.
  - Expecting read-only parameters to accept writes.
  - Forgetting module names may use underscores instead of dashes in some contexts.
- **Follow-up Questions:**
  - What does the third argument to `module_param()` mean?
  - How do you pass a parameter through `modprobe`?
  - How do built-in driver parameters differ?

### Scenario C: System Oops During `insmod`

- **Level:** Senior
- **Question:** The kernel oops shows the current process as `insmod`. Does that mean `insmod` is buggy?
- **Short Answer:** Usually no. It means the module init path was executing in the context of the loading process when the kernel bug occurred.
- **Deep Explanation:** The userspace `insmod` process asks the kernel to load the module. The kernel validates, links, applies parameters, and calls the module init function. If init dereferences a bad pointer or registers something incorrectly, the oops may show `insmod` as the current process because it triggered the load.
- **API / Code Anchor:**
  ```bash
  dmesg | tail -100
  modinfo ./foo.ko
  objdump -fS foo.ko > foo.disasm
  ```
- **Production or Debugging Angle:** Add logs around each init step, inspect the oops symbol and offset, and verify initialization order. Do not keep retrying on production hardware without understanding the failing path.
- **Common Traps:**
  - Blaming the userspace loader instead of module init code.
  - Ignoring the first oops and continuing tests on corrupted state.
  - Forgetting registration can trigger callbacks earlier than expected.
- **Follow-up Questions:**
  - Why can registration calls make a module callable immediately?
  - How do symbol offsets help locate a crashing instruction?
  - What init steps should be logged during bring-up?
