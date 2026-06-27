# 01 - Linux Kernel And Driver Development Overview

## Learning Goal

After this chapter, you should understand where a Linux device driver fits, how the kernel finds and binds it to hardware, and how to approach driver work without treating it as register programming alone.

By the end, you should be able to:

- Explain the privilege and failure-domain differences between userspace and kernel space.
- Distinguish a **device**, **driver**, **module**, **bus**, and **subsystem**.
- Describe the high-level path from hardware discovery to `probe()`, runtime callbacks, and removal.
- Explain why a driver should normally join an existing kernel subsystem.
- Recognize common kernel patterns such as embedded structures, operations tables, per-device state, managed resources, and reference counting.
- Navigate the kernel source by purpose instead of memorizing directories.
- Plan a configure, build, deploy, test, and debug loop for one explicit target kernel.
- Identify lifetime, concurrency, ABI, cleanup, and versioning risks before writing substantial code.

This is an architectural overview. Detailed environment setup and build commands belong to topic 02, while module implementation and load/unload mechanics belong to topic 03.

## Why This Matters In Real Work

A device driver translates between hardware-specific behavior and a kernel contract. That translation must cooperate with resource management, concurrency, power management, userspace interfaces, and device lifetime.

A driver can compile, load, and even access registers while still being architecturally wrong. Typical failures include:

- Binding to no device because the match data is wrong.
- Publishing an interface before hardware initialization is complete.
- Using a private ioctl or sysfs layout when an established subsystem already defines the ABI.
- Leaking resources after partial probe failure or leaving callbacks active after removal.
- Building against headers or configuration that do not match the running target.

The work therefore has two equally important sides:

| Hardware side | Kernel integration side |
| --- | --- |
| Registers, bit fields, timing, interrupts, DMA, reset sequences | Device matching, subsystem APIs, lifetime, locking, error paths, ABI, power, review |

Experienced driver developers spend substantial time reading documentation, framework code, neighboring drivers, and history before writing hardware-specific code. That investigation is part of implementation.

## Mental Model

Think of Linux hardware support as four cooperating layers:

```text
userspace applications and services
        |
        | system calls and stable userspace interfaces
        v
kernel subsystem or framework
        |
        | common object model, policy, and callbacks
        v
device driver
        |
        | register access, transfers, IRQs, DMA, power sequencing
        v
hardware
```

- **Applications** ask for useful operations: read input, send a packet, capture a frame, set a clock rate, or communicate with a sensor.
- **Kernel subsystems** define common behavior for a device family. Examples include networking, input, IIO, RTC, V4L2, ALSA/ASoC, GPIO, and watchdog.
- **Drivers** adapt one hardware implementation to the chosen subsystem and bus contracts.
- **Hardware** supplies registers, interrupts, memory, DMA engines, clocks, resets, and electrical behavior.

Not every driver directly serves userspace. A clock, regulator, GPIO controller, interrupt controller, or bus-controller driver may primarily serve other kernel drivers. Some drivers serve both kernel consumers and userspace through a subsystem.

### Userspace And Kernel Space

Userspace and kernel space are protection domains, not scheduler priorities.

| Property | Userspace process | Kernel/driver code |
| --- | --- | --- |
| Privilege | Restricted | Privileged |
| Address access | Its mapped process memory | Kernel memory and controlled access to userspace memory |
| Hardware access | Normally mediated by kernel interfaces | May access mapped device resources through kernel APIs |
| Failure impact | Usually one process | May corrupt state, hang, or crash the whole system |
| Available runtime | libc and application libraries | Kernel APIs only; no normal libc |

Applications enter the kernel through controlled interfaces such as system calls. The real route may pass through the VFS, a device class, a subsystem core, security checks, compatibility code, and architecture-specific entry code before a driver callback runs.

**Production rule:** kernel privilege does not make arbitrary userspace pointers safe. Driver code must use the relevant uaccess helpers, such as `copy_from_user()` and `copy_to_user()`, and obey their context and faulting rules.

## Core Concepts

