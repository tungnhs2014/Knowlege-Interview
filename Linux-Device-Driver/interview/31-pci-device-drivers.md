# 31 - PCI Device Drivers Interview Questions

Strong PCI driver candidates can reason from bus discovery to `probe()`, BAR mapping, DMA, interrupts, and safe teardown. The best answers do not stop at "call this API"; they explain ownership, ordering, and what evidence they would inspect during bring-up.

## Beginner Questions

### 1. What is a PCI device driver?

**Level:** Beginner

**Short Answer:**  
A PCI device driver is a Linux driver that binds to a PCI/PCIe function discovered by the PCI core and controls its device-specific registers, interrupts, DMA, and subsystem integration.

**Deep Explanation:**  
The PCI core enumerates the bus, reads each function's configuration space, creates a `struct pci_dev`, and matches it against registered PCI drivers. The driver supplies an ID table and callbacks such as `probe()` and `remove()`. Once matched, the driver enables the device, claims BAR resources, maps registers, configures DMA and interrupts, and registers the device with an upper subsystem such as networking, block, DRM, V4L2, ALSA, or a custom interface.

**API / Code Anchor:**  
`struct pci_driver`, `struct pci_device_id`, `module_pci_driver()`, `pci_register_driver()`, `probe()`, `remove()`.

**Production or Debugging Angle:**  
In production, the PCI part is usually just the lower bus layer. A NIC driver, for example, is both a PCI driver and a network driver.

**Common Traps:**  
- Thinking the driver manually scans the PCI bus.
- Confusing a PCI driver with a platform driver.
- Ignoring the upper subsystem that exposes the actual userspace ABI.

**Follow-up Questions:**  
- What creates `struct pci_dev`?
- What decides whether your driver probes a device?
- Can a single physical PCI card expose multiple functions?

### 2. How does Linux match a PCI device to a driver?

**Level:** Beginner

**Short Answer:**  
Linux matches fields from the device's PCI configuration space against the driver's `struct pci_device_id` table.

**Deep Explanation:**  
Every PCI function has identity fields such as vendor ID, device ID, subsystem vendor/device IDs, revision, and class code. A PCI driver registers an ID table describing the devices or classes it supports. When the PCI core finds a match, it calls the driver's `probe()` callback with the `struct pci_dev` and matching ID entry.

`MODULE_DEVICE_TABLE(pci, ids)` exports the match table as module alias metadata. This allows userspace hotplug tools to autoload the module when a matching PCI device appears.

**API / Code Anchor:**  
`struct pci_device_id`, `PCI_DEVICE()`, `PCI_DEVICE_CLASS()`, `PCI_DEVICE_SUB()`, `MODULE_DEVICE_TABLE(pci, ids)`.

**Production or Debugging Angle:**  
If a device appears in `lspci` but the driver never loads automatically, check the modalias and `modinfo` aliases.

**Common Traps:**  
- Forgetting `MODULE_DEVICE_TABLE(pci, ids)`.
- Matching only vendor/device when the hardware needs subsystem-specific handling.
- Assuming a visible device in `lspci` means the driver has bound.

**Follow-up Questions:**  
- How do you inspect a PCI modalias in sysfs?
- When would you match by class code instead of exact device ID?
- What happens if two drivers match the same device?

### 3. What is a BDF?

**Level:** Beginner

**Short Answer:**  
BDF means Bus:Device.Function, the address of a PCI function in the PCI hierarchy.

**Deep Explanation:**  
PCI devices are addressed by bus number, device number, and function number. Linux often displays the full address with a PCI domain, such as `0000:03:00.0`. The same physical card may have multiple functions, for example `03:00.0` and `03:00.1`.

BDF is central to debugging because `lspci`, sysfs, and kernel logs use it to identify the exact function.

**API / Code Anchor:**  
`struct pci_dev`, `/sys/bus/pci/devices/0000:bb:dd.f/`, `lspci -s bb:dd.f`.

**Production or Debugging Angle:**  
When debugging field logs, always ask for the BDF and `lspci -nn -vv -s <BDF>` output.

**Common Traps:**  
- Treating a PCI card as one device when it has multiple functions.
- Ignoring the PCI domain on systems with multiple host bridges.
- Using the wrong BDF when manually binding or inspecting sysfs.

**Follow-up Questions:**  
- Where does Linux expose PCI devices in sysfs?
- What does the `.0` in `03:00.0` mean?
- Why might the same hardware have different bus numbers after reboot?

