# 22 - Common Clock Framework

## Learning Goal
Understand how Linux represents hardware clocks, how drivers consume clocks safely, and what changes when you write a clock provider instead of an ordinary device driver.

After this topic you should be able to:

- Explain **clock provider versus clock consumer**.
- Read a Device Tree clock reference such as `clocks = <&clk ID>` and `clock-names = "ipg"`.
- Use `devm_clk_get()`, `clk_prepare_enable()`, `clk_disable_unprepare()`, and rate APIs correctly.
- Recognize when a driver should use bulk, optional, or managed clock helpers.
- Understand the provider-side objects: `struct clk_hw`, `struct clk_ops`, and CCF registration helpers.
- Debug missing clocks, bad names, wrong rates, and unused-clock shutdown during bring-up.

## Why This Matters In Real Work
Most SoC peripherals do not work just because their MMIO registers are mapped. They also need one or more hardware clock signals turned on and sometimes configured to the correct parent or rate.

You see CCF in real drivers when:

- A UART has `ipg` and `per` clocks before register access.
- An SPI/I2C/PWM controller has bus and peripheral clocks.
- A camera sensor needs an external `xclk`.
- A display controller must stop clocks during blanking or suspend.
- Audio code configures system clocks, PLLs, dividers, and bit-clock relationships.
- DVFS changes CPU/device frequency as part of power management.

Without CCF, every SoC would expose different clock APIs. With CCF, most drivers can say: "give me my named clock, enable it, maybe set a rate, and disable it later" without knowing the clock-controller registers.

## Mental Model
A **clock** is a hardware signal that feeds a device or another clock block. A **clock tree** starts at roots such as oscillators or PLLs, then flows through muxes, dividers, gates, and finally into devices.

Think of the Common Clock Framework as a switchboard:

- **Clock providers** own the hardware that creates, selects, divides, or gates clock signals.
- **Clock consumers** are ordinary drivers that need those signals.
- The **CCF core** tracks the tree, rates, parents, enable counts, prepare counts, and callback dispatch.
- **Device Tree or firmware** describes which provider output feeds which consumer input.

Consumer drivers should not poke clock-controller registers directly. They request a local clock name such as `"bus"`, `"core"`, `"ipg"`, `"per"`, or `"xclk"` and let CCF resolve the provider.

## Core Concepts
CCF has two main faces. Mixing them up is one of the fastest ways to write confusing clock code.

| Concept | Used By | Meaning |
| --- | --- | --- |
| `struct clk *` | Consumer drivers | Opaque handle returned by `clk_get()` / `devm_clk_get()`. |
| `struct clk_hw` | Provider drivers | Hardware clock object registered into CCF. |
| `struct clk_ops` | Provider drivers | Hardware callbacks for enable, disable, rate, parent, and prepare operations. |
| `struct clk_core` | CCF internals | Internal framework object; drivers should not depend on it. |
| `#clock-cells` | Provider binding | Number of cells after the provider phandle in a clock specifier. |
| `clocks` | Consumer binding | Phandle/specifier list pointing to provider clocks. |
| `clock-names` | Consumer binding | Local names used by the driver in `devm_clk_get(dev, name)`. |
| `clock-output-names` | Provider binding | Optional provider output names; consumers should not use these as their local names. |

### Clock Types
Provider code usually models hardware using one or more common clock types:

| Type | Hardware Meaning | Typical Ops |
| --- | --- | --- |
| Fixed-rate | Always runs at one frequency. | `recalc_rate` only. |
| Fixed-factor | Parent rate multiplied/divided by constants. | rate calculation, sometimes parent propagation. |
| Gate | Turns a parent-derived signal on/off. | `enable`/`disable` or `prepare`/`unprepare`. |
| Mux | Selects one parent from several inputs. | `get_parent` / `set_parent`. |
| Divider | Divides parent rate using register fields or a table. | `recalc_rate`, `round_rate`, `set_rate`. |
| Composite | One logical clock made from mux + divider/rate + gate. | combined mux/rate/gate ops. |

## Kernel Mechanism
The CCF core lives under `drivers/clk/` and is enabled by `CONFIG_COMMON_CLK`. It provides the common accounting and consumer API, while provider drivers implement hardware-specific behavior through callbacks.

### Consumer Resolution From Device Tree
A typical consumer node looks like this:

