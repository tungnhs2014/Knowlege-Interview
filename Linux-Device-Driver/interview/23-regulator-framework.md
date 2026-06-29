# 23 - Regulator Framework Interview Questions

Strong candidates should be able to reason about power rails, not just recite API names. Good answers connect Device Tree `*-supply` properties, regulator constraints, consumer enable/disable lifetime, PMIC provider registration, shared rails, and debugging symptoms on real boards.

## Beginner

### 1. What problem does the regulator framework solve?

**Short Answer:**  
It gives Linux a common way to describe, enable, disable, and configure device power supplies such as buck converters, LDOs, fixed regulators, and PMIC outputs.

**Deep Explanation:**  
Different boards and PMICs control power rails in different ways: I2C registers, GPIOs, PWM outputs, fixed always-on rails, or multi-function PMIC cells. Ordinary drivers should not know those hardware details. They should request a named supply and ask the regulator core to enable it, set safe voltage/current limits, or disable it later.

**API / Code Anchor:**  
`struct regulator *`, `devm_regulator_get()`, `regulator_enable()`, `regulator_disable()`, `struct regulator_desc`, `devm_regulator_register()`.

**Production or Debugging Angle:**  
If a device never responds on I2C/SPI/MMIO, check whether its required rails were declared and enabled before register access.

**Common Traps:**  
- Thinking regulators are only for PMIC drivers.
- Assuming bootloader-enabled rails mean the Linux driver is correct.
- Treating regulator sysfs as the normal control path for a device driver.

**Follow-up Questions:**  
- What is a regulator consumer?
- What is a regulator provider?
- Why do constraints matter?

### 2. What is the difference between a regulator provider and a regulator consumer?

**Short Answer:**  
A provider exposes power outputs to the regulator core. A consumer is a driver that requests and uses those power outputs.

**Deep Explanation:**  
Provider drivers own the hardware that supplies power: a PMIC, fixed regulator, GPIO switch, or PWM regulator. They register each output with the regulator core and implement callbacks. Consumer drivers are ordinary device drivers, such as camera, MMC, USB, display, audio, or CAN drivers, that need those supplies to operate.

**API / Code Anchor:**  
Provider: `struct regulator_desc`, `struct regulator_ops`, `struct regulator_config`, `devm_regulator_register()`.  
Consumer: `struct regulator *`, `devm_regulator_get()`, `regulator_enable()`.

**Production or Debugging Angle:**  
`-EPROBE_DEFER` from `devm_regulator_get()` often means the consumer exists, but the provider has not registered yet.

**Common Traps:**  
- Using provider APIs in an ordinary device driver.
- Depending on internal `struct regulator` fields in consumer code.
- Forgetting one PMIC driver may expose many regulator outputs.

**Follow-up Questions:**  
- What object does a provider register?
- Why is `struct regulator *` opaque to consumers?

### 3. In Device Tree, what does `vdd-supply = <&reg_3v3>;` mean?

**Short Answer:**  
It means the consumer input named `vdd` is powered by the regulator node labeled `reg_3v3`.

**Deep Explanation:**  
The property name has two parts: the consumer-local name and the `-supply` suffix. A driver requesting `devm_regulator_get(dev, "vdd")` maps to the `vdd-supply` property. The phandle points to the provider regulator node. The provider may have its own `regulator-name`, but that is not what the consumer normally passes to `devm_regulator_get()`.

**API / Code Anchor:**  
```dts
vdd-supply = <&reg_3v3>;
```

```c
vdd = devm_regulator_get(dev, "vdd");
```

**Production or Debugging Angle:**  
If lookup fails, compare the driver string with the property prefix before `-supply`.

**Common Traps:**  
- Calling `devm_regulator_get(dev, "reg_3v3")`.
- Calling `devm_regulator_get(dev, "board_3v3")` because that is `regulator-name`.
- Changing the driver lookup string instead of fixing a wrong binding.

**Follow-up Questions:**  
- What is `regulator-name` used for?
- Why are supply names local to the consumer?

