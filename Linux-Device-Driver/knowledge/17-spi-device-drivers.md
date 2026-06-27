# 17 - SPI Device Drivers

## Learning Goal
After this topic, you should be able to explain how Linux represents a SPI peripheral, write the probe path for a SPI device driver, describe a SPI child device in Device Tree, choose between helper transfers, `spi_message`, `regmap`, and `spidev`, and debug common SPI bring-up failures.

The practical goal is not to memorize every field in `<linux/spi/spi.h>`. It is to understand **who owns the controller**, **who owns the chip protocol**, **why every SPI transfer is full-duplex**, and **how a described SPI device becomes a working kernel subsystem device**.

## Why This Matters In Real Work
Embedded boards often use SPI for devices that need more speed or simpler wiring rules than I2C can provide: serial flash, ADCs, DACs, displays, CAN controllers, Ethernet controllers, GPIO expanders, RTCs, audio codecs, and camera sensors.

SPI device drivers exist because these chips are usually **not hardware-enumerated**. Linux must be told that the device exists, which chip select it uses, which mode and maximum speed it needs, and which driver should bind to it.

You need a SPI device driver when:

- a chip sits behind an existing SPI controller;
- the device is selected by a chip-select line rather than an I2C-style address;
- the board describes the device through Device Tree, ACPI, board data, or another parent driver;
- Linux needs a real subsystem interface such as `iio`, `mtd`, `net`, `can`, `input`, `rtc`, `gpio`, `media`, or `ASoC`;
- the chip needs IRQ handling, power sequencing, locking, runtime PM, register caching, or safe shared access;
- userspace raw SPI access through `spidev` is too weak, unsafe, or temporary for the final product.

**Production rule:** a SPI device driver should own the chip-specific protocol and expose a proper kernel subsystem interface. Use `spidev` for prototyping or simple lab access, not as a permanent replacement for a real kernel driver when the device belongs in a kernel subsystem.

## Mental Model
SPI has two sides in Linux: the **SPI controller driver** owns the hardware block that toggles SCK/MOSI/MISO/CS, and the **SPI device driver** owns one peripheral chip connected to that controller.

```text
Your SPI device driver
  -> knows command bytes, registers, delays, status bits
  -> submits SPI messages through the SPI core
  -> SPI core queues work for the controller driver
  -> controller driver drives SCK/MOSI/MISO/chip select
  -> selected peripheral shifts data in and out
```

Think of it this way:

- `spi_controller`: the bus controller, often a SoC hardware block.
- `spi_device`: one peripheral instance behind one chip select.
- `spi_driver`: your driver code that binds to matching `spi_device` objects.
- `spi_transfer`: one chunk of simultaneous TX/RX clocking.
- `spi_message`: one ordered group of transfers to the same device.

**Interview trap:** SPI "read" is not like reading memory. To receive bytes, the controller must still generate clock pulses, which usually means transmitting command bytes, address bytes, dummy bytes, or ignored bytes while the device returns data.

## Core Concepts
SPI is a synchronous serial bus. The controller generates the clock, selects exactly one peripheral with chip select, and shifts one bit out while shifting one bit in on every clock edge.

| Concept | Meaning | Driver-facing example |
| --- | --- | --- |
| SPI controller | Hardware that owns bus timing and chip-select driving | `struct spi_controller` in controller drivers |
| SPI device | One peripheral chip on the bus | `struct spi_device *spi` in probe |
| SPI driver | Driver code for supported peripherals | `struct spi_driver` |
| Chip select | Per-device select line, often active low | `spi->chip_select`, DT `reg = <0>` |
| Mode | Clock polarity and phase | `SPI_MODE_0` through `SPI_MODE_3` |
| Word size | Bits per word transferred | `spi->bits_per_word`, `xfer.bits_per_word` |
| Transfer | One TX/RX buffer pair | `struct spi_transfer` |
| Message | Ordered transfer list submitted as one operation | `struct spi_message` |
| Regmap | Register access abstraction over SPI | `devm_regmap_init_spi()` |
| spidev | Generic userspace SPI character driver | `/dev/spidevB.C` |

### SPI vs I2C
The two buses solve similar board-design problems, but the Linux driver shape and wire behavior differ.

