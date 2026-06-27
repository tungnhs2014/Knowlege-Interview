# Topic Brief - 11 - Device Tree APIs And Driver Integration

## Output Targets
- Knowledge: `knowledge/11-device-tree-apis-and-driver-integration.md`
- Interview: `interview/11-device-tree-apis-and-driver-integration.md`
- Example: `examples/11-device-tree-apis-and-driver-integration/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch05` | `docs/Linux Device Driver Development/Chapter 5-Platform Device Drivers.md` | read/mapped/covered/merged | Platform bus match order, `MODULE_DEVICE_TABLE()`, resource model, old platform data versus DT, and the rule that OF matching is attempted before ACPI, platform ID, and name matching. |
| `ldd1-ch06` | `docs/Linux Device Driver Development/Chapter 6-The Concept of a Device Tree .md` | read/mapped/covered/merged | Primary book source for driver-facing OF APIs: `pdev->dev.of_node`, property readers, child-node iteration, `struct of_device_id`, `.data` match data, `of_match_device()`, `of_match_ptr()`, `platform_get_resource()`, `platform_get_irq()`, and platform resources created from DT. |
| `ldd1-ch07` | `docs/Linux Device Driver Development/Chapter 7-I2C Client Drivers.md` | read/mapped/covered/merged | I2C-specific DT integration: I2C child nodes, `reg` as slave address, `struct of_device_id`, `.of_match_table`, `MODULE_DEVICE_TABLE(of, ...)`, `of_match_device()` in probe, and old kernel caveat around `.id_table`. |
| `ldd1-ch08` | `docs/Linux Device Driver Development/Chapter 8-SPI Device Drivers.md` | read/mapped/covered/merged | SPI-specific DT integration: SPI child nodes, `reg` as chip-select index, SPI mode properties, OF match table, `MODULE_DEVICE_TABLE(of, ...)`, and driver probe branching between DT and legacy data. |
| `ldd2-ch02` | `docs/Linux Device Driver Development 2/Chapter 2-Regmap_API.md` | read/mapped/deferred | Supporting source for interrupt-controller integration: IRQ domains hold firmware-node/DT translation state and `irq_domain_ops.xlate()` decodes interrupt specifiers. Detailed IRQ-domain teaching belongs to topics 15 and 19. |
| `ldd2-ch03` | `docs/Linux Device Driver Development 2/Chapter 3-MFD_Subsystem_and_Syscon_API.md` | read/mapped/deferred | Supporting source for MFD child devices using resource/name helpers such as `platform_get_irq_byname()` and `.of_compatible`. Detailed MFD/syscon material belongs to topic 19. |
| `ldd2-ch04` | `docs/Linux Device Driver Development 2/Chapter 4-Common_Clock_Framework.md` | read/mapped/covered/merged | Strong supporting source for provider/consumer phandle decoding: `#clock-cells`, `clocks`, `clock-names`, `struct of_phandle_args`, `of_parse_phandle_with_args()`, provider lookup, and why subsystem helpers hide raw OF parsing. |
| `ldd2-ch08` | `docs/Linux Device Driver Development 2/Chapter 8-Integrat with V4L2.md` | read/mapped/covered/deferred | Supporting source for modern `fwnode_handle` direction: fwnode abstracts OF and ACPI nodes; graph APIs and V4L2 endpoint parsing belong mainly to topic 33, but the API trend should be mentioned in topic 11. |
| `ldd2-ch10` | `docs/Linux Device Driver Development 2/Chapter 10-Linux_Kernel_Power_Management.md` | read/mapped/deferred | Supporting example of OF property use in production drivers, especially reading `wakeup-source` with `of_property_read_bool()`. Detailed PM semantics belong to topic 24. |
| `ldd2-ch12` | `docs/Linux Device Driver Development 2/Chapter 12-NVMEM_Framework.md` | read/mapped/deferred | Supporting example of child-node data cells and consumer phandle lists such as `nvmem-cells`/`nvmem-cell-names`. Detailed NVMEM belongs to topic 28. |
| `notion-ch05-part3` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 3 Device Provisioning & Integration.md` | read/mapped/covered/merged | Beginner-friendly bridge from board files to DT-backed platform drivers, automatic extraction of `reg` and `interrupts`, custom property parsing, mixed DT/legacy matching, and managed-resource use. |
| `notion-ch06-part2` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 2 Device Addressing and Resources.md` | read/mapped/covered/merged | Named resources across `reg-names`, `interrupt-names`, `clock-names`, `dma-names`, and modern descriptor GPIO examples using `devm_gpiod_get()`. |
| `notion-ch06-part3` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 3 OF APIs and Platform Integration.md` | read/mapped/covered/merged | Primary Notion source for `struct device_node`, `struct property`, OF property readers, sub-node APIs, phandle parsing, OF match tables, per-device match data, legacy fallback, best practices, and common pitfalls. |
| `notion-ch07-part3` | `docs/Linux-Device-Driver-Notion/Chapter 7-Part 3 Device Tree Integration.md` | read/mapped/covered/merged | I2C DT integration duplicate/expansion: richer examples, per-device match data, property parsing, `of_match_ptr()`, old `.id_table` compatibility warning, and driver/DT checklists. |
| `notion-ch08-part1` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 1.md` | read/mapped/covered/deferred | Supporting source for `fwnode_handle`, `dev_fwnode()`, `of_fwnode_handle()`, `to_of_node()`, and firmware graph parsing. V4L2 async details belong to topics 33 and 34. |
| `notion-ch08-part3` | `docs/Linux-Device-Driver-Notion/Chapter 8-Part 3 Device Tree Integration and Userspace Inter.md` | read/mapped/covered/merged | SPI DT integration duplicate/expansion: SPI addressing, common SPI properties, OF match table, `.data`, property parsing, and platform-data fallback. Userspace `spidev` material belongs to topics 17 and 08. |

