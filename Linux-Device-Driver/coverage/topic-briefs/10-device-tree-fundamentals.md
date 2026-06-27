# Topic Brief - 10 - Device Tree Fundamentals

## Output Targets
- Knowledge: `knowledge/10-device-tree-fundamentals.md`
- Interview: `interview/10-device-tree-fundamentals.md`
- Example: `examples/10-device-tree-fundamentals/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch05` | `docs/Linux Device Driver Development/Chapter 5-Platform Device Drivers.md` | read/mapped/covered/merged | Supporting source for why Device Tree replaced board-file provisioning, how DT keeps hardware description separate from kernel C code, and why `compatible` participates in device-driver matching. |
| `ldd1-ch06` | `docs/Linux Device Driver Development/Chapter 6-The Concept of a Device Tree .md` | read/mapped/covered/merged | Primary book source for DT mental model, syntax/data types, naming, labels/phandles/aliases, `.dts`/`.dtsi`/`.dtb`, `dtc`, addressing with `reg` plus `#address-cells`/`#size-cells`, simple-bus, I2C/SPI addressing, resources, interrupts, OF matching, and property extraction. |
| `ldd2-ch04` | `docs/Linux Device Driver Development 2/Chapter 4-Common_Clock_Framework.md` | read/mapped/covered/merged | Supporting source showing phandle-with-arguments as a recurring DT mechanism: clock providers expose `#clock-cells`, consumers use `clocks`/`clock-names`, and frameworks decode specifiers through OF helpers. |
| `ldd2-ch10` | `docs/Linux Device Driver Development 2/Chapter 10-Linux_Kernel_Power_Management.md` | read/mapped/deferred | Supporting evidence that DT is used across subsystems for idle states, OPPs, thermal zones, and generic power domains; detailed PM bindings belong to topic 24. |
| `ldd2-ch12` | `docs/Linux Device Driver Development 2/Chapter 12-NVMEM_Framework.md` | read/mapped/deferred | Supporting evidence for child-node resources, `reg` as offset/size in a provider, and consumer phandle lists such as `nvmem-cells`; detailed NVMEM binding belongs to topic 28. |
| `ldd2-ch13` | `docs/Linux Device Driver Development 2/Chapter 13-Watchdog_Device_Drivers.md` | read/mapped/deferred | Supporting evidence that some Linux framework devices are DT-only and depend on specific `compatible` plus GPIO specifiers; detailed watchdog binding belongs to topic 28. |
| `notion-ch01-part2` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 2 Kernel Configuration.md` | read/mapped/covered/merged | Supporting source for enabling Device Tree/Open Firmware support and overlay support in kernel configuration. |
| `notion-ch05-part3` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 3 Device Provisioning & Integration.md` | read/mapped/covered/merged | Supporting source for the beginner motivation: legacy board files versus Device Tree, one kernel with many DTBs, basic DT-backed platform-driver matching, and resource extraction from `reg`/`interrupts`. |
| `notion-ch06-part1` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 1 Device Tree Fundamentals.md` | read/mapped/covered/merged | Primary Notion source for purpose, design principles, file types, root/tree structure, syntax, data types, naming conventions, labels/phandles/aliases, compilation/decompilation, runtime inspection, schema validation, and complete DTS examples. |
| `notion-ch06-part2` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 2 Device Addressing and Resources.md` | read/mapped/covered/merged | Primary Notion source for parent-defined address/size cells, `reg` meaning by bus, MMIO/I2C/SPI/simple-bus addressing, interrupt-controller/consumer properties, phandle-based clocks/GPIO/DMA/pinctrl, named resources, and `status`. |
| `notion-ch06-part3` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 3 OF APIs and Platform Integration.md` | read/mapped/covered/deferred | Related source for OF data structures and property-reading APIs. Only the conceptual overlap is merged into topic 10; full driver integration is deferred to learning-path 11. |

## Source Files Read
- `ldd1-ch05`: relevant Device Tree provisioning and matching sections read.
  - Relevant sections: `Device provisioning - the new and recommended way`, `Devices, drivers, and bus matching`, `OF style and ACPI match`, summary notes that DT is the new mechanism for device population/configuration.
- `ldd1-ch06`: full file read.
  - Relevant sections: `Device tree mechanisms`, `Naming convention`, `Aliases, labels, and phandle`, `DT compiler`, `Representing and addressing devices`, `SPI and I2C addressing`, `Platform device addressing`, `Handling resources`, `Concept of named resources`, `Accessing registers`, `Handling interrupts`, `Extract application-specific data`, `Platform drivers and DTs`, `OF match style`, `Platform resources and DTs`, `Platform data versus DTs`.
- `ldd2-ch04`: relevant Device Tree clock-provider/consumer sections read.
  - Relevant sections: `The clock provider device tree node and its associated mechanisms`, `Understanding the of_parse_phandle_with_args() API`, clock provider/consumer phandle specifiers, `#clock-cells`, `clocks`, `clock-names`, and provider registration notes.
