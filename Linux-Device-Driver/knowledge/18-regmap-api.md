# 18 - Regmap API

## Learning Goal
After this topic, you should be able to explain what the Linux Regmap API does, decide when a driver should use it, configure a `struct regmap_config`, access registers safely with `regmap_read()`, `regmap_write()`, and `regmap_update_bits()`, reason about register cache correctness, and debug common regmap bring-up failures.

The practical goal is not to memorize every field in `<linux/regmap.h>`. It is to understand **how a register-oriented chip becomes a checked, locked, cache-aware register map** that works over I2C, SPI, MMIO, and other buses.

## Why This Matters In Real Work
Many embedded devices are mostly "a table of registers behind some access method": PMICs, audio codecs, GPIO expanders, sensors, RTCs, camera sensors, regulators, watchdog blocks, and SoC system-control registers.

Without regmap, each driver tends to write its own helpers for:

- reading and writing registers over I2C, SPI, or MMIO;
- read-modify-write bit updates;
- register range validation;
- handling read-only, write-only, volatile, or side-effect registers;
- caching register values;
- locking register accesses;
- dumping registers while debugging.

Regmap exists so drivers can describe the register layout once and then use a common API.

You should consider regmap when:

- the device has numbered registers with fixed value widths;
- the same chip family can sit behind I2C and SPI;
- the driver repeatedly reads, writes, or updates bitfields;
- register access needs serialization;
- register reads are slow and a cache would help;
- the device has clear volatile/status registers and reset defaults;
- a higher-level framework expects or benefits from regmap, such as ASoC, regulator, MFD, GPIO, IIO, V4L2 sensor drivers, or syscon users.

Keep raw bus access when:

- the protocol is mostly streaming, not register-based;
- commands are too irregular for regmap to model cleanly;
- the device uses large FIFOs or no-increment windows as the main data path;
- a tiny driver has one or two simple transactions and no cache/locking benefit.

**Production rule:** regmap is not magic. It makes register access cleaner only when `regmap_config` matches the datasheet and the driver correctly marks volatile, precious, valid, and cacheable registers.

## Mental Model
Think of regmap as a translation and policy layer between your driver and the physical access method.

```text
Your driver
  -> asks: "write 0x80 to register 0x12"
  -> regmap checks: valid? writable? aligned? cached? locked?
  -> regmap formats: address width, value width, read/write flag, endian
  -> bus backend does: I2C, SPI, MMIO, SPMI, etc.
  -> hardware register changes
```

The driver works with a `struct regmap *`. The access method is chosen once during probe:

- I2C device: `devm_regmap_init_i2c()`
- SPI device: `devm_regmap_init_spi()`
- MMIO platform device: `devm_regmap_init_mmio()`

After that, normal access looks the same:

```c
regmap_read(map, REG_ID, &val);
regmap_write(map, REG_CTRL, CTRL_ENABLE);
regmap_update_bits(map, REG_CTRL, CTRL_MODE_MASK, CTRL_MODE_AUTO);
```

**Interview trap:** a datasheet "register map" is the hardware layout. Linux `regmap` is the kernel abstraction you configure from that layout. They are related, but not the same thing.

## Core Concepts
Regmap has a small number of ideas that carry most of the design.

| Concept | Meaning | Why it matters |
| --- | --- | --- |
| Register address width | Number of bits used for register addresses | `reg_bits = 8`, `16`, `32` |
| Register value width | Number of bits in each register value | `val_bits = 8`, `16`, `32` |
| Register stride | Valid address spacing | MMIO often uses `reg_stride = 4` |
| Access policy | Which registers are readable/writable/safe | prevents invalid or destructive operations |
| Volatile register | Hardware can change it or caching is unsafe | status/IRQ/FIFO registers |
| Precious register | Reading it has side effects or is sensitive | clear-on-read or destructive registers |
| Cache | Software copy of register values | faster reads, suspend/resume support |
| Read-modify-write | Change selected bits without clobbering others | `regmap_update_bits()` |
| Bus backend | I2C/SPI/MMIO-specific implementation | hidden after initialization |

### Regmap vs Raw I2C/SPI/MMIO
Regmap is most useful when a driver would otherwise duplicate common register helper code.

| Need | Raw bus access | Regmap |
| --- | --- | --- |
| One unusual command | often simpler | may be overkill |
| Many fixed registers | repetitive | strong fit |
| Read/write same code on I2C and SPI | duplicated helpers | one access API |
| Bitfield updates | hand-written read/modify/write | `regmap_update_bits()` |
| Cache | driver-owned | regcache support |
| Range checking | driver-owned | `max_register`, callbacks, tables |
| Debug register dump | driver-owned | debugfs support when enabled |
| Locking | driver-owned | built into regmap unless disabled |

