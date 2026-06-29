# Topic Brief - 24 - Power Management

## Output Targets
- Knowledge: `knowledge/24-power-management.md`
- Interview: `interview/24-power-management.md`
- Example: `examples/24-power-management/README.md`

## Source Coverage
| Source ID | Path | Status | Key Contribution |
| --- | --- | --- | --- |
| `ldd1-source-root` | `docs/Linux Device Driver Development/` | searched/mapped/gap | No dedicated power-management chapter found in book 1. Relevant material is distributed across device-model, IRQ, RTC, regulator, IIO, I2C, and framebuffer chapters. |
| `ldd1-ch13` | `docs/Linux Device Driver Development/Chapter 13-The Linux Device Model.md` | read/mapped/covered/merged-adjacent | Device model reason for PM ordering; `struct bus_type` and `struct device_driver` suspend/resume fields; `const struct dev_pm_ops *pm`; parent/device hierarchy as basis for ordered PM transitions. |
| `ldd1-ch18` | `docs/Linux Device Driver Development/Chapter 18-RTC Drivers.md` | read/mapped/covered/merged | RTC alarm as wakeup source; `device_init_wakeup()`, `dev_pm_set_wake_irq()`, RTC `wakealarm`, `CONFIG_RTC_HCTOSYS`, and system time restore on resume. |
| `ldd1-ch20` | `docs/Linux Device Driver Development/Chapter 20-Regulator Framework.md` | read/mapped/covered/merged-adjacent | Regulator constraints include suspend states for disk, mem, and standby; useful for explaining power-resource state during system sleep. Full regulator framework remains topic 23. |
| `ldd1-ch16` | `docs/Linux Device Driver Development/Chapter 16-Advanced IRQ Management.md` | read/mapped/covered/merged-adjacent | IRQ descriptor PM fields and suspend-related IRQ action flags such as `IRQF_NO_SUSPEND` and `IRQF_FORCE_RESUME`; detailed IRQ management remains topic 15. |
| `ldd1-ch10` | `docs/Linux Device Driver Development/Chapter 10-IIO Framework.md` | read/mapped/related | IIO probe/error path uses `err_suspend` and device deinit around sensor setup; useful as a sensor lifecycle boundary but not deep runtime PM. Full IIO remains topic 25. |
| `ldd1-ch07` | `docs/Linux Device Driver Development/Chapter 7-I2C Client Drivers.md` | read/mapped/related | Generic reminder that I2C drivers may implement PM callbacks and that probe/remove shape driver behavior. Full I2C remains topic 16. |
| `ldd1-ch21` | `docs/Linux Device Driver Development/Chapter 21-Framebuffer Drivers.md` | read/mapped/related | Display blanking modes include vsync/hsync suspend and powerdown; applied display PM context. Full display remains topic 29. |
| `ldd2-ch10` | `docs/Linux Device Driver Development 2/Chapter 10-Linux_Kernel_Power_Management.md` | read/mapped/covered/merged | Primary source: runtime PM, system sleep states, CPU idle, CPUfreq/DVFS, thermal, `struct dev_pm_ops`, runtime PM callbacks/helpers/counters/autosuspend, power domains/genpd, suspend/resume phases, wakeup sources, sysfs/debugfs, `enable_irq_wake()`, `pm_wakeup_event()`, and `IRQF_NO_SUSPEND` distinction. |
| `ldd2-ch01` | `docs/Linux Device Driver Development 2/Chapter 1-Linux_Kernel_Concepts.md` | read/mapped/covered/merged-adjacent | Workqueue freezer behavior via `WQ_FREEZABLE`; IRQ suspend flags and warning about `IRQF_NO_SUSPEND` with shared IRQ lines. Detailed workqueues/IRQs remain topics 05 and 15. |
| `ldd2-ch03` | `docs/Linux Device Driver Development 2/Chapter 3-MFD_Subsystem_and_Syscon_API.md` | read/mapped/covered/merged-adjacent | MFD cells may carry suspend/resume callbacks; SNVS syscon example includes `syscon-poweroff`, power key, and `wakeup-source`. Full MFD/syscon remains topic 19. |
| `ldd2-ch05` | `docs/Linux Device Driver Development 2/Chapter 5-ASoC_Framework.md` | read/mapped/related | ASoC component/DAI suspend/resume, DAPM as dynamic audio power management, runtime-PM cleanup labels in snippets. Full ASoC remains topics 35-36. |
| `ldd2-ch06` | `docs/Linux Device Driver Development 2/Chapter 6-ASoC_Machine_Drivers.md` | read/mapped/related | `ignore_suspend` DAI link flag and applied audio suspend context. Full machine-driver routing remains topic 36. |
| `ldd2-ch14` | `docs/Linux Device Driver Development 2/Chapter 14-Linux_Kernel_Debugging_Tips.md` | read/mapped/related | Wakeup latency tracers and ftrace event lists include suspend-related driver events; detailed tracing remains topic 37. |
| `ldd2-ch09` | `docs/Linux Device Driver Development 2/Chapter 9-Leveraging the V4L2.md` | read/mapped/index-adjacent | Ends by pointing to Linux kernel power management as the next chapter; no direct PM mechanism beyond media context. |
| `notion-index` | `docs/Linux-Device-Driver-Notion/Linux Device Drivers.md` | inspected/mapped/index-only | Notion index inspected; no standalone power-management chapter file is present despite the Chapter 9 note that the next chapter would be Linux Kernel Power Management. |
| `notion-ch01-part2` | `docs/Linux-Device-Driver-Notion/Chapter 1-Part 2 Kernel Configuration.md` | read/mapped/related | Kconfig menu orientation includes power-management and ACPI options; useful only as configuration context. |
| `notion-ch05-part1` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 1 Platform Bus & Driver Basics.md` | read/mapped/covered/merged-adjacent | Platform driver structure includes legacy suspend/resume callbacks; clarifies platform drivers as common PM participants. |
| `notion-ch05-part2` | `docs/Linux-Device-Driver-Notion/Chapter 5-Part 2 Probe, Remove & Resource Management.md` | read/mapped/covered/merged-adjacent | Probe/remove resource and hardware-init ordering; useful for PM lifecycle and cleanup grounding. |
| `notion-ch03-part3` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 3 Work Queues and Scheduling.md` | read/mapped/covered/merged-adjacent | Workqueue flags include `WQ_FREEZABLE`; suspend example cancels/flushing delayed work before device suspend. |
| `notion-ch03-part4` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 4 Work Queues and Scheduling.md` | read/mapped/covered/merged-adjacent | Tasklet suspend/resume example disables tasklets before powering down and re-enables after power-up. Tasklets remain topic 15. |
| `notion-ch03-part5` | `docs/Linux-Device-Driver-Notion/Chapter 3-Part 5 Work Queues and Scheduling.md` | read/mapped/related | Timer clock note distinguishes `CLOCK_MONOTONIC` from `CLOCK_BOOTTIME` including suspend time. Full timers remain topic 05. |
| `notion-ch06-part3` | `docs/Linux-Device-Driver-Notion/Chapter 6-Part 3 OF APIs and Platform Integration.md` | read/mapped/covered/merged-adjacent | Device Tree OPP tuple example with frequency and voltage; supports OPP/DVFS discussion. Full OF APIs remain topic 11. |
| `notion-ch14-part1` | `docs/Linux-Device-Driver-Notion/Chapter 14-Part 1 Pin Control Framework.md` | read/mapped/covered/merged-adjacent | Pinctrl sleep/default state example in suspend/resume; notes multi-state PM support. Full pinctrl remains topic 13. |
| `notion-ch07-extra-v4l2-part2` | `docs/Linux-Device-Driver-Notion/Chapter 7-v4l2_part2.md` | read/mapped/related | Camera/video lifecycle with power sequencing and streaming stop context; detailed V4L2 remains topics 32-34. |
| `notion-ch16-part1` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 1 IRQ Architecture and Propagation.md` | read/mapped/related | IRQ-domain and wake path background; generic IRQ details remain topic 15. |
| `notion-ch16-part2` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 2 IRQ Multiplexing - Chained and Nested Inter.md` | read/mapped/related | Nested/chained IRQ context and restore examples; generic IRQ multiplexing remains topic 15. |
| `notion-ch16-part3` | `docs/Linux-Device-Driver-Notion/Chapter 16-Part 3 Advanced Topics.md` | read/mapped/related | PMIC and IRQ architecture context; useful only as PMIC/wakeup-adjacent background. |
| `notion-ch09-extra-userspace-part2` | `docs/Linux-Device-Driver-Notion/Chapter 9-userspace-part2.md` | read/mapped/index-adjacent | Explicit note that the next chapter would be Linux Kernel Power Management, but no corresponding Notion PM chapter file exists in the source root. |

## Source Files Read
- `ldd2-ch10`: complete chapter. Read CPU idle, CPU hotplug, CPUfreq/DVFS, OPP, thermal, system sleep states, `/sys/power/state`, `/sys/power/mem_sleep`, `/sys/power/disk`, RTC `wakealarm` test, `struct device` PM fields, `struct dev_pm_ops`, runtime PM callbacks, `SET_RUNTIME_PM_OPS()`, `pm_runtime_force_suspend()`, `pm_runtime_force_resume()`, usage/child counters, `pm_runtime_enable()`, `pm_runtime_set_active()`, `pm_runtime_get_sync()`, `pm_runtime_resume_and_get()` gap via external validation, async runtime PM, autosuspend, bh1780 IIO case study, power domains/genpd, suspend/resume phases, system sleep callbacks and macros, wakeup sources, `device_init_wakeup()`, `enable_irq_wake()`, `device_may_wakeup()`, `pm_wakeup_event()`, wakeup sysfs/debugfs, and `IRQF_NO_SUSPEND`.
- `ldd1-ch13`: device-model sections for `struct bus_type`, `struct device_driver`, parent hierarchy, PM operation pointers, and ordered PM transitions.
- `ldd1-ch18`: RTC wakeup source section and RTC sysfs/resume time sections.
- `ldd1-ch20`: regulator constraints section covering suspend states and initial state/mode.
- `ldd1-ch16`: IRQ descriptor and suspend-related IRQ action fields.
- `ldd1-ch10`, `ldd1-ch07`, `ldd1-ch21`: targeted PM-adjacent snippets around IIO error cleanup, I2C PM callback reminder, and framebuffer blank/powerdown modes.
- `ldd2-ch01`: workqueue flags and IRQ suspend flag sections.
- `ldd2-ch03`: MFD cell suspend/resume fields and SNVS `wakeup-source`/power key/syscon-poweroff excerpt.
- `ldd2-ch05`, `ldd2-ch06`, `ldd2-ch14`, `ldd2-ch09`: targeted ASoC, audio machine, tracing, and chapter-boundary snippets.
- `notion-index`: inspected to verify no standalone Notion PM chapter file.
- `notion-ch05-part1` and `notion-ch05-part2`: platform driver suspend/resume and probe/remove lifecycle sections.
- `notion-ch03-part3`, `notion-ch03-part4`, `notion-ch03-part5`: workqueue freezer, delayed-work suspend cleanup, tasklet suspend, and suspend-time clock notes.
- `notion-ch06-part3`: OF/OPP integer-array example.
- `notion-ch14-part1`: pinctrl sleep/default state suspend/resume example.
- `notion-ch07-extra-v4l2-part2`: media power/streaming lifecycle context.
- `notion-ch16-part1`, `notion-ch16-part2`, `notion-ch16-part3`: IRQ and PMIC-adjacent context.
- `notion-ch09-extra-userspace-part2`: inspected because it announces a PM next chapter; treated as index-adjacent because no PM chapter file exists.

### Inventory Decisions
- `ldd2-ch10` is the only dedicated internal power-management source and is primary for topic 24.
- Book 1 has no dedicated PM chapter; it contributes framework hooks and applied PM examples. It is recorded as `ldd1-source-root` gap plus adjacent source rows rather than treated as complete PM coverage.
- Notion has no standalone PM chapter file despite a "Next Chapter: Linux Kernel Power Management" note. Notion PM content is distributed across platform, workqueue, OF/OPP, pinctrl, V4L2, and IRQ notes.
- Same-number sources were not merged by number. For example, `ldd2-ch10` is the dedicated PM chapter, while Notion has no Chapter 10 PM file in the root; Notion Chapter 5 and Chapter 3 files were read as platform/workqueue context, not as book-2 chapter equivalents.
- CPU idle, CPUfreq, thermal, OPP, power domains, CCF, regulators, pinctrl, IRQ, RTC, ASoC, V4L2, and display are retained only as PM context where they shape driver lifecycle. Their subsystem-specific behavior remains in their dedicated learning-path topics.

## Merged Source Notes
- Linux power management has two driver-relevant models:
  - Runtime PM: per-device low-power transitions while the system is running.
  - System sleep PM: coordinated whole-system transitions such as freeze, standby, suspend-to-RAM, and hibernation.
- Runtime PM and system sleep are related but not interchangeable. A runtime-suspended device may need special handling during system sleep, and system sleep callbacks must account for device state, wakeup policy, and subsystem rules.
- The device model is the foundation for PM ordering. `struct device` contains PM state, parent/child relationships, bus/class/type links, and optional power-domain membership. Parent devices generally cannot runtime suspend while children are active unless policy such as `pm_suspend_ignore_children()` is used.
- `struct dev_pm_ops` is the central callback table. Runtime callbacks are `runtime_suspend`, `runtime_resume`, and `runtime_idle`. System-sleep callbacks include `prepare`, `complete`, `suspend`, `resume`, `freeze`, `thaw`, `poweroff`, `restore`, plus late/noirq variants.
- Runtime PM state is driven by a usage counter and active-child counter. Drivers increment the usage count before accessing hardware and decrement it when finished. When the count reaches zero, the PM core may run idle/suspend flow immediately or through autosuspend.
- Autosuspend delays suspension after the last activity to avoid expensive rapid power cycling. Drivers use `pm_runtime_use_autosuspend()`, `pm_runtime_set_autosuspend_delay()`, `pm_runtime_mark_last_busy()`, and `pm_runtime_put_autosuspend()`.
- Runtime PM helpers differ in important ways:
  - `pm_runtime_get_sync()` increments the usage count and resumes synchronously, but current headers warn that the count remains incremented even on error.
  - `pm_runtime_resume_and_get()` is the cleaner current helper when the caller checks the return value.
  - `pm_runtime_put()`, `pm_runtime_put_sync()`, `pm_runtime_put_noidle()`, and `pm_runtime_put_autosuspend()` differ in whether they run idle checks synchronously, asynchronously, or not at all.
- System sleep states exposed through `/sys/power/state` include `freeze`, `standby`, `mem`, and `disk` when supported. `/sys/power/mem_sleep` chooses the meaning of `mem` among `s2idle`, `shallow`, and `deep`; `/sys/power/disk` chooses hibernation behavior.
- System suspend rough order: sync, notifiers, freeze tasks, suspend devices, disable device IRQs, noirq suspend, offline non-boot CPUs, disable interrupts, syscore suspend, enter low-power state. Resume reverses this through syscore, IRQ/CPU restore, noirq resume, IRQ enable, device resume, task thaw, and notifiers.
- System-sleep callback grouping matters:
  - `suspend`/`resume` for memory-preserving sleep.
  - `freeze`/`thaw` and `poweroff`/`restore` for hibernation.
  - `suspend_late`/`resume_early` and `suspend_noirq`/`resume_noirq` for ordering-sensitive operations.
- Current headers still expose `SET_*` macros used by the source, but `SIMPLE_DEV_PM_OPS()` is deprecated in favor of `DEFINE_SIMPLE_DEV_PM_OPS()`. Current docs also mention newer PM flags such as smart-suspend behavior that internal sources do not cover.
- Wakeup sources are devices that can abort or exit system sleep. The capability is declared by the driver/hardware; the policy is exposed through `/sys/devices/.../power/wakeup`.
- Wakeup IRQ setup is separate from `IRQF_NO_SUSPEND`. `enable_irq_wake()` arms an IRQ as a wake event; `IRQF_NO_SUSPEND` keeps an IRQ enabled during suspend and is not sufficient to wake the system.
- Wakeup debugging uses `/sys/kernel/debug/wakeup_sources` and per-device `/sys/devices/.../power/wakeup*` attributes where available.
- Power domains/genpd extend runtime PM to groups of devices sharing power resources and may override bus/class/type PM callbacks. DT describes provider/consumer power-domain links.
- Device drivers must quiesce users, DMA, IRQs, workqueues, timers/tasklets, and debug paths before powering down hardware. Notion workqueue/tasklet snippets support this lifecycle view.
- PM interacts with resources covered in adjacent topics: clocks, regulators, resets, pinctrl states, GPIOs, IRQ wake, DMA, subsystem registration, and userspace ABI.

## Source Differences
- `ldd2-ch10` targets Linux 4.19. It uses `SIMPLE_DEV_PM_OPS()` and `SET_*` macros as normal examples. Current headers mark `SIMPLE_DEV_PM_OPS()` deprecated in favor of `DEFINE_SIMPLE_DEV_PM_OPS()`, while `SET_SYSTEM_SLEEP_PM_OPS()` and `SET_RUNTIME_PM_OPS()` still exist.
- `ldd2-ch10` teaches `pm_runtime_get_sync()` in access paths. Current `include/linux/pm_runtime.h` warns that if it returns an error, the usage counter remains incremented, and suggests `pm_runtime_resume_and_get()` for checked paths.
- `ldd2-ch10` says asynchronous PM helpers are safe from atomic context because work is queued. Current docs add an important exception: synchronous helpers in atomic context require `pm_runtime_irq_safe()` and non-sleeping callbacks. Learner docs should teach context rules carefully.
- `ldd2-ch10` states that system PM actions are always initiated from userspace. That is mostly true for ordinary manual sleep via `/sys/power/state`, but modern systems may have policy daemons, firmware/ACPI/platform events, and kernel-mediated autosleep paths. Teach user/sysfs control as the common interface, not the only policy path.
- `ldd2-ch10` uses old text binding names such as `opp.txt`, `thermal.txt`, and `power_domain.txt`. Current docs have moved many bindings to YAML schemas. Learner docs should avoid relying on old text binding paths.
- `ldd2-ch10` describes `enable_irq_wake()` in suspend/resume. Book 1 RTC also mentions `dev_pm_set_wake_irq()`. Learner docs should compare both: direct wake-IRQ enable/disable in PM callbacks versus managed wake IRQ association where suitable.
- Notion platform-driver material lists legacy `.suspend`/`.resume` members in `struct platform_driver`. Modern driver style usually uses `.driver.pm = &pm_ops` with `struct dev_pm_ops`.
- Notion tasklet examples are useful for "quiesce before power down", but tasklets are legacy-ish in modern kernels. For learner docs, frame them as deferred-work lifecycle examples and prefer modern threaded IRQ/workqueue patterns where applicable.
- ASoC DAPM is a subsystem-specific dynamic power model. It should be mentioned as an example of subsystem PM policy, not taught as generic runtime PM.
- Framebuffer blanking is legacy display PM context. Modern DRM/display PM belongs in display topics; topic 24 should only use it as a simple "blank/powerdown is not the same as whole-system suspend" comparison if useful.

## Gaps / Uncertainties
- Internal sources do not deeply cover current `pm_runtime_resume_and_get()`, `devm_pm_runtime_enable()`, runtime PM guard macros, device links, `DPM_FLAG_SMART_SUSPEND`, `DPM_FLAG_MAY_SKIP_RESUME`, or newer managed helpers.
- Internal sources do not deeply cover current generic PM domain provider APIs, OPP library APIs, device links, or interconnect/PM QoS constraints. Topic 24 should introduce these as related mechanisms and defer deep provider implementation unless external validation is added.
- Internal sources do not provide a modern buildable example. A later example should likely be a learning-only platform/I2C-style pseudo-driver or trace/debug workflow rather than a fake hardware module that cannot validate real PM transitions.
- Internal sources do not cover suspend/resume debugging in depth: `pm_test`, `no_console_suspend`, `initcall_debug`, `/sys/power/pm_debug_messages`, ftrace power events, wakeup-source diagnosis, or dynamic debug in PM callbacks. External validation is needed before final learner docs.
- Internal sources do not explain userspace ABI stability around `/sys/devices/.../power/control`, autosuspend attributes, or wakeup attributes in modern kernels.
- Internal sources do not deeply cover runtime PM with DMA, IRQs, open file handles, subsystem streaming users, or remove/unbind races. Final docs should add production-focused lifecycle rules.
- PREEMPT_RT and atomic-context constraints are only adjacent in IRQ material. Final docs should avoid broad claims that PM callbacks are atomic-safe.

## External Validation
- Used: `https://docs.kernel.org/power/runtime_pm.html`
  - Validated current runtime PM model: `pm_wq`, `struct dev_pm_info`, runtime PM callbacks, subsystem callback precedence, process-context default, `pm_runtime_irq_safe()`, usage/child counters, idle/suspend/resume rules, and current helper behavior.
