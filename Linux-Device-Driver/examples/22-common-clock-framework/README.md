# 22 - Common Clock Framework Example

## Status
This is a **learning-only** example.

It does not implement a production clock provider or bind to real hardware. It uses a standalone DTS to show how a fixed clock provider is referenced by consumers, and it includes consumer-driver pseudo-code for the normal `devm_clk_get()` / `clk_prepare_enable()` lifecycle.

## Goal
Learn how CCF wiring appears in Device Tree and how a consumer driver should handle clock lookup, enable, error unwind, and cleanup.

This example teaches:

- A provider node with `compatible = "fixed-clock"`.
- `#clock-cells = <0>` for a single-output fixed clock.
- Consumer `clocks` and `clock-names` properties.
- Why the driver uses the consumer's local clock name.
- How to inspect clock topology with `clk_summary` on a real target.
- Why managed clock handles do not remove the need to disable enabled clocks.

## Kernel Version Assumptions
The DTS syntax is generic Device Tree syntax and should compile with a normal `dtc`.

The pseudo-code assumes a modern kernel with:

- `devm_clk_get()`
- `clk_prepare_enable()`
- `clk_disable_unprepare()`
- `dev_err_probe()`
- `devm_platform_ioremap_resource()`

For production code, validate API signatures against your target kernel headers. Older kernels may not have all managed convenience helpers such as `devm_clk_get_enabled()`.

## Files
| File | Purpose |
| --- | --- |
| `common-clock-framework-demo.dts` | Standalone learning DTS with one fixed clock provider and two fake consumers. |
| `README.md` | Commands, expected output, pseudo-code, debug workflow, and cleanup explanation. |

No `Makefile` is included because there is no kernel module in this example.

## Build / Validate
From this directory:

```bash
dtc -@ -I dts -O dtb -o common-clock-framework-demo.dtb common-clock-framework-demo.dts
dtc -I dtb -O dts -o common-clock-framework-demo.roundtrip.dts common-clock-framework-demo.dtb
```

Expected result:

- `dtc` exits with status `0`.
- `common-clock-framework-demo.dtb` is created.
- `common-clock-framework-demo.roundtrip.dts` shows the same clock relationships in decompiled form.

You may see warnings on some `dtc` versions if they apply stricter board-level conventions. This file is a standalone teaching DTS, not a production board DTS.

## Load / Run
This example is not meant to be booted as-is.

On a real board, equivalent nodes would live in the board DTS/DTSI, and real drivers would bind to real `compatible` strings. The fake consumers in this example intentionally use training-only compatible strings:

- `training,ccf-uart`
- `training,ccf-sensor`

Do not add those strings to a production binding.

## DTS Walkthrough
The fixed clock provider is:

```dts
refclk: clock-24000000 {
    compatible = "fixed-clock";
    #clock-cells = <0>;
    clock-frequency = <24000000>;
    clock-output-names = "refclk_24m";
};
```

Important details:

- `fixed-clock` means the generic fixed-rate clock provider can register it.
- `#clock-cells = <0>` means consumers reference it with only the phandle: `<&refclk>`.
- `clock-output-names` names the provider output, but consumers should still use their own local `clock-names`.

One consumer is:

```dts
demo_uart: serial@10000000 {
    compatible = "training,ccf-uart";
    reg = <0x10000000 0x1000>;
    clocks = <&refclk>;
    clock-names = "core";
};
```

The matching driver would request:

```c
clk = devm_clk_get(dev, "core");
```

The driver requests `"core"` because that is the consumer input name. It does not request `"refclk_24m"`, which is the provider output name.

## Consumer Driver Pseudo-Code
This is the normal lifecycle for an ordinary driver that consumes one required clock:

```c
struct demo_ccf {
    void __iomem *base;
    struct clk *core_clk;
};

static int demo_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct demo_ccf *priv;
    int ret;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->base = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(priv->base))
        return PTR_ERR(priv->base);

    priv->core_clk = devm_clk_get(dev, "core");
    if (IS_ERR(priv->core_clk))
        return dev_err_probe(dev, PTR_ERR(priv->core_clk),
                             "failed to get core clock\n");

    ret = clk_prepare_enable(priv->core_clk);
    if (ret)
        return dev_err_probe(dev, ret,
                             "failed to enable core clock\n");

    platform_set_drvdata(pdev, priv);

    dev_info(dev, "core clock rate is %lu Hz\n",
             clk_get_rate(priv->core_clk));

    return 0;
}

static void demo_remove(struct platform_device *pdev)
{
    struct demo_ccf *priv = platform_get_drvdata(pdev);

    /* Stop hardware/users first, then disable the clock. */
    clk_disable_unprepare(priv->core_clk);
}
```

For two or more clocks with the same lifecycle, consider `clk_bulk_*()` helpers in production code.

## Debug Commands On A Real Target
On a booted kernel with debugfs enabled:

```bash
mount -t debugfs none /sys/kernel/debug
cat /sys/kernel/debug/clk/clk_summary
```

Useful checks:

- Does the provider clock appear?
- Is the rate what the DTS or provider binding says?
- Does the enable count increase while the driver is active?
- Does the prepare count return to the expected value after remove, stream stop, or runtime suspend?
- Are there orphan clocks or clocks disabled unexpectedly?

Temporary bring-up check:

```text
clk_ignore_unused
```

Use this only as a diagnostic boot argument. If it makes the device work, your driver or DTS likely failed to claim and enable a required clock.

## Expected Output / Logs
For the local `dtc` validation:

```text
common-clock-framework-demo.dtb
common-clock-framework-demo.roundtrip.dts
```

For a real consumer driver using the pseudo-code, a successful probe might log:

```text
training-ccf 10000000.serial: core clock rate is 24000000 Hz
```

If the provider has not registered yet, a real driver may defer probe:

```text
training-ccf 10000000.serial: failed to get core clock: -517
```

`-517` is `-EPROBE_DEFER`; `dev_err_probe()` handles this pattern cleanly.

## Cleanup
Remove generated validation files:

```bash
rm -f common-clock-framework-demo.dtb common-clock-framework-demo.roundtrip.dts
```

No kernel module is loaded, so there is no `rmmod` step.

## Error-Path Explanation
Important cleanup rules:

- `devm_clk_get()` manages the clock handle lifetime.
- `clk_prepare_enable()` changes clock state and must be balanced manually.
- If a later probe step fails after a clock was enabled, call `clk_disable_unprepare()` before returning.
- In `remove()`, stop users and hardware first, then disable clocks.
- Runtime PM paths must also balance enable/disable around active hardware use.

## Userspace ABI Impact
This example creates **no userspace ABI**:

- No `/dev` node.
- No sysfs attributes.
- No ioctl.
- No procfs/debugfs file created by the example.

The only debug interface mentioned is the existing kernel clock debugfs view at `/sys/kernel/debug/clk/clk_summary`.

## Production Notes
Production CCF work would add:

- A real hardware binding documented in YAML.
- Real `compatible` strings accepted upstream or by your BSP.
- Target-specific clock IDs from `include/dt-bindings/clock/...`.
- Correct reset, regulator, pinctrl, and runtime PM ordering.
- Error handling for all resources after clocks are enabled.
- Validation on the target kernel and board, not just `dtc`.
