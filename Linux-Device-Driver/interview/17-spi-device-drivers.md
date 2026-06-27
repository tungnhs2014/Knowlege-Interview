# 17 - SPI Device Drivers Interview Questions

Strong candidates can explain the SPI controller/device split, reason about full-duplex transfers, build a `spi_message`, debug Device Tree binding failures, and avoid stale `spidev` and API assumptions.

## Beginner Questions

### 1. What is a SPI device driver?
**Short Answer:** A SPI device driver is a Linux driver for one peripheral chip connected to an existing SPI controller, such as a flash, ADC, display, CAN controller, RTC, or sensor.

**Deep Explanation:** The SPI controller driver owns the hardware block that drives SCK, MOSI, MISO, and chip select. The SPI device driver owns the chip-specific protocol: command bytes, registers, delays, status polling, IRQ behavior, power sequencing, and subsystem registration. Linux represents the peripheral as `struct spi_device` and binds it to a matching `struct spi_driver`.

**API / Code Anchor:**
```c
static struct spi_driver my_driver = {
    .driver = {
        .name = "mychip",
        .of_match_table = my_of_match,
    },
    .probe = my_probe,
    .remove = my_remove,
    .id_table = my_id,
};
module_spi_driver(my_driver);
```

**Production or Debugging Angle:** If the chip belongs to a kernel subsystem, write a real SPI device driver instead of leaving product code to poke raw SPI from userspace.

**Common Traps:** Confusing a SPI device driver with a SPI controller driver. A normal peripheral driver does not touch controller registers directly.

**Follow-up Questions:**
- What does the SPI controller driver own?
- What does the SPI device driver own?
- Name three subsystems that commonly use SPI device drivers.

### 2. What is the difference between `spi_controller`, `spi_device`, and `spi_driver`?
**Short Answer:** `spi_controller` represents the bus controller, `spi_device` represents one peripheral on that controller, and `spi_driver` is the driver code that binds to matching peripherals.

**Deep Explanation:** The controller is usually created by a SoC or board-level controller driver. The `spi_device` is created from Device Tree, ACPI, board data, or a parent driver and contains the chip-select index, mode, maximum speed, IRQ, and embedded `struct device`. The `spi_driver` registers callbacks and match tables so the SPI core can bind it to devices.

**API / Code Anchor:**
```c
static int my_probe(struct spi_device *spi)
{
    dev_info(&spi->dev, "CS=%u speed=%u mode=0x%x\n",
             spi->chip_select, spi->max_speed_hz, spi->mode);
    return 0;
}
```

**Production or Debugging Angle:** When probe does not run, identify whether the controller exists, whether the `spi_device` exists, and whether the driver matched. They are separate failure points.

**Common Traps:** Saying that a SPI device is addressed like I2C. SPI selects devices using chip select, not a bus address.

**Follow-up Questions:**
- Where would you look in sysfs for SPI devices?
- Can one SPI driver bind to multiple SPI devices?
- Why do old docs sometimes say `spi_master`?

### 3. Why is SPI called full-duplex?
**Short Answer:** On every clock edge, SPI shifts one bit out on MOSI and one bit in on MISO at the same time.

**Deep Explanation:** The controller generates SCK. For each bit time, the selected peripheral samples one outgoing bit and provides one incoming bit. That means receiving data requires clocking something out, even if the transmitted bytes are dummy bytes. It also means write-only transfers may receive garbage or ignored data.

**API / Code Anchor:**
```c
struct spi_transfer xfer = {
    .tx_buf = tx,
    .rx_buf = rx,
    .len = 4,
};
```

**Production or Debugging Angle:** For a register read, the driver often sends a command/address phase and then clocks dummy bytes while receiving data. A logic analyzer should show clocks during the "read" phase.

**Common Traps:** Thinking `rx_buf` can fill without clocks, or assuming a read operation means no bytes are transmitted.

**Follow-up Questions:**
- What happens if `tx_buf` is `NULL`?
- What happens if `rx_buf` is `NULL`?
- Why do some devices require dummy bytes?

### 4. What do CPOL and CPHA mean?
**Short Answer:** CPOL controls the idle clock level, and CPHA controls which clock edge samples data. Together they form `SPI_MODE_0` through `SPI_MODE_3`.

**Deep Explanation:** CPOL 0 means the clock idles low; CPOL 1 means it idles high. CPHA 0 samples on the first edge; CPHA 1 samples on the second edge. The peripheral datasheet defines the required mode, and the driver or Device Tree must match it.

