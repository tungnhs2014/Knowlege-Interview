# 16 - I2C Client Drivers Interview Questions

Strong candidates can explain the adapter/client/driver model, trace how a Device Tree child node becomes an `i2c_client`, choose the right transfer API, and debug I2C failures without guessing at random addresses.

## Beginner Questions

### 1. What is an I2C client driver?
**Short Answer:** An I2C client driver is a Linux driver for one type of I2C slave device, such as an EEPROM, RTC, sensor, PMIC, GPIO expander, or codec.

**Deep Explanation:** The I2C controller driver owns the physical bus and exposes an `i2c_adapter`. The client driver owns chip-specific behavior: register layout, command sequences, delays, interrupts, power handling, and subsystem registration. Linux represents each physical chip as an `i2c_client`, and a matching `i2c_driver` binds to it.

**API / Code Anchor:**
```c
struct i2c_driver my_driver = {
    .driver = {
        .name = "mychip",
        .of_match_table = my_of_match,
    },
    .probe = my_probe,
    .remove = my_remove,
    .id_table = my_id,
};
module_i2c_driver(my_driver);
```

**Production or Debugging Angle:** If the board has a real I2C chip, the right long-term solution is usually a subsystem driver, not permanent userspace scripts poking `/dev/i2c-*`.

**Common Traps:** Confusing an I2C client driver with an I2C adapter/controller driver. The client driver does not toggle SDA/SCL directly.

**Follow-up Questions:**
- What is an `i2c_adapter`?
- What is an `i2c_client`?
- Name three kernel subsystems that often have I2C client drivers.

### 2. What is the difference between `i2c_adapter`, `i2c_client`, and `i2c_driver`?
**Short Answer:** `i2c_adapter` is the bus, `i2c_client` is one device on that bus, and `i2c_driver` is the driver code that binds to matching clients.

**Deep Explanation:** The adapter is usually created by a controller driver such as a SoC I2C driver. The client is created from Device Tree, ACPI, board data, or another parent driver and contains the address, adapter pointer, device object, and IRQ. The driver registers callbacks and match tables so the I2C core can bind it to clients.

**API / Code Anchor:**
```c
client->adapter; /* bus segment */
client->addr;    /* slave address */
client->dev;     /* device model object */
```

**Production or Debugging Angle:** When debugging, first identify whether the adapter exists, whether the client exists, and whether the driver bound. These are different failures.

**Common Traps:** Saying the driver creates the adapter. A normal client driver uses an existing adapter.

**Follow-up Questions:**
- Where do you see adapters in sysfs?
- Can one driver bind to multiple clients?
- Can one adapter host multiple clients?

### 3. Why are I2C devices explicitly instantiated?
**Short Answer:** I2C does not provide reliable hardware enumeration and identification like PCI or USB, so software must tell Linux what devices exist and at which addresses.

**Deep Explanation:** An I2C transfer to an address may only tell you whether something ACKed. It does not safely tell Linux what chip it is, what IRQ line it uses, what regulators it needs, or what variant-specific configuration applies. Embedded systems normally describe this through Device Tree child nodes under the I2C controller.

**API / Code Anchor:**
```dts
&i2c1 {
    #address-cells = <1>;
    #size-cells = <0>;

    rtc@68 {
        compatible = "nxp,pcf8523";
        reg = <0x68>;
    };
};
```

**Production or Debugging Angle:** Avoid generic device detection in new board-specific embedded drivers unless the hardware has a reliable identification method and the subsystem expects scanning.

**Common Traps:** Assuming `i2cdetect` is proof that a safe kernel driver can auto-detect the chip. Some devices react badly to probing.

**Follow-up Questions:**
- What does `reg` mean for an I2C child node?
- Why is `#size-cells = <0>`?
- What information besides address might the driver need?

### 4. What should an I2C probe function do?
**Short Answer:** Probe should initialize the matched chip: check adapter capability, allocate private state, verify/configure the device, set client data, request IRQ/PM resources if needed, and register with a kernel subsystem.