### 4. What do `regulator-always-on` and `regulator-boot-on` mean?

**Short Answer:**  
`always-on` means the rail should not be disabled during normal system operation. `boot-on` means firmware left it enabled, or it should be enabled when constraints are applied.

**Deep Explanation:**  
These are board constraints, not driver shortcuts. `always-on` is used for rails that must remain active for platform safety or shared system operation. `boot-on` preserves or establishes an initial enabled state, often because the bootloader needed the rail. A real consumer driver should still declare and manage the supplies it owns.

**API / Code Anchor:**  
`regulator-always-on`, `regulator-boot-on`, `struct regulation_constraints`.

**Production or Debugging Angle:**  
A board that works only because of `boot-on` may still have a missing consumer supply in the driver or DT.

**Common Traps:**  
- Using `boot-on` to hide missing consumer power management.
- Trying to disable an `always-on` shared system rail.
- Assuming `boot-on` means the rail is dedicated to your device.

**Follow-up Questions:**  
- When is `always-on` appropriate?
- How would you review a patch that adds `regulator-always-on` to fix a timeout?

## Mid-Level

### 5. Show a safe probe flow for one required regulator and one clock.

**Short Answer:**  
Get the regulator, configure it if needed, enable it, get/enable the clock, and if the clock step fails, disable the regulator before returning.

**Deep Explanation:**  
Managed acquisition only releases the regulator handle; it does not undo a successful `regulator_enable()`. Once the rail is enabled, every later failure path must disable it. The device should not be accessed until power and required clocks/resets are in the correct state.

**API / Code Anchor:**  
```c
vdd = devm_regulator_get(dev, "vdd");
if (IS_ERR(vdd))
    return dev_err_probe(dev, PTR_ERR(vdd), "vdd\n");

ret = regulator_enable(vdd);
if (ret)
    return ret;

clk = devm_clk_get(dev, "xclk");
if (IS_ERR(clk)) {
    ret = PTR_ERR(clk);
    goto err_disable_vdd;
}

ret = clk_prepare_enable(clk);
if (ret)
    goto err_disable_vdd;

return 0;

err_disable_vdd:
regulator_disable(vdd);
return ret;
```

**Production or Debugging Angle:**  
Probe failures that leave supplies on may waste power, confuse later probes, or keep hardware partially alive.

**Common Traps:**  
- Thinking `devm_regulator_get()` manages enable state.
- Touching registers before the rail is enabled.
- Forgetting settle delays or reset sequencing from the datasheet.

**Follow-up Questions:**  
- How would remove look?
- When would `devm_regulator_get_enable()` be acceptable?

### 6. What are regulator constraints, and why are they important?

**Short Answer:**  
Constraints define the safe voltage, current, mode, status, and suspend behavior for a regulator on a specific board.

**Deep Explanation:**  
The same PMIC output may be wired differently on different boards. The provider may be capable of a broad voltage range, but the board and connected device may only allow a smaller range. Constraints prevent consumers from requesting unsafe voltage/current levels or unsupported operations. They also represent policy such as `always-on`, `boot-on`, and allowed mode/status changes.

**API / Code Anchor:**  
`struct regulation_constraints`, `struct regulator_init_data`, DT properties such as `regulator-min-microvolt`, `regulator-max-microvolt`, `regulator-min-microamp`, `regulator-max-microamp`.

**Production or Debugging Angle:**  
If `regulator_set_voltage()` fails, inspect both provider capability and board constraints.

**Common Traps:**  
- Treating provider capability as board permission.
- Writing constraints just to silence errors without checking the schematic.
- Requesting exact voltage without checking return value.

**Follow-up Questions:**  
- Where do constraints come from on a DT system?
- How do constraints differ from provider hardware limits?

### 7. How should a driver handle optional regulators?

**Short Answer:**  
Use optional helpers only when the hardware truly can operate without that supply on some boards.

