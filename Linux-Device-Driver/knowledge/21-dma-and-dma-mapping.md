# 21 - DMA And DMA Mapping

## Learning Goal
After this topic, you should be able to explain how a device accesses system memory, choose between coherent and streaming DMA, set DMA masks, map and unmap buffers safely, handle scatter-gather mappings, and use a DMAengine channel without lifetime or teardown races.

You should be able to reason about:

- CPU virtual, CPU physical, and DMA addresses;
- cache visibility and buffer ownership;
- mapping direction and error handling;
- original versus mapped scatterlist entry counts;
- DMA mapping API versus DMAengine;
- completion, timeout, cancellation, and remove ordering.

## Why This Matters In Real Work
DMA moves data between memory and hardware without making the CPU copy every byte. It is central to network, storage, audio, video, display, serial, SPI, PCI, and many SoC peripheral drivers.

DMA improves throughput and reduces CPU load, but it introduces strict contracts:

- the device must receive an address it can actually use;
- CPU caches and device writes must remain coherent;
- only the current owner may access a streaming buffer;
- mappings and descriptors must outlive the transfer;
- teardown must stop hardware and callbacks before memory disappears.

A driver can work on one machine and corrupt data on another if it assumes that a CPU pointer is a device address, that all systems are cache coherent, or that an IOMMU preserves one-to-one physical mappings.

**Production rule:** treat DMA as an ownership, address-translation, and lifetime problem, not merely as a faster `memcpy()`.

## Mental Model
DMA involves two actors sharing memory:

```text
CPU                                  Device
 |                                      |
 | CPU virtual pointer                  | DMA address
 |                                      |
 +---------- system memory -------------+
                    |
             DMA API / IOMMU
```

The CPU and device do not necessarily use the same address for the same bytes.

| Address | Used by | Example |
| --- | --- | --- |
| CPU virtual address | Kernel code | `void *cpu_addr` |
| CPU physical address | MMU/platform internals | Physical RAM address |
| DMA address | Device | `dma_addr_t dma_addr` |
| MMIO address | CPU accessing device registers | `void __iomem *regs` |

For one buffer:

```text
cpu_addr ----------------> CPU reads/writes bytes
                            same backing memory
dma_addr ----------------> Device reads/writes bytes
```

An IOMMU may translate `dma_addr` to physical pages. SWIOTLB may use a bounce buffer. Therefore:

**Never derive the device address with `virt_to_phys()` or cast a pointer into a register.**

### Mapping Versus Moving
Two kernel facilities are often confused:

| Facility | Job |
| --- | --- |
| DMA mapping API | Makes memory visible and coherent for a particular device |
| DMAengine API | Programs a DMA controller to perform a transfer |

A PCI or network device may contain its own DMA engine and only need the mapping API. A UART or audio peripheral may use a separate SoC DMA controller through DMAengine. Some subsystem cores hide part of either process.

## Core Concepts
The important decisions concern addressability, visibility, ownership, and transfer shape.

### DMA Mask
A DMA mask describes which DMA address bits the hardware can drive.

```c
ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
if (ret)
        return ret;
```

- A 32-bit device cannot consume an arbitrary 64-bit DMA address.
- The kernel may satisfy a limited mask through allocation constraints, an IOMMU, or bounce buffering.
- If mask setup fails, the driver must use a supported non-DMA fallback or reject the device.
- Some hardware has different limits for streaming payloads and coherent descriptors. Only then should separate masks be used.

### Coherent DMA
Coherent memory lets the CPU and device observe each other's writes without explicit `dma_sync_*()` ownership operations.

Typical uses:

- descriptor rings;
- command/status blocks;
- hardware mailboxes;
- small control structures shared for the device lifetime.

```c
cpu_addr = dma_alloc_coherent(dev, size, &dma_addr, GFP_KERNEL);
```

The call returns two values:

- `cpu_addr`: pointer used by kernel code;
- `dma_addr`: address programmed into the device.

**Coherent does not mean ordered.** The driver may still need barriers before setting an ownership bit or ringing a doorbell:

```c
desc->addr = payload_dma;
desc->len = payload_len;
wmb();
desc->owned_by_device = 1;
```

Use `dma_pool` when hardware needs many small coherent objects with alignment or boundary constraints.

### Streaming DMA
Streaming mapping temporarily gives a device access to an existing buffer.

Typical uses:

- network packets;
- storage requests;
- audio/video payloads;
- one-shot or queued transfers.

```c
dma_addr = dma_map_single(dev, buf, len, DMA_TO_DEVICE);
if (dma_mapping_error(dev, dma_addr))
        return -EIO;
```

The ownership rule is the center of streaming DMA:

```text
CPU prepares buffer
      |
      v
dma_map_*() or dma_sync_*_for_device()
      |
      v
DEVICE OWNS BUFFER
      |
      v
transfer completes
      |
      v
dma_unmap_*() or dma_sync_*_for_cpu()
      |
      v
CPU OWNS BUFFER
```

Do not read or modify a streaming buffer while the device owns it unless the API and hardware explicitly provide a safe protocol.

### DMA Direction
Direction is from the perspective of memory:

| Direction | Meaning | CPU action before handoff |
| --- | --- | --- |
| `DMA_TO_DEVICE` | Device reads memory | Fill the buffer |
| `DMA_FROM_DEVICE` | Device writes memory | Do not consume until CPU ownership returns |
| `DMA_BIDIRECTIONAL` | Both directions | Sync both ownership transitions |

Use the narrowest correct direction. Direction affects cache maintenance and debugging checks.

### Coherent Versus Streaming
Use the mapping whose contract matches the data.

| Property | Coherent | Streaming |
| --- | --- | --- |
| Allocation | DMA API allocates and maps | Driver supplies existing backing |
| Typical lifetime | Long-lived | Per transfer or reusable mapping |
| Explicit CPU/device sync | Normally no | Required for ownership changes |
| Common use | Rings, descriptors, mailboxes | Payload data |
| CPU/device simultaneous access | Allowed by coherence contract | Generally forbidden |
| Release | `dma_free_coherent()` | Matching `dma_unmap_*()` |

Do not choose coherent memory merely because it seems easier. It may be more constrained or expensive, and payload ownership is often clearer with streaming mappings.

### Scatter-Gather DMA
Scatter-gather describes one logical transfer using multiple memory regions.

```text
CPU-side entries:
  [page A fragment] [page B] [page C fragment]

dma_map_sg()

Device-visible segments:
  [DMA segment 0] [DMA segment 1]
```

The DMA layer may merge adjacent entries. Therefore two counts matter:

| Count | Use |
| --- | --- |
| Original `nents` passed to `dma_map_sg()` | Unmap and sync calls |
| Returned mapped count | Program and iterate hardware DMA segments |

Use `sg_dma_address()` and `sg_dma_len()` after mapping. Do not program hardware from raw page or CPU addresses.

### Suitable Buffer Backing
Common streaming inputs include suitable `kmalloc()` memory, pages, and scatterlists prepared according to the subsystem contract.

Do not assume these are valid generic single-buffer inputs:

- stack memory;
- module text/data/BSS;
- arbitrary static memory;
- an ordinary `vmalloc()` range;
- a buffer that can disappear before completion.

`GFP_DMA` does not make an arbitrary allocation universally DMA-safe. It selects a legacy memory zone and is rarely the correct solution in a modern driver.

## Kernel Mechanism
The generic DMA layer hides platform-specific translation and cache maintenance behind a device-oriented API.

```text
Driver
  |
  | dma_map_*(), dma_alloc_*
  v
Generic DMA API
  |
  +-- direct physical mapping
  +-- IOMMU/IOVA mapping
  +-- SWIOTLB bounce buffer
  +-- architecture cache maintenance
  |
  v
DMA address usable by this device
```

### Why `struct device *` Matters
Every mapping is associated with the device performing DMA.

The device identifies:

- DMA masks;
- IOMMU domain;
- cache-coherency properties;
- bounce-buffer constraints;
- segment and boundary limits;
- DMA debug ownership.

