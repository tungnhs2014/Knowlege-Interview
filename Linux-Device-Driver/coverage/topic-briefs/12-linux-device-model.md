# Topic Brief - 12 - Linux Device Model

## Output Targets
- Knowledge: `knowledge/12-linux-device-model.md`
- Interview: `interview/12-linux-device-model.md`
- Example: `examples/12-linux-device-model/README.md`

## Learning Path Metadata
- Learning path number: 12
- Title: Linux Device Model
- Slug: `linux-device-model`
- Primary scope: `struct device`, `struct device_driver`, `struct bus_type`, `struct class`, sysfs, uevents, kobjects, ksets, attributes, binding, and device lifetime.
- Related topics:
  - `07-character-device-drivers`: uses `class_create()` and `device_create()` to expose `/dev` nodes.
  - `08-userspace-abi-design-for-drivers`: owns detailed sysfs ABI design guidance.
  - `09-platform-bus-and-platform-drivers`: concrete bus/device/driver matching and probe/remove.
  - `10-device-tree-fundamentals` and `11-device-tree-apis-and-driver-integration`: firmware nodes and OF matching.
  - `20-kernel-memory-management-for-drivers`: devres and managed resource lifetime.
  - `24-power-management`: `struct dev_pm_ops`, wakeup attributes, and PM ordering.

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch13` | `docs/Linux Device Driver Development/Chapter 13-The Linux Device Model.md` | read/covered/primary | Primary source for LDM goals, bus/device/driver/class model, custom bus example, `device_register()`, `driver_register()`, binding, kobject/kset/kobj_type, sysfs attributes, attribute groups, device/bus/driver/class attributes, and `sysfs_notify()`. |
| `ldd1-ch04` | `docs/Linux Device Driver Development/Chapter 4-Character Device Drivers.md` | read/merged/supporting | Shows practical `class_create()` and `device_create()` flow for char devices, `/sys/class/<class>` visibility, `/dev` node creation, and cleanup relation to `cdev`. |
| `ldd1-ch05` | `docs/Linux Device Driver Development/Chapter 5-Platform Device Drivers.md` | read/merged/supporting | Explains bus matching loop, platform bus as a device-model bus, `platform_driver_register()`, `platform_match()`, OF/ACPI/ID/name matching, `MODULE_DEVICE_TABLE()`, and probe/remove distinction from module init/exit. |
| `ldd1-ch11` | `docs/Linux Device Driver Development/Chapter 11-Kernel Memory Management.md` | read/merged/supporting | Adds devres lifetime: resources are associated with `struct device` and released on device detach or driver unload. |
| `ldd2-ch10` | `docs/Linux Device Driver Development 2/Chapter 10-Linux_Kernel_Power_Management.md` | read/merged/supporting | Adds current driver-model relevance for PM: `struct device` parent/bus/driver/power/pm_domain fields, `struct dev_pm_ops`, PM callback ordering, power-domain precedence, wakeup source sysfs attributes. |
| `notion-ch03-part1` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 1 Data Structures & Synchronization.md` | read/merged/supporting | Clarifies `container_of()` mental model used by device model wrappers such as bus-specific device/driver structures and sysfs attribute wrappers. |
| `notion-ch04-part1` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 1 Character Device Registration.md` | read/merged/supporting | Independent Notion explanation of class/device creation, udev interaction, `/sys/class/.../dev`, `IS_ERR()`/`PTR_ERR()` cleanup, and automatic `/dev` node creation. |
| `notion-ch04-part2` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 2 Basic File Operations.md` | read/merged/supporting | Apparent duplicate char-device example read separately; reinforces `struct class *`, `struct device *`, error unwind order, and device-node cleanup. |
| `notion-ch04-part3` | `docs/Linux-Device-Driver-Notion/Chapter 4-Part 3 Read, Write, and Seek Operations.md` | read/merged/supporting | Apparent duplicate EEPROM-style example read separately; confirms repeated class/device creation and cleanup pattern in a fuller file-operations example. |
| `notion-ch05-part1` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 1 Platform Bus & Driver Basics.md` | read/merged/supporting | Independent Notion platform-bus explanation with driver/device lists, pseudo-bus mental model, match/probe flow, sysfs checks under `/sys/bus/platform`, and multi-device/one-driver flow. |
| `notion-ch05-part2` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 2 Probe, Remove & Resource Management.md` | read/merged/supporting | Adds probe workflow, resource extraction, `platform_set_drvdata()`, `devm_*` cleanup, and how probe owns per-device initialization. |
| `notion-ch05-part3` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 3 Device Provisioning & Integration.md` | read/merged/supporting | Adds runtime platform-device creation, release callback requirement, platform data, resource-bearing devices, and best-practice warnings. |
| `notion-ch06-part3` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 3 OF APIs and Platform Integration.md` | read/merged/supporting | Connects `struct device_node`, embedded kobject, `pdev->dev.of_node`, OF match tables, `of_match_device()`, `MODULE_DEVICE_TABLE(of, ...)`, and DT-backed platform integration. |

