# 03 - Kernel Modules Fundamentals

## Learning Goal

After this chapter, you should be able to build, load, inspect, parameterize, unload, and debug a simple Linux kernel module, and explain how module entry/exit, metadata, dependencies, reference counts, and `modprobe` fit together.

By the end, you should be able to:

- Explain what a `.ko` file is and how it differs from a built-in driver.
- Write a small module with `module_init()`, `module_exit()`, `MODULE_LICENSE()`, parameters, and useful logs.
- Choose between `insmod`, `modprobe`, `rmmod`, and `modprobe -r`.
- Explain how exported symbols create module dependencies.
- Use `modinfo`, `lsmod`, `/proc/modules`, `/sys/module`, and `dmesg` to debug load/unload problems.
- Explain why `probe()` is not the same as `module_init()` in real bus drivers.
- Recognize version, license, taint, signing, and forced-unload traps.

## Why This Matters In Real Work

Kernel modules are the normal development unit for Linux drivers. They let you test a driver without rebuilding or rebooting the whole kernel, but they also run with full kernel privilege, so every load/unload cycle is a lifetime and cleanup test.

Common real-world uses:

- Bring up a new board driver while iterating quickly.
- Load optional drivers only when hardware is present.
- Ship hardware support outside the base kernel image.
- Test parameters such as debug level, IRQ number, buffer size, or feature enablement.
- Inspect kernel behavior with a small learning or diagnostic module.

The hard parts are not the syntax. The hard parts are:

- **Lifetime:** init must publish only fully initialized state, and exit must undo everything that succeeded.
- **Dependency ordering:** one module may need symbols from another module.
- **Kernel compatibility:** a module is built for a specific kernel configuration and ABI.
- **Unload safety:** the kernel must not remove code that is still referenced.
- **Deployment:** development loading with `insmod` is different from installed loading with `modprobe`.

## Mental Model

A kernel module is a plugin for the running kernel. Loading it links new code into kernel space, calls its init function, and leaves the code resident until it is unloaded or the system reboots.

Think of a module as three things at once:

| View | Meaning |
| --- | --- |
| File on disk | A `.ko` kernel object built by Kbuild. |
| Runtime object | Loaded code, data, parameters, metadata, and references tracked by the kernel. |
| Driver package | Often, but not always, code that registers with a subsystem such as platform, I2C, SPI, PCI, USB, char devices, networking, or input. |

The basic lifecycle:

```text
source.c
  -> kbuild compiles and links
module.ko
  -> insmod/modprobe loads and resolves symbols
module_init() function runs once
  -> module registers something useful
module active in kernel
  -> callbacks, subsystem operations, or exported APIs run
rmmod/modprobe -r requests removal
  -> kernel checks references
module_exit() function runs once
  -> module unregisters and frees resources
module code removed
```

**Important distinction:** a module is a loading mechanism; a driver is a role. Many drivers are modules, but not every module is a driver. Filesystems, crypto algorithms, network protocols, and tracing helpers can also be modules.

## Core Concepts

Kernel modules exist to keep the base kernel smaller and more flexible. A feature can be built into the kernel image, built as a loadable module, or disabled.

| Choice | Kconfig symbol value | Build result | Load time | Unload? | Common use |
| --- | --- | --- | --- | --- | --- |
| Built-in | `y` | Linked into `vmlinux` / kernel image | Boot | No | Root filesystem, early console, mandatory platform support. |
| Module | `m` | Separate `.ko` file | On demand | Usually yes | Optional drivers, development, pluggable hardware. |
| Disabled | `n` | Not built | Never | No | Unused feature. |

Key terms:

| Term | Meaning |
| --- | --- |
| `.ko` | Kernel object file produced by Kbuild. |
| `CONFIG_MODULES` | Kernel option that enables loadable module support. |
| `CONFIG_MODULE_UNLOAD` | Kernel option that allows module unloading. |
| `vermagic` | Version/config string embedded in a module and checked during loading. |
| `.modinfo` | ELF section that stores metadata from `MODULE_*` macros. |
| Exported symbol | A function or variable made visible to other modules with `EXPORT_SYMBOL()` or `EXPORT_SYMBOL_GPL()`. |
| Dependency | A module relationship created when one module needs symbols from another. |
| Reference count | Runtime usage count that prevents unsafe unload. |
| Tainted kernel | Kernel state marked as affected by something such as proprietary or unsigned/out-of-tree module loading. |

Useful comparisons:

| Development question | Usually use | Why |
| --- | --- | --- |
| Testing a just-built local `.ko` | `insmod ./driver.ko` | Direct, no install step, easy loop. |
| Loading an installed module with dependencies | `modprobe driver` | Uses dependency and alias databases. |
| Removing one direct test module | `rmmod driver` | Simple removal by module name. |
| Removing an installed module and unused dependencies | `modprobe -r driver` | Handles dependency cleanup. |

## Kernel Mechanism

The kernel module loader validates the object, resolves symbols, maps code/data into kernel memory, creates module metadata state, applies parameters, then calls the registered init function.

### Entry And Exit

Every ordinary loadable module has a load-time entry point:

- `module_init(init_fn)` tells the kernel what to call when the module loads.
- `init_fn` returns:
  - `0` for success;
  - negative errno for failure.
- If init fails, the module load fails and the module should not remain active.

Unload uses:

- `module_exit(exit_fn)` tells the kernel what to call when the module unloads.
- `exit_fn` returns `void`.
- Exit must reverse the init path and unregister anything that could call back into the module.

Common pattern:

```text
init:
  allocate/initialize private state
  register with subsystem
  publish user-visible interface
  return 0

exit:
  unpublish user-visible interface
  unregister from subsystem
  free private state
```

### Sections: `__init` And `__exit`

`__init` and `__exit` are section annotations. They put functions into special ELF/kernel sections.

Practical rules:

- Use `static int __init my_init(void)` for load/boot initialization code.
- Use `static void __exit my_exit(void)` for unload-only cleanup code.
- For built-in code, init memory can be discarded after boot initialization.
- For built-in code, `__exit` code may be omitted because built-in drivers cannot unload.
- For loadable modules, keep the mental model simple: these annotations document lifecycle sections, but the module object must remain valid while loaded.

### Metadata: `MODULE_*`

`MODULE_*` macros place strings into `.modinfo`. Tools such as `modinfo` read these strings before or after loading.

Important macros:

| Macro | Purpose |
| --- | --- |
| `MODULE_LICENSE("GPL")` | Declares license compatibility; affects taint and GPL-only symbol access. |
| `MODULE_AUTHOR("Name <email>")` | Documents author. |
| `MODULE_DESCRIPTION("...")` | Short module purpose. |
| `MODULE_VERSION("1.0")` | Optional version string. |
| `MODULE_PARM_DESC(name, "...")` | Parameter description shown by `modinfo`. |
| `MODULE_ALIAS("...")` | Adds an alias used by module loading tools. |
| `MODULE_DEVICE_TABLE(bus, table)` | Exports device ID tables so hotplug can autoload drivers. |

**Production rule:** always provide a correct `MODULE_LICENSE()`. Without a GPL-compatible license, the module cannot use `EXPORT_SYMBOL_GPL()` symbols and may taint the kernel.

### Dependencies And Symbols

One module can make functions or variables available to another module:

```c
int helper_do_work(int value)
{
        return value + 1;
}
EXPORT_SYMBOL(helper_do_work);
```

If another module calls `helper_do_work()`, it depends on the provider module.

Dependency flow:

```text
provider.ko exports helper_do_work
consumer.ko imports helper_do_work
depmod scans installed modules
  -> writes modules.dep / modules.dep.bin / modules.symbols
modprobe consumer
  -> loads provider first
  -> loads consumer
```

`EXPORT_SYMBOL_GPL()` is stricter:

- GPL-compatible modules can use the symbol.
- Proprietary modules cannot.
- This is why license metadata is not just documentation.

### Auto-Loading

Auto-loading lets the kernel and userspace module tools load a driver when matching hardware appears.

Typical flow:

```text
device appears
  -> kernel emits uevent with bus/device identity
udev/mdev receives event
  -> asks modprobe for matching module
modprobe checks modules.alias
  -> loads module and dependencies
driver registers with bus
  -> bus match calls probe()
```

Bus-specific chapters teach the details, but the important module-level idea is:

- `MODULE_DEVICE_TABLE()` and aliases feed `modules.alias`.
- `depmod` creates the alias database from installed modules.
- `modprobe` uses that database.

### `module_init()` Is Not `probe()`

In many real drivers, module init does not initialize hardware directly. It registers a driver with a subsystem; the subsystem later calls `probe()` for each matching device.

Manual registration pattern:

```c
static int __init demo_init(void)
{
        return platform_driver_register(&demo_driver);
}

static void __exit demo_exit(void)
{
        platform_driver_unregister(&demo_driver);
}

module_init(demo_init);
module_exit(demo_exit);
```

Wrapper pattern:

```c
module_platform_driver(demo_driver);
```

The wrapper expands to the same idea: register on load, unregister on unload.

Comparison:

| Function | Runs when | Runs how many times | Job |
| --- | --- | --- | --- |
| `module_init()` function | Module load or built-in init | Once per module | Register the module's top-level feature. |
| `probe()` | Device matches driver | Once per matched device | Initialize one device instance. |
| `remove()` | Device removed or driver unbound/unloaded | Once per device instance | Tear down one device instance. |
| `module_exit()` function | Module unload | Once per module | Unregister the top-level feature. |

**Interview trap:** if one module handles three matching devices, `module_init()` runs once, but `probe()` can run three times.

## Key Structs And APIs

Do not memorize these as isolated names. Each one exists at a specific layer: build, load, metadata, parameter, dependency, or subsystem registration.

### Required Headers

| Header | Typical reason |
| --- | --- |
| `<linux/module.h>` | `module_init()`, `module_exit()`, `MODULE_*`, exports. |
| `<linux/init.h>` | `__init`, `__exit`. |
| `<linux/kernel.h>` | Kernel helpers and logging in many examples. |
| `<linux/moduleparam.h>` | `module_param()`, `module_param_array()`. |

### Entry, Exit, And Metadata

| API / macro | Use |
| --- | --- |
| `module_init(fn)` | Register load-time init function. |
| `module_exit(fn)` | Register unload-time cleanup function. |
| `__init` | Mark init-only code section. |
| `__exit` | Mark unload-only code section. |
| `MODULE_LICENSE()` | License metadata and GPL-only symbol access behavior. |
| `MODULE_AUTHOR()` | Author metadata. |
| `MODULE_DESCRIPTION()` | Purpose metadata. |
| `MODULE_ALIAS()` | Alias metadata for module loading. |
| `MODULE_DEVICE_TABLE()` | Export bus ID table for autoload. |

### Parameters

Module parameters let you change module behavior at load time and sometimes at runtime through sysfs.

| API | Use |
| --- | --- |
| `module_param(name, type, perm)` | Register one scalar/string parameter. |
| `module_param_array(name, type, nump, perm)` | Register an array parameter. |
| `MODULE_PARM_DESC(name, "...")` | Show parameter help in `modinfo`. |

Supported common parameter types:

- `bool`, `invbool`
- `byte`, `short`, `ushort`
- `int`, `uint`
- `long`, `ulong`
- `charp`

Permission examples:

| Permission | Meaning |
| --- | --- |
| `0` | No `/sys/module/<name>/parameters/<param>` file. |
| `0444` | Read-only for everyone. |
| `0644` | Readable by everyone, writable by owner/root. |
| `0600` | Read/write for owner/root only. |

