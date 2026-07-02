# 31 - PCI Device Drivers

## Learning Goal

After this topic, you should be able to explain how Linux binds a PCI/PCIe device to a driver, write the shape of a PCI `probe()` path, map BAR registers, configure DMA and interrupts, and debug common bring-up failures.

You should be comfortable with:

- PCI identity: vendor ID, device ID, class, subsystem IDs, and BDF.
- Resource ownership: BARs, MMIO, I/O ports, config space, IRQ vectors, and DMA buffers.
- Driver lifecycle: match, probe, enable, request/map, register, run, quiesce, remove.
- Practical traps: stale MSI APIs, wrong DMA masks, shared INTx interrupts, unsafe cleanup ordering, and mixed managed/unmanaged cleanup.

## Why This Matters In Real Work

PCI is the bus behind many high-performance Linux devices: NICs, NVMe controllers, GPUs, capture cards, audio devices, storage adapters, accelerators, and many integrated SoC blocks exposed through PCIe.

In real drivers, PCI is often the lower bus layer. The actual driver may be a network, block, DRM, V4L2, ALSA, or accelerator driver, but it still starts with PCI binding and resource setup.

- PCI solves **hardware discovery**: the device identifies itself in configuration space.
- PCI solves **resource assignment**: firmware and the kernel assign BAR windows, bus numbers, and interrupt routing.
- PCI solves **driver binding**: the PCI core matches discovered devices against `struct pci_device_id` tables.
- PCI integrates with **sysfs, module autoloading, DMA/IOMMU, MSI/MSI-X, hotplug, and power management**.
- PCI drivers must be especially careful because they often control devices that can DMA into memory and interrupt on multiple CPUs.

## Mental Model

Think of a PCI device driver as a driver for one discovered hardware function. The PCI core finds the device first; your driver is called only after the kernel has already identified the function and matched it to your ID table.

The driver does not normally scan the PCI bus itself. Instead:

```text
Firmware / PCI core
    -> enumerate PCI hierarchy
    -> read config space
    -> create struct pci_dev
    -> match against struct pci_driver.id_table
    -> call driver probe()
```

Inside `probe()`, the driver asks the kernel for permission to use the hardware resources:

```text
enable device
    -> set DMA mask
    -> claim/map BARs
    -> enable bus mastering
    -> allocate DMA state
    -> allocate IRQ vectors
    -> register with upper subsystem
    -> enable device DMA/interrupts
```

The most important idea: **the PCI framework owns discovery and generic resources; the driver owns device-specific policy, registers, DMA rings, IRQ handling, and cleanup.**

## Core Concepts

PCI has a few words that appear everywhere in driver code and debugging output.

| Concept | Meaning | Driver Impact |
| --- | --- | --- |
| BDF | Bus:Device.Function address, often shown as `0000:bb:dd.f` with domain | Used in `lspci`, sysfs, dmesg, and debugging. |
| Function | One independently addressable PCI unit | A physical card may expose multiple functions. |
| Config space | Standard PCI metadata and control area | Read with `pci_read_config_*()` only when needed. |
| BAR | Base Address Register describing a device resource window | Usually mapped as MMIO with PCI helpers. |
| MMIO | Device registers accessed through memory-mapped I/O | Use `ioread*()` / `iowrite*()`, not normal pointer dereference. |
| I/O port BAR | Legacy port I/O resource | Mostly x86/legacy; prefer MMIO when possible. |
| INTx | Legacy shared interrupt pins | Shared and level-triggered; handler must verify device status. |
| MSI | Message Signaled Interrupts | Device raises interrupts by writing a message, usually not shared. |
| MSI-X | Extended MSI with many independent vectors | Used by multi-queue/high-throughput devices. |
| Bus mastering | Device can initiate DMA transactions | Enable with `pci_set_master()` when the hardware will DMA. |

### PCI vs Platform Drivers

Platform devices are often described by firmware data such as Device Tree or ACPI tables. PCI devices normally self-identify through configuration space.

| Question | PCI Driver | Platform Driver |
| --- | --- | --- |
| How is hardware discovered? | PCI bus enumeration reads config space. | Board firmware or platform code describes it. |
| How is matching done? | Vendor/device/class IDs in `struct pci_device_id`. | `compatible`, ACPI ID, or platform name. |
| Where are resources described? | PCI BARs and config space assigned by PCI core. | DT/ACPI/platform resources. |
| Typical hardware | NICs, GPUs, NVMe, capture cards, PCIe endpoints. | SoC peripherals, memory-mapped IP blocks. |