**API / Code Anchor:**
```c
spi->mode = SPI_MODE_3; /* CPOL=1, CPHA=1 */
ret = spi_setup(spi);
```

**Production or Debugging Angle:** Wrong mode is one of the most common bring-up bugs. The device may return shifted, unstable, or all-zero/all-ones data.

**Common Traps:** Trying random modes without checking the datasheet, or forgetting `spi_setup()` after changing `spi->mode`.

**Follow-up Questions:**
- Which mode is CPOL=0, CPHA=0?
- Which mode is CPOL=1, CPHA=1?
- How can Device Tree express CPOL and CPHA?

## Mid-Level Questions

### 5. What should a SPI probe function do?
**Short Answer:** Probe should configure bus settings, call `spi_setup()`, allocate private state, save driver data, verify/configure the chip, and register with the correct kernel subsystem.

**Deep Explanation:** Probe runs after the SPI core has matched a `spi_device` to a `spi_driver`. It should not scan chip selects. It should make the device usable or fail cleanly. Typical work includes parsing firmware properties, setting mode/speed/word size, initializing locks or regmap, reading a chip ID, requesting IRQs, enabling runtime PM if needed, and registering an IIO/input/RTC/MTD/CAN/etc. object.

**API / Code Anchor:**
```c
static int my_probe(struct spi_device *spi)
{
    struct mydev *priv;
    int ret;

    spi->mode = SPI_MODE_0;
    spi->bits_per_word = 8;
    ret = spi_setup(spi);
    if (ret)
        return ret;

    priv = devm_kzalloc(&spi->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->spi = spi;
    spi_set_drvdata(spi, priv);

    return my_register_subsystem(priv);
}
```

**Production or Debugging Angle:** Good probe logs separate setup failure, chip ID failure, missing resources, IRQ failure, and subsystem registration failure.

**Common Traps:** Returning success before registering the subsystem, or ignoring failed `spi_setup()` and then debugging phantom transfer errors.

**Follow-up Questions:**
- Why call `spi_setup()`?
- What goes into private data?
- What should remove undo?

### 6. How does Device Tree describe a SPI peripheral?
**Short Answer:** A SPI peripheral is a child node under the SPI controller. Its `reg` property is the chip-select index, and `spi-max-frequency` limits the maximum clock speed.

**Deep Explanation:** SPI devices are not CPU-addressable MMIO children, so the parent typically uses `#address-cells = <1>` and `#size-cells = <0>`. The child `compatible` string selects the driver. Mode flags such as `spi-cpol`, `spi-cpha`, and `spi-cs-high` describe bus behavior. Device-specific properties describe IRQs, regulators, reset GPIOs, clocks, and configuration.

**API / Code Anchor:**
```dts
&spi1 {
    #address-cells = <1>;
    #size-cells = <0>;
    status = "okay";

    sensor@0 {
        compatible = "vendor,my-spi-sensor";
        reg = <0>;
        spi-max-frequency = <10000000>;
        spi-cpol;
        spi-cpha;
    };
};
```

**Production or Debugging Angle:** If probe never runs, check the controller status, child placement, `compatible`, unit address, `reg`, and module aliases.

**Common Traps:** Treating SPI child `reg` as an MMIO base address. It is the chip-select number.

**Follow-up Questions:**
- Why is `#size-cells = <0>`?
- What does `spi-max-frequency` protect?
- How do you express active-high chip select?

### 7. What is the difference between `spi_transfer` and `spi_message`?
**Short Answer:** A `spi_transfer` is one buffer pair and transfer setting; a `spi_message` is an ordered list of transfers submitted to one SPI device.

**Deep Explanation:** Many devices need multiple phases: command, address, dummy clocks, data read/write. Each phase can be a transfer. The message groups those phases so the SPI core and controller execute them in order, with chip-select behavior controlled by transfer fields and controller rules.

**API / Code Anchor:**
```c
struct spi_transfer xfers[2] = {
    { .tx_buf = header, .len = sizeof(header) },
    { .rx_buf = data,   .len = len },
};
struct spi_message msg;

spi_message_init(&msg);
spi_message_add_tail(&xfers[0], &msg);
spi_message_add_tail(&xfers[1], &msg);
ret = spi_sync(spi, &msg);
```

