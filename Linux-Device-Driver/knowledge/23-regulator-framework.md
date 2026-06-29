# 23 - Regulator Framework

## Learning Goal
Understand how Linux models controllable power rails, how ordinary drivers consume supplies safely, and what changes when you write a regulator provider for a PMIC, fixed regulator, GPIO-controlled rail, or similar power source.

After this topic you should be able to:

- Explain **regulator provider versus regulator consumer**.
- Read a Device Tree supply reference such as `vdd-supply = <&reg_3v3>`.
- Use `devm_regulator_get()`, `regulator_enable()`, and `regulator_disable()` without leaking enabled rails.
- Know when voltage, current, load, and mode APIs are appropriate.
- Understand how constraints protect hardware from unsafe consumer requests.
- Recognize PMIC/MFD/regmap context around regulator providers.
- Debug missing supplies, probe deferral, bootloader-left-on power, and shared-rail surprises.

## Why This Matters In Real Work
Many embedded devices fail silently if their power rails are missing or sequenced incorrectly. A sensor may not answer on I2C, a display may stay black, a Wi-Fi chip may time out, or an MMC card may never enumerate.

You see regulators in real drivers when:

- An MMC host needs `vmmc` and maybe `vqmmc`.
- A camera sensor needs analog, digital, and I/O rails before clock/reset release.
- An audio codec declares `VDDA-supply`, `VDDIO-supply`, or `VDDD-supply`.
- A CAN controller needs `vdd` and transceiver power.
- A USB controller or connector controls `vbus`.
- A display panel has an LCD or backlight supply.
- CPUfreq or device DVFS uses frequency/voltage operating points.

Without the regulator framework, every board or PMIC driver would expose ad hoc power-control APIs. With it, ordinary drivers can request named supplies while provider drivers hide the PMIC registers, GPIOs, PWM control, or fixed-rail details.

## Mental Model
A **regulator** is a controllable power rail. It may be a buck converter, LDO, fixed voltage source, GPIO-controlled switch, PWM-driven regulator, or one output of a PMIC.

Think in three roles:

| Role | What It Means | Typical Code |
| --- | --- | --- |
| Provider | Owns the hardware that supplies power. | PMIC, fixed regulator, GPIO/PWM regulator driver. |
| Consumer | Device that needs the power rail. | Sensor, MMC, audio codec, display, USB, CAN. |
| Constraints | Board-safe rules for that rail. | Min/max voltage/current, allowed modes, boot/always-on flags. |

Device Tree connects them with a supply property:

```dts
camera@40 {
    compatible = "vendor,camera";
    reg = <0x40>;
    vdd-supply = <&reg_3v3>;
};
```

The driver requests the local supply name:

```c
vdd = devm_regulator_get(dev, "vdd");
```

The driver asks for `"vdd"` because the property is `vdd-supply`. It does not ask for the provider's `regulator-name`.

## Core Concepts
Regulators are power resources with policy. The framework is not just an on/off switch; it also enforces board limits and coordinates shared rails.

| Concept | Meaning |
| --- | --- |
| Supply name | Consumer-local name, such as `"vdd"`, `"vmmc"`, `"avdd"`, or `"xceiver"`. |
| `regulator-name` | Provider/output descriptive name, useful for debug and bindings, not the consumer lookup string. |
| Constraint | Safe allowed range or operation mask for a rail. |
| `always-on` | Rail should not be disabled while the system is running. |
| `boot-on` | Firmware left the rail enabled, or it should be enabled when constraints are applied. |
| Shared regulator | One rail feeds multiple consumers; disable/rate-like changes must account for other users. |
| Fixed regulator | Rail with fixed voltage, often board-level and simple to describe in DT. |
| PMIC regulator | One output from a multi-function power-management chip, often I2C/SPI + regmap. |

Useful comparisons:

