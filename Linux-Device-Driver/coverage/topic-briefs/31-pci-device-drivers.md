# Topic Brief - 31 - PCI Device Drivers

## Output Targets

| Learning Path | Slug | Coverage Brief | Knowledge Target | Interview Target | Example Target |
| --- | --- | --- | --- | --- | --- |
| 31 - PCI Device Drivers | `pci-device-drivers` | `coverage/topic-briefs/31-pci-device-drivers.md` | `knowledge/31-pci-device-drivers.md` | `interview/31-pci-device-drivers.md` | `examples/31-pci-device-drivers/README.md` |

## Source Coverage

| Source ID | Source Root | File | Status | Coverage Notes |
| --- | --- | --- | --- | --- |
| `ldd1-source-root` | ldd1 | `docs/Linux Device Driver Development/` | mapped, gap | No dedicated PCI driver chapter found in the first source set. PCI appears as adjacent material in platform, DMA, device-model, and NIC chapters. |
| `ldd1-ch02` | ldd1 | `Chapter 2-Device Driver Basis.md` | read, mapped, gap | Only an incidental `pci_dev_present()` mention in an error-return example; not useful as PCI driver coverage. |
| `ldd1-ch05` | ldd1 | `Chapter 5-Platform Device Drivers.md` | read, mapped, covered | Contrasts platform devices with discoverable PCI devices, notes `module_pci_driver`, PCI match tables, `MODULE_DEVICE_TABLE(pci, ...)`, and `IORESOURCE_IO`. |
| `ldd1-ch12` | ldd1 | `Chapter 12-DMA - Direct Memory Access.md` | read, mapped, covered | Supplies DMA foundation needed by PCI bus-mastering drivers: coherent and streaming mappings, DMA directions, cache ownership, synchronization, and scatter/gather. |
| `ldd1-ch13` | ldd1 | `Chapter 13-The Linux Device Model.md` | read, mapped, covered | Explains bus/device/driver matching for enumerated buses such as PCI and USB, and how PCI controller devices can themselves be platform devices. |
| `ldd1-ch22` | ldd1 | `Chapter 22-Network Interface Card Drivers.md` | read, mapped, covered | Provides PCI as a common bus for network devices and reinforces that bus-specific setup precedes subsystem registration. Detailed networking remains topic 30. |
| `ldd2-ch01` | ldd2 | `Chapter 1-Linux_Kernel_Concepts.md` | read, mapped, covered | Supports interrupt-locking guidance, especially spinlocks, interrupt context, and deferring work outside hard IRQ handlers. |
| `ldd2-ch10` | ldd2 | `Chapter 10-Linux_Kernel_Power_Management.md` | read, mapped, covered | Provides PCI/ACPI D-state context and PM callback background. Deep PM policy belongs to the power-management topic. |
| `ldd2-ch11` | ldd2 | `Chapter 11-Writing_PCI_Device_Drivers.md` | read, mapped, covered, merged | Primary source. Covers PCI topology, device IDs, enumeration, config space, BARs, MMIO/I/O port resources, INTx/MSI/MSI-X, PCI structures, probe/remove flow, DMA, and locking. |
| `ldd2-ch14` | ldd2 | `Chapter 14-Linux_Kernel_Debugging_Tips.md` | read, mapped, covered | Provides a PCI-oriented dmesg example showing BDF, IRQ, and mapped I/O memory for EHCI debugging. |
| `ldd2-ch07` | ldd2 | `Chapter 7-Demystifying V4L2 and Video Capture Device Drivers.md` | read, mapped, related | Mentions PCI capture hardware as a media bridge example and says a V4L2 bridge can be backed by platform, USB, or PCI hardware. Useful as application context only; detailed V4L2 remains later topics. |
| `notion-source-root` | notion | `docs/Linux-Device-Driver-Notion/` | mapped, gap | No standalone PCI chapter found. Relevant snippets were read from module-loading, platform, resource, and IRQ notes. |
| `notion-ch02-part2` | notion | `Chapter 2-Part 2 Module Dependencies, Loading & Information.md` | read, mapped, covered | Explains module autoloading and `modules.alias`, including a PCI alias for `e1000e`; useful for PCI ID table and hotplug behavior. |
| `notion-ch02-part3` | notion | `Chapter 2-Part 3 Error Handling & Message Printing.md` | read, mapped, gap | Only incidental `pci_dev_present()` usage in a return-value discussion. |
| `notion-ch05-part1` | notion | `Chapter 5-Part 1 Platform Bus & Driver Basics.md` | read, mapped, covered | Contrasts platform devices with PCI devices discovered by bus enumeration. |
| `notion-ch05-part2` | notion | `Chapter 5-Part 2 Probe, Remove & Resource Management.md` | read, mapped, covered | Covers resource types including `IORESOURCE_IO`, relevant to legacy PCI I/O port BARs. |
| `notion-ch16-part1` | notion | `Chapter 16-Part 1 IRQ Architecture and Propagation.md` | read, mapped, covered | Supplies generic IRQ model context for PCI INTx/MSI/MSI-X interrupt handling. |

