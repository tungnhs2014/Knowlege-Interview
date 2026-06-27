# Topic Brief - 21 - DMA And DMA Mapping

## Output Targets
- Knowledge: `knowledge/21-dma-and-dma-mapping.md`
- Interview: `interview/21-dma-and-dma-mapping.md`
- Example: `examples/21-dma-and-dma-mapping/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch05` | `docs/Linux Device Driver Development/Chapter 5-Platform Device Drivers.md` | read/mapped/related | Legacy platform-resource representation of DMA channels through `IORESOURCE_DMA`; useful historical contrast with firmware-described DMAengine channels. |
| `ldd1-ch06` | `docs/Linux Device Driver Development/Chapter 6-The Concept of a Device Tree .md` | read/mapped/covered/merged | DMA controller/consumer DT examples, `dmas`, `dma-names`, named resource lookup, and an i.MX SDMA consumer example. |
| `ldd1-ch08` | `docs/Linux Device Driver Development/Chapter 8-SPI Device Drivers.md` | read/mapped/covered/merged | Applied DMA-safe buffer and lifetime rules for `spi_transfer`, pre-mapped `tx_dma`/`rx_dma`, `spi_message.is_dma_mapped`, and completion ownership. |
| `ldd1-ch11` | `docs/Linux Device Driver Development/Chapter 11-Kernel Memory Management.md` | read/mapped/related | Address and allocation background: physical contiguity, `kmalloc` versus `vmalloc`, legacy DMA zones, and the boundary between ordinary allocation and the DMA API. |
| `ldd1-ch12` | `docs/Linux Device Driver Development/Chapter 12-DMA - Direct Memory Access.md` | read/mapped/covered/merged | Primary dedicated source: cache coherency, coherent and streaming mappings, single and scatter-gather mappings, completions, DMAengine client flow, i.MX SDMA example, and DT channel acquisition. |
| `ldd1-ch21` | `docs/Linux Device Driver Development/Chapter 21-Framebuffer Drivers.md` | read/mapped/related | Applied device-lifetime coherent framebuffer allocation and reverse-order DMA teardown context. |
| `ldd1-ch22` | `docs/Linux Device Driver Development/Chapter 22-Network Interface Card Drivers.md` | read/mapped/related | Applied asynchronous packet-buffer ownership, DMA completion, IRQ handling, and open/stop resource lifecycle. |
| `ldd2-ch01` | `docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md` | read/mapped/covered/merged | Completion semantics for DMA transactions: waiters, IRQ-safe signaling, and buffer access after completion. |
| `ldd2-ch04` | `docs/Linux Device Driver Development 2/Chapter 4-Common_Clock_Framework.md` | read/mapped/related | UART clock-consumer example also declares `dmas` and `dma-names`; confirms that DMA channel references coexist with other provider/consumer resources but adds no mapping lifecycle. |
| `ldd2-ch05` | `docs/Linux Device Driver Development 2/Chapter 5-ASoC_Framework.md` | read/mapped/related | Applied cyclic DMAengine/PCM integration, peripheral bus widths, burst configuration, and framework-owned streaming buffers. |
| `ldd2-ch06` | `docs/Linux Device Driver Development 2/Chapter 6-ASoC_Machine_Drivers.md` | read/mapped/related | ASoC DAI-link context for the DMA-capable platform component and `rx`/`tx` channel names consumed by the PCM DMAengine framework. |
| `ldd2-ch07` | `docs/Linux Device Driver Development 2/Chapter 7-Demystifying V4L2 and Video Capture Device Drivers.md` | read/mapped/covered/merged | Realistic DMA buffer lifecycle: contiguous versus SG backends, IOMMU/bounce-buffer hooks, queued/active/done/error states, IRQ completion, and stop-streaming cleanup. |
| `ldd2-ch09` | `docs/Linux Device Driver Development 2/Chapter 9-Leveraging the V4L2.md` | read/mapped/related | DMA-BUF exporter/importer ownership, userspace queue/dequeue lifecycle, and the rule that userspace must not access a buffer while hardware owns it. |
| `ldd2-ch11` | `docs/Linux Device Driver Development 2/Chapter 11-Writing_PCI_Device_Drivers.md` | read/mapped/covered/merged | Independent PCI-oriented DMA source: DMA masks, 32/64-bit addressing, IOMMU translation, coherent/streaming/SG mappings, and `sg_dma_address()`/`sg_dma_len()`. |
| `notion-ch03-part8` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 8 Wait Queues and Sleep Wake Mechanisms.md` | read/mapped/covered/merged | Strongest Notion completion treatment: initialization, reuse, IRQ signaling, timeouts, and a managed coherent-buffer example. |
| `notion-ch05-part2` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 2 Probe, Remove & Resource Management.md` | read/mapped/related | Probe/remove resource ownership and legacy `IORESOURCE_DMA` acquisition. |
| `notion-ch06-part1` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 1 Device Tree Fundamentals.md` | read/mapped/related | `dma-coherent` and `dma-names` as firmware-description examples; the manual property handling shown requires correction. |
| `notion-ch06-part2` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 2 Device Addressing and Resources.md` | read/mapped/covered/merged | Dedicated Notion DMA-reference section: provider/consumer model, `#dma-cells`, `dmas`, `dma-names`, and named channel request. |
| `notion-ch06-part3` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 3 OF APIs and Platform Integration.md` | read/mapped/related | Repeats `dma-coherent` property parsing; useful as a source-difference case because a normal client driver should not implement coherency by calling a fictional hardware helper. |
| `notion-ch07-extra-v4l2-part1` | `docs/Linux-Device-Driver-Notion/Chapter 7-v4l2_part1.md` | read/mapped/related | Applied `vb2_queue` and DMA-contiguous/scatter-gather allocator selection. |
| `notion-ch07-extra-v4l2-part2` | `docs/Linux-Device-Driver-Notion/Chapter 7-v4l2_part2.md` | read/mapped/covered/merged | Applied queued-to-active-to-done DMA ownership, buffer address handoff, IRQ completion, queue rotation, and stop-streaming cleanup. |
| `notion-ch08-part2` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 2 SPI Transfer Mechanisms and Communication APIs.md` | read/mapped/covered/merged | Apparent derivative of the SPI book source, independently read; adds clearer transfer diagrams but repeats an overly broad `GFP_DMA` recommendation. |
| `notion-ch08-part3` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 3 Device Tree Integration and Userspace Inter.md` | read/mapped/related | SPI controller DT example repeats `dmas` and `dma-names`; useful firmware-channel context but no DMA mapping, ownership, or teardown detail. |
| `notion-ch09-extra-userspace-part1` | `docs/Linux-Device-Driver-Notion/Chapter 9-userspace-part1.md` | read/mapped/related | MMAP, USERPTR, and DMABUF buffer models as subsystem-specific applications of DMA ownership. |
| `notion-ch09-extra-userspace-part2` | `docs/Linux-Device-Driver-Notion/Chapter 9-userspace-part2.md` | read/mapped/related | Applied DMABUF pipelines and cross-device sharing; detailed DMA-BUF belongs to V4L2/userspace topics. |
| `notion-index` | `docs/Linux-Device-Driver-Notion/Linux Device Drivers.md` | inspected/mapped/index-only | Notion source index; confirms there is no standalone Notion chapter equivalent to `ldd1-ch12`. |

## Source Files Read
- `ldd1-ch12`: complete chapter, including mapping types, cache ownership, single/SG mapping, completion, DMAengine configuration/preparation/submission, the i.MX SDMA example, and DT binding.
- `ldd2-ch11`: complete "PCI and Direct Memory Access (DMA)" through coherent, streaming, single-buffer, and scatter-gather sections.
- `ldd1-ch11`: DMA-zone, `kmalloc`/`vmalloc`, physical/bus address, cache, and managed-resource sections.
- `ldd1-ch05` and `ldd1-ch06`: platform DMA resources, DMA controller example, `dmas`, `dma-names`, and consumer lookup.
- `ldd1-ch08`: `spi_transfer`, `spi_message`, DMA-safe buffers, pre-mapped messages, and async buffer lifetime.
- `ldd1-ch21` and `ldd1-ch22`: framebuffer/network probe, open, completion, and teardown references involving DMA.
- `ldd2-ch01`: completion framework sections using DMA as the asynchronous operation.
- `ldd2-ch04`: UART clock-consumer example containing `dmas` and `dma-names`; checked in context to confirm it is incidental DMA channel declaration rather than mapping guidance.
- `ldd2-ch05`: DMAengine-backed PCM/cyclic transfer sections.
- `ldd2-ch06`: DAI-link platform component, SSI DMA references, and the `rx`/`tx` names expected by the PCM DMAengine framework.
- `ldd2-ch07`: VB2 allocator choices, queue callbacks, IOMMU/bounce-buffer hooks, DMA address storage, start/stop streaming, and completion handling.
- `ldd2-ch09`: queued DMA buffers, DMABUF exporter/importer, locked-buffer ownership, and streaming lifecycle.
- `notion-ch03-part8`: complete completion section and DMA completion example.
- `notion-ch05-part2`: DMA resource and probe/remove ownership references.
- `notion-ch06-part1`, `notion-ch06-part2`, and `notion-ch06-part3`: DMA properties, provider/consumer references, named channels, and coherency examples.
- `notion-ch07-extra-v4l2-part1` and `notion-ch07-extra-v4l2-part2`: VB2 memory backends, DMA buffer states, mapping/address handoff, IRQ completion, and stop cleanup.
- `notion-ch08-part2`: complete transfer/message and buffer-lifetime sections; compared independently with `ldd1-ch08`.
- `notion-ch08-part3`: SPI controller DT declaration containing `dmas` and `dma-names`; checked independently and classified as related-only.
- `notion-ch09-extra-userspace-part1` and `notion-ch09-extra-userspace-part2`: MMAP/USERPTR/DMABUF and shared pipeline sections.
- `notion-index`: inspected for a dedicated DMA source and chapter mapping.

### Inventory Decisions
- All three roots were searched for DMA, DMA API names, scatter-gather, cache coherency, DMA addresses, IOMMU, DMA-BUF, and DMAengine terms.
- `ldd1-ch12` and `ldd2-ch11` were not treated as the same chapter despite substantial overlap; both were read and compared.
- `notion-ch08-part2` was not skipped as a derivative of `ldd1-ch08`; it was independently checked for additions and errors.
- `ldd2-ch04`, `ldd2-ch06`, and `notion-ch08-part3` were independently read after keyword inventory. Their DMA references are retained as related source trace rather than merged into core mapping guidance.
- Notion has no dedicated DMA-mapping chapter. Its relevant coverage is distributed across completion, DT, SPI, and V4L2 notes.
- Files with only a passing mention such as "request DMA resources" or a DMA-capable hardware bullet were screened but not promoted to core coverage when they added no mapping, ownership, lifecycle, or debugging detail.
- DMA-BUF, V4L2, ASoC, framebuffer, network, PCI, and SPI sources are retained as applied evidence. Their subsystem-specific APIs remain owned by topics 17, 29-36.

## Merged Source Notes
- Keep three address domains distinct:
  - CPU virtual address: dereferenced by kernel code.
  - CPU physical/resource address: describes RAM or hardware resources.
  - DMA address (`dma_addr_t`): device-visible address returned or populated by the DMA API, possibly translated by an IOMMU or bounce layer.
- DMA mapping and DMAengine solve different problems:
  - The DMA mapping API makes memory addressable and coherent for a specific device.
  - DMAengine programs a DMA controller/channel to execute a transfer.
- Coherent mappings provide simultaneous CPU/device visibility without explicit `dma_sync_*()` ownership transitions. They do not imply universally uncached memory and do not remove the need for memory barriers when publishing descriptors or ownership bits.
- Streaming mappings transfer ownership between CPU and device:
  - For `DMA_TO_DEVICE`, fill the buffer before mapping or syncing for the device.
  - For `DMA_FROM_DEVICE`, do not consume device-written data until unmap or sync for the CPU.
  - A long-lived mapping may be reused only with the matching `dma_sync_*_for_cpu()` and `dma_sync_*_for_device()` transitions.
- `DMA_BIDIRECTIONAL` is not a safe default. Direction controls cache maintenance and documents permitted access.
- A device's DMA addressing capability must be established during probe. Prefer `dma_set_mask_and_coherent()` when streaming and coherent masks are the same; use separate mask calls only for real hardware differences.
- Coherent memory suits long-lived control structures such as descriptor rings and mailboxes. `dma_pool` suits many small aligned coherent objects. Streaming mapping suits payload buffers whose ownership changes per transaction.
- `dma_map_single()` and `dma_map_page()` require `dma_mapping_error()` checks. `dma_map_sg()` returns zero on failure.
- `dma_map_sg()` may merge input entries:
  - Program hardware using the returned DMA-segment count and `sg_dma_address()`/`sg_dma_len()`.
  - Unmap and sync using the original input `nents`, not the returned count.
- Ordinary stack, module-image, arbitrary static, and direct `vmalloc` addresses are not valid generic inputs to `dma_map_single()`. Suitable backing and the subsystem contract must be verified.
- `GFP_DMA` is a legacy allocation-zone constraint, not a general marker that makes a buffer DMA-safe. Device masks plus `dma_alloc_*()`/`dma_map_*()` are the normal mechanism.
- DMAengine client flow is:
  - Request a named channel with `dma_request_chan()`.
  - Configure FIFO address, bus width, burst size, and other slave parameters.
  - Map the payload with the correct DMA device.
  - Prepare a descriptor with the transfer-specific helper.
  - Set callback state, submit, check `dma_submit_error()`, and issue pending work.
  - On completion, transfer ownership back, sync/unmap as required, and publish the result.
- Teardown order is a correctness requirement: block new work, stop the peripheral, terminate and synchronize DMA/callbacks, then unmap/free buffers and release the channel.
- Completion objects are useful for synchronous waiting around asynchronous DMA, but production paths need timeouts, cancellation, status/error reporting, and race-safe reuse.
- DT `#dma-cells` meanings are provider-binding-specific. `dmas` and `dma-names` connect a consumer to named channels; client code should normally request the channel by name rather than decode provider cells itself.
- Applied SPI, V4L2, network, framebuffer, and ASoC sources reinforce the same invariant: buffers, mappings, descriptors, and callback context must remain alive until hardware and completion handlers have stopped using them.

