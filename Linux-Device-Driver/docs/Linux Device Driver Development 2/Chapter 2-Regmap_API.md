```bash
# Chapter 2 - Leveraging the Regmap API and Simplifying the Code
```
This chapter introduces the Linux kernel register mapping abstraction layer and shows how to simplify and delegate I/O operations to the regmap subsystem. Dealing with devices, whether they are built-in in the SoC (memory mapped I/O, also known as
MMIO) or seated on I2C/SPI buses, consists of accessing (reading/modifying/updating)
registers. Regmap became necessary because a lot of device drivers open-coded their register access routines. Regmap stands for Register Map. It was primarily developed for
ALSA SoC (ASoC) in order to get rid of redundant open-coded SPI/I2C register access routines in codec drivers. At its origin, regmap provided a set of APIs for reading/writing non-memory-map I/O (for example, I2C and SPI read/write). Since then, MMIO regmap has been upgraded so that we can use regmap to access MMIO.
Nowadays, this framework abstracts I2C, SPI, and MMIO register access, and not only handles locking when necessary, but also manages the register cache, as well as register readability and writability. It also handles IRQ chips and IRQs. This chapter will discuss regmap and explain the way to use it to abstract register access with I2C, SPI, and MMIO
devices. We will also describe how to use regmap to manage IRQ and IRQ controllers.
72 Leveraging the Regmap API and Simplifying the Code
This chapter will cover the following topics:
• Introduction to regmap and its data structures: I2C, SPI, and MMIO
• Regmap and IRQ management
• Regmap IRQ API and data structures
Technical requirements
In order to be comfortable when going through this chapter, you’ll need the following:
• Good C programming skills
• Familiarity with the concept of the device tree
• Linux kernel v4.19.X sources, available at https://git.kernel.org/pub/
scm/linux/kernel/git/stable/linux.git/refs/tags
Introduction to regmap and its data structures – I2C, SPI, and MMIO
Regmap is an abstraction register access mechanism provided by the Linux kernel that mainly targets SPI, I2C, and memory-mapped registers.
APIs in this framework are bus agnostic and handle the underlying configuration under the hood. That being said, the main data structure in this framework is struct regmap_config, defined in include/linux/regmap.h in the kernel source tree as follows:
```c
struct regmap_config {
```
const char *name;
```c
int reg_bits;
int reg_stride;
int pad_bits;
int val_bits;
bool (*writeable_reg)(struct device *dev, unsigned int reg);
bool (*readable_reg)(struct device *dev, unsigned int reg);
bool (*volatile_reg)(struct device *dev, unsigned int reg);
bool (*precious_reg)(struct device *dev, unsigned int reg);
int (*reg_read)(void *context, unsigned int reg,
```
Introduction to regmap and its data structures – I2C, SPI, and MMIO 73
```c
unsigned int *val);
int (*reg_write)(void *context, unsigned int reg,
unsigned int val);
bool disable_locking;
regmap_lock lock;
regmap_unlock unlock;
void *lock_arg;
bool fast_io;
unsigned int max_register;
```
const struct regmap_access_table *wr_table;
const struct regmap_access_table *rd_table;
const struct regmap_access_table *volatile_table;
const struct regmap_access_table *precious_table;
const struct reg_default *reg_defaults;
```c
unsigned int num_reg_defaults;
unsigned long read_flag_mask;
unsigned long write_flag_mask;
enum regcache_type cache_type;
bool use_single_rw;
bool can_multi_write;
};
```
For simplicity, some of the fields in this structure have been removed and are not discussed in this chapter. As long as struct regmap_config is properly completed,
users may ignore underlying bus mechanisms. Let’s introduce the fields in this data structure:
• reg_bits indicates the size of a register in terms of bits. In other words, it is the number of bits in a register’s address.
• reg_stride is the stride of the register address. A register address is valid if it is a multiple of this value. If set to 0, a value of 1 will be used, meaning any address is valid. Any read/write to an address that is not a multiple of this value will return
-EINVAL.
74 Leveraging the Regmap API and Simplifying the Code
• pad_bits is the number of bits of padding between the register and the value. This is the number of bits to shift the register’s value left when formatting.
• val_bits: This represents the number of bits used to store a register’s value. It is a mandatory field.
• writeable_reg: If provided, this optional callback will be called on each regmap write operation to check whether the given address is writable or not. If this function returns false on an address given to a regmap write transaction, the transaction will return -EIO. The following excerpt shows how this callback can be implemented:
```c
static bool foo_writeable_register(struct device *dev,
unsigned int reg)
{
switch (reg) {
```
case 0x30 ... 0x38:
case 0x40 ... 0x45:
case 0x50 ... 0x57:
case 0x60 ... 0x6e:
case 0xb0 ... 0xb2:
```c
return true;
```
default:
```c
return false;
}
}
```
• readable_reg: This is the same as writeable_reg but for register read operations.
• volatile_reg: This is an optional callback that, if provided, will be called every time a register needs to be read or written through the regmap cache. If the register is volatile (the register value can’t be cached), the function should return true. A
direct read/write is then performed on the register. If false is returned, it means the register is cacheable. In this case, the cache will be used for a read operation,
and the cache will be written to in the case of a write operation. The following is an example, with fake register addresses chosen randomly:
```c
static bool volatile_reg(struct device *dev,
unsigned int reg)
{
switch (reg) {
```
Introduction to regmap and its data structures – I2C, SPI, and MMIO 75
case 0x30:
case 0x31:
[...]
case 0xb3:
```c
return false;
```
case 0xb4:
```c
return true;
```
default:
```c
if ((reg >= 0xb5) && (reg <= 0xcc))
return false;
```
[...]
break;
```c
}
return true;
}
```
• reg_read: If your device needs special hacks for reading operations, you can provide a custom read callback and make this field point to it so that instead of using standard regmap read functions, this callback is used. That said, most devices do not need this.
• reg_write: This is the same as reg_read but for write operations.
• disable_locking: This shows whether the lock/unlock callbacks should be used or not. If false, no locking mechanisms will be used. It means this regmap is either protected by external means or is guaranteed not to be accessed from multiple threads.
• lock/unlock: These are optional lock/unlock callbacks that override the regmap’s default lock/unlock functions. These are based on spinlock or mutex, depending on whether accessing the underlying device may sleep or not.
• lock_arg: This is the only argument of the lock/unlock functions (it will be ignored if the regular lock/unlock functions are not overridden).
• fast_io: This indicates that the register’s I/O is fast. If set, the regmap will use a spinlock instead of a mutex to perform locking. This field is ignored if custom lock/
unlock (not discussed here) functions are used (see the lock/unlock fields of struct regmap_config in the kernel sources). It should be used only for
"no bus" cases (MMIO devices), not for slow buses such as I2C, SPI, or similar buses whose accesses may sleep.
76 Leveraging the Regmap API and Simplifying the Code
• wr_table: This is an alternative to the writeable_reg() callback, of type regmap_access_table, which is a structure holding a yes_range and a no_
range field, both of which are pointers to struct regmap_range. Any register that belongs to a yes_range entry is considered writable, and is considered not writable if it belongs to no_range or is not specified in yes_range.
• rd_table: This is the same as wr_table, but for any read operation.
• volatile_table: Instead of volatile_reg, you could provide volatile_
table. The principle is the same as wr_table and rd_table, but for the caching mechanism.
• max_register: This is optional; it specifies the maximum valid register address upon which no operation is permitted.
• reg_defaults is an array of elements of type reg_default, where each element is a {reg, value} pair that represents the power-on reset values for a given register. This is used along with the cache so that reading an address that exists in this array and that has not been written since a power-on reset will return the default register value in this array without performing any read transactions on the device. An example of this is the IIO device driver, which you can find out more about at https://elixir.bootlin.com/linux/v4.19/source/
drivers/iio/light/apds9960.c.
• use_single_rw: This is a Boolean that, if set, will instruct the regmap to convert any bulk write or read operations on the device into a series of single write or read operations. This is useful for devices that do not support bulk read and/or write operations.
• can_multi_write: This only targets write operations. If set, it indicates that this device supports the multi-write mode of bulk write operations. If it’s empty, multiwrite requests will be split into individual write operations.
• num_reg_defaults: This is the number of elements in reg_defaults.
• read_flag_mask: This is a mask to be set in the highest bytes of the register when doing a read. Normally, in SPI or I2C, a write or a read will have the highest bit set in the top byte to differentiate write and read operations.
• write_flag_mask: This is a mask to be set in the highest bytes of the register when doing a write.
• cache_type: This is the actual cache type, which can be either REGCACHE_NONE,
REGCACHE_RBTREE, REGCACHE_COMPRESSED, or REGCACHE_FLAT.
Introduction to regmap and its data structures – I2C, SPI, and MMIO 77
Initializing a regmap is as simple as calling one of the following functions depending on the bus behind which our device sits:
```c
struct regmap * devm_regmap_init_i2c(
struct i2c_client *client,
struct regmap_config *config)
struct regmap * devm_regmap_init_spi(
struct spi_device *spi,
```
const struct regmap_config);
```c
struct regmap * devm_regmap_init_mmio(
struct device *dev,
void __iomem *regs,
```
const struct regmap_config *config)
```c
#define devm_regmap_init_spmi_base(dev, config) \
```
__regmap_lockdep_wrapper(__devm_regmap_init_spmi_base, \
#config, dev, config)
```c
#define devm_regmap_init_w1(w1_dev, config) \
```
__regmap_lockdep_wrapper(__devm_regmap_init_w1, #config, \
w1_dev, config)
In the preceding prototypes, the return value will be a valid pointer to struct regmap or ERR_PTR() if there is an error. The regmap will be automatically freed by the device management code. regs is a pointer to the memory-mapped IO region (returned by devm_ioremap_resource() or any ioremap* family function). dev is the device
(of type struct device) that will be interacted with. The following example is an excerpt of drivers/mfd/sun4i-gpadc.c in the kernel source code:
```c
struct sun4i_gpadc_dev {
struct device *dev;
struct regmap *regmap;
struct regmap_irq_chip_data *regmap_irqc;
void __iomem *base;
};
78 Leveraging the Regmap API and Simplifying the Code static const struct regmap_config sun4i_gpadc_regmap_config = {
```
.reg_bits = 32,
.val_bits = 32,
.reg_stride = 4,
.fast_io = true,
```c
};
static int sun4i_gpadc_probe(struct platform_device *pdev)
{
struct sun4i_gpadc_dev *dev;
struct resource *mem;
```
[...]
mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
```c
dev->base = devm_ioremap_resource(&pdev->dev, mem);
if (IS_ERR(dev->base))
return PTR_ERR(dev->base);
dev->dev = &pdev->dev;
dev_set_drvdata(dev->dev, dev);
dev->regmap = devm_regmap_init_mmio(dev->dev, dev->base,
```
&sun4i_gpadc_regmap_config);
```c
if (IS_ERR(dev->regmap)) {
ret = PTR_ERR(dev->regmap);
dev_err(&pdev->dev, "failed to init regmap: %d\n", ret);
return ret;
}
```
[...]
This excerpt shows how to create a regmap. Though this excerpt is MMIO-oriented,
the concept remains the same for other types. Instead of using devm_regmap_init_
MMIO(), we would use devm_regmap_init_spi() or devm_regmap_init_i2c()
respectively for an SPI- or I2C-based regmap.
Introduction to regmap and its data structures – I2C, SPI, and MMIO 79
## Accessing device registers
There are two main functions for accessing device registers. These are regmap_write()
and regmap_read(), which take care of locking and abstracting the underlying bus:
```c
int regmap_write(struct regmap *map,
unsigned int reg,
unsigned int val);
int regmap_read(struct regmap *map,
unsigned int reg,
unsigned int *val);
```
In the preceding two functions, the first argument, map, is the regmap structure returned during initialization. reg is the register address to write/read data to/from. val is the data to be written in a write operation, or the read value in a read operation. The following is a detailed description of these APIs:
• regmap_write is used to write data to the device. The following are the steps performed by this function:
1) First, it checks whether reg is aligned with the regmap_config.reg_
stride. If not, it returns -EINVAL and the function fails.
2) It then takes the lock depending on the fast_io, lock, and unlock fields.
If a lock callback is provided, it will be used to take the lock. Otherwise, the regmap core will use its internal default lock function, using a spinlock or a mutex depending on whether fast_io has been set or not. Next, the regmap core performs some sanity checks on the register address passed as follows:
--If max_register is set, it will check whether this register’s address is less than max_register. If the address is not less than max_register, then regmap_
write() fails, returning an -EIO (invalid I/O) error code
--Then, if the writeable_reg callback is set, this callback is called with the register as a parameter. If this callback returns false, then regmap_write()
fails, returning -EIO. If writeable_reg is not set but wr_table is set, the regmap core will check whether the register address lies within no_range. If it does, then regmap_write() fails and returns -EIO. If it doesn’t, the regmap core will check whether the register address lies in yes_range. If it is not present there,
then regmap_write() fails and returns -EIO.
80 Leveraging the Regmap API and Simplifying the Code
3) If the cache_type field is set, then caching will be used. The value to be written will be cached for future reference instead of being written to the hardware.
4) If cache_type is not set, then the write routine is invoked immediately to write the value into the hardware register. This routine will first apply write_
flag_mask to the first byte of the register address before writing the value into this register.
5) Finally, the lock is released using the appropriate unlocking function.
• regmap_read is used to read data from the device. This function performs the same security and sanity checks as regmap_write(), but replaces writable_
reg and wr_table with readable_reg and rd_table. When it comes to caching, if it is enabled, the register value is read from the cache. If caching is not enabled, the read routine is called to read the value from the hardware register instead. That routine will apply read_flag_mask to the highest byte of the register address prior to the read operation, and *val is updated with the new value read. After this, the lock is released using the appropriate unlocking function.
While the preceding accessors target a single register at a time, others can perform bulk accesses, as we will see in the next section.
## Reading/writing multiple registers in a single shot
Sometimes you may want to perform bulk read/write operations of data from/to a register range at the same time. Even if you use regmap_read() or regmap_write() in a loop, the best solution would be to use the regmap APIs provided for such situations.
These functions are regmap_bulk_read() and regmap_bulk_write():
```c
int regmap_bulk_read(struct regmap *map, unsigned int reg,
void *val, size_t val_count);
int regmap_bulk_write(struct regmap *map, unsigned int reg,
```
const void *val, size_t val_count)
Introduction to regmap and its data structures – I2C, SPI, and MMIO 81
These functions read/write multiple registers from/to the device. map is the regmap used to perform operations. For a read operation, reg is the first register from where reading should start, val is a pointer to the buffer where read values should be stored in native register size of the device (it means if the device register size is 4 bytes, the read value will be stored in 4 bytes units), and val_count is the number of registers to read. For a write operation, reg is the first register to be written from, val is a pointer to the block of data to be written in native register size of the device, and val_count is the number of registers to write. For both of these functions, a value of 0 will be returned on success and a negative errno will be returned if there is an error.
Tip
There are other interesting read/write functions provided by this framework.
Take a look at the kernel header file for more information. An interesting one is regmap_multi_reg_write(), which writes multiple registers in a set of {register, value} pairs supplied in any order, possibly not all in a single range,
to the device given as a parameter.
Now that we are familiar with register access, we can go further by managing register content at a bit level.
## Updating bits in registers
To update a bit in a given register, we have regmap_update_bits(), a three-in-one function. Its prototype is as follows:
```c
int regmap_update_bits(struct regmap *map, unsigned int reg,
unsigned int mask, unsigned int val)
```
It performs a read/modify/write cycle on the register map. It is a wrapper of _regmap_
update_bits(), which looks as follows:
```c
static int _regmap_update_bits(
struct regmap *map, unsigned int reg,
unsigned int mask, unsigned int val,
bool *change, bool force_write)
{
int ret;
unsigned int tmp, orig;
if (change)
```
82 Leveraging the Regmap API and Simplifying the Code
*change = false;
```c
if (regmap_volatile(map, reg) && map->reg_update_bits) {
ret = map->reg_update_bits(map->bus_context,
```
reg, mask, val);
```c
if (ret == 0 && change)
```
*change = true;
```c
} else {
```
ret = _regmap_read(map, reg, &orig);
```c
if (ret != 0)
return ret;
```
tmp = orig & ~mask;
tmp |= val & mask;
```c
if (force_write || (tmp != orig)) {
```
ret = _regmap_write(map, reg, tmp);
```c
if (ret == 0 && change)
```
*change = true;
```c
}
}
return ret;
}
```
Bits that need to be updated should be set to 1 in mask, and the corresponding bits will be given the value of the bit of the same position in val. As an example, to set the first
(BIT(0)) and third (BIT(2)) bits to 1, mask should be 0b00000101 and the value should be 0bxxxxx1x1. To clear the seventh bit (BIT(6)), mask must be 0b01000000
and the value should be 0bx0xxxxxx, and so on.
Tip
For debugging purpose, you can use the debugfs filesystem to dump the content of the regmap managed registers, as the following excerpt shows:
```bash
# mount -t debugfs none /sys/kernel/debug
# cat /sys/kernel/debug/regmap/1-0008/registers
```
This will dump the register addresses along with their values in
<addr:value> format.
Introduction to regmap and its data structures – I2C, SPI, and MMIO 83
In this section, we have seen how easy it is to access hardware registers. Moreover, we have learned some fancy tricks for playing with registers at the bit level, which is often used in status and configuration registers. Next, we will have a look at IRQ management.
## Regmap and IRQ management
Regmap does not only abstract access to registers. Here, we will see how this framework abstracts IRQ management at a lower level, such as IRQ chip handling, thus hiding boilerplate operations.
## Quick recap on Linux kernel IRQ management
IRQs are exposed to devices by means of special devices called interrupt controllers.
From a software point of view, an interrupt controller device driver manages and exposes these lines using the virtual IRQ concept, known as the IRQ domain in the Linux kernel.
Interrupt management is built on top of the following structures:
• struct irq_chip: This structure is the Linux representation of an IRQ
controller and implements a set of methods to drive the interrupt controller that are directly called by the core IRQ code. If necessary, this structure should be filled by the driver, providing a set of callbacks allowing us to manage IRQs on the IRQ chip,
such as irq_startup, irq_shutdown, irq_enable, irq_disable, irq_
ack, irq_mask, irq_unmask, irq_eoi, and irq_set_affinity. Dumb
IRQ chip devices (chip that does not allow IRQ management, for example) should use the kernel-provided dummy_irq_chip.
• struct irq_domain: Each interrupt controller is given a domain, which is for the controller what the address space is for a process. The struct irq_domain structure stores mappings between hardware IRQs and Linux IRQs (that is, virtual
IRQs, or virq). It is the hardware interrupt number translation object. This structure provides the following:
--A pointer to the firmware node for a given interrupt controller (fwnode).
--A method to convert a firmware (device tree) description of an IRQ into an ID
local to the interrupt controller (the hardware IRQ number, known as the hwirq).
For gpio chips that also act as IRQ controllers, the hardware IRQ number (hwirq)
for a given gpio line corresponds to the local index of this line in the chip most of times.
--A way to retrieve the Linux view of an IRQ from the hwirq.
84 Leveraging the Regmap API and Simplifying the Code
• struct irq_desc: This structure is the Linux kernel view of an interrupt,
containing all of the core stuff and one-to-one mapping to the Linux interrupt number.
• struct irq_action: This is the structure Linux uses to describe an IRQ
handler.
• struct irq_data: This structure is embedded in the struct irq_desc structure, and contains the following:
--The data that is relevant to the irq_chip managing this interrupt
--Both the Linux IRQ number and the hwirq
--A pointer to the irq_chip
--A pointer to the interrupt translation domain (irq_domain)
Always keep in mind that the irq_domain is for the interrupt controller what an address space is for a process, as it stores mappings between virqs and hwirqs.
An interrupt controller driver creates and registers irq_domain by calling one of the irq_domain_add_<mapping_method>() functions. These functions are actually irq_domain_add_linear(), irq_domain_add_tree(), and irq_domain_
add_nomap(). In fact, <mapping_method> is the method by which hwirqs should be mapped to virqs.
```c
irq_domain_add_linear() creates an empty and fixed-size table, indexed by the hwirq number. struct irq_desc is allocated for each hwirq that gets mapped. The allocated IRQ descriptor is then stored in the table, at the index that equals the hwirq to which it has been allocated. This linear mapping is suitable for fixed and small numbers of hwirqs (lower than 256).
```
While the main advantages of this mapping are the fact that the IRQ number lookup time is fixed and that irq_desc is allocated for in-use IRQs only, the major drawback comes from the size of the table, which can be as large as the largest possible hwirq number. The majority of drivers should use the linear map. This function has the following prototype:
```c
struct irq_domain *irq_domain_add_linear(
struct device_node *of_node,
unsigned int size,
```
const struct irq_domain_ops *ops,
```c
void *host_data)
```
Introduction to regmap and its data structures – I2C, SPI, and MMIO 85
```c
irq_domain_add_tree() creates an empty irq_domain that maintains the mapping between Linux IRQs and hwirq numbers in a radix tree. When an hwirq is mapped, a struct irq_desc is allocated, and the hwirq is used as the lookup key for the radix tree. A tree map is a good choice if the hwirq number is very large, since it does not need to allocate a table as large as the largest hwirq number. The disadvantage is that the hwirq-to-IRQ number lookup is dependent on how many entries are in the table.
```
Very few drivers should need this mapping. It has the following prototype:
```c
struct irq_domain *irq_domain_add_tree(
struct device_node *of_node,
```
const struct irq_domain_ops *ops,
```c
void *host_data)
irq_domain_add_nomap() is something you will probably never use; however, its entire description is available in Documentation/IRQ-domain.txt, in the kernel source tree. Its prototype is as follows:
struct irq_domain *irq_domain_add_nomap(
struct device_node *of_node,
unsigned int max_irq,
```
const struct irq_domain_ops *ops,
```c
void *host_data)
```
In all of those prototypes, of_node is a pointer to the interrupt controller’s DT node.
size represents the number of interrupts in the domain in case of linear mapping. ops represents map/unmap domain callbacks, and host_data is the controller’s private data pointer. As these three functions all create empty irq domains, you should use the irq_
create_mapping() function with the hwirq and a pointer to the irq domain passed to it in order to create a mapping, and insert this mapping into the domain:
```c
unsigned int irq_create_mapping(struct irq_domain *domain,
irq_hw_number_t hwirq)
```
In the preceding prototype, domain is the domain to which this hardware interrupt belongs. A NULL value means the default domain. hwirq is the hardware IRQ number you need to create a mapping for. This function maps a hardware interrupt into the Linux
IRQ space and returns a Linux IRQ number. Also, keep in mind that only one mapping per hardware interrupt is permitted. The following is an example of creating a mapping:
```c
unsigned int virq = 0;
```
virq = irq_create_mapping(irq_domain, hwirq);
```c
86 Leveraging the Regmap API and Simplifying the Code if (!virq) {
```
ret = -EINVAL;
goto err_irq;
```c
}
```
In the preceding code, virq is the Linux kernel IRQ (the virtual IRQ number, virq)
corresponding to the mapping.
Important note
When writing drivers for GPIO controllers that are also interrupt controllers,
```c
irq_create_mapping() is called from within the gpio_chip.
```
to_irq() callback, and the virq is returned as return irq_create_
```c
mapping(gpiochip->irq_domain, hwirq), where hwirq is the
```
GPIO offset from the GPIO chip.
Some drivers prefer creating the mappings and populating the domain for each hwirq in advance inside the probe() function, as shown here:
```c
for (j = 0; j < gpiochip->chip.ngpio; j++) {
irq = irq_create_mapping(gpiochip ->irq_domain, j);
}
```
After this, such drivers just call irq_find_mapping() (given the hwirq) into the to_irq() callback function. irq_create_mapping() will allocate a new struct irq_desc structure if no mapping already exists for the given hwirq, associate it with the hwirq, and call the irq_domain_ops.map() callback (by using the irq_domain_
associate() function) so that the driver can perform any required hardware setup.
## The struct irq_domain_ops
This structure exposes some callbacks that are specific to the irq domain. As mappings are created in a given irq domain, each mapping (actually each irq_desc) should be given an irq configuration, some private data, and a translation function (given a device tree node and an interrupt specifier, the translation function decodes the hardware irq number and Linux irq type value). This is what callbacks in this structure do:
```c
struct irq_domain_ops {
int (*map)(struct irq_domain *d, unsigned int virq,
irq_hw_number_t hw);
void (*unmap)(struct irq_domain *d, unsigned int virq);
```
Introduction to regmap and its data structures – I2C, SPI, and MMIO 87
```c
int (*xlate)(struct irq_domain *d, struct device_node *node,
```
const u32 *intspec, unsigned int intsize,
```c
unsigned long *out_hwirq,
unsigned int *out_type);
};
```
Each Linux kernel IRQ management of the elements in the preceding data structure deserves a section on its own to describe it.
## irq_domain_ops.map()
The following is the prototype of this callback:
```c
int (*map)(struct irq_domain *d, unsigned int virq,
irq_hw_number_t hw);
```
Before describing what this function does, let’s describe its arguments:
• d: The IRQ domain used by this IRQ chip
• virq: The global IRQ number used by this GPIO-based IRQ chip
• hw: The local IRQ/GPIO line offset on this GPIO chip
.map() creates or updates a mapping between a virq and an hwirq. This callback sets up the IRQ configuration. It is called (internally by the irq core) only once for a given mapping. This is where we set the irq chip data for the given irq, which could be done using irq_set_chip_data(), which has this prototype:
```c
int irq_set_chip_data(unsigned int irq, void *data);
```
Depending on the type of the IRQ chip (nested or chained), additional actions can be performed.
88 Leveraging the Regmap API and Simplifying the Code
## irq_domain_ops.xlate()
Given a DT node and an interrupt specifier, this callback decodes the hardware IRQ
number along with its Linux IRQ type value. Depending on the #interrupt-cells property specified in your DT controller node, the kernel provides a generic translation function:
• irq_domain_xlate_twocell(): This generic translation function is for direct two-cell binding. The DT IRQ specifier works with two-cell bindings, where the cell values map directly to the hwirq number and Linux IRQ flags.
• irq_domain_xlate_onecell(): This is a generic xlate function for direct one-cell bindings.
• irq_domain_xlate_onetwocell(): This is a generic xlate function for one- or two-cell bindings.
An example of the domain operation is as follows:
```c
static struct irq_domain_ops mcp23016_irq_domain_ops = {
```
.map = my_irq_domain_map,
.xlate = irq_domain_xlate_twocell,
```c
};
```
The distinctive feature of the preceding data structure is the value assigned to the .xlate element, that is, irq_domain_xlate_twocell. This means we are expecting a two-cell irq specifier in the device tree in which the first cell would specify the irq, and the second would specify its flags.
## Chaining IRQs
When an interrupt occurs, the irq_find_mapping() helper function can be used to find the Linux IRQ number from the hwirq number. This hwirq number could be, for example, the GPIO offset in a bank of GPIO controllers. Once a valid virq has been found and returned, you should call either handle_nested_irq() or generic_handle_
irq() on this virq. The magic comes from the previous two functions, which manage the irq-flow handlers.
This means that there are two ways to play with interrupt handlers. Hard interrupt handlers, or chained interrupts, are atomic and run with irqs disabled and may schedule the threaded handler; there are also the simply threaded interrupt handlers, known as nested interrupts, which may be interrupted by other interrupts.
Introduction to regmap and its data structures – I2C, SPI, and MMIO 89
## Chained interrupts
This approach is used for a controller that may not sleep, such as the SoC’s internal
GPIO controller, which is memory-mapped and whose accesses do not sleep. Chained means that those interrupts are just chains of function calls (for example, the SoC’s GPIO
controller interrupt handler is being called from within the GIC interrupt handler, just like a function call). With this approach, child IRQ handlers are being called inside the parent hwirq handler. generic_handle_irq() must be used here for chaining child
IRQ handlers inside the parent hwirq handler. Even from within the child interrupt handlers, we are still in an atomic context (hardware interrupt). You cannot call functions that may sleep.
For chained (and only chained) IRQ chips, irq_domain_ops.map() is also the right place to assign a high-level irq-type flow handler to the given irq using irq_set_
chip_and_handler(), so that this high-level code, depending on what it is, will do some hacks before calling the corresponding irq handler. The magic operates here thanks to the irq_set_chip_and_handler() function:
```c
void irq_set_chip_and_handler(unsigned int irq,
struct irq_chip *chip,
irq_flow_handler_t handle)
```
In the preceding prototype, irq represents the Linux IRQ (the virq), given as a parameter to the irq_domain_ops.map() function; chip is your irq_chip structure; and handle is your high-level interrupt flow handler.
Important note
Some controllers are quite dumb and need almost nothing in their irq_chip structure. In this case, you should pass dummy_irq_chip to irq_set_
chip_and_handler(). dummy_irq_chip is defined in kernel/
irq/dummychip.c.
90 Leveraging the Regmap API and Simplifying the Code
The following code flow summarizes what irq_set_chip_and_handler() does:
```c
void irq_set_chip_and_handler(unsigned int irq,
struct irq_chip *chip,
irq_flow_handler_t handle)
{
struct irq_desc *desc = irq_get_desc(irq);
desc->irq_data.chip = chip;
desc->handle_irq = handle;
}
```
These are some possible high-level IRQ flow handlers provided by the generic layer:
/*
* Built-in IRQ handlers for various IRQ types,
```c
* callable via desc->handle_irq()
```
*/
```c
void handle_level_irq(struct irq_desc *desc);
void handle_fasteoi_irq(struct irq_desc *desc);
void handle_edge_irq(struct irq_desc *desc);
void handle_edge_eoi_irq(struct irq_desc *desc);
void handle_simple_irq(struct irq_desc *desc);
void handle_untracked_irq(struct irq_desc *desc);
void handle_percpu_irq(struct irq_desc *desc);
void handle_percpu_devid_irq(struct irq_desc *desc);
void handle_bad_irq(struct irq_desc *desc);
```
Each function name describes quite well the type of IRQ it handles. This is what irq_
domain_ops.map() may look like for a chained IRQ chip:
```c
static int my_chained_irq_domain_map(struct irq_domain *d,
unsigned int virq,
irq_hw_number_t hw)
{
irq_set_chip_data(virq, d->host_data);
irq_set_chip_and_handler(virq, &dummy_irq_chip,
handle_ edge_irq);
return 0;
}
```
Introduction to regmap and its data structures – I2C, SPI, and MMIO 91
While writing the parent irq handler for a chained IRQ chip, the code should call generic_handle_irq() on each child irq. This function simply calls irq_desc-
```c
>handle_irq(), which points to the high-level interrupt handler assigned to the given child IRQ using irq_set_chip_and_handler(). The underlying high-level irq event handler (let’s say handle_level_irq()) will first do some hacks, then will run the hard irq-handler (irq_desc->action->handler) and, depending on the return value, will run the threaded handler (irq_desc->action->thread_fn) if provided.
```
Here is an example of the parent IRQ handler for a chained IRQ chip, whose original code is located in drivers/pinctrl/pinctrl-at91.c in the kernel source:
```c
static void parent_hwirq_handler(struct irq_desc *desc)
{
struct irq_chip *chip = irq_desc_get_chip(desc);
struct gpio_chip *gpio_chip =
irq_desc_get_handler_ data(desc);
struct at91_gpio_chip *at91_gpio = gpiochip_get_data
```
(gpio_ chip);
```c
void __iomem *pio = at91_gpio->regbase;
unsigned long isr;
int n;
chained_irq_enter(chip, desc);
for (;;) {
```
/* Reading ISR acks pending (edge triggered) GPIO
* interrupts. When there are none pending, we’re
* finished unless we need to process multiple banks
* (like ID_PIOCDE on sam9263).
*/
isr = readl_relaxed(pio + PIO_ISR) &
```c
readl_relaxed(pio + PIO_IMR);
if (!isr) {
if (!at91_gpio->next)
```
break;
```c
at91_gpio = at91_gpio->next;
pio = at91_gpio->regbase;
gpio_chip = &at91_gpio->chip;
```
92 Leveraging the Regmap API and Simplifying the Code continue;
```c
}
for_each_set_bit(n, &isr, BITS_PER_LONG) {
generic_handle_irq(
irq_find_mapping(gpio_chip->irq.domain, n));
}
}
chained_irq_exit(chip, desc);
```
/* now it may re-trigger */
[...]
```c
}
```
Chained IRQ chip drivers do not need to register the parent irq handler using devm_request_threaded_irq() or devm_request_irq(). This handler is automatically registered when the driver calls irq_set_chained_handler_and_
data() on this parent irq, given the associated handler as parameter, along with some private data:
```c
void irq_set_chained_handler_and_data(unsigned int irq,
irq_flow_handler_t handle,
void *data)
```
The parameters of this function are quite self-explanatory. You should call this function in the probe function as follows:
```c
static int my_probe(struct platform_device *pdev)
{
int parent_irq, i;
struct irq_domain *my_domain;
```
parent_irq = platform_get_irq(pdev, 0);
```c
if (!parent_irq) {
pr_err("failed to map parent interrupt %d\n", parent_irq);
return -EINVAL;
}
```
my_domain =
```c
irq_domain_add_linear(np, nr_irq, &my_irq_domain_ops,
```
Introduction to regmap and its data structures – I2C, SPI, and MMIO 93
my_private_data);
```c
if (WARN_ON(!my_domain)) {
pr_warn("%s: irq domain init failed\n", __func__);
```
return;
```c
}
```
/* This may be done elsewhere */
```c
for(i = 0; i < nr_irq; i++) {
int virqno = irq_create_mapping(my_domain, i);
```
/*
* May need to mask and clear all IRQs before
* registering a handler
*/
[...]
```c
irq_set_chained_handler_and_data(parent_irq,
```
parent_hwirq_handler,
my_private_data);
/*
* May need to call irq_set_chip_data() on
* the virqno too
*/
[...]
```c
}
```
[...]
```c
}
```
94 Leveraging the Regmap API and Simplifying the Code
In the preceding fake probe method, a linear domain is created using irq_domain_
add_linear(), and an irq mapping (virtual irq) is created in this domain with irq_
create_mapping(). Finally, we set a high-level chained flow handler and its data for the main (or parent) IRQ.
Important note
Note that irq_set_chained_handler_and_data() automatically enables the interrupt (specified in the first parameter), assigns its handler
(also given as a parameter), and marks this interrupt as IRQ_NOREQUEST,
IRQ_NOPROBE, or IRQ_NOTHREAD, which mean this interrupt cannot be requested via request_irq() anymore, cannot be probed by auto probing,
and cannot be threaded at all (it is chained), respectively.
## Nested interrupts
The nested flow method is used by IRQ chips that may sleep, such as those that are on slow buses, such as I2C (for example, an I2C GPIO expander). "Nested" refers to those interrupt handlers that do not run in the hardware context (they are not really hwirq, and are not in an atomic context), but are threaded instead and can be preempted. Here, the handler function is called inside the calling threads context. For nested (and only nested)
IRQ chips, the irq_domain_ops.map() callback is also the right place to set up irq configuration flags. The most important configuration flags are as follows:
• IRQ_NESTED_THREAD: This is a flag that indicates that on devm_request_
threaded_irq(), no dedicated interrupt thread should be created for the irq handler, as it is called nested in the context of a demultiplexing interrupt handler thread (there’s more information about this in the __setup_irq() function,
implemented in kernel/irq/manage.c in the kernel source). You can use void irq_set_nested_thread(unsigned int irq, int nest) to act on this flag, where irq corresponds to the global interrupt number and nest should be 0 to clear or 1 to set the IRQ_NESTED_THREAD flag.
• IRQ_NOTHREAD: This flag can be set using void irq_set_
nothread(unsigned int irq). It is used to mark the given IRQ as non-threadable.
This is what irq_domain_ops.map() may look like for a nested IRQ chip:
```c
static int my_nested_irq_domain_map(struct irq_domain *d,
unsigned int virq,
irq_hw_number_t hw)
{
```
Introduction to regmap and its data structures – I2C, SPI, and MMIO 95
```c
irq_set_chip_data(virq, d->host_data);
irq_set_nested_thread(virq, 1);
irq_set_noprobe(virq);
return 0;
}
```
While writing the parent irq handler for a nested IRQ chip, the code should call handle_
```c
nested_irq() in order to handle child irq handlers so that they run from the parent irq thread. handle_nested_irq() does not care about irq_desc->action-
>handler, which is the hard irq handler. It simply runs irq_desc->action-
```
>thread_fn:
```c
static irqreturn_t mcp23016_irq(int irq, void *data)
{
struct mcp23016 *mcp = data;
unsigned int child_irq, i;
```
/* Do some stuff */
[...]
```c
for (i = 0; i < mcp->chip.ngpio; i++) {
if (gpio_value_changed_and_raised_irq(i)) {
child_irq = irq_find_mapping(mcp->chip.irqdomain,
```
i);
```c
handle_nested_irq(child_irq);
}
}
```
[...]
```c
}
```
Nested IRQ chip drivers must register the parent irq handler using devm_request_
threaded_irq(), as there is no function like irq_set_chained_handler_and_
data() for this kind of IRQ chip. It does not make sense to use this API for nested IRQ
chips. Nested IRQ chips, most of the time, are GPIO chip-based. Thus, we would be better off using the GPIO chip-based IRQ chip API, or using the regmap-based IRQ chip API, as shown in the next section. However, let’s see what such an example looks like:
```c
static int my_probe(struct i2c_client *client,
```
const struct i2c_device_id *id)
```c
{
int parent_irq, i;
```
96 Leveraging the Regmap API and Simplifying the Code struct irq_domain *my_domain;
[...]
```c
int irq_nr = get_number_of_needed_irqs();
```
/* Do we have an interrupt line ? Enable the IRQ chip */
```c
if (client->irq) {
```
domain = irq_domain_add_linear(
```c
client->dev.of_node, irq_nr,
```
&my_irq_domain_ops, my_private_data);
```c
if (!domain) {
dev_err(&client->dev,
```
"could not create irq domain\n");
```c
return -ENODEV;
}
```
/*
* May be creating irq mapping in this domain using
* irq_create_mapping() or let the mfd core doing
* this if it is an MFD chip device
*/
[...]
ret =
```c
devm_request_threaded_irq(
&client->dev, client->irq,
```
NULL, my_parent_irq_thread,
IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
"my-parent-irq", my_private_data);
[...]
```c
}
```
[...]
```c
}
```
Introduction to regmap and its data structures – I2C, SPI, and MMIO 97
In the preceding probe method, there are two main differences with the chained flow:
• First, the way the main IRQ is registered: While chained IRQ chips used irq_set_
chained_handler_and_data(), which automatically registered the handler,
the nested flow method has to register its handler explicitly using the request_
threaded_irq() family method.
• Second, the way the main IRQ handler invokes underlying irq handlers: In the chained flow, handle_nested_irq() is called in the main IRQ handler, which invokes the handlers of each underlying irq as a chain of function calls, which are executed in the same context as the main handler, that is, atomically (the atomicity is also known as hard-irq). However, the nested flow handler had to call handle_nested_irq(), which executes the handler (thread_fn) of the underlying irq in the thread context of the parent.
These are the main differences between chained and nested flows.
## irqchip and gpiolib API – new generation
Since each irq-gpiochip driver open-coded its own irqdomain handling, this led to a lot of redundant code. Kernel developers decided to move that code to the gpiolib framework, thus providing the GPIOLIB_IRQCHIP Kconfig symbol, enabling us to use a unified irq domain management API for GPIO chips. That portion of code helps with handling the management of GPIO IRQ chips and the associated irq_domain and resource allocation callbacks, as well as their setup, using the reduced set of helper functions. These are gpiochip_irqchip_add()or gpiochip_irqchip_add_
nested(), and gpiochip_set_chained_irqchip() or gpiochip_set_
nested_irqchip(). gpiochip_irqchip_add() or gpiochip_irqchip_add_
nested() both add an IRQ chip to a GPIO chip. Here are their respective prototypes:
```c
static inline int gpiochip_irqchip_add(
struct gpio_chip *gpiochip,
struct irq_chip *irqchip,
unsigned int first_irq,
irq_flow_handler_t handler,
unsigned int type)
static inline int gpiochip_irqchip_add_nested(
struct gpio_chip *gpiochip,
struct irq_chip *irqchip,
unsigned int first_irq,
```
98 Leveraging the Regmap API and Simplifying the Code irq_flow_handler_t handler,
```c
unsigned int type)
```
In the preceding prototypes, the gpiochip parameter is the GPIO chip to add the irqchip to. irqchip is the IRQ chip to be added to the GPIO chip in order to extend its capabilities so that it can act as an IRQ controller as well. This IRQ chip has to be configured properly, either by the driver or by the IRQ core code (if dummy_irq_chip is given as a parameter). If it’s not dynamically assigned, first_irq will be the base (first)
IRQ to allocate GPIO chip IRQs from. handler is the primary IRQ handler to use (often one of the predefined high-level IRQ core functions). type is the default type for IRQs on this IRQ chip; pass IRQ_TYPE_NONE here and let the drivers configure this upon request.
A summary of each of these function actions is as follows:
• The first one allocates a struct irq_domain to the GPIO chip using the irq_
domain_add_simple() function. This IRQ domain's ops is set with the kernel
IRQ core domain ops variable called gpiochip_domain_ops. This domain ops is defined in drivers/gpio/gpiolib.c, with the irq_domain_ops.xlate field set to irq_domain_xlate_twocell, meaning that this gpio chip will handle two-celled IRQs.
```c
• Sets the gpiochip.to_irq field to gpiochip_to_irq, which is a callback that returns irq_create_mapping(chip->irq.domain, offset), creating an IRQ mapping that corresponds to the GPIO offset. This is performed when we invoke gpiod_to_irq() on that GPIO. This function assumes that each of the pins on the gpiochip can generate a unique IRQ. The following is how the gpiochip_domain_ops IRQ domain is defined:
static const struct irq_domain_ops gpiochip_domain_ops =
{
```
.map = gpiochip_irq_map,
.unmap = gpiochip_irq_unmap,
/* Virtually all GPIO-based IRQ chips are two-celled */
.xlate = irq_domain_xlate_twocell,
```c
};
```
Introduction to regmap and its data structures – I2C, SPI, and MMIO 99
The only difference between gpiochip_irqchip_add_nested() and gpiochip_
```c
irqchip_add() is that the former adds a nested IRQ chip to the GPIO chip (it sets the gpio_chip->irq.threaded field to true), while the later adds a chained IRQ
```
chip to a GPIO chip and sets this field to false. On the other hand, gpiochip_set_
chained_irqchip() and gpiochip_set_nested_irqchip() respectively assign/connect a chained or a nested IRQ chip to the GPIO chip. The following are the prototypes of those two functions:
```c
void gpiochip_set_chained_irqchip(
struct gpio_chip *gpiochip,
struct irq_chip *irqchip,
unsigned int parent_irq,
irq_flow_handler_t parent_handler)
void gpiochip_set_nested_irqchip(struct gpio_chip *gpiochip,
struct irq_chip *irqchip,
unsigned int parent_irq)
```
In the preceding prototypes, gpiochip is the GPIO chip to set the irqchip chain to.
irqchip represents the IRQ chip to chain to the GPIO chip. parent_irq is the irq number corresponding to the parent IRQ for this chained IRQ chip. In other words, it is the IRQ number to which this chip is connected. parent_handler is the parent interrupt handler for the accumulated IRQ coming out of the GPIO chip. It is actually the hwirq handler. This is not used for nested IRQ chips, as the parent handler is threaded.
```c
The chained variant will internally call irq_set_chained_handler_and_data()
```
on parent_handler.
## Chained gpiochip-based IRQ chips gpiochip_irqchip_add() and gpiochip_set_chained_irqchip() are to be used on chained GPIO chip-based IRQ chips, while gpiochip_irqchip_add_
nested() and gpiochip_set_nested_irqchip() are used on nested GPIO
chip-based IRQ chips only. With chained GPIO chip-based IRQ chips, gpiochip_set_
chained_irqchip() will configure the parent hwirq’s handler. There’s no need to call any devm_request_* irq family function. However, the parent hwirq’s handler has to call generic_handle_irq() on the raised child irqs, as in the following example
(from drivers/pinctrl/pinctrl-at91.c in the kernel sources), somewhat similar to a standard chained IRQ chip:
```c
static void gpio_irq_handler(struct irq_desc *desc)
{
```
100 Leveraging the Regmap API and Simplifying the Code unsigned long isr;
```c
int n;
struct irq_chip *chip = irq_desc_get_chip(desc);
struct gpio_chip *gpio_chip =
irq_desc_get_handler_data(desc);
struct at91_gpio_chip *at91_gpio =
gpiochip_get_data(gpio_chip);
void __iomem *pio = at91_gpio->regbase;
chained_irq_enter(chip, desc);
for (;;) {
```
isr = readl_relaxed(pio + PIO_ISR) &
```c
readl_relaxed(pio + PIO_IMR);
```
[...]
```c
for_each_set_bit(n, &isr, BITS_PER_LONG) {
generic_handle_irq(irq_find_mapping(
gpio_chip->irq.domain, n));
}
}
chained_irq_exit(chip, desc);
```
[...]
```c
}
```
In the preceding code, the interrupt handler is introduced first. Upon an interrupt issued by the GPIO chip, its whole gpio status bank is read in order to detect each bit that is set there, which would mean a potential IRQ triggered by the device behind the corresponding gpio line.
```c
generic_handle_irq() is then invoked on each irq descriptor whose index in the domain corresponds to the index of a bit set in the gpio status bank. This method in turn will invoke each handler registered for each descriptor found in the previous step in an atomic context (the hard-irq context), except if the underlying driver for the device for which the gpio is used as an irq line requested the handler to be threaded.
```
Now we can introduce the probe method, an example of which is as follows:
```c
static int at91_gpio_probe(struct platform_device *pdev)
{
```
Introduction to regmap and its data structures – I2C, SPI, and MMIO 101
[...]
```c
ret = gpiochip_irqchip_add(&at91_gpio->chip,
```
&gpio_irqchip,
0,
```c
handle_edge_irq,
```
IRQ_TYPE_NONE);
```c
if (ret) {
```
dev_err(
```c
&pdev->dev,
```
"at91_gpio.%d: Couldn’t add irqchip to gpiochip.\n",
```c
at91_gpio->pioc_idx);
return ret;
}
```
[...]
/* Then register the chain on the parent IRQ */
```c
gpiochip_set_chained_irqchip(&at91_gpio->chip,
```
&gpio_irqchip,
```c
at91_gpio->pioc_virq,
```
gpio_irq_handler);
```c
return 0;
}
```
There’s nothing special there. The mechanism here somehow follows what we have seen in the generic IRQ chips. The parent IRQ is not requested here using any of the request_
irq() family methods because gpiochip_set_chained_irqchip() will invoke irq_set_chained_handler_and_data() under the hood.
## Nested gpiochip-based irqchips
The following excerpt shows how nested GPIO chip-based IRQ chips are registered by their drivers. This is somewhat similar to standalone nested IRQ chips:
```c
static irqreturn_t pcf857x_irq(int irq, void *data)
{
struct pcf857x *gpio = data;
unsigned long change, i, status;
status = gpio->read(gpio->client);
```
/*
102 Leveraging the Regmap API and Simplifying the Code
* call the interrupt handler if gpio is used as
* interrupt source, just to avoid bad irqs
*/
```c
mutex_lock(&gpio->lock);
change = (gpio->status ^ status) & gpio->irq_enabled;
gpio->status = status;
mutex_unlock(&gpio->lock);
for_each_set_bit(i, &change, gpio->chip.ngpio)
handle_nested_irq(
irq_find_mapping(gpio->chip.irq.domain, i));
return IRQ_HANDLED;
}
```
The preceding code is the IRQ handler. As we can see, it uses handle_nested_irq(),
which is nothing new for us. Let’s now inspect the probe method:
```c
static int pcf857x_probe(struct i2c_client *client,
```
const struct i2c_device_id *id)
```c
{
struct pcf857x *gpio;
```
[...]
/* Enable irqchip only if we have an interrupt line */
```c
if (client->irq) {
status = gpiochip_irqchip_add_nested(&gpio->chip,
&gpio->irqchip,
```
0,
```c
handle_level_irq,
```
IRQ_TYPE_NONE);
```c
if (status) {
dev_err(&client->dev, "cannot add irqchip\n");
```
goto fail;
```c
}
```
status = devm_request_threaded_irq(
```c
&client->dev, client->irq,
```
NULL, pcf857x_irq,
IRQF_ONESHOT |IRQF_TRIGGER_FALLING |
IRQF_SHARED,
Introduction to regmap and its data structures – I2C, SPI, and MMIO 103
```c
dev_name(&client->dev), gpio);
if (status)
```
goto fail;
```c
gpiochip_set_nested_irqchip(&gpio->chip,
&gpio->irqchip,
client->irq);
}
```
[...]
```c
}
```
Here, the parent irq handler is threaded and has to be registered using devm_request_
threaded_irq(). This explains why its IRQ handler has to call handle_nested_
irq() on child irqs in order to invoke their handlers. Once more, this looks like generic nested irqchips, except for the fact that gpiolib has wrapped some of the underlying nested irqchip APIs. To confirm this, you can have a look into the body of the gpiochip_set_nested_irqchip() and gpiochip_irqchip_add_nested()
methods.
## Regmap IRQ API and data structures
The regmap IRQ API is implemented in drivers/base/regmap/regmap-irq.c. It is mainly built on top of two essential functions, devm_regmap_add_irq_chip() and regmap_irq_get_virq(), and three data structures, struct regmap_irq_chip,
```c
struct regmap_irq_chip_data, and struct regmap_irq.
```
Important note
Regmap’s irqchip API entirely uses threaded IRQs. Thus, only what we have seen in the Nested interrupts section will apply here.
Regmap IRQ data structures
As mentioned earlier, we need to introduce the three data structures of the regmap irq api in order to understand how it abstracts IRQ management.
104 Leveraging the Regmap API and Simplifying the Code
## struct regmap_irq_chip and struct regmap_irq
The struct regmap_irq_chip structure describes a generic regmap irq_chip.
Prior to discussing this structure, let’s first introduce struct regmap_irq, which stores the register and the mask description of an IRQ for regmap irq_chip:
```c
struct regmap_irq {
unsigned int reg_offset;
unsigned int mask;
unsigned int type_reg_offset;
unsigned int type_rising_mask;
unsigned int type_falling_mask;
};
```
The following are descriptions of the fields in the preceding structure:
• reg_offset is the offset of the status/mask register within the bank. This bank may actually be the {status/mask/unmask/ack/wake}_base register of the
IRQ chip.
• mask is the mask used to flag/control this IRQ status register. When disabling the
IRQ, the mask value will be ORed with the actual content of reg_offset from the regmap’s irq_chip.status_base register. For irq enabling, ~mask will be
ANDed.
• type_reg_offset is the offset register (from the irqchip status base register)
for the IRQ type setting.
• type_rising_mask is the mask bit to configure rising type IRQs. This value will be ORed with the actual content of type_reg_offset when setting the type of the IRQ to IRQ_TYPE_EDGE_RISING.
• type_falling_mask is the mask bit to configure falling type IRQs. This value will be ORed with the actual content of type_reg_offset when setting the type of the IRQ to IRQ_TYPE_EDGE_FALLING. For the IRQ_TYPE_EDGE_BOTH type,
```c
(type_falling_mask | irq_data->type_rising_mask) will be used as a mask.
```
Now that we are familiar with struct regmap_irq, let’s describe struct regmap_
```c
irq_chip, the structure of which looks as follows:
struct regmap_irq_chip {
```
const char *name;
Introduction to regmap and its data structures – I2C, SPI, and MMIO 105
```c
unsigned int status_base;
unsigned int mask_base;
unsigned int unmask_base;
unsigned int ack_base;
unsigned int wake_base;
unsigned int type_base;
unsigned int irq_reg_stride;
bool mask_writeonly:1;
bool init_ack_masked:1;
bool mask_invert:1;
bool use_ack:1;
bool ack_invert:1;
bool wake_invert:1;
bool type_invert:1;
int num_regs;
```
const struct regmap_irq *irqs;
```c
int num_irqs;
int num_type_reg;
unsigned int type_reg_stride;
int (*handle_pre_irq)(void *irq_drv_data);
int (*handle_post_irq)(void *irq_drv_data);
void *irq_drv_data;
};
```
This structure describes a generic regmap_irq_chip, which can handle most interrupt controllers (not all of them, as we will see later). The following list describes the fields in this data structure:
• name is a descriptive name for the IRQ controller.
• status_base is the base status register address to which the regmap IRQ core adds regmap_irq.reg_offset prior to obtaining the final status register for the given regmap_irq.
106 Leveraging the Regmap API and Simplifying the Code
• mask_writeonly states whether the base mask register is write-only or not.
If yes, regmap_write_bits() is used to write into the register, otherwise regmap_update_bits() is used.
• unmask_base is the base unmask register address, which has to be specified for chips that have separate mask and unmask registers.
• ack_base is the acknowledgement base register address. Using a value of 0 is possible with the use_ack bit.
• wake_base is the base address for wake enable, used to control the irq power management wakeups. If the value is 0, it means this is unsupported.
• type_base is the base address for the IRQ type to which the regmap IRQ core adds regmap_irq.type_reg_offset prior to obtaining the final type register for the given regmap_irq. If it is 0, it means this is unsupported.
• irq_reg_stride is the stride to use for chips where registers are not contiguous.
• init_ack_masked states whether the regmap IRQ core should acknowledge all masked interrupts once during initialization.
• mask_invert, if true, means the mask register is inverted. It means cleared bit indexes correspond to masked out interrupts.
• use_ack, if true, means the acknowledgement register should be used even if it is
0.
• ack_invert, if true, means the acknowledgement register is inverted:
corresponding bit is cleared for a acknowledge.
• wake_invert, if true, means the wake register is inverted: cleared bits correspond to wake enabled.
• type_invert, if true, means inverted type flags are used.
• num_regs is the number of registers in each control bank. The number of registers to read when using regmap_bulk_read() will be given. Have a look at the definition of regmap_irq_thread() for more information.
• irqs is an array of descriptors for individual IRQs, and num_irqs is the total number of descriptors in the array. Interrupt numbers are assigned based on the index in this array.
Introduction to regmap and its data structures – I2C, SPI, and MMIO 107
• num_type_reg is the number of type registers, while type_reg_stride is the stride to use for chips where type registers are not contiguous. Regmap IRQ
implements the generic interrupt service routine, which is common for most devices.
• Some devices, such as MAX77620 or MAX20024, need special handling before and after servicing the interrupt. This is where handle_pre_irq and handle_
post_irq come in. These are driver-specific callbacks to handle interrupts from devices before regmap_irq_handler processes the interrupts. irq_drv_data is then the data that is passed as a parameter to those pre-/post-interrupt handlers.
For example, the MAX77620 programming guidelines for interrupt servicing says the following:
--When interrupt occurs from PMIC, mask the PMIC interrupt by setting GLBLM.
--Read IRQTOP and service the interrupt accordingly.
--Once all interrupts have been checked and serviced, the interrupt service routine un-masks the hardware interrupt line by clearing GLBLM.
Back to the regmap_irq_chip.irqs field, this field is of the regmap_irq type,
introduced earlier.
## struct regmap_irq_chip_data
This structure is the runtime data structure for the regmap IRQ controller, allocated on the successful return path of devm_regmap_add_irq_chip(). It has to be stored in a large and private data structure for later use. Its definition is as follows:
```c
struct regmap_irq_chip_data {
struct mutex lock;
struct irq_chip irq_chip;
struct regmap *map;
```
const struct regmap_irq_chip *chip;
```c
int irq_base;
struct irq_domain *domain;
int irq;
```
[...]
```c
};
```
108 Leveraging the Regmap API and Simplifying the Code
For simplicity, some fields in the structure have been removed. Here is a description of the fields in this structure:
• lock is the lock used to protect accesses to the irq_chip to which regmap_
```c
irq_chip_data belongs. As regmap IRQs are totally threaded, it is safe to use a mutex.
```
• irq_chip is the underlying interrupt chip descriptor structure (providing
IRQ-related operations) for this regmap-enabled irqchip, set with regmap_
```c
irq_chip, defined as follows in drivers/base/regmap/regmap-irq.c:
static const struct irq_chip regmap_irq_chip = {
```
.irq_bus_lock = regmap_irq_lock,
.irq_bus_sync_unlock = regmap_irq_sync_unlock,
.irq_disable = regmap_irq_disable,
.irq_enable = regmap_irq_enable,
.irq_set_type = regmap_irq_set_type,
.irq_set_wake = regmap_irq_set_wake,
```c
};
```
• map is the regmap structure for the aforementioned irq_chip.
• chip is a pointer to the generic regmap irq_chip, which should have been set up in the driver. It is given as a parameter to devm_regmap_add_irq_chip().
• base, if more than zero, is the base from which it allocates a specific IRQ number.
In other words, the numbering of IRQ starts at base.
Introduction to regmap and its data structures – I2C, SPI, and MMIO 109
• domain is the IRQ domain for the underlying IRQ chip, with ops set to regmap_
domain_ops, defined as follows:
```c
static const struct irq_domain_ops regmap_domain_ops = {
```
.map = regmap_irq_map,
.xlate = irq_domain_xlate_onetwocell,
```c
};
```
• irq is the parent (base) IRQ for irq_chip. It corresponds to the irq parameter given to devm_regmap_add_irq_chip().
## Regmap IRQ API
Earlier in the chapter, we introduced both devm_regmap_add_irq_chip() and regmap_irq_get_virq() as two essential functions the regmap IRQ API is made of.
These are actually the most important functions for regmap IRQ management and the following are their respective prototypes:
```c
int devm_regmap_add_irq_chip(struct device *dev,
struct regmap *map,
int irq, int irq_flags,
int irq_base,
```
const struct regmap_irq_chip *chip,
```c
struct regmap_irq_chip_data **data)
int regmap_irq_get_virq(struct regmap_irq_chip_data *data,
int irq)
```
In the preceding code, dev is the device pointer to which irq_chip belongs. map is a valid and initialized regmap for the device. irq_base, if more than zero, will be the number of the first allocated IRQ. chip is the configuration for the interrupt controller.
In the prototype of regmap_irq_get_virq(), *data is an initialized input parameter that must have been returned by devm_regmap_add_irq_chip() through **data.
110 Leveraging the Regmap API and Simplifying the Code devm_regmap_add_irq_chip() is the function you should use to add regmap-based irqchip support in the code. Its data parameter is an output argument that represents the runtime data structure for the controller, allocated at the success of this function call. Its irq argument is the parent and primary IRQ for the irqchip. It is the IRQ the device uses to signal interrupts, while irq_flags is a mask of IRQF_ flags to use for this primary interrupt. If this function succeeds (that is, returns 0), then output data will be set with a fresh allocated and well-configured structure of type regmap_irq_chip_data. This function returns errno on failure. devm_regmap_add_irq_chip() is a combination of the following:
• Allocating and initializing struct regmap_irq_chip_data.
• irq_domain_add_linear() (if irq_base == 0), which allocates an IRQ
domain given the number of IRQs needed in the domain. On success, the IRQ
domain will be assigned to the .domain field of the previously allocated IRQ chip’s data. This domain’s ops.map function will configure each IRQ child as nested into the parent thread, and ops.xlate will be set to irq_domain_xlate_
onetwocell. If irq_base > 0, irq_domain_add_legacy() is used instead of irq_domain_add_linear().
• request_threaded_irq(), in order to register the parent IRQ thread handler.
Regmap uses its own defined threaded handler, regmap_irq_thread(), which does some hacks prior to calling handle_nested_irq() on the child irqs.
The following is an excerpt that summarizes the preceding actions:
```c
static int regmap_irq_map(struct irq_domain *h,
unsigned int virq,
irq_hw_number_t hw)
{
struct regmap_irq_chip_data *data = h->host_data;
irq_set_chip_data(virq, data);
irq_set_chip(virq, &data->irq_chip);
irq_set_nested_thread(virq, 1);
irq_set_parent(virq, data->irq);
irq_set_noprobe(virq);
return 0;
}
static const struct irq_domain_ops regmap_domain_ops = {
```
.map = regmap_irq_map,
Introduction to regmap and its data structures – I2C, SPI, and MMIO 111
.xlate = irq_domain_xlate_onetwocell,
```c
};
static irqreturn_t regmap_irq_thread(int irq, void *d)
{
```
[...]
```c
for (i = 0; i < chip->num_irqs; i++) {
if (data->status_buf[chip->irqs[i].reg_offset /
map->reg_stride] & chip->irqs[i].mask) {
handle_nested_irq(irq_find_mapping(data->domain,
```
i));
handled = true;
```c
}
}
```
[...]
```c
if (handled)
return IRQ_HANDLED;
```
else return IRQ_NONE;
```c
}
int regmap_add_irq_chip(struct regmap *map, int irq,
int irq_ flags,
int irq_base,
```
const struct regmap_irq_chip *chip,
```c
struct regmap_irq_chip_data **data)
{
struct regmap_irq_chip_data *d;
```
[...]
d = kzalloc(sizeof(*d), GFP_KERNEL);
```c
if (!d)
return -ENOMEM;
```
/* The below is just for simplicity */
```c
initialize_irq_chip_data(d);
```
112 Leveraging the Regmap API and Simplifying the Code if (irq_base)
```c
d->domain = irq_domain_add_legacy(map->dev->of_node,
chip->num_irqs,
irq_base, 0,
```
&regmap_domain_ops,
d);
```c
else d->domain = irq_domain_add_linear(map->dev->of_node,
chip->num_irqs,
```
&regmap_domain_ops,
d);
ret = request_threaded_irq(irq, NULL, regmap_irq_thread,
```c
irq_flags | IRQF_ONESHOT,
chip->name, d);
```
[...]
*data = d;
```c
return 0;
}
regmap_irq_get_virq() maps an interrupt on a chip to a virtual IRQ. It simply returns irq_create_mapping(data->domain, irq) on the given irq and domain, as we saw earlier. Its irq parameter is the index of the interrupt requested in the chip IRQs.
```
## Regmap IRQ API example
Let’s use the max7760 GPIO controller’s driver to see how the concepts behind the regmap IRQ API are applied. This driver is located at drivers/gpio/gpiomax77620.c in the kernel source, and the following is a simplified excerpt of the way this driver uses regmap to handle IRQ management.
Let’s start by defining the data structure that will be used throughout the writing of the code:
```c
struct max77620_gpio {
struct gpio_chip gpio_chip;
struct regmap *rmap;
```
Introduction to regmap and its data structures – I2C, SPI, and MMIO 113
```c
struct device *dev;
};
struct max77620_chip {
struct device *dev;
struct regmap *rmap;
int chip_irq;
int irq_base;
```
[...]
```c
struct regmap_irq_chip_data *top_irq_data;
struct regmap_irq_chip_data *gpio_irq_data;
};
```
The meaning of the preceding data structure will become clear when you go through the code. Next, let’s define our regmap IRQs array, as follows:
```c
static const struct regmap_irq max77620_gpio_irqs[] = {
[0] = {
```
.mask = MAX77620_IRQ_LVL2_GPIO_EDGE0,
.type_rising_mask = MAX77620_CNFG_GPIO_INT_RISING,
.type_falling_mask = MAX77620_CNFG_GPIO_INT_FALLING,
.reg_offset = 0,
.type_reg_offset = 0,
```c
},
[1] = {
```
.mask = MAX77620_IRQ_LVL2_GPIO_EDGE1,
.type_rising_mask = MAX77620_CNFG_GPIO_INT_RISING,
.type_falling_mask = MAX77620_CNFG_GPIO_INT_FALLING,
.reg_offset = 0,
.type_reg_offset = 1,
```c
},
[2] = {
```
.mask = MAX77620_IRQ_LVL2_GPIO_EDGE2,
.type_rising_mask = MAX77620_CNFG_GPIO_INT_RISING,
.type_falling_mask = MAX77620_CNFG_GPIO_INT_FALLING,
.reg_offset = 0,
.type_reg_offset = 2,
114 Leveraging the Regmap API and Simplifying the Code
```c
},
```
[...]
```c
[7] = {
```
.mask = MAX77620_IRQ_LVL2_GPIO_EDGE7,
.type_rising_mask = MAX77620_CNFG_GPIO_INT_RISING,
.type_falling_mask = MAX77620_CNFG_GPIO_INT_FALLING,
.reg_offset = 0,
.type_reg_offset = 7,
```c
},
};
```
You may have noticed the array has been truncated for the sake of readability. This array can then be assigned to the regmap_irq_chip data structure, as follows:
```c
static const struct regmap_irq_chip max77620_gpio_irq_chip = {
```
.name = "max77620-gpio",
.irqs = max77620_gpio_irqs,
.num_irqs = ARRAY_SIZE(max77620_gpio_irqs),
.num_regs = 1,
.num_type_reg = 8,
.irq_reg_stride = 1,
.type_reg_stride = 1,
.status_base = MAX77620_REG_IRQ_LVL2_GPIO,
.type_base = MAX77620_REG_GPIO0,
```c
};
```
To summarize the preceding excerpts, the driver fills an array (max77620_gpio_
irqs[] ) of regmap_irq and uses it to build a regmap_irq_chip structure
(max77620_gpio_irq_chip). Once the regmap_irq_chip data structure is ready,
we start writing an irqchip callback, as required by the kernel gpiochip core:
```c
static int max77620_gpio_to_irq(struct gpio_chip *gc,
unsigned int offset)
{
struct max77620_gpio *mgpio = gpiochip_get_data(gc);
struct max77620_chip *chip =
dev_get_drvdata(mgpio->dev- >parent);
return regmap_irq_get_virq(chip->gpio_irq_data, offset);
}
```
Introduction to regmap and its data structures – I2C, SPI, and MMIO 115
In the preceding snippet, we have only defined the callback that will be assigned to the
.to_irq field of the GPIO chip. Other callbacks can be found in the original driver.
Again, the code has been truncated here. At this stage, we can talk about the probe method, which will use all of the previously defined functions:
```c
static int max77620_gpio_probe(struct platform_device *pdev)
{
struct max77620_chip *chip =
dev_get_drvdata(pdev->dev.parent);
struct max77620_gpio *mgpio;
int gpio_irq;
int ret;
```
gpio_irq = platform_get_irq(pdev, 0);
[...]
```c
mgpio = devm_kzalloc(&pdev->dev, sizeof(*mgpio),
```
GFP_KERNEL);
```c
if (!mgpio)
return -ENOMEM;
mgpio->rmap = chip->rmap;
mgpio->dev = &pdev->dev;
```
/* setting gpiochip stuffs*/
```c
mgpio->gpio_chip.direction_input =
```
max77620_gpio_dir_input;
```c
mgpio->gpio_chip.get = max77620_gpio_get;
mgpio->gpio_chip.direction_output =
```
max77620_gpio_dir_output;
```c
mgpio->gpio_chip.set = max77620_gpio_set;
mgpio->gpio_chip.set_config = max77620_gpio_set_config;
mgpio->gpio_chip.to_irq = max77620_gpio_to_irq;
mgpio->gpio_chip.ngpio = MAX77620_GPIO_NR;
mgpio->gpio_chip.can_sleep = 1;
mgpio->gpio_chip.base = -1;
```
#ifdef CONFIG_OF_GPIO
```c
mgpio->gpio_chip.of_node = pdev->dev.parent->of_node;
```
#endif
```c
116 Leveraging the Regmap API and Simplifying the Code ret = devm_gpiochip_add_data(&pdev->dev,
&mgpio->gpio_chip, mgpio);
```
[...]
```c
ret = devm_regmap_add_irq_chip(&pdev->dev,
chip->rmap, gpio_irq,
```
IRQF_ONESHOT, -1,
&max77620_gpio_irq_chip,
```c
&chip->gpio_irq_data);
```
[...]
```c
return 0;
}
```
In this probe method excerpt (which has no error checks), max77620_gpio_irq_
chip is finally given to devm_regmap_add_irq_chip in order to populate the irqchip with IRQs and then add the IRQ chip to the regmap core. This function also sets chip-
>gpio_irq_data with a valid regmap_irq_chip_data structure, and chip is the private data structure allowing us to store this IRQ chip data for later use. Since this IRQ
controller is built on top of a GPIO controller (gpiochip), the gpio_chip.to_irq field had to be set, and here it is the max77620_gpio_to_irq callback. This callback simply returns the value returned by regmap_irq_get_virq(), which creates and returns a valid irq mapping in regmap_irq_chip_data.domain according to the offset given as a parameter. The other functions have already been introduced and are not new for us.
In this section, we introduced the entirety of IRQ management using regmap. You are ready to move your MMIO-based IRQ management to regmap.
## Summary
This chapter essentially dealt with regmap core. We introduced the framework, walked through its APIs, and described some use cases. Apart from register access, we have also learned how to use regmap for MMIO-based IRQ management. The next chapter, which deals with MFD devices and the syscon framework, will make intense use of the concepts learned in this chapter. By the end of this chapter, you should be able to develop regmapenabled IRQ controllers, and you won’t find yourself reinventing the wheel and leveraging this framework for register access