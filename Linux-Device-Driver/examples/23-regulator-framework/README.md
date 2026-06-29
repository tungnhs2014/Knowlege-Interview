# 23 - Regulator Framework Example

## Status
This is a **learning-only** example.

It does not implement a regulator provider driver, bind to real hardware, or create a production Device Tree binding. It uses a standalone DTS to show how regulator providers and consumers are connected, then shows consumer-driver pseudo-code for the normal `devm_regulator_get()` / `regulator_enable()` / `regulator_disable()` lifecycle.

## Goal
Learn how a fixed regulator is described in Device Tree, how consumer supply names map to `devm_regulator_get()`, and how driver error paths must balance enabled supplies.

This example teaches:

- A fixed 3.3 V provider.
- A second fixed 2.8 V sensor rail supplied by the 3.3 V rail.
- Consumer `vdd-supply`, `iovdd-supply`, and `vmmc-supply` properties.
- Why the driver requests `"vdd"` rather than `"sensor_avdd_2v8"`.
- How to validate the DTS with `dtc`.
- How to inspect regulator state on a real target.
- Why managed regulator handles do not automatically disable enabled rails.

## Kernel Version Assumptions
The DTS is generic Device Tree syntax and should compile with a normal `dtc`.

The pseudo-code assumes a modern kernel with:

- `devm_regulator_get()`
- `devm_regulator_get_optional()`
- `regulator_set_voltage()`
- `regulator_enable()`
- `regulator_disable()`
- `dev_err_probe()`

For production code, validate helper availability and signatures against your target kernel headers. Managed helpers such as `devm_regulator_get_enable()` and managed bulk get-enable helpers are convenient on newer kernels, but this example keeps explicit enable/disable calls so the lifetime is visible.

## Files
| File | Purpose |
| --- | --- |
| `regulator-framework-demo.dts` | Standalone learning DTS with two fixed regulators and two fake consumers. |
| `README.md` | Commands, expected output, pseudo-code, debug workflow, and cleanup notes. |

No `Makefile` is included because there is no kernel module in this example.

## Build / Validate
From this directory:

```bash
dtc -@ -I dts -O dtb -o regulator-framework-demo.dtb regulator-framework-demo.dts
dtc -I dtb -O dts -o regulator-framework-demo.roundtrip.dts regulator-framework-demo.dtb
```

Expected result:

- `dtc` exits with status `0`.
- `regulator-framework-demo.dtb` is created.
- `regulator-framework-demo.roundtrip.dts` shows the same regulator phandle relationships in decompiled form.

Some `dtc` versions may warn about fake `compatible` strings or simplified bus modeling. This is a standalone teaching DTS, not a production board DTS.

## Load / Run
This example is not meant to be booted as-is.

On a real board, equivalent regulator nodes would live in the board DTS/DTSI and real drivers would bind to real `compatible` strings. The fake consumers intentionally use training-only compatible strings:

- `training,regulator-sensor`
- `training,regulator-mmc`
- `training,i2c-bus`

Do not add these strings to a production binding.

## DTS Walkthrough
The fixed 3.3 V provider is:

```dts
reg_3v3: regulator-3v3 {
    compatible = "regulator-fixed";
    regulator-name = "board_3v3";
    regulator-min-microvolt = <3300000>;
    regulator-max-microvolt = <3300000>;
    regulator-always-on;
};
```

Important details:

- `regulator-fixed` means the generic fixed-regulator provider can represent this rail.
- `regulator-name` is a provider/output name used for diagnostics and binding data.
- `regulator-min-microvolt` and `regulator-max-microvolt` constrain the rail to 3.3 V.
- `regulator-always-on` says this rail should not be disabled during normal operation.

The 2.8 V sensor rail is:

```dts
reg_sensor_2v8: regulator-sensor-2v8 {
    compatible = "regulator-fixed";
    regulator-name = "sensor_avdd_2v8";
    regulator-min-microvolt = <2800000>;
    regulator-max-microvolt = <2800000>;
    startup-delay-us = <5000>;
    vin-supply = <&reg_3v3>;
};
```

Important details:

- `vin-supply = <&reg_3v3>` describes the parent input rail.
- `startup-delay-us` documents a power-stabilization delay for this regulator.
- Real hardware may use GPIO enable properties or a PMIC-specific regulator child node instead.

One consumer is:

```dts
sensor@40 {
    compatible = "training,regulator-sensor";
    reg = <0x40>;
    vdd-supply = <&reg_sensor_2v8>;
    iovdd-supply = <&reg_3v3>;
};
```

The matching driver requests:

```c
vdd = devm_regulator_get(dev, "vdd");
iovdd = devm_regulator_get_optional(dev, "iovdd");
```

The driver requests `"vdd"` because the consumer property is `vdd-supply`. It does not request `"sensor_avdd_2v8"`, which is the provider's `regulator-name`.

## Consumer Driver Pseudo-Code
This is the normal lifecycle for a driver with one required rail and one optional rail:

```c
struct demo_regulator_sensor {
    struct regulator *vdd;
    struct regulator *iovdd;
};

static int demo_probe(struct i2c_client *client)
{
    struct device *dev = &client->dev;
    struct demo_regulator_sensor *priv;
    int ret;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->vdd = devm_regulator_get(dev, "vdd");
    if (IS_ERR(priv->vdd))
        return dev_err_probe(dev, PTR_ERR(priv->vdd),
                             "failed to get vdd\n");

    priv->iovdd = devm_regulator_get_optional(dev, "iovdd");
    if (IS_ERR(priv->iovdd)) {
        ret = PTR_ERR(priv->iovdd);
        if (ret == -ENODEV)
            priv->iovdd = NULL;
        else
            return dev_err_probe(dev, ret,
                                 "failed to get iovdd\n");
    }

    ret = regulator_set_voltage(priv->vdd, 2800000, 2800000);
    if (ret)
        return dev_err_probe(dev, ret,
                             "failed to set vdd voltage\n");

    ret = regulator_enable(priv->vdd);
    if (ret)
        return dev_err_probe(dev, ret,
                             "failed to enable vdd\n");

    if (priv->iovdd) {
        ret = regulator_enable(priv->iovdd);
        if (ret)
            goto err_disable_vdd;
    }

    i2c_set_clientdata(client, priv);

    /*
     * Safe point for later steps:
     * enable clocks, release reset, read chip ID, initialize registers,
     * and register with the subsystem.
     */
    dev_info(dev, "sensor supplies enabled\n");

    return 0;

err_disable_vdd:
    regulator_disable(priv->vdd);
    return ret;
}

static void demo_remove(struct i2c_client *client)
{
    struct demo_regulator_sensor *priv = i2c_get_clientdata(client);

    /* Stop users and hardware first, then turn off supplies. */
    if (priv->iovdd)
        regulator_disable(priv->iovdd);
    regulator_disable(priv->vdd);
}
```

For several always-together supplies, consider `regulator_bulk_*()` helpers in production code. For whole-device-lifetime rails on newer kernels, managed get-enable helpers may be useful, but use them only when their lifetime matches the hardware behavior.

For optional supplies, `-ENODEV` means the optional DT/property lookup is absent and can be treated as no supply. Other errors, especially `-EPROBE_DEFER`, should still fail or defer probe.

## Debug Commands On A Real Target
On a booted kernel with regulator sysfs/debug support:

```bash
ls /sys/class/regulator/
cat /sys/class/regulator/regulator.*/name
cat /sys/class/regulator/regulator.*/state
cat /sys/class/regulator/regulator.*/microvolts
```

With tracefs mounted and regulator trace events available:

```bash
mount -t tracefs none /sys/kernel/tracing
ls /sys/kernel/tracing/events/regulator
```

Useful checks:

- Does the provider regulator appear?
- Is the voltage what the binding or PMIC driver says?
- Does the consumer property name match the driver lookup ID?
- Does the rail turn on before register access?
- Does it turn off after remove or runtime suspend when constraints allow it?
- Does the hardware rail actually change on a meter or scope?

## Expected Output / Logs
For local `dtc` validation:

```text
regulator-framework-demo.dtb
regulator-framework-demo.roundtrip.dts
```

For a real consumer driver using the pseudo-code, a successful probe might log:

```text
training-regulator-sensor 0-0040: sensor supplies enabled
```

If the provider has not registered yet, a real driver may defer probe:

```text
training-regulator-sensor 0-0040: failed to get vdd: -517
```

`-517` is `-EPROBE_DEFER`; `dev_err_probe()` handles this pattern cleanly.

If the DT property is misspelled, for example `VDD-supply` while the driver requests `"vdd"`, lookup can fail or fall back depending on kernel configuration and regulator policy. Treat missing-supply warnings as real board-description bugs unless the supply is truly optional.

## Cleanup
Remove generated validation files:

```bash
rm -f regulator-framework-demo.dtb regulator-framework-demo.roundtrip.dts
```

No kernel module is loaded, so there is no `rmmod` step.

## Error-Path Explanation
Important cleanup rules:

- `devm_regulator_get()` manages the regulator handle lifetime.
- `regulator_enable()` changes hardware/framework state and must be balanced manually.
- If a later probe step fails after a supply was enabled, call `regulator_disable()` before returning.
- In `remove()`, stop users, DMA, IRQs, streaming, or register access first, then disable supplies.
- Runtime PM paths must also balance supply enable/disable around active hardware use.
- Shared or `always-on` rails may not physically turn off after your consumer disables them; this is normal if other users or constraints still require the rail.

## Userspace ABI Impact
This example creates **no userspace ABI**:

- No `/dev` node.
- No sysfs attributes created by example code.
- No ioctl.
- No procfs/debugfs file created by the example.

The debug interfaces mentioned are existing kernel regulator and tracing views, such as `/sys/class/regulator/` and tracefs regulator events. They are for inspection/debugging, not a private control ABI for this training device.

## Production Notes
Production regulator work would add:

- Real hardware-compatible strings documented in YAML bindings.
- Real board schematics and datasheet-derived rail constraints.
- Correct regulator provider type: fixed, GPIO, PMIC/MFD, PWM, or hardware-specific.
- Correct startup, ramp, and off-on delays.
- Correct ordering with pinctrl, reset GPIOs, clocks, interrupts, DMA, and runtime PM.
- Error handling for every resource acquired after supplies are enabled.
- Target-kernel validation for convenience helpers such as managed get-enable or bulk helpers.
- Electrical validation on hardware, not just `dtc`.