**Production or Debugging Angle:** When a helper cannot preserve the exact command/address/data shape, switch to an explicit `spi_message`.

**Common Traps:** Assuming separate helper calls preserve chip select across operations. They may not.

**Follow-up Questions:**
- What does `cs_change` influence?
- What does `actual_length` tell you?
- When would you override `speed_hz` per transfer?

### 8. When should you use `spi_sync()`, `spi_async()`, or helper APIs?
**Short Answer:** Use helpers for simple small operations, `spi_sync()` for normal process-context message transfers, and `spi_async()` when the operation must be queued without waiting.

**Deep Explanation:** Helpers such as `spi_write()`, `spi_read()`, and `spi_write_then_read()` are convenient wrappers for common cases. `spi_sync()` is easier to reason about because it returns after completion, but it may sleep. `spi_async()` queues a message and completes later through a callback/context, so lifetime rules are stricter.

**API / Code Anchor:**
```c
ret = spi_write_then_read(spi, &cmd, 1, id, 3);

/* More complex operation */
ret = spi_sync(spi, &msg);
```

**Production or Debugging Angle:** If the code runs from a hard IRQ handler, do not call `spi_sync()`. Use a threaded IRQ handler or workqueue, or design a carefully managed async path.

**Common Traps:** Using `spi_async()` with stack-allocated transfers or buffers that disappear before completion.

**Follow-up Questions:**
- Why can `spi_sync()` sleep?
- What must remain valid for `spi_async()`?
- What helper is common for command-then-read?

### 9. When is regmap better than raw SPI transfers?
**Short Answer:** Regmap is better when the device is mostly register-based and the protocol can be described with register/value widths, access policy, caching, and read/write flags.

**Deep Explanation:** Many SPI chips are simple register maps. Raw SPI code repeats command formatting, locking, error handling, and read-modify-write operations. Regmap centralizes those patterns and lets higher-level code call `regmap_read()`, `regmap_write()`, and `regmap_update_bits()` without caring whether the chip is SPI, I2C, or MMIO.

**API / Code Anchor:**
```c
static const struct regmap_config cfg = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = 0xff,
};

priv->map = devm_regmap_init_spi(spi, &cfg);
if (IS_ERR(priv->map))
    return PTR_ERR(priv->map);
```

**Production or Debugging Angle:** Regmap helps with cache, volatile registers, access validation, debugfs visibility, and shared SPI/I2C chip variants.

**Common Traps:** Forcing regmap onto a streaming protocol or a protocol with unusual framing that does not fit register semantics.

**Follow-up Questions:**
- What does `reg_bits` mean?
- What is a volatile register?
- Why is `devm_regmap_init_spi()` preferred in many drivers?

## Debugging Scenarios

### 10. Probe never runs for your SPI driver. How do you debug it?
**Short Answer:** Check device creation first, then driver matching and module autoload.

**Deep Explanation:** Probe only runs after a `spi_device` exists and matches a registered `spi_driver`. Device creation usually comes from Device Tree. Matching depends on `compatible` strings or SPI ID names and module alias data.

**API / Code Anchor:**
```bash
dmesg | grep -i spi
ls /sys/bus/spi/devices
ls /sys/bus/spi/drivers
modinfo my_driver.ko
```

**Production or Debugging Angle:** Confirm the SPI controller node is enabled, the child node is under the right controller, `reg` matches a valid chip select, and `MODULE_DEVICE_TABLE(of, ...)` is present.

**Common Traps:** Looking at transfer code before proving that the device exists and the driver bound.

**Follow-up Questions:**
- What sysfs directory lists SPI devices?
- What does `MODULE_DEVICE_TABLE()` affect?
- What does `reg = <0>` mean for a SPI child?

### 11. Probe runs, but reading the chip ID returns `0xff` or `0x00`. What do you check?
**Short Answer:** Check SPI mode, chip select, speed, pinctrl, power/reset, and the exact command sequence.

**Deep Explanation:** All-ones or all-zero data often means the peripheral is not driving MISO, the wrong chip is selected, or the timing is wrong. A wrong CPOL/CPHA can shift data. A missing reset delay or regulator can make the chip unresponsive. A too-fast clock can fail on first bring-up.

**API / Code Anchor:**
```c
spi->mode = SPI_MODE_0;
spi->max_speed_hz = 1000000; /* start slow */
ret = spi_setup(spi);
```

