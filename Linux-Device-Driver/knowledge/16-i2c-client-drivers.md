# 16 - I2C Client Drivers

## Learning Goal
After this topic, you should be able to explain how Linux represents an I2C device, write the probe path for an I2C client driver, choose between SMBus, plain I2C, and regmap access, describe I2C devices in Device Tree, and debug common probe and transfer failures.

The practical goal is not to memorize every helper in `<linux/i2c.h>`. It is to know **who owns the bus**, **who owns the chip**, **which context may sleep**, and **how a matched I2C device becomes a working kernel subsystem device**.

## Why This Matters In Real Work
Many embedded boards are full of I2C chips: PMICs, RTCs, EEPROMs, temperature sensors, touch controllers, GPIO expanders, audio codecs, camera sensors, light sensors, and ADCs.

I2C client drivers exist because these chips are not memory-mapped and are not automatically discoverable like PCI or USB devices.

You need an I2C client driver when:

- a chip sits behind an existing I2C controller;
- the device has a fixed bus address, such as `0x50` or `0x68`;
- the device is described by Device Tree, ACPI, board data, or another parent driver;
- Linux needs a real subsystem driver, not ad hoc userspace register poking;
- the chip exposes a standard kernel interface such as `hwmon`, `iio`, `input`, `rtc`, `regulator`, `gpio`, `nvmem`, `media`, or `ASoC`;
- the driver must handle interrupts, power sequencing, runtime PM, locking, or variant-specific configuration.

**Production rule:** an I2C client driver should own the chip-specific protocol and expose a proper kernel subsystem interface. Do not build a permanent userspace ABI around raw register reads if an existing subsystem fits.

## Mental Model
I2C has two sides in Linux: the **adapter/controller** owns the bus wires, and the **client driver** owns one addressed chip on that bus.

```text
Your driver
  -> knows the chip registers and behavior
  -> calls I2C core APIs
  -> I2C core calls the adapter driver
  -> adapter driver controls SDA/SCL hardware
  -> addressed chip responds on the bus
```

Think of it this way:

- `i2c_adapter`: the bus segment, usually provided by a SoC controller driver.
- `i2c_client`: one slave device at one address on that adapter.
- `i2c_driver`: the driver code that can bind to matching clients.

**Interview trap:** `probe()` is not where you scan the I2C bus. Probe means Linux already has an `i2c_client` and found a matching `i2c_driver`; now your driver must initialize the chip.

## Core Concepts
I2C client drivers sit on top of the Linux device model. They look like other bus drivers: match, probe, use private data, register with a subsystem, then clean up.

| Concept | Meaning | Driver-facing example |
| --- | --- | --- |
| I2C adapter | Bus/controller instance | `client->adapter` |
| I2C client | One chip on an adapter | `struct i2c_client` |
| I2C driver | Driver code for supported chips | `struct i2c_driver` |
| I2C address | Slave address on the bus | `client->addr`, `reg = <0x50>` |
| I2C core | Matching and helper API layer | `i2c_transfer()`, `i2c_check_functionality()` |
| SMBus | Simpler protocol family related to I2C | `i2c_smbus_read_byte_data()` |
| Plain I2C | Lower-level message transfer API | `i2c_master_send()`, `i2c_transfer()` |
| Regmap | Register access abstraction | `devm_regmap_init_i2c()` |

### Adapter vs Client vs Driver
These three names are easy to mix up.

| Object | Owns | Created by | Used for |
| --- | --- | --- | --- |
| `struct i2c_adapter` | one I2C bus segment | adapter/controller driver | bus transfers |
| `struct i2c_client` | one device instance | I2C core from firmware/board/parent info | address, IRQ, `struct device` |
| `struct i2c_driver` | driver callbacks and match tables | your module/driver code | binding to clients |

One `i2c_driver` can bind to multiple `i2c_client` instances. For example, one EEPROM driver may manage EEPROMs at `0x50` and `0x51`.

### SMBus vs Plain I2C vs Regmap
Most I2C chip drivers are register-oriented. You still need to choose the right access layer.

| Access style | Use when | Typical APIs |
| --- | --- | --- |
| SMBus helpers | simple byte/word/block register operations | `i2c_smbus_read_byte_data()` |
| Plain I2C send/recv | simple raw byte streams | `i2c_master_send()`, `i2c_master_recv()` |
| `i2c_transfer()` | combined transactions or repeated START | `struct i2c_msg`, `I2C_M_RD` |
| Regmap | many registers, caching, locking, shared SPI/I2C design | `regmap_read()`, `regmap_write()` |

