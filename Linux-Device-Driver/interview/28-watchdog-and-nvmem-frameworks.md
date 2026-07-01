# 28 - Watchdog And NVMEM Frameworks Interview Questions

Strong candidates can explain watchdogs as reliability/reset devices and NVMEM as a provider/consumer framework for small persistent data. They should reason from framework APIs to real board failures: unexpected reboots, `nowayout`, bootloader handoff, missing NVMEM cells, invalid MAC addresses, unsafe sysfs writes, and stale kernel APIs.

## Beginner

### 1. What problem does the watchdog framework solve?

**Level:** Beginner

**Short Answer:**  
It provides a standard way to control watchdog hardware that resets the system if software stops sending keepalive pings.

**Deep Explanation:**  
A watchdog is a timer meant to recover a system that has stopped making progress. Once started, it must be periodically refreshed. If user space hangs, a kernel path stalls, or the watchdog owner stops pinging it, the hardware expires and resets the board. The Linux watchdog framework gives drivers standard registration, callbacks, `/dev/watchdog*`, ioctls, sysfs, timeout policy, and restart/pretimeout integration.

**API / Code Anchor:**  
`struct watchdog_device`, `struct watchdog_ops`, `struct watchdog_info`, `devm_watchdog_register_device()`, `/dev/watchdogN`, `/sys/class/watchdog/watchdogN/`.

**Production or Debugging Angle:**  
If a product randomly reboots after a fixed interval, check whether the watchdog was started by firmware or userspace and whether Linux is feeding it.

**Common Traps:**  
- Thinking a watchdog is only a software timer.
- Forgetting that opening `/dev/watchdog` usually starts it.
- Confusing NIC transmit watchdog timeout with the watchdog subsystem.

**Follow-up Questions:**  
- What happens if nobody pings the watchdog?
- Why do embedded products use watchdogs?
- What is exposed under `/sys/class/watchdog/`?

### 2. What are timeout, keepalive, magic close, and nowayout?

**Level:** Beginner

**Short Answer:**  
Timeout is when the watchdog resets the system; keepalive refreshes the timer; magic close is a controlled userspace stop request; `nowayout` means the watchdog cannot be stopped after it starts.

**Deep Explanation:**  
The watchdog counter expires after the configured timeout unless it is fed. Userspace can feed it by writing to `/dev/watchdogN` or using `WDIOC_KEEPALIVE`. Some drivers support magic close: write the character `V`, then close the file to stop the watchdog. With `nowayout`, stopping is forbidden, so a close does not disarm the watchdog.

**API / Code Anchor:**  
`WDIOC_KEEPALIVE`, `WDIOC_SETTIMEOUT`, `WDIOC_GETTIMEOUT`, `WDIOF_MAGICCLOSE`, `watchdog_set_nowayout()`, `/sys/class/watchdog/watchdogN/nowayout`.

**Production or Debugging Angle:**  
If a test program opens `/dev/watchdog0` and exits, the board may reboot later. That is expected if magic close was not performed or `nowayout` is active.

**Common Traps:**  
- Assuming `close(fd)` always stops the watchdog.
- Writing `V` accidentally as part of a keepalive string.
- Testing expiry on a machine without a recovery path.

**Follow-up Questions:**  
- Why might a product enable `nowayout`?
- How should a userspace watchdog daemon choose its ping interval?
- How can you check whether magic close is supported?

### 3. What problem does the NVMEM framework solve?

**Level:** Beginner

**Short Answer:**  
It standardizes access to small nonvolatile data such as EEPROM, eFuse, OTP, MAC addresses, calibration values, and SoC IDs.

**Deep Explanation:**  
Many boards store small pieces of identity or calibration data outside normal filesystems. Older drivers often exposed custom char devices, raw sysfs files, direct OF properties, or bus-specific APIs. NVMEM separates storage providers from consumers. A provider exposes bytes and cells; consumers request named cells without knowing the physical storage bus or raw offset.

**API / Code Anchor:**  
`struct nvmem_config`, `struct nvmem_device`, `struct nvmem_cell`, `devm_nvmem_register()`, `devm_nvmem_cell_get()`, `nvmem_cell_read()`.

**Production or Debugging Angle:**  
A network driver should usually read a MAC address through a named NVMEM cell instead of hardcoding an EEPROM address in the network driver.

**Common Traps:**  
- Treating direct OF property parsing as equivalent to NVMEM.
- Exposing a custom `/dev/eeprom` ABI for shared board data.
- Forgetting that NVMEM contents may be security-sensitive.