```dts
uart1: serial@02020000 {
    compatible = "fsl,imx6q-uart";
    reg = <0x02020000 0x4000>;
    interrupts = <0 26 IRQ_TYPE_LEVEL_HIGH>;

    clocks = <&clks IMX6QDL_CLK_UART_IPG>,
             <&clks IMX6QDL_CLK_UART_SERIAL>;
    clock-names = "ipg", "per";
};
```

The driver asks for `"ipg"` or `"per"`. Internally, CCF:

1. Finds the consumer's `clocks` entry matching the requested name.
2. Parses the provider phandle and cells.
3. Finds the registered clock provider for that provider node.
4. Calls the provider's decode callback.
5. Returns an opaque `struct clk *` consumer handle.

The driver should not manually parse `of_parse_phandle_with_args()` for normal clock consumption. That is useful for framework code, providers, or debugging, but consumers should normally use the CCF API.

### Provider Registration
A provider driver registers hardware clock objects with the framework. A simplified provider flow is:

```text
probe()
  map provider registers
  get parent clocks if this provider depends on another provider
  allocate private provider data
  initialize each embedded struct clk_hw
  set struct clk_init_data / parent data / ops
  devm_clk_hw_register()
  devm_of_clk_add_hw_provider()
```

Provider callbacks receive `struct clk_hw *`. Provider-private objects usually embed `struct clk_hw`, then recover private state with `container_of()`.

```c
struct my_gate_clk {
    struct clk_hw hw;
    void __iomem *reg;
    u8 bit;
    spinlock_t *lock;
};

#define to_my_gate_clk(_hw) container_of(_hw, struct my_gate_clk, hw)
```

For common MMIO clock shapes, do not hand-roll everything. Prefer helpers such as:

- `devm_clk_hw_register_fixed_rate()`
- `devm_clk_hw_register_fixed_factor()`
- `devm_clk_hw_register_gate()`
- `devm_clk_hw_register_mux()`
- `devm_clk_hw_register_divider()`
- `devm_clk_hw_register_composite()`
- `devm_of_clk_add_hw_provider()`

### Prepare Versus Enable
This is the most important execution-context rule in CCF.

| Operation | May Sleep? | Typical Use |
| --- | --- | --- |
| `clk_prepare()` / provider `.prepare` | Yes | Slow setup, sleepable bus access, regulator/reset sequencing. |
| `clk_enable()` / provider `.enable` | No | Atomic gate/ungate, usually fast MMIO bit update. |
| `clk_prepare_enable()` | May sleep | Normal consumer helper in probe/open/runtime resume. |
| `clk_disable_unprepare()` | May sleep | Normal cleanup helper. |

**Provider rule:** if the clock chip is controlled over I2C/SPI, do not perform that sleepable bus access in `.enable` or `.disable`. Put it in `.prepare` and `.unprepare`.

**Consumer rule:** in process context, prefer `clk_prepare_enable()` and pair it with `clk_disable_unprepare()`.

## Key Structs And APIs
The important API is easier to remember if you group it by role.

### Consumer APIs
Use these in ordinary device drivers:

```c
struct clk *devm_clk_get(struct device *dev, const char *id);
struct clk *devm_clk_get_optional(struct device *dev, const char *id);
struct clk *devm_clk_get_enabled(struct device *dev, const char *id);

int clk_prepare_enable(struct clk *clk);
void clk_disable_unprepare(struct clk *clk);

unsigned long clk_get_rate(struct clk *clk);
long clk_round_rate(struct clk *clk, unsigned long rate);
int clk_set_rate(struct clk *clk, unsigned long rate);
int clk_set_rate_range(struct clk *clk, unsigned long min, unsigned long max);

int clk_set_parent(struct clk *clk, struct clk *parent);
struct clk *clk_get_parent(struct clk *clk);
```

For several clocks with the same lifecycle, prefer bulk helpers when available:

```c
struct clk_bulk_data clks[] = {
    { .id = "bus" },
    { .id = "core" },
};

ret = devm_clk_bulk_get(dev, ARRAY_SIZE(clks), clks);
ret = clk_bulk_prepare_enable(ARRAY_SIZE(clks), clks);
clk_bulk_disable_unprepare(ARRAY_SIZE(clks), clks);
```

### Provider APIs
Use these in clock-controller or clock-generator drivers:

```c
struct clk_hw;
struct clk_ops;
struct clk_init_data;
struct clk_parent_data;
struct clk_hw_onecell_data;

int devm_clk_hw_register(struct device *dev, struct clk_hw *hw);
int devm_of_clk_add_hw_provider(struct device *dev,
        struct clk_hw *(*get)(struct of_phandle_args *clkspec, void *data),
        void *data);

struct clk_hw *of_clk_hw_simple_get(struct of_phandle_args *clkspec,
                                    void *data);
struct clk_hw *of_clk_hw_onecell_get(struct of_phandle_args *clkspec,
                                     void *data);
```