## Source Differences
- `ldd1-ch12` is the broadest source and uniquely combines mapping, DMAengine, completion, and DT. `ldd2-ch11` independently repeats mapping concepts in PCI context and adds DMA masks, IOMMU context, and portable SG accessor guidance.
- `ldd1-ch12` and `ldd2-ch11` describe coherent memory as uncached/unbuffered and physically contiguous as if these were universal implementation facts. Teach the portable DMA API contract instead.
- Both book sources incorrectly limit single-buffer streaming mapping to one page. Current DMA API limits are device/platform-specific; use `dma_max_mapping_size()` where a driver needs the bound.
- Both sources incorrectly imply SG input entries must all be page-sized except the last. Scatterlist entries can describe page fragments with offsets and lengths; hardware and DMA API constraints determine legal segments.
- Both sources use `GFP_DMA` for ordinary payload buffers. Do not reproduce this as normal guidance.
- `ldd1-ch12` contains invalid examples using `dma_map_single(NULL, ...)`, `dma_map_sg(NULL, ...)`, and `DMA_MEM_TO_MEM` where `enum dma_data_direction` is required.
- The `ldd1-ch12` combined example omits `dma_mapping_error()`, maps with the wrong device, contains a duplicated source unmap where destination unmap is intended, and does not provide production-grade failure unwind or teardown synchronization.
- `ldd2-ch11` teaches legacy PCI wrappers such as `pci_alloc_consistent()`, `pci_map_single()`, and `pci_map_sg()`. Prefer the generic `dma_*()` interfaces using `&pdev->dev`.
- `ldd2-ch11` examples contain extraction/code defects such as wrong variable checks, pointer/type errors, a `NULL` PCI device in SG mapping, and 32-bit register writes without proving the DMA address fits.
- Internal sources do not consistently state that coherent memory still requires barriers. Descriptor field writes must be ordered before handing ownership to hardware.
- `ldd1-ch12` uses `dma_request_channel()` and `dma_request_slave_channel()`. Current named firmware-associated clients should normally use `dma_request_chan()`, handle `ERR_PTR()` results, and release with `dma_release_channel()`.
- Current DMAengine teardown uses `dmaengine_terminate_sync()` when sleepable, or `dmaengine_terminate_async()` followed by `dmaengine_synchronize()` before freeing callback or buffer state. Internal sources do not cover this.
- The Notion DT source presents `#dma-cells = <3>` as universally channel/priority/direction. Cell count and meaning belong to the DMA provider's binding.
- `ldd2-ch04`, `ldd2-ch06`, and `notion-ch08-part3` show valid consumer-side channel naming context, but none explains DMA mapping ownership or cache synchronization; they do not replace the dedicated mapping sources.
- The Notion `dma-coherent` examples manually parse the property and call `enable_coherent_dma()`. Normal DMA coherency setup is handled by firmware/platform DMA configuration; client drivers should not invent this mechanism.
- The Notion completion example is learning-only: it has undefined register constants, incomplete status/error handling, no remove path that quiesces DMA, and no proof that the DMA addresses fit the shown register width.
- Notion SPI correctly rejects stack-backed asynchronous DMA buffers but incorrectly proposes `kmalloc(..., GFP_KERNEL | GFP_DMA)` as the general solution.
- V4L2 sources provide better ownership-state examples than the dedicated mapping chapters, but claims that contiguous DMA is always faster than SG are not portable.
- DMA-BUF is related but distinct from ordinary DMA mapping. It adds cross-driver sharing, attachments, SG tables, fences, and reservation objects; do not collapse it into `dma_map_single()` lifecycle.