Do not write a platform driver for a normal PCI function just because the PCIe device is soldered on the board.

## Kernel Mechanism

The PCI subsystem is a bus implementation inside the Linux device model. It creates `struct pci_dev` objects and binds them to `struct pci_driver` objects.

The main object relationships look like this:

```text
pci_bus
  -> pci_dev
       -> embedded struct device
       -> resource[] entries for BARs
       -> IRQ/MSI/MSI-X state
       -> bound pci_driver

pci_driver
  -> id_table
  -> probe()
  -> remove()
  -> optional PM/shutdown/error callbacks
```

### Enumeration And Matching

- The PCI core walks the hierarchy from root complex through bridges and switches.
- It assigns bus numbers and discovers endpoints/functions.
- For each function, it reads config space and creates a `struct pci_dev`.
- When a PCI driver is registered, the core compares the driver's ID table with existing devices.
- On a match, the core calls the driver's `probe()`.
- If hardware appears later through hotplug, the same match/probe mechanism is used.

### Module Autoloading

`MODULE_DEVICE_TABLE(pci, ids)` is not decorative. It emits module alias metadata.

That metadata lets userspace load the module when a matching PCI device appears:

```text
PCI device modalias in sysfs
    -> udev/kmod sees alias
    -> matching module is loaded
    -> pci_register_driver()
    -> PCI core binds device and calls probe()
```

Without the alias, manual `insmod` may work, but automatic loading can fail.

## Key Structs And APIs

PCI APIs are easiest to remember by lifecycle role rather than by memorizing a flat list.

### Identification And Registration

| API / Struct | Purpose |
| --- | --- |
| `struct pci_device_id` | Describes devices this driver supports. |
| `PCI_DEVICE(vendor, device)` | Common ID-table entry for exact vendor/device match. |
| `PCI_DEVICE_CLASS(class, mask)` | Match by PCI class code. |
| `PCI_DEVICE_SUB(vendor, device, subvendor, subdevice)` | Match a specific subsystem identity. |
| `MODULE_DEVICE_TABLE(pci, ids)` | Exports aliases for module autoloading. |
| `struct pci_driver` | The driver object registered with the PCI core. |
| `module_pci_driver(driver)` | Module helper for register/unregister boilerplate. |
| `pci_register_driver()` / `pci_unregister_driver()` | Manual registration and unregistration. |

Minimal registration shape:

```c
static const struct pci_device_id demo_ids[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_FOO, 0x1234) },
	{ }
};
MODULE_DEVICE_TABLE(pci, demo_ids);

static struct pci_driver demo_driver = {
	.name = "demo_pci",
	.id_table = demo_ids,
	.probe = demo_probe,
	.remove = demo_remove,
};

module_pci_driver(demo_driver);
```

### Device Enablement And BARs

| API | Purpose |
| --- | --- |
| `pci_enable_device()` | Enable PCI resources so the driver can use the device. |
| `pci_enable_device_mem()` / `pci_enable_device_io()` | Enable only memory or I/O resources. |
| `pcim_enable_device()` | Managed enable helper. |
| `pci_disable_device()` | Disable a manually enabled device. |
| `pci_resource_start()` / `pci_resource_len()` / `pci_resource_flags()` | Inspect BAR resources. |
| `pci_request_region()` / `pci_request_regions()` | Claim BAR resources. |
| `pci_iomap()` / `pci_ioremap_bar()` | Map BAR MMIO into kernel virtual address space. |
| `pci_iounmap()` | Unmap manually mapped BAR space. |
| `pcim_iomap_regions()` / `pcim_iomap_table()` | Managed BAR claim/map helpers. |

**Rule:** request/claim a BAR before mapping or using it.

### Config Space

| API | Purpose |
| --- | --- |
| `pci_read_config_byte()` | Read an 8-bit config-space field. |
| `pci_read_config_word()` | Read a 16-bit config-space field. |
| `pci_read_config_dword()` | Read a 32-bit config-space field. |
| `pci_write_config_*()` | Write config-space fields. |
| `pci_find_capability()` | Locate a PCI capability. |

Use config-space APIs for standard PCI metadata and control. Do not confuse config space with device runtime registers behind BARs.

### DMA

