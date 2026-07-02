# External Sources

This file records external sources used for validation.

## Chapter 01 - Linux Kernel And Driver Development Overview

External sources were used to correct stale release, coding-style, and build claims and to validate the documentation-first development workflow.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/process/howto.html` | Validated the source-tree documentation-first approach, licensing context, required process documents, and the expectation that new code follows current kernel guidance. |
| `https://docs.kernel.org/process/2.Process.html` | Validated the rolling release and patch-review model, merge-window flow, and subsystem-maintainer organization. |
| `https://www.kernel.org/category/releases.html` | Validated mainline, stable, longterm, and release-candidate categories; non-semantic major versioning; and changeable projected longterm support dates. |
| `https://docs.kernel.org/process/coding-style.html` | Validated indentation, preferred rather than absolute line-length guidance, typedef exceptions, comment intent, kernel-doc usage, and the distinction between reference counting and locking. |
| `https://docs.kernel.org/kbuild/makefiles.html` | Validated the high-level relationship among the top Makefile, `.config`, architecture Makefiles, common scripts, subdirectory Kbuild files, `vmlinux`, and modules. |
| `https://docs.kernel.org/kbuild/modules.html` | Validated the external-module build model and prepared-kernel-tree requirements. |
| `https://raw.githubusercontent.com/torvalds/linux/v6.8/drivers/base/platform.c` | Validated that `PLATFORM_DEVID_AUTO` assigns a device-core name ending in `.<id>.auto`, while manually printing `pdev->name` and `pdev->id` omits that suffix. |

The Chapter 01 sources were rechecked on June 6, 2026. The two learning-only
example modules were build-checked with modpost against
`6.8.0-124-generic` headers and passed strict `checkpatch.pl` with zero
findings. Step 4/final review returned PASS after the expected log names were
corrected. Privileged runtime loading was not performed.

## Chapter 02 - Environment Setup, Kernel Source, And Build Flow

External sources were used to correct stale build claims and validate
version-sensitive Kconfig, Kbuild, toolchain, source-verification, module, and
reproducibility behavior.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/process/changes.html` | Validated that build prerequisites and minimum tool versions depend on the selected kernel and configuration, including GCC/LLVM, Make, binutils, flex, bison, Python, OpenSSL, and `pahole`. |
| `https://docs.kernel.org/admin-guide/README.html` | Validated the configure/build/install flow, consistent `O=` use, configuration migration, verbose `V=1` builds, and platform-dependent installation behavior. |
| `https://docs.kernel.org/kbuild/kconfig.html` | Validated Kconfig frontend/search behavior and handling of new symbols when migrating configurations. |
| `https://docs.kernel.org/kbuild/kbuild.html` | Validated core Kbuild variables, compiler selection, output metadata, and separate output-directory concepts. |
| `https://docs.kernel.org/kbuild/makefiles.html` | Validated `obj-y`, `obj-m`, `obj-$(CONFIG_*)`, composite objects, recursive directories, and Kbuild clean integration. |
| `https://docs.kernel.org/kbuild/modules.html` | Validated external-module `M=` builds, matching build-tree requirements, `modules_prepare`, the `Module.symvers` limitation, module installation paths, and multi-file modules. |
| `https://docs.kernel.org/kbuild/headers_install.html` | Validated that `headers_install` exports sanitized UAPI headers and does not prepare an external-module build tree. |
| `https://docs.kernel.org/kbuild/llvm.html` | Validated current Clang/LLVM Kbuild support and why GNU `CROSS_COMPILE` is not the only cross-build model. |
| `https://docs.kernel.org/kbuild/reproducible-builds.html` | Validated timestamp, build user/host, path, initramfs, and signing-key inputs that affect reproducibility. |
| `https://www.kernel.org/releases.html` | Validated kernel release categories and the time-sensitive nature of active longterm versions and projected end dates. |
| `https://www.kernel.org/signature.html` | Validated cryptographic verification of kernel.org release archives and corrected the claim that reading Makefile version fields authenticates source. |

The Chapter 02 external documentation was checked on June 6, 2026. The
learning-only external module was build-checked with modpost against
`6.8.0-124-generic` headers; `modinfo` reported matching vermagic and GPL
licensing. Step 4/final review returned PASS. Privileged runtime loading was
not performed.

## Chapter 03 - Kernel Modules Fundamentals

External sources were used for targeted validation of version-sensitive module behavior and build commands.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/kbuild/modules.html` | Validated external module build/install flow, including `make -C <kernel_dir> M=$PWD modules`, `/lib/modules/$(uname -r)/build`, `modules_prepare`, `modules_install`, `INSTALL_MOD_PATH`, and `INSTALL_MOD_DIR`. |
| `https://docs.kernel.org/admin-guide/kernel-parameters.html` | Validated module parameter behavior for loadable modules via `modprobe` and for built-in module parameters via kernel command-line `module.parameter=value` style. |

The Chapter 03 example was build-checked locally against the available `6.8.0-111-generic` kernel headers during example verification. That is local target-header validation, not an external source.

## Chapter 04 - Kernel Logging, Error Handling, And Coding Practice

External sources were used to validate current logging behavior, dynamic debug, error-pointer APIs, and kernel coding-style guidance because the internal sources contain older or simplified claims.

Chapter 04 is **not** marked covered because Step 4/final review returned FAIL. These sources are recorded for traceability only; coverage completion remains blocked until the review fixes are applied and final review returns PASS.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/core-api/printk-basics.html` | Validated `printk()` log levels, `pr_*`, `pr_fmt()`, the kernel ring buffer, console log-level controls, conditional `pr_debug()`, and warnings about excessive printing in hot paths. |
| `https://docs.kernel.org/core-api/printk-formats.html` | Validated printk format specifiers, typed pointer formats, hashed `%p` behavior, and raw-address disclosure caveats. |
| `https://docs.kernel.org/admin-guide/dynamic-debug-howto.html` | Validated runtime control of `pr_debug()`/`dev_dbg()`, control-file paths, module/file/function selectors, and the implicit `dyndbg` parameter needed for module-init-time debug messages. |
| `https://docs.kernel.org/process/coding-style.html` | Validated cleanup-label guidance, meaningful labels, "one err bugs," message quality, quiet-driver expectations, `dev_*` use, and failure-path testing. |
| `https://docs.kernel.org/core-api/kernel-api.html#error-pointers` | Validated `ERR_PTR()`, `PTR_ERR()`, `IS_ERR()`, `IS_ERR_OR_NULL()`, `ERR_CAST()`, and `PTR_ERR_OR_ZERO()` contracts and the rule that encoded error pointers are opaque. |

The Chapter 04 example module was build-checked locally against the available `6.8.0-124-generic` kernel headers and passed `checkpatch.pl --no-tree --file logerr_demo.c` with zero errors and zero warnings. These local checks do not override the final-review FAIL or mark the topic covered.

## Chapter 05 - Core Kernel Facilities