## Gaps / Uncertainties
- Internal sources do not adequately cover `dma_pool`, `dmam_alloc_coherent()`, `dma_alloc_attrs()`, `dma_alloc_noncontiguous()`, partial sync, mapping-size/segment-boundary queries, or managed DMA pools.
- SWIOTLB/bounce buffering, restricted DMA pools, IOMMU domains/faults, peer-to-peer DMA, and confidential-memory constraints are absent or only named.
- Cache-line sharing and alignment hazards on non-coherent systems need current architecture-specific validation before detailed examples.
- Internal sources do not cover DMA API debugging, mapping-leak diagnosis, or deterministic DMAengine testing in enough depth.
- Detailed DMA-BUF exporter/importer, fence, reservation-object, and userspace synchronization behavior belongs primarily to V4L2/display topics, with only a boundary explanation here.
- DMA controller/provider-driver implementation is outside this topic. Topic 21 should teach mapping and DMAengine client use, not provider registration internals.
- A production example needs a concrete device/controller contract. A memory-to-memory DMAengine demo is useful for lifecycle, but it does not replace peripheral FIFO configuration, residue/error handling, cyclic transfer behavior, or hardware reset.
- Exact APIs and available tracepoints must be validated against the target kernel. Current external checks used the latest kernel documentation visible on June 7, 2026; final code should still be built against the repository's chosen headers.

