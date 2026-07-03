# 36 - ASoC Machine Drivers And Audio Routing Example

Status: Learning-only.

This example shows the smallest useful board-level ASoC routing exercise: describe an existing CPU DAI and an existing codec DAI with `simple-audio-card`, then verify that ALSA, DAPM, clocks, and routing line up at runtime.

It intentionally does not include a fake loadable machine-driver module. A real ASoC card depends on board wiring, a CPU DAI driver, a codec driver, DMA support, clocks, regulators, pinctrl, and a valid Device Tree. A standalone demo module would compile interesting C while teaching the wrong integration boundary.

## Goal

Add a minimal audio card description to a real board DTS and learn how the machine-driver layer binds:

- CPU DAI endpoint: the SoC I2S/SAI/TDM controller.
- Codec DAI endpoint: the external or internal audio codec.
- DAI format and clock roles: I2S/TDM mode, bit clock, frame clock, and master/provider selection.
- Audio routes: board-level connections such as headphone jack, speaker, microphone, or line input.
- Userspace result: a standard ALSA sound card and PCM devices.

## Kernel Version Assumptions

- Linux 6.x ASoC concepts and Device Tree style.
- `CONFIG_SND_SOC`, `CONFIG_SND_SIMPLE_CARD`, the target CPU DAI driver, the target codec driver, and DMA support are enabled.
- The DTS fragment must be adapted to the target kernel binding files, especially `Documentation/devicetree/bindings/sound/simple-card.yaml` and the CPU/codec binding YAML files.
- The example uses provider/consumer clock terminology in prose. Some existing bindings and older examples may still contain master/slave property names.

## Files

| File | Purpose |
| --- | --- |
| `README.md` | Build, load, test, debug, cleanup, and production notes. |
| `simple-audio-card-example.dts` | Learning-only Device Tree fragment to adapt into a real board DTS. |

No `Makefile` is included because this example does not build a kernel module.

## Device Tree Fragment

Start from `simple-audio-card-example.dts`, then replace these labels with real labels from your board DTS:

- `&sai1`: CPU DAI controller, such as I2S, SAI, McASP, or TDM.
- `&codec0`: codec node that provides a registered codec DAI.
- `&audio_mclk`: optional audio master clock if your codec or CPU DAI needs one.
- Route strings such as `HP_OUT` and `MIC_IN`: codec pin/widget names from the codec driver and binding.

Do not compile the fragment as-is. It is not a complete board DTS and contains placeholder phandles.

## Build / Validate

After integrating the fragment into the board DTS in a kernel tree:

```bash
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- dtbs
```

If Device Tree schema tooling is available:

```bash
make ARCH=arm64 \
  DT_SCHEMA_FILES=Documentation/devicetree/bindings/sound/simple-card.yaml \
  dt_binding_check
```

Use your board architecture and cross compiler instead of the `arm64` example when needed.

## Load / Boot

Install the updated DTB or overlay through the board's normal boot flow, then reboot. The exact deployment command is board-specific; common paths include a bootloader DTB file, an overlay entry in the boot partition, or a firmware-managed overlay directory.

After boot:

```bash
dmesg | grep -i -E 'asoc|alsa|snd|codec|dai|simple|defer|audio'
cat /proc/asound/cards
aplay -l
arecord -l
amixer -c 0
```

## Expected Output

On real hardware with correct endpoints and routes, expect a standard ALSA card:

```text
$ cat /proc/asound/cards
 0 [training       ]: simple-card - training-simple-card
                      training-simple-card
```

The exact card index and short name can differ. `aplay -l` should list at least one playback PCM if the card has a playback path, and `arecord -l` should list capture PCMs if the card has a capture path.

Common failure logs look like:

```text
ASoC: CODEC DAI ... not registered
asoc-simple-card sound: parse error ...
asoc-simple-card sound: probe deferral ...
```

Probe deferral is not always fatal during early boot. It becomes a bug when the card never appears after all clocks, regulators, buses, and codec drivers are available.

## Debug Commands

```bash
mount -t debugfs none /sys/kernel/debug 2>/dev/null || true
ls /sys/kernel/debug/asoc
find /sys/kernel/debug/asoc -maxdepth 3 -type f | sort
grep -R . /sys/kernel/debug/asoc/*/dapm 2>/dev/null | head -80
cat /sys/kernel/debug/clk/clk_summary | grep -i -E 'mclk|sai|i2s|tdm|audio'
```

Useful checks:

- Card absent: verify compatible string, endpoint phandles, driver config, and probe deferral.
- Card present but no PCM: verify CPU DAI and codec DAI names/capabilities.
- PCM opens but no sound: verify DAPM routes, mixer controls, pinctrl, clocks, and amplifier enable GPIOs.
- Wrong sample rate or distorted audio: verify MCLK/BCLK/LRCLK ratios, DAI format, slot width, and provider/consumer clock roles.

## Userspace ABI Impact

When integrated into a real board, this creates standard ALSA userspace ABI:

- `/proc/asound/cards`
- `/dev/snd/pcm*`
- `/dev/snd/control*`
- ALSA mixer controls exposed by the codec and card

It does not create a custom character device, sysfs ABI, or ioctl ABI. Card names, PCM names, and mixer control names can still affect UCM profiles, PipeWire/PulseAudio configuration, test scripts, and manufacturing tools.

## Cleanup

To remove the example from a board:

1. Delete the added `sound` node or set it to `status = "disabled";`.
2. Remove any overlay entry from the bootloader configuration.
3. Rebuild and redeploy the DTB or overlay.
4. Reboot and confirm the card disappeared:

```bash
cat /proc/asound/cards
aplay -l
```

No module unload step is required because this example uses Device Tree description and existing kernel drivers.

## Error-Path Notes

- If the codec probes after the machine card, the card may defer until the codec DAI is registered.
- If the CPU DAI phandle is wrong, the card usually never registers.
- If route names do not match codec widgets or pins, the card can register but the DAPM path may stay powered down.
- If clock roles are reversed, stream startup may fail or audio may be silent, noisy, or at the wrong rate.
- If regulators or GPIOs are missing from the codec node, the codec may appear present but remain physically powered off.

## Production Checklist

- Validate DTS against YAML bindings for the sound card, CPU DAI, codec, clocks, regulators, and pinctrl.
- Use board-accurate route names and codec widget names.
- Confirm playback and capture with the final sample rates, formats, and channel counts.
- Confirm suspend/resume, runtime PM, jack detection, and hotplug behavior if applicable.
- Confirm UCM or userspace audio policy uses the final card/control names.
- Keep board policy in the machine/simple-card layer; keep register programming in codec and CPU DAI drivers.
