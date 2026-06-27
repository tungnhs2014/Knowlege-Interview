# Linux Device Driver Learning Path

This file defines the canonical output order for Linux Device Driver learning material.

The numbering below is **learning-path numbering**, not raw source chapter numbering.
Raw source chapters remain metadata such as `ldd1-ch04`, `ldd2-ch04`, or
`notion-ch04-part1`.

Use these paths for finished outputs:

- `knowledge/NN-<slug>.md`
- `interview/NN-<slug>.md`
- `examples/NN-<slug>/README.md`

Audit metadata belongs in `coverage/topic-briefs/NN-<slug>.md`, not in learner-facing
`knowledge/` or `interview/` files.

## Canonical Chapters

| No. | Title | Slug | Output Paths | Scope |
| --- | --- | --- | --- | --- |
| 01 | Linux Kernel And Driver Development Overview | `linux-kernel-and-driver-development-overview` | `knowledge/01-linux-kernel-and-driver-development-overview.md`, `interview/01-linux-kernel-and-driver-development-overview.md`, `examples/01-linux-kernel-and-driver-development-overview/README.md` | Kernel space vs userspace, driver role, kernel source, coding style, build mindset. |
| 02 | Environment Setup, Kernel Source, And Build Flow | `environment-setup-kernel-source-and-build-flow` | `knowledge/02-environment-setup-kernel-source-and-build-flow.md`, `interview/02-environment-setup-kernel-source-and-build-flow.md`, `examples/02-environment-setup-kernel-source-and-build-flow/README.md` | Toolchain, kernel config, build kernel/module, headers, cross-compile basics. |
| 03 | Kernel Modules Fundamentals | `kernel-modules-fundamentals` | `knowledge/03-kernel-modules-fundamentals.md`, `interview/03-kernel-modules-fundamentals.md`, `examples/03-kernel-modules-fundamentals/README.md` | Module init/exit, `insmod`, `modprobe`, `lsmod`, dependencies, parameters, `MODULE_*`. |
| 04 | Kernel Logging, Error Handling, And Coding Practice | `kernel-logging-error-handling-and-coding-practice` | `knowledge/04-kernel-logging-error-handling-and-coding-practice.md`, `interview/04-kernel-logging-error-handling-and-coding-practice.md`, `examples/04-kernel-logging-error-handling-and-coding-practice/README.md` | `pr_*`, `dev_*`, errno, `IS_ERR/PTR_ERR`, cleanup labels, kernel style. |
| 05 | Core Kernel Facilities | `core-kernel-facilities` | `knowledge/05-core-kernel-facilities.md`, `interview/05-core-kernel-facilities.md`, `examples/05-core-kernel-facilities/README.md` | Lists, `container_of`, kobjects overview, timers, workqueues, completions, wait queues. |
| 06 | Synchronization And Concurrency Basics | `synchronization-and-concurrency-basics` | `knowledge/06-synchronization-and-concurrency-basics.md`, `interview/06-synchronization-and-concurrency-basics.md`, `examples/06-synchronization-and-concurrency-basics/README.md` | Mutex, spinlock, atomic, context rules, sleeping vs atomic context, race patterns. |
| 07 | Character Device Drivers | `character-device-drivers` | `knowledge/07-character-device-drivers.md`, `interview/07-character-device-drivers.md`, `examples/07-character-device-drivers/README.md` | `dev_t`, major/minor, `cdev`, `file_operations`, `/dev`, read/write/llseek/ioctl/poll. |
| 08 | Userspace ABI Design For Drivers | `userspace-abi-design-for-drivers` | `knowledge/08-userspace-abi-design-for-drivers.md`, `interview/08-userspace-abi-design-for-drivers.md`, `examples/08-userspace-abi-design-for-drivers/README.md` | sysfs, procfs/debugfs overview, ioctl ABI, device nodes, compatibility concerns. |
| 09 | Platform Bus And Platform Drivers | `platform-bus-and-platform-drivers` | `knowledge/09-platform-bus-and-platform-drivers.md`, `interview/09-platform-bus-and-platform-drivers.md`, `examples/09-platform-bus-and-platform-drivers/README.md` | `platform_device`, `platform_driver`, probe/remove, resources, `devm_*`. |
| 10 | Device Tree Fundamentals | `device-tree-fundamentals` | `knowledge/10-device-tree-fundamentals.md`, `interview/10-device-tree-fundamentals.md`, `examples/10-device-tree-fundamentals/README.md` | DTS syntax, nodes/properties, compatible, reg, interrupts, phandles, overlays basics. |
| 11 | Device Tree APIs And Driver Integration | `device-tree-apis-and-driver-integration` | `knowledge/11-device-tree-apis-and-driver-integration.md`, `interview/11-device-tree-apis-and-driver-integration.md`, `examples/11-device-tree-apis-and-driver-integration/README.md` | OF APIs, matching, resources from DT, common DT mistakes, binding thinking. |
| 12 | Linux Device Model | `linux-device-model` | `knowledge/12-linux-device-model.md`, `interview/12-linux-device-model.md`, `examples/12-linux-device-model/README.md` | `struct device`, driver/bus/class, sysfs, uevents, device lifetime. |
| 13 | Pin Control And GPIO Consumer APIs | `pin-control-and-gpio-consumer-apis` | `knowledge/13-pin-control-and-gpio-consumer-apis.md`, `interview/13-pin-control-and-gpio-consumer-apis.md`, `examples/13-pin-control-and-gpio-consumer-apis/README.md` | Pinctrl states, GPIO descriptors, active-low, sleep/default states. |
| 14 | GPIO Controller Drivers And IRQ Integration | `gpio-controller-drivers-and-irq-integration` | `knowledge/14-gpio-controller-drivers-and-irq-integration.md`, `interview/14-gpio-controller-drivers-and-irq-integration.md`, `examples/14-gpio-controller-drivers-and-irq-integration/README.md` | `gpio_chip`, irqchip integration, chained/nested IRQ, GPIO controller design. |
| 15 | Interrupt Management | `interrupt-management` | `knowledge/15-interrupt-management.md`, `interview/15-interrupt-management.md`, `examples/15-interrupt-management/README.md` | IRQ flow, top half/bottom half, threaded IRQ, request/free IRQ, IRQ domains. |
| 16 | I2C Client Drivers | `i2c-client-drivers` | `knowledge/16-i2c-client-drivers.md`, `interview/16-i2c-client-drivers.md`, `examples/16-i2c-client-drivers/README.md` | Adapter/client model, probe, register access, DT matching, SMBus/I2C APIs. |
| 17 | SPI Device Drivers | `spi-device-drivers` | `knowledge/17-spi-device-drivers.md`, `interview/17-spi-device-drivers.md`, `examples/17-spi-device-drivers/README.md` | Controller/device model, `spi_transfer`, `spi_message`, full-duplex behavior, DT. |
| 18 | Regmap API | `regmap-api` | `knowledge/18-regmap-api.md`, `interview/18-regmap-api.md`, `examples/18-regmap-api/README.md` | Register abstraction, cache, volatile/precious registers, bulk/update bits, debugfs. |
| 19 | Regmap IRQ And MFD/Syscon | `regmap-irq-and-mfd-syscon` | `knowledge/19-regmap-irq-and-mfd-syscon.md`, `interview/19-regmap-irq-and-mfd-syscon.md`, `examples/19-regmap-irq-and-mfd-syscon/README.md` | Regmap IRQ, MFD cells, syscon, simple-mfd, shared register blocks. |
| 20 | Kernel Memory Management For Drivers | `kernel-memory-management-for-drivers` | `knowledge/20-kernel-memory-management-for-drivers.md`, `interview/20-kernel-memory-management-for-drivers.md`, `examples/20-kernel-memory-management-for-drivers/README.md` | `kmalloc`, `kzalloc`, `vmalloc`, GFP flags, DMA-safe memory, lifetime. |
| 21 | DMA And DMA Mapping | `dma-and-dma-mapping` | `knowledge/21-dma-and-dma-mapping.md`, `interview/21-dma-and-dma-mapping.md`, `examples/21-dma-and-dma-mapping/README.md` | Coherent vs streaming DMA, scatter-gather, cache coherency, DMA API traps. |
| 22 | Common Clock Framework | `common-clock-framework` | `knowledge/22-common-clock-framework.md`, `interview/22-common-clock-framework.md`, `examples/22-common-clock-framework/README.md` | Clock providers/consumers, prepare/enable, rates, parent clocks, debugfs. |
| 23 | Regulator Framework | `regulator-framework` | `knowledge/23-regulator-framework.md`, `interview/23-regulator-framework.md`, `examples/23-regulator-framework/README.md` | Regulator consumers/providers, enable/disable, voltage/current constraints, DT. |
| 24 | Power Management | `power-management` | `knowledge/24-power-management.md`, `interview/24-power-management.md`, `examples/24-power-management/README.md` | Runtime PM, system sleep, suspend/resume, wakeup, ordering and common bugs. |
| 25 | IIO Framework | `iio-framework` | `knowledge/25-iio-framework.md`, `interview/25-iio-framework.md`, `examples/25-iio-framework/README.md` | Sensors, channels, buffers, triggers, sysfs ABI, practical sensor driver design. |
| 26 | Input Device Drivers | `input-device-drivers` | `knowledge/26-input-device-drivers.md`, `interview/26-input-device-drivers.md`, `examples/26-input-device-drivers/README.md` | Input core, input devices, events, key/absolute axes, userspace event nodes, debounce and IRQ-driven input. |
| 27 | RTC And PWM Drivers | `rtc-and-pwm-drivers` | `knowledge/27-rtc-and-pwm-drivers.md`, `interview/27-rtc-and-pwm-drivers.md`, `examples/27-rtc-and-pwm-drivers/README.md` | RTC registration, time/alarm callbacks, PWM providers/consumers, period/duty-cycle semantics. |
| 28 | Watchdog And NVMEM Frameworks | `watchdog-and-nvmem-frameworks` | `knowledge/28-watchdog-and-nvmem-frameworks.md`, `interview/28-watchdog-and-nvmem-frameworks.md`, `examples/28-watchdog-and-nvmem-frameworks/README.md` | Watchdog operations, timeout/keepalive, NVMEM providers/consumers, cells, DT and sysfs access. |
| 29 | Framebuffer And Display Basics | `framebuffer-and-display-basics` | `knowledge/29-framebuffer-and-display-basics.md`, `interview/29-framebuffer-and-display-basics.md`, `examples/29-framebuffer-and-display-basics/README.md` | Framebuffer model, memory mapping, legacy display path, and modern display context. |
| 30 | Network Interface Drivers | `network-interface-drivers` | `knowledge/30-network-interface-drivers.md`, `interview/30-network-interface-drivers.md`, `examples/30-network-interface-drivers/README.md` | `net_device`, NAPI, skb, TX/RX path, ethtool, interrupts, DMA relation. |
| 31 | PCI Device Drivers | `pci-device-drivers` | `knowledge/31-pci-device-drivers.md`, `interview/31-pci-device-drivers.md`, `examples/31-pci-device-drivers/README.md` | PCI IDs, BARs, config space, enable device, MSI/MSI-X, DMA, probe/remove. |
| 32 | V4L2 Core, Video Device, And VB2 Capture | `v4l2-core-video-device-and-vb2-capture` | `knowledge/32-v4l2-core-video-device-and-vb2-capture.md`, `interview/32-v4l2-core-video-device-and-vb2-capture.md`, `examples/32-v4l2-core-video-device-and-vb2-capture/README.md` | `v4l2_device`, `video_device`, file/ioctl ops, vb2 queues, buffers, streaming, DMA handoff. |
| 33 | V4L2 Async, Subdevs, And Media Controller | `v4l2-async-subdevs-and-media-controller` | `knowledge/33-v4l2-async-subdevs-and-media-controller.md`, `interview/33-v4l2-async-subdevs-and-media-controller.md`, `examples/33-v4l2-async-subdevs-and-media-controller/README.md` | Sensor sub-devices, async notifier, media entities/pads/links, bridge-to-sensor integration, controls. |
| 34 | V4L2 Userspace, Debugging, And Compliance | `v4l2-userspace-debugging-and-compliance` | `knowledge/34-v4l2-userspace-debugging-and-compliance.md`, `interview/34-v4l2-userspace-debugging-and-compliance.md`, `examples/34-v4l2-userspace-debugging-and-compliance/README.md` | `v4l2-ctl`, formats, buffer APIs, streaming tests, dynamic debug, `v4l2-compliance`, ffmpeg/gstreamer checks. |
| 35 | ASoC Codec, Component, DAI, And DAPM | `asoc-codec-component-dai-and-dapm` | `knowledge/35-asoc-codec-component-dai-and-dapm.md`, `interview/35-asoc-codec-component-dai-and-dapm.md`, `examples/35-asoc-codec-component-dai-and-dapm/README.md` | Codec/component drivers, DAI ops, PCM constraints, kcontrols, DAPM widgets/routes. |
| 36 | ASoC Machine Drivers And Audio Routing | `asoc-machine-drivers-and-audio-routing` | `knowledge/36-asoc-machine-drivers-and-audio-routing.md`, `interview/36-asoc-machine-drivers-and-audio-routing.md`, `examples/36-asoc-machine-drivers-and-audio-routing/README.md` | Sound card registration, DAI links, CPU/codec/platform binding, routing, clocks, board-specific audio integration. |
| 37 | Kernel Debugging And Tracing | `kernel-debugging-and-tracing` | `knowledge/37-kernel-debugging-and-tracing.md`, `interview/37-kernel-debugging-and-tracing.md`, `examples/37-kernel-debugging-and-tracing/README.md` | Dynamic debug, ftrace, tracepoints, debugfs, crash clues, practical workflows. |
| 38 | Production Driver Checklist And Interview Review | `production-driver-checklist-and-interview-review` | `knowledge/38-production-driver-checklist-and-interview-review.md`, `interview/38-production-driver-checklist-and-interview-review.md` | Probe/remove checklist, locking/lifetime, ABI, DT binding, power, DMA, debug, senior interview scenarios. |

## Usage Rules

- Load this file before choosing or validating a learning-path number.
- Keep source chapter identity in coverage notes, not in output filenames.
- Keep learner-facing `knowledge/` and `interview/` files free of audit tables.
- If a topic spans multiple rows, choose the row that represents the primary learning goal and link related rows in the Topic Brief.
- If a new topic does not fit any row, mark it as a gap instead of inventing a new number without updating this file.