## Source Files Read
- `ldd1-ch13`: full chapter. Relevant sections: `LDM data structures`, `The bus`, `Bus registration`, `The device driver`, `Device driver registration`, `The device`, `Device registration`, `Deep inside LDM`, `kobject structure`, `kobj_type`, `ksets`, `Attributes`, `The attributes group`, `The device model and sysfs`, `Sysfs files and attributes`, `Device attributes`, `Bus attributes`, `Device driver attributes`, `Class attributes`, `Allowing sysfs attribute files to be pollable`.
- `ldd1-ch04`: character-device registration path around `alloc_chrdev_region()`, `class_create()`, `cdev_init()`, `cdev_add()`, `device_create()`, and cleanup summary.
- `ldd1-ch05`: platform-driver basics, device provisioning, bus matching, `platform_match()`, OF/ACPI/ID/name matching, ID-table data, and name-matching fallback.
- `ldd1-ch11`: `Device-managed resources - Devres`.
- `ldd2-ch10`: `Adding power management capabilities to device drivers`, `The concept of power domain`, `Being a source of system wakeup`, wakeup source sysfs notes.
- `notion-ch03-part1`: `Understanding container_of Macro`, real driver callback pattern.
- `notion-ch04-part1`: `Device Class and Automatic Device Node Creation`, `Error Handling Best Practices`.
- `notion-ch04-part2`: `Complete Basic Driver Example`.
- `notion-ch04-part3`: `Complete Example: EEPROM-like Device`.
- `notion-ch05-part1`: platform bus introduction, `struct platform_driver`, matching, registration, `platform_driver_probe()`, platform device creation, step-by-step bind/remove flow.
- `notion-ch05-part2`: probe workflow, resources, `devm_*`, platform data.
- `notion-ch05-part3`: runtime device creation, resource-bearing platform devices, release callback, best practices.
- `notion-ch06-part3`: `struct device_node`, platform driver with OF match, `of_match_device()`, DT best practices.

## Merged Source Notes
- The device model exists to give the kernel a common topology for devices, drivers, buses, classes, sysfs exposure, reference-counted lifetime, power-management ordering, and subsystem reuse.
- `ldd1-ch13` is the conceptual spine. It explains the generic objects and shows how a bus-specific type embeds generic device-model types:
  - bus-specific device embeds `struct device`;
  - bus-specific driver embeds `struct device_driver`;
  - helper macros use `container_of()` to recover the enclosing type;
  - wrapper registration functions set `.bus`, `.parent`, and call core helpers such as `device_register()` and `driver_register()`.
- Bus matching is a core mechanism, not a platform-only detail. `ldd1-ch05` and `notion-ch05-part1` provide concrete platform examples: registering either a device or a driver triggers the bus matching loop; if a match succeeds, probe runs; remove runs on unbind, device removal, or driver unload.
- Device classes are a functional grouping, not a physical topology. `ldd1-ch13` treats class as an LDM concept and `ldd1-ch04`/Notion chapter 4 show the everyday char-device use: `class_create()` creates a class under `/sys/class`, and `device_create()` creates a `struct device` plus a `dev` attribute that userspace can use to create `/dev/<name>`.
- Sysfs is the visible shape of kobjects. `ldd1-ch13` explains the hierarchy and low-level APIs; current kernel docs validate that sysfs attributes are backed by kobjects, should expose one value or an array of similar values, and should use `sysfs_emit()`/`sysfs_emit_at()` from `show()` methods.
- Attribute groups should be preferred for default or early attributes. `ldd1-ch13` introduces groups as easier management; current kernel docs add the important timing caveat that device attributes needed at `KOBJ_ADD` time should be defined in attribute groups before the add uevent is generated.
- `ldd1-ch11`, `notion-ch05-part2`, and `notion-ch06-part3` connect `struct device` to managed resources: `devm_*` allocations attach cleanup actions to the device and unwind automatically on detach, remove, or probe failure.
- `ldd2-ch10` extends the device model into PM: every device can be related to a parent, bus, driver, class/subsystem, power domain, and `dev_pm_info`. PM callbacks may come from device drivers, classes, buses, types, or power domains; power-domain callbacks can take precedence where present.
- Notion files that appear duplicative of book examples were read separately. They contribute beginner-friendly language, explicit test commands, and clearer error-unwind examples, but their core APIs often mirror the older book style.