External sources were used to validate current list, timer, workqueue,
completion, wait-queue, and kobject behavior because the internal sources contain
obsolete APIs and simplified lifecycle claims.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/core-api/list.html` | Validated intrusive circular lists, `container_of()`-based payload recovery, initialization, add/delete/traversal helpers, safe deletion traversal, cache-locality caveats, and caller-owned concurrency. |
| `https://docs.kernel.org/core-api/workqueue.html` | Validated concurrency-managed workqueues, worker pools, system versus allocated queues, queueing/coalescing, flags, cancellation, flushing, destruction, and teardown. |
| `https://docs.kernel.org/scheduler/completion.html` | Validated completion event retention and consumption, `complete_all()`, safe reinitialization, wait return handling, atomic-context signaling, and object lifetime. |
| `https://docs.kernel.org/core-api/kobject.html` | Validated kobject embedding, ktypes, ksets, hierarchy, sysfs relationship, get/put references, release callbacks, and subsystem-wrapper preference. |
| `https://docs.kernel.org/driver-api/basics.html` | Validated current timer/hrtimer, wait-queue, completion, workqueue, delay, and execution-context helper contracts, including synchronous timer deletion/shutdown. |

The Chapter 05 example module was build-checked with modpost against
`6.8.0-124-generic` headers and passed strict `checkpatch.pl` with zero errors,
warnings, or checks. These are local validation checks, not external sources.

## Chapter 06 - Synchronization And Concurrency Basics

External sources were used to correct stale context rules and add missing atomic,
reference-counting, real-time, and debugging guidance.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/locking/locktypes.html` | Validated sleeping, CPU-local, and spinning lock categories; nesting and owner rules; spinlock suffixes; and `PREEMPT_RT` behavior. |
| `https://docs.kernel.org/core-api/real-time/differences.html` | Validated forced-threaded interrupt behavior, RT `spinlock_t` semantics, and the role of `raw_spinlock_t` in true hard-IRQ or non-preemptible paths. |
| `https://docs.kernel.org/locking/mutex-design.html` | Validated mutex ownership, sleeping-lock behavior, non-recursion, and mutex storage/lifetime caveats. |
| `https://docs.kernel.org/kernel-hacking/locking.html` | Validated context-driven lock selection, process/IRQ spinlock patterns, trylock caveats, deadlock examples, and lock-scope documentation. |
| `https://docs.kernel.org/core-api/wrappers/atomic_t.html` | Validated atomic scalar read-modify-write APIs, ordering variants, MMIO exclusion, and the limits of non-RMW atomic access. |
| `https://docs.kernel.org/core-api/refcount-vs-atomic.html` | Validated why object lifetime counters should normally use `refcount_t` rather than generic atomics. |
| `https://docs.kernel.org/dev-tools/kcsan.html` | Validated KCSAN's purpose, marked-access handling, weak-memory support, and sampling limitations. |

The Chapter 06 external documentation was rechecked on June 6, 2026. The module
was build-checked with modpost against `6.8.0-124-generic` headers and passed
strict `checkpatch.pl`. These are local validation checks, not external sources.
Privileged runtime loading was not performed.

## Chapter 07 - Character Device Drivers

No external web sources were used for the Chapter 07 coverage update.

Version-sensitive API caveats are recorded in `coverage/topic-briefs/07-character-device-drivers.md`, `knowledge/07-character-device-drivers.md`, and `examples/07-character-device-drivers/README.md`:

- `class_create()` signature differs across kernel versions.
- `.poll` return type differs across kernel versions when poll support is implemented.

The Chapter 07 example was build-checked locally against the available `6.8.0-111-generic` kernel headers during example verification. That is local target-header validation, not an external source.

## Chapter 08 - Userspace ABI Design For Drivers

External sources were used for targeted validation of userspace ABI policy and version-sensitive driver interfaces.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/driver-api/ioctl.html` | Validated modern ioctl guidance, `_IO`, `_IOR`, `_IOW`, `_IOWR`, the difficulty of fixing broken ioctl ABI, and `compat_ioctl` concerns for 32-bit userspace on 64-bit kernels. |
| `https://docs.kernel.org/6.11/userspace-api/ioctl/ioctl-number.html` | Validated ioctl number uniqueness conventions, the userspace-perspective meaning of read/write in `_IOR`/`_IOW`, and the warning not to use `sizeof(arg)` as the third macro argument. |
| `https://docs.kernel.org/6.4/filesystems/sysfs.html` | Validated sysfs purpose, kobject relationship, `show()`/`store()` behavior, PAGE_SIZE buffer semantics, `sysfs_emit()`, and one-value/simple-values guidance. |
| `https://docs.kernel.org/next/admin-guide/gpio/sysfs.html` | Validated that GPIO sysfs is deprecated and that new userspace consumers should use the GPIO character-device ABI. |
| `https://docs.kernel.org/admin-guide/abi.html` | Validated kernel ABI documentation categories and userspace ABI stability expectations. |
| `https://docs.kernel.org/admin-guide/abi-obsolete.html` | Validated obsolete GPIO sysfs entries and replacement by the GPIO character-device ABI. |

The Chapter 08 example was build-checked locally against the available `6.8.0-124-generic` kernel headers during example verification. That is local target-header validation, not an external source.

## Chapter 09 - Platform Bus And Platform Drivers

External sources were used for targeted validation of platform driver model behavior and version-sensitive APIs.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/6.9/driver-api/driver-model/platform.html` | Validated the platform bus purpose, `platform_device`/`platform_driver` interface, platform resources, driver registration, `platform_driver_probe()`, and legacy platform-device enumeration model. |
| `https://docs.kernel.org/driver-api/infrastructure.html` | Validated `__platform_driver_register()`, `platform_driver_unregister()`, `__platform_driver_probe()`, `__platform_create_bundle()`, `platform_register_drivers()`, `platform_unregister_drivers()`, and platform-device helper behavior. |
| `https://docs.kernel.org/6.2/driver-api/driver-model/driver.html` | Validated generic driver-model callback behavior: `probe()` as the binding callback, resource release on probe failure, early `-EPROBE_DEFER`, and `remove()` quiesce/free responsibilities. |

The Chapter 09 example was build-checked locally against the available `6.8.0-111-generic` kernel headers during example verification. That is local target-header validation, not an external source.

## Chapter 10 - Device Tree Fundamentals

External sources were used for targeted validation of current Device Tree documentation, coding style, binding schema workflow, and overlay caveats.

| Source | Purpose |
| --- | --- |
| `https://kernel.org/doc/html/next/devicetree/usage-model.html` | Validated Device Tree as OS-readable hardware description, the tree of named nodes/properties, links between nodes, bindings as usage conventions, use of existing bindings, FDT/DTB history, and Linux use for platform identification, runtime configuration, and device population. |
| `https://docs.kernel.org/devicetree/bindings/dts-coding-style.html` | Validated current DTS naming/coding-style guidance: lowercase node/property names, dash for node/property names, underscore for labels, no leading zeros in unit addresses, preferred property ordering, and SoC DTSI versus board DTS organization. |
| `https://www.kernel.org/doc/html/latest/devicetree/bindings/writing-schema.html` | Validated current YAML/json-schema binding workflow and `dt_binding_check`, `dtbs_check`, and `DT_SCHEMA_FILES` validation commands. |
| `https://kernel.org/doc/html/v6.0/devicetree/bindings/writing-bindings.html` | Validated binding-design rules: describe hardware rather than Linux driver choices, use specific `compatible` strings, vendor-prefix device-specific properties, do not redefine common properties, and name multiple phandle entries with matching `*-names` when needed. |
| `https://docs.kernel.org/6.15/devicetree/bindings/submitting-patches.html` | Validated that compatible strings used in DTS should be documented in DT bindings and that DTS is treated as driver-independent hardware description. |
| `https://docs.kernel.org/devicetree/overlay-notes.html` | Validated overlay basics: overlays modify the live tree, active new nodes can create devices, label-target overlays require the base DT to be compiled with `-@`, target-path syntax is an alternative, and overlay node/property pointers must not outlive overlay removal. |

