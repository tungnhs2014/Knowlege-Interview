# 22 - Common Clock Framework Interview Questions

Strong candidates should be able to explain both sides of the clock framework: how an ordinary driver consumes clocks and how a clock provider exposes hardware clock lines. The best answers connect Device Tree, `struct clk`, `struct clk_hw`, prepare/enable ordering, rate changes, and debugging with `clk_summary`.

## Beginner

### 1. What problem does the Common Clock Framework solve?

**Short Answer:**  
CCF gives Linux a common way to describe, enable, disable, and configure hardware clocks across many SoCs and devices.

**Deep Explanation:**  
Before CCF, SoC-specific code often implemented its own clock APIs. That made ordinary drivers less portable because enabling a UART, SPI controller, display block, or sensor clock depended on platform-specific code. CCF splits the problem into providers and consumers. Providers know the clock-controller hardware. Consumers use common `clk_*()` APIs and do not need to know the provider registers.

**API / Code Anchor:**  
`CONFIG_COMMON_CLK`, `struct clk *`, `devm_clk_get()`, `clk_prepare_enable()`, `clk_disable_unprepare()`.

**Production or Debugging Angle:**  
If a peripheral times out or registers read as zero, check whether its clocks were acquired and enabled before register access.

**Common Traps:**  
- Confusing CCF clocks with RTC devices or kernel timer clockids.
- Thinking CCF is only about changing frequency; gating and parent selection matter too.
- Assuming the bootloader leaving a clock on is a valid driver strategy.

**Follow-up Questions:**  
- What is the difference between a clock provider and a clock consumer?
- Why is clock handling important for power management?

### 2. What is a clock provider and what is a clock consumer?

**Short Answer:**  
A provider exposes hardware clock outputs to CCF. A consumer is a driver or subsystem that requests and uses those clocks.

**Deep Explanation:**  
The provider side owns hardware such as PLLs, muxes, dividers, gates, fixed oscillators, or external clock chips. It registers clock hardware with CCF. The consumer side gets an opaque handle and uses it to enable, disable, query, or set clocks. A driver can be both: for example, an external clock generator may consume an input oscillator and provide multiple output clocks.

**API / Code Anchor:**  
Provider: `struct clk_hw`, `struct clk_ops`, `devm_clk_hw_register()`, `devm_of_clk_add_hw_provider()`.  
Consumer: `struct clk *`, `devm_clk_get()`, `clk_prepare_enable()`.

**Production or Debugging Angle:**  
When `devm_clk_get()` returns `-EPROBE_DEFER`, the consumer may be ready but the provider has not registered yet.

**Common Traps:**  
- Using provider APIs in a normal peripheral driver.
- Using consumer `struct clk *` APIs while writing a new provider.
- Forgetting a driver can consume one clock and provide another.

**Follow-up Questions:**  
- Why does provider code use `struct clk_hw`?
- What does `devm_of_clk_add_hw_provider()` expose?

### 3. What do `clocks` and `clock-names` mean in Device Tree?

**Short Answer:**  
`clocks` references provider outputs, while `clock-names` gives local names that the consumer driver uses.

**Deep Explanation:**  
The provider binding defines how many cells identify a clock output. A consumer lists phandles and provider-specific specifiers in `clocks`. The matching names in `clock-names` are the local input names for the consumer driver. A driver calling `devm_clk_get(dev, "bus")` expects `"bus"` to appear in `clock-names`.

**API / Code Anchor:**  
```dts
clocks = <&clk 15>, <&clk 16>;
clock-names = "bus", "core";
```

```c
bus = devm_clk_get(dev, "bus");
core = devm_clk_get(dev, "core");
```

**Production or Debugging Angle:**  
If clock lookup fails, compare the driver's lookup string against `clock-names`, not against provider output names.

**Common Traps:**  
- Using `clock-output-names` as the consumer lookup string.
- Relying on index order instead of names when the binding provides names.
- Misspelling `"per"` as `"pre"` or similar small typos.

