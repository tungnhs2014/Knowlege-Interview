# 21 - DMA And DMA Mapping Interview Questions

Strong candidates reason about address domains, cache visibility, ownership, mapping lifetime, hardware limits, and teardown. They do not reduce DMA to "hardware copies memory without the CPU," and they do not rely on assumptions that happen to work on coherent x86 systems.

## Beginner Questions

### 1. What is DMA, and why do drivers use it?

**Level:** Beginner

**Question:** What is Direct Memory Access, and what work does the CPU still perform?

**Short Answer:** DMA lets hardware transfer data between a device and memory without the CPU copying each byte. The CPU still allocates and maps buffers, programs the transfer, handles completion and errors, and manages ownership and lifetime.

**Deep Explanation:** DMA reduces CPU copy work and improves throughput for workloads such as network packets, storage requests, audio streams, and video frames. It does not make the transfer autonomous from software. The driver must provide valid device-visible addresses, lengths, direction, and hardware configuration. It must also ensure that CPU caches, the device, and any IOMMU agree about the memory and that the buffer remains alive until the transfer has stopped.

DMA can be implemented by:

- a device's own bus-mastering engine, as with many PCI devices;
- a separate SoC DMA controller used through DMAengine;
- a subsystem framework that hides part of the mapping or queueing work.

**API / Code Anchor:**
```text
allocate or obtain buffer
  -> map for the device
  -> program address and length
  -> start transfer
  -> receive IRQ/callback
  -> sync or unmap for CPU
  -> process/recycle/free
```

**Production or Debugging Angle:** A timeout is not solved merely by increasing the wait. Inspect whether the address, direction, peripheral request, descriptor submission, issue-pending call, and interrupt path were all correct.

**Common Traps:**
- Saying the CPU is not involved at all.
- Treating DMA as only a memory-to-memory copy mechanism.
- Ignoring cache ownership and completion.
- Freeing the buffer when the submit function returns.

**Follow-up Questions:**
- What is the difference between a bus-mastering device and DMAengine?
- What must a timeout path do before freeing the buffer?
- Which subsystems commonly use cyclic DMA?

### 2. Compare CPU virtual, physical, and DMA addresses.

**Level:** Beginner

**Question:** Why can a driver not simply give a kernel pointer or `virt_to_phys()` result to hardware?

**Short Answer:** A kernel pointer is for CPU access, a physical address identifies RAM in the CPU's physical address space, and a DMA address is the device-visible address produced for a specific device. An IOMMU or bounce layer may make all three values different.

**Deep Explanation:** Kernel code dereferences a CPU virtual address. The MMU translates that address for CPU access. A device performs transactions in its DMA address space, which may be translated by an IOMMU into one or more physical pages. A device can also have a narrower address width than the CPU. The generic DMA API uses the `struct device` to account for these constraints and returns a `dma_addr_t` that is valid for that device.

`virt_to_phys()` bypasses:

- the device DMA mask;
- IOMMU translation;
- SWIOTLB bounce buffering;
- cache maintenance;
- DMA API debugging and ownership tracking.

**API / Code Anchor:**
```c
dma_addr_t dma;

dma = dma_map_single(dev, buf, len, DMA_TO_DEVICE);
if (dma_mapping_error(dev, dma))
        return -EIO;

/* Program dma, not buf and not virt_to_phys(buf). */
```

**Production or Debugging Angle:** When logging a failure, print the CPU pointer and DMA address as separate values. Verify that the hardware register width can hold the returned DMA address.

**Common Traps:**
- Calling `dma_addr_t` a physical address.
- Casting a pointer to `u32`.
- Assuming an identity mapping because it worked without an IOMMU.
- Using the wrong `struct device` for mapping.

**Follow-up Questions:**
- How does an IOMMU help a device?
- What is an IOVA?
- Why can a 32-bit device work in a machine with memory above 4 GiB?

### 3. What is the difference between coherent and streaming DMA?

**Level:** Beginner

**Question:** When would you choose coherent memory instead of a streaming mapping?

**Short Answer:** Coherent DMA is commonly used for long-lived shared control structures whose CPU and device writes must remain mutually visible. Streaming DMA maps payload buffers and transfers ownership between CPU and device for each transfer or reuse cycle.

