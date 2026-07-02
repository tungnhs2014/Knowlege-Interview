# 31 - PCI Device Drivers Example

## Status

This example is **learning-only**. It does not bind to or control real hardware.

The goal is to practice the real PCI debug workflow safely:

- Find PCI devices by BDF.
- Inspect IDs, driver binding, BAR resources, modalias, and interrupt mode.
- Connect what userspace shows to the PCI driver lifecycle.
- Understand what a minimal PCI `probe()` would do without loading a fake driver into a live system.

## Kernel Version Assumptions

- Tested conceptually against modern Linux 6.x PCI behavior.
- Commands use common Linux userspace tools: `lspci`, `modinfo`, `readlink`, `cat`, and `dmesg`.
- Current drivers should prefer:
  - `PCI_IRQ_INTX` over older `PCI_IRQ_LEGACY`.
  - `pci_alloc_irq_vectors()` over old `pci_enable_msi()` / `pci_enable_msix_*()` APIs.
  - Generic DMA APIs such as `dma_set_mask_and_coherent()`, `dma_alloc_coherent()`, and `dma_map_*()`.

## Files

```text
examples/31-pci-device-drivers/
  README.md
```

No standalone kernel module is included because a useful PCI module normally needs real matching hardware IDs and a hardware manual. A fake PCI driver that never probes teaches less than inspecting actual PCI devices already present on the system.

## Build Command

No build is required.

```sh
# No-op: README-only learning lab.
```

## Load / Unload Commands

No module is loaded or unloaded by this example.

Avoid writing to these sysfs files unless you intentionally want to unbind or reprobe a real device:

```text
/sys/bus/pci/drivers/<driver>/unbind
/sys/bus/pci/drivers/<driver>/bind
/sys/bus/pci/devices/<BDF>/remove
/sys/bus/pci/rescan
/sys/bus/pci/devices/<BDF>/config
```

Those operations can disrupt storage, networking, display, USB host controllers, or other active hardware.

## Test Commands

### 1. List PCI Devices

```sh
lspci -nn
```

Expected output shape:

```text
00:14.0 USB controller [0c03]: Intel Corporation Device [8086:7ae0]
01:00.0 Ethernet controller [0200]: Intel Corporation I210 Gigabit Network Connection [8086:1533]
02:00.0 Non-Volatile memory controller [0108]: Samsung Electronics Co Ltd NVMe SSD Controller [144d:a80a]
```

What to notice:

- `01:00.0` is the BDF without the PCI domain.
- `[0200]` is the class code.
- `[8086:1533]` is vendor ID and device ID.

### 2. Pick One Device And Inspect Driver Binding

Replace `01:00.0` with a BDF from your machine:

```sh
lspci -nn -k -s 01:00.0
```

Expected output shape:

```text
01:00.0 Ethernet controller [0200]: Intel Corporation I210 Gigabit Network Connection [8086:1533]
	Subsystem: Intel Corporation Device [8086:0000]
	Kernel driver in use: igb
	Kernel modules: igb
```

Connection to driver code:

- `Kernel driver in use` means a PCI driver matched and bound.
- The driver's `struct pci_device_id` table matched the device identity.
- `MODULE_DEVICE_TABLE(pci, ids)` is what lets userspace autoload matching modules.

### 3. Inspect The Sysfs Device Directory

Most systems include the PCI domain in sysfs. Convert `01:00.0` to `0000:01:00.0`:

```sh
BDF=0000:01:00.0
ls -l /sys/bus/pci/devices/$BDF
```

Useful files:

```sh
cat /sys/bus/pci/devices/$BDF/vendor
cat /sys/bus/pci/devices/$BDF/device
cat /sys/bus/pci/devices/$BDF/class
cat /sys/bus/pci/devices/$BDF/modalias
readlink /sys/bus/pci/devices/$BDF/driver
```

Expected output shape:

```text
0x8086
0x1533
0x020000
pci:v00008086d00001533sv00008086sd00000000bc02sc00i00
../../../bus/pci/drivers/igb
```

What to notice:

- `vendor`, `device`, and `class` are read from PCI configuration space.
- `modalias` is the string userspace can match against module aliases.
- `driver` exists only when a driver is bound.

### 4. Compare Modalias With Module Aliases

Use the module name from `lspci -k`:

```sh
modinfo igb | grep '^alias:' | head
```

Expected output shape:

```text
alias:          pci:v00008086d00001533sv*sd*bc*sc*i*
alias:          pci:v00008086d000010D3sv*sd*bc*sc*i*
```

Connection to driver code:

```c
static const struct pci_device_id ids[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_INTEL, 0x1533) },
	{ }
};
MODULE_DEVICE_TABLE(pci, ids);
```

The exact production driver may use many IDs and macros, but the idea is the same: the ID table becomes module alias data.

### 5. Inspect BAR Resources

```sh
cat /sys/bus/pci/devices/$BDF/resource
```

Expected output shape:

```text
0x00000000f7c00000 0x00000000f7c1ffff 0x0000000000140204
0x00000000f7c20000 0x00000000f7c23fff 0x0000000000140204
0x0000000000000000 0x0000000000000000 0x0000000000000000
...
```

What to notice:

- Each line corresponds to a PCI resource, usually a BAR.
- Start/end values show assigned address ranges.
- Flags describe the resource type and attributes.

Driver-side equivalent:

