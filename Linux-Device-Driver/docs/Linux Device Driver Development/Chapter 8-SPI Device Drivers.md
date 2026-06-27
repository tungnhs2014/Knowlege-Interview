# Chapter 8 - SPI Device Drivers 

A Serial Peripheral Interface (SPI) is (at least) a four-wire bus: Master Input Slave Output
(MISO); Master Output Slave Input (MOSI); Serial Clock (SCK); and Chip Select (CS),
which is used to connect a serial flash, AD/DA converter. The master always generates the clock. Its speed can reach up to 80 MHz, even if there is no real speed limitation (much faster than I2C as well). The same goes for the CS line, which is always managed by the master.
Each of these signal names has a synonym:
Whenever you see SIMO, SDI, DI, or SDA, they refer to MOSI.
SOMI, SDO, DO, and SDA will refer to MISO.
SCK, CLK, and SCL will refer to SCK.
S̅ S̅ is the slave select line, also called the CS. CSx can be used (where x is an index, CS0, CS1); EN and ENB can be used too, meaning enable. The CS is usually an active low signal:
```c
SPI topology (image from Wikipedia)
```
This chapter will walk through SPI driver concepts such as:
SPI bus description
Driver architecture and data structure descriptions
Sending and receiving data in both half and full duplex
```c
Declaring SPI devices from the device tree (DT)
```
Accessing SPI devices from the user space, in both half and full duplex
## The driver architecture
```c
The required header for SPI stuff in the Linux kernel is <linux/spi/spi.h>. Before talking about the driver structure, let's see how SPI devices are defined in the kernel. An
```
SPI device is represented in the kernel as an instance of spi_device. The instance of the driver that manages them is the struct spi_driver structure.
## The device structure
The struct spi_device structure represents an SPI device, and is defined in include/linux/spi/spi.h:
```dts
struct spi_device {
c
struct devicedev;
struct spi_master*master;
u32 max_speed_hz;
u8 chip_select;
u8 bits_per_word;
u16 mode;
int irq;
```
[...]
```c
int cs_gpio; /* chip select gpio */
};
```
Some fields that are not meaningful for us have been removed. The following are the meanings of the elements in the structure:
master: This represents the SPI controller (bus) on which the device is connected.
max_speed_hz: This is the maximum clock rate to be used with this chip (on the current board); this parameter can be changed from within the driver. You can override that parameter using spi_transfer.speed_hz for each transfer. We will discuss SPI transfers later.
chip_select: This lets you enable the chip you need to talk to, distinguishing chips handled by the master. chip_select is set to low by default. This behavior can be changed in mode by adding the SPI_CS_HIGH flag.
mode: This defines how data should be clocked. The device driver may change this. The data clocking is most significant bit (MSB) first, by default for each word in a transfer. This behavior can be overridden by specifying
SPI_LSB_FIRST.
irq: This represents the interrupt number (registered as a device resource in your board init file or through the DT) you should pass to request_irq() to receive interrupts from this device.
A word about SPI modes; they are built using two characteristics:
CPOL: This is the initial clock polarity:
0: Initial clock state low, and the first edge is rising
1: Initial clock state high, and the first state is falling
CPHA: This is the clock phase, choosing at which edge the data will be sampled:
0: Data latched at falling edge (high to low transition), whereas output changes at rising edge
1: Data latched at rising edge (low to high transition), and output at falling edge
This allows for four SPI modes, which are defined in the kernel according to the following macro in include/linux/spi/spi.h:
```c
#define SPI_CPHA 0x01
#define SPI_CPOL 0x02
```
You can then produce the following array to summarize things:
Mode CPOL CPHA Kernel macro
0 0 0 #define SPI_MODE_0 (0|0)
1 0 1 #define SPI_MODE_1 (0|SPI_CPHA)
2 1 0 #define SPI_MODE_2 (SPI_CPOL|0)
3 1 1 #define SPI_MODE_3 (SPI_CPOL|SPI_CPHA)
The following is a representation of each SPI mode, as defined in the preceding array. Only the MOSI line is represented, but the principle is the same for MISO:
Commonly used modes are SPI_MODE_0 and SPI_MODE_3.
## spi_driver structure struct spi_driver represents the driver you develop to manage your SPI device. Its structure is as follows:
```dts
struct spi_driver {
c
const struct spi_device_id *id_table;
int (*probe)(struct spi_device *spi);
int (*remove)(struct spi_device *spi);
void (*shutdown)(struct spi_device *spi);
struct device_driver driver;
};
```
## The probe() function
Its prototype is as follows:
```c
static int probe(struct spi_device *spi)
```
You may refer to Chapter 7, I2C Client Drivers, in order to see what is to be done in a probe function. The same steps apply here. Therefore, unlike an I2C driver, which has no capability to change the controller bus parameters (CS state, bit per word, clock) at runtime,
an SPI driver can. You can set up the bus according to your device properties.
A typical SPI probe function would look like the following:
```c
static int my_probe(struct spi_device *spi)
dts
{
```
[...] /* declare your variable/structures */
/* bits_per_word cannot be configured in platform data */
spi->mode = SPI_MODE_0; /* SPI mode */
spi->max_speed_hz = 20000000; /* Max clock for the device */
spi->bits_per_word = 16; /* device bit per word */
ret = spi_setup(spi);
ret = spi_setup(spi);
```c
if (ret < 0)
return ret;
```
[...] /* Make some init */
[...] /* Register with appropriate framework */
```c
return ret;
}
struct spi_device* is an input parameter given to the probe function by the kernel. It represents the device you are probing. From within your probe function, you can get the spi_device_id that triggered the match using spi_get_device_id (in the case of id_table match) and extract the driver data:
const struct spi_device_id *id = spi_get_device_id(spi);
```
my_private_data = array_chip_info[id->driver_data];
## Per-device data
In the probe function, it is a common task to track private (per-device) data to be used during the module lifetime. This has been discussed in Chapter 7, I2C Client Drivers.
The following are the prototypes of functions you use for setting/getting per-device data:
/* set the data */
```c
void spi_set_drvdata(struct *spi_device, void *data);
```
/* Get the data back */
```c
void *spi_get_drvdata(const struct *spi_device);
```
Here is an example:
```dts
struct mc33880 {
c
struct mutex lock;
u8 bar;
struct foo chip;
struct spi_device *spi;
};
static int mc33880_probe(struct spi_device *spi)
dts
{
c
struct mc33880 *mc;
```
[...] /* Device set up */
mc = devm_kzalloc(&spi->dev, sizeof(struct mc33880),
GFP_KERNEL);
```c
if (!mc)
return -ENOMEM;
mutex_init(&mc->lock);
spi_set_drvdata(spi, mc);
```
mc->spi = spi;
mc->chip.label = DRIVER_NAME,
mc->chip.set = mc33880_set;
/* Register with appropriate framework */
[...]
```c
}
```
## The remove() function
The remove function must release every resource grabbed in the probe function. Its structure is as follows:
```c
static int my_remove(struct spi_device *spi);
```
A typical remove function may look like the following:
```c
static int mc33880_remove(struct spi_device *spi)
dts
{
c
struct mc33880 *mc;
```
mc = spi_get_drvdata(spi); /* Get our data back */
```c
if (!mc)
return -ENODEV;
```
/*
* unregister from frameworks with which we registered in the
* probe function
*/
[...]
```c
mutex_destroy(&mc->lock);
return 0;
}
```
## Driver initialization and registration
For devices sitting on a bus, whether it is a physical one or the pseudo platform bus, most of the time, everything is done in the probe function. The init and exit functions are just used to register/unregister the driver with the bus core:
```c
static int __init foo_init(void)
dts
{
```
[...] /*My init code */
```c
return spi_register_driver(&foo_driver);
}
module_init(foo_init);
static void __exit foo_cleanup(void)
dts
{
```
[...] /* My clean up code */
```c
spi_unregister_driver(&foo_driver);
}
module_exit(foo_cleanup);
```
If you do not do anything else but register/unregister the driver, the kernel offers a macro:
```c
module_spi_driver(foo_driver);
```
This will internally call spi_register_driver and spi_unregister_driver. It is exactly the same thing as what we have seen in the previous chapter.
## Driver and device provisioning
As we need i2c_device_id for I2C devices, we must use spi_device_id for SPI devices,
in order to provide a device_id array to match our devices. It is defined in include/linux/mod_devicetable.h:
```dts
struct spi_device_id {
```
char name[SPI_NAME_SIZE];
kernel_ulong_t driver_data; /* Data private to the driver */
```c
};
```
We need to embed our array into a struct spi_device_id, in order to inform the SPI
core about the device ID we need to manage in the driver, and call the MODULE_DEVICE_TABLE macro on the driver structure. Of course, the first parameter of the macro is the name of the bus on which the device sits. In our case, it is SPI:
```c
#define ID_FOR_FOO_DEVICE 0
#define ID_FOR_BAR_DEVICE 1
dts
static struct spi_device_id foo_idtable[] = {
c
{ "foo", ID_FOR_FOO_DEVICE },
{ "bar", ID_FOR_BAR_DEVICE },
{ }
};
MODULE_DEVICE_TABLE(spi, foo_idtable);
dts
static struct spi_driver foo_driver = {
.driver = {
```
.name = "KBUILD_MODULE",
```c
},
```
.id_table = foo_idtable,
.probe = foo_probe,
.remove = foo_remove,
```c
};
module_spi_driver(foo_driver);
```
Instantiating SPI devices in board configuration file –
old and deprecated way
Devices should be instantiated in board files only if the system does not support the DT.
Since the DT has come along, this method of instantiating is deprecated. Therefore, let's just remember that the board file resides in the arch/ directory. The structure used to represent an SPI device is struct spi_board_info, not the struct spi_device we used in the driver. It is only when you have filled and registered the struct spi_board_info using the spi_register_board_info function that the kernel will build a struct spi_device (which will be passed to your driver and register with the SPI core).
Feel free to look at the struct spi_board_info field in include/linux/spi/spi.h.
The definition of spi_register_board_info can be found in drivers/spi/spi.c.
Now, let's have a look at SPI device registration in the board file:
/**
* Our platform data
*/
```dts
struct my_platform_data {
c
int foo;
bool bar;
};
dts
static struct my_platform_data mpfd = {
```
.foo = 15,
.bar = true,
```c
};
dts
static struct spi_board_info my_board_spi_board_info[] __initdata = {
{
```
/* the modalias must be same as spi device driver name */
.modalias = "ad7887", /* Name of spi_driver for this device */
.max_speed_hz = 1000000, /* max spi clock (SCK) speed in HZ */
.bus_num = 0, /* Framework bus number */
.irq = GPIO_IRQ(40),
.chip_select = 3, /* Framework chip select */
.platform_data = &mpfd,
.mode = SPI_MODE_3,
```dts
},{
```
.modalias = "spidev",
.chip_select = 0,
.max_speed_hz = 1 * 1000 * 1000,
.bus_num = 1,
.mode = SPI_MODE_3,
```c
},
};
static int __init board_init(void)
dts
{
```
[...]
```c
spi_register_board_info(my_board_spi_board_info,
```
ARRAY_SIZE(my_board_spi_board_info));
[...]
```c
return 0;
}
```
[...]
## SPI and device tree
Like I2C devices, SPI devices belong to the non-memory mapped devices family in the DT,
but are addressable too. Here, the address means the CS index among the list of CS
(starting from 0) given to the controller (the master). As an example, we may have three different SPI devices sitting on the SPI bus, each with its CS line. The master will be given general purpose input/output (GPIO) set, each representing the CS to activate a device. If Device X uses the second GPIO line as the CS, we must set its address to 1 (as we always start from 0) in the reg property.
The following is a real DT listing for SPI devices:
```dts
ecspi1 {
c
fsl,spi-num-chipselects = <3>;
cs-gpios = <&gpio5 17 0>, <&gpio5 17 0>, <&gpio5 17 0>;
pinctrl-0 = <&pinctrl_ecspi1 &pinctrl_ecspi1_cs>;
#address-cells = <1>;
#size-cells = <0>;
dts
compatible = "fsl,imx6q-ecspi", "fsl,imx51-ecspi";
reg = <0x02008000 0x4000>;
```
status = "okay";
```dts
ad7606r8_0: ad7606r8@0 {
compatible = "ad7606-8";
reg = <0>;
c
spi-max-frequency = <1000000>;
interrupt-parent = <&gpio4>;
interrupts = <30 0x0>;
};
dts
label: fake_spi_device@1 {
compatible = "packtpub,foobar-device";
reg = <1>;
```
a-string-param = "stringvalue";
spi-cs-high;
```c
};
dts
mcp2515can: can@2 {
compatible = "microchip,mcp2515";
reg = <2>;
c
spi-max-frequency = <1000000>;
clocks = <&clk8m>;
interrupt-parent = <&gpio4>;
interrupts = <29 IRQ_TYPE_LEVEL_LOW>;
};
};
```
There is a new property introduced in SPI device nodes: spi-max-frequency. It represents the maximum SPI clocking speed of the device in Hz. Whenever you access the device, the bus controller driver will ensure the clock does not cross this limit. Other properties commonly used are:
spi-cpol: This is a Boolean (empty property) indicating the device requires inverse clock polarity mode. It corresponds to CPOL.
spi-cpha: This is an empty property indicating the device requires shifted clock phase mode. It corresponds to CPHA.
spi-cs-high: By default, SPI devices require CS low to be active. This is a
Boolean property indicating the device requires CS high to be active.
For a complete list of SPI binding elements, you can refer to
Documentation/devicetree/bindings/spi/spi-bus.txt in the kernel sources.
## Instantiate SPI devices in device tree – the new way
By filling our device node in the DT properly, the kernel will build a struct spi_device for us, and give it as a parameter to our SPI core functions. The following is just an excerpt from the SPI DT listing defined previously:
```dts
&ecspi1 {
```
status = "okay";
```dts
label: fake_spi_device@1 {
compatible = "packtpub,foobar-device";
reg = <1>;
```
a-string-param = "stringvalue";
spi-cs-high;
```c
};
};
```
## Define and register SPI driver
Again the principle is the same as that for I2C drivers. We need to define a struct of_device_id to match devices in the DT, and call the MODULE_DEVICE_TABLE macro to register with the OF core:
```dts
static const struct of_device_id foobar_of_match[] = {
{ .compatible = "packtpub,foobar-device" },
{ .compatible = "packtpub,barfoo-device" },
c
{}
};
MODULE_DEVICE_TABLE(of, foobar_of_match);
```
Then, we define our spi_driver as the following:
```dts
static struct spi_driver foo_driver = {
.driver = {
```
.name = "foo",
/* The following line adds Device tree */
.of_match_table = of_match_ptr(foobar_of_match),
```c
},
```
.probe = my_spi_probe,
.id_table = foo_id,
```c
};
```
You can then improve the probe function this way:
```c
static int my_spi_probe(struct spi_device *spi)
dts
{
c
const struct of_device_id *match;
```
match = of_match_device(of_match_ptr(foobar_of_match), &spi->dev);
```dts
if (match) {
```
/* Device tree code goes here */
```dts
} else {
```
/*
* Platform data code comes here.
* One can use
* pdata = dev_get_platdata(&spi->dev);
*
* or *id*, which is a pointer on the *spi_device_id* entry that originated
* the match, in order to use *id->driver_data* to extract the device
* specific data, as described in Chapter 5, Platform Device
Drivers.
*/
```c
}
```
[...]
```c
}
```
## Accessing and talking to the client
The SPI I/O model consists of a set of queued messages. We submit one or more struct spi_message structures, which are processed and completed synchronously or asynchronously. A single message consists of one or more struct spi_transfer objects,
each of which represents a full duplex SPI transfer. These are two main structures to exchange data between the driver and the device. They are both defined in include/linux/spi/spi.h:
SPI message structure struct spi_transfer represents a full duplex SPI transfer:
```dts
struct spi_transfer {
c
const void *tx_buf;
void *rx_buf;
unsigned len;
```
dma_addr_t tx_dma;
dma_addr_t rx_dma;
```c
unsigned cs_change:1;
unsigned tx_nbits:3;
unsigned rx_nbits:3;
#define SPI_NBITS_SINGLE 0x01 /* 1 bit transfer */
#define SPI_NBITS_DUAL 0x02 /* 2 bits transfer */
#define SPI_NBITS_QUAD 0x04 /* 4 bits transfer */
u8 bits_per_word;
u16 delay_usecs;
u32 speed_hz;
};
```
The following is the meaning of the structure elements:
tx_buf: This buffer contains the data to be written. It should be NULL or left as it is in the case of a read-only transaction. It should be dma-safe in cases where you need to perform SPI transactions through Direct Memory Access (DMA).
rx_buf: This is a buffer for data to be read (with the same properties as tx_buf),
or NULL in a write-only transaction.
tx_dma: This is the DMA address of tx_buf, in case spi_message.is_dma_mapped is set to 1. DMA is discussed in Chapter 12,
DMA – Direct Memory Access.
rx_dma: This is the same as tx_dma, but for rx_buf.
len: This represents the size of the rx and tx buffers in bytes, meaning they must have the same size if both are used.
speed_hz: This overrides the default speed, specified in spi_device.max_speed_hz, but only for the current transfer. If 0, the default value (provided in struct spi_device structure) is used.
bits_per_word: Data transfer involves one or more words. A word is a unit of data, whose size in bits may vary according to the need. Here, bits_per_word represents the size in bits of a word for this SPI transfer. This overrides the default value provided in spi_device.bits_per_word. If 0, the default (from spi_device) is used.
cs_change: This determines the state of the chip_select line after this transfer completes.
delay_usecs: This represents the delay (in microseconds) after this transfer before (optionally) changing the chip_select status, then starting the next transfer or completing this spi_message.
On the other side, the struct spi_message is used atomically to wrap one or more SPI
transfers. The SPI bus used will be hogged by the driver until every transfer that constitutes the message is completed. The SPI message structure is defined in include/linux/spi/spi.h too:
```dts
struct spi_message {
c
struct list_head transfers;
struct spi_device *spi;
unsigned is_dma_mapped:1;
```
/* completion is reported through a callback */
```c
void (*complete)(void *context);
void *context;
unsigned frame_length;
unsigned actual_length;
int status;
};
```
The preceding code is explained as follows:
transfers: This is the list of transfers that constitutes the message. We will see later how to add a transfer to this list.
is_dma_mapped: This informs the controller whether to use DMA to perform the transaction. Your code is then responsible for providing DMA and CPU virtual addresses for each transfer buffer.
complete: This is a callback called when the transaction is done, and context is the parameter to be given to the callback.
frame_length: This will be set automatically with the total number of bytes in the message.
actual_length: This is the number of bytes transferred in all successful segments.
status: This reports the transfers status. It is 0 on success, otherwise -errno.
```c
spi_transfer elements in a message are processed in FIFO order. Until the message is completed, you have to make sure not to use the transfer buffer, in order to avoid data corruption. You make a completion call to make sure you can.
```
Before a message can be submitted to the bus, it has to be initialized with void spi_message_init(struct spi_message *message), which will zero each element in the structure and initialize the transfers list. For each transfer to be added to the message, you should call void spi_message_add_tail(struct spi_transfer *t,
```c
struct spi_message *m) on that transfer, which will result in enqueuing the transfer into the transfers list. Once done, you have two choices to start the transaction:
```
Synchronously, using the int spi_sync(struct spi_device *spi, struct spi_message *message) function, which may sleep and which is not to be used in an interrupt context. Completion of the callback is not necessary here.
This function is a wrapper around the second function (spi_async()).
Asynchronously, using the spi_async() function, which can be used in an atomic context too, and whose prototype is int spi_async(struct spi_device *spi, struct spi_message *message). It is good practice to provide a callback here, since it will be executed upon message completion.
The following is what a single transfer SPI message transaction may look like:
```dts
char tx_buf[] = {
```
0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0x40, 0x00, 0x00, 0x00,
0x00, 0x95, 0xEF, 0xBA, 0xAD,
0xF0, 0x0D,
```c
};
char rx_buf[10] = {0,};
int ret;
struct spi_message single_msg;
struct spi_transfer single_xfer;
```
single_xfer.tx_buf = tx_buf;
single_xfer.rx_buf = rx_buf;
single_xfer.len = sizeof(tx_buff);
single_xfer.bits_per_word = 8;
```c
spi_message_init(&msg);
spi_message_add_tail(&xfer, &msg);
```
ret = spi_sync(spi, &msg);
Now, let's write a multi-transfer message transaction:
```dts
struct {
```
char buffer[10];
char cmd[2]
```c
int foo;
} data;
struct data my_data[3];
```
initialize_data(my_data, ARRAY_SIZE(my_data));
```c
struct spi_transfer multi_xfer[3];
struct spi_message single_msg;
int ret;
```
multi_xfer[0].rx_buf = data[0].buffer;
multi_xfer[0].len = 5;
multi_xfer[0].cs_change = 1;
/* command A */
multi_xfer[1].tx_buf = data[1].cmd;
multi_xfer[1].len = 2;
multi_xfer[1].cs_change = 1;
/* command B */
multi_xfer[2].rx_buf = data[2].buffer;
multi_xfer[2].len = 10;
```c
spi_message_init(single_msg);
spi_message_add_tail(&multi_xfer[0], &single_msg);
spi_message_add_tail(&multi_xfer[1], &single_msg);
spi_message_add_tail(&multi_xfer[2], &single_msg);
```
ret = spi_sync(spi, &single_msg);
There are other helper functions, all built around spi_sync(). Some of them are:
```c
int spi_read(struct spi_device *spi, void *buf, size_t len)
int spi_write(struct spi_device *spi, const void *buf, size_t len)
int spi_write_then_read(struct spi_device *spi,
const void *txbuf, unsigned n_tx,
void *rxbuf, unsigned n_rx)
```
Please have a look at include/linux/spi/spi.h to see the complete list. These wrappers should be used with small amounts of data.
## Putting it all together
The steps needed to write SPI client drivers are as follows:
1. Declare device IDs supported by the driver. You can do that using spi_device_id. If the DT is supported, use of_device_id too. You can make an exclusive use of the DT.
2. Call MODULE_DEVICE_TABLE(spi, my_id_table); in order to expose the driver along with its SPI device table IDs to userspace. If the DT is supported,
you must call MODULE_DEVICE_TABLE(of, your_of_match_table); in order to expose OF (device tree) related module aliases to user space. The preceding calls to MODULE_DEVICE_TABLE will export informations that will be collected by depmod to update the modules.alias file, thus allowing the driver to be automatically found and loaded if it is build as module.
3. Write probe and remove functions according to their respective prototypes. The probe function must identify your device, configure it, define per-device
(private) data, configure the bus if needed (SPI mode and so on) using the spi_setup function, and register with the appropriate kernel framework. In the remove function, simply undo everything done in the probe function.
4. Declare and fill a struct spi_driver structure, set the id_table field with the array of IDs you have created. Set the .probe and .remove fields with the name of the corresponding functions you have written. In the .driver substructure, set the .owner field to THIS_MODULE, set the driver name, and finally set the .of_match_table field with the array of of_device_id, if the
DT is supported.
5. Call the module_spi_driver function with your spi_driver structure you just filled in before module_spi_driver(serial_eeprom_spi_driver); in order to register your driver with the kernel.
## SPI user mode driver
There are two ways of using the user mode SPI device driver. To be able to do that, you need to enable your device with the spidev driver. An example would be as follows:
```dts
spidev@0x00 {
compatible = "spidev";
c
spi-max-frequency = <800000>; /* It depends on your device */
dts
reg = <0>; /* correspond to chip select 0 */
c
};
```
You can call either the read/write functions or an ioctl(). By calling read/write, you can only read or write at a time. If you need full-duplex read and write, you have to use the
Input Output Control (ioctl) commands. Examples for both are provided. This is the read/write example. You can compile it either with the cross-compiler of the platform or with the native compiler on the board:
```c
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
int main(int argc, char **argv)
dts
{
c
int i,fd;
char wr_buf[]={0xff,0x00,0x1f,0x0f};
```
char rd_buf[10];
```dts
if (argc<2) {
c
printf("Usage:\n%s [device]\n", argv[0]);
exit(1);
}
```
fd = open(argv[1], O_RDWR);
```dts
if (fd<=0) {
c
printf("Failed to open SPI device %s\n",argv[1]);
exit(1);
}
if (write(fd, wr_buf, sizeof(wr_buf)) != sizeof(wr_buf))
perror("Write Error");
if (read(fd, rd_buf, sizeof(rd_buf)) != sizeof(rd_buf))
perror("Read Error");
```
else for (i = 0; i < sizeof(rd_buf); i++)
```c
printf("0x%02X ", rd_buf[i]);
close(fd);
return 0;
}
```
## With IOCTL
The advantage of using IOCTL is that you can work in full duplex. The best example you can find is documentation/spi/spidev_test.c in the kernel source tree, of course.
The preceding example using read/write did not change any SPI configuration. However,
the kernel exposes to the user space a set of IOCTL commands, which you can use in order to set up the bus according to your needs, just like what is done in the DT. The following example shows how you can change the bus settings:
```c
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
static int pabort(const char *s)
dts
{
c
perror(s);
return -1;
}
static int spi_device_setup(int fd)
dts
{
c
int mode, speed, a, b, i;
int bits = 8;
```
/*
* spi mode: mode 0
*/
mode = SPI_MODE_0;
a = ioctl(fd, SPI_IOC_WR_MODE, &mode); /* write mode */
b = ioctl(fd, SPI_IOC_RD_MODE, &mode); /* read mode */
```dts
if ((a < 0) || (b < 0)) {
c
return pabort("can't set spi mode");
}
```
/*
* Clock max speed in Hz
*/
speed = 8000000; /* 8 MHz */
a = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed); /* Write speed */
b = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed); /* Read speed */
```dts
if ((a < 0) || (b < 0)) {
c
return pabort("fail to set max speed hz");
}
```
/*
* setting SPI to MSB first.
* Here, 0 means "not to use LSB first".
* In order to use LSB first, argument should be > 0
*/
i = 0;
a = ioctl(dev, SPI_IOC_WR_LSB_FIRST, &i);
b = ioctl(dev, SPI_IOC_RD_LSB_FIRST, &i);
```dts
if ((a < 0) || (b < 0)) {
c
pabort("Fail to set MSB first\n");
}
```
/*
* setting SPI to 8 bits per word
*/
bits = 8;
a = ioctl(dev, SPI_IOC_WR_BITS_PER_WORD, &bits);
b = ioctl(dev, SPI_IOC_RD_BITS_PER_WORD, &bits);
```dts
if ((a < 0) || (b < 0)) {
c
pabort("Fail to set bits per word\n");
}
return 0;
}
```
You can have a look at Documentation/spi/spidev for more information on spidev ioctl commands. When it comes to sending data over the bus, you can use a SPI_IOC_MESSAGE(N) request, which offers full-duplex access, and composite operations without chip select deactivation, thus offering multi-transfer support. It is the equivalent of the kernel's spi_sync(). Here, a transfer is represented as an instance of struct spi_ioc_transfer, which is the equivalent of the kernel's struct spi_transfer, and whose definition can be found in include/uapi/linux/spi/spidev.h. The following is an example of its use:
```c
static void do_transfer(int fd)
dts
{
c
int ret;
char txbuf[] = {0x0B, 0x02, 0xB5};
char rxbuf[3] = {0, };
```
char cmd_buff = 0x9f;
```dts
struct spi_ioc_transfer tr[2] = {
0 = {
```
.tx_buf = (unsigned long)&cmd_buff,
.len = 1,
.cs_change = 1; /* We need CS to change */
.delay_usecs = 50, /* wait after this transfer */
.bits_per_word = 8,
```c
},
dts
[1] = {
```
.tx_buf = (unsigned long)tx,
.rx_buf = (unsigned long)rx,
.len = txbuf(tx),
.bits_per_word = 8,
```c
},
};
```
ret = ioctl(fd, SPI_IOC_MESSAGE(2), &tr);
```dts
if (ret == 1){
c
perror("can't send spi message");
exit(1);
}
for (ret = 0; ret < sizeof(tx); ret++)
printf("%.2X ", rx[ret]);
printf("\n");
}
int main(int argc, char **argv)
dts
{
```
char *device = "/dev/spidev0.0";
```c
int fd;
int error;
```
fd = open(device, O_RDWR);
```c
if (fd < 0)
return pabort("Can't open device ");
```
error = spi_device_setup(fd);
```c
if (error)
exit (1);
do_transfer(fd);
close(fd);
return 0;
}
```
## Summary
We just dealt with SPI drivers and can now take advantage of this faster serial (and fullduplex) bus. We walked through data transfer over SPI, which was the most important section. You may need more abstraction in order not to bother with SPI or I2C APIs. This is where the next chapter comes in, dealing with the regmap API, which offers a higher and unified level of abstraction, so that SPI (or I2C) commands will become transparent to you