**Deep Explanation:** `dma_alloc_coherent()` allocates memory and returns both a CPU pointer and a DMA address. The coherence contract avoids explicit `dma_sync_*()` calls for visibility, which is useful for descriptor rings and mailboxes. Streaming APIs such as `dma_map_single()` work with existing suitable backing memory. Once mapped or synced for the device, the CPU should not access the buffer until it is unmapped or synced for the CPU.

| Property | Coherent | Streaming |
| --- | --- | --- |
| Typical data | Descriptors/control | Payload |
| Existing backing required | No | Yes |
| Ownership sync calls | Normally no | Yes for reused mappings |
| Typical lifetime | Long | Transaction-oriented |

**API / Code Anchor:**
```c
ring = dma_alloc_coherent(dev, ring_size, &ring_dma, GFP_KERNEL);

payload_dma = dma_map_single(dev, payload, len, DMA_TO_DEVICE);
if (dma_mapping_error(dev, payload_dma))
        /* handle failure */;
```

**Production or Debugging Angle:** If data is stale only on non-coherent targets, inspect streaming ownership transitions. If descriptors are partially visible, inspect memory barriers even when the ring is coherent.

**Common Traps:**
- Saying coherent memory is always uncached.
- Using coherent allocation for every payload because it is easier.
- Assuming coherent means no barriers are ever needed.
- Accessing a streaming buffer while the device owns it.

**Follow-up Questions:**
- Why can coherent memory still need `wmb()`?
- What is a good use for `dma_pool`?
- How do you reuse a streaming mapping?

### 4. What do DMA direction values mean?

**Level:** Beginner

**Question:** Explain `DMA_TO_DEVICE`, `DMA_FROM_DEVICE`, and `DMA_BIDIRECTIONAL`.

**Short Answer:** Direction is described from the memory perspective: the device reads memory for `DMA_TO_DEVICE`, writes memory for `DMA_FROM_DEVICE`, and may do both for `DMA_BIDIRECTIONAL`.

**Deep Explanation:** Direction tells the DMA layer what cache maintenance and access rules are needed. For `DMA_TO_DEVICE`, the CPU prepares data before handing it to the device. For `DMA_FROM_DEVICE`, the CPU waits until ownership returns before reading device-produced data. `DMA_BIDIRECTIONAL` handles both but may require more work and should not be used to avoid understanding the actual transfer.

**API / Code Anchor:**
```c
/* Transmit buffer: device reads bytes prepared by CPU. */
dma = dma_map_single(dev, tx_buf, len, DMA_TO_DEVICE);

/* Receive buffer: device writes bytes later consumed by CPU. */
dma = dma_map_single(dev, rx_buf, len, DMA_FROM_DEVICE);
```

**Production or Debugging Angle:** A wrong direction can produce stale or corrupted data only on some architectures, making it a classic portability bug. DMA API debug can also detect mismatched directions at unmap.

**Common Traps:**
- Describing direction from the peripheral's naming convention rather than memory.
- Always choosing `DMA_BIDIRECTIONAL`.
- Unmapping with a different direction.
- Reading an RX buffer before ownership returns.

**Follow-up Questions:**
- When are sync calls needed for a reused mapping?
- What direction does a network TX packet use?
- What direction does a camera capture buffer use?

## Mid-Level Questions

### 5. How do DMA masks work?

**Level:** Mid-Level

**Question:** What should a driver do during probe if the device supports only 32-bit DMA addresses?

**Short Answer:** Call `dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32))`, check the return value, and fail probe or use a supported non-DMA fallback if the platform cannot satisfy the requirement.

**Deep Explanation:** A DMA mask tells the kernel which DMA address bits the device can generate. It is not merely documentation: it affects allocation and mapping. The coherent and streaming masks are usually identical, so `dma_set_mask_and_coherent()` is the normal helper. Some hardware uses narrower coherent descriptor addresses than payload addresses; such devices may need separate mask setup.

Mask setup success does not remove the need to verify the device's descriptor and register formats. A driver must not truncate a returned 64-bit DMA address into a 32-bit register.

**API / Code Anchor:**
```c
ret = dma_set_mask_and_coherent(dev, DMA_BIT_MASK(32));
if (ret) {
        dev_err(dev, "32-bit DMA is unavailable\n");
        return ret;
}
```