The Chapter 10 example was compile/decompile checked locally with `dtc`. It includes no kernel module and no live target-device run is claimed.

## Chapter 11 - Device Tree APIs And Driver Integration

External sources were used for targeted validation of current driver-side Device Tree APIs, platform resource helpers, GPIO descriptor guidance, fwnode direction, and version-sensitive API choices.

Chapter 11 is **not** marked covered yet because Step 4/final review returned FAIL. These sources are recorded for traceability only; coverage completion remains blocked until final review returns PASS.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/6.4/devicetree/kernel-api.html` | Validated current Device Tree kernel APIs including typed property readers, child/phandle lookup helpers, `of_parse_phandle()`, `of_parse_phandle_with_args()`, `of_irq_get()`, and `of_node_put()` reference handling. |
| `https://docs.kernel.org/driver-api/infrastructure.html` | Validated platform resource helpers including `platform_get_resource()`, `platform_get_resource_byname()`, `platform_get_irq()`, `platform_get_irq_byname()`, `devm_platform_ioremap_resource()`, and `devm_platform_get_and_ioremap_resource()`. |
| `https://docs.kernel.org/driver-api/driver-model/platform.html` | Validated platform device/driver binding behavior and platform-driver registration/probe model. |
| `https://docs.kernel.org/driver-api/gpio/consumer.html` | Validated descriptor-based GPIO consumer APIs and current guidance against new use of legacy integer GPIO APIs. |
| `https://docs.kernel.org/driver-api/gpio/board.html` | Validated Device Tree GPIO mapping conventions where consumer properties use `<function>-gpios` and drivers request descriptors by function name. |
| `https://docs.kernel.org/driver-api/device-io.html` | Validated `devm_ioremap_resource()` as a resource-aware MMIO mapping helper. |
| `https://docs.kernel.org/driver-api/media/v4l2-fwnode.html` | Validated the modern fwnode graph/media parsing direction and supported deferring detailed graph endpoint parsing to media topics. |

The Chapter 11 example module was build-checked locally against the available `6.8.0-124-generic` kernel headers, and the standalone DTS was compile-checked with `dtc`. The DTS emits an expected warning because it intentionally contains fake learning-only providers. These are local validation checks, not external sources.

## Chapter 12 - Linux Device Model

External sources were used for targeted validation of current driver-model, kobject, sysfs, and version-sensitive class/device helper behavior.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/driver-api/driver-model/bus.html` | Validated bus registration, bus match callback purpose, bus device/driver lists, bus iterators, and `/sys/bus/<bus>/devices` plus `/sys/bus/<bus>/drivers` views. |
| `https://docs.kernel.org/driver-api/infrastructure.html` | Validated current `class_create(const char *name)`, `class_destroy()`, `device_create()`, `device_create_with_groups()`, `device_destroy()`, class iterators, and driver-model infrastructure helpers. |
| `https://docs.kernel.org/core-api/kobject.html` | Validated kobjects, ksets, ktypes, kset parentage, sysfs directory relationship, reference-counted lifetime, and uevent concepts. |
| `https://docs.kernel.org/6.3/filesystems/sysfs.html` | Validated sysfs attribute behavior, PAGE_SIZE buffer expectations, one-value/simple-values guidance, and current `sysfs_emit()` / `sysfs_emit_at()` recommendation for `show()` callbacks. |
| `https://docs.kernel.org/6.8/driver-api/driver-model/device.html` | Validated `struct device` registration, reference-counted removal, `get_device()`/`put_device()`, and the recommendation that attributes needed at add time be supplied through attribute groups before `KOBJ_ADD`. |

The Chapter 12 example module was build-checked locally against the available `6.8.0-124-generic` kernel headers. That is local target-header validation, not an external source.

## Chapter 13 - Pin Control And GPIO Consumer APIs

External sources were used for targeted validation of current pinctrl state behavior, descriptor GPIO consumer APIs, and userspace GPIO ABI status because the internal material includes stale or typo-prone examples and older sysfs/libgpiod-era guidance.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/driver-api/pin-control.html` | Validated standard pinctrl state names and behavior: `default`, `init`, `sleep`, and `idle`, including automatic/default state handling and PM state-selection expectations. |
| `https://docs.kernel.org/driver-api/gpio/consumer.html` | Validated descriptor GPIO consumer APIs, direction/value semantics, logical active-low handling, raw accessors, `_cansleep` variants, `gpiod_is_active_low()`, and GPIO descriptor best practices. |
| `https://docs.kernel.org/next/admin-guide/gpio/sysfs.html` | Validated that GPIO sysfs is obsolete/deprecated and should not be presented as a new userspace ABI. |
| `https://docs.kernel.org/userspace-api/gpio/chardev.html` | Validated GPIO character-device v2 as current userspace ABI direction and the warning against userspace bit-banging hardware that should have a kernel driver/subsystem. |
| `https://docs.kernel.org/userspace-api/gpio/chardev_v1.html` | Validated that GPIO chardev v1 is obsolete, so older libgpiod/chardev assumptions should not be presented as timeless guidance. |

The Chapter 13 example module was build-checked locally against the available `6.8.0-124-generic` kernel headers, and the standalone DTS was compile-checked with `dtc`. The DTS warnings are expected because it intentionally contains fake learning-only providers. These are local validation checks, not external sources.

## Chapter 14 - GPIO Controller Drivers And IRQ Integration

External sources were used for targeted validation of current GPIO provider and IRQ-domain guidance because the internal material mixes older gpiolib irqchip helper styles, stale field names, and pre-`gpio_chip.irq` setup patterns.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/driver-api/gpio/driver.html` | Validated current GPIO provider responsibilities, `struct gpio_chip`, `devm_gpiochip_add_data()`, `struct gpio_irq_chip`, valid-mask handling for partial IRQ-capable banks, and the preferred modern GPIO irqchip setup model. |
| `https://docs.kernel.org/core-api/irq/irq-domain.html` | Validated IRQ-domain purpose, hwirq-to-virq mapping, GPIO interrupt-controller motivation, and current hierarchical-domain context that should only be mentioned carefully in learner-facing material. |

The Chapter 14 example contains no kernel module code. Its DTS was compile-checked locally with `dtc -@ -I dts -O dtb`, and the example README was kept learning-only because a runnable GPIO-controller driver would be highly hardware-specific. That local validation is not an external source.

## Chapter 15 - Interrupt Management

External sources were used for targeted validation of generic IRQ behavior, IRQ-domain terminology, and GPIO irqchip context because the internal material includes older helper styles and version-sensitive IRQ/gpiolib details.

Chapter 15 is **not** marked covered yet because Step 4/final review returned FAIL. These sources are recorded for traceability only; coverage completion remains blocked until final review returns PASS.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/core-api/genericirq.html` | Validated current generic IRQ terminology and APIs including `request_threaded_irq()`, `free_irq()`/synchronization behavior, generic IRQ handling, handler return values, and controller/flow-handler concepts. |
| `https://docs.kernel.org/core-api/irq/irq-domain.html` | Validated IRQ-domain purpose, hwirq-to-Linux-IRQ mapping, and why cascaded/GPIO interrupt controllers need domains. |
| `https://docs.kernel.org/driver-api/gpio/driver.html` | Validated current GPIO irqchip guidance, cascaded versus nested threaded GPIO irqchips, `struct gpio_chip.irq` / `struct gpio_irq_chip` setup direction, valid masks, and older helper-style caveats. |