```c
resource_size_t start = pci_resource_start(pdev, bar);
resource_size_t len = pci_resource_len(pdev, bar);
unsigned long flags = pci_resource_flags(pdev, bar);
```

A real driver then requests and maps the BAR with helpers such as:

```c
ret = pcim_iomap_regions(pdev, BIT(bar), "my_pci_driver");
regs = pcim_iomap_table(pdev)[bar];
```

### 6. Inspect Interrupt Mode

```sh
lspci -vv -s 01:00.0 | grep -i -A8 'msi'
cat /proc/interrupts | grep -E 'igb|01:00.0|PCI-MSI'
```

Expected output shape:

```text
Capabilities: [70] MSI-X: Enable+ Count=5 Masked-
...
 124:          0          8   PCI-MSI 524288-edge      igb
 125:          2          0   PCI-MSI 524289-edge      igb-TxRx-0
```

What to notice:

- `MSI-X: Enable+` means MSI-X is active.
- `/proc/interrupts` shows whether vectors are firing.
- Multi-vector devices can run handlers on different CPUs, so shared state needs careful locking.

Driver-side equivalent:

```c
ret = pci_alloc_irq_vectors(pdev, 1, max_vecs, PCI_IRQ_ALL_TYPES);
irq = pci_irq_vector(pdev, 0);
ret = devm_request_irq(&pdev->dev, irq, handler, 0, "my_pci", priv);
```

### 7. Read Recent PCI Logs

```sh
dmesg | grep -i -E 'pci|msi|iommu|dma' | tail -80
```

Useful clues:

- Device enumeration and BDF.
- BAR assignment failures.
- Driver probe failures.
- MSI/MSI-X allocation messages.
- IOMMU or DMA mapping faults.

## Minimal Probe Skeleton

This is not a complete module. It is a lifecycle sketch that connects the inspection lab to driver code.

```c
static int my_probe(struct pci_dev *pdev, const struct pci_device_id *id)
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

	ret = pcim_iomap_regions(pdev, BIT(0), "my_pci");
	if (ret)
		return ret;

	regs = pcim_iomap_table(pdev)[0];

	ret = pci_alloc_irq_vectors(pdev, 1, 1, PCI_IRQ_ALL_TYPES);
	if (ret < 0)
		return ret;

	irq = pci_irq_vector(pdev, 0);
	ret = devm_request_irq(&pdev->dev, irq, my_irq, 0, "my_pci", pdev);
	if (ret)
		return ret;

	/* Initialize software state, then enable device DMA/interrupts. */
	return 0;
}
```

What production code would add:

- A real `struct pci_device_id` table for actual hardware.
- Hardware register definitions from a datasheet.
- Private driver state and `pci_set_drvdata()`.
- Correct interrupt acknowledgement logic.
- DMA ring or buffer management.
- Upper subsystem registration, such as netdev, block, V4L2, ALSA, or misc.
- Suspend/resume and reset/error handling.
- A clear policy for managed vs manual cleanup.

## Expected Logs

This README-only lab does not create kernel logs by itself.

When inspecting existing devices, expected logs may include lines like:

```text
pci 0000:01:00.0: enabling device (0000 -> 0002)
igb 0000:01:00.0: Intel(R) Gigabit Ethernet Network Connection
igb 0000:01:00.0: Using MSI-X interrupts
```

Exact logs depend on hardware, driver, kernel configuration, and boot options.

## Cleanup And Error-Path Explanation

Because this lab is read-only, cleanup is simply:

```sh
unset BDF
```

For a real PCI driver, cleanup is a safety problem, not just neatness:

1. Stop userspace/subsystem entry points from creating new work.
2. Disable hardware interrupt generation.
3. Stop DMA engines and wait for in-flight DMA to complete or abort.
4. Flush timers, workqueues, NAPI, threaded IRQ work, or other deferred contexts.
5. Free IRQ handlers and vectors if they are not managed.
6. Unregister upper subsystem objects.
7. Unmap/free DMA resources.
8. Release BAR mappings and regions if they are not managed.
9. Clear bus mastering and disable the PCI device if cleanup is manual.

The central rule is: **hardware must not be able to interrupt or DMA into memory after the driver has freed the state or buffers it uses.**

## Userspace ABI Impact

This example creates no new userspace ABI.

It only observes existing kernel ABI and debug surfaces:

- `/sys/bus/pci/devices/<BDF>/`
- `/sys/bus/pci/drivers/`
- `/proc/interrupts`
- `lspci` output based on PCI config space and sysfs
- `dmesg` kernel logs

A real PCI function driver may expose userspace ABI indirectly through another subsystem:

- Network interface: `ip link`, `ethtool`, sysfs, rtnetlink.
- Block/NVMe device: `/dev/nvme*`, sysfs, block queue attributes.
- V4L2 capture card: `/dev/video*`, media controller nodes.
- Sound device: ALSA PCM/control nodes.
- Misc/char driver: `/dev/<name>`, ioctl/sysfs/debugfs depending on design.

## Why This Is Not Production-Ready

This example is intentionally not production-ready because it does not control hardware.

A production PCI driver needs:

- A hardware manual and tested register access.
- Correct BAR selection and register-width handling.
- Real interrupt status checking and acknowledgement.
- DMA mask, descriptor, and buffer handling validated with IOMMU enabled.
- Robust probe rollback and remove ordering.
- Power-management and reset behavior.
- Subsystem-specific ABI and lifetime rules.
- Testing across bind/unbind, module reload, suspend/resume, and interrupt modes.

