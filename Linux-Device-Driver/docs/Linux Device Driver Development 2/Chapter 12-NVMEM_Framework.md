```bash
# Chapter 12 - Leveraging the NVMEM Framework
```
The NVMEM (Non-Volatile MEMory) framework is the kernel layer to handle non-volatile storage, such as EEPROM, eFuse, and so on. The drivers for these devices used to be stored in drivers/misc/, where most of the time each one had to implement its own API to handle identical functionalities, either for kernel users or to expose its content to user space. It turned out that these drivers seriously lacked abstraction code. Moreover, the increasing support for the number of these devices in the kernel led to a lot of code duplication.
The introduction of this framework in the kernel aims at solving these previously mentioned issues. It also introduces DT representation for consumer devices to get the data they require (MAC addresses, SoC/revision ID, part numbers, and so on) from the
NVMEM. We will begin this chapter by introducing NVMEM data structures, which are mandatory to walk through the framework, and then we will look at the NVMEM
provider drivers, where we will learn how to expose the NVMEM memory region to consumers. Finally, we will learn about NVMEM consumer drivers, to leverage the content exposed by the providers.
548 Leveraging the NVMEM Framework
In this chapter, we will cover the following topics:
• Introducing the NVMEM data structures and APIs
• Writing the NVMEM provider driver
• NVMEM consumer driver APIs
## Technical requirements
The following are prerequisites for this chapter:
• C programming skills
• Kernel programming and device driver development skills
• Linux kernel v4.19.X sources, available at https://git.kernel.org/pub/
scm/linux/kernel/git/stable/linux.git/refs/tags
## Introducing NVMEM data structures and APIs
NVMEM is a small framework with a reduced set of APIs and data structures. In this section, we will introduce those APIs and data structures, as well as the concept of a cell,
which is the base of this framework.
NVMEM is based on the producer/consumer pattern, just like the clock framework described in Chapter 4, Storming the Common Clock Framework. There is a single driver for the NVMEM device, exposing the device cells so that they can be accessed and manipulated by consumer drivers. While the NVMEM device driver must include
<linux/nvmem-provider.h>, consumers have to include <linux/nvmemconsumer.h>. This framework has only a few data structures, among which is struct nvmem_device, which looks as follows:
```c
struct nvmem_device {
const char *name;
struct module *owner;
struct device dev;
int stride;
int word_size;
int id;
int users;
```
size_t size;
```c
bool read_only;
```
Introducing NVMEM data structures and APIs 549 int flags;
```c
nvmem_reg_read_t reg_read;
nvmem_reg_write_t reg_write; void *priv;
```
[...]
```c
};
```
This structure actually abstracts the real NVMEM hardware. It is created and populated by the framework upon device registration. That said, its fields are actually set with a complete copy of the fields in struct nvmem_config, which is described as follows:
```c
struct nvmem_config {
struct device *dev;
const char *name;
int id;
struct module *owner;
const struct nvmem_cell_info *cells;
int ncells;
bool read_only;
bool root_only;
nvmem_reg_read_t reg_read;
nvmem_reg_write_t reg_write;
int size;
int word_size;
int stride;
void *priv;
```
[...]
```c
};
```
This structure is the runtime configuration of the NVMEM device, providing either information on it or the helper functions to access its data cells. Upon device registration,
most of its fields are used to populate the newly created nvmem_device structure.
550 Leveraging the NVMEM Framework
The meanings of the fields in the structure are described as follows (knowing these are used to build the underlying struct nvmem_device):
• dev is the parent device.
• name is an optional name for this NVMEM device. It is used with id filled to build the full device name. The final NVMEM device name will be <name><id>.
It is better to append - in the name so that the full name can have this pattern:
<name>-<id>. This is what is used in the PCF85363 driver. If omitted,
nvmem<id> will be used as the default name.
• id is an optional ID for this NVMEM device. It is ignored if name is NULL. If set to -1, the kernel will take care of providing a unique ID to the device.
• owner is the module that owns this NVMEM device.
• cells is an array of predefined NVMEM cells. It is optional.
• ncells is the number of elements in cells.
• read_only marks this device as read-only.
• root_only tells whether this device is accessible only to the root.
• reg_read and reg_write are the underlying callbacks used by the framework to read and write data, respectively. They are defined as follows:
```c
typedef int (*nvmem_reg_read_t)(void *priv,
unsigned int offset,
void *val, size_t bytes);
typedef int (*nvmem_reg_write_t)(void *priv,
unsigned int offset,
void *val,
```
size_t bytes);
• size represents the size of the device.
• word_size is the minimum read/write access granularity for this device. stride is the minimum read/write access stride. Its principle has already been explained in previous chapters.
• priv is context data passed to read/write callbacks. It could, for example, be a bigger structure wrapping this NVMEM device.
Introducing NVMEM data structures and APIs 551
Previously, we used the term data cell. A data cell represents a memory region
(or data region) in the NVMEM device. This may also be the whole memory of the device.
Actually, data cells are to be assigned to consumer drivers. These memory regions are maintained by the framework using two different data structures, depending on whether we are on the consumer side or on the provider side. These are the struct nvmem_
cell_info structure for the provider, and struct nvmem_cell for the consumer.
From within the NVMEM core code, the kernel uses nvmem_cell_info_to_nvmem_
cell() to switch from the former structure to the second one.
These structures are introduced as follows:
```c
struct nvmem_cell {
const char *name;
int offset;
int bytes;
int bit_offset;
int nbits;
struct nvmem_device *nvmem;
struct list_head node;
};
```
The other data structure, that is, struct nvmem_cell, looks like the following:
```c
struct nvmem_cell_info {
const char *name;
unsigned int offset;
unsigned int bytes;
unsigned int bit_offset;
unsigned int nbits;
};
```
As you can see, the preceding two data structures share almost the same properties.
Let’s look at their meanings, as follows:
• name is the name of the cell.
• offset is the offset (where it starts) of the cell from within the whole hardware data registers.
• bytes is the size (in bytes) of the data cells, starting from offset.
552 Leveraging the NVMEM Framework
• A cell may have bit-level granularity. For these cells, bit_offset should be set in order to specify the bit offset from within the cell, and nbits should be defined according to the size (in bits) of the region of interest.
• nvmem is the NVMEM device to which this cell belongs.
• node is used to track the cell system-wide. This field ends up in the nvmem_cells list, which holds all the cells available on the system, regardless of the NVMEM
device they belong to. This global list is actually protected by a mutex, nvmem_
cells_mutex, both statically defined in drivers/nvmem/core.c.
To clarify the preceding explanation, let’s take as an example a cell with the following config:
```c
static struct nvmem_cellinfo mycell = {
```
.offset = 0xc,
.bytes = 0x1,
[...],
```c
}
```
In the preceding example, if we consider .nbits and .bit_offset as both equal to 0,
it means we are interested in the whole data region of the cell, which is 1 byte-sized in our case. But what if we are interested only in bits 2 to 4 (3 bits, actually)? The structure would be as follows:
```c
staic struct nvmem_cellinfo mycell = {
```
.offset = 0xc,
.bytes = 0x1,
.bit_offset = 2,
```dts
.nbits = 2 [...]
c
}
```
Important note
The preceding examples are only for pedagogical purposes. Even though you can have predefined cells in the driver code, it is recommended that you rely on the device tree to declare the cells, as we will see later in the chapter, in the
Device tree bindings for NVMEM providers section, to be precise.
Neither the consumer nor the provider driver should create instances of struct nvmem_cell. The NVMEM core internally handles this, either when the producer provides an array of cell info, or when the consumer requests a cell.
Writing the NVMEM provider driver 553
So far, we have gone through the data structures and APIs provided by this framework.
However, NVMEM devices can be accessed either from the kernel or user space.
Moreover, in the kernel, there must be a driver exposing the device storage in order to have other drivers accessing it. This is the producer/consumer design, where the provider driver is the producer, and the other driver is the consumer. Right now, let’s start with the provider (aka the producer) part of this framework.
## Writing the NVMEM provider driver
```c
The provider is the one exposing the device memory so that other drivers (the consumers)
```
can access it. The main tasks of these drivers are as follows:
• Providing suitable NVMEM configuration with respect to the device’s datasheet,
along with the routines allowing you to access the memory
• Registering the device with the system
• Providing device tree binding documentation
That is all the provider has to do. Most (the rest) of the mechanism/logic is handled by the
NVMEM framework’s code.
## NVMEM device (un)registration
Registering/unregistering the NVMEM device is actually part of the provider-side driver,
which can use the nvmem_register()/nvmem_unregister() functions, or their managed versions, devm_nvmem_register()/devm_nvmem_unregister():
```c
struct nvmem_device *nvmem_register(const struct nvmem_config *config)
struct nvmem_device *devm_nvmem_register(struct device *dev,
const struct nvmem_config *config)
int nvmem_unregister(struct nvmem_device *nvmem)
int devm_nvmem_unregister(struct device *dev,
struct nvmem_device *nvmem)
```
554 Leveraging the NVMEM Framework
Upon registration, the /sys/bus/nvmem/devices/dev-name/nvmem binary entry will be created. In these interfaces, the *config parameter is the NVMEM config describing the NVMEM device that has to be created. The *dev parameter is only for the managed version and represents the device using the NVMEM device. On the success path, these functions return a pointer to nvmem_device, or return ERR_PTR() on error otherwise.
On the other hand, unregistration functions accept the pointer to the NVMEM device created on the success path of the registration function. They return 0 upon successful unregistration and a negative error otherwise.
## NVMEM storage in RTC devices
There are many Real-Time Clock (RTC) devices that embed non-volatile storage. This embedded storage can be either EEPROM or battery-backed RAM. Looking at the
RTC device data structure in include/linux/rtc.h, you will notice that there are
NVMEM-related fields, as follows:
```c
struct rtc_device {
```
[...]
```c
struct nvmem_device *nvmem;
```
/* Old ABI support */
```c
bool nvram_old_abi;
struct bin_attribute *nvram;
```
[...]
```c
}
```
Note the following in the preceding structure excerpt:
• nvmem abstracts the underlying hardware memory.
• nvram_old_abi is a Boolean that tells whether the NVMEM of this RTC is to be registered using the old (and now deprecated) NVRAM ABI, which uses /sys/
class/rtc/rtcx/device/nvram to expose the memory. This field should be set to true only if you have existing applications (that you do not want to break)
using this old ABI interface. New drivers should not set this.
• nvram is actually the binary attribute for the underlying memory, used by the RTC
framework only for old ABI support; that is, if nvram_old_abi is true.
Writing the NVMEM provider driver 555
The RTC-related NVMEM framework API can be enabled through the RTC_NVMEM
kernel config option. This API is defined in drivers/rtc/nvmem.c, and exposes both rtc_nvmem_register() and rtc_nvmem_unregister(), respectively, for
RTC-NVMEM registration and unregistration. These are described as follows:
```c
int rtc_nvmem_register(struct rtc_device *rtc,
struct nvmem_config *nvmem_config)
void rtc_nvmem_unregister(struct rtc_device *rtc)
rtc_nvmem_register() returns 0 on success. It accepts a valid RTC device as its first parameter. This has an impact on the code. It means the RTC’s NVMEM should be registered only after the actual RTC device has been successfully registered. In other words, rtc_nvmem_register() is to be called only after rtc_register_
```
device() has succeeded. The second argument should be a pointer to a valid nvmem_
config object. Moreover, as we have already seen, this config can be declared in the stack since all its fields are entirely copied for building the nvmem_device structure. The opposite is rtc_nvmem_unregister(), which unregisters the NVMEM.
Let’s summarize this with an excerpt of the probe function of the DS1307 RTC driver,
drivers/rtc/rtc-ds1307.c:
```c
static int ds1307_probe(struct i2c_client *client,
const struct i2c_device_id *id)
{
struct ds1307 *ds1307;
int err = -ENODEV;
int tmp;
const struct chip_desc *chip;
```
[...]
```c
ds1307->rtc->ops = chip->rtc_ops ?: &ds13xx_rtc_ops;
err = rtc_register_device(ds1307->rtc);
if (err)
return err;
if (chip->nvram_size) {
struct nvmem_config nvmem_cfg = {
```
.name = "ds1307_nvram",
.word_size = 1,
.stride = 1,
556 Leveraging the NVMEM Framework
```c
.size = chip->nvram_size,
```
.reg_read = ds1307_nvram_read,
.reg_write = ds1307_nvram_write,
.priv = ds1307,
```c
};
ds1307->rtc->nvram_old_abi = true;
rtc_nvmem_register(ds1307->rtc, &nvmem_cfg);
}
```
[...]
```c
}
```
The preceding code first registers the RTC with the kernel prior to registering the
NVMEM device, giving an NVMEM config that corresponds to the RTC’s storage space.
The preceding is RTC-related and not generic. Other NVMEM devices must have their driver expose callbacks to which the NVMEM framework will forward any read/write requests, either from user space or internally from within the kernel itself. The next section explains how this is done.
## Implementing NVMEM read/write callbacks
In order for the kernel and other frameworks to be able to read/write data from/to the
NVMEM device and its cells, each NVMEM provider must expose a couple of callbacks allowing those read/write operations. This mechanism allows hardware-independent consumer code, so any reading/writing request from the consumer side is redirected to the underlying provider’s read/write callback. The following are the read/write prototypes that every provider must conform to:
```c
typedef int (*nvmem_reg_read_t)(void *priv,
unsigned int offset,
void *val, size_t bytes);
typedef int (*nvmem_reg_write_t)(void *priv,
unsigned int offset,
void *val, size_t bytes);
```
These are independent of the underlying bus that the NVMEM device is behind.
```c
nvmem_reg_read_t is for reading data from the NVMEM device. priv is the user context provided in the NVMEM config, offset is where reading should start, val is an output buffer where the read data has to be stored, and bytes is the size of the data to be read (the number of bytes, actually). This function should return the number of successful bytes read on success, and a negative error code on error.
```
Writing the NVMEM provider driver 557
On the other hand, nvmem_reg_write_t is for writing purposes. priv has the same meaning as for reading, offset is where writing should start at, val is a buffer containing the data to be written, and bytes is the number of bytes in data in val, which should be written. bytes is not necessarily the size of val. This function should return the number of bytes written successfully on success, and a negative error code on error.
Now that we have seen how to implement provider read/write callbacks, let’s see how we can extend the provider capabilities with the device tree.
## Device tree bindings for NVMEM providers
The NVMEM data provider does not have any bindings particularly. It should be described with respect to its parent bus DT binding. This means, for example, that if it is an I2C device, it should be described (in respect to the I2C binding) as a child of the node that represents the I2C bus that it sits behind. However, there is an optional read-only property that makes the device read-only. Moreover, each child node will be considered as a data cell (a memory region in the NVMEM device).
Let’s consider the following MMIO NVMEM device along with its child nodes for explanation:
```c
ocotp: ocotp@21bc000 {
dts
#address-cells = <1>;
#size-cells = <1>;
compatible = "fsl,imx6sx-ocotp", "syscon";
reg = <0x021bc000 0x4000>;
```
[...]
```c
tempmon_calib: calib@38 {
dts
reg = <0x38 4>;
c
};
tempmon_temp_grade: temp-grade@20 {
dts
reg = <0x20 4>;
c
};
foo: foo@6 {
dts
reg = <0x6 0x2> bits = <7 2>
c
};
```
[...]
```c
};
```
558 Leveraging the NVMEM Framework
According to the properties defined in the child nodes, the NVMEM framework builds the appropriate nvmem_cell structures and inserts them into the system-wide nvmem_cells list. The following are the possible properties for data cell bindings:
• reg: This property is mandatory. It is a two-cell property, describing the offset in bytes (the first cell in the property) and the size in bytes (the second cell of the property) of the data region within the NVMEM device.
• bits: This is an optional two-cell property that specifies the offset (possible values from 0-7) in bits and the number of bits within the address range specified by the reg property.
```dts
Having defined the data cells from within the provider node, these can be assigned to consumers using the nvmem-cells property, which is a list of phandles to NVMEM
providers. Moreover, there should be an nvmem-cell-names property too, whose main purpose is to name each data cell. This assigned name can therefore be used to look for the appropriate data cell using the consumer APIs. The following is an example assignment:
c
tempmon: tempmon {
dts
compatible = "fsl,imx6sx-tempmon", "fsl,imx6q-tempmon";
```
interrupt-parent = <&gpc>;
```dts
interrupts = <GIC_SPI 49 IRQ_TYPE_LEVEL_HIGH>;
```
fsl,tempmon = <&anatop>;
```dts
clocks = <&clks IMX6SX_CLK_PLL3_USB_OTG>;
nvmem-cells = <&tempmon_calib>, <&tempmon_temp_grade>;
nvmem-cell-names = "calib", "temp_grade";
c
};
```
The full NVMEM device tree binding is available in Documentation/devicetree/
bindings/nvmem/nvmem.txt.
We just came across the implementation of drivers (the so-called producers) that expose the storage of the NVMEM device. Though it is not always the case, there may be other drivers in the kernel that would need access to the storage exposed by the producer
(aka the provider). The next section will describe these drivers in detail.
NVMEM consumer driver APIs 559
## NVMEM consumer driver APIs
NVMEM consumers are drivers who access the storage exposed by the producer. These drivers can pull the NVMEM consumer API by including <linux/nvmem-consumer.
h>, which will bring the following cell-based APIs in:
```c
struct nvmem_cell *nvmem_cell_get(struct device *dev,
const char *name);
struct nvmem_cell *devm_nvmem_cell_get(struct device *dev,
const char *name);
void nvmem_cell_put(struct nvmem_cell *cell);
void devm_nvmem_cell_put(struct device *dev,
struct nvmem_cell *cell);
void *nvmem_cell_read(struct nvmem_cell *cell, size_t *len);
int nvmem_cell_write(struct nvmem_cell *cell,
void *buf, size_t len);
int nvmem_cell_read_u32(struct device *dev,
const char *cell_id,
```
u32 *val);
The devm_-prefixed APIs are resource-managed versions, which are to be used whenever possible.
That being said, the consumer interface entirely depends on the ability of the producer to expose (part of) its cells so that they can be accessed by others. As discussed previously,
this capability of providing/exposing cells should be done via the device tree. devm_
```dts
nvmem_cell_get() serves to grab a given cell with respect to the name assigned through the nvmem-cell-names property. The nvmem_cell_read API always reads the whole cell size (that is, nvmem_cell->bytes) if possible. Its third parameter, len,
```
is an output parameter holding the actual number of nvmem_config.word_size
(actually, it holds 1 most of the time, which means a single byte) being read.
```c
On successful read, the content pointed to by len will be equal to the number of bytes in the cell: *len = nvmem_cell->bytes. nvmem_cell_read_u32(), on the other side, reads a cell value as u32.
```
The following is the code that grabs the cells allocated to the tempmon node described in the previous section, and reads their content as well:
```c
static int imx_init_from_nvmem_cells(struct platform_device *pdev)
{
```
560 Leveraging the NVMEM Framework int ret; u32 val;
```c
ret = nvmem_cell_read_u32(&pdev->dev, "calib", &val);
if (ret)
return ret;
```
ret = imx_init_calib(pdev, val);
```c
if (ret)
return ret;
ret = nvmem_cell_read_u32(&pdev->dev, "temp_grade", &val);
if (ret)
return ret;
imx_init_temp_grade(pdev, val);
return 0;
}
```
Here, we have gone through both the consumer and producer aspects of this framework.
Often, drivers need to expose their services to user space. The NVMEM framework (just like other Linux kernel frameworks) can transparently handle exposing NVMEM services to user space. The next section explains this in detail.
## NVMEM in user space
The NVMEM user space interface relies on sysfs, as most of the kernel frameworks do. Each NVMEM device registered with the system has a directory entry created in /
sys/bus/nvmem/devices, along with an nvmem binary file (on which you can use hexdump or even echo) created in that directory, which represents the device’s memory.
The full path has the following pattern: /sys/bus/nvmem/devices/<dev-name>X/
nvmem. In this path pattern, <dev-name> is the nvmem_config.name name provided by the producer driver. The following code excerpt shows how the NVMEM core constructs the <dev-name>X pattern:
```c
int rval;
```
rval = ida_simple_get(&nvmem_ida, 0, 0, GFP_KERNEL);
```c
nvmem->id = rval;
if (config->id == -1 && config->name) {
dev_set_name(&nvmem->dev, "%s", config->name);
} else {
dev_set_name(&nvmem->dev, "%s%d", config->name ? : "nvmem",
Summary 561 config->name ? config->id : nvmem->id);
}
The preceding code says if nvmem_config->id == -1, then X in the pattern is omitted and only nvmem_config->name is used to name the sysfs directory entry.
If nvmem_config->id != -1 and nvmem_config->name is set, it will be used along with the nvmem_config->id field set by the driver (which is X in the pattern).
However, if nvmem_config->name is not set by the driver, the core will use the nvmem string along with an ID that has been generated (which is X in the pattern).
```
Important note
Whatever cells are defined, the NVMEM framework exposes the full register space via the NVMEM binary, not the cells. Accessing the cells from user space requires knowing their offsets and size in advance.
NVMEM content can then be read in user space, thanks to the sysfs interface, using either hexdump or the simple cat command. For example, assuming we have an
I2C EEPROM sitting on I2C number 2 at address 0x55 registered on the system as an
NVMEM device, its sysfs path would be /sys/bus/nvmem/devices/2-00550/
nvmem. The following is how you can write/read some content:
```bash
cat /sys/bus/nvmem/devices/2-00550/nvmem echo "foo" > /sys/bus/nvmem/devices/2-00550/nvmem cat /sys/bus/nvmem/devices/2-00550/nvmem
```
Now we have seen how the NVMEM registers are exposed to user space. Though this section is short, we have covered enough to leverage this framework from user space.
## Summary
In this chapter, we went through the NVMEM framework implementation in the Linux kernel. We introduced its APIs from the producer side as well as from the consumer side,
and also discussed how to use it from user space. I have no doubt that these devices have their place in the embedded world.
In the next chapter, we will address the issue of reliability by means of watchdog devices,
discussing how to set up these devices and writing their Linux kernel drivers