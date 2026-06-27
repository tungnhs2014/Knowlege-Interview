# 20 - Kernel Memory Management For Drivers

## Learning Goal
After this topic, you should be able to choose an appropriate kernel allocator, select a valid GFP flag for the current execution context, define ownership and lifetime, unwind failures correctly, and recognize when ordinary allocation must hand off to the DMA or userspace-mapping APIs.

The goal is not to memorize allocator names. It is to reason from requirements:

- Who accesses the memory: CPU, device, or userspace?
- Must it be physically contiguous, or only virtually contiguous?
- May the caller sleep?
- How large and frequent is the allocation?
- Who owns it, and what proves the last user is gone?
- What happens when allocation fails?

## Why This Matters In Real Work
Almost every driver allocates private state, request objects, descriptors, temporary buffers, or subsystem data. Memory bugs are especially dangerous in kernel space because a leak, overflow, double free, or use-after-free can corrupt unrelated subsystems or crash the machine.

Memory decisions appear in:

- `probe()` and `remove()` for per-device state;
- `open()` and `release()` for per-file state;
- IRQ, timer, workqueue, and completion paths;
- I2C, SPI, network, video, storage, and DMA buffers;
- ioctl or firmware parsing with variable-size arrays;
- userspace `mmap()` support;
- suspend, unbind, module unload, and probe-failure cleanup.

**Production rule:** allocation is only half the design. Every object also needs an owner, a failure path, synchronization, and a precise last-use event.

## Mental Model
An allocation API does not return generic interchangeable bytes. It returns memory with a particular set of properties.

```text
Allocation request
  |
  +-- Size and shape
  +-- Zeroing and overflow safety
  +-- Sleepable or atomic context
  +-- Virtual/physical contiguity
  +-- CPU, device, or userspace access
  +-- Ownership and lifetime
  +-- Failure policy
  |
  v
Choose allocator and GFP flags
  |
  v
Initialize completely
  |
  v
Publish pointer to other contexts
  |
  v
Stop new users and synchronize old users
  |
  v
Free with the matching API
```

Keep these address spaces separate:

| Address or pointer | Used by | Meaning |
| --- | --- | --- |
| Kernel virtual address | CPU/kernel code | Pointer returned by allocators such as `kmalloc()` |
| CPU physical address | CPU/MMU internals | Physical RAM address; not automatically a device address |
| DMA address | Device | Address returned by the DMA API for a particular device |
| MMIO address | Device registers | Hardware resource mapped as `void __iomem *` |
| Userspace virtual address | Process | Address in a process VMA, possibly mapped by a driver |

**Interview trap:** a valid CPU pointer does not imply that hardware can use it. `virt_to_phys()` is not a replacement for the DMA API.

## Core Concepts
The key choices become easier when you compare contiguity, context, and lifetime explicitly.

### Virtual And Physical Contiguity
A region can be contiguous in the CPU's virtual address space while its physical pages are scattered.

| Property | `kmalloc()` family | `vmalloc()` family |
| --- | --- | --- |
| Kernel virtual address | Contiguous | Contiguous |
| Physical backing | Contiguous for the allocation | May be non-contiguous |
| Typical use | Small objects and buffers | Larger software-only buffers |
| Allocation cost | Usually lower | Usually higher due to page mappings |
| Direct ordinary DMA buffer | May be eligible for streaming mapping | Do not assume it is eligible as one buffer |
| Matching free | `kfree()` | `vfree()` |

Physical contiguity becomes harder to obtain as allocation size and system fragmentation grow. This is why a large `kmalloc()` may fail even when total free memory is much larger than the request.

### Pages, PFNs, And Orders
Physical memory is managed in page-sized units. The page size is architecture/configuration dependent and is exposed through `PAGE_SIZE`; do not hard-code 4096.

- `struct page` represents a physical page in kernel memory management.
- A page frame number, or PFN, identifies a physical page frame.
- `alloc_pages(gfp, order)` requests `2^order` physically contiguous pages.
- Order `0` means one page; order `1` means two; order `2` means four.
- Higher-order requests are more sensitive to fragmentation.

The page allocator uses a buddy system that splits and coalesces power-of-two page blocks. Slab/SLUB allocators build efficient small-object caches on top of those pages.

### Slab And SLUB
Drivers normally use the `kmalloc()` family rather than calling the slab allocator directly.

- The allocator maintains caches for common object sizes.
- A small request is rounded to a suitable cache size.
- Reuse improves speed and reduces fragmentation for small objects.
- A private `struct kmem_cache` is justified only for many identical objects with real alignment, construction, performance, or debugging needs.