**Production or Debugging Angle:** If the device works on low-memory systems but fails with more RAM or an enabled IOMMU, inspect whether masks were set and whether address high bits are programmed correctly.

**Common Traps:**
- Ignoring the mask-setting return value.
- Assuming a 64-bit CPU implies a 64-bit device.
- Setting a 64-bit mask because the software type is `dma_addr_t`.
- Using `GFP_DMA` instead of configuring the device mask.

**Follow-up Questions:**
- When would streaming and coherent masks differ?
- What can the kernel do when a device has a narrow mask?
- How would you audit descriptor address truncation?

### 6. Show the correct lifecycle for `dma_map_single()`.

**Level:** Mid-Level

**Question:** What checks and matching parameters are required?

**Short Answer:** Prepare a stable suitable buffer, map it with the correct device and direction, check `dma_mapping_error()`, let hardware finish, then unmap using the same device, DMA address, size, and direction.

**Deep Explanation:** A successful mapping consumes DMA address-space resources and transfers ownership to the device. The CPU must not free or repurpose the backing while the mapping is active and hardware can access it. For `DMA_TO_DEVICE`, data is prepared before mapping. For `DMA_FROM_DEVICE`, data is consumed only after unmap or sync for CPU.

Every successful mapping needs exactly one matching unmap unless an API explicitly transfers responsibility elsewhere.

**API / Code Anchor:**
```c
dma = dma_map_single(dev, buf, len, DMA_TO_DEVICE);
if (dma_mapping_error(dev, dma))
        return -EIO;

program_device(dma, len);
ret = wait_until_stopped();

dma_unmap_single(dev, dma, len, DMA_TO_DEVICE);
return ret;
```

**Production or Debugging Angle:** Enable `CONFIG_DMA_API_DEBUG` to catch wrong unmap APIs, sizes, devices, directions, and leaked mappings.

**Common Traps:**
- Testing the DMA address against zero instead of using `dma_mapping_error()`.
- Mapping with `NULL`.
- Unmapping before proving hardware stopped.
- Forgetting to unmap on timeout or submit failure after a successful map.

**Follow-up Questions:**
- How does a reusable mapping differ?
- What if the mapping size exceeds `dma_max_mapping_size()`?
- Can the CPU inspect the buffer while DMA is active?

### 7. How are long-lived streaming mappings reused?

**Level:** Mid-Level

**Question:** A ring keeps payload buffers mapped for many transfers. How does ownership move safely?

**Short Answer:** Map once, use `dma_sync_*_for_cpu()` before CPU access and `dma_sync_*_for_device()` before returning the buffer to hardware, then unmap once when the mapping is permanently retired.

**Deep Explanation:** Unmapping after every transfer is not always desirable. A driver may retain a streaming mapping, but mapping lifetime and access ownership remain different concepts. After device completion, sync for CPU before reading or modifying data. After CPU work, sync for device before making the descriptor available again. The sync calls use the same device, direction, and appropriate range/count as the original mapping.

**API / Code Anchor:**
```c
dma_sync_single_for_cpu(dev, dma, len, DMA_FROM_DEVICE);
consume_packet(buf);

prepare_for_next_receive(buf);
dma_sync_single_for_device(dev, dma, len, DMA_FROM_DEVICE);
publish_descriptor();
```

**Production or Debugging Angle:** This bug may be invisible on cache-coherent development machines. Test on a non-coherent architecture or use a platform configuration representative of production.

**Common Traps:**
- Treating "still mapped" as "safe for simultaneous CPU access."
- Calling only the CPU sync and forgetting to return ownership.
- Changing direction between sync calls.
- Sharing cache lines between CPU-owned and device-owned data.

**Follow-up Questions:**
- Can partial ranges be synchronized?
- Why can false sharing matter on non-coherent systems?
- When is unmap-per-transfer preferable?

### 8. Explain the two scatter-gather entry counts.

**Level:** Mid-Level

**Question:** Why can `dma_map_sg()` return fewer entries than were passed, and which count is used where?

**Short Answer:** The DMA layer may merge adjacent entries into fewer device-visible segments. Program hardware using the returned mapped count, but unmap and sync using the original input count.

**Deep Explanation:** The input scatterlist describes CPU-side memory regions. The DMA layer can combine physically adjacent ranges or use an IOMMU to create a larger contiguous DMA segment. After mapping, portable drivers obtain device addresses and lengths with `sg_dma_address()` and `sg_dma_len()`.

