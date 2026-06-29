# 25 - IIO Framework

## Learning Goal
Understand how Linux represents sensors and converters through the Industrial I/O framework. After this topic, you should be able to design a small IIO sensor driver, explain channels, sysfs attributes, triggers, buffers, and debug the common failures that happen during bring-up.

After this topic you should be able to:

- Explain **IIO device versus IIO channel**.
- Choose when IIO is the right subsystem for a sensor, ADC, or DAC.
- Fill `struct iio_dev`, `struct iio_info`, and `struct iio_chan_spec`.
- Implement `read_raw()` and, when needed, `write_raw()`.
- Explain direct sysfs reads versus triggered buffered capture.
- Decode `scan_elements/*_type` and understand buffer sample layout.
- Review locking, runtime PM, error paths, and remove ordering in an IIO driver.

## Why This Matters In Real Work
Many embedded boards are full of measurement devices: accelerometers, gyroscopes, ADCs, DACs, light sensors, pressure sensors, temperature sensors, current monitors, and voltage monitors. IIO gives those devices a standard Linux ABI instead of each driver inventing private sysfs files.

You meet IIO when:

- A chip measures a physical quantity and exposes raw samples.
- A userspace app needs standard files such as `in_voltage0_raw`, `in_accel_x_raw`, or `in_illuminance_input`.
- A device can stream repeated samples through `/dev/iio:deviceX`.
- A data-ready IRQ, hrtimer, or sysfs trigger should decide when samples are captured.
- A sensor behind I2C, SPI, or a platform MMIO block must register with a higher-level subsystem.
- A production driver must coordinate regmap, runtime PM, IRQs, buffers, and userspace ABI.

Without IIO, every sensor driver would repeat channel naming, value formatting, buffer layout, trigger handling, and userspace ABI design.

## Mental Model
Think of IIO as a **standard translator between measurement hardware and Linux userspace**.

The simple model:

```text
physical chip
  -> IIO device
       -> channels
            -> sysfs attributes for one-shot values and settings
            -> scan elements for buffered samples
       -> optional trigger
       -> optional buffer
            -> /dev/iio:deviceX
```

Two access styles matter:

| Access Style | Meaning | Userspace Interface |
| --- | --- | --- |
| Direct / one-shot | "Read this value now." | `cat /sys/bus/iio/devices/iio:deviceX/in_*_raw` |
| Triggered buffer | "Capture selected channels when a trigger fires." | Configure sysfs, then read `/dev/iio:deviceX` |

An accelerometer is a good mental example:

- The chip is one `struct iio_dev`.
- X, Y, and Z are three channels.
- `in_accel_x_raw` is a one-shot raw read.
- `in_accel_scale` tells userspace how to convert raw values.
- A triggered buffer can stream X/Y/Z samples plus timestamps.

## Core Concepts
IIO has a small vocabulary, but the details matter because they become userspace ABI.

| Concept | Meaning |
| --- | --- |
| IIO device | Kernel object representing the physical sensor/converter. |
| Channel | One measurable or output signal: ADC input, accelerometer axis, light channel, DAC output. |
| Direct mode | Sysfs attribute access through callbacks such as `read_raw()`. |
| Raw value | Hardware-ish integer value before scale/offset conversion. |
| Scale | Multiplier used to turn raw values into meaningful units. |
| Offset | Bias added before scale in common formulas. |
| Processed value | Driver-provided already-converted value, when appropriate. |
| Trigger | Event source that tells IIO when to capture a sample. |
| Buffer | Kernel buffer exposed through `/dev/iio:deviceX` for repeated samples. |
| Scan element | A channel selected for buffered capture. |
| Scan type | Binary packing description for a buffered channel. |

Useful comparisons:

| IIO | Input | hwmon | V4L2 |
| --- | --- | --- | --- |
| Measurements, ADC/DAC, streaming samples. | User input events such as keys, touch, joystick axes. | Simple hardware monitoring values such as temperatures, fans, voltages. | Camera/video sensors and media pipelines. |
| Good for raw/scale/offset and buffers. | Good for event devices under `/dev/input`. | Good for board monitoring and thermal-style readings. | Good for image formats, controls, subdevs, streaming video. |