### GFP Flags Describe Allowed Behavior
GFP flags tell the allocator what it may do to satisfy the request. They are part of the calling-context contract.

| Flag | Typical context | Important behavior |
| --- | --- | --- |
| `GFP_KERNEL` | Normal process context | May sleep and enter direct reclaim |
| `GFP_NOWAIT` | Non-sleeping optimistic attempt | Does not use emergency reserves; caller needs fallback |
| `GFP_ATOMIC` | Genuine atomic/IRQ path | Does not sleep and may use reserves; can still fail |
| `GFP_NOIO` | Reclaim-sensitive I/O path | Avoids starting physical I/O during reclaim |
| `GFP_NOFS` | Filesystem recursion-sensitive path | Avoids filesystem reclaim entry |
| `GFP_KERNEL_ACCOUNT` | Accounted kernel allocations | Charges suitable memory to kernel memory accounting |

**Production rule:** first redesign to allocate before taking a spinlock, defer work, or preallocate. Use `GFP_ATOMIC` only when allocation truly must happen in atomic context.

### Ownership And Lifetime
The allocation site should state the owner and the final release event.

| Lifetime | Common owner | Allocation/free pattern |
| --- | --- | --- |
| Device binding | `struct device` / driver | `devm_kzalloc()` in probe; devres release after detach |
| Module | Module | Allocate in init; free in exit |
| Open file | `struct file` | Allocate in `open()`; store in `private_data`; free in `release()` |
| Request/transaction | Queue or request | Allocate before submit; free after completion/cancellation |
| Shared asynchronous object | References/callbacks | Remove lookup paths, synchronize users, free after final reference |

Unlinking an object from a list does not free it. Cancelling one callback does not prove that an IRQ, timer, or another worker cannot still reach it.

### Managed Resources
Devres associates release actions with a `struct device`.

Benefits:

- simplifies probe-failure unwind;
- automatically releases managed resources on detach;
- reduces duplicated cleanup labels;
- expresses device-binding lifetime clearly.

Limits:

- it does not stop hardware;
- it does not cancel work or timers;
- it does not synchronize IRQ handlers;
- it does not invalidate existing file descriptors or VMAs;
- it cannot make a device-lifetime allocation safe for users that outlive unbind.

### Overflow-Safe Allocation
Never allocate a user- or firmware-controlled array with unchecked multiplication.

```c
items = kmalloc(count * sizeof(*items), GFP_KERNEL); /* risky */
```

Use:

- `kcalloc(count, sizeof(*items), gfp)` for a zeroed array;
- `kmalloc_array(count, sizeof(*items), gfp)` for an uninitialized array;
- `krealloc_array()` for resizing;
- `struct_size(ptr, member, count)` for a structure with a flexible array;
- an explicit semantic maximum to prevent valid-but-excessive allocation.

Overflow-safe arithmetic is necessary but not sufficient. The driver must also reject unreasonable resource consumption.

### DMA-Safe Memory Boundary
Ordinary allocation and DMA mapping solve different problems.

```text
CPU storage choice                Device-visible mapping
------------------                ----------------------
kmalloc/pages/etc.       +----->  dma_map_*() for streaming DMA
                                or dma_alloc_*() for coherent DMA
```

- Do not program a device with `virt_to_phys()`.
- Do not add `GFP_DMA` as a general "DMA-safe" switch.
- Do not use stack, module-image, arbitrary static, or ordinary `vmalloc()` memory as one streaming DMA buffer.
- Configure the device's DMA mask and use the generic DMA API.
- Detailed coherent, streaming, scatter-gather, and cache ownership rules belong to Topic 21.

## Kernel Mechanism
Kernel memory allocation is layered so drivers can request objects without implementing physical memory management.

```text
Physical pages
  |
  v
Buddy/page allocator
  |  alloc_pages(), __free_pages()
  |
  +----> Slab/SLUB object caches
  |        |
  |        +----> kmalloc(), kzalloc(), kcalloc()
  |
  +----> Page collections and virtual mappings
           |
           +----> vmalloc(), vzalloc()
```

### Page Allocation
The buddy allocator tracks free blocks by order.

1. Find a free block of the requested order.
2. If only a larger block exists, split it into buddies.
3. Return one block and retain the other in a lower-order free list.
4. On free, coalesce compatible free buddies when possible.