Passing `NULL`, a parent selected by guesswork, or a different device can produce invalid mappings even when the code appears to work on a simple platform.

### Cache Ownership
On a non-coherent system, the CPU cache may contain data different from RAM.

- Before `DMA_TO_DEVICE`, dirty CPU cache lines may need to be written back.
- After `DMA_FROM_DEVICE`, stale CPU cache lines may need invalidation.
- `dma_map_*()`, `dma_unmap_*()`, and `dma_sync_*()` provide the required architecture-specific operations.

On a coherent system, explicit cache maintenance may be unnecessary, but the same API and ownership rules preserve portability.

### IOMMU And SG Merging
An IOMMU translates device-visible I/O virtual addresses into physical pages.

Consequences:

- `dma_addr_t` may not equal the physical address;
- physically scattered pages can appear contiguous to the device;
- several input SG entries may become one DMA segment;
- an invalid device access can produce an IOMMU fault rather than silent RAM corruption.

### DMAengine Client Flow
DMAengine represents a DMA controller and its channels.

```text
Consumer device
  |
  | dma_request_chan(dev, "rx")
  v
struct dma_chan
  |
  | dmaengine_slave_config()
  v
channel configured for FIFO/bus width/burst
  |
  | dmaengine_prep_*
  v
struct dma_async_tx_descriptor
  |
  | dmaengine_submit()
  v
pending queue
  |
  | dma_async_issue_pending()
  v
hardware transfer -> callback/completion
```

For slave DMA, `struct dma_slave_config` commonly describes:

- peripheral FIFO source or destination address;
- bus width;
- maximum burst;
- controller-specific options.

The payload must also be mapped using the correct DMA device. For a DMAengine channel, current code can obtain that device with `dmaengine_get_dma_device(chan)` when required by the mapping contract.

## Key Structs And APIs
Use these APIs as parts of a lifecycle, not as isolated names.

### Addressing And Capability
| API/type | Purpose |
| --- | --- |
| `dma_addr_t` | Device-visible DMA address type |
| `DMA_BIT_MASK(n)` | Construct an address-width mask |
| `dma_set_mask_and_coherent()` | Set matching streaming and coherent masks |
| `dma_set_mask()` | Set streaming mask |
| `dma_set_coherent_mask()` | Set coherent allocation mask |
| `dma_max_mapping_size()` | Query maximum mapping size for a device |

### Coherent Allocation
| API/type | Purpose |
| --- | --- |
| `dma_alloc_coherent()` | Allocate and map coherent memory |
| `dma_free_coherent()` | Free matching coherent allocation |
| `dmam_alloc_coherent()` | Device-managed coherent allocation |
| `struct dma_pool` | Pool of small aligned coherent objects |
| `dma_pool_create()` | Create a coherent object pool |
| `dma_pool_alloc()` / `dma_pool_free()` | Allocate/free pool objects |

Managed coherent allocation simplifies release, but remove must still stop hardware and callbacks before devres frees memory.

### Streaming Mapping
| API | Purpose |
| --- | --- |
| `dma_map_single()` | Map suitable contiguous CPU virtual memory |
| `dma_map_page()` | Map a page range |
| `dma_map_sg()` | Map a scatterlist |
| `dma_mapping_error()` | Check single/page mapping failure |
| `dma_unmap_single()` / `dma_unmap_page()` | Release matching mapping |
| `dma_unmap_sg()` | Release SG mapping using original `nents` |
| `dma_sync_*_for_cpu()` | Return a reusable mapping to CPU ownership |
| `dma_sync_*_for_device()` | Return it to device ownership |

The device, size, direction, and relevant counts must match the mapping operation.

### Scatter-Gather
| API/type | Purpose |
| --- | --- |
| `struct scatterlist` | One CPU-side buffer segment |
| `struct sg_table` | Scatterlist plus entry counts |
| `sg_init_table()` | Initialize a fixed scatterlist |
| `sg_set_buf()` / `sg_set_page()` | Describe backing memory |
| `for_each_sg()` | Iterate entries |
| `sg_dma_address()` | Read mapped device address |
| `sg_dma_len()` | Read mapped device length |