### 4. What is the difference between PCI config space and BAR space?

**Level:** Beginner

**Short Answer:**  
Config space contains standard PCI identity and control fields. BAR space is the device's runtime register or memory window assigned through Base Address Registers.

**Deep Explanation:**  
Configuration space is accessed through PCI config APIs and contains fields such as vendor ID, device ID, class code, BAR registers, and capability lists. BARs describe windows that the CPU can use to access device-specific registers or memory. Once a BAR is assigned and mapped, the driver accesses it as MMIO or legacy I/O port space.

Do not use config-space APIs for normal data-path register access unless the hardware manual explicitly says the register is in config space.

**API / Code Anchor:**  
`pci_read_config_word()`, `pci_write_config_dword()`, `pci_resource_start()`, `pci_resource_len()`, `pci_iomap()`, `pcim_iomap_regions()`.

**Production or Debugging Angle:**  
Use `lspci -vv` to see BAR assignments and PCI capabilities. Use the driver's logs or sysfs `resource` file to confirm what was mapped.

**Common Traps:**  
- Treating BAR registers themselves as the device's runtime registers.
- Accessing MMIO with normal pointer dereferences.
- Mapping a BAR without checking flags or length.

**Follow-up Questions:**  
- How do you tell whether a BAR is MMIO or I/O port space?
- Why does PCIe still have config space?
- Which APIs should access mapped MMIO?

## Mid-Level Questions

### 5. Walk through a typical PCI `probe()` function.

**Level:** Mid

**Short Answer:**  
`probe()` should enable the device, set DMA masks, request/map BARs, enable bus mastering if DMA is used, allocate IRQ vectors, register IRQ handlers, initialize software state, register with the upper subsystem, and enable hardware operation last.

**Deep Explanation:**  
The key is ordering. The driver first obtains permission to use the device and resources. It then prepares software state so it can safely handle interrupts or DMA completions. Only after handlers, queues, and buffers are ready should it enable hardware interrupt generation or DMA engines.

Every successful step needs cleanup on later failure. This is especially important for PCI because the device may DMA into memory or interrupt asynchronously.

**API / Code Anchor:**  
`pcim_enable_device()` or `pci_enable_device()`, `dma_set_mask_and_coherent()`, `pci_set_master()`, `pcim_iomap_regions()`, `pci_alloc_irq_vectors()`, `pci_irq_vector()`, `request_irq()` / `devm_request_irq()`.

**Production or Debugging Angle:**  
If `probe()` fails halfway, a good driver unwinds only the resources it has acquired. Repeated bind/unbind is a useful test.

**Common Traps:**  
- Enabling hardware interrupts before requesting the IRQ.
- Allocating DMA memory before setting the DMA mask.
- Touching BAR registers before the BAR is requested and mapped.
- Returning an error without undoing manually acquired resources.

**Follow-up Questions:**  
- Where would you call `pci_set_drvdata()`?
- When should hardware DMA be enabled?
- What changes if you use managed helpers?

### 6. Why does a PCI DMA driver need both `dma_set_mask_and_coherent()` and `pci_set_master()`?

**Level:** Mid

**Short Answer:**  
`dma_set_mask_and_coherent()` tells the DMA layer what addresses the device can reach. `pci_set_master()` enables the PCI command bit that allows the device to initiate DMA transactions.

**Deep Explanation:**  
The DMA mask is a software and IOMMU constraint. It tells Linux whether the device can handle 64-bit DMA addresses, only 32-bit addresses, or something smaller. The DMA API uses this to allocate or map memory into an address range the device can actually use.

Bus mastering is a PCI control permission. Without it, the device may not be allowed to initiate reads or writes on the PCI bus even if the driver has valid DMA mappings.

**API / Code Anchor:**  
`dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64))`, fallback to `DMA_BIT_MASK(32)`, `pci_set_master(pdev)`.

**Production or Debugging Angle:**  
DMA failures often show up as IOMMU faults, corrupted buffers, or silent hardware timeouts. Check DMA mask setup before blaming the device.

**Common Traps:**  
- Giving hardware a CPU physical address instead of a DMA address.
- Assuming all PCIe devices support 64-bit DMA.
- Calling `pci_set_master()` but forgetting the DMA mask.
- Allocating coherent memory before setting the mask.