## Source Files Read
- `ldd1-ch05`: full relevant platform-driver chapter read.
  - Relevant sections: `Resources and platform data`, `Device provisioning - the new and recommended way`, `Devices, drivers, and bus matching`, `Kernel devices and drivers-matching function`, `OF style and ACPI match`, `ID table matching`.
- `ldd1-ch06`: full file read.
  - Relevant sections: `Device tree mechanisms`, `Handling resources`, `Concept of named resources`, `Accessing registers`, `Handling interrupts`, `Extract application-specific data`, `Extracting and parsing sub-nodes`, `Platform drivers and DTs`, `OF match style`, `Dealing with non-device tree platforms`, `Support multiple hardware devices with per device-specific data`, `Match style mixing`, `Platform resources and DTs`, `Platform data versus DTs`.
- `ldd1-ch07`: relevant I2C DT section read.
  - Relevant sections: `I2C and device trees`, `Defining and registering the I2C driver`, `Remark`, `Instantiating I2C devices in a DT`, `Putting it all together`.
- `ldd1-ch08`: relevant SPI DT section read.
  - Relevant sections: `SPI and device tree`, `Instantiate SPI devices in device tree`, `Define and register SPI driver`, SPI DT match notes in `Putting it all together`.
- `ldd2-ch02`: relevant IRQ-domain/DT section read.
  - Relevant sections: `Regmap and IRQ management`, `Quick recap on Linux kernel IRQ management`, `struct irq_domain`, `irq_domain_add_*()`, `irq_domain_ops.xlate`.
- `ldd2-ch03`: relevant MFD resource section read.
  - Relevant section: child resource retrieval with `platform_get_irq_byname()` and note to use `platform_get_resource*()`/`platform_get_irq*()`.
- `ldd2-ch04`: relevant clock/DT sections read.
  - Relevant sections: `struct of_clk_provider`, `of_clk_add_hw_provider()`, `The clock provider device tree node and its associated mechanisms`, `Understanding the of_parse_phandle_with_args() API`.
- `ldd2-ch08`: relevant fwnode/graph sections read.
  - Relevant sections: `From the DT (of_graph_*) API to the generic fwnode graph API`, `struct fwnode_handle`, node/property fwnode helpers, OF/fwnode conversion helpers.
- `ldd2-ch10`: relevant wakeup-source excerpt read.
  - Relevant section: `Being a source of system wakeup`, especially `of_property_read_bool(np, "wakeup-source")`.
- `ldd2-ch12`: relevant NVMEM DT binding section read.
  - Relevant section: `Device tree bindings for NVMEM providers`.
- `notion-ch05-part3`: relevant DT integration portion read.
  - Relevant sections: `Device Tree Integration`, `Platform Driver with Device Tree Support`, `Device Tree Node Example`, `Extracting Device Tree Data`, `Mixed Matching`.
- `notion-ch06-part2`: relevant named resource portion read.
  - Relevant section: `Complete Named Resources Example`.
