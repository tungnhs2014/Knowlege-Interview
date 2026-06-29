# 25 - IIO Framework Interview Questions

Strong candidates can reason from a physical sensor to the Linux ABI. They can explain why IIO exists, how channels generate sysfs files, how direct reads differ from triggered buffers, and where locking, runtime PM, and remove ordering break real drivers.

## Beginner

### 1. What problem does the IIO framework solve?

**Level:** Beginner

**Short Answer:**  
IIO gives sensors, ADCs, DACs, and measurement devices a standard Linux ABI for channels, values, events, triggers, and buffered samples.

**Deep Explanation:**  
Without IIO, each sensor driver would invent its own sysfs files and userspace format. IIO standardizes common measurement concepts: a physical chip is an IIO device, each measurable signal is a channel, direct values appear through sysfs, and repeated samples can be streamed through `/dev/iio:deviceX`. This makes applications and drivers more reusable.

**API / Code Anchor:**  
`struct iio_dev`, `struct iio_chan_spec`, `struct iio_info`, `/sys/bus/iio/devices/iio:deviceX/`, `/dev/iio:deviceX`.

**Production or Debugging Angle:**  
Choosing IIO gives userspace predictable names such as `in_voltage0_raw` and `in_accel_x_raw`. A custom ABI makes tooling, testing, and long-term support harder.

**Common Traps:**  
- Treating IIO as "just sysfs files".
- Using IIO for button/key events that belong in input.
- Creating custom attributes when standard IIO attributes exist.

**Follow-up Questions:**  
- Which devices usually belong in IIO?
- How does IIO differ from hwmon?
- Why is userspace ABI stability important?

### 2. What is the difference between an IIO device and an IIO channel?

**Level:** Beginner

**Short Answer:**  
The IIO device is the whole physical chip. A channel is one measurable or output signal of that chip.

**Deep Explanation:**  
An accelerometer is one IIO device, but X, Y, and Z are separate channels. An ADC chip is one IIO device, but each input pin may be a voltage channel. The driver registers one `struct iio_dev` and gives the core an array of `struct iio_chan_spec` entries describing the channels.

**API / Code Anchor:**  
`indio_dev->channels`, `indio_dev->num_channels`, `struct iio_chan_spec`.

**Production or Debugging Angle:**  
If the channel table is wrong, userspace sees wrong or missing files. This is often a channel-description bug, not a sysfs bug.

**Common Traps:**  
- Creating one IIO device per axis when one chip has several channels.
- Confusing channel index with scan index.
- Forgetting modifiers such as `IIO_MOD_X`.

**Follow-up Questions:**  
- How would you represent a 4-channel ADC?
- How would you represent a 3-axis accelerometer?
- When would you use `indexed` versus `modified`?

### 3. How does `in_accel_x_raw` get created?

**Level:** Beginner

**Short Answer:**  
The IIO core generates it from the channel type, modifier, direction, and info mask in `struct iio_chan_spec`.

**Deep Explanation:**  
The driver does not manually create `in_accel_x_raw`. It describes a channel with type `IIO_ACCEL`, marks it modified with `channel2 = IIO_MOD_X`, and sets `info_mask_separate = BIT(IIO_CHAN_INFO_RAW)`. The core then creates the standard attribute name.

**API / Code Anchor:**  
`IIO_ACCEL`, `IIO_MOD_X`, `info_mask_separate`, `IIO_CHAN_INFO_RAW`.

**Production or Debugging Angle:**  
If a file is missing, inspect the channel table before blaming userspace. The generated ABI is only as correct as the channel description.

**Common Traps:**  
- Manually adding duplicate sysfs attributes.
- Putting shared attributes in `info_mask_separate`.
- Using indexes for physical axes when modifiers would produce clearer names.

**Follow-up Questions:**  
- What creates `in_voltage0_raw`?
- What creates `in_accel_scale`?
- Which field controls the `x`, `y`, or `z` suffix?

### 4. What is `read_raw()` used for?

**Level:** Beginner

**Short Answer:**  
`read_raw()` is the driver callback used by the IIO core when userspace reads standard channel attributes such as raw values or scale.

**Deep Explanation:**  
When userspace reads a generated sysfs file, the IIO core calls `read_raw()` with the target channel and a mask telling the driver which value is requested. The driver reads hardware or returns a known setting, fills `val` and maybe `val2`, then returns an `IIO_VAL_*` format code.

