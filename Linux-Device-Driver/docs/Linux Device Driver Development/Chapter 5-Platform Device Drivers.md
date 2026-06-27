# Chapter 5 - Platform Device Drivers 

We all know about Plug and Play devices. They are handled by the kernel as soon as they are plugged in. These may be USB or PCI Express, or any other auto-discovered devices.
But, other device types also exist, which are not hot-pluggable and that the kernel needs to know about prior to being managed. There are I2C, UART, SPI, and other devices not wired to enumeration-capable buses.
There are real physical buses you may already know about: USB, I2S, I2C, UART, SPI, PCI,
SATA, and so on. Such buses are hardware devices called controllers. Since they are a part of SoC, they can't be removed, are non-discoverable, and are also called platform devices.
People often say platform devices are on-chip devices (embedded in the
SoC). In practice, this is partially true, since they are hard-wired into the chip and can't be removed. But devices connected to I2C or SPI are not onchip, and are platform devices too because they are not discoverable.
Similarly, there may be on-chip PCI or USB devices, but they are not platform devices, because they are discoverable.
From an SoC point of view, those devices (buses) are connected internally through dedicated buses, and are most of the time proprietary and specific to the manufacturer.
From the kernel point of view, these are root devices and connected to nothing. That is where the pseudo platform bus comes in. The pseudo platform bus, also called platform bus,
is a kernel virtual bus for devices that are not seated on a physical bus known to the kernel.
In this chapter, platform devices refers to devices that rely on the pseudo platform bus.
Dealing with platform devices essentially requires two steps:
1. Register a platform driver (with a unique name) that will manage your devices
2. Register your platform device with the same name as the driver, and their resources, in order to let the kernel know that your device is there
That being said, in this chapter, we will discuss the following:
Platform devices along with their drivers
Devices and driver matching mechanisms in the kernel
Registering platform drivers with devices, as well as platform data
## Platform drivers
Before going any further, please pay attention to the following warning: not all platform devices are handled by platform drivers (or should I say pseudo platform drivers). Platform drivers are dedicated to devices not based on conventional buses. I2C devices or SPI
devices are platform devices, but respectively rely on I2C or SPI buses not on the platform bus. Everything needs to be done manually with the platform driver. The platform driver must implement a probe function, called by the kernel when the module is inserted or when a device claims it. When developing platform drivers, the main structure you have to fill is struct platform_driver, and you have to register your driver with the platform bus core with dedicated functions, shown as follows:
```c
static struct platform_driver mypdrv = {
```
.probe = my_pdrv_probe,
.remove = my_pdrv_remove,
.driver = {
.name = "my_platform_driver",
.owner = THIS_MODULE,
```c
},
};
```
Let's see what the meaning of each element that composes the structure is, and what they are used for:
probe(): This is the function that gets called when a device claims your driver after a match occurs. Later, we will see how probe is called by the core. Its declaration is as follows:
```c
static int my_pdrv_probe(struct platform_device *pdev)
```
remove(): This is called to get rid of the driver when it is not needed anymore by devices, and its declaration looks like this:
```c
static int my_pdrv_remove(struct platform_device *pdev)
struct device_driver: This describes the driver itself, providing a name,
```
owner, and some fields, which we will see later.
Registering a platform driver with the kernel is as simple as calling platform_driver_register() or platform_driver_probe() in the init function
(when the module is loaded). The difference between those functions is that:
```c
platform_driver_register() registers and puts the driver into a list of drivers maintained by the kernel, so that its probe() function can be called on demand whenever a new match occurs. To prevent your driver from being inserted and registered in that list, just use the next function.
```
With platform_driver_probe(), the kernel immediately runs the match loop,
checks if there is a platform device with the matching name, and then calls the driver's probe() if a match occurred, meaning that the device is present. If not,
the driver is ignored. This method prevents the deferred probe, since it does not register the driver on the system. Here, the probe function is placed in an
__init section, which is freed when the kernel boot has completed, thus preventing the deferred probe and reducing the driver's memory footprint. Use this method if you are 100 percent sure the device is present in the system:
ret = platform_driver_probe(&mypdrv, my_pdrv_probe);
The following is a simple platform driver that registers itself with the kernel:
```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
static int my_pdrv_probe (struct platform_device *pdev){
pr_info("Hello! device probed!\n");
return 0;
}
static void my_pdrv_remove(struct platform_device *pdev){
pr_info("good bye reader!\n");
}
static struct platform_driver mypdrv = {
```
.probe = my_pdrv_probe,
.remove = my_pdrv_remove,
.driver = {
.name = KBUILD_MODNAME,
.owner = THIS_MODULE,
```c
},
};
static int __init my_pdrv_init(void)
{
pr_info("Hello Guy\n");
```
/* Registering with Kernel */
```c
platform_driver_register(&mypdrv);
return 0;
}
static void __exit my_pdrv_remove (void)
{
Pr_info("Good bye Guy\n");
```
/* Unregistering from Kernel */
```c
platform_driver_unregister(&mypdriver);
}
module_init(my_pdrv_init);
module_exit(my_pdrv_remove);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("John Madieu");
MODULE_DESCRIPTION("My platform Hello World module");
```
Our module does nothing else in the init/exit function but register/unregister with the platform bus core. This is the case with most drivers. In this case, we can get rid of module_init and module_exit, and use the module_platform_driver macro.
The module_platform_driver macro looks as follows:
/*
* module_platform_driver() - Helper macro for drivers that don't
* do anything special in module init/exit. This eliminates a lot
* of boilerplate. Each module may only use this macro once, and
* calling it replaces module_init() and module_exit()
*/
```c
#define module_platform_driver(__platform_driver) \
module_driver(__platform_driver, platform_driver_register, \
platform_driver_unregister)
```
This macro will be responsible for registering our module with the platform driver core. No need for module_init and module_exit macros, nor init and exit functions anymore.
It does not mean that those functions are not called anymore, just that we can forget about writing them ourselves.
The probe function is not a substitute to init function. The probe function is called every time a given device matches with the driver,
whereas the init function runs only once, when the module gets loaded.
[...]
```c
static int my_driver_probe (struct platform_device *pdev){
```
[...]
```c
}
static void my_driver_remove(struct platform_device *pdev){
```
[...]
```c
}
static struct platform_drivermy_driver = {
```
[...]
```c
};
module_platform_driver(my_driver);
```
There are specific macros for each bus that you need to register the driver with. The following list is not exhaustive:
```c
module_platform_driver(struct platform_driver) for platform drivers,
dedicated to devices that do not sit on conventional physical buses (we just used it before)
module_spi_driver(struct spi_driver) for SPI drivers module_i2c_driver(struct i2c_driver) for I2C drivers module_pci_driver(struct pci_driver) for PCI drivers module_usb_driver(struct usb_driver) for USB drivers module_mdio_driver(struct mdio_driver) for mdio
```
[...]
If you don't know which bus your driver needs to sit on, then it is a platform driver, and you should use platform_driver_register or platform_driver_probe to register the driver.
## Platform devices
Actually, we should have said pseudo platform device, since this section concerns devices that sit on pseudo platform buses. When you are done with the driver, you will have to feed the kernel with devices needing that driver. A platform device is represented in the kernel as an instance of struct platform_device, and looks as follows:
```c
struct platform_device {
const char *name;
```
u32 id;
```c
struct device dev;
```
u32 num_resources;
```c
struct resource *resource;
};
```
When it comes to the platform driver, before driver and device match, the name field of both struct platform_device and static struct platform_driver.driver.name must be the same. The num_resources and struct resource *resource field will be covered in the next section. Just remember that, since resource is an array,
num_resources must contain the size of that array.
## Resources and platform data
At the opposite end to hot-pluggable devices, the kernel has no idea what devices are present on your system, what they are capable of, or what they need in order to work properly. There is no auto-negotiation process, so any information provided to the kernel would be welcome. There are two methods to inform the kernel about the resources (IRQ,
DMA, memory region, I/O ports, buses) and data (any custom and private data structure you may want to pass to the driver) that the device needs, which are discussed here.
## Device provisioning – the old and deprecated way
This method is to be used with the kernel version that does not support a device tree. With this method, drivers remain generic and devices are registered in board-related source files.
## Resources
Resources represent all the elements that characterize the device from the hardware point of view, and that the device needs in order to be set up and work properly. There are only six types of resources in the kernel, all listed in include/linux/ioport.h, and used as flags to describe the resource's type:
```c
#define IORESOURCE_IO 0x00000100 /* PCI/ISA I/O ports */
#define IORESOURCE_MEM 0x00000200 /* Memory regions */
#define IORESOURCE_REG 0x00000300 /* Register offsets */
#define IORESOURCE_IRQ 0x00000400 /* IRQ line */
#define IORESOURCE_DMA 0x00000800 /* DMA channels */
#define IORESOURCE_BUS 0x00001000 /* Bus */
```
A resource is represented in the kernel as an instance of struct resource:
```c
struct resource {
```
resource_size_t start;
resource_size_t end;
```c
const char *name;
unsigned long flags;
};
```
Let's explain the meaning of each element in the structure:
start/end: This represents where the resource begins/ends. For I/O or memory regions, it represents where they begin/end. For IRQ lines, buses or DMA
channels, start/end must have the same value.
flags: This is a mask that characterizes the type of resource, for example
IORESOURCE_BUS.
name: This identifies or describes the resource.
Once you have provided the resources, you need to extract them back to the driver in order to work with them. The probe function is a good place to extract them. Before you go any further, let's remember the declaration of the probe function for a platform device driver:
```c
int probe(struct platform_device *pdev);
```
pdev is automatically filled by the kernel with the data and resource we registered earlier.
Let's see how to pick them.
The struct resource embedded in struct platform_device can be retrieved with the platform_get_resource() function. The following is the prototype of platform_get_resource:
```c
struct resource *platform_get_resource(struct platform_device *dev,unsigned int type, unsigned int num);
```
The first parameter is an instance of the platform device itself. The second parameter says what kind of resource we need. For memory, it should be IORESOURCE_MEM. Again, please have a look at include/linux/ioport.h for more details. The num parameter is an index that says which resource type is desired. Zero indicates the first one, and so on.
If the resource is an IRQ, we must use int platform_get_irq(struct platform_device * pdev, unsigned int num), where pdev is the platform device and num is the IRQ index within the resource (in case there is more than one). The whole probe function that we can use to extract the platform data that we registered for our device can look as follows:
```c
static int my_driver_probe(struct platform_device *pdev)
{
struct my_gpios *my_gpio_pdata =
```
(struct my_gpios*)dev_get_platdata(&pdev->dev);
```c
int rgpio = my_gpio_pdata->reset_gpio;
int lgpio = my_gpio_pdata->led_gpio;
struct resource *res1, *res2;
void *reg1, *reg2;
int irqnum;
```
res1 = platform_get_resource(pdev, IORESSOURCE_MEM, 0);
if((!res1)){
```c
pr_err(" First Resource not available");
return -1;
}
```
res2 = platform_get_resource(pdev, IORESSOURCE_MEM, 1);
if((!res2)){
```c
pr_err(" Second Resource not available");
return -1;
}
```
/* extract the irq */
irqnum = platform_get_irq(pdev, 0);
```c
pr_info("IRQ number of Device: %d\n", irqnum);
```
/*
* At this step, we can use gpio_request, on gpio,
* request_irq on irqnum and ioremap() on reg1 and reg2.
* ioremap() is discussed in chapter 11, Kernel Memory Management
*/
[...]
```c
return 0;
}
```
## Platform data
Any other data whose type is not a part of the resource types enumerated in the preceding section falls here (for example, GPIO). Whatever their type is, the struct platform_device contains a struct device field, which in turn contains a struct platform_data field. Usually, you should embed that data in a structure and pass it to the platform_device.device.platform_data field. Let's say, for example, that you declare a platform device that needs two GPIO numbers as platform data, one IRQ number, and two memory regions as resources. The following example shows how to register platform data along with the device. Here, we use the platform_device_register(struct platform_device *pdev) function, which you use to register a platform device with the platform core:
/*
* Other data than IRQ or memory must be embedded in a structure
* and passed to "platform_device.device.platform_data"
*/
```c
struct my_gpios {
int reset_gpio;
int led_gpio;
};
```
/*our platform data*/
```c
static struct my_gpios needed_gpios = {
```
.reset_gpio = 47,
.led_gpio = 41,
```c
};
```
/* Our resource array */
```c
static struct resource needed_resources[] = {
```
[0] = { /* The first memory region */
.start = JZ4740_UDC_BASE_ADDR,
.end = JZ4740_UDC_BASE_ADDR + 0x10000 - 1,
.flags = IORESOURCE_MEM,
.name = "mem1",
```c
},
```
[1] = {
.start = JZ4740_UDC_BASE_ADDR2,
.end = JZ4740_UDC_BASE_ADDR2 + 0x10000 -1,
.flags = IORESOURCE_MEM,
.name = "mem2",
```c
},
```
[2] = {
.start = JZ4740_IRQ_UDC,
.end = JZ4740_IRQ_UDC,
.flags = IORESOURCE_IRQ,
.name = "mc",
```c
},
};
static struct platform_device my_device = {
```
.name = "my-platform-device",
.id = 0,
.dev = {
.platform_data = &needed_gpios,
```c
},
```
.resource = needed_resources,
.num_resources = ARRY_SIZE(needed_resources),
```c
};
platform_device_register(&my_device);
```
In the preceding example, we used IORESOURCE_IRQ and IORESOURCE_MEM in order to inform the kernel about what kind of resource we provided. To see all other flag types,
have a look at include/linux/ioport.h in the kernel tree.
In order to retrieve the platform data we registered earlier, we could have just used pdev->dev.platform_data (remember the struct platform_device structure), but it is recommended to use the kernel-provided function (which does the same thing,
admittedly):
```c
void *dev_get_platdata(const struct device *dev)
struct my_gpios *picked_gpios = dev_get_platdata(&pdev->dev);
```
## Where to declare platform devices?
Devices are registered along with their resources and data. In this old and deprecated method, they are declared in a separate module, or in the board init file in the arch/<arch>/mach-xxx/yyyy.c, which is arch/arm/mach-imx/mach-imx6q.c in our case, since we use a UDOO quad based on an i.MX6Q from NXP. The platform_device_register() function lets you do that:
```c
static struct platform_device my_device = {
```
.name = "my_drv_name",
.id = 0,
.dev.platform_data = &my_device_pdata,
.resource = jz4740_udc_resources,
.num_resources = ARRY_SIZE(jz4740_udc_resources),
```c
};
platform_device_register(&my_device);
```
The name of the device is very important, and is used by the kernel to match the driver with the same name.
## Device provisioning – the new and recommended way
In the first method, any modification will necessitate rebuilding the whole kernel. If the kernel had to include any application/board-specific configurations, its size would greatly increase. In order to keep things simple and separate device declarations (since they are not really part of the kernel) from the kernel source, a new concept has been introduced: the device tree. The main goal of DTS is to remove very specific and never-tested code from the kernel. With the device tree, platform data and resources are homogeneous. The device tree is a hardware description file and has a format similar to a tree structure, where every device is represented with a node, and any data or resource or configuration data is represented as the node's property. This way, you only need to recompile the device tree when you make some modifications. The device tree forms the subject of the next chapter,
and we will see how to introduce it to the platform device.
## Devices, drivers, and bus matching
Before any match can occur, Linux calls platform_match(struct device *dev,
```c
struct device_driver *drv). Platform devices are matched with their drivers by means of strings. According to the Linux device model, the bus element is the most important part. Each bus maintains a list of drivers and devices that are registered with it.
```
The bus driver is responsible for devices and drivers matching. Any time you connect a new device or add a new driver to a bus, that bus starts the matching loop.
Now, suppose that you register a new I2C device using functions provided by the I2C core
(discussed in next chapter). The kernel will trigger the I2C bus matching loop, by calling the
I2C core match function registered with the I2C bus driver to check whether there is already a registered driver that matches with your device. If there is no match, nothing will happen. If a match occurs, the kernel will notify (by means of a communication mechanism called netlink socket) the device manager (udev/mdev), which will load (if not loaded yet)
the driver your device matched with. Once the driver loads, its probe() function will immediately be executed. Not only does I2C work like that, but every bus has its own matching mechanism that is roughly the same. A bus matching loop is triggered at each device or driver registration.
We can sum up what we have said in the preceding section in the following diagram:
Every registered driver and device sits on a bus. This makes a tree. USB buses may be children of PCI buses, whereas MDIO buses are generally children of other devices, and so on. Thus, our preceding diagram changes to this:
When you register a driver with the platform_driver_probe() function, the kernel walks through the table of registered platform devices and looks for a match. If any, it calls the matched driver's probe function with the platform data.
How can platform devices and platform drivers match?
So far, we have only discussed how to fill different structures of both devices and drivers,
but now we will see how they are registered with the kernel, and how Linux knows which devices are handled by which driver. The answer is MODULE_DEVICE_TABLE. At compilation time, the build process extracts this information out of the driver and builds a human readable file called modules.alias, and located in the directory
/lib/modules/kernel_version/.
This macro lets a driver expose its ID table, which describes which devices it can support.
In the meantime, if the driver can be compiled as a module, the driver.name field should match the module name. If it does not match, the module won't be automatically loaded,
unless we have used the MODULE_ALIAS macro to add another name for the module. At compilation time, that information is extracted from all the drivers in order to build a device table. When the kernel has to find the driver for a device (when a matching needs to be performed), the device table is walked through by the kernel. If an entry is found matching the compatible (for device tree), device/vendor id, or name (for device ID
table or name) values of the added device, then the module providing that match is loaded
(running the module's init function), and the probe function is called. The
```c
MODULE_DEVICE_TABLE macro is defined in linux/module.h:
#define MODULE_DEVICE_TABLE(type, name)
```
The following is the description of each parameter given to this macro:
type: This can be either i2c, spi, acpi, of, platform, usb, pci or any other bus you may find in include/linux/mod_devicetable.h. It depends on the bus our device sits on, or on the matching mechanism we want to use.
name: This is a pointer on a XXX_device_id array, used for device matching. If we were talking about I2C devices, the structure would be i2c_device_id. For an SPI device, it should be spi_device_id, and so on. For the device tree Open
Firmware (OF) matching mechanism, we must use of_device_id.
For new non-discoverable platform device drivers, it is recommended not to use platform data anymore, but to use device tree capabilities instead,
with the OF matching mechanism. Please do note that the two methods are not mutually exclusive, thus you can mix these together.
Let's go deeper into the details for matching mechanisms, except for the OF style match,
which we will discuss in Chapter 6, The Concept of a Device Tree.
## Kernel devices and drivers-matching function
The function responsible for platform devices and driver-matching functions in the kernel is defined in /drivers/base/platform.c as follows:
```c
static int platform_match(struct device *dev, struct device_driver *drv)
{
struct platform_device *pdev = to_platform_device(dev);
struct platform_driver *pdrv = to_platform_driver(drv);
```
/* When driver_override is set, only bind to the matching driver */
```c
if (pdev->driver_override)
return !strcmp(pdev->driver_override, drv->name);
```
/* Attempt an OF style match first */
```c
if (of_driver_match_device(dev, drv))
return 1;
```
/* Then try ACPI style match */
```c
if (acpi_driver_match_device(dev, drv))
return 1;
```
/* Then try to match against the id table */
```c
if (pdrv->id_table)
return platform_match_id(pdrv->id_table, pdev) != NULL;
```
/* fall-back to driver name match */
```c
return (strcmp(pdev->name, drv->name) == 0);
}
```
We can enumerate four matching mechanisms. They are all based on the string compare. If we have a look at platform_match_id, we'll understand how things work underneath:
```c
static const struct platform_device_id *platform_match_id(
const struct platform_device_id *id,
struct platform_device *pdev)
{
while (id->name[0]) {
if (strcmp(pdev->name, id->name) == 0) {
```
pdev->id_entry = id;
```c
return id;
}
```
id++;
```c
}
return NULL;
}
```
Now, let's have a look at the struct device_driver structure we discussed in Chapter
4, Character Device Drivers:
```c
struct device_driver {
const char *name;
```
[...]
```c
const struct of_device_id *of_match_table;
const struct acpi_device_id *acpi_match_table;
};
```
I intentionally removed fields that we are not interested in. struct device_driver forms the basis of every device driver. Whether it is an I2C, SPI, TTY, or other device driver, they all embed a struct device_driver element.
## OF style and ACPI match
OF style is explained in Chapter 6, The Concept of Device Tree. The second mechanism is an
ACPI table-based matching. We'll not discuss it at all in this book, but for your information,
it uses the acpi_device_id struct.
## ID table matching
This match style has been around for a long time, and is based on the struct device_id structure. All device ID structures are defined in include/linux/mod_devicetable.h.
To find the right structure name, you need to prefix device_id with the bus name your device driver sits on. Examples are struct i2c_device_id for I2C, struct platform_device_id for platform devices (which don't sit on a real physical bus),
spi_device_id for SPI devices, usb_device_id for USB, and so on. The typical structure of a device_id table for a platform device is as follows:
```c
struct platform_device_id {
```
char name[PLATFORM_NAME_SIZE];
kernel_ulong_t driver_data;
```c
};
```
Anyway, if an ID table is registered, it will be walked through whenever the kernel has run the match function to find a driver for an unknown or new platform device. If there is a match, the probe function of the matched driver will be invoked, and given a struct platform_device as a parameter, which will hold a pointer to the matching ID table entry that originated the match. The .driver_data element is an unsigned long, which is sometimes cast into pointer addresses in order to point to anything, just like in the serialimx driver. The following is an example with platform_device_id in drivers/tty/serial/imx.c:
```c
static const struct platform_device_id imx_uart_devtype[] = {
{
```
.name = "imx1-uart",
.driver_data = (kernel_ulong_t) &imx_uart_devdata[IMX1_UART],
```c
}, {
```
.name = "imx21-uart",
.driver_data = (kernel_ulong_t)
&imx_uart_devdata[IMX21_UART],
```c
}, {
```
.name = "imx6q-uart",
.driver_data = (kernel_ulong_t)
&imx_uart_devdata[IMX6Q_UART],
```c
}, {
```
/* sentinel */
```c
}
};
```
The .name field must be the same as the device's name you give when you register the device in the board-specific file. The function responsible for this match style is platform_match_id. If you look at its definition in drivers/base/platform.c, you'll see:
```c
static const struct platform_device_id *platform_match_id(
const struct platform_device_id *id,
struct platform_device *pdev)
{
while (id->name[0]) {
if (strcmp(pdev->name, id->name) == 0) {
```
pdev->id_entry = id;
```c
return id;
}
```
id++;
```c
}
return NULL;
}
```
In the following example, which is an excerpt from drivers/tty/serial/imx.c in kernel sources, you can see how the platform data is converted back into the original data structure, just by casting. That is how people sometimes pass any data structure as platform data:
```c
static void serial_imx_probe_pdata(struct imx_port *sport,
struct platform_device *pdev)
{
struct imxuart_platform_data *pdata = dev_get_platdata(&pdev->dev);
```
sport->port.line = pdev->id;
sport->devdata = (structimx_uart_data *) pdev->id_entry->driver_data;
```c
if (!pdata)
```
return;
[...]
```c
}
```
pdev->id_entry is a struct platform_device_id, which is a pointer to the matching
ID table entry made available by the kernel, and whose driver_data element is cast back to a pointer on the data structure.
## Per device-specific data on ID table matching
In the previous section, we used platform_device_id.platform_data as a pointer.
Your driver may need to support more than one device type. In this situation, you will need specific device data for each device type you support. You should then use the device ID as an index to an array that contains every possible device data, and not as a pointer address anymore. The following are detailed steps in an example:
1. We define an enumeration, depending on the device type that we need to support in our driver:
```c
enum abx80x_chip {
```
AB0801,
AB0803,
AB0804,
AB0805,
AB1801,
AB1803,
AB1804,
AB1805,
ABX80X
```c
};
```
2. We define the specific data-type structure:
```c
struct abx80x_cap {
```
u16 pn;
bool has_tc;
```c
};
```
3. We fill an array with default values, and depending on the index in device_id,
we can pick the right data:
```c
static struct abx80x_cap abx80x_caps[] = {
```
[AB0801] = {.pn = 0x0801},
[AB0803] = {.pn = 0x0803},
[AB0804] = {.pn = 0x0804, .has_tc = true},
[AB0805] = {.pn = 0x0805, .has_tc = true},
[AB1801] = {.pn = 0x1801},
[AB1803] = {.pn = 0x1803},
[AB1804] = {.pn = 0x1804, .has_tc = true},
[AB1805] = {.pn = 0x1805, .has_tc = true},
[ABX80X] = {.pn = 0}
```c
};
```
4. We define our platform_device_id with a specific index:
```c
static const struct i2c_device_id abx80x_id[] = {
{ "abx80x", ABX80X },
{ "ab0801", AB0801 },
{ "ab0803", AB0803 },
{ "ab0804", AB0804 },
{ "ab0805", AB0805 },
{ "ab1801", AB1801 },
{ "ab1803", AB1803 },
{ "ab1804", AB1804 },
{ "ab1805", AB1805 },
{ "rv1805", AB1805 },
{ }
};
```
5. Here, we just have to do the stuff in the probe function:
```c
static int rs5c372_probe(struct i2c_client *client,
const struct i2c_device_id *id)
{
```
[...]
/* We pick the index corresponding to our device */
```c
int index = id->driver_data;
```
/*
* And then, we can access the per device data
* since it is stored in abx80x_caps[index]
*/
```c
}
```
## Name matching – platform device name matching
Nowadays, most platform drivers do not provide any table at all; they simply fill in the name of the driver itself in the driver's name field. But the matching works because, if you look at the platform_match function, you will see that at the end the match falls back to name matching, comparing the driver's name and the device's name. Some older drivers still use that matching mechanism. The following is name matching from sound/soc/fsl/imx-ssi.c:
```c
static struct platform_driver imx_ssi_driver = {
```
.probe = imx_ssi_probe,
.remove = imx_ssi_remove,
/* As you can see here, only the 'name' field is filled */
.driver = {
.name = "imx-ssi",
```c
},
};
module_platform_driver(imx_ssi_driver);
```
To add a device that matches this driver, you must call platform_device_register or platform_add_devices with the same name, imx-ssi, as in the board-specific file
(usually in arch/<your_arch>/mach-*/board-*.c). For our quad core i.MX6-based
UDOO, it is arch/arm/mach-imx/mach-imx6q.c.
## Summary
The kernel pseudo platform bus has no secrets from you anymore. With bus matching mechanisms, you are able to understand how, when, and why your driver has been loaded,
as well as which device it was for. We can implement any probe function, based on the matching mechanism we want. Since the main purpose of a driver is to handle a device, we are now able to populate devices in the system (the old and depreciated way). To finish in style, the next chapter will exclusively deal with the device tree, which is the new mechanism used to populate devices, along with their configurations, on the system