The important nouns overlap in everyday speech, but they are not interchangeable.

| Term | Meaning | Example |
| --- | --- | --- |
| Device | One hardware or virtual instance | One I2C temperature sensor |
| Driver | Code capable of managing matching device instances | Sensor driver with `probe()` and subsystem callbacks |
| Bus | Matching and communication domain | Platform, I2C, SPI, PCI, USB |
| Subsystem/framework | Common model and API for a device function | IIO for many sensors |
| Module | A way to package and dynamically load kernel code | `foo_sensor.ko` |
| Userspace ABI | Interface applications depend on across kernel updates | Device node behavior, ioctl layout, documented sysfs attributes |

### Driver vs Module

A **driver is a role**; a **module is a loading mechanism**.

- A driver may be built into the kernel image or built as a loadable module.
- One module may contain one driver, several related drivers, or no hardware driver at all.
- Loading a module usually makes code available and registers a driver. It does not prove that compatible hardware exists or that `probe()` succeeded.

```text
module loaded
  -> driver registered
  -> matching device found?
       no: code remains registered, but no device is operational
       yes: kernel calls probe() for that device
```

### Bus vs Subsystem

A bus answers, "How is this device represented, matched, and communicated with?" A subsystem answers, "What kind of function does this device provide?"

For example, an accelerometer may be:

- represented as an I2C client;
- matched and transferred through the I2C core;
- exposed as an IIO device because it measures physical quantities.

The same functional subsystem can contain devices on different buses. Conversely, one bus carries devices belonging to many functional subsystems.

### Built-In vs Loadable

| Question | Built-in code | Loadable module |
| --- | --- | --- |
| Availability | Present from boot | Available after loading |
| Unload | No | Often possible when unused and configured |
| Typical reason | Needed early or always | Optional hardware, deployment policy, development |
| Update | Usually replace kernel image | May replace a compatible `.ko` |
| Main tradeoff | Fixed in kernel image | Loading, dependency, signing, and compatibility requirements |

### Internal API vs Userspace ABI

Linux makes an important distinction:

- Internal kernel APIs and structures can change between kernel releases.
- Established userspace ABI is expected to remain compatible once published.

This has practical consequences:

- Build and test an out-of-tree driver against every supported target kernel.
- Do not assume a source-compatible internal API across arbitrary versions.
- Do not expose internal implementation details casually through ioctl, sysfs, netlink, or device-node behavior.

## Kernel Mechanism

The exact structures vary by bus and subsystem, but most hardware drivers follow the same architectural flow.

### 1. A Device Becomes Known

The kernel first needs a device instance. It may come from:

- enumeration by a discoverable bus such as PCI or USB;
- firmware description such as Device Tree or ACPI;
- creation by another driver, such as an MFD parent;
- static or legacy board code;
- a virtual kernel facility.

The resulting object normally contains or relates to a `struct device`, which carries identity, parentage, firmware information, driver binding, power state, and resource-management context.

### 2. A Driver Registers

Driver code registers with an appropriate bus or subsystem. A bus-specific driver structure commonly embeds a generic `struct device_driver` and supplies:

- match information;
- a per-device `probe()` callback;
- removal, shutdown, or power-management callbacks where required;
- subsystem-specific operations.

Registration means, "This code can handle devices with these identities." It is not per-device hardware initialization.

### 3. The Kernel Matches And Probes

When a device and compatible driver are present, the bus performs its matching rules. These may use:

- Device Tree `compatible` strings;
- ACPI IDs;
- PCI or USB vendor/device IDs;
- I2C/SPI identifiers or other bus-specific ID tables.

After a match, the kernel calls `probe()` for that device instance. A well-designed probe usually:

1. Validates device description and variant data.
2. Allocates one private state object for this device.
3. Acquires MMIO, IRQs, clocks, resets, regulators, GPIOs, DMA, or bus handles.
4. Initializes locks, queues, work items, and software state.
5. Places hardware into a known state.
6. Registers with the functional subsystem.
7. Publishes interfaces only when the instance is ready.
8. Returns `0`, or unwinds partial work and returns a meaningful negative errno.