| Regulator | Clock | Reset |
| --- | --- | --- |
| Supplies voltage/current. | Supplies timing signal. | Holds hardware in/out of reset. |
| Consumer calls `regulator_enable()`. | Consumer calls `clk_prepare_enable()`. | Consumer calls reset assert/deassert APIs. |
| Constraints protect voltage/current. | Parent/rate topology constrains frequency. | Reset polarity/timing is hardware-specific. |
| Enabling may require ramp/startup delay. | Enabling may require prepare/enable split. | Deasserting too early can break probe. |

## Kernel Mechanism
The regulator core lives between consumers and provider hardware. Consumers receive opaque `struct regulator *` handles; providers register `struct regulator_dev` instances with descriptions and callbacks.

### Consumer Lookup
For a DT-backed device:

```dts
sensor@40 {
    compatible = "vendor,my-sensor";
    reg = <0x40>;
    avdd-supply = <&reg_2v8>;
    dovdd-supply = <&reg_1v8>;
};
```

The driver does:

```c
avdd = devm_regulator_get(dev, "avdd");
dovdd = devm_regulator_get(dev, "dovdd");
```

The core:

1. Builds the property name from the requested ID, for example `avdd-supply`.
2. Finds the provider node referenced by that property.
3. Finds the registered regulator provider/output.
4. Returns a consumer `struct regulator *`.

If the provider has not registered yet, lookup may return `-EPROBE_DEFER`.

### Constraints
Constraints are board policy applied by firmware description or machine data. They answer: what is safe on this board?

Examples:

```dts
reg_3v3: regulator-3v3 {
    compatible = "regulator-fixed";
    regulator-name = "board_3v3";
    regulator-min-microvolt = <3300000>;
    regulator-max-microvolt = <3300000>;
    regulator-always-on;
};
```

Important rules:

- Consumers cannot request voltage/current outside constraints.
- Not every consumer is allowed to change status, mode, voltage, or current.
- `always-on` rails are not normal shutdown candidates.
- `boot-on` means the initial state matters; it is not a replacement for a real consumer driver.

### Provider Registration
A provider driver describes each output and registers it:

```text
provider probe()
  allocate private PMIC/provider state
  initialize regmap or MMIO/GPIO/PWM control
  parse regulator child nodes and constraints
  initialize struct regulator_desc for each output
  fill struct regulator_config
  devm_regulator_register()
```

Provider callbacks are hardware-specific. For example:

- A fixed regulator may only list one voltage.
- A GPIO regulator may enable/disable through a GPIO.
- A PMIC buck regulator may use regmap to set voltage selectors.
- A current-limited USB supply may support current-limit callbacks.

## Key Structs And APIs
The API is easiest to remember by role.

### Consumer APIs
Use these in ordinary device drivers:

```c
struct regulator *devm_regulator_get(struct device *dev, const char *id);
struct regulator *devm_regulator_get_optional(struct device *dev,
                                              const char *id);

int regulator_enable(struct regulator *regulator);
int regulator_disable(struct regulator *regulator);
int regulator_is_enabled(struct regulator *regulator);

int regulator_set_voltage(struct regulator *regulator,
                          int min_uV, int max_uV);
int regulator_get_voltage(struct regulator *regulator);

int regulator_set_current_limit(struct regulator *regulator,
                                int min_uA, int max_uA);
int regulator_get_current_limit(struct regulator *regulator);

int regulator_set_load(struct regulator *regulator, int load_uA);
int regulator_set_mode(struct regulator *regulator, unsigned int mode);
unsigned int regulator_get_mode(struct regulator *regulator);
```

For several supplies with the same lifecycle:

```c
struct regulator_bulk_data supplies[] = {
    { .supply = "avdd" },
    { .supply = "dovdd" },
};

ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(supplies), supplies);
ret = regulator_bulk_enable(ARRAY_SIZE(supplies), supplies);
regulator_bulk_disable(ARRAY_SIZE(supplies), supplies);
```