This mechanism is efficient, but long-lived allocations can prevent coalescing. A high-order request therefore depends on free-memory layout, not just total bytes available.

### Small-Object Allocation
Slab/SLUB obtains pages and divides them into reusable object slots.

1. `kmalloc(size, gfp)` selects a suitable size cache.
2. The allocator obtains a free object from a slab.
3. `kzalloc()` additionally clears the requested memory.
4. `kfree()` returns the object to the allocator.

Zeroing raw bytes does not initialize semantic kernel objects:

- call `mutex_init()` or `spin_lock_init()`;
- initialize list heads with `INIT_LIST_HEAD()`;
- initialize work, timers, completions, and references explicitly;
- set state fields deliberately.

### Virtually Contiguous Allocation
`vmalloc()` allocates pages and constructs page-table mappings that present them as one kernel virtual range.

- CPU code can index the result as one byte array.
- Physical pages need not be adjacent.
- Translation and mapping overhead is greater than for ordinary small slab allocation.
- Consumers that require physical contiguity cannot assume it.

`kvmalloc()` is useful when either backing is acceptable. The caller must use `kvfree()` and must not later pass the pointer to an API that requires slab-backed or physically contiguous memory.

### Allocation Under Memory Pressure
`GFP_KERNEL` allocation may enter reclaim and sleep while memory is freed or compacted. This is useful in normal task context but illegal while holding a spinlock or running in hard IRQ context.

Paths involved in reclaim or storage completion can deadlock if they allocate while waiting for work that itself needs memory. Solutions include:

- preallocating objects;
- using bounded pools or rings;
- using a mempool when guaranteed forward progress is required;
- moving allocation to a sleepable worker;
- applying scoped reclaim restrictions only where the subsystem requires them.

### Publication And Final Release
Memory becomes dangerous when a pointer is published to other contexts.

```text
allocate
  -> initialize all fields
  -> initialize locks/work/references
  -> publish/register/queue
  -> concurrent use
  -> stop new use
  -> remove from lookup paths
  -> stop producers
  -> synchronize callbacks
  -> final free
```

Locking protects state transitions, but locks alone do not establish lifetime. Use reference counting or another explicit ownership protocol when users can retain the object beyond one protected operation.

### MMIO And Userspace Mapping Are Different Problems
MMIO resources are device registers, not allocated RAM.

- obtain the resource through the bus/platform framework;
- map it with a helper such as `devm_ioremap_resource()`;
- retain the `__iomem` type;
- access it with appropriate I/O accessors;
- never release it with `kfree()`.

A driver `mmap()` callback creates a userspace ABI and a VMA lifetime. The mapping helper depends on the backing type. Validate offsets, lengths, permissions, and what happens during `fork()`, close, and device unbind. Do not copy an old `remap_pfn_range()` recipe to every kind of memory.

## Key Structs And APIs
Use this section as a decision reference, not as a memorization list.

### Small Objects And Arrays
| API | Use |
| --- | --- |
| `kmalloc(size, gfp)` | Ordinary small uninitialized object/buffer |
| `kzalloc(size, gfp)` | Ordinary zeroed object/buffer |
| `kcalloc(n, size, gfp)` | Zeroed overflow-safe array |
| `kmalloc_array(n, size, gfp)` | Uninitialized overflow-safe array |
| `krealloc(ptr, size, gfp)` | Resize one allocation |
| `krealloc_array(ptr, n, size, gfp)` | Overflow-safe array resize |
| `kfree(ptr)` | Release `kmalloc`-family memory |
| `kfree_sensitive(ptr)` | Clear a sensitive allocation before release |

### Large Or Flexible Software Buffers
| API | Use |
| --- | --- |
| `vmalloc(size)` | Virtually contiguous, not necessarily physically contiguous |
| `vzalloc(size)` | Zeroed `vmalloc` allocation |
| `vfree(ptr)` | Release `vmalloc`-family memory |
| `kvmalloc(size, gfp)` | Try `kmalloc`, then allow `vmalloc` fallback |
| `kvzalloc(size, gfp)` | Zeroed `kvmalloc` form |
| `kvcalloc(n, size, gfp)` | Zeroed overflow-safe array with fallback |
| `kvfree(ptr)` | Release either `kmalloc` or `vmalloc` backing |
| `kvfree_sensitive(ptr, len)` | Clear sensitive `kvmalloc`-family memory before release |

