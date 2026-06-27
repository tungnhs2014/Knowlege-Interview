```bash
# Chapter 3 - Delving into the MFD Subsystem and Syscon API
```
The increasingly dense integration of devices has led to a kind of device that is made up of several other devices or IPs that can achieve a dedicated function. With the advent of this device, a new subsystem appeared in the Linux kernel. These are MFDs, which stands for multi-function devices. These devices are physically seen as standalone devices, but from a software point of view, these are represented in a parent-child relationship, where the children are subdevices.
118 Delving into the MFD Subsystem and Syscon API
While some I2C- and SPI-based devices/subdevices might need either some hacks or configurations prior to being added to the system, there are also MMIO-based devices/
subdevices where zero conf/hacks are required as they just need to share the main device's register region between subdevices. The simple-mfd helper has then been introduced to handle zero conf/hacks subdevice registering, and syscon has been introduced for sharing a device's memory region with other devices. Since regmap was handling MMIO registers and managed locking (aka synchronization) accesses to memory, it has been a natural choice to build syscon on top of regmap. To get familiar with the MFD subsystem, in this chapter, we will begin with an introduction to MFD, where you will learn about its data structures and APIs, and then we will look at device tree binding in order to describe these devices to the kernel. Finally, we will talk about syscon and introduce the simple-mfd driver for a zero conf/hacks subdevice.
This chapter will cover the following topics:
• Introducing the MFD and syscon APIs and data structures
• Device tree binding for MFD devices
• Understanding syscon and simple-mfd
Technical requirements
In order to leverage this chapter, you will need the following:
• C programming skills
• Good knowledge of Linux device driver models
• Linux kernel v4.19.X sources, available at https://git.kernel.org/pub/
scm/linux/kernel/git/stable/linux.git/refs/tags
Introducing the MFD subsystem and Syscon APIs 119
Introducing the MFD subsystem and Syscon
APIs
Prior to delving into the syscon framework and its APIs, we will cover MFDs. There are peripherals or hardware blocks exposing more than a single functionality by means of subdevices they embed into them and that are handled by separate subsystems in the kernel. That being said, a subdevice is a dedicated entity in a so-called multifunction device, responsible for a specific task, and managed through a reduced set of registers, in the chip's register map. ADP5520 is a typical example of an MFD device, as it contains a backlight, a keypad, LEDs, and GPIO controllers. Each of these is then considered as a subdevice, and as you can see, each of these falls into a different subsystem. The MFD
subsystem, defined in include/linux/mfd/core.h and implemented in drivers/
mfd/mfd-core.c, has been created to deal with these devices, allowing the following features:
• Registering the same device with multiple subsystems
• Multiplexing bus and register access, as there may be some registers shared between subdevices
• Handling IRQs and clocks
Throughout this section, we will study the driver of the da9055 device from the dialog-semiconductor and located in drivers/mfd/da9055-core.c in the kernel source tree. The datasheet for this device can be found at https://www.
dialog-semiconductor.com/sites/default/files/da9055-00-ids3a_
20120710.pdf.
120 Delving into the MFD Subsystem and Syscon API
In most cases, MFD device drivers consist of two parts:
• A core driver that should be hosted in drivers/mfd, responsible for the main initialization and registering each subdevice as a platform device (along with its platform data) on the system. This driver should provide common services for the subdevice drivers. These services include register access, control, and shared interrupt management. When a platform driver for one of the subsystems is instantiated, the core initializes the chip (which may be specified by the platform data). There can be support for multiple block devices of the same type built into a single kernel image. This is possible thanks to the mechanism of platform data.
A platform-specific data abstraction mechanism in the kernel is used to pass configurations to the core, and subsidiary drivers make it possible to support multiple block devices of the same type.
• The subdevice driver, which is responsible for handling a specific subdevice registered earlier by the core driver. These drivers are located in their respective subsystem directories. Each peripheral (subsystem device) has a limited view of the device, which is implicitly reduced to the specific set of resources that the peripheral requires in order to function correctly.
Important note
The concept of subdevices in this chapter should not be confused with the concept of the same name in Chapter 7, Demystifying V4L2 and Video Capture
Device Drivers, which is slightly different, where a subdevice also represents an entity in the video pipeline.
A subdevice is represented in the MFD subsystem by an instance of the struct mfd_
cell structure, which you can call a cell. A cell is meant to describe a subdevice. The core driver must provide an array of as many cells as there are subdevices in the given peripheral. The MFD subsystem will use the information registered in each structure in the array to create a platform device for each subdevice, along with the platform data associated with each subdevice. In a struct mfd_cell structure, you can specify more advanced things, such as the resources used by the subdevice and suspend-resume operations (to be called from the driver for the subdevice). This structure is presented as follows, with some fields removed for simplicity reasons:
/*
* This struct describes the MFD part ("cell").
* After registration the copy of this structure will
* become the platform data of the resulting platform_device
*/
Introducing the MFD subsystem and Syscon APIs 121
```c
struct mfd_cell {
const char *name;
int id;
```
[...]
```c
int (*suspend)(struct platform_device *dev);
int (*resume)(struct platform_device *dev);
```
/* platform data passed to the sub devices drivers */
```c
void *platform_data;
```
size_t pdata_size;
/* Device Tree compatible string */
```c
const char *of_compatible;
```
/* Matches ACPI */
```c
const struct mfd_cell_acpi_match *acpi_match;
```
/*
* These resources can be specified relative to the
* parent device. For accessing hardware, you should
* use resources from the platform dev
*/
```c
int num_resources;
const struct resource *resources;
```
[...]
```c
};
```
Important note
The new platform devices that are created will have the cell structure as their platform data. The real platform data can then be accessed through pdev-
```c
>mfd_cell->platform_data. A driver can also use mfd_get_
```
cell() in order to retrieve the MFD cell corresponding to a platform device:
```c
const struct mfd_cell *cell = mfd_get_cell(pdev);.
```
The functionality of each member of this structure is self-explanatory. However, the following gives you more details.
122 Delving into the MFD Subsystem and Syscon API
The .resources element is an array that represents the resources specific to the subdevice (which is also a platform device), and .num_resources in the number of entries in the array. These are defined as it was done using platform_data, and you probably want to name them for easy retrieval. The following is an example from an MFD
driver whose original core source file is drivers/mfd/da9055-core.c:
```c
static struct resource da9055_rtc_resource[] = {
{
```
.name = „ALM",
.start = DA9055_IRQ_ALARM,
.end = DA9055_IRQ_ALARM,
.flags = IORESOURCE_IRQ,
```c
},
{
```
.name = "TICK",
.start = DA9055_IRQ_TICK,
.end = DA9055_IRQ_TICK,
.flags = IORESOURCE_IRQ,
```c
},
};
static const struct mfd_cell da9055_devs[] = {
```
...
```c
{
dts
.of_compatible = "dlg,da9055-rtc",
```
.name = "da9055-rtc",
.resources = da9055_rtc_resource,
.num_resources = ARRAY_SIZE(da9055_rtc_resource),
```c
},
```
...
```c
};
```
The following example shows how to retrieve the resource from the subdevice driver, in this case, which is implemented in drivers/rtc/rtc-da9055.c:
```c
static int da9055_rtc_probe(struct platform_device *pdev)
{
```
[...]
Introducing the MFD subsystem and Syscon APIs 123
alm_irq = platform_get_irq_byname(pdev, "ALM");
```c
if (alm_irq < 0)
return alm_irq;
ret = devm_request_threaded_irq(&pdev->dev, alm_irq, NULL,
```
da9055_rtc_alm_irq,
IRQF_TRIGGER_HIGH |
IRQF_ONESHOT,
"ALM", rtc);
```c
if (ret != 0)
dev_err(rtc->da9055->dev,
```
"irq registration failed: %d\n", ret);
[...]
```c
}
```
Actually, you should use platform_get_resource(), platform_get_
resource_byname(), platform_get_irq(), and platform_get_irq_
byname() to retrieve the resources.
When using .of_compatible, the function has to be a child of the MFD (see the
Device tree binding for MFD devices section). You should statically fill an array of this structure, containing as many entries as there are subdevices on your device:
```c
static struct resource da9055_rtc_resource[] = {
{
```
.name = „ALM",
.start = DA9055_IRQ_ALARM,
.end = DA9055_IRQ_ALARM,
.flags = IORESOURCE_IRQ,
```c
},
```
[...]
```c
};
```
[...]
```c
static const struct mfd_cell da9055_devs[] = {
{
dts
.of_compatible = "dlg,da9055-gpio",
```
124 Delving into the MFD Subsystem and Syscon API
.name = "da9055-gpio",
```c
},
{
dts
.of_compatible = "dlg,da9055-regulator",
```
.name = "da9055-regulator",
.id = 1,
```c
},
```
[...]
```c
{
dts
.of_compatible = "dlg,da9055-rtc",
```
.name = "da9055-rtc",
.resources = da9055_rtc_resource,
.num_resources = ARRAY_SIZE(da9055_rtc_resource),
```c
},
{
dts
.of_compatible = "dlg,da9055-watchdog",
```
.name = "da9055-watchdog",
```c
},
};
```
After the array of struct mfd_cell is filled, it has to be passed to the devm_mfd_
add_devices() function, as follows:
```c
int devm_mfd_add_devices(
struct device *dev,
int id,
const struct mfd_cell *cells,
int n_devs,
struct resource *mem_base,
int irq_base,
struct irq_domain *domain)
```
Introducing the MFD subsystem and Syscon APIs 125
This method's arguments are explained as follows:
• dev is the generic struct device structure of the MFD chip. It will be used to set the subdevice's parent.
• id: Since subdevices are created as platform devices, they should be given an ID.
This field should be set with PLATFORM_DEVID_AUTO for automatic ID allocation,
in which case mfd_cell.id of the corresponding cell is ignored. Otherwise, you should use PLATFORM_DEVID_NONE.
• cells is a pointer to a list (an array actually) of struct mfd_cell structures that describe subdevices.
• n_dev is the number of struct mfd_cell entries to use in the array to create platform devices. To create as many platform devices as there are cells in the array,
you should use the ARRAY_SIZE() macro.
• mem_base: If not NULL, its .start field will be used as the base of each resource of type IORESOURCE_MEM of each MFD cell in the previously mentioned array.
The following is an excerpt of the mfd_add_device() function showing this:
```c
for (r = 0; r < cell->num_resources; r++) {
res[r].name = cell->resources[r].name;
res[r].flags = cell->resources[r].flags;
```
/* Find out base to use */
```c
if ((cell->resources[r].flags & IORESOURCE_MEM) &&
mem_base) {
```
res[r].parent = mem_base;
res[r].start =
```c
mem_base->start + cell->resources[r].start;
```
res[r].end =
```c
mem_base->start + cell->resources[r].end;
} else if (cell->resources[r].flags & IORESOURCE_IRQ)
{
```
[...]
126 Delving into the MFD Subsystem and Syscon API
• irq_base: This parameter is ignored if the domain is set. Otherwise, it behaves like mem_base but for each resource of type IORESOURCE_IRQ. The following is an excerpt of the mfd_add_device() function showing this:
```c
} else if (cell->resources[r].flags & IORESOURCE_IRQ)
{
if (domain) {
```
/* Unable to create mappings for IRQ ranges. */
```c
WARN_ON(cell->resources[r].start !=
cell->resources[r].end);
```
res[r].start = res[r].end =
```c
irq_create_mapping(
domain,cell->resources[r].start);
} else {
```
res[r].start =
```c
irq_base + cell->resources[r].start;
```
res[r].end =
```c
irq_base + cell->resources[r].end;
}
} else {
```
[...]
• domain: For MFD chips that also play the role of IRQ controller for their subdevices, this parameter will be used as the IRQ domain to create IRQ mappings for these subdevices. It works this way: for each resource r of type IORESOURCE_
IRQ in each cell, the MFD core will create a new resource, res, of the same type
(actually, an IRQ resource, whose res.start and res.end fields are set with the IRQ mapping in this domain that corresponds to the initial resource's .start field: res[r].start = res[r].end = irq_create_mapping(domain,
```c
cell->resources[r].start);). New IRQ resources are then assigned to the platform device of the current cell and correspond to its virqs. Please have a look at the preceding excerpt, in the previous parameter description. Note that this parameter can be NULL.
```
Let's now see how to put this all together with an excerpt of the da9055 MFD driver:
```c
#define DA9055_IRQ_NONKEY_MASK 0x01
#define DA9055_IRQ_ALM_MASK 0x02
#define DA9055_IRQ_TICK_MASK 0x04
```
Introducing the MFD subsystem and Syscon APIs 127
```c
#define DA9055_IRQ_ADC_MASK 0x08
#define DA9055_IRQ_BUCK_ILIM_MASK 0x08
```
/*
* PMIC IRQ
*/
```c
#define DA9055_IRQ_ALARM 0x01
#define DA9055_IRQ_TICK 0x02
#define DA9055_IRQ_NONKEY 0x00
#define DA9055_IRQ_REGULATOR 0x0B
#define DA9055_IRQ_HWMON 0x03
struct da9055 {
struct regmap *regmap;
struct regmap_irq_chip_data *irq_data;
struct device *dev;
struct i2c_client *i2c_client;
int irq_base;
int chip_irq;
};
```
In the preceding excerpt, the driver defined some constants, along with a private data structure, whose meaning will be clear as and when you read the code. After, the IRQs are defined for the register map core, as follows:
```c
static const struct regmap_irq da9055_irqs[] = {
[DA9055_IRQ_NONKEY] = {
```
.reg_offset = 0,
.mask = DA9055_IRQ_NONKEY_MASK,
```c
},
[DA9055_IRQ_ALARM] = {
```
.reg_offset = 0,
.mask = DA9055_IRQ_ALM_MASK,
```c
},
[DA9055_IRQ_TICK] = {
```
.reg_offset = 0,
128 Delving into the MFD Subsystem and Syscon API
.mask = DA9055_IRQ_TICK_MASK,
```c
},
[DA9055_IRQ_HWMON] = {
```
.reg_offset = 0,
.mask = DA9055_IRQ_ADC_MASK,
```c
},
[DA9055_IRQ_REGULATOR] = {
```
.reg_offset = 1,
.mask = DA9055_IRQ_BUCK_ILIM_MASK,
```c
},
};
static const struct regmap_irq_chip da9055_regmap_irq_chip = {
```
.name = "da9055_irq",
.status_base = DA9055_REG_EVENT_A,
.mask_base = DA9055_REG_IRQ_MASK_A,
.ack_base = DA9055_REG_EVENT_A,
.num_regs = 3,
.irqs = da9055_irqs,
.num_irqs = ARRAY_SIZE(da9055_irqs),
```c
};
```
In the preceding excerpt, da9055_irqs is an array of elements of type regmap_irq,
which describes a generic regmap IRQ. It is assigned to da9055_regmap_irq_chip,
which is of type regmap_irq_chip and represents the regmap IRQ chip. Both are part of the regmap IRQ data structures set. Finally, the probe method is implemented,
as follows:
```c
static int da9055_i2c_probe(struct i2c_client *client,
const struct i2c_device_id *id)
{
int ret;
struct da9055_pdata *pdata = dev_get_platdata(da9055->dev);
uint8_t clear_events[3] = {0xFF, 0xFF, 0xFF};
```
[...]
ret =
Introducing the MFD subsystem and Syscon APIs 129
```c
devm_regmap_add_irq_chip(
&client->dev, da9055->regmap,
da9055->chip_irq, IRQF_TRIGGER_LOW | IRQF_ONESHOT,
da9055->irq_base, &da9055_regmap_irq_chip,
&da9055->irq_data);
if (ret < 0)
return ret;
da9055->irq_base = regmap_irq_chip_get_base(
da9055->irq_data);
```
ret = devm_mfd_add_devices(
```c
da9055->dev, -1,
```
da9055_devs, ARRAY_SIZE(da9055_devs),
```c
NULL, da9055->irq_base,
regmap_irq_get_domain(da9055->irq_data));
if (ret)
```
goto err;
[...]
```c
}
```
In the preceding probe method, da9055_regmap_irq_chip (defined earlier) is given as a parameter to regmap_add_irq_chip() in order to add a valid regmap IRQ
controller to the IRQ core. This function returns O on success. Moreover, it also returns a fully configured regmap_irq_chip_data structure through its last parameter, which can be used later as the runtime data structure for the controller. This regmap_irq_
chip_data structure will contain the IRQ domain associated with the previously added
IRQ controller. This IRQ domain is finally given as a parameter to devm_mfd_add_
devices(), along with the array of MFD cells and its size in terms of the number of cells.
130 Delving into the MFD Subsystem and Syscon API
Important note
Do note that devm_mfd_add_devices() is actually the resourcemanaged version of mfd_add_devices(), which has the following function call sequence:
```c
mfd_add_devices()-> mfd_add_device()-> platform_device_alloc()
-> platform_device_add_data()
-> platform_device_add_resources()
-> platform_device_add()
```
There are I2C chips where both the chip itself and internal subdevices have different I2C
addresses. Such I2C subdevices can't be probed as I2C clients because the MFD core only instantiates a platform device given an MFD cell. This issue is addressed by the following:
• Creating a dummy I2C client given the subdevice's I2C address and the MFD chip's adapter. This actually corresponds to the adapter (bus) managing the MFD device.
This can be achieved using i2c_new_dummy(). The returned I2C client should be saved for later use – for example, with i2c_unregister_device(), which should be called when the module is being unloaded.
• If a subdevice needs its own regmap, then this regmap has to be built on top of its dummy I2C client.
• Storing either the I2C client only (for later removal) or with the regmap in a private data structure that can be assigned to the underlying platform device.
To summarize the preceding steps, let's walk through the driver of a real MFD device, the max8925 (which is mainly a power management IC, but is also made up of a large group of subdevices). Our code is a summary (dealing with two subdevices only) of the original one, with function names modified for the sake of readability. That being said, the original driver can be found in drivers/mfd/max8925-i2c.c in the kernel source tree.
Let's jump to our excerpt, starting with the context data structure definition, as follows:
```c
struct priv_chip {
struct device *dev;
struct regmap *regmap;
```
/* chip client for the parent chip, let's say the PMIC */
```c
struct i2c_client *client;
```
/* chip client for subdevice 1, let's say an rtc */
Introducing the MFD subsystem and Syscon APIs 131
```c
struct i2c_client *subdev1_client;
```
/* chip client for subdevice 2 let's say a gpio controller
*/
```c
struct i2c_client *subdev2_client;
struct regmap *subdev1_regmap;
struct regmap *subdev2_regmap;
unsigned short subdev1_addr; /* subdevice 1 I2C address */
unsigned short subdev2_addr; /* subdevice 2 I2C address */
};
const struct regmap_config chip_regmap_config = {
```
[...]
```c
};
const struct regmap_config subdev_rtc_regmap_config = {
```
[...]
```c
};
const struct regmap_config subdev_gpiochip_regmap_config = {
```
[...]
```c
};
```
In the preceding excerpt, the driver defines the context data structure, struct priv_chip, which contains subdevice regmaps, and then initializes the MFD device regmap configuration as well as the subdevice's own configuration. Then, the probe method is defined, as follows:
```c
static int my_mfd_probe(struct i2c_client *client,
const struct i2c_device_id *id)
{
struct priv_chip *chip;
struct regmap *map;
chip = devm_kzalloc(&client->dev,
```
sizeof(struct priv_chip), GFP_KERNEL);
map = devm_regmap_init_i2c(client, &chip_regmap_config);
```c
chip->client = client;
dts
chip->regmap = map;
```
132 Delving into the MFD Subsystem and Syscon API
```c
chip->dev = &client->dev;
dev_set_drvdata(chip->dev, chip);
i2c_set_clientdata(chip->client, chip);
chip->subdev1_addr = client->addr + 1;
chip->subdev2_addr = client->addr + 2;
```
/* subdevice 1, let's say an RTC */
```c
chip->subdev1_client = i2c_new_dummy(client->adapter,
chip->subdev1_addr);
dts
chip->subdev1_regmap =
c
devm_regmap_init_i2c(chip->subdev1_client,
```
&subdev_rtc_regmap_config);
```c
i2c_set_clientdata(chip->subdev1_client, chip);
```
/* subdevice 2, let's say a gpio controller */
```c
chip->subdev2_client = i2c_new_dummy(client->adapter,
chip->subdev2_addr);
dts
chip->subdev2_regmap =
c
devm_regmap_init_i2c(chip->subdev2_client,
```
&subdev_gpiochip_regmap_config);
```c
i2c_set_clientdata(chip->subdev2_client, chip);
```
/* mfd_add_devices() is called somewhere */
[...]
```c
}
```
Introducing the MFD subsystem and Syscon APIs 133
For the sake of readability, the preceding excerpt omits an error check. Additionally, the following code shows how to remove the dummy I2C clients:
```c
static int my_mfd_remove(struct i2c_client *client)
{
struct priv_chip *chip = i2c_get_clientdata(client);
mfd_remove_devices(chip->dev);
i2c_unregister_device(chip->subdev1_client);
i2c_unregister_device(chip->subdev2_client);
return 0;
}
```
Finally, the following simplified code shows how the subdevice driver can grab the pointer to either of the regmap data structures set up in the MFD driver:
```c
static int subdev_rtc_probe(struct platform_device *pdev)
{
struct priv_chip *chip = dev_get_drvdata(pdev->dev.parent);
dts
struct regmap *rtc_regmap = chip->subdev1_regmap;
c
int ret;
```
[...]
```c
if (!rtc_regmap) {
dev_err(&pdev->dev, "no regmap!\n");
```
ret = -EINVAL;
goto out;
```c
}
```
[...]
```c
}
```
Though we have most of the knowledge required to develop MFD device drivers, it is necessary to integrate this with the device tree in order to have a better (that is, not hardcoded) description of our MFD device. This is what we will discuss in the next section.
134 Delving into the MFD Subsystem and Syscon API
## Device tree binding for MFD devices
Even though we have the necessary tools and inputs to write our own MFD driver, it is important for the underlying MFD device to have its description defined in the device tree, since this lets the MFD core know what our MFD device is made of and how to deal with it. Moreover, the device tree remains the right place to declare devices, whether they are MFD or not. Please keep in mind that its purpose is only to describe devices on the system. As subdevices are children of the MFD device into which they are built (there is a parent-and-child bond of belonging), it is good practice to declare these subdevice nodes beneath their parent node, as in the following example. Moreover, the resources used by the subdevices are sometimes part of the resources of the parent device. So, it enforces the idea of putting the subdevice node beneath the main device node. In each subdevice node, the compatible property should match either both the subdevice's cell.
```c
of_compatible field and one of the .compatible string entries in the subdevice's platform_driver.of_match_table array, or both the subdevice's cell.name field and the subdevice's platform_driver.name field:
```
Important note
The subdevice's cell.of_compatible and cell.name fields are those declared in the subdevice's mfd_cell structure in the MFD core driver.
```c
&i2c3 {
```
pinctrl-names = "default";
pinctrl-0 = <&pinctrl_i2c3>;
clock-frequency = <400000>;
```dts
status = "okay";
c
pmic0: da9062@58 {
dts
compatible = "dlg,da9062";
reg = <0x58>;
```
pinctrl-names = "default";
pinctrl-0 = <&pinctrl_pmic>;
```dts
interrupt-parent = <&gpio6>;
interrupts = <11 IRQ_TYPE_LEVEL_LOW>;
```
interrupt-controller;
```dts
regulators {
c
DA9062_BUCK1: buck1 {
```
Device tree binding for MFD devices 135
regulator-name = "BUCK1";
regulator-min-microvolt = <300000>;
regulator-max-microvolt = <1570000>;
regulator-min-microamp = <500000>;
regulator-max-microamp = <2000000>;
regulator-boot-on;
```c
};
DA9062_LDO1: ldo1 {
```
regulator-name = "LDO_1";
regulator-min-microvolt = <900000>;
regulator-max-microvolt = <3600000>;
regulator-boot-on;
```c
};
};
dts
da9062_rtc: rtc {
compatible = "dlg,da9062-rtc";
c
};
dts
watchdog {
compatible = "dlg,da9062-watchdog";
c
};
dts
onkey {
compatible = "dlg,da9062-onkey";
```
dlg,disable-key-power;
```c
};
};
};
```
136 Delving into the MFD Subsystem and Syscon API
In the preceding device tree sample, the parent node (da9062, a PMIC, Power
Management Integrated Circuit) is declared under its bus node. The regulated output of this PMIC is declared as children of the PMIC node. Here, again, everything is normal.
Now, each subdevice is declared as a standalone device node under its parent (da9092,
actually) node. Let's focus on the subdevice's compatible properties and use onkey as an example. The MFD cell of this node is declared in the MFD core driver (whose source file is drivers/mfd/da9063-core.c), as follows:
```c
static struct resource da9063_onkey_resources[] = {
{
```
.name = "ONKEY",
.start = DA9063_IRQ_ONKEY,
.end = DA9063_IRQ_ONKEY,
.flags = IORESOURCE_IRQ,d
```c
},
};
static const struct mfd_cell da9062_devs[] = {
```
[...]
```c
{
```
.name = "da9062-onkey",
.num_resources = ARRAY_SIZE(da9062_onkey_resources),
.resources = da9062_onkey_resources,
```dts
.of_compatible = "dlg,da9062-onkey",
c
},
};
```
Now, this onekey platform driver structure is declared (along with its .of_match_
table entry) in the driver (whose source file is drivers/input/misc/da9063_
onkey.c), as follows:
```c
static const struct of_device_id da9063_compatible_reg_id_table[] = {
dts
{ .compatible = "dlg,da9063-onkey", .data = &da9063_regs },
{ .compatible = "dlg,da9062-onkey", .data = &da9062_regs },
c
{ },
};
MODULE_DEVICE_TABLE(of, da9063_compatible_reg_id_table);
```
Understanding Syscon and simple-mfd 137
[...]
```c
static struct platform_driver da9063_onkey_driver = {
```
.probe = da9063_onkey_probe,
```c
.driver = {
```
.name = DA9063_DRVNAME_ONKEY,
.of_match_table = da9063_compatible_reg_id_table,
```c
},
};
```
You can see that both compatible strings match the node's compatible string in the device's node. On the other hand, we can see that the same platform driver may be used for two or more (sub)devices. Using name matching would be confusing, then. That is why you would use a device tree for declaration and a compatible string for matching.
So far, we have learned how the MFD subsystem deals with the device and vice versa. In the next section, we will extend these concepts to syscon and simple-mfd, two frameworks that help with MFD driver development.
## Understanding Syscon and simple-mfd
Syscon stands for system controller. SoCs sometimes have a set of MMIO registers dedicated to miscellaneous features that don't relate to a specific IP. Clearly, there can't be a functional driver for this as these registers are neither representative nor cohesive enough to represent a specific type of device. The syscon driver handles this kind of situation. Syscon permits other nodes to access this register space through the regmap mechanism. It is actually just a set of wrapper APIs for regmap. When you request access to syscon, the regmap is created, if it doesn't exist yet.
The header required for using the syscon API is <linux/mfd/syscon.h>. As this
API is based on regmap, you must also include <linux/regmap.h>. The syscon API
is implemented in drivers/mfd/syscon.c in the kernel source tree. Its main data structure is struct syscon, though this structure is not to be used directly:
```c
struct syscon {
struct device_node *np;
struct regmap *regmap;
struct list_head list;
};
```
138 Delving into the MFD Subsystem and Syscon API
In the preceding structure, np is a pointer to the node acting as syscon. It is also used for syscon lookup by the device node. regmap is the regmap associated with this syscon,
and list is used for implementing a kernel linked-lists mechanism, used to link all the syscons in the system together to the system-wide list, syscon_list, defined in drivers/mfd/syscon.c. This linked-list mechanism allows walking through the whole syscon list, either for a match by node or for a match by regmap.
Syscons are declared exclusively from within the device tree, by adding "syscon" to the compatible strings list in the device node that should act as Syscon. During early-boot,
each node having syscon in its compatible string list will have its reg memory region
IO-mapped and bound to an MMIO regmap, according to a default regmap configuration,
syscon_regmap_config, as follows:
```c
static const struct regmap_config syscon_regmap_config = {
```
.reg_bits = 32,
.val_bits = 32,
.reg_stride = 4,
```c
};
```
The syscon that is created is then added to the syscon framework-wide syscon_list,
protected by the syscon_list_slock spinlock, as follows:
```c
static DEFINE_SPINLOCK(syscon_list_slock);
static LIST_HEAD(syscon_list);
static struct syscon *of_syscon_register(struct device_node
```
*np)
```c
{
struct syscon *syscon;
struct regmap *regmap;
void __iomem *base;
```
[...]
```c
if (!of_device_is_compatible(np, "syscon"))
return ERR_PTR(-EINVAL);
```
[...]
```c
spin_lock(&syscon_list_slock);
list_add_tail(&syscon->list, &syscon_list);
spin_unlock(&syscon_list_slock);
```
Understanding Syscon and simple-mfd 139
```c
return syscon;
}
```
Syscon binding requires the following mandatory properties:
• compatible: This property value should be "syscon".
• reg: This is the register region that can be accessed from syscon.
The following are optional properties, used to mangle the default syscon_regmap_
config regmap config:
• reg-io-width: The size (or width, in terms of bytes) of the IO accesses that should be performed on the device
• hwlocks: Reference to a phandle of a hardware spinlock provider node
An example is shown in the following, an excerpt from the kernel docs, whose full version is available in Documentation/devicetree/bindings/mfd/syscon.txt in the kernel sources:
```c
gpr: iomuxc-gpr@20e0000 {
dts
compatible = "fsl,imx6q-iomuxc-gpr", "syscon";
reg = <0x020e0000 0x38>;
```
hwlocks = <&hwlock1 1>;
```c
};
hwlock1: hwspinlock@40500000 {
```
...
```dts
reg = <0x40500000 0x1000>;
```
#hwlock-cells = <1>;
```c
};
```
From within the device tree, you can reference a syscon node in three different ways:
either by phandle (specified in the device node of this driver), by its path, or by searching it using a specific compatible value, after which the driver can interrogate the node (or associated OS driver of this regmap) to determine the location of the registers, and finally,
access the registers directly. You can use one of the following syscon APIs in order to grab a pointer to the regmap associated with a given syscon node:
```c
struct regmap * syscon_node_to_regmap (struct device_node *np);
struct regmap * syscon_regmap_lookup_by_compatible(const char
```
*s);
140 Delving into the MFD Subsystem and Syscon API
```c
struct regmap * syscon_regmap_lookup_by_pdevname(const char
```
*s);
```c
struct regmap * syscon_regmap_lookup_by_phandle(
struct device_node *np,
const char *property);
```
The preceding APIs have the following descriptions:
• syscon_regmap_lookup_by_compatible(): Given one of the compatible strings of the syscon device node, this function returns the associated regmap, or creates one if it does not exist yet, before returning it.
• syscon_node_to_regmap(): Given a syscon device node as a parameter, this function returns the associated regmap, or creates one if it does not exist yet, before returning it.
• syscon_regmap_lookup_by_phandle(): Given a phandle property holding an identifier of a syscon node, this function returns the regmap corresponding to this syscon node.
Before showing an example of using the preceding APIs, let's introduce the following platform device node, for which we will write the probe function. To better understand syscon_node_to_regmap(), let's declare this node as a child of the previous gpr node:
```c
gpr: iomuxc-gpr@20e0000 {
dts
compatible = "fsl,imx6q-iomuxc-gpr", "syscon";
reg = <0x020e0000 0x38>;
c
my_pdev: my_pdev {
dts
compatible = "company,regmap-sample";
```
regmap-phandle = <&gpr>;
[...]
```c
};
};
```
Now that the device tree node is defined, we can focus on the code of the driver,
implemented as follows and using the functions enumerated earlier:
```c
static struct regmap *by_node_regmap;
static struct regmap *by_compat_regmap;
static struct regmap *by_pdevname_regmap;
```
Understanding Syscon and simple-mfd 141
```c
static struct regmap *by_phandle_regmap;
static int my_pdev_regmap_sample(struct platform_device *pdev)
{
struct device_node *np = pdev->dev.of_node;
struct device_node *syscon_node;
```
[...]
syscon_node = of_get_parent(np);
```c
if (!syscon_node)
return -ENODEV;
```
/* If we have a pointer to the syscon device node,
we use it */
```dts
by_node_regmap = syscon_node_to_regmap(syscon_node);
c
of_node_put(syscon_node);
if (IS_ERR(by_node_regmap)) {
```
pr_err("%s: could not find regmap by node\n",
__func__);
```c
return PTR_ERR(by_node_regmap);
}
```
/* or we have one of the compatible string of the syscon node */
```dts
by_compat_regmap =
```
syscon_regmap_lookup_by_compatible("fsl,
imx6q-iomuxc-gpr");
```c
if (IS_ERR(by_compat_regmap)) {
```
pr_err("%s: could not find regmap by compatible\n",
__func__);
```c
return PTR_ERR(by_compat_regmap);
}
```
/* Or a phandle property pointing to the syscon device node
*/
```dts
by_phandle_regmap =
c
syscon_regmap_lookup_by_phandle(np, "fsl,tempmon");
if (IS_ERR(map)) {
```
142 Delving into the MFD Subsystem and Syscon API
pr_err("%s: could not find regmap by phandle\n",
__func__);
```c
return PTR_ERR(by_phandle_regmap);
}
```
/*
* It is the extrem and rare case fallback
* As of Linux kernel v4.18, there is only one driver
* using this, drivers/tty/serial/clps711x.c
*/
char pdev_syscon_name[9];
```c
int index = pdev->id;
sprintf(syscon_name, "syscon.%i", index + 1);
dts
by_pdevname_regmap =
c
syscon_regmap_lookup_by_pdevname(syscon_name);
if (IS_ERR(by_pdevname_regmap)) {
```
pr_err("%s: could not find regmap by pdevname\n",
__func__);
```c
return PTR_ERR(by_pdevname_regmap);
}
```
[...]
```c
return 0;
}
```
In the preceding example, if we consider that syscon_name contains the platform device name for the gpr device, then the by_node_regmap, by_compat_regmap,
by_pdevname_regmap, and by_phandle_regmap variables will all point to the same syscon regmap. However, the purpose here is just to explain the concept. my_pdev could have been the sibling (or whatever relationship) node of gpr. Using it here as its child was done for the sake of understanding the concept and the code and showing that either
API has its place, depending on the situation. Now that we are familiar with the syscon framework, let's see how it can be used along with simple-mfd.
Understanding Syscon and simple-mfd 143
## Introducing simple-mfd
For MMIO-based MFD devices, there may be no need to configure subdevices prior to adding them to the system. As this configuration is done from within the MFD core driver, the only goal of this MFD core driver would be to populate the system with platform subdevices. As a lot of these MMIO-based MFD devices exist, there would be a lot of redundant code. The simple MFD, which is a simple DT binding, addresses this.
When the simple-mfd string is added to the list of compatible strings of a given device node (considered here as the MFD device), it will make the OF (open firmware) core spawn child devices (subdevices, actually) for all subnodes of that MFD device, using the for_each_child_of_node() iterator. simple-mfd is implemented in drivers/
of/platform.c as an alias of simple-bus, and its documentation is located in
Documentation/devicetree/bindings/mfd/mfd.txt in the kernel source tree.
Used in conjunction with syscon to create the regmap, it helps to avoid writing an MFD
driver, and the developer can put their effort into writing subdevice drivers. The following is an example:
```c
snvs: snvs@20cc000 {
dts
compatible = "fsl,sec-v4.0-mon", "syscon", "simple-mfd";
reg = <0x020cc000 0x4000>;
c
snvs_rtc: snvs-rtc-lp {
dts
compatible = "fsl,sec-v4.0-mon-rtc-lp";
regmap = <&snvs>;
```
offset = <0x34>;
```dts
interrupts = <GIC_SPI 19 IRQ_TYPE_LEVEL_HIGH>,
```
<GIC_SPI 20 IRQ_TYPE_LEVEL_HIGH>;
```c
};
snvs_poweroff: snvs-poweroff {
dts
compatible = "syscon-poweroff";
regmap = <&snvs>;
```
offset = <0x38>;
value = <0x60>;
mask = <0x60>;
```dts
status = "disabled";
c
};
```
144 Delving into the MFD Subsystem and Syscon API
```c
snvs_pwrkey: snvs-powerkey {
dts
compatible = "fsl,sec-v4.0-pwrkey";
regmap = <&snvs>;
interrupts = <GIC_SPI 4 IRQ_TYPE_LEVEL_HIGH>;
linux,keycode = <KEY_POWER>;
wakeup-source;
c
};
```
[...]
```c
};
```
In the preceding device tree excerpt, snvs is the main device. It is made up of a power control subdevice (represented by a register subregion in the main device register region),
an rtc subdevice, as well as a power key, and so on. The whole definition can be found in arch/arm/boot/dts/imx6qdl.dtsi, which is the SoC vendor dtsi for the i.MX6 chip series. The respective drivers can be found in the kernel source by grepping
(searching for) the content of their compatible properties. To summarize, for each subnode in the snvs node, the MFD core will create a corresponding device along with its regmap, which would correspond to their memory region from within the main device's memory region.
This section shows the way to ease into MFD driver development when it comes to MMIO
devices. Though SPI/I2C devices do not fall into this category, it covers almost 95% of
MMIO-based MFD devices.
## Summary
This chapter dealt with MFD devices, along with the syscon and regmap APIs. Here,
we discussed how MFD devices work and how deep regmap is embedded into syscon.
Having reached the end of this chapter, we can assume that you are able to develop regmap-enabled IRQ controllers, as well as to design and use syscon to share register regions between devices. The next chapter will deal with the common clock framework and how this framework is organized, its implementation, how to use it, and how to add your own clocks