| Area | SPI | I2C |
| --- | --- | --- |
| Selection | chip select line | bus address |
| Duplex | full-duplex at the wire level | usually half-duplex |
| Clock owner | controller | controller |
| Discovery | not normally discoverable | not normally discoverable |
| Speed | often faster | often slower |
| Pins | more pins, especially with many devices | fewer pins |
| Driver object | `struct spi_device` | `struct i2c_client` |
| Transfer model | `spi_message` of `spi_transfer` | `i2c_msg` or SMBus helpers |

### Signals And Datasheet Names
Datasheets often use different names for the same SPI signals.

| Common name | Other names you may see | Direction |
| --- | --- | --- |
| MOSI | SIMO, SDI, DI, SDA | controller to peripheral |
| MISO | SOMI, SDO, DO, SDA | peripheral to controller |
| SCK | CLK, SCL | clock from controller |
| CS | SS, CSx, EN, ENB | chip select from controller |

### SPI Modes
SPI mode is the combination of clock polarity and phase.

| Mode | CPOL | CPHA | Kernel macro | Meaning |
| --- | --- | --- | --- | --- |
| 0 | 0 | 0 | `SPI_MODE_0` | clock idle low, sample on first edge |
| 1 | 0 | 1 | `SPI_MODE_1` | clock idle low, sample on second edge |
| 2 | 1 | 0 | `SPI_MODE_2` | clock idle high, sample on first edge |
| 3 | 1 | 1 | `SPI_MODE_3` | clock idle high, sample on second edge |

Common mode flags:

- `SPI_CPOL`: clock idle high.
- `SPI_CPHA`: sample on the second edge.
- `SPI_CS_HIGH`: chip select is active high.
- `SPI_LSB_FIRST`: transmit least-significant bit first.
- `SPI_3WIRE`: shared data line mode.
- `SPI_LOOP`: loopback mode for testing.

**Debugging clue:** wrong CPOL/CPHA often looks like "data is close but shifted or nonsense" on a logic analyzer. Wrong chip-select polarity often looks like the device never responds.

## Kernel Mechanism
Linux SPI follows the normal device model: a device is described or created, the SPI core creates a `spi_device`, the driver registers a `spi_driver`, matching happens, then probe initializes the chip.

The embedded Device Tree flow looks like this:

```text
Device Tree says:
  SPI controller is enabled
  child device exists at chip select 0
  child device supports compatible "vendor,mychip"
  max SPI clock is 10 MHz
  mode flags, IRQs, supplies, reset GPIOs are described

SPI core creates:
  struct spi_device

Driver registers:
  struct spi_driver with OF and SPI ID tables

SPI core matches:
  compatible/name -> driver

SPI core calls:
  probe(struct spi_device *spi)

Driver initializes:
  bus settings, private state, chip, IRQ/PM, subsystem registration
```

### Device Tree Shape
SPI devices are children of the SPI controller node. Their `reg` property is the **chip-select index**, not a CPU MMIO address.

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
        interrupt-parent = <&gpio1>;
        interrupts = <9 IRQ_TYPE_EDGE_FALLING>;
        reset-gpios = <&gpio2 3 GPIO_ACTIVE_LOW>;
        vdd-supply = <&reg_3v3>;
    };
};
```

Important DT rules:

- parent bus usually has `#address-cells = <1>` and `#size-cells = <0>`;
- child `reg = <N>` means chip select `N`;
- child unit address such as `sensor@0` should match `reg = <0>`;
- `spi-max-frequency` limits the bus speed for that device;
- `spi-cpol`, `spi-cpha`, `spi-cs-high`, `spi-lsb-first`, `spi-3wire`, `spi-tx-bus-width`, and `spi-rx-bus-width` describe SPI-specific electrical/protocol settings;
- device-specific properties should be documented in a binding, not invented silently.

### Matching And Autoload
A SPI driver commonly provides a Device Tree match table and a legacy/name table.

```c
static const struct of_device_id myspi_of_match[] = {
    { .compatible = "vendor,my-spi-sensor" },
    { }
};
MODULE_DEVICE_TABLE(of, myspi_of_match);

static const struct spi_device_id myspi_id[] = {
    { "my-spi-sensor", 0 },
    { }
};
MODULE_DEVICE_TABLE(spi, myspi_id);
```

`MODULE_DEVICE_TABLE()` exports module alias information so userspace module loading can find the right driver when the device appears.