## External Validation
- Used: https://docs.kernel.org/core-api/dma-api-howto.html
  - Validates address-domain separation, DMA mask setup, coherent versus streaming contracts, prohibited buffer sources, direction/ownership rules, mapping error checks, SG count rules, sync lifecycle, `dma_pool`, and coherent-memory barrier requirements.
- Used: https://docs.kernel.org/core-api/dma-api.html
  - Validates current generic DMA API signatures, `dma_max_mapping_size()`, DMA pools, mapping errors, SG merging, unmap/sync counts, noncontiguous allocations, and DMA API debugging.
- Used: https://docs.kernel.org/driver-api/dmaengine/client.html
  - Validates `dma_request_chan()`, `dmaengine_slave_config()`, descriptor preparation, mapping with the DMA channel's device, submit/issue lifecycle, callback context, and modern termination/synchronization requirements.
- Used: https://docs.kernel.org/driver-api/dma-buf.html
  - Validates the boundary between ordinary DMA mapping and cross-driver DMA-BUF sharing, including exporter/importer roles, attachments, SG mappings, CPU access bracketing, fences, and reservation objects.
- Used: https://docs.kernel.org/devicetree/kernel-api.html
  - Validates that DMA coherency is a firmware/platform property and that `dma-coherent`/`dma-noncoherent` handling is not normally implemented as an ad hoc client-driver switch.