Prefer octal permissions in new examples. Older code often shows symbolic forms such as `S_IRUGO`; check your target kernel and coding style before copying older snippets.

Parameter sources:

```bash
# Direct load
sudo insmod demo_module.ko debug=1 name=mydev gpios=10,11,12

# Installed module
sudo modprobe demo_module debug=1 name=mydev

# Persistent modprobe option
echo 'options demo_module debug=1 name=mydev' | sudo tee /etc/modprobe.d/demo_module.conf

# Runtime view/change if permissions allow
cat /sys/module/demo_module/parameters/debug
echo 2 | sudo tee /sys/module/demo_module/parameters/debug
```

Built-in module parameters are different: they must be supplied on the kernel command line, usually as `module_or_driver_name.parameter=value`.

### Build And Install

Minimal external module Makefile:

```makefile
obj-m := demo_module.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
```

Common build commands:

```bash
make
make -C /lib/modules/$(uname -r)/build M=$PWD modules
make -C /path/to/kernel/build M=$PWD modules
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-
```

Install flow:

```bash
sudo make -C /lib/modules/$(uname -r)/build M=$PWD modules_install
sudo depmod -a
sudo modprobe demo_module
```

Target root filesystem install:

```bash
make -C /path/to/kernel/build M=$PWD \
     INSTALL_MOD_PATH=/path/to/rootfs modules_install
```

## Lifecycle / Data Flow

A module has two related lifecycles: the file/build lifecycle and the runtime lifecycle. Many bugs come from confusing them.

### Build-Time Flow

```text
demo_module.c
  -> Kbuild compiles demo_module.o
  -> modpost checks symbols and metadata
  -> linker emits demo_module.ko
  -> .modinfo contains license, alias, parameters, vermagic, depends
```

Generated files often include:

- `demo_module.o`
- `demo_module.mod.c`
- `demo_module.mod.o`
- `demo_module.ko`
- `Module.symvers`
- `modules.order`

### Development Load Flow With `insmod`

```text
sudo insmod ./demo_module.ko debug=1
  -> kernel validates module format and version
  -> loader resolves symbols already available in the kernel
  -> parameters are parsed
  -> init function runs
  -> module appears in lsmod and /sys/module
```

`insmod` does not load dependencies for you. If a needed symbol is missing, load fails and `dmesg` usually reports an unknown symbol.

### Installed Load Flow With `modprobe`

```text
sudo cp demo_module.ko /lib/modules/$(uname -r)/extra/
sudo depmod -a
sudo modprobe demo_module
  -> modprobe reads modules.dep/modules.alias/options
  -> dependencies load first
  -> target module loads
```

`modprobe` needs the module installed in the module tree and indexed by `depmod`.

### Unload Flow

```text
sudo rmmod demo_module
  -> kernel checks module reference count
  -> if safe, exit function runs
  -> module unregisters and frees resources
  -> code/data removed
```

If the module is in use:

- `rmmod` fails.
- `lsmod` shows a nonzero `Used by` count.
- `/sys/module/<name>/holders/` may show modules that depend on it.

Avoid forced unload. It can remove code while something still has a pointer into it.

## Minimal Practical Example

This is a learning-only module. It shows entry/exit, metadata, parameters, safe return values, and a build loop. It is not a production driver because it does not register with a real subsystem or manage hardware.

```c
// demo_module.c
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

static int debug;
static char *device_name = "demo0";
static int gpios[4];
static int gpio_count;

module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "Debug level: 0=off, 1=basic, 2=verbose");

module_param(device_name, charp, 0444);
MODULE_PARM_DESC(device_name, "Logical device name used by this demo");

module_param_array(gpios, int, &gpio_count, 0444);
MODULE_PARM_DESC(gpios, "Optional GPIO numbers for demonstration");

static int __init demo_module_init(void)
{
        int i;

        pr_info("demo_module: loaded name=%s debug=%d\n",
                device_name, debug);

        for (i = 0; i < gpio_count; i++)
                pr_info("demo_module: gpios[%d]=%d\n", i, gpios[i]);

        /*
         * A real driver would register with a subsystem here.
         * If registration fails, return a negative errno.
         */
        return 0;
}

static void __exit demo_module_exit(void)
{
        /*
         * A real driver would unregister and free resources here.
         */
        pr_info("demo_module: unloaded\n");
}

module_init(demo_module_init);
module_exit(demo_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Learning Example");
MODULE_DESCRIPTION("Kernel module fundamentals demo");
```

