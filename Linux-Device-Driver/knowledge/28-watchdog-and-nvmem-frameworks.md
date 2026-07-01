# 28 - Watchdog And NVMEM Frameworks

## Learning Goal
Understand how Linux uses the watchdog framework to recover failed systems and the NVMEM framework to expose small nonvolatile data safely and consistently. After this topic, you should be able to explain both frameworks, write or review minimal driver flows, debug common bring-up failures, and avoid private one-off ABIs for common hardware classes.

After this topic you should be able to:

- Explain what a watchdog does and why `nowayout` changes the operating model.
- Describe `struct watchdog_device`, `struct watchdog_ops`, and watchdog registration.
- Use timeout, keepalive, pretimeout, restart, and userspace watchdog concepts correctly.
- Explain what NVMEM providers, consumers, and cells are.
- Describe `struct nvmem_config`, provider callbacks, DT cell bindings, and consumer APIs.
- Compare NVMEM with custom EEPROM char devices, direct OF properties, and ethtool EEPROM access.
- Debug `/dev/watchdog*`, `/sys/class/watchdog/`, and `/sys/bus/nvmem/devices/` issues.
- Recognize kernel-version-sensitive APIs and avoid stale helper names.

## Why This Matters In Real Work
Watchdog and NVMEM show up on real embedded products all the time. A watchdog is often the last line of defense when userspace hangs, boot stalls, or a field device becomes unreachable. NVMEM is where boards store values such as MAC addresses, calibration constants, SoC IDs, OTP trim values, and EEPROM configuration.

You meet these frameworks when:

- A board reboots after 30 or 60 seconds because the bootloader left a hardware watchdog running.
- Product requirements say the system must recover from userspace deadlock.
- An external watchdog is driven through a GPIO line.
- A driver needs a MAC address, calibration value, SoC revision, or OTP trim from EEPROM/eFuse.
- A previous driver exposed `/dev/eeprom`, custom ioctls, or raw OF properties and now needs a standard kernel interface.
- You need to expose nonvolatile storage to kernel consumers while controlling userspace visibility and write access.

The shared rule is: **common hardware class, common kernel framework**. Do not bit-bang a watchdog from userspace GPIO if the watchdog framework can own it. Do not make every EEPROM-like chip invent a private char device if NVMEM can describe providers and cells.

## Mental Model
The watchdog framework is about recovery. The NVMEM framework is about identity/configuration data. They solve different problems, but both hide hardware details behind a standard kernel core.

```text
Watchdog:
  hardware or GPIO timer
    -> watchdog driver
    -> watchdog core
    -> /dev/watchdogN, ioctls, sysfs
    -> reset if nobody keeps it alive

NVMEM:
  EEPROM / eFuse / OTP / battery-backed RAM
    -> NVMEM provider driver
    -> NVMEM core
    -> named cells for kernel consumers
    -> optional sysfs binary access
```

Simple comparison:

| Framework | Main Job | Driver Provides | Framework Provides |
| --- | --- | --- | --- |
| Watchdog | Recover the system when software stops making progress | Start/stop/ping/set-timeout/status hardware callbacks | `/dev/watchdog*`, ioctls, sysfs, timeout policy, pretimeout dispatch, restart integration |
| NVMEM | Expose nonvolatile bytes/cells to kernel consumers and sometimes userspace | Read/write callbacks and memory layout | Cell lookup, DT mapping, consumer APIs, sysfs binary attribute, access policy |

**Interview trap:** the word “watchdog” can also appear in network drivers as transmit timeout logic. `ndo_tx_timeout` is not the watchdog subsystem.

## Core Concepts
The watchdog core standardizes how software starts, pings, stops, and queries a watchdog device.

| Concept | Meaning |
| --- | --- |
| Watchdog timeout | Time before reset if no keepalive arrives. |
| Keepalive / ping / kick / feed | Operation that refreshes the watchdog counter. |
| `nowayout` | Policy where a started watchdog cannot be stopped. |
| Magic close | Userspace writes `V` before close to request stopping, if supported. |
| Pretimeout | Early warning event before final watchdog timeout. |
| Pretimeout governor | Policy handler for pretimeout, such as no-op or panic. |
| Restart handler | Watchdog callback used to reboot/reset the machine. |
| GPIO watchdog | External watchdog kicked by toggling or driving a GPIO. |

The NVMEM core standardizes small persistent storage regions and their consumers.