**Production rule:** use SMBus helpers when they match the device protocol and adapter capabilities. Use `i2c_transfer()` when the datasheet requires a combined transaction, such as writing a register address followed by a repeated START read.

## Kernel Mechanism
Linux cannot usually discover I2C devices electrically and identify them safely. Software must tell the kernel which devices exist and where they are.

The normal embedded flow is:

```text
Device Tree says:
  i2c bus has a child device at address 0x48

I2C core creates:
  struct i2c_client

Driver registers:
  struct i2c_driver with match tables

I2C core matches:
  compatible/name -> driver

I2C core calls:
  driver probe(client)

Driver initializes:
  chip, private data, IRQ, PM, subsystem registration
```

### Device Tree Shape
I2C devices are children of the I2C controller node. Their `reg` property is the I2C slave address, not an MMIO base address.

```dts
&i2c1 {
    #address-cells = <1>;
    #size-cells = <0>;
    clock-frequency = <100000>;
    status = "okay";

    temp-sensor@48 {
        compatible = "vendor,tmp102-like";
        reg = <0x48>;
        interrupt-parent = <&gpio1>;
        interrupts = <9 IRQ_TYPE_EDGE_FALLING>;
    };
};
```

Important DT rules:

- child node name uses `name@address`;
- unit address should match `reg`;
- `reg = <0x48>` is one cell because `#address-cells = <1>`;
- there is no size cell because I2C devices are not CPU-addressable MMIO regions;
- `compatible` is matched against the driver's OF match table;
- interrupt properties may become `client->irq`.

### Matching And Autoload
Drivers usually provide both I2C ID and OF match tables, especially if they support both legacy/name matching and Device Tree.

```c
static const struct of_device_id tmp_of_match[] = {
    { .compatible = "vendor,tmp102-like" },
    { }
};
MODULE_DEVICE_TABLE(of, tmp_of_match);

static const struct i2c_device_id tmp_id[] = {
    { "tmp102-like", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, tmp_id);
```

`MODULE_DEVICE_TABLE()` lets module alias information be emitted so userspace module loading can find the right driver.

## Key Structs And APIs
The important APIs are easier to remember when grouped by lifecycle stage.

### Driver Objects
| Struct/API | Role |
| --- | --- |
| `struct i2c_driver` | driver callbacks, match tables, embedded `struct device_driver` |
| `struct i2c_client` | matched chip instance: address, adapter, IRQ, `struct device` |
| `struct i2c_adapter` | bus segment used for transfers |
| `struct i2c_device_id` | legacy/name-based I2C match table |
| `struct of_device_id` | Device Tree compatible match table |
| `module_i2c_driver()` | registers/unregisters the I2C driver for module init/exit |

### Probe And Private Data
Use private data to hold state for one physical chip.

| API | Role |
| --- | --- |
| `devm_kzalloc(&client->dev, ...)` | allocate per-device private data |
| `i2c_set_clientdata(client, data)` | attach private data to the client |
| `i2c_get_clientdata(client)` | retrieve private data in remove/IRQ/subsystem paths |
| `dev_get_drvdata()` / `dev_set_drvdata()` | generic device-driver-data equivalents |

Private data often contains:

- `struct i2c_client *client`;
- a `struct regmap *map` if using regmap;
- a subsystem object or pointer, such as an `iio_dev`, `input_dev`, `gpio_chip`, or `rtc_device`;
- a `struct mutex` for multi-step state/register updates;
- cached register values;
- IRQ and runtime PM state.

### Capability Checking
Before using a transfer style, confirm the adapter supports it.

```c
if (!i2c_check_functionality(client->adapter,
                             I2C_FUNC_SMBUS_BYTE_DATA |
                             I2C_FUNC_SMBUS_WORD_DATA))
    return -EOPNOTSUPP;
```

Common flags:

- `I2C_FUNC_I2C`: plain I2C transfers through `i2c_transfer()`;
- `I2C_FUNC_SMBUS_BYTE_DATA`: byte register reads/writes;
- `I2C_FUNC_SMBUS_WORD_DATA`: 16-bit SMBus word operations;
- `I2C_FUNC_SMBUS_BLOCK_DATA`: SMBus block operations;
- `I2C_FUNC_SMBUS_I2C_BLOCK`: I2C-style block operations through SMBus helpers.

### Plain I2C
Use these when the protocol needs raw I2C messages.

```c
int i2c_master_send(const struct i2c_client *client,
                    const char *buf, int count);

int i2c_master_recv(const struct i2c_client *client,
                    char *buf, int count);

int i2c_transfer(struct i2c_adapter *adap,
                 struct i2c_msg *msgs, int num);
```