The return value is not a count of successfully processed input entries. A smaller nonzero value is normal success.

**API / Code Anchor:**
```c
mapped = dma_map_sg(dev, sgl, orig_nents, DMA_FROM_DEVICE);
if (!mapped)
        return -EIO;

for_each_sg(sgl, sg, mapped, i)
        program_hw(i, sg_dma_address(sg), sg_dma_len(sg));

dma_unmap_sg(dev, sgl, orig_nents, DMA_FROM_DEVICE);
```

**Production or Debugging Angle:** Validate both the returned segment count and every mapped length against hardware limits. The DMA API may produce a legal mapping that the device's descriptor format still cannot represent unless the device constraints were configured correctly.

**Common Traps:**
- Programming hardware using `orig_nents`.
- Unmapping with `mapped`.
- Reading raw page addresses after mapping.
- Treating a reduced count as partial failure.
- Assuming each source entry must be exactly one page.

**Follow-up Questions:**
- What does `sg_dma_len()` return?
- How can an IOMMU merge entries?
- What should happen if `dma_map_sg()` returns zero?

### 9. Compare DMA mapping API and DMAengine.

**Level:** Mid-Level

**Question:** Why might a driver need both?

**Short Answer:** The mapping API creates device-visible memory mappings and handles cache ownership. DMAengine acquires and programs a DMA controller channel. A peripheral DMA client commonly maps its buffers and then submits those mapped addresses through DMAengine.

**Deep Explanation:** A device with built-in bus mastering may only need the mapping API because its driver directly programs device descriptors. A UART, SPI controller, or audio interface may rely on a separate SoC DMA controller. Its driver requests a channel, configures FIFO address and bus characteristics, prepares a descriptor, submits it, and starts pending work.

Neither API replaces the other:

- DMAengine does not make an arbitrary CPU pointer device-visible.
- `dma_map_single()` does not start a DMA controller.

**API / Code Anchor:**
```c
chan = dma_request_chan(dev, "tx");
if (IS_ERR(chan))
        return PTR_ERR(chan);

ret = dmaengine_slave_config(chan, &cfg);
desc = dmaengine_prep_slave_sg(chan, sgl, mapped_nents,
                               DMA_MEM_TO_DEV, DMA_PREP_INTERRUPT);
cookie = dmaengine_submit(desc);
if (dma_submit_error(cookie))
        /* unwind */;

dma_async_issue_pending(chan);
```

**Production or Debugging Angle:** For a stuck transfer, separate mapping failures from controller failures. Confirm the mapped addresses first, then descriptor preparation, submit result, issue-pending, peripheral request generation, and callback.

**Common Traps:**
- Using `DMA_MEM_TO_DEV` as a `dma_map_*()` direction.
- Mapping with the consumer device when the DMAengine contract requires the DMA channel's device.
- Forgetting `dma_async_issue_pending()`.
- Assuming submit starts hardware immediately.

**Follow-up Questions:**
- What is stored in `struct dma_slave_config`?
- Why must preparation and submission be closely paired?
- What context invokes DMAengine completion callbacks?

### 10. How should a DMAengine client be stopped?

**Level:** Mid-Level

**Question:** Compare synchronous and asynchronous termination.

**Short Answer:** In sleepable teardown, use `dmaengine_terminate_sync()` so transfer activity and callbacks are finished before freeing state. If termination must start in atomic context, use `dmaengine_terminate_async()` and later call `dmaengine_synchronize()` before releasing buffers or callback data.

**Deep Explanation:** Termination may discard in-flight data and suppress callbacks for incomplete transfers. More importantly, starting termination does not always prove that hardware and callbacks have stopped. `dmaengine_terminate_async()` is suitable where sleeping is illegal, but it creates a later synchronization obligation. `dmaengine_terminate_all()` is legacy and should not be used for new code.

**API / Code Anchor:**
```c
/* Sleepable remove or stop path. */
dmaengine_terminate_sync(chan);
unmap_and_free_buffers();

/* Atomic phase followed by sleepable cleanup. */
dmaengine_terminate_async(chan);
schedule_work(&d->cleanup_work);

/* cleanup_work() */
dmaengine_synchronize(chan);
unmap_and_free_buffers();
```