**Deep Explanation:**  
An optional supply is not a way to ignore a broken DT. It means the device design has a valid mode where that rail is absent or internally supplied. For required rails, lookup failure should fail or defer probe. With `devm_regulator_get_optional()`, a missing optional supply is reported as `-ENODEV`; driver code usually normalizes that case to `NULL`, while other errors such as `-EPROBE_DEFER` remain real probe errors.

**API / Code Anchor:**  
```c
iovdd = devm_regulator_get_optional(dev, "iovdd");
if (IS_ERR(iovdd)) {
    ret = PTR_ERR(iovdd);
    if (ret == -ENODEV)
        iovdd = NULL;
    else
        return dev_err_probe(dev, ret, "iovdd\n");
}
```

`devm_regulator_get_optional()`, `-ENODEV`, `-EPROBE_DEFER`, `regulator_enable()`, `regulator_disable()`.

**Production or Debugging Angle:**  
Review optional regulators against the datasheet and board schematic, not just one board that happens to work.

**Common Traps:**  
- Making a required supply optional to avoid `-EPROBE_DEFER`.
- Calling `regulator_enable()` on a missing optional handle.
- Treating `-EPROBE_DEFER` like an absent optional supply.
- Failing to test the board variant where the supply is absent.

**Follow-up Questions:**  
- What should the driver do if a required supply returns `-EPROBE_DEFER`?
- How would you document optional supply behavior in a binding?

### 8. What should you check when `devm_regulator_get(dev, "vdd")` fails?

**Short Answer:**  
Check the DT property `vdd-supply`, provider node, provider driver probe status, constraints, and the exact error code.

**Deep Explanation:**  
The failure may come from a missing property, spelling mismatch, disabled provider node, provider driver not loaded, provider still probing, or invalid regulator binding. The local lookup string must match the consumer supply property prefix. `-EPROBE_DEFER` is common when the PMIC/regulator provider is not ready.

**API / Code Anchor:**  
`devm_regulator_get()`, `dev_err_probe()`, `vdd-supply`, `regulator-name`, `-EPROBE_DEFER`.

**Production or Debugging Angle:**  
Use `dev_err_probe()` so deferred probes are reported cleanly and repeated logs are less noisy.

**Common Traps:**  
- Looking for `regulator-name = "vdd"` instead of `vdd-supply`.
- Treating all failures as optional.
- Ignoring disabled parent PMIC nodes.

**Follow-up Questions:**  
- How can sysfs or debugfs help after the provider registers?
- What log would you expect for dummy supply fallback?

### 9. How do regulator bulk APIs help?

**Short Answer:**  
They simplify handling several supplies that share the same lifecycle.

**Deep Explanation:**  
Devices often need multiple rails, such as `avdd`, `dvdd`, and `iovdd`. Bulk APIs let the driver acquire and enable a group consistently. They reduce repeated boilerplate and can make unwind paths clearer. They are best when the supplies have the same enable/disable timing. If rails need strict per-rail sequencing and delays, explicit ordering may be better.

**API / Code Anchor:**  
`struct regulator_bulk_data`, `devm_regulator_bulk_get()`, `regulator_bulk_enable()`, `regulator_bulk_disable()`.

**Production or Debugging Angle:**  
Bulk helpers reduce missing-unwind bugs, but they do not replace datasheet sequencing.

**Common Traps:**  
- Using bulk enable when rails require distinct delays or ordering.
- Forgetting that bulk acquisition does not automatically enable supplies.
- Mixing optional and required supplies carelessly.

**Follow-up Questions:**  
- How would you handle one optional rail in a mostly bulk-managed driver?
- When are managed bulk get-enable helpers appropriate?

## Senior

### 10. Why might `regulator_disable()` not physically turn a rail off?

**Short Answer:**  
The regulator may be shared, constrained as always-on, enabled by another consumer, or controlled by provider policy.