- Still needed before final examples:
  - Inspect the exact in-tree DMA provider and consumer binding YAML for the selected target SoC.
  - Validate current DMAengine trace events and `dmatest` configuration on the chosen kernel.
  - Confirm hardware descriptor address width, alignment, segment size/count, boundary, burst, and residue semantics from the actual datasheet.

## Learning Content Brief
- Learning path number: `21`.
- Slug: `dma-and-dma-mapping`.
- Topic scope:
  - Device-visible addressing, DMA masks, coherent and streaming mappings, ownership/cache transitions, scatter-gather, DMAengine client lifecycle, completion/teardown, and practical debugging.
  - Keep DMA controller/provider implementation, full DMA-BUF internals, and subsystem-specific queue frameworks outside the core lesson.
- Related topics:
  - Topic 06: locking, completions, sleeping versus atomic context.
  - Topic 10/11: DT syntax, bindings, and firmware-described DMA channels.
  - Topic 15: IRQ completion and synchronization.
  - Topic 17: SPI DMA-safe buffers and async lifetime.
  - Topic 20: allocator choice and DMA-safety boundary.
  - Topics 29-36: applied framebuffer, network, PCI, V4L2, and ASoC DMA.
  - Topic 37: tracing, fault diagnosis, and DMA API debug.