| Concept | Meaning |
| --- | --- |
| NVMEM provider | Driver that owns the real storage device. |
| NVMEM consumer | Driver that reads or writes data supplied by a provider. |
| Cell | Named byte or bit range inside the provider storage. |
| `nvmem-cells` | DT property referencing provider cell phandles. |
| `nvmem-cell-names` | DT names used by consumers to request cells. |
| `reg` in a cell node | Offset and size of the cell inside provider storage. |
| `bits` in a cell node | Optional bit offset and bit count inside a byte range. |

Common NVMEM data:

- MAC addresses.
- Calibration constants.
- SoC revision or serial IDs.
- OTP trim values.
- Part numbers.
- Board configuration flags.
- RTC battery-backed RAM or small EEPROM windows.

## Kernel Mechanism
The watchdog framework keeps generic policy in the core and hardware operations in the driver. A driver typically embeds `struct watchdog_device` inside private state, fills operations, applies policy, and registers with the core.

Watchdog object relationship:

```text
platform / I2C / MFD child / GPIO watchdog device
  -> driver private data
       -> registers, regmap, clocks, GPIO, IRQ
       -> struct watchdog_device
  -> watchdog core
       -> character device and sysfs
       -> ioctl handling
       -> restart handler and pretimeout dispatch
```

The driver usually:

- Initializes hardware access resources.
- Fills `watchdog_device.info`, `ops`, `parent`, timeout limits, and default timeout.
- Stores private data with `watchdog_set_drvdata()`.
- Calls `watchdog_init_timeout()` to combine DT/module/default timeout policy.
- Applies `watchdog_set_nowayout()` when the module/kernel policy requires it.
- Marks already-running hardware with `WDOG_HW_RUNNING` when inherited from firmware.
- Optionally calls `watchdog_stop_on_reboot()` and `watchdog_stop_on_unregister()`.
- Registers with `devm_watchdog_register_device()` or `watchdog_register_device()`.

The NVMEM framework also keeps the common interface in the core. Provider drivers only need to describe the storage and implement generic offset/length read/write callbacks.

NVMEM object relationship:

```text
provider hardware: EEPROM / eFuse / OTP / RTC RAM / MMIO area
  -> provider driver
       -> struct nvmem_config
       -> reg_read / reg_write callbacks
  -> NVMEM core
       -> struct nvmem_device
       -> cells from DT or provider data
       -> consumer API and optional sysfs binary file

consumer driver
  -> devm_nvmem_cell_get(dev, "mac-address")
  -> nvmem_cell_read()
  -> validate length and use data
```

The provider usually:

- Configures the physical storage device.
- Fills `struct nvmem_config`.
- Sets access policy such as `read_only` or `root_only`.
- Provides `.reg_read` and usually `.reg_write` only when writes are safe.
- Registers with `devm_nvmem_register()` or `nvmem_register()`.
- Describes cells in DT where board layout is board-specific.

## Key Structs And APIs
Learn these APIs by ownership and flow. In both frameworks, the core owns the generic ABI; your driver owns hardware-specific behavior.

### Watchdog Driver APIs

| Struct / API | Role |
| --- | --- |
| `struct watchdog_device` | Per-watchdog object registered with the core. |
| `struct watchdog_info` | Userspace-visible identity and option flags. |
| `struct watchdog_ops` | Driver callback table. `.start` is mandatory; most others are optional. |
| `watchdog_init_timeout()` | Initializes timeout from requested/default/firmware values. |
| `watchdog_set_nowayout()` | Applies non-stoppable watchdog policy. |
| `watchdog_stop_on_reboot()` | Requests stop during reboot when hardware supports it. |
| `watchdog_stop_on_unregister()` | Requests stop when unregistering. |
| `watchdog_set_drvdata()` / `watchdog_get_drvdata()` | Attach/retrieve driver private data. |
| `devm_watchdog_register_device()` | Managed registration with the watchdog core. |
| `watchdog_register_device()` / `watchdog_unregister_device()` | Manual registration and cleanup. |
| `watchdog_notify_pretimeout()` | Notify the core that a pretimeout IRQ/event occurred. |
| `watchdog_set_restart_priority()` | Set priority when watchdog is used as restart handler. |

Common `struct watchdog_ops` callbacks:

| Callback | Purpose |
| --- | --- |
| `.start` | Arm/start the hardware watchdog. Mandatory. |
| `.stop` | Stop the watchdog if hardware and policy allow it. |
| `.ping` | Refresh the watchdog counter. If absent, core may use `.start`. |
| `.set_timeout` | Program a new timeout. |
| `.set_pretimeout` | Program an early warning event. |
| `.get_timeleft` | Report time before expiration. |
| `.status` | Return hardware status bits. |
| `.restart` | Reset/restart the machine. |
| `.ioctl` | Extra ioctl handling. Avoid unless the core defaults are insufficient. |

### Watchdog Userspace ABI

| Interface | Meaning |
| --- | --- |
| `/dev/watchdog`, `/dev/watchdogN` | Opening usually starts the watchdog; writing pings it. |
| `WDIOC_KEEPALIVE` | Explicit keepalive ioctl. |
| `WDIOC_GETSUPPORT` | Read `struct watchdog_info` capabilities. |
| `WDIOC_SETTIMEOUT` / `WDIOC_GETTIMEOUT` | Set/query timeout in seconds. |
| `WDIOC_SETPRETIMEOUT` | Set pretimeout where supported. |
| `WDIOC_GETTIMELEFT` | Query remaining time where supported. |
| `WDIOC_GETSTATUS` / `WDIOC_GETBOOTSTATUS` | Query current/boot status. |
| `/sys/class/watchdog/watchdogN/` | Sysfs attributes such as `identity`, `timeout`, `timeleft`, `state`, `nowayout`, `bootstatus`, `pretimeout`. |

### NVMEM Provider APIs

| Struct / API | Role |
| --- | --- |
| `struct nvmem_config` | Provider registration configuration. |
| `struct nvmem_device` | Core-created provider device. |
| `struct nvmem_cell_info` | Provider-side cell description. |
| `nvmem_reg_read_t` | Provider callback for offset/length reads. |
| `nvmem_reg_write_t` | Provider callback for offset/length writes. |
| `devm_nvmem_register()` | Managed provider registration. |
| `nvmem_register()` / `nvmem_unregister()` | Manual provider registration and cleanup. |
| `nvmem_add_cell_table()` | Add cells from board data when firmware cannot describe them. |
| `nvmem_add_one_cell()` | Add a cell to an existing provider. |
| `devm_rtc_nvmem_register()` | Current managed helper for RTC-backed NVMEM on kernels that support it. |

Important `struct nvmem_config` fields:

| Field | Meaning |
| --- | --- |
| `dev` | Parent device. |
| `name`, `id` | Provider name in the NVMEM bus/device namespace. |
| `cells`, `ncells` | Optional static provider-side cells. |
| `read_only` | Prevent writes through the framework. |
| `root_only` | Restrict userspace access. |
| `reg_read`, `reg_write` | Hardware access callbacks. |
| `size` | Total provider size in bytes. |
| `word_size`, `stride` | Access granularity and alignment rules. |
| `priv` | Driver context passed to callbacks. |

### NVMEM Consumer APIs

| API | Role |
| --- | --- |
| `devm_nvmem_cell_get(dev, name)` | Get a named cell managed by device lifetime. |
| `nvmem_cell_get()` / `nvmem_cell_put()` | Manual get/put cell lifetime. |
| `nvmem_cell_read()` | Read an entire cell and return allocated data. |
| `nvmem_cell_write()` | Write a cell where supported and allowed. |
| `nvmem_cell_read_u32()` | Read a fixed-size cell as `u32`. |
| `nvmem_cell_read_variable_le_u32()` | Read variable-sized little-endian integer cell. |
| `nvmem_device_read()` | Direct device offset read; use when cell API is not suitable. |

### NVMEM Device Tree And ABI

Provider with cells:

```dts
ocotp: efuse@21bc000 {
    compatible = "vendor,soc-efuse";
    reg = <0x021bc000 0x4000>;
    #address-cells = <1>;
    #size-cells = <1>;

    macaddr: mac-address@20 {
        reg = <0x20 0x6>;
    };

    temp_calib: calibration@38 {
        reg = <0x38 0x4>;
    };
};
```

Consumer referencing cells:

```dts
ethernet0 {
    nvmem-cells = <&macaddr>;
    nvmem-cell-names = "mac-address";
};
```

Userspace may see a provider binary attribute such as:

```text
/sys/bus/nvmem/devices/<provider>/nvmem
```

**Production warning:** raw NVMEM sysfs access can expose board identity, calibration, secrets, or one-time-programmable regions. Treat permissions and write support as product/security decisions.