**Follow-up Questions:**  
- What is an NVMEM provider?
- What is an NVMEM consumer?
- What kinds of values are commonly stored in NVMEM?

### 4. What is an NVMEM cell?

**Level:** Beginner

**Short Answer:**  
An NVMEM cell is a named byte or bit range inside an NVMEM provider.

**Deep Explanation:**  
The provider may be a larger EEPROM, OTP, eFuse, or battery-backed RAM region. A cell describes a meaningful subrange, such as six bytes for a MAC address or four bytes for a calibration constant. Device Tree can define cells as child nodes with `reg = <offset size>` and optional bit-level `bits`.

**API / Code Anchor:**  
`struct nvmem_cell`, `struct nvmem_cell_info`, `nvmem-cells`, `nvmem-cell-names`, cell child `reg`, optional `bits`.

**Production or Debugging Angle:**  
If a driver reads the wrong MAC address, inspect the cell's `reg` offset and size, not only the consumer code.

**Common Traps:**  
- Mixing up provider bus address with cell offset.
- Forgetting that `reg` in a cell is offset and size inside the provider.
- Reading a cell without validating its length.

**Follow-up Questions:**  
- Why are DT-defined cells useful?
- What does `nvmem-cell-names` do?
- When would a bit-level cell be useful?

## Mid-level

### 5. Walk through a minimal watchdog driver probe flow.

**Level:** Mid-level

**Short Answer:**  
Probe initializes hardware resources, fills `struct watchdog_device`, attaches ops/info/private data, initializes timeout and policy, then registers the watchdog.

**Deep Explanation:**  
A typical driver embeds `struct watchdog_device` in private data. It maps registers or gets GPIO/regmap resources, fills `.parent`, `.info`, `.ops`, `min_timeout`, `max_timeout`, and default `timeout`, then stores private state with `watchdog_set_drvdata()`. It should call `watchdog_init_timeout()`, apply `watchdog_set_nowayout()`, mark already-running hardware if needed, and register with `devm_watchdog_register_device()`.

**API / Code Anchor:**  
`struct watchdog_device`, `struct watchdog_ops`, `watchdog_set_drvdata()`, `watchdog_init_timeout()`, `watchdog_set_nowayout()`, `watchdog_stop_on_reboot()`, `watchdog_stop_on_unregister()`, `devm_watchdog_register_device()`.

**Production or Debugging Angle:**  
If firmware left the hardware watchdog running, the driver must detect that and mark the device running; otherwise the board may reset before userspace starts feeding it.

**Common Traps:**  
- Forgetting `.start`, which is mandatory.
- Registering before timeout limits and ops are initialized.
- Not handling already-running hardware.
- Forgetting to use managed registration or clean unregister paths.

**Follow-up Questions:**  
- Why is `.start` mandatory?
- What should `.ping` do?
- When should you use `watchdog_stop_on_unregister()`?

### 6. How do watchdog pretimeout and governors work?

**Level:** Mid-level

**Short Answer:**  
Pretimeout is an early event before the final reset. The driver reports it to the watchdog core, and the selected governor decides what policy action to take.

**Deep Explanation:**  
Some watchdog hardware can generate an interrupt or NMI before the real timeout expires. For example, with timeout 60 seconds and pretimeout 10 seconds, the warning event happens around 50 seconds, then reset happens 10 seconds later if the system is not recovered. The IRQ handler should call `watchdog_notify_pretimeout()`. The framework dispatches to a governor such as no-op or panic.

**API / Code Anchor:**  
`.set_pretimeout`, `WDIOF_PRETIMEOUT`, `WDIOC_SETPRETIMEOUT`, `watchdog_notify_pretimeout()`, `/sys/class/watchdog/watchdogN/pretimeout`, `pretimeout_governor`.

**Production or Debugging Angle:**  
Pretimeout is useful for collecting crash evidence or forcing panic before the hardware reset. It only works if the hardware IRQ is wired and the driver advertises/supports it.

**Common Traps:**  
- Thinking pretimeout is the total timeout value.
- Advertising `WDIOF_PRETIMEOUT` without a real event path.
- Forgetting to call `watchdog_notify_pretimeout()` from the interrupt handler.
- Setting pretimeout greater than or equal to timeout.

**Follow-up Questions:**  
- Why might a product choose the panic governor?
- How do you inspect available governors?
- What should happen if hardware lacks a pretimeout IRQ?

### 7. Walk through an NVMEM provider driver.

**Level:** Mid-level