The Chapter 15 example module was build-checked locally against the available `6.8.0-124-generic` kernel headers, and the DTS was compile-checked with `dtc -@ -I dts -O dtb`. These are local validation checks, not external sources. The final review still failed due to cleanup-order and audit-wording fixes that must be resolved before marking coverage complete.

## Chapter 16 - I2C Client Drivers

External sources were used for targeted validation of current I2C client-driver behavior, device instantiation, I2C transfer APIs, SMBus protocol behavior, and version-sensitive callback prototypes.

Chapter 16 is **not** marked covered yet because Step 4/final review returned FAIL. These sources are recorded for traceability only; coverage completion remains blocked until final review returns PASS.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/6.17/i2c/writing-clients.html` | Validated current I2C client-driver guidance, client/driver distinction, `i2c_set_clientdata()`/`i2c_get_clientdata()`, device binding/creation/detection cautions, `module_i2c_driver()`, PM/shutdown callbacks, plain I2C and SMBus APIs, return-value rules, and SMBus block buffer limit. |
| `https://docs.kernel.org/6.17/i2c/instantiating-devices.html` | Validated that I2C devices are not hardware-enumerated and must be instantiated explicitly, with Device Tree child nodes as a standard embedded-system method. |
| `https://docs.kernel.org/6.17/driver-api/i2c.html` | Validated current `struct i2c_driver`, `struct i2c_client`, `struct i2c_board_info`, `i2c_master_send()`/`i2c_master_recv()` return and size rules, task-context caveat for I2C functions, and current field/prototype drift. |
| `https://docs.kernel.org/6.17/i2c/smbus-protocol.html` | Validated SMBus protocol transaction terminology and SMBus command behavior used in the learner-facing explanation. |

The Chapter 16 example module was build-checked locally against the available `6.8.0-124-generic` kernel headers, and the DTS was compile-checked with `dtc -@ -I dts -O dtb`. These are local validation checks, not external sources. The final review still failed because `coverage/topic-briefs/16-i2c-client-drivers.md` contains stale final-validation wording that must be resolved before marking coverage complete.

## Chapter 17 - SPI Device Drivers

External sources were used for targeted validation of current SPI driver-model terminology, SPI transfer structure drift, userspace `spidev` behavior, and stale `compatible = "spidev"` guidance.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/6.17/driver-api/spi.html` | Validated current SPI architecture, controller/protocol-driver split, `struct spi_controller`, `struct spi_device`, `struct spi_driver`, message queue model, current `struct spi_transfer` field drift, controller queue behavior, and modern naming caveats versus older `spi_master` wording. |
| `https://docs.kernel.org/6.6/spi/spidev.html` | Validated userspace SPI API behavior, `read()`/`write()` half-duplex limitations, `SPI_IOC_MESSAGE(N)` for full-duplex/composite operations, ioctl settings, `/dev/spidevB.C` naming, sysfs binding notes, and upstream rejection of direct `compatible = "spidev"` device descriptions. |

The Chapter 17 example module was build-checked locally against the available `6.8.0-124-generic` kernel headers, and the DTS overlay was compile/decompile checked with `dtc`. These are local validation checks, not external sources. Chapter 17 was marked `covered` only after final review returned PASS.

## Chapter 18 - Regmap API

External sources were used for targeted validation of current regmap header/API drift and current kernel documentation coverage.

| Source | Purpose |
| --- | --- |
| `https://github.com/torvalds/linux/blob/master/include/linux/regmap.h` | Validated current `struct regmap_config` field drift, bus initialization prototypes, managed init helpers, `use_single_read`/`use_single_write`, no-increment callbacks/tables, cache type field, endian fields, and bus callbacks. |
| `https://docs.kernel.org/driver-api/index.html` | Confirmed current kernel driver API documentation organization and that no single comprehensive regmap tutorial was found in the browsed driver API index. |

The Chapter 18 example module was build-checked locally against the available `6.8.0-124-generic` kernel headers, and the DTS was compile-checked with `dtc -@ -I dts -O dtb`. These are local validation checks, not external sources. Chapter 18 was marked `covered` only after Step 4/final review returned PASS.

## Chapter 19 - Regmap IRQ And MFD/Syscon

External sources were used for targeted validation of current regmap IRQ, MFD, syscon, and binding API drift.

| Source | Purpose |
| --- | --- |
| `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/regmap.h` | Validated current `struct regmap_irq`, `struct regmap_irq_type`, `struct regmap_irq_chip`, `regmap_add_irq_chip()`, `devm_regmap_add_irq_chip()`, fwnode variants, `regmap_irq_chip_get_base()`, `regmap_irq_get_virq()`, and `regmap_irq_get_domain()`. |
| `https://raw.githubusercontent.com/torvalds/linux/master/drivers/base/regmap/regmap-irq.c` | Validated current regmap IRQ implementation: threaded parent IRQ with `IRQF_ONESHOT`, IRQ domain setup, child IRQ mapping, and `handle_nested_irq()` dispatch. |
| `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/mfd/core.h` | Validated current `struct mfd_cell`, `mfd_get_cell()`, `mfd_add_devices()`, `devm_mfd_add_devices()`, `mfd_remove_devices()`, and field drift. |
| `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/mfd/syscon.h` | Validated current syscon lookup helper prototypes and `CONFIG_MFD_SYSCON` fallback behavior. |
| `https://raw.githubusercontent.com/torvalds/linux/master/Documentation/devicetree/bindings/mfd/syscon.yaml` | Validated current syscon YAML schema direction, `compatible` and `reg` requirements, and modern schema location. |
| `https://raw.githubusercontent.com/torvalds/linux/master/Documentation/devicetree/bindings/mfd/mfd.txt` | Validated the documented `simple-mfd` binding concept and example. |

The Chapter 19 learning-only DTS example was compile-checked with `dtc -@ -I dts -O dtb` and roundtrip decompiled with `dtc -I dtb -O dts`. These are local validation checks, not external sources. No kernel module was built or loaded because the example is DTS-only and uses training-only fake bindings. Chapter 19 was marked `covered` only after Step 4/final review returned PASS.

## Chapter 20 - Kernel Memory Management For Drivers