If one driver matches three devices, `probe()` can run three times. Per-device state must therefore not be hidden in an accidental global singleton.

### 4. Runtime Callbacks Do The Work

After successful probe, activity reaches the driver through framework-defined paths:

```text
userspace request
  -> syscall / subsystem core
  -> driver operation callback
  -> hardware operation
  -> result returned through subsystem
```

or:

```text
hardware event
  -> interrupt handler
  -> acknowledge/minimal immediate work
  -> optional threaded or deferred processing
  -> subsystem notification or waiting task wakeup
```

Callbacks may run concurrently and in different execution contexts. Some can sleep; hard-IRQ callbacks cannot. The specific locking and context rules belong to topics 05, 06, and 15, but the design requirement starts here: **never assume context or serialization without checking the callback contract**.

### 5. Teardown Reverses Publication

Removal is more than freeing memory. The driver must prevent new users and stop all asynchronous activity before backing state disappears.

Typical order:

1. Stop or reject new operations.
2. Unregister user-visible or subsystem interfaces.
3. Disable hardware event generation.
4. Synchronize IRQ handlers and drain work, timers, DMA, or callbacks.
5. Put hardware into a safe state.
6. Release resources and references.
7. Allow per-device state to be freed.

Managed resources can automate many release calls, but they do not decide when callbacks are safe to stop. Correct teardown ordering remains the driver's responsibility.

## Key Structs And APIs

This overview names the common anchors. Their detailed fields and rules are taught with the owning bus or subsystem.

| Struct/API pattern | Role in the design |
| --- | --- |
| `struct device` | Generic identity, hierarchy, binding, PM, firmware, logging, and managed-resource anchor |
| `struct device_driver` | Generic driver registered with the driver core, usually embedded in a bus-specific driver |
| Bus-specific device | One instance, such as `struct platform_device`, `struct i2c_client`, `struct spi_device`, or `struct pci_dev` |
| Bus-specific driver | Match tables and callbacks, such as `struct platform_driver` or `struct pci_driver` |
| Driver-private structure | Per-device registers, locks, state, queues, and owned resources |
| Operations table | Function pointers implementing a kernel or subsystem contract |
| `struct module`, `THIS_MODULE` | Ownership and module-lifetime concepts for loadable code |
| `struct kobject` / reference count | Lifetime and sysfs machinery used by specific object models |

### Per-Device State

Drivers commonly allocate a structure such as:

```c
struct demo_device {
        struct device *dev;
        void __iomem *regs;
        int irq;
        struct mutex lock;
        bool running;
};
```

The exact members depend on the driver, but the design questions are stable:

- Who allocates and frees this object?
- Which callbacks can access it?
- What lock protects each mutable field?
- Can an IRQ, worker, timer, or userspace file outlive device removal?
- Which object owns references to other kernel objects?

The state is usually attached to the bus device through helpers such as `dev_set_drvdata()` or a bus-specific equivalent, then recovered in callbacks.

### Operations Tables

Kernel C code often represents an interface as a structure of function pointers:

```c
static const struct demo_ops demo_hw_ops = {
        .start = demo_start,
        .stop = demo_stop,
        .read_sample = demo_read_sample,
};
```

This gives the core a stable contract while hardware implementations provide different behavior. You will see this pattern in `file_operations`, network operations, clock operations, GPIO chips, V4L2 operations, and many other frameworks.

**Lifetime rule:** any published operations table or callback path can lead back into driver state. Unregister the interface and drain in-flight work before freeing that state or unloading its code.

### Managed Resources

Device-managed helpers, commonly prefixed `devm_`, attach cleanup actions to a device. Typical examples include managed allocation, register mapping, and IRQ requests.

They are useful because:

- probe error paths become shorter;
- cleanup is tied to device detach;

They are not universal:

- some resources need early release;
- callback shutdown may need to happen before automatic cleanup;
- non-device lifetimes or subsystem conventions may require explicit ownership.