- Used: `https://docs.kernel.org/driver-api/pm/devices.html`
  - Validated current device PM basics: system sleep versus runtime PM models, wakeup policy, quiesced I/O expectations, subsystem collaboration, and interaction between runtime-suspended devices and system-wide transitions.
- Used: `https://docs.kernel.org/admin-guide/pm/sleep-states.html`
  - Validated current `/sys/power/state`, `/sys/power/mem_sleep`, `/sys/power/disk`, suspend-to-idle/standby/suspend-to-RAM/hibernation terminology, and current sysfs interface framing.
- Used: `https://docs.kernel.org/driver-api/pm/index.html`
  - Validated current PM documentation organization and additional modern topics that internal sources do not cover, such as smart suspend and may-skip-resume flags.
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/pm_runtime.h`
  - Validated current helper drift: `pm_runtime_get_sync()` usage-counter behavior on error, recommendation to consider `pm_runtime_resume_and_get()`, `pm_runtime_put_autosuspend()` marking last busy, and current runtime PM guard macros.
- Used: `https://raw.githubusercontent.com/torvalds/linux/master/include/linux/pm.h`
  - Validated current `struct dev_pm_ops`, `SYSTEM_SLEEP_PM_OPS`, `RUNTIME_PM_OPS`, `SET_*` macros, `DEFINE_SIMPLE_DEV_PM_OPS()`, and deprecation notes for `SIMPLE_DEV_PM_OPS()` and `UNIVERSAL_DEV_PM_OPS()`.