**Deep Explanation:** Probe is called after the I2C core matched an `i2c_client` to an `i2c_driver`. A good probe path makes the device usable or fails cleanly. It should not guess randomly on the bus, expose partial state, or ignore transfer failures.

**API / Code Anchor:**
```c
static int my_probe(struct i2c_client *client)
{
    struct mydev *priv;

    if (!i2c_check_functionality(client->adapter,
                                 I2C_FUNC_SMBUS_BYTE_DATA))
        return -EOPNOTSUPP;

    priv = devm_kzalloc(&client->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->client = client;
    i2c_set_clientdata(client, priv);

    return my_register_with_subsystem(priv);
}
```

**Production or Debugging Angle:** Probe logs should make it obvious whether failure was allocation, capability, chip ID, power/reset, IRQ, or subsystem registration.

**Common Traps:** Returning success before subsystem registration is complete, or ignoring a failed chip ID read.

**Follow-up Questions:**
- Why call `i2c_check_functionality()`?
- What is private data used for?
- What should remove undo?

## Mid-Level Questions

### 5. When should you use SMBus helpers versus `i2c_transfer()`?
**Short Answer:** Use SMBus helpers for simple byte/word/block operations when they match the chip protocol. Use `i2c_transfer()` when the device needs full I2C message control, especially combined transactions with repeated START.

**Deep Explanation:** SMBus helpers are convenient and widely supported by adapters. They encode common register-style transactions. `i2c_transfer()` lets you build a list of `struct i2c_msg` operations with explicit read/write flags and no STOP between messages. Some devices require exactly that sequence.

**API / Code Anchor:**
```c
/* SMBus register read */
ret = i2c_smbus_read_byte_data(client, REG_STATUS);

/* Combined I2C transaction */
struct i2c_msg msgs[2] = {
    { .addr = client->addr, .flags = 0, .len = 1, .buf = &reg },
    { .addr = client->addr, .flags = I2C_M_RD, .len = len, .buf = buf },
};
ret = i2c_transfer(client->adapter, msgs, 2);
```

**Production or Debugging Angle:** If a register read works on one device but fails on another similar chip, check whether the datasheet requires repeated START and whether your helper inserts a STOP.

**Common Traps:** Assuming separate `i2c_master_send()` and `i2c_master_recv()` are equivalent to a combined transfer. They may not be.

**Follow-up Questions:**
- What does `I2C_M_RD` mean?
- What does `i2c_transfer()` return on success?
- Why is repeated START important?

### 6. How should you handle return values from I2C and SMBus APIs?
**Short Answer:** Negative values are errors. Plain send/recv return byte counts and must be checked for short transfers. SMBus reads return data or negative errno; SMBus writes return 0 or negative errno.

**Deep Explanation:** I2C APIs do not all follow the same success convention. A byte read returning `0` can be valid data, not failure. A plain send returning `1` when you expected `2` is a short transfer and should usually become `-EIO`.

**API / Code Anchor:**
```c
ret = i2c_master_send(client, buf, 2);
if (ret < 0)
    return ret;
if (ret != 2)
    return -EIO;

val = i2c_smbus_read_byte_data(client, REG);
if (val < 0)
    return val;
```

**Production or Debugging Angle:** Bad return handling causes silent misconfiguration. A driver may think it programmed a chip when only one byte was sent.

**Common Traps:** Treating `0` from `i2c_smbus_read_byte_data()` as failure, or ignoring short transfers.

**Follow-up Questions:**
- What does an SMBus block read return?
- What errno would you use for a short transfer?
- Why should logs include the operation that failed?

### 7. What does `i2c_check_functionality()` protect you from?
**Short Answer:** It checks whether the adapter supports the transfer type your driver intends to use.

**Deep Explanation:** Not every adapter supports every plain I2C or SMBus operation. A driver using SMBus word reads should check for `I2C_FUNC_SMBUS_WORD_DATA`. A driver using combined plain I2C transfers should check `I2C_FUNC_I2C`.

**API / Code Anchor:**
```c
if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    return -EOPNOTSUPP;
```

**Production or Debugging Angle:** Capability failures are cleaner than mysterious transfer failures later. They also make the driver portable across different SoCs and I2C controllers.

