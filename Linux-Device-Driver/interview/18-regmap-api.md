# 18 - Regmap API Interview Questions

Strong candidates can explain why regmap exists, configure it from a datasheet, reason about cache and side-effect registers, and debug register-access failures without falling back to vague "it abstracts I2C/SPI" answers.

## Beginner Questions

### 1. What is the Linux Regmap API?
**Short Answer:** Regmap is a Linux kernel framework that provides a common API for accessing device registers over buses such as I2C, SPI, and MMIO.

**Deep Explanation:** Many chips expose numbered registers. Without regmap, each driver writes its own register read/write helpers, read-modify-write code, access checks, locking, and cache handling. Regmap lets the driver describe the register layout in `struct regmap_config`, initialize a `struct regmap`, and then use helpers such as `regmap_read()`, `regmap_write()`, and `regmap_update_bits()` regardless of the underlying bus.

**API / Code Anchor:**
```c
map = devm_regmap_init_i2c(client, &my_regmap_config);
ret = regmap_read(map, MY_REG_ID, &val);
ret = regmap_write(map, MY_REG_CTRL, MY_CTRL_ENABLE);
```

**Production or Debugging Angle:** Regmap is useful when the device has a real register map, repeated register accesses, bitfield updates, or cache/locking needs.

**Common Traps:** Saying "regmap is only for I2C" or "regmap is just a wrapper around read/write." It also handles policy, locking, formatting, and cache behavior.

**Follow-up Questions:**
- What does `struct regmap_config` describe?
- Which buses commonly use regmap?
- Name two regmap helpers used after initialization.

### 2. What is the difference between `struct regmap` and `struct regmap_config`?
**Short Answer:** `struct regmap_config` is the driver-provided description of the register layout and policy; `struct regmap` is the runtime map instance returned by regmap initialization.

**Deep Explanation:** The config tells regmap how wide register addresses and values are, which addresses are valid, which registers are readable/writable/volatile/precious, whether caching is enabled, and how values should be formatted. The `struct regmap *` stores runtime state and is passed to access APIs.

**API / Code Anchor:**
```c
static const struct regmap_config cfg = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = 0x7f,
};

chip->map = devm_regmap_init_spi(spi, &cfg);
```

**Production or Debugging Angle:** If the config is wrong, every later register access may be wrong even though the high-level driver logic looks fine.

**Common Traps:** Allocating or modifying `struct regmap` directly. Drivers should treat it as an opaque object and use regmap APIs.

**Follow-up Questions:**
- Where is `struct regmap *` usually stored?
- Which fields are normally mandatory for a simple map?
- Why should the config often be `static const`?

### 3. Why is regmap called bus-agnostic?
**Short Answer:** After initialization, the same regmap access APIs are used whether the device is on I2C, SPI, or MMIO.

**Deep Explanation:** Initialization is bus-specific because regmap must know how to perform physical transactions. Once the map is created, the driver can call `regmap_read()` or `regmap_update_bits()` without caring whether the backend is I2C, SPI, or MMIO. This is especially useful for chip families available in both I2C and SPI variants.

**API / Code Anchor:**
```c
/* I2C variant */
chip->map = devm_regmap_init_i2c(client, &cfg);

/* SPI variant */
chip->map = devm_regmap_init_spi(spi, &cfg);

/* Runtime code can be shared */
regmap_update_bits(chip->map, REG_CTRL, MASK_ENABLE, MASK_ENABLE);
```

**Production or Debugging Angle:** Shared access code can reduce bugs across bus variants, but bus-specific details like SPI read flags still belong in the config.

**Common Traps:** Assuming bus-agnostic means bus behavior is irrelevant. I2C/SPI may sleep; MMIO may not. SPI may need read/write flag masks.

**Follow-up Questions:**
- Which helper initializes an MMIO regmap?
- Why might a SPI regmap need `read_flag_mask`?
- Can the same driver support I2C and SPI variants?

### 4. What does `regmap_update_bits()` do?
**Short Answer:** It performs a read-modify-write operation that changes only the bits selected by `mask`.