**Follow-up Questions:**  
- When would you use coherent DMA?
- When would you use streaming DMA?
- What evidence suggests a DMA mask problem?

### 7. Compare INTx, MSI, and MSI-X.

**Level:** Mid

**Short Answer:**  
INTx is legacy shared interrupt signaling. MSI uses message writes for interrupts. MSI-X extends MSI with more vectors and per-vector configuration, making it better for multi-queue devices.

**Deep Explanation:**  
INTx uses legacy interrupt pins and is often shared. The handler must check device status and return `IRQ_NONE` if the interrupt was not from this device.

MSI avoids shared legacy lines by having the device write a message to signal an interrupt. MSI-X supports more vectors and more flexible vector configuration, which helps NICs, NVMe controllers, and other high-throughput devices spread work across CPUs.

Modern drivers allocate vectors through one common API and let the kernel choose the best supported mode based on flags and hardware support.

**API / Code Anchor:**  
`pci_alloc_irq_vectors(pdev, min, max, PCI_IRQ_ALL_TYPES)`, `pci_irq_vector(pdev, n)`, `PCI_IRQ_INTX`, `PCI_IRQ_MSI`, `PCI_IRQ_MSIX`, `PCI_IRQ_AFFINITY`.

**Production or Debugging Angle:**  
Inspect `/proc/interrupts` and `lspci -vv` to see whether MSI/MSI-X is enabled and whether vectors are firing.

**Common Traps:**  
- Using old `pci_enable_msi()` or `pci_enable_msix_*()` APIs in new code.
- Using `pdev->irq` for MSI-X vectors.
- Assuming INTx is exclusive.
- Forgetting to handle fallback when MSI-X is unavailable.

**Follow-up Questions:**  
- Why is MSI-X useful for multi-queue networking?
- What should an INTx handler do on a shared interrupt?
- How do you get the Linux IRQ number for vector 3?

### 8. How should a PCI driver map and access BAR registers?

**Level:** Mid

**Short Answer:**  
It should inspect the BAR resource, request ownership, map it with PCI mapping helpers, and access it using MMIO or port I/O accessors according to the resource type.

**Deep Explanation:**  
A BAR is a window assigned by the PCI core or firmware. Before using it, the driver must verify the resource type and length. For MMIO BARs, it maps the BAR into kernel virtual address space and uses `ioread*()` / `iowrite*()` accessors. For legacy I/O port resources, it uses port I/O helpers instead.

Managed helpers can combine claiming and mapping, reducing cleanup code. The driver still needs to understand what has been managed and what must be manually released.

**API / Code Anchor:**  
`pci_resource_flags()`, `pci_resource_len()`, `pci_request_regions()`, `pci_iomap()`, `pci_ioremap_bar()`, `pcim_iomap_regions()`, `pcim_iomap_table()`, `ioread32()`, `iowrite32()`.

**Production or Debugging Angle:**  
If reads return all ones or zeros, check whether the device is enabled, the BAR index is correct, and the resource is mapped with the correct type.

**Common Traps:**  
- Using normal C pointer dereference on `__iomem`.
- Mapping an I/O port BAR as MMIO.
- Ignoring posted MMIO writes where a readback or barrier is needed.
- Not checking BAR length before accessing offsets.

**Follow-up Questions:**  
- What does `__iomem` communicate to the developer and sparse?
- Why are MMIO writes sometimes posted?
- How can `lspci -vv` help debug BAR assignment?

### 9. What should happen in `remove()` for an active PCI DMA device?

**Level:** Mid

**Short Answer:**  
The driver should stop new work, disable hardware interrupts, quiesce DMA, flush deferred work, free IRQs, unregister upper subsystem objects, free DMA resources, release BARs, and disable the PCI device if cleanup is manual.

**Deep Explanation:**  
Remove is not just the reverse of probe. The first priority is making the hardware stop touching memory or interrupting. If the driver frees private state before disabling interrupts or stopping DMA, the hardware or IRQ handler may use freed memory.

Subsystem lifetime matters too. For a network driver, queues and NAPI must be stopped. For a media driver, streaming must stop. For storage, outstanding commands must be completed or aborted.

**API / Code Anchor:**  
`free_irq()`, `devm_*` cleanup rules, `pci_free_irq_vectors()`, `pci_clear_master()`, `pci_disable_device()`, `dma_unmap_*()`, `dma_free_coherent()`.