**Short Answer:**  
The provider configures the real storage, fills `struct nvmem_config` with size, access rules, and callbacks, then registers it with the NVMEM core.

**Deep Explanation:**  
The provider owns hardware-specific access, such as I2C EEPROM transactions, MMIO eFuse reads, or RTC RAM access. The NVMEM core does not know that bus protocol. It calls provider `reg_read` and `reg_write` with a generic offset and byte count. The provider also declares size, word size, stride, read-only/root-only policy, and private context.

**API / Code Anchor:**  
`struct nvmem_config`, `nvmem_reg_read_t`, `nvmem_reg_write_t`, `devm_nvmem_register()`, `nvmem_register()`, `nvmem_unregister()`.

**Production or Debugging Angle:**  
Provider callbacks must enforce hardware constraints: alignment, page writes, write delays, OTP irreversibility, read-only policy, and bus errors.

**Common Traps:**  
- Exposing writable sysfs access to OTP/eFuse.
- Ignoring `stride` and `word_size`.
- Treating all EEPROM writes as byte-perfect immediate writes.
- Forgetting that managed registration does not require a manual unregister helper.

**Follow-up Questions:**  
- What does `priv` in `struct nvmem_config` do?
- When should a provider be `read_only`?
- Why is provider callback return behavior version-sensitive?

### 8. How does an NVMEM consumer read a MAC address or calibration cell?

**Level:** Mid-level

**Short Answer:**  
It gets a named cell with `devm_nvmem_cell_get()`, reads it with `nvmem_cell_read()` or a typed helper, validates length and contents, then uses the data.

**Deep Explanation:**  
The consumer should not hardcode the provider's physical location or raw offset. Device Tree connects the consumer to a provider cell using `nvmem-cells` and `nvmem-cell-names`. The driver asks for the cell by name. `nvmem_cell_read()` returns an allocated buffer, so the driver validates and frees it.

**API / Code Anchor:**  
`devm_nvmem_cell_get(dev, "mac-address")`, `nvmem_cell_read()`, `nvmem_cell_read_u32()`, `nvmem_cell_read_variable_le_u32()`, `kfree()`, `is_valid_ether_addr()`.

**Production or Debugging Angle:**  
If the consumer gets `-EPROBE_DEFER`, the provider may not have probed yet. If it gets invalid data, check the cell offset, size, endianness, and whether the storage was actually programmed.

**Common Traps:**  
- Forgetting to free the buffer returned by `nvmem_cell_read()`.
- Accepting a MAC cell without checking `len == ETH_ALEN`.
- Confusing cell name with provider name.
- Falling back to random MAC silently without logging a clear warning.

**Follow-up Questions:**  
- How does Device Tree bind the consumer to the cell?
- What should a driver do if the MAC cell is missing?
- When would you use `nvmem_device_read()` instead of cell APIs?

### 9. Compare NVMEM with a custom EEPROM character driver.

**Level:** Mid-level

**Short Answer:**  
A custom char driver exposes a private ABI for one device; NVMEM exposes standard provider/cell access that many kernel consumers can share.

**Deep Explanation:**  
Older EEPROM examples often implement `/dev/eeprom`, `read`, `write`, `llseek`, and custom ioctls. That teaches file operations, but it does not scale well for board identity data. A network driver, thermal driver, or SoC driver should not each know how to talk to the EEPROM. NVMEM centralizes storage access and lets consumers request named cells.

**API / Code Anchor:**  
Custom: `struct file_operations`, `.read`, `.write`, `.llseek`, `copy_to_user()`, `copy_from_user()`. NVMEM: `struct nvmem_config`, `devm_nvmem_register()`, `nvmem-cells`, `devm_nvmem_cell_get()`.

**Production or Debugging Angle:**  
Use a custom char device only when the product truly needs a custom streaming/control ABI. Use NVMEM for small persistent cells consumed by drivers.

**Common Traps:**  
- Treating a teaching EEPROM char driver as production board-data design.
- Duplicating I2C/SPI EEPROM access in every consumer.
- Exposing raw writes without policy or write protection.

**Follow-up Questions:**  
- Why is NVMEM better for MAC addresses?
- What can userspace still read when NVMEM sysfs is enabled?
- How does NVMEM reduce driver coupling?

## Senior

### 10. A board reboots 45 seconds after boot. How do you debug it?

**Level:** Senior

**Short Answer:**  
Assume a watchdog handoff problem until proven otherwise: check bootloader state, Linux watchdog probe, `WDOG_HW_RUNNING`, userspace daemon timing, timeout, nowayout, and reset reason.