**Deep Explanation:** Regmap reads the old register value, clears the bits in `mask`, ORs in `val & mask`, and writes the result back if needed. The value passed to `val` must already be positioned under the mask. For multi-bit fields, helpers like `FIELD_PREP()` prevent shift mistakes.

**API / Code Anchor:**
```c
#define MODE_MASK GENMASK(5, 4)

ret = regmap_update_bits(map, REG_CTRL,
                         MODE_MASK,
                         FIELD_PREP(MODE_MASK, 2));
```

**Production or Debugging Angle:** This avoids clobbering unrelated bits in shared control registers.

**Common Traps:** Passing an unshifted value, such as `2`, for a field located at bits `[5:4]`.

**Follow-up Questions:**
- What is the formula for the new value?
- When is `FIELD_PREP()` useful?
- Why is read-modify-write risky without locking?

## Mid-Level Questions

### 5. How do you configure which registers are readable, writable, volatile, or precious?
**Short Answer:** Use callbacks such as `readable_reg`, `writeable_reg`, `volatile_reg`, and `precious_reg`, or range tables such as `rd_table`, `wr_table`, `volatile_table`, and `precious_table`.

**Deep Explanation:** Access policy lets regmap reject invalid operations and keep cache/debug behavior safe. Readable/writable policy prevents accidental access to invalid or unsupported registers. Volatile policy tells regmap not to satisfy reads from cache. Precious policy tells regmap that casual reads, such as debug dumping, may be unsafe.

**API / Code Anchor:**
```c
static bool my_volatile_reg(struct device *dev, unsigned int reg)
{
    return reg == REG_STATUS || reg == REG_FIFO;
}

static const struct regmap_config cfg = {
    .reg_bits = 8,
    .val_bits = 8,
    .volatile_reg = my_volatile_reg,
};
```

**Production or Debugging Angle:** Incorrect policy can cause stale status reads, invalid bus transactions, or destructive reads during debugging.

**Common Traps:** Treating volatile and precious as synonyms. Volatile is about cache correctness; precious is about avoiding unsafe reads.

**Follow-up Questions:**
- Give an example of a volatile register.
- Give an example of a precious register.
- When would range tables be cleaner than callbacks?

### 6. What is regcache, and when is it dangerous?
**Short Answer:** Regcache is regmap's software cache of register values. It improves performance and helps restore state, but it is dangerous if cached values do not match hardware.

**Deep Explanation:** A cache can avoid slow bus reads and support suspend/resume state restoration. It relies on correct reset defaults and correct volatile markings. If the hardware resets, loses power, or changes a register independently, the cache may become stale unless the driver marks it dirty, bypasses it, or synchronizes it correctly.

**API / Code Anchor:**
```c
static const struct regmap_config cfg = {
    .reg_bits = 8,
    .val_bits = 8,
    .reg_defaults = defaults,
    .num_reg_defaults = ARRAY_SIZE(defaults),
    .volatile_reg = my_volatile_reg,
    .cache_type = REGCACHE_RBTREE,
};
```

**Production or Debugging Angle:** Cache bugs are subtle: register reads look successful, but the value comes from stale software state.

**Common Traps:** Enabling cache without marking status/data registers volatile, or trusting datasheet defaults after a bootloader already changed hardware state.

**Follow-up Questions:**
- What should happen after power loss?
- Why do reset defaults matter?
- Which registers should never be cached?

### 7. How would you initialize regmap in I2C, SPI, and MMIO drivers?
**Short Answer:** Use `devm_regmap_init_i2c()` for I2C, `devm_regmap_init_spi()` for SPI, and `devm_regmap_init_mmio()` after mapping the MMIO resource for memory-mapped devices.

**Deep Explanation:** The initialization helper binds the regmap core to the correct physical access method. For I2C and SPI, the bus device object provides the bus context. For MMIO, the driver first obtains an `__iomem` base address and then gives that to regmap. Runtime access then uses the same regmap APIs.

**API / Code Anchor:**
```c
map = devm_regmap_init_i2c(client, &cfg);
map = devm_regmap_init_spi(spi, &cfg);

base = devm_platform_ioremap_resource(pdev, 0);
map = devm_regmap_init_mmio(&pdev->dev, base, &mmio_cfg);
```