Minimal Makefile:

```makefile
obj-m := demo_module.o
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
```

Build and test:

```bash
make
modinfo ./demo_module.ko
sudo insmod ./demo_module.ko debug=1 device_name=mydev gpios=10,11
lsmod | grep demo_module
cat /sys/module/demo_module/parameters/debug
dmesg | tail
sudo rmmod demo_module
dmesg | tail
```

What matters in the example:

- `module_init()` and `module_exit()` define load/unload entry points.
- `MODULE_LICENSE("GPL")` prevents the missing-license taint trap and allows GPL-only exports.
- `module_param()` values are parsed before init runs.
- Parameter permissions create sysfs files under `/sys/module/demo_module/parameters/`.
- Init returns `0`; a real failed registration would return a negative errno.
- Exit reverses what init did.

## Common Bugs And Debugging

Start debugging from the symptom. Kernel module failures usually leave evidence in command output and the kernel log.

| Symptom | Likely cause | What to inspect | Fix pattern |
| --- | --- | --- | --- |
| `insmod: Unknown symbol ...` | Dependency not loaded, symbol not exported, GPL-only symbol used by non-GPL module. | `dmesg`, `modinfo -F depends`, `grep symbol /proc/kallsyms`. | Use `modprobe` after install/`depmod`; export needed symbol; fix `MODULE_LICENSE()`. |
| `Invalid module format` | Built against different kernel or config. | `modinfo ./x.ko | grep vermagic`, `uname -r`, `dmesg`. | Rebuild against the exact target kernel headers/build tree. |
| `Operation not permitted` on unload | Module unloading disabled or module in use. | Kernel config, `lsmod`, `/sys/module/<name>/refcnt`. | Enable unload support for development; close users; unload dependents first. |
| `/sys/module/<name>/parameters` missing parameter | Permission was `0`, parameter name differs, module not loaded. | `modinfo ./x.ko`, `/sys/module/<name>/parameters`. | Add nonzero permission and `MODULE_PARM_DESC()`, rebuild/reload. |
| Parameter cannot be changed | Permission is read-only or parameter not designed for runtime changes. | `ls -l /sys/module/<name>/parameters/<param>`. | Use writable mode only when runtime mutation is safe. |
| `modprobe: FATAL: Module not found` | Module not installed/indexed or wrong name. | `/lib/modules/$(uname -r)/`, `depmod -a`, `modinfo name`. | Install to module tree, run `depmod -a`, use module name without `.ko`. |
| Module loads but device driver does not bind | Module init registered driver, but no device matched. | `dmesg`, bus sysfs, aliases, DT/ACPI/ID table. | Fix device declaration, `MODULE_DEVICE_TABLE()`, compatible/ID/name, or bus registration. |
| Kernel oops during load | Bug in init or callback triggered during registration. | `dmesg`, oops backtrace, current process often `insmod`, `objdump -S module.ko`. | Fix init ordering and pointer/lifetime bug; add logs before risky steps. |
| Kernel tainted after load | Proprietary, unsigned, forced, or out-of-tree module. | `dmesg`, `/proc/sys/kernel/tainted`, `modinfo`. | Use compatible license, signing policy, and target deployment rules. |

High-signal commands:

```bash
modinfo ./demo_module.ko
modinfo -F license ./demo_module.ko
modinfo -F depends ./demo_module.ko
modinfo -F vermagic ./demo_module.ko

sudo insmod ./demo_module.ko debug=1
sudo rmmod demo_module
sudo modprobe demo_module
sudo modprobe -r demo_module

lsmod | grep demo_module
cat /proc/modules | grep demo_module
ls /sys/module/demo_module
ls /sys/module/demo_module/parameters
ls /sys/module/demo_module/holders

dmesg | tail -50
journalctl -k -n 50
```

Debugging discipline:

- Log each init step before and after important registration calls.
- Return the exact negative errno from failing init steps.
- Clean up in reverse order for partial init failures.
- Do not use forced unload to "fix" a busy module during development; find the reference.
- If `modprobe` behaves differently from `insmod`, inspect install location, `depmod`, aliases, dependencies, and `/etc/modprobe.d` options.

## Production Checklist

Use this checklist before treating a module as more than a learning experiment.

Build and compatibility:

- Build against the exact target kernel headers/config.
- Check `modinfo -F vermagic`.
- Decide whether the driver is in-tree, out-of-tree, DKMS-managed, or vendor-packaged.
- Know whether target modules are compressed as `.ko`, `.ko.xz`, or `.ko.zst`.
- Check module signing and Secure Boot/lockdown policy.

Metadata and parameters:

- Provide `MODULE_LICENSE()`, `MODULE_AUTHOR()`, and `MODULE_DESCRIPTION()`.
- Add `MODULE_PARM_DESC()` for every public parameter.
- Use read-only parameters unless runtime writes are safe.
- Treat module parameters as user-visible behavior once deployed.
- Avoid leaking sensitive information through parameters or logs.

Lifecycle:

- Init publishes only fully initialized objects.
- Exit unregisters everything that can call back into module code before freeing state.
- Failure paths unwind only what succeeded, in reverse order.
- No callback can run after its backing state is freed.
- No subsystem registration is left behind after unload.

Dependencies and loading:

- Prefer `modprobe` for installed modules.
- Run `depmod -a` after installing modules manually.
- Verify `modules.dep`, `modules.alias`, and `modinfo -F depends`.
- Use `MODULE_DEVICE_TABLE()` for bus-autoloaded drivers.
- Document required load order only when automatic dependency handling cannot express it.

Debug and operations:

- Test repeated `insmod`/`rmmod` cycles.
- Test bad parameter values and dependency failures.
- Check `dmesg` for warnings, taint, and cleanup logs.
- Avoid `rmmod -f` in normal workflows.
- Document how support engineers should collect `modinfo`, `lsmod`, `dmesg`, and `/sys/module` evidence.

## Interview Readiness

You are ready for module interview questions when you can explain the lifecycle without staring at API names.

Be able to answer:

- What happens from `insmod ./x.ko` to the init function?
- Why does `modprobe` handle dependencies but `insmod` does not?
- What does `MODULE_LICENSE("GPL")` change technically?
- How does `depmod` help `modprobe`?
- Why can a module fail to unload?
- How do module parameters reach init code?
- What is the difference between `module_init()` and `probe()`?
- What evidence do you collect when a module fails to load?

More practice: `interview/03-kernel-modules-fundamentals.md`.

## Kernel Version Notes

Module basics are stable, but examples around build commands, permissions, and subsystem helper signatures can be version-sensitive.

- External modules are normally built with `make -C /lib/modules/$(uname -r)/build M=$PWD modules`. Newer kernels also document a `make -f .../Makefile M=$PWD` style, but the `-C` form remains widely recognized.
- Older examples may use symbolic parameter permissions such as `S_IRUGO`; modern examples should prefer octal modes such as `0444` or `0644` unless the target tree says otherwise.
- `class_create()` and other subsystem APIs can change signatures across kernel versions; verify subsystem examples against the target headers.
- `struct module` internals are not a stable driver-facing API. Use observable tools and proper ownership APIs instead of depending on its layout.
- Module signing, lockdown, compression, and distro packaging behavior varies by distribution and product policy.