External sources were used to correct stale internal allocator, highmem, devres, DMA-address, and debugging guidance and to validate current version-sensitive APIs.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/core-api/memory-allocation.html` | Validated allocator selection, `GFP_KERNEL`, `GFP_NOWAIT`, restrained `GFP_ATOMIC` use, avoidance of legacy `GFP_DMA`, overflow-safe array helpers, `kvmalloc()`, and architecture/configuration-dependent allocation limits. |
| `https://docs.kernel.org/core-api/mm-api.html` | Validated current `kmalloc()`/`kzalloc()`/`kcalloc()`/`kmalloc_array()`/`krealloc()`/`kvmalloc()`/`kvfree()` behavior and sensitive-free helpers. |
| `https://docs.kernel.org/mm/highmem.html` | Validated architecture-dependent highmem behavior and preference for `kmap_local_page()`/`kunmap_local()` over legacy `kmap()` and `kmap_atomic()` patterns. |
| `https://docs.kernel.org/driver-api/driver-model/devres.html` | Validated devres association with `struct device`, release on detach/probe failure, managed allocation patterns, and the fact that devres does not replace callback and async-user synchronization. |
| `https://docs.kernel.org/core-api/dma-api-howto.html` | Validated CPU virtual, CPU physical, and DMA address distinctions; suitable streaming-DMA backing sources; rejection of stack/module-image/ordinary `vmalloc()` addresses as one direct DMA buffer; and use of the DMA API. |
| `https://docs.kernel.org/core-api/dma-api.html` | Validated coherent allocation contracts, DMA masks, streaming mapping constraints, mapping-error handling, and why physical-address conversion is not a portable DMA interface. |
| `https://docs.kernel.org/dev-tools/kmemleak.html` | Validated kmemleak purpose, debugfs workflow, tracked allocation families, and limitations such as page allocations and `ioremap()` not being tracked. |
| `https://docs.kernel.org/dev-tools/kasan.html` | Validated KASAN use for out-of-bounds and use-after-free detection and allocation/free stack reporting. |
| `https://docs.kernel.org/admin-guide/mm/slab.html` | Validated SLUB debugging controls and `slabinfo` as allocator/cache-corruption investigation tools. |

The Chapter 20 learning-only module was build-checked locally against the available `6.8.0-124-generic` kernel headers and passed strict `checkpatch.pl --file` with zero errors and zero warnings. These are local validation checks, not external sources. Privileged module loading and architecture-specific highmem, DMA-zone, cache-coherency, and driver-`mmap()` validation were not performed. Chapter 20 was marked `covered` only after Step 4/final review returned PASS.

## Chapter 21 - DMA And DMA Mapping

External sources were used to correct stale or invalid internal DMA examples and to validate current generic DMA mapping, DMAengine, DMA-BUF, and firmware-coherency behavior.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/core-api/dma-api-howto.html` | Validated CPU virtual/physical/DMA address separation, mask setup, coherent versus streaming contracts, prohibited buffer sources, direction and ownership, mapping-error checks, SG count rules, sync lifecycle, DMA pools, and coherent-memory barrier requirements. |
| `https://docs.kernel.org/core-api/dma-api.html` | Validated current generic DMA API signatures, `dma_max_mapping_size()`, mapping failures, SG merging and original-count unmapping, partial synchronization, noncontiguous allocations, and DMA API debugging. |
| `https://docs.kernel.org/driver-api/dmaengine/client.html` | Validated named channel acquisition, `dmaengine_slave_config()`, mapping with the DMA channel's device, descriptor preparation/submission, issue-pending, callback context, and terminate/synchronize teardown. |
| `https://docs.kernel.org/driver-api/dma-buf.html` | Validated the boundary between local DMA mappings and cross-driver DMA-BUF sharing, including attachments, per-device SG mappings, CPU access bracketing, fences, and reservation objects. |
| `https://docs.kernel.org/devicetree/kernel-api.html` | Validated that DMA coherency is established by firmware/platform DMA configuration rather than an ad hoc client-driver `dma-coherent` switch. |

The Chapter 21 learning-only module was build/modpost checked locally against the available `6.8.0-124-generic` kernel headers and passed strict `checkpatch.pl --no-tree --strict --file` with zero findings. These are local validation checks, not external sources. Privileged module loading, a real DMA transfer, non-coherent target behavior, IOMMU fault injection, and `dmatest` execution were not performed. Chapter 21 was marked `covered` only after Step 4/final review returned PASS.

## Chapter 22 - Common Clock Framework

External sources were used to correct v4.19-era CCF API drift and validate current provider, consumer, and Device Tree binding behavior.

| Source | Purpose |
| --- | --- |
| `https://raw.githubusercontent.com/torvalds/linux/master/Documentation/driver-api/clk.rst` | Validated current high-level CCF split, `CONFIG_COMMON_CLK`, `struct clk_ops`, provider `struct clk_hw` pattern, callback capability matrix, and `clk_ignore_unused` debug behavior. |
| `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/clk-provider.h` | Validated current provider-side API drift: `struct clk_parent_data`, current `struct clk_init_data`, provider `clk_hw_*` helpers, managed helpers, parent-data/parent-hw variants, `CLK_OPS_PARENT_ENABLE`, and current OF provider helpers. |
| `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/clk.h` | Validated current consumer-side API drift: bulk APIs, optional clocks, managed enabled/prepared helpers, rate-exclusive helpers, rate range, phase/duty-cycle APIs, and clock notifiers. |
| `https://raw.githubusercontent.com/torvalds/linux/master/Documentation/devicetree/bindings/clock/clock-bindings.txt` | Validated that the old common clock text binding has moved to DT schema and remains useful only as historical binding context. |
| `https://raw.githubusercontent.com/torvalds/linux/master/Documentation/devicetree/bindings/clock/fixed-clock.yaml` | Validated current fixed-clock YAML requirements: `compatible = "fixed-clock"`, `#clock-cells = <0>`, required `clock-frequency`, optional `clock-accuracy`, and `clock-output-names`. |

The Chapter 22 learning-only DTS example was compile-checked with `dtc -@ -I dts -O dtb` and roundtrip decompiled with `dtc -I dtb -O dts`. These are local validation checks, not external sources. No kernel module was built or loaded because the example is DTS-only and uses training-only fake consumer bindings. Chapter 22 was marked `covered` only after Step 4/final review returned PASS.

## Chapter 23 - Regulator Framework

External sources were used to validate current regulator consumer/provider API drift, current binding behavior, and version-sensitive helper availability because the primary internal regulator chapter contains older board-file and manual-registration patterns.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/driver-api/regulator.html` | Validated current regulator overview: consumer/provider split, machine constraints, sysfs/debug role, consumer APIs, bulk helpers, optional/exclusive/get-enable helpers, load/mode behavior, notifier/error APIs, and provider registration concepts. |
| `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/regulator/consumer.h` | Validated current consumer API drift: `devm_regulator_get_optional()`, `devm_regulator_get_enable()`, `devm_regulator_get_enable_optional()`, bulk helpers, load/current/voltage APIs, error flags, notifiers, and opaque `struct regulator *` use. |
| `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/regulator/driver.h` | Validated current provider API drift: `struct regulator_desc`, `struct regulator_ops`, `struct regulator_config`, `devm_regulator_register()`, linear range helpers, active discharge, bypass, pull-down, error-flag/provider callbacks, and provider-private data accessors. |
| `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/regulator/machine.h` | Validated current `struct regulation_constraints`, valid operation/mode masks, suspend-state fields, `always_on`, `boot_on`, coupling, and machine-constraint concepts. |
| `https://raw.githubusercontent.com/torvalds/linux/master/Documentation/devicetree/bindings/regulator/regulator.yaml` | Validated current common regulator binding properties and YAML schema direction. |
| `https://raw.githubusercontent.com/torvalds/linux/master/Documentation/devicetree/bindings/regulator/fixed-regulator.yaml` | Validated current fixed-regulator binding concepts, fixed-voltage constraints, GPIO-controlled enable properties, and delay/startup fields for the learning DTS example. |

The Chapter 23 learning-only DTS example was compile-checked with `dtc -@ -I dts -O dtb` and roundtrip decompiled with `dtc -I dtb -O dts`. These are local validation checks, not external sources. No kernel module was built or loaded because the example is DTS-only and uses training-only fake consumer bindings. Chapter 23 was marked `covered` only after Step 4/final review returned PASS following the optional-regulator `-ENODEV` fix.

