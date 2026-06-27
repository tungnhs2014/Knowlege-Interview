# 21 - DMA And DMA Mapping Example

This is a **learning-only** kernel module. It demonstrates valid DMA mapping API
lifecycle against a synthetic platform device, but it does not control hardware
and performs no DMA transfer.

## Goal

Observe three DMA API patterns without hiding cleanup:

```text
synthetic platform device
  -> set device DMA mask
  -> allocate one coherent region
  -> map/unmap one streaming buffer
  -> map/unmap one scatterlist
  -> keep coherent memory until remove
  -> unregister device and release memory
```

The example demonstrates:

- separate CPU pointers and `dma_addr_t` values;
- `dma_set_mask_and_coherent()` with checked failure;
- `dma_alloc_coherent()` / `dma_free_coherent()`;
- `dma_map_single()`, `dma_mapping_error()`, and matching unmap;
- `dma_map_sg()` failure handling;
- mapped SG count versus original input count;
- `sg_dma_address()` and `sg_dma_len()`;
- cleanup after deterministic probe failures;
- device removal before driver unregistration.

It deliberately does not:

- claim that mapping memory transfers data;
- program a DMA controller or peripheral;
- invent MMIO registers or a Device Tree binding;
- demonstrate DMAengine, interrupts, cyclic DMA, or hardware reset.

Use the kernel's `dmatest` module for an actual DMAengine memory-copy test when
the target has a supported DMA controller.

## Kernel Version Assumptions

Build against the exact headers for the kernel that will load the module:

```sh
uname -r
ls /lib/modules/$(uname -r)/build
```

The example was compiled against Linux `6.8.0-124-generic` headers. It uses:

- `struct platform_device_info.dma_mask`;
- `platform_device_register_full()`;
- `platform_driver.remove_new`;
- `PROBE_FORCE_SYNCHRONOUS` for deterministic learning-module initialization;
- generic coherent, streaming, and scatter-gather DMA APIs.

The exact platform-driver remove callback has changed across kernel versions.
For older trees, `.remove_new` may need to become the remove callback form used
by that target. Always validate against the target headers.

A synthetic device does not represent a real controller's address width,
alignment, segment, boundary, coherency, IOMMU, or reset constraints. A
successful run proves only that the API lifecycle is accepted on this system.

## Files

| File | Purpose |
| --- | --- |
| `dma_mapping_demo.c` | Synthetic platform device plus coherent, streaming, and SG mapping demonstrations |
| `Makefile` | Out-of-tree Kbuild wrapper |
| `README.md` | Build, load, test, logs, cleanup, debugging, and ABI notes |

No DTS is included because the module creates its learning device in code. A
real driver must use its documented firmware binding and the DMA provider's
binding-specific `dmas` cells.

## Build

From this directory:

```sh
make
```

Equivalent command:

```sh
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

Cross-build for a prepared target kernel:

```sh
make KDIR=/path/to/target/kernel/build \
     ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

Inspect the result:

```sh
modinfo ./dma_mapping_demo.ko
modinfo -p ./dma_mapping_demo.ko
modinfo -F vermagic ./dma_mapping_demo.ko
```

Expected parameters:

```text
dma_bits:Synthetic device DMA address width: 32 or 64 (uint)
fail_step:Inject probe failure after step 1..3 (uint)
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

Load with a 32-bit DMA mask:

```sh
sudo insmod ./dma_mapping_demo.ko dma_bits=32
lsmod | grep dma_mapping_demo
```

Expected log shape:

```text
dma-mapping-demo dma-mapping-demo: coherent: cpu=<pointer> dma=<address> size=1024
dma-mapping-demo dma-mapping-demo: streaming: cpu=<pointer> dma=<address> size=2048 direction=to-device
dma-mapping-demo dma-mapping-demo: streaming: unmapped with matching device/size/direction
dma-mapping-demo dma-mapping-demo: sg: original_nents=3 mapped_nents=<1..3>
dma-mapping-demo dma-mapping-demo: sg: segment=0 dma=<address> len=<bytes>
dma-mapping-demo dma-mapping-demo: sg: unmapped with original_nents=3
dma-mapping-demo dma-mapping-demo: probe complete; no hardware DMA transfer was performed
dma_mapping_demo: loaded: dma_bits=32 fail_step=0
```

The mapped SG count may equal the original count. Entry merging is
platform-dependent and is not guaranteed by this test.

Unload:

```sh
sudo rmmod dma_mapping_demo
dmesg | tail -30
```

Expected cleanup:

```text
dma-mapping-demo dma-mapping-demo: remove: coherent allocation released
dma_mapping_demo: unloaded
```

Try a 64-bit mask:

```sh
sudo insmod ./dma_mapping_demo.ko dma_bits=64
sudo rmmod dma_mapping_demo
```

Mask setup can fail on a platform that cannot support the requested DMA
capability. That is a valid result and must not be ignored in a real probe path.

## Test Validation And Error Paths

An invalid mask width is validated in module init and fails before driver or
device registration:

```sh
sudo insmod ./dma_mapping_demo.ko dma_bits=48
echo $?
lsmod | grep dma_mapping_demo || true
```

Inject failure after coherent allocation:

```sh
sudo insmod ./dma_mapping_demo.ko fail_step=1
dmesg | tail -20
```

Expected key lines:

```text
injecting failure after coherent allocation
cleanup: coherent allocation released after probe failure
```

Inject failure while the streaming mapping is active:

```sh
sudo insmod ./dma_mapping_demo.ko fail_step=2
dmesg | tail -30
```

The streaming mapping is unmapped before its buffer is freed, then the coherent
allocation is released as probe unwinds.

Inject failure while the SG mapping is active:

```sh
sudo insmod ./dma_mapping_demo.ko fail_step=3
dmesg | tail -30
```

The SG mapping is unmapped with the original count, all SG backing buffers are
freed, and then the coherent allocation is released.

For each injected failure:

```sh
lsmod | grep dma_mapping_demo || true
```

The module should not remain loaded. The driver forces synchronous probing;
module init reads the recorded probe result, unregisters the synthetic device
and driver, and returns the probe error to `insmod`.

## Actual DMAengine Test With `dmatest`

`dmatest` is an in-tree test client for DMAengine controllers. Availability
depends on kernel configuration and target hardware.

Check configuration and module availability:

```sh
grep CONFIG_DMATEST /boot/config-$(uname -r)
modinfo dmatest
```

A common test sequence is:

```sh
sudo modprobe dmatest
echo 1 | sudo tee /sys/module/dmatest/parameters/iterations
echo 4096 | sudo tee /sys/module/dmatest/parameters/test_buf_size
echo 1 | sudo tee /sys/module/dmatest/parameters/run
dmesg | grep -i dmatest
```

Parameter availability and channel selection differ across kernels. Read:

```sh
ls /sys/module/dmatest/parameters
modinfo -p dmatest
```

`dmatest` validates supported DMAengine operations such as memory copy. It does
not validate a peripheral driver's FIFO address, request line, burst settings,
interrupt path, or hardware-specific teardown.

## DMA API Debugging

Check whether the running kernel enables DMA API debug:

```sh
grep CONFIG_DMA_API_DEBUG /boot/config-$(uname -r)
```

On a development kernel with `CONFIG_DMA_API_DEBUG=y`:

```sh
sudo mount -t debugfs none /sys/kernel/debug 2>/dev/null || true
ls /sys/kernel/debug/dma-api
cat /sys/kernel/debug/dma-api/error_count
cat /sys/kernel/debug/dma-api/dump
```

Useful boot parameters include:

```text
dma_debug_driver=dma-mapping-demo
dma_debug_entries=<count>
```

DMA API debug can detect wrong devices, sizes, directions, unmap APIs, missing
mapping-error checks, and mapping leaks. It adds overhead and is intended for
development kernels.

Also inspect:

```sh
dmesg | grep -Ei 'DMA-API|IOMMU|SMMU|fault'
```

## Cleanup And Error Paths

The normal lifetime is:

```text
module init
  -> validate module parameters
  -> register platform driver
  -> register synthetic platform device
  -> probe allocates coherent memory
  -> reject module load and unregister both objects if probe fails
module exit
  -> unregister device
  -> remove frees coherent memory
  -> unregister driver
```

Transient mappings follow stricter local cleanup:

```text
allocate backing
  -> map
  -> check mapping result
  -> no CPU access while mapped
  -> unmap with matching parameters
  -> free backing
```

The SG path programs no hardware, but it still uses:

- returned `mapped_nents` to iterate device-visible segments;
- original `DMA_DEMO_SG_ENTRIES` for `dma_unmap_sg()`.

If real hardware were involved, timeout and remove would first stop the
peripheral and DMA engine and synchronize callbacks before unmapping or freeing
memory.

## Userspace ABI Impact

The module creates no character device, sysfs device attribute, debugfs file,
procfs entry, ioctl, or `mmap()` interface. It therefore defines no custom
userspace ABI.

It exposes two read-only module parameters:

```text
/sys/module/dma_mapping_demo/parameters/dma_bits
/sys/module/dma_mapping_demo/parameters/fail_step
```

These are test controls for this learning module, not a proposed production
ABI. Kernel logs are diagnostic output and must not be parsed as a stable ABI.

## Why This Is Not Production-Ready

The module is intentionally limited:

- the platform device is synthetic;
- no DMA-capable hardware consumes the mappings;
- no DMAengine channel or interrupt is used;
- hardware address, alignment, segment, and boundary limits are absent;
- no power-management, suspend, reset, hot-unplug, or concurrency design exists;
- no non-coherent hardware ownership transition is observable;
- success does not validate a real device's firmware binding or IOMMU setup.

A production DMA driver must derive constraints from the hardware and subsystem,
set correct masks, configure segment limits, implement transfer completion and
error recovery, stop hardware before unmapping, synchronize teardown, and test
on the real target with its IOMMU and cache-coherency configuration.