| API | Purpose |
| --- | --- |
| `dma_set_mask_and_coherent()` | Tell the DMA layer what bus addresses the device can use. |
| `pci_set_master()` | Allow the PCI device to initiate DMA. |
| `pci_clear_master()` | Stop bus mastering during teardown or error handling. |
| `dma_alloc_coherent()` / `dma_free_coherent()` | Allocate memory visible to both CPU and device, often for rings/descriptors. |
| `dma_map_single()` / `dma_unmap_single()` | Map a linear CPU buffer for device access. |
| `dma_map_sg()` / `dma_unmap_sg()` | Map scatter/gather lists for device access. |
| `dma_sync_*()` | Transfer ownership or visibility between CPU and device for streaming mappings. |

**Rule:** the hardware gets DMA addresses returned by the DMA API, not raw CPU physical addresses.

### Interrupts

| API | Purpose |
| --- | --- |
| `pci_alloc_irq_vectors()` | Allocate INTx, MSI, or MSI-X vectors. |
| `pci_alloc_irq_vectors_affinity()` | Allocate vectors with affinity hints. |
| `pci_irq_vector()` | Convert a PCI vector index to a Linux IRQ number. |
| `pci_free_irq_vectors()` | Free manually allocated vectors. |
| `request_irq()` / `devm_request_irq()` | Register the interrupt handler. |
| `PCI_IRQ_INTX` | Allow legacy INTx. |
| `PCI_IRQ_MSI` | Allow MSI. |
| `PCI_IRQ_MSIX` | Allow MSI-X. |
| `PCI_IRQ_ALL_TYPES` | Allow all supported interrupt types. |
| `PCI_IRQ_AFFINITY` | Ask the core to spread vectors across CPUs. |

**Rule:** after using `pci_alloc_irq_vectors()`, get IRQ numbers with `pci_irq_vector()`. Do not assume `pdev->irq` is correct for MSI-X.

## Lifecycle / Data Flow

The lifecycle is a resource ladder: every successful step must have a matching cleanup step.

```text
driver module loaded
    -> pci_register_driver()
    -> PCI core matches id_table
    -> probe(pdev, id)
        -> enable device
        -> set DMA mask
        -> request/map BARs
        -> enable bus mastering
        -> allocate driver state
        -> allocate DMA memory or map buffers
        -> allocate IRQ vectors
        -> request IRQ handlers
        -> register with upper subsystem
        -> enable hardware engines
    -> runtime operation
        -> IRQ handler acknowledges device
        -> DMA completions processed
        -> upper subsystem queues work
    -> remove(), unbind, error path, or unload
        -> stop queues/new work
        -> disable device interrupt generation
        -> stop/quiesce DMA
        -> free IRQs / vectors
        -> unregister upper subsystem object
        -> unmap/free DMA
        -> unmap/release BARs
        -> clear bus master / disable device
```

### Probe Order That Usually Works

1. Allocate or initialize private state.
2. Enable the device.
3. Set DMA masks.
4. Claim/map BARs.
5. Enable bus mastering if the device performs DMA.
6. Allocate DMA-visible control structures.
7. Allocate IRQ vectors.
8. Register IRQ handlers.
9. Register with the upper subsystem.
10. Enable hardware DMA and interrupts last.

### Remove Order That Usually Works

1. Stop new userspace or subsystem requests.
2. Disable hardware interrupt generation.
3. Stop DMA engines and wait for in-flight DMA to finish.
4. Flush workqueues, timers, NAPI, threaded IRQ work, or tasklets.
5. Free IRQ handlers and vectors.
6. Unregister subsystem objects.
7. Unmap/free DMA memory and streaming mappings.
8. Release BAR mappings and resources.
9. Disable bus mastering and the PCI device if not managed.

The exact order depends on the subsystem, but the safety rule is stable: **hardware must not be able to touch memory or call handlers after the software state is gone.**

## Minimal Practical Example

This is **learning-only pseudo-code**. It shows a modern PCI `probe()` shape, not a production-ready driver. A real driver needs hardware-specific register definitions, reset sequencing, DMA ring setup, error handling, subsystem registration, and a tested remove path.