**Production or Debugging Angle:** A use-after-free after unbind often means the driver initiated termination but freed callback state before synchronization completed.

**Common Traps:**
- Freeing state immediately after `dmaengine_terminate_async()`.
- Calling a sleeping termination helper from atomic context.
- Expecting callbacks for descriptors discarded by termination.
- Releasing the channel before stopping peripheral DMA requests.

**Follow-up Questions:**
- What should happen to subsystem-owned queued buffers during stop?
- Why must the peripheral often be stopped before the DMA channel?
- How would suspend reuse this ordering?

## Senior Questions

### 11. Why can coherent DMA memory still require memory barriers?

**Level:** Senior

**Question:** A descriptor ring is allocated with `dma_alloc_coherent()`. Why can the device still observe an ownership bit before the descriptor fields?

**Short Answer:** Coherence controls visibility of values, not ordering of independent memory operations. The CPU may reorder descriptor stores unless a barrier orders the data fields before ownership publication or the device doorbell.

**Deep Explanation:** Coherent memory ensures that CPU and device do not require explicit cache flush/invalidate operations to observe each other's writes. It does not guarantee that:

```c
desc->addr = dma;
desc->len = len;
desc->owned = 1;
```

reaches the device in source-code order. A write barrier is commonly required before publishing `owned`. MMIO doorbell ordering must also follow the architecture's I/O accessor rules. On completion, the driver may need an appropriate read barrier after observing ownership returned by the device before consuming other descriptor fields.

**API / Code Anchor:**
```c
WRITE_ONCE(desc->addr, dma);
WRITE_ONCE(desc->len, len);
dma_wmb();
WRITE_ONCE(desc->owned, DEVICE_OWNS);
writel(KICK, regs + DOORBELL);
```

The exact barrier depends on the descriptor and device protocol.

**Production or Debugging Angle:** Rare descriptor corruption under load, especially on weakly ordered architectures, is a strong signal to audit publication and completion barriers rather than adding delays.

**Common Traps:**
- Equating cache coherence with ordering.
- Adding `volatile` instead of barriers.
- Using a full barrier without understanding the producer/consumer protocol.
- Ignoring ordering between normal memory and MMIO.

**Follow-up Questions:**
- When would `dma_wmb()` be more appropriate than `wmb()`?
- What ordering does `writel()` provide on the target architecture?
- How should device-to-CPU ownership return be ordered?

### 12. Design safe remove and timeout paths for asynchronous DMA.

**Level:** Senior

**Question:** The driver has queued buffers, an IRQ, a DMAengine callback, and user-visible streaming. What is the safe teardown sequence?

**Short Answer:** Block new submissions, unpublish or stop the user-facing stream, disable peripheral DMA requests and IRQ generation, terminate and synchronize DMA, synchronize IRQ/callback contexts, return all queued buffers with error, unmap/free memory, then release the channel.

**Deep Explanation:** Several producers can retain access:

- userspace or subsystem queue operations;
- the peripheral generating DMA requests;
- the DMA controller;
- IRQ handlers;
- DMAengine callbacks;
- timeout or recovery work.

Teardown must close each source in dependency order. Locks protect queue state but do not prove callbacks have finished. Devm allocations simplify final release but do not quiesce any producer.

**API / Code Anchor:**
```text
set stopping flag
  -> reject new queue operations
  -> stop peripheral / mask DMA requests
  -> disable and synchronize device IRQ
  -> dmaengine_terminate_sync()
  -> cancel timeout/recovery work
  -> return queued buffers with ERROR
  -> dma_unmap_*()
  -> free backing memory
  -> dma_release_channel()
```

**Production or Debugging Angle:** Exercise repeated bind/unbind, stream-on/stream-off, timeout injection, and suspend/resume under load. KASAN plus DMA API debug can expose different halves of the same lifetime bug.

**Common Traps:**
- Freeing devm memory while callbacks still run.
- Holding a spinlock across `dmaengine_terminate_sync()`.
- Returning buffers before hardware is unable to write them.
- Forgetting queued buffers that never became active.
- Allowing a recovery worker to restart DMA after stop began.

**Follow-up Questions:**
- How would you serialize timeout recovery with remove?
- Which state transitions require a lock versus a reference?
- What changes if termination starts in IRQ context?

### 13. Debug a driver that works on x86 but corrupts data on ARM.