- `ldd2-ch10`: relevant DT binding mentions read.
  - Relevant sections: CPU idle DT descriptions, OPP DT descriptions, thermal DT descriptions, generic power domain DT relationship descriptions.
- `ldd2-ch12`: relevant NVMEM DT binding section read.
  - Relevant section: `Device tree bindings for NVMEM providers`.
- `ldd2-ch13`: relevant GPIO watchdog DT binding section read.
  - Relevant section: `GPIO-based watchdogs`.
- `notion-ch01-part2`: relevant kernel configuration section read.
  - Relevant section: `Device Tree Support`, including Open Firmware support and overlay support.
- `notion-ch05-part3`: full file read.
  - Relevant sections: `Device Provisioning Methods`, `Legacy Board Files`, `Device Tree Integration`, DT node example, extracting DT data, mixed matching.
- `notion-ch06-part1`: full file read.
  - Relevant sections: `Introduction to Device Tree`, `Device Tree File Types and Structure`, `Device Tree Syntax and Data Types`, `Naming Conventions`, `Aliases, Labels, and Phandles`, `Device Tree Compilation`, `Complete Examples`, summary.
- `notion-ch06-part2`: full file read.
  - Relevant sections: `Understanding Address Cells`, `The reg Property`, `I2C Devices Addressing`, `SPI Devices Addressing`, `Platform Device Addressing (simple-bus)`, `Interrupt Handling in Device Tree`, `Clock References`, `GPIO References`, `DMA References`, `Pin Control`, `Named Resources`, `Status Property`.
- `notion-ch06-part3`: full file read.
  - Relevant sections: `OF API Overview`, `Core Data Structures`, `Property Reading APIs`, `Sub-Node Handling and Iteration`, `Phandle Parsing APIs`, `Platform Driver Integration with Device Tree`, `Best Practices and Common Pitfalls`.

## Merged Source Notes
- Use `ldd1-ch06` as the primary book structure, then merge the clearer Notion explanations where they improve beginner understanding.
- `notion-ch06-part1` adds the best mental model: DT describes what hardware exists and how it is wired, not how a driver should implement policy. It also gives a clean `.dts`/`.dtsi`/`.dtb` explanation and better validation/debugging notes.
- `notion-ch06-part2` splits addressing and resources more clearly than `ldd1-ch06`. Preserve its parent-defined rule: a node's `#address-cells` and `#size-cells` define how to interpret its children, not itself.
- `ldd1-ch06` provides the compact production-facing explanation of `reg`, named resources, `interrupt-parent`, `interrupts`, `interrupt-controller`, `#interrupt-cells`, and `platform_get_resource()`/`platform_get_irq()` as bridges from DT to driver resources.
- `ldd1-ch05` and `notion-ch05-part3` should be merged only for motivation and platform population context. Detailed platform-driver teaching belongs to topic 09.
- `ldd2` has no standalone Device Tree fundamentals chapter. Its relevant content is framework-specific evidence that the same DT grammar repeats across subsystems: provider nodes advertise `#*-cells`, consumer nodes reference providers with phandles plus arguments, and bindings define each property's meaning.
- `notion-ch06-part3` overlaps strongly with learning-path 11. For topic 10, keep only enough to explain that the kernel stores DT as `struct device_node`/`struct property`, exposes it to drivers through `pdev->dev.of_node`, and lets drivers read properties. Defer API depth to topic 11.
- No apparent duplicates were skipped: the book chapter, Notion chapter 6 parts, platform-provisioning notes, and embedded `ldd2` excerpts were inspected separately and compared before merging.

## Source Differences
- `ldd1-ch06` calls DT formatting "JSON-like"; Notion repeats this. Use it only as a beginner analogy. Device Tree source is its own DTS syntax, not JSON.
- `ldd1-ch06` references older `.txt` binding paths such as `Documentation/devicetree/bindings/arm/gic.txt` and `nvmem.txt` appears in `ldd2-ch12`. Current upstream bindings are commonly YAML schema files and should be validated with schema tooling.
- `ldd1-ch06` highlights `/proc/device-tree` with `CONFIG_PROC_DEVICETREE`. Notion also mentions it. Modern docs/examples should also mention `/sys/firmware/devicetree/base` as a common runtime view.
- `ldd1-ch06` says aliases avoid walking the whole tree and mentions `find_node_by_alias()`. The final lesson should use current API naming carefully and avoid overpromising O(1) behavior unless externally confirmed.
- `ldd1-ch06` and Notion both show driver-level OF property APIs in the fundamentals chapter. The learning path separates fundamentals from API integration, so topic 10 should explain what properties mean and defer most extraction APIs to topic 11.
- Notion examples include training-style simplifications and some broad rules such as node/property naming. External kernel DTS coding style is stricter and should be the authority for final style guidance.
- Notion includes `dtbs_check` and `dt_binding_check`, which is more current than the book's older binding references. Preserve the modern validation path.
- None of the internal source material gives a deep overlay lesson. Only basic overlay awareness belongs here; implementation details and notifier/lifetime issues should be recorded as external-validation material or a future advanced note.