**Production or Debugging Angle:** The device must usually be powered, clocked, and out of reset before the first meaningful register access.

**Common Traps:** Calling MMIO regmap init before mapping the resource, or using an I2C/SPI regmap from hard IRQ context as if it were MMIO.

**Follow-up Questions:**
- Why is `reg_stride = 4` common for MMIO?
- What does `fast_io` imply?
- When would you use unmanaged `regmap_init_*()`?

### 8. A `regmap_write()` fails with `-EIO`. What do you check?
**Short Answer:** Check `max_register`, writable policy, access tables, and the underlying bus error.

**Deep Explanation:** Regmap can return errors before the physical transaction if its policy rejects the register. `-EIO` commonly points to invalid register access by policy or a failed bus transaction. A wrong `writeable_reg()` callback, missing range table entry, or too-low `max_register` can look like a bus failure if you do not inspect the config.

**API / Code Anchor:**
```c
static bool my_writeable_reg(struct device *dev, unsigned int reg)
{
    switch (reg) {
    case REG_CTRL:
    case REG_CONFIG:
        return true;
    default:
        return false;
    }
}
```

**Production or Debugging Angle:** Add temporary debug logs in access-policy callbacks and confirm the physical bus separately with known-safe ID/status reads.

**Common Traps:** Debugging only the I2C/SPI waveform while ignoring that regmap rejected the operation before the bus transfer.

**Follow-up Questions:**
- What might cause `-EINVAL` instead?
- How can debugfs help?
- Why should writable callbacks be easy to review?

### 9. Why should `fast_io` not be used for normal I2C or SPI register access?
**Short Answer:** `fast_io` is intended for fast, non-sleeping register I/O, while I2C and SPI transfers normally may sleep.

**Deep Explanation:** Regmap uses locking appropriate for the configured access path. I2C/SPI transactions are not simple CPU loads/stores; they may queue transfers, sleep, and wait for controller completion. MMIO access can often use faster locking because it is local and non-sleeping.

**API / Code Anchor:**
```c
static const struct regmap_config mmio_cfg = {
    .reg_bits = 32,
    .val_bits = 32,
    .reg_stride = 4,
    .fast_io = true, /* only when access is truly non-sleeping */
};
```

**Production or Debugging Angle:** Wrong context assumptions lead to "sleeping function called from invalid context" warnings or deadlocks.

**Common Traps:** Thinking "SPI is fast" means `fast_io = true`. The field is about sleeping and locking behavior, not bus bandwidth.

**Follow-up Questions:**
- Can `regmap_read()` sleep?
- Which regmap backends are more likely to be non-sleeping?
- What context should an I2C IRQ handler use for register access?

## Senior Questions

### 10. How do you decide whether a driver should use regmap or raw bus helpers?
**Short Answer:** Use regmap for register-oriented devices that benefit from common access policy, bit updates, locking, cache, or bus-independent code. Use raw helpers for irregular, streaming, or very simple protocols.

**Deep Explanation:** Regmap reduces repeated code and centralizes correctness for register maps. It is a poor fit if the protocol is mostly command streams, large FIFO transfers, or unusual transactions that do not map cleanly to register reads/writes. A senior decision weighs maintainability, datasheet fit, performance, cache correctness, and framework expectations.

**API / Code Anchor:**
```c
/* Good regmap-style operation */
regmap_update_bits(map, REG_POWER, POWER_EN, POWER_EN);

/* Raw bus may be clearer for unusual streaming commands */
spi_sync(spi, &custom_message);
```

**Production or Debugging Angle:** Regmap is often expected in PMIC, codec, regulator, MFD, syscon, and sensor code, but forcing it onto the wrong protocol can hide complexity rather than reduce it.

**Common Traps:** Choosing regmap because it sounds modern, not because the device is actually register-map shaped.

**Follow-up Questions:**
- What makes a device a bad regmap fit?
- How would you handle a chip with both registers and a streaming FIFO?
- Why do many ASoC codec drivers use regmap?

