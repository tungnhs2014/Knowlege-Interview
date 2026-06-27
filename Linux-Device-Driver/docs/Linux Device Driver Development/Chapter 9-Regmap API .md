# Chapter 9 - Regmap API

A Register Map Abstraction Before the regmap API was developed, there was redundant code for the device drivers dealing with SPI core, I2C core, or both. The principle was the same: accessing the register for read/write operations. The following diagram shows how either the SPI or I2C API were standalone before regmap was introduced to the kernel:
SPI and I2C subsystems before regmap
The regmap API was introduced in version 3.1 of the kernel, to factorize and unify the way kernel developers access SPI/I2C devices. It is then just a matter of how to initialize and configure a regmap, and process any read/write/modify operation fluently, whether it is
SPI or I2C:
SPI and I2C subsystems after regmap
This chapter will walk through the regmap framework as follows:
Introducing the main data structures used in the regmap framework
Walking through regmap configuration
Accessing devices using the regmap API
Introducing the regmap caching system
Providing a complete driver that summarizes the concepts learned previously
## Programming with the regmap API
The regmap API is quite simple. There are only a few structures to know about. The two most important structures of this API are struct regmap_config, which represents the configuration of the regmap, and struct regmap, which is the regmap instance itself. All of the regmap data structures are defined in include/linux/regmap.h.
## regmap_config structure struct regmap_config stores the configuration of the regmap during the driver's lifetime. What you set here affects read/write operations. It is the most important structure in the regmap API. The source looks like this:
```dts
struct regmap_config {
c
const char *name;
int reg_bits;
int reg_stride;
int pad_bits;
int val_bits;
bool (*writeable_reg)(struct device *dev, unsigned int reg);
bool (*readable_reg)(struct device *dev, unsigned int reg);
bool (*volatile_reg)(struct device *dev, unsigned int reg);
bool (*precious_reg)(struct device *dev, unsigned int reg);
regmap_lock lock;
regmap_unlock unlock;
void *lock_arg;
int (*reg_read)(void *context, unsigned int reg, unsigned int *val);
int (*reg_write)(void *context, unsigned int reg, unsigned int val);
bool fast_io;
unsigned int max_register;
const struct regmap_access_table *wr_table;
const struct regmap_access_table *rd_table;
const struct regmap_access_table *volatile_table;
const struct regmap_access_table *precious_table;
const struct reg_default *reg_defaults;
unsigned int num_reg_defaults;
```
enum regcache_type cache_type;
```c
const void *reg_defaults_raw;
unsigned int num_reg_defaults_raw;
u8 read_flag_mask;
u8 write_flag_mask;
bool use_single_rw;
bool can_multi_write;
```
enum regmap_endian reg_format_endian;
enum regmap_endian val_format_endian;
```c
const struct regmap_range_cfg *ranges;
unsigned int num_ranges;
}
```
The preceding code can be explained as follows:
reg_bits: This mandatory field is the number of bits in a register's address.
val_bits: This represents the number of bits used to store a register's value. It is a mandatory field.
writeable_reg: This is an optional callback function. If provided, it is used by the regmap subsystem when a register needs to be written. Before writing into a register, this function is automatically called to check whether the register can be written to or not:
```c
static bool foo_writeable_register(struct device *dev,unsigned int reg)
dts
{
switch (reg) {
c
case 0x30 ... 0x38:
case 0x40 ... 0x45:
case 0x50 ... 0x57:
case 0x60 ... 0x6e:
case 0x70 ... 0x75:
case 0x80 ... 0x85:
case 0x90 ... 0x95:
case 0xa0 ... 0xa5:
case 0xb0 ... 0xb2:
return true;
```
default:
```c
return false;
}
}
```
readable_reg: This is the same as writeable_reg but for every register read operation.
volatile_reg: This is a callback function called every time a register needs to be read or written through the regmap cache. If the register is volatile, the function should return true. A direct read/write is then performed on the register. If false is returned, it means the register is cacheable. In this case, the cache will be used for a read operation, and the cache will be written to in the case of a write operation:
```c
static bool foo_volatile_register(struct device *dev,unsigned int reg)
dts
{
switch (reg) {
c
case 0x24 ... 0x29:
case 0xb6 ... 0xb8:
return true;
```
default:
```c
return false;
}
}
```
wr_table: Instead of providing a writeable_reg callback, you could provide a regmap_access_table, which is a structure holding a yes_range and a no_range field, both pointers to struct regmap_range. Any register that belongs to a yes_range entry is considered writable, and is considered not writable if it belongs to a no_range.
rd_table: This is same as wr_table, but for any read operation.
volatile_table: Instead of volatile_reg, you could provide volatile_table. The principle is then the same as wr_table or rd_table, but for the caching mechanism.
max_register: This is optional; it specifies the maximum valid register address,
upon which no operation is permitted.
reg_read: Your device may not support simple I2C/SPI read operations. You'll then have no choice but to write your own customized read function. reg_read should then point to that function. That said, most devices do not need that.
reg_write: This is the same as reg_read but for write operations.
I highly recommend you look at include/linux/regmap.h for more details on each element.
The following is a kind of initialization of regmap_config:
```dts
static const struct regmap_config regmap_config = {
```
.reg_bits = 8,
.val_bits = 8,
.max_register = LM3533_REG_MAX,
.readable_reg = lm3533_readable_register,
.volatile_reg = lm3533_volatile_register,
.precious_reg = lm3533_precious_register,
```c
};
```
## regmap initialization
As we said earlier, the regmap API supports both SPI and I2C protocols. Depending on the protocol you need to support in your driver, you will have to call either regmap_init_i2c() or regmap_init_spi() in the probe function. To write a generic driver, regmap is the best choice.
The regmap API is generic and homogenous. Only the initialization changes between bus types. Other functions are the same.
It is a good practice always to initialize the regmap in the probe function,
and you must always fill the regmap_config elements prior to initializing the regmap.
Whether you allocated an I2C or SPI register map, it is freed with the regmap_exit function:
```c
void regmap_exit(struct regmap *map)
```
This function simply releases a previously allocated register map.
## SPI initialization
Regmap SPI initialization consists of setting the regmap up, so that any device access will internally be translated into SPI commands. The function that does this is regmap_init_spi().
```c
struct regmap * regmap_init_spi(struct spi_device *spi,
const struct regmap_config);
```
It takes a valid pointer to a struct spi_device structure as a parameter, which is the SPI
device that will be interacted with, and a struct regmap_config that represents the configuration for the regmap. This function returns either a pointer to the allocated struct regmap on success, or a value that will be an ERR_PTR() on error.
A full example is as follows:
```c
static int foo_spi_probe(struct spi_device *client)
dts
{
c
int err;
struct regmap *my_regmap;
struct regmap_config bmp085_regmap_config;
```
/* fill bmp085_regmap_config somewhere */
[...]
client->bits_per_word = 8;
my_regmap =
```c
regmap_init_spi(client,&bmp085_regmap_config);
dts
if (IS_ERR(my_regmap)) {
```
err = PTR_ERR(my_regmap);
```c
dev_err(&client->dev, "Failed to init regmap: %d\n", err);
return err;
}
```
[...]
```c
}
```
## I2C initialization
On the other hand, I2C regmap initialization consists of calling regmap_init_i2c() on the regmap config, which will configure the regmap so that any device access will internally be translated into I2C commands:
```c
struct regmap * regmap_init_i2c(struct i2c_client *i2c,
const struct regmap_config);
```
The function takes a struct i2c_client structure as parameter, which is the I2C device that will be used for interaction, along with a pointer to struct regmap_config, which represents the configuration for the regmap. This function returns either a pointer to the allocated struct regmap on success, or a value that will be an ERR_PTR() on error.
A full example is:
```c
static int bar_i2c_probe(struct i2c_client *i2c,
const struct i2c_device_id *id)
dts
{
c
struct my_struct * bar_struct;
struct regmap_config regmap_cfg;
```
/* fill regmap_cfgsome where */
[...]
bar_struct = kzalloc(&i2c->dev,
sizeof(*my_struct), GFP_KERNEL);
```c
if (!bar_struct)
return -ENOMEM;
i2c_set_clientdata(i2c, bar_struct);
```
bar_struct->regmap = regmap_init_i2c(i2c,
&regmap_config);
```c
if (IS_ERR(bar_struct->regmap))
return PTR_ERR(bar_struct->regmap);
```
bar_struct->dev = &i2c->dev;
bar_struct->irq = i2c->irq;
[...]
```c
}
```
## Device access functions
The API handles data parsing, formatting, and transmission. In most cases, device access is performed with regmap_read, regmap_write, and regmap_update_bits. These are the three most important functions you should always remember when it comes to storing/fetching data into/from the device. Their respective prototypes are:
```c
int regmap_read(struct regmap *map, unsigned int reg,
unsigned int *val);
int regmap_write(struct regmap *map, unsigned int reg,
unsigned int val);
int regmap_update_bits(struct regmap *map, unsigned int reg,
unsigned int mask, unsigned int val);
regmap_write: This writes data to the device. If max_register is set in regmap_config, then it will be used to check if the register address is valid. If the register address passed is lower than or equal to max_register, then the write operation will be performed; otherwise, the regmap core will return an invalid I/O error (-EIO). Immediately after, the writeable_reg callback is called. The callback must return true before going on to the next step. If it returns false, then -EIO is returned and the write operation is stopped. If wr_table is set instead of writeable_reg, then:
```
If the register address lies in no_range, -EIO is returned.
If the register address lies in yes_range, the next step is performed.
If the register address is present neither in yes_range nor no_range, then -EIO is returned and the operation is terminated.
If cache_type != REGCACHE_NONE, then the cache is enabled. In this case, the cache entry is first updated, and then a write to the hardware is performed; otherwise, a no cache action is performed.
If reg_write callback is provided, which is used to perform the write operation; otherwise, the generic regmap write function will be executed.
```c
regmap_read: This reads data from the device. It works exactly like regmap_write with appropriate data structures (readable_reg and rd_table). Therefore, if provided, reg_read is used to perform the read operation; otherwise, the generic remap read function will be performed.
```
## regmap_update_bits function regmap_update_bits is a three-in-one function. Its prototype is as follows:
```c
int regmap_update_bits(struct regmap *map, unsigned int reg,
unsigned int mask, unsigned int val)
```
It performs a read/modify/write cycle on the register map. It is a wrapper of
_regmap_update_bits, which looks as follows:
```c
static int _regmap_update_bits(struct regmap *map,
unsigned int reg, unsigned int mask,
unsigned int val, bool *change)
dts
{
c
int ret;
unsigned int tmp, orig;
```
ret = _regmap_read(map, reg, &orig);
```c
if (ret != 0)
return ret;
```
tmp = orig& ~mask;
tmp |= val & mask;
```dts
if (tmp != orig) {
```
ret = _regmap_write(map, reg, tmp);
*change = true;
```dts
} else {
```
*change = false;
```c
}
return ret;
}
```
This way, bits you need to update must be set to 1 in mask, and the corresponding bits should be set to the value you need to give to them in val.
```c
As an example, to set the first and third bits to 1, mask should be 0b00000101, and the value should be 0bxxxxx1x1. To clear the seventh bit, mask must be 0b01000000 and the value should be 0bx0xxxxxx, and so on.
```
## Special regmap_multi_reg_write function
The purpose of the regmap_multi_reg_write() function is writing multiple registers to the device. Its prototype looks as follows:
```c
int regmap_multi_reg_write(struct regmap *map,
const struct reg_sequence *regs, int num_regs)
```
To see how to use that function, you need to know what struct reg_sequence is:
/**
* Register/value pairs for sequences of writes with an optional delay in
* microseconds to be applied after each write.
*
* @reg: Register address.
* @def: Register value.
* @delay_us: Delay to be applied after the register write in microseconds
*/
```dts
struct reg_sequence {
c
unsigned int reg;
unsigned int def;
unsigned int delay_us;
};
```
And this is how it is used:
```dts
static const struct reg_sequence foo_default_regs[] = {
c
{ FOO_REG1, 0xB8 },
{ BAR_REG1, 0x00 },
{ FOO_BAR_REG1, 0x10 },
{ REG_INIT, 0x00 },
{ REG_POWER, 0x00 },
{ REG_BLABLA, 0x00 },
};
static int probe ( ...)
dts
{
```
[...]
ret = regmap_multi_reg_write(my_regmap, foo_default_regs,
```c
ARRAY_SIZE(foo_default_regs));
```
[...]
```c
}
```
## Other device access functions regmap_bulk_read() and regmap_bulk_write() are used to read/write multiple registers from/to the device. Use them with large blocks of data:
```c
int regmap_bulk_read(struct regmap *map, unsigned int reg, void
```
*val, size_tval_count);
```c
int regmap_bulk_write(struct regmap *map, unsigned int reg,
const void *val, size_t val_count);
```
Feel free to look into the regmap header file in the kernel source to see what choices you have.
## regmap and cache
Obviously, regmap supports caching. Whether the cache system is used or not depends on the value of the cache_type field in regmap_config. Looking at include/linux/regmap.h, accepted values are:
/* Anenum of all the supported cache types */
```dts
enum regcache_type {
```
REGCACHE_NONE,
REGCACHE_RBTREE,
REGCACHE_COMPRESSED,
REGCACHE_FLAT,
```c
};
```
It is set to REGCACHE_NONE by default, meaning that the cache is disabled. Other values simply define how the cache should be stored.
Your device may have a predefined power-on-reset value in certain registers. Those values can be stored in an array, so that any read operation returns the value contained in the array. However, any write operation affects the real register in the device and updates the content in the array. It is a kind of a cache that we can use to speed up access to the device.
That array is reg_defaults. Its structure looks like this in the source:
/**
* Default value for a register. We use an array of structs rather
* than a simple array as many modern devices have very sparse
* register maps.
*
* @reg: Register address.
* @def: Register default value.
*/
```dts
struct reg_default {
c
unsigned int reg;
unsigned int def;
};
```
reg_defaults is ignored if cache_type is set to none. If no default_reg is set but you still enable the cache, the corresponding cache structure will be created for you.
It is quite simple to use. Just declare it and pass it as a parameter to the regmap_config structure. Let's have a look at the LTC3589 regulator driver in drivers/regulator/ltc3589.c:
```dts
static const struct reg_default ltc3589_reg_defaults[] = {
c
{ LTC3589_SCR1, 0x00 },
{ LTC3589_OVEN, 0x00 },
{ LTC3589_SCR2, 0x00 },
{ LTC3589_VCCR, 0x00 },
{ LTC3589_B1DTV1, 0x19 },
{ LTC3589_B1DTV2, 0x19 },
{ LTC3589_VRRCR, 0xff },
{ LTC3589_B2DTV1, 0x19 },
{ LTC3589_B2DTV2, 0x19 },
{ LTC3589_B3DTV1, 0x19 },
{ LTC3589_B3DTV2, 0x19 },
{ LTC3589_L2DTV1, 0x19 },
{ LTC3589_L2DTV2, 0x19 },
};
dts
static const struct regmap_config ltc3589_regmap_config = {
```
.reg_bits = 8,
.val_bits = 8,
.writeable_reg = ltc3589_writeable_reg,
.readable_reg = ltc3589_readable_reg,
.volatile_reg = ltc3589_volatile_reg,
.max_register = LTC3589_L2DTV2,
.reg_defaults = ltc3589_reg_defaults,
.num_reg_defaults = ARRAY_SIZE(ltc3589_reg_defaults),
.use_single_rw = true,
.cache_type = REGCACHE_RBTREE,
```c
};
```
Any read operation on any one of the registers present in the array will immediately return the value in the array. However, a write operation will be performed on the device itself,
and updates the affected register in the array. This way, reading the LTC3589_VRRCR
register will return 0xff; write any value in that register and it will update its entry in the array so that any new read operation will return the last written value, directly from the cache.
## Putting it all together
Perform the following steps to set up a regmap subsystem:
1. Set up a struct regmap_config, according to your device's characteristic. Set a register range if needed, default values if any, the cache_type if needed, and so on. If custom read/write functions are needed, pass them to the reg_read/reg_write fields.
2. In the probe function, allocate a regmap using regmap_init_i2c or regmap_init_spi depending on the bus: I2C or SPI.
3. Whenever you need to read/write from/into registers, call the regmap_[read|write] functions.
4. When you are done with the regmap, call regmap_exit to free the register map allocated in probe.
## A regmap example
To achieve our goal, let's first describe a fake SPI device for which we can write a driver:
8-bit register address
8-bit register values
Max register: 0x80
The write mask is 0x80
Valid address range:
0x20 to 0x4F
0x60 to 0x7F
No custom read/write function needed
The following is a fake skeleton:
/* mandatory for regmap */
```c
#include <linux/regmap.h>
```
/* Depending on your need you should include other files */
```c
static struct private_struct
dts
{
```
/* Feel free to add whatever you want here */
```c
struct regmap *map;
int foo;
};
static const struct regmap_range wr_rd_range[] =
dts
{
{
```
.range_min = 0x20,
.range_max = 0x4F,
```dts
},{
```
.range_min = 0x60,
.range_max = 0x7F
```c
},
};
struct regmap_access_table drv_wr_table =
dts
{
```
.yes_ranges = wr_rd_range,
.n_yes_ranges = ARRAY_SIZE(wr_rd_range),
```c
};
struct regmap_access_table drv_rd_table =
dts
{
```
.yes_ranges = wr_rd_range,
.n_yes_ranges = ARRAY_SIZE(wr_rd_range),
```c
};
static bool writeable_reg(struct device *dev, unsigned int reg)
dts
{
c
if (reg>= 0x20 &&reg<= 0x4F)
return true;
if (reg>= 0x60 &&reg<= 0x7F)
return true;
return false;
}
static bool readable_reg(struct device *dev, unsigned int reg)
dts
{
c
if (reg>= 0x20 &&reg<= 0x4F)
return true;
if (reg>= 0x60 &&reg<= 0x7F)
return true;
return false;
}
static int my_spi_drv_probe(struct spi_device *dev)
dts
{
c
struct regmap_config config;
struct custom_drv_private_struct *priv;
unsigned char data;
```
/* setup the regmap configuration */
```c
memset(&config, 0, sizeof(config));
```
config.reg_bits = 8;
config.val_bits = 8;
config.write_flag_mask = 0x80;
config.max_register = 0x80;
config.fast_io = true;
config.writeable_reg = drv_writeable_reg;
config.readable_reg = drv_readable_reg;
/*
* If writeable_reg and readable_reg are set,
* there is no need to provide wr_table nor rd_table.
* Uncomment below code only if you do not want to use
* writeable_reg nor readable_reg.
*/
//config.wr_table = drv_wr_table;
//config.rd_table = drv_rd_table;
/* allocate the private data structures */
/* priv = kzalloc */
/* Init the regmap spi configuration */
priv->map = regmap_init_spi(dev, &config);
/* Use regmap_init_i2c in case of i2c bus */
/*
* Let us write into some register
* Keep in mind that, below operation will remain same
* whether you use SPI or I2C. It is and advantage when
* you use regmap.
*/
```c
regmap_read(priv->map, 0x30, &data);
```
[...] /* Process data */
data = 0x24;
```c
regmap_write(priv->map, 0x23, data); /* write new value */
```
/* set bit 2 (starting from 0) and 6 of register 0x44 */
```c
regmap_update_bits(priv->map, 0x44, 0b00100010, 0xFF);
```
[...] /* Lot of stuff */
```c
return 0;
}
```
## Summary
This chapter was all about the regmap API. How easy it is gives you an idea of how useful and widely used it is. This chapter told you everything you need to know about the regmap
API. Now, you should be able to convert any standard SPI/I2C driver into a regmap. The next chapter will cover IIO devices, a framework for an analog-to-digital converter. Those kinds of device always sit on top of the SPI/I2C buses. It will be a challenge for us, at the end of the next chapter, to write an IIO driver using the regmap API