### DMAengine
| API/type | Purpose |
| --- | --- |
| `struct dma_chan` | Acquired DMA channel |
| `struct dma_slave_config` | Peripheral address, width, and burst configuration |
| `struct dma_async_tx_descriptor` | Prepared transfer descriptor |
| `dma_request_chan()` | Request a named firmware-associated channel |
| `dmaengine_slave_config()` | Configure slave DMA |
| `dmaengine_prep_slave_sg()` | Prepare peripheral SG transfer |
| `dmaengine_prep_dma_cyclic()` | Prepare cyclic audio-style transfer |
| `dmaengine_prep_dma_memcpy()` | Prepare supported memory copy |
| `dmaengine_submit()` | Queue descriptor and return cookie |
| `dma_submit_error()` | Check submit failure |
| `dma_async_issue_pending()` | Start queued work |
| `dmaengine_terminate_sync()` | Stop and synchronize in sleepable context |
| `dmaengine_terminate_async()` | Initiate atomic-safe termination |
| `dmaengine_synchronize()` | Wait after asynchronous termination |
| `dma_release_channel()` | Release channel ownership |

### Completion
| API/type | Purpose |
| --- | --- |
| `struct completion` | One-shot event synchronization |
| `init_completion()` | Initialize |
| `reinit_completion()` | Reuse after proving no stale signaler exists |
| `wait_for_completion_timeout()` | Sleep with bounded wait |
| `complete()` | Signal from callback or IRQ-safe context |

## Lifecycle / Data Flow
Correct DMA code is mostly correct ordering.

### Probe
```text
validate hardware address width
  -> dma_set_mask_and_coherent()
  -> request named DMA channel if needed
  -> allocate coherent rings/control state
  -> initialize completions, locks, queues
  -> configure hardware but leave it stopped
```

### One Streaming Transaction
```text
obtain stable buffer
  -> fill data for DMA_TO_DEVICE
  -> dma_map_single()
  -> check dma_mapping_error()
  -> program DMA address and length
  -> start hardware
  -> wait for IRQ/callback with timeout
  -> stop/acknowledge hardware
  -> dma_unmap_single()
  -> CPU may use/free buffer
```

### Reusable Streaming Mapping
```text
dma_map_*()
  -> device transfer
  -> dma_sync_*_for_cpu()
  -> CPU reads/modifies
  -> dma_sync_*_for_device()
  -> next device transfer
  -> final dma_unmap_*()
```

### Remove Or Timeout
```text
block new submissions
  -> disable peripheral DMA requests/IRQs
  -> stop peripheral
  -> dmaengine_terminate_sync()
     OR terminate_async() + synchronize()
  -> unmap outstanding mappings
  -> return queued buffers with error
  -> free coherent/payload memory
  -> release channel
```

**Production rule:** never free a buffer, descriptor, completion object, or callback context until DMA and all completion callbacks are synchronized.

## Minimal Practical Example
This learning-only pseudo-driver fragment demonstrates one streaming `DMA_TO_DEVICE` transfer. It is not production-ready because real register definitions, hardware reset, residue/error reporting, concurrency control, and device-specific ordering are omitted.