- Beginner mental model:
  - DMA lets hardware move data without the CPU copying every byte, but the CPU still sets up buffers, addresses, ownership, ordering, completion, and error handling.
  - A CPU pointer is not a device address. Always use the DMA address returned by the API for the exact device performing DMA.
  - Coherent versus streaming is mainly a visibility and ownership contract, not merely an allocation preference.
  - Mapping memory and commanding a DMA controller are separate operations.
- Core mechanism:
  - The device DMA mask describes which DMA addresses hardware can drive.
  - The DMA API translates or maps backing memory into the device's DMA address space and performs required cache maintenance.
  - An IOMMU may make the DMA address differ from physical memory and may merge SG entries.
  - DMAengine associates a consumer with a channel, configures peripheral transfer characteristics, queues descriptors, and invokes completion callbacks.
- Important structs/APIs:
  - Addressing/capability: `dma_addr_t`, `DMA_BIT_MASK()`, `dma_set_mask_and_coherent()`, `dma_set_mask()`, `dma_set_coherent_mask()`, `dma_max_mapping_size()`.
  - Coherent allocation: `dma_alloc_coherent()`, `dma_free_coherent()`, `dmam_alloc_coherent()`, `struct dma_pool`, `dma_pool_create()`, `dma_pool_alloc()`, `dma_pool_free()`, `dma_pool_destroy()`.
  - Streaming: `dma_map_single()`, `dma_map_page()`, `dma_map_sg()`, `dma_mapping_error()`, matching unmap APIs, and `dma_sync_*_for_cpu/device()`.
  - Scatter-gather: `struct scatterlist`, `struct sg_table`, `sg_init_table()`, `sg_set_buf()`, `sg_set_page()`, `for_each_sg()`, `sg_dma_address()`, `sg_dma_len()`.
  - DMAengine: `struct dma_chan`, `struct dma_slave_config`, `struct dma_async_tx_descriptor`, `dma_request_chan()`, `dmaengine_slave_config()`, `dmaengine_prep_slave_sg()`, `dmaengine_prep_dma_cyclic()`, `dmaengine_prep_dma_memcpy()`, `dmaengine_submit()`, `dma_submit_error()`, `dma_async_issue_pending()`, `dmaengine_terminate_sync()`, `dmaengine_terminate_async()`, `dmaengine_synchronize()`, `dma_release_channel()`.
  - Completion: `struct completion`, `init_completion()`, `reinit_completion()`, timeout wait variants, `complete()`.