## Gaps / Uncertainties
- Need current-kernel validation before final learner docs for:
  - exact DTS coding-style rules, property ordering, lowercase/underscore/dash guidance, and unit-address formatting;
  - modern binding workflow, especially YAML schemas, `dt_binding_check`, `dtbs_check`, and `DT_SCHEMA_FILES`;
  - runtime DT inspection paths and whether `/proc/device-tree` should be presented as optional/legacy compared with `/sys/firmware/devicetree/base`;
  - overlay basics, especially `.dtso`, `/plugin/`, label resolution, base DT `-@`, and live-tree lifetime hazards.
- Need decide whether topic 10 should include a small DTS snippet only, or whether the example output should compile a minimal `.dts` to `.dtb`. The learning path has an `examples/10-*` target, but this request intentionally does not create it.
- Need keep subsystem-specific bindings shallow. Clocks, GPIO, DMA, pinctrl, regulators, power domains, and NVMEM each need their own later lessons; topic 10 should teach the repeated DT pattern without turning into a framework catalog.
- Need avoid claiming that editing a DTB alone always avoids kernel rebuilds. In real products, bootloader packaging, FIT images, signed boot chains, and distro deployment can still require rebuild/reflash steps.
- Need include a warning that DT is an ABI-like contract for hardware description. Changing existing bindings/properties casually can break old kernels, bootloaders, or users of the same DT.

## External Validation
- Used: https://kernel.org/doc/html/next/devicetree/usage-model.html
  - Validates Device Tree as an OS-readable hardware description, tree of named nodes/properties, links between nodes, bindings as usage conventions, use of existing bindings, FDT/DTB history, and Linux use for platform identification, runtime configuration, and device population.
- Used: https://docs.kernel.org/devicetree/bindings/dts-coding-style.html
  - Validates current DTS naming/coding-style guidance: lowercase node/property names, dash for node/property names, underscore for labels, no leading zeros in unit addresses, preferred property ordering, and organization of SoC DTSI versus board DTS.
- Used: https://www.kernel.org/doc/html/latest/devicetree/bindings/writing-schema.html
  - Validates that DT bindings are written using json-schema vocabulary in YAML, and that `dt_binding_check`, `dtbs_check`, and `DT_SCHEMA_FILES` are current validation workflows.
- Used: https://kernel.org/doc/html/v6.0/devicetree/bindings/writing-bindings.html
  - Validates binding-design rules: describe hardware rather than Linux driver choices, use specific `compatible` strings, vendor-prefix device-specific properties, do not redefine common properties, and name multiple phandle entries with matching `*-names` when needed.
- Used: https://docs.kernel.org/6.15/devicetree/bindings/submitting-patches.html
  - Validates that compatible strings used in DTS should be documented in DT bindings and that DTS is treated as driver-independent hardware description.
- Used: https://docs.kernel.org/devicetree/overlay-notes.html
  - Validates overlay basics: overlays modify the live tree, active new nodes can create devices, label-target overlays require the base DT to be compiled with `-@`, target-path syntax is an alternative, and overlay node/property pointers must not outlive overlay removal.

## Learning Content Brief
- Mental model:
  - Device Tree is a declarative hardware description passed to the kernel, commonly as a compiled DTB.
  - It lets one kernel image support multiple boards by moving board-specific hardware topology and wiring into data.
  - DT says what exists and how it is connected: buses, devices, memory ranges, interrupts, clocks, GPIOs, DMA channels, resets, regulators, and board-specific enablement.
  - DT should not encode driver policy or arbitrary Linux implementation choices.
- Core mechanism:
  - Source files are `.dts` for board-level descriptions and `.dtsi` for reusable SoC/SoM includes. The compiler produces `.dtb`.
  - The kernel receives a flattened device tree, unflattens it into in-memory nodes/properties, then uses it for platform identification, runtime configuration, and device population.
  - Each node has a name, optional unit address, properties, children, and sometimes a label for source-level references.
  - `compatible` strings connect hardware description to drivers and bindings. More-specific strings should appear before fallback-compatible strings.
  - Bindings define the valid properties and their meaning for each hardware class/device.