**Level:** Senior

**Question:** What hypotheses and evidence would you prioritize?

**Short Answer:** Prioritize cache ownership, wrong direction, missing sync calls, coherent descriptor ordering, cache-line sharing, and unsupported buffer backing. x86 coherence can hide DMA API violations that appear on non-coherent ARM systems.

**Deep Explanation:** First classify the corrupted object:

- payload corruption suggests streaming ownership, length, or direction;
- descriptor corruption suggests barriers, alignment, or lifetime;
- corruption after timeout/remove suggests use-after-unmap;
- only SG corruption suggests count or segment programming errors.

Then trace the exact lifecycle from CPU write through mapping, hardware ownership, completion, sync/unmap, and CPU read. Check whether buffers share cache lines with CPU-owned state and whether any direct physical-address conversion bypasses the DMA API.

**API / Code Anchor:**
```text
Audit:
  dma_map_* direction and return checks
  dma_sync_* ownership transitions
  coherent descriptor barriers
  buffer allocation source
  device used for mapping
  length/alignment/cache-line boundaries
```

**Production or Debugging Angle:** Reproduce with DMA API debug, IOMMU enabled, stress load, and known data patterns. Compare device-written memory before and after the CPU ownership transition.

**Common Traps:**
- Adding `msleep()` and assuming timing was the cause.
- Marking pointers `volatile`.
- Switching everything to coherent memory without finding the violated contract.
- Blaming ARM before verifying direction and ownership.

**Follow-up Questions:**
- How can cache-line sharing corrupt adjacent fields?
- Which barriers apply to coherent descriptors?
- How would an IOMMU help diagnose the issue?

### 14. Debug an IOMMU fault during DMA.

**Level:** Senior

**Question:** The system reports an SMMU fault for a device write to an unmapped IOVA. What do you inspect?

**Short Answer:** Correlate the faulting device and IOVA with current mappings, then inspect mapping lifetime, device identity, direction/permissions, address truncation, descriptor corruption, and timeout/remove races.

**Deep Explanation:** An IOMMU fault often means the device used an address after its mapping was removed, used an address created for another device/domain, accessed beyond the mapped length, or read a corrupted descriptor. The fault's read/write type helps compare expected direction. A stale completion or hardware engine continuing after timeout is a common cause.

Investigation order:

1. Identify the exact requester/device.
2. Record IOVA, access type, and fault reason.
3. Find the descriptor that contained the address.
4. Confirm the mapping used the same device.
5. Confirm address width and high-register programming.
6. Check map/unmap timestamps and transfer completion.
7. Audit timeout, reset, suspend, and remove races.

**API / Code Anchor:**
```c
dev_dbg(dev, "cpu=%p dma=%pad len=%zu dir=%d\n",
        buf, &dma, len, dir);
```

**Production or Debugging Angle:** Keep bounded descriptor/mapping history in debug builds. DMA API debug catches misuse at the API boundary, while the IOMMU fault shows the hardware access that escaped.

**Common Traps:**
- Assuming the IOVA should equal a physical address.
- Looking only at the faulting callback instead of the earlier unmap.
- Ignoring high DMA address bits.
- Resetting hardware before collecting status and descriptor evidence.

**Follow-up Questions:**
- How can use-after-unmap happen during remove?
- What would an out-of-range device write look like?
- Why does mapping with the wrong device matter with multiple IOMMU domains?

### 15. How would you review a DMA descriptor ring for production?

**Level:** Senior

**Question:** What invariants must be explicit in the design?

**Short Answer:** Review address width, descriptor layout and endianness, coherent allocation, ownership states, barriers, producer/consumer indices, locking, completion validation, reset behavior, and lifetime through teardown.

**Deep Explanation:** A descriptor ring is a concurrent protocol between CPU and hardware. Its correctness depends on more than allocation:

- each DMA address and length fits the descriptor format;
- descriptors meet alignment and boundary requirements;
- CPU initializes all fields before ownership handoff;
- the barrier protocol orders fields and ownership;
- the device returns ownership before CPU reuse;
- producer/consumer counters handle wraparound safely;
- interrupt moderation cannot lose completions;
- reset can classify completed, active, and queued descriptors;
- payload mappings remain active for every device-owned descriptor.