**Production or Debugging Angle:**  
Stress test unbind/rebind and module unload/reload. Many teardown bugs only appear after repeated cycles.

**Common Traps:**  
- Freeing DMA buffers while the device is still bus mastering.
- Leaving workqueues or timers running.
- Freeing IRQ vectors while handlers can still access state.
- Double-freeing resources managed by devres or pcim helpers.

**Follow-up Questions:**  
- Why should hardware interrupt generation be disabled before freeing IRQs?
- How do managed helpers change `remove()`?
- What should be flushed before private state is freed?

## Debugging Scenarios

### 10. A PCI device appears in `lspci`, but your driver never probes. What do you check?

**Level:** Mid

**Short Answer:**  
Check ID-table matching, module aliases, driver binding, whether another driver owns the device, and dmesg for probe or autoload errors.

**Deep Explanation:**  
`lspci` proves the PCI core enumerated the device. It does not prove your module loaded or matched. The next layer is driver binding: compare the device's vendor/device/class IDs against the driver's ID table and modalias. Then check whether `MODULE_DEVICE_TABLE(pci, ids)` exported aliases and whether udev/modprobe loaded the module.

If the module is loaded but not bound, sysfs can show whether another driver owns the device or whether the ID table is wrong.

**API / Code Anchor:**  
`MODULE_DEVICE_TABLE(pci, ids)`, `struct pci_device_id`, `/sys/bus/pci/devices/.../modalias`, `/sys/bus/pci/drivers/.../bind`.

**Production or Debugging Angle:**  
Useful commands:

```sh
lspci -nn -k -s bb:dd.f
cat /sys/bus/pci/devices/0000:bb:dd.f/modalias
modinfo your_module
readlink /sys/bus/pci/devices/0000:bb:dd.f/driver
dmesg | tail -100
```

**Common Traps:**  
- Adding an ID to source code but forgetting to rebuild/install the module.
- Assuming manual `insmod` behavior proves autoloading works.
- Binding to the wrong function on a multi-function device.

**Follow-up Questions:**  
- What is a PCI modalias?
- How can you manually test binding through sysfs?
- Why might a class-code match be too broad?

### 11. Your IRQ handler never runs after probe succeeds. How do you debug it?

**Level:** Mid

**Short Answer:**  
Verify vector allocation, requested IRQ numbers, hardware interrupt enable bits, `/proc/interrupts`, MSI/MSI-X state in `lspci -vv`, and whether the device requires status acknowledgement.

**Deep Explanation:**  
Successful probe only proves the driver initialized. Interrupt delivery depends on both kernel-side vector setup and device-side register programming. The driver should allocate vectors, obtain the Linux IRQ with `pci_irq_vector()`, request the handler, and then enable interrupt generation in the hardware.

For INTx, the line may be shared, so the handler might run but return `IRQ_NONE`. For MSI/MSI-X, the device must be programmed to use the allocated vectors according to the hardware design.

**API / Code Anchor:**  
`pci_alloc_irq_vectors()`, `pci_irq_vector()`, `request_irq()`, `devm_request_irq()`, `/proc/interrupts`.

**Production or Debugging Angle:**  
Compare expected and actual interrupt mode:

```sh
cat /proc/interrupts
lspci -vv -s bb:dd.f
dmesg | grep -i -E 'irq|msi|msix|pci'
```

**Common Traps:**  
- Enabling device interrupts before the handler is registered.
- Requesting `pdev->irq` instead of `pci_irq_vector()` for MSI-X.
- Not acknowledging device interrupt status.
- Assuming a quiet `/proc/interrupts` always means the kernel setup is wrong; the hardware may never be generating interrupts.

**Follow-up Questions:**  
- How should a shared INTx handler behave?
- How do you request multiple MSI-X vectors?
- What would you log after `pci_alloc_irq_vectors()`?

### 12. A PCI driver corrupts memory during heavy traffic. What are likely causes?

**Level:** Mid

**Short Answer:**  
Likely causes include wrong DMA mask, using CPU physical addresses instead of DMA addresses, bad streaming DMA ownership, missing unmap/sync, descriptor-ring bugs, or freeing buffers while DMA is still active.

**Deep Explanation:**  
PCI devices often perform bus-master DMA. The CPU and device do not automatically agree on buffer ownership or address translation. The DMA API provides addresses the device can use and handles cache/IOMMU details. If the driver bypasses it or violates ownership rules, corruption may only appear under load.