**Deep Explanation:**  
The regulator core tracks usage and constraints. A consumer disable balances that consumer's enable, but the core must not shut off a rail still needed elsewhere. Shared regulators, `always-on` constraints, boot-critical rails, or provider-specific behavior can prevent physical disable. This is a feature, not a bug.

**API / Code Anchor:**  
`regulator_enable()`, `regulator_disable()`, `regulator_is_enabled()`, `regulator-always-on`, `struct regulation_constraints`.

**Production or Debugging Angle:**  
If power measurements show a rail remains on after remove, inspect consumers and constraints before blaming the driver.

**Common Traps:**  
- Expecting disable to always drop the voltage.
- Using `regulator_force_disable()` in normal cleanup.
- Forgetting that debug state may reflect provider policy and shared users.

**Follow-up Questions:**  
- When is `regulator_force_disable()` appropriate?
- How would you identify other consumers of the same rail?

### 11. How would you write a regulator provider for an I2C PMIC output?

**Short Answer:**  
Use regmap for register access, describe each output with `struct regulator_desc`, implement regulator ops, parse constraints from DT, and register outputs with `devm_regulator_register()`.

**Deep Explanation:**  
An I2C PMIC may expose multiple buck and LDO regulators, often as part of an MFD. The provider driver owns the PMIC register map and implements operations such as enable/disable, voltage selector mapping, status/error reporting, and current limits. Board constraints come from DT regulator child nodes. The provider should not expose raw PMIC register details to consumers.

**API / Code Anchor:**  
`struct regulator_desc`, `struct regulator_ops`, `struct regulator_config`, `devm_regulator_register()`, `rdev_get_drvdata()`, `regmap_update_bits()`, `regulators` DT subnode.

**Production or Debugging Angle:**  
Provider bugs affect every consumer of that rail. Review locking, regmap error handling, selector tables, voltage ranges, and constraint parsing carefully.

**Common Traps:**  
- Using stale manual `regulator_register()` examples without a reason.
- Confusing DT labels with runtime regulator names.
- Implementing voltage selector math incorrectly.
- Ignoring PMIC interrupt/error flags.

**Follow-up Questions:**  
- How does this relate to MFD cells?
- What should be in the YAML binding?

### 12. Debugging scenario: a camera sensor works after bootloader init but fails on cold Linux boot. What do you suspect?

**Short Answer:**  
The Linux driver or DT likely misses a required supply, enable sequence, delay, clock/reset ordering, or voltage constraint.

**Deep Explanation:**  
Bootloaders often leave rails and clocks enabled. A broken Linux driver can appear to work when it inherits that state, but fail on cold boot or after runtime suspend. The fix is to describe every required supply in DT, request those supplies in the sensor driver, enable them in datasheet order, wait for stabilization, then enable clocks and release reset before register access.

**API / Code Anchor:**  
`devm_regulator_get()`, `regulator_enable()`, `regulator_set_voltage()`, `clk_prepare_enable()`, reset GPIO APIs, `*-supply` properties.

**Production or Debugging Angle:**  
Measure rails with a scope or meter and compare against driver logs. Software state alone may not prove the rail is physically correct.

**Common Traps:**  
- Adding `regulator-always-on` as a workaround.
- Relying on probe order rather than declaring dependencies.
- Enabling clock/reset before rails are stable.

**Follow-up Questions:**  
- What would you check in dmesg?
- How would runtime PM expose the same bug?

### 13. Why is changing voltage or mode on a shared regulator risky?

**Short Answer:**  
Because the same rail may feed multiple devices, and a voltage or mode change for one consumer can violate another consumer's requirements.

**Deep Explanation:**  
Regulators are board-level resources. A rail named `vdd_3v3` may feed several devices. Even if one driver can request a lower voltage or different mode, another device may require the old level. Constraints prevent some unsafe changes, but drivers still need subsystem policy and schematic awareness. Load requests are usually safer than direct mode control when sharing exists.

**API / Code Anchor:**  
`regulator_set_voltage()`, `regulator_set_load()`, `regulator_set_mode()`, `struct regulation_constraints`.