**Production or Debugging Angle:** A logic analyzer is extremely useful: verify CS polarity/timing, SCK frequency, MOSI command bytes, and whether MISO changes.

**Common Traps:** Randomly changing modes without checking the datasheet, or forgetting that a read may require dummy clocks after command/address bytes.

**Follow-up Questions:**
- What does `spi-cs-high` do?
- Why start at a lower clock speed?
- What does wrong CPHA look like?

### 12. A SPI interrupt handler needs to read device status. Can it call `spi_sync()` directly?
**Short Answer:** Not from a hard IRQ handler. Use a threaded IRQ handler, workqueue, or carefully managed async design.

**Deep Explanation:** `spi_sync()` may sleep while waiting for the controller queue and hardware transfer to complete. Hard IRQ context cannot sleep. Many SPI devices signal an interrupt, then the driver reads status over SPI from a threaded handler or scheduled work.

**API / Code Anchor:**
```c
ret = devm_request_threaded_irq(&spi->dev, spi->irq,
                                NULL, my_irq_thread,
                                IRQF_ONESHOT,
                                dev_name(&spi->dev), priv);

static irqreturn_t my_irq_thread(int irq, void *data)
{
    struct mydev *priv = data;

    my_read_status_over_spi(priv); /* may use spi_sync() */
    return IRQ_HANDLED;
}
```

**Production or Debugging Angle:** If the system warns about sleeping in atomic context, inspect interrupt handlers, spinlocked regions, and timer callbacks that touch SPI.

**Common Traps:** Thinking `spi_async()` removes all context concerns. It avoids waiting, but buffer lifetime and callback behavior still matter.

**Follow-up Questions:**
- What does `IRQF_ONESHOT` do for a threaded IRQ?
- What other contexts cannot sleep?
- What must remain valid for async SPI?

## Senior Questions

### 13. How do you design a safe SPI transfer path for concurrent subsystem callbacks?
**Short Answer:** Serialize shared device state and shared buffers with a mutex, keep transfer objects alive for their completion lifetime, and define clear PM and remove ordering.

**Deep Explanation:** Subsystem callbacks, IRQ threads, workqueues, and sysfs/debug paths may all access the same device. SPI transfers can sleep, so a mutex is usually appropriate for process-context state. The driver must prevent remove from freeing state while work or async messages are still in flight. Runtime PM must keep the device powered during register access.

**API / Code Anchor:**
```c
mutex_lock(&priv->lock);
ret = spi_sync(priv->spi, &msg);
mutex_unlock(&priv->lock);
```

**Production or Debugging Angle:** Random corruption often comes from shared TX/RX buffers, overlapping read-modify-write sequences, or remove racing with work. Add cancellation and completion waits before freeing resources.

**Common Traps:** Using a spinlock around `spi_sync()`. A spinlock cannot protect a sleepable transfer path.

**Follow-up Questions:**
- Why is a mutex better than a spinlock here?
- How do you stop work during remove?
- How should runtime PM wrap register access?

### 14. How would you decide between a kernel SPI driver and `spidev`?
**Short Answer:** Use a kernel driver when the device needs a kernel subsystem, IRQs, power management, safe sharing, or stable ABI. Use `spidev` for prototyping, lab tools, or simple userspace-controlled links.

**Deep Explanation:** `spidev` exposes raw SPI transactions to userspace. It does not provide a device-specific kernel abstraction, subsystem integration, interrupt handling, or robust power sequencing. It can be useful during bring-up, but production systems usually need a proper driver when the device is part of the platform.

**API / Code Anchor:**
```c
/* Userspace full-duplex transfer */
ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
```

**Production or Debugging Angle:** Permanent raw SPI access can become an undocumented ABI and a security or coordination problem. A subsystem driver gives userspace a standard interface.

**Common Traps:** Copying old examples with `compatible = "spidev"` into upstream-style Device Tree. Current upstream spidev requires a real supported device name or explicit driver override.

**Follow-up Questions:**
- Why are `read()` and `write()` on spidev limited?
- What does `SPI_IOC_MESSAGE(N)` provide?
- When is userspace SPI acceptable?

### 15. What kernel-version issues should you watch for in SPI code reviews?
**Short Answer:** Watch for old `spi_master` terminology, stale `struct spi_transfer` fields, old binding file names, and stale `compatible = "spidev"` guidance.