**Follow-up Questions:**  
- What does `#clock-cells = <0>` mean?
- What does `#clock-cells = <1>` usually mean?

### 4. Why are there separate prepare and enable operations?

**Short Answer:**  
Prepare may sleep; enable must not sleep. This lets slow clock setup and atomic gate control be separated.

**Deep Explanation:**  
Some clock hardware is controlled by fast MMIO registers, but some clock chips sit behind I2C or SPI. I2C/SPI access can sleep, so it cannot run in provider `.enable` or `.disable`, which may be called in atomic context. CCF separates sleepable setup into `.prepare`/`.unprepare` and fast gate control into `.enable`/`.disable`.

**API / Code Anchor:**  
`clk_prepare()`, `clk_enable()`, `clk_prepare_enable()`, `clk_disable_unprepare()`, provider `.prepare`, `.enable`.

**Production or Debugging Angle:**  
A sleep-in-atomic warning through clock provider callbacks often means the provider put slow bus I/O in `.enable` or `.disable`.

**Common Traps:**  
- Calling only `clk_enable()` from process context and forgetting prepare.
- Assuming `.enable` can perform I2C register writes.
- Forgetting to unprepare after disabling.

**Follow-up Questions:**  
- Which helper should a normal probe path use?
- Where should an I2C clock generator implement sleepable register writes?

## Mid-Level

### 5. Show a safe consumer-side probe flow for two clocks.

**Short Answer:**  
Get both clocks, enable the first, enable the second, and if the second fails, disable the first before returning.

**Deep Explanation:**  
Clock handles acquired with `devm_clk_get()` are automatically released, but enabled clocks are not automatically disabled just because the handle is managed. Enable state must be explicitly unwound. The driver should enable clocks before touching clocked registers and disable them when hardware is stopped.

**API / Code Anchor:**  
```c
bus = devm_clk_get(dev, "bus");
if (IS_ERR(bus))
    return dev_err_probe(dev, PTR_ERR(bus), "bus clk\n");

core = devm_clk_get(dev, "core");
if (IS_ERR(core))
    return dev_err_probe(dev, PTR_ERR(core), "core clk\n");

ret = clk_prepare_enable(bus);
if (ret)
    return ret;

ret = clk_prepare_enable(core);
if (ret) {
    clk_disable_unprepare(bus);
    return ret;
}
```

**Production or Debugging Angle:**  
Check every path after the first successful enable. Probe failures that leave clocks on can waste power or keep hardware partially active.

**Common Traps:**  
- Thinking `devm_clk_get()` manages enable state.
- Returning directly after the second enable fails.
- Disabling in a different order from the hardware's required shutdown order.

**Follow-up Questions:**  
- How would you write the remove path?
- When would `clk_bulk_prepare_enable()` be cleaner?

### 6. How does `devm_clk_get(dev, "ipg")` find the actual hardware clock?

**Short Answer:**  
It resolves the consumer's local clock name to a `clocks` entry, parses the provider specifier, asks the registered provider for the clock, and returns a consumer `struct clk *`.

**Deep Explanation:**  
In a DT-backed system, the consumer node has `clock-names = "ipg"` and a matching `clocks` entry. The OF/CCF path parses the phandle plus cells, finds the provider node registered by the clock provider, calls the provider decode callback, and creates a consumer handle. The consumer sees only `struct clk *`; the provider owns `struct clk_hw`.

**API / Code Anchor:**  
`devm_clk_get()`, `of_parse_phandle_with_args()`, `of_clk_hw_onecell_get()`, `of_clk_hw_simple_get()`, `devm_of_clk_add_hw_provider()`.

**Production or Debugging Angle:**  
For `-EPROBE_DEFER`, inspect whether the provider driver has matched, probed, and registered its OF clock provider.

**Common Traps:**  
- Manually parsing phandles in ordinary consumer drivers.
- Assuming Device Tree is the only possible lookup mechanism on all systems.
- Debugging the consumer while the provider binding is actually wrong.