Some modern kernels also provide managed convenience helpers such as `devm_regulator_get_enable()` and managed bulk get-enable helpers. Use them only when the regulator should stay enabled for the whole device lifetime and your target kernel supports them.

### Provider APIs
Use these in regulator provider drivers:

```c
struct regulator_desc;
struct regulator_ops;
struct regulator_config;
struct regulator_dev;
struct regulation_constraints;
struct regulator_init_data;

struct regulator_dev *devm_regulator_register(
        struct device *dev,
        const struct regulator_desc *regulator_desc,
        const struct regulator_config *config);
```

Common provider callbacks include:

```c
int (*enable)(struct regulator_dev *rdev);
int (*disable)(struct regulator_dev *rdev);
int (*is_enabled)(struct regulator_dev *rdev);

int (*list_voltage)(struct regulator_dev *rdev, unsigned selector);
int (*map_voltage)(struct regulator_dev *rdev, int min_uV, int max_uV);
int (*set_voltage_sel)(struct regulator_dev *rdev, unsigned selector);
int (*get_voltage_sel)(struct regulator_dev *rdev);

int (*set_current_limit)(struct regulator_dev *rdev,
                         int min_uA, int max_uA);
int (*get_current_limit)(struct regulator_dev *rdev);
```

Provider code should normally treat `struct regulator_dev` as a framework object and use helper accessors such as `rdev_get_drvdata()` and `rdev_get_id()`.

### Device Tree Properties
Provider side:

```dts
pmic@58 {
    compatible = "vendor,my-pmic";
    reg = <0x58>;

    regulators {
        buck1: buck1 {
            regulator-name = "BUCK1";
            regulator-min-microvolt = <850000>;
            regulator-max-microvolt = <1600000>;
            regulator-boot-on;
        };

        ldo1: ldo1 {
            regulator-name = "LDO1";
            regulator-min-microvolt = <3300000>;
            regulator-max-microvolt = <3300000>;
            regulator-always-on;
        };
    };
};
```

Consumer side:

```dts
device@1000 {
    compatible = "vendor,my-device";
    reg = <0x1000 0x100>;
    vdd-supply = <&ldo1>;
};
```

Driver side:

```c
vdd = devm_regulator_get(dev, "vdd");
```

## Lifecycle / Data Flow
Regulator handling is a lifetime and sequencing problem. Acquire handles early, enable only when hardware needs power, and unwind exactly.

### Consumer Probe Flow

```text
probe()
  allocate private data
  get regulators by local supply names
  get clocks/resets/GPIOs/pinctrl
  set voltage/current/load if required
  enable regulators
  wait for required startup/ramp delay if not handled by provider
  deassert reset / enable clocks as required by datasheet
  touch registers / identify hardware
  register subsystem device
```

### Consumer Error / Remove Flow

```text
later probe step fails or remove()
  stop users and subsystem activity
  stop DMA/IRQ/hardware transactions
  put hardware in safe state
  disable clocks/resets as appropriate
  regulator_disable() for every successful regulator_enable()
  devm releases regulator handles after cleanup returns
```

### Runtime PM Flow

```text
runtime_resume()
  enable supplies
  wait/settle if required
  enable clocks / deassert reset
  restore registers if power was lost

runtime_suspend()
  quiesce hardware
  stop register/DMA/IRQ access
  assert reset or disable clocks if needed
  disable supplies if the device may power down
```

Deep PM policy belongs in the power-management topic, but one rule fits everywhere: do not access registers on an unpowered device.

## Minimal Practical Example
This is **learning-only pseudo-code**, not a complete production driver. It shows a consumer with one required core rail and one optional I/O rail.