**Deep Explanation:** The conceptual SPI model is stable, but kernel names and fields evolve. Modern docs use `spi_controller` for controller-side APIs. Older examples show `delay_usecs`; current kernels have richer delay structures. Modern bindings are YAML-based. Upstream spidev no longer accepts a raw `"spidev"` compatible string.

**API / Code Anchor:**
```c
/* Validate against target kernel headers, not copied old structs. */
#include <linux/spi/spi.h>
```

**Production or Debugging Angle:** Code copied from older books may compile poorly or, worse, encode stale DT practices. Always validate against the target kernel tree and bindings.

**Common Traps:** Treating tutorial struct definitions as authoritative instead of the target kernel headers.

**Follow-up Questions:**
- What replaced much of the `spi_master` terminology?
- Why should bindings be YAML-validated?
- How should a lab bind spidev if `"spidev"` compatible is rejected?

### 16. How do you handle a SPI flash-like device with write-enable, page limits, and busy polling?
**Short Answer:** Serialize operations, issue write-enable before writes/erases, respect page and sector boundaries, poll status with timeouts, and use the right framework if one exists.

**Deep Explanation:** Flash-like devices are stateful. A page program may require a write-enable command, a command/address/data transfer, then polling a write-in-progress bit until completion. Erase can take much longer. The driver must avoid crossing page boundaries unexpectedly and must not start another command while the chip is busy.

**API / Code Anchor:**
```c
mutex_lock(&priv->lock);
ret = my_write_enable(priv);
if (!ret)
    ret = my_program_page(priv, addr, buf, len);
if (!ret)
    ret = my_wait_ready(priv, timeout_ms);
mutex_unlock(&priv->lock);
```

**Production or Debugging Angle:** For real SPI NOR flash, use the kernel's SPI-NOR/MTD infrastructure rather than inventing a raw flash ABI.

**Common Traps:** Ignoring datasheet page boundaries, using no timeout while polling busy, or exposing unsafe erase/write controls to userspace.

**Follow-up Questions:**
- Why is a timeout required?
- What protects against concurrent erase/read?
- When should you use an existing framework?

### 17. How would you review a SPI driver's Device Tree binding?
**Short Answer:** Check that generic SPI properties are used correctly, device-specific properties are documented, required supplies/clocks/GPIOs are represented, and examples pass binding validation.

**Deep Explanation:** DT is an ABI. The SPI child node should use correct generic properties such as `reg`, `spi-max-frequency`, and mode flags. Device-specific properties should have vendor prefixes and clear constraints. The binding should describe required resources and avoid board assumptions hardcoded in the driver.

**API / Code Anchor:**
```dts
sensor@0 {
    compatible = "vendor,my-spi-sensor";
    reg = <0>;
    spi-max-frequency = <10000000>;
    reset-gpios = <&gpio2 3 GPIO_ACTIVE_LOW>;
    vdd-supply = <&reg_3v3>;
};
```

**Production or Debugging Angle:** A good binding prevents future boards from relying on accidental driver defaults and makes bring-up failures easier to diagnose.

**Common Traps:** Adding undocumented properties, using generic names for vendor-specific behavior, or hiding required reset/power sequencing in board-specific driver code.

**Follow-up Questions:**
- What does "DT is ABI" mean?
- Why use vendor prefixes?
- What tool validates DT bindings?

### 18. How do you explain SPI device-driver lifetime during remove?
**Short Answer:** Remove must stop new operations, unregister subsystem interfaces, stop IRQ/work/async activity, and only then let private resources disappear.

**Deep Explanation:** A SPI driver may have subsystem callbacks, threaded IRQ handlers, delayed work, runtime PM, and async transfers. Remove must prevent new entry points, cancel or flush work, wait for any async completion if needed, unregister the subsystem object, and release non-devm resources. Devm allocation frees memory later, but it does not automatically make ordering correct.

**API / Code Anchor:**
```c
static void my_remove(struct spi_device *spi)
{
    struct mydev *priv = spi_get_drvdata(spi);

    my_unregister_subsystem(priv);
    cancel_work_sync(&priv->work);
    /* disable PM/IRQ paths as appropriate */
}
```

**Production or Debugging Angle:** Use-after-free bugs often appear after module unload, system suspend, or hot-unbind testing. Exercise bind/unbind and remove paths during review.

**Common Traps:** Assuming `devm_kzalloc()` means remove ordering no longer matters.

**Follow-up Questions:**
- What does devm cleanup not solve?
- How do async transfers affect remove?
- What should happen before freeing shared buffers?

