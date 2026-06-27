```bash
# Chapter 10 - IIO Framework Industrial I/O (IIO) is a kernel subsystem dedicated to analog-to-digital converters
```
(ADCs) and digital-to-analog converters (DACs). With the growing number of sensors
(measurement devices with analogue-to-digital, or digital-to-analogue, capabilities) with different code implementations scattered over the kernel sources, gathering them became necessary. This is what the IIO framework does, in a generic and homogeneous way.
Jonathan Cameron and the Linux IIO community have been developing it since 2009.
Accelerometers, gyroscopes, current/voltage measurement chips, light sensors, pressure sensors, and so on all fall into the IIO family of devices.
The IIO model is based on a device and channel architecture:
Device represents the chip itself. It is the top level of the hierarchy.
Channel represents a single acquisition line of the device. A device may have one or more channels. For example, an accelerometer is a device with three channels,
one for each axis (X, Y, and Z).
The IIO chip is the physical and hardware sensor/converter. It is exposed to the user space as a character device (when triggered buffering is supported), and a sysfs directory entry that will contain a set of files, some of which represent the channels. A single channel is represented with a single sysfs file entry.
These are two ways to interact with an IIO driver from the user space:
/sys/bus/iio/iio:deviceX/: This represents the sensor along with its channels
/dev/iio:deviceX: This is a character device that exports the device's events and data buffer
## IIO framework architecture and layout
The preceding diagram shows how the IIO framework is organized between kernel and user space. The driver manages the hardware and report processing to the IIO core, using a set of facilities and APIs exposed by the IIO core. The IIO subsystem then abstracts the whole underlying mechanism to the user space by means of the sysfs interface and character device, on top of which users can execute system calls.
IIO APIs are spread over several header files, listed as follows:
```c
#include <linux/iio/iio.h> /* mandatory */
#include <linux/iio/sysfs.h> /* mandatory since sysfs is used */
#include <linux/iio/events.h> /* For advanced users, to manage iio events
```
*/
```c
#include <linux/iio/buffer.h> /* mandatory to use triggered buffers */
#include <linux/iio/trigger.h>/* Only if you implement trigger in your driver (rarely used)*/
```
In this chapter, we will describe and handle every concept of the IIO framework, including:
```c
A walk through its data structure (device, channel, and so on)
```
Triggered buffer support and continuous capture, along with its sysfs interface
Exploring existing IIO triggers
Capturing data in either one-shot mode or continuous mode
Listing available tools that can help developers to test their devices
## IIO data structures
An IIO device is represented in the kernel as an instance of struct iio_dev, and described by a struct iio_info structure. All of the important IIO structures are defined in include/linux/iio/iio.h.
## iio_dev structure
This structure represents the IIO device, describing the device and the driver. It tells us about:
How many channels are available on the device
What modes can the device operate in: one-shot, triggered buffer
```dts
What hooks are available for this driver struct iio_dev {
```
[...]
```c
int modes;
int currentmode;
struct device dev;
struct iio_buffer *buffer;
int scan_bytes;
const unsigned long *available_scan_masks;
const unsigned long *active_scan_mask;
bool scan_timestamp;
struct iio_trigger *trig;
struct iio_poll_func *pollfunc;
struct iio_chan_spec const *channels;
int num_channels;
const char *name;
const struct iio_info *info;
const struct iio_buffer_setup_ops *setup_ops;
struct cdev chrdev;
};
```
The complete structure is defined in the IIO header file. Fields that we are not interested in are removed here:
modes: This represents the different modes supported by the device. Supported modes are:
INDIO_DIRECT_MODE, which says the device provides sysfs-type interfaces.
INDIO_BUFFER_TRIGGERED says that the device supports hardware triggers. This mode is automatically added to your device when you set up a trigger buffer using the iio_triggered_buffer_setup() function.
INDIO_BUFFER_HARDWARE shows the device has a hardware buffer.
INDIO_ALL_BUFFER_MODES is the union of the previous two.
currentmode: This represents the mode actually used by the device.
dev: This represents the struct device (according to the Linux device model) the
IIO device is tied to.
buffer: This is your data buffer, pushed to the user space when using triggered buffer mode. It is automatically allocated and associated to your device when you enable trigger buffer support using the iio_triggered_buffer_setup function.
scan_bytes: This is the number of bytes captured to be fed to the buffer. When using a trigger buffer from the user space, the buffer should be at least indio->scan_bytes bytes large.
available_scan_masks: This is an optional array of allowed bit masks. When using a trigger buffer, you can enable channels to be captured and fed into the
IIO buffer. If you do not want to allow some channels to be enabled, you should fill this array with only allowed ones. The following is an example of providing a scan mask for an accelerometer (with X, Y, and Z channels):
/*
```c
* Bitmasks 0x7 (0b111) and 0 (0b000) are allowed.
```
* It means one can enable none or all of them.
* one can't for example enable only channel X and Y
*/
```c
static const unsigned long my_scan_masks[] = {0x7, 0};
```
indio_dev->available_scan_masks = my_scan_masks;
active_scan_mask: This is a bitmask of enabled channels. Only the data from those channels should be pushed into the buffer. For example, for an eightchannel ADC converter, if you only enable the first (0), third (2), and last (7)
```c
channels, the bitmask would be 0b10000101 (0x85). active_scan_mask will be set to 0x85. The driver can then use the for_each_set_bit macro to walk through each set bit, fetch the data according to the channel, and fill the buffer.
```
scan_timestamp: This tells us whether to push the capture timestamp into the buffer or not. If true, the timestamp will be pushed as the last element of the buffer. The timestamp is 8 bytes (64 bits) large.
trig: This is the current device trigger (when buffer mode is supported).
pollfunc: This is the function run on the trigger being received.
channels: This represents the table channel specification structure, to describe every channel the device has.
num_channels: This represents the number of channels specified in channels.
name: This represents the device name.
info: Callbacks and constant information from the driver.
setup_ops: Set of callback functions to call before and after the buffer is enabled/disabled. This structure is defined in include/linux/iio/iio.h and shown as follows:
```dts
struct iio_buffer_setup_ops {
c
int (* preenable) (struct iio_dev *);
int (* postenable) (struct iio_dev *);
int (* predisable) (struct iio_dev *);
int (* postdisable) (struct iio_dev *);
bool (* validate_scan_mask) (struct iio_dev *indio_dev,
const unsigned long *scan_mask);
};
```
setup_ops: If this is not specified, the IIO core uses the default iio_triggered_buffer_setup_ops defined in drivers/iio/buffer/industrialio-triggered-buffer.c.
chrdev: This is the associated character device created by the IIO core.
The function used to allocate memory for an IIO device is iio_device_alloc():
```c
struct iio_dev *devm_iio_device_alloc(struct device *dev,
int sizeof_priv)
```
dev is the device for which iio_dev is allocated, and sizeof_priv is the memory space used to allocate for any private structure. This way, passing per-device (private) data structures is quite straightforward. The function returns NULL if the allocation fails:
```c
struct iio_dev *indio_dev;
struct my_private_data *data;
```
indio_dev = iio_device_alloc(sizeof(*data));
```c
if (!indio_dev)
return -ENOMEM;
```
/*data is given the address of reserved memory for private data */
data = iio_priv(indio_dev);
After the IIO device memory has been allocated, the next step is to fill different fields. Once done, you have to register the device with the IIO subsystem using the iio_device_register function:
```c
int iio_device_register(struct iio_dev *indio_dev)
```
The device will be ready to accept requests from the user space after this function executes.
The reverse operation (usually done in the release function) is iio_device_unregister():
```c
void iio_device_unregister(struct iio_dev *indio_dev)
```
Once unregistered, the memory allocated by iio_device_alloc can be freed with iio_device_free:
```c
void iio_device_free(struct iio_dev *iio_dev)
```
Given an IIO device as a parameter, you can retrieve private data in the following manner:
```c
struct my_private_data *the_data = iio_priv(indio_dev);
```
## iio_info structure
The struct iio_info structure is used to declare the hooks used by the IIO core in order to read/write channel/attribute values:
```dts
struct iio_info {
c
struct module *driver_module;
const struct attribute_group *attrs;
int (*read_raw)(struct iio_dev *indio_dev,
struct iio_chan_spec const *chan,
int *val, int *val2, long mask);
int (*write_raw)(struct iio_dev *indio_dev,
struct iio_chan_spec const *chan,
int val, int val2, long mask);
```
[...]
```c
};
```
Fields that we are not interested in have been removed:
driver_module: This is the module structure used to ensure correct ownership of chrdevs, usually set to THIS_MODULE.
attrs: This represents the device's attributes.
read_raw: This is the callback run when the user reads a device's sysfs file attribute. The mask parameter is a bitmask that allows us to know which type of value is requested. The channel parameter lets us know the channel concerned.
It can be for the sampling frequency, the scale used to convert the raw value into a usable value, or the raw value itself.
write_raw: This is the callback used to write values to the device. You can, for example, use it to set the sampling frequency.
The following code shows how to set up a struct iio_info structure:
```dts
static const struct iio_info iio_dummy_info = {
```
.driver_module = THIS_MODULE,
.read_raw = &iio_dummy_read_raw,
.write_raw = &iio_dummy_write_raw,
[...]
/*
* Provide device type specific interface functions and
* constant data.
*/
indio_dev->info = &iio_dummy_info;
## IIO channels
A channel represents a single acquisition line. An accelerometer will have, for example,
three channels (X, Y, Z), since each axis represents a single acquisition line. struct iio_chan_spec is the structure that represents and describes a single channel in the kernel:
```dts
struct iio_chan_spec {
```
enum iio_chan_type type;
```c
int channel;
int channel2;
unsigned long address;
int scan_index;
dts
struct {
```
charsign;
```c
u8 realbits;
u8 storagebits;
u8 shift;
u8 repeat;
```
enum iio_endian endianness;
```c
} scan_type;
long info_mask_separate;
long info_mask_shared_by_type;
long info_mask_shared_by_dir;
long info_mask_shared_by_all;
const struct iio_event_spec *event_spec;
unsigned int num_event_specs;
const struct iio_chan_spec_ext_info *ext_info;
const char *extend_name;
const char *datasheet_name;
unsigned modified:1;
unsigned indexed:1;
unsigned output:1;
unsigned differential:1;
};
```
The following are the meanings of each element in the structure:
type: This specifies which type of measurement the channel makes. In case of voltage measurement, it should be IIO_VOLTAGE. For a light sensor, it is
IIO_LIGHT. For an accelerometer, IIO_ACCEL is used. All available types are defined in include/uapi/linux/iio/types.h, as enum iio_chan_type. To write drivers for a given converter, look in that file to see the type that each of your channels falls into.
channel: This specifies the channel index when .indexed is set to 1.
channel2: This specifies the channel modifier when .modified is set to 1.
modified: This specifies whether a modifier is to be applied to this channel attribute name or not. In that case, the modifier is set in .channel2. (For example, IIO_MOD_X, IIO_MOD_Y, IIO_MOD_Z are modifiers for axial sensors about the xyz axes.) The available modifier list is defined in the kernel IIO header as enum iio_modifier. Modifiers only mangle the channel attribute name in sysfs, not the value.
indexed: This specifies whether the channel attribute name has an index or not.
If yes, the index is specified in the .channel field.
scan_index and scan_type: These fields are used to identify elements from a buffer, when using buffer triggers. scan_index sets the position of the captured channel inside the buffer. Channels with a lower scan_index will be placed before channels with a higher index. Setting .scan_index to -1 will prevent the channel from buffered capture (that is, when there is no entry in the scan_elements directory).
Channel sysfs attributes exposed to user space are specified in the form of bitmasks.
Depending on their shared information, attributes can be set into one of the following masks:
info_mask_separate marks the attributes as being specific to this channel.
info_mask_shared_by_type marks the attribute as being shared by all channels of the same type. The information exported is shared by all channels of the same type.
info_mask_shared_by_dir marks the attribute as being shared by all channels of the same direction. The information exported is shared by all channels of the same direction.
info_mask_shared_by_all marks the attribute as being shared by all channels,
whatever their type or direction may be. The information exported is shared by all channels. Bitmasks for enumeration of those attributes are all defined in include/linux/iio/iio.h:
```dts
enum iio_chan_info_enum {
```
IIO_CHAN_INFO_RAW = 0,
IIO_CHAN_INFO_PROCESSED,
IIO_CHAN_INFO_SCALE,
IIO_CHAN_INFO_OFFSET,
IIO_CHAN_INFO_CALIBSCALE,
[...]
IIO_CHAN_INFO_SAMP_FREQ,
IIO_CHAN_INFO_FREQUENCY,
IIO_CHAN_INFO_PHASE,
IIO_CHAN_INFO_HARDWAREGAIN,
IIO_CHAN_INFO_HYSTERESIS,
[...]
```c
};
```
The endian field should be one of:
```dts
enum iio_endian {
```
IIO_CPU,
IIO_BE,
IIO_LE,
```c
};
```
## Channel attribute naming conventions
The attribute's name is automatically generated by the IIO core with the pattern {direction}_{type}_{index}_{modifier}_{info_mask}:
direction corresponds to the attribute direction, according to the struct iio_direction structure in drivers/iio/industrialio-core.c:
```dts
static const char * const iio_direction[] = {
```
[0] = "in",
[1] = "out",
```c
};
```
type corresponds to the channel type, according to the char array const iio_chan_type_name_spec:
```dts
static const char * const iio_chan_type_name_spec[] = {
```
[IIO_VOLTAGE] = "voltage",
[IIO_CURRENT] = "current",
[IIO_POWER] = "power",
[IIO_ACCEL] = "accel",
[...]
[IIO_UVINDEX] = "uvindex",
[IIO_ELECTRICALCONDUCTIVITY] = "electricalconductivity",
[IIO_COUNT] = "count",
[IIO_INDEX] = "index",
[IIO_GRAVITY] = "gravity",
```c
};
```
The index pattern depends on the channel's .indexed field being set or not. If set, the index will be taken from the .channel field in order to replace the
```c
{index} pattern.
```
The modifier pattern depends on the channel's .modified field being set or not. If set, the modifier will be taken from the .channel2 field, and the
```c
{modifier} pattern will be replaced according to the char array struct iio_modifier_names structure:
dts
static const char * const iio_modifier_names[] = {
```
[IIO_MOD_X] = "x",
[IIO_MOD_Y] = "y",
[IIO_MOD_Z] = "z",
[IIO_MOD_X_AND_Y] = "x&y",
[IIO_MOD_X_AND_Z] = "x&z",
[IIO_MOD_Y_AND_Z] = "y&z",
[...]
[IIO_MOD_CO2] = "co2",
[IIO_MOD_VOC] = "voc",
```c
};
```
info_mask depends on the channel info mask, private or shared, indexing a value in the char array iio_chan_info_postfix:
/* relies on pairs of these shared then separate */
```dts
static const char * const iio_chan_info_postfix[] = {
```
[IIO_CHAN_INFO_RAW] = "raw",
[IIO_CHAN_INFO_PROCESSED] = "input",
[IIO_CHAN_INFO_SCALE] = "scale",
[IIO_CHAN_INFO_CALIBBIAS] = "calibbias",
[...]
[IIO_CHAN_INFO_SAMP_FREQ] = "sampling_frequency",
[IIO_CHAN_INFO_FREQUENCY] = "frequency",
[...]
```c
};
```
## Distinguishing channels
You may find yourself in trouble when there are multiple data channels per channel type.
The dilemma would be how to identify them. There are two solutions for that: indexes and modifiers.
Using indexes: Given an ADC device with one channel line, indexing is not needed. Its channel definition would be:
```dts
static const struct iio_chan_spec adc_channels[] = {
{
```
.type = IIO_VOLTAGE,
.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
```c
},
}
```
The attribute name resulting from the preceding channel would be in_voltage_raw:
/sys/bus/iio/iio:deviceX/in_voltage_raw
Now, let's say the converter has four or even eight channels. How do we identify them? The solution is to use indexes. Setting the .indexed field to 1 will mangle the channel attribute name, with the .channel value replacing the {index} pattern:
```dts
static const struct iio_chan_spec adc_channels[] = {
{
```
.type = IIO_VOLTAGE,
.indexed = 1,
.channel = 0,
.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
```c
},
dts
{
```
.type = IIO_VOLTAGE,
.indexed = 1,
.channel = 1,
.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
```c
},
dts
{
```
.type = IIO_VOLTAGE,
.indexed = 1,
.channel = 2,
.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
```c
},
dts
{
```
.type = IIO_VOLTAGE,
.indexed = 1,
.channel = 3,
.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
```c
},
}
```
The resulting channel attributes are:
/sys/bus/iio/iio:deviceX/in_voltage0_raw
/sys/bus/iio/iio:deviceX/in_voltage1_raw
/sys/bus/iio/iio:deviceX/in_voltage2_raw
/sys/bus/iio/iio:deviceX/in_voltage3_raw
Using modifiers: Given a light sensor with two channels, one for infrared light and one for both infrared and visible light, without index or modifier, an attribute name would be in_intensity_raw. Using indexes here can be error-prone, because it makes no sense to have in_intensity0_ir_raw and in_intensity1_ir_raw. Using modifiers will help to provide meaningful attribute names. The channel's definition could look as follows:
```dts
static const struct iio_chan_spec mylight_channels[] = {
{
```
.type = IIO_INTENSITY,
.modified = 1,
.channel2 = IIO_MOD_LIGHT_IR,
.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
.info_mask_shared = BIT(IIO_CHAN_INFO_SAMP_FREQ),
```c
},
dts
{
```
.type = IIO_INTENSITY,
.modified = 1,
.channel2 = IIO_MOD_LIGHT_BOTH,
.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),
.info_mask_shared = BIT(IIO_CHAN_INFO_SAMP_FREQ),
```c
},
dts
{
```
.type = IIO_LIGHT,
.info_mask_separate = BIT(IIO_CHAN_INFO_PROCESSED),
.info_mask_shared = BIT(IIO_CHAN_INFO_SAMP_FREQ),
```c
},
}
```
The resulting attributes will be:
/sys/bus/iio/iio:deviceX/in_intensity_ir_raw for the channel,
measuring IR intensity
/sys/bus/iio/iio:deviceX/in_intensity_both_raw for the channel,
measuring both infrared and visible light
/sys/bus/iio/iio:deviceX/in_illuminance_input for the processed data
/sys/bus/iio/iio:deviceX/sampling_frequency for the sampling frequency, shared by all
This is valid with an accelerometer too, as we will see further on in the case study. For now,
let's summarize what we have discussed so far in a dummy IIO driver.
## Putting it all together
Let's summarize what we have seen so far in a simple dummy driver, which will expose four voltage channels. We will ignore read() and write() functions:
```c
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/events.h>
#include <linux/iio/buffer.h>
#define FAKE_VOLTAGE_CHANNEL(num) \
{ \
```
.type = IIO_VOLTAGE, \
.indexed = 1, \
.channel = (num), \
.address = (num), \
.info_mask_separate = BIT(IIO_CHAN_INFO_RAW), \
.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE) \
```c
}
dts
struct my_private_data {
c
int foo;
int bar;
struct mutex lock;
};
static int fake_read_raw(struct iio_dev *indio_dev,
struct iio_chan_spec const *channel, int *val,
int *val2, long mask)
dts
{
c
return 0;
}
static int fake_write_raw(struct iio_dev *indio_dev,
struct iio_chan_spec const *chan,
int val, int val2, long mask)
dts
{
c
return 0;
}
dts
static const struct iio_chan_spec fake_channels[] = {
```
FAKE_VOLTAGE_CHANNEL(0),
FAKE_VOLTAGE_CHANNEL(1),
FAKE_VOLTAGE_CHANNEL(2),
FAKE_VOLTAGE_CHANNEL(3),
```c
};
dts
static const struct of_device_id iio_dummy_ids[] = {
{ .compatible = "packt,iio-dummy-random", },
c
{ /* sentinel */ }
};
dts
static const struct iio_info fake_iio_info = {
```
.read_raw = fake_read_raw,
.write_raw = fake_write_raw,
.driver_module = THIS_MODULE,
```c
};
static int my_pdrv_probe (struct platform_device *pdev)
dts
{
c
struct iio_dev *indio_dev;
struct my_private_data *data;
```
indio_dev = devm_iio_device_alloc(&pdev->dev, sizeof(*data));
```dts
if (!indio_dev) {
c
dev_err(&pdev->dev, "iio allocation failed!\n");
return -ENOMEM;
}
```
data = iio_priv(indio_dev);
```c
mutex_init(&data->lock);
```
indio_dev->dev.parent = &pdev->dev;
indio_dev->info = &fake_iio_info;
indio_dev->name = KBUILD_MODNAME;
indio_dev->modes = INDIO_DIRECT_MODE;
indio_dev->channels = fake_channels;
indio_dev->num_channels = ARRAY_SIZE(fake_channels);
indio_dev->available_scan_masks = 0xF;
```c
iio_device_register(indio_dev);
platform_set_drvdata(pdev, indio_dev);
return 0;
}
static void my_pdrv_remove(struct platform_device *pdev)
dts
{
c
struct iio_dev *indio_dev = platform_get_drvdata(pdev);
iio_device_unregister(indio_dev);
}
dts
static struct platform_driver mypdrv = {
```
.probe = my_pdrv_probe,
.remove = my_pdrv_remove,
```dts
.driver = {
```
.name = "iio-dummy-random",
.of_match_table = of_match_ptr(iio_dummy_ids),
.owner = THIS_MODULE,
```c
},
};
module_platform_driver(mypdrv);
MODULE_AUTHOR("John Madieu <john.madieu@gmail.com>");
MODULE_LICENSE("GPL");
```
After loading this module, we will have the following output, showing that our device really corresponds to the platform device we have registered:
~# ls -l /sys/bus/iio/devices/
lrwxrwxrwx 1 root root 0 Jul 31 20:26 iio:device0 ->
../../../devices/platform/iio-dummy-random.0/iio:device0
lrwxrwxrwx 1 root root 0 Jul 31 20:23 iio_sysfs_trigger ->
../../../devices/iio_sysfs_trigger
The following listing shows the channels that this device has, along with its name, which correspond exactly to what we have described in the driver:
~# ls /sys/bus/iio/devices/iio\:device0/
dev in_voltage2_raw name uevent in_voltage0_raw in_voltage3_raw power in_voltage1_raw in_voltage_scale subsystem
~# cat /sys/bus/iio/devices/iio:device0/name iio_dummy_random
## Triggered buffer support
In many data analysis applications, it is useful to be able to capture data based on some external signal (trigger). These triggers might be:
A data-ready signal
```c
An IRQ line connected to some external system (GPIO or something else)
```
On-processor periodic interrupt
User space reading/writing a specific file in sysfs
IIO device drivers are completely unrelated to triggers. A trigger may initialize data capture on one or many devices. These triggers are used to fill buffers, exposed to the user space as character devices.
You can develop your own trigger driver, but that is beyond the scope of this book. We will try to focus on existing ones only. These are:
iio-trig-interrupt: This provides support for using any IRQ as IIO triggers.
In old kernel versions, it used to be iio-trig-gpio. The kernel option to enable this trigger mode is CONFIG_IIO_INTERRUPT_TRIGGER. If built as a module, the module would be called iio-trig-interrupt.
iio-trig-hrtimer: This has provided a frequency-based IIO trigger using HRT
as the interrupt source (since kernel v4.5). In older kernel versions, it used to be iio-trig-rtc. The kernel option responsible for this trigger mode is
IIO_HRTIMER_TRIGGER. If built as a module, the module would be called iiotrig-hrtimer.
iio-trig-sysfs: This allows us to use sysfs entry to trigger data capture.
CONFIG_IIO_SYSFS_TRIGGER is the kernel option to add support for this trigger mode.
iio-trig-bfin-timer: This allows us to use a blackfin timer for IIO triggering
(still in staging).
IIO exposes APIs so that we can:
Declare any given number of triggers
Choose which channels will have their data pushed into the buffer
When your IIO device provides support for the trigger buffer, you must set iio_dev.pollfunc, which is executed when the trigger fires. This handler has responsibility for finding enabled channels through indio_dev->active_scan_mask,
retrieving their data, and feeding them into indio_dev->buffer using the iio_push_to_buffers_with_timestamp function. As such, buffers and triggers are very connected in the IIO subsystem.
The IIO core provides a set of helper functions to set up triggered buffers that you can find in drivers/iio/industrialio-triggered-buffer.c.
The following are the steps to support triggered buffers from within your driver:
1. Fill an iio_buffer_setup_ops structure if needed:
```dts
const struct iio_buffer_setup_ops sensor_buffer_setup_ops = {
```
.preenable = my_sensor_buffer_preenable,
.postenable = my_sensor_buffer_postenable,
.postdisable = my_sensor_buffer_postdisable,
.predisable = my_sensor_buffer_predisable,
```c
};
```
2. Write the top half associated to the trigger. In 99% of cases, you just have to feed the timestamp associated with the capture:
```c
irqreturn_t sensor_iio_pollfunc(int irq, void *p)
dts
{
```
pf->timestamp = iio_get_time_ns((struct indio_dev *)p);
```c
return IRQ_WAKE_THREAD;
}
```
3. Write the trigger bottom half, which will fetch data from each enabled channel,
and feed it into the buffer:
```c
irqreturn_t sensor_trigger_handler(int irq, void *p)
dts
{
c
u16 buf[8];
int bit, i = 0;
struct iio_poll_func *pf = p;
struct iio_dev *indio_dev = pf->indio_dev;
```
/* one can use lock here to protect the buffer */
/* mutex_lock(&my_mutex); */
/* read data for each active channel */
for_each_set_bit(bit, indio_dev->active_scan_mask,
indio_dev->masklength)
buf[i++] = sensor_get_data(bit)
/*
* If iio_dev.scan_timestamp = true, the capture timestamp
* will be pushed and stored too, as the last element in the
* sample data buffer before pushing it to the device buffers.
*/
```c
iio_push_to_buffers_with_timestamp(indio_dev, buf, timestamp);
```
/* Please unlock any lock */
/* mutex_unlock(&my_mutex); */
/* Notify trigger */
```c
iio_trigger_notify_done(indio_dev->trig);
return IRQ_HANDLED;
}
```
4. Finally, in the probe function, you have to set up the buffer itself, prior to registering the device with iio_device_register():
```c
iio_triggered_buffer_setup(indio_dev, sensor_iio_polfunc,
```
sensor_trigger_handler,
sensor_buffer_setup_ops);
The magic function here is iio_triggered_buffer_setup. This will also give the
INDIO_DIRECT_MODE capability to your device. When a trigger is fired (from the user space) to your device, you have no way of knowing when capture will be fired.
While continuous buffered capture is active, you should prevent the driver (by returning an error) from performing sysfs per-channel data capture (performed by the read_raw()
hook) in order to avoid undetermined behavior, since both the trigger handler and read_raw() hook will try to access the device at the same time. The function used to check whether buffered mode is actually used is iio_buffer_enabled(). The hook will look like this:
```c
static int my_read_raw(struct iio_dev *indio_dev,
const struct iio_chan_spec *chan,
int *val, int *val2, long mask)
dts
{
```
[...]
```dts
switch (mask) {
c
case IIO_CHAN_INFO_RAW:
if (iio_buffer_enabled(indio_dev))
return -EBUSY;
```
[...]
```c
}
```
The iio_buffer_enabled() function simply tests whether the buffer is enabled for a given IIO device.
Let's describe some important things used in the preceding section:
```c
iio_buffer_setup_ops provides buffer setup functions to be called at a fixed step of the buffer configuration sequence (before/after, enable/disable). If not specified, the default iio_triggered_buffer_setup_ops will be given to your device by the IIO core.
```
sensor_iio_pollfunc is the trigger's top half. As with every top half, it runs in interrupt context and must do as little processing as possible. In 99% of cases,
you just have to feed it the timestamp associated with the capture. Once again,
you can use the default IIO iio_pollfunc_store_time function.
sensor_trigger_handler is the bottom half, which runs in a kernel thread,
allowing us to do any processing, including even acquiring mutex or sleep. The heavy processing should take place here. It usually reads data from the device and stores it in the internal buffer together with the timestamp recorded in the top half, and pushes it to your IIO device buffer.
A trigger is mandatory for triggered buffering. It tells the driver when to read the sample from the device and put it into the buffer. Triggered buffering is not mandatory to write IIO device drivers. You can use singleshot capture through sysfs too, by reading the raw attributes of the channel, which will only perform a single conversion (for the channel attribute being read). Buffer mode allows continuous conversions, thus capturing more than one channel in a single shot.
## IIO trigger and sysfs (user space)
There are two locations in sysfs related to triggers:
/sys/bus/iio/devices/triggerY/, which is created once an IIO trigger is registered with the IIO core and corresponds to triggers with index Y. There is at least one attribute in the directory:
name, which is the trigger name that can be later used for association with a device
The /sys/bus/iio/devices/iio:deviceX/trigger/* directory will be automatically created if your device supports a triggered buffer. You can associate a trigger with our device by writing the trigger's name in the current_trigger file.
## Sysfs trigger interface
The sysfs trigger is enabled in the kernel by the CONFIG_IIO_SYSFS_TRIGGER=y config option, which will result in the /sys/bus/iio/devices/iio_sysfs_trigger/ folder being automatically created, and can be used for sysfs trigger management. There will be two files in the directory, add_trigger and remove_trigger. Its driver is in drivers/iio/trigger/iio-trig-sysfs.c.
## add_trigger file
This is used to create a new sysfs trigger. You can create a new trigger by writing a positive value (which will be used as a trigger ID) into that file. It will create the new sysfs trigger,
accessible at /sys/bus/iio/devices/triggerX, where X is the trigger number, for example:
```bash
# echo 2 > add_trigger
```
This will create a new sysfs trigger, accessible at /sys/bus/iio/devices/trigger2. If the trigger with the specified ID is already present in the system, an invalid argument message will be returned. The sysfs trigger name pattern is sysfstrig{ID}. The echo 2
> add_trigger command will create the trigger /sys/bus/iio/devices/trigger2,
whose name is sysfstrig2:
```bash
$ cat /sys/bus/iio/devices/trigger2/name sysfstrig2
```
Each sysfs trigger contains at least one file: trigger_now. Writing 1 into that file will instruct all devices with the corresponding trigger name in their current_trigger to start capture and push data into their respective buffer. Each device buffer must have its size set and must be enabled (echo 1 >
/sys/bus/iio/devices/iio:deviceX/buffer/enable). Assuming that each buffer size has been set correctly (large enough to hold all of your captures), each of them will contain the results of as many captures as you have written 1 into the trigger_now file.
## remove_trigger file
To remove a trigger, the following command is used:
```bash
# echo 2 > remove_trigger
```
## Tying a device with a trigger
Associating a device with a given trigger consists of writing the name of the trigger to the current_trigger file available under the device's trigger directory. For example, let's say we need to tie a device with the trigger that has index 2:
```bash
# set trigger2 as current trigger for device0
# echo sysfstrig2 >
```
/sys/bus/iio/devices/iio:device0/trigger/current_trigger
To detach the trigger from the device, you should write an empty string to the current_trigger file of the device trigger directory, shown as follows:
```bash
# echo "" > iio:device0/trigger/current_trigger
```
We will see further on in the chapter a practical example dealing with the sysfs trigger for data capture.
## The interrupt trigger interface
Consider the following sample:
```dts
static struct resource iio_irq_trigger_resources[] = {
[0] = {
```
.start = IRQ_NR_FOR_YOUR_IRQ,
.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWEDGE,
```c
},
};
dts
static struct platform_device iio_irq_trigger = {
```
.name = "iio_interrupt_trigger",
.num_resources = ARRAY_SIZE(iio_irq_trigger_resources),
.resource = iio_irq_trigger_resources,
```c
};
platform_device_register(&iio_irq_trigger);
```
Declare our IRQ trigger and it will result in the IRQ trigger standalone module being loaded. If its probe function succeeds, there will be a directory corresponding to the trigger. IRQ trigger names have the form irqtrigX, where X corresponds to the virtual
IRQ you just passed, the one you will see in /proc/interrupt:
```bash
$ cd /sys/bus/iio/devices/trigger0/
$ cat name irqtrig85: As we have done with other triggers, you just have to assign that trigger to your device, by writing its name into your device current_trigger file.
# echo "irqtrig85" >
```
/sys/bus/iio/devices/iio:device0/trigger/current_trigger
Now, every time the interrupt is fired, device data will be captured.
The IRQ trigger driver does not support the DT yet, which is the reason why we used our board init file. But it does not matter; since the driver requires a resource, we can use the DT without any code changes.
The following is an example of device tree node declaring the IRQ trigger interface:
```dts
mylabel: my_trigger@0{
compatible = "iio_interrupt_trigger";
c
interrupt-parent = <&gpio4>;
dts
interrupts = <30 0x0>;
c
};
```
The example supposes the IRQ line is the GPIO#30 that belongs to the GPIO controller node gpio4. This consists of using a GPIO as an interrupt source, so that whenever the GPIO
changes to a given state, the interrupt is raised, thus triggering the capture.
## The hrtimer trigger interface
The hrtimer trigger relies on the configfs filesystem (see
Documentation/iio/iio_configfs.txt in kernel sources), which can be enabled through the CONFIG_IIO_CONFIGFS config option and mounted on our system (usually under the /config directory):
```bash
# mkdir /config
# mount -t configfs none /config
```
Now, loading the iio-trig-hrtimer module will create IIO groups accessible under
/config/iio, allowing users to create hrtimer triggers under
/config/iio/triggers/hrtimer, for example:
```bash
# create a hrtimer trigger
$ mkdir /config/iio/triggers/hrtimer/my_trigger_name
# remove the trigger
$ rmdir /config/iio/triggers/hrtimer/my_trigger_name
```
Each hrtimer trigger contains a single sampling_frequency attribute in the trigger directory. A full and working example is provided later in the chapter in the Data capture using hrtimer trigger section.
## IIO buffers
The IIO buffer offers continuous data capture, where more than one data channel can be read at once. The buffer is accessible from the user space through the /dev/iio:device character device node. From within the trigger handler, the function used to fill the buffer is iio_push_to_buffers_with_timestamp. The function responsible for allocating the trigger buffer for your device is iio_triggered_buffer_setup().
## IIO buffer sysfs interface
An IIO buffer has an associated attributes directory under
/sys/bus/iio/iio:deviceX/buffer/*. Here are some of the existing attributes:
length: The total number of data samples (capacity) that can be stored by the buffer. This is the number of scans contained by the buffer.
enable: This activates buffer capture, starting the buffer capture.
watermark: This attribute has been available since kernel version 4.2. It is a positive number that specifies how many scan elements a blocking read should wait for. If using poll for example, it will block until the watermark is reached.
It makes sense only if the watermark is greater than the requested amount of reads. It does not affect non-blocking reads. You can block on poll with a timeout and read the available samples after the timeout expires, and thus have a maximum delay guarantee.
## IIO buffer setup
A channel whose data is to be read and pushed into the buffer is called a scan element.
Their configurations are accessible from the user space through the
/sys/bus/iio/iio:deviceX/scan_elements/* directory, containing the following attributes:
en (actually a suffix for attribute name), is used to enable the channel. If, and only if, its attribute is nonzero, then a triggered capture will contain data samples for this channel, for example, in_voltage0_en, in_voltage1_en, and so on.
type describes the scan element data storage within the buffer, and hence the form in which it is read from the user space, for example, in_voltage0_type.
The format is [be|le]:[s|u]bits/storagebitsXrepeat[>>shift]:
be or le specifies the endianness (big or little).
s or u specifies the sign, either signed (2's complement) or unsigned.
bits is the number of valid data bits.
storagebits is the number of bits this channel occupies in the buffer. A value may be really coded in 12 bits (bits), but occupies
16 bits (storagebits) in the buffer. You must therefore shift the data four times to the right to obtain the actual value. This parameter depends on the device, and you should refer to its data sheet.
shift represents the number of times you should shift the data value prior to masking out unused bits. This parameter is not always needed. If the number of valid bit (bits) is equal to the number of storage bits, the shift will be 0. You can also find this parameter in the device data sheet.
repeat specifies the number of bit/storage bit repetitions. When the repeat element is 0 or 1, then the repeat value is omitted.
The best way to explain this section is by an excerpt from the kernel documentation, which can be found here:
https://www.kernel.org/doc/html/latest/driver-api/iio/buffers.html. For example,
a driver for a three-axis accelerometer, with 12-bit resolution, where data is stored in two 8-
bit registers is as follows:
7 6 5 4 3 2 1 0
+---+---+---+---+---+---+---+---+
|D3 |D2 |D1 |D0 | X | X | X | X | (LOW byte, address 0x06)
+---+---+---+---+---+---+---+---+
7 6 5 4 3 2 1 0
+---+---+---+---+---+---+---+---+
|D11|D10|D9 |D8 |D7 |D6 |D5 |D4 | (HIGH byte, address 0x07)
+---+---+---+---+---+---+---+---+
This will have the following scan element type for each axis:
```bash
$ cat /sys/bus/iio/devices/iio:device0/scan_elements/in_accel_y_type le:s12/16>>4
```
You should interpret this as being little endian-signed data, 16 bits-sized, which needs to be shifted right by 4 bits before masking out the 12 valid bits of data.
The element in struct iio_chan_spec that is responsible for determining how a channel's value should be stored into the buffer is scant_type:
```dts
struct iio_chan_spec {
```
[...]
```dts
struct {
```
char sign; /* Should be 'u' or 's' as explained above */
```c
u8 realbits;
u8 storagebits;
u8 shift;
u8 repeat;
```
enum iio_endian endianness;
```c
} scan_type;
```
[...]
```c
};
```
This structure perfectly matches [be|le]:[s|u]bits/storagebitsXrepeat[>>shift],
which is the pattern described in the previous section. Let's have a look at each part of the structure:
sign represents the sign of the data and matches [s|u] in the pattern realbits corresponds to bits in the pattern storagebits matches the same name in the pattern shift corresponds to the shift in the pattern; the same for repeat iio_indian represents the endianness and matches [be|le] in the pattern
At this point, you are able to write the IIO channel structure that corresponds to the type previously explained:
```dts
struct struct iio_chan_spec accel_channels[] = {
{
```
.type = IIO_ACCEL,
.modified = 1,
.channel2 = IIO_MOD_X,
/* other stuff here */
.scan_index = 0,
```dts
.scan_type = {
```
.sign = 's',
.realbits = 12,
.storagebits = 16,
.shift = 4,
.endianness = IIO_LE,
```c
},
}
```
/* similar for Y (with channel2 = IIO_MOD_Y, scan_index = 1)
* and Z (with channel2 = IIO_MOD_Z, scan_index = 2) axis
*/
```c
}
```
## Putting it all together
Let's have a closer look at the digital triaxial acceleration sensor BMA220 from Bosch. This is an SPI/I2C-compatible device, with 8-bit-sized registers, along with an on-chip motiontriggered interrupt controller, which actually senses tilt, motion, and shock vibration. Its data sheet is available at http://www.mouser.fr/pdfdocs/BSTBMA220DS00308.PDF, and its driver has been introduced since kernel v4.8 (CONFIG_BMA200). Let's walk through it.
Firstly, we declare our IIO channels using struct iio_chan_spec. Once the triggered buffer is used, then we need to fill the .scan_index and .scan_type fields:
```c
#define BMA220_DATA_SHIFT 2
#define BMA220_DEVICE_NAME "bma220"
#define BMA220_SCALE_AVAILABLE "0.623 1.248 2.491 4.983"
#define BMA220_ACCEL_CHANNEL(index, reg, axis) { \
```
.type = IIO_ACCEL, \
.address = reg, \
.modified = 1, \
.channel2 = IIO_MOD_##axis, \
.info_mask_separate = BIT(IIO_CHAN_INFO_RAW), \
.info_mask_shared_by_type = BIT(IIO_CHAN_INFO_SCALE), \
.scan_index = index, \
.scan_type = { \
.sign = 's', \
.realbits = 6, \
.storagebits = 8, \
.shift = BMA220_DATA_SHIFT, \
.endianness = IIO_CPU, \
```c
}, \
}
dts
static const struct iio_chan_spec bma220_channels[] = {
```
BMA220_ACCEL_CHANNEL(0, BMA220_REG_ACCEL_X, X),
BMA220_ACCEL_CHANNEL(1, BMA220_REG_ACCEL_Y, Y),
BMA220_ACCEL_CHANNEL(2, BMA220_REG_ACCEL_Z, Z),
```c
};
```
.info_mask_separate = BIT(IIO_CHAN_INFO_RAW) says there will be a *_raw sysfs entry (attribute) for each channel, and .info_mask_shared_by_type =
```c
BIT(IIO_CHAN_INFO_SCALE) says that there is only a *_scale sysfs entry for all channels of the same type:
```
jma@jma:~$ ls -l /sys/bus/iio/devices/iio:device0/
(...)
```bash
# without modifier, a channel name would have in_accel_raw (bad)
```
-rw-r--r-- 1 root root 4096 jul 20 14:13 in_accel_scale
-rw-r--r-- 1 root root 4096 jul 20 14:13 in_accel_x_raw
-rw-r--r-- 1 root root 4096 jul 20 14:13 in_accel_y_raw
-rw-r--r-- 1 root root 4096 jul 20 14:13 in_accel_z_raw
(...)
Reading in_accel_scale calls the read_raw() hook with the mask set to
IIO_CHAN_INFO_SCALE. Reading in_accel_x_raw calls the read_raw() hook with the mask set to IIO_CHAN_INFO_RAW. The real value is therefore raw_value * scale.
What .scan_type says is that the value returned by each channel is 8-bit-sized (will occupy 8 bits in the buffer), but the useful payload only occupies 6 bits, and the data must be right-shifted twice prior to masking out unused bits. Any scan element type will look like this:
```bash
$ cat /sys/bus/iio/devices/iio:device0/scan_elements/in_accel_x_type le:s6/8>>2
```
The following is our pollfunc (actually it is the bottom half), which reads samples from the device and pushes read values into the buffer
(iio_push_to_buffers_with_timestamp()). Once done, we inform the core
(iio_trigger_notify_done()):
```c
static irqreturn_t bma220_trigger_handler(int irq, void *p)
dts
{
c
int ret;
struct iio_poll_func *pf = p;
struct iio_dev *indio_dev = pf->indio_dev;
struct bma220_data *data = iio_priv(indio_dev);
struct spi_device *spi = data->spi_device;
mutex_lock(&data->lock);
```
data->tx_buf[0] = BMA220_REG_ACCEL_X | BMA220_READ_MASK;
ret = spi_write_then_read(spi, data->tx_buf, 1, data->buffer,
```c
ARRAY_SIZE(bma220_channels) - 1);
if (ret < 0)
```
goto err;
```c
iio_push_to_buffers_with_timestamp(indio_dev, data->buffer,
```
pf->timestamp);
err:
```c
mutex_unlock(&data->lock);
iio_trigger_notify_done(indio_dev->trig);
return IRQ_HANDLED;
}
```
The following is the read function. It is a hook, called every time you read a sysfs entry of the device:
```c
static int bma220_read_raw(struct iio_dev *indio_dev,
struct iio_chan_spec const *chan,
int *val, int *val2, long mask)
dts
{
c
int ret;
u8 range_idx;
struct bma220_data *data = iio_priv(indio_dev);
dts
switch (mask) {
c
case IIO_CHAN_INFO_RAW:
```
/* If buffer mode enabled, do not process single-channel read */
```c
if (iio_buffer_enabled(indio_dev))
return -EBUSY;
```
/* Else we read the channel */
ret = bma220_read_reg(data->spi_device, chan->address);
```c
if (ret < 0)
return -EINVAL;
```
*val = sign_extend32(ret >> BMA220_DATA_SHIFT, 5);
```c
return IIO_VAL_INT;
case IIO_CHAN_INFO_SCALE:
```
ret = bma220_read_reg(data->spi_device, BMA220_REG_RANGE);
```c
if (ret < 0)
return ret;
```
range_idx = ret & BMA220_RANGE_MASK;
*val = bma220_scale_table[range_idx][0];
*val2 = bma220_scale_table[range_idx][1];
```c
return IIO_VAL_INT_PLUS_MICRO;
}
return -EINVAL;
}
```
When you read a *raw sysfs file, the hook is called, and given IIO_CHAN_INFO_RAW in the mask parameter and the corresponding channel in the *chan parameter. *val and val2
are actually output parameters. They must be set with the raw value (read from the device).
Any read performed on the *scale sysfs file will call the hook with
IIO_CHAN_INFO_SCALE in mask parameter, and so on for each attribute mask.
This is also the case with the write function, used to write values to the device. There is an
80% chance your driver does not require a write function. This write hook lets the user change the device's scale:
```c
static int bma220_write_raw(struct iio_dev *indio_dev,
struct iio_chan_spec const *chan,
int val, int val2, long mask)
dts
{
c
int i;
int ret;
int index = -1;
struct bma220_data *data = iio_priv(indio_dev);
dts
switch (mask) {
c
case IIO_CHAN_INFO_SCALE:
for (i = 0; i < ARRAY_SIZE(bma220_scale_table); i++)
if (val == bma220_scale_table[i][0] &&
dts
val2 == bma220_scale_table[i][1]) {
```
index = i;
break;
```c
}
if (index < 0)
return -EINVAL;
mutex_lock(&data->lock);
```
data->tx_buf[0] = BMA220_REG_RANGE;
data->tx_buf[1] = index;
ret = spi_write(data->spi_device, data->tx_buf,
sizeof(data->tx_buf));
```c
if (ret < 0)
dev_err(&data->spi_device->dev,
```
"failed to set measurement range\n");
```c
mutex_unlock(&data->lock);
return 0;
}
return -EINVAL;
}
This function is called whenever you write a value to the device. The most frequently changed parameters are the scale. An example could be echo <desired-scale> >
```
/sys/bus/iio/devices/iio;devices0/in_accel_scale.
Now, it's time to fill a struct iio_info structure to be given to our iio_device:
```dts
static const struct iio_info bma220_info = {
```
.driver_module = THIS_MODULE,
.read_raw = bma220_read_raw,
.write_raw = bma220_write_raw, /* Only if your driver need it */
```c
};
```
In the probe function, we allocate and set up a struct iio_dev IIO device. Memory for private data is reserved too:
/*
* We provide only two mask possibility, allowing to select none or every
* channels.
*/
```dts
static const unsigned long bma220_accel_scan_masks[] = {
c
BIT(AXIS_X) | BIT(AXIS_Y) | BIT(AXIS_Z),
```
0
```c
};
static int bma220_probe(struct spi_device *spi)
dts
{
c
int ret;
struct iio_dev *indio_dev;
struct bma220_data *data;
```
indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*data));
```dts
if (!indio_dev) {
c
dev_err(&spi->dev, "iio allocation failed!\n");
return -ENOMEM;
}
```
data = iio_priv(indio_dev);
data->spi_device = spi;
```c
spi_set_drvdata(spi, indio_dev);
mutex_init(&data->lock);
```
indio_dev->dev.parent = &spi->dev;
indio_dev->info = &bma220_info;
indio_dev->name = BMA220_DEVICE_NAME;
indio_dev->modes = INDIO_DIRECT_MODE;
indio_dev->channels = bma220_channels;
indio_dev->num_channels = ARRAY_SIZE(bma220_channels);
indio_dev->available_scan_masks = bma220_accel_scan_masks;
ret = bma220_init(data->spi_device);
```c
if (ret < 0)
return ret;
```
/* this call will enable trigger buffer support for the device */
ret = iio_triggered_buffer_setup(indio_dev, iio_pollfunc_store_time,
bma220_trigger_handler, NULL);
```dts
if (ret < 0) {
c
dev_err(&spi->dev, "iio triggered buffer setup failed\n");
```
goto err_suspend;
```c
}
```
ret = iio_device_register(indio_dev);
```dts
if (ret < 0) {
c
dev_err(&spi->dev, "iio_device_register failed\n");
iio_triggered_buffer_cleanup(indio_dev);
```
goto err_suspend;
```c
}
return 0;
```
err_suspend:
```c
return bma220_deinit(spi);
}
```
You can enable this driver by means of the CONFIG_BMA220 kernel option. This is available only from v4.8 onwards in kernel. The closest device you can use for this on older kernel versions is the Bosch BMA180, which you can enable using the CONFIG_BMA180 option.
## IIO data access
You may have guessed that there are only two ways to access data with the IIO framework:
one-shot capture through sysfs channels or continuous mode (triggered buffer) through an
IIO character device.
## One-shot capture
One-shot data capture is done through the sysfs interface. By reading the sysfs entry that corresponds to a channel, you'll capture only the data specific to that channel. Given a temperature sensor with two channels, one for the ambient temperature, and the other for the thermocouple temperature:
```bash
# cd /sys/bus/iio/devices/iio:device0
# cat in_voltage3_raw
```
6646
```bash
# cat in_voltage_scale
```
0.305175781
The processed value is obtained by multiplying the scale by the raw value:
Voltage value: 6646 * 0.305175781 = 2028.19824053
The device data sheet says the processed value is given in MV. In our case, it corresponds to
2.02819V.
## Buffer data access
To get a triggered acquisition working, trigger support must have been implemented in your driver. Then, to acquire data from within the user space, you must create a trigger,
assign it, enable the ADC channels, set the dimension of the buffer, and enable it.
## Capturing using the sysfs trigger
Capturing data using the sysfs trigger consists of sending a set of command few sysfs files.
Let's enumerate what we should do to achieve that:
1. Create the trigger: Before the trigger can be assigned to any device, it should be created:
```bash
# echo 0 > /sys/devices/iio_sysfs_trigger/add_trigger
```
Here, 0 corresponds to the index we need to assign to the trigger. After this command, the trigger directory will be available under /sys/bus/iio/devices/, as trigger0.
2. Assign the trigger to the device: A trigger is uniquely identified by its name,
which we can use in order to tie the device to the trigger. Since we used 0 as the index, the trigger will be named sysfstrig0:
```bash
# echo sysfstrig0 >
```
/sys/bus/iio/devices/iio:device0/trigger/current_trigger
We could have used this command too: cat /sys/bus/iio/devices/trigger0/name >
/sys/bus/iio/devices/iio:device0/trigger/current_trigger. That said, if the value we wrote does not correspond to an existing trigger name, nothing will happen. To make sure we really defined a trigger, we can use cat
/sys/bus/iio/devices/iio:device0/trigger/current_trigger.
3. Enable some scan elements: This step consists of choosing which channels should have their data values pushed into the buffer. You should pay attention to available_scan_masks in the driver:
```bash
# echo 1 >
```
/sys/bus/iio/devices/iio:device0/scan_elements/in_voltage4_en
```bash
# echo 1 >
```
/sys/bus/iio/devices/iio:device0/scan_elements/in_voltage5_en
```bash
# echo 1 >
```
/sys/bus/iio/devices/iio:device0/scan_elements/in_voltage6_en
```bash
# echo 1 >
```
/sys/bus/iio/devices/iio:device0/scan_elements/in_voltage7_en
4. Set up the buffer size: Here you should set the number of sample sets that may be held by the buffer:
```bash
# echo 100 > /sys/bus/iio/devices/iio:device0/buffer/length
```
5. Enable the buffer: This step consists of marking the buffer as being ready to receive pushed data:
```bash
# echo 1 > /sys/bus/iio/devices/iio:device0/buffer/enable
```
To stop the capture, we'll have to write 0 in the same file.
6. Fire the trigger: Launch acquisition:
```bash
# echo 1 > /sys/bus/iio/devices/trigger0/trigger_now
```
Now acquisition is done, we can:
7. Disable the buffer:
```bash
# echo 0 > /sys/bus/iio/devices/iio:device0/buffer/enable
```
8. Detach the trigger:
```bash
# echo "" >
```
/sys/bus/iio/devices/iio:device0/trigger/current_trigger
9. Dump the content of our IIO character device:
```bash
# cat /dev/iio\:device0 | xxd -
```
## Capturing using the hrtimer trigger
The following is the set of commands that allow you to capture data using the hrtimer trigger:
```bash
# echo /sys/kernel/config/iio/triggers/hrtimer/trigger0
# echo 50 > /sys/bus/iio/devices/trigger0/sampling_frequency
# echo 1 > /sys/bus/iio/devices/iio:device0/scan_elements/in_voltage4_en
# echo 1 > /sys/bus/iio/devices/iio:device0/scan_elements/in_voltage5_en
# echo 1 > /sys/bus/iio/devices/iio:device0/scan_elements/in_voltage6_en
# echo 1 > /sys/bus/iio/devices/iio:device0/scan_elements/in_voltage7_en
# echo 1 > /sys/bus/iio/devices/iio:device0/buffer/enable
# cat /dev/iio:device0 | xxd -
```
0000000: 0188 1a30 0000 0000 8312 68a8 c24f 5a14 ...0......h..OZ.
0000010: 0188 1a30 0000 0000 192d 98a9 c24f 5a14 ...0.....-...OZ.
[...]
And we look at the type to figure out how to process data:
```bash
$ cat /sys/bus/iio/devices/iio:device0/scan_elements/in_voltage_type be:s14/16>>2
```
Voltage processing is calculated as: 0x188 >> 2 = 98 * 250 = 24500 = 24.5 v
## IIO tools
There are some useful tools you can use in order to simplify and speed up your app development with IIO devices. They are available in tools/iio in the kernel tree:
lsiio.c: Enumerate IIO triggers, devices, and channels iio_event_monitor.c: Monitor an IIO device's ioctl interface for IIO events generic_buffer.c: Retrieve, process, and print data received from an IIO
device's buffer libiio: This is a powerful library developed for analog devices to interface with
IIO devices, and is available at https://github.com/analogdevicesinc/libiio
## Summary
Having reached the end of this chapter, you should now be familiar with the IIO
framework and vocabulary. You should also know what channels, devices, and triggers are.
You can even play with your IIO device from the user space, through sysfs, or a character device. The time to write your own IIO driver has come. There are a lot of existing drivers available that do not support trigger buffers. You can try to add such features to one of them. In the next chapter, we will play with the most useful/widely used resource on a system: the memory. Be strong, the game has just started