Important return rules:

- negative value: errno failure;
- non-negative value: number of bytes or messages transferred;
- check for short transfers;
- individual I2C message length is limited by the 16-bit `msg.len` field.

### SMBus Helpers
SMBus helpers are simpler and fit many register devices.

```c
s32 i2c_smbus_read_byte_data(struct i2c_client *client, u8 command);
s32 i2c_smbus_write_byte_data(struct i2c_client *client,
                              u8 command, u8 value);
s32 i2c_smbus_read_word_data(struct i2c_client *client, u8 command);
s32 i2c_smbus_write_word_data(struct i2c_client *client,
                              u8 command, u16 value);
s32 i2c_smbus_read_i2c_block_data(struct i2c_client *client,
                                  u8 command, u8 length, u8 *values);
```

SMBus return rules:

| Operation | Success | Failure |
| --- | --- | --- |
| write helper | `0` | negative errno |
| byte/word read | data value, non-negative | negative errno |
| block read | number of bytes read | negative errno |

**Common trap:** do not treat `0` from a read as failure. `0` can be valid data.

### Regmap
Regmap is usually the better shape for a serious register-heavy driver.

```c
static const struct regmap_config tmp_regmap_config = {
    .reg_bits = 8,
    .val_bits = 16,
    .max_register = 0xff,
};

data->map = devm_regmap_init_i2c(client, &tmp_regmap_config);
if (IS_ERR(data->map))
    return PTR_ERR(data->map);
```

Regmap helps with:

- consistent register read/write/update APIs;
- caching;
- access validation;
- locking;
- easier SPI/I2C variant support;
- debugfs visibility for many regmap users.

Regmap belongs in depth to topic 18, but I2C drivers should recognize when to use it.

## Lifecycle / Data Flow
An I2C client driver's life is mostly probe, normal access, and remove/shutdown/PM.

### Probe Flow
```text
1. I2C core calls probe with a matched client.
2. Driver checks adapter functionality.
3. Driver allocates private data.
4. Driver initializes locks, regmap/cache, and default state.
5. Driver stores private data with i2c_set_clientdata().
6. Driver optionally reads chip ID or status register.
7. Driver applies reset/configuration/power setup.
8. Driver requests IRQ if client->irq is valid.
9. Driver registers with a kernel subsystem.
10. Probe returns 0 only when the device is usable.
```

### Runtime Access Flow
```text
subsystem callback
  -> retrieve private data
  -> runtime-resume if PM can suspend the chip
  -> lock if doing multi-step state/register updates
  -> read/write through SMBus, i2c_transfer, or regmap
  -> convert raw device format into subsystem units
  -> unlock
  -> mark device idle if runtime PM is used
```

### Remove / Shutdown Flow
```text
remove/shutdown
  -> stop new users if needed
  -> unregister subsystem object
  -> disable device interrupts / cancel work
  -> runtime-resume if a final I2C write is needed
  -> put hardware in safe state
  -> disable runtime PM
  -> free non-devm resources / unregister secondary clients
```

**Production rule:** devm cleanup frees many resources, but it does not replace thinking about hardware order. Disable interrupt generation before the object handling those interrupts disappears.

## Minimal Practical Example
This is a learning-only skeleton for a simple register-based temperature sensor. It demonstrates the shape, not a production-ready datasheet driver.

```c
// Learning-only skeleton. Validate prototypes against your target kernel.
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#define TMP_REG_ID      0x00
#define TMP_REG_TEMP    0x01
#define TMP_EXPECTED_ID 0x5a

struct tmp_data {
    struct i2c_client *client;
    struct mutex lock;
};

static int tmp_read_temp_mdegc(struct tmp_data *data, int *temp)
{
    s32 raw;

    mutex_lock(&data->lock);
    raw = i2c_smbus_read_word_data(data->client, TMP_REG_TEMP);
    mutex_unlock(&data->lock);

    if (raw < 0)
        return raw;

    /*
     * Device-specific conversion belongs here.
     * Real drivers must check endian and datasheet format.
     */
    *temp = raw * 125;
    return 0;
}

static int tmp_probe(struct i2c_client *client)
{
    struct tmp_data *data;
    s32 id;

    if (!i2c_check_functionality(client->adapter,
                                 I2C_FUNC_SMBUS_BYTE_DATA |
                                 I2C_FUNC_SMBUS_WORD_DATA))
        return -EOPNOTSUPP;

    data = devm_kzalloc(&client->dev, sizeof(*data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    data->client = client;
    mutex_init(&data->lock);
    i2c_set_clientdata(client, data);

    id = i2c_smbus_read_byte_data(client, TMP_REG_ID);
    if (id < 0)
        return id;
    if (id != TMP_EXPECTED_ID)
        return -ENODEV;

    /*
     * Production driver would now configure the chip and register with
     * hwmon, IIO, input, RTC, regulator, GPIO, or another subsystem.
     */
    dev_info(&client->dev, "temperature sensor found\n");
    return 0;
}

static void tmp_remove(struct i2c_client *client)
{
    struct tmp_data *data = i2c_get_clientdata(client);

    mutex_destroy(&data->lock);
}

static const struct of_device_id tmp_of_match[] = {
    { .compatible = "vendor,tmp-learning" },
    { }
};
MODULE_DEVICE_TABLE(of, tmp_of_match);

static const struct i2c_device_id tmp_id[] = {
    { "tmp-learning", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, tmp_id);

static struct i2c_driver tmp_driver = {
    .driver = {
        .name = "tmp-learning",
        .of_match_table = tmp_of_match,
    },
    .probe = tmp_probe,
    .remove = tmp_remove,
    .id_table = tmp_id,
};
module_i2c_driver(tmp_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Learning-only I2C temperature sensor skeleton");
```

