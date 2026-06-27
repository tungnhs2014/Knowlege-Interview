# Topic Brief - 09 - Platform Bus And Platform Drivers

## Output Targets
- Knowledge: `knowledge/09-platform-bus-and-platform-drivers.md`
- Interview: `interview/09-platform-bus-and-platform-drivers.md`
- Example: `examples/09-platform-bus-and-platform-drivers/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-ch05` | `docs/Linux Device Driver Development/Chapter 5-Platform Device Drivers.md` | read/mapped/covered/merged | Primary source for platform bus mental model, `struct platform_driver`, `struct platform_device`, resource arrays, platform data, driver/device matching, `MODULE_DEVICE_TABLE()`, ID tables, and name fallback matching. |
| `ldd1-ch06` | `docs/Linux Device Driver Development/Chapter 6-The Concept of a Device Tree .md` | read/mapped/covered/merged | Supporting source for Device Tree provisioning of platform devices, OF matching, `of_match_table`, `pdev->dev.of_node`, `platform_get_resource()` from `reg`, `platform_get_irq()` from `interrupts`, named resources, and DT-vs-platform-data migration. |
| `ldd1-ch11` | `docs/Linux Device Driver Development/Chapter 11-Kernel Memory Management.md` | read/mapped/covered/merged | Supporting source for device-managed resources (`devres`), `devm_*` lifetime, automatic release on detach/unload, and simplified probe error handling. |
| `ldd2-ch01` | `docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md` | read/mapped/covered/merged | Supporting source showing platform-driver `probe()` in interrupt/workqueue examples: `platform_get_resource()`, `platform_get_irq()`, work initialization, IRQ registration, and locking context. |
| `ldd2-ch02` | `docs/Linux Device Driver Development 2/Chapter 2-Regmap_API.md` | read/mapped/covered/merged | Supporting source showing modern MMIO platform probe patterns with `devm_ioremap_resource()`, `devm_regmap_init_mmio()`, `platform_get_irq()`, and `devm_*` allocation in real subsystem drivers. |
| `ldd2-ch03` | `docs/Linux Device Driver Development 2/Chapter 3-MFD_Subsystem_and_Syscon_API.md` | read/mapped/covered/merged | Supporting source showing platform devices created by MFD cells, child resources, `platform_get_irq_byname()`, `mfd_add_devices()` internals, child platform drivers, and why compatible-string matching is clearer than name-only matching for variants/subdevices. |
| `notion-ch05-part1` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 1 Platform Bus & Driver Basics.md` | read/mapped/covered/merged | Expanded teaching version of platform-bus basics: pseudo-bus explanation, when to use platform drivers, `probe()`/`remove()` responsibilities, `module_platform_driver()`, multiple devices per driver, `platform_set_drvdata()`, and `platform_device_id`. |
| `notion-ch05-part2` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 2 Probe, Remove & Resource Management.md` | read/mapped/covered/merged | Detailed probe/remove workflow, cleanup-label pattern, resources, `platform_get_resource()`, `platform_get_irq()`, manual MMIO mapping, `devm_ioremap_resource()`, `devm_request_irq()`, platform data, and remove cleanup. |
| `notion-ch05-part3` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 3 Device Provisioning & Integration.md` | read/mapped/covered/merged | Device provisioning comparison: legacy board files, runtime platform-device creation, Device Tree integration, mixed DT/id-table matching, pseudo character-device platform-driver example, and best-practice checklist. |
| `notion-ch06-part2` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 2 Device Addressing and Resources.md` | read/mapped/covered/merged | Supporting source for `simple-bus` platform-device addressing, named memory resources, named IRQs, clock names, DMA names, GPIO descriptors, and `status`-controlled device enablement. |
| `notion-ch06-part3` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 3 OF APIs and Platform Integration.md` | read/mapped/covered/merged | Supporting source for OF API integration, `of_match_ptr()`, supporting both DT and platform data, real-world DT-backed platform probe flow, and common pitfalls. |

## Source Files Read
- `ldd1-ch05`: full file read.
  - Relevant sections: `Platform drivers`, `Platform devices`, `Resources and platform data`, `Device provisioning - the old and deprecated way`, `Resources`, `Platform data`, `Where to declare platform devices?`, `Device provisioning - the new and recommended way`, `Devices, drivers, and bus matching`, `Kernel devices and drivers-matching function`, `OF style and ACPI match`, `ID table matching`, `Per device-specific data on ID table matching`, `Name matching - platform device name matching`.
- `ldd1-ch06`: relevant platform-driver/DT sections read.
  - Relevant sections: `Platform drivers and DTs`, `OF match style`, `Dealing with non-device tree platforms`, `Match style mixing`, `Platform resources and DTs`, `Platform data versus DTs`.
- `ldd1-ch11`: relevant devres section read.
  - Relevant section: `Device-managed resources - Devres`.
- `ldd2-ch01`: relevant platform-driver excerpts read.
  - Relevant sections: GPIO IRQ probe example, workqueue bottom-half platform probe, `platform_get_resource()`, `platform_get_irq()`, `request_irq()`, `devm_request_threaded_irq()`, locking from interrupt handler.
- `ldd2-ch02`: relevant platform-driver/regmap excerpts read.
  - Relevant sections: MMIO regmap probe with `platform_get_resource()` and `devm_ioremap_resource()`, chained IRQ setup with `platform_get_irq()`, MFD GPIO child platform probe with `platform_get_irq()`, `devm_kzalloc()`, `devm_gpiochip_add_data()`, and `devm_regmap_add_irq_chip()`.
- `ldd2-ch03`: relevant MFD/platform-device sections read.
  - Relevant sections: `Introducing the MFD subsystem and Syscon APIs`, `struct mfd_cell`, subdevice resources, `platform_get_irq_byname()`, `devm_mfd_add_devices()`, `mfd_add_devices()` call sequence, child platform-device data, DT binding for MFD child platform drivers, compatible matching.
- `notion-ch05-part1`: full file read.
  - Relevant sections: `5.1 Introduction to Platform Drivers`, `5.1.2 Platform Driver Structure`, `5.1.3 Platform Device Structure`, `5.1.4 Driver Registration`, `5.1.5 Complete Basic Platform Driver Example`, `5.1.6 Platform Driver with Device Creation`, `5.1.7 Multiple Devices, One Driver`, `5.1.8 Platform Device ID Table`.
- `notion-ch05-part2`: full file read.
  - Relevant sections: `5.2 The Probe Function in Detail`, `5.2.2 Resource Management`, `5.2.3 Platform Data`, `5.2.4 The Remove Function`.
- `notion-ch05-part3`: full file read.
  - Relevant sections: `5.3 Device Provisioning Methods`, `5.3.1 Legacy Board Files`, `5.3.2 Device Tree Integration`, `5.3.3 Runtime Device Creation`, `5.3.4 Complete Real-World Example`, `5.3.5 Best Practices`.
- `notion-ch06-part2`: relevant platform resource sections read.
  - Relevant sections: `6.3.4 Platform Device Addressing (simple-bus)`, `6.9 Named Resources`, `6.10 Status Property`.
- `notion-ch06-part3`: relevant platform integration sections read.
  - Relevant sections: `OF API Overview`, `Core Data Structures`, `Platform Driver with OF Match`, `Backward Compatibility - Non-DT Platforms`, `Supporting Both DT and Platform Data`, `Complete Real-World Driver Example`, `Best Practices and Common Pitfalls`.

## Merged Source Notes
- The core merged lesson should be based on `ldd1-ch05`, with `notion-ch05-part1` through `notion-ch05-part3` used to expand the explanations, examples, and checklists.
- `ldd1-ch05` is concise and source-like. It explains the pseudo platform bus, manual device provisioning, resource arrays, platform data, and the actual `platform_match()` ordering.
- `notion-ch05-part1` is the clearest beginner mental model. It should supply the first-pass explanation: platform bus is a software pseudo-bus, `probe()` is per-device, loading a driver alone does not call `probe()` until a matching device exists, and one driver may bind to multiple devices.
- `notion-ch05-part2` adds the most practical probe/remove workflow. Preserve its cleanup ordering, `devm_*` preference, `platform_set_drvdata()`/`platform_get_drvdata()` usage, and resource extraction examples.
- `notion-ch05-part3` contributes provisioning methods and a useful bridge from chapter 07 character devices into platform drivers. The pseudo character-device example is valuable for future `examples/09-*`, but should be marked learning-only and checked against current kernel headers before use.
- `ldd1-ch06`, `notion-ch06-part2`, and `notion-ch06-part3` overlap with learning-path topics 10 and 11. For topic 09, merge only the platform-driver integration pieces: OF match table, `compatible`, `reg`/`interrupts` becoming resources, named resources, and the migration away from board-file platform data. Leave DTS syntax, phandles, overlays, binding validation, and broader OF APIs for topics 10 and 11.
- `ldd1-ch11` should be merged only for the `devm_*`/devres lifetime model: managed resources are attached to `struct device`, stay available until explicit release or device detach/driver unload, and make probe error paths shorter. Broader kernel memory management belongs to topic 20.
- `ldd2` has no standalone "Platform Device Drivers" chapter. Its relevant material is framework-specific reuse of platform-driver patterns. Merge these as real-world evidence, not as the main teaching sequence.
- `ldd2-ch03` is especially useful for an advanced note: not every platform device is board-file or DT top-level; subsystems such as MFD may instantiate child platform devices with resources and parent linkage.

## Source Differences
- `ldd1-ch05` says I2C/SPI devices are "platform devices" because they are non-discoverable, then warns that I2C/SPI devices are not handled by platform drivers. For learner-facing docs, avoid ambiguous wording: I2C/SPI controllers are often platform devices on an SoC; I2C/SPI client devices are represented by their bus-specific device types and drivers.
- `ldd1-ch05` heavily discusses legacy board files and platform data. Notion repeats this but labels board files deprecated and Device Tree as modern/recommended. The final docs should teach legacy platform data for reading old code, while making DT/firmware-described devices the normal path.
- `ldd1-ch05` includes stale or typo-prone snippets (`IORESSOURCE_MEM`, `ARRY_SIZE`, mixed `void`/`int` remove examples, inconsistent driver variable names). Do not copy these snippets directly into learner-facing docs.
- `notion-ch05` examples are more readable but include training-style simplifications and some kernel-version-sensitive signatures. Use them as structure, not as build-ready code without validation.
- `ldd1-ch06` explains `platform_get_irq()` returning `-ENXIO` when no IRQ resource exists. Current driver code commonly treats any negative value from `platform_get_irq()` as the error to return, with `-EPROBE_DEFER` possible through firmware/resource dependencies.
- `ldd2-ch01` uses `gpiod_get()` instead of managed `devm_gpiod_get()` in one example and manually maps resources in another. Prefer managed APIs in the final lesson unless intentionally teaching manual cleanup.
- `ldd2-ch03` uses MFD terminology where a child "subdevice" becomes a platform device. This should be clearly scoped to MFD and not confused with V4L2 subdevices or generic platform-device provisioning.
- `ldd2-ch05` contains many hits for "platform driver" in the ASoC sense. That source was not merged as core coverage because ASoC "platform driver" refers to an audio component role, not the generic platform bus topic. It can be revisited for topics 35-36.

## Gaps / Uncertainties
- Need target-kernel check before writing buildable examples. In particular, verify `struct platform_driver` callback signatures, current `remove` expectations, `class_create()` signature if combining with char-device examples, and preferred `devm_platform_ioremap_resource()` versus explicit `platform_get_resource()` plus `devm_ioremap_resource()`.
- Need current best practice on `of_match_ptr()`. Some modern guidance discourages hiding OF match tables behind `of_match_ptr()` when the table is also needed for module autoloading or for non-OF firmware matching patterns.
- Need clarify modern firmware matching order for platform devices: OF, ACPI, platform ID table, and name fallback. `ldd1-ch05` gives the conceptual order, but source-level details may vary by kernel version.
- Need decide how deep topic 09 should go into Device Tree. The learning path has separate topics 10 and 11, so topic 09 should mention DT enough to explain modern platform-device creation and matching, then defer DTS syntax and OF API depth.
- Need decide whether to include runtime-created `platform_device` examples. They are useful for learning and tests, but production drivers should usually let firmware/board/system infrastructure instantiate devices.
- Need include deferred-probe behavior carefully: `platform_driver_probe()` is incompatible with deferred probing, while ordinary `platform_driver_register()` leaves the driver registered for later matches.
- Need external validation for driver-core lifetime rules, especially `platform_device_register()`/`platform_device_put()` on error, device release callbacks for manually created devices, and `devm_*` cleanup ordering relative to explicit subsystem unregister operations.

## External Validation
- Used: https://docs.kernel.org/6.9/driver-api/driver-model/platform.html
  - Validates platform bus purpose, `platform_device`/`platform_driver` interface, resource lists, platform-driver registration, `platform_driver_probe()`, and legacy platform-device enumeration model.
- Used: https://docs.kernel.org/driver-api/infrastructure.html
  - Validates `__platform_driver_register()`, `platform_driver_unregister()`, `__platform_driver_probe()`, `__platform_create_bundle()`, `platform_register_drivers()`, `platform_unregister_drivers()`, and platform-device helper behavior.
- Used: https://docs.kernel.org/6.2/driver-api/driver-model/driver.html
  - Validates generic driver-model callback behavior: `probe()` is the binding callback, must release resources on failure, may return `-EPROBE_DEFER` early, and `remove()` must quiesce/free per-device state.
- The learning-only example was later build-checked against local `6.8.0-111-generic` headers. Target-kernel validation is still needed when reusing the example on a different kernel.

## Learning Content Brief
- Mental model:
  - Platform bus is a kernel pseudo-bus for devices that cannot self-enumerate through PCI/USB-style discovery.
  - The kernel still wants the normal driver model: devices and drivers register with a bus, the bus matches them, and matching calls `probe()`.
  - A platform driver is not "code for a board"; it is the driver for a non-discoverable device instance described by firmware, board code, MFD core, or learning/test setup code.
- Core mechanism:
  - `struct platform_device` carries `name`, `id`, embedded `struct device`, resource array, and optional/legacy platform data.
  - `struct platform_driver` carries `probe`, `remove`, optional `id_table`, and embedded `struct device_driver`.
  - The platform bus match path should be taught as: driver override if set, OF match, ACPI match, platform ID table, then name fallback.
  - `probe()` is per matched device. `module_init()` or `module_platform_driver()` registers the driver; it is not device initialization by itself.
- APIs and data structures:
  - Driver registration: `platform_driver_register()`, `platform_driver_unregister()`, `module_platform_driver()`, `platform_driver_probe()`.
  - Device creation for learning/legacy: `platform_device_register()`, `platform_add_devices()`, `platform_device_alloc()`, `platform_device_add_resources()`, `platform_device_add_data()`, `platform_device_add()`, `platform_device_unregister()`, `platform_device_put()`.
  - Resources: `struct resource`, `IORESOURCE_MEM`, `IORESOURCE_IRQ`, `IORESOURCE_DMA`, `platform_get_resource()`, `platform_get_resource_byname()`, `platform_get_irq()`, `platform_get_irq_byname()`, `resource_size()`.
  - MMIO: `devm_ioremap_resource()` and likely `devm_platform_ioremap_resource()` after version validation.
  - Per-device state: `devm_kzalloc()`, `platform_set_drvdata()`, `platform_get_drvdata()`.
  - Matching: `struct platform_device_id`, `platform_get_device_id()`, `MODULE_DEVICE_TABLE(platform, ...)`, `struct of_device_id`, `of_match_device()`, `MODULE_DEVICE_TABLE(of, ...)`.
- Lifecycle:
  - Register device or discover it from firmware.
  - Register driver.
  - Bus matching calls `probe(pdev)`.
  - Probe reads resources/config, allocates state, maps registers, gets IRQs/clocks/GPIO/regulators, initializes hardware, registers with the relevant subsystem, and stores private data.
  - Remove/unbind unregisters from subsystems, stops hardware, disables runtime-visible activity, and lets managed resources unwind. Explicit cleanup is still required for resources that are not devm-managed or whose ordering must precede devm cleanup.
- Examples to target later:
  - Minimal driver that registers with platform bus and shows that `probe()` is not called until a device exists.
  - Runtime-created learning-only platform device plus matching driver.
  - DT-backed MMIO platform driver skeleton using `compatible`, `reg`, `interrupts`, named resources, and `devm_*`.
  - Optional bridge example: platform-driver-backed character device, clearly marked learning-only.
- Common bugs:
  - Confusing platform driver with module init.
  - Expecting `probe()` to run just because `insmod` succeeded.
  - Hardcoding physical addresses in the driver instead of reading resources.
  - Forgetting `MODULE_DEVICE_TABLE()` and breaking module autoloading.
  - Missing cleanup on probe failure.
  - Using platform data in new code where DT/fwnode properties are expected.
  - Treating I2C/SPI clients as platform-bus devices instead of using bus-specific drivers.
  - Returning success from `probe()` before the device is really usable.
  - Using `platform_driver_probe()` for devices that may need deferred probe.
- Debugging notes:
  - Inspect `/sys/bus/platform/devices/` and `/sys/bus/platform/drivers/`.
  - Check `dmesg` for probe/remove/resource failures.
  - Inspect `/lib/modules/$(uname -r)/modules.alias` for OF/platform aliases.
  - Use dynamic debug or extra `dev_dbg()` around match/probe/resource acquisition.
  - For DT systems, inspect `/proc/device-tree` or `/sys/firmware/devicetree/base`.
  - Check `-EPROBE_DEFER` in logs when suppliers such as clocks, regulators, GPIO controllers, or IRQ domains are not ready.
- Production concerns:
  - Keep driver code board-agnostic; board differences belong in firmware/DT or match data.
  - Use managed APIs where they simplify lifetime, but still explicitly unregister subsystem objects in correct order.
  - Treat `probe()` as potentially repeated across multiple device instances.
  - Validate resources before use; handle optional resources intentionally.
  - Use `dev_*()` logging tied to `&pdev->dev`.
  - Make remove/unbind paths quiesce userspace-visible operations, IRQs, workqueues, DMA, and hardware.
- Interview angles:
  - Explain why platform bus exists and how it differs from PCI/USB/I2C/SPI.
  - Explain why I2C/SPI controllers may be platform devices, while I2C/SPI client devices use their own bus drivers.
  - Walk through `probe()` from match to hardware initialization.
  - Compare board-file platform data with Device Tree.
  - Explain resource handling and why `devm_ioremap_resource()` is preferred.
  - Explain matching order and the role of `MODULE_DEVICE_TABLE()`.
  - Explain deferred probe and why `platform_driver_probe()` can be dangerous for supplier-dependent devices.