**Follow-up Questions:**  
- What object does the provider return from its decode callback?
- Why does the consumer get `struct clk *` rather than `struct clk_hw *`?

### 7. What are fixed-rate, gate, mux, divider, and composite clocks?

**Short Answer:**  
They are common hardware clock shapes modeled by CCF helper drivers and registration APIs.

**Deep Explanation:**  
A fixed-rate clock has a constant frequency. A gate only turns a clock on or off. A mux selects one parent from several. A divider derives a rate by dividing its parent. A fixed-factor clock multiplies/divides by constants. A composite clock exposes mux, rate/divider, and gate behavior as one logical clock. Providers should use helper APIs for these common cases instead of open-coding everything.

**API / Code Anchor:**  
`clk_hw_register_fixed_rate()`, `clk_hw_register_gate()`, `clk_hw_register_mux()`, `clk_hw_register_divider()`, `clk_hw_register_composite()`.

**Production or Debugging Angle:**  
Understanding the clock type tells you which operations are legal. A fixed-rate oscillator cannot satisfy arbitrary `clk_set_rate()` requests.

**Common Traps:**  
- Trying to set the rate of a pure gate.
- Forgetting a mux clock needs valid parent data and get/set-parent behavior.
- Using MMIO helper structs for a clock controlled over I2C/SPI.

**Follow-up Questions:**  
- When would a provider need a custom `struct clk_ops`?
- Why might a divider need a value/divider table?

### 8. How should a driver set or query clock rates?

**Short Answer:**  
Use `clk_get_rate()` to query, `clk_round_rate()` to check a supported rate, and `clk_set_rate()` or rate-range/exclusive helpers when the hardware contract allows rate changes.

**Deep Explanation:**  
Clock rates are constrained by parent rates, mux selection, divider tables, fixed sources, and other consumers. A requested rate may be rounded, rejected, or propagated to a parent depending on provider ops and flags such as `CLK_SET_RATE_PARENT`. A driver should not assume the rate it asks for is exact unless it verifies the result and the binding/datasheet allow it.

**API / Code Anchor:**  
`clk_get_rate()`, `clk_round_rate()`, `clk_set_rate()`, `clk_set_rate_range()`, `clk_set_rate_exclusive()`, provider `.recalc_rate`, `.determine_rate`, `.set_rate`.

**Production or Debugging Angle:**  
Changing a shared parent rate can break another device. For audio, camera, display, and CPUfreq paths, rate ownership and constraints matter.

**Common Traps:**  
- Ignoring the return value of `clk_set_rate()`.
- Assuming `clk_get_rate()` returning nonzero means the device is enabled.
- Setting a parent rate globally to fix one peripheral without checking other consumers.

**Follow-up Questions:**  
- What does `CLK_SET_RATE_PARENT` do?
- When would rate-exclusive helpers be useful?

### 9. Debugging scenario: `devm_clk_get(dev, "per")` fails. What do you check?

**Short Answer:**  
Check the DT `clock-names`, the matching `clocks` entry, provider binding, provider probe status, and whether the failure is `-EPROBE_DEFER`.

**Deep Explanation:**  
A clock lookup failure can mean the local name is wrong, the provider phandle/specifier is invalid, the provider has not registered yet, or the provider driver/binding is missing. The local lookup string must match the consumer's `clock-names`, not the provider's output name.

**API / Code Anchor:**  
`devm_clk_get()`, `dev_err_probe()`, `clock-names`, `clocks`, `#clock-cells`.

**Production or Debugging Angle:**  
Use `dev_err_probe()` so deferrals are logged consistently and not mistaken for permanent failures.

**Common Traps:**  
- Looking only at the consumer driver and ignoring the provider node.
- Confusing `"per"` and `"ipg"` ordering.
- Treating all failures as optional clocks.