## Lifecycle / Data Flow
The watchdog lifecycle is centered on arming hardware and proving software is still alive before the timeout expires.

```text
probe
  -> allocate private data
  -> initialize hardware resources
  -> fill watchdog_device + ops + info
  -> initialize timeout and policy
  -> register watchdog

userspace open / kernel start
  -> core calls .start
  -> hardware counter starts

keepalive
  -> write(/dev/watchdog) or WDIOC_KEEPALIVE
  -> core calls .ping or .start
  -> hardware counter reloads

timeout path
  -> no keepalive arrives
  -> optional pretimeout IRQ
  -> final hardware reset/reboot

remove/reboot
  -> unregister or reboot notifier
  -> stop only if hardware and policy allow
```

The NVMEM lifecycle is centered on registering a storage provider and resolving named cells for consumers.

```text
provider probe
  -> configure storage hardware
  -> fill nvmem_config
  -> register provider
  -> core creates nvmem_device and sysfs entry
  -> core parses DT cells or provider cells

consumer probe
  -> devm_nvmem_cell_get(dev, "calib")
  -> nvmem_cell_read()
  -> validate length/content
  -> use value in driver setup

userspace debug
  -> inspect /sys/bus/nvmem/devices/
  -> read raw nvmem binary carefully
```

## Minimal Practical Example
These snippets are **learning-only**. They show flow and API relationships, not a complete buildable production driver. Real watchdog code must be tested on controlled hardware. Real NVMEM writes can permanently modify EEPROM/OTP/eFuse.

### Minimal Watchdog Driver Skeleton

```c
struct demo_wdt {
    void __iomem *base;
    struct watchdog_device wdd;
};

static int demo_wdt_start(struct watchdog_device *wdd)
{
    struct demo_wdt *d = watchdog_get_drvdata(wdd);

    /* Program hardware enable bit and load timeout counter. */
    writel(wdd->timeout, d->base + DEMO_WDT_TIMEOUT);
    writel(DEMO_WDT_ENABLE, d->base + DEMO_WDT_CTRL);
    return 0;
}

static int demo_wdt_ping(struct watchdog_device *wdd)
{
    struct demo_wdt *d = watchdog_get_drvdata(wdd);

    writel(DEMO_WDT_RELOAD_KEY, d->base + DEMO_WDT_RELOAD);
    return 0;
}

static int demo_wdt_set_timeout(struct watchdog_device *wdd, unsigned int timeout)
{
    struct demo_wdt *d = watchdog_get_drvdata(wdd);

    wdd->timeout = timeout;
    writel(timeout, d->base + DEMO_WDT_TIMEOUT);
    return demo_wdt_ping(wdd);
}

static const struct watchdog_ops demo_wdt_ops = {
    .owner = THIS_MODULE,
    .start = demo_wdt_start,
    .ping = demo_wdt_ping,
    .set_timeout = demo_wdt_set_timeout,
};

static const struct watchdog_info demo_wdt_info = {
    .identity = "demo-watchdog",
    .options = WDIOF_KEEPALIVEPING | WDIOF_SETTIMEOUT | WDIOF_MAGICCLOSE,
};

static int demo_wdt_probe(struct platform_device *pdev)
{
    struct demo_wdt *d;
    int ret;

    d = devm_kzalloc(&pdev->dev, sizeof(*d), GFP_KERNEL);
    if (!d)
        return -ENOMEM;

    d->base = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(d->base))
        return PTR_ERR(d->base);

    d->wdd.parent = &pdev->dev;
    d->wdd.info = &demo_wdt_info;
    d->wdd.ops = &demo_wdt_ops;
    d->wdd.min_timeout = 1;
    d->wdd.max_timeout = 120;
    d->wdd.timeout = 30;

    watchdog_set_drvdata(&d->wdd, d);
    watchdog_init_timeout(&d->wdd, 0, &pdev->dev);
    watchdog_set_nowayout(&d->wdd, nowayout);
    watchdog_stop_on_reboot(&d->wdd);
    watchdog_stop_on_unregister(&d->wdd);

    ret = devm_watchdog_register_device(&pdev->dev, &d->wdd);
    if (ret)
        return ret;

    return 0;
}
```

Important lines:

- `.start` is mandatory because the core needs a way to arm hardware.
- `.ping` keeps the hardware alive without restarting the whole driver.
- `watchdog_init_timeout()` handles configured timeout policy.
- `watchdog_set_nowayout()` must match product/module policy.
- `devm_watchdog_register_device()` gives the standard character device and sysfs ABI.
- A real driver must program the hardware timeout register in `.set_timeout`; updating `wdd->timeout` alone only updates the framework's cached value.