For streaming mappings, once a buffer is mapped for device access, the CPU must not read or write it until the buffer is unmapped or synchronized for CPU access.

**API / Code Anchor:**  
`dma_set_mask_and_coherent()`, `dma_alloc_coherent()`, `dma_map_single()`, `dma_unmap_single()`, `dma_map_sg()`, `dma_sync_*()`, `pci_set_master()`.

**Production or Debugging Angle:**  
Look for IOMMU faults, DMA API debug warnings, descriptor indexes wrapping incorrectly, and teardown races.

**Common Traps:**  
- Testing only on a system without IOMMU and missing bugs that appear with IOMMU enabled.
- Reusing a streaming buffer before unmapping it.
- Forgetting that descriptor rings and payload buffers often need different DMA strategies.
- Assuming 64-bit DMA always works.

**Follow-up Questions:**  
- How does coherent DMA differ from streaming DMA?
- Why might corruption appear only under high load?
- What kernel debug options help catch DMA API misuse?

## Senior Questions

### 13. How do managed PCI helpers change error paths and remove paths?

**Level:** Senior

**Short Answer:**  
Managed helpers attach cleanup to the device lifetime, reducing manual unwind code, but the driver must not manually free the same resources or assume cleanup order without understanding helper semantics.

**Deep Explanation:**  
Helpers such as `pcim_enable_device()` and `pcim_iomap_regions()` can make probe failures cleaner because resources are released automatically when the device is detached or probe fails. This is useful, but it changes ownership. If the driver mixes `pcim_*`, `devm_*`, and manual cleanup casually, it can double-free resources or release them in an order that conflicts with active hardware.

For PCI IRQ vectors, current guidance includes a subtle point: with managed PCI enable behavior, vector cleanup can also be managed. A driver should not blindly call `pci_free_irq_vectors()` if the helper path already owns that cleanup.

**API / Code Anchor:**  
`pcim_enable_device()`, `pcim_iomap_regions()`, `pcim_iomap_table()`, `devm_request_irq()`, `pci_free_irq_vectors()`.

**Production or Debugging Angle:**  
During review, build a resource ownership table: each resource has one owner and exactly one cleanup path.

**Common Traps:**  
- Calling manual free APIs for devres/pcim-managed resources.
- Assuming managed cleanup can safely stop active hardware.
- Forgetting that hardware quiescing still needs explicit driver logic.
- Relying on cleanup order without verifying it.

**Follow-up Questions:**  
- Which resources should still be explicitly quiesced even with devm/pcim?
- How would you review a probe path that mixes manual and managed cleanup?
- Why can managed cleanup hide teardown bugs during early testing?

### 14. Why can multi-vector PCI drivers need stricter locking than single-vector drivers?

**Level:** Senior

**Short Answer:**  
Multiple vectors can run handlers concurrently on different CPUs, so shared state must be protected against parallel interrupt execution and local interrupt reentry.

**Deep Explanation:**  
MSI-X enables one device to use many vectors, often one per queue. This improves scalability, but it means two interrupt handlers from the same device can run at the same time. If they touch shared registers, shared rings, global error state, or common statistics, the driver needs locking or per-queue data partitioning.

If a lock is also taken from process context and interrupt context, the process context may need `spin_lock_irqsave()` to avoid deadlock with a local interrupt handler trying to take the same lock.

**API / Code Anchor:**  
`pci_alloc_irq_vectors_affinity()`, `request_irq()`, `spin_lock()`, `spin_lock_irqsave()`, per-queue data structures.

**Production or Debugging Angle:**  
Race bugs may appear only on SMP systems under high interrupt rate. Per-vector debug counters and lockdep are useful.

**Common Traps:**  
- Assuming one device means one interrupt context.
- Using one global lock where per-queue locking would scale better.
- Taking a sleeping lock in hard IRQ context.
- Forgetting process-context vs interrupt-context lock ordering.

**Follow-up Questions:**  
- When would you prefer per-queue state?
- What does lockdep catch in this scenario?
- How can IRQ affinity affect performance and race exposure?

### 15. How should a PCI driver handle power management or suspend/resume?

**Level:** Senior

**Short Answer:**  
It should quiesce users, interrupts, and DMA before suspend, save necessary state, place hardware in a safe low-power state, and restore resources and device state before resuming operation.

