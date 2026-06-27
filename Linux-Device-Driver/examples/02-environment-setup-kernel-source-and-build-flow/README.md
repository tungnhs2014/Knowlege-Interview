# 02 - Environment Setup, Kernel Source, And Build Flow Example

This is a **learning-only** external kernel module. It demonstrates how an
out-of-tree module enters the matching kernel's Kbuild environment, how to
inspect the resulting artifact, and where to look when the target rejects it.

It does not control hardware and is not a production driver.

## Goal

Practice this build and verification flow:

```text
select the target kernel build tree
  -> build through Kbuild with M=<module-source>
  -> inspect the .ko architecture and metadata
  -> compare build identity with the running target
  -> load, observe, unload, and clean
```

The important lesson is that successful compilation does not prove target
compatibility. The source tree, configuration, generated headers, symbol
metadata, architecture, toolchain choices, and deployed kernel must form one
matching build contract.

## Kernel Version Assumptions

The C code uses long-established module APIs and should build on many modern
Linux kernels. The target kernel's own headers and Kbuild files remain
authoritative.

The example was build-checked against Ubuntu `6.8.0-124-generic` headers.

For a native distribution-kernel test, this path should exist:

```sh
uname -r
ls -ld /lib/modules/$(uname -r)/build
```

For a custom kernel, set `KDIR` to its configured output directory. A tree
prepared with `modules_prepare` is sufficient for many simple modules, but
that target does not generate `Module.symvers`. Use the matching full kernel
build when complete `CONFIG_MODVERSIONS` symbol data is required.

## Files

| File | Purpose |
| --- | --- |
| `build_flow_demo.c` | Minimal module with load and unload log messages |
| `Makefile` | External-module Kbuild declaration and wrapper |
| `README.md` | Build, inspection, test, debug, and cleanup workflow |

No DTS or userspace test program is needed because the module creates no
device and exposes no userspace interface.

## Build

Build for the currently running distribution kernel:

```sh
make
```

The wrapper runs the equivalent of:

```sh
make -C /lib/modules/$(uname -r)/build M=$PWD modules
```

`-C` enters the selected kernel build tree. `M=$PWD` identifies this external
module source directory. Kbuild then supplies generated headers, compiler
flags, configuration, symbol processing, and module post-processing.

Build against another prepared kernel output tree:

```sh
make KDIR=/path/to/kernel/output
```

For an ARM64 GNU cross-build:

```sh
make KDIR=/path/to/kernel/output \
     ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
```

Use the same `ARCH`, compiler selection, and cross-tool prefix used for the
target kernel. A cross-built ARM64 module cannot be loaded into an x86-64 host
kernel; inspect it locally and deploy it only with its matching target build.

Expected primary artifact:

```text
build_flow_demo.ko
```

Kbuild also creates intermediate files such as `.mod.c`, `.mod.o`,
`modules.order`, and `Module.symvers` in this directory.

## Inspect Before Loading

```sh
file ./build_flow_demo.ko
modinfo ./build_flow_demo.ko
modinfo -F vermagic ./build_flow_demo.ko
uname -r
```

Expected native-build shape:

```text
build_flow_demo.ko: ELF 64-bit LSB relocatable, <host architecture>, ...
<vermagic begins with the running kernel release>
```

`vermagic` is a useful compatibility clue, not complete ABI proof. It does not
replace matching source, configuration, `Module.symvers`, signing policy, and
target testing.

## Load And Test

Only load a module built for the running test kernel:

```sh
sudo insmod ./build_flow_demo.ko
lsmod | grep '^build_flow_demo'
dmesg | tail -20
```

Expected log shape:

```text
build_flow_demo: loaded; built for kernel <kernelrelease>
```

Unload it:

```sh
sudo rmmod build_flow_demo
dmesg | tail -20
```

Expected final line:

```text
build_flow_demo: unloaded
```

## Debug A Load Failure

The short `insmod` error is not enough. Read the kernel log immediately:

```sh
sudo insmod ./build_flow_demo.ko
status=$?
dmesg | tail -50
printf 'insmod status: %s\n' "$status"
```

Then compare:

```sh
uname -m
uname -r
file ./build_flow_demo.ko
modinfo -F vermagic ./build_flow_demo.ko
modinfo -F signer ./build_flow_demo.ko
```

Common diagnoses:

| Symptom or log clue | Likely cause | Correct action |
| --- | --- | --- |
| `Invalid module format` and version-magic detail | Wrong kernel release or build configuration | Rebuild against the exact target output |
| ELF machine or architecture mismatch | Wrong `ARCH` or compiler/toolchain | Rebuild for the target architecture |
| Unknown or disagreeing symbol versions | Missing, stale, or wrong `Module.symvers` | Use the matching full kernel build and rebuild |
| Required key or signature rejection | Secure Boot, lockdown, or product signing policy | Sign through the approved target process |
| `/lib/modules/.../build` is missing | Matching development/build files are unavailable | Install matching headers or use the custom kernel output |

Do not force-load a mismatched module. Bypassing compatibility checks can turn
a clear loader error into kernel memory corruption.

For verbose build diagnosis:

```sh
make clean
make V=1
```

Read the first compiler or `modpost` error rather than only the final
top-level `make` failure.

If Kbuild says it skipped BTF generation because `vmlinux` is unavailable,
the module may still be valid. BTF generation requires additional matching
kernel build data; decide whether that metadata is required by the target's
debugging, tracing, or packaging policy.

## Cleanup And Error Paths

Module initialization acquires no resources and always returns success. Module
exit only emits a log message, so there is no allocation or device teardown.
This intentionally keeps the example focused on build identity rather than
driver lifecycle.

If insertion fails, module initialization either did not run or the kernel
rejected the artifact before accepting it; there is no partially loaded module
to unload. Confirm with:

```sh
lsmod | grep '^build_flow_demo'
```

Remove generated build artifacts:

```sh
make clean
```

If the module is loaded, unload it before deleting or replacing the file:

```sh
sudo rmmod build_flow_demo
make clean
```

## Userspace ABI Impact

There is **no userspace ABI**. The example creates no device node, sysfs
attribute, ioctl, procfs/debugfs file, netlink family, or other application
interface. `insmod`, `rmmod`, `lsmod`, `modinfo`, and kernel-log access are
administrative test tools, not an ABI defined by this module.

## Why This Is Not Production-Ready

The module exists only to demonstrate the external build flow. A production
external-module process would additionally pin the exact source commit and
configuration, retain compiler and Kbuild versions, preserve matching
`Module.symvers`, record `kernelrelease` and artifact hashes, apply signing and
packaging policy, test on the final kernel image, and deploy modules as part of
a matched kernel artifact set.