- Still needed before final learner/example files:
  - Validate any example against target kernel headers, especially `pm_runtime_resume_and_get()`, `devm_pm_runtime_enable()`, `DEFINE_RUNTIME_DEV_PM_OPS()`, and PM guard macro availability.
  - Validate debug commands against the target kernel config: debugfs, ftrace power events, `pm_test`, wakeup-source attributes, runtime PM sysfs attributes, and autosuspend policy files.
  - If using DT examples, validate modern YAML binding names for `wakeup-source`, `power-domains`, OPP tables, clocks, regulators, pinctrl sleep states, and wake IRQs.

## Learning Content Brief
- Learning path number: `24`.
- Slug: `power-management`.
- Topic scope:
  - Runtime PM, system sleep, suspend/resume ordering, wakeup sources, autosuspend, PM callback tables, PM domains, PM interactions with clocks/regulators/pinctrl/IRQs/DMA/workqueues, debugging, common bugs, and version-sensitive API drift.
  - Keep CCF details in topic 22, regulator details in topic 23, generic IRQ details in topic 15, pinctrl/GPIO details in topics 13-14, IIO details in topic 25, RTC/PWM in topic 27, watchdog/NVMEM in topic 28, display/media/audio subsystem details in topics 29 and 32-36, and tracing depth in topic 37.