**Interview trap:** a chip on SPI is not a platform-bus device just because it is non-discoverable. The SPI controller may itself be a platform device, but the peripheral driver should normally be a SPI driver.

## Key Structs And APIs
The important APIs become easier when grouped by what part of the driver uses them.

### Driver Objects
| Struct/API | Role |
| --- | --- |
| `struct spi_device` | matched peripheral instance, passed to probe/remove |
| `struct spi_driver` | driver callbacks, match tables, embedded `struct device_driver` |
| `struct spi_device_id` | legacy/name-based SPI match table |
| `struct of_device_id` | Device Tree compatible match table |
| `module_spi_driver()` | registers/unregisters the SPI driver for simple modules |
| `spi_register_driver()` / `spi_unregister_driver()` | manual registration path |

### Probe And Private Data
Private data holds state for one physical chip.

| API | Role |
| --- | --- |
| `devm_kzalloc(&spi->dev, ...)` | allocate per-device state |
| `spi_set_drvdata(spi, data)` | attach private data to the SPI device |
| `spi_get_drvdata(spi)` | retrieve private data in remove/IRQ/subsystem paths |
| `spi_get_device_id(spi)` | get the matched legacy ID entry when applicable |
| `dev_get_platdata(&spi->dev)` | legacy platform-data fallback |

Private data often contains:

- `struct spi_device *spi`;
- `struct regmap *map` for register-oriented chips;
- a subsystem object or pointer such as IIO, input, GPIO, RTC, MTD, CAN, V4L2, or ASoC state;
- `struct mutex lock` for multi-step register updates and shared buffers;
- workqueues/completions for async or IRQ-driven flows;
- cached chip configuration, variant data, and runtime PM state.

### Bus Setup
SPI drivers can set device communication parameters before the first transfer.

```c
spi->mode = SPI_MODE_3;
spi->max_speed_hz = 10000000;
spi->bits_per_word = 8;

ret = spi_setup(spi);
if (ret)
    return ret;
```

Use `spi_setup()` when:

- the driver must force a mode from the datasheet;
- the board did not provide a default;
- the device supports a non-default word size;
- the driver needs to apply changed mode/speed/word settings.

Per-transfer overrides:

- `xfer.speed_hz`: override speed for one transfer.
- `xfer.bits_per_word`: override word size for one transfer.
- `xfer.tx_nbits` / `xfer.rx_nbits`: request single/dual/quad style transfers when supported.

### Transfers And Messages
The transfer APIs are the heart of a SPI device driver.

| API | Role |
| --- | --- |
| `struct spi_transfer` | one TX/RX buffer pair and transfer settings |
| `struct spi_message` | ordered list of transfers to one device |
| `spi_message_init()` | initialize a message before adding transfers |
| `spi_message_add_tail()` | append a transfer to a message |
| `spi_sync()` | submit message and sleep until completion |
| `spi_async()` | queue message and return before completion |
| `spi_read()` | simple read helper |
| `spi_write()` | simple write helper |
| `spi_write_then_read()` | common command-then-read helper |

Key `struct spi_transfer` fields:

- `tx_buf`: bytes to transmit, or `NULL` for receive-only style transfers.
- `rx_buf`: receive buffer, or `NULL` for write-only style transfers.
- `len`: number of bytes to clock for this transfer.
- `speed_hz`: optional per-transfer speed.
- `bits_per_word`: optional per-transfer word size.
- `cs_change`: request chip-select behavior between transfers.
- `tx_dma` / `rx_dma`: DMA addresses when the driver maps buffers itself.

**Production rule:** transfer buffers must remain valid until the transfer completes. This is especially important for `spi_async()` and DMA-capable controllers.

### Regmap
For register-oriented chips, regmap often makes the driver smaller and safer.

```c
static const struct regmap_config my_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = 0xff,
};

priv->map = devm_regmap_init_spi(spi, &my_regmap_config);
if (IS_ERR(priv->map))
    return PTR_ERR(priv->map);
```

Use regmap when:

- the device mostly exposes registers;
- the same chip has SPI and I2C variants;
- you need access policy for readable/writable/volatile registers;
- you want cache support, debugfs visibility, or less repeated transfer code;
- register read/write flags such as "set top bit for read" can be described cleanly.

### spidev
`spidev` exposes a SPI device as `/dev/spidevB.C` for userspace.