## Source Files Read

- `Linux-Device-Driver/CODEX.md`
- `Linux-Device-Driver/LEARNING_PATH.md`
- `Linux-Device-Driver/.agents/skills/linux-device-driver/SKILL.md`
- `Linux-Device-Driver/.agents/skills/linux-device-driver/references/topic-brief-template.md`
- `Linux-Device-Driver/.codex/agents/lld-topic-explorer.toml`
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 2-Device Driver Basis.md`
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 5-Platform Device Drivers.md`
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 12-DMA - Direct Memory Access.md`
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 13-The Linux Device Model.md`
- `Linux-Device-Driver/docs/Linux Device Driver Development/Chapter 22-Network Interface Card Drivers.md`
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md`
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 7-Demystifying V4L2 and Video Capture Device Drivers.md`
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 10-Linux_Kernel_Power_Management.md`
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 11-Writing_PCI_Device_Drivers.md`
- `Linux-Device-Driver/docs/Linux Device Driver Development 2/Chapter 14-Linux_Kernel_Debugging_Tips.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 2-Part 2 Module Dependencies, Loading & Information.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 2-Part 3 Error Handling & Message Printing.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 5-Part 1 Platform Bus & Driver Basics.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 5-Part 2 Probe, Remove & Resource Management.md`
- `Linux-Device-Driver/docs/Linux-Device-Driver-Notion/Chapter 16-Part 1 IRQ Architecture and Propagation.md`

## Inventory Decisions

- `ldd2-ch11` is the only dedicated PCI driver source and is the primary base for this topic.
- ldd1 has no dedicated PCI chapter. Its relevant content is spread across platform-bus contrast, DMA, device-model matching, and NIC bus setup.
- Notion has no standalone PCI chapter. It contributes adjacent material on module aliases, platform-vs-PCI discovery, resource flags, and IRQ architecture.
- Same-number chapters were not merged by number. `ldd2-ch11` is a PCI chapter; same-number content from the first source set is not a PCI driver source and was not treated as equivalent.
- Notion and V4L2 hits for the word "endpoint" are media graph endpoints, not PCIe endpoints, and should not be merged into this PCI topic.
- PCI capture-card and PCI NIC mentions are useful examples of PCI as a bus, but their subsystem-specific driver logic belongs to V4L2 and networking topics.

## Merged Source Notes

### Core Mental Model

- A PCI driver normally does not discover hardware by scanning manually. The PCI core enumerates buses, reads configuration space, builds `struct pci_dev` objects, and matches them against registered `struct pci_driver` ID tables.
- Matching is based on fields such as vendor ID, device ID, subsystem IDs, class code, and class mask. A correct `MODULE_DEVICE_TABLE(pci, ids)` creates module alias metadata so udev/kmod can autoload the driver for matching hardware.
- PCI, PCI-X, and PCI Express differ electrically and topologically, but most Linux driver structure is shared: match a device, enable it, request/map BAR resources, configure DMA/interrupts, register with the upper subsystem, then unwind in reverse order.

### Topology And Enumeration

- PCIe systems are built from a root complex, downstream ports, bridges/switches, links, lanes, and endpoints.
- Each function is identified by BDF: bus, device, function. The classic layout supports 256 buses, 32 devices per bus, and 8 functions per device.
- The PCI core walks the hierarchy through bridges and switch ports, assigns bus numbers and resources, and exposes devices under sysfs paths such as `/sys/bus/pci/devices/0000:bb:dd.f`.
- Configuration space contains vendor/device IDs, subsystem IDs, class/revision/header fields, BAR registers, interrupt pin/line fields, and capability lists. PCIe extends configuration space beyond the older 256-byte PCI space.