### Pages
| API/type | Use |
| --- | --- |
| `struct page` | Kernel representation of a physical page |
| `alloc_page(gfp)` | Allocate one page |
| `alloc_pages(gfp, order)` | Allocate `2^order` contiguous pages |
| `__free_pages(page, order)` | Release pages with the same order |
| `get_zeroed_page(gfp)` | Allocate one zeroed page and return a kernel address |
| `page_to_pfn()` / `pfn_to_page()` | Convert between page and PFN where valid |

### Managed Device Lifetime
| API | Use |
| --- | --- |
| `devm_kmalloc(dev, size, gfp)` | Managed uninitialized allocation |
| `devm_kzalloc(dev, size, gfp)` | Managed zeroed allocation |
| `devm_kcalloc(dev, n, size, gfp)` | Managed zeroed array |
| `devm_kfree(dev, ptr)` | Early release of a managed allocation when genuinely needed |
| `devm_add_action_or_reset()` | Attach a custom release action or run it immediately on registration failure |

### Repeated Objects And Guaranteed Progress
| API/type | Use |
| --- | --- |
| `struct kmem_cache` | Private cache for many identical objects |
| `kmem_cache_create()` | Create a private object cache |
| `kmem_cache_alloc()` / `kmem_cache_zalloc()` | Allocate from the cache |
| `kmem_cache_free()` | Return one object |
| `kmem_cache_destroy()` | Destroy only after every object is returned |
| `mempool_t` | Maintain a minimum reserve for forward-progress-sensitive paths |

### High Memory
Highmem is primarily relevant to some 32-bit configurations.

- Use `kmap_local_page()` to obtain a temporary local mapping.
- Release it with `kunmap_local()`.
- Do not teach new code around legacy `kmap()` or `kmap_atomic()` patterns.

## Lifecycle / Data Flow
The correct allocator matters, but lifecycle ordering prevents most real driver memory failures.

### Probe With Device-Lifetime State
```text
match device
  -> allocate managed private state
  -> initialize locks, lists, work, references
  -> acquire/map resources
  -> configure hardware
  -> register external interface
  -> return success
```

Rules:

- Publish the object only after required fields are initialized.
- If a managed acquisition fails, return the error; devres unwinds earlier managed resources.
- Manually owned resources still need explicit reverse-order cleanup.
- Propagate lower-layer error codes instead of replacing every failure with `-ENOMEM`.

### Runtime Request
```text
validate count and size
  -> allocate with context-compatible GFP flags
  -> initialize request completely
  -> queue/submit
  -> callback or completion runs
  -> remove from queue
  -> free after final use
```

For asynchronous requests:

- do not queue pointers to stack objects;
- keep descriptors and buffers alive until completion;
- define timeout/cancellation ownership;
- prevent callbacks from requeueing after teardown starts.

### Remove And Unbind
```text
mark stopping
  -> reject new operations
  -> unregister external interfaces
  -> stop hardware producers
  -> disable and synchronize IRQs
  -> stop timers
  -> cancel/flush work and transfers
  -> wait for retained references
  -> free manual objects
  -> return; devres releases managed resources
```

Do not hold a lock needed by a callback while calling a synchronous cancellation helper such as `cancel_work_sync()`.

### Open File Lifetime
```text
open
  -> allocate per-file context
  -> store in file->private_data
  -> read/write/ioctl use the context
release
  -> stop per-file asynchronous work
  -> free the context
```

If multiple descriptors or a VMA can share the state, use a shared owner and reference count instead of freeing on the first close.

## Minimal Practical Example
This learning-only platform-driver fragment demonstrates device-managed state, an overflow-safe runtime array, workqueue lifetime, and explicit teardown ordering. It omits real hardware and subsystem registration, so it is **not production-ready**.