## Chapter 24 - Power Management

External sources were used to validate current runtime PM, system sleep, wakeup, and PM helper behavior because the primary internal PM chapter targets Linux 4.19 and contains version-sensitive helper and binding-path details.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/power/runtime_pm.html` | Validated current runtime PM model, `pm_wq`, `struct dev_pm_info`, callback precedence, process-context defaults, `pm_runtime_irq_safe()`, usage/child counters, idle/suspend/resume rules, and helper behavior. |
| `https://docs.kernel.org/driver-api/pm/devices.html` | Validated current device PM basics, system sleep versus runtime PM, wakeup policy, quiesced I/O expectations, subsystem collaboration, and interaction between runtime-suspended devices and system-wide transitions. |
| `https://docs.kernel.org/admin-guide/pm/sleep-states.html` | Validated current `/sys/power/state`, `/sys/power/mem_sleep`, `/sys/power/disk`, suspend-to-idle/standby/suspend-to-RAM/hibernation terminology, and sysfs interface framing. |
| `https://docs.kernel.org/driver-api/pm/index.html` | Validated current PM documentation organization and modern topics missing from internal sources, including smart suspend and may-skip-resume flags. |
| `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/pm_runtime.h` | Validated current runtime PM helper drift, including `pm_runtime_get_sync()` usage-counter behavior on error, `pm_runtime_resume_and_get()`, `pm_runtime_put_autosuspend()`, and runtime PM guard macros. |
| `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/pm.h` | Validated current `struct dev_pm_ops`, `SYSTEM_SLEEP_PM_OPS`, `RUNTIME_PM_OPS`, `SET_*` macros, `DEFINE_SIMPLE_DEV_PM_OPS()`, and deprecation notes for `SIMPLE_DEV_PM_OPS()` and `UNIVERSAL_DEV_PM_OPS()`. |

The Chapter 24 example is learning-only and README-only. No kernel module was built or loaded because a fake module cannot validate real runtime PM, wake IRQ, parent/child ordering, power domains, clocks, regulators, or system sleep behavior. Chapter 24 was marked `covered` only after Step 4/final review returned PASS.

## Chapter 25 - IIO Framework

External sources were used to validate current IIO core, buffered capture, trigger, and userspace ABI behavior because the primary internal IIO chapter contains version-sensitive helper and sysfs-layout details.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/driver-api/iio/core.html` | Validated current IIO core model: `iio_device_alloc()`, `iio_device_register()`, reverse-order unregister/free, sysfs attributes, `/dev/iio:deviceX`, channels, `struct iio_chan_spec`, and info-mask grouping. |
| `https://docs.kernel.org/driver-api/iio/buffers.html` | Validated driver-API buffer concepts, scan elements, `length`, `enable`, and `/sys/bus/iio/devices/iio:deviceX/scan_elements/`. |
| `https://docs.kernel.org/iio/iio_devbuf.html` | Validated current userspace buffer docs, `bufferY` directory wording, and enabling buffers after length and scan-element selection. |
| `https://docs.kernel.org/driver-api/iio/triggers.html` | Validated trigger model, trigger sysfs paths, `current_trigger`, independent trigger sources, and managed trigger helper names. |
| `https://docs.kernel.org/driver-api/iio/triggered-buffers.html` | Validated triggered-buffer setup order: initialize `indio_dev`, set up the triggered buffer before `iio_device_register()`, and clean up with `iio_triggered_buffer_cleanup()`. |
| `https://docs.kernel.org/iio/adxl345.html` | Validated a current applied accelerometer example: raw/scale/calibration/sampling-frequency attributes, processed formula `(_raw + _offset) * _scale`, event examples, and userspace buffer flow. |

The Chapter 25 learning-only module example was build-checked against local `6.8.0-124-generic` headers and then cleaned. Privileged runtime loading was not performed. Chapter 25 was marked `covered` only after Step 4/final review returned PASS following the IIO example scale fix.

## Chapter 26 - Input Device Drivers

External sources were used to validate current input-core, evdev, event-code, and `gpio-keys` behavior because the primary internal input chapter contains legacy polled-input APIs and extraction issues in its examples.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/input/input.html` | Validated current high-level input architecture: device drivers report events into the input core, input handlers consume them, and evdev exposes `/dev/input/eventX` nodes as the preferred userspace interface. |
| `https://docs.kernel.org/input/input-programming.html` | Validated current driver-programming flow: allocate/fill/register `struct input_dev`, set event capabilities, report events with `input_report_*()`, call `input_sync()`, use `open()`/`close()` for demand-driven resources, and prefer current polling helpers over legacy `input_polled_dev`. |
| `https://docs.kernel.org/input/event-codes.html` | Validated event type/code/value framing, `EV_SYN`/`SYN_REPORT`, key/axis semantics, and userspace interpretation requirements. |
| `https://github.com/torvalds/linux/blob/master/Documentation/devicetree/bindings/input/gpio-keys.yaml` | Validated current `gpio-keys`/`gpio-keys-polled` binding direction, including `compatible`, `autorepeat`, `poll-interval`, child key nodes, `gpios`/`interrupts`, `linux,code`, debounce, and wakeup properties. |
| `https://raw.githubusercontent.com/torvalds/linux/master/include/uapi/linux/input-event-codes.h` | Validated the current UAPI location for input event type, key/button, and axis code constants. |

The Chapter 26 learning-only module example was build-checked against local `6.8.0-124-generic` headers, passed strict `checkpatch.pl`, and then cleaned. Local headers expose the APIs used by the learner docs and example, including `devm_input_allocate_device()`, `input_setup_polling()`, `input_set_poll_interval()`, `input_set_capability()`, `input_set_abs_params()`, `input_set_drvdata()`, and `devm_request_threaded_irq()`. Local headers do not expose `input_set_poll_interval_min()` or `input_set_poll_interval_max()`, so learner docs avoid presenting those as current validated APIs for this target. Privileged runtime loading and evdev interaction were not performed. Chapter 26 was marked `covered` only after Step 4/final review returned PASS.

## Chapter 27 - RTC And PWM Drivers

External sources were used to validate current RTC userspace framing and current PWM state-based API guidance because the primary internal RTC/PWM chapters contain version-sensitive or older helper/API patterns.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/admin-guide/rtc.html` | Validated current user-facing RTC framing: RTCs normally track UTC wall-clock time, portable class devices are `/dev/rtcN`, `/sys/class/rtc/rtcN`, and `/proc/driver/rtc`, not every RTC has IRQ/alarm capability, and multiple RTCs can exist on embedded systems. |
| `https://docs.kernel.org/driver-api/pwm.html` | Validated current PWM consumer guidance: `pwm_get()` / `devm_pwm_get()`, `pwm_apply_might_sleep()`, `pwm_apply_atomic()` gated by `pwm_might_sleep()`, `pwm_get_state()`, `pwm_get_args()`, sysfs files, `.apply()` / `.get_state()` provider preference, locking, and PM responsibility split. |

Local headers were inspected at `/lib/modules/6.8.0-124-generic/build/include/linux/rtc.h` and `/lib/modules/6.8.0-124-generic/build/include/linux/pwm.h`. The learner docs avoid relying on stale RTC/PWM helper signatures where local headers differ from older internal examples. The Chapter 27 example is learning-only and DTS/README-based; no kernel module was built or loaded because a fake combined RTC/PWM module would not validate real RTC alarm wake or PWM waveform behavior. Chapter 27 was marked `covered` only after Step 4/final review returned PASS.