## Source Differences
- `ldd1-ch13` says the goal is to build a complete "DT" mapping physical devices. In this context it means a device topology/tree, not Device Tree source (`.dts`). Learner-facing docs should avoid that ambiguity.
- `ldd1-ch13` focuses on a custom bus implementation. `ldd1-ch05` and Notion platform chapters show the same generic model through the platform bus, which is more immediately useful for embedded developers.
- `ldd1-ch13` uses older structure snapshots:
  - `struct bus_type` examples include fields such as `dev_attrs` and non-const callback signatures that have evolved.
  - `struct device_driver`, `struct device`, `struct class_attribute`, and sysfs internals should be described conceptually unless validated against current headers.
- `ldd1-ch04` and all Notion chapter 4 examples use `class_create(THIS_MODULE, name)`. Current kernel documentation shows `class_create(const char *name)`. Learner docs should include a kernel-version note and avoid presenting the old signature as universally current.
- `ldd1-ch13` examples use `sprintf()` or `scnprintf()` in sysfs `show()` callbacks. Current sysfs documentation says `show()` should use `sysfs_emit()` or `sysfs_emit_at()`.
- `ldd1-ch05` describes udev/mdev autoloading during matching in broad terms. For learner docs, phrase carefully: `MODULE_DEVICE_TABLE()` creates module aliases used by userspace module loading policy; the exact hotplug/autoload path depends on kernel/userspace configuration.
- Notion chapter 5 is clearer about platform bus as a pseudo-bus but simplifies the matching order. `ldd1-ch05` has a more precise platform matching order: driver override, OF, ACPI, ID table, then name fallback.
- `notion-ch06-part3` includes `struct device_node` with a visible `struct kobject kobj`; this helps connect DT nodes to sysfs/object lifetime, but field names and visibility can drift across kernels. Use it as mental model, not as a stable ABI.

## Gaps / Uncertainties
- Internal sources do not provide a modern, concise lifecycle diagram for `device_initialize()` plus `device_add()` versus `device_register()`, or `device_del()` plus `put_device()` versus `device_unregister()`.
- Internal sources mention reference counting but do not deeply explain `get_device()`, `put_device()`, `kobject_get()`, `kobject_put()`, or release-callback ownership rules. This needs current kernel-doc validation before learner-facing code.
- Internal sources do not clearly explain uevent contents, `KOBJ_ADD`, `KOBJ_REMOVE`, modalias, and the exact relationship between kernel uevents, devtmpfs, udev/mdev, and `/dev` node creation.
- Sysfs ABI rules need more validation before final learner docs: one value per file, text ABI, permissions, `store()` parsing, return counts, locking, and notification semantics.
- Driver core locking and iterator reference rules are only lightly covered. For senior interviews, validate `bus_for_each_dev()` and `class_for_each_device()` lifetime/reference details against current docs.
- No internal source provides a small self-contained custom bus example that is clearly safe for a modern kernel. Example output should probably use a char-device class/device example or a platform-device observation workflow, not a full custom bus.

## External Validation
- Used: `https://docs.kernel.org/driver-api/driver-model/bus.html`
  - Validates bus registration, bus match callback purpose, bus device/driver lists, `bus_for_each_dev()`, `bus_for_each_drv()`, and `/sys/bus/<bus>/devices` plus `/sys/bus/<bus>/drivers`.
- Used: `https://docs.kernel.org/driver-api/infrastructure.html`
  - Validates current `device_create(const struct class *class, ...)`, `device_destroy()`, current `class_create(const char *name)`, `class_destroy()`, class iterators, and basic driver-model infrastructure.
- Used: `https://docs.kernel.org/core-api/kobject.html`
  - Validates kobjects, ksets, ktypes, kset parentage, sysfs directory relationship, and uevent concepts.