**Deep Explanation:**  
PCI devices have power states such as D0 through D3, but a driver cannot treat power state as just a PCI config write. It must coordinate with the upper subsystem, stop new requests, complete or abort in-flight DMA, disable interrupts, and restore device-specific registers after resume. Some state may be lost across low-power transitions or reset.

Modern drivers usually express PM through `dev_pm_ops` attached to the embedded generic device driver rather than old direct PCI PM fields.

**API / Code Anchor:**  
`struct dev_pm_ops`, `pci_save_state()`, `pci_restore_state()`, `pci_set_power_state()`, subsystem suspend/resume hooks.

**Production or Debugging Angle:**  
Suspend/resume bugs often look like dead queues, missing interrupts, DMA faults, or registers returning reset values after resume.

**Common Traps:**  
- Suspending while DMA is still active.
- Forgetting to restore MSI/MSI-X or device interrupt enable state.
- Assuming BAR mappings imply hardware state survived.
- Ignoring runtime PM interactions.

**Follow-up Questions:**  
- What state is generic PCI state vs device-specific state?
- How would you test suspend/resume reliability?
- Why is runtime PM harder for busy PCI devices?

### 16. What is the safe design for a PCI driver that also registers with another subsystem?

**Level:** Senior

**Short Answer:**  
Treat PCI as the resource and transport layer, and register the subsystem object only after the PCI device is ready enough to service subsystem callbacks safely.

**Deep Explanation:**  
Most PCI drivers are not user-visible purely through PCI. A PCI NIC registers a `net_device`, a capture card registers V4L2 objects, and storage drivers register block or SCSI/NVMe objects. Once the upper subsystem registration succeeds, userspace or kernel clients may call into the driver. Therefore, the driver must initialize locks, DMA state, BAR mappings, and interrupt readiness before exposing the object.

On removal, unregister the upper subsystem object early enough to stop new operations, then quiesce hardware and release PCI resources.

**API / Code Anchor:**  
`pci_set_drvdata()`, subsystem-specific registration such as `register_netdev()`, V4L2 registration, block queue registration, and PCI cleanup helpers.

**Production or Debugging Angle:**  
Many races happen when userspace opens the device while probe is still finishing, or when unbind occurs while userspace still has handles.

**Common Traps:**  
- Registering the upper object before private state is complete.
- Releasing PCI resources before unregistering the subsystem object.
- Forgetting file descriptors or subsystem references can outlive remove entry.
- Assuming PCI remove is only called during module unload.

**Follow-up Questions:**  
- Why should subsystem registration usually be late in probe?
- Why should subsystem unregistration usually be early in remove?
- How do open file descriptors affect device lifetime?

### 17. How would you review a PCI probe path for production readiness?

**Level:** Senior

**Short Answer:**  
I would review ID matching, resource acquisition order, DMA mask setup, interrupt allocation, hardware enable timing, rollback paths, managed/manual cleanup ownership, locking, and teardown safety.

**Deep Explanation:**  
A production PCI probe path should be a readable resource ladder. Each step should either be managed or have a clear cleanup label. Hardware should not generate interrupts or DMA until the software can handle them. If the driver registers with an upper subsystem, that should happen only after state is ready.

I would also check whether the code handles hardware variants, unsupported revisions, reduced interrupt availability, 32-bit DMA fallback, and failure of optional features.

**API / Code Anchor:**  
`struct pci_device_id`, `pcim_enable_device()`, `dma_set_mask_and_coherent()`, `pci_set_master()`, `pcim_iomap_regions()`, `pci_alloc_irq_vectors()`, `devm_request_irq()`.

**Production or Debugging Angle:**  
Bring-up should include repeated bind/unbind, interrupt-mode variation, IOMMU enabled testing, suspend/resume, and error-path injection where possible.

**Common Traps:**  
- Reviewing only the success path.
- Missing resource lifetime after subsystem registration.
- Accepting "devm handles cleanup" even when hardware still needs explicit shutdown.
- Not testing fallback from MSI-X to MSI or INTx.

**Follow-up Questions:**  
- What logs should probe emit?
- How would you test every cleanup label?
- What should happen if 64-bit DMA is unavailable?

### 18. A driver works with INTx but fails with MSI-X under load. What are your hypotheses?

**Level:** Senior