- Beginner mental model:
  - Runtime PM is "turn this device down while Linux is still running".
  - System sleep is "coordinate the whole machine into a sleep state".
  - Wakeup sources are "devices allowed to keep or wake the system up".
  - PM is mostly about safe ordering: stop users, stop I/O, save state, disable interrupts/clocks/regulators, sleep, then restore in the right order.
- Core mechanism:
  - `struct device` stores PM state and hierarchy.
  - PM core chooses callbacks through PM domain, device type, class, bus, and driver PM ops.
  - Runtime PM uses device usage count, active-child count, runtime status, async PM work, and autosuspend timer.
  - System sleep walks devices through prepare/suspend/late/noirq phases, enters low-power state, then resumes in reverse phases.
  - Wakeup sources attach to devices and expose policy/statistics through sysfs/debugfs.
- Important structs/APIs:
  - Structs: `struct device`, `struct dev_pm_info`, `struct dev_pm_ops`, `struct wakeup_source`, `struct dev_pm_domain`.
  - Runtime PM: `pm_runtime_enable()`, `pm_runtime_disable()`, `pm_runtime_set_active()`, `pm_runtime_set_suspended()`, `pm_runtime_get_noresume()`, `pm_runtime_resume_and_get()`, `pm_runtime_get_sync()`, `pm_runtime_put()`, `pm_runtime_put_sync()`, `pm_runtime_put_noidle()`, `pm_runtime_use_autosuspend()`, `pm_runtime_set_autosuspend_delay()`, `pm_runtime_mark_last_busy()`, `pm_runtime_put_autosuspend()`, `pm_runtime_force_suspend()`, `pm_runtime_force_resume()`, `pm_suspend_ignore_children()`, `pm_runtime_irq_safe()`.
  - PM ops helpers: `RUNTIME_PM_OPS()`, `SET_RUNTIME_PM_OPS()`, `SYSTEM_SLEEP_PM_OPS()`, `SET_SYSTEM_SLEEP_PM_OPS()`, `NOIRQ_SYSTEM_SLEEP_PM_OPS()`, `DEFINE_SIMPLE_DEV_PM_OPS()`, `DEFINE_RUNTIME_DEV_PM_OPS()` where target headers support it.
  - Wakeup: `device_init_wakeup()`, `device_can_wakeup()`, `device_may_wakeup()`, `dev_pm_set_wake_irq()`, `enable_irq_wake()`, `disable_irq_wake()`, `pm_wakeup_event()`, `IRQF_NO_SUSPEND`.
  - User/debug interfaces: `/sys/power/state`, `/sys/power/mem_sleep`, `/sys/power/disk`, `/sys/devices/.../power/control`, `/sys/devices/.../power/autosuspend_delay_ms`, `/sys/devices/.../power/wakeup`, `/sys/kernel/debug/wakeup_sources`, ftrace power events where available.
