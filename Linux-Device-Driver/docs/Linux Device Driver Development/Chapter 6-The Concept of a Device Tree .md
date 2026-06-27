# Chapter 6 - The Concept of a Device Tree 

A Device Tree (DT) is an easy-to-read hardware description file, with a JSON-like formatting style, which is a simple tree structure where devices are represented by nodes with their properties. Properties can be either empty (just the key, to describe Boolean values), or key-value pairs where the value can contain an arbitrary byte stream. This chapter is a simple introduction to the DT. Every kernel subsystem or framework has its own DT binding. We will talk about those specific bindings when we deal with concerned topics. DTs originated from OF, which is a standard endorsed by computer companies, and whose main purpose is defining interfaces for computer firmware systems. That said, you can find more on DT specification at http://www.devicetree.org/. This chapter will cover the basics of DTs, such as:
Naming conventions, as well as aliases and labeling
Describing data types and their APIs
Managing address schemes and accessing device resources
Implementing OF match style and providing application-specific data
## Device tree mechanisms
A DT is enabled in the kernel by setting the CONFIG_OF option to Y. In order to pull the DT
API from within your driver, you must add the following headers:
```c
#include <linux/of.h>
#include <linux/of_device.h>
```
A DT supports a few data types. Let's have a look at them with a sample node description:
/* This is a comment */
```dts
// This is another comment node_label: nodename@reg{
```
string-property = "a string";
string-list = "red fish", "blue fish";
```c
one-int-property = <197>; /* One cell in this property */
int-list-property = <0xbeef 123 0xabcd4>; /*each number(cell)is a
```
*32 bit integer(uint32).
*There are 3 cells in
*this property
*/
```c
mixed-list-property = "a string", <0xadbcd45>, <35>, [0x01 0x23 0x45]
```
byte-array-property = [0x01 0x23 0x45 0x67];
boolean-property;
```c
};
```
The following are some definitions of data types used in DTs:
Text strings are represented with double quotes. You can use commas to create a list of the strings.
Cells are 32-bit unsigned integers delimited by angle brackets.
Boolean data is nothing but an empty property. The true or false value depends on the property being there or not.
## Naming convention
```c
Every node must have a name in the form <name>[@<address>], where <name> is a string that can be up to 31 characters in length, and [@<address>] is optional, depending on whether the node represents an addressable device or not. <address> should be the primary address used to access the device. An example of device naming is as follows:
dts
expander@20 {
compatible = "microchip,mcp23017";
reg = <20>;
```
[...]
```c
};
```
Here is another example:
```dts
i2c@021a0000 {
compatible = "fsl,imx6q-i2c", "fsl,imx21-i2c";
reg = <0x021a0000 0x4000>;
```
[...]
```c
};
```
On the other hand, the label is optional. Labeling a node is useful only if the node is intended to be referenced from a property of another node. You can see a label as a pointer to a node, as explained in the next section.
## Aliases, labels, and phandle
It is very important to understand how these three elements work. They are frequently used in DTs. Let's take the following DT to explain how they work:
```dts
aliases {
```
ethernet0 = &fec;
```c
gpio0 = &gpio1;
gpio1 = &gpio2;
```
mmc0 = &usdhc1;
[...]
```c
};
dts
gpio1: gpio@0209c000 {
compatible = "fsl,imx6q-gpio", "fsl,imx35-gpio";
```
[...]
```c
};
dts
node_label: nodename@reg {
```
[...];
```c
gpios = <&gpio1 7 GPIO_ACTIVE_HIGH>;
};
```
A label is nothing but a way to tag a node, to let the node be identified by a unique name.
```c
In the real world, that name is converted into a unique 32-bit value by the DT compiler. In the preceding example, gpio1 and node_label are both labels. Labels can then be used to refer to a node, since a label is unique to a node. A pointer handle (phandle) is a 32-bit value associated with a node that is used to uniquely identify that node so that the node can be referenced from a property in another node. Labels are used to have a pointer to the node. By using <&mylabel>, you point to the node whose label is mylabel.
```
The use of & is just like in the C programming language, to obtain the address of an element.
In the preceding example, &gpio1 is converted to a phandle so that it refers to the gpio1
node. The same goes for the following example:
```dts
thename@address {
c
property = <&mylabel>;
};
dts
mylabel: thename@adresss {
```
[...]
```c
}
```
In order not to walk through the whole tree to look for a node, the concept of aliases has been introduced. In DTs, the aliases node can be seen as a quick lookup table, an index of another node. You can use the find_node_by_alias() function to find a node given its alias. The aliases are not used directly in the DT source, but are instead deferenced by the
Linux kernel.
## DT compiler
DTs come in two forms: the textual form, which represents the sources, also known as DTS,
and the binary blob form, which represents the compiled DT, also known as DTB. Source files have the .dts extension. Actually, there are also .dtsi text files, which represent SoC
level definitions, whereas .dts files represent board level definitions. You can see .dtsi as header files that should be included in the .dts one, which are source files, not the reverse,
a bit like including header files (.h) in the source file (.c). On the other hand, binary files use the .dtb extension.
There is actually a third form, which is the runtime representation of a DT in
/proc/device-tree.
As its name says, the tool used to compile a device tree is called the device tree compiler
(dtc). From the root kernel source, you can compile either a standalone specific DT or all
DTs for the specific architecture.
Let's compile all DT (.dts) files for ARM SoCs:
```bash
ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make dtbs
```
This is for a standalone DT:
```bash
ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make imx6dl-sabrelite.dtb
```
In the preceding example, the name of the source file is imx6dl-sabrelite.dts.
Given a compiled device tree (.dtb) file, you can do the reverse operation and extract the source (.dts) file:
```bash
dtc -I dtb -O dts -o arch/arm/boot/dts imx6dl-sabrelite.dtb
```
>path/to/my_devicetree.dts
For the purpose of debugging, it could be useful to expose a DT to the user space. The CONFIG_PROC_DEVICETREE configuration variable will do that for you. You can then explore and walk through a DT in
/proc/device-tree.
## Representing and addressing devices
Each device is given at least one node in a DT. Some properties are common to many device types, especially devices sitting on a bus known to the kernel (SPI, I2C, platform, MDIO,
and so on). These properties are reg, #address-cells, and #size-cells. The purpose of these properties is device addressing on the bus they sit on. That said, the main addressing property is reg, which is a generic property whose meaning depends on the bus the device sits on. The # (hash) that prefixes size-cell and address-cell can be translated into length.
Each addressable device gets a reg property that is a list of tuples in the form reg =
```c
<address0 size0 [address1size1] [address2size2] ... >, where each tuple represents an address range used by the device. #size-cells indicates how many 32-bit cells are used to represent size, and may be 0 if size is not relevant. On the other hand,
```
#address-cells indicates how many 32-bit cells are used to represent the address. In other words, the address element of each tuple is interpreted according to #address-cell;
the same for the size element, which is interpreted according to #size-cell.
Actually, addressable devices inherit from the #size-cell and #address-cell of their parent, which is the node that represents the bus controller. The presence of #size-cell and #address-cell in a given device does not affect the device itself, but its children. In other words, before interpreting the reg property of a given node, you must know the parent node's #address-cells and #size-cells values. The parent node is free to define whatever addressing scheme is suitable for device sub-nodes (children).
## SPI and I2C addressing
SPI and I2C devices both belong to non-memory mapped devices because their addresses are not accessible to the CPU. Instead, the parent device's driver (which is the bus controller driver) would perform indirect access on behalf of the CPU. Each I2C/SPI device is always represented as a sub-node of the I2C/SPI bus node the device sits on. For non-memory mapped devices, the #size-cells property is 0, and the size element in addressing the tuple is empty. This means the reg property for this kind of device is always on the cell:
```dts
&i2c3 {
```
[...]
```c
status = "okay";
dts
temperature-sensor@49 {
compatible = "national,lm73";
reg = <0x49>;
c
};
dts
pcf8523: rtc@68 {
compatible = "nxp,pcf8523";
reg = <0x68>;
c
};
};
dts
&ecspi1 {
c
fsl,spi-num-chipselects = <3>;
cs-gpios = <&gpio5 17 0>, <&gpio5 17 0>, <&gpio5 17 0>;
status = "okay";
```
[...]
```dts
ad7606r8_0: ad7606r8@1 {
compatible = "ad7606-8";
reg = <1>;
c
spi-max-frequency = <1000000>;
interrupt-parent = <&gpio4>;
dts
interrupts = <30 0x0>;
c
convst-gpio = <&gpio6 18 0>;
};
};
```
If you look at an SoC-level file at arch/arm/boot/dts/imx6qdl.dtsi, you will notice that the #size-cells and #address-cells are set to 0 for the former and 1 for the latter,
in both i2c and spi nodes, which are respectively parents of the I2C and SPI devices enumerated in the preceding section. This helps us to understand their reg property, which is only one cell for the address value and none for the size value.
An I2C device's reg property is used to specify the device's address on the bus. For SPI
```c
devices, reg represents the index of the chip-select line assigned to the device among the list of chip-select lines the controller node has. For example, for the ad7606r8 ADC, the chip-select index is 1, which corresponds to <&gpio5 17 0> in cs-gpios, which is the list of chip-select lines in the controller node.
```
You may ask why I used the I2C/SPI node's phandle; the answer is because I2C/SPI devices should be declared in a board-level file (.dts), whereas I2C/SPI bus controllers are declared in an SoC-level file (.dtsi).
## Platform device addressing
This section addresses simple memory-mapped devices whose memory is accessible to the
CPU. Here, the reg property still defines the device's address, which is a list of memory regions on which you can access the device. Each region is represented with a tuple of cells,
where the first cell is the base address of the memory region, and the second tuple is the size of the region. It then has the form reg = <base0 length0 [base1 length1]
[address2 length2] ... >. Each tuple represents an address range used by the device.
In the real world, you should not interpret the reg property without knowing the value of two other properties,#size-cells and #address-cells. #size-cells tell us how large the length field is in each child reg tuple. The same for #address-cell, which tells us how many cells we must use to specify an address.
This kind of device should be declared within a node with a special value, compatible =
"simple-bus", meaning a simple memory-mapped bus with no specific handling or driver:
```dts
soc {
c
#address-cells = <1>;
#size-cells = <1>;
dts
compatible = "simple-bus";
```
aips-bus@02000000 { /* AIPS1 */
```dts
compatible = "fsl,aips-bus", "simple-bus";
c
#address-cells = <1>;
#size-cells = <1>;
dts
reg = <0x02000000 0x100000>;
```
[...];
```dts
spba-bus@02000000 {
compatible = "fsl,spba-bus", "simple-bus";
c
#address-cells = <1>;
#size-cells = <1>;
dts
reg = <0x02000000 0x40000>;
```
[...]
```dts
ecspi1: ecspi@02008000 {
c
#address-cells = <1>;
#size-cells = <0>;
dts
compatible = "fsl,imx6q-ecspi", "fsl,imx51-ecspi";
reg = <0x02008000 0x4000>;
```
[...]
```c
};
dts
i2c1: i2c@021a0000 {
c
#address-cells = <1>;
#size-cells = <0>;
dts
compatible = "fsl,imx6q-i2c", "fsl,imx21-i2c";
reg = <0x021a0000 0x4000>;
```
[...]
```c
};
};
};
```
In the preceding example, child nodes whose parent has simple-bus in the compatible property will be registered as platform devices. You can also see how I2C and SPI bus controllers change the addressing scheme of their children by setting #size-cells =
```c
<0>; because it is not relevant to them. A well-known place to look for any binding information is in the kernel device tree's documentation: Documentation/devicetree/bindings/.
```
## Handling resources
The main purpose of a driver is to handle and manage devices, and most of the time expose their functionalities to the user space. The objective here is to gather the device's configuration parameters, and especially resources (memory region, interrupt line, DMA
channel, clocks, and so on).
The following is the device node with which we will work during this section. It is the i.MX6 UART device's node, defined in arch/arm/boot/dts/imx6qdl.dtsi:
```dts
uart1: serial@02020000 {
compatible = "fsl,imx6q-uart", "fsl,imx21-uart";
reg = <0x02020000 0x4000>;
interrupts = <0 26 IRQ_TYPE_LEVEL_HIGH>;
c
clocks = <&clks IMX6QDL_CLK_UART_IPG>,
<&clks IMX6QDL_CLK_UART_SERIAL>;
```
clock-names = "ipg", "per";
```c
dmas = <&sdma 25 4 0>, <&sdma 26 4 0>;
```
dma-names = "rx", "tx";
```c
status = "disabled";
};
```
## Concept of named resources
When the driver expects a list of resources of a certain type, you have no guarantee the list is ordered in a manner the driver expects, since the guy who writes the board-level device tree is usually not the one that wrote the driver. A driver may expect, for example, its device node with two IRQ lines, one for the Tx event at index 0, the other for Rx at index 1.
What happens if the order is not respected? The driver will have unwanted behavior. To avoid such mismatches, the concept of named resources (clock, irq, dma, reg) has been introduced. It consists of defining our resource list and naming them, so that whatever their indexes are, a given name will always match the resource.
The corresponding properties to name the resources are as follows:
reg-names: This is for a list of memory regions in the reg property clock-names: This is to name clocks in the clocks property interrupt-names: This give a name to each interrupt in the interrupts property dma-names: This is for the dma property
Now, let's create a fake device node entry to explain that:
```dts
fake_device {
compatible = "packt,fake-device";
reg = <0x02020000 0x4000>, <0x4a064800 0x200>, <0x4a064c00 0x200>;
```
reg-names = "config", "ohci", "ehci";
```dts
interrupts = <0 66 IRQ_TYPE_LEVEL_HIGH>, <0 67 IRQ_TYPE_LEVEL_HIGH>;
```
interrupt-names = "ohci", "ehci";
```c
clocks = <&clks IMX6QDL_CLK_UART_IPG>, <&clks IMX6QDL_CLK_UART_SERIAL>;
```
clock-names = "ipg", "per";
```c
dmas = <&sdma 25 4 0>, <&sdma 26 4 0>;
```
dma-names = "rx", "tx";
```c
};
```
The code in the driver to extract each named resource is as follows:
```c
struct resource *res1, *res2;
```
res1 = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ohci");
res2 = platform_get_resource_byname(pdev, IORESOURCE_MEM, "config");
```c
struct dma_chan *dma_chan_rx, *dma_chan_tx;
dma_chan_rx = dma_request_slave_channel(&pdev->dev, "rx");
dma_chan_tx = dma_request_slave_channel(&pdev->dev, "tx");
```
inttxirq, rxirq;
txirq = platform_get_irq_byname(pdev, "ohci");
rxirq = platform_get_irq_byname(pdev, "ehci");
structclk *clck_per, *clk_ipg;
```c
clk_ipg = devm_clk_get(&pdev->dev, "ipg");
clk_ipg = devm_clk_get(&pdev->dev, "pre");
```
This way, you are sure to map the right name to the right resource, without needing to play with the index anymore.
## Accessing registers
Here, the driver will take ownership of the memory region and map it into the virtual address space. We will discuss this more in Chapter 11, Kernel Memory Management:
```c
struct resource *res;
void __iomem *base;
```
res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
/*
* Here one can request and map the memory region
* using request_mem_region(res->start, resource_size(res), pdev->name)
* and ioremap(iores->start, resource_size(iores)
*
* These function are discussed in chapter 11, Kernel Memory Management.
*/
base = devm_ioremap_resource(&pdev->dev, res);
```c
if (IS_ERR(base))
return PTR_ERR(base);
dts
platform_get_resource() will set the start and end fields of struct res according to the memory region present in the first (index 0) reg assignment. Please remember the last argument of platform_get_resource() represents the resource index. In the preceding sample, 0 indexes the first value of that resource type, just in case the device is assigned more than one memory region in the DT node. In our example, it's reg = <0x02020000
```
0x4000>, meaning that the allocated region starts at physical address 0x02020000 and has a size of 0x4000 bytes. platform_get_resource() will then set res.start =
0x02020000 and res.end = 0x02023fff.
## Handling interrupts
The interrupt interface is actually divided into two parts—the consumer side and the controller side. Four properties are used to describe interrupt connections in a DT:
The controller is the device that exposes IRQ lines to the consumer. On the controller side,
the device has the following properties:
interrupt-controller: An empty (Boolean) property that you should define in order to mark the device as being an interrupt controller.
#interrupt-cells: This is a property of interrupt controllers. It states how many cells are used to specify an interrupt for that interrupt controller.
The consumer is the device that generate the IRQ. Consumer binding expects the following properties:
interrupt-parent: For the device node that generates the interrupt, it is a property that contains a phandle pointer to the interrupt the controller node to which the device is attached. If omitted, the device inherits that property from its parent node.
```dts
interrupts: This is the interrupt specifier.
```
Interrupt binding and interrupt specifiers are tied to the interrupt controller device. The number of cells used to define an interrupt input depends on the interrupt controller, which is the only one deciding, by means of its #interrupt-cells property. In the case of i.MX6, the interrupt controller is a Global Interrupt Controller (GIC). Its binding is well explained in Documentation/devicetree/bindings/arm/gic.txt.
## The interrupt handler
This consists of fetching the IRQ number from a DT, and mapping it into Linux IRQ, thus registering a function callback for it. The driver code to do this is quite simple:
```c
int irq = platform_get_irq(pdev, 0);
```
ret = request_irq(irq, imx_rxint, 0, dev_name(&pdev->dev), sport);
The platform_get_irq() call will return the irq number; this number is usable by devm_request_irq() (irq is then visible in /proc/interrupts ). The second argument,
0, says that we need the first interrupt specified in the device node. If there is more than one interrupt, we can change this index according to the interrupt we need, or just use the named resource.
In our preceding example, the device node contains an interrupt specifier, which looks as follows:
```dts
interrupts = <0 66 IRQ_TYPE_LEVEL_HIGH>;
```
According to ARM GIC, the first cell informs us about the interrupt type:
0: Shared peripheral interrupt (SPI), for interrupt signals shared among cores, which can be routed by the GIC to any core
1: Private peripheral interrupt (PPI), for interrupt signals private to an individual core
The documentation can be found at http://infocenter.arm.com/help/index.jsp?topic=
/com.arm.doc.ddi0407e/CCHDBEBE.html.
The second cell holds the interrupt number. This number depends on whether the interrupt line is a PPI or SPI.
The third cell, IRQ_TYPE_LEVEL_HIGH in our case, represents sense level. All of the available sense levels are defined in include/linux/irq.h.
## Interrupt controller code
The interrupt-controller property is used to declare a device as an interrupt controller. The #interrupt-cells property defines how many cells must be used to define a single interrupt line. We will discuss this in detail in Chapter 16, Advanced IRQ
Management.
## Extract application-specific data
Application-specific data is data beyond the common properties (neither resources nor
GPIOs, regulator, and so on). Those are arbitrary properties and child nodes that can be assigned to a device. Such property names are usually prefixed with manufacture codes.
These can be any string, Boolean, or integer values, along with their API defined in drivers/of/base.c in the Linux sources. The following examples we discuss are not exhaustive. Let's now reuse the node defined earlier in this chapter:
```dts
node_label: nodename@reg{
```
string-property = ""a string"";
string-list = ""red fish"", ""blue fish"";
```c
one-int-property = <197>; /* One cell in this property */
int-list-property = <0xbeef 123 0xabcd4>;/* each number (cell) is 32
```
a * bit integer(uint32). There
* are 3 cells in this property
*/
```c
mixed-list-property = "a string", <0xadbcd45>, <35>, [0x01 0x23 0x45]
```
byte-array-property = [0x01 0x23 0x45 0x67];
```c
one-cell-property = <197>;
```
boolean-property;
```c
};
```
## Text string
The following is one string property:
string-property = "a string";
Back in the driver, you should use of_property_read_string()to read a string value. Its prototype is defined as follows:
```c
int of_property_read_string(const struct device_node *np, const char *propname, const char **out_string)
```
The following code shows how you can use it:
```c
const char *my_string = NULL;
of_property_read_string(pdev->dev.of_node, "string-property", &my_string);
```
## Cells and unsigned 32-bit integers
The following are our int properties:
```c
one-int-property = <197>;
int-list-property = <1350000 0x54dae47 1250000 1200000>;
```
You should use of_property_read_u32() to read a cell value. Its prototype is defined as follows:
```c
int of_property_read_u32(const struct device_node *np, const char
```
*propname, u32 *out_value)
Back in the driver, write this code:
```c
unsigned int number;
of_property_read_u32(pdev->dev.of_node, "one-cell-property", &number);
```
You can use of_property_read_u32_array to read a list of cells. Its prototype is as follows:
```c
int of_property_read_u32_array(const struct device_node *np, const char
```
*propname, u32 *out_values, size_t sz);
Here, sz is the number of array elements to read. Have a look at drivers/of/base.c to see how to interpret its return value:
```c
unsigned int cells_array[4];
if (of_property_read_u32_array(pdev->dev.of_node, "int-list-property",
dts
cells_array, 4)) {
c
dev_err(&pdev->dev, "list of cells not specified\n");
return -EINVAL;
}
```
## Boolean
You should use of_property_read_bool() to read the Boolean property whose name is given in the second argument of the function:
```c
bool my_bool = of_property_read_bool(pdev->dev.of_node, "booleanproperty");
dts
If(my_bool){
```
/* boolean is true */
```c
} else
```
/* Bolean is false */
```c
}
```
## Extracting and parsing sub-nodes
You are allowed to add any sub-node in your device node. Given a node representing a flash memory device, partitions can be represented as sub-nodes. For a device that handles a set of input and output GPIO, each set can be represented as a sub-node. A sample node is given as follows:
```dts
eeprom: ee24lc512@55 {
compatible = "microchip,24xx512";
reg = <0x55>;
partition1 {
```
read-only;
part-name = "private";
```c
offset = <0>;
size = <1024>;
};
dts
partition2 {
```
part-name = "data";
```c
offset = <1024>;
size = <64512>;
};
};
```
You can use for_each_child_of_node() to walk through the sub-nodes of the given node:
```c
struct device_node *np = pdev->dev.of_node;
struct device_node *sub_np;
dts
for_each_child_of_node(np, sub_np) {
```
/* sub_np will point successively to each sub-node */
[...]
```c
int size;
of_property_read_u32(client->dev.of_node,
```
"size", &size);
...
```c
}
```
## Platform drivers and DTs
Platform drivers also work with DTs. It is the recommended way to deal with platform devices nowadays, and there is no need to touch board files anymore, or even to recompile the kernel when a device's property changes. If you remember, in the previous chapter we discussed OF match style, which is a matching mechanism based on DTs. Let's see in the following section how it works.
## OF match style
OF match style is the first matching mechanism performed by the platform core in order to match devices with their drivers. It uses a device tree's compatible property to match the device entry in of_match_table, which is a field of the struct driver substructure.
Each device node has a compatible property, which is a string or a list of strings. Any platform driver that declares one of the strings listed in the compatible property will trigger a match and will see its probe function executed.
A DT match entry is described in the kernel as an instance of the struct of_device_id structure, which is defined in linux/mod_devicetable.h and looks as follows:
```dts
// we are only interested in the two last elements of the structure struct of_device_id {
```
[...]
char compatible[128];
```c
const void *data;
};
```
The following is the meaning of each element of the structure:
char compatible[128]: This is the string used to match the device node's compatible property in a DT. They must be identical before a match occurs.
```c
const void *data: This can point to any structure, which can be used as perdevice type configuration data.
```
Since the of_match_table is a pointer, you can pass an array of struct of_device_id to make your driver compatible with more than one device:
```dts
static const struct of_device_id imx_uart_dt_ids[] = {
{ .compatible = "fsl,imx6q-uart", },
{ .compatible = "fsl,imx1-uart", },
{ .compatible = "fsl,imx21-uart", },
c
{ /* sentinel */ }
};
```
Once you have filled your array of IDs, it must be passed to the of_match_table field of your platform driver, in the driver substructure:
```dts
static struct platform_driver serial_imx_driver = {
```
[...]
```dts
.driver = {
```
.name = "imx-uart",
.of_match_table = imx_uart_dt_ids,
[...]
```c
},
};
```
At this step, only your driver is aware of your of_device_id array. To get the kernel informed too (so that it can store your IDs in the device list maintained by the platform core), your array has to be registered with MODULE_DEVICE_TABLE, as described in Chapter
5, Platform Device Drivers:
```c
MODULE_DEVICE_TABLE(of, imx_uart_dt_ids);
```
That is all! Our driver is DT-compatible. Back in our DT, let's declare a device compatible with our driver:
```dts
uart1: serial@02020000 {
compatible = "fsl,imx6q-uart", "fsl,imx21-uart";
reg = <0x02020000 0x4000>;
interrupts = <0 26 IRQ_TYPE_LEVEL_HIGH>;
```
[...]
```c
};
```
Two compatible strings are provided here. If the first one does not match any driver, the core will perform the match with the second.
When a match occurs, the probe function of your driver is called, with a struct platform_device structure as the parameter, which contains a struct device dev field, in which there is a struct device_node *of_node field that corresponds to the node associated to our device, so you can use it to extract the device settings:
```c
static int serial_imx_probe(struct platform_device *pdev)
dts
{
```
[...]
```c
struct device_node *np;
```
np = pdev->dev.of_node;
```c
if (of_get_property(np, "fsl,dte-mode", NULL))
```
sport->dte_mode = 1;
[...]
```c
}
```
You can check if the DT node is set to know whether the driver has been loaded in response to an of_match, or instantiated from within the board's init file. You should then use the of_match_device function, in order to pick the struct *of_device_id entry that originated the match, which may contain the specific data you passed:
```c
static int my_probe(struct platform_device *pdev)
dts
{
c
struct device_node *np = pdev->dev.of_node;
const struct of_device_id *match;
```
match = of_match_device(imx_uart_dt_ids, &pdev->dev);
```dts
if (match) {
```
/* Devicetree, extract the data */
my_data = match->data;
```dts
} else {
```
/* Board init file */
my_data = dev_get_platdata(&pdev->dev);
```c
}
```
[...]
```c
}
```
## Dealing with non-device tree platforms
DT support is enabled in the kernel with the CONFIG_OF option. You would probably want to avoid using the DT API when its support is not enabled in the kernel. The way you can achieve that is to check whether CONFIG_OF is set or not. People used to do something like this:
#ifdef CONFIG_OF
```dts
static const struct of_device_id imx_uart_dt_ids[] = {
{ .compatible = "fsl,imx6q-uart", },
{ .compatible = "fsl,imx1-uart", },
{ .compatible = "fsl,imx21-uart", },
c
{ /* sentinel */ }
};
```
/* other device tree dependent code */
[...]
#endif
Even if the of_device_id data type is always defined when device tree support is missing, the code wrapped into #ifdef CONFIG_OF ... #endif will be omitted during the build. This is used for conditional compilation. It is not your only choice; there is also the of_match_ptr macro, which simply returns NULL when OF is disabled. Everywhere,
you'll need to pass your of_match_table as a parameter; it should be wrapped in the of_match_ptr macro, so that it returns NULL when OF is disabled. The macro is defined in include/linux/of.h:
```c
#define of_match_ptr(_ptr) (_ptr) /* When CONFIG_OF is enabled */
#define of_match_ptr(_ptr) NULL /* When it is not */
```
And we can use it as follows:
```c
static int my_probe(struct platform_device *pdev)
dts
{
c
const struct of_device_id *match;
```
match = of_match_device(of_match_ptr(imx_uart_dt_ids),
&pdev->dev);
[...]
```c
}
dts
static struct platform_driver serial_imx_driver = {
```
[...]
```dts
.driver = {
```
.name = "imx-uart",
.of_match_table = of_match_ptr(imx_uart_dt_ids),
```c
},
};
```
This eliminates having a #ifdef, returning NULL when OF is disabled.
Support multiple hardware devices with per devicespecific data
Sometimes, a driver can support different hardware, each with its specific configuration data. That data may be dedicated function tables, specific register values, or anything unique to each device. The following example describes a generic approach:
Let's first remember what struct of_device_id looks like in include/linux/mod_devicetable.h:
/*
* Struct used for matching a device
*/
```dts
struct of_device_id {
```
[...]
char compatible[128];
```c
const void *data;
};
```
The field we are interested in is const void *data, so we can use it to pass any data for each specific device.
Let's say we own three different devices, each with specific private data.
```c
of_device_id.data will contain a pointer to specific parameters. This example is inspired by drivers/tty/serial/imx.c.
```
First, we declare private structures:
/* i.MX21 type uart runs on all i.mx except i.MX1 and i.MX6q */
```dts
enum imx_uart_type {
```
IMX1_UART,
IMX21_UART,
IMX6Q_UART,
```c
};
```
/* device type dependent stuff */
```dts
struct imx_uart_data {
c
unsigned uts_reg;
enum imx_uart_type devtype;
};
```
Then, we fill an array with device-specific data:
```dts
static struct imx_uart_data imx_uart_devdata[] = {
[IMX1_UART] = {
```
.uts_reg = IMX1_UTS,
.devtype = IMX1_UART,
```c
},
dts
[IMX21_UART] = {
```
.uts_reg = IMX21_UTS,
.devtype = IMX21_UART,
```c
},
dts
[IMX6Q_UART] = {
```
.uts_reg = IMX21_UTS,
.devtype = IMX6Q_UART,
```c
},
};
```
Each compatible entry is tied with a specific array index:
```dts
static const struct of_device_id imx_uart_dt_ids[] = {
{ .compatible = "fsl,imx6q-uart", .data =
```
&imx_uart_devdata[IMX6Q_UART], },
```dts
{ .compatible = "fsl,imx1-uart", .data =
```
&imx_uart_devdata[IMX1_UART], },
```dts
{ .compatible = "fsl,imx21-uart", .data =
```
&imx_uart_devdata[IMX21_UART], },
```c
{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx_uart_dt_ids);
dts
static struct platform_driver serial_imx_driver = {
```
[...]
```dts
.driver = {
```
.name = "imx-uart",
.of_match_table = of_match_ptr(imx_uart_dt_ids),
```c
},
};
```
Now in the probe function, whatever the match entry is, it will hold a pointer to the device-specific structure:
```c
static int imx_probe_dt(struct platform_device *pdev)
dts
{
c
struct device_node *np = pdev->dev.of_node;
const struct of_device_id *of_id =
of_match_device(of_match_ptr(imx_uart_dt_ids), &pdev->dev);
if (!of_id)
```
/* no device tree device */
```c
return 1;
```
[...]
sport->devdata = of_id->data; /* Get private data back */
```c
}
```
In the preceding code, devdata is an element of a structure in the original source, and declared like const struct imx_uart_data *devdata; we could have stored any specific parameter in the array.
## Match style mixing
OF match style can be combined with any other matching mechanism. In the following example, we have a mix of DT and device ID match styles:
We fill an array for the device ID match style, each device with its data:
```dts
static const struct platform_device_id sdma_devtypes[] = {
{
```
.name = "imx51-sdma",
.driver_data = (unsigned long)&sdma_imx51,
```dts
}, {
```
.name = "imx53-sdma",
.driver_data = (unsigned long)&sdma_imx53,
```dts
}, {
```
.name = "imx6q-sdma",
.driver_data = (unsigned long)&sdma_imx6q,
```dts
}, {
```
.name = "imx7d-sdma",
.driver_data = (unsigned long)&sdma_imx7d,
```dts
}, {
```
/* sentinel */
```c
}
};
MODULE_DEVICE_TABLE(platform, sdma_devtypes);
```
We do the same for OF match style:
```dts
static const struct of_device_idsdma_dt_ids[] = {
{ .compatible = "fsl,imx6q-sdma", .data = &sdma_imx6q, },
{ .compatible = "fsl,imx53-sdma", .data = &sdma_imx53, },
{ .compatible = "fsl,imx51-sdma", .data = &sdma_imx51, },
{ .compatible = "fsl,imx7d-sdma", .data = &sdma_imx7d, },
c
{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, sdma_dt_ids);
```
The probe function will look as follows:
```c
static int sdma_probe(structplatform_device *pdev)
dts
{
```
conststructof_device_id *of_id =
```c
of_match_device(of_match_ptr(sdma_dt_ids), &pdev->dev);
```
structdevice_node *np = pdev->dev.of_node;
/* If devicetree, */
```c
if (of_id)
```
drvdata = of_id->data;
/* else, hard-coded */
```c
else if (pdev->id_entry)
```
drvdata = (void *)pdev->id_entry->driver_data;
```dts
if (!drvdata) {
c
dev_err(&pdev->dev, "unable to find driver data\n");
return -EINVAL;
}
```
[...]
```c
}
```
Then, we declare our platform driver; feed in all arrays defined in the preceding sections:
```dts
static struct platform_drivers dma_driver = {
.driver = {
```
.name = "imx-sdma",
.of_match_table = of_match_ptr(sdma_dt_ids),
```c
},
```
.id_table = sdma_devtypes,
.remove = sdma_remove,
.probe = sdma_probe,
```c
};
module_platform_driver(sdma_driver);
```
## Platform resources and DTs
Platform devices can work with a DT-enabled system without any extra modification. This is what we demonstrated in the Handling resources section. By using the platform_xxx family function, the core also walks-through a DT (with the of_xxx family function) to find the requested resource. The reverse is not true, since the of_xxx family function is only reserved for the DT. All resource data will be available to the driver in the usual way. The driver now knows whether this device is initialized with hardcoded parameters in the board file or not. Let's take an example with a uart device node:
```dts
uart1: serial@02020000 {
compatible = "fsl,imx6q-uart", "fsl,imx21-uart";
reg = <0x02020000 0x4000>;
interrupts = <0 26 IRQ_TYPE_LEVEL_HIGH>;
c
dmas = <&sdma 25 4 0>, <&sdma 26 4 0>;
```
dma-names = "rx", "tx";
```c
};
```
The following excerpt describes the probe function of its driver. In the probe, the function platform_get_resource() can be used to extract any property that is a resource
(memory region, dma, irq), or a specific function, such as platform_get_irq(), which extracts the irq provided by the interrupts property in the DT:
```c
static int my_probe(struct platform_device *pdev)
dts
{
c
struct iio_dev *indio_dev;
struct resource *mem, *dma_res;
struct xadc *xadc;
int irq, ret, dmareq;
```
/* irq */
```c
irq = platform_get_irq(pdev, 0);
if (irq<= 0)
return -ENXIO;
```
[...]
/* memory region */
mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
xadc->base = devm_ioremap_resource(&pdev->dev, mem);
/*
* We could have used
* devm_ioremap(&pdev->dev, mem->start, resource_size(mem));
* too.
*/
```c
if (IS_ERR(xadc->base))
return PTR_ERR(xadc->base);
```
[...]
/* second dma channel */
```c
dma_res = platform_get_resource(pdev, IORESOURCE_DMA, 1);
```
dmareq = dma_res->start;
[...]
```c
}
```
To sum up, for properties such as dma, irq, and mem, you have nothing to do in the platform driver to match dtb. If you remember, this data is of the same type as the data you can pass as a platform resource. To understand why, we just have to look inside these functions; we will see how each of them internally deals with DT functions. The following is an example of the platform_get_irq function:
```c
int platform_get_irq(struct platform_device *dev, unsigned int num)
dts
{
```
[...]
```c
struct resource *r;
dts
if (IS_ENABLED(CONFIG_OF_IRQ) &&dev->dev.of_node) {
c
int ret;
```
ret = of_irq_get(dev->dev.of_node, num);
```c
if (ret > 0 || ret == -EPROBE_DEFER)
return ret;
}
```
r = platform_get_resource(dev, IORESOURCE_IRQ, num);
```dts
if (r && r->flags & IORESOURCE_BITS) {
c
struct irq_data *irqd;
irqd = irq_get_irq_data(r->start);
if (!irqd)
return -ENXIO;
irqd_set_trigger_type(irqd, r->flags & IORESOURCE_BITS);
}
return r ? r->start : -ENXIO;
}
```
You may wonder how the platform_xxx functions extract resources from a DT. This should have been the of_xxx function family. You are right, but during the system boot,
the kernel calls of_platform_device_create_pdata() on each device node, which will result in creating a platform device with the associated resource, on which you can call the platform_xxx family function. Its prototype is as follows:
```c
static struct platform_device *of_platform_device_create_pdata(
struct device_node *np, const char *bus_id,
void *platform_data, struct device *parent)
```
## Platform data versus DTs
If your driver expects platform data, you should check the dev.platform_data pointer. A
non-null value means your driver has been instantiated the old way in the board configuration file, and DTs do not enter into it. For drivers instantiated from a DT,
dev.platform_data will be NULL, and your platform device will be given a pointer on the
DT entry (node) that corresponds to your device in the dev.of_node pointer, from which you can extract the resource and use the OF API to parse and extract application data.
There's also a hybrid method that you can use to associate platform data declared in the C files to DT nodes, but that's for special cases only: for
DMA, IRQ, and memory. This method is only used when the driver expects only resources, and no application-specific data.
You can transform a legacy declaration of an I2C controller into DT-compatible nodes as follows:
```c
#define SIRFSOC_I2C0MOD_PA_BASE 0xcc0e0000
#define SIRFSOC_I2C0MOD_SIZE 0x10000
#define IRQ_I2C0
dts
static struct resource sirfsoc_i2c0_resource[] = {
{
```
.start = SIRFSOC_I2C0MOD_PA_BASE,
.end = SIRFSOC_I2C0MOD_PA_BASE + SIRFSOC_I2C0MOD_SIZE - 1,
.flags = IORESOURCE_MEM,
```dts
},{
```
.start = IRQ_I2C0,
.end = IRQ_I2C0,
.flags = IORESOURCE_IRQ,
```c
},
};
```
And here is the DT node:
```dts
i2c0: i2c@cc0e0000 {
compatible = "sirf,marco-i2c";
reg = <0xcc0e0000 0x10000>;
c
interrupt-parent = <&phandle_to_interrupt_controller_node>
dts
interrupts = <0 24 0>;
c
#address-cells = <1>;
#size-cells = <0>;
status = "disabled";
};
```
## Summary
The time to switch from hardcoded device configuration to DTs has come. This chapter gave you all you need to handle DTs. Now you have the necessary skills to customize or add whatever nodes and properties you want to DT, and extract them from within your driver. In the next chapter, we will talk about the I2C driver, and use the DT API to enumerate and configure our I2C devices