**Common Traps:** Checking `I2C_FUNC_SMBUS_BYTE_DATA` and then using `i2c_transfer()` anyway. Check the operation you actually use.

**Follow-up Questions:**
- Which flag is needed for plain I2C transfer support?
- Which flags would you check for byte and word SMBus registers?
- Should you check functionality once or before every transfer?

### 8. How does Device Tree matching work for an I2C client?
**Short Answer:** The I2C child node's `compatible` string is matched against the driver's OF match table, and the child node's `reg` property becomes the client's I2C address.

**Deep Explanation:** The I2C controller node defines the address format for children. Each child represents one client. The I2C core creates `struct i2c_client` objects from those children. The generic driver model and I2C core then bind matching drivers through OF or ID table matching.

**API / Code Anchor:**
```c
static const struct of_device_id my_of_match[] = {
    { .compatible = "vendor,mychip" },
    { }
};
MODULE_DEVICE_TABLE(of, my_of_match);

static struct i2c_driver my_driver = {
    .driver = {
        .name = "mychip",
        .of_match_table = my_of_match,
    },
};
```

**Production or Debugging Angle:** If probe never runs, compare the DT `compatible` byte-for-byte against the driver's OF table and verify the module alias exists with `modinfo`.

**Common Traps:** Using `reg = <0x1000 0x100>` like an MMIO device. I2C child `reg` is the bus address only.

**Follow-up Questions:**
- Why does the I2C controller use `#size-cells = <0>`?
- What does `MODULE_DEVICE_TABLE(of, ...)` do?
- How can a driver support multiple chip variants?

### 9. Why is `i2c_set_clientdata()` used?
**Short Answer:** It attaches per-device private data to the `i2c_client` so callbacks can retrieve the driver's state later.

**Deep Explanation:** A driver may bind to multiple client instances. Global variables would mix state between chips. Private data keeps per-device state such as the `client` pointer, locks, cached registers, regmap pointer, IRQ data, and subsystem objects.

**API / Code Anchor:**
```c
struct mydev {
    struct i2c_client *client;
    struct mutex lock;
};

i2c_set_clientdata(client, priv);
priv = i2c_get_clientdata(client);
```

**Production or Debugging Angle:** Correct private data is essential for multi-instance safety. Two identical sensors on the same bus should not share mutable state.

**Common Traps:** Using one global `struct i2c_client *` or one global register cache for all devices.

**Follow-up Questions:**
- What usually goes in private data?
- How does devm allocation affect private-data lifetime?
- When do you retrieve client data?

### 10. Why must I2C access not happen in a hard IRQ handler?
**Short Answer:** I2C transfers can sleep, and hard IRQ context cannot sleep.

**Deep Explanation:** I2C transactions may wait for bus availability, controller completion, locks, or scheduling. A hard IRQ handler runs in atomic context. Calling sleeping APIs there can trigger kernel warnings or deadlocks. For I2C interrupt pins, use a threaded IRQ handler or workqueue.

**API / Code Anchor:**
```c
ret = devm_request_threaded_irq(&client->dev, client->irq,
                                NULL, my_thread_fn,
                                IRQF_ONESHOT | IRQF_TRIGGER_FALLING,
                                "mychip", priv);
```

**Production or Debugging Angle:** If you see "sleeping function called from invalid context", inspect IRQ handlers for SMBus, regmap, mutex, `msleep()`, or `GFP_KERNEL` calls.

**Common Traps:** Thinking a one-byte I2C read is "small enough" for hard IRQ context. Size does not change the context rule.

**Follow-up Questions:**
- Why is `IRQF_ONESHOT` useful?
- Can a threaded IRQ handler take a mutex?
- What if the IRQ line is shared?

## Senior Questions

### 11. Design the probe path for a production I2C sensor driver.
**Short Answer:** Probe should bring the hardware to a known powered state, validate adapter capabilities, allocate state, initialize locks/regmap, verify chip identity, configure defaults, request IRQ/PM resources, register with the correct subsystem, and unwind cleanly on failure.