### 11. How do you keep regcache coherent across suspend, reset, or runtime PM?
**Short Answer:** You must know whether hardware state is preserved. If it is lost or inaccessible, prevent unsafe access, mark cache dirty, use cache-only/bypass modes appropriately, and sync or reinitialize after resume.

**Deep Explanation:** Cache coherence is a contract between driver and hardware. If power is removed, cached values may no longer match registers. If the driver reads from cache after reset, it can believe hardware is configured when it is not. Volatile registers should not be cached, and registers changed by firmware, bootloader, hardware, or another driver must be treated carefully.

**API / Code Anchor:**
```c
/* Exact sequencing must be validated for the target driver/kernel. */
regcache_cache_only(map, true);
regcache_mark_dirty(map);

/* After power is restored and device is accessible: */
regcache_cache_only(map, false);
ret = regcache_sync(map);
```

**Production or Debugging Angle:** PM-related cache bugs often appear only after suspend/resume or runtime autosuspend, not at initial boot.

**Common Traps:** Enabling cache for convenience but never thinking through hardware reset, regulator disable, runtime PM, or bootloader-modified registers.

**Follow-up Questions:**
- What registers should remain volatile even with cache enabled?
- When would reinitialization be safer than cache sync?
- How can stale cache mimic a successful register read?

### 12. A debugfs regmap dump clears an interrupt status register. What went wrong?
**Short Answer:** A side-effect register was treated as safe to read. It should have been marked precious or excluded from casual dumping.

**Deep Explanation:** Some hardware registers clear on read, pop a FIFO, acknowledge events, or expose sensitive state. Regmap debugfs can be very useful, but only if access policy tells regmap which registers are unsafe for casual reads. `precious_reg` exists for this class of problem.

**API / Code Anchor:**
```c
static bool my_precious_reg(struct device *dev, unsigned int reg)
{
    return reg == REG_IRQ_STATUS_CLEAR_ON_READ ||
           reg == REG_FIFO_POP;
}
```

**Production or Debugging Angle:** Debug tooling must not perturb the hardware state you are trying to inspect. Document side-effect registers in code comments.

**Common Traps:** Marking the register volatile but not precious. Volatile prevents stale cache; it does not mean "safe to dump."

**Follow-up Questions:**
- What is a clear-on-read register?
- What is the difference between volatile and precious?
- How would you debug such a register safely?

### 13. What lifetime issues can happen with devm-managed regmap?
**Short Answer:** Devm frees the map at device teardown, but the driver must still stop all users before teardown: subsystem callbacks, IRQs, workqueues, PM paths, and child devices.

**Deep Explanation:** Managed allocation solves release mechanics, not concurrency. If an IRQ thread, delayed work item, child device, or subsystem callback can access the map after remove begins, devm does not save you. The driver must unregister users, disable IRQs/work, and order cleanup so no path can touch the regmap after the device is gone.

**API / Code Anchor:**
```c
/* Good remove ordering idea */
unregister_subsystem_users(chip);
cancel_work_sync(&chip->work);
disable_irq(chip->irq);
/* devm later releases chip->map */
```

**Production or Debugging Angle:** Use-after-free or late access bugs may appear only during driver unbind, module unload, runtime PM, or error paths.

**Common Traps:** Believing `devm_regmap_init_*()` means remove ordering no longer matters.

**Follow-up Questions:**
- What paths might still use `chip->map` after remove starts?
- When is unmanaged `regmap_init_*()` appropriate?
- How do child MFD devices affect lifetime?

### 14. How are regmap IRQ, MFD, and syscon related to core regmap?
**Short Answer:** They build on regmap but solve different problems: regmap IRQ manages interrupt-controller registers, MFD splits multifunction devices, and syscon shares MMIO register blocks.

**Deep Explanation:** Core regmap handles register access, cache, locking, and formatting. Regmap IRQ uses a regmap to manage status/mask/ack/type registers for interrupt sources. MFD core may use a parent regmap shared by child devices. Syscon exposes miscellaneous SoC MMIO registers as a regmap that other drivers can look up.

**API / Code Anchor:**
```c
/* Core regmap */
regmap_update_bits(map, REG_CTRL, MASK, val);

/* Related, separate topic */
devm_regmap_add_irq_chip(dev, map, irq, flags, base, chip, &data);
```