```c
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#define MY_MAX_SAMPLES 4096

struct my_sample {
	u32 value;
};

struct my_device {
	struct device *dev;
	struct work_struct work;
	struct mutex lock;
	struct my_sample *samples;
	size_t sample_count;
	bool stopping;
};

static void my_process_work(struct work_struct *work)
{
	struct my_device *d =
		container_of(work, struct my_device, work);

	mutex_lock(&d->lock);
	if (!d->stopping) {
		/* Process d->samples while the object is still owned. */
	}
	mutex_unlock(&d->lock);
}

static int my_resize_samples(struct my_device *d, size_t count)
{
	struct my_sample *new_samples;

	if (!count || count > MY_MAX_SAMPLES)
		return -EINVAL;

	new_samples = kcalloc(count, sizeof(*new_samples), GFP_KERNEL);
	if (!new_samples)
		return -ENOMEM;

	mutex_lock(&d->lock);
	if (d->stopping) {
		mutex_unlock(&d->lock);
		kfree(new_samples);
		return -ENODEV;
	}

	kfree(d->samples);
	d->samples = new_samples;
	d->sample_count = count;
	mutex_unlock(&d->lock);

	return 0;
}

static int my_probe(struct platform_device *pdev)
{
	struct my_device *d;
	int ret;

	d = devm_kzalloc(&pdev->dev, sizeof(*d), GFP_KERNEL);
	if (!d)
		return -ENOMEM;

	d->dev = &pdev->dev;
	mutex_init(&d->lock);
	INIT_WORK(&d->work, my_process_work);
	platform_set_drvdata(pdev, d);

	ret = my_resize_samples(d, 128);
	if (ret)
		return ret;

	return 0;
}

static void my_remove(struct platform_device *pdev)
{
	struct my_device *d = platform_get_drvdata(pdev);

	mutex_lock(&d->lock);
	d->stopping = true;
	mutex_unlock(&d->lock);

	/* Stop IRQ/timer/hardware producers before this in a real driver. */
	cancel_work_sync(&d->work);

	mutex_lock(&d->lock);
	kfree(d->samples);
	d->samples = NULL;
	d->sample_count = 0;
	mutex_unlock(&d->lock);

	/* d itself is released by devres after remove returns. */
}
```

Important lines:

- `devm_kzalloc()` fits private state whose lifetime is the device binding.
- `kcalloc()` checks multiplication overflow and zeroes the array.
- Allocation occurs before taking the mutex because `GFP_KERNEL` may sleep.
- State is revalidated under the mutex before publishing the new pointer.
- Remove sets a stopping state and synchronously cancels work before freeing the worker's data.
- The runtime array is manual because it can be replaced independently of device lifetime.

Production additions would include:

- hardware and IRQ shutdown before work cancellation;
- subsystem unregister ordering;
- a clear rule preventing new work after `stopping`;
- reference handling if userspace can retain the object through unbind;
- fault-injection tests for every allocation and registration failure.

## Common Bugs And Debugging
Start from the observable symptom, then reconstruct context and ownership.

### "Sleeping Function Called From Invalid Context"
Likely causes:

- `GFP_KERNEL` allocation in hard IRQ context;
- allocation while holding a spinlock;
- a helper that allocates or performs reclaim below an atomic caller.

Debug:

- keep the first complete warning and stack trace;
- inspect held locks, interrupt state, and all callers;
- move allocation before the lock, defer work, or preallocate;
- use `GFP_ATOMIC` only if atomic allocation is unavoidable and failure is handled.

### Use-After-Free During Unbind
Likely causes:

- queued work or timer fires after device-managed state is released;
- IRQ can requeue work after cancellation;
- an open file or VMA retains a raw device pointer;
- callback cancellation is asynchronous rather than synchronous.

Debug:

- enable KASAN and compare allocation, free, and access stacks;
- list every producer of the callback;
- stop producers before cancellation;
- run repeated bind/unbind under maximum I/O and interrupt load.

### Large Allocation Fails After Uptime
Likely cause: physical fragmentation prevents a high-order or large contiguous allocation.

Inspect:

```bash
cat /proc/meminfo
cat /proc/buddyinfo
cat /proc/pagetypeinfo
cat /proc/vmallocinfo
dmesg | grep -i "page allocation failure"
```

Fix patterns:

- use `kvmalloc()` for CPU-only buffers whose consumers accept either backing;
- allocate independent pages;
- use scatter-gather or the DMA API for capable devices;
- avoid large runtime physically contiguous requirements.

### Memory Leak On Probe Failure
Likely causes:

- cleanup label skips an acquired resource;
- object was removed from a list but not freed;
- subsystem registration succeeded but was not undone;
- work or IRQ retains the allocation.

Use kmemleak:

```bash
mount -t debugfs none /sys/kernel/debug
echo clear > /sys/kernel/debug/kmemleak
# Trigger the failing path.
echo scan > /sys/kernel/debug/kmemleak
cat /sys/kernel/debug/kmemleak
```

Kmemleak is useful for orphaned `kmalloc`, `vmalloc`, and slab objects, but it does not detect every memory type or logical leak.

### Out-Of-Bounds Or Double Free
Use:

- KASAN for precise out-of-bounds and use-after-free reports;
- KFENCE for lower-overhead sampled heap checking during longer runs;
- SLUB debugging, poisoning, red zones, and `slabinfo` for allocator/cache corruption;
- allocation fault injection to execute rarely tested error paths.