**API / Code Anchor:**  
`struct iio_info.read_raw`, `IIO_CHAN_INFO_RAW`, `IIO_CHAN_INFO_SCALE`, `IIO_VAL_INT`, `IIO_VAL_INT_PLUS_MICRO`.

**Production or Debugging Angle:**  
Bad return formats cause userspace to receive wrong values even if the hardware register read is correct.

**Common Traps:**  
- Returning `0` instead of an `IIO_VAL_*` code on success.
- Forgetting `val2` for fractional values.
- Ignoring `mask` and returning the same thing for raw and scale.

**Follow-up Questions:**  
- When would you implement `write_raw()`?
- What should `read_raw()` return for unsupported masks?
- Why might `read_raw()` return `-EBUSY`?

## Mid-level

### 5. What are raw, scale, offset, and processed values?

**Level:** Mid-level

**Short Answer:**  
Raw is the direct hardware-like sample. Scale and offset describe conversion. Processed is a driver-provided converted value.

**Deep Explanation:**  
Many IIO users calculate a physical value as `(raw + offset) * scale`. For example, an ADC raw count multiplied by `in_voltage_scale` gives a voltage. Some devices expose `IIO_CHAN_INFO_PROCESSED` when the hardware or driver already returns a converted measurement.

**API / Code Anchor:**  
`IIO_CHAN_INFO_RAW`, `IIO_CHAN_INFO_SCALE`, `IIO_CHAN_INFO_OFFSET`, `IIO_CHAN_INFO_PROCESSED`.

**Production or Debugging Angle:**  
Wrong scale or offset can make a driver look electrically broken. Always compare with the datasheet resolution, reference voltage, gain, and sign-extension rules.

**Common Traps:**  
- Treating raw as a physical unit.
- Returning scale in the wrong `IIO_VAL_*` format.
- Forgetting sign extension before reporting raw accelerometer values.

**Follow-up Questions:**  
- How would you calculate voltage from a 12-bit ADC?
- What does `IIO_VAL_INT_PLUS_MICRO` mean?
- When is `processed` better than raw plus scale?

### 6. What is the difference between direct mode and triggered buffered capture?

**Level:** Mid-level

**Short Answer:**  
Direct mode reads one attribute through sysfs. Triggered buffered capture samples selected channels repeatedly when a trigger fires and streams binary data through `/dev/iio:deviceX`.

**Deep Explanation:**  
Direct reads are simple and good for occasional values. Buffered capture is for repeated samples, synchronized channels, or higher rates. Userspace selects scan elements, chooses a trigger, sets buffer length, enables the buffer, and reads the character device.

**API / Code Anchor:**  
Direct: `INDIO_DIRECT_MODE`, `read_raw()`.  
Buffered: `iio_triggered_buffer_setup()`, `active_scan_mask`, `iio_push_to_buffers_with_timestamp()`, `scan_elements`.

**Production or Debugging Angle:**  
The driver must stop direct register reads from racing the trigger handler if both paths access the same hardware. Returning `-EBUSY` from `read_raw()` while the buffer is enabled is common.

**Common Traps:**  
- Assuming sysfs reads use the same data path as `/dev/iio:deviceX`.
- Enabling buffer before selecting scan elements and length.
- Parsing binary buffer data without reading `scan_elements/*_type`.

**Follow-up Questions:**  
- What is a scan element?
- Why do buffers need triggers?
- How does userspace choose which channels are buffered?

### 7. What is an IIO trigger?

**Level:** Mid-level

**Short Answer:**  
An IIO trigger is an event source that tells one or more IIO devices when to capture a sample.

**Deep Explanation:**  
A trigger may come from a data-ready interrupt, hrtimer, sysfs trigger, or another hardware event. The trigger is not necessarily part of the sensor being sampled. When the trigger fires, the IIO core invokes the poll function and the threaded handler pushes data into the buffer.

**API / Code Anchor:**  
`struct iio_trigger`, `trigger/current_trigger`, `iio_pollfunc_store_time`, `iio_trigger_notify_done()`.

**Production or Debugging Angle:**  
If buffered reads block forever, confirm the trigger exists, is assigned to the device, fires, and the handler notifies completion.

**Common Traps:**  
- Thinking the sensor must own the trigger.
- Forgetting `iio_trigger_notify_done()` on error paths.
- Doing slow bus I/O in a hard IRQ top half.

**Follow-up Questions:**  
- What is a sysfs trigger?
- What is an hrtimer trigger useful for?
- Can one trigger drive more than one IIO device?