**Subsystem choice is an ABI decision.** If userspace should see a measurement channel and maybe stream samples, IIO is often the right place. If the signal is a button, input is usually better. If it is a camera sensor, V4L2 is usually better.

## Kernel Mechanism
The IIO core owns the standard ABI. Your driver owns the hardware access and describes the device accurately enough for the core to expose it.

The usual object relationship:

```text
I2C/SPI/platform device
  -> driver private data
       -> regmap or bus access helpers
       -> locks, IRQ state, runtime PM state
  -> struct iio_dev
       -> struct iio_info callbacks
       -> struct iio_chan_spec[] channel table
       -> optional triggered buffer
       -> optional trigger/event support
```

The driver does these jobs:

- Allocate `struct iio_dev`, usually with `devm_iio_device_alloc()`.
- Store private state behind the IIO object with `iio_priv(indio_dev)`.
- Describe channels with `struct iio_chan_spec`.
- Provide callbacks through `struct iio_info`.
- Register the device with `iio_device_register()` after setup is complete.
- Protect hardware access against concurrent sysfs reads, writes, triggers, IRQs, PM callbacks, and remove.

The IIO core does these jobs:

- Creates standard sysfs attributes from channel descriptions.
- Calls your `read_raw()` / `write_raw()` callbacks for direct access.
- Owns the IIO character device used for buffers and events.
- Manages buffer sysfs controls and scan-element configuration.
- Connects devices to triggers and invokes poll functions.

## Key Structs And APIs
The important structs are best learned by where they sit in the data path.

### Core device object

| API / Field | Role |
| --- | --- |
| `struct iio_dev` | The registered IIO device object. |
| `devm_iio_device_alloc(dev, sizeof_priv)` | Allocate IIO object plus private driver data. |
| `iio_priv(indio_dev)` | Retrieve private data allocated with the IIO object. |
| `indio_dev->dev.parent` | Parent physical device, such as I2C/SPI/platform device. |
| `indio_dev->name` | Device name exposed through the IIO ABI. |
| `indio_dev->modes` | Supported access mode, commonly `INDIO_DIRECT_MODE`. |
| `indio_dev->channels` | Channel table. |
| `indio_dev->num_channels` | Number of channels in the table. |
| `indio_dev->info` | Callback table. |
| `iio_device_register()` | Expose the device to userspace. |
| `iio_device_unregister()` | Remove userspace visibility. |

### Callback table

`struct iio_info` connects sysfs attributes to driver code.

| Callback | Use |
| --- | --- |
| `read_raw(indio_dev, chan, val, val2, mask)` | Read raw, scale, offset, processed value, sampling frequency, integration time, and similar attributes. |
| `write_raw(indio_dev, chan, val, val2, mask)` | Change writable settings such as scale or sampling frequency. |

Common masks and return values:

| Item | Meaning |
| --- | --- |
| `IIO_CHAN_INFO_RAW` | Raw channel value. |
| `IIO_CHAN_INFO_SCALE` | Scale multiplier. |
| `IIO_CHAN_INFO_OFFSET` | Offset/bias. |
| `IIO_CHAN_INFO_PROCESSED` | Already processed value. |
| `IIO_CHAN_INFO_SAMP_FREQ` | Sampling frequency. |
| `IIO_CHAN_INFO_INT_TIME` | Integration time. |
| `IIO_VAL_INT` | `*val` is an integer. |
| `IIO_VAL_INT_PLUS_MICRO` | `*val + *val2 / 1000000`. |
| `IIO_VAL_FRACTIONAL` | `*val / *val2`. |

### Channel description

`struct iio_chan_spec` is the contract between the driver, the core, and userspace.

| Field | Use |
| --- | --- |
| `type` | Measurement type such as `IIO_VOLTAGE`, `IIO_ACCEL`, `IIO_LIGHT`, `IIO_TEMP`. |
| `channel` | Numeric index, used when `.indexed = 1`. |
| `channel2` | Modifier such as `IIO_MOD_X`, `IIO_MOD_Y`, `IIO_MOD_Z`. |
| `address` | Driver-defined register/channel address often used in callbacks. |
| `indexed` | Include numeric channel index in ABI name. |
| `modified` | Include modifier in ABI name. |
| `output` | Channel is output direction, such as a DAC output. |
| `differential` | Channel represents a differential measurement. |
| `info_mask_separate` | Attribute exists per channel. |
| `info_mask_shared_by_type` | Attribute shared by channels of the same type. |
| `scan_index` | Position in buffered sample layout. |
| `scan_type` | Binary storage format for buffered samples. |