**Follow-up Questions:**  
- What would `#clock-cells` mismatch look like?
- How can debugfs help after the provider has registered?

## Senior

### 10. How would you write a provider for an I2C-controlled clock generator?

**Short Answer:**  
Register provider `struct clk_hw` objects, expose them through an OF provider, and put sleepable I2C register operations in `.prepare`/`.unprepare` or other sleepable paths, not `.enable`/`.disable`.

**Deep Explanation:**  
MMIO gate helpers assume fast atomic register access. An I2C clock chip cannot safely perform bus transfers in `.enable` because enable callbacks must not sleep. The provider should embed `struct clk_hw` in private clock objects, use `container_of()` in callbacks, describe parents and ops, register with `devm_clk_hw_register()`, and expose clocks with `devm_of_clk_add_hw_provider()`. It may use custom ops instead of MMIO helper structs.

**API / Code Anchor:**  
`struct clk_hw`, `struct clk_ops`, `.prepare`, `.unprepare`, `.recalc_rate`, `.set_rate`, `devm_clk_hw_register()`, `devm_of_clk_add_hw_provider()`.

**Production or Debugging Angle:**  
Review callback context carefully. Locking and sleepability bugs in a provider can affect every consumer of that clock chip.

**Common Traps:**  
- Using `clk_hw_register_gate()` for a non-MMIO I2C-controlled gate.
- Sleeping in `.enable`/`.disable`.
- Returning provider-internal state before it is fully initialized.
- Forgetting to unregister or use managed provider registration.

**Follow-up Questions:**  
- How would you represent multiple outputs?
- When would `of_clk_hw_onecell_get()` fit?

### 11. Why is changing a shared clock rate risky?

**Short Answer:**  
Because the same clock or parent may feed multiple consumers, and a rate change for one device can break timing for another.

**Deep Explanation:**  
The clock tree is shared. A child may propagate rate changes upstream using provider behavior and flags. If a driver changes a parent PLL or shared divider without owning the constraint, other devices can see a changed input rate. The driver must follow the binding, subsystem policy, and hardware constraints. Some cases need `clk_set_rate_range()`, exclusive rate ownership, or subsystem-specific arbitration.

**API / Code Anchor:**  
`clk_set_rate()`, `clk_round_rate()`, `clk_set_rate_range()`, `clk_set_rate_exclusive()`, `CLK_SET_RATE_PARENT`.

**Production or Debugging Angle:**  
A display, audio, or camera bug may appear only when another device changes a shared parent clock. Debug by comparing `clk_summary` before and after enabling the other device.

**Common Traps:**  
- Calling `clk_set_rate()` in probe without checking the current topology.
- Assuming a clock named `"core"` is private.
- Ignoring rate rounding.

**Follow-up Questions:**  
- How would you detect who changed a clock rate?
- What subsystem examples often have strict clock-rate needs?

### 12. Debugging scenario: hardware works with `clk_ignore_unused` but fails without it.

**Short Answer:**  
The driver likely depends on a clock that the bootloader enabled but the kernel considers unused and later disables.

**Deep Explanation:**  
During late init, CCF may disable clocks that no kernel driver has claimed. `clk_ignore_unused` prevents that, which can make broken drivers appear to work. The real fix is to identify the required clock, describe it in firmware, acquire it in the correct driver, and enable it during the active lifecycle.

**API / Code Anchor:**  
`clk_ignore_unused`, `/sys/kernel/debug/clk/clk_summary`, `devm_clk_get()`, `clk_prepare_enable()`.

**Production or Debugging Angle:**  
Use `clk_ignore_unused` as a temporary bring-up diagnostic only. Shipping with it hides ownership bugs and wastes power.

**Common Traps:**  
- Declaring success after adding `clk_ignore_unused`.
- Enabling clocks in the wrong driver because it "fixes" the board.
- Forgetting runtime PM may later disable clocks too.

**Follow-up Questions:**  
- How would you find the missing clock?
- What should the final patch change?