Use managed resources when their lifetime matches the device, not as a substitute for understanding ownership.

### Source-Tree Landmarks

Navigate by question:

| Need | Start here |
| --- | --- |
| Existing drivers and subsystem implementations | `drivers/` and relevant subsystem directories |
| Internal kernel declarations | `include/linux/` |
| Headers exported to userspace | `include/uapi/` |
| Architecture-specific implementation | `arch/` |
| Current API, process, and subsystem guidance | `Documentation/` |
| Build infrastructure and developer tools | `scripts/` |
| Configuration declarations | `Kconfig` files |
| Build composition | `Makefile` and `Kbuild` files |
| Maintainers and patch routing | `MAINTAINERS` |

Do not begin by reading the entire tree. Find the owning subsystem, read its documentation, inspect a few maintained drivers for similar hardware, then trace only the relevant helpers into core code.

## Lifecycle / Data Flow

The full lifecycle is broader than module insertion:

```text
select target kernel, hardware, configuration, and toolchain
  -> inspect subsystem documentation and neighboring drivers
  -> describe or enumerate the device
  -> configure and build matching kernel artifacts
  -> deploy kernel image, DTB/firmware data, and modules as applicable
  -> boot or load code
  -> register driver
  -> bus matches device and driver
  -> probe one device instance
       allocate state
       acquire resources
       initialize hardware
       register subsystem object
       publish interfaces
  -> runtime callbacks, IRQs, work, PM, and userspace activity
  -> suspend/resume, unbind, device removal, shutdown, or module unload
       stop new activity
       unregister interfaces
       drain callbacks
       release resources
```

| Event | What success proves |
| --- | --- |
| Compilation | Source is acceptable to that build environment |
| Module load / built-in init | Code loaded and top-level registration succeeded |
| Driver registration | Driver is available for matching |
| Device match | Identity rules selected this driver |
| Probe success | One device instance initialized successfully |
| Interface visible | A subsystem or userspace-facing object was published |
| Functional test | Selected behavior worked under selected conditions |
| Teardown test | Lifetime and cleanup survived removal or failure |

One successful `insmod` or boot proves very little about the later stages.

## Minimal Practical Example

This is **learning-only pseudo-code**. It shows relationships and ordering, not a compilable driver. Real registration types, resources, callbacks, and cleanup rules depend on the bus and subsystem.

```c
struct demo_state {
        struct device *dev;
        void __iomem *regs;
        int irq;
        bool stopping;
};

static int demo_probe(struct bus_device *bdev)
{
        struct device *dev = bus_device_to_dev(bdev);
        struct demo_state *st;
        int ret;

        st = devm_kzalloc(dev, sizeof(*st), GFP_KERNEL);
        if (!st)
                return -ENOMEM;

        st->dev = dev;
        bus_set_drvdata(bdev, st);

        st->regs = demo_map_registers(dev);
        if (IS_ERR(st->regs))
                return PTR_ERR(st->regs);

        st->irq = demo_get_irq(dev);
        if (st->irq < 0)
                return st->irq;

        ret = demo_initialize_hardware(st);
        if (ret)
                goto err_stop_hardware;

        ret = demo_request_irq(st);
        if (ret)
                goto err_stop_hardware;

        ret = demo_register_with_subsystem(st);
        if (ret)
                goto err_stop_events;

        return 0;

err_stop_events:
        demo_disable_and_synchronize_irq(st);
err_stop_hardware:
        demo_stop_hardware(st);
        return ret;
}

static void demo_remove(struct bus_device *bdev)
{
        struct demo_state *st = bus_get_drvdata(bdev);

        st->stopping = true;
        demo_unregister_from_subsystem(st);
        demo_disable_and_synchronize_irq(st);
        demo_cancel_all_work(st);
        demo_stop_hardware(st);
        /* Device-managed resources are released after detach. */
}

static struct bus_driver demo_driver = {
        .name = "demo",
        .match_table = demo_matches,
        .probe = demo_probe,
        .remove = demo_remove,
};
```