**Deep Explanation:** A production probe path is an ownership and ordering problem. The chip must be powered before register access. The subsystem object should appear only after the device is usable. Interrupts should not fire before private data and handler state are ready. Runtime PM should not suspend the device while the driver is still initializing.

**API / Code Anchor:**
```text
probe
  -> enable supplies/reset/clock
  -> check I2C functionality
  -> allocate private data
  -> init mutex/regmap/cache
  -> read chip ID
  -> configure hardware
  -> request threaded IRQ
  -> enable runtime PM
  -> register hwmon/IIO/input/etc.
```

**Production or Debugging Angle:** Make every failure point return the real errno and leave the hardware in a safe state. If using devm, still think about interrupt disable order and PM balance.

**Common Traps:** Registering the subsystem before the device can actually respond, enabling interrupts before the handler is ready, or letting runtime PM suspend the chip during setup.

**Follow-up Questions:**
- Where would you enable runtime PM?
- What needs explicit cleanup even with devm?
- How do you handle optional IRQs?

### 12. When would you choose regmap for an I2C driver?
**Short Answer:** Use regmap when the device is register-heavy, has repeated read/write/update patterns, needs caching/access rules, or has both I2C and SPI variants.

**Deep Explanation:** Regmap abstracts register access away from raw bus transactions. It can handle locking, register widths, value widths, readable/writeable/volatile rules, caching, bulk access, and shared code between buses. This keeps subsystem logic focused on device behavior instead of open-coded I2C helpers everywhere.

**API / Code Anchor:**
```c
static const struct regmap_config my_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = 0x7f,
};

priv->map = devm_regmap_init_i2c(client, &my_regmap_config);
if (IS_ERR(priv->map))
    return PTR_ERR(priv->map);

ret = regmap_update_bits(priv->map, REG_CTRL, MASK_ENABLE, MASK_ENABLE);
```

**Production or Debugging Angle:** Regmap reduces bugs in read-modify-write sequences and often improves debugging through regmap debugfs. It is especially useful in PMIC, codec, sensor, GPIO-expander, and MFD-style drivers.

**Common Traps:** Using regmap blindly for a device whose protocol is not register-like, or forgetting to mark volatile registers that should not be cached.

**Follow-up Questions:**
- What are volatile registers?
- How does regmap help an I2C/SPI combo driver?
- When would raw `i2c_transfer()` still be needed?

### 13. Debug this: the I2C client appears in sysfs, but probe fails with `-ENODEV` after reading the chip ID.
**Short Answer:** The device was instantiated and matched, but the driver decided the hardware was not the expected chip. Check address, variant, power/reset timing, register address, endian/format, and whether the ID read transaction is correct.

**Deep Explanation:** Since the client exists, DT instantiation and matching likely worked. Failure after chip ID read points to communication content, not binding. The ACK may come from a different chip at that address, the chip may be in reset, the ID register may require a delay or page select, or the driver may use the wrong access method.

**API / Code Anchor:**
```c
id = i2c_smbus_read_byte_data(client, REG_CHIP_ID);
if (id < 0)
    return id;
if (id != EXPECTED_ID)
    return -ENODEV;
```

**Production or Debugging Angle:** Log the unexpected ID value and register read errno. Check the schematic, DT `reg`, power rails, reset GPIO, datasheet timing, and logic analyzer trace if available.

**Common Traps:** Immediately changing the expected ID constant without proving the bus address and register transaction are correct.

**Follow-up Questions:**
- What is the difference between NACK and wrong ID data?
- How can runtime PM or reset affect this?
- What would you check in Device Tree?

### 14. Debug this: an interrupt-driven I2C sensor causes an interrupt storm.
**Short Answer:** The threaded handler probably is not clearing the device interrupt cause, the trigger type/polarity is wrong, or the IRQ is re-enabled before the device-level condition is cleared.

**Deep Explanation:** I2C interrupt pins are often level-triggered. The Linux IRQ core can mask/unmask the parent line, but the device still owns the status bit that keeps the line asserted. The threaded handler must read status, process the event, clear or mask the cause in the device, and return `IRQ_HANDLED`.