**API / Code Anchor:**
```text
CPU-owned descriptor:
  initialize fields
  -> dma_wmb()
  -> set DEVICE_OWNS
  -> ring doorbell

Completion:
  observe CPU_OWNS
  -> dma_rmb()
  -> read status/length
  -> sync/unmap payload
  -> recycle descriptor
```

**Production or Debugging Angle:** Add ring dumps that show indices, ownership, DMA addresses, lengths, and status. Capture them before reset on timeout. Validate wraparound and full/empty conditions with stress and fault injection.

**Common Traps:**
- Assuming coherent allocation solves the protocol.
- Updating ring indices without defining lock or atomic ownership.
- Reusing a descriptor before payload DMA has stopped.
- Trusting device-reported lengths without bounds checking.
- Failing to handle partial completion during reset.

**Follow-up Questions:**
- How would NAPI change network ring completion handling?
- What is the difference between descriptor ownership and buffer ownership?
- How would you prevent a stale interrupt from completing a reused descriptor?

### 16. When should a driver use DMA-BUF rather than ordinary DMA mapping?

**Level:** Senior

**Question:** Compare local DMA mappings with cross-driver shared buffers.

**Short Answer:** Use ordinary DMA mapping when one driver owns backing memory and maps it for its device. Use DMA-BUF when buffers must be shared across drivers, devices, subsystems, or userspace with explicit attachment and synchronization semantics.

**Deep Explanation:** DMA-BUF wraps shared backing storage in a reference-counted object that can be exported as a file descriptor. Importing devices attach to it and obtain device-specific mapped scatterlists. Fences and reservation objects coordinate asynchronous hardware access. CPU access is bracketed with DMA-BUF CPU access helpers.

DMA-BUF does not mean that every device uses the same DMA address. Each attachment is mapped for its own device and IOMMU domain.

**API / Code Anchor:**
```text
Exporter:
  dma_buf_export() -> dma_buf_fd()

Importer:
  dma_buf_get()
  -> dma_buf_attach()
  -> dma_buf_map_attachment()
  -> device access and synchronization
  -> dma_buf_unmap_attachment()
  -> dma_buf_detach()
  -> dma_buf_put()
```

**Production or Debugging Angle:** Shared-buffer bugs often involve missing fence waits, CPU access outside begin/end brackets, leaked attachments, or detach before hardware completion. Inspect DMA-BUF statistics and subsystem-specific queue state.

**Common Traps:**
- Treating a DMA-BUF file descriptor as a DMA address.
- Assuming one attachment's SG mapping is valid for another device.
- Ignoring fences because the memory is cache coherent.
- Using DMA-BUF for a purely private single-driver buffer without a sharing need.

**Follow-up Questions:**
- What roles do exporter and importer play?
- Why are `dma-fence` and `dma-resv` needed?
- How does userspace pass a DMA-BUF between V4L2 and DRM?

## Debugging Scenarios

### 17. `dma_map_single()` succeeds, but hardware sees address zero.

**Level:** Mid-Level

**Question:** The driver checks `if (!dma)` for failure and skips programming when the returned address is zero. What is wrong?

**Short Answer:** Zero can be a valid DMA address. Mapping failure must be checked with `dma_mapping_error(dev, dma)`.

**Deep Explanation:** DMA addresses are opaque device-visible values. Their valid range is platform dependent. The DMA API provides a device-specific failure test because no universal numeric sentinel is safe for all platforms.

**API / Code Anchor:**
```c
dma = dma_map_single(dev, buf, len, dir);
if (dma_mapping_error(dev, dma))
        return -EIO;
```

**Production or Debugging Angle:** Enable DMA API debugging, which can report mappings whose error status was never checked on supported platforms.

**Common Traps:**
- Checking `dma == 0`.
- Checking against `DMA_MAPPING_ERROR` directly.
- Continuing after mapping failure.
- Converting `dma_addr_t` to a narrower integer for the check.

**Follow-up Questions:**
- How is `dma_map_sg()` failure reported?
- Why is `dma_addr_t` opaque?
- Which unmap is required after a failed mapping?

### 18. SG transfer corrupts data only when the IOMMU is enabled.

**Level:** Senior

**Question:** The driver loops over the original SG entries and programs their raw addresses after `dma_map_sg()`. Why does enabling the IOMMU expose the bug?