| Userspace operation | Behavior |
| --- | --- |
| `read()` | half-duplex read |
| `write()` | half-duplex write |
| `SPI_IOC_MESSAGE(N)` | full-duplex or composite transfer |
| `SPI_IOC_WR_MODE` / `SPI_IOC_RD_MODE` | set/read mode |
| `SPI_IOC_WR_BITS_PER_WORD` / `SPI_IOC_RD_BITS_PER_WORD` | set/read word size |
| `SPI_IOC_WR_MAX_SPEED_HZ` / `SPI_IOC_RD_MAX_SPEED_HZ` | set/read speed |

Use `spidev` for:

- quick board bring-up;
- protocol experiments;
- simple userspace-controlled microcontroller links;
- lab tools where a kernel subsystem driver is not needed.

Avoid `spidev` as the final design when:

- the device needs IRQs;
- the device belongs to a kernel subsystem;
- multiple kernel clients need coordinated access;
- power management, reset sequencing, or security matters;
- the interface would become a long-term product ABI.

## Lifecycle / Data Flow
A clean SPI driver has a predictable path from module load to transfer to cleanup.

```text
1. Module load
   module_spi_driver(my_driver)
     -> SPI core registers struct spi_driver

2. Device exists
   DT/ACPI/board/parent creates struct spi_device

3. Match
   compatible or name matches driver's table
     -> probe(spi)

4. Probe
   parse firmware data
   set mode, max speed, bits per word
   spi_setup(spi)
   allocate private data
   initialize mutex/work/PM/regmap
   spi_set_drvdata(spi, priv)
   verify chip ID or status if available
   register with subsystem
   request IRQ if needed

5. Runtime operation
   subsystem callback or work item runs
   build spi_transfer entries
   build spi_message
   spi_sync() or spi_async()
   check return/status/length
   decode data or update state

6. Remove/shutdown
   stop new operations
   disable IRQ/work/PM paths
   unregister subsystem object
   wait for outstanding async operations if needed
   release non-devm resources
```

### `spi_sync()` vs `spi_async()`
Most simple drivers use `spi_sync()` from process context. Use `spi_async()` when you need nonblocking queueing or are integrating with a design that completes later.

| API | Sleeps? | Completion style | Use when |
| --- | --- | --- | --- |
| `spi_sync()` | yes | function returns when done | simple register or command operation |
| `spi_async()` | no immediate wait | callback/context | nonblocking flow, queued operation, interrupt-adjacent design |

**Production rule:** do not call `spi_sync()` from hard IRQ context. If an interrupt says data is ready, use a threaded IRQ handler or workqueue before doing sleepable SPI I/O.

## Minimal Practical Example
This example is **learning-only**. It shows the shape of a SPI sensor driver and a command-plus-read operation. A production driver would add a real subsystem registration, binding file, stronger error paths, PM, and device-specific timing.

```c
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/spi/spi.h>

#define MY_REG_CHIP_ID      0x00
#define MY_CMD_READ_REG     0x80
#define MY_EXPECTED_ID      0x42

struct myspi {
    struct spi_device *spi;
    struct mutex lock;
};

static int myspi_read_reg(struct myspi *priv, u8 reg, u8 *val)
{
    u8 tx = MY_CMD_READ_REG | reg;

    return spi_write_then_read(priv->spi, &tx, 1, val, 1);
}

static int myspi_probe(struct spi_device *spi)
{
    struct myspi *priv;
    u8 id;
    int ret;

    spi->mode = SPI_MODE_0;
    spi->bits_per_word = 8;

    if (!spi->max_speed_hz)
        spi->max_speed_hz = 1000000;

    ret = spi_setup(spi);
    if (ret)
        return ret;

    priv = devm_kzalloc(&spi->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->spi = spi;
    mutex_init(&priv->lock);
    spi_set_drvdata(spi, priv);

    mutex_lock(&priv->lock);
    ret = myspi_read_reg(priv, MY_REG_CHIP_ID, &id);
    mutex_unlock(&priv->lock);
    if (ret)
        return ret;

    if (id != MY_EXPECTED_ID)
        return dev_err_probe(&spi->dev, -ENODEV,
                             "unexpected chip id 0x%02x\n", id);

    dev_info(&spi->dev, "SPI sensor detected\n");
    return 0;
}

static void myspi_remove(struct spi_device *spi)
{
    struct myspi *priv = spi_get_drvdata(spi);

    /* Unregister subsystem objects here in a real driver. */
    mutex_destroy(&priv->lock);
}

static const struct of_device_id myspi_of_match[] = {
    { .compatible = "vendor,myspi-sensor" },
    { }
};
MODULE_DEVICE_TABLE(of, myspi_of_match);

static const struct spi_device_id myspi_id[] = {
    { "myspi-sensor", 0 },
    { }
};
MODULE_DEVICE_TABLE(spi, myspi_id);

static struct spi_driver myspi_driver = {
    .driver = {
        .name = "myspi-sensor",
        .of_match_table = myspi_of_match,
    },
    .probe = myspi_probe,
    .remove = myspi_remove,
    .id_table = myspi_id,
};
module_spi_driver(myspi_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Learning-only SPI device driver skeleton");
```