- Lifecycle/data flow:
  - Probe: acquire resources; initialize hardware; set initial PM state coherently; enable runtime PM; set autosuspend delay if useful; register subsystem users only when hardware state is valid.
  - Active operation: before register/DMA/I/O access, runtime resume and increment usage count; after access, mark last busy and put/autosuspend; never touch powered-off registers.
  - Runtime suspend: block new I/O, ensure no active transfers, disable IRQ source if needed, save volatile state, stop DMA, assert reset or select sleep pinctrl, disable clocks/regulators/power resources as allowed.
  - Runtime resume: enable supplies/clocks/pinctrl, wait for hardware, restore state, clear stale IRQ/status, then allow I/O.
  - System suspend: quiesce users/subsystem activity; handle wakeup policy; coordinate with runtime PM state; arm wake IRQs only if `device_may_wakeup()`; avoid firmware requests after userspace is frozen.
  - Remove/unbind: unregister users first, prevent new runtime PM gets, resume if hardware must be accessed for shutdown, disable runtime PM, power device off, release resources.
- Practical examples for later:
  - Learning-only pseudo-driver with one MMIO/platform device showing `DEFINE_RUNTIME_DEV_PM_OPS()`, `pm_runtime_resume_and_get()`, `pm_runtime_put_autosuspend()`, `pm_runtime_enable()`, `pm_runtime_disable()`, and wakeup IRQ policy.
  - Debug-only README showing how to inspect `/sys/devices/.../power/*`, run `echo devices/freezer/platform/processors/core > /sys/power/pm_test`, inspect `/sys/kernel/debug/wakeup_sources`, and trace suspend/resume callbacks.
  - DTS snippet with `wakeup-source`, `power-domains`, pinctrl `default`/`sleep`, clocks, and regulators, marked learning-only unless tied to real bindings.