### Address Spaces And BARs

- PCI devices expose resources through configuration space, MMIO BARs, and sometimes legacy I/O port BARs.
- A BAR describes a device resource window. Firmware or the PCI core assigns host-visible addresses so CPU loads/stores or port I/O can reach device registers.
- Modern drivers should prefer MMIO over legacy I/O ports where hardware allows it.
- Use PCI resource helpers such as `pci_resource_start()`, `pci_resource_len()`, `pci_resource_end()`, and `pci_resource_flags()` to inspect assigned resources.
- Request resources before mapping them. Common APIs include `pci_request_region()`, `pci_request_regions()`, `pci_iomap()`, `pci_iomap_range()`, `pci_ioremap_bar()`, `pci_iounmap()`, and the managed `pcim_iomap_regions()` / `pcim_iomap_table()` family.
- Access mapped MMIO through I/O accessors such as `ioread8/16/32()` and `iowrite8/16/32()`, not normal pointer dereferences. Use barriers or readbacks when posted writes matter.

### Key Structures And APIs

- `struct pci_device_id`: describes supported devices. Key fields are `vendor`, `device`, `subvendor`, `subdevice`, `class`, `class_mask`, and `driver_data`. Common macros include `PCI_DEVICE()`, `PCI_DEVICE_CLASS()`, and `PCI_DEVICE_SUB()`.
- `struct pci_driver`: contains `name`, `id_table`, `probe`, `remove`, optional shutdown/error/PM hooks, and the embedded generic driver state.
- `struct pci_dev`: represents one PCI function and includes identity, class, resource, IRQ, MSI/MSI-X, embedded `struct device`, bus hierarchy, and driver binding state.
- Registration APIs: `pci_register_driver()`, `pci_unregister_driver()`, and `module_pci_driver()`.
- Enablement APIs: `pci_enable_device()`, `pci_enable_device_mem()`, `pci_enable_device_io()`, `pcim_enable_device()`, `pci_disable_device()`.
- Bus mastering APIs: `pci_set_master()` and `pci_clear_master()`.
- Configuration-space APIs: `pci_read_config_byte()`, `pci_read_config_word()`, `pci_read_config_dword()`, and matching write helpers.
- DMA APIs: prefer generic DMA APIs such as `dma_set_mask_and_coherent()`, `dma_alloc_coherent()`, `dma_free_coherent()`, `dma_map_single()`, `dma_unmap_single()`, `dma_map_sg()`, `dma_unmap_sg()`, and sync helpers.

### Probe Lifecycle

1. The PCI core matches a discovered `struct pci_dev` against the driver's `struct pci_device_id` table.
2. `probe()` is called with the matched `pci_dev` and ID entry.
3. Enable the device with `pci_enable_device()` or `pcim_enable_device()`.
4. Set DMA masks with `dma_set_mask_and_coherent()` before allocating DMA-visible buffers.
5. Request and map BAR resources.
6. Enable bus mastering with `pci_set_master()` when the device will initiate DMA.
7. Allocate coherent descriptor rings, command areas, or control blocks if needed.
8. Allocate interrupt vectors with `pci_alloc_irq_vectors()` and obtain Linux IRQ numbers with `pci_irq_vector()`.
9. Request IRQ handlers after the driver state is initialized but before enabling hardware interrupt generation.
10. Register the upper-level object, such as a netdev, block queue, V4L2 device, sound card, or misc/debug interface.
11. Program device registers and enable DMA/interrupt engines only after software is ready to handle completions.

### Remove And Error Paths

- Stop new device work first: disable device interrupt generation, stop queues, quiesce DMA engines, and flush deferred work.
- Free IRQs before tearing down state that handlers can access.
- Unmap streaming DMA buffers before freeing or reusing CPU buffers.
- Free coherent DMA memory after hardware can no longer access it.
- Unregister subsystem-facing objects before releasing low-level PCI resources.
- Release BAR mappings and resource reservations.
- Disable bus mastering and the PCI device unless managed helpers perform that cleanup.
- Every probe step needs a matching rollback path for partial initialization failures.