### 8. How does userspace decode IIO buffered samples?

**Level:** Mid-level

**Short Answer:**  
Userspace reads the enabled scan elements and their `*_type` files, then decodes `/dev/iio:deviceX` according to scan index, storage bits, sign, shift, endian, and timestamp layout.

**Deep Explanation:**  
The buffer is binary, not text. `scan_index` determines order. `scan_type` describes how each value is packed. A type string like `le:s12/16>>4` means little-endian signed data, 12 valid bits stored in 16 bits, shifted right by 4 before masking.

**API / Code Anchor:**  
`struct iio_chan_spec.scan_index`, `scan_type`, `/sys/bus/iio/devices/iio:deviceX/scan_elements/*_type`.

**Production or Debugging Angle:**  
Many "wrong sensor values" are userspace decoding bugs or driver `scan_type` bugs. `generic_buffer` is useful because it follows IIO metadata.

**Common Traps:**  
- Reading `/dev/iio:deviceX` with `cat` and expecting text.
- Ignoring endian or shift.
- Assuming sysfs attribute order is buffer order.

**Follow-up Questions:**  
- What does `storagebits` mean?
- Why might `realbits` be smaller than `storagebits`?
- How are timestamps represented?

### 9. How should runtime PM be used in an IIO driver?

**Level:** Mid-level

**Short Answer:**  
Resume the device before hardware access, keep it active during reads or streaming, then mark last busy and autosuspend when done.

**Deep Explanation:**  
Sensors are often idle most of the time. `read_raw()` may need to resume the chip, trigger a conversion, read the value, and release the PM reference. Buffered capture usually needs the device active for the entire buffer session. Runtime suspend must not race the trigger handler or sysfs access.

**API / Code Anchor:**  
`pm_runtime_resume_and_get()`, `pm_runtime_mark_last_busy()`, `pm_runtime_put_autosuspend()`, `iio_buffer_enabled()`.

**Production or Debugging Angle:**  
If the first read works but later reads fail after idle, suspect missing runtime PM gets or bad restore state.

**Common Traps:**  
- Letting autosuspend occur during active buffered capture.
- Accessing registers in `read_raw()` while runtime suspended.
- Returning early without balancing PM usage counts.

**Follow-up Questions:**  
- Should buffer enable take a runtime PM reference?
- What happens during remove if autosuspend work is pending?
- How would you debug a sensor that fails only after idle?

## Senior

### 10. How do you design locking for an IIO driver with sysfs reads, buffered capture, IRQs, and PM?

**Level:** Senior

**Short Answer:**  
Define one clear ownership model for hardware access and shared buffers, then use locks and PM references consistently across `read_raw()`, `write_raw()`, trigger handlers, IRQ paths, PM callbacks, and remove.

**Deep Explanation:**  
IIO drivers often have multiple entry points: sysfs reads, sysfs writes, buffer enable/disable callbacks, trigger handlers, event IRQs, runtime suspend/resume, and remove. The design must prevent direct reads from racing streaming, prevent PM suspend during active hardware access, and prevent remove from freeing state while callbacks are still running.

**API / Code Anchor:**  
`mutex`, `iio_buffer_enabled()`, `iio_device_unregister()`, `iio_triggered_buffer_cleanup()`, `pm_runtime_resume_and_get()`, `iio_trigger_notify_done()`.

**Production or Debugging Angle:**  
Rare crashes during unbind or buffer stop are usually lifetime/locking bugs, not IIO core bugs.

**Common Traps:**  
- Using a mutex in a hard IRQ top half.
- Protecting sysfs reads but not trigger handlers.
- Unregistering hardware resources before unregistering the IIO device.
- Forgetting that PM callbacks also touch hardware state.

**Follow-up Questions:**  
- Which lock protects register sequences?
- How do you stop userspace entry before remove?
- What state machine would you use for streaming?

### 11. Why should `iio_device_register()` happen late in probe?

**Level:** Senior

**Short Answer:**  
Registration exposes userspace entry points, so the device must be fully initialized before calling it.

**Deep Explanation:**  
After registration, userspace may read sysfs attributes, enable buffers, assign triggers, or open `/dev/iio:deviceX`. If channels, callbacks, locks, PM state, buffers, or resources are incomplete, userspace can enter half-initialized driver code.

**API / Code Anchor:**  
`devm_iio_device_alloc()`, channel setup, `iio_triggered_buffer_setup()`, `iio_device_register()`.