- Common bugs:
  - Using `pm_runtime_get_sync()` and returning on error without balancing usage count.
  - Touching registers while the device is runtime suspended.
  - Runtime suspending while DMA, IRQ, workqueue, delayed work, tasklet, stream, or file operation can still access hardware.
  - Forgetting `pm_runtime_disable()` and counter balancing in probe failure/remove paths.
  - Letting remove/unbind race with autosuspend work.
  - Assuming system suspend and runtime suspend callbacks can always be the same.
  - Enabling wake IRQ regardless of `/sys/devices/.../power/wakeup` policy.
  - Confusing `IRQF_NO_SUSPEND` with actual wakeup capability.
  - Requesting firmware or blocking on userspace after the freezer stage.
  - Suspending parent clocks/regulators/power domains while child devices are active.
  - Not restoring volatile device state after power loss.
- Debugging notes:
  - Check `/sys/power/state` and `/sys/power/mem_sleep` before assuming a sleep state exists.
  - Inspect `/sys/devices/.../power/runtime_status`, `control`, `runtime_usage`, and `autosuspend_delay_ms` where present.
  - Inspect `/sys/devices/.../power/wakeup` and `/sys/kernel/debug/wakeup_sources` for wakeup-capable devices.
  - Use dmesg around suspend/resume with `no_console_suspend`, `initcall_debug`, dynamic debug, ftrace power events, and `pm_test` when available.
  - Measure real rails/clocks when software state says "suspended" but power does not drop.
  - Reproduce with runtime PM disabled (`power/control=on`) to distinguish runtime PM bugs from bus/probe bugs.
- Production concerns:
  - PM callbacks must be idempotent enough for failed suspend/resume paths.
  - The driver must document which lock protects PM state and which paths may access hardware.
  - All user-visible paths, IRQ handlers, work, timers, runtime PM callbacks, system sleep callbacks, and remove must agree on device active/suspended state.
  - Wakeup policy must respect hardware capability, DT/binding, and userspace policy.
  - Parent/child ordering, device links, power domains, clocks, regulators, resets, and pinctrl states must match the datasheet.
  - Runtime PM should save power without making latency, reliability, or resume behavior unacceptable.
- Interview angles:
  - Runtime PM versus system sleep.
  - How usage counters and active children control runtime suspend.
  - Why `pm_runtime_resume_and_get()` is often safer than `pm_runtime_get_sync()` in checked paths.
  - How autosuspend avoids power-cycling thrash.
  - What `struct dev_pm_ops` callbacks mean and when late/noirq callbacks are needed.
  - How to make a device a wakeup source and why `enable_irq_wake()` differs from `IRQF_NO_SUSPEND`.
  - How to debug a system that immediately wakes after suspend.
  - How to review a driver for PM races with IRQ/work/DMA/userspace.
  - Why system suspend callbacks must account for runtime-suspended devices.