### Interrupts

- Legacy INTx uses shared, level-triggered interrupt pins and may require shared IRQ handlers. Do not assume `dev->irq` is unique.
- MSI uses in-band memory writes from the device to signal interrupts and avoids shared INTx lines.
- MSI-X supports more vectors and per-vector configuration, making it a better fit for multi-queue devices.
- Current code should use `pci_alloc_irq_vectors()` or `pci_alloc_irq_vectors_affinity()` with flags such as `PCI_IRQ_INTX`, `PCI_IRQ_MSI`, `PCI_IRQ_MSIX`, `PCI_IRQ_ALL_TYPES`, and `PCI_IRQ_AFFINITY`, then call `pci_irq_vector()`.
- Old `pci_enable_msi()` and `pci_enable_msix_*()` APIs should be treated as legacy.
- Multi-vector devices can run two handlers concurrently on different CPUs. If handlers share driver state, use appropriate locking such as `spin_lock_irqsave()` where local interrupt reentry matters.

### DMA

- PCI devices commonly perform bus-master DMA. Drivers must never hand raw CPU physical addresses to hardware directly.
- Set the device's DMA mask before DMA allocation or mapping.
- Use coherent DMA for rings, descriptors, doorbell-visible control blocks, or areas concurrently visible to CPU and device.
- Use streaming DMA for packet payloads, block I/O buffers, frame buffers, or other one-direction-at-a-time transfers.
- Respect ownership rules: after mapping a buffer for device access, the CPU must not touch it until it is unmapped or synchronized for CPU access.
- Scatter/gather DMA is common for high-throughput PCI devices and should use `dma_map_sg()` / `dma_unmap_sg()` rather than hand-built bus addresses.

### Power Management And Reset Context

- PCI devices have D-states such as D0 through D3. The source material mentions PCI/ACPI power states, but does not provide deep modern runtime PM or reset coverage.
- Later teaching should introduce PM only enough for PCI driver readiness: quiesce DMA/IRQs before suspend, save/restore state when needed, and resume hardware in a known order.
- Deep topics such as FLR, hot reset, AER recovery, DPC, ASPM, and SR-IOV are gaps for external validation or advanced follow-up.

### Debugging And Userspace Observability

- Use `lspci -nn -vv` to inspect BDF, vendor/device IDs, class, BARs, capabilities, MSI/MSI-X state, kernel driver, and bound modules.
- Use sysfs under `/sys/bus/pci/devices/0000:bb:dd.f/` for driver binding, resources, config access, power state, and modalias inspection.
- Use `/proc/interrupts` to verify whether INTx/MSI/MSI-X vectors are allocated and firing.
- Use dmesg to correlate probe order, BAR mapping, IRQ numbers, DMA mask failures, and subsystem registration.
- Module autoloading depends on correct alias metadata generated from `MODULE_DEVICE_TABLE(pci, ids)`.

## Source Differences

- `ldd2-ch11` uses `PCI_IRQ_LEGACY` in examples. Current local Linux 6.8 headers keep this as a deprecated alias and prefer `PCI_IRQ_INTX`.
- `ldd2-ch11` discusses older PCI-specific DMA wrappers such as `pci_alloc_consistent()`, `pci_map_single()`, and `pci_map_sg()`. Current learning content should prefer the generic DMA API and mention older wrappers only as legacy/source-history context.
- `ldd2-ch11` shows explicit resource management with `pci_request_regions()` and `pci_iomap()`. Current code can use managed PCI helpers such as `pcim_enable_device()`, `pcim_iomap_regions()`, and `pcim_iomap_table()` for simpler cleanup.
- Current MSI documentation adds an important cleanup nuance: if `pcim_enable_device()` is used, IRQ vectors are automatically released by managed cleanup, so manually calling `pci_free_irq_vectors()` can be wrong.
- Older source material presents `struct pci_driver` PM fields directly. Modern drivers often use `driver.pm = &dev_pm_ops` and PCI PM helpers in suspend/resume paths.
- ldd1 and Notion sources mostly describe PCI by contrast with non-discoverable platform devices. They are useful for framing, but they do not replace the dedicated ldd2 PCI chapter.