Important lines:

- `spi_setup(spi)` applies mode, speed, and word size before the first transfer.
- `spi_write_then_read()` is enough for simple command-then-read operations.
- `spi_set_drvdata()` keeps per-device state accessible from remove, IRQs, work, and subsystem callbacks.
- `mutex` protects multi-step access and shared state.
- `MODULE_DEVICE_TABLE(of, ...)` and `MODULE_DEVICE_TABLE(spi, ...)` support matching and module autoload.

### Multi-Transfer Pattern
Use `spi_message` directly when a command needs multiple ordered phases.

```c
static int myspi_burst_read(struct myspi *priv, u16 addr, u8 *buf, size_t len)
{
    u8 header[3] = {
        MY_CMD_READ_REG,
        addr >> 8,
        addr & 0xff,
    };
    struct spi_transfer xfers[2] = {
        {
            .tx_buf = header,
            .len = sizeof(header),
        },
        {
            .rx_buf = buf,
            .len = len,
        },
    };
    struct spi_message msg;

    spi_message_init(&msg);
    spi_message_add_tail(&xfers[0], &msg);
    spi_message_add_tail(&xfers[1], &msg);

    return spi_sync(priv->spi, &msg);
}
```

Use this pattern when:

- command, address, dummy cycles, and data are separate phases;
- chip-select behavior between phases matters;
- per-transfer speed or word size differs;
- helper APIs hide too much of the required protocol.

## Common Bugs And Debugging
SPI failures often look like "probe never runs", "probe runs but ID read fails", or "data is unstable". Start by separating device creation, driver binding, electrical setup, and transfer protocol.

### Symptom: Probe Never Runs
Likely causes:

- SPI controller node is disabled.
- Device Tree child node is missing or under the wrong controller.
- `compatible` does not match the driver's OF table.
- module alias was not emitted because `MODULE_DEVICE_TABLE()` is missing.
- driver is built as a module but not installed or not loadable.

Evidence to inspect:

```bash
dmesg | grep -i spi
ls /sys/bus/spi/devices
ls /sys/bus/spi/drivers
modinfo my_driver.ko
```

Fix pattern:

- verify controller `status = "okay"`;
- verify child `compatible`, `reg`, and unit address;
- add correct OF/SPI ID tables and `MODULE_DEVICE_TABLE()`;
- add a clear `dev_info()` or `dev_dbg()` at probe entry during bring-up.

### Symptom: Probe Runs But Chip ID Read Fails
Likely causes:

- wrong `SPI_MODE_0..3`;
- wrong chip-select index or polarity;
- excessive `spi-max-frequency`;
- missing pinctrl setup;
- missing regulator, reset GPIO, or required delay;
- wrong command byte, address width, dummy bytes, or bit order.

Evidence to inspect:

- `dev_err()` logs with command/register and error code;
- logic analyzer view of CS, SCK, MOSI, MISO;
- DT properties for `spi-cpol`, `spi-cpha`, `spi-cs-high`, and `spi-max-frequency`;
- reset and power sequencing in the datasheet.

Fix pattern:

- start at a slow clock rate;
- set mode exactly from the datasheet;
- confirm CS toggles for the expected device;
- read a stable ID/status register first;
- add datasheet-required reset and startup delays.

### Symptom: Transfer Works Once Then Fails Randomly
Likely causes:

- async buffer or context lifetime bug;
- shared transfer buffer without locking;
- stack buffer used in a path that can complete later or use DMA;
- runtime PM suspends device during access;
- status/write-in-progress polling is missing.