**Production or Debugging Angle:**  
Intermittent failures can appear only when another device probes, suspends, or changes performance state.

**Common Traps:**  
- Assuming a supply is private because the driver uses a local name.
- Calling `regulator_set_mode()` directly on a shared rail.
- Ignoring OPP or subsystem policy.

**Follow-up Questions:**  
- How would you determine whether a rail is shared?
- What role do constraints play here?

### 14. How do regulators interact with runtime PM?

**Short Answer:**  
Runtime resume usually enables supplies before register access; runtime suspend quiesces hardware before disabling supplies.

**Deep Explanation:**  
Regulators are part of a device's active power state. A runtime-suspended device may lose register contents or become inaccessible. The driver must prevent I/O while rails are off, restore state after resume when needed, and order regulators with clocks, resets, pinctrl, GPIOs, and power domains according to the datasheet.

**API / Code Anchor:**  
Runtime PM callbacks, `regulator_enable()`, `regulator_disable()`, `regulator_bulk_enable()`, `regulator_bulk_disable()`.

**Production or Debugging Angle:**  
If a device works at probe but fails after idle, inspect runtime suspend/resume power ordering and register restore logic.

**Common Traps:**  
- Disabling supplies while IRQ, DMA, or debug paths can still touch registers.
- Forgetting that power loss may clear hardware state.
- Letting devm cleanup replace explicit runtime PM balancing.

**Follow-up Questions:**  
- Where do clocks and resets fit in the order?
- How would you block new I/O while power is off?

### 15. What should a code review look for in regulator consumer code?

**Short Answer:**  
Review supply naming, required versus optional rails, voltage/current constraints, enable/disable pairing, sequencing, shared-rail safety, and PM/error paths.

**Deep Explanation:**  
Regulator bugs are often lifecycle and board-description bugs. The code may compile and work on one board, but fail on another if rails are absent, named differently, shared, or sequenced differently. Review must compare the driver, binding, board DTS, datasheet, and subsystem lifecycle.

**API / Code Anchor:**  
`devm_regulator_get()`, `devm_regulator_get_optional()`, `regulator_set_voltage()`, `regulator_enable()`, `regulator_disable()`, `regulator_bulk_*()`.

**Production or Debugging Angle:**  
Require evidence that every enabled rail is disabled on all failure paths, and that optional supplies are justified by hardware.

**Common Traps:**  
- Missing unwind after later clock/reset/register failures.
- Copying supply names from another chip.
- Ignoring startup delays.
- Using `always-on` or optional helpers to paper over probe failures.

**Follow-up Questions:**  
- What would you ask the hardware engineer?
- What tests would you run on suspend/resume and probe failure?

### 16. What is the relationship among PMIC, MFD, regmap, and regulator drivers?

**Short Answer:**  
A PMIC is often a multi-function chip; MFD splits it into child devices, regmap abstracts register access, and the regulator child exposes PMIC power outputs through the regulator framework.

**Deep Explanation:**  
Many PMICs provide regulators, RTC, GPIO, watchdog, and IRQ functions. The parent MFD driver may own the I2C/SPI client and regmap. Child cells bind to function drivers such as `*-regulator`. The regulator provider then registers each buck/LDO output with the regulator core. Consumers never need to know the PMIC register layout.

**API / Code Anchor:**  
`struct mfd_cell`, `devm_mfd_add_devices()`, `struct regmap`, `devm_regulator_register()`, `struct regulator_desc`.

**Production or Debugging Angle:**  
If regulator consumers defer forever, check not only the regulator child driver but also parent PMIC probe, regmap setup, IRQ setup, and MFD child creation.

**Common Traps:**  
- Putting all PMIC behavior into one giant driver without framework boundaries.
- Debugging the consumer while the MFD parent never created the regulator child.
- Confusing syscon/shared-register access with a regulator provider API.

**Follow-up Questions:**  
- What belongs in the parent MFD driver?
- What belongs in the regulator child driver?