- `notion-ch06-part3`: full file read.
  - Relevant sections: `OF API Overview`, `Core Data Structures`, `Property Reading APIs`, `Sub-Node Handling and Iteration`, `Phandle Parsing APIs`, `GPIO OF APIs`, `Platform Driver Integration with Device Tree`, `Multi-Hardware Support with Per-Device Data`, `Backward Compatibility`, `Complete Real-World Driver Example`, `Best Practices and Common Pitfalls`.
- `notion-ch07-part3`: full file read.
  - Relevant sections: I2C DT hierarchy, OF-style matching, property parsing, complete I2C DT driver example, per-device data, backward compatibility, checklists.
- `notion-ch08-part1`: relevant fwnode section read.
  - Relevant sections: `Firmware Node Abstraction`, `Graph Binding Concepts`, `Parsing Device Tree`, graph APIs.
- `notion-ch08-part3`: relevant SPI DT/API portion read.
  - Relevant sections: `SPI Device Tree Fundamentals`, `SPI Device Declaration`, `OF-Style Driver Matching`, `Reading Device Tree Properties`, `Backward Compatibility with Platform Data`.

## Merged Source Notes
- Use `ldd1-ch06` and `notion-ch06-part3` as the primary structure for the learner-facing topic.
  - `ldd1-ch06` gives the clean book flow from DT node to driver probe and resource extraction.
  - `notion-ch06-part3` gives better API grouping, beginner examples, child-node reference handling, and pitfalls.
- Merge `ldd1-ch05` and `notion-ch05-part3` only as integration context:
  - DT replaces board-file provisioning for non-discoverable devices.
  - The bus match function can try OF matching before platform ID/name fallback.
  - Standard resources should be obtained through bus/platform helper APIs where possible.
- Merge I2C and SPI sources as concrete examples, not separate full lessons:
  - I2C and SPI both use `struct of_device_id`, `.of_match_table`, `MODULE_DEVICE_TABLE(of, ...)`, and optional `.data`.
  - I2C `reg` is a slave address; SPI `reg` is the chip-select index. That distinction is a common interview/debug trap.
  - Bus core instantiates `struct i2c_client`/`struct spi_device` from DT; drivers should not manually create those in normal DT systems.
- Merge `ldd2-ch04` as the best explanation of raw phandle-with-arguments parsing:
  - Provider nodes publish `#*-cells`.
  - Consumer properties contain phandle plus argument tuples.
  - Framework helpers such as `devm_clk_get()` usually hide raw `of_parse_phandle_with_args()`.
- Use `notion-ch06-part2` to reinforce named-resource guidance:
  - Prefer `platform_get_resource_byname()`, `platform_get_irq_byname()`, `devm_clk_get(dev, "name")`, DMA channel names, and GPIO descriptor names when multiple resources exist.
- Include `ldd2-ch08` and `notion-ch08-part1` as a modern API direction, not a V4L2 lesson:
  - New cross-firmware code increasingly uses `struct fwnode_handle`, `dev_fwnode()`, `fwnode_property_read_*()`, and graph helpers.
  - Topic 11 can introduce the idea and defer media graph details to topic 33.
- Framework-specific DT usage in PM, NVMEM, MFD, GPIO IRQ domains, clocks, regulators, pinctrl, V4L2, and audio should be referenced as evidence of the repeated provider/consumer pattern, then deferred to their learning-path chapters.
- No apparent duplicate was skipped:
  - ldd1 chapter 6 and Notion chapter 6 part 3 overlap heavily but were read separately.
  - ldd1 I2C/SPI chapters and Notion I2C/SPI DT parts were read separately and compared.
  - Same-number chapters across source groups were not treated as equivalent.