Examples of generated names:

| Channel Description | Typical Attribute |
| --- | --- |
| `IIO_VOLTAGE`, indexed channel 0 | `in_voltage0_raw` |
| `IIO_ACCEL`, modified X | `in_accel_x_raw` |
| `IIO_ACCEL`, shared scale | `in_accel_scale` |
| `IIO_LIGHT`, processed | `in_illuminance_input` |

### Buffer and trigger APIs

| API / Field | Use |
| --- | --- |
| `iio_triggered_buffer_setup()` | Set up triggered buffer support before registration. |
| `iio_triggered_buffer_cleanup()` | Clean up unmanaged triggered buffer setup. |
| `iio_pollfunc_store_time` | Common top-half helper that records timestamp. |
| `iio_push_to_buffers_with_timestamp()` | Push one scan plus timestamp to buffers. |
| `iio_trigger_notify_done()` | Tell the trigger core the handler has finished. |
| `iio_buffer_enabled()` | Check whether buffered capture is active. |
| `available_scan_masks` | Valid combinations of enabled scan channels. |
| `active_scan_mask` | Channels currently enabled by userspace. |

## Lifecycle / Data Flow
IIO driver correctness mostly comes from registering only after setup is complete and tearing down only after users can no longer enter.

### Probe flow

```text
bus probe called
  enable required regulators/clocks/reset/pinctrl enough to talk to chip
  verify chip ID or hardware presence
  allocate struct iio_dev and private data
  initialize lock, regmap/bus pointer, PM state
  fill indio_dev fields
  assign channel table and iio_info callbacks
  optionally set available_scan_masks
  optionally set up triggered buffer
  register IIO device
```

Rules:

- Register last, because registration exposes sysfs and character-device entry points.
- If buffer setup succeeds but registration fails, clean up the buffer before returning.
- If runtime PM is used, the device must be resumed before hardware access and balanced on errors.

### One-shot sysfs read

```text
userspace reads in_accel_x_raw
  IIO core finds the channel and mask
  calls driver read_raw()
  driver resumes hardware if needed
  driver reads the register/conversion
  driver fills val/val2
  driver returns IIO_VAL_*
  IIO core formats value for sysfs
```

Typical processed formula:

```text
physical value = (raw + offset) * scale
```

Not every driver exposes every part. Some expose raw plus scale. Some expose processed values. Use standard IIO attributes where possible.

### Buffered capture

```text
userspace selects channels in scan_elements
userspace sets buffer length
userspace selects trigger/current_trigger
userspace enables buffer
trigger fires
  top half records timestamp
  threaded handler reads active channels
  handler packs sample according to scan_type and scan_index
  handler pushes sample to IIO buffers
userspace reads /dev/iio:deviceX
```

Example userspace flow:

```sh
cd /sys/bus/iio/devices/iio:device0
cat name
cat in_accel_scale
cat in_accel_x_raw

cat scan_elements/in_accel_x_type
echo 1 > scan_elements/in_accel_x_en
echo 1 > scan_elements/in_accel_y_en
echo 1 > scan_elements/in_accel_z_en
echo 64 > buffer/length
echo sysfstrig0 > trigger/current_trigger
echo 1 > buffer/enable
# read /dev/iio:device0 with generic_buffer or a binary-aware program
echo 0 > buffer/enable
```

Some kernels/devices expose `buffer0` or `bufferY` style directories. Check the actual sysfs tree instead of hardcoding one spelling in scripts.

## Minimal Practical Example
This is **learning-only pseudo-code**, not a production-ready driver. It shows the shape of a tiny IIO ADC-like device with indexed voltage channels and direct raw reads.