**Deep Explanation:**  
A fixed reboot interval often means hardware was armed before Linux was ready or Linux registered the watchdog but no owner fed it. The bootloader may start the watchdog and pass control to Linux. The driver must detect active hardware and keep it alive. Userspace may open the device too late, close it incorrectly, or fail to ping often enough. Reset status registers or watchdog `bootstatus` may confirm the previous reset reason.

**API / Code Anchor:**  
`watchdog_hw_running()`, `WDOG_HW_RUNNING`, `watchdog_init_timeout()`, `watchdog_set_nowayout()`, `WDIOC_KEEPALIVE`, `/sys/class/watchdog/watchdogN/timeleft`, `/sys/class/watchdog/watchdogN/bootstatus`.

**Production or Debugging Angle:**  
Use serial console, early boot logs, bootloader environment, reset reason registers, and remote power recovery. Temporarily lengthen timeout only as a controlled debug step.

**Common Traps:**  
- Disabling the watchdog in production instead of fixing ownership.
- Assuming userspace will start before timeout.
- Forgetting firmware may have already armed hardware.
- Testing with `nowayout` without a recovery plan.

**Follow-up Questions:**  
- How would systemd watchdog integration affect this?
- Where should watchdog feeding begin in a robust boot chain?
- What evidence distinguishes watchdog reset from kernel panic reboot?

### 11. How would you design watchdog ownership for a production product?

**Level:** Senior

**Short Answer:**  
Define one clear owner and handoff path: bootloader, kernel driver, and userspace daemon must agree on timeout, feeding responsibility, and failure policy.

**Deep Explanation:**  
Watchdog design is a system policy, not just a driver callback. A robust product decides whether the kernel continuously manages the watchdog, a userspace daemon owns it, or systemd supervises services and pings the hardware. Timeout must cover boot, firmware update, filesystem checks, suspend/resume, and worst-case scheduling delays. `nowayout` should match product reliability requirements.

**API / Code Anchor:**  
`watchdog_set_nowayout()`, `watchdog_stop_on_reboot()`, `watchdog_stop_on_unregister()`, `watchdog_init_timeout()`, `/dev/watchdogN`, `WDIOC_SETTIMEOUT`, `WDIOC_KEEPALIVE`.

**Production or Debugging Angle:**  
For field devices, a watchdog that is too short can brick systems during updates; one that is too long may leave failed systems unavailable for too long.

**Common Traps:**  
- Multiple processes trying to own `/dev/watchdogN`.
- Timeout shorter than worst-case boot/update path.
- Stopping watchdog during reboot when hardware should cover reboot hangs.
- Not documenting how `nowayout` is configured.

**Follow-up Questions:**  
- Should a watchdog be enabled during firmware update?
- Who should feed the watchdog during suspend?
- When is kernel-managed feeding better than userspace feeding?

### 12. What security and maintainability issues exist around NVMEM?

**Level:** Senior

**Short Answer:**  
NVMEM may expose identity, calibration, secrets, and one-time-programmable data, so access policy, write support, DT binding, and consumer validation must be deliberate.

**Deep Explanation:**  
NVMEM is small but high-value. A few bytes can identify the device, unlock features, calibrate radios/sensors, or hold security-relevant material. Raw sysfs access may leak information. Writes may be irreversible on OTP/eFuse or wear-limited on EEPROM. Maintainable drivers keep board-specific offsets in DT cells, validate data at consumers, and avoid duplicating provider access in each subsystem.

**API / Code Anchor:**  
`struct nvmem_config.read_only`, `root_only`, `reg_read`, `reg_write`, `nvmem-cells`, `nvmem-cell-names`, `/sys/bus/nvmem/devices/<provider>/nvmem`.

**Production or Debugging Angle:**  
During review, ask what data is exposed, who can read it, whether writes are allowed, and what happens on invalid or erased contents.

**Common Traps:**  
- Enabling writes because the hardware supports them, not because the product needs them.
- Exposing secret or unique provisioning data through world-readable sysfs.
- Hardcoding offsets in consumers instead of using cells.
- Not handling blank or unprogrammed cells.

**Follow-up Questions:**  
- When should `read_only` be set?
- How would you handle an invalid calibration cell?
- Why might userspace NVMEM access differ across kernel configs?

### 13. How do you handle version drift in Watchdog and NVMEM drivers?

**Level:** Senior

**Short Answer:**  
Check the target kernel headers and in-tree drivers, prefer current managed helpers, and avoid copying old helper names or old binding paths blindly.