- `demo_state` belongs to one device instance.
- Probe acquires and publishes in a deliberate order.
- Interfaces are published only after required initialization succeeds.
- Remove unpublishes and drains asynchronous users before state disappears.
- Managed allocation reduces release calls, but explicit callback shutdown is still required.
- Driver registration and module boilerplate are intentionally omitted; topics 02 and 03 cover those mechanics.

## Common Bugs And Debugging

Start by identifying which lifecycle stage failed. Randomly adding register prints is inefficient when the device never matched.

| Symptom | Likely area | Evidence to inspect |
| --- | --- | --- |
| Driver does not compile | Source/API/build mismatch | First compiler error, target headers, `.config`, Kbuild selection |
| New binary has no effect | Deployment mismatch | Running release, booted image, DTB, module path and timestamp |
| Module loads but no `probe()` log | Registration, enumeration, or matching | Bus device list, driver directory, modalias, firmware identity |
| Probe repeatedly defers | Supplier dependency | Kernel log, deferred-probe reason, clocks/regulators/GPIO/firmware providers |
| Probe fails after partial setup | Resource, ordering, or hardware issue | First negative errno and first device-scoped error |
| Runtime fails under load | Race, context, IRQ, DMA, or lifetime issue | Warnings, lock diagnostics, traces, subsystem statistics |
| Unbind/reload crashes | Teardown ordering or stale callback | First oops, use-after-free report, pending IRQ/work/timer |
| Userspace breaks after update | ABI change | Interface definitions, structure sizes, attribute semantics, compatibility path |

### A Practical Debugging Funnel

1. **Build:** Did the expected source and configuration produce the artifact?
2. **Deploy:** Is the target actually running that image, DTB, and module set?
3. **Enumerate:** Does the kernel know the device exists?
4. **Register:** Is the driver registered on the expected bus?
5. **Match:** Do IDs, names, or firmware compatible strings agree?
6. **Probe:** What is the first failing acquisition or initialization step?
7. **Publish:** Did the subsystem object or userspace interface appear?
8. **Operate:** Which request or hardware event triggers failure?
9. **Teardown:** Can failure injection, unbind, suspend, and reload complete safely?

Useful first checks:

```bash
uname -r
cat /proc/version
dmesg -w
ls /sys/bus
find /sys/bus/<bus>/devices -maxdepth 1 -type l
find /sys/bus/<bus>/drivers -maxdepth 2
```

The exact bus and subsystem tools vary. Later topics add dynamic debug, ftrace, tracepoints, debugfs, lock diagnostics, and specialized utilities.

Debugging rules:

- Preserve the **first** warning, error, or oops and its full call trace.
- Log with device context when a `struct device *` is available.
- Return the original useful errno instead of converting every failure to `-EINVAL`.
- Confirm matching and resources before blaming register access.
- Treat compilation, checkpatch, and one successful boot as evidence, not proof.

## Development And Coding Practice

Kernel style is a shared engineering language. Its value is faster review and easier reasoning about ownership, control flow, and error paths.

Practical habits:

- Follow the target tree's `Documentation/process/coding-style.rst` and subsystem conventions.
- Use tabs for indentation as defined by kernel style; keep nesting shallow.
- Keep implementation-only symbols `static`.
- Comment hardware errata, invariants, ordering constraints, and non-obvious reasons rather than narrating syntax.
- Use kernel-doc for interfaces that need structured documentation.
- Keep cleanup symmetrical and in reverse acquisition order.
- Run `scripts/checkpatch.pl` and warning-enabled builds, but review the design yourself.

Current kernel style prefers readable lines around 80 columns but does not require harmful wrapping. It also discourages many structure typedefs without banning every typedef. Copy the current documentation, not rigid folklore from an old book.

### The High-Level Development Loop