## Chapter 28 - Watchdog And NVMEM Frameworks

External sources were used to validate current watchdog kernel/userspace behavior and current NVMEM provider/consumer behavior because the primary internal watchdog/NVMEM chapters are based on Linux 4.19-era APIs and contain version-sensitive helper names and ABI details.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/watchdog/watchdog-kernel-api.html` | Validated current watchdog kernel-side API names, `struct watchdog_device`, `struct watchdog_ops`, helper functions, registration flow, driver-data handling, timeout initialization, and lifetime guidance. |
| `https://docs.kernel.org/watchdog/watchdog-api.html` | Validated current userspace watchdog behavior through `/dev/watchdog`, keepalive writes/ioctls, magic close, timeout ioctls, boot/status reporting, and numbered watchdog device nodes. |
| `https://docs.kernel.org/driver-api/nvmem.html` | Validated current NVMEM provider/consumer overview, sysfs behavior, cell-based access, and modern NVMEM concepts beyond the v4.19 internal source baseline. |

Local headers were inspected at `/lib/modules/6.8.0-124-generic/build/include/linux/watchdog.h`, `/lib/modules/6.8.0-124-generic/build/include/uapi/linux/watchdog.h`, `/lib/modules/6.8.0-124-generic/build/include/linux/nvmem-provider.h`, `/lib/modules/6.8.0-124-generic/build/include/linux/nvmem-consumer.h`, and `/lib/modules/6.8.0-124-generic/build/include/linux/rtc.h`. The learner docs avoid stale NVMEM include spelling, avoid claiming `devm_nvmem_unregister()` exists on the validated local target, and use current watchdog helpers such as `watchdog_init_timeout()`, `watchdog_stop_on_reboot()`, and `watchdog_stop_on_unregister()`. The Chapter 28 example is learning-only and DTS/README-based; no kernel module was built or loaded because a fake watchdog/NVMEM module would not validate real reset behavior, NVMEM write policy, OTP/eFuse safety, or board-specific bindings. Chapter 28 was marked `covered` only after Step 4/final review returned PASS following the watchdog `.set_timeout` example fix.

## Chapter 29 - Framebuffer And Display Basics

External sources were used to validate current fbdev userspace/kernel behavior, DRM/KMS context, fbdev emulation, and firmware framebuffer handoff because the primary internal framebuffer chapter is older fbdev-centric material.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/fb/index.html` | Validated that current kernel documentation still groups fbdev material under Frame Buffer and links API, color map, deferred I/O, fbcon, internals, and mode database documentation. |
| `https://docs.kernel.org/fb/api.html` | Validated current userspace framebuffer API framing and the caveat that driver behavior can differ. |
| `https://docs.kernel.org/fb/framebuffer.html` | Validated the `/dev/fb*` model, framebuffer character-device major/minor framing, and read/write/seek/mmap programming model. |
| `https://docs.kernel.org/fb/internals.html` | Validated current fbdev internal object roles, including `fb_fix_screeninfo`, `fb_var_screeninfo`, `fb_cmap`, `fb_info`, and driver-private data. |
| `https://docs.kernel.org/gpu/drm-kms.html` | Validated DRM/KMS framebuffer object terminology, reference-count lifetime, and the modern DRM context around fbdev emulation. |
| `https://docs.kernel.org/gpu/drm-kms-helpers.html` | Validated DRM/KMS fbdev-emulation helper context and shadow/deferred helper concepts. |
| `https://docs.kernel.org/gpu/todo.html` | Validated the modern kernel direction that very simple fbdev drivers can often be converted by starting with a new DRM driver using simple KMS/SHMEM helpers. |
| `https://docs.kernel.org/driver-api/aperture.html` | Validated framebuffer aperture ownership and handoff between generic firmware framebuffer drivers and native graphics drivers. |

Local validation inspected `/lib/modules/6.8.0-124-generic/build/include/linux/fb.h`, `/lib/modules/6.8.0-124-generic/build/include/uapi/linux/fb.h`, `/lib/modules/6.8.0-124-generic/build/drivers/video/fbdev/Kconfig`, `/lib/modules/6.8.0-124-generic/build/drivers/gpu/drm/Kconfig`, and `/boot/config-6.8.0-124-generic`. The validated local target exposes `struct fb_ops`, `struct fb_info`, `register_framebuffer()`, `devm_register_framebuffer()`, `framebuffer_alloc()`, `framebuffer_release()`, `CONFIG_FB`, `CONFIG_DRM`, `CONFIG_DRM_FBDEV_EMULATION`, and `CONFIG_DRM_SIMPLEDRM`. The Chapter 29 example is learning-only userspace fbdev ABI code, was compile-checked with `gcc -Wall -Wextra -O2`, and no kernel module or fake display driver was built or loaded. Chapter 29 was marked `covered` only after Step 4/final review returned PASS.

## Chapter 30 - Network Interface Drivers

External sources were used to validate current netdev lifetime, NAPI, PHYLINK, and ethtool-netlink behavior because the primary internal NIC chapter is older and does not deeply teach modern NAPI or PHYLINK.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/networking/netdevices.html` | Validated current `struct net_device` lifetime rules, `alloc_netdev*()`/private-data lifetime, `register_netdev()`/`unregister_netdev()` ordering, RTNL caveats, and the rule that all initialization must be complete before registration makes the device visible. |
| `https://docs.kernel.org/networking/napi.html` | Validated NAPI as the current networking event-handling mechanism, `struct napi_struct`, poll method, control API, scheduling, IRQ masking, non-idempotent enable/disable rules, and budget-driven RX/TX completion. |
| `https://docs.kernel.org/networking/kapi.html` | Validated current kernel networking APIs and PHYLINK API descriptions used in learner-facing API anchors. |
| `https://docs.kernel.org/networking/sfp-phylink.html` | Validated modern PHYLINK modes and conversion guidance from older PHYLIB-style drivers, including fixed links, in-band mode, `phylink_start()`, `phylink_stop()`, ethtool integration, and DT/FWNODE PHY connection. |
| `https://docs.kernel.org/networking/ethtool-netlink.html` | Validated current userspace ethtool netlink framing and privilege model; learner docs focus on driver-side `struct ethtool_ops` rather than netlink internals. |

Local headers were inspected at `/lib/modules/6.8.0-124-generic/build/include/linux/netdevice.h`, `/lib/modules/6.8.0-124-generic/build/include/linux/etherdevice.h`, `/lib/modules/6.8.0-124-generic/build/include/linux/skbuff.h`, `/lib/modules/6.8.0-124-generic/build/include/linux/ethtool.h`, `/lib/modules/6.8.0-124-generic/build/include/linux/phylink.h`, and `/lib/modules/6.8.0-124-generic/build/include/linux/phy.h`. The validated local target exposes the APIs used by the learner docs and learning-only example, including `struct net_device`, `struct net_device_ops`, `struct napi_struct`, `struct sk_buff`, `struct ethtool_ops`, `struct phylink`, `alloc_etherdev()`, `devm_alloc_etherdev()`, `register_netdev()`, `devm_register_netdev()`, `unregister_netdev()`, `free_netdev()`, `netif_napi_add()`, `napi_schedule()`, `napi_complete_done()`, `napi_gro_receive()`, `eth_type_trans()`, `eth_validate_addr()`, `eth_hw_addr_set()`, `device_get_ethdev_address()`, and `platform_get_ethdev_address()`. The Chapter 30 example is learning-only, build-checked against Linux `6.8.0-124-generic` headers, then cleaned; privileged runtime loading was not performed. Chapter 30 was marked `covered` only after Step 4/final review returned PASS following the example README ethtool wording fix.