## Source Differences
- `ldd1-ch06` and Notion teach `of_get_property()` and direct `struct device_node` usage. For learner-facing modern code, prefer typed helpers such as `of_property_read_u32()`, `of_property_read_string()`, `of_property_read_bool()`, and count/array helpers.
- `notion-ch06-part3` teaches `of_get_named_gpio()` and integer GPIO APIs in a full example. Current kernel guidance strongly prefers GPIO descriptors (`gpiod_get()`, `devm_gpiod_get()`, `devm_gpiod_get_optional()`) from `<linux/gpio/consumer.h>`. Keep integer GPIO as legacy/stale-source awareness only.
- `ldd1-ch06` uses `devm_ioremap_resource()` after `platform_get_resource()`. Current infrastructure also provides platform wrappers such as `devm_platform_ioremap_resource()` and `devm_platform_get_and_ioremap_resource()`, which should be considered for modern examples.
- Some internal examples use weak or generic custom property names such as `mode`, `speed`, `enable-dma`, or unprefixed `sample-rate`. Final docs should teach that device-specific properties need binding design, vendor prefixes, and schema validation rather than arbitrary driver-private knobs.
- `notion-ch06-part3` says "always check for device tree node"; this is correct for DT-only drivers but too broad for drivers intentionally supporting ACPI/software-node/platform-data paths. Better rule: choose whether the driver is DT-only, firmware-node generic, or legacy-compatible, then handle missing `dev.of_node` explicitly.
- The book uses `of_match_ptr()` broadly. Modern in-tree style varies; for always-OF-enabled match tables, wrapping can cause unused table issues depending on table use. Treat `of_match_ptr()` as useful for non-OF builds, but validate against local kernel style before copying blindly.
- `ldd1-ch07` contains an old I2C `.id_table` caveat for kernels before/around 4.10. Keep it as historical/version-sensitive context, not as a universal current rule.
- `ldd2-ch08` states new drivers should use fwnode in V4L2 graph code. That is true for media graph APIs, but not every simple DT-only platform driver must be rewritten around fwnode. Present fwnode as the generic firmware-property direction when cross-OF/ACPI/software-node support matters.
- Older source paths mention `.txt` DT bindings. Modern binding work is YAML-schema based; final docs must point learners to current binding docs and `dtbs_check`.

## Gaps / Uncertainties
- Need verify exact current kernel preferred APIs while writing learner docs:
  - whether to show `of_match_device()` or `device_get_match_data()`/`of_device_get_match_data()` for match data in each bus;
  - preferred use of `devm_platform_ioremap_resource()` versus `platform_get_resource()` plus `devm_ioremap_resource()`;
  - current I2C/SPI probe prototypes and match-data helpers for the target kernel version;
  - how strongly to recommend generic `device_property_read_*()`/`fwnode_property_read_*()` over OF-only `of_property_read_*()` for new cross-firmware drivers.
- Need avoid building a binding-authoring chapter here. Binding YAML design and schema details should be covered enough to prevent bad custom properties, but full binding authoring may need a future advanced note.
- Need decide whether example output should use a platform driver, I2C client, or SPI client. A platform-driver example best matches the topic scope, with I2C/SPI snippets as side notes.
- Need keep subsystem consumers shallow:
  - GPIO descriptors are important here, but pinctrl/GPIO detail belongs to topics 13 and 14.
  - IRQ parsing detail belongs to topic 15.
  - Clock/regulator/power/NVMEM provider details belong to topics 22, 23, 24, and 28.
- Need be careful with DT overlay/live-tree APIs. The internal source does not teach dynamic OF changes for drivers; this topic should focus on static boot-time DT consumption unless externally extended.

## External Validation
- Used: https://docs.kernel.org/6.4/devicetree/kernel-api.html
  - Validates current DeviceTree kernel APIs including `of_parse_phandle()`, `of_parse_phandle_with_args()`, typed property readers, count helpers, `of_irq_get()`, and the requirement to `of_node_put()` returned node references.
- Used: https://docs.kernel.org/driver-api/infrastructure.html
  - Validates platform resource helpers including `platform_get_resource()`, `platform_get_resource_byname()`, `platform_get_irq()`, `platform_get_irq_byname()`, `devm_platform_ioremap_resource()`, and `devm_platform_get_and_ioremap_resource()`.
- Used: https://docs.kernel.org/driver-api/driver-model/platform.html
  - Validates platform device/driver binding behavior and platform-driver registration/probe model.
- Used: https://docs.kernel.org/driver-api/gpio/consumer.html
  - Validates that descriptor-based GPIO APIs are current and that legacy `gpio_*` interfaces are strongly discouraged for new code.
- Used: https://docs.kernel.org/driver-api/gpio/board.html
  - Validates Device Tree GPIO mapping convention where consumer node properties are named `<function>-gpios` and drivers request by function name through `gpiod_get()`.
- Used: https://docs.kernel.org/driver-api/device-io.html
  - Validates `devm_ioremap_resource()` as a higher-level MMIO mapping helper and notes resource-aware behavior.
- Used: https://docs.kernel.org/driver-api/media/v4l2-fwnode.html
  - Validates current fwnode graph/media parsing direction and reinforces that graph-specific endpoint parsing belongs to media topics.