- Lifecycle/data flow:
  - Probe: establish masks -> acquire channel/resources -> allocate long-lived coherent structures -> initialize locks/completions -> leave hardware idle.
  - Transaction: allocate/obtain payload -> map or sync for device -> configure channel/peripheral -> prepare descriptor -> install callback -> submit/check cookie -> issue pending -> program/start peripheral as required.
  - Completion: acknowledge hardware/error -> stop or advance descriptor -> signal callback/completion -> sync/unmap for CPU -> process or recycle buffer.
  - Error/timeout: stop peripheral -> terminate and synchronize channel -> unmap every successful mapping -> return all owned buffers -> reset hardware if required.
  - Remove: prevent new submissions -> disable IRQ generation -> terminate/synchronize DMA -> free/unmap buffers and descriptors -> release channel and remaining resources.
- Example targets for later:
  - Learning-only coherent descriptor/control buffer showing separate CPU and DMA addresses plus a required write barrier before ownership handoff.
  - Streaming single-buffer example with mask setup, direction, `dma_mapping_error()`, completion timeout, and full unwind.
  - SG mapping example showing returned segment count versus original `nents`.
  - DMAengine slave-client skeleton using a named DT channel and safe terminate/synchronize teardown.
  - Debug workflow using DMA API debug, IOMMU faults, dynamic debug, timeout/status logging, and `dmatest` where applicable.
- Common bugs:
  - Passing virtual/physical addresses or truncated DMA addresses to hardware.
  - Setting no DMA mask or accepting a mask the device cannot really support.
  - Mapping with the wrong or `NULL` device.
  - Ignoring mapping failures or submit errors.
  - Wrong direction, size, device, or count at unmap/sync.
  - CPU touching a streaming buffer while the device owns it.
  - Programming hardware from original SG entries instead of mapped DMA segments.
  - Using the returned SG count when unmapping instead of original `nents`.
  - Missing barriers for coherent descriptor publication.
  - Using stack, `vmalloc`, module-image, or short-lived buffers without a valid subsystem-specific mapping path.
  - Freeing state before DMA and callbacks are synchronized.
  - Treating `GFP_DMA`, `dma-coherent`, or physical contiguity as universal shortcuts.
- Debugging notes:
  - Enable DMA API debugging on a development kernel and inspect `dmesg` plus `/sys/kernel/debug/dma-api/` for mismatched mappings, sizes, devices, directions, and leaks.
  - Inspect IOMMU/SMMU faults for the faulting device, IOVA/DMA address, permissions, and access direction.
  - Log CPU pointer and DMA address separately with correct formats; verify hardware register width before writing the DMA address.
  - On timeout, capture controller status, peripheral status, descriptor/cookie state, IRQ counters, queue ownership, and whether issue-pending/start ordering occurred.
  - Test on a non-coherent or IOMMU-enabled target when portability matters; x86 success does not validate cache ownership.
  - Use `dmatest` only for supported DMAengine controller operations; it does not validate a peripheral client's FIFO/configuration contract.
- Production concerns:
  - Treat mappings and DMA address space as finite resources; pair every successful map with the matching unmap.
  - Keep descriptor and callback state alive through completion or synchronized termination.
  - Validate lengths, segment counts, alignment, boundaries, and address widths before programming hardware.
  - Define timeout, cancellation, reset, and partial-transfer policy.
  - Use precise barriers and I/O accessor ordering for descriptor ownership and doorbells.
  - Avoid assuming coherence, physical contiguity, or one-to-one SG mappings from development hardware.
  - Quiesce hardware before devm-managed buffers are released.
- Interview angles:
  - CPU virtual versus physical versus DMA address.
  - Why `virt_to_phys()` is not a DMA API.
  - Coherent versus streaming mappings and why coherent still needs barriers.
  - Direction semantics and cache ownership.
  - DMA mask negotiation and IOMMU effects.
  - `dma_map_sg()` input count, returned count, hardware iteration, and unmap count.
  - DMA mapping API versus DMAengine.
  - Correct timeout/remove teardown for asynchronous DMA.
  - Diagnosing an IOMMU fault, stale buffer, or mapping leak.