**Short Answer:** After mapping, the driver must use the returned mapped count plus `sg_dma_address()` and `sg_dma_len()`. Raw input addresses are not device addresses, and the IOMMU may merge or remap entries.

**Deep Explanation:** Without an IOMMU, identity-like mappings can hide the mistake. With an IOMMU, the device must use IOVAs created for the mapping. The mapped shape may also differ from the input shape. Programming raw pages bypasses both translation and merging.

**API / Code Anchor:**
```c
mapped = dma_map_sg(dev, sgl, orig_nents, dir);
if (!mapped)
        return -EIO;

for_each_sg(sgl, sg, mapped, i)
        program_hw(sg_dma_address(sg), sg_dma_len(sg));
```

**Production or Debugging Angle:** Correlate IOMMU faults with the raw address programmed by the driver. Audit whether unmap still uses `orig_nents`.

**Common Traps:**
- Disabling the IOMMU as the "fix."
- Assuming `mapped == orig_nents`.
- Reading `sg->length` instead of `sg_dma_len()` for hardware.
- Unmapping with the returned count.

**Follow-up Questions:**
- Why may entries be merged?
- What hardware segment constraints should be configured?
- How would you log mapped SG entries safely?

### 19. Unbind causes a callback to access freed memory.

**Level:** Senior

**Question:** The driver uses `dmam_alloc_coherent()`, so remove only releases the DMA channel. Why is there a use-after-free?

**Short Answer:** Managed allocation controls final release, not asynchronous quiescence. Remove must stop the peripheral, terminate and synchronize DMA, and synchronize callbacks before devres releases the coherent memory and private state.

**Deep Explanation:** The DMA controller or callback can still hold references when remove returns. Devres then releases managed memory. A late callback dereferences storage whose lifetime ended. Correctness requires an explicit stop protocol before automatic release.

**API / Code Anchor:**
```c
static void my_remove(struct platform_device *pdev)
{
        struct mydev *d = platform_get_drvdata(pdev);

        d->stopping = true;
        stop_peripheral_dma_requests(d);
        dmaengine_terminate_sync(d->chan);
        cancel_work_sync(&d->recovery_work);
        dma_release_channel(d->chan);
        /* Managed memory is released after remove. */
}
```

**Production or Debugging Angle:** Reproduce with KASAN and rapid bind/unbind under transfer load. Add callback counters and log whether callbacks execute after the stopping flag is set.

**Common Traps:**
- Assuming devm means remove needs no ordering.
- Releasing the channel before terminating activity.
- Setting a stopping flag without synchronizing existing callbacks.
- Freeing queued subsystem buffers before hardware is stopped.

**Follow-up Questions:**
- What if remove cannot sleep?
- How should a timeout worker be cancelled?
- Which resources should be unpublished before termination?

## Rapid-Fire Traps

- **Trap:** "`GFP_DMA` makes a buffer DMA-safe."  
  **Correction:** It is a legacy zone constraint. Use device masks and DMA allocation/mapping APIs.

- **Trap:** "`dma_addr_t` is the physical address."  
  **Correction:** It is the address visible to a specific device and may be an IOVA.

- **Trap:** "Coherent memory needs no synchronization."  
  **Correction:** It avoids explicit cache ownership sync, but descriptor ordering can still require barriers.

- **Trap:** "`dma_map_sg()` returning fewer entries means partial failure."  
  **Correction:** A nonzero smaller count normally means entries were merged.

- **Trap:** "Use the returned SG count for unmap."  
  **Correction:** Program hardware with the returned count; unmap with the original input count.

- **Trap:** "`dmaengine_submit()` starts the transfer."  
  **Correction:** It queues the descriptor. `dma_async_issue_pending()` activates pending work.

- **Trap:** "`dmaengine_terminate_async()` makes memory immediately safe to free."  
  **Correction:** Follow it with `dmaengine_synchronize()` in a suitable context.

- **Trap:** "If DMA works on x86, cache handling is correct."  
  **Correction:** Coherent development hardware can hide ownership and sync violations.

- **Trap:** "`vmalloc()` gives one contiguous DMA buffer."  
  **Correction:** It is virtually contiguous, not generally suitable for one `dma_map_single()` mapping.

- **Trap:** "The driver should parse `dma-coherent` and enable coherence itself."  
  **Correction:** Firmware/platform DMA configuration normally establishes device coherence.