**API / Code Anchor:**
```c
ret = devm_request_threaded_irq(dev, client->irq,
                                NULL, sensor_irq_thread,
                                IRQF_ONESHOT | IRQF_TRIGGER_LOW,
                                "sensor", priv);
```

**Production or Debugging Angle:** Inspect `/proc/interrupts`, DT `interrupts`, device status registers, clear-on-read behavior, and whether the handler logs repeat faster than it can clear the source.

**Common Traps:** Believing `IRQF_ONESHOT` clears the device interrupt. It only keeps the IRQ masked until the thread completes.

**Follow-up Questions:**
- Why is I2C access in the threaded handler okay?
- What is different for edge-triggered IRQs?
- How would you handle a shared IRQ line?

### 15. How do you make an I2C driver safe for multiple identical devices?
**Short Answer:** Keep all mutable state per client, store it with `i2c_set_clientdata()`, avoid globals for device state, use per-device locks, and make subsystem registration instance-specific.

**Deep Explanation:** Two devices can use the same driver at different addresses or adapters. The driver code is shared, but each `i2c_client` needs separate private data, register cache, IRQ state, PM state, and subsystem object. Global state causes cross-device corruption and impossible debugging.

**API / Code Anchor:**
```c
struct mychip {
    struct i2c_client *client;
    struct mutex lock;
    u8 cached_ctrl;
};

priv = devm_kzalloc(&client->dev, sizeof(*priv), GFP_KERNEL);
i2c_set_clientdata(client, priv);
```

**Production or Debugging Angle:** Test with two instances in DT if possible. Verify logs include `dev_name(&client->dev)` or use `dev_*()` so messages identify the bus/address instance.

**Common Traps:** A global `struct i2c_client *client`, a global buffer, or a static register cache inside helper functions.

**Follow-up Questions:**
- How does `devm_kzalloc()` choose lifetime?
- Where should the mutex live?
- How do module globals differ from per-device constants?

### 16. What kernel-version traps exist for I2C client drivers?
**Short Answer:** Callback prototypes and some helper names changed across kernel versions, so examples from books or old drivers may not compile against current kernels.

**Deep Explanation:** Older examples often show `probe(struct i2c_client *, const struct i2c_device_id *)` and `int remove(struct i2c_client *)`. Current kernel documentation shows `probe(struct i2c_client *)` and `void remove(struct i2c_client *)`. Some client-creation helpers also changed names over time. A strong engineer validates against the target kernel headers instead of copying snippets blindly.

**API / Code Anchor:**
```c
/* Modern documented shape */
static int my_probe(struct i2c_client *client);
static void my_remove(struct i2c_client *client);

/* If ID table data is needed, check target-kernel helper support. */
id = i2c_match_id(my_id, client);
```

**Production or Debugging Angle:** In out-of-tree drivers, build failures around `.probe` or `.remove` often mean the code targets a different kernel API. Fix the driver for the target kernel, not just with casts.

**Common Traps:** Forcing old function pointers with casts, or deleting ID tables without checking module autoload and variant matching.

**Follow-up Questions:**
- How do you verify the target prototype?
- Why might an ID table still be useful?
- What should you do when supporting multiple kernel versions?

## Debugging Scenarios And Traps
These scenarios test whether the candidate can separate binding, transport, protocol, and subsystem problems.

| Scenario | Likely Layer | First Checks |
| --- | --- | --- |
| Probe never runs | instantiation/matching | DT `compatible`, `reg`, module alias, `/sys/bus/i2c/devices` |
| Probe runs, first read returns `-ENXIO` or transfer error | bus/address/electrical | address, pull-ups, pinctrl, reset, power |
| Read succeeds but value is wrong | protocol/conversion | register address, endian, datasheet format, repeated START |
| Interrupt storm | IRQ/device status | `IRQF_ONESHOT`, trigger type, clear order, `/proc/interrupts` |
| Sleep warning | context bug | hard IRQ path, mutex, SMBus/regmap calls |
| Works once, fails after idle | PM bug | runtime PM get/put balance, resume before I2C access |

**Best interview signal:** the candidate asks what evidence exists at each layer before changing code.