### Volatile vs Precious
These two are easy to mix up.

| Register type | Meaning | Example | Regmap behavior goal |
| --- | --- | --- | --- |
| Volatile | value may change without a regmap write, or cache is unsafe | status register, ADC result, IRQ pending bit | read hardware, do not trust cache |
| Precious | reading may consume, clear, expose, or disturb state | clear-on-read event register, FIFO pop register | avoid casual debug/cache reads |

**Debugging clue:** if debugfs register dumping changes device behavior, you probably forgot to mark a side-effect register as precious or otherwise protect it.

## Kernel Mechanism
Regmap is implemented as a core plus bus-specific backends. The core owns validation, locking, cache handling, and common helpers. The backend knows how to perform one register operation on the chosen transport.

The main objects are:

```text
driver private data
  -> struct regmap *map
       -> regmap core state
       -> struct regmap_config policy
       -> bus-specific callbacks/context
       -> optional register cache
```

What happens during a typical write:

```text
regmap_write(map, reg, val)
  -> check register alignment against reg_stride
  -> take regmap lock unless disabled/customized
  -> check max_register and writable policy
  -> update or consult cache state as appropriate
  -> format register/value for I2C/SPI/MMIO backend
  -> perform hardware access
  -> release lock
  -> return 0 or negative errno
```

What happens during a typical read:

```text
regmap_read(map, reg, &val)
  -> check alignment/range/readable policy
  -> take lock
  -> if cache is valid and register is cacheable, read cache
  -> otherwise read hardware through backend
  -> store result in *val
  -> release lock
```

### Locking And Context
Regmap serializes register access by default. That matters because many drivers have multiple paths touching the same chip:

- probe initialization;
- subsystem callbacks;
- IRQ thread;
- workqueue;
- runtime PM callbacks;
- sysfs or debug paths.

Important rules:

- I2C and SPI regmap operations may sleep because the underlying bus transfer may sleep.
- MMIO regmap can be fast and non-sleeping when configured appropriately.
- `fast_io` is for fast, non-sleeping register I/O such as MMIO, not normal I2C/SPI.
- `disable_locking = true` is only safe when the driver provides equivalent external serialization or the map is truly single-threaded.
- Custom `lock` / `unlock` callbacks are advanced tools; use the default until there is a real reason.

**Production rule:** do not use a sleeping I2C/SPI regmap operation from hard IRQ context. Use a threaded IRQ handler or workqueue.

## Key Structs And APIs
The API surface is easier when grouped by purpose.

### Core Objects
| Struct/API | Role |
| --- | --- |
| `struct regmap` | runtime register map instance used by the driver |
| `struct regmap_config` | driver-provided register layout and policy |
| `struct regmap_range` | one address range |
| `struct regmap_access_table` | allow/deny table for readable/writable/volatile/precious ranges |
| `struct reg_default` | reset/default value for one register |
| `struct reg_sequence` | register/value/delay entry for multi-register writes |
| `enum regcache_type` | cache implementation choice |

### Initialization
Use the helper that matches the device bus.

| API | Use when |
| --- | --- |
| `devm_regmap_init_i2c(client, config)` | I2C client device |
| `devm_regmap_init_spi(spi, config)` | SPI device |
| `devm_regmap_init_mmio(dev, base, config)` | memory-mapped register block |
| `regmap_init_i2c()` / `regmap_init_spi()` | unmanaged lifetime |
| `regmap_exit(map)` | cleanup for unmanaged maps |

Prefer `devm_regmap_init_*()` when the map lifetime is exactly the same as the device lifetime.

### Configuration Fields
Only a few fields are mandatory for simple devices, but the policy fields are what make regmap valuable.

| Field | Meaning |
| --- | --- |
| `reg_bits` | number of bits in register addresses |
| `val_bits` | number of bits in register values |
| `reg_stride` | valid register address spacing |
| `pad_bits` | padding bits between register and value formatting |
| `max_register` | highest valid register address |
| `read_flag_mask` / `write_flag_mask` | bus flag bits, commonly useful for SPI |
| `readable_reg` / `writeable_reg` | callbacks for access validation |
| `volatile_reg` | callback for registers that should not be cached |
| `precious_reg` | callback for registers that should not be casually read |
| `wr_table` / `rd_table` | range-table alternatives to callbacks |
| `volatile_table` / `precious_table` | range-table policy for cache/debug safety |
| `reg_defaults` / `num_reg_defaults` | reset defaults used with cache |
| `cache_type` | cache disabled or cache implementation |
| `fast_io` | choose faster lock style for non-sleeping I/O |
| `disable_locking` | disable regmap's own locking |
| `reg_read` / `reg_write` | custom bus operation callbacks |
| `reg_format_endian` / `val_format_endian` | register/value endian handling |