Root causes often include:

- integer overflow in size calculation;
- stale length after `krealloc`;
- mismatched owner and free site;
- duplicate cleanup in manual and devm paths;
- callback completion racing with timeout cleanup.

### Information Exposure
`kzalloc()` prevents stale initial bytes, but later data and structure padding can still leak.

- initialize every field copied to userspace or hardware;
- prefer explicit serialization over copying raw internal structures;
- bound transfer lengths;
- use `kfree_sensitive()` or `kvfree_sensitive()` for secrets when appropriate;
- do not clear memory before an asynchronous consumer has finished.

## Production Checklist
Use this checklist during design, review, and stress testing.

### Allocation Choice
- [ ] The allocator is chosen from consumer and contiguity requirements, not size alone.
- [ ] Large allocations do not impose unnecessary physical contiguity.
- [ ] Every variable-size array uses overflow-safe helpers.
- [ ] User/firmware-controlled counts have semantic maximums.
- [ ] The GFP flag matches every possible calling context.
- [ ] Atomic allocation has a defined failure or preallocation policy.
- [ ] DMA buffers use the DMA API rather than physical-address conversion.

### Ownership And Lifetime
- [ ] Every allocation has one documented owner.
- [ ] The final release event is explicit.
- [ ] Objects are fully initialized before publication.
- [ ] Async descriptors, buffers, and callback contexts survive completion.
- [ ] Shared objects use reference counting or another clear lifetime protocol.
- [ ] Device-managed memory is used only when device binding is the true lifetime boundary.
- [ ] Existing file descriptors and VMAs are considered during unbind.

### Error Paths
- [ ] Every allocation result is checked.
- [ ] Lower-layer errors are propagated where appropriate.
- [ ] Manual resources unwind in reverse acquisition order.
- [ ] Manual and managed cleanup do not release the same object twice.
- [ ] Probe failures have been exercised with fault injection.

### Concurrency And Teardown
- [ ] No sleepable allocation occurs under a spinlock or in hard IRQ context.
- [ ] New operations are blocked before teardown frees shared state.
- [ ] Hardware, IRQs, timers, and other producers stop before callback cancellation.
- [ ] Synchronous cancellation is not called while holding a callback-required lock.
- [ ] Remove/unbind stress tests run concurrently with I/O and interrupts.

### Security And Debugging
- [ ] No uninitialized bytes or structure padding cross an ABI.
- [ ] Sensitive data is explicitly cleared when required.
- [ ] KASAN or KFENCE has been used for lifetime and bounds testing.
- [ ] Kmemleak has been used for repeated failure/bind/unbind testing where applicable.
- [ ] SLUB diagnostics are available for cache corruption.
- [ ] Large-allocation behavior has been tested after realistic memory churn.

## Interview Readiness
You should be able to explain the following without relying on slogans:

- why allocator choice depends on contiguity, context, consumer, and lifetime;
- `kmalloc()` vs `kzalloc()` vs `vmalloc()` vs `kvmalloc()`;
- why `GFP_KERNEL` may sleep and why `GFP_ATOMIC` can still fail;
- how the page allocator, buddy system, and slab/SLUB layers relate;
- why large contiguous allocations become unreliable;
- how to allocate arrays without integer overflow;
- what devres solves and what it cannot synchronize;
- why `virt_to_phys()` is not a DMA API;
- how probe failure, open-file state, workqueues, IRQs, and unbind affect ownership;
- how KASAN, KFENCE, kmemleak, and SLUB debugging answer different questions.

Practice with [20 - Kernel Memory Management For Drivers Interview Questions](../interview/20-kernel-memory-management-for-drivers.md).

## Kernel Version Notes
Memory-management internals and helper availability vary across kernel versions and architectures.

- Treat fixed 3G/1G layouts, 896 MiB lowmem boundaries, 4 KiB pages, and fixed maximum `kmalloc()` sizes as historical examples, not portable rules.
- Prefer `kmap_local_page()`/`kunmap_local()` over legacy `kmap()` or `kmap_atomic()` patterns in new code.
- Ordinary `kzalloc()` memory is released with `kfree()`; use current sensitive-free helpers such as `kfree_sensitive()` when clearing is required.
- `readl()`/`writel()` remain valid MMIO accessors; they are not generally deprecated.
- Validate `mmap()` helpers, VMA flags, DMA APIs, and allocator helpers against the target Linux tree before publishing compilable production code.