```c
#include <linux/bitops.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/pci.h>

#define DEMO_BAR 0

struct demo_pci {
	void __iomem *regs;
	int irq;
};

static irqreturn_t demo_irq(int irq, void *data)
{
	struct demo_pci *priv = data;

	/* Read device status from priv->regs, acknowledge if this device fired. */
	if (!priv)
		return IRQ_NONE;

	return IRQ_HANDLED;
}

static int demo_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct demo_pci *priv;
	int ret;

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	pci_set_drvdata(pdev, priv);

	ret = pcim_enable_device(pdev);
	if (ret)
		return ret;

	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (ret)
		ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (ret)
		return ret;

	pci_set_master(pdev);

	ret = pcim_iomap_regions(pdev, BIT(DEMO_BAR), "demo_pci");
	if (ret)
		return ret;

	priv->regs = pcim_iomap_table(pdev)[DEMO_BAR];
	if (!priv->regs)
		return -ENOMEM;

	ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_ALL_TYPES);
	if (ret < 0)
		return ret;

	priv->irq = pci_irq_vector(pdev, 0);

	ret = devm_request_irq(&pdev->dev, priv->irq, demo_irq, 0,
			       "demo_pci", priv);
	if (ret)
		return ret;

	/*
	 * Real driver would initialize hardware registers here, then enable
	 * DMA/interrupt generation only after all software state is ready.
	 */
	return 0;
}
```

Important lines:

- `pcim_enable_device()` enables the PCI function with managed cleanup.
- `dma_set_mask_and_coherent()` is done before DMA allocation or mapping.
- `pci_set_master()` allows the device to initiate DMA.
- `pcim_iomap_regions()` claims and maps a BAR.
- `pci_alloc_irq_vectors()` chooses INTx/MSI/MSI-X according to hardware and flags.
- `pci_irq_vector()` returns the Linux IRQ number for a vector index.
- Hardware interrupts are enabled only after handler state exists.

Cleanup caveat:

- With managed PCI helpers, cleanup may be automatic for some resources.
- Do not blindly mix manual cleanup with managed cleanup.
- In particular, when IRQ vectors are managed through `pcim_enable_device()` behavior on current kernels, manually freeing vectors can be wrong. Check the helper semantics used by the driver and kernel version.

## Common Bugs And Debugging

PCI failures are usually visible through `lspci`, sysfs, dmesg, `/proc/interrupts`, or DMA/IOMMU logs. Start from the symptom, then walk backward through the lifecycle.

### Device Appears In `lspci` But Driver Never Probes

Likely causes:

- ID table does not match vendor/device/subsystem/class.
- Missing `MODULE_DEVICE_TABLE(pci, ids)`, so autoloading fails.
- Module is not installed where `modprobe` can find it.
- Another driver already bound to the device.
- Device is disabled by firmware, policy, or driver override.

Evidence to inspect:

```sh
lspci -nn -k
cat /sys/bus/pci/devices/0000:bb:dd.f/modalias
modinfo your_module
readlink /sys/bus/pci/devices/0000:bb:dd.f/driver
dmesg | tail -100
```

Fix pattern:

- Correct the `struct pci_device_id` table.
- Add or fix `MODULE_DEVICE_TABLE(pci, ids)`.
- Bind manually through sysfs only for debugging, not as the final production solution.

### Probe Fails While Mapping BARs

Likely causes:

- BAR index is wrong.
- Device has an I/O port BAR but driver assumes MMIO.
- Resource length is zero or not assigned.
- Another driver owns the resource.
- Driver maps without requesting the region.

Evidence to inspect:

```sh
lspci -vv -s bb:dd.f
cat /sys/bus/pci/devices/0000:bb:dd.f/resource
dmesg | grep -i pci
```

Fix pattern:

- Check `pci_resource_flags()` before mapping.
- Use `pci_request_regions()` / `pcim_iomap_regions()`.
- Use MMIO accessors for MMIO BARs and port I/O helpers for I/O BARs.

### Interrupt Handler Never Runs

Likely causes:

- Hardware interrupt generation was never enabled in device registers.
- Driver requested the wrong IRQ number.
- MSI/MSI-X allocation failed and fallback was not handled.
- INTx is shared and the handler returns the wrong value.
- The device is stuck because status was not acknowledged.

Evidence to inspect:

```sh
cat /proc/interrupts
lspci -vv -s bb:dd.f
dmesg | grep -i -E 'msi|irq|pci'
```

Fix pattern:

- Use `pci_alloc_irq_vectors()` and `pci_irq_vector()`.
- For INTx, use a handler that checks device status and returns `IRQ_NONE` when the interrupt is not from this device.
- Acknowledge device interrupt status in the order required by the hardware manual.

### DMA Fails Or Corrupts Memory

Likely causes:

- DMA mask not set or too large for the device.
- Driver gives hardware a CPU physical address instead of a DMA address.
- Streaming buffer is accessed by CPU while owned by device.
- Missing sync/unmap before reading device-written data.
- DMA continues after buffers are freed.
- IOMMU reports mapping faults.

Evidence to inspect:

```sh
dmesg | grep -i -E 'dma|iommu|fault|pci'
```

Fix pattern:

- Call `dma_set_mask_and_coherent()` early.
- Use `dma_alloc_coherent()` for rings/descriptors.
- Use `dma_map_single()` / `dma_map_sg()` and unmap/sync correctly for streaming data.
- Stop DMA before freeing buffers.

### Unload Or Unbind Crashes

Likely causes:

- IRQ handler runs after private state is freed.
- Workqueue/timer/tasklet/NAPI still references freed state.
- DMA engine still writes to freed memory.
- Managed and unmanaged cleanup are mixed incorrectly.

Fix pattern:

- Disable hardware interrupts first.
- Stop queues and DMA engines.
- Synchronize/flush deferred work.
- Free IRQs before freeing handler state.
- Free DMA memory only after hardware is quiesced.
- Use either managed cleanup consistently or document exactly where manual cleanup is required.

## Production Checklist

Before a PCI driver is review-ready, check the whole chain from binding to teardown.

### Binding And Identity

- ID table matches the intended vendor/device/class/subsystem IDs.
- `MODULE_DEVICE_TABLE(pci, ids)` is present.
- Probe logs include enough device identity to debug field failures.
- Driver handles unsupported revisions or subsystem variants cleanly.

### Resource Handling

- Device is enabled before resources are used.
- BAR flags and lengths are validated.
- BARs are requested before mapping.
- MMIO is accessed only through I/O accessors.
- Posted write ordering is handled when the hardware requires it.
- I/O port BARs are treated as legacy I/O resources, not MMIO.

### DMA

- DMA mask is set before allocation or mapping.
- `pci_set_master()` is called before bus-master DMA.
- Coherent and streaming DMA are used for the right purposes.
- Streaming mappings are unmapped or synchronized correctly.
- DMA is stopped before buffers are freed.
- Driver is tested with IOMMU enabled where possible.

### Interrupts And Locking

- `pci_alloc_irq_vectors()` handles MSI-X, MSI, and INTx fallback intentionally.
- IRQ numbers come from `pci_irq_vector()`.
- INTx handler supports shared interrupts.
- Multi-vector handlers protect shared state on SMP.
- Shared locks used from hard IRQ context use interrupt-safe locking where needed.
- Hardware interrupt generation is enabled after software state is ready.

### Lifecycle And Cleanup

- Probe rollback releases every successfully acquired resource.
- Remove/unbind stops hardware before freeing software state.
- Workqueues, timers, NAPI, threaded IRQs, and other deferred contexts are flushed.
- Managed and unmanaged cleanup are not double-freeing the same resource.
- Suspend/resume paths quiesce and restore hardware in a known order.
- Bind/unbind and module unload/reload are tested repeatedly.

## Interview Readiness

For interviews, aim to explain the mechanism and tradeoffs rather than reciting API names.

You should be able to answer:

- How does the PCI core match a device to a driver?
- What is the difference between config space, BAR space, and I/O port space?
- Why does a PCI DMA driver need both `dma_set_mask_and_coherent()` and `pci_set_master()`?
- How do INTx, MSI, and MSI-X differ?
- Why is `MODULE_DEVICE_TABLE(pci, ids)` needed?
- What is the safe order for `probe()` and `remove()`?
- How would you debug a device visible in `lspci` but not bound to your driver?
- How would you debug a DMA corruption or interrupt-not-firing bug?
- What can go wrong when mixing `pcim_*` helpers with manual cleanup?

See `interview/31-pci-device-drivers.md` for structured practice questions.

## Kernel Version Notes

PCI APIs have evolved, so old examples need a careful reading.

- Prefer `PCI_IRQ_INTX` in new code. Older material may use `PCI_IRQ_LEGACY`, which exists as a deprecated alias on current kernels.
- Prefer `pci_alloc_irq_vectors()` / `pci_irq_vector()` over older `pci_enable_msi()` and `pci_enable_msix_*()` APIs.
- Prefer generic DMA APIs such as `dma_set_mask_and_coherent()`, `dma_alloc_coherent()`, `dma_map_single()`, and `dma_map_sg()` over older PCI-specific DMA wrapper examples.
- Managed helpers such as `pcim_enable_device()` and `pcim_iomap_regions()` can simplify cleanup, but they change ownership rules. Always check whether a resource is devres/pcim-managed before freeing it manually.
- Modern power-management code often uses `dev_pm_ops` through the embedded generic driver, even if older examples show direct PCI driver PM fields.