```c
struct mydev {
        struct device *dev;
        void __iomem *regs;
        struct completion tx_done;
};

static irqreturn_t my_irq(int irq, void *data)
{
        struct mydev *d = data;
        u32 status = readl(d->regs + STATUS_REG);

        if (!(status & TX_DONE))
                return IRQ_NONE;

        writel(TX_DONE, d->regs + STATUS_REG);
        complete(&d->tx_done);
        return IRQ_HANDLED;
}

static int my_tx(struct mydev *d, void *buf, size_t len)
{
        dma_addr_t dma;
        unsigned long left;
        int ret = 0;

        if (!len || len > dma_max_mapping_size(d->dev))
                return -EINVAL;

        reinit_completion(&d->tx_done);

        dma = dma_map_single(d->dev, buf, len, DMA_TO_DEVICE);
        if (dma_mapping_error(d->dev, dma))
                return -EIO;

        /* Hardware-specific code must validate its address register width. */
        writeq(dma, d->regs + TX_ADDR_REG);
        writel(len, d->regs + TX_LEN_REG);
        writel(TX_START | TX_IRQ_ENABLE, d->regs + CTRL_REG);

        left = wait_for_completion_timeout(&d->tx_done,
                                           msecs_to_jiffies(1000));
        if (!left) {
                writel(TX_STOP, d->regs + CTRL_REG);
                ret = -ETIMEDOUT;
        }

        /*
         * Real hardware may require reset/polling to prove DMA has stopped
         * before unmapping after a timeout.
         */
        dma_unmap_single(d->dev, dma, len, DMA_TO_DEVICE);
        return ret;
}
```

Important lines:

- `dma_map_single()` uses the device that performs DMA.
- `dma_mapping_error()` is mandatory.
- The device receives `dma`, never the CPU pointer.
- The timeout prevents an infinite sleep.
- Unmapping returns ownership and releases DMA address-space resources.
- A real timeout path must prove the engine has stopped before unmapping.

### Scatter-Gather Count Pattern
The following pattern captures the count rule:

```c
mapped_nents = dma_map_sg(dev, sgl, orig_nents, DMA_FROM_DEVICE);
if (!mapped_nents)
        return -EIO;

for_each_sg(sgl, sg, mapped_nents, i)
        program_segment(i, sg_dma_address(sg), sg_dma_len(sg));

start_dma();
wait_for_dma_to_stop();

dma_unmap_sg(dev, sgl, orig_nents, DMA_FROM_DEVICE);
```

Program `mapped_nents`; unmap with `orig_nents`.

## Common Bugs And Debugging
Start from the symptom, then test address, ownership, ordering, and lifetime hypotheses.

### Transfer Times Out
Likely causes:

- descriptor was submitted but `dma_async_issue_pending()` was not called;
- peripheral DMA request generation was never enabled;
- wrong FIFO address, direction, width, or burst;
- interrupt is masked, misrouted, or not acknowledged;
- the hardware rejected a truncated or misaligned DMA address;
- a previous timeout left the engine wedged.

Inspect:

- peripheral and DMA-controller status registers;
- IRQ counters and handler logs;
- descriptor cookie and queue state;
- address width and programmed high/low registers;
- start ordering between peripheral and DMAengine.

Fix:

- define explicit start, timeout, terminate, reset, and retry policy;
- log enough state before resetting hardware;
- use a bounded wait rather than polling forever.

### Data Is Stale Or Corrupted
Likely causes:

- CPU accessed a streaming buffer while the device owned it;
- wrong DMA direction;
- missing `dma_sync_*()` on a reused mapping;
- missing barrier before publishing a coherent descriptor;
- buffer length or segment boundary is wrong;
- device wrote beyond the mapped region.

Inspect:

- ownership transitions;
- mapping and sync direction;
- descriptor fields and barrier placement;
- cache-line alignment on non-coherent targets;
- IOMMU fault logs and hardware-reported length.

Fix:

- make ownership transitions explicit in code;
- use the narrowest correct direction;
- add the correct DMA sync operation or memory barrier, not a random delay.

### DMA API Warning
Typical reports include:

- mapped as single, unmapped as page;
- wrong size or direction at unmap;
- mapping used with the wrong device;
- mapping never released;
- driver failed to check a mapping error.

Development workflow:

```text
Enable CONFIG_DMA_API_DEBUG
mount debugfs
inspect dmesg
inspect /sys/kernel/debug/dma-api/
```

Useful debugfs entries can include:

- `dump`;
- `error_count`;
- `driver_filter`;
- `num_free_entries`;
- `min_free_entries`.

DMA API debugging has overhead and is intended for development kernels.

### IOMMU/SMMU Fault
Likely causes:

- hardware used an unmapped or stale DMA address;
- transfer outlived the mapping;
- direction/permission is wrong;
- descriptor address was corrupted or truncated;
- the wrong device performed the mapping.

Record:

- faulting device;
- IOVA/DMA address;
- read versus write access;
- descriptor and buffer lifetime;
- whether remove, timeout, or suspend raced with DMA.

### Scatter-Gather Failure
Likely causes:

- zero return from `dma_map_sg()` ignored;
- hardware programmed from raw SG entries;
- driver assumes mapped count equals original count;
- segment length/count exceeds hardware limits;
- unmap uses mapped count instead of original count.

**Interview trap:** `dma_map_sg()` may return fewer entries because mappings were merged. This is success, not partial failure.

### Remove Or Unbind Crashes
Likely cause: managed memory was released while the DMA engine, IRQ, or completion callback still referenced it.

Correct teardown:

1. reject new submissions;
2. stop peripheral requests and interrupts;
3. terminate DMA;
4. synchronize callbacks;
5. unmap/free buffers;
6. release channel and remaining resources.

Devres does not provide steps 1-4 automatically.

## Production Checklist
Before review or hardware bring-up, verify:

- [ ] The correct `struct device *` is used for every DMA operation.
- [ ] Streaming and coherent masks match the real hardware address widths.
- [ ] Every mask-setting failure has a clear fallback or probe failure.
- [ ] Hardware registers can represent every DMA address the API may return.
- [ ] Coherent memory is used for the right workload, not as a universal shortcut.
- [ ] Descriptor publication uses the required memory barriers.
- [ ] Every `dma_map_single()`/`dma_map_page()` checks `dma_mapping_error()`.
- [ ] Every `dma_map_sg()` checks for zero.
- [ ] Every successful map has one matching unmap with the same device, size, direction, and original count.
- [ ] Hardware uses mapped SG addresses and the returned mapped segment count.
- [ ] Reused mappings have explicit CPU/device sync transitions.
- [ ] No stack, temporary, arbitrary static, or direct `vmalloc()` buffer is used through an unsupported path.
- [ ] Buffer, descriptor, and callback lifetimes extend through completion or synchronized termination.
- [ ] Timeout paths stop or reset hardware before unmapping.
- [ ] Remove, suspend, and probe-failure paths quiesce DMA before memory release.
- [ ] Segment count, length, alignment, boundary, and burst limits are validated.
- [ ] DMA addresses are logged and programmed without truncation.
- [ ] DMA API debug and IOMMU-enabled testing have been used where available.
- [ ] `dmatest` results are not mistaken for validation of a peripheral-specific DMA path.

## Interview Readiness
You should be able to explain, without relying on slogans:

- why a CPU pointer or physical address is not necessarily a DMA address;
- what a DMA mask controls;
- how coherent and streaming mappings differ;
- why coherent memory can still require barriers;
- how direction changes cache ownership rules;
- why `dma_map_sg()` has both original and mapped entry counts;
- how DMA mapping differs from DMAengine;
- how a transfer moves through map, submit, issue, complete, sync/unmap, and free;
- how to stop DMA safely during timeout, remove, suspend, or error unwind;
- how to diagnose stale data, IOMMU faults, mapping leaks, and teardown races.

Practice these in [the Topic 21 interview guide](../interview/21-dma-and-dma-mapping.md).

## Kernel Version Notes
- Prefer generic `dma_*()` APIs over legacy PCI-specific mapping wrappers.
- Named DMAengine consumers should normally use `dma_request_chan()`, which returns an error pointer.
- Use `dmaengine_terminate_sync()` in sleepable teardown. If atomic context requires `dmaengine_terminate_async()`, call `dmaengine_synchronize()` later before freeing transfer or callback state.
- DMAengine helper availability, descriptor metadata, tracepoints, and DT provider cells vary by kernel and controller. Validate against the target kernel headers and binding YAML.
- Treat old claims such as "single mappings are limited to one page," "SG entries must be page-sized," or "coherent memory is always uncached" as non-portable.