### Minimal NVMEM Consumer

```c
static int demo_read_mac(struct device *dev, u8 mac[ETH_ALEN])
{
    struct nvmem_cell *cell;
    void *buf;
    size_t len;

    cell = devm_nvmem_cell_get(dev, "mac-address");
    if (IS_ERR(cell))
        return PTR_ERR(cell);

    buf = nvmem_cell_read(cell, &len);
    if (IS_ERR(buf))
        return PTR_ERR(buf);

    if (len != ETH_ALEN || !is_valid_ether_addr(buf)) {
        kfree(buf);
        return -EINVAL;
    }

    ether_addr_copy(mac, buf);
    kfree(buf);
    return 0;
}
```

Important lines:

- The consumer asks for `"mac-address"` by name, not by hardcoded provider offset.
- `nvmem_cell_read()` returns an allocated buffer; the caller must free it.
- Length and content must be validated before use.
- `devm_nvmem_cell_get()` ties the cell handle to device lifetime, but it does not free the read buffer.

## Common Bugs And Debugging
Start debugging from the visible symptom, then walk backward through framework objects, DT, registration, and hardware behavior.

### Watchdog Bugs

| Symptom | Likely Causes | What To Check |
| --- | --- | --- |
| Board reboots shortly after boot | Bootloader left watchdog running; Linux driver did not claim/ping it; daemon starts too late | `dmesg`, bootloader config, driver probe, `WDOG_HW_RUNNING`, `/sys/class/watchdog/watchdogN/timeleft` |
| Closing `/dev/watchdog` still reboots | `nowayout` active; magic close unsupported; did not write `V` | `/sys/class/watchdog/watchdogN/nowayout`, `WDIOF_MAGICCLOSE`, userspace close path |
| `WDIOC_SETTIMEOUT` fails | Timeout outside min/max; driver lacks `.set_timeout`; hardware cannot represent value | `min_timeout`, `max_timeout`, `WDIOF_SETTIMEOUT`, dmesg |
| Pretimeout never fires | No IRQ wired; `WDIOF_PRETIMEOUT` missing; no `.set_pretimeout`; IRQ handler not calling `watchdog_notify_pretimeout()` | DT interrupts, `/proc/interrupts`, sysfs pretimeout, governor files |
| External GPIO watchdog unreliable | Userspace toggling GPIO; wrong GPIO polarity/algorithm; pinctrl issue | Use GPIO watchdog driver, DT binding, scope/logic analyzer |
| Reboot path hangs | Restart handler priority wrong; `.restart` incomplete; hardware reset line not effective | restart priority, reset registers, serial console logs |

Useful commands:

```bash
ls -l /dev/watchdog*
ls /sys/class/watchdog/
cat /sys/class/watchdog/watchdog0/identity
cat /sys/class/watchdog/watchdog0/timeout
cat /sys/class/watchdog/watchdog0/timeleft
cat /sys/class/watchdog/watchdog0/nowayout
cat /sys/class/watchdog/watchdog0/bootstatus
cat /sys/class/watchdog/watchdog0/state
dmesg | grep -i watchdog
```

**Do not casually test watchdog expiration on your workstation.** Expiry intentionally resets the system. Use a target board, serial console, remote power control, and a recovery plan.

### NVMEM Bugs

| Symptom | Likely Causes | What To Check |
| --- | --- | --- |
| Consumer gets `-ENOENT` or `-EPROBE_DEFER` | Missing provider; bad `nvmem-cells`; provider probes later | DT phandles, provider probe logs, deferred probe list |
| MAC address invalid | Wrong cell offset/size; endian/format mismatch; unprogrammed EEPROM/OTP | Cell `reg`, `hexdump`, length check, `is_valid_ether_addr()` |
| Sysfs `nvmem` missing | Provider not registered; `CONFIG_NVMEM` disabled; probe failed | `/sys/bus/nvmem/devices/`, dmesg, Kconfig |
| Write fails | Provider read-only; sysfs permissions; no `reg_write`; hardware write-protected | `read_only`, WP GPIO, provider callback, permissions |
| Data changes fail silently or corrupt | EEPROM page/write-cycle rules ignored; OTP one-time programming misunderstood | Provider implementation, write delays, hardware datasheet |
| Sensitive data visible | Raw sysfs access too permissive; secrets stored in exposed cells | `root_only`, permissions, product security policy |

