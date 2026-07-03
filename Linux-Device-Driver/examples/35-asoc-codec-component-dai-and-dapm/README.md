# 35 - ASoC Codec, Component, DAI, And DAPM Example

This directory contains a **learning-only** ASoC codec/component skeleton. It is intentionally small enough to study, but realistic enough to show how a codec driver registers a component, DAI, ALSA controls, and DAPM widgets/routes.

It is **not production-ready** and it does **not** create a sound card by itself. A real ALSA card still needs a machine driver or a simple-card style Device Tree binding that connects a CPU DAI to this codec DAI.

## Goal

Learn the shape of a minimal reusable ASoC codec/component driver:

- I2C driver probe owns low-level device lifetime.
- `devm_regmap_init_i2c()` creates register access state.
- `devm_snd_soc_register_component()` registers the ASoC component and DAI.
- `struct snd_soc_dai_driver` advertises playback/capture capabilities.
- `struct snd_soc_dai_ops` handles DAI format, sysclk, `hw_params()`, and stream mute.
- `SOC_*` controls expose ALSA mixer controls.
- `SND_SOC_DAPM_*` widgets and routes describe audio paths and power.

## Kernel Version Assumptions

Validated against local Linux `6.8.0-124-generic` headers.

The example uses modern ASoC APIs:

- `struct snd_soc_component_driver`
- `devm_snd_soc_register_component()`
- `struct snd_soc_dai_ops.mute_stream`
- `SND_SOC_DAPM_AIF_IN()` / `SND_SOC_DAPM_AIF_OUT()`
- `devm_regmap_init_i2c()`

Older kernels and older tutorials may use `struct snd_soc_codec`, `digital_mute`, or older component PCM callback names. Check your target kernel headers before copying callback signatures into real code.

## Files

| File | Purpose |
| --- | --- |
| `demo_asoc_codec.c` | Learning-only I2C ASoC codec/component module. |
| `Makefile` | Builds the module against the running kernel headers. |

The module registers an I2C driver named `demo_asoc_codec` with compatible string `ldd,demo-asoc-codec`.

It exposes one DAI named `demo-asoc-hifi` when a matching I2C device is bound.

## Build

From this directory:

```bash
make
```

Expected result:

```text
  CC [M]  demo_asoc_codec.o
  MODPOST Module.symvers
  CC [M]  demo_asoc_codec.mod.o
  LD [M]  demo_asoc_codec.ko
```

## Load And Unload

Load the module:

```bash
sudo insmod demo_asoc_codec.ko
```

Expected result:

```bash
lsmod | grep demo_asoc_codec
modinfo ./demo_asoc_codec.ko
```

At this point only the I2C driver is registered. No probe runs unless a matching I2C device exists.

Unload:

```bash
sudo rmmod demo_asoc_codec
```

Clean build outputs:

```bash
make clean
```

## Optional Probe Test With `i2c-stub`

This optional test lets you bind the driver without real codec hardware. It still does not create an ALSA sound card.

Load an emulated I2C adapter with one fake device address:

```bash
sudo modprobe i2c-stub chip_addr=0x1a
i2cdetect -l | grep -i stub
```

Pick the shown stub bus number, then load this module:

```bash
sudo insmod demo_asoc_codec.ko
```

Create a fake client on the stub bus:

```bash
echo demo_asoc_codec 0x1a | sudo tee /sys/bus/i2c/devices/i2c-<N>/new_device
```

Replace `<N>` with the stub bus number.

Expected kernel log:

```text
demo_asoc_codec <N>-001a: registered learning-only ASoC codec component
```

Expected sysfs evidence:

```bash
ls /sys/bus/i2c/drivers/demo_asoc_codec/
ls /sys/bus/i2c/devices/<N>-001a/
```

Expected ALSA result:

```bash
aplay -l
```

There should be no new sound card from this module alone. That is correct. A codec component must be linked to a CPU DAI by a machine driver or simple-card style binding before ALSA PCM devices appear.

Clean up the fake device:

```bash
echo 0x1a | sudo tee /sys/bus/i2c/devices/i2c-<N>/delete_device
sudo rmmod demo_asoc_codec
sudo rmmod i2c_stub
```

## Debug Commands

Use these during build/load experiments:

```bash
dmesg -w
modinfo ./demo_asoc_codec.ko
lsmod | grep demo_asoc_codec
ls /sys/bus/i2c/drivers/demo_asoc_codec/
```

If a real machine/simple-card binding uses the component, ASoC debugfs may show card, DAI, control, and DAPM state:

```bash
sudo mount -t debugfs none /sys/kernel/debug 2>/dev/null || true
ls /sys/kernel/debug/asoc
find /sys/kernel/debug/asoc -maxdepth 3 -type f | sort
```

Userspace ALSA tools become useful only after a sound card exists:

```bash
aplay -l
arecord -l
amixer -c <card>
alsamixer -c <card>
```

## Expected Behavior

Module load only:

- Registers an I2C driver.
- Does not probe without an I2C device.
- Does not create `/dev/snd/pcm*`.
- Does not create a new ALSA card.

Optional fake I2C client:

- Runs `demo_i2c_probe()`.
- Initializes a regmap object.
- Registers one ASoC component and one DAI.
- Still does not create a card or PCM device without a machine/simple-card binding.

If a machine/simple-card binding later connects to this component:

- The DAI name is `demo-asoc-hifi`.
- Playback stream name is `Playback`.
- Capture stream name is `Capture`.
- DAPM widgets/routes can participate in the card DAPM graph.
- Controls such as `Playback Volume` become ALSA userspace ABI for that card.

## Userspace ABI Impact

This example alone creates no stable userspace ABI:

- No character device.
- No sysfs ABI beyond normal driver-core/I2C binding files.
- No `/dev/snd` PCM or control device unless a sound card binds the component.

If this were used in a real sound card, the ALSA control names would become user-visible ABI. Names such as `Playback Volume` must be treated carefully in production because userspace profiles and mixers can depend on them.

## Cleanup And Error Paths

The example uses managed resources:

- `devm_kzalloc()` frees private data when the I2C device is removed.
- `devm_regmap_init_i2c()` frees regmap state automatically.
- `devm_snd_soc_register_component()` unregisters the component automatically.

Probe can fail with:

| Failure | Cause |
| --- | --- |
| `-ENOMEM` | Private state allocation failed. |
| regmap error | Regmap could not initialize for the I2C client. |
| component registration error | ASoC rejected the component/DAI registration. |

Real codec drivers must add more cleanup-aware resources:

- Regulators and enable/disable ordering.
- Clocks and rate programming.
- Reset GPIO sequencing.
- IRQs and jack detection.
- Runtime PM and suspend/resume regmap cache handling.
- Datasheet-backed volatile/precious register policy.

## Why This Is Not Production-Ready

This is a skeleton for learning the ASoC object model. Production codec work needs:

- A real datasheet and register map.
- Correct reset defaults and chip ID validation.
- Accurate DAI constraints for every supported sample rate/format/channel mode.
- Correct PLL, MCLK, BCLK, LRCLK, and TDM-slot programming.
- Full DAPM graph from the codec block diagram.
- Regulator, clock, reset, IRQ, and PM integration.
- Board-specific routing kept in the machine driver or Device Tree.
- Hardware validation with `aplay`, `arecord`, `amixer`, debugfs DAPM state, and scope/logic-analyzer checks.