## Gaps / Uncertainties

- No internal source gives a complete modern Linux 6.x managed PCI skeleton using `pcim_*`, `dma_set_mask_and_coherent()`, and current MSI vector flags together.
- Internal sources do not deeply cover PCIe Advanced Error Reporting, Downstream Port Containment, PCI hotplug, SR-IOV, ATS/PRI/PASID, ASPM, runtime PM, FLR, or reset recovery.
- Internal sources do not provide a real hardware datasheet, so BAR layout, register meanings, descriptor formats, endianness, reset timing, and interrupt semantics must be treated as device-specific.
- Notion does not include a dedicated PCI chapter; its coverage is adjacent and should be marked as a source gap, not skipped.
- Future examples should avoid pretending to drive nonexistent hardware. A buildable skeleton with explicit fake IDs or a sysfs/lspci exploration lab is safer than an untestable "real" device driver.

## External Validation

External validation is needed because the primary PCI chapter targets an older kernel baseline and several APIs have modern replacements or cleanup caveats.

| External Source | Purpose |
| --- | --- |
| `https://docs.kernel.org/PCI/pci.html` | Validate current PCI driver registration, initialization sequence, resource handling, DMA setup, IRQ registration, and shutdown ordering. |
| `https://docs.kernel.org/PCI/msi-howto.html` | Validate modern MSI/MSI-X APIs, vector flags, `pci_irq_vector()`, `PCI_IRQ_INTX`, and managed cleanup interaction with `pcim_enable_device()`. |
| `https://docs.kernel.org/driver-api/pci/pci.html` | Validate PCI support-library APIs, capability helpers, lookup/reference behavior, and current kernel API vocabulary. |
| `https://docs.kernel.org/PCI/index.html` | Validate official PCI documentation scope and related advanced PCI topics. |
| Local headers: `/lib/modules/6.8.0-124-generic/build/include/linux/pci.h` and `/lib/modules/6.8.0-124-generic/build/include/linux/dma-mapping.h` | Confirm current signatures and symbols for `struct pci_driver`, `module_pci_driver`, `pcim_enable_device`, `pcim_iomap_regions`, `pci_alloc_irq_vectors`, `PCI_IRQ_INTX`, `PCI_IRQ_LEGACY`, and `dma_set_mask_and_coherent`. |

## Learning Content Brief

### What To Teach

- PCI is a discoverable bus framework where the kernel enumerates hardware and binds drivers by ID tables.
- A PCI driver owns one PCI function at a time through `probe()` and must enable the device, claim resources, map BARs, configure DMA, allocate IRQ vectors, and register with the relevant upper subsystem.
- The driver author's main responsibilities are resource ownership, DMA correctness, interrupt correctness, cleanup ordering, and subsystem integration.
- PCI drivers are rarely useful alone; most are also network, storage, media, sound, accelerator, or misc drivers.

### Why It Exists

- PCI provides a common hardware discovery and resource-assignment model for plug-in and integrated devices.
- The Linux PCI core hides enumeration, config-space probing, resource assignment, and driver matching so individual drivers can focus on device-specific registers and data movement.
- The framework also integrates hotplug, power management, DMA/IOMMU setup, MSI/MSI-X, sysfs, driver binding, and module autoloading.

### When To Use It

- Use PCI driver APIs when hardware appears on PCI, PCI-X, or PCIe and has vendor/device/class IDs in PCI configuration space.
- Do not write a platform driver for a normal PCI function just because it is soldered on a board.
- Do not write manual bus-scanning logic in a normal function driver; rely on the PCI core and ID table matching.
- Combine PCI with another subsystem when appropriate: netdev for NICs, DRM/V4L2 for graphics or capture hardware, ALSA for audio, NVMe/SCSI/block for storage, and so on.

### Minimum API Set For A New Driver

- ID table: `static const struct pci_device_id ids[] = { ... };`
- Export aliases: `MODULE_DEVICE_TABLE(pci, ids);`
- Driver object: `static struct pci_driver drv = { .name = ..., .id_table = ids, .probe = ..., .remove = ... };`
- Registration: `module_pci_driver(drv);`
- Probe basics: `pcim_enable_device()` or `pci_enable_device()`, `dma_set_mask_and_coherent()`, `pci_set_master()`, BAR request/map helpers, `pci_alloc_irq_vectors()`, `pci_irq_vector()`, `request_irq()` or `devm_request_irq()`.
- Remove basics: stop hardware, free IRQs, unmap/unregister/free DMA state, release regions, disable device, respecting managed helper behavior.