- DTS syntax and data:
  - Strings: `"vendor,device"` and string lists such as `clock-names = "ipg", "per";`.
  - Cells: 32-bit values inside `<...>`, often addresses, sizes, IDs, flags, or specifier arguments.
  - Byte arrays: `[00 11 22 33 44 55]`, often for MAC addresses or raw data.
  - Boolean properties: property present means true, absent means false.
  - Labels use `label: node@addr { ... };`; references use `<&label ...>`.
- Addressing and resources:
  - `reg` is the main addressing/resource property, but its meaning depends on the parent bus.
  - `#address-cells` and `#size-cells` belong to a parent and describe how to parse child `reg` values.
  - MMIO platform devices usually use `reg = <base size>`.
  - I2C children usually use `reg = <slave_address>` under a bus with `#size-cells = <0>`.
  - SPI children usually use `reg = <chip_select_index>` under a bus with `#size-cells = <0>`.
  - Named resources such as `reg-names`, `interrupt-names`, `clock-names`, and `dma-names` reduce order-dependent driver bugs.
- Phandles and provider/consumer pattern:
  - A phandle is a compiled reference to another node.
  - Provider nodes expose a `#*-cells` property that defines how many argument cells follow their phandle.
  - Consumer nodes use properties such as `clocks`, `gpios`, `dmas`, `resets`, `interrupt-parent`, or `*-supply` to reference providers.
  - This pattern repeats across clock, GPIO, DMA, pinctrl, regulator, NVMEM, and power-domain bindings.
- Interrupt basics:
  - Interrupt controllers use `interrupt-controller` and `#interrupt-cells`.
  - Interrupt consumers use `interrupt-parent` and `interrupts`, or `interrupts-extended` for multiple controllers.
  - The meaning of interrupt cells is controller-specific, for example ARM GIC commonly uses type, number, and trigger flags.
- Compilation, validation, and runtime inspection:
  - Build DTBs through kernel build targets such as `make dtbs` or board-specific DTB targets.
  - `dtc` can compile/decompile, but syntax success is not semantic validation.
  - Modern validation uses YAML bindings with `make dt_binding_check` and `make dtbs_check`.
  - Inspect runtime DT through `/sys/firmware/devicetree/base` and, when enabled, `/proc/device-tree`.
- Overlay basics:
  - An overlay modifies the live DT and can cause new active device nodes to be populated or removed.
  - Label-target overlays are preferred when the base DT was compiled with `-@`; target-path syntax is an alternative.
  - Overlay lifetime is subtle: do not keep pointers to overlay nodes/properties after removal.
- Common bugs:
  - Treating `#address-cells`/`#size-cells` as describing the current node rather than its children.
  - Mismatching node unit address and `reg`.
  - Using `0x` or leading zeros in unit addresses contrary to style.
  - Inventing custom properties instead of checking existing bindings.
  - Using non-specific `compatible` strings or omitting fallback compatibles.
  - Reordering multi-resource lists without updating `*-names`.
  - Forgetting that I2C `reg` is a bus address while SPI `reg` is a chip-select index.
  - Relying on `dtc` alone and skipping schema validation.
  - Encoding Linux driver behavior instead of hardware facts.
- Debugging notes:
  - Check the loaded DT model, compatible strings, and node paths at runtime.
  - Decompile the DTB when unsure what the bootloader actually passed.
  - Use `dtbs_check` to catch binding/semantic mistakes that `dtc` cannot catch.
  - Verify `status = "okay"` for board-enabled devices and `status = "disabled"` in reusable SoC `.dtsi` defaults.
  - Match driver logs and `/sys/bus/platform/devices/` against DT node names and compatible strings.
- Production concerns:
  - Keep SoC-common hardware in `.dtsi`; put board-present components and wiring in board `.dts`.
  - Use documented bindings and current DTS style.
  - Treat DT bindings as long-lived contracts.
  - Prefer common properties and common bindings over vendor-specific reinvention.
  - Add binding documentation before relying on a new compatible/property.
  - Keep driver code board-agnostic; use DT data and match data for board/hardware variants.
- Interview angles:
  - Explain why Linux uses Device Tree for non-discoverable embedded hardware.
  - Explain `.dts`, `.dtsi`, `.dtb`, and `dtc`.
  - Explain nodes, properties, `compatible`, labels, phandles, aliases, and bindings.
  - Given a `reg` property, explain how to parse it from the parent bus cells.
  - Compare MMIO, I2C, and SPI meanings of `reg`.
  - Explain interrupt controller versus interrupt consumer properties.
  - Explain the provider/consumer phandle pattern using clocks or GPIOs.
  - Explain why `dtc` is not enough and what `dtbs_check` adds.
  - Explain what belongs in DT versus driver code.
  - Explain why changing a binding carelessly is risky.