**Production or Debugging Angle:**  
Probe races are hard to reproduce because they depend on udev, monitoring daemons, or test scripts touching sysfs immediately after registration.

**Common Traps:**  
- Registering before triggered-buffer setup.
- Registering before runtime PM state is coherent.
- Registering before validating chip ID.

**Follow-up Questions:**  
- What should happen if `iio_device_register()` fails after buffer setup?
- What should remove unregister first?
- How do devm helpers affect cleanup order?

### 12. How would you review an IIO channel table?

**Level:** Senior

**Short Answer:**  
Check that the channel table accurately describes the hardware and the intended userspace ABI: type, index/modifier, info masks, scan index, scan type, and shared attributes.

**Deep Explanation:**  
The channel table is not just internal metadata. It creates ABI files and describes binary buffers. A wrong modifier creates confusing names. A wrong info mask exposes too many or too few files. A wrong scan type causes userspace to decode samples incorrectly forever.

**API / Code Anchor:**  
`struct iio_chan_spec`, `type`, `indexed`, `modified`, `channel`, `channel2`, `info_mask_*`, `scan_index`, `scan_type`.

**Production or Debugging Angle:**  
Changing channel names or scan layout after release can break applications. Treat the table like ABI design.

**Common Traps:**  
- Using custom names instead of standard IIO types/modifiers.
- Making scale separate when all same-type channels share it.
- Setting `scan_index = -1` for a channel expected in buffers.
- Copying stale `scan_type` fields from old examples without checking current headers.

**Follow-up Questions:**  
- How do you represent differential channels?
- What makes a channel output?
- How do you handle unsupported scan combinations?

### 13. A buffered read blocks forever. How do you debug it?

**Level:** Senior

**Short Answer:**  
Check trigger assignment, trigger firing, scan elements, buffer enable state, handler execution, and whether the handler always calls `iio_trigger_notify_done()`.

**Deep Explanation:**  
Buffered capture needs several pieces: a valid trigger, selected scan elements, valid scan mask, buffer length, enabled buffer, a firing trigger, and a driver handler that pushes samples. If any part is missing, userspace may block waiting for data.

**API / Code Anchor:**  
`trigger/current_trigger`, `scan_elements/*_en`, `buffer/length`, `buffer/enable`, `iio_push_to_buffers_with_timestamp()`, `iio_trigger_notify_done()`.

**Production or Debugging Angle:**  
Use `lsiio`, `generic_buffer`, `dmesg`, dynamic debug, IRQ counters, and simple sysfs trigger tests to isolate whether the issue is userspace setup, trigger delivery, or driver handler logic.

**Common Traps:**  
- Enabling buffer before scan elements and length.
- Assigning a trigger name that does not exist.
- Returning from handler error paths without notifying done.
- Expecting samples from a data-ready IRQ that is masked in the chip.

**Follow-up Questions:**  
- How would you prove the trigger fires?
- What does `available_scan_masks` reject?
- Why might `generic_buffer` work while a custom app fails?

### 14. How do you decide between IIO, hwmon, input, and V4L2 for a sensor?

**Level:** Senior

**Short Answer:**  
Choose based on userspace semantics and ABI, not only the physical chip. IIO fits measurement channels and buffered samples; hwmon fits simple monitoring; input fits user-input events; V4L2 fits camera/video sensors.

**Deep Explanation:**  
The same word "sensor" can mean different Linux subsystems. A temperature monitor for system health may be hwmon. An accelerometer streaming raw samples is IIO. A touchscreen is input. A camera image sensor is V4L2. The subsystem decides ABI, tools, PM expectations, and application integration.

**API / Code Anchor:**  
IIO: `struct iio_dev`.  
Input: `struct input_dev`.  
hwmon: hwmon registration APIs.  
V4L2: `struct v4l2_subdev`.

**Production or Debugging Angle:**  
Wrong subsystem choice causes long-term ABI pain even if the driver "works" during bring-up.

**Common Traps:**  
- Putting a button or touchscreen axis in IIO.
- Putting a camera sensor in IIO because it is physically a sensor.
- Using custom sysfs instead of subsystem ABI.

**Follow-up Questions:**  
- Where would an ambient light sensor belong?
- Where would a fan tachometer belong?
- Where would a camera exposure control belong?

### 15. What are the hardest production issues in IIO drivers?

**Level:** Senior