```c
struct demo_sensor {
    struct regulator *vdd;
    struct regulator *iovdd;
    struct clk *xclk;
};

static int demo_sensor_probe(struct i2c_client *client)
{
    struct device *dev = &client->dev;
    struct demo_sensor *sensor;
    int ret;

    sensor = devm_kzalloc(dev, sizeof(*sensor), GFP_KERNEL);
    if (!sensor)
        return -ENOMEM;

    sensor->vdd = devm_regulator_get(dev, "vdd");
    if (IS_ERR(sensor->vdd))
        return dev_err_probe(dev, PTR_ERR(sensor->vdd),
                             "failed to get vdd supply\n");

    sensor->iovdd = devm_regulator_get_optional(dev, "iovdd");
    if (IS_ERR(sensor->iovdd)) {
        ret = PTR_ERR(sensor->iovdd);
        if (ret == -ENODEV)
            sensor->iovdd = NULL;
        else
            return dev_err_probe(dev, ret,
                                 "failed to get iovdd supply\n");
    }

    ret = regulator_set_voltage(sensor->vdd, 2800000, 2800000);
    if (ret)
        return dev_err_probe(dev, ret, "failed to set vdd voltage\n");

    ret = regulator_enable(sensor->vdd);
    if (ret)
        return dev_err_probe(dev, ret, "failed to enable vdd\n");

    if (sensor->iovdd) {
        ret = regulator_enable(sensor->iovdd);
        if (ret)
            goto err_disable_vdd;
    }

    sensor->xclk = devm_clk_get(dev, "xclk");
    if (IS_ERR(sensor->xclk)) {
        ret = dev_err_probe(dev, PTR_ERR(sensor->xclk),
                            "failed to get xclk\n");
        goto err_disable_iovdd;
    }

    ret = clk_prepare_enable(sensor->xclk);
    if (ret)
        goto err_disable_iovdd;

    /*
     * Now the device is powered and clocked.
     * Read chip ID, initialize registers, and register with the subsystem.
     */

    i2c_set_clientdata(client, sensor);
    return 0;

err_disable_iovdd:
    if (sensor->iovdd)
        regulator_disable(sensor->iovdd);
err_disable_vdd:
    regulator_disable(sensor->vdd);
    return ret;
}

static void demo_sensor_remove(struct i2c_client *client)
{
    struct demo_sensor *sensor = i2c_get_clientdata(client);

    /* Stop streaming/users first in a real driver. */
    clk_disable_unprepare(sensor->xclk);
    if (sensor->iovdd)
        regulator_disable(sensor->iovdd);
    regulator_disable(sensor->vdd);
}
```

Important points:

- `devm_regulator_get()` manages the handle, not the enabled state.
- Every successful `regulator_enable()` needs a matching `regulator_disable()`.
- Optional supplies must be truly optional in the hardware design; normalize
  `-ENODEV` from `devm_regulator_get_optional()` to `NULL`, but fail or defer
  on other errors.
- Set voltage/current/load before enabling when the datasheet requires it.
- Enable and disable order must follow the device datasheet.

## Common Bugs And Debugging
Regulator bugs often look like dead hardware: I2C reads fail, chip IDs are wrong, displays stay blank, cards do not enumerate, or the board works only after the bootloader leaves rails on.

### Symptoms And Likely Causes

| Symptom | Likely Cause | Evidence To Check |
| --- | --- | --- |
| `devm_regulator_get(dev, "vdd")` fails | Missing or misspelled `vdd-supply`; provider not registered. | DT property name, provider node status, `-EPROBE_DEFER`. |
| Driver works only after bootloader initialization | Driver forgot to declare or enable a rail. | Bootloader state, dmesg, regulator sysfs/debugfs, physical rail measurement. |
| Voltage request fails | Requested range violates constraints or provider cannot supply it. | `regulator-min/max-microvolt`, provider range table, return value. |
| Device fails after probe error | Enabled supply not disabled on unwind. | Probe labels, enable/disable balance, regulator events. |
| Another device breaks when this driver changes voltage/mode | Shared rail changed without ownership. | Consumer list, board schematic, DT topology. |
| `regulator_disable()` does not turn rail off | Shared users, `always-on`, boot constraints, or provider policy. | Consumer count, constraints, sysfs/debug output. |
| Sleep/atomic warning | Regulator API used in atomic context or provider uses sleepable bus I/O. | Stack trace, I2C/SPI/regmap provider path. |

