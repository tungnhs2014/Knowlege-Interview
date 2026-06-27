# 20 - Kernel Memory Management For Drivers Example

This is a **learning-only** kernel module. It does not bind to hardware and is
not production-ready. It demonstrates allocator selection, overflow-safe array
allocation, asynchronous lifetime, deterministic init failure, and matching
cleanup.

## Goal

Observe this allocation and lifetime flow:

```text
module init
  -> kzalloc() one small state object
  -> kcalloc() a bounded, overflow-safe array
  -> kvzalloc() a larger software-only buffer
  -> alloc_page() one zeroed page
  -> initialize all objects
  -> queue delayed work
delayed work
  -> read the array, blob, and page metadata
module exit
  -> cancel/synchronize delayed work
  -> free page, blob, array, and state in reverse order
```

The example demonstrates:

- `kzalloc()` and `kfree()` for a small state object;
- `kcalloc()` for overflow-safe array sizing;
- `kvzalloc()` and `kvfree()` when either kmalloc or vmalloc backing is valid;
- `alloc_page()` and `__free_page()` for page-granular allocation;
- `GFP_KERNEL` only in sleepable module-init context;
- a delayed work item that must stop before its buffers are freed;
- parameter validation before allocation;
- failure injection after each allocation stage;
- reverse-order cleanup shared by all failed-init paths.

The example deliberately excludes DMA. A CPU pointer, physical address, and DMA
address are different things; Topic 21 covers `dma_alloc_*()` and `dma_map_*()`.

## Kernel Version Assumptions

Build against the exact kernel headers for the kernel that will load the module:

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

The example was compiled successfully against Linux `6.8.0-124-generic`
headers. It uses conventional Linux 6.x module APIs:

- `kvzalloc()` and `kvfree()`;
- `kcalloc()` and `array_size()`;
- `alloc_page()` and `page_to_pfn()`;
- `INIT_DELAYED_WORK()` and `cancel_delayed_work_sync()`;
- read-only module parameters.

These APIs exist across many kernels, but always validate signatures and
configuration against the target tree. Page size, allocator limits, physical
layout, and whether `kvzalloc()` falls back to vmalloc backing are not portable
constants.

## Files

| File | Purpose |
| --- | --- |
| `kmem_demo.c` | Allocation, delayed-work lifetime, failure injection, and cleanup demonstration |
| `Makefile` | Out-of-tree Kbuild wrapper |
| `README.md` | Build, load, test, expected logs, debugging, ABI, and cleanup notes |

No DTS is needed because the module does not bind to hardware. No userspace test
program is needed because module parameters and kernel logs exercise every path.

## Build

From this directory:

```sh
make
```

Equivalent command:

```sh
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

For a target kernel:

```sh
make KDIR=/path/to/target/kernel/build \
     ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

Inspect the module and its parameters:

```sh
modinfo ./kmem_demo.ko
modinfo -p ./kmem_demo.ko
modinfo -F vermagic ./kmem_demo.ko
```

Expected parameters:

```text
entries:Number of array entries (1..65536) (uint)
blob_kb:Software-only kvzalloc buffer size in KiB (1..16384) (uint)
delay_ms:Delay before the work callback runs in milliseconds (uint)
fail_step:Inject init failure after allocation step 1..4 (uint)
```

Clean generated files:

```sh
make clean
```

## Load And Test

Watch logs in one terminal:

```sh
sudo dmesg -C
sudo dmesg -w
```

Load with defaults:

```sh
sudo insmod ./kmem_demo.ko
lsmod | grep kmem_demo
```

Expected log shape:

```text
kmem_demo: loaded: entries=128 array_bytes=1024 blob=262144 bytes delay_ms=200
kmem_demo: allocation roles: kzalloc state, kcalloc array, kvzalloc blob, alloc_page page
kmem_demo: work: entries=128 sum=8256 blob=262144 bytes edge=a5/5a page_pfn=<n>
```

Unload after the work has run:

```sh
sudo rmmod kmem_demo
dmesg | tail -20
```

Expected cleanup shape:

```text
kmem_demo: exit: work had completed before teardown
kmem_demo: cleanup: page released
kmem_demo: cleanup: blob released with kvfree
kmem_demo: cleanup: entry array released with kfree
kmem_demo: cleanup: state released with kfree
kmem_demo: unloaded
```

Try different valid sizes:

```sh
sudo insmod ./kmem_demo.ko entries=1024 blob_kb=4096 delay_ms=50
sleep 1
sudo rmmod kmem_demo
```

The array sum is:

```text
entries * (entries + 1) / 2
```

For `entries=1024`, the expected sum is `524800`.

## Test Asynchronous Cleanup

Queue work far in the future and unload immediately:

```sh
sudo insmod ./kmem_demo.ko delay_ms=30000
sudo rmmod kmem_demo
dmesg | tail -20
```

Expected key line:

```text
kmem_demo: exit: pending work canceled before it ran
```

This is the central lifetime lesson: delayed work stores a pointer to the
enclosing state, so module exit must call `cancel_delayed_work_sync()` before
freeing any allocation used by the callback.

## Test Validation And Error Paths

Invalid counts fail before allocation:

```sh
sudo insmod ./kmem_demo.ko entries=0
echo $?

sudo insmod ./kmem_demo.ko entries=65537
echo $?

sudo insmod ./kmem_demo.ko blob_kb=16385
echo $?
```

The module should not remain loaded:

```sh
lsmod | grep kmem_demo
```

Inject failure after each successful allocation:

```sh
for step in 1 2 3 4; do
    echo "fail_step=$step"
    sudo insmod ./kmem_demo.ko fail_step=$step
    echo "insmod status=$?"
    lsmod | grep kmem_demo || true
done
```

Each insertion fails with `-EIO`, the module is not left loaded, and only
resources acquired before that failure are released.

Example for `fail_step=3`:

```text
kmem_demo: injecting failure after allocation step 3
kmem_demo: cleanup: blob released with kvfree
kmem_demo: cleanup: entry array released with kfree
kmem_demo: cleanup: state released with kfree
```

The cleanup function accepts partially initialized state, so every failed-init
path can use the same reverse-order release logic.

## What To Observe

### Small State With `kzalloc`

The enclosing `struct kmem_demo` is small and needs zeroed pointers and counters.
`kzalloc()` is the normal choice, followed by explicit initialization of
semantic objects such as the completion and delayed work.

Zeroing bytes does not initialize every kernel object automatically.

### Array With `kcalloc`

The entry count is bounded and the array uses:

```c
kcalloc(entries, sizeof(*demo->entries), GFP_KERNEL)
```

This checks multiplication overflow inside the allocator helper. The explicit
maximum also prevents a valid but excessive allocation request.

### Software Buffer With `kvzalloc`

The blob is used only through normal CPU byte access. The module does not depend
on physical contiguity, so `kvzalloc()` may use kmalloc backing or fall back to
vmalloc backing.

The caller releases it with `kvfree()` because it must accept either form.

### One Page With `alloc_page`

`alloc_page(GFP_KERNEL | __GFP_ZERO)` requests one order-0 page. The example
prints its PFN but does not expose or DMA-map it.

The matching release is `__free_page()`.

### Workqueue Lifetime

The work callback reads the array and blob. Therefore:

```text
stop/synchronize the work
  -> release the page
  -> release the blob
  -> release the array
  -> release the enclosing state
```

Freeing first would create a use-after-free.

## Debug

Basic inspection:

```sh
dmesg | grep -E 'kmem_demo|allocation failure'
journalctl -k -n 50
lsmod | grep kmem_demo
cat /proc/meminfo
cat /proc/slabinfo | head
cat /proc/vmallocinfo | tail
```

Trace delayed-work execution when tracefs is available:

```sh
sudo mount -t tracefs nodev /sys/kernel/tracing 2>/dev/null || true
echo workqueue:workqueue_queue_work | \
  sudo tee /sys/kernel/tracing/set_event
sudo cat /sys/kernel/tracing/trace_pipe
```

Disable events afterward:

```sh
echo | sudo tee /sys/kernel/tracing/set_event
```

Useful debug-kernel features:

- KASAN for out-of-bounds, use-after-free, and double-free reports;
- KFENCE for lower-overhead sampled heap error detection;
- kmemleak for orphaned kmalloc/vmalloc/slab allocations;
- SLUB debugging for poisoning, red zones, and cache corruption;
- allocation fault injection for real error-path coverage.

Kmemleak workflow when supported:

```sh
sudo mount -t debugfs none /sys/kernel/debug 2>/dev/null || true
echo clear | sudo tee /sys/kernel/debug/kmemleak
sudo insmod ./kmem_demo.ko
sleep 1
sudo rmmod kmem_demo
echo scan | sudo tee /sys/kernel/debug/kmemleak
sudo cat /sys/kernel/debug/kmemleak
```

The correct example should not leave an object reported as an unreferenced
allocation. Kmemleak has limitations and does not prove the absence of every
kind of leak.

## Cleanup And Error-Path Explanation

Normal exit first handles the asynchronous consumer:

```text
cancel_delayed_work_sync()
  -> __free_page()
  -> kvfree()
  -> kfree(array)
  -> kfree(state)
```

The order is intentional:

- delayed work can dereference every allocation;
- synchronous cancellation closes the callback lifetime before freeing;
- page, blob, and array were acquired after the state and are released first;
- each pointer is set to `NULL` after release to make partial cleanup easier to
  inspect;
- `kfree(NULL)` and `kvfree(NULL)` are safe, and the page is guarded explicitly;
- module-init failure occurs before work is queued, so no cancellation is needed
  on the init error path.

The example uses one centralized cleanup helper. A real driver may use cleanup
labels or devres, but neither approach removes the need to stop asynchronous
users before storage disappears.

## Userspace ABI Impact

The module creates no:

- `/dev` node;
- ioctl;
- sysfs device attribute;
- procfs file;
- debugfs file;
- memory mapping.

It exposes read-only module parameters under:

```text
/sys/module/kmem_demo/parameters/entries
/sys/module/kmem_demo/parameters/blob_kb
/sys/module/kmem_demo/parameters/delay_ms
/sys/module/kmem_demo/parameters/fail_step
```

These parameters and kernel logs are demonstration controls, **not a promised
stable userspace ABI**.

## Why This Is Not Production-Ready

- It is a synthetic global module, not a per-device driver.
- It performs no useful hardware or subsystem work.
- It prints PFN metadata only for learning; production drivers rarely need it.
- It does not demonstrate a real IRQ, DMA mapping, userspace VMA, mempool, or
  reference-counted open-file lifetime.
- Its failure injection is module-parameter based rather than the kernel fault
  injection framework.
- It intentionally keeps allocator examples together; production code should
  allocate only what its real consumers require.

Production code would bind storage to the correct device/request/file owner,
define limits from a real ABI or hardware contract, stop all producers during
teardown, test under memory pressure, and use the DMA API when hardware accesses
memory.