**Production or Debugging Angle:** Keep the boundaries clear. Do not explain every MFD/syscon/IRQ detail when asked about core regmap, but know why the topics connect.

**Common Traps:** Treating regmap IRQ as mandatory for any regmap-using device. Many regmap drivers have no IRQ controller role.

**Follow-up Questions:**
- What does syscon provide?
- What does regmap IRQ add on top of core regmap?
- Why do MFD children often share register access?

## Debugging Scenarios And Common Traps

### Scenario 1: Probe succeeds, but all control writes return `-EIO`
**Short Answer:** The writable policy or register range is probably rejecting the writes, or the bus is failing.

**Deep Explanation:** Check whether `max_register` includes the register and whether `writeable_reg()` or `wr_table` marks it writable. If the policy accepts it, then inspect the bus transaction.

**API / Code Anchor:**
```c
ret = regmap_write(map, REG_CTRL, val);
if (ret)
    dev_err(dev, "REG_CTRL write failed: %d\n", ret);
```

**Production or Debugging Angle:** Add temporary logging in policy callbacks and read a stable ID register to separate policy failure from physical bus failure.

**Common Traps:** Assuming every `-EIO` is electrical or bus-level.

**Follow-up Questions:**
- What would `-EINVAL` suggest?
- How can `max_register` cause this?
- What makes a good first register to read?

### Scenario 2: Status register never changes after enabling cache
**Short Answer:** The status register is probably being served from cache because it was not marked volatile.

**Deep Explanation:** Hardware-updated registers must usually be volatile. Otherwise, regmap may return the cached reset or previous value instead of reading hardware. This commonly affects status, IRQ pending, conversion result, FIFO count, and data registers.

**API / Code Anchor:**
```c
static bool my_volatile_reg(struct device *dev, unsigned int reg)
{
    return reg == REG_STATUS || reg == REG_DATA;
}
```

**Production or Debugging Angle:** Stale cache bugs are easy to misdiagnose as hardware failures.

**Common Traps:** Disabling the entire cache instead of correctly marking the small set of volatile registers.

**Follow-up Questions:**
- What registers are good cache candidates?
- What registers are bad cache candidates?
- How can runtime PM make this worse?

### Scenario 3: A SPI regmap reads the wrong register values
**Short Answer:** Check SPI mode, register/value widths, endian settings, and read/write flag masks.

**Deep Explanation:** SPI devices often encode read/write direction in the high bit of the address or command byte. If `read_flag_mask` or `write_flag_mask` is wrong, regmap will format transactions incorrectly. Wrong `reg_bits`, `val_bits`, or endianness can also corrupt every access.

**API / Code Anchor:**
```c
static const struct regmap_config spi_cfg = {
    .reg_bits = 8,
    .val_bits = 8,
    .read_flag_mask = 0x80,
};
```

**Production or Debugging Angle:** A logic analyzer is often the fastest way to prove whether regmap is putting the expected address and direction bits on the wire.

**Common Traps:** Reusing an I2C config for a SPI variant without adding SPI-specific flags.

**Follow-up Questions:**
- What does `read_flag_mask` do?
- Why does SPI mode still matter under regmap?
- How would you validate the first transaction?

### Scenario 4: The driver works at boot but fails after suspend/resume
**Short Answer:** The cache and hardware state are likely out of sync, or register access happens while the device is powered off.

**Deep Explanation:** During suspend or runtime PM, hardware may lose register contents. If the driver reads cached values or skips reinitialization after resume, software and hardware diverge. If regmap access happens before regulators/clocks/reset are restored, transfers fail or read garbage.

**API / Code Anchor:**
```c
regcache_mark_dirty(map);
ret = regcache_sync(map);
```

**Production or Debugging Angle:** PM bugs require testing actual suspend/resume and runtime autosuspend, not only boot-time probe.

**Common Traps:** Assuming devm or regmap automatically understands power loss.

**Follow-up Questions:**
- When should cache be marked dirty?
- What order should power and cache sync follow?
- Which registers should be re-read instead of restored?