### Debug Commands

Depending on kernel config and target:

```bash
ls /sys/class/regulator/
cat /sys/class/regulator/regulator.*/name
cat /sys/class/regulator/regulator.*/state
cat /sys/class/regulator/regulator.*/microvolts
```

For tracing:

```bash
mount -t tracefs none /sys/kernel/tracing
ls /sys/kernel/tracing/events/regulator
```

Useful checks:

- Does the provider regulator exist?
- Does its name match the rail you expect from the schematic?
- Does the consumer property name match the driver lookup ID?
- Does the voltage match the board requirement?
- Does the rail turn on before hardware register access?
- Does it turn off after remove/runtime suspend when allowed?

### Fix Patterns

- Match `devm_regulator_get(dev, "name")` to `name-supply`, not `regulator-name`.
- Use `dev_err_probe()` for acquisition failures so probe deferral is handled cleanly.
- Use `devm_regulator_get_optional()` only for hardware that truly may omit the rail.
- Pair every successful enable with a disable on all later failure paths.
- Use bulk helpers when several supplies have the same lifecycle.
- Avoid direct mode or voltage changes on shared rails unless the binding and schematic prove ownership.
- Do not use `regulator_force_disable()` in normal driver cleanup.
- Verify real voltages on hardware during bring-up when software state looks correct but the device is still dead.

## Production Checklist
Before code review or board bring-up, verify:

- DT binding documents every required supply.
- Driver supply IDs exactly match `<name>-supply` properties.
- Required supplies are not requested with optional helpers.
- Voltage/current/load requests come from the datasheet, subsystem policy, or OPP table.
- Every enable has a matching disable on error, remove, stream stop, and runtime suspend paths.
- Power sequencing order matches the datasheet, including clocks, resets, GPIOs, and pinctrl.
- Startup/ramp/off-on delays are handled by provider bindings or explicit waits where required.
- Shared rails are not retuned, force-disabled, or mode-changed casually.
- `always-on` and `boot-on` constraints are reviewed against board design.
- Provider code uses `devm_regulator_register()` where appropriate.
- PMIC provider code uses regmap/locking consistently and does not expose unsafe direct register pokes.
- Debug paths do not access powered-off registers.
- Userspace ABI impact is understood: regulator sysfs/debug visibility is not a private control ABI for your device.

## Interview Readiness
You are ready for interviews when you can reason from a dead device back to its power tree.

Be able to explain:

- Why `struct regulator *` is a consumer handle and `struct regulator_dev` is provider-side.
- Why `vdd-supply` maps to `devm_regulator_get(dev, "vdd")`.
- Why `regulator-name` is not the consumer lookup string.
- What regulator constraints protect.
- Why enabling a regulator is not automatically undone by `devm_regulator_get()`.
- Why `regulator_disable()` may not physically turn a shared rail off.
- How to unwind two enabled supplies when a later clock or reset step fails.
- How PMIC, MFD, regmap, and regulator provider code fit together.
- How to debug a board that only works with bootloader-left-on supplies.

See `interview/23-regulator-framework.md` for structured questions and debugging scenarios.

## Kernel Version Notes
Older material often shows board-file init data, manual `regulator_register()` / `regulator_unregister()`, and internal `struct regulator` fields. Treat those as legacy recognition, not new-driver style.

Practical notes:

- Prefer DT/YAML bindings over board-file regulator constraints for modern embedded systems.
- Prefer `devm_regulator_register()` in new provider code unless manual lifetime is needed.
- Treat `struct regulator *` as opaque in consumers.
- `regulator_set_load()` is the modern load request API; older material may mention optimum-mode helpers.
- Managed get-enable and bulk get-enable helpers depend on target kernel version. Validate against target headers before writing buildable example code.