Key lines:

- `i2c_check_functionality()` prevents using unsupported adapter operations.
- `devm_kzalloc()` ties private data lifetime to `client->dev`.
- `i2c_set_clientdata()` makes private data retrievable from callbacks.
- the chip ID read proves communication and prevents binding to the wrong device.
- the OF and I2C ID tables support matching and module autoload.
- a real driver should register with a subsystem instead of stopping at `dev_info()`.

### Combined Transfer Example
Some devices require a register-address write followed by a repeated START read. Use `i2c_transfer()` for that.

```c
static int eeprom_read(struct i2c_client *client, u16 off, u8 *buf, int len)
{
    u8 addr[2] = { off >> 8, off & 0xff };
    struct i2c_msg msgs[2] = {
        {
            .addr = client->addr,
            .flags = 0,
            .len = sizeof(addr),
            .buf = addr,
        },
        {
            .addr = client->addr,
            .flags = I2C_M_RD,
            .len = len,
            .buf = buf,
        },
    };
    int ret;

    ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
    if (ret < 0)
        return ret;
    if (ret != ARRAY_SIZE(msgs))
        return -EIO;

    return 0;
}
```

Why this matters:

- there is no STOP between the address write and data read;
- many EEPROMs and sensors require that exact bus sequence;
- two separate helper calls may generate a STOP and break the transaction.

## Common Bugs And Debugging
I2C failures often look simple from userspace but have several layers: firmware instantiation, pinctrl/electrical setup, adapter capability, protocol sequence, chip power, and driver error handling.

### Symptom: Probe Never Runs
Likely causes:

- the I2C controller node is disabled;
- the child node is missing or has the wrong `compatible`;
- the module alias was not generated because `MODULE_DEVICE_TABLE()` is missing;
- the driver name/ID table does not match non-DT instantiation;
- the device is on a different bus than expected;
- the driver was built out-of-tree against stale headers.

What to inspect:

- `dmesg`;
- `/sys/bus/i2c/devices/`;
- `/sys/bus/i2c/drivers/`;
- `modinfo <module>.ko`;
- compiled DTB or `/proc/device-tree`;
- adapter presence under `/sys/class/i2c-adapter/`.

Fix pattern:

- verify the bus node is `status = "okay"`;
- verify node address and `reg`;
- verify `compatible` exactly matches the OF table;
- add `MODULE_DEVICE_TABLE(of, ...)` and `MODULE_DEVICE_TABLE(i2c, ...)`;
- check kernel-version callback prototypes.

### Symptom: Probe Runs But Register Read Fails
Likely causes:

- no pull-ups, wrong pinctrl state, or bus wiring issue;
- chip is held in reset or unpowered;
- wrong I2C address;
- adapter lacks the chosen SMBus/plain I2C operation;
- datasheet requires a delay after reset/power-on;
- the first register access requires a combined transaction.

What to inspect:

- driver `dev_err()` logs and errno;
- `i2c_check_functionality()` result;
- board schematic and pull-ups;
- regulator/reset GPIO/clock sequencing;
- logic analyzer trace if available.

Fix pattern:

- power and reset the chip before reading ID;
- use the exact transaction form from the datasheet;
- replace split send/recv with `i2c_transfer()` if repeated START is required;
- check for short transfers.

### Symptom: Data Looks Wrong
Likely causes:

- endian mismatch for word registers;
- signed-value conversion bug;
- wrong register address width;
- using SMBus word helper for a non-SMBus byte order;
- missing lock around read-modify-write sequences;
- stale cached register state.