### Device Tree Properties
Use the provider binding to determine the exact specifier format.

```dts
clk: clock-controller@020c4000 {
    compatible = "vendor,soc-clock-controller";
    reg = <0x020c4000 0x4000>;
    #clock-cells = <1>;
};

device@1000 {
    compatible = "vendor,my-device";
    reg = <0x1000 0x100>;
    clocks = <&clk 15>, <&clk 16>;
    clock-names = "bus", "core";
};
```

`#clock-cells = <0>` means the phandle alone identifies the clock, often for a single `fixed-clock`. `#clock-cells = <1>` commonly means the consumer supplies one ID cell after the phandle.

## Lifecycle / Data Flow
Clock lifecycle is a resource-ownership problem. The big idea: acquire handles early, enable only when needed, and unwind exactly.

### Consumer Probe Flow

```text
probe()
  allocate private data
  map registers / get IRQs / get resets / get regulators
  get clocks by local names
  optionally set rate or parent
  enable clocks before touching clocked registers
  initialize hardware
  register with subsystem
```

### Consumer Remove / Error Flow

```text
remove() or probe failure
  stop users/subsystem activity
  stop device transactions
  disable hardware interrupt generation
  disable/unprepare clocks in reverse order
  devm releases clock handles after the driver cleanup path returns
```

### Runtime PM Flow
For devices that can idle:

```text
runtime_resume()
  enable regulators/resets as needed
  clk_prepare_enable()
  restore registers if needed

runtime_suspend()
  quiesce hardware
  save state if needed
  clk_disable_unprepare()
  disable regulators/resets as needed
```

Deep suspend/resume policy belongs with power management, but the clock rule is simple: do not access clocked registers while the required clock is off.

## Minimal Practical Example
This is **learning-only pseudo-code**, not a complete production driver. It shows the consumer side, which is what most device drivers need.

```c
struct demo_dev {
    void __iomem *base;
    struct clk *bus_clk;
    struct clk *core_clk;
};

static int demo_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct demo_dev *d;
    int ret;

    d = devm_kzalloc(dev, sizeof(*d), GFP_KERNEL);
    if (!d)
        return -ENOMEM;

    d->base = devm_platform_ioremap_resource(pdev, 0);
    if (IS_ERR(d->base))
        return PTR_ERR(d->base);

    d->bus_clk = devm_clk_get(dev, "bus");
    if (IS_ERR(d->bus_clk))
        return dev_err_probe(dev, PTR_ERR(d->bus_clk),
                             "failed to get bus clock\n");

    d->core_clk = devm_clk_get(dev, "core");
    if (IS_ERR(d->core_clk))
        return dev_err_probe(dev, PTR_ERR(d->core_clk),
                             "failed to get core clock\n");

    ret = clk_prepare_enable(d->bus_clk);
    if (ret)
        return dev_err_probe(dev, ret, "failed to enable bus clock\n");

    ret = clk_prepare_enable(d->core_clk);
    if (ret)
        goto err_disable_bus;

    dev_info(dev, "core clock rate: %lu Hz\n",
             clk_get_rate(d->core_clk));

    platform_set_drvdata(pdev, d);
    return 0;

err_disable_bus:
    clk_disable_unprepare(d->bus_clk);
    return ret;
}

static void demo_remove(struct platform_device *pdev)
{
    struct demo_dev *d = platform_get_drvdata(pdev);

    /* Stop hardware and unregister users first. */
    clk_disable_unprepare(d->core_clk);
    clk_disable_unprepare(d->bus_clk);
}
```

Important points:

- `devm_clk_get()` manages the handle lifetime, not the enable state.
- `clk_prepare_enable()` still needs an explicit matching `clk_disable_unprepare()`.
- Enable ordering and disable ordering should match the hardware requirement; reverse order is often safest.
- `dev_err_probe()` is useful because clock providers may not have probed yet and `-EPROBE_DEFER` is common.

## Common Bugs And Debugging
Clock bugs often look like dead hardware: register reads return nonsense, interrupts never arrive, transfers timeout, or a device works only when the bootloader left clocks on.

### Symptoms And Likely Causes