```text
choose one explicit kernel revision and target
  -> start from a known configuration
  -> identify bus and functional subsystem
  -> read docs, maintained examples, and relevant history
  -> make a small change
  -> configure and build reproducibly
  -> deploy matching artifacts
  -> test registration, matching, probe, function, failure, and teardown
  -> inspect logs/traces and revise
  -> prepare a focused reviewable patch
```

## Production Checklist

Before considering a driver ready for serious review or product integration, verify:

- [ ] The supported kernel revisions, configurations, architectures, and hardware variants are explicit.
- [ ] The driver uses the correct bus and functional subsystem rather than creating an unnecessary private interface.
- [ ] Match tables and firmware descriptions identify every supported variant accurately.
- [ ] Per-device state has clear ownership; mutable fields have documented synchronization.
- [ ] Probe publishes no interface before required initialization succeeds.
- [ ] Every partial failure returns a meaningful errno and unwinds completed work.
- [ ] Removal stops new operations and drains IRQs, work, timers, DMA, and outstanding callbacks.
- [ ] Managed-resource ordering matches the real callback and hardware lifetime.
- [ ] Userspace pointers are handled only through approved uaccess APIs.
- [ ] Userspace ABI and firmware bindings are deliberate, documented, and compatible.
- [ ] Suspend/resume, shutdown, unbind, reload, and hardware-error behavior are considered where applicable.
- [ ] Logs identify the device and failure without flooding healthy or hot paths.
- [ ] The running image, DTB, configuration, and modules are from the tested build.
- [ ] Warning-enabled builds, checkpatch, and relevant subsystem tests have been run.
- [ ] Tests include missing resources, dependency deferral, partial probe failure, repeated bind/unbind, and concurrent activity.
- [ ] Licensing, SPDX tags, module-signing policy, and distribution obligations have been reviewed appropriately.
- [ ] `MAINTAINERS`, subsystem documentation, and recent accepted patches were checked before submission.

## Interview Readiness

You should be able to explain these ideas as connected reasoning rather than definitions.

### Beginner

- Userspace is restricted; kernel code is privileged and has a system-wide failure domain.
- A driver controls hardware through kernel contracts.
- A module is a packaging/loading choice, not a synonym for driver.
- Loading code and successfully probing a device are separate events.

### Mid-Level

- Describe discovery, driver registration, matching, probe, runtime callbacks, and removal.
- Explain why one private state object is normally needed per device.
- Compare bus responsibilities with functional subsystem responsibilities.
- Explain why probe error paths and remove paths must be designed together.
- Separate internal kernel API compatibility from userspace ABI compatibility.

### Senior

- Explain how interface publication, asynchronous work, reference ownership, and teardown ordering interact.
- Defend a subsystem choice and identify the long-term cost of a private ABI.
- Diagnose a module that loads but never probes without beginning at register access.
- Explain why `devm_*`, reference counting, checkpatch, or successful compilation cannot independently prove correctness.

Typical traps:

- "The module loaded, so the hardware initialized."
- "Kernel mode can dereference any userspace pointer."
- "All callbacks can sleep because probe can sleep."
- "A reference count removes the need for locking."
- "Every driver should create its own `/dev` node."
- "Internal driver APIs are stable across Linux versions."

For focused questions and answer structure, continue with `interview/01-linux-kernel-and-driver-development-overview.md`.

## Kernel Version Notes

- Internal kernel APIs, callback signatures, helper availability, Kconfig symbols, and structure fields change. Use the source and documentation shipped with the target kernel as the authority.
- Kernel version numbers are not semantic-version compatibility promises for out-of-tree drivers. Rebuild and validate for each supported kernel/configuration combination.
- Mainline, stable, and longterm are release-maintenance categories; longterm support dates and vendor commitments can change. Check current kernel.org and vendor policy when selecting a product baseline.
- Older material may assume fixed 32-bit userspace/kernel address splits, architecture-specific syscall entry, rigid 80-column rules, legacy board files, or old callback signatures. Treat these as historical examples unless confirmed in the target tree.
- C remains the dominant language for drivers, while current kernels also contain supported Rust infrastructure. Follow the language and subsystem support present in the selected target tree.