Fix pattern:

- decode raw values beside datasheet examples;
- use `get_unaligned_*()` or explicit byte assembly where appropriate;
- prefer regmap when register layout is rich;
- protect multi-register or read-modify-write paths with `mutex`.

### Symptom: "Sleeping Function Called From Invalid Context"
Likely causes:

- I2C/SMBus/regmap access in a hard IRQ handler;
- `mutex_lock()` in hard IRQ context;
- `msleep()` or `GFP_KERNEL` allocation in atomic context.

Fix pattern:

- use `devm_request_threaded_irq()` for I2C interrupt pins;
- put I2C register access in the threaded handler or workqueue;
- use `IRQF_ONESHOT` for thread-only IRQ handling.

### Symptom: Interrupt Storm
Likely causes:

- threaded handler never clears the device-level interrupt cause;
- wrong trigger polarity/type;
- missing `IRQF_ONESHOT`;
- level-triggered line remains asserted;
- status register clear order is wrong.

What to inspect:

- `/proc/interrupts`;
- `client->irq`;
- DT `interrupt-parent` and `interrupts`;
- handler logs through dynamic debug;
- device status/mask registers.

## Production Checklist
Use this checklist before review or board bring-up.

### Driver Binding
- Device Tree binding describes `compatible`, `reg`, interrupts, supplies, reset GPIOs, clocks, and custom properties.
- `compatible` strings are stable and vendor-prefixed.
- Driver has OF and I2C ID tables where appropriate.
- `MODULE_DEVICE_TABLE()` entries exist for module autoload.
- Driver uses current callback prototypes for the target kernel.

### Probe And Lifetime
- Probe checks adapter functionality before using bus operations.
- Private data is allocated once per client and attached with `i2c_set_clientdata()`.
- Device identity is verified when the hardware supports it.
- Framework registration happens only after the chip is usable.
- Error paths unwind resources in the reverse order or use devm correctly.
- Remove/shutdown disables hardware events before freeing dependent objects.

### Register Access
- Access method matches the datasheet transaction requirements.
- Short transfers are checked.
- SMBus read return values treat `0` as valid data.
- Endian and signed conversion are explicit.
- Multi-step state changes are protected by `mutex`.
- Regmap is considered for register-heavy designs.

### IRQ And Context
- I2C transfers happen only in sleepable context.
- Interrupt-driven I2C devices use threaded IRQ/workqueue for bus access.
- `IRQF_ONESHOT` is used for thread-only handlers where needed.
- Device interrupt cause is cleared/masked according to datasheet.

### Power And Board Integration
- Regulators, resets, clocks, and pinctrl are sequenced before I2C access.
- Runtime PM resumes the device before register access.
- Autosuspend delay matches device wake/conversion timing.
- The driver avoids unsafe generic bus scanning for chips without reliable ID.

### Debuggability
- Logs use `dev_err()`, `dev_warn()`, `dev_dbg()`, and `dev_info()` sparingly.
- Error messages include enough context: register, operation, errno, or address.
- Debug plan includes `/sys/bus/i2c/devices`, `dmesg`, dynamic debug, and safe i2c-tools usage.

## Interview Readiness
You are ready for interviews when you can explain the object model, the probe path, the access APIs, and the context rules without reciting prototypes.

Be able to answer:

- What is the difference between `i2c_adapter`, `i2c_client`, and `i2c_driver`?
- Why are I2C devices explicitly instantiated?
- What should an I2C probe function do?
- When do you use SMBus helpers versus `i2c_transfer()`?
- Why does repeated START matter?
- Why must I2C access not run in hard IRQ context?
- What does `i2c_check_functionality()` protect you from?
- How does Device Tree describe an I2C child device?
- When is regmap worth using?
- How do you debug "probe did not run" versus "transfer failed"?

See `interview/16-i2c-client-drivers.md` for structured beginner, mid-level, and senior questions.

## Kernel Version Notes
I2C callback prototypes are version-sensitive. Older examples commonly use:

```c
int probe(struct i2c_client *client, const struct i2c_device_id *id);
int remove(struct i2c_client *client);
```

Current kernel documentation shows the modern shape as:

```c
int probe(struct i2c_client *client);
void remove(struct i2c_client *client);
```

Before writing final code, check the headers for your target kernel. If the driver still needs ID-table data in modern code, use the matching helper appropriate for that kernel, such as `i2c_match_id()`, instead of relying on a stale probe argument.

Some I2C device-creation helper names also changed over time. Validate secondary/dummy-client helpers against the target kernel before using them in production code.