**Deep Explanation:**  
Framework APIs evolve. Older material may mention helper names that no longer exist, old text DT bindings, or older struct fields. For watchdogs, modern helpers such as `watchdog_init_timeout()` and stop-on-reboot/unregister helpers are important. For NVMEM, current `struct nvmem_config` has more fields than older examples, and current headers may not expose old managed unregister helpers. DT bindings are usually YAML now.

**API / Code Anchor:**  
Watchdog: `watchdog_init_timeout()`, `devm_watchdog_register_device()`, `watchdog_stop_on_reboot()`. NVMEM: `<linux/nvmem-consumer.h>`, `devm_nvmem_register()`, current `struct nvmem_config`, `devm_rtc_nvmem_register()`.

**Production or Debugging Angle:**  
Before writing buildable examples, inspect the target kernel's `include/linux/watchdog.h`, `include/linux/nvmem-provider.h`, `include/linux/nvmem-consumer.h`, and nearby in-tree drivers.

**Common Traps:**  
- Including `<linux/nvmemconsumer.h>` instead of `<linux/nvmem-consumer.h>`.
- Calling a helper not present in the target kernel.
- Copying old `.txt` binding examples into a YAML-validated tree.
- Assuming sysfs cell behavior is identical across kernels.

**Follow-up Questions:**  
- How do you choose an in-tree driver to copy from?
- Why are examples from older books still useful but dangerous?
- What should a code review check first for framework API drift?

### 14. Debug scenario: an Ethernet driver cannot get its NVMEM MAC address.

**Level:** Senior

**Short Answer:**  
Check provider probe, DT phandles, `nvmem-cell-names`, cell offset/size, deferred probe, and data validity before blaming the Ethernet driver.

**Deep Explanation:**  
The Ethernet driver only sees a named cell. The failure may be anywhere in the chain: EEPROM/OTP provider did not probe, NVMEM registration failed, DT points to the wrong phandle, the cell name does not match, the cell size is not six bytes, or the provider returns erased data. A robust driver logs the failure, handles `-EPROBE_DEFER`, validates the MAC, and uses a documented fallback policy.

**API / Code Anchor:**  
`devm_nvmem_cell_get(dev, "mac-address")`, `nvmem_cell_read()`, `is_valid_ether_addr()`, `nvmem-cells`, `nvmem-cell-names`, `/sys/bus/nvmem/devices/`.

**Production or Debugging Angle:**  
Use `dmesg`, DT dump/source, `/sys/bus/nvmem/devices/`, and read-only `hexdump` of the provider where safe. Do not write to production NVMEM during debug.

**Common Traps:**  
- Treating `-EPROBE_DEFER` as a permanent error.
- Accepting all-zero or all-FF MAC addresses.
- Forgetting to free the buffer returned by `nvmem_cell_read()`.
- Hardcoding offsets in the Ethernet driver as a quick fix.

**Follow-up Questions:**  
- What fallback MAC policy is acceptable?
- How would you distinguish missing cell from unprogrammed cell?
- What should logs say for manufacturing debug?

### 15. When should you use a GPIO watchdog binding instead of custom GPIO code?

**Level:** Senior

**Short Answer:**  
Use the GPIO watchdog driver when an external watchdog is kicked through a GPIO and fits the generic binding; custom GPIO loops should be avoided.

**Deep Explanation:**  
External watchdog chips often require a GPIO toggle or level transition. If userspace toggles the GPIO manually, it bypasses the watchdog framework, loses standard `/dev/watchdog*` behavior, and may fail under scheduling or daemon problems. The generic GPIO watchdog driver integrates the GPIO with watchdog core semantics, timeout behavior, and userspace ABI.

**API / Code Anchor:**  
Generic GPIO watchdog binding such as `linux,wdt-gpio`, watchdog core registration, `/dev/watchdogN`, `WDIOC_KEEPALIVE`.

**Production or Debugging Angle:**  
Validate GPIO polarity, pinmux, hardware margin, and algorithm with a scope or logic analyzer. A wrong polarity may look like a software bug but is really board-level signaling.

**Common Traps:**  
- Toggling GPIO from a shell script as a production watchdog.
- Forgetting pinctrl or GPIO active-low semantics.
- Choosing a timeout shorter than Linux scheduling/boot reality.
- Assuming the external chip can be disabled when it cannot.

**Follow-up Questions:**  
- What does the watchdog framework give you that raw GPIO does not?
- How would you test the GPIO waveform safely?
- What board documentation do you need before setting the binding?