Useful commands:

```bash
ls /sys/bus/nvmem/devices/
ls -l /sys/bus/nvmem/devices/*/
hexdump -C /sys/bus/nvmem/devices/<provider>/nvmem | head
dmesg | grep -i nvmem
dmesg | grep -i -E 'eeprom|efuse|otp|ocotp|qfprom'
```

NVMEM debug checklist:

- Is the provider node present and matched by a driver?
- Does the provider register successfully?
- Do cell child nodes use the correct `reg = <offset size>`?
- Does the consumer use the same name as `nvmem-cell-names`?
- Does the cell length match what the driver expects?
- Is the data endian/formatted as expected?
- Should userspace be allowed to read or write this region?

## Production Checklist
Use this before sending a watchdog or NVMEM driver for review.

### Watchdog Checklist

- Confirm who owns feeding policy: kernel, userspace daemon, systemd, or hardware-already-running handoff.
- Set realistic default timeout, min/max timeout, and boot-time behavior.
- Use `watchdog_init_timeout()` so firmware/module/default policy is handled cleanly.
- Apply `watchdog_set_nowayout()` according to product policy.
- Detect and mark hardware already running with `WDOG_HW_RUNNING` when applicable.
- Implement `.stop` only if hardware can truly stop and policy allows it.
- Implement `.ping`; if omitted, confirm `.start` is safe as a refresh path.
- Avoid custom `.ioctl` unless the core defaults cannot handle the device.
- Test `/dev/watchdogN`, keepalive, timeout change, magic close, and sysfs attributes.
- Test panic/reboot/suspend/update scenarios that may delay keepalive.
- For external watchdogs, prefer the GPIO watchdog driver over userspace GPIO poking.

### NVMEM Checklist

- Prefer DT-defined cells for board-specific offsets.
- Use `devm_nvmem_register()` for providers unless manual lifetime is required.
- Set `read_only`, `root_only`, and write support deliberately.
- Validate provider `size`, `stride`, and `word_size`.
- Handle bus errors and partial/unaligned access according to hardware constraints.
- Do not enable writes for OTP/eFuse unless the product flow explicitly needs programming.
- Validate consumer cell length, endianness, and content.
- Free buffers returned by `nvmem_cell_read()`.
- Treat MAC/calibration/ID values as data that may be missing, erased, or invalid.
- Review whether raw sysfs exposure leaks sensitive product data.

## Interview Readiness
You are ready for interviews when you can reason from symptom to framework object to hardware behavior.

Be able to explain:

- What happens when userspace opens, writes to, and closes `/dev/watchdog`.
- Why `nowayout` exists and how it changes testing and production policy.
- Why a bootloader-running watchdog needs special handoff handling.
- How pretimeout differs from final timeout.
- Why a GPIO watchdog should be represented as a watchdog, not a random GPIO loop.
- What NVMEM provider/consumer/cell means.
- How a MAC address or calibration value is described in DT and read by a driver.
- Why a custom EEPROM char device is usually worse than NVMEM for shared board data.
- How to debug missing NVMEM cells and invalid cell contents.

Practice with [28-watchdog-and-nvmem-frameworks.md](/home/tungnhs/TungNHS/Knowlege-Interview/Linux-Device-Driver/interview/28-watchdog-and-nvmem-frameworks.md).

## Kernel Version Notes
The internal book material is based around Linux 4.19-era APIs, while local validation used Linux 6.8 headers. Use current kernel headers and in-tree drivers before writing buildable code.

Practical caveats:

- Use `<linux/nvmem-consumer.h>` for NVMEM consumers, not the typo-like old spelling without the dash.
- Prefer `devm_nvmem_register()` for managed NVMEM providers. Current local headers do not expose `devm_nvmem_unregister()`.
- Current RTC-backed NVMEM uses `devm_rtc_nvmem_register()` on kernels that support it.
- Modern `struct nvmem_config` has fields beyond the older lesson, including layout/keepout/write-protect-related fields.
- NVMEM sysfs behavior and per-cell exposure can vary by kernel configuration and binding support; do not promise a stable per-cell userspace ABI without checking the target kernel.
- Watchdog helpers such as `watchdog_init_timeout()`, `watchdog_stop_on_reboot()`, and `watchdog_stop_on_unregister()` are important in modern drivers even if older material emphasizes the raw registration path.