- Still needed during final learner-doc writing:
  - Current kernel source examples for `of_device_get_match_data()`, `device_get_match_data()`, and bus-specific helpers such as `i2c_get_match_data()` or `spi_get_device_match_data()` if used.
  - Current binding-schema docs for property naming, vendor prefixes, and `dtbs_check` if binding-thinking content becomes more than a short warning.

## Learning Content Brief
- Mental model:
  - Device Tree integration is the driver's side of the hardware description contract.
  - DT describes hardware and wiring; the driver consumes that description during probe to bind to the right hardware variant, get resources, parse configuration, and register with the right subsystem.
  - The best driver does not hand-parse everything. It lets bus and subsystem helpers translate standard DT properties into resources, descriptors, clocks, regulators, DMA channels, IRQs, and framework objects.
- Core mechanism:
  - Boot firmware passes a DTB. The kernel unflattens it into `struct device_node` and `struct property` objects.
  - DT population creates devices from enabled nodes, often platform devices for `simple-bus` children and bus-native devices for I2C/SPI children.
  - The bus match function compares the node's `compatible` list against the driver's `.of_match_table`.
  - On match, the driver's `probe()` receives a bus-specific device object whose embedded `struct device` carries the firmware node, commonly `dev.of_node` for OF.
- Important structs:
  - `struct device`
    - Carries `of_node`/firmware-node association and is the anchor for managed resources and subsystem lookups.
  - `struct device_node`
    - In-memory DT node used by OF APIs. Returned references from lookup helpers must be released with `of_node_put()`.
  - `struct property`
    - In-memory property object; drivers should rarely touch raw fields directly.
  - `struct of_device_id`
    - Match table entry with `compatible` and optional `.data` for per-variant driver data.
  - `struct platform_device`, `struct i2c_client`, `struct spi_device`
    - Bus-specific device objects handed to `probe()`.
  - `struct resource`
    - Standard platform resource representation for MMIO, IRQ, DMA, and bus resources.
  - `struct of_phandle_args`
    - Result of parsing a phandle-with-arguments tuple.
  - `struct fwnode_handle`
    - Generic firmware-node abstraction for code that should work across OF, ACPI, and software nodes.
- Key APIs and when to use them:
  - Matching:
    - `static const struct of_device_id table[]`
    - `.driver.of_match_table`
    - `MODULE_DEVICE_TABLE(of, table)`
    - `of_match_device()` or current match-data helpers when variant data is needed.
  - Standard resources:
    - `platform_get_resource()`
    - `platform_get_resource_byname()`
    - `platform_get_irq()`
    - `platform_get_irq_byname()`
    - `devm_ioremap_resource()`
    - `devm_platform_ioremap_resource()`
    - `devm_platform_get_and_ioremap_resource()`
  - Typed OF property readers:
    - `of_property_read_u32()`
    - `of_property_read_u32_array()`
    - `of_property_read_u32_index()`
    - `of_property_count_u32_elems()`
    - `of_property_read_string()`
    - `of_property_read_string_array()`
    - `of_property_read_bool()`
    - `of_property_read_u64()`
    - `of_property_read_u8_array()`
  - Child nodes:
    - `for_each_child_of_node()`
    - `of_get_next_child()`
    - `of_get_child_by_name()`
    - `of_get_child_count()`
    - `of_node_put()`
  - Phandles:
    - `of_parse_phandle()`
    - `of_parse_phandle_with_args()`
    - `of_count_phandle_with_args()`
    - Prefer subsystem helpers where available.
  - Subsystem consumers:
    - `devm_clk_get()`, `devm_regulator_get()`, `devm_gpiod_get()`, DMA channel helpers, pinctrl helpers, NVMEM helpers.
    - Use names from `*-names` properties or `<function>-gpios` conventions.
  - Generic firmware-property direction:
    - `dev_fwnode()`
    - `fwnode_property_read_*()`
    - `device_property_read_*()`
    - `fwnode_handle_put()`
    - Use when writing firmware-agnostic driver code.
- Lifecycle/data flow:
  - DTS binding is written and validated.
  - Board DTS enables the device node and supplies required resources/properties.
  - Kernel populates a device from the DT node.
  - Bus core matches the node `compatible` against the driver table.
  - Module autoload can use aliases generated from `MODULE_DEVICE_TABLE(of, ...)`.
  - `probe()` allocates private data, gets standard resources through helper APIs, maps registers, requests IRQs, gets clocks/GPIOs/regulators/DMA channels, parses only device-specific properties, initializes hardware, and registers with a framework.
  - On error, devm-managed resources unwind automatically; manually enabled resources such as clocks may still need explicit disable or a devm action.
  - `remove()` or devres cleanup releases resources, disables runtime state, and unregisters framework objects.