```c
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>

#define MY_ADC_CHAN(_idx) {                                      \
        .type = IIO_VOLTAGE,                                     \
        .indexed = 1,                                            \
        .channel = (_idx),                                       \
        .address = (_idx),                                       \
        .info_mask_separate = BIT(IIO_CHAN_INFO_RAW),            \
        .info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE),    \
}

struct my_adc {
        struct device *dev;
        struct mutex lock;
        void __iomem *base;
};

static const struct iio_chan_spec my_adc_channels[] = {
        MY_ADC_CHAN(0),
        MY_ADC_CHAN(1),
        MY_ADC_CHAN(2),
        MY_ADC_CHAN(3),
};

static int my_adc_read_raw(struct iio_dev *indio_dev,
                           const struct iio_chan_spec *chan,
                           int *val, int *val2, long mask)
{
        struct my_adc *adc = iio_priv(indio_dev);
        int ret = 0;

        mutex_lock(&adc->lock);

        switch (mask) {
        case IIO_CHAN_INFO_RAW:
                /*
                 * Real driver: resume hardware, start/read conversion for
                 * chan->address, handle timeout/error, then return integer.
                 */
                *val = 1234 + chan->address;
                ret = IIO_VAL_INT;
                break;

        case IIO_CHAN_INFO_SCALE:
                /* Example: 1.8 V reference / 4096 counts = 0.000439 V/count */
                *val = 0;
                *val2 = 439;
                ret = IIO_VAL_INT_PLUS_MICRO;
                break;

        default:
                ret = -EINVAL;
        }

        mutex_unlock(&adc->lock);
        return ret;
}

static const struct iio_info my_adc_info = {
        .read_raw = my_adc_read_raw,
};

static int my_adc_probe(struct platform_device *pdev)
{
        struct iio_dev *indio_dev;
        struct my_adc *adc;

        indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*adc));
        if (!indio_dev)
                return -ENOMEM;

        adc = iio_priv(indio_dev);
        adc->dev = &pdev->dev;
        mutex_init(&adc->lock);

        indio_dev->dev.parent = &pdev->dev;
        indio_dev->name = "my-adc";
        indio_dev->modes = INDIO_DIRECT_MODE;
        indio_dev->info = &my_adc_info;
        indio_dev->channels = my_adc_channels;
        indio_dev->num_channels = ARRAY_SIZE(my_adc_channels);

        return iio_device_register(indio_dev);
}
```

What matters in this example:

- `devm_iio_device_alloc()` allocates both `struct iio_dev` and private state.
- `iio_priv()` retrieves the private state.
- `MY_ADC_CHAN()` creates standard `in_voltageN_raw` attributes.
- `info_mask_shared_by_type` creates one shared voltage scale attribute.
- `read_raw()` returns values in the format requested by the mask.
- A real driver must add resource mapping, PM, error paths, remove/unregister, and probably regmap or bus access.

## Common Bugs And Debugging
Start from the symptom. IIO failures usually tell you whether the channel table, sysfs path, buffer setup, trigger, or hardware access path is wrong.

| Symptom | Likely Cause | What To Check |
| --- | --- | --- |
| No `iio:deviceX` appears | Probe failed or `iio_device_register()` not reached. | `dmesg`, driver match table, chip ID, clocks/regulators, probe return. |
| Attribute name is missing | Wrong `info_mask_*`, channel type, `indexed`, or `modified`. | Channel table and generated files under `/sys/bus/iio/devices/iio:deviceX/`. |
| Raw read returns `-EINVAL` | Unsupported mask/channel or bad callback switch. | `read_raw()` mask handling and channel fields. |
| Raw read returns `-EBUSY` | Buffer is enabled and driver blocks direct reads. | `buffer/enable`, `iio_buffer_enabled()`. |
| Values are nonsense | Wrong register read, sign extension, scale, offset, endian, or shift. | Datasheet formula, `scan_type`, raw register dump, regmap trace/debug. |
| Buffer enable fails | Bad trigger, invalid scan mask, no buffer setup, bad scan elements. | `trigger/current_trigger`, `scan_elements/*_en`, `available_scan_masks`. |
| Userspace decodes buffer wrongly | Wrong `scan_index`, `scan_type`, padding, or timestamp parsing. | `scan_elements/*_type`, `scan_elements/*_index`, binary parser. |
| Capture hangs | Trigger never fires or handler forgets completion. | IRQ counts, trigger sysfs, `iio_trigger_notify_done()`, dynamic debug. |
| Sporadic bus errors after idle | Runtime PM missing around reads/trigger handler. | `power/control`, `runtime_status`, driver PM get/put paths. |
| Remove/unbind crashes | Userspace, IRQ, trigger, work, or buffer still active. | Unregister order, buffer cleanup, IRQ/work cancellation, locks. |