Evidence to inspect:

- whether the path uses `spi_async()`;
- lock coverage around shared state;
- `message.status` and `message.actual_length`;
- runtime PM logs and device power state;
- controller DMA limitations.

Fix pattern:

- keep async messages, transfers, and buffers alive until completion;
- use `mutex` around multi-step state updates;
- use devm or private buffers for long-lived operations;
- wait for device-ready status bits before the next command;
- resume the device before register access if runtime PM is enabled.

### Symptom: spidev Read/Write Does Not Match Kernel Driver Behavior
Likely causes:

- `read()` and `write()` are half-duplex and deassert chip select between calls;
- userspace did not configure mode, word size, or speed;
- the protocol needs one composite transfer with CS held active.

Fix pattern:

- use `SPI_IOC_MESSAGE(N)` with `struct spi_ioc_transfer`;
- set mode, bits per word, and speed through ioctl;
- avoid treating a successful userspace experiment as a final driver design.

## Production Checklist
Use this checklist before review or board bring-up.

Driver model:

- The driver is a `spi_driver`, not a platform driver for a SPI peripheral.
- OF and SPI ID tables match the supported devices.
- `MODULE_DEVICE_TABLE(of, ...)` and `MODULE_DEVICE_TABLE(spi, ...)` are present when needed.
- Probe fails cleanly and returns meaningful errno values.
- Remove/shutdown paths stop new activity and undo subsystem registration.

Device Tree and bindings:

- SPI controller is enabled and pinctrl is correct.
- Child node has correct `compatible`, unit address, and `reg` chip-select index.
- `spi-max-frequency` is within datasheet limits for board voltage/mode.
- Mode flags match the datasheet.
- IRQ, reset GPIO, regulators, clocks, and custom properties are documented and parsed.
- Binding YAML and `dtbs_check` are used for production DT changes.

Transfers:

- `spi_setup()` is called after changing mode/speed/word size.
- `spi_sync()` is only used from sleepable context.
- `spi_async()` buffers/context live until completion.
- Return values, `message.status`, and `actual_length` are checked where relevant.
- Shared buffers and read-modify-write sequences are locked.
- DMA-sensitive paths do not rely on unsafe temporary buffers.

Device behavior:

- Datasheet reset, startup, dummy-cycle, page-size, status-polling, and timing rules are followed.
- Slow initial clock is used during bring-up if communication is unstable.
- Runtime PM resumes the device before register access.
- IRQ handlers use threaded context or workqueues before doing SPI I/O.

Architecture:

- Regmap is used for register-heavy chips unless the protocol does not fit.
- A proper subsystem ABI is used instead of a custom raw userspace ABI.
- `spidev` is limited to prototyping, board tests, or explicitly accepted userspace-control use cases.

## Interview Readiness
You are ready for SPI interview questions when you can explain the data path without memorized slogans.

Be able to answer:

- What is the difference between a SPI controller driver and a SPI device driver?
- Why does SPI "read" require clocking data out?
- How do CPOL and CPHA map to `SPI_MODE_0..3`?
- What does `reg` mean in a SPI child Device Tree node?
- What should probe do before the first transfer?
- What is the difference between `spi_transfer` and `spi_message`?
- When should you use `spi_sync()`, `spi_async()`, helper APIs, or regmap?
- Why is `spi_sync()` wrong in hard IRQ context?
- Why is async buffer lifetime dangerous?
- What are the limits of `spidev`?

See `interview/17-spi-device-drivers.md` for structured beginner, mid-level, senior, and debugging questions.

## Kernel Version Notes
SPI APIs are stable in concept, but names and struct fields have changed across kernel versions.

- Older material often says `spi_master`; modern kernel documentation generally uses `spi_controller` for controller-side APIs.
- Older examples may show `delay_usecs` in `struct spi_transfer`; current kernels use richer delay fields such as `delay`, `cs_change_delay`, and `word_delay`.
- Current kernels have more transfer capability fields than older tutorials, such as octal transfer support, effective speed reporting, timestamp/offload fields, and error flags.
- Upstream `spidev` no longer supports describing a device directly as `compatible = "spidev"`. Use a real supported device name or explicit driver override for lab binding.
- Write final code against the target kernel headers, not copied struct definitions from old books or notes.