- Resource integration rules:
  - For MMIO, prefer platform resource helpers and managed ioremap wrappers rather than manually parsing `reg`.
  - For interrupts, prefer `platform_get_irq()`/`platform_get_irq_byname()` or bus/subsystem helpers rather than manually decoding `interrupts`.
  - For clocks, regulators, GPIOs, DMA, resets, pinctrl, and NVMEM, prefer the subsystem consumer API rather than raw phandle parsing.
  - Use named resources when order-dependent lists would be fragile.
- Match-data guidance:
  - Use `compatible` strings to identify hardware variants.
  - Use `.data`/match-data helpers for hardware differences known to the driver, such as register layout, FIFO size, quirks, feature flags, or operation tables.
  - Do not encode driver policy in arbitrary DT properties when a variant-specific compatible or binding-defined property is the right contract.
- Binding thinking:
  - DT properties describe hardware facts, board wiring, and hardware capabilities.
  - Device-specific properties should be documented, vendor-prefixed where needed, and validated by bindings.
  - Prefer existing common properties and subsystem bindings before inventing a property.
  - Treat bindings as long-lived contracts; changing them casually can break old kernels or shared boards.
- Common bugs:
  - Missing sentinel in `of_device_id` table.
  - Forgetting `MODULE_DEVICE_TABLE(of, ...)`, causing module autoload failures.
  - Matching with a `compatible` string that differs by vendor prefix, spelling, or fallback order.
  - Assuming `dev.of_node` exists in a driver that also supports ACPI/software-node/platform-data systems.
  - Reading optional properties without defaults or mandatory properties without clear errors.
  - Using untyped `of_get_property()` when a typed helper would catch size/encoding errors.
  - Leaking node references returned by `of_parse_phandle()`, `of_parse_phandle_with_args()`, or child lookup APIs.
  - Manually parsing standard resources instead of using platform/subsystem helpers.
  - Using integer GPIO APIs in new code instead of GPIO descriptors.
  - Confusing I2C `reg` address with SPI `reg` chip-select index.
  - Forgetting that boolean properties are true by presence, false by absence.
  - Parsing multi-resource lists by index after a binding provides `*-names`.
  - Treating DT as a dumping ground for driver knobs rather than a hardware description.
- Debugging notes:
  - Check whether the node exists and is enabled: runtime DT view, `status`, compatible string, and node path.
  - Check module aliases with `modinfo` and generated `modules.alias` if autoload fails.
  - Inspect `/sys/bus/platform/devices/`, I2C/SPI bus devices, and driver bind/unbind paths to confirm population and binding.
  - Use `dev_info()`/`dev_err_probe()` around probe resource acquisition; `-EPROBE_DEFER` often means a referenced provider is not ready.
  - Use `dtbs_check`/binding validation to catch property and schema mistakes before runtime.
  - Check `/proc/interrupts`, clock debugfs, GPIO debugfs/gpiod tools, regulator summary, and dynamic debug depending on which provider lookup fails.
- Production concerns:
  - Keep probe ordering resilient; phandle consumers may defer until providers bind.
  - Use devm helpers, but still manage stateful enables/disables carefully.
  - Make mandatory versus optional properties explicit.
  - Prefer `dev_err_probe()` for provider-related probe failures and deferrals.
  - Keep driver code board-agnostic; board differences should be in DT plus match data.
  - Do not duplicate binding knowledge in code comments when the YAML binding should be the source of truth.
- Interview angles:
  - Explain how a DT node becomes a probed driver instance.
  - Explain why `compatible` and `of_device_id` matter.
  - Explain why `MODULE_DEVICE_TABLE(of, ...)` matters for modules.
  - Explain when to use `platform_get_resource()` versus `of_property_read_u32()`.
  - Explain how `platform_get_irq()` relates to the DT `interrupts` property.
  - Explain named resources and why `*-names` prevents order bugs.
  - Explain `of_parse_phandle_with_args()` and the provider `#*-cells` pattern.
  - Explain why subsystem helpers are preferred over raw phandle parsing.
  - Explain `.data` in `struct of_device_id` and when it is better than a custom DT property.
  - Explain the difference between OF-only APIs and fwnode/device-property APIs.
  - Spot stale code that uses integer GPIO APIs or raw `of_get_property()`.
  - Given a failed probe, reason through missing node, bad compatible, disabled status, missing provider, wrong resource name, and `-EPROBE_DEFER`.