### 13. How do clock handling and runtime PM interact?

**Short Answer:**  
Runtime resume usually enables clocks before register access; runtime suspend quiesces hardware and disables clocks after access stops.

**Deep Explanation:**  
Clocks are often part of a device's active power state. A driver that supports runtime PM should not leave clocks permanently enabled if the hardware can idle. It also must not access clocked registers while suspended. The clock sequence often interacts with regulators, resets, power domains, and pinctrl states, so ordering should come from the datasheet and subsystem conventions.

**API / Code Anchor:**  
`pm_runtime_enable()`, runtime PM callbacks, `clk_prepare_enable()`, `clk_disable_unprepare()`.

**Production or Debugging Angle:**  
If a device works at probe but fails after idle, inspect runtime suspend/resume paths and confirm clocks are re-enabled before register restoration.

**Common Traps:**  
- Disabling clocks while IRQs or DMA can still touch registers.
- Accessing registers in debug/status paths while runtime suspended.
- Assuming devm cleanup replaces explicit runtime PM disable paths.

**Follow-up Questions:**  
- What order would you use with regulators, resets, and clocks?
- How would you prevent new I/O while clocks are off?

### 14. What should a code review look for in clock consumer code?

**Short Answer:**  
Review DT names, acquisition errors, enable/disable pairing, context rules, rate safety, optional-clock use, and power-management ordering.

**Deep Explanation:**  
Clock bugs are often lifecycle bugs. The code can compile and even work on one board because the bootloader left clocks on, while failing on another board or after suspend. Review should check that the driver declares and requests exactly the clocks it needs, enables them before hardware use, disables them after stopping hardware, and does not change rates or parents casually.

**API / Code Anchor:**  
`devm_clk_get()`, `devm_clk_get_optional()`, `clk_bulk_*()`, `clk_prepare_enable()`, `clk_disable_unprepare()`, `clk_set_rate()`.

**Production or Debugging Angle:**  
Use `clk_summary` during review/bring-up to confirm prepare and enable counts behave as expected across probe, open, close, suspend, and remove.

**Common Traps:**  
- Optional clock helper used to hide a missing required binding.
- Missing unwind path after the second or third clock enable.
- Clock enabled before resource acquisition that can still fail.
- No runtime PM thought for a power-sensitive block.

**Follow-up Questions:**  
- When are bulk clock helpers preferable?
- Why might `devm_clk_get_enabled()` be good or bad for a specific driver?

### 15. What is the difference between generic CCF APIs and subsystem clock APIs such as ASoC DAI clock helpers?

**Short Answer:**  
CCF controls hardware clock providers and consumer handles. Subsystem APIs express subsystem-specific clock relationships and may call into lower-level clock or register operations.

**Deep Explanation:**  
An ASoC machine driver may call `snd_soc_dai_set_sysclk()`, `snd_soc_dai_set_pll()`, or `snd_soc_dai_set_clkdiv()` because audio has DAI-specific concepts: master/slave bit clocks, frame clocks, PLLs, and sample-rate-derived system clocks. Those APIs are not replacements for `devm_clk_get()` in a generic driver, and CCF APIs are not replacements for subsystem policy.

**API / Code Anchor:**  
Generic: `devm_clk_get()`, `clk_set_rate()`.  
ASoC: `snd_soc_dai_set_sysclk()`, `snd_soc_dai_set_pll()`, `snd_soc_dai_set_clkdiv()`.

**Production or Debugging Angle:**  
Debug at the right abstraction. If an audio route has the wrong bit clock, inspect both CCF rates and ASoC DAI configuration.

**Common Traps:**  
- Trying to solve subsystem policy with raw `clk_set_rate()` only.
- Ignoring that subsystem callbacks may configure PLLs/dividers internally.
- Treating every occurrence of "clock" as a CCF topic.

**Follow-up Questions:**  
- How would you debug an audio sample-rate mismatch?
- Why do display and camera subsystems also care deeply about clocks?