Useful commands:

```sh
ls /sys/bus/iio/devices/
cd /sys/bus/iio/devices/iio:device0
cat name
ls
cat in_*_raw
cat in_*_scale
ls scan_elements
cat scan_elements/*_type
cat trigger/current_trigger
cat buffer/length
cat buffer/enable
```

Useful tools:

- `tools/iio/lsiio`: list IIO devices, triggers, and channels.
- `tools/iio/generic_buffer`: read and decode buffered samples.
- `tools/iio/iio_event_monitor`: watch IIO events.
- `dmesg` and dynamic debug: inspect probe, read, trigger, PM, and error paths.
- Bus-specific debug: I2C/SPI/regmap traces when register access looks wrong.

## Production Checklist
Use this before review or board bring-up.

- ABI:
  - Use standard IIO attribute names and channel types.
  - Avoid custom sysfs attributes unless the standard ABI cannot model the hardware.
  - Document raw/scale/offset relationship through standard attributes.
- Channel table:
  - `type`, `indexed`, `modified`, `channel`, and `channel2` match the hardware.
  - `info_mask_*` placement matches per-channel versus shared attributes.
  - `scan_index` order is stable.
  - `scan_type` matches datasheet bit layout, sign, storage width, shift, and endian.
- Probe and lifetime:
  - Hardware identity is verified before registration.
  - `iio_device_register()` happens after channels, callbacks, PM, and optional buffer setup are ready.
  - Error paths clean up buffer setup and balance PM/resources.
  - Remove unregisters users before tearing down hardware.
- Locking:
  - One lock strategy covers `read_raw()`, `write_raw()`, trigger handler, IRQ/event paths, PM callbacks, and remove.
  - Top halves do not sleep or perform slow bus I/O.
  - Threaded handlers and sysfs paths cannot corrupt shared buffers/register sequences.
- Power and resources:
  - Regulators, clocks, resets, GPIOs, pinctrl, and IRQs follow datasheet order.
  - Runtime PM is used around one-shot reads and streaming paths when the chip can suspend.
  - Buffered capture prevents autosuspend or resumes hardware correctly.
- Buffer/trigger:
  - `available_scan_masks` rejects unsupported channel combinations.
  - Trigger handler always calls `iio_trigger_notify_done()`.
  - Userspace can decode samples using only `scan_elements`.
- Debug:
  - `lsiio`, `generic_buffer`, and raw sysfs reads have been tested.
  - PM idle/resume and remove/unbind have been tested.
  - Invalid userspace sequences fail cleanly.

## Interview Readiness
You are ready for interviews when you can explain the mechanism without reciting a struct dump.

Be able to answer:

- Why IIO exists and how it differs from input, hwmon, and V4L2.
- What `struct iio_dev`, `struct iio_info`, and `struct iio_chan_spec` each own.
- How `in_voltage0_raw` or `in_accel_x_raw` gets generated.
- How raw, scale, offset, and processed values relate.
- Why direct sysfs reads and triggered buffers need different concurrency handling.
- How triggers, scan elements, and `/dev/iio:deviceX` work together.
- Why a driver may return `-EBUSY` from `read_raw()`.
- How runtime PM fits around one-shot reads and streaming.
- How to debug wrong values, missing attributes, buffer failures, and remove races.

Practice with [25-iio-framework.md](/home/tungnhs/TungNHS/Knowlege-Interview/Linux-Device-Driver/interview/25-iio-framework.md).

## Kernel Version Notes
IIO APIs and ABI details are stable in concept but have version-sensitive details.

- Prefer current kernel headers when writing examples; older material may show stale struct fields or helper names.
- Current IIO docs describe `struct iio_scan_type` with a newer `format` field; older examples often use `sign`. Understand both when reading old drivers.
- Sysfs buffer directories may appear as `buffer`, `buffer0`, or `bufferY` depending on kernel/device support. Inspect the actual tree.
- Trigger module names and Kconfig symbols have changed over time, especially older GPIO/RTC trigger naming versus interrupt/hrtimer trigger support.
- Managed helpers such as `devm_iio_trigger_alloc()` and `devm_iio_trigger_register()` exist in modern kernels; validate availability in your target tree.