**Short Answer:**  
Likely issues include wrong vector-to-queue mapping, using `pdev->irq`, missing per-vector locking, incomplete MSI-X hardware programming, interrupt affinity assumptions, or races exposed by concurrent handlers.

**Deep Explanation:**  
INTx often serializes interrupt handling through one shared line. MSI-X can deliver many independent interrupts concurrently. This changes both the hardware programming model and the concurrency model. A driver that accidentally relies on one-at-a-time interrupt handling may fail only when MSI-X is enabled.

The driver must map each vector to the correct queue or cause, request the correct Linux IRQ for each vector, and protect shared state touched by multiple handlers.

**API / Code Anchor:**  
`pci_alloc_irq_vectors(pdev, min, max, PCI_IRQ_MSIX)`, `pci_irq_vector(pdev, i)`, per-vector `request_irq()`, `spin_lock_irqsave()`.

**Production or Debugging Angle:**  
Log vector allocation count, vector-to-queue mapping, IRQ numbers, and per-vector counters. Compare `/proc/interrupts` with expected queue activity.

**Common Traps:**  
- `pdev->irq` used for all vectors.
- Shared data updated without locking.
- Device register acknowledgement is global but handlers assume per-queue isolation.
- Interrupt affinity masks hide or expose race patterns.

**Follow-up Questions:**  
- How would you temporarily force INTx or MSI for comparison?
- What per-vector stats would you add?
- How can affinity tuning affect throughput and correctness?

### 19. What is dangerous about accessing MMIO registers in a PCI driver?

**Level:** Senior

**Short Answer:**  
MMIO is not normal memory. Accesses need I/O accessors, correct ordering, valid mapped resources, and hardware-specific awareness of posted writes, side effects, and register widths.

**Deep Explanation:**  
MMIO accesses may have side effects, ordering requirements, and architecture-specific behavior. Writes may be posted, meaning the CPU can continue before the device has observed the write. Some registers clear on read, require specific widths, or need readback to flush writes. The driver must use the kernel's I/O accessors and follow the device manual.

BAR lifetime also matters. A mapped pointer is valid only while the resource is owned and mapped. Using it after remove or suspend/reset can crash or misprogram hardware.

**API / Code Anchor:**  
`void __iomem *`, `ioread8/16/32()`, `iowrite8/16/32()`, `readl()`, `writel()`, `pci_iomap()`, `pcim_iomap_regions()`.

**Production or Debugging Angle:**  
If register writes seem ignored, check posted write flushing, reset state, endian/width requirements, and whether the device is enabled and out of low-power state.

**Common Traps:**  
- Normal pointer dereference of `__iomem`.
- Blind register polling without timeout.
- Wrong register width.
- Assuming a write has reached the device without a required readback.

**Follow-up Questions:**  
- What does `__iomem` protect against?
- Why should polling loops have timeouts?
- How do you debug a register write that appears ignored?

### 20. What should a senior engineer say about advanced PCI topics not covered by a basic driver?

**Level:** Senior

**Short Answer:**  
They should recognize the boundaries: basic PCI drivers cover binding, BARs, DMA, and interrupts, while advanced production devices may require AER, reset recovery, hotplug, SR-IOV, runtime PM, ASPM, and PCIe-specific capabilities.

**Deep Explanation:**  
Not every PCI driver needs every advanced feature, but a senior engineer should know when to look for them. A storage, NIC, GPU, or accelerator driver may need robust reset handling, error recovery, virtualization support, and power management. These features interact with DMA, interrupts, subsystem state, and userspace visibility.

The right answer is not to pretend the basic skeleton is enough. The right answer is to identify device requirements and integrate the relevant kernel PCI services.

**API / Code Anchor:**  
PCI capability helpers such as `pci_find_capability()`, PCI error recovery callbacks, reset helpers such as function-level reset APIs, `dev_pm_ops`, SR-IOV APIs where applicable.

**Production or Debugging Angle:**  
Advanced PCI bugs often appear during hot reset, surprise removal, link errors, suspend/resume, virtualized deployment, or high-availability error recovery.

**Common Traps:**  
- Treating all PCIe devices like simple MMIO demo devices.
- Ignoring reset and error recovery until field failures.
- Enabling advanced features without subsystem-level coordination.
- Assuming firmware leaves the device in a clean state forever.

**Follow-up Questions:**  
- When would SR-IOV matter?
- What is AER trying to solve?
- Why does reset recovery need cooperation with the upper subsystem?