- Used: `https://docs.kernel.org/6.3/filesystems/sysfs.html`
  - Validates sysfs purpose, attribute behavior, one-value guideline, PAGE_SIZE buffer behavior, `show()`/`store()` expectations, and modern `sysfs_emit()` recommendation.
- Used: `https://docs.kernel.org/6.8/driver-api/driver-model/device.html`
  - Validates `struct device` registration, reference-counted removal, `get_device()`/`put_device()`, and the recommendation that attributes required at add time be supplied through attribute groups before `KOBJ_ADD`.
- Final coverage note:
  - Final review returned PASS, so `12-linux-device-model` is marked covered in the coverage index files.
  - Version-sensitive API claims were validated against official kernel documentation and the example was build-checked against local `6.8.0-124-generic` headers.
  - Uevent, devtmpfs, udev/mdev, PM, and deep userspace ABI behavior are intentionally scoped to this topic's device-model overview and deferred to the dedicated ABI, platform, Device Tree, memory, and power-management topics where deeper treatment belongs.

## Learning Content Brief

### Mental Model
- The Linux Device Model is the kernel's object graph for hardware and virtual devices.
- A device can be viewed in multiple ways:
  - physical topology: parent/child under `/sys/devices`;
  - bus membership: device and driver under `/sys/bus/<bus>`;
  - functional class: device under `/sys/class/<class>`;
  - userspace node: `/dev/<name>` for character/block devices when a device number is exposed.
- The core idea is: represent every device with `struct device`, bind it to a `struct device_driver` through a bus, expose selected state through sysfs, and manage lifetime through reference counts and release callbacks.

### Core Mechanism
- Buses own matching rules.
- Devices and drivers register with a bus.
- When either side appears, the bus match callback compares bus-specific IDs.
- If matched, the driver core binds the device and driver and runs probe.
- On removal/unbind/unload, remove runs, managed resources unwind, sysfs entries and links disappear, and final memory is released only when references reach zero.

### Important Structs / APIs
- Core objects:
  - `struct device`
  - `struct device_driver`
  - `struct bus_type`
  - `struct class`
  - `struct kobject`
  - `struct kobj_type`
  - `struct kset`
  - `struct attribute`
  - `struct attribute_group`
- Registration and lifetime:
  - `device_initialize()`, `device_add()`, `device_register()`
  - `device_del()`, `put_device()`, `device_unregister()`
  - `get_device()`, `put_device()`
  - `driver_register()`, `driver_unregister()`
  - `bus_register()`, `bus_unregister()`
  - `class_create()`, `class_destroy()`
  - `device_create()`, `device_create_with_groups()`, `device_destroy()`
- Sysfs and attributes:
  - `DEVICE_ATTR*`, `BUS_ATTR*`, `DRIVER_ATTR*`, `CLASS_ATTR*`
  - `device_create_file()`, `device_remove_file()`
  - `bus_create_file()`, `bus_remove_file()`
  - `driver_create_file()`, `driver_remove_file()`
  - `class_create_file()`, `class_remove_file()`
  - `sysfs_create_file()`, `sysfs_remove_file()`
  - `sysfs_create_group()`, `sysfs_remove_group()`
  - `sysfs_notify()`
  - `sysfs_emit()`, `sysfs_emit_at()`
- Platform/device-tree bridge:
  - `struct platform_device`
  - `struct platform_driver`
  - `platform_driver_register()`, `platform_driver_unregister()`
  - `platform_device_alloc()`, `platform_device_add()`, `platform_device_unregister()`
  - `platform_set_drvdata()`, `platform_get_drvdata()`
  - `struct of_device_id`, `of_match_device()`, `MODULE_DEVICE_TABLE(of, ...)`
- Managed resources:
  - `devm_kzalloc()`
  - `devm_ioremap_resource()`
  - `devm_request_irq()`
  - `devm_clk_get()`, `devm_regulator_get()`, `devm_gpiod_get()`

### Lifecycle / Data Flow
- Bus/controller setup:
  - bus controller exists as a device;
  - bus registers `struct bus_type`;
  - devices on that bus use the controller as parent.
- Device registration:
  - allocate bus-specific object;
  - initialize embedded `struct device`;
  - set `.parent`, `.bus`, `.release`, optional `.of_node`, `.groups`;
  - register with `device_register()` or `device_initialize()` plus `device_add()`.
- Driver registration:
  - fill bus-specific driver object with embedded `struct device_driver`;
  - set `.name`, `.bus`, `.of_match_table` or ID table, `.probe`, `.remove`, `.pm`;
  - register with driver core or bus wrapper.
