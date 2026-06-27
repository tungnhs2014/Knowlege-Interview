# 10 - Device Tree Fundamentals Example

This is a **learning-only** Device Tree example. It does not describe real hardware, and it is not meant to be booted on a board. Its purpose is to practice reading, compiling, decompiling, and debugging a minimal DTS file.

No kernel module is included because this topic is about Device Tree fundamentals. Driver-side OF APIs and resource extraction are covered in the next learning-path topic.

## Goal

Use this example to understand the first layer of Device Tree bring-up:

```text
example-board.dts
  -> compile with dtc
  -> example-board.dtb
  -> decompile back to DTS
  -> inspect nodes, properties, labels, phandles, reg, interrupts, and status
```

The DTS demonstrates:

- root `model` and `compatible`;
- `.dts` syntax, nodes, and properties;
- `aliases` and `chosen`;
- `memory@...` with `reg`;
- a `simple-bus` with child addressing;
- parent-owned `#address-cells` and `#size-cells`;
- `compatible` strings with specific-to-fallback ordering;
- `reg` for MMIO-style devices;
- `interrupt-parent`, `interrupt-controller`, and `interrupts`;
- a GPIO provider and `reset-gpios` phandle reference;
- `status = "okay"` versus `status = "disabled"`.

## Kernel Version Assumptions

The DTS syntax used here is standard Device Tree source syntax and is not tied to a specific kernel release.

Tooling assumptions:

- `dtc` is installed on the host.
- The example can be compiled as a standalone DTS with:

```sh
dtc -I dts -O dtb -o example-board.dtb example-board.dts
```

Production kernel trees should also use binding validation:

```sh
make dt_binding_check
make dtbs_check
```

This standalone example intentionally does not ship YAML bindings, so `dtbs_check` is listed as the production habit rather than as a command that validates this fake board.

## Files

| File | Purpose |
| --- | --- |
| `example-board.dts` | Learning-only DTS that demonstrates nodes, properties, `compatible`, `reg`, interrupts, phandles, and `status`. |
| `README.md` | Build, inspect, test, debug, cleanup, and production notes. |

No `Makefile` is needed because no kernel module code is included.

## Build

From this directory:

```sh
dtc -I dts -O dtb -o example-board.dtb example-board.dts
```

Expected generated file:

```text
example-board.dtb
```

You may see no output on success. That is normal for `dtc`.

## Inspect And Test

Decompile the generated blob:

```sh
dtc -I dtb -O dts -o example-board.decompiled.dts example-board.dtb
```

Inspect the result:

```sh
sed -n '1,180p' example-board.decompiled.dts
```

Useful checks:

```sh
grep -n 'compatible' example-board.decompiled.dts
grep -n 'phandle' example-board.decompiled.dts
grep -n 'reset-gpios' example-board.decompiled.dts
grep -n 'status' example-board.decompiled.dts
```

Expected observations:

- `demo-device@10010000` has `status = "okay"`.
- `demo-device@10020000` has `status = "disabled"`.
- `reset-gpios = <...>` no longer contains `&gpio0` in the decompiled file; it is resolved to a numeric phandle.
- The provider node `gpio@10002000` has a generated `phandle` property.
- `serial@10000000` keeps its two `compatible` strings.

Example output shape:

```text
compatible = "lld,fake-uart-v2", "lld,fake-uart-v1";
reset-gpios = <0xNN 0x07 0x01>;
phandle = <0xNN>;
status = "okay";
status = "disabled";
```

The exact phandle number may differ. The important point is that a source-level label reference such as `<&gpio0 7 1>` becomes a numeric reference in the compiled tree.

## Optional Runtime Comparison

On a real DT-based target, compare this learning file with the live tree:

```sh
cat /sys/firmware/devicetree/base/model
find /sys/firmware/devicetree/base -maxdepth 3 -name compatible | head
find /sys/firmware/devicetree/base -maxdepth 4 -name status | head
```

If enabled on that kernel, this may also exist:

```sh
ls /proc/device-tree
```

Do not copy this fake DTS to a real board boot flow. It has fake compatible strings and fake addresses.

## Load / Unload

There is nothing to load or unload in this example.

The example creates a `.dtb` file only. It does not:

- register a kernel module;
- create a platform device on the running host;
- touch real MMIO;
- request IRQs;
- create `/dev` nodes;
- create sysfs, procfs, or debugfs files.

Expected kernel logs: none. If you see kernel logs while running this example, they came from unrelated system activity, because these commands only compile and inspect local files.

## Userspace ABI Impact

There is **no userspace ABI impact**.

This example creates no userspace-visible driver interface. The only files produced are local build artifacts:

- `example-board.dtb`;
- `example-board.decompiled.dts`, if you run the decompile command.

If a real board booted a DTB, it could affect userspace indirectly by causing devices and their normal kernel interfaces to appear. This learning DTS is not used that way.

## Cleanup

Remove generated files:

```sh
rm -f example-board.dtb example-board.decompiled.dts
```

No kernel cleanup is needed because no module was loaded and no live devices were created.

## Error Paths And Debugging

### Syntax Error

If you break DTS syntax, `dtc` reports the file and line:

```text
Error: example-board.dts:LINE.COLUMN-COLUMN syntax error
FATAL ERROR: Unable to parse input tree
```

Fix pattern:

- check missing semicolons after properties and nodes;
- check balanced braces;
- check that cell lists use `<...>`;
- check that byte arrays use `[...]`.

### Missing Label

If you rename `gpio0` but keep `reset-gpios = <&gpio0 7 1>;`, `dtc` reports an unresolved reference.

Expected error shape:

```text
ERROR (phandle_references): /soc/demo-device@10010000: Reference to non-existent node or label "gpio0"
```

Fix pattern:

- restore the label;
- or update the phandle reference to the new label.

### Addressing Warnings

If a child `reg` does not match the parent bus cell layout, `dtc` may warn about invalid `reg` length.

Fix pattern:

- inspect the parent node;
- verify its `#address-cells` and `#size-cells`;
- recalculate the child `reg` tuples.

### Semantic Problems

`dtc` does not know whether `"lld,demo-device"` is a real device or whether `interrupts = <11 4>` is correct for real hardware.

Production fix pattern:

- write or use a real YAML binding;
- validate with `dt_binding_check` and `dtbs_check`;
- compare against the hardware manual and board schematic.

## Why This Is Not Production-Ready

This example is learning-only because:

- compatible strings are fake;
- MMIO addresses are fake;
- interrupt numbers and flags are fake;
- there are no binding schemas for these nodes;
- it is not integrated into a kernel tree or board boot flow;
- it does not prove that any real driver can probe.

Production DTS work would add:

- real hardware-compatible strings;
- binding documentation;
- `dtbs_check` coverage;
- correct SoC include files;
- board-specific pinctrl, clocks, regulators, resets, and supplies;
- confirmation that the bootloader loads the rebuilt DTB;
- runtime checks on the target board.

## What To Try Next

Small edits that teach useful failure modes:

1. Change `reset-gpios = <&gpio0 7 1>;` to use `&missing_gpio`, then run `dtc`.
2. Change the parent `soc` node to `#size-cells = <0>`, then observe `reg` warnings.
3. Remove `status = "okay"` from `demo0` and decompile again.
4. Change the order of UART compatible strings and explain why specific-to-generic order is preferred.
5. Add `reg-names = "regs";` to `demo0` and explain when named resources become useful.