### Practical Pseudo-code Direction

```c
static int demo_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int ret, irq;
	void __iomem *regs;

	ret = pcim_enable_device(pdev);
	if (ret)
		return ret;

	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (ret)
		ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;

	pci_set_master(pdev);

	ret = pcim_iomap_regions(pdev, BIT(0), "demo-pci");
	if (ret)
		return ret;

	regs = pcim_iomap_table(pdev)[0];

	ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_ALL_TYPES);
	if (ret < 0)
		return ret;

	irq = pci_irq_vector(pdev, 0);
	ret = devm_request_irq(&pdev->dev, irq, demo_irq, 0, "demo-pci", pdev);
	if (ret)
		return ret;

	/* Initialize driver state, then enable device interrupts/DMA. */
	return 0;
}
```

Teaching note: this pseudo-code needs a caveat that `pci_free_irq_vectors()` is not manually called when managed PCI cleanup owns vector release through `pcim_enable_device()`.

### Common Bugs To Cover

- Missing `MODULE_DEVICE_TABLE(pci, ids)`, causing manual `insmod` to work but autoloading to fail.
- Touching BAR registers before enabling the PCI device or mapping resources.
- Using raw physical addresses instead of DMA-mapped addresses.
- Forgetting `dma_set_mask_and_coherent()` and failing on systems where the device cannot address all RAM.
- Enabling hardware interrupts before handler state is ready.
- Assuming INTx interrupts are not shared.
- Using `dev->irq` for MSI-X vectors instead of `pci_irq_vector()`.
- Calling legacy MSI APIs in new code.
- Forgetting to stop DMA before freeing buffers.
- Unmapping/freeing state while IRQ handlers or work items can still run.
- Mishandling posted MMIO writes and missing readbacks/barriers where the hardware requires ordering.
- Mixing managed and unmanaged cleanup paths incorrectly.

### Debugging Checklist

- Confirm the device exists: `lspci -nn`.
- Confirm class/vendor/device IDs match the driver's ID table.
- Confirm module alias: `modinfo <module>` and `/sys/bus/pci/devices/.../modalias`.
- Confirm driver binding: `lspci -k` and `/sys/bus/pci/devices/.../driver`.
- Confirm resources: `lspci -vv`, sysfs `resource`, and dmesg probe logs.
- Confirm interrupts: `/proc/interrupts`, MSI/MSI-X capability state in `lspci -vv`, and driver logs.
- Confirm DMA failures through probe return codes and IOMMU/kernel logs.
- Confirm cleanup with repeated bind/unbind, module unload/reload, suspend/resume, and error-injection where available.

### Production Checklist

- Correct ID table and alias export.
- Strict probe rollback and remove ordering.
- Device quiesced before IRQ/DMA/resource teardown.
- DMA mask set before allocation/mapping.
- Coherent vs streaming DMA used intentionally.
- MSI-X/MSI/INTx fallback logic tested.
- Shared interrupt path handles unrelated interrupts.
- Locking safe across multiple vectors and CPUs.
- MMIO accessors and ordering are hardware-correct.
- Power-management and reset paths leave hardware and software state consistent.
- Sysfs, devres, and subsystem registration lifetimes are aligned.
- Tested on systems with and without IOMMU, with different interrupt modes, and through bind/unbind cycles.

### Interview Readiness

- Explain how the PCI core matches a device to a driver.
- Explain the difference between config space, BAR/MMIO space, and I/O port space.
- Walk through PCI `probe()` from ID match to hardware enablement.
- Compare INTx, MSI, and MSI-X.
- Explain why PCI drivers need the DMA API.
- Explain why `pci_set_master()` matters.
- Describe how to debug a device that appears in `lspci` but never probes.
- Describe how to debug interrupts that never fire.
- Describe safe remove/error-path ordering for an active DMA device.
- Explain the danger of mixing managed PCI cleanup and manual cleanup.