- Binding:
  - bus match callback decides compatibility;
  - driver core binds device to driver;
  - probe allocates per-device state, maps resources, registers subsystem objects, creates ABI nodes if needed, and stores private data.
- Unbind/remove:
  - remove unregisters subsystem objects and stops hardware;
  - devres releases managed resources;
  - sysfs links/files are removed;
  - final free happens through release callback after references drain.

### Examples To Build Later
- Minimal char-device example focused only on `class_create()`, `device_create()`, `/sys/class/<class>/<device>/dev`, and cleanup order.
- Platform-device observation example:
  - register a simple platform device;
  - register a matching platform driver;
  - inspect `/sys/bus/platform/devices`, `/sys/bus/platform/drivers`, dmesg probe/remove order.
- Sysfs attribute example:
  - use `DEVICE_ATTR_RW()` or attribute groups;
  - implement `show()` with `sysfs_emit()`;
  - parse `store()` safely;
  - optionally demonstrate `sysfs_notify()` with userspace `poll()`.

### Common Bugs
- Missing `.release` callback for manually created `struct device` or `platform_device`.
- Treating `probe()` as module initialization; probe is per matched device, module init is per module load.
- Creating sysfs files after the `KOBJ_ADD` uevent when userspace expects them immediately.
- Returning wrong byte count from sysfs `store()` or overflowing the `show()` buffer.
- Creating multi-value or binary sysfs files instead of a stable text ABI.
- Forgetting `device_destroy()`/`class_destroy()` cleanup for char-device helper devices.
- Using stale `class_create(THIS_MODULE, name)` signature on kernels where the modern API is `class_create(name)`.
- Freeing per-device data while open file descriptors, sysfs callbacks, workqueues, or references can still reach it.
- Assuming `/sys/class` is physical topology; physical hierarchy is under `/sys/devices`.
- Assuming matching is always by name; bus-specific order may prefer OF, ACPI, ID table, or driver override.

### Debugging Notes
- Inspect topology:
  - `find /sys/devices -name '<device-name>*'`
  - `ls -l /sys/bus/<bus>/devices`
  - `ls -l /sys/bus/<bus>/drivers`
  - `ls -l /sys/class/<class>`
- Inspect char-device numbers:
  - `cat /sys/class/<class>/<device>/dev`
  - `ls -l /dev/<device>`
- Inspect module aliases:
  - `modinfo <module>`
  - check `/lib/modules/$(uname -r)/modules.alias`
- Trace binding:
  - use dmesg with `dev_info()`/`dev_err()`;
  - enable dynamic debug for driver-core or target driver if available;
  - check `uevent` files under sysfs device directories.
- PM/lifetime debugging:
  - inspect `/sys/devices/.../power/`;
  - for wakeup-capable devices, inspect wakeup attributes and debugfs wakeup source stats when enabled.

### Production Concerns
- Prefer subsystem/framework registration over raw custom kobjects where possible.
- Prefer attribute groups for default sysfs attributes and add-time visibility.
- Treat sysfs as ABI: stable names, stable units, simple text, documented semantics.
- Use `devm_*` for resources bound to device lifetime, but still unregister externally visible objects in the right order.
- Keep parent pointers accurate so PM ordering, sysfs topology, DMA constraints, and resource management behave correctly.
- Use `dev_*()` logging with the correct `struct device *` so logs identify the physical device instance.
- Keep locking explicit in sysfs callbacks and remove paths; sysfs callbacks can run concurrently with user operations.
- Validate API signatures against target kernel headers before writing final examples.

### Interview Angles
- Explain why the Linux Device Model was introduced and why `struct device` exists.
- Distinguish bus, class, device, driver, and subsystem.
- Explain physical topology versus functional class view in sysfs.
- Walk through what happens when a device registers before its driver, and when a driver registers before its device.
- Explain who performs matching and why matching is bus-specific.
- Explain `probe()` versus module init and `remove()` versus module exit.
- Explain why `container_of()` is central to the device model.
- Explain why every device needs a release path and why reference-counted lifetime matters.
- Explain how `device_create()` helps char drivers create `/dev` nodes and what userspace does with the `dev` attribute.
- Explain why sysfs is not a dumping ground for arbitrary data and what makes a good sysfs attribute.
- Explain how `devm_*` cleanup is tied to `struct device`.
- Explain how PM uses the device hierarchy and why parent/child relationships matter.