### Register Access
These are the helpers you will use most often.

| API | Purpose |
| --- | --- |
| `regmap_read(map, reg, &val)` | read one register |
| `regmap_write(map, reg, val)` | write one register |
| `regmap_update_bits(map, reg, mask, val)` | read/modify/write selected bits |
| `regmap_bulk_read(map, reg, buf, count)` | read contiguous registers |
| `regmap_bulk_write(map, reg, buf, count)` | write contiguous registers |
| `regmap_multi_reg_write(map, seq, n)` | write register/value sequence |

The most important bitfield rule:

```text
new_value = (old_value & ~mask) | (val & mask)
```

That means `val` must already be shifted into the same bit positions as `mask`.

```c
/* Correct: set MODE field bits [5:4] to value 2 */
#define CTRL_MODE_MASK  GENMASK(5, 4)
#define CTRL_MODE_AUTO  FIELD_PREP(CTRL_MODE_MASK, 2)

ret = regmap_update_bits(map, REG_CTRL, CTRL_MODE_MASK, CTRL_MODE_AUTO);
```

**Common trap:** passing `2` directly as `val` for a field located at bits `[5:4]`. That updates the wrong bits.

## Lifecycle / Data Flow
The normal regmap lifecycle is a probe-time setup followed by ordinary subsystem use.

```text
1. Bus/device probe runs
2. Driver enables power/clock/reset enough for register access
3. Driver initializes regmap with bus-specific helper
4. Driver stores struct regmap * in private data
5. Driver verifies chip ID or status
6. Driver writes initialization sequence or default configuration
7. Driver registers with target subsystem
8. Runtime callbacks use regmap helpers
9. PM callbacks keep hardware and cache coherent
10. Remove unregisters users and stops late access before devm cleanup
```

### I2C Flow
```text
i2c_driver.probe()
  -> devm_kzalloc()
  -> devm_regmap_init_i2c()
  -> regmap_read(REG_CHIP_ID)
  -> regmap_multi_reg_write(init_sequence)
  -> register subsystem device
```

### SPI Flow
```text
spi_driver.probe()
  -> spi_setup() if needed
  -> devm_regmap_init_spi()
  -> regmap_read(REG_ID)
  -> regmap_update_bits(REG_CTRL, mask, val)
  -> register subsystem device
```

### MMIO Flow
```text
platform_driver.probe()
  -> devm_platform_ioremap_resource()
  -> devm_regmap_init_mmio()
  -> regmap_read(REG_STATUS)
  -> register subsystem device
```

### Cache And Power Flow
If the device loses power or resets, cached values may no longer match hardware.

Typical pattern to validate for your target kernel and driver:

- before suspend or power-off, prevent new register users;
- mark the cache dirty if hardware contents will be lost;
- use cache-only mode only when hardware is inaccessible;
- after resume/power-on/reset, restore hardware state with cache sync or reinitialization;
- keep volatile registers out of the cache path.

## Minimal Practical Example
This is a **learning-only** I2C example. It shows the shape of a regmap-backed probe and register access. It is not production-ready because a real driver needs a real datasheet, binding, power sequencing, subsystem registration, PM, and full error-path review.