## Chapter 31 - PCI Device Drivers

External sources were used to validate current PCI driver, MSI/MSI-X, managed cleanup, and DMA/API guidance because the primary internal PCI chapter is based on older kernel-era examples and includes stale API names.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/PCI/pci.html` | Validated current PCI driver registration, probe initialization sequence, resource handling, DMA setup, IRQ registration, and shutdown ordering. |
| `https://docs.kernel.org/PCI/msi-howto.html` | Validated modern MSI/MSI-X APIs, vector flags, `pci_irq_vector()`, `PCI_IRQ_INTX`, `PCI_IRQ_ALL_TYPES`, and the managed cleanup interaction with `pcim_enable_device()`. |
| `https://docs.kernel.org/driver-api/pci/pci.html` | Validated PCI support-library APIs, capability helpers, lookup/reference behavior, and current kernel API vocabulary. |
| `https://docs.kernel.org/PCI/index.html` | Validated the official PCI documentation scope and adjacent advanced PCI topics. |

Local headers were inspected at `/lib/modules/6.8.0-124-generic/build/include/linux/pci.h`, `/lib/modules/6.8.0-124-generic/build/include/linux/pci_ids.h`, and `/lib/modules/6.8.0-124-generic/build/include/linux/dma-mapping.h`. The validated local target exposes the APIs and symbols used by the learner docs, including `struct pci_driver`, `struct pci_device_id`, `module_pci_driver()`, `pcim_enable_device()`, `pci_set_master()`, `pci_request_regions()`, `pcim_iomap_regions()`, `pcim_iomap_table()`, `pci_alloc_irq_vectors()`, `pci_irq_vector()`, `PCI_IRQ_INTX`, deprecated alias `PCI_IRQ_LEGACY`, `PCI_IRQ_ALL_TYPES`, and `dma_set_mask_and_coherent()`. The Chapter 31 example is learning-only and README-only; no kernel module was built or loaded because a fake PCI module would not validate real BARs, DMA, interrupts, or device-specific register behavior. Chapter 31 was marked `covered` only after Step 4/final review returned PASS.

## Chapter 32 - V4L2 Core, Video Device, And VB2 Capture

External sources were used for targeted validation of current V4L2/vb2 behavior because the internal source material includes v4.19-era API details and version-sensitive media framework notes.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/driver-api/media/v4l2-videobuf2.html` | Validated current videobuf2 concepts, `struct vb2_queue`, `struct vb2_ops`, memory operations, driver-private buffer struct rules, and the vb2-aware video unregister caveat for drivers using vb2 file-operation release helpers. |
| `https://docs.kernel.org/userspace-api/media/v4l/vidioc-reqbufs.html` | Validated `VIDIOC_REQBUFS`, memory model setup, granted buffer count semantics, unsupported method `EINVAL`, buffer reallocation/freeing behavior, and `v4l2_requestbuffers.capabilities`. |
| `https://docs.kernel.org/userspace-api/media/v4l/vidioc-qbuf.html` | Validated `VIDIOC_QBUF`/`VIDIOC_DQBUF`, valid buffer index ranges, multi-planar `struct v4l2_plane` use, and queue/dequeue behavior. |
| `https://docs.kernel.org/userspace-api/media/v4l/vidioc-streamon.html` | Validated `VIDIOC_STREAMON`/`VIDIOC_STREAMOFF`, queued buffer behavior, stream-type argument, DMA abort/finish semantics, and queued/done buffer removal on streamoff. |
| `https://docs.kernel.org/userspace-api/media/v4l/io.html` | Validated V4L2 userspace I/O methods, streaming I/O with MMAP/USERPTR/DMABUF, and why streaming setup starts with `VIDIOC_REQBUFS`. |
| `https://docs.kernel.org/driver-api/media/v4l2-intro.html` | Validated the high-level V4L2 model: bridge drivers, connected sensor ICs, sub-device framework, video nodes, and shared framework code for buffer handling. |

The Chapter 32 example was not build-checked as a standalone module because it intentionally ships no module source. It is a learning-only `vivid` userspace ABI lab plus pseudo-code skeleton.

## Chapter 34 - V4L2 Userspace, Debugging, And Compliance

External sources were used for targeted validation of current V4L2 userspace ABI behavior, media debugging workflows, and v4l-utils tool behavior because the internal source material includes v4.19-era details and version-sensitive command/debug notes.

| Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/userspace-api/media/v4l/io.html` | Validated V4L2 I/O method categories, read/write default behavior, and that streaming I/O methods are selected with `VIDIOC_REQBUFS`. |
| `https://docs.kernel.org/userspace-api/media/v4l/vidioc-querycap.html` | Validated `VIDIOC_QUERYCAP`, `capabilities` versus `device_caps`, `V4L2_CAP_DEVICE_CAPS`, and capture/multi-planar capability flags. |
| `https://docs.kernel.org/userspace-api/media/v4l/vidioc-reqbufs.html` | Validated modern `VIDIOC_REQBUFS` semantics, granted buffer count, `EINVAL` for unsupported I/O method, buffer capabilities, orphaned buffers, and the distinction between MMAP allocation and USERPTR/DMABUF method setup. |
| `https://docs.kernel.org/userspace-api/media/v4l/vidioc-querybuf.html` | Validated `VIDIOC_QUERYBUF`, buffer index range, matching buffer type requirements, multi-planar `m.planes`, and MMAP buffer metadata. |
| `https://docs.kernel.org/userspace-api/media/v4l/vidioc-qbuf.html` | Validated `VIDIOC_QBUF`/`VIDIOC_DQBUF`, incoming/outgoing queue behavior, buffer locking, `O_NONBLOCK`/`EAGAIN`, `V4L2_BUF_FLAG_ERROR`, DMABUF fd behavior, multi-planar requirements, and request API caveats. |
| `https://docs.kernel.org/5.10/userspace-api/media/v4l/vidioc-streamon.html` | Validated `VIDIOC_STREAMON`/`VIDIOC_STREAMOFF`, stream-type argument, DMA abort/finish behavior, queue clearing on streamoff, and restart semantics. |
| `https://docs.kernel.org/6.18/process/debugging/media_specific_debugging_guide.html` | Validated media-subsystem debugging workflow: `dev_debug`, `dev_dbg()`/`v4l2_dbg()`, dynamic debug, ftrace/debugfs, `v4l2-compliance`, media-topology compliance options, and `v4l2-ctl --log-status`. |
| `https://man.archlinux.org/man/extra/v4l-utils/v4l2-ctl.1.en` | Validated current `v4l2-ctl` purpose as a V4L2 control/query tool and common device selection, version, and help behavior. |
| `https://man.archlinux.org/man/extra/v4l-utils/v4l2-compliance.1.en` | Validated current `v4l2-compliance` purpose, device/media-device options, streaming tests, verbose/trace options, and the recommendation to use a recent v4l-utils build when testing drivers. |

The Chapter 34 example includes a build-checked userspace C program and no kernel module. It was compile-checked locally with `gcc -Wall -Wextra -O2`; no target-device streaming run is claimed here.