**Short Answer:**  
The hard issues are ABI correctness, binary buffer layout, concurrent entry points, runtime PM, trigger completion, and remove/unbind lifetime.

**Deep Explanation:**  
Simple raw reads are only the start. Production drivers must keep ABI stable, decode samples correctly, avoid direct-read versus streaming races, keep hardware powered while needed, restore state after suspend, handle trigger errors, and unregister users before freeing resources.

**API / Code Anchor:**  
`struct iio_chan_spec`, `iio_buffer_enabled()`, `iio_triggered_buffer_setup()`, `iio_trigger_notify_done()`, `pm_runtime_*()`, `iio_device_unregister()`.

**Production or Debugging Angle:**  
Review should include invalid userspace sequences, stress tests for enable/disable, runtime PM idle testing, suspend/resume, and remove while userspace is active.

**Common Traps:**  
- Treating the first successful `cat in_*_raw` as complete.
- Testing only direct mode and not buffers.
- Ignoring suspend/resume and autosuspend.
- Freeing state while a trigger handler can still run.

**Follow-up Questions:**  
- What tests would you run before merging an IIO driver?
- How do you keep sample layout ABI stable?
- How do you handle hardware that loses register state in suspend?

## Debugging Scenarios

### Scenario A: `in_voltage0_raw` exists but always returns `-EINVAL`

**Short Answer:**  
The channel exists, but `read_raw()` probably does not handle the requested mask/channel combination correctly.

**Deep Explanation:**  
The sysfs file was generated from the channel table, so registration worked. The failure happens when the core calls `read_raw()` with `IIO_CHAN_INFO_RAW`. Check the callback switch, channel `address`, and whether the driver rejects the channel.

**API / Code Anchor:**  
`read_raw()`, `IIO_CHAN_INFO_RAW`, `chan->address`, `return -EINVAL`.

**Production or Debugging Angle:**  
Add temporary debug logs showing `mask`, `chan->type`, `chan->channel`, and `chan->address`.

**Common Traps:**  
- Handling scale but not raw.
- Comparing `chan->channel` when the driver stored the register in `chan->address`.
- Returning `-EINVAL` for a transient bus error instead of a real bus errno.

**Follow-up Questions:**  
- What should unsupported masks return?
- How would you test each generated attribute?
- Why is preserving errno useful?

### Scenario B: Raw reads work, but buffered values are wrong

**Short Answer:**  
Suspect `scan_type`, `scan_index`, sign extension, endian, shift, or userspace parser logic.

**Deep Explanation:**  
Direct raw reads and buffered reads can use different formatting paths. Buffered data is packed binary, so the driver's scan metadata must match the actual bytes pushed. Userspace must parse according to `scan_elements/*_type`, not assumptions.

**API / Code Anchor:**  
`scan_index`, `scan_type`, `iio_push_to_buffers_with_timestamp()`, `scan_elements/*_type`.

**Production or Debugging Angle:**  
Compare one direct raw reading with one triggered sample at a slow rate. Use `generic_buffer` as a known-good parser.

**Common Traps:**  
- Forgetting right shift before masking valid bits.
- Treating signed data as unsigned.
- Ignoring timestamp padding/alignment.

**Follow-up Questions:**  
- What does `le:s12/16>>4` mean?
- How would you verify buffer byte order?
- Why might timestamp position matter?

### Scenario C: The driver crashes during module unload while a buffer is active

**Short Answer:**  
Remove/unbind ordering likely allows userspace, trigger handlers, or IRQ/work paths to run after resources are being freed.

**Deep Explanation:**  
The IIO device should be unregistered before hardware resources vanish, and buffer/trigger/IRQ/work paths must be stopped or synchronized. If cleanup order is wrong, a trigger handler may dereference freed private data or access powered-off hardware.

**API / Code Anchor:**  
`iio_device_unregister()`, `iio_triggered_buffer_cleanup()`, `free_irq()` or managed IRQ lifetime, work cancellation, runtime PM disable.

**Production or Debugging Angle:**  
Stress with a loop that enables the buffer, reads samples, and unbinds/rebinds the driver. Use KASAN/lockdep when available.

**Common Traps:**  
- Assuming devm cleanup order automatically matches subsystem lifetime.
- Disabling clocks before unregistering userspace entry points.
- Not synchronizing threaded IRQ or trigger handlers.

**Follow-up Questions:**  
- What should remove do first?
- How do you prevent new userspace entry?
- What work/IRQ paths need cancellation?