```c
#include <linux/bitfield.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/slab.h>

#define MY_REG_ID        0x00
#define MY_REG_CTRL      0x10
#define MY_REG_STATUS    0x11
#define MY_REG_DATA      0x20
#define MY_REG_MAX       0x7f

#define MY_CTRL_ENABLE   BIT(0)
#define MY_CTRL_MODE     GENMASK(2, 1)

struct mychip {
    struct device *dev;
    struct regmap *map;
};

static bool mychip_readable_reg(struct device *dev, unsigned int reg)
{
    switch (reg) {
    case MY_REG_ID:
    case MY_REG_CTRL:
    case MY_REG_STATUS:
    case MY_REG_DATA:
        return true;
    default:
        return false;
    }
}

static bool mychip_writeable_reg(struct device *dev, unsigned int reg)
{
    switch (reg) {
    case MY_REG_CTRL:
        return true;
    default:
        return false;
    }
}

static bool mychip_volatile_reg(struct device *dev, unsigned int reg)
{
    return reg == MY_REG_STATUS || reg == MY_REG_DATA;
}

static const struct regmap_config mychip_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = MY_REG_MAX,
    .readable_reg = mychip_readable_reg,
    .writeable_reg = mychip_writeable_reg,
    .volatile_reg = mychip_volatile_reg,
    .cache_type = REGCACHE_RBTREE,
};

static int mychip_probe(struct i2c_client *client)
{
    struct mychip *chip;
    unsigned int id;
    int ret;

    chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
    if (!chip)
        return -ENOMEM;

    chip->dev = &client->dev;

    chip->map = devm_regmap_init_i2c(client, &mychip_regmap_config);
    if (IS_ERR(chip->map))
        return dev_err_probe(&client->dev, PTR_ERR(chip->map),
                             "failed to init regmap\n");

    i2c_set_clientdata(client, chip);

    ret = regmap_read(chip->map, MY_REG_ID, &id);
    if (ret)
        return dev_err_probe(&client->dev, ret, "failed to read chip id\n");

    ret = regmap_update_bits(chip->map, MY_REG_CTRL,
                             MY_CTRL_ENABLE | MY_CTRL_MODE,
                             MY_CTRL_ENABLE | FIELD_PREP(MY_CTRL_MODE, 2));
    if (ret)
        return dev_err_probe(&client->dev, ret, "failed to configure chip\n");

    return 0;
}
```

Important lines:

- `devm_regmap_init_i2c()` creates a managed regmap tied to `client->dev`.
- `reg_bits = 8` and `val_bits = 8` describe an 8-bit register address and 8-bit value device.
- `readable_reg` and `writeable_reg` reject invalid accesses before bus traffic.
- `volatile_reg` keeps status/data registers from being served from cache.
- `regmap_update_bits()` changes only selected control bits.

For SPI, the same runtime register access would stay the same. Only initialization changes:

```c
chip->map = devm_regmap_init_spi(spi, &mychip_regmap_config);
```

For MMIO, the driver first maps the register resource:

```c
base = devm_platform_ioremap_resource(pdev, 0);
if (IS_ERR(base))
    return PTR_ERR(base);

chip->map = devm_regmap_init_mmio(&pdev->dev, base, &mmio_regmap_config);
```

## Common Bugs And Debugging
Regmap bugs usually show up as probe failures, wrong register values, stale cached values, or suspicious side effects during debugging.

### Symptom: regmap init fails
Likely causes:

- wrong helper for the bus;
- missing I2C/SPI/MMIO resource;
- device not powered or reset properly before first access;
- invalid `regmap_config`;
- memory allocation failure.

What to inspect:

- probe logs from `dev_err_probe()`;
- bus device exists under `/sys/bus/i2c/devices`, `/sys/bus/spi/devices`, or platform device paths;
- regulator/clock/reset order;
- return value from `devm_regmap_init_*()`.

### Symptom: `regmap_read()` or `regmap_write()` returns `-EINVAL`
Likely causes:

- register address does not satisfy `reg_stride`;
- invalid argument;
- unsupported bulk/no-increment access shape.

Fix pattern:

- check register addresses against the datasheet;
- for MMIO, confirm whether hardware addresses are byte offsets or word indexes;
- set `reg_stride` to match the register layout.

### Symptom: access returns `-EIO`
Likely causes:

- register is above `max_register`;
- `readable_reg` or `writeable_reg` returned false;
- access table rejected the register;
- underlying bus transaction failed.

Fix pattern:

- temporarily add `dev_dbg()` in policy callbacks;
- verify the register appears in the correct readable/writable range;
- check physical bus errors with I2C/SPI/MMIO-specific debugging.

### Symptom: register values are wrong
Likely causes:

- wrong `reg_bits` or `val_bits`;
- wrong endian configuration;
- wrong SPI `read_flag_mask` or `write_flag_mask`;
- field value not shifted before `regmap_update_bits()`;
- hardware is still in reset or unpowered;
- datasheet register defaults differ from assumptions.

Fix pattern:

- compare bus waveform or MMIO access with datasheet format;
- read a stable chip ID first;
- use `FIELD_PREP()` for bitfields;
- validate reset/power sequencing.

### Symptom: cached values are stale
Likely causes:

- status/data register not marked volatile;
- hardware reset or power loss happened without marking cache dirty;
- another agent changed registers outside this regmap;
- cache-only/bypass/sync sequence is wrong.

Fix pattern:

- mark hardware-updated registers in `volatile_reg`;
- use the appropriate regcache dirty/sync helpers during PM after validating target-kernel behavior;
- avoid out-of-band raw bus writes to the same registers.

### Symptom: reading debugfs changes hardware behavior
Likely causes:

- clear-on-read or FIFO pop registers were not marked precious;
- debug workflow is dumping unsafe registers.

Fix pattern:

- mark side-effect registers with `precious_reg`;
- avoid blind register dumps on destructive devices;
- use targeted reads from known-safe registers.

### Useful Debug Tools
Regmap debugging depends on kernel configuration and target version, but common tools include:

- `dev_dbg()` and dynamic debug in your driver;
- debugfs regmap entries, often under `/sys/kernel/debug/regmap/`;
- ftrace events for regmap when enabled;
- bus-level tools: I2C traces, SPI logic analyzer, MMIO readback through driver logs;
- subsystem debug paths, such as regulator, ASoC, IIO, or V4L2 debug support.

**Debugging rule:** first prove the device is powered, matched, and reachable. Regmap cannot fix a wrong bus address, wrong chip-select, missing clock, or held reset line.

## Production Checklist
Use this checklist before sending a regmap-backed driver for review.

- Register layout:
  - `reg_bits` and `val_bits` match the datasheet.
  - `reg_stride` matches address spacing.
  - `max_register` is correct.
  - endian fields are correct for bus and device.
  - SPI read/write flag masks match the protocol.

- Access policy:
  - read-only registers are not writable.
  - write-only registers are not readable.
  - hardware-updated status/data registers are volatile.
  - clear-on-read, FIFO, event, or sensitive registers are precious.
  - range tables or callbacks are easy to review.

- Cache:
  - cache is enabled only when useful.
  - reset defaults are accurate.
  - volatile registers are excluded from cache.
  - suspend/resume or runtime PM keeps cache and hardware coherent.
  - power loss marks cache dirty before sync.

- Locking and context:
  - no sleeping I2C/SPI regmap calls from hard IRQ context.
  - multi-step driver state changes are serialized.
  - `fast_io` is used only for non-sleeping fast access.
  - `disable_locking` has a clear external-locking reason.

- Lifetime:
  - managed regmap lifetime matches the device.
  - subsystem users are unregistered before teardown.
  - workqueues, IRQ handlers, and PM callbacks cannot access a removed map.
  - unmanaged maps call `regmap_exit()` on every later probe failure path.

- Debug and review:
  - debugfs register dumps cannot read destructive registers.
  - errors use `dev_err_probe()` or useful `dev_err()` messages.
  - code uses `FIELD_PREP()` / `FIELD_GET()` where bitfields are non-trivial.
  - final examples and struct fields are checked against the target kernel headers.

## Interview Readiness
For interviews, focus on explaining the engineering tradeoff: regmap is valuable because it centralizes register access policy, locking, cache, and bus formatting, but it is only safe when configured from the datasheet correctly.

You should be able to explain:

- why regmap exists;
- `struct regmap` vs `struct regmap_config`;
- why initialization is bus-specific but access is bus-agnostic;
- how `regmap_update_bits()` works;
- volatile vs precious registers;
- when cache helps and when it is dangerous;
- why I2C/SPI regmap access can sleep;
- why `fast_io` is not for slow sleeping buses;
- how to debug `-EINVAL`, `-EIO`, stale reads, and side-effect reads;
- why regmap IRQ, MFD, and syscon are related but separate from the core topic.

Continue with [18-regmap-api interview questions](/home/tungnhs/TungNHS/Knowlege-Interview/Linux-Device-Driver/interview/18-regmap-api.md).

## Kernel Version Notes
Regmap APIs evolve quietly. Always check the target kernel's `<linux/regmap.h>` before copying examples.

Practical version-sensitive points:

- Older material may show unmanaged `regmap_init_i2c()` / `regmap_init_spi()` more often; modern drivers commonly prefer `devm_regmap_init_*()` when lifetime matches the device.
- Older material may show `use_single_rw`; current kernels split this into fields such as `use_single_read` and `use_single_write`.
- Current kernels contain many more `regmap_config` fields than older tutorials, including no-increment access support and additional cache/bus options.
- Regcache suspend/resume helpers and debugfs/tracepoint details should be validated against the exact kernel tree used by the project.