| Symptom | Likely Cause | Evidence To Check |
| --- | --- | --- |
| Probe returns `-ENOENT` or `-EINVAL` from `devm_clk_get()` | Wrong `clock-names`, missing `clocks`, bad provider binding. | Driver name string versus DT `clock-names`. |
| Probe keeps deferring | Clock provider not registered yet. | `dev_err_probe()` logs with `-EPROBE_DEFER`; provider driver/binding status. |
| Register access hangs or returns bad values | Bus/core clock not enabled before MMIO access. | Enable path, `clk_summary`, reset/power state. |
| Device works in bootloader but fails later | Kernel disabled unused bootloader clocks. | `clk_summary`, boot with `clk_ignore_unused` as temporary proof. |
| Rate is zero or wrong | Wrong clock input, unsupported rate, fixed-rate source, shared parent. | `clk_get_rate()`, `clk_summary`, binding clock IDs. |
| Random failures after probe error | Missing `clk_disable_unprepare()` on unwind. | Probe labels, enable-count mismatch in `clk_summary`. |
| Sleep-in-atomic warning in provider | Provider did I2C/SPI access in `.enable`/`.disable`. | Stack trace through clock provider ops. |

### Debug Commands

```bash
mount -t debugfs none /sys/kernel/debug
cat /sys/kernel/debug/clk/clk_summary
```

Useful things to inspect:

- Clock name and parent tree.
- Current rate.
- Prepare count and enable count.
- Orphan clocks.
- Clocks unexpectedly disabled after late init.

Temporary bring-up trick:

```text
clk_ignore_unused
```

Use this kernel boot argument only to prove a missing ownership problem. **Do not ship a driver that depends on it.**

### Fix Patterns

- Match `clock-names` exactly to driver `devm_clk_get()` IDs.
- Use `dev_err_probe()` for clock acquisition failures.
- Pair every successful enable with a disable on every error path.
- Use `devm_clk_get_optional()` only for truly optional hardware.
- Use `clk_bulk_*()` when several clocks have the same lifecycle.
- Move slow provider register I/O from `.enable`/`.disable` to `.prepare`/`.unprepare`.
- Avoid rate or parent changes on shared clocks unless the binding and all consumers allow it.

## Production Checklist
Before code review or board bring-up, verify:

- DT binding lists every required clock and its correct local `clock-names`.
- Driver strings match `clock-names`.
- All required clocks are acquired before register access.
- Optional clocks are optional in the datasheet, not just inconvenient on one board.
- Every successful `clk_prepare_enable()` is unwound on every later failure path.
- Remove, runtime suspend, stream stop, and blanking paths disable clocks after hardware is quiesced.
- Rate changes use `clk_round_rate()` or documented supported rates where needed.
- Shared clocks are not reparented or retuned without checking other consumers.
- Provider code uses `clk_hw_*` APIs for new code, not old `clk_register_*` patterns.
- Provider `.enable`/`.disable` cannot sleep; slow bus control is in `.prepare`/`.unprepare`.
- MMIO provider callbacks use appropriate locking around shared registers.
- Debugfs `clk_summary` looks sane during idle and active states.
- `clk_ignore_unused` is not required for normal operation.

## Interview Readiness
You are ready for interviews when you can reason from a broken device back to the clock tree.

Be able to explain:

- Why `struct clk *` is for consumers and `struct clk_hw` is for providers.
- Why CCF has both prepare and enable.
- How Device Tree `clocks` and `clock-names` become a `struct clk *`.
- What `#clock-cells` means.
- Why provider output names and consumer input names are different.
- How to unwind two enabled clocks after the second enable fails.
- How to debug a board that works with bootloader clocks but fails after Linux starts.
- Why changing a shared parent rate can break unrelated devices.

See `interview/22-common-clock-framework.md` for structured questions and debugging scenarios.

## Kernel Version Notes
Older material often shows v4.19-era APIs and text binding paths. Current kernels still support many old patterns, but new code should prefer current helpers and YAML bindings.

Practical notes:

- Prefer `clk_hw_*` and managed provider helpers for new clock-provider code.
- Use `struct clk_parent_data` or parent-hw/parent-data helper variants where appropriate instead of relying only on raw parent-name strings.
- Consumer helpers such as `devm_clk_get_enabled()`, `devm_clk_get_optional()`, `clk_bulk_*()`, `clk_set_rate_range()`, rate-exclusive helpers, phase/duty-cycle APIs, and notifiers may be available depending on target kernel.
- Binding documentation has largely moved from `.txt` files to YAML schemas.
- Always build examples against the target kernel headers before treating API names as final.
